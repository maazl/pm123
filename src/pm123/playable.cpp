/*
 * Copyright 2007-2007 M.Mueller
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


#define INCL_PM
#define INCL_BASE
#include "plugman.h"
#include "playable.h"
#include "pm123.h"
#include "copyright.h"
#include <xio.h>
#include <stdio.h>
#include <stddef.h>

#include <debuglog.h>

#ifdef DEBUG
#define RP_INITIAL_SIZE 5 // Errors come sooner...
#else
#define RP_INITIAL_SIZE 100
#endif


/****************************************************************************
*
*  class Playable
*
****************************************************************************/

const Playable::DecoderInfo Playable::DecoderInfo::InitialInfo;

void Playable::DecoderInfo::Init()
{ Format.size   = sizeof Format;
  TechInfo.size = sizeof TechInfo;
  MetaInfo.size = sizeof MetaInfo;
  size          = sizeof(DECODER_INFO2);
  format        = &Format;
  tech          = &TechInfo;
  meta          = &MetaInfo;
}

void Playable::DecoderInfo::Reset()
{ memset(&TechInfo, 0, sizeof TechInfo);
  memset(&MetaInfo, 0, sizeof MetaInfo);
  memset((DECODER_INFO2*)this, 0, sizeof(DECODER_INFO2));
  Init();
  Format  .samplerate = -1;
  Format  .channels   = -1;
  TechInfo.songlength = -1;
  TechInfo.bitrate    = -1;
  TechInfo.filesize   = -1;
  TechInfo.num_items  = -1;
  MetaInfo.track      = -1;
}

void Playable::DecoderInfo::operator=(const DECODER_INFO2& r)
{ Reset();
  // version stable copy
  if (r.format->size < sizeof Format)
    memcpy(&Format, r.format, r.format->size);
   else
    Format = *r.format;
  if (r.tech->size < sizeof TechInfo)
    memcpy(&TechInfo, r.tech, r.tech->size);
   else
    TechInfo = *r.tech;
  if (r.meta->size < sizeof MetaInfo)
    memcpy(&MetaInfo, r.meta, r.meta->size);
   else
    MetaInfo = *r.meta;
  memcpy((DECODER_INFO2*)this, &r, sizeof(DECODER_INFO2));
  Init();
}


Playable::Playable(const url123& url, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: URL(url),
  Stat(STA_Unknown),
  InfoValid(IF_None),
  InfoChangeFlags(IF_None),
  InfoRequest(IF_None)
{ DEBUGLOG(("Playable(%p)::Playable(%s, %p, %p, %p)\n", this, url.cdata(), ca_format, ca_tech, ca_meta));
  Info.Reset();

  if (ca_format)
  { *Info.format = *ca_format;
    InfoValid |= IF_Format;
  }
  if (ca_tech)
  { *Info.tech = *ca_tech;
    InfoValid |= IF_Tech;
  }
  if (ca_meta)
  { *Info.meta = *ca_meta;
    InfoValid |= IF_Meta;
  }
  Decoder[0] = 0;
}

Playable::~Playable()
{ DEBUGLOG(("Playable(%p{%s})::~Playable()\n", this, URL.cdata()));
  // Deregister from repository automatically
  { Mutex::Lock lock(RPMutex);
    XASSERT(RPInst.erase(URL), != NULL);
  }
}

/* Returns true if the specified file is a playlist file. */
bool Playable::IsPlaylist(const url123& URL)
{ const xstring& ext = URL.getExtension();
  DEBUGLOG(("Playable::IsPlaylist(%s) - %s\n", URL.cdata(), ext.cdata()));
  // TODO: this is a very bad criterion
  return ext.compareToI(".lst", 4) == 0 ||
         ext.compareToI(".mpl", 4) == 0 ||
         ext.compareToI(".pls", 4) == 0 ||
         ext.compareToI(".m3u", 4) == 0;
}

Playable::Flags Playable::GetFlags() const
{ return None;
}

void Playable::SetInUse(bool used)
{ DEBUGLOG(("Playable(%p{%s})::SetInUse(%u)\n", this, URL.cdata(), used));
  Mutex::Lock lock(Mtx);
  if (Stat <= STA_Invalid)
    return; // can't help
  UpdateStatus(used ? STA_Used : STA_Normal);
}

xstring Playable::GetDisplayName() const
{ //DEBUGLOG(("Playable(%p{%s})::GetDisplayName()\n", this, URL.cdata()));
  const META_INFO& meta = *GetInfo().meta;
  if (meta.title[0])
    return meta.title;
  else
    return URL.getShortName();
}

void Playable::UpdateInfo(const FORMAT_INFO2* info)
{ DEBUGLOG(("Playable::UpdateInfo(FORMAT_INFO2* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.format;
  InfoValid |= IF_Format;
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(Info.format, info, sizeof *Info.format) * IF_Format);
  DEBUGLOG(("Playable::UpdateInfo: %x %u {,%u,%u}\n", InfoValid, InfoChangeFlags, Info.format->samplerate, Info.format->channels));
}
void Playable::UpdateInfo(const TECH_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(TECH_INFO* %p) - %p\n", info, Info.tech));
  if (!info)
    info = DecoderInfo::InitialInfo.tech;
  InfoValid |= IF_Tech;
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(Info.tech, info, sizeof *Info.tech) * IF_Tech);
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const META_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(META_INFO* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.meta;
  InfoValid |= IF_Meta;
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(Info.meta, info, sizeof *Info.meta) * IF_Meta);
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const DECODER_INFO2* info)
{ DEBUGLOG(("Playable::UpdateInfo(DECODER_INFO2* %p)\n", info));
  if (!info)
    info = &DecoderInfo::InitialInfo;
  InfoValid |= IF_Other;
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(&Info.meta_write, &info->meta_write, sizeof(DECODER_INFO2) - offsetof(DECODER_INFO2, meta_write)) * IF_Other);
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
  UpdateInfo(info->format);
  UpdateInfo(info->tech);
  UpdateInfo(info->meta);
}

void Playable::UpdateStatus(PlayableStatus stat)
{ DEBUGLOG(("Playable::UpdateStatus(%u) - %u\n", stat, Stat));
  InfoValid |= IF_Status;
  if (Stat != stat)
  { Stat = stat;
    InfoChangeFlags |= IF_Status;
  }
}

void Playable::SetMetaInfo(const META_INFO* meta)
{ Mutex::Lock lock(Mtx);
  UpdateInfo(meta);
  RaiseInfoChange();
}

void Playable::SetTechInfo(const TECH_INFO* tech)
{ Mutex::Lock lock(Mtx);
  UpdateInfo(tech);
  RaiseInfoChange();
}

void Playable::LoadInfoAsync(InfoFlags what)
{ DEBUGLOG(("Playable(%p{%s})::LoadInfoAsync(%x) - %x\n", this, GetURL().getShortName().cdata(), what, InfoValid));
  if (what == 0)
    return;
  // schedule request
  InfoFlags oldrq = (InfoFlags)InfoRequest; // This read is non-atomic. But on the worst case an empty request is scheduled to the worker thread.
  InterlockedOr(InfoRequest, what);
  // Only write the Queue if ther was no request before.
  if (!oldrq)
    WQueue.Write(this);
}

Playable::InfoFlags Playable::EnsureInfoAsync(InfoFlags what)
{ InfoFlags avail = what & InfoValid;
  if ((what &= ~avail) != 0)
    // schedule request
    LoadInfoAsync(what);
  return avail;
}

/* Worker Queue */
queue<Playable::QEntry> Playable::WQueue;
int Playable::WTid = -1;
bool Playable::WTermRq = false;

static void PlayableWorker(void*)
{ for (;;)
  { DEBUGLOG(("PlayableWorker() looking for work\n"));
    queue<Playable::QEntry>::Reader rdr(Playable::WQueue);
    Playable::QEntry& qp = rdr;
    DEBUGLOG(("PlayableWorker received message %p\n", &*qp));

    if (Playable::WTermRq || !qp) // stop
      break;

    // Do the work
    if (qp->InfoRequest)
      qp->LoadInfo((Playable::InfoFlags)qp->InfoRequest);
  }
}

void Playable::Init()
{ DEBUGLOG(("Playable::Init()\n"));
  // start the worker
  ASSERT(WTid == -1);
  WTermRq = false;
  WTid = _beginthread(&PlayableWorker, NULL, 65536, NULL);
  ASSERT(WTid != -1);
}

void Playable::Uninit()
{ DEBUGLOG(("Playable::Uninit()\n"));
  WTermRq = true;
  WQueue.Write(NULL); // deadly pill
  if (WTid != -1)
    wait_thread_pm(amp_player_hab(), WTid, 60000);
  DEBUGLOG(("Playable::Uninit - complete\n"));
}


/* URL repository */
sorted_vector<Playable, const char*> Playable::RPInst(RP_INITIAL_SIZE);
Mutex Playable::RPMutex;

int Playable::compareTo(const char*const& str) const
{ DEBUGLOG(("Playable(%p{%s})::compareTo(%s)\n", this, URL.cdata(), str));
  return stricmp(URL, str);
}

#ifdef DEBUG
void Playable::RPDebugDump()
{ for (Playable*const* ppp = RPInst.begin(); ppp != RPInst.end(); ++ppp)
    DEBUGLOG(("Playable::RPDump: %p{%s}\n", *ppp, (*ppp)->GetURL().cdata()));
}
#endif

int_ptr<Playable> Playable::FindByURL(const char* url)
{ DEBUGLOG(("Playable::FindByURL(%s)\n", url));
  Mutex::Lock lock(RPMutex);
  Playable* pp = RPInst.find(url);
  CritSect cs;
  return pp && !pp->RefCountIsUnmanaged() ? pp : NULL;
}

int_ptr<Playable> Playable::GetByURL(const url123& URL, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
{ DEBUGLOG(("Playable::GetByURL(%s)\n", URL.cdata()));
  Mutex::Lock lock(RPMutex);
  #ifdef DEBUG
  //RPDebugDump();
  #endif
  Playable*& pp = RPInst.get(URL);
  { CritSect cs;
    if (pp && !pp->RefCountIsUnmanaged())
      return pp;
  }
  // factory
  if (URL.isScheme("file:") && URL.getObjectName().length() == 0)
    pp = new PlayFolder(URL, ca_tech, ca_meta);
   else if (IsPlaylist(URL))
    pp = new Playlist(URL, ca_tech, ca_meta);
   else // Song
    pp = new Song(URL, ca_format, ca_tech, ca_meta);
  DEBUGLOG(("Playable::GetByURL: factory &%p{%p}\n", &pp, pp));
  return pp;
}


/****************************************************************************
*
*  class PlayableSet
*
****************************************************************************/

const PlayableSet PlayableSet::Empty;

PlayableSet::PlayableSet()
: sorted_vector<Playable, Playable>(8)
{}

int PlayableSet::compareTo(const PlayableSet& r) const
{ Playable*const* ppp1 = begin();
  Playable*const* ppp2 = r.begin();
  for (;;)
  { // termination condition
    if (ppp2 == r.end())
      return ppp1 != end();
    else if (ppp1 == end())
      return -1;
    // compare content
    int ret = (*ppp1)->compareTo(**ppp2);
    if (ret)
      return ret;
    ++ppp1;
    ++ppp2;
  }
}

#ifdef DEBUG
xstring PlayableSet::DebugDump() const
{ xstring r = xstring::empty;
  for (Playable*const* ppp = begin(); ppp != end(); ++ppp)
    if (r.length())
      r = xstring::sprintf("%s, %p", r.cdata(), *ppp);
    else
      r = xstring::sprintf("%p", *ppp);
  return r;
}
#endif


/****************************************************************************
*
*  class PlayableInstance
*
****************************************************************************/

PlayableInstance::slice PlayableInstance::slice::Initial;


PlayableInstance::PlayableInstance(PlayableCollection& parent, Playable* playable)
: Parent(&parent),
  RefTo(playable),
  Stat(STA_Normal)
{ DEBUGLOG(("PlayableInstance(%p)::PlayableInstance(%p{%s}, %p{%s})\n", this,
    &parent, parent.GetURL().getShortName().cdata(), &*playable, playable->GetURL().getShortName().cdata()));
}

void PlayableInstance::SetInUse(bool used)
{ DEBUGLOG(("PlayableInstance(%p)::SetInUse(%u) - %u\n", this, used, Stat));
  PlayableStatus old_stat = Stat;
  Stat = used ? STA_Used : STA_Normal;
  if (Stat != old_stat)
  { RefTo->SetInUse(used);
    StatusChange(change_args(*this, SF_InUse));
  }
}

xstring PlayableInstance::GetDisplayName() const
{ DEBUGLOG2(("PlayableInstance(%p)::GetDisplayName()\n", this));
  return Alias ? Alias : RefTo->GetDisplayName();
}

void PlayableInstance::SetAlias(const xstring& alias)
{ DEBUGLOG(("PlayableInstance(%p)::SetAlias(%p{%s})\n", this, alias.cdata(), alias ? alias.cdata() : ""));
  if (Alias != alias)
  { Alias = alias;
    StatusChange(change_args(*this, SF_Alias));
  }
}

void PlayableInstance::SetSlice(const slice& sl)
{ DEBUGLOG(("PlayableInstance(%p)::SetSlice({%g,%g})\n", this, sl.Start, sl.Stop));
  if (Slice.Start != sl.Start || Slice.Stop != sl.Stop)
  { Slice = sl;
    StatusChange(change_args(*this, SF_Slice));
  }
}

bool operator==(const PlayableInstance& l, const PlayableInstance& r)
{ return l.Parent == r.Parent
      && l.RefTo  == r.RefTo  // Instance equality is sufficient in case of the Playable class.
      && l.Alias  == r.Alias
      && l.Slice  == r.Slice;
}


/****************************************************************************
*
*  class Song
*
****************************************************************************/

Playable::InfoFlags Song::LoadInfo(InfoFlags what)
{ DEBUGLOG(("Song(%p)::LoadInfo() - %s\n", this, Decoder));
  // get information
  sco_ptr<DecoderInfo> info(new DecoderInfo()); // a bit much for the stack
  Mutex::Lock lock(Mtx);
  int rc = dec_fileinfo(GetURL(), &*info, Decoder);
  PlayableStatus stat;
  DEBUGLOG(("Song::LoadInfo - rc = %i\n", rc));
  // update information
  if (rc != 0)
  { stat = STA_Invalid;
    info = NULL; // free structure
  } else
    stat = Stat == STA_Used ? STA_Used : STA_Normal;
  UpdateStatus(stat);
  UpdateInfo(&*info);
  InfoValid |= IF_All;
  InfoRequest = 0;
  RaiseInfoChange();
  return IF_All;
}


/****************************************************************************
*
*  class PlayableCollection
*
****************************************************************************/

void PlayableCollection::CollectionInfo::Add(double songlength, double filesize, int items)
{ DEBUGLOG(("PlayableCollection::CollectionInfo::Add(%g, %g, %i) - %g, %g, %i\n", songlength, filesize, items, Songlength, Filesize, Items));
  // cummulate playing time
  if (Songlength >= 0)
  { if (songlength >= 0)
      Songlength += songlength;
     else
      Songlength = -1;
  }
  // cummulate file size
  if (Filesize >= 0)
  { if (filesize >= 0)
      Filesize += filesize;
     else
      Filesize = -1;
  }
  // Number of items
  if (Items >= 0)
  { if (items >= 0)
      Items += items;
     else
      Items = -1;
  }
}

void PlayableCollection::CollectionInfo::Reset()
{ Songlength = 0;
  Filesize   = 0;
  Items      = 0;
  Excluded   = 0;
}

const FORMAT_INFO2 PlayableCollection::no_format =
{ sizeof(FORMAT_INFO2),
  -1,
  -1
};

PlayableCollection::PlayableCollection(const url123& URL, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: Playable(URL, &no_format, ca_tech, ca_meta),
  Head(NULL),
  Tail(NULL),
  CollectionInfoCache(16)
{ DEBUGLOG(("PlayableCollection(%p)::PlayableCollection(%s, %p, %p)\n", this, URL.cdata(), ca_tech, ca_meta));
}

PlayableCollection::~PlayableCollection()
{ DEBUGLOG(("PlayableCollection(%p{%s})::~PlayableCollection()\n", this, GetURL().getShortName().cdata()));
  // frist disable all events
  CollectionChange.sync_reset();
  InfoChange.sync_reset();
  // Cleanup SubInfoCache
  for (CollectionInfoEntry** ppci = CollectionInfoCache.begin(); ppci != CollectionInfoCache.end(); ++ppci)
    delete *ppci;
}

Playable::Flags PlayableCollection::GetFlags() const
{ return Enumerable;
}

bool PlayableCollection::IsModified() const
{ return false;
}

PlayableCollection::Entry* PlayableCollection::CreateEntry(const char* url, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
{ DEBUGLOG(("PlayableCollection(%p{%s})::CreateEntry(%s)\n", this, GetURL().getShortName().cdata(), url));
  // now create the entry with absolute path
  int_ptr<Playable> pp = GetByURL(GetURL().makeAbsolute(url), ca_format, ca_tech, ca_meta);
  // initiate prefetch of information
  pp->EnsureInfoAsync(IF_Status);
  return new Entry(*this, pp, &PlayableCollection::ChildInfoChange, &PlayableCollection::ChildInstChange);
}

void PlayableCollection::AppendEntry(Entry* entry)
{ DEBUGLOG(("PlayableCollection(%p{%s})::AppendEntry(%p{%s,%p,%p})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), &*entry->Prev, &*entry->Next));
  if ((entry->Prev = Tail) != NULL)
    Tail->Next = entry;
   else
    Head = entry;
  Tail = entry;
  DEBUGLOG(("PlayableCollection::AppendEntry - before event\n"));
  CollectionChange(change_args(*this, *entry, Insert));
  DEBUGLOG(("PlayableCollection::AppendEntry - after event\n"));
}

void PlayableCollection::InsertEntry(Entry* entry, Entry* before)
{ DEBUGLOG(("PlayableCollection(%p{%s})::InsertEntry(%p{%s,%p,%p}, %p{%s})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), &*entry->Prev, &*entry->Next, before, before->GetPlayable()->GetURL().getShortName().cdata()));
  entry->Next = before;
  if ((entry->Prev = before->Prev) != NULL)
    entry->Prev->Next = entry;
   else
    Head = entry;
  before->Prev = entry;
  DEBUGLOG(("PlayableCollection::InsertEntry - before event\n"));
  CollectionChange(change_args(*this, *entry, Insert));
  DEBUGLOG(("PlayableCollection::InsertEntry - after event\n"));
}

bool PlayableCollection::MoveEntry(Entry* entry, Entry* before)
{ DEBUGLOG(("PlayableCollection(%p{%s})::MoveEntry(%p{%s,%p,%p}, %p{%s})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), &*entry->Prev, &*entry->Next, before, before ? before->GetPlayable()->GetURL().getShortName().cdata() : ""));
  if (entry == before || entry->Next == before)
    return false; // No-op
  // remove at old location
  if (entry->Prev)
    entry->Prev->Next = entry->Next;
   else
    Head = entry->Next;
  if (entry->Next)
    entry->Next->Prev = entry->Prev;
   else
    Tail = entry->Prev;
  // insert at new location
  entry->Next = before;
  int_ptr<Entry>& next = before ? before->Prev : Tail;
  if ((entry->Prev = next) != NULL)
    next->Next = entry;
   else
    Head = entry;
  next = entry;
  // raise event
  DEBUGLOG(("PlayableCollection::MoveEntry - before event\n"));
  CollectionChange(change_args(*this, *entry, Move));
  DEBUGLOG(("PlayableCollection::MoveEntry - after event\n"));
  return true;
}

void PlayableCollection::RemoveEntry(Entry* entry)
{ DEBUGLOG(("PlayableCollection(%p{%s})::RemoveEntry(%p{%s,%p,%p})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), &*entry->Prev, &*entry->Next));
  CollectionChange(change_args(*this, *entry, Delete));
  DEBUGLOG(("PlayableCollection::RemoveEntry - after event\n"));
  ASSERT(entry->IsParent(this));
  entry->Detach();
  if (entry->Prev)
    entry->Prev->Next = entry->Next;
   else
    Head = entry->Next;
  if (entry->Next)
    entry->Next->Prev = entry->Prev;
   else
    Tail = entry->Prev;
}

void PlayableCollection::ChildInfoChange(const Playable::change_args& args)
{ DEBUGLOG(("PlayableCollection(%p{%s})::ChildInfoChange({{%s},%x})\n", this, GetURL().getShortName().cdata(),
    args.Instance.GetURL().getShortName().cdata(), args.Flags));
  if (args.Flags & IF_Tech)
  { // Update dependant tech info, but only if already available
    if (InfoValid & IF_Tech)
      LoadInfoAsync(IF_Tech);
    // Invalidate CollectionInfoCache entries.
    CritSect cs;
    for (CollectionInfoEntry** ciepp = CollectionInfoCache.begin(); ciepp != CollectionInfoCache.end(); ++ciepp)
      // Only items that do not have the sender in the exclusion list are invalidated.
      if ((*ciepp)->find(args.Instance) == NULL)
        (*ciepp)->Valid = false;
  }
}

void PlayableCollection::ChildInstChange(const PlayableInstance::change_args& args)
{ DEBUGLOG(("PlayableCollection(%p{%s})::ChildInstChange({{%s},%x})\n", this, GetURL().getShortName().cdata(),
    args.Instance.GetPlayable()->GetURL().getShortName().cdata(), args.Flags));
  if (args.Flags & PlayableInstance::SF_Slice)
    // Update dependant tech info, but only if already available
    // Same as TechInfoChange => emulate another event
    ChildInfoChange(Playable::change_args(*args.Instance.GetPlayable(), IF_Tech));
}

void PlayableCollection::CalcTechInfo(TECH_INFO& dst)  
{ DEBUGLOG(("PlayableCollection(%p{%s})::CalcTechInfo(%p{}) - %x\n", this, GetURL().getShortName().cdata(), &dst, InfoValid));

  if (Stat == STA_Invalid)
    strcpy(dst.info, "invalid");
  else
  { // generate Info
    const CollectionInfo& ci = GetCollectionInfo();
    dst.songlength = ci.Songlength;
    dst.filesize   = ci.Filesize;
    dst.num_items  = ci.Items;
    dst.recursive  = ci.Excluded;
    
    dst.bitrate = (int)( dst.filesize >= 0 && dst.songlength > 0
      ? dst.filesize / dst.songlength / (1000/8)
      : -1 );
    DEBUGLOG(("PlayableCollection::CalcTechInfo(): %s {%g, %i, %g}\n", GetURL().getShortName().cdata(), dst.songlength, dst.bitrate, dst.filesize));
    // Info string
    // Since all strings are short, there may be no buffer overrun.
    if (dst.songlength < 0)
      strcpy(dst.info, "unknown length");
     else
    { int days = (int)(dst.songlength / 86400.);
      double rem = dst.songlength - 86400.*days;
      int hours = (int)(rem / 3600.);
      rem -= 3600*hours;
      int minutes = (int)(rem / 60);
      rem -= 60*minutes;
      if (days)
        sprintf(dst.info, "Total: %ud %u:%02u:%02u", days, hours, minutes, (int)rem);
       else
        sprintf(dst.info, "Total: %u:%02u:%02u", hours, minutes, (int)rem);
    }
    if (dst.recursive)
      strcat(dst.info, ", recursion");
  }
}

int_ptr<PlayableInstance> PlayableCollection::GetPrev(PlayableInstance* cur) const
{ DEBUGLOG(("PlayableCollection(%p)::GetPrev(%p{%s})\n", this, cur, cur ? cur->GetPlayable()->GetURL().cdata() : ""));
  ASSERT(InfoValid & IF_Other);
  // The PlayableInstance (if any) may either belong the the current list or be removed earlier.
  ASSERT(cur == NULL || cur->IsParent(NULL) || cur->IsParent(this));
  return (PlayableInstance*)(cur ? ((Entry*)cur)->Prev : Tail);
}

int_ptr<PlayableInstance> PlayableCollection::GetNext(PlayableInstance* cur) const
{ DEBUGLOG(("PlayableCollection(%p)::GetNext(%p{%s})\n", this, cur, cur ? cur->GetPlayable()->GetURL().cdata() : ""));
  ASSERT(InfoValid & IF_Other);
  // The PlayableInstance (if any) may either belong the the current list or be removed earlier.
  ASSERT(cur == NULL || cur->IsParent(NULL) || cur->IsParent(this));
  return (PlayableInstance*)(cur ? ((Entry*)cur)->Next : Head);
}

const PlayableCollection::CollectionInfo& PlayableCollection::GetCollectionInfo(const PlayableSet& excluding)
{ DEBUGLOG(("PlayableCollection(%p{%s})::GetCollectionInfo({%i, %s}) - %x\n", this, GetURL().getShortName().cdata(), excluding.size(), excluding.DebugDump().cdata()));
  ASSERT(Mtx.GetStatus() == Mutex::Mine);
  
  // cache lookup
  // We have to protect the CollectionInfoCache additionally by a critical section,
  // because the TechInfoChange event from the children cannot aquire the mutex.
  CritSect cs;
  CollectionInfoEntry*& cic = CollectionInfoCache.get(excluding);
  if (cic == NULL)
    cic = new CollectionInfoEntry();
  else if (cic->Valid)
    return cic->Info;
  else
    cic->Info.Reset();
  cs.~CritSect(); // Hmm, dirty but working.

  // (re)create information
  EnsureInfo(IF_Other);
  sco_ptr<PlayableSet> xcl_sub;
  PlayableInstance* pi = NULL;
  while ((pi = GetNext(pi)) != NULL)
  { DEBUGLOG(("PlayableCollection::GetCollectionInfo Item %p{%p{%s}}\n", pi, pi->GetPlayable(), pi->GetPlayable()->GetURL().cdata()));
    if (pi->GetPlayable()->GetFlags() & Playable::Enumerable)
    { // nested playlist
      PlayableCollection* pc = (PlayableCollection*)pi->GetPlayable();
      // check for recursion
      if (excluding.find(*pc) != NULL)
      { // recursion
        DEBUGLOG(("PlayableCollection::GetCollectionInfo - recursive!\n"));
        cic->Info.Excluded = true;
        continue;
      }
      DEBUGLOG(("PlayableCollection::GetCollectionInfo - enter\n"));
      // create new exclusions on demand
      if (xcl_sub == NULL)
      { xcl_sub = new PlayableSet(excluding); // deep copy
        xcl_sub->get(*this) = this;
      }
      // get sub info
      Mutex::Lock lck(pc->Mtx);
      cic->Info.Add(pc->GetCollectionInfo(*xcl_sub));

    } else
    { // Song
      DEBUGLOG(("PlayableCollection::GetCollectionInfo - Song\n"));
      Song* song = (Song*)pi->GetPlayable();
      song->EnsureInfo(IF_Tech);
      if (song->GetStatus() > STA_Invalid)
      { const TECH_INFO& tech = *song->GetInfo().tech;
        // take care of slice
        T_TIME length = tech.songlength;
        if (pi->GetSlice().Stop >= 0 && length > pi->GetSlice().Stop)
          length = pi->GetSlice().Stop;
        if (pi->GetSlice().Start >= length)
          length = 0;
        else
          length -= pi->GetSlice().Start;
        // Scale filesize with slice to preserve bitrate. 
        cic->Info.Add(length, tech.filesize * length / tech.songlength, 1);
      } else
        // only count invalid items
        cic->Info.Add(0, 0, 1);
    }
  }
  return cic->Info;
}

Playable::InfoFlags PlayableCollection::LoadInfo(InfoFlags what)
{ DEBUGLOG(("PlayableCollection(%p{%s})::LoadInfo(%x) - %x %u\n", this, GetURL().getShortName().cdata(), what, InfoValid, Stat));
  Mutex::Lock lock(Mtx);

  // Need list content to generate tech info
  if ((what & IF_Tech) && !(InfoValid & IF_Other))
    what |= IF_Other;

  if (what & (IF_Other|IF_Status))
  { // load playlist content
    PlayableStatus stat;
    if (LoadList())
      stat = Stat == STA_Used ? STA_Used : STA_Normal;
    else
      stat = STA_Invalid;
    UpdateStatus(stat);
    InfoValid |= IF_Other;
    what |= IF_Other|IF_Status;
  }

  if (what & IF_Tech)
  { // update tech info
    TECH_INFO tech;
    CalcTechInfo(tech);
    UpdateInfo(&tech);
  }
  DEBUGLOG(("PlayableCollection::LoadInfo completed - %x %x\n", InfoValid, InfoChangeFlags));
  // default implementation to fullfill the minimum requirements of LoadInfo.
  InfoValid |= what;
  InterlockedAnd(InfoRequest, ~what);
  // raise InfoChange event?
  RaiseInfoChange();
  return what;
}

bool PlayableCollection::SaveLST(XFILE* of, bool relative)
{ DEBUGLOG(("PlayableCollection(%p{%s})::SaveLST(%p, %u)\n", this, GetURL().cdata(), of, relative));
  // Header
  xio_fputs("#\n"
            "# Playlist created with " AMP_FULLNAME "\n"
            "# Do not modify!\n"
            "# Lines starting with '>' are used for optimization.\n"
            "#\n", of);
  // Content
  Mutex::Lock lock(Mtx);
  int_ptr<PlayableInstance> pi;
  while ((pi = GetNext(pi)) != NULL)
  { Playable* pp = pi->GetPlayable();
    // alias
    if (pi->GetAlias())
    { xio_fputs("#ALIAS ", of);
      xio_fputs(pi->GetAlias(), of);
      xio_fputs("\n", of);
    }
    // slice
    if (pi->GetSlice().Start != PlayableInstance::slice::Initial.Start || pi->GetSlice().Stop != PlayableInstance::slice::Initial.Stop)
    { char buf[50];
      sprintf(buf, "#SLICE %.3f,%.3f\n", pi->GetSlice().Start, pi->GetSlice().Stop);
      xio_fputs(buf, of);
    }
    // comment
    const TECH_INFO& tech = *pp->GetInfo().tech;
    if (pp->GetStatus() > STA_Invalid && !pp->CheckInfo(IF_Tech)) // only if immediately available
    { char buf[50];
      xio_fputs("# ", of);
      if (tech.filesize < 0)
        xio_fputs("n/a", of);
      else
      { sprintf(buf, "%.0lfkiB", tech.filesize/1024.);
        xio_fputs(buf, of);
      }
      xio_fputs(", ", of);
      if (tech.songlength < 0)
        xio_fputs("n/a", of);
      else
      { unsigned long s = (unsigned long)tech.songlength;
        if (s < 60)
          sprintf(buf, "%lu s", s);
        else if (s < 3600)
          sprintf(buf, "%lu:%02lu", s/60, s%60);
        else
          sprintf(buf, "%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
        xio_fputs(buf, of);
      }
      xio_fputs(", ", of);
      xio_fputs(tech.info, of);
      xio_fputs("\n", of);
    }
    // Object name
    { xstring object = pp->GetURL().makeRelative(GetURL(), relative);
      const char* cp = object;
      if (strnicmp(cp,"file://", 7) == 0)
      { cp += 5;
        if (cp[2] == '/')
          cp += 3;
      }
      xio_fputs(cp, of);
      xio_fputs("\n", of);
    }
    // tech info
    if (pp->GetStatus() > STA_Invalid && pp->CheckInfo(IF_Tech|IF_Format) != IF_Tech|IF_Format)
    { char buf[50];
      if (pp->CheckInfo(IF_Tech) == IF_None)
      { sprintf(buf, ">%i", tech.bitrate);
        xio_fputs(buf, of);
      } else
        xio_fputs(">", of);
      if (pp->CheckInfo(IF_Format) == IF_None)
      { const FORMAT_INFO2& format = *pp->GetInfo().format;
        sprintf(buf, ",%i,%i", format.samplerate, format.channels);
        xio_fputs(buf, of);
      } else
        xio_fputs(",,", of);
      if (pp->CheckInfo(IF_Tech) == IF_None)
      { sprintf(buf, ",%.0f,%.3f", tech.filesize, tech.songlength);
        xio_fputs(buf, of);
        if (pp->GetFlags() & Enumerable)
        sprintf(buf, ",%i,%i", tech.num_items, tech.recursive);
        xio_fputs(buf, of);
      }
      xio_fputs("\n", of);
    }
  }
  // Footer
  xio_fputs("# End of playlist\n", of);
  return true;
}

bool PlayableCollection::SaveM3U(XFILE* of, bool relative)
{ DEBUGLOG(("PlayableCollection(%p{%s})::SaveM3U(%p, %u)\n", this, GetURL().cdata(), of, relative));
  Mutex::Lock lock(Mtx);
  int_ptr<PlayableInstance> pi;
  while ((pi = GetNext(pi)) != NULL)
  { Playable* pp = pi->GetPlayable();
    // Object name
    xstring object = pp->GetURL().makeRelative(GetURL(), relative);
    const char* cp = object;
    if (strnicmp(cp,"file://",7) == 0)
    { cp += 5;
      if (cp[2] == '/')
        cp += 3;
    }
    xio_fputs(cp, of);
    xio_fputs("\n", of);
  }
  return true;
}

bool PlayableCollection::Save(const url123& URL, save_options opt)
{ DEBUGLOG(("PlayableCollection(%p{%s})::Save(%s, %x)\n", this, GetURL().cdata(), URL.cdata(), opt));
  // verify URL
  if (!IsPlaylist(URL))
    return false;

  // create file
  XFILE* of = xio_fopen(URL, "w");
  if (of == NULL)
    return false;

  // now write the playlist
  bool rc;
  if (opt & SaveAsM3U)
    rc = SaveM3U(of, (opt & SaveRelativePath) != 0);
  else
    rc = SaveLST(of, (opt & SaveRelativePath) != 0);

  // done
  xio_fclose(of);

  if (!rc)
    return false;

  // notifications
  int_ptr<Playable> pp = FindByURL(URL);
  if (pp && pp != this)
  { ASSERT(pp->GetFlags() & Enumerable);
    ((PlayableCollection&)*pp).NotifySourceChange();
  }
  return true;
}

void PlayableCollection::NotifySourceChange()
{}


/****************************************************************************
*
*  class Playlist
*
****************************************************************************/

void Playlist::LSTReader::Reset()
{ DEBUGLOG(("Playlist::LSTReader::Reset()\n"));
  has_format   = false;
  has_tech     = false;
  has_techinfo = false;
  Alias        = NULL;
  Slice        = PlayableInstance::slice::Initial;
  URL          = NULL;
}

void Playlist::LSTReader::Create()
{ DEBUGLOG(("Playlist::LSTReader::Create() - %p, %u, %u, %u\n", URL.cdata(), has_format, has_tech, has_techinfo));
  if (URL)
  { Entry* ep = List.CreateEntry(URL, has_format ? &Format : NULL, has_tech && has_techinfo ? &Tech : NULL);
    ep->SetAlias(Alias);
    ep->SetSlice(Slice);
    List.AppendEntry(ep);
  }
}

void Playlist::LSTReader::ParseLine(char* line)
{ DEBUGLOG(("Playlist::LSTReader::ParseLine(%s)\n", line));
  switch (line[0])
  {case '#':
    // comments close URL
    if (URL)
    { Create();
      Reset();
    }
    // prefix lines
    if (memcmp(line+1, "ALIAS ", 6) == 0) // alias name
    { Alias = line+7;
    } else if (memcmp(line+1, "SLICE ", 6) == 0) // slice
    { sscanf(line+7, "%lf,%lf", &Slice.Start, &Slice.Stop);
    } else if (line[1] == ' ')
    { const char* cp = strstr(line+2, ", ");
      if (cp)
      { cp = strstr(cp+2, ", ");
        if (cp)
        { strlcpy(Tech.info, cp+2, sizeof Tech.info);
          has_techinfo = true;
      } }
    }
    break;

   default:
    // close last URL
    Create();
    if (URL)
      Reset();
    URL = List.GetURL().makeAbsolute(line);
    break;

   case '>':
    { // tokenize string
      const char* tokens[7] = {NULL};
      const char** tp = tokens;
      *tp = strtok(line+1, ",");
      while (*tp)
      { if (**tp == 0)
          *tp = NULL;
        *++tp = strtok(NULL, ",");
        if (tp == tokens + sizeof tokens/sizeof *tokens)
          break;
      }
      DEBUGLOG(("Playlist::LSTReader::ParseLine: tokens %s, %s, %s, %s, %s, %s, %s\n", tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]));
      // make tech info
      if (tokens[0] && tokens[3] && tokens[4])
      { has_tech = true;
        // memset(Tech, 0, sizeof Tech);
        Tech.size       = sizeof Tech;
        Tech.bitrate    = atoi(tokens[0]);
        Tech.filesize   = atof(tokens[3]);
        Tech.songlength = atof(tokens[4]);
        Tech.num_items  = tokens[5] ? atoi(tokens[5]) : 1;
        Tech.recursive  = tokens[6] != NULL;
      }
      // make format info
      if (tokens[1] && tokens[2])
      { has_format = true;
        // memset(Format, 0, sizeof Format);
        Format.size       = sizeof Format;
        Format.samplerate = atoi(tokens[1]);
        Format.channels   = atoi(tokens[2]);
        switch (Format.channels) // convert stupid modes of old PM123 versions
        {case 0:
          Format.channels = 2;
          break;
         case 3:
          Format.channels = 1;
          break;
        }
      }
      break;
    }

   case '<':
    // close URL
    Create();
    Reset();
    break;
  }
}


Playable::Flags Playlist::GetFlags() const
{ return Mutable;
}

bool Playlist::IsModified() const
{ return Modified;
}

bool Playlist::LoadLST(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadLST(%p)\n", this, GetURL().getShortName().cdata(), x));

  // TODO: fixed buffers...
  char line[4096];
  LSTReader rdr(*this);

  while (xio_fgets(line, sizeof(line), x))
  { blank_strip(line);
    if (*line == 0)
      continue;
    rdr.ParseLine(line);
  }
  return true;
}

bool Playlist::LoadMPL(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadMPL(%p)\n", this, GetURL().getShortName().cdata(), x));

  // TODO: fixed buffers...
  char   file    [4096];
  char*  eq_pos;

  while (xio_fgets(file, sizeof(file), x))
  {
    blank_strip( file );

    if( *file != 0 && *file != '#' && *file != '[' && *file != '>' && *file != '<' )
    {
      eq_pos = strchr( file, '=' );

      if( eq_pos && strnicmp( file, "File", 4 ) == 0 )
      {
        strcpy( file, eq_pos + 1 );
        // TODO: fetch position
        AppendEntry(CreateEntry(file));
      }
    }
  }
  return true;
}

bool Playlist::LoadPLS(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadPLS(%p)\n", this, GetURL().getShortName().cdata(), x));

  xstring file;
  xstring title;
  char   line    [4096];
  int    last_idx = -1;
  const char*  eq_pos;

  while( xio_fgets( line, sizeof(line), x))
  {
    blank_strip( line );

    if( *line != 0 && *line != '#' && *line != '[' && *line != '>' && *line != '<' )
    {
      eq_pos = strchr( line, '=' );

      if( eq_pos ) {
        if( strnicmp( line, "File", 4 ) == 0 )
        {
          if( file ) {
            // TODO: position
            Entry* ep = CreateEntry(file);
            ep->SetAlias(title);
            AppendEntry(ep);
            title = NULL;
            file = NULL;
          }

          file = eq_pos + 1;
          last_idx = atoi( &line[4] );
        }
        else if( strnicmp( line, "Title", 5 ) == 0 )
        {
          // We hope the title field always follows the file field
          if( last_idx == atoi( &line[5] )) {
            title = eq_pos + 1;
          }
        }
      }
    }
  }

  if( *file ) {
    // TODO: fetch position
    Entry* ep = CreateEntry(file);
    ep->SetAlias(title);
    AppendEntry(ep);
            
  }

  return true;
}

bool Playlist::LoadList()
{ DEBUGLOG(("Playlist(%p{%s})::LoadList()\n", this, GetURL().getShortName().cdata()));

  // clear content if any
  // TODO: more sophisticated approach
  while (Head)
    RemoveEntry(Head);

  bool rc = false;

  xstring ext = GetURL().getExtension();

  // open file
  XFILE* x = xio_fopen(GetURL(), "r");
  if (x == NULL)
    amp_player_error("Unable open playlist file:\n%s\n%s", GetURL().cdata(), xio_strerror(xio_errno()));
   else
  { if ( stricmp( ext, ".lst" ) == 0 || stricmp( ext, ".m3u" ) == 0 )
      rc = LoadLST(x);
    else if ( stricmp( ext, ".mpl" ) == 0 )
      rc = LoadMPL(x);
    else if ( stricmp( ext, ".pls" ) == 0 )
      rc = LoadPLS(x);
    else
      amp_player_error("Cannot determine playlist format from file extension %s.\n", ext.cdata());

    xio_fclose( x );
  }
  InfoChangeFlags |= IF_Other;
  if (Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = false;
  }
  return rc;
}

bool Playlist::InsertItem(const char* url, const xstring& alias, const PlayableInstance::slice& sl, PlayableInstance* before)
{ DEBUGLOG(("Playlist(%p{%s})::InsertItem(%s, %s, {%.1f,%.1f}, %p) - %u\n", this, GetURL().getShortName().cdata(), url, alias?alias.cdata():"<NULL>", sl.Start, sl.Stop, before, Stat));
  Mutex::Lock lock(Mtx);
  // Check whether the parameter before is still valid
  if (before && !before->IsParent(this))
    return false;
  // point of no return...
  Entry* ep = CreateEntry(url);
  ep->SetAlias(alias);
  ep->SetSlice(sl);
  if (Stat <= STA_Invalid)
    UpdateStatus(STA_Normal);

  if (before == NULL)
    AppendEntry(ep);
   else
    InsertEntry(ep, (Entry*)before);
  InfoChangeFlags |= IF_Other;
  if (!Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = true;
  }
  // update info
  LoadInfoAsync(InfoValid & IF_Tech);
  // raise InfoChange event?
  RaiseInfoChange();
  // done!
  return true;
}

bool Playlist::MoveItem(PlayableInstance* item, PlayableInstance* before)
{ DEBUGLOG(("Playlist(%p{%s})::InsertItem(%p{%s}, %p{%s}) - %u\n", this, GetURL().getShortName().cdata(),
    item, item->GetPlayable()->GetURL().cdata(), before ? before->GetPlayable()->GetURL().cdata() : ""));
  Mutex::Lock lock(Mtx);
  // Check whether the parameter before is still valid
  if (!item->IsParent(this) || (before && !before->IsParent(this)))
    return false;
  // Now move the entry.
  MoveEntry((Entry*)item, (Entry*)before);
  InfoChangeFlags |= IF_Other;
  if (!Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = true;
  }
  // raise InfoChange event?
  RaiseInfoChange();
  // done!
  return true;
}

bool Playlist::RemoveItem(PlayableInstance* item)
{ DEBUGLOG(("Playlist(%p{%s})::RemoveItem(%p{%s})\n", this, GetURL().getShortName().cdata(),
    item, item ? item->GetPlayable()->GetURL().cdata() : ""));
  Mutex::Lock lock(Mtx);
  // Check whether the item is still valid
  if (item && !item->IsParent(this))
    return false;
  // now detach the item from the container
  if (item)
    RemoveEntry((Entry*)item);
  else
  { if (!Head)
      return true; // early exit without change flag
    do
    { RemoveEntry(Head);
      DEBUGLOG(("Playlist::RemoveItem - %p\n", &*Head));
    } while (Head);
  }
  InfoChangeFlags |= IF_Other;
  if (!Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = true;
  }
  // update tech info
  LoadInfoAsync(InfoValid & IF_Tech);
  // raise InfoChange event?
  DEBUGLOG(("Playlist::RemoveItem: before raiseinfochange\n"));
  RaiseInfoChange();
  DEBUGLOG(("Playlist::RemoveItem: after raiseinfochange\n"));
  return true;
}

void Playlist::Sort(ItemComparer comp)
{ DEBUGLOG(("Playlist(%p)::Sort(%p)\n", this, comp));
  Mutex::Lock lock(Mtx);
  if (Head == Tail)
    return; // Empty or one element lists are always sorted.
  // Create index array
  vector<PlayableInstance> index(64);
  Entry* ep = Head;
  do
  { index.append() = ep;
    ep = ep->Next;
  } while (ep);
  // Sort index array
  merge_sort(index.begin(), index.end(), comp);
  // Adjust item sequence
  ep = Head;
  bool changed = false;
  for (PlayableInstance** ipp = index.begin(); ipp != index.end(); ++ipp)
  { changed |= MoveEntry((Entry*)*ipp, ep);
    ep = ((Entry*)*ipp)->Next;
  }
  // done
  if (changed)
  { InfoChangeFlags |= IF_Other;
    if (!Modified)
    { InfoChangeFlags |= IF_Status;
      Modified = true;
  } }
  // raise InfoChange event?
  DEBUGLOG(("Playlist::Sort: before raiseinfochange\n"));
  RaiseInfoChange();
  DEBUGLOG(("Playlist::Sort: after raiseinfochange\n"));
}

void Playlist::Shuffle()
{ DEBUGLOG(("Playlist(%p)::Shuffle()\n", this));
  Mutex::Lock lock(Mtx);
  if (Head == Tail)
    return; // Empty or one element lists are always sorted.
  // move records randomly to the beginning or the end.
  Entry* current = Head;
  bool changed = false;
  do
  { Entry* next = current->Next;
    changed |= MoveEntry(current, rand() & 1 ? &*Head : NULL);
    current = next;
  } while (current);
  // done
  if (changed)
  { InfoChangeFlags |= IF_Other;
    if (!Modified)
    { InfoChangeFlags |= IF_Status;
      Modified = true;
  } }
  // raise InfoChange event?
  DEBUGLOG(("Playlist::Shuffle: before raiseinfochange\n"));
  RaiseInfoChange();
  DEBUGLOG(("Playlist::Shuffle: after raiseinfochange\n")); 
}

bool Playlist::Save(const url123& URL, save_options opt)
{ DEBUGLOG(("Playlist(%p)::Save(%s, %x)\n", this, URL.cdata(), opt));
  if (!PlayableCollection::Save(URL, opt))
    return false;
  if (URL == GetURL())
  { // TODO: this should be done atomic with the save.
    Mutex::Lock lock(Mtx);
    if (Modified)
    { InfoChangeFlags |= IF_Status;
      Modified = false;
      RaiseInfoChange();
    }
  }
  return true;
}

void Playlist::NotifySourceChange()
{ DEBUGLOG(("Playlist(%p{%s})::NotifySourceChange()", this, GetURL().cdata()));
  Mutex::Lock lock(Mtx);
  if (!Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = true;
    RaiseInfoChange();
  }
}


/****************************************************************************
*
*  class PlayFolder
*
****************************************************************************/

PlayFolder::PlayFolder(const url123& URL, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: PlayableCollection(URL, ca_tech, ca_meta),
  Recursive(false)
{ DEBUGLOG(("PlayFolder(%p)::PlayFolder(%s, %p, %p)\n", this, URL.cdata(), ca_tech, ca_meta));
  ParseQueryParams();
}

void PlayFolder::ParseQueryParams()
{ xstring params = GetURL().getParameter();
  DEBUGLOG(("PlayFolder(%p)::ParseQueryParams() - %s\n", this, params.cdata()));
  if (params.length() == 0)
    return; // no params
  const char* cp = params.cdata() +1;
  if (*cp == 0)
    return; // empty params
  for(;;)
  { const char* cp2 = strchr(cp, '&');
    xstring arg(cp, cp2 ? cp2-cp : params.cdata()+params.length()-cp);
    DEBUGLOG(("PlayFolder::ParseQueryParams: arg=%s\n", arg.cdata()));
    if (arg.startsWithI("pattern="))
      Pattern.assign(arg.cdata() +8);
    else if (arg.startsWithI("recursive"))
      Recursive = true;
    else
      DEBUGLOG(("PlayFolder::ParseQueryParams: invalid option %s\n", arg.cdata()));
    if (cp2 == NULL)
      break;
    cp = cp2 +1;
  }
  DEBUGLOG(("PlayFolder::ParseQueryParams: %s %u\n", Pattern ? Pattern.cdata() : "<null>", Recursive));
}

bool PlayFolder::LoadList()
{ DEBUGLOG(("PlayFolder(%p{%s})::LoadList()\n", this, GetURL().getShortName().cdata()));
  if (!GetURL().isScheme("file:")) // Can't handle anything but filesystem folders so far.
    return false;

  while (Head)
    RemoveEntry(Head);

  xstring name = GetURL().getBasePath();
  // strinp file:[///]
  if (memcmp(name.cdata()+5, "///", 3) == 0) // local path?
    name.assign(name, 8);
   else
    name.assign(name, 5);
  // append pattern
  if (Pattern)
    name = name + Pattern;
   else
    name = name + "*";
  HDIR hdir = HDIR_CREATE;
  char result[2048];
  ULONG count = sizeof result / (offsetof(FILEFINDBUF3, achName)+2); // Well, some starting value...
  APIRET rc = DosFindFirst(name, &hdir, Recursive ? FILE_ARCHIVED|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY|FILE_DIRECTORY
                                                  : FILE_ARCHIVED|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY,
                           result, sizeof result, &count, FIL_STANDARD);
  DEBUGLOG(("PlayFolder::LoadList: %s, %p, %u, %u\n", name.cdata(), hdir, count, rc));
  if (rc == NO_ERROR)
  { do
    { // add files
      for (FILEFINDBUF3* fp = (FILEFINDBUF3*)result; count--; ((char*&)fp) += fp->oNextEntryOffset)
      { DEBUGLOG(("PlayFolder::LoadList - %s, %x\n", fp->achName, fp->attrFile));
        // skip . and ..
        if (fp->achName[0] == '.' && (fp->achName[1] == 0 || (fp->achName[1] == '.' && fp->achName[2] == 0)))
          continue;
        Entry* ep;
        if (fp->attrFile & FILE_DIRECTORY)
        { // inherit parameters
          xstring tmp = xstring::sprintf("%s/%s", fp->achName, GetURL().getParameter().cdata());
          ep = CreateEntry(tmp);
        } else
          ep = CreateEntry(fp->achName);
        AppendEntry(ep);
      }
      // next...
      count = sizeof result / sizeof(FILEFINDBUF3);
      rc = DosFindNext(hdir, &result, sizeof result, &count);
    } while (rc == 0);
    DosFindClose(hdir);
  }
  DEBUGLOG(("PlayFolder::LoadList: %u\n", rc));
  switch (rc)
  {case NO_ERROR:
   case ERROR_FILE_NOT_FOUND:
   case ERROR_NO_MORE_FILES:
    return true;
   default:
    return false;
  }
}


