/*  
 * Copyright 2011-2011 Marcel Müller
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


#ifndef COLLECTIONINFOCACHE_H
#define COLLECTIONINFOCACHE_H

#include <config.h>
#include "aplayable.h"
#include "infobundle.h"
#include "playableset.h"

#include <cpp/mutex.h>
#include <cpp/container/sorted_vector.h>


/** @brief Base structure for collection info entries.
 *
 * Public part of CollectionInfoCache entries.
 */
struct CollectionInfo
: public AggregateInfo
{ InfoState         InfoStat;
  //const PlayableSetBase&    Exclude; // Virtual base class supplied from creator or derived class.
  CollectionInfo(const PlayableSetBase& exclude, InfoFlags preset = IF_None) : AggregateInfo(exclude), InfoStat(preset) {}
  InfoFlags         RequestAI(InfoFlags& what, Priority pri, Reliability rel);
};


/** @brief Class to support any playable object.
 *
 * This class instances are unique for each URL.
 * Comparing two pointers for equality is equivalent to comparing the URL for equality.
 *
 * All instances of this class are owned by the class itself and must not be deleted.
 * You should only hold instances in the smart pointer \c int_ptr<Playable>. Otherwise
 * the referred object may be deleted in background. But it is allowed to call
 * a function with parameter \c Playable* (e.g. member functions) while you have an active
 * instance of \c int_ptr<Playable> unless the function keeps a local copy of the pointer
 * after it returns. It is also allowed to pass the result of a function returning
 * \c int_ptr<Playable> directly to a function taking \c Playable* with the same restriction
 * because temporary objects are always kept until the next statement.
 * If a function that receives \c Playable* wants to keep the reference after returning
 * it must create a \c int_ptr<Playable> object. This will work as expected.
 */
class CollectionInfoCache
{protected:
  /// Structure to hold a CollectionInfoCache entry.
  struct CacheEntry
  : public PlayableSet,
    public CollectionInfo
  { explicit CacheEntry(const PlayableSetBase& key) : PlayableSet(key), CollectionInfo((PlayableSet&)*this) {};
    explicit CacheEntry(PlayableSet& key) : PlayableSet(), CollectionInfo((PlayableSet&)*this) { swap(key); };
    static int      compare(const PlayableSetBase& l, const CacheEntry& r);
  };

 protected: // Context
  Playable&         Parent;

  /// @brief Cache with sub enumeration infos.
  /// @details This object is protected by the mutex below.
  sorted_vector_own<CacheEntry, PlayableSetBase, &CacheEntry::compare> Cache;
  /// @brief One Mutex to protect Cache of all instances.
  /// @details You must not acquire any other Mutex while holding this one.
  static Mutex      CacheMutex;

 public:
  CollectionInfoCache(Playable& parent) : Parent(parent) {}
  /// @brief Get collection info cache element for exclusion list exclude.
  /// @details The implementation takes care of the fact that *this is always excluded regardless whether
  /// it is part of \a exclude or not.
  /// @return Pointer to the appropriate \c CollectionInfo or \c NULL
  /// if there are no exclusions.
  /// The returned storage is valid until \c *this dies.
  CollectionInfo*   Lookup(const PlayableSetBase& exclude);
  /// Invalidate all entries that do \e not contain \a pp.
  /// @param what Information to invalidate. Must be a subset of \c IF_Aggreg.
  /// @param pp playable object that caused the invalidation. If pp is NULL,
  /// all entries are invalidated.
  /// @return subset of \a what that really caused an invalidation
  /// in at least one entry.
  InfoFlags         Invalidate(InfoFlags what, const Playable* pp);
  /// Advances item to the next CollectionInfoCache entry that needs some work to be done
  /// at the given or higher priority level.
  /// @param pri Priority level.
  /// @param item If item is \c NULL \c GetNextCIWorkItem moves to the first item.
  /// If there is no more work, item is set to \c NULL.
  /// @param upd The function will place the request matching the work item in this update worker class.
  /// @return The function returns \c true unless there are no more work items.
  bool              GetNextWorkItem(CollectionInfo*& item, Priority pri, InfoState::Update& upd);
  /// Access to request state for diagnostic purposes.
  void              PeekRequest(RequestState& req) const;
};


#endif
