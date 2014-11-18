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


/*  VAX/DEC CMS REPLACEMENT HISTORY, Element GU_ALLOCROUT.C*/
/*  *4    25-AUG-2004 18:00:00 WIESNETH "various bug fixes"*/
/*  *3    11-JAN-2003 22:35:00 WIESNETH "port for ndb*"*/
/*  *2     8-JUL-1997 10:57:21 WIESNETH "new for VA26A"*/
/*  *1     3-DEC-1996 17:55:15 WIESNETH "check memory allocations"*/
/*  VAX/DEC CMS REPLACEMENT HISTORY, Element GU_ALLOCROUT.C*/
/*@@BOCUH@@*/
/*Compilation Unit*************************************************************
*
* Product    :  OPSIS/LIB_GU
*
* File       :  ndb_allocrout.c
*
* Author     : Peter Wiesneth, TDI 1, Phone: 09131/84-3781
*
* Description: Memory allocation with debug infos
*
* History
* -------
* Vsn   Date            Name    CHARM   Comment
* VA25A  4-NOV-1996     pw      -       Creation
******************************************************************************/

/* --- INTERFACE ---------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ndb.h"
#include "ndb_prototypes.h"

/* external functions */
long ndb_alloc_init (long i_maxptr);
long ndb_alloc_done (void);
void *ndb_malloc (size_t i_size, char *remark);
void *ndb_calloc (size_t i_nr, size_t i_size, char *remark);
void *ndb_realloc (void *m_ptr, size_t i_size, char *remark);
long ndb_free (void *m_ptr);
long ndb_show_ptr (void);
void ndb_show_status (void);


/* --- IMPLEMENTATION ----------------------------------------------------- */


/* private functions */
void ndb_crash (void);
long ndb_get_ptr_p (void *i_ptr, long *o_index, short *o_status, size_t *o_size, char *o_remark);
long ndb_get_ptr_ps (void *i_ptr, short i_status, long *o_index, size_t *o_size, char *o_remark);
long ndb_get_ptr_i (long i_index, void **o_ptr, short *o_status, size_t *o_size, char *o_remark);
long ndb_set_ptr_i (long i_index, void *i_ptr, short i_status, size_t i_size, char *i_remark);
long ndb_add_ptr (void *o_ptr, void *i_ptr, size_t i_size, char *remark);
long ndb_set_mark (void *i_ptr_start, size_t i_size);
long ndb_chk_mark (void *i_ptr_start, size_t i_size);
long ndb_chk_ptr (void *i_ptr, long *o_index);
long ndb_free_ptr (void *m_ptr);

# define PTR_NONE             0
# define PTR_VALID            1
# define PTR_CLEAR            2

# define REMARK_LEN          50

struct ndb_ptrety_ {
   void   *ptr;
   long   size;
   short  status;
   char   remark[REMARK_LEN];
   } ndb_ptrety;

long  maxptr = 0;
struct ndb_ptrety_ *ndb_allptr = NULL;
long  highest_ptr_no = 0;

// count memory usage
unsigned long ndb_alloc_size_max   = 0;
unsigned long ndb_alloc_size_curr  = 0;
unsigned long ndb_alloc_count_max  = 0;
unsigned long ndb_alloc_count_curr = 0;


# define LASTELEM      123456789L
# define LASTELEMSIZE          sizeof(LASTELEM)

short DebugFlag = 0;

/*@@EOCUH@@*/
/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_alloc_init
*
* Description  : Inits all internal data.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_alloc_init ();
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_alloc_init (long i_maxptr)
{
/*@@EOFH@@*/

#if defined(NDB_DEBUG_ALLOC_OFF)

    return NDB_MSG_OK;

#else

   long retval;
   long i;

   ndb_alloc_size_max   = 0;
   ndb_alloc_size_curr  = 0;
   ndb_alloc_count_max  = 0;
   ndb_alloc_count_curr = 0;

   maxptr = i_maxptr;
   ndb_allptr = calloc (maxptr, sizeof (struct ndb_ptrety_));
   if (ndb_allptr == NULL)
   {
      fprintf(stderr, "ndb_alloc_init: Error: couldn't allocate memory for pointer list\n");
      return NDB_MSG_NOMEMORY;
   }


   for (i = 0; i < maxptr; i++)
   {
      retval = ndb_set_ptr_i (i, NULL, PTR_NONE, 0, "");
   }

   highest_ptr_no = 0;
   return NDB_MSG_OK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_alloc_done
*
* Description  : Tests pointer list for forgotten free()
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_alloc_done ();
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_alloc_done (void)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long retval;
   long index;
   long nr = 0;
   void *ptr;
   short status;
   size_t size;
   char remark[REMARK_LEN];

   char headline[] = "\nPointer History - forgotten deallocations\n";

   if (ndb_getDebugLevel() >= 7)
   {
      for (index = 0; index < highest_ptr_no; index++)
      {
         retval = ndb_get_ptr_i (index, &ptr, &status, &size, remark);
         if (status == PTR_VALID)
         {
            if (strlen (headline) > 0)
            {
               fprintf (stderr, headline);
               strcpy (headline, "");
            }
            fprintf (stderr, "%4ld. pointer 0x%08lx, size %6lu bytes, remark  \"%s\"\n",
                             nr++, (ULONG) ptr, (ULONG) size, remark);
            fflush(stderr);
         }
      }
   }

   return NDB_MSG_OK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_malloc
*
* Description  : replaces malloc()
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : ptr = ndb_malloc (100);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

void *
ndb_malloc (size_t i_size, char *i_remark)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return malloc(i_size);

#else

   void *ptr_malloc;
   long retval;

   if (ndb_getDebugLevel() >= 7)
      fprintf (stderr, "trying to malloc %lu bytes for '%s'\n", (ULONG) i_size, i_remark);
   ptr_malloc = calloc (i_size + LASTELEMSIZE, 1);
   if (ndb_getDebugLevel() >= 8)
      fprintf (stderr, "malloc:  0x%08lx\n", (ULONG) ptr_malloc);
   if (ptr_malloc != NULL)
   {
      retval = ndb_set_mark ((void *) ptr_malloc, i_size);
      retval = ndb_add_ptr (ptr_malloc, ptr_malloc, i_size, i_remark);
      // a little bit statistics
      ndb_alloc_size_curr += i_size;
      ndb_alloc_count_curr++;
      if (ndb_alloc_size_curr > ndb_alloc_size_max) ndb_alloc_size_max = ndb_alloc_size_curr;
      if (ndb_alloc_count_curr > ndb_alloc_count_max) ndb_alloc_count_max = ndb_alloc_count_curr;
   }
   if (ndb_getDebugLevel() >= 8) fprintf (stderr, "-------------------------------------------------------------\n");
   fflush(stderr);

   return ptr_malloc;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_calloc
*
* Description  : replaces calloc()
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_calloc (nr, size);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

void *
ndb_calloc (size_t i_nr, size_t i_size, char *i_remark)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return calloc(i_nr, i_size);

#else

   void *ptr_calloc;
   long retval;

   if (ndb_getDebugLevel() >= 7)
       fprintf (stderr, "trying to calloc %lu bytes for '%s'\n", (ULONG) (i_nr * i_size), i_remark);
   ptr_calloc = calloc ((i_nr * i_size) + LASTELEMSIZE, 1);
   if (ndb_getDebugLevel() >= 8)
    fprintf (stderr, "calloc:  0x%08lx\n", (ULONG) ptr_calloc);
   if (ptr_calloc != NULL)
   {
      retval = ndb_set_mark ((void *) ptr_calloc, (i_nr * i_size));
      retval = ndb_add_ptr (ptr_calloc, ptr_calloc, (i_nr * i_size), i_remark);
      // a little bit statistics
      ndb_alloc_size_curr += i_nr * i_size;
      ndb_alloc_count_curr++;
      if (ndb_alloc_size_curr > ndb_alloc_size_max) ndb_alloc_size_max = ndb_alloc_size_curr;
      if (ndb_alloc_count_curr > ndb_alloc_count_max) ndb_alloc_count_max = ndb_alloc_count_curr;
   }
   if (ndb_getDebugLevel() >= 8)
       fprintf (stderr, "-------------------------------------------------------------\n");
   fflush(stderr);

   return ptr_calloc;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_realloc
*
* Description  : replaces realloc()
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : ptr = ndb_realloc (ptr, newsize);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

void *
ndb_realloc (void *m_ptr, size_t i_size, char *i_remark)
{

#if defined(NDB_DEBUG_ALLOC_OFF)

    return realloc (m_ptr, i_size);

#else

/*@@EOFH@@*/
   void *ptr_realloc = NULL;
   long retval;
   long index = 0;
   size_t size = 0;
   char remark[REMARK_LEN];

   if (ndb_getDebugLevel() >= 7)
     fprintf (stderr, "trying to realloc %lu bytes (%08lx) for '%s'\n", (ULONG) i_size, (ULONG) m_ptr, i_remark);
   retval = ndb_get_ptr_ps (m_ptr, PTR_VALID, &index, &size, remark);
   if (ndb_getDebugLevel() >= 8)
     fprintf (stderr, "old pointer (%s) 0x%08lx(%lu) \"%s\"\n",
                               "ALLOC", (ULONG) m_ptr, (ULONG) size, remark);
   if (retval == NDB_MSG_OK)
   {
      if (ndb_getDebugLevel() >= 8)
        fprintf (stderr, "vor realloc:  0x%08lx, %lu\n", (ULONG) m_ptr, (ULONG) (i_size + LASTELEMSIZE));
      ptr_realloc = realloc (m_ptr, i_size + LASTELEMSIZE);
      if (ndb_getDebugLevel() >= 8)
        fprintf (stderr, "realloc:  0x%08lx\n", (ULONG) ptr_realloc);
      // a little bit statistics
      ndb_alloc_size_curr += i_size - size;
      if (ndb_alloc_size_curr > ndb_alloc_size_max) ndb_alloc_size_max = ndb_alloc_size_curr;
      if (ndb_alloc_count_curr > ndb_alloc_count_max) ndb_alloc_count_max = ndb_alloc_count_curr;
      /* clear old entry */
      retval = ndb_set_ptr_i (index, m_ptr, PTR_CLEAR, size, remark);
      if (ptr_realloc != NULL)
      {
         /* replace by new entry */
         retval = ndb_set_mark ((void *) ptr_realloc, i_size);
         retval = ndb_set_ptr_i (index, ptr_realloc, PTR_VALID, i_size, i_remark);
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "new pointer (%s) 0x%08lx(%lu) \"%s\"\n",
                                     "ALLOC", (ULONG) ptr_realloc, (ULONG) i_size, i_remark);
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "-------------------------------------------------------------\n");
      }
      else
      {
         retval = ndb_set_ptr_i (index, NULL, PTR_CLEAR, i_size, i_remark);
      }
      /* return pointer or null */
      return ptr_realloc;
   }
   /* pointer not allocated => re-allocate not possible */
   else
   {
      fprintf (stderr, "\nPointer History - error detected\n");
      fprintf (stderr, "\nrealloc to an unknown or deallocated pointer . . .\n\n");
      ndb_show_ptr ();
      fprintf (stderr, "\n\n--> stopping program with stack dump <---\n\n\n");
      ndb_crash ();
   }
   fflush(stderr);

   return ptr_realloc;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_free
*
* Description  : replaces free()
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_free ();
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_free (void *m_ptr)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   free(m_ptr);
   return NDB_MSG_OK;

#else

   long chk_status;
   long retval;
   long index;
   void *ptr;
   short o_status;
   size_t o_size;
   char o_remark[REMARK_LEN];

   chk_status = ndb_chk_ptr (m_ptr, &index);
   retval = ndb_get_ptr_i (index, &ptr, &o_status, &o_size, o_remark);

   if (chk_status != NDB_MSG_OK)
   {
      fprintf (stderr, "\nPointer History - error detected\n");
      fprintf (stderr, "should free pointer (%8lx, \"%s\") --> status %ld\n", (ULONG) m_ptr, o_remark, chk_status);
      if (chk_status != NDB_MSG_PTRNOTVALID)
      {
         ndb_show_ptr ();
         fprintf (stderr, "\n\n--> stopping program with stack dump <---\n\n\n");
         ndb_crash ();
      }
   }
   else
   {
      retval = ndb_free_ptr (m_ptr);
      if (ndb_getDebugLevel() >= 7)
         fprintf (stderr, "freeing %lu bytes for '%s'\n", (ULONG) o_size, o_remark);
      // a little bit statistics
      ndb_alloc_size_curr -= o_size;
      ndb_alloc_count_curr--;
   }
   if (ndb_getDebugLevel() >= 8) fprintf (stderr, "-------------------------------------------------------------\n");

   return NDB_MSG_OK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_set_mark
*
* Description  : Adds magic number as last element of allocated space
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_set_mark (ptr, size);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_set_mark (void *i_ptr_start, size_t i_size)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long *ptr_lastelem;

   ptr_lastelem = (long *) &(((char *) i_ptr_start) [i_size]);
   *ptr_lastelem = LASTELEM;

   return NDB_MSG_OK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_chk_mark
*
* Description  : Checks last element of allocated space not to be overwritten
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_chk_mark (ptr, size);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_chk_mark (void *i_ptr_start, size_t i_size)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long *ptr_lastelem;

   ptr_lastelem = (long *) ( ((char *) i_ptr_start) + i_size);

   if ((*ptr_lastelem) == LASTELEM)
      return NDB_MSG_OK;
   else
      return NDB_MSG_NOTOK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_get_ptr_p
*
* Description  : Look for index, status, size and remark of given pointer.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success, NDB_MSG_NOPTRFOUND if pointer unknown
*
* Example      : retval = ndb_get_ptr_p (ptr, &index, &status, &size, &remark);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_get_ptr_p (void *i_ptr, long *o_index, short *o_status, size_t *o_size, char *o_remark)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long index;

   for (index = 0; index < highest_ptr_no; index++)
   {
      if (i_ptr == ndb_allptr [index].ptr)
      {
         /* pointer found, now get its data */
         *o_index = index;
         *o_status = ndb_allptr [index].status;
         *o_size = ndb_allptr [index].size;
         strncpy (o_remark, ndb_allptr [index].remark, REMARK_LEN - 1);
         o_remark [REMARK_LEN-1] = '\0';
         return NDB_MSG_OK;
      }
   }
   /* pointer was not found */
   *o_index = 0;
   *o_status = PTR_NONE;
   *o_size = 0;
   strcpy (o_remark, "");

   return NDB_MSG_NOPTRFOUND;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_get_ptr_ps
*
* Description  : Look for index, size, remark of given pointer & status.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success, NDB_MSG_NOPTRFOUND if pointer unknown
*
* Example      : retval = ndb_get_ptr_ps (ptr, PTR_VALID, &index, &size, remark);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_get_ptr_ps (void *i_ptr, short i_status, long *o_index, size_t *o_size, char *o_remark)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long index;

   for (index = 0; index < highest_ptr_no; index++)
   {
      if ((i_ptr == ndb_allptr [index].ptr) &&
          (i_status == ndb_allptr [index].status))
      {
         /* pointer found, now get its data */
         *o_index = index;
         *o_size = ndb_allptr [index].size;
         strncpy (o_remark, ndb_allptr [index].remark, REMARK_LEN - 1);
         o_remark [REMARK_LEN-1] = '\0';
         return NDB_MSG_OK;
      }
   }
   /* pointer was not found */
   *o_index = 0;
   *o_size  = 0;
   strcpy (o_remark, "");

   return NDB_MSG_NOPTRFOUND;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_get_ptr_i
*
* Description  : Look for ptr, status, size and remark of given index.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success, NDB_MSG_NOPTRFOUND if pointer unknown
*
* Example      : retval = ndb_get_ptr_i (index, &ptr, &status, &size, &remark);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_get_ptr_i (long i_index, void **o_ptr, short *o_status, size_t *o_size, char *o_remark)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else


   if ((ndb_allptr [i_index].status == PTR_VALID) ||
       (ndb_allptr [i_index].status == PTR_CLEAR))
   {
      /* get pointer data at given index */
      *o_ptr                  = ndb_allptr [i_index].ptr;
      *o_status               = ndb_allptr [i_index].status;
      *o_size                 = ndb_allptr [i_index].size;
      strncpy (o_remark,        ndb_allptr [i_index].remark, REMARK_LEN - 1);
      o_remark [REMARK_LEN-1] = '\0';
      return NDB_MSG_OK;
   }
   else
   {
      /* pointer was not found */
      *o_ptr = NULL;
      *o_status = PTR_NONE;
      *o_size = 0;
      strcpy (o_remark, "");
      return NDB_MSG_NOPTRFOUND;
   }
   return NDB_MSG_ILLEGALRETURN;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_set_ptr_i
*
* Description  : Set all pointer data at given index.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success, NDB_MSG_NOPTRFOUND if pointer unknown
*
* Example      : retval = ndb_set_ptr_i (index, ptr, status, size, remark);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_set_ptr_i (long i_index, void *i_ptr, short i_status, size_t i_size, char *i_remark)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else


   if (i_index < maxptr)
   {
      /* set pointer data */
      ndb_allptr [i_index].ptr       = i_ptr;
      ndb_allptr [i_index].status    = i_status;
      ndb_allptr [i_index].size      = i_size;
      strncpy (ndb_allptr [i_index].remark, i_remark, REMARK_LEN - 1);
      ndb_allptr [i_index].remark [REMARK_LEN - 1] = '\0';
      return NDB_MSG_OK;
   }
   else
   {
      return NDB_MSG_NOTOK;
   }

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_add_ptr
*
* Description  : Adds given pointer.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_add_ptr ((void *)old, (void *) ptr, size, remark);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_add_ptr (void *old_ptr, void *i_ptr, size_t i_size, char *i_remark)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long retval;
   long maxptr_new;
   long o_index;
   short o_status;
   size_t o_size;
   char o_remark[REMARK_LEN];
   int i;

   /* to many entries in list ? */
   if (highest_ptr_no >= maxptr)
   {
        // increase pointer list by 1000
        maxptr_new = maxptr + 1000;
        ndb_allptr = realloc (ndb_allptr, maxptr_new * sizeof (struct ndb_ptrety_));
        if (ndb_allptr == NULL)
        {
          fprintf(stderr, "ndb_add_ptr: Error: couldn't reallocate memory for pointer list\n");
          return NDB_MSG_NOMEMORY;
        }

        for (i = maxptr; i < maxptr_new; i++)
        {
          retval = ndb_set_ptr_i (i, NULL, PTR_NONE, 0, "");
        }
        maxptr = maxptr_new;
   }

   if (ndb_getDebugLevel() >= 8) fprintf (stderr, "adding ptr -> ");

   /* look for pointer */
   retval = ndb_get_ptr_p (old_ptr, &o_index, &o_status, &o_size, o_remark);

   /* pointer found in list ? */
   if (retval == NDB_MSG_OK)
   {
      /* same memory adress, but old entry not deallocated ? */
      if (o_status == PTR_VALID)
      {
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "\n");
         fprintf (stderr, "Pointer History - double allocation of same memory . . .\n");
         fprintf (stderr, "free() instead of ndb_free()?\n");
         fprintf (stderr, "\nOld pointer  0x%08lx(%lu) \"%s\"\n", (ULONG) old_ptr, (ULONG) o_size, o_remark);
         fprintf (stderr, "New pointer  0x%08lx(%lu) \"%s\"\n", (ULONG) i_ptr, (ULONG) i_size, i_remark);
         retval = ndb_set_ptr_i (o_index, i_ptr, PTR_VALID, i_size, i_remark);
      }
     /* same memory adress, old one deallocated */
      else if (o_status == PTR_CLEAR)
      {
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "replacing ptr at index %ld\n", o_index);
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "old pointer  0x%08lx(%lu) \"%s\"\n", (ULONG) old_ptr, (ULONG) o_size, o_remark);
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "new pointer  0x%08lx(%lu) \"%s\"\n", (ULONG) i_ptr, (ULONG) i_size, i_remark);
         retval = ndb_set_ptr_i (o_index, i_ptr, PTR_VALID, i_size, i_remark);
      }
      /* status unknown -> there is something wrong */
      else
      {
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "\n");
         fprintf (stderr, "Unexpected status %d at index %ld\n", o_status, o_index);
         ndb_show_ptr ();
         fprintf (stderr, "\n\n--> stopping program with stack dump <---\n\n\n");
         ndb_crash ();
      }
   }
   /* pointer not in list -> create new entry */
   else
   {
      if (ndb_getDebugLevel() >= 8) fprintf (stderr, "creating ptr at index %li\n", highest_ptr_no);
      if (ndb_getDebugLevel() >= 8) fprintf (stderr, "new pointer  0x%08lx(%lu) \"%s\"\n", (ULONG) i_ptr, (ULONG) i_size, i_remark);
      retval = ndb_set_ptr_i (highest_ptr_no, i_ptr, PTR_VALID, i_size, i_remark);
      highest_ptr_no++;
   }

   return NDB_MSG_OK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_chk_ptr
*
* Description  : checks given pointer.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_chk_ptr ((void *) ptr, &o_size);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_chk_ptr (void *i_ptr, long *o_index)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long retval;
   short status;
   size_t size;
   char remark[REMARK_LEN];

   /* get data of pointer */
   retval = ndb_get_ptr_p (i_ptr, o_index, &status, &size, remark);

   if (retval == NDB_MSG_OK)
   {
      /* status of pointer? */
      if (status == PTR_VALID)
      {
         /* normal allocated pointer, check last element */
         retval = ndb_chk_mark (i_ptr, size);
         /* last element overwritten? */
         if (retval == NDB_MSG_OK)
            return NDB_MSG_OK;
         else
            return NDB_MSG_PTROVERWRITTEN;
      }
      /* pointer was set free before */
      else
      {
         return NDB_MSG_PTRNOTVALID;
      }
   }

   /* pointer not found in array */
   *o_index = 0;

   return NDB_MSG_NOPTRFOUND;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_free_ptr
*
* Description  : frees given pointer.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_free_ptr ((void *) ptr);
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_free_ptr (void *m_ptr)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long retval;
   long index;
   short status;
   size_t size;
   char remark[REMARK_LEN];

   /* get data of pointer */
   retval = ndb_get_ptr_p (m_ptr, &index, &status, &size, remark);

   if (retval == NDB_MSG_OK)
   {
      if (ndb_getDebugLevel() >= 8) fprintf (stderr, "old pointer (%s) 0x%08lx(%lu) \"%s\"\n",
                                  ((status == PTR_VALID)? "ALLOC" : "CLEAR" ),
                         (ULONG) m_ptr, (ULONG) size, remark);
      if (status == PTR_VALID)
      {
         if (ndb_getDebugLevel() >= 8) fprintf (stderr, "removing ptr at index %ld\n", index);
         /* reset its status */
         ndb_allptr[index].status = PTR_CLEAR;
         free (m_ptr);
      }
   }

   return NDB_MSG_OK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_show_ptr
*
* Description  : shows pointer history.
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : NDB_MSG_OK on success
*
* Example      : retval = ndb_show_ptr ();
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

long
ndb_show_ptr (void)
{
/*@@EOFH@@*/
#if defined(NDB_DEBUG_ALLOC_OFF)

   return NDB_MSG_OK;

#else

   long retval;
   long index;
   //short dummy;
   void *ptr;
   short status;
   size_t size;
   char remark[REMARK_LEN];
   char s_status[10];
   char s_overwritten[3];


   /* search for pointer in history */
   fprintf (stderr, "\nPointer History - output of history list\n\n");
   for (index = 0; index < highest_ptr_no; index++)
   {
      /* get data of pointer */
      retval = ndb_get_ptr_i (index, &ptr, &status, &size, remark);
      /* create description for pointer status */
      if (status == PTR_NONE)
      {
         strcpy (s_status, "-----");
      }
      else if (status == PTR_CLEAR)
      {
         strcpy (s_status, "clear");
      }
      else if (status == PTR_VALID)
      {
         strcpy (s_status, "alloc");
      }
      else
      {
         strcpy (s_status, "?????");
      }
      /* print out pointer data */
      fprintf (stderr, "%4ld: (%s) %08lx, %06lu ", index, s_status, (ULONG) ptr, (ULONG) size);
      fflush(stderr);
      strcpy (s_overwritten, "--");
      if (status == PTR_VALID)
      {
          if (ndb_chk_mark (ptr, size) == NDB_MSG_PTROVERWRITTEN)
             strcpy (s_overwritten, "!!");
          else
             strcpy (s_overwritten, "ok");
      }
      /* print out pointer data */
      fprintf (stderr, "%s \"%s\"\n", s_overwritten, remark);
      fflush(stderr);
   }
   fprintf (stderr, "\n");
   fflush(stderr);

   return NDB_MSG_OK;

#endif
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_show_status
*
* Description  : Shows current and maximal pointer count and memory allocated
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : -
*
* Example      : ndb_show_status ();
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

void
ndb_show_status (void)
{
/*@@EOFH@@*/

   fprintf (stderr, "\nMemory Allocation Status ------------------------------------\n");
   fprintf (stderr, "Current Count / Size:    %9lu / %10lu bytes\n", ndb_alloc_count_curr, ndb_alloc_size_curr);
   fprintf (stderr, "Maximal Count / Size:    %9lu / %10lu bytes\n", ndb_alloc_count_max,  ndb_alloc_size_max);
   fprintf (stderr, "-------------------------------------------------------------\n");
   fflush(stderr);

   return;
}


/*@@BOFH@@*/
/*Function*********************************************************************
*
* Function     : ndb_crash
*
* Description  : Crashes program to get the stack dump
* Precondition : -
* Postcondition: -
* DB Tables    : -
* Used form    : -
*
* Type         : i
*
* Returns      : -
*
* Example      : ndb_crash ();
*
******************************************************************************/
/* <type> <function> (<parameter1[,parameter2,...]>) */

void
ndb_crash (void)
{
/*@@EOFH@@*/
   long *ptr;

   ptr = NULL;
   *ptr = 0x7fffffff;
}


#ifdef MAIN

main ()
{
   long retval;
   char *test[20];
   long i;
   char remark[REMARK_LEN];

   retval = ndb_alloc_init (50);

   printf ("Test 1\n\n");
   for (i = 0; i < 20; i++)
   {
      snprintf (remark, REMARK_LEN - 1, "test1/%2i", i);
      test[i] = ndb_calloc (1, (size_t) (10 + 2 * i), remark);
   }

      ndb_show_ptr ();

   for (i = 0; i < 20; i++)
   {
      printf ("Free %2d: status %d\n", i, ndb_free (test[i]));
   }

      ndb_show_ptr ();
   test[5] = ndb_calloc (1, (size_t) (234), remark);
      ndb_show_ptr ();

   printf ("\n\nTest 2\n\n");
   for (i = 0; i < 20; i++)
   {
      snprintf (remark, REMARK_LEN - 1, "test2/5");
      test[5] = (char *) ndb_realloc (test[5], (size_t) (10 + 2 * i), remark);
   }

      ndb_show_ptr ();
   printf ("Free %2d: status %d\n", i, ndb_free (test[5]));
      ndb_show_ptr ();

   retval = ndb_alloc_done ();
}

#endif
