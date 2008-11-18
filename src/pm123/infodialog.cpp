/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 Marcel Mueller
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
#include "infodialog.h"
#include "pm123.h"
#include "pm123.rc.h"
#include "dialog.h"
#include "glue.h"
#include <decoder_plug.h>
#include <plugin.h>
#include <utilfct.h>

#include <stdio.h>


class SingleInfoDialog 
: public InfoDialog
{private:
  struct Data       DataCache;
  bool              DataCacheValid;
  class_delegate<SingleInfoDialog, const Playable::change_args> ContentChangeDeleg;

 public:
                    SingleInfoDialog(const PlayableSetBase& key);
 private:
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual struct Data GetData();
  void              ContentChangeEvent(const Playable::change_args& args);
};


class MultipleInfoDialog
: public InfoDialog
{private:
  struct Data       DataCache;
  bool              DataCacheValid;
  Playable::DecoderInfo MergedInfo;

 public:
                    MultipleInfoDialog(const PlayableSetBase& key);
 private: // Dialog functions
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
 private: // Data source
  virtual struct Data GetData();
  void              ApplyFlags(const Playable& item);
  void              JoinInfo(const Playable& item);
  void              CompareField(Fields fld, const char* l, const char* r);
  void              CompareField(Fields fld, int l, int r)       { if (l != r) DataCache.Valid &= ~fld; }
  void              CompareField(Fields fld, double l, double r) { if (l != r) DataCache.Valid &= ~fld; }
};



InfoDialog::PageBase::PageBase(InfoDialog& parent, ULONG rid)
: DialogBase(rid, NULLHANDLE),
  Parent(parent)
{}

HWND InfoDialog::PageBase::SetCtrlText(USHORT id, Fields fld, const char* text)
{ HWND ctrl = WinWindowFromID(GetHwnd(), id);
  PMASSERT(ctrl != NULLHANDLE);
  PMRASSERT(WinSetWindowText(ctrl, Valid & fld ? text : ""));
  PMRASSERT(WinEnableWindow(ctrl, !!(Enabled & fld)));
  return ctrl;
}

MRESULT InfoDialog::PageBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("InfoDialog(%p)::PageBase::DlgProc(%x, %x, %x)\n", &Parent, msg, mp1, mp2));
  switch (msg)
  {case WM_INITDLG:
    do_warpsans(GetHwnd());
    WinPostMsg(GetHwnd(), UM_UPDATE, 0, 0);
    break;

   /* does funny things
   case WM_WINDOWPOSCHANGED:
    if(((SWP*)mp1)[0].fl & SWP_SIZE)
      nb_adjust(GetHwnd());
    break;*/
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

MRESULT InfoDialog::PageTechInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::PageTechInfo::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      HWND ctrl;
      const struct Data& data = Parent.GetData();
      Enabled = data.Enabled;
      Valid = data.Valid;
      { const PHYS_INFO& phys = *data.Info->phys;
        SetCtrlText(EF_FILESIZE, F_filesize, phys.filesize < 0 ? "n/a" : (sprintf(buffer, "%.3f kiB", phys.filesize/1024.), buffer));
        SetCtrlText(EF_NUMITEMS, F_num_items, phys.num_items <= 0 ? "n/a" : (sprintf(buffer, "%i", phys.num_items), buffer));
      }
      { const TECH_INFO& tech = *data.Info->tech;
        SetCtrlText(EF_TOTALSIZE, F_totalsize, tech.totalsize < 0 ? "n/a" : (sprintf(buffer, "%.3f MiB", tech.totalsize/(1024.*1024.)), buffer));
        // playing time
        if (tech.songlength < 0)
          strcpy(buffer, "n/a");
        else
        { unsigned long s = (unsigned long)tech.songlength;
          if (s < 60)
            sprintf(buffer, "%lu s", s);
           else if (s < 3600)
            sprintf(buffer, "%lu:%02lu", s/60, s%60);
           else
            sprintf(buffer, "%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
        }
        SetCtrlText(EF_TOTALTIME, F_songlength, buffer);
        SetCtrlText(EF_BITRATE, F_bitrate, tech.bitrate < 0 ? "n/a" : (sprintf(buffer, "%i kbps", tech.bitrate), buffer));
        SetCtrlText(EF_INFOSTRINGS, F_info, tech.info);
      }
      { const RPL_INFO& rpl = *data.Info->rpl;
        SetCtrlText(EF_SONGITEMS, F_total_items, rpl.total_items <= 0 ? "n/a" : (sprintf(buffer, "%i", rpl.total_items), buffer));
        // recursion flag
        ctrl = WinWindowFromID(GetHwnd(), CB_ITEMSRECURSIVE);
        WinSendMsg(ctrl, BM_SETCHECK, MPFROMSHORT(Valid & F_recursive ? !!rpl.recursive : 2), 0);
        PMRASSERT(WinEnableWindow(ctrl, !!(Enabled & F_recursive)));
      }
      { const FORMAT_INFO2& format = *data.Info->format;
        SetCtrlText(EF_SAMPLERATE, F_samplerate, format.samplerate < 0 ? "n/a" : (sprintf(buffer, "%i Hz", format.samplerate), buffer));
        SetCtrlText(EF_NUMCHANNELS, F_channels, format.channels < 0 ? "n/a" : (sprintf(buffer, "%i", format.channels), buffer));
      }
      // Decoder
      SetCtrlText(EF_DECODER, F_decoder, data.Decoder ? data.Decoder : "n/a");
      { const META_INFO& meta = *data.Info->meta;
        SetCtrlText(EF_METARPGAINT, F_track_gain, meta.track_gain == 0 ? "" : (sprintf(buffer, "%.1f", meta.track_gain), buffer));
        SetCtrlText(EF_METARPPEAKT, F_track_peak, meta.track_peak == 0 ? "" : (sprintf(buffer, "%.1f", meta.track_peak), buffer));
        SetCtrlText(EF_METARPGAINA, F_album_gain, meta.album_gain == 0 ? "" : (sprintf(buffer, "%.1f", meta.album_gain), buffer));
        SetCtrlText(EF_METARPPEAKA, F_album_peak, meta.album_peak == 0 ? "" : (sprintf(buffer, "%.1f", meta.album_peak), buffer));
      }
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

HWND InfoDialog::PageMetaInfo::SetEFText(USHORT id, Fields fld, const char* text)
{ HWND ctrl = PageBase::SetCtrlText(id, fld, text);
  WinSendMsg(ctrl, EM_SETREADONLY, MPFROMSHORT(!MetaWrite), 0);
  return ctrl;
} 

MRESULT InfoDialog::PageMetaInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    SetHelpMgr(amp_help_mgr());
    break;
    
   case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      HWND ctrl;
      const struct Data& data = Parent.GetData();
      Enabled = data.Enabled;
      Valid = data.Valid;
      const META_INFO& meta = *data.Info->meta;
      MetaWrite = (~Enabled & (F_title|F_artist|F_album|F_year|F_comment|F_genre|F_track|F_copyright)) == 0 
        && data.Info->meta_write;

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
      SetEFText(EF_METATRACK, F_track, meta.track <= 0 ? "" : (sprintf(buffer, "%i", meta.track), buffer));
      SetEFText(EF_METADATE, F_year, meta.year);
      SetEFText(EF_METAGENRE, F_genre, meta.genre);
      SetEFText(EF_METACOMMENT, F_comment, meta.comment);
      SetEFText(EF_METACOPYRIGHT, F_copyright, meta.copyright);
      
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
        char buffer[12];
        WinQueryDlgItemText(GetHwnd(), EF_METATITLE,    sizeof info.title,    info.title    );
        WinQueryDlgItemText(GetHwnd(), EF_METAARTIST,   sizeof info.artist,   info.artist   );
        WinQueryDlgItemText(GetHwnd(), EF_METAALBUM,    sizeof info.album,    info.album    );
        WinQueryDlgItemText(GetHwnd(), EF_METATRACK,    sizeof buffer, buffer);
        info.track = *buffer ? atoi(buffer) : -1;
        WinQueryDlgItemText(GetHwnd(), EF_METADATE,     sizeof info.year,     info.year     );
        WinQueryDlgItemText(GetHwnd(), EF_METAGENRE,    sizeof info.genre,    info.genre    );
        WinQueryDlgItemText(GetHwnd(), EF_METACOMMENT,  sizeof info.comment,  info.comment  );
        WinQueryDlgItemText(GetHwnd(), EF_METACOPYRIGHT,sizeof info.copyright,info.copyright);
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
        DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: PB_APPLY - after DoDialog\n", &Parent));
        return 0;
      }
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
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
{ char buffer[60];
  switch (msg)
  {case WM_INITDLG:
    // setup first destination   
    PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMURL, Dest[0]->GetURL().cdata()));
    PMRASSERT(WinPostMsg(GetHwnd(), UM_START, 0, 0));
    break; 
     
   case UM_START:
    DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::DlgProc: UM_START\n", this));
    CurrentItem = 0;
    WorkerTID = _beginthread(&InfoDialogMetaWriteWorkerStub, NULL, 65536, this);
    ASSERT(WorkerTID != -1);
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
        PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMURL, i < Dest.size() ? Dest[i]->GetURL().cdata() : ""));
        return 0;
      }
      // Error, worker halted
      sprintf(buffer, "Failed to write meta info. %s reported error %lu", Dest[i]->GetDecoder(), LONGFROMMP(mp2));
      PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMSTATUS, buffer));
      // 'skip all' was pressed? => continue immediately
      if (SkipErrors)
      { ResumeSignal.Set();
        return 0;
      }
      // Enable skip buttons
      PMRASSERT(WinEnableControl(GetHwnd(), PB_WMRETRY,   TRUE));
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
     case PB_WMRETRY:
      // Disable skip buttons
      PMRASSERT(WinEnableControl(GetHwnd(), PB_WMRETRY,   FALSE));
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
  { Song& song = (Song&)*Dest[CurrentItem];
    DEBUGLOG(("InfoDialog::MetaWriteDlg(%p)::Worker - %u %s\n", this, CurrentItem, song.GetURL().cdata()));
    // Write info
    ULONG rc = song.SaveMetaInfo(MetaData, MetaFlags);
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


InfoDialog::InfoDialog(const PlayableSetBase& key)
: ManagedDialogBase(DLG_INFO, NULLHANDLE),
  OwnedPlayableSet(key),
  inst_index<InfoDialog, const PlayableSetBase*const>(this),
  PageTech(*this, CFG_TECHINFO),
  PageMeta(*this, CFG_METAINFO)
{ DEBUGLOG(("InfoDialog(%p)::InfoDialog({%u, %s}) - {%u, %s}\n", this,
    key.size(), key.DebugDump().cdata(), Key->size(), Key->DebugDump().cdata()));
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
  PageIDs[Page_MetaInfo] = nb_append_tab(book, PageMeta.GetHwnd(), "Meta info", NULL, 0);
  PMASSERT(PageIDs[Page_MetaInfo] != 0);
  PageIDs[Page_TechInfo] = nb_append_tab(book, PageTech.GetHwnd(), "Tech. info", NULL, 0);
  PMASSERT(PageIDs[Page_TechInfo] != 0);
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
    break;

   case WM_SYSCOMMAND:
    if( SHORT1FROMMP(mp1) == SC_CLOSE )
    { Destroy();
      return 0;
    }
    break;
  }
  return ManagedDialogBase::DlgProc(msg, mp1, mp2);
}

void InfoDialog::ShowPage(PageNo page)
{ WinSendDlgItemMsg(GetHwnd(), NB_INFO, BKM_TURNTOPAGE, MPFROMLONG(PageIDs[page]), 0);
  SetVisible(true);
}

// Factory ...

InfoDialog::Factory InfoDialog::Factory::Instance;

InfoDialog* InfoDialog::Factory::operator()(const PlayableSetBase*const& key)
{ DEBUGLOG(("InfoDialog::Factory::operator()({%u, %s})\n", key->size(), key->DebugDump().cdata()));
  InfoDialog* ret = key->size() > 1
    ? (InfoDialog*)new MultipleInfoDialog(*key)
    : new SingleInfoDialog(*key);
  ret->StartDialog();
  return ret;
}

int_ptr<InfoDialog> InfoDialog::GetByKey(const PlayableSetBase& obj)
{ DEBUGLOG(("InfoDialog::GetByKey({%u, %s})\n", obj.size(), obj.DebugDump().cdata()));
  if (((const sorted_vector<Playable, Playable>&)obj).size() == 0)
    return NULL;
  // provide factory
  int_ptr<InfoDialog> ret = inst_index<InfoDialog, const PlayableSetBase*const>::GetByKey(&obj, Factory::Instance);
  return ret;
}
int_ptr<InfoDialog> InfoDialog::GetByKey(Playable* obj)
{ OwnedPlayableSet set;
  set.get(*obj) = obj;
  return GetByKey(set);
}

int InfoDialog::compareTo(const PlayableSetBase*const& r) const
{ return Key->compareTo(*r);
}


SingleInfoDialog::SingleInfoDialog(const PlayableSetBase& key)
: InfoDialog(key),
  DataCacheValid(false),
  ContentChangeDeleg(*this, &SingleInfoDialog::ContentChangeEvent)
{ DEBUGLOG(("SingleInfoDialog(%p)::SingleInfoDialog({%p{%s}})\n", this, (*Key)[0], (*Key)[0]->GetURL().cdata()));
}

MRESULT SingleInfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    SetTitle(xstring::sprintf("PM123 Object info: %s", (*Key)[0]->GetURL().getDisplayName().cdata()));
    // register for change event
    (*Key)[0]->InfoChange += ContentChangeDeleg;
    break;

   case WM_DESTROY:
    ContentChangeDeleg.detach();
    break;
  }
  return InfoDialog::DlgProc(msg, mp1, mp2);
}

struct InfoDialog::Data SingleInfoDialog::GetData()
{ DEBUGLOG(("SingleInfoDialog(%p)::GetData() %u - %p\n", this, DataCacheValid, (*Key)[0]));
  if (!DataCacheValid)
  { DataCacheValid = true;
    // Fetch Data
    DataCache.Info = &(*Key)[0]->GetInfo();
    DataCache.Decoder = (*Key)[0]->GetDecoder();
    DataCache.Enabled = (*Key)[0]->GetFlags() & Playable::Enumerable
      ? F_decoder|F_TECH_INFO|F_PHYS_INFO|F_RPL_INFO
      : F_decoder|F_FORMAT_INFO2|F_TECH_INFO|F_META_INFO|F_filesize;
    DataCache.Valid = 
      (bool)((*Key)[0]->CheckInfo(Playable::IF_Phys  ) == 0) * F_PHYS_INFO |
      (bool)((*Key)[0]->CheckInfo(Playable::IF_Tech  ) == 0) * F_TECH_INFO |
      (bool)((*Key)[0]->CheckInfo(Playable::IF_Rpl   ) == 0 && ((*Key)[0]->GetFlags() & Playable::Enumerable)) * F_RPL_INFO |
      (bool)((*Key)[0]->CheckInfo(Playable::IF_Format) == 0) * F_FORMAT_INFO2 |
      (bool)((*Key)[0]->CheckInfo(Playable::IF_Other ) == 0) * F_decoder |
      (bool)((*Key)[0]->CheckInfo(Playable::IF_Meta  ) == 0) * F_META_INFO;
  }
  DEBUGLOG(("SingleInfoDialog::GetData() %s %x %x\n", DataCache.Decoder, DataCache.Enabled, DataCache.Valid));
  return DataCache;
}

void SingleInfoDialog::ContentChangeEvent(const Playable::change_args& args)
{ DEBUGLOG(("SingleInfoDialog(%p)::ContentChangeEvent({&%p, %x, %x})\n",
    this, &args.Instance, args.Changed, args.Loaded));
  if (args.Changed & (Playable::IF_Format|Playable::IF_Tech|Playable::IF_Phys|Playable::IF_Meta|Playable::IF_Rpl|Playable::IF_Other))
  { DataCacheValid = false;
    WinPostMsg(PageTech.GetHwnd(), PageBase::UM_UPDATE, MPFROMLONG(args.Changed), 0);
    if (args.Changed & Playable::IF_Meta)
      WinPostMsg(PageMeta.GetHwnd(), PageBase::UM_UPDATE, MPFROMLONG(args.Changed), 0);
  }
}


MultipleInfoDialog::MultipleInfoDialog(const PlayableSetBase& key)
: InfoDialog(key),
  DataCacheValid(false)
{ DEBUGLOG(("MultipleInfoDialog(%p)::MultipleInfoDialog({%u %s})\n", this, key.size(), key.DebugDump().cdata()));
}

MRESULT MultipleInfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // Since we are _Multiple_InfoDialog there are at least two items.
    SetTitle(xstring::sprintf("PM123 - %u objects: %s, %s ...", Key->size(), (*Key)[0]->GetURL().getShortName().cdata(), (*Key)[1]->GetURL().getShortName().cdata()));
    break;
  }
  return InfoDialog::DlgProc(msg, mp1, mp2);
}

struct InfoDialog::Data MultipleInfoDialog::GetData()
{ DEBUGLOG(("MultipleInfoDialog(%p)::GetData() %u - %u %s\n", this, DataCacheValid, Key->size(), Key->DebugDump().cdata()));
  if (!DataCacheValid)
  { DataCacheValid = true;
    DataCache.Info    = &MergedInfo;
    DataCache.Enabled = ~F_none;
    DataCache.Valid   = ~F_none;
    // merge data
    size_t i = 0;
    // Initial Data
    Playable* pp = (*Key)[0];
    MergedInfo = pp->GetInfo();
    DataCache.Decoder = pp->GetDecoder();
    for(;;)
    { ApplyFlags(*pp);
      if (++i == Key->size())
        break;
      pp = (*Key)[i];
      JoinInfo(*pp);
    }
  }
  DEBUGLOG(("MultipleInfoDialog::GetData() %s %x %x\n", DataCache.Decoder, DataCache.Enabled, DataCache.Valid));
  return DataCache;
}

void MultipleInfoDialog::ApplyFlags(const Playable& item)
{ DEBUGLOG(("MultipleInfoDialog(%p)::ApplyFlags(&%p)\n", this, &item));
  DataCache.Enabled &= item.GetFlags() & Playable::Enumerable
    ? F_decoder|F_TECH_INFO|F_PHYS_INFO|F_RPL_INFO
    : F_decoder|F_FORMAT_INFO2|F_TECH_INFO|F_META_INFO|F_filesize;
  DataCache.Valid &= 
    (bool)(item.CheckInfo(Playable::IF_Phys  ) == 0) * F_PHYS_INFO |
    (bool)(item.CheckInfo(Playable::IF_Tech  ) == 0) * F_TECH_INFO |
    (bool)(item.CheckInfo(Playable::IF_Rpl   ) == 0 && (item.GetFlags() & Playable::Enumerable)) * F_RPL_INFO |
    (bool)(item.CheckInfo(Playable::IF_Format) == 0) * F_FORMAT_INFO2 |
    (bool)(item.CheckInfo(Playable::IF_Other ) == 0) * F_decoder |
    (bool)(item.CheckInfo(Playable::IF_Meta  ) == 0) * F_META_INFO;
}

void MultipleInfoDialog::JoinInfo(const Playable& song)
{ // Decoder
  CompareField(F_decoder, DataCache.Decoder, song.GetDecoder());
  
  const DECODER_INFO2& info = song.GetInfo();
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
  CompareField(F_recursive,  MergedInfo.rpl->recursive,    info.rpl->recursive);
}

void MultipleInfoDialog::CompareField(Fields fld, const char* l, const char* r)
{ if (DataCache.Valid & fld && strcmp(l, r) != 0)
    DataCache.Valid &= ~fld;
}

