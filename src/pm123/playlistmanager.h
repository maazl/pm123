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

#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H


#include <cpp/queue.h>
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <cpp/container.h>
#include <cpp/xstring.h>

#include "playlistbase.h"


/****************************************************************************
*
*  This class represents an Instance of a playlist manager window.
*  This is in fact nothing else but a tree view of a playlist.
*
****************************************************************************/
class PlaylistManager : public PlaylistRepository<PlaylistManager>
{ friend PlaylistRepository<PlaylistManager>;
 public:
  // C++ part of a record
  struct Record;
  struct CPData : public PlaylistBase::CPDataBase
  { Record*           Parent;    // Pointer to parent Record or NULL if we are at the root level.
    bool              Recursive; // Flag whether this node is in the current callstack.
    CPData(PlaylistManager& pm,
           void (PlaylistBase::*infochangefn)(const Playable::change_args&, RecordBase*),
           void (PlaylistBase::*statchangefn)(const PlayableInstance::change_args&, RecordBase*),
           Record* rec,
           Record* parent);
  };
  // POD part of a record
  struct Record : public RecordBase
  { // For convenience
    CPData*&        Data() { return (CPData*&)RecordBase::Data; }
  };

 private: // working set
  xstring           Info;          // Keep the window info
  HWND              MainMenu;
  HWND              ListMenu;
  HWND              FileMenu;
  int_ptr<Playable> EmFocus;       // Playable object to display at UM_SETTITLE

 private:
  // Post record message, filtered
  virtual void      PostRecordCommand(RecordBase* rec, RecordCommand cmd); 
  // create container window
  virtual void      InitDlg();
  // Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  // Set Status info 
  void              SetInfo(const xstring& info);
  // initiate the display of a record's info
  void              ShowRecordAsync(Record* rec);
  // Determine type of Playable object
  // Subfunction to CalcIcon.
  virtual ICP       GetPlayableType(RecordBase* rec);
  // Gets the Usage type of a record.
  // Subfunction to CalcIcon.
  virtual IC        GetRecordUsage(RecordBase* rec);
  // check whether the current record is recursive
  bool              RecursionCheck(Record* rec);
  // same with explicit parent for new items not yet added
  bool              RecursionCheck(Playable* pp, Record* parent);
  // Set the window title
  virtual void      SetTitle();
 private: // Modifiying function and notifications
  // create a new entry in the container
  Record*           AddEntry(PlayableInstance* obj, Record* parent, Record* after = NULL);
  Record*           MoveEntry(Record* entry, Record* parent, Record* after = NULL);
  // Removes entries recursively from the container
  // The entry object is valid after calling this function until the next DispatchMessage.
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
  // Update play status of one record
  virtual void      UpdatePlayStatus(RecordBase* rec);

 private:
  virtual void      Open(const char* URL);

 private:
  // Create a playlist manager window for an URL, but don't open it.
  PlaylistManager(const char* URL, const char* alias);
  ~PlaylistManager();
};


inline PlaylistManager::CPData::CPData(
  PlaylistManager& pm,
  void (PlaylistBase::*infochangefn)(const Playable::change_args&, RecordBase*),
  void (PlaylistBase::*statchangefn)(const PlayableInstance::change_args&, RecordBase*),
  Record* rec,
  Record* parent)
: PlaylistBase::CPDataBase(pm, infochangefn, statchangefn, rec),
  Parent(parent),
  Recursive(false)
{}


#endif
