/*
 * Copyright 2007-2011 Marcel Mueller
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
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


#define INCL_BASE
#define INCL_PM
#include "infodialog.h"
#include "dependencyinfo.h"
#include "location.h"
#include "pm123.h"
#include "gui.h"
#include "pm123.rc.h"
#include "dialog.h"
#include "configuration.h"
#include "eventhandler.h"
#include <decoder_plug.h>
#include <plugin.h>
#include <utilfct.h>

#include <cpp/container/inst_index.h>
#include <cpp/pmutils.h>
#include <cpp/dlgcontrols.h>
#include <cpp/cppvdelegate.h>

#include <stdio.h>
#include <time.h>
#include <math.h>


int AInfoDialog::KeyType::compare(const KeyType& l, const KeyType& r)
{ if (&l == &r)
    return 0; // Comparison to itself
  size_t p1 = 0;
  size_t p2 = 0;
  for (;;)
  { // termination condition
    if (p2 == r.size())
    { if (p1 == l.size())
        break;
      return 1;
    } else if (p1 == l.size())
      return -1;
    // compare content
    if (l[p1] > r[p2])
      return 1;
    if (l[p1] < r[p2])
      return -1;
    ++p1;
    ++p2;
  }
  // compare parent (if any)
  if (l.Parent.get() < r.Parent.get())
    return -1;
  return l.Parent.get() > r.Parent.get();
}


/****************************************************************************
*
*  Class to handle info dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class InfoDialog
: public AInfoDialog
{protected:
  enum Fields
  { F_none        = 0x00000000,
    F_URL         = 0x00000001,
    // PHYS_INFO
    F_filesize    = 0x00000002,
    F_timestamp   = 0x00000004,
    F_fattr       = 0x00000008,
    F_PHYS_INFO   = 0x0000000e,
    // TECH_INFO
    F_samplerate  = 0x00000010,
    F_channels    = 0x00000020,
    F_objtype     = 0x00000040,
    F_info        = 0x00000080,
    F_format      = 0x00000100,
    F_decoder     = 0x00000200,
    F_TECH_INFO   = 0x000003f0,
    // ATTR_INFO and ITEM_INFO
    F_ATTR_INFO   = 0x00000400,
    F_ITEM_INFO   = 0x00000800,
    // OBJ_INFO
    F_songlength  = 0x00001000,
    F_bitrate     = 0x00002000,
    F_num_items   = 0x00004000,
    F_OBJ_INFO    = 0x00007000,
    // META_INFO
    F_title       = 0x00010000,
    F_artist      = 0x00020000,
    F_album       = 0x00040000,
    F_year        = 0x00080000,
    F_comment     = 0x00100000,
    F_genre       = 0x00200000,
    F_track       = 0x00400000,
    F_copyright   = 0x00800000,
    F_track_gain  = 0x01000000,
    F_track_peak  = 0x02000000,
    F_album_gain  = 0x04000000,
    F_album_peak  = 0x08000000,
    F_META_INFO   = 0x0fff0000,
    // RPL_INFO
    F_total_songs = 0x10000000,
    F_total_lists = 0x20000000,
    F_recursive   = 0x40000000,
    F_RPL_INFO    = 0x70000000,
    // DRPL_INFO
    F_totalsize   = 0x80000000,
    //F_totallength -> F_songlength
    F_DRPL_INFO   = 0x80000000,

    F_ALL         = 0xffffffff
  };
  CLASSFLAGSATTRIBUTE(Fields);
  struct Data
  { AllInfo         Info;
    xstring         URL;
    Fields          Enabled; // The following fields are shown enabled (= the content makes sense)
    Fields          Valid;   // The specified field content is valid and applies to all of the content
  };
 protected:
  // Base class for notebook pages
  class PageBase : public NotebookDialogBase::PageBase
  {public:
    enum
    { UM_UPDATE = WM_USER+1
    };
   protected:
    //InfoDialog&     Parent;
    Fields          Enabled; // Valid only while UM_UPDATE
    Fields          Valid;   // Valid only while UM_UPDATE
   protected:
                    PageBase(InfoDialog& parent, ULONG rid, const xstring& title);
    InfoDialog&     GetParent() { return (InfoDialog&)Parent; }
    xstring         QueryItemTextOrNULL(ULONG id);
    void            CheckButton(ULONG id, unsigned state);
    HWND            SetCtrlText(ULONG id, Fields fld, const char* text);
    HWND            SetCtrlCB(ULONG id, Fields fld, bool flag);
    void            SetCtrlRB(ULONG id1, Fields fld, int btn);
    bool            SetCtrlEFValid(ULONG id, bool valid);
    static const char* FormatInt(char* buffer, int value, const char* unit = "");
    static const char* FormatGain(char* buffer, float gain);
    static const char* FormatTstmp(char* buffer, time_t time);
    static const char* FormatDuration(char* buffer, PM123_TIME time);
    //static bool     ParseDuration(const char* text, float& duration);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    /// Return the InfoFlags to be requested
    virtual InfoFlags GetRequestFlags() = 0;
  };
  // Notebook page 1
  class PageTechInfo;
  friend class PageTechInfo;
  class PageTechInfo : public PageBase
  { // Dialog procedure
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    PageTechInfo(InfoDialog& parent) : PageBase(parent, DLG_TECHINFO, "Tech. info") {}
    virtual InfoFlags GetRequestFlags();
  };
  // Notebook page 2
  class PageMetaInfo;
  friend class PageMetaInfo;
  class PageMetaInfo : public PageBase
  {private:
    bool            MetaWrite;  // Valid only while UM_UPDATE
   private:
    // Dialog procedure
    HWND            SetEFText(ULONG id, Fields fld, const char* text);
    inline bool     FetchField(ULONG idcb, ULONG idef, xstring& field);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    PageMetaInfo(InfoDialog& parent) : PageBase(parent, DLG_METAINFO, "Meta info") {}
    virtual InfoFlags GetRequestFlags();
  };
 protected:
  // Progress window for meta data writes
  class MetaWriteDlg : private DialogBase
  {public:
    META_INFO       MetaData;  // Information to write
    DECODERMETA     MetaFlags;// Parts of MetaData to write
    const vector<APlayable>& Dest;// Write to this songs
   private:
    struct StatusReport
    { int           RC;
      xstring       Text;
     private:
      EventHandler::Handler OldHandler;
      VDELEGATE     VDErrorHandler;
      friend void DLLENTRY InfoDialogMetaWriteErrorHandler(StatusReport* that, MESSAGE_TYPE type, const xstring& msg);
     public:
      StatusReport();
      ~StatusReport() { EventHandler::SetLocalHandler(OldHandler); }
    };
    // Friend nightmare because InfoDialogMetaWriteErrorHandler can't be a static member function.
    friend class InfoDialog;
    friend void DLLENTRY InfoDialogMetaWriteErrorHandler(StatusReport* that, MESSAGE_TYPE type, const xstring& msg);
    enum
    { UM_START = WM_USER+1,     // Start the worker thread
      UM_STATUS                 // Worker thread reports: at item #(ULONG)mp1, status (StatusReport)mp2
    };
    bool            SkipErrors;
   private: // Worker thread
    TID             WorkerTID;
    unsigned        CurrentItem;
    bool            Cancel;
    Event           ResumeSignal;
   private: // Worker thread
    void            Worker();
    friend void TFNENTRY InfoDialogMetaWriteWorkerStub(void*);
   protected:
    // Dialog procedure
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    MetaWriteDlg(const vector<APlayable>& dest);
    ~MetaWriteDlg();
    ULONG           DoDialog(HWND owner);
  };
  friend void TFNENTRY InfoDialogMetaWriteWorkerStub(void*);
  friend void DLLENTRY InfoDialogMetaWriteErrorHandler(MetaWriteDlg::StatusReport* that, MESSAGE_TYPE type, const xstring& msg);

 public:
  const   KeyType   Key;

 private: // non copyable
                    InfoDialog(const InfoDialog&);
          void      operator=(const InfoDialog&);
 protected:
  // Initialize InfoDialog
                    InfoDialog(const KeyType& key);
          PageBase* PageFromID(ULONG pageid) { return (PageBase*)AInfoDialog::PageFromID(pageid); }
  virtual void      RequestPage(PageBase* page);
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);

 public:
  virtual           ~InfoDialog();
  virtual void      StartDialog() { ManagedDialog<NotebookDialogBase>::StartDialog(HWND_DESKTOP, NB_INFO); }
  virtual void      ShowPage(PageNo page);
  virtual const struct Data& GetData() = 0;

 private: // Repository
  static InfoDialog* Factory(const KeyType& key);
  static int        Comparer(const KeyType& key, const InfoDialog& inst);
  typedef inst_index<InfoDialog, const KeyType, &InfoDialog::Comparer> Repository;
 public:
  // Factory method. Returns always the same instance for the same set of objects.
  static int_ptr<InfoDialog> GetByKey(const KeyType& obj)
                    { return Repository::GetByKey(obj, &InfoDialog::Factory); }
};

void DLLENTRY InfoDialogMetaWriteErrorHandler(InfoDialog::MetaWriteDlg::StatusReport* that, MESSAGE_TYPE type, const xstring& msg);

inline InfoDialog::MetaWriteDlg::StatusReport::StatusReport()
: OldHandler(EventHandler::SetLocalHandler(vdelegate(&VDErrorHandler, &InfoDialogMetaWriteErrorHandler, this)))
{}


/****************************************************************************
*
*  Specialization of info dialog for a single object.
*
****************************************************************************/
class SingleInfoDialog
: public InfoDialog
{private:
  struct Data       DataCache;
  bool              DataCacheValid;
  class_delegate<SingleInfoDialog, const PlayableChangeArgs> ContentChangeDeleg;

 public:
                    SingleInfoDialog(const KeyType& key);
 private:
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual const struct Data& GetData();
  void              ContentChangeEvent(const PlayableChangeArgs& args);
};


/****************************************************************************
*
*  Specialization of info dialog for multiple objects.
*
****************************************************************************/
class MultipleInfoDialog
: public InfoDialog
{private:
  struct Data       DataCache;
  bool              DataCacheValid;
  //Playable::DecoderInfo MergedInfo;

 public:
                    MultipleInfoDialog(const KeyType& key);
 private: // Dialog functions
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
 private: // Data source
  virtual const struct Data& GetData();
  void              ApplyFlags(const Playable& item);
  void              JoinInfo(const Playable& item);
  void              CompareField(Fields fld, const char* l, const char* r);
  void              CompareField(Fields fld, int l, int r)       { if (l != r) DataCache.Valid &= ~fld; }
  void              CompareField(Fields fld, double l, double r) { if (l != r) DataCache.Valid &= ~fld; }
};


/****************************************************************************
*
*  Specialization of info dialog for playlist items.
*
****************************************************************************/
class PlayableInstanceInfoDialog
: public SingleInfoDialog
{protected:
  // Notebook page 2
  class PageItemInfo;
  friend class PageItemInfo;
  class PageItemInfo : public PageBase
  {private:
    bool            InUpdate;
    bool            Modified;
   private:
    void            SetModified();
   protected:
    HWND            SetCtrlText(ULONG id, Fields fld, const char* text)
                    { return PageBase::SetCtrlText(id, fld, text ? text : ""); }
    static const char* FormatGap(char* buffer, float time);
    void            Save();
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    PageItemInfo(InfoDialog& parent) : PageBase(parent, DLG_ITEMINFO, "Playlist item"), InUpdate(false), Modified(false) {}
    virtual InfoFlags GetRequestFlags();
  };

 public:
  PlayableInstanceInfoDialog(const KeyType& key);
};


// Implementations
#ifdef DEBUG_LOG
static xstring debug_dump(const AInfoDialog::KeyType& key)
{ xstringbuilder sb;
  for (size_t i = 0; i < key.size(); ++i)
    sb.appendf(sb.length() ? ", %p" : "%p", key[i]);
  return sb;
}
#endif

InfoDialog* InfoDialog::Factory(const KeyType& key)
{ DEBUGLOG(("InfoDialog::Factory({%u, %s})\n", key.size(), debug_dump(key).cdata()));
  InfoDialog* ret;
  #ifdef DEBUG // Check content
  if (key.Parent)
  { for (const int_ptr<APlayable>* app = key.begin(); app != key.end(); ++app)
    { APlayable* ap = *app;
      ASSERT(&ap->GetPlayable() != ap);
      ASSERT(((PlayableInstance*)ap)->HasParent(key.Parent) || ((PlayableInstance*)ap)->HasParent(NULL));
    }
  }
  #endif
  if (key.size() > 1)
    ret = new MultipleInfoDialog(key);
  else if (key.Parent)
    ret = new PlayableInstanceInfoDialog(key);
  else
    ret = new SingleInfoDialog(key);
  // Go!
  ret->StartDialog();
  return ret;
}

int InfoDialog::Comparer(const KeyType& key, const InfoDialog& inst)
{ return KeyType::compare(key, inst.Key);
}

int_ptr<AInfoDialog> AInfoDialog::GetByKey(const KeyType& set)
{ DEBUGLOG(("AInfoDialog::GetByKey({%u,...})\n", set.size()));
  if (set.size() == 0)
    return (AInfoDialog*)NULL;
  return InfoDialog::GetByKey(set).get();
}
int_ptr<AInfoDialog> AInfoDialog::GetByKey(Playable& obj)
{ InfoDialog::KeyType key(1);
  key.append() = &obj;
  return InfoDialog::GetByKey(key).get();
}
/*int_ptr<AInfoDialog> AInfoDialog::GetByKey(Playable& list, PlayableInstance& item)
{ InfoDialog::KeyType key(2);
  // TODO!!! breaks sort order!
  key.append() = &item;
  key.append() = &list;
  return InfoDialog::GetByKey(key).get();
}*/


InfoDialog::PageBase::PageBase(InfoDialog& parent, ULONG rid, const xstring& title)
: NotebookDialogBase::PageBase(parent, rid, NULLHANDLE, DF_AutoResize)
{ MajorTitle = title;
}

xstring InfoDialog::PageBase::QueryItemTextOrNULL(ULONG id)
{ const xstring& ret = WinQueryDlgItemXText(GetHwnd(), id);
  return ret.length() ? ret : xstring();
}

inline void InfoDialog::PageBase::CheckButton(ULONG id, unsigned state)
{ WinCheckButton(GetHwnd(), id, state);
}

HWND InfoDialog::PageBase::SetCtrlText(ULONG id, Fields fld, const char* text)
{ HWND ctrl = WinWindowFromID(GetHwnd(), id);
  PMASSERT(ctrl != NULLHANDLE);
  PMRASSERT(WinSetWindowText(ctrl, !(Valid & fld) ? "" : text ? text : "n/a"));
  PMRASSERT(WinEnableWindow(ctrl, !!(Enabled & fld)));
  // Revoke errors from validation
  WinRemovePresParam(ctrl, PP_BACKGROUNDCOLOR);
  return ctrl;
}

HWND InfoDialog::PageBase::SetCtrlCB(ULONG id, Fields fld, bool flag)
{ HWND ctrl = WinWindowFromID(GetHwnd(), id);
  PMASSERT(ctrl != NULLHANDLE);
  WinSendMsg(ctrl, BM_SETCHECK, MPFROMSHORT(Valid & fld ? flag : 2), 0);
  PMRASSERT(WinEnableWindow(ctrl, !!(Enabled & fld)));
  return ctrl;
}

void InfoDialog::PageBase::SetCtrlRB(ULONG id1, Fields fld, int btn)
{ DEBUGLOG(("InfoDialog(%p)::PageBase::SetCtrlRB(%u, %x, %i) - %x, %x\n", &Parent, id1, fld, btn, Valid, Enabled));
  HWND ctrl1 = WinWindowFromID(GetHwnd(), id1);
  PMASSERT(ctrl1 != NULLHANDLE);
  // For all radio buttons in this group
  HWND ctrl = ctrl1;
  HWND ctrl2;
  do
  { WinSendMsg(ctrl, BM_SETCHECK, MPFROMSHORT(Valid & fld ? btn == 0 : 2), 0);
    PMRASSERT(WinEnableWindow(ctrl, !!(Enabled & fld)));
    --btn;
    // Next button
    // Work around: BUG! WinEnumDlgItem does not wrap around.
    ctrl2 = ctrl;
    ctrl = WinEnumDlgItem(GetHwnd(), ctrl, EDI_NEXTGROUPITEM);
    DEBUGLOG(("InfoDialog::PageBase::SetCtrlRB at window %p - %p\n", ctrl, ctrl1));
  } while (ctrl != ctrl1 && ctrl != ctrl2);
}

bool InfoDialog::PageBase::SetCtrlEFValid(ULONG id, bool valid)
{ static const RGB error_color = { 0xbf, 0xbf, 0xff };
  HWND ctrl = WinWindowFromID(GetHwnd(), id);
  PMASSERT(ctrl != NULLHANDLE);
  if (valid)
    WinRemovePresParam(ctrl, PP_BACKGROUNDCOLOR);
  else
    PMRASSERT(WinSetPresParam(ctrl, PP_BACKGROUNDCOLOR, sizeof error_color, (PVOID)&error_color));
  return valid;
}

const char* InfoDialog::PageBase::FormatInt(char* buffer, int value, const char* unit)
{ if (value == -1)
    return NULL;
  sprintf(buffer, "%u%s", value, unit);
  return buffer;
}

const char* InfoDialog::PageBase::FormatGain(char* buffer, float gain)
{ if (gain <= -1000)
    return NULL;
  sprintf(buffer, "%.1f", gain);
  return buffer;
}

const char* InfoDialog::PageBase::FormatTstmp(char* buffer, time_t time)
{ if (time == (time_t)-1)
    return NULL;
  struct tm* tm = localtime(&time);
  strftime(buffer, 32, "%a %Y-%m-%d %H:%M:%S %Z", tm);
  return buffer;
}

const char* InfoDialog::PageBase::FormatDuration(char* buffer, PM123_TIME length)
{ if (length < 0)
    return NULL;
  unsigned long mins = (unsigned long)(length/60);
  PM123_TIME secs = length - 60*mins;
  if (mins < 60)
    sprintf(buffer, "%lu:%06.3f", mins, secs);
  else if (mins < 1440)
    sprintf(buffer, "%lu:%02lu:%06.3f", mins/60, mins%60, secs);
  else
    sprintf(buffer, "%lud %lu:%02lu:%06.3f", mins/1440, mins/60%24, mins%60, secs);
  // remove trailing zeros of fractional seconds.
  char* cp = buffer + strlen(buffer);
  while (*--cp == '0')
    *cp = 0;
  if (*cp == '.')
    *cp = 0;
  return buffer;
}

/*bool InfoDialog::PageBase::ParseDuration(const char* text, float& duration)
{ DEBUGLOG(("InfoDialog::PageBase::ParseDuration(%s,)\n"));
  if (!text || *text == 0)
  { duration = -1;
    return true;
  }
  text += strspn(text, " \t");
  size_t len = strlen(text);
  while (text[len-1] == ' ' || text[len-1] == '\t')
    --len;
  unsigned days = 0;
  unsigned hours = 0;
  unsigned mins = 0;
  float secs = 0;
  size_t parsed;
  size_t pos = strcspn(text, " \td");
  if (pos < len)
  { // have days
    if (sscanf(text, "%u%n", &days, &parsed) != 1 || parsed != pos)
      return false;
    text += pos;
    if (*text == 'd')
      ++text, --len;
    size_t blank = strspn(text, " \t");
    text += blank;
    len -= blank + pos;
  }
  pos = strcspn(text, ":");
  if (pos < len)
  { // have hours or minutes
    if (sscanf(text, "%u%n", &hours, &parsed) != 1 || parsed != pos)
      return false;
    text += pos+1;
    len -= pos+1;
    pos = strcspn(text, ":");
    if (pos < len)
    { // have minutes
      if (sscanf(text, "%u%n", &mins, &parsed) != 1 || parsed != pos)
        return false;
      text += pos+1;
      len -= pos+1;
    } else
    { mins = hours;
      hours = 0;
    }
  }
  if (sscanf(text, "%f%n", &secs, &parsed) != 1 || parsed != len || secs < 0)
    return false;
  // create result
  duration = (days * 1440 + hours * 60 + mins) * 60. + secs;
  return true;
}*/

MRESULT InfoDialog::PageBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("InfoDialog(%p)::PageBase::DlgProc(%x, %x, %x)\n", &Parent, msg, mp1, mp2));
  switch (msg)
  {case WM_INITDLG:
    do_warpsans(GetHwnd());
    WinPostMsg(GetHwnd(), UM_UPDATE, 0, 0);
    break;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

MRESULT InfoDialog::PageTechInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::PageTechInfo::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      const struct Data& data = GetParent().GetData();
      Enabled = data.Enabled;
      Valid = data.Valid;
      int type;
      SetCtrlText(EF_URL, F_URL, data.URL);
      { const PHYS_INFO& phys = data.Info.Phys;
        SetCtrlText(EF_FILESIZE, F_filesize, phys.filesize < 0 ? (const char*)NULL : (sprintf(buffer, "%.3f kiB", phys.filesize/1024.), buffer));
        SetCtrlText(EF_TIMESTAMP, F_timestamp, FormatTstmp(buffer, phys.tstmp));
        SetCtrlText(EF_FATTR, F_fattr, phys.attributes & PATTR_INVALID ? "invalid" : phys.attributes & PATTR_WRITABLE ? "none" : "read-only");
      }
      { const TECH_INFO& tech = data.Info.Tech;
        SetCtrlText(EF_SAMPLERATE, F_samplerate, FormatInt(buffer, tech.samplerate, " Hz"));
        SetCtrlText(EF_NUMCHANNELS, F_channels, FormatInt(buffer, tech.channels));
        static const char* objtypes[4] = {"unknown", "Song", "Playlist", "Song, indexed"};
        type = (tech.attributes & (TATTR_SONG|TATTR_PLAYLIST)) / TATTR_SONG;
        SetCtrlText(EF_OBJTYPE, F_objtype, tech.attributes & TATTR_INVALID ? "invalid" : objtypes[type]);
        SetCtrlCB(CB_SAVEABLE, F_objtype, !!(tech.attributes & TATTR_WRITABLE));
        SetCtrlCB(CB_SAVESTREAM, F_objtype, !!(tech.attributes & TATTR_STORABLE));
        SetCtrlText(EF_INFOSTRINGS, F_info, tech.info);
        xstring decoder = tech.decoder;
        if (decoder && decoder.startsWithI(amp_startpath))
          decoder.assign(decoder, amp_startpath.length());
        SetCtrlText(EF_DECODER, F_decoder, decoder);
        SetCtrlText(EF_FORMAT, F_format, tech.format);
      }
      { const OBJ_INFO& obj = data.Info.Obj;
        if (type != 2)
          SetCtrlText(EF_TOTALTIME, F_songlength, FormatDuration(buffer, obj.songlength));
        SetCtrlText(EF_BITRATE, F_bitrate, obj.bitrate < 0 ? (const char*)NULL : (sprintf(buffer, "%.1f kbps", obj.bitrate/1000.), buffer));
        SetCtrlText(EF_NUMITEMS, F_num_items, FormatInt(buffer, obj.num_items));
      }
      { const RPL_INFO& rpl = data.Info.Rpl;
        SetCtrlText(EF_SONGITEMS, F_total_songs, FormatInt(buffer, rpl.songs));
        SetCtrlText(EF_LISTITEMS, F_total_lists, FormatInt(buffer, rpl.lists));
      }
      { const DRPL_INFO& drpl = data.Info.Drpl;
        if (type == 2)
          SetCtrlText(EF_TOTALTIME, F_songlength, FormatDuration(buffer, drpl.totallength));
        SetCtrlText(EF_TOTALSIZE, F_totalsize, drpl.totalsize < 0 ? (const char*)NULL : (sprintf(buffer, "%.3f MiB", drpl.totalsize/(1024.*1024.)), buffer));
      }
      return 0;
    }
   case WM_COMMAND:
    DEBUGLOG(("InfoDialog(%p)::PageTechInfo::DlgProc: WM_COMMAND %u\n", &Parent, SHORT1FROMMP(mp1)));
    switch (SHORT1FROMMP(mp1))
    {case PB_REFRESH:
      for (size_t i = 0; i != GetParent().Key.size(); ++i)
        GetParent().Key[i]->RequestInfo(IF_Decoder|IF_Aggreg, PRI_Normal, REL_Reload);
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

InfoFlags InfoDialog::PageTechInfo::GetRequestFlags()
{ return IF_Phys|IF_Tech|IF_Obj|IF_Attr|IF_Rpl|IF_Drpl;
}


HWND InfoDialog::PageMetaInfo::SetEFText(ULONG id, Fields fld, const char* text)
{ HWND ctrl = PageBase::SetCtrlText(id, fld, text ? text : "");
  WinSendMsg(ctrl, EM_SETREADONLY, MPFROMSHORT(!MetaWrite), 0);
  return ctrl;
}

inline bool InfoDialog::PageMetaInfo::FetchField(ULONG idcb, ULONG idef, xstring& field)
{ if (SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), idcb)))
  { field = WinQueryDlgItemXText(GetHwnd(), idef);
    if (!*field)
      field.reset();
    return true;
  }
  return false;
}

MRESULT InfoDialog::PageMetaInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {//case WM_INITDLG:
   // break;

   case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      const struct Data& data = GetParent().GetData();
      Enabled = data.Enabled;
      Valid = data.Valid;
      const META_INFO& meta = data.Info.Meta;
      MetaWrite = (~Enabled & (F_title|F_artist|F_album|F_year|F_comment|F_genre|F_track|F_copyright)) == 0;
      // TODO:  && data.Info->meta_write;

      CheckButton(CB_METATITLE,    !!(Valid & F_title));
      CheckButton(CB_METAARTIST,   !!(Valid & F_artist));
      CheckButton(CB_METAALBUM,    !!(Valid & F_album));
      CheckButton(CB_METATRACK,    !!(Valid & F_track));
      CheckButton(CB_METADATE,     !!(Valid & F_year));
      CheckButton(CB_METAGENRE,    !!(Valid & F_genre));
      CheckButton(CB_METACOMMENT,  !!(Valid & F_comment));
      CheckButton(CB_METACOPYRIGHT,!!(Valid & F_copyright));

      SetEFText(EF_METATITLE,     F_title,     meta.title);
      SetEFText(EF_METAARTIST,    F_artist,    meta.artist);
      SetEFText(EF_METAALBUM,     F_album,     meta.album);
      SetEFText(EF_METATRACK,     F_track,     meta.track);
      SetEFText(EF_METADATE,      F_year,      meta.year);
      SetEFText(EF_METAGENRE,     F_genre,     meta.genre);
      SetEFText(EF_METACOMMENT,   F_comment,   meta.comment);
      SetEFText(EF_METACOPYRIGHT, F_copyright, meta.copyright);

      SetCtrlText(EF_METARPGAINT, F_track_gain, FormatGain(buffer, meta.track_gain));
      SetCtrlText(EF_METARPPEAKT, F_track_peak, FormatGain(buffer, meta.track_peak));
      SetCtrlText(EF_METARPGAINA, F_album_gain, FormatGain(buffer, meta.album_gain));
      SetCtrlText(EF_METARPPEAKA, F_album_peak, FormatGain(buffer, meta.album_peak));

      PMRASSERT(WinEnableControl(GetHwnd(), PB_APPLY, MetaWrite));
      return 0;
    }

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case EF_METATITLE:
     case EF_METAARTIST:
     case EF_METAALBUM:
     case EF_METATRACK:
     case EF_METADATE:
     case EF_METAGENRE:
     case EF_METACOMMENT:
     case EF_METACOPYRIGHT:
      if (SHORT2FROMMP(mp1) == EN_CHANGE)
      { DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: EN_CHANGE %u\n", &Parent, SHORT1FROMMP(mp1)));
        PMRASSERT(WinEnableControl(GetHwnd(), PB_UNDO, TRUE));
      }
    }
    break;

   case WM_COMMAND:
    DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: WM_COMMAND %u\n", &Parent, SHORT1FROMMP(mp1)));
    switch (SHORT1FROMMP(mp1))
    {case PB_UNDO:
      PostMsg(UM_UPDATE, 0, 0);
      return 0;
     case PB_APPLY:
      { // fetch meta data
        MetaWriteDlg wdlg(GetParent().Key);
        wdlg.MetaFlags =
            DECODER_HAVE_TITLE    * FetchField(CB_METATITLE,    EF_METATITLE,    wdlg.MetaData.title    )
          | DECODER_HAVE_ARTIST   * FetchField(CB_METAARTIST,   EF_METAARTIST,   wdlg.MetaData.artist   )
          | DECODER_HAVE_ALBUM    * FetchField(CB_METAALBUM,    EF_METAALBUM,    wdlg.MetaData.album    )
          | DECODER_HAVE_TRACK    * FetchField(CB_METATRACK,    EF_METATRACK,    wdlg.MetaData.track    )
          | DECODER_HAVE_YEAR     * FetchField(CB_METADATE,     EF_METADATE,     wdlg.MetaData.year     )
          | DECODER_HAVE_GENRE    * FetchField(CB_METAGENRE,    EF_METAGENRE,    wdlg.MetaData.genre    )
          | DECODER_HAVE_COMMENT  * FetchField(CB_METACOMMENT,  EF_METACOMMENT,  wdlg.MetaData.comment  )
          | DECODER_HAVE_COPYRIGHT* FetchField(CB_METACOPYRIGHT,EF_METACOPYRIGHT,wdlg.MetaData.copyright);
        // Invoke worker dialog
        wdlg.DoDialog(GetHwnd());
        DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: PB_APPLY - after DoDialog\n", &Parent));
        return 0;
      }
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

InfoFlags InfoDialog::PageMetaInfo::GetRequestFlags()
{ return IF_Meta;
}


InfoDialog::MetaWriteDlg::MetaWriteDlg(const vector<APlayable>& dest)
: DialogBase(DLG_WRITEMETA, NULLHANDLE)
, MetaFlags(DECODER_HAVE_NONE)
, Dest(dest)
, SkipErrors(false)
, WorkerTID(0)
, Cancel(false)
{ DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::MetaWriteDlg({%u,})\n", this, dest.size()));
}

InfoDialog::MetaWriteDlg::~MetaWriteDlg()
{ DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::~MetaWriteDlg() - %u\n", this, WorkerTID));
  // Wait for the worker thread to complete (does not hold the SIQ)
  if (WorkerTID)
  { Cancel = true;
    ResumeSignal.Set();
    wait_thread_pm(amp_player_hab, WorkerTID, 5*60*1000);
  }
}

void TFNENTRY InfoDialogMetaWriteWorkerStub(void* arg)
{ ((InfoDialog::MetaWriteDlg*)arg)->Worker();
}

MRESULT InfoDialog::MetaWriteDlg::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // setup first destination
    EntryField(+GetCtrl(EF_WMURL)).SetText(Dest[0]->GetPlayable().URL);
    PostMsg(UM_START, 0, 0);
    break;

   case UM_START:
    DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::DlgProc: UM_START\n", this));
    CurrentItem = 0;
    WorkerTID = _beginthread(&InfoDialogMetaWriteWorkerStub, NULL, 65536, this);
    ASSERT(WorkerTID != (TID)-1);
    return 0;

   case UM_STATUS:
    { DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::DlgProc: UM_STATUS %i, %i\n", this, mp1, mp2));
      int i = LONGFROMMP(mp1);
      if (i < 0)
      { // Terminate Signal
        WinDismissDlg(GetHwnd(), DID_OK);
        return 0;
      }
      // else
      { // Update progress bar
        SWP swp;
        PMRASSERT(WinQueryWindowPos(GetCtrl(SB_WMBARBG), &swp));
        swp.cx = swp.cx * (i+1) / Dest.size();
        PMRASSERT(WinSetWindowPos(GetCtrl(SB_WMBARFG), NULLHANDLE, 0,0, swp.cx,swp.cy, SWP_SIZE));
      }

      StatusReport* rep = (StatusReport*)PVOIDFROMMP(mp2);
      if (rep->RC == PLUGIN_OK)
      { // Everything fine, next item
        EntryField(+GetCtrl(EF_WMSTATUS)).SetText("");
        ++i; // next item (if any)
        EntryField(+GetCtrl(EF_WMURL)).SetText((size_t)i < Dest.size() ? Dest[i]->GetPlayable().URL.cdata() : "");
        return 0;
      }
      // Error, worker halted
      xstring errortext;
      errortext.sprintf("Error %lu - %s", rep->RC, rep->Text.cdata());
      PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMSTATUS, errortext));
      // 'skip all' was pressed? => continue immediately
      if (SkipErrors)
      { ResumeSignal.Set();
        return 0;
      }
      // Enable skip buttons
      EnableCtrl(PB_RETRY,     true);
      EnableCtrl(PB_WMSKIP,    true);
      EnableCtrl(PB_WMSKIPALL, true);
      return 0;
    }

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case DID_CANCEL:
      Cancel = true;
      ResumeSignal.Set();
      EntryField(+GetCtrl(EF_WMSTATUS)).SetText("- cancelling -");
      EnableCtrl(DID_CANCEL, false);
      break;

     case PB_WMSKIPALL:
      SkipErrors = true;
     case PB_WMSKIP:
      // next item
      ++CurrentItem;
     case PB_RETRY:
      // Disable skip buttons
      EnableCtrl(PB_RETRY,     false);
      EnableCtrl(PB_WMSKIP,    false);
      EnableCtrl(PB_WMSKIPALL, false);
      ResumeSignal.Set();
    }
    return 0;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

ULONG InfoDialog::MetaWriteDlg::DoDialog(HWND owner)
{ DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::DoDialog() - %u %x\n", this, Dest.size(), MetaFlags));
  if (Dest.size() == 0 || MetaFlags == DECODER_HAVE_NONE)
    return DID_OK;
  StartDialog(owner);
  return WinProcessDlg(GetHwnd());
}

void DLLENTRY InfoDialogMetaWriteErrorHandler(InfoDialog::MetaWriteDlg::StatusReport* that, MESSAGE_TYPE type, const xstring& msg)
{ that->Text = msg;
}

void InfoDialog::MetaWriteDlg::Worker()
{ while (!Cancel && CurrentItem < Dest.size())
  { Playable& song = Dest[CurrentItem]->GetPlayable();
    DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::Worker - %u %s\n", this, CurrentItem, song.DebugName().cdata()));
    // Write info
    StatusReport rep;
    rep.RC = song.SaveMetaInfo(MetaData, MetaFlags);
    // Notify dialog
    PostMsg(UM_STATUS, MPFROMLONG(CurrentItem), MPFROMLONG(&rep));
    if (Cancel)
      return;
    // wait for acknowledge
    if (rep.RC != PLUGIN_OK)
    { ResumeSignal.Wait();
      ResumeSignal.Reset();
    } else
      ++CurrentItem;
  }
  PostMsg(UM_STATUS, MPFROMLONG(-1), MPFROMLONG(0));
  DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::Worker completed\n", this));
}


InfoDialog::InfoDialog(const KeyType& key)
: AInfoDialog(DLG_INFO, NULLHANDLE)
, Key(key)
{ DEBUGLOG(("InfoDialog(%p)::InfoDialog({%u, %s}) - {%u, %s}\n", this,
    key.size(), debug_dump(key).cdata(), Key.size(), debug_dump(Key).cdata()));
  Pages.append() = new PageMetaInfo(*this);
  Pages.append() = new PageTechInfo(*this);
}

InfoDialog::~InfoDialog()
{ DEBUGLOG(("InfoDialog(%p)::~InfoDialog()\n", this));
  Repository::RemoveWithKey(*this, Key);
}

void InfoDialog::RequestPage(PageBase* page)
{ DEBUGLOG(("InfoDialog(%p)::RequestPage(%p)\n", this, page));
  if (page == NULL)
    return;
  InfoFlags what = page->GetRequestFlags();
  for (size_t i = 0; i != Key.size(); ++i)
    Key[i]->RequestInfo(what, PRI_Normal, REL_Confirmed);
}

MRESULT InfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    { // initial position
      SWP swp;
      PMXASSERT(WinQueryTaskSizePos(amp_player_hab, 0, &swp), == 0);
      PMRASSERT(WinSetWindowPos(GetHwnd(), NULLHANDLE, swp.x,swp.y, 0,0, SWP_MOVE));
    }
    do_warpsans(GetHwnd());
    SetHelpMgr(GUI::GetHelpMgr());
    // restore position
    Cfg::RestWindowPos(GetHwnd(), Key.size() > 1 ? NULL : Key[0]->GetPlayable().URL.cdata());
    break;

   case WM_DESTROY:
    // save position
    Cfg::SaveWindowPos(GetHwnd(), Key.size() > 1 ? NULL : Key[0]->GetPlayable().URL.cdata());
    break;

   case WM_CONTROL:
    if (SHORT1FROMMP(mp1) == NB_INFO)
      switch (SHORT2FROMMP(mp1))
      {case BKN_PAGESELECTED:
        { const PAGESELECTNOTIFY& psn = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
          DEBUGLOG(("InfoDialog(%p)::DlgProc:WM_CONTROL:BKN_PAGESELECTED: %i, %i\n", this, psn.ulPageIdCur, psn.ulPageIdNew));
          // On notebook creation there is no match.
          RequestPage(PageFromID(psn.ulPageIdNew));
        }
      }

   case WM_SYSCOMMAND:
    if (SHORT1FROMMP(mp1) == SC_CLOSE)
    { Destroy();
      return 0;
    }
    break;
  }
  return AInfoDialog::DlgProc(msg, mp1, mp2);
}

void InfoDialog::ShowPage(PageNo page)
{ ASSERT((size_t)page < Pages.size());
  PageBase* pp = (PageBase*)Pages[page];
  ASSERT(pp);
  RequestPage(pp);
  SendCtrlMsg(NB_INFO, BKM_TURNTOPAGE, MPFROMLONG(pp->GetPageID()), 0);
  SetVisible(true);
}


SingleInfoDialog::SingleInfoDialog(const KeyType& key)
: InfoDialog(key),
  DataCacheValid(false),
  ContentChangeDeleg(*this, &SingleInfoDialog::ContentChangeEvent)
{ DEBUGLOG(("SingleInfoDialog(%p)::SingleInfoDialog({%p{%s}})\n", this, Key[0], Key[0]->DebugName().cdata()));
}

MRESULT SingleInfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    SetTitle(xstring().sprintf("PM123 Object info: %s", Key[0]->GetPlayable().URL.getDisplayName().cdata()));
    // register for change event
    Key[0]->GetInfoChange() += ContentChangeDeleg;
    break;

   case WM_DESTROY:
    ContentChangeDeleg.detach();
    break;
  }
  return InfoDialog::DlgProc(msg, mp1, mp2);
}

const struct InfoDialog::Data& SingleInfoDialog::GetData()
{ DEBUGLOG(("SingleInfoDialog(%p)::GetData() %u - %p\n", this, DataCacheValid, Key[0]));
  if (!DataCacheValid)
  { DataCacheValid = true;
    // Fetch Data
    DataCache.URL = Key[0]->GetPlayable().URL;
    DataCache.Info = Key[0]->GetInfo(); // Information is copied for strong thread safety.
    InfoFlags invalid = Key[0]->RequestInfo(IF_Decoder|IF_Aggreg, PRI_None, REL_Cached);
    switch (DataCache.Info.Tech.attributes & (TATTR_SONG|TATTR_PLAYLIST))
    {case TATTR_NONE:
      DataCache.Enabled = F_URL|F_PHYS_INFO|F_info|F_decoder;
      goto def;

     case TATTR_SONG:
      DataCache.Enabled = F_URL|F_PHYS_INFO|F_TECH_INFO|F_songlength|F_bitrate|F_META_INFO|F_ATTR_INFO|F_ITEM_INFO;
      goto def;

     case TATTR_PLAYLIST:
      DataCache.Enabled = F_URL|F_PHYS_INFO|F_objtype|F_info|F_format|F_decoder|F_songlength|F_num_items|F_ATTR_INFO|F_RPL_INFO|F_DRPL_INFO|F_ITEM_INFO;
      DataCache.Valid = F_URL
        | ((invalid & IF_Phys) == 0) * F_PHYS_INFO
        | ((invalid & IF_Tech) == 0) * F_TECH_INFO
        | ((invalid & IF_Obj ) == 0) * (F_OBJ_INFO&~F_songlength)
        | ((invalid & IF_Meta) == 0) * F_META_INFO
        | ((invalid & IF_Attr) == 0) * F_ATTR_INFO
        | ((invalid & IF_Rpl ) == 0) * F_RPL_INFO
        | ((invalid & IF_Drpl) == 0) * (F_DRPL_INFO|F_songlength)
        | ((invalid & IF_Item) == 0) * F_ITEM_INFO ;
      break;

     default: // TATTR_SONG|TATTR_PLAYLIST:
      DataCache.Enabled = F_URL|F_PHYS_INFO|F_TECH_INFO|F_OBJ_INFO|F_META_INFO|F_ATTR_INFO|F_RPL_INFO|F_DRPL_INFO|F_ITEM_INFO;
     def:
      DataCache.Valid = F_URL
        | ((invalid & IF_Phys) == 0) * F_PHYS_INFO
        | ((invalid & IF_Tech) == 0) * F_TECH_INFO
        | ((invalid & IF_Obj ) == 0) * F_OBJ_INFO
        | ((invalid & IF_Meta) == 0) * F_META_INFO
        | ((invalid & IF_Attr) == 0) * F_ATTR_INFO
        | ((invalid & IF_Rpl ) == 0) * F_RPL_INFO
        | ((invalid & IF_Drpl) == 0) * F_DRPL_INFO
        | ((invalid & IF_Item) == 0) * F_ITEM_INFO ;
      break;
    }
  }
  DEBUGLOG(("SingleInfoDialog::GetData() %x %x\n", DataCache.Enabled, DataCache.Valid));
  return DataCache;
}

void SingleInfoDialog::ContentChangeEvent(const PlayableChangeArgs& args)
{ DEBUGLOG(("SingleInfoDialog(%p)::ContentChangeEvent({&%p, %x, %x})\n",
    this, &args.Instance, args.Changed, args.Loaded));
  for (AInfoDialog::PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
  { PageBase* p = (PageBase*)*pp;
    if (p == NULL)
      continue;
    InfoFlags relevant = p->GetRequestFlags();
    if (args.Changed & relevant)
    { DataCacheValid = false;
      WinPostMsg((*pp)->GetHwnd(), PageBase::UM_UPDATE, MPFROMLONG(args.Changed), 0);
    }
    // automatically request invalidated infos
    if (args.Invalidated & relevant)
      args.Instance.RequestInfo(args.Invalidated & relevant, PRI_Low);
  }
}


MultipleInfoDialog::MultipleInfoDialog(const KeyType& key)
: InfoDialog(key),
  DataCacheValid(false)
{ DEBUGLOG(("MultipleInfoDialog(%p)::MultipleInfoDialog({%u %s})\n", this, key.size(), debug_dump(key).cdata()));
}

MRESULT MultipleInfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // Since we are _Multiple_InfoDialog there are at least two items.
    SetTitle(xstring().sprintf("PM123 - %u objects: %s, %s ...", Key.size(),
      Key[0]->GetPlayable().URL.getShortName().cdata(), Key[1]->GetPlayable().URL.getShortName().cdata()));
    break;
  }
  return InfoDialog::DlgProc(msg, mp1, mp2);
}

const struct InfoDialog::Data& MultipleInfoDialog::GetData()
{ DEBUGLOG(("MultipleInfoDialog(%p)::GetData() %u - %u %s\n", this, DataCacheValid, Key.size(), debug_dump(Key).cdata()));
  if (!DataCacheValid)
  { // Initial Data
    Playable* pp = &Key[0]->GetPlayable();

    DataCacheValid = true;
    DataCache.Info    = pp->GetInfo();// TODO: &MergedInfo;
    DataCache.Enabled = ~F_none;
    DataCache.Valid   = ~F_none;
    // merge data
    size_t i = 0;
    //MergedInfo = pp->GetInfo();
    for(;;)
    { ApplyFlags(*pp);
      if (++i == Key.size())
        break;
      pp = &Key[i]->GetPlayable();
      JoinInfo(*pp);
    }
  }
  DEBUGLOG(("MultipleInfoDialog::GetData() %x %x\n", DataCache.Enabled, DataCache.Valid));
  return DataCache;
}

void MultipleInfoDialog::ApplyFlags(const Playable& item)
{ DEBUGLOG(("MultipleInfoDialog(%p)::ApplyFlags(&%p)\n", this, &item));
  DataCache.Enabled &= /* TODO: item.GetFlags() & Playable::Enumerable
    ? F_decoder|F_TECH_INFO|F_PHYS_INFO|F_RPL_INFO
    :*/ F_ALL;
  /* TODO: DataCache.Valid &=
    (bool)(item.CheckInfo(Playable::IF_Phys  ) == 0) * F_PHYS_INFO |
    (bool)(item.CheckInfo(Playable::IF_Tech  ) == 0) * F_TECH_INFO |
    (bool)(item.CheckInfo(Playable::IF_Rpl   ) == 0 && (item.GetFlags() & Playable::Enumerable)) * F_RPL_INFO |
    (bool)(item.CheckInfo(Playable::IF_Format) == 0) * F_FORMAT_INFO2 |
    (bool)(item.CheckInfo(Playable::IF_Other ) == 0) * F_decoder |
    (bool)(item.CheckInfo(Playable::IF_Meta  ) == 0) * F_META_INFO;*/
}

void MultipleInfoDialog::JoinInfo(const Playable& song)
{ // Decoder
  // TODO: CompareField(F_decoder, DataCache.Decoder, song.GetDecoder());

  /* TODO:   const INFO_BUNDLE& info = song.GetInfo();
  // Format info
  CompareField(F_samplerate, MergedInfo.format->samplerate,info.format->samplerate);
  CompareField(F_channels,   MergedInfo.format->channels,  info.format->channels);
  // Tech info
  CompareField(F_songlength, MergedInfo.tech->songlength,  info.tech->songlength);
  CompareField(F_bitrate,    MergedInfo.tech->bitrate,     info.tech->bitrate);
  CompareField(F_totalsize,  MergedInfo.tech->totalsize,   info.tech->totalsize);
  CompareField(F_info,       MergedInfo.tech->info,        info.tech->info);
  // Meta info
  CompareField(F_title,      MergedInfo.meta->title,       info.meta->title);
  CompareField(F_artist,     MergedInfo.meta->artist,      info.meta->artist);
  CompareField(F_album,      MergedInfo.meta->album,       info.meta->album);
  CompareField(F_year,       MergedInfo.meta->year,        info.meta->year);
  CompareField(F_comment,    MergedInfo.meta->comment,     info.meta->comment);
  CompareField(F_genre,      MergedInfo.meta->genre,       info.meta->genre);
  CompareField(F_track,      MergedInfo.meta->track,       info.meta->track);
  CompareField(F_copyright,  MergedInfo.meta->copyright,   info.meta->copyright);
  CompareField(F_track_gain, MergedInfo.meta->track_gain,  info.meta->track_gain);
  CompareField(F_track_peak, MergedInfo.meta->track_peak,  info.meta->track_peak);
  CompareField(F_album_gain, MergedInfo.meta->album_gain,  info.meta->album_gain);
  CompareField(F_album_peak, MergedInfo.meta->album_peak,  info.meta->album_peak);
  // Phys info
  CompareField(F_filesize,   MergedInfo.phys->filesize,    info.phys->filesize);
  CompareField(F_num_items,  MergedInfo.phys->num_items,   info.phys->num_items);
  // RPL info
  CompareField(F_total_items,MergedInfo.rpl->total_items,  info.rpl->total_items);
  CompareField(F_recursive,  MergedInfo.rpl->recursive,    info.rpl->recursive);*/
}

void MultipleInfoDialog::CompareField(Fields fld, const char* l, const char* r)
{ if (DataCache.Valid & fld && strcmp(l, r) != 0)
    DataCache.Valid &= ~fld;
}


InfoFlags PlayableInstanceInfoDialog::PageItemInfo::GetRequestFlags()
{ return IF_Item|IF_Attr;
}

void PlayableInstanceInfoDialog::PageItemInfo::SetModified()
{ if (Modified)
    return;
  Modified = true;
  EnableCtrl(PB_APPLY, true);
  EnableCtrl(PB_UNDO, true);
}

const char* PlayableInstanceInfoDialog::PageItemInfo::FormatGap(char* buffer, float gap)
{ if (gap < 0)
    return NULL;
  sprintf(buffer, "%.3f", gap);
  char * cp = buffer + strlen(buffer);
  while (*--cp == '0')
    *cp = 0;
  if (*cp == '.')
    *cp = 0;
  return buffer;
}

void PlayableInstanceInfoDialog::PageItemInfo::Save()
{
  size_t len;
  bool valid = true;
  // Extract Parameters and validate
  url123 url;
  xstring tmp = WinQueryDlgItemXText(GetHwnd(), EF_INFOURL);
  valid &= SetCtrlEFValid(EF_INFOURL, tmp.length() != 0 && (url = url123::normalizeURL(tmp)) != NULL);

  ItemInfo item;
  AttrInfo attr;
  item.alias = QueryItemTextOrNULL(EF_INFOALIAS);
  item.start = QueryItemTextOrNULL(EF_INFOSTART);
  item.stop  = QueryItemTextOrNULL(EF_INFOSTOP);
  attr.at    = QueryItemTextOrNULL(EF_INFOAT);
  // Validation of Location makes no sense unless the URL is valid.
  int_ptr<Playable> pp;
  if (valid)
  { pp = Playable::GetByURL(url);
    JobSet job(PRI_Normal);
    Location loc(pp);
    const char* cp;
    Location::NavigationResult nr;
    if (item.start)
    { cp = item.start;
      nr = loc.Deserialize(job, cp);
      job.Commit();
      // TODO error message?
      SetCtrlEFValid(EF_INFOSTART, nr == NULL || nr.length() == 0); // in dubio pro rheo
    }
    if (item.stop)
    { loc.Reset();
      cp = item.stop;
      nr = loc.Deserialize(job, cp);
      job.Commit();
      // TODO error message?
      SetCtrlEFValid(EF_INFOSTOP, nr == NULL || nr.length() == 0); // in dubio pro rheo
    }
    if (attr.at)
    { loc.Reset();
      cp = attr.at;
      nr = loc.Deserialize(job, cp);
      job.Commit();
      // TODO error message?
      SetCtrlEFValid(EF_INFOAT, nr == NULL || nr.length() == 0); // in dubio pro rheo
    }
    // TODO refresh on job completion?
  }
  tmp = WinQueryDlgItemXText(GetHwnd(), EF_INFOPREGAP);
  valid &= SetCtrlEFValid(EF_INFOPREGAP, tmp.length() == 0
    || (sscanf(tmp, "%f%n", &item.pregap, &len) == 1 && len == tmp.length() && item.pregap >= 0) );
  tmp = WinQueryDlgItemXText(GetHwnd(), EF_INFOPOSTGAP);
  valid &= SetCtrlEFValid(EF_INFOPOSTGAP, tmp.length() == 0
    || (sscanf(tmp, "%f%n", &item.postgap, &len) == 1 && len == tmp.length() && item.postgap >= 0) );
  tmp = WinQueryDlgItemXText(GetHwnd(), EF_INFOGAIN);
  valid &= SetCtrlEFValid(EF_INFOGAIN, tmp.length() == 0
    || (sscanf(tmp, "%f%n", &item.gain, &len) == 1 && len == tmp.length() && fabs(item.gain) < 100) );

  attr.ploptions = PLO_ALTERNATION * CheckBox(GetCtrl(CB_INFOALTERNATION)).QueryCheckState()
      | PLO_SHUFFLE * RadioButton(GetCtrl(RB_INFOPLSHINHERIT)).QueryCheckIndex();

  if (!valid)
    return;

  // Save
  PlayableInstance* pi = (PlayableInstance*)GetParent().Key[0];
  if (&pi->GetPlayable() != pp)
  { // URL has changed
    Playable& list = *GetParent().Key.Parent;
    Mutex::Lock(list.Mtx);
    PlayableInstance* pi2 = list.InsertItem(*pp, pi);
    if (pi2 == NULL)
      return;
    list.RemoveItem(pi);
    pi = pi2;
    // TODO: follow with current window.
  }
  Mutex::Lock lock(pp->Mtx);
  pi->OverrideItem(item.IsInitial() ? NULL : &item);
  pi->OverrideAttr(attr.IsInitial() ? NULL : &attr);
}

MRESULT PlayableInstanceInfoDialog::PageItemInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case UM_UPDATE:
    { // update info values
      DEBUGLOG(("PlayableInstanceInfoDialog(%p)::PageItemInfo::DlgProc: UM_UPDATE\n", &Parent));
      InUpdate = true; // Disable change notifications
      char buffer[32];
      const struct Data& data = GetParent().GetData();
      Enabled = data.Enabled;
      Valid = data.Valid;
      EntryField(+GetCtrl(EF_INFOURL)).SetText(data.URL);
      { const ITEM_INFO& item = data.Info.Item;
        SetCtrlText(EF_INFOALIAS,   F_ITEM_INFO, item.alias);
        SetCtrlText(EF_INFOSTART,   F_ITEM_INFO, item.start);
        SetCtrlText(EF_INFOSTOP,    F_ITEM_INFO, item.stop);
        SetCtrlText(EF_INFOPREGAP,  F_ITEM_INFO, FormatGap(buffer, item.pregap));
        SetCtrlText(EF_INFOPOSTGAP, F_ITEM_INFO, FormatGap(buffer, item.postgap));
        SetCtrlText(EF_INFOGAIN,    F_ITEM_INFO, FormatGain(buffer, item.gain));
      }
      { const ATTR_INFO& attr = data.Info.Attr;
        SetCtrlText(EF_INFOAT,      F_ATTR_INFO, attr.at);
        SetCtrlCB(CB_INFOALTERNATION, F_ATTR_INFO, (attr.ploptions & PLO_ALTERNATION) != 0);
        SetCtrlRB(RB_INFOPLSHINHERIT, F_ATTR_INFO, (attr.ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE)) / PLO_SHUFFLE);
      }
      EnableCtrl(PB_APPLY, false);
      EnableCtrl(PB_UNDO, false);
      Modified = false;
      InUpdate = false;
      return 0;
    }

   case WM_CONTROL:
    DEBUGLOG(("PlayableInstanceInfoDialog(%p)::PageItemInfo::DlgProc: WM_CONTROL %u,%u %p\n", &Parent, SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), HWNDFROMMP(mp2)));
    switch (SHORT1FROMMP(mp1))
    {case EF_INFOURL:
     case EF_INFOALIAS:
     case EF_INFOSTART:
     case EF_INFOSTOP:
     case EF_INFOAT:
     case EF_INFOPREGAP:
     case EF_INFOPOSTGAP:
     case EF_INFOGAIN:
      if (!InUpdate && SHORT2FROMMP(mp1) == EN_CHANGE)
        SetModified();
      return 0;

     case CB_INFOALTERNATION:
     case RB_INFOPLSHINHERIT:
     case RB_INFOPLSHFORCE:
     case RB_INFOPLSHCLEAR:
      if (!InUpdate && SHORT2FROMMP(mp1) == BN_CLICKED)
        SetModified();
      return 0;
    }
    break;

   case WM_COMMAND:
    DEBUGLOG(("PlayableInstanceInfoDialog(%p)::PageItemInfo::DlgProc: WM_COMMAND %u\n", &Parent, SHORT1FROMMP(mp1)));
    switch (SHORT1FROMMP(mp1))
    {case PB_UNDO:
      PostMsg(UM_UPDATE, 0, 0);
      return 0;

     case PB_APPLY:
      Save();
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}


PlayableInstanceInfoDialog::PlayableInstanceInfoDialog(const KeyType& key)
: SingleInfoDialog(key)
{ ASSERT(Key.Parent && Key.size() == 1);
  DEBUGLOG(("PlayableInstanceInfoDialog(%p)::PlayableInstanceInfoDialog({%p,%p})", this, Key.Parent.get(), Key[0]));
  Pages.append() = new PageItemInfo(*this);
}

