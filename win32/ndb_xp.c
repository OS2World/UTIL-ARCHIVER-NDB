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
#include <sys/utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <locale.h>



#include "../ndb.h"
#include "../ndb_prototypes.h"

#include <windows.h>
#include <winbase.h>

typedef BOOL (*MYPROC)(HANDLE,LPCSTR);
HINSTANCE hinstLib = NULL;
MYPROC MySetFileShortName = NULL;
BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;

void ndb_win32_init()
{
	if (ndb_win32_isWinXP())
	{
		// Get a handle to the DLL module.
		hinstLib = LoadLibrary("kernel32");

		// If the handle is valid, try to get the function address.
		if (hinstLib != NULL)
		{
		    MySetFileShortName = (MYPROC) GetProcAddress(hinstLib, "SetFileShortNameA");

		    // If the function address is valid, we can call the function later
		    if (NULL != MySetFileShortName)
		    {
		        fRunTimeLinkSuccess = TRUE;
				if (ndb_getDebugLevel() >= 7)
				{
				    fprintf(stdout, "ndb_win32_init: we have a valid handle for 'SetFileShortNameA'\n");
				    fflush(stdout);
				}
		    }
		}
	}
}


void ndb_win32_done()
{
	if (ndb_win32_isWinXP())
	{
	    if (hinstLib != NULL)
	    {
	        // Free the DLL module.
	        fFreeResult = FreeLibrary(hinstLib);
	    }
	}
}


static short bRestorePrivilege = FALSE;
static short bBackupPrivilege = FALSE;

static VOID InitLocalPrivileges(VOID)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;

    // pw: everything already done?
    if (bRestorePrivilege && bBackupPrivilege)
        return;

    /* try to enable some interesting privileges that give us the ability
       to get some security information that we normally cannot.

       note that enabling privileges is only relevant on the local machine;
       when accessing files that are on a remote machine, any privileges
       that are present on the remote machine get enabled by default. */

    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    /* try to enable SeRestoreName; we need this for using SetFIleShortName() */
    if(LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.Privileges[0].Luid))
    {
        if (AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL)
           && GetLastError() == ERROR_SUCCESS)
        {
        	bRestorePrivilege = TRUE;
        }
    }
    /* try to enable SeRestoreName; we need this for using SetFIleShortName() */
    if(LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &tp.Privileges[0].Luid))
    {
        if (AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL)
           && GetLastError() == ERROR_SUCCESS)
        {
        	bBackupPrivilege = TRUE;
        }
    }

    CloseHandle(hToken);
}
/*
    *************************************************************************************
	ndb_win32_setShortName()

	only WinXP and above: set 8.3 name for given long name;
	the check, whether NDB is running under XP or not must be
	done before calling this function

	Returns: NDB status
    *************************************************************************************
*/

// this module should be compileable with W2K, Win9X, ... also
#if (_WIN32_WINNT < 0x0501)
BOOL WINAPI SetFileShortNameA(HANDLE,LPCSTR);
#endif

int ndb_win32_setShortName(char *longname, char *shortname)
{
    long shResult = 0;
    HANDLE hFile;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_setShortName - startup\n");

	if ((!ndb_win32_isWinXP()) || (!isFileSystemNTFS(longname)))
	{
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "\nInfo: no XP or no NTFS\n");
            fflush(stdout);
        }
		return NDB_MSG_NOTOK;
	}

	if (ndb_getDebugLevel() >= 6)
	{
		fprintf(stdout, "ndb_win32_setShortName: calling SetFileShortName(..)\n    to set '%s' for '%s'\n",
		        shortname, longname);
		fflush(stdout);
	}

	// set SE_RESTORE_NAME privilege
	InitLocalPrivileges();
	// did it work?
	if ((bRestorePrivilege == FALSE) || (bRestorePrivilege == FALSE))
	{
		if (ndb_getDebugLevel() >= 6)
		{
	        fprintf(stdout, "ndb_win32_setShortName: setting privileges SE_RESTORE_NAME or SE_BACKUP_NAME didn't work\n");
	        fprintf(stdout, "    therefore aborting ndb_win32_setShortName\n");
	        fflush(stdout);
	    }
        return NDB_MSG_NOTOK;
	}
	else
	{
		if (ndb_getDebugLevel() >= 6)
		{
	        fprintf(stdout, "ndb_win32_setShortName: setting privileges SE_RESTORE_NAME and SE_BACKUP_NAME worked\n");
    	    fflush(stdout);
    	}
	}

	// clear status of GetLastError()
	shResult = GetLastError();
    // get handle for file
    hFile = CreateFile(longname, GENERIC_ALL, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
		// clear status of GetLastError()
		shResult = GetLastError();
		// set shortname for file
//    	shResult = SetFileShortNameA(hFile, shortname);
		if (fRunTimeLinkSuccess == TRUE)
			shResult = (MySetFileShortName) (hFile, shortname);
    	if (ndb_getDebugLevel() >= 7)
	    {
	        fprintf(stdout, "ndb_win32_setShortName: calling SetFileShortName(..) = %ld\n", shResult);
	        fprintf(stdout, "    long  name was '%s'\n", longname);
	        fprintf(stdout, "    short name was '%s'\n", shortname);
	        fflush(stdout);
	    }
		if (shResult == 0)
		{
			shResult = GetLastError();
			if (shResult == ERROR_ALREADY_EXISTS)
			{
				fprintf(stderr, "Warning: couldn't set shortname '%s'\n", shortname);
				fprintf(stderr, "         Reason: The specified short name is not unique.\n");
				fflush(stderr);
			}
			else if (shResult == ERROR_INVALID_PARAMETER)
			{
				fprintf(stderr, "Warning: couldn't set shortname '%s'\n", shortname);
				fprintf(stderr, "         No NTFS file system? Invalid short name? File already opened in case-sensitive mode?\n");
				fflush(stderr);
			}
			else
			{
				fprintf(stderr, "Warning: couldn't set shortname '%s'\n", shortname);
				fprintf(stderr, "         GetLastError() told: %ld\n", shResult);
				fflush(stderr);
			}

		}
		CloseHandle(hFile);
	}
	else
	{
		fprintf(stderr, "Warning: error (%d) setting 8.3 name for '%s'\n", (int) GetLastError(), longname);
		fflush(stderr);
	}

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_setShortName - finished\n");
    fflush(stdout);

    return (shResult != 0) ? NDB_MSG_OK : NDB_MSG_NOTOK;
}


/*
    *************************************************************************************
	ndb_win32_setFileSize64()

	Win98 and above: check file for size longer than 4GB; if such large file
	set pFile->lenOri8 with correct value (before it's the length mod 4 GB)
    *************************************************************************************
*/

void ndb_win32_setFileSize64(PFILEENTRY pFile)
{
    WIN32_FILE_ATTRIBUTE_DATA w32FileAttr;

    BOOL shResult;

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_setFileSize64 - startup\n");
    fflush(stdout);

    // clear GetLastError()
    GetLastError();
    // clear struct content
    memset(&w32FileAttr, 0, sizeof(WIN32_FILE_ATTRIBUTE_DATA));

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_win32_setFileSize64: previous file size is %.0f\n", (double) pFile->lenOri8);
    fflush(stdout);

    shResult = GetFileAttributesEx(pFile->filenameExt, GetFileExInfoStandard, &w32FileAttr);

    if (shResult)
    {
        if (ndb_getDebugLevel() >= 7)
        {
            fprintf(stdout, "ndb_win32_setFileSize64: file size lo %9lu / 0x%04lX\n", w32FileAttr.nFileSizeLow, w32FileAttr.nFileSizeLow);
            fprintf(stdout, "ndb_win32_setFileSize64: file size hi %9lu / 0x%04lX\n", w32FileAttr.nFileSizeHigh, w32FileAttr.nFileSizeHigh);
            fflush(stdout);
        }

        pFile->lenOri8 = (ULONG8) w32FileAttr.nFileSizeLow;

        if (w32FileAttr.nFileSizeHigh > 0)
        {
            pFile->lenOri8 += ((ULONG8) w32FileAttr.nFileSizeHigh) << 32;
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_win32_setFileSize64: set file size to %.0f\n", (double) pFile->lenOri8);
            fflush(stdout);
        }
        else
        {
            if (ndb_getDebugLevel() >= 7)
                fprintf(stdout, "ndb_win32_setFileSize64: file size %.0f -> smaller than 4 gb\n", (double) pFile->lenOri8);
            fflush(stdout);
        }
    }
    else
    {
        if (ndb_getDebugLevel() >= 7)
            fprintf(stdout, "ndb_win32_setFileSize64: GetFileAttributesEx failed, GetLastError() told: %lu\n", GetLastError());
        fflush(stdout);

    }

    if (ndb_getDebugLevel() >= 7)
        fprintf(stdout, "ndb_win32_setFileSize64: file size now %.0f\n", (double) pFile->lenOri8);
    fflush(stdout);

    if (ndb_getDebugLevel() >= 5)
        fprintf(stdout, "ndb_win32_setFileSize64 - finished\n");
    fflush(stdout);

    return;
}


/*
    *************************************************************************************
	ndb_win32_openFileOri()

	opens original file from harddrive and returns the file handle
    *************************************************************************************
*/

HANDLE ndb_win32_openFileOri(char *sFile)
{
    HANDLE hFile = 0;

    // set privileges SE_BACKUP_NAME (and SE_RESTORE_NAME)
	InitLocalPrivileges();
/*
    // get handle for file
    hFile = CreateFile(sFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                       FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_BACKUP_SEMANTICS, NULL);
*/
    hFile = CreateFile(sFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                       FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_BACKUP_SEMANTICS, NULL);

    return hFile;
}
