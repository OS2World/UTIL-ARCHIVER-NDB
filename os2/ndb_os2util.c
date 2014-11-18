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
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <os2.h>

#define INCL_BASE
#if defined(__GNUC__)
#include <os2emx.h>
#endif

#include "ndb_os2.h"

#include "../ndb.h"
#include "../ndb_prototypes.h"



/*
    *************************************************************************************
    ndb_os2_split(const char *s_files, char *pDir, char *pWild)

    teilt eine Filemaske (ggfs mit Wildcards) auf in die zwei Teile
    Pfad und File;
    falls die Maske Wildcards enthält, enthält der Teil Pfad nur den
    Pfadteil vor dem ersten Wildcard, der Teil File enthält dann den
    restlichen Pfad samt der Filemaske
    *************************************************************************************
*/
void ndb_os2_split(const char *s_files, char *pDir, char *pWild)
{
    int iLastSep = 0;
    int i;
    char pathfile[NDB_MAXLEN_FILENAME];

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_split: path+file '%s' -> ", s_files);

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
        if ((pathfile[i] == '\\') || (pathfile[i] == '/') || (pathfile[i] == ':'))
            iLastSep = i;
    }
    if (iLastSep > -1)
    {
        pathfile[iLastSep] = '\0';
        strcpy (pWild, &pathfile[iLastSep + 1]);
        strcpy (pDir, pathfile);
        // 09-Mar-2004 CHG: add a cutted slash after drive
        // s_files was e.g. d:\*.txt
        if ((strlen(pDir) == 2) && (pDir[1] == ':'))
        {
            strcat(pDir, "/");
        }
        // 15-Mar-2004 CHG: add a cutted colon after drive
        // s_files was e.g. d:*.txt
        else if ((strlen(pDir) == 1) && (s_files[1] == ':'))
        {
            strcat(pDir, ":");
        }
        // 15-Mar-2004 CHG: add slash if (original) path started with root ("/...")
        // s_files was e.g. \*.txt
        else if ((strlen(pDir) == 0) && (s_files[0] == '/'))
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
        fprintf(stdout, "ndb_os2_split: path '%s', wildcard '%s'\n", pDir, pWild);
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_os2_makeFileList()

    searches of files (and subdirs if specified) of directory <dir>;
    check all files against file mask <wildcard>;
    if <doFlags> contains NDB_FILEFLAG_DIR, search within all subdirs also
    gives back the number <pCount> of files whic are contained in <dir>
    *************************************************************************************
*/

int ndb_os2_makeFileList(char *dir, char *wildcard, int doFlags, struct ndb_l_filelist *pFileList,
                         struct ndb_l_filemasklist *pFileListExcludes, ULONG *pCount)
{
    USHORT doHidden    = 0;        /* process hidden and system files */
    USHORT doDirs      = 0;        /* process directories */
    ULONG count        = 0;
    USHORT bIsMatch    = 0;
    USHORT bIsExclude  = 0;
    int i = 0;

    OS2DIR *dirent;
    ULONG ulAttr;
    char pathfile[NDB_MAXLEN_FILENAME];
    char wildcard2[NDB_MAXLEN_FILENAME];
//    USHORT len_filename;
    char *file;
    struct ndb_s_fileentry*pNewFile = NULL;


    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_makeFileList - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_os2_makeFileList arguments:\n");
        fprintf(stdout, "- dir:              '%s'\n", dir);
        fprintf(stdout, "- wildcard:         '%s'\n", wildcard);
        fprintf(stdout, "- doFlags:          '%d'\n", doFlags);
        fprintf(stdout, "- pFileList:        '%lX'\n", (ULONG) pFileList);
        fflush(stdout);
    }

    // process hidden and system files?
    if ((doFlags & NDB_FILEFLAG_HIDDEN) == NDB_FILEFLAG_HIDDEN)
        doHidden = 1;
    // recurse into directories?
    if ((doFlags & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
        doDirs = 1;

    dirent = ndb_os2_opendir(dir, doHidden);
    if (dirent == NULL)
    {
        if (ndb_getDebugLevel() >= 2)
        {
            fprintf(stdout, "ndb_os2_makeFileList: warning: %s is no directory\n", dir);
            fflush(stdout);
        }
    }
    else
    {
        if (ndb_getDebugLevel() >= 2)
        {
            fprintf(stdout, "scanning directory '%s'\n", dir);
            fflush(stdout);
        }

        file = ndb_os2_readd(dirent);
        while (file != NULL)
        {
            if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0)
            {
                file = ndb_os2_readd(dirent);
                continue;
            }

            // add path if not empty or '.'
            if ((dir[0] != '\0') && (strcmp(dir, ".") != 0))
            {
                strcpy(pathfile, dir);
                // do we need a directory seperator?
                if ((dir[strlen(pathfile) - 1] != '/') && (dir[strlen(pathfile) - 1] != ':'))
                    strcat(pathfile, "/");
                snprintf(pathfile, NDB_MAXLEN_FILENAME - 1, "%s%s%c", pathfile, file, '\0');
            }
            else
                snprintf(pathfile, NDB_MAXLEN_FILENAME - 1, "%s%c", file, '\0');
            // get file attribut flags
            ulAttr = ndb_os2_getFileMode(pathfile);
            if (ndb_getDebugLevel() >= 5)
            {
                fprintf(stdout, "result of ndb_os2_getFileMode(): ulAttr = %ld\n", ulAttr);
                fflush(stdout);
            }

            // 23JUN03 CHG: add dir only to file list if -r was specified
            if ((doDirs == 1) && ((ulAttr & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR))
            {
                // add dir to list
                ndb_osdep_addFileToFileList(&pNewFile, pathfile, ulAttr, pFileList);
                // recurse into subdirectory if specified
                if (doDirs == 1)
                {
                    if (ndb_getDebugLevel() >= 7)
                        fprintf(stdout, "ndb_os2_makeFileList: next dir:  %s(0x%lX)\n", pathfile, ulAttr);
                    ndb_os2_makeFileList(pathfile, wildcard, doFlags, pFileList, pFileListExcludes, &pNewFile->children);
                }
                else
                {
                    if (ndb_getDebugLevel() >= 7)
                        fprintf(stdout, "ndb_os2_makeFileList: skip dir:  %s(0x%lX)\n", pathfile, ulAttr);
                }

                // keep only directories with files
                if (ndb_getDebugLevel() >= 5)
                    fprintf(stdout, "ndb_os2_makeFileList: dir %s has %lu files to archive\n", pathfile, pNewFile->children);
                fflush(stdout);
                if (pNewFile->children > 0)
                {
                    // subdirectories has files
                    count += pNewFile->children;
                }
/* pw; 01-09-03;    now done by ndb_archive.c
                else
                {
                    pNewFile->action = NDB_ACTION_IGNORE;
                }
*/
                // read next file
                file = ndb_os2_readd(dirent);

            }
            else // if ((doDirs == 1) && ((ulAttr & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR))
            {

// 30-oct-2003; pw; CHG
                // if pathfile contains a path then wildcard has to include a path
                // also (add '*/' if path missing)
// pw; 15-Mar-2004: not true for wildcards like 'h:*.test'
// 01-sep-2004; pw; CHG
                // add only '*' to avoid the following problem:
                // 'c: cd \ ndba test.ndb -rn os2*' would archive nothing
                // because the filemask 'os2*' would be converted to '*\os2*'
                // -> nothing can be found ...
// pw; 15-Mar-2004: not true for wildcards like 'h:*.test'
                if ( (strchr(pathfile, '/') != NULL || strchr(pathfile, '\\') != NULL)
                    &&
                     (strchr(wildcard, '/') == NULL && strchr(wildcard, '\\') == NULL)
                    &&
                     (strchr(wildcard, ':') == NULL)
                   )
                {
                    snprintf(wildcard2, NDB_MAXLEN_FILENAME - 1, "*%s%c", wildcard, '\0');
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
                    fprintf(stdout, "ndb_os2_makeFileList: %d = match('%s', '%s')\n", bIsMatch, wildcard2, pathfile);
                    fflush(stdout);
                }
                if (bIsMatch)
                {
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "next file to add: %s(0x%lX)\n", pathfile, ulAttr);
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
                file = ndb_os2_readd(dirent);
            }
        }
    }

    // return number of files for current directories
    *pCount = count;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_makeFileList - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
        ndb_os2_makeIntFileName()

        converts the given string o internal format

        Returns: string with '/' instead of '\'
    *************************************************************************************
*/

char pFileInt[NDB_MAXLEN_FILENAME];
char pFileExt[NDB_MAXLEN_FILENAME];

char *ndb_os2_makeIntFileName(const char *pExtFile)
{
    char *p;

    strcpy (pFileInt, pExtFile);

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_os2_makeIntFileName: before conversion: '%s'\n", pExtFile);

    // replace '\' by '/'
    strcpy (pFileInt, pExtFile);
    while (NULL != (p = strrchr(pFileInt, '\\')))
    {
        *p = '/';
    }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_os2_makeIntFileName:  after conversion: '%s'\n", pFileInt);

    return pFileInt;
}

/*
    *************************************************************************************
        ndb_os2_makeExtFileName()

        converts the given string (with UTF8 codepage) to OS codepage

        Returns: string with OS codepage
    *************************************************************************************
*/

char *ndb_os2_makeExtFileName(const char *pIntFile)
{
    char *p;

    strcpy (pFileExt, pIntFile);

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_os2_makeExtFileName: before conversion: '%s'\n", pIntFile);

    // replace '/' by '\'
    strcpy (pFileExt, pIntFile);
    while (NULL != (p = strrchr(pFileExt, '/')))
    {
        *p = '\\';
    }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_os2_makeExtFileName:  after conversion: '%s'\n", pFileExt);

    return pFileExt;
}

/* ------------------------------------------------------------------------*/
