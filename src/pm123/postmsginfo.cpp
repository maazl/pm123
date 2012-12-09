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


#include "postmsginfo.h"

#include <debuglog.h>


PostMsgWorker::PostMsgWorker()
: Filter(IF_None)
, Deleg(*this, &PostMsgWorker::InfoChangeHandler)
{}

void PostMsgWorker::Start(APlayable& ap, InfoFlags what, Priority pri,
                          HWND target, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG(("PostMsgWorker(%p)::Start(&%p, %x, %u, %p, %u, %p,%p)\n", this,
    &ap, what, pri, target, msg, mp1, mp2));
  Deleg.detach();
  Message.assign(target, msg, mp1, mp2);
  Filter = what;
  ap.GetInfoChange() += Deleg;
  if (!(Filter &= ap.RequestInfo(what, pri)) && Deleg.detach())
    OnCompleted();
}

void PostMsgWorker::InfoChangeHandler(const PlayableChangeArgs& args)
{ if (!(Filter &= ~args.Loaded) && Deleg.detach())
    OnCompleted();
}

void PostMsgWorker::OnCompleted()
{ Message.Post();
}


void AutoPostMsgWorker::Start(APlayable& ap, InfoFlags what, Priority pri,
  HWND target, ULONG msg, MPARAM mp1, MPARAM mp2)
{ if (!ap.RequestInfo(what, pri))
    WinPostMsg(target, msg, mp1, mp2);
  else
    (new AutoPostMsgWorker())->PostMsgWorker::Start(ap, what, pri, target, msg, mp1, mp2);
}

void AutoPostMsgWorker::OnCompleted()
{ PostMsgWorker::OnCompleted();
  delete this;
}


void PostMsgDIWorker::Start(DependencyInfoSet& data, HWND target, ULONG msg, MPARAM mp1, MPARAM mp2)
{ Cancel();
  Data.Swap(data);
  Message.assign(target, msg, mp1, mp2);
  DependencyInfoWorker::Start();
}

void PostMsgDIWorker::OnCompleted()
{ Message.Post();
}

