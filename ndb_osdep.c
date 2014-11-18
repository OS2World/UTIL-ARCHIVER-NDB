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
#include <fcntl.h>


#include "ndb.h"
#include "ndb_prototypes.h"

#if defined(__OS2__)
#define OS2
#endif

#if defined (OS2)

#ifdef __WATCOMC__
APIRET APIENTRY DosTmrQueryFreq(PULONG);
APIRET APIENTRY DosTmrQueryTime(PQWORD);
#endif

#ifndef __WATCOMC__
typedef struct _QWORD
{
  ULONG ulLo;
  ULONG ulHi;
} QWORD;
typedef QWORD *PQWORD;

ULONG DosTmrQueryFreq (PULONG pulTmrFreq);
ULONG DosTmrQueryTime (PQWORD pqwTmrTime);
#endif

#endif




/* ======================================================================== */
/* OS dependend functions                                                   */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_osdep_init()

        inits all os dependent functions
    *************************************************************************************
*/

void ndb_osdep_init()
{
#if defined (WIN32)
    ndb_win32_init_upper();
    ndb_win32_init();
#elif defined(OS2)
    ndb_os2_init_upper();
#else
    ;
#endif
}


/*
    *************************************************************************************
        ndb_osdep_done()

         clears resources
    *************************************************************************************
*/

void ndb_osdep_done()
{
#if defined (WIN32)
    ndb_win32_done();
#else
    ;
#endif
}


/*
    *************************************************************************************
        ndb_osdep_split()

        splits a filemask into the longest directory part without wildcard <pDir>
        and the rest (wildcard path and filename itself) <pFile>
    *************************************************************************************
*/

void ndb_osdep_split(const char *s_files, char *pDir, char *pFile)
{
#if defined (WIN32)
    ndb_win32_split(s_files, pDir, pFile);
#elif defined(OS2)
    ndb_os2_split(s_files, pDir, pFile);
#elif defined(UNIX)
    ndb_unix_split(s_files, pDir, pFile);
#else
    fprintf(stderr, "ndb_osdep_split: no generic implementation\n");
    exit(NDB_MSG_NOTOK);
#endif
}

/*
    *************************************************************************************
        ndb_osdep_makeFileList()

        scans the harddisk for files matching <wildcard> starting at path <dir>

        Returns: number of files to archive within the directory <dir> (including
        subdirectories)
    *************************************************************************************
*/

void ndb_osdep_makeFileList(char *dir, char *wildcard, int doFlags, struct ndb_l_filelist *pFileList, struct ndb_l_filemasklist *pFileListExcludes, ULONG *pcount)
{
#if defined (WIN32)
    ndb_win32_makeFileList(dir, wildcard, doFlags, pFileList, pFileListExcludes, pcount);
#elif defined(OS2)
    ndb_os2_makeFileList(dir, wildcard, doFlags, pFileList, pFileListExcludes, pcount);
#elif defined(UNIX)
    ndb_unix_makeFileList(dir, wildcard, doFlags, pFileList, pFileListExcludes, pcount);
#else
    fprintf(stderr, "ndb_osdep_makeFileList: no generic implementation\n");
    exit(NDB_MSG_NOTOK);
#endif
}

/*
    *************************************************************************************
        ndb_osdep_addFileToFileList()

        creates a fileentry structure for the file with name <pathfile> and file attributes
        <ulAttr>;
        also adds the entry to the filemask list <pFileList> and its hashtable;
        if a valid <ppNewFile> is given als argument it will point to the new fileentry
        structure
    *************************************************************************************
*/

void ndb_osdep_addFileToFileList(struct ndb_s_fileentry **ppNewFile, char *pathfile, ULONG ulAttr, struct ndb_l_filelist *pFileList)
{
    int len_filename;
    struct ndb_s_fileentry *pNewFile = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_addFileToFileList - startup\n");
    fflush(stdout);

    pNewFile = (struct ndb_s_fileentry *) ndb_calloc (sizeof(struct ndb_s_fileentry), 1, "ndb_osdep_addFileToFileList");
    if ((pNewFile ) == NULL)
    {
        fprintf(stderr, "ndb_osdep_addFileToFileList: Error: couldn't allocate memory for new file in file list\n");
        fflush(stderr);
        exit (NDB_MSG_NOMEMORY);
    }
    // set some values
    len_filename = strlen (pathfile);
    if (len_filename > NDB_MAXLEN_FILENAME)
        len_filename = NDB_MAXLEN_FILENAME;
    pNewFile->filenameExt = ndb_calloc(len_filename + 1, 1, "ndb_osdep_addFileToFileList: filenameExt");
    if (pNewFile->filenameExt == NULL)
    {
        fprintf(stderr, "ndb_osdep_addFileToFileList: couldn't allocate memory for filename\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }
    strncpy (pNewFile->filenameExt, pathfile, len_filename);
    pNewFile->filenameExt[len_filename] = '\0';
    // create UTF8 filename
    ndb_fe_makeFilenameUTF8(pNewFile);
    // set other values
    pNewFile->attributes = ulAttr;
    pNewFile->nextFile = NULL;
    pNewFile->children = 0;
    pNewFile->action = NDB_ACTION_ADD;
    ndb_list_addToFileList(pFileList, pNewFile);
    ndb_list_addFileToHash(pFileList, pNewFile);

    if (ppNewFile != NULL)
        *ppNewFile = pNewFile;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_addFileToFileList - finished\n");
    fflush(stdout);
}


/*
    *************************************************************************************
        ndb_osdep_createPath()

        creates a directory <sDefaultPath + pFile->filenameExt> and its parent directories
        if neccessary
    *************************************************************************************
*/

int ndb_osdep_createPath(struct ndb_s_fileentry *pFile, const char *sDefaultPath)
{
    char sInt[NDB_MAXLEN_FILENAME];
    char *p;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_createPath - startup\n");
    fflush(stdout);

    if (pFile == NULL)
    {
        strcpy(sInt, sDefaultPath);
    }
    else
    {
        if (strcmp(sDefaultPath, ".") == 0)
        {
            strcpy(sInt, pFile->filenameExt);
        }
        else
        {
            snprintf(sInt, NDB_MAXLEN_FILENAME - 1, "%s/%s", sDefaultPath, pFile->filenameExt);
        }
    }

        // check for file entry type - dir or file?
        if ((pFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
        {
            // we have a directory - check for trailing '/'
            if (sInt[strlen(sInt) - 1] != '/')
                strcat (sInt, "/");
        }
        else
        {
            // we have a file - remove the filename
            p = strrchr(sInt, '/');
            if (p == NULL) p = strrchr(sInt, '\\');
            //
            if (p != NULL)
            {
                *(++p) = '\0';
            }
            else
            {
                // no dir found, only filename
                // -> nothing to do
                    if (ndb_getDebugLevel() >= 7)
                        fprintf(stdout, "ndb_osdep_createPath: no path in '%s'\n", sInt);
                    fflush(stdout);
                sInt[0] = '\0';
                return NDB_MSG_OK;
            }
        }
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_osdep_createPath: path to create is '%s'\n", sInt);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_createPath - jumping to ndb OS specific function\n");
    fflush(stdout);

#if defined (WIN32)
    return ndb_win32_create_path(sInt);
#elif defined(OS2)
    return ndb_os2_create_path(sInt);
#elif defined(UNIX)
    return ndb_unix_create_path(sInt);
#else
    fprintf(stderr, "ndb_osdep_create_path: no generic implementation\n");
    exit(NDB_MSG_NOTOK);
#endif
}


/*
    *************************************************************************************
        ndb_osdep_createFile()

        create the file <sDefaultPath + pFile->filenameExt> on harddisk

        Returns: file handle of created file
    *************************************************************************************
*/

#if defined(USE_FHANDLE_INT)
int ndb_osdep_createFile(struct ndb_s_fileentry *pFile, const char *sDefaultPath, int *file)
#else
int ndb_osdep_createFile(struct ndb_s_fileentry *pFile, const char *sDefaultPath, FILE **file)
#endif
{
#if defined(USE_FHANDLE_INT)
    int fileOut;
#else
    FILE *fileOut = NULL;
#endif
    char sInt[NDB_MAXLEN_FILENAME];
    char sExt[NDB_MAXLEN_FILENAME];
    ULONG retval;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_createFile - startup\n");

    if (strcmp(sDefaultPath, ".") == 0)
    {
        strcpy(sInt, pFile->filenameExt);
    }
    else
    {
        snprintf(sInt, NDB_MAXLEN_FILENAME - 1, "%s/%s", sDefaultPath, pFile->filenameExt);
    }

    strcpy (sExt, ndb_osdep_makeExtFileName(sInt));

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_createFile: trying to create '%s'\n", sExt);
    fflush(stdout);

#if defined(USE_FHANDLE_INT)
#if defined(UNIX)
    fileOut = open(sExt, O_WRONLY  | O_CREAT | O_TRUNC            | O_LARGEFILE);
#else
    fileOut = open(sExt, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
#endif
    *file = fileOut;
    if (fileOut == -1)
#else
    fileOut = fopen(sExt, "wb");
    *file = fileOut;
    if (fileOut == NULL)
#endif

    {
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_osdep_createFile: Error: cannot create file '%s'\n", sExt);
        fflush(stdout);
        retval = NDB_MSG_NOFILEOPEN;
    }
    else
    {
        retval = NDB_MSG_OK;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_createFile - finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
        ndb_osdep_truncate()

        truncate file at given size
    *************************************************************************************
*/

void ndb_osdep_truncate(const char *pathfile, ULONG fileSize)
{
#if defined (WIN32)
    ndb_win32_truncate(pathfile, fileSize);
#elif defined(OS2)
    ndb_os2_truncate(pathfile, fileSize);
#elif defined(UNIX)
    ndb_unix_truncate(pathfile, fileSize);
#else
    fprintf(stderr, "ndb_osdep_truncate: no generic implementation\n");
    exit(NDB_MSG_NOTOK);
#endif
}



/*
    *************************************************************************************
        ndb_osdep_setMetaData()

        sets additional fileattributes if neccessary
    *************************************************************************************
*/

int ndb_osdep_setMetaData(struct ndb_s_fileentry *pFile, const char *sDefaultPath, FILE *fArchive,
                          int opt_testintegrity, int opt_ignoreextra, char *sText)
{
        int retval = NDB_MSG_OK;
#if defined (WIN32)
    ndb_win32_SetPathAttrTimes(pFile, sDefaultPath);
    if (opt_ignoreextra == 0)
            retval = ndb_osdep_setExtraData(pFile, sDefaultPath, fArchive, opt_testintegrity, sText);
#elif defined(OS2)
    if (opt_ignoreextra == 0)
    {
            retval = ndb_osdep_setExtraData(pFile, sDefaultPath, fArchive, opt_testintegrity, sText);
    }
    ndb_os2_SetPathAttrTimes(pFile, sDefaultPath);
#elif defined(UNIX)
    // set 'DOS' like file attributes
    ndb_unix_setPathAttr(pFile, sDefaultPath);
    if (opt_ignoreextra == 0)
            retval = ndb_osdep_setExtraData(pFile, sDefaultPath, fArchive, opt_testintegrity, sText);
#else
    fprintf(stderr, "ndb_osdep_setMetaData: no generic implementation\n");
    exit(NDB_MSG_NOTOK);
#endif
        return retval;
}

/*
    *************************************************************************************
        ndb_osdep_setExtraData()

        loop through all extra header
        - create a well sized buffer and copy all extra data in it
        - call the specific function to set the extra data
    *************************************************************************************
*/

int ndb_osdep_setExtraData(struct ndb_s_fileentry *pFile, const char *sDefaultPath, FILE *fArchive, int opt_testintegrity, char *sText)
{
    int retval = NDB_MSG_OK;
    char newFilename[NDB_MAXLEN_FILENAME] = "";
    struct ndb_s_fileextradataheader *pExtraHea = NULL;
#if defined (WIN32)
        int len;
        int count;
        char sDummy[NDB_MAXLEN_FILENAME];
#endif

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_setExtraData - startup\n");
    fflush(stdout);

    // nothing to do?
    if (pFile->nr_ofExtraHea == 0)
    {
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "ndb_osdep_setExtraData - finished (nothing to do)\n");
            fflush(stdout);
        return NDB_MSG_OK;
    }

    // check preconditions
    if (pFile->lenExtHea == 0)
    {
        fprintf(stderr, "ndb_osdep_setExtraData: internal error for '%s'\n", pFile->filenameExt);
        fprintf(stderr, "                        length of %d extra data header is zero\n", pFile->nr_ofExtraHea);
        fflush(stderr);
        return NDB_MSG_NOTOK;
    }

    if ((strcmp(sDefaultPath, ".") == 0) || (sDefaultPath[0] == '\0'))
        strcpy(newFilename, pFile->filenameExt);
    else
        snprintf(newFilename, NDB_MAXLEN_FILENAME - 1, "%s/%s", sDefaultPath, pFile->filenameExt);
    strcpy (newFilename, ndb_osdep_makeExtFileName(newFilename));

        // proceed through all extra header
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_setExtraData: file '%s' has %d extra header\n", newFilename, pFile->nr_ofExtraHea);
    fflush(stdout);
        pExtraHea = pFile->pExtraHeader;
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_setExtraData: first extra header at 0x%lX\n", (ULONG) pExtraHea);
    fflush(stdout);

        // loop over all extra headers -----------------------------------------------------------
        while (pExtraHea != NULL)
        {
                // if cascade: check all extra header types and all OSses
                if (pExtraHea->typExtHea == NDB_EF_OS2EA) // -----------------------------------------
                {
#if defined (OS2)
                        // set EAs
                        retval = ndb_os2_setEAs(pExtraHea, newFilename, fArchive, opt_testintegrity);
                        if (retval != NDB_MSG_OK)
                        {
                            strcat(sText, ", EAs failed");
                        fprintf(stdout, "ndb_osdep_setExtraData: error setting OS/2 EAs\n");
                            fflush(stdout);
                        }
                        else
                        {
                            strcat(sText, ", EAs ok");
                        }
#else
                    if (ndb_getDebugLevel() >= 2)
                    {
                        fprintf(stdout, "ndb_osdep_setExtraData: extra data type %d (OS/2 EAs) not supported by current OS/2\n", pExtraHea->typExtHea);
                        fflush(stdout);
                    }
#endif
                }
                else if (pExtraHea->typExtHea == NDB_EF_WIN83) // -----------------------------------------
                {
#if defined (WIN32)
                        // set Win 8.3 short name (WinXP only!)
                        count = 0;
                        len = ndb_readbuf_ushort(pExtraHea->pData, &count);
                        strncpy(sDummy, &pExtraHea->pData[2], len);
                        sDummy[len] = '\0';
//                      snprintf(sDummy, len, "%s%c", &pExtraHea->pData[2], '\0');
                        retval = ndb_win32_setShortName(newFilename, sDummy);
                        if (retval != NDB_MSG_OK)
                        {
                            strcat(sText, ", 8.3 failed");
                        }
                        else
                        {
                            strcat(sText, ", 8.3 ok");
                        }
#else
                    if (ndb_getDebugLevel() >= 2)
                    {
                        fprintf(stdout, "ndb_osdep_setExtraData: extra data type %d (Win 8.3 Shortname) not supported by current OS/2\n", pExtraHea->typExtHea);
                        fflush(stdout);
                    }
#endif
                }
                else if (pExtraHea->typExtHea == NDB_EF_NTSD) // -----------------------------------------
                {
#if defined (WIN32)
                        // set NT security
                        retval = ndb_win32_setSD(pFile, pExtraHea, newFilename, fArchive, opt_testintegrity);
                        if (retval != NDB_MSG_OK)
                        {
                            strcat(sText, ", NTFS Sec. failed");
                        fprintf(stdout, "ndb_osdep_setExtraData: error setting NT security\n");
                            fflush(stdout);
                        }
                        else
                        {
                            strcat(sText, ", NTFS Sec. ok");
                        }
#else
                    if (ndb_getDebugLevel() >= 2)
                    {
                        fprintf(stdout, "ndb_osdep_setExtraData: extra data type %d (NT security) not supported by current OS/2\n", pExtraHea->typExtHea);
                        fflush(stdout);
                    }
#endif
                }
                else if (pExtraHea->typExtHea == NDB_EF_UNIX) // -----------------------------------------
                {
#if defined (UNIX)
            if (opt_testintegrity == 0)
            {
                // set UNIX file attributes
                retval = ndb_unix_setUnixAttributes(pFile, pExtraHea, newFilename);
                if (retval != NDB_MSG_OK)
                {
                                    strcat(sText, ", Unix data failed");
                        fprintf(stdout, "ndb_osdep_setExtraData: error setting UNIX file attributes\n");
                    fflush(stdout);
                }
                else
                {
                                    strcat(sText, ", Unix data ok");
                }
            }
#else
                    if (ndb_getDebugLevel() >= 2)
                    {
                        fprintf(stdout, "ndb_osdep_setExtraData: extra data type %d (UNIX file attributes) not supported by current OS/2\n", pExtraHea->typExtHea);
                        fflush(stdout);
                    }
#endif
                }
                else // ------------------------------------------------------------------------------
                {
                    if (ndb_getDebugLevel() >= 2)
                    {
                        fprintf(stdout, "ndb_osdep_setExtraData: extra data type %d not supported by current OS/2\n", pExtraHea->typExtHea);
                        fflush(stdout);
                    }
                }
        // switch to next extra header
        pExtraHea = pExtraHea->pNextHea;
        } // while (pExtraHea != NULL)
        // ---------------------------------------------------------------------------------------

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_osdep_setExtraData - finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
        ndb_osdep_getExtraData()

        gets extra data (e.g. EAs, ACLs, ...) from file;
        currently only OS/2 EAs & NT SD implemented

        Returns: (changed) pointer to block header lsit with zipped extra data
    *************************************************************************************
*/

int ndb_osdep_getExtraData(struct ndb_s_fileentry *pFile, struct ndb_s_blockheader **ppBlocks, struct ndb_s_fileextradataheader **ppExtraHea, FILE *fArchive, char *sText)
{
        int retval = NDB_MSG_OK;
        *ppBlocks = NULL;
#if defined(OS2)
    retval = ndb_os2_getExtraData(pFile, ppExtraHea, fArchive, ppBlocks, sText);
#elif defined(WIN32)
    retval = ndb_win32_getExtraData(pFile, ppExtraHea, fArchive, ppBlocks, sText);
#elif defined(UNIX)
     retval = ndb_unix_getExtraData(pFile, ppExtraHea, fArchive, ppBlocks, sText);
#endif
    // set flags within file meta data
    if ((*ppExtraHea != NULL) && ((*ppExtraHea)->lenOriExtra > 0))
    {
        pFile->flags |= NDB_FILEFLAG_HASEXTRA; /* set flag for extradata */
    }

    return retval;
}

/*
    *************************************************************************************
        ndb_osdep_getFileMode()

        return file attributes for <name>
    *************************************************************************************
*/

int ndb_osdep_getFileMode(char *name)
{
#if defined (WIN32)
    return ndb_win32_getFileMode(name);
#elif defined(OS2)
        return ndb_os2_getFileMode(name);
#elif defined(UNIX)
        return ndb_unix_getFileMode(name, 0);
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stderr, "ndb_osdep_getFileMode: no generic implementation\n");
    fflush(stdout);
    return 0;
#endif
}


/*
    *************************************************************************************
        ndb_osdep_setFileSize64()

        Checks, if this file has a length > 2^32 Bit;
        if so, it changes the length information in pFile
    *************************************************************************************
*/

void ndb_osdep_setFileSize64(PFILEENTRY pFile)
{
#if defined (WIN32)
    ndb_win32_setFileSize64(pFile);
#elif defined(OS2)
    ndb_os2_setFileSize64(pFile);
#elif defined(UNIX)
    ndb_unix_setFileSize64(pFile);
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stdout, "ndb_osdep_setFileSize64: no generic implementation\n");
    fflush(stdout);
#endif
}


/*
    *************************************************************************************
        ndb_osdep_mapNdbAttributes2OS()

        Converts the file attributes from NDB style to the OS style
    *************************************************************************************
*/

int ndb_osdep_mapNdbAttributes2OS(ULONG ulNDBAttributes)
{
    ULONG ulNewAttr;
#if defined (WIN32)
    ulNewAttr = ndb_win32_ndbattr2winattr(ulNDBAttributes);
#elif defined(OS2)
    ulNewAttr = ndb_os2_ndbattr2os2attr(ulNDBAttributes);
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stderr, "ndb_osdep_mapNdbAttributes2OS: no generic implementation\n");
    fflush(stdout);
    return 0;
#endif

        if (ndb_getDebugLevel() >= 7)
        fprintf(stderr, "ndb_osdep_mapNdbAttributes2OS: attributes: old %lX, new %lX\n",
                         ulNDBAttributes, ulNewAttr);
    fflush(stdout);
    return ulNewAttr;
}

/*
    *************************************************************************************
        ndb_osdep_makeIntFileName()

        converts the given string (with OS codepage) to UTF8 codepage

        Returns: string with UTF8 codepage
    *************************************************************************************
*/

char *ndb_osdep_makeIntFileName(const char *pExtFile)
{
#if defined (WIN32)
    return ndb_win32_makeIntFileName(pExtFile);
#elif defined(OS2)
    return ndb_os2_makeIntFileName(pExtFile);
#elif defined(UNIX)
    return ndb_unix_makeIntFileName(pExtFile);
#else
    return (char *) pExtFile;
#endif
}

/*
    *************************************************************************************
        ndb_osdep_makeExtFileName()

        converts the given string (with UTF8 codepage) to OS codepage

        Returns: string with OS codepage
    *************************************************************************************
*/

char *ndb_osdep_makeExtFileName(const char *pIntFile)
{
#if defined (WIN32)
    return ndb_win32_makeExtFileName(pIntFile);
#elif defined(OS2)
    return ndb_os2_makeExtFileName(pIntFile);
#elif defined(UNIX)
    return ndb_unix_makeExtFileName(pIntFile);
#else
    return (char *) pIntFile;
#endif
}

/*
    *************************************************************************************
        ndb_osdep_sleep()

        lets the application sleep for <millis> milliseconds (to be multi tasking
        friendly regarding CPU/IO/net usage
    *************************************************************************************
*/

void ndb_osdep_sleep(int millis)
{
#if defined (WIN32)
    ndb_win32_sleep(millis);
#elif defined(OS2)
    ndb_os2_sleep(millis);
#elif defined(UNIX)
    ndb_unix_sleep(millis);
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stderr, "ndb_osdep_sleep: no generic implementation\n");
    fflush(stdout);
    return;
#endif
}

/*
    *************************************************************************************
        ndb_osdep_createBatchImportAll_fname()

        returns the name of the batchfile which will import all extracted chapters
        in a new archive file
    *************************************************************************************
*/

char *ndb_osdep_createBatchImportAll_fname()
{
#if defined (WIN32)
    return ("ndb_convert.bat");
#elif defined(OS2)
    return ("ndb_convert.cmd");
#elif defined(UNIX)
    return ("ndb_convert.sh");
#else
    return ("");
#endif
}

/*
    *************************************************************************************
        ndb_osdep_createBatchImportAll_start()

        creates the header of the batchfile which will import all extracted chapters
        in a new archive file
    *************************************************************************************
*/

void ndb_osdep_createBatchImportAll_start(FILE *fBatch, char *pFName, struct ndb_s_c_archive *pCurrArch)
{
#if defined (WIN32) || defined(OS2)
#if defined (WIN32)
    fprintf(fBatch, "REM === NDB Import Batch For NT/W2K/XP =============\n");
#elif defined(OS2)
    fprintf(fBatch, "REM === NDB Import Batch For OS/2 ==================\n");
#endif
    fprintf(fBatch, "REM === define variables ===========================\n");
    fprintf(fBatch, "REM correct the following defines!\n");
    fprintf(fBatch, "set b_ndbc=ndbc6\n");
    fprintf(fBatch, "set b_ndba=ndba6\n");
    fprintf(fBatch, "set b_ndbe=ndbe6\n");
    fprintf(fBatch, "set b_ndbl=ndbl6\n");

#if defined (WIN32)
    fprintf(fBatch, "set b_del=rmdir /S/Q\n");
#elif defined(OS2)
    fprintf(fBatch, "set b_del=deltree2 /y/f\n");
#endif

    fprintf(fBatch, "\n");
    fprintf(fBatch, "REM === create chapter list for old archive ========\n");
    fprintf(fBatch, "cmd /c %%b_ndbl%% %s\n", pFName);
    fprintf(fBatch, "REM === create new archive file ====================\n");
    fprintf(fBatch, "if NOT EXIST %s_new cmd /c %%b_ndbc%% %s_new -b%d -l%d -f%lu\n",
                    pFName, pFName, pCurrArch->blockSize, pCurrArch->compression, pCurrArch->dataFilesSize);
    fprintf(fBatch, "\n");
    fprintf(fBatch, "REM === import each chapter ========================\n");
    fflush(fBatch);
#elif defined (UNIX)
    fprintf(fBatch, "#! /bin/sh\n");
    fprintf(fBatch, "# === NDB Import Batch For UNIX ==================\n");
    fprintf(fBatch, "# === define variables ===========================\n");
    fprintf(fBatch, "# correct the following defines!\n");
    fprintf(fBatch, "alias ndba=/usr/local/bin/ndba6\n");
    fprintf(fBatch, "alias ndbc=/usr/local/bin/ndbc6\n");
    fprintf(fBatch, "alias ndbe=/usr/local/bin/ndbe6\n");
    fprintf(fBatch, "alias ndbl=/usr/local/bin/ndbl6\n");
    fprintf(fBatch, "\n");
    fprintf(fBatch, "# === create chapter list for check by user =========\n");
    fprintf(fBatch, "ndbl %s\n", pFName);
    fprintf(fBatch, "# === create new archive file ====================\n");
    fprintf(fBatch, "if [ ! -f \"%s_new\" ] \n", pFName);
    fprintf(fBatch, "then \n");
    fprintf(fBatch, "    ndbc %s_new -b%d -l%d -f%lu \n",
                         pFName, pCurrArch->blockSize, pCurrArch->compression, pCurrArch->dataFilesSize);
    fprintf(fBatch, "fi \n");
    fprintf(fBatch, "\n");
    fprintf(fBatch, "# === import each chapter ========================\n");
    fflush(fBatch);
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stderr, "ndb_osdep_createBatchImportAll_start: no generic implementation\n");
    fflush(stdout);
    return;
#endif
}

/*
    *************************************************************************************
        ndb_osdep_createBatchImportAll_chapter()

        creates the part of the batchfile which imports one extracted chapter
        in the new archive file
    *************************************************************************************
*/

void ndb_osdep_createBatchImportAll_chapter(FILE *fBatch,  char *pFName, char *pPath,
                                            struct ndb_s_c_chapter *pCurrChapter)
{
    // create time string
    char stime[40];
    struct tm *tim;
    tim = localtime( (const time_t *) & pCurrChapter->chapterTime);
    if (tim != NULL)
        strftime(stime, 40, "%Y%m%d%H%M%S", tim);
    else
    {
        strcpy(stime, "19700101000000");
        fprintf(stderr, "warning: creation date of chapter %04d corrupt\n", pCurrChapter->chapterNo);
        fflush (stderr);
    }

#if defined (WIN32) || defined(OS2)
    fprintf(fBatch, "cmd /c %%b_ndbe%% %s -a%u -oa\"*\"\n",
                    pFName, pCurrChapter->chapterNo);
    fprintf(fBatch, "cd %s\n", pPath);
    fprintf(fBatch, "cmd /c %%b_ndba%% ..\\%s_new -cm -Rsn\"%s\" -m\"%s\" -w5 -t%s \"*\" \n",
                    pFName, pCurrChapter->chapterName, pCurrChapter->comment, stime);
    fprintf(fBatch, "cd ..\n");
    fprintf(fBatch, "%%b_del%% %s\n", pPath);
    fprintf(fBatch, "\n");
    fflush(fBatch);
#elif defined (UNIX)
    fprintf(fBatch, "ndbe %s -a%u -oa\"*\"\n",
                    pFName, pCurrChapter->chapterNo);
    fprintf(fBatch, "cd %s\n", pPath);
    fprintf(fBatch, "ndba ../%s_new -cm -Ryn\"%s\" -m\"%s\" -w5 -t%s \"*\" \n",
                    pFName, pCurrChapter->chapterName, pCurrChapter->comment, stime);
    fprintf(fBatch, "cd ..\n");
    fprintf(fBatch, "rm -R %s\n", pPath);
    fprintf(fBatch, "\n");
    fflush(fBatch);
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stderr, "ndb_osdep_createBatchImportAll_chapter: no generic implementation\n");
    fflush(stdout);
    return;
#endif
}

/*
    *************************************************************************************
        ndb_osdep_createBatchImportAll_end()

        creates the end of the batchfile which will import all extracted chapters
        in a new archive file
    *************************************************************************************
*/

void ndb_osdep_createBatchImportAll_end(FILE *fBatch,  char *pFName)
{
#if defined (WIN32) || defined(OS2)
    fprintf(fBatch, "REM === create chapter list for check by user =========\n");
    fprintf(fBatch, "cmd /c %%b_ndbl%% %s_new\n", pFName);
    fflush(fBatch);
#elif defined (UNIX)
    fprintf(fBatch, "# === create chapter list for check by user =========\n");
    fprintf(fBatch, "ndbl %s_new\n", pFName);
    fflush(fBatch);
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stderr, "ndb_osdep_createBatchImportAll_end: no generic implementation\n");
    fflush(stdout);
    return;
#endif
}

/*
    *************************************************************************************
        ndb_osdep_matchFilename()

        checks whether <name> matches the file mask <mask>;
        for OS/2 and Win32 the names are changed to lowercase
    *************************************************************************************
*/

int ndb_osdep_matchFilename(const char *mask, const char *name)
{
    int result = 0;
    char *p;
    char lowerMask[NDB_MAXLEN_FILEMASK];
    char lowerName[NDB_MAXLEN_FILEMASK];

/*
    // copy mask and filename to local strings
    char *lowerMask = ndb_calloc(1, 1 + strlen(mask), "ndb_osdep_matchFilename: lower case mask");
    char *lowerName = ndb_calloc(1, 1 + strlen(name), "ndb_osdep_matchFilename: lower case name");
    if ((lowerMask == NULL) || (lowerName == NULL))
    {
        fprintf(stderr, "ndb_osdep_matchFilename: couldn't allocate memory for strings\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }
*/
    strcpy(lowerMask, (char *) mask);
    strcpy(lowerName, (char *) name);

    // make sure that all path delimiters are '/', not '\'
    for (p = lowerMask; *p != '\0'; p++)
    {
        if (*p == '\\')
            *p = '/';
    }
    for (p = lowerName; *p != '\0'; p++)
    {
        if (*p == '\\')
            *p = '/';
    }

#if defined (WIN32)
    // make lowercase for Win32
    ndb_win32_StringLower(lowerMask);
    ndb_win32_StringLower(lowerName);
#elif defined(OS2)
    // make lowercase for OS/2
    ndb_os2_StringLower(lowerMask);
    ndb_os2_StringLower(lowerName);
#endif


    if ((ndb_getDebugLevel() >= 7) && (strcmp(lowerMask, "*") != 0))
    {
        fprintf(stdout, "ndb_osdep_matchFilename: before: mask '%s', file '%s'\n", mask, name);
        fprintf(stdout, "ndb_osdep_matchFilename: after:  mask '%s', file '%s'\n", lowerMask, lowerName);
        fprintf(stdout, "ndb_osdep_matchFilename: testing mask '%s' against file '%s' -> ", lowerMask, lowerName);
        fflush(stdout);
    }

    result = wildcardfit (lowerMask, lowerName);

    if ((ndb_getDebugLevel() >= 7) && (strcmp(mask, "*") != 0))
    {
        fprintf(stdout, "%d\n", result);
        fflush(stdout);
    }

/*
    // free memory of local strings
    ndb_free(lowerMask);
    ndb_free(lowerName);
*/
    return result;
}

/*
    *************************************************************************************
        ndb_osdep_getOS()

        Returns: current OS
    *************************************************************************************
*/

char *ndb_osdep_getOS()
{
        static char os[50] = "\0";

#if defined (WIN32)
/*
        if (ndb_win32_isWinNT())
            strcpy(os, "Win32 (NT/W2K/XP)");
        else
            strcpy(os, "Win32 (9X/ME)");
*/
        strcpy (os, ndb_win32_getWindowsVersion());
#elif defined(OS2)
    strcpy(os, ndb_os2_GetOS2Version());
#elif defined(UNIX)
    strcpy(os, "Unix/Linux");
#else
    strcpy(os, "unknown");
#endif

        return os;
}

/*
    *************************************************************************************
        ndb_osdep_getCodepage()

        Returns: current codepage from OS, if no codepage is NULL, return an empty string
    *************************************************************************************
*/

char *ndb_osdep_getCodepage()
{
        static char codepage[50] = "\0";
        char *codepage_os = NULL;

        // optimization: remember codepage and return it
        if (codepage[0] != '\0')
                return codepage;

#if defined (WIN32)
    codepage_os = ndb_win32_getCodepage();
#elif defined(OS2)
    codepage_os = ndb_os2_getCodepage();
#elif defined(UNIX)
    codepage_os = ndb_unix_getCodepage();
#else
        if (ndb_getDebugLevel() >= 3)
        fprintf(stdout, "ndb_osdep_getCodepage: no generic implementation\n");
    fflush(stdout);
#endif

        // if codepage is empty, return empty string
        if (codepage_os == NULL)
                strcpy (codepage, "");
    else
        strcpy (codepage, codepage_os);


    return codepage;
}

/* ------------------------------------------------------------------------*/

/*
    *************************************************************************************
        ndb_osdep_getPreciseTime()

        Returns: current time in ULONG8 format (ticks now are microseconds)
    *************************************************************************************
*/

ULONG8 ndb_osdep_getPreciseTime()
{
        ULONG8 llTime = 0;

#if defined TEST_PERFORMANCE
#if defined (WIN32)
        LARGE_INTEGER frequency;
        LARGE_INTEGER curr_time;
        double dbTime;
        // get counter frequency
        QueryPerformanceFrequency(&frequency);
        // get current counter ticks
        QueryPerformanceCounter(&curr_time);
        // calculate time in seconds
        dbTime = ((double) (4294967296.0 * curr_time.HighPart + curr_time.LowPart)) / (double) frequency.LowPart;
        // convert seconds to microseconds in long long format
        llTime = (ULONG8) (dbTime * 1000000.0);
#elif defined(OS2)
        double dbTime;
        ULONG ulTmrFreq;  /*  A pointer to the frequency of the counter. */
        QWORD  qwTmrTime; /*  The current count. */
        APIRET ulrc;        /*  Return Code. */
        ulrc = DosTmrQueryFreq(&ulTmrFreq);
        ulrc = DosTmrQueryTime(&qwTmrTime);
        // calculate time in seconds
        dbTime = ((double) (4294967296.0 * qwTmrTime.ulHi + qwTmrTime.ulLo)) / (double) ulTmrFreq;
        // convert seconds to microseconds in long long format
        llTime = (ULONG8) (dbTime * 1000000.0);
#endif
#endif

        return llTime;
}


/*
    *************************************************************************************
        ndb_osdep_version_local()

        prints out the compiler, version and date of NDB
    *************************************************************************************
*/

void ndb_osdep_version_local()
{
#if defined (WIN32)
    ndb_win32_version_local();
#elif defined(OS2)
    ndb_os2_version_local();
#elif defined(UNIX)
    ndb_unix_version_local();
#else
        fprintf(stdout, "Unknown target and compiler\n\n");
    fflush(stdout);
#endif
}
