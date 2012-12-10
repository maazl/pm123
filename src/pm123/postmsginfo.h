/*
 * Copyright 2011-2012 Marcel Mueller
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

#ifndef POSTMSGINFO_H
#define POSTMSGINFO_H

#define INCL_WIN
#include "dependencyinfo.h"

#include <cpp/atomic.h>
#include <cpp/event.h>
#include <cpp/pmutils.h>

#include <os2.h>


class PostMsgWorker
{private:
  AtomicUnsigned  Filter;
  class_delegate<PostMsgWorker, const PlayableChangeArgs> Deleg;
  WindowMsg       Message;

 public:
  PostMsgWorker();
  void            Start(APlayable& ap, InfoFlags what, Priority pri,
                        HWND target, ULONG msg, MPARAM mp1, MPARAM mp2);
  bool            Cancel() { return Deleg.detach(); }
 private:
  void            InfoChangeHandler(const PlayableChangeArgs& args);
 protected:
  virtual void    OnCompleted();
};

/// Helper class to send a window message when some information becomes available.
class AutoPostMsgWorker : private PostMsgWorker
{private:
  AutoPostMsgWorker() {}
  ~AutoPostMsgWorker() {}
 public:
  /// Send a window message as soon an some information is available.
  /// @param ap APlayable object to query.
  /// @param what What kind of information is required to process the message.
  /// If \a what is zero the message is sent immediately.
  /// @param pri Priority of the information request. (Parameter to \c RequestInfo)
  /// @remarks The function allocates a observer object that dispatches the message
  /// as soon as the information is complete and destroys itself afterwards.
  /// If the information is immediately available the message is sent immediately
  /// without any allocation.
  /// \par Whatever happens, the window message is always sent exactly once
  /// if the information arrives before the application terminates.
  static  void    Start(APlayable& ap, InfoFlags what, Priority pri,
                        HWND target, ULONG msg, MPARAM mp1, MPARAM mp2);
 private:
  virtual void    OnCompleted();
};


/** Helper class that posts a window message on completion of a DependencyInfoWorker.
 */
class PostMsgDIWorker : public DependencyInfoWorker
{private:
  WindowMsg       Message;
 public:
  void            Start(DependencyInfoSet& data, HWND target, ULONG msg, MPARAM mp1, MPARAM mp2);
 private:
  virtual void    OnCompleted();
};

#endif
