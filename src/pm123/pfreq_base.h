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

#include <cpp/queue.h>
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <cpp/container.h>

#include "playable.h"
#include "playlist_base.h"


/****************************************************************************
*
*  This class represents an Instance of a playlist manager window.
*  This is in fact nothing else but a tree view of a playlist.
*
****************************************************************************/
static MRESULT EXPENTRY DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
static void TFNENTRY WorkerStub(void* arg);

class PlaylistManager : public PlaylistBase
{public:
  struct Record : public RecordBase
  { struct record_data
    { Record*           Parent;    // Pointer to parent Record or NULL if we are at the root level.
      bool              Recursive; // Flag whether this node is in the current callstack.
      class_delegate2<PlaylistManager, const Playable::change_args          , Record*> InfoChange;
      class_delegate2<PlaylistManager, const PlayableInstance::change_args  , Record*> StatChange;
      xstring Text;
      record_data(PlaylistManager& pm,
                  void (PlaylistManager::*infochangefn)(const Playable::change_args&, Record*),
                  void (PlaylistManager::*statchangefn)(const PlayableInstance::change_args&, Record*),
                  Record* rec,
                  Record* parent);
      void              DeregisterEvents();
    };
    record_data*        Data;      // C++ part of the object in the private memory arena.
  };

 private: // working set
  xstring           Info;          // Keep the window info
  HWND              MainMenu;
  HWND              ListMenu;
  HWND              FileMenu;
  int_ptr<Playable> EmFocus;       // Playable object to display at UM_SETTITLE

 private:
  // create container window
  void              CreateContainer(int id);
  // Dialog procedure, called by DlgProcStub
  MRESULT           DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  // Static members must not use EXPENTRY linkage with IBM VACPP.
  friend MRESULT EXPENTRY DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  // Set the window title
  void              SetTitle(const xstring& title);
  // Set Status info 
  void              SetInfo(const xstring& info);
  // initiate the display of a record's info
  void              ShowRecordAsync(Record* rec);
  // Gets the Usage type of a record.
  // Subfunction to CalcIcon.
  static IC         GetRecordUsage(Record* rec);
  // Calculate icon for a record. Content must be valid!
  static HPOINTER   CalcIcon(Record* rec);
  // check whether the current record is recursive
  bool              RecursionCheck(Record* rec);
  // same with explicit parent for new items not yet added
  bool              RecursionCheck(Playable* pp, Record* parent);
 private: // Modifiying function and notifications
  // Removes entries recursively from the container
  // The entry object is no longer valid after calling this function.
  void              DeleteEntry(Record* entry);
  // create a new entry in the container
  Record*           AddEntry(PlayableInstance* obj, Record* parent, Record* after = NULL);
  Record*           MoveEntry(Record* entry, Record* parent, Record* after = NULL);
  // Removes entries recursively from the container
  // The entry object is no longer valid after calling this function.
  void              RemoveEntry(Record* const entry);
  // delete all children
  int               RemoveChildren(Record* const rec);
  // Update the list of children
  // rec == NULL => root node
  void              UpdateChildren(Record* const rec);
  // Update the list of children (if available) or schedule a request.
  void              RequestChildren(Record* rec);
  // Update the tech info of a record
  void              UpdateTech(Record* rec);
  // Update the status of a record
  void              UpdateStatus(Record* rec);
  // Update the alias of a record
  void              UpdateAlias(Record* rec);
 private: // Notifications by the underlying Playable objects.
  // This function is called when meta, status or technical information of a node changes.
  void              InfoChangeEvent(const Playable::change_args& args, Record* rec);
  // This function is called when statusinformation of a PlayableInstance changes.
  void              StatChangeEvent(const PlayableInstance::change_args& inst, Record* rec);
  class_delegate2<PlaylistManager, const Playable::change_args, Record*> RootInfoDelegate;
 private: // User actions
  // Add Item
  void              UserAdd(DECODER_WIZZARD_FUNC wizzard, const char* title, Record* parent = NULL, Record* after = NULL);

 private:
  // Create a playlist manager window for an URL, but don't open it.
  PlaylistManager(const char* URL, const char* alias);
 public: // public interface
  ~PlaylistManager();
  // Make the window visible (or not)
  virtual void      SetVisible(bool show);
  
 // Repository of PM instances by URL.
 private:
  static sorted_vector<PlaylistManager, char> RPInst;
  static Mutex      RPMutex;
 private:
  virtual int       CompareTo(const char* str) const;
 public:
  static void       Init();
  static void       UnInit();          
  // Factory method. Returns always the same instance for the same URL.
  // If the specified instance already esists the parameter alias is ignored.
  static PlaylistManager* Get(const char* url, const char* alias = NULL);
};


inline PlaylistManager::Record::record_data::record_data(
  PlaylistManager& pm,
  void (PlaylistManager::*infochangefn)(const Playable::change_args&, Record*),
  void (PlaylistManager::*statchangefn)(const PlayableInstance::change_args&, Record*),
  Record* rec,
  Record* parent)
: Parent(parent),
  Recursive(false),
  InfoChange(pm, infochangefn, rec),
  StatChange(pm, statchangefn, rec)
{}

inline void PlaylistManager::Record::record_data::DeregisterEvents()
{ InfoChange.detach();
  StatChange.detach();
}

