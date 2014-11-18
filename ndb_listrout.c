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


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string.h>


#include "ndb.h"
#include "ndb_prototypes.h"



/* ======================================================================== */
/* creating and using a file list                                           */
/* ======================================================================== */

/* definition lokated at ndb.h
struct ndb_l_filelist
{
    ULONG fileCount;
    struct ndb_s_fileentry *pFirstFile;
    struct ndb_s_fileentry *pCurrFile;
    struct ndb_s_fileentry *pLastFile;
};
*/


/*
    *************************************************************************************
	ndb_list_initFileList()

	inits the file list structure
    *************************************************************************************
*/

void ndb_list_initFileList(struct ndb_l_filelist *pFileList, int bUseHash)
{
	int i;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_initFileList - startup\n");

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_initFileList: Error: couldn't init a null pointer\n");
    }
    else
    {
        pFileList->pFirstFile  = NULL;
        pFileList->pCurrFile   = NULL;
        pFileList->pLastFile   = NULL;
        pFileList->fileCount = 0;
        pFileList->ppHashtableFiles = NULL;
#if defined(USE_HASHTABLE_CRC_FILENAMES)
        if (bUseHash)
        {
            pFileList->ppHashtableFiles = ndb_calloc(NDB_SIZE_FILESTABLE, sizeof(struct ndb_s_fileentry *), "ndb_list_initFileList");
    		if (pFileList->ppHashtableFiles == NULL)
    		{
    		        fprintf(stderr, "Error: couldn't allocate memory for hashtable of filenames\n");
    		        exit (NDB_MSG_NOMEMORY);
    		}
    	    for (i = 0; i < NDB_SIZE_FILESTABLE; i++)
    	    {
    	    	pFileList->ppHashtableFiles[i] = NULL;
    	    }
        }
#endif
    }
    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_initFileList - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
	ndb_list_addToFileList

	adds a file entry structure to the file list structure;
	if the file already is in the list (check uses filenameUTF8), do nothing except
	copying the new actioncode over the old one (used for changing file addition mode
	from including to excluding and vice versa)
    *************************************************************************************
*/

void ndb_list_addToFileList(struct ndb_l_filelist *pFileList, struct ndb_s_fileentry *pFile)
{
	struct ndb_s_fileentry *pCurrent;
	short  bFileFound = 0;
#ifdef USE_HASHTABLE_CRC_FILENAMES
    ULONG crc       = 0;
    ULONG lCurrCRC  = 0;
#endif
	int len;

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


    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_addToFileList - startup\n");
    fflush(stdout);

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_list_addToFileList: start_time = %llu\n", start_time);
#endif

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_addToFileList: Error: cannot work with a null pointer\n");
	    fflush(stdout);
    }
    else if (pFile->filenameUTF8 == NULL)
    {
        fprintf(stdout, "ndb_list_addToFileList: Error: cannot work with a null filename\n");
	    fflush(stdout);
    }
    else
    {
        // empty list, add as first file?
        if (pFileList->pFirstFile == NULL)
        {
            pFileList->pFirstFile = pFile;
            pFileList->pCurrFile  = pFile;
            pFileList->pLastFile  = pFile;
            pFileList->fileCount  = 1;
        }
        else
        {
#ifdef USE_HASHTABLE_CRC_FILENAMES
if (pFileList->ppHashtableFiles == NULL)
{
#endif
            // check for existing files
	    	len = strlen(pFile->filenameUTF8);
			pCurrent = pFileList->pFirstFile;
			while (pCurrent != NULL)
			{
				// small optimization: compare filenames only if same length
				if (len == pCurrent->lenfilenameUTF8)
				{
					if (strcmp(pFile->filenameUTF8, pCurrent->filenameUTF8) == 0)
					{
						bFileFound = 1;
					    if (ndb_getDebugLevel() >= 5)
					    {
					        fprintf(stdout, "ndb_list_addToFileList: file '%s' already in file list\n",
					                        pFile->filenameExt);
						    fflush(stdout);
					    }
	                    // check action flag
	                    if (pFile->action != pCurrent->action)
	                    {
	                        if (ndb_getDebugLevel() >= 7)
	                        {
	                            fprintf(stdout, "ndb_list_addToFileList: changing file action from '%c' to '%c' for '%s'\n",
	                                            pCurrent->action, pFile->action, pFile->filenameExt);
	                            fflush(stdout);
	                        }
	                        pCurrent->action = pFile->action;
	                    }
						break;
					}
				}
				pCurrent = pCurrent->nextFile;
			}
#ifdef USE_HASHTABLE_CRC_FILENAMES
}
else
{
#endif
		    crc = ndb_crc32(0L, pFile->filenameUTF8, (ULONG) pFile->lenfilenameUTF8);
		    /* get lower 16 bits of CRC32, use it as index */
		    lCurrCRC = (ULONG) crc & (NDB_SIZE_FILESTABLE - 1);
		    // check index for violation of bounds
		    if (lCurrCRC >= NDB_SIZE_FILESTABLE)
		    {
		        fprintf(stderr, "ndb_file_filefound: CRC index %lX out of range 0..%lX\n", lCurrCRC, (ULONG) NDB_SIZE_FILESTABLE);
		        fflush(stderr);
		        exit(NDB_MSG_NOTOK);
		   	}
		    pCurrent = pFileList->ppHashtableFiles[lCurrCRC];
		    bFileFound = 1;

			if (pCurrent == NULL)
			{
			    bFileFound = 0;
			}
			else
			{
				if (strcmp(pFile->filenameUTF8, pCurrent->filenameUTF8) != 0)
				{
				    bFileFound = 0;
					pCurrent = pCurrent->sameCRC;
				    while (pCurrent != NULL)
				    {
						if (strcmp(pFile->filenameUTF8, pCurrent->filenameUTF8) == 0)
						{
							bFileFound = 1;
							break;
						}
						pCurrent = pCurrent->sameCRC;
				    }
				}
				if (bFileFound == 1)
				{
				    if (ndb_getDebugLevel() >= 5)
				    {
				        fprintf(stdout, "ndb_list_addToFileList: file '%s' already in file list\n",
				                        pFile->filenameExt);
					    fflush(stdout);
				    }
                    // check action flag
                    if (pFile->action != pCurrent->action)
                    {
                        if (ndb_getDebugLevel() >= 7)
                        {
                            fprintf(stdout, "ndb_list_addToFileList: changing file action from '%c' to '%c' for '%s'\n",
                                            pCurrent->action, pFile->action, pFile->filenameExt);
                            fflush(stdout);
                        }
                        pCurrent->action = pFile->action;
                    }
			    }
		    }
#ifdef USE_HASHTABLE_CRC_FILENAMES
}
#endif

            if (! bFileFound)
            {
	            pFileList->pLastFile->nextFile = pFile;
    	        pFileList->pLastFile = pFile;
        	    pFileList->fileCount++;
			}
        }
    }

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
//      printf("ndb_list_addToFileList: curr_time  = %llu\n", curr_time);
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_list_addToFileList: count %lu, average %lu, max %lu, sum %lu\n",
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);
        ndb_util_set_performancetest(TEST_PERFORMANCE_ADDTOFILELIST, s);
    }
#endif

    if ((ndb_getDebugLevel() >= 7) && (! bFileFound))
        fprintf(stdout, "ndb_list_addToFileList: added new file entry as list member no %lu\n",
                        pFileList->fileCount);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_addToFileList - finished\n");
    fflush(stdout);
}


/*
    *************************************************************************************
    ndb_list_addFileToHash()

	adds a new filename (given in pFile) to the filename hash table of current list
	the hash gets construced by computing the crc32 of the filename

    struct ndb_l_filelist *pFileList:      (IN)       pointer to chapter structure
    struct ndb_s_fileentry *pFile:         (IN)       pointer to file entry
    *************************************************************************************
*/
int ndb_list_addFileToHash(struct ndb_l_filelist *pFileList, struct ndb_s_fileentry *pFile)
{
#if defined(USE_HASHTABLE_CRC_FILENAMES)

    ULONG crc = 0;
    ULONG lCurrCRC = 0;
    PFILEENTRY pTempFile;


    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_list_addFileToHash - startup\n");
        fflush(stdout);
    }

    // check preconditions
    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_addFileToHash: cannot add file entry to hashtable of a NULL file list\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_list_addFileToHash: cannot add NULL file entry to hashtable\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }
    if (pFileList->ppHashtableFiles == NULL)
    {
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_list_addFileToHash: hashtable was defined as NULL\n");
            fflush(stdout);
        }
        return NDB_MSG_OK;
    }

	// add file to hashtable of filenames
    crc = ndb_crc32(0L, pFile->filenameUTF8, (ULONG) pFile->lenfilenameUTF8);
    /* get lower 16 bits of CRC32, use it as index */
    lCurrCRC = (ULONG) crc & (NDB_SIZE_FILESTABLE - 1);

	pTempFile = pFileList->ppHashtableFiles[lCurrCRC];
    if (pTempFile == NULL)
    {
        // first filename with this CRC
        pFileList->ppHashtableFiles[lCurrCRC] = pFile;
    }
    else
    {
        // other filenames with same CRC before
		if (strcmp(pFile->filenameUTF8, pTempFile->filenameUTF8) == 0)
		{
		    if (ndb_getDebugLevel() >= 5)
		    {
		        fprintf(stdout, "ndb_list_addFileToHash: file '%s' already in chapter list\n", pFile->filenameUTF8);
		        fflush(stdout);
		    }
			return NDB_MSG_DUPLICATEFILENAME;
		}
        while (pTempFile->sameCRC != NULL)
        {
			pTempFile = pTempFile->sameCRC;
			if (strcmp(pFile->filenameUTF8, pTempFile->filenameUTF8) == 0)
			{
			    if (ndb_getDebugLevel() >= 5)
			    {
			        fprintf(stdout, "ndb_list_addFileToHash: file '%s' already in chapter list\n", pFile->filenameUTF8);
			        fflush(stdout);
			    }
				return NDB_MSG_DUPLICATEFILENAME;
			}
        }
        pTempFile->sameCRC = pFile;
    }

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_list_addFileToHash - finished\n");
        fflush(stdout);
    }
#else
    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_list_addFileToHash: hashtable disabled in this build\n");
    fflush(stdout);
#endif
    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_list_removeFileFromHash()

	removes a given pFile from the filename hash table
	of file hash table of pFileList
	the hash gets construced by computing the crc32 of its filenameUTF8

    struct ndb_l_filelist *pFileList:      (IN)       pointer to file list structure
    struct ndb_s_fileentry *pFile:         (IN)       pointer to file entry
    *************************************************************************************
*/
int ndb_list_removeFileFromHash(struct ndb_l_filelist *pFileList, PFILEENTRY pFile)
{
#if defined(USE_HASHTABLE_CRC_FILENAMES)

    ULONG crc = 0;
    ULONG lCurrCRC = 0;
    PFILEENTRY pTempFile;
    PFILEENTRY pTempOld;


    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_list_removeFileFromHash - startup\n");
    }

    // check preconditions
    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_removeFileFromHash: cannot remove file entry from hashtable of a NULL chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }
    if (pFile == NULL)
    {
        fprintf(stdout, "ndb_list_removeFileFromHash: cannot remove NULL file entry from hashtable of chapter\n");
        fflush(stdout);
        return NDB_MSG_NULLERROR;
    }
    if (pFileList->ppHashtableFiles == NULL)
    {
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_list_removeFileFromHash: hashtable was defined as NULL\n");
            fflush(stdout);
        }
        return NDB_MSG_OK;
    }

	// remove file from hashtable of filenames
    crc = ndb_crc32(0L, pFile->filenameUTF8, (ULONG) pFile->lenfilenameUTF8);
    /* get lower 16 bits of CRC32, use it as index */
    lCurrCRC = (ULONG) crc & (NDB_SIZE_FILESTABLE - 1);

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_list_removeFileFromHash: looking for file '%s' at crc 0x%04lX\n", pFile->filenameUTF8, lCurrCRC);
        fflush(stdout);
    }

    pTempFile = pFileList->ppHashtableFiles[lCurrCRC];
    if (pTempFile == NULL)
    {
        if (ndb_getDebugLevel() >= 7)
        {
        fprintf(stdout, "ndb_list_removeFileFromHash: no list for crc 0x%04lX -> file not found\n", lCurrCRC);
            fprintf(stdout, "ndb_list_removeFileFromHash - finished\n");
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
                fprintf(stdout, "ndb_list_removeFileFromHash: checking hash file '%s'\n", pTempFile->filenameUTF8);
                fflush(stdout);
            }
            // other filenames with same CRC before
            if (strcmp(pFile->filenameUTF8, pTempFile->filenameUTF8) == 0)
            {
                if (ndb_getDebugLevel() >= 7)
                {
                    fprintf(stdout, "ndb_list_removeFileFromHash: file '%s' found in filelist hashtable\n", pFile->filenameUTF8);
                    fflush(stdout);
                }
                if (pTempFile == pFileList->ppHashtableFiles[lCurrCRC])
                {
                    pFileList->ppHashtableFiles[lCurrCRC] = pTempFile->sameCRC;
                    pTempFile->sameCRC = NULL;
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "ndb_list_removeFileFromHash: file '%s' removed as first entry of crc 0x%04lX\n", pFile->filenameUTF8, lCurrCRC);
                        fprintf(stdout, "ndb_list_removeFileFromHash - finished\n");
                        fflush(stdout);
                    }
                    return NDB_MSG_OK;
                }
                else
                {
                    // remove file from linklist
                    pTempOld->sameCRC = pTempFile->sameCRC;
                    pTempFile->sameCRC = NULL;
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "ndb_list_removeFileFromHash: file '%s' removed as second or later entry of crc 0x%04lX\n", pFile->filenameUTF8, lCurrCRC);
                        fprintf(stdout, "ndb_list_removeFileFromHash - finished\n");
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
            fprintf(stdout, "ndb_list_removeFileFromHash: file '%s' not found in crc 0x%04lX hashtable\n", pFile->filenameUTF8, lCurrCRC);
            fprintf(stdout, "ndb_list_removeFileFromHash - finished\n");
            fflush(stdout);
    }
    return NDB_MSG_NOTFOUND;
#else
    if (ndb_getDebugLevel() >= 5)
    	fprintf(stdout, "ndb_list_removeFileFromHash: hashtable disabled in this build\n");
    fflush(stdout);
    return NDB_MSG_OK;
#endif
}

/*
    *************************************************************************************
	ndb_list_delFromFileList()

	deletes a file entry from the file list (check for same file entry pointer)
    *************************************************************************************
*/

void ndb_list_delFromFileList(struct ndb_l_filelist *pFileList, struct ndb_s_fileentry *pFile)
{
    struct ndb_s_fileentry *pCurrent;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_delFromFileList - startup\n");
    fflush(stdout);

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_delFromFileList: Error: cannot work with a null pointer\n");
        fflush(stdout);
    }
    else
    {
        pCurrent = ndb_list_getFirstFile(pFileList);
        while (pCurrent != NULL)
        {
            if (pCurrent == pFile)
            {
                pCurrent->nextFile = pFile->nextFile;
                pFileList->fileCount--;
                if (ndb_getDebugLevel() >= 7)
                {
	                fprintf(stdout, "ndb_list_delFromFileList: deleted file from list, file count now %lu\n",
                                    pFileList->fileCount);
                    fflush(stdout);
                }
                // break;
            }
        }
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_delFromFileList - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
	ndb_list_removeFirstElement()

	removes (and returns) the first file entry from the file list
    *************************************************************************************
*/

PFILEENTRY ndb_list_removeFirstElement(struct ndb_l_filelist *pFileList)
{
    struct ndb_s_fileentry *pFirst = NULL;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_RemoveFirstElement - startup\n");
    fflush(stdout);

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_RemoveFirstElement: Error: cannot work with a null pointer\n");
        fflush(stdout);
    }
    else
    {
        pFirst = ndb_list_getFirstFile(pFileList);
        if (pFirst != NULL)
        {
            // remove file from hash table
            ndb_list_removeFileFromHash(pFileList, pFirst);
            // remove first file from list
            pFileList->pFirstFile = pFirst->nextFile;
            pFileList->fileCount--;
        }
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_RemoveFirstElement - finished\n");
    fflush(stdout);

    return pFirst;
}

/*
    *************************************************************************************
	ndb_list_mixToFileList()

	add the new file entry to the file list; if the file already is in the list
	(check uses filenameUTF8), set the actioncode to NDB_ACTION_KEEP (= this file
	is already archived in this chapter, therefore do nothing new with this file)
    *************************************************************************************
*/

void ndb_list_mixToFileList(struct ndb_l_filelist *pFileList, struct ndb_s_fileentry *pFile)
{
    struct ndb_s_fileentry *pCurrent;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_mixToFileList - startup\n");

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_mixToFileList: Error: cannot work with a null pointer\n");
    }
    else
    {
        pCurrent = ndb_list_getFirstFile(pFileList);
        while (pCurrent != NULL)
        {
			// small optimization: compare filenames only if same length
			if (pFile->lenfilenameUTF8 == pCurrent->lenfilenameUTF8)
			{
	            if (strcmp(pFile->filenameUTF8, pCurrent->filenameUTF8) == 0)
	            {
	                if (ndb_getDebugLevel() >= 7)
	                {
	                    fprintf(stdout, "ndb_list_mixToFileList: file '%s' already in file list\n",
	                            pFile->filenameExt);
                        fflush(stdout);
	                }
	                pCurrent->action = NDB_ACTION_KEEP;
	                break;
	            }
	        }
            pCurrent = ndb_list_getNextFile(pFileList);
        }
        if (pCurrent == NULL)
        {
            ndb_list_addToFileList(pFileList, pFile);
        }
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_mixToFileList - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
	ndb_list_excludeFileMask()

	"deletes" the file entry from the file list; the file entry gets *not*
	removed from the list, but his actioncode is set to NDB_ACTION_IGNORE
	(= skip this entry while processing the list)
	check uses filenameExt, because the exclude filemask is not converted to UTF8
    *************************************************************************************
*/

void ndb_list_excludeFileMask(struct ndb_l_filelist *pFileList, const char *sfile)
{
    struct ndb_s_fileentry *pFile = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_excludeFileMask - startup\n");
    fflush(stdout);

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_excludeFileMask: Error: cannot work with a null pointer\n");
        fflush(stdout);
        return;
    }

    pFile = ndb_list_getFirstFile(pFileList);
    while (pFile != NULL)
    {
	    if (ndb_getDebugLevel() >= 8)
	    {
    	    fprintf(stdout, "ndb_list_excludeFileMask: checking '%s' against mask '%s'\n",
    	                    pFile->filenameExt, sfile);
            fflush(stdout);
        }
        if (ndb_osdep_matchFilename(sfile, pFile->filenameExt) == 1)
        {
            pFile->action = NDB_ACTION_IGNORE;
		    if (ndb_getDebugLevel() >= 8)
		    {
    		    fprintf(stdout, "ndb_list_excludeFileMask: --> ignore '%s'\n", pFile->filenameExt);
                fflush(stdout);
    		}
        }
        pFile = ndb_list_getNextFile(pFileList);
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_excludeFileMask - finished\n");
    fflush(stdout);

    return;
}

/*
    *************************************************************************************
	ndb_list_getFileCount()

	returns the number of all added file entries (no matter which actioncode!)
    *************************************************************************************
*/

ULONG ndb_list_getFileCount(struct ndb_l_filelist *pFileList)
{
    ULONG count = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileCount - startup\n");
    fflush(stdout);

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_getFileCount: Error: cannot work with a null pointer\n");
        fflush(stdout);
    }
    else
    {
        count = pFileList->fileCount;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileCount - finished\n");
    fflush(stdout);

    return count;
}

/*
    *************************************************************************************
	ndb_list_getFirstFile()

	return the first file entry structure (= NULL, if the list is empty)
    *************************************************************************************
*/

struct ndb_s_fileentry *ndb_list_getFirstFile(struct ndb_l_filelist *pFileList)
{
    struct ndb_s_fileentry *pFirst = NULL;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_getFirstFile - startup\n");

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_getFirstFile: Error: cannot work with a null pointer\n");
    }
    else
    {
        pFileList->pCurrFile = pFileList->pFirstFile;
        pFirst = pFileList->pCurrFile;
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_getFirstFile - finished\n");
    fflush(stdout);

    return pFirst;
}

/*
    *************************************************************************************
	ndb_list_getNextFile()

	returns the next file netry (= NULL; if the previous entry was the last)
    *************************************************************************************
*/

struct ndb_s_fileentry *ndb_list_getNextFile(struct ndb_l_filelist *pFileList)
{
    struct ndb_s_fileentry *pFile = NULL;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_getNextFile - startup\n");

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_getNextFile: Error: cannot work with a null pointer\n");
    }
    else
    {
        if (pFileList->pCurrFile != NULL)
        {
             pFileList->pCurrFile = pFileList->pCurrFile->nextFile;
        }
        pFile = pFileList->pCurrFile;
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_getNextFile - finished\n");
    fflush(stdout);

    return pFile;
}

/*
    *************************************************************************************
	ndb_list_getLastFile()

	returns the last file entry (= NULL, if the list is empty)
    *************************************************************************************
*/

struct ndb_s_fileentry *ndb_list_getLastFile(struct ndb_l_filelist *pFileList)
{
    struct ndb_s_fileentry *pLast = NULL;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_getLastFile - startup\n");

    if (pFileList == NULL)
    {
        fprintf(stdout, "ndb_list_getLastFile: Error: cannot work with a null pointer\n");
    }
    else
    {
        pFileList->pCurrFile = pFileList->pLastFile;
        pLast = pFileList->pCurrFile;
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_getLastFile - finished\n");
    fflush(stdout);

    return pLast;
}


/* ======================================================================== */
/* creating and using a file mask list                                      */
/* ======================================================================== */

/* definition lokated at ndb.h
struct ndb_l_filemasklist
{
    USHORT maskCount;   // number of added file masks
    char *pMask;        // pointer to char array
	char mode[NDB_MAXLEN_FILEMASK];   // *i*nclude or e*x*clude mode
};
*/

/*
    *************************************************************************************
	ndb_list_initFileMaskList()

	inits the file mask list and callocates the buffer for
	NDB_MAXCOUNT_FILEMASK * NDB_MAXLEN_FILEMASK bytes
    *************************************************************************************
*/

void ndb_list_initFileMaskList(struct ndb_l_filemasklist *pFileMaskList)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_initFileMaskList - startup\n");

    if (pFileMaskList == NULL)
    {
        fprintf(stdout, "ndb_list_initFileMaskList: Error: couldn't init a null pointer\n");
    }
    else
    {
        pFileMaskList->pMask       = (char *) calloc(NDB_MAXCOUNT_FILEMASK, NDB_MAXLEN_FILEMASK);
        pFileMaskList->maskCount   = 0;
        if (pFileMaskList->pMask == NULL)
        {
            fprintf(stderr, "ndb_list_initFileMaskList: Error: couldn't allocate memory for file mask list\n");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
    }
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_initFileMaskList - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
	ndb_list_addToFileMaskList()

	adds a character string to the file mask array
    *************************************************************************************
*/

void ndb_list_addToFileMaskList(struct ndb_l_filemasklist *pFileMaskList, char *mask, char fmode)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_addToFileMaskList - startup\n");

    if (NDB_MAXCOUNT_FILEMASK == pFileMaskList->maskCount)
    {
        fprintf(stdout, "\nWarning: Maximum of %d filemasks reached. Ignoring '%s'\n",
                NDB_MAXCOUNT_FILEMASK, mask);
        fflush(stdout);
        return;
    }

    if (pFileMaskList == NULL)
    {
        fprintf(stdout, "ndb_list_addToFileMaskList: Error: cannot work with a null pointer\n");
    }
    else
    {
		// set mode: include filemask or exclude filemode
        (pFileMaskList->mode)[pFileMaskList->maskCount] = fmode;
        strncpy(&pFileMaskList->pMask[NDB_MAXLEN_FILEMASK * pFileMaskList->maskCount],
                mask, NDB_MAXLEN_FILEMASK);
        pFileMaskList->maskCount++;
    }
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_list_addToFileMaskList: added new mask '%s' as list member no %d\n",
                        mask, pFileMaskList->maskCount - 1);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_addToFileMaskList - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
	ndb_list_getFileMaskCount()

	returns the number of file masks in the array
    *************************************************************************************
*/

USHORT ndb_list_getFileMaskCount(struct ndb_l_filemasklist *pFileMaskList)
{
    USHORT count = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileMaskCount - startup\n");

    if (pFileMaskList == NULL)
    {
        fprintf(stdout, "ndb_list_getFileMaskCount: Error: cannot work with a null pointer\n");
    }
    else
        count = pFileMaskList->maskCount;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileMaskCount - finished\n");
    fflush(stdout);

    return count;
}

/*
    *************************************************************************************
	ndb_list_getFileMask()

	returns the file mask with index <index>
    *************************************************************************************
*/

char *ndb_list_getFileMask(struct ndb_l_filemasklist *pFileMaskList, USHORT index)
{
    char *s = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileMask - startup\n");

    if (pFileMaskList == NULL)
    {
        fprintf(stdout, "ndb_list_getFileMask: Error: cannot work with a null pointer\n");
    }
    else if (index > (NDB_MAXCOUNT_FILEMASK - 1))
    {
        fprintf(stdout, "ndb_list_getFileMask: Error: index (%u) out of range (0..%d)\n",
                        index, NDB_MAXCOUNT_FILEMASK - 1);
        fflush(stdout);
    }
    else
    {
        s = &pFileMaskList->pMask[NDB_MAXLEN_FILEMASK * index];
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileMask - finished\n");
    fflush(stdout);

    return s;
}

/*
    *************************************************************************************
	ndb_list_getFileMaskMode()

	returns the file mask mode (include/exclude) with index <index>
    *************************************************************************************
*/

char ndb_list_getFileMaskMode(struct ndb_l_filemasklist *pFileMaskList, USHORT index)
{
    char c = '\0';

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileMask - startup\n");

    if (pFileMaskList == NULL)
    {
        fprintf(stdout, "ndb_list_getFileMask: Error: cannot work with a null pointer\n");
    }
    else if (index > (NDB_MAXCOUNT_FILEMASK - 1))
    {
        fprintf(stdout, "ndb_list_getFileMask: Error: index (%u) out of range (0..%d)\n",
                        index, NDB_MAXCOUNT_FILEMASK - 1);
        fflush(stdout);
    }
    else
    {
        c = pFileMaskList->mode[index];
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_list_getFileMask - finished\n");
    fflush(stdout);

    return c;
}

/*
    *************************************************************************************
	ndb_list_checkWithFileMask()

	checks a given file entry (filenameExt, because file masks are not converted
	to UTF8) with all file masks (file mask mode tells whether to include or exclude)

	TODO: falls Filemaske ohne Pfadanteil, aber File mit Pfad, dann Filemaske mit
	      pfad wildcard ergänzen
    *************************************************************************************
*/

void ndb_list_checkWithFileMask(struct ndb_s_fileentry *pFile,
                                struct ndb_l_filemasklist *pMasks)
{
    char *s = NULL;
    char c = '\0';
    USHORT i1;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_checkWithFileMask - startup\n");

    if (pMasks != NULL)
    {
        // check whether the file should be included or excluded
        for (i1 = 0; i1 < ndb_list_getFileMaskCount(pMasks); i1++)
        {
            s = ndb_list_getFileMask(pMasks, i1);
            c = ndb_list_getFileMaskMode(pMasks, i1);
            if (ndb_getDebugLevel() >= 8)
            {
                fprintf(stdout, "ndb_list_checkWithFileMask: checking '%s' with include mask '%s' [%d:%c]\n",
                                pFile->filenameExt, s, i1, c);
                fflush(stdout);
            }
            if (ndb_osdep_matchFilename(s, pFile->filenameExt) == 1)
            {
				if (c == NDB_MODE_INCLUDE)
	                pFile->action = NDB_ACTION_EXTRACT;
	            else if (c == NDB_MODE_EXCLUDE)
	                pFile->action = NDB_ACTION_IGNORE;
	            else
	            {
					fprintf(stderr, "ndb_list_checkWithFileMask: unknown file mask mode %c\n", c),
                    fflush(stderr);
	            }
            }
        }
    }

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_list_checkWithFileMask: result of check of '%s': '%s'\n",
                        pFile->filenameExt,
                        (pFile->action == NDB_ACTION_EXTRACT) ? "extract" : "ignore");
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_list_checkWithFileMask - finished\n");
    fflush(stdout);

    return;
}
