/*
 * Copyright 2007-2011 Marcel Mueller
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
class PlayableSlice : public APlayable
{protected:
  enum SliceBorder
  { SB_Start = 1,
    SB_Stop  = -1
  };

  /// Aggregate info cache of \c PlayableSlice in case Start or Stop is not initial.
  struct CICache : public CollectionInfoCache
  { /// Aggregate info without exclusions
    CollectionInfo          DefaultInfo;
    /// Create aggregate info cache for a playable object
    CICache(Playable& p)    : CollectionInfoCache(p), DefaultInfo(PlayableSetBase::Empty, IF_Decoder|IF_Item|IF_Display|IF_Usage) {};
    /// Check whether an Aggregate info structure is owned by this instance.
    /// @remarks The Method is logically const. But the Mutex requires write access.
    bool                    IsMine(const AggregateInfo& ai);
  };

 private:
  enum CalcResult
  { CR_Nop     = 0x00,
    CR_Changed = 0x01,
    CR_Delayed = 0x02
  };

 public:  // Context
  const   int_ptr<APlayable> RefTo;
 protected:
  mutable INFO_BUNDLE_CV    Info;
          ItemInfo          Item;
 private: // Working vars
  // StartCache and StopCache MUST have RefTo->GetPlayable() as root.
  volatile mutable int_ptr<Location> StartCache;
  volatile mutable int_ptr<Location> StopCache;
          sco_ptr<CICache>  CIC;
  class_delegate<PlayableSlice, const PlayableChangeArgs> InfoDeleg;
  //class_delegate<PlayableSlice, const CollectionChangeArgs> CollectionDeleg;

 private:
          void              EnsureCIC()              { if (!CIC) CIC = new CICache(RefTo->GetPlayable()); }
          CalcResult        CalcLoc(const volatile xstring& strloc, volatile int_ptr<Location>& cache, SliceBorder type, JobSet& job);
 protected:
  /// Compare two slice borders taking NULL iterators by default as
  /// at the start, if type is SB_Start, or at the end if type is SB_Stop.
  /// Otherwise the same as SongIterator::CopareTo.
  static  int               CompareSliceBorder(const Location* l, const Location* r, SliceBorder type);
  /// Read-only access \c Item member of another instance.
  static  const INFO_BUNDLE& AccessInfo(const PlayableSlice& ps) { return (INFO_BUNDLE&)ps.Info; }
                            PlayableSlice();
 public:
  explicit                  PlayableSlice(APlayable& pp);
                            PlayableSlice(const PlayableSlice& r);
  virtual                   ~PlayableSlice();
  // Swap instance properties (fast)
  // You must not swap instances that belong to different APlayables.
  // You must own both instances for thread-safety.
  //virtual void              Swap(PlayableSlice& r);

  // Faster, non-virtual version.
          Playable&         GetPlayable() const      { return RefTo->GetPlayable(); }
  /// Display name
  virtual xstring           GetDisplayName() const;
  
  virtual const INFO_BUNDLE_CV& GetInfo() const;

  /// Change usage status.
  virtual void              SetInUse(unsigned used);

  /// Return the overridden information.
  virtual InfoFlags         GetOverridden() const;
  /// Return \c true if the current instance overrides SliceInfo.
          bool              IsItemOverridden() const { return Info.item == &Item; }
  /// Return true if the current instance is different from *RefTo.
          bool              IsSlice() const          { return IsItemOverridden() && (Item.start != NULL || Item.stop != NULL); }
  /// @brief Override the item information.
  /// @detail If the function is called with NULL the overriding is cancelled.
  /// Note that since the start and stop locations may not be verified immediately
  /// because the underlying playlist may not be loaded so far. You can set
  /// any invalid string here. However, the error may come up later, when
  /// IF_Slice is requested.
          void              OverrideItem(const ITEM_INFO* item);

  // This functions are only valid if IF_Slice has already been requested.
  virtual int_ptr<Location> GetStartLoc() const;
  virtual int_ptr<Location> GetStopLoc() const;

  /// Invalidate some infos, but do not reload unless this is required.
  /// @param what The kind of information that is to be invalidated.
  /// @return Return the bits in what that really caused an information to be invalidated,
  /// i.e. that have been valid before.
  /// @remarks It might look that you get not the desired result if some consumer has registered
  /// to the invalidate event and requests the information as soon as it has been invalidated.
  virtual InfoFlags         Invalidate(InfoFlags what);
  /// Access to request state for diagnostic purposes (may be slow).
  virtual void              PeekRequest(RequestState& req) const;

 protected: // Event handler
  virtual void              InfoChangeHandler(const PlayableChangeArgs& args);

 private: // APlayable interface implementations
  virtual InfoFlags         DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel);
  virtual AggregateInfo&    DoAILookup(const PlayableSetBase& exclude);
  virtual InfoFlags         DoRequestAI(AggregateInfo& ai, InfoFlags& what, Priority pri, Reliability rel);
  virtual void              DoLoadInfo(JobSet& job);
  virtual const Playable&   DoGetPlayable() const;
 protected:
  #ifdef DEBUG_LOG
  virtual xstring           DoDebugName() const;
  #endif
};


/** Reference to a Playable object.
 * While the Playable objects are unique per URL the PlayableRef is not.
 * PlayableRef can override the properties Item, Meta and Attr of the underlying APlayable.
 *
 * Calling non-constant methods unsynchronized will not cause the application
 * to have undefined behavior. But it will only cause the PlayableRef to have a
 * non-deterministic but valid state.
 */
class PlayableRef : public PlayableSlice
{private:
          MetaInfo          Meta;
          AttrInfo          Attr;

 public:
  explicit                  PlayableRef(APlayable& refto) : PlayableSlice(refto) { Info.meta = NULL; Info.attr = NULL; }

  // Swap instance properties (fast)
  // You must not swap instances that belong to different PlayableCollection objects.
  // The collection must be locked when calling Swap.
  // Swap does not swap the current status.
  //virtual void             Swap(PlayableRef& r);

  /// Display name
  virtual xstring           GetDisplayName() const;

  virtual const INFO_BUNDLE_CV& GetInfo() const;

  /// Return the overridden information.
  virtual InfoFlags         GetOverridden() const;
  // Return true if the current instance overrides ItemInfo.
          bool              IsMetaOverridden() const { return Info.meta == &Meta; }
          bool              IsAttrOverridden() const { return Info.attr == &Attr; }
  // Override properties
  // Calling the functions with NULL arguments will revoke the overriding.
          void              OverrideMeta(const META_INFO* info);
          void              OverrideAttr(const ATTR_INFO* info);
  // Override all kind of information according to override.
  virtual void              OverrideInfo(const INFO_BUNDLE& info, InfoFlags override);

          void              AssignInstanceProperties(const PlayableRef& src);

 private: // Event handler
  virtual void              InfoChangeHandler(const PlayableChangeArgs& args);
};


inline bool PlayableSlice::CICache::IsMine(const AggregateInfo& ai)
{ if (&ai == &DefaultInfo)
    return true;
  Mutex::Lock lock(CacheMutex);
  return Cache.find(ai.Exclude) == &ai;
}
#endif
