/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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
#include <utilfct.h>

#include <debuglog.h>

// Maximum number of items per submenu
#define DEF_MAX_MENU 100


inline PlaylistMenu::MapEntry::MapEntry(USHORT id, MapEntry* parent, APlayable& data, EntryFlags flags, MPARAM user, SHORT pos, PlaylistMenu& owner, void (PlaylistMenu::*infochg)(const PlayableChangeArgs&, MapEntry*))
: IDMenu(id),
  Parent(parent),
  HwndSub(NULLHANDLE),
  Data(&data),
  Flags(flags),
  User(user),
  ID1((USHORT)MID_NONE),
  Pos(pos),
  InfoDelegate(data.GetInfoChange(), owner, infochg, this)
{}

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
  UpdateEntry(NULL),
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

/*// TODO: remove
static void menutest(HWND menu)
{
  HWND owner = WinQueryWindow(menu, QW_OWNER);
  HWND parent = WinQueryWindow(menu, QW_PARENT);
  SWP pos;
  WinQueryWindowPos(menu, &pos);
  LONG count = LONGFROMMR(WinSendMsg(menu, MM_QUERYITEMCOUNT, 0, 0));
  DEBUGLOG(("menutest: menu = %x, owner = %x, parent = %x, pos = {%x, %i,%i, %i,%i, %x ...}, count = %li\n",
    menu, owner, parent, pos.fl, pos.x, pos.y, pos.cx, pos.cy, pos.hwndInsertBehind, count));
}*/

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
      DEBUGLOG(("PlaylistMenu::DlgProc: WM_INITMENU: %p{%u, %x,...}\n", mapp, mapp->IDMenu, mapp->HwndSub));
      mapp->HwndSub = HWNDFROMMP(mp2);
      // delete old stuff
      //RemoveSubItems(mapp);
      // And add new
      CreateSubItems(mapp);
      //menutest(HWNDFROMMP(mp2));
      break;
    }

   case WM_MENUEND:
    { DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_MENUEND(%u, %x)\n", this, SHORT1FROMMP(mp1), mp2));
      UpdateEntry = NULL; // Discard updates
      MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      if (mapp == NULL)
        break; // No registered map entry, continue default processing.
      //menutest(HWNDFROMMP(mp2));
      // delete old stuff later
      PMRASSERT(WinPostMsg(HwndOwner, UM_MENUEND, mp1, MPFROMP(mapp)));
    }

   case UM_MENUEND:
    { MapEntry* mapp = MenuMap.find(SHORT1FROMMP(mp1));
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: UM_MENUEND(%u, %p) - %p\n", this, SHORT1FROMMP(mp1), mp2, mapp));
      // delete old stuff, but only if still valid.
      if (mapp == (MapEntry*)mp2)
        RemoveSubItems(mapp);
      return 0;
    }

   #if 0
   case WM_MENUSELECT:
    DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_MENUSELECT(%u, %u, %x)\n", this, SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), mp2));
    break;
   #endif

   case WM_COMMAND:
    if (SHORT1FROMMP(mp2) == CMDSRC_MENU)
    { MapEntry* mp = MenuMap.find(SHORT1FROMMP(mp1));
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: WM_COMMAND(%u) - %p\n", this, SHORT1FROMMP(mp1), mp));
      if (mp) // ID unknown?
      { WinSendMsg(HwndOwner, UM_SELECTED, MPFROMP(mp->Data.get()), mp->User);
        return 0; // no further processing
    } }
    break;

   case UM_LATEUPDATE:
    DEBUGLOG(("PlaylistMenu(%p)::DlgProc: UM_LATEUPDATE(%p) - %p\n", this, PVOIDFROMMP(mp2), UpdateEntry));
    if (UpdateEntry == PVOIDFROMMP(mp2))
    { RemoveSubItems(UpdateEntry); // maybe we have to remove a dummy entry
      CreateSubItems(UpdateEntry);
      UpdateEntry = NULL;
    }
    break;
    
   case UM_UPDATESTAT:
    { MapEntry* mp = MenuMap.find(SHORT1FROMMP(mp1));
      DEBUGLOG(("PlaylistMenu(%p)::DlgProc: UM_UPDATESTAT(%u, %p) - %p\n",
        this, SHORT1FROMMP(mp1), PVOIDFROMMP(mp2), mp));
      // ID known and valid and not root?
      if (mp && mp == PVOIDFROMMP(mp2) && mp->Parent)
        UpdateItem(mp);
    }
    break;
  }
  // Call chained dialog procedure
  return (*Old_DlgProc)(HwndOwner, msg, mp1, mp2);
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

void PlaylistMenu::CreateSubMenu(MENUITEM& mi, HWND parent)
{ DEBUGLOG(("PlaylistMenu::CreateSubMenu({%u,...}, %x)\n", mi.id, parent));
  mi.afStyle |= MIS_SUBMENU;
  mi.hwndSubMenu = WinLoadMenu(parent, NULLHANDLE, MNU_SUBFOLDER);
  PMRASSERT(WinSetParent(mi.hwndSubMenu, HWND_OBJECT, FALSE));
  PMASSERT(mi.hwndSubMenu);
  PMRASSERT(WinSetWindowUShort(mi.hwndSubMenu, QWS_ID, mi.id));
  PMRASSERT(WinSetWindowBits(mi.hwndSubMenu, QWL_STYLE, MS_CONDITIONALCASCADE, MS_CONDITIONALCASCADE));
  PMRASSERT(WinSendMsg(mi.hwndSubMenu, MM_SETDEFAULTITEMID, MPFROMLONG(mi.id), 0));
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

USHORT PlaylistMenu::InsertDummy(HWND menu, SHORT where, const char* text)
{ DEBUGLOG(("PlaylistMenu::InsertDummy(%x, %i, %s)\n", menu, where, text));
  MENUITEM mi = {0};
  mi.iPosition = where;
  mi.afStyle     = MIS_TEXT|MIS_STATIC;
  mi.id          = AllocateID();
  if (mi.id != (USHORT)MID_NONE)
    PMXASSERT(SHORT1FROMMR(WinSendMsg(menu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(text))), != (USHORT)MIT_ERROR);
  return mi.id;
}

void PlaylistMenu::UpdateItem(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu::UpdateItem(%p{%u, %x, %p{%s}})\n",
    mapp, mapp->IDMenu, mapp->HwndSub, mapp->Data.get(), mapp->Data->GetPlayable().URL.getDisplayName().cdata()));
  MapEntry* parp = mapp->Parent;
  ASSERT(parp);

  Playable& p = mapp->Data->GetPlayable();
  const unsigned tattr = p.GetInfo().tech->attributes;
  const bool invalid = (p.GetInfo().phys->attributes & PATTR_INVALID) || (tattr & TATTR_INVALID);
  // remove invalid?
  if (invalid && (mapp->Flags & SkipInvalid))
  { RemoveMapEntry(mapp);
    return;
  }
  PMRASSERT(WinSendMsg(parp->HwndSub, MM_SETITEMATTR,
    MPFROM2SHORT(mapp->IDMenu, FALSE), MPFROM2SHORT(MIA_DISABLED, MIA_DISABLED * invalid)));
  MENUITEM mi;
  PMRASSERT(WinSendMsg(parp->HwndSub, MM_QUERYITEM, MPFROM2SHORT(mapp->IDMenu, FALSE), MPFROMP(&mi)));
  DEBUGLOG(("PlaylistMenu::UpdateItem: %x - %p\n", tattr, mi.hwndSubMenu));
  HWND oldmenu = NULLHANDLE;
  if (!invalid && (mapp->Flags & Recursive) && (tattr & TATTR_PLAYLIST)) // with submenu?
  { if (mi.hwndSubMenu != NULLHANDLE)
      return;
    CreateSubMenu(mi, parp->HwndSub);
    // OS/2 seems to dislike empty submenues and does not send WM_INITMENU for them.
    // So create dummy.
    //mapp->ID1 = InsertDummy(mi.hwndSubMenu, MIT_END, "- loading -");
    //mapp->Pos = MIT_END;
  } else
  { // No Playlist
    if (mi.hwndSubMenu == NULLHANDLE)
      return;
    RemoveSubItems(mapp);
    mi.afStyle &= ~MIS_SUBMENU;
    oldmenu = mi.hwndSubMenu;
    mi.hwndSubMenu = NULLHANDLE;
  }
  PMRASSERT(WinSendMsg(parp->HwndSub, MM_SETITEM, MPFROM2SHORT(0, FALSE), MPFROMP(&mi)));
  if (oldmenu)
    PMRASSERT(WinDestroyWindow(oldmenu));
}

void PlaylistMenu::CreateSubItems(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::CreateSubItems(%p{%u})\n", this, mapp, mapp->IDMenu));
  // is enumerable?
  if (!(mapp->Data->GetInfo().tech->attributes & TATTR_PLAYLIST))
    return;

  MENUITEM mi = {0};
  mi.iPosition   = MIT_END;
  //mi.hItem       = 0;
  // Find insert position
  if (mapp->Pos != (USHORT)MID_NONE)
    mi.iPosition = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->Pos, FALSE), 0));
  size_t count = 0;
  if (mapp->Data->RequestInfo(IF_Child, PRI_Normal))
  { // not immediately available => do it later
    UpdateEntry = mapp;
    mapp->ID1 = InsertDummy(mi.hwndSubMenu, mi.iPosition, "- loading -");
    // separator after
    if ((mapp->Flags & Separator) && mi.iPosition != MIT_END)
      InsertSeparator(mapp->HwndSub, mi.iPosition);
  } else
  { // lock collection
    Playable& list = mapp->Data->GetPlayable();
    Mutex::Lock lock(list.Mtx);
    int_ptr<PlayableInstance> pi;
    while (count < MaxMenu && (pi = list.GetNext(pi)))
    { // Get content
      Playable& p = pi->GetPlayable();
      if (mapp->Flags & Prefetch)
        // Prefetch nested playlist content.
        p.RequestInfo(IF_Child, PRI_Low);
      p.RequestInfo(IF_Tech, PRI_Normal);
      const unsigned tattr = p.GetInfo().tech->attributes;
      const bool invalid = (p.GetInfo().phys->attributes & PATTR_INVALID) || (tattr & TATTR_INVALID);
      DEBUGLOG(("PlaylistMenu::CreateSubItems: at %p{%s}\n", &p, p.URL.getShortName().cdata()));
      // skip invalid?
      if (invalid && (mapp->Flags & SkipInvalid))
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
      // Add map entry.
      MapEntry*& subp = MenuMap.get(mi.id);
      ASSERT(subp == NULL);
      subp = new MapEntry(mi.id, mapp, *pi, mapp->Flags, mapp->User, MIT_END, *this, &PlaylistMenu::InfoChangeHandler);
      // Invalid?
      if (invalid)
        mi.afAttribute |= MIA_DISABLED;
      // with submenu?
      else if ((mapp->Flags & Recursive) && (tattr & TATTR_PLAYLIST))
        CreateSubMenu(mi, mapp->HwndSub);
      // Add menu item
      xstring text = pi->GetDisplayName();
      if (text.length() > 30) // limit length?
        text = xstring(text, 0, 30) + "...";
      if (mapp->Flags & Enumerate) // prepend enumeration?
        text = xstring::sprintf("%s%u %s", count<10 ? "~" : "", count, text.cdata());
      SHORT rs = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(text.cdata())));
      if (mi.iPosition != MIT_END)
        ++mi.iPosition;
      DEBUGLOG(("PlaylistMenu::CreateSubItems: new item %u->%s - %i\n", mi.id, text.cdata(), rs));
      // Separator before?
      if ((mapp->Flags & Separator) && rs != 0 && count == 1)
      { USHORT id = InsertSeparator(mapp->HwndSub, rs);
        if (id != (USHORT)MID_NONE)
        { mapp->ID1 = id;
          if (mi.iPosition != MIT_END)
            ++mi.iPosition;
      } }
    }
    if (count != 0)
    { // separator after
      if ((mapp->Flags & Separator) && mi.iPosition != MIT_END)
        InsertSeparator(mapp->HwndSub, mi.iPosition);
    } else if (mapp->Flags & DummyIfEmpty)
    { // create dummy
      mi.id = InsertDummy(mapp->HwndSub, mi.iPosition, "- none -");
      if (mapp->ID1 == (USHORT)MID_NONE)
        mapp->ID1 = mi.id;
    }
  }
}

void PlaylistMenu::RemoveSubItems(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveSubItems(%p{%u,%x,%u,%u})\n", this, mapp, mapp->IDMenu, mapp->HwndSub, mapp->ID1, mapp->Pos));
  if (mapp->ID1 == (USHORT)MID_NONE || mapp->HwndSub == NULLHANDLE)
    return; // no subitems or not yet initialized

  SHORT i = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_ITEMPOSITIONFROMID, MPFROM2SHORT(mapp->ID1, FALSE), 0));
  PMASSERT(i != MIT_NONE);

  mapp->ID1 = (USHORT)MID_NONE;
  for(;;)
  { SHORT id = SHORT1FROMMR(WinSendMsg(mapp->HwndSub, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0));
    DEBUGLOG(("PlaylistMenu::RemoveSubItems - %i %d\n", i, id));
    if (id == MIT_ERROR || id == 0 || id == mapp->Pos)
      return; // end of menu or range
    RemoveMapEntry(id);
    WinSendMsg(mapp->HwndSub, MM_DELETEITEM, MPFROM2SHORT(id, FALSE), 0);
  }
}

void PlaylistMenu::RemoveMapEntry(MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::RemoveMapEntry(%p{%u,%x})\n", this, mapp, mapp->IDMenu, mapp->HwndSub));
  // Always disable the status events
  mapp->InfoDelegate.detach();
  // deregister event if it's mine
  if (UpdateEntry == mapp)
    UpdateEntry = NULL;
  // delete children recursively
  RemoveSubItems(mapp);
  // now destroy the submenu if any
  if (mapp->HwndSub) // only if menu has been shown already
  { /*MapEntry* parp = mapp->Parent;
    ASSERT(parp);
    MENUITEM mi;
    PMRASSERT(WinSendMsg(parp->HwndSub, MM_QUERYITEM, MPFROM2SHORT(mapp->IDMenu, FALSE), MPFROMP(&mi)));
    if (mi.hwndSubMenu)
      PMRASSERT(WinDestroyWindow(mi.hwndSubMenu));*/
    PMRASSERT(WinDestroyWindow(mapp->HwndSub));
  }
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

void PlaylistMenu::InfoChangeHandler(const PlayableChangeArgs& args, MapEntry* mapp)
{ DEBUGLOG(("PlaylistMenu(%p)::InfoChangeHandler({%p,%x,%x}, %p) {%u, %x, %p} - %p, %p\n", this,
    &args.Instance, args.Changed, args.Loaded, mapp,
    mapp->IDMenu, mapp->HwndSub, mapp->Data.get(), UpdateEntry));
  if (args.Changed & (IF_Phys|IF_Tech))
    PMASSERT(WinPostMsg(HwndOwner, UM_UPDATESTAT, MPFROMSHORT(mapp->IDMenu), MPFROMP(mapp)));
  // update list of children, but only for the current menu.
  if (args.Changed & IF_Child)
  { // event obsolete?
    if (UpdateEntry != NULL && &args.Instance == UpdateEntry->Data)
      PMRASSERT(WinPostMsg(HwndOwner, UM_LATEUPDATE, 0, MPFROMP(mapp)));
  }
}

bool PlaylistMenu::AttachMenu(USHORT menuid, APlayable& data, EntryFlags flags, MPARAM user, USHORT pos)
{ DEBUGLOG(("PlaylistMenu(%p)::AttachMenu(%u, &%p{%s}, %x, %p, %u)\n", this, menuid, &data, data.GetPlayable().URL.getShortName().cdata(), flags, user, pos));

  MapEntry*& mapp = MenuMap.get(menuid);
  if (mapp)
    // existing item => replace data
    mapp->Data = &data;
   else
    // new map item
    mapp = new MapEntry(menuid, NULL, data, flags, user, pos, *this, &PlaylistMenu::InfoChangeHandler);

  return true;
}


