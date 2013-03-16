/*  
 * Copyright 2009-2011 Marcel Mueller
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


#ifndef INFOSTATE_H
#define INFOSTATE_H

#include "infobundle.h"

#include <cpp/event.h>
#include <cpp/atomic.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/queue.h>
#include <cpp/cpputil.h>
#include <cpp/url123.h>


/// Flags to control the kind of information. Mostly used in the asynchronous request interface.
enum InfoFlags // must be aligned to INFOTYPE in format.h!
{ IF_None    = 0x0000U
  // Decoder information
, IF_Phys    = 0x0001U ///< applies to GetInfo().phys
, IF_Tech    = 0x0002U ///< applies to GetInfo().tech
, IF_Obj     = 0x0004U ///< applies to GetInfo().obj
, IF_Meta    = 0x0008U ///< applies to GetInfo().meta
, IF_Attr    = 0x0010U ///< applies to GetInfo().attr and GetAtLoc()
, IF_Child   = 0x0080U ///< applies to GetNext() and GetPre()
, IF_Decoder = IF_Phys|IF_Tech|IF_Obj|IF_Meta|IF_Attr|IF_Child
  // Aggregate information
, IF_Rpl     = 0x0100U ///< applies to GetInfo().rpl and RequestAggregateInfo(...)
, IF_Drpl    = 0x0200U ///< applies to GetInfo().drpl and RequestAggregateInfo(...)
, IF_Aggreg  = IF_Rpl|IF_Drpl
  // Playable Item information
, IF_Item    = 0x0400U ///< applies to GetInfo().item
  // The following flags are calculated content.
, IF_Slice   = 0x1000U ///< applies to GetStartLoc() and GetStopLoc()
, IF_Display = 0x2000U ///< applies to GetDisplayName()
  // The following flags are for events only.
, IF_Usage   = 0x4000U ///< applies to IsInUse() and IsModified()
};
FLAGSATTRIBUTE(InfoFlags);

/// Asynchronous request priority
enum Priority
{ PRI_None      = 0U   ///< Do not execute anything, only check for existence.
, PRI_Low       = 1U   ///< Execute request at idle time.
, PRI_Normal    = 2U   ///< Execute request at normal priority.
, PRI_TrySync   = 4U   ///< Try to execute request synchronously, but in doubt do not wait.
, PRI_Sync      = 8U   ///< Execute synchronously, i.e. wait for the response and do not return without info.
};
FLAGSATTRIBUTE(Priority);

/// @brief Information reliability level
/// @details When used for requests only level 1 and up make sense.
/// When used for information state only up to level 3 makes sense.
enum Reliability
{ REL_Virgin    = 0U  ///< The information is not available
, REL_Invalid   = 1U  ///< The information is loaded, but is likely to be outdated.
, REL_Cached    = 2U  ///< The information could be from a cache that is not well known to be synchronized.
, REL_Confirmed = 3U  ///< The information should be known to reflect the most recent state.
, REL_Reload    = 4U  ///< The information should be reloaded by user request regardless of it's reliability.
};

/** Structure to store result of PeekRequest. */
struct RequestState
{ InfoFlags    ReqLow;
  InfoFlags    ReqHigh;
  InfoFlags    InService;
  RequestState() : ReqLow(IF_None), ReqHigh(IF_None), InService(IF_None) {}
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
  AtomicUnsigned Available;
  AtomicUnsigned Valid;
  AtomicUnsigned Confirmed;
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
  AtomicUnsigned ReqLow;
  AtomicUnsigned ReqHigh;
  AtomicUnsigned InService;
 private: // non-copyable
  InfoState(const InfoState&);
  void operator=(const InfoState&);
 public:
  InfoState(InfoFlags preset = IF_None)       : Available(IF_None), Valid(IF_None), Confirmed(preset),
                                                ReqLow(IF_None), ReqHigh(IF_None), InService(IF_None) {}
  void           Assign(const InfoState& r, InfoFlags mask);
  
 public: // [ Diagnostic interface ]
  /*/// Query the current reliability.
  /// @note This is only reliable in a critical section.
  /// But it may be called from outside for debugging or informational purposes.
  void           PeekReliability(InfoFlags& available, InfoFlags& valid, InfoFlags& confirmed) const;*/
  /// Query the current request state.
  /// @note This is only reliable in a critical section.
  /// But it may be called from outside for debugging or informational purposes.
  void           PeekRequest(RequestState& req) const;
  /*/// Return the flags that have been requested before.
  InfoFlags      HaveBeenRequested(Priority pri) const;*/

 public: // [ Request interface ]
  /// Check which kind of information is NOT at a given reliability level.
  InfoFlags      Check(InfoFlags what, Reliability rel) const;
  /// Place a request and return the bits that were not a no-op (atomic).
  InfoFlags      Request(InfoFlags what, Priority pri);

 public: // [ Worker Interface ]
  /// Return request state for a priority.
  InfoFlags      GetRequest(Priority pri) const;

  /*/// Check what kind of information is currently in service.
  InfoFlags      InfoInService() const        { return (InfoFlags)+InService; }*/
  /// Request informations for update. This sets the appropriate bits in InfoInService.
  /// @return The function returns the bits \e not previously set. The caller must not update
  /// other information than the returned bits.
  /// It is valid to extend an active update by another call to BeginUpdate with additional flags.
  /// But you may not request the same information twice successfully.
  InfoFlags      BeginUpdate(InfoFlags what);
  /// @brief Completes the update requested by BeginUpdate.
  /// @details This implies a state transition for all bits in \a what to \c confirmed.
  /// @param what The parameter what \e must be the return value of \c BeginUpdate from the same thread.
  /// You also may split the \c EndUpdate for different kind of information.
  /// However, you must pass disjunctive flags at all calls.
  /// @return The return value contains the information types that are updated successfully.
  /// If less than what bits are returned some information has been invalidated
  /// during the update process.
  InfoFlags      EndUpdate(InfoFlags what);
  /// Revoke update request but do not revoke an outstanding request.
  /// @return return the Flags that actually have been canceled.
  InfoFlags      CancelUpdate(InfoFlags what) { return (InfoFlags)InService.maskreset(what); }
  
  /// Invalidate some kind of information and return the bits that caused
  /// the information to degrade.
  InfoFlags      Invalidate(InfoFlags what);
  /*/// Invalidate some kind of information and return the invalidated infos,
  /// but only if the specified info is not currently in service.
  /// This function should be called in synchronized context.
  InfoFlags      InvalidateSync(InfoFlags what);*/
  /// Mark information as outdated.
  void           Outdate(InfoFlags what);
  /// Mark information as cached if virgin and return the Cache flags set.
  InfoFlags      Cache(InfoFlags what);
  
 public:
  /// Helper class to address the worker interface.
  class Update
  {private:
    InfoState*   InfoStat;
    InfoFlags    What;
   public:
    /// Create an uninitialized worker. You must call \c Reset() before the worker can be used.
    Update()                                  : InfoStat(NULL), What(IF_None) {}
    /// Create worker on the state manager \a stat. Do not request any updates so far.
    Update(InfoState& stat)                   : InfoStat(&stat), What(IF_None) {}
    /// Create worker on the state manager \a stat for the information requested
    /// at priority \a pri or higher.
    /// Note that you have to check which kind of information you really could lock for update.
    /// If the class is destroyed or reset each update type that is not yet committed is canceled.
    Update(InfoState& stat, Priority pri)     : InfoStat(&stat), What(stat.BeginUpdate(stat.GetRequest(pri))) {}
    /// Cancel all remaining updates automatically.
    ~Update();
    /// Get the information types currently held for update by this instance.
    InfoFlags    GetWhat() const              { return What; }
    operator InfoFlags() const                { return What; }
    /// Try to add an additional information type to update.
    InfoFlags    Extend(InfoFlags what);
    /// Commit and release all currently held information types.
    /// @return Returns the info types that are not invalidated meanwhile.
    InfoFlags    Commit();
    /// Commit and release some of the currently held information types.
    /// @return Returns the info types that are previously held and not invalidated meanwhile.
    InfoFlags    Commit(InfoFlags what);
    /// Cancel all remaining updates.
    void         Rollback();
    /// Cancel some remaining updates.
    void         Rollback(InfoFlags what);
    // Reset a worker to uninitialized state. This implicitly cancels any uncommitted updates.
    //void         Reset()                      { Rollback(); InfoStat = NULL; }
    /// Reset a worker an rebind it to another state mananger \a stat and try
    /// to update all information requested at priority \a pri or higher.
    /// Note that you have to check which kind of information you really could lock for update.
    /// If the class is destroyed or reset each update type that is not yet committed is canceled.
    InfoState    Reset(InfoState& stat, Priority pri);
  };
};


/****************************************************************************
*
*  class InfoState inline implementation
*
****************************************************************************/

/*inline void InfoState::PeekReliability(InfoFlags& available, InfoFlags& valid, InfoFlags& confirmed) const
{ available = (InfoFlags)(Confirmed|Valid|Available);
  valid     = (InfoFlags)(Confirmed|Valid);
  confirmed = (InfoFlags)+Confirmed;
}*/

inline void InfoState::PeekRequest(RequestState& req) const
{ req.ReqLow    |= (InfoFlags)+ReqLow;
  req.ReqHigh   |= (InfoFlags)+ReqHigh;
  req.InService |= (InfoFlags)+InService;
  DEBUGLOG2(("InfoState(%p)::PeekRequest: {%x,%x, %x}\n", this, +ReqLow, +ReqHigh, +InService));
}

/*inline InfoFlags InfoState::HaveBeenRequested(Priority pri) const
{ return (InfoFlags)((pri == PRI_Low ? ReqLow|ReqHigh : +ReqHigh) | Available|Available|Confirmed);
}*/

/*inline bool InfoState::RequestAsync(Priority pri)
{ DEBUGLOG(("InfoStat(%p)::RequestAsync(%u) - %x, %x\n", this, pri, ReqHigh.get(), ReqLow.get()));
  if (pri == PRI_Normal)
    return !ReqHigh.bitset(31);
  else
    // This is not fully atomic because a high priority request may be placed
    // after the mask is applied to what. But this has the only consequence that
    // an unnecessary request is placed in the worker queue. This request causes a no-op.
    return ReqHigh >= 0 && !ReqLow.bitset(31);
}*/

inline InfoFlags InfoState::GetRequest(Priority pri) const
{ DEBUGLOG(("InfoState(%p)::GetRequest(%u) - %x,%x\n", this, pri, +ReqLow, +ReqHigh));
  return (InfoFlags)(pri & PRI_Low ? ReqLow|ReqHigh : +ReqHigh);
}

/*inline void InfoState::ResetAsync(Priority pri)
{ ReqLow.bitrst(31);
  if (pri == PRI_Normal)
    ReqHigh.bitrst(31);
}*/

inline InfoFlags InfoState::BeginUpdate(InfoFlags what)
{ InfoFlags ret = (InfoFlags)InService.maskset(what);
  DEBUGLOG(("InfoState(%p)::BeginUpdate(%x) -> %x\n", this, what, ret));
  return ret;
}


/****************************************************************************
*
*  class InfoState::Update inline implementation
*
****************************************************************************/

inline InfoState::Update::~Update()
{ if (What)
  InfoStat->CancelUpdate(What);
}

inline InfoFlags InfoState::Update::Extend(InfoFlags what)
{ what = InfoStat->BeginUpdate(what);
  What |= what;
  return what;
}

inline InfoFlags InfoState::Update::Commit()
{ InfoFlags ret = InfoStat->EndUpdate(What);
  What = IF_None;
  return ret;
}

inline InfoFlags InfoState::Update::Commit(InfoFlags what)
{ InfoFlags ret = InfoStat->EndUpdate(what & What);
  What &= ~what;
  return ret;
}

inline void InfoState::Update::Rollback()
{ if (What) InfoStat->CancelUpdate(What);
  What = IF_None;
}

inline void InfoState::Update::Rollback(InfoFlags what)
{ InfoStat->CancelUpdate(what & What);
  What &= ~what;
}


#endif
