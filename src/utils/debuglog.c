/*
 * Copyright 2006-2008 Marcel Mueller
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

/* Configuration dependant log functions.
 * To post a log message write
 *   DEBUGLOG(("Format string with printf parameters like %s\n", "parameter"));
 * Note the double braces!
 * If the application is compiled in debug mode (defining DEBUG) the message
 * is written to stderr which can be easily captured by " 2>logfile".
 * Otherwise the line will not be compiled at all. Even the arguments are not
 * evaluated for their side effects. So be sure not to use expressions with
 * side effects.
 */

#include "debuglog.h"

#ifdef DEBUG

#define INCL_DOS
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <os2.h>
#include <assert.h>
#include <malloc.h>

static HMTX logmutex = NULLHANDLE;
                        
// log to stderr
void debuglog( const char* fmt, ... )
{
  va_list va;
  PTIB ptib;
  PPIB ppib;

  // Hack to get a more precise stack info: count the number of % in the format string and subtracht taht from the stack.
  size_t n = 0;
  const char* cp = fmt;
  for (;;)
  { cp = strchr(cp, '%');
    if (cp == NULL)
      break;
    switch (*++cp)
    {default:
      ++n;
     case '%': // escape => ignore
      ++cp;
      continue;
     case 0:
      break;
    }
    break;
  }

  if (logmutex == NULLHANDLE)
  { // create mutex
    DosEnterCritSec();
    if (logmutex == NULLHANDLE)
      DosCreateMutexSem(NULL, &logmutex, 0, FALSE);
    DosExitCritSec();
  }

  va_start( va, fmt );
  DosGetInfoBlocks( &ptib, &ppib );
  DosRequestMutexSem( logmutex, SEM_INDEFINITE_WAIT );
  fprintf( stderr, "%08ld %04lx:%04ld %08lx ", clock(), ppib->pib_ulpid, ptib->tib_ptib2->tib2_ultid, (ULONG)&fmt + n * sizeof(int) );
  vfprintf( stderr, fmt, va );
  assert(_heapchk() == _HEAPOK);
  DosReleaseMutexSem( logmutex );
  va_end( va );
  // Dirty hack to enforce threading issues to occur.
  DosSleep(0);
}

void oassert(unsigned long apiret, const char* file, int line, const char* msg)
{ if (apiret)
  { char buf[1024];
    os2_strerror(apiret, buf, sizeof(buf));
    DEBUGLOG(("Assertion at %s line %i failed: %s\n%s\n", file, line, msg, buf));
    abort();    
} }

void pmassert(const char* file, int line, const char* msg)
{ char buf[1024];
  os2pm_strerror(buf, sizeof(buf));
  if (*buf)
  { DEBUGLOG(("Assertion at %s line %i failed: %s\n%s\n", file, line, msg, buf));
    abort();    
} }

#endif
