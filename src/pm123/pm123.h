/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef  RC_INVOKED
#include "plugman.h"
#include "tag.h"
#include "iniman.h"
#include "skin.h"
#include "playlist.h"
#include "properties.h"
#include "copyright.h"
#endif

#define ICO_MAIN              1
#define ICO_MP3            1700
#define ICO_MP3PLAY        1701
#define ICO_MP3GRAY        1702

#define WIN_MAIN              1
#define HLP_MAIN              1
#define ACL_MAIN              1

#define MNU_MAIN            500
#define IDM_M_LOAD          501
#define IDM_M_SHUFFLE       505
#define IDM_M_ABOUT         506
#define IDM_M_CFG           507
#define IDM_M_LOADFILE      508
#define IDM_M_PLAYLIST      509
#define IDM_M_MINIMIZE      510
#define IDM_M_SMALL         511
#define IDM_M_TINY          512
#define IDM_M_NORMAL        513
#define IDM_M_URL           514
#define IDM_M_FONT1         515
#define IDM_M_FONT2         516
#define IDM_M_FONT          517
#define IDM_M_SIZE          518
#define IDM_M_SKIN          519
#define IDM_M_SKINLOAD      520
#define IDM_M_FLOAT         521
#define IDM_M_EQUALIZE      522
#define IDM_M_TAG           523
#define IDM_M_MANAGER       540
#define IDM_M_TRACK         541
#define IDM_M_SAVE          542
#define IDM_M_ADDBOOK       545
#define IDM_M_EDITBOOK      546
#define IDM_M_PLAYBACK      547
#define IDM_M_VOL_RAISE     548
#define IDM_M_VOL_LOWER     549
#define IDM_M_MENU          550
#define IDM_M_LAST        10000 /* A lot of IDs after this need to be free. */
#define IDM_M_BOOKMARKS   11000 /* A lot of IDs after this need to be free. */
#define IDM_M_PLUG        15000 /* A lot of IDs after this need to be free. */

#define DLG_URL            2014
#define ENT_URL             101

#define DLG_TRACK          2021
#define LB_TRACKS          2022
#define ST_DRIVE           2023
#define CB_DRIVE           2024
#define PB_REFRESH         2025

#define DLG_ID3TAG         2022
#define NB_ID3TAG           100

#define DLG_ID3V10         2012
#define ST_ID3_TITLE        106
#define EN_ID3_TITLE        103
#define ST_ID3_ARTIST       107
#define EN_ID3_ARTIST       104
#define ST_ID3_ALBUM        108
#define EN_ID3_ALBUM        105
#define ST_ID3_COMMENT      110
#define EN_ID3_COMMENT      109
#define ST_ID3_GENRE        111
#define CB_ID3_GENRE        101
#define ST_ID3_YEAR         113
#define EN_ID3_YEAR         112
#define PB_ID3_UNDO         200

#define DLG_FILE           2100
#define CB_RECURSE          500
#define CB_RELATIV          501

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

#define MSG_PLAY              1
#define MSG_STOP              2
#define MSG_PAUSE             3
#define MSG_FWD               4
#define MSG_REW               5
#define MSG_JUMP              6
#define MSG_SAVE              7

#define AMP_NOFILE            0
#define AMP_SINGLE            1
#define AMP_PLAYLIST          2

/* amp_load_singlefile options */
#define AMP_LOAD_NOT_PLAY    0x0001
#define AMP_LOAD_NOT_RECALL  0x0002

/* amp_add_url options */
#define URL_ADD_TO_PLAYER    0x0000
#define URL_ADD_TO_LIST      0x0001

/* amp_add_tracks options */
#define TRK_ADD_TO_PLAYER    0x0000
#define TRK_ADD_TO_LIST      0x0001

/* amp_save_list_as options */
#define SAV_LST_PLAYLIST     0x0000
#define SAV_M3U_PLAYLIST     0x0001

/* amp_invalidate options */
#define UPD_TIMERS           0x0001
#define UPD_FILEINFO         0x0002
#define UPD_DELAYED          0x8000
#define UPD_ALL              0x7FFF

/* file dialog additional flags */
#define FDU_DIR_ENABLE       0x0001
#define FDU_RECURSEBTN       0x0002
#define FDU_RECURSE_ON       0x0004
#define FDU_RELATIVBTN       0x0008
#define FDU_RELATIV_ON       0x0010

/* file dialog standard types */
#define FDT_PLAYLIST         "Playlist files (*.LST;*.MPL;*.M3U;*.PLS)"
#define FDT_PLAYLIST_LST     "Playlist files (*.LST)"
#define FDT_PLAYLIST_M3U     "Playlist files (*.M3U)"
#define FDT_AUDIO            "All supported audio files ("
#define FDT_AUDIO_ALL        "All supported types (*.LST;*.MPL;*.M3U;*.PLS"
#define FDT_SKIN             "Skin files (*.SKN)"
#define FDT_EQUALIZER        "Equalizer presets (*.EQ)"
#define FDT_PLUGIN           "Plug-in (*.DLL)"

#if __cplusplus
extern "C" {
#endif

/* Converts time to two integer suitable for display by the timer. */
void  sec2num( long seconds, int* major, int* minor );

/* Reads ID3 tag from the specified file. */
BOOL  amp_gettag( const char* filename, DECODER_INFO* info, tune* tag );
/* Wipes ID3 tag from the specified file. */
BOOL  amp_wipetag( const char* filename );
/* Writes ID3 tag to the specified file. */
BOOL  amp_puttag( const char* filename, tune* tag );

/* Constructs a string of the displayable text from the ID3 tag. */
char* amp_construct_tag_string( char* result, const tune* tag );
/* Constructs a information text for currently loaded file. */
void  amp_display_filename( void );
/* Switches to the next text displaying mode. */
void  amp_display_next_mode( void );

/* Loads the specified playlist record into the player. */
BOOL  amp_pl_load_record( PLRECORD* );
/* Plays the specified playlist record. */
void  amp_pl_play_record( PLRECORD* );
/* Activates the current playlist. */
void  amp_pl_use( void );
/* Deactivates the current playlist. */
void  amp_pl_release( void );
/* Loads a standalone file or CD track to player. */
BOOL  amp_load_singlefile( const char *filename, int options );

/* Starts playing of the currently loaded file. */
void  amp_play( void );
/* Stops the player. */
void  amp_stop( void );
/* Pauses or continues playing of the currently loaded file. */
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
void  amp_add_url( HWND owner, int options );
/* Adds CD tracks to the playlist or load one to the player. */
void  amp_add_tracks( HWND owner, int options );
/* Adds user selected files or directory to the playlist. */
void  amp_add_files( HWND owner );

/* Saves current playlist to the file specified by user. */
void  amp_save_list_as( HWND owner, int options );
/* Loads a playlist selected by the user to the player. */
void  amp_load_list( HWND owner );
/* Loads a file selected by the user to the player. */
void  amp_load_file( HWND owner );

/* Edits a ID3 tag for the specified file. */
void  amp_id3_edit( HWND owner, const char* filename );
/* Wipes a ID3 tag for the specified file. */
void  amp_id3_wipe( HWND owner, const char* filename );

/* Sets audio volume to the current selected level. */
void  amp_volume_to_normal( void );
/* Sets audio volume to below current selected level. */
void  amp_volume_to_lower( void );

/* Default dialog procedure for the file dialog. */
MRESULT EXPENTRY amp_file_dlg_proc( HWND, ULONG, MPARAM, MPARAM );

BOOL  amp_load_eq_file( char* filename, float* gains, BOOL* mutes, float* preamp );
ULONG handle_dfi_error( ULONG rc, const char* file );

int  PM123_ENTRY pm123_getstring(int index, int subindex, size_t bufsize, char *buf);
void PM123_ENTRY pm123_control(int index, void *param);

void PM123_ENTRY keep_last_error( char *error );
void PM123_ENTRY display_info( char *info );


/* Global variables */
/* -----------------*/

extern int      amp_playmode; /* Play mode        */
extern HPOINTER mp3;          /* Song file icon   */
extern HPOINTER mp3play;      /* Played file icon */
extern HPOINTER mp3gray;      /* Broken file icon */

/* Contains startup path of the program without its name. */
extern char startpath[_MAX_PATH];
/* Contains a name of the currently loaded file. */
extern char current_filename[_MAX_PATH];
/* Other parameters of the currently loaded file. */
extern int  current_bitrate;
extern int  current_channels;
extern int  current_length;
extern int  current_freq;
extern char current_track;
extern char current_cd_drive[4];
extern tune current_tune;
extern char current_decoder[128];
extern char current_decoder_info_string[128];

/* Equalizer stuff. */
extern float gains[20];
extern BOOL  mutes[20];
extern float preamp;

/* 123_msg.c */
typedef struct
{
   char *filename;
   char *out_filename;
   char *drive;
   char track;
   HWND hMain;
   char *decoder_needed;

} MSG_PLAY_STRUCT;

void amp_msg( int msg, void* param, void* param2 );
void equalize_sound( float* gains, BOOL* mute, float preamp, BOOL enabled );

#if __cplusplus
}
#endif

