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

//#undef DEBUG
//#define DEBUG 2

#include <utilfct.h>
#include <xio.h>
#include "pm123.h"
#include "plugman.h"
#include "button95.h"
#include "playlistmanager.h"
#include "playlistview.h"
#include "copyright.h"
#include "docking.h"
#include "iniman.h"
#include "messages.h"
#include "playenumerator.h"
#include "playlistmenu.h"

#include <cpp/xstring.h>
#include "url.h"
#include "pm123.rc.h"

#include <debuglog.h>

#define  AMP_REFRESH_CONTROLS ( WM_USER + 75 )
#define  AMP_DISPLAY_MSG      ( WM_USER + 76 )
#define  AMP_PAINT            ( WM_USER + 77 )
#define  AMP_STOP             ( WM_USER + 78 )
#define  AMP_PLAY             ( WM_USER + 79 )
#define  AMP_PAUSE            ( WM_USER + 80 )

#define  TID_UPDATE_TIMERS    ( TID_USERMAX - 1 )
#define  TID_UPDATE_PLAYER    ( TID_USERMAX - 2 )
#define  TID_ONTOP            ( TID_USERMAX - 3 )

//int       amp_playmode = AMP_NOFILE;

/* Contains startup path of the program without its name.  */
char startpath[_MAX_PATH];

// Currently loaded object - that's what it's all about!
static StatusPlayEnumerator Current;

// Default playlist, representing PM123.LST in the program folder.
static PlaylistView*     DefaultPL = NULL;
// PlaylistManager window, representing PFREQ.LST in the program folder.
static PlaylistManager*  DefaultPM = NULL;
// Default instance of bookmark window, representing BOOKMARK.LST in the program folder.
static PlaylistBase*     DefaultBM = NULL;
// Most recent used entries in the load menu, representing LOADMRU.LST in the program folder.
static int_ptr<Playlist> LoadMRU;

// Default playlistmanager
//static sco_ptr<PlaylistManager> DefaultPM;


static HAB   hab        = NULLHANDLE;
//static HWND  hplaylist  = NULLHANDLE;
static HWND  heq        = NULLHANDLE;
static HWND  hframe     = NULLHANDLE;
static HWND  hplayer    = NULLHANDLE;
static HWND  hhelp      = NULLHANDLE;
static HPIPE hpipe      = NULLHANDLE;

/* Pipe name decided on startup. */
static char  pipename[_MAX_PATH] = "\\PIPE\\PM123";

static BOOL  is_have_focus   = FALSE;
static BOOL  is_volume_drag  = FALSE;
static BOOL  is_seeking      = FALSE;
static BOOL  is_slider_drag  = FALSE;
static BOOL  is_stream_saved = FALSE;
static BOOL  is_arg_shuffle  = FALSE;

/* Current load wizzards */
static DECODER_WIZZARD_FUNC load_wizzards[20];

/* Current seeking time. Valid if is_seeking == TRUE. */
static double seeking_pos = 0;
static int    upd_options = 0;

/* Equalizer stuff. */
float gains[20];
BOOL  mutes[20];
float preamp;

void DLLENTRY
amp_display_info( const char* info )
{
  char* message = strdup( info );
  if( message ) {
    amp_post_message( AMP_DISPLAY_MSG, MPFROMP( message ), MPFROMLONG( FALSE ));
  }
  amp_post_message( WM_PLAYERROR, 0, 0 );
}

void DLLENTRY
amp_display_error( const char *info )
{
  char* message = strdup( info );
  if( message ) {
    amp_post_message( AMP_DISPLAY_MSG, MPFROMP( message ), MPFROMLONG( TRUE  ));
  }
}

Song* amp_get_current_song()
{ return Current.GetCurrentSong();
}

Playable* amp_get_current_root()
{ return Current.GetRoot();
}

/* Sets audio volume to the current selected level.
static void
amp_volume_to_normal( void ) {
  out_set_volume( limit2( cfg.defaultvol, 0, 100 ));
}*/

/* Sets audio volume to below current selected level.
static void
amp_volume_to_lower( void ) {
  out_set_volume( 0.6 * limit2( cfg.defaultvol, 0, 100 ));
}*/

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
  DEBUGLOG(("amp_paint_timers(%p) - %i %i %i %f\n", hps, cfg.mode, decoder_playing(), is_seeking, out_playing_pos()));

  if( cfg.mode == CFG_MODE_REGULAR )
  {
    PlayEnumerator::Status s = Current.GetStatus();
    double play_time = 0;
    double play_left = 0;
    double list_left = 0;

    { int_ptr<Song> song = Current.GetCurrentSong();
      if (song != NULL)
        play_left = song->GetInfo().tech->songlength; // If the information is not yet available this returns 0
    }

    if (decoder_playing())
      play_time = !is_seeking ? out_playing_pos() : seeking_pos;

    if( play_left > 0 )
      play_left -= play_time;

    if( !cfg.rpt && s.TotalTime > 0 )
      list_left = s.TotalTime - s.CurrentTime - play_time;

    bmp_draw_slider( hps, play_time, dec_length());
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

  const Playable* pp = Current.GetRoot();
  if (pp != NULL && (pp->GetFlags() & Playable::Enumerable))
  { PlayEnumerator::Status s = Current.GetStatus();
    bmp_draw_plind( hps, s.CurrentItem, s.TotalItems);
  } else {
    bmp_draw_plind( hps, 0, 0 );
  }

  bmp_draw_plmode  ( hps );
  bmp_draw_timeleft( hps );
  if (pp != NULL)
  { const TECH_INFO& tech = *pp->GetInfo().tech;
    bmp_draw_rate    ( hps, tech.bitrate );
    const FORMAT_INFO2& format = *pp->GetInfo().format;
    bmp_draw_channels( hps, format.channels );
  } else
  { bmp_draw_rate    ( hps, -1 );
    bmp_draw_channels( hps, -1 );
  }
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
amp_player_window( void ) {
  return hplayer;
}

/* Returns the anchor-block handle. */
HAB
amp_player_hab( void ) {
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
  WinSendDlgItemMsg( hplayer, id, WM_SETTEXT, MPFROMP(text), 0 );
}

/* Loads the specified playlist record into the player. */
/*BOOL
amp_pl_load_record( PLRECORD* rec )
{
  struct stat fi;

  if( !rec ) {
    return FALSE;
  }

  if( is_file( rec->full ) && stat( rec->full, &fi ) != 0 ) {
    amp_error( hplayer, "Unable load file:\n%s\n%s", rec->full, strerror(errno));
    return FALSE;
  }

  strlcpy( current_file.url, rec->full, sizeof current_file.url );
  strlcpy( current_file.decoder, rec->decoder, sizeof( current_file.decoder ));
  current_file.info = rec->info;

  if( amp_playmode == AMP_PLAYLIST ) {
    // TODO: Uuh, it should be up to playlist.c to modify this item
    current_record = rec;
  } else {
    amp_playmode = AMP_SINGLE;
  }

  amp_display_filename();
  amp_invalidate( UPD_FILEINFO );
  return TRUE;
}*/

/* Loads the specified playlist record into the player and
   plays it if this is specified in the player properties or
   the player is already playing. */
/*void
amp_pl_play_record( PLRECORD* rec )
{
  BOOL decoder_was_playing = decoder_playing();

  if( decoder_was_playing ) {
    amp_stop();
  }

  if( rec  ) {
    if( amp_pl_load_record( rec )) {
      if( cfg.playonload == 1 || decoder_was_playing ) {
        amp_play( 0 );
      }
    }
  }
}*/

/* Activates the current playlist. */
/*void
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
      amp_play( 0 );
    }

    amp_invalidate( UPD_FILEINFO );
  }
}*/

/* Deactivates the current playlist. */
/*void
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
}*/

/*static ULONG handle_dfi_error( ULONG rc, const char* file )
{
  char buf[512];

  if (rc == 0) return 0;

  *buf = '\0';

  if( rc == 100 ) {
    sprintf( buf, "The file %s could not be read.", file );
  } else if( rc == 200 ) {
    sprintf( buf, "The file %s cannot be played by PM123. The file might be corrupted or the necessary plug-in not loaded or enabled.", file );
  } else {
    amp_stop();
    sprintf( buf, "%s: Error occurred: %s", file, xio_strerror( rc ));
  }

  WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, buf, "Error", 0, MB_ERROR | MB_OK );
  return 1;
}*/

static void amp_AddMRU(Playlist* list, size_t max, const char* URL)
{ DEBUGLOG(("amp_AddMRU(%p{%s}, %u, %s)\n", list, list->GetURL().cdata(), max, URL));
  Mutex::Lock lock(list->Mtx);
  sco_ptr<PlayableEnumerator> pe(list->GetEnumerator());
  // remove the desired item from the list and limit the list size
  pe->Next();
  while (pe->IsValid())
  { PlayableInstance* pi = *pe;
    pe->Next();
    DEBUGLOG(("amp_AddMRU - %p{%s}\n", pi, pi->GetPlayable().GetURL().cdata()));
    if (max == 0 || pi->GetPlayable().GetURL() == URL)
      list->RemoveItem(pi);
     else
      --max;
  }
  // prepend list with new item
  pe->Next();
  list->InsertItem(URL, xstring(), *pe);
}

/* Loads a standalone file or CD track to player. */
BOOL
amp_load_playable( const char* url, int options )
{ DEBUGLOG(("amp_load_playable(%s, %x)\n", url, options));

  int_ptr<Playable> play = Playable::GetByURL(url);
  play->EnsureInfo(Playable::IF_Status);
  if (play->GetStatus() == STA_Invalid)
  { amp_error(hframe, "Can't play %s.", url);
    return FALSE;
  }

  amp_stop();

  Current.Attach(play);
  // Move always to the first element.
  Current.Next();

  amp_display_filename();
  amp_invalidate( UPD_FILEINFO );

  DEBUGLOG(("amp_load_playable - attached\n"));

  if( !( options & AMP_LOAD_NOT_PLAY )) {
    if( cfg.playonload ) {
      amp_play( 0 );
    }
  }

  if( !( options & AMP_LOAD_NOT_RECALL ))
    amp_AddMRU(LoadMRU, MAX_RECALL, url);

  return TRUE;
}

/* Begins playback of the currently loaded file from the specified
   position. Must be called from the main thread. */
static void
amp_pb_play( float pos )
{ DEBUGLOG(("amp_pb_play(%f)\n", pos));

  Song* song = Current.GetCurrentSong();
  if (song == NULL)
  { song = Current.Next();
    if (!song)
    { // Kein Song geladen, oder leere Playliste
      WinSendDlgItemMsg( hplayer, BMP_PLAY, WM_DEPRESS, 0, 0 );
      return;
    }
  }

  msg_play( hplayer, *song, pos );
  amp_set_bubbletext( BMP_PLAY, "Stops playback" );

  WinSendDlgItemMsg( hplayer, BMP_FWD,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_REW,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PLAY,  WM_PRESS  , 0, 0 );

  xstring title = song->GetURL().getDisplayName() + " - " AMP_FULLNAME;
  WinSetWindowText( hframe, title.cdata() );
}

/* Stops the playing of the current file. Must be called
   from the main thread. */
static void
amp_pb_stop( void )
{
  QMSG qms;

  msg_stop();
  amp_set_bubbletext( BMP_PLAY, "Starts playing" );

  WinSendDlgItemMsg( hplayer, BMP_PLAY,  WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_FWD,   WM_DEPRESS, 0, 0 );
  WinSendDlgItemMsg( hplayer, BMP_REW,   WM_DEPRESS, 0, 0 );
  WinSetWindowText ( hframe,  AMP_FULLNAME );

  /* TODO:
  if( amp_playmode == AMP_PLAYLIST ) {
    pl_mark_as_stop();
  }*/

  while( WinPeekMsg( hab, &qms, hplayer, WM_PLAYSTOP, WM_PLAYSTOP, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_PLAYSTOP message.\n" ));
  }
  while( WinPeekMsg( hab, &qms, hplayer, WM_OUTPUT_OUTOFDATA, WM_OUTPUT_OUTOFDATA, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_OUTPUT_OUTOFDATA message.\n" ));
  }
  while( WinPeekMsg( hab, &qms, hplayer, WM_PLAYERROR, WM_PLAYERROR, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_PLAYERROR message.\n" ));
  }

  amp_invalidate( UPD_ALL );
}

/* Suspends or resumes playback of the currently played file.
   Must be called from the main thread. */
static void
amp_pb_pause( void )
{
  if( decoder_playing())
  {
    msg_pause();

    if( is_paused()) {
      WinSendDlgItemMsg( hplayer, BMP_PAUSE, WM_PRESS, 0, 0 );
      return;
    }
  }
}

/* Begins playback of the currently loaded file from
   the specified position. */
void amp_play( float pos ) {
  DEBUGLOG(("amp_play(%f)\n", pos));
  WinSendMsg( hplayer, AMP_PLAY, MPFROMLONG(*(int*)&pos), 0 );
}

/* Stops playback of the currently played file. */
void amp_stop( void ) {
  WinSendMsg( hplayer, AMP_STOP, 0, 0 );
}

/* Suspends or resumes playback of the currently played file. */
void amp_pause( void ) {
  WinSendMsg( hplayer, AMP_PAUSE, 0, 0 );
}

/* Shows the context menu of the playlist. */
static void
amp_show_context_menu( HWND parent )
{
  static HWND menu = NULLHANDLE;
  if( menu == NULLHANDLE )
  {
    menu = WinLoadMenu( HWND_OBJECT, 0, MNU_MAIN );

    mn_make_conditionalcascade( menu, IDM_M_LOAD, IDM_M_LOADFILE );

    PlaylistMenu* pmp = new PlaylistMenu(parent, IDM_M_LAST, IDM_M_LAST_E);
    pmp->AttachMenu(IDM_M_BOOKMARKS, DefaultBM->GetContent(), PlaylistMenu::DummyIfEmpty|PlaylistMenu::Recursive|PlaylistMenu::Enumerate, 0);
    pmp->AttachMenu(IDM_M_LOAD, LoadMRU, PlaylistMenu::Enumerate|PlaylistMenu::Separator, 0);
  }

  POINTL   pos;
  WinQueryPointerPos( HWND_DESKTOP, &pos );
  WinMapWindowPoints( HWND_DESKTOP, parent, &pos, 1 );

  if( WinWindowFromPoint( parent, &pos, TRUE ) == NULLHANDLE )
  {
    SWP swp;
    // The context menu is probably activated from the keyboard.
    WinQueryWindowPos( parent, &swp );
    pos.x = swp.cx/2;
    pos.y = swp.cy/2;
  }

  // Update regulars.
  MENUITEM mi;
  WinSendMsg( menu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_LOAD, TRUE ), MPFROMP( &mi ));

  // Append asisstents from decoder plug-ins
  memset( load_wizzards+2, 0, sizeof load_wizzards - 2*sizeof *load_wizzards ); // You never know...
  append_load_menu( mi.hwndSubMenu, IDM_M_LOADOTHER, 2, load_wizzards+2, sizeof load_wizzards / sizeof *load_wizzards -2);

  /*DEBUGLOG(("amp_show_context_menu: cfg.last = %s\n", cfg.last));
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
          sfnameext( buff, cfg.last[i], sizeof( buff ));
          sdecode( buff, buff, sizeof( buff ));
          strlcat( file, buff, sizeof( buff ));
        } else {
          sfnameext( file + strlen( file ), cfg.last[i], sizeof( file ) - strlen( file ) );
        }

        mi.iPosition = MIT_END;
        mi.afStyle = MIS_TEXT;
        mi.afAttribute = 0;
        mi.id = (IDM_M_LAST + 1) + i;
        mi.hwndSubMenu = (HWND)NULLHANDLE;
        mi.hItem = 0;

        WinSendMsg( mh, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( file ));
        DEBUGLOG2(("amp_show_context_menu: append recent \"%s\"\n", file));
      }
    }
  }*/

  // Update plug-ins.
  WinSendMsg( menu, MM_QUERYITEM,
              MPFROM2SHORT( IDM_M_PLUG, TRUE ), MPFROMP( &mi ));

  load_plugin_menu( mi.hwndSubMenu );

  // Update status
  const Song* song = Current.GetCurrentSong();
  #ifdef DEBUG
  if (!song)
    DEBUGLOG(("amp_show_context_menu: current = NULL\n"));
   else
    DEBUGLOG(("amp_show_context_menu: current = %p, meta_write = %i\n", song, song->GetInfo().meta_write ));
  #endif
  mn_enable_item( menu, IDM_M_TAG,     song != NULL && song->GetInfo().meta_write );
  mn_enable_item( menu, IDM_M_SMALL,   bmp_is_mode_supported( CFG_MODE_SMALL   ));
  mn_enable_item( menu, IDM_M_NORMAL,  bmp_is_mode_supported( CFG_MODE_REGULAR ));
  mn_enable_item( menu, IDM_M_TINY,    bmp_is_mode_supported( CFG_MODE_TINY    ));
  mn_enable_item( menu, IDM_M_FONT,    cfg.font_skinned );
  mn_enable_item( menu, IDM_M_FONT1,   bmp_is_font_supported( 0 ));
  mn_enable_item( menu, IDM_M_FONT2,   bmp_is_font_supported( 1 ));
  mn_enable_item( menu, IDM_M_ADDBOOK, song != NULL );

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

/* TODO: needs playlist handle???
    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ) &&
      ( pdinfo->cditem > 1 && pdinfo->hwndSource == hplaylist )) {

      drag    = DOR_NEVERDROP;
      drag_op = DO_UNKNOWN;
      break;

    } else*/ if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ) ||
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
{ DEBUGLOG(("amp_drag_drop(%p, %p)\n", hwnd, pdinfo));
  PDRAGITEM pditem;

  char fullname[_MAX_PATH];
  int  i;

  if( !DrgAccessDraginfo( pdinfo )) {
    return 0;
  }

  DEBUGLOG(("amp_drag_drop: {,%u,%x,%p, %u,%u, %u,}\n",
    pdinfo->cbDragitem, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem));

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    DrgQueryStrName( pditem->hstrContainerName, sizeof fullname, fullname );
    size_t len = strlen(fullname);
    DrgQueryStrName( pditem->hstrSourceName,    sizeof fullname - len, fullname+len );

    if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ))
    {
      if( pditem->hstrContainerName && pditem->hstrSourceName ) {
        // Have full qualified file name.
        if( DrgVerifyType( pditem, "UniformResourceLocator" )) {
          amp_url_from_file( fullname, fullname, sizeof( fullname ));
        }

        if (is_dir(fullname))
          strlcat(fullname, "/", sizeof fullname);

        url URL = url::normalizeURL(fullname);

        if( pdinfo->cditem == 1 ) {
          amp_load_playable( URL, 0 );
        } else {
//        TODO !!!
//          pl_add_file( fullname, NULL, 0 );
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
    /*else if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ))
    { // TODO: @@@@@@@@ unsure whether this is good for anything
      if( pdinfo->cditem == 1 ) {
        if( pdinfo->hwndSource == hplaylist && amp_playmode == AMP_PLAYLIST ) {
          amp_pl_play_record((PLRECORD*)pditem->ulItemID );
        } else {
          amp_load_singlefile( fullname, 0 );
        }
      } else {
        pl_add_file( fullname, NULL, 0 );
      }
    }*/
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

    if (is_dir(fullname))
      strlcat(fullname, "/", sizeof fullname);

    url URL = url::normalizeURL(fullname);

    if( pdsource->cditem == 1 ) {
      amp_load_playable( fullname, 0 );
    } else {
//    TODO !!!
//    pl_add_file( fullname, NULL, 0 );
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
      char* type  = (char*)malloc( len );
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

/* Wizzard function for the default entry "File..." */
ULONG DLLENTRY amp_file_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param )
{ DEBUGLOG(("amp_file_wizzard(%p, %s, %p, %p)\n", owner, title, callback, param));

  FILEDLG filedialog = { sizeof( FILEDLG ) };
  { char  buf[2048]; // well, static buffer size...
    buf[2047] = 0;
    dec_fill_types( buf, sizeof buf-1 );
    xstring type_audio = xstring::sprintf(FDT_AUDIO"%s)", buf);
    xstring type_all = xstring::sprintf(FDT_AUDIO_ALL"%s)", buf);

    APSZ types[] = {
      { (PSZ)&*type_audio }, // OS/2 and const...
      { FDT_PLAYLIST },
      { (PSZ)&*type_all }, // OS/2 and const...
      { NULL } };

    xstring wintitle = xstring::sprintf(title, " file(s)");

    filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM | FDS_MULTIPLESEL;
    filedialog.ulUser         = FDU_DIR_ENABLE | FDU_RECURSEBTN;
    filedialog.pszTitle       = (PSZ)&*wintitle; // OS/2 and const...
    filedialog.hMod           = NULLHANDLE;
    filedialog.usDlgId        = DLG_FILE;
    filedialog.pfnDlgProc     = amp_file_dlg_proc;
    filedialog.papszITypeList = types;
    filedialog.pszIType       = (PSZ)&*type_all; // OS/2 and const...

    strlcpy( filedialog.szFullFile, cfg.filedir, sizeof filedialog.szFullFile );
    WinFileDlg( HWND_DESKTOP, owner, &filedialog );
  }

  ULONG ret = 300; // Cancel unless DID_OK

  if( filedialog.lReturn == DID_OK ) {
    ret = 0;

    char* file = filedialog.ulFQFCount > 1
      ? **filedialog.papszFQFilename
      : filedialog.szFullFile;

    if (*file)
      sdrivedir( cfg.filedir, file, sizeof( cfg.filedir ));

    ULONG count = 0;
    while (*file)
    { DEBUGLOG(("amp_file_wizzard: %s\n", file));
      char fileurl[_MAX_FNAME+25]; // should be sufficient in all cases
      strcpy(fileurl, "file:///");
      strcpy(fileurl + (url::isPathDelimiter(file[0]) && url::isPathDelimiter(file[1]) ? 5 : 8), file);
      char* dp = fileurl + strlen(fileurl);
      if (is_dir(file))
      { // Folder => add trailing slash
        if (!url::isPathDelimiter(dp[-1]))
          *dp++ = '/';
        if (filedialog.ulUser & FDU_RECURSE_ON)
        { strcpy(dp, "?recursive");
          dp += 10;
        } else
          *dp = 0;
      }
      // convert slashes
      dp = strchr(fileurl+7, '\\');
      while (dp)
      { *dp = '/';
        dp = strchr(dp+1, '\\');
      }
      // Callback
      (*callback)(param, fileurl);
      // next file
      if (++count >= filedialog.ulFQFCount)
        break;
      file = (*filedialog.papszFQFilename)[count];
    }
  }

  WinFreeFileDlgList( filedialog.papszFQFilename );

  return ret;
}

/* Default dialog procedure for the URL dialog. */
static MRESULT EXPENTRY
amp_url_dlg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch( msg )
  {case WM_CONTROL:
    if (MPFROM2SHORT(ENT_URL, EN_CHANGE) != mp1)
      break;
   case WM_INITDLG:
    // Update enabled status of the OK-Button
    WinEnableWindow(WinWindowFromID(hwnd, DID_OK), WinQueryDlgItemTextLength(hwnd, ENT_URL) != 0);
    break;

   case WM_COMMAND:
    DEBUGLOG(("amp_url_dlg_proc: WM_COMMAND: %i\n", SHORT1FROMMP(mp1)));
    if (SHORT1FROMMP(mp1) == DID_OK)
    { HWND ent = WinWindowFromID(hwnd, ENT_URL);
      LONG len = WinQueryWindowTextLength(ent);
      xstring text;
      WinQueryWindowText(ent, len+1, text.raw_init(len));
      if (url::normalizeURL(text))
        break; // everything OK => continue
      WinMessageBox(HWND_DESKTOP, hwnd, xstring::sprintf("The URL \"%s\" is not well formed.", text.cdata()),
        NULL, 0, MB_CANCEL|MB_WARNING|MB_APPLMODAL|MB_MOVEABLE);
      return 0; // cancel
    }
  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

/* Adds HTTP file to the playlist or load it to the player. */
ULONG DLLENTRY
amp_url_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param )
{ DEBUGLOG(("amp_url_wizzard(%p, %s, %p, %p)\n", owner, title, callback, param));

  HWND hwnd = WinLoadDlg( HWND_DESKTOP, owner, amp_url_dlg_proc, NULLHANDLE, DLG_URL, 0 );
  if (hwnd == NULLHANDLE)
    return 500;

  do_warpsans(hwnd);

  xstring wintitle = xstring::sprintf(title, " URL");
  WinSetWindowText(hwnd, (PSZ)&*wintitle);

  // TODO: absolute size limit???
  char durl[2048];
  WinSendDlgItemMsg( hwnd, ENT_URL, EM_SETTEXTLIMIT, MPFROMSHORT(sizeof durl), 0 );

  // TODO: last URL
  // TODO: 2. recent URLs

  ULONG ret = 300;
  if (WinProcessDlg(hwnd) == DID_OK)
  { WinQueryDlgItemText(hwnd, ENT_URL, sizeof durl, durl);
    DEBUGLOG(("amp_url_wizzard: %s\n", durl));
    url nurl = url::normalizeURL(durl);
    DEBUGLOG(("amp_url_wizzard: %s\n", nurl.cdata()));
    (*callback)(param, nurl);
    ret = 0;
  }
  WinDestroyWindow(hwnd);
  return ret;
}

static void DLLENTRY
amp_load_file_callback( void* param, const char* url )
{ DEBUGLOG(("amp_load_file_callback(%p{%u}, %s)\n", param, *(bool*)param, url));
  if (*(bool*)param)
  { // TODO: handle multiple items
    amp_load_playable( url, 0 );
    *(bool*)param = false;
  }
}

/* Loads a file selected by the user to the player. */
/*static void
amp_load_file( HWND owner )
{ DEBUGLOG(("amp_load_file(%p)\n", owner));
  bool first = true;
  ULONG ul = amp_file_wizzard(owner, "Load %s", &amp_load_file_callback, &first);
  DEBUGLOG(("amp_load_file - %u\n", ul));
}*/

/*static void DLLENTRY
amp_add_files_callback( void*, const char* url )
{ DEBUGLOG(("amp_add_files_callback(, %s)\n", url));
  // compatibility to old format here
  // we only have to handle files in this place.
  if (strcmp(url, "file:") == 0)
  { url += 5; // skip "file:";
    if (url[2] == '/')
      url += 3; // skip "///" if not UNC.
    size_t len = strlen(url);
    if (url[len-2] == '/' && url[len-1] == '*')
      pl_add_directory( url, PL_DIR_RECURSIVE );
     else if (url[-1] == '/')
      pl_add_directory( url, 0 );
     else if ( is_playlist( url ))
      pl_load( url, PL_LOAD_NOT_RECALL );
     else
      pl_add_file( url, NULL, 0 );
  } else
  { size_t len = strlen(url);
    if ( is_playlist( url ))
      pl_load( url, PL_LOAD_NOT_RECALL );
     else
      pl_add_file( url, NULL, 0 );
  }
}*/

/* Adds user selected files or directory to the playlist. */
/*void
amp_add_files( HWND hwnd )
{ DEBUGLOG(("amp_add_files(%p)\n", hwnd));

  ULONG ul = amp_file_wizzard(hwnd, "Add%s to playlist", &amp_add_files_callback, NULL);
  DEBUGLOG(("amp_add_files - %u\n", ul));
  if (ul == 0)
    pl_completed();
}*/

/* Adds HTTP file to the playlist or load it to the player. */
/*void
amp_add_url( HWND owner, int options )
{ DEBUGLOG(("amp_add_url(%p)\n", owner));

  bool first;
  ULONG ul = options & URL_ADD_TO_LIST
    ? amp_url_wizzard(owner, "Add%s to playlist", &amp_add_files_callback, &first)
    : amp_url_wizzard(owner, "Load%s", &amp_load_file_callback, &first);
  DEBUGLOG(("amp_add_url - %u\n", ul));
  if (ul == 0 && options & URL_ADD_TO_LIST)
    pl_completed();
}*/

/* Edits a ID3 tag for the specified file. */
void
amp_info_edit( HWND owner, const char* filename, const char* decoder )
{
  ULONG rc = dec_editmeta( owner, filename, decoder );
  switch (rc)
  { default:
      amp_error( owner, "Cannot edit tag of file:\n%s", filename);
      return;

    case 0:   // tag changed
      Playable::GetByURL(filename)->LoadInfoAsync(Playable::IF_Meta);
      // Refresh will come automatically
    case 300: // tag unchanged
      return;

    case 500:
      amp_error( owner, "Unable write tag to file:\n%s\n%s", filename, clib_strerror(errno));
      return;
  }
}


/****************************************************************************
* Pipe Functions
****************************************************************************/

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
amp_pipe_write( HPIPE pipe, const char *buf )
{
  ULONG action;

  DosWrite( pipe, (PVOID)buf, strlen( buf ) + 1, &action );
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
// TODO: start the pipe thread!
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
        if( is_dir( buffer ))
          strlcat(buffer, "/", sizeof buffer);
        amp_load_playable( url::normalizeURL(buffer), 0 );
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
              int_ptr<Playable> current = Current.GetRoot();
              if( !current ) {
                amp_pipe_write( hpipe, "" );
              } else if( stricmp( dork, "file" ) == 0 ) {
                amp_pipe_write( hpipe, current->GetURL().cdata() );
              } else if( stricmp( dork, "tag"  ) == 0 ) {
                char info[512];
                current->EnsureInfo(Playable::IF_All);
                amp_pipe_write( hpipe, amp_construct_tag_string( info, &current->GetInfo(), sizeof( info )));
              } else if( stricmp( dork, "info" ) == 0 ) {
                current->EnsureInfo(Playable::IF_Tech);
                amp_pipe_write( hpipe, current->GetInfo().tech->info );
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
              // TODO: edit playlist
              //pl_add_directory( dork, PL_DIR_RECURSIVE );
            }
          }
          if( stricmp( zork, "dir"  ) == 0 ) {
            if( dork ) {
              // TODO: edit playlist
              //pl_add_directory( dork, 0 );
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
                // TODO: edit playlist
                //pl_add_file( file, NULL, 0 );
              }
            }
          }
          if( stricmp( zork, "load" ) == 0 ) {
            if( dork ) {
              // TODO: dir
              amp_load_playable( url::normalizeURL(dork), 0 );
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
//          TODO: makes no more sense
//            amp_pl_use();
          }
          if( stricmp( zork, "clear" ) == 0 ) {
            // TODO: edit playlist
            //pl_clear( PL_CLR_NEW );
          }
          if( stricmp( zork, "next" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_NEXT ), 0 );
          }
          if( stricmp( zork, "previous" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PREV ), 0 );
          }
          if( stricmp( zork, "remove" ) == 0 ) {
            // TODO: pipe interface have to be renewed
            /*if( current_record ) {
              PLRECORD* rec = current_record;
              pl_remove_record( &rec, 1 );
            }*/
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
              msg_seek( atoi( dork ) * 1000 );
            }
          }
          if( stricmp( zork, "play" ) == 0 ) {
            if( dork ) {
              // TODO: dir
              amp_load_playable( url::normalizeURL(dork), AMP_LOAD_NOT_PLAY );
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            } else if( !decoder_playing()) {
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            }
          }
          if( stricmp( zork, "pause" ) == 0 ) {
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


/* Adds a user selected bookmark. */
static void bm_add_bookmark(HWND owner, Playable* item, const PlayableInstance::slice& sl)
{ DEBUGLOG(("bm_add_bookmark(%x, %p{%s}, {%.1f,%.1f})\n", owner, item, item->GetDisplayName().cdata(), sl.Start, sl.Stop));
  // TODO: !!!!!! request information before
  const META_INFO& meta = *item->GetInfo().meta;
  xstring desc = "";
  if (*meta.artist)
    desc = xstring(meta.artist) + "-";
  if (*meta.title)
    desc = desc + meta.title;
   else
    desc = desc + item->GetURL().getShortName();

  HWND hdlg = WinLoadDlg(HWND_DESKTOP, owner, &WinDefDlgProc, NULLHANDLE, DLG_BM_ADD, NULL);

  WinSetDlgItemText(hdlg, EF_BM_DESC, desc);

  if (WinProcessDlg(hdlg) == DID_OK)
  { xstring alias;
    char* cp = alias.raw_init(WinQueryDlgItemTextLength(hdlg, EF_BM_DESC));
    WinQueryDlgItemText(hdlg, EF_BM_DESC, alias.length()+1, cp);
    if (alias == desc)
      alias = NULL; // Don't set alias if not required.
    ((Playlist&)*DefaultBM->GetContent()).InsertItem(item->GetURL(), alias, sl);
    // TODO !!!!!
    //bm_save( owner );
  }

  WinDestroyWindow(hdlg);
}

/* Loads a playlist selected by the user to the player. */
/*void
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
}*/

/* Saves current playlist to the file specified by user. */
/*void
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
}*/

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

/* Returns TRUE if the save stream feature has been enabled. */
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
   already played. */
static void
amp_playstop( HWND hwnd )
{ DEBUGLOG(("amp_playstop(%p)\n", hwnd));

  if (Current.Next() || (cfg.rpt && Current.Next()))
    amp_play( 0 );
   else
    amp_stop();

  /* uhh, well, do we need some of that anymore ?
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
      amp_play( 0 );
    }
  }*/
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
        value = (ULONG)(( db * slider_range / 2 ) / 12 + slider_range / 2);

        WinSendDlgItemMsg( hwnd, 101 + i, SLM_SETSLIDERINFO,
                           MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                           MPFROMSHORT( value ));

        WinSendDlgItemMsg( hwnd, 125 + i, BM_SETCHECK,
                           MPFROMSHORT( mutes[i] ), 0 );
      }

      db = 20 * log10( preamp );
      value = (ULONG)(( db * slider_range / 2 ) / 12 + slider_range / 2);

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
            msg_equalize( gains, mutes, preamp, 1 );
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
  DEBUGLOG2(("amp_dlg_proc(%p, %p, %p, %p)\n", hwnd, msg, mp1, mp2));
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

    case AMP_STOP:
      amp_pb_stop();
      return 0;

    case AMP_PLAY:
    { long L = LONGFROMMP(mp1);
      amp_pb_play( *(float*)&L);
      return 0;
    }
    case AMP_PAUSE:
      amp_pb_pause();
      return 0;

    case AMP_DISPLAY_MSG:
      if( mp2 ) {
        amp_error( hwnd, "%s", mp1 );
      } else {
        amp_info ( hwnd, "%s", mp1 );
      }
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
      bmp_draw_rate( hps, LONGFROMMP( mp1 ) );
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
      DEBUGLOG(("WM_PLAYSTOP\n"));
      // TODO: prepare next file
      out_flush(); // flush remaining content, activate WM_OUTPUT_OUTOFDATA
      return 0;

    // Posted by the output plug-in
    case WM_OUTPUT_OUTOFDATA:
      DEBUGLOG(("WM_OUTPUT_OUTOFDATA\n"));
      // All samples are played, let's stop.
      amp_playstop( hwnd );
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
        DEBUGLOG(("amp_dlg_proc: WM_TIMER\n"));
        if( out_playing_pos() && cfg.mode == CFG_MODE_REGULAR )
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
      // TODO: turn into general metadata replacement and move this to the plugman core
      // and do the update by notifications.
      /*char* metadata = (char*)PVOIDFROMMP(mp1);
      char* titlepos;
      int   i;

      if( metadata ) {
        titlepos = strstr( metadata, "StreamTitle='" );
        if( titlepos )
        {
          META_INFO meta =
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
      }*/
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
        /*if( cm->cmd > IDM_M_LAST )
        {
          amp_load_playable( url::normalizeURL(cfg.last[cm->cmd-IDM_M_LAST-1]), 0 );
          return 0;
        }*/
        if( cm->cmd >= IDM_M_LOADFILE &&
            cm->cmd < IDM_M_LOADFILE + sizeof load_wizzards / sizeof *load_wizzards &&
            load_wizzards[cm->cmd-IDM_M_LOADFILE] )
        { // TODO: create temporary playlist
          bool first = true;
          ULONG rc = (*load_wizzards[cm->cmd-IDM_M_LOADFILE])( hwnd, "Load%s", &amp_load_file_callback, &first );
          return 0;
        }
      }

      switch( cm->cmd )
      {
        case IDM_M_MANAGER:
          DefaultPM->SetVisible(true);
          return 0;

        case IDM_M_ADDBOOK:
        { // fetch song
          int_ptr<Song> song = Current.GetCurrentSong();
          if (!song)
            return 0; // can't help, the file is gone

          bm_add_bookmark(hwnd, song, PlayableInstance::slice(out_playing_pos()));
          return 0;
        }

        case IDM_M_EDITBOOK:
          DefaultBM->SetVisible(true);
          return 0;

        case IDM_M_TAG:
        { Song* song = Current.GetCurrentSong();
          if (song)
            amp_info_edit( hwnd, song->GetURL(), song->GetDecoder() );
          return 0;
        }
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

        case IDM_M_CFG:
          cfg_properties( hwnd );
          if( cfg.dock_windows ) {
            dk_arrange( hframe );
          } else {
            dk_cleanup( hframe );
          }
          return 0;

        case IDM_M_PLAYLIST:
          DefaultPL->SetVisible(true);
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
          DefaultPL->SetVisible(!DefaultPL->GetVisible());
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

          /* TODO:
          if( amp_playmode == AMP_PLAYLIST ) {
            pl_clean_shuffle();
          }*/
          if( cfg.shf ) {
            /*if( decoder_playing()) {
              pl_mark_as_play();
            }*/
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
            amp_play( 0 );
          } else {
            WinSendMsg( hwnd, WM_COMMAND, MPFROMSHORT( BMP_STOP ), mp2 );
          }
          return 0;

        case BMP_PAUSE:
          amp_pause();
          return 0;

        case BMP_FLOAD:
        { bool first = true;
          // Well, only one load entry is supported this way.
          ULONG rc = (*load_wizzards[0])( hwnd, "Load%s", &amp_load_file_callback, &first );
          return 0;
        }
        case BMP_STOP:
          amp_stop();
          // TODO: probably inclusive when the iterator is destroyed
          //pl_clean_shuffle();
          return 0;

        case BMP_NEXT:
        { if (Current.GetRoot())
          { BOOL decoder_was_playing = decoder_playing();
            if (decoder_was_playing)
              amp_stop();
            if (Current.Next() && decoder_was_playing)
              amp_play( 0 );
          }
          return 0;
        }
        case BMP_PREV:
        { if (Current.GetRoot())
          { BOOL decoder_was_playing = decoder_playing();
            if (decoder_was_playing)
              amp_stop();
            if (Current.Prev() && decoder_was_playing)
              amp_play( 0 );
          }
          return 0;
        }

        case BMP_FWD:
          if( decoder_playing() && !is_paused())
          {
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
            msg_forward();
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
            msg_rewind();
            WinSendDlgItemMsg( hwnd, BMP_REW, is_rewind() ? WM_PRESS : WM_DEPRESS, 0, 0 );
            amp_volume_adjust();
          } else {
            WinSendDlgItemMsg( hwnd, BMP_REW, WM_DEPRESS, 0, 0 );
          }
          return 0;
      }
      break;
    }

    case PlaylistMenu::UM_SELECTED:
    { // bookmark selected
      const PlaylistMenu::select_data* data = (PlaylistMenu::select_data*)PVOIDFROMMP(mp1);
      BOOL rc = amp_load_playable(data->Item->GetURL(), AMP_LOAD_NOT_PLAY|AMP_LOAD_NOT_RECALL|AMP_LOAD_KEEP_PLAYLIST);

      if( rc && (cfg.playonload))
        amp_play(data->Slice.Start);
    }

    case WM_CREATE:
    {
      HPS hps;
      hplayer = hwnd; /* we have to assign the window handle early, because WinCreateStdWindow does not have returned now. */

      hps = WinGetPS( hwnd );
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
        if( Current.GetRoot() && bmp_pt_in_text( pos )) {
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
        seeking_pos = bmp_calc_time( pos, dec_length());

        bmp_draw_slider( hps, seeking_pos, dec_length());
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
        seeking_pos = bmp_calc_time( pos, dec_length());

        msg_seek( seeking_pos );
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
        is_slider_drag = TRUE;
        is_seeking     = TRUE;
        WinSetCapture( HWND_DESKTOP, hwnd );
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
      
/*    case 0x4321:
      DEBUGLOG(("amp_dlg_proc: my pretty sent message # %i\n", LONGFROMMP(mp1)));
      return mp1;
    case 0x5432:
      DEBUGLOG(("amp_dlg_proc: my pretty post message # %i\n", LONGFROMMP(mp1)));
      return 0;*/
  }

  DEBUGLOG2(("amp_dlg_proc: before WinDefWindowProc\n"));
  return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

/*static MRESULT EXPENTRY
amp_dlg_proc2( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ MRESULT r = amp_dlg_proc(hwnd, msg, mp1, mp2);
  DEBUGLOG(("amp_dlg_proc: end %x\n", msg));
  return r;
}
static void TFNENTRY
amp_sender_thread(void* param)
{ HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);
  
  for (int i = 1; i < 100000; ++i)
  { DEBUGLOG(("amp_sender_thread: before WinPostMsg #%i\n", i));
    WinPostMsg(hframe, 0x5432, MPFROMLONG(i), 0);
    DEBUGLOG(("amp_sender_thread: before WinSendMsg #%i\n", i));
    WinSendMsg(hframe, 0x4321, MPFROMLONG(i), 0);
    DEBUGLOG(("amp_sender_thread: after WinSendMsg\n"));
    DosSleep(0);
  }
  
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}*/


/* Stops playing and resets the player to its default state. */
void
amp_reset( void )
{
  if( decoder_playing()) {
    amp_stop();
  }

  Current.Attach(NULL);

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
  { owner  =  HWND_DESKTOP;
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
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  DEBUGLOG(("ERROR: %s\n", message.cdata()));
  amp_message_box( hframe, "PM123 Error", message, MB_ERROR | MB_OK | MB_MOVEABLE );
}

/* Creates and displays a error message window.
   The specified owner window is disabled. */
void
amp_error( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  DEBUGLOG(("ERROR: %x, %s\n", owner, message.cdata()));
  amp_message_box( owner, "PM123 Error", message, MB_ERROR | MB_OK | MB_MOVEABLE );
}

/* Creates and displays a message window. */
void
amp_info( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  DEBUGLOG(("INFO: %s\n", message.cdata()));
  amp_message_box( owner, "PM123 Information", message, MB_INFORMATION | MB_OK | MB_MOVEABLE );
}

/* Requests the user about specified action. Returns
   TRUE at confirmation or FALSE in other case. */
BOOL
amp_query( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  return amp_message_box( owner, "PM123 Query", message, MB_QUERY | MB_YESNO | MB_MOVEABLE ) == MBID_YES;
}

/* Requests the user about overwriting a file. Returns
   TRUE at confirmation or at absence of a file. */
BOOL
amp_warn_if_overwrite( HWND owner, const char* filename )
{
  struct stat fi;
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


struct args
{ int argc;
  char** argv;
  int files;
};

static void TFNENTRY
main2( void* arg )
{
  int    argc = ((args*)arg)->argc;
  char** argv = ((args*)arg)->argv;
  int    files = ((args*)arg)->files;
  HMQ    hmq;
  char   bundle [_MAX_PATH];
  char   infname[_MAX_PATH];
  QMSG   qmsg;
  struct stat fi;

  ///////////////////////////////////////////////////////////////////////////
  // Initialization of infrastructure
  ///////////////////////////////////////////////////////////////////////////
  
  HELPINIT hinit;
  ULONG    flCtlData = FCF_TASKLIST | FCF_NOBYTEALIGN | FCF_ACCELTABLE | FCF_ICON;

  hab = WinInitialize( 0 );
  PMXASSERT(hmq = WinCreateMsgQueue( hab, 0 ), != NULLHANDLE);

  load_ini();
  //amp_volume_to_normal(); // Superfluous!
  InitButton( hab );

  // initialize properties
  cfg_init();

  // these two are always constant
  load_wizzards[0] = amp_file_wizzard;
  load_wizzards[1] = amp_url_wizzard;

  WinRegisterClass( hab, "PM123", amp_dlg_proc, CS_SIZEREDRAW /* | CS_SYNCPAINT */, 0 );

  hframe = WinCreateStdWindow( HWND_DESKTOP, 0, &flCtlData, "PM123",
                               AMP_FULLNAME, 0, NULLHANDLE, WIN_MAIN, &hplayer );

  DEBUGLOG(("main: window created\n"));

  do_warpsans( hframe  );
  do_warpsans( hplayer );

  Playable::Init();

  dk_init();
  dk_add_window( hframe, DK_IS_MASTER );

  DEBUGLOG(("main: create subwindows\n"));

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
    amp_load_playable( url::normalizeURL(argv[argc - 1]), 0 );
  } else if( files > 0 ) {
    // TODO: same as on load_file_callback
    /*for( i = 1; i < argc; i++ ) {
      if( argv[i][0] != '/' && argv[i][0] != '-' ) {
        if( is_dir( argv[i] )) {
          pl_add_directory( argv[i], PL_DIR_RECURSIVE );
        } else {
          pl_add_file( argv[i], NULL, 0 );
        }
      }
    }
    pl_completed();*/
  } else {
    struct stat fi;
    if( stat( bundle, &fi ) == 0 ) {
      // TODO: what's that?
//      pl_load_bundle( bundle, 0 );
    }
  }

  WinSetWindowPos( hframe, HWND_TOP,
                   cfg.main.x, cfg.main.y, 0, 0, SWP_ACTIVATE | SWP_MOVE | SWP_SHOW );

  // Init some other stuff
  PlaylistView::Init();
  PlaylistManager::Init();

  // Init default lists
  { const url path = url::normalizeURL(startpath);
    DefaultPL = PlaylistView::Get(path + "PM123.LST", "Default Playlist");
    DefaultPM = PlaylistManager::Get(path + "PFREQ.LST", "Playlist Manager");
    DefaultBM = PlaylistView::Get(path + "BOOKMARK.LST", "Bookmarks");
    LoadMRU   = (Playlist*)&*Playable::GetByURL(path + "LOADMRU.LST");
  }

  DEBUGLOG(("main: visinit...\n"));

  // initialize non-skin related visual plug-ins
  vis_init_all( FALSE );

//  bm_load( hplayer );

  if( cfg.dock_windows ) {
    dk_arrange( hframe );
  }

  // TODO: TEST!!!
  //DefaultPL->SetVisible(cfg.show_playlist);
  //DefaultPM->SetVisible(cfg.show_plman);
  //DefaultBM->SetVisible(cfg.show_bmarks);
 
  DEBUGLOG(("main: init complete\n"));
  //_beginthread(amp_sender_thread, NULL, 65536, NULL);

  ///////////////////////////////////////////////////////////////////////////
  // Main loop
  ///////////////////////////////////////////////////////////////////////////
  while( WinGetMsg( hab, &qmsg, (HWND)0, 0, 0 ))
  { DEBUGLOG2(("main: after WinGetMsg - %p, %p\n", qmsg.hwnd, qmsg.msg));
    WinDispatchMsg( hab, &qmsg );
    DEBUGLOG2(("main: after WinDispatchMsg - %p, %p\n", qmsg.hwnd, qmsg.msg));
  }
  DEBUGLOG(("main: dispatcher ended\n"));

  ///////////////////////////////////////////////////////////////////////////
  // Stop and save configuration
  ///////////////////////////////////////////////////////////////////////////
  amp_stop();

  save_ini();
//  bm_save( hplayer );
//  pl_save_bundle( bundle, 0 );

  // deinitialize all visual plug-ins
  vis_deinit_all( TRUE );
  vis_deinit_all( FALSE );

  if( heq != NULLHANDLE ) {
    WinDestroyWindow( heq );
  }

  ///////////////////////////////////////////////////////////////////////////
  // Uninitialize infrastructure
  ///////////////////////////////////////////////////////////////////////////
  Current.Attach(NULL);

  LoadMRU   = NULL;
  DefaultBM = NULL;
  DefaultPM = NULL;
  DefaultPL = NULL;

  PlaylistManager::UnInit();
  PlaylistView::UnInit();
 
  bmp_clean_skin();
  remove_all_plugins();
  dk_term();
  Playable::Uninit();

  WinDestroyWindow  ( hframe  );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );

  #ifdef __DEBUG_ALLOC__
  _dump_allocated( 0 );
  #endif
}

int
main( int argc, char *argv[] )
{
  int    i, o, files = 0;
  char   exename[_MAX_PATH];
  char   command[1024];

  // used for debug printf()s
  setvbuf( stderr, NULL, _IONBF, 0 );

  for( i = 1; i < argc; i++ ) {
    if( argv[i][0] != '/' && argv[i][0] != '-' ) {
      files++;
    }
  }

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

  srand((unsigned long)time( NULL ));

  // now init PM
  args args = { argc, argv, files };
  main2(&args);
  return 0;
}
