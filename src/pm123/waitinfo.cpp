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

#include <debuglog.h>


/****************************************************************************
*
*  class WaitLoadInfo
*
****************************************************************************/

WaitLoadInfo::WaitLoadInfo()
: Filter(IF_None),
  Deleg(*this, &WaitLoadInfo::InfoChangeEvent)
{ DEBUGLOG(("WaitLoadInfo(%p)::WaitLoadInfo()\n", this));
}
WaitLoadInfo::WaitLoadInfo(APlayable& inst, InfoFlags filter)
: Filter(filter),
  Deleg(*this, &WaitLoadInfo::InfoChangeEvent)
{ DEBUGLOG(("WaitLoadInfo(%p)::WaitLoadInfo(&%p, %x)\n", this, &inst, filter));
  if (Filter)
    inst.GetInfoChange() += Deleg;
  else
    EventSem.Set();
}

void WaitLoadInfo::Start(APlayable& inst, InfoFlags filter)
{ DEBUGLOG(("WaitLoadInfo(%p)::Start(&%p, %x)\n", this, &inst, filter));
  Deleg.detach();
  Filter = filter;
  if (Filter)
  { EventSem.Reset();
    inst.GetInfoChange() += Deleg;
  } else
    EventSem.Set();
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
*  class WaitInfo
*
****************************************************************************/

bool WaitInfo::Wait(APlayable& inst, InfoFlags what, Reliability rel, long ms)
{ Inst = &inst;
  Rel = rel;
  Start(inst, what);
  CommitInfo(what);
  return WaitLoadInfo::Wait(ms);
}

void WaitInfo::CommitInfo(InfoFlags what)
{ InfoFlags rq = what;
  Inst->DoRequestInfo(rq, PRI_None, Rel);
  WaitLoadInfo::CommitInfo(what & ~rq);
}


/****************************************************************************
*
*  class WaitAggregateInfo
*
****************************************************************************/

bool WaitAggregateInfo::Wait(APlayable& inst, AggregateInfo& ai, InfoFlags what, Reliability rel, long ms)
{ Inst = &inst;
  AI = &ai;
  Rel = rel;
  Start(inst, what);
  CommitInfo(what);
  return WaitLoadInfo::Wait(ms);
}

void WaitAggregateInfo::CommitInfo(InfoFlags what)
{ InfoFlags rq = what;
  Inst->DoRequestAI(*AI, rq, PRI_None, Rel);
  WaitLoadInfo::CommitInfo(what & ~rq);
}
