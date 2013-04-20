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


#include "playableref.h"
#include "playable.h"
#include "location.h"
#include "dependencyinfo.h"
#include <string.h>

#include <debuglog.h>


/****************************************************************************
*
*  class PlayableRef
*
****************************************************************************/

PlayableRef::PlayableRef(APlayable& pp)
: RefTo(&pp)
, Overridden(IF_None)
, InfoDeleg(*this, &PlayableRef::InfoChangeHandler)
{ DEBUGLOG(("PlayableRef(%p)::PlayableRef(APlayable&%p{%s})\n", this, &pp, pp.DebugName().cdata()));
  RefTo->GetInfoChange() += InfoDeleg;
}
PlayableRef::PlayableRef(const PlayableRef& r)
: RefTo(r.RefTo)
, Overridden(IF_None)
, Item(r.Item ? new ItemInfo(*r.Item) : NULL)
, StartCache(r.StartCache)
, StopCache(r.StopCache)
, InfoDeleg(*this, &PlayableRef::InfoChangeHandler)
{ DEBUGLOG(("PlayableRef(%p)::PlayableRef(PlayableRef&%p{%p})\n", this, &r, r.DebugName().cdata()));
  if (RefTo)
    RefTo->GetInfoChange() += InfoDeleg;
}

PlayableRef::~PlayableRef()
{ DEBUGLOG(("PlayableRef(%p)::~PlayableRef() - %p\n", this, RefTo.get()));
  InfoDeleg.detach();
  PlayableChangeArgs args(*this);
  RaiseInfoChange(args);
  // No more events.
  GetInfoChange().reset();
  GetInfoChange().sync();
};

int PlayableRef::CompareSliceBorder(const Location* l, const Location* r, Location::CompareOptions type)
{ DEBUGLOG(("PlayableRef::CompareSliceBorder(%p, %p, %x)\n", l, r, type));
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

xstring PlayableRef::GetDisplayName() const
{ if ((Overridden & IF_Item) && Item->alias)
    return Item->alias;
  if (Overridden & IF_Meta)
  { xstring ret(Meta->title);
    if (ret && ret[0U])
      return ret;
  }
  return RefTo->GetDisplayName();
}

xstring PlayableRef::DoDebugName() const
{ return "@" + RefTo.get()->DebugName();
}

int_ptr<Location> PlayableRef::GetStartLoc() const
{ if (Overridden & IF_Item)
    return StartCache;
  else
    return RefTo->GetStartLoc();
}

int_ptr<Location> PlayableRef::GetStopLoc() const
{ if (Overridden & IF_Item)
    return StopCache;
  else
    return RefTo->GetStopLoc();
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
  dst.meta = Overridden & IF_Meta ? Meta.get() : info.meta;
  dst.attr = Overridden & IF_Attr ? Attr.get() : info.attr;
  dst.item = Overridden & IF_Item ? Item.get() : info.item;
  if (Overridden & IF_Aggreg)
  { dst.rpl  = &CIC->DefaultInfo.Rpl;
    dst.drpl = &CIC->DefaultInfo.Drpl;
  } else
  { dst.rpl  = info.rpl;
    dst.drpl = info.drpl;
  }
  return dst;
}

void PlayableRef::OverrideItem(const ITEM_INFO* item)
{
  #ifdef DEBUG
  if (item)
    DEBUGLOG(("PlayableRef(%p)::OverrideItem({%s, %s,%s, %f,%f, %f})\n", this,
      item->alias.cdata(), item->start.cdata(), item->stop.cdata(), item->pregap, item->postgap, item->gain));
  else
    DEBUGLOG(("PlayableRef(%p)::OverrideItem(<NULL>)\n", this));
  #endif
  if (!item && !(Overridden & IF_Item))
    return; // This is a no-op.
  PlayableChangeArgs args(*this, this, IF_Item, IF_None, IF_None);
  InfoFlags refinvalid = RefTo->RequestInfo(IF_Item, PRI_None);

  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const ITEM_INFO& old_item = (Overridden & IF_Item) ? *Item : (const ITEM_INFO&)*RefTo->GetInfo().item;
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
  { // Ensure Item
    if (!Item)
      Item = new ItemInfo(*item);
    else
      *Item = *item;
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
    // Deduplicate strings
    xstring::deduplicator dedup;
    Item->Deduplicate(dedup);
  } else
  { if (refinvalid & IF_Item)
      args.Loaded = IF_None;
    StartCache.reset();
    StopCache.reset();
    args.Invalidated |= IF_Rpl|IF_Drpl;
    if (CIC)
      CIC->Invalidate(IF_Rpl|IF_Drpl, NULL);
    Overridden &= ~(IF_Item|IF_Aggreg);
  }
  RaiseInfoChange(args);
}

void PlayableRef::OverrideMeta(const META_INFO* meta)
{ DEBUGLOG(("PlayableRef(%p)::OverrideMeta(%p) - %x\n", this, meta, Overridden.get()));
  if (!meta && !(Overridden & IF_Meta))
    return; // no-op
  PlayableChangeArgs args(*this, this, IF_Meta, IF_None, IF_None);
  InfoFlags refinvalid = RefTo->RequestInfo(IF_Meta, PRI_None);

  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const MetaInfo& current = Overridden & IF_Meta ? *Meta : *(const MetaInfo*)RefTo->GetInfo().meta;
  if (meta == NULL)
  { if (refinvalid & IF_Meta)
      args.Loaded = IF_None;
    // revoke overloading
    if (!current.IsInitial())
      args.Changed |= IF_Meta|IF_Display;
    Overridden &= ~IF_Meta;
  } else
  { // Override
    if (!current.Equals(*meta))
      args.Changed |= IF_Meta|IF_Display;
    // Ensure Meta
    if (!Meta)
      Meta = new MetaInfo(*meta);
    else
      *Meta = *meta;
    Overridden |= IF_Meta;
    // Deduplicate strings
    xstring::deduplicator dedup;
    Meta->Deduplicate(dedup);
  }
  RaiseInfoChange(args);
}

void PlayableRef::OverrideAttr(const ATTR_INFO* attr)
{ DEBUGLOG(("PlayableRef(%p)::OverrideAttr(%p) - %x\n", this, attr, Overridden.get()));
  if (!attr && !(Overridden & IF_Attr))
    return; // no-op
  PlayableChangeArgs args(*this, this, IF_Attr, IF_None, IF_None);
  InfoFlags refinvalid = RefTo->RequestInfo(IF_Attr, PRI_None);

  Mutex::Lock lock(RefTo->GetPlayable().Mtx);
  const AttrInfo& current = Overridden & IF_Attr ? *Attr : *(const AttrInfo*)RefTo->GetInfo().attr;
  if (attr == NULL)
  { if (refinvalid & IF_Attr)
      args.Loaded = IF_None;
    // revoke overloading
    if (!current.IsInitial())
      args.Changed |= IF_Attr;
    Overridden &= ~IF_Attr;
  } else
  { // Override
    if (!current.Equals(*attr))
      args.Changed |= IF_Attr;
    // Ensure Attr
    if (!Attr)
      Attr = new AttrInfo(*attr);
    else
      *Attr = *attr;
    Overridden |= IF_Attr;
  }
  if (!args.IsInitial())
    RaiseInfoChange(args);
}

void PlayableRef::AssignInstanceProperties(const PlayableRef& src)
{ ASSERT(RefTo == src.RefTo);
  InfoFlags override = src.GetOverridden();
  OverrideAttr(override & IF_Attr ? src.Attr.get() : NULL);
  OverrideMeta(override & IF_Meta ? src.Meta.get() : NULL);
  OverrideItem(override & IF_Item ? src.Item.get() : NULL);
}

InfoFlags PlayableRef::Invalidate(InfoFlags what, const Playable* source)
{ InfoFlags ret = RefTo->Invalidate(what);
  if (CIC)
    ret |= CIC->DefaultInfo.InfoStat.Invalidate(what)
         | CIC->Invalidate(what, source);
  if (ret)
    RaiseInfoChange(PlayableChangeArgs(*this, this, IF_None, IF_None, ret));
  return ret;
}

void PlayableRef::PeekRequest(RequestState& req) const
{ DEBUGLOG(("PlayableRef(%p)::PeekRequest({%x,%x, %x})\n", this, req.ReqLow, req.ReqHigh, req.InService));
  if (CIC)
  { CIC->DefaultInfo.InfoStat.PeekRequest(req);
    CIC->PeekRequest(req);
  }
  RefTo->PeekRequest(req);
  DEBUGLOG(("PlayableSlice::PeekRequest: {%x,%x, %x}\n", req.ReqLow, req.ReqHigh, req.InService));
}

void PlayableRef::InfoChangeHandler(const PlayableChangeArgs& args)
{ DEBUGLOG(("PlayableRef(%p)::InfoChangeHandler(&{&%p, %p, %x, %x, %x})\n", this,
    &args.Instance, args.Origin, args.Changed, args.Loaded, args.Invalidated));
  PlayableChangeArgs args2(*this, args);
  if (args.Changed & (IF_Rpl|IF_Drpl))
  { if (CIC)
      args2.Invalidated |= CIC->Invalidate(args.Changed & (IF_Rpl|IF_Drpl), args.Origin ? &args.Origin->GetPlayable() : NULL);
  }
  args2.Purge((InfoFlags)Overridden.get() & (IF_Item|IF_Meta|IF_Attr));
  if (!args2.IsInitial())
    RaiseInfoChange(args2);
}

InfoFlags PlayableRef::DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("PlayableRef(%p{%s})::DoRequestInfo(%x, %x, %d)\n", this, DebugName().cdata(), what, pri, rel));

  if (!(Overridden & IF_Item))
  { //return CallDoRequestInfo(*RefTo, what, pri, rel);
    what &= RefTo->RequestInfo(what, pri, rel);
    // No async request with respect to THIS instance.
    return IF_None;
  }

  if (what & IF_Drpl)
    what |= IF_Phys|IF_Tech|IF_Obj|IF_Child|IF_Slice; // required for DRPL_INFO aggregate
  else if (what & IF_Rpl)
    what |= IF_Tech|IF_Child|IF_Slice; // required for RPL_INFO aggregate

  // IF_Item is always available if we got beyond IsItemOverridden.
  what &= ~(IF_Item|IF_Usage);
  InfoFlags what2 = what;
  // Forward all remaining requests to *RefTo.
  // TODO Forward aggregate only if no slice?
  //InfoFlags async = CallDoRequestInfo(*RefTo, what2, pri, rel);
  //InfoFlags async = RefTo->RequestInfo(what2, pri, rel);
  what2 &= RefTo->RequestInfo(what2, pri, rel);

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

  InfoFlags async = IF_None;
  if (pri != PRI_None && what != IF_None)
    // Place request for slice
    async |= infostat.Request(what, pri);

  what |= what2;
  return async;
}

/*AggregateInfo& PlayableRef::DoAILookup(const PlayableSetBase& exclude)
{ DEBUGLOG(("PlayableRef(%p)::DoAILookup({%u,})\n", this, exclude.size()));

  if ( !IsSlice()
    && (!(Overridden & IF_Attr) || (Attr && (Attr->ploptions & PLO_ALTERNATION))) )
    return CallDoAILookup(*RefTo, exclude);

  EnsureCIC();
  AggregateInfo* pai = CIC->Lookup(exclude);
  return pai ? *pai : CIC->DefaultInfo;
}*/

InfoFlags PlayableRef::DoRequestAI(const PlayableSetBase& exclude, const volatile AggregateInfo*& ai, InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("PlayableRef(%p)::DoRequestAI({%s},, %x, %x, %d)\n", this, exclude.DebugDump(), what, pri, rel));

  if ( !IsSlice()
    && (!(Overridden & IF_Attr) || (Attr && (Attr->ploptions & PLO_ALTERNATION))) )
  { //return CallDoAILookup(*RefTo, exclude);
    ai = &RefTo->RequestAggregateInfo(exclude, what, pri, rel);
    return IF_None;
  }

  EnsureCIC();
  ai = CIC->Lookup(exclude);
  if (!ai)
    ai = &CIC->DefaultInfo;

  /*// We have to check whether the supplied AggregateInfo is mine or from *RefTo.
  if (!CIC || !CIC->IsMine(ai))
    // from *RefTo
    //return CallDoRequestAI(*RefTo, ai, what, pri, rel);
    return RefTo->RequestAggregateInfo(what, pri, rel);*/

  InfoFlags what2 = IF_None;
  if (what & IF_Drpl)
    what2 = IF_Phys|IF_Tech|IF_Obj|IF_Child|IF_Slice; // required for DRPL_INFO aggregate
  else if (what & IF_Rpl)
    what2 = IF_Tech|IF_Child|IF_Slice; // required for RPL_INFO aggregate
  what2 = DoRequestInfo(what2, pri, rel);

  return ((CollectionInfo*)ai)->RequestAI(what, pri, rel) | what2;
}


bool PlayableRef::CalcLoc(const volatile xstring& strloc, volatile int_ptr<Location>& cache, Location::CompareOptions type, JobSet& job)
{ xstring localstr = strloc;
  DEBUGLOG(("PlayableRef(%p)::CalcLoc(&%s, &%p, %x, {%x,})\n", this, localstr.cdata(), cache.debug(), type, job.Pri));
  bool changed = false;
  if (localstr)
  { ASSERT(!RefTo->RequestInfo(IF_Slice, PRI_None, REL_Invalid));
    int_ptr<Location> newvalue = new Location(&RefTo->GetPlayable());
    const char* cp = localstr;
    const xstring& err = newvalue->Deserialize(job, cp);
    if (err)
    { if (err.length() == 0) // delayed
        goto end;
      // TODO: Errors
      DEBUGLOG(("PlayableRef::CalcLoc: %s at %.20s\n", err.cdata(), cp));
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
    changed = !oldvalue || *oldvalue != *newvalue;
  } else
  { // Default location => NULL
    int_ptr<Location> oldvalue;
    oldvalue.swap(cache);
    changed = oldvalue != NULL;
  }
 end:
  DEBUGLOG(("PlayableRef::CalcLoc: %u\n", changed));
  return changed;
}

void PlayableRef::DoLoadInfo(JobSet& job)
{ DEBUGLOG(("PlayableRef(%p{%s})::DoLoadInfo({%x,})\n", this, DebugName().cdata(), job.Pri));
  // Load base info first.
  //CallDoLoadInfo(*RefTo, job);

  if (CIC == NULL) // Nothing to do?
    return;

  InfoState::Update upd(CIC->DefaultInfo.InfoStat, job.Pri);
  DEBUGLOG(("PlayableRef::DoLoadInfo: update %x\n", upd.GetWhat()));
  // Prepare slice info.
  PlayableChangeArgs args(*this);
 retry1:
  if (upd & IF_Slice)
  { if (CalcLoc(Item->start, StartCache, Location::CO_Default, job))
      args.Changed |= IF_Slice;
    bool delayed = job.Commit();
    if (CalcLoc(Item->stop, StopCache, Location::CO_Reverse, job))
      args.Changed |= IF_Slice;
    delayed |= job.Commit();

    Mutex::Lock lock(RefTo->GetPlayable().Mtx);
    if (delayed)
      upd.Rollback(IF_Slice);
    else
    { args.Loaded |= upd.Commit(IF_Slice);
      if (!args.Loaded)
        goto retry1;
    }
    if (!args.IsInitial())
      RaiseInfoChange(args);
    if (delayed)
      return;
  }
  args.Reset();

  // Load aggregate info if any.
  int_ptr<Location> start = StartCache;
  int_ptr<Location> stop  = StopCache;
  OwnedPlayableSet exclude; // exclusion list
  CollectionInfo* iep = NULL;
  if (!upd)
    goto next;
  do
  { { // retrieve information
      CollectionInfo& ci = iep ? *iep : CIC->DefaultInfo;
     retry2:
      DEBUGLOG(("PlayableRef::DoLoadInfo: update aggregate info {%u}\n", ci.Exclude.size()));
      AggregateInfo ai(ci.Exclude);
      exclude = ci.Exclude; // copy exclusion list
      // We can be pretty sure that ITEM_INFO is overridden.
      InfoFlags whatnotok = RefTo->AddSliceAggregate(ai, exclude, upd, job, start, stop);
      InfoFlags whatok = upd & IF_Aggreg & ~whatnotok;
      job.Commit();

      Mutex::Lock lock(RefTo->GetPlayable().Mtx);
      if ((upd & IF_Rpl) && ci.Rpl.CmpAssign(ai.Rpl))
        args.Changed |= IF_Rpl;
      if ((upd & IF_Drpl) && ci.Drpl.CmpAssign(ai.Drpl))
        args.Changed |= IF_Drpl;

      upd.Rollback(whatnotok);
      InfoFlags done = upd.Commit(whatok);
      args.Loaded |= done;
      if (~done & whatok)
      { upd.Reset(ci.InfoStat, job.Pri);
        goto retry2;
      }

      if (iep == NULL)
        Overridden |= whatok;
    }
   next:;
  } while (CIC->GetNextWorkItem(iep, job.Pri, upd));

  if (!args.IsInitial())
  { Mutex::Lock lock(RefTo->GetPlayable().Mtx);
    RaiseInfoChange(args);
  }
}

const Playable& PlayableRef::DoGetPlayable() const
{ return RefTo->GetPlayable();
}

void PlayableRef::SetInUse(unsigned used)
{ DEBUGLOG(("PlayableRef(%p)::SetInUse(%u)\n", this, used));
  bool changed = InUse != used;
  InUse = used;
  // TODO: keep origin in case of cascaded execution
  PlayableChangeArgs args(*this, this, IF_Usage, changed * IF_Usage, IF_None);
  RaiseInfoChange(args);
  RefTo->SetInUse(used);
}
