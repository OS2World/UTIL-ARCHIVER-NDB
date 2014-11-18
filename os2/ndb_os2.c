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
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <io.h>
#include <sys/types.h>
#include <locale.h>


#define INCL_NOPM
#define INCL_DOS
#define INCL_DOSNLS
#define INCL_DOSDATETIME
#include <os2.h>

#if defined(__GNUC__)
#include <os2emx.h>
#endif
#if defined(__WATCOMC__)
#include <direct.h>
#endif

#include "ndb_os2.h"

#include "../ndb.h"
#include "../ndb_prototypes.h"


#define PAD           0
#define PATH_END      '/'
#define HIDD_SYS_BITS (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

/*
  #define DosFindFirst(p1, p2, p3, p4, p5, p6) \
          DosFindFirst(p1, p2, p3, p4, p5, p6, 1)
*/

ULONG DosSleep (ULONG);

// int hidden_files;        /* process hidden and system files */
static int attributes = _A_SUBDIR | _A_HIDDEN | _A_SYSTEM;

static char *ndb_os2_getdirent(char *);
static void ndb_os2_free_dircontents(struct _dircontents *);

OS2DIR *ndb_os2_opendir(const char *, USHORT);
void ndb_os2_closedir(OS2DIR *);
char *ndb_os2_readd(OS2DIR *);
struct os2dirent *ndb_os2_readdir(OS2DIR *);

int GetFileMode(char *);
ULONG ndb_os2_getFileMode(char *);
ULONG ndb_os2_filetime(char *, ULONG *, ULONG *, iztimes *);

int ndb_os2_IsFileNameValid(char *);
int ndb_os2_IsFileSystemFAT(char *);
void ndb_os2_ChangeNameForFAT(char *);

char *ndb_os2_GetLongNameEA(char *);
char *ndb_os2_GetLongPathEA(char *);
void ndb_os2_GetEAs(char *, char **, size_t *, char **, size_t *);

char *ndb_os2_StringLower(char *);



static HDIR hdir;
static ULONG count;
static FILEFINDBUF3 find;

/*
    *************************************************************************************
        ndb_os2_opendir()

        adapted source from Infozips Zip, but not really understood ;-)

        checks (and modifies) the path and returns a dir* object (or NULL)
    *************************************************************************************
*/

OS2DIR *ndb_os2_opendir(const char *name, USHORT hidden)
{
  struct stat statb;
  OS2DIR *dirp;
  char c;
  char *s;
  struct _dircontents *dp;
  char nbuf[MAXPATHLEN + 1];
  int len;

  attributes = hidden ? (_A_SUBDIR | _A_HIDDEN | _A_SYSTEM) : _A_SUBDIR;

  if (ndb_getDebugLevel() >= 5)
     fprintf(stdout, "ndb_os2_opendir - startup (with '%s')\n", name);
  fflush(stdout);

  strcpy(nbuf, name);
  if ((len = strlen(nbuf)) == 0)
    return NULL;

  if (((c = nbuf[len - 1]) == '\\' || c == '/') && (len > 1))
  {
    nbuf[len - 1] = 0;
    --len;

    if (nbuf[len - 1] == ':')
    {
      strcpy(nbuf+len, "\\.");
      len += 2;
    }
  }
  else
    if (nbuf[len - 1] == ':')
    {
      strcpy(nbuf+len, ".");
      ++len;
    }

  if (ndb_getDebugLevel() >= 5)
     fprintf(stdout, "ndb_os2_opendir: dir modified to '%s'\n", nbuf);
  fflush(stdout);

  if (LSSTAT(nbuf, &statb) < 0 || (statb.st_mode & S_IFMT) != S_IFDIR)
    return NULL;

  if ((dirp = malloc(sizeof(OS2DIR))) == NULL)
    return NULL;

  if (nbuf[len - 1] == '.' && (len == 1 || nbuf[len - 2] != '.'))
    strcpy(nbuf+len-1, "*.*");
  else
    if (((c = nbuf[len - 1]) == '\\' || c == '/') && (len == 1))
      strcpy(nbuf+len, "*");
    else
      strcpy(nbuf+len, "\\*");

  /* len is no longer correct (but no longer needed) */

  dirp -> dd_loc = 0;
  dirp -> dd_contents = dirp -> dd_cp = NULL;

  if ((s = ndb_os2_getdirent(nbuf)) == NULL)
    return dirp;

  do
  {
    if (((dp = malloc(sizeof(struct _dircontents))) == NULL) ||
        ((dp -> _d_entry = malloc(strlen(s) + 1)) == NULL)      )
    {
      if (dp)
        free(dp);
      ndb_os2_free_dircontents(dirp -> dd_contents);

      return NULL;
    }

    if (dirp -> dd_contents)
    {
      dirp -> dd_cp -> _d_next = dp;
      dirp -> dd_cp = dirp -> dd_cp -> _d_next;
    }
    else
      dirp -> dd_contents = dirp -> dd_cp = dp;

    strcpy(dp -> _d_entry, s);
    dp -> _d_next = NULL;

    dp -> _d_size = find.cbFile;
    dp -> _d_mode = find.attrFile;
    dp -> _d_time = *(unsigned *) &(find.ftimeLastWrite);
    dp -> _d_date = *(unsigned *) &(find.fdateLastWrite);
  }
  while ((s = ndb_os2_getdirent(NULL)) != NULL);

  dirp -> dd_cp = dirp -> dd_contents;

  return dirp;
}

/*
    *************************************************************************************
        ndb_os2_closedir()

        adapted source from Infozips Zip

        close the dir* object and frees its memory
    *************************************************************************************
*/

void ndb_os2_closedir(OS2DIR * dirp)
{
  ndb_os2_free_dircontents(dirp -> dd_contents);
  free(dirp);
}

/*
    *************************************************************************************
        ndb_os2_readd()

        adapted source from Infozips Zip

        reads the next file of directory *d
    *************************************************************************************
*/

char *ndb_os2_readd(OS2DIR *d)
/* DIR *d: directory stream to read from */
/* Return a pointer to the next name in the directory stream d, or NULL if
   no more entries or an error occurs. */
{
  struct os2dirent *e;

  e = ndb_os2_readdir(d);
  return e == NULL ? (char *) NULL : e->d_name;
}

/*
    *************************************************************************************
        ndb_os2_readdir()

        source from Infozips Zip

        converts a dir object to a os2dirent object
    *************************************************************************************
*/

struct os2dirent *ndb_os2_readdir(OS2DIR * dirp)
{
  static struct os2dirent dp;

  if (dirp -> dd_cp == NULL)
    return NULL;

  dp.d_namlen = dp.d_reclen =
    strlen(strcpy(dp.d_name, dirp -> dd_cp -> _d_entry));

  dp.d_ino = 0;

  dp.d_size = dirp -> dd_cp -> _d_size;
  dp.d_mode = dirp -> dd_cp -> _d_mode;
  dp.d_time = dirp -> dd_cp -> _d_time;
  dp.d_date = dirp -> dd_cp -> _d_date;

  dirp -> dd_cp = dirp -> dd_cp -> _d_next;
  dirp -> dd_loc++;

  return &dp;
}

/*
    *************************************************************************************
        ndb_os2_free_dircontents()

        source from Infozips Zip

        frees resources of _dircontents list
    *************************************************************************************
*/

static void ndb_os2_free_dircontents(struct _dircontents * dp)
{
  struct _dircontents *odp;

  while (dp)
  {
    if (dp -> _d_entry)
      free(dp -> _d_entry);

    dp = (odp = dp) -> _d_next;
    free(odp);
  }
}

/*
    *************************************************************************************
        ndb_os2_getdirent()

        adapted source from Infozips Zip

        returns the next file of directory <dir>
    *************************************************************************************
*/

static char *ndb_os2_getdirent(char *dir)
{
  int done;
  static int lower;

  if (dir != NULL)
  {                                    /* get first entry */
    hdir = HDIR_SYSTEM;
    count = 1;
    done = DosFindFirst(dir, &hdir, attributes, &find, sizeof(find), &count, 1);
    lower = ndb_os2_IsFileSystemFAT(dir);
  }
  else                                 /* get next entry */
    done = DosFindNext(hdir, &find, sizeof(find), &count);

  if (done == 0)
  {
    if (lower)
      ndb_os2_StringLower(find.achName);
    return find.achName;
  }
  else
  {
    DosFindClose(hdir);
    return NULL;
  }
}

/*
    *************************************************************************************
        GetFileMode()

        source from Infozips Zip

        returns OS/2 file attributes of file <name>
    *************************************************************************************
*/

/* access mode bits and time stamp */

int GetFileMode(char *name)
{
  FILESTATUS3 fs;
  return DosQueryPathInfo(name, 1, &fs, sizeof(fs)) ? -1 : fs.attrFile;
}

/*
    *************************************************************************************
        GetFileTime()

        adapted source from Infozips Zip

        returns the file time (mtime) of file <name>
    *************************************************************************************
*/

ULONG GetFileTime(char *name)
{
  FILESTATUS3 fs;
  USHORT nDate, nTime;
  DATETIME dtCurrent;

  if (strcmp(name, "-") == 0)
  {
    DosGetDateTime(&dtCurrent);
    fs.fdateLastWrite.day     = dtCurrent.day;
    fs.fdateLastWrite.month   = dtCurrent.month;
    fs.fdateLastWrite.year    = dtCurrent.year - 1980;
    fs.ftimeLastWrite.hours   = dtCurrent.hours;
    fs.ftimeLastWrite.minutes = dtCurrent.minutes;
    fs.ftimeLastWrite.twosecs = dtCurrent.seconds / 2;
  }
  else
    if (DosQueryPathInfo(name, 1, (PBYTE) &fs, sizeof(fs)))
      return -1;

  nDate = * (USHORT *) &fs.fdateLastWrite;
  nTime = * (USHORT *) &fs.ftimeLastWrite;

  return ((ULONG) nDate) << 16 | nTime;
}

/*
    *************************************************************************************
        ndb_os2_getFileMode()

        adapted source from Infozips Zip

        returns the file attributes, converted to NDB flag bits, of file <name>
    *************************************************************************************
*/

ULONG ndb_os2_getFileMode(char *name)
{
    ULONG ulAttr;
    struct stat s_stat;        /* results of stat() */
    char newname[NDB_MAXLEN_FILENAME];
    int len;


    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_getFileMode - startup (with '%s')\n", name);
    fflush(stdout);

    if ((name == NULL) || (name[0] == '\0'))
    {
        fprintf(stdout, "ndb_os2_getFileMode: internal error: filename is NULL or empty\n");
        fflush(stdout);
        return 0;
    }

    strcpy(newname, name);
    len = strlen(newname);
    if (newname[len - 1] == '/')
            newname[len - 1] = '\0';
    /* not all systems allow stat'ing a file with / appended */

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_os2_getFileMode: before LSSTAT()\n");
    fflush(stdout);

   if (LSSTAT(newname, &s_stat) != 0)
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_os2_getFileMode: LSSTAT() returned with error\n");
        fflush(stdout);
              /* Accept about any file kind including directories
               * (stored with trailing / with -r option)
               */
         return 0;
    }


    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_os2_getFileMode: after LSSTAT()\n");
    fflush(stdout);

    ulAttr = ((ULONG) s_stat.st_mode << 16) | (ULONG) GetFileMode(newname);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_getFileMode - finished (with ulAttr 0x%lX)\n", ulAttr);
    fflush(stdout);

    return
        (
          (ulAttr & _A_RDONLY    ? NDB_FILEFLAG_RONLY   : 0)
        | (ulAttr & _A_HIDDEN    ? NDB_FILEFLAG_HIDDEN  : 0)
        | (ulAttr & _A_SYSTEM    ? NDB_FILEFLAG_SYSTEM  : 0)
        | (ulAttr & _A_SUBDIR    ? NDB_FILEFLAG_DIR     : 0)
        | (ulAttr & _A_ARCHIVE   ? NDB_FILEFLAG_ARCHIVE : 0)
        );
}

/*
    *************************************************************************************
        ndb_os2_filetime()

        adapted source from Infozips Zip

        returns the file attributes (OS/2 style), the file size and the
        file times (ctime, mtime, atime)
    *************************************************************************************
*/

ULONG ndb_os2_filetime(filename, attrib, size, t)
char *filename;          /* name of file to get info on */
ULONG *attrib;           /* return value: file attributes */
ULONG *size;             /* return value: file size */
iztimes *t;              /* return value: access, modific. and creation times */
/* If file *filename does not exist, return 0.  Else, return the file's last
   modified date and time as an MSDOS date and time.  The date and
   time is returned in a long with the date most significant to allow
   unsigned integer comparison of absolute times.  Also, if a is not
   a NULL pointer, store the file attributes there, with the high two
   bytes being the Unix attributes, and the low byte being a mapping
   of that to DOS attributes.  If size is not NULL, store the file size
   there.  If t is not NULL, the file's access, modification and creation
   times are stored there as UNIX time_t values.
*/
{
        struct stat s_stat;        /* results of stat() */
        char name[NDB_MAXLEN_FILENAME];
        int len = strlen(filename);

        strcpy(name, filename);
        if (name[len - 1] == '/')
                name[len - 1] = '\0';
        /* not all systems allow stat'ing a file with / appended */

        if (LSSTAT(name, &s_stat) != 0)
        {
            /* Accept about any file kind including directories
             * (stored with trailing / with -r option)
            */
            return 0L;
        }

        if (attrib != NULL)
        {
                *attrib = ((ULONG) s_stat.st_mode << 16) | (ULONG) GetFileMode(name);
        }
        if (size != NULL)
                *size = (s_stat.st_mode & S_IFMT) == S_IFREG ? s_stat.st_size : -1L;
        if (t != NULL)
        {
                t->atime = s_stat.st_atime;
                t->mtime = s_stat.st_mtime;
                t->ctime = s_stat.st_ctime;
        }

        return GetFileTime(name);
}

/*
    *************************************************************************************
        ndb_os2_create_path()

        strip filename from <filewithpath> and create the path directory including
        parent directories
        if filewithpath is a directory (withput file), it has to end with a trailing
        slash!
    *************************************************************************************
*/

#if defined(__GNUC__)
#define MKDIR(path,mode)   mkdir(path,mode)
#endif
#if defined(__WATCOMC__)
#define MKDIR(path,mode)   mkdir(path)
#endif

int ndb_os2_create_path(const char *filewithpath)
{
    struct stat statFile;
    char sDummy[NDB_MAXLEN_FILENAME];
    char sExtPath[NDB_MAXLEN_FILENAME];
    char *p = NULL;
    char *q = NULL;

    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_create_path - startup\n");
    fflush(stdout);

    // make external
    strcpy (sExtPath, ndb_osdep_makeExtFileName(filewithpath));

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_os2_create_path: strip filename from '%s'\n", sExtPath);
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
            fprintf(stdout, "ndb_os2_create_path: file '%s' has no path\n", sExtPath);
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
        fprintf(stdout, "ndb_os2_create_path: checking '%s' with stat()\n", sDummy);
    fflush(stdout);

    // first check if path already exists
    if (LSSTAT(sDummy, &statFile) != -1)
    {
        // path already exists
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_os2_create_path: path '%s' already exists\n", sDummy);
        fflush(stdout);
    }
    else
    {
        // path does not exist, so try to create it
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_os2_create_path: trying to create dir '%s'\n", sExtPath);
        fflush(stdout);

        q = NULL;
        p = sExtPath;
        q = strchr(p, '\\');
        // check against something like 'c:'
        if ((q >= p + 2) && ((isalpha((unsigned char) q[-2])) && (q[-1] == ':')))
        {
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_os2_create_path: dir '%s' starts with device\n", sExtPath);
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
                    fprintf(stdout, "ndb_os2_create_path: dir '%s' starts with backslash\n", sExtPath);
                fflush(stdout);
                        // double '\\', i.e. start of an UNC name like '\\<server>\<share>\<...>'?
                        if (q[1] == '\\')
                        {
                        if (ndb_getDebugLevel() >= 7)
                            fprintf(stdout, "ndb_os2_create_path: start of UNC name found in dir '%s'\n", sExtPath);
                        fflush(stdout);
                        // look for the begin of the <share> part
                        q = strchr(q + 2, '\\');
                        if (q == NULL)
                        {
                                        // malformed path
                            fprintf(stderr, "ndb_os2_create_path: cannot create malformed path '%s'\n", sExtPath);
                                fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
                        }
                        if (ndb_getDebugLevel() >= 7)
                            fprintf(stdout, "ndb_os2_create_path: path after stripping \\\\<server> is '%s'\n", q);
                        fflush(stdout);
                        // look for the begin of the path after the <share> part
                        q = strchr(q + 1, '\\');
                        if (q == NULL)
                        {
                                        // malformed path
                            fprintf(stderr, "ndb_os2_create_path: cannot create malformed path '%s'\n", sExtPath);
                                fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
                        }
                        if (ndb_getDebugLevel() >= 7)
                            fprintf(stdout, "ndb_os2_create_path: path after stripping \\<share> is '%s'\n", q);
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
                fprintf(stdout, "ndb_os2_create_path: checking path '%s' for existence\n", sDummy);
            fflush(stdout);
            // create the parent directories only if neccessary
            if (LSSTAT(sDummy, &statFile) != -1)
            {
                // path already exists
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_os2_create_path: path '%s' already exists\n", sDummy);
                fflush(stdout);
            }
            else
            {
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_os2_create_path: call mkdir(..) for path '%s'\n", sDummy);
                fflush(stdout);
                // creation is neccessary
                if (-1 == MKDIR(sDummy, 0777))
                {
                    fprintf(stderr, "ndb_os2_create_path: Error: couldn't create dir '%s'\n", sDummy);
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
        fprintf(stdout, "ndb_os2_create_path - finished\n");
    fflush(stdout);

    return retval;
}

/* ------------------------------------------------------------------------*/

/* The following DOS date/time structures are machine-dependent as they
 * assume "little-endian" byte order.  For OS/2-specific code, which
 * is run on x86 CPUs (or emulators?), this assumption is valid; but
 * care should be taken when using this code as template for other ports.
 */
typedef union {
  ULONG timevalue;          /* combined value, useful for comparisons */
  struct {
    FTIME ft;               /* system file time record:
                             *    USHORT twosecs : 5
                             *    USHORT minutes : 6;
                             *    USHORT hours   : 5;   */
    FDATE fd;               /* system file date record:
                             *    USHORT day     : 5
                             *    USHORT month   : 4;
                             *    USHORT year    : 7;   */
  } _fdt;
} F_DATE_TIME, *PF_DATE_TIME;

/*
    *************************************************************************************
        Utime2DosDateTime()

        adapted source from Infozips Zip

        converts a file time in unix format (time_t structure)
        to DOS format (ULONG, seconds since ?)
    *************************************************************************************
*/

static ULONG Utime2DosDateTime(uxtime)
    time_t uxtime;
{
    F_DATE_TIME dosfiletime;
    struct tm *t;

    /* round up to even seconds */
    /* round up (down if "up" overflows) to even seconds */
    if (((ULONG)uxtime) & 1)
        uxtime = (uxtime + 1 > uxtime) ? uxtime + 1 : uxtime - 1;

    t = localtime(&(uxtime));
    if (t == (struct tm *)NULL)
    {
        /* time conversion error; use current time instead, hoping
           that localtime() does not reject it as well! */
        time_t now = time(NULL);
        t = localtime(&now);
    }
    if (t->tm_year < 80)
    {
        dosfiletime._fdt.ft.twosecs = 0;
        dosfiletime._fdt.ft.minutes = 0;
        dosfiletime._fdt.ft.hours   = 0;
        dosfiletime._fdt.fd.day     = 1;
        dosfiletime._fdt.fd.month   = 1;
        dosfiletime._fdt.fd.year    = 0;
    } else
    {
        dosfiletime._fdt.ft.twosecs = t->tm_sec >> 1;
        dosfiletime._fdt.ft.minutes = t->tm_min;
        dosfiletime._fdt.ft.hours   = t->tm_hour;
        dosfiletime._fdt.fd.day     = t->tm_mday;
        dosfiletime._fdt.fd.month   = t->tm_mon + 1;
        dosfiletime._fdt.fd.year    = t->tm_year - 80;
    }
    return dosfiletime.timevalue;

} /* end function Utime2DosDateTime() */

/*
    *************************************************************************************
        ndb_os2_SetPathAttrTimes()

        adapted source from Infozips Zip

        set file attributes and file times (ctime, mtime, atime) for file
        <pFile->filenameExt> using the data in structure <pFile>
    *************************************************************************************
*/

void ndb_os2_SetPathAttrTimes(struct ndb_s_fileentry *pFile, const char *sDefaultPath)
{
  HFILE hFile;
  ULONG nAction;
  FILESTATUS fs;
  USHORT nLength;
  char szName[NDB_MAXLEN_FILENAME];
  int dir = 0;
  USHORT fd, ft;
  ULONG ulAtime, ulMtime, ulCtime;

  char sInt[NDB_MAXLEN_FILENAME];
  char sExt[NDB_MAXLEN_FILENAME];

  if (ndb_getDebugLevel() >= 5)
      fprintf(stdout, "ndb_os2_SetPathAttrTimes - startup\n");
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

  if (ndb_getDebugLevel() >= 5)
  {
      fprintf(stdout, "ndb_os2_SetPathAttrTimes: file attributes for '%s' are 0x%lx\n",
                      sExt, pFile->attributes);
      fflush(stdout);
  }

  strcpy(szName, sExt);
  nLength = strlen(szName);
  if (szName[nLength - 1] == '/')
    szName[nLength - 1] = 0;

  if ((pFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
  {
    dir = 1;
  }

  if (dir)
  {
    if ( DosQueryPathInfo(szName, FIL_STANDARD, (PBYTE) &fs, sizeof(fs)) )
      return;
  }
  else
  {
    /* for regular files, open them and operate on the file handle, to
       work around certain network operating system bugs ... */

    if ( DosOpen(szName, &hFile, &nAction, 0, 0,
                 OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,
                 OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE, 0) )
      return;

    if ( DosQueryFileInfo(hFile, FIL_STANDARD, (PBYTE) &fs, sizeof(fs)) )
      return;
  }

  if (ndb_getDebugLevel() >= 5)
  {
      fprintf(stdout, "ndb_os2_SetPathAttrTimes: setting file stamps and file attributes for '%s'\n", sExt);
      fflush(stdout);
  }

  /* set date/time stamps */
  ulAtime = Utime2DosDateTime(pFile->atime);
  ulMtime = Utime2DosDateTime(pFile->mtime);
  ulCtime = Utime2DosDateTime(pFile->ctime);

  fd  = ulMtime >> 16;
  ft  = ulMtime & 0xffff;
  fs.fdateLastWrite   = * (FDATE *) &fd;
  fs.ftimeLastWrite   = * (FTIME *) &ft;
  fd  = ulAtime >> 16;
  ft  = ulAtime & 0xffff;
  fs.fdateLastAccess  = * (FDATE *) &fd;
  fs.ftimeLastAccess  = * (FTIME *) &ft;
  fd  = ulCtime >> 16;
  ft  = ulCtime & 0xffff;
  fs.fdateCreation    = * (FDATE *) &fd;
  fs.ftimeCreation    = * (FTIME *) &ft;

  /* set file attributes */
  fs.attrFile = 0;
  if ((pFile->attributes & NDB_FILEFLAG_RONLY)  == NDB_FILEFLAG_RONLY)
    fs.attrFile += _A_RDONLY;
  if ((pFile->attributes & NDB_FILEFLAG_HIDDEN) == NDB_FILEFLAG_HIDDEN)
    fs.attrFile += _A_HIDDEN;
  if ((pFile->attributes & NDB_FILEFLAG_SYSTEM) == NDB_FILEFLAG_SYSTEM)
    fs.attrFile += _A_SYSTEM;

  if (dir)
  {
    DosSetPathInfo(szName, FIL_STANDARD, (PBYTE) &fs, sizeof(fs), 0);
  }
  else
  {
    DosSetFileInfo(hFile, FIL_STANDARD, (PBYTE) &fs, sizeof(fs));
    DosClose(hFile);
  }

  if (ndb_getDebugLevel() >= 5)
      fprintf(stdout, "ndb_os2_SetPathAttrTimes - finished\n");
  fflush(stdout);

}

/*
    *************************************************************************************
        ndb_os2_IsFileSystemFAT()

        adapted source from Infozips Zip

        checks whether <dir> is located on a 8.3 file system

        currently not used
    *************************************************************************************
*/

/* FAT / HPFS detection */

int ndb_os2_IsFileSystemFAT(char *dir)
{
  static USHORT nLastDrive = -1, nResult;
  ULONG lMap;
  BYTE bData[64];
  char bName[3];
  ULONG nDrive, cbData;
  PFSQBUFFER2 pData = (PFSQBUFFER2) bData;

  /* We separate FAT and HPFS+other file systems here.
     at the moment I consider other systems to be similar to HPFS,
     i.e. support long file names and being case sensitive */

  if (isalpha((int) dir[0]) && ((int) dir[1] == ':'))
    nDrive = to_up(dir[0]) - '@';
  else
    DosQueryCurrentDisk(&nDrive, &lMap);

  if (nDrive == nLastDrive)
    return nResult;

  bName[0] = (char) (nDrive + '@');
  bName[1] = ':';
  bName[2] = 0;

  nLastDrive = nDrive;
  cbData = sizeof(bData);

  if (!DosQueryFSAttach(bName, 0, FSAIL_QUERYNAME, (PVOID) pData, &cbData))
    nResult = !strcmp((char *) pData -> szFSDName + pData -> cbName, "FAT");
  else
    nResult = FALSE;

  /* End of this ugly code */
  return nResult;
}

/*
    *************************************************************************************
        ndb_os2_IsFileNameValid()

        adapted source from Infozips Zip

        checks whether <name> is suitable for current file system

        currently not used
    *************************************************************************************
*/

/* FAT / HPFS name conversion stuff */

/* eigentlich schon in os2emx.h, seltsamerweise aber
   kennt ndb_os2util.c diese Defines trotzdem nicht
*/

#define NO_ERROR                                0
#define ERROR_INVALID_NAME                    123
#define ERROR_FILENAME_EXCED_RANGE            206


int ndb_os2_IsFileNameValid(char *name)
{
  HFILE hf;
  ULONG uAction;

  switch(DosOpen(name, &hf, &uAction, 0, 0, FILE_OPEN,
                 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0))
  {
    case ERROR_INVALID_NAME:
    case ERROR_FILENAME_EXCED_RANGE:
      return FALSE;
    case NO_ERROR:
      DosClose(hf);
    default:
      return TRUE;
  }
}

/*
    *************************************************************************************
        ndb_os2_ChangeNameForFAT()

        adapted source from Infozips Zip

        converts file name <name> for a 8.3 file system

        currently not used
    *************************************************************************************
*/

void ndb_os2_ChangeNameForFAT(char *name)
{
  char *src, *dst, *next, *ptr, *dot, *start;
  static char invalid[] = ":;,=+\"[]<>| \t";

  if (isalpha((int) name[0]) && (name[1] == ':'))
    start = name + 2;
  else
    start = name;

  src = dst = start;
  if ((*src == '/') || (*src == '\\'))
  {
    src++;
    dst++;
  }

  while (*src)
  {
    for (next = src; *next && (*next != '/') && (*next != '\\'); next++);

    for (ptr = src, dot = NULL; ptr < next; ptr++)
      if (*ptr == '.')
      {
        dot = ptr; /* remember last dot */
        *ptr = '_';
      }

    if (dot == NULL)
      for (ptr = src; ptr < next; ptr++)
        if (*ptr == '_')
          dot = ptr; /* remember last _ as if it were a dot */

    if (dot && (dot > src) &&
        ((next - dot <= 4) ||
         ((next - src > 8) && (dot - src > 3))))
    {
      if (dot)
        *dot = '.';

      for (ptr = src; (ptr < dot) && ((ptr - src) < 8); ptr++)
        *dst++ = *ptr;

      for (ptr = dot; (ptr < next) && ((ptr - dot) < 4); ptr++)
        *dst++ = *ptr;
    }
    else
    {
      if (dot && (next - src == 1))
        *dot = '.';           /* special case: "." as a path component */

      for (ptr = src; (ptr < next) && ((ptr - src) < 8); ptr++)
        *dst++ = *ptr;
    }

    *dst++ = *next; /* either '/' or 0 */

    if (*next)
    {
      src = next + 1;

      if (*src == 0) /* handle trailing '/' on dirs ! */
        *dst = 0;
    }
    else
      break;
  }

  for (src = start; *src != 0; ++src)
    if ((strchr(invalid, *src) != NULL) || (*src == ' '))
      *src = '_';
}

/*
    *************************************************************************************
        ndb_os2_sleep()

        lets the application sleep for <millis> milliseconds (to be multi tasking
        friendly regarding CPU/IO/net usage
    *************************************************************************************
*/

void ndb_os2_sleep(int millis)
{
    ULONG ulMillis = millis & 0xfff;
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_sleep: now waiting for %d milliseconds\n", millis);
    fflush(stdout);
    DosSleep(ulMillis);
}

/*
    *************************************************************************************
        ndb_os2_getCodepage()

        asks for the current code page
    *************************************************************************************
*/

ULONG DosQueryCp (ULONG ulLength, PULONG pCodePageList, PULONG pDataLength);

static char locale_charset[10];

char *ndb_os2_getCodepage()
{
    /* We are connected to /dev/con. */
    ULONG cp[3]; /* list of codepages */
    ULONG cplen; /* number of available codepages in cp */

    if (DosQueryCp (sizeof (cp), cp, &cplen) == 0)
     {
        /* We could determine the current codepage. */
        snprintf (locale_charset, 10 - 1, "cp%lu", cp[0]);
        if (ndb_getDebugLevel() >= 3)
        {
            fprintf (stdout, "codepage is '%s'\n", locale_charset);
            fflush(stdout);
        }
     }
     else
        locale_charset[0] = '\0';

     return locale_charset;
}

/*
    *************************************************************************************
        ndb_os2_init_upper()

        adapted source from Infozips Zip

        inits the table for codepage dependend case conversion of strings
    *************************************************************************************
*/

/* Initialize the table of uppercase characters including handling of
   country dependent characters. */

#if defined(__WATCOMC__)
ULONG DosMapCase (ULONG ulLength, COUNTRYCODE *pCountryCode,
                  PCHAR pchString);
#else
ULONG DosMapCase (ULONG ulLength, __const__ COUNTRYCODE *pCountryCode,
                  PCHAR pchString);
#endif


static unsigned char cUpperCase[256], cLowerCase[256];
static BOOL bInitialized;

void ndb_os2_init_upper()
{
  unsigned nCnt, nU;
  COUNTRYCODE cc;

  bInitialized = TRUE;

  for ( nCnt = 0; nCnt < 256; nCnt++ )
    cUpperCase[nCnt] = cLowerCase[nCnt] = (unsigned char) nCnt;

  cc.country = cc.codepage = 0;
  DosMapCase(sizeof(cUpperCase), &cc, (PCHAR) cUpperCase);

  for ( nCnt = 0; nCnt < 256; nCnt++ )
  {
    nU = cUpperCase[nCnt];
    if (nU != nCnt && cLowerCase[nU] == (unsigned char) nU)
      cLowerCase[nU] = (unsigned char) nCnt;
  }

  for ( nCnt = 'A'; nCnt <= 'Z'; nCnt++ )
    cLowerCase[nCnt] = (unsigned char) (nCnt - 'A' + 'a');
}

/*
    *************************************************************************************
        IsLower()

        adapted source from Infozips Zip

        returns whether <c> is a lowercase letter
    *************************************************************************************
*/

int IsLower(int c)
{
  if ( !bInitialized )
    ndb_os2_init_upper();
  return (cUpperCase[c] != (unsigned char) c)
    || (c == 0xE1); /* special case, german "sz" */
}

/*
    *************************************************************************************
        IsUpper()

        adapted source from Infozips Zip

        returns whether <c> is a upercase letter
    *************************************************************************************
*/

int IsUpper(int c)
{
  if ( !bInitialized )
    ndb_os2_init_upper();
  return (cLowerCase[c] != (unsigned char) c);
}

/*
    *************************************************************************************
        ToLower()

        adapted source from Infozips Zip

        converts <c> to a lowercase letter
    *************************************************************************************
*/

int ToLower(int c)
{
  if ( !bInitialized )
    ndb_os2_init_upper();
  return cLowerCase[(unsigned char) c];
}

/*
    *************************************************************************************
        ToUpper()

        adapted source from Infozips Zip

        converts <c> to a upercase letter
    *************************************************************************************
*/

int ToUpper(int c)
{
  if ( !bInitialized )
    ndb_os2_init_upper();
  return cUpperCase[(unsigned char) c];
}

/*
    *************************************************************************************
        ndb_os2_StringLower()

        adapted source from Infozips Zip

        returns string <szArg> as lowercase string
    *************************************************************************************
*/

char *ndb_os2_StringLower(char *szArg)
{
  unsigned char *szPtr;
  for (szPtr = (unsigned char *) szArg; *szPtr; szPtr++)
    *szPtr = ToLower(*szPtr);
  return szArg;
}


/*
    *************************************************************************************
        ndb_os2_GetOS2Version()

        returns string with OS/2 version
    *************************************************************************************
*/

char s_os2ver[50];

char *ndb_os2_GetOS2Version()
{
    ULONG aulBuffer[3];
    APIRET rc;


    rc = DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_VERSION_REVISION, (void *)aulBuffer, 3*sizeof(ULONG));
    if(rc)
    {
        printf("Error! DosQuerySysInfo returned %d.\n",(int) rc);
        return("");
    }
    else
    {
        if (aulBuffer[0] != 20L)
            snprintf(s_os2ver, 50 - 1, "OS/2 (V%lu.%lu)", aulBuffer[0] / 10, aulBuffer[1]);
        else
        {
            if (aulBuffer[1] == 30L)
                snprintf(s_os2ver, 50 - 1, "OS/2 Warp 3 (V%lu.%lu)", aulBuffer[0] / 10, aulBuffer[1]);
            else if (aulBuffer[1] == 40L)
                snprintf(s_os2ver, 50 - 1, "OS/2 Warp 4 (V%lu.%lu)", aulBuffer[0] / 10, aulBuffer[1]);
            else if (aulBuffer[1] >= 45L)
                snprintf(s_os2ver, 50 - 1, "OS/2 Warp 4 MCP/eCS (V%lu.%lu)", aulBuffer[0] / 10, aulBuffer[1]);
        }
    }
    return s_os2ver;
}

/* ------------------------------------------------------------------------*/

ULONG ndb_os2_ndbattr2os2attr(ULONG ndbattr)
{
        return (ndbattr & NDB_FILEFLAG_RONLY   ? _A_RDONLY   : 0)
             | (ndbattr & NDB_FILEFLAG_HIDDEN  ? _A_HIDDEN   : 0)
             | (ndbattr & NDB_FILEFLAG_SYSTEM  ? _A_SYSTEM   : 0)
             | (ndbattr & NDB_FILEFLAG_DIR     ? _A_SUBDIR   : 0)
             | (ndbattr & NDB_FILEFLAG_ARCHIVE ? _A_ARCHIVE  : 0);
}


ULONG ndb_os2_os2attr2ndbattr(ULONG os2attr)
{
        return (os2attr & _A_RDONLY    ? NDB_FILEFLAG_RONLY   : 0)
             | (os2attr & _A_HIDDEN    ? NDB_FILEFLAG_HIDDEN  : 0)
             | (os2attr & _A_SYSTEM    ? NDB_FILEFLAG_SYSTEM  : 0)
             | (os2attr & _A_SUBDIR    ? NDB_FILEFLAG_DIR     : 0)
             | (os2attr & _A_ARCHIVE   ? NDB_FILEFLAG_ARCHIVE : 0);
}

/*
    *************************************************************************************
        ndb_os2_truncate()

        truncate file at given size
    *************************************************************************************
*/

void ndb_os2_truncate(const char *pathfile, ULONG fileSize)
{
#if defined(__GNUC__)

    truncate((char *) pathfile, fileSize);

#elif defined(__WATCOMC__)

    APIRET rc;
    HFILE hFile;
    ULONG actionTaken;

    rc = DosOpen(pathfile,
                       &hFile,
                       &actionTaken,
                       0,
                       FILE_NORMAL,
                       OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                       OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,
                       NULL);

    if (rc != NO_ERROR)
    {
        rc = DosSetFileSize(hFile, fileSize);
        DosClose(hFile);
    }

#endif

}

/*
    *************************************************************************************
        ndb_os2_setFileSize64()

        gets the file size using 64 Bit functions and sets the file size entry
        of file structure pFile,
        BUT: using Innotek's GCC 3.2.2 we have nothing to do here because
        Innotek's RTL uses the 64 bit functions in the background if they are
        available
    *************************************************************************************
*/

void ndb_os2_setFileSize64(PFILEENTRY pFile)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_setFileSize64 - startup\n");
    fflush(stdout);

    // nothing to do here because of Innotek's runtime library :-)

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_os2_setFileSize64: before - file size is %.0fb\n", (double) pFile->lenOri8);
        if (pFile->lenOri8 > 2147483648.0)
        {
            fprintf(stdout, "ndb_os2_setFileSize64: file size above 2GB\n");
        }
        fflush(stdout);
    }


    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_setFileSize64 - finished\n");
    fflush(stdout);

    return;
}



/*
    *************************************************************************************
        ndb_os2_version_local()

        Code from Info-Zip project; prints out the compiler, version and date of NDB
    *************************************************************************************
*/

void ndb_os2_version_local()
{
    static char CompiledWith[] = "Compiled with %s%s for %s%s%s%s.\n\n";
#if defined(__IBMC__) || defined(__WATCOMC__) || defined(_MSC_VER)
    char buf[80];
#endif

    printf(CompiledWith,

#ifdef __GNUC__
#  ifdef __EMX__  /* __EMX__ is defined as "1" only (sigh) */
      "emx+gcc ", __VERSION__,
#  else
      "gcc/2 ", __VERSION__,
#  endif
#elif defined(__IBMC__)
      "IBM ",
#  if (__IBMC__ < 200)
      (sprintf(buf, "C Set/2 %d.%02d", __IBMC__/100,__IBMC__%100), buf),
#  elif (__IBMC__ < 300)
      (sprintf(buf, "C Set++ %d.%02d", __IBMC__/100,__IBMC__%100), buf),
#  else
      (sprintf(buf, "Visual Age C++ %d.%02d", __IBMC__/100,__IBMC__%100), buf),
#  endif
#elif defined(__WATCOMC__)
      "Watcom C", (sprintf(buf, " (__WATCOMC__ = %d)", __WATCOMC__), buf),
#elif defined(__TURBOC__)
#  ifdef __BORLANDC__
      "Borland C++",
#    if (__BORLANDC__ < 0x0460)
        " 1.0",
#    elif (__BORLANDC__ == 0x0460)
        " 1.5",
#    else
        " 2.0",
#    endif
#  else
      "Turbo C",
#    if (__TURBOC__ >= 661)
       "++ 1.0 or later",
#    elif (__TURBOC__ == 661)
       " 3.0?",
#    elif (__TURBOC__ == 397)
       " 2.0",
#    else
       " 1.0 or 1.5?",
#    endif
#  endif
#elif defined(MSC)
      "Microsoft C ",
#  ifdef _MSC_VER
      (sprintf(buf, "%d.%02d", _MSC_VER/100, _MSC_VER%100), buf),
#  else
      "5.1 or earlier",
#  endif
#else
      "unknown compiler", "",
#endif /* __GNUC__ */

      "OS/2",

/* GRR:  does IBM C/2 identify itself as IBM rather than Microsoft? */
#if (defined(MSC) || (defined(__WATCOMC__) && !defined(__386__)))
#  if defined(M_I86HM) || defined(__HUGE__)
      " (16-bit, huge)",
#  elif defined(M_I86LM) || defined(__LARGE__)
      " (16-bit, large)",
#  elif defined(M_I86MM) || defined(__MEDIUM__)
      " (16-bit, medium)",
#  elif defined(M_I86CM) || defined(__COMPACT__)
      " (16-bit, compact)",
#  elif defined(M_I86SM) || defined(__SMALL__)
      " (16-bit, small)",
#  elif defined(M_I86TM) || defined(__TINY__)
      " (16-bit, tiny)",
#  else
      " (16-bit)",
#  endif
#else
      " 2.x/3.x (32-bit)",
#endif

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    /* temporary debugging code for Borland compilers only */
#ifdef __TURBOC__
    printf("\t(__TURBOC__ = 0x%04x = %d)\n", __TURBOC__, __TURBOC__);
#ifdef __BORLANDC__
    printf("\t(__BORLANDC__ = 0x%04x)\n",__BORLANDC__);
#else
    printf("\tdebug(__BORLANDC__ not defined)\n");
#endif
#ifdef __TCPLUSPLUS__
    printf("\t(__TCPLUSPLUS__ = 0x%04x)\n", __TCPLUSPLUS__);
#else
    printf("\tdebug(__TCPLUSPLUS__ not defined)\n");
#endif
#ifdef __BCPLUSPLUS__
    printf("\t(__BCPLUSPLUS__ = 0x%04x)\n\n", __BCPLUSPLUS__);
#else
    printf("\tdebug(__BCPLUSPLUS__ not defined)\n\n");
#endif
#endif /* __TURBOC__ */

} /* end function version_local() */

