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


/* GLOBAL DEFINES TO MODIFY THE ndb EXECUTABLES */
/* switch off DEBUG globaly? */
//#define NDB_DEBUG_OFF
/* use debug wrapper for alloc functions? */
#define NDB_DEBUG_ALLOC_OFF
/* use a hashtable for all filenames of a chapter
   and for all filenames of a file list? */
#define USE_HASHTABLE_CRC_FILENAMES
/* measure time of ndb_filefound() (finding files in a chapter)? */
// #define TEST_PERFORMANCE
/* tell whether you want to use zlib or bzip2
   don't forget to add the bzip2 library in the makefile ... */
#define NDB_USE_ZLIB
//#define NDB_USE_BZIP2
/* use int file handles, not FILE file handles */
#define USE_FHANDLE_INT



/* NDB version */
#define NDB_VERSION               "V0.6.5"
/* restliche NDB-Header */
#define NDB_COPYRIGHT             "(C) Peter Wiesneth 12/2002 - 09/2004"
#define NDB_USING_ZLIB            "zlib-1.2.1"
#define NDB_USING_ZIP             "zip-2.3"
#define NDB_USING_UNZIP           "unzip-5.50"

/* supported OS */
#define NDB_OS_CODE_OS2           0x600
#define NDB_OS_CODE_WIN32         0xb00
#define NDB_OS_CODE_UNIX          0x300

/* supported file extra data */
#define NDB_EF_OS2EA     0x0009   /* OS/2 Extra Field ID (extended attributes) */
#define NDB_EF_ACL       0x4C41   /* OS/2 ACL Extra Field ID (access control list, "AL") */
#define NDB_EF_NTSD      0x4453   /* NT Security Descriptor Extra Field ID, ("SD") */
#define NDB_EF_WIN83     0x3338   /* Windows 8.3 shortname ("83") */
#define NDB_EF_UNIX      0x5855   /* UNIX Extra Field ID ("UX") */


/* Magics for the different chunks of archive file */
#define NDB_MAGIC_ARCHIV          0x4142444e     // 'NDBA'
#define NDB_MAGIC_CHAPTER         0x4342444e     // 'NDBC'
#define NDB_MAGIC_BLOCKS          0x4242444e     // 'NDBB'
#define NDB_MAGIC_BLOCKHEADER     0x4842444e     // 'NDBH'
#define NDB_MAGIC_FILEHEADER      0x4642444e     // 'NDBF'
#define NDB_MAGIC_DATAFILE        0x4442444e     // 'NDBD'

/* some defaults */
#define NDB_DEFAULT_BLOCKSIZE     61440
#define NDB_DEFAULT_COMPRESSION   5
#define NDB_MAXLEN_FILENAME       260
#define NDB_MAXLEN_FILEMASK       260
#define NDB_MAXCOUNT_FILEMASK     250
#define NDB_MAXLEN_NDBVER         24
#define NDB_MAXLEN_COMMENT        80
#define NDB_MAXLEN_CODEPAGE       40
#define NDB_MAXLEN_CHAPNAME       32
#define NDB_SIZE_BLOCKTABLE       262144  // must be 2^n
#define NDB_SIZE_FILESTABLE       65536   // must be 2^n
#define NDB_SIZE_IOBUFFER         16384
#define NDB_MODE_INCLUDE          'I'
#define NDB_MODE_EXCLUDE          'X'
#define NDB_READMAXRETRIES        5
#define NDB_READFIRSTWAIT         25
#define NDB_READMAXWAIT           500


/* return codes */
#define NDB_MSG_OK                     0
#define NDB_MSG_NOTOK                  (-1)
#define NDB_MSG_NOMEMORY               (-2)
#define NDB_MSG_NOFILEOPEN             (-3)
#define NDB_MSG_BADMAGIC               (-4)
#define NDB_MSG_READERROR              (-5)
#define NDB_MSG_WRITEERROR             (-6)
#define NDB_MSG_NULLERROR              (-7)
#define NDB_MSG_EOF                    (-8)
#define NDB_MSG_NOPTRFOUND             (-9)
#define NDB_MSG_PTRNOTVALID            (-10)
#define NDB_MSG_PTROVERWRITTEN         (-11)
#define NDB_MSG_ILLEGALRETURN          (-12)
#define NDB_MSG_ZLIBERROR              (-13)
#define NDB_MSG_NODIRCREATE            (-14)
#define NDB_MSG_NOTFOUND               (-15)
#define NDB_MSG_EXTRAFAILED            (-16)
#define NDB_MSG_BADVERSION             (-17)
#define NDB_MSG_CORRUPTDATA            (-18)
#define NDB_MSG_EXCEEDSSIZE            (-19)
#define NDB_MSG_IDENTERROR             (-20)
#define NDB_MSG_SYMLINKFAILED          (-21)
#define NDB_MSG_DUPLICATEFILENAME      (-22)
#define NDB_MSG_OTHERCOMPRESSIONALGO   (-23)
#define NDB_MSG_CRCERROR               (-24)
#define NDB_MSG_FILETOOLARGE           (-25)

/* action codes */
#define NDB_ACTION_IGNORE         'I'
#define NDB_ACTION_ADD            'A'
#define NDB_ACTION_UPDATE         'U'
#define NDB_ACTION_KEEP           'K'
#define NDB_ACTION_EXTRACT        'X'
#define NDB_ACTION_FAILED         'F'
#define NDB_ACTION_FAILEDEXTRA    'E'

/* codes for 'file compare mode' */
#define NDB_FILECOMPARE_EXACT     0
#define NDB_FILECOMPARE_FAST      1
#define NDB_FILECOMPARE_CRC32     2
#define NDB_FILECOMPARE_MD5       3

/* codes for 'query mode' */
#define NDB_QUERYMODE_PROMPT      0
#define NDB_QUERYMODE_DEFAULT     1
#define NDB_QUERYMODE_YES         2
#define NDB_QUERYMODE_NO          3

/* codes for 'overwrite existing files?' */
#define NDB_OVERWRITE_ASK         0
#define NDB_OVERWRITE_YES         1
#define NDB_OVERWRITE_NO          2
#define NDB_OVERWRITE_ALL         3
#define NDB_OVERWRITE_NONE        4
#define NDB_OVERWRITE_MODIFIED    5

/* modes for ndbextract */
#define NDB_EXTRACTMODE_DEFAULT          0
#define NDB_EXTRACTMODE_MODIFIEDONLY     1

/* block header infos */
#define NDB_BLOCKINFO_FIRST       1
#define NDB_BLOCKINFO_LAST        2
#define NDB_BLOCKINFO_EXTRADATA   4
#define NDB_BLOCKINFO_DUPLICATEDATA 8
/* block header types */
#define NDB_BLOCKTYPE_FILEDATA    0
#define NDB_BLOCKTYPE_OS2EAS      1
#define NDB_BLOCKTYPE_NTSECURITY  2
#define NDB_BLOCKTYPE_UNIX        3
#define NDB_BLOCKTYPE_OS2ACLS     4

/* Fileflags */
#define NDB_FILEFLAG_HASEXTRA     1
#define NDB_FILEFLAG_NOUTF8       2
#define NDB_FILEFLAG_IDENTICAL    4

/* datafile flags */
#define NDB_DATAFILE_NEW          0
#define NDB_DATAFILE_CLOSED       1
#define NDB_DATAFILE_OPENREAD     2
#define NDB_DATAFILE_OPENREADWRITE 4
#define NDB_DATAFILE_MAXSIZE      8

#define NDB_DATAFILE_AGEMAX       4294967290UL
#define NDB_DATAFILE_HITSMAX      4294967290UL
#if defined(OS2)
#define NDB_DATAFILE_HANDLESMAX   25
#else
#define NDB_DATAFILE_HANDLESMAX   50
#endif


/* file systems */
#define NDB_FSTYPE_UNKNOWN        255
#define NDB_FSTYPE_FAT            0
#define NDB_FSTYPE_AMIGA          1
#define NDB_FSTYPE_OPENVMS        2
#define NDB_FSTYPE_UNIX           3
#define NDB_FSTYPE_VMSCMS         4
#define NDB_FSTYPE_ATARIST        5
#define NDB_FSTYPE_HPFS           6
#define NDB_FSTYPE_MACINTOSH      7
#define NDB_FSTYPE_NTFS           10
#define NDB_FSTYPE_VFAT           14
#define NDB_FSTYPE_UNIX           3

/* file mode flags */
#define NDB_FILEFLAG_RONLY     0x01
#define NDB_FILEFLAG_HIDDEN    0x02
#define NDB_FILEFLAG_SYSTEM    0x04
#define NDB_FILEFLAG_DIR       0x10
#define NDB_FILEFLAG_ARCHIVE   0x20
#define NDB_FILEFLAG_SLNK      0x40
#define NDB_FILEFLAG_FILE      0x80
#define NDB_FILEFLAG_CHARDEV  0x100
#define NDB_FILEFLAG_BLKDEV   0x200
#define NDB_FILEFLAG_FIFO     0x400
#define NDB_FILEFLAG_SOCKET   0x800

// some constants
#define NDB_NO_HASHTABLE          0
#define NDB_USE_HASHTABLE         1

// numbers for functions which should be measured
// uncomment '#define TEST_PERFORMANCE' above to do it
#define TEST_PERFORMANCE_MAX                         10
#define TEST_PERFORMANCE_FINDFILE                     0
#define TEST_PERFORMANCE_ADDTOFILELIST                1
#define TEST_PERFORMANCE_MARKPATH2EXTRACT             2
#define TEST_PERFORMANCE_CHECKBYCRC                   3
#define TEST_PERFORMANCE_CHECKBYMD5                   4
#define TEST_PERFORMANCE_CRC32                        5
#define TEST_PERFORMANCE_MD5                          6
#define TEST_PERFORMANCE_PACK                         7
#define TEST_PERFORMANCE_UNPACK                       8


#ifndef O_LARGEFILE
#define O_LARGEFILE 0100000
#endif

#ifndef SEEK_SET
#define SEEK_SET    0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR    1
#endif

#ifndef SEEK_END
#define SEEK_END    2
#endif


#define TRUE        1
#define FALSE       0


/* definitions about compression methods */
#define NDB_COMPRESSION_ZLIB       0
#define NDB_COMPRESSION_BZIP2      1


#if defined(NDB_USE_BZIP2)
#define NDB_CURRENT_COMPRESSION   NDB_COMPRESSION_BZIP2
#endif

#if defined(NDB_USE_ZLIB)
#define NDB_CURRENT_COMPRESSION   NDB_COMPRESSION_ZLIB
#endif

/* definitions about operating systems */

/* defines for watcom */
#if defined(__WATCOMC__)
#ifdef __NT__
#define WIN32 1
#endif
#if defined(__UNIX__)
#define UNIX
#endif
#endif

/* general defines for OS/2 */
#if defined(__OS2__)
#define OS2
#endif

/* general defines for linux */
#if defined(linux) || defined(__linux__)
#ifndef UNIX
#define UNIX
#endif
#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#endif

#if defined(WIN32)
#include <windows.h>
#include "win32/ndb_win32.h"
#define NDB_OS_CODE    NDB_OS_CODE_WIN32
#endif

// GCC under MinGW (Win32) prints console output with ansi code,
// therefore conversion to "Dos"-/OEM format neccessary
#if defined(WIN32) && defined (__MINGW32__)
#define EMXWIN32CONVERT(filename) ndb_win32_ansi2oem(filename)
#else
// all other OS and compiler don't need to do anything
#define EMXWIN32CONVERT(filename) (filename)
#endif


#if defined(OS2)
#include <os2.h>
#ifndef __WATCOMC__
#include <os2emx.h>
#endif
#include "os2/ndb_os2.h"
#define NDB_OS_CODE    NDB_OS_CODE_OS2
#endif


#if defined(UNIX)
#define NDB_OS_CODE    NDB_OS_CODE_UNIX
#endif

/* define stat() functions similar to InfoZip's zip */
extern int opt_arcsymlinks;
#ifndef SSTAT
#  define SSTAT      stat
#endif
#ifdef UNIX
#  define LSTAT      lstat
#  define LSSTAT(n, s)  (opt_arcsymlinks ? lstat((n), (s)) : SSTAT((n), (s)))
#  define LSSTAT64(n, s)  (opt_arcsymlinks ? lstat64((n), (s)) : stat64((n), (s)))
#else
#  define LSTAT      SSTAT
#  define LSSTAT     SSTAT
#endif


// save memory to consume less memory at huge file lists to archive
#ifdef __GNUC__
#if !defined(UNIX)
#pragma pack(1)
#endif
#endif

#if defined(UNIX) || defined(__WATCOMC__)
typedef unsigned short USHORT;
typedef unsigned int   UINT;
typedef unsigned long  ULONG;
#endif
typedef unsigned long long    ULONG8;

typedef    struct ndb_s_c_archive            *PARCHIVE;
typedef    struct ndb_s_c_chapter            *PCHAPTER;
typedef    struct ndb_s_c_block              *PBLOCK;
typedef    struct ndb_s_blockheader          *PBLOCKHEADER;
typedef    struct ndb_s_fileentry            *PFILEENTRY;
typedef    struct ndb_s_fileextradataheader  *PFILEEXTRADATAHEADER;
typedef    struct ndb_s_datafileheader       *PDATAFILEHEADER;


/* structure for list handling of file entries */

struct ndb_l_filelist
{
    ULONG fileCount;
    struct ndb_s_fileentry *pFirstFile;
    struct ndb_s_fileentry *pCurrFile;
    struct ndb_s_fileentry *pLastFile;
    void   **ppHashtableFiles;
};


/* structure for list handling of file masks */

struct ndb_l_filemasklist
{
    USHORT maskCount;
    char *pMask;
        char mode[NDB_MAXLEN_FILEMASK];
};


/* struct describing the archive start */

struct ndb_s_c_archive
{
    ULONG  magic;                   /* SAVE: 04B: magic for archive header */
    USHORT verChunk;                /* SAVE: 02B: chunk format version */
    USHORT lenChunk;                /* SAVE: 02B: length of rest of chunk */
    USHORT crc16Hea;                /* SAVE: 02B: CRC16 of header structure data */
    USHORT blockSize;               /* SAVE: 02B: blocksize for data blocks */
// TODO: compression aufbrechen in zwei Felder: compressionmethod & compressionlevel?
    USHORT compression;             /* SAVE: 02B: compression level of zlib */
    USHORT noOfChapters;            /* SAVE: 02B: number of included chapters */
    ULONG8 archiveSize;             /* SAVE: 08B: size of complete archive without data files */
    ULONG8 archiveSizeReal;         /* SAVE: 08B: size of complete archive incl. data files */
    ULONG  firstChapter;            /* SAVE: 04B: start of first chapter */
    ULONG  lastChapter;             /* SAVE: 04B: start of last chapter */
    USHORT dataFilesCount;          /* SAVE: 02B: number of data files  */
    ULONG  dataFilesSize;           /* SAVE: 04B: max. size of data files  */
    ULONG  timestamp;               /* SAVE: 04B: creation date of archive file */
    char   ndbver[NDB_MAXLEN_NDBVER];   /* SAVE: 24B: version of NDB as ASCII */
    char   comment[NDB_MAXLEN_COMMENT]; /* SAVE: 80B: comment for archive */
    // Ident soll bei nächster Formatänderung zu 'SAVE: 04B' werden
    ULONG identCRC;                                 /* SAVE: 04B: identifikation of NDB main file */
    // fields only in memory
    ULONG  ownFilePosition;         /* TRANS: own position in archive file */
    void   *pDatFil;                /* TRANS: pointer to list with data files */
};

#define NDB_CHUNKARCHIVE_SIZEALL      (4 + 2 + 2 + 2 + 2 + 2 + 2 + 8 + 8 + 4 + 4 + 2 + 4 + 4 + 24 + 80 + 4)
#define NDB_CHUNKARCHIVE_LENCHUNK     (            2 + 2 + 2 + 2 + 8 + 8 + 4 + 4 + 2 + 4 + 4 + 24 + 80 + 4)
#define NDB_CHUNKARCHIVE_VERCHUNK     ((0 << 8) + 6)

/* struct describing the chapter start */

struct ndb_s_c_chapter
{
    // Daten über Chunk
    ULONG    magic;                           /* SAVE: 04B: magic for chapter header */
    USHORT   verChunk;                        /* SAVE: 02B: chunk format version */
    USHORT   lenChunk;                        /* SAVE: 02B: length of rest of chunk */
    USHORT   crc16Hea;                        /* SAVE: 02B: CRC16 of header structure data */
    ULONG8   chapterSize;                     /* SAVE: 08B: size of chapter without header */
    char     ndbver[NDB_MAXLEN_NDBVER];       /* SAVE: 24B: NDB version (like archive chunk) */
    USHORT   ndb_os;                          /* SAVE: 02B: NDB constant for OS */
    USHORT   flags1;                          /* SAVE: 02B: Flags für zukünftige Erweiterungen */
    USHORT   flags2;                          /* SAVE: 02B: Flags für zukünftige Erweiterungen */

    // Daten über Chapter
    USHORT   chapterNo;                       /* SAVE: 02B: chapter number */
    char     chapterName[NDB_MAXLEN_CHAPNAME];   /* SAVE: 32C: chapter name */
    ULONG    chapterTime;                     /* SAVE: 04B: creation date chapter */
    void     *pNextChapter;                   /* TRANS: link to next chapter */
    char     codepage[NDB_MAXLEN_CODEPAGE];   /* SAVE: 40B: comment for chapter */
    char     comment[NDB_MAXLEN_COMMENT];     /* SAVE: 80B: comment for chapter */
    // Daten über Blockliste
    ULONG8   blockDatLen;                     /* SAVE: 08B: length of block data in NDB main file */
    ULONG8   blockDatLenReal;                 /* SAVE: 08B: real length of zipped blocks */
    ULONG    blockDatSta;                     /* SAVE: 04B: (chunk) start of zipped blocks */
    ULONG    blockDatNo;                      /* SAVE: 04B: number of all blocks */
    // Daten über Block-Header-Liste
    ULONG    blockHeaLen;                     /* SAVE: 04B: length of block header chunk/data */
    ULONG    blockHeaSta;                     /* SAVE: 04B: (chunk) start of block header */
    ULONG    blockHeaNo;                      /* SAVE: 04B: number of all block header */
    void     *pFirstBlockHea;                 /* TRANS: pointer to first zipped block */
    // Daten über File-Entry-Liste
    ULONG    filesListLen;                    /* SAVE: 04B: length of file list */
    ULONG    filesListSta;                    /* SAVE: 04B: (chunk) start of file list */
    ULONG    filesNo;                         /* SAVE: 04B: number of files */
    ULONG    filesNoIdent;                    /* SAVE: 04B: number of files identical to prev chapter */
    ULONG8   allFileLenOri;                   /* SAVE: 08B: length of all (unzipped) files */
    void     *pFirstFile;                     /* TRANS: 04B: pointer to first file of list */
    void     *pLastFile;                      /* TRANS: 04B: pointer to last file of list */
    void     **ppHashtableFiles;              /* TRANS: 04B: pointer to hashtable of file name */
    ULONG    ownFilePosition;                 /* TRANS: own position in archive file */
    // Daten über Data-File-Header-Liste
    USHORT   noMaxDatFil;                     /* SAVE: 02B: highest no of datafile needed to extract */
    USHORT   noNewDatFil;                     /* SAVE: 02B: how many new datafiles? */
    ULONG    dataFilesHeaLen;                 /* SAVE: 04B: length of data files header chunk/data */
    ULONG    staDatFil;                       /* SAVE: 04B: (chunk) start of data files header */
    void     *pDatFil;                        /* TRANS: 04B: pointer to first data file header */
};

#define NDB_CHUNKCHAPTER_SIZEALL      (4 + 2 + 2 + 2 + 8 + 24 + 2 + 2 + 2 + 2 + 32 + 4 + 40 + 80 + 8 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 2 + 2 + 4 + 4)
#define NDB_CHUNKCHAPTER_LENCHUNK     (            2 + 8 + 24 + 2 + 2 + 2 + 2 + 32 + 4 + 40 + 80 + 8 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 2 + 2 + 4 + 4)
#define NDB_CHUNKCHAPTER_VERCHUNK     ((0 << 8) + 6)

/* struct describing the block data or header or data files chunk */

struct ndb_s_c_block
{
    ULONG    magic;                           /* SAVE: 04B: magic for chunk header */
    ULONG    dataSize;                        /* SAVE: 04B: size of block data without header chunk */
    ULONG    dataCount;                       /* SAVE: 04B: number of blocks saved here */
    USHORT   flags1;                          /* SAVE: 02B: Flags für zukünftige Erweiterungen */
    USHORT   flags2;                          /* SAVE: 02B: Flags für zukünftige Erweiterungen */
    // fields only in memory
    ULONG    ownFilePosition;                 /* TRANS: own position in archive file */
};

#define NDB_SAVESIZE_BLOCKCHUNK      (4 + 4 + 4 + 2 + 2)

/* struct describing the block header structure */

struct ndb_s_blockheader
{
    USHORT   crc16Hea;                        /* SAVE: 02B: CRC16 of header structure data */
    ULONG    blockNo;                         /* SAVE: 04B: Blocknummer */
    ULONG    fileNo;                          /* SAVE: 04B: Block gehört zu FileNo */
    USHORT   dataFileNo;                      /* SAVE: 02B: Block gehört zu Datenfile Nr */
    ULONG    crc32;                           /* SAVE: 04B: CRC32 des Blocks */
    USHORT   lenOri;                          /* SAVE: 02B: Blocklänge original */
    USHORT   lenZip;                          /* SAVE: 02B: Blocklänge nach Zippen */
    ULONG    staDat;                          /* SAVE: 04B: Adresse Blockinhalt im Archivefile */
    char     blockInfo;                       /* SAVE: 01B: First/Last/Middle/Data/Extra */
    char     blockType;                       /* SAVE: 01B: Data/EAs/NTSEC */
    // fields only in memory
    ULONG    staHea;                          /* TRANS: Adresse Blockheader im Archivefile */
    USHORT   chapNo;                          /* TRANS: File gehört zu Chapter no */
    char     *BlockDataOri;                   /* TRANS: Zeiger auf unkomprimierte Daten im Speicher */
    char     *BlockDataZip;                   /* TRANS: Zeiger auf komprimierte Daten im Speicher */
    void     *nextBlock;                      /* TRANS: nächster Block im File */
    void     *sameCRC;                        /* TRANS: nächster Block mit gleichem CRC */
};

#define NDB_SAVESIZE_BLOCKHEADER    (2 + 4 + 4 + 2 + 4 + 2 + 2 + 4 + 1 + 1)
#define NDB_BLOCKHEADER_VERCHUNK     ((0 << 8) + 6)

/* struct describing the file entries */

struct ndb_s_fileentry
{
    USHORT crc16Hea;                          /* SAVE: 02B: CRC16 of header structure data */
    ULONG    fileNo;                          /* SAVE: 04B: number of file */
    USHORT   chapNo;                          /* SAVE: 02B: file assigned to chapter chapNo */
    ULONG8   lenOri8;                          /* SAVE: 08B: file length original */
    ULONG8   lenZip8;                          /* SAVE: 08B: file length after zipping */
//    ULONG    crc32;                           /* SAVE: 04B: CRC32 of unzipped file */
    unsigned char md5[16];                    /* SAVE: 16B: md5 checksum for file */
    ULONG    ctime;                           /* SAVE: 04B: File Creation Time */
    ULONG    mtime;                           /* SAVE: 04B: File Modification Time */
    ULONG    atime;                           /* SAVE: 04B: File Last Access Time */
    ULONG    attributes;                      /* SAVE: 04B: file attributes */
    ULONG    blockCount;                      /* SAVE: 04B: number of blocks */
    char     FSType;                          /* SAVE: 01B: file system (like Zip) */
    char     flags;                           /* SAVE: 01B: NDB flags: Bits 3 until 7 still unused */
    USHORT   nr_ofExtraHea;                   /* SAVE: 02B: number of extra data header */
    USHORT   lenExtHea;                       /* SAVE: 02B: length of all extra header */
    USHORT   lenfilenameUTF8;                 /* SAVE: 02B: length of filename in UTF-8 */
    char     *filenameUTF8;                   /* SAVE: ??B: file name in UTF-8, length variable */
    // fields only in memory
    USHORT   lenfilenameExt;                  /* TRANS: length of file name as in OS */
    char     *filenameExt;                    /* TRANS: file name as in OS */
    void     *nextFile;                       /* TRANS: pointer to next file */
    void     *sameCRC;                        /* TRANS: next file with same name CRC */
    void     *parentDir;                      /* TRANS: pointer to directory entry */
    ULONG    children;                        /* TRANS: number of files of directory (incl. sub dirs) */
    void     *firstBlockHeader;               /* TRANS: pointer to first block beader */
    void     *lastBlockHeader;                /* TRANS: pointer to last block header */
    void     *pExtraHeader;                   /* TRANS: pointer to block header list for extra data */
    char     action;                          /* TRANS: 'U'pdate, 'A'dd , 'I'gnore, 'K'eep*/
    char     *remark;                         /* TRANS: field for remarks */
    ULONG    ownFilePosition;                 /* TRANS: own position in archive file */
};

#define NDB_SAVESIZE_FILEENTRY      (2 + 4 + 2 + 8 + 8 + 16 + 4 + 4 + 4 + 4 + 4 + 1 + 1 + 2 + 2 + 2)
#define NDB_FILEENTRY_VERCHUNK     ((0 << 8) + 6)

struct ndb_s_fileextradataheader
{
    USHORT   typExtHea;                  /* SAVE: 02B: type of extra data, e.g. EAs, ACLs, ... */
    USHORT   lenExtHea;                  /* SAVE: 02B: length of complete header including pData[] */
    ULONG    lenOriExtra;                /* SAVE: 04B: file length extra data original */
    ULONG    lenZipExtra;                /* SAVE: 04B: file length extra data after zipping */
    ULONG    crc32Extra;                 /* SAVE: 04B: CRC32 of extra data */
    USHORT   blockCountExtra;            /* SAVE: 02B: number of block header for extra data */
    char     *pData;                     /* SAVE: ??B: direct data, without using data blocks */
    // fields only in memory
    void     *pNextHea;                  /* TRANS: pointer to next header */
    void     *firstBlockHeader;          /* TRANS: pointer to first block beader */
};

#define NDB_SAVESIZE_FILEEXTRAHEADER      (2 + 2 + 4 + 4 + 4 + 2)



struct ndb_s_datafileheader
{
    // fields to save
    USHORT crc16Hea;            // SAVE: 02B: CRC16 of header structure data
    USHORT datFil_No;           // SAVE: 02B: number of data file
    USHORT firstChap;           // SAVE: 02B: first chapter with data here
    USHORT lastChap;            // SAVE: 02B: last chapter with data here
    ULONG  curDataSize;         // SAVE: 04B: current size
    ULONG  maxDataSize;         // SAVE: 04B: maximal allowed size
    USHORT flags1;                           // SAVE: 02B: Flags für zukünftige Erweiterungen
    USHORT flags2;                           // SAVE: 02B: Flags für zukünftige Erweiterungen
    USHORT lenmainfilename;                    // SAVE: 02B: length of file name of main archive file
    char   mainfilename[NDB_MAXLEN_FILENAME];  // SAVE/TRANS: ??B: file name of main archive file
    // Ident soll bei nächster Formatänderung zu 'SAVE: 04B' werden
    ULONG  identCRC;             // SAVE: 04B: identifikation of NDB main file
    // fields only in memory
    FILE   *fileHandle;         // TRANS: file handle
    USHORT lenfilename;       // TRANS: length of own file name
    char   filename[NDB_MAXLEN_FILENAME];   // SAVE/TRANS: own file name
    void   *nxtFileData;        // TRANS: pointer to next data file structure
    USHORT status;              // TRANS: file status (closed, read, readwrite, maxsize, ...)
    ULONG  age;                 // TRANS: last use of datafile was <age> ticks ago
    ULONG  hits;                // TRANS: <hits> times the file was opened
    ULONG  ownFilePositionDF;   // TRANS: position of this structure in data file
    ULONG  ownFilePositionMain; // TRANS: own position in archive file
};

#define NDB_SAVESIZE_DATAFILEHEADER      (2 + 2 + 2 + 2 + 4 + 4 + 2 + 2+ 2 + 4)
#define NDB_DATAFILE_VERCHUNK     ((0 << 8) + 6)
