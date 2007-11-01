/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

#ifndef  PM123_H
#define  PM123_H

#include "skin.h"
#include "plugman.h"
#include "properties.h"
#include "copyright.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

  USHORT cditem;    /*  Count of dragged objects. */
  HWND   hwndItem;  /*  Window handle of the source of the drag operation. */
  ULONG  ulItemID;  /*  Information used by the source to identify the
                        object being dragged. */
} AMP_DROPINFO;

/* Converts time to two integer suitable for display by the timer. */
void  sec2num( double seconds, unsigned int* major, unsigned int* minor );
/* Reads url from specified file. */
char* amp_url_from_file( char* result, const char* filename, size_t size );

/* Constructs a string of the displayable text from the file information. */
char* amp_construct_tag_string( char* result, const DECODER_INFO2* info, int size );
/* Constructs a information text for currently loaded file. */
void  amp_display_filename( void );
/* Switches to the next text displaying mode. */
void  amp_display_next_mode( void );

/* Loads *anything* to player. */
BOOL  amp_load_playable( const char *url, int options );

/* Begins playback of the currently loaded file from the specified position. */
void  amp_play( float pos );
/* Stops playback of the currently played file. */
void  amp_stop( void );
/* Suspends or resumes playback of the currently played file. */
void  amp_pause( void );
/* Stops playing and resets the player to its default state. */
void  amp_reset( void );
/* Marks the player window as needed of redrawing. */
void  amp_invalidate( int options );

/* Posts a message to the player window. */
BOOL  amp_post_message( ULONG msg, MPARAM mp1, MPARAM mp2 );
/* Returns the handle of the player window. */
HWND  amp_player_window( void );
/* Returns the anchor-block handle. */
HAB   amp_player_hab( void );

/* Creates and displays a error message window. */
void  amp_error( HWND owner, const char* format, ... );
/* Creates and displays a error message window. */
void  amp_player_error( const char* format, ... );
/* Creates and displays a message window. */
void  amp_info ( HWND owner, const char* format, ... );
/* Requests the user about specified action. */
BOOL  amp_query( HWND owner, const char* format, ... );
/* Requests the user about overwriting a file. */
BOOL  amp_warn_if_overwrite( HWND owner, const char* filename );
/* Tells the help manager to display a specific help window. */
void  amp_show_help( SHORT resid );

/* Adds HTTP file to the playlist or load it to the player. */
//void  amp_add_url( HWND owner, int options );
/* Adds CD tracks to the playlist or load one to the player. */
/*void  amp_add_tracks( HWND owner, int options );*/
/* Adds user selected files or directory to the playlist. */
//void  amp_add_files( HWND owner );

/* Saves current playlist to the file specified by user. */
//void  amp_save_list_as( HWND owner, int options );
/* Loads a playlist selected by the user to the player. */
//void  amp_load_list( HWND owner );
/* Loads a file selected by the user to the player. */
//void  amp_load_file( HWND owner );

/* Edits a information for the specified file. */
void  amp_info_edit( HWND owner, const char* filename, const char* decoder );

/* Adjusts audio volume to level accordingly current playing mode. */
void  amp_volume_adjust( void );

/* Default dialog procedure for the file dialog. */
MRESULT EXPENTRY amp_file_dlg_proc( HWND, ULONG, MPARAM, MPARAM );
/* Wizzard function for the default entry "File..." */
ULONG DLLENTRY amp_file_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param );
/* Wizzard function for the default entry "URL..." */
ULONG DLLENTRY amp_url_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param );

BOOL  amp_load_eq_file( char* filename, float* gains, BOOL* mutes, float* preamp );

int  DLLENTRY pm123_getstring( int index, int subindex, size_t bufsize, char* buf );
void DLLENTRY pm123_control  ( int index, void* param );

/* visualize errors from anywhere */
extern void DLLENTRY amp_display_info ( const char* );
extern void DLLENTRY amp_display_error( const char* );


#ifdef __cplusplus
}
#include "playable.h"
/* TODO: there are bad threading issues here
   if the following function is not called from the main thread */
/* Returns a information block of the currently loaded file or NULL if none. */
Song*     amp_get_current_song();
Playable* amp_get_current_root(); 
extern "C" {
#endif


/* Global variables */

//extern int      amp_playmode; /* Play mode        */

/* Contains startup path of the program without its name. */
extern char startpath[_MAX_PATH];

/* Equalizer stuff. */
extern float gains[20];
extern BOOL  mutes[20];
extern float preamp;

#ifdef __cplusplus
}
#endif
#endif /* PM123_H */

