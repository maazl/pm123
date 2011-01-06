/*
 * Copyright 2007-2009 Marcel Mueller
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
#include "playableset.h"
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <cpp/xstring.h>
#include <cpp/cpputil.h>


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

  // Structure to hold a CollectionInfoCache entry.
  struct CollectionInfoEntry
  : public PlayableSet,
    public CollectionInfo
  { explicit CollectionInfoEntry(const PlayableSetBase& key) : PlayableSet(key), CollectionInfo((PlayableSet&)*this) {};
    explicit CollectionInfoEntry(PlayableSet& key) : PlayableSet(), CollectionInfo((PlayableSet&)*this) { swap(key); };
    void                     SetRevision(unsigned rev) { Revision = rev; }
  };
  typedef sorted_vector_own<CollectionInfoEntry, PlayableSetBase> CollectionInfoList;
 private:
  enum CalcResult
  { CR_Nop     = 0x00,
    CR_Changed = 0x01,
    CR_Delayed = 0x02
  };

 public:  // Context
  const   int_ptr<APlayable> RefTo;
 protected:
  mutable INFO_BUNDLE_CV     Info;
          ItemInfo           Item;
 private:
  static  Mutex              CollectionInfoMutex;
          CollectionInfoList CollectionInfoCache;
 private: // Working vars
  // StartCache and StopCache MUST have RefTo->GetPlayable() as root.
  volatile int_ptr<Location> StartCache;  // mutable
  volatile int_ptr<Location> StopCache;   // mutable
  class_delegate<PlayableSlice, const PlayableChangeArgs> InfoDeleg;
  //class_delegate<PlayableSlice, const CollectionChangeArgs> CollectionDeleg;

 private:
          CalcResult        CalcLoc(const volatile xstring& strloc, volatile int_ptr<Location>& cache, SliceBorder type, Priority pri);
          InfoFlags         InvalidateCIC(InfoFlags what, const Playable* pp);
  /// (Re)calculate the the information what in \a cie.
  /// @return Returns true if the execution depends on open requests to other objects.
          bool              CalcRplInfo(CollectionInfoEntry& cie, InfoFlags what, Priority pri, PlayableChangeArgs& event);
          bool              CalcRplCore(AggregateInfo& ai, APlayable& cur, OwnedPlayableSet& exclude, InfoFlags what, Priority pri, const Location* start, const Location* stop, size_t level);
  /// Adds the slice [start,stop) of song to \a ai. If either of them is < 0 the beginning respectively the end of the song is used.
          bool              AddSong(AggregateInfo& ai, APlayable& song, InfoFlags what, Priority pri, PM123_TIME start, PM123_TIME stop);
 protected:
  // Advances item to the next CollectionInfoCache entry that needs some work to be done
  // at the given priority level. The function returns the requested work flags.
  // If item is NULL GetNextCIWorkItem returns moves to the first item.
  // If there is no more work, item is set to NULL and the return value is IF_None.
  // (Either of them is sufficient.) 
          InfoFlags         GetNextCIWorkItem(CollectionInfoEntry*& item, Priority pri);

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
          Playable&         GetPlayable() const { return RefTo->GetPlayable(); }
  // Display name, forwarder to RefTo
  virtual xstring           GetDisplayName() const;
  
  virtual const INFO_BUNDLE_CV& GetInfo() const;

  // Usage status
  virtual bool              IsInUse() const;
  virtual void              SetInUse(bool used);

  /// Return the overridden information.
  virtual InfoFlags         GetOverridden() const;
  /// Return \c true if the current instance overrides SliceInfo.
          bool              IsItemOverridden() const { return Info.item == &Item; }
  // Return true if the current instance is different from *RefTo.
          bool              IsSlice() const     { return IsItemOverridden() && (Item.start != NULL || Item.stop != NULL); }
  // Override the item information.
  // If the function is called with NULL the overriding is cancelled.
  // Note that since the start and stop locations may not be verified immediately
  // because the underlying playlist may not be loaded so far. You can set
  // any invalid string here. However, the error may come up later, when
  // IF_Slice is requested.
          void              OverrideItem(const ITEM_INFO* item);

  // This functions are only valid if IF_Slice has already been requested.
  virtual int_ptr<Location> GetStartLoc() const;
  virtual int_ptr<Location> GetStopLoc() const;

  #ifdef DEBUG_LOG
          xstring           DebugName() const;
  #endif

 protected: // Event handler
  virtual void              InfoChangeHandler(const PlayableChangeArgs& args);

 private: // APlayable interface implementations
  virtual InfoFlags         DoRequestInfo(InfoFlags& what, Priority pri, Reliability rel);
  virtual AggregateInfo&    DoAILookup(const PlayableSetBase& exclude);
  virtual InfoFlags         DoRequestAI(AggregateInfo& ai, InfoFlags& what, Priority pri, Reliability rel);
  virtual bool              DoLoadInfo(Priority pri);
  virtual const Playable&   DoGetPlayable() const;
};


/** Reference to a Playable object. While the Playable objects are unique per URL
 * the PlayableRef is not. PlayableRef can override the properties Item, Meta and Attr
 * of the underlying APlayable.
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

/*class PlayableRef : public APlayable
{private:
  enum RPLState
  { RPL_Invalid,   // The RPL information is currently not valid
    RPL_Request,   // The RPL information is requested asynchronuously
    RPL_InService, // The RPL information is in service
    RPL_Valid      // The RPL information is recent
  };
 private: // Context
  // This Pointer is valid as long as the PlayableRef exists.
  const int_ptr<Playable>  RefTo;
 private: // Properties
  bool                     InUse;
  xstring                  Start;
  xstring                  Stop;
  xstring                  Alias;
  InfoFlags                Overridden;
  InfoBundle               Info;
 private: // Internal state
  // The fields InfoChange and InfoLoaded hold the flags for the next InfoChange event.
  // They are collected until OnReleaseMutex is called which raises the event.
  // Valid combinations of bits for each kind of information.
  // Loaded | Changed | Meaning
  // =======|=========|=================================================================
  //    0   |    0    | Information is untouched since the last InfoChange event.
  //    1   |    0    | Information has been updated but the value did not change.
  //    0   |    1    | INVALID! - InfoLoaded never contains less bits than InfoChanged.
  //    1   |    1    | Information has been updated and changed. 
  InfoFlags                InfoChanged; // Bitvector with stored events
  InfoFlags                InfoLoaded;  // Bitvector with recently updated information
  // RPL state machine:
  // current # call    | begin   | end     | change  |
  // state   # ReqInfo | DoLoad  | DoLoad  | event   |
  // ========#=========|=========|=========|=========|
  // invalid > request | invalid | valid   | invalid |
  // request > request | in svc* | request | request | * execute CalcRplInfo
  // in svc  > request | in svc  | valid   | invalid |
  // valid   > valid*  | valid   | valid   | invalid | * fire InfoChange
  RPLState                 RplState;    // Recursive playlist information state
 private: // Working vars
  // Start and Stop are owned by PlayableRef. But since SongIterator is not yet a complete type
  // we have to deal with that manually. Furthermore Start and Stop MUST have RefTo as root,
  // but they must not use *this as root to avoid cyclic references.
  SongIterator*            StartCache; // mutable
  SongIterator*            StopCache;  // mutable

  class_delegate<PlayableRef, const PlayableChangeArgs> InfoDeleg;
  class_delegate<PlayableRef, const CollectionChangeArgs> CollectionDeleg;

 private:
  static bool              OverrideCore(void* dstptr, const void* ref, void* shadow, const void* src);
  void                     OverrideUpdate(InfoFlags flg, bool change, bool set);
  // Raise InfoChange event
  void                     RaiseInfoChange(InfoFlags mask = ~IF_None);
  void                     AdjustOverrideRpl();
  bool                     CalcRplInfo();
 public:
  PlayableRef(Playable* pp);
  PlayableRef(const PlayableRef& r);
  //PlayableRef(const url123& url, const xstring& alias = xstring());
  //PlayableRef(const url123& url, const xstring& alias, const char* start, const char* stop);
  virtual                  ~PlayableRef();
  // Swap instance properties (fast)
  // You must not swap instances that belong to different Playable objects.
  virtual void             Swap(PlayableRef& r);

  // Get't the referenced content.
  Playable&                GetPlayable() const { return *RefTo; }
  // Display name
  // This returns either Info.meta.title or the object name of the current URL.
  // Keep in mind that this may not return the expected value unless EnsureInfo(IF_Meta) has been called.
  virtual xstring          GetDisplayName() const;

  // Start and Stop position
  xstring                  GetStart() const    { return Start; }
  xstring                  GetStop() const     { return Stop; }
  // This functions are only valid if IF_Other has already been requested.
  virtual const SongIterator* GetStartIter() const;
  virtual const SongIterator* GetStopIter() const;
  // Note that since the start and stop locations may not be verified immediately
  // because the underlying polaylist may not be loaded so far. You can set
  // any invalid string here. However, the error may come up later, when
  // GetStartIter() is called. 
  virtual void             SetStart(const xstring& start);
  virtual void             SetStop(const xstring& stop);
  // The functions take the ownership of the SongIterator!
  //virtual void             SetStart(SongIterator* iter);
  //virtual void             SetStop (SongIterator* iter);
  // Aliasname
  xstring                  GetAlias() const    { return Alias; }
  virtual void             SetAlias(const xstring& alias);
  // Usage status
  bool                     IsInUse () const;
  virtual void             SetInUse(bool used);
  // Override properties
  // Calling the functions with NULL arguments will revoke the overriding.
  void                     OverrideTech(const TECH_INFO* info);
  void                     OverrideMeta(const META_INFO* info);
  void                     OverridePhys(const PHYS_INFO* info);
  void                     OverrideAttr(const ATTR_INFO* info);
  //void                     OverrideRpl (const RPL_INFO*  info);
  // Override all kind of information. The what parameter tells which kind of info
  // is to be overridden.
  void                     OverrideInfo(const INFO_BUNDLE* info, InfoFlags what);

  // Redefine GetInfo with a faster, non-virtual version.
  const INFO_BUNDLE&       GetInfo() const     { return Info; }
  // Check wether the requested information is immediately available.
  // Return the bits in what that are /not/ available.
  InfoFlags                CheckInfo(InfoFlags what, bool confirmed = false) const
                           { return RefTo->CheckInfo(what & ~Overridden, confirmed); }

  #ifdef DEBUG_LOG
  xstring                  DebugName() const;
  #endif

 private: // Event handler
  void                     InfoChangeHandler(const PlayableChangeArgs& args);
  void                     CollectionChangeHandler(const CollectionChangeArgs& args);
 protected:
  virtual bool             RequestInfo(InfoFlags what, Priority pri);
  virtual bool             DoLoadInfo(Priority);
 private: // PlayableBase interface implementations
  virtual const Playable&  DoGetPlayable() const;
  virtual const INFO_BUNDLE& DoGetInfo() const;
  virtual InfoFlags        DoCheckInfo(InfoFlags, bool) const;
};*/

#include "location.h"

#endif

