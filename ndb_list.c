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
#include <time.h>

#include "ndb.h"
#include "ndb_prototypes.h"

#ifdef __MINGW32__
   int _CRT_glob = 0;   /* suppress command line globbing by C RTL */
#endif

#ifdef UNIX
#include <errno.h>
extern int errno;
#endif


#define VERBOSITY_SMALL          0
#define VERBOSITY_MEDIUM         1
#define VERBOSITY_LONG           2

// used by other modules also
int currBlockSize   = 0;
int currCompression = 0;

/*
    *************************************************************************************
        main()

        reads and shows the given chapters and/or files (given by command line parameters)
    *************************************************************************************
*/

int main(int argc, char **argv)
{
    char s_container[NDB_MAXLEN_FILENAME];
    char sDummy[NDB_MAXLEN_FILENAME];
    int flag_zipname = 0;
    FILE *fArchive;
    int opt_debug              = 0;
    int opt_verbose            = 1;
    int opt_chapterno          = -1;
    int opt_rawmode            = 0;
    int opt_chapterno_reverse  = 0;
    int opt_allocation         = 0;
    int opt_changedfilesonly   = 0;
    int opt_showblocks         = 0;
    int opt_showhashfilename   = 0;
    char *p;
    ULONG l1;
    int i;
    time_t time_startup;
    ULONG len;
    char *iobuf = NULL;
    struct ndb_s_c_archive currArchive;
    int retval;
    struct ndb_s_c_chapter currChapter;
    struct ndb_s_c_chapter *pAllChapters;
    struct ndb_s_fileentry *pFile;
    int i1;
    ULONG currFilePos;
    int dot_found;
    char c;
    // create file summery
    ULONG ulCountEntries = 0;
    ULONG ulCountDirs    = 0;
    double dbSizeOri     = 0;
    double dbSizeZip     = 0;
    char sCountFiles[60];
    // create chapter summery
    ULONG ulCountChap = 0;
    double dbSizeOriChap = 0;
    double dbSizeZipChap = 0;
    char sCountChap[20];
    // create time string
    char stime[40];
    struct tm *tim;
        char sratio[10];
        // allow a selection of filenames to list
    char sMask[NDB_MAXLEN_FILENAME] = "\0";
    int bShowMatchesOverAllChapter = 0;
        // show block header
        PBLOCKHEADER pBlock = NULL;
        char sExtraType[40] = "\0";
    double dbAllocComplete    = 0;
    double dbAllocBlockData   = 0;
    ULONG  lAllocBlockHeader  = 0;
    ULONG  lAllocFiles        = 0;
    char sSizeOri[20];
    char sSizeZip[20];

    time_startup = 0;
    time(&time_startup);

    // print start message after processing the command line parameter
    // to allow option -r (raw mode)

    ndb_alloc_init(50000L);

    // ========================================================================
    // init functions
    // ========================================================================
    ndb_utf8_inittables(ndb_osdep_getCodepage());
    ndb_osdep_init();

    if (argc < 2)
    {
        fprintf(stdout, "NDBList %s: startup at %s", NDB_VERSION, ctime(&time_startup));
        fprintf(stdout, "%s\n", NDB_COPYRIGHT);
        fprintf(stdout, "Using %s and a few source parts of %s and %s\n", NDB_USING_ZLIB, NDB_USING_ZIP, NDB_USING_UNZIP);
        ndb_osdep_version_local();
        fprintf(stdout, "usage: ndblist <container> [-c<number>] [-l<x>] [-d<level>] [file mask]\n");
        fprintf(stdout, "       -c<number>:  list content from *c*hapter <number> <name>\n");
        fprintf(stdout, "       -l<..>:      *l*ist level:\n");
        fprintf(stdout, "         s:         *s*hort output (only chapter without -c<nr>)\n");
        fprintf(stdout, "         m:         *m*edium output (only chapter without -c<nr>) [default]\n");
        fprintf(stdout, "         l:         *l*ong output (only chapter without -c<nr>)\n");
        fprintf(stdout, "         c:         show *c*hanged files only\n");
        fprintf(stdout, "         a:         show chapter *a*llocations\n");
        fprintf(stdout, "         h:         show file *h*ash and name\n");
        fprintf(stdout, "         r:         *r*aw mode (usable for other programs)\n");
        fprintf(stdout, "         b:         show *b*lock list for each file (debugging)\n");
        fprintf(stdout, "       -d<level>:   set *d*ebug level 1..9\n");
        fflush(stdout);
        exit(0);
    }
    else
    {
        /* check for optional arguments */
        for (i = 1; i < argc; i++)
        {
            p = (char *) argv[i];
            if ((*p) != '-')
            {
                if (flag_zipname == 0)
                {
                    flag_zipname = 1;
                    strcpy (s_container, (char *) argv[i]);
                    // add ".ndb" if missing
                    len = strlen(s_container);
                    dot_found = 0;
                    for (l1 = 0; l1 < len; l1++)
                    {
                        if (s_container[l1] == '.')
                            dot_found = 1;
                        else if ((s_container[l1] == '/') || (s_container[l1] == '\\'))
                            dot_found = 0;
                    }
                    if (dot_found == 0)
                        strcat(s_container,".ndb");
                    if (ndb_getDebugLevel() >= 2)
                        fprintf(stdout, "container file is  '%s'\n", s_container);
                    fflush(stdout);
                }
                else
                {
                    flag_zipname = 2;
                    strcpy(sMask, argv[i]);
                        if (ndb_getDebugLevel() >= 2)
                            fprintf(stdout, "file mask is  '%s'\n", sMask);
                        fflush(stdout);
                }
            }
            else
            {
                p++;

                c = *p;
                if (c == 'd')
                {
                    c = *(++p);
                    if ((c >= '0') && (c <= '9'))
                    {
                        opt_debug = c - '0';
                        ndb_setDebugLevel(opt_debug);
                    }
                }
                else if (c == 'c')
                {
                    if (*(++p) == '-')
                    {
                            opt_chapterno_reverse = 1;
                            p++;
                    }
                    strcpy(sDummy, p);
                    opt_chapterno = atoi(sDummy);
                }
                else if (c == 'l')
                {
                    p++;
                    c = *p;
                    switch (c)
                    {
                        case 's':
                             opt_verbose = VERBOSITY_SMALL;
                             break;
                        case 'm':
                             opt_verbose = VERBOSITY_MEDIUM;
                             break;
                        case 'l':
                             opt_verbose = VERBOSITY_LONG;
                             break;
                        case 'a':
                             opt_allocation = 1;
                             break;
                        case 'h':
                             opt_showhashfilename = 1;
                             break;
                        case 'r':
                             opt_rawmode = 1;
                             break;
                        case 'c':
                             opt_changedfilesonly = 1;
                             break;
                        case 'b':
                             opt_showblocks = 1;
                             break;
                        default:
                             fprintf(stdout, "unknown list level -%c\n", c);
                    }
                }
                else
                {
                    fprintf(stdout, "unknown option -%s\n", p);
                }
            }
        }
    }

    // check options ----------------------------------------------------------------

    // no chapter selected and filespec given
    // -> show matching files of all chapters
    if ((opt_chapterno == -1) && (sMask[0] != '\0'))
    {
        bShowMatchesOverAllChapter = 1;
    }
    if ((sMask[0] != '\0') && (opt_allocation == 1))
    {
        fprintf(stdout, "Combination of -la and <file mask> not supported\n");
        fflush(stdout);
        exit(NDB_MSG_NOTOK);
    }
    if ((sMask[0] != '\0') && (opt_rawmode == 1))
    {
        fprintf(stdout, "Combination of -lr and <file mask> not supported -> ignoring mask '%s'\n", sMask);
        fflush(stdout);
        sMask[0] = '\0';
    }
    if ((sMask[0] == '\0') && (opt_chapterno == -1) && (opt_changedfilesonly == 1))
    {
        fprintf(stdout, "Using -lc you need a file mask or a chapter number\n");
        fflush(stdout);
        exit(NDB_MSG_NOTOK);
    }


    if ((opt_rawmode == 0) && (opt_verbose > VERBOSITY_SMALL))
    {
        fprintf(stdout, "NDBList %s: startup at %s", NDB_VERSION, ctime(&time_startup));
        fprintf(stdout, "%s\n", NDB_COPYRIGHT);
        fprintf(stdout, "Using %s and source parts of %s and %s\n", NDB_USING_ZLIB, NDB_USING_ZIP, NDB_USING_UNZIP);
        ndb_osdep_version_local();
        fflush(stdout);
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

    // read archive data
    if (ndb_getDebugLevel() >= 5) fprintf(stdout, "trying to read archiv header\n");
    retval = ndb_io_readchunkarchive(fArchive, &currArchive);
    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "error %d reading archiv header\n", retval);
        exit (retval);
    }
    currBlockSize   = currArchive.blockSize;
    currCompression = currArchive.compression;
    tim = localtime( (const time_t *) & currArchive.timestamp);
    if (tim != NULL)
        strftime(stime, 40, "%a, %Y-%m-%d, %H:%M:%S", tim);
    else
    {
        strcpy(stime, "[DATE ERROR]       ");
        fprintf(stderr, "warning: creation date of archive header corrupt\n");
        fflush (stderr);
    }

    if (opt_rawmode == 1)
    {
        ndb_arc_printArchiveInfoRaw(&currArchive);
    }
    else if (opt_verbose == VERBOSITY_LONG)
    {
        fprintf(stdout, "Archiv Info:\n\n");
        fprintf(stdout, "Chapters:                 %d\n", currArchive.noOfChapters);
        fprintf(stdout, "Creation:                 %s\n", stime);
        fprintf(stdout, "Block Size:               %d\n", currBlockSize);
        fprintf(stdout, "Compression:              %d\n", currCompression);
        if (currArchive.dataFilesSize > 0)
        {
            fprintf(stdout, "Archive Size:             %.0f (without Data Files)\n", (double) (currArchive.archiveSize + NDB_CHUNKARCHIVE_SIZEALL));
            fprintf(stdout, "Max. Data File Size       %lu\n", currArchive.dataFilesSize);
        }
        else
        {
            fprintf(stdout, "Archive Size:             %.0f\n", (double) (currArchive.archiveSize + NDB_CHUNKARCHIVE_SIZEALL));
            fprintf(stdout, "Max. Data File Size       - no Data Files -\n");
        }
        fprintf(stdout, "Archive Identcode:        %0lX\n", currArchive.identCRC);
        fprintf(stdout, "NDB version:              %s\n", currArchive.ndbver);
        fprintf(stdout, "Comment:                  %s\n",
                        (currArchive.comment[0] == '\0') ? "[no comment]" : currArchive.comment);
        fprintf(stdout, "\n");
    }


    ndb_block_init(currArchive.blockSize, currArchive.compression);

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


    // read all existing chapters
    i1 = 0;
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
        if (opt_rawmode == 0)
        {
            if (opt_allocation == 1)
            {
                fprintf(stdout, "Chapter List:\n");
                fprintf(stdout, "  No    Complete  Data Blocks  Block Header  File Entry  Name                \n");
                fprintf(stdout, "----  ----------  -----------  ------------  ----------  --------------------\n");
            }
            else
            {
                if ((opt_verbose == VERBOSITY_SMALL) && (opt_changedfilesonly == 1))
                {
                fprintf(stdout, "Chap      Date  Time MD5/CRC32       Size     Packed Ratio  Filename          \n");
                fprintf(stdout, "----  -------- ----- ---------  ---------  --------- -----  ------------------\n");
                }
                else if (bShowMatchesOverAllChapter == 0)
                {
                    if (opt_verbose < VERBOSITY_LONG)
                    {
                        if (opt_chapterno == -1)
                        {
                            fprintf(stdout, "Chapter List:\n");
                            fprintf(stdout, "  No              Date  Files       Size     Packed Ratio  Name                \n");
                            fprintf(stdout, "----  ----------------  -----  ---------  --------- -----  --------------------\n");
                        }
                        else if ((opt_changedfilesonly == 0) && (opt_verbose > VERBOSITY_MEDIUM))
                        {
                            fprintf(stdout, "Summary of chapter %d:\n", opt_chapterno);
                            fprintf(stdout, "  No              Date  Files       Size     Packed Ratio  Name                \n");
                            fprintf(stdout, "----  ----------------  -----  ---------  --------- -----  --------------------\n");
                        }
                    }
                    else
                    {
                        if (opt_chapterno == -1)
                        {
                            fprintf(stdout, "Chapter List:\n");
                            fprintf(stdout, "  No  Date                 Files all/changed        Size     Packed    Ratio\n");
                            fprintf(stdout, "      NDB Version          Chapter Name                             Codepage\n");
                            fprintf(stdout, "----------------------------------------------------------------------------\n");
                        }
                        else
                        {
                            fprintf(stdout, "Summary of chapter %d:\n", opt_chapterno);
                            fprintf(stdout, "  No  Date                 Files all/changed        Size     Packed    Ratio\n");
                            fprintf(stdout, "      NDB Version          Chapter Name                             Codepage\n");
                            fprintf(stdout, "----------------------------------------------------------------------------\n");
                        }
                    }
                }
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
                fprintf(stderr, "Error: reading chapter %d from archive file: %s\n", i1,
                                ndb_util_msgcode2print(retval));
                fflush(stderr);
                exit (retval);
            }

            if (ndb_getDebugLevel() >= 7)
                ndb_chap_print(&pAllChapters[i1]);
            if ((opt_allocation == 1) && (opt_rawmode == 0))
            {
                // do we have data files?
                if (currArchive.dataFilesSize == 0)
                {
                    // no data files, therefore everything is "within" the chapter
                    fprintf(stdout, "%4d  %10.0f   %10.0f    %10lu  %10lu  %s\n",
                                    pAllChapters[i1].chapterNo,
                                    (double) pAllChapters[i1].chapterSize,
                                    (double) pAllChapters[i1].blockDatLenReal,
                                    pAllChapters[i1].blockHeaLen,
                                    pAllChapters[i1].filesListLen,
                                    pAllChapters[i1].chapterName);
                    dbAllocComplete   += (double) pAllChapters[i1].chapterSize;
                    dbAllocBlockData  += (double) pAllChapters[i1].blockDatLenReal + (double) pAllChapters[i1].blockDatLen;
                }
                else
                {
                    // yes, we have data files, therefore add to chaptersize the blockDatLenReal which
                    // is located in the data files
                    fprintf(stdout, "%4d  %10.0f   %10.0f    %10lu  %10lu  %s\n",
                                    pAllChapters[i1].chapterNo,
                                    (double) pAllChapters[i1].chapterSize + (double) pAllChapters[i1].blockDatLenReal,
                                    (double) pAllChapters[i1].blockDatLenReal,
                                    pAllChapters[i1].blockHeaLen,
                                    pAllChapters[i1].filesListLen,
                                    pAllChapters[i1].chapterName);
                    dbAllocComplete   += (double) pAllChapters[i1].chapterSize + (double) pAllChapters[i1].blockDatLenReal;
                    dbAllocBlockData  += (double) pAllChapters[i1].blockDatLenReal + (double) pAllChapters[i1].blockDatLen;
                }
                fflush(stdout);
                lAllocBlockHeader += pAllChapters[i1].blockHeaLen;
                lAllocFiles       += pAllChapters[i1].filesListLen;
            }
            else
            {
                if ((opt_chapterno == -1) || (opt_chapterno == i1))
                {
                    if (bShowMatchesOverAllChapter == 0)
                    {
                        // no raw mode -> print chapter info
                        if ((opt_rawmode == 0) && ((opt_verbose > VERBOSITY_MEDIUM) || (opt_chapterno == -1)))
                        {
                            ndb_chap_printChapterInfo(&pAllChapters[i1], opt_verbose);
                        }
                    }
                    else if (opt_verbose > VERBOSITY_SMALL)
                    {
                        tim = localtime( (const time_t *) & pAllChapters[i1].chapterTime);
                        if (tim != NULL)
                                    strftime(stime, 40, "%Y-%m-%d %H:%M:%S", tim);
                        else
                        {
                            strcpy(stime, "[DATE ERROR]       ");
                            fprintf(stderr, "warning: creation date of chapter %04d corrupt\n", pAllChapters[i1].chapterNo);
                            fflush (stderr);
                        }
                        fprintf(stdout, "Chapter %d, \"%s\", %s\n\n",
                        pAllChapters[i1].chapterNo, pAllChapters[i1].chapterName, stime);
                    }
                    // raw mode? -> print only raw info
                    if (opt_rawmode == 1)
                    {
                        ndb_chap_printChapterInfoRaw(&pAllChapters[i1]);
                    }
                    // collect values for chapter summary
                    ulCountChap++;
                    dbSizeOriChap += (double) pAllChapters[i1].allFileLenOri;
                    dbSizeZipChap += (double) pAllChapters[i1].blockDatLenReal + (double) pAllChapters[i1].blockHeaLen;
                }
            }

            // show files of current chapter
            if ((opt_chapterno == i1) || (bShowMatchesOverAllChapter == 1))
            {
                if (opt_rawmode == 0)
                {
                    if ((opt_verbose == VERBOSITY_SMALL) && ((bShowMatchesOverAllChapter == 1) && (opt_changedfilesonly == 0)))
                        fprintf(stdout, "\n");
                    if (opt_changedfilesonly == 0)
                    {
                        if (sMask[0] == '\0')
                            fprintf(stdout, "File List of Chapter %d (all archived files):\n", i1);
                        else
                            fprintf(stdout, "File List of Chapter %d (all matching files):\n", i1);
                    }
                    else if (opt_verbose != VERBOSITY_SMALL)
                    {
                        if (bShowMatchesOverAllChapter == 1)
                            fprintf(stdout, "File List of Chapter(s) (only changed files):\n");
                        else
                            fprintf(stdout, "File List of Chapter %d (only changed files):\n", i1);
                    }

                    if (opt_showhashfilename == 1)
                    {
                        fprintf(stdout, "MD5                               Filename\n");
                        fprintf(stdout, "--------------------------------  --------------------------------------------\n");
                    }
                    else
                    {
                        if (opt_verbose == VERBOSITY_SMALL)
                        {
                            if (opt_changedfilesonly == 0)
                            {
                                fprintf(stdout, "Chap      Date  Time   MD5/CRC       Size     Packed Ratio  Filename          \n");
                                fprintf(stdout, "----  -------- -----  --------  ---------  --------- -----  ------------------\n");
                            }
                        }
                        else if (opt_verbose == VERBOSITY_MEDIUM)
                        {
                            fprintf(stdout, "    Date  Time   MD5/CRC       Size     Packed Ratio  Filename              \n");
                            fprintf(stdout, "-------- -----  --------  ---------  --------- -----  ----------------------\n");
                        }
                        else
                        {
                            fprintf(stdout, "    Date     Time   MD5/CRC       Size     Packed Ratio  Filename              \n");
                            fprintf(stdout, "-------- --------  --------  ---------  --------- -----  ----------------------\n");
                        }
                    }
                }

                if (ndb_getDebugLevel() >= 1)
                    fprintf(stdout, "showing content of chapter %d\n", i1);
                currChapter = pAllChapters[i1];

                // read all block header of current chapter
                if (opt_showblocks == 1)
                {
                    retval = ndb_chap_readAllBlockHeader(&currChapter, fArchive);
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stderr, "Error: reading block header from chapter %d\n", currChapter.chapterNo);
                        exit (retval);
                    }
                }
                retval = ndb_chap_readAllFileEntries(&currChapter, fArchive);
                if (retval != NDB_MSG_OK)
                {
                    fprintf(stderr, "Error: reading file entries from chapter %d\n", currChapter.chapterNo);
                    exit (retval);
                }

                // all block header are read already, now add them to the last chapter
                if (opt_showblocks == 1)
                {
                    retval = ndb_chap_rebuildBlockHeaderList(&currChapter);
                    if (retval != NDB_MSG_OK)
                    {
                        fprintf(stderr, "Error: rebuilding block header list of chapter %d resulted in %d\n",
                                        currChapter.chapterNo, retval);
                        exit(retval);
                    }
                }

                // count files, sizes, zipped sizes
                ulCountEntries = 0;
                dbSizeOri = 0;
                dbSizeZip = 0;

                pFile = currChapter.pFirstFile;
                while (pFile != NULL)
                {
                    // check wheather filename matches file mask
                    if ((sMask[0] == '\0') || (1 == ndb_osdep_matchFilename(sMask, pFile->filenameUTF8)))
                    {
                        if ((opt_changedfilesonly == 0) || ((pFile->lenZip8 > 0) && ((pFile->flags & NDB_FILEFLAG_IDENTICAL) != NDB_FILEFLAG_IDENTICAL)))
                        {
                            if (opt_rawmode == 0)
                            {
                                if (opt_verbose == VERBOSITY_SMALL)
                                {
                                        fprintf(stdout, "%04d  ", i1);
                                }
                                if (opt_showhashfilename == 1)
                                    ndb_fe_printFileEntryHashAndName(pFile, opt_verbose);
                                else
                                    ndb_fe_printFileEntryInfo(pFile, opt_verbose);
                                ulCountEntries++;
                                if ((pFile->attributes & NDB_FILEFLAG_DIR) == NDB_FILEFLAG_DIR)
                                    ulCountDirs++;
                                dbSizeOri += pFile->lenOri8;
                                dbSizeZip += pFile->lenZip8;
                                // show block list
                                if (opt_showblocks == 1)
                                {
                                    pBlock = pFile->firstBlockHeader;
                                    for (l1 = 0; l1 < pFile->blockCount; l1++)
                                    {
                                        if (pBlock == NULL)
                                        {
                                            fprintf(stderr, "Error: block %lu of %lu is NULL\n",
                                                    l1, pFile->blockCount);
                                            fprintf(stderr, "       aborting block list\n");
                                            fflush(stderr);
                                            break;
                                        }
                                        // create output
                                        strcpy (sExtraType, "[unknown]");
                                        if (pBlock->blockType == NDB_BLOCKTYPE_OS2EAS)
                                                strcpy (sExtraType, "[OS/2 EAs]");
                                        else if (pBlock->blockType == NDB_BLOCKTYPE_NTSECURITY)
                                                strcpy (sExtraType, "[NTFS Sec.]");
                                        else if (pBlock->blockType == NDB_BLOCKTYPE_OS2ACLS)
                                                strcpy (sExtraType, "[OS/2 ACLs]");
                                        else if (pBlock->blockType == NDB_BLOCKTYPE_UNIX)
                                                strcpy (sExtraType, "[Unix]");
                                        else if (pBlock->blockType == NDB_BLOCKTYPE_FILEDATA)
                                                strcpy (sExtraType, "[File Data]");
                                        if (pBlock->lenOri > 0)
                                            snprintf(sratio, 10 - 1, "%3d%%", (int) (100 - (100.0 * pBlock->lenZip)/pBlock->lenOri));
                                        else
                                            strcpy(sratio, "n.a.");
                                        fprintf(stdout, "   B%04lu %c %3s  %08lX  %9u  %9u %5s  %02d/%09lu %s\n",
                                                        pBlock->blockNo,
                                                        ((pBlock->blockInfo & NDB_BLOCKINFO_DUPLICATEDATA) == NDB_BLOCKINFO_DUPLICATEDATA) ? 'I' : 'N',
                                                        ((pBlock->blockInfo & NDB_BLOCKINFO_EXTRADATA) == NDB_BLOCKINFO_EXTRADATA) ? "EXT" : "FIL",
                                                        pBlock->crc32,
                                                        pBlock->lenOri,
                                                        pBlock->lenZip,
                                                        sratio,
                                                        pBlock->dataFileNo,
                                                        pBlock->staDat,
                                                        sExtraType
                                    );
                                    pBlock = pBlock->nextBlock;
                                }
                            }
                        }
                        else
                        {
                            ndb_fe_printFileEntryInfoRaw(pFile);

                        }
                    }
                }
                pFile = pFile->nextFile;
            }

            // print summary lines
            if (ulCountEntries == 0)
            {
                if (opt_rawmode == 0)
                {
                    if (opt_verbose > VERBOSITY_SMALL)
                    {
                        fprintf(stdout, "[no files matching view criteria or chapter empty]\n");
                        fflush(stdout);
                    }
                }
            }
            else
            {
                if ((opt_rawmode == 0) && (bShowMatchesOverAllChapter == 0))
                {
                    snprintf(sCountFiles, 60 - 1, "%lu = %lu Dirs + %lu Files", ulCountEntries, ulCountDirs, ulCountEntries - ulCountDirs);
                    if (dbSizeOri > 0)
                        snprintf(sratio, 10 - 1, "%3d%%", (int) (100.0 - ((100.0 * dbSizeZip)/dbSizeOri)));
                    else
                        strcpy (sratio, "n.a.");

                    if (opt_showhashfilename == 1)
                    {
                        fprintf(stdout, "--------------------------------  --------------------------------------------\n");
                        fprintf(stdout, "%32s  %s\n",
                                        " ",
                                        sCountFiles);
                    }
                    else
                    {
                        sprintf(sSizeOri, "%13.0f", dbSizeOri);
                        if (dbSizeOri > 9999.0)
                            sprintf(sSizeOri, "%10.2fK", dbSizeOri / 1024.0);
                        if (dbSizeOri > 9999999.0)
                            sprintf(sSizeOri, "%10.2fM", dbSizeOri / 1048576.0);
                        if (dbSizeOri > 9999999999.0)
                            sprintf(sSizeOri, "%10.2fG", dbSizeOri / 1073741824.0);
                        sprintf(sSizeZip, "%10.0f", dbSizeZip);
                        if (dbSizeZip > 9999.0)
                            sprintf(sSizeZip, "%7.2fK", dbSizeZip / 1024.0);
                        if (dbSizeZip > 9999999.0)
                            sprintf(sSizeZip, "%7.2fM", dbSizeZip / 1048576.0);
                        if (dbSizeZip > 9999999999.0)
                            sprintf(sSizeZip, "%7.2fG", dbSizeZip / 1073741824.0);

                        if (opt_verbose == VERBOSITY_SMALL)
                        {
                            fprintf(stdout, "----  -------- -----  --------  ---------  --------- -----  ------------------\n");
                            fprintf(stdout, "%20s  %5s %13s %10s  %4s  %-22s\n",
                                            " ",
                                            " ",
                                            sSizeOri,
                                            sSizeZip,
                                            sratio,
                                            sCountFiles);
                        }
                        else if (opt_verbose == VERBOSITY_MEDIUM)
                        {
                            fprintf(stdout, "-------- -----  --------  ---------  --------- -----  ----------------------\n");
                            fprintf(stdout, "%14s  %5s %13s %10s  %4s  %-22s\n",
                                            " ",
                                            " ",
                                            sSizeOri,
                                            sSizeZip,
                                            sratio,
                                            sCountFiles);
                        }
                        else
                        {
                            fprintf(stdout, "-------- --------  --------  ---------  --------- -----  ----------------------\n");
                            fprintf(stdout, "%17s  %5s %13s %10s  %4s  %-22s\n",
                                            " ",
                                            " ",
                                            sSizeOri,
                                            sSizeZip,
                                            sratio,
                                            sCountFiles);
                        }
                    }

                    fflush(stdout);
                }
            }
            if ((opt_verbose > VERBOSITY_SMALL) && (bShowMatchesOverAllChapter == 1))
            {
                fprintf(stdout, "\n");
            }
        }

        // position for next chapter
        currFilePos = pAllChapters[i1].ownFilePosition + NDB_CHUNKCHAPTER_SIZEALL
                                                       + pAllChapters[i1].chapterSize;
        fseek(fArchive, currFilePos, SEEK_SET);
    }




    if ((opt_chapterno == -1) && (bShowMatchesOverAllChapter == 0))
    {
        // print chapter summary
        snprintf(sCountChap, 20 - 1, "%lu chapters", ulCountChap);
        if (opt_allocation == 1)
        {
            fprintf(stdout, "----  ----------  -----------  ------------  ----------  --------------------\n");
            fprintf(stdout, "      %10.0f   %10.0f     %9lu   %9lu  %s\n",
            dbAllocComplete, dbAllocBlockData, lAllocBlockHeader, lAllocFiles,  sCountChap);
        }
        else if (opt_rawmode == 0)
        {
            sprintf(sSizeOri, "%10.0f", dbSizeOriChap);
            if (dbSizeOriChap > 9999.0)
                sprintf(sSizeOri, "%7.2fK", dbSizeOriChap / 1024.0);
            if (dbSizeOriChap > 9999999.0)
                sprintf(sSizeOri, "%7.2fM", dbSizeOriChap / 1048576.0);
            if (dbSizeOriChap > 9999999999.0)
                sprintf(sSizeOri, "%7.2fG", dbSizeOriChap / 1073741824.0);
            sprintf(sSizeZip, "%10.0f", dbSizeZipChap);
            if (dbSizeZipChap > 9999.0)
                sprintf(sSizeZip, "%7.2fK", dbSizeZipChap / 1024.0);
            if (dbSizeZipChap > 9999999.0)
                sprintf(sSizeZip, "%7.2fM", dbSizeZipChap / 1048576.0);
            if (dbSizeZipChap > 9999999999.0)
                sprintf(sSizeZip, "%7.2fG", dbSizeZipChap / 1073741824.0);

            if (dbSizeOriChap > 0)
                snprintf(sratio, 10 - 1, "%3d%%", (int) (100.0 - ((100.0 * dbSizeZipChap)/dbSizeOriChap)));
            else
                strcpy (sratio, "n.a.");
                if ((opt_verbose == VERBOSITY_SMALL) || (opt_verbose == VERBOSITY_MEDIUM))
                {
                    fprintf(stdout, "----  ----------------  -----  ---------  --------- -----  --------------------\n");
                    fprintf(stdout, "                              %10s %10s %5s  %-20s\n",
                        sSizeOri,
                        sSizeZip,
                        sratio,
                        sCountChap);
                }
                else
                {
                    fprintf(stdout, "----------------------------------------------------------------------------\n");
                    fprintf(stdout, "                           %-18s %10s %10s   %6s\n",
                        sCountChap,
                        sSizeOri,
                        sSizeZip,
                        sratio);
                }
            }
        }

    }
    else
    {
        // new created (=empty) archive file
        fprintf(stdout, "Archive File contains no chapters\n");
        fflush(stdout);
        exit (retval);
    }

    fclose(fArchive);

    ndb_alloc_done();

    if ((opt_verbose > VERBOSITY_SMALL) && (opt_rawmode == 0))
    {
        time_startup = 0;
        time(&time_startup);
        fprintf(stdout, "\nNDBList: finished at %s\n", ctime(&time_startup));
    }

    return(0);
}


/*
    *************************************************************************************
        TODO: Dummy functions to satisfy the linker
    *************************************************************************************
*/

int ndb_expandExtraStream(struct ndb_s_fileentry *pFile,
                          FILE *fArchive, const char *sDefaultPath, int opt_testintegrity)
{
        return NDB_MSG_OK;
}
