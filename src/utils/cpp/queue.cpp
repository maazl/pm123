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


#include "queue.h"
#include <debuglog.h>


queue_base::EntryBase* queue_base::RequestRead()
{ DEBUGLOG(("queue_base(%p)::RequestRead()\n", this));
  for(;;)
  { EvEmpty.Wait();
    Mutex::Lock lock(Mtx);
    EntryBase* qp = Head;
    while (qp)
    { if (!qp->ReadActive)
      { qp->ReadActive = true;
        DEBUGLOG(("queue_base::RequestRead() - %p\n", qp));
        return qp;
      }
      // Skip pending
      qp = qp->Next;
    }
    EvEmpty.Reset();
  }
}

void queue_base::CommitRead(EntryBase* qp)
{ DEBUGLOG(("queue_base(%p)::CommitRead(%p) - %p %p\n", this, qp, Head, Tail));
  ASSERT(qp);
  ASSERT(qp->ReadActive);
  Mutex::Lock lock(Mtx);
  EntryBase* bp = NULL;
  if (qp != Head)
  { // Well, we have to use linear search here.
    // But it is unlikely that there are many uncommited items.
    bp = Head;
    for(;;)
    { ASSERT(bp);
      if (bp->Next == qp)
        break;
      bp = bp->Next;
    }
    // unlink
    bp->Next = qp->Next;
    if (qp->Next == NULL)
      // at the end
      Tail = bp;
  } else
  { Head = qp->Next;
    if (Head == NULL)
    { Tail = NULL;
      EvEmpty.Reset();
  } }

  qp->ReadActive = false;
  EvRead.Set();
  EvRead.Reset(); // Hmm, does this reliable unblock all threads once?
}

void queue_base::RollbackRead(EntryBase* qp)
{ DEBUGLOG(("queue_base(%p)::RollbackRead(%p)\n", this, qp));
  ASSERT(qp);
  ASSERT(qp->ReadActive);
  // implicitely atomic
  qp->ReadActive = false;
  EvRead.Set();
  EvRead.Reset();
}

void queue_base::Write(EntryBase* entry)
{ DEBUGLOG(("queue_base(%p)::Write(%p)\n", this, entry));
  entry->Next = NULL;
  entry->ReadActive = false;
  Mutex::Lock lock(Mtx);
  if (Tail)
    Tail->Next = entry;
   else
  { Head = entry;
    EvEmpty.Set(); // First Element => Set Event
  }
  Tail = entry;
}

void queue_base::Write(EntryBase* entry, EntryBase* after)
{ DEBUGLOG(("queue_base(%p)::Write(%p, %p)\n", this, entry, after));
  entry->ReadActive = false;
  Mutex::Lock lock(Mtx);
  // insert
  EntryBase*& qp = after ? after->Next : Head;
  entry->Next = qp;
  qp = entry;
  // update Tail
  if (Tail == after)
    Tail = entry;
  // signal readers
  EvEmpty.Set();
}

