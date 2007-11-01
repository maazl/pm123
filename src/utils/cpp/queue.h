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


class queue_base
{public: // VAC++ 3.0 requires base types to be public
  struct EntryBase
  { EntryBase* Next;
  };
  class ReaderBase
  {public:
    queue_base& Queue;
   private:
    ReaderBase(const ReaderBase&);
    void operator=(const ReaderBase&);
   protected:
    ReaderBase(queue_base& queue);
  };

 protected:
  EntryBase* Head;
  EntryBase* Tail;
  Event      EvEmpty;
  Event      EvRead;
 public:
  Mutex      Mtx;

 protected: // class should not be used directly!
             queue_base()         : Head(NULL), Tail(NULL) { EvRead.Set(); }
  EntryBase* CommitRead();
 public:
  void       RequestRead();
  EntryBase* Read()               { return Head; }
  void       Write(EntryBase* entry);
};

template <class T>
class queue : private queue_base
{private:
  struct Entry : public EntryBase
  { T        Data;
    Entry(const T& data) : Data(data) {}
  };
 public:
  class Reader;
  friend class Reader;
  class Reader : private ReaderBase
  {public:
    Reader(queue<T>& queue)       : ReaderBase(queue) {}
    ~Reader()                     { ((queue<T>&)Queue).CommitRead(); }
    operator T&()                 { return ((Entry*)Queue.Read())->Data; }
  };

 public:     queue_base::Mtx;

 protected:
  void       CommitRead()         { delete (Entry*)queue_base::CommitRead(); }
 public:
             ~queue()             { Purge(); }
  // Post an element to the Queue
  void       Write(const T& data) { queue_base::Write(new Entry(data)); }
  // Remove all elements from the queue
  void       Purge();
  // Search for the first Element matching templ in the queue.
  // This function is not reliable unless it is called
  // while the current objects Mutex is locked.
  // So be careful
  T*         Find(const T& templ, bool& inuse);
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
  Entry* ep;
  { Mutex::Lock lock(Mtx);
    ep = (Entry*)Head;
    if (ep == NULL)
      return;
    ep = (Entry*)ep->Next;
    if (EvRead.IsSet())
    { // reader is active, keep the first element
      Head->Next = NULL; // truncate
    } else
    { // reader is inactive, delete all
      delete (Entry*)Head;
      Head = NULL;
    }
    Tail = Head;
    EvEmpty.Reset();
  } // we do no longer need the Mutex
  while (ep)
  { Entry* ep2 = ep;
    ep = (Entry*)ep->Next;
    delete ep2;
  }
}

template <class T>
T* queue<T>::Find(const T& templ, bool& inuse)
{ Mutex::Lock lock(Mtx);
  Entry* ep = (Entry*)Head;
  while (ep)
  { if (ep->Data == templ)
    { inuse = ep == Head && !EvRead.IsSet();
      return &ep->Data;
    }
    ep = (Entry*)ep->Next;
  }
  return NULL;
}

template <class T>
int queue<T>::Remove(const T& data)
{ int r = 0;
  Mutex::Lock lock(Mtx);
  EntryBase** epp = &Head;
  for (;;)
  { Entry* ep = (Entry*)*epp;
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
    { Entry* ep = (Entry*)*epp;
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
