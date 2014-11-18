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



/* ======================================================================== */
/* functions for writing and reading the file entries                       */
/* ======================================================================== */

/*
    *************************************************************************************
	ndb_io_writeFileEntry()

	save the current archive file position to the file entry structure (to allow
	later writing of corrected structure data easily);
	writes the file entry structure to the archive file handle;
	only if extra data exists write the meta data of extra data also
    *************************************************************************************
*/

int ndb_io_writeFileEntry(FILE *fArchive, struct ndb_s_fileentry *pFileEntry)
{
	struct ndb_s_fileextradataheader *pCurrExtraHea = NULL;
    int count, i;
    int retval = NDB_MSG_OK;
    int lenAllExtraHea = 0;
    int ulSize = 0;
    int count_crcbefore = 0;
    int count_crcafter  = 0;
    static char *buf = NULL;

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_io_writeFileEntry - startup\n");
    fflush(stdout);

	if (buf == NULL)
        buf = ndb_calloc(NDB_SAVESIZE_FILEENTRY + NDB_MAXLEN_FILENAME + 65535, 1, "ndb_io_writeFileEntry: write buffer");
	if (buf == NULL)
	{
        fprintf(stderr, "ndb_io_writeFileEntry: couldn't allocate memory for write buffer\n");
		fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
	}

    count = 0;
    pFileEntry->crc16Hea = 0;

    count_crcbefore = count;
    count = ndb_write2buf_ushort (buf, &count, pFileEntry->crc16Hea);
    count_crcafter  = count;
    count = ndb_write2buf_ulong  (buf, &count, pFileEntry->fileNo);
    count = ndb_write2buf_ushort (buf, &count, pFileEntry->chapNo);
    count = ndb_write2buf_ulong8 (buf, &count, pFileEntry->lenOri8);
    count = ndb_write2buf_ulong8 (buf, &count, pFileEntry->lenZip8);
//    count = ndb_write2buf_ulong  (buf, &count, pFileEntry->crc32);
    count = ndb_write2buf_byte_n (buf, &count, pFileEntry->md5, MD5_DIGESTSIZE);
    count = ndb_write2buf_ulong  (buf, &count, pFileEntry->ctime);
    count = ndb_write2buf_ulong  (buf, &count, pFileEntry->mtime);
    count = ndb_write2buf_ulong  (buf, &count, pFileEntry->atime);
    count = ndb_write2buf_ulong  (buf, &count, pFileEntry->attributes);
    count = ndb_write2buf_ulong  (buf, &count, pFileEntry->blockCount);
    count = ndb_write2buf_char   (buf, &count, pFileEntry->FSType);
    count = ndb_write2buf_char   (buf, &count, pFileEntry->flags);
    count = ndb_write2buf_ushort (buf, &count, pFileEntry->nr_ofExtraHea);
    count = ndb_write2buf_ushort (buf, &count, pFileEntry->lenExtHea);
    // write filename
    count = ndb_write2buf_ushort (buf, &count, pFileEntry->lenfilenameUTF8);
    count = ndb_write2buf_char_n (buf, &count, pFileEntry->filenameUTF8, pFileEntry->lenfilenameUTF8);

    // write extra data
    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_io_writeFileEntry: now writing %d extra data header\n", pFileEntry->nr_ofExtraHea);
    fflush(stdout);
    // check for consistency
    if ((pFileEntry->nr_ofExtraHea > 0) && ((pFileEntry->flags & NDB_FILEFLAG_HASEXTRA) != NDB_FILEFLAG_HASEXTRA))
    {
        pFileEntry->flags |= NDB_FILEFLAG_HASEXTRA;
        fprintf(stdout, "warning: file '%s' has extra data, but no extra data flag (now corrected)\n", pFileEntry->filenameExt);
        fflush(stdout);
    }
    pCurrExtraHea = pFileEntry->pExtraHeader;
    for (i = 0; i < pFileEntry->nr_ofExtraHea; i++)
    {
		if (pCurrExtraHea == NULL)
		{
			fprintf(stdout, "ndb_io_writeFileEntry: internal error - less extra header found than nr_ofExtraHea\nfor '%s'\n", pFileEntry->filenameUTF8);
			fflush(stdout);
			// cut number to really existing header number
			pFileEntry->nr_ofExtraHea = i;
			break;
		}
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_io_writeFileEntry: now writing extra data header %d\n", i);
        fflush(stdout);
	    count = ndb_write2buf_ushort (buf, &count, pCurrExtraHea->typExtHea);
	    count = ndb_write2buf_ushort (buf, &count, pCurrExtraHea->lenExtHea);
	    count = ndb_write2buf_ulong  (buf, &count, pCurrExtraHea->lenOriExtra);
	    count = ndb_write2buf_ulong  (buf, &count, pCurrExtraHea->lenZipExtra);
	    count = ndb_write2buf_ulong  (buf, &count, pCurrExtraHea->crc32Extra);
	    count = ndb_write2buf_ushort (buf, &count, pCurrExtraHea->blockCountExtra);
		if (pCurrExtraHea->lenExtHea > NDB_SAVESIZE_FILEEXTRAHEADER)
		{
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_io_writeFileEntry: now writing %d bytes of direct data for extra data header %d\n",
                                pCurrExtraHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER, i);
            fflush(stdout);
		    count = ndb_write2buf_byte_n (buf, &count, pCurrExtraHea->pData, pCurrExtraHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER);
		}
		lenAllExtraHea += pCurrExtraHea->lenExtHea;
		// next header
		pCurrExtraHea = pCurrExtraHea->pNextHea;
    }

    // calculate crc16 for buffer with header struct
    pFileEntry->crc16Hea = ndb_crc16(0, &buf[count_crcafter], count - count_crcafter);
    // write crc16 into buffer
    ndb_write2buf_ushort(buf, &count_crcbefore, pFileEntry->crc16Hea);

    // now save it to disk
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_io_writeFileEntry: now save buffer (%d bytes) to disk\n", count);
    fflush(stdout);
/*
    if ((pFileEntry->flags & NDB_FILEFLAG_HASEXTRA) == NDB_FILEFLAG_HASEXTRA)
    {
*/
        if (count == NDB_SAVESIZE_FILEENTRY + pFileEntry->lenfilenameUTF8 + lenAllExtraHea)
            ulSize = ndb_io_writedata(buf, count, fArchive);
        else
        {
            fprintf(stderr, "ndb_io_writeFileEntry: internal error: expected length of buffer %d, found %d\n",
                            NDB_SAVESIZE_FILEENTRY + pFileEntry->lenfilenameUTF8 + lenAllExtraHea, count);
            fprintf(stderr, "                       for file '%s'\n", pFileEntry->filenameExt);
            fprintf(stderr, "                       savesize %d, lenUTF8 %d, lenExtra %d\n",
                    NDB_SAVESIZE_FILEENTRY, pFileEntry->lenfilenameUTF8, lenAllExtraHea);
            fflush (stderr);
//            ndb_free(buf);
            return NDB_MSG_NOTOK;
        }
/*
        }
    else
    {
        if (count == (NDB_SAVESIZE_FILEENTRY + pFileEntry->lenfilenameUTF8))
            ulSize = ndb_io_writedata(buf, count, fArchive);
        else
        {
            fprintf(stderr, "ndb_io_writeFileEntry: internal error: expected length of buffer %d, found %d\n",
                            NDB_SAVESIZE_FILEENTRY+ pFileEntry->lenfilenameUTF8, count);
            fprintf(stderr, "                       for file '%s'\n", pFileEntry->filenameExt);
            fprintf(stderr, "                       savesize %d, lenUTF8 %d, lenExtra %d\n",
                    NDB_SAVESIZE_FILEENTRY, pFileEntry->lenfilenameUTF8, lenAllExtraHea);
            fflush (stderr);
//            ndb_free(buf);
            return NDB_MSG_NOTOK;
        }
    }
*/

    if (ulSize != count)
    {
        fprintf(stdout, "tried to write %d bytes but could only %d bytes\n",
                        count, ulSize);
        fflush (stdout);
//        ndb_free(buf);
        return NDB_MSG_WRITEERROR;
    }

    // free resources
//    ndb_free(buf);

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_io_writeFileEntry - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
	ndb_io_readFileEntry()

	reads the file entry structure from the archive file handle;
	only if extra data exists (file flag contains NDB_FILEFLAG_HASEXTRA), read the
	meta data of extra data also (otherwise set it to zero)
    *************************************************************************************
*/

int ndb_io_readFileEntry(FILE *fArchive, struct ndb_s_fileentry *pFileEntry)
{
	struct ndb_s_fileextradataheader *pCurrExtraHea = NULL;
    int count, i;
    static char *buf = NULL;
    ULONG ulSize = 0L;
    int count_aftercrc = 0;
    USHORT crc16 = 0;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 6)
    	fprintf(stdout, "ndb_io_readFileEntry - startup\n");
    fflush(stdout);

	if (buf == NULL)
        buf = ndb_calloc(NDB_SAVESIZE_FILEENTRY + NDB_MAXLEN_FILENAME + 65535, 1, "ndb_io_readFileEntry: read buffer");
	if (buf == NULL)
	{
        fprintf(stderr, "ndb_io_readFileEntry: couldn't allocate memory for read buffer\n");
		fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
	}

    /* read fixed length data of file entry to archive file */
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_io_readFileEntry: trying to read %d bytes (file entry data) at file position 0x%lX\n",
                        NDB_SAVESIZE_FILEENTRY, ftell(fArchive));
    fflush(stdout);
    ulSize = ndb_io_readdata(buf, NDB_SAVESIZE_FILEENTRY, fArchive);
    if (ulSize != NDB_SAVESIZE_FILEENTRY)
    {
//        ndb_free(buf);
        return NDB_MSG_READERROR;
    }

    count = 0;

	// set memory only fields
    pFileEntry->sameCRC        = NULL;
    pFileEntry->nextFile       = NULL;
	// read saved data
    pFileEntry->crc16Hea       = ndb_readbuf_ushort(buf, &count);
    count_aftercrc = count;
    pFileEntry->fileNo         = ndb_readbuf_ulong(buf, &count);
    pFileEntry->chapNo         = ndb_readbuf_ushort(buf, &count);
    pFileEntry->lenOri8        = ndb_readbuf_ulong8(buf, &count);
    pFileEntry->lenZip8        = ndb_readbuf_ulong8(buf, &count);
//    pFileEntry->crc32          = ndb_readbuf_ulong(buf, &count);
    memcpy(pFileEntry->md5, ndb_readbuf_char_n(buf, &count, MD5_DIGESTSIZE), MD5_DIGESTSIZE);
    pFileEntry->ctime          = ndb_readbuf_ulong(buf, &count);
    pFileEntry->mtime          = ndb_readbuf_ulong(buf, &count);
    pFileEntry->atime          = ndb_readbuf_ulong(buf, &count);
    pFileEntry->attributes     = ndb_readbuf_ulong(buf, &count);
    pFileEntry->blockCount     = ndb_readbuf_ulong(buf, &count);
    pFileEntry->FSType         = ndb_readbuf_char(buf, &count);
    pFileEntry->flags          = ndb_readbuf_char(buf, &count);
	pFileEntry->nr_ofExtraHea  = ndb_readbuf_ushort(buf, &count);
    pFileEntry->lenExtHea      = ndb_readbuf_ushort(buf, &count);
    pFileEntry->lenfilenameUTF8 = ndb_readbuf_ushort(buf, &count);
    // additionally read filename and extra data if existing
//    count = 0;
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_io_readFileEntry: trying to read %d bytes (filename) at file position 0x%lX\n",
                        pFileEntry->lenfilenameUTF8, ftell(fArchive));
    fflush(stdout);
    ulSize = ndb_io_readdata(&buf[count], pFileEntry->lenfilenameUTF8, fArchive);
    if (ulSize != pFileEntry->lenfilenameUTF8)
    {
//        ndb_free(buf);
        return NDB_MSG_READERROR;
    }

    // get memory for the filename
    pFileEntry->filenameUTF8 = ndb_calloc(pFileEntry->lenfilenameUTF8 + 1, 1, "ndb_io_readFileEntry: filenameUTF8");
	if (pFileEntry->filenameUTF8 == NULL)
	{
        fprintf(stderr, "ndb_io_readFileEntry: couldn't allocate memory for filename\n");
		fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
	}

    // now copy filename to file entry structure
    strcpy (pFileEntry->filenameUTF8, ndb_readbuf_char_n(buf, &count, pFileEntry->lenfilenameUTF8));
    pFileEntry->filenameUTF8[pFileEntry->lenfilenameUTF8] = '\0';

    // read extra data or set them to zero
	if (pFileEntry->nr_ofExtraHea > 0)
	{
	    /* read fixed length data of file entry to archive file */
	    if (ndb_getDebugLevel() >= 7)
	        fprintf(stdout, "ndb_io_readFileEntry: trying to read %d bytes (extra data) at file position 0x%lX for %d extra header\n",
	                        pFileEntry->lenExtHea, ftell(fArchive), pFileEntry->nr_ofExtraHea);
	    fflush(stdout);
	    ulSize = ndb_io_readdata(&buf[count], pFileEntry->lenExtHea, fArchive);
	    if (ulSize != pFileEntry->lenExtHea)
	    {
//            ndb_free(buf);
	        return NDB_MSG_READERROR;
	    }
//	    count = 0;

	    for (i = 0; i < pFileEntry->nr_ofExtraHea; i++)
	    {
            if (ndb_getDebugLevel() >= 8)
                fprintf(stdout, "ndb_io_readFileEntry - calloc() for header %d of %d\n", i, pFileEntry->nr_ofExtraHea);
            fflush(stdout);
			pCurrExtraHea = ndb_calloc(sizeof(struct ndb_s_fileextradataheader), 1, "ndb_io_readFileEntry: pExtraHea");
	        if (pCurrExtraHea == NULL)
	        {
	            fprintf(stderr, "ndb_io_readFileEntry: couldn't allocate extra data header\n");
	            fflush(stderr);
	            exit(NDB_MSG_NOMEMORY);
	        }
		    pCurrExtraHea->typExtHea          = ndb_readbuf_ushort(buf, &count);
		    pCurrExtraHea->lenExtHea          = ndb_readbuf_ushort(buf, &count);
		    pCurrExtraHea->lenOriExtra        = ndb_readbuf_ulong(buf, &count);
		    pCurrExtraHea->lenZipExtra        = ndb_readbuf_ulong(buf, &count);
		    pCurrExtraHea->crc32Extra         = ndb_readbuf_ulong(buf, &count);
		    pCurrExtraHea->blockCountExtra    = ndb_readbuf_ushort(buf, &count);
			if (pCurrExtraHea->lenExtHea > NDB_SAVESIZE_FILEEXTRAHEADER)
			{
		        pCurrExtraHea->pData = ndb_calloc(pCurrExtraHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER,
		                                          1, "ndb_io_readFileEntry: pExtraHea->pData");
		        if (pCurrExtraHea == NULL)
		        {
		            fprintf(stderr, "ndb_io_readFileEntry: couldn't allocate direct extra data\n");
		            fflush(stderr);
		            exit(NDB_MSG_NOMEMORY);
		        }
		        memcpy(pCurrExtraHea->pData, &buf[count], pCurrExtraHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER);
			    count += pCurrExtraHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER;
			}
            // add extra header to file
            retval = ndb_fe_addExtraHeader(pFileEntry, pCurrExtraHea);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stderr, "ndb_io_readFileEntry: adding extra header to file '%s' failed\n",
                                pFileEntry->filenameUTF8);
                fflush(stderr);
            }
	    } // for (i = 0; i < pFileEntry->nr_ofExtraHea; i++)
	}

    // calculate crc16 for buffer with header struct
    crc16 = ndb_crc16(0, &buf[count_aftercrc], count - count_aftercrc);
    // compare with saved crc16
    if (pFileEntry->crc16Hea != crc16)
    {
        fprintf(stderr, "Warning: crc 0x%04X of header of file %lu doesn't result in the saved crc 0x%04X\n",
                        crc16, pFileEntry->fileNo, pFileEntry->crc16Hea);
        fflush(stderr);
//        retval = NDB_MSG_CRCERROR;
    }

    // free resources
//    ndb_free(buf);

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_io_readFileEntry - finished\n");
    fflush(stdout);

    return retval;
}

/* ======================================================================== */
/* functions for writing and reading the data file header                   */
/* ======================================================================== */


/*
    *************************************************************************************
	ndb_io_writeDataFile()

	save the current archive file pointer to the data file header structure
	(to allow later writing of corrected structure data easily);
	writes the data file header structure to the archive file handle;
    *************************************************************************************
*/

int ndb_io_writeDataFile(FILE *fArchive, PDATAFILEHEADER pDataFile, int bIsMain)
{
    int count;
    static char *buf = NULL;
    int retval = NDB_MSG_OK;
    int count_crcbefore = 0;
    int count_crcafter  = 0;


    if (ndb_getDebugLevel() >= 6)
    	fprintf(stdout, "ndb_io_writeDataFile - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 7)
    {
    	char sdummy[40];
    	snprintf(sdummy, 40 - 1, "ndb_io_writeDataFile(%s)", (bIsMain == 1) ? "NDB Main" : "Data File");
		ndb_df_printDFHeadder(pDataFile, sdummy);
	}

	if (buf == NULL)
        buf = ndb_calloc(NDB_SAVESIZE_DATAFILEHEADER + NDB_MAXLEN_FILENAME + 1, 1, "ndb_io_writeDataFile: write buffer");
	if (buf == NULL)
	{
        fprintf(stderr, "ndb_io_writeDataFile: couldn't allocate memory for write buffer\n");
		fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
	}

    count = 0;
    pDataFile->crc16Hea = 0;

    count_crcbefore = count;
    count = ndb_write2buf_ushort (buf, &count, pDataFile->crc16Hea);
    count_crcafter  = count;
    count = ndb_write2buf_ushort (buf, &count, pDataFile->datFil_No);
    count = ndb_write2buf_ushort (buf, &count, pDataFile->firstChap);
    count = ndb_write2buf_ushort  (buf, &count, pDataFile->lastChap);
    count = ndb_write2buf_ulong  (buf, &count, pDataFile->curDataSize);
    count = ndb_write2buf_ulong  (buf, &count, pDataFile->maxDataSize);
    count = ndb_write2buf_ulong  (buf, &count, pDataFile->identCRC);
    count = ndb_write2buf_ushort  (buf, &count, pDataFile->flags1);
    count = ndb_write2buf_ushort  (buf, &count, pDataFile->flags2);
    // write filename
	if (bIsMain == 1)
	{
		// in NDB main file write name of data file
		count = ndb_write2buf_ushort (buf, &count, pDataFile->lenfilename);
		count = ndb_write2buf_char_n (buf, &count, pDataFile->filename, pDataFile->lenfilename);
		pDataFile->ownFilePositionMain = ftell(fArchive);
	}
	else
	{
		// in data file write name of NDB main file
		count = ndb_write2buf_ushort (buf, &count, pDataFile->lenmainfilename);
		count = ndb_write2buf_char_n (buf, &count, pDataFile->mainfilename, pDataFile->lenmainfilename);
		pDataFile->ownFilePositionDF = ftell(fArchive);
	}

    if (ndb_getDebugLevel() >= 9)
        ndb_block_dumpblock(&buf[count_crcafter], count - count_crcafter);

    // calculate crc16 for buffer with header struct
    pDataFile->crc16Hea = ndb_crc16(0, &buf[count_crcafter], count - count_crcafter);
    // write crc16 into buffer
    ndb_write2buf_ushort(buf, &count_crcbefore, pDataFile->crc16Hea);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_writeDataFile: calculated CRC is %04X\n", pDataFile->crc16Hea);
        fflush(stdout);
    }

    // now save it to disk
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_io_writeDataFile: now save buffer (%d bytes) to disk at file pos 0x%lX\n",
                        count, ftell(fArchive));
    fflush(stdout);

    if (count == NDB_SAVESIZE_DATAFILEHEADER + (bIsMain ? pDataFile->lenfilename : pDataFile->lenmainfilename))
        retval = ndb_io_writedata(buf, count, fArchive);
    else
    {
        fprintf(stderr, "ndb_io_writeDataFile: internal error: expected length of buffer %d, found %d\n",
                        NDB_SAVESIZE_DATAFILEHEADER + (bIsMain ? pDataFile->lenfilename : pDataFile->lenmainfilename), count);
        fprintf(stderr, "                      for file '%s'\n", (bIsMain ? pDataFile->filename : pDataFile->mainfilename));
        fflush (stderr);
//        ndb_free(buf);
        return NDB_MSG_NOTOK;
    }

    if (retval != count)
    {
        fprintf(stdout, "tried to write %d bytes but could only %d bytes\n",
                        count, retval);
        fflush (stdout);
//        ndb_free(buf);
        return NDB_MSG_WRITEERROR;
    }

    // free resources
//    ndb_free(buf);

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_io_writeDataFile - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
	ndb_io_readDataFile()

	reads the data file header structure from the archive file handle;
    *************************************************************************************
*/

int ndb_io_readDataFile(FILE *fArchive, PDATAFILEHEADER pDataFile, int bIsMain)
{
    int count;
    static char *buf = NULL;
    ULONG ulSize = 0L;
    int count_aftercrc = 0;
    USHORT crc16 = 0;
    int retval = NDB_MSG_OK;


    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_io_readDataFile - startup\n");

	if (buf == NULL)
        buf = ndb_calloc(NDB_SAVESIZE_DATAFILEHEADER + NDB_MAXLEN_FILENAME + 65535, 1, "ndb_io_readDataFile: read buffer");
	if (buf == NULL)
	{
        fprintf(stderr, "ndb_io_readDataFile: couldn't allocate memory for read buffer\n");
		fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
	}

    /* read fixed length data of file entry to archive file */
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_io_readDataFile: trying to read %d bytes (data file header) at file position 0x%lX\n",
                        NDB_SAVESIZE_DATAFILEHEADER, ftell(fArchive));
    fflush(stdout);
    ulSize = ndb_io_readdata(buf, NDB_SAVESIZE_DATAFILEHEADER, fArchive);
    if (ulSize != NDB_SAVESIZE_DATAFILEHEADER)
    {
//        ndb_free(buf);
        return NDB_MSG_READERROR;
    }

    count = 0;

    pDataFile->crc16Hea         = ndb_readbuf_ushort(buf, &count);
    count_aftercrc = count;
    pDataFile->datFil_No        = ndb_readbuf_ushort(buf, &count);
    pDataFile->firstChap        = ndb_readbuf_ushort(buf, &count);
    pDataFile->lastChap         = ndb_readbuf_ushort(buf, &count);
    pDataFile->curDataSize      = ndb_readbuf_ulong(buf, &count);
    pDataFile->maxDataSize      = ndb_readbuf_ulong(buf, &count);
    pDataFile->identCRC         = ndb_readbuf_ulong(buf, &count);
    pDataFile->flags1           = ndb_readbuf_ushort(buf, &count);
    pDataFile->flags2           = ndb_readbuf_ushort(buf, &count);

	if (ndb_getDebugLevel() >= 7)
	{
		char sdummy[40];
		snprintf(sdummy, 40 - 1, "ndb_io_readDataFile(%s): read curr size %lu",
                 (bIsMain == 1) ? "NDB Main" : "Data File", pDataFile->curDataSize);
	}

	// if NDB main file then read data file name
	// else in data file then read NDB main file name
	if (bIsMain == 1)
	    pDataFile->lenfilename = ndb_readbuf_ushort(buf, &count);
	else
	    pDataFile->lenmainfilename = ndb_readbuf_ushort(buf, &count);

    // read filename of data/main file
    // we read everything continously into the buffer,
    // therefore no reset to buffer start via count = 0!
    // pw; 17-May-20040: count = 0;
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_io_readDataFile: trying to read %d bytes (filename) at file position 0x%lX\n",
                        (bIsMain ? pDataFile->lenfilename : pDataFile->lenmainfilename), ftell(fArchive));
    fflush(stdout);
    ulSize = ndb_io_readdata(&buf[count], (bIsMain ? pDataFile->lenfilename : pDataFile->lenmainfilename), fArchive);
    if (ulSize != (bIsMain ? pDataFile->lenfilename : pDataFile->lenmainfilename))
    {
//        ndb_free(buf);
        return NDB_MSG_READERROR;
    }

    // now copy filename to data file header structure
	if (bIsMain == 1)
	{
		// in NDB main archive file read data file name
	    strcpy (pDataFile->filename, ndb_readbuf_char_n(buf, &count, pDataFile->lenfilename));
	    pDataFile->filename[pDataFile->lenfilename] = '\0';
	    // let NDB main filename fill outside
	    pDataFile->lenmainfilename = 0;
	    pDataFile->mainfilename[0] = '\0';
		pDataFile->ownFilePositionMain = ftell(fArchive);
	}
	else
	{
		// in data file read NDB main file name
	    strcpy (pDataFile->mainfilename, ndb_readbuf_char_n(buf, &count, pDataFile->lenmainfilename));
	    pDataFile->mainfilename[pDataFile->lenmainfilename] = '\0';
	    // let 'normal' filename fill outside
	    pDataFile->lenfilename = 0;
	    pDataFile->filename[0] = '\0';
		pDataFile->ownFilePositionDF = ftell(fArchive);
	}


    if (ndb_getDebugLevel() >= 9)
        ndb_block_dumpblock(&buf[count_aftercrc], count - count_aftercrc);

    // calculate crc16 for buffer with header struct
    crc16 = ndb_crc16(0, &buf[count_aftercrc], count - count_aftercrc);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_readDataFile: calculated CRC is %04X\n", crc16);
        fprintf(stdout, "                          saved CRC is %04X\n", pDataFile->crc16Hea);
        fflush(stdout);
    }
    // compare with saved crc16
    if (pDataFile->crc16Hea != crc16)
    {
        fprintf(stderr, "Warning: crc 0x%04X of header of data file %u doesn't result in the saved crc 0x%04X\n",
                        crc16, pDataFile->datFil_No, pDataFile->crc16Hea);
        fflush(stderr);
//        retval = NDB_MSG_CRCERROR;
    }

 	// set fields not saved in NDB archive file
	pDataFile->fileHandle      = NULL;
	pDataFile->nxtFileData     = NULL;
	pDataFile->status          = 0;

    if (ndb_getDebugLevel() >= 7)
    {
    	char sdummy[40];
    	snprintf(sdummy, 40 - 1, "ndb_io_readDataFile(%s)", (bIsMain == 1) ? "NDB Main" : "Data File");
		ndb_df_printDFHeadder(pDataFile, sdummy);
	}

   // free resources
//    ndb_free(buf);

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_io_readDataFile - finished\n");
    fflush(stdout);

    return retval;
}
