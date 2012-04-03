/*
 * Copyright 2007-2010 Marcel Mueller
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

#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H


#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/container/inst_index.h>

#include "playlistbase.h"


/****************************************************************************
*
*  This class represents an Instance of a playlist manager window.
*  This is in fact nothing else but a tree view of a playlist.
*
****************************************************************************/
class PlaylistManager
: public PlaylistBase
{public:
  struct Record;
  /// C++ part of a record
  struct CPData : public CPDataBase
  { /// Pointer to parent Record or NULL if we are at the root level.
    Record*           Parent;
    /// Flag whether this node is in the current callstack.
    bool              Recursive;
    CPData(PlayableInstance& content, PlaylistManager& pm,
           void (PlaylistBase::*infochangefn)(const PlayableChangeArgs&, RecordBase*),
           RecordBase* rec,
           Record* parent);
  };
  /// POD part of a record
  struct Record : public RecordBase
  { // For convenience
    const CPData*   Data() const { return (const CPData*&)RecordBase::Data; }
    CPData*&        Data() { return (CPData*&)RecordBase::Data; }
  };

 private: // working set
  xstring           Info;          // Keep the window info
  HWND              MainMenu;
  HWND              RecMenu;
  int_ptr<Playable> EmFocus;       // Playable object to display at UM_SETTITLE
  bool              DecChanged2;   // Shadow of PlaylistBase::DecChanged to handle the record menu independantly.

 private:
  /// Create a playlist manager window for an object, but don't open it.
  PlaylistManager(Playable& obj);
  static PlaylistManager* Factory(Playable& key);
  static int        Comparer(const PlaylistManager& pl, const Playable& key);
  typedef inst_index<PlaylistManager, Playable, &PlaylistManager::Comparer> RepositoryType;
 public:
  static int_ptr<PlaylistManager> GetByKey(Playable& key) { return RepositoryType::GetByKey(key, &PlaylistManager::Factory); }
  /// Get an instance of the same type as the current instance for URL.
  virtual const int_ptr<PlaylistBase> GetSame(Playable& obj);
                    ~PlaylistManager() { RepositoryType::RemoveWithKey(*this, *Content); }
  static void       DestroyAll();

 private:
  /// Reduce the request flags to the level required for this record.
  /// @return The infos that are to be requested at high priority.
  virtual InfoFlags FilterRecordRequest(RecordBase* rec, InfoFlags& flags);
  /// create container window
  virtual void      InitDlg();
  /// Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  /// Load context menu for a record
  virtual HWND      InitContextMenu();
  /// Update plug-in specific accelerator table.
  virtual void      UpdateAccelTable();
  /// Set Status info
  void              SetInfo(const xstring& info);
  /// initiate the display of a record's info
  void              ShowRecordAsync(Record* rec);
  /// Determine type of Playable object
  /// Subfunction to CalcIcon.
  virtual ICP       GetPlaylistType(const RecordBase* rec) const;
  /// Gets the Usage type of a record.
  /// Subfunction to CalcIcon.
  virtual IC        GetRecordUsage(const RecordBase* rec) const;
  /// check whether the current record is recursive
  bool              RecursionCheck(const RecordBase* rec) const;
  /// same with explicit parent for new items not yet added
  bool              RecursionCheck(const Playable& pp, const RecordBase* parent) const;

 private: // Modifiying function and notifications
  /// Subfunction to the factory below.
  virtual RecordBase* CreateNewRecord(PlayableInstance& obj, RecordBase* parent);
  /// Find parent record. Returns NULL if rec is at the top level.
  virtual RecordBase* GetParent(const RecordBase* const rec) const;

  /// Update the list of children
  /// rec == NULL => root node
  virtual void      UpdateChildren(RecordBase* const rec);
  /// Update a record
  virtual void      UpdateRecord(RecordBase* rec);
  /// Update play status of one record
  virtual void      UpdatePlayStatus(RecordBase* rec);
  /// Navigate to
  virtual void      UserNavigate(const RecordBase* rec);
};


inline PlaylistManager::CPData::CPData(
  PlayableInstance& content,
  PlaylistManager& pm,
  void (PlaylistBase::*infochangefn)(const PlayableChangeArgs&, RecordBase*),
  RecordBase* rec,
  Record* parent)
: PlaylistBase::CPDataBase(content, pm, infochangefn, rec),
  Parent(parent),
  Recursive(false)
{}


#endif
