/*
 * Copyright 2007-2012 M.Mueller
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
#include "dependencyinfo.h"
#include <string.h>

#include <debuglog.h>


/****************************************************************************
*
*  class PlayableSlice
*
****************************************************************************/

PlayableSlice::PlayableSlice(APlayable& pp)
: RefTo(&pp)
, Overridden(IF_None)
, InfoDeleg(*this, &PlayableSlice::InfoChangeHandler)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(APlayable&%p{%s})\n", this, &pp, pp.DebugName().cdata()));
  RefTo->GetInfoChange() += InfoDeleg;
}
PlayableSlice::PlayableSlice(const PlayableSlice& r)
: RefTo(r.RefTo)
, Overridden(IF_None)
, Item(r.Item)
, StartCache(r.StartCache)
, StopCache(r.StopCache)
, InfoDeleg(*this, &PlayableSlice::InfoChangeHandler)
{ DEBUGLOG(("PlayableSlice(%p)::PlayableSlice(PlayableSlice&%p{%p})\n", this, &r, r.DebugName().cdata()));
  if (RefTo)
    RefTo->GetInfoChange() += InfoDeleg;
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

int PlayableSlice::CompareSliceBorder(const Location* l, const Location* r, Location::CompareOptions type)
{ DEBUGLOG(("PlayableSlice::CompareSliceBorder(%p, %p, %x)\n", l, r, type));
  if (l == NULL)
  { if (r == NULL)
      return 0; // both iterators are zero
    else
      return (type & Location::CO_Reverse) ? 1 : -1;
  } else
  { if (r == NULL)
      return (type & Location::CO_Reverse) ? -1 : 1;
    else
      return l->CompareTo(*r);
  }
}

xstring PlayableSlice::GetDisplayName() const
{ return (Overridden & IF_Item) && Item.alias ? Item.alias : RefTo->GetDisplayName();
}

xstring PlayableSlice::DoDebugName() const
{ return "@" + RefTo.get()->DebugName();
}

int_ptr<Location> PlayableSlice::GetStartLoc() const
{ if (Overridden & IF_Item)
    return StartCache;
  else
    return RefTo->GetStartLoc();
}

int_ptr<Location> PlayableSlice::GetStopLoc() const
{ if (Overridden & IF_Item)
    return StopCache;
  else
    return RefTo->GetStopLoc();
}

const INFO_BUNDLE_CV& PlayableSlice::GetInfo() const
{ const INFO_BUNDLE_CV& info = RefTo->GetInfo();
  if (!(Overridden & IF_Item))
    return info;
  // Updated pointers of inherited info
  INFO_BUNDLE_CV& dst = (INFO_BUNDLE_CV&)Info; // Access mutable field
  dst.phys = info.phys;
  dst.tech = info.tech;
  dst.obj  = info.obj;
  dst.meta = info.meta;
  dst.attr = info.attr;
  if (Overridden & IF_Aggreg)
  { dst.rpl  = &CIC->DefaultInfo.Rpl;
    dst.drpl = &CIC->DefaultInfo.Drpl;
  } else
  { dst.rpl  = info.rpl;
    dst.drpl = info.drpl;
  }
  return dst;
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
  if (!item && !(Overridden & IF_Item))
    return; // This is a no-op.
  PlayableChangeArgs args(*this, this, IF_Item, IF_None, IF_None);
  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const ITEM_INFO& old_item = (Overridden & IF_Item) ? Item : (const ITEM_INFO&)*RefTo->GetInfo().item;
  const ITEM_INFO& new_item = item ? *item : (const ITEM_INFO&)*RefTo->GetInfo().item;
  // Analyze changed flags
  const bool startchanged = new_item.start != old_item.start;
  const bool stopchanged  = new_item.stop != old_item.stop;
  if (startchanged|stopchanged)
  { args.Invalidated |= IF_Slice;
    args.Changed |= IF_Item;
  }
  if (new_item.alias != old_item.alias)
    args.Changed |= IF_Item|IF_Display;
  else if (new_item.pregap != old_item.pregap || new_item.postgap != old_item.postgap || new_item.gain != old_item.gain)
    args.Changed |= IF_Item;
  if (item)  
  { Item = *item;
    if (startchanged)
      StartCache.reset();
    if (stopchanged)
      StopCache.reset();
    if (!item->start && !item->stop)
      Overridden &= ~IF_Aggreg;
    if (startchanged|stopchanged)
    { args.Invalidated |= IF_Rpl|IF_Drpl;
      if (CIC)
        CIC->Invalidate(IF_Rpl|IF_Drpl, NULL);
    }
    Overridden |= IF_Item;
  } else
  { StartCache.reset();
    StopCache.reset();
    args.Invalidated |= IF_Rpl|IF_Drpl;
    if (CIC)
      CIC->Invalidate(IF_Rpl|IF_Drpl, NULL);
    Overridden &= ~(IF_Item|IF_Aggreg);
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
  if (Overridden & IF_Item)
    args2.Purge(IF_Item);
  if (!args2.IsInitial())
    InfoChange(args2);
}

InfoFlags PlayableSlice::DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("PlayableSlice(%p{%s})::DoRequestInfo(%x, %d, %d)\n", this, DebugName().cdata(), what, pri, rel));

  if (!(Overridden & IF_Item))
    return CallDoRequestInfo(*RefTo, what, pri, rel);

  if (what & IF_Drpl)
    what |= IF_Phys|IF_Tech|IF_Obj|IF_Child|IF_Slice; // required for DRPL_INFO aggregate
  else if (what & IF_Rpl)
    what |= IF_Tech|IF_Child|IF_Slice; // required for RPL_INFO aggregate

  // IF_Item is always available if we got beyond IsItemOverridden.
  what &= ~(IF_Item|IF_Usage);
  InfoFlags what2 = what;
  // Forward all remaining requests to *RefTo.
  // TODO Forward aggregate only if no slice?
  InfoFlags async = CallDoRequestInfo(*RefTo, what2, pri, rel);

  EnsureCIC();
  InfoState& infostat = CIC->DefaultInfo.InfoStat;
  what &= infostat.Check(what & (IF_Slice|IF_Aggreg), rel);

  /* if (what & IF_Slice)
    // Fastpath for IF_Slice?
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

  if (pri != PRI_None && what != IF_None)
    // Place request for slice
    async |= infostat.Request(what, pri);

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
{ DEBUGLOG(("PlayableSlice(%p{%s})::DoRequestAI(&%p, %x, %d, %d)\n", this, DebugName().cdata(), &ai, what, pri, rel));

  // We have to check whether the supplied AggregateInfo is mine or from *RefTo.
  if (!CIC || !CIC->IsMine(ai))
    // from *RefTo
    return CallDoRequestAI(*RefTo, ai, what, pri, rel);

  InfoFlags what2 = IF_None;
  if (what & IF_Drpl)
    what2 = IF_Phys|IF_Tech|IF_Obj|IF_Child|IF_Slice; // required for DRPL_INFO aggregate
  else if (what & IF_Rpl)
    what2 = IF_Tech|IF_Child|IF_Slice; // required for RPL_INFO aggregate
  DoRequestInfo(what2, pri, rel);

  return ((CollectionInfo&)ai).RequestAI(what, pri, rel);
}


PlayableSlice::CalcResult PlayableSlice::CalcLoc(const volatile xstring& strloc, volatile int_ptr<Location>& cache, Location::CompareOptions type, JobSet& job)
{ xstring localstr = strloc;
  DEBUGLOG(("PlayableSlice(%p)::CalcLoc(&%s, &%p, %x, {%u,})\n", this, localstr.cdata(), cache.debug(), type, job.Pri));
  if (localstr)
  { ASSERT(!RefTo->RequestInfo(IF_Slice, PRI_None, REL_Invalid));
    int_ptr<Location> newvalue = new Location(&RefTo->GetPlayable());
    const char* cp = localstr;
    const xstring& err = newvalue->Deserialize(job, cp);
    if (err)
    { if (err.length() == 0) // delayed
        return CR_Delayed;
      // TODO: Errors
      DEBUGLOG(("PlayableSlice::CalcLoc: %s at %.20s\n", err.cdata(), cp));
    }
    // Intersection with RefTo->GetStart/StopLoc()
    if (type & Location::CO_Reverse)
    { int_ptr<Location> refstart = RefTo->GetStopLoc();
      if (CompareSliceBorder(newvalue, refstart, type) >= 0)
        newvalue = refstart;
    } else
    { int_ptr<Location> refstop = RefTo->GetStartLoc();
      if (CompareSliceBorder(newvalue, refstop, type) <= 0)
        newvalue = refstop;
    }
    // commit
    int_ptr<Location> oldvalue = newvalue;
    oldvalue.swap(cache);
    return oldvalue && *oldvalue == *newvalue ? CR_Nop : CR_Changed;
  } else
  { // Default location => NULL
    int_ptr<Location> oldvalue;
    oldvalue.swap(cache);
    return oldvalue ? CR_Changed : CR_Nop;
  }
}

void PlayableSlice::DoLoadInfo(JobSet& job)
{ DEBUGLOG(("PlayableSlice(%p{%s})::DoLoadInfo({%u,})\n", this, DebugName().cdata(), job.Pri));
  // Load base info first.
  CallDoLoadInfo(*RefTo, job);

  if (CIC == NULL) // Nothing to do?
    return;

  PlayableChangeArgs args(*this);
  InfoState::Update upd(CIC->DefaultInfo.InfoStat, job.Pri);
  DEBUGLOG(("PlayableSlice::DoLoadInfo: update %x\n", upd.GetWhat()));
  // Prepare slice info.
  if (upd & IF_Slice)
  { int cr = CalcLoc(Item.start, StartCache, Location::CO_Default, job);
    job.Commit();
    cr |= CalcLoc(Item.stop, StopCache, Location::CO_Reverse, job);
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
  if (upd)
  { DEBUGLOG(("PlayableSlice::DoLoadInfo: update aggregate info {0}\n"));
    AggregateInfo ai(PlayableSet::Empty);
    OwnedPlayableSet exclude; // exclusion list
    // We can be pretty sure that ITEM_INFO is overridden.
    int_ptr<Location> start = StartCache;
    int_ptr<Location> stop  = StopCache;
    InfoFlags whatnotok = RefTo->AddSliceAggregate(ai, exclude, upd, job, start, stop, 0);
    job.Commit();
    upd.Rollback(whatnotok);

    InfoFlags whatok = upd & IF_Aggreg & ~whatnotok;
    if (whatok)
    { // At least one info completed
      Mutex::Lock lock(RefTo->GetPlayable().Mtx);
      if ((whatok & IF_Rpl) && CIC->DefaultInfo.Rpl.CmpAssign(ai.Rpl))
        args.Changed |= IF_Rpl;
      if ((whatok & IF_Drpl) && CIC->DefaultInfo.Drpl.CmpAssign(ai.Drpl))
        args.Changed |= IF_Drpl;
      args.Loaded |= upd.Commit(IF_Aggreg);
      Overridden |= IF_Aggreg;
    }
  }
  for (CollectionInfo* iep = NULL; CIC->GetNextWorkItem(iep, job.Pri, upd);)
  { // retrieve information
    DEBUGLOG(("PlayableSlice::DoLoadInfo: update aggregate info {%u}\n", iep->Exclude.size()));
    AggregateInfo ai(iep->Exclude);
    OwnedPlayableSet exclude(iep->Exclude); // copy exclusion list
    // We can be pretty sure that ITEM_INFO is overridden.
    int_ptr<Location> start = StartCache;
    int_ptr<Location> stop  = StopCache;
    InfoFlags whatnotok = RefTo->AddSliceAggregate(ai, exclude, upd, job, start, stop, 0);
    job.Commit();
    upd.Rollback(whatnotok);

    InfoFlags whatok = upd & IF_Aggreg & ~whatnotok;
    if (whatok)
    { // At least one info completed
      Mutex::Lock lock(RefTo->GetPlayable().Mtx);
      if ((whatok & IF_Rpl) && iep->Rpl.CmpAssign(ai.Rpl))
        args.Changed |= IF_Rpl;
      if ((whatok & IF_Drpl) && iep->Drpl.CmpAssign(ai.Drpl))
        args.Changed |= IF_Drpl;
      args.Loaded |= upd.Commit(whatok);
    }
  }
  if (!args.IsInitial())
    InfoChange(args);
}

const Playable& PlayableSlice::DoGetPlayable() const
{ return RefTo->GetPlayable();
}

void PlayableSlice::SetInUse(unsigned used)
{ DEBUGLOG(("PlayableSlice(%p)::SetInUse(%u)\n", this, used));
  bool changed = InUse != used;
  InUse = used;
  // TODO: keep origin in case of cascaded execution
  PlayableChangeArgs args(*this, this, IF_Usage, changed * IF_Usage, IF_None);
  InfoChange(args);
  RefTo->SetInUse(used);
}


/****************************************************************************
*
*  class PlayableRef
*
****************************************************************************/

xstring PlayableRef::GetDisplayName() const
{ if ((Overridden & IF_Item) && Item.alias)
    return Item.alias;
  if (Overridden & IF_Meta)
  { xstring ret(Info.meta->title);
    if (ret && ret[0U])
      return ret;
  }
  return RefTo->GetDisplayName();
}

const INFO_BUNDLE_CV& PlayableRef::GetInfo() const
{ const INFO_BUNDLE_CV& info = RefTo->GetInfo();
  if (!Overridden)
    return info;
  // Updated pointers of inherited info
  INFO_BUNDLE_CV& dst = (INFO_BUNDLE_CV&)Info; // Access mutable field
  dst.phys = info.phys;
  dst.tech = info.tech;
  dst.obj  = info.obj;
  dst.meta = Overridden & IF_Meta ? &Meta : info.meta;
  dst.attr = Overridden & IF_Attr ? &Attr : info.attr;
  dst.item = Overridden & IF_Item ? &Item : info.item;
  if (Overridden & IF_Aggreg)
  { dst.rpl  = &CIC->DefaultInfo.Rpl;
    dst.drpl = &CIC->DefaultInfo.Drpl;
  } else
  { dst.rpl  = info.rpl;
    dst.drpl = info.drpl;
  }
  return dst;
}

/*InfoFlags PlayableRef::GetOverridden() const
{ return PlayableSlice::GetOverridden()
    | IF_Meta * (Info.meta == &Meta)
    | IF_Attr * (Info.attr == &Attr);
}*/

void PlayableRef::OverrideMeta(const META_INFO* meta)
{ DEBUGLOG(("PlayableRef(%p)::OverrideMeta(%p) - %x\n", this, meta, Overridden.get()));
  if (!meta && !(Overridden & IF_Meta))
    return; // no-op
  PlayableChangeArgs args(*this, this, IF_Meta, IF_None, IF_None);
  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const MetaInfo& current = Overridden & IF_Meta ? Meta : *(const MetaInfo*)RefTo->GetInfo().meta;
  if (meta == NULL)
  { // revoke overloading
    if (!current.IsInitial())
      args.Changed |= IF_Meta|IF_Display;
    Overridden &= ~IF_Meta;
  } else
  { // Override
    if (!current.Equals(*meta))
      args.Changed |= IF_Meta|IF_Display;
    Meta = *meta;
    Overridden |= IF_Meta;
  }
  if (!args.IsInitial())
    InfoChange(args);
}

void PlayableRef::OverrideAttr(const ATTR_INFO* attr)
{ DEBUGLOG(("PlayableRef(%p)::OverrideAttr(%p) - %x\n", this, attr, Overridden.get()));
  if (!attr && !(Overridden & IF_Attr))
    return; // no-op
  PlayableChangeArgs args(*this, this, IF_Attr, IF_None, IF_None);
  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const AttrInfo& current = Overridden & IF_Attr ? Attr : *(const AttrInfo*)RefTo->GetInfo().attr;
  if (attr == NULL)
  { // revoke overloading
    if (!current.IsInitial())
      args.Changed |= IF_Attr;
    Overridden &= ~IF_Attr;
  } else
  { // Override
    if (!current.Equals(*attr))
      args.Changed |= IF_Attr;
    Attr = *attr;
    Overridden |= IF_Attr;
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
  InfoFlags override = src.GetOverridden();
  OverrideAttr(override & IF_Attr ? &src.Attr : NULL);
  OverrideMeta(override & IF_Meta ? &src.Meta : NULL);
  OverrideItem(override & IF_Item ? &src.Item : NULL);
}

void PlayableRef::InfoChangeHandler(const PlayableChangeArgs& args)
{ DEBUGLOG(("PlayableRef(%p)::InfoChangeHandler(&{&%p, %p, %x, %x, %x})\n", this,
    &args.Instance, args.Origin, args.Changed, args.Loaded, args.Invalidated));
  PlayableChangeArgs args2(args);
  if (Overridden & IF_Meta)
  { args2.Changed     &= ~IF_Meta;
    args2.Invalidated &= ~IF_Meta;
  }
  if (Overridden & IF_Attr)
  { args2.Changed     &= ~IF_Attr;
    args2.Invalidated &= ~IF_Attr;
  }
  PlayableSlice::InfoChangeHandler(args2);
}

