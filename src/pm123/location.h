/*
 * Copyright 2007-2011 M.Mueller
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
#include "playableinstance.h"

#include <limits.h>
#include <cpp/container/vector.h>


class Playable;
class PlayableSetBase;
/** Class to identify a deep location within a playlist or a song.
 * @details A location can point to anything within the scope of it's root. I.e.
 *  - a start of a song,
 *  - a start of a song in a nested playlist,
 *  - a time offset within a song and
 *  - a nested playlist (this is not the same as the start of the first song within that list).
 * Instances of this class are not thread safe.
 * @remarks Note that a Location does not own its root object.
 * So you have to ensure that no Location can live longer than the referenced root.
 * But it owns the items in the callstack. So they can be safely removed from the collection
 * while the location exists.
 */
class Location : public Iref_count
{public:
  /// Result of Navigation command
  /// It can take three logically distinct values:
  ///  - NULL      => Successful.
  ///  - ""        => Further information from other objects is required. Retry later.
  ///  - "End"     => The end of the list has been reached.
  ///  - any other => Not successful because of a syntax error or a non-existing
  ///                 or out of bounds reference. Error Message as plain text.
  typedef xstring             NavigationResult;

 protected:
  /// Direction function type for \c PrevNextCore
  typedef int_ptr<PlayableInstance> (Playable::*DirFunc)(const PlayableInstance*) const;

 private:
  Playable*                   Root;
  vector_int<PlayableInstance> Callstack;
  PM123_TIME                  Position;
  //vector_own<class_delegate2<Location, const PlayableChangeArgs, const int> > CallstackDeleg;
#ifdef DEBUG
 private:
  class_delegate<Location, const PlayableChangeArgs> RootChangeDeleg;
 private:
  void                        RootChange(const PlayableChangeArgs& args); 
#endif
 private:
  /// Recursive iteration over *this.

  /// The function returns only items that match stopat.
  /// The iteration stops with an error when the callstack comes to level slice (default top).
  /// In this case "End" is returned.
  void x();
  /// Navigate to the next or previous matching item.
  /// @param dirfunc \c &Playable::GetNext or \c &Playable::GetPrev to identify the direction.
  /// @param stopat Only stop at items that have at least one bit of this parameter set.
  /// @param job Priority and \c DependencySet of asynchronous requests if the navigation command could not be completed
  ///        because of missing informations on sub items. (Tech info and slice info is required.)
  /// @param slice Do not ascend beyond depth slice in the callstack.
  ///        If we are currently at depth 2 and the end of the list is reached and slice is also 2
  ///        PrevNextCore will return an error ("End") instead of applying \a dirfunc to the callstack
  ///        item at depth 1 or less.
  /// @return See \c NavigationResult.
  NavigationResult            PrevNextCore(DirFunc dirfunc, TECH_ATTRIBUTES stopat, JobSet& job, unsigned slice = 0);
  /// Assign new root (Location must be initial before)
  #ifdef DEBUG
  void                        AssignRoot(Playable* root);
  #else
  void                        AssignRoot(Playable* root)   { Root = root; }
  #endif
  static const volatile AggregateInfo& DoFetchAI(APlayable& p, const PlayableSetBase& exclude, InfoFlags what, InfoFlags& complete, Priority pri);
 
 public:
  /// Create a new Location object.
  /// @param root Optionally assign a root object where the Location belongs to.
  ///        This can be changed later by \c SetRoot().
  explicit                    Location(Playable* root = NULL)
                              : Position(0)
  #ifdef DEBUG
                              , RootChangeDeleg(*this, &Location::RootChange)
  #endif
                              { DEBUGLOG(("Location(%p)::Location(%p)\n", this, root)); AssignRoot(root); }
  /// Copy constructor.
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
  /// Check whether this Location has assigned a root.
  bool                        operator!() const            { return !Root; }
  /// Swap two instances.
  void                        Swap(Location& r);
  /// Resets the current Location to its initial value at the start of the current root.
  void                        Reset();
  /// Assign other instance
  Location&                   operator=(const Location& r);
  /// Reinitialize the Location with a new root. This implicitly clears the Location.
  void                        SetRoot(Playable* root)      { Reset(); AssignRoot(root); }
  /// Query the current root object.
  Playable*                   GetRoot() const              { return Root; }
  /// Get the current item within the current root where this Location points to.
  /// @details This returns the root itself unless the Location has been modified since the last initialization.
  APlayable&                  GetCurrent() const;
  /// Get the entire callstack to get from the root to the current item.
  /// @details The current item is the last entry in the returned callstack
  /// unless the current item equals the root. In this case an empty container
  /// is returned. The root never appears in the callstack.
  const vector<PlayableInstance>& GetCallstack() const     { return Callstack; }
  /// Returns the time offset of this Location within the current item.
  /// @remarks This is not the same as the time offset within the current root.
  PM123_TIME                  GetPosition() const          { return Position; }
  /// Depth of the current location. (= size of the callstack)
  /// @details
  /// - 0 = At the root.
  /// - 1 = Root is a playlist and we are at an immediate sub item of this list.
  /// - 2 = We are in a nested playlist item.
  /// - ...
  size_t                      GetLevel() const             { return Callstack.size(); }

  /// Check whether a Playable object is already in the call stack.
  /// @param pp Playable item to search for. This might be NULL.
  /// @return true \a *pp is in the current callstack or if it is the current root.
  bool                        IsInCallstack(const Playable* pp) const;

  /// Navigate \a count items forward or backward.
  /// @param count Number of items to skip. >&nbsp;0 => forward <&nbsp;0 => backward, =&nbsp;0 => no-op.
  /// @param stopat only count items with at least one bit in \a stopat set.
  ///        E.g. if you pass TATTR_SONG, playlist items and invalid items will be skipped.
  /// @param pri Priority and \c DependencySet of asynchronous requests if the navigation command could not be completed
  ///        because of missing informations on sub items. (Tech info and slice info is required.)
  /// @param slice Do not ascend beyond depth slice in the callstack.
  ///        If we are currently at depth 2 and the end of the list is reached and slice is also 2
  ///        PrevNextCore will return an error ("End") instead of applying \a dirfunc to the callstack
  ///        item at depth 1 or less.
  /// @return See \c NavigationResult.
  NavigationResult            NavigateCount(int count, TECH_ATTRIBUTES stopat, JobSet& job, unsigned slice = 0);
  /// Navigate to the parent \a index times.
  /// @param index Number of navigations. It must be >= 0 and <= \c GetLevel().
  ///        Of course, you cannot navigate up from the root.
  /// @return See \c NavigationResult.
  NavigationResult            NavigateUp(int index = 1);
  /// Navigate explicitly to a given PlayableInstance.
  /// @param pi The PlayableInstance must be a child of the current item.
  /// @return See \c NavigationResult, but NavigateTo will never depend on outstanding information and return "".
  /// @details This method can be used to explicitely build the callstack.
  NavigationResult            NavigateTo(PlayableInstance& pi);
  /// Navigate to an occurency of \a url within the current deepmost playlist.
  /// @details If the addressed item is a playlist the list is entered implicitely.
  /// @param url
  ///  - If the url is \c '..' the current playlist is left (see NavigateUp).
  ///  - If the url is \c NULL only the index is used to count the items (see NavigateCount).
  /// @param index Navigate to the index-th occurency of url within the current playlist.
  ///        The index must not be 0, otherwise navigation fails. A negative index counts from the back.
  /// @param flat If flat is \c true all content from the current location is flattened.
  ///        So the Navigation goes to the item in the set of non-recursive items in the current playlist
  ///        and all sublists.
  ///        If Navigate with flat = true succeeds, the current item is always a song.
  /// @return See \c NavigationResult.
  NavigationResult            Navigate(const xstring& url, int index, bool flat, JobSet& job);
  /// Set the current location and song as time offset.
  /// @param offset If the offset is less than zero it counts from the back.
  ///        The navigation starts from the current location.
  /// @return See \c NavigationResult.
  /// @remarks If this is a song, the call is similar to SetLocation().
  /// If current is a playlist the navigation applies to the whole list content.
  /// If you want to navigate within the root call Reset before.
  /// If Navigate succeeds, the current item is always a song.
  NavigationResult            Navigate(PM123_TIME offset, JobSet& job);
  
  /// Serialize the iterator into a string.
  /// @param withpos \c true: include the time offset within the deepest item.
  xstring                     Serialize(bool withpos = true) const;
  /// Deserialize the current instance from a string and return a error message (if any).
  /// @param str \a str is relative. To enforce an absolute location call \c Reset() before.
  /// @param job Priority of asynchronous requests if the navigation command could not be completed
  ///        because of missing informations on sub items. And a \c DependencySet which
  ///        is populated if the latter happens. If the priority is \c PRI_Sync
  ///        \c Deserialize will never return \c "" but wait for the information
  ///        to become available instead. This may take quite long on large recursive playlists.
  /// @details The current root must be consistent with the iterator string otherwise
  /// the function signals an error. In case of an error a partial navigation may take place.
  /// \a str is advanced to the current read location. This can be used to localize errors.
  /// If the return value is \c "" then requests have been placed on other objects
  /// at the priority level \a pri and you have to retry the deserialization later.
  /// Note that the value of \c *this is undefined in this case and must restore
  /// the previous value in case of relative navigation.
  NavigationResult            Deserialize(const char*& str, JobSet& job);

  /// @brief relational comparison
  /// @return The return value is zero if the iterators are equal.
  /// The return value is less than zero if the current iterator is less than r.
  /// The return value is greater than zero if the current iterator is greater than r.
  /// The absolute value of the returned int is the level where the two iterators are different.
  /// E.g.: a return value of 2 means that r is greater than \c *this in a sublist that is part of the current root.
  /// The returned absolute may exceed the current level of the SongIterator by one
  /// if both SongIterators address the same song and callstack but at a different location.
  /// A return value of \c INT_MIN means that the two iterators differ in their root node.
  /// In this case they are unordered.
  /// @param level The optional level parameter compares r to a slice of \c *this starting at level.
  /// I.e. \c level \c = \c 2 means that the root of r is compared against level 2 of \c *this.
  /// level must be less than the current depth of \c *this.
  /// The absolute return value will not be less than level except for a equal condition.
  /// The current thread must own both Location objects.
  int                         CompareTo(const Location& r, unsigned level = 0, bool withpos = true) const;

  /// @brief Return the aggregate from the front of root to this location within root.
  /// @param dest Destination aggregate. All items in the exclude list of the destination
  /// will be excluded from the calculation. The result is \e added to the destination
  /// rather than assigned.
  /// @param level Offset from the levelth callstack entry instead of root.
  /// The default level 0 calculates the offsets from the root.
  /// \a level must be in the range [0,GetLevel()].
  /// @remarks The added aggregate is always equal to the Aggregate of Root minus AddBackAggregate
  /// (with the same exclusions).
  InfoFlags                   AddFrontAggregate(AggregateInfo& dest, InfoFlags what, Priority pri, size_t level = 0);
  /// @brief Return the aggregate from this location to the end of root within root.
  /// @param dest Destination aggregate. All items in the exclude list of the destination
  /// will be excluded from the calculation. The result is \e added to the destination
  /// rather than assigned.
  /// @param level Offset from the levelth callstack entry instead of root.
  /// The default level 0 calculates the offsets from the root.
  /// \a level must be in the range [0,GetLevel()].
  /// @remarks The added aggregate is always equal to the Aggregate of Root minus AddBackAggregate
  /// (with the same exclusions).
  InfoFlags                   AddBackAggregate(AggregateInfo& dest, InfoFlags what, Priority pri, size_t level = 0);

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

#endif
