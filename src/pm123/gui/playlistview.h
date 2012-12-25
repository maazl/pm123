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

#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#define INCL_WIN

#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/container/inst_index.h>

#include "playlistbase.h"
#include <decoder_plug.h>

#include <os2.h>


class PlaylistMenu;

/****************************************************************************
*
*  This class represents an Instance of a playlist window.
*  This is in fact nothing else but a detailed view of a playlist.
*
****************************************************************************/
//static MRESULT EXPENTRY DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

class PlaylistView
: public PlaylistBase
{public:
  //struct Record;
  typedef CPDataBase CPData;
  /*struct CPData : public CPDataBase
  { // Constructor
    CPData(PlayableInstance* content, PlaylistView& pm,
           void (PlaylistBase::*infochangefn)(const PlayableChangeArgs&, RecordBase*),
           Record* rec) : CPDataBase(content, pm, infochangefn, rec) {}
  };*/
  /** Record specific structure for PM.
   * The structure uses the implicit binary compatibility
   * of xstring to const char*.
   */
  struct Record : public RecordBase
  { // Attribute references for PM
    xstring         Start;
    xstring         Stop;
    xstring         At;
    xstring         Song;
    xstring         Size;
    xstring         Time;
    xstring         MoreInfo;
    const char*     URL;
    // For convenience
    CPData*&        Data() { return (CPData*&)RecordBase::Data; }
  };
  // Column IDs for direct manipulation.
  enum ColumnID
  { CID_Alias = 1,
    CID_Start,
    CID_Stop,
    CID_At,
    CID_URL
  };
 private:
  struct Column
  { ULONG    DataAttr;
    ULONG    TitleAttr;
    const    char* Title;
    ULONG    Offset;
    ColumnID CID;
  };
 private:
  static const Column MutableColumns[];
  //static const Column ConstColumns[];
  static FIELDINFO* MutableFieldinfo;
  //static FIELDINFO* ConstFieldinfo;
  static HWND       MainMenu;
  static HWND       RecMenu;

 private:
 private:
  /// Create a playlist manager window for an URL, but don't open it.
  PlaylistView(Playable& obj);
  static PlaylistView* Factory(Playable& key);
  static int        Comparer(const Playable& key, const PlaylistView& pl);
  typedef inst_index<PlaylistView, Playable, &PlaylistView::Comparer> RepositoryType;
 public:
  static int_ptr<PlaylistView> GetByKey(Playable& key) { return RepositoryType::GetByKey(key, &PlaylistView::Factory); }
  static int_ptr<PlaylistView> FindByKey(Playable& key) { return RepositoryType::FindByKey(key); }
  // Get an instance of the same type as the current instance for URL.
  virtual const int_ptr<PlaylistBase> GetSame(Playable& obj);
                    ~PlaylistView();
  static void       DestroyAll();

 private:
  /// Reduce the request flags to the level required for this record.
  /// @return The infos that are to be requested at high priority.
  virtual InfoFlags FilterRecordRequest(RecordBase* rec, InfoFlags& flags);
  /// Initialization on the first call.
  FIELDINFO*        CreateFieldinfo(const Column* cols, size_t count);
  /// Create container window.
  virtual void      InitDlg();
  /// Dialog procedure, called by \c DlgProcStub.
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  /// Load context menu for a record.
  virtual void      InitContextMenu();
  /// Update plug-in specific accelerator table.
  virtual void      UpdateAccelTable();

  /// Determine type of Playable object
  /// Sub function to \c CalcIcon.
  virtual ICP       GetPlaylistType(const RecordBase* rec) const;
  /// Convert \a size [bytes] to a human readable format
  static const xstring FormatSize(double size);
  /// Convert \a time [s] to a human readable format
  static const xstring FormatTime(double time);
  /// Append separator and \a app to \a dst if \a app has at least length 1.
  static void       AppendNonEmpty(xstring& dst, const char* sep, const volatile xstring& app);
  /// Assign value to \a dst and \a ref and return \c true if they were different.
  static bool       UpdateColumnText(xstring& dst, const xstring& value);
  /// (re-)calculate column content, return \c true if changes are made.
  bool              CalcCols(Record* rec, InfoFlags flags);

  /// Sub function to the factory below.
  virtual RecordBase* CreateNewRecord(PlayableInstance& obj, RecordBase* parent);
  /// Find parent record. Returns \c NULL if \a rec is at the top level.
  virtual RecordBase* GetParent(const RecordBase* const rec) const;
  /// Update a record
  virtual void      UpdateRecord(RecordBase* rec);
  /// Select all
  void              UserSelectAll();
};


#endif
