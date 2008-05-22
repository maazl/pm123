/*
 * Copyright 2007-2008 M.Mueller
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
//#include "plugman.h"
#include "playablecollection.h"
#include "songiterator.h"
#include "123_util.h"
#include "copyright.h"
#include <xio.h>
#include <strutils.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <debuglog.h>


/****************************************************************************
*
*  class Slice
*
****************************************************************************/

/*bool operator==(const Slice& l, const Slice& r)
{ return (l.Start == NULL && r.Start == NULL || l.Start != NULL && r.Start != NULL && *l.Start == *r.Start)
      && (l.Stop  == NULL && r.Stop  == NULL || l.Stop  != NULL && r.Stop  != NULL && *l.Stop  == *r.Stop );
}*/


/****************************************************************************
*
*  class PlayableSlice
*
****************************************************************************/

PlayableSlice::PlayableSlice(Playable* pp)
: RefTo(pp),
  Start(NULL),
  Stop(NULL)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(%p)\n", this, pp));
  ASSERT(pp);
}
PlayableSlice::PlayableSlice(const PlayableSlice& r)
: RefTo(r.RefTo),
  Alias(r.Alias)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(&%p)\n", this, &r));
  if (r.Start)
    Start = new SongIterator(*r.Start);
  else
    Start = NULL;
  if (r.Stop)
    Stop = new SongIterator(*r.Stop);
  else
    Stop = NULL;
}
PlayableSlice::PlayableSlice(const url123& url, const xstring& alias)
: RefTo(Playable::GetByURL(url)),
  Alias(alias),
  Start(NULL),
  Stop(NULL)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(%s, %s) - %p\n", this, url.cdata(), alias ? alias.cdata() : "<null>", RefTo.get()));
  ASSERT((int)RefTo.get() > 0x10000); // OS/2 check trick
  ASSERT(&RefTo);
  if (Alias && Alias.length() == 0)
    Alias = NULL;
}
PlayableSlice::PlayableSlice(const url123& url, const xstring& alias, const char* start, const char* stop)
: RefTo(Playable::GetByURL(url)),
  Alias(alias),
  Start(NULL),
  Stop(NULL)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(%s, %s, %s, %s) - %p\n", this,
    url.cdata() ? url.cdata() : "<NULL>", alias.cdata() ? alias.cdata() : "<NULL>",
    start ? start : "<NULL>", stop ? stop : "<NULL>", RefTo.get()));
  if (Alias && Alias.length() == 0)
    Alias = NULL;
  if ((start && *start) || (stop && *stop))
  { // Internal instance to avoid cyclic references
    PlayableSlice* ps = new PlayableSlice(GetPlayable());
    if (start && *start)
    { SongIterator* psi = new SongIterator();
      psi->SetRoot(ps);
      psi->Deserialize(start);
      // TODO: errors
      Start = psi;
    }
    if (stop && *stop)
    { SongIterator* psi = new SongIterator();
      psi->SetRoot(ps);
      psi->Deserialize(stop);
      // TODO: errors
      Stop = psi;
    }
  }
}

PlayableSlice::~PlayableSlice()
{ DEBUGLOG(("PlayableSlice(%p)::~PlayableSlice() - %p %p %p\n", this, RefTo.get(), Start, Stop));
  delete Start;
  delete Stop;
};

void PlayableSlice::SetStart(SongIterator* iter)
{ DEBUGLOG(("PlayableSlice(%p)::SetStart(%p{%s})\n", this, iter, SongIterator::DebugName(iter).cdata()));
  delete Start;
  ASSERT(iter == NULL || iter->GetRoot()->GetPlayable() == GetPlayable());
  Start = iter;
}

void PlayableSlice::SetStop(SongIterator* iter)
{ DEBUGLOG(("PlayableSlice(%p)::SetStop(%p{%s})\n", this, iter, SongIterator::DebugName(iter).cdata()));
  delete Stop;
  ASSERT(iter == NULL || iter->GetRoot()->GetPlayable() == GetPlayable());
  Stop = iter;
}

void PlayableSlice::SetAlias(const xstring& alias)
{ DEBUGLOG(("PlayableSlice(%p)::SetAlias(%s)\n", this, alias ? alias.cdata() : ""));
  Alias = alias;
}

void PlayableSlice::SetInUse(bool used)
{ // Forward to Playable only
  RefTo->SetInUse(used);
}

#ifdef DEBUG
xstring PlayableSlice::DebugName() const
{ return xstring::sprintf("{%s, %s,%s, %s}",
    RefTo->GetURL().cdata(),
    SongIterator::DebugName(Start).cdata(),
    SongIterator::DebugName(Stop).cdata(),
    Alias ? Alias.cdata() : "");
}
#endif

/*PlayableInstRef::PlayableInstRef(PlayableInstance* ref)
: PlayableSlice(ref->GetPlayable()),
  InstRef(ref)
{}

PlayableInstance* PlayableInstRef::GetPlayableInstance() const
{ return InstRef;
}*/


/****************************************************************************
*
*  class PlayableInstance
*
****************************************************************************/

PlayableInstance::PlayableInstance(PlayableCollection& parent, Playable* playable)
: PlayableSlice(playable),
  Parent(&parent),
  Stat(STA_Normal)
{ DEBUGLOG(("PlayableInstance(%p)::PlayableInstance(%p{%s}, %p{%s})\n", this,
    &parent, parent.GetURL().getShortName().cdata(), playable, playable->GetURL().getShortName().cdata()));
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
  return GetAlias() ? GetAlias() : RefTo->GetDisplayName();
}

void PlayableInstance::SetStart(SongIterator* iter)
{ DEBUGLOG(("PlayableInstance(%p)::SetStart(%p{%s})\n", this, iter, SongIterator::DebugName(iter).cdata()));
  if ((!iter ^ !GetStart()) || iter && GetStart() && GetStart()->CompareTo(*iter) != 0)
  { PlayableSlice::SetStart(iter);
    StatusChange(change_args(*this, SF_Slice));
  }
}

void PlayableInstance::SetStop(SongIterator* iter)
{ DEBUGLOG(("PlayableInstance(%p)::SetStop(%p{%s})\n", this, iter, SongIterator::DebugName(iter).cdata()));
  if ((!iter ^ !GetStop()) || iter && GetStop() && GetStop()->CompareTo(*iter) != 0)
  { PlayableSlice::SetStop(iter);
    StatusChange(change_args(*this, SF_Slice));
  }
}

void PlayableInstance::SetAlias(const xstring& alias)
{ DEBUGLOG(("PlayableInstance(%p)::SetAlias(%p{%s})\n", this, alias.cdata(), alias ? alias.cdata() : ""));
  if (GetAlias() != alias)
  { PlayableSlice::SetAlias(alias);
    StatusChange(change_args(*this, SF_Alias));
  }
}

bool operator==(const PlayableInstance& l, const PlayableInstance& r)
{ return l.Parent == r.Parent
      && l.RefTo  == r.RefTo  // Instance equality is sufficient in case of the Playable class.
      && l.GetAlias()    == r.GetAlias();
//      && (const Slice&)l == (const Slice&)r;
}

int PlayableInstance::CompareTo(const PlayableInstance& r) const
{ if (this == &r)
    return 0;
  int_ptr<PlayableCollection> lp;
  lp.assign_weak(Parent);
  // Removed items or different collection.
  if (lp == NULL || r.Parent == NULL || lp != r.Parent)
    return 4;
  // Attension: nothing is locked here except for the life-time of the parent playlists.
  // So the item l or r may still be removed from the collection by another thread.
  const PlayableInstance* pi = this;
  for (;;)
  { pi = lp->GetNext(pi);
    if (pi == NULL)
      return 1;
    if (pi == &r)
      return -1;
  }
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
void PlayableCollection::CollectionInfo::Add(const CollectionInfo& ci)
{ Add(ci.Songlength, ci.Filesize, ci.Items);
  // Excluded items
  for (Playable*const* i = ci.Excluded.begin(); i != ci.Excluded.end(); ++i)
    Excluded.get(**i) = *i;
}

void PlayableCollection::CollectionInfo::Reset()
{ Songlength = 0;
  Filesize   = 0;
  Items      = 0;
  Excluded.clear();
}

const FORMAT_INFO2 PlayableCollection::no_format =
{ sizeof(FORMAT_INFO2),
  -1,
  -1
};

PlayableCollection::PlayableCollection(const url123& URL, const TECH_INFO* ca_tech, const META_INFO* ca_meta, const PHYS_INFO* ca_phys)
: Playable(URL, &no_format, ca_tech, ca_meta, ca_phys),
  CollectionInfoCache(16)
{ DEBUGLOG(("PlayableCollection(%p)::PlayableCollection(%s, %p, %p, %p)\n", this, URL.cdata(), ca_tech, ca_meta, ca_phys));
}

PlayableCollection::~PlayableCollection()
{ DEBUGLOG(("PlayableCollection(%p{%s})::~PlayableCollection()\n", this, GetURL().getShortName().cdata()));
  // frist disable all events
  CollectionChange.sync_reset();
  InfoChange.sync_reset();
  // Cleanup SubInfoCache
  for (CollectionInfoEntry** ppci = CollectionInfoCache.begin(); ppci != CollectionInfoCache.end(); ++ppci)
    delete *ppci;
  // Cleanup container items because removing Head and Tail is not sufficient
  // since the doubly linked list has implicit cyclic references.
  while (Head)
  { ASSERT(Head->IsParent(this));
    Entry* ep = Head;
    Head = Head->Next;
    // ep ist still valid because the item is still held by Head->Prev or Tail.
    ep->Detach();
    ep->Prev = NULL;
    ep->Next = NULL;
  }
  // *Tail is implicitely deleted by the destructor of Tail. 
}

Playable::Flags PlayableCollection::GetFlags() const
{ return Enumerable;
}

bool PlayableCollection::IsModified() const
{ return false;
}

PlayableCollection::Entry* PlayableCollection::CreateEntry(const char* url, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta, const PHYS_INFO* ca_phys)
{ DEBUGLOG(("PlayableCollection(%p{%s})::CreateEntry(%s, %p, %p, %p, %p)\n",
    this, GetURL().getShortName().cdata(), url, ca_format, ca_tech, ca_meta, ca_phys));
  // now create the entry with absolute path
  int_ptr<Playable> pp = GetByURL(GetURL().makeAbsolute(url), ca_format, ca_tech, ca_meta, ca_phys);
  // initiate prefetch of information
  pp->EnsureInfoAsync(IF_Status, true);
  return new Entry(*this, pp, &PlayableCollection::ChildInfoChange, &PlayableCollection::ChildInstChange);
}

void PlayableCollection::AppendEntry(Entry* entry)
{ DEBUGLOG(("PlayableCollection(%p{%s})::AppendEntry(%p{%s,%p,%p})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), entry->Prev.get(), entry->Next.get()));
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
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), entry->Prev.get(), entry->Next.get(), before, before->GetPlayable()->GetURL().getShortName().cdata()));
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
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), entry->Prev.get(), entry->Next.get(), before, before ? before->GetPlayable()->GetURL().getShortName().cdata() : ""));
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
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), entry->Prev.get(), entry->Next.get()));
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
    dst.totalsize  = ci.Filesize;
    dst.total_items= ci.Items;
    dst.recursive  = ci.Excluded.find(*this) != NULL;

    dst.bitrate = (int)( dst.totalsize >= 0 && dst.songlength > 0
      ? dst.totalsize / dst.songlength / (1000/8)
      : -1 );
    DEBUGLOG(("PlayableCollection::CalcTechInfo(): %s {%g, %i, %g, %i, %u}\n",
      GetURL().getShortName().cdata(), dst.songlength, dst.bitrate, dst.totalsize, dst.total_items, dst.recursive));
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
      else if (hours)
        sprintf(dst.info, "Total: %u:%02u:%02u", hours, minutes, (int)rem);
      else
        sprintf(dst.info, "Total: %u:%02u", minutes, (int)rem);
    }
    if (dst.recursive)
      strcat(dst.info, ", recursion");
  }
}

int_ptr<PlayableInstance> PlayableCollection::GetPrev(const PlayableInstance* cur) const
{ DEBUGLOG(("PlayableCollection(%p)::GetPrev(%p{%s})\n", this, cur, cur ? cur->GetPlayable()->GetURL().cdata() : ""));
  ASSERT(InfoValid & IF_Other);
  // The PlayableInstance (if any) may either belong the the current list or be removed earlier.
  ASSERT(cur == NULL || cur->IsParent(NULL) || cur->IsParent(this));
  return (PlayableInstance*)(cur ? ((Entry*)cur)->Prev : Tail);
}

int_ptr<PlayableInstance> PlayableCollection::GetNext(const PlayableInstance* cur) const
{ DEBUGLOG(("PlayableCollection(%p)::GetNext(%p{%s})\n", this, cur, cur ? cur->GetPlayable()->GetURL().cdata() : ""));
  ASSERT(InfoValid & IF_Other);
  // The PlayableInstance (if any) may either belong the the current list or be removed earlier.
  ASSERT(cur == NULL || cur->IsParent(NULL) || cur->IsParent(this));
  return (PlayableInstance*)(cur ? ((Entry*)cur)->Next : Head);
}

xstring PlayableCollection::SerializeItem(const PlayableInstance* cur, serialization_options opt) const
{ DEBUGLOG(("PlayableCollection(%p{%s})::SerializeItem(%p, %u)\n",
    this, GetURL().getShortName().cdata(), cur, opt));
  // check whether we are unique
  size_t count = 1;
  int_ptr<PlayableInstance> pi = GetPrev(cur);
  while (pi != NULL)
  { if (opt & SerialIndexOnly || pi->GetPlayable() == cur->GetPlayable())
      ++count;
    pi = GetPrev(pi);
  }
  // Index only?
  if (opt & SerialIndexOnly)
    return xstring::sprintf("[%u]", count);
  xstring ret;
  // fetch relative or absolute URL
  if (opt & SerialRelativePath)
    ret = cur->GetPlayable()->GetURL().makeRelative(GetURL(), !!(opt & SerialUseUpdir));
  else
    ret = cur->GetPlayable()->GetURL();
  // append count?
  return xstring::sprintf(count > 1 ? "\"%s\"[%u]" : "\"%s\"", ret.cdata(), count);
}

int_ptr<PlayableInstance> PlayableCollection::DeserializeItem(const xstring& str) const
{ DEBUGLOG(("PlayableCollection(%p{%s})::DeserializeItem(%s)\n",
    this, GetURL().getShortName().cdata(), str.cdata()));
  // TODO:
  return NULL;
}

void PlayableCollection::PrefetchSubInfo(const PlayableSet& excluding)
{ DEBUGLOG(("PlayableCollection(%p{%s})::PrefetchSubInfo() - %x\n", this, GetURL().getShortName().cdata(), InfoValid));
  EnsureInfo(IF_Other);
  int_ptr<PlayableInstance> pi;
  sco_ptr<PlayableSet> xcl_sub;
  while ((pi = GetNext(pi)) != NULL)
  { Playable* pp = pi->GetPlayable();
    if (excluding.find(*pp) != NULL)
      continue; // recursion
    if (pp->GetFlags() & Playable::Enumerable)
    { // create new exclusions on demand
      if (xcl_sub == NULL)
      { xcl_sub = new PlayableSet(excluding); // deep copy
        xcl_sub->get(*this) = this;
      }
      // prefetch sublists too.
      ((PlayableCollection*)pp)->PrefetchSubInfo(*xcl_sub);
    }
  }
}

const PlayableCollection::CollectionInfo& PlayableCollection::GetCollectionInfo(const PlayableSet& excluding)
{ DEBUGLOG(("PlayableCollection(%p{%s})::GetCollectionInfo({%i, %s})\n", this, GetURL().getShortName().cdata(), excluding.size(), excluding.DebugDump().cdata()));
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

  // If we do not own the mutex so far we ensure some information on sub items
  // before we request access to the mutex to avoid deadlocks.
  // If we already own the mutex it makes no difference anyway.
  if (Mtx.GetStatus() != Mutex::Mine)
    PrefetchSubInfo(excluding);
  else
    // Ensure to have the list content before anything else
    // This is inclusive in the above term. 
    EnsureInfo(IF_Other);

  // (re)create information
  sco_ptr<PlayableSet> xcl_sub;
  PlayableInstance* pi = NULL;
  // now it's time to lock the collection (if not yet done)
  Mutex::Lock lock(Mtx);
  while ((pi = GetNext(pi)) != NULL)
  { DEBUGLOG(("PlayableCollection::GetCollectionInfo Item %p{%p{%s}}\n", pi, pi->GetPlayable(), pi->GetPlayable()->GetURL().cdata()));
    if (pi->GetPlayable()->GetFlags() & Playable::Enumerable)
    { // nested playlist
      PlayableCollection* pc = (PlayableCollection*)pi->GetPlayable();
      // TODO: support slicing!
      // check for recursion
      if (excluding.find(*pc) != NULL)
      { // recursion
        DEBUGLOG(("PlayableCollection::GetCollectionInfo - recursive!\n"));
        cic->Info.Excluded.get(*pc) = pc;
        continue;
      }
      DEBUGLOG(("PlayableCollection::GetCollectionInfo - enter\n"));
      // create new exclusions on demand
      if (xcl_sub == NULL)
      { xcl_sub = new PlayableSet(excluding); // deep copy
        xcl_sub->get(*this) = this;
      }
      // get sub info
      cic->Info.Add(pc->GetCollectionInfo(*xcl_sub));

    } else
    { // Song
      DEBUGLOG(("PlayableCollection::GetCollectionInfo - Song\n"));
      Song* song = (Song*)pi->GetPlayable();
      song->EnsureInfo(IF_Tech|IF_Phys);
      if (song->GetStatus() > STA_Invalid)
      { const DECODER_INFO2& info = song->GetInfo();
        // take care of slice
        T_TIME length = info.tech->songlength;
        if (pi->GetStop() && length > pi->GetStop()->GetLocation())
          length = pi->GetStop()->GetLocation();
        if (pi->GetStart())
        { if (pi->GetStart()->GetLocation() >= length)
            length = 0;
          else
            length -= pi->GetStart()->GetLocation();
        }
        // Scale filesize with slice to preserve bitrate.
        cic->Info.Add(length, info.phys->filesize * length / info.tech->songlength, 1);
      } else
        // only count invalid items
        cic->Info.Add(0, 0, 1);
    }
  }
  DEBUGLOG(("PlayableCollection::GetCollectionInfo: {%f, %f, %i, {%u,...}}}\n",
    cic->Info.Songlength, cic->Info.Filesize, cic->Info.Items, cic->Info.Excluded.size()));
  return cic->Info;
}

Playable::InfoFlags PlayableCollection::LoadInfo(InfoFlags what)
{ DEBUGLOG(("PlayableCollection(%p{%s})::LoadInfo(%x) - %x %u\n", this, GetURL().getShortName().cdata(), what, InfoValid, Stat));
  Mutex::Lock lock(Mtx);

  // Need list content to generate tech info
  if ((what & IF_Phys|IF_Tech) && !(InfoValid & IF_Other))
    what |= IF_Other;

  if (what & (IF_Other|IF_Status))
  { // load playlist content
    PlayableStatus stat;
    if (LoadList())
      stat = Stat == STA_Used ? STA_Used : STA_Normal;
    else
      stat = STA_Invalid;
    UpdateStatus(stat);
    strcpy(Decoder, "PM123.EXE");
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
  strcpy(Decoder, "PM123.EXE");
  InfoValid |= IF_Other;
  // raise InfoChange event?
  RaiseInfoChange();
  return Playable::LoadInfo(what);
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
    if (pi->GetStart())
    { xio_fputs("#START", of);
      xio_fputs(pi->GetStart()->Serialize(), of);
      xio_fputs("\n", of);
    }
    if (pi->GetStop())
    { xio_fputs("#STOP", of);
      xio_fputs(pi->GetStop()->Serialize(), of);
      xio_fputs("\n", of);
    }
    // comment
    const TECH_INFO& tech = *pp->GetInfo().tech;
    const PHYS_INFO& phys = *pp->GetInfo().phys;
    if (pp->GetStatus() > STA_Invalid && pp->CheckInfo(IF_Tech|IF_Phys) == 0) // only if immediately available
    { char buf[50];
      xio_fputs("# ", of);
      if (phys.filesize < 0)
        xio_fputs("n/a", of);
      else
      { sprintf(buf, "%.0lfkiB", phys.filesize/1024.);
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
    if (pp->GetStatus() > STA_Invalid && pp->CheckInfo(IF_Tech|IF_Format|IF_Phys) == 0)
    { char buf[128];
      // 0: bitrate, 1: samplingrate, 2: channels, 3: file size, 4: total length,
      // 5: no. of song items, 6: total file size, 7: no. of items, 8: recursive. 
      const FORMAT_INFO2& format = *pp->GetInfo().format;
      sprintf(buf, ">%i,%i,%i,%.0f,%.3f", tech.bitrate, format.samplerate, format.channels,
        phys.filesize, tech.songlength);
      xio_fputs(buf, of);
      // Playlists only...
      if (pp->GetFlags() & Enumerable)
      { sprintf(buf, ",%i,%.0f,%i", tech.total_items, tech.totalsize, phys.num_items);
        xio_fputs(buf, of);
        if (tech.recursive)
          xio_fputs(",1", of);
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
  has_phys     = false;
  Alias        = NULL;
  Start        = NULL;
  Stop         = NULL;
  URL          = NULL;
}

void Playlist::LSTReader::Create()
{ DEBUGLOG(("Playlist::LSTReader::Create() - %p, %u, %u, %u, %u\n", URL.cdata(), has_format, has_tech, has_techinfo, has_phys));
  if (URL)
  { Entry* ep = List.CreateEntry(URL, has_format ? &Format : NULL, has_tech && has_techinfo ? &Tech : NULL, NULL, has_phys ? &Phys : NULL);
    ep->SetAlias(Alias);
    if (Start || Stop)
    { PlayableSlice* ps = new PlayableSlice(ep->GetPlayable());
      if (Start)
      { SongIterator* psi = new SongIterator;
        psi->SetRoot(ps);
        const char* cp = Start;
        psi->Deserialize(cp);
        // TODO: errors
        ep->SetStart(psi);
      }
      if (Stop)
      { SongIterator* psi = new SongIterator;
        psi->SetRoot(ps);
        const char* cp = Stop;
        psi->Deserialize(cp);
        // TODO: errors
        ep->SetStop(psi);
      }
    }
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
    } else if (memcmp(line+1, "START ", 6) == 0) // slice
    { Start = line+7;
    } else if (memcmp(line+1, "STOP ", 6) == 0) // slice
    { Stop = line+6;
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
      const char* tokens[9] = {NULL};
      const char** tp = tokens;
      *tp = strtok(line+1, ",");
      while (*tp)
      { if (**tp == 0)
          *tp = NULL;
        *++tp = strtok(NULL, ",");
        if (tp == tokens + sizeof tokens/sizeof *tokens)
          break;
      }
      // 0: bitrate, 1: samplingrate, 2: channels, 3: file size, 4: total length,
      // 5: no. of song items, 6: total file size, 7: no. of items, 8: recursive. 
      DEBUGLOG(("Playlist::LSTReader::ParseLine: tokens %s, %s, %s, %s, %s, %s, %s, %s, %s\n", tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6], tokens[7], tokens[8]));
      // make tech info
      if (tokens[0] && tokens[4])
      { has_tech = true;
        // memset(Tech, 0, sizeof Tech);
        Tech.size       = sizeof Tech;
        Tech.bitrate    = atoi(tokens[0]);
        Tech.songlength = atof(tokens[4]);
        Tech.totalsize  = tokens[6] ? atof(tokens[6]) : -1;
        Tech.total_items = tokens[5] ? atoi(tokens[5]) : 1;
        Tech.recursive  = tokens[8] != NULL;
      }
      // make phys info
      if (tokens[3])
      { has_phys = true;
        // memset(Phys, 0, sizeof Tech);
        Phys.size       = sizeof Tech;
        Phys.filesize   = atof(tokens[3]);
        Phys.num_items  = tokens[7] ? atoi(tokens[7]) : 1;
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
  { char* cp = line + strspn(line, " \t");
    strchomp(cp);
    if (*cp == 0)
      continue;
    rdr.ParseLine(cp);
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
    char* cp = file + strspn(file, " \t");
    strchomp(cp);

    if( *cp!= 0 && *cp!= '#' && *cp!= '[' && *cp!= '>' && *cp!= '<' )
    {
      eq_pos = strchr( cp, '=' );

      if( eq_pos && strnicmp( cp, "File", 4 ) == 0 )
      {
        strcpy( cp, eq_pos + 1 );
        // TODO: fetch position
        AppendEntry(CreateEntry(cp));
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
    char* cp = line + strspn(line, " \t");
    strchomp(cp);

    if( *cp!= 0 && *cp != '#' && *cp != '[' && *cp != '>' && *cp != '<' )
    {
      eq_pos = strchr( cp, '=' );

      if( eq_pos ) {
        if( strnicmp( cp, "File", 4 ) == 0 )
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
          last_idx = atoi( &cp[4] );
        }
        else if( strnicmp( cp, "Title", 5 ) == 0 )
        {
          // We hope the title field always follows the file field
          if( last_idx == atoi( &cp[5] )) {
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
    // TODO: error handling should be up to the caller
    pm123_display_error(xstring::sprintf("Unable open playlist file:\n%s\n%s", GetURL().cdata(), xio_strerror(xio_errno())));
   else
  { if ( stricmp( ext, ".lst" ) == 0 || stricmp( ext, ".m3u" ) == 0 )
      rc = LoadLST(x);
    else if ( stricmp( ext, ".mpl" ) == 0 )
      rc = LoadMPL(x);
    else if ( stricmp( ext, ".pls" ) == 0 )
      rc = LoadPLS(x);
    else
      pm123_display_error(xstring::sprintf("Cannot determine playlist format from file extension %s.\n", ext.cdata()));

    PHYS_INFO phys;
    phys.size = sizeof phys;
    phys.filesize = xio_fsize(x);
    phys.num_items = 0;

    xio_fclose( x );
    
    // Count Items
    Entry* cur = Head;
    while (cur)
    { ++phys.num_items;
      cur = cur->Next;
    }
    // Write phys
    UpdateInfo(&phys);
  }
  if (Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = false;
  }
  return rc;
}

bool Playlist::InsertItem(const PlayableSlice& item, PlayableInstance* before)
{ DEBUGLOG(("Playlist(%p{%s})::InsertItem(%s, %p) - %u\n", this, GetURL().getShortName().cdata(), item.DebugName().cdata(), before, Stat));
  Mutex::Lock lock(Mtx);
  // Check whether the parameter before is still valid
  if (before && !before->IsParent(this))
    return false;
  // point of no return...
  Entry* ep = CreateEntry(item.GetPlayable()->GetURL());
  ep->SetAlias(item.GetAlias());
  if (item.GetStart())
    ep->SetStart(new SongIterator(*item.GetStart()));
  if (item.GetStop())
    ep->SetStop(new SongIterator(*item.GetStop()));
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
      DEBUGLOG(("Playlist::RemoveItem - %p\n", Head.get()));
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
    changed |= MoveEntry(current, rand() & 1 ? Head.get() : NULL);
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
  PHYS_INFO phys;
  phys.size = sizeof(phys);
  phys.filesize = -1;
  phys.num_items = 0;
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
        ++phys.num_items;
      }
      // next...
      count = sizeof result / sizeof(FILEFINDBUF3);
      rc = DosFindNext(hdir, &result, sizeof result, &count);
    } while (rc == 0);
    DosFindClose(hdir);
  }
  UpdateInfo(&phys);
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


