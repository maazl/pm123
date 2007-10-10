/*
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

#include "playable.h"

#include <vdelegate.h>
#include <cpp/smartptr.h>
#include <cpp/container.h>
#include <cpp/xstring.h>

#include <os2.h>


/****************************************************************************
*
*  Class to view the content of a PlayableCollection as menu structure.
*
*  All instances of this class MUST be created with new PlaylistMenu(...).
*  The class has automatic lifetime management. That is it is automatically
*  deleted when the owner Window receives a WM_DESTROY message.
*  However, you might delete it earlier. But be careful if the owner window
*  handle is subclassed after the creation of this class.
*
*  Instances of this class are not thread-safe. Furthermore it is strongly
*  recommended to call all functions including the constructor from the
*  window procedure's thread.
*
****************************************************************************/
class PlaylistMenu
{private:
  enum // The ID's here must be distinct from the user messages of any other window.
  { UM_LATEUPDATE = WM_USER+0x201
  };
  struct MapEntry : public IComparableTo<USHORT>
  { USHORT          IDMenu;
    HWND            HwndMenu;
    int_ptr<Playable> Data;
    USHORT          ID1;  // First generated item ID or MID_NONE (if none)
    USHORT          Pos;  // ID of the first object after the last generated entry or MIT_END if this is the end of the menu.
    xstring         Text; // Strong reference to the text. 
    MapEntry(USHORT id, int_ptr<Playable> data, SHORT pos) : IDMenu(id), HwndMenu(NULLHANDLE), Data(data), ID1((USHORT)MID_NONE), Pos(pos) {}
    virtual int     CompareTo(const USHORT* key) const;
  };

 private:
  const HWND        HwndOwner;
  const USHORT      ID1st;
  const USHORT      IDlast;
 private:
  VREPLACE1         VR_DlgProc;
  PFNWP             Old_DlgProc;
  sorted_vector<MapEntry, USHORT> MenuMap;
  USHORT            ID1stfree;
  class_delegate<PlaylistMenu, const Playable::change_args> InfoDelegate;
  MapEntry*         UpdateEntry;
  
 private:
  // Dialog procedure, called by DlgProcStub
  MRESULT           DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  friend MRESULT EXPENTRY pm_DlgProcStub(PlaylistMenu* that, ULONG msg, MPARAM mp1, MPARAM mp2);

  // Fetch and reserve free menu ID
  // Start Searching after "start". This is an optimization. 
  USHORT            AllocateID();
  // Create Subitems according to the content of the playable object 
  void              CreateSubItems(MapEntry* mapp);
  // Removes all matching items from the menu
  void              RemoveSubItems(MapEntry* mapp);
  // Removes map entry including sub items.
  void              RemoveMapEntry(MapEntry* mapp);
  // Removes map entry including sub items.
  void              RemoveMapEntry(USHORT mid);

 private:
  void              ResetDelegate();
  void              ResetDelegate(MapEntry* mapp);
  void              InfoChangeCallback(const Playable::change_args& args);
 
 public:
  PlaylistMenu(HWND owner, USHORT mid1st, USHORT midlast);
                    ~PlaylistMenu();
  // Attach a submenu to a PlayableCollection.
  // This replaces all items in the menu with IDs in the range [mid1st,midlast)
  // by the content of the PlayableCollection. Nested playlists will show as submenus.
  // If the IDs are not sufficient the content is truncated. But the IDs to nested items
  // are only assigned if the submenu is opened.
  bool              AttachMenu(USHORT menuid, int_ptr<Playable> data, USHORT pos = (USHORT)MID_NONE);
};
