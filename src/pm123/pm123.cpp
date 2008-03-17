/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 M.Mueller
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
#include "123_util.h"
#include "plugman.h"
#include "button95.h"
#include "playlistmanager.h"
#include "playlistview.h"
#include "copyright.h"
#include "docking.h"
#include "iniman.h"
#include "controller.h"
#include "playlistmenu.h"
#include "skin.h"
#include "pipe.h"
#include <visual_plug.h>

#include <cpp/xstring.h>
#include <cpp/url123.h>
#include <cpp/showaccel.h>
#include "pm123.rc.h"

#include <debuglog.h>

#define  AMP_REFRESH_CONTROLS   ( WM_USER + 1000 ) /* 0,         0                            */
#define  AMP_PAINT              ( WM_USER + 1001 ) /* options,   0                            */
#define  AMP_LOAD               ( WM_USER + 1002 )
#define  AMP_DISPLAY_MESSAGE    ( WM_USER + 1013 ) /* message,   TRUE (info) or FALSE (error) */
#define  AMP_DISPLAY_MODE       ( WM_USER + 1014 ) /* 0,         0                            */
#define  AMP_QUERY_STRING       ( WM_USER + 1015 ) /* buffer,    size and type                */
#define  AMP_INFO_EDIT          ( WM_USER + 1016 )
#define  AMP_CTRL_EVENT         ( WM_USER + 1020 )
#define  AMP_CTRL_EVENT_CB      ( WM_USER + 1021 )
#define  AMP_REFRESH_ACCEL      ( WM_USER + 1022 )

#define  TID_UPDATE_TIMERS    ( TID_USERMAX - 1 )
#define  TID_UPDATE_PLAYER    ( TID_USERMAX - 2 )
#define  TID_ONTOP            ( TID_USERMAX - 3 )

/* file dialog additional flags */
#define  FDU_DIR_ENABLE   0x0001
#define  FDU_RECURSEBTN   0x0002
#define  FDU_RECURSE_ON   0x0004
#define  FDU_RELATIVBTN   0x0008
#define  FDU_RELATIV_ON   0x0010


/* Contains startup path of the program without its name.  */
char startpath[_MAX_PATH];

// Default playlist, representing PM123.LST in the program folder.
static int_ptr<Playlist> DefaultPL;
// PlaylistManager window, representing PFREQ.LST in the program folder.
static int_ptr<Playlist> DefaultPM;
// Default instance of bookmark window, representing BOOKMARK.LST in the program folder.
static int_ptr<Playlist> DefaultBM;
// Most recent used entries in the load menu, representing LOADMRU.LST in the program folder.
static int_ptr<Playlist> LoadMRU;


static HAB   hab        = NULLHANDLE;
//static HWND  hplaylist  = NULLHANDLE;
static HWND  heq        = NULLHANDLE;
static HWND  hframe     = NULLHANDLE;
static HWND  hplayer    = NULLHANDLE;
static HWND  hhelp      = NULLHANDLE;

static BOOL  is_have_focus    = FALSE;
static BOOL  is_volume_drag   = FALSE;
static BOOL  is_seeking       = FALSE;
static BOOL  is_slider_drag   = FALSE;
static BOOL  is_arg_shuffle   = FALSE;
static bool  is_msg_status    = false; // true if a MsgStatus to the controller message is on the way
static bool  is_accel_changed = false;
static bool  is_plugin_changed = true;

/* Current load wizzards */
static DECODER_WIZZARD_FUNC load_wizzards[20];

/* Current seeking time. Valid if is_slider_drag == TRUE. */
static T_TIME seeking_pos = 0;
static int    upd_options = 0;

/* Equalizer stuff. */
float gains[20];
BOOL  mutes[20];
float preamp;

/* status cache */
static Ctrl::PlayStatus last_status;


static void amp_reset_status()
{ last_status.CurrentItem     = -1;
  last_status.TotalItems      = -1;
  last_status.CurrentTime     = -1;
  last_status.TotalTime       = -1;
  last_status.CurrentSongTime = -1;
  last_status.TotalSongTime   = -1;
}

void DLLENTRY
amp_display_info( const char* info )
{
  char* message = strdup( info );
  if( message ) {
    WinPostMsg( hplayer, AMP_DISPLAY_MESSAGE, MPFROMP( message ), MPFROMLONG( FALSE ));
  }
  WinPostMsg( hplayer, WM_PLAYERROR, 0, 0 );
}

void DLLENTRY
amp_display_error( const char *info )
{
  char* message = strdup( info );
  if( message ) {
    WinPostMsg( hplayer, AMP_DISPLAY_MESSAGE, MPFROMP( message ), MPFROMLONG( TRUE ));
  }
}

void DLLENTRY pm123_control( int index, void* param )
{
  switch (index)
  {
    case CONTROL_NEXTMODE:
      WinSendMsg( hplayer, AMP_DISPLAY_MODE, 0, 0 );
      break;
  }
}

int DLLENTRY pm123_getstring( int index, int subindex, size_t bufsize, char* buf )
{ if (bufsize)
    *buf = 0;
  switch (index)
  {case STR_VERSION:
    strlcpy( buf, AMP_FULLNAME, bufsize );
    break;

   case STR_DISPLAY_TEXT:
    strlcpy( buf, bmp_query_text(), bufsize );
    break;

   case STR_FILENAME:
    { int_ptr<Song> song = Ctrl::GetCurrentSong();
      if (song)
        strlcpy(buf, song->GetURL(), bufsize);
      break;
    }
   
   case STR_DISPLAY_TAG:
    { int_ptr<Song> song = Ctrl::GetCurrentSong();
      if (song)
      { const xstring& text = amp_construct_tag_string( &song->GetInfo() );
        strlcpy(buf, text, bufsize);
      }
      break;
    }
    
   case STR_DISPLAY_INFO:
    { int_ptr<Song> song = Ctrl::GetCurrentSong();
      if (song)
        strlcpy(buf, song->GetInfo().tech->info, bufsize);
      break;
    }

   default: break;
  }
 return(0);
}

Playlist* amp_get_default_pl()
{ return DefaultPL;
}

static void amp_control_event_handler(void* rcv, const Ctrl::EventFlags& flags)
{ DEBUGLOG(("amp_control_event_handler(%p, %x)\n", rcv, flags));
  QMSG msg;
  if (WinPeekMsg(hab, &msg, hframe, AMP_CTRL_EVENT, AMP_CTRL_EVENT, PM_REMOVE))
  { DEBUGLOG(("amp_control_event_handler - hit: %x\n", msg.mp1));
    // join messages
    (Ctrl::EventFlags&)msg.mp1 |= flags;
    WinPostMsg(hframe, AMP_CTRL_EVENT, msg.mp1, 0);
  } else
    WinPostMsg(hframe, AMP_CTRL_EVENT, MPFROMLONG(flags), 0);
}

static void amp_control_event_callback(Ctrl::ControlCommand* cmd)
{ DEBUGLOG(("amp_control_event_callback(%p{%i, ...)\n", cmd, cmd->Cmd));
  WinPostMsg(hframe, AMP_CTRL_EVENT_CB, MPFROMP(cmd), 0);
}

/* Draws all player timers and the position slider. */
static void
amp_paint_timers( HPS hps )
{
  DEBUGLOG(("amp_paint_timers(%p) - %i %i {%i/%i, %g/%g, %g/%g}\n", hps, cfg.mode, is_seeking,
    last_status.CurrentItem, last_status.TotalItems, last_status.CurrentTime, last_status.TotalTime, last_status.CurrentSongTime, last_status.TotalSongTime));

  T_TIME list_left = -1;
  T_TIME play_time = is_seeking ? seeking_pos : last_status.CurrentSongTime;
  T_TIME play_left = last_status.TotalSongTime;

  if (play_left > 0)
    play_left -= play_time;

  if( !Ctrl::IsRepeat() && last_status.TotalTime > 0 )
    list_left = last_status.TotalTime - last_status.CurrentTime - play_time;

  bmp_draw_slider( hps, play_time, last_status.TotalSongTime);
  bmp_draw_timer ( hps, play_time );

  bmp_draw_tiny_timer( hps, POS_TIME_LEFT, play_left );
  bmp_draw_tiny_timer( hps, POS_PL_LEFT,   list_left );

  bmp_draw_plind( hps, last_status.CurrentItem, last_status.CurrentItem > 0 ? last_status.TotalItems : 0);
}

/* Draws all attributes of the currently loaded file. */
static void
amp_paint_fileinfo( HPS hps )
{
  DEBUGLOG(("amp_paint_fileinfo(%p)\n", hps));

  int_ptr<Playable> pp = Ctrl::GetRoot();
  bmp_draw_plmode( hps, pp != NULL, pp ? pp->GetFlags() : Playable::None );

  pp = Ctrl::GetCurrentSong();
  if (pp != NULL)
  { bmp_draw_rate    ( hps, pp->GetInfo().tech->bitrate );
    bmp_draw_channels( hps, pp->GetInfo().format->channels );
  } else
  { bmp_draw_rate    ( hps, -1 );
    bmp_draw_channels( hps, -1 );
  }
  bmp_draw_text( hps );
}

/* Marks the player window as needed of redrawing. */
void
amp_invalidate( int options )
{ DEBUGLOG(("amp_invalidate(%x)\n", options));
  if( options & UPD_WINDOW ) {
    WinInvalidateRect( hplayer, NULL, 1 );
    options &= ~UPD_WINDOW;
  }
  if( options & UPD_DELAYED ) {
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

/* Posts a command to the message queue associated with the player window. */
/*BOOL
amp_post_command( USHORT id ) {
  return WinPostMsg( hplayer, WM_COMMAND, MPFROMSHORT( id ), 0 );
}*/

/* Sets bubble text for specified button. */
static void
amp_set_bubbletext( USHORT id, const char *text )
{
  WinSendDlgItemMsg( hplayer, id, WM_SETHELP, MPFROMP(text), 0 );
}

/* Constructs a information text for currently loaded file
   and selects it for displaying. */
void
amp_display_filename( void )
{
  int_ptr<Song> song = Ctrl::GetCurrentSong();
  DEBUGLOG(("amp_display_filename() %p %u\n", &*song, cfg.viewmode));
  if (!song) {
    bmp_set_text( "No file loaded" );
    return;
  }

  xstring text;
  switch( cfg.viewmode )
  {
    case CFG_DISP_ID3TAG:
      text = amp_construct_tag_string(&song->GetInfo());
      if (text.length())
        break;
      // if tag is empty - use filename instead of it.
    case CFG_DISP_FILENAME:
      text = song->GetURL().getShortName();
      break;

    case CFG_DISP_FILEINFO:
      text = song->GetInfo().tech->info;
      break;
  }
  bmp_set_text( text );
}

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
  int_ptr<PlayableInstance> pi;
  // remove the desired item from the list and limit the list size
  while ((pi = list->GetNext(pi)) != NULL)
  { DEBUGLOG(("amp_AddMRU - %p{%s}\n", &*pi, pi->GetPlayable()->GetURL().cdata()));
    if (max == 0 || pi->GetPlayable()->GetURL() == URL)
      list->RemoveItem(pi);
     else
      --max;
  }
  // prepend list with new item
  list->InsertItem(URL, xstring(), list->GetNext(NULL));
}

/* Loads a standalone file or CD track to player. */
void
amp_load_playable( const char* url, T_TIME start, int options )
{ DEBUGLOG(("amp_load_playable(%s, %x)\n", url, options));

  if (options & AMP_LOAD_APPEND)
  { ASSERT(Ctrl::GetRoot() != NULL);
    PlayableInstance::slice sl(start);
    // multi mode
    if (Ctrl::GetRoot() != DefaultPL)
    { // we do not yet use the current playlist => use it
      // move current item to the list
      DefaultPL->Clear();
      DefaultPL->InsertItem(Ctrl::GetRoot()->GetURL(), (const char*)NULL, sl);
      // reset current to first item of the playlist
      Ctrl::PostCommand(Ctrl::MkLoad(DefaultPL->GetURL()));
    }
    // append item
    DefaultPL->InsertItem(url, (const char*)NULL, sl);
    return;
  }

  Ctrl::ControlCommand* cmd = Ctrl::MkLoad(url);
  if (start)
    cmd->Link = Ctrl::MkNavigate(xstring(), start, false, false);
  if( !( options & AMP_LOAD_NOT_PLAY )) {
    if( cfg.playonload )
      // start playback immediately after loading has completed
      (cmd->Link ? cmd->Link->Link : cmd->Link) = Ctrl::MkPlayStop(Ctrl::Op_Set);
  }
  Ctrl::PostCommand(cmd);

  if( !( options & AMP_LOAD_NOT_RECALL ))
    amp_AddMRU(LoadMRU, MAX_RECALL, url);
}

/* loads the accelerater table and modifies it by the plugins */
static void
amp_load_accel()
{ DEBUGLOG(("amp_load_accel()\n"));
  // Generate new accelerator table.
  HACCEL haccel = WinLoadAccelTable(hab, NULLHANDLE, ACL_MAIN);
  PMASSERT(haccel != NULLHANDLE);
  memset( load_wizzards+2, 0, sizeof load_wizzards - 2*sizeof *load_wizzards ); // You never know...
  dec_append_accel_table( haccel, IDM_M_LOADOTHER, 0, load_wizzards+2, sizeof load_wizzards / sizeof *load_wizzards -2);
  // Replace table of current window.
  HACCEL haccel_old = WinQueryAccelTable(hab, hframe);
  PMRASSERT(WinSetAccelTable(hab, haccel, hframe));
  if (haccel_old != NULLHANDLE)
    PMRASSERT(WinDestroyAccelTable(haccel_old));
  // done
  is_accel_changed = true;
}

/* Shows the context menu of the playlist. */
static void
amp_show_context_menu( HWND parent )
{
  static HWND menu = NULLHANDLE;
  bool new_menu = false;
  if( menu == NULLHANDLE )
  { menu = WinLoadMenu( HWND_OBJECT, 0, MNU_MAIN );
    mn_make_conditionalcascade( menu, IDM_M_LOAD, IDM_M_LOADFILE );

    PlaylistMenu* pmp = new PlaylistMenu(parent, IDM_M_LAST, IDM_M_LAST_E);
    pmp->AttachMenu(IDM_M_BOOKMARKS, DefaultBM, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Recursive|PlaylistMenu::Enumerate, 0);
    pmp->AttachMenu(IDM_M_LOAD, LoadMRU, PlaylistMenu::Enumerate|PlaylistMenu::Separator, 0);
    new_menu = true;
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

  if (is_plugin_changed || new_menu)
  { // Update plug-ins.
    MENUITEM mi;
    PMRASSERT(WinSendMsg( menu, MM_QUERYITEM, MPFROM2SHORT( IDM_M_PLUG, TRUE ), MPFROMP( &mi )));
    is_plugin_changed = false;
    load_plugin_menu( mi.hwndSubMenu );
  }

  if (is_accel_changed || new_menu)
  { MENUITEM mi;
    PMRASSERT(WinSendMsg( menu, MM_QUERYITEM, MPFROM2SHORT( IDM_M_LOAD, TRUE ), MPFROMP( &mi )));
    // Append asisstents from decoder plug-ins
    memset( load_wizzards+2, 0, sizeof load_wizzards - 2*sizeof *load_wizzards ); // You never know...
    is_accel_changed = false;
    dec_append_load_menu( mi.hwndSubMenu, IDM_M_LOADOTHER, 2, load_wizzards+2, sizeof load_wizzards / sizeof *load_wizzards -2);
    ( MenuShowAccel( WinQueryAccelTable( hab, hframe ))).ApplyTo( new_menu ? menu : mi.hwndSubMenu );
  }

  // Update status
  int_ptr<Song> song = Ctrl::GetCurrentSong();
  int_ptr<Playable> root = Ctrl::GetRoot();

  mn_enable_item( menu, IDM_M_TAG,     song != NULL && song->GetInfo().meta_write );
  mn_enable_item( menu, IDM_M_CURRENT_PL, root != NULL && root->GetFlags() & Playable::Enumerable );
  mn_enable_item( menu, IDM_M_SMALL,   bmp_is_mode_supported( CFG_MODE_SMALL   ));
  mn_enable_item( menu, IDM_M_NORMAL,  bmp_is_mode_supported( CFG_MODE_REGULAR ));
  mn_enable_item( menu, IDM_M_TINY,    bmp_is_mode_supported( CFG_MODE_TINY    ));
  mn_enable_item( menu, IDM_M_FONT,    cfg.font_skinned );
  mn_enable_item( menu, IDM_M_FONT1,   bmp_is_font_supported( 0 ));
  mn_enable_item( menu, IDM_M_FONT2,   bmp_is_font_supported( 1 ));
  mn_enable_item( menu, IDM_M_ADDBOOK, song != NULL );

  mn_check_item ( menu, IDM_M_FLOAT,   cfg.floatontop  );
  mn_check_item ( menu, IDM_M_SAVE,    !!Ctrl::GetSavename() );
  mn_check_item ( menu, IDM_M_FONT1,   cfg.font == 0   );
  mn_check_item ( menu, IDM_M_FONT2,   cfg.font == 1   );
  mn_check_item ( menu, IDM_M_SMALL,   cfg.mode == CFG_MODE_SMALL   );
  mn_check_item ( menu, IDM_M_NORMAL,  cfg.mode == CFG_MODE_REGULAR );
  mn_check_item ( menu, IDM_M_TINY,    cfg.mode == CFG_MODE_TINY    );

  WinPopupMenu( parent, parent, menu, pos.x, pos.y, 0,
                PU_HCONSTRAIN   | PU_VCONSTRAIN |
                PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD );
}

/* Prepares the player to the drop operation. */
static MRESULT
amp_drag_over( HWND hwnd, DRAGINFO* pdinfo )
{ DEBUGLOG(("amp_drag_over(%p, %p)\n", hwnd, pdinfo));

  PDRAGITEM pditem;
  int       i;
  USHORT    drag_op = 0;
  USHORT    drag    = DOR_NEVERDROP;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROM2SHORT( DOR_NEVERDROP, 0 );
  }

  DEBUGLOG(("amp_drag_over(%p, %p{,,%x, %p, %i,%i, %u,})\n", hwnd,
    pdinfo, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem));

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    if (DrgVerifyRMF(pditem, "DRM_OS2FILE", NULL) || DrgVerifyRMF(pditem, "DRM_123FILE", NULL))
    { drag    = DOR_DROP;
      drag_op = DO_COPY;
    } else {
      drag    = DOR_NEVERDROP;
      drag_op = 0;
      break;
    }
  }

  DrgFreeDraginfo( pdinfo );
  return MPFROM2SHORT( drag, drag_op );
}

struct DropInfo
{ //USHORT count;    // Count of dragged objects.
  //USHORT index;    // Index of current item [0..Count)
  xstring URL;      // Object URL
  int     options;  // Options for amp_load_playable
  HWND    hwndItem; // Window handle of the source of the drag operation.
  ULONG   ulItemID; // Information used by the source to identify the
                    // object being dragged.
};

/* Receives dropped files or playlist records. */
static MRESULT
amp_drag_drop( HWND hwnd, PDRAGINFO pdinfo )
{ DEBUGLOG(("amp_drag_drop(%p, %p)\n", hwnd, pdinfo));

  if( !DrgAccessDraginfo( pdinfo )) {
    return 0;
  }

  DEBUGLOG(("amp_drag_drop: {,%u,%x,%p, %u,%u, %u,}\n",
    pdinfo->cbDragitem, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem));

  int options = AMP_LOAD_NOT_RECALL;
  if (pdinfo->cditem > 1 || cfg.append_dnd)
  { options |= AMP_LOAD_APPEND;
    if (!cfg.append_dnd)
      DefaultPL->Clear();
  }

  for( int i = 0; i < pdinfo->cditem; i++ )
  {
    DRAGITEM* pditem = DrgQueryDragitemPtr( pdinfo, i );
    DEBUGLOG(("amp_drag_drop: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, amp_string_from_drghstr(pditem->hstrType).cdata(), amp_string_from_drghstr(pditem->hstrRMF).cdata(),
      amp_string_from_drghstr(pditem->hstrContainerName).cdata(), amp_string_from_drghstr(pditem->hstrSourceName).cdata(), amp_string_from_drghstr(pditem->hstrTargetName).cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    ULONG reply = DMFL_TARGETFAIL;

    if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ))
    {
      // fetch full qualified path
      size_t lenP = DrgQueryStrNameLen(pditem->hstrContainerName);
      size_t lenN = DrgQueryStrNameLen(pditem->hstrSourceName);
      xstring fullname;
      char* cp = fullname.raw_init(lenP + lenN);
      DrgQueryStrName(pditem->hstrContainerName, lenP+1, cp);
      DrgQueryStrName(pditem->hstrSourceName,    lenN+1, cp+lenP);

      if (pditem->hwndItem && DrgVerifyType(pditem, "UniformResourceLocator"))
      { // URL => The droped item must be rendered.
        DRAGTRANSFER* pdtrans = DrgAllocDragtransfer(1);
        if (pdtrans)
        { DropInfo* pdsource = new DropInfo();
          pdsource->options  = options;
          pdsource->hwndItem = pditem->hwndItem;
          pdsource->ulItemID = pditem->ulItemID;

          pdtrans->cb               = sizeof( DRAGTRANSFER );
          pdtrans->hwndClient       = hwnd;
          pdtrans->pditem           = pditem;
          pdtrans->hstrSelectedRMF  = DrgAddStrHandle("<DRM_OS2FILE,DRF_TEXT>");
          pdtrans->hstrRenderToName = 0;
          pdtrans->ulTargetInfo     = (ULONG)pdsource;
          pdtrans->fsReply          = 0;
          pdtrans->usOperation      = pdinfo->usOperation;

          // Send the message before setting a render-to name.
          if ( pditem->fsControl & DC_PREPAREITEM
            && !DrgSendTransferMsg(pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdtrans, 0) )
          { // Failure => do not send DM_ENDCONVERSATION
            DrgFreeDragtransfer(pdtrans);
            delete pdsource;
            continue;
          }
          pdtrans->hstrRenderToName = DrgAddStrHandle(tmpnam(NULL));
          // Send the message after setting a render-to name.
          if ( (pditem->fsControl & (DC_PREPARE | DC_PREPAREITEM)) == DC_PREPARE
            && !DrgSendTransferMsg(pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdtrans, 0) )
          { // Failure => do not send DM_ENDCONVERSATION
            DrgFreeDragtransfer(pdtrans);
            delete pdsource;
            continue;
          }
          // Ask the source to render the selected item.
          BOOL ok = LONGFROMMR(DrgSendTransferMsg(pditem->hwndItem, DM_RENDER, (MPARAM)pdtrans, 0));

          if (ok) // OK => DM_ENDCONVERSATION is send at DM_RENDERCOMPLETE
            continue;
          // something failed => we have to cleanup ressources immediately and cancel the conversation
          DrgFreeDragtransfer(pdtrans);
          delete pdsource;
          // ... send DM_ENDCONVERSATION below
        }
      } else if (pditem->hstrContainerName && pditem->hstrSourceName)
      { // Have full qualified file name.
        // Hopefully this criterion is sufficient to identify folders.
        if (pditem->fsControl & DC_CONTAINER)
        { // TODO: should be alterable
          if (cfg.recurse_dnd)
            fullname = fullname + "/?Recursive";
          else
            fullname = fullname + "/";
        }

        DropInfo* pdsource = new DropInfo();
        pdsource->URL      = url123::normalizeURL(fullname);
        pdsource->options  = options;
        WinPostMsg(hwnd, AMP_LOAD, MPFROMP(pdsource), 0);
        reply = DMFL_TARGETSUCCESSFUL;
      }

    } else if (DrgVerifyRMF(pditem, "DRM_123FILE", NULL))
    { // In the DRM_123FILE transfer mechanism the target is responsable for doing the target related stuff
      // while the source does the source related stuff. So a DO_MOVE operation causes
      // - a create in the target window and
      // - a remove in the source window.
      // The latter is done when DM_ENDCONVERSATION arrives with DMFL_TARGETSUCCESSFUL.

      DRAGTRANSFER* pdtrans = DrgAllocDragtransfer(1);
      if (pdtrans)
      { pdtrans->cb               = sizeof(DRAGTRANSFER);
        pdtrans->hwndClient       = hwnd;
        pdtrans->pditem           = pditem;
        pdtrans->hstrSelectedRMF  = DrgAddStrHandle("<DRM_123FILE,DRF_UNKNOWN>");
        pdtrans->hstrRenderToName = 0;
        pdtrans->fsReply          = 0;
        pdtrans->usOperation      = pdinfo->usOperation;

        // Ask the source to render the selected item.
        DrgSendTransferMsg(pditem->hwndItem, DM_RENDER, (MPARAM)pdtrans, 0);

        // insert item
        if ((pdtrans->fsReply & DMFL_NATIVERENDER))
        { DropInfo* pdsource = new DropInfo();
          pdsource->URL      = amp_string_from_drghstr(pditem->hstrSourceName);
          pdsource->options  = options;
          WinPostMsg(hwnd, AMP_LOAD, MPFROMP(pdsource), 0);
          reply = DMFL_TARGETSUCCESSFUL;
        }
        // cleanup
        DrgFreeDragtransfer(pdtrans);
      }
    }
    // Tell the source you're done.
    DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION, MPFROMLONG(pditem->ulItemID), MPFROMLONG(reply));
  }

  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );
  return 0;
}

/* Receives dropped and rendered files and urls. */
static MRESULT
amp_drag_render_done( HWND hwnd, PDRAGTRANSFER pdtrans, USHORT rc )
{
  DropInfo* pdsource = (DropInfo*)pdtrans->ulTargetInfo;

  ULONG reply = DMFL_TARGETFAIL;
  // If the rendering was successful, use the file, then delete it.
  if ((rc & DMFL_RENDEROK) && pdsource)
  { // fetch render to name
    const xstring& rendered = amp_string_from_drghstr(pdtrans->hstrRenderToName);
    // fetch file content
    const xstring& fullname = amp_url_from_file(rendered);
    DosDelete(rendered);

    if (fullname)
    { pdsource->URL = url123::normalizeURL(fullname);
      WinPostMsg(hwnd, AMP_LOAD, MPFROMP(pdsource), 0);
      pdsource = NULL; // Do not delete the DropInfo below.
      reply = DMFL_TARGETSUCCESSFUL;
    }
  }

  // Tell the source you're done.
  DrgSendTransferMsg( pdsource->hwndItem, DM_ENDCONVERSATION,
                     (MPARAM)pdsource->ulItemID, (MPARAM)reply);

  delete pdsource;
  DrgDeleteStrHandle ( pdtrans->hstrSelectedRMF );
  DrgDeleteStrHandle ( pdtrans->hstrRenderToName );
  DrgFreeDragtransfer( pdtrans );
  return 0;
}

/* Default dialog procedure for the file dialog. */
MRESULT EXPENTRY
amp_file_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ DEBUGLOG(("amp_file_dlg_proc(%x, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
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
    PMRASSERT( WinFileDlg( HWND_DESKTOP, owner, &filedialog ));
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
      strcpy(fileurl + (url123::isPathDelimiter(file[0]) && url123::isPathDelimiter(file[1]) ? 5 : 8), file);
      char* dp = fileurl + strlen(fileurl);
      if (is_dir(file))
      { // Folder => add trailing slash
        if (!url123::isPathDelimiter(dp[-1]))
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
      if (url123::normalizeURL(text))
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
{ DEBUGLOG(("amp_url_wizzard(%x, %s, %p, %p)\n", owner, title, callback, param));

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
    url123 nurl = url123::normalizeURL(durl);
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
  amp_load_playable( url, 0, *(bool*)param ? 0 : AMP_LOAD_APPEND );
  *(bool*)param = false;
}

struct info_edit;
static void amp_info_event(info_edit* iep, const Playable::change_args& args);

struct info_edit
{ HWND              Owner;
  int_ptr<Playable> Song;
  delegate<info_edit, const Playable::change_args> InfoDelegate;
  info_edit(HWND owner, Playable* song)
  : Owner(owner),
    Song(song),
    InfoDelegate(song->InfoChange, &amp_info_event, this)
  {}
};

void amp_info_event(info_edit* iep, const Playable::change_args& args)
{ if (args.Flags & Playable::IF_Other)
  { iep->InfoDelegate.detach();
    PMRASSERT(WinPostMsg(hframe, AMP_INFO_EDIT, iep, 0));
  }
}

/* Edits a ID3 tag for the specified file. */
void
amp_info_edit( HWND owner, Playable* song )
{ DEBUGLOG(("amp_info_edit(%x, %p)\n", owner, song));
  info_edit* iep = new info_edit(owner, song);
  if (song->EnsureInfoAsync(Playable::IF_Other))
  { // Information immediately available => post message
    iep->InfoDelegate.detach(); // do no longer wait for the event
    PMRASSERT(WinPostMsg(hframe, AMP_INFO_EDIT, iep, 0));
  }
}

/* Adds a user selected bookmark. */
static void amp_add_bookmark(HWND owner, Playable* item, const PlayableInstance::slice& sl = PlayableInstance::slice::Initial)
{ DEBUGLOG(("amp_add_bookmark(%x, %p{%s}, {%.1f,%.1f})\n", owner, item, item->GetDisplayName().cdata(), sl.Start, sl.Stop));
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
    DefaultBM->InsertItem(item->GetURL(), alias, sl);
    // TODO !!!!!
    //bm_save( owner );
  }

  WinDestroyWindow(hdlg);
}

/* Saves a playlist */
void amp_save_playlist(HWND owner, PlayableCollection* playlist)
{
  APSZ  types[] = {{ FDT_PLAYLIST_LST }, { FDT_PLAYLIST_M3U }, { 0 }};

  FILEDLG filedialog = {sizeof(FILEDLG)};
  filedialog.fl             = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_CUSTOM | FDS_ENABLEFILELB;
  filedialog.pszTitle       = "Save playlist";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.ulUser         = FDU_RELATIVBTN;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_PLAYLIST_LST;

  if ((playlist->GetFlags() & Playable::Mutable) == Playable::Mutable && playlist->GetURL().isScheme("file://"))
  { // Playlist => save in place allowed => preselect our own file name
    const char* cp = playlist->GetURL().cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    strlcpy(filedialog.szFullFile, cp, sizeof filedialog.szFullFile);
    // preselect file type
    if (playlist->GetURL().getExtension().compareToI(".M3U") == 0)
      filedialog.pszIType = FDT_PLAYLIST_M3U;
    // TODO: other playlist types
  } else
  { // not mutable => only save as allowed
    // TODO: preselect directory
  }

  PMXASSERT(WinFileDlg(HWND_DESKTOP, owner, &filedialog), != NULLHANDLE);

  if(filedialog.lReturn == DID_OK)
  { url123 file = url123::normalizeURL(filedialog.szFullFile);
    if (!(Playable::IsPlaylist(file)))
    { if (file.getExtension().length() == 0)
      { // no extension => choose automatically
        if (strcmp(filedialog.pszIType, FDT_PLAYLIST_M3U) == 0)
          file = file + ".m3u";
        else // if (strcmp(filedialog.pszIType, FDT_PLAYLIST_LST) == 0)
          file = file + ".lst";
        // TODO: other playlist types
      } else
      { amp_error(owner, "PM123 cannot write playlist files with the unsupported extension %s.", file.getExtension().cdata());
        return;
      }
    }
    const char* cp = file.cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    if (amp_warn_if_overwrite(owner, cp))
    { PlayableCollection::save_options so = PlayableCollection::SaveDefault;
      if (file.getExtension().compareToI(".m3u") == 0)
        so |= PlayableCollection::SaveAsM3U;
      if (filedialog.ulUser & FDU_RELATIV_ON)
        so |= PlayableCollection::SaveRelativePath;
      // now save
      if (!playlist->Save(file, so))
        amp_error(owner, "Failed to create playlist \"%s\". Error %s.", file.cdata(), xio_strerror(xio_errno()));
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

/* Returns TRUE if the save stream feature has been enabled. */
static void
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
        Ctrl::PostCommand(Ctrl::MkSave(filedialog.szFullFile), &amp_control_event_callback);
        sdrivedir( cfg.savedir, filedialog.szFullFile, sizeof( cfg.savedir ));
      }
    }
  } else {
    Ctrl::PostCommand(Ctrl::MkSave(xstring()));
  }
}

static void amp_eq_update()
{ DEBUGLOG(("amp_eq_update()\n"));

  Ctrl::EQ_Data data;
  for (int i = 0; i < 20; i++)
    data.bandgain[0][i] = gains[i] * preamp * !mutes[i]; // Attension: dirty out of bounds access to data.bandgain[1][...]

  Ctrl::PostCommand(Ctrl::MkEqualize(&data));
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

        WinSendDlgItemMsg( hwnd, SL_EQ_0 + i, SLM_SETSLIDERINFO,
                           MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                           MPFROMSHORT( value ));

        WinSendDlgItemMsg( hwnd, CB_EQ_0 + i, BM_SETCHECK,
                           MPFROMSHORT( mutes[i] ), 0 );
      }

      db = 20 * log10( preamp );
      value = (ULONG)(( db * slider_range / 2 ) / 12 + slider_range / 2);

      WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_SETSLIDERINFO,
                         MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                         MPFROMSHORT( value ));

      WinSendDlgItemMsg( hwnd, CB_EQ_ENABLED, BM_SETCHECK,
                         MPFROMSHORT( cfg.eq_enabled ), 0 );

      nottheuser = FALSE;
      break;
    }

    case WM_COMMAND:
      switch( COMMANDMSG(&msg)->cmd )
      {
        case BT_EQ_LOAD: // load button
          if( amp_load_eq( hwnd, gains, mutes, &preamp )) {
            WinSendMsg( hwnd, AMP_REFRESH_CONTROLS, 0, 0 );
          }
          if( WinQueryButtonCheckstate( hwnd, 121 )) {
            amp_eq_update();
          }
          break;

        case BT_EQ_SAVE: // save button
          amp_save_eq( hwnd, gains, mutes, preamp );
          break;

        case BT_EQ_DEFAULT: // default button
        {
          int i;

          for( i = 0; i < 20; i++ ) {
            gains[i] = 1.0;
          }
          for( i = 0; i < 20; i++ ) {
            mutes[i] = 0;
          }
          for( i = 0; i < 10; i++ ) {
            WinSendDlgItemMsg( hwnd, SL_EQ_0 + i, SLM_SETSLIDERINFO,
                               MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                               MPFROMLONG( slider_range / 2 ));
          }
          for( i = 0; i < 10; i++ ) {
            WinSendDlgItemMsg( hwnd, CB_EQ_0 + i, BM_SETCHECK, MPFROMSHORT( FALSE ), 0 );
          }

          WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_SETSLIDERINFO,
                             MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                             MPFROMLONG( slider_range / 2 ));

          if( WinQueryButtonCheckstate( hwnd, CB_EQ_ENABLED )) {
            amp_eq_update();
          } else {
            Ctrl::PostCommand(Ctrl::MkEqualize(NULL));
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
        if( id >= CB_EQ_0 && id < CB_EQ_0 + 10 ) // Mute
        {
          mutes[id - CB_EQ_0] = WinQueryButtonCheckstate( hwnd, id );       // Left
          mutes[id - CB_EQ_0 + 10] = WinQueryButtonCheckstate( hwnd, id );  // Right

          if( WinQueryButtonCheckstate( hwnd, CB_EQ_ENABLED )) {
            amp_eq_update();
            break;
          }
        }

        if( id == 121 ) {
          cfg.eq_enabled = WinQueryButtonCheckstate( hwnd, CB_EQ_ENABLED );
          if( cfg.eq_enabled ) {
            amp_eq_update();
          } else {
            Ctrl::PostCommand(Ctrl::MkEqualize(NULL));
          }
          break;
        }
      }

      switch( SHORT2FROMMP( mp1 ))
      {
        case SLN_CHANGE:
          // Slider adjust
          if(( id >= SL_EQ_0 && id < SL_EQ_0 + 10 ) || id == SL_EQ_PREAMP )
          {
            float g2;

            _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
                        EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM);

            // -12 to 12 dB
            g2 = ((float)LONGFROMMP(mp2) - slider_range / 2 ) / ( slider_range / 2 ) * 12;
            // transforming into voltage gain
            g2 = pow( 10.0, g2 / 20.0 );

            if( id == SL_EQ_PREAMP ) {
              preamp = g2;
            } else {
              gains[SHORT1FROMMP(mp1) - SL_EQ_0] = g2;      // Left
              gains[SHORT1FROMMP(mp1) - SL_EQ_0 + 10] = g2; // Right
            }

            if( WinQueryButtonCheckstate( hwnd, CB_EQ_ENABLED )) {
              amp_eq_update();
            }
          }
      }
      break;
    }

    case WM_INITDLG:
    {
      int i;

      nottheuser = TRUE;
      slider_range = SHORT2FROMMR( WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_QUERYSLIDERINFO,
                                   MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                                   MPFROMLONG(0))) - 1;

      for( i = SL_EQ_0; i < SL_EQ_0 + 10; i++ )
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

       WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_SETTICKSIZE, MPFROM2SHORT( SMA_SETALLTICKS, 0 ), 0 );

       WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_ADDDETENT, MPFROMSHORT( 0 ), 0 );
       WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_ADDDETENT, MPFROMSHORT( slider_range / 4 ), 0 );
       WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_ADDDETENT, MPFROMSHORT( slider_range / 2 ), 0 );
       WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_ADDDETENT, MPFROMSHORT( 3 * slider_range / 4 ), 0 );
       WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_ADDDETENT, MPFROMSHORT( slider_range ), 0 );
       WinSendDlgItemMsg( hwnd, SL_EQ_PREAMP, SLM_SETSLIDERINFO,
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
      break;
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

    case AMP_LOAD:
    { DropInfo* pdi = (DropInfo*)mp1;
      amp_load_playable(pdi->URL, 0, pdi->options);
      delete pdi;
      return 0;
    }

    case AMP_INFO_EDIT:
    { info_edit* iep = (info_edit*)PVOIDFROMMP(mp1);
      DEBUGLOG(("amp_dlg_proc: AMP_INFO_EDIT %p{%x, %p,}\n", iep, iep->Owner, &*iep->Song));
      // TODO: THREAD?
      ULONG rc = dec_editmeta( iep->Owner, iep->Song->GetURL(), iep->Song->GetDecoder());
      DEBUGLOG(("amp_dlg_proc: AMP_INFO_EDIT rc = %u\n", rc));
      switch (rc)
      { default:
          amp_error( iep->Owner, "Cannot edit tag of file:\n%s", iep->Song->GetURL().cdata());
          break;
        case 0:   // tag changed
          iep->Song->LoadInfoAsync(Playable::IF_Meta);
          // Refresh will come automatically
        case 300: // tag unchanged
          break;
        case 500:
          amp_error( iep->Owner, "Unable write tag to file:\n%s\n%s", iep->Song->GetURL().cdata(), clib_strerror(errno));
          break;
      }
      delete iep;
      return 0;
    }

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

    case AMP_CTRL_EVENT:
      { // Event from the controller
        Ctrl::EventFlags flags = (Ctrl::EventFlags)LONGFROMMP(mp1);
        DEBUGLOG(("amp_dlg_proc: AMP_CTRL_EVENT %x\n", flags));
        if (flags & Ctrl::EV_PlayStop)
        { if (Ctrl::IsPlaying())
          { WinSendDlgItemMsg(hplayer, BMP_PLAY,  WM_PRESS, 0, 0);
            amp_set_bubbletext(BMP_PLAY, "Stops playback");
          } else
          { WinSendDlgItemMsg(hplayer, BMP_PLAY,  WM_DEPRESS, 0, 0);
            amp_set_bubbletext(BMP_PLAY, "Starts playback");
            WinSetWindowText (hframe,  AMP_FULLNAME);
            if (!is_msg_status)
            { is_msg_status = true;
              Ctrl::PostCommand(Ctrl::MkStatus(), &amp_control_event_callback);
            }
            amp_invalidate( UPD_WINDOW );
          }
        }
        if (flags & Ctrl::EV_Pause)
          WinSendDlgItemMsg(hplayer, BMP_PAUSE,   Ctrl::IsPaused() ? WM_PRESS : WM_DEPRESS, 0, 0);
        if (flags & Ctrl::EV_Forward)
          WinSendDlgItemMsg(hplayer, BMP_FWD,     Ctrl::GetScan() & DECFAST_FORWARD ? WM_PRESS : WM_DEPRESS, 0, 0);
        if (flags & Ctrl::EV_Rewind)
          WinSendDlgItemMsg(hplayer, BMP_REW,     Ctrl::GetScan() & DECFAST_REWIND  ? WM_PRESS : WM_DEPRESS, 0, 0);
        if (flags & Ctrl::EV_Shuffle)
          WinSendDlgItemMsg(hplayer, BMP_SHUFFLE, Ctrl::IsShuffle() ? WM_PRESS : WM_DEPRESS, 0, 0);
        if (flags & Ctrl::EV_Repeat)
        { WinSendDlgItemMsg(hplayer, BMP_REPEAT,  Ctrl::IsRepeat() ? WM_PRESS : WM_DEPRESS, 0, 0);
          amp_invalidate( UPD_TIMERS );
        }
        if (flags & Ctrl::EV_Song && !is_msg_status)
        { is_msg_status = true;
          Ctrl::PostCommand(Ctrl::MkStatus(), &amp_control_event_callback);
        }

        if (flags & (Ctrl::EV_Volume|Ctrl::EV_Tech|Ctrl::EV_Meta))
        { HPS hps = WinGetPS(hplayer);

          if (flags & Ctrl::EV_Volume)
            bmp_draw_volume(hps, Ctrl::GetVolume());

          if (flags & Ctrl::EV_Tech)
            amp_paint_fileinfo(hps);
          if (flags & Ctrl::EV_Meta)
          { xstring title;
            int_ptr<Song> song = Ctrl::GetCurrentSong();
            if (song)
              title = Ctrl::GetCurrentSong()->GetURL().getDisplayName() + " - " AMP_FULLNAME;
            else
              title = AMP_FULLNAME;
            WinSetWindowText(hframe, title.cdata());
            amp_display_filename();
            amp_invalidate( UPD_FILEINFO );
          }
          WinReleasePS(hps);
        }
      }
      return 0;

    case AMP_CTRL_EVENT_CB:
      { Ctrl::ControlCommand* cmd = (Ctrl::ControlCommand*)PVOIDFROMMP(mp1);
        DEBUGLOG(("amp_dlg_proc: AMP_CTRL_EVENT_CB %p{%i, %x}\n", cmd, cmd->Cmd, cmd->Flags));
        switch (cmd->Cmd)
        {case Ctrl::Cmd_Skip:
          WinSendDlgItemMsg(hplayer, BMP_PREV, WM_DEPRESS, 0, 0);
          WinSendDlgItemMsg(hplayer, BMP_NEXT, WM_DEPRESS, 0, 0);
          break;
         case Ctrl::Cmd_PlayStop:
          if (cmd->Flags != Ctrl::RC_OK)
            // release button immediately
            WinSendDlgItemMsg(hplayer, BMP_PLAY, WM_DEPRESS, 0, 0);
          break;
         case Ctrl::Cmd_Pause:
          if (cmd->Flags != Ctrl::RC_OK)
            // release button immediately
            WinSendDlgItemMsg(hplayer, BMP_PAUSE, WM_DEPRESS, 0, 0);
          break;
         case Ctrl::Cmd_Scan:
          if (cmd->Flags != Ctrl::RC_OK)
          { // release buttons immediately
            WinSendDlgItemMsg(hplayer, BMP_FWD, WM_DEPRESS, 0, 0);
            WinSendDlgItemMsg(hplayer, BMP_REW, WM_DEPRESS, 0, 0);
          }
          break;
         case Ctrl::Cmd_Navigate:
          is_seeking = FALSE;
          break;
         case Ctrl::Cmd_Save:
          if (cmd->Flags != Ctrl::RC_OK)
            amp_display_error( "The current active decoder don't support saving of a stream.\n" );
          break;
         case Ctrl::Cmd_Status:
          is_msg_status = false;
          if (cfg.mode == CFG_MODE_REGULAR)
          { HPS hps = WinGetPS( hwnd );
            if (cmd->Flags == Ctrl::RC_OK)
              last_status = *(Ctrl::PlayStatus*)cmd->PtrArg;
            else
              amp_reset_status();
            amp_paint_timers( hps );
            WinReleasePS( hps );
          }
          break;
         default:; // supress warnings
        }
        // TODO: reasonable error messages in case of failed commands.
        // now the command can die
        delete cmd;
      }
      return 0;

    case AMP_REFRESH_ACCEL:
      { // eat other identical messages
        QMSG qmsg;
        while (WinPeekMsg(hab, &qmsg, hwnd, AMP_REFRESH_ACCEL, AMP_REFRESH_ACCEL, PM_REMOVE));
      }
      amp_load_accel();
      return 0;

    case WM_CONTEXTMENU:
      amp_show_context_menu( hwnd );
      return 0;

    case DM_DRAGOVER:
      return amp_drag_over( hwnd, (PDRAGINFO)mp1 );
    case DM_DROP:
      return amp_drag_drop( hwnd, (PDRAGINFO)mp1 );
    case DM_RENDERCOMPLETE:
      return amp_drag_render_done( hwnd, (PDRAGTRANSFER)mp1, SHORT1FROMMP( mp2 ));

    case WM_TIMER:
      DEBUGLOG2(("amp_dlg_proc: WM_TIMER - %x\n", LONGFROMMP(mp1)));
      switch (LONGFROMMP(mp1))
      {case TID_ONTOP:
        WinSetWindowPos( hframe, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER );
        DEBUGLOG2(("amp_dlg_proc: WM_TIMER done\n"));
        return 0;

       case TID_UPDATE_PLAYER:
        { HPS hps = WinGetPS( hwnd );

          if( bmp_scroll_text()) {
            bmp_draw_text( hps );
          }

          if( upd_options ) {
            WinPostMsg( hwnd, AMP_PAINT, MPFROMLONG( upd_options ), 0 );
            upd_options = 0;
          }

          WinReleasePS( hps );
        }
        DEBUGLOG2(("amp_dlg_proc: WM_TIMER done\n"));
        return 0;

       case TID_UPDATE_TIMERS:
        if (Ctrl::IsPlaying() && cfg.mode == CFG_MODE_REGULAR && !is_msg_status)
        { is_msg_status = true;
          Ctrl::PostCommand(Ctrl::MkStatus(), &amp_control_event_callback);
        }
        DEBUGLOG2(("amp_dlg_proc: WM_TIMER done\n"));
        return 0;
      }
      break;

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
        amp_display_filename();
      }
      if( options & UPD_TIMERS   ) {
        amp_paint_timers( hps );
      }
      if( options & UPD_FILEINFO ) {
        amp_paint_fileinfo( hps );
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
      amp_paint_fileinfo ( hps );
      amp_paint_timers   ( hps );
      bmp_draw_led       ( hps, is_have_focus  );
      bmp_draw_volume    ( hps, Ctrl::GetVolume() );

      WinEndPaint( hps );
      return 0;
    }

    case WM_COMMAND:
    { USHORT cmd = SHORT1FROMMP(mp1);
      DEBUGLOG(("amp_dlg_proc: WM_COMMAND(%u, %u, %u)\n", cmd, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      if( cmd > IDM_M_PLUG && cmd <= IDM_M_PLUG_E ) {
        configure_plugin( PLUGIN_NULL, cmd - IDM_M_PLUG - 1, hplayer );
        return 0;
      }
      if( cmd >= IDM_M_LOADFILE && cmd < IDM_M_LOADFILE + sizeof load_wizzards / sizeof *load_wizzards
        && load_wizzards[cmd-IDM_M_LOADFILE] )
      { // TODO: create temporary playlist
        bool first = true;
        (*load_wizzards[cmd-IDM_M_LOADFILE])( hwnd, "Load%s", &amp_load_file_callback, &first );
        return 0;
      }

      switch( cmd )
      {
        case IDM_M_MANAGER:
          PlaylistManager::Get(DefaultPM, "Playlist Manager")->SetVisible(true);
          break;

        case IDM_M_ADDBOOK:
        { int_ptr<Song> song = Ctrl::GetCurrentSong();
          if (song)
            amp_add_bookmark(hwnd, song);
          break;
        }
        case IDM_M_ADDBOOK_TIME:
        { int_ptr<Song> song = Ctrl::GetCurrentSong();
          if (song)
            // Use the time index from last_status here.
            amp_add_bookmark(hwnd, song, PlayableInstance::slice(last_status.CurrentSongTime));
          break;
        }
        case IDM_M_ADDPLBOOK:
        { int_ptr<Playable> root = Ctrl::GetRoot();
          if (root)
            amp_add_bookmark(hwnd, root);
          break;
        }
        case IDM_M_ADDPLBOOK_TIME:
        { // TODO:
          break;
        }

        case IDM_M_EDITBOOK:
          PlaylistView::Get(DefaultBM, "Bookmarks")->SetVisible(true);
          break;

        case IDM_M_TAG:
        { int_ptr<Song> song = Ctrl::GetCurrentSong();
          if (song)
            amp_info_edit( hwnd, song );
          break;
        }
        case IDM_M_DETAILED:
        { int_ptr<Playable> pp = Ctrl::GetRoot();
          if (pp && (pp->GetFlags() & Playable::Enumerable))
            PlaylistView::Get(pp)->SetVisible(true);
          break;
        }
        case IDM_M_TREEVIEW:
        { int_ptr<Playable> pp = Ctrl::GetRoot();
          if (pp && (pp->GetFlags() & Playable::Enumerable))
            PlaylistManager::Get(pp)->SetVisible(true);
          break;
        }
        case IDM_M_ADDPMBOOK:
        { int_ptr<Playable> pp = Ctrl::GetRoot();
          if (pp)
            DefaultPM->InsertItem(pp->GetURL(), (const char*)NULL);
          break;
        }
        case IDM_M_PLSAVE:
        { int_ptr<Playable> pp = Ctrl::GetRoot();
          if (pp && (pp->GetFlags() & Playable::Enumerable))
            amp_save_playlist( hwnd, (PlayableCollection*)&*pp );
          break;
        }
        case IDM_M_SAVE:
          amp_save_stream( hwnd, !Ctrl::GetSavename() );
          break;

        case IDM_M_EQUALIZE:
          amp_eq_show();
          break;

        case IDM_M_FLOAT:
          cfg.floatontop = !cfg.floatontop;

          if( !cfg.floatontop ) {
            WinStopTimer ( hab, hwnd, TID_ONTOP );
          } else {
            WinStartTimer( hab, hwnd, TID_ONTOP, 100 );
          }
          break;

        case IDM_M_FONT1:
          cfg.font = 0;
          amp_invalidate( UPD_FILEINFO | UPD_FILENAME );
          break;

        case IDM_M_FONT2:
          cfg.font = 1;
          amp_invalidate( UPD_FILEINFO | UPD_FILENAME );
          break;

        case IDM_M_TINY:
          cfg.mode = CFG_MODE_TINY;
          bmp_reflow_and_resize( hframe );
          break;

        case IDM_M_NORMAL:
          cfg.mode = CFG_MODE_REGULAR;
          bmp_reflow_and_resize( hframe );
          break;

        case IDM_M_SMALL:
          cfg.mode = CFG_MODE_SMALL;
          bmp_reflow_and_resize( hframe );
          break;

        case IDM_M_MINIMIZE:
          WinSetWindowPos( hframe, HWND_DESKTOP, 0, 0, 0, 0, SWP_HIDE );
          WinSetActiveWindow( HWND_DESKTOP, WinQueryWindow( hwnd, QW_NEXTTOP ));
          break;

        case IDM_M_SKINLOAD:
        {
          HPS hps = WinGetPS( hwnd );
          amp_loadskin( hab, hwnd, hps );
          WinReleasePS( hps );
          amp_invalidate( UPD_WINDOW );
          break;
        }

        case IDM_M_CFG:
          cfg_properties( hwnd );
          amp_invalidate( UPD_FILEINFO | UPD_FILENAME );

          if( cfg.dock_windows ) {
            dk_arrange( hframe );
          } else {
            dk_cleanup( hframe );
          }
          break;

        case IDM_M_PLAYLIST:
          PlaylistView::Get(DefaultPL, "Default Playlist")->SetVisible(true);
          break;

        case IDM_M_VOL_RAISE:
        { // raise volume by 5%
          Ctrl::PostCommand(Ctrl::MkVolume(.05, true));
          break;
        }

        case IDM_M_VOL_LOWER:
        { // lower volume by 5%
          Ctrl::PostCommand(Ctrl::MkVolume(-.05, true));
          break;
        }

        case IDM_M_MENU:
          amp_show_context_menu( hwnd );
          break;

        case BMP_PL:
        { int_ptr<PlaylistView> pv = PlaylistView::Get(DefaultPL, "Default Playlist");
          pv->SetVisible(!pv->GetVisible());
          break;
        }

        case BMP_REPEAT:
          Ctrl::PostCommand(Ctrl::MkRepeat(Ctrl::Op_Toggle));
          WinSendDlgItemMsg( hwnd, BMP_REPEAT, Ctrl::IsRepeat() ? WM_PRESS : WM_DEPRESS, 0, 0 );
          break;

        case BMP_SHUFFLE:
          Ctrl::PostCommand(Ctrl::MkShuffle(Ctrl::Op_Toggle));
          WinSendDlgItemMsg( hwnd, BMP_SHUFFLE, Ctrl::IsShuffle() ? WM_PRESS : WM_DEPRESS, 0, 0 );
          break;

        case BMP_POWER:
          WinPostMsg( hwnd, WM_QUIT, 0, 0 );
          break;

        case BMP_PLAY:
          Ctrl::PostCommand(Ctrl::MkPlayStop(Ctrl::Op_Toggle), &amp_control_event_callback);
          break;

        case BMP_PAUSE:
          Ctrl::PostCommand(Ctrl::MkPause(Ctrl::Op_Toggle), &amp_control_event_callback);
          break;

        case BMP_FLOAD:
        { bool first = true;
          // Well, only one load entry is supported this way.
          ULONG rc = (*load_wizzards[0])( hwnd, "Load%s", &amp_load_file_callback, &first );
          break;
        }
        case BMP_STOP:
          Ctrl::PostCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
          // TODO: probably inclusive when the iterator is destroyed
          //pl_clean_shuffle();
          break;

        case BMP_NEXT:
          Ctrl::PostCommand(Ctrl::MkSkip(1, true), &amp_control_event_callback);
          break;

        case BMP_PREV:
          Ctrl::PostCommand(Ctrl::MkSkip(-1, true), &amp_control_event_callback);
          break;

        case BMP_FWD:
          Ctrl::PostCommand(Ctrl::MkScan(Ctrl::Op_Toggle), &amp_control_event_callback);
          break;

        case BMP_REW:
          Ctrl::PostCommand(Ctrl::MkScan(Ctrl::Op_Toggle|Ctrl::Op_Rewind), &amp_control_event_callback);
          break;
      }
      return 0;
    }

    case PlaylistMenu::UM_SELECTED:
    { // bookmark selected
      const PlaylistMenu::select_data* data = (PlaylistMenu::select_data*)PVOIDFROMMP(mp1);
      amp_load_playable(data->Item->GetURL(), data->Slice.Start, AMP_LOAD_NOT_RECALL|AMP_LOAD_KEEP_PLAYLIST);
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

      if( Ctrl::IsRepeat() ) {
        WinSendDlgItemMsg( hwnd, BMP_REPEAT,  WM_PRESS, 0, 0 );
      }
      if( Ctrl::IsShuffle() ) {
        WinSendDlgItemMsg( hwnd, BMP_SHUFFLE, WM_PRESS, 0, 0 );
      }
      // TODO: the controller should initialize from the startup environment
      if ( is_arg_shuffle )
        Ctrl::PostCommand(Ctrl::MkShuffle(Ctrl::Op_Set));

      WinStartTimer( hab, hwnd, TID_UPDATE_TIMERS, 100 );
      WinStartTimer( hab, hwnd, TID_UPDATE_PLAYER,  50 );
      
      // fetch some initial states
      WinPostMsg( hwnd, AMP_CTRL_EVENT, MPFROMLONG(Ctrl::EV_Repeat|Ctrl::EV_Shuffle), 0 );
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
        Ctrl::PostCommand(Ctrl::MkVolume(bmp_calc_volume(pos), false));
      else if( Ctrl::GetRoot() && bmp_pt_in_text( pos )) {
        WinPostMsg( hwnd, AMP_DISPLAY_MODE, 0, 0 );
      }

      WinReleasePS( hps );
      return 0;
    }

    case WM_MOUSEMOVE:
      if( is_volume_drag )
      {
        POINTL pos;
        pos.x = SHORT1FROMMP(mp1);
        pos.y = SHORT2FROMMP(mp1);
        Ctrl::PostCommand(Ctrl::MkVolume(bmp_calc_volume(pos), false));
      }

      if( is_slider_drag )
      {
        POINTL pos;
        pos.x = SHORT1FROMMP(mp1);
        pos.y = SHORT2FROMMP(mp1);
        seeking_pos = bmp_calc_time( pos, last_status.TotalSongTime );

        HPS hps = WinGetPS( hwnd );
        bmp_draw_slider( hps, seeking_pos, last_status.TotalSongTime );
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
        seeking_pos = bmp_calc_time(pos, last_status.TotalSongTime );

        // TODO: the song may have changed
        Ctrl::PostCommand(Ctrl::MkNavigate(xstring(), seeking_pos, false, false), &amp_control_event_callback);
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
      } else if( bmp_pt_in_slider( pos ) && last_status.TotalSongTime > 0 && Ctrl::GetCurrentSong() ) {
        is_slider_drag = TRUE;
        is_seeking     = TRUE;
        // We can use the time index from last_status here because nothing else is shown currently.
        seeking_pos    = last_status.CurrentSongTime;
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
  }

  DEBUGLOG2(("amp_dlg_proc: before WinDefWindowProc\n"));
  return WinDefWindowProc( hwnd, msg, mp1, mp2 );
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

static void amp_plugin_eventhandler(void*, const PLUGIN_EVENTARGS& args)
{ DEBUGLOG(("amp_plugin_eventhandler(, {%p, %x, %i})\n", args.plugin, args.type, args.operation));
  switch (args.operation)
  {case PLUGIN_EVENTARGS::Enable:
   case PLUGIN_EVENTARGS::Disable:
   case PLUGIN_EVENTARGS::Unload:
    if (args.type == PLUGIN_DECODER)
      WinPostMsg(hplayer, AMP_REFRESH_ACCEL, 0, 0);
    is_plugin_changed = true;
   default:; // avoid warnings
  }
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
  ULONG    flCtlData = FCF_TASKLIST | FCF_NOBYTEALIGN | FCF_ICON;

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

  PMRASSERT( WinRegisterClass( hab, "PM123", amp_dlg_proc, CS_SIZEREDRAW /* | CS_SYNCPAINT */, 0 ));
  hframe = WinCreateStdWindow( HWND_DESKTOP, 0, &flCtlData, "PM123",
                               AMP_FULLNAME, 0, NULLHANDLE, WIN_MAIN, &hplayer );
  PMASSERT( hframe != NULLHANDLE );

  DEBUGLOG(("main: window created\n"));

  do_warpsans( hframe  );
  do_warpsans( hplayer );

  // prepare pluginmanager
  plugman_init();
  // Keep track of plugin changes.
  delegate<void, const PLUGIN_EVENTARGS> plugin_delegate(plugin_event, &amp_plugin_eventhandler);
  PMRASSERT( WinPostMsg( hplayer, AMP_REFRESH_ACCEL, 0, 0 )); // load accelerators

  // start controller
  Ctrl::Init();
  amp_reset_status();

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
  if( !hhelp )
    amp_error( hplayer, "Error create help instance: %s", infname );
  else
    WinAssociateHelpInstance( hhelp, hframe );

  strcpy( bundle, startpath   );
  strcat( bundle, "pm123.lst" );

  // register control event handler
  delegate<void, const Ctrl::EventFlags> ctrl_delegate(Ctrl::ChangeEvent, &amp_control_event_handler);

  if( files == 1 && !is_dir( argv[argc - 1] )) {
    amp_load_playable( url123::normalizeURL(argv[argc - 1]), 0, 0 );
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
  { const url123& path = url123::normalizeURL(startpath);
    DefaultPL = (Playlist*)&*Playable::GetByURL(path + "PM123.LST");
    DefaultPM = (Playlist*)&*Playable::GetByURL(path + "PFREQ.LST");
    DefaultBM = (Playlist*)&*Playable::GetByURL(path + "BOOKMARK.LST");
    LoadMRU   = (Playlist*)&*Playable::GetByURL(path + "LOADMRU.LST");
    // The default playlist the bookmarks and the MRU list must be ready to use
    DefaultPL->EnsureInfo(Playable::IF_Other);
    DefaultBM->EnsureInfo(Playable::IF_Other);
    LoadMRU->EnsureInfo(Playable::IF_Other);
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

  amp_pipe_create();

  ///////////////////////////////////////////////////////////////////////////
  // Main loop
  ///////////////////////////////////////////////////////////////////////////
  while( WinGetMsg( hab, &qmsg, (HWND)0, 0, 0 ))
  { DEBUGLOG2(("main: after WinGetMsg - %p, %p\n", qmsg.hwnd, qmsg.msg));
    WinDispatchMsg( hab, &qmsg );
    DEBUGLOG2(("main: after WinDispatchMsg - %p, %p\n", qmsg.hwnd, qmsg.msg));
  }
  DEBUGLOG(("main: dispatcher ended\n"));

  amp_pipe_destroy();
  ///////////////////////////////////////////////////////////////////////////
  // Stop and save configuration
  ///////////////////////////////////////////////////////////////////////////
  Ctrl::Uninit();

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
  PlaylistManager::UnInit();
  PlaylistView::UnInit();

  LoadMRU   = NULL;
  DefaultBM = NULL;
  DefaultPM = NULL;
  DefaultPL = NULL;

  bmp_clean_skin();
  remove_all_plugins();
  dk_term();
  Playable::Uninit();
  plugman_uninit();

  WinDestroyWindow  ( hframe  );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
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
      char pipename[_MAX_PATH] = "\\PIPE\\PM123";
      o++;
      if( strncmp( argv[o], "\\\\", 2 ) == 0 )
      { // TODO: static buffer, pipe name
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
     // TODO: this only takes the last argument we hope is the filename
     // this should be changed.
     // TODO: pipename
    if( amp_pipe_open_and_write( "\\PIPE\\PM123", argv[argc-1], strlen( argv[argc-1]) + 1 )) {
      exit(0);
    }
  }

  srand((unsigned long)time( NULL ));

  // now init PM
  args args = { argc, argv, files };
  main2(&args);
  return 0;
}

#ifdef __DEBUG_ALLOC__
static struct MyTerm
{ ~MyTerm();
} MyTermInstance;

MyTerm::~MyTerm()
{ _dump_allocated( 0 );
}
#endif

