/*
 * Copyright 2009-2011 M.Mueller
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


#include "aplayable.h"
#include "playable.h"
#include "playableset.h"
#include "waitinfo.h"
#include "dependencyinfo.h"
#include "pm123.h"
#include "configuration.h"
#include <utilfct.h>

#include <debuglog.h>

#ifdef DEBUG_LOG
#define RP_INITIAL_SIZE 5 // Errors come sooner...
#else
#define RP_INITIAL_SIZE 100
#endif


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


InfoFlags APlayable::RequestInfo(InfoFlags what, Priority pri, Reliability rel)
{ DEBUGLOG(("APlayable(%p{%s})::RequestInfo(%x, %d, %d)\n", this, GetPlayable().URL.getShortName().cdata(), what, pri, rel));
  InfoFlags rq = what;
  InfoFlags async = DoRequestInfo(rq, pri, rel);
  DEBUGLOG(("APlayable::RequestInfo rq = %x, async = %x\n", rq, async));
  ASSERT(async == IF_None || pri != PRI_None);
  /*#ifdef DEBUG
  if (pri != PRI_None && rq)
  { RequestState req;
    PeekRequest(req);
    if (pri == PRI_Low)
      req.ReqHigh |= req.ReqLow;
    DoRequestInfo(rq, PRI_None, rel);
    ASSERT((~req.ReqHigh & rq) == 0);
  }
  #endif*/
  // what  - requested information
  // rq    - missing information, might be more than what
  // async - asynchronously requested information, subset of rq
  //         because the information may be on the way by another thread.
  
  if (pri != PRI_Sync)
  { if (async != IF_None)
      ScheduleRequest(pri);
    return rq & what;
  }
  // PRI_Sync
  if (rq == IF_None)
    return IF_None;
  // Execute immediately
  HandleRequest(PRI_Sync);
  DoRequestInfo(rq, PRI_None, rel);
  if (rq == IF_None)
    return IF_None;
  // Synchronous processing failed because of dependencies or concurrency
  // => execute asynchronously
  // Restrict rel to avoid to load an information twice.
  if (rel == REL_Reload)
    rel = REL_Confirmed;
  // Wait for information currently in service (if any).
  WaitInfo().Wait(*this, rq, rel);

  return IF_None;
}

volatile const AggregateInfo& APlayable::RequestAggregateInfo(
  const PlayableSetBase& exclude, InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("APlayable(%p)::RequestAggregateInfo(%x, %d, %d)\n", this, what, pri, rel));
  AggregateInfo& ai = DoAILookup(exclude);
  InfoFlags rq = what;
  InfoFlags async = DoRequestAI(ai, rq, pri, rel);
  DEBUGLOG(("APlayable::RequestAggregateInfo ai = &%p{%s,}, rq = %x, async = %x\n",
    &ai, ai.Exclude.DebugDump().cdata(), rq, async));
  ASSERT(async == IF_None || pri != PRI_None);
  /*#ifdef DEBUG
  if (pri != PRI_None && rq)
  { RequestState req;
    PeekRequest(req);
    if (pri == PRI_Low)
      req.ReqHigh |= req.ReqLow;
    DoRequestAI(ai, rq, PRI_None, rel);
    ASSERT((~req.ReqHigh & rq) == 0);
  }
  #endif*/
  // what  - requested information
  // rq    - missing information, subset of what
  // async - asynchronously requested information, subset of rq
  //         because the information may be on the way by another thread or PRI_None was specified.
  
  // Restrict rel to avoid to load an information twice.
  if (rel == REL_Reload)
    rel = REL_Confirmed;

  if (async != IF_None)
  { // Something to do
    if (pri != PRI_Sync)
    { // Asynchronous request => Schedule request
      ScheduleRequest(pri);
      goto done;
    }
    // Synchronous request => use the current thread
    HandleRequest(PRI_Sync);

  } else if (pri != PRI_Sync)
    goto done;
  // PRI_Sync
  if (rq != IF_None)
  { // Synchronous processing failed because of dependencies or concurrency
    // => execute asynchronously
    // Wait for information currently in service (if any).
    WaitAggregateInfo().Wait(*this, ai, rq, rel);
    rq = IF_None;
  }

 done:
  what = rq;
  return ai;
}

/*void APlayable::RaiseInfoChange(InfoFlags loaded, InfoFlags changed)
{ if (loaded|changed)
    InfoChange(PlayableChangeArgs(*this, loaded, changed, IF_None));
}*/

/**
 * Worker classes that keeps track of requested informations that are not
 * ready to be evaluated because of missing dependencies.
 * Instances of this class have built-in life time management.
 * Once the dependencies are fulfilled, it schedules the dependent item
 * in the worker queue of APlayable and destroys itself.
 * Simply create new instances with new and discard the returned pointer.
 */
class RescheduleWorker : DependencyInfoWorker
{public:
  const int_ptr<APlayable>  Inst;
  const Priority            Pri;
 public:
  RescheduleWorker(DependencyInfoSet& data, APlayable& inst, Priority pri);
 private:
  ~RescheduleWorker();
  virtual void              OnCompleted();

 private:
  static sorted_vector<RescheduleWorker,RescheduleWorker,CompareInstance<RescheduleWorker> > Instances;
 public:
  static  void              QueueTraverse(void (*action)(APlayable& inst, Priority pri, const DependencyInfoSet& depends, void* arg), void* arg);
};

sorted_vector<RescheduleWorker,RescheduleWorker,CompareInstance<RescheduleWorker> > RescheduleWorker::Instances;

RescheduleWorker::RescheduleWorker(DependencyInfoSet& data, APlayable& inst, Priority pri)
: DependencyInfoWorker()
, Inst(&inst)
, Pri(pri)
{ DEBUGLOG(("RescheduleWorker(%p)::RescheduleWorker(&%p{%s}, &%p, %u)\n", this,
    &data, data.DebugDump().cdata(), &inst, pri));
  Data.Swap(data);
  { Mutex::Lock lock(Mtx);
    Instances.get(*this) = this;
  }
  Start();
}

RescheduleWorker::~RescheduleWorker()
{ Mutex::Lock lock(Mtx);
  Instances.erase(*this);
}

void RescheduleWorker::OnCompleted()
{ DEBUGLOG(("RescheduleWorker(%p)::OnCompleted()\n", this));
  Inst->ScheduleRequest(Pri);
  // Hack: destroy ourself. It is guaranteed that DependencyInfoWorker does exactly nothing after this callback.
  delete this;
}

void RescheduleWorker::QueueTraverse(void (*action)(APlayable& inst, Priority pri, const DependencyInfoSet& depends, void* arg), void* arg)
{ Mutex::Lock lock(Mtx);
  foreach (RescheduleWorker**, rpp, Instances)
    action(*(*rpp)->Inst, (*rpp)->Pri, (*rpp)->Data, arg);
}


void APlayable::ScheduleRequest(Priority pri)
{ DEBUGLOG(("APlayable(%p)::ScheduleRequest(%u)\n", this, pri));
  if (!AsyncRequest.bitset(pri == PRI_Low))
  { WQueue.Write(new QEntry(this), pri == PRI_Low);
    DEBUGLOG(("APlayable::ScheduleRequest: placed async request\n"));
  }
}

void APlayable::HandleRequest(Priority pri)
{ DEBUGLOG(("APlayable(%p{%s})::HandleRequest(%u)\n", this, GetPlayable().URL.getShortName().cdata(), pri));
  JobSet job(pri > PRI_Normal ? PRI_Normal : pri); // Avoid recursive propagation of PRI_Sync that can cause deadlocks.
  if (pri == PRI_Low)
    AsyncRequest = 0;
  else
    AsyncRequest.bitrst(0);
  DoLoadInfo(job);
  // reschedule required later?
  if (job.AllDepends.Size())
    // The worker deletes itself!
    new RescheduleWorker(job.AllDepends, *this, pri);
}

void APlayable::PlayableWorker(WInit& init)
{ const size_t pri = init.Pri <= PRI_Low;
  // Do the work!
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
    qp->Data->HandleRequest(init.Pri);
    delete qp;
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

void APlayable::WaitQueueTraverse(void (*action)(APlayable& inst, Priority pri, const DependencyInfoSet& depends, void* arg), void* arg)
{ RescheduleWorker::QueueTraverse(action, arg);
}


void APlayable::Init()
{ DEBUGLOG(("APlayable::Init()\n"));
  // start the worker threads
  ASSERT(WItems == NULL);
  WTermRq = false;
  WNumWorkers = Cfg::Get().num_workers;     // sample this atomically
  WNumDlgWorkers = Cfg::Get().num_dlg_workers; // sample this atomically
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
    wait_thread_pm(amp_player_hab, (--wp)->TID, 60000);
  // Now remove the deadly pill from the queue
  WQueue.Purge();
  delete[] WItems;
  WItems = NULL;
  DEBUGLOG(("APlayable::Uninit - complete\n"));
}

