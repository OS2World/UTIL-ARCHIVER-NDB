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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ndb.h"
#include "ndb_prototypes.h"

#ifdef __MINGW32__
   int _CRT_glob = 0;   /* suppress command line globbing by C RTL */
#endif

#ifdef UNIX
#include <errno.h>
extern int errno;
#endif


// used by other modules also
int currBlockSize   = 0;
int currCompression = 0;
int currMaxFHandles     = NDB_DATAFILE_HANDLESMAX;

// options set by command line parameter
int  opt_hidden      = 0;
int  opt_recursedir  = 0;
int  opt_createbatch = 0;
int  opt_changedfilesonly = 0;
int  opt_extractmode = NDB_EXTRACTMODE_DEFAULT;
int  opt_querymode   = NDB_QUERYMODE_PROMPT;


/*
    *************************************************************************************
    ndb_expandStream()

    gets a fileentry structure, reads all of its block headers from the archive file
    and expandes the block data; the unzipped block data is written to the given file
    handle; checks the CRC of each block and calculates and checks the CRC of the whole
    file (error messages if any CRC doesn't match the saved ones)
    *************************************************************************************
*/

#if defined(USE_FHANDLE_INT)
int ndb_expandStream(struct ndb_s_c_chapter *pChapter, struct ndb_s_fileentry *pFile, int fFile)
#else
int ndb_expandStream(struct ndb_s_c_chapter *pChapter, struct ndb_s_fileentry *pFile, FILE *fFile)
#endif
{
    struct ndb_s_blockheader *pBlock;
    char *pBufferIn   = NULL;
    char *pBufferOut  = NULL;
    int retval = NDB_MSG_OK;
    int retval2 = NDB_MSG_OK;
    ULONG8 lenOri8New = 0;
    ULONG8 lenZip8New = 0;
//    ULONG fileCRC = 0;
    // needed for MD5 calculations
    MD5CTX  hasher;
    unsigned char digest[MD5_DIGESTSIZE];

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_expandStream - startup\n");
    fflush(stdout);

    // allocate memory for input and output buffer
    pBufferIn   = (char *) ndb_calloc(ndb_getBlockSize() + 4096, 1, "ndb_expandStream: block for zip");
    pBufferOut  = (char *) ndb_calloc(ndb_getBlockSize() + 4096, 1, "ndb_expandStream: block for ori");

    if ((pBufferIn == NULL) || (pBufferOut == NULL))
    {
        fprintf(stderr, "ndb_expandStream: couldn't allocate memory (%db) for data buffer\n", ndb_getBlockSize() + 4096);
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }


    if (ndb_getDebugLevel() >= 8)
        ndb_fe_print(pFile);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_expandStream: file '%s' has %lu  blocks to unpack\n",
                        pFile->filenameExt, pFile->blockCount);
    fflush(stdout);

    // init MD5 context
    MD5_Initialize(&hasher);

    // start with first block
    pBlock = pFile->firstBlockHeader;

    if ((pBlock != NULL) && (ndb_getDebugLevel() >= 5))
    {
        fprintf(stdout, "ndb_expandStream: now working with block F%lu/B%lu/D%u, block info %d, block type %d\n",
                         pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo, pBlock->blockInfo, pBlock->blockType);
        fflush(stdout);
    }
    // extract file data
    while((pBlock != NULL) && ((pBlock->blockInfo & NDB_BLOCKINFO_EXTRADATA) != NDB_BLOCKINFO_EXTRADATA))
    {
        if (ndb_getDebugLevel() >= 8)
            ndb_block_print(pBlock);
        fflush(stdout);

        if (ndb_getDebugLevel() >= 5)
            fprintf(stdout, "ndb_expandStream: unpacking file data block (F%lu/B%lu/D%u)\n",
                            pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo);
        fflush(stdout);

        /* set buffer pointers */
        pBlock->BlockDataOri = pBufferOut;
        pBlock->BlockDataZip = pBufferIn;
        /* position to start of block data inside of ndb_io_readBlockZipDataFromFile */
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_expandStream: reading file data block (%X bytes) at file position %lX\n",
                        pBlock->lenZip, pBlock->staDat);
        fflush(stdout);
        retval2 = ndb_io_readBlockZipDataFromFile(pBlock);
        if (retval2 != NDB_MSG_OK)
        {
            fprintf(stdout, "ndb_expandStream: Error reading data (F%lu/B%lu/D%u) from archive file\n",
                            pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo);
            fflush(stdout);
            retval = NDB_MSG_READERROR;
        }
        else
        {
            if (NDB_MSG_OK != (retval2 = ndb_block_uncompressblock(pBlock)))
            {
                fprintf(stderr, "Error: unpacking file data block (F%lu/B%lu/D%u) from file '%s' failed (using %u zero bytes instead)\n",
                                pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo, pFile->filenameExt, pBlock->lenOri);
                fflush(stderr);
                                // create dummy 'original' data for current block
                                memset(pBlock->BlockDataOri, 0, pBlock->lenOri);
                retval = NDB_MSG_CORRUPTDATA;
            }

            // recompute file data for checking at the end
            lenOri8New = lenOri8New + (ULONG8) pBlock->lenOri;
            if ((pBlock->blockInfo & NDB_BLOCKINFO_DUPLICATEDATA) != NDB_BLOCKINFO_DUPLICATEDATA)
                lenZip8New = lenZip8New + (ULONG8) pBlock->lenZip;
//            fileCRC = ndb_crc32(fileCRC, pBlock->BlockDataOri, pBlock->lenOri);
            MD5_Update(&hasher, pBlock->BlockDataOri, pBlock->lenOri);
            // don't write, if -t (only test, no extraction)
#if defined(USE_FHANDLE_INT)
            if (fFile != -1)
            {
                retval2 = (pBlock->lenOri== write(fFile, pBlock->BlockDataOri, pBlock->lenOri)) ? NDB_MSG_OK : NDB_MSG_WRITEERROR;
#else
            if (fFile != NULL)
            {
                retval2 = ndb_io_writeBlockOriDataToFile(pBlock, fFile);
#endif
                    if (retval2 != NDB_MSG_OK)
                    {
                        fprintf(stderr, "ndb_expandStream: Error saving original data from block (F%lu/B%lu/D%u) to new file\n",
                                        pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo);
                        fflush(stderr);
                        retval = NDB_MSG_WRITEERROR;
                    }
                }
        } // retval = ndb_io_readBlockDataFromFile(...)
        // preceed with next block
        pBlock = pBlock->nextBlock;
        if ((ndb_getDebugLevel() >= 5) && (pBlock != NULL))
        {
            fprintf(stdout, "ndb_expandStream: now working with block F%lu/B%lu/D%u, block info %d, block type %d\n",
                             pBlock->fileNo, pBlock->blockNo, pBlock->dataFileNo, pBlock->blockInfo, pBlock->blockType);
            fflush(stdout);
        }
    }

    // set pointer to start of extra blocks if found
    if ((pBlock != NULL) && ((pBlock->blockInfo & NDB_BLOCKINFO_EXTRADATA) == NDB_BLOCKINFO_EXTRADATA))
    {
        if (pFile->pExtraHeader == NULL)
        {
            if (ndb_getDebugLevel() >= 5)
            {
                fprintf(stdout, "ndb_expandStream: warning: pointer to extra data block list was not set for '%s'\n",
                                 pFile->filenameExt);
                fflush(stdout);
            }
            pFile->pExtraHeader = pBlock;
        }
    }

    MD5_Final(digest, &hasher);
    memcpy(pFile->md5, digest, MD5_DIGESTSIZE);

    // do checks from recomputed file data against the saved values
//    if ((fileCRC != pFile->crc32) || (lenOriNew != pFile->lenOri) || (lenZipNew != pFile->lenZip))
    // don't check the zipped lengths also, they may differ if we count duplicate blocks or not
    if ((memcmp(digest, pFile->md5, MD5_DIGESTSIZE) != 0) || (lenOri8New != pFile->lenOri8))
        {
        fprintf(stderr, "unpacking file '%s' didn't result in all original values:\n", pFile->filenameExt);
        fprintf(stderr, "expected: md5 0x%s, length %.0f, zipped %.0f\n", ndb_util_printMD5(pFile->md5), (double) pFile->lenOri8, (double) pFile->lenZip8);
        fprintf(stderr, "     got: md5 0x%s, length %.0f, zipped %.0f\n", ndb_util_printMD5(digest), (double) lenOri8New, (double) lenZip8New);
        fflush (stderr);
        retval = NDB_MSG_CORRUPTDATA;
    }

    // free memory again
    ndb_free(pBufferIn);
    ndb_free(pBufferOut);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_expandStream - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
    copyFileLinesToExcludeList()

    - opens <s_files> as file with a filemask each line
    - add the mask of each line to the filemask list
    - close <s_files>
    *************************************************************************************
*/
void copyFileLinesToExcludeList(char *s_files, struct ndb_l_filemasklist *pfilemasks)
{
    FILE *fList = NULL;
    char sLine[NDB_MAXLEN_FILEMASK];

    if (NDB_MSG_OK != ndb_io_check_exist_file(s_files, NULL))
    {
        fprintf(stderr, "\nError: could not read filemasks from file '%s'\n", s_files);
        fprintf(stderr, "ignoring '-x@%s'\n\n", s_files);
        fflush(stderr);
    }
    else
    {
        fList = fopen(s_files, "r");
        if (fList == NULL)
        {
            fprintf(stderr, "\nError: could not open file '%s': %s\n", s_files, strerror (errno));
            fflush(stderr);
        }
        else
        {
            while(!feof(fList) )
            {
                fgets(sLine, sizeof(sLine), fList);
                if (feof(fList))
                    break;
                // remove EOL character
                while ((sLine[strlen(sLine) - 1] == '\n') || (sLine[strlen(sLine) - 1] == '\n'))
                    sLine[strlen(sLine) - 1] = '\0';
                ndb_list_addToFileMaskList(pfilemasks, ndb_osdep_makeIntFileName(sLine), NDB_MODE_EXCLUDE);
            }
            fclose(fList);
        }
    }
}


/*
    *************************************************************************************
    copyFileLinesToIncludeList()

    - opens <s_files> as file with a filemask each line
    - add the mask of each line to the filemask list
    - close <s_files>
    *************************************************************************************
*/
void copyFileLinesToIncludeList(char *s_files, struct ndb_l_filemasklist *pfilemasks)
{
    FILE *fList = NULL;
    char sLine[NDB_MAXLEN_FILEMASK];

    if (NDB_MSG_OK != ndb_io_check_exist_file(s_files, NULL))
    {
        fprintf(stderr, "\nError: could not read filemasks from file '%s'\n", s_files);
        fprintf(stderr, "ignoring '-i@%s'\n\n", s_files);
        fflush(stderr);
    }
    else
    {
        fList = fopen(s_files, "r");
        if (fList == NULL)
        {
            fprintf(stderr, "\nError: could not open file '%s': %s\n", s_files, strerror (errno));
            fflush(stderr);
        }
        else
        {
            while(!feof(fList) )
            {
                fgets(sLine, sizeof(sLine), fList);
                if (feof(fList))
                    break;
                // remove EOL character
                while ((sLine[strlen(sLine) - 1] == '\n') || (sLine[strlen(sLine) - 1] == '\n'))
                    sLine[strlen(sLine) - 1] = '\0';
                ndb_list_addToFileMaskList(pfilemasks, ndb_osdep_makeIntFileName(sLine), NDB_MODE_INCLUDE);
            }
            fclose(fList);
        }
    }
}

/*
    *************************************************************************************
    set_option()

    processes one option of command line and sets the corresponding global variable(s)

    char *options:                         (IN)    pointer to string with current option
    *************************************************************************************
*/

int opt_debug         = 0;
int opt_extractall    = 0;
int opt_extract_from  = -1;
int opt_extract_to    = -1;
int opt_chapterno     = -1;
int opt_chapterno_reverse = 0;
int opt_exclude       = 0;
int opt_ignoreextra   = 0;
int opt_extractdirs   = 0;
int opt_waitmillis    = 0;
int opt_testintegrity = 0;
int opt_overwrite     = NDB_OVERWRITE_ASK;
char s_batch[NDB_MAXLEN_FILENAME] = "";
char s_path[NDB_MAXLEN_FILENAME] = "";

void set_option (char *options)
{
    int i1;
    char ch, d;
    int retval;
    char text[300];
    char sDummy[NDB_MAXLEN_FILENAME];

    for (i1 = 1; options [i1] != '\0'; i1++)
    {
        ch = options [i1++];
        switch (ch)
        {
            case 'd':                            /* set debug level */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: set debug level\n");
                ch = options [i1];
                if ((ch >= '0') && (ch <= '9'))
                {
                    opt_debug = ch - '0';
                    ndb_setDebugLevel(opt_debug);
                }
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: debug level set to %d\n", opt_debug);
                return;

            case 'c':
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: set chapter\n");
                if (options[i1] == '-')
                {
                    opt_chapterno_reverse = 1;
                    i1++;
                }
                strcpy(sDummy, &options [i1]);
                opt_chapterno = atoi(sDummy);
                if (ndb_getDebugLevel() >= 7)
                    fprintf(stdout, "set_option: chapter no set to%s %d\n",
                                    (opt_chapterno_reverse == 1) ? " (reverse)" : "",
                                    opt_chapterno);
                return;

            case 'a':
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: extract all\n");
                opt_extractall = 1;
                // get chapter no <from>, if existing
                ch = options[i1];
                if (ch != '\0')
                {
                    if (isdigit((int) ch) || (ch == '-'))
                    {
                        // starting with a digit -> a <from> number was given
                        opt_extract_from = 0;
                    }
                    while (isdigit((int) ch))
                    {
                        opt_extract_from = 10 * opt_extract_from + (ch - '0');
                        ch = options[++i1];
                    }
                    // does a '-' follow?
                    if (ch == '-')
                    {
                        // get chapter no <from> if existing
                        ch = options[++i1];
                        if (isdigit((int) ch))
                        {
                            opt_extract_to = 0;
                            while (isdigit((int) ch))
                            {
                                opt_extract_to = 10 * opt_extract_to + (ch - '0');
                                ch = options[++i1];
                            }
                        }
                        else
                        {
                                // after '-' come no digits -> extract all chapters after opt_extract_from
                                opt_extract_to = 65535;
                        }
                    }

                }
                // adapt chapterno bounds if neccessary
                if ((opt_extract_from > -1) && (opt_extract_to < opt_extract_from))
                        opt_extract_to = opt_extract_from;
                if ((opt_extract_to > -1) && (opt_extract_from == -1))
                        opt_extract_from = opt_extract_to;
                if ((opt_extract_to == -1) && (opt_extract_from == -1))
                {
                        opt_extract_from = 0;
                        opt_extract_to = 65535;
                }
                if (ndb_getDebugLevel() >= 2)
                {
                    fprintf(stdout, "extract from chapter %d until %d (included)\n",
                                    opt_extract_from, opt_extract_to);
                    fflush(stdout);
                }
                return;

            case 'b':
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: create batch\n");
                opt_createbatch = 1;
                // get chapter no <from>, if existing
                ch = options[i1];
                if (ch != '\0')
                {
                    if (isdigit((int) ch) || (ch == '-'))
                    {
                        // starting with a digit -> a <from> number was given
                        opt_extract_from = 0;
                    }
                    while (isdigit((int) ch))
                    {
                        opt_extract_from = 10 * opt_extract_from + (ch - '0');
                        ch = options[++i1];
                    }
                    // does a '-' follow?
                    if (ch == '-')
                    {
                        // get chapter no <from> if existing
                        ch = options[++i1];
                        if (isdigit((int) ch))
                        {
                            opt_extract_to = 0;
                            while (isdigit((int) ch))
                            {
                                opt_extract_to = 10 * opt_extract_to + (ch - '0');
                                ch = options[++i1];
                            }
                        }
                        else
                        {
                                // after '-' come no digits -> batch for all chapters after opt_extract_from
                                opt_extract_to = 65535;
                        }
                    }

                }
                // adapt chapterno bounds if neccessary
                if ((opt_extract_from > -1) && (opt_extract_to < opt_extract_from))
                        opt_extract_to = opt_extract_from;
                if ((opt_extract_to > -1) && (opt_extract_from == -1))
                        opt_extract_from = opt_extract_to;
                if ((opt_extract_to == -1) && (opt_extract_from == -1))
                {
                        opt_extract_from = 0;
                        opt_extract_to = 65535;
                }
                if (ndb_getDebugLevel() >= 2)
                {
                    fprintf(stdout, "create conversion batch from chapter %d until %d (included)\n",
                                    opt_extract_from, opt_extract_to);
                    fflush(stdout);
                }
                // create OS dependent filename for the import batch
                strcpy (s_batch, ndb_osdep_createBatchImportAll_fname());
                return;

            case 'e':                            /* extract mode */
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: set extract mode\n");
                ch = options [i1];
                if (ch == 'm')
                {
                    opt_extractmode = NDB_EXTRACTMODE_MODIFIEDONLY;
                }
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: extract mode set to NDB_EXTRACTMODE_MODIFIEDONLY\n");
                return;

            case 'R':
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: extract empty dirs also\n");
                opt_extractdirs = 1;
                i1--;
                break;

            case 'o':
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: set overwrite mode\n");
                opt_overwrite = NDB_OVERWRITE_ASK;
                // second character?
                ch = options[i1];
                if (ch == 'a')
                    opt_overwrite = NDB_OVERWRITE_ALL;
                else if (ch == 'n')
                        opt_overwrite = NDB_OVERWRITE_NONE;
                else if (ch == 'm')
                        opt_overwrite = NDB_OVERWRITE_MODIFIED;
                else
                    fprintf(stdout, "set_option: overwrite mode '%s' unknown\n",  &options[0]);
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: overwrite mode set to %d\n", opt_overwrite);
                return;

            case 'q':
                if (ndb_getDebugLevel() >= 3) fprintf(stdout, "set_option: set query mode\n");
                opt_querymode = NDB_QUERYMODE_PROMPT;
                // second character?
                ch = options[i1];
                if (ch == 'd')
                    opt_querymode = NDB_QUERYMODE_DEFAULT;
                else if (ch == 'n')
                	opt_querymode = NDB_QUERYMODE_NO;
                else if (ch == 'y')
                	opt_querymode = NDB_QUERYMODE_YES;
                else
                    fprintf(stdout, "set_option: query mode '%s' unknown\n",  &options[0]);
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: query mode set to %d\n", opt_querymode);
                return;

            case 'x':
                opt_exclude = 1;
                i1--;
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: exclude mode set\n");
                break;

            case 'i':
                opt_exclude = 0;
                i1--;
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: include mode set\n");
                break;

            case 't':
                opt_testintegrity = 1;
                i1--;
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: testing mode set\n");
                break;

            case 'X':
                opt_ignoreextra = 1;
                i1--;
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: ignore extra data mode set\n");
                break;

            case 'w':
                strcpy(sDummy, &options[i1]);
                opt_waitmillis = atoi(sDummy);
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: wait %d milliseconds\n", opt_waitmillis);
                return;

            case 'p':
                strcpy(s_path, &options[i1]);
                if (strlen(s_path) == 0)
                {
                    fprintf(stderr, "Error: option -p: no path given\n");
                    exit(NDB_MSG_NOTOK);
                }
                else if ((NULL != strchr(s_path, '*')) || (NULL != strchr(s_path, '?')))
                {
                    fprintf(stderr, "Error: option -p: path '%s' may not contain wildcards\n", s_path);
                    exit(NDB_MSG_NOTOK);
                }
                else
                {
                    // cut trailing slash, if not root dir
#ifdef UNIX
                    if (strcmp(s_path, "/") != 0)
#else
                    if (! ((strlen(s_path) == 3) && (strcmp(&s_path[1], ":\\") == 0)))
#endif
                    {
                        if ((s_path[strlen(s_path) - 1] == '\\') || (s_path[strlen(s_path) - 1] == '/'))
                            s_path[strlen(s_path) - 1] = '\0';
                    }
                }

		        // does extract path exist?
		        if (NDB_MSG_OK != ndb_io_check_exist_file(s_path, NULL))
		        {
                    if (opt_querymode == NDB_QUERYMODE_PROMPT)
                    {
                        fprintf(stdout, "The directory '%s' does not exist\n", s_path);
                        snprintf(text, 300 - 1, "Should I create it? (y)es [D], (n)o) ");
                        fflush(stdout);
                        d = ndb_util_prompt_user(text, "ynYN");
                    }
                    else if (opt_querymode == NDB_QUERYMODE_DEFAULT)
                    {
                        d = 'y';
                    }
                    else if (opt_querymode == NDB_QUERYMODE_YES)
                    {
                        d = 'y';
                    }
                    else if (opt_querymode == NDB_QUERYMODE_NO)
                    {
                        d = 'n';
                    }
                    else
                    {
                        d = 'n';
                    }
                    // don't overwrite this file
                    if ((d == 'y') || (d == 'Y'))
                    {
                        // check for trailing '/'
                        strcpy(s_path, ndb_osdep_makeIntFileName(s_path));
                        d = s_path[strlen(s_path) - 1];
                        if ((d != '\\') && (d != '/'))
                            strcat (s_path, "/");
#if defined (WIN32)
                        retval = ndb_win32_create_path(s_path);
#elif defined(OS2)
                        retval = ndb_os2_create_path(s_path);
#elif defined(UNIX)
                        retval = ndb_unix_create_path(s_path);
#endif
                        if (retval != NDB_MSG_OK)
                        {
                                fprintf(stderr, "could not create path to extract '%s'\n", s_path);
                                fflush(stderr);
                                exit (NDB_MSG_NOFILEOPEN);
                        }
                    }
                    else
                    {
                                            fprintf(stdout, "path '%s' was not created\n", s_path);
                                            fflush(stdout);
                                            exit (NDB_MSG_OK);
                    }
                } // if (.. ndb_io_check_exist_file() ..)
                if (ndb_getDebugLevel() >= 7) fprintf(stdout, "set_option: extract to path '%s'\n", s_path);
                return;
            case 'f':                            /* file handle count? */
                ch = options [++i1];
                if (ch == 'h')
                {
                    strcpy(sDummy, &options [i1 + 1]);
                    currMaxFHandles = atoi(sDummy);
                    if (ndb_getDebugLevel() >= 3)
                        fprintf(stdout, "set_option: maximal number of concurrent file handles set to %d\n",
                                        currMaxFHandles);
                }
                else
                {
                    fprintf(stdout, "set_option: ignoring unknown option '-f%c'\n", ch);
                    break;
                }
                return;

            default:
                fprintf(stdout, "set_option: ignoring unknown option '-%c'\n", ch);
                break;
        }
    }
    return;
}


typedef struct ndb_diritem *PNDBDIRITEM;
struct ndb_diritem
{
    struct ndb_s_fileentry *pDir;
    PNDBDIRITEM pNext;
};

/************************/
/*  Function dircomp()  */
/************************/

static int dircomp(const void *a, const void *b)   /* used by qsort(); swiped from Zip */
{
    /* order is significant:  this sorts in reverse order (deepest first) */
    return strcmp(((PNDBDIRITEM)b)->pDir->filenameUTF8, ((PNDBDIRITEM)a)->pDir->filenameUTF8);
}


/*
    *************************************************************************************
    main()

    extracts the given chapters and files (given by command line parameters)
    *************************************************************************************
*/


int main(int argc, char **argv)
{
    char s_container[NDB_MAXLEN_FILENAME] = "";
    char s_files[NDB_MAXLEN_FILENAME] = "";
    FILE *fArchive;
    int flag_zipname      = 0;
    ULONG lDummy = 0;
    char text[300];
    char sDummy[NDB_MAXLEN_FILENAME];
    int i, j;
    struct ndb_l_filemasklist filemasks;
    time_t time_startup;
    time_t time_finished;
    double db_duration = 0;
    char *iobuf = NULL;
    struct ndb_s_c_archive currArchive;
    struct ndb_s_c_chapter *pCurrChapter = NULL;
    struct ndb_s_c_chapter *pAllChapters;
    struct ndb_s_fileentry *pFile;
    int retval;
    int i1;
    ULONG currFilePos;
    char sDefaultPath[NDB_MAXLEN_FILENAME];
    char sFileWithDefault[NDB_MAXLEN_FILENAME];
#if defined(USE_FHANDLE_INT)
    int fileOut = -1;
#else
    FILE *fileOut = NULL;
#endif
    FILE *fBatch  = NULL;
    char c;
    struct stat statFile;
    ULONG ulAttributes;
    // 25Jun03 pw NEW collect all files which couldn't be extracted
    char stime_m[40];
    struct tm *tim_m;
    ULONG ulFilesExtracted = 0;
    ULONG ulFilesFailed = 0;
    ULONG ulFilesExtraFailed = 0;
    // create time string
    char stime[40];
    char smtimeHD[40];
    char smtimeArc[40];
    // list of directories (later needed for sorting)
    PNDBDIRITEM pFirstDirItem = NULL;
    PNDBDIRITEM pLastDirItem = NULL;
    PNDBDIRITEM pCurrDirItem = NULL;
    PNDBDIRITEM pSortedDirItem = NULL;
    ULONG nDirItemCount = 0;
    struct ndb_s_c_block dataFilesChunk;
    int len = strlen(s_container);
    int dot_found = 0;
    ULONG ndbArchivSize = 0L;
    // flags for file extraction yes/no
    int iSkipFile = 0;
    int iExistsFile = 0;
    char *p;

    time_startup = 0;
    time(&time_startup);

    fprintf(stdout, "NDBExtract %s: startup at %s", NDB_VERSION, ctime(&time_startup));
    fprintf(stdout, "%s\n", NDB_COPYRIGHT);
    fprintf(stdout, "Using %s and a few source parts of %s and %s\n", NDB_USING_ZLIB, NDB_USING_ZIP, NDB_USING_UNZIP);
    ndb_osdep_version_local();
    fflush(stdout);


    ndb_alloc_init(50000L);

    // *************************************************************************
    // init functions
    // *************************************************************************
    ndb_utf8_inittables(ndb_osdep_getCodepage());
    ndb_osdep_init();

        fprintf(stdout, "Current OS:          '%s'\n", ndb_osdep_getOS());
        fprintf(stdout, "Current OS codepage: '%s'\n", ndb_osdep_getCodepage());
        fprintf(stdout, "\n");
        fflush(stdout);

#if defined(WIN32)
        if (! ndb_win32_isWinXP())
        {
                fprintf(stdout, "Warning:\nWindows below WinXP cannot restore the 8.3 short names.\n");
                fprintf(stdout, "\n");
                fflush(stdout);
        }
#endif

    ndb_list_initFileMaskList(&filemasks);

    if ((argc < 3) || (strcmp("-h", argv[1]) == 0))
    {
        fprintf(stdout, "usage:\n");
        fprintf(stdout, "ndbextract <archiv> [options] -a|-c<number> <files>\n");
        fprintf(stdout, "    List of all parameters and options:\n");
        fprintf(stdout, "    -h:               this help text\n");
        fprintf(stdout, "    <archiv>:         name of NDB archive file\n");
        fprintf(stdout, "    <files>:          filename(s), wildcards are * and ?\n");
        fprintf(stdout, "                      (use double quotes for unix)\n");
        fprintf(stdout, "    -a[<from>-<to>]:  extract *a*ll chapters (restrict to <from> - <to>)\n");
        fprintf(stdout, "    -b[<from>-<to>]:  create conversion batch\n");
        fprintf(stdout, "    -em:              *e*xtract only 'm'odified files\n");
        fprintf(stdout, "    -c<number>:       extract content from *c*hapter <number>\n");
        fprintf(stdout, "    -R:               *r*ecursivly extract empty dirs also\n");
        fprintf(stdout, "    -o:               *o*verwrite existing files without any question\n");
        fprintf(stdout, "     -oa:             overwrite *a*ll existing files\n");
        fprintf(stdout, "     -on:             overwrite *n*o existing files\n");
        fprintf(stdout, "     -om:             overwrite *m*odified files only\n");
        fprintf(stdout, "    -x:               e*x*clude following files\n");
        fprintf(stdout, "    -i:               *i*nclude following files\n");
        fprintf(stdout, "    -x@<file>:        e*x*clude file(mask)s listed in <file>\n");
        fprintf(stdout, "    -i@<file>:        *i*nclude file(mask)s listed in <file>\n");
        fprintf(stdout, "    -t:               *t*est archiv integrity (no extraction)\n");
        fprintf(stdout, "    -X:               ignore e*X*tra data (e.g. OS/2 EAs)\n");
        fprintf(stdout, "    -qd:              answer *q*ueries with *d*efault\n");
        fprintf(stdout, "    -p<path>:         extract to <path>, not to current dir\n");
        fprintf(stdout, "    -d<level>:        set *d*ebug level 1..9\n");
        fprintf(stdout, "    -w<millis>:       *w*ait <millis> milliseconds after each file\n");
        fprintf(stdout, "                      even if there are more datafiles ");
#if defined (OS2)
        fprintf(stdout, "(default 25)\n");
#else
        fprintf(stdout, "(default 50)\n");
#endif
        exit(0);
    }
    else
    {
        /* check for optional arguments */
        for (i = 1; i < argc; i++)
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "processing argv[%d] = '%s'\n", i, argv[i]);
            fflush(stdout);
            p = argv[i];
            if ((*p) == '-')
            {
                if (strncmp(p, "-i@", 3) == 0)
                {
                    if (flag_zipname == 0)
                    {
                        fprintf(stderr, "Error: please specify the archive file name first.\n");
                        fflush(stderr);
                        exit(NDB_MSG_NOTOK);
                    }
                    else
                        flag_zipname = 2;
                    strcpy (s_files, &p[3]);
                    copyFileLinesToIncludeList(s_files, &filemasks);
                }
                else if (strncmp(p, "-x@", 3) == 0)
                {
                    if (flag_zipname == 0)
                    {
                        fprintf(stderr, "Error: please specify the archive file name first.\n");
                        fflush(stderr);
                        exit(NDB_MSG_NOTOK);
                    }
                    strcpy (s_files, &p[3]);
                    copyFileLinesToExcludeList(s_files, &filemasks);
                }
                else
                {
                    set_option ((char *) argv[i]);
                }
            }
            else
            {
                if (flag_zipname == 0)
                {
                    flag_zipname = 1;
                    strcpy (s_container, (char *) argv[i]);
                    // add ".ndb" if missing
                    len = strlen(s_container);
                    dot_found = 0;
			        for (j = 0; j < len; j++)
			        {
			            if (s_container[j] == '.')
			                dot_found = 1;
			            else if ((s_container[j] == '/') || (s_container[j] == '\\'))
			                dot_found = 0;
                    }
                    if (dot_found == 0)
                        strcat(s_container,".ndb");
                    // does archive file exist?
                    if (NDB_MSG_OK != ndb_io_check_exist_file(s_container, NULL))
                    {
                                    fprintf(stderr, "could not open archive file '%s'\n", s_container);
                                    fflush(stderr);
                                    exit (NDB_MSG_NOFILEOPEN);
                    }
                } else
                {
                    flag_zipname = 2;
                    strcpy (s_files, (char *) argv[i]);
                    // include or exclude files with this file mask?
                    if (opt_exclude == 0)
                    {
                        // include files
                        ndb_list_addToFileMaskList(&filemasks, s_files, NDB_MODE_INCLUDE);
                        if (ndb_getDebugLevel() >= 5)
                            fprintf(stdout, "file mask list 0x%lX has no %d elements\n", (ULONG) &filemasks,
                                            ndb_list_getFileMaskCount(&filemasks));
                    }
                    else
                    {
                        // exclude files
                        ndb_list_addToFileMaskList(&filemasks, s_files, NDB_MODE_EXCLUDE);
                        if (ndb_getDebugLevel() >= 5)
                            fprintf(stdout, "file mask list 0x%lX has no %d elements\n", (ULONG) &filemasks,
                                            ndb_list_getFileMaskCount(&filemasks));
                    }
                }
            }
        }
    }

    // check parameter
    if (s_container[0] == '\0')
    {
        fprintf(stderr, "Error: cannot find archive file name\n");
        exit(NDB_MSG_NOTOK);
    }
    if (((opt_extractall == 0) && (opt_createbatch == 0)) && (opt_chapterno < 0))
    {
        fprintf(stderr, "Error: need chapter number or -a/-b for extraction of all chapters\n");
        exit(NDB_MSG_NOTOK);
    }
    if (flag_zipname < 2)
    {
        // default: include all files if no filemask is given
        ndb_list_addToFileMaskList(&filemasks, "*", NDB_MODE_INCLUDE);
    }
    if ((opt_extractall == 1) && (opt_chapterno >= 0))
    {
        fprintf(stderr, "Error: combined use of -a and -c<no> senseless\n");
        exit(NDB_MSG_NOTOK);
    }
    if ((opt_extractall == 1) && (opt_extract_from < 0 || opt_extract_to < 0))
    {
        fprintf(stderr, "Error: option -a: pass both <from> and <to> chapter numbers greater zero\n");
        exit(NDB_MSG_NOTOK);
    }
    if ((opt_extractall == 1) && (opt_extract_from > opt_extract_to))
    {
        fprintf(stderr, "Error: option -a: <from> chapter bigger than <to> chapter\n");
        exit(NDB_MSG_NOTOK);
    }
    if ((opt_createbatch == 1) && (opt_extractall == 1))
    {
        fprintf(stderr, "Error: combined use of -b and -a senseless\n");
        exit(NDB_MSG_NOTOK);
    }

    if ((opt_createbatch == 1) && (opt_chapterno >= 0))
    {
        fprintf(stderr, "Error: combined use of -b and -c<no> senseless\n");
        exit(NDB_MSG_NOTOK);
    }
    if ((opt_createbatch == 1) && (opt_extract_from < 0 || opt_extract_to < 0))
    {
        fprintf(stderr, "Error: option -b pass both <from> and <to> chapter numbers greater zero\n");
        exit(NDB_MSG_NOTOK);
    }
    if ((opt_createbatch == 1) && (opt_extract_from > opt_extract_to))
    {
        fprintf(stderr, "Error: option -b: <from> chapter bigger than <to> chapter\n");
        exit(NDB_MSG_NOTOK);
    }

    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "container file is  '%s'\n", s_container);

    /* open archive file */
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "trying to open archiv file\n");
    fArchive = fopen(s_container,"rb");
    if (fArchive == NULL)
    {
        fprintf(stderr, "could not open archive file '%s'\n", s_container);
        fprintf(stderr, "reason: %s\n", strerror (errno));
        exit (NDB_MSG_NOFILEOPEN);
    }
    //  use a buffer for archive file IO
    iobuf = (char *) ndb_calloc(NDB_SIZE_IOBUFFER, 1, "IOBuffer");
    if (iobuf != NULL)
    {
        if (setvbuf(fArchive, iobuf, _IOFBF, NDB_SIZE_IOBUFFER) == 0)
        {
            if (ndb_getDebugLevel() >= 7) fprintf(stdout, "using IO buffer with %d bytes\n\n", NDB_SIZE_IOBUFFER);
        }
        else
        {
            fprintf(stdout, "Error: could not activate IO buffer\n\n");
        }
    }
    // save archive size for later checks
    ndb_io_check_exist_file(s_container, &statFile);
    ndbArchivSize = statFile.st_size;

    // read archive data
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "trying to read archiv header\n");
    retval = ndb_io_readchunkarchive(fArchive, &currArchive);
    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "error %d reading archiv header\n", retval);
        exit (retval);
    }
    currBlockSize   = currArchive.blockSize;
    currCompression = currArchive.compression & 0xff;
    ndb_setBlockSize(currBlockSize);
    ndb_setCompression(currCompression);

    tim_m = localtime( (const time_t *) & currArchive.timestamp);
    if (tim_m != NULL)
        strftime(stime, 40, "%a, %Y-%m-%d, %H:%M:%S", tim_m);
    else
    {
        strcpy(stime, "[DATE ERROR]");
        fprintf(stderr, "warning: creation date of archive header corrupt\n");
        fflush (stderr);
    }

    fprintf(stdout, "Archiv Info:\n\n");
    fprintf(stdout, "Creation:                 %s\n", stime);
    fprintf(stdout, "Block Size:               %d\n", currBlockSize);
    fprintf(stdout, "Compression:              %d\n", currCompression);
    fprintf(stdout, "NDB version:              %s\n", currArchive.ndbver);
    fprintf(stdout, "Archive Identcode:        %0lX\n", currArchive.identCRC);
    fprintf(stdout, "Comment:                  %s\n",
                    (currArchive.comment[0] == '\0') ? "[no comment]" : currArchive.comment);
    fprintf(stdout, "\n");


    ndb_block_init(currArchive.blockSize, currCompression);
    ndb_block_datachunk_init(&dataFilesChunk, NDB_MAGIC_DATAFILE, 0, 0);

    ndb_df_init(&currArchive, fArchive, ndb_osdep_makeIntFileName(s_container));
    ndb_df_setMaxFileHandles(currMaxFHandles);

    // correct chapterno if reverse told
    if (opt_chapterno_reverse == 1)
    {
        opt_chapterno = currArchive.noOfChapters - opt_chapterno - 1;
        if (opt_chapterno < 0)
        {
            fprintf(stderr, "Error: archive file doesn't contain a chapter no %d\n", opt_chapterno);
            exit (NDB_MSG_NOTOK);
        }
    }

    // *************************************************************************
    // read all existing chapters
    // *************************************************************************
    i1 = 0;
    // add + 1 at calloc() to avoid a NULL result when noOfChapters = 0
    pAllChapters = (struct ndb_s_c_chapter *) ndb_calloc (currArchive.noOfChapters + 1, sizeof(struct ndb_s_c_chapter), "all chapters");
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "Info: allocation memory for chapter list at 0x%lX with size 0x%lX\n",
                        (ULONG) pAllChapters, (ULONG) currArchive.noOfChapters * sizeof(struct ndb_s_c_chapter));
    if (pAllChapters == NULL)
    {
        fprintf(stderr, "Error: couldn't allocate memory for chapter list\n");
        exit (NDB_MSG_NOMEMORY);
    }
    currFilePos = currArchive.firstChapter;
    if (currArchive.noOfChapters > 0)
    {
        if ((opt_createbatch == 1))
        {
            fBatch = fopen(s_batch, "w");
            if (fBatch == NULL)
            {
                fprintf(stdout, "could not open batch import file '%s'\n", s_batch);
            }
            else
            {
                fprintf(stdout, "Creating conversion batch '%s'\n", s_batch);
                fprintf(stdout, "Writing initial batch commands\n");
                fflush(stdout);
                ndb_osdep_createBatchImportAll_start(fBatch,s_container, &currArchive);
            }
        }
        for (i1 = 0; i1 < currArchive.noOfChapters; i1++)
        {
            if (ndb_getDebugLevel() >= 5)
                fprintf(stdout, "Info: reading chapter %d at archive file position 0x%lX\n",
                                i1, currFilePos);
            fseek(fArchive, currFilePos, SEEK_SET);
            ndb_io_doQuadAlign(fArchive);
            retval = ndb_io_readchunkchapter(fArchive, &pAllChapters[i1]);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stderr, "Error: reading chapter %d from archive file\n", i1);
//                exit (retval);
				if (i1 > 0)
				{
                    fprintf(stderr, "-> therefore ignoring all chapters after %04u\n\n", i1 - 1);
	            }
	            else
	            {
	            	fclose(fArchive);
                    exit (NDB_MSG_NOTOK);
	            }
                fflush(stderr);
                currArchive.noOfChapters = i1;
                break;
            }
            // debug output level 7
            if (ndb_getDebugLevel() >= 8) ndb_chap_print(&pAllChapters[i1]);

            // check that complete chapter is within the archive file
            if (ndbArchivSize < currFilePos + NDB_CHUNKCHAPTER_SIZEALL + pAllChapters[i1].chapterSize)
            {
                fprintf(stderr, "error: archive size '%s' seems to be corrupt ...\n", s_container);
                fprintf(stderr, "       file size (%lu bytes) should be at least %lu bytes\n",
                        ndbArchivSize, (ULONG) (currFilePos + NDB_CHUNKCHAPTER_SIZEALL + pAllChapters[i1].chapterSize));
                fprintf(stderr, "       to cover body data of chapter %04u\n", i1);
                if (i1 > 0)
                {
                    fprintf(stderr, "-> therefore ignoring all chapters after %04u\n\n", i1 - 1);
                }
                else
                {
	            	fclose(fArchive);
                    exit (NDB_MSG_NOTOK);
                }
                fflush(stderr);
                currArchive.noOfChapters = i1;
                break;
            }

            // read all data file header of current chapter
            if (ndb_getDebugLevel() >= 2)
            {
                ndb_printTimeMillis();
                fprintf(stdout, "reading all %u data file headers of chapter %d\n", pAllChapters[i1].noNewDatFil, pAllChapters[i1].chapterNo);
                fflush(stdout);
            }
            retval = ndb_chap_readAllDataFileHeader(&pAllChapters[i1], fArchive);
            if (retval != NDB_MSG_OK)
            {
                fprintf(stderr, "Error: reading data file header from chapter %d\n", i1);
//                exit (retval);
				if (i1 > 0)
				{
                    fprintf(stderr, "-> therefore ignoring all chapters after %04u\n\n", i1 - 1);
	            }
	            else
	            {
	            	fclose(fArchive);
	            	exit (NDB_MSG_NOTOK);
	            }
                fflush(stderr);
                currArchive.noOfChapters = i1;
                break;
            }

            // position for next chapter
            currFilePos = pAllChapters[i1].ownFilePosition + NDB_CHUNKCHAPTER_SIZEALL
                                                           + pAllChapters[i1].chapterSize;
            // check archive size if not the last chapter
            if (i1 < currArchive.noOfChapters - 1)
            {
                if (ndbArchivSize >= currFilePos + NDB_CHUNKCHAPTER_SIZEALL)
                {
                    fseek(fArchive, currFilePos, SEEK_SET);
                }
                else
                {
                    fprintf(stderr, "error: archive size '%s' seems to be corrupt ...\n", s_container);
                    fprintf(stderr, "       file size (%lu bytes) should be at least %lu bytes\n",
                                    ndbArchivSize, (ULONG) (currFilePos + NDB_CHUNKCHAPTER_SIZEALL - 1));
                    fprintf(stderr, "       to cover header data of chapter %04u\n", i1);
                    fprintf(stderr, "-> therefore ignoring all chapters after %04u\n\n", i1);
                    fflush(stderr);
                    currArchive.noOfChapters = i1 + 1;
                    break;
                }
            }
        }
    }
    else
    {
        // new created (=empty) archive file
        fprintf(stderr, "Error: cannot extract files from an empty archive file\n");
        fclose(fArchive);
        exit (NDB_MSG_NOTOK);
    }

    // *************************************************************************
    // chapter loop:
    // walk through all chapters;
    // if -a was specified, extract all chapter
    // if -b was specified, create batch for all chapter
    // if not, extract only the specified chapter
    // *************************************************************************

    for (i1 = 0; i1 < currArchive.noOfChapters; i1++)
    {
        // if -a or -b not specified, ignore all other chapters than -cxxx
        if (((opt_extractall == 0) && (opt_createbatch == 0)) && (i1 != opt_chapterno))
        {
            if (ndb_getDebugLevel() >= 1)
                fprintf(stdout, "skipping chapter %d\n", i1);
            continue;
        }

        // if -a specified, ignore all chapters outside of opt_extract_from and opt_extract_to
        if (((opt_extractall == 1) || (opt_createbatch == 1)) && ((i1 < opt_extract_from) || (i1 > opt_extract_to)))
        {
            if (ndb_getDebugLevel() >= 1)
                fprintf(stdout, "skipping chapter %d\n", i1);
            continue;
        }

        if ((opt_extractall == 1) || (opt_createbatch == 1))
        {
#if defined(UNIX)
            if (s_path[0] == '\0')
                snprintf(sDefaultPath, NDB_MAXLEN_FILENAME - 1, "chap%04d", i1);
            else
            {
                // s_path has a trailing slash already!
                snprintf(sDefaultPath, NDB_MAXLEN_FILENAME - 1, "%schap%04d", s_path, i1);
            }
#else
            if (s_path[0] == '\0')
                snprintf(sDefaultPath, NDB_MAXLEN_FILENAME - 1, "Chap%04d", i1);
            else
            {
                // s_path has a trailing slash already!
                snprintf(sDefaultPath, NDB_MAXLEN_FILENAME - 1, "%sChap%04d", s_path, i1);
            }
#endif
        }
        else
        {
            if (s_path[0] == '\0')
                strcpy(sDefaultPath, ".");
            else
                strcpy(sDefaultPath, s_path);
        }

        if (ndb_getDebugLevel() >= 2)
            fprintf(stdout, "reading content of chapter %d\n", i1);
        pCurrChapter = &pAllChapters[i1];

        // write chapter import command into the import batch file ...
        if ((opt_createbatch == 1) != 0)
        {
            if (fBatch != NULL)
            {
                fprintf(stdout, "Writing batch commands for chapter %04d\n", i1);
                fflush(stdout);
                ndb_osdep_createBatchImportAll_chapter(fBatch, s_container, sDefaultPath, pCurrChapter);
            }
            // ... and continue with the next chapter
            continue;
        }

        // read all block header of current chapter
        if (ndb_getDebugLevel() >= 2)
        {
            ndb_printTimeMillis();
            fprintf(stdout, "reading all block header of chapter %d\n", i1);
            fflush(stdout);
        }
        retval = ndb_chap_readAllBlockHeader(pCurrChapter, fArchive);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error: reading block header from chapter %d\n", pCurrChapter->chapterNo);
            exit (retval);
        }

        // init hash table for filenames
        ndb_chap_initFileHash(pCurrChapter);

        // read all file header of current chapter
        if (ndb_getDebugLevel() >= 2)
        {
            ndb_printTimeMillis();
            fprintf(stdout, "reading all file header of chapter %d\n", i1);
            fflush(stdout);
        }
        retval = ndb_chap_readAllFileEntries(pCurrChapter, fArchive);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error: reading all file entries of chapter %d resulted in %d\n",
                            pCurrChapter->chapterNo, retval);
            exit(retval);
        }

        // all block header are read already, now add them to the last chapter
        if (ndb_getDebugLevel() >= 2)
        {
            ndb_printTimeMillis();
            fprintf(stdout, "rebuilding block header lists for all files of chapter %d\n", i1);
            fflush(stdout);
        }
        retval = ndb_chap_rebuildBlockHeaderList(pCurrChapter);
        if (retval != NDB_MSG_OK)
        {
            fprintf(stderr, "Error: rebuilding block header list of chapter %d resulted in %d\n",
                            pCurrChapter->chapterNo, retval);
            exit(retval);
        }

        // show from wich chapter we extract
        if (opt_testintegrity != 1)
            fprintf(stdout, "Extracting from chapter %04d:\n", i1);
        else
            fprintf(stdout, "Testing chapter %04d:\n", i1);
        fflush(stdout);

        // if -t (test integrity) set overwrite mode to NDB_OVERWRITE_ALL
        // to avoid questions if archived files exist on the harddrive
        if (opt_testintegrity == 1)
            opt_overwrite = NDB_OVERWRITE_ALL;



        // *************************************************************************
        // first loop:
        // walk through the file entry list and extract all files;
        // every (for a file) needed directory gets created automatically
        // (dir attributes are set in the second loop);
        // if -R, all directories are created also (without attributes)
        // *************************************************************************

        nDirItemCount = 0;
        pFirstDirItem = NULL;
        pLastDirItem  = NULL;
        pFile = pCurrChapter->pFirstFile;
        while (pFile != NULL)
        {
            if ((opt_extractmode == NDB_EXTRACTMODE_MODIFIEDONLY)
                && ((pFile->flags & NDB_FILEFLAG_IDENTICAL) == NDB_FILEFLAG_IDENTICAL))
            {
                if (ndb_getDebugLevel() >= 5)
                {
                    fprintf(stdout, "unchanged to previous chapters -> skip '%s'\n", EMXWIN32CONVERT(pFile->filenameExt));
                    fflush(stdout);
                }
                pFile->action = NDB_ACTION_IGNORE;
            }

            // check file with filemask (pFile->action is set by ndb_list_checkWithFileMask()
            ndb_list_checkWithFileMask(pFile, &filemasks);

            // check for malicious filenames
            // - absolute paths  (strip leading '/')
            // - drive letters   (strip leading 'x:')
            // - drive UNC paths (strip leading '//server/share/')
            // - sequences of '../'
            strcpy (sDummy, pFile->filenameUTF8);
            ndb_util_cleanIntPath(sDummy);
            if ((strcmp(sDummy, pFile->filenameUTF8) != 0) || (strstr(pFile->filenameUTF8, "../") != NULL))
            {
                if (opt_querymode == NDB_QUERYMODE_PROMPT)
                {
                    fprintf(stderr, "warning: filename '%s' may contain a malicious path.\n", EMXWIN32CONVERT(pFile->filenameExt));
                    strcpy (text, "Extract this file nevertheless? (n)o [D], (y)es?");
                    c = ndb_util_prompt_user(text, "ny");
                }
                else if (opt_querymode == NDB_QUERYMODE_DEFAULT)
                {
                    c = 'n';
                }
                else if (opt_querymode == NDB_QUERYMODE_YES)
                {
                    c = 'y';
                }
                else if (opt_querymode == NDB_QUERYMODE_NO)
                {
                    c = 'n';
                }
                else
                {
                    c = 'n';
                }
                // don't extract this file
                if (c == 'n')
                {
                    pFile->action = NDB_ACTION_IGNORE;
                }
            }

            // create whole filename (with default path)
            if (strcmp(sDefaultPath, ".") == 0)
                strcpy(sFileWithDefault, pFile->filenameExt);
            else
                snprintf(sFileWithDefault, NDB_MAXLEN_FILENAME - 1, "%s/%s", sDefaultPath, pFile->filenameExt);

            // remember all directory items for the second loop
            if ((pFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
            {
                pCurrDirItem = (PNDBDIRITEM) ndb_calloc(sizeof(struct ndb_diritem), 1, "ndb_diritem");
                if (pCurrDirItem == NULL)
                {
                    fprintf(stderr, "Error: couldn't allocate memory for struct ndb_diritem\n");
                    exit (NDB_MSG_NOMEMORY);
                }
                // set pointer to current directory
                pCurrDirItem->pDir = pFile;
                pCurrDirItem->pNext = NULL;
                // add to list
                if (pFirstDirItem == NULL)
                {
                    // first item
                    pFirstDirItem = pCurrDirItem;
                    pLastDirItem  = pCurrDirItem;
                }
                else
                {
                    // second or later item
                    pLastDirItem->pNext = pCurrDirItem;
                    pLastDirItem  = pCurrDirItem;
                }
                nDirItemCount++;
            }

            // debug: print current overwrite mode
            if (ndb_getDebugLevel() >= 6)
            {
                fprintf(stdout, "program mode is ");
                if (opt_overwrite == NDB_OVERWRITE_ASK)
                    fprintf(stdout, "'ask user'\n");
                if (opt_overwrite == NDB_OVERWRITE_ALL)
                    fprintf(stdout, "'overwrite all'\n");
                if (opt_overwrite == NDB_OVERWRITE_NONE)
                    fprintf(stdout, "'overwrite none'\n");
                if (opt_overwrite == NDB_OVERWRITE_YES)
                    fprintf(stdout, "'overwrite this'\n");
                if (opt_overwrite == NDB_OVERWRITE_NO)
                    fprintf(stdout, "'don't overwrite this'\n");
                if (opt_overwrite == NDB_OVERWRITE_MODIFIED)
                    fprintf(stdout, "'overwrite only modified'\n");
                fflush(stdout);
            }

                        // if mode is not 'overwrite all existing files', then check, if current file/dir,
                        // which should be extracted, does already exist
                        // pw; 28-Feb-04; and if pFile->action says, it should be extracted
            iSkipFile = 0;
// TODO: Unix stat64
            iExistsFile = (NDB_MSG_OK == ndb_io_check_exist_file(sFileWithDefault, &statFile));
            if (iExistsFile && (ndb_getDebugLevel() >= 6))
            {
                fprintf(stdout, "file '%s' exists on hard drive already\n", EMXWIN32CONVERT(pFile->filenameExt));
                fflush(stdout);
            }

                        if (iExistsFile && (pFile->action == NDB_ACTION_EXTRACT) && (opt_overwrite != NDB_OVERWRITE_ALL))
                        {
                // check all modes
                if (opt_overwrite == NDB_OVERWRITE_ALL)
                {
                    iSkipFile = 0;
                }
                else if (opt_overwrite == NDB_OVERWRITE_NONE)
                {
                    if (iExistsFile)
                    {
                        iSkipFile = 1;
                    }
                    else
                    {
                        iSkipFile = 0;
                    }
                }
                else if (opt_overwrite == NDB_OVERWRITE_ASK)
                {
                    iSkipFile = 0;
                    if (iExistsFile)
                    {
                        tim_m = localtime( (const time_t *) &statFile.st_mtime);
                        strftime(smtimeHD, 40, "%Y-%m-%d, %H:%M:%S", tim_m);
                        tim_m = localtime( (const time_t *) &pFile->mtime);
                        strftime(smtimeArc, 40, "%Y-%m-%d, %H:%M:%S", tim_m);
                        snprintf (text, 300 - 1, "'%s' (%.0f bytes, %s) does already exist.\n",
                                       EMXWIN32CONVERT(pFile->filenameExt), (double) statFile.st_size, smtimeHD);
                        snprintf (text, 300 - 1, "%sArchive version (%.0f bytes, %s).\nOverwrite it: %s? ",
                                       text, (double) pFile->lenOri8, smtimeArc, "(y)es, (n)o, (m)odified only, (A)ll, (N)one");
                        c = ndb_util_prompt_user(text, "ynmAN");
                        // don't overwrite this file
                        if (c == 'n')
                        {
                            iSkipFile = 1;
                        }
                        // don't overwrite any file
                        if (c == 'N')
                        {
                            opt_overwrite = NDB_OVERWRITE_NONE;
                            iSkipFile = 1;
                        }
                        // overwrite all file
                        if (c == 'A')
                        {
                            opt_overwrite = NDB_OVERWRITE_ALL;
                            iSkipFile = 0;
                        }
                        // overwrite modified files only
                        if (c == 'm')
                        {
                            opt_overwrite = NDB_OVERWRITE_MODIFIED;
                            // check follows later
                        }
                        // overwrite this file
                        if (c == 'y')
                        {
                            iSkipFile = 0;
                        }
                    }
                }

                // this check is seperate; to call it also if mode is NDB_OVERWRITE_MODIFIED
                // is set in NDB_OVERWRITE_ASK
                if (opt_overwrite == NDB_OVERWRITE_MODIFIED)
                {
                    ulAttributes = ndb_osdep_getFileMode(sFileWithDefault);
                    if (NDB_MSG_OK == ndb_file_checkattributeswithstat(pFile, &statFile, ulAttributes))
                    {
                        iSkipFile = 1;
                        if (ndb_getDebugLevel() >= 1)
                        {
                            fprintf(stdout, "skip unchanged file '%s'\n", EMXWIN32CONVERT(pFile->filenameExt));
                            fflush(stdout);
                        }
                    }
                    else
                    {
                        iSkipFile = 0;
                        if (ndb_getDebugLevel() >= 5)
                        {
                            fprintf(stdout, "file is modified against backup ('%s')\n", EMXWIN32CONVERT(pFile->filenameExt));
                            fflush(stdout);
                        }
                    }
                }

                if (iSkipFile == 1)
                {
                    pFile->action = NDB_ACTION_IGNORE;
                }
            }


                        if (ndb_getDebugLevel() >= 4)
                        {
                            fprintf(stdout, "pFile: '%s', action: %c, opt_extractdirs: %d\n", EMXWIN32CONVERT(sFileWithDefault),
                                            pFile->action, opt_extractdirs);
                            fflush(stdout);
                        }

            // only files are extracted directly (if ndb_list_checkWithFileMask() succeeded)
            if ((pFile->action == NDB_ACTION_EXTRACT) && ((pFile->attributes & NDB_FILEFLAG_DIR) != NDB_FILEFLAG_DIR))
            {
                // extract file
                if (ndb_getDebugLevel() >= 2)
                {
                    ndb_printTimeMillis();
                    fprintf(stdout, "trying to extract file/dir '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                    fflush(stdout);
                }
                // unpack file (if not -t specified)
                retval = NDB_MSG_OK;
#if defined(USE_FHANDLE_INT)
                fileOut = -1;
#else
                fileOut = NULL;
#endif
                // if only testing mode
                // then no path creation and no file creation
                if (opt_testintegrity != 1)
                {
                    // make sure that we can (over)write our file
                    if (iExistsFile && (! (statFile.st_mode & S_IWRITE)))
                    {
                        if (ndb_getDebugLevel() >= 4)
                                {
                                    fprintf(stdout, "pFile: '%s' is readonly, try to change file attributes\n", EMXWIN32CONVERT(sFileWithDefault));
                                    fflush(stdout);
                                }
                        // set read and write access ...
                        chmod(sFileWithDefault, S_IREAD | S_IWRITE);
                        // ... to delete the file before extracting from archive
                        if (unlink(sFileWithDefault) != 0)
                        {
                            fprintf(stderr, "warning: cannot overwrite '%s' for extraction\n",
                                    EMXWIN32CONVERT(sFileWithDefault));
                            fflush(stderr);
                        }
                    }
                    // first create path if neccessary
                    retval = ndb_osdep_createPath(pFile, sDefaultPath);
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stdout, "Error: cannot create dir '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                        fprintf(stdout, "       skipping dir\n");
                        fflush(stdout);
                        pFile->action = NDB_ACTION_FAILED;
                        ulFilesFailed++;
                    }
                    else
                    {
                        retval = ndb_osdep_createFile(pFile, sDefaultPath, &fileOut);
                    }
                }
                if (retval != NDB_MSG_OK)
                {
                    fprintf(stdout, "Error: cannot create file '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                    fprintf(stdout, "       skipping file\n");
                    fflush(stdout);
                    pFile->action = NDB_ACTION_FAILED;
                    ulFilesFailed++;
                }
                else
                {
                    // unpack and write the file (test mode: fileOut = NULL)
                    retval = ndb_expandStream(pCurrChapter, pFile, fileOut);
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stdout, "Error unpacking file '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                        fprintf(stdout, "      file is corrupted\n");
                        fflush(stdout);
                        pFile->action = NDB_ACTION_FAILED;
                        ndb_fe_addRemark(pFile, ndb_util_msgcode2print(retval));
                        ulFilesFailed++;
                    }
                    else
                    {
                        if (opt_testintegrity != 1)
                            fprintf(stdout, "extracted file '%s' (file ok", EMXWIN32CONVERT(sFileWithDefault));
                        else
                            fprintf(stdout, "tested file '%s' (file ok", EMXWIN32CONVERT(sFileWithDefault));
                    }
                    // close file
                    if (opt_testintegrity != 1)
                    {
                        strcpy(text, "");
#if defined(USE_FHANDLE_INT)
                        if (fileOut != -1)
                        {
                            close(fileOut);
                        }
#else
                        if (fileOut != NULL)
                        {
                            fclose(fileOut);
                        }
#endif
                        retval = ndb_osdep_setMetaData(pFile, sDefaultPath, fArchive, opt_testintegrity, opt_ignoreextra, text);
                        if (opt_ignoreextra == 0)
                        {
                            if (retval != NDB_MSG_OK)
                            {
                                fprintf(stdout, "\nError: cannot unpack extra data of file '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                                fflush(stdout);
                                pFile->action = NDB_ACTION_FAILEDEXTRA;
                                ulFilesExtraFailed++;
                            }
                            else
                            {
                                if (text[0] != '\0')
                                    fprintf(stdout, text);
                            }
                        }
                    }
                    // successfull extraction? (NDB_ACTION_EXTRACT not overwritten
                    // with NDB_ACTION_FAILED or NDB_ACTION_FAILEDEXTRA)?
                    if (pFile->action == NDB_ACTION_EXTRACT)
                    {
                        ulFilesExtracted++;
                    }
                    fprintf(stdout, ")\n");
                    fflush(stdout);
                    // mark all directories of file's path also to extract
                    ndb_file_markpath2extract(pCurrChapter, pFile);
                }
            }
            // with option -R all directories are extracted
            else if ((opt_extractdirs == 1) && ((pFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR))
            {
                pFile->action = NDB_ACTION_EXTRACT;
                retval = ndb_osdep_createPath(pFile, sDefaultPath);
                if (retval != NDB_MSG_OK)
                {
                    fprintf(stdout, "Error: cannot create dir '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                    fprintf(stdout, "       skipping dir\n");
                    fflush(stdout);
                    pFile->action = NDB_ACTION_FAILED;
                    ulFilesFailed++;
                }
                else
                {
                    // successfull extraction
                    ulFilesExtracted++;
                } // if (retval != NDB_MSG_OK)
            } // create files/ create dirs if -r

            // wait if specified
            if (opt_waitmillis > 0)
            {
                ndb_osdep_sleep(opt_waitmillis);
            }

            // switch to next file
            pFile = pFile->nextFile;
        }

        // *************************************************************************
        // second loop:
        // first sort the directory item list;
        // then walk through the directory entry list;
        // for each existing directory (previously created) set file attributes
        // and extra data
        // *************************************************************************

        // first sort the directory entries
        if (nDirItemCount > 0)
        {
            if (ndb_getDebugLevel() >= 5)
            {
                fprintf(stdout, "found %lu directories\n", nDirItemCount);
                fflush(stdout);
            }
            pSortedDirItem = (PNDBDIRITEM) ndb_calloc(nDirItemCount, sizeof(struct ndb_diritem), "pSortedDirItem");
            if (pSortedDirItem == (PNDBDIRITEM) NULL)
            {
                fprintf(stderr, "Error: couldn't allocate memory for sorted ndb_diritem list\n");
                exit (NDB_MSG_NOMEMORY);
            }

            if (ndb_getDebugLevel() >= 6)
            {
                fprintf(stdout, "copying %lu directory entries for sorting\n", nDirItemCount);
                fflush(stdout);
            }

            if (nDirItemCount == 1)
                pSortedDirItem[0] = *pFirstDirItem;
            else
            {
                pCurrDirItem = pFirstDirItem;
                for (i = 0;  i < nDirItemCount;  ++i)
                {
                    pSortedDirItem[i] = *pCurrDirItem;
                    pLastDirItem = pCurrDirItem;
                    pCurrDirItem = pCurrDirItem->pNext;
                    ndb_free(pLastDirItem);
                }
                qsort((char *) pSortedDirItem, nDirItemCount, sizeof(struct ndb_diritem), dircomp);
            }

                        // then walk through all directories of archive chapter
            for (i = 0;  i < nDirItemCount;  ++i)
            {
                pFile = pSortedDirItem[i].pDir;

                // skip files now
                if ((pFile->attributes & NDB_FILEFLAG_DIR) != NDB_FILEFLAG_DIR)
                {
                    if (ndb_getDebugLevel() >= 8)
                    {
                        fprintf(stdout, "skipping file '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                        fflush(stdout);
                    }
                    continue;
                }

                // create whole filename (with default path)
                if (strcmp(sDefaultPath, ".") == 0)
                    strcpy(sFileWithDefault, pFile->filenameExt);
                else
                    snprintf(sFileWithDefault, NDB_MAXLEN_FILENAME - 1, "%s/%s", sDefaultPath, pFile->filenameExt);

                if (ndb_getDebugLevel() >= 5)
                {
                    fprintf(stdout, "checking dir (%c) '%s'\n", pFile->action, EMXWIN32CONVERT(sFileWithDefault));
                    fflush(stdout);
                }


                // *************************************************************************
                // now walk through all directories:
                // don't create new directories, modify only previously created ones
                // which are marked to be extracted

                if ((pFile->action == NDB_ACTION_EXTRACT) && (ndb_io_check_exist_file(sFileWithDefault, NULL) == NDB_MSG_OK))
                {
                    if (ndb_getDebugLevel() >= 7)
                    {
                        lDummy = ndb_osdep_getFileMode(sFileWithDefault);
                        fprintf(stdout, "(%d) file attributes for dir '%s': %lu\n", i, sFileWithDefault, lDummy);
                        fflush(stdout);
                    }
                    if (opt_testintegrity != 1)
                    {
                        if (ndb_getDebugLevel() >= 5)
                        {
                            ndb_printTimeMillis();
                            fprintf(stdout, "setting meta data for dir '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                            fflush(stdout);
                        }
                        strcpy(text, "");
                        retval = ndb_osdep_setMetaData(pFile, sDefaultPath, fArchive, 0, opt_ignoreextra, text);
                        if (retval != NDB_MSG_OK)
                        {
                            fprintf(stdout, "Error: cannot unpack extra data of directory '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                            fflush(stdout);
                            pFile->action = NDB_ACTION_FAILEDEXTRA;
                            ulFilesExtraFailed++;
                        }
                        else
                        {
                            // successfull extraction
                            ulFilesExtracted++;
                        if (opt_testintegrity != 1)
                            fprintf(stdout, "extracted dir '%s' (all CRCs ok", EMXWIN32CONVERT(sFileWithDefault));
                        else
                            fprintf(stdout, "tested dir '%s' (all CRCs ok", EMXWIN32CONVERT(sFileWithDefault));
                        }
                        if (pFile->pExtraHeader != NULL)
                            fprintf(stdout, text);
                        fprintf(stdout, ")\n");
                        fflush(stdout);
                    }
                } // if (ndb_io_check_exist_file(sFileWithDefault, NULL))
                else
                {
                    if (ndb_getDebugLevel() >= 7)
                    {
                        fprintf(stdout, "not extracted dir '%s'\n", EMXWIN32CONVERT(sFileWithDefault));
                        fflush(stdout);
                    }
                } // if (ndb_io_check_exist_file(sFileWithDefault, NULL))
            } // for (i = 0;  i < nDirItemCount;  ++i)
            ndb_free(pSortedDirItem);
        }
    } // for (i1 = 0; i1 < currArchive.noOfChapters; i1++)


    fclose(fArchive);

    // write chapter import command into the import batch file
    if ((opt_createbatch == 1))
    {
        if (fBatch != NULL)
        {
            ndb_osdep_createBatchImportAll_end(fBatch,  s_container);
            fclose(fBatch);
            fprintf(stdout, "Writing finishing batch commands\n");
            fflush(stdout);
#if defined(UNIX)
            if (chmod(s_batch, 0xffff & 0755))
            {
                fprintf(stderr, "error: couldn't set file access 0755 for '%s'\n%s\n",
                                s_batch, strerror(errno));
            }
#endif
        }
    }

    // show all failed files
    if (ulFilesFailed > 0)
    {
        pFile = pCurrChapter->pFirstFile;
        if (pFile != NULL)
        {
            fprintf(stdout, "\nWarning: following %lu file(s) could not be extracted:\n", ulFilesFailed);
            while ( pFile != NULL )
            {
                if (pFile->action == NDB_ACTION_FAILED)
                {
                    // create time string
                    tim_m = localtime( (const time_t *) &pFile->mtime);
                    strftime(stime_m, 40, "%Y-%m-%d, %H:%M:%S", tim_m);
                    // copy error message if existing
                    if (pFile->remark != NULL)
                        snprintf (sDummy, 200 - 1, ": %s", pFile->remark);
                    else
                                                sDummy[0] = '\0';
                    fprintf(stdout, "%s (%.0f bytes, %s)%s\n", EMXWIN32CONVERT(pFile->filenameExt),
                                    (double) pFile->lenOri8, stime_m, sDummy);
                }
                // go to next file
                pFile = pFile->nextFile;
            }
        }
    }

    // show all files with failed extra data
    if (ulFilesExtraFailed > 0)
    {
        pFile = pCurrChapter->pFirstFile;
        if (pFile != NULL)
        {
            fprintf(stdout, "\nWarning: following %lu file(s) have been extracted, but the extra data failed:\n", ulFilesExtraFailed);
            while ( pFile != NULL )
            {
                if (pFile->action == NDB_ACTION_FAILEDEXTRA)
                {
                    // create time string
                    tim_m = localtime( (const time_t *) &pFile->mtime);
                    strftime(stime_m, 40, "%Y-%m-%d, %H:%M:%S", tim_m);
                    // copy error message if existing
                    if (pFile->remark != NULL)
                        snprintf (sDummy, 200 - 1, ": %s", pFile->remark);
                    else
                                                sDummy[0] = '\0';
                    fprintf(stdout, "%s (%.0f bytes, %s)%s\n", EMXWIN32CONVERT(pFile->filenameExt),
                                    (double) pFile->lenOri8, stime_m, sDummy);
                }
                // go to next file
                pFile = pFile->nextFile;
            }
        }
    }

    ndb_alloc_done();
    ndb_osdep_done();

    time_finished = 0;
    time(&time_finished);
    db_duration = difftime(time_finished, time_startup);

    fprintf(stdout, "\nNDBExtract: finished at %s\n", ctime(&time_startup));
    if ((opt_createbatch == 0))
    {
        fprintf(stdout, "Needed %.0f seconds, ", db_duration);
        fprintf(stdout, "%s %lu files/directories, failed %lu, extra data failed %lu\n",
                opt_testintegrity == 0 ? "extracted" : "tested", ulFilesExtracted, ulFilesFailed, ulFilesExtraFailed);
    }

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
        ndb_util_print_performancetest();
    }
#endif

    if ((ulFilesFailed == 0) && (ulFilesExtraFailed == 0))
        return(NDB_MSG_OK);
    else
        return(NDB_MSG_NOTOK);
}


