/*
 * Copyright 2007-2009 M.Mueller
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
#include <snprintf.h>


priority_queue_base::priority_queue_base(size_t priorities)
: Head(NULL),
  PriEntries(new PriEntry[priorities]),
  Priorities(priorities) 
{ ASSERT(priorities);
}

priority_queue_base::~priority_queue_base()
{ delete[] PriEntries;
}

#ifdef DEBUG_LOG
void priority_queue_base::Dump() const
{ char buf[1024];
  char* cp = buf;
  cp += sprintf(cp, "%p", Head);
  for (size_t i = 0; i < Priorities; ++i)
    cp += sprintf(cp, ", %p(%u)", PriEntries[i].Tail, PriEntries[i].EvEmpty.IsSet());
  DEBUGLOG(("priority_queue_base::Dump Head, Tail: %s\n", buf));
  cp = buf;
  cp += sprintf(cp, "%p", Head);
  const qentry* qp = Head;
  while (qp)
  { qp = qp->Next;
    cp += snprintf(cp, buf + sizeof(buf) - cp, ", %p", qp);
  }
  DEBUGLOG(("priority_queue_base::Dump Content: %s\n", buf));
}
#endif

#ifdef DEBUG
void priority_queue_base::Fail(const char* file, int line, const char* msg) const
{ Dump();
  dassert(file, line, msg);
}

#define qASSERT(expr) ((expr) ? (void)0 : Fail(__FILE__, __LINE__, #expr))

void priority_queue_base::Check() const
{ if (Head == NULL)
  { // empty queue
    for (size_t i = 0; i < Priorities; ++i)
      qASSERT(PriEntries[i].Tail == NULL);
  } else
  { size_t pri = 0;
    // Find first non NULL tail pointer.
    while (PriEntries[pri].Tail == NULL)
    { ++pri;
      qASSERT(pri < Priorities);
    }
    // Check queue items
    const qentry* qp = Head;
    do
    { qASSERT(pri < Priorities);
      while (PriEntries[pri].Tail == qp)
      { ++pri;
        if (pri >= Priorities)
          break;
      }
      qp = qp->Next;
    } while (qp);
    qASSERT(pri == Priorities);
  }
}
#undef qASSERT
#endif

qentry* priority_queue_base::Read(size_t* priority)
{ DEBUGLOG(("priority_queue_base(%p)::Read(*%u)\n", this, *priority));
  ASSERT(*priority < Priorities);
  PriEntry* pe = PriEntries + *priority;
  for(;;)
  { pe->EvEmpty.Wait();
    Mutex::Lock lock(Mtx);
    //Dump();
    if (pe->Tail) // The event is not reliable.
    { qentry* entry = Head;
      // Detect real priority level
      pe = PriEntries;
      while (pe->Tail == NULL)
        ++pe;
      *priority = pe - PriEntries;
      // Update Head
      qentry* next = entry->Next;
      Head = next;
      // Update Tail pointers
      if (pe->Tail == entry)
      { PriEntry* const pee = PriEntries + Priorities;
        do
        { pe->Tail = next;
        } while (++pe != pee && pe->Tail == entry);
      }
      DEBUGLOG(("priority_queue_base::Read(*%u): %p\n", *priority, entry));
      Check();
      return entry;
    }
    pe->EvEmpty.Reset();
  }
}

void priority_queue_base::Write(qentry* entry, size_t priority)
{ DEBUGLOG(("priority_queue_base(%p)::Write(%p, %u)\n", this, entry, priority));
  ASSERT(entry);
  ASSERT(priority < Priorities);
  PriEntry* pe = PriEntries + priority;
  Mutex::Lock lock(Mtx);
  Dump();
  if (pe->Tail)
  { entry->Next = pe->Tail->Next;
    pe->Tail->Next = entry;
  } else
  { entry->Next = Head;
    Head = entry;
  }
  qentry* tp = pe->Tail;
  do
  { pe->Tail = entry;
    pe->EvEmpty.Set();
  } while (++pe != PriEntries + Priorities && pe->Tail == tp);
  Check();
}

void priority_queue_base::WriteFront(qentry* entry, size_t priority)
{ DEBUGLOG(("priority_queue_base(%p)::RollbackRead(%p, %u)\n", this, entry, priority));
  ASSERT(entry);
  ASSERT(priority < Priorities);
  PriEntry* pe = PriEntries + priority;
  qentry* tp;
  Mutex::Lock lock(Mtx);
  Dump();
  if (priority == 0 || pe[-1].Tail == NULL)
  { tp = NULL;
    entry->Next = Head;
    Head = entry;
  } else
  { tp = pe[-1].Tail;
    entry->Next = tp->Next;
    tp->Next = entry;
  }
  do
  { if (pe->Tail != tp)
      break;
    pe->Tail = entry;
    pe->EvEmpty.Set();
  } while (++pe != PriEntries + Priorities);
  Check();
}

qentry* priority_queue_base::Purge()
{ DEBUGLOG(("priority_queue_base(%p)::Purge() - %p\n", this, Head));
  PriEntry* pe = PriEntries;
  Mutex::Lock lock(Mtx);
  do
    pe->Tail = NULL;
  while (++pe != PriEntries + Priorities);
  qentry* rhead = Head;
  Head = NULL;
  return rhead;
}

bool priority_queue_base::ForEach(bool (*action)(const qentry* entry, size_t priority, void* arg), void* arg)
{ DEBUGLOG(("queue_base(%p)::ForEach(%p, %p) - %p\n", this, action, arg, Head));
  const PriEntry* pep = PriEntries;
  size_t pri = 0;
  Mutex::Lock lock(Mtx);
  const qentry* ep = Head;
  if (ep == NULL)
    return true;
  // Starting priority
  while (pep->Tail == NULL)
    ++pri, ++pep;
  do
  { if (!action(ep, pri, arg))
      return false;
    while (ep == pep->Tail)
      ++pri, ++pep;
    ep = ep->Next;
  } while (ep);
  return true;
}

