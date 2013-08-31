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
#include "job.h"
#include <cpp/algorithm.h>
#include <stdint.h>

#include <debuglog.h>


/*
*
*  class ShuffleWorker
*
*/

SongIterator::ShuffleWorker::ShuffleWorker(Playable& playlist, long seed)
: Playlist(playlist)
, Seed(seed)
, PlaylistDeleg(*this, &ShuffleWorker::ListChangeHandler)
{ // Set marker for full update on the first request.
  ChangeSet.append();
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

void SongIterator::ShuffleWorker::ListChangeHandler(const CollectionChangeArgs& args)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::ListChangeHandler({&%p, %p, %x..., %u})\n",
    this, &Playlist, &args.Instance, args.Origin, args.Changed, args.Type));
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
{ DEBUGLOG(("ShuffleWorker(%p{%p})::UpdateItem(&%p)\n", this, &Playlist, &item));
  KeyType key(MakeKey(item));
  Iterator pos;
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
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Update()\n", this, &Playlist));
  if (!ChangeSet.size()) // Double check below
    return;
  // Get changes
  ChangeSetType changes;
  { // Synchronize to Playlist to avoid collisions with PlaylistChangeNotification.
    Mutex::Lock lock(Playlist.Mtx);
    changes.swap(ChangeSet);
  }
  switch (changes.size())
  {case 0:
    return;
   case 1:
    if (changes[0] == NULL)
    { // Full update
      int_ptr<PlayableInstance> pi;
      while ((pi = Playlist.GetNext(pi)) != NULL)
        UpdateItem(*pi);
      break;
    }
   default:
    // Delta update
    foreach (const int_ptr<PlayableInstance>,*, pip, changes)
      UpdateItem(**pip);
  }
}

SongIterator::ShuffleWorker::Iterator SongIterator::ShuffleWorker::GetLocation(PlayableInstance& item)
{ Update();
  Iterator pos;
  Items.locate(MakeKey(item), pos);
  return pos;
}

int_ptr<PlayableInstance> SongIterator::ShuffleWorker::Next(PlayableInstance* pi)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Next(%p)\n", this, &Playlist, pi));
  Update();
  if (Items.empty())
    return NULL;
  if (!pi)
    return *Items.begin();
  Iterator index;
  if (Items.locate(MakeKey(*pi), index))
    ++index;
  return !index.isend() ? *index : NULL;
}

int_ptr<PlayableInstance> SongIterator::ShuffleWorker::Prev(PlayableInstance* pi)
{ DEBUGLOG(("ShuffleWorker(%p{%p})::Prev(%p)\n", this, &Playlist, pi));
  Update();
  if (Items.empty())
    return NULL;
  Iterator index;
  if (!pi)
  { index = Items.end();
  } else
  { Items.locate(MakeKey(*pi), index);
    if (index.isbegin())
      return NULL;
  }
  return *--index;
}


/*
*  class SongIterator::OffsetCacheEntry
*/

SongIterator::OffsetCacheEntry::OffsetCacheEntry(APlayable& root, const vector<PlayableInstance> callstack, unsigned level)
: Parent(level ? *callstack[level-1] : root)
, Front(Exclude)
, Back(Exclude)
, Valid(IF_None)
, PlaylistDeleg(*this, &OffsetCacheEntry::ListChangeHandler)
{ // Prepare exclude
  if (level)
  { Exclude.reserve(level-1);
    Exclude.add(root.GetPlayable());
    for (unsigned i; i < level-1; ++i)
      Exclude.add(callstack[i]->GetPlayable());
  }
  // activate delegate after Exclude is frozen.
  Parent.GetInfoChange() += PlaylistDeleg;
}
SongIterator::OffsetCacheEntry::OffsetCacheEntry(const OffsetCacheEntry& r)
: Parent(r.Parent)
, Exclude(r.Exclude)
, Front(Exclude)
, Back(Exclude)
, Valid(r.Valid)
, PlaylistDeleg(Parent.GetInfoChange(), *this, &OffsetCacheEntry::ListChangeHandler)
{ // Be sure not to miss an invalidation event
  Valid &= r.Valid;
}

void SongIterator::OffsetCacheEntry::ListChangeHandler(const PlayableChangeArgs& args)
{ DEBUGLOG(("SongIterator::OffsetCacheEntry(%p{%p})::ListChangeHandler({, %x, %x, %x}) - %x\n", this,
    &Parent, args.Loaded, args.Changed, args.Invalidated, Valid.get()));
  InfoFlags what = args.Changed | args.Invalidated;
  if (what & IF_Child)
    what |= IF_Aggreg;
  Valid &= ~what;
}


/*
*  class SongIterator
*/

SongIterator::SongIterator(APlayable* root)
: Location(root ? &root->GetPlayable() : NULL)
, Root(root)
, Options(PLO_NO_SHUFFLE)
, ShuffleSeed(0)
, IsShuffleCache(PLO_NONE)
{ //Reshuffle();
}
SongIterator::SongIterator(const Location& r)
: Location(r)
, Root(r.GetRoot())
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
, OffsetCache(r.OffsetCache)
{}

SongIterator::~SongIterator()
{ // Discard Root of Location before Playable might die.
  Location::SetRoot(NULL);
}

SongIterator::ShuffleWorker& SongIterator::EnsureShuffleWorker(unsigned depth)
{ if (depth >= ShuffleWorkerCache.size())
    ShuffleWorkerCache.set_size(depth+1);
  int_ptr<ShuffleWorker>& swp(ShuffleWorkerCache[depth]);
  if (!swp)
    // Cache miss
    swp = new ShuffleWorker(depth ? GetCallstack()[depth-1]->GetPlayable() : *Location::GetRoot(), ShuffleSeed);
  return *swp;
}

void SongIterator::ShuffleWorkerCacheCleanup()
{ DEBUGLOG(("SongIterator(%p)::ShuffleWorkerCacheCleanup() - %u, %u\n", this, GetCallstack().size(), ShuffleWorkerCache.size()));
  size_t maxvalid = 0;
  APlayable* list = Root;
  size_t level = 0;
  while (list && level < ShuffleWorkerCache.size())
  { ShuffleWorker* sw = ShuffleWorkerCache[level];
    if (sw)
    { if (&sw->Playlist != &list->GetPlayable())
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
  Location::Leave();
  ShuffleWorkerCacheCleanup();
  size_t depth = GetCallstack().size();
  if (OffsetCache.size() > depth)
    OffsetCache.set_size(depth);
}

bool SongIterator::PrevNextCore(Job& job, bool direction)
{ DEBUGLOG(("SongIterator(%p)::PrevNextCore(,%u)\n", this, direction));
  const vector<PlayableInstance>& callstack = GetCallstack();
  size_t depth = callstack.size();
  ASSERT(depth);
  --depth;
  APlayable* list = depth ? callstack[depth-1] : Root;


  // Only play one item of an alternation list, and always the (logical) first.
  bool alternation = list->GetInfo().attr->ploptions & PLO_ALTERNATION;
  if (alternation)
  { if (callstack[depth])
    { NavigateTo(NULL);
      return true;
    }
    direction = true;
  }
 next_try:
  // Optimization: ignore shuffle option for very small playlists.
  if ((unsigned)list->GetInfo().obj->num_items > 1 && IsShuffle())
  { // Prev/next in shuffle mode
    // Shuffle cache lookup
    ShuffleWorker& sw = EnsureShuffleWorker(depth);
    // Navigate
    int_ptr<PlayableInstance> pi = callstack[depth];
    pi = direction ? sw.Next(pi) : sw.Prev(pi);
    NavigateTo(pi);
  } else if (!Location::PrevNextCore(job, direction))
    return false;

  if (alternation)
  { // check whether item is valid.
    PlayableInstance* pi = callstack[depth];
    if (pi)
    { if (job.RequestInfo(*pi, IF_Tech))
        return false;
      if (pi->GetInfo().tech->attributes & TATTR_INVALID)
        goto next_try; // no, try next one
    }
  }

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
/*SongIterator& SongIterator::operator=(const SongIterator& r)
{ DEBUGLOG(("SongIterator(%p)::operator=(SongIterator&%p)\n", this, &r));
  Root = r.Root;
  Location::operator=(r);
  Options =
  return *this;
}*/

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

bool SongIterator::IsShuffle(unsigned depth) const
{ if (Options & PLO_NO_SHUFFLE)
    return false;
  const vector<PlayableInstance>& callstack = GetCallstack();
  while (depth)
  { unsigned options = callstack[--depth]->GetInfo().attr->ploptions & (PLO_SHUFFLE|PLO_NO_SHUFFLE);
    if (options)
      return !(options & PLO_NO_SHUFFLE);
  }
  return !!(Options & PLO_SHUFFLE);
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

SongIterator::OffsetCacheEntry& SongIterator::EnsureOffsetCache(size_t level)
{ ASSERT(level < GetCallstack().size());
  if (OffsetCache.size() <= level)
    OffsetCache.set_size(level +1);
  int_ptr<OffsetCacheEntry>& cep(OffsetCache[level]);
  if (!cep)
    cep = new OffsetCacheEntry(*Root, GetCallstack(), level);
  return *cep;
}

InfoFlags SongIterator::CalcOffsetCacheEntry(Job& job, unsigned level, InfoFlags what)
{ DEBUGLOG(("SongIterator(%p)::CalcOffsetCacheEntry({%x,}, %u, %x)\n", this, level, what, job.Pri));
  OffsetCacheEntry& ce = EnsureOffsetCache(level);
  // Cleanup before start
  what &= (InfoFlags)~ce.Valid;
  if (!what)
    return IF_None;
  if (what & IF_Rpl)
  { ce.Front.Rpl.Reset();
    ce.Back.Rpl.Reset();
  }
  if (what & IF_Drpl)
  { ce.Front.Drpl.Reset();
    ce.Back.Drpl.Reset();
  }
  ce.Valid |= what;
  InfoFlags whatnotok = IF_None;
  PlayableInstance* current = GetCallstack()[level];

 recurse:
  int_ptr<PlayableInstance> psp; // start element
  PM123_TIME ss = -1; // start position
  /* TODO: slicing
  if (start)
  { DEBUGLOG(("SongIterator::AddSliceAggregate start: %s\n", start->Serialize().cdata()));
    size_t size = start->GetCallstack().size();
    if (size > level)
      psp = start->GetCallstack()[level];
    else if (size == level)
    { if (job.RequestInfo(*this, IF_Tech))
        whatnotok = what;
      else if (GetInfo().tech->attributes & TATTR_SONG)
        ss = start->GetPosition();
    }
  }*/
  int_ptr<PlayableInstance> pep; // stop element
  PM123_TIME es = -1; // stop position
  /* TODO slicing
  if (stop)
  { DEBUGLOG(("APlayable::AddSliceAggregate stop: %s\n", stop->Serialize().cdata()));
    size_t size = stop->GetCallstack().size();
    if (size > level)
      pep = stop->GetCallstack()[level];
    else if (size == level)
    { if (job.RequestInfo(*this, IF_Tech))
        whatnotok = what;
      else if (GetInfo().tech->attributes & TATTR_SONG)
        es = stop->GetPosition();
    }
  }*/

  { Playable& pc = ce.Parent.GetPlayable();
    /* TODO: slicing
    if (psp)
    { // Check for negative slice
      if (pep && psp->GetIndex() > pep->GetIndex())
        goto empty;
      if (!ce.Exclude.contains(psp->GetPlayable()))
        whatnotok |= psp->AddSliceAggregate(ai, exclude, what, job, start, NULL, level+1);
      // else: item in the call stack => ignore entire sub tree.
    }
    if (pep)
    { if (!ce.Exclude.contains(pep->GetPlayable()))
        whatnotok |= pep->AddSliceAggregate(ai, exclude, what, job, NULL, stop, level+1);
      // else: item in the call stack => ignore entire sub tree.
    }*/

    // Add the range (psp..pep). Exclusive interval!
    if (job.RequestInfo(pc, IF_Child))
      return what;
    bool shuffle_checked = false;
    ShuffleWorker* swp = NULL;
    ShuffleWorker::Iterator currentloc;

    while ((psp = pc.GetNext(psp)) != pep)
    { // Always skip current item.
      if (psp == current)
        continue;
      Playable& p = psp->GetPlayable();
      if (&p == &pc || ce.Exclude.contains(p))
        continue;
      InfoFlags what2 = what;
      const volatile AggregateInfo& ai = job.RequestAggregateInfo(*psp, ce.Exclude, what2);
      whatnotok |= what2;
      // Is shuffle?
      if (!shuffle_checked)
      { shuffle_checked = true;
        if (IsShuffle(level))
        { swp = &EnsureShuffleWorker(level);
          if (current)
            currentloc = swp->GetLocation(*current);
        }
      }
      // Check whether *psp is before or after current.
      bool isbefore = current && (swp ? swp->GetLocation(*psp) < currentloc : psp->GetIndex() < current->GetIndex());
      AggregateInfo& target = (isbefore ? ce.Front : ce.Back);
      target.Add(ai, what);
    }
   empty:;
  }
  return whatnotok;
}

InfoFlags SongIterator::AddFrontAggregate(AggregateInfo& ai, InfoFlags what, Job& job)
{ DEBUGLOG(("SongIterator(%p)::AddFrontAggregate(, %x, {%x,})\n", this, what, job.Pri));
  ASSERT((what & ~IF_Aggreg) == 0);
  InfoFlags whatnotok = IF_None;
  if (what)
  { const vector<PlayableInstance>& callstack = GetCallstack();
    for (unsigned i = 0; i < callstack.size(); ++i)
    { CalcOffsetCacheEntry(job, i, what);
      ai.Add(OffsetCache[i]->Front, what);
    }
    if ((what & IF_Drpl) && GetPosition() > 0)
      ai.Drpl.totallength += GetPosition();
  }
  return whatnotok;
}

InfoFlags SongIterator::AddBackAggregate(AggregateInfo& ai, InfoFlags what, Job& job)
{ DEBUGLOG(("SongIterator(%p)::AddBackAggregate(, %x, {%x,})\n", this, what, job.Pri));
  ASSERT((what & ~IF_Aggreg) == 0);
  InfoFlags whatnotok = IF_None;
  if (what)
  { const vector<PlayableInstance>& callstack = GetCallstack();
    for (unsigned i = 0; i < callstack.size(); ++i)
    { whatnotok |= CalcOffsetCacheEntry(job, i, what);
      ai.Add(OffsetCache[i]->Back, what);
    }
    APlayable* cur = GetCurrent();
    if (cur)
    { PlayableSet exclude(callstack.size() ? callstack.size()-1 : 0);
      if (callstack.size())
      { exclude.add(*Location::GetRoot());
        for (unsigned i = 0; i < callstack.size()-1; ++i)
          exclude.add(callstack[i]->GetPlayable());
      }
      InfoFlags what2 = what;
      const volatile AggregateInfo& lai = job.RequestAggregateInfo(*cur, exclude, what2);
      whatnotok |= what2;
      ai.Add(lai, what);
      if (GetPosition() > 0 && (what & IF_Drpl))
        ai.Drpl.totallength -= GetPosition();
    }
  }
  return whatnotok;
}
