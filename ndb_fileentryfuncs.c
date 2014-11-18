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
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ndb.h"
#include "ndb_prototypes.h"


/*
    *************************************************************************************
	ndb_fe_newFileEntry()

	initializes an empty fileentry structure; sets the given function parameters
	into the structure
    *************************************************************************************
*/

#if defined(UNIX)
int ndb_fe_newFileEntry(struct ndb_s_fileentry *pFileEntry, const char *name, struct stat64 *pStat, ULONG fileNo)
#else
int ndb_fe_newFileEntry(struct ndb_s_fileentry *pFileEntry, const char *name, struct stat *pStat, ULONG fileNo)
#endif
{
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_newFileEntry: startup\n");
    fflush(stdout);

    pFileEntry->crc16Hea         = 0;
    /* fill in data of stat() */
    pFileEntry->fileNo           = fileNo;
    pFileEntry->chapNo           = 0;
    pFileEntry->lenOri8           = (ULONG8) pStat->st_size;
    pFileEntry->lenZip8           = 0;
//  st_mode has other definitions therefore set mode using NDB_FILEFLAGS in
//  function ndb_osdep_makeFileList()
//  pFileEntry->attributes       = pStat->st_mode;
    pFileEntry->ctime            = pStat->st_ctime;
    pFileEntry->mtime            = pStat->st_mtime;
    pFileEntry->atime            = pStat->st_atime;
//    pFileEntry->crc32            = 0;
    memset(pFileEntry->md5, 0, MD5_DIGESTSIZE);
    pFileEntry->blockCount       = 0;
    pFileEntry->FSType           = NDB_FSTYPE_UNKNOWN;
    pFileEntry->flags            = 0;
    // filename is handled below
    pFileEntry->nr_ofExtraHea    = 0;
    pFileEntry->lenExtHea        = 0;
    pFileEntry->nextFile         = NULL;
    pFileEntry->parentDir        = NULL;
    pFileEntry->children         = 0;
    pFileEntry->firstBlockHeader = NULL;
    pFileEntry->lastBlockHeader  = NULL;
    pFileEntry->action           = '\0';
    pFileEntry->remark           = NULL;
    // add external filename
    pFileEntry->lenfilenameExt   = strlen(name);
    pFileEntry->filenameExt = ndb_calloc(pFileEntry->lenfilenameExt + 1, 1, "ndb_fe_newFileEntry: filenameExt");
    if (pFileEntry->filenameExt == NULL)
    {
        fprintf(stderr, "ndb_fe_newFileEntry: couldn't allocate memory for filename\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }
    strcpy(pFileEntry->filenameExt, name);


    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_newFileEntry: finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
	ndb_fe_setFileEntry()

	sets the given function parameters into the structure; clears some other
    *************************************************************************************
*/

#if defined(UNIX)
int ndb_fe_setFileEntry(struct ndb_s_fileentry *pFileEntry, struct stat64 *pStat, ULONG fileNo)
#else
int ndb_fe_setFileEntry(struct ndb_s_fileentry *pFileEntry, struct stat *pStat, ULONG fileNo)
#endif
{
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_setFileEntry: startup\n");
    fflush(stdout);

    pFileEntry->crc16Hea         = 0;
    /* fill in data of stat() */
    pFileEntry->fileNo           = fileNo;
    pFileEntry->chapNo           = 0;
    pFileEntry->lenOri8           = (ULONG8) pStat->st_size;
    pFileEntry->lenZip8           = 0;
//  attributes are already set
//  pFileEntry->attributes       = pStat->st_mode;
    pFileEntry->ctime            = pStat->st_ctime;
    pFileEntry->mtime            = pStat->st_mtime;
    pFileEntry->atime            = pStat->st_atime;
//  now we use MD5 instead of CRC32
//    pFileEntry->crc32            = 0;
    memset(pFileEntry->md5, 0, MD5_DIGESTSIZE);
    pFileEntry->blockCount       = 0;
    pFileEntry->FSType           = NDB_FSTYPE_UNKNOWN;
    pFileEntry->flags            = 0;
    // filename is handled below
    pFileEntry->nr_ofExtraHea    = 0;
    pFileEntry->lenExtHea        = 0;
    pFileEntry->nextFile         = NULL;
    pFileEntry->parentDir        = NULL;
    pFileEntry->children         = 0;
    pFileEntry->firstBlockHeader = NULL;
    pFileEntry->lastBlockHeader  = NULL;
    // action is already set
//    pFileEntry->action           = '\0';
    pFileEntry->remark           = NULL;
    // external filename is already set


    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_newFileEntry: finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
	ndb_fe_print()

	prints out the file meta data (for debug purposes)
    *************************************************************************************
*/

void ndb_fe_print(struct ndb_s_fileentry *pFileEntry)
{
    fprintf(stdout, "File Entry Info ==========================================\n");
    fprintf(stdout, "mem: own location           : 0x%lX\n", (ULONG) pFileEntry);
    fprintf(stdout, "chapter no                  : 0x%X\n", pFileEntry->chapNo);
    fprintf(stdout, "file no                     : 0x%lX\n", pFileEntry->fileNo);
    fprintf(stdout, "length of filename          : %d\n",   pFileEntry->lenfilenameExt);
    fprintf(stdout, "filename (OS codepage)      : '%s'\n", pFileEntry->filenameExt);
    fprintf(stdout, "filename (UTF8)             : '%s'\n", pFileEntry->filenameUTF8);
    fprintf(stdout, "file creation time          : 0x%lX\n", pFileEntry->ctime);
    fprintf(stdout, "file modification time      : 0x%lX\n", pFileEntry->mtime);
    fprintf(stdout, "file last access time       : 0x%lX\n", pFileEntry->atime);
    fprintf(stdout, "file attributes             : 0x%lX\n", pFileEntry->attributes);
    fprintf(stdout, "original length             : 0x%llX\n", pFileEntry->lenOri8);
    fprintf(stdout, "zipped length               : 0x%llX\n", pFileEntry->lenZip8);
    fprintf(stdout, "md5                         : 0x%s\n",  ndb_util_printMD5(pFileEntry->md5));
    fprintf(stdout, "no of blocks                : %lu\n",   pFileEntry->blockCount);
    fprintf(stdout, "pointer to next file        : 0x%lX\n", (ULONG) pFileEntry->nextFile);
    fprintf(stdout, "mem: block list             : 0x%lX\n", (ULONG) pFileEntry->firstBlockHeader);
    fprintf(stdout, "no of extra data headers    : %u\n",   pFileEntry->nr_ofExtraHea);
    fprintf(stdout, "length of all extra headers : %u\n",   pFileEntry->lenExtHea);
    fprintf(stdout, "mem: extra data header list : 0x%lX\n", (ULONG) pFileEntry->pExtraHeader);
    fprintf(stdout, "==========================================================\n");
}

/*
    *************************************************************************************
	ndb_fe_printFileEntryInfo()

	prints out the file meta data (for ndb_list.c);
	there is a short version (everything in one line)
	and a long version (each data in a seperate line)
    *************************************************************************************
*/

void ndb_fe_printFileEntryInfo(struct ndb_s_fileentry *pFileEntry, int verbose)
{
	// create time string
//    struct tm *tim_c;
//    char stime_c[40];
//    tim_c = localtime( (const time_t *) &pFileEntry->ctime);
//    char stime_a[40];
//    struct tm *tim_a;
//    struct tm *tim_a;
//    tim_a = localtime( (const time_t *) &pFileEntry->atime);
    char stime_m[40];
    struct tm *tim_m;
	// compression ratio
	char sratio[10];
	char sExtraType[40];
    // add a slash to directory names
    char s_name[NDB_MAXLEN_FILENAME];
    ULONG ulExtraOri = 0;
    ULONG ulExtraZip = 0;
    PFILEEXTRADATAHEADER pExtraHea = NULL;
    tim_m = localtime( (const time_t *) &pFileEntry->mtime);

    strcpy (s_name, pFileEntry->filenameExt);
    if ((pFileEntry->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
        strcat (s_name, "/");
    if (pFileEntry->lenOri8 > 0)
		snprintf(sratio, 10 - 1, "%3d%%", (int) (100 - (100.0 * pFileEntry->lenZip8)/pFileEntry->lenOri8));
	else
		strcpy(sratio, "n.a.");

    // show file info: verbose = 0: short, 1:medium, 2:long
	if (verbose > 1)
	{
        if (tim_m != NULL)
    	    strftime(stime_m, 40, "%y-%m-%d %H:%M:%S", tim_m);
        else
        {
            strcpy(stime_m, "[DATE ERROR]       ");
            fprintf(stderr, "warning: creation date of file '%s' corrupt\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
            fflush (stderr);
        }

	    fprintf(stdout, "%17s  ..%02X%04X %10.0f %10.0f  %-4s  %s\n",
	                    stime_m,
	                    *((unsigned char *) &pFileEntry->md5[13]),
	                    *((USHORT *) &pFileEntry->md5[14]),
	                    (double) pFileEntry->lenOri8,
	                    (double) pFileEntry->lenZip8,
	                    sratio,
	                    EMXWIN32CONVERT(s_name));
	}
	else
	{
        if (tim_m != NULL)
    	    strftime(stime_m, 40, "%y-%m-%d %H:%M", tim_m);
        else
        {
            strcpy(stime_m, "[DATE ERROR]    ");
            fprintf(stderr, "warning: creation date of file '%s' corrupt\n", EMXWIN32CONVERT(pFileEntry->filenameExt));
            fflush (stderr);
        }
	    fprintf(stdout, "%14s  ..%02X%04X %10.0f %10.0f  %-4s  %s\n",
	                    stime_m,
	                    *((unsigned char *) &pFileEntry->md5[13]),
	                    *((USHORT *) &pFileEntry->md5[14]),
	                    (double) pFileEntry->lenOri8,
	                    (double) pFileEntry->lenZip8,
	                    sratio,
	                    EMXWIN32CONVERT(s_name));
	}
    // print extra data if existing
    if ((verbose > 1) && (pFileEntry->nr_ofExtraHea > 0))
    {
		pExtraHea = pFileEntry->pExtraHeader;
		while (pExtraHea != NULL)
		{
	    	ulExtraOri += pExtraHea->lenOriExtra;
	    	ulExtraZip += pExtraHea->lenZipExtra;
			if ((verbose == 2) || (pFileEntry->nr_ofExtraHea == 1))
			{
				// create output
				strcpy (sExtraType, "[unknown extra data type]");
				if (pExtraHea->typExtHea == NDB_EF_OS2EA)
					strcpy (sExtraType, "[OS/2 EAs]");
				else if (pExtraHea->typExtHea == NDB_EF_NTSD)
					strcpy (sExtraType, "[NTFS Security]");
				else if (pExtraHea->typExtHea == NDB_EF_ACL)
					strcpy (sExtraType, "[OS/2 ACLs]");
				else if (pExtraHea->typExtHea == NDB_EF_UNIX)
					strcpy (sExtraType, "[Unix]");
				else if (pExtraHea->typExtHea == NDB_EF_WIN83)
					strcpy (sExtraType, "[Win 8.3 Shortname]");
			    if (pExtraHea->lenOriExtra > 0)
					snprintf(sratio, 10 - 1, "%3d%%", (int) (100 - (100.0 * pExtraHea->lenZipExtra)/pExtraHea->lenOriExtra));
				else
					strcpy(sratio, "n.a.");
				if (verbose < 2)
				{
				    fprintf(stdout, "%14s  %8lX  %9lu  %9lu  %-4s  %s\n",
				                    "",
				                    pExtraHea->crc32Extra,
				                    pExtraHea->lenOriExtra,
				                    pExtraHea->lenZipExtra,
				                    sratio,
				                    sExtraType);
				}
			    else
				{
				    fprintf(stdout, "%17s  %8lX  %9lu  %9lu  %-4s  %s\n",
				                    "",
				                    pExtraHea->crc32Extra,
				                    pExtraHea->lenOriExtra,
				                    pExtraHea->lenZipExtra,
				                    sratio,
				                    sExtraType);
				}
			}
	    	// proceed with next extra header
	    	pExtraHea = pExtraHea->pNextHea;
	    }
    }
}

/*
    *************************************************************************************
	ndb_fe_printFileEntryHashAndName()

	Prints out the file checksum (= md5) and the filename
    *************************************************************************************
*/


void ndb_fe_printFileEntryHashAndName(struct ndb_s_fileentry *pFile, int verbose)
{
    // add a slash to directory names
    char s_name[NDB_MAXLEN_FILENAME];

    strcpy (s_name, pFile->filenameExt);
    if ((pFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
        strcat (s_name, "/");

    // show file info: verbose = 0: short, 1:medium, 2:long
//	if (verbose > 1)
	{
	    fprintf(stdout, "%32s  %s\n",
	                    ndb_util_printMD5(pFile->md5),
	                    EMXWIN32CONVERT(s_name));
	}
}


/*
    *************************************************************************************
	ndb_fe_printFileEntryInfoRaw()

	Returns the file meta data for ndb_list.c in a "raw" format,
	which is usable for computable processing
    *************************************************************************************
*/


void ndb_fe_printFileEntryInfoRaw(struct ndb_s_fileentry *pFileEntry)
{
	static char sline[1024];
    // create time string
    char stime_c[40];
    char stime_m[40];
    char stime_a[40];
    struct tm *tim_c;
    struct tm *tim_m;
    struct tm *tim_a;
    char s_chapNo[12];
    char s_fileNo[12];
	char s_fileAttr[12];
	char s_lenOri[20];
	char s_lenZip[20];
	char s_md5[40];
    // extra data
	char s_CountExtra[40];
	char s_TypesExtra[40];
	char s_lenOriExtra[12];
	char s_lenZipExtra[12];
	char s_crc32Extra[40];
    ULONG ulExtraOri = 0;
    ULONG ulExtraZip = 0;
	PFILEEXTRADATAHEADER pExtraHea = NULL;
    char s_name[NDB_MAXLEN_FILENAME];
    tim_c = localtime( (const time_t *) &pFileEntry->ctime);
    tim_m = localtime( (const time_t *) &pFileEntry->mtime);
    tim_a = localtime( (const time_t *) &pFileEntry->atime);

    // add a slash to directory names
    strcpy (s_name, pFileEntry->filenameExt);
    if ((pFileEntry->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
        strcat (s_name, "/");

    s_TypesExtra[0] = '\0';
    s_crc32Extra[0] = '\0';
    // creation date
    if (tim_c != NULL)
        strftime(stime_c, 40, "%Y%m%d_%H%M%S", tim_c);
    else
        strcpy(stime_c, "19700101_000000");
    // modification date
    if (tim_m != NULL)
        strftime(stime_m, 40, "%Y%m%d_%H%M%S", tim_m);
    else
        strcpy(stime_m, "19700101_000000");
    // last access date
    if (tim_a != NULL)
        strftime(stime_a, 40, "%Y%m%d_%H%M%S", tim_a);
    else
        strcpy(stime_a, "19700101_000000");

    // print extra data if existing
    if (pFileEntry->nr_ofExtraHea > 0)
    {
		pExtraHea = pFileEntry->pExtraHeader;
		while (pExtraHea != NULL)
		{
	    	ulExtraOri += pExtraHea->lenOriExtra;
	    	ulExtraZip += pExtraHea->lenZipExtra;
			snprintf (s_crc32Extra, 40 - 1, "%s%lX, ", s_crc32Extra, pExtraHea->crc32Extra);
			if (pExtraHea->typExtHea == NDB_EF_OS2EA)
				snprintf (s_TypesExtra, 40 - 1, "%sOS/2 EAs, ", s_TypesExtra);
			else if (pExtraHea->typExtHea == NDB_EF_NTSD)
				snprintf (s_TypesExtra, 40 - 1, "%sNTFS Security, ", s_TypesExtra);
			else if (pExtraHea->typExtHea == NDB_EF_ACL)
				snprintf (s_TypesExtra, 40 - 1, "%sOS/2 ACLs, ", s_TypesExtra);
			else if (pExtraHea->typExtHea == NDB_EF_UNIX)
				snprintf (s_TypesExtra, 40 - 1, "%sUnix, ", s_TypesExtra);
			else if (pExtraHea->typExtHea == NDB_EF_WIN83)
				snprintf (s_TypesExtra, 40 - 1, "%sWin 8.3, ", s_TypesExtra);
			else
				snprintf (s_TypesExtra, 40 - 1, "%s[unknown], ", s_TypesExtra);

	    	// proceed with next extra header
	    	pExtraHea = pExtraHea->pNextHea;
	    }
        if (strlen(s_TypesExtra) > 1)
        {
            s_TypesExtra[strlen(s_TypesExtra) - 2] = '\0';
        }
        if (pFileEntry->nr_ofExtraHea == 1)
            snprintf (s_crc32Extra, 40 - 1, "%lX", ((PFILEEXTRADATAHEADER) pFileEntry->pExtraHeader)->crc32Extra);
        else
            strcpy (s_crc32Extra, "[n.a.]");
    }

	// make strings from meta data if numbers
	snprintf(s_chapNo, 12 - 1, "%d", pFileEntry->chapNo);
	snprintf(s_fileNo, 12 - 1, "%lu", pFileEntry->fileNo);
	snprintf(s_fileAttr, 12 - 1, "%lX", pFileEntry->attributes);
	snprintf(s_lenOri, 20 - 1, "%.0f", (double) pFileEntry->lenOri8);
	snprintf(s_lenZip, 20 - 1, "%.0f", (double) pFileEntry->lenZip8);
	snprintf(s_md5, 40 - 1,  "%s", ndb_util_printMD5(pFileEntry->md5));
	snprintf(s_CountExtra, 12 - 1,  "%d", pFileEntry->nr_ofExtraHea);
	snprintf(s_lenOriExtra, 12 - 1, "%lu", ulExtraOri);
	snprintf(s_lenZipExtra, 12 - 1, "%lu", ulExtraOri);

	snprintf(sline, 1024 - 1, "F:%s", ndb_util_formatraw(s_chapNo));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_fileNo));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(stime_c));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(stime_m));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(stime_a));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_fileAttr));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_lenOri));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_lenZip));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_md5));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_name));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_CountExtra));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_TypesExtra));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_lenOriExtra));
	snprintf(sline, 1024 - 1, "%s:%s", sline, ndb_util_formatraw(s_lenZipExtra));
	snprintf(sline, 1024 - 1, "%s\n",  sline);

	fprintf(stdout,  sline);
	fflush(stdout);
}

/*
    *************************************************************************************
	ndb_fe_addRemark()

	add a remark to the file entry structure; this remark is *not* saved into the
	archive file, it's only used for the error output at the end of the archive
	or extract process
    *************************************************************************************
*/

void ndb_fe_addRemark(struct ndb_s_fileentry *pFile, char *remark)
{
    int len = strlen(remark);

    if (pFile->remark != NULL)
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_fe_addRemark: freeing old remark for '%s'\n", pFile->filenameExt);
            fflush(stdout);
        }
        ndb_free(pFile->remark);
    }

    pFile->remark = ndb_calloc(len + 1, 1, "ndb_fe_addRemark: file remark");
    strcpy(pFile->remark, remark);
}

/*
    *************************************************************************************
	ndb_fe_addBlock()

	add a new block to the file entry block list
    *************************************************************************************
*/

void ndb_fe_addBlock(struct ndb_s_fileentry *pFileEntry, struct ndb_s_blockheader *pBlock)
{
    struct ndb_s_blockheader *pCurrBlock = NULL;

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_fe_addBlock - startup\n");

    // first test preconditions
    if (pFileEntry == NULL)
    {
        fprintf(stdout, "ndb_fe_addBlock: Error adding block - file entry is null\n");
        return;
    }
    if (pBlock == NULL)
    {
        fprintf(stdout, "ndb_fe_addBlock: Error adding block - block is null\n");
        return;
    }
    if (pBlock->nextBlock != NULL)
    {
        fprintf(stdout, "ndb_fe_addBlock: Error adding block - block is a list\n");
        return;
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_fe_addBlock: adding block with CRC %8lX\n", pBlock->crc32);
    // Länge der gezippten Daten im Hauptprogramm ergänzen,
    // sobald bekannt ist, ob der Block bereits vorhanden ist
    // Block in Liste einfügen
    if (pFileEntry->firstBlockHeader == NULL)
    {
        pFileEntry->firstBlockHeader = pBlock;
        // 24Jun03 pw CHG add lastBlockHeader
        pFileEntry->lastBlockHeader  = pBlock;
    }
    else
    {
        // 24Jun03 pw CHG use lastBlockHeader instead of loop over all blocks
        pCurrBlock = pFileEntry->lastBlockHeader;
        pCurrBlock->nextBlock  = pBlock;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "new block added after block %lu/%lu\n",
                            pCurrBlock->fileNo, pCurrBlock->blockNo);
        fflush(stdout);
        pFileEntry->lastBlockHeader = pBlock;
/*
        pCurrBlock = pFileEntry->firstBlockHeader;
        while (pCurrBlock->nextBlock != NULL)
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_fe_addBlock: walking from block %lu/%lu",
                                pCurrBlock->fileNo, pCurrBlock->blockNo);
            fflush(stdout);
            pCurrBlock = pCurrBlock->nextBlock;
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, " to %lu/%lu\n", pCurrBlock->fileNo, pCurrBlock->blockNo);
        }
        pCurrBlock->nextBlock = pBlock;
*/
    }

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_fe_addBlock - finished\n");
    fflush(stdout);

}

/*
    *************************************************************************************
	ndb_file_checkattributes()

	checks whether the current file meta data (given in pFileNew) are still the
	same as in a previous chapter (given in pFileOld).
	checked values are:
	- same filename (using field filenameUTF8)
	- same unzipped length (using field lenOri)
	- same creation time (using field ctime)
	- same last modification time (using field mtime)
	- same file attributes (using field attributes)
    *************************************************************************************
*/

int ndb_file_checkattributes(struct ndb_s_fileentry *pFileNew, struct ndb_s_fileentry *pFileOld)
{
    USHORT isEqual = 1;
    USHORT iBlockSize = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkattributes - startup\n");
    fflush(stdout);

    // check preconditiones
    if (pFileNew == NULL)
    {
        fprintf(stdout, "ndb_file_checkattributes: cannot test a NULL new file\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFileOld == NULL)
    {
        fprintf(stdout, "ndb_file_checkattributes: cannot test against a NULL old file\n");
        return NDB_MSG_NULLERROR;
    }

	// first check if old file looks "okay" - the number of data blocks should be
	// suitable for the file size (perhaps more because of extradata)
	// if the old file in the previous chapter is wrong (but unchanged on disk),
	// then the archiving into the current chapter would copy this fault, if we
	// use '-cf'
	iBlockSize = ndb_getBlockSize();
	if ((pFileOld->lenOri8 > 0) && (((pFileOld->lenOri8 + (iBlockSize - 1))/iBlockSize) > pFileOld->blockCount))
		{
		fprintf(stdout, "warning: file '%s' of previous chapter is corrupt:\n", pFileOld->filenameUTF8);
		fprintf(stdout, "         file size %.0f, expected at least %lu block(s), got %lu\n",
		                (double) pFileOld->lenOri8, (ULONG) ((pFileOld->lenOri8 + (iBlockSize - 1))/iBlockSize), pFileOld->blockCount);
		fflush(stdout);
		isEqual = 0;
	}
	else
	{
		// file size is zero or pFileOld has enough blocks for its filesize
	    if (strcmp(pFileNew->filenameUTF8, pFileOld->filenameUTF8) != 0)
	        isEqual = 0;
	    if (pFileNew->lenOri8 != pFileOld->lenOri8)
	        isEqual = 0;
	    if (pFileNew->ctime != pFileOld->ctime)
	        isEqual = 0;
	    if (pFileNew->mtime != pFileOld->mtime)
	        isEqual = 0;
	    if (pFileNew->attributes != pFileOld->attributes)
	        isEqual = 0;
	}

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkattributes: files are %s\n", (isEqual == 1 ? "equal" : "not equal"));
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_checkattributes - finished\n");
    fflush(stdout);

    if (isEqual == 1)
        return NDB_MSG_OK;
    else
        return NDB_MSG_NOTOK;
}


/*
    *************************************************************************************
	ndb_file_checkattributeswithstat()

	checks whether the current file meta data (given in pStat) are still the
	same as in the previous chapter (given in pFileOld).
	checked values are:
	- same unzipped length (using field lenOri)
	- same creation time (using field ctime)
	- same last modification time (using field mtime)
	- same file attributes (using field attributes)
    *************************************************************************************
*/

int ndb_file_checkattributeswithstat(struct ndb_s_fileentry *pFileOld, struct stat *pStat, ULONG ulAttributes)
{
    USHORT isEqual = 1;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkattributeswithstat - startup\n");
    fflush(stdout);

    // check preconditiones
    if (pFileOld == NULL)
    {
        fprintf(stdout, "ndb_file_checkattributeswithstat: cannot test against a NULL old file\n");
        return NDB_MSG_NULLERROR;
    }
    if (pStat == NULL)
    {
        fprintf(stdout, "ndb_file_checkattributeswithstat: cannot test against a NULL stat structure\n");
        return NDB_MSG_NULLERROR;
    }

    if (pFileOld->lenOri8 != pStat->st_size)
    {
        isEqual = 0;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_file_checkattributeswithstat: size is different (%.0f/%.0f)\n",
                            (double) pFileOld->lenOri8, (double) pStat->st_size);
    }
    if (pFileOld->ctime != pStat->st_ctime)
    {
        isEqual = 0;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_file_checkattributeswithstat: ctime is different (%lu/%lu)\n",
                            (ULONG) pFileOld->ctime, (ULONG) pStat->st_ctime);
    }
    if (pFileOld->mtime != pStat->st_mtime)
    {
        isEqual = 0;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_file_checkattributeswithstat: mtime is different (%lu/%lu)\n",
                            (ULONG) pFileOld->mtime, (ULONG) pStat->st_mtime);
    }
    if (pFileOld->attributes != ulAttributes)
    {
        isEqual = 0;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_file_checkattributeswithstat: file attributes are different (%lX/%lX)\n",
                            pFileOld->attributes, ulAttributes);
    }
    // don't check filename because this function was called because of the same filename

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkattributeswithstat: files are %s\n",
                        (isEqual == 1 ? "equal" : "not equal"));
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_checkattributeswithstat - finished\n");
    fflush(stdout);

    if (isEqual == 1)
        return NDB_MSG_OK;
    else
        return NDB_MSG_NOTOK;
}


/*
    *************************************************************************************
	ndb_file_checkfileswithcrc()

	checks whether the current file has the same length and the same CRC32 values for
	its blocks as the corresponding file of the previous chapter (given in pFileOld).
    *************************************************************************************
*/

int ndb_file_checkfileswithcrc(struct ndb_s_fileentry *pFileNew, struct ndb_s_fileentry *pFileOld)
{
    USHORT isEqual  = 1;
    ULONG8 ul8CurFilLen = 0;
    ULONG8 ul8FilLen    = 0;
    ULONG8 ul8Diff      = 0;
    int  iCurDiff   = 0;
    int currBlockSize = 0;
    struct ndb_s_blockheader *pBlockOld = NULL;
    char *buffer = NULL;
    ULONG luCounter = 0;
    ULONG crc32 = 0L;
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

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkfileswithcrc - startup\n");
    fflush(stdout);

    // check preconditiones
    if (pFileNew == NULL)
    {
        fprintf(stdout, "ndb_file_checkfileswithcrc: cannot test a NULL new file\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFileOld == NULL)
    {
        fprintf(stdout, "ndb_file_checkfileswithcrc: cannot test a NULL old file\n");
        return NDB_MSG_NULLERROR;
    }

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_file_checkfileswithcrc: start_time = %llu\n", start_time);
#endif

    if (pFileNew->lenOri8 != pFileOld->lenOri8)
    {
        isEqual = 0;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_file_checkfileswithcrc: size is different (n: %.0f/o: %.0f)\n",
                            (double) pFileNew->lenOri8, (double) pFileOld->lenOri8);
    }
    else
    {
        // same length -> read all blocks and check their CRC values against pFileOld
        FILE *infile = fopen(pFileNew->filenameExt, "rb");
        if (infile != NULL)
        {
            // init all needed variables
            luCounter = 0;
            ul8FilLen    = pFileNew->lenOri8;
            ul8CurFilLen = 0;
            currBlockSize = ndb_getBlockSize();
            pBlockOld = pFileOld->firstBlockHeader;
            buffer = (char *) ndb_calloc(currBlockSize + 16, 1, "ndb_file_checkfileswithcrc: buffer for reading");
            if (buffer == NULL)
            {
                fprintf(stderr, "ndb_file_checkfileswithcrc: couldn't allocate buffer memory for file reading\n");
                fflush(stderr);
                exit(NDB_MSG_NOMEMORY);
            }

            // now walk through the file on harddrive
            while (ul8CurFilLen < ul8FilLen)
            {
                // first check old block, do we have one?
                if (pBlockOld == NULL)
                {
                    isEqual = 0;
                    break;
                }
                // now read BLOCKSIZE bytes (or the whole rest if lesser)
                ul8Diff = ul8FilLen - ul8CurFilLen;
                if (ul8Diff > currBlockSize) ul8Diff = currBlockSize;
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_file_checkfileswithcrc: reading block %lu with %.0f bytes\n", luCounter, (double) ul8Diff);
                fflush(stdout);
                // read block from file
                iCurDiff = ndb_io_readdata(buffer, ul8Diff, infile);
                // anything went wrong?
                if (iCurDiff != ul8Diff)
                {
                    if (iCurDiff == 0)
                    {
                        fprintf(stderr, "Warning: couldn't read from file '%s'\n-> aborting its CRC check\n",
                                EMXWIN32CONVERT(pFileNew->filenameExt));
                        fflush(stderr);
                    }
                    isEqual = 0;
                    break;
                }
                // calculate crc of block data
                crc32 = ndb_crc32(0L, buffer, iCurDiff);
                // and compare with corresponding block of old file
                if (crc32 != pBlockOld->crc32)
                {
                    if (ndb_getDebugLevel() >= 2)
                    {
                        fprintf(stdout, "Info: block %lu has a different crc\n", luCounter);
                        fflush(stdout);
                    }
                    isEqual = 0;
                    break;
                }
                // proceed with next block
                pBlockOld = pBlockOld->nextBlock;
                // correct read length
                ul8CurFilLen += iCurDiff;
                // update block counter
                luCounter++;
            }

            fclose(infile);
            ndb_free(buffer);
        }
        else
        {
            isEqual = 0;
        }
    }

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_file_checkfileswithcrc: count %lu, average %lu, max %lu, sum %lu\n",
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);
        ndb_util_set_performancetest(TEST_PERFORMANCE_CHECKBYCRC, s);
    }
#endif

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkfileswithcrc: files are %s\n",
                        (isEqual == 1 ? "equal" : "not equal"));
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_checkfileswithcrc - finished\n");
    fflush(stdout);

    if (isEqual == 1)
        return NDB_MSG_OK;
    else
        return NDB_MSG_NOTOK;
}



/*
    *************************************************************************************
	ndb_file_checkfileswithmd5()

	checks whether the current file has the same length and the same md5 value
	as the corresponding file of the previous chapter (given in pFileOld).
    *************************************************************************************
*/

int ndb_file_checkfileswithmd5(struct ndb_s_fileentry *pFileNew, struct ndb_s_fileentry *pFileOld)
{
    USHORT isEqual  = 1;
    ULONG8 ul8CurFilLen = 0;
    ULONG8 ul8FilLen    = 0;
    ULONG8 ul8Diff      = 0;
    int  iCurDiff   = 0;
    int currBlockSize = 0;
    char *buffer = NULL;
    ULONG luCounter = 0;
    // needed for MD5 calculations
    MD5CTX  hasher;
    unsigned char digest[MD5_DIGESTSIZE];
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

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkfileswithmd5 - startup\n");
    fflush(stdout);

    // check preconditiones
    if (pFileNew == NULL)
    {
        fprintf(stdout, "ndb_file_checkfileswithmd5: cannot test a NULL new file\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFileOld == NULL)
    {
        fprintf(stdout, "ndb_file_checkfileswithmd5: cannot test a NULL old file\n");
        return NDB_MSG_NULLERROR;
    }

    // init MD5 context
    MD5_Initialize(&hasher);

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_file_checkfileswithmd5: start_time = %llu\n", start_time);
#endif

    if (pFileNew->lenOri8 != pFileOld->lenOri8)
    {
        isEqual = 0;
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_file_checkfileswithmd5: size is different (n: %.0f/o: %.0f)\n",
                            (double) pFileNew->lenOri8, (double) pFileOld->lenOri8);
    }
    else
    {
        // same length -> read whole file content and check the md5 value against pFileOld
        FILE *infile = fopen(pFileNew->filenameExt, "rb");
        if (infile != NULL)
        {
            // init all needed variables
            luCounter = 0;
            ul8FilLen    = pFileNew->lenOri8;
            ul8CurFilLen = 0;
            currBlockSize = ndb_getBlockSize();
            buffer = (char *) ndb_calloc(currBlockSize + 16, 1, "ndb_file_checkfileswithmd5: buffer for reading");
            if (buffer == NULL)
            {
                fprintf(stderr, "ndb_file_checkfileswithmd5: couldn't allocate buffer memory for file reading\n");
                fflush(stderr);
                exit(NDB_MSG_NOMEMORY);
            }

            // now walk through the file on harddrive
            while (ul8CurFilLen < ul8FilLen)
            {
                // now read BLOCKSIZE bytes (or the whole rest if lesser)
                ul8Diff = ul8FilLen - ul8CurFilLen;
                if (ul8Diff > currBlockSize) ul8Diff = currBlockSize;
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_file_checkfileswithmd5: reading block %lu with %.0f bytes\n", luCounter, (double) ul8Diff);
                fflush(stdout);
                // read block from file
                iCurDiff = ndb_io_readdata(buffer, ul8Diff, infile);
                // anything went wrong?
                if (iCurDiff != ul8Diff)
                {
                    if (iCurDiff == 0)
                    {
                        fprintf(stderr, "Warning: couldn't read from file '%s'\n-> aborting its md5 check\n",
                                EMXWIN32CONVERT(pFileNew->filenameExt));
                        fflush(stderr);
                    }
                    isEqual = 0;
                    break;
                }
                // update md5 with next data block
                MD5_Update(&hasher, buffer, iCurDiff);
                // correct read length
                ul8CurFilLen += iCurDiff;
                // update block counter
                luCounter++;
            }

            fclose(infile);
            ndb_free(buffer);

            // and compare with corresponding block of old file
            MD5_Final(digest, &hasher);
            if (memcmp(pFileOld->md5, digest, MD5_DIGESTSIZE) != 0)
            {
                if (ndb_getDebugLevel() >= 2)
                {
                    fprintf(stdout, "Info: both files have different md5 checksums\n");
                    fflush(stdout);
                }
                isEqual = 0;
            }
        }
        else
        {
            isEqual = 0;
        }
    }

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_file_checkfileswithmd5: count %lu, average %lu, max %lu, sum %lu\n",
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);
        ndb_util_set_performancetest(TEST_PERFORMANCE_CHECKBYMD5, s);
    }
#endif

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_checkfileswithmd5: files are %s\n",
                        (isEqual == 1 ? "equal" : "not equal"));
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_checkfileswithmd5 - finished\n");
    fflush(stdout);

    if (isEqual == 1)
        return NDB_MSG_OK;
    else
        return NDB_MSG_NOTOK;
}



/*
    *************************************************************************************
	ndb_file_copyheaderlist()

	if a file is unchanged we can use the blockheader list of the previous chapter,
	therefore copy all block header from pFileOld to the blockheader lsit of pFileNew
	and use its meta data also

	this is done only for the file data, *not* for the extra data; because extra data
	changes may not be visible by a change of the "usual" file attributes (mtime,
	attributes, file length, ...), the extra data are always archived newly
    *************************************************************************************
*/

int ndb_file_copyheaderlist(struct ndb_s_c_chapter  *pChapter, struct ndb_s_fileentry *pFileNew, struct ndb_s_fileentry *pFileOld)
{
    struct ndb_s_blockheader *pBlockOld;
    struct ndb_s_blockheader *pBlockNew;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_copyheaderlist - startup\n");
    fflush(stdout);

    // check preconditiones
    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_file_copyheaderlist: cannot work with a NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFileNew == NULL)
    {
        fprintf(stdout, "ndb_file_copyheaderlist: cannot copy headers to a NULL file\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFileOld == NULL)
    {
        fprintf(stdout, "ndb_file_copyheaderlist: cannot copy headers from a NULL file\n");
        return NDB_MSG_NULLERROR;
    }

    pBlockOld = pFileOld->firstBlockHeader;

    // copy all block header for 'normal' file data
    // extra data are always created fresh in ndb_archive.c
    while ((pBlockOld != NULL) && ((pBlockOld->blockInfo & NDB_BLOCKINFO_EXTRADATA) != NDB_BLOCKINFO_EXTRADATA))
    {
        if (ndb_getDebugLevel() >= 8) fprintf(stdout, "ndb_file_copyheaderlist: trying to copy block %lu/%lu\n",
                                              pBlockOld->blockNo, pBlockOld->fileNo);
        fflush(stdout);
        // allocate new block
        pBlockNew = (struct ndb_s_blockheader *) ndb_calloc(sizeof(struct ndb_s_blockheader), 1, "ndb_compressStream: new pBlock");
        if (pBlockNew == NULL)
        {
            fprintf(stderr, "ndb_file_copyheaderlist: couldn't allocate block header\n");
            exit(NDB_MSG_NOMEMORY);
        }
        // copy block data
        pBlockNew->blockNo = pBlockOld->blockNo;
        pBlockNew->fileNo = pFileNew->fileNo;
        pBlockNew->chapNo = pChapter->chapterNo;
        pBlockNew->dataFileNo = pBlockOld->dataFileNo;
        pBlockNew->crc32 = pBlockOld->crc32;
        pBlockNew->lenOri = pBlockOld->lenOri;
        pBlockNew->lenZip = pBlockOld->lenZip;
//      staHead is the adress of the blockheader itself in the
//      archive file and set by ndb_io_writeBlockHeader()
//        pBlockNew->staHea = pBlockOld->staHea;
        pBlockNew->staDat = pBlockOld->staDat;
        pBlockNew->blockInfo = pBlockOld->blockInfo;
        pBlockNew->blockType = pBlockOld->blockType;
        pBlockNew->BlockDataOri = NULL;
        pBlockNew->BlockDataZip = NULL;
        pBlockNew->nextBlock = NULL;
        pBlockNew->sameCRC = NULL;
        // add block to file list
        ndb_fe_addBlock(pFileNew, pBlockNew);
        pFileNew->blockCount++;
        // block needs not to be added to the block table
        // because we know it's identical to an existing ;-)
//        ndb_block_addBlockToTable(pBlockNew);
        // increment block header counter in chapter
        pChapter->blockHeaNo++;
        // switch to next block
        pBlockOld = pBlockOld->nextBlock;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_copyheaderlist: added %lu of %lu blocks to the new file\n",
                        pFileNew->blockCount, pFileOld->blockCount);
    fflush(stdout);

    // now copy the file attributes etc itself
    pFileNew->lenOri8         = pFileOld->lenOri8;
    pFileNew->lenZip8         = 0; // pw, 17-Jan-2004: file data identical!
    memcpy(pFileNew->md5, pFileOld->md5, MD5_DIGESTSIZE);
//  nlockcount was set in the loop above
//    pFileNew->blockCount      = pFileOld->blockCount;
    pFileNew->FSType          = pFileOld->FSType;
    // set flag for NDB that file identical to same of previous chapter
    pFileNew->flags           = pFileOld->flags | NDB_FILEFLAG_IDENTICAL;
    // extra data are always created fresh in ndb_archive.c
    // therefore set it to zero here
    pFileNew->nr_ofExtraHea = 0;
    pFileNew->lenExtHea = 0;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_copyheaderlist - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
	ndb_file_markpath2extract()

	scan the filename for directory parts and mark each found dir with the
	action flag NDB_ACTION_EXTRACT
    *************************************************************************************
*/

int ndb_file_markpath2extract(PCHAPTER pChapter, PFILEENTRY pFile)
{
    static char sPathOld[NDB_MAXLEN_FILENAME] = "\0";
    char sPathNew[NDB_MAXLEN_FILENAME] = "\0";
    char filenameUTF8[NDB_MAXLEN_FILENAME] = "\0";
    struct ndb_s_fileentry feDummy;
    PFILEENTRY pFEDir;
    int retval = NDB_MSG_OK;
    char *p;
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

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_markpath2extract - startup\n");
    fflush(stdout);

    // check preconditiones
    if (pChapter == NULL)
    {
        fprintf(stdout, "ndb_file_markpath2extract: cannot test against a NULL chapter\n");
        return NDB_MSG_NULLERROR;
    }
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_file_markpath2extract: cannot test a NULL file\n");
        return NDB_MSG_NULLERROR;
    }


    if (ndb_getDebugLevel() >= 8)
        ndb_chap_printHashInfo(pChapter);
    fflush(stdout);

    // allocate memory for filename
    memset(&feDummy, 0, sizeof(struct ndb_s_fileentry));
    feDummy.filenameUTF8 = filenameUTF8;

    // extract complete path from filename
    strcpy(sPathNew, pFile->filenameUTF8);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_file_markpath2extract: process filename '%s'\n", sPathNew);
    fflush(stdout);

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_file_markpath2extract: start_time = %llu\n", start_time);
#endif

    if (NULL != (p = strrchr(sPathNew, '/')))
    {
        // cut after last slash
        *(p) = '\0';

        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_file_markpath2extract: extracted path '%s'\n", sPathNew);
        fflush(stdout);

        // have we processed this dir at the last time?
        if (strcmp(sPathNew, sPathOld) != 0)
        {
            do
            {
                // other dir -> process all parts of it and mark them
                strcpy(feDummy.filenameUTF8, sPathNew);
                feDummy.lenfilenameUTF8 = strlen(sPathNew);
                if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_file_markpath2extract: created dummy file entry for path '%s'\n", sPathNew);
                fflush(stdout);
                if (NDB_MSG_OK == ndb_chap_findfile(pChapter, &feDummy, &pFEDir))
                {
	                if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_file_markpath2extract: path found in chapter -> marking for extraction\n");
	                fflush(stdout);
	                    pFEDir->action = NDB_ACTION_EXTRACT;
                }
                else
                {
                    if (ndb_getDebugLevel() >= 3)
                    {
    					fprintf(stdout, "warning: dir '%s' not found in current chapter %04u\n",
    					        sPathNew, pChapter->chapterNo);
    					fflush(stdout);
    			    }
                }
                // proceed with next directory
                p = strrchr(sPathNew, '/');
                if (p != NULL)
                {
                    *p = '\0';
                    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_file_markpath2extract: cut path to '%s'\n", sPathNew);
                    fflush(stdout);
                }
            }
            while(NULL != p);
            // save current path for next time
            strcpy(sPathOld, sPathNew);
        }
    }

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
//      printf("ndb_file_markpath2extract: curr_time  = %llu\n", curr_time);
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_file_markpath2extract: count %lu, average %lu, max %lu, sum %lu\n",
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);		fflush(stdout);
        ndb_util_set_performancetest(TEST_PERFORMANCE_MARKPATH2EXTRACT, s);
    }
#endif
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_file_markpath2extract - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
	ndb_fe_makeFilenameUTF8()

	uses the filename from harddisk and converts it to UTF8; if this can't be done,
	copy the original filename to field filenameUTF8 and set the flag NDB_FILEFLAG_NOUTF8;
	also do some cleaning: remove drive letter and leading slashes to avoid saving
	absolute paths
    *************************************************************************************
*/

int ndb_fe_makeFilenameUTF8(struct ndb_s_fileentry *pFileEntry)
{
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_makeFilenameUTF8: startup\n");
    fflush(stdout);

    pFileEntry->filenameUTF8 = ndb_utf8_createStringUTF8(pFileEntry->filenameExt);
    // could filename be converted to UTF-8?
    if (pFileEntry->filenameUTF8 == NULL)
    {
        // clear conversion flag
        pFileEntry->flags |= NDB_FILEFLAG_NOUTF8;
        // copy external name to internal name
        pFileEntry->filenameUTF8 = ndb_calloc(1, 1 + strlen(pFileEntry->filenameExt), "ndb_fe_makeFilenameUTF8: raw filenameUTF8");
        if (pFileEntry->filenameUTF8 == NULL)
        {
            fprintf(stderr, "ndb_fe_makeFilenameUTF8: couldn't allocate memory for filename\n");
            fflush(stderr);
            exit (NDB_MSG_NOMEMORY);
        }
        // no conversion -> copy "raw" filename, set length
        strcpy (pFileEntry->filenameUTF8, pFileEntry->filenameExt);
    }

    // strip drives, UNC names, leading '/' and './' from filename
    // clean filename from leading path separator
	ndb_util_cleanIntPath(pFileEntry->filenameUTF8);

	// set length of filename
    pFileEntry->lenfilenameUTF8   = strlen(pFileEntry->filenameUTF8);

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_fe_makeFilenameUTF8: filename (ext):  '%s'\n", pFileEntry->filenameExt);
        fprintf(stdout, "ndb_fe_makeFilenameUTF8: filename (UTF8): '%s'\n", pFileEntry->filenameUTF8);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_makeFilenameUTF8: finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
	ndb_fe_makeFilenameCP()

	converts the filename in UTF8 format (as saved in the archive) in a filename
	with the current codepage; if this is not possible (e.g. current codepage not
	implemented in NDB) copy the UTF8 value only
    *************************************************************************************
*/

int ndb_fe_makeFilenameCP(struct ndb_s_fileentry *pFileEntry)
{
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_makeFilenameCP: startup\n");
    fflush(stdout);

    if ((pFileEntry->flags & NDB_FILEFLAG_NOUTF8) != NDB_FILEFLAG_NOUTF8)
    {
        pFileEntry->filenameExt = ndb_utf8_createStringCP(pFileEntry->filenameUTF8);
    }

    // could filename be converted to OS codepage?
    if (pFileEntry->filenameExt == NULL)
    {
        // copy external name to internal name
        pFileEntry->filenameExt = ndb_calloc(1, 1 + strlen(pFileEntry->filenameUTF8), "ndb_fe_makeFilenameCP: raw filenameExt");
        if (pFileEntry->filenameExt == NULL)
        {
            fprintf(stderr, "ndb_fe_makeFilenameCP: couldn't allocate memory for filename\n");
            fflush(stderr);
            exit (NDB_MSG_NOMEMORY);
        }
        // no conversion -> copy "raw" filename, set length
        strcpy (pFileEntry->filenameExt, pFileEntry->filenameUTF8);
    }
    pFileEntry->lenfilenameExt   = strlen(pFileEntry->filenameExt);

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_fe_makeFilenameCP: filename (UTF8): '%s'\n", pFileEntry->filenameUTF8);
        fprintf(stdout, "ndb_fe_makeFilenameCP: filename (ext):  '%s'\n", pFileEntry->filenameExt);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_makeFilenameCP: finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
	ndb_fe_writeAllBlockHeader()

	writes all block header structures (file data header and extra data header) to the
	given archive file handle

	Precondition: the file pointer of the archive file has to set before this function
    *************************************************************************************
*/

int ndb_fe_writeAllBlockHeader(struct ndb_s_fileentry *pFileEntry, FILE *f, ULONG *pBlockcount)
{
    int retval      = 0;
    ULONG blocksFile  = 0;
    ULONG blocksExtra = 0;
    struct ndb_s_blockheader *pCurrHeader = NULL;
    char action[8];

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_writeAllBlockHeader - startup\n");

    if (pFileEntry == NULL)
    {
        fprintf(stdout, "ndb_fe_writeAllBlockHeader: cannot save block header of a NULL file entry\n");
        return NDB_MSG_NULLERROR;
    }

    if (pFileEntry->action == NDB_ACTION_IGNORE)
       strcpy(action, "IGNORE ");
    else if (pFileEntry->action == NDB_ACTION_ADD)
       strcpy(action, "ADD    ");
    else if (pFileEntry->action == NDB_ACTION_UPDATE)
       strcpy(action, "UPDATE ");
    else if (pFileEntry->action == NDB_ACTION_KEEP)
       strcpy(action, "KEEP   ");
    else if (pFileEntry->action == NDB_ACTION_EXTRACT)
       strcpy(action, "EXTRACT");
    else if (pFileEntry->action == NDB_ACTION_FAILED)
       strcpy(action, "FAILED ");
    else
       strcpy(action, "[NONE] ");

    // save all file header (ignore block type)
    pCurrHeader = pFileEntry->firstBlockHeader;
    if (pCurrHeader == NULL)
    {
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_fe_writeAllBlockHeader: no block header (file data) to save for '%s'\n",
                            pFileEntry->filenameExt);
        fflush(stdout);
    }
    else
    {
        if (ndb_getDebugLevel() >= 5)
        {
            if  (pCurrHeader != NULL)
            {
                fprintf(stdout, "ndb_fe_writeAllBlockHeader: writing blocks for file number %lu\n", pFileEntry->fileNo);
                fflush(stdout);
            }
        }
        while (pCurrHeader != NULL)
        {
            if (pCurrHeader->fileNo != pFileEntry->fileNo)
            {
                fprintf(stdout, "ndb_fe_writeAllBlockHeader: warning: fileNo file is %lu, fileNo block is %lu\n",
                                pFileEntry->fileNo, pCurrHeader->fileNo);
                fflush(stdout);
            }
            if (ndb_getDebugLevel() >= 8)
            {
                fprintf(stdout, "ndb_fe_writeAllBlockHeader: trying to save block %lu/%lu with block type %d\n",
                                 pCurrHeader->fileNo, pCurrHeader->blockNo, pCurrHeader->blockInfo);
                fflush(stdout);
            }
            retval = ndb_io_writeBlockHeader(f, pCurrHeader);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stdout, "ndb_fe_writeAllBlockHeader: Error saving block header\n");
                return retval;
            }
            if ((pCurrHeader->blockInfo & NDB_BLOCKINFO_EXTRADATA) == NDB_BLOCKINFO_EXTRADATA)
                blocksExtra++;
            else
                blocksFile++;
            pCurrHeader = pCurrHeader->nextBlock;
        }
        if (pFileEntry->blockCount != blocksFile + blocksExtra)
        {
            fprintf(stderr, "ndb_fe_writeAllBlockHeader: internal error: blockcount %lu, saved %lu + %lu blocks for '%s'\n",
                            pFileEntry->blockCount, blocksFile, blocksExtra, pFileEntry->filenameExt);
            fflush(stderr);
        }
        if ((ndb_getDebugLevel() >= 5) && (pFileEntry->blockCount == blocksFile + blocksExtra))
        {
            fprintf(stdout, "ndb_fe_writeAllBlockHeader: %s  %lu (%lu + %lu) block header saved for '%s'\n",
                            action, pFileEntry->blockCount, blocksFile, blocksExtra, pFileEntry->filenameExt);
        }
        fflush(stdout);
    }

    (*pBlockcount) += blocksFile + blocksExtra;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_writeAllBlockHeader - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
	ndb_fe_addExtraHeader()

	adds extra data header structure(s) to the given fileentry structure
    Attention: this function doesn't increment pFile->nr_ofExtraHea,
    do it for yourself outside of this function!
    *************************************************************************************
*/

int ndb_fe_addExtraHeader(struct ndb_s_fileentry *pFile, struct ndb_s_fileextradataheader *pNewExtraHea)
{
	int retval = NDB_MSG_OK;
	struct ndb_s_fileextradataheader *pCurrHea = NULL;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_addExtraHeader - startup\n");
    fflush(stdout);

	// check preconditions
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_fe_addExtraHeader: cannot add extra data header to a NULL file entry\n");
        return NDB_MSG_NULLERROR;
    }
    if (pNewExtraHea == NULL)
    {
        fprintf(stdout, "ndb_fe_addExtraHeader: cannot add a NULL extra data header to a file entry\n");
        return NDB_MSG_NULLERROR;
    }

	// add pExtraHea to extra data header list
    if (ndb_getDebugLevel() >= 7)
    	fprintf(stdout, "ndb_fe_addExtraHeader: trying to add extra header (type %X) - should be %d. header\n",
       			pNewExtraHea->typExtHea, pFile->nr_ofExtraHea);
    fflush(stdout);
	if (pFile->pExtraHeader == NULL)
	{
		pFile->pExtraHeader = pNewExtraHea;
        if (ndb_getDebugLevel() >= 7)
        	fprintf(stdout, "ndb_fe_addExtraHeader: extra header added as first header\n");
        fflush(stdout);
	}
	else
	{
		pCurrHea = pFile->pExtraHeader;
		while (pCurrHea->pNextHea != NULL)
		{
			pCurrHea = pCurrHea->pNextHea;
		}
		pCurrHea->pNextHea = pNewExtraHea;
        if (ndb_getDebugLevel() >= 7) fprintf(stdout, "ndb_fe_addExtraHeader: extra header added as second or later header\n");
        fflush(stdout);
	}

    if (ndb_getDebugLevel() >= 7) fprintf(stdout, "ndb_fe_addExtraHeader: pFile->pExtraHeader: 0x%lX\n", (ULONG) pFile->pExtraHeader);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_addExtraHeader - finished\n");
    fflush(stdout);

	return retval;
}


/*
    *************************************************************************************
	ndb_fe_printExtraHeader()

	prints an extra data header structure
    *************************************************************************************
*/


void ndb_fe_printExtraHeader(struct ndb_s_fileextradataheader *pCurrHea)
{
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_printExtraHeader - startup\n");
    fflush(stdout);

	// check preconditions
    if (pCurrHea == NULL)
    {
        fprintf(stdout, "ndb_fe_printExtraHeader: cannot print a NULL extra data header\n");
        return;
    }

	// start with first extra header
    fprintf(stdout, "Extra Data Header:\n");
    fprintf(stdout, "-------------------------------------------------------------\n");
    fflush(stdout);
    fprintf(stdout, "extra header type:        %d\n", pCurrHea->typExtHea);
    fprintf(stdout, "extra header length:      %d\n", pCurrHea->lenExtHea);
    fprintf(stdout, "original data length:     %lu\n", pCurrHea->lenOriExtra);
    fprintf(stdout, "zipped data length:       %lu\n", pCurrHea->lenZipExtra);
    fprintf(stdout, "crc32:                    0x%lX\n", pCurrHea->crc32Extra);
    fprintf(stdout, "number of blocks:         %d\n", pCurrHea->blockCountExtra);
    fprintf(stdout, "pointer to first block:   0x%lX\n", (ULONG) pCurrHea->firstBlockHeader);
    fprintf(stdout, "length of direct data:    %d\n", pCurrHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER);
    fprintf(stdout, "-------------------------------------------------------------\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_printExtraHeader - finished\n");
    fflush(stdout);

	return;
}


/*
    *************************************************************************************
	ndb_fe_printAllExtraHeader()

	prints all extra data header structures of the given fileentry structure
    *************************************************************************************
*/


void ndb_fe_printAllExtraHeader(struct ndb_s_fileentry *pFile)
{
	struct ndb_s_fileextradataheader *pCurrHea = NULL;
	int i = 0;

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_printAllExtraHeader - startup\n");
    fflush(stdout);

	// check preconditions
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_fe_printAllExtraHeader: cannot print extra data header of a NULL file entry\n");
        return;
    }

	// start with first extra header
    fprintf(stdout, "File '%s' has %d extra header\n", pFile->filenameUTF8, pFile->nr_ofExtraHea);
    fprintf(stdout, "-------------------------------------------------------------\n");
    fflush(stdout);
	pCurrHea = pFile->pExtraHeader;
    for (i = 0; i < pFile->nr_ofExtraHea; i++)
	{
        if (pCurrHea == NULL)
        {
            fprintf(stdout, "error - extra header %d is NULL\n", i);
            fprintf(stdout, "-------------------------------------------------------------\n");
            fflush(stdout);
            continue;
        }
        ndb_fe_printExtraHeader(pCurrHea);
        pCurrHea = pCurrHea->pNextHea;
	}

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "ndb_fe_printAllExtraHeader - finished\n");
    fflush(stdout);

	return;
}


/*
    *************************************************************************************
	ndb_fe_createExtraHeader2Buffer()

	create one buffer from direct data or zipped buffers of given block list;
    *************************************************************************************
*/

int ndb_fe_createExtraHeader2Buffer(struct ndb_s_fileextradataheader *pExtraHea, char **ppBuf,
                             ULONG *pSizeExtra, FILE *fArchive)
{
    struct ndb_s_blockheader *pBlock;
    int retval = NDB_MSG_OK;
    int currSizeExtraData = 0;
    char *pBufExtra = NULL;
	char *pBufferIn = NULL;
    int i = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_fe_createExtraHeader2Buffer - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 8)
        ndb_fe_printExtraHeader(pExtraHea);
    fflush(stdout);

    // allocate memory for input buffer (can be the same for every block)
    pBufferIn   = (char *) ndb_calloc(ndb_getBlockSize() + 4096, 1, "ndb_fe_createExtraHeader2Buffer: block for zip");
    if (pBufferIn == NULL)
    {
        fprintf(stderr, "ndb_fe_createExtraHeader2Buffer: couldn't allocate memory (%db) for data buffer\n", ndb_getBlockSize() + 4096);
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }

    // first check for direct data (or data in blocks?)
    if (pExtraHea->lenExtHea > NDB_SAVESIZE_FILEEXTRAHEADER)
    {
        // direct extra data - included in pExtraHeader->pData
        *ppBuf = pExtraHea->pData;
        *pSizeExtra = pExtraHea->lenExtHea - NDB_SAVESIZE_FILEEXTRAHEADER;
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: found direct data with %lu bytes extra data\n", *pSizeExtra);
        fflush(stdout);
        return NDB_MSG_OK;
    }

    // otherwise the data must be extracted from a buffer list
    currSizeExtraData = 0;
    pBufExtra = ndb_calloc(1, 1, "ndb_fe_createExtraHeader2Buffer: buffer for extra data");
    if (pBufExtra == NULL)
    {
        fprintf(stderr, "ndb_fe_createExtraHeader2Buffer: Error: couldn't allocate memory for extra data buffer\n");
        fflush(stderr);
        exit (NDB_MSG_NOMEMORY);
    }

    pBlock = pExtraHea->firstBlockHeader;
    for (i = 0; i < pExtraHea->blockCountExtra; i++)
    {
        // do some checks
        if (pBlock == NULL)
        {
            fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: internal error - expected %d blocks, found %d\n",
                            pExtraHea->blockCountExtra, i);
            fflush(stdout);
            break;
        }
        if ((pBlock->blockInfo & NDB_BLOCKINFO_EXTRADATA) != NDB_BLOCKINFO_EXTRADATA)
        {
            fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: internal error - expected extra data block, found block type %d\n",
                            pBlock->blockInfo);
            fflush(stdout);
            break;
        }
		// if cascade: check for same extra data type in extra data header and block type
        if ((pExtraHea->typExtHea == NDB_EF_OS2EA) && (pBlock->blockType != NDB_BLOCKTYPE_OS2EAS))
        {
            fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: internal error - block type %d found in OS/2 EA data\n",
                            pBlock->blockType);
            fflush(stdout);
            break;
        }
        if ((pExtraHea->typExtHea == NDB_EF_NTSD) && (pBlock->blockType != NDB_BLOCKTYPE_NTSECURITY))
        {
            fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: internal error - block type %d found in NT SD data\n",
                            pBlock->blockType);
            fflush(stdout);
            break;
        }
		// TODO: check other extra data types also (as soon as they are implemented)

		// load zipped data from archive file
        // set pointer for load in buffer
        pBlock->BlockDataZip = pBufferIn;
        /* position to start of block data */
        fseek(fArchive, pBlock->staDat, SEEK_SET);
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: reading extra data block (%X bytes) at file position %lX\n",
                        pBlock->lenZip, pBlock->staDat);
        fflush(stdout);
        retval = ndb_io_readBlockZipDataFromFile(pBlock);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "ndb_fe_createExtraHeader2Buffer: Error reading extra data (%lu/%lu) from archive file\n",
                            pBlock->fileNo, pBlock->blockNo);
            fflush(stderr);
            retval = NDB_MSG_READERROR;
            break;
        }

        // original data buffer must be extra for each block
        pBlock->BlockDataOri = (char *) ndb_calloc(ndb_getBlockSize() + 4096, 1, "ndb_fe_createExtraBuffer: block for ori");
	    if (pBlock->BlockDataOri == NULL)
	    {
	        fprintf(stderr, "ndb_fe_createExtraHeader2Buffer: couldn't allocate memory (%db) for data output buffer\n", ndb_getBlockSize() + 4096);
	        fflush(stderr);
	        exit(NDB_MSG_NOMEMORY);
	    }

		// unzip data
        if (NDB_MSG_OK != (retval = ndb_block_uncompressblock(pBlock)))
        {
            fprintf(stderr, "ndb_fe_createExtraHeader2Buffer: Error: unpacking extra data block (%lu/%lu) from archive\n",
                            pBlock->fileNo, pBlock->blockNo);
            fflush(stderr);
            retval = NDB_MSG_CORRUPTDATA;
            break;
        }

        // modify size of extra data buffer
        pBufExtra = ndb_realloc(pBufExtra, currSizeExtraData + pBlock->lenOri, "ndb_fe_createExtraHeader2Buffer: realloc extra data buffer");
        if (pBufExtra == NULL)
        {
            fprintf(stderr, "ndb_fe_createExtraHeader2Buffer: Error: couldn't reallocate memory for extra data buffer\n");
            fflush(stderr);
            exit (NDB_MSG_NOMEMORY);
        }
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: copying %u bytes to buffer position %u\n", pBlock->lenOri, currSizeExtraData);
        fflush(stdout);
        memcpy (&pBufExtra[currSizeExtraData], pBlock->BlockDataOri, pBlock->lenOri);
        currSizeExtraData += pBlock->lenOri;

        // switch to next block
        pBlock = pBlock->nextBlock;
    }

    if (retval == NDB_MSG_OK)
    {
        *ppBuf = pBufExtra;
        *pSizeExtra = currSizeExtraData;
    }
    else
    {
        *ppBuf = NULL;
        *pSizeExtra = 0L;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_fe_createExtraHeader2Buffer: found extra data (type %X) with %lu bytes extra data\n",
                        pExtraHea->typExtHea, *pSizeExtra);
    fflush(stdout);

    // input buffer can be freed in any case
    ndb_free(pBufferIn);
    // the output buffers of all blocks might not all be allocated
    // better to have a memory leak than a crash ...
    if (retval == NDB_MSG_OK)
    {
        // free memory of all blocks
        pBlock = pExtraHea->firstBlockHeader;
        while(pBlock != NULL)
        {
            ndb_free(pBlock->BlockDataOri);
            pBlock = pBlock->nextBlock;
        }
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_fe_createExtraHeader2Buffer - finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
	ndb_fe_createBuffer2ExtraHeader()

	copy the extra data to the direct data field of pExtraHea or create a list of
    zipped buffers for pExtraHea;
    *************************************************************************************
*/

int ndb_fe_createBuffer2ExtraHeader(struct ndb_s_fileextradataheader *pExtraHea, char *pBufExtra,
                             ULONG sizeExtra, FILE *fArchive)
{
    char   *pNextExtra;
    USHORT currSizeExtra;
    USHORT nextSize;
    struct ndb_s_blockheader *pBlock;
    struct ndb_s_blockheader *pFirstBlock;
    struct ndb_s_blockheader *pPreviousBlock;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_fe_createBuffer2ExtraHeader - startup\n");
    fflush(stdout);

    // Default: no extra data
    pExtraHea->lenExtHea = 0;
    pExtraHea->lenOriExtra = 0;
    pExtraHea->pData = NULL;
    pExtraHea->pNextHea = NULL;
    pExtraHea->crc32Extra = 0L;
    pExtraHea->lenZipExtra = 0L;
    pExtraHea->blockCountExtra = 0;
    pFirstBlock    = NULL;
    pPreviousBlock = NULL;
    // don't change pExtraHea->typExtHea, it was set outside before

    if (sizeExtra == 0)
        return NDB_MSG_OK;

    // fill structure data
    pExtraHea->lenExtHea = NDB_SAVESIZE_FILEEXTRAHEADER;
    pExtraHea->lenOriExtra = sizeExtra;
    pExtraHea->pData = NULL;
    pExtraHea->pNextHea = NULL;
    pExtraHea->crc32Extra = ndb_crc32(0L, pBufExtra,sizeExtra);
    pExtraHea->lenZipExtra = 0L;
    pExtraHea->blockCountExtra = 0;

    pNextExtra     = pBufExtra;
    currSizeExtra  = sizeExtra;

    // convert extra data into block header format
    while (currSizeExtra > 0)
// TODO: later optimization: use direct extra data if currSizeExtra < 100 (100 or simular)
    {
        pBlock = (struct ndb_s_blockheader *) ndb_calloc(sizeof(struct ndb_s_blockheader), 1, "ndb_fe_createBuffer2ExtraHeader: new pBlock");
        if (pBlock == NULL)
        {
            fprintf(stderr, "ndb_fe_createBuffer2ExtraHeader: couldn't allocate block header\n");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
        // set pointer to first block
        if (pFirstBlock == NULL)
        {
            pFirstBlock = pBlock;
			pExtraHea->firstBlockHeader = pBlock;
        }
        // take blocksize or the rest of extra data
        if (currSizeExtra > ndb_getBlockSize())
            nextSize = ndb_getBlockSize();
        else
            nextSize = currSizeExtra;
        // get block
        pBlock->BlockDataOri = (char *) ndb_calloc(nextSize, 1, "ndb_fe_createBuffer2ExtraHeader: data buffer for extra data");
        if (pBlock->BlockDataOri == NULL)
        {
            fprintf(stderr, "ndb_fe_createBuffer2ExtraHeader: couldn't allocate buffer memory\n");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
        // copy found extra data into pBlock memory
        memcpy(pBlock->BlockDataOri, pNextExtra, nextSize);
        // set values for header block
        pBlock->lenOri         = nextSize;
        pBlock->staHea         = 0;
        pBlock->staDat         = 0;
//        pBlock->blockInfo      = NDB_BLOCKINFO_FIRST | NDB_BLOCKINFO_LAST | NDB_BLOCKINFO_EXTRADATA;
        pBlock->blockInfo      = NDB_BLOCKINFO_EXTRADATA;
		if (pExtraHea->typExtHea == NDB_EF_OS2EA)
        	pBlock->blockType = NDB_BLOCKTYPE_OS2EAS;
		else if (pExtraHea->typExtHea == NDB_EF_NTSD)
        	pBlock->blockType = NDB_BLOCKTYPE_NTSECURITY;
		else if (pExtraHea->typExtHea == NDB_EF_UNIX)
        	pBlock->blockType = NDB_BLOCKTYPE_UNIX;
		else if (pExtraHea->typExtHea == NDB_EF_ACL)
        	pBlock->blockType = NDB_BLOCKTYPE_OS2ACLS;
        pBlock->nextBlock      = NULL;
        pBlock->sameCRC        = NULL;
        /* calculate CRC from original data */
        pBlock->crc32 = ndb_crc32(0L, pBlock->BlockDataOri, pBlock->lenOri);
        /* compress original data */
        pBlock->BlockDataZip = (char *) ndb_calloc(nextSize + 4096, 1, "ndb_fe_createBuffer2ExtraHeader: buffer for zipped extra data");
        if (pBlock->BlockDataZip == NULL)
        {
            fprintf(stderr, "ndb_fe_createBuffer2ExtraHeader: callocs for buffer (zipped extra data) failed\n");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
        ndb_block_compressblock(pBlock);
		// fill data of extra header structure
		pExtraHea->lenZipExtra += pBlock->lenZip;
		pExtraHea->blockCountExtra++;
        /* pBlock->lenZip already set in ndb_block_compressblock() */
        // switch to next block of extra data
        pNextExtra += nextSize;
        currSizeExtra -= nextSize;
        if (pPreviousBlock != NULL)
        {
            pPreviousBlock->nextBlock = pBlock;
        }
        pPreviousBlock = pBlock;
    }

    if (pFirstBlock != NULL)
    {
        pFirstBlock->blockInfo     = pFirstBlock->blockInfo | NDB_BLOCKINFO_FIRST;
    }
    if (pPreviousBlock != NULL)
    {
        // pPreviousBlock contains last block after while loop
        pPreviousBlock->blockInfo  = pPreviousBlock->blockInfo | NDB_BLOCKINFO_LAST;
     }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_fe_createBuffer2ExtraHeader - finished\n");
    fflush(stdout);

    return retval;
}

