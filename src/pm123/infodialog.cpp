/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
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


#define INCL_BASE
#define INCL_PM
#include "infodialog.h"
#include "pm123.h"
#include "gui.h"
#include "pm123.rc.h"
#include "dialog.h"
#include "glue.h"
#include "properties.h"
#include <decoder_plug.h>
#include <plugin.h>
#include <utilfct.h>

#include <stdio.h>


/****************************************************************************
*
*  Class to handle info dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class InfoDialog
: public AInfoDialog,
  public IComparableTo<const sorted_vector_int<APlayable, APlayable> >,
  public inst_index<InfoDialog, const sorted_vector_int<APlayable, APlayable> > // depends on base class OwnedPlayableSet
{public:
  typedef sorted_vector_int<APlayable, APlayable> KeyType;
 protected:
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
   public:
    ULONG           PageID;  // PM Notebook page ID from.
   protected:
                    PageBase(InfoDialog& parent, ULONG rid);
    HWND            SetCtrlText(USHORT id, Fields fld, const char* text);
    HWND            SetCtrlCB(USHORT id, Fields fld, bool flag);
    void            SetCtrlRB(USHORT id1, Fields fld, int btn);
    bool            SetCtrlEFValid(USHORT id, bool valid);
    static const char* FormatInt(char* buffer, int value, const char* unit = "");
    static const char* FormatTstmp(char* buffer, time_t time);
    static const char* FormatDuration(char* buffer, PM123_TIME time);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    void            StartDialog() { DialogBase::StartDialog(Parent.GetHwnd(), Parent.GetHwnd()); }
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
    PageTechInfo(InfoDialog& parent) : PageBase(parent, DLG_TECHINFO) {}
    virtual InfoFlags GetRequestFlags();
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
                    PageMetaInfo(InfoDialog& parent) : PageBase(parent, DLG_METAINFO) {}
    virtual InfoFlags GetRequestFlags();
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
    unsigned        CurrentItem;
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
  friend void TFNENTRY InfoDialogMetaWriteWorkerStub(void*);

 private:
  // Functor for inst_index factory.
  class Factory : public inst_index<InfoDialog, const KeyType>::IFactory
  { // singleton
    Factory() {}
   public:
    static Factory  Instance;
    virtual InfoDialog* operator()(const KeyType& key);
  };

 private:
  PageTechInfo      PageTech;
  PageMetaInfo      PageMeta;
 protected:
  vector<PageBase>  Pages;

 private: // non copyable
                    InfoDialog(const InfoDialog&);
          void      operator=(const InfoDialog&);
 protected:
  // Initialize InfoDialog
                    InfoDialog(const KeyType& key);
          PageBase* PageFromID(ULONG pageid);
  virtual void      RequestPage(PageBase* page);
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);

 public:
  virtual           ~InfoDialog();
  virtual void      StartDialog();
  virtual void      ShowPage(PageNo page);
  virtual const struct Data& GetData() = 0;

  // Factory method. Returns always the same instance for the same set of objects.
  static int_ptr<InfoDialog> GetByKey(const KeyType& obj)
                    { return inst_index<InfoDialog, const KeyType>::GetByKey(obj, Factory::Instance); }
  // IComparableTo<Playable>
  virtual int       compareTo(const KeyType& r) const;
};

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
    HWND            SetCtrlText(USHORT id, Fields fld, const char* text)
                    { return PageBase::SetCtrlText(id, fld, text ? text : ""); }
    void            Save();
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    PageItemInfo(InfoDialog& parent) : PageBase(parent, DLG_ITEMINFO), InUpdate(false), Modified(false) {}
    virtual InfoFlags GetRequestFlags();
  };

 private:
  PageItemInfo      PageItem;

 public:
  PlayableInstanceInfoDialog(const KeyType& key);
  /*PlayableInstance& GetItem() const { return (PlayableInstance&)*Key[0]; }
  Playable&         GetList() const { return (Playable&)*Key[1]; }*/
  virtual void      StartDialog();
};


// Implementations

// Factory ...
InfoDialog::Factory InfoDialog::Factory::Instance;

InfoDialog* InfoDialog::Factory::operator()(const KeyType& key)
{ DEBUGLOG(("InfoDialogFactory::operator()({%u, %s})\n", key.size(), key.debug_dump().cdata()));
  InfoDialog* ret;
  if (&key[0]->GetPlayable() != key[0])
    ret = new PlayableInstanceInfoDialog(key);
  else if (key.size() > 1)
    ret = new MultipleInfoDialog(key);
  else
    ret = new SingleInfoDialog(key);
  ret->StartDialog();
  return ret;
}

int_ptr<AInfoDialog> AInfoDialog::GetByKey(const PlayableSetBase& obj)
{ DEBUGLOG(("AInfoDialog::GetByKey({%u,...})\n", obj.size()));
  if (((const sorted_vector<Playable, Playable>&)obj).size() == 0)
    return (AInfoDialog*)NULL;
  InfoDialog::KeyType key(obj.size());
  for (size_t i = 0; i < obj.size(); ++i)
    key.append() = obj[i];
  return InfoDialog::GetByKey(key).get();
}
int_ptr<AInfoDialog> AInfoDialog::GetByKey(Playable& obj)
{ InfoDialog::KeyType key(1);
  key.append() = &obj;
  return InfoDialog::GetByKey(key).get();
}
int_ptr<AInfoDialog> AInfoDialog::GetByKey(Playable& list, PlayableInstance& item)
{ InfoDialog::KeyType key(2);
  key.append() = &item;
  key.append() = &list;
  return InfoDialog::GetByKey(key).get();
}


InfoDialog::PageBase::PageBase(InfoDialog& parent, ULONG rid)
: DialogBase(rid, NULLHANDLE),
  Parent(parent)
{}

HWND InfoDialog::PageBase::SetCtrlText(USHORT id, Fields fld, const char* text)
{ HWND ctrl = WinWindowFromID(GetHwnd(), id);
  PMASSERT(ctrl != NULLHANDLE);
  PMRASSERT(WinSetWindowText(ctrl, !(Valid & fld) ? "" : text ? text : "n/a"));
  PMRASSERT(WinEnableWindow(ctrl, !!(Enabled & fld)));
  // Revoke errors from validation
  WinRemovePresParam(ctrl, PP_BACKGROUNDCOLOR);
  return ctrl;
}

HWND InfoDialog::PageBase::SetCtrlCB(USHORT id, Fields fld, bool flag)
{ HWND ctrl = WinWindowFromID(GetHwnd(), id);
  PMASSERT(ctrl != NULLHANDLE);
  WinSendMsg(ctrl, BM_SETCHECK, MPFROMSHORT(Valid & fld ? flag : 2), 0);
  PMRASSERT(WinEnableWindow(ctrl, !!(Enabled & fld)));
  return ctrl;
}

void InfoDialog::PageBase::SetCtrlRB(USHORT id1, Fields fld, int btn)
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

bool InfoDialog::PageBase::SetCtrlEFValid(USHORT id, bool valid)
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
  unsigned long s = (unsigned long)(length + .5);
  if (s < 60)
    sprintf(buffer, "%lu s", s);
  else if (s < 3600)
    sprintf(buffer, "%lu:%02lu", s/60, s%60);
  else if (s < 86400)
    sprintf(buffer, "%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
  else
    sprintf(buffer, "%lud %lu:%02lu:%02lu", s/86400, s/3600%24, s/60%60, s%60);
  return buffer;
}

MRESULT InfoDialog::PageBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("InfoDialog(%p)::PageBase::DlgProc(%x, %x, %x)\n", &Parent, msg, mp1, mp2));
  switch (msg)
  {case WM_INITDLG:
    do_warpsans(GetHwnd());
    WinPostMsg(GetHwnd(), UM_UPDATE, 0, 0);
    break;

   // does funny things
   //case WM_WINDOWPOSCHANGED:
   // if(((SWP*)mp1)[0].fl & SWP_SIZE)
   //   nb_adjust(GetHwnd());
   // break;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

MRESULT InfoDialog::PageTechInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::PageTechInfo::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      const struct Data& data = Parent.GetData();
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
        SetCtrlCB(CB_SAVESTREAM, F_objtype, !!(tech.attributes & TATTR_SAVEABLE));
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
        SetCtrlText(EF_SONGITEMS, F_total_songs, FormatInt(buffer, rpl.totalsongs));
        SetCtrlText(EF_LISTITEMS, F_total_lists, FormatInt(buffer, rpl.totallists));
        SetCtrlCB(CB_ITEMSRECURSIVE, F_recursive, rpl.recursive > 0);
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
      for (size_t i = 0; i != Parent.Key.size(); ++i)
        Parent.Key[i]->RequestInfo(IF_Decoder|IF_Aggreg, PRI_Normal, REL_Reload);
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

InfoFlags InfoDialog::PageTechInfo::GetRequestFlags()
{ return IF_Phys|IF_Tech|IF_Obj|IF_Attr|IF_Rpl|IF_Drpl;
}


HWND InfoDialog::PageMetaInfo::SetEFText(USHORT id, Fields fld, const char* text)
{ HWND ctrl = PageBase::SetCtrlText(id, fld, text);
  WinSendMsg(ctrl, EM_SETREADONLY, MPFROMSHORT(!MetaWrite), 0);
  return ctrl;
}

MRESULT InfoDialog::PageMetaInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {//case WM_INITDLG:
   // break;

   case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      const struct Data& data = Parent.GetData();
      Enabled = data.Enabled;
      Valid = data.Valid;
      const META_INFO& meta = data.Info.Meta;
      MetaWrite = (~Enabled & (F_title|F_artist|F_album|F_year|F_comment|F_genre|F_track|F_copyright)) == 0;
      // TODO:  && data.Info->meta_write;

      WinCheckButton(GetHwnd(), CB_METATITLE,    !!(Valid & F_title));
      WinCheckButton(GetHwnd(), CB_METAARTIST,   !!(Valid & F_artist));
      WinCheckButton(GetHwnd(), CB_METAALBUM,    !!(Valid & F_album));
      WinCheckButton(GetHwnd(), CB_METATRACK,    !!(Valid & F_track));
      WinCheckButton(GetHwnd(), CB_METADATE,     !!(Valid & F_year));
      WinCheckButton(GetHwnd(), CB_METAGENRE,    !!(Valid & F_genre));
      WinCheckButton(GetHwnd(), CB_METACOMMENT,  !!(Valid & F_comment));
      WinCheckButton(GetHwnd(), CB_METACOPYRIGHT,!!(Valid & F_copyright));

      SetEFText(EF_METATITLE, F_title, meta.title);
      SetEFText(EF_METAARTIST, F_artist, meta.artist);
      SetEFText(EF_METAALBUM, F_album, meta.album);
      SetEFText(EF_METATRACK, F_track, meta.track);
      SetEFText(EF_METADATE, F_year, meta.year);
      SetEFText(EF_METAGENRE, F_genre, meta.genre);
      SetEFText(EF_METACOMMENT, F_comment, meta.comment);
      SetEFText(EF_METACOPYRIGHT, F_copyright, meta.copyright);

      SetCtrlText(EF_METARPGAINT, F_track_gain, meta.track_gain <= -1000 ? "n/a" : (sprintf(buffer, "%.1f", meta.track_gain), buffer));
      SetCtrlText(EF_METARPPEAKT, F_track_peak, meta.track_peak <= -1000 ? "n/a" : (sprintf(buffer, "%.1f", meta.track_peak), buffer));
      SetCtrlText(EF_METARPGAINA, F_album_gain, meta.album_gain <= -1000 ? "n/a" : (sprintf(buffer, "%.1f", meta.album_gain), buffer));
      SetCtrlText(EF_METARPPEAKA, F_album_peak, meta.album_peak <= -1000 ? "n/a" : (sprintf(buffer, "%.1f", meta.album_peak), buffer));

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
      WinPostMsg(GetHwnd(), UM_UPDATE, 0, 0);
      return 0;
     case PB_APPLY:
      { // fetch meta data
        META_INFO info;
        /* TODO:   char buffer[12];
        WinQueryDlgItemText(GetHwnd(), EF_METATITLE,    sizeof info.title,    info.title.cdata()    );
        WinQueryDlgItemText(GetHwnd(), EF_METAARTIST,   sizeof info.artist,   info.artist.cdata()   );
        WinQueryDlgItemText(GetHwnd(), EF_METAALBUM,    sizeof info.album,    info.album.cdata()    );
        WinQueryDlgItemText(GetHwnd(), EF_METATRACK,    sizeof buffer, buffer);
        // TODO: info.track = *buffer ? atoi(buffer) : -1;
        WinQueryDlgItemText(GetHwnd(), EF_METADATE,     sizeof info.year,     info.year.cdata()     );
        WinQueryDlgItemText(GetHwnd(), EF_METAGENRE,    sizeof info.genre,    info.genre.cdata()    );
        WinQueryDlgItemText(GetHwnd(), EF_METACOMMENT,  sizeof info.comment,  info.comment.cdata()  );
        WinQueryDlgItemText(GetHwnd(), EF_METACOPYRIGHT,sizeof info.copyright,info.copyright.cdata());
        // Setup flags
        int flags =
          DECODER_HAVE_TITLE    * SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METATITLE    )) |
          DECODER_HAVE_ARTIST   * SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METAARTIST   )) |
          DECODER_HAVE_ALBUM    * SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METAALBUM    )) |
          DECODER_HAVE_TRACK    * SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METATRACK    )) |
          DECODER_HAVE_YEAR     * SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METADATE     )) |
          DECODER_HAVE_GENRE    * SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METAGENRE    )) |
          DECODER_HAVE_COMMENT  * SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METACOMMENT  )) |
          DECODER_HAVE_COPYRIGHT* SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), CB_METACOPYRIGHT));
        // Invoke worker dialog
        MetaWriteDlg(info, flags, *Parent.Key).DoDialog(GetHwnd());
        DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: PB_APPLY - after DoDialog\n", &Parent));*/
        return 0;
      }
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

InfoFlags InfoDialog::PageMetaInfo::GetRequestFlags()
{ return IF_Meta;
}


InfoDialog::MetaWriteDlg::MetaWriteDlg(const META_INFO& info, int flags, const PlayableSetBase& dest)
: DialogBase(DLG_WRITEMETA, NULLHANDLE),
  MetaData(info),
  MetaFlags(flags),
  Dest(dest),
  SkipErrors(false),
  WorkerTID(0),
  Cancel(false)
{ DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::MetaWriteDlg(, %x, {%u,})\n", this, flags, dest.size()));
}

InfoDialog::MetaWriteDlg::~MetaWriteDlg()
{ DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::~MetaWriteDlg() - %u\n", this, WorkerTID));
  // Wait for the worker thread to complete (does not hold the SIQ)
  if (WorkerTID)
  { Cancel = true;
    ResumeSignal.Set();
    wait_thread_pm(amp_player_hab(), WorkerTID, 5*60*1000);
  }
}

MRESULT InfoDialog::MetaWriteDlg::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // setup first destination
    PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMURL, Dest[0]->URL.cdata()));
    PMRASSERT(WinPostMsg(GetHwnd(), UM_START, 0, 0));
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
        PMRASSERT(WinQueryWindowPos(WinWindowFromID(GetHwnd(), SB_WMBARBG), &swp));
        swp.cx = swp.cx * (i+1) / Dest.size();
        PMRASSERT(WinSetWindowPos(WinWindowFromID(GetHwnd(), SB_WMBARFG), NULLHANDLE, 0,0, swp.cx,swp.cy, SWP_SIZE));
      }

      if (mp2 == PLUGIN_OK)
      { // Everything fine, next item
        PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMSTATUS, ""));
        ++i; // next item (if any)
        PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMURL, (size_t)i < Dest.size() ? Dest[i]->URL.cdata() : ""));
        return 0;
      }
      // Error, worker halted
      // TODO:    sprintf(buffer, "%s failed to write meta info. Error %lu - %s", Dest[i]->GetDecoder(), LONGFROMMP(mp2), xio_strerror(LONGFROMMP(mp2)));
      // PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMSTATUS, buffer));
      // 'skip all' was pressed? => continue immediately
      if (SkipErrors)
      { ResumeSignal.Set();
        return 0;
      }
      // Enable skip buttons
      PMRASSERT(WinEnableControl(GetHwnd(), PB_RETRY,     TRUE));
      PMRASSERT(WinEnableControl(GetHwnd(), PB_WMSKIP,    TRUE));
      PMRASSERT(WinEnableControl(GetHwnd(), PB_WMSKIPALL, TRUE));
      return 0;
    }

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case DID_CANCEL:
      Cancel = true;
      ResumeSignal.Set();
      PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMSTATUS, "- cancelling -"));
      PMRASSERT(WinEnableControl( GetHwnd(), DID_CANCEL,  FALSE));
      break;

     case PB_WMSKIPALL:
      SkipErrors = true;
     case PB_WMSKIP:
      // next item
      ++CurrentItem;
     case PB_RETRY:
      // Disable skip buttons
      PMRASSERT(WinEnableControl(GetHwnd(), PB_RETRY,     FALSE));
      PMRASSERT(WinEnableControl(GetHwnd(), PB_WMSKIP,    FALSE));
      PMRASSERT(WinEnableControl(GetHwnd(), PB_WMSKIPALL, FALSE));
      ResumeSignal.Set();
    }
    return 0;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

ULONG InfoDialog::MetaWriteDlg::DoDialog(HWND owner)
{ DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::DoDialog() - %u %x\n", this, Dest.size(), MetaFlags));
  if (Dest.size() == 0 || MetaFlags == 0)
    return DID_OK;
  StartDialog(owner);
  return WinProcessDlg(GetHwnd());
}

void InfoDialog::MetaWriteDlg::Worker()
{ while (!Cancel && CurrentItem < Dest.size())
  { Playable& song = *Dest[CurrentItem];
    DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::Worker - %u %s\n", this, CurrentItem, song.URL.cdata()));
    // Write info
    ULONG rc = 0; // TODO:   song.SaveMetaInfo(MetaData, MetaFlags);
    // Notify dialog
    PMRASSERT(WinPostMsg(GetHwnd(), UM_STATUS, MPFROMLONG(CurrentItem), MPFROMLONG(rc)));
    if (Cancel)
      return;
    // wait for acknowledge
    if (rc != PLUGIN_OK)
    { ResumeSignal.Wait();
      ResumeSignal.Reset();
    } else
      ++CurrentItem;
  }
  WinPostMsg(GetHwnd(), UM_STATUS, MPFROMLONG(-1), MPFROMLONG(0));
  DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::Worker completed\n", this));
}

void TFNENTRY InfoDialogMetaWriteWorkerStub(void* arg)
{ ((InfoDialog::MetaWriteDlg*)arg)->Worker();
}


InfoDialog::InfoDialog(const KeyType& key)
: AInfoDialog(DLG_INFO, NULLHANDLE),
  inst_index<InfoDialog, const KeyType>(key),
  PageTech(*this),
  PageMeta(*this),
  Pages(Page_ItemInfo + 1) // Reserve optimal space for all cases.
{ DEBUGLOG(("InfoDialog(%p)::InfoDialog({%u, %s}) - {%u, %s}\n", this,
    key.size(), key.debug_dump().cdata(), Key.size(), Key.debug_dump().cdata()));
  Pages.append() = &PageMeta;
  Pages.append() = &PageTech;
}

InfoDialog::~InfoDialog()
{ DEBUGLOG(("InfoDialog(%p)::~InfoDialog()\n", this));
}

void InfoDialog::StartDialog()
{ DEBUGLOG(("InfoDialog(%p)::StartDialog()\n", this));
  ManagedDialogBase::StartDialog(HWND_DESKTOP);
  // setup notebook windows
  PageMeta.StartDialog();
  PageTech.StartDialog();
  HWND book = WinWindowFromID(GetHwnd(), NB_INFO);
  PageMeta.PageID = nb_append_tab(book, PageMeta.GetHwnd(), "Meta info", NULL, 0);
  PMASSERT(PageMeta.PageID != 0);
  PageTech.PageID = nb_append_tab(book, PageTech.GetHwnd(), "Tech. info", NULL, 0);
  PMASSERT(PageTech.PageID != 0);
}

InfoDialog::PageBase* InfoDialog::PageFromID(ULONG pageid)
{ for (PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
    if ((*pp)->PageID == pageid)
      return *pp;
  return NULL;
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
      PMXASSERT(WinQueryTaskSizePos(amp_player_hab(), 0, &swp), == 0);
      PMRASSERT(WinSetWindowPos(GetHwnd(), NULLHANDLE, swp.x,swp.y, 0,0, SWP_MOVE));
    }
    do_warpsans(GetHwnd());
    SetHelpMgr(GUI::GetHelpMgr());
    // restore position
    rest_window_pos(GetHwnd(), Key.size() > 1 ? NULL : Key[0]->GetPlayable().URL.cdata());
    break;

   case WM_DESTROY:
    // save position
    save_window_pos(GetHwnd(), Key.size() > 1 ? NULL : Key[0]->GetPlayable().URL.cdata());
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
  return ManagedDialogBase::DlgProc(msg, mp1, mp2);
}

void InfoDialog::ShowPage(PageNo page)
{ ASSERT((size_t)page < Pages.size());
  PageBase* pp = Pages[page];
  ASSERT(pp);
  RequestPage(pp);
  WinSendDlgItemMsg(GetHwnd(), NB_INFO, BKM_TURNTOPAGE, MPFROMLONG(pp->PageID), 0);
  SetVisible(true);
}

int InfoDialog::compareTo(const KeyType& r) const
{ if (&r == &Key)
    return 0; // Comparison to itself
  size_t p1 = 0;
  size_t p2 = 0;
  for (;;)
  { // termination condition
    if (p2 == r.size())
      return p1 != Key.size();
    else if (p1 == Key.size())
      return -1;
    // compare content
    if (Key[p1] > r[p2])
      return 1;
    if (Key[p1] < r[p2])
      return -1;
    ++p1;
    ++p2;
  }
}


SingleInfoDialog::SingleInfoDialog(const KeyType& key)
: InfoDialog(key),
  DataCacheValid(false),
  ContentChangeDeleg(*this, &SingleInfoDialog::ContentChangeEvent)
{ DEBUGLOG(("SingleInfoDialog(%p)::SingleInfoDialog({%p{%s}})\n", this, Key[0], Key[0]->GetPlayable().URL.cdata()));
}

MRESULT SingleInfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    SetTitle(xstring::sprintf("PM123 Object info: %s", Key[0]->GetPlayable().URL.getDisplayName().cdata()));
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
        | ((invalid & IF_Obj ) == 0) * (F_OBJ_INFO&F_songlength)
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
  for (PageBase** pp = Pages.begin(); pp != Pages.end(); ++pp)
    if (*pp && (args.Changed & (*pp)->GetRequestFlags()))
      { DataCacheValid = false;
        WinPostMsg((*pp)->GetHwnd(), PageBase::UM_UPDATE, MPFROMLONG(args.Changed), 0);
      }
}


MultipleInfoDialog::MultipleInfoDialog(const KeyType& key)
: InfoDialog(key),
  DataCacheValid(false)
{ DEBUGLOG(("MultipleInfoDialog(%p)::MultipleInfoDialog({%u %s})\n", this, key.size(), key.debug_dump().cdata()));
}

MRESULT MultipleInfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // Since we are _Multiple_InfoDialog there are at least two items.
    SetTitle(xstring::sprintf("PM123 - %u objects: %s, %s ...", Key.size(), Key[0]->GetPlayable().URL.getShortName().cdata(), Key[1]->GetPlayable().URL.getShortName().cdata()));
    break;
  }
  return InfoDialog::DlgProc(msg, mp1, mp2);
}

const struct InfoDialog::Data& MultipleInfoDialog::GetData()
{ DEBUGLOG(("MultipleInfoDialog(%p)::GetData() %u - %u %s\n", this, DataCacheValid, Key.size(), Key.debug_dump().cdata()));
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
  PMRASSERT(WinEnableControl(GetHwnd(), PB_APPLY, TRUE));
  PMRASSERT(WinEnableControl(GetHwnd(), PB_UNDO, TRUE));
}

void PlayableInstanceInfoDialog::PageItemInfo::Save()
{
  char buffer[1024];
  size_t len;
  bool valid = true;
  // Extract Parameters and validate
  url123 url;
  valid &= SetCtrlEFValid(EF_INFOURL,
    WinQueryDlgItemText(GetHwnd(), EF_INFOURL, sizeof buffer, buffer) != 0
    && (url = url123::normalizeURL(buffer)) != NULL );

  ItemInfo item;
  AttrInfo attr;
  if (WinQueryDlgItemText(GetHwnd(), EF_INFOALIAS, sizeof buffer, buffer) != 0)
    item.alias = buffer;
  if (WinQueryDlgItemText(GetHwnd(), EF_INFOSTART, sizeof buffer, buffer) != 0)
    item.start = buffer;
  if (WinQueryDlgItemText(GetHwnd(), EF_INFOSTOP, sizeof buffer, buffer) != 0)
    item.stop = buffer;
  if (WinQueryDlgItemText(GetHwnd(), EF_INFOAT, sizeof buffer, buffer) != 0)
    attr.at = buffer;
  // Validation of Location makes no sense unless the URL is valid.
  int_ptr<Playable> pp;
  if (valid)
  { pp = Playable::GetByURL(url);
    Location loc(pp);
    const char* cp;
    Location::NavigationResult nr;
    if (item.start)
    { cp = item.start;
      nr = loc.Deserialize(cp, PRI_None);
      // TODO error message?
      SetCtrlEFValid(EF_INFOSTART, nr == NULL || nr.length() == 0); // in dubio pro rheo
    }
    if (item.stop)
    { loc.Reset();
      cp = item.stop;
      nr = loc.Deserialize(cp, PRI_None);
      // TODO error message?
      SetCtrlEFValid(EF_INFOSTOP, nr == NULL || nr.length() == 0); // in dubio pro rheo
    }
    if (attr.at)
    { loc.Reset();
      cp = attr.at;
      nr = loc.Deserialize(cp, PRI_None);
      // TODO error message?
      SetCtrlEFValid(EF_INFOAT, nr == NULL || nr.length() == 0); // in dubio pro rheo
    }
  }
  valid &= SetCtrlEFValid(EF_INFOPREGAP,
    WinQueryDlgItemText(GetHwnd(), EF_INFOPREGAP, sizeof buffer, buffer) == 0
    || (sscanf(buffer, "%f%n", &item.pregap, &len) == 1 && len == strlen(buffer)) );
  valid &= SetCtrlEFValid(EF_INFOPOSTGAP,
    WinQueryDlgItemText(GetHwnd(), EF_INFOPOSTGAP, sizeof buffer, buffer) == 0
    || (sscanf(buffer, "%f%n", &item.postgap, &len) == 1 && len == strlen(buffer)) );
  valid &= SetCtrlEFValid(EF_INFOGAIN,
    WinQueryDlgItemText(GetHwnd(), EF_INFOGAIN, sizeof buffer, buffer) == 0
    || (sscanf(buffer, "%f%n", &item.gain, &len) == 1 && len == strlen(buffer)) );

  attr.ploptions = PLO_ALTERNATION * WinQueryButtonCheckstate(GetHwnd(), CB_INFOALTERNATION)
      | PLO_SHUFFLE * WinSendDlgItemMsg(GetHwnd(), RB_INFOPLSHINHERIT, BM_QUERYCHECKINDEX, 0, 0) ;

  if (!valid)
    return;

  // Save
  PlayableInstance* pi = (PlayableInstance*)Parent.Key[0];
  if (&pi->GetPlayable() != pp)
  { // URL has changed
    Playable& list = (Playable&)*Parent.Key[1];
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
      const struct Data& data = Parent.GetData();
      Enabled = data.Enabled;
      Valid = data.Valid;
      PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_INFOURL, data.URL));
      { const ITEM_INFO& item = data.Info.Item;
        SetCtrlText(EF_INFOALIAS,   F_ITEM_INFO, item.alias);
        SetCtrlText(EF_INFOSTART,   F_ITEM_INFO, item.start);
        SetCtrlText(EF_INFOSTOP,    F_ITEM_INFO, item.stop);
        sprintf(buffer, "%.3f", item.pregap);
        SetCtrlText(EF_INFOPREGAP,  F_ITEM_INFO, item.pregap ? buffer : NULL);
        sprintf(buffer, "%.3f", item.postgap);
        SetCtrlText(EF_INFOPOSTGAP, F_ITEM_INFO, item.postgap ? buffer : NULL);
        sprintf(buffer, "%.1f", item.gain);
        SetCtrlText(EF_INFOGAIN,    F_ITEM_INFO, item.gain ? buffer : NULL);
      }
      { const ATTR_INFO& attr = data.Info.Attr;
        SetCtrlText(EF_INFOAT,      F_ATTR_INFO, attr.at);
        SetCtrlCB(CB_INFOALTERNATION, F_ATTR_INFO, (attr.ploptions & PLO_ALTERNATION) != 0);
        SetCtrlRB(RB_INFOPLSHINHERIT, F_ATTR_INFO, (attr.ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE)) / PLO_SHUFFLE);
      }
      PMRASSERT(WinEnableControl(GetHwnd(), PB_APPLY, FALSE));
      PMRASSERT(WinEnableControl(GetHwnd(), PB_UNDO, FALSE));
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
      WinPostMsg(GetHwnd(), UM_UPDATE, 0, 0);
      return 0;

     case PB_APPLY:
      Save();
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}


PlayableInstanceInfoDialog::PlayableInstanceInfoDialog(const KeyType& key)
: SingleInfoDialog(key),
  PageItem(*this)
{ ASSERT(Key.size() == 2);
  DEBUGLOG(("PlayableInstanceInfoDialog(%p)::PlayableInstanceInfoDialog({%p,%p})", this, Key[0], Key[1]));
  Pages.append() = &PageItem;
}

void PlayableInstanceInfoDialog::StartDialog()
{ SingleInfoDialog::StartDialog();
  PageItem.StartDialog();
  HWND book = WinWindowFromID(GetHwnd(), NB_INFO);
  PageItem.PageID = nb_append_tab(book, PageItem.GetHwnd(), "Playlist item", NULL, 0);
  PMASSERT(PageItem.PageID != 0);
}

