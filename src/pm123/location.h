/*
 * Copyright 2007-2009 M.Mueller
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


#ifndef LOCATION_H
#define LOCATION_H

#include <format.h>
#include "aplayable.h"

#include <limits.h>
#include <cpp/container/vector.h>


class PlayableInstance;
class Playable;
/* Class to identify a deep location within a playlist or a song.
 * Note that a Location does not own its root object.
 * So you have to ensure that no Location can live longer than the referenced root.
 * But it owns the items in the callstack. So they can be safely removed from the collection
 * while the location exists.
 */
class Location : public Iref_count
{public:
  // Result of Navigation command
  // It can take three logically distinct values:
  //  - NULL      => Successful.
  //  - ""        => Further information from other objects is required. Retry later.
  //  - any other => Not successful because of a syntax error or a non-existing
  //                 or out of bounds reference. Error Message as plan text.
  typedef xstring             NavigationResult;
 protected:
  typedef int_ptr<PlayableInstance> (Playable::*DirFunc)(const PlayableInstance*) const;


 private:
  Playable*                   Root;
  vector_int<PlayableInstance> Callstack;
  PM123_TIME                  Position;
  #ifdef DEBUG
 private:
  class_delegate<Location, const PlayableChangeArgs> RootChangeDeleg;
 private:
  void                        RootChange(const PlayableChangeArgs& args); 
  #endif
  // Recursive iteration over *this.
  // The function returns only items that match stopat.
  // The iteration stops with an error when the callstack comes to level slice (default top).
  // In this case "End" is returned.
  NavigationResult            PrevNextCore(DirFunc dirfunc, TECH_ATTRIBUTES stopat, Priority pri, unsigned slice = 0);
  // Assign new root (Location must be initial before)
  #ifdef DEBUG
  void                        AssignRoot(Playable* root);
  #else
  void                        AssignRoot(Playable* root)   { Root = root; }
  #endif
 
 public:
  explicit                    Location(Playable* root = NULL)
                              : Position(0)
  #ifdef DEBUG
                              , RootChangeDeleg(*this, &Location::RootChange)
  #endif
                              { DEBUGLOG(("Location(%p)::Location(%p)\n", this, root)); AssignRoot(root); }
                              Location(const Location& r)
                              : Callstack(r.Callstack)
                              , Position(r.Position)
  #ifdef DEBUG
                              , RootChangeDeleg(*this, &Location::RootChange)
  #endif
                              { DEBUGLOG(("Location(%p)::Location(%p&{%p})\n", this, &r, r.Root)); AssignRoot(r.Root); }
  #ifdef DEBUG
                              ~Location()                  { DEBUGLOG(("Location(%p)::~Location()\n", this)); }
  #endif
  bool                        operator!() const            { return !Root; }
  // Swap two instances.
  void                        Swap(Location& r);
  // Resets the current Location to its initial value at the start of the current root.
  void                        Reset();
  // Assign instance
  Location&                   operator=(const Location& r);
  // Assign a new root object. This implicitly resets the location. 
  void                        SetRoot(Playable* root)      { Reset(); AssignRoot(root); }
  Playable*                   GetRoot() const              { return Root; }
  APlayable&                  GetCurrent() const;
  const vector_int<PlayableInstance>& GetCallstack() const { return Callstack; }
  PM123_TIME                  GetPosition() const          { return Position; }
  // Depth of the current location.
  // 0 = At the root.
  // 1 = Root is a playlist and we are at an immediate sub item of this list.
  // 2 = We are in a nested playlist item.
  // ...
  size_t                      GetLevel() const             { return Callstack.size(); }

  // Check whether a Playable object is in the call stack. Not thread-safe!
  bool                        IsInCallstack(const Playable* pp) const;

  // Navigate Count Items that match stopat forward or backward (count < 0).
  // return NULL on success,
  // "" if required information is requested and you should retry later,
  // "End" if the end of the list is reached and
  // anything else on error.
  NavigationResult            NavigateCount(int count, TECH_ATTRIBUTES stopat, Priority pri, unsigned slice = 0);
  // Navigate to the parent index times.
  // Of course, you cannot navigate up from the root.
  NavigationResult            NavigateUp(int index = 1);
  // Navigate explicitely to a given PlayableInstance.
  // The PlayableInstance must be a child of the current item.
  // NavigateTo will never depend on outstanding information and return "".
  // This can be used to explicitely build the callstack.
  NavigationResult            NavigateTo(PlayableInstance& pi);
  // Navigate to url[index] within the current deepmost playlist.
  // If the addressed item is a playlist the list is entered implicitely.
  // If the url is '..' the current playlist is left.
  // If the url is NULL only the index is used.
  // A negative index counts from the back. The index must not be 0, otherwise navigation fails.
  // If flat is true all content from the current location is flattened.
  // So the Navigation goes to the item in the set of non-recursive items in the current playlist
  // and all sublists that matches url/index.
  // If Navigate with flat = true succeeds, the current item is always a song.
  NavigationResult            Navigate(const xstring& url, int index, bool flat, Priority pri);
  // Set the current location and song as time offset.
  // If the offset is less than zero it counts from the back.
  // The Navigation starts from the current location.
  // If this is a song, the call is similar to SetLocation().
  // If current is a playlist the navigation applies to the whole list content.
  // If you want to Navigate within the root call Reset before.
  // If NavigateTime succeeds, the current item is always a song.
  NavigationResult            Navigate(PM123_TIME offset, Priority pri);
  
  // Serialize the iterator into a string.
  xstring                     Serialize(bool withpos = true) const;
  // Deserialize the current instance from a string and return a error message (if any).
  // str may be relative. To enforce an absolute location call Reset() before.
  // The current root must be consistent with the iterator string otherwise
  // the function signals an error. In case of an error a partial navigation may take place.
  // str is advanced to the current read location. This can be used to localize errors.
  // If the return value is DSR_Delayed then requests have been placed on other objects
  // at the priority level pri and you have to retry the deserialization later.
  // Note that the value of *this is undefined in this case and must restore
  // the previous value in case of relative navigation.
  // If pri is PRI_Sync Deserialize will never return DSR_Delayed but wait for the information
  // to become available instead. This may take quite long on large recursive playlists.
  NavigationResult            Deserialize(const char*& str, Priority pri);

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
  int                         CompareTo(const Location& r, unsigned level = 0, bool withpos = true) const;
};

// for convenience
inline bool operator==(const Location& l, const Location& r)
{ return l.CompareTo(r) == 0;
}

inline bool operator!=(const Location& l, const Location& r)
{ return l.CompareTo(r) != 0;
}

inline bool operator<(const Location& l, const Location& r)
{ int ret = r.CompareTo(l);
  ASSERT(ret != INT_MIN);
  return 0 < ret;
}

inline bool operator<=(const Location& l, const Location& r)
{ int ret = r.CompareTo(l);
  ASSERT(ret != INT_MIN);
  return 0 <= ret;
}

inline bool operator>(const Location& l, const Location& r)
{ int ret = l.CompareTo(r);
  ASSERT(ret != INT_MIN);
  return ret > 0;
}

inline bool operator>=(const Location& l, const Location& r)
{ int ret = l.CompareTo(r);
  ASSERT(ret != INT_MIN);
  return ret >= 0;
}


/* A SongIterator is a Location intended to be used to play a Playable object.
 * In Contrast to Location it owns it's root.
 * The class is non-polymorphic. You must not change the root with Location::
 * functions like SetRoot, Swap, assignment.  
 */
class SongIterator : public Location
{ // Implementation note:
  // Strictly speaking SongIterator should have a class member of type
  // int_ptr<Playable> to keep the reference to the root object alive.
  // To save this extra member the functionality is emulated by the
  // C-API interface of int_ptr<>.
 public:
  explicit                    SongIterator(Playable* root = NULL)
                              : Location(root)
                              { // Increment reference counter
                                int_ptr<Playable> ptr(GetRoot());
                                ptr.toCptr();
                              }
                              SongIterator(const SongIterator& r)
                              : Location(r)
                              { // Increment reference counter
                                int_ptr<Playable> ptr(GetRoot());
                                ptr.toCptr();
                              }
                              ~SongIterator()
                              { int_ptr<Playable>().fromCptr(GetRoot());
                              }
  void                        SetRoot(Playable* root);
  SongIterator&               operator=(const SongIterator& r);
};

#endif
