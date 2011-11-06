/*
 * Copyright 2007-2011 M.Mueller
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

#include <cpp/interlockedptr.h>
#include <os2.h>

#include <debuglog.h>


Mutex event_base::Mtx;

event_base::delegate_iterator::delegate_iterator(event_base& root)
: Next(NULL)
, Root(root)
{ DEBUGLOG(("delegate_iterator(%p)::delegate_iterator(&%p)\n", this, &root));
  // register iterator
  register delegate_iterator* old_root = root.Iter;
  do
  { Link = old_root;
    old_root = InterlockedCxc(&root.Iter, old_root, this);
  } while (old_root != Link);
  // Start iteration
  Next = root.Deleg;
}

event_base::delegate_iterator::~delegate_iterator()
{ DEBUGLOG(("delegate_iterator(%p{%p})::~delegate_iterator()\n", this, &Root));
  // cancel iterator registration
  Mutex::Lock lock(Mtx);
  delegate_iterator** ipp = &Root.Iter;
  for (;;)
  { delegate_iterator* ip = *ipp;
    if (!ip)
      break;
    if (ip == this)
    { *ipp = ip->Link;
      return;
    }
    ipp = &ip->Link;
  }
  ASSERT(false);
}

delegate_base* event_base::delegate_iterator::next()
{ delegate_base* mp;
  // next
  Mutex::Lock lock(Mtx);
  do
  { mp = Next;
    if (!mp)
      break; // done
    Next = mp->Link;
  } while (mp->Ev != &Root); // skip if detached
  return mp;
}

void event_base::operator+=(delegate_base& d)
{ DEBUGLOG(("event_base(%p)::operator+=(&%p{%p}) - %p\n", this, &d, d.Rcv, Deleg));
  ASSERT(!d.Ev);
  PASSERT(&d);
  d.Ev = this;
  register delegate_base* old_root = Deleg;
  do
  { d.Link = old_root;
    old_root = InterlockedCxc(&Deleg, old_root, &d);
  } while (old_root != d.Link);
}

bool event_base::operator-=(delegate_base& d)
{ DEBUGLOG(("event_base(%p)::operator-=(&%p{%p})\n", this, &d, d.Rcv));
  PASSERT(&d);
  // 1. logical detach atomically
  event_base* ev = InterlockedXch(&d.Ev, (event_base*)NULL);
  if (ev != this)
    return false;
  ASSERT(ev == this);

  { Mutex::Lock lock(Mtx);
    // check iterators
    delegate_iterator* ip = Iter;
    while (ip)
    { if (ip->Next == &d)
        ip->Next = d.Link;
      ip = ip->Link;
    }
    // remove from list
    delegate_base** mpp = &Deleg;
    for (;;)
    { delegate_base* mp = *mpp;
      if (!mp)
        break;
      if (mp == &d)
      { PASSERT(d.Link);
        *mpp = d.Link;
        mp->Link = NULL;
        goto ok;
      }
      mpp = &mp->Link;
    }
  }
  ASSERT(false);
 ok:
  DEBUGLOG(("event_base::operator-= %p\n", ev));
  return true;
}

void event_base::operator()(dummy& param)
{ DEBUGLOG(("event_base(%p)::operator()(%p) - %p, %p\n", this, &param, Deleg, Iter));
  delegate_iterator iter(*this);
  do
  { delegate_base* mp = iter.next();
    if (!mp)
      break; // done
    PASSERT(mp);
    // invoke observer
    DEBUGLOG(("event_base::operator() - %p{%p,%p,%p,%p} &%p{%p}\n",
      mp, mp->Fn, mp->Rcv, mp->Ev, mp->Link, &param, param.dummy));
    if (mp->Ev == this)
      (*mp->Fn)(mp->Rcv, param);
  } while (true);
}

void event_base::reset()
{ DEBUGLOG(("event_base(%p)::reset() %p, %p\n", this, Deleg, Iter));
  if (!Deleg && !Iter)
    return; // fast path
  Mutex::Lock lock(Mtx);
  // Cancel iterators
  delegate_iterator** ipp = &Iter;
  for (;;)
  { delegate_iterator* ip = *ipp;
    if (!ip)
      break;
    *ipp = NULL;
    ipp = &ip->Link;
  }
  // Cancel registrations
  delegate_base** mpp = &Deleg;
  for (;;)
  { delegate_base* mp = *mpp;
    if (!mp)
      break;
    event_base* ev = InterlockedXch(&mp->Ev, (event_base*)NULL);
    if (ev)
    { ASSERT(ev == this);
      *mpp = mp->Link;
      mp->Link = NULL;
    } else
      mpp = &mp->Link;
  }
}

void event_base::sync()
{ DEBUGLOG(("event_base(%p)::Wait() - %p\n", this, Iter));
  unsigned i = 5; // fast cycles
  do
  { if (!Iter)
      return;
    DosSleep(0);
  } while (--i);
  // slow cycles
  while (!Iter)
  { DEBUGLOG(("event_base::Wait loop %p\n", Iter));
    DosSleep(1);
  }
}


delegate_base::delegate_base(event_base& ev, func_type fn, const void* rcv)
: Fn(fn),
  Rcv(rcv),
  Ev(&ev)
{ DEBUGLOG(("delegate_base(%p)::delegate_base(&%p, %p, %p)\n", this, &ev, fn, rcv));
  register delegate_base* old_root = ev.Deleg;
  do
  { Link = old_root;
    old_root = InterlockedCxc(&ev.Deleg, old_root, this);
  } while (old_root != Link);
} 

delegate_base::~delegate_base()
{ DEBUGLOG(("delegate_base(%p)::~delegate_base() - %p\n", this, Ev));
  detach();
  sync();
}

/*void delegate_base::rebind(func_type fn, const void* rcv)
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
}*/

bool delegate_base::detach()
{ DEBUGLOG(("delegate_base(%p)::detach() - %p\n", this, Ev));
  bool ret = false;
  if (Ev)
  { ret = (*Ev) -= *this;
    ASSERT(!Ev);
  }
  return ret;
}

void delegate_base::sync()
{ DEBUGLOG(("delegate_base(%p)::sync() - %p\n", this, Ev));
  // Hmm, not that nice
  if (Ev)
    Ev->sync();
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

