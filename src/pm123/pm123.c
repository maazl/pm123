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
#include "pm123.h"
#include "plugman.h"
#include "bookmark.h"
#include "button95.h"
#include "pfreq.h"
#include "httpget.h"
#include "copyright.h"
#include "docking.h"
#include "iniman.h"

#include <debuglog.h>

#define  AMP_REFRESH_CONTROLS ( WM_USER + 121   )
#define  AMP_DISPLAYMSG       ( WM_USER + 76    )
#define  AMP_PAINT            ( WM_USER + 77    )
/* Stop playback from main thread */
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

/* currently loaded file */
static MSG_PLAY_STRUCT current_file = { "" }; // initialize by default

static HAB   hab        = NULLHANDLE;
static HWND  hplaylist  = NULLHANDLE;
static HWND  heq        = NULLHANDLE;
static HWND  hframe     = NULLHANDLE;
static HWND  hplayer    = NULLHANDLE;
static HWND  hhelp      = NULLHANDLE;

static BOOL  is_have_focus    = FALSE;
static BOOL  is_volume_drag   = FALSE;
static BOOL  is_seeking       = FALSE;
static BOOL  is_slider_drag   = FALSE;
static BOOL  is_stream_saved  = FALSE;
static BOOL  is_arg_shuffle   = FALSE;
static BOOL  is_decoder_done  = FALSE;

/* Current load wizzards */
static DECODER_WIZZARD_FUNC load_wizzards[16];

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

/* Adjusts audio volume to level accordingly current playing mode. */
void
amp_volume_adjust( void )
{
  double i = cfg.defaultvol;
  if( i > 100 ) {
      i = 100;
  }
  if( is_forward() || is_rewind()) {
    i *= 3./5.;
  }
  out_set_volume(i/100.);
}

/* Draws all player timers and the position slider. */
static void
amp_paint_timers( HPS hps )
{
  int play_time = 0;
  int play_left = current_file.info.tech.songlength/1000;
  int list_left = 0;

  DEBUGLOG(("amp_paint_timers(%p) - %i %i %i %i %i\n", hps, cfg.mode, decoder_playing(), is_seeking, time_played(), play_left));

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
  DEBUGLOG(("amp_paint_fileinfo(%p)\n", hps));

  if( amp_playmode == AMP_PLAYLIST ) {
    bmp_draw_plind( hps, pl_current_index(), pl_size());
  } else {
    bmp_draw_plind( hps, 0, 0 );
  }

  bmp_draw_plmode  ( hps );
  bmp_draw_timeleft( hps );
  bmp_draw_rate    ( hps, current_file.info.tech.bitrate );
  bmp_draw_channels( hps, current_file.info.format.channels );
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

/* Returns a information block of the currently loaded file or NULL if none. */
const MSG_PLAY_STRUCT*
amp_get_current_file( void )
{ return *current_file.url ? &current_file : NULL;
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

/* Constructs a information text for currently loaded file
   and selects it for displaying. */
void
amp_display_filename( void )
{
  char display[512];

  if( amp_playmode == AMP_NOFILE ) {
    bmp_set_text( "No file loaded" );
    return;
  }

  switch( cfg.viewmode )
  {
    case CFG_DISP_ID3TAG:
      amp_construct_tag_string( display, &current_file.info.meta );

      if( *display ) {
        bmp_set_text( display );
        break;
      }

      // if ID3 tag is empty - use filename instead of it.

    case CFG_DISP_FILENAME:
    {
      const char* current_filename = current_file.url;
      if( *current_filename )
      {
        if( is_file( current_filename )) {
          bmp_set_text( sfname( display, current_filename, sizeof( display )));
        } else if ( is_cdda( current_filename )) {
          char* cp = strchr( current_filename, ':' );
          bmp_set_text( cp +1 );
        } else {
          bmp_set_text( current_filename );
        }
      } else {
         bmp_set_text( "This is a bug!" );
      }
      break;
    }
    case CFG_DISP_FILEINFO:
      bmp_set_text( current_file.info.tech.info );
      break;
  }
}

/* Switches to the next text displaying mode. */
void
amp_display_next_mode( void )
{
  if( cfg.viewmode == CFG_DISP_FILEINFO ) {
    cfg.viewmode = CFG_DISP_FILENAME;
  } else {
    cfg.viewmode++;
  }

  amp_display_filename();
}

/* Loads the specified playlist record into the player. */
BOOL
amp_pl_load_record( PLRECORD* rec )
{
  struct stat  fi;

  if( !rec ) {
    return FALSE;
  }

  if( is_file( rec->full ) && stat( rec->full, &fi ) != 0 ) {
    amp_error( hplayer, "Unable load file:\n%s\n%s", rec->full, clib_strerror(errno));
    return FALSE;
  }

  if ( dec_fileinfo ( rec->full, &current_file.info, current_file.decoder ) != 0 ) {
    amp_error( hplayer, "Unable load:\n%s", rec->full);
    return FALSE;
  }
  
  strlcpy( current_file.url, rec->full, sizeof current_file.url );

  // merge meta information from playlist record - hmm?
  if ( !*current_file.info.meta.title ) {
    strlcpy( current_file.info.meta.title, rec->songname, sizeof( current_file.info.meta.title ));
  }

  if( amp_playmode == AMP_PLAYLIST ) {
    // TODO: Uuh, it should be up to playlist.c to modify this item
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
      current_record = pl_query_file_record( current_file.url );
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
  int    i;
  ULONG  rc;
  CDDA_REGION_INFO cd_info = {"", 0};
  struct stat fi;

  if( is_playlist( filename )) {
    return pl_load( filename, PL_LOAD_CLEAR );
  }

  if( is_file( filename ) && stat( filename, &fi ) != 0 ) {
    amp_error( hplayer, "Unable load file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  if ( is_cdda( filename ) && !scdparams( &cd_info, filename ) ) {
    amp_error( hplayer, "Invalid CD URL:\n%s", filename );
    return FALSE;
  }

  rc = dec_fileinfo((char*)filename, &current_file.info, current_file.decoder );

  if( rc != 0 ) { /* error, invalid file */
    amp_reset();
    handle_dfi_error( rc, filename );
    return FALSE;
  }

  amp_stop();
  amp_pl_release();

  strlcpy( current_file.url, filename, sizeof current_file.url );

  // merge some information
  if ( current_file.info.meta.track <= 0 && cd_info.track > 0 ) { 
    current_file.info.meta.track = cd_info.track;
  }

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
  // Resets a waiting of an emptiness of the buffers for a case if the WM_PLAYSTOP
  // message it has already been received.
  is_decoder_done = FALSE;
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
  char caption [_MAX_PATH];
  char filename[_MAX_PATH];

  if( amp_playmode == AMP_NOFILE ) {
    WinSendDlgItemMsg( hplayer, BMP_PLAY, WM_DEPRESS, 0, 0 );
    return;
  }

  amp_msg( MSG_PLAY, &current_file, NULL );
  amp_set_bubbletext( BMP_PLAY, "Stops playback" );

  WinSendDlgItemMsg( hplayer, BMP_FWD,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_REW,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PLAY,  WM_PRESS  , 0, 0 );

  if( amp_playmode == AMP_PLAYLIST ) {
    pl_mark_as_play();
  }

  sfnameext( filename, current_file.url, sizeof( filename ));
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
    if( is_paused()) {
      WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_PRESS, 0, 0 );
      return;
    }
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
  const MSG_PLAY_STRUCT* current;

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

  // Remove all items from the load menu except of two first
  // intended for a choice of object of loading.
  for( i = 2; i < count; i++ )
  {
    id = LONGFROMMR( WinSendMsg( mh, MM_ITEMIDFROMPOSITION, MPFROMSHORT(2), 0 ));
    WinSendMsg( mh, MM_DELETEITEM, MPFROM2SHORT( id, FALSE ), 0 );
  }

  // Append asisstents from decoder plug-ins
  memset( load_wizzards, 0, sizeof load_wizzards / sizeof *load_wizzards ); // You never know...
  append_load_menu( mi.hwndSubMenu, IDM_M_ADDOTHER, FALSE, load_wizzards, sizeof load_wizzards / sizeof *load_wizzards );

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
  current = amp_get_current_file();
  // TODO: edit more than ID3 tags
  mn_enable_item( menu, IDM_M_TAG,     current != NULL && current->info.meta_write );
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

/* Reads url from specified file. */
char*
amp_url_from_file( char* result, const char* filename, size_t size )
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

    /* debug
    {
      char info[2048];

      printf( "hwndItem: %08X\n", pditem->hwndItem );
      printf( "ulItemID: %lu\n", pditem->ulItemID );
      DrgQueryStrName( pditem->hstrType, sizeof( info ), info );
      printf( "hstrType: %s\n", info );
      DrgQueryStrName( pditem->hstrRMF, sizeof( info ), info );
      printf( "hstrRMF: %s\n", info );
      DrgQueryStrName( pditem->hstrContainerName, sizeof( info ), info );
      printf( "hstrContainerName: %s\n", info );
      DrgQueryStrName( pditem->hstrSourceName, sizeof( info ), info );
      printf( "hstrSourceName: %s\n", info );
      DrgQueryStrName( pditem->hstrTargetName, sizeof( info ), info );
      printf( "hstrTargetName: %s\n", info );
      printf( "fsControl: %08X\n", pditem->fsControl );
    } */

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
          amp_load_singlefile( fullname, 0 );
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

/* Receives dropped and rendered files and urls. */
static MRESULT
amp_drag_render_done( HWND hwnd, PDRAGTRANSFER pdtrans, USHORT rc )
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

    if( is_dir( fullname )) {
      pl_add_directory( fullname, PL_DIR_RECURSIVE );
    } else if( is_playlist( fullname )) {
      pl_load( fullname, 0 );
    } else if( pdsource->cditem == 1 ) {
      amp_load_singlefile( fullname, 0 );
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

/* Edits a ID3 tag for the specified file. */
void
amp_id3_edit( HWND owner, const char* filename, const char* decoder )
{
  ULONG rc = dec_editmeta( owner, filename, decoder );
  DECODER_INFO2 info;
  const MSG_PLAY_STRUCT* current;

  switch (rc)
  { default:
      amp_error( owner, "Cannot edit tag of file:\n%s", filename);
      return;

    case 0:   // tag changed
      dec_fileinfo( filename, &info, NULL );
      pl_refresh_file( filename );

      current = amp_get_current_file();
      if( current != NULL && stricmp( current->url, filename ) == 0 ) {
        // TODO: post WM_METADATA
        // the structures are binary compatible so far
        current_file.info.meta = info.meta;
        amp_display_filename();
        amp_invalidate( UPD_ALL );
      }
    case 300: // tag unchanged
      return;
      
    case 500:
      amp_error( owner, "Unable write tag to file:\n%s\n%s", filename, clib_strerror(errno));
      return;
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
      // TODO: turn into change of metadata in general
      HPS hps = WinGetPS( hwnd );
      current_file.info.tech.bitrate = LONGFROMMP( mp1 );
      bmp_draw_rate( hps, current_file.info.tech.bitrate );
      WinReleasePS( hps );
      return 0;
    }

    // Posted by decoder
    case WM_PLAYSTOP:
      DEBUGLOG(("WM_PLAYSTOP\n"));
      // The decoder has finished the work, but we should wait until output
      // buffers will become empty.
      is_decoder_done = TRUE;
      // Work-around for old decoders
      out_flush();
      return 0;

    // Posted by output
    case WM_OUTPUT_OUTOFDATA:
      if( is_decoder_done ) {
        amp_playstop( hwnd );
      }
      is_decoder_done = FALSE;
      return 0;

    case DM_DRAGOVER:
      return amp_drag_over( hwnd, (PDRAGINFO)mp1 );
    case DM_DROP:
      return amp_drag_drop( hwnd, (PDRAGINFO)mp1 );
    case DM_RENDERCOMPLETE:
      return amp_drag_render_done( hwnd, (PDRAGTRANSFER)mp1, SHORT1FROMMP( mp2 ));

    case WM_TIMER:
      DEBUGLOG2(("amp_dlg_proc: WM_TIMER - %x\n", LONGFROMMP(mp1)));
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
      // TODO: turn into general metadata replacement
      char* metadata = PVOIDFROMMP(mp1);
      char* titlepos;
      int   i;

      if( metadata ) {
        titlepos = strstr( metadata, "StreamTitle='" );
        if( titlepos )
        {
          titlepos += 13;
          for( i = 0; i < sizeof( current_file.info.meta.title ) - 1 && *titlepos
                      && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
          {
            current_file.info.meta.title[i] = *titlepos++;
          }

          current_file.info.meta.title[i] = 0;
          amp_display_filename();
          amp_invalidate( UPD_FILEINFO );

          if( current_record != NULL ) {
            free( current_record->songname );
            current_record->songname = strdup( current_file.info.meta.title );
            pl_refresh_record( current_record, CMA_TEXTCHANGED );
          }
        }
      }
      return 0;
    }

    case WM_COMMAND:
    { const CMDMSG* cm = COMMANDMSG(&msg);
      if( cm->source == CMDSRC_MENU )
      {
        if( cm->cmd > IDM_M_PLUG ) {
          configure_plugin( PLUGIN_NULL, cm->cmd - IDM_M_PLUG - 1, hplayer );
          return 0;
        }
        if( cm->cmd > IDM_M_BOOKMARKS ) {
          process_possible_bookmark( cm->cmd );
          return 0;
        }
        if( cm->cmd > IDM_M_LAST )
        {
          char filename[_MAX_FNAME];
          int  i = cm->cmd - IDM_M_LAST - 1;

          strcpy( filename, cfg.last[i] );
          amp_load_singlefile( filename, 0 );
          return 0;
        }
        if( cm->cmd >= IDM_M_ADDOTHER &&
            cm->cmd < IDM_M_ADDOTHER + sizeof load_wizzards / sizeof *load_wizzards &&
            load_wizzards[cm->cmd-IDM_M_ADDOTHER] )
        {
          char url[1024];
          ULONG rc = (*load_wizzards[cm->cmd-IDM_M_ADDOTHER])( hwnd, url, sizeof url );
          if ( rc == 0 )
            amp_load_singlefile( url, 0 );
          return 0;
        }
      }

      switch( cm->cmd )
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
          amp_id3_edit( hplayer, current_file.url, current_file.decoder );
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
          if( decoder_playing() && !is_paused())
          {
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
            amp_msg( MSG_FWD, 0, 0 );
            WinSendDlgItemMsg( hwnd, BMP_FWD, is_forward() ? WM_PRESS : WM_DEPRESS, 0, 0 );
            amp_volume_adjust();
          } else {
            WinSendDlgItemMsg( hwnd, BMP_FWD, WM_DEPRESS, 0, 0 );
          }
          return 0;

        case BMP_REW:
          if( decoder_playing() && !is_paused())
          {
            WinSendDlgItemMsg( hwnd, BMP_FWD, WM_DEPRESS, 0, 0 );
            amp_msg( MSG_REW, 0, 0 );
            WinSendDlgItemMsg( hwnd, BMP_REW, is_rewind() ? WM_PRESS : WM_DEPRESS, 0, 0 );
            amp_volume_adjust();
          } else {
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
          }
          return 0;
      }
      return 0;
    }
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
  memset( &current_file, 0, sizeof current_file );

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

      pipe_open_and_write( pipename, command, strlen( command ) + 1 );
      exit(0);
    }
  }

  // If we have files in argument, try to open \\pipe\pm123 and write to it.
  if( files > 0 ) {
     // this only takes the last argument we hope is the filename
     // this should be changed.
    if( pipe_open_and_write( pipename, argv[argc-1], strlen( argv[argc-1]) + 1 )) {
      exit(0);
    }
  }

  if( !pipe_create()) {
    exit(1);
  }

  srand((unsigned long)time( NULL ));
  load_ini();
  //amp_volume_to_normal(); // Superfluous!
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

