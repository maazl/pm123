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

#define INCL_WIN
#define INCL_BASE
#include "songiterator.h"
#include "properties.h"

#include <cpp/mutex.h>
#include <cpp/xstring.h>
#include <strutils.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>


/****************************************************************************
*
*  class SongIterator
*
****************************************************************************/

void SongIterator::Offsets::Add(int index, T_TIME offset)
{ Index += index;
  if (Offset >= 0)
  { if (offset < 0)
      Offset = -1;
    else
      Offset += offset;
  }
}

void SongIterator::Offsets::Sub(int index, T_TIME offset)
{ Index -= index;
  if (Offset >= 0)
  { if (offset < 0)
      Offset = -1;
    else
      Offset -= offset;
  }
}

SongIterator::CallstackEntry::CallstackEntry(const PlayableSet& exc)
: OffValid(true),
  Exclude(exc)
{ DEBUGLOG(("SongIterator::CallstackEntry(%p)::CallstackEntry(&%p{%u,...})\n", this, &exc, exc.size()));
}

SongIterator::CallstackEntry::CallstackEntry(const CallstackEntry& r)
: Item(r.Item),
  Off(r.Off),
  OffValid(r.OffValid),
  Exclude(r.Exclude)
{ DEBUGLOG(("SongIterator::CallstackEntry(%p)::CallstackEntry(&%p)\n", &r));
  if (r.OverrideStart != NULL)
    OverrideStart = new SongIterator(*r.OverrideStart);
  if (r.OverrideStop != NULL)
    OverrideStop = new SongIterator(*r.OverrideStop);
}

void SongIterator::CallstackEntry::Swap(CallstackEntry& r)
{ Item.swap(r.Item);
  swap(Off, r.Off);
  swap((bool&)OffValid, (bool&)r.OffValid);
  Exclude.swap(r.Exclude);
  OverrideStart.swap(r.OverrideStart);
  OverrideStop.swap(r.OverrideStop);
}

void SongIterator::CallstackEntry::Reset()
{ Item          = NULL;
  Off.Index     = 0;
  Off.Offset    = 0;
  OverrideStart = NULL;
  OverrideStop  = NULL;
}

void SongIterator::CallstackEntry::Assign(PlayableSlice* pc, Playable* exclude)
{ DEBUGLOG(("SongIterator::CallstackEntry(%p)::Assign(%p, %p)\n", this, pc, exclude));
  Item          = pc;
  Off.Index     = 0;
  Off.Offset    = 0;
  OverrideStart = NULL;
  OverrideStop  = NULL;
  Exclude.clear();
  if (exclude)
    Exclude.append() = exclude;
}

PlayableCollection::CollectionInfo SongIterator::CallstackEntry::GetInfo() const
{ PlayableCollection::CollectionInfo ret;
  ret.Filesize = -1; // not supported by this function
  const SongIterator* si = GetStop();
  if (si)
  { // use stop offset
    const Offsets& off = si->GetOffset(true);
    ret.Items = off.Index;
    ret.Songlength = off.Offset;
  } else
  { // use total length
    Playable* pp = Item->GetPlayable();
    if (pp->GetFlags() & Playable::Enumerable)
      ret = ((PlayableCollection*)pp)->GetCollectionInfo(Playable::IF_Tech|Playable::IF_Rpl, Exclude);
    else
    { // Song => use tech info
      ret.Items = 1;
      ret.Songlength = pp->GetInfo().tech->songlength;
  } }
  // Apply Start
  si = GetStart();
  if (si)
  { // subtract start offset
    const Offsets& off = si->GetOffset(true);
    if (ret.Items >= 0)
    { if (off.Index < 0)
        ret.Items = -1;
      else
        ret.Items -= off.Index;
    }
    if (ret.Songlength >= 0)
    { if (off.Offset < 0)
        ret.Songlength = -1;
      else
        ret.Songlength -= off.Offset;
    }
  }
  return ret;
}

SongIterator::CallstackSubFactory::CallstackSubFactory(const CallstackEntry& parent)
: SongIterator::CallstackEntry(parent.GetExclude())
{ ASSERT(parent.Item);
  Playable* pp = parent.Item->GetPlayable();
  ASSERT(pp->GetFlags() & Playable::Enumerable);
  Exclude.get(*pp) = pp;
}

SongIterator::CallstackSubEntry::CallstackSubEntry(const CallstackSubEntry& r, SongIterator& owner)
: SongIterator::CallstackSubFactory(r),
  ChangeDeleg(*r.ChangeDeleg.get_event(), owner, &SongIterator::ListChangeHandler, this)
{}

SongIterator::CallstackSubEntry::CallstackSubEntry(SongIterator& owner, const CallstackEntry& parent)
: SongIterator::CallstackSubFactory(parent),
  ChangeDeleg(((PlayableCollection&)*parent.Item->GetPlayable()).CollectionChange, owner, &SongIterator::ListChangeHandler, this)
{}


// In fact static constructor of SongIterator::InitialCallstack.
SongIterator::InitialCallstackType::InitialCallstackType()
: SongIterator::CallstackType(1)
{ // Always create a top level entry for Current.
  append() = new CallstackEntry();
  // Ensure that this item is NEVER deleted
  int_ptr<InitialCallstackType> ptr = this;
  ptr.toCptr();
}

SongIterator::InitialCallstackType SongIterator::InitialCallstack;


SongIterator::SongIterator()
: Callstack(&InitialCallstack),
  Location(0),
  CurrentCacheValid(false)
{ DEBUGLOG(("SongIterator(%p)::SongIterator()\n", this));
}

// identical to the standard copy constructorr so far
SongIterator::SongIterator(const SongIterator& r)
: Callstack(r.Callstack), // reference the other callstack until changes apply
  Location(r.Location),
  CurrentCache(r.CurrentCacheValid ? r.CurrentCache.get() : NULL), // CurrentCache is also valid until changes apply
  CurrentCacheValid(r.CurrentCacheValid)
{ DEBUGLOG(("SongIterator(%p)::SongIterator(&%p)\n", this, &r));
}

SongIterator::SongIterator(const SongIterator& r, unsigned slice)
: Location(r.Location),
  CurrentCache(r.CurrentCacheValid ? r.CurrentCache.get() : NULL), // Even with slicing CurrentCache is also valid until changes apply
  CurrentCacheValid(r.CurrentCacheValid)
{ DEBUGLOG(("SongIterator(%p)::SongIterator(&%p, %u)\n", this, &r, slice));
  ASSERT(slice <= r.Callstack->size());
  if (slice)
  { Callstack = new CallstackType(r.Callstack->size() - slice > 4 ? r.Callstack->size() - slice + 4 : 8);
    // copy callstack
    const CallstackEntry*const* ppce = r.Callstack->begin() + slice;
    Callstack->append() = new CallstackEntry(**ppce);
    while (++ppce != r.Callstack->end())
      Callstack->append() = new CallstackSubEntry((CallstackSubEntry&)**ppce, *this);
    // TODO: do we need to remove the non copied callstack entries from exclude?
  } else
    Callstack = r.Callstack;
}

SongIterator::~SongIterator()
{ DEBUGLOG(("SongIterator(%p)::~SongIterator()\n", this));
}

// Identical to the standard assignment operator so far
SongIterator& SongIterator::operator=(const SongIterator& r)
{ DEBUGLOG2(("SongIterator(%p)::operator=(%s)\n", this, r.Serialize().cdata()));
  Callstack    = r.Callstack; // reference the other callstack until changes apply
  Location     = r.Location;
  CurrentCache = r.CurrentCacheValid ? r.CurrentCache.get() : NULL; // CurrentCache is also valid until changes apply
  CurrentCacheValid = r.CurrentCacheValid;
  return *this;
}

void SongIterator::Swap(SongIterator& r)
{ Callstack.swap(r.Callstack);
  swap(Location, r.Location);
  CurrentCache.swap(r.CurrentCache);
  swap(CurrentCacheValid, r.CurrentCacheValid);
}

#ifdef DEBUG
xstring SongIterator::DebugName(const SongIterator* sip)
{ return sip ? sip->Serialize() : xstring::empty;
}

/*void SongIterator::DebugDump() const
{ DEBUGLOG(("SongIterator(%p{%p, {%u }, {%u }})::DebugDump()\n", this, &*Root, Callstack.size(), Exclude.size()));
  for (const CallstackEntry*const* ppce = Callstack.begin(); ppce != Callstack.end(); ++ppce)
    DEBUGLOG(("SongIterator::DebugDump %p{%i, %g, %p}\n", *ppce, (*ppce)->Index, (*ppce)->Offset, &*(*ppce)->Item));
  Exclude.DebugDump();
}*/
#endif

void SongIterator::MakeCallstackUnique()
{ if (Callstack->RefCountIsUnique())
    return;
  // create new callstack
  CallstackType* newstack = new CallstackType(Callstack->size() > 8 ? Callstack->size() : 8);
  // copy callstack
  const CallstackEntry*const* ppce = Callstack->begin();
  newstack->append() = new CallstackEntry(**ppce);
  while (++ppce != Callstack->end())
    newstack->append() = new CallstackSubEntry((CallstackSubEntry&)**ppce, *this);
  // replace by unique copy
  Callstack = newstack;
}

void SongIterator::Reset()
{ DEBUGLOG(("SongIterator(%p)::Reset()\n", this));
  // Is there something to reset?
  if (Callstack->size() > 1)
  { MakeCallstackUnique();
    // destroy callstack entries except for the root.
    do
      delete Callstack->erase(Callstack->size()-1);
    while (Callstack->size() > 1);
  }
  InvalidateCurrentCache();
}

void SongIterator::SetRoot(PlayableSlice* pc, Playable* exclude)
{ DEBUGLOG(("SongIterator(%p)::SetRoot(%p, %p)\n", this, pc, exclude));
  MakeCallstackUnique();
  Reset();
  (*Callstack)[0]->Assign(pc, exclude);
}

PlayableSlice* SongIterator::GetCurrent() const
{ if (!CurrentCacheValid)
  { // The cache is not valid => fill it, but cause as few reallocations as possible.
    const CallstackEntry* cep = (*Callstack)[Callstack->size()-1];
    if (cep->OverrideStart != NULL || cep->OverrideStop != NULL || Location != 0)
    { // We have to create a virtual slice
      // Make new or unique current cache that belongs to the current playable
      if (!CurrentCache || !CurrentCache->RefCountIsUnique() || CurrentCache->GetPlayable() != cep->Item->GetPlayable())
        // Modify mutable object
        ((SongIterator*)this)->CurrentCache = new PlayableSlice(cep->Item->GetPlayable());
      CurrentCache->SetAlias(cep->Item->GetAlias());
      const SongIterator* si = cep->GetStart();
      if (Location || si)
      { // Dirty hack: we modify CurrentCache->Start by the backdoor to avoid unneccessary reallocations.
        SongIterator* si2 = (SongIterator*)CurrentCache->GetStart();
        if (si)
        { if (si2)
            *si2 = *si;
          else
          { si2 = new SongIterator(*si);
            CurrentCache->SetStart(si2);
          }
        } else
        { if (!si2) 
          { si2 = new SongIterator();
            si2->SetRoot(new PlayableSlice(CurrentCache->GetPlayable()));
            CurrentCache->SetStart(si2);
          } else
            si2->SetRoot(new PlayableSlice(CurrentCache->GetPlayable()));
        }
        si2->SetLocation(Location);
      } else
        CurrentCache->SetStart(NULL);
      si = cep->GetStop();
      if (si)
      { // Dirty hack: we modify CurrentCache->Stop by the backdoor to avoid unneccessary reallocations.
        SongIterator* si2 = (SongIterator*)CurrentCache->GetStop();
        if (si2)
          *si2 = *si;
        else
          CurrentCache->SetStop(new SongIterator(*si));
      } else
        CurrentCache->SetStop(NULL);
    } else
      ((SongIterator*)this)->CurrentCache = cep->Item;
  }
  return CurrentCache;
}

void SongIterator::Enter()
{ DEBUGLOG(("SongIterator::Enter()\n"));
  PlayableSlice* psp = Current(); // The ownership is held anyway by the callstack object.
  ASSERT(psp != NULL);
  ASSERT(psp->GetPlayable()->GetFlags() & Playable::Enumerable);
  // We must do this in two steps, because of the side effects of Callstack.append().
  CallstackEntry* cep = (*Callstack)[Callstack->size()-1];
  Callstack->append() = new CallstackSubEntry(*this, *cep);
  Location = 0;
}

void SongIterator::Leave()
{ DEBUGLOG(("SongIterator::Leave()\n"));
  ASSERT(Callstack->size() > 1); // Can't remove the last item.
  delete Callstack->erase(Callstack->size()-1);
  Location = 0;
}

int SongIterator::GetLevel() const
{ return Current() ? Callstack->size() : Callstack->size()-1;
}

PlayableCollection* SongIterator::GetList() const
{ DEBUGLOG(("SongIterator(%p)::GetList()\n", this));
  PlayableSlice* ps = Current();
  if (!ps)
    return NULL;
  if (ps->GetPlayable()->GetFlags() & Playable::Enumerable)
    return (PlayableCollection*)ps->GetPlayable();
  if (Callstack->size() == 1) // no playlist in the callstack
    return NULL;
  return (PlayableCollection*)(*Callstack)[Callstack->size()-2]->Item->GetPlayable();
}

void SongIterator::PrevCore()
{ DEBUGLOG(("SongIterator(%p)::PrevCore()\n", this));
  ASSERT(Callstack->size() > 1);
  CallstackEntry* const pce  = (*Callstack)[Callstack->size()-1]; // The item
  CallstackEntry* const pce2 = (*Callstack)[Callstack->size()-2]; // The playlist
  pce->OverrideStart = NULL;
  Location = 0;
  if (pce->Item == NULL)
  { // Navigate to the last item.
    // Check stop positions first.
    const SongIterator* stop = pce2->GetStop();
    if (stop && stop->GetCallstack().size() >= 2)
    { // We have a start iterator => start from there
      pce->Item = stop->GetCallstack()[1]->Item;
      // Check wether we have to override the stop iterator of the item.
      if (!pce->Item->GetStop() || stop->CompareTo(*pce->Item->GetStop(), 1) < 0)
        pce->OverrideStop = new SongIterator(*stop, 1);
      // We explicitely do not take the Location property here.
    } else
    { // move to the last item
      pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetPrev(NULL);
    }
    if (pce->OffValid)
    { // calc from the end
      pce->Off.Reset();
      // += length(parent)
      pce->Off += pce2->GetInfo();
      // -= length(current)
      pce->Off -= pce->GetInfo();
    }
  } else
  { // The fact that we had a overridden start iterator
    // is suficcient to know that we do not need to look for further items.
    if (pce->OverrideStart != NULL)
    { pce->Reset();
      return;
    }
    // move to the previous item
    pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetPrev((PlayableInstance*)pce->Item.get());
    // calc offsets
    if (pce->OffValid && pce->Item)
      pce->Off -= pce->GetInfo();
  }

  // Skip objects in the callstack
  while (pce->Item && IsInCallstack(pce->Item->GetPlayable()))
  { pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetPrev((PlayableInstance*)pce->Item.get());
    // TODO: Update overridden stop iterator if any
  }

  pce->OverrideStart = NULL;
  if (pce->Item != NULL)
  { // Check wether we have to override the start iterator
    const SongIterator* start = pce2->GetStart();
    if (start && start->GetCallstack().size() >= 2)
    { // We have a start iterator => check start condition
      int cmp = CompareTo(*start, Callstack->size() - 2);
      if (cmp < 0)
      { // start iterator crossed => end
        pce->Reset();
        return;
      }
      // Check wether we have to override the start iterator of the item.
      if (cmp > (int)Callstack->size() - 2)
      { // We are greater in a subitem of the current location.
        // But we need to override start only if the current item has no start point
        // greater than or equal to start.
        if (!pce->Item->GetStart() || start->CompareTo(*pce->Item->GetStart(), 1) > 0)
          pce->OverrideStart = new SongIterator(*start, 1);
      }
    }

    // set Location in case of a song
    { const SongIterator* start = pce->GetStart();
      if (start && start->GetCallstack().size() == 1)
        Location = start->GetLocation();
    }
  }
}

void SongIterator::NextCore()
{ DEBUGLOG(("SongIterator(%p)::NextCore()\n", this));
  ASSERT(Callstack->size() > 1);
  CallstackEntry* const pce  = (*Callstack)[Callstack->size()-1]; // The item
  CallstackEntry* const pce2 = (*Callstack)[Callstack->size()-2]; // The playlist
  Location = 0;
  pce->OverrideStart = NULL;
  if (pce->Item == NULL)
  { // Navigate to the first item.
    const SongIterator* start = pce2->GetStart();
    if (start && start->GetCallstack().size() >= 2)
    { // We have a start iterator => start from there
      pce->Item = start->GetCallstack()[1]->Item;
      // Check wether we have to override the start iterator of the item.
      if (!pce->Item->GetStart() || start->CompareTo(*pce->Item->GetStart(), 1) > 0)
        pce->OverrideStart = new SongIterator(*start, 1);
    } else
    { // move to the first item
      pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetNext(NULL);
    }
    // clear offsets
    pce->Off.Reset();
    pce->OffValid = true; // this is cheap
  } else
  { // The fact that we had a overridden stop iterator
    // is suficcient to know that we do not need to look for further items.
    if (pce->OverrideStop != NULL)
    { pce->Reset();
      return;
    }
    // calc offsets
    if (pce->OffValid)
      pce->Off += pce->GetInfo();
    // move to the next item
    pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetNext((PlayableInstance*)pce->Item.get());
  }

  // Skip objects in the callstack
  while (pce->Item && IsInCallstack(pce->Item->GetPlayable()))
  { pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetNext((PlayableInstance*)pce->Item.get());
    // TODO: Update overridden start iterator if any
  }

  pce->OverrideStop = NULL;
  if (pce->Item != NULL)
  { // Override stop if the stop iterator of the parent callstack entry 
    // is less than the end of the current item. Signal the end of the list
    // if it is less than the start of the current item.

    // set Location in case of a song
    { const SongIterator* start = pce->GetStart();
      if (start && start->GetCallstack().size() == 1)
        Location = start->GetLocation();
    }
    // Check wether we have to override the stop iterator
    const SongIterator* stop = pce2->GetStop();
    if (stop && stop->GetCallstack().size() >= 2)
    { // We have a stop iterator => check stop condition
      int cmp = CompareTo(*stop, Callstack->size() - 2);
      if (cmp >= 0)
      { // stop iterator crossed => end
        pce->Reset();
        return;
      }
      // Check wether we have to override the stop iterator of the item.
      if (cmp < -(int)(Callstack->size() - 2))
      { // We are less in a subitem of the current location.
        // But we need to override stop only if the current item has no stop point
        // less than or equal to stop.
        if (!pce->Item->GetStop() || stop->CompareTo(*pce->Item->GetStop(), 1) < 0)
          pce->OverrideStop = new SongIterator(*stop, 1);
      }
    }
  }
}

bool SongIterator::PrevNextCore(void (SongIterator::*func)(), unsigned slice)
{ DEBUGLOG(("SongIterator(%p)::PrevNextCore(, %u) - %u\n", this, slice, Callstack->size()));
  ASSERT(Callstack->size() >= slice);
  ASSERT(GetList());
  MakeCallstackUnique();
  InvalidateCurrentCache();
  // Do we have to enter a List first?
  if (Current()->GetPlayable()->GetFlags() & Playable::Enumerable)
  { Current()->GetPlayable()->EnsureInfo(Playable::IF_Other);
    Enter();
  }
  for (;;)
  { (this->*func)();
    CallstackEntry* pce = (*Callstack)[Callstack->size()-1];
    const PlayableSlice* pi = pce->Item;
    DEBUGLOG(("SongIterator::PrevNextCore - {%i, %f, %p, %p, %p}\n", pce->Off.Index, pce->Off.Offset, pi, pce->OverrideStart.get(), pce->OverrideStop.get()));
    if (pi == NULL)
    { // end of list => leave
      Leave();
      if (Callstack->size()-1 == slice)
      { DEBUGLOG(("SongIterator::PrevNextCore - NULL\n"));
        return false;
      }
    } else
    { // item found => check wether it is enumerable
      if (!(pi->GetPlayable()->GetFlags() & Playable::Enumerable))
      { Location = 0;
        DEBUGLOG(("SongIterator::PrevNextCore - {%i, %f, %p{%p{%s}}}\n", pce->Off.Index, pce->Off.Offset, &*pi, pi->GetPlayable(), pi->GetPlayable()->GetURL().getShortName().cdata()));
        return true;
      } else
      { pi->GetPlayable()->EnsureInfo(Playable::IF_Other);
        Enter();
      }
    }
  }
}

void SongIterator::EnsureOffset(CallstackEntry& ce, PlayableCollection& parent)
{ DEBUGLOG(("SongIterator::EnsureOffset(&%p, &%p)\n", &ce, &parent));
  while (!ce.OffValid)
  {restart:
    // recalculate offset
    DEBUGLOG(("SongIterator::EnsureOffset - reload\n"));
    ce.OffValid = true;
    ce.Off.Reset();
    PlayableInstance* pi = (PlayableInstance*)ce.Item.get();
    // If no item is set -> leave offset at zero.
    if (pi == NULL)
      continue;
    for(;;)
    { pi = parent.GetPrev(pi);
      // No more previous items => finished
      if (pi == NULL)
        break;
      // Abort because the offset is invalidated anew.
      if (!ce.OffValid)
      { // We make a small break here to let the things consolidate.
        DosSleep(1);
        goto restart;
      }
      ce.Off += ce.GetInfo();
    }
  }
}

void SongIterator::ListChangeHandler(const PlayableCollection::change_args& args, CallstackEntry*const& cep)
{ DEBUGLOG(("SongIterator(%p)::ListChangeHandler(, %p)\n", this, cep));
  // It is cheaper to invalidate the offset always than to check whether we must invalidate the offset or not.
  cep->OffValid = false;
  Change(0);
}

SongIterator::Offsets SongIterator::GetOffset(bool withlocation) const
{ DEBUGLOG(("SongIterator(%p)::GetOffset(%u) - %u\n", this, withlocation, Callstack->size()));
  ASSERT(Callstack->size());

  // Sum all offsets
  CallstackEntry*const* cepp = Callstack->begin();
  CallstackEntry* cep = *cepp;
  Offsets ret = cep->Off;
  while (++cepp != Callstack->end())
  { // The is another subentry => Item of the last entry must be non-zero and a enumerable.
    ASSERT(cep->Item);
    ASSERT(cep->Item->GetPlayable()->GetFlags() & Playable::Enumerable);
    PlayableCollection& parent = (PlayableCollection&)*cep->Item->GetPlayable();
    cep = *cepp;
    EnsureOffset(*cep, parent);
    ret += (*cepp)->Off;
  }
  DEBUGLOG(("SongIterator::GetOffset: {%i, %f}\n", ret.Index, ret.Offset));
  // cep now points to current

  if (withlocation && cep->Item && cep->Item->GetPlayable()->GetFlags() == Playable::None)
  { ret.Offset += Location;
    const SongIterator* start = cep->GetStart();
    DEBUGLOG(("SongIterator::GetOffset - %f %s\n", Location, DebugName(start).cdata()));
    if (start)
      ret.Offset -= start->GetLocation();
  }
  return ret;
}

bool SongIterator::SetTimeOffset(T_TIME offset)
{ DEBUGLOG(("SongIterator(%p)::SetTimeOffset(%f)\n", this, offset));
  MakeCallstackUnique();
  CallstackEntry* cep = (*Callstack)[Callstack->size()-1];
  ASSERT(cep->Item);
  // calculate and verify offset
  if (offset < 0)
  { const PlayableCollection::CollectionInfo& off = cep->GetInfo();
    if (off.Songlength < 0)
    { DEBUGLOG(("SongIterator::SetTimeOffset: undefined item length with reverse navigation\n"));
      return false;
    }
    offset += off.Songlength;
    if (offset < 0)
    { DEBUGLOG(("SongIterator::SetTimeOffset: reverse navigation out of bounds %f/%f\n", offset, off.Songlength));
      return false;
    }
  }
  // do the navigation
  Playable* pp = cep->Item->GetPlayable();
  if (pp->GetFlags() & Playable::Enumerable)
  { // Current is a playlist
    // Enter the playlist
    Enter();
    InvalidateCurrentCache();
    // loop until we cross the offset
    { pp->EnsureInfo(Playable::IF_Other);
      // lock collection
      Playable::Lock lock(*pp);
      for (;;)
      { // fetch first/next element
        NextCore();
        cep = (*Callstack)[Callstack->size()-1];
        if (cep->Item == NULL)
        { DEBUGLOG(("SongIterator::SetTimeOffset: unexpected end of list. Offset out of range.\n"));
          return false;
        }
        const PlayableCollection::CollectionInfo& off = cep->GetInfo();
        // Terminate loop if the length of the current item is not finite or if it is larger than the required offset.
        if (off.Songlength < 0 || off.Songlength > offset)
          break;
        offset -= off.Songlength;
      }
    } // unlock collection
    // We found a matching location. Apply the residual offset to the subitem.
    return SetTimeOffset(offset);

  } else
  { // current is a song
    SetLocation(offset);
    return true;
  }
}

bool SongIterator::Navigate(const xstring& url, int index)
{ DEBUGLOG(("SongIterator(%p)::Navigate(%s, %i)\n", this, url ? url.cdata() : "<null>", index));
  ASSERT(index);
  PlayableCollection* list = GetList();
  ASSERT(list);
  if (index == 0)
    return false;
  MakeCallstackUnique();
  // lock collection
  Playable::Lock lck(*list);
  // search item in the list
  if (!url)
  { list->EnsureInfo(Playable::IF_Phys|Playable::IF_Other);
    // address from back
    if (index < 0)
      index += list->GetInfo().phys->num_items +1;
    // address by index
    if (index <= 0 || index > list->GetInfo().phys->num_items)
      return false; // index out of range
    CallstackEntry* cep = (*Callstack)[Callstack->size()-1];
    // List is not yet open. Enter it.
    if (cep->Item->GetPlayable() == list)
    { Enter();
      cep = (*Callstack)[Callstack->size()-1];
    } else
      cep->Item = NULL; // Start over
    // Offsets structure slicing
    (Offsets&)*cep = (const Offsets&)*(*Callstack)[Callstack->size()-2];
    InvalidateCurrentCache();
    /*// Speed up: start from back if closer to it
    if ((index<<1) > list->GetInfo().phys->num_items)
    { // iterate from behind
      index = list->GetInfo().phys->num_items - index;
      do
        pi = list->GetPrev(pi);
      while (index--);
    } else*/
    { // iterate from the front
      for(;;)
      { cep->Item = list->GetNext((PlayableInstance*)cep->Item.get());
        if (--index == 0)
          break;
        // adjust offsets
        if (!IsInCallstack(cep->Item->GetPlayable()))
          cep->Off += cep->GetInfo();
      }
      // TODO: conflicting start offsets?
    }
  } else if (url == "..")
  { do
    { // navigate one level up
      if (list == GetRoot()->GetPlayable())
        return false; // Error: can't navigate above the top level.
      Leave();
    } while (--index);
    return true;
  } else
  { // look for a certain URL
    url123 absurl = list->GetURL().makeAbsolute(url);
    if (!absurl)
      return false; // The URL is broken. This may happen if a time offset is applied to a playlist.
    const int_ptr<Playable>& pp = Playable::FindByURL(absurl);
    if (pp == NULL)
      return false; // If the url is in the playlist it /must/ exist in the repository, otherwise => error.
    if (IsInCallstack(pp))
      return false; // Error: Cannot navigate to recursive item
    // List is not yet open. Enter it.
    list->EnsureInfo(Playable::IF_Other);
    if (Current()->GetPlayable() == list)
      Enter();
    else
      (*Callstack)[Callstack->size()-1]->Item = NULL; // Start over
    CallstackSubFactory ce(*(*Callstack)[Callstack->size()-2]);
    if (index > 0)
    { // forward lookup
      for(;;)
      { ce.Item = list->GetNext((PlayableInstance*)ce.Item.get());
        if (ce.Item == NULL)
          return false; // Error: not found
        if (stricmp(ce.Item->GetPlayable()->GetURL(), absurl) == 0 && --index == 0)
          break;
        // adjust offsets
        if (!ce.IsExcluded())
          ce.Off += ce.GetInfo();
      }
    } else
    { // reverse lookup
      do
      { ce.Item = list->GetPrev((PlayableInstance*)ce.Item.get());
        if (ce.Item == NULL)
          return false; // Error: not found
      } while (stricmp(ce.Item->GetPlayable()->GetURL(), absurl) != 0 || --index != 0);
      // adjust offsets (always from front)
      PlayableSlice* found = ce.Item; // save Item because we abuse ce for offset calculation
      while ((ce.Item = list->GetPrev((PlayableInstance*)ce.Item.get())) != NULL)
        if (!ce.IsExcluded())
          ce.Off += ce.GetInfo();
      // restore item
      ce.Item = found;
    }
    // commit
    (*Callstack)[Callstack->size()-1]->Swap(ce);
    // TODO: slices?
  }
  /*// Auto-enter playlist objects
  if (Current() && (Current()->GetPlayable()->GetFlags() & Playable::Enumerable))
    Enter();*/
  return true;
}

bool SongIterator::NavigateFlat(const xstring& url, int index)
{ DEBUGLOG(("SongIterator(%p)::NavigateFlat(%s, %i)\n", this, url ? url.cdata() : "<null>", index));
  ASSERT(index);
  MakeCallstackUnique();
  // TODO: Rollback !!!
  // Implicitely enter current playlists
  if ((Current()->GetPlayable()->GetFlags() & Playable::Enumerable))
    Enter();
  if (index == 0 || Callstack->size() < 2)
  { DEBUGLOG(("SongIterator::NavigateFlat: precondition failed\n"));
    return false;
  }
  // search item in the list
  CallstackEntry* cep = (*Callstack)[Callstack->size()-2];
  PlayableCollection* const list = (PlayableCollection*)cep->Item->GetPlayable();
  if (!url)
  { // Navigation by index only
    list->EnsureInfo(Playable::IF_Other);
    { // lock collection
      Playable::Lock lck(*list);
      const PlayableCollection::CollectionInfo& ci = cep->GetInfo();
      if (ci.Items < 0)
      { DEBUGLOG(("SongIterator::NavigateFlat: undefined number of playlist items\n"));
        return false;
      }
      // calculate and verify offset
      if (index < 0)
        index += ci.Items + 1;
      if (index < 1 || index > ci.Items)
      { DEBUGLOG(("SongIterator::NavigateFlat: index out of bounds %i/%i\n", index, ci.Items));
        return false;
      }
      // loop until we cross the number of items
      (*Callstack)[Callstack->size()-1]->Item = NULL;
      for (;;)
      { // fetch first/next element
        NextCore();
        cep = (*Callstack)[Callstack->size()-1];
        if (cep->Item == NULL)
        { DEBUGLOG(("SongIterator::NavigateFlat: unexpected end of list. ???\n"));
          Leave();
          return false;
        }
        const PlayableCollection::CollectionInfo& off = cep->GetInfo();
        // Terminate loop if the number of subitems is unknown or if it is larger than the required index.
        if (off.Items < 0 || off.Items >= index)
          break;
        index -= off.Items;
      }
    }
    // We found a matching location. Apply the residual offset to the subitem if any.
    if (cep->Item->GetPlayable()->GetFlags() & Playable::Enumerable)
      return NavigateFlat(url, index);
    // If we are at a song item, the index should match exactly.
    ASSERT(index == 1);
    return true;

  } else if (url == "..")
  { // invalid within this context
    return false;

  } else
  { // look for a certain URL
    url123 absurl = list->GetURL().makeAbsolute(url);
    const int_ptr<Playable>& pp = Playable::FindByURL(absurl);
    if (pp == NULL)
    { DEBUGLOG(("SongIterator::NavigateFlat: URL not well known.\n"));
      return false; // If the url is in the playlist it /must/ exist in the repository, otherwise => error.
    }
    if (IsInCallstack(pp))
    { DEBUGLOG(("SongIterator::NavigateFlat: Cannot navigate to recursive item.\n"));
      return false; // Error: Cannot navigate to recursive item
    }
    // do the navigation
    InvalidateCurrentCache();
    size_t level = Callstack->size() -1;
    if (index > 0)
    { // forward lookup
      do 
      { if (!PrevNextCore(&SongIterator::NextCore, level))
          return false; // Error: not found
      } while (stricmp(Current()->GetPlayable()->GetURL(), absurl) != 0 || --index != 0);
    } else
    { // reverse lookup
      do
      { if (!PrevNextCore(&SongIterator::PrevCore, level))
          return false; // Error: not found
      } while (stricmp(Current()->GetPlayable()->GetURL(), absurl) != 0 || --index != 0);
    }
    // commit
    // TODO: well we should support rollback.
    return true;
  }
}

xstring SongIterator::Serialize(bool withlocation) const
{ DEBUGLOG(("SongIterator(%p)::Serialize()\n", this));
  if (GetRoot() == NULL)
    return xstring(); // NULL
  xstring ret = xstring::empty;
  const CallstackEntry*const* cpp = Callstack->begin();
  const Playable* list = GetRoot()->GetPlayable();
  while (++cpp != Callstack->end()) // Start at Callstack[1]
  { const PlayableInstance* pi = (const PlayableInstance*)(*cpp)->Item.get();
    if (pi == NULL)
      break;
    // Fetch info
    const url123& url = pi->GetPlayable()->GetURL();
    // append url to result
    ret = ret + "\"" + url.makeRelative(list->GetURL()) + "\"";
    // check if the item is unique
    unsigned n = 1;
    const PlayableInstance* pi2 = pi;
    while ((pi2 = ((PlayableCollection*)list)->GetPrev(pi2)) != NULL)
      if (stricmp(pi2->GetPlayable()->GetURL(), url) == 0)
        ++n;
    if (n > 1)
    { // Not unique => provide index
      char buf[12];
      sprintf(buf, "%u", n);
      ret = ret + buf;
    }
    // next
    ret = ret + ";";
    list = pi->GetPlayable();
  }
  if (withlocation && Location)
  { char buf[30];
    char* cp = buf;
    if (Location < 0)
      *cp++ = '-';
    T_TIME loc = fabs(Location);
    unsigned long secs = (unsigned long)loc;
    if (secs >= 86400)
      sprintf(cp, "%ld:%02ld:%02ld", secs / 86400, secs / 3600 % 60, secs / 60 % 60);
    else if (secs > 3600)
      sprintf(cp, "%ld:%02ld", secs / 3600, secs / 60 % 60);
    else
      sprintf(cp, "%ld", secs / 60);
    cp += strlen(cp);
    loc -= secs;
    ASSERT(loc >= 0 && loc < 1);
    if (loc)
      sprintf(cp+2, "%f", loc); // Dirty hack to remove the leading 0.
    sprintf(cp, ":%02ld", secs % 60);
    cp[3] = '.';
    ret = ret + buf;
  }
  return ret;
}

bool SongIterator::Deserialize(const char*& str)
{ DEBUGLOG(("SongIterator(%p)::Deserialize(%s)\n", this, str));
  ASSERT(GetRoot());
  MakeCallstackUnique();
  InvalidateCurrentCache();
  for (; *str; str += strspn(str, ";\r\n"))
  { PlayableSlice* pi = (*Callstack)[Callstack->size()-1]->Item;
    if (pi == NULL || (pi->GetPlayable()->GetFlags() & Playable::Enumerable))
    { // Playlist navigation
      // parse part into url, index
      xstring url;
      size_t index = 1;
      if (str[0] == '"')
      { // quoted item
        const char* ep = strchr(str+1, '"');
        if (ep == NULL)
        { url = str+1;
          str += 1 + url.length(); // end of string => set str to the terminating \0
        } else
        { url.assign(str+1, ep-str-1);
          ++ep; // move behind the '"'
          // look for index
          size_t len = strcspn(ep, ";\r\n");
          if (len)
          { size_t n;
            if (sscanf(ep, "[%u]%n", &index, &n) != 1 || n != len || index == 0)
            { str = ep;
              DEBUGLOG(("SongIterator::Deserialize: invalid index at %s\n", str));
              return false; // Syntax error (invalid index)
            }
          }
          str = ep + len;
        }
      } else
      { // unquoted => find delimiter
        size_t len = strcspn(str, ";\r\n");
        if (len == 0)
        { // String starts with a part seperator => use absolute navigation
          Reset();
          continue;
        }
        if (str[len-1] == ']')
        { // with index
          char* ep = strnrchr(str, '[', len);
          size_t n;
          if (ep == NULL || sscanf(ep+1, "%u%n", &index, &n) != 1 || (int)n != str+len-ep-2 || index == 0)
          { str = ep+1;
            DEBUGLOG(("SongIterator::Deserialize: invalid index (2) at %s\n", ep));
            return false; // Syntax error (invalid index)
          }
          if (ep != str)
            url.assign(str, ep-str);
            // otherwise only index is specified => leave url NULL instead of ""
        } else
          url.assign(str, len);
        str += len;
      }

      // do the navigation
      Navigate(url, index);

    } else
    { // Song navigation
      bool sign = false;
      if (*str == '-')
      { sign = true;
        ++str;
      }
      T_TIME t[4] = {0};
      T_TIME* dp = t;
      while (*str)
      { size_t n = 0;
        sscanf(str, "%lf%n", dp, &n);
        str += n;
        if (*str == 0)
          break;
        if (*str != ':')
        { DEBUGLOG(("SongIterator::Deserialize: invalid character at %s\n", str));
          return false; // Error: Invalid character
        }
        do
        { if (++dp == t + sizeof t / sizeof *t)
          { DEBUGLOG(("SongIterator::Deserialize: to many ':' at %s\n", str));
            return false; // Error: too many ':'
          }
        } while (*++str == ':');
      }

      // make time
      if (dp-- != t)
        dp[0] = 60*dp[0] + dp[1]; // Minutes
      if (dp-- != t)
        dp[0] = 3600*dp[0] + dp[1]; // Hours
      if (dp-- != t)
        dp[0] = 86400*dp[0] + dp[1]; // Days

      // do the navigation
      T_TIME songlen = Current()->GetPlayable()->GetInfo().tech->songlength;
      if (sign)
      { // reverse location
        if (songlen < 0)
        { DEBUGLOG(("SongIterator::Deserialize: cannot navigate %f from back with unknown songlength\n", songlen));
          return false; // Error: cannot navigate from back if the song length is unknown.
        }
        t[0] = songlen - t[0];
        if (t[0] < 0)
        { DEBUGLOG(("SongIterator::Deserialize: %f is beyond the end of the song at %f\n", songlen, t[0]));
          return false; // Error: offset larger than the song
        }
      } else if (songlen >= 0 && t[0] > songlen)
      { DEBUGLOG(("SongIterator::Deserialize: %f is beyond the end of the song at %f\n", songlen, t[0]));
        return false; // Error: offset larger than the song
      }
      SetLocation(t[0]);
    }
  } // next part
  return true;
}

int SongIterator::CompareTo(const SongIterator& r, unsigned level, bool withlocation) const
{ DEBUGLOG(("SongIterator(%p)::CompareTo(%p) - %s - %s\n", this, &r, Serialize().cdata(), r.Serialize().cdata()));
  ASSERT(level < Callstack->size());
  const SongIterator::CallstackEntry*const* lcpp = Callstack->begin() + level;
  const SongIterator::CallstackEntry*const* rcpp = r.Callstack->begin();
  if ((*lcpp)->Item == NULL && (*rcpp)->Item == NULL)
    return 0; // Both iterators are unassigned
  if ( (*lcpp)->Item == NULL || (*rcpp)->Item == NULL
    || (*lcpp)->Item->GetPlayable() != (*rcpp)->Item->GetPlayable() )
    return INT_MIN; // different root
  for (;;)
  { // Fetch next level
    ++lcpp;
    ++rcpp;
    ++level;
    if (lcpp == Callstack->end() || (*lcpp)->Item == NULL)
    { if (rcpp != r.Callstack->end() && (*rcpp)->Item != NULL)
        return -level; // current SongIterator is deeper
      // Item identical => compare location below
      if (!withlocation || Location == r.Location || (Location < 0 && r.Location < 0))
        return 0; // same
      ++level; // Location difference returns level+1
      return Location < 0 || (r.Location >= 0 && Location > r.Location) ? level : -level;
    }
    if (rcpp == r.Callstack->end() || (*rcpp)->Item == NULL)
      return level; // current SongIterator is deeper
    // Not the same Item => check their order
    if ((*lcpp)->Item != (*rcpp)->Item)
    { // Check ordering of the Items
      int cmp = ((PlayableInstance*)(*lcpp)->Item.get())->CompareTo(*(PlayableInstance*)(*rcpp)->Item.get());
      if (cmp != 0)
        return cmp > 0 ? level : -level;
    }
  }
}
