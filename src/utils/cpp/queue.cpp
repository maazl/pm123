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


/*queue_base::ReaderBase::ReaderBase(queue_base& queue)
: Queue(queue)
{ Queue.RequestRead();
}*/

void queue_base::RequestRead()
{ DEBUGLOG(("queue_base(%p)::RequestRead()\n", this));
  for(;;)
  { if (Head) // Double check
    { Mutex::Lock lock(Mtx);
      if (Head)
      { ASSERT(EvRead.IsSet());
        EvRead.Reset();
        DEBUGLOG(("queue_base::RequestRead() - %p\n", Head));
        return;
      }
      EvEmpty.Reset();
    }
    EvEmpty.Wait();
  }
}

void queue_base::CommitRead(EntryBase* qp)
{ DEBUGLOG(("queue_base(%p)::CommitRead(%p) - %p %p\n", this, qp, Head, Tail));
  ASSERT(!EvRead.IsSet());
  ASSERT(qp == Head);
  Mutex::Lock lock(Mtx);
  Head = Head->Next;
  if (Head == NULL)
  { Tail = NULL;
    EvEmpty.Reset();
  }
  EvRead.Set();
}

void queue_base::Write(EntryBase* entry)
{ DEBUGLOG(("queue_base(%p)::Write(%p)\n", this, entry));
  entry->Next = NULL;
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
  Mutex::Lock lock(Mtx);
  if (after == NULL)
  { // insert at head
    entry->Next = Head;
    Head = entry;
    if (Tail == NULL)
    { Tail = entry;
      EvEmpty.Set(); // First Element => Set Event
    }
  } else
  { entry->Next = after->Next;
    after->Next = entry;
    if (Tail == after)
      Tail = entry;
  }
}

