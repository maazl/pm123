/*
 * Copyright 2007-2012 M.Mueller
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


#ifndef CPP_MUTEX_H
#define CPP_MUTEX_H

#define INCL_DOS
#include <config.h>
#include <assert.h>
#include <interlocked.h>
#include <os2.h>

#include <debuglog.h>


/*****************************************************************************
*
*  generic mutual exclusive section class
*
*  This is general mutex semaphore for use inside and outside the current
*  process. The Mutex can be inherited by fork() or in case of a named mutex
*  by simply using the same name.
*
*  usage:
*    Mutex mu;
*    ...
*    { Mutex::Lock lock(mu); // disable thread switching
*      ...                   // critical code section
*    }                       // release semaphore
*
*  named Mutex:
*    Mutex mu("MySpecialMutex");
*    ...
*    { Mutex::Lock lock(mu); // thread switching
*      ...                   // critical code section
*    }                       // release semaphore
*
*  usage:
*  - Create Mutex object at a shared memory location, e.g. as class
*    member, or pass the object's reference.
*      Mutex cs;
*  - To enter a critical section simply create an instance of
*    Mutex::Lock. The constructor will wait until the resource
*    becomes available. The destrurctor will release the semaphore when the
*    Lock object goes out of scope. This is particularly useful when
*    exceptions are cought outside the scope of the critical section.
*      { Mutex::Lock lo(cs); // aquire semaphore
*        ...                 // critical code section
*      }                     // release semaphore
*  - To check if the resource is successfully aquired after performing a timed
*    wait, simply query lo as boolean.
*      if (lo) ...
*  - If the scope dependency of the Mutex::Lock Object is not
*    adequate to you, you can use the Request and Release functions of the
*    Mutex object directly.
*  - It is permitted to destroy the critical section object while other
*    threads are waiting for the resource to become ready if all of these
*    threads check the return value of the Request function, even if they don't
*    use the timeout feature. This is guaranteed for the Mutex::Lock
*    class. Of course, other threads must not access the Mutex
*    Object after the error condition anymore.
*    You shoud be owner of the critical section when you use this feature to
*    ensure that no other thread is inside the critical section and tries
*    to release the resource afterwards.
*
*****************************************************************************/
/*class IMutex
{public:
  class Lock
  { IMutex&    CS;   // critical section handle, not owned
    bool       Own;
   private:    // non-copyable
               Lock(const Lock&);
    void       operator=(const Lock&);
   public:
               Lock(IMutex& cs, long ms = -1) : CS(cs), Own(cs.Request(ms)) {}
               ~Lock()                       { if (Own) Release(); }
    bool       Request(long ms = -1);
    bool       Release();
    operator   bool() const                  { return Own; }
  };

 public:
  virtual bool Request(long ms = -1) = 0;
  virtual bool Release() = 0;
};*/

class Mutex
{public:
  class Lock
  { Mutex&     CS;   // critical section handle, not owned
    bool       Own;
   private:    // non-copyable
               Lock(const Lock&);
    void       operator=(const Lock&);
   public:
               Lock(Mutex& cs, long ms = -1) : CS(cs), Own(cs.Request(ms)) {}
               ~Lock()                       { if (Own) Release(); }
    bool       Request(long ms = -1);
    bool       Release();
    operator   bool() const                  { return Own; }
  };

 protected:
  HMTX         Handle;
 private:      // non-copyable
               Mutex(const Mutex&);
  void         operator=(const Mutex&);
 public:
               Mutex(bool share = false);
               Mutex(const char* name); // Creates a _named_ mutex.
               ~Mutex();
  // Aquire Mutex object
  // If it takes more than ms milliseconds the funtion will return false.
  bool         Request(long ms = -1);
  // Release the Mutex object
  // On error, e.g. if you are not owner, the funtion returns false.
  bool         Release();
  // Check whether the current mutex is owned.
  // The function returns the number of nested requests currently active.
  // If the mutex is unowned the return value is zero.
  // If the return value is negative the requests belong to another thread.
  // If the return value is positive the Mutex is owned by the current thread.
  // These are the only reliable return values.
  int          GetStatus() const;
};


/*****************************************************************************
*
*  Critical section class.
*
*  This is an OS/2 special. Thread switching is disabled for the current task
*  as long as at least an instance of this class exists.
*
*  Since it is by definition impossible to create instances from different
*  threads, this class can never cause deadlocks.
*
*  You should keep the lifetime of this class as low as possible, because
*  disabling the thread switching implicitely ignores the priority of the
*  other threads. So even real time threads may be stopped from a idle time
*  thread. And if a higher priority thread from another task interrupts the
*  low priority thread that holds the critical section class your application
*  gets completely stopped.
*
*  usage:
*  { CrtiSect cs; // Block to protect
*    ...          // Do the protected steps
*  }
*  
*****************************************************************************/
class CritSect
{private:    // non-copyable
       CritSect (const CritSect&);
  void operator=(const CritSect&);
 public:
       CritSect  () { DEBUGLOG2(("CritSect::CritSect()\n")); ORASSERT(DosEnterCritSec()); }
       ~CritSect () { DosExitCritSec(); DEBUGLOG2(("CritSect::~CritSect()\n")); }
};


/*****************************************************************************
*
*  Helper to wait for a spin lock
*
*  This class provides
*  The Function Wait will block until Count returns to zero.
*  But Count may no longer be zero when Wait returns.
*
*****************************************************************************/
class SpinWait
{private:
  unsigned FastCycles;
  long     Timeout;
 public:
  SpinWait(long ms = -1, unsigned fast = 3) : FastCycles(fast+1), Timeout(ms) {}
  bool Wait();
  void Reset(long ms = -1, unsigned fast = 3) { FastCycles = fast+1, Timeout = ms; }
};


/*****************************************************************************
*
*  spin-lock class
*
*  This class retains a counter that counts the calls of Inc minus Dec.
*  The Function Wait will block until Count returns to zero.
*  But Count may no longer be zero when Wait returns.
*
*****************************************************************************/
class SpinLock
{private:
  volatile unsigned Count;
 private:      // non-copyable
               SpinLock(const SpinLock&);
  void         operator=(const SpinLock&);
 public:
               SpinLock()        : Count(0) {}
  unsigned     Peek() const      { return Count; }             
  void         Inc()             { InterlockedInc(&Count); }
  bool         Dec()             { ASSERT(Count); return InterlockedDec(&Count); }
  unsigned     Reset()           { return InterlockedXch(&Count, 0); } 
  void         Wait();
 public:
  class Use
  { SpinLock&  SL;
   private:    // non-copyable
               Use(const Use&);
    void       operator=(const Use&);
   public:
               Use(SpinLock& sl) : SL(sl) { sl.Inc(); }
               ~Use()            { SL.Dec(); }
  };
};


/*****************************************************************************
*
*  recusive spin-lock class
*
*  This is the same as SpinLock except that it does not wait for the own thread.
*
*****************************************************************************/
class RecSpinLock : public SpinLock
{private:
  volatile TID CurrentTID;
 public:
               RecSpinLock()     : CurrentTID(0) {}
  void         Inc();
  // return false if the SpinLock is freed
  bool         Dec();
  void         Wait();
};


/*****************************************************************************
*
*  event semaphore class
*
*  usage:
*  - Create Event object at a shared memory location, e.g. as class
*    member, or pass the object's reference.
*      Event ev;
*  - To wait for a signal call ev.Wait().
*  - To stop the Wait function from blocking call ev.Set().
*  - To case the Wait function to block in future call ev.Reset().
*  - All Event functions except for the destructor are thread safe.
*
*****************************************************************************/
class Event
{protected:
  HEV    Handle;
 private:// non-copyable
         Event(const Event&);
  void   operator=(const Event&);
 public:
         Event(bool share = false);
         Event(const char* name);
         ~Event();
  // Wait for a notify call.
  // If it takes more than ms milliseconds the funtion will return false.
  bool   Wait(long ms = -1);
  // Set the event to the signalled state causing all waiting threads to
  // resume.
  void   Set();
  // Reset the semaphore
  // The wait function will block in future.
  void   Reset();
  // Check whether the event is set.
  bool   IsSet() const;
};


template <class T> class SyncAccess;

struct ASyncAccess
{ Mutex Mtx;
};
template <class T>
class SyncRef
{ friend class SyncAccess<T>;
  template <class T2> friend class SyncRef;
  T* Obj;
 public:
  SyncRef() : Obj(NULL) {}
  SyncRef(T& obj) : Obj(&obj) {}
  template <class T2>
  SyncRef(const SyncRef<T2>& r) : Obj(r.Obj) {}
};

template <class T>
class SyncAccess
{ T& Obj;
 public:
  SyncAccess(T& obj)                : Obj(obj) { Obj.Mtx.Request(); }
  SyncAccess(const SyncRef<T>& obj) : Obj(*obj.Obj) { Obj.Mtx.Request(); }
  operator T*()                     { return &Obj; }
  T* operator->()                   { return &Obj; }
  ~SyncAccess()                     { Obj.Mtx.Release(); }
};

#endif
