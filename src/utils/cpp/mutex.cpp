/*
 * Copyright 2002-2011 Marcel Mueller
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

// mutex.cpp - platform specific mutex semaphore class

#define INCL_BASE
#include <errno.h>

#include "mutex.h"

#include <debuglog.h>
#include <string.h>
#include <alloca.h>


bool Mutex::Lock::Request(long ms)
{  if (Own)
      return true;
   return Own = CS.Request(ms);
}

bool Mutex::Lock::Release()
{  if (!Own)
      return false;
   Own = false;
   return CS.Release();
}


Mutex::Mutex(bool share)
{  DEBUGLOG(("Mutex(%p)::Mutex(%u)\n", this, share));
   ORASSERT(DosCreateMutexSem(NULL, &Handle, share*DC_SEM_SHARED, FALSE));
}

Mutex::Mutex(const char* name)
{  DEBUGLOG(("Mutex(%p)::Mutex(%s)\n", this, name));
   char* cp = (char*)alloca(strlen(name)+8);
   strcpy(cp, "\\SEM32\\");
   strcpy(cp+7, name);
   APIRET rc;
   do
   {  rc = DosCreateMutexSem((PSZ)cp, &Handle, DC_SEM_SHARED, FALSE);
      if (rc != ERROR_DUPLICATE_NAME)
         break;
      Handle = NULLHANDLE;
      rc = DosOpenMutexSem((PSZ)cp, &Handle);
   } while (rc == ERROR_SEM_NOT_FOUND);
   OASSERT(rc);
}

Mutex::~Mutex()
{  DEBUGLOG(("Mutex(%p)::~Mutex()\n", this));
   ORASSERT(DosCloseMutexSem(Handle));
}

bool Mutex::Request(long ms)
{  DEBUGLOG(("Mutex(%p{%p})::Request(%li)\n", this, Handle, ms));
   #ifdef DEBUG_LOG
   // rough deadlock check
   APIRET rc = DosRequestMutexSem(Handle, ms < 0 ? 60000 : ms); // The mapping from ms == -1 to SEM_INDEFINITE_WAIT is implicitly OK.
   PID pid;
   TID tid;
   ULONG count;
   DosQueryMutexSem(Handle, &pid, &tid, &count);
   DEBUGLOG(("Mutex(%p)::Request - %u @ %u\n", this, rc, count));
   if (ms < 0 && rc == ERROR_TIMEOUT)
   { DEBUGLOG(("Mutex(%p)::Request - UNHANDLED TIMEOUT!!!\n", this));
     DosBeep(2000, 500);
   }
   ASSERT(rc == 0 || rc == ERROR_TIMEOUT);
   return rc == 0;
   #else
   return DosRequestMutexSem(Handle, ms) == 0; // The mapping from ms == -1 to SEM_INDEFINITE_WAIT is implicitly OK.
   #endif
}

bool Mutex::Release()
{  
   #ifdef DEBUG_LOG
   PID pid;
   TID tid;
   ULONG count;
   DosQueryMutexSem(Handle, &pid, &tid, &count);
   DEBUGLOG(("Mutex(%p{%p})::Release() @ %u\n", this, Handle, count));
   APIRET rc = DosReleaseMutexSem(Handle);
   DEBUGLOG(("Mutex::Release() done %u\n", rc));
   OASSERT(rc == NO_ERROR);
   return true;
   #else
   return DosReleaseMutexSem(Handle) == 0;
   #endif
}

int Mutex::GetStatus() const
{  DEBUGLOG2(("Mutex(%p)::GetStatus()\n", this));
   PID pid;
   TID tid;
   ULONG count;
   APIRET rc = DosQueryMutexSem(Handle, &pid, &tid, &count);
   if (rc == ERROR_SEM_OWNER_DIED || count == 0)
     return 0;
   OASSERT(rc);
   PTIB tib;
   PPIB pib;
   DosGetInfoBlocks(&tib, &pib);
   return tib->tib_ptib2->tib2_ultid == tid && pib->pib_ulpid == pid ? count : -count;
}


/*****************************************************************************
*
*  SpinLock class
*
*****************************************************************************/

void SpinLock::Wait()
{ DEBUGLOG(("SpinLock(%p)::Wait() - %u\n", this, Count));
  unsigned i = 5; // fast cycles
  do
  { if (!Count)
      return;
    DosSleep(0);
  } while (--i);
  // slow cycles
  while (Count)
  { DEBUGLOG(("SpinLock::Wait loop %u\n", Count));
    DosSleep(1);
  }
}


void RecSpinLock::Inc()
{ SpinLock::Inc();
  PTIB ptib;
  DosGetInfoBlocks(&ptib, NULL);
  ASSERT(CurrentTID == 0 || CurrentTID == ptib->tib_ptib2->tib2_ultid);
  CurrentTID = ptib->tib_ptib2->tib2_ultid;
}

bool RecSpinLock::Dec()
{ PTIB ptib;
  DosGetInfoBlocks(&ptib, NULL);
  ASSERT(CurrentTID == ptib->tib_ptib2->tib2_ultid);
  if (Peek() == 1)
    CurrentTID = 0;
  return SpinLock::Dec();
}

void RecSpinLock::Wait()
{ DEBUGLOG(("RecSpinLock(%p)::Wait() - %u\n", this));
  PTIB ptib;
  DosGetInfoBlocks(&ptib, NULL);
  if (CurrentTID == ptib->tib_ptib2->tib2_ultid)
    DEBUGLOG(("RecSpinLock::Wait recusrsion!\n"));
  else
    SpinLock::Wait();
}


/*****************************************************************************
*
*  Event class
*  Wrapper to OS2 EventSem.
*
*****************************************************************************/
Event::Event(bool share)
{  DEBUGLOG(("Event(%p)::Event(%u)\n", this, share));
   ORASSERT(DosCreateEventSem(NULL, &Handle, share*DC_SEM_SHARED, FALSE));
}

Event::Event(const char* name)
{  DEBUGLOG(("Event(%p)::Event(%s)\n", this, name));
   char* cp = new char[strlen(name)+8];
   strcpy(cp, "\\SEM32\\");
   strcpy(cp+7, name);
   APIRET rc;
   do
   {  rc = DosCreateEventSem((PSZ)name, &Handle, DC_SEM_SHARED, FALSE);
      if (rc != ERROR_DUPLICATE_NAME)
         break;
      Handle = 0;
      rc = DosOpenEventSem((PSZ)name, &Handle);
   } while (rc == ERROR_SEM_NOT_FOUND);
   delete[] cp;
   OASSERT(rc);
}

Event::~Event()
{  DEBUGLOG(("Event(%p)::~Event()\n", this));
   DosCloseEventSem(Handle); // can't handle errors here
}

bool Event::Wait(long ms)
{  DEBUGLOG(("Event(%p)::Wait(%li)\n", this, ms));
   APIRET rc = DosWaitEventSem(Handle, ms); // The mapping from ms == -1 to INFINITE is implicitely OK.
   if (rc == 0)
     return true;
   #ifdef DEBUG_LOG
   if (rc != ERROR_TIMEOUT)
     OASSERT(rc);
   #endif
   return false;
}

void Event::Set()
{  DEBUGLOG(("Event(%p)::Set()\n", this));
   APIRET rc = DosPostEventSem(Handle);
   OASSERT(rc == 0 || rc == ERROR_ALREADY_POSTED || rc == ERROR_TOO_MANY_POSTS);
   //DEBUGLOG(("Event(%p)::Set - %x\n", this, rc));
}

void Event::Reset()
{  DEBUGLOG(("Event(%p)::Reset()\n", this));
   ULONG cnt;
   APIRET rc = DosResetEventSem(Handle, &cnt);
   OASSERT(rc == 0 || rc == ERROR_ALREADY_RESET);
}

bool Event::IsSet() const
{  ULONG cnt;
   DosQueryEventSem(Handle, &cnt);
   //DEBUGLOG(("Event(%p)::IsSet() - %lu\n", this, cnt));
   return cnt != 0; 
}
