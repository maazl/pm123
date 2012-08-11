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

void Location::Swap2(Location& l)
{ DEBUGLOG(("Location(%p)::Swap2(&%p)\n", this, &l));
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
  r.Swap2(*this);
}

void Location::Reset()
{ Position = -2;
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
    return NULL;
   case 1:
    return Root;
   default:
    PlayableInstance* pi = Callstack[Callstack.size()-2];
    return pi ? &pi->GetPlayable() : NULL;
  }
}

PM123_TIME Location::GetTime() const
{ if (Position < 0)
    return Position;
  APlayable* pp = GetCurrent(); // GetCurrent() must not be NULL if Position >= 0
  int_ptr<Location> start = pp->GetStartLoc();
  if (start && start->GetPosition() > 0)
    return Position - start->GetPosition();
  return Position;
}

unsigned Location::FindInCallstack(const Playable* pp) const
{ if (pp)
  { if (Root == pp)
      return 0;
    foreach (const int_ptr<PlayableInstance>*, pipp, Callstack)
      if (&(*pipp)->GetPlayable() == pp)
        return pipp - Callstack.begin() + 1;
  }
  return UINT_MAX;
}

int Location::RelationCheck(const Location& r) const
{ unsigned level = FindInCallstack(r.GetRoot());
  if (level != UINT_MAX)
    return level;
  level = r.FindInCallstack(GetRoot());
  if (level != UINT_MAX)
    return -level;
  return INT_MIN;
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
    direction, &list, list.DebugName().cdata(), pi.get(), pi.get()->DebugName().cdata()));
  pi = direction ? list.GetNext(pi) : list.GetPrev(pi);
  DEBUGLOG(("Location::PrevNextCore -> %p{%s}\n", pi.get(), pi.get()->DebugName().cdata()));
}

Location::NavigationResult Location::NavigateCountCore(JobSet& job, bool dir, TECH_ATTRIBUTES stopat, unsigned mindepth, unsigned maxdepth)
{ DEBUGLOG(("Location(%p)::NavigateCountCore({%u,}, %u, %x, %u, %u) - %u\n", this, job.Pri, dir, stopat, mindepth, maxdepth, Callstack.size()));
  ASSERT(stopat);
  ASSERT(Callstack.size() >= mindepth);
  ASSERT(maxdepth >= Callstack.size());

  if (!Root)
    return "Cannot navigate without root.";
  Position = -2;

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
  xstring ret;
  unsigned level = GetLevel();
  if (count > level)
  { ret.sprintf("Cannot navigate %i times to the parent item while the current depth is only %u.",
      count, level);
    count = level;
  }
  if (count)
  { if (Position >= -1)
      --count;
    Position = -2;
    while (count--)
      Leave();
  }
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
  if (cur->GetInfo().tech->attributes & TATTR_PLAYLIST)
    Enter();
  else if (Position == -2)
    Position = -1; // Enter song
  else
    return "Navigation is already at a song.";
  return xstring(); // NULL
}

Location::NavigationResult Location::NavigateTo(PlayableInstance* pi)
{ DEBUGLOG(("Location(%p)::NavigateTo(%p{%s})\n", this, pi, pi->DebugName().cdata()));
  Playable* list = GetPlaylist();
  if (!list)
    return "Must enter a playlist before using NavigateTo.";
  if (pi && !pi->HasParent(list))
    return "Instance is no child of the current playlist.";
  Position = -2;
  Callstack[Callstack.size()-1] = pi;
  return xstring(); // NULL
}

Location::NavigationResult Location::NavigateTo(const Location& target)
{ DEBUGLOG(("Location(%p)::NavigateTo({%s})\n", this, target.Serialize().cdata()));
  int rel = RelationCheck(target);
  if (rel == INT_MIN)
    return "Cannot navigate to unrelated location.";
  // Move up to least common root
  while (Callstack.size() && (int)Callstack.size() > rel)
    Leave();
  PlayableInstance*const* pipp = target.GetCallstack().begin();
  PlayableInstance*const* pipe = target.GetCallstack().end();
  if (rel < 0)
    pipp -= rel;
  for (;pipp != pipe; ++pipp)
  { Enter();
    Callstack[Callstack.size()-1] = *pipp;
  }
  Position = target.Position;
  return xstring();
}

Location::NavigationResult Location::Navigate(JobSet& job, const xstring& url, int index, bool flat)
{ DEBUGLOG(("Location(%p)::Navigate({%u,}, %s, %i, %u)\n", this,
    job.Pri, url.cdata(), index, flat));

  if (!Root)
    return "Cannot navigate without root.";

  int maxdepth = flat ? INT_MAX : Callstack.size();

  if (!url)
    return NavigateCount(job, index, ~TATTR_NONE, Callstack.size(), maxdepth);

  xstring ret;
  if (url == "/" || url == "\\")
  { Reset();
    return ret;
  } else if (url == "..")
  { if (flat)
      return "Flat navigation to .. is not valid.";
    if (index < 0)
      return "Cannot navigate up a negative number of times.";
    return NavigateUp(index);
  }
  // Auto enter root
  { APlayable* cur = GetCurrent();
    if (cur && cur == Root)
    { if (job.RequestInfo(*cur, IF_Tech))
        return xstring::empty; // delayed
      if ((cur->GetInfo().tech->attributes & (TATTR_PLAYLIST|TATTR_SONG)) == TATTR_PLAYLIST)
      { Enter();
        if (!flat)
          ++maxdepth;
      }
    }
  }
  if (url == ".")
  { if (Position != -2)
      Position = -1;
    else if (Callstack.size())
      NavigateTo(NULL);
    return ret;
  }
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
      { ret = NavigateCountCore(job, dir, ~TATTR_NONE, Callstack.size(), maxdepth);
        if (ret)
          return ret;
      } while (&GetCurrent()->GetPlayable() != pp);
    } while (--index);
  }
  return ret;
}

Location::NavigationResult Location::NavigateTime(JobSet& job, PM123_TIME offset, int mindepth, bool absolute)
{ DEBUGLOG(("Location(%p)::NavigateTime({%u,}, %f, %i, %u) - %u\n", this,
    job.Pri, offset, mindepth, absolute, Callstack.size()));
  
  if (Root == NULL)
    return "Cannot Navigate without root.";

  xstring ret;
  const bool direction = offset >= 0;

  for (;;)
  { unsigned maxdepth = UINT_MAX;
    APlayable* cur = GetCurrent();
    if (!cur)
      goto next; // we entered a playlist but did not navigate to an item.
    if (job.RequestInfo(*cur, IF_Tech|IF_Obj|IF_Child|IF_Slice))
      return xstring::empty;
    if (cur->GetInfo().tech->attributes & TATTR_SONG)
    { // Try to navigate within the current song.
      int_ptr<Location> start = cur->GetStartLoc();
      int_ptr<Location> stop = cur->GetStopLoc();
      PM123_TIME begin = start && start->GetPosition() > 0 ? start->GetPosition() : 0;
      PM123_TIME end = stop && stop->GetPosition() >= 0 ? stop->GetPosition() : cur->GetInfo().obj->songlength;
      if (direction)
      { if (Position < 0)
          Position = absolute ? 0 : begin;
        Position += offset;
        if (Position < end)
          return ret; // Navigation within song succeeded
        else if (end < 0)
          return "Indeterminate song length.";
        offset = Position - end; // remaining part
      } else // Backwards
      { if (Position < 0)
        { Position = absolute ? cur->GetInfo().obj->songlength : end;
          if (Position < 0)
            return "Indeterminate song length.";
        }
        Position += offset;
        if (Position >= begin)
          return ret; // Navigation within song succeeded
        offset = Position - begin; // remaining part
      }
    } else
    { // Playlist => try to skip entirely
      PlayableSet exclude;
      size_t i = Callstack.size();
      if (i)
      { exclude.reserve(--i);
        while (i)
          exclude.add(Callstack[--i]->GetPlayable());
        if (exclude.contains(cur->GetPlayable()))
          goto next; // recursive list
      }
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
      maxdepth = Callstack.size();
    }
   next:
    ret = NavigateCountCore(job, direction, TATTR_SONG|TATTR_PLAYLIST, mindepth, maxdepth);
    if (ret)
      return ret;
  }
}

xstring Location::Serialize(bool withpos, char delimiter) const
{ DEBUGLOG(("Location(%p)::Serialize(%u, %c)\n", this, withpos, delimiter));
  ASSERT(strchr(";\t\n", delimiter));
  /*if (!Root)
    return xstring();*/ // NULL
  xstringbuilder ret;
  Playable* list = Root;
  for (const int_ptr<PlayableInstance>* cpp = Callstack.begin(); cpp != Callstack.end(); ++cpp)
  { // next
    if (ret.length())
      ret.append(delimiter);
    if (*cpp)
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
  }
  if (withpos && Position != -2)
  { if (ret.length())
      ret.append(delimiter);
    if (Position >= 0)
    { char buf[32];
      ret.append(PM123Time::toString(buf, Position));
    }
  }
  return ret;
}

Location::NavigationResult Location::Deserialize(JobSet& job, const char*& str)
{ DEBUGLOG(("Location(%p)::Deserialize({%u, }, %s) - %s\n", this, job.Pri, str, Serialize(true).cdata()));
  NavigationResult ret;
  bool flat = false;
  size_t len = 0;
  
  for (;; str += len)
  { xstring url;
    int index = 1;
    DEBUGLOG(("Location::Deserialize now at %s\n", str));
    // Determine type of part
    switch (*str)
    {case 0: // end
      return ret; // NULL

     case ' ':
     case '\t': // whitespace => skip
      len = strspn(++str, " \t");
      continue;

     case ';':
     case '\r':
     case '\n': // delimiter => enter (whatever to enter)
      ret = NavigateInto(job);
      if (ret)
        return ret;
      flat = false;
      len = strspn(++str, ";\r\n");
      continue;
      
     case '*': // Flat navigation
      flat = true;
      len = 1;
      continue;
     
     case '"': // Quoted playlist item
      { const char* ep = strchr(str+1, '"');
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
            if (sscanf(ep, "[%i]%n", &index, &n) != 1 || n != len2 || index == 0)
            { str = ep;
              return ret.sprintf("Syntax: invalid index at %.*s", len2, ep);
            }
          }
        }
      }
      break;

     case '0':
     case '1':
     case '2':
     case '3':
     case '4':
     case '5':
     case '6':
     case '7':
     case '8':
     case '9':
     case '.':
     case '+':
     case '-':
      { len = strcspn(str+1, ";\r\n");
        size_t end = strspn(str+1, "0123456789:. \t");
        if (end == len && strcspn(str, "0123456789") <= end)
        { // Time value
          while (str[len] == ' ' || str[len] == '\t')
            --len;
          ++len;
          const char* cp = str;
          bool sign = false;
          if (*cp == '-')
          { sign = true;
            ++cp;
          } else if (*cp == '+')
            ++cp;
          double t[4] = {0};
          double* dp = t;
          for(;;)
          { size_t n = 0;
            sscanf(cp, "%lf%n", dp, &n);
            cp += n;
            if (cp == str + len)
              break;
            if (*cp != ':')
            { len -= cp - str;
              str = cp;
              return ret.sprintf("Syntax: invalid time value at %.*s\n", len, cp);
            }
            do
            { if (++dp == t + sizeof t / sizeof *t)
              { len -= cp - str;
                str = cp;
                return ret.sprintf("Syntax: to many ':' at %.*s\n", len, cp);
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
          NavigateTime(job, t[0], GetLevel(), true);
          continue;
        }
      }
     default: // unquoted playlist item
      { len = strcspn(str, ";\r\n");
        while (str[len-1] == ' ' || str[len-1] == '\t')
          --len;
        if (str[len-1] == ']')
        { // with index
          char* ep = strnrchr(str, '[', len-1);
          size_t n;
          if (ep == NULL || sscanf(ep+1, "%u%n", &index, &n) != 1 || (int)n != str+len-ep-2 || index == 0)
          { len -= ep - str;
            str = ep+1;
            return ret.sprintf("Syntax: invalid index at %*s", len, ep);
          }
          if (ep != str)
            url.assign(str, ep-str);
            // otherwise only index is specified => leave url NULL instead of ""
        } else
          url.assign(str, len);
    } }
    // do the navigation
    bool noenter = url == "." || url == "..";
    ret = Navigate(job, url, index, flat);
    if (ret)
      return ret;
    if (noenter) // skip delimiter to enter after "." or ".."
      len = strspn(str += len, ";\r\n");
    flat = false;
  } // next part
}

int Location::CompareTo(const Location& r, CompareOptions options, unsigned level) const
{ DEBUGLOG(("Location(%p)::CompareTo(%p, %x, %u) - %s - %s\n", this, &r, options, level, Serialize().cdata(), r.Serialize().cdata()));
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
       lNULL:
        return (options & CO_Reverse) ? level : -level; // other Location is deeper
      // Callstack identical => compare location below
      if ((options & CO_IgnorePosition) || Position == r.Position)
        return 0; // same
      ++level; // Location difference returns level+1
     rNULL:
      return Position > r.Position ? level : -level;
    }
    if (rcpp == r.Callstack.end())
      return (options & CO_Reverse) ? -level : level; // current Location is deeper

    // Check their order
    if (*lcpp != *rcpp) // Instance equality is equivalent to equality.
    { if (!*lcpp)
        goto lNULL;
      if (!*rcpp)
        goto rNULL;
      // Lock the parent collection to make the comparison below reliable.
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

/*InfoFlags Location::AddFrontAggregate(AggregateInfo& dest, InfoFlags what, Priority pri, size_t level)
{ DEBUGLOG(("Location(%p)::AddFrontAggregate(&%p{%s,}, %x, %u, %u)\n", this, &dest, dest.Exclude.size(), what, pri, level));
  ASSERT(level <= GetLevel());
  ASSERT((what & ~IF_Aggreg) == 0);
  // Position
  if ((what & IF_Drpl) && Position > 0)
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
}*/

