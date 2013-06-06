/*
 * Copyright 2007-2011 M.Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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


#include "gui.h"
#include "../core/songiterator.h"
#include "../core/job.h"
#include "../engine/visual.h"
#include "../engine/decoder.h"
#include "../engine/filter.h"
#include "../engine/output.h"
#include "../engine/loadhelper.h"
#include "dialog.h"
#include "filedlg.h"
#include "properties.h"
#include "../configuration.h"
#include "../pm123.h"
#include "pm123.rc.h"
#include "../123_util.h"
#include "skin.h"
#include "docking.h"
#include "infodialog.h"
#include "playlistview.h"
#include "playlistmanager.h"
#include "inspector.h"
#include "postmsginfo.h"
#include "playlistmenu.h"
#include "button95.h"
#include "../copyright.h"

#include <utilfct.h>
#include <fileutil.h>
#include <cpp/pmutils.h>
#include <cpp/showaccel.h>

#include <sys/stat.h>
#include <math.h>
#include <stdio.h>


#if defined(DEBUG_MEM) && defined(DEBUG_LOG)
#ifdef __GNUC__
static size_t heap_sum[2];
static unsigned heap_count[2];
static int heap_walker(const void* ptr, size_t size, int flag, int status, const char* file, unsigned line)
{
  ++heap_count[flag&1];
  heap_sum[flag&1] += size;
  debuglog("HEAP %p(%u) %i %i - %s %u\n", ptr, size, flag, status, file, line);
  return 0;
}
#endif
#endif

static void vis_InitAll(HWND owner)
{ int_ptr<PluginList> visuals(Plugin::GetPluginList(PLUGIN_VISUAL));
  foreach (const int_ptr<Plugin>,*, vpp, *visuals)
  { Visual& vis = (Visual&)**vpp;
    if (vis.GetEnabled() && !vis.IsInitialized())
      vis.InitPlugin(owner);
  }
}

static void vis_UninitAll()
{ int_ptr<PluginList> visuals(Plugin::GetPluginList(PLUGIN_VISUAL));
  foreach (const int_ptr<Plugin>,*, vpp, *visuals)
  { Visual& vis = (Visual&)**vpp;
    if (vis.IsInitialized())
      vis.UninitPlugin();
  }
}

static void append_restricted(xstringbuilder& target, const xstring& str, unsigned maxlen)
{
  if (maxlen && str.length() > maxlen)
  { target.append(str, maxlen);
    target.append("...");
  } else
    target += str;
}


/****************************************************************************
*
*  Private Implementation of class GUI
*
****************************************************************************/
class GUIImp : private GUI, private DialogBase
{private:
  enum UpdateFlags
  { UPD_NONE             = 0
  , UPD_TIMERS           = 0x0001         // Current time index, remaining time and slider
  , UPD_PLMODE           = 0x0004         // Playlist mode icons
  , UPD_PLINDEX          = 0x0008         // Playlist index, playlist totals
  , UPD_RATE             = 0x0010         // Bit rate
  , UPD_CHANNELS         = 0x0020         // Number of channels
  , UPD_VOLUME           = 0x0040         // Volume slider
  , UPD_TEXT             = 0x0080         // Text in scroller
  , UPD_ALL              = 0x00ff         // All the above
  , UPD_BACKGROUND       = 0x0100         // Draw player background
  , UPD_LED              = 0x0200         // Draw player activity LED
  };
  CLASSFLAGSATTRIBUTE(UpdateFlags);

  struct HandleDrop : public HandleDragTransfers
  { LoadHelper Loader;
    HandleDrop(DRAGINFO* di, HWND hwnd);
    virtual ~HandleDrop();
  };

  /// Cache lifetime of unused playable objects in seconds
  /// Objects are removed after [CLEANUP_INTERVALL, 2*CLEANUP_INTERVALL].
  /// However, since the removed objects may hold references to other objects,
  /// only one generation is cleaned up per time.
  static const int CLEANUP_INTERVAL = 10;

 private: // Working set
  static bool              Terminate;     // True when WM_QUIT arrived
  static DECODER_WIZARD_FUNC LoadWizards[16]; // Current load wizards
  static HWND              ContextMenu;   // Window handle of the PM123 main context menu
  static PlaylistMenu*     MenuWorker;    // Instance of PlaylistMenu to handle events of the context menu.
  static sco_arr<xstring>  PluginMenuContent; // Plug-ins in the context menu

  static bool              IsLocMsg;      // true if a MsgLocation to the controller is on the way
  static UpdateFlags       UpdFlags;      // Pending window updates.
  static UpdateFlags       UpdAtLocMsg;   // Update this fields at the next location message.
  static bool              IsHaveFocus;   // PM123 main window currently has the focus.
  static bool              IsVolumeDrag;  // We currently drag the volume bar.
  static bool              IsSliderDrag;  // We currently drag the navigation slider.
  static bool              IsAltSlider;   // Alternate Slider currently active
  static bool              IsAccelChanged;// Flag whether the accelerators have changed since the last context menu invocation.
  static bool              IsPluginChanged;// Flag whether the plug-in list has changed since the last context menu invocation.

  static int_ptr<SongIterator> CurrentIter;

  static delegate<const void, const PluginEventArgs>    PluginDeleg;
  static delegate<const void, const Ctrl::EventFlags>   ControllerDeleg;
  static delegate<const void, const PlayableChangeArgs> RootDeleg;
  static delegate<const void, const PlayableChangeArgs> CurrentDeleg;
  static delegate<const void, const CfgChangeArgs>      ConfigDeleg;

 private:
  GUIImp(); // static class
  // Static members must not use EXPENTRY linkage with IBM VACPP.
  friend MRESULT EXPENTRY GUI_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  static MRESULT   GUIDlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);

  static void      Invalidate(UpdateFlags what);
  static void      SetAltSlider(bool alt);
  /// WMP_SLIDERDRAG processing.
  static void      SliderDrag(LONG x, LONG y, JobSet& job);

  /// Ensure that a MsgLocation message is processed by the controller
  static void      ForceLocationMsg();
  static void      ControllerEventCB(Ctrl::ControlCommand* cmd);
  static void      ControllerNotification(const void*, const Ctrl::EventFlags& flags);
  static void      PlayableNotification(const void*, const PlayableChangeArgs& args);
  static void      PluginNotification(const void*, const PluginEventArgs& args);
  static void      ConfigNotification(const void*, const CfgChangeArgs& args);

  static void      PostCtrlCommand(Ctrl::ControlCommand* cmd) { Ctrl::PostCommand(cmd, &GUIImp::ControllerEventCB); }

  //static Playable& EnsurePlaylist(Playable& list);

  static void      SaveStream(HWND hwnd, BOOL enable);
  friend BOOL EXPENTRY GUI_HelpHook(HAB hab, ULONG usMode, ULONG idTopic, ULONG idSubTopic, PRECTL prcPosition);

  static void      LoadAccel();           ///< (Re-)Loads the accelerator table and modifies it by the plug-ins.
  static void      LoadPluginMenu(HWND hmenu); ///< Refresh plug-in menu in the main pop-up menu.
  static void      ShowContextMenu();     ///< View to context menu
  static bool      ShowHideInfoDlg(APlayable& p, AInfoDialog::PageNo page, DialogAction action);
  static bool      ShowHidePlaylist(PlaylistBase* plp, DialogAction action);
  /// Refresh current playing time, remaining time and slider.
  /// @param index Remaining number of songs to play or -1 if not available..
  /// @param offset Remaining playlist time or < 0 if not available.
  static void      RefreshTimers(HPS hps, int rem_songs, PM123_TIME rem_time);
  static void      PrepareText();         ///< Constructs a information text for currently loaded file and selects it for displaying.
  /// Execute screen updates from \c WMP_PAINT.
  static void      Paint(HPS hps, UpdateFlags mask);

  friend void DLLENTRY GUI_LoadFileCallback(void* param, const char* url, const INFO_BUNDLE* info, int cached, int override);

  static void      AutoSave();
  static void      AutoSave(Playable& list);

  // Drag & drop
  static MRESULT   DragOver(DRAGINFO* pdinfo);
  static void      DragDrop(DRAGINFO* pdinfo);
  static void      DropRenderComplete(DRAGTRANSFER* pdtrans, USHORT rc);

 public: // Initialization interface
  static void      Init();
  static void      Uninit();

  static void      Load(LoadHelper& lhp) { Ctrl::ControlCommand* cmd = lhp.ToCommand(); if (cmd) GUIImp::PostCtrlCommand(cmd); }
};


bool                GUIImp::Terminate = false;
DECODER_WIZARD_FUNC GUIImp::LoadWizards[16];
HWND                GUIImp::ContextMenu = NULLHANDLE;
PlaylistMenu*       GUIImp::MenuWorker  = NULL;
sco_arr<xstring>    GUIImp::PluginMenuContent;

bool                GUIImp::IsLocMsg        = false;
GUIImp::UpdateFlags GUIImp::UpdFlags        = UPD_NONE;
GUIImp::UpdateFlags GUIImp::UpdAtLocMsg     = UPD_NONE;
bool                GUIImp::IsHaveFocus     = false;
bool                GUIImp::IsVolumeDrag    = false;
bool                GUIImp::IsSliderDrag    = false;
bool                GUIImp::IsAltSlider     = false;
bool                GUIImp::IsAccelChanged  = false;
bool                GUIImp::IsPluginChanged = false;

int_ptr<SongIterator> GUIImp::CurrentIter;

delegate<const void, const PluginEventArgs>    GUIImp::PluginDeleg    (&GUIImp::PluginNotification);
delegate<const void, const Ctrl::EventFlags>   GUIImp::ControllerDeleg(&GUIImp::ControllerNotification);
delegate<const void, const PlayableChangeArgs> GUIImp::RootDeleg      (&GUIImp::PlayableNotification);
delegate<const void, const PlayableChangeArgs> GUIImp::CurrentDeleg   (&GUIImp::PlayableNotification);
delegate<const void, const CfgChangeArgs>      GUIImp::ConfigDeleg    (&GUIImp::ConfigNotification);


/// Helper that posts a message when alternate navigation is delayed
/// because of missing dependencies for recursive playlist information.
PostMsgDIWorker DelayedAltSliderDragWorker;


void DLLENTRY GUI_LoadFileCallback(void* param, const char* url, const INFO_BUNDLE* info, int cached, int override)
{ DEBUGLOG(("GUI_LoadFileCallback(%p, %s)\n", param, url));
  ((LoadHelper*)param)->AddItem(*Playable::GetByURL(url123(url)));
}

MRESULT EXPENTRY GUI_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ // Adjust calling convention
  if (msg == WM_CREATE)
    GUI::HPlayer = hwnd; // we have to assign the window handle early, because WinCreateStdWindow does not have returned now.
  return GUIImp::GUIDlgProc(msg, mp1, mp2);
}

MRESULT GUIImp::GUIDlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("GUIImp::DlgProc(%p, %p, %p)\n", msg, mp1, mp2));
  switch (msg)
  { case WM_CREATE:
    { DEBUGLOG(("GUIImp::DlgProc: WM_CREATE\n"));

      WinPostMsg(HPlayer, WMP_RELOADSKIN, 0, 0);

      HAB hab = amp_player_hab;
      if (Cfg::Get().floatontop)
        WinStartTimer(hab, HPlayer, TID_ONTOP, 100 );

      WinStartTimer(hab, HPlayer, TID_UPDATE_TIMERS, 90);
      WinStartTimer(hab, HPlayer, TID_UPDATE_PLAYER, 60);
      WinStartTimer(hab, HPlayer, TID_CLEANUP, 1000*CLEANUP_INTERVAL);

      // fetch some initial states
      WinPostMsg(HPlayer, WMP_CTRL_EVENT, MPFROMLONG(Ctrl::EV_Repeat|Ctrl::EV_Shuffle|Ctrl::EV_Song), 0 );
      break;
    }

   case WM_FOCUSCHANGE:
    { PresentationSpace hps(HPlayer);
      IsHaveFocus = SHORT1FROMMP(mp2);
      bmp_draw_led(hps, IsHaveFocus);
    }
    // Return to basic slider behavior when focus is lost.
    // Set the slider behavior according to the Alt key when the focus is set.
    SetAltSlider(IsHaveFocus && ((WinGetKeyState(HWND_DESKTOP, VK_ALT)|WinGetKeyState(HWND_DESKTOP, VK_ALTGRAF)) & 0x8000));
    break;

   case 0x041e: // WM_MOUSEENTER
   case 0x041f: // WM_MOUSELEAVE
    if (!IsHaveFocus)
    { PresentationSpace hps(HPlayer);
      bmp_draw_led(hps, !(msg&1));
    }
    return 0;

   case WMP_NAVIGATE:
    { Location& target = *(Location*)PVOIDFROMMP(mp1);
      DEBUGLOG(("GUIImp::DlgProc: WMP_NAVIGATE {%s}\n", target.Serialize(false).cdata()));
      ASSERT(target.GetRoot());
      if (!target.GetCurrent()) // Do not enter playlists if not necessary.
        target.NavigateUp();
      const volatile amp_cfg& cfg = Cfg::Get();
      if (cfg.itemaction == CFG_ACTION_NAVTO)
        GUIImp::PostCtrlCommand(Ctrl::MkJump(new Location(target), true));
      else
      { LoadHelper lhp(cfg.playonload*LoadHelper::LoadPlay | (cfg.itemaction == CFG_ACTION_QUEUE)*LoadHelper::LoadAppend);
        lhp.AddItem(*target.GetCurrent());
        Load(lhp);
      }
      return 0;
    }

   case WMP_SHOW_DIALOG:
    { int_ptr<APlayable> pp;
      pp.fromCptr((Playable*)mp1);
      DialogType type = (DialogType)SHORT1FROMMP(mp2);
      DialogAction action = (DialogAction)SHORT2FROMMP(mp2);
      DEBUGLOG(("GUIImp::DlgProc: WMP_SHOW_DIALOG %p, %u, %u\n", pp.get(), type, action));
      switch (type)
      {case DLT_INFOEDIT:
        if (action == DialogBase::DLA_SHOW)
          AutoPostMsgWorker::Start(*(Playable*)mp1, IF_Decoder|IF_Meta, PRI_Normal, HPlayer, WMP_EDIT_META, MPFROMP(pp.toCptr()), 0);
        break;
       case DLT_METAINFO:
        return MRFROMLONG(ShowHideInfoDlg(*pp, AInfoDialog::Page_MetaInfo, action));
       case DLT_TECHINFO:
        return MRFROMLONG(ShowHideInfoDlg(*pp, AInfoDialog::Page_TechInfo, action));
       case DLT_PLAYLIST:
        return MRFROMLONG(ShowHidePlaylist(action ? PlaylistView::GetByKey(pp->GetPlayable()) : PlaylistView::FindByKey(pp->GetPlayable()), action));
       case DLT_PLAYLISTTREE:
        return MRFROMLONG(ShowHidePlaylist(action ? PlaylistManager::GetByKey(pp->GetPlayable()) : PlaylistManager::FindByKey(pp->GetPlayable()), action));
      }
      return false;
    }

   case WMP_EDIT_META:
    { int_ptr<Playable> pp;
      pp.fromCptr((Playable*)PVOIDFROMMP(mp1));
      DEBUGLOG(("GUIImp::DlgProc: WMP_EDIT_META %p\n", pp.get()));
      ULONG rc;
      try
      { int_ptr<Decoder> dp = Decoder::GetDecoder(xstring(pp->GetInfo().tech->decoder));
        if (!dp)
          rc = 400;
        else
          rc = dp->EditMeta(HFrame, pp->URL);
      } catch (const ModuleException& ex)
      { amp_messagef(HFrame, MSG_ERROR, "Cannot edit tag of file:\n%s", ex.GetErrorText().cdata());
        break;
      }
      DEBUGLOG(("GUIImp::DlgProc: WMP_EDIT_META rc = %u\n", rc));
      switch (rc)
      { default:
          amp_messagef(HFrame, MSG_ERROR, "Cannot edit tag of file:\n%s", pp->URL.cdata());
          break;
        case 0:   // tag changed
          pp->RequestInfo(IF_Meta, PRI_Normal);
          // Refresh will come automatically
        case 300: // tag unchanged
          break;
        case 400: // decoder does not provide decoder_editmeta => use info dialog instead
          AInfoDialog::GetByKey(*pp)->ShowPage(AInfoDialog::Page_MetaInfo);
          break;
        case 500:
          amp_messagef(HFrame, MSG_ERROR, "Unable write tag to file:\n%s\n%s", pp->URL.cdata(), strerror(errno));
          break;
      }
      break;
    }

   case WMP_SHOW_CONFIG:
    { int_ptr<Module> mp;
      mp.fromCptr((Module*)PVOIDFROMMP(mp1));
      DialogAction action = (DialogAction)SHORT1FROMMP(mp2);
      DEBUGLOG(("GUIImp::DlgProc: WMP_SHOW_CONFIG %p, %u\n", mp.get(), action));
      bool rc;
      if (mp)
      { // Plug-in configuration
        rc = mp->IsConfig();
        switch (action)
        {case DialogBase::DLA_SHOW:
          if (!rc)
            PMRASSERT(WinPostMsg(HPlayer, WMP_DO_CONFIG, MPFROMP(mp.toCptr()), 0));
          break;
         case DLA_CLOSE:
          if (rc)
            mp->Config(NULLHANDLE);
         default:; // avoid warning
        }
      } else
      { // PM123 configuration
        rc = APropertyDialog::IsVisible();
        switch (action)
        {case DLA_SHOW:
          if (rc)
            APropertyDialog::Show();
          else
            PMRASSERT(WinPostMsg(HPlayer, WMP_DO_CONFIG, MPFROMP(NULL), 0));
          break;
         case DLA_CLOSE:
          if (rc)
            APropertyDialog::Close();
         default:; // avoid warning
        }
      }
      return MRFROMLONG(rc);
    }

   case WMP_DO_CONFIG:
   { int_ptr<Module> mp;
     mp.fromCptr((Module*)PVOIDFROMMP(mp1));
     DEBUGLOG(("GUIImp::DlgProc: WMP_DO_CONFIG %p\n", mp.get()));
     if (mp)
       mp->Config(HFrame);
     else
       APropertyDialog::Do(HFrame);
     return 0;
   }

   case WMP_DISPLAY_MESSAGE:
    { xstring text;
      text.fromCstr((const char*)mp1);
      amp_message(HFrame, (MESSAGE_TYPE)SHORT1FROMMP(mp2), text.cdata());
      return 0;
    }
   case WMP_DISPLAY_MODE:
    { Cfg::ChangeAccess cfg;
      if (cfg.viewmode == CFG_DISP_FILEINFO)
        cfg.viewmode = CFG_DISP_FILENAME;
      else
        cfg.viewmode = (cfg_disp)(cfg.viewmode+1);
      return 0;
    }
   case WMP_RELOADSKIN:
    { bmp_load_skin(xstring(Cfg::Get().defskin), HPlayer);
      WinInvalidateRect(HPlayer, NULL, 1);
      return 0;
    }

   case WMP_LOAD_VISUAL:
    { int_ptr<Visual> vp;
      vp.fromCptr((Visual*)PVOIDFROMMR(mp1));
      if (vp->GetEnabled() && !vp->IsInitialized())
        vp->InitPlugin(HPlayer);
      return 0;
    }

   case WMP_CTRL_EVENT_CB:
    { Ctrl::ControlCommand* cmd = (Ctrl::ControlCommand*)PVOIDFROMMP(mp1);
      DEBUGLOG(("GUIImp::DlgProc: WMP_CTRL_EVENT_CB %p{%i, %x}\n", cmd, cmd->Cmd, cmd->Flags));
      switch (cmd->Cmd)
      {case Ctrl::Cmd_Load:
        // Successful? if not display error.
        if (cmd->Flags != Ctrl::RC_OK)
          amp_messagef(HFrame, MSG_ERROR, "Error loading %s\n%s", cmd->StrArg.cdata(), xstring(Playable::GetByURL(cmd->StrArg)->GetInfo().tech->info).cdata());
        break;
       case Ctrl::Cmd_Skip:
        WinSendDlgItemMsg(HPlayer, BMP_PREV, WM_DEPRESS, 0, 0);
        WinSendDlgItemMsg(HPlayer, BMP_NEXT, WM_DEPRESS, 0, 0);
        break;
       case Ctrl::Cmd_PlayStop:
        if (cmd->Flags != Ctrl::RC_OK)
          // release button immediately
          WinSendDlgItemMsg(HPlayer, BMP_PLAY, WM_DEPRESS, 0, 0);
        break;
       case Ctrl::Cmd_Pause:
        if (cmd->Flags != Ctrl::RC_OK)
          // release button immediately
          WinSendDlgItemMsg(HPlayer, BMP_PAUSE, WM_DEPRESS, 0, 0);
        break;
       case Ctrl::Cmd_Scan:
        if (cmd->Flags != Ctrl::RC_OK)
        { // release buttons immediately
          WinSendDlgItemMsg(HPlayer, BMP_FWD, WM_DEPRESS, 0, 0);
          WinSendDlgItemMsg(HPlayer, BMP_REW, WM_DEPRESS, 0, 0);
        }
        break;
       case Ctrl::Cmd_Volume:
        Invalidate(UPD_VOLUME);
        break;
       case Ctrl::Cmd_Jump:
        delete (Location*)cmd->PtrArg;
        break;
       case Ctrl::Cmd_Save:
        if (cmd->Flags != Ctrl::RC_OK)
          amp_message(HFrame, MSG_ERROR, "The current active decoder don't support saving of a stream.");
        break;
       case Ctrl::Cmd_Location:
        { IsLocMsg = false;
          if (IsSliderDrag)
            break;
          // Update CurrentIter
          if (cmd->Flags == Ctrl::RC_OK)
            CurrentIter = Ctrl::GetLoc();
          // Redraws
          UpdateFlags upd = UpdAtLocMsg;
          UpdAtLocMsg = UPD_NONE;
          if (Cfg::Get().mode == CFG_MODE_REGULAR)
            upd |= UPD_TIMERS;
          Invalidate(upd);
          break;
        }
       default:; // suppress warnings
      }
      // TODO: reasonable error messages in case of failed commands.
      // now the command can die
      delete cmd;
    }
    return 0;

   case WMP_CTRL_EVENT:
    { // Event from the controller
      Ctrl::EventFlags flags = (Ctrl::EventFlags)LONGFROMMP(mp1);
      DEBUGLOG(("GUIImp::DlgProc: WMP_CTRL_EVENT %x\n", flags));
      UpdateFlags inval = UPD_NONE;
      if (flags & Ctrl::EV_PlayStop)
      { if (Ctrl::IsPlaying())
        { WinSendDlgItemMsg(HPlayer, BMP_PLAY, WM_PRESS, 0, 0);
          WinSendDlgItemMsg(HPlayer, BMP_PLAY, WM_SETHELP, MPFROMP("Stops playback"), 0);
        } else
        { WinSendDlgItemMsg(HPlayer, BMP_PLAY, WM_DEPRESS, 0, 0);
          WinSendDlgItemMsg(HPlayer, BMP_PLAY, WM_SETHELP, MPFROMP("Starts playback"), 0);
          WinSetWindowText (HFrame,  AMP_FULLNAME);
          ForceLocationMsg();
          inval |= UPD_ALL;
        }
      }
      if (flags & Ctrl::EV_Pause)
        WinSendDlgItemMsg(HPlayer, BMP_PAUSE,   Ctrl::IsPaused() ? WM_PRESS : WM_DEPRESS, 0, 0);
      if (flags & Ctrl::EV_Scan)
      { WinSendDlgItemMsg(HPlayer, BMP_FWD,     Ctrl::GetScan() > 0 ? WM_PRESS : WM_DEPRESS, 0, 0);
        WinSendDlgItemMsg(HPlayer, BMP_REW,     Ctrl::GetScan() < 0 ? WM_PRESS : WM_DEPRESS, 0, 0);
      }
      if (flags & Ctrl::EV_Shuffle)
      { WinSendDlgItemMsg(HPlayer, BMP_SHUFFLE, Ctrl::IsShuffle() ? WM_PRESS : WM_DEPRESS, 0, 0);
        // Update remaining time because playback sequence changed.
        UpdAtLocMsg |= UPD_TIMERS|UPD_PLINDEX;
      }
      if (flags & Ctrl::EV_Repeat)
        WinSendDlgItemMsg(HPlayer, BMP_REPEAT,  Ctrl::IsRepeat() ? WM_PRESS : WM_DEPRESS, 0, 0);
      if (flags & (Ctrl::EV_Root|Ctrl::EV_Song))
      { CurrentIter = Ctrl::GetLoc();
        APlayable* root = CurrentIter->GetRoot();
        if (flags & Ctrl::EV_Root)
        { // New root => request some infos, attach observer
          RootDeleg.detach();
          DEBUGLOG(("GUIImp::DlgProc: AMP_CTRL_EVENT new root: %p\n", root));
          if (root)
          { root->RequestInfo(IF_Aggreg, PRI_Low);
            root->GetInfoChange() += RootDeleg;
          }
          inval |= UPD_TIMERS|UPD_PLMODE|UPD_PLINDEX|UPD_RATE|UPD_CHANNELS|UPD_TEXT;
        }
        // New song => attach observer
        CurrentDeleg.detach();
        int_ptr<APlayable> song = Ctrl::GetCurrentSong();
        DEBUGLOG(("GUIImp::DlgProc: AMP_CTRL_EVENT new song: %p\n", song.get()));
        ASSERT(!root || song);
        if (song && song != root) // Do not observe song items twice.
          song->GetInfoChange() += CurrentDeleg;
        // Refresh display text
        inval |= UPD_TIMERS|UPD_PLINDEX|UPD_RATE|UPD_CHANNELS|UPD_TEXT;
      } else if (flags & Ctrl::EV_Location)
        UpdAtLocMsg |= UPD_TIMERS;

      if (flags & Ctrl::EV_Volume)
        inval |= UPD_VOLUME;

      if (inval)
        Invalidate(inval);
      if (UpdAtLocMsg)
        ForceLocationMsg();
    }
    return 0;
    
   case WMP_PLAYABLE_EVENT:
    { APlayable* root = CurrentIter->GetRoot();
      DEBUGLOG(("GUIImp::DlgProc: WMP_PLAYABLE_EVENT %p %x\n", mp1, SHORT1FROMMP(mp2)));
      if (!root)
        return 0;
      APlayable* ap = (APlayable*)mp1;
      InfoFlags changed = (InfoFlags)SHORT1FROMMP(mp2);
      InfoFlags touched = changed | (InfoFlags)SHORT2FROMMP(mp2);
      //InfoFlags loaded  = (InfoFlags)SHORT2FROMMP(mp2);
      // Dependency map:
      // UPD_TIMERS   <- current | current.obj.songlength | root.obj.songlength | seek end | WM_TIMER
      // UPD_TOTALS   <- (*current.obj.songlength | root.obj.songlength*) | root.rpl.totalsongs | TODO: plylist reorderings
      // UPD_PLMODE   <- root.tech.attributes
      // UPD_PLINDEX  <- current | TODO: plylist reorderings
      // UPD_RATE     <- current.obj.bitrate
      // UPD_CHANNELS <- current.tech.channels
      // UPD_VOLUME   <- controller.volume
      // UPD_TEXT @ FILENAME <- n/a
      // UPD_TEXT @ ID3      <- current.meta.*
      // UPD_TEXT @ INFO     <- current.tech.info
      UpdateFlags upd = UPD_NONE;
      if (root == ap)
      { // Root change event
        if (touched & IF_Rpl)
          UpdAtLocMsg |= UPD_PLINDEX;
        if (touched & IF_Drpl)
          UpdAtLocMsg |= UPD_TIMERS;
        if (changed & IF_Tech)
          upd |= UPD_PLMODE;
      }
      if (CurrentIter->GetCurrent() == ap)
      { // Current change event
        if (changed & IF_Obj)
          upd |= UPD_RATE|UPD_TIMERS;
        if (changed & IF_Drpl)
          UpdAtLocMsg |= UPD_TIMERS;
        if (changed & IF_Tech)
          upd |= UPD_CHANNELS|UPD_TEXT;
        if ((changed & IF_Meta) && Cfg::Get().viewmode == CFG_DISP_ID3TAG)
          upd |= UPD_TEXT;
      }

      switch (Cfg::Get().mode)
      {case CFG_MODE_TINY:
        return 0; // No updates in tiny mode
       case CFG_MODE_SMALL:
        upd &= UPD_TEXT; // reduced update in small mode
       default:;
      }
      if (upd)
        Invalidate(upd);
      if (UpdAtLocMsg)
        ForceLocationMsg();
    }
    return 0;
    
   case WMP_REFRESH_ACCEL:
    { // eat other identical messages
      QMSG qmsg;
      while (WinPeekMsg(amp_player_hab, &qmsg, HFrame, WMP_REFRESH_ACCEL, WMP_REFRESH_ACCEL, PM_REMOVE));
    }
    LoadAccel();
    return 0;

   case WMP_PAINT:
    if (!Terminate)
    { // is there anything to draw with HPS?
      UpdateFlags mask = (UpdateFlags)LONGFROMMP(mp1) & UpdFlags;
      if (mask)
      { // Acknowledge bits
        UpdFlags &= ~mask;
        PresentationSpace hps(HPlayer);
        Paint(hps, mask);
      }
    }
    return 0;

   case WM_CONTEXTMENU:
    ShowContextMenu();
    return 0;

   case WM_INITMENU:
    if (SHORT1FROMMP(mp1) == IDM_M_PLAYBACK)
    { HWND menu = HWNDFROMMP(mp2);
      mn_check_item(menu, BMP_PLAY,    Ctrl::IsPlaying());
      mn_check_item(menu, BMP_PAUSE,   Ctrl::IsPaused() );
      mn_check_item(menu, BMP_REW,     Ctrl::GetScan() < 0);
      mn_check_item(menu, BMP_FWD,     Ctrl::GetScan() > 0);
      mn_check_item(menu, BMP_SHUFFLE, Ctrl::IsShuffle());
      mn_check_item(menu, BMP_REPEAT,  Ctrl::IsRepeat() );
    }
    break;

   case WM_MENUEND:
    if (HWNDFROMMP(mp2) == ContextMenu)
    { // Detach the current playlist from the menu to avoid strong references.
      if (MenuWorker)
        MenuWorker->DetachMenu(IDM_M_PLCONTENT);
    }
    break;

   case WM_TIMER:
    DEBUGLOG2(("GUIImp::DlgProc: WM_TIMER - %x\n", LONGFROMMP(mp1)));
    switch (LONGFROMMP(mp1))
    {case TID_ONTOP:
      WinSetWindowPos(HFrame, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
      DEBUGLOG2(("GUIImp::DlgProc: WM_TIMER done\n"));
      return 0;

     case TID_UPDATE_PLAYER:
      { if( bmp_scroll_text())
        { PresentationSpace hps(HPlayer);
          bmp_draw_text(hps);
        }
      }
      DEBUGLOG2(("GUIImp::DlgProc: WM_TIMER done\n"));
      return 0;

     case TID_UPDATE_TIMERS:
      if (Ctrl::IsPlaying())
        // We must not mask this message request further, because the controller relies on that polling
        // to update it's internal status.
        ForceLocationMsg();
      DEBUGLOG2(("GUIImp::DlgProc: WM_TIMER done\n"));
      return 0;

     case TID_CLEANUP:
      Playable::Cleanup();
      xstring::deduplicator().cleanup();
      return 0;
    }
    break;

   case WM_ERASEBACKGROUND:
    return 0;

   case WM_REALIZEPALETTE:
    Visual::BroadcastMsg(msg, mp1, mp2);
    break;

   case WM_PAINT:
    { HPS hps = WinBeginPaint(HPlayer, NULLHANDLE, NULL);
      Paint(hps, Terminate ? UPD_BACKGROUND|UPD_LED : UPD_ALL|UPD_BACKGROUND|UPD_LED);
      WinEndPaint(hps);
      return 0;
    }

   case WM_COMMAND:
    { USHORT cmd = SHORT1FROMMP(mp1);
      DEBUGLOG(("GUI::DlgProc: WM_COMMAND(%u, %u, %u)\n", cmd, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      if (cmd > IDM_M_PLUG && cmd <= IDM_M_PLUG_E)
      { ASSERT((size_t)(cmd-(IDM_M_PLUG+1)) < PluginMenuContent.size());
        const int_ptr<Module>& mp = Module::FindByKey(PluginMenuContent[cmd-(IDM_M_PLUG+1)]);
        if (mp) // Does the plug-in still exist?
          mp->Config(HPlayer);
        return 0;
      }
      if ( cmd >= IDM_M_LOADFILE && cmd < IDM_M_LOADFILE + sizeof LoadWizards / sizeof *LoadWizards
        && LoadWizards[cmd-IDM_M_LOADFILE] )
      { // TODO: create temporary playlist
        LoadHelper lh(Cfg::Get().playonload*LoadHelper::LoadPlay | LoadHelper::LoadRecall);
        if ((*LoadWizards[cmd-IDM_M_LOADFILE])(HPlayer, "Load%s", &GUI_LoadFileCallback, &lh) == 0)
          PostCtrlCommand(lh.ToCommand());
        return 0;
      }

      switch( cmd )
      {case IDM_M_ADDBOOK:
        { APlayable* cur = CurrentIter->GetCurrent();
          if (cur)
            amp_add_bookmark(HPlayer, *cur);
          break;
        }
       case IDM_M_ADDBOOK_TIME:
        { APlayable* cur = CurrentIter->GetCurrent();
          if (cur)
          { PlayableRef ps(*cur);
            // Use the time index from last_status here.
            if (CurrentIter->GetPosition() >= 0)
            { AttrInfo attr(*ps.GetInfo().attr);
              attr.at = PM123Time::toString(CurrentIter->GetPosition());
              ps.OverrideAttr(&attr);
            }
            amp_add_bookmark(HPlayer, ps);
          }
          break;
        }
       case IDM_M_ADDPLBOOK:
        { APlayable* ap = CurrentIter->GetRoot();
          if (ap)
            amp_add_bookmark(HPlayer, *ap);
          break;
        }
       case IDM_M_ADDPLBOOK_TIME:
        { APlayable* ap = CurrentIter->GetRoot();
          if (ap)
          { PlayableRef ps(*ap);
            AttrInfo attr(*ps.GetInfo().attr);
            attr.at = CurrentIter->Serialize(true);
            ps.OverrideAttr(&attr);
            amp_add_bookmark(HPlayer, ps);
          }
          break;
        }
       case IDM_M_EDITBOOK:
        PlaylistView::GetByKey(*DefaultBM)->SetVisible(true);
        break;

       case IDM_M_EDITBOOKTREE:
        PlaylistManager::GetByKey(*DefaultBM)->SetVisible(true);
        break;

       case IDM_M_INFO:
        { APlayable* cur = CurrentIter->GetCurrent();
          if (cur)
            ShowDialog(cur->GetPlayable(), DLT_METAINFO);
          break;
        }
       case IDM_M_PLINFO:
        { APlayable* root = CurrentIter->GetRoot();
          if (root)
            ShowDialog(*root, root->GetInfo().tech->attributes & TATTR_SONG ? DLT_METAINFO : DLT_TECHINFO);
          break;
        }
       case IDM_M_TAG:
        { APlayable* cur = CurrentIter->GetCurrent();
          if (cur)
            ShowDialog(cur->GetPlayable(), DLT_INFOEDIT);
          break;
        }
       case IDM_M_RELOAD:
        { APlayable* cur = CurrentIter->GetCurrent();
          if (cur)
            cur->RequestInfo(IF_Decoder, PRI_Normal, REL_Reload);
          break;
        }
       case IDM_M_DETAILED:
        { APlayable* root = CurrentIter->GetRoot();
          if (root && root->IsPlaylist())
            ShowDialog(*root, DLT_PLAYLIST);
          break;
        }
       case IDM_M_TREEVIEW:
        { APlayable* root = CurrentIter->GetRoot();
          if (root && root->IsPlaylist())
            ShowDialog(*root, DLT_PLAYLISTTREE);
          break;
        }
       case IDM_M_ADDPMBOOK:
        { APlayable* root = CurrentIter->GetRoot();
          if (root)
            DefaultPM->InsertItem(*root, NULL);
          break;
        }
       case IDM_M_PLRELOAD:
        { APlayable* root = CurrentIter->GetRoot();
          if ( root && root->IsPlaylist()
            && (root->GetPlayable().IsModified() || amp_query(HPlayer, "The current list is modified. Discard changes?")) )
            root->RequestInfo(IF_Decoder, PRI_Normal, REL_Reload);
          break;
        }
       case IDM_M_PLSAVE:
       case IDM_M_PLSAVEAS:
        { APlayable* root = CurrentIter->GetRoot();
          if (root && root->IsPlaylist())
            amp_save_playlist(HPlayer, root->GetPlayable(), cmd == IDM_M_PLSAVEAS);
          break;
        }
       case IDM_M_SAVE:
          // TODO: Save stream
          //amp_save_stream(HPlayer, !Ctrl::GetSavename());
          break;

       case IDM_M_PLAYLIST:
        PlaylistView::GetByKey(*DefaultPL)->SetVisible(true);
        break;

       case IDM_M_MANAGER:
        PlaylistManager::GetByKey(*DefaultPM)->SetVisible(true);
        break;

       case IDM_M_PLOPENDETAIL:
        { url123 URL = amp_playlist_select(HPlayer, "Open Playlist");
          if (URL)
            ShowDialog(*Playable::GetByURL(URL), DLT_PLAYLIST);
          break;
        }
       case IDM_M_PLOPENTREE:
        { url123 URL = amp_playlist_select(HPlayer, "Open Playlist Tree");
          if (URL)
            ShowDialog(*Playable::GetByURL(URL), DLT_PLAYLISTTREE);
          break;
        }

       case IDM_M_FLOAT:
        { Cfg::ChangeAccess cfg;
          cfg.floatontop = !cfg.floatontop;
          break;
        }
       case IDM_M_FONT1:
       case IDM_M_FONT2:
        { Cfg::ChangeAccess cfg;
          cfg.font = cmd - IDM_M_FONT1;
          cfg.font_skinned = true;
          break;
        }
       case IDM_M_NORMAL:
       case IDM_M_SMALL:
       case IDM_M_TINY:
        Cfg::ChangeAccess().mode = (cfg_mode)(cmd - IDM_M_NORMAL);
        break;

       case IDM_M_MINIMIZE:
        WinSetWindowPos(HFrame, HWND_DESKTOP, 0, 0, 0, 0, SWP_HIDE);
        WinSetActiveWindow(HWND_DESKTOP, WinQueryWindow(HPlayer, QW_NEXTTOP));
        break;

       case IDM_M_SKINLOAD:
        amp_loadskin();
        break;

       case IDM_M_CFG:
        WinSendMsg(HPlayer, WMP_SHOW_CONFIG, MPFROMP(NULL), MPFROMSHORT(DLA_SHOW));
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
        ShowContextMenu();
        break;

       case IDM_M_INSPECTOR:
        InspectorDialog::GetInstance()->SetVisible(true);
        break;

       case BMP_PL:
        { APlayable* root = CurrentIter->GetRoot();
          int_ptr<PlaylistView> pv(PlaylistView::GetByKey(root && root->IsPlaylist() ? root->GetPlayable() : *DefaultPL));
          ShowHidePlaylist(pv, pv->GetVisible() ? DLA_CLOSE : DLA_SHOW);
          break;
        }

       case BMP_REPEAT:
        Ctrl::PostCommand(Ctrl::MkRepeat(Ctrl::Op_Toggle));
        WinSendDlgItemMsg(HPlayer, BMP_REPEAT, Ctrl::IsRepeat() ? WM_PRESS : WM_DEPRESS, 0, 0);
        break;

       case BMP_SHUFFLE:
        Ctrl::PostCommand(Ctrl::MkShuffle(Ctrl::Op_Toggle));
        WinSendDlgItemMsg(HPlayer, BMP_SHUFFLE, Ctrl::IsShuffle() ? WM_PRESS : WM_DEPRESS, 0, 0);
        break;

       case BMP_POWER:
        Terminate = true;
        WinPostMsg(HPlayer, WM_QUIT, 0, 0);
        break;

       case BMP_PLAY:
        PostCtrlCommand(Ctrl::MkPlayStop(Ctrl::Op_Toggle));
        break;

       case BMP_PAUSE:
        PostCtrlCommand(Ctrl::MkPause(Ctrl::Op_Toggle));
        break;

       case BMP_FLOAD:
        return WinSendMsg(HPlayer, WM_COMMAND, MPFROMSHORT(IDM_M_LOADFILE), mp2);

       case BMP_STOP:
        Ctrl::PostCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
        break;

       case BMP_NEXT:
        PostCtrlCommand(Ctrl::MkSkip(1, true));
        break;

       case BMP_PREV:
        PostCtrlCommand(Ctrl::MkSkip(-1, true));
        break;

       case BMP_FWD:
        PostCtrlCommand(Ctrl::MkScan(Ctrl::Op_Toggle));
        break;

       case BMP_REW:
        PostCtrlCommand(Ctrl::MkScan(Ctrl::Op_Toggle|Ctrl::Op_Rewind));
        break;
      }
      return 0;
    }

   case WM_CONTROL:
    DEBUGLOG(("GUIImp::DlgProc: WM_CONTROL(%p, %p)\n", mp1, mp2));
    switch (SHORT1FROMMP(mp1))
    {case IDM_M_BOOKMARKS:
     case IDM_M_LOAD:
     case IDM_M_PLCONTENT:
      { // bookmark is selected
        PlaylistMenu::MenuCtrlData& cd = *(PlaylistMenu::MenuCtrlData*)PVOIDFROMMP(mp2);
        NavigateTo(cd.Item);
        break;
      }
    }
    return 0;

   case WM_HELP:
    DEBUGLOG(("GUIImp::DlgProc: WM_HELP(%u, %u, %u)\n", SHORT1FROMMP(mp1), SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
    switch (SHORT1FROMMP(mp2))
    {case CMDSRC_MENU:
      // The default menu processing seems to call the help hook with unreasonable values for SubTopic.
      // So let's introduce a very short way.
      ShowHelp(SHORT1FROMMP(mp1));
      return 0;
    }
    break;

   case WM_BUTTON1DBLCLK:
   case WM_BUTTON1CLICK:
    { POINTL pos;
      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      if (bmp_pt_in_volume(pos))
        Ctrl::PostCommand(Ctrl::MkVolume(bmp_calc_volume(pos), false));
      else if (CurrentIter->GetRoot() && bmp_pt_in_text(pos) && msg == WM_BUTTON1DBLCLK)
        WinPostMsg(HPlayer, WMP_DISPLAY_MODE, 0, 0);
      return 0;
    }

   case WM_MOUSEMOVE:
    if (IsVolumeDrag)
    { POINTL pos;
      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);
      PostCtrlCommand(Ctrl::MkVolume(bmp_calc_volume(pos), false));
    }
    if (IsSliderDrag)
      WinPostMsg(HPlayer, WMP_SLIDERDRAG, mp1, MPFROMSHORT(FALSE));

    WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE));
    return 0;

   case WM_BUTTON1MOTIONSTART:
    { POINTL pos;
      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      if (bmp_pt_in_volume(pos))
      { IsVolumeDrag = true;
        WinSetCapture(HWND_DESKTOP, HPlayer);
      } else if (bmp_pt_in_slider(pos))
      { APlayable* root = CurrentIter->GetRoot();
        if (root && (IsAltSlider
          ? root->GetInfo().rpl->songs > 0
          : CurrentIter->GetCurrent()->GetInfo().drpl->totallength > 0 ))
        { IsSliderDrag = true;
          WinSetCapture(HWND_DESKTOP, HPlayer);
        }
      }
      return 0;
    }

   case WM_BUTTON1MOTIONEND:
    if (IsVolumeDrag)
    { IsVolumeDrag = false;
      WinSetCapture(HWND_DESKTOP, NULLHANDLE);
    }
    if (IsSliderDrag)
    { WinPostMsg(HPlayer, WMP_SLIDERDRAG, mp1, MPFROMSHORT(TRUE));
      WinSetCapture(HWND_DESKTOP, NULLHANDLE);
    }
    return 0;

   case WM_BUTTON2MOTIONSTART:
    WinSendMsg(HFrame, WM_TRACKFRAME, MPFROMSHORT(TF_MOVE|TF_STANDARD), 0);
    //WinQueryWindowPos(HFrame, &cfg.main);
    Cfg::SaveWindowPos(HFrame);
    return 0;

   case WM_CHAR:
    { USHORT fsflags = SHORT1FROMMP(mp1);
      DEBUGLOG(("GUIImp::DlgProc: WM_CHAR: %x, %u, %u, %x, %x\n", fsflags,
        SHORT2FROMMP(mp1)&0xff, SHORT2FROMMP(mp1)>>8, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      if (fsflags & KC_VIRTUALKEY)
      { switch (SHORT2FROMMP(mp2))
        {case VK_ESC:
          if(!(fsflags & KC_KEYUP))
            IsSliderDrag = false;
          break;
        }
      }
      // Force heap dumps by the D key
      #if defined(DEBUG_MEM) && defined(DEBUG_LOG)
      #ifdef __DEBUG_ALLOC__
      if ((fsflags & (KC_CHAR|KC_ALT|KC_CTRL)) == (KC_CHAR) && toupper(SHORT1FROMMP(mp2)) == 'D')
      { if (fsflags & KC_SHIFT)
          _dump_allocated_delta(0);
        else
          _dump_allocated(0);
      }
      #elif defined(__GNUC__)
      if ((fsflags & (KC_CHAR|KC_ALT|KC_CTRL)) == (KC_CHAR) && toupper(SHORT1FROMMP(mp2)) == 'D')
      { heap_sum[0] = heap_sum[1] = 0;
        heap_count[0] = heap_count[1] = 0;
        _heap_walk(heap_walker);
        debuglog("HEAP sum %u/%u, count %u/%u\n", heap_sum[0], heap_sum[1], heap_count[0], heap_count[1]);
      }
      #endif
      #endif
      break;
    }

   case WM_TRANSLATEACCEL:
    { /* WM_CHAR with VK_ALT and KC_KEYUP does not survive this message, so we catch it before. */
      QMSG* pqmsg = (QMSG*)mp1;
      DEBUGLOG(("GUIImp::DlgProc: WM_TRANSLATEACCEL: %x, %x, %x\n", pqmsg->msg, pqmsg->mp1, pqmsg->mp2));
      if (pqmsg->msg == WM_CHAR)
      { USHORT fsflags = SHORT1FROMMP(pqmsg->mp1);
        if (fsflags & KC_VIRTUALKEY)
        { switch (SHORT2FROMMP(pqmsg->mp2))
          {case VK_ALT:
           case VK_ALTGRAF:
             if (Cfg::Get().altbutton != CFG_BUT_ALT)
               break;
            goto setanav;
           case VK_CTRL:
            if (Cfg::Get().altbutton != CFG_BUT_CTRL)
              break;
            goto setanav;
           case VK_SHIFT:
            if (Cfg::Get().altbutton != CFG_BUT_SHIFT)
              break;
           setanav:
            APlayable* root = CurrentIter->GetRoot();
            SetAltSlider(!(fsflags & KC_KEYUP) && root && root->IsPlaylist());
          }
        }
      }
      break;
    }

   case WMP_SLIDERDRAG:
    if (IsSliderDrag)
    { JobSet job(PRI_Normal);
      SliderDrag(SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), job);
      // Did we have unfulfilled dependencies?
      job.Commit();
      if (job.AllDepends.Size())
      { // yes => reschedule the request later.
        DelayedAltSliderDragWorker.Start(job.AllDepends, HPlayer, WMP_SLIDERDRAG, mp1, mp2);
      } else if (SHORT1FROMMP(mp2))
      { // Set new position
        PostCtrlCommand(Ctrl::MkJump(new Location(*CurrentIter), false));
        IsSliderDrag = false;
      } else
        // Show new position
        Invalidate(UPD_TIMERS|(UPD_TEXT|UPD_RATE|UPD_CHANNELS|UPD_PLINDEX)*IsAltSlider);
    }
    return 0;

   case WMP_ARRANGEDOCKING:
    if (Cfg::Get().dock_windows)
      dk_arrange(HPlayer); // TODO: frame window???
    else
      dk_cleanup(HPlayer);
    return 0;

   case DM_DRAGOVER:
    return DragOver((PDRAGINFO)mp1);
   case DM_DROP:
    DragDrop((PDRAGINFO)mp1);
    return 0;
   case DM_RENDERCOMPLETE:
    DropRenderComplete((PDRAGTRANSFER)mp1, SHORT1FROMMP(mp2));
    DEBUGLOG(("GUIImp::DlgProc: DM_RENDERCOMPLETE done\n"));
    return 0;
   case DM_DROPHELP:
    ShowHelp(IDH_DRAG_AND_DROP);
    // Continue in default procedure to free ressources.
    break;
  }

  DEBUGLOG2(("GUIImp::DlgProc: before WinDefWindowProc\n"));
  return WinDefWindowProc(HPlayer, msg, mp1, mp2);
}

void GUIImp::Invalidate(UpdateFlags what)
{ DEBUGLOG(("GUIImp::Invalidate(%x) - %x\n", what, UpdFlags));
  if ((~UpdFlags & what) == 0)
    return; // Nothing to set
  UpdFlags |= what;
  WinPostMsg(HPlayer, WMP_PAINT, MPFROMLONG(what), 0);
}

/* set alternate navigation status to 'alt'. */
void GUIImp::SetAltSlider(bool alt)
{ DEBUGLOG(("GUIImp::SetAltSlider(%u) - %u\n", alt, IsAltSlider));
  if (IsAltSlider != alt)
  { IsAltSlider = alt;
    Invalidate(UPD_TIMERS|UPD_PLINDEX);
  }
}

void GUIImp::SliderDrag(LONG x, LONG y, JobSet& job)
{ DEBUGLOG(("GUIImp::SliderDrag(%li, %li, ) - %u\n", x,y, IsAltSlider));
  // Cancel delayed slider drag if any
  DelayedAltSliderDragWorker.Cancel();

  APlayable* root = CurrentIter->GetRoot();
  if (!root)
    return;
  // get position
  POINTL pos;
  pos.x = x;
  pos.y = y;
  double relpos = bmp_calc_time(pos);
  if (relpos < 0.)
    relpos = 0;
  else if (relpos >= 1.)
    relpos = 1. - 1E-6; // Well, PM123 dislikes to navigate exactly to the end of the song.

  // adjust CurrentIter from pos
  if (!IsAltSlider)
  { const PM123_TIME songlength = CurrentIter->GetCurrent()->GetInfo().drpl->totallength;
    // navigation within the current song
    if (songlength <= 0)
      return;
    CurrentIter->NavigateRewindSong();
    CurrentIter->NavigateTime(job, relpos * songlength);
  } else
    switch (Cfg::Get().altnavig)
    {case CFG_ANAV_TIME:
      // Navigate only at the time scale
      if (root->GetInfo().drpl->totallength > 0)
      { CurrentIter->Reset();
        bool r = CurrentIter->NavigateTime(job, relpos * root->GetInfo().drpl->totallength);
        DEBUGLOG(("GUIImp::DlgProc: AMP_SLIDERDRAG: CFG_ANAV_TIME: %u\n", r));
        break;
      }
      // else if no total time is available use song time navigation
     case CFG_ANAV_SONG:
     case CFG_ANAV_SONGTIME:
      // navigate at song and optional time scale
      { const int num_items = root->GetInfo().rpl->songs;
        if (num_items <= 0)
          return;
        relpos *= num_items;
        relpos += 1.;
        if (relpos > num_items)
          relpos = num_items;
        CurrentIter->Reset();
        bool r = CurrentIter->NavigateCount(job, (int)floor(relpos), TATTR_SONG);
        DEBUGLOG(("GUIImp::DlgProc: AMP_SLIDERDRAG: CFG_ANAV_SONG: %f, %u\n", relpos, r));
        // navigate within the song
        const PM123_TIME songlength = CurrentIter->GetCurrent()->GetInfo().drpl->totallength;
        if (Cfg::Get().altnavig != CFG_ANAV_SONG && songlength > 0)
        { relpos -= floor(relpos);
          CurrentIter->NavigateRewindSong();
          CurrentIter->NavigateTime(job, relpos * songlength);
        }
      }
    }
}

void GUIImp::ForceLocationMsg()
{ if (!IsLocMsg)
  { IsLocMsg = true;
    PostCtrlCommand(Ctrl::MkLocation(Ctrl::LM_UpdatePos));
  }
}

void GUIImp::ControllerEventCB(Ctrl::ControlCommand* cmd)
{ DEBUGLOG(("GUIImp::ControllerEventCB(%p{%i, ...)\n", cmd, cmd->Cmd));
  WinPostMsg(HFrame, WMP_CTRL_EVENT_CB, MPFROMP(cmd), 0);
}

void GUIImp::ControllerNotification(const void* rcv, const Ctrl::EventFlags& flags)
{ DEBUGLOG(("GUIImp::ControllerNotification(, %x)\n", flags));
  Ctrl::EventFlags flg = flags;
  QMSG msg;
  if (WinPeekMsg(amp_player_hab, &msg, HFrame, WMP_CTRL_EVENT, WMP_CTRL_EVENT, PM_REMOVE))
  { DEBUGLOG(("GUIImp::ControllerNotification - hit: %x\n", msg.mp1));
    // join messages
    flg |= (Ctrl::EventFlags)LONGFROMMP(msg.mp1);
  }
  WinPostMsg(HFrame, WMP_CTRL_EVENT, MPFROMLONG(flg), 0);
}

void GUIImp::PlayableNotification(const void* rcv, const PlayableChangeArgs& args)
{ DEBUGLOG(("GUIImp::PlayableNotification(, {&%p, %p, %x, %x, %x})\n",
    &args.Instance, args.Origin, args.Changed, args.Loaded, args.Invalidated));
  // TODO: Filter to improve performance
  WinPostMsg(HFrame, WMP_PLAYABLE_EVENT, MPFROMP(&args.Instance), MPFROM2SHORT(args.Changed, args.Loaded));
}

void GUIImp::PluginNotification(const void*, const PluginEventArgs& args)
{ DEBUGLOG(("GUIImp::PluginNotification(, {&%p{%s}, %i})\n", args.Plug, args.Plug ? args.Plug->ModRef->Key.cdata() : "", args.Operation));
  switch (args.Operation)
  {case PluginEventArgs::Load:
   case PluginEventArgs::Unload:
    if (!args.Plug->GetEnabled())
      break;
   case PluginEventArgs::Enable:
   case PluginEventArgs::Disable:
    switch (args.Plug->PluginType)
    {case PLUGIN_DECODER:
      WinPostMsg(HPlayer, WMP_REFRESH_ACCEL, 0, 0);
      break;
     case PLUGIN_VISUAL:
      { int_ptr<Visual> vis = (Visual*)args.Plug;
        if (args.Operation & 1)
          // activate visual, delayed
          PMRASSERT(WinPostMsg(HPlayer, WMP_LOAD_VISUAL, MPFROMP(vis.toCptr()), MPFROMLONG(TRUE)));
        else
          // deactivate visual
          vis->UninitPlugin();
      }
      break;
     default:; // avoid warnings
    }
    IsPluginChanged = true;
   default:; // avoid warnings
  }
}

void GUIImp::ConfigNotification(const void*, const CfgChangeArgs& args)
{
  if (args.New.floatontop != args.Old.floatontop)
  { if (!args.New.floatontop)
      WinStopTimer(amp_player_hab, HPlayer, TID_ONTOP);
    else
      WinStartTimer(amp_player_hab, HPlayer, TID_ONTOP, 100);
  }

  if (args.New.defskin != args.Old.defskin)
    PMRASSERT(WinPostMsg(HPlayer, WMP_RELOADSKIN, 0, 0));

  if (args.New.mode != args.Old.mode)
  { bmp_reflow_and_resize(HPlayer);
    Invalidate(UPD_ALL);
  } else
    Invalidate(UPD_TEXT);

  if ( args.New.dock_windows != args.Old.dock_windows
    || args.New.dock_margin != args.Old.dock_margin )
    PMRASSERT(WinPostMsg(HPlayer, WMP_ARRANGEDOCKING, 0, 0));
}


void GUIImp::SaveStream(HWND hwnd, BOOL enable)
{
  if( enable )
  { FILEDLG filedialog = { sizeof(FILEDLG) };
    filedialog.fl         = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB;
    filedialog.pszTitle   = "Save stream as";

    xstring savedir = Cfg::Get().savedir;
    if (savedir.length() > 8)
      // strip file:///
      strlcpy(filedialog.szFullFile, surl2file(savedir), sizeof filedialog.szFullFile);
    else
      filedialog.szFullFile[0] = 0;
    amp_file_dlg(HWND_DESKTOP, hwnd, &filedialog);

    if (filedialog.lReturn == DID_OK)
    { if (amp_warn_if_overwrite( hwnd, filedialog.szFullFile ))
      { url123 url = url123::normalizeURL(filedialog.szFullFile);
        PostCtrlCommand(Ctrl::MkSave(url));
        Cfg::ChangeAccess().savedir = url.getBasePath();
      }
    }
  } else
    Ctrl::PostCommand(Ctrl::MkSave(xstring()));
}

void GUIImp::LoadAccel()
{ DEBUGLOG(("GUI::LoadAccel()\n"));
  const HAB hab = amp_player_hab;
  // Generate new accelerator table.
  HACCEL haccel = WinLoadAccelTable(hab, NULLHANDLE, ACL_MAIN);
  PMASSERT(haccel != NULLHANDLE);
  memset(LoadWizards+2, 0, sizeof LoadWizards - 2*sizeof *LoadWizards ); // You never know...
  Decoder::AppendAccelTable(haccel, IDM_M_LOADOTHER, 0, LoadWizards + 2, sizeof LoadWizards / sizeof *LoadWizards - 2);
  // Replace table of current window.
  HACCEL haccel_old = WinQueryAccelTable(hab, HFrame);
  PMRASSERT(WinSetAccelTable(hab, haccel, HFrame));
  if (haccel_old != NULLHANDLE)
    PMRASSERT(WinDestroyAccelTable(haccel_old));
  // done
  IsAccelChanged = true;
}

void GUIImp::LoadPluginMenu(HWND hMenu)
{
  size_t   i;
  DEBUGLOG(("GUIImp::LoadPluginMenu(%p)\n", hMenu));

  // Delete all
  size_t count = LONGFROMMR( WinSendMsg( hMenu, MM_QUERYITEMCOUNT, 0, 0 ));
  for( i = 0; i < count; i++ ) {
    short item = LONGFROMMR( WinSendMsg( hMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(0), 0 ));
    WinSendMsg( hMenu, MM_DELETEITEM, MPFROM2SHORT( item, FALSE ), 0);
  }
  // Fetch list of plug-ins atomically
  vector_int<Module> plugins;
  Module::CopyAllInstancesTo(plugins);
  // Extract only the relevant informations.
  // I.e. do not keep strong references to the plug-ins.
  const int_ptr<Module>* mpp = plugins.begin();
  const int_ptr<Module>*const mpe = plugins.end();
  xstring* ep = new xstring[plugins.size()];
  PluginMenuContent.assign(ep, plugins.size());
  MENUITEM mi;
  USHORT id = IDM_M_PLUG;
  while (mpp != mpe)
  { Module& plug = **mpp++;
    *ep++ = plug.Key;
    // Check for enabled plug-in instances
    // Set the mark if any plug-in flavor of the module is enabled.
    int type = plug.GetParams().type;
    bool enabled = false;
    if (type & PLUGIN_DECODER)
    { const int_ptr<Decoder>& plg = Decoder::FindInstance(plug);
      if (plg && plg->GetEnabled())
      { enabled = true;
        goto en;
      }
    }
    if (type & PLUGIN_OUTPUT)
    { const int_ptr<Output>& plg = Output::FindInstance(plug);
      if (plg && plg->GetEnabled())
      { enabled = true;
        goto en;
      }
    }
    if (type & PLUGIN_FILTER)
    { const int_ptr<Filter>& plg = Filter::FindInstance(plug);
      if (plg && plg->GetEnabled())
      { enabled = true;
        goto en;
      }
    }
    if (type & PLUGIN_VISUAL)
    { const int_ptr<Visual>& plg = Visual::FindInstance(plug);
      if (plg && plg->GetEnabled())
      { enabled = true;
        goto en;
      }
    }
   en:

    mi.iPosition   = MIT_END;
    mi.afStyle     = MIS_TEXT;
    mi.afAttribute = 0;
    if (!plug.GetParams().configurable)
      mi.afAttribute |= MIA_DISABLED;
    if (enabled)
      mi.afAttribute |= MIA_CHECKED;

    mi.id          = ++id;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem = 0;
    xstring title;
    title.sprintf("%s (%s)", plug.GetParams().desc, plug.Key.cdata());
    WinSendMsg(hMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(title.cdata()));
    DEBUGLOG2(("GUIImp::LoadPluginMenu: add \"%s\"\n", title.cdata()));
  }

  if (plugins.size() == 0)
  { mi.iPosition   = MIT_END;
    mi.afStyle     = MIS_TEXT;
    mi.afAttribute = MIA_DISABLED;
    mi.id          = ++id;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem       = 0;
    WinSendMsg(hMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP("- No plug-ins -"));
  }
}

void GUIImp::ShowContextMenu()
{
  bool new_menu = false;
  if (ContextMenu == NULLHANDLE)
  { ContextMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_MAIN);
    mn_make_conditionalcascade(ContextMenu, IDM_M_LOAD,   IDM_M_LOADFILE);
    mn_make_conditionalcascade(ContextMenu, IDM_M_PLOPEN, IDM_M_PLOPENDETAIL);
    mn_make_conditionalcascade(ContextMenu, IDM_M_HELP,   IDH_MAIN);

    MenuWorker = new PlaylistMenu(HPlayer, IDM_M_LAST, IDM_M_LAST_E);
    MenuWorker->AttachMenu(ContextMenu, IDM_M_BOOKMARKS, *DefaultBM, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Recursive|PlaylistMenu::Enumerate, 0);
    MenuWorker->AttachMenu(ContextMenu, IDM_M_LOAD, *LoadMRU, PlaylistMenu::Enumerate|PlaylistMenu::Separator, 0);
    new_menu = true;
  }

  POINTL pos;
  WinQueryPointerPos(HWND_DESKTOP, &pos);
  WinMapWindowPoints(HWND_DESKTOP, HPlayer, &pos, 1);

  if (WinWindowFromPoint(HPlayer, &pos, TRUE) == NULLHANDLE)
  { SWP swp;
    // The context menu is probably activated from the keyboard.
    WinQueryWindowPos(HPlayer, &swp);
    pos.x = swp.cx/2;
    pos.y = swp.cy/2;
  }

  if (IsPluginChanged || new_menu)
  { // Update plug-ins.
    MENUITEM mi;
    PMRASSERT(WinSendMsg(ContextMenu, MM_QUERYITEM, MPFROM2SHORT(IDM_M_PLUG, TRUE), MPFROMP(&mi)));
    IsPluginChanged = false;
    LoadPluginMenu(mi.hwndSubMenu);
  }

  if (IsAccelChanged || new_menu)
  { MENUITEM mi;
    PMRASSERT(WinSendMsg(ContextMenu, MM_QUERYITEM, MPFROM2SHORT(IDM_M_LOAD, TRUE), MPFROMP(&mi)));
    // Append asisstents from decoder plug-ins
    memset(LoadWizards+2, 0, sizeof LoadWizards - 2*sizeof *LoadWizards); // You never know...
    IsAccelChanged = false;
    Decoder::AppendLoadMenu(mi.hwndSubMenu, IDM_M_LOADOTHER, 2, LoadWizards + 2, sizeof LoadWizards / sizeof *LoadWizards - 2);
    (MenuShowAccel(WinQueryAccelTable(amp_player_hab, HFrame))).ApplyTo(new_menu ? ContextMenu : mi.hwndSubMenu);
  }

  // Update status
  APlayable* const cur = CurrentIter->GetCurrent();
  APlayable* const root = CurrentIter->GetRoot();
  mn_enable_item(ContextMenu, IDM_M_TAG,     cur && (cur->GetInfo().phys->attributes & PATTR_WRITABLE) && (cur->GetInfo().tech->attributes & TATTR_WRITABLE));
  mn_enable_item(ContextMenu, IDM_M_SAVE,    cur && (cur->GetInfo().tech->attributes & TATTR_STORABLE));
  mn_enable_item(ContextMenu, IDM_M_CURRENT_SONG, cur != NULL);
  bool ispl = root && (root->GetInfo().tech->attributes & TATTR_PLAYLIST);
  mn_enable_item(ContextMenu, IDM_M_CURRENT_PL, ispl);
  mn_enable_item(ContextMenu, IDM_M_SMALL,   bmp_is_mode_supported(CFG_MODE_SMALL));
  mn_enable_item(ContextMenu, IDM_M_NORMAL,  bmp_is_mode_supported(CFG_MODE_REGULAR));
  mn_enable_item(ContextMenu, IDM_M_TINY,    bmp_is_mode_supported(CFG_MODE_TINY));
  mn_enable_item(ContextMenu, IDM_M_FONT,    Cfg::Get().font_skinned );
  mn_enable_item(ContextMenu, IDM_M_FONT1,   bmp_is_font_supported(0));
  mn_enable_item(ContextMenu, IDM_M_FONT2,   bmp_is_font_supported(1));

  mn_check_item(ContextMenu, IDM_M_FLOAT,  Cfg::Get().floatontop);
  mn_check_item(ContextMenu, IDM_M_SAVE,   !!Ctrl::GetSavename());
  mn_check_item(ContextMenu, IDM_M_FONT1,  Cfg::Get().font == 0);
  mn_check_item(ContextMenu, IDM_M_FONT2,  Cfg::Get().font == 1);
  mn_check_item(ContextMenu, IDM_M_NORMAL, Cfg::Get().mode == CFG_MODE_REGULAR);
  mn_check_item(ContextMenu, IDM_M_SMALL,  Cfg::Get().mode == CFG_MODE_SMALL);
  mn_check_item(ContextMenu, IDM_M_TINY,   Cfg::Get().mode == CFG_MODE_TINY);

  if (ispl)
    MenuWorker->AttachMenu(ContextMenu, IDM_M_PLCONTENT, *root, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Enumerate|PlaylistMenu::Recursive, 0);

  WinPopupMenu(HPlayer, HPlayer, ContextMenu, pos.x, pos.y, 0,
               PU_HCONSTRAIN|PU_VCONSTRAIN|PU_MOUSEBUTTON1|PU_MOUSEBUTTON2|PU_KEYBOARD);
}

bool GUIImp::ShowHideInfoDlg(APlayable& p, AInfoDialog::PageNo page, DialogAction action)
{ DEBUGLOG(("GUIImp::ShowHideInfoDlg(&%p, %u, %u)\n", &p, page, action));
  int_ptr<AInfoDialog> ip = action ? AInfoDialog::GetByKey(p) : AInfoDialog::FindByKey(p);
  if (!ip)
    return false;
  bool rc = ip->IsVisible(page);
  switch (action)
  {case DLA_SHOW:
    if (rc)
      ip->SetVisible(true);
    else
      ip->ShowPage(page);
    break;
   case DLA_CLOSE:
    if (rc)
      ip->Close();
   default:; // avoid warning
  }
  return rc;
}

bool GUIImp::ShowHidePlaylist(PlaylistBase* plp, DialogAction action)
{ DEBUGLOG(("GUIImp::ShowHidePlaylist(%p, %u)\n", plp, action));
  if (!plp)
    return false;
  bool rc = plp->GetVisible();
  switch (action)
  {case DLA_SHOW:
    plp->SetVisible(true);
    break;
   case DLA_CLOSE:
    if (rc)
      plp->Close();
   default:; // avoid warning
  }
  return rc;
}


void GUIImp::RefreshTimers(HPS hps, int rem_song, PM123_TIME rem_time)
{ DEBUGLOG(("GUI::RefreshTimers(%p, %i, %f) - %i\n", hps, rem_song, rem_time, Cfg::Get().mode));

  APlayable* root = CurrentIter->GetRoot();
  if (!root)
  { bmp_draw_slider(hps, -1, IsAltSlider);
    bmp_draw_timer(hps, -1);
    bmp_draw_tiny_timer(hps, POS_TIME_LEFT, -1);
    bmp_draw_tiny_timer(hps, POS_PL_LEFT,   -1);
    return;
  }
  APlayable* cur = CurrentIter->GetCurrent();

  const bool is_playlist = !!(root->GetInfo().tech->attributes & TATTR_PLAYLIST);
  PM123_TIME total_song = cur ? cur->GetInfo().drpl->totallength : -1;

  PM123_TIME position = CurrentIter->GetTime();
  if (position < 0)
    position = 0;

  double pos = -1.;
  if (!IsAltSlider)
  { if (total_song > 0)
      pos = position/total_song;
  } else switch (Cfg::Get().altnavig)
  {case CFG_ANAV_SONG:
    if (is_playlist)
    { int total_items = root->GetInfo().rpl->songs;
      if (total_items == 1)
        pos = 0;
      else if (total_items > 1)
        pos = 1 - (rem_song-1) / (double)(total_items-1);
    }
    break;
   case CFG_ANAV_TIME:
    if (root->GetInfo().drpl->totallength > 0)
    { pos = 1 - rem_time / root->GetInfo().drpl->totallength;
      break;
    } // else CFG_ANAV_SONGTIME
   case CFG_ANAV_SONGTIME:
    if (is_playlist)
    { int total_items = root->GetInfo().rpl->songs;
      if (total_items == 1)
        pos = 0;
      else if (total_items > 1)
        pos = total_items - rem_song;
      else break;
      // Add current song time
      if (total_song > 0)
        pos += position/total_song;
      pos /= total_items;
    }
  }
  bmp_draw_slider(hps, pos, IsAltSlider);
  bmp_draw_timer (hps, position);

  bmp_draw_tiny_timer(hps, POS_TIME_LEFT, total_song - position);
  bmp_draw_tiny_timer(hps, POS_PL_LEFT,   rem_time);
}

/* Constructs a information text for currently loaded file
   and selects it for displaying. */
void GUIImp::PrepareText()
{
  APlayable* cur = CurrentIter->GetCurrent();
  if (!cur)
  { cur = CurrentIter->GetRoot();
    if (!cur)
    { DEBUGLOG(("GUI::PrepareText() NULL %u\n", Cfg::Get().viewmode));
      bmp_set_text("- no file loaded -");
      return;
    }
  }
  DEBUGLOG(("GUI::PrepareText() %p %u\n", cur, Cfg::Get().viewmode));

  const volatile amp_cfg& cfg = Cfg::Get();
  xstring text;
  InfoFlags invalid = cur->RequestInfo(IF_Tech|IF_Meta|IF_Item, PRI_Normal);
  const INFO_BUNDLE_CV& info = cur->GetInfo();
  switch (cfg.viewmode)
  {case CFG_DISP_ID3TAG:
    if (!(invalid & IF_Meta))
    { text = ConstructTagString(*info.meta);
      if (text.length())
        break;
      text.reset();
    }
    // if tag is empty - use filename instead of it.
   case CFG_DISP_FILENAME:
    { // Give Priority to an alias name if any
      if (!(invalid & IF_Item))
        text = info.item->alias;
      if (!text)
      { text = cur->GetPlayable().URL.getShortName();
        if (cfg.restrict_meta && text.length() > cfg.restrict_length)
          text = xstring(text, 0, cfg.restrict_length) + "...";
      }
      // In case the information is pending display a message
      if (invalid & IF_Tech)
        text = text + " - loading...";
      // In case of an invalid item display an error message.
      else if (info.tech->attributes & TATTR_INVALID)
        text = text + " - " + xstring(info.tech->info);
      break;
    }
   case CFG_DISP_FILEINFO:
    // In case the information is pending display a message
    if (invalid & IF_Tech)
      text = text + " - loading...";
    else
      text = info.tech->info;
    if (cfg.restrict_meta && text.length() > cfg.restrict_length)
      text = xstring(text, 0, cfg.restrict_length) + "...";
    break;
  }
  bmp_set_text(!text ? "" : text);
}

void GUIImp::Paint(HPS hps, UpdateFlags mask)
{ DEBUGLOG(("GuiImp::Paint(,%x) - %x\n", mask, UpdFlags));
  if (mask & UPD_BACKGROUND)
    bmp_draw_background(hps, HPlayer);
  APlayable* root = CurrentIter->GetRoot();
  APlayable* cur = CurrentIter->GetCurrent();

  if (mask & (UPD_TIMERS|UPD_PLINDEX))
  { int front_index = -1;
    int back_index = -1;
    PM123_TIME back_offset = -1;
    if (root && root->IsPlaylist())
    { AggregateInfo ai(PlayableSetBase::Empty);
      JobSet job(PRI_Low); // The job is discarded, because the update notification will call this function again anyways.
      InfoFlags what = mask & UPD_TIMERS ? IF_Aggreg : IF_Rpl;
      what &= ~CurrentIter->AddBackAggregate(ai, what, job);
      if (what & IF_Rpl)
        back_index = ai.Rpl.songs;
      if (what & IF_Drpl)
        back_offset = ai.Drpl.totallength;
      ai.Reset();
      if (!CurrentIter->AddFrontAggregate(ai, IF_Rpl, job))
        front_index = ai.Rpl.songs;
    }
    if (mask & UPD_TIMERS)
      RefreshTimers(hps, back_index, back_offset);
    if (mask & UPD_PLINDEX)
      if (root && (root->GetInfo().tech->attributes & (TATTR_SONG|TATTR_PLAYLIST)) == TATTR_PLAYLIST)
      { int total = root->GetInfo().rpl->songs;
        bmp_draw_plind(hps, front_index + 1, total);
      } else
        bmp_draw_plind(hps, -1, -1);
  }
  if (mask & UPD_PLMODE)
    bmp_draw_plmode(hps, root != NULL, root && root->IsPlaylist());
  if (mask & UPD_RATE)
    bmp_draw_rate(hps, cur ? cur->GetInfo().obj->bitrate : -1);
  if (mask & UPD_CHANNELS)
    bmp_draw_channels(hps, cur ? cur->GetInfo().tech->channels : -1);
  if (mask & UPD_VOLUME)
    bmp_draw_volume(hps, Ctrl::GetVolume());
  if (mask & UPD_TEXT)
  { PrepareText();
    bmp_draw_text(hps);
  }
  if (mask & UPD_LED)
    bmp_draw_led(hps, IsHaveFocus);
}

void GUIImp::AutoSave(Playable& list)
{ if (list.IsModified())
    list.Save(list.URL, "plist123.dll", NULL, false);
}

void GUIImp::AutoSave()
{ // Save by default
  AutoSave(*LoadMRU);
  AutoSave(*UrlMRU);
  // Others on request
  if (Cfg::Get().autosave)
  { // Keep saves out of the mutex.
    vector<Playable> tosave;
    { Playable::RepositoryAccess rep;
      for (Playable::RepositoryType::iterator ppp(rep->begin()); !ppp.isend(); ++ppp)
      { Playable& p = **ppp;
        if (!p.IsModified())
          continue;
        const INFO_BUNDLE_CV& info = p.GetInfo();
        if ( (info.tech->attributes & (TATTR_PLAYLIST|TATTR_WRITABLE|TATTR_INVALID)) == (TATTR_PLAYLIST|TATTR_WRITABLE)
          && (info.phys->attributes & (PATTR_WRITABLE|PATTR_INVALID)) == PATTR_WRITABLE )
          tosave.append() = &p;
      }
    }
    // Now save
    foreach (Playable,*const*, ppp, tosave)
      AutoSave(**ppp);
  }
  // save too
  AutoSave(*DefaultBM);
  AutoSave(*DefaultPM);
  AutoSave(*DefaultPL);
}

/* Prepares the player to the drop operation. */
MRESULT GUIImp::DragOver(DRAGINFO* pdinfo_)
{ DEBUGLOG(("GUIImp::DragOver(%p)\n", pdinfo_));
  ScopedDragInfo di(pdinfo_);
  if (!di.get())
    return MRFROM2SHORT(DOR_NEVERDROP, 0);
  DEBUGLOG(("GUIImp::DragOver({,,%x, %p, %i,%i, %u,})\n",
    di->usOperation, di->hwndSource, di->xDrop, di->yDrop, di->cditem));

  USHORT drag = DOR_NEVERDROP;
  USHORT op = 0;

  for (int i = 0; i < di->cditem; i++)
  { DragItem item(di[i]);
    if (item.VerifyRMF("DRM_123LST", NULL))
      drag = DOR_DROP;
    else if (item.VerifyRMF("DRM_OS2FILE", NULL))
    { if (item.VerifyType("UniformResourceLocator") || item.SourceName().endsWithI(".skn"))
        op = DO_COPY;
      drag = DOR_DROP;
    } else
    { drag = DOR_NEVERDROP;
      break;
    }
  }

  if (drag == DOR_DROP && op == 0)
    // we can only copy or link to the main window, not move.
    switch (di->usOperation)
    {default:
      drag = DOR_NODROPOP;
      break;
     case DO_COPY:
     case DO_LINK:
      op = di->usOperation;
      break;
     case DO_DEFAULT:
      op = Cfg::Get().append_dnd ? DO_LINK : DO_COPY;
    }

  DEBUGLOG(("GUIImp::DragOver: %u, %x\n", drag, op));
  return MPFROM2SHORT(drag, op);
}

GUIImp::HandleDrop::HandleDrop(DRAGINFO* di, HWND hwnd)
: HandleDragTransfers(di, hwnd)
, Loader(Cfg::Get().playonload*LoadHelper::LoadPlay | (di->usOperation == DO_LINK)*LoadHelper::LoadAppend)
{}

GUIImp::HandleDrop::~HandleDrop()
{ // post load command automatically if anything to post.
  GUIImp::Load(Loader);
}

/* Receives dropped files or playlist records. */
void GUIImp::DragDrop(PDRAGINFO pdinfo_)
{ DEBUGLOG(("GUIImp::DragDrop(%p)\n", pdinfo_));

  int_ptr<HandleDrop> worker(new HandleDrop(pdinfo_, HPlayer));
  DEBUGLOG(("GUIImp::DragDrop: {,%u,%x,%p, %u,%u, %u,}\n",
    (*worker)->cbDragitem, (*worker)->usOperation, (*worker)->hwndSource, (*worker)->xDrop, (*worker)->yDrop, (*worker)->cditem));

  ULONG reply = DMFL_TARGETFAIL;
  for (int i = 0; i < (*worker)->cditem; i++)
  { UseDragTransfer dt((*worker)[i]);
    DragItem item(dt.Item());
    DEBUGLOG(("GUIImp::DragDrop: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      item->hwndItem, item->ulItemID, item.Type().cdata(), item.RMF().cdata(),
      item.ContainerName().cdata(), item.SourceName().cdata(), item.TargetName().cdata(),
      item->cxOffset, item->cyOffset, item->fsControl, item->fsSupportedOps));

    reply = DMFL_TARGETFAIL;

    if (item.VerifyRMF("DRM_123LST", NULL))
    { dt.SelectedRMF("<DRM_123LST,DRF_UNKNOWN>");
      dt.RenderTo(amp_dnd_temp_file());
    }
    else if (item.VerifyRMF("DRM_OS2FILE", NULL))
    { // fetch full qualified path
      xstring fullname = item.SourcePath();
      if (fullname)
      { // have file name => use target rendering
        DEBUGLOG(("GUIImp::DragDrop: DRM_OS2FILE && fullname - %x\n", item->fsControl));
        if (item.VerifyType("UniformResourceLocator"))
        { // have file name => use target rendering
          fullname = amp_url_from_file(fullname);
          if (!fullname)
            continue;
        } else if (fullname.endsWithI(".skn"))
        { // Dropped skin file
          Cfg::ChangeAccess().defskin = fullname;
          dt.EndConversation();
          continue;
        }
        // Hopefully this criterion is sufficient to identify folders.
        if (item->fsControl & DC_CONTAINER)
          fullname = amp_make_dir_url(fullname, Cfg::Get().recurse_dnd); // TODO: should be alterable

        const url123& url = url123::normalizeURL(fullname);
        DEBUGLOG(("GUIImp::DragDrop: url=%s\n", url.cdata()));
        if (url)
        { worker->Loader.AddItem(*Playable::GetByURL(url));
          dt.EndConversation();
        }
      }
      else if (item->hwndItem && item.VerifyType("UniformResourceLocator"))
      { // URL w/o file name => need source rendering
        DEBUGLOG(("GUIImp::DragDrop: DRM_OS2FILE && URL && !fullname - %x\n", item->fsControl));
        dt.SelectedRMF("<DRM_OS2FILE,DRF_TEXT>");
        dt.RenderTo(amp_dnd_temp_file().cdata());
      }
    }
  } // foreach pditem
}

/* Receives dropped and rendered files and urls. */
void GUIImp::DropRenderComplete(PDRAGTRANSFER pdtrans_, USHORT flags)
{ DEBUGLOG(("GUIImp::DropRenderComplete(%p, %x)\n", pdtrans_, flags));
  UseDragTransfer dt(pdtrans_, true);
  DEBUGLOG(("GUIImp::DropRenderComplete({, %x, %p{...}, %s, %s, %p, %i, %x}, )\n",
    dt->hwndClient, dt->pditem, dt.SelectedRMF().cdata(), dt.RenderToName().cdata(),
    dt->ulTargetInfo, dt->usOperation, dt->fsReply));

  if (!(flags & DMFL_RENDEROK))
    return;

  int_ptr<HandleDrop> worker((HandleDrop*)dt.Worker());
  if (dt.VerifySelectedRMF("<DRM_OS2FILE,DRF_TEXT>"))
  { // URL-File => read content and insert URL into playlist.
    const xstring& url = url123::normalizeURL(amp_url_from_file(dt.RenderToName()));
    if (!url)
      return;
    int_ptr<Playable> pp = Playable::GetByURL(url);
    if (!pp)
      return;

    APlayable* ap = pp;
    if (dt->pditem->hstrTargetName)
    { ap = new PlayableRef(*pp);
      ItemInfo item;
      item.alias = dt.Item().TargetName();
      ((PlayableRef*)ap)->OverrideItem(&item);
    }
    worker->Loader.AddItem(*ap);

    // Remove temp file
    remove(dt.RenderToName());

    dt.EndConversation(true);
  }
  else if (dt.VerifySelectedRMF("<DRM_123LST,DRF_UNKNOWN>"))
  { // PM123 temporary playlist to insert.
    const xstring& url = url123::normalizeURL(dt.RenderToName());
    if (!url)
      return;
    int_ptr<Playable> pp = Playable::GetByURL(url);
    if (!pp)
      return;

    if (pp->RequestInfo(IF_Child, PRI_Normal, flags & 0x8000 ? REL_Cached : REL_Reload))
    { // Info not available synchronously => post DM_RENDERCOMPLETE later, when the infos arrived.
      DEBUGLOG(("GUIImp::DropRenderComplete: delayed\n"));
      dt.Hold();
      AutoPostMsgWorker::Start(*pp, IF_Child, PRI_Normal,
        HPlayer, DM_RENDERCOMPLETE, MPFROMP(pdtrans_), MPFROMSHORT(flags|0x8000));
    } else
    { // Read playlist
      DEBUGLOG(("GUIImp::DropRenderComplete: execute\n"));
      int_ptr<PlayableInstance> pi;
      while ((pi = pp->GetNext(pi)) != NULL)
        worker->Loader.AddItem(*pi);

      // Remove temp file
      #ifndef DEBUG // keep in case of debug builds
      remove(url.cdata() + 8);
      #endif

      // Never tell the source success, because that could cause deletions with DO_MOVE.
    }
  }
}


BOOL EXPENTRY GUI_HelpHook(HAB hab, ULONG usMode, ULONG idTopic, ULONG idSubTopic, PRECTL prcPosition)
{ DEBUGLOG(("HelpHook(%p, %x, %x, %x, {%li,%li, %li,%li})\n", hab,
    usMode, idTopic, idSubTopic, prcPosition->xLeft, prcPosition->yBottom, prcPosition->xRight, prcPosition->yTop));
  return FALSE;
}

void GUIImp::Init()
{ DEBUGLOG(("GUIImp::Init()\n"));

  CurrentIter = Ctrl::GetLoc();
  ASSERT(CurrentIter);

  InitButton(amp_player_hab);

  // these two are always constant
  LoadWizards[0] = amp_file_wizard;
  LoadWizards[1] = amp_url_wizard;

  PMRASSERT(WinRegisterClass(amp_player_hab, "PM123", &GUI_DlgProcStub, CS_SIZEREDRAW, 0));
  ULONG flCtlData = FCF_TASKLIST|FCF_NOBYTEALIGN|FCF_ICON;
  HFrame = WinCreateStdWindow(HWND_DESKTOP, 0, &flCtlData, "PM123",
                              AMP_FULLNAME, 0, NULLHANDLE, WIN_MAIN, &HPlayer);
  PMASSERT(HFrame != NULLHANDLE);
  DEBUGLOG(("GUIImp::Init: window created: frame = %x, player = %x\n", HFrame, HPlayer));

  // Init skin
  bmp_load_skin(xstring(Cfg::Get().defskin), HPlayer);

  // Init default lists
  { const url123& path = url123::normalizeURL(amp_basepath);
    DefaultPL = Playable::GetByURL(path + "PM123.LST");
    DefaultPL->SetAlias("Default Playlist");
    DefaultPM = Playable::GetByURL(path + "PFREQ.LST");
    DefaultPM->SetAlias("Playlist Manager");
    DefaultBM = Playable::GetByURL(path + "BOOKMARK.LST");
    DefaultBM->SetAlias("Bookmarks");
    LoadMRU   = Playable::GetByURL(path + "LOADMRU.LST");
    UrlMRU    = Playable::GetByURL(path + "URLMRU.LST");
    // The default playlist the bookmarks and the MRU list must be ready to use
    DefaultPL->RequestInfo(IF_Child, PRI_Normal);
    DefaultBM->RequestInfo(IF_Child, PRI_Normal);
    LoadMRU->RequestInfo(IF_Child, PRI_Normal);
    UrlMRU->RequestInfo(IF_Child, PRI_Normal);
  }

  // Keep track of configuration changes.
  Cfg::GetChange() += ConfigDeleg;

  // Keep track of plug-in changes.
  Plugin::GetChangeEvent() += PluginDeleg;
  PMRASSERT( WinPostMsg(HFrame, WMP_REFRESH_ACCEL, 0, 0 )); // load accelerators

  // register control event handler
  Ctrl::GetChangeEvent() += ControllerDeleg;

  // Init help manager
  xstring infname(amp_startpath + "pm123.inf");
  struct stat fi;
  if( stat( infname, &fi ) != 0  )
    // If the file of the help does not placed together with the program,
    // we shall give to the help manager to find it.
    infname = "pm123.inf";

  HELPINIT hinit = { sizeof( hinit ) };
  hinit.phtHelpTable = (PHELPTABLE)MAKELONG( HLP_MAIN, 0xFFFF );
  hinit.pszHelpWindowTitle = "PM123 Help";
  #ifdef DEBUG
  hinit.fShowPanelId = CMIC_SHOW_PANEL_ID;
  #else
  hinit.fShowPanelId = CMIC_HIDE_PANEL_ID;
  #endif
  hinit.pszHelpLibraryName = (PSZ)infname.cdata();

  HHelp = WinCreateHelpInstance(amp_player_hab, &hinit);
  if( HHelp == NULLHANDLE )
    amp_messagef(HFrame, MSG_ERROR, "Error create help instance: %s", infname.cdata());
  else
    PMRASSERT(WinAssociateHelpInstance(HHelp, HPlayer));

  WinSetHook(amp_player_hab, HMQ_CURRENT, HK_HELP, (PFN)&GUI_HelpHook, NULLHANDLE);

  // Docking...
  dk_init();
  dk_add_window(HFrame, DK_IS_MASTER);
  if (Cfg::Get().dock_windows)
    dk_arrange(HFrame);

  // initialize visual plug-ins
  vis_InitAll(HPlayer);

  DEBUGLOG(("GUIImp::Init: complete\n"));
}

void GUIImp::Uninit()
{ DEBUGLOG(("GUIImp::Uninit()\n"));

  PlaylistManager::DestroyAll();
  PlaylistView::DestroyAll();
  InspectorDialog::UnInit();

  CurrentDeleg.detach();
  RootDeleg.detach();
  ControllerDeleg.detach();
  PluginDeleg.detach();
  ConfigDeleg.detach();
  // deinitialize all visual plug-ins
  vis_UninitAll();

  delete MenuWorker;

  if (HHelp != NULLHANDLE)
    WinDestroyHelpInstance(HHelp);

  // Eat user messages from the queue
  // Causes memory leaks, but who cares on termination.
  QMSG   qmsg;
  while (WinPeekMsg(amp_player_hab, &qmsg, HFrame, WM_USER, 0xbfff, PM_REMOVE));

  AutoSave();

  dk_term();
  WinDestroyWindow(HFrame);
  bmp_clean_skin();

  UrlMRU    = NULL;
  LoadMRU   = NULL;
  DefaultBM = NULL;
  DefaultPM = NULL;
  DefaultPL = NULL;
}

/****************************************************************************
*
*  class GUI
*
****************************************************************************/

int_ptr<Playable> GUI::DefaultPL;
int_ptr<Playable> GUI::DefaultPM;
int_ptr<Playable> GUI::DefaultBM;
int_ptr<Playable> GUI::LoadMRU;
int_ptr<Playable> GUI::UrlMRU;

HWND              GUI::HFrame  = NULLHANDLE;
HWND              GUI::HPlayer = NULLHANDLE;
HWND              GUI::HHelp   = NULLHANDLE;


/* Constructs a string of the displayable text from the file information. */
const xstring GUI::ConstructTagString(const volatile META_INFO& meta)
{
  unsigned maxlen = 0;
  { const volatile amp_cfg& cfg = Cfg::Get();
    if (cfg.restrict_meta)
      maxlen = cfg.restrict_length;
  }

  xstringbuilder result;

  xstring tmp1(meta.artist);
  xstring tmp2(meta.title);
  if (tmp1 && *tmp1)
  { append_restricted(result, tmp1, maxlen);
    if (tmp2 && *tmp2)
      result += ": ";
  }
  if (tmp2 && *tmp2)
    append_restricted(result, tmp2, maxlen);

  tmp1 = meta.album;
  tmp2 = meta.year;
  if (tmp1 && *tmp1)
  { result += " (";
    append_restricted(result, tmp1, maxlen);
    if (tmp2 && *tmp2)
      result.appendf(", %s)", tmp2.cdata());
    else
      result += ')';
  } else if (tmp2 && *tmp2)
    result.appendf(" (%s)", tmp2.cdata());

  tmp1 = meta.comment;
  if (tmp1)
  { result += " -- ";
    append_restricted(result, tmp1, maxlen);
  }

  return result;
}

void GUI::Add2MRU(Playable& list, size_t max, APlayable& ps)
{ DEBUGLOG(("GUI::Add2MRU(&%p{%s}, %u, %s)\n", &list, list.DebugName().cdata(), max, ps.GetPlayable().URL.cdata()));
  list.RequestInfo(IF_Child, PRI_Sync|PRI_Normal);
  Mutex::Lock lock(list.Mtx);
  int_ptr<PlayableInstance> pi;
  // remove the desired item from the list and limit the list size
  while ((pi = list.GetNext(pi)) != NULL)
  { DEBUGLOG(("GUI::Add2MRU - %p{%s}\n", pi.get(), pi->DebugName().cdata()));
    if (max == 0 || pi->GetPlayable() == ps.GetPlayable()) // Instance equality of Playable is sufficient
      list.RemoveItem(*pi);
     else
      --max;
  }
  // prepend list with new item
  list.InsertItem(ps, list.GetNext(NULL));
}

/*void GUI::RefreshTotals(HPS hps)
{	DEBUGLOG(("GUI::RefreshTotals()\n"));
  // Currently a NOP
}*/

void GUI::PostMessage(MESSAGE_TYPE type, xstring text)
{ if (text)
    WinPostMsg(HPlayer, WMP_DISPLAY_MESSAGE, MPFROMP(text.toCstr()), MPFROMLONG(type));
}

bool GUI::Show(DialogBase::DialogAction action)
{ SWP swp;
  PMRASSERT(WinQueryWindowPos(HFrame, &swp));
  switch (action)
  {case DialogBase::DLA_SHOW:
    Cfg::RestWindowPos(HFrame);
    //WinSetWindowPos(HFrame, HWND_TOP,
    //                cfg.main.x, cfg.main.y, 0, 0, SWP_ACTIVATE|SWP_MOVE|SWP_SHOW);
    WinSetWindowPos(HFrame, HWND_TOP, 0,0, 0,0, SWP_ACTIVATE|SWP_SHOW);
    break;
   case DialogBase::DLA_CLOSE:
    WinSendMsg(HPlayer, WM_COMMAND, MPFROMSHORT(IDM_M_MINIMIZE), 0);
   default:; // avoid warning
  }
  return !(swp.fl & SWP_HIDE);
}

void GUI::Load(LoadHelper& lhp)
{ GUIImp::Load(lhp);
}

/*struct dialog_show;
static void amp_dlg_event(dialog_show* iep, const PlayableChangeArgs& args);

struct dialog_show
{ HWND              Owner;
  int_ptr<Playable> Item;
  GUI::DialogType   Type;
  //delegate<dialog_show, const PlayableChangeArgs> InfoDelegate;
  dialog_show(HWND owner, Playable& item, GUI::DialogType type)
  : Owner(owner)
  , Item(&item)
  , Type(type)
  //, InfoDelegate(item->InfoChange, &amp_dlg_event, this)
  {}
};*/

/*static void amp_dlg_event(dialog_show* iep, const PlayableChangeArgs& args)
{ if (args.Changed & IF_Other)
  { iep->InfoDelegate.detach();
    PMRASSERT(WinPostMsg(hframe, AMP_SHOW_DIALOG, iep, 0));
  }
}*/

bool GUI::ShowDialog(APlayable& item, DialogType dlg, DialogBase::DialogAction action)
{ DEBUGLOG(("GUI::ShowDialog(&%p, %u, %u)\n", &item, dlg, action));
  int_ptr<APlayable> pp(&item);
  return (bool)LONGFROMMR(WinSendMsg(HFrame, WMP_SHOW_DIALOG, MPFROMP(pp.toCptr()), MPFROM2SHORT(dlg, action)));
}

bool GUI::ShowConfig(Module& plugin, DialogBase::DialogAction action)
{ DEBUGLOG(("GUI::ShowConfig(&%p, %u)\n", &plugin, action));
  int_ptr<Module> pm(&plugin);
  return (bool)LONGFROMMR(WinSendMsg(HFrame, WMP_SHOW_CONFIG, MPFROMP(pm.toCptr()), MPFROMSHORT(action)));
}

void GUI::Init()
{ GUIImp::Init();
}

void GUI::Uninit()
{ GUIImp::Uninit();
}

