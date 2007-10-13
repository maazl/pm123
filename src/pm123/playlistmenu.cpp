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


#define INCL_WIN
#define INCL_BASE
#include "pm123.h" // HAB
#include "pm123.rc.h"
#include "playlistmenu.h"

#include <assert.h>

#include <debuglog.h>

// Maximum number of items per submenu
#define MAX_MENU 100 


int PlaylistMenu::MapEntry::CompareTo(const USHORT* key) const
{ return (int)IDMenu - *key;
}

MRESULT EXPENTRY pm_DlgProcStub(PlaylistMenu* that, ULONG msg, MPARAM mp1, MPARAM mp2)
{ return that->DlgProc(msg, mp1, mp2);
}

PlaylistMenu::PlaylistMenu(HWND owner, USHORT mid1st, USHORT midlast)
: HwndOwner(owner),
  ID1st(mid1st),
  IDlast(midlast),
  MenuMap(50),
  ID1stfree(mid1st),
  InfoDelegate(*this, &InfoChangeCallback)
{ DEBUGLOG(("PlaylistMenu(%p)::PlaylistMenu(%x, %u,%u)\n", this, owner, mid1st, midlast));
  // Generate dialog procedure and replace the current one
  Old_DlgProc = WinSubclassWindow(owner, (PFNWP)mkvreplace1(&VR_DlgProc, pm_DlgProcStub, this));
  assert(Old_DlgProc != NULL);
}

PlaylistMenu::~PlaylistMenu()
{ DEBUGLOG(("PlaylistMenu(%p)::~PlaylistMenu()\n", this));
  // Deregister dialog procedure
  WinSubclassWindow(HwndOwner, Old_DlgProc);
  // Destroy menu map
  while (MenuMap.size())
    RemoveMapEntry(MenuMap.erase(MenuMap.size()-1));
}

MRESULT PlaylistMenu::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_DESTROY:
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_DESTROY\n", this));
      PFNWP old_DlgProc = Old_DlgProc; // copy value to the stack
      delete this; // uh! Now we must not touch *this anymore.
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_DESTROY - after delete\n", this));
      return (*old_DlgProc)(HwndOwner, msg, mp1, mp2);
    }
 
   case WM_INITMENU:
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_INITMENU(%u, %x)\n", this, mp1, mp2));
      MapEntry* mapp = MenuMap.find(&(USHORT&)SHORT1FROMMP(mp1));
      if (mapp == NULL)
        break; // No registered map entry, continue default processing.
      mapp->HwndMenu = HWNDFROMMP(mp2);
      // delete old stuff
      RemoveSubItems(mapp);
      // And add new
      CreateSubItems(mapp);
      break;
    }

   case UM_LATEUPDATE:
    if (UpdateEntry)
    { CreateSubItems(UpdateEntry);
      UpdateEntry = NULL;
    }
    break;
  }
  // Call chained dialog procedure
  return (*Old_DlgProc)(HwndOwner, msg, mp1, mp2);
}

USHORT PlaylistMenu::AllocateID()
{ DEBUGLOG(("PlaylistMenu(%p)::AllocateID()\n", this)); 
  size_t pos;
  if (!MenuMap.binary_search(&ID1stfree, pos))
    return ID1stfree++; // immediate match
  // search the next free ID from here
  while (++ID1stfree <= IDlast)
  { USHORT id;
    if (++pos >= MenuMap.size() || (id = MenuMap[pos]->IDMenu) > ID1stfree)
      return ID1stfree++; // ID found
    ID1stfree = id; 
  }
  // no more IDs
  DEBUGLOG(("PlaylistMenu::AllocateID: ERROR: OUT OF MENU IDs.\n"));
  return (USHORT)MID_NONE;
}

void PlaylistMenu::CreateSubItems(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::CreateSubItems(%p{%u})\n", this, mapp, mapp->IDMenu));
  // is enumerable?
  if (!(mapp->Data->GetFlags() & Playable::Enumerable))
    return;

  Mutex::Lock lock(mapp->Data->Mtx); // lock collection
  if (!mapp->Data->EnsureInfoAsync(Playable::IF_Other))
  { // not immediately availabe => do it later
    ResetDelegate(mapp);
    return;
  }
  MENUITEM mi;
  mi.iPosition   = MIT_END;
  mi.afAttribute = 0;
  mi.hItem       = 0;
  if (mapp->Pos != (USHORT)MID_NONE)
  { mi.iPosition = SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->Pos, FALSE), 0));
  }
  size_t count = 0;
  sco_ptr<PlayableEnumerator> pe = ((PlayableCollection&)*mapp->Data).GetEnumerator();
  while (++count <= MAX_MENU && pe->Next())
  { USHORT id = AllocateID();
    if (id == (USHORT)MID_NONE)
      break; // can't help
    if (mapp->ID1 == (USHORT)MID_NONE)
      mapp->ID1 = id;
    mi.id          = id;
    mi.afStyle = MIS_TEXT;
    mi.hwndSubMenu = NULLHANDLE;
    // Get content
    Playable* pp = &(*pe)->GetPlayable();
    if (pp->GetFlags() & Playable::Enumerable)
    { pp->EnsureInfoAsync(Playable::IF_Other); // Prefetch nested playlist content
      // Create submenu
      mi.afStyle |= MIS_SUBMENU;
      mi.hwndSubMenu = WinLoadMenu(mapp->HwndMenu, NULLHANDLE, MNU_EMPTY);
      WinSetWindowUShort(mi.hwndSubMenu, QWS_ID, id);
      WinSetWindowBits(mi.hwndSubMenu, QWL_STYLE, MS_CONDITIONALCASCADE, MS_CONDITIONALCASCADE);
    }
    // Add map entry
    MapEntry*& subp = MenuMap.get(&id);
    assert(subp == NULL);
    subp = new MapEntry(id, pp, MIT_END);
    // Add menu item
    subp->Text = (*pe)->GetDisplayName();
    WinSendMsg(mapp->HwndMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(subp->Text.cdata()));
    if (mi.iPosition != MIT_END)
      ++mi.iPosition;
  }
}

void PlaylistMenu::RemoveSubItems(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveSubItems(%p{%u,%x,%u})\n", this, mapp, mapp->IDMenu, mapp->HwndMenu, mapp->ID1)); 
  if (mapp->ID1 == (USHORT)MID_NONE || mapp->HwndMenu == NULLHANDLE)
    return; // no subitems or not yet initialized
  
  SHORT i = SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->ID1, FALSE), 0));
  assert(i != MIT_NONE);

  mapp->ID1 = (USHORT)MID_NONE;
  for(;;)
  { USHORT id = SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0));
    DEBUGLOG(("PlaylistMenu::RemoveSubItems - %i %u\n", i, id));
    if (id == MIT_ERROR || id == 0 || id == mapp->Pos)
      return; // end of menu or range
    RemoveMapEntry(id);
    WinSendMsg(mapp->HwndMenu, MM_DELETEITEM, MPFROM2SHORT(id, FALSE), 0);
  }
}

void PlaylistMenu::RemoveMapEntry(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveMapEntry(%p{%u,%x})\n", this, mapp, mapp->IDMenu, mapp->HwndMenu));
  // deregister event if it's mine
  if (UpdateEntry == mapp)
    ResetDelegate();
  // delete children recursively
  RemoveSubItems(mapp);
  // now destroy the submenu if any
  if (mapp->HwndMenu) // only if menu has been shown already
  { HWND par_menu = WinQueryWindow(mapp->HwndMenu, QW_OWNER); // For some reason QW_PARENT will not work.
    DEBUGLOG(("PlaylistMenu::RemoveMapEntry - %p\n", par_menu)); 
    MENUITEM mi;
    #ifndef NDEBUG
    assert(WinSendMsg(par_menu, MM_QUERYITEM, MPFROM2SHORT(mapp->IDMenu, FALSE), MPFROMP(&mi)));
    #else
    WinSendMsg(par_menu, MM_QUERYITEM, MPFROM2SHORT(mapp->IDMenu, FALSE), MPFROMP(&mi));
    #endif
    if (mi.hwndSubMenu)
      WinDestroyWindow(mi.hwndSubMenu);
  }
  // update first free ID (optimization) 
  if (mapp->IDMenu < ID1stfree)
    ID1stfree = mapp->IDMenu;
  // and now game over
  delete mapp;
}
void PlaylistMenu::RemoveMapEntry(USHORT mid)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveMapEntry(%u)\n", this, mid)); 
  MapEntry* mapp = MenuMap.erase(&mid);
  if (mapp)
    RemoveMapEntry(mapp);
}

void PlaylistMenu::ResetDelegate()
{ DEBUGLOG(("PlaylistMenu(%p)::ResetDelegate()\n", this)); 
  InfoDelegate.detach();
  UpdateEntry = NULL;
  QMSG msg;
  WinPeekMsg(amp_player_hab(), &msg, HwndOwner, UM_LATEUPDATE, UM_LATEUPDATE, PM_REMOVE);
}
void PlaylistMenu::ResetDelegate(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::ResetDelegate(%p)\n", this, mapp)); 
  ResetDelegate();
  UpdateEntry = mapp;
  mapp->Data->InfoChange += InfoDelegate;
}

void PlaylistMenu::InfoChangeCallback(const Playable::change_args& args)
{ DEBUGLOG(("PlaylistMenu(%p)::InfoChangeCallback({%p,%x})\n", this, args.Instance, args.Flags)); 
  if ((args.Flags & Playable::IF_Other) == 0)
    return; // not the right event
  InfoDelegate.detach(); // Only catch the first event
  if (UpdateEntry == NULL || &args.Instance != &*UpdateEntry->Data)
    return; // event obsolete
  QMSG msg;
  if (!WinPeekMsg(amp_player_hab(), &msg, HwndOwner, UM_LATEUPDATE, UM_LATEUPDATE, PM_NOREMOVE)) 
    WinPostMsg(HwndOwner, UM_LATEUPDATE, 0, 0);
}

bool PlaylistMenu::AttachMenu(USHORT menuid, int_ptr<Playable> data, USHORT pos)
{ DEBUGLOG(("PlaylistMenu(%p)::AttachMenu(%u, %p{%s}, %u)\n", this, menuid, &*data, data->GetURL().getObjName().cdata(), pos)); 

  MapEntry*& mapp = MenuMap.get(&menuid);
  if (mapp)
    // existing item => replace data
    mapp->Data = data;
   else
    // new map item
    mapp = new MapEntry(menuid, data, pos);

  return true;
}


