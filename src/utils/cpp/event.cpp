/*
 * Copyright 2007-2007 M.Mueller
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


#include "event.h"

#include <debuglog.h>


void event_base::operator+=(delegate_base& d)
{ DEBUGLOG(("event_base(%p)::operator+=(%p{%p}) - %p\n", this, &d, d.Rcv, Root));
  ASSERT(d.Ev == NULL);
  CritSect cs();
  d.Ev = this;
  d.Link = Root;
  Root = &d;
}

bool event_base::operator-=(delegate_base& d)
{ DEBUGLOG(("event_base(%p)::operator-=(%p{%p})\n", this, &d, d.Rcv));
  delegate_base** mpp = &Root;
  CritSect cs();
  while (*mpp != NULL)
  { if (*mpp == &d)
    { ASSERT(d.Ev == this);
      d.Ev = NULL;
      *mpp = d.Link;
      DEBUGLOG(("event_base::operator-= OK\n"));
      return true;
    }
    mpp = &(*mpp)->Link;
  }
  DEBUGLOG(("event_base::operator-= FAILED\n"));
  return false;
}

void event_base::operator()(dummy& param) const
{ DEBUGLOG(("event_base(%p)::operator()(%p) - %p\n", this, &param, Root));
  delegate_base* mp = Root;
  // TODO: This no-lock implementation may not 100% safe with respect to operator-=.
  while (mp != NULL)
  { delegate_base* mp2 = mp->Link; // threading issue...
    DEBUGLOG(("event_base::operator() - %p{%p,%p} &%p{%p}\n", mp, mp->Fn, mp->Rcv, &param, param.dummy));
    (*mp->Fn)(mp->Rcv, param); // callback
    mp = mp2;
  }
}

event_base::~event_base()
{ DEBUGLOG(("event_base(%p)::~event_base() - %p\n", this, Root));
  // detach events
  reset();
}

void event_base::reset()
{ DEBUGLOG(("event_base(%p)::reset()\n", this));
  CritSect cs();
  delegate_base* mp = Root;
  Root = NULL;
  while (mp != NULL)
  { mp->Ev = NULL;
    mp = mp->Link;
  }
}


delegate_base::delegate_base(event_base& ev, func_type fn, const void* rcv)
: Fn(fn),
  Rcv(rcv),
  Ev(&ev)
{ DEBUGLOG(("delegate_base(%p)::delegate_base(%p, %p, %p)\n", this, &ev, fn, rcv));
  CritSect cs();
  Link = ev.Root;
  ev.Root = this;
} 

void delegate_base::detach()
{
  #ifdef DEBUG
  DEBUGLOG(("delegate_base(%p)::detach() - %p\n", this, Ev));
  if (Ev)
  { CritSect cs();
    if (Ev)
    { RASSERT((*Ev) -= *this);
      Ev = NULL;
  } }
  #else
  // no critical section required in case of non-debug builds,
  // because operator -= simply will turn into a no-op in doubt.
  if (Ev)
  { (*Ev) -= *this;
    Ev = NULL;
  }
  #endif
} 

void PostMsgDelegateBase::callback(PostMsgDelegateBase* receiver, const void* param)
{ WinPostMsg(receiver->Window, receiver->Msg, MPFROMP(param), MPFROMP(receiver->Param2));
}

