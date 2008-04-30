/*
 * Copyright 2007-2008 M.Mueller
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

#include <format.h>
#include "playable.h"
#include "playablecollection.h"

#include <cpp/container.h>
#include <cpp/xstring.h>

#include <stdlib.h>


/* Class to iterate over a PlayableCollection recursively returning only Song items.
 * All functions are not thread safe.
 */
class SongIterator
{public:
  struct Offsets
  { int                       Index;     // Item index of the current PlayableInstance counting from the top level.
    T_TIME                    Offset;    // Time offset of the current PlayableInstance counting from the top level or -1 if not available.
    Offsets(int index, T_TIME offset) : Index(index), Offset(offset) {}
    void                      Add(const Offsets& r);
    void                      Sub(const Offsets& r);
  };
  struct CallstackEntry : public Offsets
  { int_ptr<PlayableSlice>    Item;
    CallstackEntry()          : Offsets(1, 0) {}
    CallstackEntry(const Offsets& r) : Offsets(r) {}
  };
  typedef vector<CallstackEntry> CallstackType;
  
 private:
  CallstackType               Callstack; // Current callstack, including the top level playlist.
                                         // This collection contains only enumerable objects except for the last entry.
                                         // The callstack has at least one element.
  PlayableSet                 Exclude;   // List of playable objects to exclude.
                                         // This are in fact the enumerable objects from Callstack.
  T_TIME                      Location;  // Location within the current song
  static const Offsets        ZeroOffsets; // Offsets for root entry.

 protected: // internal functions, not thread-safe.
  // Push the current item to the call stack.
  // Precondition: Current is Enumerable.
  void                        Enter();
  // Pop the last object from the stack and make it to Current.
  void                        Leave();
  // Fetch length and number of subitems from a Playable object with respect to Exclude.
  Offsets                     TechFromPlayable(PlayableCollection* pc);
  Offsets                     TechFromPlayable(Playable* pp);
  // Navigate to the previous item im the current sublist, check start and stop offsets. 
  void                        PrevCore();
  // Navigate to the previous item im the current sublist, check start and stop offsets. 
  void                        NextCore();
  // Implementation of Prev and Next.
  // dir must be -1 for backward direction and +1 for forward.
  bool                        PrevNextCore(void (SongIterator::*func)());
  // Navigate to url[index] within the current deepmost playlist.
  // If the addressed item is a playlist the list is entered implicitely.
  // If the url is '..' the current playlist is left.
  // If the url is NULL only the index is used.
  // A negative index counts from the back. The index must not be 0.
  bool                        Navigate(const xstring& url, int index);

 public:
  // Create a SongIterator for iteration over a PlayableCollection.
                              SongIterator();
  // Copy constructor                              
                              SongIterator(const SongIterator& r);
  // Cleanup (removes all strong references)
                              ~SongIterator();
  // swap two instances (fast)
  void                        Swap(SongIterator& r);
  /*#ifdef DEBUG
  void                        DebugDump() const;
  #endif*/  
  
  // Replace the attached root object. This resets the iterator.
  // You may explicitly set root to NULL to invalidate the iterator.
  void                        SetRoot(PlayableSlice* pp);
  // Gets the current root.
  PlayableSlice*              GetRoot() const          { return Callstack[0]->Item; }
  // Get the current song. This may be NULL.  
  PlayableSlice*              GetCurrent() const       { return Callstack[Callstack.size()-1]->Item; }
  // Returns the depth of the current Instance. This is zero if no root is assigned,
  // 1 if the the current item is the root, 2 if root is a playlist and the current
  // item ist in this playlist and more if the current item is in a nested playlist.
  int                         GetLevel() const;
  // Gets the deepest playlist. This may be the root if the callstack is empty.
  // If no playlist is selected the function returns NULL.
  PlayableCollection*         GetList() const;
  // Get Offsets of current item
  Offsets                     GetOffset() const;
  // Gets the call stack. Not thread-safe!
  // The callstack will always be empty if the Root is not enumerable.
  // Otherwise it will contain at least one element.
  const CallstackType&        GetCallstack() const     { return Callstack; }
  // Gets the excluded entries. Not thread-safe!
  // This is a sorted list of all playlists in the callstack.
  const PlayableSet&          GetExclude() const       { return Exclude; }
  // Check whether a Playable object is in the call stack. Not thread-safe!
  // This will return always false if the Playable object is not enumerable.
  bool                        IsInCallstack(const Playable* pp) const { return Exclude.find(*pp) != NULL; }

  // Go to the previous Song.
  bool                        Prev()                   { return PrevNextCore(&SongIterator::PrevCore); }
  // Go to the next Song.
  bool                        Next()                   { return PrevNextCore(&SongIterator::NextCore); }
  // reset the Iterator to it's initial state. 
  void                        Reset();

  T_TIME                      GetLocation() const      { return Location; }
  void                        SetLocation(T_TIME loc)  { Location = loc; }

  // Serialize the iterator into a string.
  xstring                     Serialize(bool withlocation) const;
  // Deserialize the current instance from a string.
  // The current root must have been set before and it must be consistent with the iterator string
  // otherwise the function returns false. In case of an error a partial navigation may take place.
  // To enforce an absolute location you must call Reset() before.
  bool                        Deserialize(const char*& str);
  
  // relational comparsion
  // The return value is zero if the iterators are equal.
  // The return value is less than zero if the current iterator is less than r.
  // The return value is greater than zero if the current iterator is greater than r.
  // The absolute value of the returned int is the level where the two iterators are different.
  // E.g.: a return value of 2 means that r is greater than *this in a sublist that is part of the current root.
  // The returned absolute may exceed the current level of the SongIterator by one
  // if both SongIterators address the same song and callstack but at a different location. 
  // A return value of INT_MIN means that the two iterators differ in their root node.
  // In this case they are unordered.
  // The optional level parameter compares r to a slice of *this starting at level.
  // I.e. level = 2 means that the root of r is compared against level 2 of *this.
  // level must be less than the current depth of *this.
  // The absolute return value will not be less than level except for a equal condition.
  // The current thread must own both SongIterators.
  int                         CompareTo(const SongIterator& r, int level = 0) const;
};

// Now we can include the implementation of the above predeclared classes.
#include <playablecollection.h> 

// inline functions
/*inline SongIterator::CallstackEntry::CallstackEntry()
: Offsets(-1, Offset = -1)
{}
inline SongIterator::CallstackEntry::~CallstackEntry()
{}*/

inline bool operator<(const SongIterator& l, const SongIterator& r)
{ return l.CompareTo(r) < 0;
}
inline bool operator<=(const SongIterator& l, const SongIterator& r)
{ return l.CompareTo(r) <= 0;
}
inline bool operator>(const SongIterator& l, const SongIterator& r)
{ return l.CompareTo(r) > 0;
}
inline bool operator>=(const SongIterator& l, const SongIterator& r)
{ return l.CompareTo(r) >= 0;
}

#endif
