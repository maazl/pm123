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
    bool     ReadActive;
  };

 protected:
  EntryBase* Head;
  EntryBase* Tail;
  Event      EvEmpty; // set when the queue is not empty
  Event      EvRead;  // set when a reader completes
 public:
  Mutex      Mtx;

 protected: // class should not be used directly!
             queue_base()         : Head(NULL), Tail(NULL) { EvRead.Set(); }
  EntryBase* RequestRead();
  void       CommitRead(EntryBase* entry);
  void       RollbackRead(EntryBase* entry);
 protected:
  void       Write(EntryBase* entry);
  void       Write(EntryBase* entry, EntryBase* after);
  EntryBase* Purge();
};

template <class T>
class queue : public queue_base
{public:
  struct qentry : public queue_base::EntryBase
  { T        Data;
    qentry(const T& data) : Data(data) {}
  };

 public:
             ~queue()             { Purge(); }
  // Read the next item
  qentry*    RequestRead()        { return (qentry*)queue_base::RequestRead(); }
  // Read complete => remove item from the queue
  void       CommitRead(qentry* e){ queue_base::CommitRead(e); delete e; }
  // Read aborted => reactivate the item in the queue
  void       RollbackRead(qentry* e){ queue_base::RollbackRead(e); }
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
{ qentry* rhead = (qentry*)queue_base::Purge();
  while (rhead)
  { qentry* ep = rhead;
    rhead = (qentry*)ep->Next;
    delete ep;
  }
}

template <class T>
queue<T>::qentry* queue<T>::Find(const T& templ, bool& inuse)
{ Mutex::Lock lock(Mtx);
  qentry* ep = (qentry*)Head;
  while (ep)
  { if (ep->Data == templ)
    { inuse = ep->ReadActive;
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
    if (ep->Data == data && !ep->ReadActive)
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
  for(;;)
  { { Mutex::Lock lock(Mtx);
      bool syncreader = false;
      EntryBase** epp = &Head;
      for (;;)
      { qentry* ep = (qentry*)*epp;
        if (ep == NULL)
          break;
        if (!(ep->Data == data))
          epp = &ep->Next;
         else if (ep->ReadActive)
        { syncreader = true;
          epp = &ep->Next;
        } else
        { *epp = ep->Next;
          epp = &ep->Next;
          delete ep;
          ++r;
      } }
      if (!syncreader)
        return r;
    }
    EvRead.Wait();
  }
}

#endif
