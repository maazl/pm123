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


#ifndef CPP_QUEUE_H
#define CPP_QUEUE_H

#include <config.h>
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <stdlib.h>

#include <debuglog.h>

/** This type MUST be base of all queued items. */
struct qentry
{ qentry*    Next;
};

/** Non template base class of queue */
class queue_base
{protected:
  /// Head of queue items
  qentry*    Head;
  /// Tail of queue items
  qentry*    Tail;
  /// set when a message is written
  Event      EvEmpty;
 public:
  Mutex      Mtx;

 public:
             queue_base()         : Head(NULL), Tail(NULL) {}
  qentry*    Read();
  void       Write(qentry* entry);
  void       Write(qentry* entry, qentry* after);
  void       WriteFront(qentry* entry);
  qentry*    Purge();
  bool       ForEach(bool (*action)(const qentry* entry, void* arg), void* arg);
};

/** Queue class.
 * @tparam T must derive from qentry.
 */ 
template <class T>
class queue : public queue_base
{private:
  struct ActionProxyParam
  { bool     (*Action)(const T& entry, void* arg);
    void*    Arg;
  };
  static bool ActionProxy(const qentry* entry, void* arg)
             { return (*((ActionProxyParam*)arg)->Action)(*(const T*)entry, ((ActionProxyParam*)arg)->Arg); }
 public:
             ~queue()             { Purge(); }
  /// Read and return the next item.
  T*         Read()               { return (T*)queue_base::Read(); }
  /// Post an element to the queue.
  void       Write(T* entry)      { queue_base::Write(entry); }
  /// Post an element to the queue at a certain location.
  void       Write(T* entry, T* after) { queue_base::Write(entry, after); }
  /// Puts back an element to the front queue.
  void       WriteFront(T* entry) { queue_base::WriteFront(entry); }
  /// Remove all elements from the queue
  void       Purge();
  /// @brief Enumerate queue content.
  /// @param action Call-back method. Called once for each queued item.
  /// If the call-back returns false, the enumeration is aborted with return code false.
  /// @return true if the enumeration completed without abort.
  /// @note Note that action is called from synchronized context.
  bool       ForEach(bool (*action)(const T& entry, void* arg), void* arg)
             { ActionProxyParam args_proxy = { action, arg };
               return queue_base::ForEach(&ActionProxy, &args_proxy);
             }
};

template <class T>
void queue<T>::Purge()
{ qentry* rhead = queue_base::Purge();
  while (rhead)
  { qentry* ep = rhead;
    rhead = (qentry*)ep->Next;
    delete (T*)ep;
  }
}


/** @brief Non template base class of \c priority_queue.
 *
 * The queue is internally organizes a a single queue where the items are ordered
 * with descending priority. An index, \c PriEntries, is used to keep all operations O(1).
 *
 * Examples
 * -- Number of Items ---|- Queue pointers (-> item no) -| Content (priority vs. item no)
 * Pri.0 | Pri.1 | Pri.2 | Head  |Tail[0]|Tail[1]|Tail[2]| 0 1 2 3 4 5 6
 * ======|=======|=======|=======|=======|=======|=======|===============================
 *   0   |   0   |   0   | NULL  | NULL  | NULL  | NULL  |
 *   1   |   0   |   0   | -> 0  | -> 0  | -> 0  | -> 0  | 0
 *   0   |   1   |   0   | -> 0  | NULL  | -> 0  | -> 0  | 1
 *   0   |   0   |   1   | -> 0  | NULL  | NULL  | -> 0  | 2
 *   2   |   2   |   1   | -> 0  | -> 1  | -> 3  | -> 4  | 0 0 1 1 2
 *   2   |   0   |   2   | -> 0  | -> 1  | -> 1  | -> 3  | 0 0 2 2
 *
 * @note The class is optimized for only a limited number of priority levels.
 */
class priority_queue_base
{protected:
  struct PriEntry
  { qentry*    Tail;    // Tail of high priority queue items
    Event      EvEmpty; // Set when a message is written
    PriEntry() : Tail(NULL) {}
  };
  
 protected:
  /// Head of queue items
  qentry*      Head;
  /// Tail pointers and Events for each priority level.
  PriEntry* const PriEntries;
  const size_t Priorities;
 public:
  Mutex        Mtx;

 protected:
  #if defined(DEBUG_LOG) && DEBUG_LOG >= 2
  void         Dump() const;
  #else
  void         Dump() const {}
  #endif
  #ifdef DEBUG
  void         Check() const;
  void         Fail(const char* file, int line, const char* msg) const;
  #else
  void         Check() const {}
  #endif
  
 public:
               priority_queue_base(size_t priorities);
               ~priority_queue_base();
  /// Read the next item with at least \a *priority (numerically less or same).
  /// @param priority Input: priority level to read.
  ///                 Output: priority level of the returned message (less or same than on input).
  qentry*      Read(size_t& priority);
  /// Append a queue item at priority level \a priority.
  void         Write(qentry* entry, size_t priority);
  /// Prepend a queue item at priority level \a priority.
  void         WriteFront(qentry* entry, size_t priority);
  /// Clear the queue and return the original head of the queue.
  qentry*      Purge();
  /// Enumerate all queue items.
  /// @param action \a *action is called for each item.
  /// If the call-back returns false, the enumeration is aborted with return code false.
  /// @return true if the enumeration completed without abort.
  /// @note \a *action is called from synchronized context. So be careful to avoid deadlocks.
  bool         ForEach(bool (*action)(const qentry* entry, size_t priority, void* arg), void* arg);
};

template <class T>
class priority_queue : public priority_queue_base
{private:
  struct ActionProxyParam
  { bool       (*Action)(const T& entry, size_t priority, void* arg);
    void*      Arg;
  };
  static bool ActionProxy(const qentry* entry, size_t priority, void* arg)
               { return (*((ActionProxyParam*)arg)->Action)(*(const T*)entry, priority, ((ActionProxyParam*)arg)->Arg); }
 public:
               priority_queue(size_t priorities) : priority_queue_base(priorities) {}
               ~priority_queue()      { Purge(); }
  /// Read the next item and return the priority of the read item.
  T*           Read(size_t& priority) { return (T*)priority_queue_base::Read(priority); }
  /// Post an element to the Queue
  void         Write(T* entry, size_t priority) { priority_queue_base::Write(entry, priority); }
  /// Puts back an element into the queue.
  void         WriteFront(T* entry, size_t priority) { priority_queue_base::WriteFront(entry, priority); }
  /// Remove all elements from the queue.
  void         Purge();
  /// Enumerate all queue items.
  /// @param action \a *action is called for each item.
  /// If the call-back returns false, the enumeration is aborted with return code false.
  /// @return true if the enumeration completed without abort.
  /// @remarks Note that the call-back is called from synchronized context.
  bool         ForEach(bool (*action)(const T& entry, size_t priority, void* arg), void* arg)
               { ActionProxyParam args_proxy = { action, arg };
                 return priority_queue_base::ForEach(&ActionProxy, &args_proxy);
               }
};

template <class T>
void priority_queue<T>::Purge()
{ qentry* rhead = (qentry*)priority_queue_base::Purge();
  while (rhead)
  { qentry* ep = rhead;
    rhead = (qentry*)ep->Next;
    delete (T*)ep;
  }
}

#endif
