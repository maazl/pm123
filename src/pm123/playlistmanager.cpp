/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2007-2009 Marcel Mueller
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

#include "playlistmanager.h"
#include "playlistmenu.h"
#include "gui.h"
#include "dialog.h"
#include "configuration.h"
#include "pm123.rc.h"
#include "docking.h"
#include "playable.h"
#include "decoder.h"

#include <utilfct.h>
#include <cpp/smartptr.h>
#include <cpp/showaccel.h>
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debuglog.h>

/* class PlaylistManager ***************************************************/

HWND PlaylistManager::MainMenu = NULLHANDLE;
HWND PlaylistManager::RecMenu  = NULLHANDLE;


PlaylistManager::PlaylistManager(Playable& content)
: PlaylistBase(content, DLG_PM)
{ DEBUGLOG(("PlaylistManager(%p)::PlaylistManager(&%p)\n", this, &content));
  StartDialog();
}

PlaylistManager::~PlaylistManager()
{ RepositoryType::RemoveWithKey(*this, *Content);
}

PlaylistManager* PlaylistManager::Factory(Playable& key)
{ return new PlaylistManager(key);
}

int PlaylistManager::Comparer(const Playable& key, const PlaylistManager& pl)
{ return CompareInstance(key, *pl.Content);
}

const int_ptr<PlaylistBase> PlaylistManager::GetSame(Playable& obj)
{ return &*GetByKey(obj);
}

void PlaylistManager::DestroyAll()
{ RepositoryType::IXAccess index;
  DEBUGLOG(("PlaylistManager::DestroyAll() - %d\n", index->size()));
  // The instances deregister itself from the repository.
  // Starting at the end avoids the memcpy calls for shrinking the vector.
  while (index->size())
    (*index)[index->size()-1]->Destroy();
}


InfoFlags PlaylistManager::FilterRecordRequest(RecordBase* const rec, InfoFlags& filter)
{ //DEBUGLOG(("PlaylistManager(%p)::FilterRecordRequest(%s, %x)\n", this, Record::DebugName(rec).cdata(), filter));
  if (rec)
  { filter &= ((Record*)rec)->Data()->Recursive // Do not request children of recursive records
      ? IF_Tech|IF_Rpl|IF_Display|IF_Usage
      : IF_Tech|IF_Rpl|IF_Display|IF_Usage|IF_Child;
  } else
    filter &= IF_Phys|IF_Tech|IF_Display|IF_Usage|IF_Child;
  return filter & ~IF_Rpl;
}

void PlaylistManager::InitDlg()
{ DEBUGLOG(("PlaylistManager(%p{%s})::InitDlg()\n", this, DebugName().cdata()));
  HwndContainer = WinWindowFromID(GetHwnd(), CO_CONTENT);
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
  cnrInfo.pszCnrTitle  = "Empty list. Right click for menu.";
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_FLWINDOWATTR|CMA_CXTREEINDENT|CMA_CNRTITLE|CMA_LINESPACING)));

  PlaylistBase::InitDlg();
}

MRESULT PlaylistManager::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistManager(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_CONTROL:
    if (SHORT1FROMMP(mp1) == CO_CONTENT)
    { switch (SHORT2FROMMP(mp1))
      {
       case CN_HELP:
        GUI::ShowHelp(IDH_PM);
        return 0;

       case WM_ERASEBACKGROUND:
        return FALSE;

       case CN_EMPHASIS:
        { NOTIFYRECORDEMPHASIS* emphasis = (NOTIFYRECORDEMPHASIS*)mp2;
          if (emphasis->fEmphasisMask & CRA_CURSORED)
          { // Update title
            Record* rec = (Record*)emphasis->pRecord;
            if (rec && (rec->flRecordAttr & CRA_CURSORED))
            { EmFocus = &rec->Data()->Content->GetPlayable();
              if (!EmFocus->RequestInfo(IF_Tech, PRI_Low))
                PMRASSERT(WinPostMsg(GetHwnd(), UM_UPDATEINFO, MPFROMP(EmFocus.get()), 0));
            }
          }
        }
        return 0;

       case CN_EXPANDTREE:
        { Record* rec = (Record*)PVOIDFROMMP(mp2);
          DEBUGLOG(("PlaylistManager::DlgProc CN_EXPANDTREE %p\n", rec));
          PlaylistBase::PostRecordUpdate(rec, IF_Usage);
          // iterate over children
          rec = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rec), MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));
          while (rec != NULL && rec != (Record*)-1)
          { DEBUGLOG(("CM_QUERYRECORD: %s\n", Record::DebugName(rec).cdata()));
            PlaylistBase::PostRecordUpdate(rec, RequestRecordInfo(rec, IF_Child));
            rec = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rec), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
          }
          PMASSERT(rec != (Record*)-1);
        }
        return 0;

       case CN_COLLAPSETREE:
        PostRecordUpdate((Record*)PVOIDFROMMP(mp2), IF_Usage);
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
          PlayableInstance& pi = *rec->Data()->Content;
          Mutex::Lock lock(pi.GetPlayable().Mtx);
          ItemInfo info = *pi.GetInfo().item;
          if (DirectEdit.length())
            info.alias = DirectEdit;
          else
            info.alias.reset();
          pi.OverrideItem(&info);
        }
      } // switch (SHORT2FROMMP(mp1))
    }
    break;

   /*case WM_WINDOWPOSCHANGED:
    // TODO: nonsense?
    { bool show = Cfg::Get().show_plman;
      SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
      if( pswp[0].fl & SWP_SHOW )
        show = TRUE;
      if( pswp[0].fl & SWP_HIDE )
        show = FALSE;
      if (show != Cfg::Get().show_plman)
      { Cfg::ChangeAccess cfg;
        cfg.show_plman = show;
        Cfg::Set(cfg);
      }
      break;
    }*/

   case UM_UPDATEINFO:
    { if (EmFocus.get() != (Playable*)mp1)
        return 0; // No longer neccessary, because another UM_UPDATEINFO message is in the queue or the record does no longer exists.
      SetInfo(xstring().sprintf("%s, %s", EmFocus->URL.getDisplayName().cdata(), xstring(EmFocus->GetInfo().tech->info).cdata()));
      if (EmFocus.get() == (Playable*)mp1) // double check, because EmFocus may change while SetInfo
        EmFocus = NULL;
      return 0;
    }

   /*case UM_RECORDCOMMAND:
    { Record* rec = (Record*)PVOIDFROMMP(mp1);
      DEBUGLOG(("PlaylistManager::DlgProc: UM_RECORDCOMMAND: %s %x\n", Record::DebugName(rec).cdata(), StateFromRec(rec).PostMsg));
      AtomicUnsigned il(StateFromRec(rec).PostMsg);
      do
      { // We do the processing here step by step because the processing may set some of the bits
        // that are handled later. This avoids double actions and reduces the number of posted messages.
        if (il.bitrst(RC_UPDATERPL))
          UpdateRpl(rec); // may set RC_UPDATEUSAGE
        if (il.bitrst(RC_UPDATETECH))
          UpdateTech(rec);
        if (il.bitrst(RC_UPDATEOTHER))
        { if (PlayableFromRec(rec)->GetFlags() & Playable::Enumerable)
            UpdateChildren(rec); // may set RC_UPDATEUSAGE
          if (rec != NULL)
            il.bitset(RC_UPDATEUSAGE);
        }
        if (il.bitrst(RC_UPDATEUSAGE) | il.bitrst(RC_UPDATEPHYS)) // RC_UPDATEPHYS covers changes of num_items
          UpdateIcon(rec);
        if (il.bitrst(RC_UPDATEALIAS) | il.bitrst(RC_UPDATEMETA)) // Changing of meta data may reflect to the display name too.
          UpdateInstance(rec, PlayableInstance::SF_Alias);
        // It is essential that all messages are handled here. Otherwise: infinite loop.
        ASSERT((il & ~(1<<RC_UPDATERPL|1<<RC_UPDATEOTHER|1<<RC_UPDATETECH|1<<RC_UPDATEUSAGE|1<<RC_UPDATEPHYS|1<<RC_UPDATEALIAS|1<<RC_UPDATEMETA)) == 0);
      } while (il);
      break; // continue in base class
    }*/

   case UM_UPDATEDEC:
    PlaylistBase::DlgProc(msg, mp1, mp2);
    DecChanged2 = true;
    return 0;

  } // switch (msg)

  return PlaylistBase::DlgProc(msg, mp1, mp2);
}

void PlaylistManager::InitContextMenu()
{ DEBUGLOG(("PlaylistManager(%p)::InitContextMenu() - %u\n", this, Source.size()));
  MENUITEM item;
  bool new_menu = false;
  bool* dec_changed;
  bool is_pl = true;
  if (Source.size() == 0)
  { // Nothing selected => menu for the whole container
    if (MainMenu == NULLHANDLE)
    { MainMenu = WinLoadMenu(HWND_OBJECT, 0, PM_MAIN_MENU);
      PMASSERT(MainMenu != NULLHANDLE);
      PMRASSERT(mn_make_conditionalcascade(MainMenu, IDM_PL_APPENDALL, IDM_PL_APPFILEALL));
      new_menu = true; // force update below
    }
    HwndMenu = MainMenu;
    GetMenuWorker().AttachMenu(HwndMenu, IDM_PL_CONTENT, *Content, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Enumerate|PlaylistMenu::Recursive, 0);

    item.id = IDM_PL_APPENDALL;
    dec_changed = &DecChanged;
  } else
  { // Selected object is record => record menu
    ASSERT(Source.size() == 1);
    if (RecMenu == NULLHANDLE)
    { RecMenu = WinLoadMenu(HWND_OBJECT, 0, PM_REC_MENU);
      PMASSERT(RecMenu != NULLHANDLE);
      PMRASSERT(mn_make_conditionalcascade(RecMenu, IDM_PL_FLATTEN, IDM_PL_FLATTEN_1));
      PMRASSERT(mn_make_conditionalcascade(RecMenu, IDM_PL_APPEND, IDM_PL_APPFILE));
      new_menu = true; // force update below
    }
    RecordType rt = AnalyzeRecordTypes();
    if (rt == RT_None)
      return;
    HwndMenu = RecMenu;

    mn_enable_item(HwndMenu, IDM_PL_NAVIGATE, !((Record*)Source[0])->Data()->Recursive && IsUnderCurrentRoot(Source[0]));
    mn_enable_item(HwndMenu, IDM_PL_DETAILED, rt & (RT_Enum|RT_List));
    mn_enable_item(HwndMenu, IDM_PL_TREEVIEW, rt & (RT_Enum|RT_List));
    mn_enable_item(HwndMenu, IDM_PL_EDIT,     rt == RT_Meta);
    mn_enable_item(HwndMenu, IDM_PL_FLATTEN,  rt & (RT_Enum|RT_List));
    //mn_enable_item(hwndMenu, IDM_PL_REFRESH,  rt & (RT_Song|RT_Meta));
    mn_enable_item(HwndMenu, IDM_PL_APPEND,   rt & (RT_Enum|RT_List));
    mn_enable_item(HwndMenu, IDM_PL_SORT,     rt & (RT_Enum|RT_List));
    is_pl = Source.size() == 1 && (rt & (RT_Enum|RT_List));
    mn_enable_item(HwndMenu, IDM_PL_CONTENT,  is_pl);
    if (is_pl)
      GetMenuWorker().AttachMenu(HwndMenu, IDM_PL_CONTENT, *Source[0]->Data->Content, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Enumerate|PlaylistMenu::Recursive, 0);

    item.id = IDM_PL_APPEND;
    dec_changed = &DecChanged2;
  }
  // item.id contains the ID of the load menu
  // Update accelerators?
  if (*dec_changed || new_menu)
  { *dec_changed = false;
    // Populate context menu with plug-in specific stuff.
    PMRASSERT(WinSendMsg(HwndMenu, MM_QUERYITEM, MPFROM2SHORT(item.id, TRUE), MPFROMP(&item)));
    memset(LoadWizards+2, 0, sizeof LoadWizards - 2*sizeof *LoadWizards ); // You never know...
    Decoder::AppendLoadMenu(item.hwndSubMenu, item.id + IDM_PL_APPOTHER-IDM_PL_APPEND, 2, LoadWizards+2, sizeof LoadWizards/sizeof *LoadWizards - 2);
    (MenuShowAccel(WinQueryAccelTable(WinQueryAnchorBlock(GetHwnd()), GetHwnd()))).ApplyTo(new_menu ? HwndMenu : item.hwndSubMenu);
  }
  DEBUGLOG(("PlaylistManager::InitContextMenu: Menu: %p %p\n", MainMenu, RecMenu));
}

void PlaylistManager::UpdateAccelTable()
{ DEBUGLOG(("PlaylistManager::UpdateAccelTable()\n"));
  AccelTable = WinLoadAccelTable( WinQueryAnchorBlock( GetHwnd() ), NULLHANDLE, ACL_PLAYLIST );
  PMASSERT(AccelTable != NULLHANDLE);
  memset( LoadWizards+2, 0, sizeof LoadWizards - 2*sizeof *LoadWizards); // You never know...
  Decoder::AppendAccelTable( AccelTable, IDM_PL_APPOTHERALL, IDM_PL_APPOTHER-IDM_PL_APPOTHERALL, LoadWizards+2, sizeof LoadWizards/sizeof *LoadWizards - 2);
}

void PlaylistManager::SetInfo(const xstring& text)
{ DEBUGLOG(("PlaylistManager(%p)::SetInfo(%s)\n", this, text.cdata()));
  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.pszCnrTitle = (PSZ)text.cdata(); // OS/2 API doesn't like const...
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_CNRTITLE)));
  Info = text; // Keep text alive
}

PlaylistBase::ICP PlaylistManager::GetPlaylistType(const RecordBase* rec) const
{ DEBUGLOG(("PlaylistManager::GetPlaylistType(%s)\n", Record::DebugName(rec).cdata()));
  PlaylistBase::ICP ret;
  Playable& pp = rec->Data->Content->GetPlayable();
  if (pp.RequestInfo(IF_Child, PRI_None, REL_Invalid) || pp.GetNext(NULL) == NULL)
    ret = ICP_Empty;
  else if (((const Record*)rec)->Data()->Recursive)
    ret = ICP_Recursive;
  else
    ret = (rec->flRecordAttr & CRA_EXPANDED) ? ICP_Open : ICP_Closed;
  DEBUGLOG(("PlaylistManager::GetPlaylistType: %u\n", ret));
  return ret;
}

bool PlaylistManager::RecursionCheck(const Playable& pp, const RecordBase* parent) const
{ DEBUGLOG(("PlaylistManager(%p)::RecursionCheck(&%p{%s}, %s)\n", this, &pp, pp.DebugName().cdata(), Record::DebugName(parent).cdata()));
  if (pp != *Content)
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
  const Playable& pp = rp->Data->Content->GetPlayable();
  if (pp != *Content)
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


PlaylistBase::RecordBase* PlaylistManager::CreateNewRecord(PlayableInstance& obj, RecordBase* parent)
{ DEBUGLOG(("PlaylistManager(%p{%s})::CreateNewRecord(%p{%s}, %p)\n", this, DebugName().cdata(),
    &obj, obj.DebugName().cdata(), parent));

  // Allocate a record in the HwndContainer
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  PMASSERT(rec != NULL);

  rec->UseCount     = 1;
  rec->Data()       = new CPData(obj, *this, &PlaylistManager::InfoChangeEvent, rec, (Record*)parent);
  // before we catch any information setup the update events
  // The record is not yet correctly initialized, but this don't matter since all that the event handlers can do
  // is to post a UM_RECORDCOMMAND which is not handled unless this message is completed.
  obj.GetInfoChange() += rec->Data()->InfoChange;
  // now get initial info's
  rec->Data()->Text = obj.GetDisplayName();
  rec->Data()->Recursive = obj.GetInfo().tech->attributes & TATTR_PLAYLIST
                        && RecursionCheck(obj.GetPlayable(), parent);
  rec->flRecordAttr = 0;
  rec->pszIcon      = (PSZ)rec->Data()->Text.cdata();
  rec->hptrIcon     = CalcIcon(rec);

  // Request initial infos for new children
  PlaylistBase::PostRecordUpdate(rec, RequestRecordInfo(rec));
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
}

void PlaylistManager::UpdateRecord(RecordBase* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateRecord(%p)\n", this, rec));
  bool update = false;
  for(;;)
  { // reset pending message flag
    InfoFlags flags = (InfoFlags)StateFromRec(rec).UpdateFlags.swap(IF_None);
    DEBUGLOG(("PlaylistManager::UpdateRecord - %x\n", flags));
    if (flags == IF_None)
      break;
    // Do update
    if (rec && (flags & IF_Rpl)) // not for root level
    { APlayable& p = *rec->Data->Content;
      bool recursive = p.GetInfo().tech->attributes & TATTR_PLAYLIST
                    && RecursionCheck(rec);
      if (recursive != ((Record*)rec)->Data()->Recursive)
      { ((Record*)rec)->Data()->Recursive = recursive;
        // Update Icon also
        PostRecordUpdate(rec, IF_Usage);
        // Remove children?
        if (recursive)
          RemoveChildren(rec);
      }
    }
    if (rec && (flags & (IF_Tech|IF_Display))) // not for root level
    { // techinfo changed => check whether it is currently visible.
      if (rec->flRecordAttr & CRA_CURSORED)
      { // TODO: maybe this should be better up to the calling thread
        EmFocus = &rec->Data->Content->GetPlayable();
        // continue later
        PMRASSERT(WinPostMsg(GetHwnd(), UM_UPDATEINFO, MPFROMP(rec), 0));
      }
    }
    if (flags & IF_Child)
    { UpdateChildren(rec);
      if (rec != NULL)
        flags |= IF_Usage;
    }
    if (rec && (flags & (IF_Tech|IF_Child|IF_Usage)))
    { HPOINTER icon = CalcIcon(rec);
      // update icon?
      if (rec->hptrIcon != icon)
      { rec->hptrIcon = icon;
        update = true;
      }
    }
    // update window title
    if (!rec && (flags & (IF_Phys|IF_Tech|IF_Display|IF_Usage)))
      SetTitle();
    // Update record title
    if (rec && (flags & IF_Display))
    { xstring text = rec->Data->Content->GetDisplayName();
      rec->pszIcon = (PSZ)text.cdata();
      update = true;
      rec->Data->Text = text; // now free the old text
    }
  }
  // update screen
  if (update)
    PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED)));
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
