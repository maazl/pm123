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

#include <debuglog.h>


/*
*
*  class ShuffleWorker
*
*/

ShuffleWorker::ShuffleWorker(Playable& playlist, long seed)
: Playlist(&playlist)
, Seed(seed)
, PlaylistDeleg(*this, &ShuffleWorker::PlaylistChangeNotification)
{ // Set marker for full update on the first request.
  ChangeSet.append();
  // Observe playlist changes
  playlist.GetInfoChange() += PlaylistDeleg;
}

long ShuffleWorker::CalcHash(const PlayableInstance& item, long seed)
{ // TODO: use a reasonable hash algorithm.
  return seed ^ (long)&item;
}

int ShuffleWorker::ItemComparer(const KeyType& key, const PlayableInstance& item)
{ long pikey = CalcHash(item, key.Seed);
  if (key.Value > pikey)
    return 1;
  if (key.Value < pikey)
    return -1;
  return (int)&key.Item - (int)&item;
}

int ShuffleWorker::ChangeSetComparer(const PlayableInstance& key, const PlayableInstance& item)
{ return (int)&key - (int)&item;
}

void ShuffleWorker::PlaylistChangeNotification(const CollectionChangeArgs& args)
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
       if ((int)ChangeSet.size() <= args.Instance.GetPlayable().GetInfo().obj->num_items / 7 + 1)
       { PlayableInstance* pi = (PlayableInstance*)args.Origin;
         ChangeSet.get(*pi) = pi;
         break;
       }
       // Change set too large => convert to PCT_All mode
     case PCT_All:
       if (ChangeSet.size() != 1 || ChangeSet[0])
       { ChangeSet.clear();
         ChangeSet.append();
       }
     default:;
    }
  }
}

void ShuffleWorker::UpdateItem(PlayableInstance& item)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::UpdateItem(&%p)\n", this, Playlist.get(), &item));
  KeyType key(MakeKey(item));
  size_t pos;
  if (Items.binary_search(key, pos))
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

void ShuffleWorker::Update()
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
      int_ptr<PlayableInstance> pi;
      while ((pi = Playlist->GetNext(pi)) != NULL)
        UpdateItem(*pi);
    }
   default:
    // Delta update
    foreach (const int_ptr<PlayableInstance>*, pip, changes)
      UpdateItem(**pip);
  }
}

int ShuffleWorker::GetIndex(PlayableInstance& item)
{ Update();
  size_t pos;
  if (!Items.binary_search(MakeKey(item), pos))
    return -1;
  return pos;
}

int_ptr<PlayableInstance> ShuffleWorker::Next(PlayableInstance* pi)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Next(%p)\n", this, Playlist.get(), pi));
  Update();
  size_t index = 0;
  if (pi && Items.binary_search(MakeKey(*pi), index))
    ++index;
  return index < Items.size() ? Items[index] : NULL;
}

int_ptr<PlayableInstance> ShuffleWorker::Prev(PlayableInstance* pi)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Prev(%p)\n", this, Playlist.get(), pi));
  Update();
  size_t index = Items.size() -1;
  if (pi)
  { size_t pos;
    Items.binary_search(MakeKey(*pi), pos);
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

SongIterator::OffsetInfo& SongIterator::OffsetInfo::operator+=(const OffsetInfo& r)
{ if (Index >= 0)
  { if (r.Index >= 0)
      Index += r.Index;
    else
      Index = -1;
  }
  if (Time >= 0)
  { if (r.Time >= 0)
      Time += r.Time;
    else
      Time = -1;
  }
  return *this;
}

SongIterator::SongIterator(Playable* root)
: Location(root)
, Options(PLO_NO_SHUFFLE)
//, ShuffleSeed(0)
, IsShuffleCache(PLO_NONE)
{ // Increment reference counter
  int_ptr<Playable> ptr(GetRoot());
  ptr.toCptr();
  Reshuffle();
}
SongIterator::SongIterator(const SongIterator& r)
: Location(r)
, Options(r.Options)
, ShuffleSeed(r.ShuffleSeed)
, ShuffleWorkerCache(r.ShuffleWorkerCache)
, IsShuffleCache(r.IsShuffleCache)
{ // Increment reference counter
  int_ptr<Playable> ptr(GetRoot());
  ptr.toCptr();
}

SongIterator::~SongIterator()
{ int_ptr<Playable>().fromCptr(GetRoot());
}

int SongIterator::ShuffleWorkerComparer(const Playable& key, const ShuffleWorker& elem)
{ return (int)&key - (int)elem.Playlist.get();
}

void SongIterator::ShuffleWorkerCacheCleanup()
{ size_t pos = ShuffleWorkerCache.size();
  DEBUGLOG(("SongIterator(%p)::ShuffleWorkerCacheCleanup() - %u\n", this, pos));
  while (pos--)
    if (!FindInCallstack(ShuffleWorkerCache[pos]->Playlist))
      ShuffleWorkerCache.erase(pos);
}

void SongIterator::Enter()
{ Playable& list = GetCurrent()->GetPlayable();
  PL_OPTIONS plo = (PL_OPTIONS)list.GetInfo().attr->ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE);
  if (plo)
    IsShuffleCache = plo;
  Location::Enter();
}

void SongIterator::Leave()
{ APlayable* cur = GetCurrent();
  if (cur)
  ShuffleWorkerCache.erase(cur->GetPlayable());
  IsShuffleCache = PLO_NONE;
  Location::Leave();
}

void SongIterator::PrevNextCore(bool direction)
{ if (!IsShuffle())
  { Location::PrevNextCore(direction);
    return;
  }
  // Prev/next in shuffle mode
  DEBUGLOG(("SongIterator(%p)::PrevNextCore(%u)\n", this, direction));
  const size_t depth = Callstack.size() -1;
  Playable& list = depth ? Callstack[depth-1]->GetPlayable() : *Root;
  // Shuffle cache lookup
  int_ptr<ShuffleWorker>& sw = ShuffleWorkerCache.get(list);
  if (!sw)
    // Cache miss
    sw = new ShuffleWorker(list, ShuffleSeed);
  // Navigate
  int_ptr<PlayableInstance>& pi = Callstack[depth];
  pi = direction ? sw->Next(pi) : sw->Prev(pi);
}

void SongIterator::SetRoot(Playable* root)
{ int_ptr<Playable> ptr(root);
  ptr.toCptr();
  ptr.fromCptr(GetRoot());
  ShuffleWorkerCache.clear();
  IsShuffleCache = PLO_NONE;
  Location::SetRoot(root);
}

SongIterator& SongIterator::operator=(const SongIterator& r)
{ DEBUGLOG(("SongIterator(%p)::operator=(&%p)\n", this, &r));
  int_ptr<Playable> ptr(r.GetRoot());
  ptr.toCptr();
  ptr.fromCptr(GetRoot());
  Location::operator=(r);
  return *this;
}

void SongIterator::Swap2(Location& l, int magic)
{ DEBUGLOG(("SongIterator(%p)::Swap2(&%p, %i)\n", this, &l, magic));
  // swap location slice
  Location::Swap2(l, magic);
  if (magic)
  { // swap two SongIterator
    SongIterator& si = (SongIterator&)l;
    swap(Options, si.Options);
    swap(ShuffleSeed, si.ShuffleSeed);
    ShuffleWorkerCache.swap(si.ShuffleWorkerCache);
    swap(IsShuffleCache, si.IsShuffleCache);
  } else
  { // do post processing
    IsShuffleCache = PLO_NONE;
    ShuffleWorkerCacheCleanup();
  }
}

void SongIterator::Swap(Location& r)
{ DEBUGLOG(("SongIterator(%p)::Swap(&%p)\n", this, &r));
  CallSwap2(r, *this, 1);
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
  if (!(options & PLO_SHUFFLE))
  { // Clear cache up to the first entry that overrides PLO_*SHUFFLE.
    Playable* root = GetRoot();
    if (root)
    { ShuffleWorkerCache.erase(*root);
      foreach (PlayableInstance*const*, ppi, GetCallstack())
      { PlayableInstance* pi = *ppi;
        if (!pi)
          break;
        PL_OPTIONS plo = (PL_OPTIONS)pi->GetInfo().attr->ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE);
        if (!plo)
          break;
        ShuffleWorkerCache.erase(pi->GetPlayable());
      }
    }
  }
}

bool SongIterator::IsShuffle() const
{ if (Options & PLO_NO_SHUFFLE)
    return false;
  if (!IsShuffleCache)
  { size_t depth = Callstack.size();
    while (depth > 1)
    { ((SongIterator&)*this).IsShuffleCache = (PL_OPTIONS)Callstack[depth-2]->GetInfo().attr->ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE);
      if (IsShuffleCache)
        goto done;
      --depth;
    }
    ((SongIterator&)*this).IsShuffleCache = (Options & PLO_SHUFFLE)|PLO_NO_SHUFFLE;
  }
 done:
  return !(IsShuffleCache & PLO_NO_SHUFFLE);
}

SongIterator::OffsetInfo SongIterator::CalcOffsetInfo(size_t level)
{ DEBUGLOG(("SongIterator(%p)::GetOffsetInfo(%u)\n", this, level));
  ASSERT(level <= GetLevel());
  if (level == GetLevel())
    return OffsetInfo(0, GetPosition());
  OffsetInfo off = CalcOffsetInfo(level+1);

  return off;
}
