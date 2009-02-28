/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2009 Marcel Mueller
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

#define  INCL_DOS
#define  INCL_PM
#define  INCL_ERRORS

#include "utilfct.h"
#include <os2.h>

#include "debuglog.h"

/* Places the current thread into a wait state until another thread
   in the current process has ended. Kills another thread if the
   time expires and return FALSE. */
BOOL
wait_thread( TID tid, ULONG msec )
{
  while( DosWaitThread( &tid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED )
  { if (msec < 100)
    { // Emergency exit
      DEBUGLOG(("wait_thread: Thread %u has not terminated within time\n", tid));
      DosKillThread( tid );
      return FALSE;
    }
    DosSleep( 100 );
    msec -= 100;
  }
  return TRUE;
}

BOOL
wait_thread_pm( HAB hab, TID tid, ULONG msec )
{
  while( DosWaitThread( &tid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED )
  { QMSG qmsg;
    if (WinPeekMsg(hab, &qmsg, NULLHANDLE, 0, 0, PM_REMOVE))
      WinDispatchMsg( hab, &qmsg );
    else if (msec < 31) // Well, not that exact
    { // Emergency exit
      DEBUGLOG(("wait_thread: Thread %u has not terminated within time\n", tid));
      DosKillThread( tid );
      return FALSE;
    } else
    { DosSleep( 31 );
      msec -= 31;
    }
  }
  return TRUE;
}
