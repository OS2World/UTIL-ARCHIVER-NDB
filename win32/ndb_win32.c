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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <time.h>
#include <sys/utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <locale.h>

#define PAD           0
#define PATH_END      '/'
#define HIDD_SYS_BITS (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

/* DBCS support for Info-ZIP's zip  (mainly for japanese (-: )
 * by Yoshioka Tsuneo (QWF00133@nifty.ne.jp,tsuneo-y@is.aist-nara.ac.jp)
 * This code is public domain!   Date: 1998/12/20
 */
#   define CLEN(ptr) 1
#   define PREINCSTR(ptr) (++(ptr))
#   define POSTINCSTR(ptr) ((ptr)++)
#   define lastchar(ptr) ((*(ptr)=='\0') ? '\0' : ptr[strlen(ptr)-1])
#   define MBSCHR(str, c) strchr(str, c)
#   define MBSRCHR(str, c) strrchr(str, c)
#   define SETLOCALE(category, locale)
#define INCSTR(ptr) PREINCSTR(ptr)


#include "../ndb.h"
#include "../ndb_prototypes.h"

#include <winbase.h>

#if defined(__WATCOMC__)
#include <direct.h>
#endif

// extern int hidden_files;        /* process hidden and system files */


/* Module level constants */
const char wild_match_all[] = "*";

/*
    *************************************************************************************
        ndb_win32_s_direntry()

        adapted source from Infozips Zip, but not really understood ;-)

        checks (and modifies) the path and returns a direntry* object (or NULL)
    *************************************************************************************
*/

struct ndb_win32_s_direntry *Opendir(n)
const char *n;          /* directory to open */
/* Start searching for files in the MSDOS directory n */
{
  struct ndb_win32_s_direntry *d;     /* malloc'd return value */
  char *p;                            /* malloc'd temporary string */
  char *q;
  WIN32_FIND_DATA fd;

  if (ndb_getDebugLevel() >= 5)
    fprintf(stdout, "(Win32) Opendir - startup\n");
  fflush(stdout);

  if (ndb_getDebugLevel() >= 7)
    fprintf(stdout, "(Win32) Opendir: got dir name '%s'\n", n);
  fflush(stdout);

  if ((d = (struct ndb_win32_s_direntry *)malloc(sizeof(struct ndb_win32_s_direntry))) == NULL ||
      (p = malloc(strlen(n) + (2 + sizeof(wild_match_all)))) == NULL)
  {
    if (d != NULL)
        free((void *)d);
    return NULL;
  }
  strcpy(p, n);
  q = p + strlen(p);
  if ((q - p) > 0 && MBSRCHR(p, ':') == (q - 1))
      *q++ = '.';
  if ((q - p) > 0 && MBSRCHR(p, '/') != (q - 1))
    *q++ = '/';
  strcpy(q, wild_match_all);

  if (ndb_getDebugLevel() >= 7)
    fprintf(stdout, "(Win32) Opendir: look for a dir like '%s'\n", p);
  fflush(stdout);

  d->d_hFindFile = FindFirstFile(p, &fd);
  free((void *)p);

  if (d->d_hFindFile == INVALID_HANDLE_VALUE)
  {
    if (ndb_getDebugLevel() >= 7)
      fprintf(stdout, "(Win32) Opendir: nothing found, return NULL\n");
    fflush(stdout);
    free((void *)d);
    return NULL;
  }

  strcpy(d->d_name, fd.cFileName);
  d->d_attr = (unsigned char) fd.dwFileAttributes;
  d->d_first = 1;

  if (ndb_getDebugLevel() >= 7)
    fprintf(stdout, "(Win32) Opendir: found a dir '%s'\n", d->d_name);
  fflush(stdout);

  if (ndb_getDebugLevel() >= 5)
    fprintf(stdout, "(Win32) Opendir - finished\n");
  fflush(stdout);

  return d;
}

/*
    *************************************************************************************
        Readdir()

        Returns pointer to first or next directory entry, or NULL if end
    *************************************************************************************
*/

struct ndb_win32_s_direntry *Readdir(d)
struct ndb_win32_s_direntry *d;             /* directory stream to read from */
/* Return pointer to first or next directory entry, or NULL if end. */
{
  if (d->d_first)
    d->d_first = 0;
  else
  {
    WIN32_FIND_DATA fd;

    if (!FindNextFile(d->d_hFindFile, &fd))
        return NULL;
    strcpy(d->d_name, fd.cFileName);
    d->d_attr = (unsigned char) fd.dwFileAttributes;
  }
  return (struct ndb_win32_s_direntry *)d;
}

/*
    *************************************************************************************
        Closedir()

        closes direntry and frees resources
    *************************************************************************************
*/

void Closedir(d)
struct ndb_win32_s_direntry *d;               /* directory stream to close */
{
  FindClose(d->d_hFindFile);
  free((void *)d);
}

/*
    *************************************************************************************
        readd()

        Return a pointer to the next name in the directory stream d, or NULL if no more
        entries or an error occurs
    *************************************************************************************
*/

char *readd(d)
struct ndb_win32_s_direntry *d;           /* directory stream to read from */
/* Return a pointer to the next name in the directory stream d, or NULL if
   no more entries or an error occurs. */
{
  struct ndb_win32_s_direntry *e;

//  do
        e = Readdir(d);
//  while (!hidden_files && e && e->d_attr & HIDD_SYS_BITS);

  return e == NULL ? (char *) NULL : e->d_name;
}

/*
    *************************************************************************************
        ndb_win32_getFileMode()

        get file attributes for <name> and map them to NDB fileflags

        Returns: NDB fileflags of file
    *************************************************************************************
*/

int ndb_win32_getFileMode(char *name)
{
    DWORD dwAttr;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_win32_getFileMode - startup\n");

    if ((name == NULL) || (name[0] == '\0'))
    {
        fprintf(stdout, "ndb_win32_getFileMode: internal error: filename is NULL or empty\n");
        fflush(stdout);
        return 0;
    }


    dwAttr = GetFileAttributes(name);
    if ( dwAttr == 0xFFFFFFFF )
    {
                if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "ndb_win32_getFileMode: ");
        fprintf(stdout, "getting file attributes failed for '%s'\n", name);
        fflush (stdout);
        return(0x20); /* the most likely, though why the error? security? */
    }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_win32_getFileMode: file attributes 0x%lX for file '%s'\n",
                        dwAttr, name);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_win32_getFileMode - finished\n");
    fflush(stdout);

/*
    return
        (
          (dwAttr & FILE_ATTRIBUTE_READONLY  ? NDB_FILEFLAG_RONLY   : 0)
        | (dwAttr & FILE_ATTRIBUTE_HIDDEN    ? NDB_FILEFLAG_HIDDEN  : 0)
        | (dwAttr & FILE_ATTRIBUTE_SYSTEM    ? NDB_FILEFLAG_SYSTEM  : 0)
        | (dwAttr & FILE_ATTRIBUTE_DIRECTORY ? NDB_FILEFLAG_DIR     : 0)
        | (dwAttr & FILE_ATTRIBUTE_ARCHIVE   ? NDB_FILEFLAG_ARCHIVE : 0)
        );
*/
    return ndb_win32_winattr2ndbattr(dwAttr);
}


/* ------------------------------------------------------------------------*/

/*****************************/
/* Function utime2FileTime() */     /* convert Unix time_t format into the */
/*****************************/     /* form used by SetFileTime() in NT/95 */

#define UNIX_TIME_ZERO_HI  0x019DB1DEUL
#define UNIX_TIME_ZERO_LO  0xD53E8000UL
#define NT_QUANTA_PER_UNIX 10000000L

static void utime2FileTime(time_t ut, FILETIME *pft)
{
    unsigned int b1, b2, carry = 0;
    unsigned long r0, r1, r2, r3;
    long r4;            /* signed, to catch environments with signed time_t */

    b1 = ut & 0xFFFF;
    b2 = (ut >> 16) & 0xFFFF;       /* if ut is over 32 bits, too bad */
    r1 = b1 * (NT_QUANTA_PER_UNIX & 0xFFFF);
    r2 = b1 * (NT_QUANTA_PER_UNIX >> 16);
    r3 = b2 * (NT_QUANTA_PER_UNIX & 0xFFFF);
    r4 = b2 * (NT_QUANTA_PER_UNIX >> 16);
    r0 = (r1 + (r2 << 16)) & 0xFFFFFFFFL;
    if (r0 < r1)
        carry++;
    r1 = r0;
    r0 = (r0 + (r3 << 16)) & 0xFFFFFFFFL;
    if (r0 < r1)
        carry++;
    pft->dwLowDateTime = r0 + UNIX_TIME_ZERO_LO;
    if (pft->dwLowDateTime < r0)
        carry++;
    pft->dwHighDateTime = r4 + (r2 >> 16) + (r3 >> 16)
                            + UNIX_TIME_ZERO_HI + carry;
} /* end function utime2FileTime() */

/* ------------------------------------------------------------------------*/

/****************************/      /* Get the file time in a format that */
/* Function getNTfiletime() */      /*  can be used by SetFileTime() in NT */
/****************************/

static int getNTfiletime(unixtime_c, unixtime_m, unixtime_a, pModFT, pAccFT, pCreFT)
    ULONG unixtime_c;
    ULONG unixtime_m;
    ULONG unixtime_a;
    FILETIME *pModFT;
    FILETIME *pAccFT;
    FILETIME *pCreFT;
{

    utime2FileTime(unixtime_c, pCreFT);
    utime2FileTime(unixtime_m, pModFT);
    utime2FileTime(unixtime_a, pAccFT);

    return (1);

} /* end function getNTfiletime() */

/*
    *************************************************************************************
        ndb_win32_SetPathAttrTimes()

        sets file attributes and file times (ctime, mtime, atime)
    *************************************************************************************
*/

void ndb_win32_SetPathAttrTimes(struct ndb_s_fileentry *pFile, const char *sDefaultPath)
{
    FILETIME Modft = { 0, 0 };    /* File time type defined in NT, `last modified' time */
    FILETIME Accft = { 0, 0 };    /* NT file time type, `last access' time */
    FILETIME Creft = { 0, 0 };    /* NT file time type, `file creation' time */
    HANDLE hFile;      /* File handle defined in NT    */
    int gotTime;
    char sInt[NDB_MAXLEN_FILENAME];
    char sExt[NDB_MAXLEN_FILENAME];
    ULONG ulFileAttributes = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_SetPathAttrTimes - startup\n");
    fflush(stdout);

    if (strcmp(sDefaultPath, ".") == 0)
    {
        strcpy(sInt, pFile->filenameExt);
    }
    else
    {
        snprintf(sInt, NDB_MAXLEN_FILENAME - 1, "%s/%s", sDefaultPath, pFile->filenameExt);
    }

    strcpy (sExt, ndb_osdep_makeExtFileName(sInt));

#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
    char *ansi_name = (char *)alloca(strlen(sExt) + 1);
    INTERN_TO_ISO(sExt, ansi_name);
#   define Ansi_Fname  ansi_name
#else
#   define Ansi_Fname  sExt
#endif

    gotTime = getNTfiletime(pFile->ctime, pFile->mtime, pFile->atime, &Modft, &Accft, &Creft);

    ulFileAttributes = ndb_osdep_mapNdbAttributes2OS(pFile->attributes);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_win32_SetPathAttrTimes: file attributes are NDB: 0x%lx/Win32: 0x%lX\n",
                        pFile->attributes, ulFileAttributes);
        fflush(stdout);
    }

    /* sfield@microsoft.com: set attributes before time in case we decide to
       support other filetime members later.  This also allows us to apply
       attributes before the security is changed, which may prevent this
       from succeeding otherwise.  Also, since most files don't have
       any interesting attributes, only change them if something other than
       NDB_FILEFLAG_ARCHIVE appears in the attributes.  This works well
       as an optimization because NDB_FILEFLAG_ARCHIVE gets applied to the
       file anyway, when it's created new.
    */
// 26-May-2004, pw: since creating files files now with open() instead fopen(),
//                  the files are READONLY per default
//                  -> therefore we need to set the attributes each time
//    if((pFile->attributes & 0x7F) & ~NDB_FILEFLAG_ARCHIVE)
    {
        if (!SetFileAttributes(Ansi_Fname, ulFileAttributes & 0x7F))
        {
            fprintf(stdout, "ndb_win32_SetPathAttrTimes: Error (%d): could not set file attributes for '%s'\n",
                    (int) GetLastError(), pFile->filenameExt);
            fflush(stdout);
        }
    }

    /* open a handle to the file before processing extra fields;
       we do this in case new security on file prevents us from updating
       time stamps */
    if((pFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
    {
        // get handle for directory
        hFile = CreateFile(Ansi_Fname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
         OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    }
    else
    {
        // get handle for file
        hFile = CreateFile(Ansi_Fname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        fprintf(stdout, "\nWarning: error (%d) setting file time for '%s'\n",
                (int)GetLastError(), Ansi_Fname);
        fflush(stdout);
    }
    else
    {
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_win32_SetPathAttrTimes: trying to set file times\n");
            fflush(stdout);
        }
        if ((gotTime > 0) && !SetFileTime(hFile, &Creft, &Accft, &Modft))
        {
            fprintf(stdout, "ndb_win32_SetPathAttrTimes: Error (%d): could not set file times for '%s'\n",
                            (int)GetLastError(), pFile->filenameExt);
            fflush(stdout);
        }
        CloseHandle(hFile);
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_SetPathAttrTimes - finished\n");
    fflush(stdout);

    return;

#undef Ansi_Fname
}

/* ------------------------------------------------------------------------*/

#if (defined(__EMX__) || defined(__CYGWIN__))
#  define MKDIR(path,mode)   mkdir(path,mode)
#else
#  define MKDIR(path,mode)   mkdir(path)
#endif

/*
    *************************************************************************************
        ndb_win32_create_path()

        strip filename from <filewithpath> and create the path directory including
        parent directories
        if filewithpath is a directory (withput file), it has to end with a trailing
        slash!
    *************************************************************************************
*/

int ndb_win32_create_path(const char *filewithpath)
{
    struct stat statFile;
    char sDummy[NDB_MAXLEN_FILENAME];
    char sExtPath[NDB_MAXLEN_FILENAME];
    char *p = NULL;
        char *q = NULL;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_create_path - startup\n");
    fflush(stdout);

    // make external
    strcpy (sExtPath, ndb_osdep_makeExtFileName(filewithpath));

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_win32_create_path: strip filename from '%s'\n", sExtPath);
    fflush(stdout);

    // strip filename (try both delimiter versions)
        // this fails if a pure directory doesn't end with a trailing slash!
    p = strrchr (sExtPath, '\\');
    if (p == NULL) p = strrchr (sExtPath, '/');
    if (NULL != p)
    {
        *(++p) = '\0'; // keep '\' as last char
    }
    else
    {
        // no '\' found -> file without path
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_win32_create_path: file '%s' has no path\n", sExtPath);
        fflush(stdout);
        return retval;
    }

    // stat() doesn't like trailing '\'
    strcpy (sDummy, sExtPath);
    if (sDummy[strlen(sDummy) - 1] == '\\')
    {
            sDummy[strlen(sDummy) - 1] = '\0';
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_win32_create_path: checking '%s' with stat()\n", sDummy);
    fflush(stdout);

    // first check if path already exists
    if (LSSTAT(sDummy, &statFile) != -1)
    {
        // path already exists
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_win32_create_path: path '%s' already exists\n", sDummy);
        fflush(stdout);
    }
    else
    {
        // path does not exist, so try to create it
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_win32_create_path: trying to create dir '%s'\n", sExtPath);
        fflush(stdout);

        q = NULL;
        p = sExtPath;
        q = strchr(p, '\\');
        // check against something like 'c:'
        if ((q >= p + 2) && ((isalpha((unsigned char) q[-2])) && (q[-1] == ':')))
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_win32_create_path: dir '%s' starts with device\n", sExtPath);
            fflush(stdout);
            // does sExtPath look like 'c:\<..>'?
            if (q == p + 2)
            {
                // skip first '\' also
                q = strchr(q + 1, '\\');
            }
        }
        // does sExtPath start with an trailing slash?
        if (q == p)
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_win32_create_path: dir '%s' starts with backslash\n", sExtPath);
            fflush(stdout);
            // double '\\', i.e. start of an UNC name like '\\<server>\<share>\<...>'?
            if (q[1] == '\\')
            {
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_win32_create_path: start of UNC name found in dir '%s'\n", sExtPath);
                fflush(stdout);
                // look for the begin of the <share> part
                q = strchr(q + 2, '\\');
                if (q == NULL)
                {
                    // malformed path
                    fprintf(stderr, "ndb_win32_create_path: cannot create malformed path '%s'\n", sExtPath);
                    fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
                }
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_win32_create_path: path after stripping \\\\<server> is '%s'\n", q);
                fflush(stdout);
                // look for the begin of the path after the <share> part
                q = strchr(q + 1, '\\');
                if (q == NULL)
                {
                    // malformed path
                    fprintf(stderr, "ndb_win32_create_path: cannot create malformed path '%s'\n", sExtPath);
                    fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
                }
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_win32_create_path: path after stripping \\<share> is '%s'\n", q);
                fflush(stdout);
                // skip leading backslash
                q = strchr(q + 1, '\\');
            }
            else
            {
                q = strchr(p + 1, '\\');
            }
        }
        // break if no other '\' can be found
        while (q != NULL)
        {
            // copy path from start to the currently found '\'
            strncpy (sDummy, sExtPath, q - sExtPath);
            sDummy[q - sExtPath] = '\0';
            // debug output
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_win32_create_path: checking path '%s' for existence\n", sDummy);
            fflush(stdout);
            // create the parent directories only if neccessary
            if (LSSTAT(sDummy, &statFile) != -1)
            {
                // path already exists
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_win32_create_path: path '%s' already exists\n", sDummy);
                fflush(stdout);
            }
            else
            {
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_win32_create_path: call mkdir(..) for path '%s'\n", sDummy);
                fflush(stdout);
                // creation is neccessary
                if (-1 == MKDIR(sDummy, 0777))
                {
                    fprintf(stderr, "ndb_win32_create_path: Error: couldn't create dir '%s'\n", sDummy);
                    fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
                }
            }
            // set p to the first char after the current '\'
            p = q + 1;
            q = strchr(p, '\\');
        }
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_create_path - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
        ndb_win32_create_outfile()

        adapted from Infozip's unzip

        creates the file <filewithpath>
    *************************************************************************************
*/

void ndb_win32_create_outfile(const char *filewithpath)
{
    HANDLE hFile;      /* File handle defined in NT    */
    char sExtFilename[NDB_MAXLEN_FILENAME];

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_create_outfile - startup\n");
    fflush(stdout);

    strcpy (sExtFilename, ndb_osdep_makeExtFileName(filewithpath));

#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
    char *ansi_name = (char *)alloca(strlen(sExtFilename) + 1);

    INTERN_TO_ISO(sExtFilename, ansi_name);
#   define Ansi_Fname  ansi_name
#else
#   define Ansi_Fname  sExtFilename
#endif

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_create_outfile: try to create file '%s'\n", Ansi_Fname);
    fflush(stdout);

    /* clear last erro */
    GetLastError();

    /* try to create the file */
    hFile = CreateFile(Ansi_Fname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        fprintf(stdout, "ndb_win32_create_outfile: CreateFile error (%d) for '%s'\n",
                (int)GetLastError(), Ansi_Fname);
        fflush(stdout);
    }
    else
    {
        CloseHandle(hFile);
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_create_outfile - finished\n");
    fflush(stdout);

    return;

#undef Ansi_Fname
}

/*
    *************************************************************************************
        ndb_win32_sleep()

        lets the application sleep for <millis> milliseconds (to be multi tasking
        friendly regarding CPU/IO/net usage
    *************************************************************************************
*/

void ndb_win32_sleep(int millis)
{
        DWORD dwMillis = 0;
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_sleep: now waiting for %d milliseconds\n", millis);
    fflush(stdout);
    dwMillis = millis & 0xfff;
    Sleep(dwMillis);
}

/*
    *************************************************************************************
        ndb_win32_getCodepage()

        asks for the current code page
    *************************************************************************************
*/

#define streq(s1,s2) (!strcmp(s1,s2))

char* locale_charset = NULL;

char *ndb_win32_getCodepage()
{
    // code extrated from Bruno Haible's macros/texinfo/texinfo/intl/localcharset.c
    char buf[2 + 10 + 1];
    /* Win32 has a function returning the locale's codepage as a number.  */
    snprintf (buf, 13 - 1, "CP%u", GetACP ());
    locale_charset = buf;

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_win32_getCodepage: result is '%s'\n",
                                                    locale_charset);
    fflush(stdout);

    return locale_charset;
}

/*
    *************************************************************************************
        ndb_win32_init_upper()

        adapted source from Infozips Zip

        inits the table for codepage dependend case conversion of strings
    *************************************************************************************
*/

unsigned char upper[256];          /* Country dependent case map table */
unsigned char lower[256];

void ndb_win32_init_upper()
{
  unsigned int c;
  for (c = 0; c < sizeof(upper); c++) upper[c] = lower[c] = (unsigned char)c;
  for (c = 'a'; c <= 'z';        c++) upper[c] = (unsigned char)(c - 'a' + 'A');
  for (c = 'A'; c <= 'Z';        c++) lower[c] = (unsigned char)(c - 'A' + 'a');
}

/*
    *************************************************************************************
        ndb_win32_StringLower()

        adapted source from Infozips Zip

        returns string <szArg> as lowercase string
    *************************************************************************************
*/

char *ndb_win32_StringLower(char *szArg)
{
  unsigned char *szPtr;
  for (szPtr = (unsigned char *) szArg; *szPtr; szPtr++)
    *szPtr = lower[*szPtr];
  return szArg;
}

/*
    *************************************************************************************
        ndb_win32_ansi2oem()

        converts a string from ansi (Win codepage) to OEM (DOS codepage)

        return <filename> converted to OEM codepage
    *************************************************************************************
*/

char filenameOEM[NDB_MAXLEN_FILENAME];

char *ndb_win32_ansi2oem(char *filename)
{
    AnsiToOem(filename, filenameOEM);
    return filenameOEM;
}

/* ------------------------------------------------------------------------*/


ULONG ndb_win32_ndbattr2winattr(ULONG ndbattr)
{
    ULONG ulWinAttr =
           (ndbattr & NDB_FILEFLAG_RONLY   ? FILE_ATTRIBUTE_READONLY   : 0)
         | (ndbattr & NDB_FILEFLAG_HIDDEN  ? FILE_ATTRIBUTE_HIDDEN     : 0)
         | (ndbattr & NDB_FILEFLAG_SYSTEM  ? FILE_ATTRIBUTE_SYSTEM     : 0)
         | (ndbattr & NDB_FILEFLAG_DIR     ? FILE_ATTRIBUTE_DIRECTORY  : 0)
         | (ndbattr & NDB_FILEFLAG_ARCHIVE ? FILE_ATTRIBUTE_ARCHIVE    : 0);

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_win32_ndbattr2winattr: ndb attr 0x%lX, win attr 0x%lX\n", ndbattr, ulWinAttr);
        fflush(stdout);
    }

    return ulWinAttr;
}


ULONG ndb_win32_winattr2ndbattr(ULONG winattr)
{
    ULONG ulNdbAttr =
           (winattr & FILE_ATTRIBUTE_READONLY  ? NDB_FILEFLAG_RONLY   : 0)
         | (winattr & FILE_ATTRIBUTE_HIDDEN    ? NDB_FILEFLAG_HIDDEN  : 0)
         | (winattr & FILE_ATTRIBUTE_SYSTEM    ? NDB_FILEFLAG_SYSTEM  : 0)
         | (winattr & FILE_ATTRIBUTE_DIRECTORY ? NDB_FILEFLAG_DIR     : 0)
         | (winattr & FILE_ATTRIBUTE_ARCHIVE   ? NDB_FILEFLAG_ARCHIVE : 0);

    if (ndb_getDebugLevel() >= 7)
    {
            fprintf(stdout, "ndb_win32_winattr2ndbattr: win attr 0x%lX, ndb attr 0x%lX\n", winattr, ulNdbAttr);
        fflush(stdout);
    }

    return ulNdbAttr;
}


/********************************/
/* Function ndb_win32_isWinNT() */
/* unmodified source from       */
/* Infozips unzip               */
/********************************/

int ndb_win32_isWinNT(void)       /* returns TRUE if real NT, FALSE if Win95 or Win32s */
{
    static DWORD g_PlatformId = 0xFFFFFFFF; /* saved platform indicator */
        static DWORD dwWinVersion = 0L;

    if (g_PlatformId == 0xFFFFFFFF)
    {
        /* note: GetVersionEx() doesn't exist on WinNT 3.1 */
                dwWinVersion = GetVersion();
        if (dwWinVersion < 0x80000000)
        {
            g_PlatformId = TRUE;
        }
        else
        {
            g_PlatformId = FALSE;
        }
    }
    return (int)g_PlatformId;
}



/********************************/
/* Function ndb_win32_isWinXP() */
/********************************/

int ndb_win32_isWinXP()       /* returns TRUE if real XP, FALSE if other */
{
    static DWORD g_PlatformId = 0xF1F1F1F1; /* saved platform indicator */
        static DWORD dwWinVersion = 0L;

    if (g_PlatformId == 0xF1F1F1F1)
    {
        dwWinVersion = GetVersion();
        if (dwWinVersion < 0x80000000)
        {
            // major version = 5 says that OS is W2K or newer
            if ((dwWinVersion & 0xff) == 5)
            {
                // minor version > 0 isp W2K or newer
                if (((dwWinVersion >> 8) & 0xff) > 0)
                {
                    g_PlatformId = TRUE;
                }
                else
                {
                    g_PlatformId = FALSE;
                }
            }
        }
        else
        {
            g_PlatformId = FALSE;
        }
    }
    return (int)g_PlatformId;
}



/*
    *************************************************************************************
        ndb_win32_getWindowsVersion()

        returns a string with the (hopefully) current windows version
    *************************************************************************************
*/

char sWinVersion[40] = "\0";

char *ndb_win32_getWindowsVersion()
{
    DWORD dwWinVersion = 0L;

    /* note: GetVersionEx() doesn't exist on WinNT 3.1 */
    dwWinVersion = GetVersion();
    if (dwWinVersion < 0x80000000)
    {
        if ((dwWinVersion & 0xff) == 3)
                snprintf(sWinVersion, 39, "Windows NT SP%ld", (dwWinVersion >> 8) & 0xff);
        else if ((dwWinVersion & 0xff) == 4)
                snprintf(sWinVersion, 39, "Windows 2000 SP%ld", (dwWinVersion >> 8) & 0xff);
        else if ((dwWinVersion & 0xff) == 5)
        {
            if (((dwWinVersion >> 8) & 0xff) == 0)
                snprintf(sWinVersion, 39, "Windows 2000");
            else if (((dwWinVersion >> 8) & 0xff) == 1)
                snprintf(sWinVersion, 39, "Windows XP");
            else if (((dwWinVersion >> 8) & 0xff) == 2)
                snprintf(sWinVersion, 39, "Windows 2003");
        }
        else
            snprintf(sWinVersion, 39, "unknown (%ld SP%ld)", dwWinVersion & 0xff, (dwWinVersion >> 8) & 0xff);
    }
    else
    {
        if ((dwWinVersion & 0xff) == 3)
            strcpy(sWinVersion, "Windows 3.x");
        else if ((dwWinVersion & 0xff) == 4)
            strcpy(sWinVersion, "Windows 9x/ME");
        else
            snprintf(sWinVersion, 39, "unknown (%ld/%ld)", dwWinVersion & 0xff, (dwWinVersion >> 8) & 0xfff);
    }

    return sWinVersion;
}



/*
    *************************************************************************************
        ndb_win32_getShortName()

        get 8.3 name for given long name

        Returns: NDB status
    *************************************************************************************
*/

char shortname[NDB_MAXLEN_FILENAME];

char *ndb_win32_getShortName(char *name)
{
    DWORD dwResult;
    char *p, *q;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_getShortName - startup\n");

    dwResult = GetShortPathName(name, shortname, NDB_MAXLEN_FILENAME - 1);

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_win32_getShortName: calling GetShortPathName(..) = %lu\n", dwResult);
        fprintf(stdout, "    long  name is '%s'\n", name);
        fprintf(stdout, "    short name is '%s'\n", shortname);
        fflush(stdout);
    }

        // check if there is any difference between shortname and longname
        // (check only filename, no path!)
        p = strrchr(name, '\\');
        q = strrchr(shortname, '\\');
        if (p == NULL) p = name; else p++;
        if (q == NULL) q = shortname; else q++;
        // same string? then we don't need to archive the shortname
        if (strcmp(p, q) == 0)
                return NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_getShortName - finished\n");
    fflush(stdout);

    return (dwResult != 0) ? q : NULL;
}


/* FAT / NTFS detection */

int isFileSystemNTFS(char *dir)
{
        static char root[4] = "";
        static short result = 0;
        char vname[128];
        DWORD vnamesize = sizeof(vname);
        DWORD vserial;
        DWORD vfnsize;
        DWORD vfsflags;
        char vfsname[128];
        DWORD vfsnamesize = sizeof(vfsname);

        // do we have the same path as last time?
        // if so we can return the last result
        if ((strlen(dir) > 3) && (strncmp(root, dir, 3) == 0))
        {
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "\nisFileSystemNTFS: use cached result %d ('%s' same as '%s')\n", result, root, dir);
                fflush(stdout);
            }
            return result;
        }
        // do we have no drive and root is '\'?
        // if so we can return the last result
        if ((strlen(dir) > 3) &&
            ((dir[1] != ':') && (strcmp(root, "\\") == 0)))
        {
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "\nisFileSystemNTFS: use cached result %d (no drive at both '%s' and '%s')\n", result, root, dir);
                fflush(stdout);
            }
            return result;
        }

        strncpy(root, dir, 3);
        if ( isalpha((unsigned char)root[0]) && (root[1] == ':') )
        {
            root[0] = toupper(dir[0]);
            root[2] = '\\';
            root[3] = 0;
        }
        else
        {
            root[0] = '\\';
            root[1] = 0;
        }

        if ( !GetVolumeInformation(root, vname, vnamesize,
             &vserial, &vfnsize, &vfsflags,
             vfsname, vfsnamesize))
        {
            fprintf(stderr, "warning: GetVolumeInformation failed\n");
            result = 0;
            return(FALSE);
        }

        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "\nisFileSystemNTFS: '%s' filesystem for '%s'\n",
                            vfsname, dir);
            fflush(stdout);
        }

        result = (strcmp(vfsname, "NTFS") == 0);

        return result;
}


/*
    *************************************************************************************
	ndb_win32_truncate()

	truncate file at given size
    *************************************************************************************
*/

void ndb_win32_truncate(const char *pathfile, ULONG fileSize)
{
    HANDLE hFile;

     hFile = CreateFile(pathfile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE )
    {
        if (INVALID_SET_FILE_POINTER != SetFilePointer(hFile, fileSize, NULL, FILE_BEGIN))
        {
            SetEndOfFile(hFile);
        }
        CloseHandle(hFile);
    }
}


/*
    *************************************************************************************
	ndb_unix_version_local()

	Code from Info-Zip project; prints out the compiler, version and date of NDB
    *************************************************************************************
*/

void ndb_win32_version_local()
{
    static char CompiledWith[] = "Compiled with %s%s for %s%s%s%s.\n\n";
#if (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__DJGPP__))
    char buf[80];
#if (defined(_MSC_VER) && (_MSC_VER > 900))
    char buf2[80];
#endif
#endif

    printf(CompiledWith,

#if defined(_MSC_VER)  /* MSC == MSVC++, including the SDK compiler */
      (sprintf(buf, "Microsoft C %d.%02d ", _MSC_VER/100, _MSC_VER%100), buf),
#  if (_MSC_VER == 800)
        "(Visual C++ v1.1)",
#  elif (_MSC_VER == 850)
        "(Windows NT v3.5 SDK)",
#  elif (_MSC_VER == 900)
        "(Visual C++ v2.x)",
#  elif (_MSC_VER > 900)
        (sprintf(buf2, "(Visual C++ v%d.%d)", _MSC_VER/100 - 6,
                 _MSC_VER%100/10), buf2),
#  else
        "(bad version)",
#  endif
#elif defined(__WATCOMC__)
#  if (__WATCOMC__ % 10 > 0)
/* We do this silly test because __WATCOMC__ gives two digits for the  */
/* minor version, but Watcom packaging prefers to show only one digit. */
        (sprintf(buf, "Watcom C/C++ %d.%02d", __WATCOMC__ / 100,
                 __WATCOMC__ % 100), buf), "",
#  else
        (sprintf(buf, "Watcom C/C++ %d.%d", __WATCOMC__ / 100,
                 (__WATCOMC__ % 100) / 10), buf), "",
#  endif /* __WATCOMC__ % 10 > 0 */
#elif defined(__TURBOC__)
#  ifdef __BORLANDC__
     "Borland C++",
#    if (__BORLANDC__ == 0x0452)   /* __BCPLUSPLUS__ = 0x0320 */
        " 4.0 or 4.02",
#    elif (__BORLANDC__ == 0x0460)   /* __BCPLUSPLUS__ = 0x0340 */
        " 4.5",
#    elif (__BORLANDC__ == 0x0500)   /* __TURBOC__ = 0x0500 */
        " 5.0",
#    elif (__BORLANDC__ == 0x0520)   /* __TURBOC__ = 0x0520 */
        " 5.2 (C++ Builder)",
#    else
        " later than 5.2",
#    endif
#  else /* !__BORLANDC__ */
     "Turbo C",
#    if (__TURBOC__ >= 0x0400)     /* Kevin:  3.0 -> 0x0401 */
        "++ 3.0 or later",
#    elif (__TURBOC__ == 0x0295)     /* [661] vfy'd by Kevin */
        "++ 1.0",
#    endif
#  endif /* __BORLANDC__ */
#elif defined(__GNUC__)
#  ifdef __RSXNT__
#    if (defined(__DJGPP__) && !defined(__EMX__))
      (sprintf(buf, "rsxnt(djgpp v%d.%02d) / gcc ",
        __DJGPP__, __DJGPP_MINOR__), buf),
#    elif defined(__DJGPP__)
      (sprintf(buf, "rsxnt(emx+djgpp v%d.%02d) / gcc ",
        __DJGPP__, __DJGPP_MINOR__), buf),
#    elif (defined(__GO32__) && !defined(__EMX__))
      "rsxnt(djgpp v1.x) / gcc ",
#    elif defined(__GO32__)
      "rsxnt(emx + djgpp v1.x) / gcc ",
#    elif defined(__EMX__)
      "rsxnt(emx)+gcc ",
#    else
      "rsxnt(unknown) / gcc ",
#    endif
#  elif defined(__CYGWIN__)
      "Cygnus win32 / gcc ",
#  elif defined(__MINGW32__)
      "mingw32 / gcc ",
#  else
      "gcc ",
#  endif
      __VERSION__,
#elif defined(__LCC__)
      "LCC-Win32", "",
#else
      "unknown compiler (SDK?)", "",
#endif

      "\nWindows 9x / Windows NT", " (32-bit)",

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    return;

} /* end function version_local() */
