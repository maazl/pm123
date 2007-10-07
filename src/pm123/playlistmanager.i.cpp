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
#include "playlistmanager.i.h"
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
: PlaylistRepository<PlaylistManager>(url, alias),
  MainMenu(NULLHANDLE),
  ListMenu(NULLHANDLE),
  FileMenu(NULLHANDLE)
{ DEBUGLOG(("PlaylistManager(%p)::PlaylistManager(%s, %s)\n", this, url, alias));
  init_dlg_struct ids = { sizeof(init_dlg_struct), this };
  WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, pl_DlgProcStub, NULLHANDLE, DLG_PM, &ids);
}

PlaylistManager::~PlaylistManager()
{ DEBUGLOG(("PlaylistManager::~PlaylistManager() - %p\n", HwndFrame));
  save_window_pos(HwndFrame, 0);
  WinDestroyWindow(MainMenu);
  WinDestroyWindow(ListMenu);
  WinDestroyWindow(FileMenu);
  WinDestroyWindow(HwndFrame);
  DEBUGLOG(("PlaylistManager::~PlaylistManager() done - %p\n", WinGetLastError(NULL)));
}

void PlaylistManager::PostRecordCommand(RecordBase* rec, RecordCommand cmd)
{ // Ignore some messages
  switch (cmd)
  {case RC_UPDATETECH:
    if (rec)
      break;
   case RC_UPDATEFORMAT:
   case RC_UPDATEMETA:
   case RC_UPDATEPOS:
    return;
  }
  PlaylistBase::PostRecordCommand(rec, cmd);
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
  if (!HwndContainer)
  { DEBUGLOG(("PlaylistManager::InitDlg: failed to create HwndContainer, error = %lx\n", WinGetLastError(NULL)));
    return;
  }

  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.flWindowAttr = CV_TREE|CV_NAME|CV_MINI|CA_TREELINE|CA_CONTAINERTITLE|CA_TITLELEFT|CA_TITLESEPARATOR|CA_TITLEREADONLY;
  //cnrInfo.hbmExpanded  = NULLHANDLE;
  //cnrInfo.hbmCollapsed = NULLHANDLE;
  cnrInfo.cxTreeIndent = WinQuerySysValue(HWND_DESKTOP, SV_CYMENU) +2;
  //cnrInfo.cyLineSpacing = 0;
  DEBUGLOG(("PlaylistManager::InitDlg: %u\n", cnrInfo.slBitmapOrIcon.cx));
  cnrInfo.pszCnrTitle  = "No playlist selected. Right click for menu.";
  WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_FLWINDOWATTR|CMA_CXTREEINDENT|CMA_CNRTITLE|CMA_LINESPACING));

  PlaylistBase::InitDlg();

  // populate the root node
  RequestChildren(NULL);
}

MRESULT PlaylistManager::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistManager(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_DESTROY:
    // delete all records
    { DEBUGLOG(("PlaylistManager::DlgProc: WM_DESTROY\n"));
      RemoveChildren(NULL);
      // Continue in base class
      break;
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
          Record* focus = (Record*)emphasis->pRecord;
          if (focus != NULL && !focus->IsRemoved() && (focus->flRecordAttr & CRA_CURSORED))
          { EmFocus = &focus->Content->GetPlayable();
            if (EmFocus->EnsureInfoAsync(Playable::IF_Tech))
              WinPostMsg(HwndFrame, UM_UPDATEINFO, MPFROMP(&*EmFocus), 0);
          }
        }
      }
      return 0;
     
     case CN_EXPANDTREE:
      { Record* focus = (Record*)PVOIDFROMMP(mp2);
        DEBUGLOG(("PlaylistManager::DlgProc CN_EXPANDTREE %p\n", focus));
        PostRecordCommand(focus, RC_UPDATESTATUS);
        // iterate over children
        focus = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(focus), MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));
        while (focus != NULL && focus != (Record*)-1)
        { DEBUGLOG(("CM_QUERYRECORD: %s\n", Record::DebugName(focus).cdata()));
          RequestChildren(focus);
          focus = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(focus), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
        }
      }  
      return 0;

     case CN_COLLAPSETREE:
      PostRecordCommand((Record*)PVOIDFROMMP(mp2), RC_UPDATESTATUS);
      return 0;

     case CN_CONTEXTMENU:
      { Record* focus = (Record*)mp2; // save focus for later processing of menu messages
        CmFocus = focus;
        if (Record::IsRemoved(focus))
          return 0; // record removed
        if (focus)
          WinPostMsg(HwndContainer, CM_SETRECORDEMPHASIS, MPFROMP(focus), MPFROM2SHORT(TRUE, CRA_CURSORED)); 

        POINTL ptlMouse;
        WinQueryPointerPos(HWND_DESKTOP, &ptlMouse);
        // TODO: Mouse Position may not be reasonable, when the menu is invoked by keyboard.

        HWND   hwndMenu;
        if (focus == NULL)
        { if (MainMenu == NULLHANDLE)
          { MainMenu = WinLoadMenu(HWND_OBJECT, 0, PM_MAIN_MENU);
            mn_make_conditionalcascade(MainMenu, IDM_PL_APPEND, IDM_PL_APPFILE);
            mn_make_conditionalcascade(MainMenu, IDM_PL_OPEN,   IDM_PL_OPENL);
            mn_make_conditionalcascade(MainMenu, IDM_PL_SAVE,   IDM_PL_LST_SAVE);
          }
          hwndMenu = MainMenu;
          mn_enable_item( hwndMenu, IDM_PL_APPEND, (Content->GetFlags() & Playable::Mutable) == Playable::Mutable );
        } else if (focus->Content->GetPlayable().GetFlags() & Playable::Enumerable)
        { if (ListMenu == NULLHANDLE)
          { ListMenu = WinLoadMenu(HWND_OBJECT, 0, PM_LIST_MENU);
            mn_make_conditionalcascade(ListMenu, IDM_PL_APPEND, IDM_PL_APPFILE);
          }
          if (Record::IsRemoved(focus))
            return 0; // record could be removed at WinLoadMenu
          hwndMenu = ListMenu;
          mn_enable_item( hwndMenu, IDM_PL_APPEND, (focus->Content->GetPlayable().GetFlags() & Playable::Mutable) == Playable::Mutable );
        } else
        { if (FileMenu == NULLHANDLE)
          { FileMenu = WinLoadMenu(HWND_OBJECT, 0, PM_FILE_MENU);
          }
          hwndMenu = FileMenu;
          mn_enable_item(hwndMenu, IDM_PL_EDIT, focus->Content->GetPlayable().GetInfo().meta_write);
        }
        if (Record::IsRemoved(focus))
          return 0; // record could be removed now
        DEBUGLOG2(("PlaylistManager::DlgProc: Menu: %p %p %p\n", MainMenu, ListMenu, FileMenu));
        WinPopupMenu(HWND_DESKTOP, HwndFrame, hwndMenu, ptlMouse.x, ptlMouse.y, 0,
                     PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
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
      DEBUGLOG(("PlaylistManager::DlgProc: UM_RECORDCOMMAND: %s\n", Record::DebugName(rec).cdata()));
      // reset pending message flag
      Interlocked il(StateFromRec(rec).PostMsg);
      if (il.bitrst(RC_UPDATECHILDREN))
        UpdateChildren(rec);
      if (il.bitrst(RC_UPDATETECH))
        UpdateTech(rec);
      if (il.bitrst(RC_UPDATESTATUS))
        UpdateStatus(rec);
      if (il.bitrst(RC_UPDATEALIAS))
        UpdateInstance(rec, PlayableInstance::SF_Alias);
      // continue in base class
      break;
    }
    
  } // switch (msg)

  return PlaylistBase::DlgProc(msg, mp1, mp2);
}


void PlaylistManager::SetInfo(const xstring& text)
{ DEBUGLOG(("PlaylistManager(%p)::SetInfo(%s)\n", this, text.cdata()));
  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.pszCnrTitle = (char*)text.cdata(); // OS/2 API doesn't like const...
  if (WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_CNRTITLE)))
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

bool PlaylistManager::RecursionCheck(Playable* pp, Record* parent)
{ DEBUGLOG(("PlaylistManager(%p)::RecursionCheck(%p{%s}, %s)\n", this, pp, pp->GetURL().getObjName().cdata(), Record::DebugName(parent).cdata()));
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
      DEBUGLOG(("PlaylistManager::RecursionCheck: recusrion check %s\n", Record::DebugName(parent).cdata()));
    }
    // recursion in playlist tree
  } // else recursion with top level
  DEBUGLOG(("PlaylistManager::RecursionCheck: recursion!\n"));
  return true;
}
bool PlaylistManager::RecursionCheck(Record* rp)
{ DEBUGLOG(("PlaylistManager::RecursionCheck(%s)\n", Record::DebugName(rp).cdata()));
  Playable* pp = &rp->Content->GetPlayable();
  if (pp != &*Content)
  { do
    { rp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(CMA_PARENT, CMA_ITEMORDER));
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

PlaylistManager::Record* PlaylistManager::AddEntry(PlayableInstance* obj, Record* parent, Record* after)
{ DEBUGLOG(("PlaylistManager(%p{%s})::AddEntry(%p{%s}, %p, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable().GetURL().getObjName().cdata(), parent, after));
  /* Allocate a record in the HwndContainer */
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  if (rec == NULL)
  { DEBUGLOG(("PlaylistManager::AddEntry: CM_ALLOCRECORD failed, error %lx\n", WinGetLastError(NULL)));
    DosBeep(500, 100);
    return NULL;
  }
  if (Record::IsRemoved(parent))
    return NULL; // Parent removed

  rec->Content         = obj;
  rec->UseCount        = 1;
  rec->Data()          = new CPData(*this, &InfoChangeEvent, &StatChangeEvent, rec, parent);
  rec->Data()->Text    = obj->GetDisplayName();
  rec->Data()->Recursive = obj->GetPlayable().GetInfo().tech->recursive && RecursionCheck(&obj->GetPlayable(), parent);
          
  rec->flRecordAttr    = 0;
  rec->pszIcon         = (PSZ)rec->Data()->Text.cdata();
  rec->hptrIcon        = CalcIcon(rec);

  RECORDINSERT insert      = { sizeof(RECORDINSERT) };
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
  ULONG rc = LONGFROMMR(WinSendMsg(HwndContainer, CM_INSERTRECORD, MPFROMP(rec), MPFROMP(&insert)));
  
  obj->GetPlayable().InfoChange += rec->Data()->InfoChange;
  obj->StatusChange             += rec->Data()->StatChange;

  DEBUGLOG(("PlaylistManager::AddEntry: succeeded: %p %u %x\n", rec, rc, WinGetLastError(NULL)));
  return rec;
}

PlaylistManager::Record* PlaylistManager::MoveEntry(Record* entry, Record* parent, Record* after)
{ DEBUGLOG(("PlaylistManager(%p{%s})::MoveEntry(%s, %s, %s)\n", this, DebugName().cdata(),
    Record::DebugName(entry).cdata(), Record::DebugName(parent).cdata(), Record::DebugName(after).cdata()));
  TREEMOVE move =
  { (RECORDCORE*)entry,
    (RECORDCORE*)parent,
    (RECORDCORE*)(after ? after : (Record*)CMA_FIRST),
    FALSE
  };
  if (WinSendMsg(HwndContainer, CM_MOVETREE, MPFROMP(&move), 0) == FALSE)
    return NULL;
  return entry;
}

void PlaylistManager::RemoveEntry(Record* const entry)
{ DEBUGLOG(("PlaylistManager(%p{%s})::RemoveEntry(%p)\n", this, DebugName().cdata(), entry));
  if (CmFocus == entry) // TODO: no good solution
    CmFocus = NULL;
  // detach content
  entry->Content = NULL;
  // deregister events
  entry->Data()->DeregisterEvents();
  // Delete the children
  RemoveChildren(entry);
  // Remove record from container
  assert(LONGFROMMR(WinSendMsg(HwndContainer, CM_REMOVERECORD, MPFROMP(&entry), MPFROM2SHORT(1, CMA_INVALIDATE))) != -1);
  // Release reference counter
  // The record will be deleted when no outstanding PostMsg is on the way.  
  FreeRecord(entry);
  DEBUGLOG(("PlaylistManager::RemoveEntry completed\n"));
}

int PlaylistManager::RemoveChildren(Record* const rp)
{ DEBUGLOG(("PlaylistManager(%p)::DeleteChildren(%p)\n", this, rp));
  Record* crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(rp ? CMA_FIRSTCHILD : CMA_FIRST, CMA_ITEMORDER));
  int count = 0;
  while (crp != NULL && crp != (Record*)-1)
  { DEBUGLOG(("CM_QUERYRECORD: %s\n", Record::DebugName(crp).cdata()));
    Record* crp2 = crp;
    crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    RemoveEntry(crp2);
    ++count;
  }
  return count;
}

void PlaylistManager::UpdateChildren(Record* const rp)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateChildren(%s)\n", this, Record::DebugName(rp).cdata()));
  // get content
  Playable* pp;
  if (rp == NULL)
    pp = Content;
  else if (rp->IsRemoved() || rp->Data()->Recursive)
    return; // record removed or recursive
  else
    pp = &rp->Content->GetPlayable();
  DEBUGLOG(("PlaylistManager::UpdateChildren - %u\n", pp->GetStatus()));
  if ((pp->GetFlags() & Playable::Enumerable) == 0 || pp->GetStatus() <= STA_Invalid)
  { // Nothing to enumerate, delete children if any
    DEBUGLOG(("PlaylistManager::UpdateChildren - no children possible.\n"));
    if (RemoveChildren(rp))
      PostRecordCommand(rp, RC_UPDATESTATUS); // update icon
    return;
  }
  
  // Check if techinfo is available, otherwise wait
  if (pp->EnsureInfoAsync(Playable::IF_Tech) == 0)
  { StateFromRec(rp).WaitUpdate = true;
    return;
  }

  // First check what's currently in the container.
  Record* OldHead = NULL;
  Record* OldTail = NULL;
  int status = 0;
  { // The collection is not locked so far, but the Delete Event ensures that the parent node is removed
    // from the worker's queue before the children are deleted. So we are safe here.
    Record* crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(rp ? CMA_FIRSTCHILD : CMA_FIRST, CMA_ITEMORDER));
    while (crp != NULL && crp != (Record*)-1)
    { DEBUGLOG(("PlaylistManager::UpdateChildren CM_QUERYRECORD: %s\n", Record::DebugName(crp).cdata()));
      if (OldTail)
        OldTail->preccNextRecord = crp;
       else
        OldHead = crp;
      OldTail = crp;
      crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    }
    if (OldTail)
    { status |= 1; // old exists
      OldTail->preccNextRecord = NULL;
    }
   
    // Now check what should be in the container
    DEBUGLOG(("PlaylistManager::UpdateChildren - check container.\n"));
    Mutex::Lock lock(pp->Mtx); // Lock the collection
    sco_ptr<PlayableEnumerator> ep = ((PlayableCollection*)pp)->GetEnumerator();
    crp = NULL;
    while (ep->Next())
    { status |= 2; // new exists
      // Find entry in the current content
      Record** nrpp = &OldHead;
      for (;;)
      { if (*nrpp == NULL)
        { // not found! => add
          DEBUGLOG(("PlaylistManager::UpdateChildren - not found: %p{%s}\n", &**ep, (*ep)->GetPlayable().GetURL().getObjName().cdata()));
          crp = AddEntry(&**ep, rp, crp);
          if (crp && (rp == NULL || ((rp->flRecordAttr & CRA_EXPANDED) && !crp->Data()->Recursive)))
            RequestChildren(crp);
          break;
        }
        if ((*nrpp)->Content == &**ep)
        { // found! => move
          if (nrpp == &OldHead)
          { // already in right order
            DEBUGLOG(("PlaylistManager::UpdateChildren - found: %p{%s} at HEAD\n", &**ep, (*ep)->GetPlayable().GetURL().getObjName().cdata()));
            crp = OldHead;
          } else
          { // move
            DEBUGLOG(("PlaylistManager::UpdateChildren - found: %p{%s} at %p\n", &**ep, (*ep)->GetPlayable().GetURL().getObjName().cdata(), *nrpp));
            crp = MoveEntry(*nrpp, rp, crp);
          }
          // Remove from queue
          *nrpp = (Record*)(*nrpp)->preccNextRecord;
          break;
        }
        nrpp = &(Record*&)(*nrpp)->preccNextRecord; 
      }
    }
  }
  // Icon update required? (Number of items changed from or to zero.)
  if (((status>>1) ^ status) & 1)
    PostRecordCommand(rp, RC_UPDATESTATUS);

  // Well normally the queue should be empty now...
  DEBUGLOG(("PlaylistManager::UpdateChildren - OldHead = %p\n", OldHead));
  while (OldHead) // if not: delete!
  { RemoveEntry(OldHead); // The record is deleted later, so we cann access OldHead safely.
    OldHead = (Record*)OldHead->preccNextRecord;
  }
}

void PlaylistManager::RequestChildren(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::RequestChildren(%s)\n", this, Record::DebugName(rec).cdata()));
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

void PlaylistManager::UpdateTech(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateTech(%p)\n", this, rec));
  if (Record::IsRemoved(rec))
    return;
  // techinfo changed => check whether it is currently visible.
  if (rec->flRecordAttr & CRA_CURSORED)
  { // TODO: maybe this should be better up to the calling thread
    EmFocus = &rec->Content->GetPlayable();
    // continue later, because I/O may be necessary
    WinPostMsg(HwndFrame, UM_UPDATEINFO, MPFROMP(&*rec), 0);
  }
  bool recursive = rec->Content->GetPlayable().GetInfo().tech->recursive && RecursionCheck(rec);
  if (recursive != rec->Data()->Recursive)
  { rec->Data()->Recursive = recursive;
    // Update Icon also 
    PostRecordCommand(rec, RC_UPDATESTATUS);
  }
  // Check if UpdateChildren is waiting
  bool& wait = StateFromRec(rec).WaitUpdate;
  if (wait)
  { // Try again
    wait = false;
    PostRecordCommand(rec, RC_UPDATECHILDREN);
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
}

void PlaylistManager::Open(const char* URL)
{ 
}

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


