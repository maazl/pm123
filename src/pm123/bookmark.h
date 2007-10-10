/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 *
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PM123_BOOKMARK_H
#define PM123_BOOKMARK_H

#define INCL_WIN
#include <playable.h>
#include <os2.h>

/* Structure that contains information for records in
   the bookmarks container control. */

typedef struct _BMRECORD : public RECORDCORE
{
  char*       desc;     /* Name of the bookmark.     */
  char*       filename; /* Full path and file name.  */
  char*       time;     /* Displayed position time.  */
  ULONG       play_pos; /* Position.                 */

} BMRECORD, *PBMRECORD;

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the bookmarks presentation window. */
void bm_create( void );
/* Sets the visibility state of the bookmarks presentation window. */
void bm_show( BOOL show );
/* Destroys the bookmark presentation window. */
void bm_destroy( void );
/* get bookmark backend */
Playable* bm_get( void );


/* WARNING!! All functions returning a pointer to the
   bookmark record, return a NULL if suitable record is not found. */

/* Returns the pointer to the first bookmark record. */
//BMRECORD* bm_first_record( void );
/* Returns the pointer to the next bookmark record of specified. */
//BMRECORD* bm_next_record( BMRECORD* rec );
/* Returns the pointer to the bookmark record with the specified description. */
//BMRECORD* bm_find_record( const char* desc );
/* Returns the pointer to the first selected bookmark record. */
//BMRECORD* bm_first_selected( void );
/* Returns the pointer to the next selected bookmark record of specified. */
//BMRECORD* bm_next_selected( BMRECORD* rec );
/* Returns the pointer to the cursored bookmark record. */
//BMRECORD* bm_cursored( void );

/* Loads bookmarks from the file. */
//BOOL bm_load( HWND owner );
/* Saves bookmarks to the file. */
BOOL bm_save( HWND owner );
/* Adds a user selected bookmark. */
void bm_add_bookmark( HWND owner );

/* Bookmarks menu in the main pop-up menu */
void load_bookmark_menu( HWND hmenu );
BOOL process_possible_bookmark( USHORT cmd );

#ifdef __cplusplus
}
#endif
#endif /* PM123_BOOKMARK_H */
