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


#ifndef CPP_CONTAINER_H
#define CPP_CONTAINER_H

#include <cpp/mutex.h>
#include <cpp/smartptr.h>


/*class vector_base
{protected:
  void** Data;
  size_t Size;
 private:
  // non-copyable
  sorted_vector_base(const sorted_vector_base& r);
  void operator=(const sorted_vector_base& r);
 public:
  // Create an event.
  sorted_vector_base(size_t size = 0) : Data(NULL), Size(size) {}
  ~sorted_vector_base();

  void insert(void* elem, size_t where);
  void removeat(size_t where); 
};

template <class T, class K>
sorted_vector : protected vector_base
{
  Mutex Mtx; // protect this instance
  
};*/

class queue_base
{protected:
  struct EntryBase
  { EntryBase* Next;
    EntryBase() : Next(NULL) {}
  };
 protected:
  EntryBase* Head;
  EntryBase* Tail;
  Mutex      Mtx;
  Event      Evnt;
  
 protected: // class should not be used directly!
             queue_base() : Head(NULL), Tail(NULL) {}
  void       Write(EntryBase* entry);
  EntryBase* Read();
};

template <class T>
class queue : public queue_base
{protected:
  struct Entry : public EntryBase
  { T        Data;
    Entry(const T& data) : Data(data) {}
  };
 
 public:
             ~queue()             { Purge(); }
  void       Write(const T& data) { queue_base::Write(new Entry(data)); }
  T          Read();
  void       Purge();
};

template <class T>
void queue<T>::Read()
{ sco_ptr<Entry> ep = (Entry*)queue_base::Read();
  return ep->Data;
}

template <class T>
void queue<T>::Purge()
{ Mutex::Lock lock(Mtx);
  while (Head)
  { Entry* ep = (Entry*)Head;
    Head = ep->Next;
    delete ep;
  }
  Tail = NULL;
  Evnt->Reset();
}

#endif
