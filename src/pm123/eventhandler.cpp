/*
 * Copyright 2011      M.Mueller
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

#define INCL_DOS

#include "eventhandler.h"

#include <cpp/cpputil.h>

#include <os2.h>
#include <stdlib.h>
#include <stdarg.h>


volatile EventHandler::Handler EventHandler::DefaultHandler = NULL;
volatile int_ptr<EventHandler::DispatchTable> EventHandler::LocalHandlers;
Mutex EventHandler::LocalMtx;


void EventHandler::Post(MESSAGE_TYPE level, const xstring& msg)
{ // Look for thread-local handler.
  int_ptr<DispatchTable> dp(LocalHandlers);
  if (dp)
  { // Get Thread-ID
    ULONG tid = getTID();
    // lookup
    if (dp->size() >= tid)
    { Handler lh = (*dp)[tid-1];
      if (lh)
      { // use local handler
        (*lh)(level, msg);
        return;
      }
    }
  }
  // use global handler
  Handler gh = DefaultHandler;
  if (gh)
  { (*gh)(level, msg);
    return;
  }
  DEBUGLOG(("EventHandler::Post: unhandled event: %i, %s\n", level, msg.cdata()));
}

void EventHandler::PostFormat(MESSAGE_TYPE level, const char* format, ...)
{ va_list va;
  va_start(va, format);
  Post(level, xstring().vsprintf(format, va));
  va_end(va);
}

EventHandler::Handler EventHandler::SetLocalHandler(Handler eh)
{ // Get Thread-ID
  ULONG tid = getTID();

  Mutex::Lock lock(LocalMtx);
  int_ptr<DispatchTable> dp(LocalHandlers);
  if (!eh)
  { // remove a local handler
    if (!dp || dp->size() < tid)
      return NULL;
  } else if (!dp)
  { dp = new DispatchTable(tid);
    memset(dp->get(), 0, tid * sizeof(Handler));
  } else if (dp->size() < tid)
  { DispatchTable* ndp = new DispatchTable(tid);
    memcpy(ndp->get(), dp->get(), dp->size());
    memset(ndp->get() + dp->size(), 0, (tid - dp->size()) * sizeof(Handler));
    dp = ndp;
  }
  swap(eh, (*dp)[tid-1]);
  LocalHandlers = dp;
  return eh;
}
