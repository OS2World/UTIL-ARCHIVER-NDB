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
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifndef UNIX
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ndb.h"
#include "ndb_prototypes.h"

#ifdef __MINGW32__
   int _CRT_glob = 0;   /* suppress command line globbing by C RTL */
#endif

#ifdef UNIX
#include <errno.h>
extern int errno;
#endif

// used by other modules also
int currBlockSize       = 0;
int currCompression     = 0;
int currMaxFHandles     = NDB_DATAFILE_HANDLESMAX;

// options set by command line parameter
static int  opt_lastchapter    = 0;
static int  opt_newchapter     = 0;
static int  opt_comparemode    = NDB_FILECOMPARE_EXACT;
static int  opt_waitmillis     = 0;
static int  opt_extradata      = 1;
static char opt_s_chaptername[200];
static char opt_s_chaptercomment[200];
static int  opt_debug          = 0;
static int  opt_fileflags      = 0;
static int  opt_recursedir     = 0;
static int  opt_arcemptydir    = 0;
static int  opt_exclude        = 0;
static int  opt_timestamp      = 0;
// opt_arcsymlinks is defined in ndb.h already
// extern int  opt_arcsymlinks;
static char opt_s_timestamp[20];

// a little bit statistics
ULONG8 ul8BlocksNewCount  = 0;
ULONG8 ul8BlocksNewLenOri = 0;
ULONG8 ul8BlocksNewLenZip = 0;
ULONG8 ul8BlocksOldCount  = 0;
ULONG8 ul8BlocksOldLenOri = 0;
ULONG8 ul8BlocksOldLenZip = 0;

/*
    *************************************************************************************
    ndb_compressStream()

    compresses the file content of a given file and saves the compressed data blocks
    in the archive file

    struct ndb_s_c_chapter *pChapter:      (IN)    pointer to current chapter structure
    struct ndb_s_fileentry *pFileEntry:    (IN)    pointer to current file structure
    FILE *fArchive:                        (IN)    file handle for archive file
    FILE *fFile:                           (IN)    file handle for current file (to compress)
    *************************************************************************************
*/
#if defined(USE_FHANDLE_INT)
int ndb_compressFile(struct ndb_s_c_chapter *pChapter, struct ndb_s_fileentry *pFileEntry,
                       FILE *fArchive, char *sFile)
#else
int ndb_compressStream(struct ndb_s_c_chapter *pChapter, struct ndb_s_fileentry *pFileEntry,
                       FILE *fArchive, FILE *fFile)
#endif
{
#if defined(USE_FHANDLE_INT)
#if defined(WIN32)
    HANDLE hFile = 0;
    DWORD dwCurDiff = 0;
    USHORT bIsSuccess = 0;
#else
    int hfile = 0;
#endif
#endif
    char *pBufferIn  = NULL;
    char *pBufferOut = NULL;
    ULONG8 ul8CurFilLen = 0;
    ULONG8 ul8Diff      = 0;
    int iCurDiff   = 0;
    int retval          = NDB_MSG_OK;
    short  bIsIdentical;
    short bIsFirstBlock = 1;
    short bAnyBlockChanged = 0;
    struct ndb_s_blockheader *pBlock = NULL;
    ULONG8 ul8FilLen;
    FILE *fileToWrite = NULL;
    int iFileNoToWrite  = -1;
    // needed for MD5 calculations
    MD5CTX  hasher;
    unsigned char digest[MD5_DIGESTSIZE];
    // remember read errors
    short countReadErrors = 0;


    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_compressStream: startup\n");
    fflush(stdout);

    // init MD5 context
    MD5_Initialize(&hasher);
    // get complete length of file
    ul8FilLen = pFileEntry->lenOri8;

    // allocate memory for input and output buffer
    pBufferIn  = (char *) ndb_calloc(currBlockSize + 4096, 1, "ndb_compressStream: block for ori");
    pBufferOut = (char *) ndb_calloc(currBlockSize + 4096, 1, "ndb_compressStream: block for zip");
    if (pBufferIn == NULL)
    {
        fprintf(stderr, "ndb_compressStream: couldn't allocate input buffer\n");
        exit(NDB_MSG_NOMEMORY);
    }
    if (pBufferOut == NULL)
    {
        fprintf(stderr, "ndb_compressStream: couldn't allocate output buffer\n");
        exit(NDB_MSG_NOMEMORY);
    }

#if defined(WIN32)
    // get handle for file
    hFile = ndb_win32_openFileOri(sFile);
//  fprintf(stdout, "ndb_compressFile: GetLastError() for ndb_win32_openFileOri() gave %lu\n", GetLastError());
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // free memory again
        ndb_free(pBufferIn);
        ndb_free(pBufferOut);
        return NDB_MSG_NOFILEOPEN;
    }

#else
#if defined(USE_FHANDLE_INT)
    hfile = open(sFile, O_RDONLY
#if !defined(UNIX)
                       | O_BINARY
#endif
                       | O_LARGEFILE);
    if (hfile == -1)
    {
        // free memory again
        ndb_free(pBufferIn);
        ndb_free(pBufferOut);
        return NDB_MSG_NOFILEOPEN;
    }
#endif
#endif
    while (ul8CurFilLen < ul8FilLen)
    {
        // in diesem Durchlauf BLOCKSIZE bytes oder rest kopieren
        ul8Diff = ul8FilLen - ul8CurFilLen;
        if (ul8Diff > currBlockSize) ul8Diff = currBlockSize;
        // block daten aufnehmen
        pBlock = (struct ndb_s_blockheader *) ndb_calloc(sizeof(struct ndb_s_blockheader), 1, "ndb_compressStream: new pBlock");
        if (pBlock == NULL)
        {
            fprintf(stderr, "ndb_compressStream: couldn't allocate block header\n");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
        pBlock->lenOri = ul8Diff;
        pBlock->lenZip = currBlockSize + 4096;
        pBlock->BlockDataOri = pBufferIn;
        pBlock->BlockDataZip = pBufferOut;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_compressStream: trying to read %d bytes at %.0f from %.0f\n",
                    pBlock->lenOri, (double) ul8CurFilLen, (double) ul8FilLen);
        fflush(stdout);
        // puffer einlesen
#if defined(WIN32)
        dwCurDiff = 0;
        bIsSuccess = ReadFile(hFile, pBlock->BlockDataOri, pBlock->lenOri, &dwCurDiff, NULL);
//      fprintf(stdout, "ndb_compressFile: GetLastError() for ReadFile() gave %lu\n", GetLastError());
        iCurDiff = dwCurDiff;
        if (! bIsSuccess)
            iCurDiff = 0;
#else
#if defined(USE_FHANDLE_INT)
        iCurDiff = read(hfile, pBlock->BlockDataOri, pBlock->lenOri);
#else
        iCurDiff = ndb_io_readdata(pBlock->BlockDataOri, (int) ul8Diff, fFile);
#endif
#endif
        // anything went wrong?
        if (iCurDiff == ul8Diff)
        {
            // currently no error
            countReadErrors = 0;
        }
        else
        {
            // there seems to be a problem?
            // -> be patient, perhaps it will correct itself if we wait ...
            ndb_osdep_sleep(NDB_READFIRSTWAIT);  // sleep 25 msec
            // didn't we get any bytes?
            // this might be a serious problem
            if (iCurDiff == 0)
            {
                // let's try 5 times to get data again
                if (countReadErrors < NDB_READMAXRETRIES)
                {
                    // let's wait and then try again ...
                    countReadErrors++;
                    fprintf(stdout, "Warning: got 0 bytes reading from file -> waiting for 500 msec\n");
                    fflush(stdout);
                    ndb_osdep_sleep(NDB_READMAXWAIT);  // sleep 500 msec
                    ndb_free(pBlock);
                    continue;
                }
                else
                {
                    // 5 times no bytes -> abort archiving this file
                    fprintf(stderr, "Warning: couldn't read from file '%s'\n-> aborting its archiving\n",
                            EMXWIN32CONVERT(pFileEntry->filenameExt));
                    fflush(stderr);
                    // correct chapter sum of blocks
                    pChapter->blockHeaNo -= pFileEntry->blockCount;
                    retval = NDB_MSG_READERROR;
                    break;
                }
            }
        }
        pBlock->lenOri = iCurDiff;
        // calculate CRCs for block entry and for complete file
        pBlock->crc32 = ndb_crc32(pBlock->crc32, pBlock->BlockDataOri, pBlock->lenOri);
//        pFileEntry->crc32 = ndb_crc32(pFileEntry->crc32, pBlock->BlockDataOri, pBlock->lenOri);
        MD5_Update(&hasher, pBlock->BlockDataOri, pBlock->lenOri);
        retval = ndb_block_compressblock(pBlock);
        // pFileEntry->lenZip will only be incremented if block data really
        // has to be saved in this chapter!
        // Blocknummer setzen
        pBlock->blockNo = pFileEntry->blockCount;
        pFileEntry->blockCount++;
        // Filenummer setzen
        pBlock->fileNo = pFileEntry->fileNo;
        // ggfs Blockinfo (first block) setzen
        if (bIsFirstBlock == 1)
        {
            pBlock->blockInfo = pBlock->blockInfo | NDB_BLOCKINFO_FIRST;
            bIsFirstBlock = 0;
        }
        // ggfs Blockinfo (last block) setzen
        if (ul8CurFilLen + pBlock->lenOri == ul8FilLen)
        {
            pBlock->blockInfo = pBlock->blockInfo | NDB_BLOCKINFO_LAST;
        }
        pBlock->blockType = NDB_BLOCKTYPE_FILEDATA;
        // Block in Blockliste des FileEntry aufnehmen
        ndb_fe_addBlock(pFileEntry, pBlock);
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_compressStream: now %lu blocks for file\n", pFileEntry->blockCount);
        fflush(stdout);
        // Block in Block-Tabelle aufnehmen
        bIsIdentical = ndb_block_addBlockToTable(pBlock);
        // increment block header counter in chapter
        pChapter->blockHeaNo++;
        // Block ins File schreiben
        if (!bIsIdentical)
        {
            // get file handle from current data file
            retval = ndb_df_getCheckedWriteHandle(&fileToWrite, &iFileNoToWrite, pChapter, pBlock);
            // set the corrected data file number and write the data to disk
            pBlock->dataFileNo = iFileNoToWrite;
            retval = ndb_io_writeBlockZipDataToFile(pBlock, fileToWrite);
            if (retval != NDB_MSG_OK)
            {
                if (ndb_getDebugLevel() >= 3)
                    fprintf(stdout, "ndb_compressStream: error saving block %lu with %d bytes\n",
                        pBlock->blockNo, pBlock->lenZip);
                fflush(stdout);
                retval = NDB_MSG_WRITEERROR;
                break;
            }
            // correct size counter in data file header structure
            ndb_df_addFileSize(iFileNoToWrite, (ULONG) pBlock->lenZip);
            // increment block data counter in chapter
            pChapter->blockDatNo++;
            // set flag that at least one block of file has changed
            bAnyBlockChanged = 1;
            // this block was a new one therefore increment pFileEntry->lenZip
            pFileEntry->lenZip8 += pBlock->lenZip;
            // increment "real size" of chapter
            pChapter->blockDatLenReal += pBlock->lenZip;
            // a little bit statistics
            ul8BlocksNewCount++;
            ul8BlocksNewLenOri += pBlock->lenOri;
            ul8BlocksNewLenZip += pBlock->lenZip;
        }
        else
        {
            // a little bit statistics
            ul8BlocksOldCount++;
            ul8BlocksOldLenOri += pBlock->lenOri;
            ul8BlocksOldLenZip += pBlock->lenZip;
        }
        if (ndb_getDebugLevel() >= 8)
        {
            ndb_block_print(pBlock);
            fflush(stdout);
        }
        if (ndb_getDebugLevel() >= 4)
        {
            fprintf(stdout, "ndb_compressStream: block %4lu, %5d -> %5d, ident %d, lenZip now %.0f bytes\n",
                    pBlock->blockNo, pBlock->lenOri, pBlock->lenZip, bIsIdentical, (double) pFileEntry->lenZip8);
            fflush(stdout);
        }
        // Länge anpassen
        ul8CurFilLen += pBlock->lenOri;
    } // while (iCurFilLen < iFilLen)

    // free memory again
    ndb_free(pBufferIn);
    ndb_free(pBufferOut);

#if defined(WIN32)
    CloseHandle(hFile);
#else
#if defined(USE_FHANDLE_INT)
    close(hfile);
#endif
#endif

    if (retval == NDB_MSG_OK)
    {
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_compressStream: file has %lu blocks\n", pFileEntry->blockCount);
        fflush(stdout);

        MD5_Final(digest, &hasher);
        memcpy(pFileEntry->md5, digest, MD5_DIGESTSIZE);

        if (ndb_getDebugLevel() >= 3)
        {
            fprintf(stdout, "ndb_compressStream: MD5 checksum is %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
                     digest[ 0], digest[ 1], digest[ 2], digest[ 3],
                     digest[ 4], digest[ 5], digest[ 6], digest[ 7],
                     digest[ 8], digest[ 9], digest[10], digest[11],
                     digest[12], digest[13], digest[14], digest[15]);
            fflush(stdout);
        }

        // mark file as unchanged (against previous chapters)
        if (bAnyBlockChanged == 0)
        {
            pFileEntry->flags |= NDB_FILEFLAG_IDENTICAL;
            pChapter->filesNoIdent++;
            if (ndb_getDebugLevel() >= 6)
                fprintf(stdout, "ndb_compressStream: file data same as in previous chapters\n");
            fflush(stdout);
        }
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_compressStream: finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
    ndb_addExtraData()

    adds the already compressed blocks of the given block list to the archive file
    in the archive file; also set some block meta data

    struct ndb_s_c_chapter *pChapter:      (IN)    pointer to current chapter structure
    struct ndb_s_fileentry *pFileEntry:    (IN)    pointer to current file structure
    FILE *fArchive:                        (IN)    file handle for archive file
    struct ndb_s_blockheader *pExtraBlocks (IN)    list of blocks with compressed extra data
    ULONG *size                            (OUT)   length of all uncompressed extra data
    *************************************************************************************
*/
int ndb_addExtraData(struct ndb_s_c_chapter *pChapter, struct ndb_s_fileentry *pFileEntry,
                     FILE *fArchive, struct ndb_s_blockheader *pExtraBlocks, ULONG *size,
                     struct ndb_s_fileextradataheader *pExtraHea)
{
    struct ndb_s_blockheader *pBlock;
    struct ndb_s_blockheader *pNextBlock;
    short  bIsIdentical;
    int retval = NDB_MSG_OK;
    FILE *fileToWrite = NULL;
    int iFileNoToWrite  = -1;
    USHORT i2;
    struct ndb_s_fileextradataheader *pCurrExtraHea = NULL;
    struct ndb_s_fileextradataheader *pNextExtraHea = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_addExtraData - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 7)
        ndb_fe_printAllExtraHeader(pFileEntry);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_addExtraData: before adding extra blocks file '%s' has %lu blocks\n",
                        EMXWIN32CONVERT(pFileEntry->filenameExt), pFileEntry->blockCount);
    fflush(stdout);

    *size = 0;
    // any blocks with extra data?
    pBlock = pExtraBlocks;
    pCurrExtraHea = pExtraHea;
    while(pCurrExtraHea != NULL)
    {
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_addExtraData: proceeding with extra data header type %X\n",
                    pCurrExtraHea->typExtHea);
        fflush(stdout);

        // any direct data in this header?
        if (pCurrExtraHea->lenExtHea > NDB_SAVESIZE_FILEEXTRAHEADER)
        {
            *size += pCurrExtraHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER;
        }
        for (i2 = 0; i2 < pCurrExtraHea->blockCountExtra; i2++)
        {
            if (pBlock == NULL)
            {
                fprintf(stderr, "Internal error: missing block no %d (of %d) from extra data header type %X\n",
                        i2, pCurrExtraHea->blockCountExtra, pCurrExtraHea->typExtHea);
                fflush(stderr);
                break;
            }

            if (ndb_getDebugLevel() >= 6)
                fprintf(stdout, "ndb_addExtraData: proceeding with extra data block  (F%lu/B%lu/I%d/T%d)\n",
                        pBlock->fileNo, pBlock->fileNo, pBlock->blockInfo, pBlock->blockType);
            fflush(stdout);
            if ((i2 == 0) && ((pBlock->blockInfo & NDB_BLOCKINFO_FIRST) != NDB_BLOCKINFO_FIRST))
            {
                fprintf(stderr, "Internal error: first block (F%lu/B%lu/I%d/T%d) of extra header type %X misses flag FIRST\n",
                        pBlock->fileNo, pBlock->fileNo, pBlock->blockInfo, pBlock->blockType,
                        pCurrExtraHea->typExtHea);
                fflush(stderr);
                break;
            }
            if ((i2 == pCurrExtraHea->blockCountExtra - 1) &&
               ((pBlock->blockInfo & NDB_BLOCKINFO_LAST) != NDB_BLOCKINFO_LAST))
            {
                fprintf(stderr, "Internal error: last block (F%lu/B%lu/I%d/T%d) of extra header type %X misses flag LAST\n",
                        pBlock->fileNo, pBlock->fileNo, pBlock->blockInfo, pBlock->blockType,
                        pCurrExtraHea->typExtHea);
                fflush(stderr);
                break;
            }

            if (ndb_getDebugLevel() >= 5)
            {
                fprintf(stdout, "ndb_addExtraData: trying to use block (F%lu/B%lu/I%d/T%d)\n",
                                pBlock->fileNo, pBlock->blockNo, pBlock->blockInfo, pBlock->blockType);
                fflush(stdout);
            }
            // Blocknummer setzen
            pBlock->blockNo = pFileEntry->blockCount++;
            // Filenummer setzen
            pBlock->fileNo = pFileEntry->fileNo;
            *size += pBlock->lenOri;
            // ndb_fe_addBlock() likes only single blocks ...
            pNextBlock = pBlock->nextBlock;
            pBlock->nextBlock = NULL;
            // block type must be set in the os functions which get the extra data
            // add block to block list  of FileEntry
            ndb_fe_addBlock(pFileEntry, pBlock);
/*
            // repair block pointer after ndb_fe_addBlock()
            pBlock->nextBlock = pNextBlock;
*/
            if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_addExtraData: now %lu blocks for file\n", pFileEntry->blockCount);
            fflush(stdout);
            // Block in Block-Tabelle aufnehmen
            // if block is identical then ndb_block_addBlockToTable() will corrected the data file number
            bIsIdentical = ndb_block_addBlockToTable(pBlock);
            // increment block header counter in chapter
            pChapter->blockHeaNo++;
            // Block ins File schreiben
            if (!bIsIdentical)
            {
                // get file handle from current data file
                retval = ndb_df_getCheckedWriteHandle(&fileToWrite, &iFileNoToWrite, pChapter, pBlock);
                // set the correct data file number ...
                pBlock->dataFileNo = iFileNoToWrite;
                ndb_io_writeBlockZipDataToFile(pBlock, fileToWrite);
                // correct size counter in data file header structure
                ndb_df_addFileSize(iFileNoToWrite, (ULONG) pBlock->lenZip);
                // increment block data counter in chapter
                pChapter->blockDatNo++;
                // increment "real size" of chapter
                pChapter->blockDatLenReal += pBlock->lenZip;
                // a little bit statistics
                ul8BlocksNewCount++;
                ul8BlocksNewLenOri += pBlock->lenOri;
                ul8BlocksNewLenZip += pBlock->lenZip;
            }
            else
            {
                // a little bit statistics
                ul8BlocksOldCount++;
                ul8BlocksOldLenOri += pBlock->lenOri;
                ul8BlocksOldLenZip += pBlock->lenZip;
            }
            if (ndb_getDebugLevel() >= 8)
            {
                ndb_block_print(pBlock);
                fflush(stdout);
            }
            // free memory again
            ndb_free(pBlock->BlockDataOri);
            ndb_free(pBlock->BlockDataZip);
                        // proceed with next block
//            pBlock = pBlock->nextBlock;
                        pBlock = pNextBlock;
        }

                // ndb_fe_addExtraHeader() allows only single extra data header!
                pNextExtraHea = pCurrExtraHea->pNextHea;
                pCurrExtraHea->pNextHea = NULL;
        retval = ndb_fe_addExtraHeader(pFileEntry, pCurrExtraHea);
        if (retval != NDB_MSG_OK)
        {
                fprintf(stderr, "ndb_addExtraData: adding extra header to file '%s' failed\n",
                                pFileEntry->filenameUTF8);
                fflush(stderr);
        }
        // correct number of all extra header in fileentry structure
        pFileEntry->nr_ofExtraHea++;
        // correct length of all extra header in fileentry structure
        pFileEntry->lenExtHea += pCurrExtraHea->lenExtHea;

        if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_addExtraData: after extra header now %lu bytes extra data\n",
                                *size);
        fflush(stdout);

        // proceed with next extra header
        pCurrExtraHea = pNextExtraHea;
    }

        if (pCurrExtraHea != NULL)
        {
                fprintf(stderr, "Internal error: found unexpected additional extra header type %X\n",
                        pCurrExtraHea->typExtHea);
                fflush(stderr);
        }
        if (pBlock != NULL)
        {
                fprintf(stderr, "Internal error: found unexpected additional extra data block (F%lu/B%lu/I%d/T%d)\n",
                                    pBlock->fileNo, pBlock->blockNo, pBlock->blockInfo, pBlock->blockType);
                fflush(stderr);
        }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_addExtraData: after adding extra blocks file '%s' has %lu blocks\n",
                        EMXWIN32CONVERT(pFileEntry->filenameExt), pFileEntry->blockCount);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_addExtraData: finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
    addFileMaskToList()

    checks the filemask from  the commandline and adds them to the file list,
    if it doesn't match the exclude file masks:
    - first check, if it's a file or directory on disk
      - if it's a file, skip -r or -R for this entry
      - if it's a dir, add '\*' to get the content of it
    - call ndb_osdep_makeFileList() to get it / it's files added to the filelist
    *************************************************************************************
*/
void addFileMaskToList(char *s_files, struct ndb_l_filelist *pFileListExternal, struct ndb_l_filemasklist *pFileListExcludes)
{
    char sDir[NDB_MAXLEN_FILENAME];
    char sFile[NDB_MAXLEN_FILENAME];
    int opt_recursedir2 = 0;
    ULONG ulAttr;
    ULONG ulDummy;
    int i = 0;

    // do first the checks against the exclude filemasks
    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "addFileMaskToList: testing '%s' against exludes masks\n",
                EMXWIN32CONVERT(s_files));
        fflush(stdout);
    }
    for (i = 0; i < ndb_list_getFileMaskCount(pFileListExcludes); i++)
    {
        if (ndb_osdep_matchFilename(ndb_list_getFileMask(pFileListExcludes, i), s_files) == 1)
        {
            if (ndb_getDebugLevel() >= 6)
            {
                fprintf(stdout, "addFileMaskToList: '%s' matches against exludes mask '%s'\n",
                        EMXWIN32CONVERT(s_files), ndb_list_getFileMask(pFileListExcludes, i));
                fflush(stdout);
                return;
            }

        }
    }

    // check whether filemask is a concrete file or concrete dir
    opt_recursedir2 = opt_recursedir;
    if ((NULL == strchr(s_files, '*')) && (NULL == strchr(s_files, '?')))
    {
        if (ndb_getDebugLevel() >= 8)
        {
            fprintf(stdout, "'%s' contains no wildcards\n", EMXWIN32CONVERT(s_files));
            fflush(stdout);
        }
        // no wildcard, now check for existence
        if (NDB_MSG_OK == ndb_io_check_exist_file(s_files, NULL))
        {
            if (ndb_getDebugLevel() >= 8)
            {
                fprintf(stdout, "'%s' exists on harddrive\n", EMXWIN32CONVERT(s_files));
                fflush(stdout);
            }
            // no directory? -> if yes, clear opt_recursedir2
#if defined(UNIX)
            if (((ulAttr = ndb_unix_getFileMode(s_files, opt_arcsymlinks)) & NDB_FILEFLAG_DIR) != NDB_FILEFLAG_DIR)
#else
            if (((ulAttr = ndb_osdep_getFileMode(s_files)) & NDB_FILEFLAG_DIR) != NDB_FILEFLAG_DIR)
#endif
            {
                if (ndb_getDebugLevel() >= 2)
                {
                    ndb_printTimeMillis();
                    fprintf(stdout, "adding file '%s' directly to filelist\n", EMXWIN32CONVERT(s_files));
                    fflush(stdout);
                }
                // add file to list
                ndb_osdep_addFileToFileList(NULL, ndb_osdep_makeIntFileName(s_files), ulAttr, pFileListExternal);
                return;
            }
            else
            {
                // filemask is an existing directory
                if (ndb_getDebugLevel() >= 8)
                {
                    fprintf(stdout, "'%s' is a dir therefore add '*' or '/*'\n", EMXWIN32CONVERT(s_files));
                    fflush(stdout);
                }
                // add dir to list
                ndb_osdep_addFileToFileList(NULL, ndb_osdep_makeIntFileName(s_files), ulAttr, pFileListExternal);
                // add wildcard for all files
#if defined (UNIX)
                if (s_files[strlen(s_files) -1] == '/')
#elif defined (WIN32) || defined (OS2)
                if (s_files[strlen(s_files) -1] == '\\')
#endif
                {
                        strcat(s_files, "*");
                }
                else
                {
#if defined (WIN32) || defined (OS2)
                                strcat(s_files, "\\*");
#elif defined (UNIX)
                                strcat(s_files, "/*");
#endif
                }
            }
        }
    }
    // include files
    ndb_osdep_split(ndb_osdep_makeIntFileName(s_files), sDir, sFile);
    if (sFile[0] == '\0')
        strcpy (sFile, "*");
    if (ndb_getDebugLevel() >= 8)
    {
        fprintf(stdout, "'%s' splitted into '%s' and '%s'\n",
                EMXWIN32CONVERT(s_files), sDir, sFile);
        fflush(stdout);
    }
    if (sFile[0] == '\0')
        strcpy (sFile, "*");
    if (ndb_getDebugLevel() >= 2)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "adding files '%s' to filelist\n", EMXWIN32CONVERT(s_files));
        fflush(stdout);
    }
    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_archive: flags: hidden = %d, recursive = %d\n", opt_fileflags, opt_recursedir);
        fflush(stdout);
    }
    ndb_osdep_makeFileList(sDir, ndb_osdep_makeIntFileName(s_files), opt_fileflags | opt_recursedir2, pFileListExternal, pFileListExcludes, &ulDummy);
}

/*
    *************************************************************************************
    copyFileLinesToExcludeList()

    - opens <s_files> as file with a filemask each line
    - call for each line addFileMaskToList()
    - close <s_files>
    *************************************************************************************
*/
void copyFileLinesToExcludeList(char *s_files, struct ndb_l_filemasklist *pFileListExcludes)
{
    FILE *fList = NULL;
    char sLine[NDB_MAXLEN_FILEMASK];

    if (NDB_MSG_OK != ndb_io_check_exist_file(s_files, NULL))
    {
        fprintf(stderr, "\nError: could not read filemasks from file '%s'\n", s_files);
        fprintf(stderr, "ignoring '-x@%s'\n\n", s_files);
        fflush(stderr);
    }
    else
    {
        fList = fopen(s_files, "r");
        if (fList == NULL)
        {
            fprintf(stderr, "\nError: could not open file '%s': %s\n", s_files, strerror (errno));
                fflush(stderr);
        }
        else
        {
            while(!feof(fList) )
            {
                fgets(sLine, sizeof(sLine), fList);
                if (feof(fList))
                    break;
                // remove EOL character
                while ((sLine[strlen(sLine) - 1] == '\n') || (sLine[strlen(sLine) - 1] == '\n'))
                        sLine[strlen(sLine) - 1] = '\0';
                if (ndb_getDebugLevel() >= 6)
                {
                    fprintf(stdout, "copyFileLinesToExcludeList: adding '%s' to exclude masks\n", s_files);
                    fflush(stdout);
                }
                ndb_list_addToFileMaskList(pFileListExcludes, ndb_osdep_makeIntFileName(sLine), NDB_MODE_EXCLUDE);
            }
            fclose(fList);
        }
    }
}


/*
    *************************************************************************************
    copyFileLinesToIncludeList()

    - opens <s_files> as file with a filemask each line
    - call for each line addFileMaskToList()
    - close <s_files>
    *************************************************************************************
*/
void copyFileLinesToIncludeList(char *s_files, struct ndb_l_filelist *pFileListExternal, struct ndb_l_filemasklist *pFileListExcludes)
{
    FILE *fList = NULL;
    char sLine[NDB_MAXLEN_FILEMASK];

    if (NDB_MSG_OK != ndb_io_check_exist_file(s_files, NULL))
    {
                fprintf(stderr, "\nError: could not read filemasks from file '%s'\n", s_files);
                fprintf(stderr, "ignoring '-i@%s'\n\n", s_files);
                fflush(stderr);
    }
    else
    {
        fList = fopen(s_files, "r");
        if (fList == NULL)
        {
            fprintf(stderr, "\nError: could not open file '%s': %s\n", s_files, strerror (errno));
                fflush(stderr);
        }
        else
        {
            while(!feof(fList) )
            {
                fgets(sLine, sizeof(sLine), fList);
                if (feof(fList))
                    break;
                // remove EOL character
                while ((sLine[strlen(sLine) - 1] == '\n') || (sLine[strlen(sLine) - 1] == '\n'))
                        sLine[strlen(sLine) - 1] = '\0';
                addFileMaskToList(sLine, pFileListExternal, pFileListExcludes);
            }
            fclose(fList);
        }
    }
}

/*
    *************************************************************************************
    set_option()

    processes one option of command line and sets the corresponding global variable(s)

    char *options:                         (IN)    pointer to string with current option
    *************************************************************************************
*/
void set_option (char *options)
{
    int i1;
    char ch;
    char stime[40];
    struct tm *tim;
    time_t timT;
    char sDummy[NDB_MAXLEN_FILENAME];

    for (i1 = 1; options [i1] != '\0'; i1++)
    {
        ch = options [i1];
        switch (ch)
        {
            case 'd':                            /* set debug level */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: set debug level\n");
                ch = options [++i1];
                if ((ch >= '0') && (ch <= '9'))
                {
                    opt_debug = ch - '0';
                    ndb_setDebugLevel(opt_debug);
                }
                return;
            case 'c':                            /* compare method (exactly/fast) */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: set compare method\n");
                ch = options [++i1];
                if (ch == 'x')
                {
                    opt_comparemode = NDB_FILECOMPARE_EXACT;
                }
                else if (ch == 'f')
                {
                    opt_comparemode = NDB_FILECOMPARE_FAST;
                }
                else if (ch == 'c')
                {
                    opt_comparemode = NDB_FILECOMPARE_CRC32;
                }
                else if (ch == 'm')
                {
                    opt_comparemode = NDB_FILECOMPARE_MD5;
                }
                return;
            case 'X':                            /* don't archive extra data */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: clear flag for extra data\n");
                opt_extradata = 0;
                break;
            case 'l' :                           /* use last chapter */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: use last chapter\n");
                opt_lastchapter = 1;
                opt_s_chaptername[0] = '\0';
                break;
            case 'n':                            /* add new chapter */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: use new chapter\n");
                opt_newchapter = 1;
                // chaptername nix, oder geht bis zum Ende
                strcpy (opt_s_chaptername, &options [i1+1]);
                if (opt_s_chaptername[0] != '\0')
                {
                    // ggfs Anführungszeichen entfernen
                    if (opt_s_chaptername[0] == '"')
                    {
                        strcpy (opt_s_chaptername, &opt_s_chaptername[1]);
                        opt_s_chaptername[strlen(opt_s_chaptername) - 1] = '\0';
                    }
                }
                else
                {
                    // create default name for chapter
                    timT = time(NULL);
                    tim = localtime((const time_t *) &timT);
                    strftime(stime, 40, "%Y-%m-%d %H:%M", tim);
                    snprintf(opt_s_chaptername, 200 - 1, "BAK %s", stime);
                }
                return;
            case 'm':                            /* chapter comment*/
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: chapter comment\n");
                // chaptercomment geht bis zum Ende
                strcpy (opt_s_chaptercomment, &options [i1+1]);
                // Anführungszeichen entfernen, falls notwendig
                if (opt_s_chaptercomment[0] == '"')
                {
                    strcpy (opt_s_chaptercomment, &opt_s_chaptercomment[1]);
                    opt_s_chaptercomment[strlen(opt_s_chaptercomment) - 1] = '\0';
                }
                return;
            case 't':                            /* chapter timestamp */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: chapter timestamp\n");
                // Timestamp geht bis zum Ende
                opt_timestamp = 1;
                strcpy (opt_s_timestamp, &options [i1+1]);
                return;
            case 'r':                           /* recurse into subdirectory */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: recurse into subdirectory\n");
                opt_recursedir  = NDB_FILEFLAG_DIR;
                opt_arcemptydir = 0;
                break;
            case 'R':                           /* recurse into subdirectory, archive empty dirs also */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: recurse, empty directories also\n");
                opt_recursedir  = NDB_FILEFLAG_DIR;
                opt_arcemptydir = 1;
                break;
            case 's':                           /* add hidden fields */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: add hidden fields\n");
                opt_fileflags  = NDB_FILEFLAG_HIDDEN;
                break;
            case 'x':                           /* exclude files */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: exclude files\n");
                opt_exclude = 1;
                break;
            case 'i':                           /* include files */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: include files\n");
                opt_exclude = 0;
                break;
            case 'y':                           /* store symbolic links as the link instead of the referenced file */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: store symbolic links as the link instead of the referenced file\n");
                opt_arcsymlinks = 1;
                opt_fileflags  = NDB_FILEFLAG_SLNK;
                break;
            case 'w':                           /* wait msecs after each file */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: wait after each file\n");
                opt_waitmillis = atoi(&options [i1 + 1]);
                return;
            case 'f':                            /* file handle count? */
                ch = options [++i1];
                if (ch == 'h')
                {
                    strcpy(sDummy, &options [i1 + 1]);
                    currMaxFHandles = atoi(sDummy);
                    if (ndb_getDebugLevel() >= 3)
                        fprintf(stdout, "set_option: maximal number of concurrent file handles set to %d\n",
                                        currMaxFHandles);
                }
                else
                {
                    fprintf(stdout, "set_option: ignoring unknown option '-f%c'\n", ch);
                    break;
                }
                return;

            default:
                fprintf(stdout, "set_option: ignoring unknown option '-%c'\n", ch);
                break;
        }
    }
   return;
}


/*
    *************************************************************************************
    main()

    - processes the command line arguments
    - creates the list with the files to be archived
    - opens the archive file
    - compresses each file and saves the data into the archive
    - gets the extra data (if available and implemented), compresses and saves them also
    - updates the administrative data of the archive file

    int argc:                              (IN)    number of command line arguments
    char **argv:                           (IN)    array of command line arguments
    *************************************************************************************
*/

int ndb_cleanUpAfterError(char *, FILE *, PARCHIVE, PCHAPTER);

int main(int argc, char **argv)
{
    char s_container[NDB_MAXLEN_FILENAME];
    char s_files[NDB_MAXLEN_FILENAME];
    FILE *fArchive;
#if defined(UNIX)
    struct stat64 stat64File;
#else
    struct stat statFile;
#endif
#if defined(WIN32)
    LPVOID lpMsgBuf;
#endif

    char *p;
    ULONG i;
    USHORT i1 = 0;
    int flag_zipname = 0;
    struct ndb_l_filelist fileListExternal;
    struct ndb_l_filemasklist fileListExcludes;
    struct ndb_l_filelist fileListFailed;
    struct ndb_s_fileentry *pCurrFile = NULL;
    time_t time_startup;
    time_t time_finished;
    double db_duration = 0;
    USHORT len;
    int dot_found;
    char *iobuf;
    struct ndb_s_c_archive currArchive;
    struct ndb_s_c_archive saveArchive;
    int retval = NDB_MSG_NOTOK;
    int retval2 = NDB_MSG_NOTOK;
    struct ndb_s_c_chapter currChapter;
    struct ndb_s_c_chapter *pLastChapter = NULL;
    struct ndb_s_c_chapter *pAllChapters = NULL;
    struct ndb_s_c_block blockDataChunk;
    struct ndb_s_c_block blockHeaderChunk;
    struct ndb_s_c_block dataFilesChunk;
    ULONG currFilePos;
    int filecount;
#if !defined(USE_FHANDLE_INT)
    FILE *iFile;
#endif
    struct ndb_s_blockheader *pExtraBlocks = NULL;
    struct ndb_s_fileentry *pFileEntry = NULL;
    struct ndb_s_fileentry *pOldFile = NULL;
    struct ndb_s_c_block fileEntryChunk;
    ULONG highestFilePos;
    ULONG ulFilesSaved = 0;
    ULONG ulFilesFailed = 0;
    ULONG ulFilesFailedExtra = 0;
    ULONG sizeExtra = 0;
    char sText[100];
    // 25Jun03 pw NEW collect all files which couldn't be extracted
    char stime_m[40];
    struct tm *tim_m;
    char dummy[200];
    // 17Oct03 pw NEW track number of bytes read and written
    double db_bytesread    = 0;
    double db_byteswritten = 0;
    //03Dec03 pw NEW rewritten extra data handling
    struct ndb_s_fileextradataheader *pExtraHea = NULL;
    PDATAFILEHEADER pNewDataFile = NULL;
    USHORT doCompress = 1;

    time_startup = 0;
    time(&time_startup);
    // ctime() ends with '\n' already
    fprintf(stdout, "NDBArchive %s: startup at %s", NDB_VERSION, ctime(&time_startup));
    fprintf(stdout, "%s\n", NDB_COPYRIGHT);
    fprintf(stdout, "Using %s and a few source parts of %s and %s\n", NDB_USING_ZLIB, NDB_USING_ZIP, NDB_USING_UNZIP);
    ndb_osdep_version_local();
    fflush(stdout);

    ndb_alloc_init(50000L);

    // ========================================================================
    // init functions
    // ========================================================================
    ndb_utf8_inittables(ndb_osdep_getCodepage());
    ndb_osdep_init();

        fprintf(stdout, "Current OS:          '%s'\n", ndb_osdep_getOS());
        fprintf(stdout, "Current OS codepage: '%s'\n", ndb_osdep_getCodepage());
        fprintf(stdout, "\n");
        fflush(stdout);

    if ((argc < 3) || (strcmp("-h", argv[1]) == 0))
    {
        fprintf(stdout, "usage:\n");
        fprintf(stdout, "ndbarchive <archiv> -n<name> [options] [-d<level>] [-w<millis>] <files>\n");
        fprintf(stdout, "                    [-x|i <files>] [-i@<file>] [-x@<file>]\n");
        fprintf(stdout, "    List of all parameters and options:\n");
        fprintf(stdout, "    -h:           this help text\n");
        fprintf(stdout, "    <archiv>:     name of NDB archive file\n");
        fprintf(stdout, "    <files>:      filename(s), wildcards are * and ?\n");
        fprintf(stdout, "                  (use double quotes for unix)\n");
        fprintf(stdout, "    -n[<\"name\">]: create a *n*ew chapter with name <\"name\">\n");
        fprintf(stdout, "    -l:           add files to *l*ast chapter\n");
        fprintf(stdout, "    -r:           *r*ecurse into subdirectories (don't archive empty dirs)\n");
        fprintf(stdout, "    -R:           like -r, archive empty dirs also\n");
#ifdef UNIX
        fprintf(stdout, "    -y:           store s*y*mbolic links as the link instead of the file\n");
#else
        fprintf(stdout, "    -s:           add hidden and *s*ystem files\n");
#endif
        fprintf(stdout, "    -cx:          *c*ompare files e*x*actly (default)\n");
        fprintf(stdout, "                  (slowest speed, exact test)\n");
        fprintf(stdout, "    -cm:          *c*ompare files by their *m*d5\n");
        fprintf(stdout, "                  (medium speed, exact test)\n");
        fprintf(stdout, "    -cc:          *c*ompare files by their block *c*rcs\n");
        fprintf(stdout, "                  (faster than -cm, but risk of 1:4294967296\n");
        fprintf(stdout, "                   to miss a change in the file)\n");
        fprintf(stdout, "    -cf:          *c*ompare files *f*ast\n");
        fprintf(stdout, "                  (lightning speed, exact test if all OS conform)\n");
        fprintf(stdout, "    -X:           ignore e*X*tra data (e.g. OS/2 EAs)\n");
        fprintf(stdout, "    -x <files>:   e*x*clude following files\n");
        fprintf(stdout, "    -i <files>:   *i*nclude following files\n");
        fprintf(stdout, "    -x@<file>:    e*x*clude file(mask)s listed in <file>\n");
        fprintf(stdout, "    -i@<file>:    *i*nclude file(mask)s listed in <file>\n");
        fprintf(stdout, "    -t<yyyymmddhhmmss>  set chapter creation *t*ime\n");
        fprintf(stdout, "    -m<text>:     add a co*m*ment (max 79 chars) for this chapter\n");
        fprintf(stdout, "    -d<level>:    set *d*ebug level 1..9\n");
        fprintf(stdout, "    -w<millis>:   *w*ait <millis> milliseconds after each file\n");
        fprintf(stdout, "    -fh<max>:     use no more than <max> concurrent file handles\n");
        fprintf(stdout, "                  even if there are more datafiles ");
#if defined (OS2)
        fprintf(stdout, "(default 25)\n");
#else
        fprintf(stdout, "(default 50)\n");
#endif
        exit(0);
    }
    else
    {
        // init file list
        ndb_list_initFileList(&fileListExternal, NDB_USE_HASHTABLE);
        ndb_list_initFileMaskList(&fileListExcludes);
        opt_s_chaptername[32] = '\0';

        // 1st pass: scan for '-x' and -x@<file>' and for '-i' also
        // because it sets NDB back to include mode
        // (and for '-dx' for convenience also)
        opt_exclude = 0;  // start with mode 'include next filemask(s)'
        for (i1 = 1; i1 < argc; i1++)
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "1st pass: processing argv[%d] = '%s'\n", i1, argv[i1]);
            fflush(stdout);
            p = argv[i1];
            if ( (*p) == '-')
            {
                if (strncmp(p, "-x@", 3) == 0)
                {
                    if (flag_zipname == 0)
                    {
                        fprintf(stderr, "Error: please specify the archive file name first.\n");
                        fflush(stderr);
                        exit(NDB_MSG_NOTOK);
                    }
                    strcpy (s_files, &p[3]);
                    copyFileLinesToExcludeList(s_files, &fileListExcludes);
                }
                else if (strncmp(p, "-d", 2) == 0)
                    set_option ((char *) argv[i1]);
                else if (strcmp(p, "-x") == 0)
                    set_option ((char *) argv[i1]);
                else if (strcmp(p, "-i") == 0)
                    set_option ((char *) argv[i1]);
            }
            else
            {
                // first file name is name of archive file
                if (flag_zipname == 0)
                {
                    flag_zipname = 1;
                } else
                {
                    // second and later file names are file masks to include or exclude
                    flag_zipname = 2;
                    strcpy (s_files, (char *) argv[i1]);
                    // exclude files with this file mask?
                    if (opt_exclude == 1)
                    {
                        if (ndb_getDebugLevel() >= 6)
                        {
                            fprintf(stdout, "adding '%s' to exclude masks\n", s_files);
                            fflush(stdout);
                        }
                        ndb_list_addToFileMaskList(&fileListExcludes, s_files, NDB_MODE_EXCLUDE);
                        if (ndb_getDebugLevel() >= 4)
                            fprintf(stdout, "exclude list 0x%lX has no %d elements\n", (ULONG) &fileListExcludes,
                                            ndb_list_getFileMaskCount(&fileListExcludes));
                    }
                }
            }
        }

        // 2nd pass: work through all arguments
        flag_zipname = 0;  // do everything again (except excludes)
        opt_exclude  = 0;  // start with mode 'include next filemask(s)'
        for (i1 = 1; i1 < argc; i1++)
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "2nd pass: processing argv[%d] = '%s'\n", i1, argv[i1]);
            fflush(stdout);
            p = argv[i1];
            if ( (*p) == '-')
            {
                if (strncmp(p, "-i@", 3) == 0)
                {
                    if (flag_zipname == 0)
                    {
                        fprintf(stderr, "Error: please specify the archive file name first.\n");
                        fflush(stderr);
                        exit(NDB_MSG_NOTOK);
                    }
                    else
                        flag_zipname = 2;
                    strcpy (s_files, &p[3]);
                    copyFileLinesToIncludeList(s_files, &fileListExternal, &fileListExcludes);
                }
                else if (strncmp(p, "-x@", 3) == 0)
                {
                    ; // already done in 1st pass
                }
                else
                    set_option ((char *) argv[i1]);
            }
            else
            {
                // first file name is name of archive file
                if (flag_zipname == 0)
                {
                    flag_zipname = 1;
                    strcpy (s_container, (char *) argv[i1]);
                    // add ".ndb" if missing
                    len = strlen(s_container);
                    dot_found = 0;
                    for (i = 0; i < len; i++)
			        {
                        if (s_container[i] == '.')
                            dot_found = 1;
                        else if ((s_container[i] == '/') || (s_container[i] == '\\'))
                            dot_found = 0;
                    }
                    if (dot_found == 0)
                        strcat(s_container,".ndb");
                    // does archive file exist?
                    if (NDB_MSG_OK != ndb_io_check_exist_file(s_container, NULL))
                    {
                        fprintf(stderr, "could not open archive file '%s'\n", s_container);
                        fflush(stderr);
                        exit (NDB_MSG_NOFILEOPEN);
                    }
                } else
                {
                    // second and later file names are file masks to include or exclude
                    flag_zipname = 2;
                    strcpy (s_files, (char *) argv[i1]);
                    // include or exclude files with this file mask?
                    if (opt_exclude == 0)
                    {
                        addFileMaskToList(s_files, &fileListExternal, &fileListExcludes);
                        if (ndb_getDebugLevel() >= 4)
                            fprintf(stdout, "file list 0x%lX has no %lu elements\n", (ULONG) &fileListExternal,
                                            ndb_list_getFileCount(&fileListExternal));
                    }
                    else
                    {
                        ; // already done in 1st pass
                    }
                }
            }
        }

        // show list of files with retval (archive/ignore/...)
        if (ndb_getDebugLevel() >= 3)
        {
            fprintf(stdout, "resulting file list with %lu members:\n", ndb_list_getFileCount(&fileListExternal));
            if (ndb_getDebugLevel() >= 5)
            {
                pFileEntry = ndb_list_getFirstFile(&fileListExternal);
                while (pFileEntry != NULL)
                {
                    if (pFileEntry->action == NDB_ACTION_IGNORE)
                        fprintf(stdout, "IGNORE  '%s'\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
                    else if (pFileEntry->action == NDB_ACTION_ADD)
                        fprintf(stdout, "ADD     '%s'\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
                    else if (pFileEntry->action == NDB_ACTION_UPDATE)
                        fprintf(stdout, "UPDATE  '%s'\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
                    else if (pFileEntry->action == NDB_ACTION_KEEP)
                        fprintf(stdout, "KEEP    '%s'\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
                    else if (pFileEntry->action == NDB_ACTION_EXTRACT)
                        fprintf(stdout, "EXTRACT '%s'\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
                    else
                        fprintf(stdout, "[NONE]  '%s'\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
                    pFileEntry = ndb_list_getNextFile(&fileListExternal);
                }
            }
        }
        fflush(stdout);


        if (ndb_getDebugLevel() >= 2)
        {
            ndb_printTimeMillis();
            fprintf(stdout, "found %lu files and/or directories\n\n", ndb_list_getFileCount(&fileListExternal));
            fflush(stdout);
        }
    }


#if !defined(NDB_DEBUG_ALLOC_OFF)
        ndb_show_status();
//        ndb_show_ptr();
#endif


    // check parameter
    if ((opt_newchapter == 1) && (opt_lastchapter == 1))
    {
        fprintf(stderr, "Error: combined use of -l and -n<no> senseless\n");
        exit(NDB_MSG_NOTOK);
    }
    if ((opt_newchapter == 0) && (opt_lastchapter == 0))
    {
        fprintf(stderr, "Error: one of -l or -n must be specified\n");
        exit(NDB_MSG_NOTOK);
    }
    if ((opt_lastchapter == 1) && (opt_timestamp == 1))
    {
        // timestamp makes only sense at new chapter
        opt_timestamp = 0;
    }
    if (s_container[0] == '\0')
    {
        fprintf(stderr, "Error: cannot find archive file name\n");
        exit(NDB_MSG_NOTOK);
    }
    if (flag_zipname < 2)
    {
        fprintf(stderr, "Error: missing which file(s) to archive\n");
        exit(NDB_MSG_NOTOK);
    }



    if (ndb_getDebugLevel() >= 2)
    {
        for (i = 0; i < argc; i++)
        {
            fprintf(stdout, "command line parameter %lu: '%s'\n", i, argv[i]);
        }
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "container file   is  %s\n", s_container);
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "files to archive are %s\n", s_files);
    fflush(stdout);

    // ==============================================================================
    // start of main work
    // ==============================================================================

    /* open archive file */
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "trying to open archiv file\n");
    fArchive = fopen(s_container, "rb+");
    if (fArchive == NULL)
    {
        fprintf(stderr, "could not open archive file '%s'\n", s_container);
                fflush(stderr);
        exit (NDB_MSG_NOFILEOPEN);
    }

    //  use a buffer for archive file IO
    iobuf = (char *) ndb_calloc(NDB_SIZE_IOBUFFER, 1, "IO buffer");
    if (iobuf != NULL)
    {
        if (setvbuf(fArchive, iobuf, _IOFBF, NDB_SIZE_IOBUFFER) == 0)
        {
            if (ndb_getDebugLevel() >= 7) fprintf(stdout, "using IO buffer with %d bytes\n\n", NDB_SIZE_IOBUFFER);
        }
        else
        {
            fprintf(stdout, "Error: could not activate IO buffer\n\n");
        }
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 2)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "opening current archive file\n\n");
        fflush(stdout);
    }

    // read archive data
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "trying to read archiv header\n");
    ndb_io_doQuadAlign(fArchive);
    retval = ndb_io_readchunkarchive(fArchive, &currArchive);

    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "error %d reading archiv header\n", retval);
        fclose(fArchive);
        exit (retval);
    }
    currBlockSize   = currArchive.blockSize;
    currCompression = currArchive.compression & 0xff;
    ndb_setBlockSize(currBlockSize);
    ndb_setCompression(currCompression);

    // copy archive header to rollback changes after errors
    memcpy(&saveArchive, &currArchive, sizeof(struct ndb_s_c_archive));

    // check compression method
    if (((currArchive.compression >> 8) & 0xff) != NDB_CURRENT_COMPRESSION)
    {
        fprintf(stderr, "\nError: archiv '%s' uses %s compression method\n", s_container,
                        (currArchive.compression >> 8) == NDB_COMPRESSION_ZLIB ? "ZLIB" : "BZIP2");
        fprintf(stderr, "Your NDB is compiled with %s compression method\n",
                        NDB_CURRENT_COMPRESSION == NDB_COMPRESSION_ZLIB ? "ZLIB" : "BZIP2");
        fflush(stderr);
        fclose(fArchive);
        exit (NDB_MSG_OTHERCOMPRESSIONALGO);
    }

    if (ndb_getDebugLevel() >= 2) fprintf(stdout, "archive file has %d chapters\n", currArchive.noOfChapters);
    if (ndb_getDebugLevel() >= 3) fprintf(stdout, "archive file created with block size %d\n", currBlockSize);
    if (ndb_getDebugLevel() >= 3) fprintf(stdout, "archive file created with compression %d\n", currCompression);
    if (ndb_getDebugLevel() >= 3) fprintf(stdout, "archive file created with data files size %lu\n", currArchive.dataFilesSize);
    if (ndb_getDebugLevel() >= 3) fprintf(stdout, "archive file created with NDB version %s\n", currArchive.ndbver);
    fprintf(stdout, "\n");

    // check option: modify options if not useful for empty archive
    if (currArchive.noOfChapters == 0)
    {
        // new created (=empty) archive file
        opt_comparemode = NDB_FILECOMPARE_EXACT;
        opt_newchapter  = 1;
        opt_lastchapter = 0;
    }


    // don't archive the archive file itself ;-)
    // this function checks the files list und sets all
    // matching files' action to NDB_ACTION_IGNORE
    ndb_list_excludeFileMask(&fileListExternal, s_container);
    // ignore data files also
    if (currArchive.dataFilesSize > 0)
    {
        sprintf (dummy, "%s_*", s_container);
        ndb_list_excludeFileMask(&fileListExternal, dummy);
    }


    ndb_block_init(currArchive.blockSize, currCompression);

    ndb_block_datachunk_init(&blockDataChunk, NDB_MAGIC_BLOCKS, 0, 0);
    ndb_block_datachunk_init(&blockHeaderChunk, NDB_MAGIC_BLOCKHEADER, 0, 0);
    ndb_block_datachunk_init(&dataFilesChunk, NDB_MAGIC_DATAFILE, 0, 0);

    ndb_df_init(&currArchive, fArchive, ndb_osdep_makeIntFileName(s_container));
    ndb_df_setMaxFileHandles(currMaxFHandles);


    // ========================================================================
    // read all existing chapters
    // ========================================================================

    i1 = 0;
    currFilePos = currArchive.firstChapter;

    if (currArchive.noOfChapters > 0)
    {
        pAllChapters = (struct ndb_s_c_chapter *) ndb_calloc (currArchive.noOfChapters, sizeof(struct ndb_s_c_chapter), "ndb_archive: pAllChapters");
        if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "Info: allocation memory for chapter list at 0x%lX with size 0x%lX\n",
                                (ULONG) pAllChapters, (ULONG) currArchive.noOfChapters * sizeof(struct ndb_s_c_chapter));
        if (pAllChapters == NULL)
        {
                fprintf(stderr, "Error: couldn't allocate memory for chapter list\n");
                fclose(fArchive);
                exit (NDB_MSG_NOMEMORY);
        }

        for (i1 = 0; i1 < currArchive.noOfChapters; i1++)
        {
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "Info: reading chapter %d at archive file position 0x%lX\n",
                                i1, currFilePos);
            fflush(stdout);
            fseek(fArchive, currFilePos, SEEK_SET);
                    ndb_io_doQuadAlign(fArchive);
            retval = ndb_io_readchunkchapter(fArchive, &pAllChapters[i1]);

            if (retval != NDB_MSG_OK)
            {
                fprintf(stderr, "Error: reading chapter %d from archive file\n", i1);
                fclose(fArchive);
                exit (retval);
            }
            // debug output level 7
            if (ndb_getDebugLevel() >= 8) ndb_chap_print(&pAllChapters[i1]);

            // read all block header of current chapter
            if (ndb_getDebugLevel() >= 3)
            {
                ndb_printTimeMillis();
                fprintf(stdout, "reading all %lu block headers of chapter %d\n", pAllChapters[i1].blockHeaNo, pAllChapters[i1].chapterNo);
                fflush(stdout);
            }
            retval = ndb_chap_readAllBlockHeader(&pAllChapters[i1], fArchive);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stderr, "Error: reading block header from chapter %d\n", i1);
                fclose(fArchive);
                exit (retval);
            }

            // read all data file header of current chapter
            if (ndb_getDebugLevel() >= 3)
            {
                ndb_printTimeMillis();
                fprintf(stdout, "reading all %u data file headers of chapter %d\n", pAllChapters[i1].noNewDatFil, pAllChapters[i1].chapterNo);
                fflush(stdout);
            }
            retval = ndb_chap_readAllDataFileHeader(&pAllChapters[i1], fArchive);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stderr, "Error: reading data file header from chapter %d\n", i1);
                fclose(fArchive);
                exit (retval);
            }

            // if fast compare then get file and header data of previous chapter
            if (opt_comparemode != NDB_FILECOMPARE_EXACT)
            {
                if (((opt_newchapter == 1) && (i1 == currArchive.noOfChapters - 1))
                    ||
                    ((opt_newchapter == 0) && (i1 == currArchive.noOfChapters - 2)))
                {
                    // init hash table for filenames
                    ndb_chap_initFileHash(&pAllChapters[i1]);
                    // now read all file entries
                    if (ndb_getDebugLevel() >= 3)
                    {
                        ndb_printTimeMillis();
                        fprintf(stdout, "reading all %lu file entries of chapter %d\n", pAllChapters[i1].filesNo, pAllChapters[i1].chapterNo);
                        fflush(stdout);
                    }
                    retval = ndb_chap_readAllFileEntries(&pAllChapters[i1], fArchive);
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stderr, "Error: reading all file entries of chapter %d resulted in %d\n",
                                        (&(pAllChapters[i1]))->chapterNo, retval);
                        fclose(fArchive);
                        exit(retval);
                    }
                    // all block header are read already, now add them to the last chapter
                    if (ndb_getDebugLevel() >= 3)
                    {
                        ndb_printTimeMillis();
                        fprintf(stdout, "rebuilding block header list for each file of chapter %d\n", pAllChapters[i1].chapterNo);
                        fflush(stdout);
                    }
                    retval = ndb_chap_rebuildBlockHeaderList(&pAllChapters[i1]);
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stderr, "Error: rebuilding block header list of chapter %d resulted in %d\n",
                                        (&(pAllChapters[i1]))->chapterNo, retval);
                        fclose(fArchive);
                        exit(retval);
                    }
                }
            }

            pLastChapter = &pAllChapters[i1];
            // position for next chapter
            currFilePos = pAllChapters[i1].ownFilePosition + NDB_CHUNKCHAPTER_SIZEALL
                                                           + pAllChapters[i1].chapterSize;
            fseek(fArchive, currFilePos, SEEK_SET);
        }
            // check all data files for existence and beeing data files
            ndb_df_checkAllDataFile();
    }
    else
    {
        // currently nothing to to here
    }


    // ========================================================================
    // use last chapter?
    // create new chapter?
    // ========================================================================

    if (opt_newchapter == 1)
    {
        // append new chapter
        if (ndb_getDebugLevel() >= 3)
        {
            ndb_printTimeMillis();
            fprintf(stdout, "appending new chapter to archive file\n\n");
        }
        // if no new chapter name was given create one
        if (opt_s_chaptername[0] == '\0')
        {
            snprintf(opt_s_chaptername, 200 - 1, "Chapter %04d", currArchive.noOfChapters);
        }

        ndb_chap_init(&currChapter, currArchive.noOfChapters, opt_s_chaptername, opt_s_chaptercomment,
                      NDB_VERSION, ndb_utf8_getConvertedCP());
        // init hash table for filenames
        ndb_chap_initFileHash(&currChapter);
        // add highest data file to new chapter to be sure to notice a change
    	if ((currArchive.dataFilesSize > 0) && (ndb_df_getFileCount() > 1))
    	{
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "ndb_archive: add latest data file to new chapter, ndb_df_getFileCount() = %d\n", ndb_df_getFileCount());
                fflush(stdout);
            }
            pNewDataFile = ndb_df_getCurrDataFile(ndb_df_getFileCount() - 1);
            if (pNewDataFile != NULL)
    	    {
                if (ndb_getDebugLevel() >= 7)
                    ndb_df_printDFHeadder(pNewDataFile, "ndb_archive: add latest data file to new chapter");
        		ndb_chap_addDataFileHeader(&currChapter, pNewDataFile);
        		currChapter.noNewDatFil++;
            }
    	}

        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "trying to write new chapter header\n");
        // position after last chapter data
        if (currArchive.noOfChapters > 0)
        {
            currFilePos = pLastChapter->ownFilePosition + NDB_CHUNKCHAPTER_SIZEALL + pLastChapter->chapterSize;
        }
        else
        {
            currFilePos = NDB_CHUNKARCHIVE_SIZEALL;
        }
        // do quad align for start of new chapter
        fseek(fArchive, currFilePos, SEEK_SET);
        ndb_io_doQuadAlign(fArchive);
        currFilePos = ftell(fArchive);
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "Info: seek file position 0x%lX for new chapter\n", currFilePos);
        // write new chapter
        retval = ndb_io_writechunkchapter(fArchive, &currChapter);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error writing chapter chunk\n");
            ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, NULL);
            exit(retval);
        }
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "Info: new chapter written\n");
        currArchive.noOfChapters++;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "Info: chapter count now set to %d\n", currArchive.noOfChapters);
        // append new block data chunk
        ndb_io_doQuadAlign(fArchive);
        retval =  ndb_io_writechunkblock(fArchive, &blockDataChunk);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error writing block data chunk\n");
            ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, NULL);
            exit(retval);
        }
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "Info: new block data chunk written\n");
    }
    else // ---------------------------------------------------------------
    {
        // - read last chapter & position after last block data of chapter
        if (ndb_getDebugLevel() >= 3)
        {
            ndb_printTimeMillis();
            fprintf(stdout, "using last chapter of archive file\n\n");
        }
        // fill block data chunk with the correct values from the archive file
        fseek(fArchive, pLastChapter->blockDatSta, SEEK_SET);
        retval =  ndb_io_readchunkblock(fArchive, &blockDataChunk);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error reading block data chunk from last chapter\n");
            fclose(fArchive);
            exit(retval);
        }

        // read all file entries
        retval = ndb_chap_readAllFileEntries(pLastChapter, fArchive);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error: reading all file entries of last chapter resulted in %d\n", retval);
            fclose(fArchive);
            exit(retval);
        }
        // all block header are read already, now add them to the last chapter
        retval = ndb_chap_rebuildBlockHeaderList(pLastChapter);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error: rebuilding block header list of last chapter resulted in %d\n", retval);
            fclose(fArchive);
            exit(retval);
        }
        // check external file list for already archived files
        retval = ndb_chap_checkFileListWithChapter(pLastChapter, &fileListExternal);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error: rebuilding block header list of last chapter resulted in %d\n", retval);
            fclose(fArchive);
            exit(retval);
        }

        // position at last chapter, start of block header chunk (overwrite later
        // with new block data)
        currFilePos = pLastChapter->blockHeaSta;
        fseek(fArchive, currFilePos, SEEK_SET);
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "Info: seek file position 0x%lX for appendig at last chapter\n", currFilePos);
        // set current chapter to last chapter
        currChapter = *pLastChapter;
        // correct pointer to previous chapter
        pLastChapter = &pAllChapters[currArchive.noOfChapters-2];
    }

    // set timestamp of chapter if choosen by user
    if (opt_timestamp == 1)
    {
        tim_m = calloc(1, sizeof (struct tm));
        // TODO: don't ignore timeflag (summer/winter)
        tim_m->tm_isdst = 0;
        // create year
        strncpy (dummy, opt_s_timestamp, 4);
        dummy[4] = '\0';
        tim_m->tm_year = atoi(dummy) - 1900;
        // create month
        strncpy (dummy, &opt_s_timestamp[4], 2);
        dummy[2] = '\0';
        tim_m->tm_mon = atoi(dummy) - 1;
        // create day
        strncpy (dummy, &opt_s_timestamp[6], 2);
        dummy[2] = '\0';
        tim_m->tm_mday = atoi(dummy);
        // create hour
        strncpy (dummy, &opt_s_timestamp[8], 2);
        dummy[2] = '\0';
        tim_m->tm_hour = atoi(dummy) - 1;
        // create minute
        strncpy (dummy, &opt_s_timestamp[10], 2);
        dummy[2] = '\0';
        tim_m->tm_min = atoi(dummy);
        // create second
        strncpy (dummy, &opt_s_timestamp[12], 2);
        dummy[2] = '\0';
        tim_m->tm_sec = atoi(dummy);
        // set timestamp
        currChapter.chapterTime = mktime(tim_m);
    }

    if (ndb_getDebugLevel() >= 5)
    {
        if (pLastChapter != NULL)
        {
            ndb_chap_printHashInfo(pLastChapter);
            fflush(stdout);
        }
    }

    if (ndb_getDebugLevel() >= 5)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "now starting archiving all files\n");
    }
    // current chapter contains how many files?
    filecount = currChapter.filesNo;

    // if first chapter and data file size limited, create a first extra data file
    if ((currArchive.dataFilesSize > 0) && (ndb_df_getFileCount() == 1))
    {
        // create new data file
        pNewDataFile = ndb_df_createNewDataFile(ndb_df_getFileCount(), NULL, NULL);
        pNewDataFile->firstChap = currChapter.chapterNo;
        pNewDataFile->lastChap  = currChapter.chapterNo;
        ndb_df_addDataFile(pNewDataFile);
        ndb_chap_addDataFileHeader(&currChapter, pNewDataFile);
        currChapter.noNewDatFil++;
        ndb_df_writeNewDataFile(1, pNewDataFile, ndb_df_getReadWriteHandle(1));
    }


    // ========================================================================
    // now pack all found files in the current archive chapter
    // ========================================================================


//    pCurrFile = ndb_list_getFirstFile(&fileListExternal);
    pCurrFile = ndb_list_removeFirstElement(&fileListExternal);

    if (pCurrFile == NULL)
    {
            fprintf(stdout, "warning: nothing to archive ...\n");
            fflush(stdout);
            ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, NULL);
            exit(NDB_MSG_NOTOK);
    }

    // write start message for archiving
    if (opt_newchapter == 1)
    {
		fprintf(stdout, "Archiving into new chapter %04d:\n", currChapter.chapterNo);
		fflush(stdout);
    }
    else
    {
		fprintf(stdout, "Archiving into last chapter %04d:\n", currChapter.chapterNo);
		fflush(stdout);
    }

    // this list will contain all files where compression failed
    ndb_list_initFileList(&fileListFailed, NDB_NO_HASHTABLE);

    // ========================================================================
    // start of loop over all found files
    // ========================================================================
    while ( pCurrFile != NULL )
    {
        strcpy(s_files, pCurrFile->filenameExt);

        // ignore empty dirs if -r set, but not -R
        if ( ((opt_recursedir > 0) && (opt_arcemptydir == 0))
           && (pCurrFile->children == 0)
           && ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR))
        {
            pCurrFile->action = NDB_ACTION_IGNORE;
            if (ndb_getDebugLevel() >= 3)
            {
                ndb_printTimeMillis();
                fprintf(stdout, "skipping empty dir '%s'\n", EMXWIN32CONVERT(s_files));
                fflush(stdout);
            }
        }

        // skip excluded files
        if (pCurrFile->action == NDB_ACTION_IGNORE)
        {
            if (ndb_getDebugLevel() >= 7)
            {
                ndb_printTimeMillis();
                fprintf(stdout, "skipping %s '%s'\n",
                                ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                EMXWIN32CONVERT(s_files));
                fflush(stdout);
            }
        }
        else if (pCurrFile->action == NDB_ACTION_KEEP)
        {
            if (ndb_getDebugLevel() >= 3)
            {
                ndb_printTimeMillis();
                fprintf(stdout, "keeping %s '%s'\n",
                                ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                EMXWIN32CONVERT(s_files));
                fflush(stdout);
             }
        }
        else if (pCurrFile->action == NDB_ACTION_ADD)
        {
            if (ndb_getDebugLevel() >= 2)
            {
                ndb_printTimeMillis();
                fprintf(stdout, "trying to add %s '%s'\n",
                                ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                EMXWIN32CONVERT(s_files));
                fflush(stdout);
            }

#if defined(UNIX)
            if (LSSTAT64(s_files, &stat64File) == -1)
#else
            if (LSSTAT(s_files, &statFile) == -1)
#endif
            {
                pCurrFile->action = NDB_ACTION_FAILED;
                ulFilesFailed++;
                fprintf(stderr, "error trying stat() for %s '%s': ",
                                ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                EMXWIN32CONVERT(s_files));
                ndb_fe_addRemark(pCurrFile, strerror (errno));
                perror(s_files);
                fflush(stderr);
            }
            else
            {
#if !defined(USE_FHANDLE_INT)
                // 'open file' only for files, not for directories
                iFile = NULL;
                if (((pCurrFile->attributes & NDB_FILEFLAG_DIR) != NDB_FILEFLAG_DIR)
                    && ((pCurrFile->attributes & NDB_FILEFLAG_SLNK) != NDB_FILEFLAG_SLNK))
                {
                    iFile = fopen(s_files, "rb");
                }
                // check for files wheather fopen() worked ...
                if (((pCurrFile->attributes & NDB_FILEFLAG_DIR) != NDB_FILEFLAG_DIR)
                     && ((pCurrFile->attributes & NDB_FILEFLAG_SLNK) != NDB_FILEFLAG_SLNK)
                     && (iFile == NULL))
                {
                    pCurrFile->action = NDB_ACTION_FAILED;
                    ulFilesFailed++;
                    fprintf(stdout, "error opening %s '%s': ",
                                ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                EMXWIN32CONVERT(s_files));
                    ndb_fe_addRemark(pCurrFile, strerror (errno));
                    perror(s_files);
                    fprintf(stdout, "%s\n", pCurrFile->remark);
                    fflush(stdout);
                    // set some values for error output later
#if defined(UNIX)
                    pCurrFile->mtime  = stat64File.st_mtime;
                    pCurrFile->lenOri8 = stat64File.st_size;
#else
                    pCurrFile->mtime  = statFile.st_mtime;
                    pCurrFile->lenOri8 = statFile.st_size;
#endif
                }
                else // fopen() worked or current entry is a directory or symbolic link
                {
#endif
/*
                    // lege FileEntry-Struktur zu Datei an
                    pFileEntry = (struct ndb_s_fileentry *) ndb_calloc (sizeof(struct ndb_s_fileentry), 1, s_files);
                    if (pFileEntry == NULL)
                    {
                        fprintf(stderr, "Error: couldn't allocate memory for %s '%s'\n",
                                    ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                    s_files);
                        ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
                        exit (NDB_MSG_NOMEMORY);
                    }
*/
pFileEntry = pCurrFile;
                    // copy info from stat()
/*
#if defined(UNIX)
                    retval = ndb_fe_newFileEntry(pFileEntry, s_files, &stat64File, filecount++);
#else
                    retval = ndb_fe_newFileEntry(pFileEntry, s_files, &statFile, filecount++);
#endif
*/
#if defined(UNIX)
                    retval = ndb_fe_setFileEntry(pFileEntry, &stat64File, filecount++);
#else
                    retval = ndb_fe_setFileEntry(pFileEntry, &statFile, filecount++);
#endif
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stderr, "Error: ndb_fe_newFileEntry(..) gave: %s\n",
                                ndb_util_msgcode2print(retval));
                        fflush(stderr);
                        ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
                        exit (retval);
                    }
                    // create internal filename (UTF-8 format)
                    ndb_fe_makeFilenameUTF8(pFileEntry);
/*
                    // additional informations
                    pFileEntry->attributes = pCurrFile->attributes;
                    pFileEntry->action = pCurrFile->action;
*/
                    // check for file size bigger than 2GB
                    if (ndb_getDebugLevel() >= 7)
                        fprintf(stdout, "ndb_archive: file size after stat()/before stat64() %.0f\n", (double) pFileEntry->lenOri8);
                    fflush(stdout);
                    // correct filesize to 64 bit
                    ndb_osdep_setFileSize64(pFileEntry);
                    // add fileentry to chapter hash table
                    if (currChapter.ppHashtableFiles != NULL)
                    {
                        retval = ndb_chap_addFileToHash(&currChapter, pFileEntry);
                        if (retval != NDB_MSG_OK)
                        {
                            fprintf(stderr, "Couldn't add '%s' to chapter hashtable\n", pFileEntry->filenameExt);
                            fprintf(stderr, "Error: %s\n", ndb_util_msgcode2print(retval));
                            fflush(stderr);
                            ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
                            exit (retval);
                        }
                    }
                    // add fileentry to chapter
                    retval = ndb_chap_addFileEntry(&currChapter, pFileEntry);
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stderr, "Couldn't add '%s' to chapter file list\n", pFileEntry->filenameExt);
                        fprintf(stderr, "Error: %s", ndb_util_msgcode2print(retval));
                        fflush(stderr);
                        ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
                        exit (retval);
                    }
                    pFileEntry->chapNo = currChapter.chapterNo;


                    // compress file and write zipped blocks to archive file
                    if (((pFileEntry->attributes & NDB_FILEFLAG_DIR) != NDB_FILEFLAG_DIR)
                        && ((pFileEntry->attributes & NDB_FILEFLAG_SLNK) != NDB_FILEFLAG_SLNK))
                    {
                        // compare file content
                        if (opt_comparemode == NDB_FILECOMPARE_EXACT)
                        {
                            doCompress = 1;
                        }
                        // compare current file with file from previous chapter?
                        else
                        {
                            if (ndb_getDebugLevel() >= 5)
                                fprintf(stdout, "searching for '%s' in last chapter\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
                            fflush(stdout);
                            // default: compression of file can't be skipped
                            doCompress = 1;
                            if (NDB_MSG_OK == ndb_chap_findfile(pLastChapter, pFileEntry, &pOldFile))
                            {
                                if (ndb_getDebugLevel() >= 5)
                                    fprintf(stdout, "file found in last chapter\n");
                                fflush(stdout);
                                if (opt_comparemode == NDB_FILECOMPARE_FAST)
                                    retval2 = ndb_file_checkattributes(pFileEntry, pOldFile);
                                if (opt_comparemode == NDB_FILECOMPARE_CRC32)
                                    retval2 = ndb_file_checkfileswithcrc(pFileEntry, pOldFile);
                                if (opt_comparemode == NDB_FILECOMPARE_MD5)
                                    retval2 = ndb_file_checkfileswithmd5(pFileEntry, pOldFile);
                                if (NDB_MSG_OK == retval2)
                                {
                                    if (ndb_getDebugLevel() >= 2)
                                        fprintf(stdout, "found file with same attributes in previous chapter -> assuming both are identical\n");
                                    fflush(stdout);
                                    // file found in previous chapter and with same file attributes:
                                    // therefore new compression of file is not neccessary
                                    doCompress = 0;
                                }
                            }
                        }

                        if (doCompress == 0)
                        {
                            // file identical to previous chapter, no need to proceed again
                            // -> copy only block header list
                            ndb_file_copyheaderlist(&currChapter, pFileEntry, pOldFile);
                            // a little bit statistics
                            ul8BlocksOldCount   += pFileEntry->blockCount;
                            ul8BlocksOldLenOri  += pFileEntry->lenOri8;
                            ul8BlocksOldLenZip  += pFileEntry->lenZip8;
                        }
                        else
                        {
                            // NDB_FILECOMPARE_EXACT or file is different to previous chapter
#if defined(USE_FHANDLE_INT)
                            retval = ndb_compressFile(&currChapter, pFileEntry, fArchive, s_files);
#else
                            retval = ndb_compressStream(&currChapter, pFileEntry, fArchive, iFile);
#endif
                            if (retval != NDB_MSG_OK)
                            {
                                pCurrFile->action = NDB_ACTION_FAILED;
                                pFileEntry->action = NDB_ACTION_FAILED;
                                ulFilesFailed++;
                                // add values for error output later
                                pCurrFile->mtime = pFileEntry->mtime;
                                pCurrFile->lenOri8 = pFileEntry->lenOri8;
                                if ((retval == NDB_MSG_NOFILEOPEN) || (retval == NDB_MSG_READERROR))
                                {
#if defined(WIN32)
                                    if (ndb_getDebugLevel() >= 5)
                                    {
                                        fprintf(stdout, "ndb_compressFile: CreateFile error (%d) for '%s'\n",
                                                        (int) GetLastError(), EMXWIN32CONVERT(s_files));
                                    }
                                    fprintf(stdout, "error opening %s '%s': ",
                                            ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                            EMXWIN32CONVERT(s_files));
                                    fflush(stdout);

                                    if (FormatMessage(
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_IGNORE_INSERTS,
                                        NULL,
                                        GetLastError(),
                                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                        (LPTSTR) &lpMsgBuf,
                                        0,
                                        NULL ))
                                    {
                                        // remove \n at end of error text
                                         ndb_fe_addRemark(pCurrFile, lpMsgBuf);
                                        if ((strlen(pCurrFile->remark) > 0) && (pCurrFile->remark[strlen(pCurrFile->remark) - 1] == '\n'))
                                            pCurrFile->remark[strlen(pCurrFile->remark) - 1] = '\0';
                                    }
#else
                                    fprintf(stdout, "Error opening %s '%s': ",
                                                ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                                                EMXWIN32CONVERT(s_files));
                                    ndb_fe_addRemark(pCurrFile, strerror (errno));
                                    perror(s_files);
#endif
                                    fprintf(stdout, "%s\n", pCurrFile->remark);
                                    fflush(stdout);
                                }
                                else
                                {
                                    fprintf(stderr, "Error: couldn't read file content -> ");
                                    fflush(stderr);
                                    ndb_fe_addRemark(pCurrFile, ndb_util_msgcode2print(retval));
                                }

                                // remove fileentry from chapter hash table
                                if (currChapter.ppHashtableFiles != NULL)
                                {
                                    retval = ndb_chap_removeFileFromHash(&currChapter, pFileEntry);
                                    if (retval != NDB_MSG_OK)
                                    {
                                        fprintf(stderr, "Error: %s", ndb_util_msgcode2print(retval));
                                        fflush(stderr);
                                        // exit (retval);
                                    }
                                }
                                // remove fileentry from chapter
                                retval = ndb_chap_removeLastFileEntry(&currChapter);
                                if (retval != NDB_MSG_OK)
                                {
                                    fprintf(stderr, "Error: %s", ndb_util_msgcode2print(retval));
                                    fflush(stderr);
                                    // exit (retval);
                                }
                                // add file to list of failed files
                                pFileEntry->nextFile = NULL;
                                pFileEntry->sameCRC  = NULL;
                                ndb_list_addToFileList(&fileListFailed, pFileEntry);
//                                exit (retval);
                            }
                            // track number of bytes read
                            db_bytesread += pFileEntry->lenOri8;
                            db_byteswritten += pFileEntry->lenZip8;
                        }

                        // close file in all cases
#if !defined(USE_FHANDLE_INT)
                        fclose(iFile);
#endif
                    }
                    else
                    {
                        // make sure that added dirs/symlinks don't have file content
                        pFileEntry->lenOri8 = 0;
                        pFileEntry->lenZip8 = 0;
                        memset(pFileEntry->md5, 0, MD5_DIGESTSIZE);
                        pFileEntry->blockCount = 0;
                    }
                    // update the block header count of the block data/header chunk
                    blockDataChunk.dataCount   = currChapter.blockDatNo;
                    blockHeaderChunk.dataCount = currChapter.blockHeaNo;
                    currChapter.allFileLenOri += pFileEntry->lenOri8;

                    // now get extra info if available
                    sText[0] = '\0';
                    // should we look for extra data?
                    if ((opt_extradata == 1) && !(pCurrFile->action == NDB_ACTION_FAILED))
                    {
                        if (ndb_getDebugLevel() >= 3)
                        {
                            fprintf(stdout, "get extra data from file\n");
                            fflush(stdout);
                        }
                        retval = ndb_osdep_getExtraData(pFileEntry, &pExtraBlocks, &pExtraHea, fArchive, sText);
                        if (retval == NDB_MSG_EXTRAFAILED)
                        {
                            ulFilesFailedExtra++;
                            pFileEntry->action = NDB_ACTION_FAILEDEXTRA;
/*
                            pCurrFile->action = NDB_ACTION_FAILEDEXTRA;
                            // add values for error output later
                            pCurrFile->mtime = pFileEntry->mtime;
                            pCurrFile->lenOri8 = pFileEntry->lenOri8;
*/
                            // remove file flag hasExtra
                            if ((pFileEntry->flags & NDB_FILEFLAG_HASEXTRA) == NDB_FILEFLAG_HASEXTRA)
                                pFileEntry->flags = pFileEntry->flags - NDB_FILEFLAG_HASEXTRA;
                        }
                        else if (((retval == NDB_MSG_OK)) && (pExtraHea != NULL))
                        {
                            // save only if content
                            retval = ndb_addExtraData(&currChapter, pFileEntry, fArchive, pExtraBlocks, &sizeExtra, pExtraHea);
                            if (retval != NDB_MSG_OK)
                            {
                                fprintf(stderr, "Error: ndb_addExtraData(..) gave: %s\n",
                                        ndb_util_msgcode2print(retval));
                                fflush(stderr);
                                ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
                                exit (retval);
                            }
                            // update the block header count of the block data/header chunk
                            blockDataChunk.dataCount   = currChapter.blockDatNo;
                            blockHeaderChunk.dataCount = currChapter.blockHeaNo;
                            currChapter.allFileLenOri += pFileEntry->lenExtHea;
                            // print the rest of the output
                            if (ndb_getDebugLevel() >= 2)
                            {
                                ndb_printTimeMillis();
                            }
                        }
                        else
                        {
                            // file has no extra data
                            if (ndb_getDebugLevel() >= 5)
                            {
                                fprintf(stdout, "File '%s' has no extra data\n", pFileEntry->filenameExt);
                                fflush(stdout);
                            }
                        }
                    }
                    if (pCurrFile->action != NDB_ACTION_FAILED)
                    {
                        if (pFileEntry->lenOri8 > 0)
                        {
                            fprintf(stdout, "added %s '%s' (%.0fb, packed %d%%%s)\n",
                                    ((pFileEntry->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir " : "file",
                                    EMXWIN32CONVERT(s_files),
                                    (double) pFileEntry->lenOri8,
                                    100 - (int) ((pFileEntry->lenZip8 * 100.0)/pFileEntry->lenOri8), sText);
                            fflush(stdout);
                        }
                        else
                        {
                            fprintf(stdout, "added %s '%s' (0b%s)\n",
                                    ((pFileEntry->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir " : "file",
                                     EMXWIN32CONVERT(s_files),
                                     sText);
                            fflush(stdout);
                        }
                        ulFilesSaved++;
                    }
                    else
                    {
                        fprintf(stdout, "error adding %s '%s'\n",
                                ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir " : "file",
                                 EMXWIN32CONVERT(s_files));
                        fflush(stdout);
                    }
#if !defined(USE_FHANDLE_INT)
                } // if (iFile == NULL)
#endif
            } // if (stat(s_files, &statFile) == -1)

            // wait if specified and if pCurrFile->action == NDB_ACTION_ADD
            if (opt_waitmillis > 0)
            {
                ndb_osdep_sleep(opt_waitmillis);
            }

        } // pCurrFile->action == NDB_ACTION_ADD/KEEP/IGNORE
        else
        {
            fprintf(stdout, "unknown action '%c': skipping %s '%s'\n",
                            pCurrFile->action,
                            ((pCurrFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR) ? "dir" : "file",
                            EMXWIN32CONVERT(s_files));
            fflush(stdout);

        }


        if (ndb_getDebugLevel() >= 8)
        {
            ndb_show_ptr();
            fflush(stdout);
        }

        // go to next file
//        pCurrFile = ndb_list_getNextFile(&fileListExternal);
        pCurrFile = ndb_list_removeFirstElement(&fileListExternal);

    } // while ( pCurrFile != NULL )
    // ========================================================================
    // end of loop over all found files
    // ========================================================================

    // close last data file
    if (currArchive.dataFilesSize > 0)
        ndb_df_closeDataFile(ndb_df_getCurrWriteFileNo());

    // print hash table info
    if (ndb_getDebugLevel() >= 2)
        ndb_block_printHashInfo();

        // recycle variable currFilePos
        currFilePos = ftell(fArchive);

    // write block header chunk to file
    ndb_io_doQuadAlign(fArchive);
    retval =  ndb_io_writechunkblock(fArchive, &blockHeaderChunk);
    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "Error writing block data chunk\n");
        ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
        exit(retval);
    }
    // write all block header to archive file
    if (ndb_getDebugLevel() >= 3)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "write all block header ========================================\n");
        fflush(stdout);
    }
    ndb_chap_writeAllBlockHeader(&currChapter, fArchive);

    // track archive size change
    db_byteswritten += ftell(fArchive) - currFilePos;
    currFilePos = ftell(fArchive);

    // write file entry chunk to file
    ndb_block_datachunk_init(&fileEntryChunk, NDB_MAGIC_FILEHEADER, 0, 0);
    ndb_io_doQuadAlign(fArchive);
    retval =  ndb_io_writechunkblock(fArchive, &fileEntryChunk);
    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "Error writing file entry chunk\n");
        ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
        exit(retval);
    }
    // write all file entries
    if (ndb_getDebugLevel() >= 3)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "write all file entries ========================================\n");
        fflush(stdout);
    }
    ndb_chap_writeAllFileEntries(&currChapter, fArchive);

        // track archive size change
        db_byteswritten += ftell(fArchive) - currFilePos;
        currFilePos = ftell(fArchive);

    // write data files header chunk to file
    ndb_io_doQuadAlign(fArchive);
    retval =  ndb_io_writechunkblock(fArchive, &dataFilesChunk);
    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "Error writing data files data chunk\n");
        ndb_cleanUpAfterError(s_container, fArchive, &saveArchive, &currChapter);
        exit(retval);
    }
    // write all data files header to archive file
    if (ndb_getDebugLevel() >= 3)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "write all data files header ===================================\n");
        fflush(stdout);
    }
    ndb_chap_writeAllDatFilesHeader(&currChapter, fArchive);

        // track archive size change
        db_byteswritten += ftell(fArchive) - currFilePos;

    // save end of chapter file position
    highestFilePos = ftell(fArchive);
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "end of file at 0x%lX\n", highestFilePos);


    // now calculate all size and file offset data ...
    fprintf(stdout, "\n");
    if (ndb_getDebugLevel() >= 2)
    {
        ndb_printTimeMillis();
    }
    fprintf(stdout, "now updating archive inventory\n");
    fflush(stdout);

    // update archive chunk
    currArchive.lastChapter    = currChapter.ownFilePosition;
    currArchive.archiveSize    = highestFilePos - NDB_CHUNKARCHIVE_SIZEALL;
    currArchive.firstChapter   = ((NDB_CHUNKARCHIVE_SIZEALL & 3) > 0) ?
                                  (NDB_CHUNKARCHIVE_SIZEALL & 0xfffffffc) + 4 : NDB_CHUNKARCHIVE_SIZEALL;
    currArchive.lastChapter    = currChapter.ownFilePosition;
    currArchive.dataFilesCount  = ndb_df_getFileCount() - 1;
    // update block data chunk
    blockDataChunk.dataSize    = blockHeaderChunk.ownFilePosition - blockDataChunk.ownFilePosition
                                                                  - NDB_SAVESIZE_BLOCKCHUNK;
    // update block header chunk
    blockHeaderChunk.dataSize  = fileEntryChunk.ownFilePosition - blockHeaderChunk.ownFilePosition
                                                                - NDB_SAVESIZE_BLOCKCHUNK;
    // update file entry chunk
    fileEntryChunk.dataSize    = dataFilesChunk.ownFilePosition - fileEntryChunk.ownFilePosition
                                                                - NDB_SAVESIZE_BLOCKCHUNK;
    // update data files header chunk
    dataFilesChunk.dataSize    = highestFilePos - dataFilesChunk.ownFilePosition
                                                - NDB_SAVESIZE_BLOCKCHUNK;
    // update chapter chunk
    currChapter.blockDatSta    = blockDataChunk.ownFilePosition;
    currChapter.blockDatLen    = blockDataChunk.dataSize;
    currChapter.blockHeaSta    = blockHeaderChunk.ownFilePosition;
    currChapter.blockHeaLen    = blockHeaderChunk.dataSize;
    currChapter.filesListSta   = fileEntryChunk.ownFilePosition;
    currChapter.filesListLen   = fileEntryChunk.dataSize;
    currChapter.staDatFil      = dataFilesChunk.ownFilePosition;
    currChapter.dataFilesHeaLen  = dataFilesChunk.dataSize;
    currChapter.chapterSize    = highestFilePos - (currChapter.ownFilePosition + NDB_CHUNKCHAPTER_SIZEALL);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "chapter start   is 0x%lX\n", currChapter.ownFilePosition);
        fprintf(stdout, "chunk size      is 0x%x\n",  NDB_CHUNKCHAPTER_SIZEALL);
        fprintf(stdout, "-> chapter size is 0x%llX\n", currChapter.chapterSize);
    }
    // ... save corrected archive chunk again
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "and write them again ===========================================\n");
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "chunk 'archive' at %lX again ...\n", currArchive.ownFilePosition);
    fseek(fArchive, currArchive.ownFilePosition, SEEK_SET);
    retval = ndb_io_writechunkarchive(fArchive, &currArchive);
    // ... save corrected chapter chunk again
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "chunk 'chapter' at %lX again ...\n", currChapter.ownFilePosition);
    fseek(fArchive, currChapter.ownFilePosition, SEEK_SET);
    retval = ndb_io_writechunkchapter(fArchive, &currChapter);
    // ... save corrected block data chunk again
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "chunk 'block data' at %lX again ...\n", blockDataChunk.ownFilePosition);
    fseek(fArchive, blockDataChunk.ownFilePosition, SEEK_SET);
    retval =  ndb_io_writechunkblock(fArchive, &blockDataChunk);
    // ... save corrected block header chunk again
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "chunk 'block header' at %lX again ...\n", blockHeaderChunk.ownFilePosition);
    fseek(fArchive, blockHeaderChunk.ownFilePosition, SEEK_SET);
    retval =  ndb_io_writechunkblock(fArchive, &blockHeaderChunk);
    // ... save corrected file entry chunk again
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "chunk 'file entry' at %lX again ...\n", fileEntryChunk.ownFilePosition);
    fseek(fArchive, fileEntryChunk.ownFilePosition, SEEK_SET);
    retval = ndb_io_writechunkblock(fArchive, &fileEntryChunk);
    // ... save corrected data files header chunk again
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "chunk 'data files header' at %lX again ...\n", dataFilesChunk.ownFilePosition);
    fseek(fArchive, dataFilesChunk.ownFilePosition, SEEK_SET);
    retval = ndb_io_writechunkblock(fArchive, &dataFilesChunk);

    fclose(fArchive);


    // show all failed files
    if (ulFilesFailed > 0)
    {
        pCurrFile = ndb_list_getFirstFile(&fileListFailed);
        if (pCurrFile != NULL)
        {
            fprintf(stdout, "\nWarning: following %lu file(s) could not be archived:\n", ulFilesFailed);
            while ( pCurrFile != NULL )
            {
                if (pCurrFile->action == NDB_ACTION_FAILED)
                {
                    // create time string
                    tim_m = localtime( (const time_t *) &pCurrFile->mtime);
                    if (tim_m != NULL)
                        strftime(stime_m, 40, "%Y-%m-%d, %H:%M:%S", tim_m);
                    else
                    {
                        strcpy(stime_m, "[DATE ERROR]");
                        fprintf(stderr, "warning: creation date of file '%s' corrupt\n", EMXWIN32CONVERT(pCurrFile->filenameExt));
                        fflush (stderr);
                    }

                    // copy error message if existing
                    if (pCurrFile->remark != NULL)
                        snprintf (dummy, 200 - 1, ": %s", pCurrFile->remark);
                    else
                                                dummy[0] = '\0';
                                        // make output
                    fprintf(stdout, "%s (%.0f bytes, %s)%s\n", EMXWIN32CONVERT(pCurrFile->filenameExt),
                                    (double) pCurrFile->lenOri8, stime_m, dummy);
                }
                // go to next file
                pCurrFile = ndb_list_getNextFile(&fileListFailed);
            }
        }
        fflush (stdout);
    }

    // show all failed files
    if (ulFilesFailedExtra > 0)
    {
        pCurrFile = currChapter.pFirstFile;
        if (pCurrFile != NULL)
        {
            fprintf(stdout, "\nWarning: couldn't check for existence of extra data or archiving\n");
            fprintf(stdout, "of extra data failed for following %lu file(s):\n", ulFilesFailedExtra);
            while ( pCurrFile != NULL )
            {
                if (pCurrFile->action == NDB_ACTION_FAILEDEXTRA)
                {
                    // create time string
                    tim_m = localtime( (const time_t *) &pCurrFile->mtime);
                    if (tim_m != NULL)
                        strftime(stime_m, 40, "%Y-%m-%d, %H:%M:%S", tim_m);
                    else
                    {
                        strcpy(stime_m, "[DATE ERROR]");
                        fprintf(stderr, "warning: creation date of file '%s' corrupt\n", EMXWIN32CONVERT(pCurrFile->filenameExt));
                        fflush (stderr);
                    }
                    // copy error message if existing
                    if (pCurrFile->remark != NULL)
                        snprintf (dummy, 200 - 1, ": %s", pCurrFile->remark);
                    else
                                                dummy[0] = '\0';
                                        // make output
                    fprintf(stdout, "%s (%.0f bytes, %s)%s\n", EMXWIN32CONVERT(pCurrFile->filenameExt),
                                    (double) pCurrFile->lenOri8, stime_m, dummy);
                }
                // go to next file
                pCurrFile = pCurrFile->nextFile;
            }
        }
        fflush (stdout);
    }


/*
    // free the contents of the original filelist
    pCurrFile = ndb_list_removeFirstElement(&fileListExternal);
    while (pCurrFile != NULL)
    {
        ndb_free(pCurrFile->filenameUTF8);
        ndb_free(pCurrFile->filenameExt);
        ndb_free(pCurrFile);
        // go to next file
        pCurrFile = ndb_list_removeFirstElement(&fileListExternal);
    }
*/


#if !defined(NDB_DEBUG_ALLOC_OFF)
    ndb_show_status();
    ndb_alloc_done();
    if (ndb_getDebugLevel() >= 5)
        ndb_show_ptr();
#endif

    time_finished = 0;
    time(&time_finished);
    db_duration = difftime(time_finished, time_startup);

    // write summary
    fprintf(stdout, "\n\nAdded %.0f new blocks (original: %.0fb, zipped: %.0fb),\n",
                    (double) ul8BlocksNewCount, (double) ul8BlocksNewLenOri, (double) ul8BlocksNewLenZip);
    fprintf(stdout, "reused %.0f already archived blocks (original: %.0fb, zipped: %.0fb)\n",
                    (double) ul8BlocksOldCount, (double) ul8BlocksOldLenOri, (double) ul8BlocksOldLenZip);
    fprintf(stdout, "\n");
    fprintf(stdout, "Archived %lu files/directories, failed %lu, failed extra data %lu\n", ulFilesSaved, ulFilesFailed, ulFilesFailedExtra);
    // track number of bytes read
    fprintf(stdout, "Needed %.0f seconds, read %.0f bytes", db_duration, db_bytesread);
    if (db_duration > 0)
        fprintf(stdout, " (%.2f KB/s)", (db_bytesread / 1024.0) / db_duration);
    fprintf(stdout, ", written %.0f bytes", db_byteswritten);
    if (db_duration > 0)
        fprintf(stdout, " (%.2f KB/s)", (db_byteswritten / 1024.0) / db_duration);
    fprintf(stdout, "\n");
    fprintf(stdout, "\nNDBArchive: finished at %s", ctime(&time_finished));
    fflush(stdout);

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
        ndb_util_print_performancetest();
    }
#endif

    if ((ulFilesFailed == 0) && (ulFilesFailedExtra == 0))
        return(NDB_MSG_OK);
    else
        return(NDB_MSG_NOTOK);
}



// ======================================================================================


/*
    *************************************************************************************
    ndb_cleanUpAfterError()

    try to close all files, truncate main archive file and so on
    *************************************************************************************
*/

int ndb_cleanUpAfterError(char *s_container, FILE *fArchive, PARCHIVE pOldArchive, PCHAPTER pChapter)
{
    int retval = NDB_MSG_OK;
    ULONG currSize = 0;

    if (pOldArchive != NULL)
    {
        // replace (perhaps) modified values with the originales
        fseek(fArchive, pOldArchive->ownFilePosition, SEEK_SET);
        retval = ndb_io_writechunkarchive(fArchive, pOldArchive);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stdout, "\nError: couldn't rollback NDB main archive file '%s'\n", s_container);
            fprintf(stdout, "The NDB archive might be corrupt now!\n");
            fflush(stdout);
        }
        // check current size of archive
        fseek(fArchive, 0, SEEK_END);
        currSize = ftell(fArchive);
    }

    if (fArchive != NULL)
        fclose(fArchive);

    if ((pOldArchive != NULL)
        && (pOldArchive->archiveSize + NDB_CHUNKARCHIVE_SIZEALL> 0) && (currSize > 0)
        && (pOldArchive->archiveSize + NDB_CHUNKARCHIVE_SIZEALL < currSize))
    {
        fprintf(stdout, "Truncating NDB main archive file '%s'\n", s_container);
        fprintf(stdout, "from current size of %lub to previous %.0fb.\n",
                currSize, (double) (pOldArchive->archiveSize + NDB_CHUNKARCHIVE_SIZEALL));
        fflush(stdout);
        ndb_osdep_truncate(s_container, pOldArchive->archiveSize + NDB_CHUNKARCHIVE_SIZEALL);
    }

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
    ndb_expandExtraStream()

        TODO: Dummy function to satisfy the linker, has to be corrected
    *************************************************************************************
*/
int ndb_expandExtraStream(struct ndb_s_fileentry *pFile,
                          FILE *fArchive, const char *sDefaultPath, int opt_testintegrity)
{
        return NDB_MSG_OK;
}


