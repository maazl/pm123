/*  
 * Copyright 2009-2010 Marcel Mueller
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

#include "infobundle.h"

#include <cpp/event.h>
#include <cpp/atomic.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/queue.h>
#include <cpp/cpputil.h>
#include <cpp/url123.h>


enum InfoFlags // must be aligned to INFOTYPE in format.h
{ IF_None    = 0x0000,
  // Decoder information
  IF_Phys    = 0x0001, // applies to GetInfo().phys
  IF_Tech    = 0x0002, // applies to GetInfo().tech
  IF_Obj     = 0x0004, // applies to GetInfo().obj
  IF_Meta    = 0x0008, // applies to GetInfo().meta
  IF_Attr    = 0x0010, // applies to GetInfo().attr and GetAtLoc()
  IF_Child   = 0x0080, // applies to GetCollection()
  IF_Decoder = IF_Phys|IF_Tech|IF_Obj|IF_Meta|IF_Attr|IF_Child,
  // Aggregate information
  IF_Rpl     = 0x0100, // applies to GetInfo().rpl and GetCollectionInfo(...)
  IF_Drpl    = 0x0200, // applies to GetInfo().drpl and GetCollectionInfo(...)
  IF_Aggreg  = IF_Rpl|IF_Drpl,
  // Playable Item information
  IF_Item    = 0x0400, // applies to GetInfo().item
  // The following flags are calculated content.
  IF_Slice   = 0x1000, // applies to GetStartLoc() and GetStopLoc()
  IF_Display = 0x2000, // applies to GetDisplayName()
  // The following flags are for events only.
  IF_Usage   = 0x4000, // applies to IsInUse() and IsModified()
  // INTERNAL USE ONLY
  IF_Async   = 0x80000000 // Queued request on the way
};
FLAGSATTRIBUTE(InfoFlags);


class APlayable;
class PlayableInstance;
/** Parameters for InfoChange Event
 * An initial Event with Changed, Loaded and Invalidated == IF_None is fired
 * just when the playable instance dies. You can check for that with IsInitial().
 * Observers should not access instance after this event completed.
 * They are deregistered automatically.
 * The Loaded flags are set if an asynchronous request is handled. This does not imply
 * that the corresponding Changed bits are set too, because the desired information may not
 * have changed. So if you are waiting for the completion of an asynchronous request
 * use the Loaded bits, and if you want to track changes use the Changed bits.
 * Loaded | Changed | Meaning
 * =======|=========|=================================================================
 *    0   |    0    | Information is untouched since the last InfoChange event.
 *    1   |    0    | Information has been updated but the value did not change.
 *    0   |    1    | Cached information has been loaded.
 *    1   |    1    | Information has been updated and changed.
 * If the bit IF_Child is set in the Changed vector the event is in fact of the derived type
 * CollectionChangeArgs. */
struct PlayableChangeArgs
{ APlayable&                  Instance;   // Related Playable object.
  APlayable*                  Origin;     // Points to an object that caused that initiated the events, optional.
  InfoFlags                   Changed;    // Bit vector with changed information.
  InfoFlags                   Loaded;     // Bit vector with loaded information.
  InfoFlags                   Invalidated;// Bit vector with invalidated information.
  PlayableChangeArgs(APlayable& inst, APlayable* orig, InfoFlags changed, InfoFlags loaded, InfoFlags invalidated)
                                                  : Instance(inst), Origin(orig), Changed(changed), Loaded(loaded),  Invalidated(invalidated) {}
  PlayableChangeArgs(APlayable& inst)             : Instance(inst), Origin(&inst), Changed(IF_None), Loaded(IF_None), Invalidated(IF_None) {}
  bool                        IsInitial() const   { return (Changed|Loaded|Invalidated) == IF_None; }
  void                        Reset()             { Changed = IF_None; Loaded = IF_None; Invalidated = IF_None; }
};

enum CollectionChangeType
{ PCT_All,                 // The collection has entirely changed
  PCT_Insert,              // Item just inserted
  PCT_Move,                // Item has just moved
  PCT_Delete               // Item about to be deleted
};

struct CollectionChangeArgs : public PlayableChangeArgs
{ CollectionChangeType        Type;
  PlayableInstance*           Before;
  CollectionChangeArgs(APlayable& inst)
  : PlayableChangeArgs(inst) {}
  /*CollectionChangeArgs(PlayableCollection& coll, PlayableInstance& item, CollectionChangeType type)
  : Collection(coll), Item(item), Type(type) {}*/
};

/// Asynchronous request priority
enum Priority
{ PRI_None      = 0,        // Do not execute anything, only check for existance. 
  PRI_Low       = 1,        // Execute request at idle time.
  PRI_Normal    = 2,        // Execute request.
  PRI_Sync      = 3,        // Execute request synchronously, i.e. wait for the response.
};

/// @brief Information reliability level
/// @details When used for requests only level 1 and up make sense.
/// When used for information state only up to level 3 makes sense.
enum Reliability
{ REL_Virgin    = 0,        // The information is not available
  REL_Invalid   = 1,        // The information is loaded, but is likely to be outdated.
  REL_Cached    = 2,        // The information could be from a cache that is not well known to be synchronized.
  REL_Confirmed = 3,        // The information should be known to reflect the most recent state.
  REL_Reload    = 4,        // The information should be reloaded by user request regardless of it's reliability.
};


/** @brief Info state machine.
 *
 * This state machine is thread-safe and lock-free. */
class InfoState
{private:
  // The Available, Valid and Confirmed bit voctors defines the reliability status
  // of the certain information components. See InfoFlags for the kind of information.
  //  avail.| valid |confirm.| reliab. | meaning
  // =======|=======|========|=========|==================================================
  //    0   |   0   |    0   | Virgin  | The information is not available.
  //    1   |   0   |    0   | Invalid | The information was available and is invalidated.
  //    x   |   1   |    0   | Cached  | The information is available but may be outdated.
  //    x   |   x   |    1   | Confirm.| The information is confirmed.
  AtomicUnsigned              Available;
  AtomicUnsigned              Valid;
  AtomicUnsigned              Confirmed;
  // The Fields InfoRequest, InfoRequestLow and InfoInService define the request status of the information.
  // The following bit combinations are valid for each kind of information.
  //  low | high |in svc.| meaning
  // =====|======|=======|============================================================================
  //   0  |   0  |   0   | The Information is stable.
  //   1  |   0  |   0   | The Information is requested asynchronously at low priority.
  //   x  |   1  |   0   | The Information is requested asynchronously at high priority.
  //   0  |   0  |   1   | The Information is in service without a request (e.g. because it is cheap).
  //   1  |   0  |   1   | The Information is in service because of a low priority request.
  //   x  |   1  |   1   | The Information is in service because of a high priority request.
  AtomicUnsigned              ReqLow;
  AtomicUnsigned              ReqHigh;
  AtomicUnsigned              InService;
 private: // non-copyable
  InfoState(const InfoState&);
  void operator=(const InfoState&);
 public:
  InfoState(InfoFlags preset = IF_None)
                              : Available(IF_None), Valid(IF_None), Confirmed(preset),
                                ReqLow(IF_None), ReqHigh(IF_None), InService(IF_None) {}
  void                        Assign(const InfoState& r, InfoFlags mask);
  
 public: // [ Diagnostic interface ]
  /// Query the current reliability.
  /// @note This is only reliable in a critical section.
  /// But it may be called from outside for debugging or informational purposes.
  void                        PeekReliability(InfoFlags& available, InfoFlags& valid, InfoFlags& confirmed) const
                              { available = (InfoFlags)(Confirmed|Valid|Available);
                                valid     = (InfoFlags)(Confirmed|Valid);
                                confirmed = (InfoFlags)+Confirmed;
                              }
  /// Query the current request state.
  /// @note This is only reliable in a critical section.
  /// But it may be called from outside for debugging or informational purposes.
  void                        PeekRequest(InfoFlags& low, InfoFlags& high, InfoFlags& inservice) const
                              { low       = (InfoFlags)+ReqLow;
                                high      = (InfoFlags)+ReqHigh;
                                inservice = (InfoFlags)+InService;
                              }

 public: // [ Request interface ]
  /// Check which kind of information is NOT at a given reliability level.
  InfoFlags                   Check(InfoFlags what, Reliability rel) const;
  /// Place a request and return the bits that were not a no-op (atomic).
  InfoFlags                   Request(InfoFlags what, Priority pri);

  bool                        RequestAsync(Priority pri)
                              { DEBUGLOG(("InfoStat(%p)::RequestAsync(%u) - %x, %x\n", this, pri, ReqHigh.get(), ReqLow.get()));
                                if (pri == PRI_Normal)
                                  return !ReqHigh.bitset(31);
                                else
                                  // This is not fully atomic because a high priority request may be placed
                                  // after the mask is applied to what. But this has the only consequence that
                                  // an unnecessary request is placed in the worker queue. This request causes a no-op.
                                  return ReqHigh >= 0 && !ReqLow.bitset(31);
                              }

 public: // [ Worker Interface ]
  /// Return request state for a priority.
  InfoFlags                   GetRequest(Priority pri) const
                              { DEBUGLOG(("InfoState(%p)::GetRequest(%u) - %x,%x\n", this, pri, +ReqLow, +ReqHigh));
                                return (InfoFlags)(pri == PRI_Low ? ReqLow|ReqHigh : +ReqHigh);
                              }

  void                        ResetAsync(Priority pri)
                              { ReqLow.bitrst(31);
                                if (pri == PRI_Normal)
                                  ReqHigh.bitrst(31);
                              }
  /// Check what kind of information is currently in service.
  InfoFlags                   InfoInService() const
                              { return (InfoFlags)+InService; }
  /// Request informations for update. This sets the appropriate bits in InfoInService.
  /// @return The function returns the bits \e not previously set. The caller must not update
  /// other information than the returned bits.
  /// It is valid to extend an active update by another call to BeginUpdate with additional flags.
  InfoFlags                   BeginUpdate(InfoFlags what)
                              { InfoFlags ret = (InfoFlags)InService.maskset(what);
                                DEBUGLOG(("InfoState(%p)::BeginUpdate(%x) -> %x\n", this, what, ret));
                                return ret;
                              }
  /// @brief Completes the update requested by BeginUpdate.
  /// @details This implies a state transition for all bits in \a what to \c confirmed.
  /// @param what The parameter what \e must be the return value of \c BeginUpdate from the same thread.
  /// You also may split the \c EndUpdate for different kind of information.
  /// However, you must pass disjunctive flags at all calls.
  /// @return The return value contains the information types that are updated successfully.
  /// If less than what bits are returned some information has been invalidated
  /// during the update process.
  InfoFlags                   EndUpdate(InfoFlags what);
  /// Revoke update request but do not revoke an outstanding request.
  InfoFlags                   CancelUpdate(InfoFlags what)
                              { return (InfoFlags)InService.maskreset(what); }
  
  /// Invalidate some kind of information and return the bits that caused
  /// the information to degrade.
  InfoFlags                   Invalidate(InfoFlags what);
  /// Invalidate some kind of information and return the invalidated infos,
  /// but only if the specified info is not currently in service.
  /// This function should be called in synchronized context.
  InfoFlags                   InvalidateSync(InfoFlags what);
  /// Mark information as outdated.
  void                        Outdate(InfoFlags what);
  /// Mark information as cached if virgin and return the Cache flags set.
  InfoFlags                   Cache(InfoFlags what);
  
 public:
  /// Helper class to address the worker interface.
  class Update
  {private:
    InfoState&                InfoStat;
    InfoFlags                 What;
   public:
    /// Create worker on the state manager \p stat for the information type what.
    /// Note that you have to check which kind of information you really could lock for update.
    /// If the class is destroyed each update type that is not yet committed is canceled.
    Update(InfoState& stat, InfoFlags what)
                              : InfoStat(stat), What(InfoStat.BeginUpdate(what)) {}
    /// Cancel all remaining updates automatically.
    ~Update()                 { if (What) InfoStat.CancelUpdate(What); }
    /// Get the information types currently held for update by this instance.
    InfoFlags                 GetWhat() const { return What; }
    operator InfoFlags() const { return What; }
    /// Try to add an additional information type to update.
    InfoFlags                 Extend(InfoFlags what)
                              { what = InfoStat.BeginUpdate(what); What |= what; return what; }
    /// Commit and release all currently held information types.
    /// @return Returns the info types that are not invalidated meanwhile.
    InfoFlags                 Commit()
                              { InfoFlags ret = InfoStat.EndUpdate(What); What = IF_None; return ret; }
    /// Commit and release some of the currently held information types.
    /// @return Returns the info types that are previously held and not invalidated meanwhile.
    InfoFlags                 Commit(InfoFlags what)
                              { InfoFlags ret = InfoStat.EndUpdate(what & What); What &= ~what; return ret; }
    /// Cancel all remaining updates.
    void                      Rollback()
                              { if (What) InfoStat.CancelUpdate(What); What = IF_None; }
    /// Cancel some remaining updates.
    void                      Rollback(InfoFlags what)
                              { InfoStat.CancelUpdate(what & What); What &= ~what; }
  };
};


class Playable;
class Location;
class PlayableSetBase;
/** @brief Common interface of \c Playable objects as well as references to \c Playable objects.
 *
 * Objects of this class may either be reference counted, managed by a \c int_ptr
 * or used as temporaries.
 */
class APlayable : public Iref_count
{public:
  /// Class to wait for a desired information.
  /// This is something like a conditional variable.
  class WaitInfo
  {private:
    InfoFlags                 Filter;
    Event                     EventSem;
    class_delegate<WaitInfo, const PlayableChangeArgs> Deleg;
   private:
    void                      InfoChangeEvent(const PlayableChangeArgs& args);
   public:
    /// @brief Create a \c WaitInfo Semaphore that is posted once all information specified by Filter
    /// is loaded.
    /// @details To be safe the constructor of this class must be invoked either while the mutex
    /// of the object is held or before the asynchronous request. And after the request
    /// the available information should be removed from the request by calling \c CommitInfo.
                              WaitInfo(APlayable& inst, InfoFlags filter);
                              ~WaitInfo()         { DEBUGLOG(("APlayable::WaitInfo(%p)::~WaitInfo()\n", this)); }
    /// Notify the class instance about informations that is now available.
    /// This kind of information is removed from the wait condition.
    void                      CommitInfo(InfoFlags what);
    /// Wait until all requested information is loaded or an error occurs.
    /// the function returns false if the given time elapsed or the Playable
    /// object died.
    bool                      Wait(long ms = -1)  { EventSem.Wait(ms); return Filter == 0; }
  };
 protected:
  /// Internal base class for collection info cache entries.
  struct CollectionInfo
  : public AggregateInfo
  { InfoState                 InfoStat;
    CollectionInfo(const PlayableSetBase& exclude) : AggregateInfo(exclude) {}
  };
  // Return value of DoRequestInfo
  enum ReqInfoResult
  { RIR_NoRequest,            // The request did not place any asynchronous requests.
    RIR_AsyncRequest,         // The request placed asynchronous requests, but there are already other async. requests outstanding at the requested priority level.
    RIR_FirstRequest          // The request placed asynchronous requests, and this is the first request at the priority level.
  };

 protected:
  /// Information request state
  InfoState                   InfoStat;
  /// Event on info change
  event<const PlayableChangeArgs> InfoChange;

 public:
                              APlayable()         : InfoStat(IF_Usage|IF_Display) {}
  virtual                     ~APlayable() {}
  /// Get't the referenced content.
          Playable&           GetPlayable()       { return (Playable&)DoGetPlayable(); }
  const   Playable&           GetPlayable() const { return DoGetPlayable(); }
  /// Return true if the current object is marked as in use.
  virtual bool                IsInUse() const = 0;
  /// Mark the object as used (or not)
  virtual void                SetInUse(bool used) = 0;
  /// Display name
  /// @return This returns either \c Info.meta.title or the object name of the current URL.
  /// Keep in mind that this may not return the expected value unless \c RequestInfo(IF_Meta,...) has been called.
  virtual xstring             GetDisplayName() const = 0;
  /// Get start position
  virtual int_ptr<Location>   GetStartLoc() const = 0;
  /// Get stop position
  virtual int_ptr<Location>   GetStopLoc() const = 0;

  /// Current object is identified as playlist.
          bool                IsPlaylist() const  { return GetInfo().tech->attributes & TATTR_PLAYLIST; }

  /*// Check if this instance (still) belongs to a certain collection.
  // The return value true is not reliable unless the collection is locked.
  // Calling this method with NULL will check whether the instance does no longer belog to a collection.
  // In this case only the return value true is reliable.
  // The default implementation will always return false.
  virtual bool                HasParent(const APlayable* parent) const;
  // Returns the index within the current collection counting from 1.
  // A return value of 0 indicates the the instance currently does not belong to a collection.
  // A return value of -1 indicates that this is no Item of a collection.
  // The return value is only reliable while the collection is locked.
  // But comparing indices from the same collection is always reliable while taking the difference is not.
  // The default implementation always returns -1.
  virtual int                 GetIndex() const;*/

  /// @brief Request Information
  /// @details Calls DoLoadInfo asynchronously if the requested information is not yet available.
  /// @return The function returns the flags of the requested information that is
  /// \e not immediately available at the requested reliability level.
  /// @details For all the returned bits the information is requested asynchronously.
  /// You should wait for the matching \c InfoChange event to get the requested information.
  /// @note Note that requesting information not necessarily causes a Changed flag to be set.
  /// Only the Loaded bits are guaranteed.
  /// @note Note that the function may return more bits than requested in case some of the
  /// requested informations depend on other not requested ones.
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
  /// But you may specify \c REL_Confirmed to explicitly ignore cached infos und \c REL_Reload
  /// to explicitly reload some kind of information.
  virtual InfoFlags           RequestInfo(InfoFlags what, Priority pri, Reliability rel = REL_Cached);

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
  /// the missing infos are requested asynchronously.
  /// On output \a what contain only the bits of information that is requested asynchronously.
  /// @return The method always returns a reference to a AggregateInfo structure.
  /// But the information is only reliable if the corresponding bits in what are set on return.
  /// The returned storage is valid until the current Playable object dies,
  /// but subsequent calls with the same parameters may return a different storage.
  /// @note Calling RequestAggregateInfo with an empty set is equivalent to retrieving GetInfo().rpl/.drpl;
  virtual volatile const AggregateInfo& RequestAggregateInfo(const PlayableSetBase& excluding,
                              InfoFlags& what, Priority pri, Reliability rel = REL_Cached);

  /// Return the currently valid types of information.
          InfoFlags         GetValid(Reliability rel = REL_Cached) { return ~RequestInfo(~IF_None, PRI_None, rel); }
  /// Return the overridden information.
  virtual InfoFlags         GetOverridden() const = 0;

  /// Access the InfoChange event, but only the public part.
  event_pub<const PlayableChangeArgs>& GetInfoChange() { return InfoChange; }
  /// Access to InfoState for diagnostic purposes
  const InfoState&          GetInfoState() const  { return InfoStat; }

 private:
  /// @brief Place a request for the kind informations identified by the bit vector \a what
  /// at the priority level \a pri. But only if the current reliability (according to CheckInfo)
  /// is less than \a rel.
  /// @details The Information will be loaded at the next call to DoLoadInfo with less or same or priority.
  /// @param what DoRequestInfo resets the bits in \a what that already have the requested reliability.
  /// @return The function returns the bits in what that did not cause a no-op with respect to placing the request.
  ///
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
  /// - Retrieve the requested infos (intrinsic property of *this) and return false.
  /// - If all required information is retrieved or on the way by another thread return false.
  /// - If further information on other objects is required to complete, schedule requests
  ///   to other playable objects at the same priority and return true.
  ///   In this case the framework reschedules the request later until DoLoadInfo returns false.
  /// DoLoadInfo must also retrieve context dependent aggregate information that is requested by
  /// DoRequestAI.
  virtual bool                DoLoadInfo(Priority pri) = 0;
  /// Request a reference to a context dependent aggregate information. The returned reference should
  /// not be dereferenced unless It is passed to DoRequestAI. The returned reference must be valid
  /// until *this dies. However, the content may get outdated.
  virtual AggregateInfo&      DoAILookup(const PlayableSetBase& exclude) = 0;
  /// DoRequestAI is the context sensitive variant of DoRequestInfo to request aggregate information.
  /// The AggregateInfo reference MUST have been returned by DoAILookup of the same object.
  virtual InfoFlags           DoRequestAI(AggregateInfo& ai, InfoFlags& what, Priority pri, Reliability rel) = 0;

 protected: // Proxy functions to call the above methods also for other objects that derive from APlayable.
  static  InfoFlags           CallDoRequestInfo(APlayable& that, InfoFlags& what, Priority pri, Reliability rel)
                              { return that.DoRequestInfo(what, pri, rel); }  
  static  bool                CallDoLoadInfo(APlayable& that, Priority pri)
                              { return that.DoLoadInfo(pri); }  
  static  AggregateInfo&      CallDoAILookup(APlayable& that, const PlayableSetBase& exclude)
                              { return that.DoAILookup(exclude); }  
  static  InfoFlags           CallDoRequestAI(APlayable& that, AggregateInfo& ai, InfoFlags what, Priority pri, Reliability rel)
                              { return that.DoRequestAI(ai, what, pri, rel); }  

 private: // Internal dispatcher functions
  virtual const Playable&     DoGetPlayable() const = 0;

 private: // Asynchronous request service
  struct QEntry : public qentry
  { int_ptr<APlayable> Data;
    QEntry(APlayable* pp) : Data(pp) {}
  };
  struct WInit
  { Priority Pri;
    volatile int_ptr<APlayable> Current;
    int      TID;
  };
  static  priority_queue<APlayable::QEntry> WQueue;
  static  size_t              WNumWorkers;  // number of workers in the above list
  static  size_t              WNumDlgWorkers;// number of workers in the above list
  static  WInit*              WItems;       // List of workers
  static  bool                WTermRq;      // Termination Request to Worker
  struct QueueTraverseProxyData
  { void  (*Action)(APlayable* entry, Priority pri, bool insvc, void* arg);
    void* Arg;
  };
  static  void                QueueTraverseProxy(const QEntry& entry, size_t priority, void* arg);
 private:
  static  void                PlayableWorker(WInit& init);
  friend  void TFNENTRY       PlayableWorkerStub(void* arg);

 public:
  /// Initialize worker
  static  void                Init();
  /// Destroy worker
  static  void                Uninit();

  /// @brief Inspect worker queue
  /// @details The \a action function is called once for each queue item. But be careful,
  /// this is done from synchronized context.
  static  void                QueueTraverse(void (*action)(APlayable* entry, Priority pri, bool insvc, void* arg), void* arg);
};


#endif
