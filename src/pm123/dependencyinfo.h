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


#ifndef DEPENDENCYINFO_H
#define DEPENDENCYINFO_H

#include "aplayable.h"

#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/container/sorted_vector.h>


class DependencyInfoPath
{public:
  struct Entry
  { const int_ptr<APlayable> Inst;
    InfoFlags               What;
    Entry(APlayable& inst, InfoFlags what)      : Inst(&inst), What(what) {}
    static int              compare(const Entry& l, const APlayable& r);
  };
 protected:
  typedef sorted_vector_own<Entry,APlayable,&Entry::compare> SetType;

 protected:
  SetType                   MandatorySet;

 protected:
  static SetType&           AccessMandatorySet(DependencyInfoPath& that) { return that.MandatorySet; }
  #ifdef DEBUG_LOG
  static void               DumpSet(xstringbuilder& dest, const SetType& set);
  #endif
 public:
  /// Swap two instances.
  void                      Swap(DependencyInfoPath& r) { MandatorySet.swap(r.MandatorySet); }
  /// Return the total number of dependencies.
  size_t                    Size() const        { return MandatorySet.size(); }
  /// Query item in set.
  /// @param i zero based index, must be less than Size()
  const Entry&              operator[](size_t i) const { return *MandatorySet[i]; }

  /// Add a new dependency.
  /// @param inst We depend on \a inst.
  /// @param what We depend on the information \a what of \a inst.
  /// @remarks All dependencies added by this functions are mandatory. If the same instance is added twice,
  /// the \a what flags are joined.
  void                      Add(APlayable& inst, InfoFlags what);
  /// Reinitialize
  void                      Clear()             { MandatorySet.clear(); }
  #ifdef DEBUG_LOG
  xstring                   DebugDump() const;
  #endif
};
/** @brief Set of required dependencies to complete a request.
 * @details This set consists of two parts: a list of mandatory dependencies and a list of
 * optional dependencies. The dependency is completed exactly if all mandatory dependencies
 * and at least one optional dependency has been resolved.
 */
class DependencyInfoSet : private DependencyInfoPath
{ friend class DependencyInfoWorker;

 private:
  SetType                   OptionalSet;

 public:
  /// Swap two instances.
  void                      Swap(DependencyInfoSet& r) { MandatorySet.swap(r.MandatorySet); OptionalSet.swap(r.OptionalSet); }
  /// Return the number of mandatory dependencies.
  size_t                    MandatorySize() const { return MandatorySet.size(); }
  /// Return the number of optional dependencies.
  size_t                    OptionalSize() const { return OptionalSet.size(); }
  /// Return the total number of dependencies.
  size_t                    Size() const        { return MandatorySet.size() + OptionalSet.size(); }
  /// Query item in set.
  /// @param i zero based index, must be less than Size()
  const Entry&              operator[](size_t i) const { return i < MandatorySet.size() ? *MandatorySet[i] : *OptionalSet[i-MandatorySet.size()]; }
  /// Reinitialize
  void                      Clear()             { MandatorySet.clear(); OptionalSet.clear(); }
  /// @brief Add an alternate dependency set plan to this object.
  /// @param r Dependencies to join. The right list is always cleared after this call.
  /// @details All mandatory dependencies that are common in both objects are mandatory in the result too.
  /// Dependencies that occur only in one of the operands (optional or mandatory) are optional.
  /// Except if one operand is a subset of the other, in which case the optional dependencies
  /// are cleared, because in this case one of the operands has logically an empty optional list,
  /// which makes the optional condition always true. So in fact the optional list will contain
  /// either no or at least two entries.
  void                      Join(DependencyInfoPath& r);
  #ifdef DEBUG_LOG
  xstring                   DebugDump() const;
  #endif
};


/** @brief Signal when a dependency set is ready.
 * @details The class takes a \c DependencySet and waits for all Information mentioned in the mandatory dependency
 * list of the set to become available at least with the reliability \c REL_Cached. Once this completed
 * it waits for at least one optional dependency to become reliable. If there are no optional dependencies, this
 * is immediately true. After that the abstract method OnCompletion is called exactly once.
 */
class DependencyInfoWorker
{protected:
  typedef class_delegate<DependencyInfoWorker, const PlayableChangeArgs> DelegType;
 protected:
  /// @brief Mutex to protect the instance data.
  /// @remarks The mutex is static to save resources. Threads should not own it for long.
  static Mutex              Mtx;
  /// Remaining dependencies. Must be initialized before \c Start().
  DependencyInfoSet         Data;
  /// Information types to wait for when waiting for a mandatory dependency.
  InfoFlags                 NowWaitingFor;
  /// Delegate to wait for an mandatory dependency.
  DelegType                 Deleg;
  /// @brief List of delegates for optional dependencies.
  /// @details This list is non-null if and only if we are waiting for optional dependencies.
  vector_own<DelegType>     DelegList;

 protected:
  /// @brief Create worker to wait for a DependencyInfoSet.
  /// @remarks You must override \c OnCompleted to instantiate this class.
                            DependencyInfoWorker();
  /// @brief Destroy the worker.
  /// @remarks Destroying a worker that has been started and not yet completed is undefined behavior.
  /// Further note that this method is non virtual and objects of this class should not be destroyed
  /// directly polymorphically. Therefore it is protected.
                            ~DependencyInfoWorker() {};
  /// @brief Start the worker.
  /// @remarks This explicit call is required because starting too early could result in a virtual pure function call.
  void                      Start();
 public:
  void                      Cancel();
 private:
  /// Abstract function that is called exactly once when the dependencies are fulfilled.
  /// This could happen from almost any thread and should be treated as something like interrupt context.
  virtual void              OnCompleted() = 0;
 private:
  void                      MandatoryInfoChangeEvent(const PlayableChangeArgs& args);
  void                      OptionalInfoChangeEvent(const PlayableChangeArgs& args);
};

/*class WaitDependencyInfo : public DependencyInfo
{private:
  Event                     EventSem;

 private:
  virtual void              OnCompleted();
 public:
                            WaitDependencyInfo(DependencyInfoQueue& data);
  bool                      Wait(long ms = -1)  { EventSem.Wait(ms); return QueueData.IsEmpty() && NowWaitingFor == IF_None; }
};*/


/// @brief Represents a worker job.
/// @details A Job is the priority of the job and a set of dependencies
/// that prevents the job from being executed completely.
/// If a job is applied to an \c APlayable object all previously requested tasks
/// at the given or higher priority should be executed.
/// The methods \c RequestInfo and \c RequestAggregateInfo provide a convenient way
/// to ensure this.
class JobSet
{private:
  /// Next dependency set.
  DependencyInfoPath        Depends;
 public:
  /// Priority of the job.
  const Priority            Pri;
  /// Dependencies of uncompleted tasks. If this is not initial
  /// after the job completed, a \c DependencyInfoWorker will wait
  /// for the dependencies, and once it signals, the job will be
  /// rescheduled.
  DependencyInfoSet         AllDepends;
  /// Create a \c JobSet for the given priority level.
                            JobSet(Priority pri) : Pri(pri) {}
  /// Request information for \a target and if unsuccessful store \a target in the dependency list.
  InfoFlags                 RequestInfo(APlayable& target, InfoFlags what);
  /// Request aggregate information for \a target and if unsuccessful
  /// store \a target in the dependency list.
  volatile const AggregateInfo& RequestAggregateInfo(APlayable& target, const PlayableSetBase& excluding, InfoFlags& what);
  /// Commit partial job.
  void                      Commit()            { AllDepends.Join(Depends); }
  /// Discard partial job.
  void                      Rollback()          { Depends.Clear(); }
};

#endif
