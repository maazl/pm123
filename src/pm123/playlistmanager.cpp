/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2007-2008 Marcel Mueller
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
#include "dialog.h"
#include "properties.h"
#include "pm123.rc.h"
#include <utilfct.h>
#include "playlistmanager.h"
#include "docking.h"
#include "iniman.h"
#include "playable.h"
#include "plugman.h"

#include <cpp/smartptr.h>
#include <cpp/showaccel.h>

#include <debuglog.h>

/****************************************************************************
*
*  class PlaylistManager
*
****************************************************************************/


PlaylistManager::PlaylistManager(Playable* content, const xstring& alias)
: PlaylistRepository<PlaylistManager>(content, alias, DLG_PM),
  MainMenu(NULLHANDLE),
  RecMenu(NULLHANDLE)
{ DEBUGLOG(("PlaylistManager(%p)::PlaylistManager(%p, %s)\n", this, content, alias.cdata()));
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
  HwndContainer = WinWindowFromID(GetHwnd(), CNR_PM);
  PMASSERT(HwndContainer != NULLHANDLE);
  // Attension!!! Intended side effect: CCS_VERIFYPOINTERS is only set in degug builds
  PMASSERT(WinSetWindowBits(HwndContainer, QWL_STYLE, CCS_VERIFYPOINTERS, CCS_VERIFYPOINTERS));

  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.flWindowAttr = CV_TREE|CV_NAME|CV_MINI|CA_TREELINE|CA_CONTAINERTITLE|CA_TITLELEFT|CA_TITLESEPARATOR|CA_TITLEREADONLY;
  //cnrInfo.hbmExpanded  = NULLHANDLE;
  //cnrInfo.hbmCollapsed = NULLHANDLE;
  cnrInfo.cxTreeIndent = WinQuerySysValue(HWND_DESKTOP, SV_CYMENU) +2;
  //cnrInfo.cyLineSpacing = 0;
  DEBUGLOG(("PlaylistManager::InitDlg: %u\n", cnrInfo.slBitmapOrIcon.cx));
  cnrInfo.pszCnrTitle  = "Emplty list. Right click for menu.";
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_FLWINDOWATTR|CMA_CXTREEINDENT|CMA_CNRTITLE|CMA_LINESPACING)));

  PlaylistBase::InitDlg();
}

MRESULT PlaylistManager::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistManager(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_CONTROL:
    switch (SHORT2FROMMP(mp1))
    {
     case CN_HELP:
      amp_show_help( IDH_PM );
      return 0;

     case WM_ERASEBACKGROUND:
      return FALSE;

     case CN_EMPHASIS:
      { NOTIFYRECORDEMPHASIS* emphasis = (NOTIFYRECORDEMPHASIS*)mp2;
        if (emphasis->fEmphasisMask & CRA_CURSORED)
        { // Update title
          Record* rec = (Record*)emphasis->pRecord;
          if (rec != NULL && (rec->flRecordAttr & CRA_CURSORED))
          { EmFocus = rec->Data()->Content->GetPlayable();
            if (EmFocus->EnsureInfoAsync(Playable::IF_Tech))
              PMRASSERT(WinPostMsg(GetHwnd(), UM_UPDATEINFO, MPFROMP(EmFocus.get()), 0));
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

     /* TODO: normally we have to lock some functions here...
      case CN_BEGINEDIT:
      break; // Continue in base class;
     */

     case CN_ENDEDIT:
      if (DirectEdit) // ignore undo's
      { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
        Record* rec = (Record*)ed->pRecord;
        DEBUGLOG(("PlaylistManager::DlgProc CN_ENDEDIT %p{,%p->%p{%s},%u,} %p %s\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText, rec, DirectEdit.cdata()));
        rec->Data()->Content->SetAlias(DirectEdit.length() ? DirectEdit : xstring());
      }
      break;
    } // switch (SHORT2FROMMP(mp1))
    break;

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
    { if (EmFocus.get() != (Playable*)mp1)
        return 0; // No longer neccessary, because another UM_UPDATEINFO message is in the queue or the record does no longer exists.
      SetInfo(xstring::sprintf("%s, %s", EmFocus->GetURL().getDisplayName().cdata(), EmFocus->GetInfo().tech->info));
      if (EmFocus.get() == (Playable*)mp1) // double check, because EmFocus may change while SetInfo
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

   case UM_UPDATEDEC:
    PlaylistBase::DlgProc(msg, mp1, mp2);
    DecChanged2 = true;
    return 0;

  } // switch (msg)

  return PlaylistBase::DlgProc(msg, mp1, mp2);
}

HWND PlaylistManager::InitContextMenu()
{ DEBUGLOG(("PlaylistManager(%p)::InitContextMenu() - %u\n", this, Source.size()));
  HWND hwndMenu;
  MENUITEM item;
  bool new_menu = false;
  bool* dec_changed;
  if (Source.size() == 0)
  { // Nothing selected => menu for the whole container
    if (MainMenu == NULLHANDLE)
    { MainMenu = WinLoadMenu(HWND_OBJECT, 0, PM_MAIN_MENU);
      PMASSERT(MainMenu != NULLHANDLE);
      mn_make_conditionalcascade(MainMenu, IDM_PL_APPENDALL, IDM_PL_APPFILEALL);
      new_menu = true; // force update below
    }
    hwndMenu = MainMenu;
    mn_enable_item(hwndMenu, IDM_PL_APPENDALL, (Content->GetFlags() & Playable::Mutable) == Playable::Mutable);

    item.id = IDM_PL_APPENDALL;
    dec_changed = &DecChanged;
  } else
  { // Selected object is record => record menu
    ASSERT(Source.size() == 1);
    if (RecMenu == NULLHANDLE)
    { RecMenu = WinLoadMenu(HWND_OBJECT, 0, PM_REC_MENU);
      PMASSERT(RecMenu != NULLHANDLE);
      mn_make_conditionalcascade(RecMenu, IDM_PL_APPEND, IDM_PL_APPFILE);
      new_menu = true; // force update below
    }
    hwndMenu = RecMenu;
    RecordType rt = AnalyzeRecordTypes();
    if (rt == RT_None)
      return NULLHANDLE;

    mn_enable_item(hwndMenu, IDM_PL_NAVIGATE, !((Record*)Source[0])->Data()->Recursive && IsUnderCurrentRoot(Source[0]));
    mn_enable_item(hwndMenu, IDM_PL_DETAILED, rt != RT_Song);
    mn_enable_item(hwndMenu, IDM_PL_TREEVIEW, rt != RT_Song);
    mn_enable_item(hwndMenu, IDM_PL_EDIT,     Source[0]->Data->Content->GetPlayable()->GetInfo().meta_write);
    mn_enable_item(hwndMenu, IDM_PL_REMOVE,   (PlayableFromRec(((CPData*)Source[0]->Data)->Parent)->GetFlags() & Playable::Mutable) == Playable::Mutable );
    mn_enable_item(hwndMenu, IDM_PL_REFRESH,  rt == RT_Song);
    mn_enable_item(hwndMenu, IDM_PL_APPEND,   rt == RT_List);
    mn_enable_item(hwndMenu, IDM_PL_SORT,     rt == RT_List);

    item.id = IDM_PL_APPEND;
    dec_changed = &DecChanged2;
  }
  // item.id contains the ID of the load menu
  // Update accelerators?
  if (*dec_changed || new_menu)
  { *dec_changed = false;
    // Populate context menu with plug-in specific stuff.
    PMRASSERT(WinSendMsg(hwndMenu, MM_QUERYITEM, MPFROM2SHORT(item.id, TRUE), MPFROMP(&item)));
    memset(LoadWizzards+2, 0, sizeof LoadWizzards - 2*sizeof *LoadWizzards ); // You never know...
    dec_append_load_menu(item.hwndSubMenu, item.id + IDM_PL_APPOTHER-IDM_PL_APPEND, 2, LoadWizzards+2, sizeof LoadWizzards/sizeof *LoadWizzards - 2);
    (MenuShowAccel(WinQueryAccelTable(WinQueryAnchorBlock(GetHwnd()), GetHwnd()))).ApplyTo(new_menu ? hwndMenu : item.hwndSubMenu);
  }
  // emphasize record
  DEBUGLOG(("PlaylistManager::InitContextMenu: Menu: %p %p\n", MainMenu, RecMenu));
  return hwndMenu;
}

void PlaylistManager::UpdateAccelTable()
{ DEBUGLOG(("PlaylistManager::UpdateAccelTable()\n"));
  AccelTable = WinLoadAccelTable( WinQueryAnchorBlock( GetHwnd() ), NULLHANDLE, ACL_PLAYLIST );
  PMASSERT(AccelTable != NULLHANDLE);
  memset( LoadWizzards+2, 0, sizeof LoadWizzards - 2*sizeof *LoadWizzards); // You never know...
  dec_append_accel_table( AccelTable, IDM_PL_APPOTHERALL, IDM_PL_APPOTHER-IDM_PL_APPOTHERALL, LoadWizzards+2, sizeof LoadWizzards/sizeof *LoadWizzards - 2);
}

void PlaylistManager::SetInfo(const xstring& text)
{ DEBUGLOG(("PlaylistManager(%p)::SetInfo(%s)\n", this, text.cdata()));
  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.pszCnrTitle = (char*)text.cdata(); // OS/2 API doesn't like const...
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_CNRTITLE)));
  // now free the old title
  Info = text;
}

PlaylistBase::ICP PlaylistManager::GetPlayableType(const RecordBase* rec) const
{ DEBUGLOG(("PlaylistManager::GetPlaylistState(%s)\n", Record::DebugName(rec).cdata()));
  PlaylistBase::ICP ret;
  const Playable* pp = rec->Data->Content->GetPlayable();
  if ((pp->GetFlags() & Playable::Enumerable) == 0)
    ret = ICP_Song;
  else if (pp->GetInfo().tech->num_items == 0)
    ret = ICP_Empty;
  else if (((const Record*)rec)->Data()->Recursive)
    ret = ICP_Recursive;
  else
    ret = (rec->flRecordAttr & CRA_EXPANDED) ? ICP_Open : ICP_Closed;
  DEBUGLOG(("PlaylistManager::GetPlaylistState: %u\n", ret));
  return ret;
}

PlaylistBase::IC PlaylistManager::GetRecordUsage(const RecordBase* rec) const
{ DEBUGLOG(("PlaylistManager::GetRecordUsage(%s)\n", Record::DebugName(rec).cdata()));
  if (rec->Data->Content->GetPlayable()->GetStatus() != STA_Used)
  { DEBUGLOG(("PlaylistManager::GetRecordUsage: unused\n"));
    return IC_Normal;
  }
  // Check wether the current call stack is the same as for the current Record ...
  int_ptr<PlayableSlice> root = Ctrl::GetRoot();
  do
  { if (root && rec->Data->Content->GetPlayable() == root->GetPlayable())
    { // We are a the current root, so the call stack compared equal.
      DEBUGLOG(("PlaylistManager::GetRecordUsage: current root\n"));
      if (!((const Record*)rec)->Data()->Recursive)
        break;
      return IC_Used;
    }
    if (rec->Data->Content->GetStatus() != STA_Used)
    { // The PlayableInstance is not used, so the call stack is not equal.
      DEBUGLOG(("PlaylistManager::GetRecordUsage: instance unused\n"));
      return IC_Used;
    }
    rec = ((const Record*)rec)->Data()->Parent;
  } while (rec != NULL);
  // No more Parent, let's consider this as (partially) equal.
  DEBUGLOG(("PlaylistManager::GetRecordUsage: top level\n"));
  return decoder_playing() ? IC_Play : IC_Active;
}

bool PlaylistManager::RecursionCheck(const Playable* pp, const RecordBase* parent) const
{ DEBUGLOG(("PlaylistManager(%p)::RecursionCheck(%p{%s}, %s)\n", this, pp, pp->GetURL().getShortName().cdata(), Record::DebugName(parent).cdata()));
  if (pp != Content.get())
  { for(;;)
    { if (parent == NULL)
      { DEBUGLOG(("PlaylistManager::RecursionCheck: no rec.\n"));
        return false;
      }
      if (parent->Data->Content->GetPlayable() == pp)
        break;
      parent = ((const Record*)parent)->Data()->Parent;
      DEBUGLOG2(("PlaylistManager::RecursionCheck: recusrion check %s\n", Record::DebugName(parent).cdata()));
    }
    // recursion in playlist tree
  } // else recursion with top level
  DEBUGLOG(("PlaylistManager::RecursionCheck: recursion!\n"));
  return true;
}
bool PlaylistManager::RecursionCheck(const RecordBase* rp) const
{ DEBUGLOG(("PlaylistManager::RecursionCheck(%s)\n", Record::DebugName(rp).cdata()));
  const Playable* pp = rp->Data->Content->GetPlayable();
  if (pp != Content.get())
  { do
    { rp = ((const Record*)rp)->Data()->Parent;
      DEBUGLOG2(("PlaylistManager::RecursionCheck: recusrion check %p\n", rp));
      if (rp == NULL)
        return false;
    } while (rp->Data->Content->GetPlayable() != pp);
    // recursion in playlist tree
  } // else recursion with top level
  DEBUGLOG(("PlaylistManager::RecursionCheck: recursion!\n"));
  return true;
}


PlaylistBase::RecordBase* PlaylistManager::CreateNewRecord(PlayableInstance* obj, RecordBase* parent)
{ DEBUGLOG(("PlaylistManager(%p{%s})::CreateNewRecord(%p{%s}, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable()->GetURL().getShortName().cdata(), parent));
  // Allocate a record in the HwndContainer
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  PMASSERT(rec != NULL);

  rec->UseCount        = 1;
  rec->Data()          = new CPData(obj, *this, &PlaylistManager::InfoChangeEvent, &PlaylistManager::StatChangeEvent, rec, (Record*)parent);
  // before we catch any information setup the update events
  // The record is not yet corretly initialized, but this don't metter since all that the event handlers can do
  // is to post a UM_RECORDCOMMAND which is not handled unless this message is completed.
  obj->GetPlayable()->InfoChange += rec->Data()->InfoChange;
  obj->StatusChange              += rec->Data()->StatChange;
  // now get initial info's
  rec->Data()->Text    = obj->GetDisplayName();
  Playable* pp = obj->GetPlayable();
  rec->Data()->Recursive = (pp->GetFlags() & Playable::Enumerable)
                        && (!pp->CheckInfo(Playable::IF_Tech) || pp->GetInfo().tech->recursive)
                        && RecursionCheck(pp, parent);
  rec->flRecordAttr    = 0;
  rec->pszIcon         = (PSZ)rec->Data()->Text.cdata();
  rec->hptrIcon        = CalcIcon(rec);
  return rec;
}

PlaylistManager::RecordBase* PlaylistManager::GetParent(const RecordBase* const rec) const
{ return ((const Record*)rec)->Data()->Parent;
}

void PlaylistManager::UpdateChildren(RecordBase* const rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateChildren(%s)\n", this, Record::DebugName(rec).cdata()));
  // Do not update children of recursive records
  if (rec && ((Record*)rec)->Data()->Recursive)
    return;

  PlaylistBase::UpdateChildren(rec);
  // Update icon always because the number of items may have changed from or to zero.
  PostRecordCommand(rec, RC_UPDATESTATUS);
}

void PlaylistManager::RequestChildren(RecordBase* const rec)
{ DEBUGLOG(("PlaylistManager(%p)::RequestChildren(%s)\n", this, Record::DebugName(rec).cdata()));
  // Do not request children of recursive records
  if (rec && ((Record*)rec)->Data()->Recursive)
    return;

  PlaylistBase::RequestChildren(rec);
}

void PlaylistManager::UpdateTech(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateTech(%p)\n", this, rec));
  if (rec) // not for root level
  { // techinfo changed => check whether it is currently visible.
    if (rec->flRecordAttr & CRA_CURSORED)
    { // TODO: maybe this should be better up to the calling thread
      EmFocus = rec->Data()->Content->GetPlayable();
      // continue later
      PMRASSERT(WinPostMsg(GetHwnd(), UM_UPDATEINFO, MPFROMP(rec), 0));
    }
    bool recursive = rec->Data()->Content->GetPlayable()->GetInfo().tech->recursive && RecursionCheck(rec);
    if (recursive != rec->Data()->Recursive)
    { rec->Data()->Recursive = recursive;
      // Update Icon also
      PostRecordCommand(rec, RC_UPDATESTATUS);
      // Remove children?
      if (recursive)
        RemoveChildren(rec);
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

void PlaylistManager::UserNavigate(const RecordBase* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UserNavigate(%p)\n", this, rec));
  if (((Record*)rec)->Data()->Recursive) // do not navigate to recursive items
    return;
  const Playable* pp;
  // Fetch current root
  { int_ptr<PlayableSlice> ps = Ctrl::GetRoot();
    if (ps == NULL)
      return;
    pp = ps->GetPlayable();
  }
  // make navigation string starting from rec
  xstring nav = xstring::empty;
  while (rec)
  { PlayableInstance* pi = rec->Data->Content;
    // Find parent
    rec = GetParent(rec);
    PlayableCollection* list = (PlayableCollection*)PlayableFromRec(rec);
    // TODO: Configuration options
    nav = list->SerializeItem(pi, PlayableCollection::SerialRelativePath) + ";" + nav;
    DEBUGLOG(("PlaylistManager::UserNavigate - %s\n", nav.cdata()));
    if (list == pp)
    { // now we are at the root => send navigation command
      Ctrl::PostCommand(Ctrl::MkNavigate(nav, 0, false, true));
      return;
    }
  }
  // We did not reach the root => cannot navigate because callstack is incomplete.
  // This may happen if the current root has recently changed.
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
