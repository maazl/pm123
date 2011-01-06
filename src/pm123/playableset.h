/*
 * Copyright 2008-2009 Marcel Mueller
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


#ifndef PLAYABLESET_H
#define PLAYABLESET_H

#include <cpp/xstring.h>
#include <cpp/container/sorted_vector.h>

class Playable;

class PlayableSetBase
: public IComparableTo<PlayableSetBase>
{public:
  static const PlayableSetBase& Empty; // empty instance

 protected:
                           PlayableSetBase() {}
                           ~PlayableSetBase() {}
 public:
  virtual size_t           size() const = 0;
  virtual Playable*        operator[](size_t where) const = 0;
  virtual bool             contains(const Playable& key) const = 0;
  
  virtual int              compareTo(const PlayableSetBase& r) const;
  // returns true if and only if all elements in this set are also in r.
  bool                     isSubsetOf(const PlayableSetBase& r) const;
};

// Unique sorted set of Playable objects
// This class does not take ownership of the Playable objects!
// So you have to ensure that the Playable objects are held by another int_ptr instance
// as long as they are in this collection.
class PlayableSet
: public sorted_vector<Playable, Playable>,
  public PlayableSetBase
{public:
                           PlayableSet();
                           PlayableSet(size_t size);
                           PlayableSet(const PlayableSetBase& r);
                           PlayableSet(const PlayableSet& r);
                           ~PlayableSet();

  virtual size_t           size() const
                           { return sorted_vector<Playable, Playable>::size(); }
  virtual Playable*        operator[](size_t where) const
                           { return sorted_vector<Playable, Playable>::operator[](where); }
  virtual bool             contains(const Playable& key) const;
          bool             add(Playable& p);
};

// Unique sorted set of Playable objects.
// The ownership of the content is held by this class.
class OwnedPlayableSet
: public sorted_vector_int<Playable, Playable>,
  public PlayableSetBase
{public:
                           OwnedPlayableSet();
                           OwnedPlayableSet(const PlayableSetBase& r);
                           OwnedPlayableSet(const OwnedPlayableSet& r);
                           ~OwnedPlayableSet();

  virtual size_t           size() const
                           { return sorted_vector_int<Playable, Playable>::size(); }
  virtual Playable*        operator[](size_t where) const
                           { return sorted_vector_int<Playable, Playable>::operator[](where); }
  virtual bool             contains(const Playable& key) const;
          bool             add(Playable& p);
};

#endif

