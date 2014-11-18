/*
    Copyright (C) 2002, 2003, 2004 Peter Wiesneth

	This file is part from NDB ("no Double Blocks"), an application
	suite for OS/2, Win32 and Linux to generate a packed archive file
	with subsequent full backups which need much lesser size as usual.

    NDB is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ndb.h"
#include "ndb_prototypes.h"



static void *blocktable[NDB_SIZE_BLOCKTABLE];
ULONG  blockCount          = 0;
ULONG  blocksSaved         = 0;

static int currBlockSize   = 0;
static int currCompression = 0;


/*
    *************************************************************************************
    ndb_block_datachunk_init()

	fills an empty chunk structure with the given values

    struct ndb_s_c_block *pBlockDataChunk: (INOUT) pointer to structure of data chunk
    ULONG magic:                           (IN)    magic value to identify the chunk type
    ULONG dataSize:                        (IN)    size of data following the chunk
    ULONG dataCount:                       (IN)    number of data elements
    *************************************************************************************
*/
void ndb_block_datachunk_init(struct ndb_s_c_block *pBlockDataChunk, ULONG magic,
                              ULONG dataSize, ULONG dataCount)
{
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_datachunk_init - startup\n");
	fflush(stdout);

    pBlockDataChunk->magic      = magic;
    pBlockDataChunk->dataSize   = dataSize;      /* size of block data without header chunk */
    pBlockDataChunk->dataCount  = dataCount;     /* number of blocks saved here */
    pBlockDataChunk->flags1     = 0;             /* for future extensions */
    pBlockDataChunk->flags2     = 0;             /* for future extensions */
    pBlockDataChunk->ownFilePosition = 0;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_datachunk_init - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_block_setBlockType()

	fills an block header structure with block type and block info flags

    struct ndb_s_blockheader *pBlockHeader:(INOUT) pointer to structure of block header
    char type:                             (IN)    block is: first/last/data/extra data?
    char info:                             (IN)    block is: data/EAs/...
    *************************************************************************************
*/
void ndb_block_setBlockType(struct ndb_s_blockheader *pBlockHeader, char type, char info)
{
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_setBlockType - startup\n");
    fflush(stdout);

    pBlockHeader->blockType  = type;             /* First/Last/Middle//Data/Extra */
    pBlockHeader->blockInfo  = info;             /* Data/EAs/NTSEC */

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_setBlockType - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_block_setBlockType()

	inits the hashtable for the blocks and sets the block size and the compression
	level for zlib

    int blockSize:                         (IN)    file content is divided into <blocksize> bytes blocks
    int compression:                       (IN)    compression level for zlib function
    *************************************************************************************
*/
void ndb_block_init(int blockSize, int compression)
{
    int i;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_init - startup\n");
    fflush(stdout);

    for (i = 0; i < NDB_SIZE_BLOCKTABLE; i++)
    {
        blocktable[i] = NULL;
    }
    blockCount      = 0;
    blocksSaved     = 0;
    currBlockSize   = blockSize;
    currCompression = compression;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_init - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_block_printHashInfo()

	prints out statistics about the blocks hashtable
    *************************************************************************************
*/
void ndb_block_printHashInfo()
{
    int i, j, k, l;
    int count[101];
    struct ndb_s_blockheader *pBlock = NULL;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_printHashInfo - startup\n");
    fflush(stdout);

    // empty field which counts the list lengths
    for (i = 0; i < 101; i++)
    {
        count[i] = 0;
    }
    // set maximum length counter
    k = 0;
    // set block counter
    l = 0;

    // process the hash list
    for (i = 0; i < NDB_SIZE_BLOCKTABLE; i++)
    {
        pBlock = blocktable[i];
        j = 0;
        // count how long the list for every hash value is
        while (pBlock != NULL)
        {
            pBlock = pBlock->sameCRC;
            j++;
            l++;
        }
        // cut length at 100 and save to count field
        j = (j > 100) ? 100 : j;
        count[j] ++;
        k = (j > k)? j : k;
    }

    // show result of counting
    fprintf(stdout, "\nBlock Header Hashtable Infos:\n\n");
    fprintf(stdout, "Table has %d size and contains %d block header\n\n", NDB_SIZE_BLOCKTABLE, l);
    fprintf(stdout, "Length   Count\n");
    fprintf(stdout, "------   -----\n");
    for (i = 1; i < k + 1; i++)
    {
        fprintf(stdout, "   %3d: %6d\n", i, count[i]);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_block_printHashInfo - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_block_print()

	prints out the meta data of a block (= block header)

    struct ndb_s_blockheader *pHeader:     (IN)    pointer to block header
    *************************************************************************************
*/
void ndb_block_print(struct ndb_s_blockheader *pHeader)
{
    fprintf(stdout, "Block Header Info ========================================\n");
    fprintf(stdout, "block no                    : 0x%lX\n", pHeader->blockNo);
    fprintf(stdout, "file no                     : 0x%lX\n", pHeader->fileNo);
    fprintf(stdout, "data file no                : 0x%X\n", pHeader->dataFileNo);
    fprintf(stdout, "chapter no                  : 0x%x\n", pHeader->chapNo);
    fprintf(stdout, "crc32                       : 0x%lX\n", pHeader->crc32);
    fprintf(stdout, "length (unzipped)           : 0x%x\n", pHeader->lenOri);
    fprintf(stdout, "length (zipped)             : 0x%x\n", pHeader->lenZip);
    fprintf(stdout, "block info (f/l/e)          : 0x%x\n", pHeader->blockInfo);
    fprintf(stdout, "block type (d/e)            : 0x%x\n", pHeader->blockType);
    fprintf(stdout, "file start header           : 0x%lX\n", pHeader->staHea);
    fprintf(stdout, "file start data             : 0x%lX\n", pHeader->staDat);
    fprintf(stdout, "next block at               : 0x%lX\n", (ULONG) pHeader->nextBlock);
    fprintf(stdout, "next block with same crc    : 0x%lX\n", (ULONG) pHeader->sameCRC);
    fprintf(stdout, "==========================================================\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_block_dumpblock()

	prints out a hex dump of the given buffer

    char *buff:                            (IN)    pointer to buffer
    int iLen:                              (IN)    size of buffer data
    *************************************************************************************
*/
void ndb_block_dumpblock(char *buff, int iLen)
{
    int i, j;

    if (buff == NULL)
    {
        fprintf(stdout, "Block Dump: Error: couldn't dump a NULL block\n");
	    fflush(stdout);
        return;
    }

    for (i=0; i < iLen; i += 16 )
    {
        fprintf(stdout, "%6X:  ", i);
        for (j = i; j < i+16; j++ )
        {
            if( j < iLen)
            {
                fprintf(stdout, "%2X ", buff[j] & 0xFF);
            }
            else
            {
                fprintf(stdout, "   ");
            }
        }

        for (j = i; j < i+16; j++ )
        {
            if( j < iLen)
            {
                if (((buff[j] & 0xFF) < 32) || ((buff[j] & 0xFF) > 128))
                {
                    fprintf(stdout, ".");
                }
                else
                {
                    fprintf(stdout, "%c", buff[j] & 0xFF);
                }
            }
            else
            {
                fprintf(stdout, " ");
            }
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_block_addBlockToTable()

	checks whether the compressed block data is the same as of an already saved
	block, otherwise it adds it to the hashtable
	- get the CRC of the unzipped block data
	- use this crc as hash value in the hash table
	- do you find another block with this hash?
	- if same data pointer into archive file -> identical, exit
	- compare both (zipped) block data
	- if identical -> exit
	- if different, add it to hashtable

    struct ndb_s_blockheader *pBlock:      (IN)    pointer to block header
    *************************************************************************************
*/
int ndb_block_addBlockToTable(struct ndb_s_blockheader *pBlock)
{
    short bIsIdentical = 0;
    struct ndb_s_blockheader *pLastBlock = NULL;
    ULONG lCurrCRC = 0;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_block_addBlockToTable - startup\n");
	    fflush(stdout);
    }

    /* get lower 16 bits of CRC32, use it as index */
    lCurrCRC = (ULONG) (pBlock->crc32 & (NDB_SIZE_BLOCKTABLE - 1));
    // check index for violation of bounds
    if (lCurrCRC >= NDB_SIZE_BLOCKTABLE)
    {
        fprintf(stderr, "ndb_block_addBlockToTable: CRC index %lX out of range 0..%lX\n", lCurrCRC, (ULONG) NDB_SIZE_BLOCKTABLE);
        fflush(stderr);
        exit(NDB_MSG_NOTOK);
   	}

    // first test precondition
    if (pBlock == NULL)
    {
        fprintf(stdout, "Error adding block - block is null\n");
	    fflush(stdout);
        return bIsIdentical;
    }
    if (pBlock->sameCRC != NULL)
    {
        // if sameCRC is set to something then the block already was added to
        // the hash table
        fprintf(stdout, "Error adding block - block->sameCRC is not NULL\n");
	    fflush(stdout);
        return bIsIdentical;
    }

    if (blocktable[lCurrCRC] == NULL)
    {
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_block_addBlockToTable: F%lu/B%lu/D%u: first block with lo-CRC %lX\n",
                            pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo, lCurrCRC);
	    fflush(stdout);
        pBlock->sameCRC = NULL;
        blocktable[lCurrCRC] = pBlock;
        blockCount++;
        bIsIdentical = 0;
    }
    else
    {
        struct ndb_s_blockheader *pOldBlock = blocktable[lCurrCRC];
        do
        {
            if (ndb_getDebugLevel() >= 6)
                fprintf(stdout, "ndb_block_addBlockToTable: F%lu/B%lu/D%u: found other block F%lu/B%lu/D%u with same lo-word of CRC %lX\n",
                                pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo, pOldBlock->fileNo, pOldBlock->blockNo, pOldBlock->dataFileNo, lCurrCRC);

		    fflush(stdout);
            // check if both blocks with same data
            // first check the complete CRC and sizes of uncompressed and compressed data
            if ((pBlock->crc32 == pOldBlock->crc32) && (pBlock->lenOri == pOldBlock->lenOri) && (pBlock->lenZip == pOldBlock->lenZip))
            {
                if (ndb_getDebugLevel() >= 9)
                    ndb_block_dumpblock(pBlock->BlockDataZip, pBlock->lenZip);
                // now compare the blocks:
				// if the "new" Block has the same data pointer into the archive file
				// as an older block then it has been loaded from the archive file
				// and we can trust that it has been processed in a previous
				// archiving process
				if (pBlock->staDat != 0)
				{
					if ((pBlock->staDat == pOldBlock->staDat) && (pBlock->dataFileNo == pOldBlock->dataFileNo))
					{
    	                // same block -> no write to disk neccessary
        	            bIsIdentical = 1;
            	        pBlock->blockInfo = pBlock->blockInfo | NDB_BLOCKINFO_DUPLICATEDATA;
                	    if (ndb_getDebugLevel() >= 6)
                    	    fprintf(stdout, "ndb_block_addBlockToTable: block data pointer identical -> no write to disk\n");
					    fflush(stdout);
	                    blocksSaved++;
    	                return bIsIdentical;
					}
					else
					{
        	            bIsIdentical = 0;
                    	if (ndb_getDebugLevel() >= 6)
                        	fprintf(stdout, "ndb_block_addBlockToTable: binary data differs (different data pointer into archive)\n");
					    fflush(stdout);
					}
				}
				else // newly created block with staDat = 0
				{
	                // compare the blocks byte by byte
        	        if (ndb_block_compareBlockData(pBlock, pOldBlock) == 1)
            	    {
                	    // same block -> no write to disk neccessary
                    	bIsIdentical = 1;
	                    pBlock->blockInfo = pBlock->blockInfo | NDB_BLOCKINFO_DUPLICATEDATA;
    	                // kopiere alten file offset in neuen Block Header
        	            pBlock->staDat = pOldBlock->staDat;
    	                // copy data file number in neuen Block Header
        	            pBlock->dataFileNo = pOldBlock->dataFileNo;
            	        if (ndb_getDebugLevel() >= 6)
                	        fprintf(stdout, "ndb_block_addBlockToTable: block data identical -> no write to disk\n");
					    fflush(stdout);
                    	blocksSaved++;
	                    return bIsIdentical;
    	            }
        	        else
            	    {
                	    bIsIdentical = 0;
                    	if (ndb_getDebugLevel() >= 6)
                        	fprintf(stdout, "ndb_block_addBlockToTable: binary data differs (different block content)\n");
					    fflush(stdout);
	                }
	            }
            }
            else
            {
                bIsIdentical = 0;
                if (ndb_getDebugLevel() >= 6)
                    fprintf(stdout, "ndb_block_addBlockToTable: data size (zip or ori) differs\n");
			    fflush(stdout);
            }
            // try with next block
            pLastBlock = pOldBlock;
            pOldBlock = pOldBlock->sameCRC;
        } while (pOldBlock != NULL);
        pLastBlock->sameCRC = pBlock;
        blockCount++;
    }
    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_block_addBlockToTable: now %lu in block table\n", blockCount);

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_block_addBlockToTable - finished\n");
    fflush(stdout);

    return bIsIdentical;
}

/*
    *************************************************************************************
    ndb_block_compareBlockData()

	checks whether the compressed block data of both blocks is the same
	- load zipped data from old block
	- if neccessary load zipped data of new block also
	- do a byte for byte comparision of zipped data

	return 1 if identical, 0 if different

    struct ndb_s_blockheader *pBlockNew:   (IN)    pointer to new block header
    struct ndb_s_blockheader *pBlockOld:   (IN)    pointer to old block header
    *************************************************************************************
*/
int ndb_block_compareBlockData(struct ndb_s_blockheader *pBlockNew,
                               struct ndb_s_blockheader *pBlockOld)
{
    int iBytesRead;
    int ii;
    int retval = 1;
    ULONG lOldFileOffset = 0;
    FILE *fileHandle = NULL;

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_block_compareBlockData - startup\n");
    fflush(stdout);

    // get memory (packed & unpacked) for old block from archive (size: bettes safe than sorry)
    pBlockOld->BlockDataZip = (char *) ndb_calloc(currBlockSize + 4096, 1, "ndb_block_compareBlockData: old buffer zip");
    if (pBlockOld->BlockDataZip == NULL)
    {
        fprintf(stderr, "ndb_block_compareBlockData: calloc() for old block failed\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }

    /* remember old position in file */
    lOldFileOffset = ftell(ndb_df_getReadHandle(ndb_df_getCurrWriteFileNo()));

    /* position to start of old block data */
	fileHandle = ndb_df_getReadHandle(pBlockOld->dataFileNo);
    fseek(fileHandle, pBlockOld->staDat, SEEK_SET);
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_block_compareBlockData: reading old block B%lu/F%lu/D%u (%x bytes) at file position %lx/%lx\n",
                        pBlockOld->blockNo, pBlockOld->fileNo, pBlockOld->dataFileNo, pBlockOld->lenZip, pBlockOld->staDat, ftell(fileHandle));
    fflush(stdout);

    iBytesRead = fread(pBlockOld->BlockDataZip, 1, (int) pBlockOld->lenZip, fileHandle);
    if (iBytesRead != pBlockOld->lenZip)
    {
        fprintf(stderr, "ndb_block_compareBlockData: Error: got %d instead of %d bytes while reading packed data of old block F%lu/B%lu/D%u at fatafile pos %lu\n",
                        iBytesRead, pBlockOld->lenZip, pBlockOld->fileNo, pBlockOld->blockNo, pBlockOld->dataFileNo, pBlockOld->staDat);
        fflush(stderr);
		/* position back to file position before*/
		fseek(ndb_df_getReadWriteHandle(ndb_df_getCurrWriteFileNo()), lOldFileOffset, SEEK_SET);
        return NDB_MSG_READERROR;
    }
    /* if neccessary get zipped data for new block also */
    if (pBlockNew->BlockDataZip == NULL)
    {
        pBlockNew->BlockDataZip = (char *) ndb_calloc(currBlockSize + 4096, 1, "ndb_block_compareBlockData: old buffer zip");
        if (pBlockNew->BlockDataZip == NULL)
        {
            fprintf(stderr, "ndb_block_compareBlockData: calloc(zip) for new block failed\n");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
        /* position to start of new block data */
		fileHandle = ndb_df_getReadHandle(pBlockNew->dataFileNo);
        fseek(fileHandle, pBlockNew->staDat, SEEK_SET);
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_block_compareBlockData: reading new block (%x bytes) at file position %lX\n",
                            pBlockNew->lenZip, pBlockNew->staDat);
        fflush(stdout);
        iBytesRead = fread(pBlockNew->BlockDataZip, 1, (int) pBlockNew->lenZip, fileHandle);
	    if (iBytesRead != pBlockNew->lenZip)
    	{
        	fprintf(stderr, "ndb_block_compareBlockData: Error: got %d instead of %d bytes while reading packed data of new block F%lu/B%lu/D%u\n",
            	            iBytesRead, pBlockNew->lenZip, pBlockNew->fileNo, pBlockNew->blockNo, pBlockNew->dataFileNo);
        	fflush(stderr);
			/* position back to file position before*/
			fseek(ndb_df_getReadWriteHandle(ndb_df_getCurrWriteFileNo()), lOldFileOffset, SEEK_SET);
    	    return NDB_MSG_READERROR;
	    }
    }

    /* position back to file position before*/
    fseek(ndb_df_getReadWriteHandle(ndb_df_getCurrWriteFileNo()), lOldFileOffset, SEEK_SET);

    // compare byte for byte between both buffers
    retval = 1;
    for (ii = 0; ii < pBlockNew->lenZip; ii++)
    {
        if (pBlockOld->BlockDataZip[ii] != pBlockNew->BlockDataZip[ii])
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_block_compareBlockData: found difference at byte %X: %2X <-> %2X\n",
                                ii, pBlockOld->BlockDataZip[ii], pBlockNew->BlockDataZip[ii]);
            fflush(stdout);
            retval = 0;
            break;
        }
    }

    // free memory (packed & unpacked) of old block from archive
    ndb_free(pBlockOld->BlockDataZip);

    fflush(stdout);

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_block_compareBlockData - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
    ndb_block_compressblock()

	wrapper for ndb_pack(pBlock), which zips the data of pBlock->BlockDataOri
	and writes them to pBlock->BlockDataZip

	Precondition: pBlock->BlockDataZip must be a valid pointer to an
	              already reserved buffer

    struct ndb_s_blockheader *pBlock:      (INOUT)    pointer to block header
    *************************************************************************************
*/
int ndb_block_compressblock(struct ndb_s_blockheader *pBlock)
{
    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_block_compressblock - startup\n");
    fflush(stdout);

    // first test precondition
    if (pBlock == NULL)
    {
        fprintf(stdout, "ndb_block_compressblock: error: block is null\n");
	    fflush(stdout);
        return NDB_MSG_NULLERROR;
    }

//    if ((ndb_getDebugLevel() >= 9) || (pBlock->blockType == NDB_BLOCKTYPE_OS2EAS))
    if (ndb_getDebugLevel() >= 9)
    {
        fprintf(stdout, "ndb_block_compressblock: dumping data before zipping\n");
        ndb_block_dumpblock(pBlock->BlockDataOri, pBlock->lenOri);
        fflush(stdout);
    }

    // now pack and get correct size for zipped data
    pBlock->lenZip = ndb_pack (pBlock);

//    if ((ndb_getDebugLevel() >= 9) || (pBlock->blockType == NDB_BLOCKTYPE_OS2EAS))
    if (ndb_getDebugLevel() >= 9)
    {
        fprintf(stdout, "ndb_block_compressblock: dumping zipped data\n");
        ndb_block_dumpblock(pBlock->BlockDataZip, pBlock->lenZip);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_block_compressblock - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_block_uncompressblock()

	wrapper for ndb_unpack(pBlock), which unzips the data of pBlock->BlockDataZip
	and writes them to pBlock->BlockDataOri

	Precondition: pBlock->BlockDataOri must be a valid pointer to an
	              already reserved buffer

    struct ndb_s_blockheader *pBlock:      (INOUT)    pointer to block header
    *************************************************************************************
*/
int ndb_block_uncompressblock(struct ndb_s_blockheader *pBlock)
{
    USHORT newLenOri = 0;
    ULONG  newCrc    = 0;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_block_uncompressblock - startup\n");
    fflush(stdout);

//    if ((ndb_getDebugLevel() >= 9) || (pBlock->blockType == NDB_BLOCKTYPE_OS2EAS))
    if (ndb_getDebugLevel() >= 9)
    {
        fprintf(stdout, "ndb_block_uncompressblock: dumping zipped data before unzipping\n");
        ndb_block_dumpblock(pBlock->BlockDataZip, pBlock->lenZip);
        fflush(stdout);
    }

    // now unpack and get correct size for original data
    newLenOri = ndb_unpack (pBlock);

//    if ((ndb_getDebugLevel() >= 9) || (pBlock->blockType == NDB_BLOCKTYPE_OS2EAS))
    if (ndb_getDebugLevel() >= 9)
    {
        fprintf(stdout, "ndb_block_uncompressblock: dumping data after unzipping\n");
        ndb_block_dumpblock(pBlock->BlockDataOri, pBlock->lenOri);
        fflush(stdout);
    }
    // check returned length with original length
    if (newLenOri != pBlock->lenOri)
    {
	    if (ndb_getDebugLevel() >= 1)
	    {
	        fprintf(stderr, "ndb_block_uncompressblock: Error: unpacking data block F%lu/B%lu/D%u resulted in %d instead of %d bytes\n",
	                        pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo, newLenOri, pBlock->lenOri);
	        fflush(stderr);
	    }
        retval = NDB_MSG_NOTOK;
    }
    // check crc with original crc
    newCrc = ndb_crc32(newCrc, pBlock->BlockDataOri, pBlock->lenOri);
    if (newCrc != pBlock->crc32)
    {
	    if (ndb_getDebugLevel() >= 1)
	    {
	        fprintf(stderr, "ndb_block_uncompressblock: Error: unpacking data block F%lu/B%lu/D%u resulted in CRC 0x%lX instead of 0x%lX\n",
	                        pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo, newCrc, pBlock->crc32);
	        fflush(stderr);
	    }
        retval = NDB_MSG_NOTOK;
    }

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_block_uncompressblock - finished\n");
    fflush(stdout);

    return retval;
}

