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

#ifndef PLAYLISTBASE_H
#define PLAYLISTBASE_H

#define INCL_WIN

#define DECODER_PLUGIN_LEVEL 2
#include "windowbase.h"
#include "playable.h"
#include "controller.h"
#include <decoder_plug.h>
#include <os2.h>

#include <cpp/queue.h>
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <cpp/container.h>
#include <cpp/xstring.h>
#include <cpp/url123.h>

#ifndef DC_PREPAREITEM
#define DC_PREPAREITEM   0x0040
#endif

#include <debuglog.h>


/****************************************************************************
*
*  This abstract class represents an instance of a playlist window.
*  Whether it is a tree view or a detailed view detremines the derived class.
*
****************************************************************************/
class PlaylistBase
: public ManagedDialogBase,
  public IComparableTo<Playable>
{public:
  struct CommonState
  { unsigned            PostMsg;   // Bitvector of outstanding record commands
    bool                WaitUpdate;// UpdateTech is waiting for UpdateInfo event.
    CommonState() : PostMsg(0), WaitUpdate(false) {}
  };
  // C++ part of a record
  struct RecordBase;
  struct CPDataBase
  { int_ptr<PlayableInstance> const Content; // The pointer to the backend content.
    xstring             Text;      // Storage for alias name <-> pszIcon
    CommonState         EvntState;
    class_delegate2<PlaylistBase, const Playable::change_args        , RecordBase*> InfoChange;
    class_delegate2<PlaylistBase, const PlayableInstance::change_args, RecordBase*> StatChange;
    CPDataBase(PlayableInstance* content, PlaylistBase& pm,
               void (PlaylistBase::*infochangefn)(const Playable::change_args&,         RecordBase*),
               void (PlaylistBase::*statchangefn)(const PlayableInstance::change_args&, RecordBase*),
               RecordBase* rec);
    virtual             ~CPDataBase() {} // free the right objects
    virtual void        DeregisterEvents();
  };
  // POD part of a record
  struct RecordBase : public MINIRECORDCORE
  { volatile unsigned   UseCount;  // Reference counter to keep Records alive while Posted messages are on the way.
    CPDataBase*         Data;      // C++ part of the object in the private memory arena.
    #ifdef DEBUG
    xstring             DebugName() const;
    static xstring      DebugName(const RecordBase* rec);
    #endif
  };
  struct UserAddCallbackParams;
  friend struct UserAddCallbackParams;
  struct UserAddCallbackParams
  { PlaylistBase* GUI;
    PlaylistBase::RecordBase* Parent;
    PlaylistBase::RecordBase* Before;
    sco_ptr<Playable::Lock> Lock;
    UserAddCallbackParams(PlaylistBase* gui, PlaylistBase::RecordBase* parent, PlaylistBase::RecordBase* before)
    : GUI(gui),
      Parent(parent),
      Before(before)
    { // Increment usage count of the records unless the dialog completed.
      GUI->BlockRecord(Parent);
      GUI->BlockRecord(Before);
    }
    ~UserAddCallbackParams()
    { GUI->FreeRecord(Before);
      GUI->FreeRecord(Parent);
    }
  };
 protected: // Message Processing
  enum
  { UM_UPDATEINFO = WM_USER+0x101,
    // Execute a command for a record, see below.
    // mp1 = Record* or NULL
    // The reference counter of the Record is decremented after the message is processed.
    // The Commands to be executed are atomically taken from the PostMsg bitvector
    // in the referenced record or the container.
    UM_RECORDCOMMAND,
    // Delete a record structure and put it back to PM
    // mp1 = Record
    UM_DELETERECORD,
    // Playstatus-Event hit.
    // mp1 = status
    UM_PLAYSTATUS,
    // Asynchronouos insert operation
    // mp1 = InsertInfo*
    // The InsertInfo structure is deleted after the message is processed.
    UM_INSERTITEM,
    // Remove item adressed by a record asynchronuously
    // mp1 = Record*
    // The reference counter of the Record is decremented after the message is processed.
    UM_REMOVERECORD,
    // Update decoder tables.
    UM_UPDATEDEC
  };
  // Valid flags in PostMsg fields
  enum RecordCommand
  { // Update the children of the Record
    // If mp1 == NULL the root node is refreshed.
    RC_UPDATECHILDREN,
    // Update the format information of a record
    // If mp1 == NULL the root node is refreshed.
    RC_UPDATEFORMAT,
    // Update the technical information of a record
    // If mp1 == NULL the root node is refreshed.
    RC_UPDATETECH,
    // Update the meta information of a record
    // If mp1 == NULL the root node is refreshed.
    RC_UPDATEMETA,
    // Update the physical file information of a record
    // If mp1 == NULL the root node is refreshed.
    RC_UPDATEPHYS,
    // Update the recursive playlist information of a record
    // If mp1 == NULL the root node is refreshed.
    RC_UPDATERPL,
    // Update the status of a record
    // If mp1 == NULL the root node is refreshed.
    RC_UPDATESTATUS,
    // Update the alias text of a record
    RC_UPDATEALIAS,
    // Update starting position
    RC_UPDATEPOS    
  };
 public:
  // return value of AnalyzeRecordTypes
  enum RecordType
  { // record list is empty
    RT_None = 0x00,
    // record list contains at least one song
    RT_Song = 0x01,
    // record list contains at least one enumeratable item 
    RT_Enum = 0x02,
    // record list contains at least one playlist
    RT_List = 0x04
  };
 protected: // User actions
  struct InsertInfo
  { int_ptr<Playlist> Parent; // List where to insert the new item
    xstring      URL;         // URL to insert
    xstring      Alias;       // Alias name (if desired)
    xstring      Start;       // Start of slice (if any)
    xstring      Stop;        // Stop of slice (if any)
    int_ptr<PlayableInstance> Before; // Item where to do the insert
  };
 protected: // D'n'd
  struct DropInfo
  { HWND         Hwnd;    // Window handle of the source of the drag operation.
    ULONG        ItemID;  // Information used by the source to identify the object being dragged.
    int_ptr<Playlist> Parent; // List where to insert the new item
    xstring      Alias;   // Alias name at the target
    RecordBase*  Before;  // Record where to insert the item
                          // Keeping the Record here requires an allocation with BlockRecord.
  };

 protected: // Cached Icon ressources
  enum ICP
  { ICP_Song,
    ICP_Empty,
    ICP_Closed,
    ICP_Open,
    ICP_Recursive
  };
  enum IC
  { IC_Normal,
    IC_Used,
    IC_Active,
    IC_Play
  };
  static HPOINTER   IcoWait;
  static HPOINTER   IcoInvalid;
  static HPOINTER   IcoPlayable[5][4];
  static void       InitIcons();

 protected: // content
  int_ptr<Playable> Content;
  xstring           Alias;         // Alias name for the window title
  HACCEL            AccelTable;    // Accelerator table - TODO: use shared accelerator table
  #if DEBUG
  xstring           DebugName() const;
  #endif
 protected: // working set
  HWND              HwndContainer; // content window handle
  DECODER_WIZZARD_FUNC LoadWizzards[20]; // Current load wizzards
  bool              NoRefresh;     // Avoid update events to ourself
  xstring           DirectEdit;    // String that holds result of direct manipulation between CN_REALLOCPSZ and CN_ENDEDIT
  CommonState       EvntState;     // Event State
  vector<RecordBase> Source;       // Array of records used for source emphasis
  HWND              HwndMenu;      // Window handle of last context menu
  bool              DragAfter;     // Recent drag operation was ORDERED
  PlayableCollection::ItemComparer SortComparer; // Current comparer for next sort operation
  bool              DecChanged;    // Flag whether the decoder table has changed since the last invokation of the context menu.
 private:
  int_ptr<PlaylistBase> Self;      // we hold a reference to ourself as long as the current window is open
  class_delegate2<PlaylistBase, const Playable::change_args, RecordBase*> RootInfoDelegate;
  class_delegate<PlaylistBase, const Ctrl::EventFlags> RootPlayStatusDelegate;
  class_delegate<PlaylistBase, const PLUGIN_EVENTARGS> PluginDelegate;

 protected:
  // Create a playlist manager window for an object, but don't open it.
  PlaylistBase(Playable* content, const xstring& alias, ULONG rid);

  CommonState&      StateFromRec(const RecordBase* rec) { return rec ? rec->Data->EvntState : EvntState; }
  // Fetch the Playable object associated with rec. If rec is NULL the root object is returned.
  Playable*         PlayableFromRec(const RecordBase* rec) const { return rec ? rec->Data->Content->GetPlayable() : &*Content; }
  // Prevent a Record from deletion until FreeRecord is called
  void              BlockRecord(RecordBase* rec)
                    { if (rec) InterlockedInc(rec->UseCount); }
  // Free a record after it is no longer used e.g. because a record message sent with PostRecordCommand completed.
  // This function does not free the record immediately. It posts a message which does the job.
  // So you can safely access the record data until the next PM call.
  void              FreeRecord(RecordBase* rec);
  // Post record message
  virtual void      PostRecordCommand(RecordBase* rec, RecordCommand cmd);

  // Gives the Record back to the PM and destoys the C++ part of it.
  // The Record object is no longer valid after calling this function.
  void              DeleteEntry(RecordBase* entry);

  void              StartDialog();
  // Called at WM_INITDLG
  // Should set Container
  virtual void      InitDlg();

  // Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);

  // Determine type of Playable object
  // Subfunction to CalcIcon.
  virtual ICP       GetPlayableType(const RecordBase* rec) const = 0;
  // Gets the Usage type of a record.
  // Subfunction to CalcIcon.
  virtual IC        GetRecordUsage(const RecordBase* rec) const = 0;
  // Calculate icon for a record. Content must be valid!
  HPOINTER          CalcIcon(RecordBase* rec);
  // Set the window title
  void              SetTitle();
  // Load context menu for a record
  virtual HWND      InitContextMenu() = 0;
  // Update plug-in specific accelerator table.
  virtual void      UpdateAccelTable() = 0;

  // Subfunction to the factory below.
  virtual RecordBase* CreateNewRecord(PlayableInstance* obj, RecordBase* parent) = 0;
  // Factory: create a new entry in the container.
  RecordBase*       AddEntry(PlayableInstance* obj, RecordBase* parent, RecordBase* after);
  // Moves the record "entry" including its subitems as child to "parent" after the record "after".
  // If parent is NULL the record is moved to the top level.
  // If after is NULL the record is moved to the first place of parent.
  // The function returns the moved entry or NULL in case of an error.
  RecordBase*       MoveEntry(RecordBase* entry, RecordBase* parent, RecordBase* after);
  // Removes entries from the container
  void              RemoveEntry(RecordBase* const rec);
  // Find parent record. Returns NULL if rec is at the top level.
  virtual RecordBase* GetParent(const RecordBase* const rec) const = 0;
  // Remove all childrens (recursively) and return the number of deleted objects in the given level.
  // This function does not remove the record itself.
  // If root is NULL the entire Collection is cleared.
  int               RemoveChildren(RecordBase* const root);
  // Update the list of children (if available) or schedule a request.
  virtual void      RequestChildren(RecordBase* const rec);
  // Update the list of children
  // rec == NULL => root node
  virtual void      UpdateChildren(RecordBase* const rec);
  
  // Populate Source array with records with a given emphasis or an empty list if none.
  void              GetRecords(USHORT emphasis);
  // Populate Source array for context menu or drag and drop with anchor rec.
  bool              GetSource(RecordBase* rec);
  // Apply a function to all objects in the Source array.
  void              Apply2Source(void (*op)(Playable*)) const;
  void              Apply2Source(void (PlaylistBase::*op)(Playable*));
  // Set or clear the emphasis of the records in the Source array.
  void              SetEmphasis(USHORT emphasis, bool set) const;
  // Analyze the records in the Source array for it's types.
  RecordType        AnalyzeRecordTypes() const;
  // Return true if and only if rec is a subitem of the currently loaded root.
  // This will not return true if rec points directly to the current root.
  bool              IsUnderCurrentRoot(RecordBase* rec) const;

 protected: // Update Functions.
            // They are logically virtual, but they are not called from this class.
            // They may be used in the window procedure of derived classes.
  // Update the status of a record
  void              UpdateStatus(RecordBase* rec);
  // Update the information from a playable instance
  void              UpdateInstance(RecordBase* rec, PlayableInstance::StatusFlags iflags);
  // Update status of all active PlayableInstances
  void              UpdatePlayStatus();
  // Update play status of one record
  virtual void      UpdatePlayStatus(RecordBase* rec);

 protected: // Notifications by the underlying Playable objects.
  // This function is called when meta, status or technical information of a node changes.
  void              InfoChangeEvent(const Playable::change_args& args, RecordBase* rec);
  // This function is called when status information of a PlayableInstance changes.
  void              StatChangeEvent(const PlayableInstance::change_args& inst, RecordBase* rec);
  // This function is called when playing starts or stops.
  void              PlayStatEvent(const Ctrl::EventFlags& flags);
  // This function is called when the list of enabled plug-ins changed.
  void              PluginEvent(const PLUGIN_EVENTARGS& args);

 protected: // User actions
  // Select a list with a file dialog.
  static url123     PlaylistSelect(HWND owner, const char* title);
  // Add Item
  void              UserAdd(DECODER_WIZZARD_FUNC wizzard, RecordBase* parent = NULL, RecordBase* before = NULL);
  // Insert a new item
  void              UserInsert(const InsertInfo* pii);
  // Remove item by Record pointer
  void              UserRemove(RecordBase* rec);
  // Save list
  void              UserSave();
  // Navigate to
  virtual void      UserNavigate(const RecordBase* rec) = 0;
  // Open tree view 
  void              UserOpenTreeView(Playable* pp);
  // Open detailed view 
  void              UserOpenDetailedView(Playable* pp);
  // View Info
  void              UserOpenInfoView(Playable* pp);
  // Clear playlist
  static void       UserClearPlaylist(Playable* pp);
  // Refresh records
  void              UserReload(Playable* pp);
  // Edit metadata
  void              UserEditMeta(Playable* pp);
  // Sort records
  void              UserSort(Playable* pp);
  // Place records in random order.
  static void       UserShuffle(Playable* pp);
  // comparers
  static int        CompURL(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompTitle(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompArtist(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompAlbum(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompAlias(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompSize(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompTime(const PlayableInstance* l, const PlayableInstance* r);
  
 protected: // D'n'd target
  // Handle CN_DRAGOVER/CN_DRAGAFTER
  MRESULT           DragOver(DRAGINFO* pdinfo, RecordBase* target);
  // Handle CN_DROP
  void              DragDrop(DRAGINFO* pdinfo, RecordBase* target);
  // Handle DM_RENDERCOMPLETE
  void              DropRenderComplete(DRAGTRANSFER* pdtrans, USHORT flags);
 protected: // D'n'd source
  // Handle CN_INITDRAG
  void              DragInit();
  // Handle DM_DISCARDOBJECT
  bool              DropDiscard(DRAGINFO* pdinfo);
  // Handle DM_RENDER
  BOOL              DropRender(DRAGTRANSFER* pdtrans);
  // Handle DM_ENDCONVERSATION
  void              DropEnd(RecordBase* rec, bool ok);

 public: // public interface
  virtual           ~PlaylistBase();
  // Gets the content
  Playable*         GetContent() { return Content; }
  // Get the display name of this instance. This is either the alias (if any) or the display name of the underlying URL.
  xstring           GetDisplayName() const { return Alias ? Alias : Content->GetURL().getDisplayName(); }
  // Get an instance of the same type as the current instance for URL.
  virtual int_ptr<PlaylistBase> GetSame(Playable* obj) = 0;

  // IComparableTo<Playable>
  virtual int       compareTo(const Playable& r) const;

};
FLAGSATTRIBUTE(PlaylistBase::RecordType);

inline PlaylistBase::CPDataBase::CPDataBase(
  PlayableInstance* content,
  PlaylistBase& pm,
  void (PlaylistBase::*infochangefn)(const Playable::change_args&,         RecordBase*),
  void (PlaylistBase::*statchangefn)(const PlayableInstance::change_args&, RecordBase*),
  RecordBase* rec)
: Content(content),
  InfoChange(pm, infochangefn, rec),
  StatChange(pm, statchangefn, rec)
{}

inline void PlaylistBase::CPDataBase::DeregisterEvents()
{ InfoChange.detach();
  StatChange.detach();
}


/****************************************************************************
*
*  Template class to assist the implementation of an instance repository
*  of type T. T must derive from PlaylistBase.
*
****************************************************************************/
template <class T>
class PlaylistRepository : public PlaylistBase
{private:
  static sorted_vector<T, Playable> RPInst;
  static Mutex      RPMutex;
 public:
  // currently a no-op
  static void       Init() {}
  static void       UnInit();
  // Lookup wether an object is already in the repository. 
  static int_ptr<T> Find(Playable* obj);
  // Factory method. Returns always the same instance for the same Playable.
  // If the specified instance already exists the parameter alias is ignored.
  static int_ptr<T> Get(Playable* obj, const xstring& alias = xstring());
  // Get an instance of the same type as the current instance for URL.
  virtual int_ptr<PlaylistBase> GetSame(Playable* obj) { return &*Get(obj); }
 protected:
  // Forward Constructor
  PlaylistRepository(Playable* content, const xstring& alias, ULONG rid) : PlaylistBase(content, alias, rid) {}
  // Unregister from the repository automatically
  ~PlaylistRepository();
};

template <class T>
sorted_vector<T, Playable> PlaylistRepository<T>::RPInst(8);
template <class T>
Mutex PlaylistRepository<T>::RPMutex;

// Implementations
template <class T>
void PlaylistRepository<T>::UnInit()
{ DEBUGLOG(("PlaylistRepository::UnInit() - %d\n", RPInst.size()));
  // Close all windows.
  // The instances deregister itself from the repository.
  // Starting at the end avoids the memcpy calls for shrinking the vector.
  size_t n = RPInst.size();
  while (n)
    RPInst[--n]->Destroy();
}

template <class T>
int_ptr<T> PlaylistRepository<T>::Find(Playable* obj)
{ DEBUGLOG(("PlaylistRepository<T>::Find(%p)\n", obj));
  Mutex::Lock lock(RPMutex);
  T* pp = RPInst.find(*obj);
  CritSect cs;
  return pp && !pp->RefCountIsUnmanaged() ? pp : NULL;
}

template <class T>
int_ptr<T> PlaylistRepository<T>::Get(Playable* obj, const xstring& alias)
{ DEBUGLOG(("PlaylistRepository<T>::Get(%p, %s)\n", obj, alias ? alias.cdata() : "<NULL>"));
  Mutex::Lock lock(RPMutex);
  T*& pp = RPInst.get(*obj);
  { CritSect cs;
    if (pp && !pp->RefCountIsUnmanaged())
      return pp;
  }
  pp = new T(obj, alias);
  return pp;
}

template <class T>
PlaylistRepository<T>::~PlaylistRepository()
{ Mutex::Lock lock(RPMutex);
  #if NDEBUG
  RPInst.erase(*Content);
  #else
  assert(RPInst.erase(*Content) != NULL);
  #endif
}


#endif
