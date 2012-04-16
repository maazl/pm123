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

int SongIterator::ShuffleWorkerComparer(const Playable& key, const ShuffleWorker& elem)
{ return (int)&key - (int)elem.Playlist.get();
}

void SongIterator::SetRoot(Playable* root)
{ int_ptr<Playable> ptr(root);
  ptr.toCptr();
  ptr.fromCptr(GetRoot());
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

SongIterator::OffsetInfo SongIterator::CalcOffsetInfo(size_t level)
{ DEBUGLOG(("SongIterator(%p)::GetOffsetInfo(%u)\n", this, level));
  ASSERT(level <= GetLevel());
  if (level == GetLevel())
    return OffsetInfo(0, GetPosition());
  OffsetInfo off = CalcOffsetInfo(level+1);

  return off;
}
