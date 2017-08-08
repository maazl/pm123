/*
 * Copyright 2007-2014 Marcel Mueller
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


#ifndef PM123_INFODIALOG_H
#define PM123_INFODIALOG_H

#define INCL_BASE
#define INCL_PM

#include "ainfodialog.h"
#include "../eventhandler.h"
#include "pm123.rc.h"
#include <vdelegate.h>
#include <cpp/atomic.h>


/****************************************************************************
*
*  Class to handle info dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class InfoDialog
: public AInfoDialog
{private:
  // Flags enum with more than 32 bits
  struct Fields
  { unsigned U1;
    unsigned U2;
    Fields()                                     : U1(0), U2(0) {}
    Fields(unsigned u1, unsigned u2)             : U1(u1), U2(u2) {}

    friend Fields operator |(Fields l, Fields r) { return Fields(l.U1|r.U1, l.U2|r.U2); }
    friend Fields operator &(Fields l, Fields r) { return Fields(l.U1&r.U1, l.U2&r.U2); }
    friend Fields operator ~(Fields v)           { return Fields(~v.U1, ~v.U2); }
    friend Fields operator *(Fields v, bool b)   { return Fields(v.U1&-b, v.U2&-b); }
    friend Fields operator *(bool b, Fields v)   { return Fields(v.U1&-b, v.U2&-b); }
    operator bool() const                        { return U1|U2 != 0; }
    Fields& operator |=(Fields r)                { U1 |= r.U1; U2 |= r.U2; return *this; }
    Fields& operator &=(Fields r)                { U1 &= r.U1; U2 &= r.U2; return *this; }
  };
  static const Fields F_none;
  static const Fields F_URL;
  // PHYS_INFO
  static const Fields F_filesize;
  static const Fields F_timestamp;
  static const Fields F_fattr;
  static const Fields F_PHYS_INFO;
  // TECH_INFO
  static const Fields F_samplerate;
  static const Fields F_channels;
  static const Fields F_tattr;
  static const Fields F_info;
  static const Fields F_format;
  static const Fields F_decoder;
  static const Fields F_TECH_INFO;
  // ATTR_INFO
  static const Fields F_plalt;
  static const Fields F_plshuffle;
  static const Fields F_at;
  static const Fields F_ATTR_INFO;
  // OBJ_INFO
  static const Fields F_songlength;
  static const Fields F_bitrate;
  static const Fields F_num_items;
  static const Fields F_OBJ_INFO;
  // META_INFO
  static const Fields F_title;
  static const Fields F_artist;
  static const Fields F_album;
  static const Fields F_year;
  static const Fields F_comment;
  static const Fields F_genre;
  static const Fields F_track;
  static const Fields F_copyright;
  static const Fields F_track_gain;
  static const Fields F_track_peak;
  static const Fields F_album_gain;
  static const Fields F_album_peak;
  static const Fields F_META_INFO;
  // RPL_INFO - there is no more detailed distinction required
  static const Fields F_RPL_INFO;
  // DRPL_INFO - there is no more detailed distinction required
  static const Fields F_DRPL_INFO;
  // ITEM_INFO
  static const Fields F_alias;
  static const Fields F_start;
  static const Fields F_stop;
  static const Fields F_pregap;
  static const Fields F_postgap;
  static const Fields F_gainadj;
  static const Fields F_ITEM_INFO;

  static const Fields F_ALL;

  struct Data
  { AllInfo         Info;       ///< Thread safe local copy of the item information. In case of multiple items this is a blend.
    xstring         URL;        ///< URL of the item.
    Fields          Enabled;    ///< The following fields are shown enabled, i.e. the content makes sense.
    Fields          Invalid;    ///< The specified field content is invalid, i.e. outstanding information is on the way.
    Fields          NonUnique;  ///< The Content of this field is not unique (only when multiple items, controls check boxes besides fields)
  };
 private:
  /// Base class for notebook pages
  class PageBase : public NotebookDialogBase::PageBase
  {private:
    enum
    { UM_UPDATE = WM_USER+1     ///< Refresh the control's content from the backend.
    };
   protected:
    //InfoDialog&     Parent;
    const Data*     PData;      // Valid only while UM_UPDATE
   private:
    AtomicUnsigned  UpdateFlags;
   protected:
                    PageBase(InfoDialog& parent, ULONG rid, const xstring& title);
    InfoDialog&     GetParent() { return (InfoDialog&)Parent; }
    xstring         QueryItemTextOrNULL(ULONG id);
    bool            QueryGatedTextOrNULL(ULONG idcb, ULONG idef, xstring& field);
    void            CheckButton(ULONG id, unsigned state);
    HWND            SetCtrlText(ULONG id, Fields fld, const char* text);
    void            SetCtrlCB(ULONG id, Fields fld, USHORT check);
    void            SetCtrlRB(ULONG id1, Fields fld, int btn);
    bool            SetCtrlEFValid(ULONG id, bool valid);
    static const char* FormatInt(char* buffer, int value, const char* unit = "");
    static const char* FormatGain(char* buffer, float gain);
    static const char* FormatTstmp(char* buffer, time_t time);
    static const char* FormatDuration(char* buffer, PM123_TIME time);
    //static bool     ParseDuration(const char* text, float& duration);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    virtual void    Update(InfoFlags what) = 0;
   public:
    void            RequestUpdate(InfoFlags what);
    /// Return the InfoFlags to be requested
    virtual InfoFlags GetRequestFlags() = 0;
  };
  /// Notebook page 1
  class PageTechInfo;
  friend class PageTechInfo;
  class PageTechInfo : public PageBase
  { void            SetCtrlCB3(ULONG id, Fields fld, bool flag) { SetCtrlCB(id, fld, PData->Invalid & fld ? 2 : flag); }
    /// Dialog procedure
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    virtual void    Update(InfoFlags what);
   public:
    PageTechInfo(InfoDialog& parent) : PageBase(parent, DLG_TECHINFO, "Tech. info") {}
    virtual InfoFlags GetRequestFlags();
  };
  /// Notebook page 2
  class PageMetaInfo;
  friend class PageMetaInfo;
  class PageMetaInfo : public PageBase
  {private:
    bool            MetaWrite;  // Valid only while UM_UPDATE
   private:
    /// Dialog procedure
    HWND            SetEFText(ULONG id, Fields fld, const char* text);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    virtual void    Update(InfoFlags what);
   public:
    PageMetaInfo(InfoDialog& parent) : PageBase(parent, DLG_METAINFO, "Meta info") {}
    virtual InfoFlags GetRequestFlags();
  };
  /// Notebook page 3
  class PageItemInfo;
  friend class PageItemInfo;
  class PageItemInfo : public PageBase
  {private:
    bool            InUpdate;
    bool            Modified;
   private:
    void            SetModified();
    HWND            SetCtrlText(ULONG id, Fields fld, const char* text)
                    { return PageBase::SetCtrlText(id, fld, text ? text : ""); }
    static const char* FormatGap(char* buffer, float time);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    virtual void    Update(InfoFlags what);
    void            Save();
   public:
    PageItemInfo(InfoDialog& parent) : PageBase(parent, DLG_ITEMINFO, "Playlist item"), InUpdate(false), Modified(false) {}
    virtual InfoFlags GetRequestFlags();
  };
 private:
  class Merger
  { Data&           Target;
    Fields          AggEnabled;
    Fields          AggInvalid;
    Fields          Enabled;
    Fields          Invalid;
    void            MergeField(xstring& dst, const volatile xstring& src, Fields fld);
    void            MergeField(double& dst, double src, Fields fld);
    void            MergeField(int& dst, int src, Fields fld);
    void            MergeField(unsigned& dst, unsigned src, Fields fld);
    void            MergeMaskedField(unsigned& dst, unsigned src, Fields fld, unsigned mask);
    void            MergeField(float& dst, float src, Fields fld);
    void            MergeRVAField(float& dst, float src, Fields fld);
   public:
                    Merger(Data& target) : Target(target), AggEnabled(F_none), AggInvalid(~F_none) {}
    void            Merge(APlayable& item);
  };

 private:
          KeyType   Key;
 private:
  struct  Data      DataCache;
          bool      DataCacheValid;
 private:
  // Enable array creation
  struct DelegateHelper
  : public class_delegate<InfoDialog, const PlayableChangeArgs>
  { DelegateHelper() : class_delegate<InfoDialog, const PlayableChangeArgs>(*(InfoDialog*)NULL, &InfoDialog::ContentChangeEvent) {}
  };
  sco_arr<DelegateHelper> ChangeDelegs;

 private: // non copyable
                    InfoDialog(const InfoDialog&);
          void      operator=(const InfoDialog&);
 private:
  /// Initialize InfoDialog
                    InfoDialog(const KeyType& key);
  const   KeyType&  GetKey() const           { return Key; }
          PageBase* PageFromID(ULONG pageid) { return (PageBase*)AInfoDialog::PageFromID(pageid); }
          void      RequestPage(PageBase* page);
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
          void      SetTitle();
          void      ContentChangeEvent(const PlayableChangeArgs& args);
          void      LoadData();
  /// Replace the key of the current instance and update the repository.
          void      ReKey(KeyType& key);

 public:
  virtual           ~InfoDialog();
          void      StartDialog() { ManagedDialog<NotebookDialogBase>::StartDialog(HWND_DESKTOP, NB_INFO); }
  virtual bool      IsVisible(PageNo page) const;
  virtual void      ShowPage(PageNo page);

 private: // Instance Repository
  static InfoDialog* Factory(const KeyType& key);
  static InfoDialog* DummyFactory(const KeyType& key, void* that);
  static int        Comparer(const KeyType& key, const InfoDialog& inst);
  typedef inst_index<InfoDialog, const KeyType, &InfoDialog::Comparer> Repository;
 public:
  /// Factory method. Returns always the same instance for the same set of objects.
  static int_ptr<InfoDialog> GetByKey(const KeyType& obj)
                    { return Repository::GetByKey(obj, &InfoDialog::Factory); }
  static int_ptr<InfoDialog> FindByKey(const KeyType& obj)
                    { return Repository::FindByKey(obj); }
  static void       DestroyAll();
};

#endif
