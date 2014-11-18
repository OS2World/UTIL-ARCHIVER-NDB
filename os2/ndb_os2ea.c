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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#if !defined(__WATCOMC__)
#include <alloca.h>
#endif
#include <sys/types.h>


#define INCL_DOSDATETIME
#include <os2.h>

#if defined(__GNUC__)
#include <os2emx.h>
#endif

#include "ndb_os2.h"

#include "../ndb.h"
#include "../ndb_prototypes.h"



#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

#ifndef max
#define max(a, b) ((a) < (b) ? (b) : (a))
#endif

int use_longname_ea = 0; /* 1=use the .LONGNAME EA as the file's name */

/* .LONGNAME EA code */

typedef struct
{
  ULONG cbList;               /* length of value + 22 */
  ULONG oNext;
  BYTE fEA;                   /* 0 */
  BYTE cbName;                /* length of ".LONGNAME" = 9 */
  USHORT cbValue;             /* length of value + 4 */
  BYTE szName[10];            /* ".LONGNAME" */
  USHORT eaType;              /* 0xFFFD for length-preceded ASCII */
  USHORT eaSize;              /* length of value */
  BYTE szValue[CCHMAXPATH];
}
FEALST;

typedef struct
{
  ULONG cbList;
  ULONG oNext;
  BYTE cbName;
  BYTE szName[10];            /* ".LONGNAME" */
}
GEALST;


/* general EA code */

/* Perhaps due to bugs in the current OS/2 2.0 kernel, the success or
   failure of the DosEnumAttribute() and DosQueryPathInfo() system calls
   depends on the area where the return buffers are allocated. This
   differs for the various compilers, for some alloca() works, for some
   malloc() works, for some, both work. We'll have to live with that. */

/* The use of malloc() is not very convenient, because it requires
   backtracking (i.e. free()) at error returns. We do that for system
   calls that may fail, but not for malloc() calls, because they are VERY
   unlikely to fail. If ever, we just leave some memory allocated
   over the usually short lifetime of a zip process ... */

#ifdef __GNUC__
// #define alloc(x) alloca(x)
// change by pw, alloca() gave unexpected crashes, malloc() seems to be ok
//#define alloc(x) malloc(x)
//#define unalloc(x)
// 03-Sep-2004; pw; change for Innotek GCC with -Zhigh-mem
#if defined(INNOTEK)
#define alloc(x) _lmalloc(x)
#define unalloc(x) free(x)
void *  _lmalloc(size_t);
void *  _lcalloc(size_t, size_t);
void *  _lrealloc(void *, size_t);
#else
#define alloc(x) malloc(x)
#define unalloc(x) free(x)
#endif

#else
#define alloc(x) malloc(x)
#define unalloc(x) free(x)
#endif



// ********************************************************************************************************
// code to get EAs
// adapted from Infozip's zip sources, not fully understood ;-)
// ********************************************************************************************************

#if defined(__WATCOMC__)
typedef FEA2 DENA2;
typedef PFEA2 PDENA2;
#define ENUMEA_REFTYPE_PATH             1
#define ENUMEA_LEVEL_NO_VALUE           1
typedef unsigned long   APIRET;
#endif

int GetEAs(char *path, char **bufptr, USHORT *size, char *remark)
{
  FILESTATUS4 fs;
  PDENA2 pDENA, pFound;
  EAOP2 eaop;
  PGEA2 pGEA;
  PGEA2LIST pGEAlist;
  PFEA2LIST pFEAlist;
  ULONG ulAttributes;
  ULONG nLength;
  ULONG nBlock;
  char szName[NDB_MAXLEN_FILENAME];
  APIRET rc;

  *size = 0;

/*
{
  char *p = NULL;
  char *q = NULL;
  char *r = NULL;

  p = malloc(100);
  q = alloca(100);
  r = _lcalloc(1, 100);
  fprintf(stderr, "GetEAs: malloc() gave: %08lX\n", (ULONG) p);
  fprintf(stderr, "        alloca() gave: %08lX\n", (ULONG) q);
  fprintf(stderr, "       _lalloc() gave: %08lX\n", (ULONG) r);
  fflush(stderr);
}
*/

  strcpy(szName, path);
  nLength = strlen(szName);
  if (szName[nLength - 1] == '/')
    szName[nLength - 1] = 0;

  if (ndb_getDebugLevel() >= 7)
      fprintf(stdout, "GetEAs: calling 1st DosQueryPathInfo() for '%s'\n", szName);
  fflush(stdout);

  // ask how much buffer size is neccessary for all EAs
  if ((rc = DosQueryPathInfo(szName, FIL_QUERYEASIZE, (PBYTE) &fs, sizeof(fs))))
  {
      fprintf(stderr, "GetEAs: error (rc = %lu) calling 1st DosQueryPathInfo() for '%s'\n", rc, szName);
      fflush(stderr);
      snprintf(remark, 199, "GetEAs: error (rc = %lu) calling 1st DosQueryPathInfo()%c", rc, '\0');
      return NDB_MSG_EXTRAFAILED;
  }
  if (ndb_getDebugLevel() >= 7)
    fprintf(stdout, "GetEAs: after 1st DosQueryPathInfo(), needed size for EAs (fs.cbList) = %ld bytes\n", fs.cbList);
  fflush(stdout);

  // fs.cbList contains the needed size
  nBlock = min(fs.cbList, 65535);
  if (ndb_getDebugLevel() >= 7)
    fprintf(stdout, "GetEAs: trying to alloc %ld bytes for EA name buffer and for EA value buffer\n", nBlock);
  fflush(stdout);

  // allocate the same size for the first buffer (EA names only)
  if ((pDENA = alloc((size_t) nBlock)) == NULL)
  {
    fprintf(stdout, "GetEAs: alloc() for pDENA (EA name buffer) failed\n");
    fflush(stdout);
    return NDB_MSG_NOMEMORY;
  }
  // allocate the size for the second buffer (EA names and EA values)
  if ((pGEAlist = alloc((size_t) nBlock)) == NULL)
  {
    if (ndb_getDebugLevel() >= 5)
      fprintf(stdout, "GetEAs: alloc() for pGEAlist (EA value buffer) failed\n");
    fflush(stdout);
    unalloc(pDENA);
    return NDB_MSG_NOMEMORY;
  }

  // now: flag to read as much EAs as possible for buffer size
  // later: number of read EAs
  ulAttributes = -1;

  if (ndb_getDebugLevel() >= 7)
      fprintf(stdout, "GetEAs: calling DosEnumAttribute() for EA name list of '%s'\n", szName);
  fflush(stdout);

  // get list of all EA names as FEA list (without data values)
  // szName: filename;
  // pDENA: buffer;
  // nBlock: buffer size;
  // &ulAttributes: IN: flag to fill buffer size OUT: number of read EAs;
  if ((rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH, szName, 1, pDENA, nBlock,
                       &ulAttributes, ENUMEA_LEVEL_NO_VALUE))
    || (ulAttributes == 0))
  {
    if (ndb_getDebugLevel() >= 5)
    {
          if (ulAttributes == 0)
        fprintf(stdout, "GetEAs: no EAs found\n");
      else
        fprintf(stdout, "GetEAs: error calling DosEnumAttribute() [rc=%lu]\n", rc);
      fflush(stdout);
    }
    // free both EA buffer
    unalloc(pDENA);
    unalloc(pGEAlist);
    // return with appropriate status
    if (rc != 0)
        return NDB_MSG_EXTRAFAILED;
    else
        return NDB_MSG_OK;
  }

  // clear EA data buffer
  memset(pGEAlist, 0, nBlock);
  // set pGEA to first entry of pGEAlist
  pGEA = pGEAlist -> list;
  // start with first entry
  pFound = pDENA;

  if (ndb_getDebugLevel() >= 7)
      fprintf(stdout, "GetEAs: create GEA list from FEA list\n");
  fflush(stdout);

  /* create GEA list from FEA list (skip .LONGNAME) */
  while (ulAttributes--)
  {
        // skip .LONGNAME because we save the complete name already in pFile
    if (!(strcmp(pFound -> szName, ".LONGNAME") == 0 && use_longname_ea))
    {
       if (ndb_getDebugLevel() >= 7)
          fprintf(stdout, "GetEAs: found EA %s\n", pFound->szName);
       fflush(stdout);

      // copy length of EA name
      pGEA -> cbName = pFound -> cbName;
      // copy EA name
      strcpy(pGEA -> szName, pFound -> szName);

      // length of EA file name entry including last '\0'
      nLength = sizeof(GEA2) + strlen(pGEA -> szName);
          // align on ULONG (double word) boundaries
      nLength = ((nLength - 1) / sizeof(ULONG) + 1) * sizeof(ULONG);

      // set offset = 0 for last EA entry
      pGEA -> oNextEntryOffset = ulAttributes ? nLength : 0;
      // set pGEA list to next free entry
      pGEA   = (PGEA2)  ((PCH) pGEA + nLength);
    }
    else
    {
       if (ndb_getDebugLevel() >= 7)
          fprintf(stdout, "GetEAs: skip EA %s\n", pFound->szName);
       fflush(stdout);
    }
        // goto next EA name
    pFound = (PDENA2) ((PCH) pFound + pFound->oNextEntryOffset);
  }

  // no EA remaining? pGEA still at the start?
  if (pGEA == pGEAlist->list) /* no attributes to save */
  {
    if (ndb_getDebugLevel() >= 5)
      fprintf(stdout, "GetEAs: only EA is '.LONGNAME' - not needed here\n");
    fflush(stdout);
    // free EA buffers
    unalloc(pDENA);
    unalloc(pGEAlist);
    // no EAs is ok
    return NDB_MSG_OK;
  }

  // calculate really needed buffer size
  pGEAlist -> cbList = (PCH) pGEA - (PCH) pGEAlist;

  // reuse buffer
  pFEAlist = (PVOID) pDENA;
  // set available size
  pFEAlist -> cbList = nBlock;

  // set EA value buffer
  eaop.fpGEA2List = pGEAlist;
  // set EA name buffer
  eaop.fpFEA2List = pFEAlist;
  eaop.oError = 0;

  if (ndb_getDebugLevel() >= 7)
      fprintf(stdout, "GetEAs: calling 2nd DosQueryPathInfo() for EA values list of '%s'\n", szName);
  fflush(stdout);

  /* get 'real' FEA list (with data values also) */
  if ((rc = DosQueryPathInfo(szName, FIL_QUERYEASFROMLIST,
                       (PBYTE) &eaop, sizeof(eaop))))
  {
    fprintf(stderr, "GetEAs: error (rc = %lu) calling 2nd DosQueryPathInfo() for '%s'\n", rc, szName);
    fflush(stderr);
    unalloc(pDENA);
    unalloc(pGEAlist);
    return NDB_MSG_EXTRAFAILED;
  }

  *size = pFEAlist -> cbList;
  *bufptr = (char *) pFEAlist;

  if (ndb_getDebugLevel() >= 5)
      fprintf(stdout, "GetEAs - finished\n");
  fflush(stdout);

  return NDB_MSG_OK;
}

/*
    *************************************************************************************
        ndb_os2_getExtraData()

        get EAs and pack the EA data into a list of blocks (if size of EAs not bigger
        than the blocksize one block is enough);
        return the pointer of the block header list start
    *************************************************************************************
*/

int ndb_os2_getExtraData(struct ndb_s_fileentry *pFile, struct ndb_s_fileextradataheader **ppExtraHea, FILE *fArchive, struct ndb_s_blockheader **ppBlocks, char *sText)
{
    char   *pBufEAs;
    USHORT sizeEAs;
    char sExtName[NDB_MAXLEN_FILENAME];
    char remark[200];
    char sDummy[200];
    int statusEAs;
        struct ndb_s_fileextradataheader *pExtraHea = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_getExtraData - startup\n");
    fflush(stdout);

    strcpy(sExtName, ndb_osdep_makeExtFileName(pFile->filenameExt));

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_getExtraData: calling GetEAs() for '%s'\n", sExtName);
    fflush(stdout);

    pBufEAs = NULL;
    sizeEAs = 0;
    remark[0] = '\0';
    statusEAs = GetEAs(sExtName, &pBufEAs,&sizeEAs, remark);
    if (remark[0] != '\0')
        ndb_fe_addRemark(pFile, remark);

    if (statusEAs != NDB_MSG_OK)
    {
        sizeEAs = 0;
        strcat(sText, ", EAs failed");
    }
    else
    {
                if (sizeEAs > 0)
        {
            snprintf(sDummy, 200 - 1, ", %db EAs%c", sizeEAs, '\0');
            strcat(sText, sDummy);
        }
    }

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_getExtraData: %d bytes EAs found\n", sizeEAs);
    fflush(stdout);

    if (sizeEAs > 0)
    {
        pExtraHea = ndb_calloc(sizeof(struct ndb_s_fileextradataheader), 1, "ndb_os2_getExtraData: pExtraHea");
        if (pExtraHea == NULL)
        {
            fprintf(stderr, "ndb_os2_getExtraData: couldn't allocate extra data header\n");
            fflush(stderr);
            exit(NDB_MSG_NOMEMORY);
        }
        pExtraHea->typExtHea = NDB_EF_OS2EA;
        statusEAs = ndb_fe_createBuffer2ExtraHeader(pExtraHea, pBufEAs, sizeEAs, fArchive);
                *ppBlocks = pExtraHea->firstBlockHeader;
        free (pBufEAs);
    }

    *ppExtraHea = pExtraHea;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_getExtraData - finished\n");
    fflush(stdout);

    return statusEAs;
}


// ********************************************************************************************************
// code to set EAs
// adapted from Infozip's unzip sources, not fully understood ;-)
// ********************************************************************************************************


int SetEAs(char *path, void *ea_block)
{
    EAOP2 eaop;
    PFEA2LIST pFEA2list;
    USHORT nLength;
    char szName[NDB_MAXLEN_FILENAME] = "[n.a.]";
    APIRET rc = 0;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "SetEAs - startup\n");
    fflush(stdout);

    strcpy(szName, path);
    nLength = strlen(szName);
    if (szName[nLength - 1] == '/')
        szName[nLength - 1] = 0;

    // make sure that file is not READONLY otherwise setting EAs wont work
    chmod(szName, S_IREAD | S_IWRITE);

    pFEA2list = ea_block;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "SetEAs: try to set %lu bytes EAs from 0x%08lX for '%s'\n", 
                pFEA2list -> cbList, (ULONG) ea_block, szName);
    fflush(stdout);

    eaop.fpGEA2List = NULL;
    eaop.fpFEA2List = pFEA2list;
    eaop.oError = 0;

    // currently only DosSetPathInfo() works ... ;->
    rc = DosSetPathInfo(szName, FIL_QUERYEASIZE, (PBYTE) &eaop, sizeof(eaop), 0);

    if (rc != 0)
        fprintf(stderr, "SetEAs: DosSetPathInfo() returned %lu for '%s'\n", (ULONG) rc, szName);
    fflush(stderr);

/*
    Better use DosSetFileInfo() instead of DosSetPathInfo(), because DosSetFileInfo()
    needs exclusive write access, in other words, the file must not be open - but it
    is from ndb_expandstream() [now closed before ndb_os2_setExtraData() is called].
    Therefore we open a file handle and use DosSetFileInfo(), to avoid this problem.
*/
/*
    HFILE hFile;
    ULONG nAction;

    if (! DosOpen(szName, &hFile, &nAction, 0, 0,
                 OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,
                 OPEN_SHARE_DENYNONE, 0) )
    {
        rc = DosSetFileInfo(hFile, FIL_QUERYEASIZE, (PBYTE) &eaop, sizeof(eaop));
        DosClose(hFile);

            if (rc != 0)
                fprintf(stderr, "SetEAs: DosSetFileInfo() returned %lu for '%s'\n", (ULONG) rc, szName);
            fflush(stderr);
    }
*/

    
    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "SetEAs - finished\n");
    fflush(stdout);

    if (rc != 0)
        return NDB_MSG_NOTOK;
    else
        return NDB_MSG_OK;
}


/*
    *************************************************************************************
        ndb_os2_setEAs()

        create EA data from the zipped buffers of given block list;
        call setEAs with the EAs buffer to set the EAs to the file (or dir)
    *************************************************************************************
*/

int ndb_os2_setEAs(struct ndb_s_fileextradataheader *pExtraHea, char *newFilename, FILE *fArchive, int opt_testintegrity)
{
    int retval = NDB_MSG_OK;
    ULONG currSizeEAs = 0L;
    char *pBufEAs  = NULL;
    char *pBufEAs2 = NULL;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_setEAs - startup\n");
    fflush(stdout);

    retval = ndb_fe_createExtraHeader2Buffer(pExtraHea, &pBufEAs2, &currSizeEAs, fArchive);

    if ((retval == NDB_MSG_OK) && (currSizeEAs > 0L))
    {
#if defined(INNOTEK)
        // copy data from pBufEAs2 (high mem) to pBufEAs (lo mem)
        pBufEAs = alloc(currSizeEAs);
#else
#define pBufEAs pBufEAs2
#endif
        if (pBufEAs == NULL)
        {
            fprintf(stderr, "\nndb_os2_setEAs: couldn't allocate %lu bytes for EAs buffer\n", currSizeEAs);
            fflush(stderr);
            retval = NDB_MSG_EXTRAFAILED;
        }
        else
        {
#if defined(INNOTEK)
            // copy data to lo mem
            if (ndb_getDebugLevel() >= 5)
            {
                fprintf(stdout, "\nndb_os2_setEAs: trying to copy %lu bytes EAs from 0x%8lX to 0x%08lX\n",
                                currSizeEAs, (ULONG) pBufEAs2, (ULONG) pBufEAs);
                fflush(stdout);
            }
            memcpy(pBufEAs, pBufEAs2, currSizeEAs);
#endif
            
            if (ndb_getDebugLevel() >= 5)
            {
                fprintf(stdout, "ndb_os2_setEAs: trying to set %lu bytes EA data for '%s'\n", currSizeEAs, newFilename);
                fflush(stdout);
            }
            // set EAs only if not -t (test only) specified
            if ((opt_testintegrity == 0) && NDB_MSG_OK != (retval = SetEAs(newFilename, pBufEAs)))
            {
                fprintf(stdout, "ndb_os2_setEAs: Error: setting %lu bytes EA data for '%s'\n", 
                        currSizeEAs, newFilename);
                fflush(stdout);
                retval = NDB_MSG_EXTRAFAILED;
            }
        }
    }
    else
    {
        fprintf(stdout, "ndb_os2_setEAs: Error: unpacking %lu bytes EA data for '%s' failed\n", currSizeEAs, newFilename);
        fflush(stdout);
    }

#if defined(INNOTEK)
    if(pBufEAs  != NULL)
        ndb_free(pBufEAs);
#else
#undef pBufEAs
#endif
    if(pBufEAs2 != NULL)
        ndb_free(pBufEAs2);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_os2_setEAs - finished\n");
    fflush(stdout);

    return retval;
}


