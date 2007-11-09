/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2007-2007 Marcel Mueller
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
#include "pm123.h"
#include "pm123.rc.h"
#include <utilfct.h>
#include "playlistmanager.h"
#include "docking.h"
#include "iniman.h"
#include "playable.h"

#include <cpp/smartptr.h>
#include "url.h"

#include <debuglog.h>

/****************************************************************************
*
*  class PlaylistManager
*
****************************************************************************/


PlaylistManager::PlaylistManager(const char* url, const char* alias)
: PlaylistRepository<PlaylistManager>(url, alias, DLG_PM),
  MainMenu(NULLHANDLE),
  ListMenu(NULLHANDLE),
  FileMenu(NULLHANDLE)
{ DEBUGLOG(("PlaylistManager(%p)::PlaylistManager(%s, %s)\n", this, url, alias));
  NameApp = " (Tree)";
  //WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, pl_DlgProcStub, NULLHANDLE, DLG_PM, &ids);
  StartDialog();
}

void PlaylistManager::PostRecordCommand(RecordBase* rec, RecordCommand cmd)
{ DEBUGLOG(("PlaylistManager(%p)::PostRecordCommand(%p, %x)\n", this, rec, cmd));
  // Ignore some messages
  switch (cmd)
  {case RC_UPDATEFORMAT:
   case RC_UPDATEMETA:
   case RC_UPDATEPOS:
    return;
   default:
    PlaylistBase::PostRecordCommand(rec, cmd);
  }
}

void PlaylistManager::InitDlg()
{ DEBUGLOG(("PlaylistManager(%p{%s})::InitDlg()\n", this, DebugName().cdata()));
  #ifdef DEBUG
  HwndContainer = WinCreateWindow( HwndFrame, WC_CONTAINER, "", WS_VISIBLE|CCS_SINGLESEL|CCS_MINIICONS|CCS_MINIRECORDCORE|CCS_VERIFYPOINTERS,
                                   0, 0, 0, 0, HwndFrame, HWND_TOP, FID_CLIENT, NULL, NULL);
  #else
  HwndContainer = WinCreateWindow( HwndFrame, WC_CONTAINER, "", WS_VISIBLE|CCS_SINGLESEL|CCS_MINIICONS|CCS_MINIRECORDCORE,
                                   0, 0, 0, 0, HwndFrame, HWND_TOP, FID_CLIENT, NULL, NULL);
  #endif
  PMASSERT(HwndContainer != NULLHANDLE);

  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.flWindowAttr = CV_TREE|CV_NAME|CV_MINI|CA_TREELINE|CA_CONTAINERTITLE|CA_TITLELEFT|CA_TITLESEPARATOR|CA_TITLEREADONLY;
  //cnrInfo.hbmExpanded  = NULLHANDLE;
  //cnrInfo.hbmCollapsed = NULLHANDLE;
  cnrInfo.cxTreeIndent = WinQuerySysValue(HWND_DESKTOP, SV_CYMENU) +2;
  //cnrInfo.cyLineSpacing = 0;
  DEBUGLOG(("PlaylistManager::InitDlg: %u\n", cnrInfo.slBitmapOrIcon.cx));
  cnrInfo.pszCnrTitle  = "No playlist selected. Right click for menu.";
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_FLWINDOWATTR|CMA_CXTREEINDENT|CMA_CNRTITLE|CMA_LINESPACING)));

  PlaylistBase::InitDlg();
}

MRESULT PlaylistManager::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistManager(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_DESTROY:
    // delete all records
    { DEBUGLOG(("PlaylistManager::DlgProc: WM_DESTROY\n"));
      RemoveChildren(NULL);
      WinDestroyWindow(MainMenu);
      WinDestroyWindow(ListMenu);
      WinDestroyWindow(FileMenu);
      break; // continue in base class
    }

   case WM_CONTROL:
    switch (SHORT2FROMMP(mp1))
    {
#if 0
     case CN_INITDRAG:
      drag = (PMYMPREC) (((PCNRDRAGINIT) mp2) -> pRecord);
      if (drag != NULL)
      {

      }
      break;
#endif

     case CN_HELP:
      amp_show_help( IDH_PM );
      return 0;

     case CN_EMPHASIS:
      { PNOTIFYRECORDEMPHASIS emphasis = (PNOTIFYRECORDEMPHASIS)mp2;
        if (emphasis->fEmphasisMask & CRA_CURSORED)
        { // Update title
          Record* rec = (Record*)emphasis->pRecord;
          if (rec != NULL && !rec->IsRemoved() && (rec->flRecordAttr & CRA_CURSORED))
          { EmFocus = &rec->Content->GetPlayable();
            if (EmFocus->EnsureInfoAsync(Playable::IF_Tech))
              PMRASSERT(WinPostMsg(HwndFrame, UM_UPDATEINFO, MPFROMP(&*EmFocus), 0));
          }
        }
      }
      return 0;

     case CN_EXPANDTREE:
      { Record* rec = (Record*)PVOIDFROMMP(mp2);
        DEBUGLOG(("PlaylistManager::DlgProc CN_EXPANDTREE %p\n", rec));
        PostRecordCommand(rec, RC_UPDATESTATUS);
        // iterate over children
        rec = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rec), MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));
        while (rec != NULL && rec != (Record*)-1)
        { DEBUGLOG(("CM_QUERYRECORD: %s\n", Record::DebugName(rec).cdata()));
          RequestChildren(rec);
          rec = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rec), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
        }
        PMASSERT(rec != (Record*)-1);
      }
      return 0;

     case CN_COLLAPSETREE:
      PostRecordCommand((Record*)PVOIDFROMMP(mp2), RC_UPDATESTATUS);
      return 0;

     case CN_CONTEXTMENU:
      { Record* rec = (Record*)mp2; // save focus for later processing of menu messages
        DEBUGLOG(("PlaylistManager::DlgProc CN_CONTEXTMENU %p\n", rec));
        if (rec)
        { if (rec->IsRemoved())
            return 0;
          PMRASSERT(WinPostMsg(HwndContainer, CM_SETRECORDEMPHASIS, MPFROMP(rec), MPFROM2SHORT(TRUE, CRA_CURSORED)));
        }
        CmFocus = rec;
        POINTL ptlMouse;
        PMRASSERT(WinQueryPointerPos(HWND_DESKTOP, &ptlMouse));
        // TODO: Mouse Position may not be reasonable, when the menu is invoked by keyboard.

        DEBUGLOG(("PlaylistManager::DlgProc A\n"));
        HWND   hwndMenu;
        if (rec == NULL)
        { if (MainMenu == NULLHANDLE)
          { MainMenu = WinLoadMenu(HWND_OBJECT, 0, PM_MAIN_MENU);
            PMASSERT(MainMenu != NULLHANDLE);
            mn_make_conditionalcascade(MainMenu, IDM_PL_APPEND, IDM_PL_APPFILE);
          }
          hwndMenu = MainMenu;
          mn_enable_item( hwndMenu, IDM_PL_APPEND, (Content->GetFlags() & Playable::Mutable) == Playable::Mutable );
          DEBUGLOG(("PlaylistManager::DlgProc B\n"));
        } else if (rec->Content->GetPlayable().GetFlags() & Playable::Enumerable)
        { if (ListMenu == NULLHANDLE)
          { ListMenu = WinLoadMenu(HWND_OBJECT, 0, PM_LIST_MENU);
            PMASSERT(ListMenu != NULLHANDLE);
            mn_make_conditionalcascade(ListMenu, IDM_PL_APPEND, IDM_PL_APPFILE);
          }
          if (Record::IsRemoved(rec))
            return 0; // record could be removed at WinLoadMenu
          hwndMenu = ListMenu;
          mn_enable_item( hwndMenu, IDM_PL_APPEND, (rec->Content->GetPlayable().GetFlags() & Playable::Mutable) == Playable::Mutable );
        } else
        { if (FileMenu == NULLHANDLE)
          { FileMenu = WinLoadMenu(HWND_OBJECT, 0, PM_FILE_MENU);
            PMASSERT(FileMenu != NULLHANDLE);
          }
          hwndMenu = FileMenu;
          mn_enable_item(hwndMenu, IDM_PL_EDIT, rec->Content->GetPlayable().GetInfo().meta_write);
        }
        DEBUGLOG(("PlaylistManager::DlgProc C %p\n", hwndMenu));
        if (Record::IsRemoved(rec))
          return 0; // record could be removed now
        DEBUGLOG(("PlaylistManager::DlgProc: Menu: %p %p %p\n", MainMenu, ListMenu, FileMenu));
        PMRASSERT(WinPopupMenu(HWND_DESKTOP, HwndFrame, hwndMenu, ptlMouse.x, ptlMouse.y, 0,
                               PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD));
      }
      return 0;

     case CN_BEGINEDIT:
      // TODO: normally we have to lock some functions here...
      return 0;

     case CN_REALLOCPSZ:
      { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
        Record* rec = (Record*)ed->pRecord;
        DEBUGLOG(("PlaylistManager::DlgProc CN_REALLOCPSZ %p{,%p->%p{%s},%u,} %p\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText, rec));
        *ed->ppszText = rec->Data()->Text.raw_init(ed->cbText); // because of the string sharing we always have to initialize the instance
      }
      return MRFROMLONG(TRUE);

     case CN_ENDEDIT:
      { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
        Record* rec = (Record*)ed->pRecord;
        DEBUGLOG(("PlaylistManager::DlgProc CN_ENDEDIT %p{,%p->%p{%s},%u,} %p\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText, rec));
        if (rec->IsRemoved())
          return 0; // record removed
        NoRefresh = true;
        rec->Content->SetAlias(**ed->ppszText ? *ed->ppszText : NULL);
        NoRefresh = false;
      }
      return 0;
    } // switch (SHORT2FROMMP(mp1))
    break;

   case WM_COMMAND:
    { SHORT cmd = SHORT1FROMMP(mp1);
      Record* focus = (Record*)CmFocus;
      if (RecordBase::IsRemoved(focus))
        return 0;
      switch (cmd)
      {
       case IDM_PL_REMOVE:
        { DEBUGLOG(("PlaylistManager(%p{%s})::DlgProc: IDM_PL_REMOVE %p\n", this, DebugName().cdata(), focus));
          if (focus == NULL)
            return 0;
          // find parent playlist
          Record* parent = focus->Data()->Parent;
          Playable* playlist = PlayableFromRec(parent);
          if (playlist->GetFlags() & Playable::Mutable) // don't modify constant object
            ((Playlist&)*playlist).RemoveItem(focus->Content);
          // the update of the container is implicitely done by the notification mechanism
          return 0;
        }
      } // switch (SHORT1FROMMP(mp1))
      break;
    }

   case WM_WINDOWPOSCHANGED:
    // TODO: Bl”dsinn
    { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
      if( pswp[0].fl & SWP_SHOW )
        cfg.show_plman = TRUE;
      if( pswp[0].fl & SWP_HIDE )
        cfg.show_plman = FALSE;
      break;
    }

   case UM_UPDATEINFO:
    { if (&*EmFocus != (Playable*)mp1)
        return 0; // No longer neccessary, because another UM_UPDATEINFO message is in the queue or the record does no longer exists.
      SetInfo(xstring::sprintf("%s, %s", &*EmFocus->GetURL().getDisplayName(), EmFocus->GetInfo().tech->info));
      if (&*EmFocus == (Playable*)mp1) // double check, because EmFocus may change while SetInfo
        EmFocus = NULL;
      return 0;
    }

   case UM_RECORDCOMMAND:
    { Record* rec = (Record*)PVOIDFROMMP(mp1);
      DEBUGLOG(("PlaylistManager::DlgProc: UM_RECORDCOMMAND: %s %x\n", Record::DebugName(rec).cdata(), StateFromRec(rec).PostMsg));
      Interlocked il(StateFromRec(rec).PostMsg);
      do
      { // We do the processing here step by step because the processing may set some of the bits
        // that are handled later. This avoids double actions and reduces the number of posted messages.
        if (il.bitrst(RC_UPDATETECH))
        { // Check if UpdateChildren is waiting
          bool& wait = StateFromRec(rec).WaitUpdate;
          if (wait)
          { // Try again
            wait = false;
            il.bitset(RC_UPDATECHILDREN);
          }
          UpdateTech(rec); // may set RC_UPDATESTATUS
        }
        if (il.bitrst(RC_UPDATECHILDREN))
          UpdateChildren(rec); // may set RC_UPDATESTATUS
        if (il.bitrst(RC_UPDATESTATUS))
          UpdateStatus(rec);
        if (il.bitrst(RC_UPDATEALIAS))
          UpdateInstance(rec, PlayableInstance::SF_Alias);
        // It is essential that all messages that are not masked by the overloaded PosRecordCommand
        // are handled here. Otherwise: infinite loop. The assertion below checks for this.
        ASSERT((il & ~(1<<RC_UPDATETECH | 1<<RC_UPDATECHILDREN | 1<<RC_UPDATESTATUS | 1<<RC_UPDATEALIAS)) == 0);
        il &= ~(1<<RC_UPDATETECH | 1<<RC_UPDATECHILDREN | 1<<RC_UPDATESTATUS | 1<<RC_UPDATEALIAS);
      } while (il);
      break; // continue in base class
    }

  } // switch (msg)

  return PlaylistBase::DlgProc(msg, mp1, mp2);
}


void PlaylistManager::SetInfo(const xstring& text)
{ DEBUGLOG(("PlaylistManager(%p)::SetInfo(%s)\n", this, text.cdata()));
  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.pszCnrTitle = (char*)text.cdata(); // OS/2 API doesn't like const...
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_CNRTITLE)));
  // now free the old title
  Info = text;
}

PlaylistBase::ICP PlaylistManager::GetPlayableType(RecordBase* rec)
{ DEBUGLOG(("PlaylistManager::GetPlaylistState(%s)\n", Record::DebugName(rec).cdata()));
  if ((rec->Content->GetPlayable().GetFlags() & Playable::Enumerable) == 0)
    return ICP_Song;
  if (rec->Content->GetPlayable().GetInfo().tech->num_items == 0)
    return ICP_Empty;
  if (((CPData*)(rec->Data))->Recursive)
    return ICP_Recursive;
  return (rec->flRecordAttr & CRA_EXPANDED) ? ICP_Open : ICP_Closed;
}

PlaylistBase::IC PlaylistManager::GetRecordUsage(RecordBase* rec)
{ DEBUGLOG(("PlaylistManager::GetRecordUsage(%s)\n", Record::DebugName(rec).cdata()));
  if (rec->Content->GetPlayable().GetStatus() != STA_Used)
  { DEBUGLOG(("PlaylistManager::GetRecordUsage: unused\n"));
    return IC_Normal;
  }
  // Check wether the current call stack is the same as for the current Record ...
  const Playable* root = amp_get_current_root(); // We need no ownership here, since we only compare the reference
  do
  { if (&rec->Content->GetPlayable() == root)
    { // We are a the current root, so the call stack compared equal.
      DEBUGLOG(("PlaylistManager::GetRecordUsage: current root\n"));
      if (!((CPData*)(rec->Data))->Recursive)
        break;
      return IC_Used;
    }
    if (rec->Content->GetStatus() != STA_Used)
    { // The PlayableInstance is not used, so the call stack is not equal.
      DEBUGLOG(("PlaylistManager::GetRecordUsage: instance unused\n"));
      return IC_Used;
    }
    rec = ((CPData*)(rec->Data))->Parent;
  } while (rec != NULL);
  // No more Parent, let's consider this as (partially) equal.
  DEBUGLOG(("PlaylistManager::GetRecordUsage: top level\n"));
  return decoder_playing() ? IC_Play : IC_Active;
}

bool PlaylistManager::RecursionCheck(Playable* pp, RecordBase* parent)
{ DEBUGLOG(("PlaylistManager(%p)::RecursionCheck(%p{%s}, %s)\n", this, pp, pp->GetURL().getShortName().cdata(), Record::DebugName(parent).cdata()));
  if (pp != &*Content)
  { for(;;)
    { if (parent == NULL || parent == (Record*)-1)
      { DEBUGLOG(("PlaylistManager::RecursionCheck: no rec.\n"));
        return false;
      }
      if (parent->IsRemoved())
         // Well, we return true in case of a removed record to avoid more actions
         return true;
      if (&parent->Content->GetPlayable() == pp)
        break;
      parent = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(parent), MPFROM2SHORT(CMA_PARENT, CMA_ITEMORDER));
      PMASSERT(parent != (Record*)-1);
      DEBUGLOG(("PlaylistManager::RecursionCheck: recusrion check %s\n", Record::DebugName(parent).cdata()));
    }
    // recursion in playlist tree
  } // else recursion with top level
  DEBUGLOG(("PlaylistManager::RecursionCheck: recursion!\n"));
  return true;
}
bool PlaylistManager::RecursionCheck(RecordBase* rp)
{ DEBUGLOG(("PlaylistManager::RecursionCheck(%s)\n", Record::DebugName(rp).cdata()));
  Playable* pp = &rp->Content->GetPlayable();
  if (pp != &*Content)
  { do
    { rp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(CMA_PARENT, CMA_ITEMORDER));
      PMASSERT(rp != (Record*)-1);
      DEBUGLOG2(("PlaylistManager::RecursionCheck: recusrion check %p\n", rp));
      if (rp == NULL || rp == (Record*)-1)
        return false;
      if (rp->IsRemoved())
         // Well, we return true in case of a removed record to avoid more actions
         return true;
    } while (&rp->Content->GetPlayable() != pp);
    // recursion in playlist tree
  } // else recursion with top level
  DEBUGLOG(("PlaylistManager::RecursionCheck: recursion!\n"));
  return true;
}

PlaylistBase::RecordBase* PlaylistManager::CreateNewRecord(PlayableInstance* obj, RecordBase* parent)
{ DEBUGLOG(("PlaylistManager(%p{%s})::CreateNewRecord(%p{%s}, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable().GetURL().getShortName().cdata(), parent));
  // Allocate a record in the HwndContainer
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  PMASSERT(rec != NULL);
  
  if (Record::IsRemoved(parent))
  { // Parent removed => forget about it
    rec->Data() = NULL;
    // delete the record
    PMRASSERT(WinPostMsg(HwndFrame, UM_DELETERECORD, MPFROMP(rec), 0));
    return NULL;
  }
  rec->Content         = obj;
  rec->UseCount        = 1;
  rec->Data()          = new CPData(*this, &PlaylistManager::InfoChangeEvent, &PlaylistManager::StatChangeEvent, rec, (Record*)parent);
  // before we catch any information setup the update events
  // The record is not yet corretly initialized, but this don't metter since all that the event handlers can do
  // is to post a UM_RECORDCOMMAND which is not handled unless this message is completed.
  obj->GetPlayable().InfoChange += rec->Data()->InfoChange;
  obj->StatusChange             += rec->Data()->StatChange;
  // now get initial info's
  rec->Data()->Text    = obj->GetDisplayName();
  rec->Data()->Recursive = obj->GetPlayable().GetInfo().tech->recursive && RecursionCheck(&obj->GetPlayable(), parent);

  rec->flRecordAttr    = 0;
  rec->pszIcon         = (PSZ)rec->Data()->Text.cdata();
  rec->hptrIcon        = CalcIcon(rec);
  return rec;
}

void PlaylistManager::UpdateChildren(RecordBase* const rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateChildren(%s)\n", this, Record::DebugName(rec).cdata()));
  // Do not update children of recursive records
  if (rec && (rec->IsRemoved() || ((Record*)rec)->Data()->Recursive))
    return;

  PlaylistBase::UpdateChildren(rec);
  // Update icon always because the number of items may have changed from or to zero.
  PostRecordCommand(rec, RC_UPDATESTATUS);
}

void PlaylistManager::RequestChildren(RecordBase* const rec)
{ DEBUGLOG(("PlaylistManager(%p)::RequestChildren(%s)\n", this, Record::DebugName(rec).cdata()));
  // Do not request children of recursive records
  if (rec && (rec->IsRemoved() || ((Record*)rec)->Data()->Recursive))
    return;

  PlaylistBase::RequestChildren(rec);
}

void PlaylistManager::UpdateTech(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateTech(%p)\n", this, rec));
  if (Record::IsRemoved(rec))
    return;
  if (rec) // not for root level
  { // techinfo changed => check whether it is currently visible.
    if (rec->flRecordAttr & CRA_CURSORED)
    { // TODO: maybe this should be better up to the calling thread
      EmFocus = &rec->Content->GetPlayable();
      // continue later
      PMRASSERT(WinPostMsg(HwndFrame, UM_UPDATEINFO, MPFROMP(&*rec), 0));
    }
    bool recursive = rec->Content->GetPlayable().GetInfo().tech->recursive && RecursionCheck(rec);
    if (recursive != rec->Data()->Recursive)
    { rec->Data()->Recursive = recursive;
      // Update Icon also
      PostRecordCommand(rec, RC_UPDATESTATUS);
    }
  }
}

void PlaylistManager::UpdatePlayStatus(RecordBase* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdatePlayStatus(%p)\n", this, rec));
  PlaylistBase::UpdatePlayStatus(rec);
  rec = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rec), MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));
  while (rec != NULL && rec != (RecordBase*)-1)
  { UpdatePlayStatus(rec);
    rec = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rec), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
  }
  PMASSERT(rec != (RecordBase*)-1);
}

/*void PlaylistManager::Open(const char* URL)
{
}*/

/*

static MRESULT EXPENTRY
pm_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  POINTL ptlMouse;
  PPMREC focus, zf;
  HWND   hwndMenu;
  PFREC  fr;
  char   buf[256], buf2[256], buf3[256];
  PNOTIFYRECORDEMPHASIS emphasis;

  switch (msg)
  {case WM_CONTROL:
    switch (SHORT2FROMMP(mp1))
    {

     case CN_HELP:
      amp_show_help( IDH_PM );
      return 0;

    } // switch (SHORT2FROMMP(mp1))
    return 0;


   case WM_ERASEBACKGROUND:
    return FALSE;

   case WM_SYSCOMMAND:
    if( SHORT1FROMMP(mp1) == SC_CLOSE )
    { pm_show( FALSE );
      return 0;
    }
    break;

   case WM_WINDOWPOSCHANGED:
    { SWP* pswp = PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW )
        cfg.show_HwndMgr = TRUE;
      if( pswp[0].fl & SWP_HIDE )
         cfg.show_HwndMgr = FALSE;
      break;
    }
  } // switch (msg)

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}
*/


