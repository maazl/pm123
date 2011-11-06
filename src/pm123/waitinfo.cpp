/*
 * Copyright 2010-2011 M.Mueller
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


#include "waitinfo.h"
#include "playableset.h"
#include "playable.h"

#include <debuglog.h>


/****************************************************************************
*
*  class WaitLoadInfo
*
****************************************************************************/

WaitLoadInfo::WaitLoadInfo()
: Filter(IF_None),
  Deleg(*this, &WaitLoadInfo::InfoChangeEvent)
{ DEBUGLOG(("WaitLoadInfo(%p)::WaitInfo()\n", this));
}
WaitLoadInfo::WaitLoadInfo(APlayable& inst, InfoFlags filter)
: Filter(filter),
  Deleg(inst.GetInfoChange(), *this, &WaitLoadInfo::InfoChangeEvent)
{ DEBUGLOG(("WaitLoadInfo(%p)::WaitInfo(&%p, %x)\n", this, &inst, filter));
  CommitInfo(IF_None);
}

void WaitLoadInfo::Start(APlayable& inst, InfoFlags filter)
{ DEBUGLOG(("WaitLoadInfo(%p)::Start(&%p, %x)\n", this, &inst, filter));
  Deleg.detach();
  Filter = filter;
  inst.GetInfoChange() += Deleg;
  CommitInfo(IF_None);
}

void WaitLoadInfo::InfoChangeEvent(const PlayableChangeArgs& args)
{ DEBUGLOG(("WaitLoadInfo(%p)::InfoChangeEvent({&%p, %x, %x}) - %x\n",
    this, &args.Instance, args.Changed, args.Loaded, Filter));
  ASSERT(!args.IsInitial()); // Owner died! - This must not happen.
  CommitInfo(args.Loaded);
}

void WaitLoadInfo::CommitInfo(InfoFlags what)
{ DEBUGLOG(("WaitLoadInfo(%p)::CommitInfo(%x) - %x\n", this, what, Filter));
  Filter &= ~what;
  if (!Filter)
  { Deleg.detach();
    EventSem.Set();
  }
}


/****************************************************************************
*
*  class DependencyInfoPath
*
****************************************************************************/

int DependencyInfoPath::Entry::compare(const Entry& l, const APlayable& r)
{ // Arbitrary but reliable sequence.
  return (int)l.Inst.get() - (int)&r;
}

void DependencyInfoPath::Add(APlayable& inst, InfoFlags what)
{ DEBUGLOG(("DependencyInfoPath::Add(&%p, %x)\n", &inst, what));
  Entry*& ep = MandatorySet.get(inst);
  if (ep)
    // Hit => merge
    ep->What |= what;
  else
    // No hit => insert
    ep = new Entry(inst, what);
}

#ifdef DEBUG_LOG
void DependencyInfoPath::DumpSet(xstringbuilder& dest, const SetType& set)
{ for (Entry*const* epp = set.begin(); epp != set.end(); ++epp)
  { if (epp != set.begin())
      dest.append(", ");
    const Entry& entry = **epp;
    dest.appendf("%p{%s} : %x", &entry.Inst, entry.Inst->GetPlayable().URL.getShortName().cdata(), entry.What);
  }
}

xstring DependencyInfoPath::DebugDump() const
{ xstringbuilder sb;
  sb.append("MandatorySet = ");
  DumpSet(sb, MandatorySet);
  return sb;
}
#endif

/****************************************************************************
*
*  class DependencyInfoSet
*
****************************************************************************/

void DependencyInfoSet::Join(DependencyInfoPath& r)
{ DEBUGLOG(("DependencyInfoSet(%p{{%u,},{%u,}})::Join(&{{%u,}})\n",
    this, MandatorySet.size(), OptionalSet.size(), r.Size()));
  if (r.Size() == 0)
    return; // no-op
  if (Size() == 0)
  { DependencyInfoPath::Swap(r);
    return;
  }
  // Calculate intersection of mandatory sets.
  int outer = 0; // 1 = right outer set exists, 2 = left outer set exists
  size_t li = 0;
  size_t ri = 0;
  Entry* le = li < MandatorySet.size() ? MandatorySet[li] : NULL;
  Entry* re = ri < r.Size() ? AccessMandatorySet(r)[ri] : NULL;
  SetType roset; // right outer set
  while (le || re)
  { if (le == NULL || le->Inst.get() > re->Inst.get())
    { // current right entry does not match
      outer |= 1;
      Entry*& lo = OptionalSet.get(*re->Inst);
      if (lo)
      { lo->What |= re -> What;
        ++ri;
      } else
      { lo = re;
        AccessMandatorySet(r).erase(ri);
      }
      re = ri < r.Size() ? AccessMandatorySet(r)[ri] : NULL;
    } else if (re == NULL || le->Inst.get() < re->Inst.get())
    { // current left entry does not match
      outer |= 2;
      Entry*& ro = roset.get(*le->Inst);
      if (ro)
      { ro->What |= le->What;
        ++li;
      } else
      { ro = le;
        MandatorySet.erase(li);
      }
      le = li < MandatorySet.size() ? MandatorySet[li] : NULL;
    } // else match
    { le->What |= re->What;
      le = ++li < MandatorySet.size() ? MandatorySet[li] : NULL;
      re = ++ri < r.Size() ? AccessMandatorySet(r)[ri] : NULL;
    }
  }
  // 0: mandatory Sets are equal
  // 1: left mandatory set is subset of right mandatory set
  // 2: right mandatory set is subset of left mandatory set
  // 3: mandatory sets overlap partially
  if (outer == 1 || outer == 2)
  { // => Take intersection of optional sets
    li = 0;
    ri = 0;
    le = li < OptionalSet.size() ? OptionalSet[li] : NULL;
    re = ri < roset.size() ? roset[ri] : NULL;
    while (le || re)
    { if (le == NULL || le->Inst.get() > re->Inst.get())
      { // current right entry does not match
      } else if (re == NULL || le->Inst.get() < re->Inst.get())
      { // current left entry does not match
        OptionalSet.erase(li);
        le = li < OptionalSet.size() ? OptionalSet[li] : NULL;
        continue;
      } // else match
      { le->What |= re->What;
        le = ++li < OptionalSet.size() ? OptionalSet[li] : NULL;
      }
      re = ++ri < roset.size() ? roset[ri] : NULL;
    }
  } else
  { // Calculate union of optional sets
    li = 0;
    ri = 0;
    le = li < OptionalSet.size() ? OptionalSet[li] : NULL;
    re = ri < roset.size() ? roset[ri] : NULL;
    while (le || re)
    { if (le == NULL || le->Inst.get() > re->Inst.get())
      { // current right entry does not match
        roset.erase(ri);
        OptionalSet.insert(li) = re;
        ++li;
        re = ri < roset.size() ? roset[ri] : NULL;
        continue;
      } else if (re == NULL || le->Inst.get() < re->Inst.get())
      { // current left entry does not match
      } // else match
      { le->What |= re->What;
        re = ++ri < roset.size() ? roset[ri] : NULL;
      }
      le = ++li < OptionalSet.size() ? OptionalSet[li] : NULL;
    }
  }
  r.Clear();
}

#ifdef DEBUG_LOG
xstring DependencyInfoSet::DebugDump() const
{ xstringbuilder sb;
  sb.append("MandatorySet = ");
  DumpSet(sb, MandatorySet);
  sb.append("; OptionalSet = ");
  DumpSet(sb, OptionalSet);
  return sb;
}
#endif

/****************************************************************************
*
*  class DependencyInfoWorker
*
****************************************************************************/

Mutex DependencyInfoWorker::Mtx;

DependencyInfoWorker::DependencyInfoWorker()
: Deleg(*this, &DependencyInfoWorker::MandatoryInfoChangeEvent)
{ DEBUGLOG(("DependencyInfoWorker(%p)::DependencyInfoWorker()\n", this));
}

void DependencyInfoWorker::Start()
{ DEBUGLOG(("DependencyInfoWorker(%p)::Start() - %u\n", this, Data.Size()));
  for (;;)
  { sco_ptr<DependencyInfoSet::Entry> ep; // Defined before lock => released after lock
    Mutex::Lock lock(Mtx);
    if (Data.MandatorySet.size() == 0) // Completed?
    { // Mandatory dependencies completed, check optional dependencies now.
      if (Data.OptionalSet.size() == 0)
        break;
      DelegList.reserve(Data.OptionalSet.size());
      size_t i = 0;
      while (i < Data.OptionalSet.size())
      { DependencyInfoSet::Entry* ep2 = Data.OptionalSet[i];
        DEBUGLOG(("DependencyInfoWorker::Start checking optional entry %p{%s} : %x\n",
          &ep2->Inst, ep2->Inst->GetPlayable().URL.getShortName().cdata(), ep2->What));
        DelegList.append() = new DelegType(ep2->Inst->GetInfoChange(), *this, &DependencyInfoWorker::OptionalInfoChangeEvent);
        ep2->What = ep2->Inst->RequestInfo(ep2->What, PRI_None);
        if (ep2->What == IF_None)
        { DelegList.clear();
          goto completed;
        }
      }
      // Now wait for optional entry.
      goto wait;
    }
    ep = Data.MandatorySet.erase(Data.MandatorySet.size()-1);
    DEBUGLOG(("DependencyInfoWorker::Start checking mandatory dependency %p{%s} : %x\n",
      &ep->Inst, ep->Inst->GetPlayable().URL.getShortName().cdata(), ep->What));
    ep->What = ep->Inst->RequestInfo(ep->What, PRI_None);
    if (ep->What)
    { DEBUGLOG(("DependencyInfoWorker::Start still waiting for %x\n", ep->What));
      NowWaitingFor = ep->What;
      ep->Inst->GetInfoChange() += Deleg;
      // double check
      ep->What = ep->Inst->RequestInfo(ep->What, PRI_None);
      if ( ep->What != IF_None // no double check hit
        || !Deleg.detach() )   // or /we/ did not deregister ourself
        goto wait;             // => wait for other thread
      // else move on with the next queue item
    }
    DEBUGLOG(("DependencyInfoWorker::Start item has already completed.\n"));
  }
 completed:
  // Completed (handled outside the mutex)
  OnCompleted();
 wait:
  DEBUGLOG(("DependencyInfoWorker::Start done\n"));
}

void DependencyInfoWorker::Cancel()
{ DEBUGLOG(("DependencyInfoWorker(%p)::Cancel()\n", this));
  Mutex::Lock lock(Mtx);
  Deleg.detach();
  DelegList.clear();
}

void DependencyInfoWorker::MandatoryInfoChangeEvent(const PlayableChangeArgs& args)
{ DEBUGLOG(("DependencyInfoWorker(%p)::MandatoryInfoChangeEvent({&%p})\n", &args.Instance));
  NowWaitingFor &= ~args.Loaded;
  if ( NowWaitingFor == IF_None // Have we finished?
    && Deleg.detach() ) // and did /we/ deregister ourself?
    // => look for further infos.
    Start();
  // else we have either not finished or someone else deregistered faster.
}

void DependencyInfoWorker::OptionalInfoChangeEvent(const PlayableChangeArgs& args)
{ DEBUGLOG(("DependencyInfoWorker(%p)::MandatoryInfoChangeEvent({&%p})\n", &args.Instance));
  { Mutex::Lock lock(Mtx);
    DependencyInfoSet::Entry* ep = Data.OptionalSet.find(args.Instance);
    if (ep == NULL)
      return; // Someone else did the job
    ep->What &= ~args.Loaded;
    // Info complete?
    if (ep->What != IF_None)
      return; // no
    Data.OptionalSet.clear();
  }
  OnCompleted();
}


/****************************************************************************
*
*  class WaitDependencyInfo
*
****************************************************************************/

/*WaitDependencyInfo::WaitDependencyInfo(DependencyInfoQueue& data)
: DependencyInfo(data)
{ Start();
}

void WaitDependencyInfo::OnCompleted()
{ DEBUGLOG(("WaitDependencyInfo(%p)::OnCompleted()\n", this));
  EventSem.Set();
}*/


/****************************************************************************
*
*  class JobSet
*
****************************************************************************/

InfoFlags JobSet::RequestInfo(APlayable& target, InfoFlags what)
{ DEBUGLOG(("JobSet::RequestInfo(&%p, %x)\n", &target, what));
  InfoFlags what2 = target.RequestInfo(what, Pri);
  if (what)
    Depends.Add(target, what);
  return what2;
}

volatile const AggregateInfo& JobSet::RequestAggregateInfo(APlayable& target, const PlayableSetBase& excluding, InfoFlags& what)
{ DEBUGLOG(("JobSet::RequestAggregateInfo(&%p, {%u,}, %x)\n", &target, excluding.size(), what));
  volatile const AggregateInfo& ret = target.RequestAggregateInfo(excluding, what, Pri);
  if (what)
    Depends.Add(target, what);
  return ret;
}
