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
#include "ndb.h"
#include "ndb_prototypes.h"

#if defined(NDB_USE_ZLIB)
#include "zlib.h"
#endif

#if defined(NDB_USE_BZIP2)
#include "bzlib.h"
#endif

#include <string.h>
#include <stdlib.h>


#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        /* exit(1); */ \
    } \
}


extern int currBlockSize;
extern int currCompression;




/* ======================================================================== */
/*     pack buffer                                                          */
/* ======================================================================== */


USHORT ndb_pack (struct ndb_s_blockheader *pBlock)
{
    int err;
    uLongf newLenZip;
    uLongf oldLenOri;
#if defined(TEST_PERFORMANCE)
	// debug: time measurement
	static ULONG dbg_count   = 0;
	static ULONG dbg_timall = 0;
	static ULONG dbg_timmax = 0;
	ULONG8 start_time;
	ULONG8 curr_time;
	ULONG dbg_clock_diff    = 0;
    char s[200];
#endif

    if (ndb_getDebugLevel() >= 6)
        fprintf (stdout, "ndb_pack - startup\n");

    newLenZip = (uLongf) (currBlockSize + 4096);
    // current lenOri is set to blocksize + 4096
    oldLenOri = ((uLongf) pBlock->lenOri) & 0xffff;

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf (stdout, "ndb_pack: starting compress2() with compression level %d\n", currCompression);
        fprintf (stdout, "          source buffer  0x%lX\n", (ULONG) pBlock->BlockDataOri);
        fprintf (stdout, "          source length  0x%lX\n", oldLenOri);
        fprintf (stdout, "          dest buffer    0x%lX\n", (ULONG) pBlock->BlockDataZip);
        fprintf (stdout, "          dest length    0x%lX\n", newLenZip);
    }

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_filefound: start_time = %llu\n", start_time);
#endif

#if defined(NDB_USE_ZLIB)
    err = compress2 ((Bytef *) pBlock->BlockDataZip, &newLenZip,
                     (Bytef *) pBlock->BlockDataOri, oldLenOri, currCompression);
    CHECK_ERR(err, "compress2");
#endif
#if defined(NDB_USE_BZIP2)
    err = BZ2_bzBuffToBuffCompress ((char *) pBlock->BlockDataZip, (unsigned int *) &newLenZip,
                                    (char *) pBlock->BlockDataOri, oldLenOri, currCompression, 0, 0);
#endif

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_pack: count %lu, average %lu, max %lu, sum %lu\n",
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);
        ndb_util_set_performancetest(TEST_PERFORMANCE_PACK, s);
    }
#endif

    pBlock->lenZip = (USHORT) newLenZip & 0xffff;

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf (stdout, "  packing block data from %5d to %5d", pBlock->lenOri, pBlock->lenZip);
        if ((pBlock->lenOri > 0) && (pBlock->lenZip > 0))
        {
            fprintf (stdout, "  (%d%%)\n",  - (int) (100.0 - ((100.0*pBlock->lenZip) / pBlock->lenOri)));
        } else
        {
            fprintf (stdout, "  (0%%)\n");
        }
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf (stdout, "ndb_pack - finished\n");
    fflush(stdout);

    return pBlock->lenZip;
}


/* ======================================================================== */
/*     unpack buffer                                                        */
/* ======================================================================== */



USHORT ndb_unpack (struct ndb_s_blockheader *pBlock)
{
    int err;
    uLongf newLenOri;
    uLongf oldLenZip;
#if defined(TEST_PERFORMANCE)
	// debug: time measurement
	static ULONG dbg_count   = 0;
	static ULONG dbg_timall = 0;
	static ULONG dbg_timmax = 0;
	ULONG8 start_time;
	ULONG8 curr_time;
	ULONG dbg_clock_diff    = 0;
    char s[200];
#endif

    if (ndb_getDebugLevel() >= 6)
        fprintf (stdout, "ndb_unpack - startup\n");

    newLenOri = (uLongf) (currBlockSize + 4096);
    oldLenZip = (uLongf) pBlock->lenZip;

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf (stdout, "ndb_unpack: starting uncompress()\n");
        fprintf (stdout, "            source buffer  0x%lX\n", (ULONG) pBlock->BlockDataZip);
        fprintf (stdout, "            source length  0x%lX\n", oldLenZip);
        fprintf (stdout, "            dest buffer    0x%lX\n", (ULONG) pBlock->BlockDataOri);
        fprintf (stdout, "            dest length    0x%lX\n", newLenOri);
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf (stdout, "ndb_unpack: start of uncompress()\n");

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_filefound: start_time = %llu\n", start_time);
#endif

#if defined(NDB_USE_ZLIB)
    err = uncompress((Bytef *) pBlock->BlockDataOri, &newLenOri,
                     (Bytef *) pBlock->BlockDataZip, oldLenZip);
    CHECK_ERR(err, "uncompress");
#endif
#if defined(NDB_USE_BZIP2)
    err = BZ2_bzBuffToBuffDecompress ((char *) pBlock->BlockDataOri, (unsigned int *) &newLenOri,
                                    (char *) pBlock->BlockDataZip, oldLenZip, 0, 0);
#endif

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_unpack: count %lu, average %lu, max %lu, sum %lu\n",
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);
        ndb_util_set_performancetest(TEST_PERFORMANCE_UNPACK, s);
    }
#endif

    if (ndb_getDebugLevel() >= 6)
        fprintf (stdout, "ndb_unpack - finished\n");
    fflush(stdout);

    return (USHORT) (newLenOri & 0xffff);
}

