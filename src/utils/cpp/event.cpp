/*
 * Copyright 2007-2008 M.Mueller
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


#define INCL_BASE
#include "event.h"
#include "cpputil.h"
#include <os2.h>

#include <debuglog.h>


void event_base::operator+=(delegate_base& d)
{ DEBUGLOG(("event_base(%p)::operator+=(&%p{%p}) - %p, %u\n", this, &d, d.Rcv, Root, Count.Peek()));
  ASSERT(d.Ev == NULL);
  ASSERT((unsigned)&d >= 0x10000); // OS/2 trick to validate pointer roughly.
  CritSect cs;
  d.Ev = this;
  d.Link = Root;
  Root = &d;
}

bool event_base::operator-=(delegate_base& d)
{ DEBUGLOG(("event_base(%p)::operator-=(&%p{%p}) - %u\n", this, &d, d.Rcv, Count.Peek()));
  event_base* ev = NULL; // removed event, must not be dereferenced
  delegate_base** mpp = &Root;
  { CritSect cs;
    while (*mpp != NULL)
    { if (*mpp == &d)
      { ev = d.Ev; // do not assert in crtitical section (see below)
        d.Ev = NULL;
        *mpp = d.Link;
        break;
      }
      mpp = &(*mpp)->Link;
    }
  }
  DEBUGLOG(("event_base::operator-= %p\n", ev));
  ASSERT(ev == NULL || ev == this);
  return ev != NULL;
}

void event_base::operator()(dummy& param)
{ DEBUGLOG(("event_base(%p)::operator()(%p) - %p\n", this, &param, Root));
  SpinLock::Use useev(Count);
  // Fetch and lock delegate
  delegate_base* mp;
  { CritSect cs;
    mp = Root;
    if (mp == NULL)
      return;
    mp->Count.Inc();
  }
  for(;;)
  { DEBUGLOG(("event_base::operator() - %p{%p,%p,%p,%p,%u} &%p{%p}\n",
      mp, mp->Fn, mp->Rcv, mp->Ev, mp->Link, mp->Count.Peek(), &param, param.dummy));
    (*mp->Fn)(mp->Rcv, param); // callback
    // unlock delegate and fetch and lock next
    do
    { CritSect cs;
      mp->Count.Dec();
      mp = mp->Link;
      if (mp == NULL)
        return; // no more
      mp->Count.Inc();
      // Skip detached delegates (unlikely, but may happen because of threading)
    } while (mp->Ev == NULL);
  }
}

void event_base::reset()
{ DEBUGLOG(("event_base(%p)::reset() %p\n", this, Root));
  { CritSect cs;
    delegate_base* mp = Root;
    Root = NULL;
    while (mp != NULL)
    { mp->Ev = NULL;
      delegate_base* mp2 = mp->Link;
      mp->Link = NULL; // Abort any currently working operator() after this item.
      mp = mp2;
    }
  }
  sync();
}


delegate_base::delegate_base(event_base& ev, func_type fn, const void* rcv)
: Fn(fn),
  Rcv(rcv),
  Ev(&ev)
{ DEBUGLOG(("delegate_base(%p)::delegate_base(&%p, %p, %p)\n", this, &ev, fn, rcv));
  CritSect cs;
  Link = ev.Root;
  ev.Root = this;
} 

void delegate_base::rebind(func_type fn, const void* rcv)
{ DEBUGLOG(("delegate_base(%p)::rebind(%p, %p)\n", this, fn, rcv));
  sync();
  CritSect cs;
  Fn  = fn;
  Rcv = rcv;
}

void delegate_base::swap_rcv(delegate_base& r)
{ DEBUGLOG(("delegate_base(%p)::swap_rcv(&%p)\n", this, &r));
  sync();
  CritSect cs;
  swap(Fn,  r.Fn);
  swap(Rcv, r.Rcv);
}

bool delegate_base::detach()
{ DEBUGLOG(("delegate_base(%p)::detach() - %p\n", this, Ev));
  bool ret = false;
  if (Ev)
  { ret = (*Ev) -= *this;
    ASSERT(Ev == NULL);
  }
  sync();
  return ret;
} 

/*void PostMsgDelegateBase::callback(PostMsgDelegateBase* receiver, const void* param)
{ WinPostMsg(receiver->Window, receiver->Msg, MPFROMP(param), receiver->MP2);
}

void PostMsgDelegateBase::swap_rcv(PostMsgDelegateBase& r)
{ DEBUGLOG(("PostMsgDelegateBase(%p)::swap_rcv(&%p)\n", this, r));
  sync();
  CritSect cs;
  swap(Window, r.Window);
  swap(Msg, r.Msg);
  swap(MP2, r.MP2);
}*/

