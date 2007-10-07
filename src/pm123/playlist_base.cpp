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
#include "playlist_base.h"
#include "playlistmanager.i.h"
#include "playlist.i.h"
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
  return xstring::sprintf("%p{%p{%s}}", this, Content, Content->GetPlayable().GetURL().getObjName().cdata());
} 
xstring PlaylistBase::RecordBase::DebugName(const RecordBase* rec)
{ static const xstring nullstring = "<NULL>";
  if (!rec)
    return nullstring;
  return rec->DebugName();
}

xstring PlaylistBase::DebugName() const
{ return Content->GetURL().getObjName();
} 
#endif


/****************************************************************************
*
*  class PlaylistBase
*
****************************************************************************/

HPOINTER PlaylistBase::IcoPlayable[5][4];
HPOINTER PlaylistBase::IcoInvalid;

void PlaylistBase::InitIcons()
{ IcoPlayable[ICP_Song     ][IC_Normal] = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3);
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
  IcoInvalid                            = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3INVLD);
}

PlaylistBase::PlaylistBase(const char* url, const char* alias)
: Content(Playable::GetByURL(url)),
  Alias(alias),
  HwndFrame(NULLHANDLE),
  HwndContainer(NULLHANDLE),
  CmFocus(NULL),
  NoRefresh(false),
  RootInfoDelegate(*this, &InfoChangeEvent, NULL),
  RootPlayStatusDelegate(*this, &PlayStatEvent)
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

void PlaylistBase::PostRecordCommand(RecordBase* rec, RecordCommand cmd)
{ DEBUGLOG(("PlaylistBase(%p)::PostRecordCommand(%p, %u)\n", this, rec, cmd));
  Interlocked il(StateFromRec(rec).PostMsg);
  // Check whether the requested bit is already set or another event or there are other events pending.
  if (il.bitset(cmd) || il != (1U<<cmd))
  { DEBUGLOG(("PlaylistBase::PostRecordCommand - nope! %u\n", (unsigned)il));
    return; // requested command is already on the way or another unexecuted message is outstanding
  }
  // There is a little chance that we generate two messages for the same record.
  // The second one will be a no-op in the window procedure.  
  if (rec)
    InterlockedInc(rec->UseCount);
  if (!WinPostMsg(HwndFrame, UM_RECORDCOMMAND, MPFROMP(rec), MPFROMSHORT(TRUE)))
    FreeRecord(rec); // avoid memory leaks
}

void PlaylistBase::FreeRecord(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::FreeRecord(%p)\n", this, rec));
  if (rec && InterlockedDec(rec->UseCount) == 0)
    // we can safely post this message because the record is now no longer used anyway.
    WinPostMsg(HwndFrame, UM_DELETERECORD, MPFROMP(rec), 0);
}

void PlaylistBase::DeleteEntry(RecordBase* entry)
{ DEBUGLOG(("PlaylistBase(%p{%s})::DeleteEntry(%s)\n", this, DebugName().cdata(), RecordBase::DebugName(entry).cdata()));
  delete entry->Data;
  assert(LONGFROMMR(WinSendMsg(HwndContainer, CM_FREERECORD, MPFROMP(&entry), MPFROMSHORT(1))));
  DEBUGLOG(("PlaylistBase::DeleteEntry completed\n"));
}


MRESULT EXPENTRY pl_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistBase::DlgProcStub(%x, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  PlaylistBase* pb;
  if (msg == WM_INITDLG)
  { // Attach the class instance to the window.
    PlaylistBase::init_dlg_struct* ip = (PlaylistBase::init_dlg_struct*)mp2;
    DEBUGLOG(("PlaylistBase::DlgProcStub: WM_INITDLG - %p{%i, %p}}\n", ip, ip->size, ip->pm));
    WinSetWindowPtr(hwnd, QWL_USER, ip->pm);
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
  WinSendMsg(HwndFrame, WM_SETICON, (MPARAM)hicon, 0);
  do_warpsans(HwndFrame);
  { LONG     color;
    color = CLR_GREEN;
    WinSetPresParam(HwndContainer, PP_FOREGROUNDCOLORINDEX, sizeof(color), &color);
    color = CLR_BLACK;
    WinSetPresParam(HwndContainer, PP_BACKGROUNDCOLORINDEX, sizeof(color), &color);
  }

  rest_window_pos(HwndFrame, 0);

  // TODO: do not open all playlistmanager windows at the same location
  dk_add_window(HwndFrame, 0);
  
  // register events
  Content->InfoChange += RootInfoDelegate;
  PlayStatusChange += RootPlayStatusDelegate;
  
  SetTitle();
}

MRESULT PlaylistBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    { InitDlg();
      return 0;
    }

   case WM_DESTROY:
    { DEBUGLOG(("PlaylistBase::DlgProc: WM_DESTROY\n"));
      // Deregister Events
      RootInfoDelegate.detach();
      RootPlayStatusDelegate.detach();
      // process outstanding UM_DELETERECORD messages before we quit to ensure that all records are back to the PM before we die.
      QMSG qmsg;
      while (WinPeekMsg(amp_player_hab(), &qmsg, HwndFrame, UM_DELETERECORD, UM_DELETERECORD, PM_REMOVE))
      { DEBUGLOG2(("PlaylistManager::DlgProc: WM_DESTROY: %x %x %x %x\n", qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2));
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
      // Populate context menu with plug-in specific stuff.
      { HWND mh = HWNDFROMMP(mp2);
        int i = 2;
        for (;;)
        { SHORT id = SHORT1FROMMR(WinSendMsg(mh, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0));
          if (id == MIT_ERROR)
            break;
          WinSendMsg( mh, MM_DELETEITEM, MPFROM2SHORT( id, FALSE ), 0 );
        }
        memset(LoadWizzards+2, 0, sizeof LoadWizzards - 2*sizeof *LoadWizzards ); // You never know...
        append_load_menu( mh, SHORT1FROMMP(mp1) == IDM_PL_APPOTHER,
          LoadWizzards+2, sizeof LoadWizzards / sizeof *LoadWizzards - 2 );
      }
    }
    break;

   case WM_COMMAND:
    { SHORT cmd = SHORT1FROMMP(mp1);
      RecordBase* focus = CmFocus;
      if (RecordBase::IsRemoved(focus))
        return 0;
      if (cmd >= IDM_PL_APPFILE && cmd < IDM_PL_APPFILE + sizeof LoadWizzards / sizeof *LoadWizzards)
      { DECODER_WIZZARD_FUNC func = LoadWizzards[cmd-IDM_PL_APPFILE];
        if (func != NULL)
          UserAdd(func, "Append%s", focus); // focus from CN_CONTEXTMENU
        return 0;
      }
      switch (cmd)
      {
       case IDM_PL_USEALL:
        { amp_load_playable(Content->GetURL(), 0);
          return 0;
        }

       case IDM_PL_USE:
        { if (focus)
          amp_load_playable(focus->Content->GetPlayable().GetURL(), 0);
          return 0;
        }

       case IDM_PL_TREEVIEW:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_TREEVIEW %s\n", this, DebugName().cdata(), RecordBase::DebugName(focus).cdata()));
          // get new or existing instance
          PlaylistManager* pm = PlaylistManager::Get(PlayableFromRec(focus)->GetURL());
          pm->SetVisible(true);
          return 0;
        }
       
       case IDM_PL_DETAILEDALL:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_DETAILEDALL %s\n", this, DebugName().cdata(), RecordBase::DebugName(focus).cdata()));
          PlaylistView::Get(Content->GetURL())->SetVisible(true);
          return 0;
        }
       
       case IDM_PL_DETAILED:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_DETAILED %s\n", this, DebugName().cdata(), RecordBase::DebugName(focus).cdata()));
          if (focus)
          { Playable* pp = &focus->Content->GetPlayable();
            if (pp->GetFlags() & Playable::Enumerable)
              PlaylistView::Get(pp->GetURL())->SetVisible(true);
          }
          return 0;
        }
       
       case IDM_PL_EDIT:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_EDIT %s\n", this, DebugName().cdata(), RecordBase::DebugName(focus).cdata()));
          if (focus)
          { Playable* pp = &focus->Content->GetPlayable();
            if (pp->GetInfo().meta_write)
              amp_info_edit(HwndFrame, pp->GetURL(), pp->GetDecoder());
          }
          return 0;
        }
       
       case IDM_PL_REMOVE:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_REMOVE %p\n", this, DebugName().cdata(), focus));
          if (focus == NULL)
            return 0;
          // This won't work for the tree view. Overload it!
          if ((Content->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
            ((Playlist&)*Content).RemoveItem(focus->Content);
          // the update of the container is implicitely done by the notification mechanism
          return 0;
        }
        
       case IDM_PL_CLEAR:
        { DEBUGLOG(("PlaylistBase(%p{%s})::DlgProc: IDM_PL_CLEAR %p\n", this, DebugName().cdata(), focus));
          Playable* pp = PlayableFromRec(focus);
          if ((pp->GetFlags() & Playable::Mutable) == Playable::Mutable) // don't modify constant object
            ((Playlist*)pp)->Clear();
          return 0;
        }
        
       case IDM_PL_OPEN:
       case IDM_PL_OPENL:
        { url URL = PlaylistSelect(HwndFrame, "Open Playlist");
          if (URL)
          { PlaylistBase* pp = GetSame(URL);
            pp->SetVisible(true);
          }
        }
        
      } // switch (cmd)
      break;
    }

   case UM_RECORDCOMMAND:
    { RecordBase* rec = (RecordBase*)PVOIDFROMMP(mp1);
      // Do nothing but free the record in case this is requested.
      DEBUGLOG(("PlaylistBase::DlgProc: UM_RECORDCOMMAND: %s - %i\n", RecordBase::DebugName(rec).cdata(), SHORT1FROMMP(mp2)));
      if (SHORT1FROMMP(mp2))
        FreeRecord(rec);
      return 0;
    }

   case UM_DELETERECORD:
    DeleteEntry((RecordBase*)PVOIDFROMMP(mp1));
    return 0;
    
   case UM_SYNCREMOVE:
    { RecordBase* rec = (RecordBase*)PVOIDFROMMP(mp1);
      rec->Content = NULL;
      // deregister event handlers too.
      rec->Data->DeregisterEvents(); 
      return 0;
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
  HSWITCH hswitch = WinQuerySwitchHandle( HwndFrame, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( HwndFrame, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( HwndFrame, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}

HPOINTER PlaylistBase::CalcIcon(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase::CalcIcon(%s)\n", RecordBase::DebugName(rec).cdata()));
  assert(rec->Content != NULL);
  if (rec->Content->GetPlayable().GetStatus() == STA_Invalid)
    return IcoInvalid;
  else
    return IcoPlayable[GetPlayableType(rec)][GetRecordUsage(rec)];
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
  }
  const xstring& dn = Content->GetURL().getDisplayName(); // must not use a temporary in a conditional expresionne with ICC
  xstring title = xstring::sprintf("PM123: %s%s", (Alias ? Alias.cdata() : dn.cdata()), append);
  // Update Window Title
  if (WinSetWindowText(HwndFrame, (char*)title.cdata()))
    // now free the old title
    Title = title;
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
      WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_NOREPOSITION));
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
      WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED));
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
}

void PlaylistBase::UpdatePlayStatus(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::UpdatePlayStatus(%p)\n", this, rec));
  if (!rec->IsRemoved() && rec->Content->GetStatus() == STA_Used)
    PostRecordCommand(rec, RC_UPDATESTATUS);
}

void PlaylistBase::InfoChangeEvent(const Playable::change_args& args, RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p{%s})::InfoChangeEvent({%p{%s}, %x}, %s)\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.GetURL().getObjName().cdata(), args.Flags, RecordBase::DebugName(rec).cdata()));

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
    &args.Instance, args.Instance.GetPlayable().GetURL().getObjName().cdata(), args.Flags, rec));

  if (args.Flags & PlayableInstance::SF_Destroy)
  { WinSendMsg(HwndFrame, UM_SYNCREMOVE, MPFROMP(rec), 0);
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
  WinPostMsg(HwndFrame, UM_PLAYSTATUS, MPFROMLONG(status), 0);
}


struct UserAddCallbackParams
{ int_ptr<Playlist> Parent;
  PlayableInstance* Before;
  sco_ptr<Mutex::Lock> Lock;
  UserAddCallbackParams(Playlist* parent, PlayableInstance* before) : Parent(parent), Before(before) {}
};

static void DLLENTRY UserAddCallback(void* param, const char* url)
{ DEBUGLOG(("PlaylistManager:UserAddCallback(%p, %s)\n", param, url));
  UserAddCallbackParams& ucp = *(UserAddCallbackParams*)param;
  // TODO: Before might be no longer valid.
  // On the first call Lock the Playlist until the Wizzard returns.
  if (ucp.Lock == NULL)
    ucp.Lock = new Mutex::Lock(ucp.Parent->Mtx);
  ucp.Parent->InsertItem(url, ucp.Before);
} 

void PlaylistBase::UserAdd(DECODER_WIZZARD_FUNC wizzard, const char* title, RecordBase* parent, RecordBase* before)
{ DEBUGLOG(("PlaylistManager(%p)::UserAdd(%p, %s, %p, %p)\n", this, wizzard, title, parent, before));
  if (RecordBase::IsRemoved(parent) || RecordBase::IsRemoved(before))
    return; // sync remove happened
  Playable& play = parent ? parent->Content->GetPlayable() : *Content;
  if ((play.GetFlags() & Playable::Mutable) != Playable::Mutable)
    return; // Can't add something to a non-playlist.
  UserAddCallbackParams ucp(parent ? &(Playlist&)parent->Content->GetPlayable() : &(Playlist&)*Content, before ? before->Content : NULL);
  ULONG ul = (*wizzard)(HwndFrame, title, &UserAddCallback, &ucp);
  DEBUGLOG(("PlaylistManager::UserAdd - %u\n", ul));
  // TODO: cfg.listdir obsolete?
  //sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
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
  WinFileDlg(HWND_DESKTOP, owner, &filedialog);

  if( filedialog.lReturn == DID_OK )
  { sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof cfg.listdir );
    return url::normalizeURL(filedialog.szFullFile);
  } else
  { return url();
  }
}

