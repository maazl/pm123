/*
 * Copyright 2010-2011 Marcel Mueller
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

#ifndef WAITINFO_H
#define WAITINFO_H

#include "aplayable.h"

#include <cpp/event.h>
#include <cpp/mutex.h>


/** Class to wait for a desired information.
 * @remarks Note that for aggregate type information you must additionally check
 * whether the right kind of information is already available.
 */
class WaitLoadInfo
{private:
  InfoFlags                 Filter;
  Event                     EventSem;
  class_delegate<WaitLoadInfo, const PlayableChangeArgs> Deleg;
 private:
  void                      InfoChangeEvent(const PlayableChangeArgs& args);
 public:
                            WaitLoadInfo();
  /// @brief Create a \c WaitInfo Semaphore that is posted once all information specified by Filter
  /// is loaded.
  /// @details To be safe the constructor of this class must be invoked either while the mutex
  /// of the object is held or before the asynchronous request. And after the request
  /// the available information should be removed from the request by calling \c CommitInfo.
  /// Otherwise WaitInfo will deadlock.
                            WaitLoadInfo(APlayable& inst, InfoFlags filter);
                            ~WaitLoadInfo()     { DEBUGLOG(("WaitInfo(%p)::~WaitInfo()\n", this)); }
  void                      Start(APlayable& inst, InfoFlags filter);
  /// Notify the class instance about informations that is now available.
  /// This kind of information is removed from the wait condition.
  virtual void              CommitInfo(InfoFlags what);
  /// Wait until all requested kind of information has been loaded or an error occurred.
  /// the function returns false if the given time elapsed or the Playable
  /// object died.
  bool                      Wait(long ms = -1)  { DEBUGLOG(("WaitLoadInfo(%p)::Wait(%li) - %x\n", this, ms, Filter)); EventSem.Wait(ms); return Filter == 0; }
};

class WaitInfo : private WaitLoadInfo
{private:
  int_ptr<APlayable>        Inst;
  Reliability               Rel;
 private:
  virtual void              CommitInfo(InfoFlags what);
 public:
  bool                      Wait(APlayable& inst, InfoFlags what, Reliability rel, long ms = -1);
};

class WaitAggregateInfo : private WaitLoadInfo
{private:
  int_ptr<APlayable>        Inst;
  AggregateInfo*            AI;
  Reliability               Rel;
 private:
  virtual void              CommitInfo(InfoFlags what);
 public:
  bool                      Wait(APlayable& inst, AggregateInfo& ai, InfoFlags what, Reliability rel, long ms = -1);
};

#endif
