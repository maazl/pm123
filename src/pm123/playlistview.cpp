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
: PlaylistRepository<PlaylistView>(URL, alias),
  MainMenu(NULLHANDLE),
  ListMenu(NULLHANDLE),
  FileMenu(NULLHANDLE)
{ DEBUGLOG(("PlaylistView::PlaylistView(%s, %s)\n", URL, alias));
  init_dlg_struct ids = { sizeof(init_dlg_struct), this };
  HwndFrame = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP, pl_DlgProcStub, NULLHANDLE, DLG_PLAYLIST, &ids );
}

PlaylistView::~PlaylistView()
{ DEBUGLOG(("PlaylistView(%p{%s})::~PlaylistView() - %p\n", this, DebugName().cdata(), HwndFrame));
  save_window_pos(HwndFrame, 0);
  WinDestroyWindow(HwndFrame);
  DEBUGLOG(("PlaylistView::~PlaylistView() done - %p\n", WinGetLastError(NULL)));
}

void PlaylistView::PostRecordCommand(RecordBase* rec, RecordCommand cmd)
{ // Ignore some messages
  switch (cmd)
  {case RC_UPDATEFORMAT:
   case RC_UPDATEMETA:
    if (rec == NULL)
      return;
  }
  PlaylistBase::PostRecordCommand(rec, cmd);
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
  { CFA_STRING | CFA_HORZSEPARATOR,
    CFA_FITITLEREADONLY,
    "Start",
    offsetof(PlaylistView::Record, Pos)
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
  if (!HwndContainer)
  { DEBUGLOG(("PlaylistView::InitDlg: failed to create HwndContainer, error = %lx\n", WinGetLastError(NULL)));
    return;
  }

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
    cnrinfo.pFieldInfoLast  = first->pNextFieldInfo->pNextFieldInfo; // The first 3 colums are left to the bar.
  } else
  { insert.cFieldInfoInsert = sizeof ConstColumns / sizeof *ConstColumns;
    first = ConstFieldinfo;
    cnrinfo.pFieldInfoLast  = first->pNextFieldInfo; // The first 2 colums are left to the bar.
  } 
  WinSendMsg(HwndContainer, CM_INSERTDETAILFIELDINFO, MPFROMP(first), MPFROMP(&insert));
  WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrinfo), MPFROMLONG( CMA_PFIELDINFOLAST|CMA_XVERTSPLITBAR|CMA_FLWINDOWATTR));

  /* Initializes the playlist presentation window. */
  PlaylistBase::InitDlg();

  RequestChildren();
}

MRESULT PlaylistView::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistView(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_DESTROY:
    // delete all records
    { DEBUGLOG(("PlaylistView::DlgProc: WM_DESTROY\n"));
      RemoveChildren();
      break; // Continue in base class
    }

   case WM_WINDOWPOSCHANGED:
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

   case WM_CONTROL:
    switch (SHORT2FROMMP(mp1))
    {
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
          { MainMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_PLAYLIST);
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
        } else if (focus->Content->GetPlayable().GetFlags() & Playable::Enumerable)
        { if (ListMenu == NULLHANDLE)
          { ListMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_PLRECORD);
          }
          hwndMenu = ListMenu;
        } else
        { if (FileMenu == NULLHANDLE)
          { FileMenu = WinLoadMenu(HWND_OBJECT, 0, MNU_RECORD);
          }
          hwndMenu = FileMenu;
          if (focus->IsRemoved())
            return 0; // record could be removed at WinLoadMenu
          mn_enable_item(hwndMenu, IDM_PL_EDIT, focus->Content->GetPlayable().GetInfo().meta_write);
        }
        if (Record::IsRemoved(focus))
          return 0; // record could be removed now
        DEBUGLOG2(("PlaylistManager::DlgProc: Menu: %p %p %p\n", MainMenu, ListMenu, FileMenu));
        WinPopupMenu(HWND_DESKTOP, HwndFrame, hwndMenu, ptlMouse.x, ptlMouse.y, 0,
                     PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
        return 0;
      }
    }
    break;
  
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
        if (flags & 1<<RC_UPDATECHILDREN && rec == NULL) // Only Root node has children
          UpdateChildren();
        Playable::InfoFlags flg = Playable::IF_None;
        PlayableInstance::StatusFlags iflg = PlayableInstance::SF_None;
        if (flags & 1<<RC_UPDATEFORMAT)
          flg |= Playable::IF_Format;
        if (flags & 1<<RC_UPDATETECH)
          flg |= Playable::IF_Tech;
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
      }
      break; // continue in base class
    }
      
   case UM_SYNCREMOVE:
    { Record* rec = (Record*)PVOIDFROMMP(mp1);
      rec->URL = NULL; // We have to discard the URL immediately because it is an alias of Content.
      break; // continue in base class
    }
  }
  return PlaylistBase::DlgProc(msg, mp1, mp2);
}

PlaylistBase::ICP PlaylistView::GetPlayableType(RecordBase* rec)
{ DEBUGLOG(("PlaylistView::GetPlaylistState(%s)\n", Record::DebugName(rec).cdata()));
  return (rec->Content->GetPlayable().GetFlags() & Playable::Enumerable) == 0
    ? ICP_Song : ICP_Empty;
}

PlaylistBase::IC PlaylistView::GetRecordUsage(RecordBase* rec)
{ DEBUGLOG(("PlaylistView::GetRecordUsage(%s)\n", Record::DebugName(rec).cdata()));
  if (rec->Content->GetPlayable().GetStatus() != STA_Used)
    return IC_Normal;
  if (rec->Content->GetStatus() != STA_Used)
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
  const DECODER_INFO2& info = rec->Content->GetPlayable().GetInfo();
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
    tmp = rec->Content->GetDisplayName();
    rec->pszIcon = (PSZ)tmp.cdata();
    rec->Data()->Text = tmp; // free old value
    ret = true;
  }
  if (iflags & PlayableInstance::SF_Slice)
  { // Starting position
    tmp = FormatTime(rec->Content->GetSlice().Start);
    rec->Pos = tmp;
    rec->Data()->Pos = tmp;
    ret = true;
  }
  return ret;
}

PlaylistView::Record* PlaylistView::AddEntry(PlayableInstance* obj, Record* after)
{ DEBUGLOG(("PlaylistView(%p{%s})::AddEntry(%p{%s}, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable().GetURL().getShortName().cdata(), after));
  /* Allocate a record in the HwndContainer */
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  if (rec == NULL)
  { DEBUGLOG(("PlaylistView::AddEntry: CM_ALLOCRECORD failed, error %lx\n", WinGetLastError(NULL)));
    DosBeep(500, 100);
    return NULL;
  }

  rec->Content         = obj;
  rec->UseCount        = 1;
  rec->Data()          = new CPData(*this, &InfoChangeEvent, &StatChangeEvent, rec);
  rec->URL             = obj->GetPlayable().GetURL().cdata();
  CalcCols(rec, obj->GetPlayable().EnsureInfoAsync(Playable::IF_Format|Playable::IF_Tech|Playable::IF_Meta), PlayableInstance::SF_All);
          
  rec->flRecordAttr    = 0;
  rec->hptrIcon        = CalcIcon(rec);

  RECORDINSERT insert      = { sizeof(RECORDINSERT) };
  insert.pRecordOrder      = (after ? (RECORDCORE*)after : (RECORDCORE*)CMA_FIRST);
  //insert.pRecordParent   = NULL;
  insert.fInvalidateRecord = TRUE;
  insert.zOrder            = CMA_TOP;
  insert.cRecordsInsert    = 1;
  ULONG rc = LONGFROMMR(WinSendMsg(HwndContainer, CM_INSERTRECORD, MPFROMP(rec), MPFROMP(&insert)));
  
  obj->GetPlayable().InfoChange += rec->Data()->InfoChange;
  obj->StatusChange             += rec->Data()->StatChange;

  DEBUGLOG(("PlaylistView::AddEntry: succeeded: %p %u %x\n", rec, rc, WinGetLastError(NULL)));
  return rec;
}

PlaylistView::Record* PlaylistView::MoveEntry(Record* entry, Record* after)
{ DEBUGLOG(("PlaylistView(%p{%s})::MoveEntry(%s, %s)\n", this, DebugName().cdata(),
    Record::DebugName(entry).cdata(), Record::DebugName(after).cdata()));
  TREEMOVE move =
  { (RECORDCORE*)entry,
    NULL,
    (RECORDCORE*)(after ? after : (Record*)CMA_FIRST),
    FALSE
  };
  if (WinSendMsg(HwndContainer, CM_MOVETREE, MPFROMP(&move), 0) == FALSE)
    return NULL;
  return entry;
}

void PlaylistView::RemoveEntry(Record* const entry)
{ DEBUGLOG(("PlaylistView(%p{%s})::RemoveEntry(%p)\n", this, DebugName().cdata(), entry));
  if (CmFocus == entry) // TODO: no good solution
    CmFocus = NULL;
  // detach content
  entry->Content = NULL;
  // deregister events
  entry->Data()->DeregisterEvents();
  // Remove record from container
  #ifndef NDEBUG
  assert(LONGFROMMR(WinSendMsg(HwndContainer, CM_REMOVERECORD, MPFROMP(&entry), MPFROM2SHORT(1, CMA_INVALIDATE))) != -1);
  #else
  WinSendMsg(HwndContainer, CM_REMOVERECORD, MPFROMP(&entry), MPFROM2SHORT(1, CMA_INVALIDATE));
  #endif
  // Release reference counter
  // The record will be deleted when no outstanding PostMsg is on the way.  
  FreeRecord(entry);
  DEBUGLOG(("PlaylistView::RemoveEntry completed\n"));
}

void PlaylistView::RemoveChildren()
{ DEBUGLOG(("PlaylistView(%p)::RemoveChildren()\n", this));
  Record* crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, 0, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
  while (crp != NULL && crp != (Record*)-1)
  { DEBUGLOG(("CM_QUERYRECORD: %s\n", Record::DebugName(crp).cdata()));
    Record* crp2 = crp;
    crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    RemoveEntry(crp2);
  }
}

void PlaylistView::RequestChildren()
{ DEBUGLOG(("PlaylistView(%p)::RequestChildren()\n", this));
  if ((Content->GetFlags() & Playable::Enumerable) == 0)
    return;
  // Call event either immediately or later, asynchronuously.
  InfoChangeEvent(Playable::change_args(*Content, Content->EnsureInfoAsync(Playable::IF_Other|Playable::IF_Tech)), NULL);
}

void PlaylistView::UpdateChildren()
{ DEBUGLOG(("PlaylistView(%p)::UpdateChildren() - %u\n", this, Content->GetStatus()));
  if ((Content->GetFlags() & Playable::Enumerable) == 0 || Content->GetStatus() <= STA_Invalid)
  { // Nothing to enumerate, delete children if any
    DEBUGLOG(("PlaylistView::UpdateChildren - no children possible.\n"));
    return;
  }
  
  // Check if techinfo is available, otherwise wait
  if (Content->EnsureInfoAsync(Playable::IF_Tech) == 0)
  { EvntState.WaitUpdate = true;
    return;
  }

  // First check what's currently in the container.
  Record* OldHead = NULL;
  Record* OldTail = NULL;
  { // The collection is not locked so far, but the Delete Event ensures that the parent node is removed
    // from the worker's queue before the children are deleted. So we are safe here.
    Record* crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(NULL), MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
    DEBUGLOG(("PlaylistView::UpdateChildren - %p\n", crp));
    while (crp != NULL && crp != (Record*)-1)
    { DEBUGLOG(("PlaylistView::UpdateChildren CM_QUERYRECORD: %s\n", Record::DebugName(crp).cdata()));
      if (OldTail)
        OldTail->preccNextRecord = crp;
       else
        OldHead = crp;
      OldTail = crp;
      crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    }
    if (OldTail)
      OldTail->preccNextRecord = NULL;
   
    // Now check what should be in the container
    DEBUGLOG(("PlaylistView::UpdateChildren - check container.\n"));
    WinEnableWindowUpdate(HwndContainer, FALSE); // suspend redraw
    Mutex::Lock lock(Content->Mtx); // Lock the collection
    sco_ptr<PlayableEnumerator> ep = ((PlayableCollection&)*Content).GetEnumerator();
    crp = NULL;
    while (ep->Next())
    { // Find entry in the current content
      Record** nrpp = &OldHead;
      for (;;)
      { if (*nrpp == NULL)
        { // not found! => add
          DEBUGLOG(("PlaylistView::UpdateChildren - not found: %p{%s}\n", &**ep, (*ep)->GetPlayable().GetURL().getShortName().cdata()));
          crp = AddEntry(&**ep, crp);
          break;
        }
        if ((*nrpp)->Content == &**ep)
        { // found! => move
          if (nrpp == &OldHead)
          { // already in right order
            DEBUGLOG(("PlaylistView::UpdateChildren - found: %p{%s} at HEAD\n", &**ep, (*ep)->GetPlayable().GetURL().getShortName().cdata()));
            crp = OldHead;
          } else
          { // move
            DEBUGLOG(("PlaylistView::UpdateChildren - found: %p{%s} at %p\n", &**ep, (*ep)->GetPlayable().GetURL().getShortName().cdata(), *nrpp));
            crp = MoveEntry(*nrpp, crp);
          }
          // Remove from queue
          *nrpp = (Record*)(*nrpp)->preccNextRecord;
          break;
        }
        nrpp = &(Record*&)(*nrpp)->preccNextRecord; 
      }
    }
  }

  // delete remaining records
  DEBUGLOG(("PlaylistView::UpdateChildren - OldHead = %p\n", OldHead));
  while (OldHead) // if not: delete!
  { Record* crp = OldHead;
    OldHead = (Record*)OldHead->preccNextRecord;
    RemoveEntry(crp);
  }
  // resume redraw
  WinEnableWindowUpdate(HwndContainer, TRUE);
}

void PlaylistView::UpdateRecord(Record* rec, Playable::InfoFlags flags, PlayableInstance::StatusFlags iflags)
{ DEBUGLOG(("PlaylistView(%p)::UpdateRecord(%p, %x, %x)\n", this, rec, flags, iflags));
  if (Record::IsRemoved(rec))
    return;
  // Check if UpdateChildren is waiting
  bool& wait = StateFromRec(rec).WaitUpdate;
  if (wait)
  { // Try again
    wait = false;
    PostRecordCommand(rec, RC_UPDATECHILDREN);
  }
  // Update columns of items
  if (rec && CalcCols(rec, flags, iflags))
    WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED));
}


