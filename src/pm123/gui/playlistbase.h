/*
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

#ifndef PLAYLISTBASE_H
#define PLAYLISTBASE_H

#define INCL_WIN

#include "../core/playableinstance.h"
#include "../core/playable.h"
#include "../engine/controller.h"
#include "ainfodialog.h"
#include <decoder_plug.h>

#include <cpp/queue.h>
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/container/vector.h>
#include <cpp/container/sorted_vector.h>
#include <cpp/xstring.h>
#include <cpp/url123.h>
#include <cpp/windowbase.h>

#include <os2.h>

#include <debuglog.h>


class PluginEventArgs;
class PlaylistMenu;

/****************************************************************************
*
*  This abstract class represents an instance of a playlist window.
*  Whether it is a tree view or a detailed view determines the derived class.
*
****************************************************************************/
class PlaylistBase
: public ManagedDialog<DialogBase>
{protected:
  /// Strongly typed, thread-safe bit vector of update flags for an item or the entire playlist.
  struct CommonState
  { /// Bit vector of outstanding update commands.
    AtomicUnsigned      UpdateFlags;
    /// Create empty update vector.
    CommonState() : UpdateFlags(0) {}
  };
  struct RecordBase;
  /// @brief C++ part of a container record.
  /// @details This is referenced by the extended \c MINIRECORDCORE structure.
  struct CPDataBase
  { /// The pointer to the back-end content.
    int_ptr<PlayableInstance> const Content;
    /// Storage for alias name <-> pszIcon
    xstring             Text;
    /// Update flags
    CommonState         EvntState;
    /// Delegate for change events of the back-end content.
    class_delegate2<PlaylistBase, const PlayableChangeArgs, RecordBase> InfoChange;
    /// Create C++ part of the record data.
    /// @param content Back-end content of this record.
    /// @param pm parent PlaylistBase
    /// @param infochangefn Function to be called when the change event of the backend content fires.
    CPDataBase(PlayableInstance& content, PlaylistBase& pm,
               void (PlaylistBase::*infochangefn)(const PlayableChangeArgs&, RecordBase*),
               RecordBase* rec);
    /// Polymorphic destructor.
    virtual             ~CPDataBase() {} // free the right objects
  };
  /// @brief POD part of a record
  /// @remarks This is a POD struct that derives from \c MINIRECORDCORE.
  /// It can be directly passed to PM as long as PM is instructed to
  /// reserve additional \c sizeof(RecordBase) - \c sizeof(MINIRECORDCORE) bytes for each record structure.
  struct RecordBase : public MINIRECORDCORE
  { /// Reference counter to keep Records alive while posted messages are on the way.
    AtomicUnsigned      UseCount;
    /// C++ part of the object in the private memory arena.
    CPDataBase*         Data;
    #ifdef DEBUG_LOG
    xstring             DebugName() const;
    static xstring      DebugName(const RecordBase* rec);
    #endif
  };

  struct HandleDrop : public HandleDragTransfers
  { int_ptr<Playable> DropTarget;     ///< Playlist where to insert dropped items.
    int_ptr<PlayableInstance> DropBefore; ///< Where to insert in the above playlist.
    HandleDrop(DRAGINFO* di, HWND hwnd) : HandleDragTransfers(di, hwnd) {}
  };
 public:
  struct UserAddCallbackParams;
  friend struct UserAddCallbackParams;
  struct UserAddCallbackParams
  { PlaylistBase* GUI;
    PlaylistBase::RecordBase* Parent;
    PlaylistBase::RecordBase* Before;
    vector_int<APlayable> Content;
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
  /// User messages related to playlist container views.
  enum
  { UM_UPDATEINFO = WM_USER+0x101,
    /// Execute a update command for a record.
    /// @param mp1 Record* or NULL
    /// The reference counter of the Record is decremented after the message is processed.
    /// The Commands to be executed are atomically taken from the Update bitvector
    /// in the referenced record or the container.
    UM_RECORDUPDATE,
    /// Delete a record structure and put it back to PM
    /// @param mp1 Record
    UM_DELETERECORD,
    /// Play status event hit.
    /// @param mp1 status
    UM_PLAYSTATUS,
    /// Asynchronous insert operation
    /// @param mp1 InsertInfo*
    /// The InsertInfo structure is deleted after the message is processed.
    UM_INSERTITEM,
    /// Remove item addressed by a record asynchronously
    /// @param mp1 Record*
    /// The reference counter of the Record is decremented after the message is processed.
    UM_REMOVERECORD,
    /// @brief Do Source rendering in Drag and drop.
    /// @param mp1 DRAGTRANSFER*
    /// @param mp2 int_ptr<Playable>
    /// @details The message expects that the passed \c Playlist already contains all the content.
    /// Only the I/O is still missing. After saving a \c DM_RENDERCOMPLETE message is sent to the target.
    UM_RENDERASYNC,
    /// Update decoder tables.
    UM_UPDATEDEC
  };
 public:
  /// return value of AnalyzeRecordTypes
  enum RecordType
  { /// record list is empty
    RT_None = 0x00,
    /// record list contains at least one song without writable meta attributes
    RT_Song = 0x01,
    /// record list contains at least one song with writable meta attributes
    RT_Meta = 0x02,
    /// record list contains at least one non-mutable list
    RT_Enum = 0x04,
    /// record list contains at least one mutable playlist
    RT_List = 0x08,
    /// record list contains at least one invalid item
    RT_Invld = 0x10,
    /// record list contains at least one unknown item (RequestInfo on the way)
    RT_Unknwn = 0x20
  };
 protected: // User actions
  struct InsertInfo
  { int_ptr<Playable>         Parent; ///< List where to insert the new item
    int_ptr<PlayableInstance> Before; ///< Item where to do the insert
    int_ptr<PlayableRef>      Item;   ///< What to insert
  };

 protected: // Cached icon resources
  /// Tree state of a playlist entry
  enum ICP
  { ICP_Empty                       ///< Empty playlist
  , ICP_Closed                      ///< non-empty playlist, collapsed
  , ICP_Open                        ///< non-empty playlist, open
  , ICP_Recursive                   ///< recursive playlist entry
  };
  /// State on a playlist entry
  enum IC
  { IC_Pending                      ///< Item not yet discovered
  , IC_Invalid                      ///< Item is invalid
  , IC_Normal                       ///< normal playlist item
  , IC_Active                       ///< playlist item is in the current call stack
  , IC_Play                         ///< item is currently playing
  , IC_Shadow                       ///< another reference of this item is currently playling
  };
  /// Playlist type
  enum IP
  { IP_Playlist                     ///< ordinary playlist
  , IP_Folder                       ///< virtual playlist (folder)
  , IP_Alternation                  ///< alternation list
  };
  // Song icons
  // Dimension 1: IC_*
  static HPOINTER   IcoSong[6];
  // Playlist icons
  // Dimension 1: [Playlist, Folder]
  // Dimension 2: IC_*
  // Dimension 3: ICP_*
  static HPOINTER   IcoPlaylist[3][6][4];

 protected: // content
  int_ptr<Playable> Content;
  HACCEL            AccelTable;     ///< Accelerator table - TODO: use shared accelerator table
  #if DEBUG_LOG
  xstring           DebugName() const;
  #endif
 protected: // working set
  HWND              HwndContainer;  ///< content window handle
  DECODER_WIZARD_FUNC LoadWizards[16];///< Current load wizards
  enum              { StaticWizzards = 3 };
  bool              NoRefresh;      ///< Avoid update events to ourself
  xstring           DirectEdit;     ///< String that holds result of direct manipulation between CN_REALLOCPSZ and CN_ENDEDIT
  CommonState       EvntState;      ///< Event State
  vector<RecordBase> Source;        ///< Array of records used for source emphasis
  bool              DragAfter;      ///< Recent drag operation was ORDERED
  HWND              HwndMenu;       ///< Window handle of last context menu
  Playable::ItemComparer SortComparer; ///< Current comparer for next sort operation
  bool              DecChanged;     ///< Flag whether the decoder table has changed since the last invokation of the context menu.
 private:
  int_ptr<PlaylistBase> Self;       ///< We hold a reference to ourself as long as the current window is open.
  PlaylistMenu*     MenuWorker;     ///< Worker for playlist content in menu. Implicitely deleted by WM_DESTROY.
  class_delegate2<PlaylistBase, const PlayableChangeArgs, RecordBase> RootInfoDelegate;
  class_delegate<PlaylistBase, const Ctrl::EventFlags> RootPlayStatusDelegate;
  class_delegate<PlaylistBase, const PluginEventArgs> PluginDelegate;

 private:
  static HPOINTER   LoadIcon(ULONG id) { return WinLoadPointer(HWND_DESKTOP, 0, id); }
  static void       InitIcons();

  static void       MsgJumpCompleted(Ctrl::ControlCommand* cmd);
 protected:
  /// Create a playlist window for an object, but don't open it.
  PlaylistBase(Playable& content, ULONG rid);

  CommonState&      StateFromRec(const RecordBase* rec) { return rec ? rec->Data->EvntState : EvntState; }
  /// Fetch the Playable object associated with rec. If rec is NULL the root object is returned.
  APlayable&        APlayableFromRec(const RecordBase* rec) const { return rec ? (APlayable&)*rec->Data->Content : *Content; }
  /// Fetch the Playable object associated with rec. If rec is NULL the root object is returned.
  Playable&         PlayableFromRec(const RecordBase* rec) const { return rec ? rec->Data->Content->GetPlayable() : *Content; }
  /// Prevent a Record from deletion until FreeRecord is called
  void              BlockRecord(RecordBase* rec)
                    { if (rec) ++rec->UseCount; }
  /// Free a record after it is no longer used e.g. because a record message sent with PostRecordCommand completed.
  /// This function does not free the record immediately. It posts a message which does the job.
  /// So you can safely access the record data until the next PM call.
  void              FreeRecord(RecordBase* rec);
  /// Post record message
  void              PostRecordUpdate(RecordBase* rec, InfoFlags flags);
  /// Reduce the request flags to the level required for this record.
  /// @return The infos that are to be requested at high priority.
  virtual InfoFlags FilterRecordRequest(RecordBase* rec, InfoFlags& flags) = 0;
  /// Request record information
  InfoFlags         RequestRecordInfo(RecordBase* rec, InfoFlags filter = ~IF_None);

  /// Gives the Record back to the PM and destroys the C++ part of it.
  /// The Record object is no longer valid after calling this function.
  void              DeleteEntry(RecordBase* entry);

  void              StartDialog();
  /// Called at WM_INITDLG
  /// Should set Container
  virtual void      InitDlg();

  /// Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);

  /// Determine type of Playable object. See ICP_* constants.
  /// Sub function to CalcIcon.
  /// This function is only called for enumerable items.
  virtual ICP       GetPlaylistState(const RecordBase* rec) const = 0;
  /// Gets the display state of a record. See IC_* constants.
  /// Sub function to CalcIcon.
  /// This function is only called for used items.
  virtual IC        GetRecordUsage(const RecordBase* rec) const;
  /// Calculate icon for a record. Content must be valid!
  HPOINTER          CalcIcon(RecordBase* rec);
  /// Set the window title
  void              SetTitle();
  /// Load context menu for a record or the entire playlist and assign \c HwndMenu.
  virtual void      InitContextMenu() = 0;
  /// Access the menu worker from derived class. Creates a worker if necessary.
  PlaylistMenu&     GetMenuWorker();
  /// Update plug-in specific accelerator table.
  virtual void      UpdateAccelTable() = 0;

  /// Sub function to the factory below.
  virtual RecordBase* CreateNewRecord(PlayableInstance& obj, RecordBase* parent) = 0;
  /// Factory: create a new entry in the container.
  RecordBase*       AddEntry(PlayableInstance& obj, RecordBase* parent, RecordBase* after);
  /// Moves the record "entry" including its sub items as child to "parent" after the record "after".
  /// If parent is NULL the record is moved to the top level.
  /// If after is NULL the record is moved to the first place of parent.
  /// The function returns the moved entry or NULL in case of an error.
  RecordBase*       MoveEntry(RecordBase* entry, RecordBase* parent, RecordBase* after);
  /// Removes entries from the container
  void              RemoveEntry(RecordBase* const rec);
  /// Find parent record. Returns NULL if \a rec is at the top level.
  virtual RecordBase* GetParent(const RecordBase* const rec) const = 0;
  /// Remove all children (recursively) and return the number of deleted objects in the given level.
  /// This function does not remove the record itself.
  /// If root is NULL the entire Collection is cleared.
  int               RemoveChildren(RecordBase* const root);
  /// Update the list of children
  /// rec == NULL => root node
  virtual void      UpdateChildren(RecordBase* const rec);

  /// Populate Source array with records with a given emphasis or an empty list if none.
  void              GetRecords(USHORT emphasis);
  /// Populate Source array for context menu or drag and drop with anchor \a rec.
  bool              GetSource(RecordBase* rec);
  /// Apply a function to all objects in the Source array.
  void              Apply2Source(void (PlaylistBase::*op)(Playable&));
  void              Apply2Source(void (PlaylistBase::*op)(RecordBase*));

  void              PopulateSetFromSource(AInfoDialog::KeyType& dest);
  /// Set or clear the emphasis of the records in the Source array.
  void              SetEmphasis(USHORT emphasis, bool set) const;
  /// Analyze the records in the Source array for it's types.
  RecordType        AnalyzeRecordTypes() const;
  /// Return true if and only if \a rec is a sub item of the currently loaded root.
  /// This will not return true if \a rec points directly to the current root.
  bool              IsUnderCurrentRoot(RecordBase* rec) const;

 protected: // Update Functions.
            // They are logically virtual, but they are not called from this class.
            // They may be used in the window procedure of derived classes.
  /// Update the icon of a record
  void              UpdateIcon(RecordBase* rec);
  /// Update the information from a playable instance
  virtual void      UpdateRecord(RecordBase* rec) = 0;
  /// Update status of all active PlayableInstances
  void              UpdatePlayStatus();
  /// Update play status of one record
  virtual void      UpdatePlayStatus(RecordBase* rec);

 protected: // Notifications by the underlying Playable objects.
  /// This function is called when meta, status or technical information of a node changes.
  void              InfoChangeEvent(const PlayableChangeArgs& args, RecordBase* rec);
  /// This function is called when playing starts or stops.
  void              PlayStatEvent(const Ctrl::EventFlags& flags);
  /// This function is called when the list of enabled plug-ins changed.
  void              PluginEvent(const PluginEventArgs& args);

 protected: // User actions
  /// Add Item
  void              UserAdd(DECODER_WIZARD_FUNC wizard, RecordBase* parent = NULL, RecordBase* before = NULL);
  /// Insert a new item
  void              UserInsert(const InsertInfo* pii);
  /// Remove item by Record pointer
  void              UserRemove(RecordBase* rec);
  /// Flatten item by Record pointer, non-recursive
  void              UserFlatten(RecordBase* rec);
  /// Flatten item by Record pointer, recursive
  void              UserFlattenAll(RecordBase* rec);
  /// Save list
  void              UserSave(bool saveas);
  /// Navigate to
  void              UserNavigate(const RecordBase* rec);
  /// Open tree view
  void              UserOpenTreeView(Playable& p);
  /// Open detailed view
  void              UserOpenDetailedView(Playable& p);
  /// View Info
  void              UserOpenInfoView(const AInfoDialog::KeyType& set, AInfoDialog::PageNo page);
  /// Clear playlist
  void              UserClearPlaylist(Playable& p);
  /// Refresh records
  void              UserReload(Playable& p);
  /// Edit meta data
  void              UserEditMeta();
  /// Sort records
  void              UserSort(Playable& p);
  /// Place records in random order.
  void              UserShuffle(Playable& p);
  // comparers
  static int        CompURL(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompTitle(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompArtist(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompAlbum(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompAlias(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompSize(const PlayableInstance* l, const PlayableInstance* r);
  static int        CompTime(const PlayableInstance* l, const PlayableInstance* r);

 protected: // D'n'd target
  /// Handle CN_DRAGOVER/CN_DRAGAFTER
  MRESULT           DragOver(DRAGINFO* pdinfo, RecordBase* target);
  /// Handle CN_DROP
  void              DragDrop(DRAGINFO* pdinfo, RecordBase* target);
  /// Handle DM_RENDERCOMPLETE
  void              DropRenderComplete(DRAGTRANSFER* pdtrans, USHORT flags);
 protected: // D'n'd source
  /// Handle CN_INITDRAG
  void              DragInit();
  /// Handle DM_DISCARDOBJECT
  ULONG             DropDiscard(DRAGINFO* pdinfo);
  /// Handle DM_RENDER
  BOOL              DropRender(DragTransfer pdtrans);
  /// Handle UM_RENDERASYNC
  void              DropRenderAsync(DragTransfer pdtrans, Playable* pp);
  /// Handle DM_ENDCONVERSATION
  void              DropEnd(vector<RecordBase>* source, bool ok);

 public: // public interface
  virtual           ~PlaylistBase();
  /// Gets the content
  Playable*         GetContent() { return Content; }
  /// Get an instance of the same type as the current instance for URL.
  virtual const int_ptr<PlaylistBase> GetSame(Playable& obj) = 0;

};
FLAGSATTRIBUTE(PlaylistBase::RecordType);

inline PlaylistBase::CPDataBase::CPDataBase(
  PlayableInstance& content,
  PlaylistBase& pm,
  void (PlaylistBase::*infochangefn)(const PlayableChangeArgs&, RecordBase*),
  RecordBase* rec)
: Content(&content),
  InfoChange(pm, infochangefn, rec)
{}


#endif
