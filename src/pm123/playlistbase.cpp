/*
 * Copyright 2007-2011 Marcel Mueeller
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


/* Code for the playlist manager */

#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DOS

#include <utilfct.h>
#include "playlistbase.h"
#include "playlistmanager.h"
#include "playlistview.h"
#include "infodialog.h"
#include "inspector.h"
#include "playable.h"
#include "waitinfo.h"
#include "loadhelper.h"
#include "pm123.h"
#include "gui.h"
#include "dialog.h"
#include "configuration.h"
#include "123_util.h"
#include "pm123.rc.h"
#include "docking.h"
#include "filedlg.h"

#include <cpp/smartptr.h>
#include <cpp/url123.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <stdarg.h>
#include <snprintf.h>
#include <os2.h>

#include <debuglog.h>


#define MAX_DRAG_IMAGES 6


#ifdef DEBUG_LOG
xstring PlaylistBase::RecordBase::DebugName() const
{ return xstring::sprintf("%p{%p{%s}}", this, Data->Content.get(), Data->Content->GetPlayable().URL.getShortName().cdata());
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
: ManagedDialog<DialogBase>(rid, NULLHANDLE),
  Content(&content),
  HwndContainer(NULLHANDLE),
  NoRefresh(false),
  Source(8),
  DecChanged(false),
  RootInfoDelegate(*this, &PlaylistBase::InfoChangeEvent, NULL),
  RootPlayStatusDelegate(*this, &PlaylistBase::PlayStatEvent),
  PluginDelegate(Plugin::ChangeEvent, *this, &PlaylistBase::PluginEvent)
{ DEBUGLOG(("PlaylistBase(%p)::PlaylistBase(&%p{%s}, %u)\n", this, &content, content.URL.cdata(), rid));
  static bool first = true;
  if (first)
  { InitIcons();
    first = false;
  }
  // These two ones are constant
  LoadWizards[0] = &amp_file_wizard;
  LoadWizards[1] = &amp_url_wizard;
}

PlaylistBase::~PlaylistBase()
{ DEBUGLOG(("PlaylistBase(%p)::~PlaylistBase()\n", this));
  WinDestroyAccelTable(AccelTable);
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
  Content->GetInfoChange() += RootInfoDelegate;
  Ctrl::GetChangeEvent() += RootPlayStatusDelegate;

  SetTitle();
  // initialize decoder dependent information once.
  PMRASSERT(WinPostMsg(GetHwnd(), UM_UPDATEDEC, 0, 0));
}

MRESULT PlaylistBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistBase(%p)::DlgProc(%x, %x, %x)\n", this, msg, mp1, mp2));
  switch (msg)
  {case WM_INITDLG:
    { InitDlg();
      // populate the root node
      RequestChildren(NULL);
      return 0;
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
      Destroy();
      return 0;
    }
    break;

   case WM_MENUEND:
    if (HWNDFROMMP(mp2) == HwndMenu)
    { //GetRecords(CRA_SOURCE); //Should normally be unchanged...
      DEBUGLOG(("PlaylistBase::DlgProc WM_MENUEND %u\n", Source.size()));
      SetEmphasis(CRA_SOURCE, false);
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
    switch (SHORT2FROMMP(mp1))
    {
     case CN_CONTEXTMENU:
      { RecordBase* rec = (RecordBase*)mp2;
        if (!GetSource(rec))
          break;
        HwndMenu = InitContextMenu();
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
        // Bei Song -> Navigate to
        if (pp.GetInfo().tech->attributes & TATTR_PLAYLIST)
          UserOpenDetailedView(pp.GetPlayable());
        else
          UserNavigate((RecordBase*)nre->pRecord);
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
        if (!DrgAccessDraginfo(pcdi->pDragInfo))
        { // Most likely an error of the source application. So do not blow out.
          DEBUGLOG(("PlaylistBase::DlgProc: CM_DRAG... - DrgAccessDraginfo failed\n"));
          return MRFROM2SHORT(DOR_NEVERDROP, 0);
        }
        DragAfter = SHORT2FROMMP(mp1) == CN_DRAGAFTER;
        MRESULT mr = DragOver(pcdi->pDragInfo, (RecordBase*)pcdi->pRecord);
        DrgFreeDraginfo(pcdi->pDragInfo);
        return mr;
      }
     case CN_DROP:
      { CNRDRAGINFO* pcdi = (CNRDRAGINFO*)PVOIDFROMMP(mp2);
        if (!DrgAccessDraginfo(pcdi->pDragInfo))
        { // Most likely an error of the source application. So do not blow out.
          DEBUGLOG(("PlaylistBase::DlgProc: CM_DRAG... - DrgAccessDraginfo failed\n"));
          return MRFROM2SHORT(DOR_NEVERDROP, 0);
        }
        DragDrop(pcdi->pDragInfo, (RecordBase*)pcdi->pRecord);
        DrgFreeDraginfo(pcdi->pDragInfo);
        return 0;
      }
     case CN_DROPHELP:
      GUI::ShowHelp(IDH_DRAG_AND_DROP);
      return 0;
    }
    break;

   case DM_RENDERCOMPLETE:
    DropRenderComplete((DRAGTRANSFER*)PVOIDFROMMP(mp1), SHORT1FROMMP(mp2));
    return 0;

   case DM_DISCARDOBJECT:
    { DRAGINFO* pdinfo = (DRAGINFO*)PVOIDFROMMP(mp1);
      bool r = false;
      if (DrgAccessDraginfo(pdinfo))
      { r = DropDiscard(pdinfo);
        DrgFreeDraginfo(pdinfo);
      }
      return MRFROMLONG(r ? DRR_SOURCE : DRR_ABORT);
    }
   case DM_RENDER:
    { DRAGTRANSFER* pdtrans = (DRAGTRANSFER*)PVOIDFROMMP(mp1);
      BOOL rc = DropRender(pdtrans);
      DrgFreeDragtransfer(pdtrans);
      return MRFROMLONG(rc);
    }
   case DM_ENDCONVERSATION:
    DropEnd((RecordBase*)PVOIDFROMMP(mp1), !!(LONGFROMMP(mp2) & DMFL_TARGETSUCCESSFUL));
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
          HwndMenu = InitContextMenu();
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
        { LoadHelper* lhp = new LoadHelper(Cfg::Get().playonload*LoadHelper::LoadPlay | LoadHelper::LoadRecall);
          lhp->AddItem(Content);
          GUI::Load(lhp);
          break;
        }
       case IDM_PL_USE:
        { LoadHelper* lhp = new LoadHelper(Cfg::Get().playonload*LoadHelper::LoadPlay | LoadHelper::LoadRecall);
          for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
            lhp->AddItem((*rpp)->Data->Content);
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
          UserOpenInfoView(set, AInfoDialog::Page_MetaInfo);
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
      DEBUGLOG(("PlaylistBase::DlgProc: UM_RECORDCOMMAND: %s, %i, %x\n",
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

HPOINTER PlaylistBase::CalcIcon(RecordBase* rec)
{ ASSERT(rec->Data->Content != NULL);
  Playable& pp = rec->Data->Content->GetPlayable();
  unsigned tattr = pp.GetInfo().tech->attributes;
  DEBUGLOG(("PlaylistBase::CalcIcon(%s) - %u\n", RecordBase::DebugName(rec).cdata(), tattr));
  IC state = IC_Pending;
  if ((pp.GetInfo().phys->attributes & PATTR_INVALID) || (tattr & TATTR_INVALID))
    state = IC_Invalid;
  else if (tattr & (TATTR_SONG|TATTR_PLAYLIST))
    state = IC_Normal;
  if (pp.IsInUse())
    state = GetRecordUsage(rec);
  if (tattr & TATTR_PLAYLIST)
    return IcoPlaylist[(tattr & TATTR_WRITABLE) != 0][state][GetPlaylistType(rec)];
  else
    return IcoSong[state];
}

void PlaylistBase::SetTitle()
{ DEBUGLOG(("PlaylistBase(%p)::SetTitle()\n", this));
  // Generate Window Title
  const char* append = "";
  unsigned tattr = Content->GetInfo().tech->attributes;
  if ((Content->GetInfo().phys->attributes & PATTR_INVALID) || (tattr & TATTR_INVALID))
    append = " [invalid]";
  else if (Content->IsInUse())
    append = " [used]";
  DialogBase::SetTitle(xstring::sprintf("PM123: %s%s%s", Content->GetDisplayName().cdata(),
    (tattr & TATTR_PLAYLIST) && Content->IsModified() ? " (*)" : "", append));
}


PlaylistBase::RecordBase* PlaylistBase::AddEntry(PlayableInstance& obj, RecordBase* parent, RecordBase* after)
{ DEBUGLOG(("PlaylistBase(%p{%s})::AddEntry(&%p{%s}, %p, %p)\n", this, DebugName().cdata(), &obj, obj.GetPlayable().URL.getShortName().cdata(), parent, after));
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

void PlaylistBase::RequestChildren(RecordBase* const rec)
{ DEBUGLOG(("PlaylistBase(%p)::RequestChildren(%s)\n", this, RecordBase::DebugName(rec).cdata()));
  APlayable& pp = APlayableFromRec(rec);
  // Call event either immediately or later, asynchronuously.
  PostRecordUpdate(rec, IF_Child & ~pp.RequestInfo(IF_Child, PRI_Normal));
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
      PostRecordUpdate(rp, IF_Usage); // update icon
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
          DEBUGLOG(("PlaylistBase::UpdateChildren - not found: %p{%s}\n", pi.get(), pi->GetPlayable().URL.getShortName().cdata()));
          crp = AddEntry(*pi, rp, crp);
          if (crp && (rp == NULL || (rp->flRecordAttr & CRA_EXPANDED)))
            RequestChildren(crp);
          break;
        }
        if ((*orpp)->Data->Content == pi)
        { // found!
          DEBUGLOG(("PlaylistBase::UpdateChildren - found: %p{%s} at %u\n", pi.get(), pi->GetPlayable().URL.getShortName().cdata(), orpp - old.begin()));
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
  { Playable& p = (*rpp)->Data->Content->GetPlayable();
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
    if ((pattr & PATTR_INVALID) || (tattr & TATTR_INVALID))
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
  ASSERT(rec != NULL);
  const Playable* pp;
  // Fetch current root
  { const int_ptr<APlayable>& ps = Ctrl::GetRoot();
    if (ps == NULL)
      return false;
    pp = &ps->GetPlayable();
  }
  // check stack
  do
  { rec = GetParent(rec);
    if (APlayableFromRec(rec).GetPlayable() == *pp)
      return true;
  } while (rec);
  return false;
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

/*void PlaylistBase::UpdateRecord(RecordBase* rec, InfoFlags iflags)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateRecord(%p, %x)\n", this, rec, iflags));
  if (rec == NULL)
  { // Root node => update title
    SetTitle();
  } else
  { // Record node
    if (iflags & IF_Alias)
    { xstring text = rec->Data->Content->GetDisplayName();
      rec->pszIcon = (PSZ)text.cdata();
      PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED)));
      rec->Data->Text = text; // now free the old text
    }
  }
}*/

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
  if (rec->Data->Content->GetPlayable().IsInUse())
    PostRecordUpdate(rec, IF_Usage);
}

void PlaylistBase::InfoChangeEvent(const PlayableChangeArgs& args, RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p{%s})::InfoChangeEvent({%p{%s},, %x, %x,}, %s)\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.GetPlayable().URL.getShortName().cdata(), args.Changed, args.Loaded, RecordBase::DebugName(rec).cdata()));
  PostRecordUpdate(rec, args.Changed);
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

void PlaylistBase::PluginEvent(const Plugin::EventArgs& args)
{ DEBUGLOG(("PlaylistBase(%p)::PluginEvent({&%p{%s}, %i})\n", this,
    &args.Plug, args.Plug.GetModule().Key.cdata(), args.Operation));
  if (args.Plug.GetType() == PLUGIN_DECODER)
  { switch (args.Operation)
    {case Plugin::EventArgs::Enable:
     case Plugin::EventArgs::Disable:
     case Plugin::EventArgs::Unload:
      WinPostMsg(GetHwnd(), UM_UPDATEDEC, 0, 0);
     default:; // avoid warnings
    }
  }
}

static void DLLENTRY UserAddCallback(void* param,
  const char* url, const INFO_BUNDLE* info, int cached, int override)
{ DEBUGLOG(("UserAddCallback(%p, %s,, %x, %x)\n", param, url));
  PlaylistBase::UserAddCallbackParams& ucp = *(PlaylistBase::UserAddCallbackParams*)param;

  int_ptr<Playable> ip = Playable::GetByURL(url123(url));
  ip->SetCachedInfo(*info, (InfoFlags)cached);
  
  PlayableRef* pr = ucp.Content.append() = new PlayableRef(*ip);
  pr->OverrideInfo(*info, (InfoFlags)override);
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
    pii, pii->Parent->URL.getShortName().cdata(),
    pii->Before.get(), pii->Before ? pii->Before->GetPlayable().URL.cdata() : "",
    pii->Item.get(), pii->Item->GetPlayable().URL.cdata()));
  pii->Parent->InsertItem(*pii->Item, pii->Before);
}

void PlaylistBase::UserRemove(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UserRemove(%s)\n", this, rec->DebugName().cdata()));
  // find parent playlist
  APlayable& p = APlayableFromRec(GetParent(rec));
  //DEBUGLOG(("PlaylistBase::UserRemove %s %p %p\n", RecordBase::DebugName(parent).cdata(), parent, playlist));
  if (p.GetInfo().tech->attributes & TATTR_PLAYLIST) // don't modify songs
    p.GetPlayable().RemoveItem(rec->Data->Content);
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
    par_list.RemoveItem(rec->Data->Content);
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
      if (si.NavigateTo(*rec->Data->Content))
        // Error sub-item is no longer part of the playlist.
        return;
      JobSet job(PRI_Normal);
      for (;;)
      { Location::NavigationResult rc = si.NavigateCount(1, TATTR_SONG, job, 1);
        if (rc)
        { if (rc.length() == 0)
            // TODO: dependency ???
            return;
          else
            break;
        }
        new_items.append() = &(PlayableRef&)si.GetCurrent();
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
    par_list.RemoveItem(rec->Data->Content);

  } // the update of the container is implicitly done by the notification mechanism
}

void PlaylistBase::UserSave(bool saveas)
{ DEBUGLOG(("PlaylistBase(%p)::UserSave(%u)\n", this, saveas));
  amp_save_playlist(GetHwnd(), *Content, saveas);
  // TODO change window root in case of success.
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
    p.RequestInfo(IF_Decoder, PRI_Normal, REL_Reload);
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
    GUI::ShowDialog(Source[0]->Data->Content->GetPlayable(), GUI::DLT_INFOEDIT);
   case 0:;
  }
}

void PlaylistBase::UserSort(Playable& p)
{ DEBUGLOG(("PlaylistBase(%p)::UserSort(&%p{%s})\n", this, &p, p.URL.cdata()));
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

/****************************************************************************
*
*  Drag and drop - Target side
*
****************************************************************************/
MRESULT PlaylistBase::DragOver(DRAGINFO* pdinfo, RecordBase* target)
{ DEBUGLOG(("PlaylistBase::DragOver(%p{,,%x, %p, %i,%i, %u,}, %s) - %u\n",
    pdinfo, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem,
    RecordBase::DebugName(target).cdata(), DragAfter));
  // Check source items
  USHORT drag    = DOR_DROP;
  USHORT drag_op = DO_DEFAULT;
  for (int i = 0; i < pdinfo->cditem; ++i)
  { DRAGITEM* pditem = DrgQueryDragitemPtr(pdinfo, i);
    DEBUGLOG(("PlaylistBase::DragOver: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, amp_string_from_drghstr(pditem->hstrType).cdata(), amp_string_from_drghstr(pditem->hstrRMF).cdata(),
      amp_string_from_drghstr(pditem->hstrContainerName).cdata(), amp_string_from_drghstr(pditem->hstrSourceName).cdata(), amp_string_from_drghstr(pditem->hstrTargetName).cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    // native PM123 object
    if (DrgVerifyRMF(pditem, "DRM_123FILE", NULL))
    { // Check for recursive operation
      if (!DragAfter && target && amp_string_from_drghstr(pditem->hstrSourceName) == target->Data->Content->GetPlayable().URL)
      { drag = DOR_NODROP;
        break;
      }
      if (pdinfo->usOperation == DO_DEFAULT)
        drag_op = pdinfo->hwndSource == GetHwnd() ? DO_MOVE : DO_COPY;
      else if (pdinfo->usOperation == DO_COPY || pdinfo->usOperation == DO_MOVE)
        drag_op = pdinfo->usOperation;
      else
        drag = DOR_NODROPOP;
      continue;
    }
    // File system object?
    else if (DrgVerifyRMF(pditem, "DRM_OS2FILE", NULL))
    { if ( (pdinfo->usOperation == DO_DEFAULT || pdinfo->usOperation == DO_LINK)
        && pditem->fsSupportedOps & DO_LINKABLE )
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

void PlaylistBase::DragDrop(DRAGINFO* pdinfo, RecordBase* target)
{ DEBUGLOG(("PlaylistBase::DragDrop(%p, %p)\n", pdinfo, target));
  DEBUGLOG(("PlaylistBase::DragDrop(%p{,,%x, %p, %i,%i, %u,}, %s) - %u\n",
    pdinfo, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem,
    RecordBase::DebugName(target).cdata(), DragAfter));

  // Locate parent
  int_ptr<APlayable> parent = DragAfter ? (APlayable*)Content.get() : &APlayableFromRec(target);
  // Locate insertion point
  if (!DragAfter)
    target = NULL;
  else if (target)
  { if (target == (RecordBase*)CMA_FIRST)
      target = NULL;
    // Search record after target, because the container returns the record /before/ the insertion point,
    // while PlayableCollection requires the entry /after/ the insertion point.
    target = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(target), MPFROM2SHORT(target ? CMA_NEXT : CMA_FIRST, CMA_ITEMORDER));
    PMASSERT(target != (RecordBase*)-1);
  }

  // For each dropped item...
  for (int i = 0; i < pdinfo->cditem; ++i)
  { DRAGITEM* pditem = DrgQueryDragitemPtr(pdinfo, i);
    DEBUGLOG(("PlaylistBase::DragDrop: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, amp_string_from_drghstr(pditem->hstrType).cdata(), amp_string_from_drghstr(pditem->hstrRMF).cdata(),
      amp_string_from_drghstr(pditem->hstrContainerName).cdata(), amp_string_from_drghstr(pditem->hstrSourceName).cdata(), amp_string_from_drghstr(pditem->hstrTargetName).cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    ULONG reply = DMFL_TARGETFAIL;

    // fetch target name
    xstring alias = amp_string_from_drghstr(pditem->hstrTargetName);
    // Do not set an empty alias
    if (alias.length() == 0)
      alias = NULL;

    if (DrgVerifyRMF(pditem, "DRM_OS2FILE", NULL))
    {
      // fetch full qualified path
      size_t lenP = DrgQueryStrNameLen(pditem->hstrContainerName);
      size_t lenN = DrgQueryStrNameLen(pditem->hstrSourceName);
      xstring fullname;
      char* cp = fullname.allocate(lenP + lenN);
      DrgQueryStrName(pditem->hstrContainerName, lenP+1, cp);
      DrgQueryStrName(pditem->hstrSourceName,    lenN+1, cp+lenP);
      // do not set identical alias
      if (alias.length() == 0 || alias == fullname.cdata()+lenP)
        alias = NULL;

      if (pditem->hwndItem && DrgVerifyType(pditem, "UniformResourceLocator"))
      { // URL => The droped item must be rendered.
        DRAGTRANSFER* pdtrans = DrgAllocDragtransfer(1);
        if (pdtrans)
        { // Prevent the target from beeing disposed until the render has completed.
          // However, the underlying PlayableInstance may still be removed, but this is checked.
          BlockRecord(target);
          DropInfo* pdsource = new DropInfo();
          pdsource->Hwnd   = pditem->hwndItem;
          pdsource->ItemID = pditem->ulItemID;
          pdsource->Parent = &parent->GetPlayable();
          pdsource->Alias  = alias;
          pdsource->Before = target;

          pdtrans->cb               = sizeof( DRAGTRANSFER );
          pdtrans->hwndClient       = GetHwnd();
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
        DEBUGLOG(("PlaylistBase::DragDrop: DRM_OS2FILE && !UniformResourceLocator - %x\n", pditem->fsControl));
        // Hopefully this criterion is sufficient to identify folders.
        if (pditem->fsControl & DC_CONTAINER)
        { // TODO: should be alterable
          if (Cfg::Get().recurse_dnd)
            fullname = fullname + "/?Recursive";
          else
            fullname = fullname + "/";
        }

        // prepare insert item
        InsertInfo* pii = new InsertInfo();
        pii->Parent = &parent->GetPlayable();
        pii->Item   = new PlayableRef(*Playable::GetByURL(url123::normalizeURL(fullname)));
        // TODO: //pii->Alias  = alias;
        if (target)
          pii->Before = target->Data->Content;
        WinPostMsg(GetHwnd(), UM_INSERTITEM, MPFROMP(pii), 0);
        reply = DMFL_TARGETSUCCESSFUL;
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
        pdtrans->hwndClient       = GetHwnd();
        pdtrans->pditem           = pditem;
        pdtrans->hstrSelectedRMF  = DrgAddStrHandle("<DRM_123FILE,DRF_UNKNOWN>");
        pdtrans->hstrRenderToName = 0;
        pdtrans->fsReply          = 0;
        pdtrans->usOperation      = pdinfo->usOperation;

        // Ask the source to render the selected item.
        DrgSendTransferMsg(pditem->hwndItem, DM_RENDER, (MPARAM)pdtrans, 0);

        // insert item
        if (pdtrans->fsReply & DMFL_NATIVERENDER)
        { InsertInfo* pii = new InsertInfo();
          pii->Parent = &parent->GetPlayable();
          pii->Item   = new PlayableRef(*Playable::GetByURL(amp_string_from_drghstr(pditem->hstrSourceName)));
          // TODO:
          //pii->Alias  = alias;
          // TODO: The slice information is currently lost during D'n'd
          if (target)
            pii->Before = target->Data->Content;
          WinPostMsg(GetHwnd(), UM_INSERTITEM, MPFROMP(pii), 0);
          reply = DMFL_TARGETSUCCESSFUL;
        }
        // cleanup
        DrgFreeDragtransfer(pdtrans);
      }
    }
    // Tell the source you're done.
    DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION, MPFROMLONG(pditem->ulItemID), MPFROMLONG(reply));
  }
  DrgDeleteDraginfoStrHandles(pdinfo);
}

void PlaylistBase::DropRenderComplete(DRAGTRANSFER* pdtrans, USHORT flags)
{ DEBUGLOG(("PlaylistBase::DropRenderComplete(%p{, %x, %p{...}, %s, %s, %p, %i, %x}, %x)\n",
    pdtrans, pdtrans->hwndClient, pdtrans->pditem, amp_string_from_drghstr(pdtrans->hstrSelectedRMF).cdata(), amp_string_from_drghstr(pdtrans->hstrRenderToName).cdata(),
    pdtrans->ulTargetInfo, pdtrans->usOperation, pdtrans->fsReply, flags));

  ULONG reply = DMFL_TARGETFAIL;
  DropInfo* pdsource = (DropInfo*)pdtrans->ulTargetInfo;
  // If the rendering was successful, use the file, then delete it.
  if ((flags & DMFL_RENDEROK) && pdsource)
  {
    // fetch render to name
    const xstring& rendered = amp_string_from_drghstr(pdtrans->hstrRenderToName);
    // fetch file content
    const xstring& fullname = amp_url_from_file(rendered);
    DosDelete(rendered);

    // insert item(s)
    // In theory the items are not inserted in the right order if the DM_RENDERCOMPLETE messages
    // arrive not in the same order as the dragitems in the DRAGINFO structure.
    // Since this is very unlikely, we ignore that here.
    // Since DM_RENDERCOMPLETE should be posted we do not need to post UM_INSERTITEM
    PlayableRef pr(*Playable::GetByURL(url123::normalizeURL(fullname)));
    // TODO:
    // pr.SetAlias(pdsource->Alias);
    pdsource->Parent->InsertItem(pr, pdsource->Before ? pdsource->Before->Data->Content.get() : NULL);
    reply = DMFL_TARGETSUCCESSFUL;
  }

  // Tell the source you're done.
  DrgSendTransferMsg(pdsource->Hwnd, DM_ENDCONVERSATION, MPFROMLONG(pdsource->ItemID), MPFROMLONG(reply));
  delete pdsource;

  DrgDeleteStrHandle (pdtrans->hstrSelectedRMF);
  DrgDeleteStrHandle (pdtrans->hstrRenderToName);
  DrgFreeDragtransfer(pdtrans);
}

/****************************************************************************
*
*  Drag and drop - Source side
*
****************************************************************************/
void PlaylistBase::DragInit()
{ DEBUGLOG(("PlaylistBase::DragInit() - %u\n", Source.size()));

  // If the Source Array is empty, we must be over whitespace,
  // in which case we don't want to drag any records.
  if (Source.size() == 0)
    return;

  SetEmphasis(CRA_SOURCE, true);

  // Let PM allocate enough memory for a DRAGINFO structure as well as
  // a DRAGITEM structure for each record being dragged. It will allocate
  // shared memory so other processes can participate in the drag/drop.
  DRAGINFO* drag_infos = drag_infos = DrgAllocDraginfo(Source.size());
  PMASSERT(drag_infos);

  // Allocate an array of DRAGIMAGE structures. Each structure contains
  // info about an image that will be under the mouse pointer during the
  // drag. This image will represent a container record being dragged.
  DRAGIMAGE* drag_images = new DRAGIMAGE[min(Source.size(), MAX_DRAG_IMAGES)];

  for (size_t i = 0; i < Source.size(); ++i)
  { RecordBase* rec = Source[i];
    DEBUGLOG(("PlaylistBase::DragInit: init item %i: %s\n", i, rec->DebugName().cdata()));

    // Prevent the records from beeing disposed.
    // The records are normally freed by DropEnd, except in case DrgDrag returns with an error.
    BlockRecord(rec);

    DRAGITEM* pditem = DrgQueryDragitemPtr(drag_infos, i);
    pditem->hwndItem          = GetHwnd();
    pditem->ulItemID          = (ULONG)rec;
    pditem->hstrType          = DrgAddStrHandle(DRT_BINDATA);
    pditem->hstrRMF           = DrgAddStrHandle("(DRM_123FILE,DRM_DISCARD)x(DRF_UNKNOWN)");
    pditem->hstrContainerName = DrgAddStrHandle(Content->URL);
    pditem->hstrSourceName    = DrgAddStrHandle(rec->Data->Content->GetPlayable().URL);
    // TODO:
    /*if (rec->Data->Content->GetAlias())
      pditem->hstrTargetName  = DrgAddStrHandle(rec->Data->Content->GetAlias());*/
    pditem->fsSupportedOps    = DO_MOVEABLE | DO_COPYABLE;
    DEBUGLOG(("PlaylistBase::DragInit: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, amp_string_from_drghstr(pditem->hstrType).cdata(), amp_string_from_drghstr(pditem->hstrRMF).cdata(),
      amp_string_from_drghstr(pditem->hstrContainerName).cdata(), amp_string_from_drghstr(pditem->hstrSourceName).cdata(), amp_string_from_drghstr(pditem->hstrTargetName).cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    if (i >= MAX_DRAG_IMAGES)
      continue;
    DRAGIMAGE* drag_image = drag_images + i;
    drag_image->cb       = sizeof(DRAGIMAGE);
    drag_image->hImage   = rec->hptrIcon;
    drag_image->fl       = DRG_ICON|DRG_MINIBITMAP;
    drag_image->cxOffset = 5 * i;
    drag_image->cyOffset = -5 * i;
  }

  // If DrgDrag returns NULLHANDLE, that means the user hit Esc or F1
  // while the drag was going on so the target didn't have a chance to
  // delete the string handles. So it is up to the source window to do
  // it. Unfortunately there doesn't seem to be a way to determine
  // whether the NULLHANDLE means Esc was pressed as opposed to there
  // being an error in the drag operation. So we don't attempt to figure
  // that out. To us, a NULLHANDLE means Esc was pressed...
  if (!DrgDrag(GetHwnd(), drag_infos, drag_images, min(Source.size(), MAX_DRAG_IMAGES), VK_ENDDRAG, NULL))
  { DEBUGLOG(("PlaylistBase::DragInit: DrgDrag returned FALSE - %x\n", WinGetLastError(NULL)));
    DrgDeleteDraginfoStrHandles(drag_infos);
    // release the records
    for (RecordBase** prec = Source.begin(); prec != Source.end(); ++prec)
      FreeRecord(*prec);
  }

  DrgFreeDraginfo(drag_infos);
  delete[] drag_images;

  SetEmphasis(CRA_SOURCE, false);
}

bool PlaylistBase::DropDiscard(DRAGINFO* pdinfo)
{ DEBUGLOG(("PlaylistBase::DropDiscard(%p{,,%x, %p, %i,%i, %u,})\n",
    pdinfo, pdinfo->usOperation, pdinfo->hwndSource, pdinfo->xDrop, pdinfo->yDrop, pdinfo->cditem));

  for (int i = 0; i < pdinfo->cditem; ++i)
  { DRAGITEM* pditem = DrgQueryDragitemPtr(pdinfo, i);
    DEBUGLOG(("PlaylistBase::DropDiscard: item {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
      pditem->hwndItem, pditem->ulItemID, amp_string_from_drghstr(pditem->hstrType).cdata(), amp_string_from_drghstr(pditem->hstrRMF).cdata(),
      amp_string_from_drghstr(pditem->hstrContainerName).cdata(), amp_string_from_drghstr(pditem->hstrSourceName).cdata(), amp_string_from_drghstr(pditem->hstrTargetName).cdata(),
      pditem->cxOffset, pditem->cyOffset, pditem->fsControl, pditem->fsSupportedOps));

    // get record
    RecordBase* rec = (RecordBase*)pditem->ulItemID;
    // Remove object later
    BlockRecord(rec);
    WinPostMsg(GetHwnd(), UM_REMOVERECORD, MPFROMP(rec), 0);
  }
  return true;
}

BOOL PlaylistBase::DropRender(DRAGTRANSFER* pdtrans)
{ DEBUGLOG(("PlaylistBase::DropRender(%p{, %x, %p{...}, %s, %s, %p, %i, %x})\n",
    pdtrans, pdtrans->hwndClient, pdtrans->pditem, amp_string_from_drghstr(pdtrans->hstrSelectedRMF).cdata(), amp_string_from_drghstr(pdtrans->hstrRenderToName).cdata(),
    pdtrans->ulTargetInfo, pdtrans->usOperation, pdtrans->fsReply));
  DEBUGLOG(("PlaylistBase::DropRender: item: {%p, %p, %s, %s, %s, %s, %s, %i,%i, %x, %x}\n",
    pdtrans->pditem->hwndItem, pdtrans->pditem->ulItemID, amp_string_from_drghstr(pdtrans->pditem->hstrType).cdata(), amp_string_from_drghstr(pdtrans->pditem->hstrRMF).cdata(),
    amp_string_from_drghstr(pdtrans->pditem->hstrContainerName).cdata(), amp_string_from_drghstr(pdtrans->pditem->hstrSourceName).cdata(), amp_string_from_drghstr(pdtrans->pditem->hstrTargetName).cdata(),
    pdtrans->pditem->cxOffset, pdtrans->pditem->cyOffset, pdtrans->pditem->fsControl, pdtrans->pditem->fsSupportedOps));

  // Remove record reference from the DRAGITEM structure unless opeartion is move.
  // This turns the DropEnd into a NOP.
  if (pdtrans->usOperation != DO_MOVE)
  { RecordBase* rec = (RecordBase*)pdtrans->pditem->ulItemID;
    pdtrans->pditem->ulItemID = 0;
    FreeRecord(rec);
  }
  pdtrans->fsReply = DMFL_NATIVERENDER;
  return FALSE;
}

void PlaylistBase::DropEnd(RecordBase* rec, bool ok)
{ DEBUGLOG(("PlaylistBase::DropEnd(%s, %i)\n", RecordBase::DebugName(rec).cdata(), ok));
  if (!rec)
    return;
  if (ok)
    // We do not lock the record here. Instead we do not /release/ it.
    WinPostMsg(GetHwnd(), UM_REMOVERECORD, MPFROMP(rec), 0);
  else
    // Release the record locked in DragInit.
    FreeRecord(rec);
}

