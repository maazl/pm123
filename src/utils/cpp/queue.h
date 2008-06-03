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


#ifndef CPP_QUEUE_H
#define CPP_QUEUE_H

#include <config.h>
#include <cpp/mutex.h>
#include <cpp/smartptr.h>

#include <debuglog.h>


class queue_base
{public:
  struct EntryBase
  { EntryBase* Next;
  };
  /*class ReaderBase
  {public:
    queue_base& Queue;
   private: // non-copyable
    ReaderBase(const ReaderBase&);
    void operator=(const ReaderBase&);
   protected:
    ReaderBase(queue_base& queue);
  };*/

 protected:
  EntryBase* Head;
  EntryBase* Tail;
  Event      EvEmpty;
  Event      EvRead;
 public:
  Mutex      Mtx;

 protected: // class should not be used directly!
             queue_base()         : Head(NULL), Tail(NULL) { EvRead.Set(); }
  EntryBase* Read() const         { return Head; }
  void       CommitRead(EntryBase* e);
 public:
  void       RequestRead();
 protected:
  void       Write(EntryBase* entry);
  void       Write(EntryBase* entry, EntryBase* after);
};

template <class T>
class queue : public queue_base
{public:
  struct qentry : public queue_base::EntryBase
  { T        Data;
    qentry(const T& data) : Data(data) {}
  };
  /*class Reader;
  friend class Reader;
  class Reader : private ReaderBase
  {public:
    Reader(queue<T>& queue)       : ReaderBase(queue) {}
    ~Reader()                     { ((queue<T>&)Queue).CommitRead(); DEBUGLOG(("queue<T>::Reader::~Reader()\n")); }
    operator T&()                 { return ((Entry*)Queue.Read())->Data; }
  };*/

 public:
             ~queue()             { Purge(); }
  // Read the current head
  qentry*    Read()               { return (qentry*)queue_base::Read(); }
  // Read complete => remove item from the queue
  void       CommitRead(qentry* e){ queue_base::CommitRead(e); delete e; }
  // Post an element to the Queue
  void       Write(const T& data) { queue_base::Write(new qentry(data)); }
  // Remove all elements from the queue
  void       Purge();
  // Search for the first Element matching templ in the queue.
  // This function is not reliable unless it is called
  // while the current objects Mutex is locked.
  // So be careful
  qentry*    Find(const T& templ, bool& inuse);
  // Remove all elements which equals data from the queue and
  // return the number of removed Elements.
  // Elements in current uncommited reads are not affected.
  int        Remove(const T& data);
  // Same as Remove but do not return until the element is really removed.
  // In case that the element is part of a current uncommited read the function
  // will block until the read is commited.
  int        ForceRemove(const T& data);
};

template <class T>
void queue<T>::Purge()
{ if (Head == NULL)
    return;
  qentry* ep;
  { Mutex::Lock lock(Mtx);
    ep = (qentry*)Head;
    if (ep == NULL)
      return;
    ep = (qentry*)ep->Next;
    if (EvRead.IsSet())
    { // reader is active, keep the first element
      Head->Next = NULL; // truncate
    } else
    { // reader is inactive, delete all
      delete (qentry*)Head;
      Head = NULL;
    }
    Tail = Head;
    EvEmpty.Reset();
  } // we do no longer need the Mutex
  while (ep)
  { qentry* ep2 = ep;
    ep = (qentry*)ep->Next;
    delete ep2;
  }
}

template <class T>
queue<T>::qentry* queue<T>::Find(const T& templ, bool& inuse)
{ Mutex::Lock lock(Mtx);
  qentry* ep = (qentry*)Head;
  while (ep)
  { if (ep->Data == templ)
    { inuse = ep == Head && !EvRead.IsSet();
      return ep;
    }
    ep = (qentry*)ep->Next;
  }
  return NULL;
}

template <class T>
int queue<T>::Remove(const T& data)
{ int r = 0;
  Mutex::Lock lock(Mtx);
  EntryBase** epp = &Head;
  for (;;)
  { qentry* ep = (qentry*)*epp;
    if (ep == NULL)
      break;
    if (ep->Data == data && (epp != &Head || EvRead.IsSet()))
    { *epp = ep->Next;
      epp = &ep->Next;
      delete ep;
      ++r;
    } else
      epp = &ep->Next;
  }
  return r;
}

template <class T>
int queue<T>::ForceRemove(const T& data)
{ int r = 0;
  bool syncreader = false;
  { Mutex::Lock lock(Mtx);
    EntryBase** epp = &Head;
    for (;;)
    { qentry* ep = (qentry*)*epp;
      if (ep == NULL)
        break;
      if (!(ep->Data == data))
        epp = &ep->Next;
       else if (epp == &Head && !EvRead.IsSet())
      { syncreader = true;
        epp = &ep->Next;
      } else
      { *epp = ep->Next;
        epp = &ep->Next;
        delete ep;
        ++r;
  } } }
  if (syncreader) // Well, this /may/ wait for the next reader.
    EvRead.Wait();
  return r;
}

#endif
