/*
 * Copyright 2008-2011 M.Mueller
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


#include "playableset.h"
#include "playable.h"

#include <debuglog.h>


/****************************************************************************
*
*  class PlayableSet
*
****************************************************************************/

class EmptyPlayableSet : public PlayableSetBase
{ virtual size_t           size() const;
  virtual Playable*        operator[](size_t where) const;
  virtual bool             contains(const Playable& key) const;
};

size_t EmptyPlayableSet::size() const
{ return 0;
}

Playable* EmptyPlayableSet::operator[](size_t) const
{ // Must not call this function!
  ASSERT(1==0);
  return NULL;
}

bool EmptyPlayableSet::contains(const Playable&) const
{ return false;
}

static const EmptyPlayableSet EmptySet;
const PlayableSetBase& PlayableSetBase::Empty(EmptySet);

int PlayableSetBase::compare(const PlayableSetBase& l, const PlayableSetBase& r)
{ DEBUGLOG(("PlayableSetBase::compare(&%p{%u,...}, &%p{%u,...})\n", &l, l.size(), &r, r.size()));
  if (&l == &r)
    return 0; // Comparison to itself
  size_t p1 = 0;
  size_t p2 = 0;
  for (;;)
  { // termination condition
    if (p2 == r.size())
      return p1 != l.size();
    else if (p1 == l.size())
      return -1;
    // compare content
    int ret = CompareInstance(*l[p1], *r[p2]);
    DEBUGLOG2(("PlayableSetBase::compareTo %p <=> %p = %i\n", l[p1], r[p2], ret));
    if (ret)
      return ret;
    ++p1;
    ++p2;
  }
}

bool PlayableSetBase::isSubsetOf(const PlayableSetBase& r) const
{ DEBUGLOG(("PlayableSetBase(%p{%u,...})::isSubsetOf(&%p{%u,...})\n", this, size(), &r, r.size()));
  if (&r == this)
    return true;
  if (size() > r.size())
    return false; // since PlayableSet is a unique container this condition is sufficient
  if (size() == 0)
    return true; // an empty set is always included
  size_t p1 = 0;
  size_t p2 = 0;
  for (;;)
  { // compare content
    int ret = CompareInstance(*(*this)[p1], *r[p2]);
    DEBUGLOG2(("PlayableSetBase::isSubsetOf %p <=> %p = %i\n", (*this)[p1], r[p2], ret));
    if (ret > 0)
      return false; // no match for **ppp1
    ++p2;
    if (p2 == r.size())
      return false; // no match for **ppp1 because no more elements in r
    if (ret < 0)
      continue; // only increment ppp2
    ++p1;
    if (p1 == size())
      return true; // all elements found
  }
}

#ifdef DEBUG_LOG
xstring PlayableSetBase::DebugDump() const
{ xstringbuilder sb;
  for (size_t i = 0; i < size(); ++i)
  { if (i)
      sb.append(',');
    sb.appendf("%p", (*this)[i]);
  }
  return sb;
}
#endif


PlayableSet::PlayableSet(size_t size)
: sorted_vector<Playable, Playable, &CompareInstance<Playable> >(size)
{ DEBUGLOG(("PlayableSet(%p)::PlayableSet(%u)\n", this, size));
}
PlayableSet::PlayableSet(const PlayableSetBase& r)
: sorted_vector<Playable, Playable, &CompareInstance<Playable> >(r.size())
{ DEBUGLOG(("PlayableSet(%p)::PlayableSet(const PlayableSetBase&{%u...})\n", this, r.size()));
  for (size_t i = 0; i != r.size(); ++i)
    append() = r[i];
}
PlayableSet::PlayableSet(const PlayableSet& r)
: sorted_vector<Playable, Playable, &CompareInstance<Playable> >(r)
{ DEBUGLOG(("PlayableSet(%p)::PlayableSet(const PlayableSet&{%u...})\n", this, r.size()));
}

PlayableSet::~PlayableSet()
{ DEBUGLOG(("PlayableSet(%p)::~PlayableSet()\n", this));
}

bool PlayableSet::contains(const Playable& key) const
{ return sorted_vector<Playable, Playable, &CompareInstance<Playable> >::find(key) != NULL;
}

bool PlayableSet::add(Playable& p)
{ Playable*& pr = get(p);
  if (pr)
    return false;
  pr = &p;
  return true;
}


OwnedPlayableSet::OwnedPlayableSet()
: sorted_vector_int<Playable, Playable, &CompareInstance<Playable> >(8)
{ DEBUGLOG(("OwnedPlayableSet(%p)::OwnedPlayableSet()\n", this));
}
OwnedPlayableSet::OwnedPlayableSet(const PlayableSetBase& r)
: sorted_vector_int<Playable, Playable, &CompareInstance<Playable> >(r.size())
{ DEBUGLOG(("OwnedPlayableSet(%p)::OwnedPlayableSet(const PlayableSetBase&{%u...})\n", this, r.size()));
  for (size_t i = 0; i != r.size(); ++i)
    append() = r[i];
}
OwnedPlayableSet::OwnedPlayableSet(const OwnedPlayableSet& r)
: sorted_vector_int<Playable, Playable, &CompareInstance<Playable> >(r)
{ DEBUGLOG(("OwnedPlayableSet(%p)::OwnedPlayableSet(const OwnedPlayableSet&{%u...})\n", this, r.size()));
}

OwnedPlayableSet::~OwnedPlayableSet()
{ DEBUGLOG(("OwnedPlayableSet(%p)::~OwnedPlayableSet()\n", this));
}

bool OwnedPlayableSet::contains(const Playable& key) const
{ return sorted_vector_int<Playable, Playable, &CompareInstance<Playable> >::find(key) != NULL;
}

bool OwnedPlayableSet::add(Playable& p)
{ int_ptr<Playable>& pr = get(p);
  if (pr)
    return false;
  pr = &p;
  return true;
}

