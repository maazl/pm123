/*
 * Copyright 2007-2012 Marcel Mueeller
 *           1997-2003 Samuel Audet <guardia@step.polymtl.ca>
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


#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DOS

#include "playlistbase.h"
#include "../core/dependencyinfo.h"
#include "../engine/plugman.h"
#include "../engine/loadhelper.h"
#include "playlistmanager.h"
#include "playlistview.h"
#include "playlistmenu.h"
#include "infodialog.h"
#include "inspector.h"
#include "postmsginfo.h"
#include "pm123.h" // for amp_player_hab
#include "gui.h"
#include "dialog.h"
#include "configuration.h"
#include "123_util.h" // amp_url_from_file
#include "pm123.rc.h"
#include "docking.h"

#include <utilfct.h>
#include <cpp/url123.h>
#include <cpp/pmutils.h>
#include <stdio.h>
#include <stdlib.h>
#include <os2.h>

#include <debuglog.h>


#define MAX_DRAG_IMAGES 6


#ifdef DEBUG_LOG
xstring PlaylistBase::RecordBase::DebugName() const
{ return xstring().sprintf("%p{%p{%s}}", this, Data->Content.get(), Data->Content->DebugName().cdata());
}
xstring PlaylistBase::RecordBase::DebugName(const RecordBase* rec)
{ static const xstring nullstring = "<NULL>";
  static const xstring firststring = "<CMA_FIRST>";
  if (!rec)
    return nullstring;
  if (rec == (RecordBase*)CMA_FIRST)
    return firststring;
  return rec->DebugName();
}

xstring PlaylistBase::DebugName() const
{ return Content->URL.getShortName();
}
#endif


/****************************************************************************
*
*  class PlaylistBase
*
****************************************************************************/

HPOINTER PlaylistBase::IcoSong[6];
HPOINTER PlaylistBase::IcoPlaylist[2][6][4] = { 0 };

void PlaylistBase::InitIcons()
{ IcoSong       [IC_Pending]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_WAIT);
  IcoSong       [IC_Invalid]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_SONG_INVALID);
  IcoSong       [IC_Normal ]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_SONG);
  IcoSong       [IC_Active ]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_SONG_ACTIVE);
  IcoSong       [IC_Play   ]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_SONG_PLAY);
  IcoSong       [IC_Shadow ]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_SONG_SHADOW);
  // Playlists
  IcoPlaylist[0][IC_Pending][ICP_Empty    ] = IcoSong       [IC_Pending];
  IcoPlaylist[0][IC_Invalid][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_INVALID);
  IcoPlaylist[0][IC_Normal ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_EMPTY);
  IcoPlaylist[0][IC_Active ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_EMPTY_ACTIVE);
  IcoPlaylist[0][IC_Play   ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_EMPTY_PLAY);
  IcoPlaylist[0][IC_Shadow ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_EMPTY_SHADOW);
  IcoPlaylist[0][IC_Pending][ICP_Closed   ] = IcoPlaylist[0][IC_Pending][ICP_Empty    ];
  IcoPlaylist[0][IC_Invalid][ICP_Closed   ] = IcoPlaylist[0][IC_Invalid][ICP_Empty    ];
  IcoPlaylist[0][IC_Normal ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_CLOSE);
  IcoPlaylist[0][IC_Active ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_CLOSE_ACTIVE);
  IcoPlaylist[0][IC_Play   ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_CLOSE_PLAY);
  IcoPlaylist[0][IC_Shadow ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_CLOSE_SHADOW);
  IcoPlaylist[0][IC_Normal ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_OPEN);
  IcoPlaylist[0][IC_Active ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_OPEN_ACTIVE);
  IcoPlaylist[0][IC_Play   ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_OPEN_PLAY);
  IcoPlaylist[0][IC_Shadow ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_OPEN_SHADOW);
  IcoPlaylist[0][IC_Normal ][ICP_Recursive] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_RECSV);
  IcoPlaylist[0][IC_Active ][ICP_Recursive] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_RECSV_ACTIVE);
  IcoPlaylist[0][IC_Play   ][ICP_Recursive] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PL_RECSV_PLAY);
  IcoPlaylist[0][IC_Shadow ][ICP_Recursive] = IcoPlaylist[0][IC_Normal ][ICP_Recursive];
  // Folders
  IcoPlaylist[1][IC_Pending][ICP_Empty    ] = IcoPlaylist[0][IC_Pending][ICP_Empty    ];
  IcoPlaylist[1][IC_Invalid][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_INVALID);
  IcoPlaylist[1][IC_Normal ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_EMPTY);
  IcoPlaylist[1][IC_Active ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_EMPTY_ACTIVE);
  IcoPlaylist[1][IC_Play   ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_EMPTY_PLAY);
  IcoPlaylist[1][IC_Shadow ][ICP_Empty    ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_EMPTY_SHADOW);
  IcoPlaylist[1][IC_Pending][ICP_Closed   ] = IcoPlaylist[1][IC_Pending][ICP_Empty    ];
  IcoPlaylist[1][IC_Invalid][ICP_Closed   ] = IcoPlaylist[1][IC_Invalid][ICP_Empty    ];
  IcoPlaylist[1][IC_Normal ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_CLOSE);
  IcoPlaylist[1][IC_Active ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_CLOSE_ACTIVE);
  IcoPlaylist[1][IC_Play   ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_CLOSE_PLAY);
  IcoPlaylist[1][IC_Shadow ][ICP_Closed   ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_CLOSE_SHADOW);
  IcoPlaylist[1][IC_Normal ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_OPEN);
  IcoPlaylist[1][IC_Active ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_OPEN_ACTIVE);
  IcoPlaylist[1][IC_Play   ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_OPEN_PLAY);
  IcoPlaylist[1][IC_Shadow ][ICP_Open     ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_FL_OPEN_SHADOW);
  IcoPlaylist[1][IC_Normal ][ICP_Recursive] = IcoPlaylist[0][IC_Normal ][ICP_Recursive];
  IcoPlaylist[1][IC_Active ][ICP_Recursive] = IcoPlaylist[0][IC_Active ][ICP_Recursive];
  IcoPlaylist[1][IC_Play   ][ICP_Recursive] = IcoPlaylist[0][IC_Play   ][ICP_Recursive];
  IcoPlaylist[1][IC_Shadow ][ICP_Recursive] = IcoPlaylist[0][IC_Normal ][ICP_Recursive];
}


PlaylistBase::PlaylistBase(Playable& content, ULONG rid)
: ManagedDialog<DialogBase>(rid, NULLHANDLE, DF_AutoResize)
, Content(&content)
, HwndContainer(NULLHANDLE)
, NoRefresh(false)
, HwndMenu(NULLHANDLE)
, DecChanged(false)
, MenuWorker(NULL)
, RootInfoDelegate(*this, &PlaylistBase::InfoChangeEvent, NULL)
, RootPlayStatusDelegate(*this, &PlaylistBase::PlayStatEvent)
, PluginDelegate(Plugin::GetChangeEvent(), *this, &PlaylistBase::PluginEvent)
{ DEBUGLOG(("PlaylistBase(%p)::PlaylistBase(&%p{%s}, %u)\n", this, &content, content.URL.cdata(), rid));
  static bool first = true;
  if (first)
  { InitIcons();
    first = false;
  }
  // These ones are constant
  LoadWizards[0] = &amp_file_wizard;
  LoadWizards[1] = &amp_url_wizard;
  LoadWizards[2] = &amp_new_list_wizard;
}

PlaylistBase::~PlaylistBase()
{ DEBUGLOG(("PlaylistBase(%p)::~PlaylistBase()\n", this));
  WinDestroyAccelTable(AccelTable);
}

void PlaylistBase::MsgJumpCompleted(Ctrl::ControlCommand* cmd)
{ delete (Location*)cmd->PtrArg;
  delete cmd;
}

void PlaylistBase::PostRecordUpdate(RecordBase* rec, InfoFlags flags)
{ AtomicUnsigned& il = StateFromRec(rec).UpdateFlags;
  DEBUGLOG(("PlaylistBase(%p)::PostRecordUpdate(%p, %u) - %x\n", this, rec, flags, il.get()));
  // Check whether the requested bit is already set or there are other events pending.
  unsigned last = il;
  il |= flags;
  if (last || il != (unsigned)flags)
  { DEBUGLOG(("PlaylistBase::PostRecordCommand - nope! %x\n", il.get()));
    return; // requested command is already on the way or another unexecuted message is outstanding
  }
  // There is a little chance that we generate two messages for the same record.
  // The second one will be a no-op in the window procedure.
  BlockRecord(rec);
  PMRASSERT(WinPostMsg(GetHwnd(), UM_RECORDUPDATE, MPFROMP(rec), MPFROMSHORT(TRUE)));
}

InfoFlags PlaylistBase::RequestRecordInfo(RecordBase* const rec, InfoFlags filter)
{ DEBUGLOG(("PlaylistBase(%p)::RequestRecordInfo(%p, %x)\n", this, rec, filter));
  if (filter == IF_None)
    return IF_None;
  InfoFlags high = FilterRecordRequest(rec, filter);
  APlayable& pp = APlayableFromRec(rec);
  if (high != filter)
    filter &= ~pp.RequestInfo(filter, PRI_Low);
  filter &= ~pp.RequestInfo(filter & high, PRI_Normal);
  return filter;
}

void PlaylistBase::FreeRecord(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::FreeRecord(%p)\n", this, rec));
  if (rec && !--rec->UseCount)
    // we can safely post this message because the record is now no longer used anyway.
    PMRASSERT(WinPostMsg(GetHwnd(), UM_DELETERECORD, MPFROMP(rec), 0));
}

void PlaylistBase::DeleteEntry(RecordBase* entry)
{ DEBUGLOG(("PlaylistBase(%p{%s})::DeleteEntry(%s)\n", this, DebugName().cdata(), RecordBase::DebugName(entry).cdata()));
  delete entry->Data;
  PMRASSERT(WinSendMsg(HwndContainer, CM_FREERECORD, MPFROMP(&entry), MPFROMSHORT(1)));
  DEBUGLOG(("PlaylistBase::DeleteEntry completed\n"));
}


void PlaylistBase::StartDialog()
{ DialogBase::StartDialog(HWND_DESKTOP);
}

void PlaylistBase::InitDlg()
{ // Icon
  HPOINTER hicon = WinLoadPointer(HWND_DESKTOP, 0, ICO_MAIN);
  PMASSERT(hicon != NULLHANDLE);
  PMRASSERT(WinSendMsg(GetHwnd(), WM_SETICON, (MPARAM)hicon, 0));
  do_warpsans(GetHwnd());
  { // set colors. PRESPARAMS in the RC file seems not to work for some reason.
    LONG     color;
    color = CLR_GREEN;
    PMRASSERT(WinSetPresParam(HwndContainer, PP_FOREGROUNDCOLORINDEX, sizeof(color), &color));
    color = CLR_BLACK;
    PMRASSERT(WinSetPresParam(HwndContainer, PP_BACKGROUNDCOLORINDEX, sizeof(color), &color));
  }

  // Help manager
  SetHelpMgr(GUI::GetHelpMgr());

  { // initial position
    SWP swp;
    PMXASSERT(WinQueryTaskSizePos(amp_player_hab, 0, &swp), == 0);
    PMRASSERT(WinSetWindowPos(GetHwnd(), NULLHANDLE, swp.x,swp.y, 0,0, SWP_MOVE));
  }
  Cfg::RestWindowPos(GetHwnd(), Content->URL);

  dk_add_window(GetHwnd(), 0);

  // register events
  Content->APlayable::GetInfoChange() += RootInfoDelegate;
  Ctrl::GetChangeEvent() += RootPlayStatusDelegate;

  SetTitle();
  // initialize decoder dependent information once.
  PMRASSERT(WinPostMsg(GetHwnd(), UM_UPDATEDEC, 0, 0));

  // Request initial information for root level.
  PostRecordUpdate(NULL, RequestRecordInfo(NULL));
}

MRESULT PlaylistBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistBase(%p)::DlgProc(%x, %x, %x)\n", this, msg, mp1, mp2));
  switch (msg)
  {case WM_INITDLG:
    { MRESULT ret = ManagedDialog<DialogBase>::DlgProc(msg, mp1, mp2);
      InitDlg();
      return ret;
    }

   case WM_DESTROY:
    { DEBUGLOG(("PlaylistBase::DlgProc: WM_DESTROY\n"));
      // Deregister Events
      RootInfoDelegate.detach();
      RootPlayStatusDelegate.detach();
      // delete all records
      RemoveChildren(NULL);
      // save position
      Cfg::SaveWindowPos(GetHwnd(), Content->URL);

      // process outstanding UM_DELETERECORD messages before we quit to ensure that all records are back to the PM before we die.
      QMSG qmsg;
      while (WinPeekMsg(amp_player_hab, &qmsg, GetHwnd(), UM_DELETERECORD, UM_DELETERECORD, PM_REMOVE))
      { DEBUGLOG2(("PlaylistBase::DlgProc: WM_DESTROY: %x %x %x %x\n", qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2));
        DlgProc(qmsg.msg, qmsg.mp1, qmsg.mp2); // We take the short way here.
      }
      break;
    }

   case WM_SYSCOMMAND:
    if (SHORT1FROMMP(mp1) == SC_CLOSE)
    { if ( (Content->GetInfo().tech->attributes & TATTR_PLAYLIST)
        && Content->IsModified() )
      { switch (amp_query3(GetHwnd(), "The current list is modified. Save changes?"))
        {default: // cancel
          return 0;
         case MBID_YES:
          UserSave(false);
         case MBID_NO:;
        }
      }
    }
    break;

   case WM_MENUEND:
    if (HWNDFROMMP(mp2) == HwndMenu)
    { //GetRecords(CRA_SOURCE); //Should normally be unchanged...
      DEBUGLOG(("PlaylistBase::DlgProc WM_MENUEND %u\n", Source.size()));
      SetEmphasis(CRA_SOURCE, false);
      if (MenuWorker)
        MenuWorker->DetachMenu(IDM_PL_CONTENT);
      HwndMenu = NULLHANDLE;
    }
    break;

   case WM_WINDOWPOSCHANGED:
    { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
      if ((pswp[0].fl & (SWP_SHOW|SWP_HIDE)) == 0)
        break;
      dk_set_state( GetHwnd(), pswp[0].fl & SWP_SHOW ? 0 : DK_IS_GHOST );
      /*HSWITCH hswitch = WinQuerySwitchHandle( GetHwnd(), 0 );
      if (hswitch == NULLHANDLE)
        break; // For some reasons the switchlist entry may be destroyed before.
      SWCNTRL swcntrl;
      PMXASSERT(WinQuerySwitchEntry(hswitch, &swcntrl), == 0);
      swcntrl.uchVisibility = pswp[0].fl & SWP_SHOW ? SWL_VISIBLE : SWL_INVISIBLE;
      PMXASSERT(WinChangeSwitchEntry( hswitch, &swcntrl ), == 0);*/
      break;
    }

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case CO_CONTENT:
      switch (SHORT2FROMMP(mp1))
      {
       case CN_CONTEXTMENU:
        { RecordBase* rec = (RecordBase*)mp2;
          if (!GetSource(rec))
            break;
          InitContextMenu();
          if (HwndMenu != NULLHANDLE)
          { SetEmphasis(CRA_SOURCE, true);
            // Show popup menu
            POINTL ptlMouse;
            PMRASSERT(WinQueryPointerPos(HWND_DESKTOP, &ptlMouse));

            PMRASSERT(WinPopupMenu(HWND_DESKTOP, GetHwnd(), HwndMenu, ptlMouse.x, ptlMouse.y, 0,
                                   PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD));
          }
        }
        break;

       case CN_ENTER:
        { NOTIFYRECORDENTER* nre = (NOTIFYRECORDENTER*)mp2;
          APlayable& pp = APlayableFromRec((RecordBase*)nre->pRecord);
          // Playlist => open, song => see config
          if (pp.GetInfo().tech->attributes & TATTR_PLAYLIST)
            UserOpenDetailedView(pp.GetPlayable());
          else
          { volatile const amp_cfg& cfg = Cfg::Get();
            if (cfg.itemaction == CFG_ACTION_NAVTO)
              UserNavigate((RecordBase*)nre->pRecord);
            else
            { LoadHelper lhp(cfg.playonload*LoadHelper::LoadPlay | (cfg.itemaction == CFG_ACTION_QUEUE)*LoadHelper::LoadAppend | LoadHelper::LoadRecall);
              lhp.AddItem(pp);
              GUI::Load(lhp);
            }
          }
        }
        break;

       // Direct editing
       case CN_BEGINEDIT:
        DEBUGLOG(("PlaylistBase::DlgProc CN_BEGINEDIT\n"));
        PMRASSERT(WinSetAccelTable(WinQueryAnchorBlock(GetHwnd()), NULLHANDLE, GetHwnd()));
        // TODO: normally we have to lock some functions here...
        return 0;

       case CN_REALLOCPSZ:
        { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
          DEBUGLOG(("PlaylistBase::DlgProc CN_REALLOCPSZ %p{,%p->%p{%s},%u,}\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText));
          *ed->ppszText = DirectEdit.allocate(ed->cbText-1); // because of the string sharing we always have to initialize the instance
          return MRFROMLONG(TRUE);
        }

       case CN_ENDEDIT:
        DEBUGLOG(("PlaylistBase::DlgProc CN_ENDEDIT\n"));
        DirectEdit.reset(); // Free string
        PMRASSERT(WinSetAccelTable(WinQueryAnchorBlock(GetHwnd()), AccelTable, GetHwnd()));
        return 0;

       // D'n'D Source
       case CN_INITDRAG:
        { CNRDRAGINIT* cdi = (CNRDRAGINIT*)mp2;
          if (!GetSource((RecordBase*)cdi->pRecord))
            return 0;
          DragInit();
          return 0;
        }

       // D'n'D Target
       case CN_DRAGOVER:
       case CN_DRAGAFTER:
        { CNRDRAGINFO* pcdi = (CNRDRAGINFO*)PVOIDFROMMP(mp2);
          if (!mp2 || !pcdi->pDragInfo)
          { // Most likely an error of the source application. So do not blow out.
            DEBUGLOG(("PlaylistBase::DlgProc: CM_DRAG... - DrgAccessDraginfo failed\n"));
            return MRFROM2SHORT(DOR_NEVERDROP, 0);
          }
          DragAfter = SHORT2FROMMP(mp1) == CN_DRAGAFTER;
          return DragOver(pcdi->pDragInfo, (RecordBase*)pcdi->pRecord);
        }
       case CN_DROP:
        { CNRDRAGINFO* pcdi = (CNRDRAGINFO*)PVOIDFROMMP(mp2);
          if (!mp2 || !pcdi->pDragInfo)
            // Most likely an error of the source application. So do not blow out.
            DEBUGLOG(("PlaylistBase::DlgProc: CM_DRAG... - DrgAccessDraginfo failed\n"));
          else
            DragDrop(pcdi->pDragInfo, (RecordBase*)pcdi->pRecord);
          return 0;
        }
       case CN_DROPHELP:
        GUI::ShowHelp(IDH_DRAG_AND_DROP);
        return 0;
      } // switch (SHORT2FROMMP(mp1))
      break;

     case IDM_PL_CONTENT:
      { // bookmark is selected
        PlaylistMenu::MenuCtrlData& cd = *(PlaylistMenu::MenuCtrlData*)PVOIDFROMMP(mp2);
        GUI::NavigateTo(cd.Item);
        break;
      }
    } // switch (SHORT1FROMMP(mp1))
    break;

   case DM_RENDERCOMPLETE:
    { DropRenderComplete((DRAGTRANSFER*)PVOIDFROMMP(mp1), SHORT1FROMMP(mp2));
      return 0;
    }
   case DM_DISCARDOBJECT:
    return MRFROMLONG(DropDiscard((DRAGINFO*)PVOIDFROMMP(mp1)));

   case DM_RENDER:
    return MRFROMLONG(DropRender((DRAGTRANSFER*)PVOIDFROMMP(mp1)));

   case UM_RENDERASYNC:
    DropRenderAsync((DRAGTRANSFER*)PVOIDFROMMP(mp1), (Playable*)PVOIDFROMMP(mp2));
    return 0;

   case DM_ENDCONVERSATION:
    DropEnd((vector<RecordBase>*)PVOIDFROMMP(mp1), !!(LONGFROMMP(mp2) & DMFL_TARGETSUCCESSFUL));
    return 0;

   case WM_COMMAND:
    { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: WM_COMMAND %u %u\n", this, DebugName().cdata(), SHORT1FROMMP(mp1), Source.size()));
      // Determine source in case of a accelerator-key
      if (SHORT1FROMMP(mp2) == CMDSRC_ACCELERATOR)
        GetRecords(CRA_SELECTED);

      if (SHORT1FROMMP(mp1) >= IDM_PL_APPFILEALL && SHORT1FROMMP(mp1) < IDM_PL_APPFILEALL + sizeof LoadWizards / sizeof *LoadWizards)
      { DECODER_WIZARD_FUNC func = LoadWizards[SHORT1FROMMP(mp1)-IDM_PL_APPFILEALL];
        if (func != NULL)
          UserAdd(func, NULL);
        return 0;
      }
      if (SHORT1FROMMP(mp1) >= IDM_PL_APPFILE && SHORT1FROMMP(mp1) < IDM_PL_APPFILE + sizeof LoadWizards / sizeof *LoadWizards)
      { DECODER_WIZARD_FUNC func = LoadWizards[SHORT1FROMMP(mp1)-IDM_PL_APPFILE];
        if (func != NULL && Source.size() == 1)
          UserAdd(func, Source[0]);
        return 0;
      }
      switch (SHORT1FROMMP(mp1))
      {case IDM_PL_MENU:
        { RecordBase* rec = (RecordBase*)mp2;
          if (!GetSource(rec))
            break;
          InitContextMenu();
          if (HwndMenu != NULLHANDLE)
          { SetEmphasis(CRA_SOURCE, true);
            // Show popup menu
            POINTL ptlMouse;
            PMRASSERT(WinQueryPointerPos(HWND_DESKTOP, &ptlMouse));
            // TODO: Mouse Position is not be reasonable, when the menu is invoked by keyboard.

            PMRASSERT(WinPopupMenu(HWND_DESKTOP, GetHwnd(), HwndMenu, ptlMouse.x, ptlMouse.y, 0,
                                   PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD));
          }
          break;
        }

       case IDM_PL_USEALL:
        { LoadHelper lhp(Cfg::Get().playonload*LoadHelper::LoadPlay | LoadHelper::LoadRecall);
          lhp.AddItem(*Content);
          GUI::Load(lhp);
          break;
        }
       case IDM_PL_USE:
        { LoadHelper lhp(Cfg::Get().playonload*LoadHelper::LoadPlay | LoadHelper::LoadRecall);
          for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
            lhp.AddItem(*(*rpp)->Data->Content);
          GUI::Load(lhp);
          break;
        }
       case IDM_PL_NAVIGATE:
        if (Source.size() == 1)
          UserNavigate(Source[0]);
        break;

       case IDM_PL_TREEVIEWALL:
        UserOpenTreeView(*Content);
        break;
       case IDM_PL_TREEVIEW:
        Apply2Source(&PlaylistBase::UserOpenTreeView);
        break;

       case IDM_PL_DETAILEDALL:
        UserOpenDetailedView(*Content);
        break;
       case IDM_PL_DETAILED:
        Apply2Source(&PlaylistBase::UserOpenDetailedView);
        break;

       case IDM_PL_REFRESH:
        Apply2Source(&PlaylistBase::UserReload);
        break;

       case IDM_PL_PROPERTIES:
        if (Source.size() == 1)
        { AInfoDialog::KeyType set;
          PopulateSetFromSource(set);
          UserOpenInfoView(set, AInfoDialog::Page_ItemInfo);
        }
        break;
       case IDM_PL_INFOALL:
        { AInfoDialog::KeyType set;
          set.get(*Content) = Content;
          UserOpenInfoView(set, AInfoDialog::Page_TechInfo);
        }
        break;
       case IDM_PL_INFO:
        { AInfoDialog::KeyType set;
          PopulateSetFromSource(set);
          UserOpenInfoView(set, AInfoDialog::Page_TechInfo);
        }
        break;

       case IDM_PL_EDIT:
        UserEditMeta();
        break;

       case IDM_PL_REMOVE:
        Apply2Source(&PlaylistBase::UserRemove);
        break;

       case IDM_PL_FLATTEN_1:
        Apply2Source(&PlaylistBase::UserFlatten);
        break;
       case IDM_PL_FLATTEN_ALL:
        Apply2Source(&PlaylistBase::UserFlattenAll);
        break;

       case IDM_PL_SORT_URLALL:
        SortComparer = &PlaylistBase::CompURL;
        UserSort(*Content);
        break;
       case IDM_PL_SORT_URL:
        SortComparer = &PlaylistBase::CompURL;
        Apply2Source(&PlaylistBase::UserSort);
        break;
       case IDM_PL_SORT_SONGALL:
        SortComparer = &PlaylistBase::CompTitle;
        UserSort(*Content);
        break;
       case IDM_PL_SORT_SONG:
        SortComparer = &PlaylistBase::CompTitle;
        Apply2Source(&PlaylistBase::UserSort);
        break;
       case IDM_PL_SORT_ARTALL:
        SortComparer = &PlaylistBase::CompArtist;
        UserSort(*Content);
        break;
       case IDM_PL_SORT_ART:
        SortComparer = &PlaylistBase::CompArtist;
        Apply2Source(&PlaylistBase::UserSort);
        break;
       case IDM_PL_SORT_ALBUMALL:
        SortComparer = &PlaylistBase::CompAlbum;
        UserSort(*Content);
        break;
       case IDM_PL_SORT_ALBUM:
        SortComparer = &PlaylistBase::CompAlbum;
        Apply2Source(&PlaylistBase::UserSort);
        break;
       case IDM_PL_SORT_ALIASALL:
        SortComparer = &PlaylistBase::CompAlias;
        UserSort(*Content);
        break;
       case IDM_PL_SORT_ALIAS:
        SortComparer = &PlaylistBase::CompAlias;
        Apply2Source(&PlaylistBase::UserSort);
        break;
       case IDM_PL_SORT_TIMEALL:
        SortComparer = &PlaylistBase::CompTime;
        UserSort(*Content);
        break;
       case IDM_PL_SORT_TIME:
        SortComparer = &PlaylistBase::CompTime;
        Apply2Source(&PlaylistBase::UserSort);
        break;
       case IDM_PL_SORT_SIZEALL:
        SortComparer = &PlaylistBase::CompSize;
        UserSort(*Content);
        break;
       case IDM_PL_SORT_SIZE:
        SortComparer = &PlaylistBase::CompSize;
        Apply2Source(&PlaylistBase::UserSort);
        break;

       case IDM_PL_SORT_RANDALL:
        UserShuffle(*Content);
        break;
       case IDM_PL_SORT_RAND:
        Apply2Source(&PlaylistBase::UserShuffle);
        break;

       case IDM_PL_CLEARALL:
        UserClearPlaylist(*Content);
        break;

       /*case IDM_PL_CLEAR:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_CLEAR %p\n", this, DebugName().cdata(), focus));
          for (RecordBase** rpp = source.begin(); rpp != source.end(); ++rpp)
          { Playable* pp = (*rpp)->Data->Content->GetPlayable();
            if ((pp->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
              ((Playlist*)pp)->Clear();
          }
          return 0;
        }*/
       case IDM_PL_RELOAD:
        UserReload(*Content);
        break;

       case IDM_PL_OPEN:
        { url123 URL = amp_playlist_select(GetHwnd(), "Open Playlist");
          if (URL)
          { PlaylistBase* pp = GetSame(*Playable::GetByURL(URL));
            pp->SetVisible(true);
          }
          break;
        }
       case IDM_PL_SAVE:
        UserSave(false);
        break;
       case IDM_PL_SAVE_AS:
        UserSave(true);
        break;

       case IDM_M_INSPECTOR:
        InspectorDialog::GetInstance()->SetVisible(true);
        break;

      } // switch (cmd)
      return 0;
    }

   case UM_RECORDUPDATE:
    { RecordBase* rec = (RecordBase*)PVOIDFROMMP(mp1);
      DEBUGLOG(("PlaylistBase::DlgProc: UM_RECORDUPDATE: %s, %i, %x\n",
        RecordBase::DebugName(rec).cdata(), SHORT1FROMMP(mp2), StateFromRec(rec).UpdateFlags.get()));
      UpdateRecord(rec);
      // Free the record in case this is requested.
      if (SHORT1FROMMP(mp2))
        FreeRecord(rec);
      return MRFROMLONG(TRUE);
    }

   case UM_DELETERECORD:
    DEBUGLOG(("PlaylistBase::DlgProc: UM_DELETERECORD: %s\n", RecordBase::DebugName((RecordBase*)PVOIDFROMMP(mp1)).cdata()));
    DeleteEntry((RecordBase*)PVOIDFROMMP(mp1));
    return MRFROMLONG(TRUE);

   case UM_PLAYSTATUS:
    UpdatePlayStatus();
    return 0;

   case UM_INSERTITEM:
    { InsertInfo* pii = (InsertInfo*)PVOIDFROMMP(mp1);
      UserInsert(pii);
      delete pii;
      return 0;
    }

   case UM_REMOVERECORD:
    { RecordBase* rec = (RecordBase*)PVOIDFROMMP(mp1);
      UserRemove(rec);
      FreeRecord(rec);
      return 0;
    }

   case UM_UPDATEDEC:
    { DEBUGLOG(("PlaylistBase::DlgProc: UM_UPDATEDEC\n"));
      UpdateAccelTable();
      // Replace table of current window.
      HAB hab = WinQueryAnchorBlock(GetHwnd());
      HACCEL haccel = WinQueryAccelTable(hab, GetHwnd());
      PMRASSERT(WinSetAccelTable(hab, AccelTable, GetHwnd()));
      if (haccel != NULLHANDLE)
        PMRASSERT(WinDestroyAccelTable(haccel));
      // done
      // Notify context menu handlers
      DecChanged = true;
      return 0;
    }
  }
  return ManagedDialog<DialogBase>::DlgProc(msg, mp1, mp2);
}

PlaylistBase::IC PlaylistBase::GetRecordUsage(const RecordBase* rec) const
{ DEBUGLOG(("PlaylistBase::GetRecordUsage(%s)\n", RecordBase::DebugName(rec).cdata()));
  PlayableInstance& pi = *rec->Data->Content;
  unsigned usage = pi.GetInUse();
  if (!usage)
  { unsigned tattr = pi.GetInfo().tech->attributes;
    if (tattr & TATTR_INVALID)
      return IC_Invalid;
    if (tattr & (TATTR_SONG|TATTR_PLAYLIST))
      return IC_Normal;
    return IC_Pending;
  }
  // Check whether the current call stack is the same as for the current Record ...
  do
  { rec = GetParent(rec);
    if (APlayableFromRec(rec).GetInUse() != --usage)
      return IC_Shadow;
  } while (rec != NULL && usage);
  return Ctrl::IsPlaying() ? IC_Play : IC_Active;
}

HPOINTER PlaylistBase::CalcIcon(RecordBase* rec)
{ ASSERT(rec->Data->Content != NULL);
  PlayableInstance& pi = *rec->Data->Content;
  unsigned tattr = pi.GetInfo().tech->attributes;
  DEBUGLOG(("PlaylistBase::CalcIcon(%s) - %u\n", RecordBase::DebugName(rec).cdata(), tattr));
  IC state = GetRecordUsage(rec);
  if (tattr & TATTR_PLAYLIST)
    return IcoPlaylist[(tattr & TATTR_WRITABLE) == 0][state][GetPlaylistType(rec)];
  else
    return IcoSong[state];
}

void PlaylistBase::SetTitle()
{ DEBUGLOG(("PlaylistBase(%p)::SetTitle()\n", this));
  // Generate Window Title
  const char* append = "";
  unsigned tattr = Content->GetInfo().tech->attributes;
  if (tattr & TATTR_INVALID)
    append = " [invalid]";
  else if (Content->GetInUse())
    append = " [used]";
  DialogBase::SetTitle(xstring().sprintf("PM123: %s%s%s", Content->GetDisplayName().cdata(),
    (tattr & TATTR_PLAYLIST) && Content->IsModified() ? " (*)" : "", append));
}

PlaylistMenu& PlaylistBase::GetMenuWorker()
{ if (MenuWorker == NULL)
    MenuWorker = new PlaylistMenu(GetHwnd(), IDM_M_LAST, IDM_M_LAST_E);
  return *MenuWorker;
}


PlaylistBase::RecordBase* PlaylistBase::AddEntry(PlayableInstance& obj, RecordBase* parent, RecordBase* after)
{ DEBUGLOG(("PlaylistBase(%p{%s})::AddEntry(&%p{%s}, %p, %p)\n", this, DebugName().cdata(), &obj, obj.DebugName().cdata(), parent, after));
  /* Allocate a record in the HwndContainer */
  RecordBase* rec = CreateNewRecord(obj, parent);
  if (rec)
  { RECORDINSERT insert      = { sizeof(RECORDINSERT) };
    if (after)
    { insert.pRecordOrder    = (RECORDCORE*)after;
      //insert.pRecordParent = NULL;
    } else
    { insert.pRecordOrder    = (RECORDCORE*)CMA_FIRST;
      insert.pRecordParent   = (RECORDCORE*)parent;
    }
    insert.fInvalidateRecord = TRUE;
    insert.zOrder            = CMA_TOP;
    insert.cRecordsInsert    = 1;
    PMXASSERT(LONGFROMMR(WinSendMsg(HwndContainer, CM_INSERTRECORD, MPFROMP(rec), MPFROMP(&insert))), != 0);
    DEBUGLOG(("PlaylistBase::AddEntry: succeeded: %p\n", rec));
  }
  return rec;
}

PlaylistBase::RecordBase* PlaylistBase::MoveEntry(RecordBase* entry, RecordBase* parent, RecordBase* after)
{ DEBUGLOG(("PlaylistBase(%p{%s})::MoveEntry(%s, %s, %s)\n", this, DebugName().cdata(),
    RecordBase::DebugName(entry).cdata(), RecordBase::DebugName(parent).cdata(), RecordBase::DebugName(after).cdata()));
  TREEMOVE move =
  { (RECORDCORE*)entry,
    (RECORDCORE*)parent,
    (RECORDCORE*)(after ? after : (RecordBase*)CMA_FIRST),
    FALSE
  };
  PMRASSERT(WinSendMsg(HwndContainer, CM_MOVETREE, MPFROMP(&move), 0));
  return entry;
}

void PlaylistBase::RemoveEntry(RecordBase* const entry)
{ DEBUGLOG(("PlaylistBase(%p{%s})::RemoveEntry(%p)\n", this, DebugName().cdata(), entry));
  // deregister events
  entry->Data->InfoChange.detach();
  // Delete the children
  RemoveChildren(entry);
  // Remove record from container
  PMXASSERT(WinSendMsg(HwndContainer, CM_REMOVERECORD, MPFROMP(&entry), MPFROM2SHORT(1, CMA_INVALIDATE)), != (MRESULT)-1);
  // Release reference counter
  // The record will be deleted when no outstanding PostMsg is on the way.
  FreeRecord(entry);
  DEBUGLOG(("PlaylistBase::RemoveEntry completed\n"));
}

int PlaylistBase::RemoveChildren(RecordBase* const rp)
{ DEBUGLOG(("PlaylistBase(%p)::RemoveChildren(%p)\n", this, rp));
  RecordBase* crp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(rp ? CMA_FIRSTCHILD : CMA_FIRST, CMA_ITEMORDER));
  PMASSERT(crp != (RecordBase*)-1);
  int count = 0;
  while (crp != NULL && crp != (RecordBase*)-1)
  { DEBUGLOG(("PlaylistBase::RemoveChildren: CM_QUERYRECORD: %s\n", RecordBase::DebugName(crp).cdata()));
    RecordBase* crp2 = crp;
    crp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    PMASSERT(crp != (RecordBase*)-1);
    RemoveEntry(crp2);
    ++count;
  }
  return count;
}

void PlaylistBase::UpdateChildren(RecordBase* const rp)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateChildren(%s)\n", this, RecordBase::DebugName(rp).cdata()));
  // get content
  APlayable& pp = APlayableFromRec(rp);
  TECH_ATTRIBUTES tattr = (TECH_ATTRIBUTES)pp.GetInfo().tech->attributes;
  DEBUGLOG(("PlaylistBase::UpdateChildren - %x\n", tattr));
  if ((tattr & (TATTR_PLAYLIST|TATTR_INVALID)) != TATTR_PLAYLIST)
  { // Nothing to enumerate, delete children if any
    DEBUGLOG(("PlaylistBase::UpdateChildren - no children possible.\n"));
    if (RemoveChildren(rp))
    { InfoFlags req = IF_Usage; // update icon
      FilterRecordRequest(rp, req);
      PostRecordUpdate(rp, req);
    }
    return;
  }

  // Check whether to request state information immediately.
  bool reqstate;
  if (rp == NULL)
    // Root node -> always yes
    reqstate = true;
  else
  { // If not the root check if the parent is expanded
    PMRASSERT(WinSendMsg(HwndContainer, CM_QUERYRECORDINFO, MPFROMP(&(MINIRECORDCORE*&)rp), MPFROMSHORT(1)));
    reqstate = (rp->flRecordAttr & CRA_EXPANDED) != 0;
  }

  // Check whether we should suspend the redraw.
  bool pauseredraw = false;
  if (rp == NULL || rp->flRecordAttr & CRA_EXPANDED)
  { pauseredraw = true;
    PMRASSERT(WinEnableWindowUpdate(HwndContainer, FALSE)); // suspend redraw
  }

  // First check what's currently in the container.
  // The collection is /not/ locked so far.
  vector<RecordBase> old(32);
  RecordBase* crp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(rp ? CMA_FIRSTCHILD : CMA_FIRST, CMA_ITEMORDER));
  while (crp != NULL && crp != (RecordBase*)-1)
  { DEBUGLOG(("PlaylistBase::UpdateChildren CM_QUERYRECORD: %s\n", RecordBase::DebugName(crp).cdata()));
    // prefetch next record
    old.append() = crp;
    crp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
  }
  PMASSERT(crp != (RecordBase*)-1);

  { // Now check what should be in the container
    DEBUGLOG(("PlaylistBase::UpdateChildren - check container.\n"));
    Playable& list = pp.GetPlayable();
    Mutex::Lock lock(list.Mtx); // Lock the collection
    int_ptr<PlayableInstance> pi;
    crp = NULL; // Last entry, insert new items after that.
    while ((pi = list.GetNext(pi)) != NULL)
    { // request state?
      if (reqstate)
        pi->GetPlayable().RequestInfo(IF_Tech, PRI_Normal);
      // Find entry in the current content
      RecordBase** orpp = old.begin();
      for (;;)
      { if (orpp == old.end())
        { // not found! => add
          DEBUGLOG(("PlaylistBase::UpdateChildren - not found: %p{%s}\n", pi.get(), pi->DebugName().cdata()));
          crp = AddEntry(*pi, rp, crp);
          if (crp && (crp->flRecordAttr & CRA_EXPANDED))// (rp == NULL || (rp->flRecordAttr & CRA_EXPANDED)))
            PostRecordUpdate(crp, RequestRecordInfo(crp, IF_Child));
          break;
        }
        if ((*orpp)->Data->Content == pi)
        { // found!
          DEBUGLOG(("PlaylistBase::UpdateChildren - found: %p{%s} at %u\n", pi.get(), pi->DebugName().cdata(), orpp - old.begin()));
          if (orpp == old.begin())
            // already in right order
            crp = *orpp;
          else
            // move
            crp = MoveEntry(*orpp, rp, crp);
          // Remove from old queue
          old.erase(orpp - old.begin());
          break;
        }
        ++orpp;
      }
    }
  }
  // delete remaining records
  DEBUGLOG(("PlaylistBase::UpdateChildren - old.size() = %u\n", old.size()));
  for (RecordBase** orpp = old.begin(); orpp != old.end(); ++orpp)
    RemoveEntry(*orpp);
  // resume redraw
  if (pauseredraw)
    PMRASSERT(WinShowWindow(HwndContainer, TRUE));
  //PMRASSERT(WinEnableWindowUpdate(HwndContainer, TRUE));
}

void PlaylistBase::GetRecords(USHORT emphasis)
{ Source.clear();
  RecordBase* rec = (RecordBase*)PVOIDFROMMR(WinSendMsg(HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMP(CMA_FIRST), MPFROMSHORT(emphasis)));
  while (rec != NULL && rec != (RecordBase*)-1)
  { DEBUGLOG(("PlaylistBase::GetRecords: %p\n", rec));
    Source.append() = rec;
    rec = (RecordBase*)PVOIDFROMMR(WinSendMsg(HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMP(rec), MPFROMSHORT(emphasis)));
  }
  PMASSERT(rec != (RecordBase*)-1);
}

bool PlaylistBase::GetSource(RecordBase* rec)
{ Source.clear();
  if (rec)
  { PMRASSERT(WinSendMsg(HwndContainer, CM_QUERYRECORDINFO, MPFROMP(&rec), MPFROMSHORT(1)));
    DEBUGLOG(("PlaylistBase::GetSource(%p{...%x...})\n", rec, rec->flRecordAttr));
    // check wether the record is selected
    if (rec->flRecordAttr & CRA_SELECTED)
    { GetRecords(CRA_SELECTED);
      if (Source.size() == 0)
        return false;
    } else
      Source.append() = rec;
  }
  return true;
}

/*void PlaylistBase::Apply2Source(void (*op)(Playable*)) const
{ DEBUGLOG(("PlaylistBase(%p)::Apply2Source(%p) - %u\n", this, op, Source.size()));
  for (RecordBase*const* rpp = Source.begin(); rpp != Source.end(); ++rpp)
    (*op)((*rpp)->Data->Content->GetPlayable());
}*/
void PlaylistBase::Apply2Source(void (PlaylistBase::*op)(Playable&))
{ DEBUGLOG(("PlaylistBase(%p)::Apply2Source(Playable::%p) - %u\n", this, op, Source.size()));
  for (RecordBase*const* rpp = Source.begin(); rpp != Source.end(); ++rpp)
    (this->*op)((*rpp)->Data->Content->GetPlayable());
}
void PlaylistBase::Apply2Source(void (PlaylistBase::*op)(RecordBase*))
{ DEBUGLOG(("PlaylistBase(%p)::Apply2Source(RecorsBase::%p) - %u\n", this, op, Source.size()));
  for (RecordBase*const* rpp = Source.begin(); rpp != Source.end(); ++rpp)
    (this->*op)(*rpp);
}

void PlaylistBase::PopulateSetFromSource(AInfoDialog::KeyType& dest)
{ const RecordBase* parent = NULL;
  int state = 0; // 0 = virgin, 1 = assigned, 2 = non-unique
  for (const RecordBase*const* rpp = Source.begin(); rpp != Source.end(); ++rpp)
  { PlayableInstance& p = *(*rpp)->Data->Content;
    dest.get(p) = &p;
    const RecordBase* prec = GetParent(*rpp);
    if (!state)
    { parent = prec;
      state = 1;
    } else if (parent != prec)
      state = 2;
  }
  // unique parent?
  if (state == 1)
    dest.Parent = &PlayableFromRec(parent);
}

void PlaylistBase::SetEmphasis(USHORT emphasis, bool set) const
{ DEBUGLOG(("PlaylistBase(%p)::SetEmphasis(%x, %u) - %u\n", this, emphasis, set, Source.size()));
  if (Source.size() == 0)
    PMRASSERT(WinSendMsg(HwndContainer, CM_SETRECORDEMPHASIS, MPFROMP(NULL), MPFROM2SHORT(set, emphasis)));
  else
  { for (RecordBase*const* rpp = Source.begin(); rpp != Source.end(); ++rpp)
    { DEBUGLOG(("PlaylistBase::SetEmphasis: %p\n", *rpp));
      PMRASSERT(WinSendMsg(HwndContainer, CM_SETRECORDEMPHASIS, MPFROMP(*rpp), MPFROM2SHORT(set, emphasis)));
    }
    // Somtimes the update is done automatically, sometimes not ...
    if (!set)
      PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(Source.begin()), MPFROM2SHORT(Source.size(), CMA_NOREPOSITION|CMA_ERASE)));
  }
}

PlaylistBase::RecordType PlaylistBase::AnalyzeRecordTypes() const
{ RecordType ret = RT_None;
  for (RecordBase*const* rpp = Source.end(); rpp-- != Source.begin(); )
  { const INFO_BUNDLE_CV& info = (*rpp)->Data->Content->GetInfo();
    unsigned pattr = info.phys->attributes;
    unsigned tattr = info.tech->attributes;
    if (tattr & TATTR_INVALID)
    { ret |= RT_Invld;
      continue;
    }
    bool writable = (tattr & TATTR_WRITABLE) && (pattr & PATTR_WRITABLE);
    if (tattr & TATTR_SONG)
      ret |= writable ? RT_Meta : RT_Song;
    if (tattr & TATTR_PLAYLIST)
      ret |= writable ? RT_List : RT_Enum;
    if ((tattr & (TATTR_SONG|TATTR_PLAYLIST|TATTR_INVALID)) == TATTR_NONE)
      ret |= RT_Unknwn;
  }
  DEBUGLOG(("PlaylistBase::AnalyzeRecordTypes(): %u, %x\n", Source.size(), ret));
  return ret;
}

bool PlaylistBase::IsUnderCurrentRoot(RecordBase* rec) const
{ DEBUGLOG(("PlaylistBase::IsUnderCurrentRoot(%p)\n", rec));
  unsigned usage = 0;
  // check stack
  for(;;)
  { unsigned recusage = APlayableFromRec(rec).GetInUse();
    if (!usage)
      usage = recusage;
    else if (recusage != --usage)
      return false; // Only a shadow
    else if (!usage)
      return true;
    if (!rec)
      return !!usage;
    rec = GetParent(rec);
  }
}


void PlaylistBase::UpdateIcon(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateIcon(%p)\n", this, rec));
  if (rec == NULL)
  { // Root node => update title
    SetTitle();
  } else
  { // Record node
    HPOINTER icon = CalcIcon(rec);
    // update icon?
    if (rec->hptrIcon != icon)
    { rec->hptrIcon = icon;
      PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_NOREPOSITION)));
    }
  }
}

void PlaylistBase::UpdatePlayStatus()
{ DEBUGLOG(("PlaylistBase(%p)::UpdatePlayStatus()\n", this));
  RecordBase* rec = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, 0, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
  while (rec != NULL && rec != (RecordBase*)-1)
  { UpdatePlayStatus(rec);
    rec = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rec), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
  }
  PMASSERT(rec != (RecordBase*)-1);
}
void PlaylistBase::UpdatePlayStatus(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UpdatePlayStatus(%p)\n", this, rec));
  if (rec->Data->Content->GetPlayable().GetInUse())
  { InfoFlags req = IF_Usage;
    FilterRecordRequest(rec, req);
    PostRecordUpdate(rec, req);
  }
}

void PlaylistBase::InfoChangeEvent(const PlayableChangeArgs& args, RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p{%s})::InfoChangeEvent({%p{%s},, %x, %x,}, %s)\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.DebugName().cdata(), args.Changed, args.Loaded, RecordBase::DebugName(rec).cdata()));
  // Filter events
  InfoFlags req = args.Changed;
  FilterRecordRequest(rec, req);
  // request invalidated infos if needed
  req |= args.Invalidated & ~RequestRecordInfo(rec, args.Invalidated);
  PostRecordUpdate(rec, req);
}

void PlaylistBase::PlayStatEvent(const Ctrl::EventFlags& flags)
{ if (flags & Ctrl::EV_PlayStop)
  { // Cancel any outstanding UM_PLAYSTATUS ...
    QMSG qmsg;
    WinPeekMsg(amp_player_hab, &qmsg, GetHwnd(), UM_PLAYSTATUS, UM_PLAYSTATUS, PM_REMOVE);
    // ... and send a new one.
    PMRASSERT(WinPostMsg(GetHwnd(), UM_PLAYSTATUS, MPFROMLONG(Ctrl::IsPlaying()), 0));
  }
}

void PlaylistBase::PluginEvent(const PluginEventArgs& args)
{ DEBUGLOG(("PlaylistBase(%p)::PluginEvent({&%p{%s}, %i})\n", this,
    args.Plug, args.Plug ? args.Plug->ModRef->Key.cdata() : "", args.Operation));
  if (args.Type == PLUGIN_DECODER)
  { switch (args.Operation)
    {case PluginEventArgs::Enable:
     case PluginEventArgs::Disable:
     case PluginEventArgs::Unload:
      WinPostMsg(GetHwnd(), UM_UPDATEDEC, 0, 0);
     default:; // avoid warnings
    }
  }
}

static void DLLENTRY UserAddCallback(void* param,
  const char* url, const INFO_BUNDLE* info, int cached, int reliable)
{ DEBUGLOG(("UserAddCallback(%p, %s,, %x, %x)\n", param, url, cached, reliable));
  PlaylistBase::UserAddCallbackParams& ucp = *(PlaylistBase::UserAddCallbackParams*)param;

  int_ptr<Playable> ip = Playable::GetByURL(url123(url));
  ip->SetCachedInfo(*info, (InfoFlags)cached, (InfoFlags)reliable);
  
  PlayableRef* pr = ucp.Content.append() = new PlayableRef(*ip);
  pr->OverrideInfo(*info, (InfoFlags)(cached&reliable));
}

void PlaylistBase::UserAdd(DECODER_WIZARD_FUNC wizard, RecordBase* parent, RecordBase* before)
{ DEBUGLOG(("PlaylistBase(%p)::UserAdd(%p, %p, %p)\n", this, wizard, parent, before));
  // Check
  APlayable& p = APlayableFromRec(parent);
  if (p.GetInfo().tech->attributes & TATTR_SONG)
    return; // Can't add something to a song.

  // Dialog
  UserAddCallbackParams ucp(this, parent, before); // Implicitly locks the records
  const xstring& title = "Append%s to " + APlayableFromRec(parent).GetDisplayName();
  ULONG ul = (*wizard)(GetHwnd(), title, &UserAddCallback, &ucp);
  DEBUGLOG(("PlaylistBase::UserAdd - %u\n", ul));

  if (ul == PLUGIN_OK && ucp.Content.size())
  { Playable& list = p.GetPlayable();
    Mutex::Lock lock(list.Mtx);
    for (const int_ptr<PlayableRef>* pps = ucp.Content.begin(); pps != ucp.Content.end(); ++pps)
      list.InsertItem(**pps, before ? before->Data->Content.get() : NULL); 
  }
  // TODO: cfg.listdir obsolete?
  //sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
}

void PlaylistBase::UserInsert(const InsertInfo* pii)
{ DEBUGLOG(("PlaylistBase(%p)::UserInsert(%p{{%s}, %p{%s}, %p{%s}})\n", this,
    pii, pii->Parent->DebugName().cdata(),
    pii->Before.get(), pii->Before->DebugName().cdata(),
    pii->Item.get(), pii->Item->DebugName().cdata()));
  pii->Parent->InsertItem(*pii->Item, pii->Before);
}

void PlaylistBase::UserRemove(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UserRemove(%s)\n", this, rec->DebugName().cdata()));
  // find parent playlist
  APlayable& p = APlayableFromRec(GetParent(rec));
  //DEBUGLOG(("PlaylistBase::UserRemove %s %p %p\n", RecordBase::DebugName(parent).cdata(), parent, playlist));
  if (p.GetInfo().tech->attributes & TATTR_PLAYLIST) // don't modify songs
    p.GetPlayable().RemoveItem(*rec->Data->Content);
    // the update of the container is implicitely done by the notification mechanism
}

void PlaylistBase::UserFlatten(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UserFlatten(%s)\n", this, rec->DebugName().cdata()));
  ASSERT(rec);
  // find parent playlist
  APlayable& parent = APlayableFromRec(GetParent(rec));
  APlayable& subitem = *rec->Data->Content;
  if ( (parent.GetInfo().tech->attributes & TATTR_PLAYLIST) // don't modify songs
    && (subitem.GetInfo().tech->attributes & TATTR_PLAYLIST) // and only flatten playlists
    && subitem.GetPlayable() != parent.GetPlayable() ) // and not recursive
  { Playable& par_list = parent.GetPlayable();
    Mutex::Lock lock(par_list.Mtx); // lock collection and avoid unneccessary events
    if (!rec->Data->Content->HasParent(&par_list))
      return; // somebody else has been faster

    // insert subitems before the old one
    Playable& sub_list = subitem.GetPlayable();
    int_ptr<PlayableInstance> pi = NULL;
    for(;;)
    { pi = sub_list.GetNext(pi);
      if (pi == NULL)
        break;
      par_list.InsertItem(*pi, rec->Data->Content);
    }

    // then delete the old one
    par_list.RemoveItem(*rec->Data->Content);
  } // the update of the container is implicitely done by the notification mechanism
}

void PlaylistBase::UserFlattenAll(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UserFlattenAll(%s)\n", this, rec->DebugName().cdata()));
  ASSERT(rec);
  // find parent playlist
  APlayable& parent = APlayableFromRec(GetParent(rec));
  APlayable& subitem = *rec->Data->Content;
  if ( (parent.GetInfo().tech->attributes & TATTR_PLAYLIST) // don't modify songs
    && (subitem.GetInfo().tech->attributes & TATTR_PLAYLIST) // and only flatten playlists
    && subitem.GetPlayable() != parent.GetPlayable() ) // and not recursive
  { // fetch desired content before we lock the collection to avoid deadlocks
    Playable& par_list = parent.GetPlayable();
    vector_int<PlayableRef> new_items;
    { Location si(&par_list);
      if (si.NavigateTo(rec->Data->Content))
        // Error sub-item is no longer part of the playlist.
        return;
      JobSet job(PRI_Normal);
      for (;;)
      { Location::NavigationResult rc = si.NavigateCount(job, 1, TATTR_SONG, 1);
        if (rc)
        { if (rc.length() == 0)
            // TODO: dependency ???
            return;
          else
            break;
        }
        new_items.append() = (PlayableRef*)si.GetCurrent();
        DEBUGLOG(("PlaylistBase::UserFlattenAll found %p\n", &*new_items[new_items.size()-1]));
      }
    }
    DEBUGLOG(("PlaylistBase::UserFlattenAll replacing by %u items\n", new_items.size()));

    Mutex::Lock lock(par_list.Mtx); // lock collection and avoid unnecessary events
    if (!rec->Data->Content->HasParent(&par_list))
      return; // somebody else has been faster
    // insert new sub-items before the old one
    for (const int_ptr<PlayableRef>* pps = new_items.begin(); pps != new_items.end(); ++pps)
      par_list.InsertItem(**pps, rec->Data->Content);
    // then delete the old one
    par_list.RemoveItem(*rec->Data->Content);

  } // the update of the container is implicitly done by the notification mechanism
}

void PlaylistBase::UserSave(bool saveas)
{ DEBUGLOG(("PlaylistBase(%p)::UserSave(%u)\n", this, saveas));
  amp_save_playlist(GetHwnd(), *Content, saveas);
  // TODO change window root in case of success.
}

void PlaylistBase::UserNavigate(const RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UserNavigate(%p)\n", this, rec));
  // Build call stack from record
  vector<PlayableInstance> callstack;
  // find root and collect call stack
  do
  { callstack.append() = rec->Data->Content;
    rec = GetParent(rec);
  } while (rec);
  // Now callstack contains all sub items within this root.

  // Create location object
  JobSet noblockjob(PRI_None);
  sco_ptr<Location> loc(new Location(Content));
  for (size_t i = callstack.size(); i--; )
  { if (loc->NavigateInto(noblockjob))
    { DEBUGLOG(("PlaylistMenu::SelectEntry -> NavigateInto failed."));
      return;
    }
    if (loc->NavigateTo(callstack[i]))
    { DEBUGLOG(("PlaylistMenu::SelectEntry -> NavigateTo failed."));
      return;
    }
  }
  callstack.clear();

  DEBUGLOG(("PlaylistBase::UserNavigate - %s\n", loc->Serialize().cdata()));
  Ctrl::PostCommand(Ctrl::MkJump(loc.detach()), &PlaylistBase::MsgJumpCompleted);
}

void PlaylistBase::UserOpenTreeView(Playable& p)
{ if (p.GetInfo().tech->attributes & TATTR_PLAYLIST)
    PlaylistManager::GetByKey(p)->SetVisible(true);
}

void PlaylistBase::UserOpenDetailedView(Playable& p)
{ if (p.GetInfo().tech->attributes & TATTR_PLAYLIST)
    PlaylistView::GetByKey(p)->SetVisible(true);
}

void PlaylistBase::UserOpenInfoView(const AInfoDialog::KeyType& set, AInfoDialog::PageNo page)
{ AInfoDialog::GetByKey(set)->ShowPage(page);
}

void PlaylistBase::UserClearPlaylist(Playable& p)
{ if (p.GetInfo().tech->attributes & TATTR_PLAYLIST)
    p.Clear();
}

void PlaylistBase::UserReload(Playable& p)
{ if ( !(p.GetInfo().tech->attributes & TATTR_PLAYLIST)
    || !p.IsModified()
    || amp_query(GetHwnd(), "The current list is modified. Discard changes?") )
    p.RequestInfo(IF_Decoder|IF_Aggreg|IF_Display|IF_Usage, PRI_Normal, REL_Reload);
}

void PlaylistBase::UserEditMeta()
{ { // Check meta_write
    const RecordBase*const* rpp = Source.end();
    while (rpp-- != Source.begin())
      if (!(*rpp)->Data->Content->GetInfo().tech->attributes & TATTR_WRITABLE)
        return; // Can't modify this item
  }
  switch (Source.size())
  {default: // multiple items
    { AInfoDialog::KeyType set;
      PopulateSetFromSource(set);
      AInfoDialog::GetByKey(set)->ShowPage(AInfoDialog::Page_MetaInfo);
    }
    break;
   case 1:
    GUI::ShowDialog(Source[0]->Data->Content->GetPlayable(), GUI::DLT_INFOEDIT, DLA_SHOW);
   case 0:;
  }
}

void PlaylistBase::UserSort(Playable& p)
{ DEBUGLOG(("PlaylistBase(%p)::UserSort(&%p{%s})\n", this, &p, p.DebugName().cdata()));
  if (p.GetInfo().tech->attributes & TATTR_PLAYLIST)
    p.Sort(SortComparer);
}

void PlaylistBase::UserShuffle(Playable& p)
{ if (p.GetInfo().tech->attributes & TATTR_PLAYLIST)
    p.Shuffle();
}

int PlaylistBase::CompURL(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetPlayable().URL > r->GetPlayable().URL;
}

int PlaylistBase::CompTitle(const PlayableInstance* l, const PlayableInstance* r)
{ return xstring(l->GetInfo().meta->title).compareToI(xstring(r->GetInfo().meta->title)) > 0;
}

int PlaylistBase::CompArtist(const PlayableInstance* l, const PlayableInstance* r)
{ return xstring(l->GetInfo().meta->artist).compareToI(xstring(r->GetInfo().meta->artist)) > 0;
}

int PlaylistBase::CompAlbum(const PlayableInstance* l, const PlayableInstance* r)
{ return xstring(l->GetInfo().meta->album).compareToI(xstring(r->GetInfo().meta->album)) > 0;
}

int PlaylistBase::CompAlias(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetDisplayName() > r->GetDisplayName();
}

int PlaylistBase::CompSize(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetInfo().phys->filesize > r->GetInfo().phys->filesize;
}

int PlaylistBase::CompTime(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetInfo().obj->songlength > r->GetInfo().obj->songlength;
}

/* Drag and drop - Target side *********************************************/

MRESULT PlaylistBase::DragOver(DRAGINFO* pdinfo, RecordBase* target)
{ ScopedDragInfo di(pdinfo);
  if (!di.get())
    return MRFROM2SHORT(DOR_NEVERDROP, 0);
  DEBUGLOG(("PlaylistBase::DragOver(%p{,,%x, %p, %i,%i, %u,}, %s) - %u\n",
    pdinfo, di->usOperation, di->hwndSource, di->xDrop, di->yDrop, di->cditem,
    RecordBase::DebugName(target).cdata(), DragAfter));
  // Check source items
  USHORT drag    = DOR_DROP;
  USHORT drag_op = DO_DEFAULT;
  for (USHORT i = 0; i < di->cditem; ++i)
  { DragItem pditem = di[i];
    DEBUGLOG(("PlaylistBase::DragOver: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, pditem.Type().cdata(), pditem.RMF().cdata(),
      DrgQueryStrXName(pditem->hstrContainerName).cdata(), pditem.SourceName().cdata(), pditem.TargetName().cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    // native PM123 object
    if (DrgVerifyRMF(pditem.get(), "DRM_123LST", NULL))
    { // Check for recursive operation
      if (!DragAfter && target && DrgQueryStrXName(pditem->hstrSourceName) == target->Data->Content->GetPlayable().URL)
      { drag = DOR_NODROP;
        break;
      }
      if (di->usOperation == DO_DEFAULT)
        drag_op = di->hwndSource == GetHwnd() ? DO_MOVE : DO_COPY;
      else if (di->usOperation == DO_COPY || di->usOperation == DO_MOVE)
        drag_op = di->usOperation;
      else
        drag = DOR_NODROPOP;
      continue;
    }
    // File system object?
    else if (DrgVerifyRMF(pditem.get(), "DRM_OS2FILE", NULL))
    { if ( (di->usOperation == DO_DEFAULT || di->usOperation == DO_LINK)
        && (pditem->fsSupportedOps & DO_LINKABLE) )
        drag_op = DO_LINK;
      else
        drag = DOR_NODROPOP;
      continue;
    }
    // invalid object
    return MRFROM2SHORT(DOR_NEVERDROP, 0);
  }
  // Check target
  if ( ((DragAfter ? *Content : APlayableFromRec(target)).GetInfo().tech->attributes & TATTR_PLAYLIST) == 0 )
    drag = DOR_NODROP;
  // finished
  return MRFROM2SHORT(drag, drag_op);
}

void PlaylistBase::DragDrop(DRAGINFO* pdinfo_, RecordBase* target)
{ DEBUGLOG(("PlaylistBase::DragDrop(%p, %p)\n", pdinfo_, target));
  // The HandleDrop has implicit lifetime management, so we do not need to care about lifetime.
  int_ptr<HandleDrop> worker(new HandleDrop(pdinfo_, GetHwnd()));
  DEBUGLOG(("PlaylistBase::DragDrop(%p{,,%x, %p, %i,%i, %u,}, %s) - %u\n",
    pdinfo_, (*worker)->usOperation, (*worker)->hwndSource, (*worker)->xDrop, (*worker)->yDrop, (*worker)->cditem,
    RecordBase::DebugName(target).cdata(), DragAfter));

  // Locate parent
  worker->DropTarget = DragAfter ? Content.get() : &PlayableFromRec(target);
  // Locate insertion point
  if (DragAfter && target)
  { if (target == (RecordBase*)CMA_FIRST)
      target = NULL;
    // Search record after target, because the container returns the record /before/ the insertion point,
    // while PlayableCollection requires the entry /after/ the insertion point.
    target = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(target), MPFROM2SHORT(target ? CMA_NEXT : CMA_FIRST, CMA_ITEMORDER));
    PMASSERT(target != (RecordBase*)-1);
    if (target)
      worker->DropBefore = target->Data->Content;
  }

  // For each dropped item...
  for (size_t i = 0; i < (*worker)->cditem; ++i)
  { UseDragTransfer dt((*worker)[i]);
    DragItem item(dt.Item());
    DEBUGLOG(("PlaylistBase::DragDrop: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      item->hwndItem, item->ulItemID, item.Type().cdata(), item.RMF().cdata(),
      item.ContainerName().cdata(), item.SourceName().cdata(), item.TargetName().cdata(),
      item->cxOffset, item->cyOffset, item->fsControl, item->fsSupportedOps));

    if (item.VerifyRMF("DRM_OS2FILE", NULL))
    { // File source
      // fetch full qualified path
      xstring fullname = item.SourcePath();
      if (fullname)
      { // have file name => use target rendering
        DEBUGLOG(("PlaylistBase::DragDrop: DRM_OS2FILE && fullname - %x\n", item->fsControl));
        if (item.VerifyType("UniformResourceLocator"))
        { // have file name => use target rendering
          fullname = amp_url_from_file(fullname);
          if (!fullname)
            continue;
        }
        // Hopefully this criterion is sufficient to identify folders.
        if (item->fsControl & DC_CONTAINER)
          fullname = amp_make_dir_url(fullname, Cfg::Get().recurse_dnd); // TODO: should be alterable

        // prepare insert item
        InsertInfo* pii = new InsertInfo();
        pii->Parent = worker->DropTarget;
        pii->Before = worker->DropBefore;
        pii->Item   = new PlayableRef(*Playable::GetByURL(url123::normalizeURL(fullname)));
        if (item->hstrTargetName)
        { ItemInfo ii;
          ii.alias = item.TargetName();
          pii->Item->OverrideItem(&ii);
        }
        WinPostMsg(GetHwnd(), UM_INSERTITEM, MPFROMP(pii), 0);
        dt.EndConversation();
      }
      else if (item->hwndItem && item.VerifyType("UniformResourceLocator"))
      { // URL w/o file name => need source rendering
        DEBUGLOG(("PlaylistBase::DragDrop: DRM_OS2FILE && URL && !fullname - %x\n", item->fsControl));
        dt.SelectedRMF("<DRM_OS2FILE,DRF_TEXT>");
        dt.RenderTo(amp_dnd_temp_file().cdata() + 8);
      }
    }
    else if (item.VerifyRMF("DRM_123LST", NULL))
    { // PM123 source
      DEBUGLOG(("PlaylistBase::DragDrop: DRM_123LST - %x\n", item->fsControl));

      // Check whether the source is our own process.
      if (!WinIsMyProcess(item->hwndItem))
      { // Source is in another process => use source rendering
        dt.SelectedRMF("<DRM_123LST,DRF_UNKNOWN>");
        dt.RenderTo(amp_dnd_temp_file());
      }
      else
      { // The source is the same process => we can safely access the memory from the source.
        // Strictly speaking we should not do the processing in the DM_DROP handler,
        // but it is fast and does not cause I/O.
        vector<RecordBase>* source = (vector<RecordBase>*)item->ulItemID;
        if (!source)
          continue;
        // Now check whether it is not only the same process but also the same parent playlist.
        // If true and DO_MOVE => move in place.
        if ((*worker)->usOperation == DO_MOVE && item.VerifyContainerName(worker->DropTarget->URL))
        { foreach(RecordBase**, rpp, *source)
            worker->DropTarget->MoveItem(*(*rpp)->Data->Content, worker->DropBefore);
          // do not set success to indicate that the source should not remove the items.
        } else
        { foreach(RecordBase**, rpp, *source)
          worker->DropTarget->InsertItem(*(*rpp)->Data->Content, worker->DropBefore);
          // Set success flag only for DO_MOVE so the source knows when to remove the source items.
          // at DM_END_CONVERSATION.
          if ((*worker)->usOperation == DO_MOVE)
            dt.EndConversation();
        }
      }
    }
  }
  DEBUGLOG(("PlaylistBase::DragDrop completed\n"));
}

void PlaylistBase::DropRenderComplete(DRAGTRANSFER* pdtrans_, USHORT flags)
{ DEBUGLOG(("PlaylistBase(%p)::DropRenderComplete(%p, %x)\n", this, pdtrans_, flags));
  UseDragTransfer dt(pdtrans_, true);
  DEBUGLOG(("PlaylistBase::DropRenderComplete({, %x, %p{...}, %s, %s, %p, %i, %x}, )\n",
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

    // Remove temp file
    remove(dt.RenderToName());

    worker->DropTarget->InsertItem(*ap, worker->DropBefore);
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
      dt.Hold();
      AutoPostMsgWorker::Start(*pp, IF_Child, PRI_Normal,
        GetHwnd(), DM_RENDERCOMPLETE, MPFROMP(pdtrans_), MPFROMSHORT(flags|0x8000));
      return;
    }
    // Read playlist
    DEBUGLOG(("GUIImp::DropRenderComplete: execute\n"));
    int_ptr<PlayableInstance> pi;
    while ((pi = pp->GetNext(pi)) != NULL)
      worker->DropTarget->InsertItem(*pi, worker->DropBefore);

    // Remove temp file
    #ifndef DEBUG // keep in case of debug builds
    remove(url.cdata() + 8);
    #endif

    // Return success only in case of DO_MOVE.
    if ((*worker)->usOperation == DO_MOVE)
      dt.EndConversation(true);
  }
}

/* Drag and drop - Source side *********************************************/

void PlaylistBase::DragInit()
{ DEBUGLOG(("PlaylistBase::DragInit() - %u\n", Source.size()));

  // If the Source Array is empty, we must be over whitespace,
  // in which case we don't want to drag any records.
  if (Source.size() == 0)
    return;

  // Allocate an array of DRAGIMAGE structures. Each structure contains
  // info about an image that will be under the mouse pointer during the
  // drag. This image will represent a container record being dragged.
  sco_arr<DRAGIMAGE> drag_images(min(Source.size(), MAX_DRAG_IMAGES));

  for (size_t i = 0; i < Source.size(); ++i)
  { RecordBase* rec = Source[i];
    DEBUGLOG(("PlaylistBase::DragInit: init item %i: %s\n", i, rec->DebugName().cdata()));
    // Prevent the records from being disposed.
    // The records are normally freed by DropEnd, except in case DrgDrag returns with an error.
    BlockRecord(rec);

    if (i >= MAX_DRAG_IMAGES)
      continue;
    DRAGIMAGE& drag_image = drag_images[i];
    drag_image.cb       = sizeof(DRAGIMAGE);
    drag_image.hImage   = rec->hptrIcon;
    drag_image.fl       = DRG_ICON|DRG_MINIBITMAP;
    drag_image.cxOffset = 5 * i;
    drag_image.cyOffset = -5 * i;
  }
  SetEmphasis(CRA_SOURCE, true);

  // Let PM allocate enough memory for a DRAGINFO structure as well as
  // a DRAGITEM structure for each record being dragged. It will allocate
  // shared memory so other processes can participate in the drag/drop.
  // In fact all items render to a single DRAGITEM.
  ScopedDragInfo drag_infos(1);
  DragItem pditem = drag_infos[0];
  pditem->hwndItem          = GetHwnd();
  pditem->ulItemID          = (ULONG)&Source;
  //pditem->hstrType          = DrgAddStrHandle(DRT_UNKNOWN);
  pditem.RMF("(DRM_123LST,DRM_DISCARD)x(DRF_UNKNOWN)");
  pditem.ContainerName(Content->URL);
  pditem->fsControl         = DC_GROUP;
  pditem->fsSupportedOps    = DO_MOVEABLE|DO_COPYABLE|DO_LINKABLE;
  DEBUGLOG(("PlaylistBase::DragInit: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
    pditem->hwndItem, pditem->ulItemID, pditem.Type().cdata(), pditem.RMF().cdata(),
    pditem.ContainerName().cdata(), pditem.SourceName().cdata(), pditem.TargetName().cdata(),
    pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));


  // If DrgDrag returns NULLHANDLE, that means the user hit Esc or F1
  // while the drag was going on so the target didn't have a chance to
  // delete the string handles. So it is up to the source window to do
  // it. Unfortunately there doesn't seem to be a way to determine
  // whether the NULLHANDLE means Esc was pressed as opposed to there
  // being an error in the drag operation. So we don't attempt to figure
  // that out. To us, a NULLHANDLE means Esc was pressed...
  HWND target = drag_infos.Drag(GetHwnd(), drag_images.get(), drag_images.size());

  SetEmphasis(CRA_SOURCE, false);
  // release the records
  if (!target)
    for (RecordBase** prec = Source.begin(); prec != Source.end(); ++prec)
      FreeRecord(*prec);
}

ULONG PlaylistBase::DropDiscard(DRAGINFO* pdinfo)
{ ScopedDragInfo di(pdinfo);
  if (!di.get())
    return DRR_ABORT;
  DEBUGLOG(("PlaylistBase::DropDiscard(%p{,,%x, %p, %i,%i, %u,})\n",
    pdinfo, di->usOperation, di->hwndSource, di->xDrop, di->yDrop, di->cditem));

  for (USHORT i = 0; i < di->cditem; ++i)
  { DragItem pditem = di[i];
    DEBUGLOG(("PlaylistBase::DropDiscard: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, pditem.Type().cdata(), pditem.RMF().cdata(),
      pditem.ContainerName().cdata(), pditem.SourceName().cdata(), pditem.TargetName().cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    // get record
    RecordBase* rec = (RecordBase*)pditem->ulItemID;
    // Remove object later
    BlockRecord(rec);
    PostMsg(UM_REMOVERECORD, MPFROMP(rec), 0);
  }
  return DRR_SOURCE;
}

BOOL PlaylistBase::DropRender(DragTransfer pdtrans)
{ DEBUGLOG(("PlaylistBase(%p)::DropRender(%p{, %x, %p{...}, %s, %s, %p, %i, %x})\n", this,
    pdtrans.get(), pdtrans->hwndClient, pdtrans->pditem, pdtrans.SelectedRMF().cdata(), pdtrans.RenderToName().cdata(),
    pdtrans->ulTargetInfo, pdtrans->usOperation, pdtrans->fsReply));
  DEBUGLOG(("PlaylistBase::DropRender: item: {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
    pdtrans->pditem->hwndItem, pdtrans->pditem->ulItemID, pdtrans.Item().Type().cdata(), pdtrans.Item().RMF().cdata(),
    pdtrans.Item().ContainerName().cdata(), pdtrans.Item().SourceName().cdata(), pdtrans.Item().TargetName().cdata(),
    pdtrans->pditem->cxOffset, pdtrans->pditem->cyOffset, pdtrans->pditem->fsControl, pdtrans->pditem->fsSupportedOps));

  if (!DrgVerifyStrXValue(pdtrans->hstrSelectedRMF, "<DRM_123LST,DRF_UNKNOWN>"))
    return FALSE;

  // Render temp file
  int_ptr<Playable> pp = Playable::GetByURL(pdtrans.RenderToName());
  if (pp == NULL)
    return FALSE;
  if (!pp->Clear())
    return FALSE;
  foreach(RecordBase**, rp, Source)
    if (!pp->InsertItem(*(*rp)->Data->Content, NULL))
      return FALSE;
  pdtrans->fsReply = DMFL_NATIVERENDER;
  PostMsg(UM_RENDERASYNC, MPFROMP(pdtrans.get()), MPFROMP(pp.toCptr()));
  return TRUE;
}

void PlaylistBase::DropRenderAsync(DragTransfer pdtrans, Playable* pp_)
{ int_ptr<Playable> pp;
  pp.fromCptr(pp_);
  bool ok = pp->Save(pp->URL, "plist123.dll", NULL, false);
  // Acknowledge the target.
  pdtrans.RenderComplete(ok ? DMFL_RENDEROK : DMFL_RENDERFAIL);
}

void PlaylistBase::DropEnd(vector<RecordBase>* source, bool ok)
{ DEBUGLOG(("PlaylistBase(%p)::DropEnd(%p, %i)\n", this, source, ok));
  if (!source)
    return;

  foreach (RecordBase**, rp, *source)
  { if (ok)
      // We do not lock the record here. Instead we do not /release/ it.
      PostMsg(UM_REMOVERECORD, MPFROMP(*rp), 0);
    else
      // Release the record locked in DragInit.
      FreeRecord(*rp);
  }
}

