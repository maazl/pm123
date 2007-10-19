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

#ifndef PLAYLIST_BASE_H
#define PLAYLIST_BASE_H

#define INCL_WIN

#include <cpp/queue.h>
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <cpp/container.h>
#include <cpp/xstring.h>

#define DECODER_PLUGIN_LEVEL 2
#include "playable.h"
#include <decoder_plug.h>
#include <os2.h>

#include <debuglog.h>


/****************************************************************************
*
*  This abstract class represents an instance of a playlist window.
*  Whether it is a tree view or a detailed view detremines the derived class.
*
****************************************************************************/
class PlaylistBase : public IComparableTo<char>
{public:
  struct CommonState
  { unsigned            PostMsg;   // Bitvector of outstanding record commands
    bool                WaitUpdate;// UpdateTech is waiting for UpdateInfo event.
    CommonState() : PostMsg(0), WaitUpdate(false) {}
  };
  // C++ part of a record
  struct RecordBase;
  struct CPDataBase
  { class_delegate2<PlaylistBase, const Playable::change_args        , RecordBase*> InfoChange;
    class_delegate2<PlaylistBase, const PlayableInstance::change_args, RecordBase*> StatChange;
    xstring             Text;      // Storage for alias name <-> pszIcon
    CPDataBase(PlaylistBase& pm,
               void (PlaylistBase::*infochangefn)(const Playable::change_args&,         RecordBase*),
               void (PlaylistBase::*statchangefn)(const PlayableInstance::change_args&, RecordBase*),
               RecordBase* rec);
    virtual             ~CPDataBase() {} // free the right objects
    virtual void        DeregisterEvents();
  };
  // POD part of a record
  struct RecordBase : public MINIRECORDCORE
  { PlayableInstance*   Content;   // The pointer to the backend content may be NULL if the object is deleted.
    volatile unsigned   UseCount;  // Reference counter to keep Records alive while Posted messages are on the way.
    CommonState         EvntState;
    CPDataBase*         Data;      // C++ part of the object in the private memory arena.
    #ifdef DEBUG
    xstring             DebugName() const;
    static xstring      DebugName(const RecordBase* rec);
    #endif
    bool                IsRemoved() const                { return Content == NULL; }
    static bool         IsRemoved(const RecordBase* rec) { return rec != NULL && rec->IsRemoved(); }
  };
 protected:
  // Message Processing
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
    // Mark the record as removed
    // This destroys the Content reference.
    // You may send this message with async flag to free the record if it is no longer used by an outstanding message.
    UM_SYNCREMOVE,
    // Playstatus-Event hit.
    // mp1 = status
    UM_PLAYSTATUS
  };
  // Valid flags in PostMsg fields
  enum RecordCommand
  { // Update the children of the Record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATECHILDREN,
    // Update the format information of a record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATEFORMAT,
    // Update the technical information of a record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATETECH,
    // Update the meta information of a record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATEMETA,
    // Update the status of a record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATESTATUS,
    // Update the alias text of a record
    RC_UPDATEALIAS,
    // Update starting position
    RC_UPDATEPOS
  };
  // wrap pointer to keep PM happy
  struct init_dlg_struct
  { USHORT        size;
    PlaylistBase* pm;
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
  static HPOINTER   IcoPlayable[5][4];
  static HPOINTER   IcoInvalid;
  static void       InitIcons();

 protected: // content
  int_ptr<Playable> Content;
  xstring           Alias;         // Alias name for the window title
  #if DEBUG
  xstring           DebugName() const;
  #endif
 protected: // working set
  HWND              HwndFrame;     // playlist window handle
  HWND              HwndContainer; // content window handle
  xstring           Title;         // Keep the window title
  RecordBase*       CmFocus;       // Focus of last selected context menu item
  DECODER_WIZZARD_FUNC LoadWizzards[20]; // Current load wizzards
  bool              NoRefresh;     // Avoid update events to ourself
  CommonState       EvntState;     // Event State
 private:
  class_delegate2<PlaylistBase, const Playable::change_args, RecordBase*> RootInfoDelegate;
  class_delegate<PlaylistBase, const bool> RootPlayStatusDelegate;

 protected:
  // Create a playlist manager window for an URL, but don't open it.
  PlaylistBase(const char* URL, const char* alias);

  CommonState&      StateFromRec(RecordBase* rec) { return rec ? rec->EvntState : EvntState; }
  Playable*         PlayableFromRec(RecordBase* rec) { return rec ? &rec->Content->GetPlayable() : &*Content; }
  // Prevent a Record from deletion until FreeRecord is called
  void              BlockRecord(RecordBase* rec)
                    { if (rec) InterlockedInc(rec->UseCount); }
  // Free a record after it is no longer used e.g. because a record message sent with PostRecordCommand completed.
  void              FreeRecord(RecordBase* rec);
  // Post record message
  virtual void      PostRecordCommand(RecordBase* rec, RecordCommand cmd); 

  // Gives the Record back to the PM and destoys the C++ part of it.
  // The Record object is no longer valid after calling this function.
  void              DeleteEntry(RecordBase* entry);

  // Called at WM_INITDLG
  // Should set Container 
  virtual void      InitDlg();
  
  // Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  // Static members must not use EXPENTRY linkage with IBM VACPP.
  friend MRESULT EXPENTRY pl_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  
  // Determine type of Playable object
  // Subfunction to CalcIcon.
  virtual ICP       GetPlayableType(RecordBase* rec) = 0;
  // Gets the Usage type of a record.
  // Subfunction to CalcIcon.
  virtual IC        GetRecordUsage(RecordBase* rec) = 0;
  // Calculate icon for a record. Content must be valid!
  HPOINTER          CalcIcon(RecordBase* rec);
  // Set the window title
  virtual void      SetTitle();

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
  void              PlayStatEvent(const bool& status);

 protected: // User actions
  // Add Item
  void              UserAdd(DECODER_WIZZARD_FUNC wizzard, const char* title, RecordBase* parent = NULL, RecordBase* before = NULL);
  // Select a list with a file dialog.
  static url        PlaylistSelect(HWND owner, const char* title);

 public: // public interface
  // Make the window visible (or not)
  void              SetVisible(bool show);
  bool              GetVisible() const { return WinIsWindowVisible(HwndFrame); }
  // Gets the content
  Playable*         GetContent() { return Content; }
  // Get an instance of the same type as the current instance for URL.
  virtual PlaylistBase* GetSame(const url& URL) = 0;
  
 private: // IComparableTo<char>
  virtual int       CompareTo(const char* str) const;
  
};


inline PlaylistBase::CPDataBase::CPDataBase(
  PlaylistBase& pm,
  void (PlaylistBase::*infochangefn)(const Playable::change_args&,         RecordBase*),
  void (PlaylistBase::*statchangefn)(const PlayableInstance::change_args&, RecordBase*),
  RecordBase* rec)
: InfoChange(pm, infochangefn, rec),
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
  static sorted_vector<T, char> RPInst;
  static Mutex      RPMutex;
 public:
  // currently a no-op
  // TODO: we should cleanup unused and invisible instances once in a while.
  static void       Init() {}
  static void       UnInit();          
  // Factory method. Returns always the same instance for the same URL.
  // If the specified instance already exists the parameter alias is ignored.
  static T*         Get(const char* url, const char* alias = NULL);
  // Get an instance of the same type as the current instance for URL.
  virtual PlaylistBase* GetSame(const url& URL) { return Get(URL); }
 protected:
  // Forward Constructor
  PlaylistRepository(const char* URL, const char* alias) : PlaylistBase(URL, alias) {}
  // Unregister from the repository automatically
  ~PlaylistRepository();
};

template <class T>
sorted_vector<T, char> PlaylistRepository<T>::RPInst(8);
template <class T>
Mutex PlaylistRepository<T>::RPMutex;

// Implementations
template <class T>
void PlaylistRepository<T>::UnInit()
{ DEBUGLOG(("PlaylistRepository::UnInit() - %d\n", RPInst.size()));
  // Free stored instances.
  // The instances deregister itself from the repository.
  // Starting at the end avoids the memcpy calls for shrinking the vector.
  while (RPInst.size())
    delete RPInst[RPInst.size()-1];
}

template <class T>
T* PlaylistRepository<T>::Get(const char* url, const char* alias)
{ DEBUGLOG(("PlaylistRepository<T>::Get(%s, %s)\n", url, alias ? alias : "<NULL>"));
  Mutex::Lock lock(RPMutex);
  T*& pp = RPInst.get(url);
  /*int_ptr<T> rp;
  // This assignment will not catch objects that are about to be deleted.
  rp.assign_weak(pp);
  if (!rp)
    // In case the above assignment returned zero while the pointer is still in the repository
    // the repository is updated immediately.
  */ 
  if (!pp)
    pp = new T(url, alias);
  return pp;
}

template <class T>
PlaylistRepository<T>::~PlaylistRepository()
{ Mutex::Lock lock(RPMutex);
  size_t pos;
  /*#if NDEBUG
  RPInst.binary_search(Content->GetURL(), pos);
  #else
  assert(RPInst.binary_search(Content->GetURL(), pos));
  #endif
  // Only delete the index if it is really our own instance.
  // A different instance might be in the repository if someone got a new instance
  // by calling get while the reference counter of this instance was already zero
  // but this code has not yet been executed.
  if (RPInst[pos] == this)
    RPInst.erase(pos);*/
  #if NDEBUG
  RPInst.erase(((T*)this)->Content->GetURL());
  #else
  assert(RPInst.erase(((T*)this)->Content->GetURL()) != NULL);
  #endif
}


#endif
