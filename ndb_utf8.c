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
#include "ndb_utf8.h"


/* ------------------------------------------------------------------------*/

// pointer to table with <codepage>/<UTF8> code pairs
int *table_utf;
char codepage_os[50] = "";
char codepage_used[50] = "";
char codepage_convertedto[50] = "";


/* ------------------------------------------------------------------------*/

/*
    *************************************************************************************
	ndb_utf8_inittables()

	finds out which NDB codepage has to be used;
	sets the pointer table_utf to the table with <codepage>/<UTF8> code pairs

	warning: the table only contains 32bit values therefore UTF8 values
	above 32 bit cannot converted or reconverted!
    *************************************************************************************
*/

void ndb_utf8_inittables(const char *cp)
{
    char *cp2;
    char *p;

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_utf8_inittables - startup\n");
        fflush(stdout);
    }

 	// check preconditions
    if (cp == NULL)
    {
        fprintf(stderr, "ndb_utf8_inittables: error: codepage is NULL\n");
        fflush(stderr);
        exit(NDB_MSG_NULLERROR);
    }

    // if no known codepage, don't use any default,
    // because we want the possibility to extract it unchanged
    // (and therefore correct) on a same system

   	// save OS codepage
   	strcpy (codepage_os, cp);
    // remember for chapter to which codepage we convert the filename;
    // overwrite it with OS codepage if NDB doesn't know it (and
    // therefore cannot convert OS filenames to UTF-8)
	strcpy (codepage_convertedto, "UTF-8");

    if (ndb_getDebugLevel() >= 5)
    {
	    fprintf(stdout, "ndb_utf8_inittables: Current OS codepage '%s'\n", cp);
	    fflush(stdout);
	}

	// change to uppercase
	for (p = (char *) cp; *p != '\0'; p++)
	{
		if (isalpha((int) *p))
			*p = toupper(*p);
	}

    // look only for the part ISOxxx-y after the dot
    if ( (cp2 = strchr(cp, '.')) == NULL)
        cp2 = (char *) cp;
    else
        cp2 = (char *) &cp2[1];
    // remove '@....' if existing
    if ( (p = strchr(cp, '@')) != NULL)
        *p = '\0';

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, " -> '%s'\n", cp2);
        fflush(stdout);
    }

    if (ndb_getDebugLevel() >= 1)
    {
        fprintf(stdout, "current OS codepage assumed to '%s'\n\n", cp2);
        fflush(stdout);
    }

    // compare with known codepages
    if (strcmp(cp2, "ISO-8859-1") == 0)
    {
        table_utf = &iso8859_1[0];
    	strcpy (codepage_used, "ISO-8859-1");
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table for codepage 'iso8859-1'\n");
            fflush(stdout);
        }
        // remember for chapter to which codepage we convert the filename
    	strcpy (codepage_convertedto, "UTF-8");
    }
    else if (strcmp(cp2, "ISO-8859-15") == 0)
    {
        table_utf = &iso8859_15[0];
    	strcpy (codepage_used, "ISO-8859-15");
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table for codepage 'iso8859-15'\n");
            fflush(stdout);
        }
    }
    else if (strcmp(cp2, "US-ASCII") == 0)
    {
        table_utf = &us_ascii[0];
    	strcpy (codepage_used, "US-ASCII");
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table for codepage 'us-ascii'\n");
            fflush(stdout);
        }
    }
    else if (strcmp(cp2, "CP437") == 0)
    {
        table_utf = &cp437[0];
    	strcpy (codepage_used, "CP437");
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table for codepage 'cp437'\n");
            fflush(stdout);
        }
    }
    else if (strcmp(cp2, "CP850") == 0)
    {
        table_utf = &cp850[0];
    	strcpy (codepage_used, "CP850");
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table for codepage 'cp850'\n");
            fflush(stdout);
        }
    }
    else if (strcmp(cp2, "CP1004") == 0)
    {
        table_utf = &cp1252[0];
        fprintf(stdout, "Warning: codepage 'CP1004' not yet implemented, therefore using 'cp1252' instead.\n\n");
    	strcpy (codepage_used, "CP1252");
        fflush(stdout);
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table 'cp1252' for codepage 'cp1004'\n");
            fflush(stdout);
        }
    }
    else if (strcmp(cp2, "CP1252") == 0)
    {
        table_utf = &cp1252[0];
    	strcpy (codepage_used, "CP1252");
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table for codepage 'cp1252'\n");
            fflush(stdout);
        }
    }
    else if (strcmp(cp2, "UTF-8") == 0)
    {
        table_utf = &utf_8[0];
    	strcpy (codepage_used, "UTF-8");
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: using table for codepage 'utf-8'\n");
            fflush(stdout);
        }
    }
    else // not implemented -> no translation
    {
    	strcpy (codepage_convertedto, cp2);
        table_utf = NULL;
        fprintf(stdout, "Warning: no codepage found or codepage not implemented here.\n");
        fprintf(stdout, "         Therefore no conversion to/from UTF-8 possible.\n\n");
    	strcpy (codepage_used, "");
        fflush(stdout);
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_inittables: codepage '%s' not implemented, therefore no conversion from/to UTF-8\n", cp2);
            fflush(stdout);
        }
    }

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_utf8_inittables - finished\n");
        fflush(stdout);
    }

}

/*
    *************************************************************************************
	ndb_utf8_getConvertedCP()

	returns UTF-8 if NDB knows the codepage of OS and can convert its filenames to UTF-8;
	if the codepage is not implemented within NDB the OS codepage name is given back
	(therefore the chapter can tell how its files are encoded)

	Returns: string with codepage name
    *************************************************************************************
*/

char *ndb_utf8_getConvertedCP()
{
    return codepage_convertedto;
}


/*
    *************************************************************************************
	ndb_utf8_createStringUTF8()

	converts the string (with OS codepage) to UTF8 codepage;

	Returns: string with UTF8 coding

	warning: the table only contains 32bit values therefore UTF8 values
	above 32 bit cannot converted or reconverted!
    *************************************************************************************
*/

char *ndb_utf8_createStringUTF8(const char *filenameExt)
{
    char *p;
    char *q;
    ULONG cUTF8;
    // worst case: every char is a four byte UTF8
    char tempbuffer[4 * NDB_MAXLEN_FILENAME + 1] = "";

    // first work with tempbuffer;
    // after the conversion is done, allocate the needed memory and copy the data into it
    char *name_utf8 = tempbuffer;


    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_utf8_createStringUTF8 - startup\n");
        fflush(stdout);
    }

    // check preconditions
    if (name_utf8 == NULL)
    {
        fprintf(stderr, "ndb_utf8_createStringUTF8: couldn't allocate memory for filename conversion to UTF8\n");
        exit (NDB_MSG_NOMEMORY);
    }
    if (table_utf == NULL)
    {
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_createStringUTF8: no conversion table set -> return NULL\n");
            fflush(stdout);
        }
        return NULL;
    }
    if ((filenameExt == NULL) || (filenameExt[0] == '\0'))
    {
        fprintf(stdout, "ndb_utf8_createStringUTF8: internal error: filename is NULL or empty\n");
        fflush(stdout);
        return 0;
    }

    // process through filename and accumulate the corresponding UTF8 chars
    for (p = (char *) filenameExt, q = name_utf8; *p != '\0'; p++)
    {
        cUTF8 = table_utf [ ((int) *p) & 0xff];
        if (ndb_getDebugLevel() >= 8)
        {
            fprintf(stdout, "ndb_utf8_createStringUTF8: original char: '%2x', UTF-8 char: '%4lX'\n",
                            ((int) *p) & 0xff, cUTF8);
            fflush(stdout);
        }

        if (cUTF8 > 0xffff)
        {
            *(q++) = (char) ((((cUTF8 & 0x1c0000) >> 18) & 0xff) + 0xf0);
            *(q++) = (char) ((((cUTF8 & 0x03f000) >> 12) & 0xff) + 0x80);
            *(q++) = (char) ((((cUTF8 & 0x000fc0) >>  6) & 0xff) + 0x80);
            *(q++) = (char) ((( cUTF8 & 0x00003f)        & 0xff) + 0x80);
        }
        else if (cUTF8 > 0x7ff)
        {
            *(q++) = (char) ((((cUTF8 & 0xf000) >> 12) & 0xff) + 0xe0);
            *(q++) = (char) ((((cUTF8 & 0x0fc0) >>  6) & 0xff) + 0x80);
            *(q++) = (char) ((( cUTF8 & 0x003f)        & 0xff) + 0x80);
        }
        else if (cUTF8 > 0x7f)
        {
            *(q++) = (char) ((((cUTF8 & 0x7c0) >> 6) & 0xff) + 0xc0);
            *(q++) = (char) ((( cUTF8 & 0x03f)       & 0xff) + 0x80);
        }
        else
        {
            *(q++) = cUTF8;
        }
        // end new filename with '\0'
        *q = '\0';
    }

    if (ndb_getDebugLevel() >= 7)
    {
        fprintf(stdout, "ndb_utf8_createStringUTF8: filename with codepage '%s':\n", ndb_osdep_getCodepage());
        for (p = (char *) filenameExt; *p != '\0'; p++)
            fprintf (stdout, "%2x ", (int) (*p & 0xff));
        fprintf(stdout, "\n");
        fprintf(stdout, "filename with codepage 'UTF-8':\n");
        for (q = (char *) name_utf8; *q != '\0'; q++)
            fprintf (stdout, "%2x ", (int) (*q & 0xff));
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    // save memory, allocate only needed space
    name_utf8 = ndb_calloc(1, strlen(name_utf8) + 1, "ndb_utf8_createUTF8: calloc() for name_utf8");
    if (name_utf8 == NULL)
    {
        fprintf(stderr, "ndb_utf8_createStringUTF8: couldn't reallocate memory for UTF8 filename\n");
        exit (NDB_MSG_NOMEMORY);
    }
    strcpy (name_utf8, tempbuffer);


    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_utf8_createStringUTF8 - finished\n");
        fflush(stdout);
    }

    return name_utf8;
}


/*
    *************************************************************************************
	ndb_utf8_createStringCP()

	converts the string with UTF8 codepage to current OS codepage

	Returns: string with current OS codepage coding

	warning: the table only contains 32bit values therefore UTF8 values
	above 32 bit cannot converted or reconverted!
    *************************************************************************************
*/

char *ndb_utf8_createStringCP(const char *filenameUTF8)
{
    char *p;
    char *q;
    ULONG cUTF8;
    int iSuccess = 0;
    int iModeUTF8 = 0;
    int i = 0;
    // worst case: no UTF8 char is known by current codepage
    char tempbuffer[9 * NDB_MAXLEN_FILENAME + 1] = "";

    // first work with tempbuffer;
    // after the conversion is done, allocate the needed memory and copy the data into it
    char *name_ext = tempbuffer;

    if (ndb_getDebugLevel() >= 6)
    {
        fprintf(stdout, "ndb_utf8_createStringCP - startup\n");
        fflush(stdout);
    }

    // check precondition
    if (name_ext == NULL)
    {
        fprintf(stderr, "ndb_utf8_createStringCP: couldn't allocate memory for filename conversion to cp\n");
        exit (NDB_MSG_NOMEMORY);
    }
    if (table_utf == NULL)
    {
        if (ndb_getDebugLevel() >= 5)
        {
            fprintf(stdout, "ndb_utf8_createStringCP: no conversion table set -> return NULL\n");
            fflush(stdout);
        }
        return NULL;
    }
    if ((filenameUTF8 == NULL) || (filenameUTF8[0] == '\0'))
    {
        fprintf(stdout, "ndb_utf8_createStringCP: internal error: filename is NULL or empty\n");
        fflush(stdout);
        return 0;
    }

    // process through UTF-8 filename and recompute the original filename
    for (p = (char *) filenameUTF8, q = name_ext; *p != '\0'; )
    {
        // get first byte
        cUTF8 = ((int) *(p++)) & 0xff;
        if (cUTF8 >= 0xf7)
        {
            // get second byte
            cUTF8 = ((cUTF8 & 0x07) << 6) + (((int) *(p++)) & 0x3f);
            // get third byte
            cUTF8 = ( cUTF8         << 6) + (((int) *(p++)) & 0x3f);
            // get forth byte (= last byte)
            cUTF8 = ( cUTF8         << 6) + (((int) *(p++)) & 0x3f);
        }
        else if (cUTF8 >= 0xe0)
        {
            // get second byte
            cUTF8 = ((cUTF8 & 0x0f) << 6) + (((int) *(p++)) & 0x3f);
            // get third byte (= last byte)
            cUTF8 = ( cUTF8         << 6) + (((int) *(p++)) & 0x3f);
        }
        else if (cUTF8 >= 0xc1)
        {
            // get second byte (= last byte)
            cUTF8 = ((cUTF8 & 0x1f) << 6) + (((int) *(p++)) & 0x3f);
        }
        // get corresponding code from codepage
        iSuccess = -1;
        // first try to get quick results for characters of normal ascii
        if (cUTF8 < 0x100)
        {
            if (table_utf[cUTF8] == cUTF8)
            {
                iSuccess = cUTF8;
                // do we need a closing '_' from before,
                // because we leave undecodable UTF-8 char?
                if (iModeUTF8 == 1)
                    *(q++) = '_';
                // add found character to string
                *(q++) = (char) cUTF8;
                iModeUTF8 = 0;
            }
        }
        // if nothing found then loop over the complete array
        if (iSuccess == -1)
        {
            for (i = 255; i >= 0; i--)
            {
                if (table_utf[i] == cUTF8)
                {
                    iSuccess = i;
                    // do we need a closing '_' from before,
                    // because we leave undecodable UTF-8 char?
                    if (iModeUTF8 == 1)
                        *(q++) = '_';
                    // add found character to string
                    *(q++) = (char) i;
                    iModeUTF8 = 0;
                    break;
                }
            }
        }

        // character not found -> add "_%xxxx_"
        if (iSuccess == -1)
        {
            char hex[10];
            int i;
            // add a separation between normal characters and undecodable UTF-8 characters
            if (iModeUTF8 == 0)
                *(q++) = '_';
            *(q++) = '%';
            snprintf(hex, 10 - 1, "%lX", cUTF8);
            for (i = 0; i < strlen(hex); i++)
            {
                *(q++) = hex[i];
            }
            // now we are in mode 'undecodable UTF-8 character'
            iModeUTF8 = 1;
        }

        if (ndb_getDebugLevel() >= 8)
        {
            fprintf(stdout, "ndb_utf8_createStringCP: UTF-8 char: '%4lx', original char: '%2X'\n",
                            cUTF8, iSuccess);
            fflush(stdout);
        }

    }
    // end string with zero byte
    *q = '\0';

    if (ndb_getDebugLevel() >= 8)
    {
        fprintf(stdout, "filename with codepage 'UTF-8':\n");
        for (q = (char *) filenameUTF8; *q != '\0'; q++)
            fprintf (stdout, "%2x ", (int) (*q & 0xff));
        fprintf(stdout, "\n");
        fprintf(stdout, "filename with codepage '%s':\n", ndb_osdep_getCodepage());
        for (p = (char *) name_ext; *p != '\0'; p++)
            fprintf (stdout, "%2x ", (int) (*p & 0xff));
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    // allocate only the needed memory
    name_ext = ndb_calloc(1, strlen(name_ext) + 1, "ndb_utf8_createCP: calloc() for name_ext");
    if (name_ext == NULL)
    {
        fprintf(stderr, "ndb_utf8_createStringCP: couldn't reallocate memory for cp filename\n");
        exit (NDB_MSG_NOMEMORY);
    }
    strcpy (name_ext, tempbuffer);

    if (ndb_getDebugLevel() >= 5)
    {
        fprintf(stdout, "ndb_utf8_createStringCP - finished\n");
        fflush(stdout);
    }
    return name_ext;
}


/*
    *************************************************************************************
	ndb_utf8_aliasWinCP()

	converts the win32 codepage name to the ISO (?) codepage name
    *************************************************************************************
*/

char *ndb_utf8_aliasWinCP(const char *cp_win)
{
    if (strcmp(cp_win, "Afrikaans_South Africa.1252") == 1)
        return "af_ZA.iso8859-1";
    if (strcmp(cp_win, "Arabic_Bahrain.1256") == 1)
        return "ar_BH.iso8859-6";
    if (strcmp(cp_win, "Arabic_Algeria.1256") == 1)
        return "ar_DZ.iso8859-6";
    if (strcmp(cp_win, "Arabic_Egypt.1256") == 1)
        return "ar_EG.iso8859-6";
    if (strcmp(cp_win, "Arabic_Iraq.1256") == 1)
        return "ar_IQ.iso8859-6";
    if (strcmp(cp_win, "Arabic_Jordan.1256") == 1)
        return "ar_JO.iso8859-6";
    if (strcmp(cp_win, "Arabic_Kuwait.1256") == 1)
        return "ar_KW.iso8859-6";
    if (strcmp(cp_win, "Arabic_Lebanon.1256") == 1)
        return "ar_LB.iso8859-6";
    if (strcmp(cp_win, "Arabic_Libya.1256") == 1)
        return "ar_LY.iso8859-6";
    if (strcmp(cp_win, "Arabic_Morocco.1256") == 1)
        return "ar_MA.iso8859-6";
    if (strcmp(cp_win, "Arabic_Oman.1256") == 1)
        return "ar_OM.iso8859-6";
    if (strcmp(cp_win, "Arabic_Qatar.1256") == 1)
        return "ar_QA.iso8859-6";
    if (strcmp(cp_win, "Arabic_Saudi Arabia.1256") == 1)
        return "ar_SA.iso8859-6";
    if (strcmp(cp_win, "Arabic_Tunisia.1256") == 1)
        return "ar_TN.iso8859-6";
    if (strcmp(cp_win, "Arabic_Yemen.1256") == 1)
        return "ar_YE.iso8859-6";
    if (strcmp(cp_win, "Belarusian_Belarus.1251") == 1)
        return "be_BY.iso8859-5";
    if (strcmp(cp_win, "Bulgarian_Bulgaria.1251") == 1)
        return "bg_BG.iso8859-5";
    if (strcmp(cp_win, "Catalan_Spain.1252") == 1)
        return "ca_ES.iso8859-1";
    if (strcmp(cp_win, "Czech_Czech Republic.1250") == 1)
        return "cs_CZ.iso8859-2";
    if (strcmp(cp_win, "Danish_Denmark.1252") == 1)
        return "da_DK.iso8859-1";
    if (strcmp(cp_win, "German_Austria.1252") == 1)
        return "de_AT.iso8859-1";
    if (strcmp(cp_win, "German_Switzerland.1252") == 1)
        return "de_CH.iso8859-1";
    if (strcmp(cp_win, "German_Germany.1252") == 1)
        return "de_DE.iso8859-1";
    if (strcmp(cp_win, "German_Liechtenstein.1252") == 1)
        return "de_LI.iso8859-1";
    if (strcmp(cp_win, "German_Luxembourg.1252") == 1)
        return "de_LU.iso8859-1";
    if (strcmp(cp_win, "Greek_Greece.1253") == 1)
        return "el_GR.iso8859-7";
    if (strcmp(cp_win, "English_Australia.1252") == 1)
        return "en_AU.iso8859-1";
    if (strcmp(cp_win, "English_Belize.1252") == 1)
        return "en_BZ.iso8859-1";
    if (strcmp(cp_win, "English_Canada.1252") == 1)
        return "en_CA.iso8859-1";
    if (strcmp(cp_win, "English_Ireland.1252") == 1)
        return "en_IE.iso8859-1";
    if (strcmp(cp_win, "English_Jamaica.1252") == 1)
        return "en_JM.iso8859-1";
    if (strcmp(cp_win, "English_New Zealand.1252") == 1)
        return "en_NZ.iso8859-1";
    if (strcmp(cp_win, "English_Trinidad y Tobago.1252") == 1)
        return "en_TT.iso8859-1";
    if (strcmp(cp_win, "English_United Kingdom.1252") == 1)
        return "en_UK.iso8859-1";
    if (strcmp(cp_win, "English_United States.1252") == 1)
        return "en_US.iso8859-1";
    if (strcmp(cp_win, "English_South Africa.1252") == 1)
        return "en_ZA.iso8859-1";
    if (strcmp(cp_win, "Spanish_Argentina.1252") == 1)
        return "es_AR.iso8859-1";
    if (strcmp(cp_win, "Spanish_Bolivia.1252") == 1)
        return "es_BO.iso8859-1";
    if (strcmp(cp_win, "Spanish_Chile.1252") == 1)
        return "es_CL.iso8859-1";
    if (strcmp(cp_win, "Spanish_Colombia.1252") == 1)
        return "es_CO.iso8859-1";
    if (strcmp(cp_win, "Spanish_Costa Rica.1252") == 1)
        return "es_CR.iso8859-1";
    if (strcmp(cp_win, "Spanish_Dominican Republic.1252") == 1)
        return "es_DO.iso8859-1";
    if (strcmp(cp_win, "Spanish_Ecuador.1252") == 1)
        return "es_EC.iso8859-1";
    if (strcmp(cp_win, "Spanish - Modern Sort_Spain.1252") == 1)
        return "es_ES.iso8859-1";
    if (strcmp(cp_win, "Spanish - Traditional Sort_Spain.1252") == 1)
        return "es_ES.iso8859-1";
    if (strcmp(cp_win, "Spanish_Guatemala.1252") == 1)
        return "es_GT.iso8859-1";
    if (strcmp(cp_win, "Spanish_Honduras.1252") == 1)
        return "es_HN.iso8859-1";
    if (strcmp(cp_win, "Spanish_Mexican.1252") == 1)
        return "es_MX.iso8859-1";
    if (strcmp(cp_win, "Spanish_Nicaragua.1252") == 1)
        return "es_NI.iso8859-1";
    if (strcmp(cp_win, "Spanish_Panama.1252") == 1)
        return "es_PA.iso8859-1";
    if (strcmp(cp_win, "Spanish_Paraguay.1252") == 1)
        return "es_PY.iso8859-1";
    if (strcmp(cp_win, "Spanish_Peru.1252") == 1)
        return "es_PE.iso8859-1";
    if (strcmp(cp_win, "Spanish_Puerto Rico.1252") == 1)
        return "es_PR.iso8859-1";
    if (strcmp(cp_win, "Spanish_El Salvador.1252") == 1)
        return "es_SV.iso8859-1";
    if (strcmp(cp_win, "Spanish_Uruguay.1252") == 1)
        return "es_UY.iso8859-1";
    if (strcmp(cp_win, "Spanish_Venezuela.1252") == 1)
        return "es_VE.iso8859-1";
    if (strcmp(cp_win, "Estonian_Estonia.1257") == 1)
        return "et_EE.iso8859-4";
    if (strcmp(cp_win, "Basque_Spain.1252") == 1)
        return "eu_ES.iso8859-1";
        return "eu_ES.iso8859-1";
    if (strcmp(cp_win, "Finnish_Finland.1252") == 1)
        return "fi_FI.iso8859-1";
    if (strcmp(cp_win, "French_Belgium.1252") == 1)
        return "fr_BE.iso8859-1";
    if (strcmp(cp_win, "French_Canada.1252") == 1)
        return "fr_CA.iso8859-1";
    if (strcmp(cp_win, "French_Switzerland.1252") == 1)
        return "fr_CH.iso8859-1";
    if (strcmp(cp_win, "French_France.1252") == 1)
        return "fr_FR.iso8859-1";
    if (strcmp(cp_win, "French_Luxembourg.1252") == 1)
        return "fr_LU.iso8859-1";
    if (strcmp(cp_win, "Faeroese_Faeroe Islands.1252") == 1)
        return "fo_FO.iso8859-1";
    if (strcmp(cp_win, "Hebrew_Israel.1255") == 1)
        return "he_IL.iso8859-8";
    if (strcmp(cp_win, "Croatian_Croatia.1250") == 1)
        return "hr_HR.iso8859-2";
    if (strcmp(cp_win, "Hungarian_Hungary.1250") == 1)
        return "hu_HU.iso8859-2";
    if (strcmp(cp_win, "Indonesian_Indonesia.1252") == 1)
        return "id_ID.iso8859-1";
    if (strcmp(cp_win, "Icelandic_Iceland.1252") == 1)
        return "is_IS.iso8859-1";
    if (strcmp(cp_win, "Italian_Switzerland.1252") == 1)
        return "it_CH.iso8859-1";
    if (strcmp(cp_win, "Italian_Italy.1252") == 1)
        return "it_IT.iso8859-1";
    if (strcmp(cp_win, "Japanese_Japan.932") == 1)
        return "ja_JP.SJIS";
    if (strcmp(cp_win, "Korean_Korea.949") == 1)
        return "ko_KR.EUC";
    if (strcmp(cp_win, "Lithuanian_Lithuania.1257") == 1)
        return "lt_LT.iso8859-4";
    if (strcmp(cp_win, "Latvian_Latvia.1257") == 1)
        return "lv_LV.iso8859-4";
        return "lv_LV.iso8859-4";
    if (strcmp(cp_win, "Dutch_Belgium.1252") == 1)
        return "nl_BE.iso8859-1";
    if (strcmp(cp_win, "Dutch_Netherlands.1252") == 1)
        return "nl_NL.iso8859-1";
    if (strcmp(cp_win, "Norwegian (Nynorsk)_Norway.1252") == 1)
        return "no_NO.iso8859-1";
    if (strcmp(cp_win, "Norwegian (Bokmål)_Norway.1252") == 1)
        return "no_NO.iso8859-1";
    if (strcmp(cp_win, "Polish_Poland.1250") == 1)
        return "pl_PL.iso8859-2";
    if (strcmp(cp_win, "Portuguese_Brazil.1252") == 1)
        return "pt_BR.iso8859-1";
    if (strcmp(cp_win, "Portuguese_Portugal.1252") == 1)
        return "pt_PT.iso8859-1";
    if (strcmp(cp_win, "Romanian_Romania.1250") == 1)
        return "ro_RO.iso8859-2";
    if (strcmp(cp_win, "Russian_Russia.1251") == 1)
        return "ru_RU.iso8859-5";
    if (strcmp(cp_win, "Slovak_Slovakia.1250") == 1)
        return "sk_SK.iso8859-2";
    if (strcmp(cp_win, "Slovene_Slovenia.1250") == 1)
        return "sl_SI.iso8859-2";
    if (strcmp(cp_win, "Albanian_Albania.1250") == 1)
        return "sq_AL.iso8859-2";
    if (strcmp(cp_win, "Serbian (Latin)_Serbia.1250") == 1)
        return "sr_YU.iso8859-2";
    if (strcmp(cp_win, "Swedish_Finland.1252") == 1)
        return "sv_FI.iso8859-1";
    if (strcmp(cp_win, "Swedish_Sweden.1252") == 1)
        return "sv_SE.iso8859-1";
    if (strcmp(cp_win, "Turkish_Turkey.1254") == 1)
        return "tr_TR.iso8859-9";
    if (strcmp(cp_win, "Ukrainian_Ukraine.1251") == 1)
        return "uk_UA.iso8859-5";
    if (strcmp(cp_win, "Chinese(PRC)_People's Republic of China.936") == 1)
        return "zh_CN.EUC";
    if (strcmp(cp_win, "Chinese(PRC)_Hong Kong.950") == 1)
        return "zh_HK.EUC";
    if (strcmp(cp_win, "Chinese(Singapore)_Signapore.936") == 1)
        return "zh_SG.EUC";
    if (strcmp(cp_win, "Chinese(Taiwan)_Taiwan.950") == 1)
        return "zh_TW.EUC";

    return "";
}


/* ------------------------------------------------------------------------*/
