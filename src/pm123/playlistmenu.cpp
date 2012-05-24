/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2007-2011 Marcel Mueller
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
#include "dependencyinfo.h"
#include "location.h"
#include <cpp/cppvdelegate.h>
#include <utilfct.h>

#include <debuglog.h>

// Maximum number of items per submenu
#define DEF_MAX_MENU 100


inline PlaylistMenu::MapEntry::MapEntry(USHORT id, MapEntry* parent, APlayable& data, EntryFlags flags, MPARAM user, SHORT pos, PlaylistMenu& owner, void (PlaylistMenu::*infochg)(const PlayableChangeArgs&, MapEntry*))
: IDMenu(id),
  Flags(flags),
  Parent(parent),
  HwndSub(NULLHANDLE),
  Data(&data),
  Status(0),
  User(user),
  ID1((USHORT)MID_NONE),
  Pos(pos),
  InfoDelegate(data.GetInfoChange(), owner, infochg, this)
{}

int PlaylistMenu::MapEntry::compare(const USHORT& key, const MapEntry& entry)
{ return (int)key - (int)entry.IDMenu;
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
  MaxMenu(DEF_MAX_MENU)
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

/* Sub menu state engine (u = in use, D = in destroy, U = in update)
 *
 * Event           | State | Condition          | Action                  | State
 * ================|=======|====================|=========================|=======
 * create item     | n/a   | playlist item      | create empty sub-menu   | . . .
 * ----------------+-------+--------------------+-------------------------+-------
 * UM_UPDATESTAT   | . ? ? |                    | no-op                   | . ? ?
 * ----------------+-------+--------------------+-------------------------+-------
 * UM_UPDATESTAT   | u ? ? | no playlist item   | destroy sub menu if any | u ? ?
 * ----------------+-------+--------------------+-------------------------+-------
 * UM_UPDATESTAT   | u ? ? | playlist item      | ensure sub menu         | u ? ?
 * ----------------+-------+--------------------+-------------------------+-------
 * WM_INITMENU     | . . . | children not       | ensure sub menu         | u . .
 *                 |       |  available         |  create dummy entry     |
 * ----------------+-------+--------------------+-------------------------+-------
 * WM_INITMENU     | . . . | children available | ensure sub menu         | u . .
 *                 |       |                    |  update sub items       |
 * ----------------+-------+--------------------+-------------------------+-------
 * UM_UPDATELIST   | . ? ? |                    | no-op                   | . ? ?
 * ----------------+-------+--------------------+-------------------------+-------
 * UM_UPDATELIST   | u . . |                    | no-op                   | . ? ?
 * ----------------+-------+--------------------+-------------------------+-------
 * WM_MENUEND      | u . . |                    | post UM_MENUEND         | . . .
 * ----------------+-------+--------------------+-------------------------+-------
 * UM_MENUEND      | u ? ? |                    | no-op                   | u ? ?
 * ----------------+-------+--------------------+-------------------------+-------
 * UM_MENUEND      | . . . |                    | begin destroy sub items | . D .
 *                 | u D . |                    | cancel operation        | u . .
 *                 |       | children available |  post UM_UPDATE         |
 * ----------------+-------+--------------------+-------------------------+-------
 */

MRESULT PlaylistMenu::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistMenu(%p{%x})::DlgProc(%x, %x, %x)\n", this, HwndOwner, msg, mp1, mp2));
  switch (msg)
  {case WM_DESTROY:
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_DESTROY\n", this));
      PFNWP old_DlgProc = Old_DlgProc; // copy value to the stack
      HWND hwndOwner = HwndOwner;
      delete this; // uh! Now we must not touch *this anymore.
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_DESTROY - after delete\n", this));
      return (*old_DlgProc)(hwndOwner, msg, mp1, mp2);
    }

   case WM_INITMENU:
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_INITMENU(%u, %x)\n", this, SHORT1FROMMP(mp1), mp2));
      MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      if (mapp == NULL)
        break; // No registered map entry, continue default processing.
      DEBUGLOG(("PlaylistMenu::DlgProc: WM_INITMENU: %p{%u, %p, %x,...}\n", mapp,
          mapp->IDMenu, mapp->Parent, mapp->HwndSub));
      ASSERT(mapp->HwndSub == HWNDFROMMP(mp2));
      mapp->Status |= InUse;
      if ((mapp->Status & (InUpdate|InDestroy)) == 0)
        UpdateSubItems(mapp);
      else if (!mapp->Status.bitset(1))
        PMRASSERT(WinPostMsg(HwndOwner, UM_UPDATELIST, mp1, MPFROMP(mapp)));
      //menutest(HWNDFROMMP(mp2));
      break;
    }

   case WM_MENUEND:
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_MENUEND(%u, %x)\n", this, SHORT1FROMMP(mp1), mp2));
      MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      if (mapp) // registered map entry?
      { mapp->Status &= ~InUse;
        //menutest(HWNDFROMMP(mp2));
        // delete old stuff later
        PMRASSERT(WinPostMsg(HwndOwner, UM_MENUEND, mp1, MPFROMP(mapp)));
      }
      break;
    }

   #if 0
   case WM_MENUSELECT:
    DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_MENUSELECT(%u, %u, %x)\n", this, SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), mp2));
    break;
   #endif

   case WM_COMMAND:
    if (SHORT1FROMMP(mp2) == CMDSRC_MENU)
    { MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_COMMAND(%u) - %p\n", this, SHORT1FROMMP(mp1), mapp));
      if (mapp) // ID unknown?
      { SelectEntry(mapp);
        return 0; // no further processing
    } }
    break;

   case UM_MENUEND:
    { MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: UM_MENUEND(%u, %p) - %p\n",
        this, SHORT1FROMMP(mp1), PVOIDFROMMP(mp2), mapp));
      // delete old stuff, but only if still valid.
      if (mapp == PVOIDFROMMP(mp2) && !(mapp->Status & InUse))
        RemoveSubItems(mapp);
    }
    return 0;

   case UM_UPDATEITEM:
    { MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: UM_UPDATEITEM(%u, %p) - %p\n",
        this, SHORT1FROMMP(mp1), PVOIDFROMMP(mp2), mapp));
      // ID known and valid and not root?
      if (mapp == PVOIDFROMMP(mp2) && mapp->Parent && mapp->Status.bitrst(0))
        UpdateItem(mapp);
    }
    break;

   case UM_UPDATELIST:
    { MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: UM_UPDATELIST(%u, %p) - %p\n",
        this, SHORT1FROMMP(mp1), PVOIDFROMMP(mp2), mapp));
      // Update content, but only for visible items.
      if (mapp == PVOIDFROMMP(mp2) && (mapp->Status & InUse) && mapp->Status.bitrst(1))
        UpdateSubItems(mapp);
    }
    break;
  }
  // Call chained dialog procedure
  return (*Old_DlgProc)(HwndOwner, msg, mp1, mp2);
}

void PlaylistMenu::SelectEntry(const MapEntry* mapp) const
{
  //MPARAM user = mapp->User;
  vector<PlayableInstance> callstack;
  // find root and collect call stack
  for (;;)
  { const MapEntry* parent = mapp->Parent;
    if (parent == NULL)
      break;
    callstack.append() = (PlayableInstance*)mapp->Data.get();
    mapp = parent;
  }
  // Now mapp points to the attached root and callstack
  // contains all sub items within this root.

  // Create location object
  JobSet noblockjob(PRI_None);
  MenuCtrlData loc(mapp->Data->GetPlayable(), mapp->User);
  for (size_t i = callstack.size(); i--; )
  { if (loc.Item.NavigateInto(noblockjob))
    { DEBUGLOG(("PlaylistMenu::SelectEntry -> NavigateInto failed."));
      return;
    }
    if (loc.Item.NavigateTo(callstack[i]))
    { DEBUGLOG(("PlaylistMenu::SelectEntry -> NavigateTo failed."));
      return;
    }
  }
  callstack.clear();
  DEBUGLOG(("PlaylistMenu(%p)::SelectEntry: {%s, %p}\n", this, loc.Item.Serialize().cdata(), loc.User));
  WinSendMsg(HwndOwner, WM_CONTROL, MPFROMSHORT(mapp->IDMenu), MPFROMP(&loc));
}

USHORT PlaylistMenu::AllocateID()
{ DEBUGLOG(("PlaylistMenu(%p)::AllocateID()\n", this));
  // Start searching after ID1stfree. This is an optimization.
  size_t pos;
  if (!MenuMap.binary_search(ID1stfree, pos))
  { DEBUGLOG(("PlaylistMenu::AllocateID: %u\n", ID1stfree));
    return ID1stfree++; // immediate match
  }
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

HWND PlaylistMenu::CreateSubMenu(MENUITEM& mi, HWND parent)
{ DEBUGLOG(("PlaylistMenu::CreateSubMenu({%u,...}, %x)\n", mi.id, parent));
  mi.afStyle |= MIS_SUBMENU;
  mi.hwndSubMenu = WinLoadMenu(parent, NULLHANDLE, MNU_SUBFOLDER);
  PMRASSERT(WinSetParent(mi.hwndSubMenu, HWND_OBJECT, FALSE));
  PMASSERT(mi.hwndSubMenu);
  PMRASSERT(WinSetWindowUShort(mi.hwndSubMenu, QWS_ID, mi.id));
  PMRASSERT(WinSetWindowBits(mi.hwndSubMenu, QWL_STYLE, MS_CONDITIONALCASCADE, MS_CONDITIONALCASCADE));
  PMRASSERT(WinSendMsg(mi.hwndSubMenu, MM_SETDEFAULTITEMID, MPFROMLONG(mi.id), 0));
  return mi.hwndSubMenu;
}

USHORT PlaylistMenu::InsertSeparator(HWND menu, SHORT where)
{ DEBUGLOG(("PlaylistMenu::InsertSeparator(%x, %i)\n", menu, where));
  MENUITEM mi = {0};
  mi.iPosition = where;
  mi.afStyle = MIS_SEPARATOR;
  mi.id = AllocateID();
  if (mi.id != (USHORT)MID_NONE)
    PMXASSERT(SHORT1FROMMR(WinSendMsg(menu, MM_INSERTITEM, MPFROMP(&mi), 0)), != (USHORT)MIT_ERROR);
  return mi.id;
}

USHORT PlaylistMenu::InsertDummy(HWND menu, SHORT where, const char* text)
{ DEBUGLOG(("PlaylistMenu::InsertDummy(%x, %i, %s)\n", menu, where, text));
  MENUITEM mi = {0};
  mi.iPosition = where;
  mi.afStyle = MIS_TEXT|MIS_STATIC;
  mi.id = AllocateID();
  if (mi.id != (USHORT)MID_NONE)
    PMXASSERT(SHORT1FROMMR(WinSendMsg(menu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(text))), != (USHORT)MIT_ERROR);
  return mi.id;
}

PlaylistMenu::MapEntry* PlaylistMenu::InsertEntry(MapEntry* parent, SHORT where, APlayable& data, size_t index)
{ DEBUGLOG(("PlaylistMenu::InsertEntry(%p{%u, %x}, %i, &%p{%s}, %i)\n", parent, parent->IDMenu, parent->HwndSub, where, &data, data.DebugName().cdata(), index));
  MENUITEM mi = {0};
  mi.id          = AllocateID();
  mi.iPosition   = where;
  if (mi.id == (USHORT)MID_NONE)
    return NULL; // can't help
  if (parent->ID1 == (USHORT)MID_NONE)
    parent->ID1 = mi.id;
  mi.afStyle     = MIS_TEXT;
  mi.afAttribute = 0;
  mi.hwndSubMenu = NULLHANDLE;
  // Add map entry.
  MapEntry*& subp = MenuMap.get(mi.id);
  ASSERT(subp == NULL);
  subp = new MapEntry(mi.id, parent, data, parent->Flags, parent->User, MIT_END, *this, &PlaylistMenu::InfoChangeHandler);

  const unsigned tattr = data.GetInfo().tech->attributes;
  const bool invalid = (tattr & TATTR_INVALID) != 0;
  DEBUGLOG(("PlaylistMenu::InsertEntry: %i\n", mi.id));
  mi.afAttribute = MIA_CHECKED*CheckInUse(subp);
  // Invalid?
  if (invalid)
    mi.afAttribute |= MIA_DISABLED;
  // with submenu?
  else if ((parent->Flags & Recursive) && (tattr & TATTR_PLAYLIST))
  { subp->HwndSub = CreateSubMenu(mi, parent->HwndSub);
    // OS/2 does not call WM_INITMENU for sub menus with no children, so insert a dummy.
    // (Note that the real content is added later.)
    subp->ID1 = InsertDummy(subp->HwndSub, subp->Pos, "- loading -");
  }
  // Add menu item
  const xstring& text = MakeMenuItemText(subp, index);
  SHORT rs = SHORT1FROMMR(WinSendMsg(parent->HwndSub, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(text.cdata())));
  DEBUGLOG(("PlaylistMenu::InsertEntry: new item %s - %i\n", text.cdata(), rs));
  return subp;
}

void PlaylistMenu::UpdateItem(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu::UpdateItem(%p{%u, %x, %p{%s}})\n",
    mapp, mapp->IDMenu, mapp->HwndSub, mapp->Data.get(), mapp->Data->DebugName().cdata()));
  MapEntry* parp = mapp->Parent;
  ASSERT(parp);

  mapp->Status |= InUpdate;

  APlayable& p = *mapp->Data;
  const unsigned tattr = p.GetInfo().tech->attributes;
  const bool invalid = (p.GetInfo().phys->attributes & PATTR_INVALID) || (tattr & TATTR_INVALID);
  // remove invalid?
  if (invalid && (mapp->Flags & SkipInvalid))
  { RemoveMapEntry(mapp);
    mapp->Status &= ~InUpdate;
    return;
  }
  PMRASSERT(WinSendMsg( parp->HwndSub, MM_SETITEMATTR,
    MPFROM2SHORT(mapp->IDMenu, FALSE),
    MPFROM2SHORT(MIA_DISABLED|MIA_CHECKED, MIA_DISABLED*invalid | MIA_CHECKED*CheckInUse(mapp)) ));
  MENUITEM mi;
  PMRASSERT(WinSendMsg(parp->HwndSub, MM_QUERYITEM, MPFROM2SHORT(mapp->IDMenu, FALSE), MPFROMP(&mi)));
  DEBUGLOG(("PlaylistMenu::UpdateItem: %x - %p\n", tattr, mi.hwndSubMenu));
  if (!invalid && (mapp->Flags & Recursive) && (tattr & TATTR_PLAYLIST)) // with submenu?
  { if (mi.hwndSubMenu == NULLHANDLE)
    { mapp->HwndSub = CreateSubMenu(mi, parp->HwndSub);
      // OS/2 does not call WM_INITMENU for sub menus with no children, so insert a dummy.
      // (Note that the real content is added later.)
      mapp->ID1 = InsertDummy(mapp->HwndSub, mapp->Pos, "- loading -");
      PMRASSERT(WinSendMsg(parp->HwndSub, MM_SETITEM, MPFROM2SHORT(0, FALSE), MPFROMP(&mi)));
    }
  } else
  { // No Playlist
    if (mi.hwndSubMenu != NULLHANDLE)
    { RemoveSubItems(mapp);
      mi.afStyle &= ~MIS_SUBMENU;
      HWND oldmenu = mi.hwndSubMenu;
      mi.hwndSubMenu = NULLHANDLE;
      PMRASSERT(WinSendMsg(parp->HwndSub, MM_SETITEM, MPFROM2SHORT(0, FALSE), MPFROMP(&mi)));
      if (oldmenu)
        PMRASSERT(WinDestroyWindow(oldmenu));
    }
  }

  // Update Text
  // Get item index in the menu.
  SHORT ix = SHORT1FROMMR(WinSendMsg(parp->HwndSub, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->IDMenu, FALSE), 0));
  SHORT ix1 = SHORT1FROMMR(WinSendMsg(parp->HwndSub, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(parp->ID1, FALSE), 0));
  size_t pos = ix == MIT_NONE || ix1 == MIT_NONE ? (size_t)-1 : ix - ix1;
  const xstring& text = MakeMenuItemText(mapp, pos);
  PMRASSERT(WinSendMsg(parp->HwndSub, MM_SETITEMTEXT, MPFROMSHORT(mapp->IDMenu), MPFROMP(text.cdata())));

  mapp->Status &= ~InUpdate;
}

void PlaylistMenu::UpdateSubItems(MapEntry* const mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::UpdateSubItems(%p{%u,%x,%p{%s},%x, %i,%i})\n", this,
    mapp, mapp->IDMenu, mapp->Flags, &mapp->Data->GetPlayable(), mapp->Data->DebugName().cdata(),
    mapp->Status.get(), (SHORT)mapp->ID1, (SHORT)mapp->Pos));
  // is enumerable?
  if (!(mapp->Data->GetInfo().tech->attributes & TATTR_PLAYLIST))
    return;
  mapp->Status |= InUpdate;

  const char* dummy = NULL; // Dummy Entry (if any)
  vector_int<PlayableInstance> children; // New menu content

  if (mapp->Data->RequestInfo(IF_Child|IF_Obj, PRI_Normal))
    // not immediately available => do it later
    dummy = "- loading -";
  else
  { // copy collection synchronized into vector
    { Playable& list = mapp->Data->GetPlayable();
      int_ptr<PlayableInstance> pi;
      Mutex::Lock lock(list.Mtx);
      size_t count = list.GetInfo().obj->num_items;
      if (count > MaxMenu)
        count = MaxMenu;
      children.reserve(count);
      while (count-- && (pi = list.GetNext(pi)))
        children.append() = pi;
    }
    // now unsynchronized loop
    for (size_t i = children.size(); i-- != 0;)
    { PlayableInstance* pi = children[i];
      // Prefetch nested playlist content?
      if (mapp->Flags & Prefetch)
        pi->RequestInfo(IF_Child, PRI_Low);
      pi->RequestInfo(IF_Tech|IF_Display, PRI_Normal);
      // remove invalid items?
      if ((mapp->Flags & SkipInvalid) && (pi->GetInfo().tech->attributes & TATTR_INVALID))
        children.erase(i);
    }
    DEBUGLOG(("PlaylistMenu::UpdateSubItems: Found %u children\n", children.size()));
    // Empty Menu?
    if (children.size() == 0)
    { if (!(mapp->Flags & DummyIfEmpty))
      { RemoveSubItems(mapp);
        mapp->Status &= ~InUpdate;
        return;
      }
      dummy = "- none -";
    }
  }
  // At this point we are sure that we need at least a dummy menu entry.

  // Find position and id of first item, MIT_END/MID_NONE if none.
  int position1 = MIT_END;
  USHORT id = mapp->ID1;
  if (id == (USHORT)MID_NONE)
    id = mapp->Pos;
  if (id != (USHORT)MID_NONE)
    position1 = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(id, FALSE), 0));
  DEBUGLOG(("PlaylistMenu::UpdateSubItems: Start at %i/%i, dummy = %s\n", (SHORT)position1, (SHORT)id, dummy));

  // Create list of menu entries in the current sub menu that belong to PlaylistMenu,
  // create separators and dummy (if any). The List does not include any separators,
  // but it will include empty slots for removed items. This ensures that the index
  // in the list stays valid.
  vector<MapEntry> menuentries;
  // Stop if either
  // - Error
  // - End of menu
  // - End of user range handled by this class
  // Loop over current menu entries
  int position = position1;
  while (id != (USHORT)MIT_NONE && id != (USHORT)MIT_ERROR && id != mapp->Pos)
  { if (mapp->ID1 == (USHORT)MID_NONE)
      mapp->ID1 = id;
    MapEntry* mapp2 = MenuMap.find(id);
    DEBUGLOG(("PlaylistMenu::UpdateSubItems: Scan menu item %i/%i -> %p\n", (SHORT)position, (SHORT)id, mapp2));
    if (mapp2 == NULL)
    { // separator or dummy
      MENUITEM mi;
      PMRASSERT(WinSendMsg(mapp->HwndSub, MM_QUERYITEM, MPFROM2SHORT(id, FALSE), MPFROMP(&mi)));
      DEBUGLOG(("PlaylistMenu::UpdateSubItems: Scan menu item style %x\n", mi.afStyle));
      if (mi.afStyle & MIS_SEPARATOR)
      { // Separator
        if (position != MIT_END)
          ++position;
        goto next;
      } else if (dummy)
      { // Dummy and dummy still required
        PMRASSERT(WinSendMsg(mapp->HwndSub, MM_SETITEMTEXT, MPFROMSHORT(id), MPFROMP(dummy)));
        mapp->Status &= ~InUpdate;
        return; // This is necessarily the last action, because dummy entries and normal entries are mutually exclusive.
      } else
        // Dummy and no dummy required
        goto del;
    }
    // is MapEntry!
    // Check if needed any longer
    for (size_t i = 0; i < children.size(); ++i)
      if (children[i] == mapp2->Data)
      { // needed
        DEBUGLOG(("PlaylistMenu::UpdateSubItems: Menu item %u used at %u\n", menuentries.size(), i));
        menuentries.append() = mapp2;
        if (position != MIT_END)
          ++position;
        goto next;
      }
    // no longer needed => delete
    DEBUGLOG(("PlaylistMenu::UpdateSubItems: Menu item %u orphaned\n", menuentries.size()));
    RemoveMapEntry(id);
    menuentries.append(); // Append dummy for removed entry
   del:
    WinSendMsg(mapp->HwndSub, MM_DELETEITEM, MPFROM2SHORT(id, FALSE), 0);
    mapp->ID1 = (USHORT)MID_NONE; // The ID is assigned later if there are more entries.
   next:
    id = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_ITEMIDFROMPOSITION, MPFROMSHORT(position), 0));
    DEBUGLOG(("PlaylistMenu::UpdateSubItems: Scan at %i/%i\n", (SHORT)position, (SHORT)id));
  }

  // Create separators if required
  if (mapp->ID1 == (USHORT)MID_NONE && (mapp->Flags & Separator))
  { if (position > 0)
    { mapp->ID1 = InsertSeparator(mapp->HwndSub, position);
      if (position != MIT_END)
        ++position;
    }
    if (mapp->Pos != (USHORT)MIT_END)
    { id = InsertSeparator(mapp->HwndSub, position);
      if (mapp->ID1 == (USHORT)MID_NONE)
        mapp->ID1 = id;
    }
  }

  // If a dummy is already in place, we won't get here.
  if (dummy)
  { id = InsertDummy(mapp->HwndSub, position, dummy);
    if (mapp->ID1 == (USHORT)MID_NONE)
      mapp->ID1 = id;
    mapp->Status &= ~InUpdate;
    return;
  }

  // Now loop over new menu entries and adjust the menu content.
  // Because of the loop above we can be sure that no menu entry has to be deleted anymore.
  // But we might need new entries.
  position = position1;
  size_t jnext = 0;
  for (size_t i = 0; i < children.size(); ++i)
  { PlayableInstance& pi = *children[i];
    DEBUGLOG(("PlaylistMenu::UpdateSubItems: Update at %i->%p, %u\n", i, &pi, jnext));
    // Seek for existing menu entry.
    for (size_t j = jnext; j < menuentries.size(); ++j)
    { MapEntry* mapp2 = menuentries[j];
      // Adjust jmin to the next existing and not moved entry in
      if (mapp2 == NULL)
      { if (j == jnext)
        { DEBUGLOG(("PlaylistMenu::UpdateSubItems: Update jnext\n"));
          ++jnext;
        }
      } else if (mapp2->Data == &pi)
      { // Match!
        DEBUGLOG(("PlaylistMenu::UpdateSubItems: Update match at %u, %u\n", j, jnext));
        if (j != jnext)
        { // Move required
          MENUITEM mi = {0};
          PMRASSERT(WinSendMsg(mapp->HwndSub, MM_QUERYITEM, MPFROM2SHORT(mapp2->IDMenu, FALSE), MPFROMP(&mi)));
          ASSERT(mi.id == mapp2->IDMenu);
          WinSendMsg(mapp->HwndSub, MM_REMOVEITEM, MPFROM2SHORT(mapp2->IDMenu, FALSE), 0);
          mi.iPosition = position;
          const xstring& text = MakeMenuItemText(mapp2, i);
          SHORT rs = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(text.cdata())));
          PMASSERT(rs >= 0);
          if (position == 0 || (i == 0 && !(mapp->Flags & Separator)))
            mapp->ID1 = mapp2->IDMenu;
        } else
        { if (i != j && (mapp->Flags & Enumerate))
          { // Rename required
            const xstring& text = MakeMenuItemText(mapp2, i);
            PMRASSERT(WinSendMsg(mapp->HwndSub, MM_SETITEMTEXT, MPFROMSHORT(mapp2->IDMenu), MPFROMP(text.cdata())));
          } // else: Match with matching position
          ++jnext;
        }
        menuentries[j] = NULL;
        if (position != MIT_END)
          ++position;
        goto nextmenu;
      }
    }
    DEBUGLOG(("PlaylistMenu::UpdateSubItems: Update no match\n"));
    // No-match => insert
    InsertEntry(mapp, position, pi, i);
    if (position != MIT_END)
      ++position;
   nextmenu:;
  }
  mapp->Status &= ~InUpdate;
}

void PlaylistMenu::RemoveSubItems(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveSubItems(%p{%u,%x,%x,%u,%u})\n", this,
    mapp, mapp->Flags, mapp->IDMenu, mapp->HwndSub, mapp->ID1, mapp->Pos));
  if (mapp->ID1 == (USHORT)MID_NONE || (mapp->Status & InDestroy))
    return; // no sub items, not yet initialized or already in destroy
  mapp->Status |= InDestroy;

  SHORT i = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->ID1, FALSE), 0));
  PMASSERT(i != MIT_NONE);

  mapp->ID1 = (USHORT)MID_NONE;
  for(;;)
  { SHORT id = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0));
    DEBUGLOG(("PlaylistMenu::RemoveSubItems - %i %d\n", i, id));
    // Stop if either
    // - Error
    // - End of menu
    // - End of user range handled by this class
    // - WM_INITMENU called for the item while we clean up
    //   In this case an UM_UPDATE later will restor the content
    if (id == MIT_ERROR || id == 0 || id == mapp->Pos || (mapp->Status & InUse))
      break; // end of menu or range
    RemoveMapEntry(id);
    WinSendMsg(mapp->HwndSub, MM_DELETEITEM, MPFROM2SHORT(id, FALSE), 0);
  }
  mapp->Status &= ~InDestroy;
}

void PlaylistMenu::RemoveMapEntry(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveMapEntry(%p{%u,%x})\n", this, mapp, mapp->IDMenu, mapp->HwndSub));
  // Always disable the status events
  mapp->InfoDelegate.detach();
  // delete children recursively
  RemoveSubItems(mapp);
  // now destroy the submenu if any, but not at the top level
  if (mapp->HwndSub != NULLHANDLE && mapp->Parent)
    PMRASSERT(WinDestroyWindow(mapp->HwndSub));
  // update first free ID (optimization)
  if (mapp->IDMenu < ID1stfree)
    ID1stfree = mapp->IDMenu;
  // and now game over
  delete mapp;
  DEBUGLOG(("PlaylistMenu::RemoveMapEntry done - %u\n", MenuMap.size()));
}
void PlaylistMenu::RemoveMapEntry(USHORT mid)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveMapEntry(%u)\n", this, mid));
  MapEntry* mapp = MenuMap.erase(mid);
  if (mapp)
    RemoveMapEntry(mapp);
}

xstring PlaylistMenu::MakeMenuItemText(const MapEntry* mapp, size_t index)
{ xstring text = mapp->Data->GetDisplayName();
  if (text.length() > 30) // limit length?
    text.sprintf("%.30s...", text.cdata());
  if (mapp->Flags & Enumerate) // prepend enumeration?
    text.sprintf("%s%u %s", index<10 ? "~" : "", index, text.cdata());
  return text;
}

bool PlaylistMenu::CheckInUse(const MapEntry* mapp) const
{ unsigned inuse = mapp->Data->GetInUse();
  while (inuse)
  { mapp = mapp->Parent;
    if (mapp == NULL)
      return true;
    if (mapp->Data->GetInUse() != --inuse)
      return false;
  }
  return false;
}

void PlaylistMenu::InfoChangeHandler(const PlayableChangeArgs& args, MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::InfoChangeHandler({%p{%s},%x,%x}, %p) {%u, %x, %p}\n", this,
    &args.Instance, args.Instance.DebugName().cdata(), args.Changed, args.Loaded, mapp, mapp->IDMenu, mapp->HwndSub, mapp->Data.get()));
  if ((args.Changed & (IF_Phys|IF_Tech|IF_Display|IF_Usage)) && !mapp->Status.bitset(0))
    PMASSERT(WinPostMsg(HwndOwner, UM_UPDATEITEM, MPFROMSHORT(mapp->IDMenu), MPFROMP(mapp)));
  if ((args.Changed & IF_Child) && !mapp->Status.bitset(1))
    PMRASSERT(WinPostMsg(HwndOwner, UM_UPDATELIST, MPFROMSHORT(mapp->IDMenu), MPFROMP(mapp)));
}

bool PlaylistMenu::AttachMenu(HWND menu, USHORT menuid, APlayable& data, EntryFlags flags, MPARAM user, USHORT pos)
{ DEBUGLOG(("PlaylistMenu(%p)::AttachMenu(%x, %u, &%p{%s}, %x, %p, %u)\n", this,
    menu, menuid, &data, data.DebugName().cdata(), flags, user, pos));

  MapEntry*& mapp = MenuMap.get(menuid);
  if (mapp)
    // existing item => replace data
    mapp->Data = &data;
   else
    // new map item
    mapp = new MapEntry(menuid, NULL, data, flags, user, pos, *this, &PlaylistMenu::InfoChangeHandler);
  mapp->HwndSub = menu;

  // Fetch sub menu handle
  MENUITEM mi;
  PMRASSERT(WinSendMsg(menu, MM_QUERYITEM, MPFROM2SHORT(menuid, TRUE), MPFROMP(&mi)));
  ASSERT(mi.hwndSubMenu != NULLHANDLE); // Must be a submenu
  mapp->HwndSub = mi.hwndSubMenu;
  return true;
}

bool PlaylistMenu::DetachMenu(USHORT menuid)
{ DEBUGLOG(("PlaylistMenu(%p)::DetachMenu(%u)\n", this, menuid));
  MapEntry* mapp = MenuMap.find(menuid);
  if (!mapp) // registered map entry?
    return false;
  // delete old stuff later
  PMRASSERT(WinPostMsg(HwndOwner, UM_MENUEND, MPFROMSHORT(menuid), MPFROMP(mapp)));
  return true;
}
