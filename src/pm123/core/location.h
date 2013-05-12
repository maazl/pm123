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

#include <limits.h>
#include <cpp/container/vector.h>


class Playable;
class CollectionChangeArgs;
class PlayableSetBase;
class PlayableInstance;

/** @brief Class to identify a deep location within a playlist or a song.
 * @details A location can point to anything within the scope of it's root. I.e.
 * - the root,
 * - a song,
 * - a song within a (nested) playlist,
 * - a time offset within a song,
 * - a nested playlist and
 * - into a nested playlist before its first song.
 * Instances of this class are not thread safe.
 * @remarks Note that a Location does not own its root object.
 * So you have to ensure that no Location can live longer than the referenced root.
 * But it owns the items in the callstack. So they can be safely removed from the collection
 * while the location exists. */
class Location : public Iref_count
{public:
  /// @brief Result of Navigation command
  /// @details It can take three logically distinct values:
  /// - NULL      => Successful.
  /// - ""        => Further information from other objects is required. Retry later.
  /// - "END"     => The end of the list has been reached.
  /// - any other => Not successful because of a syntax error or a non-existing
  ///                or out of bounds reference. Error Message as plain text.
  typedef xstring             NavigationResult;
  /// Constant "END"
  static const NavigationResult End;
  /// Options for \c CompareTo.
  enum CompareOptions
  { CO_Default        = 0x00, ///< Do default comparions
    CO_IgnorePosition = 0x01, ///< Do not compare the position within a song. Treat different positions within the same song as equal.
    CO_Reverse        = 0x02  ///< Take not entered objects as after the end rather than before the start.
  };

 private:
  /// Current Root element
  Playable*                   Root;
  /// @brief Current CallStack
  /// @details The last entry is the currently selected item.
  /// If the Callstack is empty, the current item is \c Root.
  /// The last entry may be \c NULL, indicating that you entered a playlist
  /// but you did navigate to a sub item so far. The location is then
  /// before the start of the list or after the end respectively.
  vector_int<PlayableInstance> Callstack;
  /// @brief Location within the current song (if any).
  /// @details Everything >= 0 is a location within the current song.
  /// Negative values are singular:
  /// - -1 means that the position is before the start or after the end respectively.
  /// - -2 means that the current song has not yet been entered and we are still at the level of the enclosing playlist.
  /// The actual meaning depends on whether you navigate forward or backwards.
  PM123_TIME                  Position;
  //vector_own<class_delegate2<Location, const PlayableChangeArgs, const int> > CallstackDeleg;
#ifdef DEBUG
 private:
  class_delegate<Location, const CollectionChangeArgs> RootChangeDeleg;
 private:
  void                        RootChange(const CollectionChangeArgs& args);
#endif
 protected:
  /// Enter the current playlist.
  /// @pre \c *GetCurrent() is a valid playlist.
  virtual void                Enter();
  /// Leave the current innermost playlist.
  /// @pre \c Callstack.size() is at least 1.
  virtual void                Leave();
  /// Advance current item to the previous or next one. This takes care of shuffle mode.
  /// @param job Priority and \c DependencySet of asynchronous requests if the navigation command could not be completed
  /// because of missing informations on sub items. (Tech info and slice info is required.)
  /// @param direction \c true := forward, \c false := backward.
  /// @return false: failed because of job dependencies.
  /// @pre \c Callstack.size() is at least 1. Child information of \c GetPlaylist() is available.
  virtual bool                PrevNextCore(Job& job, bool direction);
  /// Navigate to the next or previous matching item.
  /// @param dir \c true := forward, \c false := backward.
  /// @param stopat Only stop at items that have at least one bit of this parameter set.
  /// @param job Priority and \c DependencySet of asynchronous requests if the navigation command could not be completed
  /// because of missing informations on sub items. (Tech info and slice info is required.)
  /// @param mindepth Do not ascend beyond depth \a mindepth in the callstack.
  /// If we are currently at depth 2 and the end of the list is reached and \a mindepth is also 2
  /// PrevNextCore will return an error ("END") instead of applying \a dirfunc to the callstack
  /// item at depth 1 or less.
  /// @param maxdepth Do not descend beyond \a maxdepth slice in the callstack.
  /// If we are currently at depth 1 a a nested playlist and \a maxdepth is also 1.
  /// PrevNextCore will not enter this list. It will return or skip the nested list,
  /// depending on whether \c TATTR_PLAYLIST is set in \a stopat.
  /// @return See \c NavigationResult.
  /// @post If the function returned \c NULL then \c GetCurrent() points to an item
  /// with \c tech->attributes \c & \a stopat.
  NavigationResult            NavigateCountCore(Job& job, bool dir, TECH_ATTRIBUTES stopat, unsigned mindepth = 0, unsigned maxdepth = UINT_MAX);

  /// Helper for double dispatch at \c Swap.
  virtual void                Swap2(Location& l);
  /// Helper for calling Swap2 on different objects.
  static void                 CallSwap2(Location& r, Location& l) { r.Swap2(l); }

 private:
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
                              : Position(-1)
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
  virtual                     ~Location()                  { DEBUGLOG(("Location(%p)::~Location()\n", this)); }
  #endif
  /// Swap two instances.
  virtual void                Swap(Location& r);
  /// Assign other instance
  virtual Location&           operator=(const Location& r);
  /// Reinitialize the Location with a new root. This implicitly clears the Location.
  virtual void                SetRoot(Playable* root)      { Reset(); AssignRoot(root); }
  /// Query the current root object.
  Playable*                   GetRoot() const              { return Root; }
  /// Get the current item within the current root where this Location points to.
  /// @return This returns the root itself unless the Location has been modified since the last initialization.
  /// It will return NULL if there is currently no root or the location is before the start of a playlist.
  APlayable*                  GetCurrent() const;
  /// Get the innermost entered playlist.
  /// @return NULL if none.
  /// @remarks \c GetPlaylist() will never return the current item.
  Playable*                   GetPlaylist() const;
  /// Get the entire call stack to get from the root to the current item.
  /// @details The current item is the last entry in the returned call stack
  /// unless the current item equals the root. In this case an empty container
  /// is returned. The root never appears in the call stack.
  /// The last entry in the call stack may be \c NULL if the Location points
  /// before the first item of a playlist.
  const vector<PlayableInstance>& GetCallstack() const     { return Callstack; }
  /// @brief Returns the time offset of this Location within the current Playable.
  /// @return The offset in seconds from the beginning of the current song.
  /// A value of \c <0 indicates that the position has not yet been set.
  /// In case the current item is a playlist the function always return \c -2.
  /// @remarks This is not the same as the time offset within the current root
  /// as well as not the same than the time displayed in PM123. See GetTime.
  PM123_TIME                  GetPosition() const          { return Position; }
  /// @brief Returns the time offset of this Location within the current APlayable.
  /// @return The offset in seconds from the beginning of the current song.
  /// A value of \c <0 indicates that the position has not yet been set.
  /// In case the current item is a playlist the function always return \c -2.
  /// @remarks This is the time display in PM123, i.e. the time from the start
  /// of the current slice (if any).
  PM123_TIME                  GetTime() const;
  /// Depth of the current location.
  /// @details
  /// - 0 = At the root.
  /// - 1 = Root is a playlist and we are at an immediate sub item of this list.
  /// - 2 = We are in a nested playlist item.
  /// - ...
  /// @remarks Note that \c GetLevel could return more than \c GetCallstack().size()
  /// if a song is currently entered.
  unsigned                    GetLevel() const             { return Callstack.size() + (Position >= -1); }

  /// Check whether a Playable object is already in the call stack.
  /// @param pp Playable item to search for. This might be NULL.
  /// @return Depth of pp in the current call stack.
  /// - 0 -> pp is the current root
  /// - 1 -> child of root
  /// - ...
  /// - UINT_MAX -> pp is not in call stack
  unsigned                    FindInCallstack(const Playable* pp) const;
  /// Check whether two locations are related, i.e. one is a subset of the other one.
  /// @return Level where \a r is in the call stack.
  /// - 0  -> Both locations share the same root.
  /// - <0 -> This Location's root is at level \a -return value in the call stack of \a r.
  /// - >0 -> \a r is at level \a return value in the call stack of this Location.
  /// - INT_MIN -> The two locations are unrelated.
  int                         RelationCheck(const Location& r) const;

  /// Resets the current Location to its initial value at the start of the current root.
  /// @param level reset up to
  void                        Reset();
  /// Navigate \a count items forward or backward.
  /// @param count Number of items to skip.
  /// - >&nbsp;0 => forward
  /// - <&nbsp;0 => backward
  /// - =&nbsp;0 => adjust to the next item that matches \a stopat.
  /// @param stopat only count items with at least one bit in \a stopat set in \c TECH_INFO::attributes.
  /// E.g. if you pass TATTR_SONG, playlist items and invalid items will be skipped.
  /// @param mindepth Do not ascend beyond depth \a mindepth in the call stack.
  /// If we are currently at depth 2 and the end of the list is reached and \a mindepth is also 2
  /// PrevNextCore will return an error ("END") instead of applying \a dirfunc to the call stack
  /// item at depth 1 or less.
  /// @param maxdepth Do not descend beyond \a maxdepth slice in the callstack.
  /// If we are currently at depth 1 a a nested playlist and \a maxdepth is also 1.
  /// PrevNextCore will not enter this list. It will return or skip the nested list,
  /// depending on whether \c TATTR_PLAYLIST is set in \a stopat.
  /// @return See \c NavigationResult.
  NavigationResult            NavigateCount(Job& job, int count, TECH_ATTRIBUTES stopat, unsigned mindepth = 0, unsigned maxdepth = INT_MAX);
  /// @brief Navigate to the parent \a index times.
  /// @param index Number of navigations. It must be <= \c GetLevel().
  ///        Of course, you cannot navigate up from the root.
  /// @return See \c NavigationResult.
  /// @details Calling \c NavigateUp(0) will reset the Position within the current song (if any).
  NavigationResult            NavigateUp(unsigned count = 1);
  /// @brief Navigate into a (nested) playlist
  /// @details The function enters a playlist but does not advance to it's first item.
  /// This results in a \c NULL entry at the end of the call stack.
  /// You can't enter sons or invalid items.
  /// @return See \c NavigationResult.
  NavigationResult            NavigateInto(Job& job);
  /// Restart the current song if any.
  void                        NavigateRewindSong()         { if (Position >= 0) Position = -1; }
  /// Navigate explicitly to a given PlayableInstance.
  /// @param pi The PlayableInstance must be a child of the current item.
  /// \a pi might be \c NULL to explicitly navigate before the start of a playlist without leaving it.
  /// @return See \c NavigationResult, but NavigateTo will never depend on outstanding information and return "".
  /// @details This method can be used to explicitly build the callstack.
  NavigationResult            NavigateTo(PlayableInstance* pi);
  /// Navigate explicitly to a given location.
  /// @param Target location. Note that \a target need not to have the same root than the current Location.
  /// It is sufficient if the two locations are related. The navigateion will then take place
  /// in the common subset. If the locations are unrelated, an error is returned.
  /// @return See \c NavigationResult, but \c NavigateTo will never depend on outstanding information and return "".
  NavigationResult            NavigateTo(const Location& target);
  /// Navigate to an occurrence of \a url within the current deepmost playlist.
  /// @param url
  /// - If the \a url is \c '..' the current playlist is left (see \c NavigateUp).
  /// - If the \a url is \c NULL only the index is used to count the items (see NavigateCount).
  /// @param index Navigate to the index-th occurrence of \a url within the current playlist.
  /// The index must not be 0, otherwise navigation fails. A negative index counts from the back.
  /// @param mindepth Do not ascend beyond depth \a mindepth in the call stack.
  /// If we are currently at depth 2 and the current list is left for some reason and \a mindepth is also 2
  /// Naviate will stop and return an error ("END").
  /// @param maxdepth Do not descend beyond \a maxdepth slice in the callstack.
  /// If we are currently at depth 1 a a nested playlist and \a maxdepth is also 1.
  /// Navigate will not enter this list and search inside for matching nodes.
  /// @return See \c NavigationResult.
  /// @remarks \c Navigate will automatically enter the root playlist if necessary.
  /// It will not enter any other playlist.
  NavigationResult            Navigate(Job& job, APlayable* pp, int index, unsigned mindepth, unsigned maxdepth);
  /// Navigate to an occurrence of \c Playable or \c PlayableInstance within the current deepmost playlist.
  /// @param pp Navigation target.
  /// If the \a target is \c NULL only the index is used to count the items (see NavigateCount).
  /// @param index Navigate to the index-th occurrence of \a url within the current playlist.
  /// The index must not be 0, otherwise navigation fails. A negative index counts from the back.
  /// @param mindepth Do not ascend beyond depth \a mindepth in the call stack.
  /// If we are currently at depth 2 and the current list is left for some reason and \a mindepth is also 2
  /// Naviate will stop and return an error ("END").
  /// @param maxdepth Do not descend beyond \a maxdepth slice in the callstack.
  /// If we are currently at depth 1 a a nested playlist and \a maxdepth is also 1.
  /// Navigate will not enter this list and search inside for matching nodes.
  /// @return See \c NavigationResult.
  /// @remarks \c Navigate will automatically enter the root playlist if necessary.
  /// It will not enter any other playlist.
  NavigationResult            Navigate(Job& job, const xstring& url, int index, unsigned mindepth, unsigned maxdepth);
  /// Move the current location and song as time offset.
  /// @param offset If the offset is less than zero it counts from the back.
  /// The navigation starts from the current location.
  /// @return See \c NavigationResult.
  /// @remarks If you want to navigate within the root call Reset before.
  /// If Navigate succeeds, the current item is always a song.
  NavigationResult            NavigateTime(Job& job, PM123_TIME offset, int mindepth = 0, bool absolute = false);
  
  /// Serialize the iterator into a string.
  /// @param withpos \c true: include the time offset within the deepest item.
  /// @param delimiter Call stack component delimiter. \c ';' by default.
  xstring                     Serialize(bool withpos = true, char delimiter = ';') const;
  /// Deserialize the current instance from a string and return a error message (if any).
  /// @param str \a str is relative. To enforce an absolute location call \c Reset() before.
  /// @param job Priority of asynchronous requests if the navigation command could not be completed
  /// because of missing informations on sub items. And a \c DependencySet which
  /// is populated if the latter happens. If the priority is \c PRI_Sync
  /// \c Deserialize will never return \c "" but wait for the information
  /// to become available instead. This may take quite long on large recursive playlists.
  /// @details The current root must be consistent with the iterator string otherwise
  /// the function signals an error. In case of an error a partial navigation may take place.
  /// \a str is advanced to the current read location. This can be used to localize errors.
  /// If the return value is \c "" then requests have been placed on other objects
  /// at the priority level \a pri and you have to retry the deserialization later.
  /// Note that the value of \c *this is undefined in this case and must restore
  /// the previous value in case of relative navigation.
  NavigationResult            Deserialize(Job& job, const char*& str);

  /// @brief Relational comparison
  /// @return - The return value is zero if the locations are equal.
  /// - The return value is less than zero if the current iterator is less than r.
  /// - The return value is greater than zero if the current iterator is greater than r.
  /// - The absolute value is the level where the two iterators are different.
  /// E.g.: a return value of 2 means that r is greater than \c *this in a sublist that is part of the current root.
  /// The returned absolute may exceed the current level of the \c Location by one
  /// if both \c Locations address the same song and call stack but at a different location.
  /// - A return value of \c INT_MIN means that the two iterators differ in their root node.
  /// In this case they are unordered.
  /// @param options Optional compare options.
  /// @param level The optional level parameter compares r to a slice of \c *this starting at level.
  /// I.e. \c level \c = \c 2 means that the root of r is compared against level 2 of \c *this.
  /// level must be less than the current depth of \c *this.
  /// The absolute return value will not be less than level except for a equal condition.
  /// The current thread must own both Location objects.
  int                         CompareTo(const Location& r, CompareOptions opt = CO_Default, unsigned level = 0) const;
};

FLAGSATTRIBUTE(Location::CompareOptions);

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
