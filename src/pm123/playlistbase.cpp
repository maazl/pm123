/*
 * Copyright 2007-2008 Marcel MÅeller
 *           1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli LeppÑ <rosmo@sektori.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include <os2.h>
#include <utilfct.h>
#include "playlistbase.h"
#include "playlistmanager.h"
#include "playlistview.h"
#include "playable.h"
#include "pm123.h"
#include "123_util.h"
#include "pm123.rc.h"
#include "docking.h"
#include "iniman.h"
#include "plugman.h"

#include <stdarg.h>
#include <snprintf.h>
#include <cpp/smartptr.h>
#include <cpp/url123.h>

#include <debuglog.h>


#define MAX_DRAG_IMAGES 6


#ifdef DEBUG
xstring PlaylistBase::RecordBase::DebugName() const
{ return xstring::sprintf("%p{%p{%s}}", this, &*Data->Content, Data->Content->GetPlayable()->GetURL().getShortName().cdata());
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
{ return Content->GetURL().getShortName();
}
#endif


/****************************************************************************
*
*  class PlaylistBase
*
****************************************************************************/

HPOINTER PlaylistBase::IcoWait;
HPOINTER PlaylistBase::IcoInvalid;
HPOINTER PlaylistBase::IcoPlayable[5][4];

void PlaylistBase::InitIcons()
{ IcoWait                               = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3WAIT);
  IcoInvalid                            = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3INVLD);
  IcoPlayable[ICP_Song     ][IC_Normal] = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3);
  IcoPlayable[ICP_Song     ][IC_Used  ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3USED);
  IcoPlayable[ICP_Song     ][IC_Active] = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3ACTIVE);
  IcoPlayable[ICP_Song     ][IC_Play  ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3PLAY);
  IcoPlayable[ICP_Empty    ][IC_Normal] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLEMPTY);
  IcoPlayable[ICP_Empty    ][IC_Used  ] = IcoPlayable[ICP_Empty    ][IC_Normal];
  IcoPlayable[ICP_Empty    ][IC_Active] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLACTIVE);
  IcoPlayable[ICP_Empty    ][IC_Play  ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLPLAY);
  IcoPlayable[ICP_Closed   ][IC_Normal] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLCLOSE);
  IcoPlayable[ICP_Closed   ][IC_Used  ] = IcoPlayable[ICP_Closed   ][IC_Normal];
  IcoPlayable[ICP_Closed   ][IC_Active] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLCLOSEACTIVE);
  IcoPlayable[ICP_Closed   ][IC_Play  ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLCLOSEPLAY);
  IcoPlayable[ICP_Open     ][IC_Normal] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLOPEN);
  IcoPlayable[ICP_Open     ][IC_Used  ] = IcoPlayable[ICP_Open     ][IC_Normal];
  IcoPlayable[ICP_Open     ][IC_Active] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLOPENACTIVE);
  IcoPlayable[ICP_Open     ][IC_Play  ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLOPENPLAY);
  IcoPlayable[ICP_Recursive][IC_Normal] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLRECURSIVE);
  IcoPlayable[ICP_Recursive][IC_Used  ] = IcoPlayable[ICP_Recursive][IC_Normal];
  IcoPlayable[ICP_Recursive][IC_Active] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLRECURSIVEACTIVE);
  IcoPlayable[ICP_Recursive][IC_Play  ] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLRECURSIVEPLAY);
}

PlaylistBase::PlaylistBase(const char* url, const xstring& alias, ULONG rid)
: Content(Playable::GetByURL(url)),
  Alias(alias),
  NameApp(""),
  DlgRID(rid),
  HwndFrame(NULLHANDLE),
  HwndContainer(NULLHANDLE),
  NoRefresh(false),
  Source(8),
  DecChanged(false),
  RootInfoDelegate(*this, &PlaylistBase::InfoChangeEvent, NULL),
  RootPlayStatusDelegate(*this, &PlaylistBase::PlayStatEvent),
  PluginDelegate(plugin_event, *this, &PlaylistBase::PluginEvent),
  InitialVisible(false),
  Initialized(0)
{ DEBUGLOG(("PlaylistBase(%p)::PlaylistBase(%s, %s, %u)\n", this, url, alias ? alias.cdata() : "<NULL>", rid));
  static bool first = true;
  if (first)
  { InitIcons();
    first = false;
  }
  // These two ones are constant
  LoadWizzards[0] = &amp_file_wizzard;
  LoadWizzards[1] = &amp_url_wizzard;
}

PlaylistBase::~PlaylistBase()
{ DEBUGLOG(("PlaylistBase(%p)::~PlaylistBase()\n", this));
  // The window may be closed already => ignore the error
  // This may give an error if called from our own thread. This is intensionally ignored here.
  WinDestroyWindow(HwndFrame);
  WinDestroyAccelTable(AccelTable);
}

void PlaylistBase::StartDialog()
{ // initialize dialog
  init_dlg_struct ids = { sizeof(init_dlg_struct), this };
  PMRASSERT(WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, pl_DlgProcStub, NULLHANDLE, DlgRID, &ids));
}

void PlaylistBase::PostRecordCommand(RecordBase* rec, RecordCommand cmd)
{ DEBUGLOG(("PlaylistBase(%p)::PostRecordCommand(%p, %u) - %x\n", this, rec, cmd, StateFromRec(rec).PostMsg));
  Interlocked il(StateFromRec(rec).PostMsg);
  // Check whether the requested bit is already set or there are other events pending.
  if (il.bitset(cmd) || il != (1U<<cmd))
  { DEBUGLOG(("PlaylistBase::PostRecordCommand - nope! %x\n", (unsigned)il));
    return; // requested command is already on the way or another unexecuted message is outstanding
  }
  // There is a little chance that we generate two messages for the same record.
  // The second one will be a no-op in the window procedure.
  BlockRecord(rec);
  PMRASSERT(WinPostMsg(HwndFrame, UM_RECORDCOMMAND, MPFROMP(rec), MPFROMSHORT(TRUE)));
}

void PlaylistBase::FreeRecord(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::FreeRecord(%p)\n", this, rec));
  if (rec && InterlockedDec(rec->UseCount) == 0)
    // we can safely post this message because the record is now no longer used anyway.
    PMRASSERT(WinPostMsg(HwndFrame, UM_DELETERECORD, MPFROMP(rec), 0));
}

void PlaylistBase::DeleteEntry(RecordBase* entry)
{ DEBUGLOG(("PlaylistBase(%p{%s})::DeleteEntry(%s)\n", this, DebugName().cdata(), RecordBase::DebugName(entry).cdata()));
  delete entry->Data;
  PMRASSERT(WinSendMsg(HwndContainer, CM_FREERECORD, MPFROMP(&entry), MPFROMSHORT(1)));
  DEBUGLOG(("PlaylistBase::DeleteEntry completed\n"));
}


MRESULT EXPENTRY pl_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistBase::DlgProcStub(%x, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  PlaylistBase* pb;
  if (msg == WM_INITDLG)
  { // Attach the class instance to the window.
    PlaylistBase::init_dlg_struct* ip = (PlaylistBase::init_dlg_struct*)mp2;
    DEBUGLOG(("PlaylistBase::DlgProcStub: WM_INITDLG - %p{%i, %p}}\n", ip, ip->size, ip->pm));
    PMRASSERT(WinSetWindowPtr(hwnd, QWL_USER, ip->pm));
    pb = (PlaylistBase*)ip->pm;
    pb->HwndFrame = hwnd; // Store the hwnd early, since LoadDlg will return too late.
  } else
  { // Lookup class instance
    pb = (PlaylistBase*)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pb == NULL)
      // No class attached. We only can do default processing so far.
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  // Now call the class method
  return pb->DlgProc(msg, mp1, mp2);
}

void PlaylistBase::InitDlg()
{ HPOINTER hicon = WinLoadPointer(HWND_DESKTOP, 0, ICO_MAIN);
  PMASSERT(hicon != NULLHANDLE);
  PMRASSERT(WinSendMsg(HwndFrame, WM_SETICON, (MPARAM)hicon, 0));
  do_warpsans(HwndFrame);
  { LONG     color;
    color = CLR_GREEN;
    PMRASSERT(WinSetPresParam(HwndContainer, PP_FOREGROUNDCOLORINDEX, sizeof(color), &color));
    color = CLR_BLACK;
    PMRASSERT(WinSetPresParam(HwndContainer, PP_BACKGROUNDCOLORINDEX, sizeof(color), &color));
  }

  rest_window_pos(HwndFrame, 0);

  // TODO: do not open all playlistmanager windows at the same location
  dk_add_window(HwndFrame, 0);

  // register events
  Content->InfoChange += RootInfoDelegate;
  Ctrl::ChangeEvent += RootPlayStatusDelegate;

  SetTitle();
  // initialize decoder dependant information once.
  PMRASSERT(WinPostMsg(HwndFrame, UM_UPDATEDEC, 0, 0));
    
  // Visibility
  Initialized = 1;
  SetVisible(InitialVisible);
  Initialized = 2;
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
      // process outstanding UM_DELETERECORD messages before we quit to ensure that all records are back to the PM before we die.
      QMSG qmsg;
      while (WinPeekMsg(amp_player_hab(), &qmsg, HwndFrame, UM_DELETERECORD, UM_DELETERECORD, PM_REMOVE))
      { DEBUGLOG2(("PlaylistBase::DlgProc: WM_DESTROY: %x %x %x %x\n", qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2));
        DlgProc(qmsg.msg, qmsg.mp1, qmsg.mp2); // We take the short way here.
      }
      break;
    }

   case WM_SYSCOMMAND:
    if( SHORT1FROMMP(mp1) == SC_CLOSE )
    { SetVisible(false);
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
      dk_set_state( HwndFrame, pswp[0].fl & SWP_SHOW ? 0 : DK_IS_GHOST );
      HSWITCH hswitch = WinQuerySwitchHandle( HwndFrame, 0 );
      if (hswitch == NULLHANDLE)
        break; // For some reasons the switchlist entry may be destroyed before.
      SWCNTRL swcntrl;
      PMXASSERT(WinQuerySwitchEntry(hswitch, &swcntrl), == 0);
      swcntrl.uchVisibility = pswp[0].fl & SWP_SHOW ? SWL_VISIBLE : SWL_INVISIBLE;
      PMXASSERT(WinChangeSwitchEntry( hswitch, &swcntrl ), == 0);
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
          // TODO: Mouse Position may not be reasonable, when the menu is invoked by keyboard.

          PMRASSERT(WinPopupMenu(HWND_DESKTOP, HwndFrame, HwndMenu, ptlMouse.x, ptlMouse.y, 0,
                                 PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD));
        }
        break;
      }

     // Direct editing
     case CN_BEGINEDIT:
      DEBUGLOG(("PlaylistBase::DlgProc CN_BEGINEDIT\n"));
      PMRASSERT(WinSetAccelTable(WinQueryAnchorBlock(HwndFrame), NULLHANDLE, HwndFrame));
      // TODO: normally we have to lock some functions here...
      return 0;

     case CN_REALLOCPSZ:
      { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
        DEBUGLOG(("PlaylistBase::DlgProc CN_REALLOCPSZ %p{,%p->%p{%s},%u,}\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText));
        *ed->ppszText = DirectEdit.raw_init(ed->cbText-1); // because of the string sharing we always have to initialize the instance
      }
      return MRFROMLONG(TRUE);

     case CN_ENDEDIT:
      DirectEdit = NULL; // Free string
      PMRASSERT(WinSetAccelTable(WinQueryAnchorBlock(HwndFrame), AccelTable, HwndFrame));
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
      
      if (SHORT1FROMMP(mp1) >= IDM_PL_APPFILEALL && SHORT1FROMMP(mp1) < IDM_PL_APPFILEALL + sizeof LoadWizzards / sizeof *LoadWizzards)
      { DECODER_WIZZARD_FUNC func = LoadWizzards[SHORT1FROMMP(mp1)-IDM_PL_APPFILEALL];
        if (func != NULL)
          UserAdd(func, NULL);
        return 0;
      }
      if (SHORT1FROMMP(mp1) >= IDM_PL_APPFILE && SHORT1FROMMP(mp1) < IDM_PL_APPFILE + sizeof LoadWizzards / sizeof *LoadWizzards)
      { DECODER_WIZZARD_FUNC func = LoadWizzards[SHORT1FROMMP(mp1)-IDM_PL_APPFILE];
        if (func != NULL && Source.size() == 1)
          UserAdd(func, Source[0]);
        return 0;
      }
      switch (SHORT1FROMMP(mp1))
      {case IDM_PL_USEALL:
        { amp_load_playable(Content->GetURL(), 0, 0);
          return 0;
        }
       case IDM_PL_USE:
        { for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
            amp_load_playable((*rpp)->Data->Content->GetPlayable()->GetURL(), (*rpp)->Data->Content->GetSlice().Start, Source.size() > 1 ? AMP_LOAD_APPEND : 0);
          return 0;
        }
       case IDM_PL_TREEVIEWALL:
        UserOpenTreeView(Content);
        return 0;

       case IDM_PL_TREEVIEW:
        Apply2Source(&PlaylistBase::UserOpenTreeView);
        return 0;

       case IDM_PL_DETAILEDALL:
        UserOpenDetailedView(Content);
        return 0;

       case IDM_PL_DETAILED:
        Apply2Source(&PlaylistBase::UserOpenDetailedView);
        return 0;

       case IDM_PL_REFRESH:
        Apply2Source(&PlaylistBase::UserReload);
        return 0;

       case IDM_PL_EDIT:
        Apply2Source(&PlaylistBase::UserEditMeta);
        return 0;

       case IDM_PL_REMOVE:
        { for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
            UserRemove(*rpp);
          return 0;
        }

       case IDM_PL_SORT_URLALL:
        SortComparer = &PlaylistBase::CompURL;
        UserSort(Content);
        return 0;
       case IDM_PL_SORT_URL: 
        SortComparer = &PlaylistBase::CompURL;
        Apply2Source(&PlaylistBase::UserSort);
        return 0;
       case IDM_PL_SORT_SONGALL:
        SortComparer = &PlaylistBase::CompTitle;
        UserSort(Content);
        return 0;
       case IDM_PL_SORT_SONG: 
        SortComparer = &PlaylistBase::CompTitle;
        Apply2Source(&PlaylistBase::UserSort);
        return 0;
       case IDM_PL_SORT_ARTALL:
        SortComparer = &PlaylistBase::CompArtist;
        UserSort(Content);
        return 0;
       case IDM_PL_SORT_ART: 
        SortComparer = &PlaylistBase::CompArtist;
        Apply2Source(&PlaylistBase::UserSort);
        return 0;
       case IDM_PL_SORT_ALBUMALL:
        SortComparer = &PlaylistBase::CompAlbum;
        UserSort(Content);
        return 0;
       case IDM_PL_SORT_ALBUM: 
        SortComparer = &PlaylistBase::CompAlbum;
        Apply2Source(&PlaylistBase::UserSort);
        return 0;
       case IDM_PL_SORT_ALIASALL:
        SortComparer = &PlaylistBase::CompAlias;
        UserSort(Content);
        return 0;
       case IDM_PL_SORT_ALIAS: 
        SortComparer = &PlaylistBase::CompAlias;
        Apply2Source(&PlaylistBase::UserSort);
        return 0;
       case IDM_PL_SORT_TIMEALL:
        SortComparer = &PlaylistBase::CompTime;
        UserSort(Content);
        return 0;
       case IDM_PL_SORT_TIME: 
        SortComparer = &PlaylistBase::CompTime;
        Apply2Source(&PlaylistBase::UserSort);
        return 0;
       case IDM_PL_SORT_SIZEALL:
        SortComparer = &PlaylistBase::CompSize;
        UserSort(Content);
        return 0;
       case IDM_PL_SORT_SIZE: 
        SortComparer = &PlaylistBase::CompSize;
        Apply2Source(&PlaylistBase::UserSort);
        return 0;

       case IDM_PL_SORT_RANDALL:
        UserShuffle(Content);
        return 0;
       case IDM_PL_SORT_RAND: 
        Apply2Source(&PlaylistBase::UserShuffle);
        return 0;

       case IDM_PL_CLEARALL:
        UserClearPlaylist(Content);
        return 0;

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
        UserReload(Content);
        return 0;

       case IDM_PL_OPEN:
        { url123 URL = PlaylistSelect(HwndFrame, "Open Playlist");
          if (URL)
          { PlaylistBase* pp = GetSame(URL);
            pp->SetVisible(true);
          }
          return 0;
        }
       case IDM_PL_SAVE:
        UserSave();
        return 0;

      } // switch (cmd)
      break;
    }

   case UM_RECORDCOMMAND:
    { RecordBase* rec = (RecordBase*)PVOIDFROMMP(mp1);
      // Do nothing but free the record in case this is requested.
      DEBUGLOG(("PlaylistBase::DlgProc: UM_RECORDCOMMAND: %s - %i\n", RecordBase::DebugName(rec).cdata(), SHORT1FROMMP(mp2)));
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
      HAB hab = WinQueryAnchorBlock(HwndFrame);   
      HACCEL haccel = WinQueryAccelTable(hab, HwndFrame);
      PMRASSERT(WinSetAccelTable(hab, AccelTable, HwndFrame));
      if (haccel != NULLHANDLE)
        PMRASSERT(WinDestroyAccelTable(haccel));
      // done
      // Notify context menu handlers
      DecChanged = true;
      return 0;
    }
  }
  return WinDefDlgProc( HwndFrame, msg, mp1, mp2 );
}

int PlaylistBase::compareTo(const char*const& str) const
{ return stricmp(Content->GetURL(), str);
}

void PlaylistBase::SetVisible(bool show)
{ DEBUGLOG(("PlaylistBase(%p{%s})::SetVisible(%u)\n", this, DebugName().cdata(), show));

  if (Initialized == 0) // double check
  { CritSect cs;
    if (Initialized == 0)
    { InitialVisible = show;
      return;
  } }
  
  PMRASSERT(WinSetWindowPos( HwndFrame, HWND_TOP, 0, 0, 0, 0, show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE ));
}

bool PlaylistBase::GetVisible() const
{ if (Initialized < 2)
  { CritSect cs;
    if (Initialized < 2)
      return InitialVisible;
  }
  return WinIsWindowVisible(HwndFrame);
}

HPOINTER PlaylistBase::CalcIcon(RecordBase* rec)
{ ASSERT(rec->Data->Content != NULL);
  DEBUGLOG(("PlaylistBase::CalcIcon(%s) - %u\n", RecordBase::DebugName(rec).cdata(), rec->Data->Content->GetPlayable()->GetStatus()));
  switch (rec->Data->Content->GetPlayable()->GetStatus())
  {case STA_Unknown:
    return IcoWait;
   case STA_Invalid:
    return IcoInvalid;
   default:
    return IcoPlayable[GetPlayableType(rec)][GetRecordUsage(rec)];
  }
}

void PlaylistBase::SetTitle()
{ DEBUGLOG(("PlaylistBase(%p)::SetTitle()\n", this));
  // Generate Window Title
  const char* append = "";
  switch (Content->GetStatus())
  {case STA_Invalid:
    append = " [invalid]";
    break;
   case STA_Used:
    append = " [used]";
    break;
   default:;
  }
  xstring title = xstring::sprintf("PM123: %s%s%s%s", GetDisplayName().cdata(), NameApp,
    (Content->GetFlags() & Playable::Enumerable) && ((PlayableCollection&)*Content).IsModified() ? " (*)" : "", append);
  // Update Window Title
  PMRASSERT(WinSetWindowText(HwndFrame, (char*)title.cdata()));
  // now free the old title
  Title = title;
}


PlaylistBase::RecordBase* PlaylistBase::AddEntry(PlayableInstance* obj, RecordBase* parent, RecordBase* after)
{ DEBUGLOG(("PlaylistBase(%p{%s})::AddEntry(%p{%s}, %p, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable()->GetURL().getShortName().cdata(), parent, after));
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
  entry->Data->DeregisterEvents();
  // Delete the children
  RemoveChildren(entry);
  // Remove record from container
  PMXASSERT(LONGFROMMR(WinSendMsg(HwndContainer, CM_REMOVERECORD, MPFROMP(&entry), MPFROM2SHORT(1, CMA_INVALIDATE))), != -1);
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
  Playable* const pp = PlayableFromRec(rec);
  if ((pp->GetFlags() & Playable::Enumerable) == 0)
    return;
  // Call event either immediately or later, asynchronuously.
  InfoChangeEvent(Playable::change_args(*pp, pp->EnsureInfoAsync(Playable::IF_Other|Playable::IF_Tech)), rec);
}

void PlaylistBase::UpdateChildren(RecordBase* const rp)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateChildren(%s)\n", this, RecordBase::DebugName(rp).cdata()));
  // get content
  Playable* const pp = PlayableFromRec(rp);

  DEBUGLOG(("PlaylistBase::UpdateChildren - %u\n", pp->GetStatus()));
  if ((pp->GetFlags() & Playable::Enumerable) == 0 || pp->GetStatus() <= STA_Invalid)
  { // Nothing to enumerate, delete children if any
    DEBUGLOG(("PlaylistBase::UpdateChildren - no children possible.\n"));
    if (RemoveChildren(rp))
      PostRecordCommand(rp, RC_UPDATESTATUS); // update icon
    return;
  }

  // Check if techinfo is available, otherwise wait
  if (pp->EnsureInfoAsync(Playable::IF_Tech) == 0)
  { StateFromRec(rp).WaitUpdate = true;
    return;
  }

  PMRASSERT(WinEnableWindowUpdate(HwndContainer, FALSE)); // suspend redraw
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
    Mutex::Lock lock(pp->Mtx); // Lock the collection
    int_ptr<PlayableInstance> pi;
    crp = NULL; // Last entry, insert new items after that.
    while ((pi = ((PlayableCollection*)pp)->GetNext(pi)) != NULL)
    { // Find entry in the current content
      RecordBase** orpp = old.begin();
      for (;;)
      { if (orpp == old.end())
        { // not found! => add
          DEBUGLOG(("PlaylistBase::UpdateChildren - not found: %p{%s}\n", &*pi, pi->GetPlayable()->GetURL().getShortName().cdata()));
          crp = AddEntry(pi, rp, crp);
          if (crp && (rp == NULL || (rp->flRecordAttr & CRA_EXPANDED)))
            RequestChildren(crp);
          break;
        }
        if ((*orpp)->Data->Content == pi)
        { // found!
          DEBUGLOG(("PlaylistBase::UpdateChildren - found: %p{%s} at %u\n", &*pi, pi->GetPlayable()->GetURL().getShortName().cdata(), orpp - old.begin()));
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
  PMRASSERT(WinEnableWindowUpdate(HwndContainer, TRUE));
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

void PlaylistBase::Apply2Source(void (*op)(Playable*)) const
{ DEBUGLOG(("PlaylistBase(%p)::Apply2Source(%p) - %u\n", this, op, Source.size()));
  for (RecordBase*const* rpp = Source.begin(); rpp != Source.end(); ++rpp)
    (*op)((*rpp)->Data->Content->GetPlayable());
}
void PlaylistBase::Apply2Source(void (PlaylistBase::*op)(Playable*))
{ DEBUGLOG(("PlaylistBase(%p)::Apply2Source(PlaylistBase::%p) - %u\n", this, op, Source.size()));
  for (RecordBase*const* rpp = Source.begin(); rpp != Source.end(); ++rpp)
    (this->*op)((*rpp)->Data->Content->GetPlayable());
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
  { Playable::Flags flg = ((*rpp)->Data->Content->GetPlayable()->GetFlags());
    if (flg == Playable::None)
      ret |= RT_Song;
    else if (flg & Playable::Mutable)
      ret |= RT_List;
    else if (flg & Playable::Enumerable)
      ret |= RT_Enum;
  }
  DEBUGLOG(("PlaylistBase::AnalyzeRecordTypes(): %u, %x\n", Source.size(), ret));
  return ret;
}


void PlaylistBase::UpdateStatus(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateStatus(%p)\n", this, rec));
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

void PlaylistBase::UpdateInstance(RecordBase* rec, PlayableInstance::StatusFlags iflags)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateInstance(%p, %x)\n", this, rec, iflags));
  if (rec == NULL)
  { // Root node => update title
    SetTitle();
  } else
  { // Record node
    if (iflags & PlayableInstance::SF_Alias)
    { xstring text = rec->Data->Content->GetDisplayName();
      rec->pszIcon = (PSZ)text.cdata();
      PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED)));
      rec->Data->Text = text; // now free the old text
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
  if (rec->Data->Content->GetStatus() == STA_Used)
    PostRecordCommand(rec, RC_UPDATESTATUS);
}

void PlaylistBase::InfoChangeEvent(const Playable::change_args& args, RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p{%s})::InfoChangeEvent({%p{%s}, %x}, %s)\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.GetURL().getShortName().cdata(), args.Flags, RecordBase::DebugName(rec).cdata()));

  if (args.Flags & Playable::IF_Other)
    PostRecordCommand(rec, RC_UPDATECHILDREN);
  if (args.Flags & Playable::IF_Status)
    PostRecordCommand(rec, RC_UPDATESTATUS);
  if (args.Flags & Playable::IF_Format)
    PostRecordCommand(rec, RC_UPDATEFORMAT);
  if (args.Flags & Playable::IF_Tech)
    PostRecordCommand(rec, RC_UPDATETECH);
  if (args.Flags & Playable::IF_Meta)
    PostRecordCommand(rec, RC_UPDATEMETA);
}

void PlaylistBase::StatChangeEvent(const PlayableInstance::change_args& args, RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p{%s})::StatChangeEvent({%p{%s}, %x}, %p)\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.GetPlayable()->GetURL().getShortName().cdata(), args.Flags, rec));

  if (args.Flags & PlayableInstance::SF_InUse)
    PostRecordCommand(rec, RC_UPDATESTATUS);
  if (args.Flags & PlayableInstance::SF_Alias)
    PostRecordCommand(rec, RC_UPDATEALIAS);
  if (args.Flags & PlayableInstance::SF_Slice)
    PostRecordCommand(rec, RC_UPDATEPOS);
}

void PlaylistBase::PlayStatEvent(const Ctrl::EventFlags& flags)
{ if (flags & Ctrl::EV_PlayStop)
  { // Cancel any outstanding UM_PLAYSTATUS ...
    QMSG qmsg;
    WinPeekMsg(amp_player_hab(), &qmsg, HwndFrame, UM_PLAYSTATUS, UM_PLAYSTATUS, PM_REMOVE);
    // ... and send a new one.
    PMRASSERT(WinPostMsg(HwndFrame, UM_PLAYSTATUS, MPFROMLONG(Ctrl::IsPlaying()), 0));
  }
}

void PlaylistBase::PluginEvent(const PLUGIN_EVENTARGS& args)
{ DEBUGLOG(("PlaylistBase(%p)::PluginEvent({%p, %x, %i})\n", this, args.plugin, args.type, args.operation));
  if (args.type == PLUGIN_DECODER)
  { switch (args.operation)
    {case PLUGIN_EVENTARGS::Enable:
     case PLUGIN_EVENTARGS::Disable:
     case PLUGIN_EVENTARGS::Unload:
      WinPostMsg(HwndFrame, UM_UPDATEDEC, 0, 0);
     default:; // avoid warnings
    }
  }
}  

static void DLLENTRY UserAddCallback(void* param, const char* url)
{ DEBUGLOG(("UserAddCallback(%p, %s)\n", param, url));
  PlaylistBase::UserAddCallbackParams& ucp = *(PlaylistBase::UserAddCallbackParams*)param;
  Playable* const pp = ucp.Parent ? &*ucp.Parent->Data->Content->GetPlayable() : ucp.GUI->GetContent();
  if ((pp->GetFlags() & Playable::Mutable) != Playable::Mutable)
    return; // Can't add something to a non-playlist.
  // On the first call lock the Playlist until the wizzard returns.
  if (ucp.Lock == NULL)
    ucp.Lock = new Mutex::Lock(pp->Mtx);
  ((Playlist*)pp)->InsertItem(url, (const char*)NULL, 0, ucp.Before ? &*ucp.Before->Data->Content : NULL);
}

void PlaylistBase::UserAdd(DECODER_WIZZARD_FUNC wizzard, RecordBase* parent, RecordBase* before)
{ DEBUGLOG(("PlaylistBase(%p)::UserAdd(%p, %p, %p)\n", this, wizzard, parent, before));
  Playable* const pp = PlayableFromRec(parent);
  if ((pp->GetFlags() & Playable::Mutable) != Playable::Mutable)
    return; // Can't add something to a non-playlist.
  // Dialog title
  UserAddCallbackParams ucp(this, parent, before);
  const xstring& title = "Append%s to " + (parent ? parent->Data->Content->GetDisplayName() : GetDisplayName());
  ULONG ul = (*wizzard)(HwndFrame, title, &UserAddCallback, &ucp);
  DEBUGLOG(("PlaylistBase::UserAdd - %u\n", ul));
  // TODO: cfg.listdir obsolete?
  //sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
}

void PlaylistBase::UserInsert(const InsertInfo* pii)
{ DEBUGLOG(("PlaylistBase(%p)::UserInsert(%p{{%s}, %s, %s, {%g,%g}, %p{%s}})\n", this,
    pii, pii->Parent->GetURL().getShortName().cdata(), pii->URL.cdata(), pii->Alias.cdata(), pii->Slice.Start, pii->Slice.Stop, pii->Before, pii->Before ? pii->Before->GetPlayable()->GetURL().cdata() : ""));
  pii->Parent->InsertItem(pii->URL, pii->Alias, pii->Slice, pii->Before);
}

void PlaylistBase::UserRemove(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UserRemove(%s)\n", this, rec->DebugName().cdata()));
  // find parent playlist
  Playable* playlist = PlayableFromRec(GetParent(rec));
  //DEBUGLOG(("PlaylistBase::UserRemove %s %p %p\n", RecordBase::DebugName(parent).cdata(), parent, playlist));
  if ((playlist->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
    ((Playlist&)*playlist).RemoveItem(rec->Data->Content);
    // the update of the container is implicitely done by the notification mechanism
}

void PlaylistBase::UserSave()
{ DEBUGLOG(("PlaylistBase(%p)::UserSave()\n", this));
  ASSERT(Content->GetFlags() & Playable::Enumerable);
  amp_save_playlist(HwndFrame, (PlayableCollection*)&*Content);
}

void PlaylistBase::UserOpenTreeView(Playable* pp)
{ if (pp->GetFlags() & Playable::Enumerable)
  { xstring alias;
    if (pp == Content)
      alias = Alias;
    PlaylistManager::Get(pp->GetURL(), alias)->SetVisible(true);
  }
}

void PlaylistBase::UserOpenDetailedView(Playable* pp)
{ if (pp->GetFlags() & Playable::Enumerable)
  { xstring alias;
    if (pp == Content)
      alias = Alias;
    PlaylistView::Get(pp->GetURL(), alias)->SetVisible(true);
  }
}

void PlaylistBase::UserClearPlaylist(Playable* pp)
{ if ((pp->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
    ((Playlist*)pp)->Clear();
}

void PlaylistBase::UserReload(Playable* pp)
{ if ( !(pp->GetFlags() & Playable::Enumerable)
    || !((PlayableCollection&)*pp).IsModified()
    || amp_query(HwndFrame, "The current list is modified. Discard changes?") )
    pp->LoadInfoAsync(Playable::IF_All);
}

void PlaylistBase::UserEditMeta(Playable* pp)
{ amp_info_edit(HwndFrame, pp);
}

void PlaylistBase::UserSort(Playable* pp)
{ DEBUGLOG(("PlaylistBase(%p)::UserSort(%p{%s})\n", this, pp, pp->GetURL().cdata()));
  if ((pp->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
    ((Playlist*)pp)->Sort(SortComparer);
}

void PlaylistBase::UserShuffle(Playable* pp)
{ if ((pp->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
    ((Playlist*)pp)->Shuffle();
}

int PlaylistBase::CompURL(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetPlayable()->GetURL() > r->GetPlayable()->GetURL();
}

int PlaylistBase::CompTitle(const PlayableInstance* l, const PlayableInstance* r)
{ return strnicmp(l->GetPlayable()->GetInfo().meta->title, r->GetPlayable()->GetInfo().meta->title, sizeof l->GetPlayable()->GetInfo().meta->title);
}

int PlaylistBase::CompArtist(const PlayableInstance* l, const PlayableInstance* r)
{ return strnicmp(l->GetPlayable()->GetInfo().meta->artist, r->GetPlayable()->GetInfo().meta->artist, sizeof l->GetPlayable()->GetInfo().meta->artist);
}

int PlaylistBase::CompAlbum(const PlayableInstance* l, const PlayableInstance* r)
{ return strnicmp(l->GetPlayable()->GetInfo().meta->album, r->GetPlayable()->GetInfo().meta->album, sizeof l->GetPlayable()->GetInfo().meta->album);
}

int PlaylistBase::CompAlias(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetDisplayName() > r->GetDisplayName();
}

int PlaylistBase::CompSize(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetPlayable()->GetInfo().tech->filesize > r->GetPlayable()->GetInfo().tech->filesize;
}

int PlaylistBase::CompTime(const PlayableInstance* l, const PlayableInstance* r)
{ return l->GetPlayable()->GetInfo().tech->songlength > r->GetPlayable()->GetInfo().tech->songlength;
}

url123 PlaylistBase::PlaylistSelect(HWND owner, const char* title)
{
  APSZ types[] = {{ FDT_PLAYLIST }, { 0 }};
  FILEDLG filedialog = { sizeof(FILEDLG) };
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = (PSZ)title;
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_PLAYLIST;

  strncpy( filedialog.szFullFile, cfg.listdir, sizeof filedialog.szFullFile );
  PMXASSERT(WinFileDlg(HWND_DESKTOP, owner, &filedialog), != NULLHANDLE);

  if( filedialog.lReturn == DID_OK )
  { sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof cfg.listdir );
    return url123::normalizeURL(filedialog.szFullFile);
  } else
  { return url123();
  }
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
      if (!DragAfter && target && amp_string_from_drghstr(pditem->hstrSourceName) == target->Data->Content->GetPlayable()->GetURL())
      { drag = DOR_NODROP;
        break;
      }
      if (pdinfo->usOperation == DO_DEFAULT)
        drag_op = pdinfo->hwndSource == HwndFrame ? DO_MOVE : DO_COPY;
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
  if ( ((DragAfter ? &*Content : PlayableFromRec(target))->GetFlags() & Playable::Mutable) != Playable::Mutable )
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
  int_ptr<Playlist> parent = (Playlist*)(DragAfter ? (Playable*)Content : PlayableFromRec(target));
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
      char* cp = fullname.raw_init(lenP + lenN);
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
          pdsource->Parent = parent;
          pdsource->Alias  = alias;
          pdsource->Before = target;

          pdtrans->cb               = sizeof( DRAGTRANSFER );
          pdtrans->hwndClient       = HwndFrame;
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
          if (cfg.recurse_dnd)
            fullname = fullname + "/?Recursive";
          else
            fullname = fullname + "/";
        }

        // prepare insert item
        InsertInfo* pii = new InsertInfo();
        pii->Parent = parent;
        pii->URL    = url123::normalizeURL(fullname);
        pii->Alias  = alias;
        if (target)
          pii->Before = target->Data->Content;
        WinPostMsg(HwndFrame, UM_INSERTITEM, MPFROMP(pii), 0);
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
        pdtrans->hwndClient       = HwndFrame;
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
          pii->Parent = parent;
          pii->URL    = amp_string_from_drghstr(pditem->hstrSourceName);
          pii->Alias  = alias;
          // TODO: The slice information is currently lost during D'n'd
          if (target)
            pii->Before = target->Data->Content;
          WinPostMsg(HwndFrame, UM_INSERTITEM, MPFROMP(pii), 0);
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
    pdsource->Parent->InsertItem(url123::normalizeURL(fullname), pdsource->Alias, pdsource->Before ? &*pdsource->Before->Data->Content : NULL);
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

  for (int i = 0; i < Source.size(); ++i)
  { RecordBase* rec = Source[i];
    DEBUGLOG(("PlaylistBase::DragInit: init item %i: %s\n", i, rec->DebugName().cdata()));
  
    // Prevent the records from beeing disposed.
    // The records are normally freed by DropEnd, except in case DrgDrag returns with an error.
    BlockRecord(rec);

    DRAGITEM* pditem = DrgQueryDragitemPtr(drag_infos, i);
    pditem->hwndItem          = HwndFrame;
    pditem->ulItemID          = (ULONG)rec;
    pditem->hstrType          = DrgAddStrHandle(DRT_BINDATA);
    pditem->hstrRMF           = DrgAddStrHandle("(DRM_123FILE,DRM_DISCARD)x(DRF_UNKNOWN)");
    pditem->hstrContainerName = DrgAddStrHandle(Content->GetURL());
    pditem->hstrSourceName    = DrgAddStrHandle(rec->Data->Content->GetPlayable()->GetURL());
    if (rec->Data->Content->GetAlias())
      pditem->hstrTargetName  = DrgAddStrHandle(rec->Data->Content->GetAlias());
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
  if (!DrgDrag(HwndFrame, drag_infos, drag_images, min(Source.size(), MAX_DRAG_IMAGES), VK_ENDDRAG, NULL))
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
    WinPostMsg(HwndFrame, UM_REMOVERECORD, MPFROMP(rec), 0);
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
    WinPostMsg(HwndFrame, UM_REMOVERECORD, MPFROMP(rec), 0);
  else 
    // Release the record locked in DragInit.
    FreeRecord(rec);
}

