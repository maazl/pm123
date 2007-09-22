/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef PM123_PLAYLIST_H
#define PM123_PLAYLIST_H

#define INCL_PM
#include <format.h>
#include <decoder_plug.h>
#include <os2.h>

/* Structure that contains information for records in
   the playlist container control. */

typedef struct _PLRECORD {

  RECORDCORE   rc;
  char*        songname;    /* Name of the song.              */
  char*        full;        /* Full path and file name.       */
  char*        size;        /* Size of the file.              */
  char*        time;        /* Play time of the file.         */
  char*        moreinfo;    /* Information about the song.    */
  int          played;      /* Is it already played file.     */
  BOOL         exist;       /* Is this file exist.            */
  DECODER_INFO2 info;       /* File info returned by decoder. */
  char         decoder[16]; /* Name of a decoder module.      */

} PLRECORD, *PPLRECORD;

/* The target window send this message to the a source windows
   after drag'n'drop operation if it is necessary to delete
   the moved records. */

#define WM_PM123_REMOVE_RECORD  ( WM_USER + 500 )

/* pl_load options */
#define PL_LOAD_CLEAR       0x0001
#define PL_LOAD_NOT_RECALL  0x0002

/* pl_save options */
#define PL_SAVE_PLS         0x0000
#define PL_SAVE_M3U         0x0001
#define PL_SAVE_RELATIVE    0x0002

/* pl_add_file options */
#define PL_ADD_SELECT       0x0001
#define PL_ADD_IF_EXIST     0x0002
#define PL_ADD_LOAD         0x0004

/* pl_add_directory options */
#define PL_DIR_RECURSIVE    0x0001

/* pl_sort controls */
#define PL_SORT_RAND        0x0001
#define PL_SORT_SIZE        0x0002
#define PL_SORT_TIME        0x0003
#define PL_SORT_NAME        0x0004
#define PL_SORT_SONG        0x0005

/* pl_clear options */
#define PL_CLR_NEW          0x0001

#ifdef __cplusplus
extern "C" {
#endif

/* The pointer to playlist record of the currently played file,
   the pointer is NULL if such record is not present. */
//extern PLRECORD* current_record;

/* Creates the playlist presentation window. */
HWND  pl_create( void );
/* Sets the visibility state of the playlist presentation window. */
void  pl_show( BOOL show );
/* Sets the title of the playlist window according to current playlist state. */
//void  pl_display_status( void );
/* Destroys the playlist presentation window. */
void  pl_destroy( void );

/* Returns a summary play time of the remained part of the playlist. */
//ULONG pl_playleft( void );
/* Returns a number of records in the playlist. */
//ULONG pl_size( void );
/* Returns a ordinal number of the currently loaded record. */
//ULONG pl_current_index( void );

/* Marks the currently loaded playlist record as currently played. */
//void  pl_mark_as_play( void );
/* Marks the currently loaded playlist record as currently stopped. */
//void  pl_mark_as_stop( void );
/* Removes "played" marks from all playlist records. */
//void  pl_clean_shuffle( void );

/* WARNING!! All functions returning a pointer to the
   playlist record, return a NULL if suitable record is not found. */

/* Queries a random playlist record for playing. */
//PLRECORD* pl_query_random_record( void );
/* Queries a first playlist record for playing. */
//PLRECORD* pl_query_first_record( void );
/* Queries a previous playlist record for playing. */
//PLRECORD* pl_query_prev_record( void );
/* Queries a next playlist record for playing. */
//PLRECORD* pl_query_next_record( void );
/* Queries a playlist record of the specified file for playing. */
//PLRECORD* pl_query_file_record( const char* filename );

/* Refreshes the specified playlist record. */
//void pl_refresh_record( PLRECORD* rec, USHORT flags );
/* Refreshes the playlist record of the specified file. */
//void pl_refresh_file( const char* filename );
/* Removes the specified playlist record. */
//void pl_remove_record( PLRECORD** array, USHORT count );

/* Returns true if the specified file is a playlist file. */
//BOOL is_playlist( const char *filename );
/* Loads the specified playlist file. */
BOOL pl_load( const char *filename, int options );
/* Saves playlist to the specified file. */
BOOL pl_save( const char* filename, int options );

/* Sends request about clearing of the playlist. */
//BOOL pl_clear( int options );
/* Sends request about addition of the file to the playlist. */
//BOOL pl_add_file( const char* filename, const char* title, int options );
/* Sends request about addition of the whole directory to the playlist. */
//BOOL pl_add_directory( const char* path, int options );
/* Notifies on completion of the playlist. */
//BOOL pl_completed( void );

/* Saves the playlist and player status to the specified file. */
BOOL pl_save_bundle( const char* filename, int options );
/* Loads the playlist and player status from specified file. */
BOOL pl_load_bundle( const char *filename, int options );

#ifdef __cplusplus
}
#endif
#endif /* PM123_PLAYLIST_H */

