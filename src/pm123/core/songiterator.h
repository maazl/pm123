/*
 * Copyright 2007-2012 M.Mueller
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


#ifndef SONGITERATOR_H
#define SONGITERATOR_H

#include "location.h"
#include "playable.h"
#include <cpp/container/btree.h>
#include <cpp/event.h>


/** A \c SongIterator is a \c Location intended to be used to play a \c APlayable object.
 * In contrast to \c Location it owns it's root and it can be attached to \c APlayable rather than \c Playable only.
 */
class SongIterator : public Location
{private:
  struct OffsetCacheEntry
  { /// Exclusion list
    PlayableSet               Exclude;
    /// Cached aggregate information.
    AggregateInfo             Offset;
    /// Valid components of \c Offset. \c IF_Rpl|IF_Drpl.
    AtomicUnsigned            Valid;
    class_delegate<OffsetCacheEntry,const PlayableChangeArgs> ListChangeDeleg;
   private:
    void                      ListChangeHandler(const PlayableChangeArgs& args);
   public:
    OffsetCacheEntry();
  };
  /** Helper class to provide a deterministic shuffle algorithm for a playlist.
   */
  class ShuffleWorker : public Iref_count
  {private:
    /// Hack to inject the seed into the comparer without making CacheComparer
    /// a member function. Instances of this class are immutable.
    struct KeyType
    { /// Playlist item to seek for.
      const PlayableInstance&   Item;
      /// Hash value of this item.
      const long                Value;
      /// Seed used for calculation of hash.
      const long                Seed;
      /// Create a KeyType.
      KeyType(PlayableInstance& item, long value, long seed) : Item(item), Value(value), Seed(seed) {}
    };
   public:
    /// Playlist for which this instance provides shuffle support.
    const int_ptr<Playable>     Playlist;
    /// Seed that is used for the random sequence calculation. Using the same seed results in the same sequence.
    const long                  Seed;
   private:
    /// Calculate the hash value for a playlist item.
    /// @param item Playlist item.
    /// @param seed Use this seed.
    /// @return Hash value for this item. The hash of an item never changes in context of a \a seed.
    static long                 CalcHash(const PlayableInstance& item, long seed);
    /// Factory for KeyType
    KeyType                     MakeKey(PlayableInstance& item) { return KeyType(item, CalcHash(item, Seed), Seed); }
    /// Comparer used to compare entries in the \c Items array.
    static int                  ItemComparer(const KeyType& key, const PlayableInstance& item);
    /// Comparer used to compare entries in the \c ChangeSet array.
    static int                  ChangeSetComparer(const PlayableInstance& key, const PlayableInstance& item);
    /// Event handler for changes of the underlying Playlist.
    void                        PlaylistChangeNotification(const CollectionChangeArgs& args);
    /// Update the cache entry for \a item.
    /// This might add or remove cache entries, depending on whether the item
    /// still belongs to a playlist.
    void                        UpdateItem(PlayableInstance& item);
    /// Proceed and clear the ChangeSet.
    void                        Update();

   private:
    typedef sorted_vector_int<PlayableInstance,KeyType,&ShuffleWorker::ItemComparer> ItemsType;
    typedef sorted_vector_int<PlayableInstance,PlayableInstance,&ShuffleWorker::ChangeSetComparer> ChangeSetType;
   private:
    /// Collection of playlist items in the order of the calculated hash.
    /// @remarks The has value is the key of this sorted list, but the key is not actually stored
    /// in the collection neither there is sufficient information to calculate the key.
    /// The hash values are calculated on demand by the \c ItemComparer.
    /// The necessary information to do so is passed to the \c ItemComparer in the \c KeyType structure.
    ItemsType                   Items;
    /// @brief List of changes of the underlying playlist since the last call to \c Update.
    /// @details The list may either
    /// - be empty, which means that there is nothing to update, or
    /// - contain a list of added or removed children ordered by their memory address or
    /// - contain a single \c NULL reference meaning
    ///   that the entire \c Items array should be recreated from scratch.
    /// Initially the \c ChangeSet is in a single NULL reference. This causes
    /// the population of the \c Items array to be deferred until it is really needed.
    ChangeSetType               ChangeSet;
    /// Delegate to observer playlist changes.
    class_delegate<ShuffleWorker,const CollectionChangeArgs> PlaylistDeleg;

   public:
    /// Create a \c ShuffleWorker instance for a playlist using a given \a seed.
    ShuffleWorker(Playable& playlist, long seed);
    /// Query the zero based index of a playlist item in the randomized sequence.
    unsigned                    GetIndex(PlayableInstance& item);
    /// Advance from one playlist item to the next one, following the individual
    /// sequence of this instance.
    int_ptr<PlayableInstance>   Next(PlayableInstance* pi);
    /// Advance from one playlist item to the previous one, following the individual
    /// sequence of this instance.
    int_ptr<PlayableInstance>   Prev(PlayableInstance* pi);
  };

 private:
  int_ptr<APlayable>          Root;
  /// Use Shuffle mode?
  /// - \c PLO_NO_SHUFFLE means that shuffle is disabled, even if a playlist item sets \c PLO_SHUFFLE.
  /// - \c PLO_SHUFFLE means that shuffle is enabled at the top level. But this might be overridden
  ///   by a nested playlist item.
  /// - \c PLO_NONE means that shuffle is disabled at the top level. This might be overridden
  ///   by a nested playlist item.
  PL_OPTIONS                  Options;
  /// Seed currently used for the pseudo random sort order of all \c ShuffleWorker
  /// instances.
  long                        ShuffleSeed;
  /// @brief Cache for \c ShuffleWorker instances.
  /// @details The items in the cache are correlated to the content of the call stack,
  /// But the index is off by one. The first entry belongs to the root.
  /// The second entry to the first playlist in the call stack and so on.
  vector_int<ShuffleWorker>   ShuffleWorkerCache;
  /// - PLO_NONE := undefined
  /// - PLO_SHUFFLE := true
  /// - PLO_NO_SHUFFLE|x := false
  mutable PL_OPTIONS          IsShuffleCache;
  /// Cached front offsets for each call stack entry.
  /// @details The items in the cache are correlated to the content of the call stack,
  /// But the index is off by one. The first entry belongs to the root.
  /// The second entry to the first playlist in the call stack and so on.
  /// There is no entry for the last item in the call stack. So
  /// \c OffsetCache() is never greater than \c Callstack.size().
  vector_own<OffsetCacheEntry> OffsetCache;
 private:
  /// Remove invalid items from \c ShuffleWorkerCache.
  /// This is called after \c Swap.
  void                        ShuffleWorkerCacheCleanup();

 protected:
  virtual void                Enter();
  virtual void                Leave();
  virtual bool                PrevNextCore(JobSet& job, bool direction);
  virtual void                Swap2(Location& l);
 private:
  virtual void                SetRoot(Playable* root);
  virtual SongIterator&       operator=(const Location& r);
 public:
  explicit                    SongIterator(APlayable* root = NULL);
                              SongIterator(const SongIterator& r);
  virtual                     ~SongIterator();

          APlayable*          GetRoot() const { return Root; }
          void                SetRoot(APlayable* root);
          SongIterator&       operator=(const SongIterator& r);

  virtual void                Swap(Location& r);

  /// @brief Set iteration options
  /// @details The options have slightly different meaning than in ATTR_INFO.
  /// - Setting \c PLO_SHUFFLE enables shuffle mode at the top level.
  /// - Setting \c PLO_NO_SHUFFLE entirely disables shuffle mode also overriding
  ///   \c PLO_SHUFFLE in \c ATTR_INFO of nested playlists. This is the default.
  /// - Setting neither of them enables shuffle processing but does not turn it on
  ///   at the top level.
  // - Setting \c PLO_ALTERNATION treats Root as an alternation list.
  void                        SetOptions(PL_OPTIONS options);

  /// @brief Check whether the current innermost playlist is in shuffle mode.
  /// @details The function never checks the current item, even if it is a playlist.
  bool                        IsShuffle() const;

  /// Set new Shuffle seed
  void                        Reshuffle() { ShuffleSeed = rand() ^ (rand() << 16); ShuffleWorkerCache.clear(); }
};

#endif
