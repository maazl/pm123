/*  
 * Copyright 2009-2012 Marcel Mueller
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


#ifndef APLAYABLE_H
#define APLAYABLE_H

#include "infostate.h"

#include <cpp/event.h>
#include <cpp/atomic.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/queue.h>
#include <cpp/cpputil.h>
#include <cpp/url123.h>


class APlayable;
/** Parameters for InfoChange Event
 * An initial Event with Changed, Loaded and Invalidated == IF_None is fired
 * just when the playable instance dies. You can check for that with IsInitial().
 * Observers should not access instance after this event completed.
 * They are canceled automatically.
 * The Loaded flags are set if an asynchronous request is handled. This does not imply
 * that the corresponding Changed bits are set too, because the desired information may not
 * have changed. So if you are waiting for the completion of an asynchronous request
 * use the Loaded bits, and if you want to track changes use the Changed bits.
 * Loaded | Changed | Meaning
 * =======+=========+=======================================================================
 *    0   |    0    | Information is untouched since the last InfoChange event.
 *    1   |    0    | Information has been refreshed but the value did not change.
 *    0   |    1    | Cached information has been loaded.
 *    1   |    1    | Information has been updated and changed.
 * If the bit IF_Child is set in the Changed vector the event is in fact of the derived type
 * CollectionChangeArgs. */
struct PlayableChangeArgs
{ APlayable&                  Instance;   // Related Playable object.
  APlayable*                  Origin;     // Points to an object that caused that initiated the events, optional.
  InfoFlags                   Loaded;     // Bit vector with loaded information.
  InfoFlags                   Changed;    // Bit vector with changed information.
  InfoFlags                   Invalidated;// Bit vector with invalidated information.
  PlayableChangeArgs(APlayable& inst, APlayable* orig, InfoFlags loaded, InfoFlags changed, InfoFlags invalidated)
                                                  : Instance(inst), Origin(orig), Loaded(loaded), Changed(changed), Invalidated(invalidated) {}
  PlayableChangeArgs(APlayable& inst, InfoFlags loaded, InfoFlags changed)
                                                  : Instance(inst), Origin(&inst), Loaded(loaded), Changed(changed), Invalidated(IF_None) {}
  PlayableChangeArgs(APlayable& inst)             : Instance(inst), Origin(&inst), Loaded(IF_None), Changed(IF_None), Invalidated(IF_None) {}
  PlayableChangeArgs(APlayable& inst, const PlayableChangeArgs& r) : Instance(inst), Origin(r.Origin), Loaded(r.Loaded), Changed(r.Changed), Invalidated(r.Invalidated) {}
  bool                        IsInitial() const   { return (Changed|Loaded|Invalidated) == IF_None; }
  void                        Reset()             { Changed = IF_None; Loaded = IF_None; Invalidated = IF_None; }
  void                        Purge(InfoFlags what) { Loaded &= ~what; Changed &= ~what; Invalidated &= ~what; }
};


class Playable;
class Location;
class PlayableSetBase;
class OwnedPlayableSet;
class DependencyInfoSet;
class Job;
class JobSet;
//class DependencyInfoWorker;
/** @brief Common interface of \c Playable objects as well as references to \c Playable objects.
 *
 * Objects of this class may either be reference counted, managed by a \c int_ptr
 * or used as temporaries.
 */
class APlayable
: public Iref_count
{ friend class WaitInfo;
  friend class WaitAggregateInfo;

 protected:
  unsigned                    InUse;
 private:
  /// Event on info change
  event<const PlayableChangeArgs> InfoChange;
  /// Keep track of scheduled asynchronous requests.
  AtomicUnsigned              AsyncRequest;
  #ifdef DEBUG_LOG
  /// Cache DebugName in case of debug builds that call this function frequently.
  /// May be reset in case it is invalid.
  /// @note Strictly speaking this member must be volatile to get strong thread safety.
  /// But since it applies only to debug builds and it changes rarely
  /// a small race condition is left open.
  mutable xstring             DebugNameCache;
  #endif

 protected:
                              APlayable()         : InUse(0) {}
  void                        RaiseInfoChange(const PlayableChangeArgs& args);
  #ifdef DEBUG_LOG
          void                InvalidateDebugName() { DebugNameCache.reset(); }
  #endif
 public:
  virtual                     ~APlayable()        {}
  /// Get't the referenced content.
          Playable&           GetPlayable()       { return (Playable&)DoGetPlayable(); }
  const   Playable&           GetPlayable() const { return DoGetPlayable(); }
  /// Return >0 if the current object is marked as in use.
  /// The value is the depth in the callstack. 1 = Root, 2 = child of root ---
          unsigned            GetInUse() const    { return InUse; }
  /// Mark the object as used (or not)
  virtual void                SetInUse(unsigned used) = 0;
  /// Display name
  /// @return This returns either \c Info.meta.title or the object name of the current URL.
  /// Keep in mind that this may not return the expected value unless \c RequestInfo(IF_Display,...) has been called.
  virtual xstring             GetDisplayName() const = 0;
  /// Create some human readable output for debug logging.
  /// @note This function is aware of being invoked on a \c NULL pointer.
  /// Strictly speaking this is u.b. but in practice it works.
  #ifdef DEBUG_LOG
          xstring             DebugName() const;
  #else
          xstring             DebugName() const   { return (int)this < 0x10000 ? "<null>" : DoDebugName(); } // Hack: allow this == NULL
  #endif

  /// Get start position
  /// @return Pointer to a start \c Location if the object does not start at the beginning of \c this->GetPlayable(),
  /// \c NULL otherwise. The returned \c Location stay valid as long as \c *this is valid.
  /// @remarks The Location is never thread-safe, because all thread-safe instances of \c APLayable
  /// are of type \c Playable and they will always return \c NULL.
  virtual int_ptr<Location>   GetStartLoc() const = 0;
  /// Get stop position
  /// @return Pointer to a stop \c Location if the object ends before the end of \c this->GetPlayable(),
  /// \c NULL otherwise. The returned \c Location stay valid as long as \c *this is valid.
  /// @remarks The Location is never thread-safe, because all thread-safe instances of \c APLayable
  /// are of type \c Playable and they will always return \c NULL.
  virtual int_ptr<Location>   GetStopLoc() const = 0;

  /// Current object is identified as playlist.
  /// @remarks Note that the result is only reliable if IF_Tech has been requested.
          bool                IsPlaylist() const  { return GetInfo().tech->attributes & TATTR_PLAYLIST; }

  /// @brief Request Information
  /// @details Calls DoLoadInfo asynchronously if the requested information is not yet available.
  /// @return The function returns the flags of the requested information that is
  /// \e not immediately available at the requested reliability level.
  /// @details For all the returned bits the information is requested asynchronously.
  /// You should wait for the matching \c InfoChange event to get the requested information.
  /// @note Note that requesting information not necessarily causes a \c Changed flag to be set.
  /// Only the \c Loaded bits are guaranteed.
  /// @note Note that the function may return more bits than requested in case some of the
  /// requested informations depend on other not requested ones. But it may not return \e only
  /// information that is not requested.
  /// @details Of course you have to register the event handler before the call to \c RequestInfo.
  /// If no information is yet available, the function returns \c IF_None.
  /// If \c RequestInfo returned all requested bits the \c InfoChange event is not raised
  /// as result of this call.
  /// @param pri The parameter \a pri schedules the request at a certain priority.
  /// This is useful for prefetching.
  /// If \a pri is \c PRI_Sync the function will not return unless all information is available.
  /// The return value is then always \c IF_None.
  /// If \a pri is \c PRI_None nothing is requested asynchronously. You can use this to check
  /// which information is immediately available.
  /// @param rel The parameter \a rel controls the quality of the requested information.
  /// By default cached information is sufficient and will not cause any further action.
  /// But you may specify \c REL_Confirmed to explicitly ignore cached infos and \c REL_Reload
  /// to explicitly reload some kind of information.
          InfoFlags           RequestInfo(InfoFlags what, Priority pri, Reliability rel = REL_Cached);

  /// @brief Get current Object info.
  /// @details If the required information is not yet loaded, you will get an empty structure.
  /// Call \c RequestInfo to prefetch the information.
  /// Since the returned information is a reference to a potentially mutable structure
  /// you have to synchronize the access by either locking the current instance before the call
  /// or by copying the data inside a critical section unless your read access is implicitly atomic.
  /// However the returned memory reference itself is guaranteed to be valid
  /// as long as the current Playable instance exists. Only the content may change.
  /// So there is no strict need to lock before the GetInfo call.
  virtual const INFO_BUNDLE_CV& GetInfo() const = 0;

  /// @brief Request the AggregateInfo structure.
  /// @details This returns aggregate information on the current collection.
  /// @param excluding All items in the set \a excluding are excluded from the calculation.
  /// The current item is always excluded.
  /// @param what Depending on \a what, only partial information is requested.
  /// If the information is not immediately available and pri is not PRI_None,
  /// the missing infos are requested asynchronously.\n
  /// On output \a what contain only the bits of information that is \e not immediately available.
  /// In case all infos are at the required reliability level what is set to \c IF_None.
  /// @return The method always returns a reference to a AggregateInfo structure.
  /// But the information is only reliable if the corresponding bits in what are set on return.
  /// The returned storage is valid until the current Playable object dies,
  /// but subsequent calls with the same parameters may return a different storage.
  /// @note Calling \c RequestAggregateInfo with an empty set is equivalent to retrieving GetInfo().rpl/.drpl;
  volatile const AggregateInfo& RequestAggregateInfo(const PlayableSetBase& excluding,
                              InfoFlags& what, Priority pri, Reliability rel = REL_Cached);

  /// Invalidate some infos, but do not reload unless this is required.
  /// @param what The kind of information that is to be invalidated.
  /// @return Return the bits in what that really caused an information to be invalidated,
  /// i.e. that have been valid before.
  /// @remarks It might look that you get not the desired result if some consumer has registered
  /// to the invalidate event and requests the information as soon as it has been invalidated.
  virtual InfoFlags           Invalidate(InfoFlags what, const Playable* source = NULL) = 0;

  /// Access the InfoChange event, but only the public part.
  event_pub<const PlayableChangeArgs>& GetInfoChange() { return InfoChange; }

  /// Access to request state for diagnostic purposes (may be slow).
  virtual void                PeekRequest(RequestState& req) const = 0;

  /// Helper function to calculate aggregate information of a slice of the current item.
  /// @param ai Add the the information to \a ai.
  /// @param exclude Exclude the items in this list from the calculation.
  /// The list is modified during the calculation, but restored before completion.
  /// @param what Which information should be updated? Only \c IF_Rpl|IF_Drpl is valid.
  /// @param job Remember asynchronous dependencies in this job. The Job also defines the Priority.
  /// @param start Start location in \c *this. If \c NULL then the start of \c *this is taken.
  /// @param stop Stop location in \c *this. If \c NULL then the end of \c *this is taken.
  /// @param level Take the start and stop locations from depth \a level.
  /// I.e. \a cur is not the root of \a start/stop but at level in the call stack of \a start/stop.
  /// @return Returns the kind of information that is \e not successfully obtained.
  /// If nonzero then some information depends on other objects. The dependencies in \a job have been adjusted.
  /// @remarks Slices are always calculated inclusively, i.e. if the start position is at the first song,
  /// but not \e inside the first song, the first song is part of the slice. The same applies to the stop position.
          InfoFlags           AddSliceAggregate(AggregateInfo& ai, OwnedPlayableSet& exclude, InfoFlags what, Job& job,
                                                const Location* start, const Location* stop, unsigned level = 0);

 private:
  /// @brief Place a request for the kind informations identified by the bit vector \a what
  /// at the priority level \a pri. But only if the current reliability (according to CheckInfo)
  /// is less than \a rel.
  /// @param what DoRequestInfo resets the bits in \a what that already have the requested reliability.
  /// @return The function returns the bits in what that did not cause a no-op with respect to placing the request.
  /// @details The Information will be loaded at the next call to DoLoadInfo with less or same or priority.
  /// If DoRequestInfo is called with \c PRI_None the call should not change anything and
  /// the return value must always be \c IF_None. This is used to check for information availability.
  /// Any implementing class must ensure that all returned bits cause an InfoChange event
  /// with the corresponding \c Loaded bits set sooner or later.
  /// For each bit in what this is a no-op if
  /// - the information already has the requested reliability or
  /// - there is an outstanding request for the same information at higher or same priority.
  /// If there is an outstanding request at a lower priority, this request
  /// is replaced by a request with the given priority.
  /// If an information is very cheap to obtain (non-blocking!), DoRequestInfo may immediately do the job
  /// and reset the corresponding bits in what. This must fire the InfoChange event.
  virtual InfoFlags           DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel) = 0;
  /// This function is called by a worker thread to retrieve the requested kind of information.
  /// The function must either:
  /// - Retrieve the requested infos (intrinsic property of *this).
  /// - Ensure that all required information is retrieved or on the way by another thread.
  /// - If further information on other objects is required to complete, schedule requests
  ///   to other playable objects at the same priority and populate the dependency queue
  ///   with the placed requests. In this case the framework reschedules the request
  ///   as soon as the dependent information is available.
  /// DoLoadInfo must also retrieve context dependent aggregate information that is requested by
  /// DoRequestAI. The same dependency rules apply.
  virtual void                DoLoadInfo(JobSet& job) = 0;
  /// Request a reference to a context dependent aggregate information. The returned reference should
  /// not be dereferenced unless It is passed to DoRequestAI. The returned reference must be valid
  /// until *this dies. However, the content may get outdated.
  //virtual AggregateInfo&      DoAILookup(const PlayableSetBase& exclude) = 0;
  /// DoRequestAI is the context sensitive variant of DoRequestInfo to request aggregate information.
  /// The AggregateInfo reference MUST have been returned by DoAILookup of the same object.
  /// @remarks Note that DoRequestAI may return more bits than requested. It may even return bits
  /// that are not valid for an aggregate info request, if they are required to compute the result.
  virtual InfoFlags           DoRequestAI(const PlayableSetBase& excluding, const volatile AggregateInfo*& ai,
                                          InfoFlags& what, Priority pri, Reliability rel) = 0;

  //virtual InfoFlags           DoGetOutstanding(Priority pri) = 0;
  virtual xstring             DoDebugName() const = 0;

 protected: // Proxy functions to call the above methods also for other objects that derive from APlayable.
  /*static  InfoFlags           CallDoRequestInfo(APlayable& that, InfoFlags& what, Priority pri, Reliability rel)
                              { return that.DoRequestInfo(what, pri, rel); }  
  static  void                CallDoLoadInfo(APlayable& that, JobSet& job)
                              { that.DoLoadInfo(job); }
  static  AggregateInfo&      CallDoAILookup(APlayable& that, const PlayableSetBase& exclude)
                              { return that.DoAILookup(exclude); }  
  static  InfoFlags           CallDoRequestAI(APlayable& that, AggregateInfo& ai, InfoFlags& what, Priority pri, Reliability rel)
                              { return that.DoRequestAI(ai, what, pri, rel); }*/

 private: // Internal dispatcher functions
  virtual const Playable&     DoGetPlayable() const = 0;

 private: // Asynchronous request service
  struct QEntry : public qentry
  { int_ptr<APlayable> Data;
    QEntry(APlayable* pp) : Data(pp) {}
  };
  friend class RescheduleWorker;
  struct WInit
  { Priority Pri;
    volatile int_ptr<APlayable> Current;
    int      TID;
  };
  struct QueueTraverseProxyData
  { bool  (*Action)(APlayable* entry, Priority pri, bool insvc, void* arg);
    void* Arg;
  };
 private:
  static  priority_queue<APlayable::QEntry> WQueue;
  static  size_t              WNumWorkers;  ///< number of workers in the list below (must not be zero)
  static  WInit*              WItems;       ///< List of workers
  static  bool                WTermRq;      ///< Termination Request to Worker
 private:
          void                ScheduleRequest(Priority pri);
          void                HandleRequest(Priority pri);
  static  void                PlayableWorker(WInit& init);
  friend  void TFNENTRY       PlayableWorkerStub(void* arg);
  static  bool                QueueTraverseProxy(const QEntry& entry, size_t priority, void* arg);

 public:
  /// Initialize worker
  static  void                Init();
  /// Destroy worker
  static  void                Uninit();

  /// Inspect worker queue
  /// @param action The \a action function is called once for each queue item.
  /// When it returns false the enumeration is aborted.
  /// @return true if the enumeration completed without abort.
  /// This is done from synchronized context.
  static  bool                QueueTraverse(bool (*action)(APlayable* entry, Priority pri, bool insvc, void* arg), void* arg);
  /// Inspect waiting worker items.
  /// @details The \a action function is called once for each queue item.
  /// This is done from synchronized context.
  static  bool                WaitQueueTraverse(bool (*action)(APlayable& inst, Priority pri, const DependencyInfoSet& depends, void* arg), void* arg);
};

#endif
