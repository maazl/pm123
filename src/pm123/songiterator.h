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


class ShuffleWorker : public Iref_count
{private:
  /// Hack to inject the seed into the comparer without making CacheComparer
  /// a member function.
  struct KeyType
  { const PlayableInstance&   Item;
    long                      Value;
    long                      Seed;
    KeyType(PlayableInstance& item, long value, long seed) : Item(item), Value(value), Seed(seed) {}
  };
 public:
  const int_ptr<Playable>     Playlist;
  const long                  Seed;
 private:
  static long                 CalcHash(const PlayableInstance& item, long seed);
  KeyType                     MakeKey(PlayableInstance& item) { return KeyType(item, CalcHash(item, Seed), Seed); }
  static int                  ItemComparer(const KeyType& key, const PlayableInstance& item);
  static int                  ChangeSetComparer(const PlayableInstance& key, const PlayableInstance& item);
  void                        PlaylistChangeNotification(const CollectionChangeArgs& args);
  void                        UpdateItem(PlayableInstance& item);
  void                        Update();

 private:
  typedef sorted_vector_int<PlayableInstance,KeyType,&ShuffleWorker::ItemComparer> ItemsType;
  typedef sorted_vector_int<PlayableInstance,PlayableInstance,&ShuffleWorker::ChangeSetComparer> ChangeSetType;
 private:
  ItemsType                   Items;
  ChangeSetType               ChangeSet;
  class_delegate<ShuffleWorker,const CollectionChangeArgs> PlaylistDeleg;

 public:
  ShuffleWorker(Playable& playlist, long seed);
  int                         GetIndex(PlayableInstance& item);
  int_ptr<PlayableInstance>   Next(PlayableInstance* pi);
  int_ptr<PlayableInstance>   Prev(PlayableInstance* pi);
};

/** A SongIterator is a Location intended to be used to play a Playable object.
 * In contrast to Location it owns it's root.
 * @remarks The class is non-polymorphic. You must not change the root by calling
 * Location::SetRoot, Location::Swap or Location::operator= directly.
 */
class SongIterator : public Location
{private:

 public:
  struct OffsetInfo
  { /// Song index within the current root counting from 0. The value is -1
    /// if the information is not yet available because of an asynchronous requests.
    int                       Index;
    /// Time offset of this location within root.
    /// The value is -1 if the time offset is undefined because either
    /// some information is missing and requested asynchronously or
    /// at least one object has indefinite length.
    PM123_TIME                Time;
    OffsetInfo()              : Index(0), Time(0) {}
    OffsetInfo(int index, PM123_TIME time) : Index(index), Time(time) {}
    /// Add another offset. Note that the singular values -1 always win.
    OffsetInfo& operator+=(const OffsetInfo& r);
  };

 public:
  /// @brief Iteration options
  /// @details The options have slightly different meaning than in ATTR_INFO.
  /// Setting \c PLO_SHUFFLE enables shuffle mode at the top level.
  /// Setting \c PLO_NO_SHUFFLE entirely disables shuffle mode also overriding
  /// \c PLO_SHUFFLE in nested playlists. This is the default.
  /// Setting neither of them enables shuffle processing but does not turn it on
  /// at the top level.
  PL_OPTIONS                  Options;
 private:
  static int                  ShuffleWorkerComparer(const Playable& key, const ShuffleWorker& elem);
 private:
  long                        ShuffleSeed;
  sorted_vector_int<ShuffleWorker, Playable, &SongIterator::ShuffleWorkerComparer> ShuffleWorkerCache;

  // Implementation note:
  // Strictly speaking SongIterator should have a class member of type
  // int_ptr<Playable> to keep the reference to the root object alive.
  // To save this extra member the functionality is emulated by the
  // C-API interface of int_ptr<>.
 public:
  explicit                    SongIterator(Playable* root = NULL)
                              : Location(root)
                              , Options(PLO_NO_SHUFFLE)
                              { // Increment reference counter
                                int_ptr<Playable> ptr(GetRoot());
                                ptr.toCptr();
                              }
                              SongIterator(const SongIterator& r)
                              : Location(r)
                              , Options(PLO_NO_SHUFFLE)
                              { // Increment reference counter
                                int_ptr<Playable> ptr(GetRoot());
                                ptr.toCptr();
                              }
                              ~SongIterator()
                              { int_ptr<Playable>().fromCptr(GetRoot());
                              }
  void                        SetRoot(Playable* root);
  SongIterator&               operator=(const SongIterator& r);

  /// @brief Return the song and time offset of this location within root.
  /// @param level Offset from the levelth callstack entry.
  /// The default level 0 calculates the offsets from the root.
  /// \a level must be in the range [0,GetLevel()].
  /// @remarks Note that GetOffsetInfo may return different results
  /// for the same Playable object depending on the callstack entries < \a level.
  OffsetInfo                  CalcOffsetInfo(size_t level = 0);
};

#endif
