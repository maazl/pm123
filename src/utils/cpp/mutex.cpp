/*
 * Copyright 2002-2007 Marcel Mueller
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
#include <assert.h>
#include <errno.h>

#include "mutex.h"

#include <debuglog.h>
#include <string.h>


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
   APIRET rc = DosCreateMutexSem(NULL, &Handle, share*DC_SEM_SHARED, FALSE);
   assert(rc == 0);
}

Mutex::Mutex(const char* name)
{  DEBUGLOG(("Mutex(%p)::Mutex(%s)\n", this, name));
   char* cp = new char[strlen(name)+8];
   strcpy(cp, "\\SEM32\\");
   strcpy(cp+7, name);
   APIRET rc;
   do
   {  rc = DosCreateMutexSem((PSZ)name, &Handle, DC_SEM_SHARED, FALSE);
      if (rc != ERROR_DUPLICATE_NAME)
         break;
      Handle = 0;
      rc = DosOpenMutexSem((PSZ)name, &Handle);
   } while (rc == ERROR_SEM_NOT_FOUND);
   delete[] cp;
   assert(rc == 0);
}

Mutex::~Mutex()
{  DEBUGLOG(("Mutex(%p)::~Mutex()\n", this));
   DosCloseMutexSem(Handle);
}

bool Mutex::Request(long ms)
{  DEBUGLOG(("Mutex(%p)::Request(%li)\n", this, ms));
   #ifdef DEBUG
   // rough deadlock check
   APIRET rc = DosRequestMutexSem(Handle, ms < 0 ? 60000 : ms); // The mapping from ms == -1 to SEM_INDEFINITE_WAIT is implicitely OK.
   PID pid;
   TID tid;
   ULONG count;
   DosQueryMutexSem(Handle, &pid, &tid, &count);
   DEBUGLOG(("Mutex(%p)::Request - %u @ %u\n", this, rc, count));
   if (ms < 0 && rc == ERROR_TIMEOUT)
   { DEBUGLOG(("Mutex(%p)::Request - UNHANDLED TIMEOUT!!!\n", this));
     DosBeep(2000, 500);
   }
   assert(rc == 0 || rc == ERROR_TIMEOUT);
   return rc == 0;
   #else
   return DosRequestMutexSem(Handle, ms) == 0; // The mapping from ms == -1 to SEM_INDEFINITE_WAIT is implicitely OK.
   #endif
}

bool Mutex::Release()
{  PID pid;
   TID tid;
   ULONG count;
   DosQueryMutexSem(Handle, &pid, &tid, &count);
   DEBUGLOG(("Mutex(%p)::Release() @ %u\n", this, count));
   #ifdef DEBUG
   APIRET rc = DosReleaseMutexSem(Handle);
   assert(rc == 0);
   return true;
   #else
   return DosReleaseMutexSem(Handle) == 0;
   #endif
}

Mutex::Status Mutex::GetStatus() const
{  DEBUGLOG2(("Mutex(%p)::GetStatus()\n", this));
   PID pid;
   TID tid;
   ULONG count;
   APIRET rc = DosQueryMutexSem(Handle, &pid, &tid, &count);
   if (rc == ERROR_SEM_OWNER_DIED || count == 0)
     return Unowned;
   assert(rc == 0);
   PTIB tib;
   PPIB pib;
   DosGetInfoBlocks(&tib, &pib);
   return tib->tib_ptib2->tib2_ultid == tid && pib->pib_ulpid ? Mine : Owned;
}


/*****************************************************************************
*
*  Event class
*  Wrapper to OS2 EventSem.
*
*****************************************************************************/
Event::Event(bool share)
{  DEBUGLOG(("Event(%p)::Event(%u)\n", this, share));
   APIRET rc = DosCreateEventSem(NULL, &Handle, share*DC_SEM_SHARED, FALSE);
   assert(rc == 0);
}

Event::Event(const char* name)
{  char* cp = new char[strlen(name)+8];
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
   assert(rc == 0);
}

Event::~Event()
{  DosCloseEventSem(Handle); // can't handle errors here
}

bool Event::Wait(long ms)
{  APIRET rc = DosWaitEventSem(Handle, ms); // The mapping from ms == -1 to INFINITE is implicitely OK.
   if (rc == 0)
     return true;
   assert(rc == ERROR_TIMEOUT);
   return false;
}

void Event::Set()
{  APIRET rc = DosPostEventSem(Handle);
   //DEBUGLOG(("Event(%p)::Set - %x\n", this, rc));
   assert(rc == 0 || rc == ERROR_TOO_MANY_POSTS);
}

void Event::Reset()
{  ULONG cnt;
   APIRET rc = DosResetEventSem(Handle, &cnt);
   assert(rc == 0 || rc == ERROR_ALREADY_RESET);
}

Event::operator bool() const
{  ULONG cnt;
   DosQueryEventSem(Handle, &cnt);
   return cnt != 0; 
}


#if defined(__GNUC__)
const unsigned char InterlockedXchCode[] = 
{ 0x87, 0x11,             // xchg [ecx], edx
  0x89, 0xd0,             // mov eax, edx 
  0xC3                    // ret 
};
const unsigned char InterlockedIncCode[] = 
{ 0xF0, 0xFF, 0x01,       // lock inc dword [ecx] 
  0xC3                    // ret 
};
const unsigned char InterlockedDecCode[] = 
{ 0xF0, 0xFF, 0x09,       // lock dec dword [ecx]
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret
};
const unsigned char InterlockedAddCode[] = 
{ 0xF0, 0x01, 0x11,       // lock add [ecx], edx
  0xC3                    // ret 
};
const unsigned char InterlockedSubCode[] = 
{ 0xF0, 0x29, 0x11,       // lock sub [ecx], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedAndCode[] = 
{ 0xF0, 0x21, 0x11,       // lock and [ecx], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedOrCode[] = 
{ 0xF0, 0x09, 0x11,       // lock or [ecx], edx
  0xC3                    // ret 
};
const unsigned char InterlockedXorCode[] = 
{ 0xF0, 0x31, 0x11,       // lock xor [ecx], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedBtsCode[] = 
{ 0xF0, 0x0F, 0xAB, 0x11, // lock bts [ecx], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtrCode[] = 
{ 0xF0, 0x0F, 0xB3, 0x11, // lock btr [ecx], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtcCode[] = 
{ 0xF0, 0x0F, 0xBB, 0x11, // lock btr [ecx], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
#else
const unsigned char InterlockedXchCode[] = 
{ 0x87, 0x10,             // xchg [eax], edx
  0x89, 0xd0,             // mov eax, edx 
  0xC3                    // ret 
};
const unsigned char InterlockedIncCode[] = 
{ 0xF0, 0xFF, 0x00,       // lock inc dword [eax]
  0xC3                    // ret
};
const unsigned char InterlockedDecCode[] = 
{ 0xF0, 0xFF, 0x08,       // lock dec dword [eax]
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret
};
const unsigned char InterlockedAddCode[] = 
{ 0xF0, 0x01, 0x10,       // lock add [eax], edx
  0xC3                    // ret 
};
const unsigned char InterlockedSubCode[] = 
{ 0xF0, 0x29, 0x10,       // lock sub [eax], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedAndCode[] = 
{ 0xF0, 0x21, 0x10,       // lock and [eax], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedOrCode[] = 
{ 0xF0, 0x09, 0x10,       // lock or [eax], edx
  0xC3                    // ret 
};
const unsigned char InterlockedXorCode[] = 
{ 0xF0, 0x31, 0x10,       // lock xor [eax], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedBtsCode[] = 
{ 0xF0, 0x0F, 0xAB, 0x10, // lock bts [eax], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtrCode[] = 
{ 0xF0, 0x0F, 0xB3, 0x10, // lock btr [eax], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtcCode[] = 
{ 0xF0, 0x0F, 0xBB, 0x10, // lock btr [eax], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
#endif


