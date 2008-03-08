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

#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_WINSTDDRAG
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <math.h>
#include <float.h>
#include <process.h>
#include <direct.h>
#include <io.h>

#include <utilfct.h>
#include <snprintf.h>

#include "pm123.h"
#include "bookmark.h"
#include "button95.h"
#include "pfreq.h"
#include "docking.h"
#include "iniman.h"
#include "messages.h"
#include "debuglog.h"
#include "assertions.h"
#include "playlist.h"
#include "tags.h"

#define  AMP_REFRESH_CONTROLS   ( WM_USER + 1000 ) /* 0,         0                            */
#define  AMP_PAINT              ( WM_USER + 1001 ) /* options,   0                            */
#define  AMP_PB_STOP            ( WM_USER + 1002 ) /* 0,         0                            */
#define  AMP_PB_PLAY            ( WM_USER + 1003 ) /* pos,       0                            */
#define  AMP_PB_RESET           ( WM_USER + 1004 ) /* 0,         0                            */
#define  AMP_PB_PAUSE           ( WM_USER + 1005 ) /* 0,         0                            */
#define  AMP_PB_LOAD_SINGLEFILE ( WM_USER + 1006 ) /* filename,  options                      */
#define  AMP_PB_LOAD_URL        ( WM_USER + 1007 ) /* hwnd,      options                      */
#define  AMP_PB_LOAD_TRACK      ( WM_USER + 1008 ) /* hwnd,      options                      */
#define  AMP_PB_USE             ( WM_USER + 1009 ) /* use,       0                            */
#define  AMP_PB_VOLUME          ( WM_USER + 1010 ) /* volume,    0                            */
#define  AMP_PB_SEEK            ( WM_USER + 1011 ) /* volume,    0                            */
#define  AMP_DISPLAY_MESSAGE    ( WM_USER + 1013 ) /* message,   TRUE (info) or FALSE (error) */
#define  AMP_DISPLAY_MODE       ( WM_USER + 1014 ) /* 0,         0                            */
#define  AMP_QUERY_STRING       ( WM_USER + 1015 ) /* buffer,    size and type                */

#define  TID_UPDATE_TIMERS      ( TID_USERMAX - 1 )
#define  TID_UPDATE_PLAYER      ( TID_USERMAX - 2 )
#define  TID_ONTOP              ( TID_USERMAX - 3 )

int       amp_playmode = AMP_NOFILE;
HPOINTER  mp3;      /* Song file icon   */
HPOINTER  mp3play;  /* Played file icon */
HPOINTER  mp3gray;  /* Broken file icon */
HMODULE   hmodule;

/* Contains startup path of the program without its name.  */
char startpath[_MAX_PATH];
/* Contains a name of the currently loaded file. */
char current_filename[_MAX_PATH];
/* Contains a information about of the currently loaded file. */
DECODER_INFO current_info;
/* Other parameters of the currently loaded file. */
char current_decoder[_MAX_MODULE_NAME];

static HAB   hab       = NULLHANDLE;
static HWND  heq       = NULLHANDLE;
static HWND  hframe    = NULLHANDLE;
static HWND  hplayer   = NULLHANDLE;
static HWND  hhelp     = NULLHANDLE;
static HWND  hmenu     = NULLHANDLE;
static HPIPE hpipe     = NULLHANDLE;

static TID   pipe_tid  = -1;

/* Pipe name decided on startup. */
static char  pipename[_MAX_PATH] = "\\PIPE\\PM123";

static BOOL  is_have_focus   = FALSE;
static BOOL  is_volume_drag  = FALSE;
static BOOL  is_seeking      = FALSE;
static BOOL  is_slider_drag  = FALSE;
static BOOL  is_arg_shuffle  = FALSE;
static BOOL  is_terminate    = FALSE;

/* Current seeking time. Valid if is_seeking == TRUE. */
static int   seeking_pos = 0;
static int   upd_options = 0;

/* Equalizer stuff. */
float gains[20];
BOOL  mutes[20];
float preamp;

void DLLENTRY
pm123_control( int type, void* param )
{
  switch( type )
  {
    case CONTROL_NEXTMODE:
      WinSendMsg( hplayer, AMP_DISPLAY_MODE, 0, 0 );
      break;
  }
}

int DLLENTRY
pm123_getstring( int type, int subtype, int size, char* buffer )
{
  WinSendMsg( hplayer, AMP_QUERY_STRING,
              MPFROMP( buffer ), MPFROM2SHORT( size, type ));
  return 0;
}

void DLLENTRY
pm123_display_info( char* info )
{
  char* message = strdup( info );
  if( message ) {
    WinPostMsg( hplayer, AMP_DISPLAY_MESSAGE, MPFROMP( message ), MPFROMLONG( FALSE ));
  }
}

void DLLENTRY
pm123_display_error( char* info )
{
  char* message = strdup( info );
  if( message ) {
    WinPostMsg( hplayer, AMP_DISPLAY_MESSAGE, MPFROMP( message ), MPFROMLONG( TRUE  ));
  }
}

/* Adjusts audio volume to level accordingly current playing mode.
   Must be called from the main thread. */
static void
amp_pb_volume_adjust( void )
{
  int volume = truncate( cfg.defaultvol, 0, 100 );
  ASSERT_IS_MAIN_THREAD;

  if( is_forward() || is_rewind()) {
    out_set_volume( volume * 0.6 );
  } else {
    out_set_volume( volume );
  }
}

/* Sets the audio volume to the specified level.
   Must be called from the main thread. */
static BOOL
amp_pb_set_volume( int volume )
{
  ASSERT_IS_MAIN_THREAD;

  cfg.defaultvol = truncate( volume, 0, 100 );
  amp_pb_volume_adjust();
  amp_invalidate( UPD_VOLUME );
  return TRUE;
}

/* Constructs a string of the displayable text from the file information.
   Must be called from the main thread. */
static char*
amp_pb_construct_tag_string( char* result, const DECODER_INFO* info, const char* filename, int size )
{
  ASSERT_IS_MAIN_THREAD;

  *result = 0;

  if( *info->artist ) {
    strlcat( result, info->artist, size );
    strlcat( result, ": ", size );
  }

  if( *info->title ) {
    strlcat( result, info->title, size );
  } else {
    char songname[_MAX_PATH];
    amp_title_from_filename( songname, filename, sizeof( songname ));
    strlcat( result, songname, size );
  }

  if( *info->album && *info->year )
  {
    strlcat( result, " (", size );
    strlcat( result, info->album, size );
    strlcat( result, ", ", size );
    strlcat( result, info->year,  size );
    strlcat( result, ")",  size );
  }
  else
  {
    if( *info->album && !*info->year )
    {
      strlcat( result, " (", size );
      strlcat( result, info->album, size );
      strlcat( result, ")",  size );
    }
    if( !*info->album && *info->year )
    {
      strlcat( result, " (", size );
      strlcat( result, info->year, size );
      strlcat( result, ")",  size );
    }
  }

  if( *info->comment )
  {
    strlcat( result, " -- ", size );
    strlcat( result, info->comment, size );
  }

  return control_strip( result );
}

/* Constructs a information text for currently loaded file and selects
   it for displaying. Must be called from the main thread. */
static void
amp_pb_display_filename( void )
{
  char display[ 512 ];
  ASSERT_IS_MAIN_THREAD;

  if( amp_playmode == AMP_NOFILE ) {
    bmp_set_text( "No file loaded" );
    return;
  }

  switch( cfg.viewmode )
  {
    case CFG_DISP_ID3TAG:
      amp_pb_construct_tag_string( display, &current_info, current_filename, sizeof( display ));

      if( *display ) {
        bmp_set_text( display );
        break;
      }

      // if tag is empty - use filename instead of it.

    case CFG_DISP_FILENAME:
      if( *current_filename ) {
        if( is_url( current_filename ))
        {
          char buff[_MAX_PATH];
          scheme( buff, current_filename, sizeof( buff ));

          if( strchr( buff, ':' ) != 0 ) {
             *strchr( buff, ':' )  = 0;
          }

          strlcpy( display, "[" , sizeof( display ));
          strlcat( display, buff, sizeof( display ));
          strlcat( display, "] ", sizeof( display ));
          strlcat( display, sdname( buff, current_filename, sizeof( buff )), sizeof( display ));
        } else {
          sdname ( display, current_filename, sizeof( display ));
        }
        bmp_set_text( display );
      } else {
        bmp_set_text( "This is a bug!" );
      }
      break;

    case CFG_DISP_FILEINFO:
      bmp_set_text( current_info.tech_info );
      break;
  }
}

/* Draws all player timers and the position slider.
   Must be called from the main thread. */
static void
amp_pb_paint_timers( HPS hps )
{
  int play_time = 0;
  int play_left = current_info.songlength / 1000;
  int list_left = 0;

  ASSERT_IS_MAIN_THREAD;

  if( cfg.mode == CFG_MODE_REGULAR )
  {
    if( decoder_playing()) {
      if( !is_seeking ) {
        play_time = time_played();
      } else {
        play_time = seeking_pos;
      }
    }

    if( play_left > 0 ) {
      play_left -= play_time;
    }

    if( amp_playmode == AMP_PLAYLIST && !cfg.rpt ) {
      list_left = pl_playleft() - play_time;
    }

    bmp_draw_slider( hps, play_time, time_total());
    bmp_draw_timer ( hps, play_time );

    bmp_draw_tiny_timer( hps, POS_TIME_LEFT, play_left );
    bmp_draw_tiny_timer( hps, POS_PL_LEFT,   list_left );
  }
}

/* Draws all attributes of the currently loaded file.
   Must be called from the main thread. */
static void
amp_pb_paint_fileinfo( HPS hps )
{
  ASSERT_IS_MAIN_THREAD;

  if( amp_playmode == AMP_PLAYLIST ) {
    bmp_draw_plind( hps, pl_loaded_index(), pl_size());
  } else {
    bmp_draw_plind( hps, 0, 0 );
  }

  bmp_draw_plmode  ( hps );
  bmp_draw_timeleft( hps );
  bmp_draw_rate    ( hps, current_info.bitrate );
  bmp_draw_channels( hps, current_info.mode );
  bmp_draw_text    ( hps );
}

/* Marks the player window as needed of redrawing. */
void
amp_invalidate( int options )
{
  if( options & UPD_WINDOW ) {
    WinInvalidateRect( hplayer, NULL, 1 );
    options &= ~UPD_WINDOW;
  }
  if( options & UPD_DELAYED || hplayer == NULLHANDLE ) {
    upd_options |= ( options & ~UPD_DELAYED );
  } else if( options ) {
    WinPostMsg( hplayer, AMP_PAINT, MPFROMLONG( options ), 0 );
  }
}

/* Returns the handle of the player window. */
HWND
amp_player_window( void ) {
  return hplayer;
}

/* Posts a command to the message queue associated with the player window. */
BOOL
amp_post_command( USHORT id ) {
  return WinPostMsg( hplayer, WM_COMMAND, MPFROMSHORT( id ), 0 );
}

/* Returns the anchor-block handle. */
HAB
amp_player_hab( void ) {
  return hab;
}

/* Begins playback of the currently loaded file from the specified
   position. Must be called from the main thread. */
static BOOL
amp_pb_play( int pos )
{
  char caption [_MAX_PATH];
  ASSERT_IS_MAIN_THREAD;

  if( amp_playmode == AMP_NOFILE ) {
    WinSendDlgItemMsg( hplayer, BMP_PLAY, WM_DEPRESS, 0, 0 );
    return FALSE;
  }

  WinSendDlgItemMsg( hplayer, BMP_FWD,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_REW,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );

  if( msg_play( hplayer, current_filename,
                         current_decoder, &current_info.format, pos ))
  {
    amp_pb_volume_adjust();
    WinSendDlgItemMsg( hplayer, BMP_PLAY, WM_SETHELP, MPFROMP( "Stops playback" ), 0 );

    if( amp_playmode == AMP_PLAYLIST ) {
      pl_mark_as_play();
    }

    sprintf( caption, "%s - ", AMP_FULLNAME );

    if( *current_info.title ) {
      strlcat( caption, current_info.title, sizeof( caption ));
    } else {
      char songname[_MAX_PATH];
      amp_title_from_filename( songname, current_filename, sizeof( songname ));
      strlcat( caption, songname, sizeof( caption ));
    }

    WinSetWindowText ( hframe, caption );
    WinSendDlgItemMsg( hplayer, BMP_PLAY, WM_PRESS  , 0, 0 );
  } else {
    WinSendDlgItemMsg( hplayer, BMP_PLAY, WM_DEPRESS, 0, 0 );
  }
  return TRUE;
}

/* Stops the playing of the current file. Must be called
   from the main thread. */
static BOOL
amp_pb_stop( void )
{
  QMSG qms;
  ASSERT_IS_MAIN_THREAD;

  msg_stop();
  WinSendDlgItemMsg( hplayer, BMP_PLAY,  WM_SETHELP, MPFROMP( "Starts playing" ), 0 );
  WinSendDlgItemMsg( hplayer, BMP_PLAY,  WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_FWD,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_REW,   WM_DEPRESS, 0, 0 );
  WinSetWindowText ( hframe,  AMP_FULLNAME );

  if( amp_playmode == AMP_PLAYLIST ) {
    pl_mark_as_stop();
  }

  while( WinPeekMsg( hab, &qms, hplayer, WM_PLAYSTOP, WM_PLAYSTOP, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_PLAYSTOP message.\n" ));
  }
  while( WinPeekMsg( hab, &qms, hplayer, WM_OUTPUT_OUTOFDATA, WM_OUTPUT_OUTOFDATA, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_OUTPUT_OUTOFDATA message.\n" ));
  }
  while( WinPeekMsg( hab, &qms, hplayer, WM_PLAYERROR, WM_PLAYERROR, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_PLAYERROR message.\n" ));
  }

  amp_invalidate( UPD_WINDOW );
  return TRUE;
}

/* Stops playing and resets the player to its default state.
   Must be called from the main thread. */
static BOOL
amp_pb_reset( void )
{
  ASSERT_IS_MAIN_THREAD;

  if( decoder_playing()) {
    amp_pb_stop();
  }

  amp_playmode = AMP_NOFILE;
  current_filename[0] = 0;
  current_decoder [0] = 0;

  memset( &current_info, 0, sizeof( current_info ));

  pl_refresh_status();
  amp_invalidate( UPD_WINDOW | UPD_FILENAME );
  return TRUE;
}

/* Suspends or resumes playback of the currently played file.
   Must be called from the main thread. */
static BOOL
amp_pb_pause( void )
{
  ASSERT_IS_MAIN_THREAD;

  if( decoder_playing())
  {
    msg_pause();

    if( is_paused()) {
      WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_PRESS, 0, 0 );
      return TRUE;
    }
  }

  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );
  return TRUE;
}

/* Activates or deactivates the current playlist. Must be
   called from the main thread. */
static BOOL
amp_pb_use( BOOL use )
{
  ASSERT_IS_MAIN_THREAD;

  if( use ) {
    if( pl_size()) {
      amp_playmode = AMP_PLAYLIST;
      pl_refresh_status();

      if( pl_load_file_record( current_filename )) {
        if( decoder_playing()) {
          pl_mark_as_play();
        } else if( cfg.playonuse ) {
          amp_pb_play( 0 );
        }
      } else {
        amp_pb_stop();

        if( pl_load_first_record() && cfg.playonuse ) {
          amp_pb_play( 0 );
        }
      }
    }
  } else {
    if( amp_playmode != AMP_SINGLE )
    {
      amp_playmode = AMP_SINGLE;
      pl_refresh_status();
      pl_mark_as_stop  ();
      pl_clean_shuffle ();
    }
  }

  amp_invalidate( UPD_FILEINFO | UPD_TIMERS );
  return TRUE;
}

/* Loads a standalone file or CD track to the player and
   plays it if this is specified in the player properties.
   Must be called from the main thread. */
static BOOL
amp_pb_load_singlefile( const char* filename, int options )
{
  int    i;
  ULONG  rc;
  char   module_name[_MAX_FNAME] = "";
  BOOL   decoder_was_playing = decoder_playing();
  struct stat fi;

  ASSERT_IS_MAIN_THREAD;

  if( is_playlist( filename )) {
    return pl_load( filename, PL_LOAD_CLEAR );
  }

  if( is_file( filename ) && stat( filename, &fi ) != 0 ) {
    amp_error( hplayer, "Unable load file:\n%s\n%s", filename, strerror(errno));
    return FALSE;
  }

  rc = dec_fileinfo((char*)filename, &current_info, module_name );

  if( rc != PLUGIN_OK )
  {
    amp_pb_reset();

    if( rc == PLUGIN_NO_READ ) {
      amp_error( hplayer, "The file %s could not be read.", filename );
    } else if( rc == PLUGIN_NO_PLAY ) {
      amp_error( hplayer, "The file %s cannot be played by PM123. "
                          "The file might be corrupted or the necessary plug-in is "
                          "not loaded or enabled.", filename );
    } else {
      amp_pb_stop();
      amp_error( hplayer, "%s: Error occurred: %s", filename, xio_strerror( rc ));
    }

    return FALSE;
  }

  if( decoder_was_playing ) {
    amp_pb_stop();
  }

  strcpy( current_filename, filename );
  strcpy( current_decoder, module_name );

  amp_pb_use( FALSE );
  amp_invalidate( UPD_FILENAME | UPD_FILEINFO );

  if( !( options & AMP_LOAD_NOT_PLAY )) {
    if( cfg.playonload || decoder_was_playing ) {
      amp_pb_play( 0 );
    }
  }

  if( !( options & AMP_LOAD_NOT_RECALL ) && !is_track( filename )) {
    for( i = 0; i < MAX_RECALL; i++ ) {
      if( stricmp( cfg.last[i], filename ) == 0 ) {
        while( ++i < MAX_RECALL ) {
          strcpy( cfg.last[i-1], cfg.last[i] );
        }
        break;
      }
    }

    for( i = MAX_RECALL - 2; i >= 0; i-- ) {
      strcpy( cfg.last[i + 1], cfg.last[i] );
    }

    strcpy( cfg.last[0], filename );
  }

  return TRUE;
}

/* Activates or deactivates the current playlist. */
BOOL
amp_pl_use( BOOL use ) {
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_USE, MPFROMBOOL( use ), 0 ));
}

/* Loads a standalone file or CD track to the player and
   plays it if this is specified in the player properties. */
BOOL
amp_load_singlefile( const char* filename, int options )
{
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_LOAD_SINGLEFILE,
                                 MPFROMP( filename ), MPFROMLONG( options )));
}

/* Begins playback of the currently loaded file from
   the specified position. */
BOOL
amp_play( int pos ) {
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_PLAY, MPFROMLONG( pos ), 0 ));
}

/* Stops playback of the currently played file. */
BOOL
amp_stop( void ) {
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_STOP, 0, 0 ));
}

/* Stops playing and resets the player to its default state. */
BOOL
amp_reset( void ) {
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_RESET, 0, 0 ));
}

/* Suspends or resumes playback of the currently played file. */
BOOL
amp_pause( void ) {
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_PAUSE, 0, 0 ));
}

/* Shows the context menu of the playlist. Must be called from
   the main thread. */
static void
amp_pb_show_context_menu( HWND parent )
{
  POINTL   pos;
  SWP      swp;
  char     file[_MAX_PATH];
  HWND     mh;
  MENUITEM mi;
  short    id;
  int      i;
  int      count;

  ASSERT_IS_MAIN_THREAD;

  if( hmenu == NULLHANDLE )
  {
    hmenu = WinLoadMenu( HWND_OBJECT, hmodule, MNU_MAIN );

    WinSendMsg( hmenu, MM_QUERYITEM,
                MPFROM2SHORT( IDM_M_LOAD_MENU, TRUE ), MPFROMP( &mi ));

    WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
      WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

    WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                                MPFROMLONG( IDM_M_LOAD_FILE ), 0 );
  }

  WinQueryPointerPos( HWND_DESKTOP, &pos );
  WinMapWindowPoints( HWND_DESKTOP, parent, &pos, 1 );

  if( WinWindowFromPoint( parent, &pos, TRUE ) == NULLHANDLE )
  {
    // The context menu is probably activated from the keyboard.
    WinQueryWindowPos( parent, &swp );
    pos.x = swp.cx / 2;
    pos.y = swp.cy / 2;
  }

  // Update regulars.
  WinSendMsg( hmenu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_LOAD_MENU, TRUE ), MPFROMP( &mi ));

  mh    = mi.hwndSubMenu;
  count = LONGFROMMR( WinSendMsg( mh, MM_QUERYITEMCOUNT, 0, 0 ));

  // Remove all items from the load menu except of three first
  // intended for a choice of object of loading.
  for( i = 3; i < count; i++ )
  {
    id = LONGFROMMR( WinSendMsg( mh, MM_ITEMIDFROMPOSITION, MPFROMSHORT(3), 0 ));
    WinSendMsg( mh, MM_DELETEITEM, MPFROM2SHORT( id, FALSE ), 0 );
  }

  if( *cfg.last[0] )
  {
    // Add separator.
    mi.iPosition = MIT_END;
    mi.afStyle = MIS_SEPARATOR;
    mi.afAttribute = 0;
    mi.id = 0;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem = 0;

    WinSendMsg( mh, MM_INSERTITEM, MPFROMP( &mi ), NULL );

    // Fill the recall list.
    for( i = 0; i < MAX_RECALL; i++ ) {
      if( *cfg.last[i] )
      {
        sprintf( file, "~%u ", i + 1 );

        if( is_url( cfg.last[i] ))
        {
          char buff[_MAX_PATH];

          scheme( buff, cfg.last[i], sizeof( buff ));

          if( strchr( buff, ':' ) != 0 ) {
             *strchr( buff, ':' )  = 0;
          }

          strlcat( file, "[" , sizeof( file ));
          strlcat( file, buff, sizeof( file ));
          strlcat( file, "] ", sizeof( file ));
          sdnameext( buff, cfg.last[i], sizeof( buff ));
          strlcat( file, buff, sizeof( file ));
        } else {
          sfnameext( file + strlen( file ), cfg.last[i], sizeof( file ) - strlen( file ) );
        }

        mi.iPosition = MIT_END;
        mi.afStyle = MIS_TEXT;
        mi.afAttribute = 0;
        mi.id = (IDM_M_LOAD_LAST + 1) + i;
        mi.hwndSubMenu = (HWND)NULLHANDLE;
        mi.hItem = 0;

        WinSendMsg( mh, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( file ));
      }
    }
  }

  // Update bookmarks.
  WinSendMsg( hmenu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_BOOKMARKS, TRUE ), MPFROMP( &mi ));

  bm_add_bookmarks_to_menu( mi.hwndSubMenu );

  // Update plug-ins.
  WinSendMsg( hmenu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_PLUGINS, TRUE ), MPFROMP( &mi ));

  pg_prepare_plugin_menu( mi.hwndSubMenu );

  // Update status
  mn_enable_item( hmenu, IDM_M_EDIT,       *current_filename );
  mn_enable_item( hmenu, IDM_M_SIZE_SMALL,  bmp_is_mode_supported( CFG_MODE_SMALL   ));
  mn_enable_item( hmenu, IDM_M_SIZE_NORMAL, bmp_is_mode_supported( CFG_MODE_REGULAR ));
  mn_enable_item( hmenu, IDM_M_SIZE_TINY,   bmp_is_mode_supported( CFG_MODE_TINY    ));
  mn_enable_item( hmenu, IDM_M_FONT_MENU,   cfg.font_skinned );
  mn_enable_item( hmenu, IDM_M_FONT_SET1,   bmp_is_font_supported( 0 ));
  mn_enable_item( hmenu, IDM_M_FONT_SET2,   bmp_is_font_supported( 1 ));
  mn_enable_item( hmenu, IDM_M_BM_ADD,      amp_playmode != AMP_NOFILE );

  mn_check_item( hmenu, IDM_M_FLOAT,        cfg.floatontop   );
  mn_check_item( hmenu, IDM_M_SAVE_STREAM,  is_stream_saved());
  mn_check_item( hmenu, IDM_M_FONT_SET1,    cfg.font == 0    );
  mn_check_item( hmenu, IDM_M_FONT_SET2,    cfg.font == 1    );
  mn_check_item( hmenu, IDM_M_SIZE_SMALL,   cfg.mode == CFG_MODE_SMALL   );
  mn_check_item( hmenu, IDM_M_SIZE_NORMAL,  cfg.mode == CFG_MODE_REGULAR );
  mn_check_item( hmenu, IDM_M_SIZE_TINY,    cfg.mode == CFG_MODE_TINY    );
  mn_check_item( hmenu, IDM_M_PAUSE,        is_paused ());
  mn_check_item( hmenu, IDM_M_FWD,          is_forward());
  mn_check_item( hmenu, IDM_M_REW,          is_rewind ());

  WinPopupMenu( parent, parent, hmenu, pos.x, pos.y, 0,
                PU_HCONSTRAIN   | PU_VCONSTRAIN |
                PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
}

/* Adds URL to the playlist or load it to the player.
   Must be called from the main thread. */
static BOOL
amp_pb_load_url( HWND owner, int options )
{
  char  url[512];
  HWND  hwnd;

  hwnd = WinLoadDlg( HWND_DESKTOP, owner, WinDefDlgProc, hmodule, DLG_URL, 0 );
  ASSERT_IS_MAIN_THREAD;

  if( hwnd == NULLHANDLE ) {
    return FALSE;
  }

  do_warpsans( hwnd );

  if( WinProcessDlg( hwnd ) == DID_OK ) {
    WinQueryDlgItemText( hwnd, ENT_URL, 1024, url );
    if( *url ) {
      if( options & URL_ADD_TO_LIST ) {
        if( is_playlist( url )) {
          pl_load( url, 0 );
        } else {
          pl_add_file( url, NULL, 0 );
        }
      } else {
        amp_pb_load_singlefile( url, 0 );
      }
    }
  }
  WinDestroyWindow( hwnd );
  return TRUE;
}

/* Adds URL to the playlist or load it to the player. */
BOOL
amp_load_url( HWND owner, int options ) {
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_LOAD_URL,
                                 MPFROMHWND( owner ), MPFROMLONG( options )));
}

/* Processes messages of the dialog of addition of CD tracks.
   Must be called from the main thread. */
static MRESULT EXPENTRY
amp_pb_load_track_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_DRIVE && SHORT2FROMMP(mp1) == CBN_EFCHANGE ) {
        WinPostMsg( hwnd, WM_COMMAND,
                    MPFROMSHORT( PB_REFRESH ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));
      }
      break;

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->cmd == PB_REFRESH )
      {
        char drive[3];
        char track[32];
        int  options = WinQueryWindowULong( hwnd, QWL_USER );
        int  i;

        DECODER_CDINFO cdinfo;

        WinQueryDlgItemText( hwnd, CB_DRIVE, sizeof( drive ), drive );
        WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_DELETEALL, 0, 0 );

        if( dec_cdinfo( drive, &cdinfo ) == 0 ) {
          if( cdinfo.firsttrack ) {
            for( i = cdinfo.firsttrack; i <= cdinfo.lasttrack; i++ ) {
              sprintf( track,"Track %02d", i );
              WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_INSERTITEM,
                                 MPFROMSHORT( LIT_END ), MPFROMP( track ));
            }
            if( options & TRK_ADD_TO_LIST ) {
              for( i = cdinfo.firsttrack; i <= cdinfo.lasttrack; i++ ) {
                WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_SELECTITEM,
                                   MPFROMSHORT( i - cdinfo.firsttrack ),
                                   MPFROMLONG( TRUE ));
              }
            } else {
              WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_SELECTITEM,
                                 MPFROMSHORT( 0 ), MPFROMLONG( TRUE ));
            }
          }
        } else {
          amp_error( hwnd, "Cannot find decoder that supports CD tracks." );
        }

        return 0;
      }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Adds CD tracks to the playlist or load one to the player.
   Must be called from the main thread. */
static BOOL
amp_pb_load_track( HWND owner, int options )
{
  HFILE hcdrom;
  ULONG action;
  HWND  hwnd;

  ASSERT_IS_MAIN_THREAD;
  hwnd = WinLoadDlg( HWND_DESKTOP, owner,
                     amp_pb_load_track_dlg_proc, hmodule, DLG_TRACK, 0 );

  if( hwnd == NULLHANDLE ) {
    return FALSE;
  }

  WinSetWindowULong( hwnd, QWL_USER, options );
  do_warpsans( hwnd );

  if( options & TRK_ADD_TO_LIST ) {
    WinSetWindowBits( WinWindowFromID( hwnd, LB_TRACKS ), QWL_STYLE, 1, LS_MULTIPLESEL );
    WinSetWindowBits( WinWindowFromID( hwnd, LB_TRACKS ), QWL_STYLE, 1, LS_EXTENDEDSEL );
    WinSetWindowText( hwnd, "Add Tracks" );
  } else {
    WinSetWindowText( hwnd, "Load Track" );
  }

  if( DosOpen( "\\DEV\\CD-ROM2$", &hcdrom, &action, 0,
               FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY, NULL ) == NO_ERROR )
  {
    struct {
      USHORT count;
      USHORT first;
    } cd_info;

    char  drive[3] = "X:";
    ULONG len = sizeof( cd_info );
    ULONG i;

    if( DosDevIOCtl( hcdrom, 0x82, 0x60, NULL, 0, NULL,
                             &cd_info, len, &len ) == NO_ERROR )
    {
      for( i = 0; i < cd_info.count; i++ ) {
        drive[0] = 'A' + cd_info.first + i;
        WinSendDlgItemMsg( hwnd, CB_DRIVE, LM_INSERTITEM,
                           MPFROMSHORT( LIT_END ), MPFROMP( drive ));
      }
    }
    DosClose( hcdrom );

    if( *cfg.cddrive ) {
      WinSetDlgItemText( hwnd, CB_DRIVE, cfg.cddrive );
    } else {
      WinSendDlgItemMsg( hwnd, CB_DRIVE, LM_SELECTITEM,
                         MPFROMSHORT( 0 ), MPFROMSHORT( TRUE ));
    }
  }

  if( WinProcessDlg( hwnd ) == DID_OK ) {
    SHORT selected =
          SHORT1FROMMR( WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_QUERYSELECTION,
                        MPFROMSHORT( LIT_FIRST ), 0 ));

    WinQueryDlgItemText( hwnd, CB_DRIVE, sizeof( cfg.cddrive ), cfg.cddrive );

    if( options & TRK_ADD_TO_LIST ) {
      char track[32];
      char cdurl[64];

      while( selected != LIT_NONE ) {
        WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_QUERYITEMTEXT,
                           MPFROM2SHORT( selected, sizeof(track)), MPFROMP(track));
        sprintf( cdurl, "cd:///%s\\%s", cfg.cddrive, track );
        pl_add_file( cdurl, NULL, 0 );
        selected = SHORT1FROMMR( WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_QUERYSELECTION,
                                 MPFROMSHORT( selected ), 0 ));
      }
      pl_completed();
    } else {
      char track[32];
      char cdurl[64];

      WinSendDlgItemMsg( hwnd, LB_TRACKS, LM_QUERYITEMTEXT,
                         MPFROM2SHORT( selected, sizeof(track)), MPFROMP(track));
      sprintf( cdurl, "cd:///%s\\%s", cfg.cddrive, track );
      amp_load_singlefile( cdurl, 0 );
    }
  }
  WinDestroyWindow( hwnd );
  return TRUE;
}

/* Adds CD tracks to the playlist or load one to the player. */
BOOL
amp_load_track( HWND owner, int options ) {
  return BOOLFROMMR( WinSendMsg( hplayer, AMP_PB_LOAD_TRACK,
                                 MPFROMHWND( owner ), MPFROMLONG( options )));
}

/* Reads url from the specified file. */
char*
amp_url_from_file( char* result, const char* filename, int size )
{
  FILE* file = fopen( filename, "r" );

  if( file ) {
    if( fgets( result, size, file )) {
        blank_strip( result );
    }
    fclose( file );
  } else {
    *result = 0;
  }

  return result;
}

/* Extracts song title from the specified file name. */
char*
amp_title_from_filename( char* result, const char* filename, int size )
{
  char* p;

  if( result && filename ) {
    sdname( result, filename, size );
    for( p = result; *p; p++ ) {
      if( *p == '_' ) {
          *p =  ' ';
      }
    }
  }

  return result;
}

/* Prepares the player to the drop operation. Must be
   called from the main thread. */
static MRESULT
amp_pb_drag_over( HWND hwnd, PDRAGINFO pdinfo )
{
  PDRAGITEM pditem;
  int       i;
  USHORT    drag_op = 0;
  USHORT    drag    = DOR_NEVERDROP;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROM2SHORT( DOR_NEVERDROP, 0 );
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    #ifdef DEBUG
      char info[2048];
      pditem = DrgQueryDragitemPtr( pdinfo, i );

      DEBUGLOG(( "pm123: DnD info (%02d) hwndSource: %08X\n", i, pdinfo->hwndSource ));
      DEBUGLOG(( "pm123: DnD info (%02d) hwndItem:   %08X\n", i, pditem->hwndItem   ));
      DEBUGLOG(( "pm123: DnD info (%02d) ulItemID:   %lu\n",  i, pditem->ulItemID   ));
      DEBUGLOG(( "pm123: DnD info (%02d) fsControl:  %08X\n", i, pditem->fsControl  ));

      DrgQueryStrName( pditem->hstrType, sizeof( info ), info );
      DEBUGLOG(( "pm123: DnD info (%02d) hstrType: %s\n", i, info ));
      DrgQueryStrName( pditem->hstrRMF,  sizeof( info ), info );
      DEBUGLOG(( "pm123: DnD info (%02d) hstrRMF: %s\n", i, info ));
      DrgQueryStrName( pditem->hstrContainerName, sizeof( info ), info );
      DEBUGLOG(( "pm123: DnD info (%02d) hstrContainerName: %s\n", i, info ));
      DrgQueryStrName( pditem->hstrSourceName, sizeof( info ), info );
      DEBUGLOG(( "pm123: DnD info (%02d) hstrSourceName: %s\n", i, info ));
      DrgQueryStrName( pditem->hstrTargetName, sizeof( info ), info );
      DEBUGLOG(( "pm123: DnD info (%02d) hstrTargetName: %s\n", i, info ));
    #else
      pditem = DrgQueryDragitemPtr( pdinfo, i );
    #endif

    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL )) {
      if( pdinfo->usOperation == DO_DEFAULT ) {
        drag    = DOR_DROP;
        drag_op = DO_COPY;
      }
      else if(( pdinfo->usOperation == DO_COPY && pditem->fsSupportedOps & DO_COPYABLE ) ||
              ( pdinfo->usOperation == DO_MOVE && pditem->fsSupportedOps & DO_MOVEABLE ) ||
              ( pdinfo->usOperation == DO_LINK && pditem->fsSupportedOps & DO_LINKABLE ))
      {
        drag    = DOR_DROP;
        drag_op = pdinfo->usOperation;
      } else {
        drag    = DOR_NODROPOP;
        drag_op = DO_UNKNOWN;
        break;
      }
    } else if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL )) {
      if( pdinfo->usOperation == DO_DEFAULT &&
          pditem->fsSupportedOps & DO_COPYABLE )
      {
        drag    = DOR_DROP;
        drag_op = DO_COPY;
      }
      else if( pdinfo->usOperation == DO_LINK &&
               pditem->fsSupportedOps & DO_LINKABLE )
      {
        drag    = DOR_DROP;
        drag_op = pdinfo->usOperation;
      } else {
        drag    = DOR_NODROPOP;
        drag_op = DO_UNKNOWN;
        break;
      }
    } else {
      drag    = DOR_NEVERDROP;
      drag_op = DO_UNKNOWN;
      break;
    }
  }

  DrgFreeDraginfo( pdinfo );
  return MPFROM2SHORT( drag, drag_op );
}

/* Returns TRUE if the specified window is belong to
   the current process. */
static BOOL
amp_pb_is_app_window( HWND hwnd )
{
  PID pid1, pid2;
  TID tid1, tid2;

  if( WinQueryWindowProcess( hplayer, &pid1, &tid1 ) &&
      WinQueryWindowProcess( hwnd,    &pid2, &tid2 ))
  {
    return pid1 == pid2;
  }

  return FALSE;
}

/* Receives dropped files or playlist records. Must be
   called from the main thread. */
static MRESULT
amp_pb_drag_drop( HWND hwnd, PDRAGINFO pdinfo )
{
  PDRAGITEM pditem;

  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];
  char fullname[_MAX_PATH];
  int  i;

  if( !DrgAccessDraginfo( pdinfo )) {
    return 0;
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    DrgQueryStrName( pditem->hstrSourceName,    sizeof( filename ), filename );
    DrgQueryStrName( pditem->hstrContainerName, sizeof( pathname ), pathname );
    strcpy( fullname, pathname );
    strcat( fullname, filename );

    if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ))
    {
      if( pditem->hstrContainerName && pditem->hstrSourceName ) {
        // Have full qualified file name.
        if( DrgVerifyType( pditem, "UniformResourceLocator" )) {
          amp_url_from_file( fullname, fullname, sizeof( fullname ));
        }
        if( is_dir( fullname )) {
          pl_add_directory( fullname, PL_DIR_RECURSIVE );
        } else if( is_playlist( fullname )) {
          pl_load( fullname, 0 );
        } else if( pdinfo->cditem == 1 ) {
          amp_pb_load_singlefile( fullname, 0 );
        } else {
          pl_add_file( fullname, NULL, 0 );
        }

        if( pditem->hwndItem ) {
          // Tell the source you're done.
          DrgSendTransferMsg( pditem->hwndItem, DM_ENDCONVERSATION, (MPARAM)pditem->ulItemID,
                                                                    (MPARAM)DMFL_TARGETSUCCESSFUL );
        }
      }
      else if( pditem->hwndItem &&
               DrgVerifyType( pditem, "UniformResourceLocator" ))
      {
        // The droped item must be rendered.
        PDRAGTRANSFER pdtrans  = DrgAllocDragtransfer(1);
        AMP_DROPINFO* pdsource = (AMP_DROPINFO*)malloc( sizeof( AMP_DROPINFO ));
        char renderto[_MAX_PATH];

        if( !pdtrans || !pdsource ) {
          return 0;
        }

        pdsource->cditem   = pdinfo->cditem;
        pdsource->hwndItem = pditem->hwndItem;
        pdsource->ulItemID = pditem->ulItemID;

        pdtrans->cb               = sizeof( DRAGTRANSFER );
        pdtrans->hwndClient       = hwnd;
        pdtrans->pditem           = pditem;
        pdtrans->hstrSelectedRMF  = DrgAddStrHandle( "<DRM_OS2FILE,DRF_TEXT>" );
        pdtrans->hstrRenderToName = 0;
        pdtrans->ulTargetInfo     = (ULONG)pdsource;
        pdtrans->fsReply          = 0;
        pdtrans->usOperation      = pdinfo->usOperation;

        // Send the message before setting a render-to name.
        if( pditem->fsControl & DC_PREPAREITEM ) {
          DrgSendTransferMsg( pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdtrans, 0 );
        }

        strlcpy( renderto, startpath , sizeof( renderto ));
        strlcat( renderto, "pm123.dd", sizeof( renderto ));

        pdtrans->hstrRenderToName = DrgAddStrHandle( renderto );

        // Send the message after setting a render-to name.
        if(( pditem->fsControl & ( DC_PREPARE | DC_PREPAREITEM )) == DC_PREPARE ) {
          DrgSendTransferMsg( pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdtrans, 0 );
        }

        // Ask the source to render the selected item.
        DrgSendTransferMsg( pditem->hwndItem, DM_RENDER, (MPARAM)pdtrans, 0 );
      }
    }
    else if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ))
    {
      if( pdinfo->cditem == 1 ) {
        if( amp_pb_is_app_window( pdinfo->hwndSource )) {
          WinSendMsg( pdinfo->hwndSource, WM_123FILE_LOAD,
                      MPFROMLONG( pditem->ulItemID ), 0 );
        } else {
          amp_pb_load_singlefile( fullname, 0 );
        }
        if( pdinfo->usOperation == DO_MOVE ) {
          WinSendMsg( pdinfo->hwndSource, WM_123FILE_REMOVE, MPFROMLONG( pditem->ulItemID ), 0 );
        }
      } else {
        pl_add_file( fullname, NULL, 0 );

        if( pdinfo->usOperation == DO_MOVE ) {
          WinSendMsg( pdinfo->hwndSource, WM_123FILE_REMOVE, MPFROMLONG( pditem->ulItemID ), 0 );
        }
      }
    }
  }

  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );
  return 0;
}

/* Receives dropped and rendered files and urls. Must be
   called from the main thread. */
static MRESULT
amp_pb_drag_render_done( HWND hwnd, PDRAGTRANSFER pdtrans, USHORT rc )
{
  char rendered[_MAX_PATH];
  char fullname[_MAX_PATH];

  AMP_DROPINFO* pdsource = (AMP_DROPINFO*)pdtrans->ulTargetInfo;

  // If the rendering was successful, use the file, then delete it.
  if(( rc & DMFL_RENDEROK ) && pdsource &&
       DrgQueryStrName( pdtrans->hstrRenderToName, sizeof( rendered ), rendered ))
  {
    amp_url_from_file( fullname, rendered, sizeof( fullname ));
    DosDelete( rendered );

    if( is_playlist( fullname )) {
      pl_load( fullname, 0 );
    } else if( pdsource->cditem == 1 ) {
      amp_pb_load_singlefile( fullname, 0 );
    } else {
      pl_add_file( fullname, NULL, 0 );
    }
  }

  // Tell the source you're done.
  DrgSendTransferMsg( pdsource->hwndItem, DM_ENDCONVERSATION,
                     (MPARAM)pdsource->ulItemID, (MPARAM)DMFL_TARGETSUCCESSFUL );

  DrgDeleteStrHandle ( pdtrans->hstrSelectedRMF );
  DrgDeleteStrHandle ( pdtrans->hstrRenderToName );
  DrgFreeDragtransfer( pdtrans );
  return 0;
}

/* It is called after successful saving of the meta information.
   Must be called from the main thread. */
static void
amp_pb_refresh_file( AMP_FILEINFO* data )
{
  ASSERT_IS_MAIN_THREAD;

  pl_refresh_file( data->filename, &data->info );

  if( stricmp( current_filename, data->filename ) == 0 ) {
    current_info = data->info;
    amp_pb_display_filename();
    amp_invalidate( UPD_FILEINFO );
  }

  free( data->filename );
  free( data );
}

/* Default dialog procedure for the file dialog. */
MRESULT EXPENTRY
amp_file_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  FILEDLG* filedialog =
    (FILEDLG*)WinQueryWindowULong( hwnd, QWL_USER );

  switch( msg )
  {
    case WM_INITDLG:
      if( filedialog && !(filedialog->ulUser & FDU_RECURSEBTN )) {
        WinShowWindow( WinWindowFromID( hwnd, CB_RECURSE ), FALSE );
      } else {
        WinCheckButton( hwnd, CB_RECURSE, cfg.add_recursive );
      }
      if( filedialog && !(filedialog->ulUser & FDU_RELATIVBTN )) {
        WinShowWindow( WinWindowFromID( hwnd, CB_RELATIV ), FALSE );
      } else {
        WinCheckButton( hwnd, CB_RELATIV, cfg.save_relative );
      }
      if( filedialog && filedialog->ulUser & FDU_DIR_ENABLE ) {
        WinEnableControl( hwnd, DID_OK, TRUE  );
      }
      do_warpsans( hwnd );
      break;

    case WM_HELP:
      amp_show_help( IDH_MAIN );
      return 0;

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == DID_FILENAME_ED && SHORT2FROMMP(mp1) == EN_CHANGE )
      {
        char file[_MAX_PATH];
        WinQueryDlgItemText( hwnd, DID_FILENAME_ED, sizeof(file), file );

        if( filedialog->ulUser & FDU_RECURSEBTN ) {
          if( !*file || strcmp( file, "*"   ) == 0 ||
                        strcmp( file, "*.*" ) == 0 )
          {
            WinEnableControl( hwnd, CB_RECURSE, TRUE  );
          } else {
            WinEnableControl( hwnd, CB_RECURSE, FALSE );
          }
        }

        // Prevents DID_OK from being greyed out.
        if( filedialog->ulUser & FDU_DIR_ENABLE ) {
          return 0;
        }
      }
      break;

    case WM_COMMAND:
      if( SHORT1FROMMP(mp1) == DID_OK )
      {
        if( filedialog->ulUser & FDU_RELATIVBTN ) {
          if( !WinQueryButtonCheckstate( hwnd, CB_RELATIV )) {
            filedialog->ulUser &= ~FDU_RELATIV_ON;
            cfg.save_relative = FALSE;
          } else {
            filedialog->ulUser |=  FDU_RELATIV_ON;
            cfg.save_relative = TRUE;
          }
        }

        if( filedialog->ulUser & FDU_DIR_ENABLE )
        {
          char file[_MAX_PATH];
          WinQueryDlgItemText( hwnd, DID_FILENAME_ED, sizeof(file), file );

          if( !*file ||
              strcmp( file, "*"   ) == 0 ||
              strcmp( file, "*.*" ) == 0 )
          {
            if( !is_root( filedialog->szFullFile )) {
              filedialog->szFullFile[strlen(filedialog->szFullFile)-1] = 0;
            }

            filedialog->lReturn    = DID_OK;
            filedialog->ulFQFCount = 1;

            if( filedialog->ulUser & FDU_RECURSEBTN ) {
              if( !WinQueryButtonCheckstate( hwnd, CB_RECURSE )) {
                filedialog->ulUser &= ~FDU_RECURSE_ON;
                cfg.add_recursive = FALSE;
              } else {
                filedialog->ulUser |=  FDU_RECURSE_ON;
                cfg.add_recursive = TRUE;
              }
            }

            WinDismissDlg( hwnd, DID_OK );
            return 0;
          }
        }
      }
      break;

    case FDM_FILTER:
    {
      HWND  hcbox = WinWindowFromID( hwnd, DID_FILTER_CB );
      ULONG pos   = WinQueryLboxSelectedItem( hcbox );
      ULONG len   = LONGFROMMR( WinSendMsg( hcbox, LM_QUERYITEMTEXTLENGTH, MPFROMSHORT(pos), 0 ));
      char* type  = malloc( len );
      BOOL  rc    = FALSE;
      char* filt;
      char  file[_MAX_PATH];

      if( !type ) {
        return WinDefFileDlgProc( hwnd, msg, mp1, mp2 );
      }

      WinQueryLboxItemText( hcbox, pos, type, len );
      WinQueryDlgItemText ( hwnd, DID_FILENAME_ED, sizeof(file), file );

      // If the selected type is not have extensions list - that it <All Files>
      // which OS/2 always adds in the list.
      if( !strchr( type, '(' )) {
        rc = TRUE;
      } else {
        strtok( type, "(" );

        while(( filt = strtok( NULL, ";)" )) != NULL ) {
          if( wildcardfit( filt, (char*)mp1 )) {
            rc = TRUE;
            break;
          }
        }
      }

      if( rc && ( strchr( file, '*' ) || strchr( file, '?' ))) {
        rc = wildcardfit( file, (char*)mp1 );
      }

      free( type );
      return MRFROMLONG( rc );
    }
  }
  return WinDefFileDlgProc( hwnd, msg, mp1, mp2 );
}

/* Create main pipe with only one instance possible since these pipe
   is almost all the time free, it wouldn't make sense having multiple
   intances. */
static HPIPE
amp_pb_pipe_create( void )
{
  ULONG rc;
  HPIPE hpipe;
  int   i = 1;

  while(( rc = DosCreateNPipe( pipename, &hpipe,
                               NP_ACCESS_DUPLEX,
                               NP_WAIT | NP_TYPE_BYTE | NP_READMODE_BYTE | 1,
                               2048,
                               2048,
                               500 )) == ERROR_PIPE_BUSY )
  {
    sprintf( pipename,"\\PIPE\\PM123_%d", ++i );
  }

  if( rc != 0 ) {
    amp_player_error( "Could not create pipe %s, rc = %d.", pipename, rc );
    return NULLHANDLE;
  } else {
    return hpipe;
  }
}

/* Opens the specified pipe. */
static HPIPE
amp_pb_pipe_open( const char* pipename )
{
  HPIPE  hpipe;
  ULONG  action;
  APIRET rc;

  rc = DosOpen((PSZ)pipename, &hpipe, &action, 0, FILE_NORMAL,
                OPEN_ACTION_FAIL_IF_NEW  | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
                NULL );

  if( rc == NO_ERROR ) {
    return hpipe;
  } else {
    return NULLHANDLE;
  }
}

/* Closes the specified pipe. */
static BOOL
amp_pb_pipe_close( HPIPE hpipe ) {
  return ( DosClose( hpipe ) == NO_ERROR );
}

/* Writes string to the specified pipe. */
static void
amp_pb_pipe_write( HPIPE hpipe, const char* string )
{
  ULONG action;

  DosWrite( hpipe, (PVOID)string, strlen( string ) + 1, &action );
  DosResetBuffer( hpipe );
}

/* Reads string from the specified pipe. */
static char*
amp_pb_pipe_read( HPIPE pipe, char* buffer, int size )
{
  ULONG read = 0;
  int   done = 0;
  char* p    = buffer;

  while( done < size - 1 ) {
    if( DosRead( hpipe, p, 1, &read ) == NO_ERROR && read == 1 ) {
      if( *p == '\r' ) {
        continue;
      } else if( *p == '\n' || !*p ) {
        break;
      } else {
        ++p;
        ++done;
      }
    } else {
      if( !done ) {
        return NULL;
      } else {
        break;
      }
    }
  }

  *p = 0;
  return buffer;
}

/* Opens the specified pipe and writes data to it. */
static BOOL
amp_pb_pipe_open_and_write( const char* pipename, const char* string )
{
  HPIPE hpipe = amp_pb_pipe_open( pipename );

  if( hpipe != NULLHANDLE )
  {
    amp_pb_pipe_write( hpipe, string );
    amp_pb_pipe_close( hpipe );
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Dispatches requests received from the pipe. */
static void TFNENTRY
amp_pipe_thread( void* scrap )
{
  char  buffer [2048];
  char  command[2048];
  char* zork;
  char* dork;
  HAB   hab = WinInitialize( 0 );
  HMQ   hmq = WinCreateMsgQueue( hab, 0 );

  while( !is_terminate )
  {
    DosDisConnectNPipe( hpipe );
    DosConnectNPipe( hpipe );

    while( amp_pb_pipe_read( hpipe, buffer, sizeof( buffer )) &&
           !is_terminate )
    {
      blank_strip( buffer );
      DEBUGLOG(( "pm123: receive from pipe %08X command %s.\n", hpipe, buffer ));

      if( *buffer && *buffer != '*' )
      {
        if( is_dir( buffer )) {
          pl_clear();
          pl_add_directory( buffer, PL_DIR_RECURSIVE );
        } else {
          amp_load_singlefile( buffer, 0 );
        }
      }
      else if( *buffer == '*' )
      {
        strcpy( command, buffer + 1 ); // Strip the '*' character.
        blank_strip( command );

        zork = strtok( command, " " );
        dork = strtok( NULL,    ""  );

        if( zork ) {
          if( stricmp( zork, "status" ) == 0 )
          {
            char  result[1024];
            short type = STR_NULL;

            if( dork ) {
              if( stricmp( dork, "file" ) == 0 ) {
                type = STR_FILENAME;
              } else if( stricmp( dork, "tag"  ) == 0 ) {
                type = STR_DISPLAY_TAG;
              } else if( stricmp( dork, "info" ) == 0 ) {
                type = STR_DISPLAY_INFO;
              }
            }

            WinSendMsg( hplayer, AMP_QUERY_STRING,
                        MPFROMP( result ), MPFROM2SHORT( sizeof( result ), type ));

            amp_pb_pipe_write( hpipe, result );
          }
          else if( stricmp( zork, "size" ) == 0 )
          {
            if( dork ) {
              if( stricmp( dork, "regular" ) == 0 ||
                  stricmp( dork, "0"       ) == 0 ||
                  stricmp( dork, "normal"  ) == 0  )
              {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_SIZE_NORMAL ), 0 );
              }
              if( stricmp( dork, "small"   ) == 0 ||
                  stricmp( dork, "1"       ) == 0  )
              {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_SIZE_SMALL  ), 0 );
              }
              if( stricmp( dork, "tiny"    ) == 0 ||
                  stricmp( dork, "2"       ) == 0  )
              {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_SIZE_TINY   ), 0 );
              }
            }
          }
          else if( stricmp( zork, "rdir" ) == 0 )
          {
            if( dork ) {
              pl_add_directory( dork, PL_DIR_RECURSIVE );
            }
          }
          else if( stricmp( zork, "dir"  ) == 0 )
          {
            if( dork ) {
              pl_add_directory( dork, 0 );
            }
          }
          else if( stricmp( zork, "font" ) == 0 )
          {
            if( dork ) {
              if( stricmp( dork, "1" ) == 0 ) {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FONT_SET1 ), 0 );
              }
              if( stricmp( dork, "2" ) == 0 ) {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FONT_SET2 ), 0 );
              }
            }
          }
          else if( stricmp( zork, "add" ) == 0 )
          {
            char* file;

            if( dork ) {
              while( *dork ) {
                file = dork;
                while( *dork && *dork != ';' ) {
                  ++dork;
                }
                if( *dork == ';' ) {
                  *dork++ = 0;
                }
                if( is_playlist( file )) {
                  pl_load( file, PL_LOAD_NOT_RECALL | PL_LOAD_CONTINUE );
                } else {
                  pl_add_file( file, NULL, 0 );
                }
              }
            }
          }
          else if( stricmp( zork, "load" ) == 0 )
          {
            if( dork ) {
              amp_load_singlefile( dork, 0 );
            }
          }
          else if( stricmp( zork, "hide"  ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_MINIMIZE ), 0 );
          }
          else if( stricmp( zork, "float" ) == 0 )
          {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                if( cfg.floatontop ) {
                  WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FLOAT ), 0 );
                }
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                if( !cfg.floatontop ) {
                  WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FLOAT ), 0 );
                }
              }
            }
          }
          else if( stricmp( zork, "use" ) == 0 )
          {
            amp_pl_use( TRUE );
          }
          else if( stricmp( zork, "clear" ) == 0 )
          {
            pl_clear();
          }
          else if( stricmp( zork, "next" ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_NEXT ), 0 );
          }
          else if( stricmp( zork, "previous" ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PREV ), 0 );
          }
          else if( stricmp( zork, "remove" ) == 0 )
          {
            pl_remove_records( PL_REMOVE_LOADED );
          }
          else if( stricmp( zork, "forward" ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_FWD  ), 0 );
          }
          else if( stricmp( zork, "rewind" ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_REW  ), 0 );
          }
          else if( stricmp( zork, "stop" ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_STOP ), 0 );
          }
          else if( stricmp( zork, "jump" ) == 0 )
          {
            if( dork && decoder_playing() && time_total() > 0 ) {
              WinSendMsg( hplayer, AMP_PB_SEEK, MPFROMLONG( atoi( dork ) * 1000 ), 0 );
            }
          }
          else if( stricmp( zork, "play" ) == 0 )
          {
            if( dork ) {
              amp_load_singlefile( dork, AMP_LOAD_NOT_PLAY );
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            } else if( !decoder_playing()) {
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            }
          }
          else if( stricmp( zork, "pause" ) == 0 )
          {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                if( is_paused()) {
                  WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
                }
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                if( !is_paused()) {
                  WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
                }
              }
            } else {
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
            }
          }
          else if( stricmp( zork, "playonload" ) == 0 )
          {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.playonload = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.playonload = TRUE;
              }
            }
          }
          else if( stricmp( zork, "autouse" ) == 0 )
          {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.autouse = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.autouse = TRUE;
              }
            }
          }
          else if( stricmp( zork, "playonuse" ) == 0 )
          {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.playonuse = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.playonuse = TRUE;
              }
            }
          }
          else if( stricmp( zork, "repeat"  ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_REPEAT  ), 0 );
          }
          else if( stricmp( zork, "shuffle" ) == 0 )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_SHUFFLE ), 0 );
          }
          else if( stricmp( zork, "volume" ) == 0 )
          {
            char buf[64];

            if( dork )
            {
              int volume = cfg.defaultvol;

              if( *dork == '+' ) {
                volume += atoi( dork + 1 );
              } else if( *dork == '-' ) {
                volume -= atoi( dork + 1 );
              } else {
                volume  = atoi( dork );
              }

              WinSendMsg( hplayer, AMP_PB_VOLUME, MPFROMLONG( volume ), 0 );
            }
            amp_pb_pipe_write( hpipe, itoa( cfg.defaultvol, buf, 10 ));
          }
        }
      }
    }
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
  _endthread();
}

/* Loads a file selected by the user to the player. Must be
   called from the main thread. */
static void
amp_pb_load_file( HWND owner )
{
  FILEDLG filedialog;

  char  type_audio[2048];
  char  type_all  [2048];
  APSZ  types[4] = {{ 0 }};

  ASSERT_IS_MAIN_THREAD;

  types[0][0] = type_audio;
  types[1][0] = FDT_PLAYLIST;
  types[2][0] = type_all;

  memset( &filedialog, 0, sizeof( FILEDLG ));
  filedialog.cbSize     = sizeof( FILEDLG );
  filedialog.fl         = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle   = "Load file";
  filedialog.hMod       = hmodule;
  filedialog.usDlgId    = DLG_FILE;
  filedialog.pfnDlgProc = amp_file_dlg_proc;

  // WinFileDlg returns error if a length of the pszIType string is above
  // 255 characters. Therefore the small part from the full filter is used
  // as initial extended-attribute type filter. This part has enough to
  // find the full filter in the papszITypeList.
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_AUDIO_ALL;

  strcpy( type_audio, FDT_AUDIO     );
  dec_fill_types( type_audio + strlen( type_audio ),
                  sizeof( type_audio ) - strlen( type_audio ) - 1 );
  strcat( type_audio, ")" );

  strcpy( type_all, FDT_AUDIO_ALL );
  dec_fill_types( type_all + strlen( type_all ),
                  sizeof( type_all ) - strlen( type_all ) - 1 );
  strcat( type_all, ")" );

  DEBUGLOG(( "pm123: %s\n", type_audio ));
  DEBUGLOG(( "pm123: %s\n", type_all   ));

  strcpy( filedialog.szFullFile, cfg.filedir );
  if( WinFileDlg( HWND_DESKTOP, owner, &filedialog ) == NULLHANDLE ) {
    amp_error( owner, "WinFileDlg error: %08X\n", WinGetLastError( hab ));
  }

  if( filedialog.lReturn == DID_OK ) {
    sdrivedir( cfg.filedir, filedialog.szFullFile, sizeof( cfg.filedir ));
    amp_pb_load_singlefile( filedialog.szFullFile, 0 );
  }
}

/* Loads a skin selected by the user. Must be called from
   the main thread. */
static void
amp_pb_load_skin( HAB hab, HWND owner, HPS hps )
{
  FILEDLG filedialog;
  APSZ types[] = {{ FDT_SKIN }, { 0 }};

  ASSERT_IS_MAIN_THREAD;
  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Load PM123 skin";
  filedialog.hMod           = hmodule;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_SKIN;

  sdrivedir( filedialog.szFullFile, cfg.defskin, sizeof( filedialog.szFullFile ));
  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    bmp_load_skin( filedialog.szFullFile, hab, owner, hps );
    strcpy( cfg.defskin, filedialog.szFullFile );
  }
}

static BOOL
amp_pb_save_eq( HWND owner, float* gains, BOOL *mutes, float preamp )
{
  FILEDLG filedialog;
  FILE*   file;
  int     i;
  char    ext[_MAX_EXT];
  APSZ    types[] = {{ FDT_EQUALIZER }, { 0 }};

  ASSERT_IS_MAIN_THREAD;
  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Save equalizer";
  filedialog.hMod           = hmodule;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_EQUALIZER;

  strcpy( filedialog.szFullFile, cfg.lasteq );
  WinFileDlg( HWND_DESKTOP, owner, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    if( strcmp( sfext( ext, filedialog.szFullFile, sizeof( ext )), "" ) == 0 ) {
      strcat( filedialog.szFullFile, ".eq" );
    }

    if( amp_warn_if_overwrite( owner, filedialog.szFullFile ))
    {
      strcpy( cfg.lasteq, filedialog.szFullFile );
      file = fopen( filedialog.szFullFile, "w" );

      if( file == NULL ) {
        return FALSE;
      }

      fprintf( file, "#\n# Equalizer created with %s\n# Do not modify!\n#\n", AMP_FULLNAME );
      fprintf( file, "# Band gains\n" );
      for( i = 0; i < 20; i++ ) {
        fprintf( file, "%g\n", gains[i] );
      }
      fprintf( file, "# Mutes\n" );
      for( i = 0; i < 20; i++ ) {
        fprintf( file, "%lu\n", mutes[i] );
      }
      fprintf( file, "# Preamplifier\n" );
      fprintf( file, "%g\n", preamp );

      fprintf( file, "# End of equalizer\n" );
      fclose( file );
      return TRUE;
    }
  }
  return FALSE;
}

static BOOL
amp_pb_load_eq_file( char* filename, float* gains, BOOL* mutes, float* preamp )
{
  FILE* file;
  char  vz[CCHMAXPATH];
  int   i;

  ASSERT_IS_MAIN_THREAD;

  file = fopen( filename, "r" );
  if( !file ) {
    return FALSE;
  }

  i = 0;
  while( !feof( file ))
  {
    fgets( vz, sizeof( vz ), file );
    blank_strip( vz );
    if( *vz && vz[0] != '#' && vz[0] != ';' && i < 41 )
    {
      if( i < 20 ) {
        gains[i] = atof(vz);
      }
      if( i > 19 && i < 40 ) {
        mutes[i-20] = atoi(vz);
      }
      if( i == 40 ) {
        *preamp = atof(vz);
      }
      i++;
    }
  }
  fclose( file );
  return TRUE;
}

static BOOL
amp_pb_load_eq( HWND hwnd, float* gains, BOOL* mutes, float* preamp )
{
  FILEDLG filedialog;
  APSZ    types[] = {{ FDT_EQUALIZER }, { 0 }};

  ASSERT_IS_MAIN_THREAD;
  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Load equalizer";
  filedialog.hMod           = hmodule;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_EQUALIZER;

  sdrivedir( filedialog.szFullFile, cfg.lasteq, sizeof( filedialog.szFullFile ));
  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    strcpy( cfg.lasteq, filedialog.szFullFile );
    return amp_pb_load_eq_file( filedialog.szFullFile, gains, mutes, preamp );
  }
  return FALSE;
}

/* Returns TRUE if the save stream feature has been enabled.
   Must be called from the main thread. */
static BOOL
amp_pb_save_stream( HWND hwnd, BOOL enable )
{
  ASSERT_IS_MAIN_THREAD;

  if( enable )
  {
    FILEDLG filedialog;

    memset( &filedialog, 0, sizeof( FILEDLG ));
    filedialog.cbSize     = sizeof( FILEDLG );
    filedialog.fl         = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_CUSTOM;
    filedialog.pszTitle   = "Save stream as";
    filedialog.hMod       = hmodule;
    filedialog.usDlgId    = DLG_FILE;
    filedialog.pfnDlgProc = amp_file_dlg_proc;

    strcpy( filedialog.szFullFile, cfg.savedir );
    WinFileDlg( HWND_DESKTOP, hwnd, &filedialog );

    if( filedialog.lReturn == DID_OK ) {
      if( amp_warn_if_overwrite( hwnd, filedialog.szFullFile ))
      {
        msg_savestream( filedialog.szFullFile );
        sdrivedir( cfg.savedir, filedialog.szFullFile, sizeof( cfg.savedir ));
        return TRUE;
      }
    }
  } else {
    msg_savestream( NULL );
  }

  return FALSE;
}

/* Starts playing a next file or stops the player if all files
   already played. Must be called from the main thread. */
static void
amp_pb_playstop( void )
{
  ASSERT_IS_MAIN_THREAD;

  amp_pb_stop();

  if( amp_playmode == AMP_SINGLE && cfg.rpt ) {
    amp_pb_play( 0 );
  } else if ( amp_playmode == AMP_PLAYLIST  ) {
    if( pl_load_next_record()) {
      amp_pb_play( 0 );
    } else {
      pl_clean_shuffle();
      if( pl_load_first_record() && cfg.rpt ) {
        amp_pb_play( 0 );
      }
    }
  }
}

static MRESULT EXPENTRY
amp_pb_eq_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  static ULONG slider_range;
  static BOOL  nottheuser = FALSE;

  switch( msg )
  {
    case WM_CLOSE:
      heq = NULLHANDLE;
      WinDestroyWindow( hwnd );
      return 0;

    case WM_DESTROY:
      save_window_pos( hwnd, WIN_MAP_POINTS );
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );

    case WM_HELP:
      amp_show_help( IDH_EQUALIZER );
      return 0;

    case AMP_REFRESH_CONTROLS:
    {
      float db;
      ULONG value;
      int   i;

      nottheuser = TRUE;

      _control87( EM_INVALID   | EM_DENORMAL | EM_ZERODIVIDE | EM_OVERFLOW |
                  EM_UNDERFLOW | EM_INEXACT, MCW_EM );

      for( i = 0; i < 10; i++ )
      {
        db = 20 * log10( gains[i] );
        value = ( db * slider_range / 2 ) / 12 + slider_range / 2;

        WinSendDlgItemMsg( hwnd, 101 + i, SLM_SETSLIDERINFO,
                           MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                           MPFROMSHORT( value ));

        WinSendDlgItemMsg( hwnd, 125 + i, BM_SETCHECK,
                           MPFROMSHORT( mutes[i] ), 0 );
      }

      db = 20 * log10( preamp );
      value = ( db * slider_range / 2 ) / 12 + slider_range / 2;

      WinSendDlgItemMsg( hwnd, 139, SLM_SETSLIDERINFO,
                         MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                         MPFROMSHORT( value ));

      WinSendDlgItemMsg( hwnd, 121, BM_SETCHECK,
                         MPFROMSHORT( cfg.eq_enabled ), 0 );

      nottheuser = FALSE;
      break;
    }

    case WM_COMMAND:
      switch( COMMANDMSG(&msg)->cmd )
      {
        case 123:
          if( amp_pb_load_eq( hwnd, gains, mutes, &preamp )) {
            WinSendMsg( hwnd, AMP_REFRESH_CONTROLS, 0, 0 );
          }
          if( WinQueryButtonCheckstate( hwnd, 121 )) {
            msg_equalize( gains, mutes, preamp, 1 );
          }
          break;

        case 124:
          amp_pb_save_eq( hwnd, gains, mutes, preamp );
          break;

        case 122:
        {
          int i;

          for( i = 0; i < 20; i++ ) {
            gains[i] = 1.0;
          }
          for( i = 0; i < 20; i++ ) {
            mutes[i] = 0;
          }
          for( i = 0; i < 10; i++ ) {
            WinSendDlgItemMsg( hwnd, 101 + i, SLM_SETSLIDERINFO,
                               MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                               MPFROMLONG( slider_range / 2 ));
          }
          for( i = 0; i < 10; i++ ) {
            WinSendDlgItemMsg( hwnd, 125 + i, BM_SETCHECK, MPFROMSHORT( FALSE ), 0 );
          }

          WinSendDlgItemMsg( hwnd, 139, SLM_SETSLIDERINFO,
                             MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                             MPFROMLONG( slider_range / 2 ));

          if( WinQueryButtonCheckstate( hwnd, 121 )) {
            msg_equalize( gains, mutes, preamp, 1 );
          } else {
            msg_equalize( gains, mutes, preamp, 0 );
          }
          break;
        }
      }
      break;

    case WM_CONTROL:
    {
      int id = SHORT1FROMMP(mp1);
      if( nottheuser ) {
        break;
      }

      if( SHORT2FROMMP( mp1 ) == BN_CLICKED )
      {
        if( id > 124 && id < 135 ) // Mute
        {
          mutes[id - 125] = WinQueryButtonCheckstate( hwnd, id );       // Left
          mutes[id - 125 + 10] = WinQueryButtonCheckstate( hwnd, id );  // Right

          if( WinQueryButtonCheckstate( hwnd, 121 )) {
            msg_equalize( gains, mutes, preamp, 1 );
            break;
          }
        }

        if( id == 121 ) {
          cfg.eq_enabled = WinQueryButtonCheckstate( hwnd, 121 );
          msg_equalize( gains, mutes, preamp, cfg.eq_enabled );
          break;
        }
      }

      switch( SHORT2FROMMP( mp1 ))
      {
        case SLN_CHANGE:
          // Slider adjust
          if(( id > 100 && id < 111 ) || id == 139 )
          {
            float g2;

            _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
                        EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM);

            // -12 to 12 dB
            g2 = ((float)LONGFROMMP(mp2) - slider_range / 2 ) / ( slider_range / 2 ) * 12;
            // transforming into voltage gain
            g2 = pow( 10.0, g2 / 20.0 );

            if( id == 139 ) {
              preamp = g2;
            } else {
              gains[SHORT1FROMMP(mp1) - 101] = g2;      // Left
              gains[SHORT1FROMMP(mp1) - 101 + 10] = g2; // Right
            }

            if( WinQueryButtonCheckstate( hwnd, 121 )) {
              msg_equalize( gains, mutes, preamp, 1 );
            }
          }
      }
      break;
    }

    case WM_INITDLG:
    {
      int i;

      nottheuser = TRUE;
      slider_range = HIUSHORT( WinSendDlgItemMsg( hwnd, 139, SLM_QUERYSLIDERINFO,
                               MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                               MPFROMLONG(0))) - 1;

      for( i = 101; i < 111; i++ )
      {
        WinSendDlgItemMsg( hwnd, i, SLM_ADDDETENT, MPFROMSHORT( 0 ), 0 );
        WinSendDlgItemMsg( hwnd, i, SLM_ADDDETENT, MPFROMSHORT( slider_range / 4 ), 0 );
        WinSendDlgItemMsg( hwnd, i, SLM_ADDDETENT, MPFROMSHORT( slider_range / 2 ), 0 );
        WinSendDlgItemMsg( hwnd, i, SLM_ADDDETENT, MPFROMSHORT( 3 * slider_range / 4 ), 0 );
        WinSendDlgItemMsg( hwnd, i, SLM_ADDDETENT, MPFROMSHORT( slider_range ), 0 );
        WinSendDlgItemMsg( hwnd, i, SLM_SETSLIDERINFO,
                           MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                           MPFROMLONG( slider_range / 2 ));
      }

       WinSendDlgItemMsg( hwnd, 110, SLM_SETTICKSIZE, MPFROM2SHORT( SMA_SETALLTICKS, 0 ), 0 );

       WinSendDlgItemMsg( hwnd, 139, SLM_ADDDETENT, MPFROMSHORT( 0 ), 0 );
       WinSendDlgItemMsg( hwnd, 139, SLM_ADDDETENT, MPFROMSHORT( slider_range / 4 ), 0 );
       WinSendDlgItemMsg( hwnd, 139, SLM_ADDDETENT, MPFROMSHORT( slider_range / 2 ), 0 );
       WinSendDlgItemMsg( hwnd, 139, SLM_ADDDETENT, MPFROMSHORT( 3 * slider_range / 4 ), 0 );
       WinSendDlgItemMsg( hwnd, 139, SLM_ADDDETENT, MPFROMSHORT( slider_range ), 0 );
       WinSendDlgItemMsg( hwnd, 139, SLM_SETSLIDERINFO,
                          MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                          MPFROMLONG( slider_range / 2 ));

       WinSendMsg( hwnd, AMP_REFRESH_CONTROLS, 0, 0 );
       nottheuser = FALSE;
       break;
    }

  default:
    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

static void
amp_pb_eq_show( void )
{
  ASSERT_IS_MAIN_THREAD;
  if( heq == NULLHANDLE )
  {
    heq = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                      amp_pb_eq_dlg_proc, hmodule, DLG_EQUALIZER, NULL );

    do_warpsans( heq );
    rest_window_pos( heq, WIN_MAP_POINTS );
  }

  WinSetWindowPos( heq, HWND_TOP, 0, 0, 0, 0,
                        SWP_ZORDER | SWP_SHOW | SWP_ACTIVATE );
}

/* Processes messages of the player client window.
   Must be called from the main thread. */
static MRESULT EXPENTRY
amp_pb_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_FOCUSCHANGE:
    {
      HPS hps = WinGetPS( hwnd );
      is_have_focus = SHORT1FROMMP( mp2 );
      bmp_draw_led( hps, is_have_focus  );
      WinReleasePS( hps );
      break;
    }

    case 0x041E:
      if( !is_have_focus ) {
        HPS hps = WinGetPS( hwnd );
        bmp_draw_led( hps, TRUE);
        WinReleasePS( hps );
      }
      return 0;

    case 0x041F:
      if( !is_have_focus ) {
        HPS hps = WinGetPS( hwnd );
        bmp_draw_led( hps, FALSE );
        WinReleasePS( hps );
      }
      return 0;

    case AMP_PB_STOP:
      return MRFROMBOOL( amp_pb_stop());
    case AMP_PB_PLAY:
      return MRFROMBOOL( amp_pb_play( LONGFROMMP( mp1 )));
    case AMP_PB_RESET:
      return MRFROMBOOL( amp_pb_reset());
    case AMP_PB_PAUSE:
      return MRFROMBOOL( amp_pb_pause());
    case AMP_PB_USE:
      return MRFROMBOOL( amp_pb_use( LONGFROMMP( mp1 )));
    case AMP_PB_LOAD_SINGLEFILE:
      return MRFROMBOOL( amp_pb_load_singlefile( mp1, LONGFROMMP( mp2 )));
    case AMP_PB_LOAD_URL:
      return MRFROMBOOL( amp_pb_load_url( HWNDFROMMP( mp1 ), LONGFROMMP( mp2 )));
    case AMP_PB_LOAD_TRACK:
      return MRFROMBOOL( amp_pb_load_track( HWNDFROMMP( mp1 ), LONGFROMMP( mp2 )));
    case AMP_PB_VOLUME:
      return MRFROMBOOL( amp_pb_set_volume( LONGFROMMP( mp1 )));
    case AMP_PB_SEEK:
      return MRFROMBOOL( msg_seek( LONGFROMMP( mp1 )));

    case AMP_DISPLAY_MESSAGE:
      if( mp2 ) {
        amp_error( hwnd, "%s", mp1 );
      } else {
        amp_info ( hwnd, "%s", mp1 );
      }
      free( mp1 );
      return 0;

    case AMP_DISPLAY_MODE:
      if( cfg.viewmode == CFG_DISP_FILEINFO ) {
        cfg.viewmode = CFG_DISP_FILENAME;
      } else {
        cfg.viewmode++;
      }

      amp_invalidate( UPD_FILEINFO | UPD_FILENAME );
      return 0;

    case AMP_QUERY_STRING:
    {
      char* buffer = (char*)mp1;
      SHORT size   = SHORT1FROMMP(mp2);
      SHORT type   = SHORT2FROMMP(mp2);

      switch( type )
      {
        case STR_NULL:
         *buffer = 0;
          break;
        case STR_VERSION:
          strlcpy( buffer, AMP_FULLNAME, size );
          break;
        case STR_DISPLAY_TEXT:
          strlcpy( buffer, bmp_query_text(), size );
          break;
        case STR_DISPLAY_TAG:
          amp_pb_construct_tag_string( buffer, &current_info, current_filename, size );
          break;
        case STR_DISPLAY_INFO:
          strlcpy( buffer, current_info.tech_info, size );
          break;
        case STR_FILENAME:
          strlcpy( buffer, current_filename, size );
          break;
        default:
          break;
      }
      return 0;
    }

    case WM_123FILE_REFRESH:
      amp_pb_refresh_file( mp1 );
      return 0;

    case WM_CONTEXTMENU:
      amp_pb_show_context_menu( hwnd );
      return 0;

    case WM_SEEKSTOP:
      is_seeking = FALSE;
      return 0;

    case WM_CHANGEBR:
    {
      HPS hps = WinGetPS( hwnd );
      current_info.bitrate = LONGFROMMP( mp1 );
      bmp_draw_rate( hps, current_info.bitrate );
      WinReleasePS( hps );
      return 0;
    }

    // Posted by decoder
    case WM_PLAYERROR:
      if( dec_status() == DECODER_STOPPED || !out_playing_data()) {
        amp_stop();
      }
      return 0;

    // Posted by decoder
    case WM_PLAYSTOP:
      // The decoder has finished the work, but we should wait until output
      // buffers will become empty.

      DEBUGLOG(( "pm123: receive WM_PLAYSTOP message.\n" ));

      // If output is always hungry, WM_OUTPUT_OUTOFDATA will not be
      // posted again so we go there by our selves.
      if( is_always_hungry()) {
        amp_pb_playstop();
      }
      return 0;

    // Posted by output
    case WM_OUTPUT_OUTOFDATA:
    {
      ULONG status = dec_status();

      DEBUGLOG(( "pm123: receive WM_OUTPUT_OUTOFDATA message, dec_status=%d, out_playing_data=%d\n",
                  status, out_playing_data()));

      // The fade plug-in always returns TRUE as result of the
      // out_playing_data. Added special case for it.
      if( status == DECODER_STOPPED &&
        ( !out_playing_data() || out_is_active( "fade" )))
      {
        amp_pb_playstop();
      } else if( status == DECODER_ERROR ) {
        amp_pb_stop();
      }
      return 0;
    }

    case DM_DRAGOVER:
      return amp_pb_drag_over( hwnd, (PDRAGINFO)mp1 );
    case DM_DROP:
      return amp_pb_drag_drop( hwnd, (PDRAGINFO)mp1 );
    case DM_RENDERCOMPLETE:
      return amp_pb_drag_render_done( hwnd, (PDRAGTRANSFER)mp1, SHORT1FROMMP( mp2 ));

    case WM_TIMER:
      if( LONGFROMMP(mp1) == TID_ONTOP ) {
        WinSetWindowPos( hframe, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER );
      }

      if( LONGFROMMP(mp1) == TID_UPDATE_PLAYER )
      {
        HPS hps = WinGetPS( hwnd );

        if( bmp_scroll_text()) {
          bmp_draw_text( hps );
        }

        if( upd_options ) {
          WinPostMsg( hwnd, AMP_PAINT, MPFROMLONG( upd_options ), 0 );
          upd_options = 0;
        }

        WinReleasePS( hps );
      }

      if( LONGFROMMP(mp1) == TID_UPDATE_TIMERS && decoder_playing()) {
        if( time_played() && cfg.mode == CFG_MODE_REGULAR )
        {
          HPS hps = WinGetPS( hwnd );
          amp_pb_paint_timers( hps );
          WinReleasePS( hps );
        }
      }
      return 0;

    case WM_ERASEBACKGROUND:
      return 0;

    case WM_REALIZEPALETTE:
      vis_broadcast( msg, mp1, mp2 );
      break;

    case AMP_PAINT:
    {
      HPS hps     = WinGetPS( hwnd );
      int options = LONGFROMMP( mp1 );

      if( options & UPD_FILENAME ) {
        amp_pb_display_filename();
      }
      if( options & UPD_TIMERS   ) {
        amp_pb_paint_timers( hps );
      }
      if( options & UPD_FILEINFO ) {
        amp_pb_paint_fileinfo( hps );
      }
      if( options & UPD_VOLUME   ) {
        bmp_draw_volume( hps, cfg.defaultvol );
      }

      WinReleasePS( hps );
      return 0;
    }

    case WM_PAINT:
    {
      HPS hps = WinBeginPaint( hwnd, NULLHANDLE, NULL );
      bmp_draw_background( hps, hwnd );
      amp_pb_paint_fileinfo( hps );
      amp_pb_paint_timers( hps );
      bmp_draw_led( hps, is_have_focus );
      bmp_draw_volume( hps, cfg.defaultvol );

      WinEndPaint( hps );
      return 0;
    }

    case WM_METADATA:
    {
      char* metadata = PVOIDFROMMP(mp1);
      char* titlepos;
      int   i;

      if( metadata ) {
        titlepos = strstr( metadata, "StreamTitle='" );
        if( titlepos )
        {
          titlepos += 13;
          for( i = 0; i < sizeof( current_info.title ) - 1 && *titlepos
                      && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
          {
            current_info.title[i] = *titlepos++;
          }

          current_info.title[i] = 0;
          amp_invalidate( UPD_FILEINFO | UPD_FILENAME );
          pl_refresh_file( current_filename, &current_info );
        }
      }
      return 0;
    }

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->source == CMDSRC_MENU )
      {
        if( COMMANDMSG(&msg)->cmd > IDM_M_PLUGINS ) {
          pg_process_plugin_menu( hwnd, mn_get_submenu( hmenu, IDM_M_PLUGINS  ),
                                                        COMMANDMSG(&msg)->cmd );
          return 0;
        }
        if( COMMANDMSG(&msg)->cmd > IDM_M_BOOKMARKS ) {
          bm_use_bookmark_from_menu( COMMANDMSG(&msg)->cmd );
          return 0;
        }
        if( COMMANDMSG(&msg)->cmd > IDM_M_LOAD_LAST )
        {
          char filename[_MAX_FNAME];
          int  i = COMMANDMSG(&msg)->cmd - IDM_M_LOAD_LAST - 1;

          strcpy( filename, cfg.last[i] );
          amp_pb_load_singlefile( filename, 0 );
          return 0;
        }
      }

      switch( COMMANDMSG(&msg)->cmd )
      {
        case IDM_M_MANAGER:
          pm_show( TRUE );
          return 0;

        case IDM_M_BM_ADD:
          if( *current_filename ) {
            bm_add_bookmark( hwnd, current_filename, &current_info, out_playing_pos());
          }
          return 0;

        case IDM_M_BM_EDIT:
          bm_show( TRUE );
          return 0;

        case IDM_M_EDIT:
        {
          DECODER_INFO info = current_info;
          char decoder [_MAX_MODULE_NAME];
          char filename[_MAX_PATH];

          strlcpy( filename, current_filename, sizeof( filename ));
          strlcpy( decoder,  current_decoder,  sizeof( decoder  ));

          if( tag_edit( hwnd, filename, decoder, &info, 0 ) == TAG_APPLY ) {
            tag_apply( filename, decoder, &info, TAG_APPLY_ALL );
          }
          return 0;
        }

        case IDM_M_SAVE_STREAM:
          amp_pb_save_stream( hwnd, !is_stream_saved());
          return 0;

        case IDM_M_EQUALIZE:
          amp_pb_eq_show();
          return 0;

        case IDM_M_FLOAT:
          cfg.floatontop = !cfg.floatontop;

          if( !cfg.floatontop ) {
            WinStopTimer ( hab, hwnd, TID_ONTOP );
          } else {
            WinStartTimer( hab, hwnd, TID_ONTOP, 100 );
          }
          return 0;

        case IDM_M_LOAD_URL:
          amp_pb_load_url( hwnd, URL_ADD_TO_PLAYER );
          return 0;

        case IDM_M_LOAD_TRACK:
          amp_pb_load_track( hwnd, TRK_ADD_TO_PLAYER );
          return 0;

        case IDM_M_FONT_SET1:
          cfg.font = 0;
          amp_invalidate( UPD_FILEINFO | UPD_FILENAME );
          return 0;

        case IDM_M_FONT_SET2:
          cfg.font = 1;
          amp_invalidate( UPD_FILEINFO | UPD_FILENAME );
          return 0;

        case IDM_M_SIZE_TINY:
          cfg.mode = CFG_MODE_TINY;
          bmp_reflow_and_resize( hframe );
          return 0;

        case IDM_M_SIZE_NORMAL:
          cfg.mode = CFG_MODE_REGULAR;
          bmp_reflow_and_resize( hframe );
          return 0;

        case IDM_M_SIZE_SMALL:
          cfg.mode = CFG_MODE_SMALL;
          bmp_reflow_and_resize( hframe );
          return 0;

        case IDM_M_MINIMIZE:
          WinSetWindowPos( hframe, HWND_DESKTOP, 0, 0, 0, 0, SWP_HIDE );
          WinSetActiveWindow( HWND_DESKTOP, WinQueryWindow( hwnd, QW_NEXTTOP ));
          return 0;

        case IDM_M_LOAD_SKIN:
        {
          HPS hps = WinGetPS( hwnd );
          amp_pb_load_skin( hab, hwnd, hps );
          WinReleasePS( hps );
          amp_invalidate( UPD_WINDOW );
          return 0;
        }

        case IDM_M_LOAD_FILE:
          amp_pb_load_file( hwnd );
          return 0;

        case IDM_M_PROPERTIES:
          cfg_properties( hwnd );
          amp_invalidate( UPD_FILEINFO | UPD_FILENAME );

          if( cfg.dock_windows ) {
            dk_arrange( hframe );
          } else {
            dk_cleanup( hframe );
          }
          return 0;

        case IDM_M_PLAYLIST:
          pl_show( TRUE );
          return 0;

        case IDM_M_VOL_RAISE:
          amp_pb_set_volume( cfg.defaultvol + 5 );
          return 0;

        case IDM_M_VOL_LOWER:
          amp_pb_set_volume( cfg.defaultvol - 5 );
          return 0;

        case IDM_M_MENU:
          amp_pb_show_context_menu( hwnd );
          return 0;

        case BMP_PL:
          pl_show( !pl_is_visible());
          return 0;

        case BMP_REPEAT:
          cfg.rpt = !cfg.rpt;

          if( cfg.rpt ) {
            WinSendDlgItemMsg( hwnd, BMP_REPEAT, WM_PRESS,   0, 0 );
          } else {
            WinSendDlgItemMsg( hwnd, BMP_REPEAT, WM_DEPRESS, 0, 0 );
          }

          amp_invalidate( UPD_FILEINFO | UPD_TIMERS );
          return 0;

        case BMP_SHUFFLE:
          cfg.shf =  !cfg.shf;

          if( amp_playmode == AMP_PLAYLIST ) {
            pl_clean_shuffle();
          }
          if( cfg.shf ) {
            if( decoder_playing()) {
              pl_mark_as_play();
            }
            WinSendDlgItemMsg( hwnd, BMP_SHUFFLE, WM_PRESS,   0, 0 );
          } else {
            WinSendDlgItemMsg( hwnd, BMP_SHUFFLE, WM_DEPRESS, 0, 0 );
          }
          return 0;

        case IDM_M_QUIT:
        case BMP_POWER:
          WinPostMsg( hwnd, WM_QUIT, 0, 0 );
          return 0;

        case IDM_M_PLAY:
        case BMP_PLAY:
          if( !decoder_playing()) {
            amp_pb_play( 0 );
          } else {
            WinSendMsg( hwnd, WM_COMMAND, MPFROMSHORT( BMP_STOP ), mp2 );
          }
          return 0;

        case IDM_M_PAUSE:
        case BMP_PAUSE:
          amp_pb_pause();
          return 0;

        case BMP_FLOAD:
          amp_pb_load_file(hwnd);
          return 0;

        case BMP_STOP:
          amp_pb_stop();
          pl_clean_shuffle();
          return 0;

        case IDM_M_NEXT:
        case BMP_NEXT:
          if( amp_playmode == AMP_PLAYLIST ) {
            if( pl_load_next_record()) {
              if( decoder_playing()) {
                amp_pb_stop();
                amp_pb_play( 0 );
              }
            }
          }
          return 0;

        case IDM_M_PREV:
        case BMP_PREV:
          if( amp_playmode == AMP_PLAYLIST ) {
            if( pl_load_prev_record()) {
              if( decoder_playing()) {
                amp_pb_stop();
                amp_pb_play( 0 );
              }
            }
          }
          return 0;

        case IDM_M_FWD:
        case BMP_FWD:
          if( decoder_playing() && !is_paused())
          {
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
            msg_forward();
            WinSendDlgItemMsg( hwnd, BMP_FWD, is_forward() ? WM_PRESS : WM_DEPRESS, 0, 0 );
            amp_pb_volume_adjust();
          } else {
            WinSendDlgItemMsg( hwnd, BMP_FWD, WM_DEPRESS, 0, 0 );
          }
          return 0;

        case IDM_M_REW:
        case BMP_REW:
          if( decoder_playing() && !is_paused())
          {
            WinSendDlgItemMsg( hwnd, BMP_FWD, WM_DEPRESS, 0, 0 );
            msg_rewind();
            WinSendDlgItemMsg( hwnd, BMP_REW, is_rewind() ? WM_PRESS : WM_DEPRESS, 0, 0 );
            amp_pb_volume_adjust();
          } else {
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
          }
          return 0;
      }
      return 0;

    case WM_CREATE:
    {
      HPS hps = WinGetPS( hwnd );
      bmp_load_skin( cfg.defskin, hab, hwnd, hps );
      WinReleasePS( hps );

      if( cfg.floatontop ) {
        WinStartTimer( hab, hwnd, TID_ONTOP, 100 );
      }

      if( cfg.rpt ) {
        WinSendDlgItemMsg( hwnd, BMP_REPEAT,  WM_PRESS, 0, 0 );
      }
      if( cfg.shf || is_arg_shuffle ) {
        WinSendDlgItemMsg( hwnd, BMP_SHUFFLE, WM_PRESS, 0, 0 );
        cfg.shf = TRUE;
      }

      WinStartTimer( hab, hwnd, TID_UPDATE_TIMERS, 100 );
      WinStartTimer( hab, hwnd, TID_UPDATE_PLAYER,  50 );
      break;
    }

    case WM_BUTTON1DBLCLK:
    case WM_BUTTON1CLICK:
    {
      POINTL pos;

      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      if( bmp_pt_in_volume( pos )) {
        amp_pb_set_volume( bmp_calc_volume( pos ));
      } else {
        if( amp_playmode != AMP_NOFILE && bmp_pt_in_text( pos )) {
          WinPostMsg( hwnd, AMP_DISPLAY_MODE, 0, 0 );
        }
      }

      return 0;
    }

    case WM_MOUSEMOVE:
      if( is_volume_drag )
      {
        POINTL pos;

        pos.x = SHORT1FROMMP(mp1);
        pos.y = SHORT2FROMMP(mp1);

        amp_pb_set_volume( bmp_calc_volume( pos ));
      }

      if( is_slider_drag )
      {
        HPS    hps = WinGetPS( hwnd );
        POINTL pos;

        pos.x = SHORT1FROMMP(mp1);
        pos.y = SHORT2FROMMP(mp1);
        seeking_pos = bmp_calc_time( pos, time_total());

        bmp_draw_slider( hps, seeking_pos, time_total());
        bmp_draw_timer ( hps, seeking_pos );
        WinReleasePS( hps );
      }

      WinSetPointer( HWND_DESKTOP, WinQuerySysPointer( HWND_DESKTOP, SPTR_ARROW, FALSE ));
      return 0;

    case WM_BUTTON1MOTIONEND:
      if( is_volume_drag ) {
        is_volume_drag = FALSE;
        WinSetCapture( HWND_DESKTOP, NULLHANDLE );
      }
      if( is_slider_drag )
      {
        POINTL pos;

        pos.x = SHORT1FROMMP(mp1);
        pos.y = SHORT2FROMMP(mp1);
        seeking_pos = bmp_calc_time( pos, time_total());

        msg_seek( seeking_pos * 1000 );
        is_slider_drag = FALSE;
        WinSetCapture( HWND_DESKTOP, NULLHANDLE );
      }
      return 0;

    case WM_BUTTON1MOTIONSTART:
    {
      POINTL pos;

      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      if( bmp_pt_in_volume( pos )) {
        is_volume_drag = TRUE;
        WinSetCapture( HWND_DESKTOP, hwnd );
      } else if( bmp_pt_in_slider( pos ) && decoder_playing()) {
        if( time_total() > 0 ) {
          is_slider_drag = TRUE;
          is_seeking     = TRUE;
          WinSetCapture( HWND_DESKTOP, hwnd );
        }
      } else {
        WinSendMsg( hframe, WM_TRACKFRAME, MPFROMSHORT( TF_MOVE | TF_STANDARD ), 0 );
        WinQueryWindowPos( hframe, &cfg.main );
      }
      return 0;
    }

    case WM_BUTTON2MOTIONSTART:
      WinSendMsg( hframe, WM_TRACKFRAME, MPFROMSHORT( TF_MOVE | TF_STANDARD ), 0 );
      WinQueryWindowPos( hframe, &cfg.main );
      return 0;

    case WM_CHAR:
      if(!( SHORT1FROMMP(mp1) & KC_KEYUP ) && ( SHORT1FROMMP(mp1) & KC_VIRTUALKEY )) {
        if( SHORT2FROMMP(mp2) == VK_ESC ) {
          is_slider_drag = FALSE;
          is_seeking     = FALSE;
        }
      }
      break;
  }

  return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

static USHORT
amp_message_box( HWND owner, const char* title,
                             const char* message, ULONG style  )
{
  char padded_title[60];
  sprintf( padded_title, "%-59s", title );

  if( owner == NULLHANDLE )
  {
    owner  =  HWND_DESKTOP;
    style &= ~MB_APPLMODAL;
  } else {
    style |=  MB_APPLMODAL;
  }

  return WinMessageBox( HWND_DESKTOP, owner, (PSZ)message,
                                      padded_title, 0, style );
}

/* Creates and displays a error message window.
   Use the player window as message window owner. */
void
amp_player_error( const char* format, ... )
{
  char message[4096];
  va_list args;

  va_start( args, format );
  vsprintf( message, format, args );

  amp_message_box( hframe, "PM123 Error", message,
                   MB_ERROR | MB_OK | MB_MOVEABLE );
}

/* Creates and displays a error message window.
   The specified owner window is disabled. */
void
amp_error( HWND owner, const char* format, ... )
{
  char message[4096];
  va_list args;

  va_start( args, format );
  vsprintf( message, format, args );

  amp_message_box( owner, "PM123 Error", message,
                   MB_ERROR | MB_OK | MB_MOVEABLE );
}

/* Creates and displays a message window. */
void
amp_info( HWND owner, const char* format, ... )
{
  char message[4096];
  va_list args;

  va_start( args, format );
  vsprintf( message, format, args );

  amp_message_box( owner, "PM123 Information", message,
                   MB_INFORMATION | MB_OK | MB_MOVEABLE );
}

/* Requests the user about specified action. Returns
   TRUE at confirmation or FALSE in other case. */
BOOL
amp_query( HWND owner, const char* format, ... )
{
  char message[4096];
  va_list args;

  va_start( args, format );
  vsprintf( message, format, args );

  return amp_message_box( owner, "PM123 Query", message,
                          MB_QUERY | MB_YESNO | MB_MOVEABLE ) == MBID_YES;
}

/* Requests the user about overwriting a file. Returns
   TRUE at confirmation or at absence of a file. */
BOOL
amp_warn_if_overwrite( HWND owner, const char* filename )
{
  char message[4096];
  struct stat fi;

  sprintf( message, "File %s already exists. Overwrite it?", filename );

  if( stat( filename, &fi ) == 0 ) {
    return amp_query( owner, "File %s already exists. Overwrite it?", filename );
  } else {
    return TRUE;
  }
}

/* Tells the help manager to display a specific help window. */
void
amp_show_help( SHORT resid )
{
  WinSendMsg( hhelp, HM_DISPLAY_HELP, MPFROMLONG( MAKELONG( resid, NULL )),
                                      MPFROMSHORT( HM_RESOURCEID ));
}

/* Parses and processes a commandline arguments. Must be
   called from the main thread. */
static void
amp_pb_process_file_arguments( HPIPE hpipe, int files, int argc, char *argv[] )
{
  char command[1024];
  char curpath[_MAX_PATH];
  char file   [_MAX_PATH];
  int  i;

  ASSERT_IS_MAIN_THREAD;
  getcwd( curpath, sizeof( curpath ));

  if( files > 1 && hpipe != NULLHANDLE ) {
    amp_pb_pipe_write( hpipe, "*clear" );
  }

  for( i = 1; i < argc; i++ ) {
    if( *argv[i] != '/' && *argv[i] != '-' ) {
      if( !rel2abs( curpath, argv[i], file, sizeof(file))) {
        strcpy( file, argv[i] );
      }
      if( is_dir( file )) {
        if( hpipe == NULLHANDLE ) {
          pl_add_directory( file, PL_DIR_RECURSIVE );
        } else {
          snprintf( command, sizeof( command ), "*rdir %s", file );
          amp_pb_pipe_write( hpipe, command );
        }
      } else {
        if( files == 1 ) {
          if( hpipe == NULLHANDLE ) {
            amp_pb_load_singlefile( file, 0 );
          } else {
            snprintf( command, sizeof( command ), "*load %s", file );
            amp_pb_pipe_write( hpipe, command );
          }
        } else {
          if( hpipe == NULLHANDLE ) {
            if( is_playlist( file )) {
              pl_load( file, PL_LOAD_NOT_RECALL | PL_LOAD_CONTINUE );
            } else {
              pl_add_file( file, NULL, 0 );
            }
          } else {
            snprintf( command, sizeof( command ), "*add %s", file );
            amp_pb_pipe_write( hpipe, command );
          }
        }
      }
    }
  }

  if( files > 1 && hpipe == NULLHANDLE ) {
    pl_completed();
  }
}

/* The main PM123 entry point. */
int DLLENTRY
dll_main( int argc, char *argv[] )
{
  HMQ    hmq;
  int    i, o, files = 0;
  char   exename[_MAX_PATH];
  char   bundle [_MAX_PATH];
  char   infname[_MAX_PATH];
  char   module [_MAX_PATH];
  char   command[1024];
  QMSG   qmsg;
  struct stat fi;

  HELPINIT hinit     = { 0 };
  ULONG    flCtlData = FCF_TASKLIST | FCF_NOBYTEALIGN | FCF_ACCELTABLE | FCF_ICON;

  // used for debug printf()s
  setvbuf( stdout, NULL, _IONBF, 0 );
  setvbuf( stderr, NULL, _IONBF, 0 );

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  getExeName( exename, sizeof( exename ));
  getModule( &hmodule, module, sizeof( module ));
  sdrivedir( startpath, exename, sizeof( startpath ));

  for( o = 1; o < argc; o++ )
  {
    if( stricmp( argv[o], "-shuffle" ) == 0 ||
        stricmp( argv[o], "/shuffle" ) == 0  )
    {
      is_arg_shuffle = TRUE;
    }
    else if( stricmp( argv[o], "-smooth" ) == 0 ||
             stricmp( argv[o], "/smooth" ) == 0  )
    {
      // Not supported since 1.32
      // is_arg_smooth  = TRUE;
    }
    else if( stricmp( argv[o], "-cmd" ) == 0 ||
             stricmp( argv[o], "/cmd" ) == 0  )
    {
      o++;
      if( strncmp( argv[o], "\\\\", 2 ) == 0 )
      {
        strlcpy( pipename, argv[o], sizeof( pipename ));  // machine name
        strlcat( pipename, "\\PIPE\\PM123", sizeof( pipename ));
        o++;
      }
      strcpy( command, "*" );
      for( i = o; i < argc; i++ )
      {
        strlcat( command, argv[i], sizeof( command ));
        strlcat( command, " ", sizeof( command ));
      }

      amp_pb_pipe_open_and_write( pipename, command );
      exit(0);
    } else if( *argv[o] != '/' && *argv[o] != '-' ) {
      files++;
    }
  }

  // If we have files in argument, try to open \\pipe\pm123 and write to it.
  if( files > 0 ) {
    if(( hpipe = amp_pb_pipe_open( pipename )) != NULLHANDLE ) {
      amp_pb_process_file_arguments( hpipe, files, argc, argv );
      amp_pb_pipe_close( hpipe );
      exit(0);
    }
  }

  if(( hpipe = amp_pb_pipe_create()) == NULLHANDLE ) {
    exit(1);
  }

  pg_init();
  srand((unsigned long)time( NULL ));
  load_ini();

  xio_set_http_proxy_host( cfg.proxy_host );
  xio_set_http_proxy_port( cfg.proxy_port );
  xio_set_http_proxy_user( cfg.proxy_user );
  xio_set_http_proxy_pass( cfg.proxy_pass );
  xio_set_buffer_size( cfg.buff_size * 1024 );
  xio_set_buffer_wait( cfg.buff_wait );
  xio_set_buffer_fill( cfg.buff_fill );

  InitButton( hab );
  WinRegisterClass( hab, "PM123", amp_pb_dlg_proc, CS_SIZEREDRAW /* | CS_SYNCPAINT */, 0 );

  hframe = WinCreateStdWindow( HWND_DESKTOP, 0, &flCtlData, "PM123",
                               AMP_FULLNAME, 0, hmodule, WIN_MAIN, &hplayer );
  do_warpsans( hframe  );
  do_warpsans( hplayer );

  dk_init();
  dk_add_window( hframe, DK_IS_MASTER );

  mp3       = WinLoadPointer( HWND_DESKTOP, hmodule, ICO_MP3     );
  mp3play   = WinLoadPointer( HWND_DESKTOP, hmodule, ICO_MP3PLAY );
  mp3gray   = WinLoadPointer( HWND_DESKTOP, hmodule, ICO_MP3GRAY );

  pm_create();
  bm_create();
  pl_create();

  strlcpy( infname, startpath,   sizeof( infname ));
  strlcat( infname, "pm123.inf", sizeof( infname ));

  if( stat( infname, &fi ) != 0  ) {
    // If the file of the help does not placed together with the program,
    // we shall give to the help manager to find it.
    strcpy( infname, "pm123.inf" );
  }

  hinit.cb = sizeof( hinit );
  hinit.phtHelpTable = (PHELPTABLE)MAKELONG( HLP_MAIN, 0xFFFF );
  hinit.hmodHelpTableModule = hmodule;
  hinit.pszHelpWindowTitle = "PM123 Help";
  hinit.fShowPanelId = CMIC_SHOW_PANEL_ID;
  hinit.pszHelpLibraryName = infname;

  if(( hhelp = WinCreateHelpInstance( hab, &hinit )) == NULLHANDLE ) {
    amp_error( hplayer, "Error create help instance: %s", infname );
  } else {
    WinAssociateHelpInstance( hhelp, hframe );
  }

  strlcpy( bundle, startpath,   sizeof( bundle ));
  strlcat( bundle, "pm123.lst", sizeof( bundle ));

  if( files > 0 ) {
    amp_pb_process_file_arguments( NULLHANDLE, files, argc, argv );
  } else {
    amp_pb_display_filename();
    if( stat( bundle, &fi ) == 0 ) {
      pl_load_bundle( bundle, 0 );
    }
  }

  WinSetWindowPos( hframe, HWND_TOP,
                   cfg.main.x, cfg.main.y, 0, 0, SWP_ACTIVATE | SWP_MOVE | SWP_SHOW );

  if( cfg.dock_windows ) {
    dk_arrange( hframe );
  }

  amp_pb_load_eq_file( cfg.lasteq, gains, mutes, &preamp );
  amp_pb_volume_adjust();

  if( hpipe ) {
    if(( pipe_tid = _beginthread( amp_pipe_thread, NULL, 65535, NULL )) == -1 ) {
      amp_error( hplayer, "Unable start the service thread." );
    }
  }

  vis_initialize_all( hplayer, FALSE );
  tag_init();

  while( WinGetMsg( hab, &qmsg, (HWND)0, 0, 0 )) {
    WinDispatchMsg( hab, &qmsg );
  }

  amp_pb_stop();
  is_terminate = TRUE;

  if( pipe_tid != -1 ) {
    amp_pb_pipe_open_and_write( pipename, "*status" );
    wait_thread( pipe_tid, 2000 );
    amp_pb_pipe_close( hpipe );
  }

  pl_save_bundle( bundle, 0 );
  pl_destroy();
  bm_destroy();
  pm_destroy();
  save_ini();
  tag_term();
  bmp_clean_skin();
  dk_term();
  pg_cleanup_plugin_menu( mn_get_submenu( hmenu, IDM_M_PLUGINS ));
  pg_term();

  if( heq != NULLHANDLE ) {
    WinDestroyWindow( heq );
  }

  WinDestroyPointer ( mp3     );
  WinDestroyPointer ( mp3play );
  WinDestroyPointer ( mp3gray );
  WinDestroyWindow  ( hframe  );
  WinDestroyWindow  ( hmenu   );
  WinDestroyMsgQueue( hmq     );
  WinTerminate( hab );

  #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
  #endif
  return 0;
}
