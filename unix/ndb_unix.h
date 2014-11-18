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


/* structure for unix extra fields */

struct ndb_ed_unix
{
    USHORT st_mode;
    USHORT st_nlink;
    USHORT st_uid;
    USHORT st_gid;
    USHORT symlnk_len;
    char *pSymlnk;
};

#define NDB_SAVESIZE_EXTRADATA_UNIX  (2 + 2 + 2 + 2 + 2)
#define NDB_EXTRADATA_UNIX_VER       ((0 << 8) + 4)

typedef struct iztimes
{
   time_t atime;             /* new access time */
   time_t mtime;             /* new modification time */
   time_t ctime;             /* used for creation time; NOT same as st_ctime */
} iztimes;
