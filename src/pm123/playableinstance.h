/*  
 * Copyright 2007-2011 Marcel Müller
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


#ifndef PLAYABLEINSTANCE_H
#define PLAYABLEINSTANCE_H

#include "playableref.h"


/** @brief Instance of a PlayableRef within a laylist.
 *
 * Calling non-constant methods unsynchronized will not cause the application
 * to have undefined behavior. It will only cause the PlayableInstance to have a
 * non-deterministic but valid state.
 * The relationship of a PlayableInstance to it's parent container is weak.
 * That means that the PlayableInstance may be detached from the container before it dies.
 * A detached PlayableInstance will die as soon as it is no longer used. It may not be
 * reattached to a container.
 * That also means that there is no way to get a reference to the actual parent,
 * because once you got the reference it might immediately get invalid.
 * And to avoid this, the parent must be locked, which is impossible to ensure, of course.
 * All you can do is to verify whether the current PlayableInstance belongs to a given parent.
 */
class PlayableInstance : public PlayableRef
{public:
  typedef int (*Comparer)(const PlayableInstance& l, const PlayableInstance& r);

 protected:
  /// Weak reference to the parent Collection.
  const Playable* volatile Parent;
  /// Index within the collection.
  int                      Index;

 private: // non-copyable
  PlayableInstance(const PlayableInstance& r);
  void operator=(const PlayableInstance& r);
 protected:
  PlayableInstance(const Playable& parent, APlayable& refto);
 public:
  //virtual                  ~PlayableInstance() {}
  /// Check if this instance (still) belongs to a collection.
  /// The return value true is not reliable unless the collection is locked.
  /// Calling this method with NULL will check whether the instance does no longer belong to a collection.
  /// In this case only the return value true is reliable.
  #ifdef DEBUG_LOG
  virtual bool             HasParent(const Playable* parent) const
                           { if (Parent == parent)
                               return true;
                             DEBUGLOG(("PlayableInstnace(%p)::HasParent(%p) - %p\n", this, parent, Parent));
                             return false;
                           }
  #else
  virtual bool             HasParent(const Playable* parent) const
                           { return Parent == parent; }
  #endif

  /// Returns the index within the current collection counting from 1.
  /// A return value of 0 indicates the the instance does no longer belong to a collection.
  /// The return value is only reliable while the collection is locked.
  int                      GetIndex() const    { return Index; }

  /*// Swap instance properties (fast)
  // You must not swap instances that belong to different PlayableCollection objects or
  // refer to different objects.
  // The collection must be locked when calling Swap.
  // Swap does not swap the current status.
          void             Swap(PlayableRef& r);*/

  // Compare two PlayableInstance objects by value.
  // Two instances are equal if on only if they belong to the same PlayableCollection,
  // share the same Playable object (=URL) and have the same properties values for alias and slice.
  //friend bool              operator==(const PlayableInstance& l, const PlayableInstance& r);

  // Return the logical ordering of two Playableinstances.
  // 0       = equal
  // <0      = l before r
  // >0      = l after r
  // INT_MIN = unordered (The PlayableInstances do not belong to the same collection.)
  // Note that the result is not reliable unless you hold the mutex of the parent.
  //int                      CompareTo(const PlayableInstance& r) const;
 private:
  virtual xstring           DoDebugName() const;
};


#endif

