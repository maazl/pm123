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
#include "waitinfo.h"
#include <string.h>

#include <debuglog.h>


/****************************************************************************
*
*  class PlayableSlice
*
****************************************************************************/

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
      if (CIC)
        CIC->Invalidate(IF_Rpl|IF_Drpl, NULL);
    }
  } else
  { Info.item = (ITEM_INFO*)RefTo->GetInfo().item;
    StartCache.reset();
    StopCache.reset();
    args.Invalidated |= IF_Rpl|IF_Drpl;
    if (CIC)
      CIC->Invalidate(IF_Rpl|IF_Drpl, NULL);
  }
  InfoChange(args);
}

InfoFlags PlayableSlice::Invalidate(InfoFlags what)
{ InfoFlags ret = RefTo->Invalidate(what);
  if (CIC)
    ret |= CIC->DefaultInfo.InfoStat.Invalidate(what);
    // TODO: invalidate CIC entries also
  if (ret)
    InfoChange(PlayableChangeArgs(*this, this, IF_None, IF_None, ret));
  return ret;
}

void PlayableSlice::PeekRequest(RequestState& req) const
{ DEBUGLOG(("PlayableSlice(%p)::PeekRequest({%x,%x, %x})\n", this, req.ReqLow, req.ReqHigh, req.InService));
  if (CIC)
  { CIC->DefaultInfo.InfoStat.PeekRequest(req);
    CIC->PeekRequest(req);
  }
  RefTo->PeekRequest(req);
  DEBUGLOG(("PlayableSlice::PeekRequest: {%x,%x, %x}\n", req.ReqLow, req.ReqHigh, req.InService));
}

void PlayableSlice::InfoChangeHandler(const PlayableChangeArgs& args)
{ DEBUGLOG(("PlayableSlice(%p)::InfoChangeHandler(&{&%p, %p, %x, %x, %x})\n", this,
    &args.Instance, args.Origin, args.Changed, args.Loaded, args.Invalidated));
  PlayableChangeArgs args2(args);
  if (args.Changed & (IF_Rpl|IF_Drpl))
  { if (CIC)
      args2.Invalidated |= CIC->Invalidate(args.Changed & (IF_Rpl|IF_Drpl), args.Origin ? &args.Origin->GetPlayable() : NULL);
  }
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

  if (what & IF_Drpl)
    what |= IF_Phys|IF_Tech|IF_Obj|IF_Child|IF_Slice; // required for DRPL_INFO aggregate
  else if (what & IF_Rpl)
    what |= IF_Tech|IF_Child|IF_Slice; // required for RPL_INFO aggregate

  // Forward all remaining requests to *RefTo.
  InfoFlags what2 = what & ~IF_Item;
  // TODO Forward aggregate only if no slice?
  InfoFlags async = CallDoRequestInfo(*RefTo, what, pri, rel);

  EnsureCIC();
  InfoState& infostat = CIC->DefaultInfo.InfoStat;

  what2 |= what & IF_Item;
  what2 &= infostat.Check(what2, rel);
  // IF_Item is always available if we got beyond IsItemOverridden.
  if (what2 & IF_Item)
  { infostat.EndUpdate(infostat.BeginUpdate(IF_Item));
    what2 &= ~IF_Item;
    what &= ~IF_Item;
  }

  if (pri == PRI_None || what2 == IF_None)
    return async;

  /*// Fastpath for IF_Item and possibly IF_Slice.
  InfoFlags upd = InfoStat.BeginUpdate(what2 & (IF_Item|IF_Slice));
  if (what2 & IF_Slice)
  { JobSet dummy(PRI_None);
    int cr = CalcLoc(Item.start, StartCache, SB_Start, dummy)
           | CalcLoc(Item.stop,  StopCache,  SB_Stop,  dummy);

    switch (cr)
    {case CR_Changed:
      InfoChange(PlayableChangeArgs(*this, this, IF_Slice * (cr == CR_Changed), IF_Slice, IF_None));
     case CR_Nop:
      InfoStat.
      return async;
      
      case CR_Delayed:
  }*/
    
  // Place request for slice
  async |= infostat.Request(what2, pri);
  what |= what2;

  return async;
}

AggregateInfo& PlayableSlice::DoAILookup(const PlayableSetBase& exclude)
{ DEBUGLOG(("PlayableSlice(%p)::DoAILookup({%u,})\n", this, exclude.size()));

  if (!IsSlice())
    return CallDoAILookup(*RefTo, exclude);

  EnsureCIC();
  AggregateInfo* pai = CIC->Lookup(exclude);
  return pai ? *pai : CIC->DefaultInfo;
}

InfoFlags PlayableSlice::DoRequestAI(AggregateInfo& ai, InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("PlayableSlice(%p)::DoRequestAI(&%p, %x, %d, %d)\n", this, &ai, what, pri, rel));

  // We have to check whether the supplied AggregateInfo is mine or from *RefTo.
  if (!CIC || !CIC->IsMine(ai))
    // from *RefTo
    return CallDoRequestAI(*RefTo, ai, what, pri, rel);

  return ((CollectionInfo&)ai).RequestAI(what, pri, rel);
}


PlayableSlice::CalcResult PlayableSlice::CalcLoc(const volatile xstring& strloc, volatile int_ptr<Location>& cache, SliceBorder type, JobSet& job)
{ xstring localstr = strloc;
  DEBUGLOG(("PlayableSlice(%p)::CalcLoc(&%s, &%p, %d, {%u,})\n", this, localstr.cdata(), cache.debug(), type, job.Pri));
  if (localstr)
  { ASSERT(!RefTo->RequestInfo(IF_Slice, PRI_None, REL_Invalid));
    Location* si = new Location(&RefTo->GetPlayable());
    const char* cp = localstr;
    const xstring& err = si->Deserialize(cp, job);
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
  { // Default location => NULL
    int_ptr<Location> localcache;
    localcache.swap(cache);
    return localcache ? CR_Changed : CR_Nop;
  }
}

InfoFlags PlayableSlice::CalcRplCore(AggregateInfo& ai, APlayable& cur, OwnedPlayableSet& exclude, InfoFlags what, JobSet& job, const Location* start, const Location* stop, size_t level)
{ DEBUGLOG(("PlayableSlice::ClacRplCore(, &%p, {%u,}, %x, {%u,}, %p, %p, %i)\n",
    &cur, exclude.size(), what, job.Pri, start, stop, level));
  InfoFlags whatok = what;

 recurse:
  int_ptr<PlayableInstance> psp; // start element
  PM123_TIME ss = -1; // start position
  if (start)
  { if (start->GetLevel() > level)
      psp = start->GetCallstack()[level];
    else if (start->GetLevel() == level)
    { if (job.RequestInfo(cur, IF_Tech))
        whatok = IF_None;
      else if (cur.GetInfo().tech->attributes & TATTR_SONG)
        ss = start->GetPosition();
    }
  }
  int_ptr<PlayableInstance> pep; // stop element
  PM123_TIME es = -1; // stop position
  if (stop)
  { if (stop->GetLevel() > level)
      pep = stop->GetCallstack()[level];
    else if (stop->GetLevel() == level)
    { if (job.RequestInfo(cur, IF_Tech))
        whatok = IF_None;
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
        return whatok;
    } else
    { // both are NULL
      // Either because start and stop is NULL
      // or because both locations point to the same object
      // or because one location points to a song while the other one is NULL.

      InfoFlags what2 = what;
      volatile const AggregateInfo& sai = job.RequestAggregateInfo(cur, exclude, what2);
      whatok &= ~what2;

      if (what2 & IF_Rpl)
        ai.Rpl += sai.Rpl;
      if (what2 & IF_Drpl)
      { if (ss > 0 || es >= 0)
        { PM123_TIME len = sai.Drpl.totallength;
          // time slice
          if (ss < 0)
            ss = 0;
          if (es < 0)
            es = len;
          ai.Drpl.totallength += es - ss;
          // approximate size
          ai.Drpl.totalsize += (es - ss) / len * sai.Drpl.totalsize;
          ai.Drpl.unk_length += sai.Drpl.unk_length;
          ai.Drpl.unk_size += sai.Drpl.unk_size;
        } else
          ai.Drpl += sai.Drpl;
      }
    }
  }
    
  if (psp)
  { if (exclude.add(psp->GetPlayable()))
    { whatok &= CalcRplCore(ai, *psp, exclude, what, job, start, NULL, level+1);
      // Restore previous state.
      exclude.erase(psp->GetPlayable());
    } // else: item in the call stack => ignore entire sub tree. 
  }
  if (pep)
  { if (exclude.add(pep->GetPlayable()))
    { whatok &= CalcRplCore(ai, *pep, exclude, what, job, NULL, stop, level+1);
      // Restore previous state.
      exclude.erase(pep->GetPlayable());
    } // else: item in the call stack => ignore entire sub tree. 
  }

  // Add the range (psp, pep). Exclusive interval!
  if (job.RequestInfo(cur, IF_Child))
    return IF_None;
  Playable& pc = cur.GetPlayable();
  while (psp = pc.GetNext(psp))
  { InfoFlags what2 = what;
    const volatile AggregateInfo& lai = job.RequestAggregateInfo(*psp, exclude, what2);
    whatok &= ~what2;
    if (what2 & IF_Rpl)
      ai.Rpl += lai.Rpl;
    if (what2 & IF_Drpl)
      ai.Drpl += lai.Drpl;
  }
  return whatok;
}

void PlayableSlice::DoLoadInfo(JobSet& job)
{ DEBUGLOG(("PlayableSlice(%p)::DoLoadInfo({%u,})\n", this, job.Pri));
  // Load base info first.
  CallDoLoadInfo(*RefTo, job);

  if (CIC == NULL) // Nothing to do?
    return;

  PlayableChangeArgs args(*this);
  InfoState::Update upd(CIC->DefaultInfo.InfoStat, job.Pri);
  DEBUGLOG(("PlayableSlice::DoLoadInfo: update %x\n", upd.GetWhat()));
  // Prepare slice info.
  if (upd & IF_Slice)
  { int cr = CalcLoc(Item.start, StartCache, SB_Start, job);
    job.Commit();
    cr |= CalcLoc(Item.stop, StopCache, SB_Stop, job);
    job.Commit();
    if (cr & CR_Changed)
      args.Changed |= IF_Slice;
    if (!(cr & CR_Delayed))
      args.Loaded |= IF_Slice;

    if (!args.IsInitial())
    { Mutex::Lock lock(RefTo->GetPlayable().Mtx);
      args.Loaded &= upd.Commit(IF_Slice) | ~IF_Slice;
      if (!args.IsInitial())
      { InfoChange(args);
        args.Reset();
      }
    }
    if (cr & CR_Delayed)
      return;
  }

  // Load aggregate info if any.
  for (CollectionInfo* iep = NULL; CIC->GetNextWorkItem(iep, job.Pri, upd);)
  { // retrieve information
    DEBUGLOG(("PlayableSlice::DoLoadInfo: update aggregate info {%u}\n", iep->Exclude.size()));
    AggregateInfo ai(iep->Exclude);
    // We can be pretty sure that ITEM_INFO is overridden.
    int_ptr<Location> start = StartCache;
    int_ptr<Location> stop  = StopCache;
    OwnedPlayableSet exclude(iep->Exclude); // copy exclusion list
    InfoFlags whatok = CalcRplCore(ai, *RefTo, exclude, upd, job, start, stop, 0);
    job.Commit();

    if (whatok)
    { // At least one info completed
      Mutex::Lock lock(RefTo->GetPlayable().Mtx);
      if ((whatok & IF_Rpl) && iep->Rpl.CmpAssign(ai.Rpl))
        args.Changed |= IF_Rpl;
      if ((whatok & IF_Drpl) && iep->Drpl.CmpAssign(ai.Drpl))
        args.Changed |= IF_Drpl;
      args.Loaded |= upd.Commit(whatok);
      upd.Rollback();

      if (iep->Exclude.size() == 0)
      { // Store RPL info pointers
        Info.rpl  = &iep->Rpl;
        Info.drpl = &iep->Drpl;
      }
    }
  }
  if (!args.IsInitial())
    InfoChange(args);
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

