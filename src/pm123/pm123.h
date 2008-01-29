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

#define INCL_WIN
#include <config.h>
#include <decoder_plug.h>
#include "properties.h"
#include "copyright.h"
#include <os2.h>

#ifdef __cplusplus
#include "playable.h"
#include <cpp/xstring.h>
extern "C" {


/* Constructs a string of the displayable text from the file information. */
xstring amp_construct_tag_string( const DECODER_INFO2* info );
#endif

/* Constructs a information text for currently loaded file. */
void  amp_display_filename( void );
/* Switches to the next text displaying mode. */
void  amp_display_next_mode( void );

void  DLLENTRY pm123_control( int index, void* param );

int   DLLENTRY pm123_getstring( int index, int subindex, size_t bufsize, char* buf );

/* Loads *anything* to player. */
void  amp_load_playable( const char *url, double start, int options );
/* amp_load_playable options */
#define AMP_LOAD_NOT_PLAY      0x0001 // Load a playable object, but do not start playback automatically
#define AMP_LOAD_NOT_RECALL    0x0002 // Load a playable object, but do not add an entry into the list of recent files
#define AMP_LOAD_KEEP_PLAYLIST 0x0004 // Play a playable object. If A playlist containing this item is loaded, the item is activated only.
#define AMP_LOAD_APPEND        0x0008 // Take a playable object as part of multiple playable objects to load. 

/* Marks the player window as needed of redrawing. */
void  amp_invalidate( int options );
/* amp_invalidate options */
#define UPD_TIMERS           0x0001
#define UPD_FILEINFO         0x0002
#define UPD_DELAYED          0x8000
#define UPD_ALL              0x7FFF

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

/* Edits a information for the specified file. */
void  amp_info_edit( HWND owner, const char* filename, const char* decoder );

/* Default dialog procedure for the file dialog. */
MRESULT EXPENTRY amp_file_dlg_proc( HWND, ULONG, MPARAM, MPARAM );
/* Wizzard function for the default entry "File..." */
ULONG DLLENTRY amp_file_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param );
/* Wizzard function for the default entry "URL..." */
ULONG DLLENTRY amp_url_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param );

BOOL  amp_load_eq_file( char* filename, float* gains, BOOL* mutes, float* preamp );

/* visualize errors from anywhere */
extern void DLLENTRY amp_display_info ( const char* );
extern void DLLENTRY amp_display_error( const char* );


#ifdef __cplusplus
}
/* Saves a playlist */
void  amp_save_playlist( HWND owner, PlayableCollection* playlist );

/* Reads url from specified file. [123_utils] */
xstring amp_url_from_file(const char* filename);
/* Reads an String from a drag and drop structure */
xstring amp_string_from_drghstr(HSTR hstr);

extern "C" {
#endif


/* Global variables */

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

