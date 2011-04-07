/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2010 M.Mueller
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
#include "dialog.h"
#include "filedlg.h"
#include "properties.h"
#include "pm123.h"
#include "pm123.rc.h"
#include "123_util.h"
#include "glue.h"
#include "skin.h"
#include "docking.h"
#include "infodialog.h"
#include "playlistview.h"
#include "playlistmanager.h"
#include "inspector.h"
#include "waitinfo.h"
#include "button95.h"
#include "copyright.h"

#include <utilfct.h>
#include <cpp/pmutils.h>
#include <cpp/showaccel.h>

#include <sys/stat.h>
#include <math.h>
#include <stdio.h>


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
bool              GUI::Terminate = false;
DECODER_WIZARD_FUNC GUI::LoadWizards[16];
SongIterator      GUI::IterBuffer[2];
HWND              GUI::ContextMenu = NULLHANDLE;
PlaylistMenu*     GUI::MenuWorker  = NULL;

SongIterator*     GUI::CurrentIter = IterBuffer;
bool              GUI::IsLocMsg        = false;
GUI::UpdateFlags  GUI::UpdFlags        = UPD_NONE;
GUI::UpdateFlags  GUI::UpdAtLocMsg     = UPD_NONE;
bool              GUI::IsHaveFocus     = false;
int               GUI::IsSeeking       = 0;
bool              GUI::IsVolumeDrag    = false;
bool              GUI::IsSliderDrag    = false;
bool              GUI::IsAltSlider     = false;
bool              GUI::IsAccelChanged  = false;
bool              GUI::IsPluginChanged = false;

delegate<const void, const Plugin::EventArgs>  GUI::PluginDeleg    (&GUI::PluginNotification,     NULL);
delegate<const void, const Ctrl::EventFlags>   GUI::ControllerDeleg(&GUI::ControllerNotification, NULL);
delegate<const void, const PlayableChangeArgs> GUI::RootDeleg      (&GUI::PlayableNotification,   NULL);
delegate<const void, const PlayableChangeArgs> GUI::CurrentDeleg   (&GUI::PlayableNotification,   NULL);


class PostMsgWorker : public DependencyInfoWorker
{private:
  HWND            Target;
  ULONG           Msg;
  MPARAM          MP1;
  MPARAM          MP2;
 public:
  void            Start(DependencyInfoSet& data, HWND target, ULONG msg, MPARAM mp1, MPARAM mp2);
 private:
  virtual void    OnCompleted();
};

void PostMsgWorker::Start(DependencyInfoSet& data, HWND target, ULONG msg, MPARAM mp1, MPARAM mp2)
{ Cancel();
  Data.Swap(data);
  Target = target;
  Msg = msg;
  MP1 = mp1;
  MP2 = mp2;
  DependencyInfoWorker::Start();
}

void PostMsgWorker::OnCompleted()
{ WinPostMsg(Target, Msg, MP1, MP2);
}


PostMsgWorker DelayedAltSliderDragWorker;


MRESULT EXPENTRY GUI_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ // Adjust calling convention
  if (msg == WM_CREATE)
    GUI::HPlayer = hwnd; // we have to assign the window handle early, because WinCreateStdWindow does not have returned now.
  return GUI::DlgProc(msg, mp1, mp2);
}

MRESULT GUI::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("GUI::DlgProc(%p, %p, %p)\n", msg, mp1, mp2));
  switch (msg)
  { case WM_CREATE:
    { DEBUGLOG(("GUI::DlgProc: WM_CREATE\n"));

      WinPostMsg(HPlayer, WMP_RELOADSKIN, 0, 0);

      HAB hab = amp_player_hab();
      if (cfg.floatontop)
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

   case WMP_LOAD:
    { DEBUGLOG(("GUI::DlgProc: WMP_LOAD %p\n", mp1));
      LoadHelper* lhp = (LoadHelper*)mp1;
      Ctrl::ControlCommand* cmd = lhp->ToCommand();
      if (cmd)
      { cmd->Callback = &GUI::ControllerEventCB;
        Ctrl::PostCommand(cmd);
      }
      delete lhp;
      return 0;
    }

   case WMP_SHOW_DIALOG:
    { //dialog_show* iep = (dialog_show*)PVOIDFROMMP(mp1);
      int_ptr<Playable> pp;
      pp.fromCptr((Playable*)mp1);
      DialogType type = (DialogType)SHORT1FROMMP(mp2);
      DEBUGLOG(("GUI::DlgProc: WMP_SHOW_DIALOG %p, %u\n", pp.get(), type));
      switch (type)
      { case DLT_INFOEDIT:
        { // TODO: ensure Decoder availability
          ULONG rc = dec_editmeta(HFrame, pp->URL, xstring(pp->GetInfo().tech->decoder));
          DEBUGLOG(("GUI::DlgProc: WMP_SHOW_DIALOG rc = %u\n", rc));
          switch (rc)
          { default:
              amp_error(HFrame, "Cannot edit tag of file:\n%s", pp->URL.cdata());
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
              amp_error(HFrame, "Unable write tag to file:\n%s\n%s", pp->URL.cdata(), clib_strerror(errno));
              break;
          }
          break;
        }
        case DLT_METAINFO:
          AInfoDialog::GetByKey(*pp)->ShowPage(AInfoDialog::Page_MetaInfo);
          break;
        case DLT_TECHINFO:
          AInfoDialog::GetByKey(*pp)->ShowPage(AInfoDialog::Page_TechInfo);
          break;
        case DLT_PLAYLIST:
          PlaylistView::Get(*pp)->SetVisible(true);
          break;
        case DLT_PLAYLISTTREE:
          PlaylistManager::Get(*pp)->SetVisible(true);
          break;
      }
      return 0;
    }

   case WMP_DISPLAY_MESSAGE:
    { xstring text;
      text.fromCstr((const char*)mp1); 
      if( mp2 ) {
        amp_error(HPlayer, "%s", text.cdata() );
      } else {
        amp_info (HPlayer, "%s", text.cdata() );
      }
      return 0;
    }
   case WMP_DISPLAY_MODE:
      if (cfg.viewmode == CFG_DISP_FILEINFO) {
        cfg.viewmode = CFG_DISP_FILENAME;
      } else {
        cfg.viewmode++;
      }
      Invalidate(UPD_TEXT, true);
      return 0;

   case WMP_RELOADSKIN:
    { PresentationSpace hps(HPlayer);
      bmp_load_skin(cfg.defskin, HPlayer, hps);
      Invalidate(UPD_WINDOW, true);
      return 0;
    }

   case WMP_CTRL_EVENT_CB:
    { Ctrl::ControlCommand* cmd = (Ctrl::ControlCommand*)PVOIDFROMMP(mp1);
      DEBUGLOG(("GUI::DlgProc: AMP_CTRL_EVENT_CB %p{%i, %x}\n", cmd, cmd->Cmd, cmd->Flags));
      switch (cmd->Cmd)
      {case Ctrl::Cmd_Load:
        // Successful? if not display error.
        if (cmd->Flags != Ctrl::RC_OK)
          amp_player_error("Error loading %s\n%s", cmd->StrArg.cdata(), xstring(Playable::GetByURL(cmd->StrArg)->GetInfo().tech->info).cdata());
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
        Invalidate(UPD_VOLUME, true);
        break;
       case Ctrl::Cmd_Jump:
        IsSeeking = FALSE;
        delete (SongIterator*)cmd->PtrArg;
        ForceLocationMsg();
        break;
       case Ctrl::Cmd_Save:
        if (cmd->Flags != Ctrl::RC_OK)
          amp_player_error( "The current active decoder don't support saving of a stream.\n" );
        break;
       case Ctrl::Cmd_Location:
        { IsLocMsg = false;
          if (IsSeeking) // do not overwrite location while seeking
              break;
          // Update CurrentIter
          if (cmd->Flags == Ctrl::RC_OK)
            CurrentIter = (SongIterator*)cmd->PtrArg;
          // Redraws
          UpdateFlags upd = UpdAtLocMsg;
          UpdAtLocMsg = UPD_NONE;
          if (cfg.mode == CFG_MODE_REGULAR)
              upd |= UPD_TIMERS;
          Invalidate(upd, true);
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
      DEBUGLOG(("GUI::DlgProc: AMP_CTRL_EVENT %x\n", flags));
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
          inval |= UPD_WINDOW;
        }
      }
      if (flags & Ctrl::EV_Pause)
        WinSendDlgItemMsg(HPlayer, BMP_PAUSE,   Ctrl::IsPaused() ? WM_PRESS : WM_DEPRESS, 0, 0);
      if (flags & Ctrl::EV_Forward)
        WinSendDlgItemMsg(HPlayer, BMP_FWD,     Ctrl::GetScan() & DECFAST_FORWARD ? WM_PRESS : WM_DEPRESS, 0, 0);
      if (flags & Ctrl::EV_Rewind)
        WinSendDlgItemMsg(HPlayer, BMP_REW,     Ctrl::GetScan() & DECFAST_REWIND  ? WM_PRESS : WM_DEPRESS, 0, 0);
      if (flags & Ctrl::EV_Shuffle)
        WinSendDlgItemMsg(HPlayer, BMP_SHUFFLE, Ctrl::IsShuffle() ? WM_PRESS : WM_DEPRESS, 0, 0);
      if (flags & Ctrl::EV_Repeat)
      { WinSendDlgItemMsg(HPlayer, BMP_REPEAT,  Ctrl::IsRepeat() ? WM_PRESS : WM_DEPRESS, 0, 0);
        inval |= UPD_TOTALS;
      }
      if (flags & (Ctrl::EV_Root|Ctrl::EV_Song))
      { int_ptr<APlayable> root = Ctrl::GetRoot();
        if (flags & Ctrl::EV_Root)
        { // New root => attach observer
          RootDeleg.detach();
          DEBUGLOG(("GUI::DlgProc: AMP_CTRL_EVENT new root: %p\n", root.get()));
          if (root)
            root->GetInfoChange() += RootDeleg;
          UpdAtLocMsg |= UPD_TIMERS|UPD_TOTALS|UPD_PLMODE|UPD_PLINDEX|UPD_RATE|UPD_CHANNELS|UPD_TEXT;
        }
        // New song => attach observer
        CurrentDeleg.detach();
        int_ptr<APlayable> song = Ctrl::GetCurrentSong();
        DEBUGLOG(("GUI::DlgProc: AMP_CTRL_EVENT new song: %p\n", song.get()));
        if (song && song != root) // Do not observe song items twice.
          song->GetInfoChange() += CurrentDeleg;
        // Refresh display text
        UpdAtLocMsg |= UPD_TIMERS|UPD_PLINDEX|UPD_RATE|UPD_CHANNELS|UPD_TEXT;
        ForceLocationMsg();
      }

      if (flags & Ctrl::EV_Volume)
        inval |= UPD_VOLUME;

      if (inval)
        Invalidate(inval, true);
    }
    return 0;
    
   case WMP_PLAYABLE_EVENT:
    { Playable* root = CurrentRoot();
      DEBUGLOG(("GUI::DlgProc: WMP_PLAYABLE_EVENT %p %x\n", mp1, SHORT1FROMMP(mp2)));
      if (!root)
        return 0;
      APlayable* ap = (APlayable*)mp1;
      InfoFlags changed = (InfoFlags)SHORT1FROMMP(mp2);
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
        if (changed & IF_Obj)
          upd |= UPD_TIMERS;
        if (changed & IF_Rpl)
        { upd |= UPD_TOTALS;
          // TODO: needed? ForceLocationMsg();
        }
        if (changed & IF_Tech)
          upd |= UPD_PLMODE;
      }
      if (&CurrentIter->GetCurrent() == ap)
      { // Current change event
        if (changed & IF_Obj)
          upd |= UPD_TIMERS|UPD_RATE;
        if (changed & IF_Tech)
          upd |= cfg.viewmode == CFG_DISP_FILEINFO ? UPD_CHANNELS|UPD_TEXT : UPD_CHANNELS;
        if (changed & IF_Meta && cfg.viewmode == CFG_DISP_ID3TAG)
          upd |= UPD_TEXT;
      }

      switch (cfg.mode)
      {case	CFG_MODE_TINY:
        return 0; // No updates in tiny mode
       case CFG_MODE_SMALL:
        upd &= UPD_TEXT; // reduced update in small mode
      }
      Invalidate(upd, true);
    }
    return 0;
    
   case WMP_REFRESH_ACCEL:
    { // eat other identical messages
      QMSG qmsg;
      while (WinPeekMsg(amp_player_hab(), &qmsg, HFrame, WMP_REFRESH_ACCEL, WMP_REFRESH_ACCEL, PM_REMOVE));
    }
    LoadAccel();
    return 0;

   case WMP_PAINT:
    if (!Terminate)
    { UpdateFlags mask = (UpdateFlags)LONGFROMMP(mp1) & UpdFlags;
      UpdFlags &= ~mask;
      // is there anything to draw with HPS?
      if (mask & ~UPD_WINDOW)
      { // TODO: optimize redundant redraws?
        PresentationSpace hps(HPlayer);
        Playable* root = CurrentRoot();
        if (mask & UPD_TIMERS)
          RefreshTimers(hps);
        if (mask & UPD_TOTALS|UPD_PLINDEX)
        { int index = 0; // TODO: is_playlist ? index+1 : 0;
          bmp_draw_plind(hps, index, root ? root->GetInfo().rpl->songs: 0);
        }
        if (mask & UPD_PLMODE)
          bmp_draw_plmode(hps, root != NULL, root && root->IsPlaylist());
        if (mask & UPD_RATE)
          bmp_draw_rate(hps, root ? CurrentIter->GetCurrent().GetInfo().obj->bitrate : -1);
        if (mask & UPD_CHANNELS)
          bmp_draw_channels(hps, root ? CurrentIter->GetCurrent().GetInfo().tech->channels : -1);
        if (mask & UPD_VOLUME)
          bmp_draw_volume(hps, Ctrl::GetVolume());
        if (mask & UPD_TEXT)
        { PrepareText();
          bmp_draw_text(hps);
        }
      }
      if (mask & UPD_WINDOW)
        WinInvalidateRect(HPlayer, NULL, 1);
    }
    return 0;

   case WM_CONTEXTMENU:
    ShowContextMenu();
    return 0;

   case WM_INITMENU:
    if (SHORT1FROMMP(mp1) == IDM_M_PLAYBACK)
    { HWND menu = HWNDFROMMP(mp2);
      mn_check_item(menu, BMP_PLAY,    Ctrl::IsPlaying() );
      mn_check_item(menu, BMP_PAUSE,   Ctrl::IsPaused()  );
      mn_check_item(menu, BMP_REW,     Ctrl::GetScan() == DECFAST_REWIND  );
      mn_check_item(menu, BMP_FWD,     Ctrl::GetScan() == DECFAST_FORWARD );
      mn_check_item(menu, BMP_SHUFFLE, Ctrl::IsShuffle() );
      mn_check_item(menu, BMP_REPEAT,  Ctrl::IsRepeat()  );
    }
    break;

   case WM_TIMER:
    DEBUGLOG2(("GUI::DlgProc: WM_TIMER - %x\n", LONGFROMMP(mp1)));
    switch (LONGFROMMP(mp1))
    {case TID_ONTOP:
      WinSetWindowPos(HFrame, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
      DEBUGLOG2(("GUI::DlgProc: WM_TIMER done\n"));
      return 0;

     case TID_UPDATE_PLAYER:
      { if( bmp_scroll_text())
        { PresentationSpace hps(HPlayer);
          bmp_draw_text(hps);
        }
      }
      DEBUGLOG2(("GUI::DlgProc: WM_TIMER done\n"));
      return 0;

     case TID_UPDATE_TIMERS:
      if (Ctrl::IsPlaying())
      // We must not mask this message request further, because the controller relies on that polling
      // to update it's internal status.
         ForceLocationMsg();
      DEBUGLOG2(("GUI::DlgProc: WM_TIMER done\n"));
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

   case WM_PAINT:
    {
      HPS hps = WinBeginPaint(HPlayer, NULLHANDLE, NULL);
      bmp_draw_background(hps, HPlayer);
      if (!Terminate)
      {
        Playable* root = CurrentRoot();
        RefreshTimers(hps);
        int index = 0; // TODO: is_playlist ? index+1 : 0;
        bmp_draw_plind(hps, index, root ? root->GetInfo().rpl->songs: 0);
        bmp_draw_plmode(hps, root != NULL, root && root->IsPlaylist());
        bmp_draw_rate(hps, root ? CurrentIter->GetCurrent().GetInfo().obj->bitrate : -1);
        bmp_draw_channels(hps, root ? CurrentIter->GetCurrent().GetInfo().tech->channels : -1);
        bmp_draw_volume(hps, Ctrl::GetVolume());
        bmp_draw_text(hps);
      }
      bmp_draw_led(hps, IsHaveFocus);
      WinEndPaint( hps );
      return 0;
    }

   case WM_COMMAND:
    { USHORT cmd = SHORT1FROMMP(mp1);
      DEBUGLOG(("GUI::DlgProc: WM_COMMAND(%u, %u, %u)\n", cmd, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      if (cmd > IDM_M_PLUG && cmd <= IDM_M_PLUG_E)
      { cmd -= IDM_M_PLUG+1;
        // atomic plug-in request
        Module* plug;
        { const Module::IXAccess ix;
          if (cmd >= ix->size())
            return 0;
          plug = (*ix)[cmd];
        }
        plug->Config(HPlayer);
        return 0;
      }
      if ( cmd >= IDM_M_LOADFILE && cmd < IDM_M_LOADFILE + sizeof LoadWizards / sizeof *LoadWizards
        && LoadWizards[cmd-IDM_M_LOADFILE] )
      { // TODO: create temporary playlist
        LoadHelper lh(cfg.playonload*LoadHelper::LoadPlay | LoadHelper::LoadRecall);
        if ((*LoadWizards[cmd-IDM_M_LOADFILE])(HPlayer, "Load%s", &GUI_LoadFileCallback, &lh) == 0)
          Ctrl::PostCommand(lh.ToCommand(), &GUI::ControllerEventCB);
        return 0;
      }

      switch( cmd )
      {
        case IDM_M_ADDBOOK:
          if (CurrentRoot())
            amp_add_bookmark(HPlayer, CurrentSong());
          break;

        case IDM_M_ADDBOOK_TIME:
          if (CurrentRoot())
          { PlayableRef ps(CurrentSong());
            // Use the time index from last_status here.
            if (CurrentIter->GetPosition() >= 0)
            { AttrInfo attr(*ps.GetInfo().attr);
              attr.at = PM123Time::toString(CurrentIter->GetPosition());
              ps.OverrideAttr(&attr);
            }
            amp_add_bookmark(HPlayer, ps);
          }
          break;

        case IDM_M_ADDPLBOOK:
          if (CurrentRoot())
            amp_add_bookmark(HPlayer, *CurrentIter->GetRoot());
          break;

        case IDM_M_ADDPLBOOK_TIME:
          if (CurrentRoot())
          { PlayableRef ps(*CurrentIter->GetRoot());
            AttrInfo attr(*ps.GetInfo().attr);
            attr.at = CurrentIter->Serialize(true);
            ps.OverrideAttr(&attr);
            amp_add_bookmark(HPlayer, ps);
          }
          break;

        case IDM_M_EDITBOOK:
          PlaylistView::Get(*DefaultBM, "Bookmarks")->SetVisible(true);
          break;

        case IDM_M_EDITBOOKTREE:
          PlaylistManager::Get(*DefaultBM, "Bookmark Tree")->SetVisible(true);
          break;

        case IDM_M_INFO:
          if (CurrentRoot())
            ShowDialog(CurrentSong().GetPlayable(), DLT_METAINFO);
          break;

        case IDM_M_PLINFO:
          if (CurrentRoot())
            ShowDialog(*CurrentRoot(),
              CurrentRoot()->GetInfo().tech->attributes & TATTR_SONG ? DLT_METAINFO : DLT_TECHINFO);
          break;

        case IDM_M_TAG:
          if (CurrentRoot())
            ShowDialog(CurrentSong().GetPlayable(), DLT_INFOEDIT);
          break;

        case IDM_M_RELOAD:
          if (CurrentRoot())
            CurrentSong().RequestInfo(IF_Decoder, PRI_Normal, REL_Reload);
          break;

        case IDM_M_DETAILED:
        { Playable* root = CurrentRoot();
          if (root && root->IsPlaylist())
            ShowDialog(*root, DLT_PLAYLIST);
          break;
        }
        case IDM_M_TREEVIEW:
        { Playable* root = CurrentRoot();
          if (root && root->IsPlaylist())
            ShowDialog(*root, DLT_PLAYLISTTREE);
          break;
        }
        case IDM_M_ADDPMBOOK:
          if (CurrentRoot())
            DefaultPM->InsertItem(*CurrentRoot());
          break;

        case IDM_M_PLRELOAD:
        { Playable* root = CurrentRoot();
          if ( root && root->IsPlaylist()
            && (root->IsModified() || amp_query(HPlayer, "The current list is modified. Discard changes?")) )
            root->RequestInfo(IF_Decoder, PRI_Normal, REL_Reload);
          break;
        }
        case IDM_M_PLSAVE:
        case IDM_M_PLSAVEAS:
        { Playable* root = CurrentRoot();
          if (root && root->IsPlaylist())
            amp_save_playlist(HPlayer, *root, cmd == IDM_M_PLSAVEAS);
          break;
        }
        case IDM_M_SAVE:
          // TODO: Save stream
          //amp_save_stream(HPlayer, !Ctrl::GetSavename());
          break;

        case IDM_M_PLAYLIST:
          PlaylistView::Get(*DefaultPL, "Default Playlist")->SetVisible(true);
          break;

        case IDM_M_MANAGER:
          PlaylistManager::Get(*DefaultPM, "Playlist Manager")->SetVisible(true);
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
          cfg.floatontop = !cfg.floatontop;
          if (!cfg.floatontop)
            WinStopTimer(amp_player_hab(), HPlayer, TID_ONTOP );
          else
            WinStartTimer(amp_player_hab(), HPlayer, TID_ONTOP, 100);
          break;

        case IDM_M_FONT1:
          cfg.font = 0;
          cfg.font_skinned = true;
          Invalidate(UPD_TEXT, false);
          break;

        case IDM_M_FONT2:
          cfg.font = 1;
          cfg.font_skinned = true;
          Invalidate(UPD_TEXT, false);
          break;

        case IDM_M_TINY:
          cfg.mode = CFG_MODE_TINY;
          bmp_reflow_and_resize(HFrame);
          break;

        case IDM_M_NORMAL:
          cfg.mode = CFG_MODE_REGULAR;
          bmp_reflow_and_resize(HFrame);
          break;

        case IDM_M_SMALL:
          cfg.mode = CFG_MODE_SMALL;
          bmp_reflow_and_resize(HFrame);
          break;

        case IDM_M_MINIMIZE:
          WinSetWindowPos(HFrame, HWND_DESKTOP, 0, 0, 0, 0, SWP_HIDE);
          WinSetActiveWindow(HWND_DESKTOP, WinQueryWindow(HPlayer, QW_NEXTTOP));
          break;

        case IDM_M_SKINLOAD:
          if (amp_loadskin());
            WinPostMsg(HFrame, WMP_RELOADSKIN, 0, 0);
          break;

        case IDM_M_CFG:
          cfg_properties(HPlayer);
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
        { Playable* root = CurrentRoot();
          int_ptr<PlaylistView> pv(root && root != DefaultPL && root->IsPlaylist()
                                   ? PlaylistView::Get(*root)
                                   : PlaylistView::Get(*DefaultPL, "Default Playlist") );
          pv->SetVisible(!pv->GetVisible());
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
          Ctrl::PostCommand(Ctrl::MkPlayStop(Ctrl::Op_Toggle), &GUI::ControllerEventCB);
          break;

        case BMP_PAUSE:
          Ctrl::PostCommand(Ctrl::MkPause(Ctrl::Op_Toggle), &GUI::ControllerEventCB);
          break;

        case BMP_FLOAD:
          return WinSendMsg(HPlayer, WM_COMMAND, MPFROMSHORT(IDM_M_LOADFILE), mp2);

        case BMP_STOP:
          Ctrl::PostCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
          // TODO: probably inclusive when the iterator is destroyed
          //pl_clean_shuffle();
          break;

        case BMP_NEXT:
          Ctrl::PostCommand(Ctrl::MkSkip(1, true), &GUI::ControllerEventCB);
          break;

        case BMP_PREV:
          Ctrl::PostCommand(Ctrl::MkSkip(-1, true), &GUI::ControllerEventCB);
          break;

        case BMP_FWD:
          Ctrl::PostCommand(Ctrl::MkScan(Ctrl::Op_Toggle), &GUI::ControllerEventCB);
          break;

        case BMP_REW:
          Ctrl::PostCommand(Ctrl::MkScan(Ctrl::Op_Toggle|Ctrl::Op_Rewind), &GUI::ControllerEventCB);
          break;
      }
      return 0;
    }

   case PlaylistMenu::UM_SELECTED:
    { DEBUGLOG(("GUI::DlgProc: UM_SELECTED(%p, %p)\n", mp1, mp2));
      // bookmark is selected
      LoadHelper* lhp = new LoadHelper(cfg.playonload*LoadHelper::LoadPlay);
      lhp->AddItem((APlayable*)PVOIDFROMMP(mp1));
      Load(lhp);
    }
    return 0;

   case WM_HELP:
    DEBUGLOG(("GUI::DlgProc: WM_HELP(%u, %u, %u)\n", SHORT1FROMMP(mp1), SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
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
      else if (CurrentRoot() && bmp_pt_in_text(pos) && msg == WM_BUTTON1DBLCLK)
        WinPostMsg(HPlayer, WMP_DISPLAY_MODE, 0, 0);
      return 0;
    }

   case WM_MOUSEMOVE:
    if (IsVolumeDrag)
    { POINTL pos;
      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);
      Ctrl::PostCommand(Ctrl::MkVolume(bmp_calc_volume(pos), false), &GUI::ControllerEventCB);
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
      { if (CurrentRoot() && (IsAltSlider
          ? CurrentRoot()->GetInfo().rpl->songs > 0
          : CurrentSong().GetInfo().obj->songlength > 0 ))
        { IsSliderDrag = true;
          IsSeeking    = true;
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
    WinQueryWindowPos(HFrame, &cfg.main);
    return 0;

   case WM_CHAR:
    { // Force heap dumps by the D key
      USHORT fsflags = SHORT1FROMMP(mp1);
      DEBUGLOG(("GUI::DlgProc: WM_CHAR: %x, %u, %u, %x, %x\n", fsflags,
        SHORT2FROMMP(mp1)&0xff, SHORT2FROMMP(mp1)>>8, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      if (fsflags & KC_VIRTUALKEY)
      { switch (SHORT2FROMMP(mp2))
        {case VK_ESC:
          if(!(fsflags & KC_KEYUP))
          { IsSliderDrag = false;
            IsSeeking    = false;
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
      DEBUGLOG(("GUI::DlgProc: WM_TRANSLATEACCEL: %x, %x, %x\n", pqmsg->msg, pqmsg->mp1, pqmsg->mp2));
      if (pqmsg->msg == WM_CHAR)
      { USHORT fsflags = SHORT1FROMMP(pqmsg->mp1);
        if (fsflags & KC_VIRTUALKEY)
        { switch (SHORT2FROMMP(pqmsg->mp2))
          {case VK_ALT:
           case VK_ALTGRAF:
           //case VK_CTRL:
            SetAltSlider(!(fsflags & KC_KEYUP) && CurrentRoot() && CurrentRoot()->IsPlaylist());
            break;
          }
        }
      }
      break;
    }

   case WMP_SLIDERDRAG:
    if (IsSliderDrag)
    { // Cancel delayed slider drag if any
      DelayedAltSliderDragWorker.Cancel();
      // get position
      POINTL pos;
      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);
      double relpos = bmp_calc_time(pos);
      JobSet job(PRI_Normal);
      Playable* root = CurrentRoot();
      if (!root)
        return 0;
      // adjust CurrentIter from pos
      if (!IsAltSlider)
      { // navigation within the current song
        if (CurrentSong().GetInfo().obj->songlength <= 0)
          return 0;
        CurrentIter->Navigate(relpos * CurrentSong().GetInfo().obj->songlength, job);
      } else switch (cfg.altnavig)
      {case CFG_ANAV_TIME:
        // Navigate only at the time scale
        if (root->GetInfo().drpl->totallength > 0)
        { CurrentIter->Reset();
          bool r = CurrentIter->Navigate(relpos * root->GetInfo().drpl->totallength, job);
          DEBUGLOG(("GUI::DlgProc: AMP_SLIDERDRAG: CFG_ANAV_TIME: %u\n", r));
          break;
        }
        // else if no total time is availabe use song time navigation
       case CFG_ANAV_SONG:
       case CFG_ANAV_SONGTIME:
        // navigate at song and optional time scale
        { const int num_items = root->GetInfo().rpl->songs;
          if (num_items <= 0)
            return 0;
          relpos *= num_items;
          relpos += 1.;
          if (relpos > num_items)
            relpos = num_items;
          CurrentIter->Reset();
          bool r = CurrentIter->NavigateCount((int)floor(relpos), TATTR_SONG, job);
          DEBUGLOG(("GUI::DlgProc: AMP_SLIDERDRAG: CFG_ANAV_SONG: %f, %u\n", relpos, r));
          // navigate within the song
          if (cfg.altnavig != CFG_ANAV_SONG && CurrentSong().GetInfo().obj->songlength > 0)
          { relpos -= floor(relpos);
            CurrentIter->Navigate(relpos * CurrentSong().GetInfo().obj->songlength, job);
          }
        }
      }
      // Did we have unfulfilled dependencies?
      job.Commit();
      if (job.AllDepends.Size())
      { // yes => reschedule the request later.
        DelayedAltSliderDragWorker.Start(job.AllDepends, HPlayer, WMP_SLIDERDRAG, mp1, mp2);
      } else if (SHORT1FROMMP(mp2))
      { // Set new position
        Ctrl::PostCommand(Ctrl::MkJump(new SongIterator(*CurrentIter)), &GUI::ControllerEventCB);
        IsSliderDrag = false;
      } else
        // Show new position
        Invalidate(UPD_TIMERS|(UPD_TEXT|UPD_RATE|UPD_CHANNELS|UPD_PLINDEX)*IsAltSlider, true);
    }
    return 0;

   case WMP_ARRANGEDOCKING:
    if (cfg.dock_windows)
      dk_arrange(HPlayer); // TODO: frame window???
    else
      dk_cleanup(HPlayer);
    return 0;

   case DM_DRAGOVER:
    return DragOver((PDRAGINFO)mp1);
   case DM_DROP:
    return DragDrop((PDRAGINFO)mp1);
   case DM_RENDERCOMPLETE:
    return DragRenderDone((PDRAGTRANSFER)mp1, SHORT1FROMMP(mp2));
   case DM_DROPHELP:
    ShowHelp(IDH_DRAG_AND_DROP);
    // Continue in default procedure to free ressources.
    break;

    /*case WM_SYSCOMMAND:
      DEBUGLOG(("amp_dlg_proc: WM_SYSCVOMMAND: %u, %u, %u\n", SHORT1FROMMP(mp1), SHORT1FROMMP(mp2), SHORT2FROMMP(mp2)));
      break;*/
  }

  DEBUGLOG2(("GUI::DlgProc: before WinDefWindowProc\n"));
  return WinDefWindowProc(HPlayer, msg, mp1, mp2);
}

void GUI::Invalidate(UpdateFlags what, bool immediate)
{ DEBUGLOG(("GUI::Invalidate(%x, %u)\n", what, immediate));
  if (immediate && (what & UPD_WINDOW))
  { WinInvalidateRect(HPlayer, NULL, 1);
    what &= ~UPD_WINDOW;
  }
  if ((~UpdFlags & what) == 0)
    return; // Nothing to set
  UpdFlags |= what;
  if (immediate)
    WinPostMsg(HPlayer, WMP_PAINT, MPFROMLONG(what), 0);
}

/* set alternate navigation status to 'alt'. */
void GUI::SetAltSlider(bool alt)
{ DEBUGLOG(("GUI::SetAltSlider(%u) - %u\n", alt, IsAltSlider));
  if (IsAltSlider != alt)
  { IsAltSlider = alt;
    Invalidate(UPD_TIMERS, true);
  }
}

void GUI::ForceLocationMsg()
{ if (!IsLocMsg)
  { IsLocMsg = true;
    // Hack: the Location messages always writes to an iterator that is currently not in use by CurrentIter
    // to avoid threading problems.
    Ctrl::PostCommand(Ctrl::MkLocation(IterBuffer + (CurrentIter == IterBuffer), 0), &GUI::ControllerEventCB);
  }
}

void GUI::ControllerEventCB(Ctrl::ControlCommand* cmd)
{ DEBUGLOG(("GUI::ControllerEventCB(%p{%i, ...)\n", cmd, cmd->Cmd));
  WinPostMsg(HFrame, WMP_CTRL_EVENT_CB, MPFROMP(cmd), 0);
}

void GUI::ControllerNotification(const void* rcv, const Ctrl::EventFlags& flags)
{ DEBUGLOG(("GUI::ControllerNotification(, %x)\n", flags));
  Ctrl::EventFlags flg = flags;
  QMSG msg;
  if (WinPeekMsg(amp_player_hab(), &msg, HFrame, WMP_CTRL_EVENT, WMP_CTRL_EVENT, PM_REMOVE))
  { DEBUGLOG(("GUI::ControllerNotification - hit: %x\n", msg.mp1));
    // join messages
    flg |= (Ctrl::EventFlags)LONGFROMMP(msg.mp1);
  }
  WinPostMsg(HFrame, WMP_CTRL_EVENT, MPFROMLONG(flg), 0);
}

void GUI::PlayableNotification(const void* rcv, const PlayableChangeArgs& args)
{ DEBUGLOG(("GUI::PlayableNotification(, {&%p, %p, %x, %x, %x})\n",
    &args.Instance, args.Origin, args.Changed, args.Loaded, args.Invalidated));
  // TODO: Filter to improve performance
  WinPostMsg(HFrame, WMP_PLAYABLE_EVENT, MPFROMP(&args.Instance), MPFROM2SHORT(args.Changed, args.Loaded));
}

void GUI::PluginNotification(const void*, const Plugin::EventArgs& args)
{ DEBUGLOG(("GUI::PluginNotification(, {&%p{%s}, %i})\n", &args.Plug, args.Plug.GetModuleName().cdata(), args.Operation));
  switch (args.Operation)
  {case Plugin::EventArgs::Enable:
   case Plugin::EventArgs::Disable:
   case Plugin::EventArgs::Load:
   case Plugin::EventArgs::Unload:
    if (args.Plug.GetType() == PLUGIN_DECODER)
      WinPostMsg(HPlayer, WMP_REFRESH_ACCEL, 0, 0);
    IsPluginChanged = true;
    break;
   case Plugin::EventArgs::Active:
   case Plugin::EventArgs::Inactive:
    if (args.Plug.GetType() == PLUGIN_OUTPUT)
      IsPluginChanged = true;
   default:; // avoid warnings
  }
}

void GUI::Add2MRU(Playable& list, size_t max, APlayable& ps)
{ DEBUGLOG(("GUI::Add2MRU(&%p{%s}, %u, %s)\n", &list, list.URL.cdata(), max, ps.GetPlayable().URL.cdata()));
  Mutex::Lock lock(list.Mtx);
  int_ptr<PlayableInstance> pi;
  // remove the desired item from the list and limit the list size
  while ((pi = list.GetNext(pi)) != NULL)
  { DEBUGLOG(("GUI::Add2MRU - %p{%s}\n", pi.get(), pi->GetPlayable().URL.cdata()));
    if (max == 0 || pi->GetPlayable() == ps.GetPlayable()) // Instance equality of Playable is sufficient
      list.RemoveItem(pi);
     else
      --max;
  }
  // prepend list with new item
  list.InsertItem(ps, list.GetNext(NULL));
}

void DLLENTRY GUI_LoadFileCallback(void* param, const char* url)
{ DEBUGLOG(("GUI_LoadFileCallback(%p, %s)\n", param, url));
  ((LoadHelper*)param)->AddItem(Playable::GetByURL(url123(url)));
}

/* Returns TRUE if the save stream feature has been enabled. */
void GUI::SaveStream(HWND hwnd, BOOL enable)
{
  if( enable )
  { FILEDLG filedialog = { sizeof(FILEDLG) };
    filedialog.fl         = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB;
    filedialog.pszTitle   = "Save stream as";

    if (cfg.savedir.length() > 8)
      // strip file:///
      strlcpy(filedialog.szFullFile, cfg.savedir+8, sizeof filedialog.szFullFile);
    else
      filedialog.szFullFile[0] = 0;
    amp_file_dlg(HWND_DESKTOP, hwnd, &filedialog);

    if (filedialog.lReturn == DID_OK)
    { if (amp_warn_if_overwrite( hwnd, filedialog.szFullFile ))
      { url123 url = url123::normalizeURL(filedialog.szFullFile);
        Ctrl::PostCommand(Ctrl::MkSave(url), &GUI::ControllerEventCB);
        cfg.savedir = url.getBasePath();
      }
    }
  } else
    Ctrl::PostCommand(Ctrl::MkSave(xstring()));
}

void GUI::LoadAccel()
{ DEBUGLOG(("GUI::LoadAccel()\n"));
  const HAB hab = amp_player_hab();
  // Generate new accelerator table.
  HACCEL haccel = WinLoadAccelTable(hab, NULLHANDLE, ACL_MAIN);
  PMASSERT(haccel != NULLHANDLE);
  memset(LoadWizards+2, 0, sizeof LoadWizards - 2*sizeof *LoadWizards ); // You never know...
  dec_append_accel_table( haccel, IDM_M_LOADOTHER, 0, LoadWizards + 2, sizeof LoadWizards / sizeof *LoadWizards - 2);
  // Replace table of current window.
  HACCEL haccel_old = WinQueryAccelTable(hab, HFrame);
  PMRASSERT(WinSetAccelTable(hab, haccel, HFrame));
  if (haccel_old != NULLHANDLE)
    PMRASSERT(WinDestroyAccelTable(haccel_old));
  // done
  IsAccelChanged = true;
}

void GUI::ShowContextMenu()
{
  static HWND context_menu = NULLHANDLE;
  bool new_menu = false;
  if (context_menu == NULLHANDLE)
  { context_menu = WinLoadMenu(HWND_OBJECT, 0, MNU_MAIN);
    mn_make_conditionalcascade(context_menu, IDM_M_LOAD,   IDM_M_LOADFILE);
    mn_make_conditionalcascade(context_menu, IDM_M_PLOPEN, IDM_M_PLOPENDETAIL);
    mn_make_conditionalcascade(context_menu, IDM_M_HELP,   IDH_MAIN);

    MenuWorker = new PlaylistMenu(HPlayer, IDM_M_LAST, IDM_M_LAST_E);
    MenuWorker->AttachMenu(IDM_M_BOOKMARKS, *DefaultBM, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Recursive|PlaylistMenu::Enumerate, 0);
    MenuWorker->AttachMenu(IDM_M_LOAD, *LoadMRU, PlaylistMenu::Enumerate|PlaylistMenu::Separator, 0);
    new_menu = true;
  }

  POINTL   pos;
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
    PMRASSERT(WinSendMsg(context_menu, MM_QUERYITEM, MPFROM2SHORT(IDM_M_PLUG, TRUE), MPFROMP(&mi)));
    IsPluginChanged = false;
    load_plugin_menu(mi.hwndSubMenu);
  }

  if (IsAccelChanged || new_menu)
  { MENUITEM mi;
    PMRASSERT(WinSendMsg(context_menu, MM_QUERYITEM, MPFROM2SHORT(IDM_M_LOAD, TRUE), MPFROMP(&mi)));
    // Append asisstents from decoder plug-ins
    memset(LoadWizards+2, 0, sizeof LoadWizards - 2*sizeof *LoadWizards); // You never know...
    IsAccelChanged = false;
    dec_append_load_menu(mi.hwndSubMenu, IDM_M_LOADOTHER, 2, LoadWizards + 2, sizeof LoadWizards / sizeof *LoadWizards - 2);
    (MenuShowAccel(WinQueryAccelTable(amp_player_hab(), HFrame))).ApplyTo(new_menu ? context_menu : mi.hwndSubMenu);
  }

  // Update status
  mn_enable_item(context_menu, IDM_M_TAG,     CurrentIter && CurrentIter->GetRoot() && (CurrentIter->GetRoot()->GetInfo().tech->attributes & TATTR_WRITABLE));
  mn_enable_item(context_menu, IDM_M_SAVE,    CurrentIter && CurrentIter->GetRoot() && (CurrentIter->GetRoot()->GetInfo().tech->attributes & TATTR_SAVEABLE));
  mn_enable_item(context_menu, IDM_M_CURRENT_SONG, CurrentIter && CurrentIter->GetRoot());
  mn_enable_item(context_menu, IDM_M_CURRENT_PL, CurrentIter && CurrentIter->GetRoot() && (CurrentIter->GetRoot()->GetInfo().tech->attributes & TATTR_PLAYLIST));
  mn_enable_item(context_menu, IDM_M_SMALL,   bmp_is_mode_supported(CFG_MODE_SMALL));
  mn_enable_item(context_menu, IDM_M_NORMAL,  bmp_is_mode_supported(CFG_MODE_REGULAR));
  mn_enable_item(context_menu, IDM_M_TINY,    bmp_is_mode_supported(CFG_MODE_TINY));
  mn_enable_item(context_menu, IDM_M_FONT,    cfg.font_skinned );
  mn_enable_item(context_menu, IDM_M_FONT1,   bmp_is_font_supported(0));
  mn_enable_item(context_menu, IDM_M_FONT2,   bmp_is_font_supported(1));
  mn_enable_item(context_menu, IDM_M_ADDBOOK, CurrentIter && CurrentIter->GetRoot());

  mn_check_item(context_menu, IDM_M_FLOAT,  cfg.floatontop);
  mn_check_item(context_menu, IDM_M_SAVE,   !!Ctrl::GetSavename());
  mn_check_item(context_menu, IDM_M_FONT1,  cfg.font == 0);
  mn_check_item(context_menu, IDM_M_FONT2,  cfg.font == 1);
  mn_check_item(context_menu, IDM_M_SMALL,  cfg.mode == CFG_MODE_SMALL);
  mn_check_item(context_menu, IDM_M_NORMAL, cfg.mode == CFG_MODE_REGULAR);
  mn_check_item(context_menu, IDM_M_TINY,   cfg.mode == CFG_MODE_TINY);

  WinPopupMenu(HPlayer, HPlayer, context_menu, pos.x, pos.y, 0,
               PU_HCONSTRAIN|PU_VCONSTRAIN|PU_MOUSEBUTTON1|PU_MOUSEBUTTON2|PU_KEYBOARD);
}

void GUI::RefreshTimers(HPS hps)
{ DEBUGLOG(("GUI::RefreshTimers(%p) - %i %i\n", hps, cfg.mode, IsSeeking));

  Playable* root = CurrentIter->GetRoot();
  if (root == NULL)
  { bmp_draw_slider(hps, -1, IsAltSlider);
    bmp_draw_timer(hps, -1);
    bmp_draw_tiny_timer(hps, POS_TIME_LEFT, -1);
    bmp_draw_tiny_timer(hps, POS_PL_LEFT,   -1);
    return;
  }

  const bool is_playlist = !!(root->GetInfo().tech->attributes & TATTR_PLAYLIST);
  PM123_TIME total_song = CurrentIter->GetCurrent().GetInfo().obj->songlength;
  PM123_TIME total_time = is_playlist ? root->GetInfo().obj->songlength : -1;
  // TODO: calculate offset from call stack.
  PM123_TIME offset = -1;
  int index = -1;
  //const SongIterator::Offsets& off = CurrentIter->GetOffset(false);

  PM123_TIME list_left = -1;
  PM123_TIME play_left = total_song;
  if (play_left > 0)
    play_left -= CurrentIter->GetPosition();
  if (total_time > 0)
    list_left = total_time - offset - CurrentIter->GetPosition();

  double pos = -1.;
  if (!IsAltSlider)
  { if (total_song > 0)
      pos = CurrentIter->GetPosition()/total_song;
  } else switch (cfg.altnavig)
  {case CFG_ANAV_SONG:
    if (is_playlist)
    { int total_items = root->GetInfo().rpl->songs;
      if (total_items == 1)
        pos = 0;
      else if (total_items > 1)
        pos = index / (double)(total_items-1);
    }
    break;
   case CFG_ANAV_TIME:
    if (root->GetInfo().obj->songlength > 0)
    { pos = (offset + CurrentIter->GetPosition()) / root->GetInfo().obj->songlength;
      break;
    } // else CFG_ANAV_SONGTIME
   case CFG_ANAV_SONGTIME:
    if (is_playlist)
    { int total_items = root->GetInfo().rpl->songs;
      if (total_items == 1)
        pos = 0;
      else if (total_items > 1)
        pos = index;
      else break;
      // Add current song time
      if (total_song > 0)
        pos += CurrentIter->GetPosition()/total_song;
      pos /= total_items;
    }
  }
  bmp_draw_slider( hps, pos, IsAltSlider );
  bmp_draw_timer ( hps, CurrentIter->GetPosition());

  bmp_draw_tiny_timer( hps, POS_TIME_LEFT, play_left );
  bmp_draw_tiny_timer( hps, POS_PL_LEFT,   list_left );
}

/* Constructs a information text for currently loaded file
   and selects it for displaying. */
void GUI::PrepareText()
{
  if (CurrentRoot() == NULL)
  { DEBUGLOG(("GUI::PrepareText() NULL %u\n", cfg.viewmode));
    bmp_set_text( "- no file loaded -" );
    return;
  }
  DEBUGLOG(("GUI::PrepareText() %p %u\n", &CurrentSong(), cfg.viewmode));

  xstring text;
  const INFO_BUNDLE_CV& info = CurrentSong().GetInfo();
  switch( cfg.viewmode )
  {
    case CFG_DISP_ID3TAG:
      text = amp_construct_tag_string(&info);
      if (text.length())
        break;
      // if tag is empty - use filename instead of it.
    case CFG_DISP_FILENAME:
      // Give Priority to an alias name if any
      text = info.item->alias;
      if (!text)
        text = CurrentSong().GetPlayable().URL.getShortName();
      // In case of an invalid item display an error message.
      if (info.tech->attributes & TATTR_INVALID)
        text = text + " - " + xstring(info.tech->info);
      break;

    case CFG_DISP_FILEINFO:
      text = info.tech->info;
      break;
  }
  bmp_set_text(text);
}

void GUI::AutoSave(Playable& list)
{ if (list.IsModified())
    dec_saveplaylist(list.URL, list, "plist123.dll", NULL, false);
}

/*void GUI::RefreshTotals(HPS hps)
{	DEBUGLOG(("GUI::RefreshTotals()\n"));
  // Currently a NOP
}*/

void GUI::ViewMessage(xstring info, bool error)
{ if (info)
    WinPostMsg(HPlayer, WMP_DISPLAY_MESSAGE, MPFROMP(info.toCstr()), MPFROMLONG(error));
}

void GUI::Show(bool visible)
{ if (visible)
  { WinSetWindowPos(HFrame, HWND_TOP,
                    cfg.main.x, cfg.main.y, 0, 0, SWP_ACTIVATE|SWP_MOVE|SWP_SHOW);
  } else
    WinSendMsg(HPlayer, WM_COMMAND, MPFROMSHORT(IDM_M_MINIMIZE), 0);
}

void GUI::SetWindowMode(cfg_mode mode)
{ MPARAM mp1;
  switch (mode)
  {default:
    return;
   case CFG_MODE_REGULAR:
    mp1 = MPFROMSHORT(IDM_M_NORMAL);
    break;
   case CFG_MODE_SMALL:
    mp1 = MPFROMSHORT(IDM_M_SMALL);
    break;
   case CFG_MODE_TINY:
    mp1 = MPFROMSHORT(IDM_M_TINY);
    break;
  } 
  WinSendMsg(HPlayer, WM_COMMAND, mp1, 0);
}

void GUI::SetWindowFloat(bool flag)
{ // TODO: not atomic
  if (flag == cfg.floatontop)
    return;
  WinSendMsg(HPlayer, WM_COMMAND, MPFROMSHORT(IDM_M_FLOAT), 0);
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
}

void GUI::ShowDialog(Playable& item, DialogType dlg)
{ DEBUGLOG(("GUI::ShowDialog(&%p, %u)\n", &item, dlg));
  dialog_show* iep = new dialog_show(HPlayer, item, dlg);
  // At least we need the decoder to process this request.
  if (item->EnsureInfoAsync(Playable::IF_Other, false))
  { // Information immediately available => post message
    iep->InfoDelegate.detach(); // do no longer wait for the event
    PMRASSERT(WinPostMsg(HFrame, AMP_SHOW_DIALOG, iep, 0));
  }
}*/

void GUI::ShowDialog(Playable& item, DialogType dlg)
{ DEBUGLOG(("GUI::ShowDialog(&%p, %u)\n", &item, dlg));
  int_ptr<Playable> pp(&item);
  PMRASSERT(WinPostMsg(HFrame, WMP_SHOW_DIALOG, MPFROMP(pp.toCptr()), MPFROMSHORT(dlg)));
}

BOOL EXPENTRY GUI_HelpHook(HAB hab, ULONG usMode, ULONG idTopic, ULONG idSubTopic, PRECTL prcPosition)
{ DEBUGLOG(("HelpHook(%p, %x, %x, %x, {%li,%li, %li,%li})\n", hab,
    usMode, idTopic, idSubTopic, prcPosition->xLeft, prcPosition->yBottom, prcPosition->xRight, prcPosition->yTop));
  return FALSE;
}

/* Prepares the player to the drop operation. */
MRESULT GUI::DragOver(DRAGINFO* pdinfo)
{ DEBUGLOG(("amp_drag_over(%p)\n", pdinfo));

  PDRAGITEM pditem;
  int       i;
  USHORT    drag_op = 0;
  USHORT    drag    = DOR_NEVERDROP;

  if (!DrgAccessDraginfo(pdinfo))
    return MRFROM2SHORT(DOR_NEVERDROP, 0);

  DEBUGLOG(("amp_drag_over(%p{,,%x, %p, %i,%i, %u,})\n",
    pdinfo, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem));

  for( i = 0; i < pdinfo->cditem; i++ )
  { pditem = DrgQueryDragitemPtr( pdinfo, i );

    if (DrgVerifyRMF(pditem, "DRM_OS2FILE", NULL) || DrgVerifyRMF(pditem, "DRM_123FILE", NULL))
    { drag    = DOR_DROP;
      drag_op = DO_COPY;
    } else {
      drag    = DOR_NEVERDROP;
      drag_op = 0;
      break;
    }
  }

  DrgFreeDraginfo(pdinfo);
  return MPFROM2SHORT(drag, drag_op);
}

struct DropInfo
{ //USHORT count;    // Count of dragged objects.
  //USHORT index;    // Index of current item [0..Count)
  xstring URL;      // Object to drop
  xstring Start;    // Start Iterator
  xstring Stop;     // Stop iterator
  sco_ptr<LoadHelper> LH;
  int     options;  // Options for amp_load_playable
  HWND    hwndItem; // Window handle of the source of the drag operation.
  ULONG   ulItemID; // Information used by the source to identify the
                    // object being dragged.
};

/* Receives dropped files or playlist records. */
MRESULT GUI::DragDrop(PDRAGINFO pdinfo)
{ DEBUGLOG(("amp_drag_drop(%p)\n", pdinfo));

  if (!DrgAccessDraginfo(pdinfo))
    return 0;

  DEBUGLOG(("amp_drag_drop: {,%u,%x,%p, %u,%u, %u,}\n",
    pdinfo->cbDragitem, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem));

  sco_ptr<LoadHelper> lhp(new LoadHelper(cfg.playonload*LoadHelper::LoadPlay | cfg.append_dnd*LoadHelper::LoadAppend));
  ULONG reply = DMFL_TARGETFAIL;

  for( int i = 0; i < pdinfo->cditem; i++ )
  {
    DRAGITEM* pditem = DrgQueryDragitemPtr( pdinfo, i );
    DEBUGLOG(("amp_drag_drop: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, amp_string_from_drghstr(pditem->hstrType).cdata(), amp_string_from_drghstr(pditem->hstrRMF).cdata(),
      amp_string_from_drghstr(pditem->hstrContainerName).cdata(), amp_string_from_drghstr(pditem->hstrSourceName).cdata(), amp_string_from_drghstr(pditem->hstrTargetName).cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    reply = DMFL_TARGETFAIL;

    if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ))
    {
      // fetch full qualified path
      size_t lenP = DrgQueryStrNameLen(pditem->hstrContainerName);
      size_t lenN = DrgQueryStrNameLen(pditem->hstrSourceName);
      xstring fullname;
      char* cp = fullname.allocate(lenP + lenN);
      DrgQueryStrName(pditem->hstrContainerName, lenP+1, cp);
      DrgQueryStrName(pditem->hstrSourceName,    lenN+1, cp+lenP);

      if (pditem->hwndItem && DrgVerifyType(pditem, "UniformResourceLocator"))
      { // URL => The droped item must be rendered.
        DRAGTRANSFER* pdtrans = DrgAllocDragtransfer(1);
        if (pdtrans)
        { DropInfo* pdsource = new DropInfo();
          pdsource->LH       = lhp.detach();
          pdsource->hwndItem = pditem->hwndItem;
          pdsource->ulItemID = pditem->ulItemID;

          pdtrans->cb               = sizeof( DRAGTRANSFER );
          pdtrans->hwndClient       = HPlayer;
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

        const url123& url = url123::normalizeURL(fullname);
        DEBUGLOG(("amp_drag_drop: url=%s\n", url ? url.cdata() : "<null>"));
        if (url)
        { lhp->AddItem(Playable::GetByURL(url));
          reply = DMFL_TARGETSUCCESSFUL;
        }
      }

    } else if (DrgVerifyRMF(pditem, "DRM_123FILE", NULL))
    { // In the DRM_123FILE transfer mechanism the target is responsible for doing the target related stuff
      // while the source does the source related stuff. So a DO_MOVE operation causes
      // - a create in the target window and
      // - a remove in the source window.
      // The latter is done when DM_ENDCONVERSATION arrives with DMFL_TARGETSUCCESSFUL.

      DRAGTRANSFER* pdtrans = DrgAllocDragtransfer(1);
      if (pdtrans)
      { pdtrans->cb               = sizeof(DRAGTRANSFER);
        pdtrans->hwndClient       = HPlayer;
        pdtrans->pditem           = pditem;
        pdtrans->hstrSelectedRMF  = DrgAddStrHandle("<DRM_123FILE,DRF_UNKNOWN>");
        pdtrans->hstrRenderToName = 0;
        pdtrans->fsReply          = 0;
        pdtrans->usOperation      = pdinfo->usOperation;

        // Ask the source to render the selected item.
        DrgSendTransferMsg(pditem->hwndItem, DM_RENDER, (MPARAM)pdtrans, 0);

        // insert item
        if ((pdtrans->fsReply & DMFL_NATIVERENDER))
        { // TODO: slice!
          lhp->AddItem(Playable::GetByURL(amp_string_from_drghstr(pditem->hstrSourceName)));
          reply = DMFL_TARGETSUCCESSFUL;
        }
        // cleanup
        DrgFreeDragtransfer(pdtrans);
      }
    }
    // Tell the source you're done.
    DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION, MPFROMLONG(pditem->ulItemID), MPFROMLONG(reply));

  } // foreach pditem

  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );

  if (reply == DMFL_TARGETSUCCESSFUL)
    Load(lhp.detach());
  return 0;
}

/* Receives dropped and rendered files and urls. */
MRESULT GUI::DragRenderDone(PDRAGTRANSFER pdtrans, USHORT rc)
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
    { const url123& url = url123::normalizeURL(fullname);
      DEBUGLOG(("amp_drag_render_done: url=%s\n", url ? url.cdata() : "<null>"));
      if (url)
      { pdsource->LH->AddItem(Playable::GetByURL(url));
        reply = DMFL_TARGETSUCCESSFUL;
      }
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



void GUI::Init()
{ DEBUGLOG(("GUI::Init()\n"));

  // these two are always constant
  LoadWizards[0] = amp_file_wizard;
  LoadWizards[1] = amp_url_wizard;

  PMRASSERT(WinRegisterClass(amp_player_hab(), "PM123", &GUI_DlgProcStub, CS_SIZEREDRAW, 0));
  ULONG flCtlData = FCF_TASKLIST|FCF_NOBYTEALIGN|FCF_ICON;
  HFrame = WinCreateStdWindow(HWND_DESKTOP, 0, &flCtlData, "PM123",
                              AMP_FULLNAME, 0, NULLHANDLE, WIN_MAIN, &HPlayer);
  PMASSERT(HFrame != NULLHANDLE);
  DEBUGLOG(("GUI::Init: window created: frame = %x, player = %x\n", HFrame, HPlayer));

  // Init skin
  { PresentationSpace hps(HPlayer);
    bmp_load_skin(cfg.defskin, HPlayer, hps);
  }

  // Init default lists
  { const url123& path = url123::normalizeURL(amp_startpath);
    DefaultPL = Playable::GetByURL(path + "PM123.LST");
    DefaultPM = Playable::GetByURL(path + "PFREQ.LST");
    DefaultBM = Playable::GetByURL(path + "BOOKMARK.LST");
    LoadMRU   = Playable::GetByURL(path + "LOADMRU.LST");
    UrlMRU    = Playable::GetByURL(path + "URLMRU.LST");
    // The default playlist the bookmarks and the MRU list must be ready to use
    DefaultPL->RequestInfo(IF_Child, PRI_Normal);
    DefaultBM->RequestInfo(IF_Child, PRI_Normal);
    LoadMRU->RequestInfo(IF_Child, PRI_Normal);
    UrlMRU->RequestInfo(IF_Child, PRI_Normal);
  }

  // Keep track of plug-in changes.
  Plugin::ChangeEvent += PluginDeleg;
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

  HHelp = WinCreateHelpInstance( amp_player_hab(), &hinit );
  if( HHelp == NULLHANDLE )
    amp_error( HFrame, "Error create help instance: %s", infname.cdata() );
  else
    PMRASSERT(WinAssociateHelpInstance(HHelp, HPlayer));
    
  WinSetHook( amp_player_hab(), HMQ_CURRENT, HK_HELP, (PFN)&GUI_HelpHook, 0 );

  // Docking...
  dk_init();
  dk_add_window(HFrame, DK_IS_MASTER);
  if (cfg.dock_windows)
    dk_arrange(HFrame);

  // initialize non-skin related visual plug-ins
  vis_init_all( HPlayer, FALSE );

  DEBUGLOG(("GUI::Init: complete\n"));

  // Init some other stuff
  PlaylistView::Init();
  PlaylistManager::Init();

  // Now it is time to show the window
  PMRASSERT(WinSetWindowPos(HFrame, HWND_TOP, cfg.main.x,cfg.main.y, 0,0, SWP_ACTIVATE|SWP_MOVE|SWP_SHOW));
}

void GUI::Uninit()
{ DEBUGLOG(("GUI::Uninit()\n"));
 
  PlaylistManager::UnInit();
  PlaylistView::UnInit();
  InspectorDialog::UnInit();

  // deinitialize all visual plug-ins
  vis_deinit_all( TRUE );
  vis_deinit_all( FALSE );

  ControllerDeleg.detach();
  PluginDeleg.detach();
  delete MenuWorker;

  if (HHelp != NULLHANDLE)
    WinDestroyHelpInstance(HHelp);

  // Eat user messages from the queue
  // TODO: causes memory leaks, but who cares on termination.
  QMSG   qmsg;
  while (WinPeekMsg(amp_player_hab(), &qmsg, HFrame, WM_USER, 0xbfff, PM_REMOVE));

  // Clear iterators to avoid dead references.
  IterBuffer[0].SetRoot(NULL);
  IterBuffer[1].SetRoot(NULL);

  // Save by default
  AutoSave(*DefaultBM);
  AutoSave(*DefaultPM);
  AutoSave(*DefaultPL);
  AutoSave(*LoadMRU);
  AutoSave(*UrlMRU);

  dk_term();
  WinDestroyWindow(HFrame);
  bmp_clean_skin();

  UrlMRU    = NULL;
  LoadMRU   = NULL;
  DefaultBM = NULL;
  DefaultPM = NULL;
  DefaultPL = NULL;
}

