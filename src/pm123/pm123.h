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

#ifndef  RC_INVOKED
#include <decoder_plug.h>
#include "skin.h"
#include "properties.h"
#include "copyright.h"
#include "plugman.h"
#endif

#define ICO_MAIN              1
#define ICO_MP3            1700
#define ICO_MP3PLAY        1701
#define ICO_MP3GRAY        1702

#define WIN_MAIN              1
#define HLP_MAIN              1
#define ACL_MAIN              1

#define MNU_MAIN            500
#define IDM_M_LOAD_MENU     501
#define IDM_M_LOAD_FILE     502
#define IDM_M_LOAD_URL      503
#define IDM_M_LOAD_TRACK    504
#define IDM_M_EDIT          505
#define IDM_M_BM_ADD        506
#define IDM_M_BM_EDIT       507
#define IDM_M_PLAYLIST      508
#define IDM_M_EQUALIZE      509
#define IDM_M_MANAGER       510
#define IDM_M_PROPERTIES    511
#define IDM_M_PLAYBACK      512
#define IDM_M_PLAY          513
#define IDM_M_PAUSE         514
#define IDM_M_FWD           515
#define IDM_M_REW           516
#define IDM_M_NEXT          517
#define IDM_M_PREV          518
#define IDM_M_VOL_RAISE     519
#define IDM_M_VOL_LOWER     520
#define IDM_M_SKINS         521
#define IDM_M_LOAD_SKIN     522
#define IDM_M_FONT_MENU     523
#define IDM_M_FONT_SET1     524
#define IDM_M_FONT_SET2     525
#define IDM_M_SIZE_MENU     526
#define IDM_M_SIZE_SMALL    527
#define IDM_M_SIZE_TINY     528
#define IDM_M_SIZE_NORMAL   529
#define IDM_M_SAVE_STREAM   530
#define IDM_M_FLOAT         531
#define IDM_M_QUIT          532
#define IDM_M_MINIMIZE      533
#define IDM_M_MENU          534
#define IDM_M_LOAD_DISC     535

#define IDM_M_LOAD_LAST   10000 /* A lot of IDs after this need to be free. */
#define IDM_M_BOOKMARKS   11000 /* A lot of IDs after this need to be free. */
#define IDM_M_PLUGINS     15000 /* A lot of IDs after this need to be free. */
#define IDM_M_DISCS       16000 /* A lot of IDs after this need to be free. */

#define DLG_URL            2014
#define ENT_URL             101

#define DLG_TRACK          2021
#define LB_TRACKS          2022
#define ST_DRIVE           2023
#define CB_DRIVE           2024
#define PB_REFRESH         2025

#define DLG_EQUALIZER      2015

#define HLP_MAIN_TABLE      100
#define HLP_NULL_TABLE      101

#define IDH_MAIN           1000
#define IDH_ADVANTAGES     1001
#define IDH_ANALYZER       1002
#define IDH_SUPPORT        1003
#define IDH_COPYRIGHT      1004
#define IDH_DRAG_AND_DROP  1005
#define IDH_EQUALIZER      1006
#define IDH_ID3_EDITOR     1007
#define IDH_INTERFACE      1008
#define IDH_MAIN_MENU      1009
#define IDH_MAIN_WINDOW    1010
#define IDH_PM             1011
#define IDH_NETSCAPE       1012
#define IDH_COMMANDLINE    1013
#define IDH_PL             1014
#define IDH_PROPERTIES     1015
#define IDH_REMOTE         1016
#define IDH_SKIN_GUIDE     1017
#define IDH_SKIN_UTILITY   1018
#define IDH_TROUBLES       1019

#define AMP_NOFILE            0
#define AMP_SINGLE            1
#define AMP_PLAYLIST          2

/* amp_load_singlefile options */
#define AMP_LOAD_NOT_PLAY    0x0001
#define AMP_LOAD_NOT_RECALL  0x0002

/* amp_load_url options */
#define URL_ADD_TO_PLAYER    0x0000
#define URL_ADD_TO_LIST      0x0001

/* amp_load_track options */
#define TRK_ADD_TO_PLAYER    0x0000
#define TRK_ADD_TO_LIST      0x0001

/* amp_invalidate options */
#define UPD_TIMERS           0x0001
#define UPD_FILEINFO         0x0002
#define UPD_VOLUME           0x0004
#define UPD_WINDOW           0x0008
#define UPD_FILENAME         0x0010
#define UPD_DELAYED          0x8000

#ifndef DC_PREPAREITEM
#define DC_PREPAREITEM  0x0040
#endif

/* The target window send this message to the a source windows
   after drag'n'drop operation if it is necessary to delete
   the moved records. */

#define WM_123FILE_REMOVE  ( WM_USER + 500 )

/* The target window send this message to the a source windows
   after drag'n'drop operation if it is necessary to load of
   the record. */

#define WM_123FILE_LOAD    ( WM_USER + 501 )

/* It is posted after successful saving of the meta information
   of the specified file. */

#define WM_123FILE_REFRESH ( WM_USER + 503 )

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

  USHORT cditem;    /* Count of dragged objects. */
  HWND   hwndItem;  /* Window handle of the source of the drag operation. */
  ULONG  ulItemID;  /* Information used by the source to identify the
                       object being dragged. */
} AMP_DROPINFO;

typedef struct {

  char*        filename;
  char         decoder[_MAX_MODULE_NAME];
  DECODER_INFO info;
  int          options;

} AMP_FILEINFO;

/* Returns the handle of the player window. */
HWND  amp_player_window( void );
/* Returns the anchor-block handle. */
HAB   amp_player_hab( void );
/* Marks the player window as needed of redrawing. */
void  amp_invalidate( int options );
/* Posts a command to the message queue associated with the player window. */
BOOL  amp_post_command( USHORT id );

/* Activates or deactivates the current playlist. */
BOOL  amp_pl_use( BOOL use );
/* Loads a standalone file or CD track to the player and
   plays it if this is specified in the player properties. */
BOOL  amp_load_singlefile( const char* filename, int options );
/* Begins playback of the currently loaded file from the specified position. */
BOOL  amp_play( int pos );
/* Stops playback of the currently played file. */
BOOL  amp_stop( void );
/* Stops playing and resets the player to its default state. */
BOOL  amp_reset( void );
/* Suspends or resumes playback of the currently played file. */
BOOL  amp_pause( void );

/* Adds URL to the playlist or load it to the player. */
BOOL  amp_load_url( HWND owner, int options );
/* Adds CD tracks to the playlist or load one to the player. */
BOOL  amp_load_track( HWND owner, int options );
/* Reads url from the specified file. */
char* amp_url_from_file( char* result, const char* filename, int size );
/* Extracts song title from the specified file name. */
char* amp_title_from_filename( char* result, const char* filename, int size );

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

int  DLLENTRY pm123_getstring    ( int type, int subtype, int size, char* buffer );
void DLLENTRY pm123_control      ( int type, void* param );
void DLLENTRY pm123_display_info ( char* );
void DLLENTRY pm123_display_error( char* );

/* Global variables */

extern int      amp_playmode; /* Play mode        */
extern HPOINTER mp3;          /* Song file icon   */
extern HPOINTER mp3play;      /* Played file icon */
extern HPOINTER mp3gray;      /* Broken file icon */
extern HMODULE  hmodule;

/* Contains startup path of the program without its name. */
extern char startpath[_MAX_PATH];
/* Contains a name of the currently loaded file. */
extern char current_filename[_MAX_PATH];
/* Contains a information about of the currently loaded file. */
extern DECODER_INFO current_info;
/* Other parameters of the currently loaded file. */
extern char current_decoder[_MAX_MODULE_NAME];

/* Equalizer stuff. */
extern float gains[20];
extern BOOL  mutes[20];
extern float preamp;

/* specana.c FFT functions */
int DLLENTRY specana_init( int setnumsamples );
int DLLENTRY specana_dobands( float* bands );

#ifdef __cplusplus
}
#endif
#endif /* PM123_H */
