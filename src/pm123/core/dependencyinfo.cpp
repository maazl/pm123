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


#include "dependencyinfo.h"
#include "playableset.h"
#include "playable.h"

#include <debuglog.h>


/****************************************************************************
*
*  class DependencyInfoPath
*
****************************************************************************/

void DependencyInfoPath::Entry::Merge(InfoFlags what, const PlayableSetBase* exclude)
{ What |= what;
  if (!Exclude) // Hmm, dependencies on the same object with different exclusion lists???
    Exclude = exclude;
}

InfoFlags DependencyInfoPath::Entry::Check()
{ InfoFlags what2 = What & IF_Aggreg;
  if (what2 && Exclude)
  { (void)Inst->RequestAggregateInfo(*Exclude, what2, PRI_None);
    What &= what2 | ~IF_Aggreg;
    if ((What & ~IF_Aggreg) == 0)
      goto done;
  }
  What = Inst->RequestInfo(What, PRI_None);
 done:
  return What;
}

int DependencyInfoPath::Entry::compare(const APlayable& l, const Entry& r)
{ // Arbitrary but reliable sequence.
  return (int)&l - (int)r.Inst.get();
}

void DependencyInfoPath::Add(APlayable& inst, InfoFlags what, const PlayableSetBase* exclude)
{ DEBUGLOG(("DependencyInfoPath::Add(&%p, %x, %p)\n", &inst, what, exclude));
  ASSERT(!exclude || (what & IF_Aggreg));
  Entry*& ep = MandatorySet.get(inst);
  if (ep)
    // Hit => merge
    ep->Merge(what, exclude);
  else
    // No hit => insert
    ep = new Entry(inst, what, exclude);
}

#ifdef DEBUG_LOG
void DependencyInfoPath::DumpSet(xstringbuilder& dest, const SetType& set)
{ foreach (Entry,*const*, epp, set)
  { if (epp != set.begin())
      dest.append(", ");
    const Entry& entry = **epp;
    dest.appendf("%p{%s} : %x", entry.Inst.get(), entry.Inst->DebugName().cdata(), entry.What);
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

/*struct JoinIterator
{ DependencyInfoPath::Entry* L;
  DependencyInfoPath::Entry* R;
  enum Mode
  { Done,
    LeftOuter,
    RightOuter,
    Inner
  } Compare();
};

JoinIterator::Mode JoinIterator::Compare()
{ if (L == NULL)
   return R == NULL ? Done : RightOuter;
  if (R == NULL || L->Inst.get() < R->Inst.get())
   return LeftOuter;
  if (L->Inst.get() > R->Inst.get())
    return RightOuter;
  return Inner;
}*/

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
  { if (re == NULL)
      goto lo1;
    if (le == NULL || le->Inst.get() > re->Inst.get())
    { // current right entry does not match
      outer |= 1;
      Entry*& lo = OptionalSet.get(*re->Inst);
      if (lo)
      { lo->Merge(re->What, re->Exclude);
        ++ri;
      } else
      { lo = re;
        AccessMandatorySet(r).erase(ri);
      }
      re = ri < r.Size() ? AccessMandatorySet(r)[ri] : NULL;
    } else if (le->Inst.get() < re->Inst.get())
    { // current left entry does not match
     lo1:
      outer |= 2;
      Entry*& ro = roset.get(*le->Inst);
      if (ro)
      { ro->Merge(le->What, le->Exclude);
        ++li;
      } else
      { ro = le;
        MandatorySet.erase(li);
      }
      le = li < MandatorySet.size() ? MandatorySet[li] : NULL;
    } else
    { //  match
      le->Merge(re->What, re->Exclude);
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
    { if (re == NULL)
        goto lo2;
      if (le == NULL || le->Inst.get() > re->Inst.get())
      { // current right entry does not match
      } else if (le->Inst.get() < re->Inst.get())
      { // current left entry does not match
       lo2:
        OptionalSet.erase(li);
        le = li < OptionalSet.size() ? OptionalSet[li] : NULL;
        continue;
      } else
      { // match
        le->Merge(re->What, re->Exclude);
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
    { if (re == NULL)
        goto lo3;
      if (le == NULL || le->Inst.get() > re->Inst.get())
      { // current right entry does not match
        roset.erase(ri);
        OptionalSet.insert(li) = re;
        ++li;
        re = ri < roset.size() ? roset[ri] : NULL;
        continue;
      } else if (le->Inst.get() < re->Inst.get())
      { // current left entry does not match
       lo3:;
      } else
      { // match
        le->Merge(re->What, re->Exclude);
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
#ifdef DEBUG
, NowWaitingOn(NULL)
#endif
{ DEBUGLOG(("DependencyInfoWorker(%p)::DependencyInfoWorker()\n", this));
}

void DependencyInfoWorker::Start()
{ DEBUGLOG(("DependencyInfoWorker(%p)::Start() - %u\n", this, Data.Size()));
  for (;;)
  { sco_ptr<DependencyInfoSet::Entry> ep; // Defined before lock => released after lock
    Mutex::Lock lock(Mtx);
    if (Data.MandatorySet.size() == 0) // Completed?
    { // Mandatory dependencies completed, check optional dependencies now.
      #ifdef DEBUG
      NowWaitingOn = NULL;
      #endif
      if (Data.OptionalSet.size() == 0)
        break;
      DelegList.reserve(Data.OptionalSet.size());
      foreach (DependencyInfoSet::Entry,*const*, epp, Data.OptionalSet)
      { DependencyInfoSet::Entry& entry = **epp;
        DEBUGLOG(("DependencyInfoWorker::Start checking optional entry %p{%s} : %x\n",
          entry.Inst.get(), entry.Inst->DebugName().cdata(), entry.What));
        DelegList.append() = new DelegType(entry.Inst->GetInfoChange(), *this, &DependencyInfoWorker::OptionalInfoChangeEvent);
        if (entry.Check() == IF_None)
        { DelegList.clear();
          goto completed;
        }
      }
      // Now wait for optional entry.
      goto wait;
    }
    ep = Data.MandatorySet.erase(Data.MandatorySet.size()-1); // Start from back to avoid unnecessary moving.
    DEBUGLOG(("DependencyInfoWorker::Start checking mandatory dependency %p{%s} : %x\n",
      ep->Inst.get(), ep->Inst->DebugName().cdata(), ep->What));
    if (ep->Check())
    { DEBUGLOG(("DependencyInfoWorker::Start still waiting for %x\n", ep->What));
      NowWaitingFor = ep->What;
      #ifdef DEBUG
      NowWaitingOn = ep->Inst;
      #endif
      ep->Inst->GetInfoChange() += Deleg;
      // double check
      if ( ep->Check() != IF_None // no double check hit
        || !Deleg.detach() )      // or /we/ did not deregister ourself
        goto wait;                // => wait for other thread
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
{ DEBUGLOG(("DependencyInfoWorker(%p)::MandatoryInfoChangeEvent({&%p, %x}) - %x\n",
    this, &args.Instance, args.Loaded, NowWaitingFor));
  ASSERT(NowWaitingOn == &args.Instance);
  NowWaitingFor &= ~args.Loaded;
  if ( NowWaitingFor == IF_None // Have we finished?
    && Deleg.detach() ) // and did /we/ deregister ourself?
    // => look for further infos.
    Start();
  // else we have either not finished or someone else deregistered faster.
}

void DependencyInfoWorker::OptionalInfoChangeEvent(const PlayableChangeArgs& args)
{ DEBUGLOG(("DependencyInfoWorker(%p)::OptionalInfoChangeEvent({&%p, %x})\n", this, &args.Instance, args.Loaded));
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
