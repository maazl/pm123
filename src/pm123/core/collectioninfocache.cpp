/*
 * Copyright 2011-2011 M.Mueller
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


#include "collectioninfocache.h"
#include "playable.h"
#include <cpp/algorithm.h>
#include <string.h>

#include <debuglog.h>


/****************************************************************************
*
*  class CollectionInfo
*
****************************************************************************/

InfoFlags CollectionInfo::RequestAI(InfoFlags& what, Priority pri, Reliability rel)
{ DEBUGLOG(("CollectionInfo(%p)::RequestAI(&%x, %u, %u)\n", this, what, pri, rel));
  what = InfoStat.Check(what, rel);
  if (pri == PRI_None || (what & IF_Aggreg) == IF_None)
    return IF_None;
  return InfoStat.Request(what & IF_Aggreg, pri);
}


/****************************************************************************
*
*  class CollectionInfoCache
*
****************************************************************************/

int CollectionInfoCache::CacheEntry::compare(const PlayableSetBase& l, const CacheEntry& r)
{ return PlayableSetBase::compare(l, r);
}


Mutex CollectionInfoCache::CacheMutex;


/*void Playable::InvalidateInfo(InfoFlags what)
{ DEBUGLOG(("Playable::InvalidateInfo(%x)\n", what));
  InfoSvc.EndUpdate(what);
  what = InfoRel.Invalidate(what);
  InfoChange(PlayableChangeArgs(*this, IF_None, IF_None, what));
}

InfoFlags Playable::InvalidateInfoSync(InfoFlags what)
{ DEBUGLOG(("Playable::InvalidateInfoSync(%x)\n", what));
  what &= ~InfoSvc.IsInService();
  what = InfoRel.Invalidate(what);
  InfoChange(PlayableChangeArgs(*this, IF_None, IF_None, what));
  return what;
}*/

CollectionInfo* CollectionInfoCache::Lookup(const PlayableSetBase& exclude)
{ DEBUGLOG(("CollectionInfoCache(%p)::Lookup({%u,})\n", this, exclude.size()));
  PASSERT(this);
  // Fastpath: no cache entry for the default object.
  size_t size = exclude.size();
  if (size == 0 || (size == 1 && exclude[0] == &Parent))
    return NULL;

  // Remove current Playable from the exclude list.
  sco_ptr<PlayableSet> more;
  const PlayableSetBase* current = &exclude;
  if (exclude.contains(Parent))
  { more = new PlayableSet(size - 1);
    for (size_t i = 0; i < size; ++i)
    { Playable* pp = exclude[i];
      if (pp != &Parent)
        more->append() = pp;
    }
    current = more.get();
  }

  Mutex::Lock lock(CacheMutex);
  CacheEntry*& cic = Cache.get(*current);
  if (cic == NULL)
    cic = more != NULL ? new CacheEntry(*more) : new CacheEntry(exclude);
  return cic;
}

InfoFlags CollectionInfoCache::Invalidate(InfoFlags what, const Playable* pp)
{ DEBUGLOG(("CollectionInfoCache(%p)::Invalidate(%p)\n", this, pp));
  PASSERT(this);
  InfoFlags ret = IF_None;
  Mutex::Lock lock(CacheMutex);
  CacheEntry*const* cipp = Cache.begin();
  CacheEntry*const*const eipp = Cache.end();
  while (cipp != eipp)
  { CacheEntry* cip = *cipp++;
    if (!pp || !cip->Exclude.contains(*pp));
      ret |= cip->InfoStat.Invalidate(what);
  }
  return ret;
}

bool CollectionInfoCache::GetNextWorkItem(CollectionInfo*& item, Priority pri, InfoState::Update& upd)
{ DEBUGLOG(("CollectionInfoCache(%p{&%p})::GetNextWorkItem(%p, %u)\n", this, &Parent, item, pri));
  PASSERT(this);
  if (Cache.size()) // Fastpath without mutex
  { Mutex::Lock lck(CacheMutex);
    CacheEntry*const* cipp = Cache.begin();
    if (item)
    { size_t pos;
      RASSERT(Cache.locate(item->Exclude, pos));
      cipp += pos +1;
    }
    for (CacheEntry*const*const cepp = Cache.end(); cipp != cepp; ++cipp)
    { upd.Reset((*cipp)->InfoStat, pri);
      if (upd)
      { item = *cipp;
        return true;
      }
    }
  }
  item = NULL;
  return false;
}

void CollectionInfoCache::PeekRequest(RequestState& req) const
{ if (Cache.size()) // Fastpath without mutex
  { Mutex::Lock lck(CacheMutex);
    CacheEntry*const* cipp = Cache.begin();
    for (CacheEntry*const*const cepp = Cache.end(); cipp != cepp; ++cipp)
      (*cipp)->InfoStat.PeekRequest(req);
  }
}
