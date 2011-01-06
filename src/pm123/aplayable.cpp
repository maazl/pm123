/*
 * Copyright 2009-2009 M.Mueller
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
#include "aplayable.h"
#include "playable.h"
#include "playableset.h"
#include "pm123.h"
#include <utilfct.h>

#include <debuglog.h>

#ifdef DEBUG_LOG
#define RP_INITIAL_SIZE 5 // Errors come sooner...
#else
#define RP_INITIAL_SIZE 100
#endif


/****************************************************************************
*
*  class InfoState
*
****************************************************************************/

void InfoState::Assign(const InfoState& r, InfoFlags mask)
{ Available = r.Available & mask;
  Valid     = r.Valid     & mask;
  Confirmed = r.Confirmed & mask;
  ReqLow    = r.ReqLow    & mask;
  ReqHigh   = r.ReqHigh   & mask;
  InService = r.InService & mask;
}

InfoFlags InfoState::Check(InfoFlags what, Reliability rel) const
{ switch (rel)
  {case REL_Virgin:
    return IF_None;
   case REL_Invalid:
    return (InfoFlags)(what & ~(Confirmed|Valid|Available));
   case REL_Cached:
    return (InfoFlags)(what & ~(Confirmed|Valid));
   case REL_Confirmed:
    return (InfoFlags)(what & ~Confirmed);
   default:
    return what;
  }
}

InfoFlags InfoState::Invalidate(InfoFlags what)
{ InService &= ~what;
  Available |= what;
  return (InfoFlags)(Valid.maskreset(what) | Confirmed.maskreset(what));
}

InfoFlags InfoState::InvalidateSync(InfoFlags what)
{ what &= (InfoFlags)~InService;
  Available |= what;
  return (InfoFlags)(Valid.maskreset(what) | Confirmed.maskreset(what));
}

void InfoState::Outdate(InfoFlags what)
{ what &= (InfoFlags)+Confirmed;
  Valid |= what;
  Confirmed &= ~what;
}

InfoFlags InfoState::Cache(InfoFlags what)
{ return (InfoFlags)Valid.maskset(what & ~(Confirmed|Available));
}

InfoFlags InfoState::Request(InfoFlags what, Priority pri)
{ ASSERT(pri != PRI_None);
  if (pri == PRI_Low)
    // This is not fully atomic because a high priority request may be placed
    // after the mask is applied to what. But this has the only consequence that
    // an unnecessary request is placed in the worker queue. This request causes a no-op.
    return (InfoFlags)ReqLow.maskset(what & ~ReqHigh);
  else
    return (InfoFlags)ReqHigh.maskset(what);
}

InfoFlags InfoState::EndUpdate(InfoFlags what)
{ InfoFlags ret = (InfoFlags)InService.maskreset(what);
  if (ret)
  { Confirmed |= ret;
    ReqLow    &= ~ret;
    ReqHigh   &= ~ret;
  }
  DEBUGLOG(("InfoState(%p)::EndUpdate(%x) -> %x\n", this, what, ret));
  return ret;
}


/****************************************************************************
*
*  class APlayable
*
****************************************************************************/

priority_queue<APlayable::QEntry> APlayable::WQueue(2);
APlayable::WInit* APlayable::WItems = NULL;
size_t APlayable::WNumWorkers    = 0;
size_t APlayable::WNumDlgWorkers = 0;
bool   APlayable::WTermRq        = false;

/*#ifdef DEBUG_LOG
void worker_queue::DumpQ() const
{ DEBUGLOG(("Playable::worker_queue::DumpQ - %p, %p, %p\n", Head, HPTail, Tail));
  qentry* cur = (qentry*)Head;
  while (cur)
  { DEBUGLOG(("Playable::worker_queue::DumpQ %p{%p}\n", cur, cur->Data.get()));
    cur = (qentry*)cur->Next;
  }
}
#endif*/


APlayable::WaitInfo::WaitInfo(APlayable& inst, InfoFlags filter)
: Filter(filter),
  Deleg(inst.InfoChange, *this, &APlayable::WaitInfo::InfoChangeEvent)
{ DEBUGLOG(("APlayable::WaitInfo(%p)::WaitInfo(&%p, %x)\n", this, &inst, filter));
  CommitInfo(IF_None);
}

void APlayable::WaitInfo::InfoChangeEvent(const PlayableChangeArgs& args)
{ DEBUGLOG(("APlayable::WaitInfo(%p)::InfoChangeEvent({&%p, %x, %x}) - %x\n",
    this, &args.Instance, args.Changed, args.Loaded, Filter));
  ASSERT(!args.IsInitial()); // Owner died! - This must not happen.
  CommitInfo(args.Loaded);
}

void APlayable::WaitInfo::CommitInfo(InfoFlags what)
{ DEBUGLOG(("APlayable::WaitInfo(%p)::CommitInfo(%x) - %x\n", this, what, Filter));
  Filter &= ~what;
  if (!Filter)
  { Deleg.detach();
    EventSem.Set();
  }
}


InfoFlags APlayable::RequestInfo(InfoFlags what, Priority pri, Reliability rel)
{ DEBUGLOG(("APlayable(%p{%s})::RequestInfo(%x, %d, %d)\n", this, GetPlayable().URL.getShortName().cdata(), what, pri, rel));
  InfoFlags rq = what;
  InfoFlags async = DoRequestInfo(rq, pri, rel);
  DEBUGLOG(("APlayable::RequestInfo: %x\n", async));
  ASSERT(async == IF_None || pri != PRI_None);
  // what  - requested information
  // rq    - missing information, subset of what
  // async - asynchronously requested information, subset of rq
  //         because the information may be on the way by another thread or PRI_None was specified.
  
  // Restrict rel to avoid to load an information twice.
  if (rel == REL_Reload)
    rel = REL_Confirmed;

  if (async != IF_None)
  { // Something to do
    if (pri == PRI_Sync)
    { // Synchronous request => use the current thread
      // Case 1: DoLoadInfo returned true, other objects required.
      //      => Schedule request and wait.
      // Case 2: DoLoadInfo returned false, but still some info's missing.
      //      => Wait.
      // Case 3: DoLoadInfo returned false and all information is complete.
      //      => Done.
      if (DoLoadInfo(PRI_Sync))
      { WQueue.Write(new QEntry(this), 0);
      } else
      { DoRequestInfo(rq, PRI_None, rel);
        if (rq == IF_None)
          return IF_None;
      }
      // Synchronous processing failed because of dependencies or concurrency
      // => execute asynchronously
      WaitInfo waitinfo(*this, async);
      // Double check because some information can be valid now.
      DoRequestInfo(rq, PRI_None, rel);
      waitinfo.CommitInfo(rq);
      // Wait for information currently in service (if any).
      waitinfo.Wait();
      return IF_None;
    }  
    // Asynchronous request => Schedule request
    if (InfoStat.RequestAsync(pri))
      WQueue.Write(new QEntry(this), pri == PRI_Low);
  }
  
  return rq;
}

volatile const AggregateInfo& APlayable::RequestAggregateInfo(
  const PlayableSetBase& exclude, InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("APlayable(%p)::RequestAggregateInfo(%x, %d, %d)\n", this, what, pri, rel));
  AggregateInfo& ai = DoAILookup(exclude);
  InfoFlags rq = what;
  InfoFlags async = DoRequestAI(ai, rq, pri, rel);
  // what  - requested information
  // rq    - missing information, subset of what
  // async - asynchronously requested information, subset of rq
  //         because the information may be on the way by another thread or PRI_None was specified.
  
  // Restrict rel to avoid to load an information twice.
  if (rel == REL_Reload)
    rel = REL_Confirmed;

  if (async != IF_None)
  { // Something to do
    if (pri == PRI_Sync)
    { // Synchronous request => use the current thread
      // Case 1: DoLoadInfo returned true, other objects required.
      //      => Schedule request and wait.
      // Case 2: DoLoadInfo returned false, but still some info's missing.
      //      => Wait.
      // Case 3: DoLoadInfo returned false and all information is complete.
      //      => Done.
      if (DoLoadInfo(PRI_Sync))
      { WQueue.Write(new QEntry(this), 0);
      } else
      { DoRequestAI(ai, rq, PRI_None, rel);
        if (rq == IF_None)
          goto done;
      }
      // Synchronous processing failed because of dependencies or concurrency
      // => execute asynchronously
      WaitInfo waitinfo(*this, what);
      // Double check because some information can be valid now.
      DoRequestAI(ai, rq, PRI_None, rel);
      waitinfo.CommitInfo(rq);
      // Wait for information currently in service (if any).
      waitinfo.Wait();
      goto done;
    }  
    // Asynchronous request => Schedule request
    if (InfoStat.RequestAsync(pri))
      WQueue.Write(new QEntry(this), pri == PRI_Low);
  }
  what = rq;
 done:
  return ai;
}

/*void APlayable::RaiseInfoChange(InfoFlags loaded, InfoFlags changed)
{ if (loaded|changed)
    InfoChange(PlayableChangeArgs(*this, loaded, changed, IF_None));
}*/


void APlayable::PlayableWorker(WInit& init)
{ // Do the work! 
  const size_t pri = init.Pri <= PRI_Low;
  for (;;)
  { DEBUGLOG(("PlayableWorker(%u) looking for work\n", init.Pri));
    QEntry* qp = WQueue.Read(pri);
    DEBUGLOG(("PlayableWorker received message %p %p\n", qp, qp->Data.get()));

    if (!qp->Data) // stop
    { // leave the deadly pill for the next thread
      WQueue.WriteFront(qp, 0);
      break;
    }

    init.Current = qp->Data;
    qp->Data->InfoStat.ResetAsync(init.Pri);
    if (!qp->Data->DoLoadInfo(pri ? PRI_Low : PRI_Normal))
      delete qp;
    else
      // reschedule required
      if (qp->Data->InfoStat.RequestAsync(init.Pri))
        WQueue.Write(qp, pri);
    init.Current.reset();
  }
}

void TFNENTRY PlayableWorkerStub(void* arg)
{ APlayable::PlayableWorker(*(APlayable::WInit*)arg);
}

void APlayable::QueueTraverseProxy(const QEntry& entry, size_t priority, void* arg)
{ DEBUGLOG(("APlayable::QueueTraverseProxy(&%p{%p,...}, %u, %p)\n", &entry, entry.Data.get(), priority, arg));
  (*((QueueTraverseProxyData*)arg)->Action)(entry.Data, priority ? PRI_Low : PRI_Normal, false, ((QueueTraverseProxyData*)arg)->Arg);
}

void APlayable::QueueTraverse(void (*action)(APlayable* entry, Priority pri, bool insvc, void* arg), void* arg)
{ QueueTraverseProxyData arg_proxy = { action, arg };
  const WInit* const wpe = WItems + WNumWorkers+WNumDlgWorkers;
  Mutex::Lock lock(WQueue.Mtx);
  // In service worker items first
  for (const WInit* wp = WItems; wp != wpe; ++wp)
  { // Acquire local strong reference.
    int_ptr<APlayable> obj = wp->Current;
    if (obj)
      (*action)(obj, wp->Pri, true, arg);
  }
  WQueue.ForEach(&QueueTraverseProxy, &arg_proxy);
}


void APlayable::Init()
{ DEBUGLOG(("APlayable::Init()\n"));
  // start the worker threads
  ASSERT(WItems == NULL);
  WTermRq = false;
  WNumWorkers = cfg.num_workers;     // sample this atomically
  WNumDlgWorkers = cfg.num_dlg_workers; // sample this atomically
  WItems = new WInit[WNumWorkers+WNumDlgWorkers];
  for (WInit* wp = WItems + WNumWorkers+WNumDlgWorkers; wp-- != WItems; )
  { wp->Pri = wp >= WItems + WNumDlgWorkers ? PRI_Low : PRI_Normal;
    wp->TID = _beginthread(&PlayableWorkerStub, NULL, 65536, wp);
    ASSERT(wp->TID != -1);
  }
}

void APlayable::Uninit()
{ DEBUGLOG(("APlayable::Uninit()\n"));
  WTermRq = true;
  WQueue.Purge();
  WQueue.Write(new QEntry(NULL), false); // deadly pill
  // Synchronize worker threads
  for (WInit* wp = WItems + WNumWorkers; wp != WItems; )
    wait_thread_pm(amp_player_hab(), (--wp)->TID, 60000);
  // Now remove the deadly pill from the queue
  WQueue.Purge();
  delete[] WItems;
  WItems = NULL;
  DEBUGLOG(("APlayable::Uninit - complete\n"));
}

