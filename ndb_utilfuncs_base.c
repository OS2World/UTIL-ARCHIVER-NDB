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
#include <ctype.h>

#include "ndb.h"
#include "ndb_prototypes.h"


int debug_level = 0;
int blocksize   = NDB_DEFAULT_BLOCKSIZE;
int compression = NDB_DEFAULT_COMPRESSION;

clock_t clock_start;
clock_t clock_curr;


/*
    *************************************************************************************
	ndb_write_struct()

	writes <size> bytes of buffer <p> to file <f>
    *************************************************************************************
*/

int ndb_write_struct(FILE *f, void *p, ULONG size)
{
    if (fwrite(p,size,1,f) != size)
        return NDB_MSG_NOTOK;
    else
        return NDB_MSG_OK;
}

/*
    *************************************************************************************
	ndb_read_struct()

	reads <size> bytes into buffer <p> from file <f>

    *************************************************************************************
*/

int ndb_read_struct(FILE *f, void *p, ULONG size)
{
    if (fread(p,size,1,f) != size)
        return NDB_MSG_NOTOK;
    else
        return NDB_MSG_OK;
}


/*
    *************************************************************************************
	ndb_write2buf_char()

	copies a single char <value> (as single byte) in the buffer <buf>;
	<nr> is the current pointer within the buffer and gets incremented by 1
    *************************************************************************************
*/

int ndb_write2buf_char(char *buf, int *nr, char value)
{
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "saving char 0x%X at buffer pos 0x%x\n", value & 0xff, *nr);
    buf[(*nr)++] = value & 0xff;
    return (*nr);
}

/*
    *************************************************************************************
	ndb_write2buf_ushort()

	copies a USHORT <value> (as two byte, little endian) in the buffer <buf>;
	<nr> is the current pointer within the buffer and gets incremented by 2
    *************************************************************************************
*/

int ndb_write2buf_ushort(char *buf, int *nr, USHORT value)
{
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "saving ushort 0x%X at buffer pos 0x%x\n", value, *nr);
    buf[(*nr)++] = value & 0xff;
    buf[(*nr)++] = (value >>  8) & 0xff;
    return (*nr);
}

/*
    *************************************************************************************
	ndb_write2buf_ulong()

	copies a ULONG <value> (as four byte, little endian) in the buffer <buf>;
	<nr> is the current pointer within the buffer and gets incremented by 4
    *************************************************************************************
*/

int ndb_write2buf_ulong(char *buf, int *nr, ULONG value)
{
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "saving ulong 0x%lX at buffer pos 0x%x\n", value, *nr);
    buf[(*nr)++] = value & 0xff;
    buf[(*nr)++] = (value >>  8) & 0xff;
    buf[(*nr)++] = (value >> 16) & 0xff;
    buf[(*nr)++] = (value >> 24) & 0xff;
    return (*nr);
}

/*
    *************************************************************************************
	ndb_write2buf_ulong8()

	copies a ULONG8 <value> (as eight byte, little endian) in the buffer <buf>;
	<nr> is the current pointer within the buffer and gets incremented by 8
    *************************************************************************************
*/

int ndb_write2buf_ulong8(char *buf, int *nr, ULONG8 value)
{
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "saving ulong8 0x%llX at buffer pos 0x%x\n", value, *nr);
    buf[(*nr)++] = value & 0xff;
    buf[(*nr)++] = (value >>  8) & 0xff;
    buf[(*nr)++] = (value >> 16) & 0xff;
    buf[(*nr)++] = (value >> 24) & 0xff;
    buf[(*nr)++] = (value >> 32) & 0xff;
    buf[(*nr)++] = (value >> 40) & 0xff;
    buf[(*nr)++] = (value >> 48) & 0xff;
    buf[(*nr)++] = (value >> 56) & 0xff;
    return (*nr);
}

/*
    *************************************************************************************
	ndb_write2buf_char_n()

	copies a string <value> of <n> chars (as single bytes) in the buffer <buf>;
        after the first '\0' the rest of the chars is written as '\0', too;
	<nr> is the current pointer within the buffer and gets incremented by n
    *************************************************************************************
*/

int ndb_write2buf_char_n(char *buf, int *nr, char *value, int len)
{
    int count;
    int end = 0;
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "saving 0x%x chars at buffer pos 0x%x\n", len, *nr);
    for (count = 0; count < len; count++)
    {
        if (end == 0) if (value[count] == '\0') end = 1;
        buf[(*nr)++] = (end == 0) ? value[count] : '\0';
    }
    return (*nr);
}

/*
    *************************************************************************************
	ndb_write2buf_byte_n()

	copies an array <value> of <n> bytes in the buffer <buf>;
	<nr> is the current pointer within the buffer and gets incremented by n
    *************************************************************************************
*/

int ndb_write2buf_byte_n(char *buf, int *nr, char *value, int len)
{
    int count;
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "saving 0x%x bytes at buffer pos 0x%x\n", len, *nr);
    for (count = 0; count < len; count++)
    {
        buf[(*nr)++] = value[count];
    }
    return (*nr);
}

/*
    *************************************************************************************
	ndb_readbuf_char()

	reads a single char (as single byte) from the buffer <buf>, position <*nr>;
	<nr> is the current pointer within the buffer and gets incremented by 1

	Returns: read char
    *************************************************************************************
*/

char ndb_readbuf_char(char *buf, int *nr)
{
    char value = 0;
    value =  (buf[(*nr)++] & 0xff);
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "reading char 0x%X from buffer pos 0x%x\n", value & 0xff, (*nr) - 1);
    return (value & 0xff);
}

/*
    *************************************************************************************
	ndb_readbuf_ushort()

	reads a single USHORT (as two bytes, little endian)
	from the buffer <buf>, position <*nr>;
	<nr> is the current pointer within the buffer and gets incremented by 2

	Returns: read USHORT
    *************************************************************************************
*/

USHORT ndb_readbuf_ushort(char *buf, int *nr)
{
    USHORT value = 0;
    value =  (buf[(*nr)++] & 0xff);
    value += (buf[(*nr)++] & 0xff) << 8;
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "reading ushort 0x%X from buffer pos 0x%x\n", value, (*nr) - 2);
    return (value & 0xffff);
}

/*
    *************************************************************************************
	ndb_readbuf_ulong()

	reads a single ULONG (as four bytes, little endian)
	from the buffer <buf>, position <*nr>;
	<nr> is the current pointer within the buffer and gets incremented by 4

	Returns: read ULONG
    *************************************************************************************
*/

ULONG ndb_readbuf_ulong(char *buf, int *nr)
{
    ULONG value = 0;
    value =  (buf[(*nr)++] & 0xff);
    value += (buf[(*nr)++] & 0xff) <<  8;
    value += (buf[(*nr)++] & 0xff) << 16;
    value += (buf[(*nr)++] & 0xff) << 24;
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "reading ulong 0x%lX from buffer pos 0x%x\n", value, (*nr) - 4);
    return (value & 0xffffffff);
}

/*
    *************************************************************************************
	ndb_readbuf_ulong8()

	reads a single ULONG8 (as eight bytes, little endian)
	from the buffer <buf>, position <*nr>;
	<nr> is the current pointer within the buffer and gets incremented by 8

	Returns: read ULONG8
    *************************************************************************************
*/

ULONG8 ndb_readbuf_ulong8(char *buf, int *nr)
{
    ULONG8 value = 0;
    value =  (buf[(*nr)++] & 0xff);
    value += ((ULONG8) (buf[(*nr)++] & 0xff)) <<  8;
    value += ((ULONG8) (buf[(*nr)++] & 0xff)) << 16;
    value += ((ULONG8) (buf[(*nr)++] & 0xff)) << 24;
    value += ((ULONG8) (buf[(*nr)++] & 0xff)) << 32;
    value += ((ULONG8) (buf[(*nr)++] & 0xff)) << 40;
    value += ((ULONG8) (buf[(*nr)++] & 0xff)) << 48;
    value += ((ULONG8) (buf[(*nr)++] & 0xff)) << 56;
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "reading ulong8 0x%llX from buffer pos 0x%x\n", value, (*nr) - 8);
    return (value & 0xffffffffffffffffLL);
}


/*
    *************************************************************************************
	ndb_readbuf_char_n()

	reads an object of <n> bytes from the buffer <buf>;
        add an additional '\0' to the end of the read chars to make sure,
        that the string is closed;
	<nr> is the current pointer within the buffer and gets incremented by n

	Returns: the byte object as (char *)

	not thread safe
    *************************************************************************************
*/

char p_buf_tmp[1024];

char *ndb_readbuf_char_n(char *buf, int *nr, int len)
{
    int count;
    char *q;

    for (count = 0, q = p_buf_tmp; count < len; count++)
    {
        *(q++) = (char) buf[(*nr)++];
    }
    *q = '\0';
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "reading 0x%x chars from buffer pos 0x%x\n", len, (*nr) - len);
    return (p_buf_tmp);
}


/*
    *************************************************************************************
	ndb_readbuf_byte_n()

	reads an object of <n> bytes from the buffer <buf>;
	<nr> is the current pointer within the buffer and gets incremented by n

	Returns: the byte object as (char *)

	not thread safe
    *************************************************************************************
*/

char *ndb_readbuf_byte_n(char *buf, int *nr, int len)
{
    int count;
    char *q;

    for (count = 0, q = p_buf_tmp; count < len; count++)
    {
        *(q++) = (char) buf[(*nr)++];
    }
    if (ndb_getDebugLevel() >= 8) fprintf(stdout, "reading 0x%x bytes from buffer pos 0x%x\n", len, (*nr) - len);
    return (p_buf_tmp);
}



/*
    *************************************************************************************
	ndb_util_prompt_user()

	prompts the user by showing text <text> and waiting for an user input (with CR);
	the typed key must be one char from the chars given in the string <chars>,
	otherwise the input must be done again

	Returns: the char the user typed in
    *************************************************************************************
*/

char ndb_util_prompt_user(const char *text, const char *chars)
{
	char c;

	fprintf(stdout, text);
	fflush(stdout);

	// let user answer
	c = fgetc(stdin);
	while (NULL == strchr(chars, c))
	{
    	fprintf(stdout, "\nWrong character, please use only one of the above: ");
    	fflush(stdout);
		c = fgetc(stdin);
	}

	fprintf(stdout, "\n");
	fflush(stdout);

	return c;
}

/*
    *************************************************************************************
	ndb_util_formatraw()

	gets a string and converts it into a special format <count>:<value>
	where <count> is the string length (written in hex) and <value> the
	given string

	Returns: the converted string
    *************************************************************************************
*/

char sraw[1024];

char *ndb_util_formatraw(char *value)
{
	int i = strlen(value);

	snprintf(sraw, 1024 - 1, "%X:%s", i, value);

	return sraw;
}


/*
    *************************************************************************************
	ndb_util_cleanIntPath()

	removes drive letters (e.g. c:) and UNC paths (e.g. //<server>/<share>);
	because the string is already changed to internal format, we have
	only to care about the internal path delimiter, no matter which OS
    *************************************************************************************
*/

void ndb_util_cleanIntPath(char *pPath)
{
	int i;
	char *p, *q;

	if (ndb_getDebugLevel() >= 7)
	{
		fprintf(stdout, "ndb_util_cleanIntPath: filename before: '%s'\n", pPath);
		fflush(stdout);
	}

	// first a check
	if ((pPath == NULL) || pPath[0] == '\0')
		return;

	p = q = pPath;

	// strip drive letter
	if (strlen(pPath) > 1)
	{
		if ((toupper(p[0]) >= 'A') && (toupper(p[0]) <= 'Z') && (p[1] == ':'))
			p += 2;
	}

	// strip UNC paths (//<host>/<share>/)
	if (!strncmp(p, "//", 2) && ((p[2] != '\0') && (p[2] != '/')))
	{
		// skip '//'
		q = p + 2;
		// skip <host>
		while ((*q != '\0') && (*q != '/'))
			q++;
		if (*q != '\0')
		{
			// skip '/' between <host> and <share>
			q++;
			// skip <share>
			while ((*q != '\0') && (*q != '/'))
				q++;
		}
		// skip '/' after <share>
		if (*q != '\0')
			q++;
	}
	else // no UNC name
		q = p;

	// skip leading '/' to get relative paths
	while (*q == '/')
		q++;

	// skip leading './' to get relative paths
	while (*q == '.' && q[1] == '/')
		q += 2;

	// now copy shortened name over string
	for (i = 0; ; i++)
	{
		pPath[i] = q[i];
		// copy string terminator also
		if (q[i] == '\0')
			break;
	}

	if (ndb_getDebugLevel() >= 7)
	{
		fprintf(stdout, "ndb_util_cleanIntPath: filename after:  '%s'\n", pPath);
		fflush(stdout);
	}
}


/* ======================================================================== */
/* functions for getting and setting blocksize or compression level         */
/* ======================================================================== */

/*
    *************************************************************************************
	ndb_setBlockSize()

	set the block size of the archive file

	not thread safe
    *************************************************************************************
*/

void ndb_setBlockSize(USHORT size)
{
    blocksize = size;
}

/*
    *************************************************************************************
	ndb_getBlockSize()

	Returns: blocksize

	not thread safe
    *************************************************************************************
*/

USHORT ndb_getBlockSize()
{
    return blocksize;
}

/*
    *************************************************************************************
	ndb_setCompression()

	sets the compression level of the archive file

	not thread safe
    *************************************************************************************
*/

void ndb_setCompression(USHORT level)
{
    compression = level;
}

/*
    *************************************************************************************
	ndb_getCompression()

	Returns: compression level

	not thread safe
    *************************************************************************************
*/

USHORT ndb_getCompression()
{
    return compression;
}


/* ======================================================================== */
/* functions for debugging                                                  */
/* ======================================================================== */


/*
    *************************************************************************************
	ndb_setDebugLevel()

	set the debug level 0..9 for NDB;
	starts the clock for time measurement
    *************************************************************************************
*/

void ndb_setDebugLevel(int n)
{
    debug_level = n;
    fprintf(stdout, "debug level set to %d\n", debug_level);
    clock_start = clock();
}

/*
    *************************************************************************************
	ndb_getDebugLevel()

	Returns the current debug level
    *************************************************************************************
*/

#if !defined (NDB_DEBUG_OFF)
int ndb_getDebugLevel()
{
    return debug_level;
}
#endif

/*
    *************************************************************************************
	ndb_printTimeMillis();

	prints the time (since last ndb_setDebugLevel()) in milliseconds to stdout
    *************************************************************************************
*/

void ndb_printTimeMillis()
{
	char smillis[10];
    clock_curr = clock();
    snprintf(smillis, 10 - 1, "%08u", (unsigned)((clock_curr - clock_start) * 1000 / CLOCKS_PER_SEC));
	// put a dot between seconds and milliseconds
	smillis[9] = smillis[8];
	smillis[8] = smillis[7];
	smillis[7] = smillis[6];
	smillis[6] = smillis[5];
	smillis[5] = '.';
    fprintf(stdout, "%s: ", smillis);
}

/*
    *************************************************************************************
	ndb_printDebug()

	prints the passed time and the given string <p> for debug purposes to stdout

	currently not used
    *************************************************************************************
*/

void ndb_printDebug(const char *p)
{
    clock_curr = clock();
    fprintf(stdout, "%06u: %s\n", (unsigned)((clock_curr - clock_start) * 1000 / CLOCKS_PER_SEC), p);
}


/*
    *************************************************************************************
	ndb_util_msgcode2print()

	create text for NDB message code (NDB_MSG_...)
    *************************************************************************************
*/

char *ndb_util_msgcode2print(int msgCode)
{
	if (msgCode == NDB_MSG_OK)
		return "ok";
	else if (msgCode == NDB_MSG_NOTOK)
		return "something is not ok";
	else if (msgCode == NDB_MSG_NOMEMORY)
		return "memory allocation failed";
	else if (msgCode == NDB_MSG_NOFILEOPEN)
		return "file could not be opened";
	else if (msgCode == NDB_MSG_BADMAGIC)
		return "didn't get the expected archive chunk magic";
	else if (msgCode == NDB_MSG_READERROR)
		return "error reading from file";
	else if (msgCode == NDB_MSG_WRITEERROR)
		return "error writing to file";
	else if (msgCode == NDB_MSG_NULLERROR)
		return "unexpected NULL pointer";
	else if (msgCode == NDB_MSG_EOF)
		return "unexpected end of file";
	else if (msgCode == NDB_MSG_NOPTRFOUND)
		return "memory pointer not found in pointer list";
	else if (msgCode == NDB_MSG_PTRNOTVALID)
		return "tried to use an already freed memory pointer";
	else if (msgCode == NDB_MSG_PTROVERWRITTEN)
		return "memory bounds of pointer exceeded";
	else if (msgCode == NDB_MSG_ILLEGALRETURN)
		return "should not happen - internal error";
	else if (msgCode == NDB_MSG_ZLIBERROR)
		return "library zlib returned an error";
	else if (msgCode == NDB_MSG_NODIRCREATE)
		return "could not create directory";
	else if (msgCode == NDB_MSG_NOTFOUND)
		return "could not find element";
	else if (msgCode == NDB_MSG_EXTRAFAILED)
		return "getting or setting file extra data failed";
	else if (msgCode == NDB_MSG_BADVERSION)
		return "archive file has a different version";
	else if (msgCode == NDB_MSG_CORRUPTDATA)
		return "packed data block(s) corrupted";
	else if (msgCode == NDB_MSG_EXCEEDSSIZE)
		return "data file size exceeds size limit";
	else if (msgCode == NDB_MSG_IDENTERROR)
		return "data file and main file have different idents";
	else if (msgCode == NDB_MSG_SYMLINKFAILED)
		return "creating symbolic link failed";
	else if (msgCode == NDB_MSG_DUPLICATEFILENAME)
		return "filename already exists in chapter";
	else if (msgCode == NDB_MSG_OTHERCOMPRESSIONALGO)
		return "archiv file uses another compression method";
	else if (msgCode == NDB_MSG_CRCERROR)
		return "calculated CRC value differs from saved CRC";
	else if (msgCode == NDB_MSG_FILETOOLARGE)
		return "file size exceeds 2 GB limit of NDB";
	else
		return "[unknown]";
}



/* Lookup table for fast CRC computation
 * See 'CRC_update_lookup'
 * Uses the polynomial x^16+x^15+x^2+1 */
unsigned int crc16_lookup[256] =
{
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/* fast CRC-16 computation - uses table crc16_lookup */
USHORT ndb_crc16(USHORT crc, unsigned char *buffer, int len)
{
        int i;
        USHORT tmp;

        for (i = 0; i < len; i++)
        {
            tmp = crc ^ (buffer[i] && 0xff);
            crc = (crc >> 8) ^ crc16_lookup[tmp & 0xff];
        }
        return crc;
}


/*
    *************************************************************************************
	ndb_crc32()

	calculates the CRC32 of a byte array
    *************************************************************************************
*/

/* crc32 function of zlib */
unsigned long crc32(ULONG, unsigned char *, unsigned);


ULONG ndb_crc32(ULONG crc, unsigned char *buf, ULONG len)
{
	ULONG newcrc32 = 0;
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

    if (buf == NULL) return 0L;

#if defined(TEST_PERFORMANCE)
	// start performance measurement
	start_time = ndb_osdep_getPreciseTime();
//  printf("ndb_filefound: start_time = %llu\n", start_time);
#endif

	newcrc32 =  crc32(crc, buf, (unsigned) len);

#if defined(TEST_PERFORMANCE)
    if (ndb_getDebugLevel() >= 1)
    {
		curr_time = ndb_osdep_getPreciseTime();
    	dbg_clock_diff = (ULONG) (curr_time - start_time);
    	dbg_count++;
    	dbg_timall += dbg_clock_diff;
    	if (dbg_clock_diff > dbg_timmax)
    		dbg_timmax = dbg_clock_diff;
		snprintf(s, 199, "ndb_crc32: count %lu, average %lu, max %lu, sum %lu\n",
						 dbg_count, (dbg_timall / dbg_count), dbg_timmax, dbg_timall);
        ndb_util_set_performancetest(TEST_PERFORMANCE_CRC32, s);
    }
#endif

	return newcrc32;
}

/*
    *************************************************************************************
	ndb_util_printMD5()

	converts the MD5 checksum from the binary form into a hex string
    *************************************************************************************
*/

char *ndb_util_printMD5(unsigned char *md5)
{
    static char s_digest[40];
    snprintf(s_digest, 40 - 1, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%c",
                     md5[ 0], md5[ 1], md5[ 2], md5[ 3],
                     md5[ 4], md5[ 5], md5[ 6], md5[ 7],
                     md5[ 8], md5[ 9], md5[10], md5[11],
                     md5[12], md5[13], md5[14], md5[15],
                     '\0');
    return s_digest;
}

/*
    *************************************************************************************
	ndb_util_init_performancetest()

	inits the table for the performance output lines
    *************************************************************************************
*/

#if defined(TEST_PERFORMANCE)

#define TEST_PERFORMANCE_MAXLENGTH 200
short performancetest_init = 0;
char performancetest_lines[TEST_PERFORMANCE_MAX][TEST_PERFORMANCE_MAXLENGTH];

void ndb_util_init_performancetest()
{
    int i;

	for (i = 0; i < TEST_PERFORMANCE_MAX; i++)
    	memset(performancetest_lines[i], 0, TEST_PERFORMANCE_MAXLENGTH);
    performancetest_init = 1;
}

void ndb_util_set_performancetest(int index, char *line)
{
    if (performancetest_init == 0)
        ndb_util_init_performancetest();

    if (index >= TEST_PERFORMANCE_MAX)
    {
        fprintf(stderr, "ndb_util_set_performancetest: error: index %d out of range 0..%d\n",
                index, TEST_PERFORMANCE_MAX);
        fflush(stderr);
    }
    else
    {
        strcpy(performancetest_lines[index], line);
    }

}

/*
    *************************************************************************************
	ndb_util_print_performancetest()

	outputs the lines with the performance measurements of every recorded function
    *************************************************************************************
*/

void ndb_util_print_performancetest()
{
    int i;

    fprintf(stdout, "\n----- Debug Output Of Time Measurement -----------------------------\n");
    fprintf(stdout, "(all times in microseconds)\n");
    fflush(stdout);

    for (i = 0; i < TEST_PERFORMANCE_MAX; i++)
    {
        if (performancetest_lines[i] != '\0')
            fprintf(stdout, performancetest_lines[i]);
    }
    fprintf(stdout, "--------------------------------------------------------------------\n");
    fflush(stdout);
}

#endif

