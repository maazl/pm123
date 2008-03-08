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

#ifndef  RC_INVOKED
#include "format.h"
#include "decoder_plug.h"
#endif

#define DLG_PLAYLIST         42
#define CNR_PLAYLIST FID_CLIENT
#define ACL_PLAYLIST       8001

#define MNU_PLAYLIST        900
#define IDM_PL_USE          906
#define IDM_PL_ADD_MENU     985
#define IDM_PL_ADD_FILES    901
#define IDM_PL_ADD_TRACKS   972
#define IDM_PL_ADD_URL      965
#define IDM_PL_CLEAR        907
#define IDM_PL_SORT_MENU    990
#define IDM_PL_SORT_SIZE    903
#define IDM_PL_SORT_TIME    904
#define IDM_PL_SORT_FILE    905
#define IDM_PL_SORT_TRACK   991
#define IDM_PL_SORT_SONG    912
#define IDM_PL_SORT_RANDOM  970
#define IDM_PL_SAVE_LIST    908
#define IDM_PL_OPEN_MENU    989
#define IDM_PL_OPEN_LIST    986

#define MNU_RECORD          910
#define IDM_RC_PLAY         981
#define IDM_RC_REMOVE       982
#define IDM_RC_EDIT         983
#define IDM_RC_DELETE       987

#define IDM_PL_MENU         950
#define IDM_PL_MENU_RECORD  971

#define IDM_PL_LAST        1000 /* A lot of IDs after this need to be free. */

/* pl_load options */
#define PL_LOAD_CLEAR       0x0001
#define PL_LOAD_NOT_RECALL  0x0002
#define PL_LOAD_TO_PM       0x0004
#define PL_LOAD_UTF8        0x0008
#define PL_LOAD_CONTINUE    0x0010
#define PL_LOAD_NOT_QUEUE   0x0020

/* pl_save options */
#define PL_SAVE_LST         0x0000
#define PL_SAVE_M3U         0x0001
#define PL_SAVE_RELATIVE    0x0002
#define PL_SAVE_UTF8        0x0004

/* pl_add_file options */
#define PL_ADD_IF_EXIST     0x0001

/* pl_add_directory options */
#define PL_DIR_RECURSIVE    0x0001

/* pl_remove_records options */
#define PL_REMOVE_SELECTED  0x0001
#define PL_REMOVE_LOADED    0x0002
#define PL_DELETE_FILES     0x0004

#ifdef __cplusplus
extern "C" {
#endif

/* Prepares the first playlist record to playing. */
BOOL  pl_load_first_record( void );
/* Prepares the next playlist record to playing. */
BOOL  pl_load_next_record( void );
/* Prepares the previous playlist record to playing. */
BOOL  pl_load_prev_record( void );
/* Prepares the playlist record of the specified file to playing. */
BOOL  pl_load_file_record( const char* filename );
/* Removes the specified playlist records. */
void  pl_remove_records( int options );

/* Marks the currently loaded playlist record as currently played. */
void  pl_mark_as_play( void );
/* Marks the currently loaded playlist record as currently stopped. */
void  pl_mark_as_stop( void );
/* Removes "already played" marks from all playlist records. */
void  pl_clean_shuffle( void );

/* Returns a ordinal number of the currently loaded record. */
ULONG pl_loaded_index( void );
/* Returns a number of records in the playlist. */
ULONG pl_size( void );
/* Returns a summary play time of the remained part of the playlist. */
ULONG pl_playleft( void );

/* Refreshes the playlist records of the specified file. */
void pl_refresh_file( const char* filename, const DECODER_INFO* info );
/* Sets the songname for the specified file. */
void pl_refresh_songname( const char* filename, const char* songname );
/* Sets the title of the playlist window according to current playlist state. */
void pl_refresh_status( void );

/* Sends request about clearing of the playlist. */
BOOL pl_clear( void );
/* Sends request about addition of the file to the playlist. */
BOOL pl_add_file( const char* filename, const char* title, int options );
/* Sends request about addition of the whole directory to the playlist. */
BOOL pl_add_directory( const char* path, int options );
/* Notifies on completion of the playlist. */
BOOL pl_completed( void );
/* Sends request about loading the specified playlist file. */
BOOL pl_load( const char *filename, int options );
/* Saves playlist to the specified file. */
BOOL pl_save( const char* filename, int options );

/* Returns true if the specified file is a playlist file. */
BOOL is_playlist( const char *filename );

/* Saves the playlist and the player status to the specified file. */
BOOL pl_save_bundle( const char* filename, int options );
/* Loads the playlist and the player status from specified file. */
BOOL pl_load_bundle( const char *filename, int options );

/* Creates  the playlist presentation window. Must be called
   from the main thread. */
HWND pl_create( void );
/* Destroys the playlist presentation window. Must be called
   from the main thread. */
void pl_destroy( void );
/* Sets the visibility state of the playlist presentation window. */
void pl_show( BOOL show );
/* Returns the visibility state of the playlist presentation window. */
BOOL pl_is_visible( void );
/* Changes the playlist colors. */
BOOL pl_set_colors( ULONG, ULONG, ULONG, ULONG );

#ifdef __cplusplus
}
#endif
#endif /* PM123_PLAYLIST_H */
