/*
 * Copyright 2007-2009 M.Mueller
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


#include "properties.h"
#include "playableref.h"
#include "playable.h"
#include "location.h"
#include <utilfct.h>
#include <string.h>

#include <debuglog.h>


/****************************************************************************
*
*  class PlayableSlice
*
****************************************************************************/

Mutex PlayableSlice::CollectionInfoMutex;


PlayableSlice::PlayableSlice(APlayable& pp)
: RefTo(&pp),
  InfoDeleg(*this, &PlayableSlice::InfoChangeHandler)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(APlayable&%p)\n", this, &pp));
  Info.rpl = NULL;
  Info.drpl = NULL;
  Info.item = &Item;
  if (RefTo)
  { RefTo->GetInfoChange() += InfoDeleg;
  }
}
PlayableSlice::PlayableSlice(const PlayableSlice& r)
: RefTo(r.RefTo),
  Item(r.Item),
  StartCache(r.StartCache),
  StopCache(r.StopCache),
  InfoDeleg(*this, &PlayableSlice::InfoChangeHandler)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(PlayableSlice&%p)\n", this, &r));
  Info.rpl  = NULL;
  Info.drpl = NULL;
  Info.item = &Item;
  if (RefTo)
  { RefTo->GetInfoChange() += InfoDeleg;
  }
}

PlayableSlice::~PlayableSlice()
{ DEBUGLOG(("PlayableSlice(%p)::~PlayableSlice() - %p\n", this, RefTo.get()));
  InfoDeleg.detach();
};

/*void PlayableSlice::Swap(PlayableSlice& r)
{ DEBUGLOG(("PlayableSlice(%p)::Swap(&%p)\n", this, &r));
  ASSERT(RefTo == r.RefTo);
  Start.swap(r.Start);
  Stop.swap(r.Stop);
  //swap(RplState, r.RplState);
  //StartCache.swap(r.StartCache);
  //StopCache.swap(r.StopCache);
  //swap(RplInfo, r.RplInfo);
  swap(Info, r.Info);
  // There is no need to touch the event registrations, since RefTo is the same.
}*/

int PlayableSlice::CompareSliceBorder(const Location* l, const Location* r, SliceBorder type)
{ DEBUGLOG(("PlayableSlice::CompareSliceBorder(%p, %p, %d)\n", l, r, type));
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

xstring PlayableSlice::GetDisplayName() const
{ return RefTo->GetDisplayName();
}

/*int_ptr<Location> PlayableSlice::GetLocCore(const volatile xstring& strloc, volatile int_ptr<Location>& cache, SliceBorder type, Priority pri) const
{ xstring localstr = strloc;
  DEBUGLOG(("PlayableSlice(%p)::GetLocCore(&%s, &%p, %d)\n", this, localstr.cdata(), cache.debug(), type));
  int_ptr<Location> localcache = cache;
  if (localstr && !localcache)
  { // Two threads may run into the same code. We accept this redundancy because the impact is small.
    ASSERT(!RefTo->RequestInfo(IF_Child, PRI_None, REL_Invalid));
    Location* si = new Location(RefTo->GetPlayable());
    const char* cp = localstr;
    const xstring& err = si->Deserialize(cp, pri);
    if (err)
      // TODO: Errors
      DEBUGLOG(("PlayableSlice::GetLocCore: %s at %s\n", err.cdata(), cp));
    // Intersection with RefTo->GetStartLoc()
    localcache = RefTo->GetStartLoc();
    if (CompareSliceBorder(si, localcache, type) < 0)
      delete si;
    else
      localcache = si;
    Mutex::Lock(RefTo->GetPlayable().Mtx);
    if (localstr.instEquals((const xstring&)strloc)) // Double check
      cache = localcache;
  }
  return localcache;
}*/

PlayableSlice::CalcResult PlayableSlice::CalcLoc(const volatile xstring& strloc, volatile int_ptr<Location>& cache, SliceBorder type, Priority pri)
{ xstring localstr = strloc;
  DEBUGLOG(("PlayableSlice(%p)::CalcLoc(&%s, &%p, %d, %u)\n", this, localstr.cdata(), cache.debug(), type, pri));
  if (localstr)
  { ASSERT(!RefTo->RequestInfo(IF_Slice, PRI_None, REL_Invalid));
    Location* si = new Location(&RefTo->GetPlayable());
    const char* cp = localstr;
    const xstring& err = si->Deserialize(cp, pri);
    if (err)
    { if (err.length() == 0) // delayed
        return CR_Delayed;
      // TODO: Errors
      DEBUGLOG(("PlayableSlice::CalcLoc: %s at %.20s\n", err.cdata(), cp));
    }
    // Intersection with RefTo->GetStartLoc()
    int_ptr<Location> newval = RefTo->GetStartLoc();
    if (CompareSliceBorder(si, newval, type) < 0)
      delete si;
    else
      newval = si;
    // commit
    int_ptr<Location> localcache = newval;
    localcache.swap(cache);
    return localcache && *localcache == *newval ? CR_Nop : CR_Changed; 
  } else
  { int_ptr<Location> localcache;
    localcache.swap(cache);
    return localcache ? CR_Changed : CR_Nop;
  }
}

int_ptr<Location> PlayableSlice::GetStartLoc() const
{ if (IsItemOverridden())
    return StartCache;
  else
    return RefTo->GetStartLoc();
}

int_ptr<Location> PlayableSlice::GetStopLoc() const
{ if (IsItemOverridden())
    return StopCache;
  else
    return RefTo->GetStopLoc();
}

InfoFlags PlayableSlice::InvalidateCIC(InfoFlags what, const Playable* pp) 
{ DEBUGLOG(("PlayableSlice::InvalidateCIC(%p)\n", pp));
  InfoFlags ret = IF_None;
  Mutex::Lock lock(CollectionInfoMutex);
  CollectionInfoEntry** cipp = CollectionInfoCache.begin();
  CollectionInfoEntry** eipp = CollectionInfoCache.end();
  while (cipp != eipp)
  { CollectionInfoEntry* cip = *cipp++;
    if (!pp || !cip->Exclude.contains(*pp));
      ret |= cip->InfoStat.Invalidate(what);
  }
  return ret;
}

InfoFlags PlayableSlice::GetOverridden() const
{ return RefTo->GetOverridden()
    | IF_Item * (Info.item == &Item);
}

void PlayableSlice::OverrideItem(const ITEM_INFO* item)
{
  #ifdef DEBUG
  if (item)
    DEBUGLOG(("PlayableSlice(%p)::OverrideSlice({%s, %s,%s, %f,%f, %f})\n", this,
      item->alias.cdata(), item->start.cdata(), item->stop.cdata(), item->pregap, item->postgap, item->gain));
  else
    DEBUGLOG(("PlayableSlice(%p)::OverrideSlice(<NULL>)\n", this));
  #endif
  if (!IsItemOverridden() && item == NULL)
    return; // This is a no-op.
  PlayableChangeArgs args(*this, this, IF_Item, IF_None, IF_None);
  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const ITEM_INFO& old_item = IsItemOverridden() ? Item : (const ITEM_INFO&)*RefTo->GetInfo().item;
  const ITEM_INFO& new_item = item ? *item : (const ITEM_INFO&)*RefTo->GetInfo().item;
  // Analyze changed flags
  const bool startchanged = new_item.start != old_item.start;
  const bool stopchanged  = new_item.stop != old_item.stop;
  if (startchanged|stopchanged)
  { args.Invalidated |= IF_Slice;
    args.Changed |= IF_Item;
  }
  if (new_item.alias != old_item.alias)
  { args.Invalidated |= IF_Display;
    args.Changed |= IF_Item;
  } else
  if (new_item.pregap != old_item.pregap || new_item.postgap != old_item.postgap || new_item.gain != old_item.gain)
    args.Changed |= IF_Item;
  if (item)  
  { Item = *item;
    Info.item = &Item;
    if (startchanged)
      StartCache.reset();
    if (stopchanged)
      StopCache.reset();
    if (startchanged|stopchanged)
    { args.Invalidated |= IF_Rpl|IF_Drpl;
      InvalidateCIC(IF_Rpl|IF_Drpl, NULL);
    }
  } else
  { Info.item = (ITEM_INFO*)RefTo->GetInfo().item;
    StartCache.reset();
    StopCache.reset();
    args.Invalidated |= IF_Rpl|IF_Drpl;
    InvalidateCIC(IF_Rpl|IF_Drpl, NULL);
  }
  InfoChange(args);
}


void PlayableSlice::InfoChangeHandler(const PlayableChangeArgs& args)
{ DEBUGLOG(("PlayableSlice(%p)::InfoChangeHandler(&{&%p, %p, %x, %x, %x})\n", this,
    &args.Instance, args.Origin, args.Changed, args.Loaded, args.Invalidated));
  PlayableChangeArgs args2(args);
  if (args.Changed & (IF_Rpl|IF_Drpl))
    args2.Invalidated |= InvalidateCIC(args.Changed & (IF_Rpl|IF_Drpl), args.Origin ? &args.Origin->GetPlayable() : NULL);
  if (IsItemOverridden())
  { args2.Changed     &= ~IF_Item;
    args2.Invalidated &= ~IF_Item;
  }
  if (!args2.IsInitial())
    InfoChange(args2);
}

InfoFlags PlayableSlice::DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("PlayableSlice(%p)::DoRequestInfo(%x, %d, %d)\n", this, what, pri, rel));

  if (!IsItemOverridden())
    return CallDoRequestInfo(*RefTo, what, pri, rel);

  what &= ~IF_Item; // Nothing to do for ItemInfo if it is overridden.

  if (what & IF_Drpl)
    what |= IF_Phys|IF_Tech|IF_Obj|IF_Child|IF_Slice; // required for DRPL_INFO aggregate
  else if (what & IF_Rpl)
    what |= IF_Tech|IF_Child|IF_Slice; // required for RPL_INFO aggregate

  // Forward all remaining requets to *RefTo.
  InfoFlags what2 = what & IF_Slice;
  InfoFlags async = CallDoRequestInfo(*RefTo, what, pri, rel);

  what2 &= InfoStat.Check(what2, rel);
  if (pri == PRI_None || what2 == IF_None)
    return async;

  // Try fastpath for slice info.
  /*if ((what & IF_Slice) == 0)
  { 
  
    int cr = CalcLoc(Item.start, StartCache, SB_Start, PRI_None)
           | CalcLoc(Item.stop,  StopCache,  SB_Stop,  PRI_None);
    switch (cr)
    {case CR_Changed:
      InfoChange(PlayableChangeArgs(*this, this, IF_Slice * (cr == CR_Changed), IF_Slice, IF_None));
     case CR_Nop:
      InfoStat.
      return async;
      
      case CR_Delayed:
  }*/
    
  // Place request for slice
  async |= InfoStat.Request(what2, pri);
  what |= what2;
    
  return async;
}

InfoFlags PlayableSlice::GetNextCIWorkItem(CollectionInfoEntry*& item, Priority pri)
{ DEBUGLOG(("PlayableSlice(%p)::GetNextCIWorkItem(%p, %u)\n", this, item, pri));
  Mutex::Lock lck(CollectionInfoMutex);
  CollectionInfoEntry** cipp = CollectionInfoCache.begin();
  if (item)
  { size_t pos;
    RASSERT(CollectionInfoCache.binary_search(item->Exclude, pos));
    cipp += pos +1;
  }
  CollectionInfoEntry** eipp = CollectionInfoCache.end();
  while (cipp != eipp)
  { InfoFlags req = (*cipp)->InfoStat.GetRequest(pri);
    if (req)
    { item = *cipp;
      return req;
    }
  }
  item = NULL;
  return IF_None;
}

bool PlayableSlice::AddSong(AggregateInfo& ai, APlayable& song, InfoFlags what, Priority pri, PM123_TIME start, PM123_TIME stop)
{ DEBUGLOG(("PlayableSlice::AddSong(, &%p, %x, %u, %f, %f)\n", &song, what, pri, start, stop));
  ai.Rpl.totalsongs += 1;
  if (!(what & IF_Drpl))
    return false;
  if (song.RequestInfo(IF_Phys|IF_Obj, pri))
    return true;
  // calculate playing time
  PM123_TIME len;
  if (stop >= 0)
    len = stop;
  else
  { len = song.GetInfo().obj->songlength;
    if (len < 0)
    { ai.Drpl.unk_length += 1;
      goto nolen;
  } }
  if (start >= 0)
  { if (start >= len)
      len = 0;
    else
      len -= start;
  }     
  ai.Drpl.totallength += len;
 nolen:
  // approximate size
  if (len < 0 && start >= 0)
    ai.Drpl.unk_size += 1;
  else
  { PM123_SIZE size = song.GetInfo().phys->filesize;
    if (size >= 0)
      size *= len / song.GetInfo().obj->songlength; // may get negative...
    if (size < 0)
      ai.Drpl.unk_size += 1;
    else
      ai.Drpl.totalsize += size;
  }
  return false;
}

bool PlayableSlice::CalcRplCore(AggregateInfo& ai, APlayable& cur, OwnedPlayableSet& exclude, InfoFlags what, Priority pri, const Location* start, const Location* stop, size_t level)
{ DEBUGLOG(("PlayableSlice::ClacRplCore(, &%p, {%u,}, %x, %u, %p, %p, %i)\n",
    &cur, exclude.size(), what, pri, start, stop, level));
  bool ret = false;

 recurse:
  int_ptr<PlayableInstance> psp;
  PM123_TIME ss = -1;
  if (start)
  { if (start->GetLevel() > level)
      psp = start->GetCallstack()[level];
    else if (start->GetLevel() == level)
    { if (cur.RequestInfo(IF_Tech|IF_Obj, pri))
        ret = true;
      else if (cur.GetInfo().tech->attributes & TATTR_SONG)
        ss = start->GetPosition();
    }
  }
  int_ptr<PlayableInstance> pep;
  PM123_TIME es = -1;
  if (stop)
  { if (stop->GetLevel() > level)
      pep = stop->GetCallstack()[level];
    else if (stop->GetLevel() == level)
    { if (cur.RequestInfo(IF_Tech|IF_Obj, pri))
        ret = true;
      else if (cur.GetInfo().tech->attributes & TATTR_SONG)
        es = stop->GetPosition();
    }
  }

  if (psp == pep)
  { if (psp)
    { if (exclude.add(psp->GetPlayable()))
      { ++level;
        goto recurse;
      } else
        return ret;
    } else
    { // both are NULL
      // Either because start and stop is NULL
      // or because both locations point to the same song
      // or because one location points to a song while the other one is NULL.
      ret |= AddSong(ai, cur, what, pri, ss, es);
      return ret;
    }
  }
    
  if (psp)
  { if (exclude.add(psp->GetPlayable()))
    { ret |= CalcRplCore(ai, *psp, exclude, what, pri, start, NULL, level+1);
      // Restore previous state.
      exclude.erase(psp->GetPlayable());
    } // else: item in the call stack => ignore entire sub tree. 
  }
  if (pep)
  { if (exclude.add(pep->GetPlayable()))
    { ret |= CalcRplCore(ai, *pep, exclude, what, pri, NULL, stop, level+1);
      // Restore previous state.
      exclude.erase(pep->GetPlayable());
    } // else: item in the call stack => ignore entire sub tree. 
  }

  // Add the range (psp, pep). Exclusive interval!
  if (cur.RequestInfo(IF_Child, pri))
    return true;
  Playable& pc = cur.GetPlayable();
  for (;;)
  { psp = pc.GetNext(psp);
    if (!psp)
      break;
    const volatile AggregateInfo& lai = psp->RequestAggregateInfo(exclude, what, pri);
    if (what)
      ret = true;
    else
    { ai.Rpl += lai.Rpl;
      ai.Drpl += lai.Drpl;
    }
  }
  return ret;
}

bool PlayableSlice::CalcRplInfo(CollectionInfoEntry& cie, InfoFlags what, Priority pri, PlayableChangeArgs& event)
{ DEBUGLOG(("PlayableSlice(%p)::CalcRplInfo(&%p, %x, %u,)\n", this, &cie, what, pri));
  InfoState::Update upd(cie.InfoStat, what);
  if (upd == IF_None)
    return false; // Somebody else was faster 
  if (RefTo->RequestInfo(IF_Tech|IF_Child|IF_Slice, pri))
  { DEBUGLOG(("PlayableSlice::CalcRplInfo: missing info???\n"));
    return true;
  }

  AggregateInfo ai(cie.Exclude);
  // We can be pretty sure that ITEM_INFO is overridden.
  int_ptr<Location> start = StartCache;
  int_ptr<Location> stop  = StopCache;
  OwnedPlayableSet exclude(cie.Exclude); // copy exclusion list
  if (CalcRplCore(ai, *RefTo, exclude, upd, pri, start, stop, 0))
    return true;

  Mutex::Lock lck(RefTo->GetPlayable().Mtx);
  if ((upd & IF_Rpl) && cie.Rpl.CmpAssign(ai.Rpl))
    event.Changed |= IF_Rpl;
  if ((upd & IF_Drpl) && cie.Drpl.CmpAssign(ai.Drpl))
    event.Changed |= IF_Drpl;
  event.Loaded |= upd.Commit();
  return false;
}

bool PlayableSlice::DoLoadInfo(Priority pri)
{ DEBUGLOG(("PlayableSlice(%p)::DoLoadInfo(%d)\n", this, pri));
  // Load base info first.
  bool ret = CallDoLoadInfo(*RefTo, pri);

  PlayableChangeArgs args(*this);

  InfoState::Update upd(InfoStat, InfoStat.GetRequest(pri));
  // Prepare slice info.
  if (upd & IF_Slice)
  { int cr = CalcLoc(Item.start, StartCache, SB_Start, pri)
           | CalcLoc(Item.stop,  StopCache,  SB_Stop,  pri);
    if (cr & CR_Changed)
      args.Changed |= IF_Slice;
    if (cr & CR_Delayed)
      ret = true;
    else
      args.Loaded |= IF_Slice;

    args.Loaded &= upd.Commit(IF_Slice) | ~IF_Slice;
    if (!args.IsInitial())
    { InfoChange(args);
      args.Reset();
    }
  } 

  // Load aggregate info if any.
  CollectionInfoEntry* iep = NULL;
  for(;;)
  { // find next item to handle
    InfoFlags req = GetNextCIWorkItem(iep, pri);
    if (iep == NULL)
      break;
    // retrieve information
    InfoFlags chg = req;
    ret |= CalcRplInfo(*iep, chg, pri, args);
    
    if (iep->Exclude.size() == 0)
    { // Store RPL info pointers
      Info.rpl  = &iep->Rpl;
      Info.drpl = &iep->Drpl;
    }
  }
  args.Loaded &= upd.Commit() | ~IF_Aggreg;
  if (!args.IsInitial())
    InfoChange(args);
  return ret;  
}

AggregateInfo& PlayableSlice::DoAILookup(const PlayableSetBase& exclude)
{ DEBUGLOG(("PlayableSlice(%p)::DoAILookup({%u,})\n", this, exclude.size()));

  if (!IsSlice())
    return CallDoAILookup(*RefTo, exclude);

  Mutex::Lock lock(CollectionInfoMutex);
  CollectionInfoEntry*& cic = CollectionInfoCache.get(exclude);
  if (cic == NULL)
    cic = new CollectionInfoEntry(exclude);
  return *cic;
}

InfoFlags PlayableSlice::DoRequestAI(AggregateInfo& ai, InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("PlayableSlice(%p)::DoRequestAI(&%p, %x, %d, %d)\n", this, &ai, what, pri, rel));

  // We have to check wether the supplied AggregateInfo is mine or from *RefTo.
  CollectionInfoEntry* cic;
  { Mutex::Lock lock(CollectionInfoMutex);
    cic = CollectionInfoCache.find(ai.Exclude);
  }
  if (cic != &ai)
    // from *RefTo
    return CallDoRequestAI(*RefTo, ai, what, pri, rel);
  
  CollectionInfoEntry& ci = (CollectionInfoEntry&)ai;
  what = ci.InfoStat.Check(what, rel);
  if (pri == PRI_None || (what & IF_Aggreg) == IF_None)
    return IF_None;
  return ci.InfoStat.Request(what & IF_Aggreg, pri);
}


const Playable& PlayableSlice::DoGetPlayable() const
{ return RefTo->GetPlayable();
}

bool PlayableSlice::IsInUse() const
{ return RefTo->IsInUse();
}

void PlayableSlice::SetInUse(bool inuse)
{ DEBUGLOG(("PlayableSlice(%p)::SetInUse(%u)\n", this, inuse));
  RefTo->SetInUse(inuse);
}

const INFO_BUNDLE_CV& PlayableSlice::GetInfo() const
{ const INFO_BUNDLE_CV& info = RefTo->GetInfo();
  if (IsItemOverridden())
    return info;
  // Updated pointers of inherited info
  INFO_BUNDLE_CV& dst = (INFO_BUNDLE_CV&)Info; // Access mutable field
  dst.phys = info.phys;
  dst.tech = info.tech;
  dst.obj  = info.obj;
  dst.meta = info.meta;
  dst.attr = info.attr;
  if (dst.rpl == NULL || (Item.start == NULL && Item.stop == NULL))
  { dst.rpl  = info.rpl;
    dst.drpl = info.drpl;
  }
  return dst;
}


/****************************************************************************
*
*  class PlayableRef
*
****************************************************************************/

const INFO_BUNDLE_CV& PlayableRef::GetInfo() const
{ const INFO_BUNDLE_CV& info = RefTo->GetInfo();
  // Updated pointers of inherited info
  INFO_BUNDLE_CV& dst = (INFO_BUNDLE_CV&)Info; // Access mutable field
  dst.phys = info.phys;
  dst.tech = info.tech;
  dst.obj  = info.obj;
  if (!IsMetaOverridden())
    dst.meta = info.meta;
  if (!IsAttrOverridden())
    dst.attr = info.attr;
  if (!IsItemOverridden())
    dst.item = info.item;
  else if (dst.rpl != NULL && (Item.start || Item.stop))
    return dst;
  dst.rpl  = info.rpl;
  dst.drpl = info.drpl;
  return dst;
}

InfoFlags PlayableRef::GetOverridden() const
{ return PlayableSlice::GetOverridden()
    | IF_Meta * (Info.meta == &Meta)
    | IF_Attr * (Info.attr == &Attr);
}

void PlayableRef::OverrideMeta(const META_INFO* meta)
{ DEBUGLOG(("PlayableRef(%p)::OverrideMeta({...})\n", this));
  PlayableChangeArgs args(*this, this, IF_Meta, IF_None, IF_None);
  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const MetaInfo& current = IsMetaOverridden() ? Meta : *(const MetaInfo*)RefTo->GetInfo().meta;
  if (meta == NULL)
  { // revoke overloading
    if (!current.IsInitial())
      args.Changed |= IF_Meta;
    Info.meta = RefTo->GetInfo().meta;
  } else
  { // Override
    if (!current.Equals(*meta))
    { args.Changed |= IF_Meta;
      Meta = *meta;
    }
    Info.meta = &Meta;
  }
  if (!args.IsInitial())
    InfoChange(args);
}

void PlayableRef::OverrideAttr(const ATTR_INFO* attr)
{ DEBUGLOG(("PlayableRef(%p)::OverrideAttr({...})\n", this));
  PlayableChangeArgs args(*this, this, IF_Attr, IF_None, IF_None);
  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const AttrInfo& current = IsAttrOverridden() ? Attr : *(const AttrInfo*)RefTo->GetInfo().attr;
  if (attr == NULL)
  { // revoke overloading
    if (!current.IsInitial())
      args.Changed |= IF_Attr;
    Info.attr = RefTo->GetInfo().attr;
  } else
  { // Override
    if (!current.Equals(*attr))
    { args.Changed |= IF_Attr;
      Attr = *attr;
    }
    Info.meta = &Meta;
  }
  if (!args.IsInitial())
    InfoChange(args);
}

void PlayableRef::OverrideInfo(const INFO_BUNDLE& info, InfoFlags override)
{ DEBUGLOG(("PlayableRef(%p)::OverrideInfo(&%p, %x)\n", this, &info, override));
  OverrideMeta(override & IF_Meta ? info.meta : NULL);
  OverrideAttr(override & IF_Attr ? info.attr : NULL);
  OverrideItem(override & IF_Item ? info.item : NULL);
  ASSERT((override & ~(IF_Meta|IF_Attr|IF_Item)) == 0);
}

void PlayableRef::AssignInstanceProperties(const PlayableRef& src)
{ ASSERT(RefTo == src.RefTo);
  OverrideAttr(AccessInfo(src).attr);
  OverrideMeta(AccessInfo(src).meta);
  OverrideItem(AccessInfo(src).item);
}

void PlayableRef::InfoChangeHandler(const PlayableChangeArgs& args)
{ DEBUGLOG(("PlayableRef(%p)::InfoChangeHandler(&{&%p, %p, %x, %x, %x})\n", this,
    &args.Instance, args.Origin, args.Changed, args.Loaded, args.Invalidated));
  PlayableChangeArgs args2(args);
  if (IsMetaOverridden())
  { args2.Changed     &= ~IF_Meta;
    args2.Invalidated &= ~IF_Meta;
  }
  if (IsAttrOverridden())
  { args2.Changed     &= ~IF_Attr;
    args2.Invalidated &= ~IF_Attr;
  }
  PlayableSlice::InfoChangeHandler(args2);
}

