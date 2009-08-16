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

#include <cpp/container/vector.h>
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
    Offsets()                         : Index(0), Offset(0) {}
    Offsets(int index, T_TIME offset) : Index(index), Offset(offset) {}
    void                      Add(int index, T_TIME offset);
    void                      operator+=(const Offsets& r) { Add(r.Index, r.Offset); }
    void                      operator+=(const PlayableCollection::CollectionInfo& r) { Add(r.Items, r.Songlength); }
    void                      Sub(int index, T_TIME offset);
    void                      operator-=(const PlayableCollection::CollectionInfo& r) { Sub(r.Items, r.Songlength); }
    void                      Reset() { Index = 0; Offset = 0; }
  };
  class CallstackEntry
  {public:
    int_ptr<PlayableSlice>    Item;
    Offsets                   Off;       // Offset of Item within parent and [Start, Stop)
    volatile bool             OffValid;
    sco_ptr<SongIterator>     OverrideStart;
    sco_ptr<SongIterator>     OverrideStop;
   protected:
    PlayableSet               Exclude;   // List of playable objects to exclude.
                                         // This are in fact the enumerable objects from Callstack objects above this.
   protected:
    // For CallstackSubEntry
                              CallstackEntry(const PlayableSet& exc);
   public:
                              CallstackEntry()         {}
                              CallstackEntry(const CallstackEntry& r);
    virtual                   ~CallstackEntry()        {}
    // Swap two CallstackEntry instances. This is fast and causes no reallocation.
    // Remember that Swap changes the content but not the registered event handlers!
    void                      Swap(CallstackEntry& r);
    // Resets the current CallstackEntry to its initial value.
    // This does not clear the exclude list.
    void                      Reset();
    // Assign a new root to a callstack entry.
    // In contrast to Reset() this also assigns exclude.
    void                      Assign(PlayableSlice* pc, Playable* exclude = NULL);
    // Return the exclude list associated to this instance.
    // This list is only modified by the constructor and by Assign().
    const PlayableSet&        GetExclude() const       { return Exclude; }
    // Gets the effective start location.
    // This location is the intersection of all start pointers of this and all parent CallstackEntry items.
    // The function returns NULL if the start is not restricted.
    const SongIterator*       GetStart() const         { return OverrideStart != NULL ? OverrideStart.get() : Item->GetStart(); }
    // Gets the effective stop location.
    // This location is the intersection of all stop pointers of this and all parent CallstackEntry items.
    // The function returns NULL if there is no early stop location.
    const SongIterator*       GetStop() const          { return OverrideStop != NULL ? OverrideStop.get() : Item->GetStop(); }
    // Return summary information on the current callstack entry.
    // This is similar to PlayableCollection::GetCollectionInfo but it takes care of GetStart() and GetStop().
    // Furthermore there is no calculation of the total file size.
    PlayableCollection::CollectionInfo GetInfo() const;
    // Checks whether the current item is in the exclude list.
    bool                      IsExcluded() const       { ASSERT(Item); return Exclude.find(*Item->GetPlayable()) != NULL; }
  };
  // Type of the Callstack collection.
  // This is a vector_own with an intrusive reference count.
  // vector_own destroys the objects from back to front. This is required because the callstack entry objects contain delegates
  // that rely on the parent playlist. This is always held by one entry before the current.
  class CallstackType;
  friend class CallstackType;
  class CallstackSubEntry;
  friend class CallstackSubEntry;
  class CallstackType : public Iref_count, public vector_own<CallstackEntry>
  { friend class CallstackSubEntry;
   private:
    // Event when a open callstackentry changes
    void                      ListChangeHandler(const PlayableCollection::change_args& args, CallstackEntry* cep);
   public:
    CallstackType(size_t capacity) : vector_own<CallstackEntry>(capacity)
    { DEBUGLOG(("SongIterator::CallstackType(%p)::CallstackType(%u)\n", this, capacity)); }
    CallstackType(const CallstackType& r);
    ~CallstackType()
    { DEBUGLOG(("SongIterator::CallstackType(%p)::~CallstackType() %u\n", this, size())); }
    // This event is signalled whenever the offset of a callstack entry is invalidated.
    event<const CallstackEntry> Change;
  };
 private:
  // Subentries in the callstack.
  class CallstackSubFactory : public CallstackEntry
  {protected:
    CallstackSubFactory(const CallstackSubFactory& r) : CallstackEntry(r) {}
   public:
    // Create Callstackentry for the next deeper level
    // Precondition: Item is not NULL && Item->GetPlayable() is enumerable.
    CallstackSubFactory(const CallstackEntry& parent);
  };
  class CallstackSubEntry : public CallstackSubFactory
  {private:
    class_delegate2<CallstackType, const PlayableCollection::change_args, CallstackEntry> ChangeDeleg;
   public:
    CallstackSubEntry(const CallstackSubEntry& r, CallstackType& owner);
    // Create Callstackentry for the next deeper level
    // Precondition: Item is not NULL && Item->GetPlayable() is enumerable.
    CallstackSubEntry(CallstackType& owner, const CallstackEntry& parent);
    virtual                   ~CallstackSubEntry()
    { DEBUGLOG(("SongIterator::CallstackSubEntry(%p)::~CallstackSubEntry()\n", this)); }
  };
  // Thhis type is only to have a static constructor.
  struct InitialCallstackType : public CallstackType
  { InitialCallstackType();
  };
  
 private:
  int_ptr<CallstackType>      Callstack; // Current callstack, including the top level playlist.
                                         // This collection contains only enumerable objects except for the last entry.
                                         // The callstack has at least one element. The first element is always of type CallstackEntry.
                                         // All further elements (if any) are always of type CallstackSubEntry.
  T_TIME                      Location;  // Location within the current song
  int_ptr<PlayableSlice>      CurrentCache; // mutable! Points either to CurrentCacheRef or to CurrentCacheInst or is NULL.
  bool                        CurrentCacheValid;
  static InitialCallstackType InitialCallstack; // Initial Callstack of a newly created Item
  class_delegate<SongIterator, const CallstackEntry> CallstackDeleg;

 private: // internal functions, not thread-safe.
  // Ensures that only this instance owns the callstack.
  // This function must always be called before doing any modifications to the Callstack.
  // It is the essential part of the copy on write semantic.
  void                        MakeCallstackUnique();
  void                        InvalidateCurrentCache() { CurrentCacheValid = false; }
  // Get the current item. This may be NULL if and only if no root is assigned.  
  PlayableSlice*              Current() const          { return (*Callstack)[Callstack->size()-1]->Item; }
  // Push the current item to the call stack.
  // Precondition: Current is Enumerable.
  void                        Enter();
  // Pop the last object from the stack and make it to Current.
  void                        Leave();
  // Navigate to the previous item im the current sublist, check start and stop offsets. 
  void                        PrevCore();
  // Navigate to the previous item im the current sublist, check start and stop offsets. 
  void                        NextCore();
  // Implementation of Prev and Next. Pass &PrevCore or &NextCore.
  // The iteration stops when the callstack comes to level slice.
  bool                        PrevNextCore(void (SongIterator::*func)(), unsigned slice = 0);
  // Recalculate offset if lost
  static void                 EnsureOffset(CallstackEntry& ce, PlayableCollection& parent);
  // Event when a open callstackentry changes
  void                        CallstackChangeHandler(const CallstackEntry& ce);

 public:
  // Create a SongIterator for iteration over a PlayableCollection.
                              SongIterator();
  // Copy constructor                              
                              SongIterator(const SongIterator& r);
  // Generate slice of a Callstack
                              SongIterator(const SongIterator& r, unsigned slice);
  // Cleanup (removes all strong references)
                              ~SongIterator();
  // assignment
  SongIterator&               operator=(const SongIterator& r);                              
  // swap two instances (fast)
  void                        Swap(SongIterator& r);
  #ifdef DEBUG_LOG
  static xstring              DebugName(const SongIterator* sip);
  #endif
  
  // Replace the attached root object. This resets the iterator.
  // You may explicitly set root to NULL to invalidate the iterator.
  void                        SetRoot(PlayableSlice* pp, Playable* exclude = NULL);
  // Gets the current root.
  PlayableSlice*              GetRoot() const          { return (*Callstack)[0]->Item; }
  // Get the current song. This may be NULL.
  Playable*                   GetCurrentItem() const   { PlayableSlice* ps = Current(); return ps ? ps->GetPlayable() : NULL; }
  // Get the current song slice. This may be NULL.  
  PlayableSlice*              GetCurrent() const;
  // Returns the depth of the current Instance. This is zero if no root is assigned,
  // 1 if the the current item is the root, 2 if root is a playlist and the current
  // item ist in this playlist and more if the current item is in a nested playlist.
  int                         GetLevel() const;
  // Gets the deepest playlist. This may be the root if the callstack is empty.
  // If no playlist is selected the function returns NULL.
  PlayableCollection*         GetList() const;
  // Gets the call stack. Not thread-safe!
  // The callstack will always be empty if the Root is not enumerable.
  // Otherwise it will contain at least one element.
  const CallstackType&        GetCallstack() const     { return *Callstack; }
  // Gets the excluded entries. Not thread-safe!
  // This is a sorted list of all playlists in the callstack.
  const PlayableSet&          GetExclude() const       { return (*Callstack)[Callstack->size()-1]->GetExclude(); }
  // Check whether a Playable object is in the call stack. Not thread-safe!
  // This will return always false if the Playable object is not enumerable.
  bool                        IsInCallstack(const Playable* pp) const { return GetExclude().find(*pp) != NULL; }

  // Get Offsets of current item.
  // Calling this function may calculate the offsets.
  Offsets                     GetOffset(bool withlocation) const;
  // Set the current location and song as time offset.
  // If the offset is less than zero it counts from the back.
  // The Navigation starts from the current location.
  // If this is a song, the call is similar to SetLocation().
  // If current is a playlist the navigation applies to the whole list content.
  // If you want to Navigate within the root call Reset before.
  // The function returns true if the navigation was successful.
  // In this case the current item is always a song.
  bool                        SetTimeOffset(T_TIME offset);

  // Go to the previous Song.
  bool                        Prev()                   { return PrevNextCore(&SongIterator::PrevCore); }
  // Go to the next Song.
  bool                        Next()                   { return PrevNextCore(&SongIterator::NextCore); }
  // reset the Iterator to it's initial state. 
  void                        Reset();
  // return the start location within the current song.
  T_TIME                      GetLocation() const      { return Location; }
  // modify the start location within the current song.
  void                        SetLocation(T_TIME loc)  { Location = loc; CurrentCache = NULL; }

  // Navigate to url[index] within the current deepmost playlist.
  // If the addressed item is a playlist the list is entered implicitely.
  // If the url is '..' the current playlist is left.
  // If the url is NULL only the index is used.
  // A negative index counts from the back. The index must not be 0, otherwise navigation fails.
  // The function returns true if the Navigation succeeded.
  bool                        Navigate(const xstring& url, int index);
  // Same as Navigate but all content from the current deepmost playlist is flattened.
  // So the Navigation goes to the item in the set of non-recursive items in the current playlist
  // and all sublists that matches url/index.
  // If NavigateFlat succeeds, the current item is always a song.
  bool                        NavigateFlat(const xstring& url, int index);

  // Serialize the iterator into a string.
  xstring                     Serialize(bool withlocation = true) const;
  // Deserialize the current instance from a string.
  // The current root must have been set before and it must be consistent with the iterator string
  // otherwise the function returns false. In case of an error a partial navigation may take place.
  // To enforce an absolute location you must call Reset() before.
  bool                        Deserialize(const char*& str);
  
  // This event is signalled whenever the offset of a callstack item is invalidated.
  event<const CallstackEntry> Change;
  
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
  int                         CompareTo(const SongIterator& r, unsigned level = 0, bool withlocation = true) const;
};

// Now we can include the implementation of the above predeclared classes.
#include <playablecollection.h> 

inline bool operator==(const SongIterator& l, const SongIterator& r)
{ return l.CompareTo(r) == 0;
}
inline bool operator!=(const SongIterator& l, const SongIterator& r)
{ return l.CompareTo(r) != 0;
}
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
