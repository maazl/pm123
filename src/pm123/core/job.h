/*  
 * Copyright 2012-2013 Marcel Mueller
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


#ifndef JOB_H
#define JOB_H

#include "dependencyinfo.h"


class Job
{public:
  /// Priority of the job.
  const Priority            Pri;
 public:
  static Job                SyncJob;
  static Job                NoBlockJob;
 private:
  Job(const Job&);
  void operator=(const Job&);
 protected:
  Job(Priority pri)        : Pri(pri) {}
  ~Job()                   {}
 public:
  /// Request information for \a target and if unsuccessful store \a target in the dependency list.
  virtual InfoFlags         RequestInfo(APlayable& target, InfoFlags what);
  /// Request aggregate information for \a target and if unsuccessful
  /// store \a target in the dependency list.
  virtual volatile const AggregateInfo& RequestAggregateInfo(APlayable& target,
                            const PlayableSetBase& excluding, InfoFlags& what);
};


/// @brief Represents a worker job.
/// @details A Job is the priority of the job and a set of dependencies
/// that prevents the job from being executed completely.
/// If a job is applied to an \c APlayable object all previously requested tasks
/// at the given or higher priority should be executed.
/// The methods \c RequestInfo and \c RequestAggregateInfo provide a convenient way
/// to ensure this.
class JobSet : public Job
{protected:
  /// Next dependency set.
  DependencyInfoPath        Depends;
 public:
  /// Dependencies of uncompleted tasks. If this is not initial
  /// after the job completed, a \c DependencyInfoWorker will wait
  /// for the dependencies, and once it signals, the job will be
  /// rescheduled.
  DependencyInfoSet         AllDepends;

 public:
  /// Create a \c JobSet for the given priority level.
  /// @param pri Priority of the requests placed by this job.
                            JobSet(Priority pri) : Job(pri) {}
  /// Request information for \a target and if unsuccessful store \a target in the dependency list.
  virtual InfoFlags         RequestInfo(APlayable& target, InfoFlags what);
  /// Request aggregate information for \a target and if unsuccessful
  /// store \a target in the dependency list.
  virtual volatile const AggregateInfo& RequestAggregateInfo(APlayable& target, const PlayableSetBase& excluding, InfoFlags& what);
  /// Commit partial job.
  /// @return true if there is anything to commit.
  bool                      Commit();
  /// Discard partial job.
  void                      Rollback()          { Depends.Clear(); }
};

class RecursiveJobSet : public JobSet
{private:
  /// Do not execute requests on objects other than this synchronously.
  /// @remarks The pointer does not take a strong reference to the referred object
  /// and should only be used foe instance equality checks.
  const Playable* const     SyncOnly;

 private:
  Priority                  GetRequestPriority(APlayable& target);
 public:
  /// Create a \c JobSet for the given priority level.
  /// @param pri Priority of the requests placed by this job.
  /// @param synconly If this parameter is not \c NULL requests do not inherit the \c TrySync flag.
  /// if they refer <em>not</em> to the object \a synconly.
  RecursiveJobSet(Priority pri, Playable* synconly) : JobSet((pri & ~PRI_Sync) | PRI_TrySync), SyncOnly(synconly) {}
  /// Request information for \a target and if unsuccessful store \a target in the dependency list.
  virtual InfoFlags         RequestInfo(APlayable& target, InfoFlags what);
  /// Request aggregate information for \a target and if unsuccessful
  /// store \a target in the dependency list.
  virtual volatile const AggregateInfo& RequestAggregateInfo(APlayable& target, const PlayableSetBase& excluding, InfoFlags& what);
};

#endif
