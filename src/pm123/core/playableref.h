/*
 * Copyright 2007-2012 Marcel Mueller
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


#ifndef PLAYABLEREF_H
#define PLAYABLEREF_H

#include "aplayable.h"
#include "collectioninfocache.h"
#include "location.h"
#include <cpp/atomic.h>
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/cpputil.h>


class PlayableSetBase;
class OwnedPlayableSet;

struct CollectionChangeArgs;
/** PlayableSlice := Playable object together with a start and stop position.
 * Objects of this class may either be reference counted, managed by a int_ptr
 * or used as temporaries.
 * Only const functions of this class are thread-safe.
 */
class PlayableRef : public APlayable
{protected:
  /// Aggregate info cache of \c PlayableSlice in case Start or Stop is not initial.
  struct CICache : public CollectionInfoCache
  { /// Aggregate info without exclusions
    CollectionInfo          DefaultInfo;
    /// Create aggregate info cache for a playable object
    CICache(Playable& p)    : CollectionInfoCache(p), DefaultInfo(PlayableSetBase::Empty, IF_Decoder|IF_Item|IF_Display|IF_Usage) {};
    /// Check whether an Aggregate info structure is owned by this instance.
    /// @remarks The Method is logically const. But the Mutex requires write access.
    //bool                    IsMine(const AggregateInfo& ai);
  };

 public:  // Context
  const   int_ptr<APlayable> RefTo;
 protected:
          AtomicUnsigned    Overridden;
  mutable INFO_BUNDLE_CV    Info;
          sco_ptr<ItemInfo> Item;
          sco_ptr<MetaInfo> Meta;
          sco_ptr<AttrInfo> Attr;
          sco_ptr<CICache>  CIC;
 private: // Working vars
  // StartCache and StopCache MUST have RefTo->GetPlayable() as root.
  volatile mutable int_ptr<Location> StartCache;
  volatile mutable int_ptr<Location> StopCache;
  class_delegate<PlayableRef, const PlayableChangeArgs> InfoDeleg;

 private:
          void              EnsureCIC()              { if (!CIC) CIC = new CICache(RefTo->GetPlayable()); }
  /// @brief Evaluate location string.
  /// @param strloc String to evaluate.
  /// @param cache Target.
  /// @param type Options, @see CompareOptions.
  /// @param job Place dependencies here.
  /// @return \c true if the evaluation changed \a cache.
          bool              CalcLoc(const volatile xstring& strloc, volatile int_ptr<Location>& cache, Location::CompareOptions type, JobSet& job);
 protected:
  /// Compare two slice borders taking NULL iterators by default as
  /// at the start or the end, depending on \c Location::CO_Reverse.
  /// Otherwise the same as \c Location::CopareTo.
  static  int               CompareSliceBorder(const Location* l, const Location* r, Location::CompareOptions type);
                            PlayableRef();
 public:
  explicit                  PlayableRef(APlayable& pp);
                            PlayableRef(const PlayableRef& r);
  virtual                   ~PlayableRef();
  // Faster, non-virtual version.
          Playable&         GetPlayable() const      { return RefTo->GetPlayable(); }
  /// Display name
  virtual xstring           GetDisplayName() const;
  
  virtual const INFO_BUNDLE_CV& GetInfo() const;

  /// Change usage status.
  virtual void              SetInUse(unsigned used);

  /// Return the overridden information.
          InfoFlags         GetOverridden() const    { return (InfoFlags)Overridden.get(); }
  /// Return true if the current instance is different from *RefTo.
          bool              IsSlice() const          { return (Overridden & IF_Item) && (Item->start || Item->stop); }
  /// @brief Override the item information.
  /// @param item New item information. If \c NULL then the overriding is canceled.
  /// @remarks Note that since the start and stop locations may not be verified immediately
  /// because the underlying playlist may not be loaded so far. You can set
  /// any invalid string here. However, the error may come up later, when
  /// \c IF_Slice is requested.
          void              OverrideItem(const ITEM_INFO* item);
  /// Override the meta information.
  /// @param meta New meta information. If \c NULL the the overriding is canceled.
  /// @remarks The overriding is \e not canceled if the information is overridden by identical values.
          void              OverrideMeta(const META_INFO* info);
  /// Override the attribute information.
  /// @param attr New attribute information. If \c NULL the the overriding is canceled.
  /// @remarks The overriding is \e not canceled if the information is overridden by identical values.
          void              OverrideAttr(const ATTR_INFO* info);

          void              AssignInstanceProperties(const PlayableRef& src);

  // This functions are only valid if IF_Slice has already been requested.
  virtual int_ptr<Location> GetStartLoc() const;
  virtual int_ptr<Location> GetStopLoc() const;

  /// Invalidate some infos, but do not reload unless this is required.
  /// @param what The kind of information that is to be invalidated.
  /// @return Return the bits in what that really caused an information to be invalidated,
  /// i.e. that have been valid before.
  /// @remarks It might look that you get not the desired result if some consumer has registered
  /// to the invalidate event and requests the information as soon as it has been invalidated.
  virtual InfoFlags         Invalidate(InfoFlags what, const Playable* source = NULL);
  /// Access to request state for diagnostic purposes (may be slow).
  virtual void              PeekRequest(RequestState& req) const;

 protected: // Event handler
  virtual void              InfoChangeHandler(const PlayableChangeArgs& args);

 private: // APlayable interface implementations
  virtual InfoFlags         DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel);
  //virtual AggregateInfo&    DoAILookup(const PlayableSetBase& exclude);
  virtual InfoFlags         DoRequestAI(const PlayableSetBase& exclude, const volatile AggregateInfo*& aip,
                                        InfoFlags& what, Priority pri, Reliability rel);
  virtual void              DoLoadInfo(JobSet& job);
  virtual const Playable&   DoGetPlayable() const;
  virtual xstring           DoDebugName() const;
};


/*inline bool PlayableRef::CICache::IsMine(const AggregateInfo& ai)
{ if (&ai == &DefaultInfo)
    return true;
  Mutex::Lock lock(CacheMutex);
  return Cache.find(ai.Exclude) == &ai;
}*/
#endif
