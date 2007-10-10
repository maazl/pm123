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
#include <cpp/xstring.h>

#define DECODER_PLUGIN_LEVEL 2
#include "playable.h"
#include "playlist_base.h"
#include <decoder_plug.h>


/****************************************************************************
*
*  This class represents an Instance of a playlist window.
*  This is in fact nothing else but a detailed view of a playlist.
*
****************************************************************************/
static MRESULT EXPENTRY DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

class PlaylistView : public PlaylistRepository<PlaylistView>
{ friend PlaylistRepository<PlaylistView>;
 public:
  struct Record;
  struct CPData : public CPDataBase
  { // Field attributes
    xstring             Pos;
    xstring             Song;
    xstring             Size;
    xstring             Time;
    xstring             MoreInfo;
    // Constructor
    CPData(PlaylistView& pm,
           void (PlaylistBase::*infochangefn)(const Playable::change_args&, RecordBase*),
           void (PlaylistBase::*statchangefn)(const PlayableInstance::change_args&, RecordBase*),
           Record* rec) : CPDataBase(pm, infochangefn, statchangefn, rec) {}
  };
  struct Record : public RecordBase
  { // Attribute references for PM
    const char*         Pos;
    const char*         Song;
    const char*         Size;
    const char*         Time;
    const char*         MoreInfo;
    const char*         URL;
    // For convenience
    CPData*&        Data() { return (CPData*&)RecordBase::Data; }
  };
 private:
  static const struct Column
  { ULONG DataAttr;
    ULONG TitleAttr;
    const char* Title;
    ULONG Offset;
  } Columns[];
 private: // working set
  HWND              MainMenu;
  HWND              ListMenu;
  HWND              FileMenu;

 private:
  // Create a playlist manager window for an URL, but don't open it.
  PlaylistView(const char* URL, const char* alias);
  ~PlaylistView();
 
 private:
  // Post record message, filtered
  virtual void      PostRecordCommand(RecordBase* rec, RecordCommand cmd); 
  // create container window
  virtual void      InitDlg();
  // Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);

  // Determine type of Playable object
  // Subfunction to CalcIcon.
  virtual ICP       GetPlayableType(RecordBase* rec);
  // Gets the Usage type of a record.
  // Subfunction to CalcIcon.
  virtual IC        GetRecordUsage(RecordBase* rec);
  // Convert size [bytes] to a human readable format
  xstring           FormatSize(double size);
  // Convert time [s] to a human readable format
  xstring           FormatTime(double time);
  // (re-)calculate colum content, return true if changes are made
  bool              CalcCols(Record* rec, Playable::InfoFlags flags, PlayableInstance::StatusFlags iflags);

  // create a new entry in the container
  Record*           AddEntry(PlayableInstance* obj, Record* after = NULL);
  Record*           MoveEntry(Record* entry, Record* after = NULL);
  // Removes entries from the container
  // The entry object is valid after calling this function until the next DispatchMessage.
  void              RemoveEntry(Record* const entry);
  // delete all children
  void              RemoveChildren();
  // request container records 
  void              RequestChildren();
  // Update the list of children
  void              UpdateChildren();
  // Update a record
  void              UpdateRecord(Record* rec, Playable::InfoFlags flags, PlayableInstance::StatusFlags iflags);
};

