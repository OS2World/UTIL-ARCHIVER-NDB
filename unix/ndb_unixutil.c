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

/*
    This file contains source code from InfoZip 2.3 and InfoUnzip 5.5 also.
    I only modified little things to fit the InfoZip functions into NDB.
    Therefore see the Info-ZIP license for these functions.

    Peter Wiesneth, 03/2004
*/
/*
  Copyright (c) 1990-1999 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 1999-Oct-05 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, both of these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.cdrom.com/pub/infozip/license.html
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>

#include <errno.h>
extern int errno;

#include "../ndb.h"
#include "../ndb_prototypes.h"

#include "ndb_unix.h"



char *readd(DIR *d)
/* Return a pointer to the next name in the directory stream d, or NULL if
   no more entries or an error occurs. */
{
  struct dirent *e;

  e = readdir(d);
  return e == NULL ? (char *) NULL : e->d_name;
}


/*
    *************************************************************************************
    ndb_unix_split(const char *s_files, char *pDir, char *pWild)

    teilt eine Filemaske (ggfs mit Wildcards) auf in die zwei Teile
    Pfad und File;
    falls die Maske Wildcards enthält, enthält der Teil Pfad nur den
    Pfadteil vor dem ersten Wildcard, der Teil File enthält dann den
    restlichen Pfad samt der Filemaske
    *************************************************************************************
*/
void ndb_unix_split(const char *s_files, char *pDir, char *pWild)
{
    int iLastSep;
    int i;
    char pathfile[NDB_MAXLEN_FILENAME];

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_split: path+file '%s' -> ", s_files);

    pDir[0] = '\0';
    pWild[0] = '\0';
    strcpy (pathfile, s_files);

    // check for the last directory separator before a wildcard
    // try both: '\' win style & '/' unix style
    iLastSep = -1;
    for (i = 0; pathfile[i] != '\0'; i++)
    {
        // if wildcard found, leave loop
        if ((pathfile[i] == '*') || (pathfile[i] == '?') || (pathfile[i] == '['))
            break;
        // if path delimiter found, remember position
        if ((pathfile[i] == '\\') || (pathfile[i] == '/'))
            iLastSep = i;
    }
    if (iLastSep > -1)
    {
        pathfile[iLastSep] = '\0';
        strcpy (pWild, &pathfile[iLastSep + 1]);
        strcpy (pDir, pathfile);
/* cannot happen with Linux ;-)
        // 09-Mar-2004 CHG: add backslash after drive
        if ((strlen(pDir) == 2) && (pDir[1] == ':'))
        {
            strcat(pDir, "/");
        }
*/
        // 15-Mar-2004 CHG: add slash if (original) path started with root ("/...")
        if ((strlen(pDir) == 0) && (s_files[0] == '/'))
        {
            strcpy(pDir, "/");
        }
    }
    else
    {
        strcpy (pDir,  ".");
        strcpy (pWild, pathfile);
    }
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_split: path '%s', wildcard '%s'\n", pDir, pWild);
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_unix_makeFileList()

    searches of files (and subdirs if specified) of directory <dir>;
    check all files against file mask <wildcard>;
    if <doFlags> contains NDB_FILEFLAG_DIR, search within all subdirs also
    gives back the number <pCount> of files whic are contained in <dir>
    *************************************************************************************
*/

int ndb_unix_makeFileList(char *dir, char *wildcard, int doFlags, struct ndb_l_filelist *pFileList,
                          struct ndb_l_filemasklist *pFileListExcludes, ULONG *pCount)
{
    USHORT doArcSymLink = 0;     /* 0: archive symlinks as files */
    USHORT doDirs   = 0;        /* 0: don't process directories */
    ULONG count = 0;
    USHORT bIsMatch    = 0;
    USHORT bIsExclude  = 0;
    int i = 0;

    DIR *dirent;
    ULONG ulAttr;
    char pathfile[NDB_MAXLEN_FILENAME];
    char wildcard2[NDB_MAXLEN_FILENAME];
    char sDummy[NDB_MAXLEN_FILENAME];
//    USHORT len_filename;
    char *file;
    struct ndb_s_fileentry *pNewFile = NULL;


    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_makeFileList - startup\n");
    fflush(stdout);

    // process symbolic links?
    if ((doFlags & NDB_FILEFLAG_SLNK) == NDB_FILEFLAG_SLNK)
        doArcSymLink = 1;
    // recurse into directories?
    if ((doFlags & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
        doDirs = 1;

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_unix_makeFileList: flags: symlink = %d, recursive = %d\n", doArcSymLink, doDirs);
        fflush(stdout);
    }

    dirent = opendir(dir);
    if (dirent == NULL)
    {
        fprintf(stdout, "ndb_unix_makeFileList: Error: no permission or %s is no directory\n", dir);
        fflush(stdout);
    }
    else // if (dirent == NULL)
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "scanning directory '%s'\n", dir);
            fflush(stdout);
        }

        file = readd(dirent);
        while (file != NULL)
        {
            if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0)
            {
                file = readd(dirent);
                continue;
            }

            // add path if not empty or '.'
            if ((dir[0] != '\0') && (strcmp(dir, ".") != 0))
            {
                strcpy(pathfile, dir);
                // do we need a directory seperator?
                if (dir[strlen(pathfile) - 1] != '/')
                    strcat(pathfile, "/");
                snprintf(sDummy, NDB_MAXLEN_FILENAME - 1, "%s%s", pathfile, file);
                strcpy(pathfile, sDummy);
            }
            else
                snprintf(sDummy, NDB_MAXLEN_FILENAME - 1, "%s", file);
                strcpy(pathfile, sDummy);

            // get file attribut flags
            ulAttr = ndb_unix_getFileMode(pathfile, doArcSymLink);

            // skip unsupported file types
            if ((ulAttr & NDB_FILEFLAG_CHARDEV) == NDB_FILEFLAG_CHARDEV)
            {
                fprintf(stdout, "skipping character device '%s'\n", pathfile);
                fflush(stdout);
                file = readd(dirent);
                continue;
            }
            if ((ulAttr & NDB_FILEFLAG_BLKDEV) == NDB_FILEFLAG_BLKDEV)
            {
                fprintf(stdout, "skipping block device '%s'\n", pathfile);
                fflush(stdout);
                file = readd(dirent);
                continue;
            }
            if ((ulAttr & NDB_FILEFLAG_FIFO) == NDB_FILEFLAG_FIFO)
            {
                fprintf(stdout, "skipping fifo '%s'\n", pathfile);
                fflush(stdout);
                file = readd(dirent);
                continue;
            }
            if ((ulAttr & NDB_FILEFLAG_SOCKET) == NDB_FILEFLAG_SOCKET)
            {
                fprintf(stdout, "skipping socket '%s'\n", pathfile);
                fflush(stdout);
                file = readd(dirent);
                continue;
            }

            // 23JUN03 CHG: add dir only to file list if -r was specified
            if ((doDirs == 1) && ((ulAttr & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR))
            {
                // add dir to list
                ndb_osdep_addFileToFileList(&pNewFile, pathfile, ulAttr, pFileList);
                // recurse into (sub-)directory if specified
                if (doDirs == 1)
                {
                    if (ndb_getDebugLevel() >= 7)
                        fprintf(stdout, "ndb_unix_makeFileList: next dir:  %s(0x%lX)\n", pathfile, ulAttr);
                    ndb_unix_makeFileList(pathfile, wildcard, doFlags, pFileList, pFileListExcludes, &pNewFile->children);
                }
                else
                {
                    if (ndb_getDebugLevel() >= 7)
                        fprintf(stdout, "ndb_unix_makeFileList: skip dir:  %s(0x%lX)\n", pathfile, ulAttr);
                }
                // keep only directories with files
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_unix_makeFileList: dir %s has %lu files to archive\n", pathfile, pNewFile->children);
                fflush(stdout);
                if (pNewFile->children > 0)
                {
                    // subdirectories has files
                    count += pNewFile->children;
                }
                // read next file
                file = readd(dirent);
            }
            else // if ((doDirs == 1) && ((ulAttr & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR))
            {
// 30-oct-2003; pw; CHG
                // if pathfile contains a path then wildcard has to include a path
                // also (add '*/' if path missing)
// 01-sep-2004; pw; CHG
                // add only '*' to avoid the following problem:
                // 'cd / ndba test.ndb -rn home*' would archive nothing
                // because the filemask 'home*' would be converted to '*/home*'
                // -> nothing can be found ...
// pw; 15-Mar-2004: not true for wildcards like 'h:*.test'
                if ( (strchr(pathfile, '/') != NULL || strchr(pathfile, '\\') != NULL)
                &&
                (strchr(wildcard, '/') == NULL && strchr(wildcard, '\\') == NULL)
                )
                {
                        snprintf(wildcard2, NDB_MAXLEN_FILENAME - 1, "*%s", wildcard);
                }
                else
                {
                        strcpy(wildcard2, wildcard);
                }

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
                // do first the checks against the exclude filemasks
                if (ndb_getDebugLevel() >= 6)
                {
                    fprintf(stdout, "ndb_os2_makeFileList: testing '%s' against excludes masks\n", pathfile);
                    fflush(stdout);
                }
                bIsExclude = 0;
                for (i = 0; i < ndb_list_getFileMaskCount(pFileListExcludes); i++)
                {
                    if (ndb_osdep_matchFilename(ndb_list_getFileMask(pFileListExcludes, i), pathfile) == 1)
                    {
                        if (ndb_getDebugLevel() >= 6)
                        {
                            fprintf(stdout, "ndb_os2_makeFileList: '%s' matches against excludes mask '%s'\n",
                                    pathfile, ndb_list_getFileMask(pFileListExcludes, i));
                            fflush(stdout);
                        }
                        bIsExclude = 1;
                        break;
                    }
                }
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
                bIsMatch = (bIsExclude == 1) ? 0 : ndb_osdep_matchFilename(wildcard2, pathfile);
                if ((bIsExclude == 0) && (ndb_getDebugLevel() >= 7))
                {
                    fprintf(stdout, "ndb_unix_makeFileList: %d = match('%s', '%s')\n", bIsMatch, wildcard2, pathfile);
                    fflush(stdout);
                }
                if (bIsMatch)
                {
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "next file: %s(0x%lX)\n", pathfile, ulAttr);
                        fflush(stdout);
                    }
                    ndb_osdep_addFileToFileList(NULL, pathfile, ulAttr, pFileList);
                    // count files for current directory
                    count++;
                }
                else
                {
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "skip file: %s(0x%lX)\n", pathfile, ulAttr);
                    }
                }
                file = readd(dirent);
            } // if ((doDirs == 1) && ((ulAttr & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR))
        } // while (file != NULL)
        closedir(dirent);
    } // if (dirent == NULL)

    // return number of files for current directories
    *pCount = count;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_makeFileList - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
	ndb_unix_makeIntFileName()

	converts the given string (with OS codepage) to UTF8 codepage

	Returns: string with UTF8 codepage
    *************************************************************************************
*/

char pFileInt[NDB_MAXLEN_FILENAME];
char pFileExt[NDB_MAXLEN_FILENAME];

char *ndb_unix_makeIntFileName(const char *pExtFile)
{
    char *p;

    strcpy (pFileInt, pExtFile);

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_makeIntFileName: before conversion: '%s'\n", pExtFile);

    // replace '\' by '/'
    strcpy (pFileInt, pExtFile);
    while (NULL != (p = strrchr(pFileInt, '\\')))
    {
        *p = '/';
    }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_makeIntFileName:  after conversion: '%s'\n", pFileInt);

    return pFileInt;
}

/*
    *************************************************************************************
	ndb_unix_makeExtFileName()

	converts the given string (with UTF8 codepage) to OS codepage

	Returns: string with OS codepage
    *************************************************************************************
*/

char *ndb_unix_makeExtFileName(const char *pIntFile)
{
    strcpy (pFileExt, pIntFile);

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_makeExtFileName: before conversion: '%s'\n", pIntFile);

/*  Unix: no slash conversion neccessary
    // replace '/' by '\'
    char *p;
    strcpy (pFileExt, pIntFile);
    while (NULL != (p = strrchr(pFileExt, '/')))
    {
        *p = '\\';
    }
*/
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_makeExtFileName:  after conversion: '%s'\n", pFileExt);

    return pFileExt;
}


/*
    *************************************************************************************
	ndb_unix_addFileAttributes()

	adds these unix standard file attributes which are not covered by
        the file entry as an extra header
    *************************************************************************************
*/

int ndb_unix_addFileAttributes(char *filename, PFILEEXTRADATAHEADER pExtraHea, ULONG fileattr)
{
    char *pData = NULL;
    int count;
    struct stat myStat;
    char buffer[NDB_MAXLEN_FILENAME] = "\0";
    int len = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_addFileAttributes - startup\n");
    fflush(stdout);

    if (-1 == LSSTAT(filename, &myStat))
    {
        return NDB_MSG_NOFILEOPEN;
    }

    // create direct data buffer for extra data header
    pData = ndb_calloc(1, NDB_SAVESIZE_EXTRADATA_UNIX + NDB_MAXLEN_FILENAME, "ndb_unix_addFileAttributes: direct data");
    if (pData == NULL)
    {
        fprintf(stderr, "ndb_unix_addFileAttributes: couldn't allocate direct data\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }

    if ((fileattr & NDB_FILEFLAG_SLNK) == NDB_FILEFLAG_SLNK)
    {
        // read target of symbolic link
        len = readlink(filename, buffer, NDB_MAXLEN_FILENAME);
        buffer[len] = '\0';
    }

    // fill direct data of extra header
    count = 0;
    ndb_write2buf_ushort(pData, &count, myStat.st_mode);
    ndb_write2buf_ushort(pData, &count, myStat.st_nlink);
    ndb_write2buf_ushort(pData, &count, myStat.st_uid);
    ndb_write2buf_ushort(pData, &count, myStat.st_gid);
    // write data of symbolic link content
    ndb_write2buf_ushort(pData, &count, len);
    if (len > 0)
        ndb_write2buf_char_n(pData, &count, buffer, len);


    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_unix_addFileAttributes: file attributes 0%o\n", myStat.st_mode);
        fprintf(stdout, "ndb_unix_addFileAttributes: nr of links     %d\n", myStat.st_nlink);
        fprintf(stdout, "ndb_unix_addFileAttributes: user ID         %d\n", myStat.st_uid);
        fprintf(stdout, "ndb_unix_addFileAttributes: group ID        %d\n", myStat.st_gid);
        fprintf(stdout, "ndb_unix_addFileAttributes: len symlnk      %d\n", len);
        fflush(stdout);
    }

    pExtraHea->typExtHea   = NDB_EF_UNIX;
    pExtraHea->lenExtHea   = NDB_SAVESIZE_FILEEXTRAHEADER + NDB_SAVESIZE_EXTRADATA_UNIX + len;
    pExtraHea->lenOriExtra = NDB_SAVESIZE_EXTRADATA_UNIX + len;
    pExtraHea->lenZipExtra = NDB_SAVESIZE_EXTRADATA_UNIX + len;
    pExtraHea->crc32Extra  = ndb_crc32(0L, pData, count);
    pExtraHea->blockCountExtra = 0;
    pExtraHea->pData       = pData;
    pExtraHea->pNextHea    = NULL;
    pExtraHea->firstBlockHeader  = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_addFileAttributes - finished\n");
    fflush(stdout);

	return NDB_MSG_OK;
}


/*
    *************************************************************************************
	ndb_unix_getExtraData()

	get EAs and pack the EA data into a list of blocks (if size of EAs not bigger
	than the blocksize one block is enough);
	return the pointer of the block header list start
    *************************************************************************************
*/

int ndb_unix_getExtraData(struct ndb_s_fileentry *pFile, struct ndb_s_fileextradataheader **ppExtraHea, FILE *fArchive, struct ndb_s_blockheader **ppBlocks, char *sText)
{
    int retval;
    char sExtName[NDB_MAXLEN_FILENAME];
    char sDummy[200];
    struct ndb_s_fileextradataheader *pExtraHea = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_getExtraData - startup\n");
    fflush(stdout);

    pExtraHea = ndb_calloc(sizeof(struct ndb_s_fileextradataheader), 1, "ndb_os2_getExtraData: pExtraHea");
    if (pExtraHea == NULL)
    {
        fprintf(stderr, "ndb_unix_getExtraData: couldn't allocate extra data header\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }

    strcpy(sExtName, ndb_osdep_makeExtFileName(pFile->filenameExt));

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_getExtraData: calling ndb_unix_addFileAttributes() for '%s'\n", sExtName);
    fflush(stdout);

    retval = ndb_unix_addFileAttributes(sExtName, pExtraHea, pFile->attributes);
	if (retval != NDB_MSG_OK)
	{
	    strcat(sText, ", Unix data failed");
	}
	else
	{
		snprintf(sDummy, 200 - 1, ", %lub Unix data%c", pExtraHea->lenOriExtra, '\0');
        strcat(sText, sDummy);
	}

    *ppExtraHea = pExtraHea;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_getExtraData - finished\n");
    fflush(stdout);

    return retval;
}

