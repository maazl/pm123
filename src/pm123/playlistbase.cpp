/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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
#include "pm123.rc.h"
#include "docking.h"
#include "iniman.h"
#include "messages.h" // PlayStatusChange event

#include <stdarg.h>
#include <snprintf.h>
#include <cpp/smartptr.h>
#include "url.h"

#include <debuglog.h>


#ifdef DEBUG
xstring PlaylistBase::RecordBase::DebugName() const
{ if (IsRemoved())
    return xstring::sprintf("%p{<removed>}", this);
  return xstring::sprintf("%p{%p{%s}}", this, Content, Content->GetPlayable().GetURL().getShortName().cdata());
}
xstring PlaylistBase::RecordBase::DebugName(const RecordBase* rec)
{ static const xstring nullstring = "<NULL>";
  if (!rec)
    return nullstring;
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

PlaylistBase::PlaylistBase(const char* url, const char* alias, ULONG rid)
: Content(Playable::GetByURL(url)),
  Alias(alias),
  NameApp(""),
  DlgRID(rid),
  HwndFrame(NULLHANDLE),
  HwndContainer(NULLHANDLE),
  NoRefresh(false),
  Source(8),
  RootInfoDelegate(*this, &PlaylistBase::InfoChangeEvent, NULL),
  RootPlayStatusDelegate(*this, &PlaylistBase::PlayStatEvent),
  ThreadID(0),
  InitialVisible(false),
  Initialized(0)
{ DEBUGLOG(("PlaylistBase(%p)::PlaylistBase(%s)\n", this, url));
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
  PMRASSERT(WinPostMsg(HwndFrame, WM_QUIT, 0, 0));
  // This may give an error if called from our own thread. This is intensionally ignored here.
  ORASSERT(DosWaitThread(&ThreadID, DCWW_WAIT));
}

void TFNENTRY pl_DlgThreadStub(void* arg)
{ ((PlaylistBase*)arg)->DlgThread();
}

void PlaylistBase::StartDialog()
{ ThreadID = _beginthread(pl_DlgThreadStub, NULL, 1024*1024, this);
}

void PlaylistBase::DlgThread()
{ DEBUGLOG(("PlaylistBase(%p)::DlgThread()\n", this));
  // initialize PM
  HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);
  PMASSERT(hmq != NULLHANDLE);
  // initialize dialog
  init_dlg_struct ids = { sizeof(init_dlg_struct), this };
  PMRASSERT(WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, pl_DlgProcStub, NULLHANDLE, DlgRID, &ids));
  
  // get and dispatch messages from queue
  QMSG msg;
  while (WinGetMsg(hab, &msg, 0, 0, 0))
    WinDispatchMsg(hab, &msg);
  // cleanup
  save_window_pos(HwndFrame, 0);
  PMRASSERT(WinDestroyWindow(HwndFrame));
  PMRASSERT(WinDestroyMsgQueue(hmq));
  PMRASSERT(WinTerminate(hab));
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
  PlayStatusChange += RootPlayStatusDelegate;

  SetTitle();

  // TODO: acceleration table entries for plug-in extensions
  HAB    hab   = WinQueryAnchorBlock( HwndFrame );
  HACCEL accel = WinLoadAccelTable( hab, NULLHANDLE, ACL_PLAYLIST );
  PMASSERT(accel != NULLHANDLE);
  if( accel )
    PMRASSERT(WinSetAccelTable( hab, accel, HwndFrame ));
    
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

   case WM_INITMENU:
    switch (SHORT1FROMMP(mp1))
    {case IDM_PL_APPEND:
      { // Populate context menu with plug-in specific stuff.
        memset(LoadWizzards+2, 0, sizeof LoadWizzards - 2*sizeof *LoadWizzards ); // You never know...
        append_load_menu(HWNDFROMMP(mp2), SHORT1FROMMP(mp1) == IDM_PL_APPOTHER, 2,
          LoadWizzards+2, sizeof LoadWizzards/sizeof *LoadWizzards - 2);
      }
    }
    break;

   case WM_MENUEND:
    { GetSourceRecords(Source);
      DEBUGLOG(("PlaylistBase::DlgProc WM_MENUEND %u\n", Source.size()));
      if (Source.size() == 0)
      { // whole container was source
        PMRASSERT(WinSendMsg(HwndContainer, CM_SETRECORDEMPHASIS, MPFROMP(NULL), MPFROM2SHORT(FALSE, CRA_SOURCE)));
      } else
      { RecordBase** rpp = Source.end();
        while (rpp != Source.begin())
        { --rpp;
          PMRASSERT(WinSendMsg(HwndContainer, CM_SETRECORDEMPHASIS, MPFROMP(*rpp), MPFROM2SHORT(FALSE, CRA_SOURCE)));
        }
        // rpp is now implicitely equal to source.begin()
        PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(rpp), MPFROM2SHORT(Source.size(), CMA_NOREPOSITION)));
      }
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
    {case CN_CONTEXTMENU:
      { HWND hwndMenu = InitContextMenu((RecordBase*)mp2);
        if (hwndMenu != NULLHANDLE)
        { POINTL ptlMouse;
          PMRASSERT(WinQueryPointerPos(HWND_DESKTOP, &ptlMouse));
          // TODO: Mouse Position may not be reasonable, when the menu is invoked by keyboard.

          PMRASSERT(WinPopupMenu(HWND_DESKTOP, HwndFrame, hwndMenu, ptlMouse.x, ptlMouse.y, 0,
                                 PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD));
        }
        break;
      }
    }  
    break;
    
   case WM_COMMAND:
    { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: WM_COMMAND %u %u\n", this, DebugName().cdata(), SHORT1FROMMP(mp1), Source.size()));
      if (SHORT1FROMMP(mp1) >= IDM_PL_APPFILE && SHORT1FROMMP(mp1) < IDM_PL_APPFILE + sizeof LoadWizzards / sizeof *LoadWizzards)
      { DECODER_WIZZARD_FUNC func = LoadWizzards[SHORT1FROMMP(mp1)-IDM_PL_APPFILE];
        if (func != NULL && Source.size() == 1)
          UserAdd(func, "Append%s", Source[0]);
        return 0;
      }
      switch (SHORT1FROMMP(mp1))
      {case IDM_PL_USEALL:
        { amp_load_playable(Content->GetURL(), 0);
          return 0;
        }
       case IDM_PL_USE:
        { for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
            amp_load_playable((*rpp)->Content->GetPlayable().GetURL(), rpp == Source.begin() ? 0 : AMP_LOAD_APPEND);
          return 0;
        }
       case IDM_PL_TREEVIEWALL:
        { PlaylistManager::Get(Content->GetURL())->SetVisible(true);
          return 0;
        }
       case IDM_PL_TREEVIEW:
        { for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
          { Playable* pp = &(*rpp)->Content->GetPlayable();
            if (pp->GetFlags() & Playable::Enumerable)
              PlaylistManager::Get(pp->GetURL())->SetVisible(true);
          }
          return 0;
        }
       case IDM_PL_DETAILEDALL:
        { PlaylistView::Get(Content->GetURL())->SetVisible(true);
          return 0;
        }
       case IDM_PL_DETAILED:
        { for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
          { Playable* pp = &(*rpp)->Content->GetPlayable();
            if (pp->GetFlags() & Playable::Enumerable)
              PlaylistView::Get(pp->GetURL())->SetVisible(true);
          }
          return 0;
        }
       case IDM_PL_REFRESH:
        { for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
            (*rpp)->Content->GetPlayable().LoadInfoAsync(Playable::IF_All);
          return 0;
        }
       case IDM_PL_EDIT:
        { for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
          { Playable* pp = &(*rpp)->Content->GetPlayable();
            if (pp->GetInfo().meta_write)
              amp_info_edit(HwndFrame, pp->GetURL(), pp->GetDecoder());
          }
          return 0;
        }
       case IDM_PL_REMOVE:
        { // This won't work for the tree view. Overload it!
          for (RecordBase** rpp = Source.begin(); rpp != Source.end(); ++rpp)
          { if ((Content->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
              ((Playlist&)*Content).RemoveItem((*rpp)->Content);
          }
          // the update of the container is implicitely done by the notification mechanism
          return 0;
        }
       case IDM_PL_CLEARALL:
        { Playable* pp = Content;
          if ((pp->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
            ((Playlist*)pp)->Clear();
          return 0;
        }
       /*case IDM_PL_CLEAR:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_CLEAR %p\n", this, DebugName().cdata(), focus));
          for (RecordBase** rpp = source.begin(); rpp != source.end(); ++rpp)
          { Playable* pp = &(*rpp)->Content->GetPlayable();
            if ((pp->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
              ((Playlist*)pp)->Clear();
          }
          return 0;
        }*/
       case IDM_PL_RELOAD:
        { if ( !(Content->GetFlags() & Playable::Enumerable)
            || !((PlayableCollection&)*Content).IsModified()
            || amp_query(HwndFrame, "The current list is modified. Discard changes?") )
            Content->LoadInfoAsync(Playable::IF_All);
          return 0;
        }
       case IDM_PL_OPEN:
        { url URL = PlaylistSelect(HwndFrame, "Open Playlist");
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

   case UM_SYNCREMOVE:
    { RecordBase* rec = (RecordBase*)PVOIDFROMMP(mp1);
      DEBUGLOG(("PlaylistBase::DlgProc: UM_SYNCREMOVE: %s}\n", RecordBase::DebugName(rec).cdata()));
      rec->Content = NULL;
      // deregister event handlers too.
      rec->Data->DeregisterEvents();
      return MRFROMLONG(TRUE);
    }

   case UM_PLAYSTATUS:
    UpdatePlayStatus();
    return 0;
  }
  return WinDefDlgProc( HwndFrame, msg, mp1, mp2 );
}

int PlaylistBase::CompareTo(const char* str) const
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
{ ASSERT(rec->Content != NULL);
  DEBUGLOG(("PlaylistBase::CalcIcon(%s) - %u\n", RecordBase::DebugName(rec).cdata(), rec->Content->GetPlayable().GetStatus()));
  switch (rec->Content->GetPlayable().GetStatus())
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
  const xstring& dn = Content->GetURL().getDisplayName(); // must not use a temporary in a conditional expression with ICC
  xstring title = xstring::sprintf("PM123: %s%s%s%s", (Alias ? Alias.cdata() : dn.cdata()), NameApp,
    (Content->GetFlags() & Playable::Enumerable) && ((PlayableCollection&)*Content).IsModified() ? " (*)" : "", append);
  // Update Window Title
  PMRASSERT(WinSetWindowText(HwndFrame, (char*)title.cdata()));
  // now free the old title
  Title = title;
}

PlaylistBase::RecordBase* PlaylistBase::AddEntry(PlayableInstance* obj, RecordBase* parent, RecordBase* after)
{ DEBUGLOG(("PlaylistBase(%p{%s})::AddEntry(%p{%s}, %p, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable().GetURL().getShortName().cdata(), parent, after));
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
  // detach content
  entry->Content = NULL;
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
  Playable* pp;
  if (rec == NULL)
    pp = Content;
  else if (rec->IsRemoved())
    return;
  else
    pp = &rec->Content->GetPlayable();
  if ((pp->GetFlags() & Playable::Enumerable) == 0)
    return;
  // Call event either immediately or later, asynchronuously.
  InfoChangeEvent(Playable::change_args(*pp, pp->EnsureInfoAsync(Playable::IF_Other|Playable::IF_Tech)), rec);
}

void PlaylistBase::UpdateChildren(RecordBase* const rp)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateChildren(%s)\n", this, RecordBase::DebugName(rp).cdata()));
  // get content
  Playable* pp;
  if (rp == NULL)
    pp = Content;
  else if (rp->IsRemoved())
    return; // record removed
  else
    pp = &rp->Content->GetPlayable();

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
  // The collection is not locked so far.
  vector<RecordBase> old(32);
  RecordBase* crp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(rp ? CMA_FIRSTCHILD : CMA_FIRST, CMA_ITEMORDER));
  PMASSERT(crp != (RecordBase*)-1);
  while (crp != NULL && crp != (RecordBase*)-1)
  { DEBUGLOG(("PlaylistBase::UpdateChildren CM_QUERYRECORD: %s\n", RecordBase::DebugName(crp).cdata()));
    // prefetch next record
    RecordBase* ncrp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    PMASSERT(ncrp != (RecordBase*)-1);
    if (crp->Content)
      // record is valid => add to list
      old.insert(old.size()) = crp;
    else
      // Remove records where UM_SYNCREMOVE has been arrived to reduce the number of record moves.
      RemoveEntry(crp);
    crp = ncrp;
  }

  { // Now check what should be in the container
    DEBUGLOG(("PlaylistBase::UpdateChildren - check container.\n"));
    Mutex::Lock lock(pp->Mtx); // Lock the collection
    sco_ptr<PlayableEnumerator> ep(((PlayableCollection*)pp)->GetEnumerator());
    crp = NULL; // Last entry, insert new items after that.
    while (ep->Next())
    { // Find entry in the current content
      RecordBase** orpp = old.begin();
      for (;;)
      { if (orpp == old.end())
        { // not found! => add
          DEBUGLOG(("PlaylistBase::UpdateChildren - not found: %p{%s}\n", &**ep, (*ep)->GetPlayable().GetURL().getShortName().cdata()));
          crp = AddEntry(&**ep, rp, crp);
          if (crp && (rp == NULL || (rp->flRecordAttr & CRA_EXPANDED)))
            RequestChildren(crp);
          break;
        }
        // In case we received a UM_SYNCREMOVE event for a record, the comparsion below will fail.
        if ((*orpp)->Content == &**ep)
        { // found!
          DEBUGLOG(("PlaylistBase::UpdateChildren - found: %p{%s} at %u\n", &**ep, (*ep)->GetPlayable().GetURL().getShortName().cdata(), orpp - old.begin()));
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

void PlaylistBase::GetSourceRecords(vector<RecordBase>& result) const
{ result.clear();
  RecordBase* rec = (RecordBase*)PVOIDFROMMR(WinSendMsg(HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMP(CMA_FIRST), MPFROMSHORT(CRA_SOURCE)));
  while (rec != NULL && rec != (RecordBase*)-1)
  { DEBUGLOG(("PlaylistBase::GetSourceRecords: %p\n", rec));
    if (!rec->IsRemoved()) // Skip removed
      result.append() = rec;
    rec = (RecordBase*)PVOIDFROMMR(WinSendMsg(HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMP(rec), MPFROMSHORT(CRA_SOURCE)));
  }
  PMASSERT(rec != (RecordBase*)-1);
}

void PlaylistBase::UpdateStatus(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UpdateStatus(%p)\n", this, rec));
  if (rec == NULL)
  { // Root node => update title
    SetTitle();
  } else
  { // Record node
    if (rec->IsRemoved())
      return;
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
    if (rec->IsRemoved())
      return;
    if (iflags & PlayableInstance::SF_Alias)
    { xstring text = rec->Content->GetDisplayName();
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
  if (!rec->IsRemoved() && rec->Content->GetStatus() == STA_Used)
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
    &args.Instance, args.Instance.GetPlayable().GetURL().getShortName().cdata(), args.Flags, rec));

  if (args.Flags & PlayableInstance::SF_Destroy)
  { DEBUGLOG(("PlaylistBase::StatChangeEvent: SF_Destroy %p\n", rec));
    RASSERT(WinSendMsg(HwndFrame, UM_SYNCREMOVE, MPFROMP(rec), 0));
    return;
  }

  if (args.Flags & PlayableInstance::SF_InUse)
    PostRecordCommand(rec, RC_UPDATESTATUS);
  if (args.Flags & PlayableInstance::SF_Alias)
    PostRecordCommand(rec, RC_UPDATEALIAS);
  if (args.Flags & PlayableInstance::SF_Alias)
    PostRecordCommand(rec, RC_UPDATEPOS);
}

void PlaylistBase::PlayStatEvent(const bool& status)
{ // Cancel any outstanding UM_PLAYSTATUS ...
  QMSG qmsg;
  WinPeekMsg(amp_player_hab(), &qmsg, HwndFrame, UM_PLAYSTATUS, UM_PLAYSTATUS, PM_REMOVE);
  // ... and send a new one.
  PMRASSERT(WinPostMsg(HwndFrame, UM_PLAYSTATUS, MPFROMLONG(status), 0));
}


static void DLLENTRY UserAddCallback(void* param, const char* url)
{ DEBUGLOG(("UserAddCallback(%p, %s)\n", param, url));
  PlaylistBase::UserAddCallbackParams& ucp = *(PlaylistBase::UserAddCallbackParams*)param;
  // parent might be no longer valid.
  if (PlaylistBase::RecordBase::IsRemoved(ucp.Parent))
    return; // Ignore (can't help)
  int_ptr<Playable> play = ucp.Parent ? &ucp.Parent->Content->GetPlayable() : ucp.GUI->GetContent();
  if ((play->GetFlags() & Playable::Mutable) != Playable::Mutable)
    return; // Can't add something to a non-playlist.
  // On the first call Lock the Playlist until the Wizzard returns.
  if (ucp.Lock == NULL)
    ucp.Lock = new Mutex::Lock(play->Mtx);
  // parent and before might be no longer valid.
  if (PlaylistBase::RecordBase::IsRemoved(ucp.Parent) || PlaylistBase::RecordBase::IsRemoved(ucp.Before))
    return; // Ignore (can't help)
  ((Playlist&)*play).InsertItem(url, (const char*)NULL, 0, ucp.Before ? ucp.Before->Content : NULL);
}

void PlaylistBase::UserAdd(DECODER_WIZZARD_FUNC wizzard, const char* title, RecordBase* parent, RecordBase* before)
{ DEBUGLOG(("PlaylistBase(%p)::UserAdd(%p, %s, %p, %p)\n", this, wizzard, title, parent, before));
  if (RecordBase::IsRemoved(parent) || RecordBase::IsRemoved(before))
    return; // sync remove happened
  Playable& play = parent ? parent->Content->GetPlayable() : *Content;
  if ((play.GetFlags() & Playable::Mutable) != Playable::Mutable)
    return; // Can't add something to a non-playlist.
  UserAddCallbackParams ucp(this, parent, before);
  ULONG ul = (*wizzard)(HwndFrame, title, &UserAddCallback, &ucp);
  DEBUGLOG(("PlaylistBase::UserAdd - %u\n", ul));
  // TODO: cfg.listdir obsolete?
  //sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
}

void PlaylistBase::UserSave()
{ DEBUGLOG(("PlaylistBase(%p)::UserSave()\n", this));
  ASSERT(Content->GetFlags() & Playable::Enumerable);

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

  if ((Content->GetFlags() & Playable::Mutable) == Playable::Mutable && Content->GetURL().isScheme("file://"))
  { // Playlist => save in place allowed => preselect our own file name
    const char* cp = Content->GetURL().cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    strlcpy(filedialog.szFullFile, cp, sizeof filedialog.szFullFile);
    // preselect file type
    if (Content->GetURL().getExtension().compareToI(".M3U") == 0)
      filedialog.pszIType = FDT_PLAYLIST_M3U;
    // TODO: other playlist types
  } else
  { // not mutable => only save as allowed
    // TODO: preselect directory
  }

  PMXASSERT(WinFileDlg(HWND_DESKTOP, HwndFrame, &filedialog), != NULLHANDLE);

  if(filedialog.lReturn == DID_OK)
  { url file = url::normalizeURL(filedialog.szFullFile);
    if (!(Playable::IsPlaylist(file)))
    { if (file.getExtension().length() == 0)
      { // no extension => choose automatically
        if (strcmp(filedialog.pszIType, FDT_PLAYLIST_M3U) == 0)
          file = file + ".m3u";
        else // if (strcmp(filedialog.pszIType, FDT_PLAYLIST_LST) == 0)
          file = file + ".lst";
        // TODO: other playlist types
      } else
      { amp_error(HwndFrame, "PM123 cannot write playlist files with the unsupported extension %s.", file.getExtension().cdata());
        return;
      }
    }
    const char* cp = file.cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    if (amp_warn_if_overwrite(HwndFrame, cp))
    { PlayableCollection::save_options so = PlayableCollection::SaveDefault;
      if (file.getExtension().compareToI(".m3u") == 0)
        so |= PlayableCollection::SaveAsM3U;
      if (filedialog.ulUser & FDU_RELATIV_ON)
        so |= PlayableCollection::SaveRelativePath;
      // now save
      if (!((PlayableCollection&)*Content).Save(file, so))
        amp_error(HwndFrame, "Failed to create playlist \"%s\". Error %s.", file.cdata(), xio_strerror(xio_errno()));
    }
  }
}

url PlaylistBase::PlaylistSelect(HWND owner, const char* title)
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
    return url::normalizeURL(filedialog.szFullFile);
  } else
  { return url();
  }
}

