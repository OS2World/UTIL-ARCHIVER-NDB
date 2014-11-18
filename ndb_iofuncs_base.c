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
#include <sys/types.h>
#include <sys/stat.h>

#include "ndb.h"
#include "ndb_prototypes.h"



int opt_arcsymlinks = 0;


/* ======================================================================== */
/* functions for writing and reading the archive start chunk                */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_io_writechunkarchive()

        save the current archive file position to the archive structure (to allow
        later writing of corrected structure data easily);
        writes the archive chunk structure to the archive file handle
        *************************************************************************************
*/

int ndb_io_writechunkarchive(FILE *fArchive, struct ndb_s_c_archive *pArchive)
{
    int count;
    int retval = NDB_MSG_OK;
    char buf[NDB_CHUNKARCHIVE_SIZEALL];
    int count_crcbefore = 0;
    int count_crcafter  = 0;

    /* remember current file position for ease refreshing afterwards */
    rewind(fArchive);
    pArchive->ownFilePosition = ftell(fArchive);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_writechunkarchive: writing at file position 0x%lX\n",
                        pArchive->ownFilePosition);
        fflush(stdout);
    }

    if ((ftell(fArchive) & 3) != 0)
        fprintf(stdout, "Error ------------------------------------------------\nno quad file position\n");

    count = 0;
    pArchive->crc16Hea = 0;

    count = ndb_write2buf_ulong (buf, &count, pArchive->magic);
    count = ndb_write2buf_ushort(buf, &count, pArchive->verChunk);
    count = ndb_write2buf_ushort(buf, &count, pArchive->lenChunk);
    count_crcbefore = count;
    count = ndb_write2buf_ushort(buf, &count, pArchive->crc16Hea);
    count_crcafter  = count;
    count = ndb_write2buf_ushort(buf, &count, pArchive->blockSize);
    count = ndb_write2buf_ushort(buf, &count, pArchive->compression);
    count = ndb_write2buf_ushort(buf, &count, pArchive->noOfChapters);
    count = ndb_write2buf_ulong8(buf, &count, pArchive->archiveSize);
    count = ndb_write2buf_ulong8(buf, &count, pArchive->archiveSizeReal);
    count = ndb_write2buf_ulong (buf, &count, pArchive->firstChapter);
    count = ndb_write2buf_ulong (buf, &count, pArchive->lastChapter);
    count = ndb_write2buf_ushort(buf, &count, pArchive->dataFilesCount);
    count = ndb_write2buf_ulong (buf, &count, pArchive->dataFilesSize);
    count = ndb_write2buf_ulong (buf, &count, pArchive->timestamp);
    count = ndb_write2buf_char_n(buf, &count, pArchive->ndbver,  NDB_MAXLEN_NDBVER);
    count = ndb_write2buf_char_n(buf, &count, pArchive->comment, NDB_MAXLEN_COMMENT);
    count = ndb_write2buf_ulong (buf, &count, pArchive->identCRC);

    // calculate crc16 for buffer with header struct
    pArchive->crc16Hea = ndb_crc16(0, &buf[count_crcafter], count - count_crcafter);
    // write crc16 into buffer
    ndb_write2buf_ushort(buf, &count_crcbefore, pArchive->crc16Hea);

    retval = ndb_io_writedata(buf, count, fArchive);

    if (retval != count)
    {
        return NDB_MSG_WRITEERROR;
    }
    else
        return NDB_MSG_OK;
}

/*
    *************************************************************************************
        ndb_io_readchunkarchive()

        reads the archive chunk structure from the archive file handle;
        check for the correct magic (NDB_MAGIC_ARCHIV);
        check that the chunk version is understood from this NDB version
    *************************************************************************************
*/

int ndb_io_readchunkarchive(FILE *fArchive, struct ndb_s_c_archive *pArchive)
{
    char buf[NDB_CHUNKARCHIVE_SIZEALL];
    int count;
    int count_aftercrc = 0;
    USHORT crc16 = 0;
    int retval = NDB_MSG_OK;
    ULONG ulSize = 0L;

    rewind(fArchive);

    /* remember current file position for ease refreshing afterwards */
    pArchive->ownFilePosition = ftell(fArchive);

    if ((ftell(fArchive) & 3) != 0)
    {
        fprintf(stdout, "Error ------------------------------------------------\nno quad file position\n");
        fflush(stdout);
    }

    ulSize = ndb_io_readdata(buf, NDB_CHUNKARCHIVE_SIZEALL, fArchive);
    if (ulSize != NDB_CHUNKARCHIVE_SIZEALL)
    {
        return NDB_MSG_READERROR;
    }

    count = 0;
    pArchive->magic = ndb_readbuf_ulong (buf, &count);
    if (pArchive->magic != NDB_MAGIC_ARCHIV)
    {
        fprintf(stderr, "bad archive - expected magic %X but got %lX\n", NDB_MAGIC_ARCHIV, pArchive->magic);
        fflush(stderr);
        return NDB_MSG_BADMAGIC;
    }
    pArchive->verChunk       = ndb_readbuf_ushort(buf, &count);
    if (pArchive->verChunk != NDB_CHUNKARCHIVE_VERCHUNK)
    {
        fprintf(stderr, "wrong archive chunk version - expected version '%X' but got '%X'\n", NDB_CHUNKARCHIVE_VERCHUNK, pArchive->verChunk);
        fprintf(stderr, "You need NDB V0.%d.x for this archive file.\n", pArchive->verChunk);
        fflush(stderr);
        return NDB_MSG_BADVERSION;
    }

    pArchive->lenChunk       = ndb_readbuf_ushort(buf, &count);
    pArchive->crc16Hea       = ndb_readbuf_ushort(buf, &count);
    count_aftercrc = count;
    pArchive->blockSize      = ndb_readbuf_ushort(buf, &count);
    pArchive->compression    = ndb_readbuf_ushort(buf, &count);
    pArchive->noOfChapters   = ndb_readbuf_ushort(buf, &count);
    pArchive->archiveSize    = ndb_readbuf_ulong8(buf, &count);
    pArchive->archiveSizeReal= ndb_readbuf_ulong8(buf, &count);
    pArchive->firstChapter   = ndb_readbuf_ulong (buf, &count);
    pArchive->lastChapter    = ndb_readbuf_ulong (buf, &count);
    pArchive->dataFilesCount = ndb_readbuf_ushort(buf, &count);
    pArchive->dataFilesSize  = ndb_readbuf_ulong (buf, &count);
    pArchive->timestamp      = ndb_readbuf_ulong (buf, &count);
    strcpy(pArchive->ndbver,  ndb_readbuf_char_n(buf, &count, NDB_MAXLEN_NDBVER));
    strcpy(pArchive->comment, ndb_readbuf_char_n(buf, &count, NDB_MAXLEN_COMMENT));
    pArchive->identCRC       = ndb_readbuf_ulong (buf, &count);

    // calculate crc16 for buffer with header struct
    crc16 = ndb_crc16(0, &buf[count_aftercrc], count - count_aftercrc);
    // compare with saved crc16
    if (pArchive->crc16Hea != crc16)
    {
        fprintf(stderr, "Warning: crc 0x%04X of archive header doesn't result in the saved crc 0x%04X\n", crc16, pArchive->crc16Hea);
        fflush(stderr);
//        retval = NDB_MSG_CRCERROR;
    }

    return retval;
}

/* ======================================================================== */
/* functions for writing and reading the chapter chunk                      */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_io_writechunkchapter()

        save the current archive file position to the chapter structure (to allow
        later writing of corrected structure data easily);
        writes the chapter chunk structure to the archive file handle
    *************************************************************************************
*/

int ndb_io_writechunkchapter(FILE *fArchive, struct ndb_s_c_chapter *pChapter)
{
    char buf[NDB_CHUNKCHAPTER_SIZEALL + 32];
    int count;
    int retval = NDB_MSG_OK;
    int count_crcbefore = 0;
    int count_crcafter  = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_io_writechunkchapter - startup\n");
        fflush(stdout);

    /* remember current file position for ease refreshing afterwards */
    pChapter->ownFilePosition = ftell(fArchive);
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_io_writechunkchapter: writing at file position 0x%lX\n",
                        pChapter->ownFilePosition);

    if ((ftell(fArchive) & 3) != 0)
    {
        fprintf(stdout, "Error ------------------------------------------------\nno quad file position\n");
        fflush(stdout);
    }

    count = 0;
    pChapter->crc16Hea = 0;

    count = ndb_write2buf_ulong (buf, &count, pChapter->magic);         /* magic for chapter header  */
    count = ndb_write2buf_ushort(buf, &count, pChapter->verChunk);      /* version of chapter format */
    count = ndb_write2buf_ushort(buf, &count, pChapter->lenChunk);      /* length of chunk rest      */
    count_crcbefore = count;
    count = ndb_write2buf_ushort(buf, &count, pChapter->crc16Hea);
    count_crcafter  = count;
    count = ndb_write2buf_ulong8(buf, &count, pChapter->chapterSize);   /* size of chapter without header */
    count = ndb_write2buf_char_n(buf, &count, pChapter->ndbver, NDB_MAXLEN_NDBVER);    /* NDB version (like archive chunk) */
    count = ndb_write2buf_ushort(buf, &count, pChapter->ndb_os);        /* NDB constant for OS */
    count = ndb_write2buf_ushort (buf, &count, pChapter->flags1);       /* Flags für zukünftige Erweiterungen */
    count = ndb_write2buf_ushort (buf, &count, pChapter->flags2);       /* Flags für zukünftige Erweiterungen */

    // Daten über Chapter
    count = ndb_write2buf_ushort (buf, &count, pChapter->chapterNo);    /* Chapternummer */
    count = ndb_write2buf_char_n(buf, &count, pChapter->chapterName, NDB_MAXLEN_CHAPNAME); /* Chaptername */

    count = ndb_write2buf_ulong (buf, &count, pChapter->chapterTime);   /* Erzeugungsdatum Chapter */
    count = ndb_write2buf_char_n(buf, &count, pChapter->codepage, NDB_MAXLEN_CODEPAGE);   /* Codepage of Chapter */
    count = ndb_write2buf_char_n(buf, &count, pChapter->comment,  NDB_MAXLEN_COMMENT);    /* Kommentar zum Chapter */
    // Daten über Blockliste
    count = ndb_write2buf_ulong8(buf, &count, pChapter->blockDatLen);   /* Länge der Blockchunks im NDB main file */
    count = ndb_write2buf_ulong8(buf, &count, pChapter->blockDatLenReal); /* tatsächliche Länger der gezippten Blöcke */
    count = ndb_write2buf_ulong (buf, &count, pChapter->blockDatSta);   /* Start der gezippten Blöcke */
    count = ndb_write2buf_ulong (buf, &count, pChapter->blockDatNo);    /* Gesamtanzahl aller Blöcke */
    // Daten über Block-Header-Liste
    count = ndb_write2buf_ulong (buf, &count, pChapter->blockHeaLen);   /* Länge der Block-Header */
    count = ndb_write2buf_ulong (buf, &count, pChapter->blockHeaSta);   /* Start der Block-Header */
    count = ndb_write2buf_ulong (buf, &count, pChapter->blockHeaNo);    /* Gesamtanzahl aller Block-Header */
    // Daten über File-Entry-Liste
    count = ndb_write2buf_ulong (buf, &count, pChapter->filesListLen);  /* Länge der Fileliste */
    count = ndb_write2buf_ulong (buf, &count, pChapter->filesListSta);  /* Start der Fileliste */
    count = ndb_write2buf_ulong (buf, &count, pChapter->filesNo);       /* Anzahl der Files */
    count = ndb_write2buf_ulong (buf, &count, pChapter->filesNoIdent);  /* Anzahl Files gleich zu letztem Chapter */
    count = ndb_write2buf_ulong8(buf, &count, pChapter->allFileLenOri); /* Gesamtlänge aller Files */

    count = ndb_write2buf_ushort (buf, &count, pChapter->noMaxDatFil);  /* highest datafile to extract? */
    count = ndb_write2buf_ushort (buf, &count, pChapter->noNewDatFil);  /* how many new datafiles? */
    count = ndb_write2buf_ulong  (buf, &count, pChapter->dataFilesHeaLen); /* Length of data files header chunk */
    count = ndb_write2buf_ulong  (buf, &count, pChapter->staDatFil);    /* (chunk) start of data files header */

    // calculate crc16 for buffer with header struct
    pChapter->crc16Hea = ndb_crc16(0, &buf[count_crcafter], count - count_crcafter);
    // write crc16 into buffer
    ndb_write2buf_ushort(buf, &count_crcbefore, pChapter->crc16Hea);

    retval = ndb_io_writedata(buf, count, fArchive);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_io_writechunkchapter - finished\n");
    fflush(stdout);

    if (retval != count)
    {
        fprintf(stdout, "tried to write %d bytes but could only %d bytes\n",
                        count, retval);
        fflush(stdout);
        return NDB_MSG_WRITEERROR;
    }
    else
        return NDB_MSG_OK;
}

/*
    *************************************************************************************
        ndb_io_readchunkchapter()

        reads the chapter chunk structure from the archive file handle;
        check for the correct magic (NDB_MAGIC_CHAPTER);
        check that the chunk version is understood from this NDB version
    *************************************************************************************
*/

int ndb_io_readchunkchapter(FILE *fArchive, struct ndb_s_c_chapter *pChapter)
{
    char buf[NDB_CHUNKCHAPTER_SIZEALL + 32];
    int count;
    ULONG ulSize = 0L;
    int count_aftercrc = 0;
    USHORT crc16 = 0;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_io_readchunkchapter - startup\n");
        fflush(stdout);

    /* remember current file position for ease refreshing afterwards */
    pChapter->ownFilePosition = ftell(fArchive);

    if ((ftell(fArchive) & 3) != 0)
    {
        fprintf(stdout, "Error ------------------------------------------------\nno quad file position\n");
        fflush(stdout);
    }

    ulSize = ndb_io_readdata(buf, NDB_CHUNKCHAPTER_SIZEALL, fArchive);
    if (ulSize != NDB_CHUNKCHAPTER_SIZEALL)
    {
            fprintf(stderr, "ndb_io_readchunkchapter: %s\n", ndb_util_msgcode2print(NDB_MSG_READERROR));
                fflush(stderr);
        return NDB_MSG_READERROR;
    }

    count = 0;
    pChapter->magic = ndb_readbuf_ulong (buf, &count);
    if (pChapter->magic != NDB_MAGIC_CHAPTER)
    {
        fprintf(stderr, "bad archive - expected magic %X but got %lX\n", NDB_MAGIC_CHAPTER, pChapter->magic);
                fflush(stderr);
        fflush(stderr);
        return NDB_MSG_BADMAGIC;
    }

    pChapter->verChunk           = ndb_readbuf_ushort(buf, &count);
    if (pChapter->verChunk != NDB_CHUNKCHAPTER_VERCHUNK)
    {
        fprintf(stdout, "wrong chapter chunk version - expected version %X but got %X\n", NDB_CHUNKCHAPTER_VERCHUNK, pChapter->verChunk);
        fflush(stdout);
        return NDB_MSG_BADVERSION;
    }

    pChapter->lenChunk           = ndb_readbuf_ushort(buf, &count);
    pChapter->crc16Hea           = ndb_readbuf_ushort(buf, &count);
    count_aftercrc = count;
    pChapter->chapterSize        = ndb_readbuf_ulong8(buf, &count);
    strcpy(pChapter->ndbver,       ndb_readbuf_char_n(buf, &count, NDB_MAXLEN_NDBVER));
    pChapter->ndbver[NDB_MAXLEN_NDBVER-1] = '\0';
    pChapter->ndb_os             = ndb_readbuf_ushort(buf, &count);
    pChapter->flags1             = ndb_readbuf_ushort(buf, &count);
    pChapter->flags2             = ndb_readbuf_ushort(buf, &count);

    // Daten über Chapter
    pChapter->chapterNo          = ndb_readbuf_ushort(buf, &count);
    strcpy(pChapter->chapterName,  ndb_readbuf_char_n(buf, &count, NDB_MAXLEN_CHAPNAME));
    pChapter->chapterName[NDB_MAXLEN_CHAPNAME-1] = '\0';
    pChapter->chapterTime        = ndb_readbuf_ulong (buf, &count);
    strcpy(pChapter->codepage,     ndb_readbuf_char_n(buf, &count, NDB_MAXLEN_CODEPAGE));
    pChapter->codepage[NDB_MAXLEN_CODEPAGE-1] = '\0';
    strcpy(pChapter->comment,      ndb_readbuf_char_n(buf, &count, NDB_MAXLEN_COMMENT));
    pChapter->comment[NDB_MAXLEN_COMMENT-1] = '\0';
    // Daten über Blockliste
    pChapter->blockDatLen        = ndb_readbuf_ulong8(buf, &count);
    pChapter->blockDatLenReal    = ndb_readbuf_ulong8(buf, &count);
    pChapter->blockDatSta        = ndb_readbuf_ulong (buf, &count);
    pChapter->blockDatNo         = ndb_readbuf_ulong (buf, &count);
    // Daten über Block-Header-Liste
    pChapter->blockHeaLen        = ndb_readbuf_ulong (buf, &count);
    pChapter->blockHeaSta        = ndb_readbuf_ulong (buf, &count);
    pChapter->blockHeaNo         = ndb_readbuf_ulong (buf, &count);
    // Daten über File-Entry-Liste
    pChapter->filesListLen       = ndb_readbuf_ulong (buf, &count);
    pChapter->filesListSta       = ndb_readbuf_ulong (buf, &count);
    pChapter->filesNo            = ndb_readbuf_ulong (buf, &count);
    pChapter->filesNoIdent       = ndb_readbuf_ulong (buf, &count);
    pChapter->allFileLenOri      = ndb_readbuf_ulong8(buf, &count);
//    pChapter->allFileLenOri      = ndb_readbuf_ulong (buf, &count);
    pChapter->noMaxDatFil        = ndb_readbuf_ushort(buf, &count);
    pChapter->noNewDatFil        = ndb_readbuf_ushort(buf, &count);
        pChapter->dataFilesHeaLen    = ndb_readbuf_ulong (buf, &count);
    pChapter->staDatFil          = ndb_readbuf_ulong (buf, &count);

    // calculate crc16 for buffer with header struct
    crc16 = ndb_crc16(0, &buf[count_aftercrc], count - count_aftercrc);
    // compare with saved crc16
    if (pChapter->crc16Hea != crc16)
    {
        fprintf(stderr, "Warning: crc 0x%04X of header of chapter %u doesn't result in the saved crc 0x%04X\n",
                        crc16, pChapter->chapterNo, pChapter->crc16Hea);
        fflush(stderr);
//        retval = NDB_MSG_CRCERROR;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_io_readchunkchapter - finished\n");
    fflush(stdout);

    return retval;
}

/* ======================================================================== */
/* functions for writing and reading the block data or header start chunk   */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_io_writechunkblock()

        used for
        - block data chunk
        - block header chunk
        - file entries chunk

        save the current archive file position to the block chunk structure (to allow
        later writing of corrected structure data easily);
        writes the block chunk structure to the archive file handle
    *************************************************************************************
*/

int ndb_io_writechunkblock(FILE *fArchive, struct ndb_s_c_block *pChunk)
{
    char buf[NDB_SAVESIZE_BLOCKCHUNK];
    int count;
    int retval;

    count = 0;
    count = ndb_write2buf_ulong  (buf, &count, pChunk->magic);
    count = ndb_write2buf_ulong  (buf, &count, pChunk->dataSize);
    count = ndb_write2buf_ulong  (buf, &count, pChunk->dataCount);
    count = ndb_write2buf_ushort (buf, &count, pChunk->flags1);
    count = ndb_write2buf_ushort (buf, &count, pChunk->flags2);

    /* remember current file position for ease refreshing afterwards */
    pChunk->ownFilePosition = ftell(fArchive);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_writechunkblock: writing at file position 0x%lX\n",
                        pChunk->ownFilePosition);
        fflush(stdout);
    }

    if ((ftell(fArchive) & 3) != 0)
    {
        fprintf(stdout, "Error ------------------------------------------------\nno quad file position\n");
        fflush(stdout);
    }

    /* now save data to archive file */
    retval = ndb_io_writedata(buf, count, fArchive);

    if (retval != count)
    {
        fprintf(stdout, "tried to write %d bytes but could only %d bytes\n",
                        count, retval);
        fflush(stdout);
        return NDB_MSG_WRITEERROR;
    }
    else
        return NDB_MSG_OK;
}

/*
    *************************************************************************************
        ndb_io_readchunkblock()

        used for
        - block data chunk
        - block header chunk
        - file entries chunk

        reads the data chunk structure from the archive file handle;
        check for the correct magic (NDB_MAGIC_BLOCKS [zipped block data chunk] or
        NDB_MAGIC_BLOCKHEADER [block header chunk] or NDB_MAGIC_FILEHEADER [file entries
        chunk]);
        currently: don't check, that the chunk version is understood from this NDB version
    *************************************************************************************
*/

int ndb_io_readchunkblock(FILE *fArchive, struct ndb_s_c_block *pChunk)
{
    char buf[NDB_SAVESIZE_BLOCKCHUNK];
    int count;
    int retval;

    /* remember current file position for ease refreshing afterwards */
    pChunk->ownFilePosition = ftell(fArchive);

    if ((ftell(fArchive) & 3) != 0)
    {
        fprintf(stdout, "Error ------------------------------------------------\nno quad file position\n");
        fflush(stdout);
    }

    retval = ndb_io_readdata(buf, NDB_SAVESIZE_BLOCKCHUNK, fArchive);
    if (retval != NDB_SAVESIZE_BLOCKCHUNK)
    {
        return NDB_MSG_READERROR;
    }

    count = 0;
    pChunk->magic = ndb_readbuf_ulong (buf, &count);
    if ((pChunk->magic != NDB_MAGIC_BLOCKS) && (pChunk->magic != NDB_MAGIC_BLOCKHEADER)
       && (pChunk->magic != NDB_MAGIC_FILEHEADER) && (pChunk->magic != NDB_MAGIC_DATAFILE))
    {
        fprintf(stdout, "bad archive - expected magic %X, %X, %X or %X but got %lX\n",
                NDB_MAGIC_BLOCKS, NDB_MAGIC_BLOCKHEADER, NDB_MAGIC_FILEHEADER, NDB_MAGIC_DATAFILE,
                pChunk->magic);
        fflush(stdout);
        return NDB_MSG_BADMAGIC;
    }
    pChunk->dataSize       = ndb_readbuf_ulong (buf, &count);
    pChunk->dataCount      = ndb_readbuf_ulong (buf, &count);
    pChunk->flags1         = ndb_readbuf_ushort(buf, &count);
    pChunk->flags2         = ndb_readbuf_ushort(buf, &count);

    return NDB_MSG_OK;
}


/* ======================================================================== */
/* functions for writing and reading the block headers                      */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_io_writeBlockHeader()

        save the current archive file position to the block header structure (to allow
        later writing of corrected structure data easily);
        writes the block header structure to the archive file handle
    *************************************************************************************
*/

int ndb_io_writeBlockHeader(FILE *fArchive, struct ndb_s_blockheader *pHeader)
{
    char buf[NDB_SAVESIZE_BLOCKHEADER];
    int count;
    int retval;
    int count_crcbefore = 0;
    int count_crcafter  = 0;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_writeBlockHeader - startup\n");
        fflush(stdout);
    }

    // set own position in archive file
    pHeader->staHea = ftell(fArchive);
    pHeader->crc16Hea = 0;

    count = 0;
    count_crcbefore = count;
    count = ndb_write2buf_ushort (buf, &count, pHeader->crc16Hea);
    count_crcafter = count;
    count = ndb_write2buf_ulong  (buf, &count, pHeader->blockNo);
    count = ndb_write2buf_ulong  (buf, &count, pHeader->fileNo);
    count = ndb_write2buf_ushort (buf, &count, pHeader->dataFileNo);
    count = ndb_write2buf_ulong  (buf, &count, pHeader->crc32);
    count = ndb_write2buf_ushort (buf, &count, pHeader->lenOri);
    count = ndb_write2buf_ushort (buf, &count, pHeader->lenZip);
//    count = ndb_write2buf_ulong  (buf, &count, pHeader->staHea);
    count = ndb_write2buf_ulong  (buf, &count, pHeader->staDat);
    count = ndb_write2buf_char   (buf, &count, pHeader->blockInfo);
    count = ndb_write2buf_char   (buf, &count, pHeader->blockType);

    // calculate crc16 for buffer with header struct
    pHeader->crc16Hea = ndb_crc16(0, &buf[count_crcafter], count - count_crcafter);
    // write crc16 into buffer
    ndb_write2buf_ushort(buf, &count_crcbefore, pHeader->crc16Hea);

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_writeBlockHeader: writing block header F:%lu/B:%lu/D:%u/CRC:%04X to file position 0x%lX\n",
                pHeader->fileNo, pHeader->blockNo, pHeader->dataFileNo, pHeader->crc16Hea, ftell(fArchive));
        fflush(stdout);
    }

    /* write data to archive file */
    retval = ndb_io_writedata(buf, count, fArchive);
    if (retval != count)
    {
        fprintf(stdout, "tried to write %d bytes but could only %d bytes\n",
                        count, retval);
        fflush(stdout);
        return NDB_MSG_WRITEERROR;
    }

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_writeBlockHeader - finished\n");
        fflush(stdout);
    }

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
    ndb_io_readBlockHeader()

    reads the block header structure to the archive file handle
    *************************************************************************************
*/

int ndb_io_readBlockHeader(FILE *fArchive, struct ndb_s_blockheader *pHeader)
{
    char buf[NDB_SAVESIZE_BLOCKHEADER];
    int count;
    int count_aftercrc;
    USHORT crc16 = 0;
    int retval = NDB_MSG_OK;
    ULONG ulSize = 0L;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_readBlockHeader - startup\n");
        fflush(stdout);
    }

    /* read data from archive file */
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_readBlockHeader: reading block header from file position 0x%lX\n", ftell(fArchive));
        fflush(stdout);
    }

    pHeader->staHea      = ftell(fArchive);
    ulSize = ndb_io_readdata(buf, NDB_SAVESIZE_BLOCKHEADER, fArchive);
    if (ulSize != NDB_SAVESIZE_BLOCKHEADER)
    {
        return NDB_MSG_READERROR;
    }

    count = 0;

    pHeader->crc16Hea    = ndb_readbuf_ushort(buf, &count);
    count_aftercrc = count;
    pHeader->blockNo     = ndb_readbuf_ulong(buf, &count);
    pHeader->fileNo      = ndb_readbuf_ulong(buf, &count);
    pHeader->dataFileNo  = ndb_readbuf_ushort(buf, &count);
    pHeader->crc32       = ndb_readbuf_ulong(buf, &count);
    pHeader->lenOri      = ndb_readbuf_ushort(buf, &count);
    pHeader->lenZip      = ndb_readbuf_ushort(buf, &count);
//    pHeader->staHea      = ndb_readbuf_ulong(buf, &count);
    pHeader->staDat      = ndb_readbuf_ulong(buf, &count);
    pHeader->blockInfo   = ndb_readbuf_char(buf, &count);
    pHeader->blockType   = ndb_readbuf_char(buf, &count);

    // calculate crc16 for buffer with header struct
    crc16 = ndb_crc16(0, &buf[count_aftercrc], count - count_aftercrc);
    // compare with saved crc16
    if (pHeader->crc16Hea != crc16)
    {
        fprintf(stderr, "Warning: crc 0x%04X of header of block F%lu/B%lu/D%u doesn't result in the saved crc 0x%04X\n",
                        crc16, pHeader->fileNo, pHeader->blockNo, pHeader->dataFileNo, pHeader->crc16Hea);
        fflush(stderr);
//        retval = NDB_MSG_CRCERROR;
    }

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_readBlockHeader - finished\n");
        fflush(stdout);
    }

    return retval;
}


/* ======================================================================== */
/* functions for writing and reading the file entries                       */
/* ======================================================================== */

// both functions are now in ndb_iofuncs_ext.c
// because ndb_io_readFileEntry() needs a ndb_calloc() for the filename


/* ======================================================================== */
/* write/read zipped blockdata to/from archive file                         */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_io_writeBlockZipDataToFile()

        wrapper for ndb_io_writedata()

        writes the zipped block data of a block header structure to the archive file handle
    *************************************************************************************
*/

int ndb_io_writeBlockZipDataToFile(struct ndb_s_blockheader *pCurrBlock, FILE *fDataFile)
{
    int retval;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_writeBlockZipDataToFile - startup\n");
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_writeBlockZipDataToFile: block F%lu/B%lu/D%u\n",
                pCurrBlock->fileNo, pCurrBlock->blockNo, pCurrBlock->dataFileNo);
        fprintf(stdout, "ndb_io_writeBlockZipDataToFile: writing zipped data (0x%X bytes) for CRC %8lX to file offset 0x%lX\n",
                        pCurrBlock->lenZip, pCurrBlock->crc32, ftell(fDataFile));
        fflush(stdout);
    }
    // remember file position in block header data
    pCurrBlock->staDat = ftell(fDataFile);
    // write zipped data block to file
    retval = ndb_io_writedata(pCurrBlock->BlockDataZip, pCurrBlock->lenZip, fDataFile);
    if (retval != pCurrBlock->lenZip)
    {
        return NDB_MSG_WRITEERROR;
    }

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_writeBlockZipDataToFile - finished\n");
        fflush(stdout);
    }

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
        ndb_io_readBlockZipDataFromFile()

        wrapper for ndb_io_readdata()

        reads the zipped block data of a block header structure from the archive
        data file handle
    *************************************************************************************
*/

int ndb_io_readBlockZipDataFromFile(struct ndb_s_blockheader *pCurrBlock)
{
    int retval;
    FILE *dataFileHandle = NULL;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_readBlockZipDataFromFile - startup\n");
        fflush(stdout);
    }

    dataFileHandle = ndb_df_getReadHandle(pCurrBlock->dataFileNo);
    fseek(dataFileHandle, pCurrBlock->staDat, SEEK_SET);

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_readBlockZipDataFromFile: block F%lu/B%lu/D%u\n",
                pCurrBlock->fileNo, pCurrBlock->blockNo, pCurrBlock->dataFileNo);
        fprintf(stdout, "ndb_io_readBlockZipDataFromFile: reading zipped data (0x%X bytes) from file offset 0x%lX\n",
                        pCurrBlock->lenZip, ftell(dataFileHandle));
        fflush(stdout);
    }
    // write zipped data block to file
    retval = ndb_io_readdata(pCurrBlock->BlockDataZip, pCurrBlock->lenZip, dataFileHandle);
    if (retval != pCurrBlock->lenZip)
    {
        return NDB_MSG_READERROR;
    }

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_readBlockZipDataFromFile - finished\n");
        fflush(stdout);
    }

    return NDB_MSG_OK;
}


/* ======================================================================== */
/* write/read original blockdata to/from original file                      */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_io_writeBlockOriDataToFile()

        wrapper for ndb_io_writedata()

        writes the unzipped block data of a block header structure to the file handle
    *************************************************************************************
*/

int ndb_io_writeBlockOriDataToFile(struct ndb_s_blockheader *pCurrBlock, FILE *fFile)
{
    int retval;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_writeBlockOriDataToFile - startup\n");
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_writeBlockOriDataToFile: writing original data (0x%X bytes) for CRC %8lX to file offset 0x%lX\n",
                        pCurrBlock->lenOri, pCurrBlock->crc32, ftell(fFile));
        fflush(stdout);
    }
    // write zipped data block to file
    retval = ndb_io_writedata(pCurrBlock->BlockDataOri, pCurrBlock->lenOri, fFile);
    if (retval != pCurrBlock->lenOri)
    {
        return NDB_MSG_WRITEERROR;
    }

    if (ndb_getDebugLevel() >= 6)
    {    
        fprintf(stdout, "ndb_io_writeBlockOriDataToFile - finished\n");
        fflush(stdout);
    }

    return NDB_MSG_OK;
}

/*
    *************************************************************************************
        ndb_io_readBlockOriDataFromFile()

        wrapper for ndb_io_readdata()

        reads the unzipped block data of a block header structure from the file handle
    *************************************************************************************
*/

int ndb_io_readBlockOriDataFromFile(struct ndb_s_blockheader *pCurrBlock, FILE *fFile)
{
    int retval;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_readBlockOriDataFromFile - startup\n");
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_io_readBlockOriDataFromFile: reading original data (0x%X bytes) from file offset 0x%lX\n",
                        pCurrBlock->lenOri, ftell(fFile));
        fflush(stdout);
    }
    // write zipped data block to file
    retval = ndb_io_readdata(pCurrBlock->BlockDataOri, pCurrBlock->lenOri, fFile);
    if (retval != pCurrBlock->lenOri)
    {
        return NDB_MSG_READERROR;
    }

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_readBlockOriDataFromFile - finished\n");
        fflush(stdout);
    }
    fflush(stdout);

    return NDB_MSG_OK;
}


/* ======================================================================== */
/* general functions                                                        */
/* ======================================================================== */

/*
    *************************************************************************************
        ndb_io_readdata()

        reads a number of bytes from the file handle into a buffer
    *************************************************************************************
*/

int ndb_io_readdata(char *p, int size, FILE *f)
{
    int bytesread;

    if (ndb_getDebugLevel() >= 6)
    {
        fflush(stdout);
        fprintf(stdout, "ndb_io_readdata - startup\n");
    }

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "%6lx + %4x = ", ftell(f), size);
        fflush(stdout);
    }
    bytesread = fread(p, (size_t) 1, (size_t) size, f);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "%6lX: read 0x%4X from buffer %lX\n", ftell(f), bytesread, (ULONG) p);
        fflush(stdout);
    }
    if (ferror(f))
    {
        fprintf(stdout, "Error reading %d bytes from file\n", size);
        perror("ndb_io_readdata");
        fflush(stderr);
        clearerr(f);
    }
    if (bytesread != size)
    {
        if (feof(f))
        {
            fprintf(stdout, "Warning: read %d bytes because of EOF, expected %d bytes\n", bytesread, size);
        }
        else
        {
            fprintf(stdout, "Warning: read %d bytes, expected %d bytes\n", bytesread, size);
        }
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_readdata - finished\n");
        fflush(stdout);
    }
    fflush(stdout);

    return bytesread;
}

/*
    *************************************************************************************
        ndb_io_writedata()

        writes a number of bytes from a buffer to the file handle
    *************************************************************************************
*/

int ndb_io_writedata(char *p, int size, FILE *f)
{
    int byteswritten;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_writedata - startup\n");
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "%6lx + %4x = ", ftell(f), size);
        fflush(stdout);
    }
    byteswritten = fwrite(p, (size_t) 1, (size_t) size, f);
    fflush(f);
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "%6lX: written 0x%4X from buffer %lX\n", ftell(f), byteswritten, (ULONG) p);
        fflush(stdout);
    }

    if (ferror(f))
    {
        fprintf(stdout, "Error writing %d bytes to file\n", size);
        perror("ndb_io_writedata");
        fflush(stderr);
        clearerr(f);
    }
    if (byteswritten != size)
    {
        fprintf(stdout, "Error: written %d bytes, expected %d bytes\n", byteswritten, size);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_writedata - finished\n");
        fflush(stdout);
    }

    return byteswritten;
}

/*
    *************************************************************************************
        ndb_io_check_exist_file()

        checks using stat() whether the file <filename> exists on harddrive;
        if a stat structure is given the stat() result is given back
    *************************************************************************************
*/

int ndb_io_check_exist_file(const char *filename, struct stat *pStat)
{
    struct stat statFile;
    struct stat *pStat2;
        int retval;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_check_exist_file - startup\n");
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 6) fprintf(stdout, "ndb_io_check_exist_file: does file exist? '%s'\n",
                 filename);
    fflush(stdout);

    if (pStat != NULL)
        pStat2 = pStat;
    else
        pStat2 = &statFile;

    if (LSSTAT(filename, pStat2) == -1)
        retval = NDB_MSG_NOTFOUND;
    else
        retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_check_exist_file: file %s\n",
                  (retval == NDB_MSG_OK) ? "exists" : "does not exist");
        fflush(stdout);
    }
    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_io_check_exist_file - finished\n");
        fflush(stdout);
    }

    return retval;
}

/*
    *************************************************************************************
        ndb_io_doQuadAlign()

        check the current position  in the file and add 0..3 bytes to come to a quad (4)
        aligned file position (all chunks should start at a quad aligned file position)

        Remark: I don't know whether this makes sense; the hope was to allow quicker
        searches for magics (= chunk starts) if the archive file is corrupted or shortened
    *************************************************************************************
*/

void ndb_io_doQuadAlign(FILE *f)
{
    ULONG filepos = ftell(f);
    if ((filepos & 3) > 0)
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_io_doQuadAlign: changing file position from 0x%lX ", ftell(f));
            fflush(stdout);
        }
        filepos = (filepos & 0xfffffffc) + 4;
        fseek(f, filepos, SEEK_SET);
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "to 0x%lX ", ftell(f));
            fflush(stdout);
        }
    }
    else
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_io_doQuadAlign: file position 0x%lX already quad aligned\n", ftell(f));
            fflush(stdout);
        }
    }
    return;
}
