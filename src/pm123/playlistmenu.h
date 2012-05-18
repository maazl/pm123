/*
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

#ifndef PLAYLISTMENU_H
#define PLAYLISTMENU_H

#define INCL_WIN
#define INCL_BASE

#include "playable.h"

#include <vdelegate.h>
#include <cpp/smartptr.h>
#include <cpp/container/sorted_vector.h>
#include <cpp/xstring.h>
#include <cpp/cpputil.h>

#include <os2.h>


/****************************************************************************
*
*  Class to view the content of a PlayableCollection as menu structure.
*
*  All instances of this class \e must be created with \c new \c PlaylistMenu(...).
*  The class has automatic lifetime management. That is it is automatically
*  deleted when the owner Window receives a \c WM_DESTROY message.
*  However, you might delete it earlier. But be careful if the owner window
*  handle is subclassed after the creation of this class.
*
*  Instances of this class are not thread-safe. Furthermore it is strongly
*  recommended to call all functions including the constructor from the
*  window procedure's thread.
*
****************************************************************************/
class PlaylistMenu
{public:
  enum EntryFlags
  { None            = 0x00,
    /// Create dummy entry if content is empty.
    DummyIfEmpty    = 0x01,
    /// Enumerate first 10 items.
    Enumerate       = 0x02,
    /// Place separator before and after the generated entries
    /// unless it's the begin or end.
    Separator       = 0x04,
    /// Create sub menus for playlist or folder items.
    Recursive       = 0x10,
    /// Exclude invalid entries.
    SkipInvalid     = 0x20,
    /// Prefetch list content of next menu level.
    Prefetch        = 0x40
  };
  /// Window messages related to the PlaylistMenu.
  /// @remarks The ID's here must be distinct from the user messages of any other window.
  enum
  { /// This message is send to the owner of the menu when a generated subitem of the menu is selected.
    /// mp1 is set to a pointer to APlayable. The pointer is only valid while the message is sent.
    /// mp2 is the user parameter passed to AttachMenu.
    UM_SELECTED = WM_USER+0x201,
    /// This message is internally used by this class to notify changes of the selected items.
    /// mp1 id of the sub item
    /// mp2 MapEntry* for the ID. This is used for validation.
    UM_UPDATELIST,
    /// Status or text of the underlying Playable object of a menu item arrived.
    /// mp1 id of the sub item
    /// mp2 MapEntry* for the ID. This is used for validation.
    UM_UPDATEITEM,
    /// This message is posted to ourself to delay the destruction of the menu items
    /// until WM_COMMAND arrives.
    /// mp1 id of the sub item
    /// mp2 MapEntry* for the ID. This is used for validation.
    UM_MENUEND
  };

 private:
  enum EntryStatus
  { StatusRequest   = 0x01,     // A status update has been placed for this item.
    UpdateRequest   = 0x02,     // An update has been requested for this item.
    InUse           = 0x10,     // Entry is currently instantiated (WM_INITMENU).
    InUpdate        = 0x20,     // Entry is currently updated (UM_LATEUPDATE).
    InDestroy       = 0x40      // Entry is currently destroyed (UM_MENUEND).
  };
  CLASSFLAGSATTRIBUTE(PlaylistMenu::EntryStatus);
  struct MapEntry
  { const USHORT    IDMenu;     // Menu item ID, primary key
    EntryFlags      Flags;      // See EntryFlags
    MapEntry*       Parent;     // Parent menu item or NULL in case of root.
    HWND            HwndSub;    // Sub menu window handle.
    int_ptr<APlayable> Data;    // Backend data
    AtomicUnsigned  Status;     // See EntryStatus
    const MPARAM    User;       // User param from AttachMenu. Inherited to submenus.
    USHORT          ID1;        // First generated item ID or MID_NONE (if none)
    USHORT          Pos;        // ID of the first object after the last generated entry or MIT_END if this is the end of the menu.
    class_delegate2<PlaylistMenu, const PlayableChangeArgs, MapEntry> InfoDelegate;

    MapEntry(USHORT id, MapEntry* parent, APlayable& data, EntryFlags flags, MPARAM user, SHORT pos, PlaylistMenu& owner, void (PlaylistMenu::*infochg)(const PlayableChangeArgs&, MapEntry*));
    static int      compare(const USHORT& key, const MapEntry& entry);
  };
  typedef sorted_vector<MapEntry, USHORT, &MapEntry::compare> MapType;

 private:
  const HWND        HwndOwner;
  const USHORT      ID1st;
  const USHORT      IDlast;
 private:
  VREPLACE1         VR_DlgProc;
  PFNWP             Old_DlgProc;
  MapType           MenuMap;
  USHORT            ID1stfree;

 private:
  /// Dialog procedure, called by DlgProcStub
  MRESULT           DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  friend MRESULT EXPENTRY pm_DlgProcStub(PlaylistMenu* that, ULONG msg, MPARAM mp1, MPARAM mp2);

  /// @brief Fetch and reserve free menu ID
  USHORT            AllocateID();
  /// Create a sub menu for \a mi.
  HWND              CreateSubMenu(MENUITEM& mi, HWND parent);
  /// Insert a menu separator at position \a where.
  USHORT            InsertSeparator(HWND menu, SHORT where);
  /// Create a dummy entry at position \a where into a \a menu.
  /// @return The ID of the created dummy.
  USHORT            InsertDummy(HWND menu, SHORT where, const char* text);
  /// Insert a new entry in a menu.
  MapEntry*         InsertEntry(MapEntry* parent, SHORT where, APlayable& data, size_t index);
  /// Create/refresh the content of a sub menu from the playlist data.
  void              UpdateSubItems(MapEntry* mapp);
  /// Removes all matching items from the menu
  void              RemoveSubItems(MapEntry* mapp);
  /// Removes map entry including sub items.
  /// The MapEntry should be removed from MenuMap already.
  void              RemoveMapEntry(MapEntry* mapp);
  /// Removes map entry including sub items.
  /// This also removes the entry from MenuMap.
  void              RemoveMapEntry(USHORT mid);
  /// Update menu item
  void              UpdateItem(MapEntry* mapp);
  /// Generate the item text for a menu item.
  xstring           MakeMenuItemText(MapEntry* mapp, size_t index);
  /// Called from \c APlayable->InfoChange when a menu item changes.
  void              InfoChangeHandler(const PlayableChangeArgs& args, MapEntry* mapp);

 private: // non-copyable
                    PlaylistMenu(const PlaylistMenu&);
  void              operator=(const PlaylistMenu&);
 public:
                    PlaylistMenu(HWND owner, USHORT mid1st, USHORT midlast);
                    ~PlaylistMenu();
  /// @brief Attach a submenu to a playlist.
  /// @details This replaces all items in the menu with IDs in the range [mid1st,midlast)
  /// by the content of the playlist. Nested playlists will show as submenus.
  /// If the IDs are not sufficient the content is truncated. But the IDs to nested items
  /// are only assigned if a submenu is opened.
  bool              AttachMenu(HWND menu, USHORT menuid, APlayable& data, EntryFlags flags, MPARAM user, USHORT pos = (USHORT)MID_NONE);
  /// Detach a menu from a submenu item.
  bool              DetachMenu(USHORT menuid);

 public:
  /// Maximum number of menu items in one sub menu. Larger playlists are truncated.
  size_t            MaxMenu;
};

FLAGSATTRIBUTE(PlaylistMenu::EntryFlags);
//FLAGSATTRIBUTE(PlaylistMenu::EntryStatus);

#endif
