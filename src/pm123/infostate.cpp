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


#include "infostate.h"

#include <debuglog.h>


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
*  class InfoState::Update
*
****************************************************************************/

InfoState InfoState::Update::Reset(InfoState& stat, Priority pri)
{ if (What)
    InfoStat->CancelUpdate(What);
  InfoStat = &stat;
  return What = stat.BeginUpdate(stat.GetRequest(pri));
}
