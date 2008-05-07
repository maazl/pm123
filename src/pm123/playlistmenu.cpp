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


#define INCL_WIN
#define INCL_BASE
#include "pm123.h" // HAB
#include "pm123.rc.h"
#include "playlistmenu.h"

#include <debuglog.h>

// Maximum number of items per submenu
#define MAX_MENU 100


int PlaylistMenu::MapEntry::compareTo(const USHORT& key) const
{ return (int)IDMenu - key;
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
  InfoDelegate(*this, &PlaylistMenu::InfoChangeCallback)
{ DEBUGLOG(("PlaylistMenu(%p)::PlaylistMenu(%x, %u,%u)\n", this, owner, mid1st, midlast));
  // Generate dialog procedure and replace the current one
  Old_DlgProc = WinSubclassWindow(owner, (PFNWP)vreplace1(&VR_DlgProc, pm_DlgProcStub, this));
  PMASSERT(Old_DlgProc != NULL);
}

PlaylistMenu::~PlaylistMenu()
{ DEBUGLOG(("PlaylistMenu(%p)::~PlaylistMenu()\n", this));
  // Deregister dialog procedure
  PMXASSERT(WinSubclassWindow(HwndOwner, Old_DlgProc), != NULL);
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
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_INITMENU(%u, %x)\n", this, SHORT1FROMMP(mp1), mp2));
      MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      if (mapp == NULL)
        break; // No registered map entry, continue default processing.
      mapp->HwndMenu = HWNDFROMMP(mp2);
      // delete old stuff
      RemoveSubItems(mapp);
      // And add new
      CreateSubItems(mapp);
      break;
    }

   case WM_MENUSELECT:
    DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_MENUSELECT(%u, %u, %x)\n", this, SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), mp2));
    break;

   case WM_COMMAND:
    if (SHORT1FROMMP(mp2) == CMDSRC_MENU)
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_COMMAND(%u)\n", this, SHORT1FROMMP(mp1)));
      MapEntry* mp = MenuMap.find(SHORT1FROMMP(mp1));
      if (mp) // ID unknown?
      { WinSendMsg(HwndOwner, UM_SELECTED, MPFROMP(&mp->Data), mp->User);
        return 0; // no further processing
    } }
    break;

   case UM_LATEUPDATE:
    if (UpdateEntry)
    { RemoveSubItems(UpdateEntry); // maybe we have to remove a dummy entry
      CreateSubItems(UpdateEntry);
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
  if (!MenuMap.binary_search(ID1stfree, pos))
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

USHORT PlaylistMenu::InsertSeparator(HWND menu, SHORT where)
{ MENUITEM mi = {0};
  mi.iPosition = where;
  mi.afStyle = MIS_SEPARATOR;
  mi.id = AllocateID();
  if (mi.id != (USHORT)MID_NONE)
    PMXASSERT(SHORT1FROMMR(WinSendMsg(menu, MM_INSERTITEM, MPFROMP(&mi), 0)), != (USHORT)MIT_ERROR);
  return mi.id;
}

void PlaylistMenu::CreateSubItems(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::CreateSubItems(%p{%u})\n", this, mapp, mapp->IDMenu));
  // is enumerable?
  if (!(mapp->Data.Item->GetPlayable()->GetFlags() & Playable::Enumerable))
    return;

  MENUITEM mi = {0};
  mi.iPosition   = MIT_END;
  //mi.hItem       = 0;
  if (mapp->Pos != (USHORT)MID_NONE)
  { mi.iPosition = SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->Pos, FALSE), 0));
  }
  size_t count = 0;
  // lock collection
  Mutex::Lock lock(mapp->Data.Item->GetPlayable()->Mtx);
  if (!mapp->Data.Item->GetPlayable()->EnsureInfoAsync(Playable::IF_Other))
  { // not immediately availabe => do it later
    ResetDelegate(mapp);
  } else
  { int_ptr<PlayableInstance> pi;
    while (count < MAX_MENU && (pi = ((PlayableCollection&)*mapp->Data.Item->GetPlayable()).GetNext(pi)))
    { // Get content
      Playable* pp = pi->GetPlayable();
      DEBUGLOG(("PlaylistMenu::CreateSubItems: at %s\n", pp->GetURL().getShortName().cdata()));
      // skip invalid?
      if ((mapp->Flags & SkipInvalid) && pp->GetStatus() <= STA_Invalid)
        continue;
      ++count;
      // fetch ID
      mi.id          = AllocateID();
      if (mi.id == (USHORT)MID_NONE)
        break; // can't help
      if (mapp->ID1 == (USHORT)MID_NONE)
        mapp->ID1 = mi.id;
      mi.afStyle     = MIS_TEXT;
      mi.afAttribute = 0;
      mi.hwndSubMenu = NULLHANDLE;
      // Invalid?
      if (pp->GetStatus() <= STA_Invalid)
      { mi.afAttribute |= MIA_DISABLED;
      // with submenu?
      } else if ((mapp->Flags & Recursive) && (pp->GetFlags() & Playable::Enumerable))
      { DEBUGLOG(("PlaylistMenu::CreateSubMenu: submenu!\n"));
        pp->EnsureInfoAsync(Playable::IF_Other); // Prefetch nested playlist content
        // Create submenu
        mi.afStyle     |= MIS_SUBMENU;
        mi.afAttribute |= MIA_DISABLED;
        mi.hwndSubMenu = WinLoadMenu(mapp->HwndMenu, NULLHANDLE, MNU_SUBFOLDER);
        PMRASSERT(WinSetWindowUShort(mi.hwndSubMenu, QWS_ID, mi.id));
        PMRASSERT(WinSetWindowBits(mi.hwndSubMenu, QWL_STYLE, MS_CONDITIONALCASCADE, MS_CONDITIONALCASCADE));
        PMRASSERT(WinSendMsg(mi.hwndSubMenu, MM_SETDEFAULTITEMID, MPFROMLONG(mi.id), 0));
      }
      // Add map entry
      MapEntry*& subp = MenuMap.get(mi.id);
      ASSERT(subp == NULL);
      subp = new MapEntry(mi.id, *pi, mapp->Flags, mapp->User, MIT_END);
      // Add menu item
      subp->Text = pi->GetDisplayName();
      if (subp->Text.length() > 30) // limit length?
        subp->Text = xstring(subp->Text, 0, 30) + "...";
      if (mapp->Flags & Enumerate) // prepend enumeration?
        subp->Text = xstring::sprintf("%s%u %s", count<10 ? "~" : "", count, subp->Text.cdata());
      SHORT rs = SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(subp->Text.cdata())));
      if (mi.iPosition != MIT_END)
        ++mi.iPosition;
      DEBUGLOG(("PlaylistMenu::CreateSubItems: new item %u->%s - %i\n", mi.id, subp->Text.cdata(), rs));
      // Separator before?
      if ((mapp->Flags & Separator) && rs != 0)
      { USHORT id = InsertSeparator(mapp->HwndMenu, rs);
        if (id != (USHORT)MID_NONE)
        { mapp->ID1 = id;
          if (mi.iPosition != MIT_END)
            ++mi.iPosition;
      } }
    }
  }
  if (count != 0)
  { // separator after
    if ((mapp->Flags & Separator) && mi.iPosition != MIT_END)
      InsertSeparator(mapp->HwndMenu, mi.iPosition);
    // enable parent
    HWND par_menu = WinQueryWindow(mapp->HwndMenu, QW_OWNER);
    DEBUGLOG(("PlaylistMenu::CreateSubMenu: enable parent %p\n", par_menu));
    WinSendMsg(par_menu, MM_SETITEMATTR, MPFROM2SHORT(WinQueryWindowUShort(par_menu, QWS_ID), FALSE), MPFROM2SHORT(MIA_DISABLED, 0));
  } else if (mapp->Flags & DummyIfEmpty)
  { // create dummy
    DEBUGLOG(("PlaylistMenu::CreateSubMenu: create dummy!\n"));
    // fetch ID
    mi.id          = AllocateID();
    if (mi.id == (USHORT)MID_NONE)
      return; // can't help
    if (mapp->ID1 == (USHORT)MID_NONE)
      mapp->ID1 = mi.id;
    mi.afStyle     = MIS_TEXT|MIS_STATIC;
    mi.hwndSubMenu = NULLHANDLE;
    // Add menu item
    PMXASSERT(SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP("- none -"))), != (USHORT)MIT_ERROR);
  }
}

void PlaylistMenu::RemoveSubItems(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveSubItems(%p{%u,%x,%u,%u})\n", this, mapp, mapp->IDMenu, mapp->HwndMenu, mapp->ID1, mapp->Pos));
  if (mapp->ID1 == (USHORT)MID_NONE || mapp->HwndMenu == NULLHANDLE)
    return; // no subitems or not yet initialized

  SHORT i = SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->ID1, FALSE), 0));
  PMASSERT(i != MIT_NONE);

  mapp->ID1 = (USHORT)MID_NONE;
  for(;;)
  { SHORT id = SHORT1FROMMR(WinSendMsg(mapp->HwndMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0));
    DEBUGLOG(("PlaylistMenu::RemoveSubItems - %i %d\n", i, id));
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
  { HWND par_menu = WinQueryWindow(mapp->HwndMenu, QW_OWNER);
    DEBUGLOG(("PlaylistMenu::RemoveMapEntry - %p\n", par_menu));
    MENUITEM mi;
    PMRASSERT(WinSendMsg(par_menu, MM_QUERYITEM, MPFROM2SHORT(mapp->IDMenu, FALSE), MPFROMP(&mi)));
    if (mi.hwndSubMenu)
      PMRASSERT(WinDestroyWindow(mi.hwndSubMenu));
  }
  // update first free ID (optimization)
  if (mapp->IDMenu < ID1stfree)
    ID1stfree = mapp->IDMenu;
  // and now game over
  delete mapp;
  DEBUGLOG(("PlaylistMenu::RemoveMapEntry done\n"));
}
void PlaylistMenu::RemoveMapEntry(USHORT mid)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveMapEntry(%u)\n", this, mid));
  MapEntry* mapp = MenuMap.erase(mid);
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
  mapp->Data.Item->GetPlayable()->InfoChange += InfoDelegate;
}

void PlaylistMenu::InfoChangeCallback(const Playable::change_args& args)
{ DEBUGLOG(("PlaylistMenu(%p)::InfoChangeCallback({%p,%x})\n", this, &args.Instance, args.Flags));
  if ((args.Flags & Playable::IF_Other) == 0)
    return; // not the right event
  InfoDelegate.detach(); // Only catch the first event
  if (UpdateEntry == NULL || &args.Instance != UpdateEntry->Data.Item->GetPlayable())
    return; // event obsolete
  QMSG msg;
  if (!WinPeekMsg(amp_player_hab(), &msg, HwndOwner, UM_LATEUPDATE, UM_LATEUPDATE, PM_NOREMOVE))
    PMRASSERT(WinPostMsg(HwndOwner, UM_LATEUPDATE, 0, 0));
}

bool PlaylistMenu::AttachMenu(USHORT menuid, Playable* data, EntryFlags flags, MPARAM user, USHORT pos)
{ DEBUGLOG(("PlaylistMenu(%p)::AttachMenu(%u, %p{%s}, %x, %p, %u)\n", this, menuid, data, data->GetURL().getShortName().cdata(), flags, user, pos));

  MapEntry*& mapp = MenuMap.get(menuid);
  if (mapp)
    // existing item => replace data
    mapp->Data = data;
   else
    // new map item
    mapp = new MapEntry(menuid, data, flags, user, pos);

  return true;
}


