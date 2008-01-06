/* * Copyright 2007-2007 M.Mueller
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

#ifndef PLAYITERATOR_H
#define PLAYITERATOR_H

#include "playable.h"

#include <cpp/smartptr.h>
#include <cpp/event.h>


/* Abstract class to handle recursive enumeration over playable objects
 */
class RecursiveEnumerator
{
 protected:
  RecursiveEnumerator* const   Parent;
  // state vars
  // Valid states:
  // Root   SubIterator Valid  State
  //  NULL       X      false  unassigned
  // !NULL      NULL    false  Song assigned, before start or at the end
  // !NULL      NULL    true   Song assigned, at the song
  // !NULL     !NULL    false  PlayableCollection assigned, before start or at the end
  // !NULL   ->Current  true   PlayableCollection assigned, at an item
  int_ptr<Playable>            Root;
  sco_ptr<RecursiveEnumerator> SubIterator;
  bool                         Valid;
  int_ptr<PlayableInstance>    Current;

 protected:
  RecursiveEnumerator(RecursiveEnumerator* parent = NULL);
  // Factory Function to initialize SubIterator
  virtual void                 InitNextLevel() = 0;
  // Fetch the previous element from the PlayableCollection, even if the current object already died.
  // The Function must be called while the underlying list is locked.
  virtual void                 PrevEnum();
  // Fetch the next element from the PlayableCollection, even if the current object already died.
  // The Function must be called while the underlying list is locked.
  virtual void                 NextEnum();
  // Check whether item is already in the call stack.
  virtual RecursiveEnumerator* RecursionCheck(const Playable* item);
 public:
  virtual ~RecursiveEnumerator() {}
  // Set the object to enumerate
  virtual void                 Attach(Playable* play);
  // Get currently attached object
  Playable*                    GetRoot() const   { return Root; }
  // Get parent enumerator (from constructor)
  RecursiveEnumerator*         GetParent() const { return Parent; }
  // Reset the current enumerator
  virtual void                 Reset();
};

/* Class to handle the iteration over a playable object
 * The interface of this class is not thread-safe.
 * However, it provides a thread-safe way to iterate over a tree of Playable objects
 * even if the collections are modified asynchronuously. If an item is removed from the
 * collection while it is already in use it will be played completly including it's subitems
 * as if it had been removed after playback.
 * Furthermore this class prevents infinite recursions by disallowing items
 * that are already in the call stack.
 */
class PlayEnumerator : public RecursiveEnumerator
{public:
  // Current iterator status
  struct Status
  { int    CurrentItem;   // index of the currently played song
    int    TotalItems;    // total number of items in the queue
    double CurrentTime;   // current time index from the start of the queue excluding the currently loaded one
    double TotalTime;     // total time of all items in the queue
  };

 private:
  // Core logic to Prev() and Next()
  int_ptr<Song>     PrevNextCore(int_ptr<Song> (PlayEnumerator::*subfn)(), void (PlayEnumerator::*enumfn)());
 protected:
  // Factory Function to initialize SubIterator
  virtual void      InitNextLevel();
 public:
  PlayEnumerator(PlayEnumerator* parent = NULL) : RecursiveEnumerator(parent) {}

  Status            GetStatus() const;

  // Get the current song.
  int_ptr<Song>     GetCurrentSong() const;
  // Check whether the enumerator is dereferencable
  // This implies GetCurrentSong() != NULL, but it is faster.
  bool              IsValid() const { return Valid; }

  int_ptr<Song>     Prev()          { return PrevNextCore(&PlayEnumerator::Prev, &PlayEnumerator::PrevEnum); }
  int_ptr<Song>     Next()          { return PrevNextCore(&PlayEnumerator::Next, &PlayEnumerator::NextEnum); }

  //virtual PlayEnumerator* Clone() const;
};

/* This class enhances PlayEnumerator by the feature to set the status
   of all item in the current call stack to InUse.
 */
class StatusPlayEnumerator : public PlayEnumerator
{
 protected:
  // Factory Function to initialize SubIterator
  virtual void      InitNextLevel();
  // overload: update status
  virtual void      PrevEnum();
  // overload: update status
  virtual void      NextEnum();
 public:
  StatusPlayEnumerator(StatusPlayEnumerator* parent = NULL) : PlayEnumerator(parent) {}
  // overload: update status
  virtual void      Reset();
  // overload: update status
  virtual void      Attach(Playable* play);
};

#endif
