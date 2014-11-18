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
#include <sys/types.h>
#include <sys/stat.h>


// file ndb_extract.c
int ndb_expandExtraStream(struct ndb_s_fileentry *, FILE *, const char *, int);


// file ndb_allocrout.c
long ndb_alloc_init (long i_maxptr);
long ndb_alloc_done (void);
void *ndb_malloc (size_t i_size, char *remark);
void *ndb_calloc (size_t i_nr, size_t i_size, char *remark);
void *ndb_realloc (void *m_ptr, size_t i_size, char *remark);
long ndb_free (void *m_ptr);
long ndb_show_ptr (void);
void ndb_show_status (void);


// file ndb_blockfuncs.c
void ndb_block_datachunk_init(struct ndb_s_c_block *, ULONG, ULONG, ULONG);
void ndb_block_init(int, int);
void ndb_block_setBlockType(struct ndb_s_blockheader *, char, char);
void ndb_block_printHashInfo();
int ndb_block_addBlockToTable(struct ndb_s_blockheader *);
int ndb_block_compareBlockData(struct ndb_s_blockheader *, struct ndb_s_blockheader *);
int ndb_block_compressblock(struct ndb_s_blockheader *);
int ndb_block_uncompressblock(struct ndb_s_blockheader *);
void ndb_block_print(struct ndb_s_blockheader *);
void ndb_block_dumpblock(char *, int);


// file ndb_chapterfuncs.c
void ndb_chap_init(struct ndb_s_c_chapter *, int, const char *, const char *, const char *, const char *);
void ndb_chap_initFileHash(struct ndb_s_c_chapter *);
void ndb_chap_print(struct ndb_s_c_chapter *);
void ndb_chap_printChapterInfo(struct ndb_s_c_chapter *, int);
void ndb_chap_printChapterInfoRaw(struct ndb_s_c_chapter *pChapter);
void ndb_chap_printHashInfo(PCHAPTER);
int ndb_chap_addFileToHash(PCHAPTER, PFILEENTRY);
int ndb_chap_removeFileFromHash(PCHAPTER, PFILEENTRY);
int ndb_chap_addFileEntry(PCHAPTER, PFILEENTRY);
int ndb_chap_removeLastFileEntry(struct ndb_s_c_chapter *);
int ndb_chap_readAllBlockHeader(struct ndb_s_c_chapter *, FILE *);
int ndb_chap_writeAllBlockHeader(struct ndb_s_c_chapter *, FILE *);
int ndb_chap_readAllFileEntries(struct ndb_s_c_chapter *, FILE *);
int ndb_chap_writeAllFileEntries(struct ndb_s_c_chapter *, FILE *);
int ndb_chap_checkFileListWithChapter(struct ndb_s_c_chapter *, struct ndb_l_filelist *);
int ndb_chap_rebuildBlockHeaderList(struct ndb_s_c_chapter *);
int ndb_chap_readAllDataFileHeader(struct ndb_s_c_chapter *, FILE *);
int ndb_chap_writeAllDatFilesHeader(struct ndb_s_c_chapter *, FILE *);
int ndb_chap_addDataFileHeader(PCHAPTER, PDATAFILEHEADER);
int ndb_chap_findfile(struct ndb_s_c_chapter  *, struct ndb_s_fileentry *, struct ndb_s_fileentry **);
// archive functions
void ndb_arc_printArchiveInfoRaw(struct ndb_s_c_archive *);
void ndb_arc_print(struct ndb_s_c_archive *);


// file ndb_dffuncs.c
int ndb_df_init(PARCHIVE pArchive, FILE *, char *);
void ndb_df_setChapNo(int);
int ndb_df_addDataFile(PDATAFILEHEADER);
int ndb_df_checkDataFile(int);
int ndb_df_checkAllDataFile();
PDATAFILEHEADER ndb_df_createNewDataFile(int, char *, char *);
int ndb_df_writeNewDataFile(int, PDATAFILEHEADER, FILE *);
FILE *ndb_df_getReadHandle(int);
FILE *ndb_df_getReadWriteHandle(int);
int ndb_df_getCheckedWriteHandle(FILE **, int *, PCHAPTER, PBLOCKHEADER);
int ndb_df_getCurrWriteFileNo();
int ndb_df_getFileCount();
void ndb_df_addFileSize(int, ULONG);
void ndb_df_setFileSize(int, ULONG);
PDATAFILEHEADER ndb_df_getCurrDataFile(int);
char *ndb_df_getCurrDataFilename(int);
int ndb_df_checkSizeToAdd(int, ULONG);
int ndb_df_closeDataFile(int);
void ndb_df_printDFHeadder(PDATAFILEHEADER, char *);
USHORT ndb_df_getMaxFileHandles();
void ndb_df_setMaxFileHandles(USHORT);


// file ndb_fileentryfuncs.c
#if defined(UNIX)
int ndb_fe_newFileEntry(struct ndb_s_fileentry *, const char *, struct stat64 *, ULONG);
#else
int ndb_fe_newFileEntry(struct ndb_s_fileentry *, const char *, struct stat *, ULONG);
#endif
#if defined(UNIX)
int ndb_fe_setFileEntry(struct ndb_s_fileentry *, struct stat64 *, ULONG);
#else
int ndb_fe_setFileEntry(struct ndb_s_fileentry *, struct stat *, ULONG);
#endif
int ndb_fe_makeFilenameUTF8(struct ndb_s_fileentry *);
int ndb_fe_makeFilenameCP(struct ndb_s_fileentry *);
void ndb_fe_print(struct ndb_s_fileentry *);
void ndb_fe_printFileEntryInfo(struct ndb_s_fileentry *, int);
void ndb_fe_printFileEntryHashAndName(struct ndb_s_fileentry *, int);
void ndb_fe_printFileEntryInfoRaw(struct ndb_s_fileentry *);
void ndb_fe_addRemark(struct ndb_s_fileentry *, char *);
void ndb_fe_addBlock(struct ndb_s_fileentry *, struct ndb_s_blockheader *);
int ndb_fe_writeAllBlockHeader(struct ndb_s_fileentry *, FILE *, ULONG *);
int ndb_file_checkattributes(struct ndb_s_fileentry *, struct ndb_s_fileentry *);
int ndb_file_checkattributeswithstat(struct ndb_s_fileentry *, struct stat *, ULONG);
int ndb_file_checkfileswithcrc(struct ndb_s_fileentry *, struct ndb_s_fileentry *);
int ndb_file_checkfileswithmd5(struct ndb_s_fileentry *, struct ndb_s_fileentry *);
int ndb_file_copyheaderlist(struct ndb_s_c_chapter  *, struct ndb_s_fileentry *, struct ndb_s_fileentry *);
int ndb_file_markpath2extract(PCHAPTER, PFILEENTRY);
int ndb_fe_addExtraHeader(struct ndb_s_fileentry *, struct ndb_s_fileextradataheader *);
void ndb_fe_printExtraHeader(struct ndb_s_fileextradataheader *);
void ndb_fe_printAllExtraHeader(struct ndb_s_fileentry *);
int ndb_fe_createExtraHeader2Buffer(struct ndb_s_fileextradataheader *, char **, ULONG *, FILE *);
int ndb_fe_createBuffer2ExtraHeader(struct ndb_s_fileextradataheader *, char *, ULONG , FILE *);

// file ndb_iofuncs_base.c
int ndb_io_writechunkarchive(FILE *, struct ndb_s_c_archive *);
int ndb_io_readchunkarchive(FILE *, struct ndb_s_c_archive *);
int ndb_io_writechunkchapter(FILE *, struct ndb_s_c_chapter *);
int ndb_io_readchunkchapter(FILE *, struct ndb_s_c_chapter *);
int ndb_io_writechunkblock(FILE *, struct ndb_s_c_block *);
int ndb_io_readchunkblock(FILE *, struct ndb_s_c_block *);
int ndb_io_writeBlockHeader(FILE *, struct ndb_s_blockheader *);
int ndb_io_readBlockHeader(FILE *, struct ndb_s_blockheader *);
int ndb_io_writeBlockZipDataToFile(struct ndb_s_blockheader *, FILE *);
int ndb_io_readBlockZipDataFromFile(struct ndb_s_blockheader *);
int ndb_io_writeBlockOriDataToFile(struct ndb_s_blockheader *, FILE *);
int ndb_io_readBlockOriDataFromFile(struct ndb_s_blockheader *, FILE *);
int ndb_io_readdata(char *, int, FILE *);
int ndb_io_writedata(char *, int, FILE *);
int ndb_io_check_exist_file(const char *, struct stat *);
void ndb_io_doQuadAlign(FILE *);

// file ndb_iofuncs_ext.c
int ndb_io_writeFileEntry(FILE *, struct ndb_s_fileentry *);
int ndb_io_readFileEntry(FILE *, struct ndb_s_fileentry *);
int ndb_io_writeDataFile(FILE *, PDATAFILEHEADER, int);
int ndb_io_readDataFile(FILE *, PDATAFILEHEADER, int);


// file ndb_listrout.c
void ndb_list_initFileList(struct ndb_l_filelist *, int);
void ndb_list_addToFileList(struct ndb_l_filelist *, struct ndb_s_fileentry *);
int ndb_list_addFileToHash(struct ndb_l_filelist *, struct ndb_s_fileentry *);
int ndb_list_removeFileFromHash(struct ndb_l_filelist *, PFILEENTRY);
void ndb_list_delFromFileList(struct ndb_l_filelist *, struct ndb_s_fileentry *);
PFILEENTRY ndb_list_removeFirstElement(struct ndb_l_filelist *);
void ndb_list_mixToFileList(struct ndb_l_filelist *, struct ndb_s_fileentry *);
void ndb_list_excludeFileMask(struct ndb_l_filelist *, const char *);
ULONG ndb_list_getFileCount(struct ndb_l_filelist *pFileList);
struct ndb_s_fileentry *ndb_list_getFirstFile(struct ndb_l_filelist *);
struct ndb_s_fileentry *ndb_list_getNextFile(struct ndb_l_filelist *);
struct ndb_s_fileentry *ndb_list_getLastFile(struct ndb_l_filelist *);
void ndb_list_initFileMaskList(struct ndb_l_filemasklist *);
void ndb_list_addToFileMaskList(struct ndb_l_filemasklist *, char *, char);
USHORT ndb_list_getFileMaskCount(struct ndb_l_filemasklist *);
char *ndb_list_getFileMask(struct ndb_l_filemasklist *, USHORT);
char ndb_list_getFileMaskMode(struct ndb_l_filemasklist *, USHORT);
void ndb_list_checkWithFileMask(struct ndb_s_fileentry *, struct ndb_l_filemasklist *);


// file ndb_osdep.c
void ndb_osdep_init();
void ndb_osdep_done();
void ndb_osdep_split(const char *, char *, char *);
void ndb_osdep_makeFileList(char *, char *, int, struct ndb_l_filelist *, struct ndb_l_filemasklist *, ULONG *);
void ndb_osdep_addFileToFileList(struct ndb_s_fileentry **ppNewFile, char *pathfile, ULONG dwAttr, struct ndb_l_filelist *pFileList);
int ndb_osdep_EnhancedMaskTest(const char *, int, const char *, int);
#if defined(USE_FHANDLE_INT)
int ndb_osdep_createFile(struct ndb_s_fileentry *, const char *, int *);
#else
int ndb_osdep_createFile(struct ndb_s_fileentry *, const char *, FILE **);
#endif
void ndb_osdep_truncate(const char *, ULONG);
int ndb_osdep_setExtraData(struct ndb_s_fileentry *, const char *, FILE *, int, char *);
int ndb_osdep_setMetaData(struct ndb_s_fileentry *, const char *, FILE *, int, int, char *);
int ndb_osdep_createPath(struct ndb_s_fileentry *, const char *);
int ndb_osdep_getFileMode(char *);
void ndb_osdep_setFileSize64(PFILEENTRY);
int ndb_osdep_mapNdbAttributes2OS(ULONG);
char *ndb_osdep_makeIntFileName(const char *);
char *ndb_osdep_makeExtFileName(const char *);
void ndb_osdep_sleep(int);
char *ndb_osdep_createBatchImportAll_fname();
void ndb_osdep_createBatchImportAll_start(FILE *, char *, struct ndb_s_c_archive *);
void ndb_osdep_createBatchImportAll_chapter(FILE *,  char *, char *, struct ndb_s_c_chapter *);
void ndb_osdep_createBatchImportAll_end(FILE *,  char *);
int ndb_osdep_matchFilename(const char *, const char *);
void ndb_osdep_newFileEntry(PFILEENTRY, struct stat *);
int ndb_osdep_getExtraData(PFILEENTRY, struct ndb_s_blockheader **,
        struct ndb_s_fileextradataheader **, FILE *, char *);
char *ndb_osdep_getOS();
char *ndb_osdep_getCodepage();
ULONG8 ndb_osdep_getPreciseTime();
void ndb_osdep_version_local();

// file ndb_packrout.c
USHORT ndb_pack (struct ndb_s_blockheader *);
USHORT ndb_unpack (struct ndb_s_blockheader *);


// file ndb_utf8.c
void ndb_utf8_inittables(const char *);
char *ndb_utf8_getConvertedCP();
char *ndb_utf8_createStringUTF8(const char *);
char *ndb_utf8_createStringCP(const char *);
char *ndb_utf8_aliasWinCP(const char *);

// file ndb_utilfuncs_base.c
int ndb_write_struct(FILE *, void *, ULONG);
int ndb_read_struct(FILE *, void *, ULONG);
int ndb_write2buf_char(char *, int *, char);
int ndb_write2buf_ushort(char *, int *, USHORT);
int ndb_write2buf_ulong(char *, int *, ULONG);
int ndb_write2buf_ulong8(char *, int *, ULONG8);
int ndb_write2buf_char_n(char *, int *, char *, int);
int ndb_write2buf_byte_n(char *, int *, char *, int);
char ndb_readbuf_char(char *, int *);
USHORT ndb_readbuf_ushort(char *, int *);
ULONG ndb_readbuf_ulong(char *, int *);
ULONG8 ndb_readbuf_ulong8(char *, int *);
char *ndb_readbuf_char_n(char *, int *, int);
char *ndb_readbuf_byte_n(char *, int *, int);
char *ndb_util_msgcode2print(int);
char ndb_util_prompt_user(const char *, const char *);
char *ndb_util_formatraw(char *);
void ndb_util_cleanIntPath(char *);
void ndb_setBlockSize(USHORT);
USHORT ndb_getBlockSize();
void ndb_setCompression(USHORT);
USHORT ndb_getCompression();
void ndb_setDebugLevel(int);
void ndb_printTimeMillis();
void ndb_printDebug(const char *);
USHORT ndb_crc16(USHORT, unsigned char *, int);
ULONG ndb_crc32(ULONG , unsigned char *, ULONG);
char *ndb_util_printMD5(unsigned char *);
#if defined (NDB_DEBUG_OFF)
#define ndb_getDebugLevel()                  0
#else
int ndb_getDebugLevel();
#endif
#if defined(TEST_PERFORMANCE)
void ndb_util_set_performancetest(int, char *);
void ndb_util_print_performancetest();
#endif

// file wildcards.c
int wildcardfit (char *wildcard, char *test);

//file MD5.c
#define MD5_API
#define MD5_DIGESTSIZE  16
typedef struct
{
  unsigned int state[4];        // state (ABCD)
  unsigned int count[2];        // number of bits, modulo 2^64 (lsb first)
  unsigned char buffer[64];     // input buffer
}
MD5CTX, *PMD5CTX;
void MD5_API MD5_Initialize (PMD5CTX);
void MD5_API MD5_Update (PMD5CTX, const void*, unsigned int);
void MD5_API MD5_Final (unsigned char[MD5_DIGESTSIZE], PMD5CTX);



/* ======================================================================== */
/* OS dependend functions                                                   */
/* ======================================================================== */


#if defined(OS2) /* ------------------------------------------------------- */

#include "os2/ndb_os2.h"

// file os2/ndb_os2.c
OS2DIR *ndb_os2_opendir(const char *, USHORT);
struct os2dirent *ndb_os2_readdir(OS2DIR *);
char *ndb_os2_readd(OS2DIR *);
void ndb_os2_closedir(OS2DIR *);
void ndb_os2_close_outfile(struct ndb_s_fileentry *, const char *, FILE *);
int ndb_os2_create_path(const char *);
ULONG ndb_os2_getFileMode(char *);
ULONG ndb_os2_filetime(char *, ULONG *, ULONG *, iztimes *);
void ndb_os2_setFileSize64(PFILEENTRY);
void ndb_os2_SetPathAttrTimes(struct ndb_s_fileentry *, const char *sDefaultPath);
void ndb_os2_sleep(int);
char *ndb_os2_getCodepage();
void ndb_os2_init_upper();
char *ndb_os2_StringLower(char *);
char *ndb_os2_GetOS2Version();
ULONG ndb_os2_ndbattr2os2attr(ULONG);
ULONG ndb_os2_os2attr2ndbattr(ULONG);
void ndb_os2_truncate(const char *, ULONG);
void ndb_os2_setFileSize64(PFILEENTRY);
void ndb_os2_version_local();


// file os2/ndb_os2util.c
void ndb_os2_split(const char *, char *, char *);
int ndb_os2_makeFileList(char *, char *, int, struct ndb_l_filelist *, struct ndb_l_filemasklist *, ULONG *);
int ndb_os2_EnhancedMaskTest(const char *, int, const char *, int);
char *ndb_os2_makeIntFileName(const char *);
char *ndb_os2_makeExtFileName(const char *);


// file os2/ndb_os2ea.c
int ndb_os2_getExtraData(struct ndb_s_fileentry *, struct ndb_s_fileextradataheader **, FILE *, struct ndb_s_blockheader **, char *);
int ndb_os2_setEAs(struct ndb_s_fileextradataheader *, char *, FILE *, int);
#endif


#if defined(WIN32) /* ----------------------------------------------------- */

#include "win32/ndb_win32.h"

// file win32/ndb_win32.c
struct ndb_win32_s_direntry *Opendir (const char *);
struct ndb_win32_s_direntry *Readdir (struct ndb_win32_s_direntry *);
void                         Closedir(struct ndb_win32_s_direntry *);
char                         *readd   (struct ndb_win32_s_direntry *);
void ndb_win32_SetPathAttrTimes(struct ndb_s_fileentry *, const char *);
int ndb_win32_getFileMode(char *);
int ndb_win32_create_path(const char *);
void ndb_win32_sleep(int);
char *ndb_win32_getCodepage();
void ndb_win32_init_upper();
char *ndb_win32_StringLower(char *);
char *ndb_win32_ansi2oem(char *);
ULONG ndb_win32_ndbattr2winattr(ULONG);
ULONG ndb_win32_winattr2ndbattr(ULONG);
int ndb_win32_isWinNT();
int ndb_win32_isWinXP();
char *ndb_win32_getWindowsVersion();
char *ndb_win32_getShortName(char *);
int isFileSystemNTFS(char *);
void ndb_win32_truncate(const char *, ULONG);
void ndb_win32_version_local();

// file win32/ndb_win32util.c
void ndb_win32_split(const char *, char *, char *);
int ndb_win32_makeFileList(char *, char *, int, struct ndb_l_filelist *, struct ndb_l_filemasklist *, ULONG *);
int ndb_win32_EnhancedMaskTest(const char *, int, const char *, int);
char *ndb_win32_makeIntFileName(const char *);
char *ndb_win32_makeExtFileName(const char *);
int ndb_win32_getExtraData(struct ndb_s_fileentry *, struct ndb_s_fileextradataheader **, FILE *, struct ndb_s_blockheader **, char *);

// file win32/ndb_nt.c
int ndb_win32_getSD(char *, char **, USHORT *);
int ndb_win32_setSD(struct ndb_s_fileentry *, struct ndb_s_fileextradataheader *, char *, FILE *, int);

// file win32/ndb_xp.c
int ndb_win32_setShortName(char *, char *);
void ndb_win32_setFileSize64(PFILEENTRY);
HANDLE ndb_win32_openFileOri(char *);
void ndb_win32_init();
void ndb_win32_done();
#endif



#if defined(UNIX) /* ----------------------------------------------------- */

#include <dirent.h>

// file unix/ndb_unix.c
int ndb_unix_setPathAttr(struct ndb_s_fileentry *, const char *);
int ndb_unix_setUnixAttributes(struct ndb_s_fileentry *, PFILEEXTRADATAHEADER, char *);
int ndb_unix_getFileMode(char *name, int);
void ndb_unix_setFileSize64(PFILEENTRY);
int ndb_unix_create_path(const char *);
void ndb_unix_sleep(int);
char *ndb_unix_getCodepage();
void ndb_unix_truncate(const char *, ULONG);
void ndb_unix_version_local();


// file unix/ndb_unixutil.c
void ndb_unix_split(const char *, char *, char *);
int ndb_unix_makeFileList(char *, char *, int, struct ndb_l_filelist *, struct ndb_l_filemasklist *, ULONG *);
int ndb_unix_EnhancedMaskTest(const char *, int, const char *, int);
char *ndb_unix_makeIntFileName(const char *);
char *ndb_unix_makeExtFileName(const char *);
int ndb_unix_getExtraData(struct ndb_s_fileentry *, struct ndb_s_fileextradataheader **, FILE *, struct ndb_s_blockheader **, char *);
#endif


