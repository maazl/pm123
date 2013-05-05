/*
 * Copyright 2012-2012 M.Mueller
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

#include "songiterator.h"
#include "playable.h"
#include "dependencyinfo.h"
#include <cpp/algorithm.h>
#include <stdint.h>

#include <debuglog.h>


/*
*
*  class ShuffleWorker
*
*/

SongIterator::ShuffleWorker::ShuffleWorker(Playable& playlist, long seed)
: Playlist(&playlist)
, Seed(seed)
, PlaylistDeleg(*this, &ShuffleWorker::PlaylistChangeNotification)
{ // Set marker for full update on the first request.
  ChangeSet.append();
  // Observe playlist changes
  playlist.GetInfoChange() += PlaylistDeleg;
}

long SongIterator::ShuffleWorker::CalcHash(const PlayableInstance& item, long seed)
{ uint64_t key = ((uint64_t)seed << 32) + (long)&item;
  key += ~key << 18;
  key ^= key >> 31;
  key *= 21;
  key ^= key >> 11;
  key += key << 6;
  key ^= key >> 22;
  return (long)key;
}

int SongIterator::ShuffleWorker::ItemComparer(const KeyType& key, const PlayableInstance& item)
{ long pikey = CalcHash(item, key.Seed);
  if (key.Value > pikey)
    return 1;
  if (key.Value < pikey)
    return -1;
  return (int)&key.Item - (int)&item;
}

int SongIterator::ShuffleWorker::ChangeSetComparer(const PlayableInstance& key, const PlayableInstance& item)
{ return (int)&key - (int)&item;
}

void SongIterator::ShuffleWorker::PlaylistChangeNotification(const CollectionChangeArgs& args)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::PlaylistChangeNotification({&%p, %p, %x..., %u})\n",
    this, Playlist.get(), &args.Instance, args.Origin, args.Changed, args.Type));
  if (args.Changed & IF_Child)
  { switch (args.Type)
    {case PCT_Insert:
     case PCT_Delete:
       // Already PCT_All mode
       if (ChangeSet.size() == 1 && ChangeSet[0] == NULL)
         break;
       // Limit change set size
       if ((int)ChangeSet.size() <= args.Instance.GetInfo().obj->num_items / 7 + 1)
       { PlayableInstance* pi = (PlayableInstance*)args.Origin;
         ChangeSet.get(*pi) = pi;
         break;
       }
       // Change set too large => convert to PCT_All mode
     case PCT_All:
       if (ChangeSet.size() != 1 || ChangeSet[0])
       { ChangeSet.set_size(1);
         ChangeSet[0].reset();
       }
     default:;
    }
  }
}

void SongIterator::ShuffleWorker::UpdateItem(PlayableInstance& item)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::UpdateItem(&%p)\n", this, Playlist.get(), &item));
  KeyType key(MakeKey(item));
  size_t pos;
  if (Items.locate(key, pos))
  { // Item is in the list.
    if (item.GetIndex() == 0)
      // But it should no longer be.
      Items.erase(pos);
  } else
  { // Item is not in the list.
    if (item.GetIndex())
      // But it should be.
      Items.insert(pos) = &item;
  }
}

void SongIterator::ShuffleWorker::Update()
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Update()\n", this, Playlist.get()));
  if (!ChangeSet.size()) // Double check below
    return;
  // Get changes
  ChangeSetType changes;
  { // Synchronize to Playlist to avoid collisions with PlaylistChangeNotification.
    Mutex::Lock lock(Playlist->Mtx);
    changes.swap(ChangeSet);
  }
  switch (changes.size())
  {case 0:
    return;
   case 1:
    if (changes[0] == NULL)
    { // Full update
      int count = Playlist->GetInfo().obj->num_items;
      if (Items.size() < count)
        Items.reserve(count);
      int_ptr<PlayableInstance> pi;
      while ((pi = Playlist->GetNext(pi)) != NULL)
        UpdateItem(*pi);
      break;
    }
   default:
    // Delta update
    foreach (const int_ptr<PlayableInstance>,*, pip, changes)
      UpdateItem(**pip);
  }
}

unsigned SongIterator::ShuffleWorker::GetIndex(PlayableInstance& item)
{ Update();
  size_t pos;
  if (!Items.locate(MakeKey(item), pos))
    return -1;
  return pos;
}

int_ptr<PlayableInstance> SongIterator::ShuffleWorker::Next(PlayableInstance* pi)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Next(%p)\n", this, Playlist.get(), pi));
  Update();
  size_t index = 0;
  if (pi && Items.locate(MakeKey(*pi), index))
    ++index;
  return index < Items.size() ? Items[index] : NULL;
}

int_ptr<PlayableInstance> SongIterator::ShuffleWorker::Prev(PlayableInstance* pi)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Prev(%p)\n", this, Playlist.get(), pi));
  Update();
  size_t index = Items.size() -1;
  if (pi)
  { size_t pos;
    Items.locate(MakeKey(*pi), pos);
    if (pos)
      index = pos -1;
  }
  return index < Items.size() ? Items[index] : NULL;
}


/*
*
*  class SongIterator
*
*/

SongIterator::OffsetCacheEntry::OffsetCacheEntry()
: Offset(Exclude)
, ListChangeDeleg(*this, &OffsetCacheEntry::ListChangeHandler)
{}

void SongIterator::OffsetCacheEntry::ListChangeHandler(const PlayableChangeArgs& args)
{ DEBUGLOG(("SongIterator::OffsetCacheEntry(%p)::ListChangeHandler({, %x, %x, %x}) - %x\n", this,
    args.Loaded, args.Changed, args.Invalidated, Valid.get()));
  InfoFlags what = args.Changed | args.Invalidated;
  if (what & IF_Child)
    what |= IF_Aggreg;
  Valid &= ~what;
}


SongIterator::SongIterator(APlayable* root)
: Location(root ? &root->GetPlayable() : NULL)
, Root(root)
, Options(PLO_NO_SHUFFLE)
, ShuffleSeed(0)
, IsShuffleCache(PLO_NONE)
{ //Reshuffle();
}
SongIterator::SongIterator(const SongIterator& r)
: Location(r)
, Root(r.Root)
, Options(r.Options)
, ShuffleSeed(r.ShuffleSeed)
, ShuffleWorkerCache(r.ShuffleWorkerCache)
, IsShuffleCache(r.IsShuffleCache)
{}

SongIterator::~SongIterator()
{ // Discard Root of Location before Playable might die.
  Location::SetRoot(NULL);
}

void SongIterator::ShuffleWorkerCacheCleanup()
{ DEBUGLOG(("SongIterator(%p)::ShuffleWorkerCacheCleanup() - %u, %u\n", this, GetCallstack().size(), ShuffleWorkerCache.size()));
  size_t maxvalid = 0;
  APlayable* list = Root;
  size_t level = 0;
  while (list && level < ShuffleWorkerCache.size())
  { ShuffleWorker* sw = ShuffleWorkerCache[level];
    if (sw)
    { if (sw->Playlist != &list->GetPlayable())
        break;
      maxvalid = level; // cache is valid
    }
    if (level >= GetCallstack().size())
      break;
    list = GetCallstack()[level];
    ++level;
  }
  ShuffleWorkerCache.set_size(maxvalid);
}

void SongIterator::Enter()
{ Playable& list = GetCurrent()->GetPlayable();
  PL_OPTIONS plo = (PL_OPTIONS)list.GetInfo().attr->ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE);
  if (plo)
    IsShuffleCache = plo;
  Location::Enter();
}

void SongIterator::Leave()
{ IsShuffleCache = PLO_NONE;
  size_t depth = GetCallstack().size();
  if (ShuffleWorkerCache.size() > depth)
    ShuffleWorkerCache.set_size(depth);
  Location::Leave();
  depth = GetCallstack().size();
  if (OffsetCache.size() > depth)
    OffsetCache.set_size(depth);
}

bool SongIterator::PrevNextCore(JobSet& job, bool direction)
{ DEBUGLOG(("SongIterator(%p)::PrevNextCore(,%u)\n", this, direction));
  const vector<PlayableInstance>& callstack = GetCallstack();
  size_t depth = callstack.size();
  ASSERT(depth);
  APlayable* list = depth > 1 ? callstack[depth-2] : Root;

  // Only play one item of an alternation list, and always the (logical) first.
  if (list->GetInfo().attr->ploptions & PLO_ALTERNATION)
  { direction = true;
    APlayable* cur = callstack[depth-1];
    if (!cur)
      goto leave;
    if (job.RequestInfo(*cur, IF_Tech))
      return false;
    if ((cur->GetInfo().tech->attributes & TATTR_INVALID) == 0)
    {leave:
      NavigateTo(NULL);
      return true;
    }
  }

  if (!IsShuffle())
    return Location::PrevNextCore(job, direction);
  // Prev/next in shuffle mode
  DEBUGLOG(("SongIterator(%p)::PrevNextCore(%u)\n", this, direction));
  // Shuffle cache lookup
  ShuffleWorkerCache.set_size(depth);
  ShuffleWorker* sw = ShuffleWorkerCache[--depth];
  if (!sw)
    // Cache miss
    ShuffleWorkerCache[depth] = sw = new ShuffleWorker(list->GetPlayable(), ShuffleSeed);
  else
    ASSERT(sw->Playlist == (depth ? &callstack[depth-1]->GetPlayable() : Location::GetRoot()));
  // Navigate
  PlayableInstance* pi = callstack[depth];
  pi = direction ? sw->Next(pi) : sw->Prev(pi);
  NavigateTo(pi);
  return true;
}

void SongIterator::SetRoot(Playable* root)
{ ShuffleWorkerCache.clear();
  IsShuffleCache = PLO_NONE;
  Root = root;
  Location::SetRoot(root);
}
void SongIterator::SetRoot(APlayable* root)
{ ShuffleWorkerCache.clear();
  IsShuffleCache = PLO_NONE;
  Root = root;
  Location::SetRoot(root ? &root->GetPlayable() : NULL);
}

SongIterator& SongIterator::operator=(const Location& r)
{ DEBUGLOG(("SongIterator(%p)::operator=(Location&%p)\n", this, &r));
  Root = r.GetRoot();
  Location::operator=(r);
  IsShuffleCache = PLO_NONE;
  ShuffleWorkerCacheCleanup();
  return *this;
}
SongIterator& SongIterator::operator=(const SongIterator& r)
{ DEBUGLOG(("SongIterator(%p)::operator=(SongIterator&%p)\n", this, &r));
  Root = r.Root;
  Location::operator=(r);
  IsShuffleCache = PLO_NONE;
  ShuffleWorkerCacheCleanup();
  return *this;
}

void SongIterator::Swap2(Location& l)
{ DEBUGLOG(("SongIterator(%p)::Swap2(&%p)\n", this, &l));
  // swap location slice
  Location::Swap2(l);
  // do post processing
  Root = l.GetRoot();
  IsShuffleCache = PLO_NONE;
  ShuffleWorkerCacheCleanup();
}

void SongIterator::Swap(Location& r)
{ DEBUGLOG(("SongIterator(%p)::Swap(&%p)\n", this, &r));
  CallSwap2(r, *this);
  // do post processing
  Root = r.GetRoot();
  IsShuffleCache = PLO_NONE;
  ShuffleWorkerCacheCleanup();
}

void SongIterator::SetOptions(PL_OPTIONS options)
{ Options = options;
  // Cache cleanup
  if (options & PLO_NO_SHUFFLE)
  { // clear cache completely
    ShuffleWorkerCache.clear();
    return;
  }
  IsShuffleCache = PLO_NONE;
  if (!(options & PLO_SHUFFLE) && ShuffleWorkerCache.size())
  { // Clear cache up to the first entry that overrides PLO_*SHUFFLE.
    if (Root)
    { ShuffleWorkerCache[0].reset();
      size_t i = 0;
      const vector<PlayableInstance>& callstack = GetCallstack();
      while (i < callstack.size())
      { if (i >= ShuffleWorkerCache.size())
          break;
        PlayableInstance* pi = callstack[i];
        if (!pi)
          break;
        PL_OPTIONS plo = (PL_OPTIONS)pi->GetInfo().attr->ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE);
        if (!plo)
          break;
        ShuffleWorkerCache[++i].reset();
      }
    }
  }
}

bool SongIterator::IsShuffle() const
{ if (Options & PLO_NO_SHUFFLE)
    return false;
  if (!IsShuffleCache)
  { const vector<PlayableInstance>& callstack = GetCallstack();
    size_t depth = callstack.size();
    while (depth > 1)
    { ((SongIterator&)*this).IsShuffleCache = (PL_OPTIONS)callstack[depth-2]->GetInfo().attr->ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE);
      if (IsShuffleCache)
        goto done;
      --depth;
    }
    ((SongIterator&)*this).IsShuffleCache = Options & PLO_SHUFFLE ? PLO_SHUFFLE : PLO_NO_SHUFFLE;
  }
 done:
  return !(IsShuffleCache & PLO_NO_SHUFFLE);
}
