/*
 * Copyright 2009-2011 M.Mueller
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

#include "location.h"
#include "playable.h"
#include "dependencyinfo.h"

#include <limits.h>
#include <stdio.h>


/****************************************************************************
*
*  class Location
*
****************************************************************************/

#ifdef DEBUG
void Location::RootChange(const CollectionChangeArgs& args)
{ DEBUGLOG(("Location(%p)::RootChange({%p,...})\n", this, &args.Instance));
  // Check whether the root dies before this instance.
  ASSERT((args.Changed|args.Loaded|args.Invalidated) || Root != &args.Instance);
}

void Location::AssignRoot(Playable* root)
{ if (Root == root)
    return;
  RootChangeDeleg.detach();
  if ((Root = root) != NULL)
    root->GetInfoChange() += RootChangeDeleg;
}
#endif

/*void Location::SetRoot(APlayable* p)
{ DEBUGLOG(("Location(%p)::SetRoot(%p)\n", this, p));
  //MakeCallstackUnique();
  Reset();
  Root = p;
}*/

void Location::Swap2(Location& l, int magic)
{ DEBUGLOG(("Location(%p)::Swap2(&%p, %i)\n", this, &l, magic));
  #ifdef DEBUG
  Playable* tmp = Root;
  AssignRoot(l.Root);
  l.AssignRoot(tmp);
  #else
  swap(Root, r.Root);
  #endif
  Callstack.swap(l.Callstack);
  swap(Position, l.Position);
}

void Location::Swap(Location& r)
{ DEBUGLOG(("Location(%p)::Swap(&%p)\n", this, &r));
  r.Swap2(*this, 0);
}

void Location::Reset()
{ Position = 0;
  while (Callstack.size())
    Leave();
}

Location& Location::operator=(const Location& r)
{ AssignRoot(r.Root);
  Callstack = r.Callstack;
  Position = r.Position;
  return *this;
}

APlayable* Location::GetCurrent() const
{ if (Callstack.size() == 0)
    return Root;
  else
    return Callstack[Callstack.size()-1];
}

Playable* Location::GetPlaylist() const
{ switch (Callstack.size())
  {case 0:
    return 0;
   case 1:
    return Root;
   default:
    return &Callstack[Callstack.size()-1]->GetPlayable();
  }
}

size_t Location::FindInCallstack(const Playable* pp) const
{ if (pp)
  { if (Root == pp)
      return 0;
    foreach (const int_ptr<PlayableInstance>*, pipp, Callstack)
      if (&(*pipp)->GetPlayable() == pp)
        return pipp - Callstack.begin() + 1;
  }
  return UINT_MAX;
}

void Location::Enter()
{ DEBUGLOG(("Location(%p)::Enter()\n", this));
  Callstack.append();
}

void Location::Leave()
{ DEBUGLOG(("Location(%p)::Leave()\n", this));
  Callstack.erase(Callstack.size()-1);
}

void Location::PrevNextCore(bool direction)
{ size_t cssize = Callstack.size();
  ASSERT(cssize);
  int_ptr<PlayableInstance>& pi = Callstack[cssize-1];
  Playable& list = cssize > 1 ? Callstack[cssize-2]->GetPlayable() : *Root;
  DEBUGLOG(("Location(%p)::PrevNextCore(%u) %p{%s} -> %p{%s}\n", this,
    direction, &list, list.URL.getDisplayName().cdata(), pi.get(), PlayableInstance::DebugName(pi)));
  pi = direction ? list.GetNext(pi) : list.GetPrev(pi);
  DEBUGLOG(("Location::PrevNextCore -> %p{%s}\n", pi.get(), PlayableInstance::DebugName(pi)));
}

Location::NavigationResult Location::NavigateCountCore(JobSet& job, bool dir, TECH_ATTRIBUTES stopat, unsigned mindepth, unsigned maxdepth)
{ DEBUGLOG(("Location(%p)::NavigateCountCore({%u,}, %u, %x, %i, %i) - %u\n", this, job.Pri, dir, stopat, mindepth, maxdepth, Callstack.size()));
  ASSERT(stopat);
  ASSERT(Callstack.size() >= mindepth);
  ASSERT(maxdepth >= Callstack.size());

  if (!Root)
    return "Cannot navigate without root.";
  Position = 0;

  APlayable* cur;
  do
  { // Enter playlist?
    if (Callstack.size() == maxdepth)
      goto noenter;
    cur = GetCurrent();
    if ( !cur
      || FindInCallstack(&cur->GetPlayable()) < Callstack.size() )
      goto noenter;
    if (job.RequestInfo(*cur, IF_Tech|IF_Slice|IF_Child))
      return xstring::empty; // Delayed!!!
    if ((cur->GetInfo().tech->attributes & (TATTR_SONG|TATTR_PLAYLIST|TATTR_INVALID)) == TATTR_PLAYLIST)
      Enter();
    else
    {noenter:
      if (Callstack.size() == 0)
        return "End";
    }
    // Now Callstack.size() is at least 1 and
    // cur points to the parent playlist of Callstack[Callstack.size()-1].

    // TODO: start/stop location
    // Find next item in *cur before/after pi that is not in call stack (recursion check)
    PrevNextCore(dir);
    // End of list => go to parent
    if (Callstack[Callstack.size()-1] == NULL)
    { if (Callstack.size() == mindepth)
        return "End";
      Leave();
      goto noenter;
    }

    cur = Callstack[Callstack.size()-1];
    if (cur && job.RequestInfo(*cur, IF_Tech|IF_Slice|IF_Child))
      return xstring::empty; // Delayed!!!

  } while ((cur->GetInfo().tech->attributes & stopat) == 0);
  // done
  return xstring(); // NULL
}

Location::NavigationResult Location::NavigateCount(JobSet& job, int count, TECH_ATTRIBUTES stopat, unsigned mindepth, unsigned maxdepth)
{ Location::NavigationResult ret;
  if (count)
  { bool dir = count > 0;
    if (!dir)
      count = -count;
    // TODO: we could skip entire playlists here if < count.
    do
    { ret = NavigateCountCore(job, dir, stopat, mindepth, maxdepth);
      if (ret)
        break;
    } while (--count);
  }
  return ret;
}

Location::NavigationResult Location::NavigateUp(unsigned count)
{ DEBUGLOG(("Location::NavigateUp(%u)\n", count));
  Position = 0;
  xstring ret;
  if (count > Callstack.size())
  { ret.sprintf("Cannot navigate %i times to the parent item while the current depth is only %u.",
      count, Callstack.size());
    count = Callstack.size();
  }
  while (count--)
    Leave();
  return ret;
}

Location::NavigationResult Location::NavigateInto(JobSet& job)
{ DEBUGLOG(("Location(%p)::NavigateInto({%u,})\n", job.Pri));
  APlayable* cur = GetCurrent();
  if (!cur)
    return "Can not enter NULL.";
  if (FindInCallstack(&cur->GetPlayable()) < Callstack.size())
    return "Can not enter recursive playlist.";
  job.RequestInfo(*cur, IF_Tech);
  if (!(cur->GetInfo().tech->attributes & TATTR_PLAYLIST))
    return "Current item is not a playlist.";
  Enter();
  return xstring(); // NULL
}

Location::NavigationResult Location::NavigateTo(PlayableInstance* pi)
{ DEBUGLOG(("Location(%p)::NavigateTo(&%p)\n", this, &pi));
  if (pi && !pi->HasParent(GetPlaylist()))
    return "Instance is no child of the current item.";
  Callstack[Callstack.size()-1] = pi;
  return xstring(); // NULL
}

Location::NavigationResult Location::Navigate(JobSet& job, const xstring& url, int index, bool flat)
{ DEBUGLOG(("Location(%p)::Navigate({%u,}, %s, %i, %u)\n", this,
    job.Pri, url.cdata(), index, flat));

  if (!Root)
    return "Cannot navigate without root.";

  int maxdepth = flat ? INT_MAX : Callstack.size();

  if (!url)
    return NavigateCount(job, index, TATTR_SONG, 0, maxdepth);

  if (url == "..")
  { if (flat)
      return "Flat navigation to .. is not valid.";
    if (index < 0)
      return "Cannot navigate up a negative number of times.";
    return NavigateUp(index);
  }
  
  // Auto enter playlist
  { APlayable* cur = GetCurrent();
    if (cur)
    { if (job.RequestInfo(*cur, IF_Tech))
        return xstring::empty; // delayed
      if ((cur->GetInfo().tech->attributes & (TATTR_PLAYLIST|TATTR_SONG)) == TATTR_PLAYLIST)
      { Enter();
        if (!flat)
          ++maxdepth;
      }
    }
  }
  xstring ret;
  // URL lookup
  Playable* pp = GetPlaylist();
  if (!pp)
    return "Cannot navigate to item inside a song.";
  const url123& absurl = pp->URL.makeAbsolute(url);
  if (!absurl)
    return ret.sprintf("Invalid URL %s", url.cdata());
  pp = Playable::GetByURL(absurl);

  // Do navigation
  if (index)
  { bool dir = index > 0;
    if (!dir)
      index = -index;
    do
    { do
      { ret = NavigateCountCore(job, dir, ~TATTR_NONE, 0, maxdepth);
        if (ret)
          goto done;
      } while (&GetCurrent()->GetPlayable() != pp);
    } while (--index);
  }
 done:
  return ret;

/* APlayable* cur = GetCurrent();
  if (cur == NULL)
    return "Cannot navigate without root.";
  // Request required information
  if (job.RequestInfo(*cur, IF_Tech|IF_Child|IF_Slice))
    return xstring::empty; // Delayed

  // For flat navigation...
  PlayableSet exclude;
  volatile const AggregateInfo* ai;
  InfoFlags what;
  if (flat)
  { // Request RPL info
    ai = &job.RequestAggregateInfo(*cur, exclude, what = IF_Rpl);
    if (what)
      return xstring::empty; // Delayed
    // Prepare exclude list
    const int_ptr<PlayableInstance>* pipp = Callstack.begin();
    const int_ptr<PlayableInstance>* pepp = Callstack.begin();
    while (pipp != pepp)
    { Playable& p = (*pipp++)->GetPlayable();
      exclude.get(p) = &p;
    } 
  }

 recurse:
  // Navigate in synchronized context
  Mutex::Lock lck(cur->GetPlayable().Mtx);
  // Preconditions
  const Playable& pc = cur->GetPlayable();
  unsigned attr = cur->GetInfo().tech->attributes;
  if (attr & TATTR_INVALID)
    return xstring().sprintf("Cannot navigate into invalid item %s.", pc.URL.cdata());
  if (!(attr & TATTR_PLAYLIST))
    return xstring().sprintf("Cannot navigate into song item %s.", pc.URL.cdata());

  PlayableInstance* pi = NULL;
  // search item in the list
  if (!url || url.length() == 0)
  { // Navigation by index only
    if (flat)
    { // Flat index navigation
      int songs = ai->Rpl.songs;
      if (index < 0)
        index += songs;
      if (index < 1 || index > songs)
        return xstring().sprintf("Index %i is out of bounds. %s has %i song items",
          index, pc.URL.cdata(), songs);
      // loop until we cross the number of items
      for (;;)
      { pi = pc.GetNext(pi);
        if (pi == NULL)
          return xstring().sprintf("Unexpected end of list %i/%i.", index, songs);
        // Terminate loop if the number of subitems is unknown or if it is larger than the required index.
        ai = &pi->RequestAggregateInfo(exclude, what = IF_Rpl, PRI_None);
        if (what)
        { DEBUGLOG(("Location::NavigateFlat: Rpl info of %s not available.\n", pi->GetPlayable().URL.cdata()));
          return xstring::empty; // Should not happen
        }
        if ((int)ai->Rpl.songs >= index)
          break;
        index -= ai->Rpl.songs;
      }
      // We found a matching location.
      ASSERT(index > 0);
      Position = 0;
      Callstack.append() = pi;
      // Apply the residual offset to the subitem if any.
      if (job.RequestInfo(*pi, IF_Tech|IF_Child|IF_Slice))
      { DEBUGLOG(("Location::NavigateFlat: Tech info of %s not available.\n", cur->GetPlayable().URL.cdata()));
        return xstring::empty; // Should not happen
      }
      if ((pi->GetInfo().tech->attributes & (TATTR_SONG|TATTR_PLAYLIST)) == TATTR_PLAYLIST)
      { cur = pi;
        exclude.get(cur->GetPlayable()) = &cur->GetPlayable();
        // effectively a recursive call to ourself.
        goto recurse;
      }
      // If we are at a song item, the index should match exactly.
      ASSERT(index == 1);
      return xstring(); // NULL

    } else // !flat
    { // Index navigation
      int count = pc.GetInfo().obj->num_items;
      // address from back
      if (index < 0)
        index += count + 1;
      // address by index
      if (index <= 0 || index > count)
        return xstring().sprintf("Index %i is beyond the limits (%u) of the playlist %s.",
          index, count, cur->GetPlayable().URL.cdata());

      // Speed up: start from back if closer to it
      if ((index<<1) > count)
      { // iterate from behind
        index = count - index;
        do
          pi = pc.GetPrev(pi);
        while (index--);
      } else
      { // iterate from the front
        do
          pi = pc.GetNext(pi);
        while (index--);
      }
      // Postcondition
      if (FindInCallstack(&pi->GetPlayable()) != UINT_MAX)
        return xstring().sprintf("Cannot navigate into the recursive item %s.",
          pi->GetPlayable().URL.cdata());
    }
    
    // TODO: conflicting slice?
    Position = 0;
    Callstack.append() = pi;
    return xstring(); // NULL

  } else
  { // look for a certain URL
    const url123& absurl = cur->GetPlayable().URL.makeAbsolute(url);
    if (!absurl)
      return xstring().sprintf("The URL %s cannot be understood within the context of %s.",
        url.cdata(), cur->GetPlayable().URL.cdata());
    const int_ptr<Playable>& pp = Playable::FindByURL(absurl);
    if (pp == NULL)
      // If the url is in the playlist it MUST exist in the repository, otherwise => error.
      return xstring().sprintf("The item %s is not part of %s.",
        url.cdata(), cur->GetPlayable().URL.cdata());
    if (FindInCallstack(pp) != UINT_MAX)
      return xstring().sprintf("Cannot navigate into the recursive item %s.",
        pi->GetPlayable().URL.cdata());
    
    bool direction = index > 0;
    if (!direction) // reverse lookup
      index = -index;

    if (flat)
    { // flat URL navigation
      size_t level = Callstack.size();
      do
      { const xstring& ret = NavigateCountCore(direction, TATTR_SONG, job, level);
        if (ret)
        { if (ret == xstring::empty)
            return ret;
          else
            return xstring().sprintf("The item %s is not part of %s.",
              url.cdata(), GetCurrent()->GetPlayable().URL.cdata());
        }
      } while (&GetCurrent()->GetPlayable() != pp || --index);

    } else // !flat
    { // Normal URL navigation
      do
      { PrevNextCore(direction);
        pi = Callstack[Callstack.size()-1];
        if (pi == NULL)
          return xstring().sprintf("The item %s is no child of %s.",
            url.cdata(), cur->GetPlayable().URL.cdata());
      } while (&pi->GetPlayable() != pp || --index);
    }
    return xstring(); // NULL
  }*/
}

Location::NavigationResult Location::NavigateTime(JobSet& job, PM123_TIME offset, unsigned mindepth)
{ DEBUGLOG(("Location(%p)::Navigate({%u,}, %f, %u) - %u\n", this, job.Pri, offset, mindepth, Callstack.size()));
  
  if (Root == NULL)
    return "Cannot Navigate without root.";

  xstring ret;
  if (offset == 0)
    return ret; // no-op

  const bool direction = offset > 0;

  // Try to navigate within the current song.
  APlayable* cur = GetCurrent();
  if (cur)
  {retry:
    Position += offset;
    if (!direction)
    { if (Position >= 0)
        return ret; // Navigation within song succeeded
      offset = Position; // remaining part
    } else
    { if (job.RequestInfo(*cur, IF_Tech|IF_Obj|IF_Child))
        return xstring::empty;
      if (cur->GetInfo().tech->attributes & TATTR_SONG)
      { const PM123_TIME songlength = cur->GetInfo().obj->songlength;
        if (Position < songlength)
          return ret; // Navigation within song succeeded
        else if (songlength < 0)
          return "Indeterminate song length.";
        offset = Position - songlength; // remaining part
      }
    }
  } // else we entered a playlist but did not navigate to an item.
  Position = 0;

 next:
  ret = NavigateCountCore(job, direction, TATTR_SONG|TATTR_PLAYLIST, mindepth);
  if (ret)
    return ret;

 afterskip:
  cur = GetCurrent();
  if (!(cur->GetInfo().tech->attributes & TATTR_PLAYLIST))
  { // Song item
    if (!direction)
    { // Navigate from back.
      if (job.RequestInfo(*cur, IF_Obj))
        return xstring::empty;
      Position = cur->GetInfo().obj->songlength;
      if (Position < 0)
      { Position = 0;
        return "Indeterminate song length.";
      }
    }
    goto retry;
  } else
  { size_t i = Callstack.size() -1;
    PlayableSet exclude(i);
    while (i--)
      exclude.add(Callstack[i]->GetPlayable());
    if (exclude.contains(Callstack[Callstack.size()-1]->GetPlayable()))
      goto next; // recursive list
    InfoFlags what = IF_Drpl;
    const volatile AggregateInfo& ai = cur->RequestAggregateInfo(exclude, what, PRI_None);
    if (what)
      goto next; // We do not have aggregate info.
    PM123_TIME totallength = ai.Drpl.totallength;
    if (totallength < 0)
      goto next; // Length of playlist indeterminate
    if (!direction)
    { // backward
      if (totallength + offset > 0)
        goto next;
      offset += totallength;
    } else
    { // forward
      if (offset > totallength)
        goto next;
      offset -= totallength;
    }
    // Skip playlist
    ret = NavigateCountCore(job, direction, TATTR_SONG|TATTR_PLAYLIST, mindepth, Callstack.size());
    if (ret)
      return ret;
    goto afterskip;
  }

/*  // Prepare exclude list
  PlayableSet exclude;
  foreach (const int_ptr<PlayableInstance>*, pipp, Callstack)
  { cur = *pipp;
    Playable& p = cur->GetPlayable();
    exclude.get(p) = &p;
  }
  // Implicit: cur = &GetCurrent();

  // Identify direction
  int sign;
  if (offset >= 0)
  { sign = 1;
  } else
  { sign = -1;
    offset = -offset;
  }

 recurse:
  // Navigate in synchronized context
  Mutex::Lock lck(cur->GetPlayable().Mtx);

  if ((cur->GetInfo().tech->attributes & (TATTR_SONG|TATTR_PLAYLIST)) == TATTR_PLAYLIST)
  { // cur is playlist => flat time navigation
    const Playable& pc = cur->GetPlayable();
    int_ptr<PlayableInstance> pi;

    volatile const AggregateInfo* ai;
    for(;;)
    { PrevNextCore(sign > 0);
      pi = Callstack[Callstack.size()-1];
      if (pi == NULL)
        return xstring().sprintf("Time %f is beyond the length of %s.",
          offset*sign, pc.URL.cdata());
      // Request aggregate info
      InfoFlags what;
      ai = &job.RequestAggregateInfo(*pi, exclude, what = IF_Drpl);
      if (what)
        return xstring::empty; // Delayed
      // Check offset
      if (ai->Drpl.unk_length || ai->Drpl.totallength > offset)
      { // Found location
        cur = pi;
        exclude.get(cur->GetPlayable()) = &cur->GetPlayable();
        // effectively a recursive call to ourself.
        goto recurse;
      }
      offset -= ai->Drpl.totallength;
    }
  
  } else
  { // cur is song
    PM123_TIME songlen = cur->GetInfo().obj->songlength;
    if (sign < 0)
    { // reverse navigation
      if (songlen < 0)
        return xstring().sprintf("Cannot navigate from back of %s with unknown songlength.",
          cur->GetPlayable().URL.cdata());
      if (offset >= songlen)
        return xstring().sprintf("Time index -%f is before the start of the song %s with length %f.",
          offset, cur->GetPlayable().URL.cdata(), songlen);
      offset = songlen - offset;
    } else if (songlen >= 0 && offset >= songlen)
    { return xstring().sprintf("Time index %f is behind the end of the song %s at %f.",
        offset, cur->GetPlayable().URL.cdata(), songlen);
    }
    // TODO: start and stop slice
    Position = offset;
  }
  return xstring(); // NULL*/
}

xstring Location::Serialize(bool withpos) const
{ DEBUGLOG(("Location(%p)::Serialize()\n", this));
  /*if (!Root)
    return xstring();*/ // NULL
  xstringbuilder ret;
  Playable* list = Root;
  for (const int_ptr<PlayableInstance>* cpp = Callstack.begin(); cpp != Callstack.end(); ++cpp)
  { if (*cpp)
    { // Fetch info
      Playable& p = (*cpp)->GetPlayable();
      // append url to result
      ret.append('\"');
      ret.append(p.URL.makeRelative(list->URL));
      ret.append('\"');
      // check if the item is unique
      unsigned n = 1;
      const PlayableInstance* pi2 = *cpp;
      while ((pi2 = list->GetPrev(pi2)) != NULL)
        n += &p == &pi2->GetPlayable();
      if (n > 1) // Not unique => provide index
        ret.appendf("[%u]", n);
      list = &p;
    }
    // next
    ret.append(';');
  }
  if (withpos && Position)
  { char buf[32];
    ret.append(PM123Time::toString(buf, Position));
  }
  return ret;
}

Location::NavigationResult Location::Deserialize(JobSet& job, const char*& str)
{ DEBUGLOG(("Location(%p)::Deserialize({%u, }, %s) - %s\n", this, job.Pri, str, Serialize(true).cdata()));
  // Skip leading white space.
  str += strspn(str, " \t");
  
  for (;; str += strspn(str, ";\r\n"))
  { bool flat = false;
   restart:
    DEBUGLOG(("Location::Deserialize now at %s\n", str));
    // Determine type of part
    switch (*str)
    {case 0: // end
      return xstring(); // NULL

     case ';': // String starts with a part seperator => use absolute navigation. 
     case '\r':
     case '\n':
      Reset();
      break;
      
     case '*': // Flat navigation
      flat = true;
      str += 1 + strspn(str+1, " \t");
      goto restart;
     
     case '"': // Quoted playlist item
      { xstring url;
        size_t index = 1;
        size_t len;
        const char* ep = strchr(str+1, '"');
        if (ep == NULL)
        { url = str+1;
          len = 1 + url.length(); // end of string => set str to the terminating \0
        } else
        { url.assign(str+1, ep-str-1);
          ++ep; // move behind the '"'
          size_t len2 = len = strcspn(ep, ";\r\n");
          len += ep - str;
          // look for index
          while (len2 && (ep[len2-1] == ' ' || ep[len2-1] == '\t'))
            --len2;
          if (len2)
          { size_t n;
            if (sscanf(ep, "[%u]%n", &index, &n) != 1 || n != len2 || index == 0)
            { str = ep;
              return xstring().sprintf("Syntax: invalid index at %*s", len2, ep);
            }
          }
        }

        // do the navigation
        const xstring& ret = Navigate(job, url, index, flat);
        if (ret)
          return ret;

        str += len;
      }
      break;

     default:
      { xstring url;
        size_t index = 1;
        size_t len = strcspn(str, ";\r\n");
        size_t len2 = len;
        while (str[len2-1] == ' ' || str[len2-1] == '\t')
          --len2;
        size_t end = strspn(str, "+-0123456789:. \t");
        if (end >= len2)
        { // Time value
          const char* cp = str;
          bool sign = false;
          if (*cp == '-')
          { sign = true;
            ++cp;
          }
          double t[4] = {0};
          double* dp = t;
          for(;;)
          { size_t n = 0;
            sscanf(cp, "%lf%n", dp, &n);
            cp += n;
            if (cp == str + len2)
              break;
            if (*cp != ':')
            { len2 -= cp - str;
              str = cp;
              return xstring().sprintf("Syntax: invalid character at %*s\n", len2, cp);
            }
            do
            { if (++dp == t + sizeof t / sizeof *t)
              { len2 -= cp - str;
                str = cp;
                return xstring().sprintf("Syntax: to many ':' at %*s\n", len2, cp);
              }
            } while (*++cp == ':');
          }

          // make time
          if (dp-- != t)
          { dp[0] = 60*dp[0] + dp[1]; // Minutes
            if (dp-- != t)
            { dp[0] = 3600*dp[0] + dp[1]; // Hours
              if (dp-- != t)
                dp[0] = 86400*dp[0] + dp[1]; // Days
          } }
          if (sign)
            t[0] = -t[0];
          // do the navigation
          const xstring& ret = NavigateTime(job, t[0]);
          if (ret)
            return ret;

        } else
        { // Unquoted playlist item
          if (str[len2-1] == ']')
          { // with index
            char* ep = strnrchr(str, '[', len2);
            size_t n;
            if (ep == NULL || sscanf(ep+1, "%u%n", &index, &n) != 1 || (int)n != str+len2-ep-2 || index == 0)
            { len2 -= ep - str;
              str = ep+1;
              return xstring().sprintf("Syntax: invalid index at %*s", len2, ep);
            }
            if (ep != str)
              url.assign(str, ep-str);
              // otherwise only index is specified => leave url NULL instead of ""
          } else
            url.assign(str, len2);

          // do the navigation
          const xstring& ret = Navigate(job, url, index, flat);
          if (ret)
            return ret;
        }
        str += len;
    } }
  } // next part
}

int Location::CompareTo(const Location& r, unsigned level, bool withpos) const
{ DEBUGLOG(("Location(%p)::CompareTo(%p) - %s - %s\n", this, &r, Serialize().cdata(), r.Serialize().cdata()));
  ASSERT(Root && r.Root);
  ASSERT(level <= Callstack.size());
  const Playable* lroot = level ? &Callstack[level-1]->GetPlayable() : Root;
  /*if (lroot == NULL && r.Root == NULL)
    return 0; // Both iterators are unassigned
  if ( lroot == NULL || r.Root == NULL*/
  if (lroot != r.GetRoot()) // instance comparison
    return INT_MIN; // different root
  
  const int_ptr<PlayableInstance>* lcpp = Callstack.begin() + level;
  const int_ptr<PlayableInstance>* rcpp = r.Callstack.begin();
  for (;;)
  { // Fetch next level
    ++level;
    if (lcpp == Callstack.end())
    { if (rcpp != r.Callstack.end())
        return -level; // current Location is deeper
      // Callstack identical => compare location below
      if (!withpos || Position == r.Position)
        return 0; // same
      ++level; // Location difference returns level+1
      return Position > r.Position ? level : -level;
    }
    if (rcpp == r.Callstack.end())
      return level; // current Location is deeper

    // Check their order
    if (*lcpp != *rcpp) // Instance equality is equivalent to equality.
    { // Lock the parent collection to make the comparison below reliable.
      Mutex::Lock lock((Mutex&)lroot->Mtx); // Mutex is mutable
      // TODO: unordered because one item has been removed?
      // Currently the remaining one is always considered to be greater.
      return (*lcpp)->GetIndex() > (*rcpp)->GetIndex() ? level : -level;
    }
    // Next level
    lroot = &(*lcpp)->GetPlayable();
    ++lcpp;
    ++rcpp;
  }
}

struct AggregateHelper
{ const InfoFlags           What;
  const Priority            Pri;
  InfoFlags                 Complete;
  PlayableSet               Exclude;
  AggregateHelper(InfoFlags what, Priority pri) : What(what), Pri(pri), Complete(what) {}
  const volatile AggregateInfo& FetchAI(APlayable& p);
  InfoFlags                 GetIncomplete() const { return What & ~Complete; }
};

const volatile AggregateInfo& AggregateHelper::FetchAI(APlayable& p)
{ DEBUGLOG(("Location::AggregateHelper({%x, %u, %x, {%u,}})::FetchAI(&%p)", What, Pri, Complete, Exclude.size(), &p));
  InfoFlags what2 = What;
  const volatile AggregateInfo& ai = p.RequestAggregateInfo(Exclude, what2, Pri);
  Complete &= what2;
  return ai;
}

InfoFlags Location::AddFrontAggregate(AggregateInfo& dest, InfoFlags what, Priority pri, size_t level)
{ DEBUGLOG(("Location(%p)::AddFrontAggregate(&%p{%s,}, %x, %u, %u)\n", this, &dest, dest.Exclude.size(), what, pri, level));
  ASSERT(level <= GetLevel());
  ASSERT((what & ~IF_Aggreg) == 0);
  // Position
  if ((what & IF_Drpl) && Position)
  { dest.Drpl.totallength += Position;
    APlayable& cur = *GetCurrent();
    if ( cur.RequestInfo(IF_Phys|IF_Tech, pri) == IF_None
      && cur.GetInfo().phys->filesize >= 0 && cur.GetInfo().obj->songlength >= 0 )
      dest.Drpl.totalsize += cur.GetInfo().phys->filesize * Position / cur.GetInfo().obj->songlength;
  }
  if (GetLevel() == 0)
    return IF_None;
  // Playlist offsets
  AggregateHelper agg(what, pri);
  agg.Exclude.reserve(dest.Exclude.size() + Callstack.size()-1);
  agg.Exclude = dest.Exclude;
  Playable* list = Root;
  for(size_t l = 0;;)
  { PlayableInstance& item = *Callstack[l];
    if (l >= level)
    { // Take the faster way
      if (item.GetIndex() <= list->GetInfo().obj->num_items >> 1)
      { // Calculate from front
        int_ptr<PlayableInstance> pi = &item;
        while ((pi = list->GetPrev(pi)) != NULL)
          dest += agg.FetchAI(item);
      } else
      { // Calculate from back
        dest += agg.FetchAI(*list);
        int_ptr<PlayableInstance> pi = &item;
        do dest -= agg.FetchAI(*pi);
        while ((pi = list->GetNext(pi)) != NULL);
      }
    }
    // next level
    if (++l >= GetLevel())
      break;
    list = &item.GetPlayable();
    agg.Exclude.add(*list);
  }
  return agg.GetIncomplete();
}

