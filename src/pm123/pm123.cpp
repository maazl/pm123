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
#include "dialog.h"
#include "123_util.h"
#include "plugman.h"
#include "button95.h"
#include "playlistmanager.h"
#include "playlistview.h"
#include "infodialog.h"
#include "copyright.h"
#include "docking.h"
#include "iniman.h"
#include "properties.h"
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


// Cache lifetime of unused playable objects in seconds
// Objects are removed after [CLEANUP_INTERVALL, 2*CLEANUP_INTERVALL].
// However, since the romoved objects may hold references to other objects,
// only one generation is cleand up per time.
#define  CLEANUP_INTERVALL 10

#define  TID_UPDATE_TIMERS    ( TID_USERMAX - 1 )
#define  TID_UPDATE_PLAYER    ( TID_USERMAX - 2 )
#define  TID_ONTOP            ( TID_USERMAX - 3 )
#define  TID_CLEANUP          ( TID_USERMAX - 4 )

/* file dialog additional flags */
#define  FDU_DIR_ENABLE   0x0001
#define  FDU_RECURSEBTN   0x0002
#define  FDU_RECURSE_ON   0x0004
#define  FDU_RELATIVBTN   0x0008
#define  FDU_RELATIV_ON   0x0010


/* Contains startup path of the program without its name.  */
static xstring startpath_local;
const xstring& startpath = startpath_local;

// Default playlist, representing PM123.LST in the program folder.
static int_ptr<Playlist> DefaultPL;
// PlaylistManager window, representing PFREQ.LST in the program folder.
static int_ptr<Playlist> DefaultPM;
// Default instance of bookmark window, representing BOOKMARK.LST in the program folder.
static int_ptr<Playlist> DefaultBM;
// Most recent used entries in the load menu, representing LOADMRU.LST in the program folder.
static int_ptr<Playlist> LoadMRU;
// Most recent used entries in the load URL dialog, representing URLMRU.LST in the program folder.
static int_ptr<Playlist> UrlMRU;


static HAB   hab        = NULLHANDLE;
static HWND  hframe     = NULLHANDLE;
static HWND  hplayer    = NULLHANDLE;

static BOOL  is_have_focus    = FALSE;
static BOOL  is_volume_drag   = FALSE;
static BOOL  is_seeking       = FALSE;
static BOOL  is_slider_drag   = FALSE;
static bool  is_pl_slider     = false;
static BOOL  is_arg_shuffle   = FALSE;
static bool  is_accel_changed = false;
static bool  is_plugin_changed = true;

/* Current load wizzards */
static DECODER_WIZZARD_FUNC load_wizzards[20];

static volatile unsigned upd_options = 0;

/* location cache */
static SongIterator* CurrentIter = NULL; // current SongIterator
static Song*         CurrentSong = NULL; // Song from the current SongIterator
static Playable*     CurrentRoot = NULL; // Root from the current SongIterator, may be == CurrentSong
static bool          IsLocMsg    = false;// true if a MsgLocation to the controller is on the way
static unsigned      UpdAtLocMsg = 0;    // do these redraws (with amp_invalidate) when the next Location info arrives
static SongIterator* LocationIter;       // Two SongIterators. CurrentIter points to one of them. (Initialized lately)

static bool          Terminate   = false;// True when WM_QUIT arrived
static PlaylistMenu* MenuWorker  = NULL; // Instance of PlaylistMenu to handle events of the context menu.


/* Returns the handle of the player window. */
HWND
amp_player_window( void ) {
  return hframe;
}

/* Returns the anchor-block handle. */
HAB
amp_player_hab( void ) {
  return hab;
}

Playlist* amp_get_default_pl()
{ return DefaultPL;
}

Playlist* amp_get_default_bm()
{ return DefaultBM;
}

Playlist* amp_get_url_mru()
{ return UrlMRU;
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

void amp_control_event_callback(Ctrl::ControlCommand* cmd)
{ DEBUGLOG(("amp_control_event_callback(%p{%i, ...)\n", cmd, cmd->Cmd));
  WinPostMsg(hframe, AMP_CTRL_EVENT_CB, MPFROMP(cmd), 0);
}

/* Draws all player timers and the position slider. */
static void
amp_paint_timers( HPS hps )
{
  DEBUGLOG(("amp_paint_timers(%p) - %i %i\n", hps, cfg.mode, is_seeking));

  if (CurrentIter == NULL)
    return;

  const bool is_playlist = CurrentRoot && (CurrentRoot->GetFlags() & Playable::Enumerable);
  T_TIME total_song = CurrentSong ? CurrentSong->GetInfo().tech->songlength : -1;
  T_TIME total_time = is_playlist ? CurrentRoot->GetInfo().tech->songlength : -1;
  const SongIterator::Offsets& off = CurrentIter->GetOffset(false);

  T_TIME list_left = -1;
  T_TIME play_left = total_song;
  if (play_left > 0)
    play_left -= CurrentIter->GetLocation();
  if (total_time > 0)
    list_left = total_time - off.Offset - CurrentIter->GetLocation();

  double pos = -1.;
  if (!is_pl_slider)
  { if (total_song > 0)
      pos = CurrentIter->GetLocation()/total_song;
  } else switch (cfg.altnavig)
  {case CFG_ANAV_SONG:
    if (is_playlist)
    { int total_items = CurrentRoot->GetInfo().rpl->total_items;
      if (total_items == 1)
        pos = 0;
      else if (total_items > 1)
        pos = off.Index / (double)(total_items-1);
    }
    break;
   case CFG_ANAV_TIME:
    if (CurrentRoot->GetInfo().tech->songlength > 0)
    { pos = (off.Offset + CurrentIter->GetLocation()) / CurrentRoot->GetInfo().tech->songlength;
      break;
    } // else CFG_ANAV_SONGTIME
   case CFG_ANAV_SONGTIME:
    if (is_playlist)
    { int total_items = CurrentRoot->GetInfo().rpl->total_items;
      if (total_items == 1)
        pos = 0;
      else if (total_items > 1)
        pos = off.Index;
      else break;
      // Add current song time
      if (total_song > 0)
        pos += CurrentIter->GetLocation()/total_song;
      pos /= total_items;
    }
  }
  bmp_draw_slider( hps, pos, is_pl_slider );
  bmp_draw_timer ( hps, CurrentIter->GetLocation());

  bmp_draw_tiny_timer( hps, POS_TIME_LEFT, play_left );
  bmp_draw_tiny_timer( hps, POS_PL_LEFT,   list_left );

  int index = is_playlist ? off.Index+1 : 0;
  bmp_draw_plind( hps, index, index > 0 ? CurrentRoot->GetInfo().rpl->total_items : 0);
}

/* Draws all attributes of the currently loaded file. */
static void
amp_paint_fileinfo( HPS hps )
{
  DEBUGLOG(("amp_paint_fileinfo(%p)\n", hps));

  bmp_draw_plmode( hps, CurrentRoot != NULL, CurrentRoot ? CurrentRoot->GetFlags() : Playable::None );

  if (CurrentSong != NULL)
  { bmp_draw_rate    ( hps, CurrentSong->GetInfo().tech->bitrate );
    bmp_draw_channels( hps, CurrentSong->GetInfo().format->channels );
  } else
  { bmp_draw_rate    ( hps, -1 );
    bmp_draw_channels( hps, -1 );
  }
  bmp_draw_text( hps );
}

/* Marks the player window as needed of redrawing. */
void
amp_invalidate( unsigned options )
{ DEBUGLOG(("amp_invalidate(%x)\n", options));
  if( options & UPD_WINDOW ) {
    WinInvalidateRect( hplayer, NULL, 1 );
    options &= ~UPD_WINDOW;
  }
  InterlockedOr(upd_options, options & ~UPD_DELAYED);
  if(!(options & UPD_DELAYED))
    WinPostMsg(hplayer, AMP_PAINT, MPFROMLONG(options), 0);
}

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
  DEBUGLOG(("amp_display_filename() %p %u\n", CurrentSong, cfg.viewmode));
  if (!CurrentSong) {
    bmp_set_text( "No file loaded" );
    return;
  }

  xstring text;
  switch( cfg.viewmode )
  {
    case CFG_DISP_ID3TAG:
      text = amp_construct_tag_string(&CurrentSong->GetInfo());
      if (text.length())
        break;
      // if tag is empty - use filename instead of it.
    case CFG_DISP_FILENAME:
      text = CurrentSong->GetURL().getShortName();
      // In case of an invalid item display an error message.
      if (CurrentSong->GetStatus() == STA_Invalid)
        text = text + " - " + CurrentSong->GetInfo().tech->info;
      break;

    case CFG_DISP_FILEINFO:
      text = CurrentSong->GetInfo().tech->info;
      break;
  }
  bmp_set_text( text );
}

/* Ensure that a MsgLocation message is on processed by the controller */
static void amp_force_locmsg()
{ if (!IsLocMsg)
  { IsLocMsg = true;
    // Hack: the Location messages always writes to an iterator that is currently not in use by CurrentIter
    // to avoid threading problems.
    Ctrl::PostCommand(Ctrl::MkLocation(LocationIter + (CurrentIter == LocationIter)), &amp_control_event_callback);
  }
}

void amp_AddMRU(Playlist* list, size_t max, const PlayableSlice& ps)
{ DEBUGLOG(("amp_AddMRU(%p{%s}, %u, %s)\n", list, list->GetURL().cdata(), max, ps.GetPlayable()->GetURL().cdata()));
  Playable::Lock lock(*list);
  int_ptr<PlayableInstance> pi;
  // remove the desired item from the list and limit the list size
  while ((pi = list->GetNext(pi)) != NULL)
  { DEBUGLOG(("amp_AddMRU - %p{%s}\n", pi.get(), pi->GetPlayable()->GetURL().cdata()));
    if (max == 0 || pi->GetPlayable() == ps.GetPlayable()) // Instance equality of Playable is sufficient
      list->RemoveItem(pi);
     else
      --max;
  }
  // prepend list with new item
  list->InsertItem(ps, list->GetNext(NULL));
}

/* Loads a standalone file or CD track to player. */
void
amp_load_playable( const PlayableSlice& ps, int options )
{ DEBUGLOG(("amp_load_playable({%s,...}, %x)\n", ps.GetPlayable()->GetURL().cdata(), options));

  if (options & AMP_LOAD_APPEND)
  { ASSERT(Ctrl::GetRoot() != NULL);
    // multi mode
    if (Ctrl::GetRoot()->GetPlayable() != DefaultPL)
    { // we do not yet use the current playlist => use it
      // move current item to the list
      DefaultPL->Clear();
      DefaultPL->InsertItem(*Ctrl::GetRoot());
      // reset current to first item of the playlist
      Ctrl::PostCommand(Ctrl::MkLoad(DefaultPL->GetURL(), true), &amp_control_event_callback);
    }
    // append new item
    DefaultPL->InsertItem(ps);
    return;
  }

  Ctrl::ControlCommand* cmd = Ctrl::MkLoad(ps.GetPlayable()->GetURL(), false);
  Ctrl::ControlCommand* tail = cmd;
  if (ps.GetStop() != NULL)
    tail = tail->Link = Ctrl::MkStopAt(ps.GetStop()->Serialize(), 0, false);
  if (ps.GetStart() != NULL)
    tail = tail->Link = Ctrl::MkNavigate(ps.GetStart()->Serialize(), 0, false, false);
  if( !( options & AMP_LOAD_NOT_PLAY )) {
    if( cfg.playonload )
      // start playback immediately after loading has completed
      tail = tail->Link = Ctrl::MkPlayStop(Ctrl::Op_Set);
  }
  Ctrl::PostCommand(cmd, &amp_control_event_callback);

  if( !( options & AMP_LOAD_NOT_RECALL ))
    amp_AddMRU(LoadMRU, MAX_RECALL, ps);
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

    MenuWorker = new PlaylistMenu(parent, IDM_M_LAST, IDM_M_LAST_E);
    MenuWorker->AttachMenu(IDM_M_BOOKMARKS, DefaultBM, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Recursive|PlaylistMenu::Enumerate, 0);
    MenuWorker->AttachMenu(IDM_M_LOAD, LoadMRU, PlaylistMenu::Enumerate|PlaylistMenu::Separator, 0);
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
  mn_enable_item( menu, IDM_M_TAG,     CurrentSong != NULL && CurrentSong->GetInfo().meta_write );
  mn_enable_item( menu, IDM_M_CURRENT_SONG, CurrentSong != NULL );
  mn_enable_item( menu, IDM_M_CURRENT_PL, CurrentRoot != NULL && CurrentRoot->GetFlags() & Playable::Enumerable );
  mn_enable_item( menu, IDM_M_SMALL,   bmp_is_mode_supported( CFG_MODE_SMALL   ));
  mn_enable_item( menu, IDM_M_NORMAL,  bmp_is_mode_supported( CFG_MODE_REGULAR ));
  mn_enable_item( menu, IDM_M_TINY,    bmp_is_mode_supported( CFG_MODE_TINY    ));
  mn_enable_item( menu, IDM_M_FONT,    cfg.font_skinned );
  mn_enable_item( menu, IDM_M_FONT1,   bmp_is_font_supported( 0 ));
  mn_enable_item( menu, IDM_M_FONT2,   bmp_is_font_supported( 1 ));
  mn_enable_item( menu, IDM_M_ADDBOOK, CurrentSong != NULL );

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
  xstring URL;      // Object to drop
  xstring Start;    // Start Iterator
  xstring Stop;     // Stop iterator
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
          // TODO: slice!
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

static void DLLENTRY
amp_load_file_callback( void* param, const char* url )
{ DEBUGLOG(("amp_load_file_callback(%p{%u}, %s)\n", param, *(bool*)param, url));
  amp_load_playable( PlayableSlice(url), *(bool*)param ? 0 : AMP_LOAD_APPEND );
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
void amp_info_edit( HWND owner, Playable* song )
{ DEBUGLOG(("amp_info_edit(%x, %p)\n", owner, song));
  info_edit* iep = new info_edit(owner, song);
  // At least we need the decoder to process this request.
  if (song->EnsureInfoAsync(Playable::IF_Other))
  { // Information immediately available => post message
    iep->InfoDelegate.detach(); // do no longer wait for the event
    PMRASSERT(WinPostMsg(hframe, AMP_INFO_EDIT, iep, 0));
  }
}

/* set alternate navigation status to 'alt'. */
static void amp_set_alt_slider(bool alt)
{ if (is_pl_slider != alt);
  { is_pl_slider = alt;
    amp_invalidate(UPD_TIMERS);
  }
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
      // Return to basic slider behaviour when focus is lost.
      // Set the slider behaviour according to the Alt key when the focus is set.
      amp_set_alt_slider(is_have_focus && ((WinGetKeyState(HWND_DESKTOP, VK_ALT)|WinGetKeyState(HWND_DESKTOP, VK_ALTGRAF)) & 0x8000));
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
      PlayableSlice ps(pdi->URL, xstring(), pdi->Start.cdata(), pdi->Stop.cdata());
      ASSERT(ps.GetPlayable());
      amp_load_playable(ps, pdi->options);
      delete pdi;
      return 0;
    }

    case AMP_INFO_EDIT:
    { info_edit* iep = (info_edit*)PVOIDFROMMP(mp1);
      DEBUGLOG(("amp_dlg_proc: AMP_INFO_EDIT %p{%x, %p,}\n", iep, iep->Owner, iep->Song.get()));
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

      amp_invalidate(UPD_FILENAME);
      return 0;

    case AMP_CTRL_EVENT:
      { // Event from the controller
        Ctrl::EventFlags flags = (Ctrl::EventFlags)LONGFROMMP(mp1);
        DEBUGLOG(("amp_dlg_proc: AMP_CTRL_EVENT %x\n", flags));
        unsigned inval = 0;
        if (flags & Ctrl::EV_PlayStop)
        { if (Ctrl::IsPlaying())
          { WinSendDlgItemMsg(hplayer, BMP_PLAY,  WM_PRESS, 0, 0);
            amp_set_bubbletext(BMP_PLAY, "Stops playback");
          } else
          { WinSendDlgItemMsg(hplayer, BMP_PLAY,  WM_DEPRESS, 0, 0);
            amp_set_bubbletext(BMP_PLAY, "Starts playback");
            WinSetWindowText (hframe,  AMP_FULLNAME);
            amp_force_locmsg();
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
          inval |= UPD_TIMERS;
        }
        if (flags & (Ctrl::EV_Song|Ctrl::EV_SongPhys|Ctrl::EV_RootTech|Ctrl::EV_RootRpl))
          amp_force_locmsg();

        if (flags & Ctrl::EV_Volume)
          inval |= UPD_VOLUME;

        if (flags & Ctrl::EV_SongTech)
        { UpdAtLocMsg |= UPD_FILEINFO; // update later
          amp_force_locmsg();
        }
        if (flags & (Ctrl::EV_SongMeta|Ctrl::EV_SongStat))
        { int_ptr<Song> song = Ctrl::GetCurrentSong();
          if (song)
            WinSetWindowText(hframe, (song->GetURL().getDisplayName() + " - " AMP_FULLNAME).cdata());
          else
            WinSetWindowText (hframe,  AMP_FULLNAME);
          UpdAtLocMsg |= UPD_FILENAME|UPD_FILEINFO; // update screen later
          amp_force_locmsg();
        }

        if (inval)
          amp_invalidate(inval);
      }
      return 0;

    case AMP_CTRL_EVENT_CB:
      { Ctrl::ControlCommand* cmd = (Ctrl::ControlCommand*)PVOIDFROMMP(mp1);
        DEBUGLOG(("amp_dlg_proc: AMP_CTRL_EVENT_CB %p{%i, %x}\n", cmd, cmd->Cmd, cmd->Flags));
        switch (cmd->Cmd)
        {case Ctrl::Cmd_Load:
          // Successful? if not display error.
          if (cmd->Flags != Ctrl::RC_OK)
            pm123_display_error(xstring::sprintf("Error loading %s\n%s", cmd->StrArg.cdata(), Playable::GetByURL(cmd->StrArg)->GetInfo().tech->info));
          break;
         case Ctrl::Cmd_Skip:
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
         case Ctrl::Cmd_Volume:
          amp_invalidate(UPD_VOLUME);
          break;
         case Ctrl::Cmd_Jump:
          is_seeking = FALSE;
          delete (SongIterator*)cmd->PtrArg;
          amp_force_locmsg();
          break;
         case Ctrl::Cmd_Save:
          if (cmd->Flags != Ctrl::RC_OK)
            amp_player_error( "The current active decoder don't support saving of a stream.\n" );
          break;
         case Ctrl::Cmd_Location:
          IsLocMsg = false;
          if (is_seeking) // do not overwrite location while seeking
            break;
          // Fetch commonly used properties
          if (cmd->Flags == Ctrl::RC_OK)
          { CurrentIter = (SongIterator*)cmd->PtrArg;
            PlayableSlice* psi = CurrentIter->GetCurrent();
            CurrentSong = psi && psi->GetPlayable()->GetFlags() == Playable::None
              ? (Song*)psi->GetPlayable() : NULL;
            CurrentRoot = CurrentIter->GetRoot() ? CurrentIter->GetRoot()->GetPlayable() : NULL;
          } else
          { CurrentIter = NULL;
            CurrentSong = NULL;
            CurrentRoot = NULL;
          }
          // Redraws
          if (cfg.mode == CFG_MODE_REGULAR)
            UpdAtLocMsg |= UPD_TIMERS;
          amp_invalidate(UpdAtLocMsg);
          UpdAtLocMsg = 0;
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
    case DM_DROPHELP:
      amp_show_help( IDH_DRAG_AND_DROP );
      // Continue in default procedure to free ressources.
      break;

    case WM_TIMER:
      DEBUGLOG2(("amp_dlg_proc: WM_TIMER - %x\n", LONGFROMMP(mp1)));
      switch (LONGFROMMP(mp1))
      {case TID_ONTOP:
        WinSetWindowPos( hframe, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER );
        DEBUGLOG2(("amp_dlg_proc: WM_TIMER done\n"));
        return 0;

       case TID_UPDATE_PLAYER:
        { if( bmp_scroll_text()) {
            HPS hps = WinGetPS( hwnd );
            bmp_draw_text( hps );
            WinReleasePS( hps );
          }
          // initiate delayed paint
          if( upd_options )
            WinPostMsg( hwnd, AMP_PAINT, MPFROMLONG(-1), 0 );
        }
        DEBUGLOG2(("amp_dlg_proc: WM_TIMER done\n"));
        return 0;

       case TID_UPDATE_TIMERS:
        if (Ctrl::IsPlaying())
        // We must not mask this message request further, because the controller relies on that polling
        // to update it's internal status.
           amp_force_locmsg();
        DEBUGLOG2(("amp_dlg_proc: WM_TIMER done\n"));
        return 0;
       
       case TID_CLEANUP:
        Playable::Cleanup();
        return 0;
      }
      break;

    case WM_ERASEBACKGROUND:
      return 0;

    case WM_REALIZEPALETTE:
      vis_broadcast( msg, mp1, mp2 );
      break;

    case AMP_PAINT:
    { int mask = LONGFROMMP( mp1 );
      // is there anything to draw with HPS?
      if (mask & upd_options & (UPD_FILENAME|UPD_TIMERS|UPD_FILEINFO|UPD_VOLUME))
      { HPS hps  = WinGetPS( hwnd );
        if ((mask & UPD_FILENAME) && InterlockedBtr(upd_options, 4))
        { amp_display_filename();
          bmp_draw_text(hps);
        }
        if ((mask & UPD_TIMERS  ) && InterlockedBtr(upd_options, 0))
          amp_paint_timers(hps);
        if ((mask & UPD_FILEINFO) && InterlockedBtr(upd_options, 1))
          amp_paint_fileinfo(hps);
        if ((mask & UPD_VOLUME  ) && InterlockedBtr(upd_options, 2))
          bmp_draw_volume(hps, Ctrl::GetVolume());
        WinReleasePS( hps );
      }
      return 0;
    }

    case WM_PAINT:
    {
      HPS hps = WinBeginPaint( hwnd, NULLHANDLE, NULL );
      bmp_draw_background( hps, hwnd );
      if (!Terminate)
      { amp_paint_fileinfo ( hps );
        amp_paint_timers   ( hps );
        bmp_draw_volume    ( hps, Ctrl::GetVolume() );
      }
      bmp_draw_led       ( hps, is_have_focus  );
      
      WinEndPaint( hps );
      return 0;
    }

    case WM_COMMAND:
    { USHORT cmd = SHORT1FROMMP(mp1);
      DEBUGLOG(("amp_dlg_proc: WM_COMMAND(%u, %u, %u)\n", cmd, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      if( cmd > IDM_M_PLUG && cmd <= IDM_M_PLUG_E ) {
        cmd -= IDM_M_PLUG;
        // atomic plug-in request
        Module* plug;
        { const Module::IXAccess ix;
          if (cmd >= ix->size())
            return 0;
          plug = (*ix)[cmd];
        }
        plug->Config(hwnd);
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
          if (CurrentSong)
            amp_add_bookmark(hwnd, PlayableSlice(CurrentSong));
          break;

        case IDM_M_ADDBOOK_TIME:
          if (CurrentSong)
          { PlayableSlice ps(CurrentSong);
            // Use the time index from last_status here.
            if (CurrentIter->GetLocation() > 0)
            { SongIterator* iter = new SongIterator();
              iter->SetLocation(CurrentIter->GetLocation());
              ps.SetStart(iter);
            }
            amp_add_bookmark(hwnd, ps);
          }
          break;

        case IDM_M_ADDPLBOOK:
          if (CurrentRoot)
            amp_add_bookmark(hwnd, *CurrentIter->GetRoot());
          break;

        case IDM_M_ADDPLBOOK_TIME:
          if (CurrentRoot)
          { PlayableSlice ps(*CurrentIter->GetRoot());
            ps.SetStart(new SongIterator(*CurrentIter));
            amp_add_bookmark(hwnd, ps);
          }
          break;

        case IDM_M_EDITBOOK:
          PlaylistView::Get(DefaultBM, "Bookmarks")->SetVisible(true);
          break;

        case IDM_M_INFO:
          if (CurrentSong)
            InfoDialog::GetByKey(CurrentSong)->SetVisible(true);
          break;

        case IDM_M_PLINFO:
          if (CurrentRoot)
            InfoDialog::GetByKey(CurrentRoot)->SetVisible(true);
          break;

        case IDM_M_TAG:
          if (CurrentSong)
            amp_info_edit(hwnd, CurrentSong);
          break;

        case IDM_M_RELOAD:
          if (CurrentSong)
            CurrentSong->LoadInfoAsync(Playable::IF_All);
          break;

        case IDM_M_DETAILED:
          if (CurrentRoot && (CurrentRoot->GetFlags() & Playable::Enumerable))
            PlaylistView::Get(CurrentRoot)->SetVisible(true);
          break;

        case IDM_M_TREEVIEW:
          if (CurrentRoot && (CurrentRoot->GetFlags() & Playable::Enumerable))
            PlaylistManager::Get(CurrentRoot)->SetVisible(true);
          break;

        case IDM_M_ADDPMBOOK:
          if (CurrentRoot)
            DefaultPM->InsertItem(*CurrentIter->GetRoot());
          break;

        case IDM_M_PLRELOAD:
          if ( CurrentRoot && (CurrentRoot->GetFlags() & Playable::Enumerable)
            && ( !((PlayableCollection&)*CurrentRoot).IsModified()
              || amp_query(hwnd, "The current list is modified. Discard changes?") ))
            CurrentRoot->LoadInfoAsync(Playable::IF_All);
          break;

        case IDM_M_PLSAVE:
          if (CurrentRoot && (CurrentRoot->GetFlags() & Playable::Enumerable))
            amp_save_playlist(hwnd, (PlayableCollection&)*CurrentRoot);
          break;

        case IDM_M_SAVE:
          amp_save_stream(hwnd, !Ctrl::GetSavename());
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
          amp_invalidate(UPD_FILENAME);
          break;

        case IDM_M_FONT2:
          cfg.font = 1;
          amp_invalidate(UPD_FILENAME);
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
          amp_invalidate( UPD_FILENAME|UPD_DELAYED );

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
          (*load_wizzards[0])( hwnd, "Load%s", &amp_load_file_callback, &first );
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
    { // bookmark is selected
      amp_load_playable(*(PlayableSlice*)PVOIDFROMMP(mp1), AMP_LOAD_NOT_RECALL|AMP_LOAD_KEEP_PLAYLIST);
    }

    case WM_HELP:
     DEBUGLOG(("amp_dlg_proc: WM_HELP(%u, %u, %u)\n", SHORT1FROMMP(mp1), SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
     break;

    case WM_CREATE:
    {
      HPS hps;
      hframe = hwnd; /* we have to assign the window handle early, because WinCreateStdWindow does not have returned now. */

      hps = WinGetPS( hwnd );
      bmp_load_skin( cfg.defskin, hab, hwnd, hps );
      WinReleasePS( hps );

      if( cfg.floatontop ) {
        WinStartTimer( hab, hwnd, TID_ONTOP, 100 );
      }

      WinStartTimer( hab, hwnd, TID_UPDATE_TIMERS,  80 );
      WinStartTimer( hab, hwnd, TID_UPDATE_PLAYER,  50 );
      WinStartTimer( hab, hwnd, TID_CLEANUP, 1000*CLEANUP_INTERVALL );

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
      else if( CurrentRoot && bmp_pt_in_text( pos )) {
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
        Ctrl::PostCommand(Ctrl::MkVolume(bmp_calc_volume(pos), false), &amp_control_event_callback);
      }
      if( is_slider_drag )
        WinPostMsg(hwnd, AMP_SLIDERDRAG, mp1, MPFROMSHORT(FALSE));

      WinSetPointer( HWND_DESKTOP, WinQuerySysPointer( HWND_DESKTOP, SPTR_ARROW, FALSE ));
      return 0;

    case WM_BUTTON1MOTIONSTART:
    {
      POINTL pos;

      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      if( bmp_pt_in_volume( pos ))
      { is_volume_drag = TRUE;
        WinSetCapture( HWND_DESKTOP, hwnd );
      } else if( bmp_pt_in_slider( pos ) )
      { if (CurrentSong && ( is_pl_slider
          ? CurrentRoot->GetInfo().rpl->total_items > 0
          : CurrentSong->GetInfo().tech->songlength > 0 ))
        { is_slider_drag = TRUE;
          is_seeking     = TRUE;
          WinSetCapture( HWND_DESKTOP, hwnd );
        }
      }
      return 0;
    }

    case WM_BUTTON1MOTIONEND:
      if( is_volume_drag ) {
        is_volume_drag = FALSE;
        WinSetCapture( HWND_DESKTOP, NULLHANDLE );
      }
      if( is_slider_drag )
      {
        WinPostMsg(hwnd, AMP_SLIDERDRAG, mp1, MPFROMSHORT(TRUE));
        WinSetCapture( HWND_DESKTOP, NULLHANDLE );
      }
      return 0;

    case WM_BUTTON2MOTIONSTART:
      WinSendMsg( hframe, WM_TRACKFRAME, MPFROMSHORT( TF_MOVE | TF_STANDARD ), 0 );
      WinQueryWindowPos( hframe, &cfg.main );
      return 0;

    case WM_CHAR:
    { // Force heap dumps by the D key
      USHORT fsflags = SHORT1FROMMP(mp1);
      DEBUGLOG(("amp_dlg_proc: WM_CHAR: %x, %u, %u, %x, %x\n", fsflags,
        SHORT2FROMMP(mp1)&0xff, SHORT2FROMMP(mp1)>>8, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      if (fsflags & KC_VIRTUALKEY)
      { switch (SHORT2FROMMP(mp2))
        {case VK_ESC:
          if(!(fsflags & KC_KEYUP))
          { is_slider_drag = FALSE;
            is_seeking     = FALSE;
          }
          break;
        }
      }
      #ifdef __DEBUG_ALLOC__
      if ((fsflags & (KC_CHAR|KC_ALT|KC_CTRL)) == (KC_CHAR) && toupper(SHORT1FROMMP(mp2)) == 'D')
      { if (fsflags & KC_SHIFT)
          _dump_allocated_delta(0);
        else
          _dump_allocated(0);
      }
      #endif
      break;
    }
    
    case WM_TRANSLATEACCEL:
    { /* WM_CHAR with VK_ALT and KC_KEYUP does not survive this message, so we catch it before. */
      QMSG* pqmsg = (QMSG*)mp1;
      DEBUGLOG(("amp_dlg_proc: WM_TRANSLATEACCEL: %x, %x, %x\n", pqmsg->msg, pqmsg->mp1, pqmsg->mp2));
      if (pqmsg->msg == WM_CHAR)
      { USHORT fsflags = SHORT1FROMMP(pqmsg->mp1);
        if (fsflags & KC_VIRTUALKEY)
        { switch (SHORT2FROMMP(pqmsg->mp2))
          {case VK_ALT:
           case VK_ALTGRAF:
           //case VK_CTRL:
            amp_set_alt_slider(!(fsflags & KC_KEYUP));
            break;
          }
        }
      }
      break;
    }

    case AMP_SLIDERDRAG:
    if (is_slider_drag)
    { // get position
      POINTL pos;
      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);
      double relpos = bmp_calc_time(pos);
      // adjust CurrentIter from pos
      if (!is_pl_slider)
      { // navigation within the current song
        if (CurrentSong->GetInfo().tech->songlength <= 0)
          return 0;
        CurrentIter->SetLocation(relpos * CurrentSong->GetInfo().tech->songlength);
      } else switch (cfg.altnavig)
      {case CFG_ANAV_TIME:
        // Navigate only at the time scale
        if (CurrentRoot->GetInfo().tech->songlength > 0)
        { CurrentIter->Reset();
          bool r = CurrentIter->SetTimeOffset(relpos * CurrentRoot->GetInfo().tech->songlength);
          DEBUGLOG(("amp_dlg_proc: AMP_SLIDERDRAG: CFG_ANAV_TIME: %u\n", r));
          break;
        }
        // else if no total time is availabe use song time navigation 
       case CFG_ANAV_SONG:
       case CFG_ANAV_SONGTIME:
        // navigate at song and optional time scale
        { const int num_items = CurrentRoot->GetInfo().rpl->total_items;
          if (num_items <= 0)
            return 0;
          relpos *= num_items;
          relpos += 1.;
          if (relpos > num_items)
            relpos = num_items;
          CurrentIter->Reset();
          bool r = CurrentIter->NavigateFlat(xstring(), (int)floor(relpos));
          DEBUGLOG(("amp_dlg_proc: AMP_SLIDERDRAG: CFG_ANAV_SONG: %f, %u\n", relpos, r));
          // update CurrentSong
          PlayableSlice* psi = CurrentIter->GetCurrent();
          CurrentSong = psi && psi->GetPlayable()->GetFlags() == Playable::None
            ? (Song*)psi->GetPlayable() : NULL;
          // navigate within the song
          if (cfg.altnavig != CFG_ANAV_SONG && CurrentSong && CurrentSong->GetInfo().tech->songlength > 0)
          { relpos -= floor(relpos);
            CurrentIter->SetLocation(relpos * CurrentSong->GetInfo().tech->songlength);
          }
        }
      }
      // new position set
      if (SHORT1FROMMP(mp2))
      { Ctrl::PostCommand(Ctrl::MkJump(new SongIterator(*CurrentIter)), &amp_control_event_callback);
        is_slider_drag = FALSE;
      } else
        amp_invalidate(UPD_TIMERS|(UPD_FILEINFO|UPD_FILENAME)*is_pl_slider);
    }
    return 0;
    
    /*case WM_SYSCOMMAND:
      DEBUGLOG(("amp_dlg_proc: WM_SYSCVOMMAND: %u, %u, %u\n", SHORT1FROMMP(mp1), SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      break;*/
  }

  DEBUGLOG2(("amp_dlg_proc: before WinDefWindowProc\n"));
  return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}


static void amp_plugin_eventhandler(void*, const Plugin::EventArgs& args)
{ DEBUGLOG(("amp_plugin_eventhandler(, {&%p{%s}, %i})\n",
    &args.Plug, args.Plug.GetModuleName().cdata(), args.Operation));
  switch (args.Operation)
  {case Plugin::EventArgs::Enable:
   case Plugin::EventArgs::Disable:
   case Plugin::EventArgs::Unload:
    if (args.Plug.GetType() == PLUGIN_DECODER)
      WinPostMsg(hframe, AMP_REFRESH_ACCEL, 0, 0);
    is_plugin_changed = true;
   default:; // avoid warnings
  }
}

static void amp_save_list(Playlist& pl)
{ if (!pl.CheckInfo(Playable::IF_Other))
    pl.Save(PlayableCollection::SaveRelativePath);
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
  QMSG   qmsg;

  ///////////////////////////////////////////////////////////////////////////
  // Initialization of infrastructure
  ///////////////////////////////////////////////////////////////////////////

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

  do_warpsans( hplayer );

  // prepare pluginmanager
  plugman_init();
  // Keep track of plugin changes.
  delegate<void, const Plugin::EventArgs> plugin_delegate(Plugin::ChangeEvent, &amp_plugin_eventhandler);
  PMRASSERT( WinPostMsg( hframe, AMP_REFRESH_ACCEL, 0, 0 )); // load accelerators

  Playable::Init();
  
  // register control event handler
  delegate<void, const Ctrl::EventFlags> ctrl_delegate(Ctrl::ChangeEvent, &amp_control_event_handler);
  // start controller
  Ctrl::Init();
  if ( is_arg_shuffle )
    Ctrl::PostCommand(Ctrl::MkShuffle(Ctrl::Op_Set));

  // Initialize the two songiterators for status updates.
  // They must not be static to avoid initialization sequence problems.
  LocationIter = new SongIterator[2];

  dk_init();
  dk_add_window( hframe, DK_IS_MASTER );

  DEBUGLOG(("main: create subwindows\n"));

  dlg_init();

  if( files == 1 && !is_dir( argv[argc - 1] )) {
    amp_load_playable(PlayableSlice(url123::normalizeURL(argv[argc - 1])), 0 );
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
  }

  WinSetWindowPos( hframe, HWND_TOP,
                   cfg.main.x, cfg.main.y, 0, 0, SWP_ACTIVATE | SWP_MOVE | SWP_SHOW );

  // Init some other stuff
  PlaylistView::Init();
  PlaylistManager::Init();

  // Init default lists
  { const url123& path = url123::normalizeURL(startpath);
    DefaultPL = &(Playlist&)*Playable::GetByURL(path + "PM123.LST");
    DefaultPM = &(Playlist&)*Playable::GetByURL(path + "PFREQ.LST");
    DefaultBM = &(Playlist&)*Playable::GetByURL(path + "BOOKMARK.LST");
    LoadMRU   = &(Playlist&)*Playable::GetByURL(path + "LOADMRU.LST");
    UrlMRU    = &(Playlist&)*Playable::GetByURL(path + "URLMRU.LST");
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
  Terminate = true;

  amp_pipe_destroy();

  ///////////////////////////////////////////////////////////////////////////
  // Stop and save configuration
  ///////////////////////////////////////////////////////////////////////////
  ctrl_delegate.detach();
  plugin_delegate.detach();
  delete MenuWorker;
  // Eat user messages from the queue
  while (WinPeekMsg( hab, &qmsg, hframe, WM_USER, 0xbfff, PM_REMOVE));
  // TODO: save volume
  Ctrl::Uninit();
  delete[] LocationIter;

  save_ini();
  amp_save_list(*DefaultBM);
  amp_save_list(*DefaultPM);
  amp_save_list(*DefaultPL);
  amp_save_list(*LoadMRU);
  amp_save_list(*UrlMRU);

  // deinitialize all visual plug-ins
  vis_deinit_all( TRUE );
  vis_deinit_all( FALSE );

  dlg_uninit();

  ///////////////////////////////////////////////////////////////////////////
  // Uninitialize infrastructure
  ///////////////////////////////////////////////////////////////////////////
  PlaylistManager::UnInit();
  PlaylistView::UnInit();

  UrlMRU    = NULL;
  LoadMRU   = NULL;
  DefaultBM = NULL;
  DefaultPM = NULL;
  DefaultPL = NULL;

  dk_term();
  Playable::Uninit();
  Playable::Clear();
  plugman_uninit();

  WinDestroyWindow(hframe);
  bmp_clean_skin();
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}

int
main( int argc, char *argv[] )
{
  int    i, o, files = 0;
  char   exename[_MAX_PATH];
  char   buffer[1024];

  // used for debug printf()s
  setvbuf( stderr, NULL, _IONBF, 0 );

  for( i = 1; i < argc; i++ ) {
    if( argv[i][0] != '/' && argv[i][0] != '-' ) {
      files++;
    }
  }

  getExeName( exename, sizeof( exename ));
  sdrivedir ( buffer, exename, sizeof buffer);
  startpath_local = buffer;

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
      strcpy( buffer, "*" );
      for( i = o; i < argc; i++ )
      {
        strcat( buffer, argv[i] );
        strcat( buffer, " " );
      }

      amp_pipe_open_and_write( pipename, buffer, strlen( buffer ) + 1 );
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

