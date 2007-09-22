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
#include <decoder_plug.h>

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
  struct RecordBase : public MINIRECORDCORE
  { PlayableInstance*   Content;   // The pointer to the backend content may be NULL if the object is deleted.
    volatile unsigned   UseCount;  // Reference counter to keep Records alive while Posted messages are on the way.
    CommonState         EvntState;
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
    UM_SYNCREMOVE
  };
  // Valid flags in PostMsg fields
  enum RecordCommand
  { // Update the children of the Record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATECHILDREN,
    // Update the technical information of a record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATETECH,
    // Update the status of a record
    // If mp2 == NULL the root node is refreshed. 
    RC_UPDATESTATUS,
    // Update the alias text of a record
    RC_UPDATEALIAS
  };
  // wrap pointer to keep PM happy
  struct init_dlg_struct
  { USHORT        size;
    PlaylistBase* pm;
  };
  
 protected: // Cached Icon ressources
  enum IC
  { IC_Normal,
    IC_Used,
    IC_Play
  };
  enum ICP
  { ICP_Closed,
    ICP_Open,
    ICP_Recursive
  };
  static HPOINTER   IcoSong[3];
  static HPOINTER   IcoPlaylist[3][3];
  static HPOINTER   IcoEmptyPlaylist;
  static HPOINTER   IcoInvalid;
  static void       InitIcons();

 protected: // content
  int_ptr<Playable> Content;
  xstring           Alias;         // Alias name for the window title
  #if DEBUG
  xstring           DebugName() const;
  #endif
 protected: // working set
  HWND              HwndFrame;     // playlist manager window handle
  HWND              HwndContainer; // content window handle
  xstring           Title;         // Keep the window title
  RecordBase*       CmFocus;       // Focus of last selected context menu item
  DECODER_WIZZARD_FUNC LoadWizzards[20]; // Current load wizzards
  bool              NoRefresh;     // Avoid update events to ourself
  CommonState       EvntState;     // Event State

 protected:
  // Create a playlist manager window for an URL, but don't open it.
  PlaylistBase(const char* URL, const char* alias);

  CommonState&      StateFromRec(RecordBase* rec) { return rec ? rec->EvntState : EvntState; }
  // Post record message
  void              PostRecordCommand(RecordBase* rec, RecordCommand cmd); 
  // Free a record after a record message sent with PostRecordCommand completed.
  void              FreeRecord(RecordBase* rec);
  

  // Dialog procedure, called by DlgProcStub
  //virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);


 public: // public interface
  //~PlaylistBase();
  // Make the window visible (or not)
  void              SetVisible(bool show);

 private: // IComparableTo<char>
  virtual int       CompareTo(const char* str) const;
  
};

