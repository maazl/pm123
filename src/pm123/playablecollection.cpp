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
#include "playablecollection.h"
#include "songiterator.h"
#include "properties.h"
#include "123_util.h"
#include "copyright.h"
#include <xio.h>
#include <strutils.h>
#include <snprintf.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <charset.h>

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

void PlayableSlice::Swap(PlayableSlice& r)
{ DEBUGLOG(("PlayableSlice(%p)::Swap(&%p)\n", this, &r));
  ASSERT(GetPlayable() == r.GetPlayable());
  Alias.swap(r.Alias);
  swap(Start, r.Start);
  swap(Stop, r.Stop);
}

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
  InUse(false)
{ DEBUGLOG(("PlayableInstance(%p)::PlayableInstance(%p{%s}, %p{%s})\n", this,
    &parent, parent.GetURL().getShortName().cdata(), playable, playable->GetURL().getShortName().cdata()));
}

xstring PlayableInstance::GetDisplayName() const
{ DEBUGLOG2(("PlayableInstance(%p)::GetDisplayName()\n", this));
  return GetAlias() ? GetAlias() : RefTo->GetDisplayName();
}

int PlayableInstance::CompareSliceBorder(const SongIterator* l, const SongIterator* r, SliceBorder type)
{ DEBUGLOG(("PlayableInstance::CompareSliceBorder(%p, %p, %i)\n", l, r, type));
  if (l == NULL)
  { if (r == NULL)
      return 0; // both iterators are zero
    else
      return type;
  } else
  { if (r == NULL)
      return -type;
    else
      return l->CompareTo(*r);
  }
}

void PlayableInstance::Swap(PlayableSlice& r)
{ DEBUGLOG(("PlayableInstance(%p)::Swap(&%p)\n", this, &r));
  ASSERT(Parent == ((PlayableInstance&)r).Parent);
  // Analyze changes
  StatusFlags events = SF_None;
  if (GetAlias() != r.GetAlias())
    events |= SF_Alias;
  if ( CompareSliceBorder(GetStart(), r.GetStart(), SB_Start)
    || CompareSliceBorder(GetStop(),  r.GetStop(),  SB_Stop) )
    events |= SF_Slice;
  // Execute changes
  if (events)
  { PlayableSlice::Swap(r);
    StatusChange(change_args(*this, events));
  }
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

void PlayableInstance::SetInUse(bool inuse)
{ DEBUGLOG(("PlayableInstance(%p)::SetInUse(%u) - %u\n", this, inuse, InUse));
  if (InUse != inuse)
  { InUse = inuse;
    RefTo->SetInUse(inuse);
    StatusChange(change_args(*this, SF_InUse));
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

void PlayableCollection::CollectionInfo::Add(T_TIME songlength, T_SIZE filesize, int items)
{ DEBUGLOG(("PlayableCollection::CollectionInfo::Add(%f, %f, %i) - %f, %f, %i\n", songlength, filesize, items, Songlength, Filesize, Items));
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


PlayableCollection::Entry::Entry(PlayableCollection& parent, Playable* playable, TDType::func_type tfn, IDType::func_type ifn)
: PlayableInstance(parent, playable),
  TechDelegate(parent, tfn),
  InstDelegate(parent, ifn)
{ DEBUGLOG(("PlayableCollection::Entry(%p)::Entry(&%p, %p, )\n", this, &parent, playable));
}

void PlayableCollection::Entry::Attach()
{ DEBUGLOG(("PlayableCollection::Entry(%p)::Attach()\n", this));
  GetPlayable()->InfoChange += TechDelegate;
  StatusChange += InstDelegate;
}

void PlayableCollection::Entry::Detach()
{ DEBUGLOG(("PlayableCollection::Entry(%p)::Detach()\n", this));
  TechDelegate.detach();
  InstDelegate.detach();
  Parent = NULL;
}


Mutex PlayableCollection::CollectionInfoCacheMtx;

PlayableCollection::PlayableCollection(const url123& URL, const DECODER_INFO2* ca)
: Playable(URL, ca),
  Modified(false),
  CollectionInfoCache(16)
{ DEBUGLOG(("PlayableCollection(%p)::PlayableCollection(%s, %p)\n", this, URL.cdata(), ca));
  // Always overwrite format info
  UpdateFormat(NULL, true);
}

PlayableCollection::~PlayableCollection()
{ DEBUGLOG(("PlayableCollection(%p{%s})::~PlayableCollection()\n", this, GetURL().getShortName().cdata()));
  // frist disable all events
  CollectionChange.reset();
  InfoChange.reset();
  // Cleanup SubInfoCache
  // We do not need to lock CollectionInfoCacheMtx here, because at the destructor the instance
  // is no longer shared between threads.
  for (CollectionInfoEntry** ppci = CollectionInfoCache.begin(); ppci != CollectionInfoCache.end(); ++ppci)
    delete *ppci;
}

Playable::Flags PlayableCollection::GetFlags() const
{ return Enumerable;
}

void PlayableCollection::InvalidateCIC(InfoFlags what, Playable& p)
{ DEBUGLOG(("PlayableCollection::InvalidateCIC(%x, &%p)\n", what, &p));
  /*if (RefreshActive)
  { // Invalidate CIC later to avoid O(n^2)
    ModifiedSubitems.get(p) = &p;
    return;
  }*/
                         
  Mutex::Lock lock(CollectionInfoCacheMtx);
  what = ~what; // prepare flags for deletion
  for (CollectionInfoEntry** ciepp = CollectionInfoCache.begin(); ciepp != CollectionInfoCache.end(); ++ciepp)
    // Only items that do not have the object in the exclusion list are invalidated to prevent recursions.
    if ((*ciepp)->find(p) == NULL)
      (*ciepp)->Valid &= what;
}/*
void PlayableCollection::InvalidateCIC(InfoFlags what, const PlayableSetBase& set)    
{ DEBUGLOG(("PlayableCollection::InvalidateCIC(%x, {%u,...})\n", what, set.size()));
  Mutex::Lock lock(CollectionInfoCacheMtx);
  what = ~what; // prepare flags for deletion
  for (CollectionInfoEntry** ciepp = CollectionInfoCache.begin(); ciepp != CollectionInfoCache.end(); ++ciepp)
    // Only items that do not have all objects in the exclusion list are invalidated to prevent recursions.
    if (!set.isSubsetOf(**ciepp))
      (*ciepp)->Valid &= what;
}*/

void PlayableCollection::ChildInfoChange(const Playable::change_args& args)
{ DEBUGLOG(("PlayableCollection(%p{%s})::ChildInfoChange({{%s},%x,})\n", this, GetURL().getShortName().cdata(),
    args.Instance.GetURL().getShortName().cdata(), args.Changed));
  InfoFlags f = args.Changed & (IF_Tech|IF_Rpl);
  if (f)
  { // Invalidate dependant info and reaload if already known
    InvalidateInfo(f, true);
    // Invalidate CollectionInfoCache entries.
    InvalidateCIC(f, args.Instance);
  }
}

void PlayableCollection::ChildInstChange(const PlayableInstance::change_args& args)
{ DEBUGLOG(("PlayableCollection(%p{%s})::ChildInstChange({{%s},%x})\n", this, GetURL().getShortName().cdata(),
    args.Instance.GetPlayable()->GetURL().getShortName().cdata(), args.Flags));
  if (args.Flags & PlayableInstance::SF_Slice)
    // Update dependant tech info, but only if already available
    // Same as TechInfoChange => emulate another event
    ChildInfoChange(Playable::change_args(*args.Instance.GetPlayable(), IF_Tech, IF_Tech));
}

PlayableCollection::Entry* PlayableCollection::CreateEntry(const char* url, const DECODER_INFO2* ca) const
{ DEBUGLOG(("PlayableCollection(%p{%s})::CreateEntry(%s, %p)\n",
    this, GetURL().getShortName().cdata(), url, ca));
  // now create the entry with absolute path
  int_ptr<Playable> pp = GetByURL(GetURL().makeAbsolute(url), ca);
  // initiate prefetch of information
  //pp->EnsureInfoAsync(IF_Status, true);
  return new Entry((PlayableCollection&)*this, pp, &PlayableCollection::ChildInfoChange, &PlayableCollection::ChildInstChange);
}

void PlayableCollection::UpdateModified(bool modified)
{ DEBUGLOG(("PlayableCollection::UpdateModified(%u) - %u\n", modified, Modified));
  //ASSERT(Mtx.GetStatus() > 0);
  bool changed = Modified != modified;
  Modified = modified;
  ValidateInfo(IF_Usage, changed, true);
}

bool PlayableCollection::UpdateListCore(Entry* cur_new, Entry*& first_new)
{ DEBUGLOG(("PlayableCollection::UpdateListCore(%p, &%p)\n", cur_new, first_new));
  Entry* cur_search = NULL;
  // Keep the first element for stop conditions below
  if (first_new == NULL)
    first_new = cur_new;
  for(;;)
  { cur_search = List.next(cur_search);
    // End of list of old items?
    if (cur_search == NULL || cur_search == first_new)
    { // No matching entry found => take the new one.
      InsertEntry(cur_new, NULL);
      return true;
    }
    // Is matching item?
    // We ignore the properties of PlayableInstance here.
    if (cur_search->GetPlayable() == cur_new->GetPlayable())
    { DEBUGLOG(("PlayableCollection::UpdateListCore match: %p\n", cur_search));
      // Match! => Swap properties
      // If the slice changes this way an ChildInstChange event is fired here that invalidates the CollectionInfoCache.
      cur_search->Swap(*cur_new);
      // If it happened to be the first new entry we have to update first_new too.
      if (cur_new == first_new)
        first_new = cur_search;
      // move entry to the new location
      return MoveEntry(cur_search, NULL);
    }
  }
}

bool PlayableCollection::PurgeUntil(Entry* keep_from)
{ DEBUGLOG(("PlayableCollection::PurgeUntil(%p)\n", keep_from));
  Entry* cur = List.next(NULL);
  if (cur == keep_from)
    return false;
  do
  { RemoveEntry(cur);
    cur = List.next(NULL);
  } while (cur != keep_from);
  return true;
}

void PlayableCollection::UpdateList(EntryList& newlist)
{ DEBUGLOG(("PlayableCollection(%p)::UpdateList({})\n", this));
  bool ret = false;
  Entry* first_new = NULL;
  int_ptr<Entry> cur_new;
  // Place new entries, try to recycle existing ones.
  while ((cur_new = newlist.pop_front()) != NULL)
    ret |= UpdateListCore(cur_new, first_new); 
  // Remove remaining old entries not recycled so far.
  // We loop here until we reach first_new. This /must/ be reached sooner or later.
  // first_new may be null if there are no new entries.
  ret |= PurgeUntil(first_new);
  if (ret)
  { ValidateInfo(IF_Other, true, true);
    UpdateModified(true);
  }
}
void PlayableCollection::UpdateList(const vector<PlayableInstance>& newlist)
{ DEBUGLOG(("PlayableCollection(%p)::UpdateList({})\n", this));
  bool ret = false;
  Entry* first_new = NULL;
  // Place new entries, try to recycle existing ones.
  for (PlayableInstance*const* cur = newlist.begin(); cur != newlist.end(); ++cur)
    ret |= UpdateListCore((Entry*)*cur, first_new);
  // Remove remaining old entries not recycled so far.
  // We loop here until we reach first_new. This /must/ be reached sooner or later.
  // first_new may be null if there are no new entries.
  ret |= PurgeUntil(first_new);
  if (ret)
  { ValidateInfo(IF_Other, true, true);
    UpdateModified(true);
  }
}

Playable::InfoFlags PlayableCollection::DoLoadInfo(InfoFlags what)
{ DEBUGLOG(("PlayableCollection(%p{%s})::DoLoadInfo(%x) - %u\n", this, GetURL().getShortName().cdata(), what, GetStatus()));
  // extend update bits
  { InfoFlags what2 = IF_None;
    if (what & (IF_Phys|IF_Tech))
      what2 |= CheckInfo(IF_Other);
    if (what & (IF_Other|IF_Phys))
      what2 |= IF_Other|IF_Phys;
    if (what & IF_Tech)
      what2 |= IF_Rpl;
    what |= Playable::BeginUpdate(what2);
  }
  // Step 1: load the list content
  if (what & (IF_Other|IF_Phys))
  { // load playlist content
    EntryList list;
    PHYS_INFO phys = { sizeof phys };
    Status stat;
    if (LoadList(list, phys))
      stat = STA_Valid;
    else
    { stat = STA_Invalid;
      if ((what & IF_Tech) == 0)
        what |= BeginUpdate(IF_Tech);
    }
    // update playable
    Lock lock(*this);
    if (what & IF_Other)
    { UpdateOther(stat, false, "PM123.EXE");
      UpdateList(list);
    }
    if (what & IF_Phys)
      UpdatePhys(&phys);
    UpdateModified(false);
    // End of step 1, raise the first events here.
    EndUpdate(what & (IF_Other|IF_Phys));
    what &= ~(IF_Other|IF_Phys);
  }

  // Stage 2: calculate information from sublist entries.
  if (what & (IF_Tech|IF_Rpl))
  { // Prefetch some infos before the mutex to avoid deadlocks with recursive playlists.
    PrefetchSubInfo(PlayableSet::Empty, (what & IF_Tech) != 0);
    Lock lock(*this);
    // update tech info
    TECH_INFO tech = { sizeof(TECH_INFO), -1, -1, -1, "" };
    RPL_INFO rpl = { sizeof(RPL_INFO) };
    if (GetStatus() > STA_Invalid)
    { // generate Info
      const CollectionInfo& ci = GetCollectionInfo(what);
      tech.songlength = ci.Songlength;
      tech.totalsize  = ci.Filesize;
      rpl.total_items = ci.Items;
      rpl.recursive   = ci.Excluded.find(*this) != NULL;
      if (what & IF_Tech)
      { // calculate dependant information in tech
        tech.bitrate = (int)( tech.totalsize >= 0 && tech.songlength > 0
          ? tech.totalsize / tech.songlength / (1000/8)
          : -1 );
        DEBUGLOG(("PlayableCollection::DoLoadInfo: %s {%g, %i, %g}\n",
          GetURL().getShortName().cdata(), tech.songlength, tech.bitrate, tech.totalsize));
        // Info string
        // Since all strings are short, there may be no buffer overrun.
        if (tech.songlength < 0)
          strcpy(tech.info, "unknown length");
         else
        { int days = (int)(tech.songlength / 86400.);
          double rem = tech.songlength - 86400.*days;
          int hours = (int)(rem / 3600.);
          rem -= 3600*hours;
          int minutes = (int)(rem / 60);
          rem -= 60*minutes;
          if (days)
            sprintf(tech.info, "Total: %ud %u:%02u:%02u", days, hours, minutes, (int)rem);
          else if (hours)
            sprintf(tech.info, "Total: %u:%02u:%02u", hours, minutes, (int)rem);
          else
            sprintf(tech.info, "Total: %u:%02u", minutes, (int)rem);
        }
        if (rpl.recursive)
          strcat(tech.info, ", recursion");
      }
    } else
    { // Invalid object
      strlcpy(tech.info, ErrorMsg.cdata(), sizeof tech.info);
    }
    if (what & IF_Tech)
      UpdateTech(&tech);
    if (what & IF_Rpl)
      UpdateRpl(&rpl);
  }
  DEBUGLOG(("PlayableCollection::DoLoadInfo completed: %x\n", what));
  return what;
}

void PlayableCollection::InsertEntry(Entry* entry, Entry* before)
{ DEBUGLOG(("PlayableCollection(%p{%s})::InsertEntry(%p{%s,%p,%p}, %p{%s})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), List.prev(entry), List.next(entry),
    before, before ? before->GetPlayable()->GetURL().cdata() : ""));
  // insert new item at the desired location
  entry->Attach();
  List.insert(entry, before);
  DEBUGLOG(("PlayableCollection::InsertEntry - before event\n"));
  InvalidateCIC(IF_All, *entry->GetPlayable());
  CollectionChange(change_args(*this, *entry, Insert));
  DEBUGLOG(("PlayableCollection::InsertEntry - after event\n"));
}

bool PlayableCollection::MoveEntry(Entry* entry, Entry* before)
{ DEBUGLOG(("PlayableCollection(%p{%s})::MoveEntry(%p{%s,%p,%p}, %p{%s})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), List.prev(entry), List.next(entry),
    before, (before ? before->GetPlayable()->GetURL() : url123::EmptyURL).getShortName().cdata()));
  if (!List.move(entry, before))
    return false;
  // raise event
  DEBUGLOG(("PlayableCollection::MoveEntry - before event\n"));
  CollectionChange(change_args(*this, *entry, Move));
  DEBUGLOG(("PlayableCollection::MoveEntry - after event\n"));
  return true;
}

void PlayableCollection::RemoveEntry(Entry* entry)
{ DEBUGLOG(("PlayableCollection(%p{%s})::RemoveEntry(%p{%s,%p,%p})\n", this, GetURL().getShortName().cdata(),
    entry, entry->GetPlayable()->GetURL().getShortName().cdata(), List.prev(entry), List.next(entry)));
  InvalidateCIC(IF_All, *entry->GetPlayable());
  CollectionChange(change_args(*this, *entry, Delete));
  DEBUGLOG(("PlayableCollection::RemoveEntry - after event\n"));
  ASSERT(entry->IsParent(this));
  entry->Detach();
  List.remove(entry);
}

int_ptr<PlayableInstance> PlayableCollection::GetPrev(const PlayableInstance* cur) const
{ DEBUGLOG(("PlayableCollection(%p)::GetPrev(%p{%s})\n", this, cur, cur ? cur->GetPlayable()->GetURL().cdata() : ""));
  ASSERT(!CheckInfo(IF_Other));
  // The PlayableInstance (if any) may either belong the the current list or be removed earlier.
  ASSERT(cur == NULL || cur->IsParent(NULL) || cur->IsParent(this));
  return List.prev((Entry*)cur);
}

int_ptr<PlayableInstance> PlayableCollection::GetNext(const PlayableInstance* cur) const
{ DEBUGLOG(("PlayableCollection(%p)::GetNext(%p{%s})\n", this, cur, cur ? cur->GetPlayable()->GetURL().cdata() : ""));
  ASSERT(!CheckInfo(IF_Other));
  // The PlayableInstance (if any) may either belong the the current list or be removed earlier.
  ASSERT(cur == NULL || cur->IsParent(NULL) || cur->IsParent(this));
  return List.next((Entry*)cur);
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

void PlayableCollection::PrefetchSubInfo(const PlayableSetBase& excluding, bool incl_songs)
{ DEBUGLOG(("PlayableCollection(%p{%s})::PrefetchSubInfo({%u,}, %u) - %x\n", this,
    GetURL().getShortName().cdata(), excluding.size(), incl_songs, CheckInfo(~IF_All)));
  EnsureInfo(IF_Other);
  int_ptr<PlayableInstance> pi;
  sco_ptr<PlayableSet> xcl_sub;
  while ((pi = GetNext(pi)) != NULL)
  { Playable* pp = pi->GetPlayable();
    if (pp->GetFlags() & Playable::Enumerable)
    { if (excluding.contains(*pp))
        continue; // recursion
      // create new exclusions on demand
      if (xcl_sub == NULL)
      { xcl_sub = new PlayableSet(excluding); // deep copy
        xcl_sub->get(*this) = this;
      }
      // prefetch sublists too.
      ((PlayableCollection*)pp)->PrefetchSubInfo(*xcl_sub, incl_songs);
    } else if (incl_songs)
      pp->EnsureInfo(IF_Tech|IF_Phys);
  }
}

const PlayableCollection::CollectionInfo& PlayableCollection::GetCollectionInfo(InfoFlags what, const PlayableSetBase& excluding)
{ DEBUGLOG(("PlayableCollection(%p{%s})::GetCollectionInfo(%x, [%s])\n", this, GetURL().getShortName().cdata(), what, excluding.DebugDump().cdata()));
  ASSERT(!excluding.contains(*this));
  what &= IF_Tech|IF_Rpl; // only these two flags are handled here.
  ASSERT(what);

  CollectionInfoEntry* cic;
  // cache lookup
  // We have to protect the CollectionInfoCache additionally by a critical section,
  // because the TechInfoChange event from the children cannot aquire the mutex.
  { Mutex::Lock lock(CollectionInfoCacheMtx);
    cic = CollectionInfoCache.find(excluding);
  }
  
  // We ensure some information on sub items before we request access to the mutex
  // to avoid deadlocks.
  if (cic == NULL)
    PrefetchSubInfo(excluding, (what & IF_Tech) != 0);
  
  // Lock the collection
  Lock lock(*this);
  if (cic == NULL) // double check below
  { // cache lookup
    // We have to protect the CollectionInfoCache additionally,
    // because the TechInfoChange event from the children must not aquire the instance mutex.
    Mutex::Lock lock(CollectionInfoCacheMtx);
    CollectionInfoEntry*& cicr = CollectionInfoCache.get(excluding);
    if (cicr == NULL)
      cicr = new CollectionInfoEntry(excluding);
    cic = cicr;
  }
  what &= ~cic->Valid;  
  if (!what)
  { DEBUGLOG(("PlayableCollection::GetCollectionInfo HIT: %x\n", cic->Valid));
    return cic->Info;
  }
  DEBUGLOG(("PlayableCollection::GetCollectionInfo LOAD: %x %x\n", cic->Valid, what));
  cic->Info.Reset();
    
  // (re)create information
  sco_ptr<PlayableSet> xcl_sub;
  PlayableInstance* pi = NULL;
  while ((pi = GetNext(pi)) != NULL)
  { DEBUGLOG(("PlayableCollection::GetCollectionInfo Item %p{%p{%s}}\n", pi, pi->GetPlayable(), pi->GetPlayable()->GetURL().cdata()));
    if (pi->GetPlayable()->GetFlags() & Playable::Enumerable)
    { // nested playlist
      PlayableCollection* pc = (PlayableCollection*)pi->GetPlayable();
      // TODO: support slicing!
      // check for recursion
      if (excluding.contains(*pc))
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
      cic->Info.Add(pc->GetCollectionInfo(what, *xcl_sub));

    } else if (what & IF_Tech)
    { // Song, but only if IF_Tech is requested
      Song* song = (Song*)pi->GetPlayable();
      DEBUGLOG(("PlayableCollection::GetCollectionInfo - Song, status %u\n", song->GetStatus()));
      if (song->GetStatus() == STA_Invalid)
      { // only count invalid items
        cic->Info.Add(0, 0, 1);
      } else
      { // take care of slice
        const DECODER_INFO2& info = song->GetInfo();
        T_TIME length = info.tech->songlength;
        DEBUGLOG(("PlayableCollection::GetCollectionInfo - len = %f\n", length));
        if (length <= 0)
        { cic->Info.Add(length, info.phys->filesize, 1);
          // TODO: use bitrate for slicing ???
        } else
        { if (pi->GetStop() && length > pi->GetStop()->GetLocation())
            length = pi->GetStop()->GetLocation();
          if (pi->GetStart())
          { if (pi->GetStart()->GetLocation() >= length)
              length = 0;
            else
              length -= pi->GetStart()->GetLocation();
          }
          // Scale filesize with slice to preserve bitrate.
          cic->Info.Add(length, info.phys->filesize * length / info.tech->songlength, 1);
        }
      }
    } else
    { // Song Item without IF_Tech => only count
      DEBUGLOG(("PlayableCollection::GetCollectionInfo - Song, no detail\n"));
      cic->Info.Add(0, 0, 1);
    }
    DEBUGLOG(("PlayableCollection::GetCollectionInfo - loop end\n"));
  }
  cic->Valid |= what | IF_Rpl; // IF_Rpl is always included
  DEBUGLOG(("PlayableCollection::GetCollectionInfo: {%f, %f, %i, {%u,...}}}\n",
    cic->Info.Songlength, cic->Info.Filesize, cic->Info.Items, cic->Info.Excluded.size()));
  return cic->Info;
}

bool PlayableCollection::InsertItem(const PlayableSlice& item, PlayableInstance* before)
{ DEBUGLOG(("PlayableCollection(%p{%s})::InsertItem(%s, %p{%s}) - %u\n", this, GetURL().getShortName().cdata(),
    item.DebugName().cdata(), before, before ? before->GetPlayable()->GetURL().cdata() : "", GetStatus()));
  Lock lock(*this);
  // Check whether the parameter before is still valid
  if (before && !before->IsParent(this))
    return false;
  InfoFlags what = BeginUpdate(IF_Other|IF_Phys|IF_Usage);
  if ((what & (IF_Other|IF_Phys)) != (IF_Other|IF_Phys))
  { EndUpdate(what); // Object locked by pending loadinfo
    return false;
  }
  // point of no return...
  Entry* ep = CreateEntry(item.GetPlayable()->GetURL());
  ep->SetAlias(item.GetAlias());
  if (item.GetStart())
    ep->SetStart(new SongIterator(*item.GetStart()));
  if (item.GetStop())
    ep->SetStop(new SongIterator(*item.GetStop()));
  PHYS_INFO phys = *GetInfo().phys;
  phys.filesize = -1;
  if (GetStatus() <= STA_Invalid)
    phys.num_items = 1;
  else
    ++phys.num_items;
  InsertEntry(ep, (Entry*)before);
  UpdateOther(STA_Valid, false, "PM123.EXE");
  ValidateInfo(IF_Other, true, true);
  UpdatePhys(&phys);
  UpdateModified(true);
  // update info
  InvalidateInfo(IF_Tech|IF_Rpl, true);
  // done!
  EndUpdate(what);
  return true;
}

bool PlayableCollection::MoveItem(PlayableInstance* item, PlayableInstance* before)
{ DEBUGLOG(("PlayableCollection(%p{%s})::InsertItem(%p{%s}, %p{%s}) - %u\n", this, GetURL().getShortName().cdata(),
    item, item->GetPlayable()->GetURL().cdata(), before ? before->GetPlayable()->GetURL().cdata() : ""));
  Lock lock(*this);
  // Check whether the parameter before is still valid
  if (!item->IsParent(this) || (before && !before->IsParent(this)))
    return false;
  InfoFlags what = BeginUpdate(IF_Other|IF_Usage);
  if ((what & IF_Other) == 0)
  { EndUpdate(what); // Object locked by pending loadinfo
    return false;
  }
  // Now move the entry.
  if (MoveEntry((Entry*)item, (Entry*)before))
  { ValidateInfo(IF_Other, true, true);
    UpdateModified(true);
  }
  // done!
  EndUpdate(what);
  return true;
}

bool PlayableCollection::RemoveItem(PlayableInstance* item)
{ DEBUGLOG(("PlayableCollection(%p{%s})::RemoveItem(%p{%s})\n", this, GetURL().getShortName().cdata(),
    item, item->GetPlayable()->GetURL().cdata()));
  Lock lock(*this);
  // Check whether the item is still valid
  if (item && !item->IsParent(this))
    return false;
  InfoFlags what = BeginUpdate(IF_Other|IF_Phys|IF_Usage);
  if ((what & (IF_Other|IF_Phys)) != (IF_Other|IF_Phys))
  { EndUpdate(what); // Object locked by pending loadinfo
    return false;
  }
  // now detach the item from the container
  RemoveEntry((Entry*)item);
  ValidateInfo(IF_Other, true, true);
  // Update Phys
  PHYS_INFO phys = *GetInfo().phys;
  phys.filesize = -1;
  --phys.num_items;
  UpdatePhys(&phys);
  UpdateModified(true);
  // update tech info
  InvalidateInfo(IF_Tech|IF_Rpl, true);
  // done
  EndUpdate(what);
  return true;
}

bool PlayableCollection::Clear()
{ DEBUGLOG(("PlayableCollection(%p{%s})::Clear()\n", this, GetURL().getShortName().cdata()));
  Lock lock(*this);
  InfoFlags what = BeginUpdate(IF_Other|IF_Phys|IF_Usage);
  if ((what & (IF_Other|IF_Phys)) != (IF_Other|IF_Phys))
  { EndUpdate(what); // Object locked by pending loadinfo
    return false;
  }
  // now detach the item from the container
  bool changed = false;
  while (!List.is_empty())
  { DEBUGLOG(("PlayableCollection::Clear - %p\n", List.next(NULL)));
    RemoveEntry(List.next(NULL));
    changed = true;
  }
  UpdateOther(STA_Valid, false, "PM123.EXE");
  ValidateInfo(IF_Other, true, true);
  // Update Phys
  PHYS_INFO phys = { sizeof phys };
  phys.filesize = -1;
  phys.num_items = 0;
  UpdatePhys(&phys);
  if (changed)
    UpdateModified(true);
  // update tech info
  InvalidateInfo(IF_Tech|IF_Rpl, true);
  // done
  EndUpdate(what);
  return true;
}

bool PlayableCollection::Sort(ItemComparer comp)
{ DEBUGLOG(("PlayableCollection(%p)::Sort(%p)\n", this, comp));
  Lock lock(*this);
  if (List.prev(NULL) == List.next(NULL))
    return true; // Empty or one element lists are always sorted.
  InfoFlags what = BeginUpdate(IF_Other|IF_Usage);
  if ((what & IF_Other) == 0)
  { EndUpdate(what); // Object locked by pending loadinfo
    return false;
  }
  // Create index array
  int n = GetInfo().phys->num_items;
  vector<PlayableInstance> index(n < 0 ? 64 : n);
  Entry* ep = NULL;
  while ((ep = List.next(ep)) != NULL)
    index.append() = ep;
  // Sort index array
  merge_sort(index.begin(), index.end(), comp);
  // Adjust item sequence
  UpdateList(index);
  // done
  EndUpdate(what);
  return true;
}

bool PlayableCollection::Shuffle()
{ DEBUGLOG(("PlayableCollection(%p)::Shuffle()\n", this));
  Lock lock(*this);
  if (List.prev(NULL) == List.next(NULL))
    return true; // Empty or one element lists are always sorted.
  InfoFlags what = BeginUpdate(IF_Other|IF_Usage);
  if ((what & IF_Other) == 0)
  { EndUpdate(what); // Object locked by pending loadinfo
    return false;
  }
  // Create index array
  int n = GetInfo().phys->num_items;
  if (n < 0)
    return false; // Can't Shuffle with unknown list size.
  vector<PlayableInstance> index(n);
  Entry* ep = NULL;
  while ((ep = List.next(ep)) != NULL)
    index.insert(rand() % (index.size()+1)) = ep;
  // Adjust item sequence
  UpdateList(index);
  // done
  EndUpdate(what);
  return true;
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
  Lock lock(*this);
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
    register const DECODER_INFO2& info = pp->GetInfo();
    if (pp->GetStatus() > STA_Invalid && pp->CheckInfo(IF_Tech|IF_Phys) == 0) // only if immediately available
    { char buf[50];
      xio_fputs("# ", of);
      if (info.phys->filesize < 0)
        xio_fputs("n/a", of);
      else
      { sprintf(buf, "%.0lfkiB", info.phys->filesize/1024.);
        xio_fputs(buf, of);
      }
      xio_fputs(", ", of);
      if (info.tech->songlength < 0)
        xio_fputs("n/a", of);
      else
      { unsigned long s = (unsigned long)info.tech->songlength;
        if (s < 60)
          sprintf(buf, "%lu s", s);
        else if (s < 3600)
          sprintf(buf, "%lu:%02lu", s/60, s%60);
        else
          sprintf(buf, "%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
        xio_fputs(buf, of);
      }
      xio_fputs(", ", of);
      xio_fputs(info.tech->info, of);
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
    InfoFlags infovalid = ~pp->CheckInfo(IF_All);
    if (pp->GetStatus() > STA_Invalid && (infovalid & (IF_Tech|IF_Format|IF_Phys|IF_Rpl)))
    { char buf[128];
      // 0: bitrate, 1: samplingrate, 2: channels, 3: file size, 4: total length,
      // 5: no. of song items, 6: total file size, 7: no. of items, 8: recursive.
      strcpy(buf, ">,");
      if (infovalid & IF_Tech)
        sprintf(buf + 1, "%i,", info.tech->bitrate);
      if (infovalid & IF_Format)
        sprintf(buf + strlen(buf), "%i,%i,", info.format->samplerate, info.format->channels);
      else
        strcat(buf + 2, ",,");
      if (infovalid & IF_Phys)
        sprintf(buf + strlen(buf), "%.0f,", info.phys->filesize);
      else
        strcat(buf + 4, ",");
      if (infovalid & IF_Tech)
        sprintf(buf + strlen(buf), "%.3f", info.tech->songlength);
      xio_fputs(buf, of);
      // Playlists only...
      if (pp->GetFlags() & Enumerable)
      { strcpy(buf, ",");
        if (infovalid & IF_Rpl)
          sprintf(buf + 1, ",%i", info.rpl->total_items);
        if (infovalid & IF_Tech)  
          sprintf(buf + strlen(buf), ",%.0f", info.tech->totalsize);
        else
          strcat(buf + 1, ",");
        if (infovalid & IF_Phys)
          sprintf(buf + strlen(buf), ",%i", info.phys->num_items);
        if (infovalid & IF_Rpl && info.rpl->recursive)
          strcat(buf + 2, ",1");
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
  Lock lock(*this);
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
  XFILE* of = xio_fopen(URL, "wU");
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
  if (pp && pp->GetFlags() & Enumerable)
  { if (pp == this)
    { Lock lock(*this);
      ASSERT(BeginUpdate(IF_Usage));
      UpdateModified(false);
      EndUpdate(IF_Usage);
    } else
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
  memset(&Info, 0, sizeof Info);
  Info.size = sizeof Info;
  Alias = NULL;
  Start = NULL;
  Stop  = NULL;
  URL   = NULL;
}

void Playlist::LSTReader::Create()
{ DEBUGLOG(("Playlist::LSTReader::Create() - %p, {%p, %p, %p, %p, %p}\n",
    URL.cdata(), Info.format, Info.tech, Info.tech, Info.phys, Info.rpl));
  if (URL)
  { Entry* ep = Parent.CreateEntry(URL, &Info);
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
    Dest.push_back(ep);
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
          // has_techinfo = true; ... only if further info below is available too
      } }
    }
    break;

   default:
    // close last URL
    Create();
    if (URL)
      Reset();
    URL = Parent.GetURL().makeAbsolute(line);
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
      DEBUGLOG(("Playlist::LSTReader::ParseLine: tokens %s, %s, %s, %s, %s, %s, %s, %s, %s\n",
        tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6], tokens[7], tokens[8]));
      // Data type conversion
      
      // make tech info
      if (tokens[0] && tokens[4])
      { Info.tech = &Tech;
        // memset(Tech, 0, sizeof Tech);
        Tech.size       = sizeof Tech;
        Tech.bitrate    = atoi(tokens[0]);
        Tech.songlength = atof(tokens[4]);
        Tech.totalsize  = tokens[6] ? atof(tokens[6]) : -1;
      }
      // make playlist info
      if (tokens[5])
      { Info.rpl = &Rpl;
        // memset(Tech, 0, sizeof Tech);
        Rpl.size        = sizeof Rpl;
        Rpl.total_items = atoi(tokens[5]);
        Rpl.recursive   = tokens[8] != NULL;
      }
      // make phys info
      if (tokens[3])
      { Info.phys = &Phys;
        // memset(Phys, 0, sizeof Tech);
        Phys.size       = sizeof Phys;
        Phys.filesize   = atof(tokens[3]);
        Phys.num_items  = tokens[7] ? atoi(tokens[7]) : 1;
      }
      // make format info
      if (tokens[1] && tokens[2])
      { Info.format = &Format;
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

bool Playlist::LoadLST(EntryList& list, XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadLST(%p)\n", this, GetURL().getShortName().cdata(), x));

  // TODO: fixed buffers...
  char line[4096];
  LSTReader rdr(*this, list);

  while (xio_fgets(line, sizeof(line), x))
  { char* cp = line + strspn(line, " \t");
    strchomp(cp);
    if (*cp == 0)
      continue;
    rdr.ParseLine(cp);
  }
  return true;
}

bool Playlist::LoadM3U8(EntryList& list, XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadM38U(%p)\n", this, GetURL().getShortName().cdata(), x));

  // TODO: fixed buffers...
  char line_raw[4096];
  char line[4096];
  static const char bom[3] = { 0xEF, 0xBB, 0xBF };
  LSTReader rdr(*this, list);

  while (xio_fgets(line_raw, sizeof(line_raw), x))
  { char* cp = line_raw;
    // Skip BOM?
    if (memcmp(cp, bom, 3) == 0)
      cp += 3;
    // convert to system codepage
    if (ch_convert(1208, cp, 0, line, sizeof line, 0) == NULL)
      return false;
    // and now businness as usual
    cp = line + strspn(line, " \t");
    strchomp(cp);
    if (*cp == 0)
      continue;
    rdr.ParseLine(cp);
  }
  return true;
}

bool Playlist::LoadMPL(EntryList& list, XFILE* x)
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
        list.push_back(CreateEntry(cp));
      }
    }
  }
  return true;
}

bool Playlist::LoadPLS(EntryList& list, XFILE* x)
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
            list.push_back(ep);
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
    list.push_back(ep);
  }

  return true;
}

bool Playlist::LoadList(EntryList& list, PHYS_INFO& phys)
{ DEBUGLOG(("Playlist(%p{%s})::LoadList()\n", this, GetURL().getShortName().cdata()));

  bool rc = false;
  xstring ext = GetURL().getExtension();

  // open file
  XFILE* x = xio_fopen(GetURL(), "rU");
  if (x == NULL)
  { ErrorMsg = xstring::sprintf("Unable open file:\n%s\n%s", GetURL().cdata(), xio_strerror(xio_errno()));
  } else
  { if ( stricmp( ext, ".lst" ) == 0 || stricmp( ext, ".m3u" ) == 0 )
      rc = LoadLST(list, x);
    else if ( stricmp( ext, ".m3u8" ) == 0 )
      rc = LoadM3U8(list, x);
    else if ( stricmp( ext, ".mpl" ) == 0 )
      rc = LoadMPL(list, x);
    else if ( stricmp( ext, ".pls" ) == 0 )
      rc = LoadPLS(list, x);
    else
      ErrorMsg = xstring::sprintf("Cannot determine playlist format from file extension %s.\n", ext.cdata());

    phys.filesize = xio_fsize(x);
    xio_fclose( x );
    
    // Count Items
    phys.num_items = 0;
    Entry* cur = NULL;
    while ((cur = list.next(cur)) != NULL)
      ++phys.num_items;
  }
  return rc;
}

void Playlist::NotifySourceChange()
{ DEBUGLOG(("Playlist(%p{%s})::NotifySourceChange()\n", this, GetURL().cdata()));
  Lock lock(*this);
  ASSERT(BeginUpdate(IF_Usage));
  UpdateModified(true);
  EndUpdate(IF_Usage);
}


/****************************************************************************
*
*  class PlayFolder
*
****************************************************************************/

PlayFolder::PlayFolder(const url123& URL, const DECODER_INFO2* ca)
: PlayableCollection(URL, ca),
  Recursive(false),
  SortMode(0),
  FoldersFirst(0)
{ DEBUGLOG(("PlayFolder(%p)::PlayFolder(%s, %p)\n", this, URL.cdata(), ca));
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
      Pattern = arg.cdata() +8;
    else if (arg.startsWithI("recursive"))
      Recursive = true;
    else if (arg.startsWithI("sort="))
      SortMode = (signed char)atoi(arg.cdata() +5);
    else if (arg.startsWithI("foldersfirst="))
      FoldersFirst = (signed char)atoi(arg.cdata() +13);
    else
      DEBUGLOG(("PlayFolder::ParseQueryParams: invalid option %s\n", arg.cdata()));
    if (cp2 == NULL)
      break;
    cp = cp2 +1;
  }
  DEBUGLOG(("PlayFolder::ParseQueryParams: %s %u\n", Pattern ? Pattern.cdata() : "<null>", Recursive));
}

int PlayFolder::CompName(const PlayableInstance* l, const PlayableInstance* r)
{ // map to IComparableTo<const char*>
  return l->GetPlayable()->compareTo(r->GetPlayable()->GetURL().cdata());
}

int PlayFolder::CompNameFoldersFirst(const PlayableInstance* l, const PlayableInstance* r)
{ int d = (r->GetPlayable()->GetFlags() & Playable::Mutable & ~Playable::Enumerable)
        - (l->GetPlayable()->GetFlags() & Playable::Mutable & ~Playable::Enumerable);
  return d != 0 ? d : CompName(l, r);
}

bool PlayFolder::LoadList(EntryList& list, PHYS_INFO& phys)
{ DEBUGLOG(("PlayFolder(%p{%s})::LoadList(,)\n", this, GetURL().getShortName().cdata()));
  if (!GetURL().isScheme("file:")) // Can't handle anything but filesystem folders so far.
    return false;

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
  switch (rc)
  {default:
    { char buf[80];
      os2_strerror(rc, buf, sizeof buf);
      ErrorMsg = xstring::sprintf("Error %ul: %s\n%s", rc, buf, GetURL().cdata());
      break;
    } 
   case NO_ERROR:
    phys.filesize = -1;
    phys.num_items = 0;
    do
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
        list.push_back(ep);
        ++phys.num_items;
      }
      // next...
      count = sizeof result / sizeof(FILEFINDBUF3);
      rc = DosFindNext(hdir, &result, sizeof result, &count);
    } while (rc == 0);
    DosFindClose(hdir);
   case ERROR_NO_MORE_FILES:
   case ERROR_FILE_NOT_FOUND:;
  }
  DEBUGLOG(("PlayFolder::LoadList: %u\n", rc));
  switch (rc)
  {case NO_ERROR:
   case ERROR_FILE_NOT_FOUND:
   case ERROR_NO_MORE_FILES:
    if (SortMode + cfg.sort_folders > 0)
      Sort(FoldersFirst + cfg.folders_first > 0 ? &PlayFolder::CompNameFoldersFirst : &PlayFolder::CompName);
    return true;
   default:
    return false;
  }
}

