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
#include <time.h>

#include "ndb.h"
#include "ndb_prototypes.h"


extern int currBlockSize;
extern int currCompression;


/*
    *************************************************************************************
    ndb_chap_init()

	inits a new, calloc'ed chapter structure and sets some defaults and
	the given parameter

    struct ndb_s_c_chapter *pNewChapter:   (INOUT)    pointer to chapter structure
    int chapNo:                            (IN)       number for chapter
    const char *name:                      (IN)       name for chapter
    const char *comment:                   (IN)       comment for chapter
    *************************************************************************************
*/
void ndb_chap_init(struct ndb_s_c_chapter *pNewChapter, int chapNo, const char *name, const char *comment,
                   const char *ndbver, const char *codepage)
{
    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_init - startup\n");
    fflush(stdout);

    pNewChapter->magic               = NDB_MAGIC_CHAPTER;
    pNewChapter->verChunk            = NDB_CHUNKCHAPTER_VERCHUNK; /* SAVE: 02B: chunk format version */
    pNewChapter->lenChunk            = NDB_CHUNKCHAPTER_LENCHUNK; /* SAVE: 02B: length of rest of chunk */
    pNewChapter->crc16Hea            = 0;            /* SAVE: 02B: crc16 of header structure */
    strncpy(pNewChapter->ndbver, ndbver, NDB_MAXLEN_NDBVER-1);             /* SAVE: 25B: NDB version (like archive chunk) */
    pNewChapter->ndbver[NDB_MAXLEN_NDBVER-1] = '\0';
    pNewChapter->ndb_os              = NDB_OS_CODE;  /* SAVE: 02B: NDB constant for OS */
    pNewChapter->flags1              = 0;            /* SAVE: 02B: Flags für zukünftige Erweiterungen */
    pNewChapter->flags2              = 0;            /* SAVE: 02B: Flags für zukünftige Erweiterungen */
    pNewChapter->chapterSize         = 0;             /* SAVE: 04B: size of chapter without header */
    pNewChapter->chapterNo           = chapNo;        /* SAVE: 02B: Chapternummer */
    strncpy(pNewChapter->chapterName, name, NDB_MAXLEN_CHAPNAME);      /* SAVE: 32C: Chaptername */
    pNewChapter->chapterName[NDB_MAXLEN_CHAPNAME-1] = '\0';
    pNewChapter->chapterTime         = time(NULL);    /* SAVE: 04B: Erzeugungsdatum Chapter */
    strncpy(pNewChapter->comment, comment, NDB_MAXLEN_COMMENT-1);       /* SAVE: 80C: Chapterkommentar */
    pNewChapter->comment[NDB_MAXLEN_COMMENT-1] = '\0';
    strncpy(pNewChapter->codepage, codepage, NDB_MAXLEN_CODEPAGE-1);    /* SAVE: 40B: Codepage of chapter */
    pNewChapter->codepage[NDB_MAXLEN_CODEPAGE-1] = '\0';
    pNewChapter->pNextChapter        = NULL;          /* TRANS: Verkettung mit nächstem Chapter */
    pNewChapter->blockDatLen         = 0;             /* SAVE: 04B: length of block data in NDB main file */
    pNewChapter->blockDatLenReal     = 0;             /* SAVE: 04B: real length of zipped blocks */
    pNewChapter->blockDatSta         = 0;             /* SAVE: 04B: Start der gezippten Blöcke */
    pNewChapter->blockDatNo          = 0;             /* SAVE: 04B: Gesamtanzahl aller Blöcke */
    pNewChapter->blockHeaLen         = 0;             /* SAVE: 04B: Länge der Block-Header */
    pNewChapter->blockHeaSta         = 0;             /* SAVE: 04B: Start der Block-Header */
    pNewChapter->blockHeaNo          = 0;             /* SAVE: 04B: Gesamtanzahl aller Block-Header */
    pNewChapter->pFirstBlockHea      = NULL;          /* TRANS: Zeiger auf ersten gezippten Block */
    pNewChapter->filesListLen        = 0;             /* SAVE: 04B: Länge der Fileliste */
    pNewChapter->filesListSta        = 0;             /* SAVE: 04B: Start der Fileliste */
    pNewChapter->filesNo             = 0;             /* SAVE: 04B: Anzahl der Files */
    pNewChapter->filesNoIdent        = 0;             /* SAVE: 04B: Anzahl Files gleich zu letztem Chapter */
    pNewChapter->allFileLenOri       = 0;             /* SAVE: 04B: Gesamtlänge aller Files */
    pNewChapter->pLastFile           = NULL;          /* TRANS: 04B: Zeiger auf erste Datei in Liste */
    pNewChapter->pFirstFile          = NULL;          /* TRANS: 04B: Zeiger auf letzte Datei in Liste */
    // 24Jun03 pw NEW chapter contains a hashtable for quick check for filenames
    pNewChapter->ppHashtableFiles    = NULL;          /* TRANS: 04B: Zeiger auf Hashtable mit Filenamen */
    pNewChapter->ownFilePosition     = 0;             /* TRANS: own position in archive file */
    // 02Dec03 pw NEW chapter contains fields for external data files
    pNewChapter->noMaxDatFil         = 0;             /* SAVE: 02B: highest no of datafile needed to extract */
    pNewChapter->noNewDatFil         = 0;             /* SAVE: 02B: how many new datafiles? */
    pNewChapter->dataFilesHeaLen     = 0;             /* SAVE: 04B: length of data files header chunk/data */
    pNewChapter->staDatFil           = 0;             /* SAVE: 04B: (chunk) start of data files header */
    pNewChapter->pDatFil             = NULL;          /* TRANS: 04B: pointer to first data file header */


    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_init - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_chap_initFileHash()

	inits the hashtable for the filenames of this chapter

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    *************************************************************************************
*/
void ndb_chap_initFileHash(struct ndb_s_c_chapter *pChapter)
{
#if defined(USE_HASHTABLE_CRC_FILENAMES)

	int i;

    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_initFileHash - startup\n");
    fflush(stdout);

    pChapter->ppHashtableFiles    = ndb_calloc(NDB_SIZE_FILESTABLE, sizeof(struct ndb_s_fileentry *), "ndb_chap_initFileHash");
	if (pChapter->ppHashtableFiles == NULL)
	{
	        fprintf(stderr, "Error: couldn't allocate memory for hashtable of filenames\n");
	        exit (NDB_MSG_NOMEMORY);
	}

    // 24Jun03 pw NEW empty hashtable for files
    for (i = 0; i < NDB_SIZE_FILESTABLE; i++)
    {
    	pChapter->ppHashtableFiles[i] = NULL;
    }

    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_initFileHash - finished\n");
    fflush(stdout);
#else
    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_initFileHash: hashtable disabled in this build\n");
    fflush(stdout);
#endif
}



/*
    *************************************************************************************
    ndb_chap_print()

	prints out the chapter meta data

    struct ndb_s_c_chapter *pChapter:      (IN)    pointer to chapter structure
    *************************************************************************************
*/
void ndb_chap_print(struct ndb_s_c_chapter *pChapter)
{
    fprintf(stdout, "Chapter Info =============================================\n");
    fprintf(stdout, "mem: own location           : 0x%lX\n", (ULONG) pChapter);
    fprintf(stdout, "own file position           : 0x%lX\n", pChapter->ownFilePosition);
    fprintf(stdout, "magic                       : 0x%lX\n", pChapter->magic);
    fprintf(stdout, "chapter no                  : 0x%X\n", pChapter->chapterNo);
    fprintf(stdout, "chapter name                : '%s'\n", pChapter->chapterName);
    fprintf(stdout, "Chapter Codepage            : %s\n",
                    (pChapter->codepage[0] == '\0') ? "[no codepage]" : pChapter->codepage);
    fprintf(stdout, "Chapter Comment             : \n%s\n",
                    (pChapter->comment[0] == '\0') ? "[no comment]" : pChapter->comment);
    fprintf(stdout, "creation time               : 0x%lX\n", pChapter->chapterTime);
    fprintf(stdout, "chapter size                : 0x%llX\n", pChapter->chapterSize);
    fprintf(stdout, "size of block data chunk -- : 0x%llX\n", pChapter->blockDatLen);
    fprintf(stdout, "real size of block data     : 0x%llX\n", pChapter->blockDatLenReal);
    fprintf(stdout, "file start block data       : 0x%lX\n", pChapter->blockDatSta);
    fprintf(stdout, "no of blocks                : 0x%lX\n", pChapter->blockDatNo);
    fprintf(stdout, "size of block header ------ : 0x%lX\n", pChapter->blockHeaLen);
    fprintf(stdout, "file start block header     : 0x%lX\n", pChapter->blockHeaSta);
    fprintf(stdout, "no of block header          : 0x%lX\n", pChapter->blockHeaNo);
    fprintf(stdout, "mem: block header list      : 0x%lX\n", (ULONG) pChapter->pFirstBlockHea);
    fprintf(stdout, "size of file entries ------ : 0x%lX\n", pChapter->filesListLen);
    fprintf(stdout, "file start of file entries  : 0x%lX\n", pChapter->filesListSta);
    fprintf(stdout, "no of files                 : 0x%lX\n", pChapter->filesNo);
    fprintf(stdout, "no of unchanged files       : 0x%lX\n", pChapter->filesNoIdent);
    fprintf(stdout, "original length of all files: 0x%llX\n", pChapter->allFileLenOri);
    fprintf(stdout, "mem: file entry list        : 0x%lX\n", (ULONG) pChapter->pFirstFile);
    fprintf(stdout, "size of data files -------- : 0x%lX\n", pChapter->dataFilesHeaLen);
    fprintf(stdout, "file start of data files    : 0x%lX\n", pChapter->staDatFil);
    fprintf(stdout, "no of data files            : 0x%X\n", pChapter->noNewDatFil);
    fprintf(stdout, "mem: data files list        : 0x%lX\n", (ULONG) pChapter->pDatFil);
    fprintf(stdout, "==========================================================\n");
}

/*
    *************************************************************************************
    ndb_chap_printChapterInfo()

	prints the chapter info (meta data) for ndb_list

    struct ndb_s_c_chapter *pChapter:      (IN)       pointer to chapter structure
    int verbose:                           (IN)       0: short; 1: medium; 2: long
    *************************************************************************************
*/
void ndb_chap_printChapterInfo(struct ndb_s_c_chapter *pChapter, int verbose)
{
    // create time string
    char stime[40];
    char sfiles[40];
    struct tm *tim;
    int compression;
    double allFilesOri;
    double allFilesZip;

    tim = localtime( (const time_t *) & pChapter->chapterTime);

    // there seem to be problems calculating or printing ULONG8 (= long long) values
    // therefore we use doubles instead
    allFilesOri = (double) pChapter->allFileLenOri;
    allFilesZip = (double) pChapter->blockDatLenReal + (double)  pChapter->blockHeaLen;
    if (allFilesOri > 0)
    {
        compression = (int) (100.0 - (100.0 * allFilesZip)/allFilesOri);
    }
    else
    {
        compression = 0;
    }

    if (verbose > 1)
    {
        // show long chapter info
        if (tim != NULL)
            strftime(stime, 40, "%Y-%m-%d %H:%M:%S", tim);
        else
        {
            strcpy(stime, "[DATE ERROR]       ");
            fprintf(stderr, "warning: creation date of chapter %04d corrupt\n", pChapter->chapterNo);
            fflush (stderr);
        }
        snprintf(sfiles, 40 - 1, "%lu/%lu%c", pChapter->filesNo, pChapter->filesNo - pChapter->filesNoIdent, '\0');
        fprintf(stdout, "%4d  %13s  %-18s %10.0f %10.0f   %5d%%\n",
                        pChapter->chapterNo,
                        stime,
                        sfiles,
                        allFilesOri,
                        allFilesZip,
                        compression);
        fprintf(stdout, "      %-20s %-32s %16s\n",
                        pChapter->ndbver,
                        pChapter->chapterName,
                        pChapter->codepage);
        // if long output, print chapter comment if existing
        if (pChapter->comment[0] != '\0')
        {
            fprintf(stdout, "      %-70s\n", pChapter->comment);
        }
    }
    else
    {
        // show short or medium chapter info
        if (tim != NULL)
            strftime(stime, 40, "%Y-%m-%d %H:%M", tim);
        else
        {
            strcpy(stime, "[DATE ERROR]    ");
            fprintf(stderr, "warning: creation date of chapter %04d corrupt\n", pChapter->chapterNo);
            fflush (stderr);
        }
        fprintf(stdout, "%4d  %10s  %5lu %10.0f %10.0f %4d%%  %-20s\n",
                        pChapter->chapterNo,
                        stime,
                        pChapter->filesNo,
                        allFilesOri,
                        allFilesZip,
                        compression,
						pChapter->chapterName);
        // if medium output, print chapter comment if existing
        if ((verbose == 1) && pChapter->comment[0] != '\0')
        {
            fprintf(stdout, "      %-70s\n", pChapter->comment);
        }
    }
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_chap_printChapterInfoRaw()

	prints the chapter info (meta data) for ndb_list in a raw format

    struct ndb_s_c_chapter *pChapter:      (IN)       pointer to chapter structure
    *************************************************************************************
*/

char sline[1024];

void ndb_chap_printChapterInfoRaw(struct ndb_s_c_chapter *pChapter)
{
    // create time string
    char stime[40];
    struct tm *tim;
    int compression;
    double allFilesOri;
    double allFilesZip;
    char s_chapNo[12];
    char s_onPos[12];
    char s_size[20];
    char s_noOfFiles[12];
    char s_lenOri[20];
    char s_lenZip[20];
    char s_ratio[12];
    char s_os[40];

    tim = localtime( (const time_t *) & pChapter->chapterTime);

    allFilesOri = (double) pChapter->allFileLenOri;
    allFilesZip = (double) pChapter->blockDatLenReal + (double)  pChapter->blockHeaLen;

    if (allFilesOri > 0)
    {
        compression = (int) (100.0 - (100.0 * allFilesZip)/allFilesOri);
    }
    else
    {
        compression = 0;
    }

    if (tim != NULL)
        strftime(stime, 40, "%Y%m%d_%H%M%S", tim);
    else
    {
        strcpy(stime, "[DATE ERROR]");
        fprintf(stderr, "warning: creation date of chapter %04d corrupt\n", pChapter->chapterNo);
        fflush (stderr);
    }

    snprintf(s_chapNo, 12 - 1, "%d",  pChapter->chapterNo);
    snprintf(s_onPos, 12 - 1, "%lX",  pChapter->ownFilePosition);
    snprintf(s_size, 20 - 1, "%.0f",  (double) pChapter->chapterSize);
    snprintf(s_noOfFiles, 12 - 1, "%lu",  pChapter->filesNo);
    snprintf(s_lenOri, 20 -1, "%.0f", allFilesOri);
    snprintf(s_lenZip, 20 -1, "%.0f", allFilesZip);
    snprintf(s_ratio, 12 -1, "%d",  compression);

    if (pChapter->ndb_os == NDB_OS_CODE_OS2)
        snprintf(s_os, 40 - 1, "OS/2");
    else if (pChapter->ndb_os == NDB_OS_CODE_WIN32)
        snprintf(s_os, 40 - 1, "Win32");
    else if (pChapter->ndb_os == NDB_OS_CODE_UNIX)
        snprintf(s_os, 40 - 1, "Unix");
    else
        snprintf(s_os, 40 - 1, "unknown");

    snprintf(sline, 1024 - 1, "C:%s", ndb_util_formatraw(s_chapNo));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(pChapter->chapterName));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(stime));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_onPos));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_size));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(pChapter->comment));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_noOfFiles));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_lenOri));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_lenZip));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_ratio));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(pChapter->ndbver));
    snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_os));
    snprintf(sline, 1024 - 1, "%s\n",  sline);

    fprintf(stdout,  sline);
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_chap_printHashInfo()

	prints out statistics about the filename hashtable
    *************************************************************************************
*/
void ndb_chap_printHashInfo(PCHAPTER pChapter)
{
    int i, j, k, l;
    int count[101];
    PFILEENTRY pFile = NULL;

    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_printHashInfo - startup\n");
    fflush(stdout);

	if (pChapter->ppHashtableFiles == NULL)
	{
    	fprintf(stdout, "ndb_chap_printHashInfo: no hashtable defined - finished\n");
	    fflush(stdout);
	    return;
	}

    // empty field which counts the list lengths
    for (i = 0; i < 101; i++)
    {
        count[i] = 0;
    }
    // set maximum length counter
    k = 0;
    // set file counter
    l = 0;

    // process the hash list
    if (ndb_getDebugLevel() >= 9)
        fprintf(stdout, "Start of Hashtable -------------------------------------------------------\n");
    fflush(stdout);
    for (i = 0; i < NDB_SIZE_FILESTABLE; i++)
    {
        pFile = pChapter->ppHashtableFiles[i];
        j = 0;
        // count how long the list for every hash value is
        while (pFile != NULL)
        {
            if (ndb_getDebugLevel() >= 9)
//                fprintf(stdout, "%5d:%3d:[%08lX]: '%s'\n", i, j, pFile->crc32, pFile->filenameUTF8);
                fprintf(stdout, "%5d:%3d: '%s'\n", i, j, pFile->filenameUTF8);
            pFile = pFile->sameCRC;
            j++;
            l++;
        }
        // cut length at 100 and save to count field
        j = (j > 100) ? 100 : j;
        count[j] ++;
        k = (j > k)? j : k;
    }
    if (ndb_getDebugLevel() >= 9)
        fprintf(stdout, "End of Hashtable ---------------------------------------------------------\n");
    fflush(stdout);

    // show result of counting
    fprintf(stdout, "\nFilename Hashtable Infos:\n\n");
    fprintf(stdout, "Table has %d size and contains %d file header\n\n", NDB_SIZE_FILESTABLE, l);
    fprintf(stdout, "Length   Count\n");
    fprintf(stdout, "------   -----\n");
    for (i = 1; i < k + 1; i++)
    {
        fprintf(stdout, "   %3d: %6d\n", i, count[i]);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_printHashInfo - finished\n");
    fflush(stdout);
}


/*
    *************************************************************************************
    ndb_chap_addFileToHash()

	adds a new filename (given in pFile) to the filename hash table
	of chapter pChapter
	the hash gets construced by computing the crc32 of its filenameUTF8

    struct ndb_s_c_chapter *pChapter:      (IN)       pointer to chapter structure
    struct ndb_s_fileentry *pFile:         (IN)       pointer to file entry
    *************************************************************************************
*/
int ndb_chap_addFileToHash(PCHAPTER pChapter, PFILEENTRY pFile)
{
#if defined(USE_HASHTABLE_CRC_FILENAMES)

    ULONG crc = 0;
    ULONG lCurrCRC = 0;
    PFILEENTRY pTempFile;


    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_chap_addFileToHash - startup\n");
    }

    // check preconditions
    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_addFileToHash: cannot add file entry to hashtable of a NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_chap_addFileToHash: cannot add NULL file entry to hashtable of chapter\n");
        return NDB_MSG_NULLERROR;
    }

	// add file to hashtable of filenames
    crc = ndb_crc32(0L, pFile->filenameUTF8, (ULONG) pFile->lenfilenameUTF8);
    /* get lower 16 bits of CRC32, use it as index */
    lCurrCRC = (ULONG) crc & (NDB_SIZE_FILESTABLE - 1);

	pTempFile = pChapter->ppHashtableFiles[lCurrCRC];
    if (pTempFile == NULL)
    {
        // first filename with this CRC
        pChapter->ppHashtableFiles[lCurrCRC] = pFile;
    }
    else
    {
        // other filenames with same CRC before
		if (strcmp(pFile->filenameUTF8, pTempFile->filenameUTF8) == 0)
		{
		    if (ndb_getDebugLevel() >= 5)
		    {
		        fprintf(stdout, "ndb_chap_addFileToHash: file '%s' already in chapter list\n", pFile->filenameUTF8);
		        fflush(stdout);
		    }
			return NDB_MSG_DUPLICATEFILENAME;
		}
        while (pTempFile->sameCRC != NULL)
        {
			pTempFile = pTempFile->sameCRC;
			if (strcmp(pFile->filenameUTF8, pTempFile->filenameUTF8) == 0)
			{
//			    if (ndb_getDebugLevel() >= 5)
			    {
			        fprintf(stdout, "ndb_chap_addFileToHash: file '%s' already in chapter list\n", pFile->filenameUTF8);
fprintf(stdout, "ndb_chap_addFileToHash: %08lX\n", crc);
fprintf(stdout, "ndb_chap_addFileToHash: fileNo %8lu\%8lu\n", pFile->fileNo, pTempFile->fileNo);
fprintf(stdout, "ndb_chap_addFileToHash: chapNo %8u\%8u\n", pFile->chapNo, pTempFile->chapNo);
			        fflush(stdout);
			    }
				return NDB_MSG_DUPLICATEFILENAME;
			}
        }
        pTempFile->sameCRC = pFile;
    }

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_chap_addFileToHash - finished\n");
    }
#else
    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_addFileToHash: hashtable disabled in this build\n");
    fflush(stdout);
#endif
    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_removeFileFromHash()

	removes a given pFile from the filename hash table
	of chapter pChapter
	the hash gets construced by computing the crc32 of its filenameUTF8

    struct ndb_s_c_chapter *pChapter:      (IN)       pointer to chapter structure
    struct ndb_s_fileentry *pFile:         (IN)       pointer to file entry
    *************************************************************************************
*/
int ndb_chap_removeFileFromHash(PCHAPTER pChapter, PFILEENTRY pFile)
{
#if defined(USE_HASHTABLE_CRC_FILENAMES)

    ULONG crc = 0;
    ULONG lCurrCRC = 0;
    PFILEENTRY pTempFile;
    PFILEENTRY pTempOld;


    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_chap_removeFileFromHash - startup\n");
    }

    // check preconditions
    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_removeFileFromHash: cannot remove file entry from hashtable of a NULL chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_chap_removeFileFromHash: cannot remove NULL file entry from hashtable of chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }

	// remove file from hashtable of filenames
    crc = ndb_crc32(0L, pFile->filenameUTF8, (ULONG) pFile->lenfilenameUTF8);
    /* get lower 16 bits of CRC32, use it as index */
    lCurrCRC = (ULONG) crc & (NDB_SIZE_FILESTABLE - 1);

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_chap_removeFileFromHash: looking for file '%s' at crc 0x%04lX\n", pFile->filenameUTF8, lCurrCRC);
        fflush(stdout);
    }

    pTempFile = pChapter->ppHashtableFiles[lCurrCRC];
    if (pTempFile == NULL)
    {
        if (ndb_getDebugLevel() >= 7)
        {
        fprintf(stdout, "ndb_chap_removeFileFromHash: no list for crc 0x%04lX -> file not found\n", lCurrCRC);
            fprintf(stdout, "ndb_chap_removeFileFromHash - finished\n");
            fflush(stdout);
        }
        return NDB_MSG_NOTFOUND;
    }
    else
    {
        pTempOld = pTempFile;
        do
        {
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "ndb_chap_removeFileFromHash: checking hash file '%s'\n", pTempFile->filenameUTF8);
                fflush(stdout);
            }
            // other filenames with same CRC before
            if (strcmp(pFile->filenameUTF8, pTempFile->filenameUTF8) == 0)
            {
                if (ndb_getDebugLevel() >= 7)
                {
                    fprintf(stdout, "ndb_chap_removeFileFromHash: file '%s' found in chapter hashtable\n", pFile->filenameUTF8);
                    fflush(stdout);
                }
                if (pTempFile == pChapter->ppHashtableFiles[lCurrCRC])
                {
                    pChapter->ppHashtableFiles[lCurrCRC] = pTempFile->sameCRC;
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "ndb_chap_removeFileFromHash: file '%s' removed as first entry of crc 0x%04lX\n", pFile->filenameUTF8, lCurrCRC);
                        fprintf(stdout, "ndb_chap_removeFileFromHash - finished\n");
                        fflush(stdout);
                    }
                    return NDB_MSG_OK;
                }
                else
                {
                    // remove file from linklist
                    pTempOld->sameCRC = pTempFile->sameCRC;
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "ndb_chap_removeFileFromHash: file '%s' removed as second or later entry of crc 0x%04lX\n", pFile->filenameUTF8, lCurrCRC);
                        fprintf(stdout, "ndb_chap_removeFileFromHash - finished\n");
                        fflush(stdout);
                    }
                    return NDB_MSG_OK;
                }
            }
            // switch to next file
			pTempOld  = pTempFile;
            pTempFile = pTempFile->sameCRC;
        } while (pTempFile != NULL);
    }

    if (ndb_getDebugLevel() >= 5)
    {
            fprintf(stdout, "ndb_chap_removeFileFromHash: file '%s' not found in crc 0x%04lX hashtable\n", pFile->filenameUTF8, lCurrCRC);
            fprintf(stdout, "ndb_chap_removeFileFromHash - finished\n");
            fflush(stdout);
    }
    return NDB_MSG_NOTFOUND;
#else
    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_removeFileFromHash: hashtable disabled in this build\n");
    fflush(stdout);
    return NDB_MSG_OK;
#endif
}

/*
    *************************************************************************************
    ndb_chap_addFileEntry()

	adds a new file entry (given in pFile) to the file list of chapter pChapter

    struct ndb_s_c_chapter *pChapter:      (IN)       pointer to chapter structure
    struct ndb_s_fileentry *pFile:         (IN)       pointer to file entry
    *************************************************************************************
*/
int ndb_chap_addFileEntry(struct ndb_s_c_chapter *pChapter, struct ndb_s_fileentry *pFile)
{
    struct ndb_s_fileentry *pCurrEntry = NULL;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_addFileEntry - startup\n");

    if (ndb_getDebugLevel() >= 8) ndb_chap_print(pChapter);
    if (ndb_getDebugLevel() >= 8) ndb_fe_print(pFile);

    // check preconditions
    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_addFileEntry: cannot add file entry to NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_chap_addFileEntry: cannot add NULL file entry to chapter\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFile->nextFile != NULL)
    {
        fprintf(stdout, "ndb_chap_addFileEntry: no single file entry\n");
        return NDB_MSG_NULLERROR;
    }

    pCurrEntry = pChapter->pFirstFile;
    if (pCurrEntry == NULL)
    {
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_chap_addFileEntry: adding first file entry\n");
        pChapter->pFirstFile = pFile;
        // 24Jun03 pw CHG add pLastFile
        pChapter->pLastFile  = pFile;
        pChapter->filesNo = 1;
    }
    else
    {
        // 24-Jun-03; pw; CHG: use pLastFile instead of loop over all files
        // 24-Mar-04; pw: FIX: using -l  pChapter->pLastFile is undefined
        if (pChapter->pLastFile != NULL)
        {
            pCurrEntry = pChapter->pLastFile;
        }
        else
        {
            // using 'ndba .. -l ..' this case happens
            while (pCurrEntry->nextFile != NULL)
            {
                pCurrEntry = pCurrEntry->nextFile;
            }
            pChapter->pLastFile = pCurrEntry;
        }
        pCurrEntry->nextFile = pFile;
        // 24Jun03 pw CHG use pLastFile instead of loop over all files
        pChapter->pLastFile  = pFile;
        pChapter->filesNo++;
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_chap_addFileEntry: adding file entry '%s' as entry no %lu\n",
                    pCurrEntry->filenameExt == NULL ? pCurrEntry->filenameExt : pCurrEntry->filenameUTF8,
                    pChapter->filesNo);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_addFileEntry - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_removeLastFileEntry()

	removes the last file from the file list of chapter pChapter

    struct ndb_s_c_chapter *pChapter:      (IN)       pointer to chapter structure
    *************************************************************************************
*/
int ndb_chap_removeLastFileEntry(struct ndb_s_c_chapter *pChapter)
{
    struct ndb_s_fileentry *pCurrEntry = NULL;
    struct ndb_s_fileentry *pPrevEntry = NULL;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_removeLastFileEntry - startup\n");

    if (ndb_getDebugLevel() >= 8) ndb_chap_print(pChapter);

    // check preconditions
    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_removeLastFileEntry: cannot remove file entry from NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }

    pCurrEntry = pChapter->pFirstFile;
    if (pCurrEntry == NULL)
    {
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_chap_removeLastFileEntry: chapter %u is empty, cannot remove last file\n",
                    pChapter->chapterNo);
        fflush(stdout);
    }
    else
    {
        if (pCurrEntry->nextFile == NULL)
        {
            // remove first and only file
            pChapter->pFirstFile = NULL;
            pChapter->pLastFile = NULL;
            pChapter->filesNo = 0;
        }
        else
        {
            pPrevEntry = pCurrEntry;
            while (pCurrEntry->nextFile != NULL)
            {
                pPrevEntry = pCurrEntry;
                pCurrEntry = pCurrEntry->nextFile;
            }
            pChapter->pLastFile = pPrevEntry;
            pPrevEntry->nextFile = NULL;
        }
        pChapter->filesNo--;
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_chap_removeLastFileEntry: chapter no %u now has %lu files\n", pChapter->chapterNo, pChapter->filesNo);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_removeLastFileEntry - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_readAllBlockHeader()

	- positions the file pointer to the correct position in the archive file
	- reads and checks the block chunk
	- reads all blockheaders from the block chunk and adds them to the chapter
	  structure

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    FILE *fArchive:                        (IN)       file handle of archive file
    *************************************************************************************
*/
int ndb_chap_readAllBlockHeader(struct ndb_s_c_chapter *pChapter, FILE *fArchive)
{
    struct ndb_s_c_block blockHeaderChunk;
    struct ndb_s_blockheader *pAllBlocks;
    struct ndb_s_blockheader *pCurrBlock = NULL;
    int retval = 0;
    ULONG i1;
    ULONG currFileNo = 0;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_readAllBlockHeader - startup\n");

    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_readAllBlockHeader: cannot work with a NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }

    if (ndb_getDebugLevel() >= 8) ndb_chap_print(pChapter);

    // read block chunk
    fseek(fArchive, pChapter->blockHeaSta, SEEK_SET);
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "Info: reading block chunk at archive file position 0x%ld\n", ftell(fArchive));
    retval = ndb_io_readchunkblock(fArchive, &blockHeaderChunk);
    if ((retval != NDB_MSG_OK) || (blockHeaderChunk.magic != NDB_MAGIC_BLOCKHEADER))
    {
        fprintf(stdout, "ndb_chap_readAllBlockHeader: error reading block header chunk\n");
        return retval;
    }
    // read all block header for current chapter

    // allocate memory for all block header
    // (add + 1 to avoid problems with empty chapters)
    pAllBlocks = (struct ndb_s_blockheader *) ndb_calloc (pChapter->blockHeaNo + 1, sizeof(struct ndb_s_blockheader), "ndb_chap_readAllBlockHeader");
    if (pAllBlocks == NULL)
    {
        fprintf(stdout, "Error: couldn't allocate memory for block header list\n");
        return (NDB_MSG_NOMEMORY);
    }

    // position at begin of first header
    fseek(fArchive, pChapter->blockHeaSta + NDB_SAVESIZE_BLOCKCHUNK, SEEK_SET);

    // load all block header
    if (ndb_getDebugLevel() >= 3)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "ndb_chap_readAllBlockHeader: trying to read %lu block header\n",
                            pChapter->blockHeaNo);
        fflush(stdout);
    }
    for (i1 = 0; i1 < pChapter->blockHeaNo; i1++)
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_chap_readAllBlockHeader: trying to read block header no %lu of 0..%lu\n",
                            i1, (pChapter->blockHeaNo - 1));
        // read next block header
        retval = ndb_io_readBlockHeader(fArchive, &pAllBlocks[i1]);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stdout, "ndb_chap_readAllBlockHeader: error reading block header no %lu\n", i1);
            return retval;
        }
    }

    // now proccess through all blocks and add them to the chapter and the hast table
    if (ndb_getDebugLevel() >= 3)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "ndb_chap_readAllBlockHeader: now processing %lu block header\n",
                            pChapter->blockHeaNo);
        fflush(stdout);
    }
    for (i1 = 0; i1 < pChapter->blockHeaNo; i1++)
    {
        if (ndb_getDebugLevel() >= 5)
        {
            if  (pAllBlocks[i1].fileNo != currFileNo)
            {
                currFileNo = pAllBlocks[i1].fileNo;
                fprintf(stdout, "ndb_chap_readAllBlockHeader: read first block for file number %lu\n", currFileNo);
                fflush(stdout);
            }
        }
        // add chapter number
        pAllBlocks[i1].chapNo = pChapter->chapterNo;
        // be safe, clear transient pointers
        pAllBlocks[i1].BlockDataOri = NULL;
        pAllBlocks[i1].BlockDataZip = NULL;
        pAllBlocks[i1].sameCRC      = NULL;
        pAllBlocks[i1].nextBlock    = NULL;
        // now add block to block table, if not marked as block
        // pointing to already existing binary data
        if ((pAllBlocks[i1].blockInfo & NDB_BLOCKINFO_DUPLICATEDATA) == 0)
        {
            if (ndb_getDebugLevel() >= 6)
                fprintf(stdout, "ndb_chap_readAllBlockHeader: trying to add block header no %lu to block table\n", i1);
            ndb_block_addBlockToTable(&pAllBlocks[i1]);
        }
        // add block to chapter
        if (pCurrBlock == NULL)
        {
            // first block
            pChapter->pFirstBlockHea = &pAllBlocks[i1];
            pCurrBlock = pChapter->pFirstBlockHea;
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "ndb_chap_readAllBlockHeader: setting first block header to chapter (0x%lX)\n",
                                (ULONG) pChapter->pFirstBlockHea);
        }
        else
        {
            // second and later blocks
            if (ndb_getDebugLevel() >= 6)
                fprintf(stdout, "ndb_chap_readAllBlockHeader: appending block header (%lu/%lu) to (%lu/%lu)\n",
                                pAllBlocks[i1].fileNo, pAllBlocks[i1].blockNo, pCurrBlock->fileNo, pCurrBlock->blockNo);
            pCurrBlock->nextBlock = &pAllBlocks[i1];
            pCurrBlock = pCurrBlock->nextBlock;
        }
        if (ndb_getDebugLevel() >= 8)
            ndb_block_print(&pAllBlocks[i1]);
        // free memory if allocated
        if (pAllBlocks[i1].BlockDataOri != NULL)
            ndb_free(pAllBlocks[i1].BlockDataOri);
        if (pAllBlocks[i1].BlockDataZip != NULL)
            ndb_free(pAllBlocks[i1].BlockDataZip);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_readAllBlockHeader - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/* ----------------------------------------------------------------------- */

/*
    *************************************************************************************
    ndb_chap_writeAllBlockHeader()

	writes all blockheaders from the given chapter to the archive file
	structure

	Precondition: file pointer must be set to the correct file position
	              before

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    FILE *fArchive:                        (IN)       file handle of archive file
    *************************************************************************************
*/
int ndb_chap_writeAllBlockHeader(struct ndb_s_c_chapter *pChapter, FILE *fArchive)
{
    int retval = 0;
    struct ndb_s_fileentry *pCurrEntry = NULL;
    ULONG blockcount = 0;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_writeAllBlockHeader - startup\n");

    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_writeAllBlockHeader: cannot save file entries of a NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }

    pCurrEntry = pChapter->pFirstFile;
    if (pCurrEntry == NULL)
    {
        if (ndb_getDebugLevel() >= 2)
            fprintf(stdout, "warning: no block lists to save\n");
        fflush(stdout);
        return NDB_MSG_OK;
    }
    else
    {
        while (pCurrEntry != NULL)
        {
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "ndb_chap_writeAllBlockHeader: writing block header list of file %s\n",
                                pCurrEntry->filenameExt);
            if (ndb_getDebugLevel() >= 8)
                ndb_fe_print(pCurrEntry);
            retval = ndb_fe_writeAllBlockHeader(pCurrEntry, fArchive, &blockcount);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stdout, "ndb_chap_writeAllBlockHeader: Error saving file entry\n");
                return retval;
            }
            pCurrEntry = pCurrEntry->nextFile;
        }
        if (ndb_getDebugLevel() >= 3)
            fprintf(stdout, "ndb_chap_writeAllBlockHeader: %lu block headers (of %lu file entries) saved\n", blockcount, pChapter->filesNo);
    }

    // check block count with chapter info
    if (blockcount != pChapter->blockHeaNo)
    {
        fprintf(stdout, "ndb_chap_writeAllBlockHeader: Error: block header count is differing: %lu != %lu\n", blockcount, pChapter->blockHeaNo);
        fflush(stdout);
        // correct chapter value
        pChapter->blockHeaNo = blockcount;
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_writeAllBlockHeader - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_writeAllFileEntries()

	writes all file entries from the given chapter to the archive file
	structure

	Precondition: file pointer must be set to the correct file position
	              before

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    FILE *fArchive:                        (IN)       file handle of archive file
    *************************************************************************************
*/
int ndb_chap_writeAllFileEntries(struct ndb_s_c_chapter *pChapter, FILE *fArchive)
{
    int retval = 0;
    ULONG countFiles = 0;
    struct ndb_s_fileentry *pFile = NULL;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_writeAllFileEntries - startup\n");
    fflush(stdout);

    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_writeAllFileEntries: cannot save file entries of a NULL chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }

    pFile = pChapter->pFirstFile;
    if (pFile == NULL)
    {
        fprintf(stdout, "warning: no file entry to save\n");
        fflush(stdout);
        // make sure that the chapter value is correct
        pChapter->filesNo = 0;
        return NDB_MSG_OK;
    }
    else
    {
        while (pFile != NULL)
        {
            countFiles++;
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "ndb_chap_writeAllFileEntries: writing file entry for file '%s'\n",
                            pFile->filenameExt);
            if (ndb_getDebugLevel() >= 8) ndb_fe_print(pFile);
            fflush(stdout);
            retval = ndb_io_writeFileEntry(fArchive, pFile);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stdout, "ndb_chap_writeAllFileEntries: Error saving file entry\n");
                fflush(stdout);
                return retval;
            }
            if (ndb_getDebugLevel() >= 8)
                ndb_fe_printAllExtraHeader(pFile);
            pFile = pFile->nextFile;
        }
        // check file count with chapter info
        if (countFiles != pChapter->filesNo)
        {
            fprintf(stdout, "ndb_chap_writeAllFileEntries: Error: file count is differing: %lu != %lu\n", countFiles, pChapter->filesNo);
            fflush(stdout);
            // correct chapter value
            pChapter->filesNo = countFiles;
        }
        if (ndb_getDebugLevel() >= 3)
            fprintf(stdout, "ndb_chap_writeAllFileEntries: %lu file entries saved\n", pChapter->filesNo);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_writeAllFileEntries - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_readAllFileEntries()

	- positions the file pointer to the correct position in the archive file
	- reads and checks the file entry chunk
	- reads all file entries from the block chunk and adds them to the chapter
	  structure

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    FILE *fArchive:                        (IN)       file handle of archive file
    *************************************************************************************
*/
int ndb_chap_readAllFileEntries(struct ndb_s_c_chapter *pChapter, FILE *fArchive)
{
    int retval = 0;
    struct ndb_l_filelist fileListArchiv;
    struct ndb_s_fileentry *pFile = NULL;
    ULONG l1 = 0;
    struct ndb_s_c_block chunkFileEntry;


    if (ndb_getDebugLevel() >= 5)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "ndb_chap_readAllFileEntries - startup\n");
        fflush(stdout);
    }

    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_readAllFileEntries: cannot read file entries of a NULL chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }

    // init file list
    ndb_list_initFileList(&fileListArchiv, NDB_USE_HASHTABLE);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_chap_readAllFileEntries: trying to read the file entry chunk at file position 0x%lX\n",
                            pChapter->filesListSta);
    fflush(stdout);

    fseek(fArchive, pChapter->filesListSta, SEEK_SET);
    ndb_io_doQuadAlign(fArchive);
    retval = ndb_io_readchunkblock(fArchive, &chunkFileEntry);
    if (chunkFileEntry.magic != NDB_MAGIC_FILEHEADER)
    {
        fprintf(stderr, "bad archive - expected magic %X but got %lX\n",
                NDB_MAGIC_BLOCKS, chunkFileEntry.magic);
        fflush(stderr);
        exit(NDB_MSG_BADMAGIC);
    }
    for (l1 = 0; l1 < pChapter->filesNo; l1++)
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_chap_readAllFileEntries: trying to read file entry no %lu at file position 0x%lX\n",
                        l1, ftell(fArchive));
        fflush(stdout);
        pFile = (struct ndb_s_fileentry *) ndb_calloc(sizeof(struct ndb_s_fileentry), 1, "ndb_chap_readAllFileEntries: file entry");
        if (pFile == NULL)
        {
            fprintf(stderr, "ndb_chap_readAllFileEntries: Error: couldn't allocate memory for file entry");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
        retval = ndb_io_readFileEntry(fArchive, pFile);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "ndb_chap_readAllFileEntries: Error trying to read file entry no %ld\n", l1);
            fflush(stderr);
            exit (retval);
        }
        if (ndb_getDebugLevel() >= 8)
        {
			fprintf(stdout, "ndb_chap_readAllFileEntries: nach ndb_io_readFileEntry()\n");
			ndb_fe_print(pFile);
			ndb_fe_printAllExtraHeader(pFile);
        }
        // create external filename if internal is UTF-8
        if ((pFile->flags & NDB_FILEFLAG_NOUTF8) == NDB_FILEFLAG_NOUTF8)
        {
            // no conversion, copy only
            pFile->filenameExt = ndb_calloc(1, 1 + strlen(pFile->filenameUTF8), "ndb_chap_readAllFileEntries: filenameExt");
            if (pFile->filenameExt == NULL)
            {
                // memory allocation failed
                fprintf(stderr, "ndb_chap_readAllFileEntries: could'nt allocate memory for filename\n");
                fflush(stderr);
                exit(NDB_MSG_NOMEMORY);
            }
            // now do the copy
            strcpy (pFile->filenameExt, pFile->filenameUTF8);
            pFile->lenfilenameExt   = strlen(pFile->filenameExt);
        }
        else
        {
            // convert filename from UTF-8 to current codepage
            ndb_fe_makeFilenameCP(pFile);
        }
        // add to file list
        ndb_list_addToFileList(&fileListArchiv, pFile);
		if (pChapter->ppHashtableFiles != NULL)
		{
            if (ndb_getDebugLevel() >= 8)
                fprintf(stdout, "ndb_chap_readAllFileEntries: adding '%s' to hashtable\n", pFile->filenameUTF8);
            fflush(stdout);
			retval = ndb_chap_addFileToHash(pChapter, pFile);
			if (retval != NDB_MSG_OK)
			{
			    fprintf(stderr, "error: %s", ndb_util_msgcode2print(retval));
			    fflush(stderr);
			    exit (retval);
			}
		}
        if (ndb_getDebugLevel() >= 7)
            ndb_fe_print(pFile);
        fflush(stdout);

        if (ndb_getDebugLevel() >= 9)
            ndb_show_ptr();
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 3)
        fprintf(stdout, "ndb_chap_readAllFileEntries: found %lu file(s) within the chapter no %d\n",
                        pChapter->filesNo, pChapter->chapterNo);
    fflush(stdout);

    // now add the list of file to the chapter structure
    pChapter->pFirstFile = fileListArchiv.pFirstFile;
    // free hashtable of now unused list structure
    ndb_free(fileListArchiv.ppHashtableFiles);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_readAllFileEntries - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_rebuildBlockHeaderList()

	goes through the chapter list of all block headers (loaded by ndb_chap_readAllBlockHeader
	before) and adds the blocks to the file entries of the chapter

	Preconditions: ndb_chap_readAllBlockHeader(...) must be called before
	               ndb_chap_readAllFileEntries(...) must be called before

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    *************************************************************************************
*/
int ndb_chap_rebuildBlockHeaderList(struct ndb_s_c_chapter *pChapter)
{
    struct ndb_s_fileentry   *pFile      = NULL;
    struct ndb_s_blockheader *pBlock     = NULL;
    struct ndb_s_blockheader *pOldBlock  = NULL;
    struct ndb_s_blockheader *pNextBlock = NULL;
    struct ndb_s_fileextradataheader *pCurrExtraHea = NULL;
    ULONG ul = 0;

    if (ndb_getDebugLevel() >= 5)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "ndb_chap_rebuildBlockHeaderList - startup\n");
        fflush(stdout);
    }

    pFile  = pChapter->pFirstFile;
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: Error: couldn't add block header because of no file entries\n");
    }

    pBlock = pChapter->pFirstBlockHea;
    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: chapter says it has %lu blocks\n",
                        pChapter->blockHeaNo);
        fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: counting from block list: ");
        pOldBlock = pChapter->pFirstBlockHea;
        for (ul=0; pOldBlock != NULL; ul++)
        {
            pOldBlock = pOldBlock->nextBlock;
        }
        fprintf(stdout, "found %lu blocks\n", ul);
    }
    fflush(stdout);

    // count processed blocks
    ul = 0;
    // now proceed through block list
    pOldBlock = pBlock;
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: starting with first block header of chapter (0x%lX)\n",
                        (ULONG) pChapter->pFirstBlockHea);
    while (pBlock != NULL)
    {
        pNextBlock = pBlock->nextBlock;
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: trying to use block header (%lu/%lu, info %d, type %d)\n",
                            pBlock->fileNo, pBlock->blockNo, pBlock->blockInfo, pBlock->blockType);
            fprintf(stdout, "                                 for current file no %lu\n\n", pFile->fileNo);
            fflush(stdout);
        }
        if (ndb_getDebugLevel() >= 8)
            ndb_block_print(pBlock);
        fflush(stdout);
        while (pBlock->fileNo > pFile->fileNo)
        {
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: current block file no (%lu) higher than current file no %lu\n",
                                pBlock->fileNo, pFile->fileNo);
            fflush(stdout);
            // new file -> set extra data header back to zero
            pCurrExtraHea = NULL;
            // cut block list for current file entry ...
            pOldBlock->nextBlock = NULL;
            // ... and switch to the next file entry
            if (pFile->nextFile != NULL)
            {
                pFile = pFile->nextFile;
                if (ndb_getDebugLevel() >= 5)
                    fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: switched to next file no %lu\n",
                                    pFile->fileNo);
                fflush(stdout);
            }
            else
            {
                fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: Error: no more file entries after %lu of %lu blocks\n", ul, pChapter->blockHeaNo);
                fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: current block file no (%lu), current file no %lu\n",
                                pBlock->fileNo, pFile->fileNo);

                return (NDB_MSG_NOTOK);
            }
        }
        pOldBlock = pBlock;
        pBlock = pNextBlock;
        // ndb_fe_addBlock() needs a single block therefore cut block->nextBlock
        // block->nextBlock already saved in variable pBlock (see code line above)
        pOldBlock->nextBlock = NULL;
        ul++;
        ndb_fe_addBlock(pFile, pOldBlock);
        // set pointer to start of extra blocks if found
        if (((pOldBlock->blockInfo & NDB_BLOCKINFO_EXTRADATA) == NDB_BLOCKINFO_EXTRADATA) && ((pOldBlock->blockInfo & NDB_BLOCKINFO_FIRST) == NDB_BLOCKINFO_FIRST))
        {
			// get first (or next) extra data header
            if (pCurrExtraHea == NULL)
                pCurrExtraHea = pFile->pExtraHeader;
            else
                pCurrExtraHea = pCurrExtraHea->pNextHea;
            // skip extra data header which say they have no blocks
			while ((pCurrExtraHea->blockCountExtra == 0) && (pCurrExtraHea->pNextHea != NULL))
			{
				if (ndb_getDebugLevel() >= 7)
				{
					fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: skipping extra header with type '%X'\n",
									pCurrExtraHea->typExtHea);
					fflush(stdout);
				}
				pCurrExtraHea = pCurrExtraHea->pNextHea;
			}
            // set block list pointer of extra header to current block
            if ((pCurrExtraHea != NULL) && (pCurrExtraHea->firstBlockHeader == NULL))
                pCurrExtraHea->firstBlockHeader = pOldBlock;
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "ndb_chap_rebuildBlockHeaderList: found extra data block, adding to extra header (type %X)\n", pCurrExtraHea->typExtHea);
            if (ndb_getDebugLevel() >= 8)
                ndb_fe_printExtraHeader(pCurrExtraHea);
            fflush(stdout);
        }
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_rebuildBlockHeaderList - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_checkFileListWithChapter()

	used if you want to add files not by creating a new chapter but by
	appending to an already existing chapter (ndbarchive -l ...)

	gets a list of files to archive (pFileList) and checks which files
	have been archived before. These files are marked with action NDB_ACTION_KEEP
	to avoid doubles in the chapter file entriy list

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    struct ndb_l_filelist *pFileList:      (IN)       pointer to list with file entries
    *************************************************************************************
*/
int ndb_chap_checkFileListWithChapter(struct ndb_s_c_chapter *pChapter, struct ndb_l_filelist *pFileList)
{
    struct ndb_s_fileentry *pCurrArcFile = NULL;
    struct ndb_s_fileentry *pCurrExtFile = NULL;

    if (ndb_getDebugLevel() >= 5)
    {
        ndb_printTimeMillis();
        fprintf(stdout, "ndb_chap_checkFileListWithChapter - startup\n");
        fflush(stdout);
    }

    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_checkFileListWithChapter: Error: cannot work with a null chapter\n");
        return NDB_MSG_NOTOK;
    }
    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_chap_checkFileListWithChapter: Error: cannot work with a null file list\n");
        return NDB_MSG_NOTOK;
    }

    pCurrArcFile = pChapter->pFirstFile;
    while (pCurrArcFile != NULL)
    {
        pCurrExtFile = ndb_list_getFirstFile(pFileList);
        while (pCurrExtFile != NULL)
        {
            if (ndb_osdep_matchFilename(pCurrExtFile->filenameUTF8, pCurrArcFile->filenameUTF8) == 1)
            {
                pCurrExtFile->action = NDB_ACTION_KEEP;
                if (ndb_getDebugLevel() >= 5)
                    fprintf(stdout, "ndb_chap_checkFileListWithChapter - file '%s' already in archive\n",
                                    pCurrExtFile->filenameExt);
                fflush(stdout);
            }
            pCurrExtFile = ndb_list_getNextFile(pFileList);
        }
        pCurrArcFile = pCurrArcFile->nextFile;
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_checkFileListWithChapter - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_chap_readAllDataFileHeader()

	- positions the file pointer to the correct position in the archive file
	- reads and checks the data file chunk
	- reads all data file headers from the data files chunk and adds them to the chapter
	  structure

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    FILE *fArchive:                        (IN)       file handle of archive file
    *************************************************************************************
*/
int ndb_chap_readAllDataFileHeader(struct ndb_s_c_chapter *pChapter, FILE *fArchive)
{
    struct ndb_s_c_block dataFilesHeaderChunk;
    PDATAFILEHEADER pAllDatFiles;
    int retval = 0;
    USHORT i1;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_readAllDataFileHeader - startup\n");

    // check preconditions
    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_readAllDataFileHeader: cannot work with a NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }

    if (ndb_getDebugLevel() >= 8) ndb_chap_print(pChapter);

//	pChapter->pDatFil = NULL;
	if (pChapter->noNewDatFil > 0)
	{
	    // read block chunk
	    fseek(fArchive, pChapter->staDatFil, SEEK_SET);
	    if (ndb_getDebugLevel() >= 7)
	        fprintf(stdout, "Info: reading data files chunk at archive file position 0x%ld\n", ftell(fArchive));
	    retval = ndb_io_readchunkblock(fArchive, &dataFilesHeaderChunk);
	    if ((retval != NDB_MSG_OK) || (dataFilesHeaderChunk.magic != NDB_MAGIC_DATAFILE))
	    {
	        fprintf(stdout, "ndb_chap_readAllDataFileHeader: error reading data files header chunk\n");
	        return retval;
	    }

	    // read all data files header for current chapter
	    // allocate memory for all data files header
	    // (add + 1 to avoid problems with empty chapters)
	    pAllDatFiles = (PDATAFILEHEADER) ndb_calloc (pChapter->noNewDatFil + 1, sizeof(struct ndb_s_datafileheader), "ndb_chap_readAllDataFileHeader");
	    if (pAllDatFiles == NULL)
	    {
	        fprintf(stdout, "Error: couldn't allocate memory for data files header list\n");
	        return (NDB_MSG_NOMEMORY);
	    }

	    // position at begin of first header
	    fseek(fArchive, pChapter->staDatFil + NDB_SAVESIZE_BLOCKCHUNK, SEEK_SET);

	    // load all data file header
	    if (ndb_getDebugLevel() >= 3)
	    {
	        ndb_printTimeMillis();
	        fprintf(stdout, "ndb_chap_readAllDataFileHeader: trying to read %u data file header\n",
	                            pChapter->noNewDatFil);
	        fflush(stdout);
	    }
	    for (i1 = 0; i1 < pChapter->noNewDatFil; i1++)
	    {
	        if (ndb_getDebugLevel() >= 7)
	            fprintf(stdout, "ndb_chap_readAllDataFileHeader: trying to read data file header no %u of 0..%u\n",
	                            i1, (pChapter->noNewDatFil - 1));
	        // read next block header
	        retval = ndb_io_readDataFile(fArchive, &pAllDatFiles[i1], 1);
	        if (retval != NDB_MSG_OK)
	        {
	            fprintf(stdout, "ndb_chap_readAllDataFileHeader: error reading data file header no %u\n", i1);
	            return retval;
	        }
	        // fill in data filename also
			strcpy(pAllDatFiles[i1].mainfilename, ndb_df_getCurrDataFilename(0));
			pAllDatFiles[i1].lenmainfilename = strlen(pAllDatFiles[i1].mainfilename);
			if (ndb_getDebugLevel() >= 7)
				ndb_df_printDFHeadder(&pAllDatFiles[i1], "ndb_chap_readAllDataFileHeader");
	    }

	    // now proccess through all data file header and add them to the chapter and the data file functions
	    if (ndb_getDebugLevel() >= 3)
	    {
	        ndb_printTimeMillis();
	        fprintf(stdout, "ndb_chap_readAllDataFileHeader: now processing %u data file header\n",
	                            pChapter->noNewDatFil);
	        fflush(stdout);
	    }
	    for (i1 = 0; i1 < pChapter->noNewDatFil; i1++)
	    {
	        // now add data files header to data files header table
            ndb_df_addDataFile(&pAllDatFiles[i1]);
	        // add data file header to chapter
			retval = ndb_chap_addDataFileHeader( pChapter, &pAllDatFiles[i1]);
			// don't increment pChapter->noNewDatFil++
			// noNewDatFil is already read from the archive file
			if (retval != NDB_MSG_OK)
			{
		        fprintf(stderr, "ndb_chap_readAllDataFileHeader: internal error in ndb_chap_addDataFileHeader(..)\n");
			    fflush(stderr);
			}
	    }
		// pChapter->noMaxDatFil is already read from the archive file
	}

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_readAllDataFileHeader - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
    ndb_chap_writeAllDatFilesHeader()

	writes all data files header from the given chapter to the archive file
	structure

	Precondition: file pointer must be set to the correct file position
	              before

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    FILE *fArchive:                        (IN)       file handle of archive file
    *************************************************************************************
*/
int ndb_chap_writeAllDatFilesHeader(PCHAPTER pChapter, FILE *fArchive)
{
    int retval = 0;
	int countDF = 0;
    PDATAFILEHEADER pDatFil = NULL;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_writeAllDatFilesHeader - startup\n");
    fflush(stdout);

    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_writeAllDatFilesHeader: cannot save data files header of a NULL chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }

    pDatFil = pChapter->pDatFil;
    while (pDatFil != NULL)
    {
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_chap_writeAllDatFilesHeader: writing data files header no %d\n", countDF);
        if (ndb_getDebugLevel() >= 7)
            ndb_df_printDFHeadder(pDatFil, "ndb_chap_writeAllDatFilesHeader");
        fflush(stdout);
        retval = ndb_io_writeDataFile(fArchive, pDatFil, 1);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stdout, "ndb_chap_writeAllDatFilesHeader: Error saving data files header\n");
            fflush(stdout);
            return retval;
        }
        pDatFil = pDatFil->nxtFileData;
        countDF++;
    }
	if (countDF != pChapter->noNewDatFil)
	{
        fprintf(stderr, "ndb_chap_writeAllDatFilesHeader: Error: expected %d, but found %u data files header\n",
                        pChapter->noNewDatFil, countDF);
	    fflush(stderr);
        // correct chapter value
        pChapter->noNewDatFil = countDF;
	}

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_chap_writeAllDatFilesHeader: %u file entries saved\n", pChapter->noNewDatFil);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_writeAllDatFilesHeader - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
    ndb_chap_addDataFileHeader()

	add anew data files header to the given chapter

    struct ndb_s_c_chapter *pChapter:      (INOUT)    pointer to chapter structure
    PDATAFILEHEADER pNewDataFileHeader:    (IN)       new data file header
    *************************************************************************************
*/
int ndb_chap_addDataFileHeader(PCHAPTER pChapter, PDATAFILEHEADER pNewDataFileHeader)
{
	int i1 = 1;
    PDATAFILEHEADER pDatFil = NULL;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_addDataFileHeader - startup\n");
    fflush(stdout);

    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_chap_addDataFileHeader: cannot save data files header of a NULL chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }

    // add data file header to chapter
    if (pChapter->pDatFil == NULL)
    {
        // first data file header
        pChapter->pDatFil = pNewDataFileHeader;
        pNewDataFileHeader->nxtFileData     = NULL;
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_chap_addDataFileHeader: setting first data file header to chapter (0x%lX)\n",
                            (ULONG) pChapter->pDatFil);
        fflush(stdout);
    }
    else
    {
        // second and later data file header
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_chap_addDataFileHeader: adding data file header %u\n", i1);
        fflush(stdout);
		pDatFil = pChapter->pDatFil;
		while (pDatFil->nxtFileData != NULL)
		{
			pDatFil = pDatFil->nxtFileData;
			i1++;
		}
        pDatFil->nxtFileData = pNewDataFileHeader;
        pNewDataFileHeader->nxtFileData     = NULL;
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_addDataFileHeader - finished\n");
    fflush(stdout);

	return NDB_MSG_OK;
}


/*
    *************************************************************************************
	ndb_chap_findfile()

	checks whether a given filename is already included in the file list of the
	current chapter

	this function should be optimized in the future to use a hash table or a
	tree to avoid the currently implemented sequential search in a list
    *************************************************************************************
*/

int ndb_chap_findfile(struct ndb_s_c_chapter  *pLastChapter, struct ndb_s_fileentry *pFileNew, struct ndb_s_fileentry **ppFileOld)
{
    struct ndb_s_fileentry *pTempFile = NULL;
    USHORT isEqual  = 0;

#ifdef USE_HASHTABLE_CRC_FILENAMES
    ULONG crc       = 0;
    ULONG lCurrCRC  = 0;
#else
	int len;
#endif
#if defined(TEST_PERFORMANCE)
	// debug: time measurement
	static ULONG dbg_count   = 0;
	static ULONG dbg_timall = 0;
	static ULONG dbg_timmax = 0;
	ULONG8 start_time;
	ULONG8 curr_time;
	ULONG dbg_clock_diff    = 0;
    char s[200];
#endif

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_findfile - startup\n");
    fflush(stdout);

    // check preconditiones
    if (pLastChapter == NULL)
    {
        fprintf(stderr, "ndb_chap_findfile: cannot work with a NULL chapter\n");
	    fflush(stderr);
        return NDB_MSG_NULLERROR;
    }
#ifdef USE_HASHTABLE_CRC_FILENAMES
    if (pLastChapter->ppHashtableFiles == NULL)
    {
        fprintf(stdout, "ndb_chap_findfile: no filename hashtable -> assuming als files as different\n");
	    fflush(stdout);
        return NDB_MSG_NOTOK;
    }
#endif
    if ((pFileNew == NULL) || (pFileNew->filenameUTF8 == NULL))
    {
        fprintf(stderr, "ndb_chap_findfile: cannot search for a NULL file or NULL filename\n");
	    fflush(stderr);
        return NDB_MSG_NULLERROR;
    }
    if (ppFileOld == NULL)
    {
        fprintf(stderr, "ndb_chap_findfile: cannot assign search result to a NULL pointer\n");
	    fflush(stderr);
        return NDB_MSG_NULLERROR;
    }

    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_chap_findfile: searching in chapter '%d' for filename '%s'\n",
                         pLastChapter->chapterNo, pFileNew->filenameUTF8);
    fflush(stdout);

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_chap_findfile: start_time = %llu\n", start_time);
#endif

#ifndef USE_HASHTABLE_CRC_FILENAMES

    // walk through all files of the chapter
    len = strlen(pFileNew->filenameUTF8);
    pTempFile = pLastChapter->pFirstFile;
    while (pTempFile != NULL)
    {
		// small optimization: compare filenames only if same length
		if (len == pTempFile->lenfilenameUTF8)
		{
			if (strcmp(pTempFile->filenameUTF8, pFileNew->filenameUTF8) == 0)
	        {
	            isEqual = 1;
	            if (ndb_getDebugLevel() >= 5)
	            {
	                fprintf(stdout, "ndb_chap_findfile: found file with same file name\n");
	                fprintf(stdout, "ndb_chap_findfile: file info for found file\n");
	                ndb_fe_print(pTempFile);
	                fflush(stdout);
	            }
	            *ppFileOld = pTempFile;
	            break;
	        }
	    }
        pTempFile = pTempFile->nextFile;
    }

#else

    crc = ndb_crc32(0L, pFileNew->filenameUTF8, (ULONG) pFileNew->lenfilenameUTF8);
    /* get lower 16 bits of CRC32, use it as index */
    lCurrCRC = (ULONG) crc & (NDB_SIZE_FILESTABLE - 1);
    // check index for violation of bounds
    if (lCurrCRC >= NDB_SIZE_FILESTABLE)
    {
        fprintf(stderr, "ndb_chap_findfile: CRC index %lX out of range 0..%lX\n", lCurrCRC, (ULONG) NDB_SIZE_FILESTABLE);
        fflush(stderr);
        exit(NDB_MSG_NOTOK);
   	}
    pTempFile = pLastChapter->ppHashtableFiles[lCurrCRC];
    isEqual = 1;

	if (pTempFile == NULL)
	{
	    isEqual = 0;
	}
	else
	{
		if (strcmp(pFileNew->filenameUTF8, pTempFile->filenameUTF8) != 0)
		{
		    isEqual = 0;
			pTempFile = pTempFile->sameCRC;
		    while (pTempFile != NULL)
		    {
				if (strcmp(pFileNew->filenameUTF8, pTempFile->filenameUTF8) == 0)
				{
					isEqual = 1;
			        *ppFileOld = pTempFile;
					break;
				}
				pTempFile = pTempFile->sameCRC;
		    }
		}
		else
		{
		    isEqual = 1;
	        *ppFileOld = pTempFile;
	    }
    }

#endif

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
//      printf("ndb_filefound: curr_time  = %llu\n", curr_time);
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_chap_findfile: %s: count %lu, average %lu, max %lu, sum %lu\n",
#ifndef USE_HASHTABLE_CRC_FILENAMES
						 "LinS",
#else
						 "HASH",
#endif
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);
        ndb_util_set_performancetest(TEST_PERFORMANCE_FINDFILE, s);
    }
#endif

    if (ndb_getDebugLevel() >= 5)
    {
        if (isEqual == 1)
            fprintf(stdout, "ndb_chap_findfile: filename '%s' found\n",
                            pFileNew->filenameUTF8);
        else
            fprintf(stdout, "ndb_chap_findfile: filename '%s' not found\n",
                            pFileNew->filenameUTF8);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_chap_findfile - finished\n");
    fflush(stdout);

    if (isEqual == 1)
        return NDB_MSG_OK;
    else
        return NDB_MSG_NOTOK;
}


/*
    *************************************************************************************
	ndb_arc_printArchiveInfoRaw()

	Returns the archive file meta data for ndb_list.c in a "raw" format,
	which is usable for computable processing
    *************************************************************************************
*/

char sline2[1024];

void ndb_arc_printArchiveInfoRaw(struct ndb_s_c_archive *pArc)
{
    // create time string
    char stime_c[40];
    struct tm *tim_c;
	char s_blockSize[12];
	char s_compression[12];
	char s_noOfChapters[12];
	char s_archiveSize[12];
	char s_dataFilesCount[12];
	char s_dataFilesSize[12];
    tim_c = localtime( (const time_t *) &pArc->timestamp);


    if (tim_c != NULL)
        strftime(stime_c, 40, "%Y%m%d_%H%M%S", tim_c);
    else
    {
        strcpy(stime_c, "19700101_000000");
    }

	// make strings from meta data if numbers
	snprintf(s_blockSize, 12 - 1, "%u", pArc->blockSize);
	snprintf(s_compression, 12 - 1, "%u", pArc->compression);
	snprintf(s_noOfChapters, 12 - 1, "%u", pArc->noOfChapters);
	snprintf(s_archiveSize, 12 - 1, "%llu", pArc->archiveSize);
	snprintf(s_dataFilesCount, 12 - 1, "%u", pArc->dataFilesCount);
	snprintf(s_dataFilesSize, 12 - 1, "%lu", pArc->dataFilesSize);

	snprintf(sline2, 1024 - 1, "A:%s", ndb_util_formatraw(s_blockSize));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(s_compression));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(stime_c));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(s_noOfChapters));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(s_archiveSize));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(s_dataFilesCount));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(s_dataFilesSize));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(pArc->ndbver));
	snprintf(sline2, 1024 - 1, "%s:%s", sline2, ndb_util_formatraw(pArc->comment));
	snprintf(sline2, 1024 - 1, "%s\n",  sline2);

	fprintf(stdout,  sline2);
	fflush(stdout);
}


/*
    *************************************************************************************
	ndb_arc_print()

	debug output of archive structure
    *************************************************************************************
*/

void ndb_arc_print(struct ndb_s_c_archive *pArc)
{
	fprintf(stdout, "Global Data Of Archive Chunk ---------------------------\n");
	fprintf(stdout, "magic:              %lu\n", pArc->magic);
	fprintf(stdout, "verChunk:           %u\n", pArc->verChunk);
	fprintf(stdout, "lenChunk:           %u\n", pArc->lenChunk);
	fprintf(stdout, "blockSize:          %u\n", pArc->blockSize);
	fprintf(stdout, "compression:        %u\n", pArc->compression);
	fprintf(stdout, "noOfChapters:       %u\n", pArc->noOfChapters);
	fprintf(stdout, "archiveSize:        %llu\n", pArc->archiveSize);
	fprintf(stdout, "archiveSizeReal:    %llu\n", pArc->archiveSizeReal);
	fprintf(stdout, "firstChapter:       %lu\n", pArc->firstChapter);
	fprintf(stdout, "lastChapter:        %lu\n", pArc->lastChapter);
	fprintf(stdout, "dataFilesCount:     %u\n", pArc->dataFilesCount);
	fprintf(stdout, "dataFilesSize:      %lu\n", pArc->dataFilesSize);
	fprintf(stdout, "timestamp:          %lu\n", pArc->timestamp);
	fprintf(stdout, "ndbver:             '%s'\n", pArc->ndbver);
	fprintf(stdout, "comment:            '%s'\n", pArc->comment);
	fprintf(stdout, "ownFilePosition:    %lu\n", pArc->ownFilePosition);
	fprintf(stdout, "pDatFil:            %lu\n", (ULONG) pArc->pDatFil);
	fprintf(stdout, "--------------------------------------------------------\n");
}
