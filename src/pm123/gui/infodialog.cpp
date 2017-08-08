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


#define INCL_BASE
#define INCL_PM

#include "infodialog.h"
#include "metawritedlg.h"
#include "../core/job.h"
#include "../pm123.h"
#include "gui.h"
#include "pm123.rc.h"
#include "../configuration.h"
#include <utilfct.h> // do_warpsans
#include <cpp/container/inst_index.h>
#include <cpp/pmutils.h>
#include <cpp/dlgcontrols.h>

#include <stdio.h>
#include <math.h>


const InfoDialog::Fields InfoDialog::F_none      (0x00000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_URL       (0x00000001, 0x00000000);
// PHYS_INFO
const InfoDialog::Fields InfoDialog::F_filesize  (0x00000002, 0x00000000);
const InfoDialog::Fields InfoDialog::F_timestamp (0x00000004, 0x00000000);
const InfoDialog::Fields InfoDialog::F_fattr     (0x00000008, 0x00000000);
const InfoDialog::Fields InfoDialog::F_PHYS_INFO (0x0000000e, 0x00000000);
// TECH_INFO
const InfoDialog::Fields InfoDialog::F_samplerate(0x00000010, 0x00000000);
const InfoDialog::Fields InfoDialog::F_channels  (0x00000020, 0x00000000);
const InfoDialog::Fields InfoDialog::F_tattr     (0x00000040, 0x00000000);
const InfoDialog::Fields InfoDialog::F_info      (0x00000080, 0x00000000);
const InfoDialog::Fields InfoDialog::F_format    (0x00000100, 0x00000000);
const InfoDialog::Fields InfoDialog::F_decoder   (0x00000200, 0x00000000);
const InfoDialog::Fields InfoDialog::F_TECH_INFO (0x000003f0, 0x00000000);
// ATTR_INFO
const InfoDialog::Fields InfoDialog::F_plalt     (0x00001000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_plshuffle (0x00002000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_at        (0x00004000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_ATTR_INFO (0x00007000, 0x00000000);
// OBJ_INFO
const InfoDialog::Fields InfoDialog::F_songlength(0x00010000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_bitrate   (0x00020000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_num_items (0x00040000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_OBJ_INFO  (0x000e0000, 0x00000000);
// META_INFO
const InfoDialog::Fields InfoDialog::F_title     (0x00100000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_artist    (0x00200000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_album     (0x00400000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_year      (0x00800000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_comment   (0x01000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_genre     (0x02000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_track     (0x04000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_copyright (0x08000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_track_gain(0x10000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_track_peak(0x20000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_album_gain(0x40000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_album_peak(0x80000000, 0x00000000);
const InfoDialog::Fields InfoDialog::F_META_INFO (0xfff00000, 0x00000000);
// RPL_INFO - there is no more detailed distinction required
const InfoDialog::Fields InfoDialog::F_RPL_INFO  (0x00000000, 0x00000001);
// DRPL_INFO - there is no more detailed distinction required
const InfoDialog::Fields InfoDialog::F_DRPL_INFO (0x00000000, 0x00000002);
// ITEM_INFO
const InfoDialog::Fields InfoDialog::F_alias     (0x00000000, 0x00000010);
const InfoDialog::Fields InfoDialog::F_start     (0x00000000, 0x00000020);
const InfoDialog::Fields InfoDialog::F_stop      (0x00000000, 0x00000040);
const InfoDialog::Fields InfoDialog::F_pregap    (0x00000000, 0x00000080);
const InfoDialog::Fields InfoDialog::F_postgap   (0x00000000, 0x00000100);
const InfoDialog::Fields InfoDialog::F_gainadj   (0x00000000, 0x00000200);
const InfoDialog::Fields InfoDialog::F_ITEM_INFO (0x00000000, 0x000003f0);

const InfoDialog::Fields InfoDialog::F_ALL       (0xffffffff, 0xffffffff);


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
  ret = new InfoDialog(key);
  // Go!
  ret->StartDialog();
  return ret;
}

InfoDialog* InfoDialog::DummyFactory(const KeyType& key, void* that)
{ ASSERT(&((InfoDialog*)that)->Key == &key); // Ensure instance equality.
  return (InfoDialog*)that;
}

int InfoDialog::Comparer(const KeyType& key, const InfoDialog& inst)
{ return KeyType::compare(key, inst.Key);
}

void InfoDialog::DestroyAll()
{ Repository::IXAccess index;
  DEBUGLOG(("InfoDialog::DestroyAll()\n"));
  // The instances deregister itself from the repository.
  for (;;)
  { Repository::IndexType::iterator pp(index->begin());
    if (pp.isend())
      break;
    (*pp)->Destroy();
  }
}

int_ptr<AInfoDialog> AInfoDialog::GetByKey(const KeyType& set)
{ DEBUGLOG(("AInfoDialog::GetByKey({%u,...})\n", set.size()));
  if (set.size() == 0)
    return (AInfoDialog*)NULL;
  return InfoDialog::GetByKey(set).get();
}
int_ptr<AInfoDialog> AInfoDialog::GetByKey(APlayable& obj)
{ InfoDialog::KeyType key(1);
  key.append() = &obj;
  return InfoDialog::GetByKey(key).get();
}

int_ptr<AInfoDialog> AInfoDialog::FindByKey(APlayable& obj)
{ InfoDialog::KeyType key(1);
  key.append() = &obj;
  return InfoDialog::FindByKey(key).get();
}

void AInfoDialog::DestroyAll()
{ InfoDialog::DestroyAll();
}


InfoDialog::PageBase::PageBase(InfoDialog& parent, ULONG rid, const xstring& title)
: NotebookDialogBase::PageBase(parent, rid, NULLHANDLE, DF_AutoResize)
{ MajorTitle = title;
}

xstring InfoDialog::PageBase::QueryItemTextOrNULL(ULONG id)
{ const xstring& ret = WinQueryDlgItemXText(GetHwnd(), id);
  return ret.length() ? ret : xstring();
}

bool InfoDialog::PageBase::QueryGatedTextOrNULL(ULONG idcb, ULONG idef, xstring& field)
{ if (SHORT1FROMMR(WinQueryButtonCheckstate(GetHwnd(), idcb)))
  { field = QueryItemTextOrNULL(idef);
    return true;
  }
  return false;
}

inline void InfoDialog::PageBase::CheckButton(ULONG id, unsigned state)
{ WinCheckButton(GetHwnd(), id, state);
}

HWND InfoDialog::PageBase::SetCtrlText(ULONG id, Fields fld, const char* text)
{ HWND ctrl = WinWindowFromID(GetHwnd(), id);
  PMASSERT(ctrl != NULLHANDLE);
  PMRASSERT(WinSetWindowText(ctrl, PData->Invalid & fld ? "" : text ? text : "n/a"));
  PMRASSERT(WinEnableWindow(ctrl, !!(PData->Enabled & fld)));
  // Revoke errors from validation
  WinRemovePresParam(ctrl, PP_BACKGROUNDCOLOR);
  return ctrl;
}

void InfoDialog::PageBase::SetCtrlCB(ULONG id, Fields fld, USHORT check)
{ CheckBox ctrl(GetCtrl(id));
  ctrl.CheckState(check);
  ctrl.Enabled((PData->Enabled & fld) != 0);
}

void InfoDialog::PageBase::SetCtrlRB(ULONG id1, Fields fld, int btn)
{ DEBUGLOG(("InfoDialog(%p)::PageBase::SetCtrlRB(%u, %x:%x, %i) - %x:%x, %x:%x\n", &Parent, id1, fld.U2,fld.U1, btn, PData->Invalid.U2,PData->Invalid.U1, PData->Enabled.U2,PData->Enabled.U1));
  HWND ctrl1 = WinWindowFromID(GetHwnd(), id1);
  PMASSERT(ctrl1 != NULLHANDLE);
  // For all radio buttons in this group
  HWND ctrl = ctrl1;
  HWND ctrl2;
  BOOL enabled = !!(PData->Enabled & fld);
  do
  { PMRASSERT(WinEnableWindow(ctrl, enabled));
    WinSendMsg(ctrl, BM_SETCHECK, MPFROMSHORT(btn == 0), 0);
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
    RequestUpdate(~IF_None);
    break;

   case UM_UPDATE:
    { DEBUGLOG(("InfoDialog(%p)::PageBase::DlgProc: UM_UPDATE\n", &Parent));
      InfoFlags what = (InfoFlags)UpdateFlags.swap(0);
      InfoDialog& parent = GetParent();
      parent.LoadData();
      PData = &parent.DataCache;
      Update(what);
      break;
    }
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

void InfoDialog::PageBase::RequestUpdate(InfoFlags what)
{ if (!what)
    return;
  unsigned last = UpdateFlags;
  UpdateFlags |= what;
  if (!last)
    PostMsg(UM_UPDATE, 0,0);
}

MRESULT InfoDialog::PageTechInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_COMMAND:
    DEBUGLOG(("InfoDialog(%p)::PageTechInfo::DlgProc: WM_COMMAND %u\n", &Parent, SHORT1FROMMP(mp1)));
    switch (SHORT1FROMMP(mp1))
    {case PB_REFRESH:
      for (size_t i = 0; i != GetParent().GetKey().size(); ++i)
        GetParent().GetKey()[i]->RequestInfo(IF_Decoder|IF_Aggreg, PRI_Normal, REL_Reload);
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

void InfoDialog::PageTechInfo::Update(InfoFlags what)
{ DEBUGLOG(("InfoDialog(%p)::PageTechInfo::Update(%x)\n", &Parent, what));
  char buffer[32];
  SetCtrlText(EF_URL, F_URL, PData->URL);
  if (what & IF_Phys)
  { const PHYS_INFO& phys = PData->Info.Phys;
    SetCtrlText(EF_FILESIZE, F_filesize, phys.filesize < 0 ? (const char*)NULL : (sprintf(buffer, "%.3f kiB", phys.filesize/1024.), buffer));
    SetCtrlText(EF_TIMESTAMP, F_timestamp, FormatTstmp(buffer, phys.tstmp));
    SetCtrlText(EF_FATTR, F_fattr, phys.attributes & PATTR_INVALID ? "invalid" : phys.attributes & PATTR_WRITABLE ? "none" : "read-only");
  }
  if (what & IF_Tech)
  { const TECH_INFO& tech = PData->Info.Tech;
    SetCtrlText(EF_SAMPLERATE, F_samplerate, FormatInt(buffer, tech.samplerate, " Hz"));
    SetCtrlText(EF_NUMCHANNELS, F_channels, FormatInt(buffer, tech.channels));
    static const char* objtypes[4] = {"unknown", "Song", "Playlist", "Song, indexed"};
    int type = (tech.attributes & (TATTR_SONG|TATTR_PLAYLIST)) / TATTR_SONG;
    SetCtrlText(EF_OBJTYPE, F_tattr  , tech.attributes & TATTR_INVALID ? "invalid" : objtypes[type]);
    SetCtrlCB3(CB_SAVEABLE, F_tattr  , !!(tech.attributes & TATTR_WRITABLE));
    SetCtrlCB3(CB_SAVESTREAM, F_tattr  , !!(tech.attributes & TATTR_STORABLE));
    SetCtrlText(EF_INFOSTRINGS, F_info, tech.info);
    xstring decoder = tech.decoder;
    if (decoder && decoder.startsWithI(amp_startpath))
      decoder.assign(decoder, amp_startpath.length());
    SetCtrlText(EF_DECODER, F_decoder, decoder);
    SetCtrlText(EF_FORMAT, F_format, tech.format);
  }
  if (what & IF_Obj)
  { const OBJ_INFO& obj = PData->Info.Obj;
    SetCtrlText(EF_BITRATE, F_bitrate, obj.bitrate < 0 ? (const char*)NULL : (sprintf(buffer, "%.1f kbps", obj.bitrate/1000.), buffer));
    SetCtrlText(EF_NUMITEMS, F_num_items, FormatInt(buffer, obj.num_items));
  }
  if (what & IF_Rpl)
  { const RPL_INFO& rpl = PData->Info.Rpl;
    SetCtrlText(EF_SONGITEMS, F_RPL_INFO, FormatInt(buffer, rpl.songs));
    SetCtrlText(EF_LISTITEMS, F_RPL_INFO, FormatInt(buffer, rpl.lists));
  }
  if (what & IF_Drpl)
  { const DRPL_INFO& drpl = PData->Info.Drpl;
    SetCtrlText(EF_TOTALTIME, F_songlength, FormatDuration(buffer, drpl.totallength));
    SetCtrlText(EF_TOTALSIZE, F_DRPL_INFO, drpl.totalsize < 0 ? (const char*)NULL : (sprintf(buffer, "%.3f MiB", drpl.totalsize/(1024.*1024.)), buffer));
  }
}

InfoFlags InfoDialog::PageTechInfo::GetRequestFlags()
{ return IF_Phys|IF_Tech|IF_Obj|IF_Attr|IF_Rpl|IF_Drpl;
}


HWND InfoDialog::PageMetaInfo::SetEFText(ULONG id, Fields fld, const char* text)
{ HWND ctrl = PageBase::SetCtrlText(id, fld, text ? text : "");
  WinSendMsg(ctrl, EM_SETREADONLY, MPFROMSHORT(!MetaWrite), 0);
  return ctrl;
}

MRESULT InfoDialog::PageMetaInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // disable checkboxes in single edit mode
    if (GetParent().GetKey().size() == 1)
    { EnableCtrl(CB_METATITLE,     false);
      EnableCtrl(CB_METAARTIST,    false);
      EnableCtrl(CB_METAALBUM,     false);
      EnableCtrl(CB_METATRACK,     false);
      EnableCtrl(CB_METADATE,      false);
      EnableCtrl(CB_METAGENRE,     false);
      EnableCtrl(CB_METACOMMENT,   false);
      EnableCtrl(CB_METACOPYRIGHT, false);
    }
    break;

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
      RequestUpdate(~IF_None);
      return 0;
     case PB_APPLY:
      { // fetch meta data
        MetaWriteDlg wdlg(GetParent().GetKey());
        wdlg.MetaFlags =
            DECODER_HAVE_TITLE    * QueryGatedTextOrNULL(CB_METATITLE,    EF_METATITLE,    wdlg.MetaData.title    )
          | DECODER_HAVE_ARTIST   * QueryGatedTextOrNULL(CB_METAARTIST,   EF_METAARTIST,   wdlg.MetaData.artist   )
          | DECODER_HAVE_ALBUM    * QueryGatedTextOrNULL(CB_METAALBUM,    EF_METAALBUM,    wdlg.MetaData.album    )
          | DECODER_HAVE_TRACK    * QueryGatedTextOrNULL(CB_METATRACK,    EF_METATRACK,    wdlg.MetaData.track    )
          | DECODER_HAVE_YEAR     * QueryGatedTextOrNULL(CB_METADATE,     EF_METADATE,     wdlg.MetaData.year     )
          | DECODER_HAVE_GENRE    * QueryGatedTextOrNULL(CB_METAGENRE,    EF_METAGENRE,    wdlg.MetaData.genre    )
          | DECODER_HAVE_COMMENT  * QueryGatedTextOrNULL(CB_METACOMMENT,  EF_METACOMMENT,  wdlg.MetaData.comment  )
          | DECODER_HAVE_COPYRIGHT* QueryGatedTextOrNULL(CB_METACOPYRIGHT,EF_METACOPYRIGHT,wdlg.MetaData.copyright);
        // Invoke worker dialog
        wdlg.DoDialog(GetHwnd());
        DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::DlgProc: PB_APPLY - after DoDialog\n", &Parent));
        return 0;
      }
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

void InfoDialog::PageMetaInfo::Update(InfoFlags what)
{ DEBUGLOG(("InfoDialog(%p)::PageMetaInfo::Update(%x)\n", &Parent, what));
  char buffer[32];
  const META_INFO& meta = PData->Info.Meta;
  MetaWrite = (~PData->Enabled & (F_title|F_artist|F_album|F_year|F_comment|F_genre|F_track|F_copyright)) == 0
    && (PData->Info.Tech.attributes & TATTR_WRITABLE) && (PData->Info.Phys.attributes & PATTR_WRITABLE);

  CheckButton(CB_METATITLE,    !(PData->NonUnique & F_title));
  CheckButton(CB_METAARTIST,   !(PData->NonUnique & F_artist));
  CheckButton(CB_METAALBUM,    !(PData->NonUnique & F_album));
  CheckButton(CB_METATRACK,    !(PData->NonUnique & F_track));
  CheckButton(CB_METADATE,     !(PData->NonUnique & F_year));
  CheckButton(CB_METAGENRE,    !(PData->NonUnique & F_genre));
  CheckButton(CB_METACOMMENT,  !(PData->NonUnique & F_comment));
  CheckButton(CB_METACOPYRIGHT,!(PData->NonUnique & F_copyright));

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
}

InfoFlags InfoDialog::PageMetaInfo::GetRequestFlags()
{ return IF_Meta;
}


InfoFlags InfoDialog::PageItemInfo::GetRequestFlags()
{ return IF_Item|IF_Attr;
}

void InfoDialog::PageItemInfo::SetModified()
{ if (Modified)
    return;
  Modified = true;
  EnableCtrl(PB_APPLY, true);
  EnableCtrl(PB_UNDO,  true);
}

const char* InfoDialog::PageItemInfo::FormatGap(char* buffer, float gap)
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

MRESULT InfoDialog::PageItemInfo::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // disable checkboxes in single edit mode
    if (GetParent().GetKey().size() == 1)
    { EnableCtrl(CB_INFOSTART,   false);
      EnableCtrl(CB_INFOSTOP,    false);
      EnableCtrl(CB_INFOAT,      false);
      EnableCtrl(CB_INFOPREGAP,  false);
      EnableCtrl(CB_INFOPOSTGAP, false);
      EnableCtrl(CB_INFOGAIN,    false);
    } else
    { ControlBase(+GetCtrl(EF_INFOURL)).Style(ES_READONLY, ES_READONLY);
      ControlBase(+GetCtrl(EF_INFOALIAS)).Style(ES_READONLY, ES_READONLY);
      ControlBase(+GetCtrl(CB_INFOALTERNATION)).Style(BS_AUTO3STATE, BS_PRIMARYSTYLES);
    }
    break;

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

     case RB_INFOPLSHINHERIT:
     case RB_INFOPLSHFORCE:
     case RB_INFOPLSHCLEAR:
      if (!InUpdate && SHORT2FROMMP(mp1) == BN_CLICKED)
      { if (WinQueryButtonCheckstate(GetHwnd(), SHORT1FROMMP(mp1)) == 1)
        { if (GetParent().GetKey().size() > 1)
            CheckButton(SHORT1FROMMP(mp1), 0);
        } else
        { for (USHORT id = RB_INFOPLSHINHERIT; id <= RB_INFOPLSHCLEAR; ++id)
            CheckButton(id, id == SHORT1FROMMP(mp1));
          SetModified();
        }
      }
      return 0;
     case CB_INFOALTERNATION:
      if (!InUpdate && SHORT2FROMMP(mp1) == BN_CLICKED)
        SetModified();
      return 0;
    }
    break;

   case WM_COMMAND:
    DEBUGLOG(("PlayableInstanceInfoDialog(%p)::PageItemInfo::DlgProc: WM_COMMAND %u\n", &Parent, SHORT1FROMMP(mp1)));
    switch (SHORT1FROMMP(mp1))
    {case PB_UNDO:
      RequestUpdate(~IF_None);
      return 0;

     case PB_APPLY:
      Save();
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

void InfoDialog::PageItemInfo::Update(InfoFlags what)
{ DEBUGLOG(("PlayableInstanceInfoDialog(%p)::PageItemInfo::Update(%x)\n", &Parent, what));
  InUpdate = true; // Disable change notifications
  char buffer[32];
  SetCtrlText(EF_INFOURL, F_URL, PData->URL);
  if (what & IF_Item)
  { const ITEM_INFO& item = PData->Info.Item;
    SetCtrlText(EF_INFOALIAS,   F_alias,  item.alias);
    SetCtrlText(EF_INFOSTART,   F_start,  item.start);
    SetCtrlText(EF_INFOSTOP,    F_stop,   item.stop);
    SetCtrlText(EF_INFOPREGAP,  F_pregap,  FormatGap(buffer, item.pregap));
    SetCtrlText(EF_INFOPOSTGAP, F_postgap, FormatGap(buffer, item.postgap));
    SetCtrlText(EF_INFOGAIN,    F_gainadj, FormatGain(buffer, item.gain));
    CheckButton(CB_INFOSTART,   !(PData->NonUnique & F_start));
    CheckButton(CB_INFOSTOP,    !(PData->NonUnique & F_stop));
    CheckButton(CB_INFOPREGAP,  !(PData->NonUnique & F_pregap));
    CheckButton(CB_INFOPOSTGAP, !(PData->NonUnique & F_postgap));
    CheckButton(CB_INFOGAIN,    !(PData->NonUnique & F_gainadj));
  }
  if (what & IF_Attr)
  { const ATTR_INFO& attr = PData->Info.Attr;
    SetCtrlText(EF_INFOAT,      F_at, attr.at);
    CheckButton(CB_INFOAT,      !(PData->NonUnique & F_at));
    SetCtrlCB(CB_INFOALTERNATION, F_plalt, PData->NonUnique & F_plalt ? 2 : !!(attr.ploptions & PLO_ALTERNATION));
    SetCtrlRB(RB_INFOPLSHINHERIT, F_plshuffle, PData->NonUnique & F_plshuffle ? -1 : (attr.ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE)) / PLO_SHUFFLE);
  }
  EnableCtrl(PB_APPLY, false);
  EnableCtrl(PB_UNDO, false);
  Modified = false;
  InUpdate = false;
}

void InfoDialog::PageItemInfo::Save()
{ DEBUGLOG(("InfoDialog::PageItemInfo::Save()\n"));
  // Extract Parameters and validate
  size_t len;
  url123 url;
  xstring tmp = WinQueryDlgItemXText(GetHwnd(), EF_INFOURL);
  const size_t count = GetParent().GetKey().size();
  bool valid = true;
  ItemInfo item;
  AttrInfo attr;
  Fields write = F_none;

  if (count == 1) // URL and alias only in single mode
  { valid &= SetCtrlEFValid(EF_INFOURL, tmp.length() != 0 && (url = url123::normalizeURL(tmp)) != NULL);
    item.alias = QueryItemTextOrNULL(EF_INFOALIAS);
    write |= F_alias;
  }
  write |=
      F_start * QueryGatedTextOrNULL(CB_INFOSTART, EF_INFOSTART, item.start)
    | F_stop  * QueryGatedTextOrNULL(CB_INFOSTOP,  EF_INFOSTOP,  item.stop)
    | F_at    * QueryGatedTextOrNULL(CB_INFOAT,    EF_INFOAT,    attr.at);
  // Validation of Location makes no sense unless the URL is valid.
  int_ptr<Playable> pp;
  if (count == 1 && valid)
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
  if (QueryGatedTextOrNULL(CB_INFOPREGAP, EF_INFOPREGAP, tmp))
  { valid &= SetCtrlEFValid(EF_INFOPREGAP, tmp.length() == 0
      || (sscanf(tmp, "%f%n", &item.pregap, &len) == 1 && len == tmp.length()) );
    write |= F_pregap;
  }
  if (QueryGatedTextOrNULL(CB_INFOPOSTGAP, EF_INFOPOSTGAP, tmp))
  { valid &= SetCtrlEFValid(EF_INFOPOSTGAP, tmp.length() == 0
      || (sscanf(tmp, "%f%n", &item.postgap, &len) == 1 && len == tmp.length()) );
    write |= F_postgap;
  }
  if (QueryGatedTextOrNULL(CB_INFOGAIN, EF_INFOGAIN, tmp))
  { valid &= SetCtrlEFValid(EF_INFOGAIN, tmp.length() == 0
      || (sscanf(tmp, "%f%n", &item.gain, &len) == 1 && len == tmp.length() && fabs(item.gain) < 100) );
    write |= F_gainadj;
  }
  int alt = CheckBox(GetCtrl(CB_INFOALTERNATION)).CheckState();
  if (alt != 2)
  { attr.ploptions |= PLO_ALTERNATION * alt;
    write |= F_plalt;
  }
  int shuffle = RadioButton(GetCtrl(RB_INFOPLSHINHERIT)).CheckIndex();
  if (shuffle >= 0)
  { attr.ploptions |= PLO_SHUFFLE * shuffle;
    write |= F_plshuffle;
  }

  if (!valid)
    return;

  // Save
  for (unsigned i = 0; i < count; ++i)
  { // This sub dialog is only invoked for objects of type PlayableInstance
    Playable& list = *GetParent().Key.Parent;
    PlayableInstance* pi = (PlayableInstance*)GetParent().GetKey()[i];
    if (pp && &pi->GetPlayable() != pp)
    { // URL has changed. This can only happen for one entry!
      { Mutex::Lock lock(list.Mtx);
        PlayableInstance* pi2 = list.InsertItem(*pp, pi);
        if (pi2 == NULL)
          return;
        list.RemoveItem(*pi);
        pi = pi2;
      }
      // follow with current window.
      // update window key
      KeyType newkey(1);
      newkey.Parent = &list;
      newkey.append() = pi;
      GetParent().ReKey(newkey);
    }
    Mutex::Lock lock(list.Mtx);
    // Keep fields not to write
    { const volatile ITEM_INFO& old_item = *pi->GetInfo().item;
      if (!(write & F_alias))
        item.alias = old_item.alias;
      if (!(write & F_start))
        item.start = old_item.start;
      if (!(write & F_stop))
        item.stop = old_item.stop;
      if (!(write & F_pregap))
        item.pregap = old_item.pregap;
      if (!(write & F_postgap))
        item.postgap = old_item.postgap;
      if (!(write & F_gainadj))
        item.gain = old_item.gain;
    }
    { const volatile ATTR_INFO& old_attr = *pi->GetInfo().attr;
      if (!(write & F_plalt))
        attr.ploptions = (attr.ploptions & ~PLO_ALTERNATION) | (old_attr.ploptions & PLO_ALTERNATION);
      if (!(write & F_plshuffle))
        attr.ploptions = (attr.ploptions & ~(PLO_SHUFFLE|PLO_NO_SHUFFLE)) | (old_attr.ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE));
      if (!(write & F_at))
        attr.at = old_attr.at;
    }
    // TODO: Maybe we should revoke overriding also when the info is the same than on the base Playable.
    pi->OverrideItem(item.IsInitial() ? NULL : &item);
    pi->OverrideAttr(attr.IsInitial() ? NULL : &attr);
  }
}


InfoDialog::InfoDialog(const KeyType& key)
: AInfoDialog(DLG_INFO, NULLHANDLE)
, Key(key)
, DataCacheValid(false)
, ChangeDelegs(key.size())
{ DEBUGLOG(("InfoDialog(%p)::InfoDialog({%u, %s})\n", this, key.size(), debug_dump(key).cdata()));
  // need to initialize array content lately
  foreach (DelegateHelper,*, dp, ChangeDelegs)
    dp->Inst = this;
  Pages.append() = new PageMetaInfo(*this);
  Pages.append() = new PageTechInfo(*this);
  if (Key.Parent) // Playlist items?
    Pages.append() = new PageItemInfo(*this);
}

InfoDialog::~InfoDialog()
{ DEBUGLOG(("InfoDialog(%p)::~InfoDialog()\n", this));
  Repository::RemoveWithKey(*this, Key);
}

void InfoDialog::ReKey(KeyType& key)
{ DEBUGLOG(("InfoDialog(%p)::ReKey({%u, %s})\n", this, key.size(), debug_dump(key).cdata()));
  // Detach events
  unsigned i = Key.size();
  while (i--)
    ChangeDelegs[i].detach();

  { Repository::IXAccess acc;
    XASSERT(Repository::RemoveWithKey(*this, Key), == this);
    Key.swap(key);
    XASSERT(Repository::GetByKey(Key, &InfoDialog::DummyFactory, this), == this);
  }
  DataCacheValid = false;
  SetTitle();

  // Reregister change events
  i = Key.size();
  while (i--)
    Key[i]->GetInfoChange() += ChangeDelegs[i];

  // update content
  for (AInfoDialog::PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
    ((PageBase*)*pp)->RequestUpdate(~IF_None);
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
{ unsigned i;
  switch (msg)
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
    SetTitle();
    // register for change events
    i = Key.size();
    while (i--)
      Key[i]->GetInfoChange() += ChangeDelegs[i];
    break;

   case WM_DESTROY:
    // detach change events
    i = Key.size();
    while (i--)
      ChangeDelegs[i].detach();
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
  }
  return AInfoDialog::DlgProc(msg, mp1, mp2);
}

void InfoDialog::SetTitle()
{ if (Key.size() > 1)
    AInfoDialog::SetTitle(xstring().sprintf("PM123 - %u objects: %s, %s ...", Key.size(),
      Key[0]->GetPlayable().URL.getShortName().cdata(), Key[1]->GetPlayable().URL.getShortName().cdata()));
  else
    AInfoDialog::SetTitle(xstring().sprintf("PM123 Object info: %s", Key[0]->GetPlayable().URL.getDisplayName().cdata()));
}

void InfoDialog::ContentChangeEvent(const PlayableChangeArgs& args)
{ DEBUGLOG(("InfoDialog(%p)::ContentChangeEvent({&%p, %x, %x})\n",
    this, &args.Instance, args.Changed, args.Loaded));
  for (AInfoDialog::PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
  { PageBase* p = (PageBase*)*pp;
    if (p == NULL)
      continue;
    InfoFlags relevant = p->GetRequestFlags();
    InfoFlags upd = args.Changed | args.Loaded;
    if (upd & relevant)
    { DataCacheValid = false;
      p->RequestUpdate(upd);
    }
    // automatically request invalidated infos
    if (args.Invalidated & relevant)
      args.Instance.RequestInfo(args.Invalidated & relevant, PRI_Low);
  }
}

void InfoDialog::LoadData()
{ DEBUGLOG(("InfoDialog(%p)::LoadData() %u - %u %s\n", this, DataCacheValid, Key.size(), debug_dump(Key).cdata()));
  if (DataCacheValid)
    return;
  DataCacheValid = true;

  DataCache.Info.Reset();
  DataCache.Enabled = F_none;
  DataCache.Invalid = F_none;
  DataCache.NonUnique = F_none;

  // merge data
  Merger mg(DataCache);
  foreach (const int_ptr<APlayable>,*, pp, Key)
    mg.Merge(**pp);

  if (Key.size() > 1)
    DataCache.Enabled &= ~(F_URL|F_alias);

  DEBUGLOG(("InfoDialog::GetData() %x:%x %x:%x\n", DataCache.Enabled.U2,DataCache.Enabled.U1, DataCache.Invalid.U2,DataCache.Invalid.U1));
}

bool InfoDialog::IsVisible(PageNo page) const
{ ASSERT((size_t)page < Pages.size());
  PageBase* pp = (PageBase*)Pages[page];
  ASSERT(pp);
  return GetVisible() && NotebookCtrl.CurrentPageID() == pp->GetPageID();
}

void InfoDialog::ShowPage(PageNo page)
{ ASSERT((size_t)page < Pages.size());
  PageBase* pp = (PageBase*)Pages[page];
  ASSERT(pp);
  NotebookCtrl.CurrentPageID(pp->GetPageID());
  SetVisible(true);
}


void InfoDialog::Merger::Merge(APlayable& item)
{ // Work around for missing constexpr
  static const Fields f_ena_none(F_URL|F_PHYS_INFO|F_info|F_decoder);
  static const Fields f_ena_song(F_URL|F_PHYS_INFO|F_TECH_INFO|F_songlength|F_bitrate|F_META_INFO|F_at|F_ITEM_INFO);
  static const Fields f_ena_list(F_URL|F_PHYS_INFO|F_tattr|F_info|F_format|F_decoder|F_songlength|F_num_items|F_ATTR_INFO|F_RPL_INFO|F_DRPL_INFO|F_ITEM_INFO);
  static const Fields f_ena_def (F_URL|F_PHYS_INFO|F_TECH_INFO|F_OBJ_INFO|F_META_INFO|F_ATTR_INFO|F_RPL_INFO|F_DRPL_INFO|F_ITEM_INFO);
  // Fetch Data
  InfoFlags invalid = item.RequestInfo(IF_Decoder|IF_Aggreg, PRI_None);
  const INFO_BUNDLE_CV& info = item.GetInfo();
  switch (info.tech->attributes & (TATTR_SONG|TATTR_PLAYLIST))
  {case TATTR_NONE:
    Enabled = f_ena_none;
    goto def;

   case TATTR_SONG:
    Enabled = f_ena_song;
    goto def;

   case TATTR_PLAYLIST:
    Enabled = f_ena_list;
    Invalid
      = ((invalid & IF_Phys) != 0) * F_PHYS_INFO
      | ((invalid & IF_Tech) != 0) * F_TECH_INFO
      | ((invalid & IF_Obj ) != 0) * (F_OBJ_INFO&~F_songlength)
      | ((invalid & IF_Meta) != 0) * F_META_INFO
      | ((invalid & IF_Attr) != 0) * F_ATTR_INFO
      | ((invalid & IF_Rpl ) != 0) * F_RPL_INFO
      | ((invalid & IF_Drpl) != 0) * (F_DRPL_INFO|F_songlength)
      | ((invalid & IF_Item) != 0) * F_ITEM_INFO ;
    break;

   default: // TATTR_SONG|TATTR_PLAYLIST:
    Enabled = f_ena_def;
   def:
    Invalid
      = ((invalid & IF_Phys) != 0) * F_PHYS_INFO
      | ((invalid & IF_Tech) != 0) * F_TECH_INFO
      | ((invalid & IF_Obj ) != 0) * F_OBJ_INFO
      | ((invalid & IF_Meta) != 0) * F_META_INFO
      | ((invalid & IF_Attr) != 0) * F_ATTR_INFO
      | ((invalid & IF_Rpl ) != 0) * F_RPL_INFO
      | ((invalid & IF_Drpl) != 0) * F_DRPL_INFO
      | ((invalid & IF_Item) != 0) * F_ITEM_INFO ;
    break;
  }

  MergeField(Target.URL, item.GetPlayable().URL, F_URL);
  { volatile const PHYS_INFO& phys = *info.phys;
    MergeField(Target.Info.Phys.filesize,   phys.filesize,   F_filesize);
    MergeField(Target.Info.Phys.tstmp,      phys.tstmp,      F_timestamp);
    MergeField(Target.Info.Phys.attributes, phys.attributes, F_fattr);
  }
  { volatile const TECH_INFO& tech = *info.tech;
    MergeField(Target.Info.Tech.samplerate, tech.samplerate, F_samplerate);
    MergeField(Target.Info.Tech.channels,   tech.channels,   F_channels);
    MergeField(Target.Info.Tech.attributes, tech.attributes, F_tattr);
    MergeField(Target.Info.Tech.info,       tech.info,       F_info);
    MergeField(Target.Info.Tech.format,     tech.format,     F_format);
    MergeField(Target.Info.Tech.decoder,    tech.decoder,    F_decoder);
  }
  { volatile const ATTR_INFO& attr = *info.attr;
    MergeMaskedField(Target.Info.Attr.ploptions, attr.ploptions, F_plalt,     PLO_ALTERNATION);
    MergeMaskedField(Target.Info.Attr.ploptions, attr.ploptions, F_plshuffle, PLO_SHUFFLE|PLO_NO_SHUFFLE);
    MergeField(Target.Info.Attr.at,         attr.at,         F_at);
  }
  { volatile const OBJ_INFO& obj = *info.obj;
    MergeField(Target.Info.Obj.songlength,  obj.songlength,  F_songlength);
    MergeField(Target.Info.Obj.bitrate,     obj.bitrate,     F_bitrate);
    MergeField(Target.Info.Obj.num_items,   obj.num_items,   F_num_items);
  }
  { volatile const META_INFO& meta = *info.meta;
    MergeField(Target.Info.Meta.title,      meta.title,      F_title);
    MergeField(Target.Info.Meta.artist,     meta.artist,     F_artist);
    MergeField(Target.Info.Meta.album,      meta.album,      F_album);
    MergeField(Target.Info.Meta.year,       meta.year,       F_year);
    MergeField(Target.Info.Meta.comment,    meta.comment,    F_comment);
    MergeField(Target.Info.Meta.genre,      meta.genre,      F_genre);
    MergeField(Target.Info.Meta.track,      meta.track,      F_track);
    MergeField(Target.Info.Meta.copyright,  meta.copyright,  F_copyright);
    MergeRVAField(Target.Info.Meta.track_gain, meta.track_gain, F_track_gain);
    MergeRVAField(Target.Info.Meta.track_peak, meta.track_peak, F_track_peak);
    MergeRVAField(Target.Info.Meta.album_gain, meta.album_gain, F_album_gain);
    MergeRVAField(Target.Info.Meta.album_peak, meta.album_peak, F_album_peak);
  }
  { volatile const ITEM_INFO& item = *info.item;
    MergeField(Target.Info.Item.alias,      item.alias,      F_alias);
    MergeField(Target.Info.Item.start,      item.start,      F_start);
    MergeField(Target.Info.Item.stop,       item.stop,       F_stop);
    MergeField(Target.Info.Item.pregap,     item.pregap,     F_pregap);
    MergeField(Target.Info.Item.postgap,    item.postgap,    F_postgap);
    MergeField(Target.Info.Item.gain,       item.gain,       F_gainadj);
  }
  Target.Info.Rpl += *info.rpl;
  Target.Info.Drpl += *info.drpl;

  AggEnabled |= Enabled;
  AggInvalid &= Invalid;

  Target.Enabled |= Enabled;
  Target.Invalid |= Invalid;
}

// has Disabled value = 0
// has Invalid value = 1
// has Valid value = 2
//   D  I  V
// D D< I^ V^
// I I< I< V^
// V V< V< V*
void InfoDialog::Merger::MergeField(xstring& dst, const volatile xstring& src, Fields fld)
{
  int srcrel = !!(Enabled & fld) * (!(Invalid & fld) + 1);
  int dstrel = !!(AggEnabled & fld) * (!(AggInvalid & fld) + 1);
  if (srcrel > dstrel)
  { dst = src;
    if (dstrel == 0)
      Target.NonUnique &= ~fld;
  } else if ((srcrel > 0 || dstrel == 0) && dst != xstring(src))
  { Target.NonUnique |= fld;
    if (dstrel == 2)
      dst.reset();
  }
}
void InfoDialog::Merger::MergeField(double& dst, double src, Fields fld)
{
  int srcrel = !!(Enabled & fld) * (!(Invalid & fld) + 1);
  int dstrel = !!(AggEnabled & fld) * (!(AggInvalid & fld) + 1);
  if (srcrel > dstrel)
  { dst = src;
    if (dstrel == 0)
      Target.NonUnique &= ~fld;
  } else if ((srcrel > 0 || dstrel == 0) && dst != src)
  { Target.NonUnique |= fld;
    if (dstrel == 2)
      dst = -1;
  }
}
void InfoDialog::Merger::MergeField(int& dst, int src, Fields fld)
{
  int srcrel = !!(Enabled & fld) * (!(Invalid & fld) + 1);
  int dstrel = !!(AggEnabled & fld) * (!(AggInvalid & fld) + 1);
  if (srcrel > dstrel)
  { dst = src;
    if (dstrel == 0)
      Target.NonUnique &= ~fld;
  } else if ((srcrel > 0 || dstrel == 0) && dst != src)
  { Target.NonUnique |= fld;
    if (dstrel == 2)
      dst = -1;
  }
}
void InfoDialog::Merger::MergeField(unsigned& dst, unsigned src, Fields fld)
{
  int srcrel = !!(Enabled & fld) * (!(Invalid & fld) + 1);
  int dstrel = !!(AggEnabled & fld) * (!(AggInvalid & fld) + 1);
  if (srcrel > dstrel)
  { dst = src;
    if (dstrel == 0)
      Target.NonUnique &= ~fld;
  } else if ((srcrel > 0 || dstrel == 0) && dst != src)
  { Target.NonUnique |= fld;
    if (dstrel == 2)
      dst = 0;
  }
}
void InfoDialog::Merger::MergeMaskedField(unsigned& dst, unsigned src, Fields fld, unsigned mask)
{
  int srcrel = !!(Enabled & fld) * (!(Invalid & fld) + 1);
  int dstrel = !!(AggEnabled & fld) * (!(AggInvalid & fld) + 1);
  if (srcrel > dstrel)
  { dst = (dst & ~mask) | (src & mask);
    if (dstrel == 0)
      Target.NonUnique &= ~fld;
  } else if ((srcrel > 0 || dstrel == 0) && (dst & mask) != (src & mask))
  { Target.NonUnique |= fld;
    if (dstrel == 2)
      dst &= ~mask;
  }
}
void InfoDialog::Merger::MergeField(float& dst, float src, Fields fld)
{
  int srcrel = !!(Enabled & fld) * (!(Invalid & fld) + 1);
  int dstrel = !!(AggEnabled & fld) * (!(AggInvalid & fld) + 1);
  if (srcrel > dstrel)
  { dst = src;
    if (dstrel == 0)
      Target.NonUnique &= ~fld;
  } else if ((srcrel > 0 || dstrel == 0) && dst != src)
  { Target.NonUnique |= fld;
    if (dstrel == 2)
      dst = 0;
  }
}
void InfoDialog::Merger::MergeRVAField(float& dst, float src, Fields fld)
{
  int srcrel = !!(Enabled & fld) * (!(Invalid & fld) + 1);
  int dstrel = !!(AggEnabled & fld) * (!(AggInvalid & fld) + 1);
  if (srcrel > dstrel)
  { dst = src;
    if (dstrel == 0)
      Target.NonUnique &= ~fld;
  } else if ((srcrel > 0 || dstrel == 0) && dst != src)
  { Target.NonUnique |= fld;
    if (dstrel == 2)
      dst = -1000;
  }
}

