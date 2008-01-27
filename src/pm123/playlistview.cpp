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
#include "playlistview.h"
#include "docking.h"
#include "iniman.h"
#include "playable.h"
#include "plugman.h"

#include <stddef.h>

#include <debuglog.h>


/*sorted_vector<PlaylistView, char> PlaylistView::RPInst(8);
Mutex PlaylistView::RPMutex;

void PlaylistView::Init()
{ // currently a no-op
  // TODO: we should cleanup unused and invisibla PlaylistManager instances once in a while.
}

void PlaylistView::UnInit()
{ DEBUGLOG(("PlaylistView::UnInit()\n"));
  // Free stored instances.
  // The instances deregister itself from the repository.
  // Starting at the end avoids the memcpy calls for shrinking the vector.
  while (RPInst.size())
    delete RPInst[RPInst.size()-1];
}

PlaylistView* PlaylistView::Get(const char* url, const char* alias)
{ DEBUGLOG(("PlaylistView::Get(%s, %s)\n", url, alias ? alias : "<NULL>"));
  Mutex::Lock lock(RPMutex);
  PlaylistView*& pp = RPInst.get(url);
  if (!pp)
    pp = new PlaylistView(url, alias);
  return pp;
}*/


PlaylistView::PlaylistView(const char* URL, const char* alias)
: PlaylistRepository<PlaylistView>(URL, alias, DLG_PLAYLIST),
  MainMenu(NULLHANDLE),
  RecMenu(NULLHANDLE)
{ DEBUGLOG(("PlaylistView::PlaylistView(%s, %s)\n", URL, alias));
  //HwndFrame = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP, pl_DlgProcStub, NULLHANDLE, DLG_PLAYLIST, &ids );
  StartDialog();
}

void PlaylistView::PostRecordCommand(RecordBase* rec, RecordCommand cmd)
{ // Ignore some messages
  switch (cmd)
  {case RC_UPDATEFORMAT:
   case RC_UPDATEMETA:
    if (rec == NULL)
      break;
   default:
    PlaylistBase::PostRecordCommand(rec, cmd);
  }
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
    offsetof(PlaylistView::Record, pszIcon)
  },
  { CFA_STRING | CFA_SEPARATOR | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Start",
    offsetof(PlaylistView::Record, Pos)
  },
  { CFA_STRING | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Stop",
    offsetof(PlaylistView::Record, End)
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
};

const PlaylistView::Column PlaylistView::ConstColumns[] =
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
};

FIELDINFO* PlaylistView::MutableFieldinfo;
FIELDINFO* PlaylistView::ConstFieldinfo;

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
    // next
    field = field->pNextFieldInfo;
    ++cp;
  } while (field);
  return first;
}

void PlaylistView::InitDlg()
{ DEBUGLOG(("PlaylistView(%p)::InitDlg()\n", this));

  // Initializes the container of the playlist.
  #ifdef DEBUG
  HwndContainer = WinCreateWindow( HwndFrame, WC_CONTAINER, "", WS_VISIBLE|CCS_EXTENDSEL|CCS_MINIICONS|CCS_MINIRECORDCORE|CCS_VERIFYPOINTERS,
                                   0, 0, 0, 0, HwndFrame, HWND_TOP, FID_CLIENT, NULL, NULL);
  #else
  HwndContainer = WinCreateWindow( HwndFrame, WC_CONTAINER, "", WS_VISIBLE|CCS_EXTENDSEL|CCS_MINIICONS|CCS_MINIRECORDCORE,
                                   0, 0, 0, 0, HwndFrame, HWND_TOP, FID_CLIENT, NULL, NULL);
  #endif
  PMASSERT(HwndContainer != NULLHANDLE);

  static bool initialized = false;
  if (!initialized)
  { MutableFieldinfo = CreateFieldinfo(MutableColumns, sizeof MutableColumns / sizeof *MutableColumns);
    ConstFieldinfo   = CreateFieldinfo(ConstColumns,   sizeof ConstColumns   / sizeof *ConstColumns  );
  }

  FIELDINFO* first;
  FIELDINFOINSERT insert  = { sizeof(FIELDINFOINSERT) };
  CNRINFO cnrinfo         = { sizeof(cnrinfo) };
  insert.pFieldInfoOrder  = (PFIELDINFO)CMA_FIRST;
  insert.fInvalidateFieldInfo = TRUE;
  cnrinfo.flWindowAttr    = CV_DETAIL|CV_MINI|CA_DRAWICON|CA_DETAILSVIEWTITLES|CA_ORDEREDTARGETEMPH;
  cnrinfo.xVertSplitbar   = 180;
  if ((Content->GetFlags() & Playable::Mutable) == Playable::Mutable)
  { insert.cFieldInfoInsert = sizeof MutableColumns / sizeof *MutableColumns;
    first = MutableFieldinfo;
    cnrinfo.pFieldInfoLast  = first->pNextFieldInfo->pNextFieldInfo->pNextFieldInfo; // The first 3 colums are left to the bar.
  } else
  { insert.cFieldInfoInsert = sizeof ConstColumns / sizeof *ConstColumns;
    first = ConstFieldinfo;
    cnrinfo.pFieldInfoLast  = first->pNextFieldInfo; // The first 2 colums are left to the bar.
  }
  PMXASSERT(SHORT1FROMMR(WinSendMsg(HwndContainer, CM_INSERTDETAILFIELDINFO, MPFROMP(first), MPFROMP(&insert))), != 0);
  PMRASSERT(WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrinfo), MPFROMLONG( CMA_PFIELDINFOLAST|CMA_XVERTSPLITBAR|CMA_FLWINDOWATTR)));

  /* Initializes the playlist presentation window. */
  PlaylistBase::InitDlg();
}

MRESULT PlaylistView::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistView(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_WINDOWPOSCHANGED:
    // TODO: Bl”dsinn
    { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
      if( pswp[0].fl & SWP_SHOW )
        cfg.show_playlist = TRUE;
      if( pswp[0].fl & SWP_HIDE )
        cfg.show_playlist = FALSE;
      break;
    }

   case WM_HELP:
    amp_show_help( IDH_PL );
    return 0;

   /*case WM_COMMAND:
    { SHORT cmd = SHORT1FROMMP(mp1);
      Record* focus = (Record*)CmFocus;
      if (cmd >= IDM_PL_APPFILE && cmd < IDM_PL_APPFILE + sizeof LoadWizzards / sizeof *LoadWizzards)
      { DECODER_WIZZARD_FUNC func = LoadWizzards[cmd-IDM_PL_APPFILE];
        if (func != NULL)
          //UserAdd(func, "Append%s", focus); // focus from CN_CONTEXTMENU
        return 0;
      }
      switch (cmd)
      {
      }
      break;
    }*/

   case UM_RECORDCOMMAND:
    { Record* rec = (Record*)PVOIDFROMMP(mp1);
      DEBUGLOG(("PlaylistView::DlgProc: UM_RECORDCOMMAND: %s, %x\n", Record::DebugName(rec).cdata(), StateFromRec(rec).PostMsg));
      for(;;)
      { // reset pending message flag
        unsigned flags = InterlockedXch(StateFromRec(rec).PostMsg, 0);
        if (flags == 0)
          break;
        Playable::InfoFlags flg = Playable::IF_None;
        PlayableInstance::StatusFlags iflg = PlayableInstance::SF_None;
        if (flags & 1<<RC_UPDATEFORMAT)
          flg |= Playable::IF_Format;
        if (flags & 1<<RC_UPDATETECH)
        { flg |= Playable::IF_Tech;
          bool& wait = StateFromRec(rec).WaitUpdate;
          if (wait)
          { // Schedule UpdateChildren too
            wait = false;
            flags |= 1<<RC_UPDATECHILDREN;
          }
        }
        if (flags & 1<<RC_UPDATEMETA)
          flg |= Playable::IF_Meta;
        if (flags & 1<<RC_UPDATEALIAS)
          iflg |= PlayableInstance::SF_Alias;
        if (flags & 1<<RC_UPDATEPOS)
          iflg |= PlayableInstance::SF_Slice;

        if ((int)flg | iflg)
          UpdateRecord(rec, flg, iflg);
        if (flags & 1<<RC_UPDATESTATUS)
          UpdateStatus(rec);
        if (flags & 1<<RC_UPDATECHILDREN && rec == NULL) // Only Root node has children
          UpdateChildren(NULL);
      }
      break; // continue in base class
    }
  }
  return PlaylistBase::DlgProc(msg, mp1, mp2);
}

HWND PlaylistView::InitContextMenu()
{ DEBUGLOG(("PlaylistView(%p)::InitContextMenu() - %p{%u}\n", this, &Source, Source.size()));
  HWND hwndMenu;
  if (Source.size() == 0)
  { if (MainMenu == NULLHANDLE)
    { MainMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_PLAYLIST);
      PMASSERT(MainMenu != NULLHANDLE);
      mn_make_conditionalcascade(MainMenu, IDM_PL_APPEND, IDM_PL_APPFILE);
    }
    hwndMenu = MainMenu;
    if ((Content->GetFlags() & Playable::Mutable) == Playable::Mutable)
    { mn_enable_item(hwndMenu, IDM_PL_SAVE,   true);
      mn_enable_item(hwndMenu, IDM_PL_APPEND, true);
    } else
    { mn_enable_item(hwndMenu, IDM_PL_SAVE,   false);
      mn_enable_item(hwndMenu, IDM_PL_APPEND, false);
    }
  } else
  { if (RecMenu == NULLHANDLE)
    { RecMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_RECORD);
      PMASSERT(RecMenu != NULLHANDLE);
    }
    hwndMenu = RecMenu;
    RecordType rt = AnalyzeRecordTypes();
    if (rt == RT_None)
      return NULLHANDLE;
    mn_enable_item(hwndMenu, IDM_PL_NAVIGATE, Source.size() == 1);
    mn_enable_item(hwndMenu, IDM_PL_EDIT,     Source.size() == 1 && Source[0]->Data->Content->GetPlayable()->GetInfo().meta_write);
    mn_enable_item(hwndMenu, IDM_PL_REFRESH,  (rt & (RT_Enum|RT_List)) == 0);
    mn_enable_item(hwndMenu, IDM_PL_DETAILED, Source.size() == 1 && rt != RT_Song);
    mn_enable_item(hwndMenu, IDM_PL_TREEVIEW, Source.size() == 1 && rt != RT_Song);
  }
  // emphasize record
  DEBUGLOG2(("PlaylistView::InitContextMenu: Menu: %p %p\n", MainMenu, RecMenu));
  return hwndMenu;
}

PlaylistBase::ICP PlaylistView::GetPlayableType(RecordBase* rec)
{ DEBUGLOG(("PlaylistView::GetPlaylistState(%s)\n", Record::DebugName(rec).cdata()));
  if ((rec->Data->Content->GetPlayable()->GetFlags() & Playable::Enumerable) == 0)
    return ICP_Song;
  // TODO: the dependancy to the technical info is not handled by the update events
  return rec->Data->Content->GetPlayable()->GetInfo().tech->num_items ? ICP_Closed : ICP_Empty;
}

PlaylistBase::IC PlaylistView::GetRecordUsage(RecordBase* rec)
{ DEBUGLOG(("PlaylistView::GetRecordUsage(%s)\n", Record::DebugName(rec).cdata()));
  if (rec->Data->Content->GetPlayable()->GetStatus() != STA_Used)
    return IC_Normal;
  if (rec->Data->Content->GetStatus() != STA_Used)
    return IC_Used;
  return decoder_playing() ? IC_Play : IC_Active;
}

xstring PlaylistView::FormatSize(double size)
{ if (size <= 0)
    return xstring();
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
  return xstring::sprintf("%lu %cB", s, unit);
}

xstring PlaylistView::FormatTime(double time)
{ if (time <= 0)
    return xstring();
  unsigned long s = (unsigned long)time;
  if (s < 60)
    return xstring::sprintf("%lu s", s);
   else if (s < 3600)
    return xstring::sprintf("%lu:%02lu", s/60, s%60);
   else
    return xstring::sprintf("%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
}

bool PlaylistView::CalcCols(Record* rec, Playable::InfoFlags flags, PlayableInstance::StatusFlags iflags)
{ DEBUGLOG(("PlaylistView::CalcCols(%s, %x)\n", Record::DebugName(rec).cdata(), flags));
  const DECODER_INFO2& info = rec->Data()->Content->GetPlayable()->GetInfo();
  bool ret = false;
  xstring tmp;
  // Columns that only depend on metadata changes
  if (flags & Playable::IF_Meta)
  { // song
    if (*info.meta->artist)
      tmp = xstring(info.meta->artist) + " / " + info.meta->title;
     else if (*info.meta->title)
      tmp = info.meta->title;
     else
      tmp = NULL;
    rec->Song = tmp;
    rec->Data()->Song = tmp; // free old value
  }
  // Columns that only depend on tech changes
  if (flags & Playable::IF_Tech)
  { // size
    tmp = FormatSize(info.tech->filesize);
    rec->Size = tmp;
    rec->Data()->Size = tmp; // free old value
    // time
    tmp = FormatTime(info.tech->songlength);
    rec->Time = tmp;
    rec->Data()->Time = tmp; // free old value
  }
  // Columns that depend on metadata or tech changes
  if (flags & (Playable::IF_Meta|Playable::IF_Tech))
  { // moreinfo
    tmp = info.meta->album;
    if (info.meta->track > 0)
      tmp = xstring::sprintf("%s #%u", tmp.cdata(), info.meta->track);
    if (*info.meta->year)
      tmp = tmp + " " + info.meta->year;
    if (*info.meta->genre)
      tmp = tmp + ", " + info.meta->genre;
    if (*info.tech->info)
      tmp = tmp + ", " + info.tech->info;
    if (*info.meta->comment)
      tmp = tmp + ", comment: " + info.meta->comment;
    ret = true;
  }
  // Colums that depend on PlayableInstance changes
  if (iflags & PlayableInstance::SF_Alias)
  { // Alias
    tmp = rec->Data()->Content->GetDisplayName();
    rec->pszIcon = (PSZ)tmp.cdata();
    rec->Data()->Text = tmp; // free old value
    ret = true;
  }
  if (iflags & PlayableInstance::SF_Slice)
  { // Starting position
    tmp = FormatTime(rec->Data()->Content->GetSlice().Start);
    rec->Pos = tmp;
    rec->Data()->Pos = tmp;
    // Ending position
    if (rec->Data()->Content->GetSlice().Stop < 0)
      tmp = xstring::empty;
    else
      tmp = FormatTime(rec->Data()->Content->GetSlice().Stop);
    rec->End = tmp;
    rec->Data()->End = tmp;
    ret = true;
  }
  return ret;
}

PlaylistBase::RecordBase* PlaylistView::CreateNewRecord(PlayableInstance* obj, RecordBase* parent)
{ DEBUGLOG(("PlaylistView(%p{%s})::CreateNewRecord(%p{%s}, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable()->GetURL().getShortName().cdata(), parent));
  // No nested records in this view
  ASSERT(parent == NULL);
  // Allocate a record in the HwndContainer
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  PMASSERT(rec != NULL);

  rec->UseCount        = 1;
  rec->Data()          = new CPData(obj, *this, &PlaylistView::InfoChangeEvent, &PlaylistView::StatChangeEvent, rec);
  // before we catch any information setup the update events
  // The record is not yet corretly initialized, but this don't metter since all that the event handlers can do
  // is to post a UM_RECORDCOMMAND which is not handled unless this message is completed.
  obj->GetPlayable()->InfoChange += rec->Data()->InfoChange;
  obj->StatusChange              += rec->Data()->StatChange;

  rec->URL             = obj->GetPlayable()->GetURL().cdata();
  CalcCols(rec, obj->GetPlayable()->EnsureInfoAsync(Playable::IF_Format|Playable::IF_Tech|Playable::IF_Meta), PlayableInstance::SF_All);

  rec->flRecordAttr    = 0;
  rec->hptrIcon        = CalcIcon(rec);
  return rec;
}

void PlaylistView::UpdateRecord(Record* rec, Playable::InfoFlags flags, PlayableInstance::StatusFlags iflags)
{ DEBUGLOG(("PlaylistView(%p)::UpdateRecord(%p, %x, %x)\n", this, rec, flags, iflags));
  // Check if UpdateChildren is waiting
  bool& wait = StateFromRec(rec).WaitUpdate;
  if (wait)
  { // Try again
    wait = false;
    PostRecordCommand(rec, RC_UPDATECHILDREN);
  }
  bool update = false;
  // update icon?
  if (rec && ((flags & Playable::IF_Tech) != 0 || (iflags & PlayableInstance::SF_InUse) != 0))
  { HPOINTER icon = CalcIcon(rec);
    // update icon?
    if (rec->hptrIcon != icon)
    { rec->hptrIcon = icon;
      update = true;
    }
  }
  // Update columns of items
  if (rec && CalcCols(rec, flags, iflags))
    update = true;
  // update screen
  if (update)
    PMRASSERT(WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED)));
}

void PlaylistView::UserRemove(RecordBase* rec)
{ DEBUGLOG(("PlaylistView(%p)::UserRemove(%s)\n", this, rec->DebugName().cdata()));
  if (Content->GetFlags() & Playable::Mutable) // don't modify constant object
    ((Playlist&)*Content).RemoveItem(rec->Data->Content);
  // the update of the container is implicitely done by the notification mechanism
}
