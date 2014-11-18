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
#include <utime.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>

#include <errno.h>
extern int errno;

#include "../ndb.h"
#include "../ndb_prototypes.h"

#include "ndb_unix.h"


/*
    *************************************************************************************
	ndb_unix_sleep()

	lets the application sleep for <millis> milliseconds (to be multi tasking
	friendly regarding CPU/IO/net usage
    *************************************************************************************
*/

void ndb_unix_sleep(int millis)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "sleep: now waiting for %d milliseconds\n", millis);
    fflush(stdout);

	usleep(1000L * millis);
}


/*
    *************************************************************************************
	ndb_unix_create_path()

	strip filename from <filewithpath> and create the path directory including
	parent directories;
	if filewithpath is a directory (withput file), it has to end with a trailing
	slash!
    *************************************************************************************
*/

int ndb_unix_create_path(const char *filewithpath)
{
    struct stat statFile;
    char sDummy[NDB_MAXLEN_FILENAME];
    char sExtPath[NDB_MAXLEN_FILENAME];
    char *p = NULL, *q = NULL;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_create_path - startup\n");
    fflush(stdout);

    // make external
    strcpy (sExtPath, ndb_osdep_makeExtFileName(filewithpath));

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_create_path: strip filename from '%s'\n", sExtPath);
    fflush(stdout);

    // strip filename
	// this fails if a pure directory doesn't end with a trailing slash!
	p = strrchr (sExtPath, '/');
    if (NULL != p)
    {
        *(++p) = '\0'; // keep '/' as last char
    }
    else
    {
        // no '/' found -> file without path
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_unix_create_path: file '%s' has no path\n", sExtPath);
        fflush(stdout);
        return retval;
    }

    // stat() doesn't like trailing '/'
    strcpy (sDummy, sExtPath);
    if (sDummy[strlen(sDummy) - 1] == '/')
    {
            sDummy[strlen(sDummy) - 1] = '\0';
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_unix_create_path: checking '%s' with stat()\n", sDummy);
    fflush(stdout);

    // first check if path already exists
    if (LSSTAT(sDummy, &statFile) != -1)
    {
        // path already exists
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_unix_create_path: path '%s' already exists\n", sDummy);
        fflush(stdout);
    }
    else
    {
        // path does not exist, so try to create it
        if (ndb_getDebugLevel() >= 6)
            fprintf(stdout, "ndb_unix_create_path: trying to create dir '%s'\n", sExtPath);
        fflush(stdout);

        q = NULL;
        p = sExtPath;
        q = strchr(p, '/');
        // does sExtPath start with an trailing slash?
        if (q == p)
        {
	        if (ndb_getDebugLevel() >= 7)
	            fprintf(stdout, "ndb_unix_create_path: dir '%s' starts with slash\n", sExtPath);
	        fflush(stdout);
/* 20040106; pw; unix doesn't know about UNC paths
			// double '//', i.e. start of an UNC name like '//<server>/<share>\<...>'?
			if (q[1] == '/')
			{
		        if (ndb_getDebugLevel() >= 7)
		            fprintf(stdout, "ndb_unix_create_path: start of UNC name found in dir '%s'\n", sExtPath);
		        fflush(stdout);
		        // look for the begin of the <share> part
		        q = strchr(q + 2, '/');
		        if (q == NULL)
		        {
					// malformed path
		            fprintf(stderr, "ndb_unix_create_path: cannot create malformed path '%s'\n", sExtPath);
			        fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
		        }
		        if (ndb_getDebugLevel() >= 7)
		            fprintf(stdout, "ndb_unix_create_path: path after stripping //<server> is '%s'\n", q);
		        fflush(stdout);
		        // look for the begin of the path after the <share> part
		        q = strchr(q + 1, '/');
		        if (q == NULL)
		        {
					// malformed path
		            fprintf(stderr, "ndb_unix_create_path: cannot create malformed path '%s'\n", sExtPath);
			        fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
		        }
		        if (ndb_getDebugLevel() >= 7)
		            fprintf(stdout, "ndb_unix_create_path: path after stripping //<share> is '%s'\n", q);
		        fflush(stdout);
				// skip leading slash
		        q = strchr(q + 1, '/');
			} // if (q[1] == '/'); path is UNC
			else
*/
            {
		        q = strchr(p + 1, '/');
			}
        } // if (q == p); i.e. the path starts with a slash
        // break if no other '\' can be found
        while (q != NULL)
        {
            // copy path from start to the currently found '\'
            strncpy (sDummy, sExtPath, q - sExtPath);
            sDummy[q - sExtPath] = '\0';
            // debug output
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_unix_create_path: checking path '%s' for existence\n", sDummy);
            fflush(stdout);
            // create the parent directories only if neccessary
            if (LSSTAT(sDummy, &statFile) != -1)
            {
                // path already exists
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "ndb_unix_create_path: path '%s' already exists\n", sDummy);
                fflush(stdout);
            }
            else
            {
	            if (ndb_getDebugLevel() >= 7)
	                fprintf(stdout, "ndb_unix_create_path: call mkdir(..) for path '%s'\n", sDummy);
	            fflush(stdout);
                // creation is neccessary
                if (-1 == mkdir(sDummy, 0777))
                {
                    fprintf(stderr, "ndb_unix_create_path: Error: couldn't create dir '%s'\n", sDummy);
                    fflush(stderr);
                    return NDB_MSG_NODIRCREATE;
                }
            }
            // set p to the first char after the current '/'
            p = q + 1;
            q = strchr(p, '/');
        }
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_create_path - finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
	ndb_unix_map_fileattributes()

	maps DOS file attributes to UNIX attributes
    *************************************************************************************
*/

int ndb_unix_map_fileattributes(struct ndb_s_fileentry *pFile)
{
    int fmodeUnix = 0444;    // default r--r--r--
    int fmodeNDB  = pFile->attributes;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_map_fileattributes: NDB attributes 0x%X\n", fmodeNDB);
    fflush(stdout);

    // subdir bit --> dir exec bit
    if ((fmodeNDB & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
    {
        fmodeUnix |= 0100;
    }
    // no read-only bit --> write perms
    if ((fmodeNDB & NDB_FILEFLAG_RONLY) != NDB_FILEFLAG_RONLY)
    {
        fmodeUnix |= 0222;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_map_fileattributes: unix attributes 0%o\n", fmodeUnix);
    fflush(stdout);

    return fmodeUnix;
}


/*
    *************************************************************************************
	ndb_unix_setPathAttr()

	sets 'generic' file attributes
    *************************************************************************************
*/

int ndb_unix_setPathAttr(struct ndb_s_fileentry *pFile, const char *sDefaultPath)
{
    char newFilename[NDB_MAXLEN_FILENAME] = "";
    long fmode = ndb_unix_map_fileattributes(pFile);

    if ((strcmp(sDefaultPath, ".") == 0) || (sDefaultPath[0] == '\0'))
        strcpy(newFilename, pFile->filenameExt);
    else
        snprintf(newFilename, NDB_MAXLEN_FILENAME - 1, "%s/%s", sDefaultPath, pFile->filenameExt);
    strcpy (newFilename, ndb_osdep_makeExtFileName(newFilename));

    if ((pFile->attributes & NDB_FILEFLAG_SLNK) == NDB_FILEFLAG_SLNK)
    {
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_unix_setPathAttr: not needed for symlink '%s'\n",
                    newFilename);
        fflush(stdout);
        return NDB_MSG_OK;
    }

    if (chmod(newFilename, 0xffff & fmode))
    {
        fprintf(stderr, "ndb_unix_setPathAttr: error setting file attributes (0x%lx) for '%s'\n%s\n",
                        (0xffff & fmode), newFilename, strerror(errno));
        return NDB_MSG_NOTOK;
    }
    return NDB_MSG_OK;
}


/*
    *************************************************************************************
	ndb_unix_setUnixAttributes()

	get Unix file attributes from the extradata header;
    if file is a symbolic link, delete the created file and create the link,
    then exit;
    at all other files the file attributes, the UID/GID anf the file times
    must be set.
    *************************************************************************************
*/

int ndb_unix_setUnixAttributes(struct ndb_s_fileentry *pFile, PFILEEXTRADATAHEADER pExtraHea,
                               char *newFilename)
{
    iztimes zt;
    int count          = 0;
    USHORT st_mode     = 0;
    USHORT st_nlink    = 0;
    USHORT st_uid      = 0;
    USHORT st_gid      = 0;
    USHORT len_symlink = 0;
    char sLink2File[NDB_MAXLEN_FILENAME];
    int retval = NDB_MSG_OK;

    // read special UNIX file attributes
    st_mode   = ndb_readbuf_ushort(pExtraHea->pData, &count);
    st_nlink  = ndb_readbuf_ushort(pExtraHea->pData, &count);
    st_uid    = ndb_readbuf_ushort(pExtraHea->pData, &count);
    st_gid    = ndb_readbuf_ushort(pExtraHea->pData, &count);

    // is pFile a symbolic link?
    if ((pFile->attributes & NDB_FILEFLAG_SLNK) == NDB_FILEFLAG_SLNK)
    {
        if (pExtraHea->lenExtHea < 10)
        {
            fprintf(stderr, "internal error: no target for symbolic link '%s' in archive file\n",
                            newFilename);
            fflush(stderr);
            return NDB_MSG_SYMLINKFAILED;
        }
        len_symlink = ndb_readbuf_ushort(pExtraHea->pData, &count);
        strcpy (sLink2File, ndb_readbuf_char_n(pExtraHea->pData, &count, len_symlink));
        sLink2File[len_symlink] = '\0';
        // delete file
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_unix_setUnixAttributes:  deleting file '%s'\n", newFilename);
        fflush(stdout);
        unlink(newFilename);
        // create the symbolic link
        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_unix_setUnixAttributes:  creating symlink '%s' for '%s'\n",
                    newFilename, sLink2File);
        fflush(stdout);
        if (symlink(sLink2File, newFilename))
        {
            ndb_fe_addRemark(pFile, strerror (errno));
            retval = NDB_MSG_SYMLINKFAILED;
        }
        return retval;
    }

    // set file times
    if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_unix_setUnixAttributes:  restoring file times for '%s'\n",
                        newFilename);
    fflush(stdout);
    zt.atime = pFile->atime;
    zt.mtime = pFile->mtime;
    zt.ctime = pFile->ctime;
    if (utime(newFilename, (struct utimbuf *) &zt))
    {
        fprintf(stdout, "warning:  cannot set file times for '%s'\n", newFilename);
        fflush(stdout);
        ndb_fe_addRemark(pFile, strerror (errno));
        retval = NDB_MSG_EXTRAFAILED;
    }
    // set 'user' and 'group'
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_setUnixAttributes:  restoring Unix UID/GID info (%d/%d) for '%s'\n",
                st_uid, st_gid, newFilename);
    fflush(stdout);
    if (chown(newFilename, (uid_t)st_uid, (gid_t)st_gid))
    {
        fprintf(stdout, "\nwarning:  cannot set UID %d and/or GID %d for '%s'\n",
                        st_uid, st_gid, newFilename);
        fflush(stdout);
        ndb_fe_addRemark(pFile, strerror (errno));
        retval = NDB_MSG_EXTRAFAILED;
    }


    // set file attributes at last (perhaps we disable our own access by setting
    // the read/writeattributes
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_setUnixAttributes:  restoring file attributes 0%o for '%s'\n",
                0xffff & st_mode, newFilename);
    fflush(stdout);
    if (chmod(newFilename, 0xffff & st_mode))
    {
        fprintf(stdout, "warning:  cannot set file attributes for '%s'\n", newFilename);
        fprintf(stdout, strerror(errno));
        ndb_fe_addRemark(pFile, strerror (errno));
        retval = NDB_MSG_EXTRAFAILED;
    }

    return retval;
}


/*
    *************************************************************************************
	ndb_unix_getFileMode()

	gets file attributes for <name> and maps them to NDB fileflags

	Returns: NDB fileflags of file
    *************************************************************************************
*/

int ndb_unix_getFileMode(char *name, int showLink)
{
    int fmode = 0;
    struct stat statFile;
    struct stat64 mystat64;
    int statmode = 0;
    int retval;

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_unix_getFileMode - startup\n");
    fflush(stdout);

    if ((name == NULL) || (name[0] == '\0'))
    {
        fprintf(stdout, "ndb_unix_getFileMode: internal error: filename is NULL or empty\n");
        fflush(stdout);
        return 0;
    }

    memset(&statFile, 0, sizeof(struct stat));

    if (showLink == 0)
        // show file attributes of link target
        retval = SSTAT(name, &statFile);
    else
        // show file attributes of link itself
        retval = LSSTAT(name, &statFile);
    statmode = statFile.st_mode;

    // errno = 75 says that file size exceeds unsigned long
    if ((retval == -1) && (errno == 75))
    {
        retval = stat64(name, &mystat64);
        if (retval != -1)
        {
            statmode = mystat64.st_mode;
        }
    }

    if (retval == -1)
    {
        fprintf(stderr, "ndb_unix_getFileMode: error calling stat()/stat64() for '%s'\n(%d) -> %s\n",
                name, errno, strerror(errno));
        fflush(stderr);
    	fmode = 0;
    }
    else
    {
        fmode = 0;
    	fmode |= S_ISDIR(statmode) ? NDB_FILEFLAG_DIR  : 0;
    	fmode |= S_ISLNK(statmode) ? NDB_FILEFLAG_SLNK : 0;
    	fmode |= S_ISCHR(statmode) ? NDB_FILEFLAG_CHARDEV : 0;
    	fmode |= S_ISBLK(statmode) ? NDB_FILEFLAG_BLKDEV : 0;
    	fmode |= S_ISFIFO(statmode) ? NDB_FILEFLAG_FIFO : 0;
    	fmode |= S_ISSOCK(statmode) ? NDB_FILEFLAG_SOCKET : 0;
        fmode |= S_ISREG(statmode) ? NDB_FILEFLAG_FILE : 0;
    }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_getFileMode: file mode 0x%2X->0x%2X of '%s'\n",
                statmode, fmode, name);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_unix_getFileMode - finished\n");
    fflush(stdout);

    return fmode;
}



#define DOSTIME_MINIMUM         ((ULONG)0x00210000L)
#define DOSTIME_2038_01_18      ((ULONG)0x74320000L)


ULONG dostime(int y, int n, int d, int h, int m, int s)
/* Convert the date y/n/d and time h:m:s to a four byte DOS date and
   time (date in high two bytes, time in low two bytes allowing magnitude
   comparison). */
{
  return y < 1980 ? DOSTIME_MINIMUM /* dostime(1980, 1, 1, 0, 0, 0) */ :
        (((ULONG)y - 1980) << 25) | ((ULONG)n << 21) | ((ULONG)d << 16) |
        ((ULONG)h << 11) | ((ULONG)m << 5) | ((ULONG)s >> 1);
}


/* MSDOS file or directory attributes */
#define MSDOS_HIDDEN_ATTR 0x02
#define MSDOS_DIR_ATTR 0x10

void filetime(char *filename, ULONG *attr)
/* If file *f does not exist, return 0.  Else, return the file's last
   modified date and time as an MSDOS date and time.  The date and
   time is returned in a long with the date most significant to allow
   unsigned integer comparison of absolute times.  Also, if a is not
   a NULL pointer, store the file attributes there, with the high two
   bytes being the Unix attributes, and the low byte being a mapping
   of that to DOS attributes. */
{
  struct stat s;        /* results of stat() */
  char name[NDB_MAXLEN_FILENAME];
  int len = strlen(filename);

  strcpy(name, filename);
  if (name[len - 1] == '/')
    name[len - 1] = '\0';
  /* not all systems allow stat'ing a file with / appended */
  if (LSSTAT(name, &s) != 0)
    /* Accept about any file kind including directories
     * (stored with trailing / with -r option)
     */
    return;

  if (attr != NULL)
  {
    *attr = ((ULONG)s.st_mode << 16) | !(s.st_mode & S_IWRITE);
    if ((s.st_mode & S_IFMT) == S_IFDIR)
    {
      *attr |= MSDOS_DIR_ATTR;
    }
  }
}



/*
    *************************************************************************************
	ndb_unix_getCodepage()

	asks for the current code page
    *************************************************************************************
*/

#define streq(s1,s2) (!strcmp(s1,s2))

char* locale_charset = NULL;
char* get_locale_charset();

char *ndb_unix_getCodepage()
{
    return get_locale_charset();
}


/*
    Function get_locale_charset() is from Bruno Haible,
    see ftp://ftp.ilog.fr/pub/Users/haible/utf8/locale_charset.c
*/

 /*
   * This C function returns a canonical name for the character encoding
   * used in the current locale. It returns NULL if it cannot be determined.
   *
   * This is an alternative to nl_langinfo(CODESET).
   */


  char* get_locale_charset ()
    {
      // When you call setlocale(LC_CTYPE,""), is examines the environment
      // variables:
      // 1. environment variable LC_ALL - an override for all LC_* variables,
      // 2. environment variable LC_CTYPE,
      // 3. environment variable LANG - a default for all LC_* variables.
      const char * locale;
      locale = getenv("LC_ALL");
      if (!locale || !*locale) {
        locale = getenv("LC_CTYPE");
        if (!locale || !*locale)
          locale = getenv("LANG");
      }
      // Determine locale_charset from the environment variables.
      // Unfortunately there is no documented way of getting the character set
      // that was specified as part of the LC_CTYPE category. We have to parse
      // the environment variables ourselves.
      // Recall that a locale specification has the form
      //   language_COUNTRY.charset
      // but there are also aliases. Here is the union of what I found in
      // /usr/X11R6/lib/X11/locale/locale.alias (X11R6) and
      // /usr/share/locale/locale.alias (GNU libc2).
      //
      // X11R6 locale.alias:
      //   POSIX                   C
      //   POSIX-UTF2              C
      //   C_C.C                   C
      //   C.en                    C
      //   C.iso88591              en_US.ISO8859-1
      //   Cextend                 en_US.ISO8859-1
      //   Cextend.en              en_US.ISO8859-1
      //   English_United-States.437       C
      //   #
      //   ar                      ar_AA.ISO8859-6
      //   ar_AA                   ar_AA.ISO8859-6
      //   ar_AA.ISO_8859-6        ar_AA.ISO8859-6
      //   ar_SA.iso88596          ar_AA.ISO8859-6
      //   bg                      bg_BG.ISO8859-5
      //   bg_BG                   bg_BG.ISO8859-5
      //   bg_BG.ISO_8859-5        bg_BG.ISO8859-5
      //   bg_BG.iso88595          bg_BG.ISO8859-5
      //   cs                      cs_CZ.ISO8859-2
      //   cs_CS                   cs_CZ.ISO8859-2
      //   cs_CS.ISO8859-2         cs_CZ.ISO8859-2
      //   cs_CS.ISO_8859-2        cs_CZ.ISO8859-2
      //   cs_CZ.iso88592          cs_CZ.ISO8859-2
      //   cz                      cz_CZ.ISO8859-2
      //   cz_CZ                   cz_CZ.ISO8859-2
      //   cs_CZ.ISO_8859-2        cs_CZ.ISO8859-2
      //   da                      da_DK.ISO8859-1
      //   da_DK                   da_DK.ISO8859-1
      //   da_DK.88591             da_DK.ISO8859-1
      //   da_DK.88591.en          da_DK.ISO8859-1
      //   da_DK.iso88591          da_DK.ISO8859-1
      //   da_DK.ISO_8859-1        da_DK.ISO8859-1
      //   de                      de_DE.ISO8859-1
      //   de_AT                   de_AT.ISO8859-1
      //   de_AT.ISO_8859-1        de_AT.ISO8859-1
      //   de_CH                   de_CH.ISO8859-1
      //   de_CH.ISO_8859-1        de_CH.ISO8859-1
      //   de_DE                   de_DE.ISO8859-1
      //   de_DE.88591             de_DE.ISO8859-1
      //   de_DE.88591.en          de_DE.ISO8859-1
      //   de_DE.iso88591          de_DE.ISO8859-1
      //   de_DE.ISO_8859-1        de_DE.ISO8859-1
      //   GER_DE.8859             de_DE.ISO8859-1
      //   GER_DE.8859.in          de_DE.ISO8859-1
      //   el                      el_GR.ISO8859-7
      //   el_GR                   el_GR.ISO8859-7
      //   el_GR.iso88597          el_GR.ISO8859-7
      //   el_GR.ISO_8859-7        el_GR.ISO8859-7
      //   en                      en_US.ISO8859-1
      //   en_AU                   en_AU.ISO8859-1
      //   en_AU.ISO_8859-1        en_AU.ISO8859-1
      //   en_CA                   en_CA.ISO8859-1
      //   en_CA.ISO_8859-1        en_CA.ISO8859-1
      //   en_GB                   en_GB.ISO8859-1
      //   en_GB.88591             en_GB.ISO8859-1
      //   en_GB.88591.en          en_GB.ISO8859-1
      //   en_GB.iso88591          en_GB.ISO8859-1
      //   en_GB.ISO_8859-1        en_GB.ISO8859-1
      //   en_UK                   en_GB.ISO8859-1
      //   ENG_GB.8859             en_GB.ISO8859-1
      //   ENG_GB.8859.in          en_GB.ISO8859-1
      //   en_IE                   en_IE.ISO8859-1
      //   en_NZ                   en_NZ.ISO8859-1
      //   en_US                   en_US.ISO8859-1
      //   en_US.88591             en_US.ISO8859-1
      //   en_US.88591.en          en_US.ISO8859-1
      //   en_US.iso88591          en_US.ISO8859-1
      //   en_US.ISO_8859-1        en_US.ISO8859-1
      //   es                      es_ES.ISO8859-1
      //   es_AR                   es_AR.ISO8859-1
      //   es_BO                   es_BO.ISO8859-1
      //   es_CL                   es_CL.ISO8859-1
      //   es_CO                   es_CO.ISO8859-1
      //   es_CR                   es_CR.ISO8859-1
      //   es_EC                   es_EC.ISO8859-1
      //   es_ES                   es_ES.ISO8859-1
      //   es_ES.88591             es_ES.ISO8859-1
      //   es_ES.88591.en          es_ES.ISO8859-1
      //   es_ES.iso88591          es_ES.ISO8859-1
      //   es_ES.ISO_8859-1        es_ES.ISO8859-1
      //   es_GT                   es_GT.ISO8859-1
      //   es_MX                   es_MX.ISO8859-1
      //   es_NI                   es_NI.ISO8859-1
      //   es_PA                   es_PA.ISO8859-1
      //   es_PE                   es_PE.ISO8859-1
      //   es_PY                   es_PY.ISO8859-1
      //   es_SV                   es_SV.ISO8859-1
      //   es_UY                   es_UY.ISO8859-1
      //   es_VE                   es_VE.ISO8859-1
      //   fi                      fi_FI.ISO8859-1
      //   fi_FI                   fi_FI.ISO8859-1
      //   fi_FI.88591             fi_FI.ISO8859-1
      //   fi_FI.88591.en          fi_FI.ISO8859-1
      //   fi_FI.iso88591          fi_FI.ISO8859-1
      //   fi_FI.ISO_8859-1        fi_FI.ISO8859-1
      //   fr                      fr_FR.ISO8859-1
      //   fr_BE                   fr_BE.ISO8859-1
      //   fr_BE.88591             fr_BE.ISO8859-1
      //   fr_BE.88591.en          fr_BE.ISO8859-1
      //   fr_BE.ISO_8859-1        fr_BE.ISO8859-1
      //   fr_CA                   fr_CA.ISO8859-1
      //   fr_CA.88591             fr_CA.ISO8859-1
      //   fr_CA.88591.en          fr_CA.ISO8859-1
      //   fr_CA.iso88591          fr_CA.ISO8859-1
      //   fr_CA.ISO_8859-1        fr_CA.ISO8859-1
      //   fr_CH                   fr_CH.ISO8859-1
      //   fr_CH.88591             fr_CH.ISO8859-1
      //   fr_CH.88591.en          fr_CH.ISO8859-1
      //   fr_CH.ISO_8859-1        fr_CH.ISO8859-1
      //   fr_FR                   fr_FR.ISO8859-1
      //   fr_FR.88591             fr_FR.ISO8859-1
      //   fr_FR.88591.en          fr_FR.ISO8859-1
      //   fr_FR.iso88591          fr_FR.ISO8859-1
      //   fr_FR.ISO_8859-1        fr_FR.ISO8859-1
      //   FRE_FR.8859             fr_FR.ISO8859-1
      //   FRE_FR.8859.in          fr_FR.ISO8859-1
      //   he                      he_IL.ISO8859-8
      //   he_IL                   he_IL.ISO8859-8
      //   he_IL.iso88598          he_IL.ISO8859-8
      //   hr                      hr_HR.ISO8859-2
      //   hr_HR                   hr_HR.ISO8859-2
      //   hr_HR.iso88592          hr_HR.ISO8859-2
      //   hr_HR.ISO_8859-2        hr_HR.ISO8859-2
      //   hu                      hu_HU.ISO8859-2
      //   hu_HU                   hu_HU.ISO8859-2
      //   hu_HU.iso88592          hu_HU.ISO8859-2
      //   hu_HU.ISO_8859-2        hu_HU.ISO8859-2
      //   is                      is_IS.ISO8859-1
      //   is_IS                   is_IS.ISO8859-1
      //   is_IS.iso88591          is_IS.ISO8859-1
      //   is_IS.ISO_8859-1        is_IS.ISO8859-1
      //   it                      it_IT.ISO8859-1
      //   it_CH                   it_CH.ISO8859-1
      //   it_CH.ISO_8859-1        it_CH.ISO8859-1
      //   it_IT                   it_IT.ISO8859-1
      //   it_IT.88591             it_IT.ISO8859-1
      //   it_IT.88591.en          it_IT.ISO8859-1
      //   it_IT.iso88591          it_IT.ISO8859-1
      //   it_IT.ISO_8859-1        it_IT.ISO8859-1
      //   iw                      iw_IL.ISO8859-8
      //   iw_IL                   iw_IL.ISO8859-8
      //   iw_IL.iso88598          iw_IL.ISO8859-8
      //   iw_IL.ISO_8859-8        iw_IL.ISO8859-8
      //   ja                      ja_JP.eucJP
      //   ja_JP                   ja_JP.eucJP
      //   ja_JP.ujis              ja_JP.eucJP
      //   ja_JP.eucJP             ja_JP.eucJP
      //   Jp_JP                   ja_JP.eucJP
      //   ja_JP.AJEC              ja_JP.eucJP
      //   ja_JP.EUC               ja_JP.eucJP
      //   ja_JP.ISO-2022-JP       ja_JP.JIS7
      //   ja_JP.JIS               ja_JP.JIS7
      //   ja_JP.jis7              ja_JP.JIS7
      //   ja_JP.mscode            ja_JP.SJIS
      //   ja_JP.SJIS              ja_JP.SJIS
      //   ko                      ko_KR.eucKR
      //   ko_KR                   ko_KR.eucKR
      //   ko_KR.EUC               ko_KR.eucKR
      //   ko_KR.euc               ko_KR.eucKR
      //   # most locales in FreeBSD 2.1.[56] do not work, allow use of generic latin-1
      //   lt_LN.ISO_8859-1        lt_LN.ISO8859-1
      //   mk                      mk_MK.ISO8859-5
      //   mk_MK                   mk_MK.ISO8859-5
      //   mk_MK.ISO_8859-5        mk_MK.ISO8859-5
      //   nl                      nl_NL.ISO8859-1
      //   nl_BE                   nl_BE.ISO8859-1
      //   nl_BE.88591             nl_BE.ISO8859-1
      //   nl_BE.88591.en          nl_BE.ISO8859-1
      //   nl_BE.ISO_8859-1        nl_BE.ISO8859-1
      //   nl_NL                   nl_NL.ISO8859-1
      //   nl_NL.88591             nl_NL.ISO8859-1
      //   nl_NL.88591.en          nl_NL.ISO8859-1
      //   nl_NL.iso88591          nl_NL.ISO8859-1
      //   nl_NL.ISO_8859-1        nl_NL.ISO8859-1
      //   no                      no_NO.ISO8859-1
      //   no_NO                   no_NO.ISO8859-1
      //   no_NO.88591             no_NO.ISO8859-1
      //   no_NO.88591.en          no_NO.ISO8859-1
      //   no_NO.iso88591          no_NO.ISO8859-1
      //   no_NO.ISO_8859-1        no_NO.ISO8859-1
      //   pl                      pl_PL.ISO8859-2
      //   pl_PL                   pl_PL.ISO8859-2
      //   pl_PL.iso88592          pl_PL.ISO8859-2
      //   pl_PL.ISO_8859-2        pl_PL.ISO8859-2
      //   pt                      pt_PT.ISO8859-1
      //   pt_BR                   pt_BR.ISO8859-1
      //   pt_PT                   pt_PT.ISO8859-1
      //   pt_PT.88591             pt_PT.ISO8859-1
      //   pt_PT.88591.en          pt_PT.ISO8859-1
      //   pt_PT.iso88591          pt_PT.ISO8859-1
      //   pt_PT.ISO_8859-1        pt_PT.ISO8859-1
      //   ro                      ro_RO.ISO8859-2
      //   ro_RO                   ro_RO.ISO8859-2
      //   ro_RO.iso88592          ro_RO.ISO8859-2
      //   ro_RO.ISO_8859-2        ro_RO.ISO8859-2
      //   ru                      ru_RU.ISO8859-5
      //   ru_RU                   ru_RU.ISO8859-5
      //   ru_RU.iso88595          ru_RU.ISO8859-5
      //   ru_RU.ISO_8859-5        ru_RU.ISO8859-5
      //   ru_SU                   ru_RU.ISO8859-5
      //   ru_SU.ISO8859-5         ru_RU.ISO8859-5
      //   ru_SU.ISO_8859-5        ru_RU.ISO8859-5
      //   ru_SU.KOI8-R            ru_RU.KOI8-R
      //   sh                      sh_YU.ISO8859-2
      //   sh_YU                   sh_YU.ISO8859-2
      //   sh_YU.ISO_8859-2        sh_YU.ISO8859-2
      //   sh_SP                   sh_YU.ISO8859-2
      //   sk                      sk_SK.ISO8859-2
      //   sk_SK                   sk_SK.ISO8859-2
      //   sk_SK.ISO_8859-2        sk_SK.ISO8859-2
      //   sl                      sl_CS.ISO8859-2
      //   sl_CS                   sl_CS.ISO8859-2
      //   sl_CS.ISO_8859-2        sl_CS.ISO8859-2
      //   sl_SI                   sl_SI.ISO8859-2
      //   sl_SI.iso88592          sl_SI.ISO8859-2
      //   sl_SI.ISO_8859-2        sl_SI.ISO8859-2
      //   sp                      sp_YU.ISO8859-5
      //   sp_YU                   sp_YU.ISO8859-5
      //   sp_YU.ISO_8859-5        sp_YU.ISO8859-5
      //   sr_SP                   sr_SP.ISO8859-2
      //   sr_SP.ISO_8859-2        sr_SP.ISO8859-2
      //   sv                      sv_SE.ISO8859-1
      //   sv_SE                   sv_SE.ISO8859-1
      //   sv_SE.88591             sv_SE.ISO8859-1
      //   sv_SE.88591.en          sv_SE.ISO8859-1
      //   sv_SE.iso88591          sv_SE.ISO8859-1
      //   sv_SE.ISO_8859-1        sv_SE.ISO8859-1
      //   th_TH                   th_TH.TACTIS
      //   tr                      tr_TR.ISO8859-9
      //   tr_TR                   tr_TR.ISO8859-9
      //   tr_TR.iso88599          tr_TR.ISO8859-9
      //   tr_TR.ISO_8859-9        tr_TR.ISO8859-9
      //   zh                      zh_CN.eucCN
      //   zh_CN                   zh_CN.eucCN
      //   zh_CN.EUC               zh_CN.eucCN
      //   zh_TW                   zh_TW.eucTW
      //   zh_TW.EUC               zh_TW.eucTW
      //   # The following locale names are used in SCO 3.0
      //   english_uk.8859         en_GB.ISO8859-1
      //   english_us.8859         en_US.ISO8859-1
      //   english_us.ascii        en_US.ISO8859-1
      //   french_france.8859      fr_FR.ISO8859-1
      //   german_germany.8859     de_DE.ISO8859-1
      //   portuguese_brazil.8859  pt_BR.ISO8859-1
      //   spanish_spain.8859      es_ES.ISO8859-1
      //   # The following locale names are used in HPUX 9.x
      //   american.iso88591       en_US.ISO8859-1
      //   arabic.iso88596         ar_AA.ISO8859-6
      //   bulgarian               bg_BG.ISO8859-5
      //   c-french.iso88591       fr_CA.ISO8859-1
      //   chinese-s               zh_CN.eucCN
      //   chinese-t               zh_TW.eucTW
      //   croatian                hr_HR.ISO8859-2
      //   czech                   cs_CS.ISO8859-2
      //   danish.iso88591         da_DK.ISO8859-1
      //   dutch.iso88591          nl_BE.ISO8859-1
      //   english.iso88591        en_EN.ISO8859-1
      //   finnish.iso88591        fi_FI.ISO8859-1
      //   french.iso88591         fr_CH.ISO8859-1
      //   german.iso88591         de_CH.ISO8859-1
      //   greek.iso88597          el_GR.ISO8859-7
      //   hebrew.iso88598         iw_IL.ISO8859-8
      //   hungarian               hu_HU.ISO8859-2
      //   icelandic.iso88591      is_IS.ISO8859-1
      //   italian.iso88591        it_IT.ISO8859-1
      //   japanese                ja_JP.SJIS
      //   japanese.euc            ja_JP.eucJP
      //   korean                  ko_KR.eucKR
      //   norwegian.iso88591      no_NO.ISO8859-1
      //   polish                  pl_PL.ISO8859-2
      //   portuguese.iso88591     pt_PT.ISO8859-1
      //   rumanian                ro_RO.ISO8859-2
      //   russian                 ru_SU.ISO8859-5
      //   serbocroatian           sh_YU.ISO8859-2
      //   slovak                  sk_SK.ISO8859-2
      //   slovene                 sl_CS.ISO8859-2
      //   spanish.iso88591        es_ES.ISO8859-1
      //   swedish.iso88591        sv_SE.ISO8859-1
      //   turkish.iso88599        tr_TR.ISO8859-9
      //   # Solaris and SunOS have iso_8859_1 LC_CTYPES to augment LANG=C
      //   iso_8859_1              en_US.ISO8859-1
      //   # Microsoft Windows/NT 3.51 Japanese Edition
      //   Korean_Korea.949        ko_KR.eucKR
      //   Japanese_Japan.932      ja_JP.SJIS
      //   # Other miscellaneous locale names
      //   ISO8859-1               en_US.ISO8859-1
      //   ISO-8859-1              en_US.ISO8859-1
      //   japan                   ja_JP.eucJP
      //   Japanese-EUC            ja_JP.eucJP
      //
      // GNU locale.alias:
      //   czech                   cs_CZ.ISO-8859-2
      //   danish                  da_DK.ISO-8859-1
      //   dansk                   da_DK.ISO-8859-1
      //   deutsch                 de_DE.ISO-8859-1
      //   dutch                   nl_NL.ISO-8859-1
      //   finnish                 fi_FI.ISO-8859-1
      //   fran�ais                fr_FR.ISO-8859-1
      //   french                  fr_FR.ISO-8859-1
      //   german                  de_DE.ISO-8859-1
      //   greek                   el_GR.ISO-8859-7
      //   hebrew                  iw_IL.ISO-8859-8
      //   hungarian               hu_HU.ISO-8859-2
      //   icelandic               is_IS.ISO-8859-1
      //   italian                 it_IT.ISO-8859-1
      //   japanese                ja_JP.SJIS
      //   japanese.euc            ja_JP.eucJP
      //   norwegian               no_NO.ISO-8859-1
      //   polish                  pl_PL.ISO-8859-2
      //   portuguese              pt_PT.ISO-8859-1
      //   romanian                ro_RO.ISO-8859-2
      //   russian                 ru_RU.ISO-8859-5
      //   slovak                  sk_SK.ISO-8859-2
      //   slovene                 sl_CS.ISO-8859-2
      //   spanish                 es_ES.ISO-8859-1
      //   swedish                 sv_SE.ISO-8859-1
      //   turkish                 tr_TR.ISO-8859-9
      //
      if (locale && *locale) {
        // The most general syntax of a locale (not all optional parts
        // recognized by all systems) is
        // language[_territory][.codeset][@modifier][+special][,[sponsor][_revision]]
        // To retrieve the codeset, search the first dot. Stop searching when
        // a '@' or '+' or ',' is encountered.
        char* buf = (char*) malloc(strlen(locale)+1);
        const char* codeset = NULL;
        {
          const char* cp = locale;
          for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++) {
            if (*cp == '.') {
              codeset = ++cp;
              for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++);
              if (*cp != '\0') {
                size_t n = cp - codeset;
                memcpy(buf,codeset,n);
                buf[n] = '\0';
                codeset = buf;
              }
              break;
            }
          }
        }
        if (codeset) {
          // Canonicalize the charset given after the dot.
          if (   streq(codeset,"ISO8859-1")
              || streq(codeset,"ISO_8859-1")
              || streq(codeset,"iso88591")
              || streq(codeset,"88591")
              || streq(codeset,"88591.en")
              || streq(codeset,"8859")
              || streq(codeset,"8859.in")
              || streq(codeset,"ascii")
             )
            locale_charset = "ISO-8859-1";
          else
          if (   streq(codeset,"ISO8859-2")
              || streq(codeset,"ISO_8859-2")
              || streq(codeset,"iso88592")
             )
            locale_charset = "ISO-8859-2";
          else
          if (   streq(codeset,"ISO8859-5")
              || streq(codeset,"ISO_8859-5")
              || streq(codeset,"iso88595")
             )
            locale_charset = "ISO-8859-5";
          else
          if (   streq(codeset,"ISO8859-6")
              || streq(codeset,"ISO_8859-6")
              || streq(codeset,"iso88596")
             )
            locale_charset = "ISO-8859-6";
          else
          if (   streq(codeset,"ISO8859-7")
              || streq(codeset,"ISO_8859-7")
              || streq(codeset,"iso88597")
             )
            locale_charset = "ISO-8859-7";
          else
          if (   streq(codeset,"ISO8859-8")
              || streq(codeset,"iso88598")
             )
            locale_charset = "ISO-8859-8";
          else
          if (   streq(codeset,"ISO8859-9")
              || streq(codeset,"ISO_8859-9")
              || streq(codeset,"iso88599")
             )
            locale_charset = "ISO-8859-9";
          else
          if (streq(codeset,"KOI8-R"))
            locale_charset = "KOI8-R";
          else
          if (streq(codeset,"KOI8-U"))
            locale_charset = "KOI8-U";
          else
          if (   streq(codeset,"eucJP")
              || streq(codeset,"ujis")
              || streq(codeset,"AJEC")
             )
            locale_charset = "eucJP";
          else
          if (   streq(codeset,"JIS7")
              || streq(codeset,"jis7")
              || streq(codeset,"JIS")
              || streq(codeset,"ISO-2022-JP")
             )
            locale_charset = "ISO-2022-JP"; /* was: "JIS7"; */
          else
          if (   streq(codeset,"SJIS")
              || streq(codeset,"mscode")
              || streq(codeset,"932")
             )
            locale_charset = "SJIS";
          else
          if (   streq(codeset,"eucKR")
              || streq(codeset,"949")
             )
            locale_charset = "eucKR";
          else
          if (streq(codeset,"eucCN"))
            locale_charset = "eucCN";
          else
          if (streq(codeset,"eucTW"))
            locale_charset = "eucTW";
          else
          if (streq(codeset,"TACTIS"))
            locale_charset = "TIS-620"; /* was: "TACTIS"; */
          else
          if (streq(codeset,"EUC") || streq(codeset,"euc")) {
            if (locale[0]=='j' && locale[1]=='a')
              locale_charset = "eucJP";
            else if (locale[0]=='k' && locale[1]=='o')
              locale_charset = "eucKR";
            else if (locale[0]=='z' && locale[1]=='h' && locale[2]=='_') {
              if (locale[3]=='C' && locale[4]=='N')
                locale_charset = "eucCN";
              else if (locale[3]=='T' && locale[4]=='W')
                locale_charset = "eucTW";
            }
          }
          else
          // The following are CLISP extensions.
          if (   streq(codeset,"UTF-8")
              || streq(codeset,"utf8")
             )
            locale_charset = "UTF-8";
        } else {
          // No dot found. Choose a default, based on locale.
          if (   streq(locale,"iso_8859_1")
              || streq(locale,"ISO8859-1")
              || streq(locale,"ISO-8859-1")
             )
            locale_charset = "ISO-8859-1";
          else
          if (0)
            locale_charset = "ISO-8859-2";
          else
          if (0)
            locale_charset = "ISO-8859-5";
          else
          if (0)
            locale_charset = "ISO-8859-6";
          else
          if (0)
            locale_charset = "ISO-8859-7";
          else
          if (0)
            locale_charset = "ISO-8859-8";
          else
          if (0)
            locale_charset = "ISO-8859-9";
          else
          if (0)
            locale_charset = "KOI8-R";
          else
          if (0)
            locale_charset = "KOI8-U";
          else
          if (0)
            locale_charset = "eucJP";
          else
          if (0)
            locale_charset = "ISO-2022-JP"; /* was: "JIS7"; */
          else
          if (0)
            locale_charset = "SJIS";
          else
          if (0)
            locale_charset = "eucKR";
          else
          if (streq(locale,"zh_CN") || streq(locale,"zh")
             )
            locale_charset = "eucCN";
          else
          if (streq(locale,"zh_TW")
             )
            locale_charset = "eucTW";
          else
          if (0)
            locale_charset = "TIS-620"; /* was: "TACTIS"; */
          else {
            // Choose a default, based on the language only.
            const char* underscore = strchr(locale,'_');
            const char* lang;
            if (underscore) {
              size_t n = underscore - locale;
              memcpy(buf,locale,n);
              buf[n] = '\0';
              lang = buf;
            } else {
              lang = locale;
            }
            if (   streq(lang,"af") || streq(lang,"afrikaans")
                || streq(lang,"ca") || streq(lang,"catalan")
                || streq(lang,"da") || streq(lang,"danish") || streq(lang,"dansk")
                || streq(lang,"de") || streq(lang,"german") || streq(lang,"deutsch")
                || streq(lang,"en") || streq(lang,"english")
                || streq(lang,"es") || streq(lang,"spanish")
                                    #ifndef ASCII_CHS
                                    || streq(lang,"espa\361ol") || streq(lang,"espa\303\261ol")
                                    #endif
                || streq(lang,"eu") || streq(lang,"basque")
                || streq(lang,"fi") || streq(lang,"finnish")
                || streq(lang,"fo") || streq(lang,"faroese") || streq(lang,"faeroese")
                || streq(lang,"fr") || streq(lang,"french")
                                    #ifndef ASCII_CHS
                                    || streq(lang,"fran\347ais") || streq(lang,"fran\303\247ais")
                                    #endif
                || streq(lang,"ga") || streq(lang,"irish")
                || streq(lang,"gd") || streq(lang,"scottish")
                || streq(lang,"gl") || streq(lang,"galician")
                || streq(lang,"is") || streq(lang,"icelandic")
                || streq(lang,"it") || streq(lang,"italian")
                || streq(lang,"nl") || streq(lang,"dutch")
                || streq(lang,"no") || streq(lang,"norwegian")
                || streq(lang,"pt") || streq(lang,"portuguese")
                || streq(lang,"sv") || streq(lang,"swedish")
               )
              locale_charset = "ISO-8859-1";
            else
            if (   streq(lang,"cs") || streq(lang,"czech")
                || streq(lang,"cz")
                || streq(lang,"hr") || streq(lang,"croatian")
                || streq(lang,"hu") || streq(lang,"hungarian")
                || streq(lang,"pl") || streq(lang,"polish")
                || streq(lang,"ro") || streq(lang,"romanian") || streq(lang,"rumanian")
                || streq(lang,"sh") /* || streq(lang,"serbocroatian") ?? */
                || streq(lang,"sk") || streq(lang,"slovak")
                || streq(lang,"sl") || streq(lang,"slovene") || streq(lang,"slovenian")
                || streq(lang,"sq") || streq(lang,"albanian")
               )
              locale_charset = "ISO-8859-2";
            else
            if (   streq(lang,"eo") || streq(lang,"esperanto")
                || streq(lang,"mt") || streq(lang,"maltese")
               )
              locale_charset = "ISO-8859-3";
            else
            if (   streq(lang,"be") || streq(lang,"byelorussian")
                || streq(lang,"bg") || streq(lang,"bulgarian")
                || streq(lang,"mk") || streq(lang,"macedonian")
                || streq(lang,"sp")
                || streq(lang,"sr") || streq(lang,"serbian")
               )
              locale_charset = "ISO-8859-5";
            else
            if (streq(lang,"ar") || streq(lang,"arabic")
               )
              locale_charset = "ISO-8859-6";
            else
            if (streq(lang,"el") || streq(lang,"greek")
               )
              locale_charset = "ISO-8859-7";
            else
            if (streq(lang,"iw") || streq(lang,"he") || streq(lang,"hebrew")
               )
              locale_charset = "ISO-8859-8";
            else
            if (streq(lang,"tr") || streq(lang,"turkish")
               )
              locale_charset = "ISO-8859-9";
            else
            if (   streq(lang,"et") || streq(lang,"estonian")
                || streq(lang,"lt") || streq(lang,"lithuanian")
                || streq(lang,"lv") || streq(lang,"latvian")
               )
              locale_charset = "ISO-8859-10";
            else
            if (streq(lang,"ru") || streq(lang,"russian")
               )
              locale_charset = "KOI8-R";
            else
            if (streq(lang,"uk") || streq(lang,"ukrainian")
               )
              locale_charset = "KOI8-U";
            else
            if (   streq(lang,"ja")
                || streq(lang,"Jp")
                || streq(lang,"japan")
                || streq(lang,"Japanese-EUC")
               )
              locale_charset = "eucJP";
            else
            if (0)
              locale_charset = "ISO-2022-JP"; /* was: "JIS7"; */
            else
            if (streq(lang,"japanese")
               )
              locale_charset = "SJIS";
            else
            if (streq(lang,"ko") || streq(lang,"korean")
               )
              locale_charset = "eucKR";
            else
            if (streq(lang,"chinese-s")
               )
              locale_charset = "eucCN";
            else
            if (streq(lang,"chinese-t")
               )
              locale_charset = "eucTW";
            else
            if (streq(lang,"th")
               )
              locale_charset = "TIS-620"; /* was: "TACTIS"; */
            else {
            }
          }
        }
        free(buf);
      }
      return locale_charset;
    }


/*
    *************************************************************************************
	ndb_unix_setFileSize64()

	gets the file size using 64 Bit functions and sets the file size entry
	of file structure pFile
    *************************************************************************************
*/

void ndb_unix_setFileSize64(PFILEENTRY pFile)
{
    struct stat64 mystat64;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_setFileSize64 - startup\n");
    fflush(stdout);



    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_setFileSize64: previous file size is %.0f\n", (double) pFile->lenOri8);
    fflush(stdout);

    if (stat64(pFile->filenameExt, &mystat64) != -1)
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_unix_setFileSize64: file size %9llu / 0x%09llX\n", (ULONG8) mystat64.st_size, (ULONG8) mystat64.st_size);
        fflush(stdout);

        pFile->lenOri8 = (ULONG8) mystat64.st_size;

        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_unix_setFileSize64: set file size to %.0f\n", (double) pFile->lenOri8);
        fflush(stdout);
    }
    else
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_unix_setFileSize64: stat64 failed, perror() told: %s\n", strerror (errno));
        fflush(stdout);

    }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_unix_setFileSize64: file size now %.0f\n", (double) pFile->lenOri8);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_unix_setFileSize64 - finished\n");
    fflush(stdout);

    return;
}

/*
    *************************************************************************************
	ndb_unix_version_local()

	Code from Info-Zip project; prints out the compiler, version and date of NDB
    *************************************************************************************
*/

void ndb_unix_version_local()
{
    static char CompiledWith[] = "Compiled with %s%s for %s%s%s%s.\n\n";
#if (defined(__GNUC__) && defined(NX_CURRENT_COMPILER_RELEASE))
    char cc_namebuf[40];
    char cc_versbuf[40];
#else
#if (defined(CRAY) && defined(_RELEASE))
    char cc_versbuf[40];
#endif
#endif
#if ((defined(CRAY) || defined(cray)) && defined(_UNICOS))
    char os_namebuf[40];
#else
#if defined(__NetBSD__)
    char os_namebuf[40];
#endif
#endif

    /* Pyramid, NeXT have problems with huge macro expansion, too:  no Info() */
    printf(CompiledWith,

#ifdef __GNUC__
#  ifdef NX_CURRENT_COMPILER_RELEASE
      (sprintf(cc_namebuf, "NeXT DevKit %d.%02d ",
        NX_CURRENT_COMPILER_RELEASE/100, NX_CURRENT_COMPILER_RELEASE%100),
       cc_namebuf),
      (strlen(__VERSION__) > 8)? "(gcc)" :
        (sprintf(cc_versbuf, "(gcc %s)", __VERSION__), cc_versbuf),
#  else
      "gcc ", __VERSION__,
#  endif
#else
#  if defined(CRAY) && defined(_RELEASE)
      "cc ", (sprintf(cc_versbuf, "version %d", _RELEASE), cc_versbuf),
#  else
#  ifdef __VERSION__
#   ifndef IZ_CC_NAME
#    define IZ_CC_NAME "cc "
#   endif
      IZ_CC_NAME, __VERSION__
#  else
#   ifndef IZ_CC_NAME
#    define IZ_CC_NAME "cc"
#   endif
      IZ_CC_NAME, "",
#  endif
#  endif
#endif /* ?__GNUC__ */

#ifndef IZ_OS_NAME
#  define IZ_OS_NAME "Unix"
#endif
      IZ_OS_NAME,

#if defined(sgi) || defined(__sgi)
      " (Silicon Graphics IRIX)",
#else
#ifdef sun
#  ifdef sparc
#    ifdef __SVR4
      " (Sun SPARC/Solaris)",
#    else /* may or may not be SunOS */
      " (Sun SPARC)",
#    endif
#  else
#  if defined(sun386) || defined(i386)
      " (Sun 386i)",
#  else
#  if defined(mc68020) || defined(__mc68020__)
      " (Sun 3)",
#  else /* mc68010 or mc68000:  Sun 2 or earlier */
      " (Sun 2)",
#  endif
#  endif
#  endif
#else
#ifdef __hpux
      " (HP/UX)",
#else
#ifdef __osf__
      " (DEC OSF/1)",
#else
#ifdef _AIX
      " (IBM AIX)",
#else
#ifdef aiws
      " (IBM RT/AIX)",
#else
#if defined(CRAY) || defined(cray)
#  ifdef _UNICOS
      (sprintf(os_namebuf, " (Cray UNICOS release %d)", _UNICOS), os_namebuf),
#  else
      " (Cray UNICOS)",
#  endif
#else
#if defined(uts) || defined(UTS)
      " (Amdahl UTS)",
#else
#ifdef NeXT
#  ifdef mc68000
      " (NeXTStep/black)",
#  else
      " (NeXTStep for Intel)",
#  endif
#else              /* the next dozen or so are somewhat order-dependent */
#ifdef LINUX
#  ifdef __ELF__
      " (Linux ELF)",
#  else
      " (Linux a.out)",
#  endif
#else
#ifdef MINIX
      " (Minix)",
#else
#ifdef M_UNIX
      " (SCO Unix)",
#else
#ifdef M_XENIX
      " (SCO Xenix)",
#else
#ifdef __NetBSD__
#  ifdef NetBSD0_8
      (sprintf(os_namebuf, " (NetBSD 0.8%c)", (char)(NetBSD0_8 - 1 + 'A')),
       os_namebuf),
#  else
#  ifdef NetBSD0_9
      (sprintf(os_namebuf, " (NetBSD 0.9%c)", (char)(NetBSD0_9 - 1 + 'A')),
       os_namebuf),
#  else
#  ifdef NetBSD1_0
      (sprintf(os_namebuf, " (NetBSD 1.0%c)", (char)(NetBSD1_0 - 1 + 'A')),
       os_namebuf),
#  else
      (BSD4_4 == 0.5)? " (NetBSD before 0.9)" : " (NetBSD 1.1 or later)",
#  endif
#  endif
#  endif
#else
#ifdef __FreeBSD__
      (BSD4_4 == 0.5)? " (FreeBSD 1.x)" : " (FreeBSD 2.0 or later)",
#else
#ifdef __bsdi__
      (BSD4_4 == 0.5)? " (BSD/386 1.0)" : " (BSD/386 1.1 or later)",
#else
#ifdef __386BSD__
      (BSD4_4 == 1)? " (386BSD, post-4.4 release)" : " (386BSD)",
#else
#ifdef __CYGWIN__
      " (Cygwin)",
#else
#if defined(i686) || defined(__i686) || defined(__i686__)
      " (Intel 686)",
#else
#if defined(i586) || defined(__i586) || defined(__i586__)
      " (Intel 586)",
#else
#if defined(i486) || defined(__i486) || defined(__i486__)
      " (Intel 486)",
#else
#if defined(i386) || defined(__i386) || defined(__i386__)
      " (Intel 386)",
#else
#ifdef pyr
      " (Pyramid)",
#else
#ifdef ultrix
#  ifdef mips
      " (DEC/MIPS)",
#  else
#  ifdef vax
      " (DEC/VAX)",
#  else /* __alpha? */
      " (DEC/Alpha)",
#  endif
#  endif
#else
#ifdef gould
      " (Gould)",
#else
#ifdef MTS
      " (MTS)",
#else
#ifdef __convexc__
      " (Convex)",
#else
#ifdef __QNX__
      " (QNX 4)",
#else
#ifdef __QNXNTO__
      " (QNX Neutrino)",
#else
#ifdef Lynx
      " (LynxOS)",
#else
      "",
#endif /* Lynx */
#endif /* QNX Neutrino */
#endif /* QNX 4 */
#endif /* Convex */
#endif /* MTS */
#endif /* Gould */
#endif /* DEC */
#endif /* Pyramid */
#endif /* 386 */
#endif /* 486 */
#endif /* 586 */
#endif /* 686 */
#endif /* Cygwin */
#endif /* 386BSD */
#endif /* BSDI BSD/386 */
#endif /* NetBSD */
#endif /* FreeBSD */
#endif /* SCO Xenix */
#endif /* SCO Unix */
#endif /* Minix */
#endif /* Linux */
#endif /* NeXT */
#endif /* Amdahl */
#endif /* Cray */
#endif /* RT/AIX */
#endif /* AIX */
#endif /* OSF/1 */
#endif /* HP/UX */
#endif /* Sun */
#endif /* SGI */

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

} /* end function version() */



/*
    *************************************************************************************
        ndb_unix_truncate()

        truncate file at given size
    *************************************************************************************
*/

void ndb_unix_truncate(const char *pathfile, ULONG fileSize)
{
    truncate((char *) pathfile, fileSize);
}

