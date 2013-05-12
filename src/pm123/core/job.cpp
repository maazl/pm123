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


#include "job.h"
#include "playableset.h"

#include <debuglog.h>


/* class Job */

Job Job::SyncJob(PRI_Sync|PRI_Normal);
Job Job::NoBlockJob(PRI_None);

InfoFlags Job::RequestInfo(APlayable& target, InfoFlags what)
{ return target.RequestInfo(what, Pri);
}

volatile const AggregateInfo& Job::RequestAggregateInfo(APlayable& target, const PlayableSetBase& excluding, InfoFlags& what)
{ return target.RequestAggregateInfo(excluding, what, Pri);
}


/* class JobSet */

InfoFlags JobSet::RequestInfo(APlayable& target, InfoFlags what)
{ DEBUGLOG(("JobSet::RequestInfo(&%p, %x)\n", &target, what));
  what = target.RequestInfo(what, Pri);
  if (what)
    Depends.Add(target, what);
  return what;
}

volatile const AggregateInfo& JobSet::RequestAggregateInfo(APlayable& target, const PlayableSetBase& excluding, InfoFlags& what)
{ DEBUGLOG(("JobSet::RequestAggregateInfo(&%p, {%u,}, %x)\n", &target, excluding.size(), what));
  volatile const AggregateInfo& ret = target.RequestAggregateInfo(excluding, what, Pri);
  if (what)
    Depends.Add(target, what, &ret.Exclude);
  return ret;
}

bool JobSet::Commit()
{ if (!Depends.Size())
    return false;
  AllDepends.Join(Depends);
  return true;
}


/* class RecursiveJobSet */

inline Priority RecursiveJobSet::GetRequestPriority(APlayable& target)
{ return SyncOnly && SyncOnly != &target.GetPlayable() ? Pri & ~PRI_TrySync : Pri;
}

InfoFlags RecursiveJobSet::RequestInfo(APlayable& target, InfoFlags what)
{ DEBUGLOG(("RecursiveJobSet::RequestInfo(&%p, %x)\n", &target, what));
  what = target.RequestInfo(what, GetRequestPriority(target));
  if (what)
    Depends.Add(target, what);
  return what;
}

volatile const AggregateInfo& RecursiveJobSet::RequestAggregateInfo(APlayable& target, const PlayableSetBase& excluding, InfoFlags& what)
{ DEBUGLOG(("RecursiveJobSet::RequestAggregateInfo(&%p, {%u,}, %x)\n", &target, excluding.size(), what));
  volatile const AggregateInfo& ret = target.RequestAggregateInfo(excluding, what, GetRequestPriority(target));
  if (what)
    Depends.Add(target, what, &ret.Exclude);
  return ret;
}

