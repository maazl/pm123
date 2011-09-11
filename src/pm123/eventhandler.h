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

#ifndef  PM123_EVENTHANDLER_H
#define  PM123_EVENTHANDLER_H

#include <plugin.h>
#include <cpp/xstring.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <interlocked.h>
#include <cpp/container/vector.h>


/// Dispatcher class to deal with thread-local event handlers.
/// All PM123 messages should be routed through this class.
/// This enables PM123 to dispatch messages related to a remote command
/// to their origin rather than showing it to the user.
class EventHandler
{public:
  /// Type for the event handlers that consume the messages.
  typedef void DLLENTRYP(Handler)(MESSAGE_TYPE type, const xstring& msg);
 private:
  struct DispatchTable : public Iref_count, public sco_arr<Handler>
  { DispatchTable(size_t size) : sco_arr<Handler>(size) {}
  };

 private:
  /// global event handler, if any
  static Handler volatile DefaultHandler;
  /// List of local event handlers. The list index is the is TID-1.
  /// @remarks The list never gets reallocated. Instead the entire dispatch table might be replaced.
  static volatile int_ptr<DispatchTable> LocalHandlers;
  /// Protect LocalHandlers for writing.
  static Mutex   LocalMtx;

 public:
  /// Emit a message
  static void Post(MESSAGE_TYPE level, const xstring& msg);
  /// Emit a formatted message
  static void PostFormat(MESSAGE_TYPE level, const char* fmt, ...);

  /// Set the global event handler and return the previous one.
  /// @param eh New global event handler, might be \c NULL.
  /// @remarks The exchange operation itself is atomic, but it is not guaranteed
  /// that the previous event handler is not currently called by another thread
  /// after the function returned.
  static Handler SetDefaultHandler(Handler eh) { return (Handler)InterlockedXch((volatile unsigned int*)&DefaultHandler, (unsigned int)eh); }
  /// Set the local event handler for the current thread and return the previous one.
  /// If a local event handler is set, it receives all events from the current thread.
  /// The global event handler is no longer called.
  /// @param eh New local event handler, might be \c NULL to revoke thread-local
  /// event redirection.
  /// @remarks  The exchange operation itself is atomic, but it is not guaranteed
  /// that the previous event handler is not currently called by another thread
  /// after the function returned.
  static Handler SetLocalHandler(Handler eh);

 public:
  /// Redirect events from the current thread until this class instance goes out of scope.
  class LocalRedirect
  { Handler Old;
   public:
    LocalRedirect(Handler eh) : Old(SetLocalHandler(eh)) {}
    ~LocalRedirect() { SetLocalHandler(Old); }
  };
};


#endif

