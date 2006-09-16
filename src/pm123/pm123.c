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

#include <utilfct.h>
#include <format.h>
#include "pm123.h"
#include "plugman.h"
#include "bookmark.h"
#include "button95.h"
#include "pfreq.h"
#include "httpget.h"
#include "genre.h"
#include "copyright.h"
#include "docking.h"

#include <debuglog.h>

#define  AMP_REFRESH_CONTROLS ( WM_USER + 121   )
#define  AMP_DISPLAYMSG       ( WM_USER + 76    )
#define  AMP_PAINT            ( WM_USER + 77    )
#define  AMP_STOP             ( WM_USER + 78    )

#define  TID_UPDATE_TIMERS    ( TID_USERMAX - 1 )
#define  TID_UPDATE_PLAYER    ( TID_USERMAX - 2 )
#define  TID_ONTOP            ( TID_USERMAX - 3 )

int       amp_playmode = AMP_NOFILE;
HPOINTER  mp3;      /* Song file icon   */
HPOINTER  mp3play;  /* Played file icon */
HPOINTER  mp3gray;  /* Broken file icon */

/* Contains startup path of the program without its name.  */
char   startpath[_MAX_PATH];
/* Contains a name of the currently loaded file. */
// TODO: too short for an URL
char   current_filename[_MAX_PATH];
/* Other parameters of the currently loaded file. */
int    current_bitrate     =  0;
int    current_channels    = -1;
int    current_length      =  0;
int    current_freq        =  0;
char   current_track       =  0;
char   current_cd_drive[4] = "";
tune   current_tune;
char   current_decoder[128];
char   current_decoder_info_string[128];
static FORMAT_INFO current_format; /* current data format */

static HAB   hab        = NULLHANDLE;
static HWND  hplaylist  = NULLHANDLE;
static HWND  heq        = NULLHANDLE;
static HWND  hframe     = NULLHANDLE;
static HWND  hplayer    = NULLHANDLE;
static HWND  hhelp      = NULLHANDLE;
static HPIPE hpipe      = NULLHANDLE;

/* Pipe name decided on startup. */
static char  pipename[_MAX_PATH] = "\\PIPE\\PM123";

static BOOL  is_have_focus    = FALSE;
static BOOL  is_volume_drag   = FALSE;
static BOOL  is_seeking       = FALSE;
static BOOL  is_slider_drag   = FALSE;
static BOOL  is_stream_saved  = FALSE;
static BOOL  is_arg_shuffle   = FALSE;

/* Current seeking time. Valid if is_seeking == TRUE. */
static int   seeking_pos = 0;
static int   upd_options = 0;

/* Equalizer stuff. */
float gains[20];
BOOL  mutes[20];
float preamp;

static char last_error[2048];


void PM123_ENTRY
keep_last_error( char *error )
{
  strlcpy( last_error, error, sizeof( last_error ));
  amp_post_message( WM_PLAYERROR, 0, 0 );
}

void PM123_ENTRY
display_info( char *info )
{
  char* info_display = strdup( info );
  if( info_display ) {
    amp_post_message( AMP_DISPLAYMSG, MPFROMP( info_display ), 0 );
  }
}

/* Returns a playing time of the current file, in seconds. */
static int
time_played( void ) {
  return out_playing_pos()/1000;
}

/* Returns a total playing time of the current file. */
static int
time_total( void ) {
  return dec_length()/1000;
}

/* Sets audio volume to the current selected level. */
void
amp_volume_to_normal( void )
{
  int i = cfg.defaultvol;
  if( i > 100 ) {
      i = 100;
  }
  out_set_volume( i/100. );
}

/* Sets audio volume to below current selected level. */
void
amp_volume_to_lower( void )
{
  int i = cfg.defaultvol;
  if( i > 100 ) {
      i = 100;
  }
  out_set_volume(i*(3./500.));
}

/* Adjusts audio volume to level accordingly current playing mode. */
static void
amp_volume_adjust( void )
{
  if( is_fast_forward() || is_fast_backward() ) {
    amp_volume_to_lower ();
  } else {
    amp_volume_to_normal();
  }
}

/* Draws all player timers and the position slider. */
static void
amp_paint_timers( HPS hps )
{
  int play_time = 0;
  int play_left = current_length;
  int list_left = 0;

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

/* Draws all attributes of the currently loaded file. */
static void
amp_paint_fileinfo( HPS hps )
{
  if( amp_playmode == AMP_PLAYLIST ) {
    bmp_draw_plind( hps, pl_current_index(), pl_size());
  } else {
    bmp_draw_plind( hps, 0, 0 );
  }

  bmp_draw_plmode  ( hps );
  bmp_draw_timeleft( hps );
  bmp_draw_rate    ( hps, current_bitrate );
  bmp_draw_channels( hps, current_channels );
  bmp_draw_text    ( hps );
  amp_paint_timers ( hps );
}

/* Marks the player window as needed of redrawing. */
void
amp_invalidate( int options )
{
  if( options == UPD_ALL ) {
    WinInvalidateRect( hplayer, NULL, 1 );
  } else if( options & UPD_DELAYED ) {
    upd_options |= ( options & ~UPD_DELAYED );
  } else {
    WinPostMsg( hplayer, AMP_PAINT, MPFROMLONG( options ), 0 );
  }
}

/* Returns the handle of the player window. */
HWND
amp_player_window( void )
{
  return hplayer;
}

/* Returns the anchor-block handle. */
HAB
amp_player_hab( void )
{
  return hab;
}

/* Posts a message to the message queue associated with
   the player window. */
BOOL
amp_post_message( ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  return WinPostMsg( hplayer, msg, mp1, mp2 );
}

/* Sets bubble text for specified button. */
static void
amp_set_bubbletext( USHORT id, const char *text )
{
  WinPostMsg( WinWindowFromID( hplayer, id ),
                               WM_SETTEXT, MPFROMP(text), 0 );
}

/* Loads the specified playlist record into the player. */
BOOL
amp_pl_load_record( PLRECORD* rec )
{
  DECODER_INFO info;
  struct stat  fi;

  if( !rec ) {
    return FALSE;
  }

  if( is_file( rec->full ) && stat( rec->full, &fi ) != 0 ) {
    amp_error( hplayer, "Unable load file:\n%s\n%s", rec->full, clib_strerror(errno));
    return FALSE;
  }

  strcpy( current_filename, rec->full );
  strcpy( current_decoder,  rec->decoder_module_name );
  strcpy( current_cd_drive, rec->cd_drive );
  strcpy( current_decoder_info_string, rec->info_string );

  if( is_track( current_filename )) {
    dec_trackinfo( rec->cd_drive, rec->track, &info, NULL );
  } else {
    dec_fileinfo ( current_filename, &info, NULL );
  }

  amp_gettag( current_filename, &info, &current_tune );

  if( is_http( current_filename ) && !*current_tune.title ) {
    strlcpy( current_tune.title, rec->songname, sizeof( current_tune.title ));
  }

  current_format   = rec->format;

  current_bitrate  = rec->bitrate;
  current_channels = rec->channels;
  current_length   = info.songlength/1000;
  current_freq     = rec->freq;
  current_track    = rec->track;

  if( amp_playmode == AMP_PLAYLIST ) {
    current_record = rec;
  } else {
    amp_playmode = AMP_SINGLE;
  }

  amp_display_filename();
  amp_invalidate( UPD_FILEINFO );
  return TRUE;
}

/* Loads the specified playlist record into the player and
   plays it if this is specified in the player properties or
   the player is already playing. */
void
amp_pl_play_record( PLRECORD* rec )
{
  BOOL decoder_was_playing = decoder_playing();

  if( decoder_was_playing ) {
    amp_stop();
  }

  if( rec  ) {
    if( amp_pl_load_record( rec )) {
      if( cfg.playonload == 1 || decoder_was_playing ) {
        amp_play();
      }
    }
  }
}

/* Activates the current playlist. */
void
amp_pl_use( void )
{
  BOOL rc = TRUE;

  if( pl_size()) {
    if( amp_playmode == AMP_SINGLE ) {
      current_record = pl_query_file_record( current_filename );
      if( decoder_playing()) {
        if( !current_record ) {
          amp_stop();
        } else {
          pl_mark_as_play();
        }
      }
    }

    amp_playmode = AMP_PLAYLIST;
    pl_display_status();

    if( !current_record ) {
      rc = amp_pl_load_record( pl_query_first_record());
    }

    if( rc && cfg.playonuse && !decoder_playing()) {
      amp_play();
    }

    amp_invalidate( UPD_FILEINFO );
  }
}

/* Deactivates the current playlist. */
void
amp_pl_release( void )
{
  if( amp_playmode != AMP_SINGLE )
  {
    amp_playmode = AMP_SINGLE;
    pl_display_status();

    if( current_record ) {
      pl_mark_as_stop();
      current_record = NULL;
    }

    pl_clean_shuffle();
    amp_invalidate( UPD_FILEINFO );
  }
}

/* Loads a standalone file or CD track to player. */
BOOL
amp_load_singlefile( const char* filename, int options )
{
  DECODER_INFO info;

  int    i;
  ULONG  rc;
  char   module_name[128];
  char   cd_drive[4] = "1:";
  int    cd_track    = -1;
  struct stat fi;

  if( is_playlist( filename )) {
    return pl_load( filename, PL_LOAD_CLEAR );
  }

  if( is_file( filename ) && stat( filename, &fi ) != 0 ) {
    amp_error( hplayer, "Unable load file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  memset( &info, 0, sizeof( info ));

  if( is_track( filename )) {
    sscanf( filename, "cd:///%1c:\\Track %d", cd_drive, &cd_track );
    if( cd_drive[0] == '1' || cd_track == -1 ) {
      amp_error( hplayer, "Invalid CD URL:\n%s", filename );
      return FALSE;
    }
    rc = dec_trackinfo( cd_drive, cd_track, &info, module_name );
  } else {
    rc = dec_fileinfo((char*)filename, &info, module_name );
    cd_drive[0] = 0;
  }

  if( rc != 0 ) { /* error, invalid file */
    amp_reset();
    handle_dfi_error( rc, filename );
    return FALSE;
  }

  amp_stop();
  amp_pl_release();

  // TODO: buffer overflow!
  strcpy( current_filename, filename );
  strcpy( current_decoder, module_name );
  strcpy( current_cd_drive, cd_drive );
  strcpy( current_decoder_info_string, info.tech_info );

  amp_gettag( filename, &info, &current_tune );
  current_format   = info.format;
  current_bitrate  = info.bitrate;
  current_channels = info.mode;
  current_length   = info.songlength / 1000;
  current_freq     = info.format.samplerate;
  current_track    = cd_track;

  amp_display_filename();
  amp_invalidate( UPD_FILEINFO );

  if( !( options & AMP_LOAD_NOT_PLAY )) {
    if( cfg.playonload ) {
      amp_play();
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

/* Stops the playing of the current file. Must be called
   from main thread. */
static void
amp_stop_playing( void )
{
  QMSG qms;

  amp_msg( MSG_STOP, 0, 0 );
  amp_set_bubbletext( BMP_PLAY, "Starts playing" );

  WinSendDlgItemMsg( hplayer, BMP_PLAY,  WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_FWD,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_REW,   WM_DEPRESS, 0, 0 );
  WinSetWindowText ( hframe,  AMP_FULLNAME );

  if( amp_playmode == AMP_PLAYLIST ) {
    pl_mark_as_stop();
  }

  // Discards WM_PLAYSTOP message, posted by decoder.
  WinPeekMsg( hab, &qms, hplayer, WM_PLAYSTOP, WM_PLAYSTOP, PM_REMOVE );
  amp_invalidate( UPD_ALL );
}

/* Stops the player. */
void
amp_stop( void )
{
  // The stop of the player always should be initiated from
  // the main thread. Otherwise we can receive the unnecessary
  // WM_PLAYSTOP message.
  WinSendMsg( hplayer, AMP_STOP, 0, 0 );
}

/* Starts playing of the currently loaded file. */
void
amp_play( void )
{
  MSG_PLAY_STRUCT msgplayinfo = { 0 };
  char caption [_MAX_PATH];
  char filename[_MAX_PATH];

  if( amp_playmode == AMP_NOFILE ) {
    WinSendDlgItemMsg( hplayer, BMP_PLAY, WM_DEPRESS, 0, 0 );
    return;
  }

  msgplayinfo.filename = current_filename;
  msgplayinfo.out_filename = current_filename;
  msgplayinfo.drive = current_cd_drive;
  msgplayinfo.track = current_track;
  msgplayinfo.hMain = hplayer;
  msgplayinfo.decoder_needed = current_decoder;

  amp_msg( MSG_PLAY, &msgplayinfo, &current_format );

  amp_set_bubbletext( BMP_PLAY, "Stops playback" );

  WinSendDlgItemMsg( hplayer, BMP_FWD,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_REW,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PLAY,  WM_PRESS  , 0, 0 );

  if( amp_playmode == AMP_PLAYLIST ) {
    pl_mark_as_play();
  }

  sfnameext( filename, current_filename, sizeof( filename ));
  sprintf( caption, "%s - ", AMP_FULLNAME );
  strlcat( caption, filename, sizeof( filename ));
  WinSetWindowText( hframe, caption );
}

/* Pauses or continues playing of the currently loaded file. */
void
amp_pause( void )
{
  if( decoder_playing())
  {
    amp_msg( MSG_PAUSE, 0, 0 );
  }

  if( is_paused() ) {
    WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_PRESS, 0, 0 );
    return;
  }
}

/* Shows the context menu of the playlist. */
static void
amp_show_context_menu( HWND parent )
{
  static HWND menu = NULLHANDLE;

  POINTL   pos;
  SWP      swp;
  char     file[_MAX_PATH];
  HWND     mh;
  MENUITEM mi;
  short    id;
  int      i;
  int      count;

  if( menu == NULLHANDLE )
  {
    menu = WinLoadMenu( HWND_OBJECT, 0, MNU_MAIN );

    WinSendMsg( menu, MM_QUERYITEM,
                MPFROM2SHORT( IDM_M_LOAD, TRUE ), MPFROMP( &mi ));

    WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
      WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

    WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                                MPFROMLONG( IDM_M_LOADFILE ), 0 );
  }

  WinQueryPointerPos( HWND_DESKTOP, &pos );
  WinMapWindowPoints( HWND_DESKTOP, parent, &pos, 1 );

  if( WinWindowFromPoint( parent, &pos, TRUE ) == NULLHANDLE )
  {
    // The context menu is probably activated from the keyboard.
    WinQueryWindowPos( parent, &swp );
    pos.x = swp.cx/2;
    pos.y = swp.cy/2;
  }

  // Update regulars.
  WinSendMsg( menu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_LOAD, TRUE ), MPFROMP( &mi ));

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

        if( is_http( cfg.last[i] )) {
          strcat( file, "[http] " );
        }

        sfnameext( file + strlen( file ), cfg.last[i], sizeof( file ) - strlen( file ));

        mi.iPosition = MIT_END;
        mi.afStyle = MIS_TEXT;
        mi.afAttribute = 0;
        mi.id = (IDM_M_LAST + 1) + i;
        mi.hwndSubMenu = (HWND)NULLHANDLE;
        mi.hItem = 0;

        WinSendMsg( mh, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( file ));
      }
    }
  }

  // Update bookmarks.
  WinSendMsg( menu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_BOOKMARKS, TRUE ), MPFROMP( &mi ));

  load_bookmark_menu( mi.hwndSubMenu );

  // Update plug-ins.
  WinSendMsg( menu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_PLUG, TRUE ), MPFROMP( &mi ));

  load_plugin_menu( mi.hwndSubMenu );

  // Update status
  mn_enable_item( menu, IDM_M_TAG,     is_file( current_filename ));
  mn_enable_item( menu, IDM_M_SMALL,   bmp_is_mode_supported( CFG_MODE_SMALL   ));
  mn_enable_item( menu, IDM_M_NORMAL,  bmp_is_mode_supported( CFG_MODE_REGULAR ));
  mn_enable_item( menu, IDM_M_TINY,    bmp_is_mode_supported( CFG_MODE_TINY    ));
  mn_enable_item( menu, IDM_M_FONT,    cfg.font_skinned );
  mn_enable_item( menu, IDM_M_FONT1,   bmp_is_font_supported( 0 ));
  mn_enable_item( menu, IDM_M_FONT2,   bmp_is_font_supported( 1 ));
  mn_enable_item( menu, IDM_M_ADDBOOK, amp_playmode != AMP_NOFILE );

  mn_check_item ( menu, IDM_M_FLOAT,   cfg.floatontop  );
  mn_check_item ( menu, IDM_M_SAVE,    is_stream_saved );
  mn_check_item ( menu, IDM_M_FONT1,   cfg.font == 0   );
  mn_check_item ( menu, IDM_M_FONT2,   cfg.font == 1   );
  mn_check_item ( menu, IDM_M_SMALL,   cfg.mode == CFG_MODE_SMALL   );
  mn_check_item ( menu, IDM_M_NORMAL,  cfg.mode == CFG_MODE_REGULAR );
  mn_check_item ( menu, IDM_M_TINY,    cfg.mode == CFG_MODE_TINY    );

  WinPopupMenu( parent, parent, menu, pos.x, pos.y, 0,
                PU_HCONSTRAIN   | PU_VCONSTRAIN |
                PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
}

/* Adds HTTP file to the playlist or load it to the player. */
void
amp_add_url( HWND owner, int options )
{
  char  url[512];
  HWND  hwnd =
        WinLoadDlg( HWND_DESKTOP, owner, WinDefDlgProc, NULLHANDLE, DLG_URL, 0 );

  if( hwnd == NULLHANDLE ) {
    return;
  }

  WinSendDlgItemMsg( hwnd, ENT_URL, EM_SETTEXTLIMIT, MPFROMSHORT(512), 0 );
  do_warpsans( hwnd );

  if( WinProcessDlg( hwnd ) == DID_OK ) {
    WinQueryDlgItemText( hwnd, ENT_URL, 1024, url );
    if( options & URL_ADD_TO_LIST ) {
      if( is_playlist( url )) {
        pl_load( url, 0);
      } else {
        pl_add_file( url, NULL, 0 );
      }
    } else {
      amp_load_singlefile( url, 0 );
    }
  }
  WinDestroyWindow( hwnd );
}

/* Processes messages of the dialog of addition of CD tracks. */
static MRESULT EXPENTRY
amp_add_tracks_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
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

/* Adds CD tracks to the playlist or load one to the player. */
void
amp_add_tracks( HWND owner, int options )
{
  HFILE hcdrom;
  ULONG action;
  HWND  hwnd =
        WinLoadDlg( HWND_DESKTOP, owner, amp_add_tracks_dlg_proc, NULLHANDLE, DLG_TRACK, 0 );

  if( hwnd == NULLHANDLE ) {
    return;
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
}

/* Adds user selected files or directory to the playlist. */
void
amp_add_files( HWND hwnd )
{
  FILEDLG filedialog;

  int   i = 0;
  char* file;
  char  type_audio[2048];
  char  type_all  [2048];
  APSZ  types[4] = {{ 0 }};

  types[0][0] = type_audio;
  types[1][0] = FDT_PLAYLIST;
  types[2][0] = type_all;

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM | FDS_MULTIPLESEL;
  filedialog.ulUser         = FDU_DIR_ENABLE | FDU_RECURSEBTN;
  filedialog.pszTitle       = "Add file(s) to playlist";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = type_audio;

  strcpy( type_audio, FDT_AUDIO );
  dec_fill_types( type_audio + strlen( type_audio ),
                  sizeof( type_audio ) - strlen( type_audio ) - 1 );
  strcat( type_audio, ")" );

  strcpy( type_all, FDT_AUDIO_ALL );
  dec_fill_types( type_all + strlen( type_all ),
                  sizeof( type_all ) - strlen( type_all ) - 1 );
  strcat( type_all, ")" );

  strcat( filedialog.szFullFile, cfg.filedir );
  WinFileDlg( HWND_DESKTOP, hwnd, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    if( filedialog.ulFQFCount > 1 ) {
      file = (*filedialog.papszFQFilename)[i];
    } else {
      file = filedialog.szFullFile;
    }

    while( *file ) {
      if( is_dir( file )) {
        pl_add_directory( file, filedialog.ulUser & FDU_RECURSE_ON ? PL_DIR_RECURSIVE : 0 );
        strcpy( cfg.filedir, file );
        if( !is_root( file )) {
          strcat( cfg.filedir, "\\" );
        }
      } else if( is_playlist( file )) {
        pl_load( file, PL_LOAD_NOT_RECALL );
      } else {
        pl_add_file( file, NULL, 0 );
        sdrivedir( cfg.filedir, file, sizeof( cfg.filedir ));
      }

      if( ++i >= filedialog.ulFQFCount ) {
        break;
      } else {
        file = (*filedialog.papszFQFilename)[i];
      }
    }
    pl_completed();
  }

  WinFreeFileDlgList( filedialog.papszFQFilename );
}

/* Prepares the player to the drop operation. */
static MRESULT
amp_drag_over( HWND hwnd, PDRAGINFO pdinfo )
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
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ) &&
      ( pdinfo->cditem > 1 && pdinfo->hwndSource == hplaylist )) {

      drag    = DOR_NEVERDROP;
      drag_op = DO_UNKNOWN;
      break;

    } else if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ) ||
               DrgVerifyRMF( pditem, "DRM_123FILE", NULL )) {

      drag    = DOR_DROP;
      drag_op = DO_COPY;

    } else {

      drag    = DOR_NEVERDROP;
      drag_op = DO_UNKNOWN;
      break;
    }
  }

  DrgFreeDraginfo( pdinfo );
  return MPFROM2SHORT( drag, drag_op );
}

/* Receives dropped files or playlist records. */
static MRESULT
amp_drag_drop( HWND hwnd, PDRAGINFO pdinfo )
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
      if( is_dir( fullname )) {
        pl_add_directory( fullname, PL_DIR_RECURSIVE );
      } else if( is_playlist( fullname )) {
        pl_load( fullname, 0 );
      } else if( pdinfo->cditem == 1 ) {
        amp_load_singlefile( fullname, 0 );
      } else {
        pl_add_file( fullname, NULL, 0 );
      }
    }
    else if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ))
    {
      if( pdinfo->cditem == 1 ) {
        if( pdinfo->hwndSource == hplaylist && amp_playmode == AMP_PLAYLIST ) {
          amp_pl_play_record((PLRECORD*)pditem->ulItemID );
        } else {
          amp_load_singlefile( fullname, 0 );
        }
      } else {
        pl_add_file( fullname, NULL, 0 );
      }
    }
  }

  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );
  return 0;
}

/* local structure to pass information through WinSetWindowPtr */
typedef struct
{
  tune* tag;
  char  track[4];
} tagdata;

/* Processes messages of the dialog of edition of ID3 tag. */
static MRESULT EXPENTRY
id3_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      int i;

      for( i = 0; i <= GENRE_LARGEST; i++) {
        WinSendDlgItemMsg( hwnd, CB_ID3_GENRE, LM_INSERTITEM,
                           MPFROMSHORT( LIT_END ), MPFROMP( genres[i] ));
      }
      break;
    }

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->cmd == PB_ID3_UNDO )
      {
        tagdata* data = (tagdata*)WinQueryWindowPtr( hwnd, 0 );
        int genre = data->tag->gennum;

        // map all unknown to the text "unknown"
        if ( genre < 0 || genre >= GENRE_LARGEST )
          genre = GENRE_LARGEST;

        if ( data->tag->track <= 0 || data->tag->track > 999 )
          data->track[0] = 0;
        else
          sprintf( data->track, "%d", data->tag->track );

        WinSetDlgItemText( hwnd, EN_ID3_TITLE,   data->tag->title   );
        WinSetDlgItemText( hwnd, EN_ID3_ARTIST,  data->tag->artist  );
        WinSetDlgItemText( hwnd, EN_ID3_ALBUM,   data->tag->album   );
        WinSetDlgItemText( hwnd, EN_ID3_TRACK,   data->track        );
        WinSetDlgItemText( hwnd, EN_ID3_COMMENT, data->tag->comment );
        WinSetDlgItemText( hwnd, EN_ID3_YEAR,    data->tag->year    );

        WinSendDlgItemMsg( hwnd, CB_ID3_GENRE, LM_SELECTITEM,
                           MPFROMSHORT( genre ), MPFROMSHORT( TRUE ));
      }
      return 0;

    case WM_CONTROL:
      if ( SHORT1FROMMP(mp1) == EN_ID3_TRACK && SHORT2FROMMP(mp1) == EN_CHANGE )
      {
        // read the track immediately to verify the syntax
        int tmp_track;
        char buf[4];
        int len1, len2;
        tagdata* data = (tagdata*)WinQueryWindowPtr( hwnd, 0 );

        len1 = WinQueryWindowText( HWNDFROMMP(mp2), sizeof buf, buf );
        if ( len1 != 0 &&                                       // empty is always OK
             ( sscanf( buf, "%u%n", &tmp_track, &len2 ) != 1 || // can't read
               len1      != len2                             || // more input
               tmp_track >= 256 ) )                             // too large
        {
          // bad input, restore last value
          WinSetDlgItemText( hwnd, EN_ID3_TRACK, data->track );
        } else
        {
          // OK, update last value
          strcpy( data->track, buf ); 
        }  
      }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the dialog of edition of ID3 tag. */
static MRESULT EXPENTRY
id3_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    case WM_WINDOWPOSCHANGED:
    {
      RECTL rect;

      WinQueryWindowRect( hwnd, &rect );
      WinCalcFrameRect( hwnd, &rect, TRUE );

      WinSetWindowPos( WinWindowFromID( hwnd, NB_ID3TAG ), 0,
                       rect.xLeft,
                       rect.yBottom,
                       rect.xRight-rect.xLeft,
                       rect.yTop-rect.yBottom, SWP_SIZE | SWP_MOVE );
      break;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Edits a ID3 tag for the specified file. */
void amp_id3_edit( HWND owner, const char* filename )
{
  char    caption[_MAX_FNAME] = "ID3 Tag Editor - ";
  HWND    hwnd;
  HWND    book;
  HWND    page01;
  MRESULT id;
  tune    old_tag;
  tune    new_tag;
  tagdata mywindowdata;

  if( !is_file( filename )) {
    DosBeep( 800, 100 );
    return;
  }

  hwnd = WinLoadDlg( HWND_DESKTOP, owner,
                     id3_dlg_proc, NULLHANDLE, DLG_ID3TAG, 0 );

  sfnameext( caption + strlen( caption ), filename, sizeof( caption ) - strlen( caption ));
  WinSetWindowText( hwnd, caption );

  book = WinWindowFromID( hwnd, NB_ID3TAG );
  do_warpsans( book );

  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROM2SHORT(100,25), MPFROMSHORT(BKA_MAJORTAB));
  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROMLONG(0), MPFROMSHORT(BKA_MINORTAB));
  WinSendMsg( book, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_FIELDBACKGROUND),
              MPFROMSHORT(BKA_BACKGROUNDPAGECOLORINDEX));

  page01 = WinLoadDlg( book, book, id3_page_dlg_proc, NULLHANDLE, DLG_ID3V10, 0 );
  id     = WinSendMsg( book, BKM_INSERTPAGE, MPFROMLONG(0),
                       MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR, BKA_LAST ));
  do_warpsans( page01 );

  WinSendMsg ( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG(id), MPFROMLONG( page01 ));
  WinSendMsg ( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "ID3 Tag" ));
  WinSetOwner( page01, book );

  emptytag( &old_tag );
  emptytag( &new_tag );

  amp_gettag( filename, NULL, &old_tag );
  new_tag.codepage = old_tag.codepage;

  mywindowdata.tag = &old_tag;
  WinSetWindowPtr( page01, 0, &mywindowdata );
  WinPostMsg( page01, WM_COMMAND,
              MPFROMSHORT( PB_ID3_UNDO ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));

  WinProcessDlg( hwnd );

  WinQueryDlgItemText( page01, EN_ID3_TITLE,   sizeof( new_tag.title   ), new_tag.title   );
  WinQueryDlgItemText( page01, EN_ID3_ARTIST,  sizeof( new_tag.artist  ), new_tag.artist  );
  WinQueryDlgItemText( page01, EN_ID3_ALBUM,   sizeof( new_tag.album   ), new_tag.album   );
  WinQueryDlgItemText( page01, EN_ID3_COMMENT, sizeof( new_tag.comment ), new_tag.comment );
  WinQueryDlgItemText( page01, EN_ID3_YEAR,    sizeof( new_tag.year    ), new_tag.year    );

  sscanf( mywindowdata.track, "%u", &new_tag.track );

  new_tag.gennum =
    SHORT1FROMMR( WinSendDlgItemMsg( page01, CB_ID3_GENRE,
                                     LM_QUERYSELECTION, MPFROMSHORT( LIT_CURSOR ), 0 ));
  // keep genres that PM123 does not know
  if ( new_tag.gennum == GENRE_LARGEST && old_tag.gennum < GENRE_LARGEST )
    new_tag.gennum = old_tag.gennum;
  WinDestroyWindow( page01 );
  WinDestroyWindow( hwnd   );

  if( strcmp( old_tag.title,   new_tag.title   ) == 0 &&
      strcmp( old_tag.artist,  new_tag.artist  ) == 0 &&
      strcmp( old_tag.album,   new_tag.album   ) == 0 &&
      strcmp( old_tag.comment, new_tag.comment ) == 0 &&
      strcmp( old_tag.year,    new_tag.year    ) == 0 &&
      old_tag.track  == new_tag.track                 &&
      old_tag.gennum == new_tag.gennum )
  {
    return;
  }

  if( !amp_puttag( filename, &new_tag )) {
    amp_error( owner, "Unable write ID3 tag to file:\n%s\n%s", filename, clib_strerror(errno));
    return;
  }

  amp_gettag( filename, NULL, &new_tag );
  pl_refresh_file( filename );

  if( stricmp( current_filename, filename ) == 0 ) {
    current_tune = new_tag;
    amp_display_filename();
    amp_invalidate( UPD_ALL );
  }
}

/* Wipes a ID3 tag for the specified file. */
void amp_id3_wipe( HWND owner, const char* filename )
{
  tune tag;

  if( !is_file( filename )) {
    DosBeep( 800, 100 );
    return;
  }

  if( !amp_wipetag( filename )) {
    amp_error( owner, "Unable wipe ID3 tag to file:\n%s\n%s", filename, clib_strerror(errno));
    return;
  }

  amp_gettag( filename, NULL, &tag );
  pl_refresh_file( filename );

  if( stricmp( current_filename, filename ) == 0 ) {
    current_tune = tag;
    amp_display_filename();
    amp_invalidate( UPD_ALL );
  }
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
static BOOL
amp_pipe_create( void )
{
  ULONG rc;
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
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Writes data to the specified pipe. */
static void
amp_pipe_write( HPIPE pipe, char *buf )
{
  ULONG action;

  DosWrite( pipe, buf, strlen( buf ) + 1, &action );
  DosResetBuffer( pipe );
}

/* Opens specified pipe and writes data to it. */
static BOOL
amp_pipe_open_and_write( const char* pipename, const char* data, size_t size )
{
  HPIPE  hpipe;
  ULONG  action;
  APIRET rc;

  rc = DosOpen((PSZ)pipename, &hpipe, &action, 0, FILE_NORMAL,
                OPEN_ACTION_FAIL_IF_NEW  | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
                NULL );

  if( rc == NO_ERROR )
  {
    DosWrite( hpipe, (PVOID)data, size, &action );
    DosDisConnectNPipe( hpipe );
    DosClose( hpipe );
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Dispatches requests received from the pipe. */
static void
amp_pipe_thread( void* scrap )
{
  char  buffer [2048];
  char  command[2048];
  char* zork;
  char* dork;
  ULONG bytesread;
  HAB   hab = WinInitialize( 0 );

  WinCreateMsgQueue( hab, 0 );

  for(;;)
  {
    DosDisConnectNPipe( hpipe );
    DosConnectNPipe( hpipe );

    if( DosRead( hpipe, buffer, sizeof( buffer ), &bytesread ) == NO_ERROR )
    {
      buffer[bytesread] = 0;
      blank_strip( buffer );

      if( *buffer && *buffer != '*' )
      {
        if( is_dir( buffer )) {
          pl_clear( PL_CLR_NEW );
          pl_add_directory( buffer, PL_DIR_RECURSIVE );
        } else {
          amp_load_singlefile( buffer, 0 );
        }
      }
      else if( *buffer == '*' )
      {
        strcpy( command, buffer + 1 ); /* Strip the '*' character */
        blank_strip( command );

        zork = strtok( command, " " );
        dork = strtok( NULL,    ""  );

        if( zork )
        {
          if( stricmp( zork, "status" ) == 0 ) {
            if( dork ) {
              if( amp_playmode == AMP_NOFILE ) {
                amp_pipe_write( hpipe, "" );
              } else if( stricmp( dork, "file" ) == 0 ) {
                amp_pipe_write( hpipe, current_filename );
              } else if( stricmp( dork, "tag"  ) == 0 ) {
                char info[512];
                amp_pipe_write( hpipe, amp_construct_tag_string( info,  &current_tune ));
              } else if( stricmp( dork, "info" ) == 0 ) {
                amp_pipe_write( hpipe, current_decoder_info_string );
              }
            }
          }
          if( stricmp( zork, "size" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "regular" ) == 0 ||
                  stricmp( dork, "0"       ) == 0 ||
                  stricmp( dork, "normal"  ) == 0  )
              {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_NORMAL ), 0 );
              }
              if( stricmp( dork, "small"   ) == 0 ||
                  stricmp( dork, "1"       ) == 0  )
              {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_SMALL  ), 0 );
              }
              if( stricmp( dork, "tiny"    ) == 0 ||
                  stricmp( dork, "2"       ) == 0  )
              {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_TINY   ), 0 );
              }
            }
          }
          if( stricmp( zork, "rdir" ) == 0 ) {
            if( dork ) {
              pl_add_directory( dork, PL_DIR_RECURSIVE );
            }
          }
          if( stricmp( zork, "dir"  ) == 0 ) {
            if( dork ) {
              pl_add_directory( dork, 0 );
            }
          }
          if( stricmp( zork, "font" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "1" ) == 0 ) {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FONT1 ), 0 );
              }
              if( stricmp( dork, "2" ) == 0 ) {
                WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FONT2 ), 0 );
              }
            }
          }
          if( stricmp( zork, "add" ) == 0 )
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
                pl_add_file( file, NULL, 0 );
              }
            }
          }
          if( stricmp( zork, "load" ) == 0 ) {
            if( dork ) {
              amp_load_singlefile( dork, 0 );
            }
          }
          if( stricmp( zork, "hide"  ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_MINIMIZE ), 0 );
          }
          if( stricmp( zork, "float" ) == 0 ) {
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
          if( stricmp( zork, "use" ) == 0 ) {
            amp_pl_use();
          }
          if( stricmp( zork, "clear" ) == 0 ) {
            pl_clear( PL_CLR_NEW );
          }
          if( stricmp( zork, "next" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_NEXT ), 0 );
          }
          if( stricmp( zork, "previous" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PREV ), 0 );
          }
          if( stricmp( zork, "remove" ) == 0 ) {
            if( current_record ) {
              PLRECORD* rec = current_record;
              pl_remove_record( &rec, 1 );
            }
          }
          if( stricmp( zork, "forward" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_FWD  ), 0 );
          }
          if( stricmp( zork, "rewind" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_REW  ), 0 );
          }
          if( stricmp( zork, "stop" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_STOP ), 0 );
          }
          if( stricmp( zork, "jump" ) == 0 ) {
            if( dork ) {
              int i = atoi( dork ) * 1000;
              amp_msg( MSG_JUMP, &i, 0 );
            }
          }
          if( stricmp( zork, "play" ) == 0 ) {
            if( dork ) {
              amp_load_singlefile( dork, AMP_LOAD_NOT_PLAY );
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            } else if( !decoder_playing()) {
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            }
          }
          if( stricmp( zork, "pause" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                if( is_paused() ) {
                  WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
                }
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                if( !is_paused() ) {
                  WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
                }
              }
            } else {
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
            }
          }
          if( stricmp( zork, "playonload" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.playonload = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.playonload = TRUE;
              }
            }
          }
          if( stricmp( zork, "autouse" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.autouse = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.autouse = TRUE;
              }
            }
          }
          if( stricmp( zork, "playonuse" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.playonuse = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.playonuse = TRUE;
              }
            }
          }
          if( stricmp( zork, "repeat"  ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_REPEAT  ), 0 );
          }
          if( stricmp( zork, "shuffle" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_SHUFFLE ), 0 );
          }
          if( stricmp( zork, "volume" ) == 0 )
          {
            char buf[64];

            if( dork )
            {
              HPS hps = WinGetPS( hplayer );

              if( *dork == '+' ) {
                cfg.defaultvol += atoi( dork + 1 );
              } else if( *dork == '-' ) {
                cfg.defaultvol -= atoi( dork + 1 );
              } else {
                cfg.defaultvol  = atoi( dork );
              }

              if( cfg.defaultvol > 100 ) {
                  cfg.defaultvol = 100;
              }
              if( cfg.defaultvol < 0   ) {
                  cfg.defaultvol = 0;
              }

              bmp_draw_volume( hps, cfg.defaultvol );
              WinReleasePS( hps );
              amp_volume_adjust();
            }
            amp_pipe_write( hpipe, _itoa( cfg.defaultvol, buf, 10 ));
          }
        }
      }
    }
  }
}

/* Loads a file selected by the user to the player. */
void
amp_load_file( HWND owner )
{
  FILEDLG filedialog;

  char  type_audio[2048];
  char  type_all  [2048];
  APSZ  types[4] = {{ 0 }};

  types[0][0] = type_audio;
  types[1][0] = FDT_PLAYLIST;
  types[2][0] = type_all;

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Load file";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = type_all;

  strcpy( type_audio, FDT_AUDIO     );
  dec_fill_types( type_audio + strlen( type_audio ),
                  sizeof( type_audio ) - strlen( type_audio ) - 1 );
  strcat( type_audio, ")" );

  strcpy( type_all,   FDT_AUDIO_ALL );
  dec_fill_types( type_all   + strlen( type_all   ),
                  sizeof( type_all   ) - strlen( type_all ) - 1 );
  strcat( type_all  , ")" );

  strcpy( filedialog.szFullFile, cfg.filedir );
  WinFileDlg( HWND_DESKTOP, owner, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    sdrivedir( cfg.filedir, filedialog.szFullFile, sizeof( cfg.filedir ));
    amp_load_singlefile( filedialog.szFullFile, 0 );
  }
}

/* Loads a playlist selected by the user to the player. */
void
amp_load_list( HWND owner )
{
  FILEDLG filedialog;
  APSZ types[] = {{ FDT_PLAYLIST }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Load playlist";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_PLAYLIST;

  strcpy( filedialog.szFullFile, cfg.listdir );
  WinFileDlg( HWND_DESKTOP, owner, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
    if( is_playlist( filedialog.szFullFile )) {
      pl_load( filedialog.szFullFile, PL_LOAD_CLEAR );
    }
  }
}

/* Saves current playlist to the file specified by user. */
void
amp_save_list_as( HWND owner, int options )
{
  FILEDLG filedialog;

  APSZ  types[] = {{ FDT_PLAYLIST_LST }, { FDT_PLAYLIST_M3U }, { 0 }};
  char  filez[_MAX_PATH];
  char  ext  [_MAX_EXT ];

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_CUSTOM | FDS_ENABLEFILELB;
  filedialog.pszTitle       = options & SAV_M3U_PLAYLIST ? "Save M3U playlist" : "Save playlist";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.ulUser         = FDU_RELATIVBTN;
  filedialog.papszITypeList = types;

  if( options & SAV_M3U_PLAYLIST ) {
    filedialog.pszIType = FDT_PLAYLIST_M3U;
  } else {
    filedialog.pszIType = FDT_PLAYLIST_LST;
  }

  strcpy( filedialog.szFullFile, cfg.listdir );
  WinFileDlg( HWND_DESKTOP, owner, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
    strcpy( filez, filedialog.szFullFile );
    if( strcmp( sfext( ext, filez, sizeof( ext )), "" ) == 0 ) {
      if( options & SAV_M3U_PLAYLIST ) {
        strcat( filez, ".m3u" );
      } else {
        strcat( filez, ".lst" );
      }
    }
    if( amp_warn_if_overwrite( owner, filez )) {
      pl_save( filez, ( filedialog.ulUser & FDU_RELATIV_ON ? PL_SAVE_RELATIVE : 0 )
                    | ( options & SAV_M3U_PLAYLIST ? PL_SAVE_M3U : PL_SAVE_PLS    ));
    }
  }
}

/* Loads a skin selected by the user. */
static void
amp_loadskin( HAB hab, HWND hwnd, HPS hps )
{
  FILEDLG filedialog;
  APSZ types[] = {{ FDT_SKIN }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Load PM123 skin";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_SKIN;

  sdrivedir( filedialog.szFullFile, cfg.defskin, sizeof( filedialog.szFullFile ));
  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    bmp_load_skin( filedialog.szFullFile, hab, hplayer, hps );
    strcpy( cfg.defskin, filedialog.szFullFile );
  }
}

static BOOL
amp_save_eq( HWND owner, float* gains, BOOL *mutes, float preamp )
{
  FILEDLG filedialog;
  FILE*   file;
  int     i;
  char    ext[_MAX_EXT];
  APSZ    types[] = {{ FDT_EQUALIZER }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Save equalizer";
  filedialog.hMod           = NULLHANDLE;
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

BOOL amp_load_eq_file( char* filename, float* gains, BOOL* mutes, float* preamp )
{
  FILE* file;
  char  vz[CCHMAXPATH];
  int   i;

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
amp_load_eq( HWND hwnd, float* gains, BOOL* mutes, float* preamp )
{
  FILEDLG filedialog;
  APSZ    types[] = {{ FDT_EQUALIZER }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Load equalizer";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_EQUALIZER;

  sdrivedir( filedialog.szFullFile, cfg.lasteq, sizeof( filedialog.szFullFile ));
  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    strcpy( cfg.lasteq, filedialog.szFullFile );
    return amp_load_eq_file( filedialog.szFullFile, gains, mutes, preamp );
  }
  return FALSE;
}

/* Returns if the save stream feature has been enabled. */
static BOOL
amp_save_stream( HWND hwnd, BOOL enable )
{
  if( enable )
  {
    FILEDLG filedialog;

    memset( &filedialog, 0, sizeof( FILEDLG ));
    filedialog.cbSize     = sizeof( FILEDLG );
    filedialog.fl         = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_CUSTOM;
    filedialog.pszTitle   = "Save stream as";
    filedialog.hMod       = NULLHANDLE;
    filedialog.usDlgId    = DLG_FILE;
    filedialog.pfnDlgProc = amp_file_dlg_proc;

    strcpy( filedialog.szFullFile, cfg.savedir );
    WinFileDlg( HWND_DESKTOP, hwnd, &filedialog );

    if( filedialog.lReturn == DID_OK ) {
      if( amp_warn_if_overwrite( hwnd, filedialog.szFullFile ))
      {
        amp_msg( MSG_SAVE, filedialog.szFullFile, 0 );
        sdrivedir( cfg.savedir, filedialog.szFullFile, sizeof( cfg.savedir ));
        return TRUE;
      }
    }
  } else {
    amp_msg( MSG_SAVE, NULL, 0 );
  }

  return FALSE;
}

/* Starts playing a next file or stops the player if all files
   already played. */
static void
amp_playstop( HWND hwnd )
{
  amp_stop();

  if( amp_playmode == AMP_SINGLE && cfg.rpt ) {
    amp_play();
  }
  if( amp_playmode == AMP_PLAYLIST )
  {
    PLRECORD* rec = pl_query_next_record();
    BOOL      eol = FALSE;

    if( !rec ) {
      pl_clean_shuffle();
      rec = pl_query_first_record();
      eol = TRUE;
    }

    if( rec && amp_pl_load_record( rec ) && ( cfg.rpt || !eol )) {
      amp_play();
    }
  }
}

static MRESULT EXPENTRY
amp_eq_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
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
          if( amp_load_eq( hwnd, gains, mutes, &preamp )) {
            WinSendMsg( hwnd, AMP_REFRESH_CONTROLS, 0, 0 );
          }
          if( WinQueryButtonCheckstate( hwnd, 121 )) {
            equalize_sound( gains, mutes, preamp, 1 );
          }
          break;

        case 124:
          amp_save_eq( hwnd, gains, mutes, preamp );
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
            equalize_sound( gains, mutes, preamp, 1 );
          } else {
            equalize_sound( gains, mutes, preamp, 0 );
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
            equalize_sound( gains, mutes, preamp, 1 );
            break;
          }
        }

        if( id == 121 ) {
          cfg.eq_enabled = WinQueryButtonCheckstate( hwnd, 121 );
          equalize_sound( gains, mutes, preamp, cfg.eq_enabled );
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
              equalize_sound( gains, mutes, preamp, 1 );
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
amp_eq_show( void )
{
  if( heq == NULLHANDLE )
  {
    heq = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                      amp_eq_dlg_proc, NULLHANDLE, DLG_EQUALIZER, NULL );

    do_warpsans( heq );
    rest_window_pos( heq, WIN_MAP_POINTS );
  }

  WinSetWindowPos( heq, HWND_TOP, 0, 0, 0, 0,
                        SWP_ZORDER | SWP_SHOW | SWP_ACTIVATE );
}

/* Processes messages of the player client window. */
static MRESULT EXPENTRY
amp_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  static BOOL decoder_finished = FALSE;

  switch( msg )
  {
    case WM_FOCUSCHANGE:
    {
      HPS hps = WinGetPS( hwnd );
      is_have_focus = SHORT1FROMMP( mp2 );
      bmp_draw_led( hps, is_have_focus  );
      WinReleasePS( hps );
      break;;
    }

    case 0x041e:
      if( !is_have_focus ) {
        HPS hps = WinGetPS( hwnd );
        bmp_draw_led( hps, TRUE);
        WinReleasePS( hps );
      }
      return 0;

    case 0x041f:
      if( !is_have_focus ) {
        HPS hps = WinGetPS( hwnd );
        bmp_draw_led( hps, FALSE );
        WinReleasePS( hps );
      }
      return 0;

    case WM_PLAYERROR:
      if( dec_status() == DECODER_STOPPED || !out_playing_data())
      {
        amp_stop();
        if( *last_error ) {
          amp_error( hwnd, "%s", last_error );
          *last_error = 0;
        }
      }
      return 0;

    case AMP_STOP:
      // The stop of the player always should be initiated from
      // the main thread. Otherwise we can receive the unnecessary
      // WM_PLAYSTOP message.
      amp_stop_playing();
      return 0;

    case AMP_DISPLAYMSG:
      amp_info( hwnd, "%s", mp1 );
      free( mp1 );
      return 0;

    case WM_CONTEXTMENU:
      amp_show_context_menu( hwnd );
      return 0;

    case WM_SEEKSTOP:
      is_seeking = FALSE;
      return 0;

    case WM_CHANGEBR:
    {
      HPS hps = WinGetPS( hwnd );
      current_bitrate = LONGFROMMP( mp1 );
      bmp_draw_rate( hps, current_bitrate );
      WinReleasePS( hps );
      return 0;
    }

    // Posted by decoder
    case WM_PLAYSTOP:
      DEBUGLOG(("WM_PLAYSTOP\n"));
      // The decoder has finished the work, but we should wait until output
      // buffers will become empty.
      decoder_finished = TRUE;
      // Work-around for old decoders
      out_flush();
      return 0;

    // Posted by output
    case WM_OUTPUT_OUTOFDATA:
      if( decoder_finished ) {
        amp_playstop( hwnd );
      }
      decoder_finished = FALSE;
      return 0;

    case DM_DRAGOVER:
      return amp_drag_over( hwnd, (PDRAGINFO)mp1 );
    case DM_DROP:
      return amp_drag_drop( hwnd, (PDRAGINFO)mp1 );

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
          amp_paint_timers( hps );
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

      if( options & UPD_TIMERS   ) {
        amp_paint_timers( hps );
      }
      if( options & UPD_FILEINFO ) {
        amp_paint_fileinfo( hps );
      }

      WinReleasePS( hps );
      return 0;
    }

    case WM_PAINT:
    {
      HPS hps = WinBeginPaint( hwnd, NULLHANDLE, NULL );

      bmp_draw_background( hps, hwnd );
      amp_paint_fileinfo ( hps );
      bmp_draw_led       ( hps, is_have_focus  );
      bmp_draw_volume    ( hps, cfg.defaultvol );

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
          for( i = 0; i < sizeof( current_tune.title ) - 1 && *titlepos
                      && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
          {
            current_tune.title[i] = *titlepos++;
          }

          current_tune.title[i] = 0;
          amp_display_filename();
          amp_invalidate( UPD_FILEINFO );

          if( current_record != NULL ) {
            free( current_record->songname );
            current_record->songname = strdup( current_tune.title );
            pl_refresh_record( current_record, CMA_TEXTCHANGED );
          }
        }
      }
      return 0;
    }

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->source == CMDSRC_MENU )
      {
        if( COMMANDMSG(&msg)->cmd > IDM_M_PLUG ) {
          process_possible_plugin( hwnd, COMMANDMSG(&msg)->cmd );
          return 0;
        }
        if( COMMANDMSG(&msg)->cmd > IDM_M_BOOKMARKS ) {
          process_possible_bookmark( COMMANDMSG(&msg)->cmd );
          return 0;
        }
        if( COMMANDMSG(&msg)->cmd > IDM_M_LAST )
        {
          char filename[_MAX_FNAME];
          int  i = COMMANDMSG(&msg)->cmd - IDM_M_LAST - 1;

          strcpy( filename, cfg.last[i] );
          amp_load_singlefile( filename, 0 );
          return 0;
        }
      }

      switch( COMMANDMSG(&msg)->cmd )
      {
        case IDM_M_MANAGER:
          pm_show( TRUE );
          return 0;

        case IDM_M_ADDBOOK:
          bm_add_bookmark( hwnd );
          return 0;

        case IDM_M_EDITBOOK:
          bm_show( TRUE );
          return 0;

        case IDM_M_TAG:
          amp_id3_edit( hwnd, current_filename );
          return 0;

        case IDM_M_SAVE:
          is_stream_saved = amp_save_stream( hwnd, !is_stream_saved );
          return 0;

        case IDM_M_EQUALIZE:
          amp_eq_show();
          return 0;

        case IDM_M_FLOAT:
          cfg.floatontop = !cfg.floatontop;

          if( !cfg.floatontop ) {
            WinStopTimer ( hab, hwnd, TID_ONTOP );
          } else {
            WinStartTimer( hab, hwnd, TID_ONTOP, 100 );
          }
          return 0;

        case IDM_M_URL:
          amp_add_url( hwnd, URL_ADD_TO_PLAYER );
          return 0;

        case IDM_M_TRACK:
          amp_add_tracks( hwnd, TRK_ADD_TO_PLAYER );
          return 0;

        case IDM_M_FONT1:
          cfg.font = 0;
          amp_display_filename();
          amp_invalidate( UPD_ALL );
          return 0;

        case IDM_M_FONT2:
          cfg.font = 1;
          amp_display_filename();
          amp_invalidate( UPD_ALL );
          return 0;

        case IDM_M_TINY:
          cfg.mode = CFG_MODE_TINY;
          bmp_reflow_and_resize( hframe );
          return 0;

        case IDM_M_NORMAL:
          cfg.mode = CFG_MODE_REGULAR;
          bmp_reflow_and_resize( hframe );
          return 0;

        case IDM_M_SMALL:
          cfg.mode = CFG_MODE_SMALL;
          bmp_reflow_and_resize( hframe );
          return 0;

        case IDM_M_MINIMIZE:
          WinSetWindowPos( hframe, HWND_DESKTOP, 0, 0, 0, 0, SWP_HIDE );
          WinSetActiveWindow( HWND_DESKTOP, WinQueryWindow( hwnd, QW_NEXTTOP ));
          return 0;

        case IDM_M_SKINLOAD:
        {
          HPS hps = WinGetPS( hwnd );
          amp_loadskin( hab, hwnd, hps );
          WinReleasePS( hps );
          amp_invalidate( UPD_ALL );
          return 0;
        }

        case IDM_M_LOADFILE:
          amp_load_file( hwnd );
          return 0;

        case IDM_M_CFG:
          cfg_properties( hwnd );
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
        {
          HPS hps = WinGetPS( hwnd );

          cfg.defaultvol += 5;
          if( cfg.defaultvol > 100 ) {
            cfg.defaultvol = 100;
          }

          bmp_draw_volume( hps, cfg.defaultvol );
          WinReleasePS( hps );
          amp_volume_adjust();
          return 0;
        }

        case IDM_M_VOL_LOWER:
        {
          HPS hps = WinGetPS( hwnd );

          cfg.defaultvol -= 5;
          if( cfg.defaultvol < 0 ) {
            cfg.defaultvol = 0;
          }

          bmp_draw_volume( hps, cfg.defaultvol );
          WinReleasePS( hps );
          amp_volume_adjust();
          return 0;
        }

        case IDM_M_MENU:
          amp_show_context_menu( hwnd );
          return 0;

        case BMP_PL:
          pl_show( !WinIsWindowVisible( hplaylist ));
          return 0;

        case BMP_REPEAT:
          cfg.rpt = !cfg.rpt;

          if( cfg.rpt ) {
            WinSendDlgItemMsg( hwnd, BMP_REPEAT, WM_PRESS,   0, 0 );
          } else {
            WinSendDlgItemMsg( hwnd, BMP_REPEAT, WM_DEPRESS, 0, 0 );
          }

          amp_invalidate( UPD_FILEINFO );
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

        case BMP_POWER:
          WinPostMsg( hwnd, WM_QUIT, 0, 0 );
          return 0;

        case BMP_PLAY:
          if( !decoder_playing()) {
            amp_play();
          } else {
            WinSendMsg( hwnd, WM_COMMAND, MPFROMSHORT( BMP_STOP ), mp2 );
          }
          return 0;

        case BMP_PAUSE:
          amp_pause();
          return 0;

        case BMP_FLOAD:
          amp_load_file(hwnd);
          return 0;

        case BMP_STOP:
          amp_stop();
          pl_clean_shuffle();
          return 0;

        case BMP_NEXT:
          if( amp_playmode == AMP_PLAYLIST )
          {
            BOOL decoder_was_playing = decoder_playing();
            PLRECORD* rec = pl_query_next_record();

            if( rec ) {
              if( decoder_was_playing ) {
                amp_stop();
              }
              if( amp_pl_load_record( rec )) {
                if( decoder_was_playing ) {
                  amp_play();
                }
              }
            }
          }
          return 0;

        case BMP_PREV:
          if( amp_playmode == AMP_PLAYLIST )
          {
            BOOL decoder_was_playing = decoder_playing();
            PLRECORD* rec = pl_query_prev_record();

            if( rec ) {
              if( decoder_was_playing ) {
                amp_stop();
              }
              if( amp_pl_load_record( rec )) {
                if( decoder_was_playing ) {
                  amp_play();
                }
              }
            }
          }
          return 0;

        case BMP_FWD:
          if( decoder_playing() && !is_paused() )
          { 
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
            amp_msg( MSG_FWD, 0, 0 );
            WinSendDlgItemMsg( hwnd, BMP_FWD, is_fast_forward() ? WM_PRESS : WM_DEPRESS, 0, 0 );
            amp_volume_adjust();
          } else {
            WinSendDlgItemMsg( hwnd, BMP_FWD, WM_DEPRESS, 0, 0 );
          }
          return 0;

        case BMP_REW:
          if( decoder_playing() && !is_paused() )
          {
            WinSendDlgItemMsg( hwnd, BMP_FWD, WM_DEPRESS, 0, 0 );
            amp_msg( MSG_REW, 0, 0 );
            WinSendDlgItemMsg( hwnd, BMP_REW, is_fast_backward() ? WM_PRESS : WM_DEPRESS, 0, 0 );
            amp_volume_adjust();
          } else {
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
          }
          return 0;
      }
      return 0;

    case WM_CREATE:
    {
      HPS hps = WinGetPS( hwnd );
      hplayer = hwnd; /* we have to assign the window handle early, because WinCreateStdWindow does not have returned now. */
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
      HPS    hps = WinGetPS( hwnd );
      POINTL pos;

      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      if( bmp_pt_in_volume( pos ))
      {
        bmp_draw_volume( hps, cfg.defaultvol = bmp_calc_volume( pos ));
        amp_volume_adjust();
      }
      else
      {
        if( amp_playmode != AMP_NOFILE && bmp_pt_in_text( pos )) {
          amp_display_next_mode();
          amp_invalidate( UPD_FILEINFO );
        }
      }

      WinReleasePS( hps );
      return 0;
    }

    case WM_MOUSEMOVE:
      if( is_volume_drag )
      {
        HPS    hps = WinGetPS( hwnd );
        POINTL pos;

        pos.x = SHORT1FROMMP(mp1);
        pos.y = SHORT2FROMMP(mp1);

        bmp_draw_volume( hps, cfg.defaultvol = bmp_calc_volume( pos ));
        WinReleasePS( hps );
        amp_volume_adjust();
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
      }
      if( is_slider_drag )
      {
        ULONG  ms;
        POINTL pos;

        pos.x = SHORT1FROMMP(mp1);
        pos.y = SHORT2FROMMP(mp1);
        seeking_pos = bmp_calc_time( pos, time_total());
        ms = seeking_pos * 1000;

        amp_msg( MSG_JUMP, &ms, 0 );
        is_slider_drag = FALSE;
      }
      return 0;

    case WM_BUTTON1MOTIONSTART:
    {
      POINTL pos;

      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      if( bmp_pt_in_volume( pos )) {
        is_volume_drag = TRUE;
      } else if( bmp_pt_in_slider( pos ) && decoder_playing()) {
        is_slider_drag = TRUE;
        is_seeking     = TRUE;
      }
      /* disabled moving with the left button when missing the above controls (MM)
       else {
        WinSendMsg( hframe, WM_TRACKFRAME, MPFROMSHORT( TF_MOVE | TF_STANDARD ), 0 );
        WinQueryWindowPos( hframe, &cfg.main );
      }*/
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

/* Stops playing and resets the player to its default state. */
void
amp_reset( void )
{
  if( decoder_playing()) {
    amp_stop();
  }

  amp_playmode     = AMP_NOFILE;
  current_record   = NULL;
  current_bitrate  = 0;
  current_channels = -1;
  current_length   = 0;
  current_freq     = 0;
  current_track    = 0;
 
  current_cd_drive[0] = 0;
  current_decoder_info_string[0] = 0;
  current_filename[0] = 0;

  amp_display_filename();
  amp_invalidate( UPD_ALL );
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
  
  DEBUGLOG(("ERROR: %s\n", message));

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

  DEBUGLOG(("ERROR: %x, %s\n", owner, message));

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

  DEBUGLOG(("INFO: %s\n", message));

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

int
main( int argc, char *argv[] )
{
  HMQ    hmq;
  int    i, o, files = 0;
  char   exename[_MAX_PATH];
  char   bundle [_MAX_PATH];
  char   infname[_MAX_PATH];
  char   command[1024];
  QMSG   qmsg;
  struct stat fi;

  HELPINIT hinit;
  ULONG    flCtlData = FCF_TASKLIST | FCF_NOBYTEALIGN | FCF_ACCELTABLE | FCF_ICON;

  // used for debug printf()s
  setvbuf( stdout, NULL, _IONBF, 0 );
  setvbuf( stderr, NULL, _IONBF, 0 );

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  for( i = 1; i < argc; i++ ) {
    if( argv[i][0] != '/' && argv[i][0] != '-' ) {
      files++;
    }
  }

  set_proxyurl( cfg.proxy );
  set_httpauth( cfg.auth  );

  getExeName( exename, sizeof( exename ));
  sdrivedir ( startpath, exename, sizeof( startpath ));

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
        strcpy( pipename, argv[o] );  // machine name
        strcat( pipename, "\\PIPE\\PM123" );
        o++;
      }
      strcpy( command, "*" );
      for( i = o; i < argc; i++ )
      {
        strcat( command, argv[i] );
        strcat( command, " " );
      }

      amp_pipe_open_and_write( pipename, command, strlen( command ) + 1 );
      exit(0);
    }
  }

  // If we have files in argument, try to open \\pipe\pm123 and write to it.
  if( files > 0 ) {
     // this only takes the last argument we hope is the filename
     // this should be changed.
    if( amp_pipe_open_and_write( pipename, argv[argc-1], strlen( argv[argc-1]) + 1 )) {
      exit(0);
    }
  }

  if( !amp_pipe_create()) {
    exit(1);
  }

  _beginthread( amp_pipe_thread, NULL, 1024*1024, NULL );

  srand((unsigned long)time( NULL ));
  load_ini();
  amp_volume_to_normal();
  InitButton( hab );

  WinRegisterClass( hab, "PM123", amp_dlg_proc, CS_SIZEREDRAW | CS_SYNCPAINT, 0 );

  hframe = WinCreateStdWindow( HWND_DESKTOP, 0, &flCtlData, "PM123",
                               AMP_FULLNAME, 0, NULLHANDLE, WIN_MAIN, &hplayer );
  do_warpsans( hframe  );
  do_warpsans( hplayer );

  dk_init();
  dk_add_window( hframe, DK_IS_MASTER );

  mp3        = WinLoadPointer( HWND_DESKTOP, 0, ICO_MP3     );
  mp3play    = WinLoadPointer( HWND_DESKTOP, 0, ICO_MP3PLAY );
  mp3gray    = WinLoadPointer( HWND_DESKTOP, 0, ICO_MP3GRAY );
  hplaylist  = pl_create();

  pm_create();
  bm_create();

  memset( &hinit, 0, sizeof( hinit ));
  strcpy( infname, startpath   );
  strcat( infname, "pm123.inf" );

  if( stat( infname, &fi ) != 0  ) {
    // If the file of the help does not placed together with the program,
    // we shall give to the help manager to find it.
    strcpy( infname, "pm123.inf" );
  }

  hinit.cb = sizeof( hinit );
  hinit.phtHelpTable = (PHELPTABLE)MAKELONG( HLP_MAIN, 0xFFFF );
  hinit.pszHelpWindowTitle = "PM123 Help";
  hinit.fShowPanelId = CMIC_SHOW_PANEL_ID;
  hinit.pszHelpLibraryName = infname;

  hhelp = WinCreateHelpInstance( hab, &hinit );
  if( !hhelp ) {
    amp_error( hplayer, "Error create help instance: %s", infname );
  }

  WinAssociateHelpInstance( hhelp, hframe );

  strcpy( bundle, startpath   );
  strcat( bundle, "pm123.lst" );

  if( files == 1 && !is_dir( argv[argc - 1] )) {
    amp_load_singlefile( argv[argc - 1], 0 );
  } else if( files > 0 ) {
    for( i = 1; i < argc; i++ ) {
      if( argv[i][0] != '/' && argv[i][0] != '-' ) {
        if( is_dir( argv[i] )) {
          pl_add_directory( argv[i], PL_DIR_RECURSIVE );
        } else {
          pl_add_file( argv[i], NULL, 0 );
        }
      }
    }
    pl_completed();
  } else {
    struct stat fi;
    if( stat( bundle, &fi ) == 0 ) {
      pl_load_bundle( bundle, 0 );
    }
  }

  WinSetWindowPos( hframe, HWND_TOP,
                   cfg.main.x, cfg.main.y, 0, 0, SWP_ACTIVATE | SWP_MOVE | SWP_SHOW );

  // initialize non-skin related visual plug-ins
  vis_init_all( FALSE );

  bm_load( hplayer );

  if( cfg.dock_windows ) {
    dk_arrange( hframe );
  }
  
  #ifdef DEBUG
  fprintf(stderr, "main: init complete\n");
  #endif

  while( WinGetMsg( hab, &qmsg, (HWND)0, 0, 0 ))
    WinDispatchMsg( hab, &qmsg );

  amp_stop();

  // deinitialize all visual plug-ins
  vis_deinit_all( TRUE );
  vis_deinit_all( FALSE );

  if( heq != NULLHANDLE ) {
    WinDestroyWindow( heq );
  }

  save_ini();
  bm_save( hplayer );
  pl_save_bundle( bundle, 0 );

  pm_destroy();
  bm_destroy();
  pl_destroy();

  bmp_clean_skin();
  remove_all_plugins();
  dk_term();

  WinDestroyPointer ( mp3     );
  WinDestroyPointer ( mp3play );
  WinDestroyPointer ( mp3gray );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );

  #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
  #endif
  return 0;
}

