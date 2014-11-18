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
#include <unistd.h>


#include "ndb.h"
#include "ndb_prototypes.h"



/* ======================================================================== */
/* functions for working with data files                                    */
/* ======================================================================== */

struct ndb_s_gl_datafiles
{
    USHORT currFilCnt;      // current number of datafiles
    USHORT currFilOpn;      // current number of opened datafiles
    USHORT maxFilOpn;       // max allowed number of opened datafiles
    USHORT currChap;        // current chapter number
    ULONG  maxDataSize;     // maximal allowed size of datafile
    USHORT maxFilCnt;       // maximal number of datafiles, to increase use realloc()
    PDATAFILEHEADER *ppDataFiles;  // array of datafiles
    char mainpath[NDB_MAXLEN_FILENAME];   // file name of main archive file
    USHORT lenmainfilename; // length of file name of main archive file
    char   mainfilename[NDB_MAXLEN_FILENAME];   // file name of main archive file
    ULONG identCRC;         // identifikation of NDB main file
};

static struct ndb_s_gl_datafiles gl_df =
{
    0,
    0,
    0,
    0,
    0L,
    0,
    NULL,
    "\0",
    0,
    "\0",
    0L
};

static USHORT ndb_df_handles_max = NDB_DATAFILE_HANDLESMAX;

void ndb_df_printGlData();
char *ndb_df_status2print(int);


USHORT ndb_df_getMaxFileHandles()
{
    return ndb_df_handles_max;
}

void ndb_df_setMaxFileHandles(USHORT count)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_setMaxFileHandles - setting max. number of file handles to %d\n", count);
    fflush(stdout);
    ndb_df_handles_max = count;
}


/*
    *************************************************************************************
    ndb_df_closeOldestHandle()

    Initialisierung der statischen Variablen, Arrays und Strukturen,
    Erzeugen des ersten Elements (Steuerdatei) für die Filenummer 0.
    *************************************************************************************
*/

int ndb_df_closeOldestHandle(USHORT fileToOpen)
{
    PDATAFILEHEADER pCurrDFHeader = NULL;
    int i;
    int    noOldest     = 0;
    ULONG  ulOldestAge  = 0;
    ULONG  ulOldestHits = 0;
    USHORT noOpenFiles  = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_closeOldestHandle - startup\n");
    fflush(stdout);

    // skip NDB main file, start with first data file
    for (i = 1; i < gl_df.currFilCnt; i++)
    {
        pCurrDFHeader = gl_df.ppDataFiles[i];
        // count number of currently open files
        if ((pCurrDFHeader->status == NDB_DATAFILE_OPENREAD) || (pCurrDFHeader->status == NDB_DATAFILE_OPENREADWRITE))
            noOpenFiles++;
        // don't look for the file to opeen, only look for files with NDB_DATAFILE_OPENREAD
        if ((fileToOpen != i) && (pCurrDFHeader->status == NDB_DATAFILE_OPENREAD)
            && (pCurrDFHeader->age > ulOldestAge))
        {
            noOldest = i;
            ulOldestAge = pCurrDFHeader->age;
            ulOldestHits = pCurrDFHeader->hits;
        }
        if (pCurrDFHeader->status != NDB_DATAFILE_CLOSED)
        {
            if (ndb_getDebugLevel() >= 6)
            {
                fprintf(stdout, "DataFile %02d ------------------------------------------------\n", i);
                fprintf(stdout, "File Name:       '%s'\n",    pCurrDFHeader->filename);
                fprintf(stdout, "File Size:       %lu\n",     pCurrDFHeader->curDataSize);
                fprintf(stdout, "File Status:     '%s'\n",    ndb_df_status2print(pCurrDFHeader->status));
                fprintf(stdout, "File Handle:     0x%08lX\n", (ULONG) pCurrDFHeader->fileHandle);
                fprintf(stdout, "File Age:        %lu\n",     pCurrDFHeader->age);
                fprintf(stdout, "File Hits:       %lu\n",     pCurrDFHeader->hits);
                fflush(stdout);
            }
        }
    }
    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "Result =======================================================\n");
        fprintf(stdout, "Currently Opened:  %u of %u (max.)\n", noOpenFiles, ndb_df_getMaxFileHandles());
        fprintf(stdout, "Oldest File:       %d\n", noOldest);
        fprintf(stdout, "File Name:         '%s'\n", gl_df.ppDataFiles[noOldest]->filename);
        fprintf(stdout, "Result =======================================================\n");
        fflush(stdout);
    }

    if (noOpenFiles >= ndb_df_getMaxFileHandles())
    {
        // close oldest file handle
        if ((noOldest > 0) && (noOldest < gl_df.currFilCnt) && (noOldest != fileToOpen))
        {
            if (ndb_getDebugLevel() >= 6)
            {
                fprintf(stdout, "now closing handle for datafile %d\n", noOldest);
                fflush(stdout);
            }
            ndb_df_closeDataFile(noOldest);
        }
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_closeOldestHandle - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


void ndb_df_makeDFOlder()
{
    PDATAFILEHEADER pCurrDFHeader = NULL;
    int i;

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_df_makeDFOlder - startup\n");
    fflush(stdout);

    for (i = 1; i < gl_df.currFilCnt; i++)
    {
        // make it older by one tick
        pCurrDFHeader = gl_df.ppDataFiles[i];
        if (pCurrDFHeader->age < NDB_DATAFILE_AGEMAX)
        {
            pCurrDFHeader->age++;
        }
    }

    if (ndb_getDebugLevel() >= 6)
        fprintf(stdout, "ndb_df_makeDFOlder - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_df_init()

    Initialisierung der statischen Variablen, Arrays und Strukturen,
    Erzeugen des ersten Elements (Steuerdatei) für die Filenummer 0.
    *************************************************************************************
*/

int ndb_df_init(PARCHIVE pArchive, FILE *fArchive, char *ndbmainfile)
{
    char path[NDB_MAXLEN_FILENAME];
    char mainfile[NDB_MAXLEN_FILENAME];
    PDATAFILEHEADER pMainDataFile = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_init - startup\n");
    fflush(stdout);

    gl_df.currChap = -1;
    gl_df.maxDataSize = pArchive->dataFilesSize;
    gl_df.identCRC = pArchive->identCRC;
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_df_init: max data file size  is %lu\n", gl_df.maxDataSize);
        fprintf(stdout, "ndb_df_init: NDB main file ident is %lX\n", gl_df.identCRC);
        fflush(stdout);
    }

    ndb_osdep_split(ndbmainfile, path, mainfile);
    strcpy (gl_df.mainpath, path);
    gl_df.lenmainfilename = strlen(mainfile);
    strcpy(gl_df.mainfilename, mainfile);
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_df_init: split '%s' into '%s' and '%s'\n", ndbmainfile, gl_df.mainpath, gl_df.mainfilename);
    fflush(stdout);

    ndb_df_setMaxFileHandles(NDB_DATAFILE_HANDLESMAX);
    gl_df.maxFilCnt = 10;
    gl_df.ppDataFiles = ndb_calloc(sizeof(PDATAFILEHEADER), gl_df.maxFilCnt, "ndb_df_init: allocate array");
    if (gl_df.ppDataFiles == NULL)
    {
        fprintf(stderr, "ndb_df_init: couldn't allocate data file header array\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }

    // create ndb main file as first list element
    gl_df.currFilCnt = 0;
    pMainDataFile = ndb_df_createNewDataFile(0, path, mainfile);
    pMainDataFile->fileHandle = fArchive;
    pMainDataFile->status     = NDB_DATAFILE_OPENREADWRITE;
    ndb_df_addDataFile(pMainDataFile);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_init - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
    ndb_df_createDFName()

    ermittelt den Filename des Datafiles
    *************************************************************************************
*/

char *ndb_df_createDFName(PDATAFILEHEADER pDFFile)
{
    static char pathfile[NDB_MAXLEN_FILENAME];

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_createDFName - startup\n");
    fflush(stdout);

    // add path if not empty or '.'
    if ((gl_df.mainpath[0] != '\0') && (strcmp(gl_df.mainpath, ".") != 0))
    {
        strcpy(pathfile, gl_df.mainpath);
        // do we need a directory seperator?
        if ((gl_df.mainpath[strlen(pathfile) - 1] != '/') && (gl_df.mainpath[strlen(pathfile) - 1] != ':'))
            strcat(pathfile, "/");
        snprintf(pathfile, NDB_MAXLEN_FILENAME - 1, "%s%s%c", pathfile, pDFFile->filename, '\0');
      if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_df_createDFName: data file '%s' from '%s' and '%s'\n", pathfile, gl_df.mainpath, pDFFile->filename);
        fflush(stdout);
    }
    else
        strcpy(pathfile, pDFFile->filename);

    return pathfile;
}


/*
    *************************************************************************************
    ndb_df_setChapNo()

    setzt die aktuelle Chapternummer
    *************************************************************************************
*/

void ndb_df_setChapNo(int iChapNo)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_setChapNo - startup\n");
    fflush(stdout);

    gl_df.currChap = iChapNo;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_setChapNo - finished\n");
    fflush(stdout);
}

/*
    *************************************************************************************
    ndb_df_addDataFile()

    Erweitern der Verwaltung um das übergebene DataFile;
    falls eine DataFile-Struktur mit gleicher datFil_No existiert,
    wird diese ersetzt
    *************************************************************************************
*/

int ndb_df_addDataFile(PDATAFILEHEADER pDataFile)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_addDataFile - startup\n");
    fflush(stdout);

    // check preconditions
    if (pDataFile == NULL)
    {
        fprintf(stdout, "ndb_df_addDataFile: cannot work with a NULL data file header\n");
        return NDB_MSG_NULLERROR;
    }

    // do we have to replace an existing entry?
    if (pDataFile->datFil_No < gl_df.currFilCnt)
    {
        if (ndb_getDebugLevel() >= 7)
            ndb_df_printDFHeadder(gl_df.ppDataFiles[pDataFile->datFil_No], "ndb_df_addDataFile: replacing old data file ...");
        if (ndb_getDebugLevel() >= 7)
            ndb_df_printDFHeadder(pDataFile, "ndb_df_checkDataFile: ... with new data file");
        gl_df.ppDataFiles[pDataFile->datFil_No] = pDataFile;
    }
    else
    {
        // first check if maximum size of datafile array is reached
        if (ndb_getDebugLevel() >= 8)
            fprintf(stdout, "ndb_df_addDataFile: currFilCnt %d, maxFilCnt %d\n", gl_df.currFilCnt, gl_df.maxFilCnt);
        fflush(stdout);
        if (gl_df.currFilCnt == gl_df.maxFilCnt)
        {
            if (ndb_getDebugLevel() >= 8)
            {
                fprintf(stdout, "ndb_df_addDataFile: inc max data file number by 10\n");
                fflush(stdout);
            }
            // increase array by 10
            gl_df.ppDataFiles = ndb_realloc(gl_df.ppDataFiles, (gl_df.maxFilCnt + 10) * sizeof(PDATAFILEHEADER), "ndb_df_init: allocate array");
            if (gl_df.ppDataFiles == NULL)
            {
                fprintf(stderr, "ndb_df_init: couldn't allocate data file header array\n");
                fflush(stderr);
                exit(NDB_MSG_NOMEMORY);
            }
            gl_df.maxFilCnt += 10;
        }

        if (ndb_getDebugLevel() >= 8)
            fprintf(stdout, "ndb_df_addDataFile: add data file header 0x%lX as %d. header\n", (ULONG) pDataFile, gl_df.currFilCnt);
        fflush(stdout);
        gl_df.ppDataFiles[gl_df.currFilCnt] = pDataFile;
        gl_df.currFilCnt++;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_addDataFile - finished\n");
    fflush(stdout);

    return NDB_MSG_OK;
}


/*
    *************************************************************************************
    ndb_df_checkDataFile()

    Prüfen, ob Datei existiert, ob Dateigröße stimmt.
    Ggfs Magic bzw Headerstruktur am Anfang prüfen?
    *************************************************************************************
*/

int ndb_df_checkDataFile(int fileNo)
{
    struct ndb_s_c_block dataFilesChunk;
    int retval = NDB_MSG_OK;
    struct stat myStat;
    struct ndb_s_datafileheader currDataFile;
    PDATAFILEHEADER pNdbDataFile;
    char pathfile[NDB_MAXLEN_FILENAME];
    FILE *filehandle = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_checkDataFile - startup\n");
    fflush(stdout);

    // check preconditions
    if ((fileNo < 1) || (fileNo >= ndb_df_getFileCount()))
    {
        fprintf(stdout, "ndb_df_checkDataFile: data file number %d is beyond ranges (1..%d)\n",
                        fileNo, ndb_df_getFileCount() - 1);
        fflush(stdout);
        return NDB_MSG_NOTOK;
    }

    // add path if not empty or '.'
    if ((gl_df.mainpath[0] != '\0') && (strcmp(gl_df.mainpath, ".") != 0))
    {
        strcpy(pathfile, gl_df.mainpath);
        // do we need a directory seperator?
        if ((gl_df.mainpath[strlen(pathfile) - 1] != '/') && (gl_df.mainpath[strlen(pathfile) - 1] != ':'))
            strcat(pathfile, "/");
        snprintf(pathfile, NDB_MAXLEN_FILENAME - 1, "%s%s%c", pathfile, ndb_df_getCurrDataFilename(fileNo), '\0');
    }
    else
        strcpy(pathfile, ndb_df_getCurrDataFilename(fileNo));

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_df_checkDataFile: data file '%s' from '%s' and '%s'\n", pathfile, gl_df.mainpath, ndb_df_getCurrDataFilename(fileNo));
    fflush(stdout);


    pNdbDataFile = gl_df.ppDataFiles[fileNo];

    // clear structure content
    memset(&currDataFile, 0, sizeof(struct ndb_s_datafileheader));

    // does file exist?
    retval = ndb_io_check_exist_file(pathfile, &myStat);
    if (retval == NDB_MSG_OK)
    {
        // size < max allowed size?
        if ((gl_df.maxDataSize > 0L) && (myStat.st_size > gl_df.maxDataSize))
        {
            fprintf(stdout, "DF Check Warning: data file '%s' exceeds maximum size of %lu bytes\n",
                            ndb_df_getCurrDataFilename(fileNo), gl_df.maxDataSize);
            fflush(stdout);
        }
        // starts with chunk with correct magic?
        filehandle = ndb_df_getReadHandle(fileNo);
        retval = ndb_io_readchunkblock(filehandle, &dataFilesChunk);
        if ((retval == NDB_MSG_OK) && (dataFilesChunk.magic == NDB_MAGIC_DATAFILE))
        {
            retval = ndb_io_readDataFile(ndb_df_getReadHandle(fileNo), &currDataFile, 0);
            // close everything again
            fclose(filehandle);
            gl_df.ppDataFiles[fileNo]->status = NDB_DATAFILE_CLOSED;
            gl_df.ppDataFiles[fileNo]->fileHandle = NULL;
            if (ndb_getDebugLevel() >= 7)
                ndb_df_printDFHeadder(&currDataFile, "ndb_df_checkDataFile");
            // add own filename
            strcpy (currDataFile.filename, ndb_df_getCurrDataFilename(fileNo));
            currDataFile.lenfilename = strlen(currDataFile.filename);
            // first check: is data file identCRC equal to current NDB main identCRC?
            if (currDataFile.identCRC != gl_df.identCRC)
            {
                fprintf(stderr, "DF Check Error: NDB main file ident '%lX', but data file has ident '%lX'\n",
                        gl_df.identCRC, currDataFile.identCRC);
                fprintf(stderr, "wrong data file '%s' for NDB main file '%s'\n",
                        currDataFile.filename, gl_df.mainfilename);
                fflush(stderr);
                // identCRC check enabled since 0.5.0
                exit (NDB_MSG_IDENTERROR);
            }
            // second check: is saved data file number equal to expected file number?
            if (currDataFile.datFil_No != pNdbDataFile->datFil_No)
            {
                fprintf(stdout, "DF Check Warning: data file %2d: expected data file number %d, but data file says %d\n",
                        fileNo, pNdbDataFile->datFil_No, currDataFile.datFil_No);
                fflush(stdout);
            }
            // third check: is saved NDB main filename equal to current NDB main name?
            if (strcmp(currDataFile.mainfilename, gl_df.mainfilename) != 0)
            {
                fprintf(stdout, "DF Check Warning: data file %2d: expected NDB main file name '%s', but data file says '%s'\n",
                        fileNo, gl_df.mainfilename, currDataFile.mainfilename);
                fflush(stdout);
            }
            //  fourth: is saved file size (saved in NDB archive file!!) equal to real file size?
            if (myStat.st_size != pNdbDataFile->curDataSize)
            {
/*
                fprintf(stdout, "DF Check Warning: data file %2d: expected file size %lu, real file size %lu\n",
                        fileNo, pNdbDataFile->curDataSize, myStat.st_size);
                fflush(stdout);
                // truncate data file to saved size?
                if (myStat.st_size > pNdbDataFile->curDataSize)
                {

                    fprintf(stdout, "DF Warning: The data file '%s' exceeds its expected size.\n", pathfile);
                    fprintf(stdout, "It will be truncated now from %lub to the saved %lub.\n",
                                            myStat.st_size, pNdbDataFile->curDataSize);
                    fflush(stdout);
                    ndb_osdep_truncate(pathfile, pNdbDataFile->curDataSize);
                }
*/
            }
        }
        else
        {
            fprintf(stderr, "DF Check Error: '%s' is no NDB data file\n", pathfile);
            fflush(stderr);
            fclose (filehandle);
            exit(NDB_MSG_NOTOK);
        }
    }
    else
    {
        fprintf(stderr, "DF Check Error: NDB data file '%s' not found!\n", pathfile);
        fflush(stderr);
        fclose (filehandle);
        exit(NDB_MSG_NOTOK);
    }


    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_checkDataFile - finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
    ndb_df_checkAllDataFile()

    check all data files for existence, correct magic and so on
    *************************************************************************************
*/

int ndb_df_checkAllDataFile()
{
    int retval = NDB_MSG_OK;
    int i = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_checkAllDataFile - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_checkAllDataFile: checking for %d data files\n", gl_df.currFilCnt);
    fflush(stdout);

    for (i = 1; i < gl_df.currFilCnt; i++)
    {
        retval = ndb_df_checkDataFile(i);
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_checkAllDataFile - finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
    ndb_df_createNewDataFile()

    Neue, leere DataFile-Structur erzeugen
    *************************************************************************************
*/

PDATAFILEHEADER ndb_df_createNewDataFile(int fileNo, char *path, char *filename)
{
    char newFilename[NDB_MAXLEN_FILENAME];
    PDATAFILEHEADER pCurrDFHeader = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_createNewDataFile - startup\n");
    fflush(stdout);

    pCurrDFHeader = (PDATAFILEHEADER) ndb_calloc(sizeof(struct ndb_s_datafileheader), 1, "ndb_df_createNewDataFile: new pCurrDFHeader");
    if (pCurrDFHeader == NULL)
    {
        fprintf(stderr, "ndb_df_createNewDataFile: couldn't allocate data file header\n");
        fflush(stderr);
        exit(NDB_MSG_NOMEMORY);
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_createNewDataFile: create new data file header %d with name '%s'\n",
                        fileNo, filename);
    fflush(stdout);

    pCurrDFHeader->datFil_No = fileNo;
    pCurrDFHeader->firstChap = 0;
    pCurrDFHeader->lastChap = 0;
    pCurrDFHeader->curDataSize = 0;
    pCurrDFHeader->maxDataSize = gl_df.maxDataSize;
    pCurrDFHeader->fileHandle = NULL;
    pCurrDFHeader->ownFilePositionDF   = 0L;
    pCurrDFHeader->ownFilePositionMain = 0L;
    pCurrDFHeader->identCRC = gl_df.identCRC;
    pCurrDFHeader->flags1   = 0;
    pCurrDFHeader->flags2   = 0;
    pCurrDFHeader->age      = NDB_DATAFILE_AGEMAX;
    pCurrDFHeader->hits     = 0;
    // filename of main NDB file
    pCurrDFHeader->lenmainfilename = gl_df.lenmainfilename;
    strcpy(pCurrDFHeader->mainfilename, gl_df.mainfilename);
    // filename of current data file
    if (fileNo == 0)
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_df_createNewDataFile: create first data file header with name '%s'\n",
                            gl_df.mainfilename);
            fflush(stdout);
        }
        // fileno 0 is NDB main file
        pCurrDFHeader->lenfilename = pCurrDFHeader->lenmainfilename;
        strcpy(pCurrDFHeader->filename, pCurrDFHeader->mainfilename);
    }
    else
    {
        if (filename != NULL)
        {
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "ndb_df_createNewDataFile: create new data file header %d using name '%s'\n",
                                fileNo, filename);
                fflush(stdout);
            }
            // filename for new data file is given by argument
            pCurrDFHeader->lenfilename = strlen(filename);
            strcpy(pCurrDFHeader->filename, filename);
        }
        else
        {
            // no filename given -> create new data filename
            snprintf (newFilename, NDB_MAXLEN_FILENAME - 1, "%s_%04d%c", gl_df.mainfilename, fileNo, '\0');
            pCurrDFHeader->lenfilename = strlen(newFilename);
            strcpy(pCurrDFHeader->filename, newFilename);
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "ndb_df_createNewDataFile: create new data file header %d using name '%s'\n",
                                fileNo, newFilename);
                fflush(stdout);
            }
        }
    }
    // fields only in memory
    pCurrDFHeader->nxtFileData = NULL;
    pCurrDFHeader->status = NDB_DATAFILE_NEW;

    if (ndb_getDebugLevel() >= 7)
        ndb_df_printDFHeadder(pCurrDFHeader, "ndb_df_createNewDataFile");

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_createNewDataFile - finished\n");
    fflush(stdout);

    return pCurrDFHeader;
}

/*
    *************************************************************************************
    ndb_df_writeNewDataFile()

    Datei wirklich auf Platte erzeugen
    *************************************************************************************
*/

int ndb_df_writeNewDataFile(int fileNo, PDATAFILEHEADER pDataFile, FILE *fileNew)
{
    struct ndb_s_c_block dataFilesChunk;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_writeNewDataFile - startup\n");
    fflush(stdout);

    ndb_block_datachunk_init(&dataFilesChunk, NDB_MAGIC_DATAFILE, 0, 0);

    // position to start
    fseek(fileNew, 0L, SEEK_SET);
    // write data files header chunk to file
    retval = ndb_io_writechunkblock(fileNew, &dataFilesChunk);
    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "Error writing data files chunk for data file '%s'\n",
                pDataFile->filename);
        exit(retval);
    }
    retval = ndb_io_writeDataFile(fileNew, pDataFile, 0);
    if (retval != NDB_MSG_OK)
    {
        fprintf(stderr, "Error writing data files header for data file '%s'\n",
                pDataFile->filename);
        exit(retval);
    }

    ndb_df_setFileSize(fileNo, ftell(fileNew));
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_df_writeNewDataFile: set filesize of data file %d to %lu\n", fileNo, ftell(fileNew));
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_writeNewDataFile - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
    ndb_df_getReadHandle()

    Prüfen, ob fileNo im Rahmen, DataFile-retval prüfen:
    - wenn geschlossen, zum Lesen öffnen
    - wenn zum Lesen oder Lesen/Schreiben geöffnet, Filehandle
      zurückgeben
    *************************************************************************************
*/

FILE *ndb_df_getReadHandle(int fileNo)
{
    FILE *fileRead = NULL;
    char pathfile[NDB_MAXLEN_FILENAME];
    PDATAFILEHEADER pCurrDataFile;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getReadHandle - startup\n");
    fflush(stdout);

    // check preconditions
    if ((fileNo < 0) || (fileNo >= ndb_df_getFileCount()))
    {
        fprintf(stdout, "ndb_df_getReadHandle: data file number %d is beyond ranges (0..%d)\n",
                        fileNo, ndb_df_getFileCount() - 1);
        fflush(stdout);
        return NULL;
    }

    // make all data files one tick older
    ndb_df_makeDFOlder();
    // make current datafile young again
    pCurrDataFile = gl_df.ppDataFiles[fileNo];
    pCurrDataFile->age = 0;
    // increment hit rate
    if (pCurrDataFile->hits < NDB_DATAFILE_HITSMAX)
        pCurrDataFile->hits++;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getReadHandle: retval of data file header %d is %s\n",
                        fileNo, ndb_df_status2print(pCurrDataFile->status));
    fflush(stdout);

    if ((pCurrDataFile->status == NDB_DATAFILE_OPENREAD)
        || (pCurrDataFile->status == NDB_DATAFILE_OPENREADWRITE))
    {
        fileRead = pCurrDataFile->fileHandle;
    }

    if (fileRead == NULL)
    {
        strcpy (pathfile, ndb_df_createDFName(pCurrDataFile));
        // check for too many open files
        if (gl_df.currFilCnt > ndb_df_getMaxFileHandles())
        {
            // check all data files and close the oldest one
            ndb_df_closeOldestHandle(fileNo);
        }
        // open file
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_df_getReadHandle: opening file %d for rb\n", fileNo);
            fflush(stdout);
        }
        fileRead = fopen(pathfile,"rb");
        if (fileRead == NULL)
        {
            fprintf(stderr, "ndb_df_getReadHandle: could not open data file '%s' for read\n", pCurrDataFile->filename);
            perror("Reason");
            fflush(stderr);
            exit (NDB_MSG_NOFILEOPEN);
        }
        pCurrDataFile->fileHandle = fileRead;
        pCurrDataFile->status = NDB_DATAFILE_OPENREAD;
        gl_df.currFilOpn++;
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getReadHandle - finished\n");
    fflush(stdout);

    return fileRead;
}

/*
    *************************************************************************************
    ndb_df_getReadWriteHandle()

    Prüfen, ob fileNo im Rahmen, DataFile-retval prüfen:
    - wenn geschlossen, zum Lesen/Schreiben öffnen
    - wenn zum Lesen geöffnet, Datei schliessen und zum Lesen/Schreiben öffnen
    *************************************************************************************
*/

FILE *ndb_df_getReadWriteHandle(int fileNo)
{
    FILE *fileReadWrite = NULL;
    char pathfile[NDB_MAXLEN_FILENAME];
    PDATAFILEHEADER pCurrDataFile;
    ULONG ulFilePos = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getReadWriteHandle - startup\n");
    fflush(stdout);

    // check preconditions
    if ((fileNo < 0) || (fileNo >= ndb_df_getFileCount()))
    {
        fprintf(stdout, "ndb_df_getReadWriteHandle: data file number %d is beyond ranges (0..%d)\n",
                        fileNo, ndb_df_getFileCount() - 1);
        fflush(stdout);
        return NULL;
    }

    pCurrDataFile = gl_df.ppDataFiles[fileNo];

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_df_getReadWriteHandle: retval of filehandle %d is %s\n",
                        fileNo, ndb_df_status2print(pCurrDataFile->status));
        fflush(stdout);
    }

    if (pCurrDataFile->status == NDB_DATAFILE_OPENREADWRITE)
        fileReadWrite = pCurrDataFile->fileHandle;
    else if (pCurrDataFile->status == NDB_DATAFILE_OPENREAD)
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_df_getReadWriteHandle: closing file %d (only read mode)\n", fileNo);
            fflush(stdout);
        }
        fclose(pCurrDataFile->fileHandle);
        fileReadWrite = NULL;
    }

    if (fileReadWrite == NULL)
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_df_getReadWriteHandle: opening file %d (read/write mode)\n", fileNo);
            fflush(stdout);
        }
        strcpy (pathfile, ndb_df_createDFName(pCurrDataFile));
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_df_getReadWriteHandle: status of '%s' is %s\n", pathfile, ndb_df_status2print(pCurrDataFile->status));
            fflush(stdout);
        }

        // check for too many open files
        pCurrDataFile->age = 0;
        pCurrDataFile->hits++;
        if (gl_df.currFilCnt > ndb_df_getMaxFileHandles())
        {
            ndb_df_closeOldestHandle(fileNo);
        }

        if (pCurrDataFile->status == NDB_DATAFILE_NEW)
        {
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "ndb_df_getReadWriteHandle: trying to open '%s' for 'w+b'\n", pathfile);
                fflush(stdout);
            }
            fileReadWrite = fopen(pathfile,"w+b");
        }
        else
        {
            if (ndb_getDebugLevel() >= 7)
            {
                fprintf(stdout, "ndb_df_getReadWriteHandle: trying to open '%s' for 'r+b'\n", pathfile);
                fflush(stdout);
            }
            fileReadWrite = fopen(pathfile,"r+b");
        }

        if (fileReadWrite == NULL)
        {
            fprintf(stderr, "Error: couldn't open file '%s' with mode 'r+b'\n", pathfile);
            perror("Reason");
            fflush(stderr);
            exit(NDB_MSG_NOFILEOPEN);
        }
        gl_df.currFilOpn++;
        // jump to the end because the next write may want to append data
        fseek(fileReadWrite, 0L, SEEK_END);
        // check for correct file size
        ulFilePos = ftell(fileReadWrite);
        if (ulFilePos != pCurrDataFile->curDataSize)
        {
            fprintf(stdout, "warning: DF %d: had to correct file position from %lu to %lu\n",
                            fileNo, pCurrDataFile->curDataSize, ulFilePos);
            fflush(stdout);
            pCurrDataFile->curDataSize = ulFilePos;
        }
    }

    if (fileReadWrite == NULL)
    {
        fprintf(stderr, "ndb_df_getReadWriteHandle: could not open data file '%s' for read/write\n", pCurrDataFile->filename);
        exit (NDB_MSG_NOFILEOPEN);
    }

    pCurrDataFile->fileHandle = fileReadWrite;
    pCurrDataFile->status = NDB_DATAFILE_OPENREADWRITE;
    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_df_getReadWriteHandle: setting file handle and filestatus %s\n", ndb_df_status2print(pCurrDataFile->status));
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getReadWriteHandle - finished\n");
    fflush(stdout);

    return fileReadWrite;
}


/*
    *************************************************************************************
    ndb_df_getCheckedWriteHandle()

    Prüfen, ob fileNo im Rahmen, DataFile-retval prüfen:
    - wenn geschlossen, zum Lesen/Schreiben öffnen
    - wenn zum Lesen geöffnet, Datei schliessen und zum Lesen/Schreiben öffnen
    *************************************************************************************
*/

int ndb_df_getCheckedWriteHandle(FILE **fileHandle, int *pFileNoToWrite, PCHAPTER pChapter, PBLOCKHEADER pBlock)
{
    FILE *fileToWrite = NULL;
    PDATAFILEHEADER pNewDataFile = NULL;
    PDATAFILEHEADER pCurrDataFile;
    char sDFFilename[NDB_MAXLEN_FILENAME];
    int retval = NDB_MSG_OK;
    ULONG ulFilePos = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCheckedWriteHandle - startup\n");
    fflush(stdout);

    // check preconditions
//    if ((*iFileNoToWrite < 0) || (*iFileNoToWrite >= ndb_df_getFileCount()))
    if (*pFileNoToWrite >= ndb_df_getFileCount())
    {
        fprintf(stdout, "ndb_df_getCheckedWriteHandle: data file number %d is beyond ranges (0..%d)\n",
                        *pFileNoToWrite, ndb_df_getFileCount() - 1);
        fflush(stdout);
        exit(NDB_MSG_NOTOK);
    }

    // get file handle from current data file
    *pFileNoToWrite = ndb_df_getCurrWriteFileNo();
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_df_getCheckedWriteHandle: current data file to write  = %d\n", *pFileNoToWrite);
        fprintf(stdout, "ndb_df_getCheckedWriteHandle: current status of data file = %s\n",
                        ndb_df_status2print(gl_df.ppDataFiles[*pFileNoToWrite]->status));
        fflush(stdout);
    }
    fileToWrite = ndb_df_getReadWriteHandle(*pFileNoToWrite);
    // check whether file size is lower than max allowed data file size
    if (ndb_df_checkSizeToAdd(*pFileNoToWrite, (ULONG) pBlock->lenZip) == NDB_MSG_EXCEEDSSIZE)
    {
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_df_getCheckedWriteHandle: after size check - need a new data file\n");
            fflush(stdout);
        }
        // close current data file
        ndb_df_closeDataFile(*pFileNoToWrite);
        // create new data file
        pNewDataFile = ndb_df_createNewDataFile(ndb_df_getFileCount(), NULL, NULL);
        pNewDataFile->firstChap = pChapter->chapterNo;
        pNewDataFile->lastChap  = pChapter->chapterNo;
        ndb_df_addDataFile(pNewDataFile);
        ndb_chap_addDataFileHeader(pChapter, pNewDataFile);
        pChapter->noNewDatFil++;
        *pFileNoToWrite = ndb_df_getCurrWriteFileNo();
        pCurrDataFile = gl_df.ppDataFiles[*pFileNoToWrite];
        strcpy(sDFFilename, ndb_df_createDFName(pCurrDataFile));

        // delete file if it does exist already
        if (NDB_MSG_OK == ndb_io_check_exist_file(sDFFilename, NULL))
        {
            fprintf(stdout, "warning: DF %d does already exist and will be truncated zu length 0b now\n", *pFileNoToWrite);
            fflush(stdout);
            ndb_osdep_truncate(sDFFilename, 0);
        }
        fileToWrite = ndb_df_getReadWriteHandle(*pFileNoToWrite);
        retval = ndb_df_writeNewDataFile(*pFileNoToWrite, pNewDataFile, fileToWrite);
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_df_getCheckedWriteHandle: current data file to write = %d\n", *pFileNoToWrite);
            fflush(stdout);
        }
    }


    // be sure: position at end of file
    ulFilePos = ftell(fileToWrite);
    fseek(fileToWrite, 0L, SEEK_END);
    if (ftell(fileToWrite) != ulFilePos)
    {
        // first opening -> ulFilePos = 0 -> no warning neccessary
        // ulFilePos > 0, but wrong -> warning neccessary
        if (ulFilePos > 0)
        {
            fprintf(stdout, "warning: DF %d: had to correct file position from %lu to %lu\n",
                            *pFileNoToWrite, ulFilePos, ftell(fileToWrite));
            fflush(stdout);
        }
    }

    // current highest data file number needed for this chapter
    pChapter->noMaxDatFil = ndb_df_getFileCount() - 1;

    // return correct file handle to write the current (zipped) block data
    *fileHandle = fileToWrite;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCheckedWriteHandle - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
    ndb_df_getCurrWriteFileNo()

    gibt Index des höchsten Data Files zurück
    *************************************************************************************
*/

int ndb_df_getCurrWriteFileNo()
{
    int index = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCurrWriteFileNo - startup\n");
    fflush(stdout);

    index = gl_df.currFilCnt - 1;
    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_df_getCurrWriteFileNo: curr file count %d, returning %d\n", gl_df.currFilCnt, index);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCurrWriteFileNo - finished\n");
    fflush(stdout);

    return index;
}



/*
    *************************************************************************************
    ndb_df_getFileCount()

    liefert Zahl der vorhandenen DataFiles
    *************************************************************************************
*/

int ndb_df_getFileCount()
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getFileCount - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getFileCount - finished\n");
    fflush(stdout);

    return gl_df.currFilCnt;
}

/*
    *************************************************************************************
    ndb_df_getCurrDataFile()

    liefert Struktur zum DataFile <iFilNo> zurück
    *************************************************************************************
*/

PDATAFILEHEADER ndb_df_getCurrDataFile(int iFilNo)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCurrDataFile - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCurrDataFile - finished\n");
    fflush(stdout);

    return gl_df.ppDataFiles[iFilNo];
}

/*
    *************************************************************************************
    ndb_df_getCurrDataFilename()

    liefert Filename zum DataFile <iFilNo> zurück
    *************************************************************************************
*/

char *ndb_df_getCurrDataFilename(int iFilNo)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCurrDataFilename - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_getCurrDataFilename - finished\n");
    fflush(stdout);

    return gl_df.ppDataFiles[iFilNo]->filename;
}



/*
    *************************************************************************************
    ndb_df_checkSizeToAdd()

    Prüfen, ob Schreibvorgang in DataFile zulässige Größe überschreiten würde;
    wenn ja, dann Fehlermeldung zurückgeben
    *************************************************************************************
*/

int ndb_df_checkSizeToAdd(int fileNo, ULONG ulNewDataSize)
{
    ULONG currFilePos = 0L;
    int retval = NDB_MSG_OK;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_checkSizeToAdd - startup\n");
    fflush(stdout);

    currFilePos = ftell(ndb_df_getReadWriteHandle(fileNo));

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_df_checkSizeToAdd: current data file %d (%s)\n",
                        fileNo, gl_df.ppDataFiles[fileNo]->filename);
        fprintf(stdout, "ndb_df_checkSizeToAdd: curr size %lu, add %lu, max size %lu\n",
                        currFilePos, ulNewDataSize, gl_df.ppDataFiles[fileNo]->maxDataSize);
        fflush(stdout);
    }

    // no maxsize given -> no size check
    if (gl_df.ppDataFiles[fileNo]->maxDataSize == 0)
        retval = NDB_MSG_OK;
    // real maxsize given -> do size check
    else if ((currFilePos + ulNewDataSize) > gl_df.ppDataFiles[fileNo]->maxDataSize)
        retval = NDB_MSG_EXCEEDSSIZE;

    if (ndb_getDebugLevel() >= 8)
    {
        fprintf(stdout, "ndb_df_checkSizeToAdd: DF %d: curr size %lu, theor. size %lu, add %lu, max size %lu\n",
                        fileNo, currFilePos, gl_df.ppDataFiles[fileNo]->curDataSize, ulNewDataSize, gl_df.ppDataFiles[fileNo]->maxDataSize);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_checkSizeToAdd - finished\n");
    fflush(stdout);

    return retval;
}


/*
    *************************************************************************************
    ndb_df_addFileSize()

    führt aktuelle Filegröße in Strukturdaten mit
    *************************************************************************************
*/

void ndb_df_addFileSize(int fileNo, ULONG ulAddDataSize)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_addFileSize - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 8)
    {
        fprintf(stdout, "ndb_df_addFileSize:  DF %d, old size %lu, add %lu, new size %lu, real size %lu\n",
                                fileNo, gl_df.ppDataFiles[fileNo]->curDataSize, ulAddDataSize,
                                gl_df.ppDataFiles[fileNo]->curDataSize + ulAddDataSize,
                                ftell(ndb_df_getReadHandle(fileNo)));
        fflush(stdout);
    }
    gl_df.ppDataFiles[fileNo]->curDataSize += ulAddDataSize;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_addFileSize - finished\n");
    fflush(stdout);

    return;
}


/*
    *************************************************************************************
    ndb_df_setFileSize()

    setzt aktuelle Filegröße in Strukturdaten
    *************************************************************************************
*/

void ndb_df_setFileSize(int fileNo, ULONG ulNewDataSize)
{
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_setFileSize - startup\n");
    fflush(stdout);

    if (ndb_getDebugLevel() >= 8)
    {
        fprintf(stdout, "ndb_df_setFileSize:  DF %d: old size %lu, new size %lu, real size %lu\n",
                                fileNo, gl_df.ppDataFiles[fileNo]->curDataSize, ulNewDataSize,
                                ftell(ndb_df_getReadHandle(fileNo)));
        fflush(stdout);
    }
    gl_df.ppDataFiles[fileNo]->curDataSize = ulNewDataSize;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_setFileSize - finished\n");
    fflush(stdout);

    return;
}


/*
    *************************************************************************************
    ndb_df_closeDataFile()

    Datei schliessen;
    falls zum Schreiben offen, echte Filelänge gegen Struktur-Theoriewert prüfen,
    ggfs Struktur korrigieren; File schliessen
    falls nur zum Lesen offne, nur schliessen
    *************************************************************************************
*/

int ndb_df_closeDataFile(int fileNo)
{
    PDATAFILEHEADER pCurrDataFile = NULL;
    ULONG ulCurrSize = 0;
    int retval = NDB_MSG_OK;
    FILE *filehandle = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_closeDataFile - startup\n");
    fflush(stdout);

    pCurrDataFile = gl_df.ppDataFiles[fileNo];
    filehandle = pCurrDataFile->fileHandle;

    if (pCurrDataFile->status == NDB_DATAFILE_OPENREADWRITE)
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_df_closeDataFile: look for position of end of file\n");
        fflush(stdout);
        fflush(pCurrDataFile->fileHandle);
        fseek(filehandle, 0L, SEEK_END);
        ulCurrSize = ftell(filehandle);

        if (ulCurrSize != pCurrDataFile->curDataSize)
        {
            fprintf(stdout, "warning: DF %d: real file size (%lu) differs from calculated file size (%lu)\n",
                    fileNo, ulCurrSize, pCurrDataFile->curDataSize);
            fprintf(stdout, "         (data file '%s')\n", pCurrDataFile->filename);
            fflush(stdout);
            pCurrDataFile->curDataSize = ulCurrSize;
        }
        // now write data file header with corrected data again
        fseek(filehandle, NDB_SAVESIZE_BLOCKCHUNK, SEEK_SET);
        retval = ndb_io_writeDataFile(filehandle, pCurrDataFile, 0);
    }

    if ((pCurrDataFile->status == NDB_DATAFILE_OPENREADWRITE) ||
        (pCurrDataFile->status == NDB_DATAFILE_OPENREAD))
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_df_closeDataFile: now calling fclose() for handle 0x%08lX\n", (ULONG) filehandle);
        fflush(stdout);
        fclose(filehandle);
    }

    pCurrDataFile->status = NDB_DATAFILE_CLOSED;
    pCurrDataFile->fileHandle = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_df_closeDataFile - finished\n");
    fflush(stdout);

    return retval;
}

/*
    *************************************************************************************
    ndb_df_printGlData()

    debug output for the global data
    *************************************************************************************
*/

void ndb_df_printGlData()
{
    fprintf(stdout, "Global data of all Data Files --------------------------\n");
    fprintf(stdout, "currFilCnt:            %u\n", gl_df.currFilCnt);
    fprintf(stdout, "currChap:              %u\n", gl_df.currChap);
    fprintf(stdout, "maxDataSize:           %lu\n", gl_df.maxDataSize);
    fprintf(stdout, "maxFilCnt:             %u\n", gl_df.maxFilCnt);
    fprintf(stdout, "ppDataFiles:           %lu\n", (ULONG) gl_df.ppDataFiles);
    fprintf(stdout, "mainpath:              '%s'\n", gl_df.mainpath);
    fprintf(stdout, "lenmainfilename:       %d\n", gl_df.lenmainfilename);
    fprintf(stdout, "mainfilename:          '%s'\n", gl_df.mainfilename);
    fprintf(stdout, "identCRC:              0x%08lX\n", gl_df.identCRC);
    fprintf(stdout, "--------------------------------------------------------\n");
    fflush(stdout);
}


/*
    *************************************************************************************
    ndb_df_printDFHeadder()

    debug output of a data file header
    *************************************************************************************
*/

void ndb_df_printDFHeadder(PDATAFILEHEADER pCurrDataFile, char *comment)
{
    fprintf(stdout, "Data of Data File Header %d ---------------------\n", pCurrDataFile->datFil_No);
    if ((comment != NULL) && (comment[0] != '\0'))
        fprintf(stdout, "%s\n", comment);
    fprintf(stdout, "datFil_No:             %u\n", pCurrDataFile->datFil_No);
    fprintf(stdout, "firstChap:             %u\n", pCurrDataFile->firstChap);
    fprintf(stdout, "lastChap:              %u\n", pCurrDataFile->lastChap);
    fprintf(stdout, "curDataSize:           %lu\n", pCurrDataFile->curDataSize);
    fprintf(stdout, "maxDataSize:           %lu\n", pCurrDataFile->maxDataSize);
    fprintf(stdout, "fileHandle:            %lu\n", (ULONG) pCurrDataFile->fileHandle);
    fprintf(stdout, "fileStatus:            %s\n", ndb_df_status2print(pCurrDataFile->status));
    fprintf(stdout, "lenmainfilename:       %d\n", pCurrDataFile->lenmainfilename);
    fprintf(stdout, "mainfilename:          '%s'\n", pCurrDataFile->mainfilename);
    fprintf(stdout, "lenfilename:           %d\n", pCurrDataFile->lenfilename);
    fprintf(stdout, "filename:              '%s'\n", pCurrDataFile->filename);
    fprintf(stdout, "nxtFileData:           %lu\n", (ULONG) pCurrDataFile->nxtFileData);
    fprintf(stdout, "ownFilePositionDF:     %lu\n", pCurrDataFile->ownFilePositionDF);
    fprintf(stdout, "ownFilePositionMain:   %lu\n", pCurrDataFile->ownFilePositionMain);
    fprintf(stdout, "identCRC:              %lX\n", pCurrDataFile->identCRC);
    fprintf(stdout, "hea16CRC:              0x%04X\n", pCurrDataFile->crc16Hea);
    fprintf(stdout, "--------------------------------------------------------\n");
    fflush(stdout);
}


/*
    *************************************************************************************
    ndb_df_status2print()

    create text for retval value
    *************************************************************************************
*/

char *ndb_df_status2print(int dfStatus)
{
    if (dfStatus == NDB_DATAFILE_NEW)
        return "NDB_DATAFILE_NEW";
    else if (dfStatus == NDB_DATAFILE_CLOSED)
        return "NDB_DATAFILE_CLOSED";
    else if (dfStatus == NDB_DATAFILE_OPENREAD)
        return "NDB_DATAFILE_OPENREAD";
    else if (dfStatus == NDB_DATAFILE_OPENREADWRITE)
        return "NDB_DATAFILE_OPENREADWRITE";
    else if (dfStatus == NDB_DATAFILE_MAXSIZE)
        return "NDB_DATAFILE_MAXSIZE";
    else
        return "[unknown]";
}
