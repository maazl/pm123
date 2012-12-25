/*
 * Copyright 2007-2012 Marcel Mueller
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
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


/* Code for the playlist detailed view */

#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DOS
#include "playlistview.h"
#include "../core/playable.h"
#include "../engine/decoder.h"
#include "playlistmenu.h"
#include "gui.h"
#include "dialog.h"
#include "pm123.rc.h"
#include <cpp/showaccel.h>
#include <utilfct.h>

#include <os2.h>

#include <debuglog.h>


HWND PlaylistView::MainMenu = NULLHANDLE;
HWND PlaylistView::RecMenu  = NULLHANDLE;


PlaylistView::PlaylistView(Playable& obj)
: PlaylistBase(obj, DLG_PLAYLIST)
{ DEBUGLOG(("PlaylistView::PlaylistView(&%p)\n", &obj));
  StartDialog();
}

PlaylistView::~PlaylistView()
{ RepositoryType::RemoveWithKey(*this, *Content);
}

PlaylistView* PlaylistView::Factory(Playable& key)
{ return new PlaylistView(key);
}

const int_ptr<PlaylistBase> PlaylistView::GetSame(Playable& obj)
{ return &*GetByKey(obj);
}

int PlaylistView::Comparer(const Playable& key, const PlaylistView& pl)
{ return CompareInstance(key, *pl.Content);
}

void PlaylistView::DestroyAll()
{ RepositoryType::IXAccess index;
  DEBUGLOG(("PlaylistView::DestroyAll() - %d\n", index->size()));
  // The instances deregister itself from the repository.
  // Starting at the end avoids the memcpy calls for shrinking the vector.
  while (index->size())
    (*index)[index->size()-1]->Destroy();
}

InfoFlags PlaylistView::FilterRecordRequest(RecordBase* const rec, InfoFlags& filter)
{ //DEBUGLOG(("PlaylistView(%p)::FilterRecordRequest(%s, %x)\n", this, Record::DebugName(rec).cdata(), filter));
  if (rec)
    filter &= IF_Decoder|IF_Item|IF_Display|IF_Usage|IF_Slice|IF_Drpl;
  else
    filter &= IF_Tech|IF_Display|IF_Usage|IF_Child;
  return filter & (IF_Decoder|IF_Item|IF_Display|IF_Usage);
}

const PlaylistView::Column PlaylistView::MutableColumns[] =
{ { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_BITMAPORICON | CFA_CENTER,
    CFA_FITITLEREADONLY,
    "",
    offsetof(PlaylistView::Record, hptrIcon)
  },
  { CFA_STRING | CFA_SEPARATOR | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Name (Alias)",
    offsetof(PlaylistView::Record, pszIcon),
    CID_Alias,
  },
  { CFA_STRING | CFA_SEPARATOR | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Start",
    offsetof(PlaylistView::Record, Start),
    CID_Start,
  },
  { CFA_STRING | CFA_SEPARATOR | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Stop",
    offsetof(PlaylistView::Record, Stop),
    CID_Stop,
  },
  { CFA_STRING | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Location",
    offsetof(PlaylistView::Record, At),
    CID_At,
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING,
    CFA_FITITLEREADONLY,
    "Title",
    offsetof(PlaylistView::Record, Song)
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING | CFA_RIGHT,
    CFA_FITITLEREADONLY | CFA_CENTER,
    "Size",
    offsetof(PlaylistView::Record, Size)
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING | CFA_RIGHT,
    CFA_FITITLEREADONLY | CFA_CENTER,
    "Time",
    offsetof(PlaylistView::Record, Time)
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING,
    CFA_FITITLEREADONLY,
    "Information",
    offsetof(PlaylistView::Record, MoreInfo)
  },
  { CFA_HORZSEPARATOR | CFA_STRING,
    CFA_FITITLEREADONLY,
    "Source",
    offsetof(PlaylistView::Record, URL),
    CID_URL
  }
};

/*const PlaylistView::Column PlaylistView::ConstColumns[] =
{ { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_BITMAPORICON | CFA_CENTER,
    CFA_FITITLEREADONLY,
    "",
    offsetof(PlaylistView::Record, hptrIcon)
  },
  { CFA_FIREADONLY | CFA_STRING | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Name (Alias)",
    offsetof(PlaylistView::Record, pszIcon)
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING,
    CFA_FITITLEREADONLY,
    "Song",
    offsetof(PlaylistView::Record, Song)
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING | CFA_RIGHT,
    CFA_FITITLEREADONLY | CFA_CENTER,
    "Size",
    offsetof(PlaylistView::Record, Size)
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING | CFA_RIGHT,
    CFA_FITITLEREADONLY | CFA_CENTER,
    "Time",
    offsetof(PlaylistView::Record, Time)
  },
  { CFA_FIREADONLY | CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING,
    CFA_FITITLEREADONLY,
    "Information",
    offsetof(PlaylistView::Record, MoreInfo)
  },
  { CFA_HORZSEPARATOR | CFA_STRING,
    CFA_FITITLEREADONLY,
    "Source",
    offsetof(PlaylistView::Record, URL)
  }
};*/

FIELDINFO* PlaylistView::MutableFieldinfo;
//FIELDINFO* PlaylistView::ConstFieldinfo;

FIELDINFO* PlaylistView::CreateFieldinfo(const Column* cols, size_t count)
{ FIELDINFO* first = (FIELDINFO*)WinSendMsg(HwndContainer, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(count), 0);
  PMASSERT(first != NULL);
  FIELDINFO* field = first;
  const Column* cp = cols;
  do
  { field->flData     = cp->DataAttr;
    field->flTitle    = cp->TitleAttr;
    field->pTitleData = (PVOID)cp->Title;
    field->offStruct  = cp->Offset;
    field->pUserData  = (PVOID)cp->CID;
    // next
    field = field->pNextFieldInfo;
    ++cp;
  } while (field);
  return first;
}

void PlaylistView::InitDlg()
{ DEBUGLOG(("PlaylistView(%p)::InitDlg()\n", this));

  // Initializes the container of the playlist.
  HwndContainer = WinWindowFromID(GetHwnd(), CO_CONTENT);
  PMASSERT(HwndContainer != NULLHANDLE);
  // Attention!!! Intended side effect: CCS_VERIFYPOINTERS is only set in debug builds
  PMASSERT(WinSetWindowBits(HwndContainer, QWL_STYLE, CCS_VERIFYPOINTERS, CCS_VERIFYPOINTERS));

  static bool initialized = false;
  if (!initialized)
  { MutableFieldinfo = CreateFieldinfo(MutableColumns, sizeof MutableColumns / sizeof *MutableColumns);
    //ConstFieldinfo   = CreateFieldinfo(ConstColumns,   sizeof ConstColumns   / sizeof *ConstColumns  );
  }

  FIELDINFO* first;
  FIELDINFOINSERT insert  = { sizeof(FIELDINFOINSERT) };
  CNRINFO cnrinfo         = { sizeof(cnrinfo) };
  insert.pFieldInfoOrder  = (PFIELDINFO)CMA_FIRST;
  insert.fInvalidateFieldInfo = TRUE;
  cnrinfo.flWindowAttr    = CV_DETAIL|CV_MINI|CA_DRAWICON|CA_DETAILSVIEWTITLES|CA_ORDEREDTARGETEMPH;
  cnrinfo.xVertSplitbar   = 180;
  //if ((Content->GetFlags() & Playable::Mutable) == Playable::Mutable)
  { insert.cFieldInfoInsert = sizeof MutableColumns / sizeof *MutableColumns;
    first = MutableFieldinfo;
    cnrinfo.pFieldInfoLast  = first->pNextFieldInfo->pNextFieldInfo->pNextFieldInfo->pNextFieldInfo; // The first 4 columns are left to the bar.
  }/* else
  { insert.cFieldInfoInsert = sizeof ConstColumns / sizeof *ConstColumns;
    first = ConstFieldinfo;
    cnrinfo.pFieldInfoLast  = first->pNextFieldInfo; // The first 2 columns are left to the bar.
  }*/
  PMXASSERT(SHORT1FROMMR(WinSendMsg(HwndContainer, CM_INSERTDETAILFIELDINFO, MPFROMP(first), MPFROMP(&insert))), != 0);
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrinfo), MPFROMLONG( CMA_PFIELDINFOLAST|CMA_XVERTSPLITBAR|CMA_FLWINDOWATTR)));

  // Initializes the playlist presentation window.
  PlaylistBase::InitDlg();
}

MRESULT PlaylistView::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistView(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_CONTROL:
    if (SHORT1FROMMP(mp1) == CO_CONTENT)
    { switch (SHORT2FROMMP(mp1))
      {
       case CN_HELP:
        GUI::ShowHelp(IDH_PL);
        return 0;

       case CN_ENDEDIT:
        if (DirectEdit) // ignore undo's
        { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
          Record* rec = (Record*)ed->pRecord;
          DEBUGLOG(("PlaylistView::DlgProc CN_ENDEDIT %p{,%p->%p{%s},%u,} %p %s\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText, rec, DirectEdit.cdata()));
          ASSERT(rec != NULL);
          switch ((ColumnID)(ULONG)ed->pFieldInfo->pUserData)
          {case CID_Alias:
            { PlayableInstance& pi = *rec->Data()->Content;
              Mutex::Lock lock(pi.GetPlayable().Mtx);
              ItemInfo info = *pi.GetInfo().item;
              if (DirectEdit.length())
                info.alias = DirectEdit;
              else
              { info.alias.reset();
                PostRecordUpdate(rec, IF_Display);
              }
              pi.OverrideItem(info.IsInitial() ? NULL : &info);
            }
            break;
           case CID_Start:
            { PlayableInstance& pi = *rec->Data()->Content;
              Mutex::Lock lock(pi.GetPlayable().Mtx);
              ItemInfo info = *pi.GetInfo().item;
              if (DirectEdit.length())
                info.start = DirectEdit;
              else
                info.start.reset();
              pi.OverrideItem(info.IsInitial() ? NULL : &info);
              // TODO: Error message
            }
            break;
           case CID_Stop:
            { PlayableInstance& pi = *rec->Data()->Content;
              Mutex::Lock lock(pi.GetPlayable().Mtx);
              ItemInfo info = *pi.GetInfo().item;
              if (DirectEdit.length())
                info.stop = DirectEdit;
              else
                info.stop.reset();
              pi.OverrideItem(info.IsInitial() ? NULL : &info);
              // TODO: Error message
            }
            break;
           case CID_At:
            { PlayableInstance& pi = *rec->Data()->Content;
              Mutex::Lock lock(pi.GetPlayable().Mtx);
              AttrInfo info = *pi.GetInfo().attr;
              if (DirectEdit.length())
                info.at = DirectEdit;
              else
                info.at.reset();
              pi.OverrideAttr(info.IsInitial() ? NULL : &info);
              // TODO: Error message
            }
            break;
           case CID_URL:
            { rec->URL    = xstring::empty; // intermediate string that is not invalidated
              // Convert to insert/delete pair.
              InsertInfo* pii = new InsertInfo();
              pii->Parent = &APlayableFromRec(GetParent(rec)).GetPlayable();
              PlayableInstance& pi = *rec->Data()->Content;
              pii->Before = &pi;
              pii->Item   = new PlayableRef(*Playable::GetByURL(Content->URL.makeAbsolute(DirectEdit)));
              if (pi.GetOverridden() & IF_Item)
              { ItemInfo info = *pi.GetInfo().item;
                pii->Item->OverrideItem(&info);
              }
              // Send GUI commands
              BlockRecord(rec);
              WinPostMsg(GetHwnd(), UM_INSERTITEM, MPFROMP(pii), 0);
              WinPostMsg(GetHwnd(), UM_REMOVERECORD, MPFROMP(rec), 0);
            }
          }
        }
      }
      break;
    }

   case WM_COMMAND:
    { switch (SHORT1FROMMP(mp1))
      {case IDM_PL_SELECT_ALL:
        UserSelectAll();
        return 0;
      }
      break;
    }
  }
  return PlaylistBase::DlgProc(msg, mp1, mp2);
}

void PlaylistView::InitContextMenu()
{ DEBUGLOG(("PlaylistView(%p)::InitContextMenu() - %p{%u}\n", this, &Source, Source.size()));

  if (Source.size() == 0)
  { // no items selected => main context menu
    bool new_menu;
    if ((new_menu = MainMenu == NULLHANDLE) != false)
    { MainMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_PLAYLIST);
      PMASSERT(MainMenu != NULLHANDLE);
      PMRASSERT(mn_make_conditionalcascade(MainMenu, IDM_PL_APPENDALL, IDM_PL_APPFILEALL));
    }
    HwndMenu = MainMenu;
    // Update accelerators?
    if (DecChanged || new_menu)
    { DecChanged = false;
      // Populate context menu with plug-in specific stuff.
      MENUITEM item;
      PMRASSERT(WinSendMsg(HwndMenu, MM_QUERYITEM, MPFROM2SHORT(IDM_PL_APPENDALL, TRUE), MPFROMP(&item)));
      memset(LoadWizards+StaticWizzards, 0, sizeof LoadWizards - StaticWizzards*sizeof *LoadWizards ); // You never know...
      Decoder::AppendLoadMenu(item.hwndSubMenu, IDM_PL_APPOTHERALL, StaticWizzards, LoadWizards+StaticWizzards, sizeof LoadWizards/sizeof *LoadWizards - StaticWizzards);
      (MenuShowAccel(WinQueryAccelTable(WinQueryAnchorBlock(GetHwnd()), GetHwnd()))).ApplyTo(new_menu ? HwndMenu : item.hwndSubMenu);
    }
    GetMenuWorker().AttachMenu(HwndMenu, IDM_PL_CONTENT, *Content, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Enumerate|PlaylistMenu::Recursive, 0);
  } else
  { // at least one item selected
    if (RecMenu == NULLHANDLE)
    { RecMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_RECORD);
      PMASSERT(RecMenu != NULLHANDLE);
      PMRASSERT(mn_make_conditionalcascade(RecMenu, IDM_PL_FLATTEN, IDM_PL_FLATTEN_1));
      (MenuShowAccel(WinQueryAccelTable(WinQueryAnchorBlock(GetHwnd()), GetHwnd()))).ApplyTo(RecMenu);
    }
    RecordType rt = AnalyzeRecordTypes();
    if (rt == RT_None)
      return;
    HwndMenu = RecMenu;

    mn_enable_item(HwndMenu, IDM_PL_NAVIGATE, Source.size() == 1 && IsUnderCurrentRoot(Source[0]));
    mn_enable_item(HwndMenu, IDM_PL_EDIT,     rt == RT_Meta);
    mn_enable_item(HwndMenu, IDM_PL_FLATTEN,  (rt & ~(RT_Enum|RT_List)) == 0);
    //mn_enable_item(hwndMenu, IDM_PL_REFRESH,  (rt & (RT_Enum|RT_List)) == 0);
    bool is_pl = Source.size() == 1 && (rt & (RT_Enum|RT_List));
    mn_enable_item(HwndMenu, IDM_PL_DETAILED, is_pl);
    mn_enable_item(HwndMenu, IDM_PL_TREEVIEW, is_pl);
    mn_enable_item(HwndMenu, IDM_PL_CONTENT,  is_pl);
    if (is_pl)
      GetMenuWorker().AttachMenu(HwndMenu, IDM_PL_CONTENT, *Source[0]->Data->Content, PlaylistMenu::DummyIfEmpty|PlaylistMenu::Enumerate|PlaylistMenu::Recursive, 0);
  }
  DEBUGLOG2(("PlaylistView::InitContextMenu: Menu: %p %p\n", MainMenu, RecMenu));
}

void PlaylistView::UpdateAccelTable()
{ DEBUGLOG(("PlaylistView::UpdateAccelTable()\n"));
  AccelTable = WinLoadAccelTable( WinQueryAnchorBlock( GetHwnd() ), NULLHANDLE, ACL_PLAYLIST );
  PMASSERT(AccelTable != NULLHANDLE);
  memset( LoadWizards+StaticWizzards, 0, sizeof LoadWizards - StaticWizzards*sizeof *LoadWizards); // You never know...
  Decoder::AppendAccelTable( AccelTable, IDM_PL_APPOTHERALL, 0, LoadWizards+StaticWizzards, sizeof LoadWizards/sizeof *LoadWizards - StaticWizzards);
}

PlaylistBase::ICP PlaylistView::GetPlaylistType(const RecordBase* rec) const
{ DEBUGLOG(("PlaylistView::GetPlaylistType(%s)\n", Record::DebugName(rec).cdata()));
  Playable& pp = rec->Data->Content->GetPlayable();
  if (pp == *Content)
    return ICP_Recursive;
  return pp.RequestInfo(IF_Child, PRI_None, REL_Invalid) || pp.GetNext(NULL) == NULL ? ICP_Empty : ICP_Closed;
}

const xstring PlaylistView::FormatSize(double size)
{ if (size <= 0)
    return xstring::empty;
  char unit = 'k';
  unsigned long s = (unsigned long)(size / 1024.);
  if (s > 9999)
  { s /= 1024;
    unit = 'M';
    if (s > 9999)
    { s /= 1024;
      unit = 'G';
    }
  }
  return xstring().sprintf("%lu %cB", s, unit);
}

const xstring PlaylistView::FormatTime(double time)
{ if (time <= 0)
    return xstring::empty;
  unsigned long s = (unsigned long)time;
  xstring ret;
  if (s < 60)
    ret.sprintf("%lu s", s);
   else if (s < 3600)
    ret.sprintf("%lu:%02lu", s/60, s%60);
   else
    ret.sprintf("%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
  return ret;
}

void PlaylistView::AppendNonEmpty(xstring& dst, const char* sep, const volatile xstring& app)
{ xstring tmp(app);
  if (tmp && tmp.length())
    dst = dst + sep + tmp;
}

bool PlaylistView::UpdateColumnText(xstring& dst, const xstring& value)
{ const xstring& val2 = value ? value : xstring::empty;
  if (dst == val2)
    return false;
  (volatile xstring&)dst = val2; // atomic assign
  return true;
}

bool PlaylistView::CalcCols(Record* rec, InfoFlags flags)
{ DEBUGLOG(("PlaylistView::CalcCols(%s, %x)\n", Record::DebugName(rec).cdata(), flags));
  const INFO_BUNDLE_CV& info = rec->Data()->Content->GetInfo();
  bool ret = false;
  // Columns that only depend on metadata changes
  if (flags & IF_Meta)
  { // song
    xstring artist = info.meta->artist;
    if (!artist)
      artist = xstring::empty;
    xstring title  = info.meta->title;
    if (!title)
      title  = xstring::empty;
    xstring tmp;
    if (artist.length())
      tmp = artist + " / " + title;
     else if (title.length())
      tmp = title;
     else
      tmp = NULL;
    ret |= UpdateColumnText(rec->Song, tmp);
  }
  // size
  if (flags & (IF_Phys|IF_Tech|IF_Drpl))
    ret |= UpdateColumnText(rec->Size, FormatSize(info.tech->attributes & TATTR_SONG ? info.drpl->totalsize : info.phys->filesize));
  // time
  if (flags & IF_Drpl)
    ret |= UpdateColumnText(rec->Time, FormatTime(info.drpl->totallength));
  // Columns that only depend on attribute info changes
  if (flags & IF_Attr)
    // location
    ret |= UpdateColumnText(rec->At, xstring(info.attr->at));
  // Columns that depend on metadata or tech changes
  if (flags & (IF_Meta|IF_Tech))
  { // moreinfo
    xstring tmp = info.meta->album;
    if (!tmp)
      tmp = xstring::empty;
    AppendNonEmpty(tmp, " #", info.meta->track);
    AppendNonEmpty(tmp, " ", info.meta->year);
    AppendNonEmpty(tmp, ", ", info.meta->genre);
    AppendNonEmpty(tmp, ", ", info.tech->info);
    ret |= UpdateColumnText(rec->MoreInfo, tmp);
  }
  // Columns that depend on name changes
  if (flags & IF_Display)
  { // Alias
    xstring tmp = rec->Data()->Content->GetDisplayName();
    rec->pszIcon = (PSZ)tmp.cdata();
    rec->Data()->Text = tmp; // free old value
    ret = true;
  }
  if (flags & IF_Item)
  { // Starting position
    const volatile ITEM_INFO& ii = *rec->Data()->Content->GetInfo().item;
    ret |= UpdateColumnText(rec->Start, xstring(ii.start));
    ret |= UpdateColumnText(rec->Stop, xstring(ii.stop));
  }
  return ret;
}

PlaylistBase::RecordBase* PlaylistView::CreateNewRecord(PlayableInstance& obj, RecordBase* parent)
{ DEBUGLOG(("PlaylistView(%p{%s})::CreateNewRecord(&%p{%s}, %p)\n", this, DebugName().cdata(),
    &obj, obj.DebugName().cdata(), parent));
  // No nested records in this view
  ASSERT(parent == NULL);
  // Allocate a record in the HwndContainer
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  PMASSERT(rec != NULL);

  rec->UseCount     = 1;
  rec->Data()       = new CPData(obj, *this, &PlaylistView::InfoChangeEvent, rec);
  // before we catch any information setup the update events
  // The record is not yet correctly initialized, but this don't matter since all that the event handlers can do
  // is to post a UM_RECORDCOMMAND which is not handled unless this message is completed.
  obj.GetInfoChange() += rec->Data()->InfoChange;

  rec->URL          = obj.GetPlayable().URL;
  // Request initial infos
  CalcCols(rec, RequestRecordInfo(rec));

  rec->flRecordAttr = 0;
  rec->hptrIcon     = CalcIcon(rec);
  return rec;
}

PlaylistView::RecordBase* PlaylistView::GetParent(const RecordBase* const rec) const
{ return NULL;
}

void PlaylistView::UpdateRecord(RecordBase* rec)
{ DEBUGLOG(("PlaylistView(%p)::UpdateRecord(%p)\n", this, rec));
  bool update = false;
  for(;;)
  { // reset pending message flag
    InfoFlags flags = (InfoFlags)StateFromRec(rec).UpdateFlags.swap(IF_None);
    DEBUGLOG(("PlaylistView::UpdateRecord - %x\n", flags));
    if (flags == IF_None)
      break;
    // update window title
    if (!rec && (flags & (IF_Tech|IF_Display|IF_Usage)))
      SetTitle();
    // update icon?
    if (rec && (flags & (IF_Tech|IF_Child|IF_Usage)))
    { HPOINTER icon = CalcIcon(rec);
      // update icon?
      if (rec->hptrIcon != icon)
      { rec->hptrIcon = icon;
        update = true;
      }
    }
    // Update columns of items
    if (rec && CalcCols((Record*)rec, flags))
      update = true;
    if (!rec && (flags & IF_Child)) // Only Root node has children
      UpdateChildren(NULL);
  }
  // update screen
  if (update)
    PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED)));
}

void PlaylistView::UserSelectAll()
{ Source.clear();
  RecordBase* crp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(NULL), MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
  while (crp != NULL && crp != (RecordBase*)-1)
  { DEBUGLOG(("PlaylistView::UserSelectAll CM_QUERYRECORD: %s\n", RecordBase::DebugName(crp).cdata()));
    Source.append() = crp;
    crp = (RecordBase*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
  }
  SetEmphasis(CRA_SELECTED, true);
}
