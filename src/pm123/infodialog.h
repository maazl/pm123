/*
 * Copyright 2008-2008 Marcel Mueller
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

#ifndef PM123_INFODIALOG_H
#define PM123_INFODIALOG_H

#define INCL_WIN
#define INCL_BASE
#include "windowbase.h"
#include "playable.h"
#include <cpp/container/inst_index.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <cpp/event.h>
#include <os2.h>


/****************************************************************************
*
*  Class to handle info dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class InfoDialog 
: public ManagedDialogBase,
  private OwnedPlayableSet, // aggregation is not sufficient because of the destruction sequence
  public IComparableTo<const PlayableSetBase*>,
  public inst_index<InfoDialog, const PlayableSetBase*const> // depends on base class OwnedPlayableSet
{public:
  enum PageNo
  { Page_MetaInfo,
    Page_TechInfo
  };
 protected:
  enum Fields
  { F_none        = 0x00000000,
    // misc
    F_decoder     = 0x00000001,
    // FORMAT_INFO2
    F_samplerate  = 0x00000004,
    F_channels    = 0x00000008,
    F_FORMAT_INFO2= 0x0000000c,
    // TECH_INFO
    F_songlength  = 0x00000010,
    F_bitrate     = 0x00000020,
    F_totalsize   = 0x00000040,
    F_info        = 0x00000080,
    F_TECH_INFO   = 0x000000f0,
    // META_INFO
    F_title       = 0x00000100,
    F_artist      = 0x00000200,
    F_album       = 0x00000400,
    F_year        = 0x00000800,
    F_comment     = 0x00001000,
    F_genre       = 0x00002000,
    F_track       = 0x00004000,
    F_copyright   = 0x00008000,
    F_track_gain  = 0x00010000,
    F_track_peak  = 0x00020000,
    F_album_gain  = 0x00040000,
    F_album_peak  = 0x00080000,
    F_META_INFO   = 0x000fff00,
    // PHYS_INFO
    F_filesize    = 0x01000000,
    F_num_items   = 0x02000000,
    F_PHYS_INFO   = 0x03000000,
    // RPL_INFO
    F_total_items = 0x10000000,
    F_recursive   = 0x20000000,
    F_RPL_INFO    = 0x30000000
  };
  CLASSFLAGSATTRIBUTE(Fields);
  struct Data; // ICC Work-Around
  friend struct Data;
  struct Data
  { const DECODER_INFO2* Info;
    const char*     Decoder;
    Fields          Enabled; // The following fields are shown enabled (= the content makes sense)
    Fields          Valid;   // The specified field content is valid an applies to all of the content
  };
 private:
  // Functor for inst_index factory.
  class Factory;
  friend class Factory;
  class Factory : public inst_index<InfoDialog, const PlayableSetBase*const>::IFactory
  { // singleton
    Factory() {}
   public:
    static Factory  Instance;
    virtual InfoDialog* operator()(const PlayableSetBase*const& key);
  };
 protected:
  // Base class for notebook pages
  class PageBase;
  friend class PageBase;
  class PageBase : public DialogBase
  {public:
    enum
    { UM_UPDATE = WM_USER+1
    };
   protected:
    InfoDialog&     Parent;
    Fields          Enabled; // Valid only while UM_UPDATE
    Fields          Valid;   // Valid only while UM_UPDATE
   protected:
    PageBase(InfoDialog& parent, ULONG rid);
    HWND            SetCtrlText(USHORT id, Fields fld, const char* text); 
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    void            StartDialog() { DialogBase::StartDialog(Parent.GetHwnd(), Parent.GetHwnd()); }
  };
  // Notebook page 1
  class PageTechInfo;
  friend class PageTechInfo;
  class PageTechInfo : public PageBase
  { // Dialog procedure
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    PageTechInfo(InfoDialog& parent, ULONG rid) : PageBase(parent, rid) {}
  };
  // Notebook page 2
  class PageMetaInfo;
  friend class PageMetaInfo;
  class PageMetaInfo : public PageBase
  {private:
    bool            MetaWrite; // Valid only while UM_UPDATE
   private:
    // Dialog procedure
    HWND            SetEFText(USHORT id, Fields fld, const char* text); 
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    PageMetaInfo(InfoDialog& parent, ULONG rid) : PageBase(parent, rid) {}
  };
 protected:
  // Progress window for meta data writes
  class MetaWriteDlg : private DialogBase
  {private: // Work order!
    const META_INFO& MetaData; // Information to write
    const int       MetaFlags; // Parts of MetaData to write
    const PlayableSetBase& Dest;// Write to this songs
   private:
    enum
    { UM_START = WM_USER+1,    // Start the worker thread
      UM_STATUS                // Worker thread reports: at item #(ULONG)mp1, status (ULONG)mp2
    };
    bool            SkipErrors;
   private:
   private: // Worker thread
    TID             WorkerTID;
    int             CurrentItem;
    bool            Cancel;
    Event           ResumeSignal;
   private:
    // Worker thread
    void            Worker();
    friend void TFNENTRY InfoDialogMetaWriteWorkerStub(void*);
   protected:
    // Dialog procedure
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    MetaWriteDlg(const META_INFO& info, int flags, const PlayableSetBase& dest);
    ~MetaWriteDlg();
    ULONG           DoDialog(HWND owner);
  };

 protected:
  PageTechInfo      PageTech;
  PageMetaInfo      PageMeta;
  ULONG             PageIDs[2];

 private: // non copyable
                    InfoDialog(const InfoDialog&);
  void              operator=(const InfoDialog&);
 protected:
  // Initialize InfoDialog
                    InfoDialog(const PlayableSetBase& key);
  void              StartDialog();
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual struct Data GetData() = 0;

 public:
  virtual           ~InfoDialog();
  void              ShowPage(PageNo page);

  // Factory method. Returns always the same instance for the same set of objects.
  static int_ptr<InfoDialog> GetByKey(const PlayableSetBase& obj);
  // Factory method. Returns always the same instance for the same Playable.
  static int_ptr<InfoDialog> GetByKey(Playable* obj);
  // IComparableTo<Playable>
  virtual int       compareTo(const PlayableSetBase*const& r) const;
};


#endif
