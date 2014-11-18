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
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ndb.h"
#include "ndb_prototypes.h"

#ifdef __MINGW32__
   int _CRT_glob = 0;   /* suppress command line globbing by C RTL */
#endif


// used by other modules also
int currBlockSize   = 0;
int currCompression = 0;


/*
    *************************************************************************************
        main()

        - processes and checks the command line parameters
        - creates a corresponding, empty archive chapter (with no chapters)
    *************************************************************************************
*/

int main(int argc, char **argv)
{
    char s_container[NDB_MAXLEN_FILENAME];
    FILE *fArchive;
    int i;
    int opt_overwrite  = 0;
    int opt_debug      = 0;
    int opt_blocksize  = NDB_DEFAULT_BLOCKSIZE;
    int opt_compress_level = NDB_DEFAULT_COMPRESSION;
    ULONG opt_datafilesize  = 0;
    time_t time_startup;
    int len;
    int dot_found;
    char *p = NULL;
        char text[200] = "";
    char comment[200] = "";
    struct stat statFile;
        char stime_m[40];
    struct tm *tim_m;
    char c;
        char sIdent[100] = "\0";

    struct ndb_s_c_archive newarchive =
    {
        NDB_MAGIC_ARCHIV,             /* SAVE: 04B: magic for archive header */
        NDB_CHUNKARCHIVE_VERCHUNK,    /* SAVE: 02B: chunk format version */
        NDB_CHUNKARCHIVE_LENCHUNK,    /* SAVE: 02B: length of rest of chunk */
        0,                            /* SAVE: 02B: CRC16 of header structure data */
        NDB_DEFAULT_BLOCKSIZE,        /* SAVE: 02B: blocksize for data blocks */
        NDB_DEFAULT_COMPRESSION,      /* SAVE: 02B: compression level of zlib */
        0,                            /* SAVE: 02B: number of included chapters */
        0LL,                          /* SAVE: 08B: size of archive without data files */
        0LL,                          /* SAVE: 08B: size of archive incl. data files */
        (ULONG) NULL,                 /* SAVE: 04B: start of first chapter */
        (ULONG) NULL,                 /* SAVE: 04B: start of last chapter */
        0,                            /* SAVE: 01B: number of data files  */
        0L,                           /* SAVE: 04B: max. size of data files  */
        0L,                           /* SAVE: 04B: Erzeugungsdatum Archiv */
        "[ndbver]",                   /* SAVE: 20B: ASCII version of NDB */
        "[comment]",                  /* SAVE: 80B: comment for archive */
        0L,                           /* SAVE: 04B: identification for NDB main file */
        (ULONG) NULL,                 /* TRANS: own position in archive file */
        NULL                          /* TRANS: pointer to list with data files*/
    };


    time_startup = 0;
    time(&time_startup);
    // ctime() ends with '\n' already
    fprintf(stdout, "NDBCreate %s: startup at %s", NDB_VERSION, ctime(&time_startup));
    fprintf(stdout, "%s\n", NDB_COPYRIGHT);
    fprintf(stdout, "Using %s and a few source parts of %s and %s\n", NDB_USING_ZLIB, NDB_USING_ZIP, NDB_USING_UNZIP);
    fprintf(stdout, "\n");
    fflush(stdout);

// 2003-Oct-21; pw; remove to reduce size of executable
//    ndb_alloc_init(500);

    // ========================================================================
    // init functions
    // ========================================================================
// 2003-Oct-21; pw; remove to reduce size of executable
//    ndb_utf8_inittables(ndb_osdep_getCodepage());
//    ndb_osdep_init();

    if (argc == 1)
    {
        fprintf(stdout, "usage: ndbcreate <archive>.ndb [-o] [-b<blocksize>] [-l<compressionlevel>]\n");
        fprintf(stdout, "       -o:              *o*verwrite existing file without any question\n");
        fprintf(stdout, "       -b<size>:        *b*lock size, must be between 1024 and 61440\n");
        fprintf(stdout, "       -f<size>:        data*f*ile size, must be between 64k and 2g\n");
        fprintf(stdout, "       -l<level>:       use zlib compression *l*evel <level>, 0..9\n");
        fprintf(stdout, "       -m\"<comment>\":   add co*m*ment (max. 79 chars)\n");
        fprintf(stdout, "       -d<level>:       set *d*ebug level 1..9\n");
        exit(0);
    }
    else
    {
        strcpy(s_container, (char *) argv[1]);
        // add ".ndb" if missing
        len = strlen(s_container);
        dot_found = 0;
        for (i = 0; i < len; i++)
        {
            if (s_container[i] == '.')
                dot_found = 1;
            else if ((s_container[i] == '/') || (s_container[i] == '\\'))
                dot_found = 0;
        }
        if (dot_found == 0)
            strcat(s_container,".ndb");
        /* check for optional arguments */
        for (i = 2; i < argc; i++)
        {
            p = (char *) argv[i];
            if ((*p) == '-')
            {
                p++;

                c = *p;
                if (c == 'o')
                {
                    opt_overwrite = 1;
                    if (ndb_getDebugLevel() >= 5)
                            fprintf(stdout, "option overwrite %d\n", opt_overwrite);
                }
                else if (c == 'd')
                {
                    c = *(++p);
                    if ((c >= '0') && (c <= '9'))
                    {
                        opt_debug = c - '0';
                        ndb_setDebugLevel(opt_debug);
                    }
                }
                else if (c == 'l')
                {
                    c = *(++p);
                    if ((c >= '0') && (c <= '9'))
                    {
                        opt_compress_level = c - '0';
                        if (ndb_getDebugLevel() >= 5)
                            fprintf(stdout, "option: compression set to %d\n", opt_compress_level);
                    }
                }
                else if (c == 'b')
                {
                    opt_blocksize = atoi(++p);
                    while (isdigit((int) *p))
                        p++;
                    if ((*p == 'k') || (*p == 'K'))
                        opt_blocksize *= 1024;
                    if (ndb_getDebugLevel() >= 5)
                        fprintf(stdout, "option: blocksize set to %d\n", opt_blocksize);
                    if ((opt_blocksize < 512) || (opt_blocksize > 61440))
                    {
                        fprintf(stdout, "Error: blocksize must be between 512 and 61440 (default)\n");
                        fprintf(stdout, "       recommend value is 60K\n");
                        exit (NDB_MSG_NOTOK);
                    }
                }

                else if (c == 'f')
                {
                    opt_datafilesize = atoi(++p);
                    while (isdigit((int) *p))
                        p++;
                    if ((*p == 'k') || (*p == 'K'))
                        opt_datafilesize *= 1024L;
                    if ((*p == 'm') || (*p == 'M'))
                        opt_datafilesize *= 1024L * 1024L;
                    if ((*p == 'g') || (*p == 'G'))
                        opt_datafilesize *= 1024L * 1024L * 1024L;
                    if (ndb_getDebugLevel() >= 5)
                        fprintf(stdout, "option: datafile size set to %lu\n", opt_datafilesize);
                    if ((opt_datafilesize != 0L) && (opt_datafilesize < 65536L))
                    {
                        fprintf(stdout, "Error: datafile size may not less than 64K.\n");
                        fprintf(stdout, "Therefore setting datafile size to minimum of 64k.\n\n");
                        opt_datafilesize = 65536L;

                    }
                    if ((opt_datafilesize != 0L) && (opt_datafilesize >= 2147483647L))
                    {
                        fprintf(stdout, "Error: datafile size may not exceed 2GB.\n");
                        fprintf(stdout, "Therefore setting datafile size to maximun of 2147.483.647 bytes.\n\n");
                        opt_datafilesize = 2147483647L;
                    }
                }

                else if (c == 'm')
                {
                    c = *(++p);
                    if (c == '\"') p++;
                    strcpy (comment, p);
                    if (p[strlen(p)] == '\"')
                        p[strlen(p)] = '\0';
                    if (ndb_getDebugLevel() >= 5)
                        fprintf(stdout, "option: added archiv comment '%s'\n", comment);
                }
                else
                {
                    fprintf(stdout, "unknown option -%s\n", p);
                }
            }
            else
            {
                fprintf(stdout, "unknown option %s\n", p);
            }
        }
    }

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "NDBArchive: container file is %s\n", s_container);
        fprintf(stdout, "NDBArchive: compression    is %d\n", opt_compress_level);
        fprintf(stdout, "NDBArchive: blocksize      is %d\n", opt_blocksize);
        fprintf(stdout, "NDBArchive: datafile size  is %lu\n", opt_datafilesize);
        fflush(stdout);
    }


    // check whether a file with the same name does already exist?
    if (NDB_MSG_OK == ndb_io_check_exist_file(s_container, &statFile))
    {
        // file found
        if (opt_overwrite == 0)
        {
            tim_m = localtime( (const time_t *) &statFile.st_mtime);
            strftime(stime_m, 40, "%Y-%m-%d, %H:%M:%S", tim_m);
            snprintf(text, 200 - 1, "The file '%s' (%.0f bytes, %s) already exists.\n%s",
                            s_container, (double) statFile.st_size, stime_m,
                            "Are you sure to overwrite it (y/n)?");
            // let user answer
            c = ndb_util_prompt_user(text, "yYnN");
            if (c == 'N' || c == 'n')
            {
                    fprintf(stdout, "Exit without overwriting '%s'.\n", s_container);
                    fflush(stdout);
                    exit(NDB_MSG_OK);
            }
        }
        else
        {
                fprintf(stdout, "Info: overwriting old file '%s'\n\n", s_container);
                fflush (stdout);
        }
    }

    // set values in archive chunk
    newarchive.compression    = (NDB_CURRENT_COMPRESSION << 8) + opt_compress_level;
    newarchive.blockSize      = opt_blocksize;
    newarchive.dataFilesSize  = opt_datafilesize;
    newarchive.timestamp      = time(NULL);
    strncpy(newarchive.ndbver,  NDB_VERSION, 19);
    strncpy(newarchive.comment, comment, 79);

    // create identification
    snprintf(sIdent, 100 - 1, "%s|%s|%s", s_container, NDB_VERSION, ctime(&time_startup));
    newarchive.identCRC = ndb_crc32(0L, sIdent, strlen(sIdent));
    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "NDBArchive: ident string is %s", sIdent);
        fprintf(stdout, "NDBArchive: ident CRC is    %lX\n", newarchive.identCRC);
        fflush(stdout);
    }

    // create new archive file and save to disk
    fArchive = fopen(s_container,"wb+");
    if (fArchive == NULL)
    {
        fprintf(stderr, "Error: could not create archive file '%s'\n", s_container);
        exit (NDB_MSG_NOFILEOPEN);
    }

    ndb_io_writechunkarchive(fArchive, &newarchive);

    // flush and close archive file
    fclose(fArchive);

    fprintf(stdout, "Archiv file '%s' created with:\n", s_container);
    fprintf(stdout, "- zip level %d\n", opt_compress_level);
    fprintf(stdout, "- blocksize %d bytes\n", opt_blocksize);
    if (opt_datafilesize == 0)
    {
            fprintf(stdout, "- using no datafiles\n");
    }
    else
    {
        if (opt_datafilesize < 65536)
            fprintf(stdout, "- datafile size %lu bytes\n", opt_datafilesize);
        else if (opt_datafilesize < 1024L * 1024L)
            fprintf(stdout, "- datafile size %.2f kilobytes\n", (opt_datafilesize / 1024.0));
        else
            fprintf(stdout, "- datafile size %.2f megabytes\n", (opt_datafilesize / (1024.0 * 1024.0)));
    }
    fprintf(stdout, "- archive identification CRC %lX\n", newarchive.identCRC);

//    ndb_alloc_done();

    time_startup = 0;
    time(&time_startup);
    fprintf(stdout, "\nNDBCreate: finished at %s", ctime(&time_startup));

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
        TODO: Dummy functions to satisfy the linker, has to be corrected
    *************************************************************************************
*/
FILE *ndb_df_getReadHandle(int fileNo)
{
        return NULL;
}

ULONG8 ndb_osdep_getPreciseTime()
{
        return 0LL;
}
