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

#define INCL_WIN
#define INCL_BASE
#include "songiterator.h"
#include "properties.h"

#include <cpp/mutex.h>
#include <cpp/xstring.h>
#include <strutils.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>


/****************************************************************************
*
*  class SongIterator
*
****************************************************************************/

void SongIterator::Offsets::Add(const Offsets& r)
{ Index += r.Index;
  if (Offset >= 0)
  { if (r.Offset < 0)
      Offset = -1;
    else
      Offset += r.Offset;
  }
}

void SongIterator::Offsets::Sub(const Offsets& r)
{ Index -= r.Index;
  if (Offset >= 0)
  { if (r.Offset < 0)
      Offset = -1;
    else
      Offset -= r.Offset;
  }
}


const SongIterator::Offsets SongIterator::ZeroOffsets(1, 0);


SongIterator::SongIterator()
: Callstack(8),
  Location(0)
{ DEBUGLOG(("SongIterator(%p)::SongIterator()\n", this));
  // Always create a top level entry for Current.
  // TODO: song roots?
  Callstack.append() = new CallstackEntry();
}

SongIterator::SongIterator(const SongIterator& r)
: Callstack(r.Callstack.size() > 4 ? r.Callstack.size() + 4 : 8),
  Exclude(r.Exclude),
  Location(r.Location)
{ DEBUGLOG(("SongIterator(%p)::SongIterator(&%p)\n", this, &r));
  // copy callstack
  for (const CallstackEntry*const* ppce = r.Callstack.begin(); ppce != r.Callstack.end(); ++ppce)
    Callstack.append() = new CallstackEntry(**ppce);
}

SongIterator::~SongIterator()
{ DEBUGLOG(("SongIterator(%p)::~SongIterator()\n", this));
  // destroy callstack entries
  for (CallstackEntry** p = Callstack.begin(); p != Callstack.end(); ++p)
    delete *p;
}

void SongIterator::Swap(SongIterator& r)
{ Callstack.swap(r.Callstack);
  Exclude.swap(r.Exclude);
  swap(Location, r.Location);
}

/*#ifdef DEBUG
void SongIterator::DebugDump() const
{ DEBUGLOG(("SongIterator(%p{%p, {%u }, {%u }})::DebugDump()\n", this, &*Root, Callstack.size(), Exclude.size()));
  for (const CallstackEntry*const* ppce = Callstack.begin(); ppce != Callstack.end(); ++ppce)
    DEBUGLOG(("SongIterator::DebugDump %p{%i, %g, %p}\n", *ppce, (*ppce)->Index, (*ppce)->Offset, &*(*ppce)->Item));
  Exclude.DebugDump();
}
#endif*/

void SongIterator::Reset()
{ DEBUGLOG(("SongIterator(%p)::Reset()\n", this));
  Exclude.clear();
  // destroy callstack entries except for the root.
  while (Callstack.size() > 1)
    delete Callstack.erase(Callstack.size()-1);
  if (GetRoot() && (GetRoot()->GetPlayable()->GetFlags() & Playable::Enumerable))
    Exclude.append() = GetRoot()->GetPlayable();
}

void SongIterator::SetRoot(PlayableSlice* pc)
{ DEBUGLOG(("SongIterator(%p)::SetRoot(%p)\n", this, pc));
  Reset();
  Callstack[0]->Item = pc;
  // The offsets of the root node are zero anyway.
}

SongIterator::Offsets SongIterator::GetOffset() const
{ return Callstack.size() ? (Offsets&)*Callstack[Callstack.size()-1] : ZeroOffsets;
}

void SongIterator::Enter()
{ DEBUGLOG(("SongIterator::Enter()\n"));
  PlayableSlice* pp = GetCurrent(); // The ownership is held anyway by the callstack object.
  ASSERT(pp != NULL);
  ASSERT(pp->GetPlayable()->GetFlags() & Playable::Enumerable);
  // push
  Exclude.get(*pp->GetPlayable()) = pp->GetPlayable();
  // We must do this in two steps, because of the side effects of Callstack.append().
  CallstackEntry* cep = new CallstackEntry((Offsets&)*Callstack[Callstack.size()-1]);
  Callstack.append() = cep;
  Location = 0;
}

void SongIterator::Leave()
{ DEBUGLOG(("SongIterator::Leave()\n"));
  ASSERT(Callstack.size() > 1); // Can't remove the last item.
  delete Callstack.erase(Callstack.end()-1);
  RASSERT(Exclude.erase(*GetCurrent()->GetPlayable()));
  Location = 0;
}

int SongIterator::GetLevel() const
{ return GetCurrent() ? Callstack.size() : Callstack.size()-1;
}

PlayableCollection* SongIterator::GetList() const
{ DEBUGLOG(("SongIterator(%p)::GetList()\n", this));
  PlayableSlice* ps = GetCurrent();
  if (!ps)
    return NULL;
  if (ps->GetPlayable()->GetFlags() & Playable::Enumerable)
    return (PlayableCollection*)ps->GetPlayable();
  if (Callstack.size() == 1) // no playlist in the callstack
    return NULL;
  return (PlayableCollection*)Callstack[Callstack.size()-2]->Item->GetPlayable();
}

SongIterator::Offsets SongIterator::TechFromPlayable(PlayableCollection* pc)
{ Mutex::Lock lck(pc->Mtx);
  const PlayableCollection::CollectionInfo& ci = pc->GetCollectionInfo(Exclude);
  return Offsets(ci.Items, ci.Songlength);
} 
SongIterator::Offsets SongIterator::TechFromPlayable(Playable* pp)
{ if (pp->GetFlags() & Playable::Enumerable)
    return TechFromPlayable((PlayableCollection*)pp);
  // Song => use tech info
  return Offsets(1, pp->GetInfo().tech->songlength);
} 

void SongIterator::PrevCore()
{ DEBUGLOG(("SongIterator(%p)::PrevCore()\n", this));
  CallstackEntry* const pce = Callstack[Callstack.size()-1];
  if (pce->Item == NULL)
  { // Navigate to the last item.
    // Check stop positions first.
    for (int i = 0; i < Callstack.size()-1; ++i)
    { CallstackEntry* pce2 = Callstack[i];
      if (pce2->Item->GetStop()) // Stop Position exists
      { int cmp = CompareTo(*pce2->Item->GetStop(), i); // Here we go O(n^2)
        DEBUGLOG(("SongIterator::PrevCore: StopCompare %i, %i\n", i, cmp));
        if (-cmp > i) // Stop Position is behind current
        { // This implies that pce2->Stop has at least i levels
          pce->Item = pce2->Item->GetStop()->GetCallstack()[i]->Item;
          // calc offsets
          (Offsets&)*pce = *pce2;
          pce->Add(*pce2->Item->GetStop()->GetCallstack()[i]);
        }
      }
    }
    if (pce->Item == NULL)
    { CallstackEntry* pce2 = Callstack[Callstack.size()-2];
      pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetPrev((PlayableInstance*)pce->Item.get());
      if (pce->Item)
      { // calculate offsets from parent
        DEBUGLOG(("SongIterator::PrevNextCore - reverse parent offset\n"));
        (Offsets&)*pce = *pce2;
        // calc from the end
        // += length(parent)
        pce->Add(TechFromPlayable((PlayableCollection*)pce2->Item->GetPlayable()));
        // -= length(current)
        pce->Sub(TechFromPlayable(pce->Item->GetPlayable()));
      }
    }
  } else
  { pce->Item = ((PlayableCollection*)Callstack[Callstack.size()-2]->Item->GetPlayable())->GetPrev((PlayableInstance*)pce->Item.get());
    if (pce->Item)
    { // calc offets
      pce->Sub(TechFromPlayable(pce->Item->GetPlayable()));
    }
  }
  if (pce->Item != NULL)
  { // Check wether we crossed some start iterator
    for (int i = 0; i < Callstack.size()-1; ++i)
    { CallstackEntry* pce2 = Callstack[i];
      if (pce2->Item->GetStart()) // Start Position exists
      { int cmp = CompareTo(*pce2->Item->GetStart(), i); // Here we go O(n^2)
        DEBUGLOG(("SongIterator::PrevCore: StartCompare %i, %i\n", i, cmp));
        if (-cmp > i) // Start Position is behind current
        { // This implies that pce2->Start has at least i levels
          pce->Item = NULL;
          break;
        }
      }
    }
  }
}

void SongIterator::NextCore()
{ DEBUGLOG(("SongIterator(%p)::NextCore()\n", this));
  CallstackEntry* const pce = Callstack[Callstack.size()-1];
  if (pce->Item == NULL)
  { // Navigate to the first item.
    // Check starting positions first.
    for (int i = 0; i < Callstack.size()-1; ++i)
    { CallstackEntry* pce2 = Callstack[i];
      if (pce2->Item->GetStart()) // Start Position exists
      { int cmp = CompareTo(*pce2->Item->GetStart(), i); // Here we go O(n^2)
        DEBUGLOG(("SongIterator::NextCore: StartCompare %i, %i\n", i, cmp));
        if (cmp > i) // Start Position is behind current
        { // This implies that pce2->Start has at least i levels
          pce->Item = pce2->Item->GetStart()->GetCallstack()[i]->Item;
          // calc offsets
          (Offsets&)*pce = *pce2;
          pce->Add(*pce2->Item->GetStart()->GetCallstack()[i]);
        }
      }
    }
    if (pce->Item == NULL)
    { CallstackEntry* pce2 = Callstack[Callstack.size()-2];
      pce->Item = ((PlayableCollection*)pce2->Item->GetPlayable())->GetNext((PlayableInstance*)pce->Item.get());
      // derive offsets
      (Offsets&)*pce = *pce2;
    }
  } else
  { // calc offsets
    pce->Add(TechFromPlayable(pce->Item->GetPlayable()));
    pce->Item = ((PlayableCollection*)Callstack[Callstack.size()-2]->Item->GetPlayable())->GetNext((PlayableInstance*)pce->Item.get());
  }
  if (pce->Item != NULL)
  { // Check wether we crossed some stop iterator
    for (int i = 0; i < Callstack.size()-1; ++i)
    { CallstackEntry* pce2 = Callstack[i];
      if (pce2->Item->GetStop()) // Stop Position exists
      { int cmp = CompareTo(*pce2->Item->GetStop(), i); // Here we go O(n^2)
        DEBUGLOG(("SongIterator::NextCore: StopCompare %i, %i\n", i, cmp));
        if (-cmp > i) // Stop Position is behind current
        { pce->Item = NULL;
          break;
        }
      }
    }
  }
}

bool SongIterator::PrevNextCore(void (SongIterator::*func)())
{ DEBUGLOG(("SongIterator(%p)::PrevNextCore(%p) - %u\n", this, func, Callstack.size()));
  ASSERT(Callstack.size());
  ASSERT(GetList());
  // Do we have to enter a List first?
  if (GetCurrent()->GetPlayable()->GetFlags() & Playable::Enumerable)
  { GetCurrent()->GetPlayable()->EnsureInfo(Playable::IF_Other);
    Enter();
  }
  for (;;)
  { (this->*func)();
    CallstackEntry* pce = Callstack[Callstack.size()-1];
    const PlayableSlice* pi = pce->Item;
    DEBUGLOG(("SongIterator::PrevNextCore - {%i, %g, %p}\n", pce->Index, pce->Offset, pi));
    if (pi == NULL)
    { // end of list => leave
      Leave();
      if (Callstack.size() == 1)
      { DEBUGLOG(("SongIterator::PrevNextCore - NULL\n"));
        return false;
      }
    } else
    { // item found => check wether it is enumerable
      if (!(pi->GetPlayable()->GetFlags() & Playable::Enumerable))
      { Location = 0;
        DEBUGLOG(("SongIterator::PrevNextCore - {%i, %g, %p{%p{%s}}}\n", pce->Index, pce->Offset, &*pi, pi->GetPlayable(), pi->GetPlayable()->GetURL().getShortName().cdata()));
        return true;
      } else if (!IsInCallstack(GetCurrent()->GetPlayable())) // skip objects in the call stack
      { pi->GetPlayable()->EnsureInfo(Playable::IF_Other);
        Enter();
      }
    }
  }
}

bool SongIterator::Navigate(const xstring& url, int index)
{ DEBUGLOG(("SongIterator(%p)::Navigate(%s, %i)\n", this, url ? url.cdata() : "<null>", index));
  PlayableCollection* list = GetList();
  ASSERT(list);
  if (index == 0)
    return false;
  // lock collection
  Mutex::Lock lck(list->Mtx);
  // search item in the list
  if (!url)
  { // address from back
    if (index < 0)
      index += list->GetInfo().tech->num_items +1;  
    // address by index
    if (index <= 0 || index > list->GetInfo().tech->num_items)
      return false; // index out of range
    CallstackEntry* cep = Callstack[Callstack.size()-1];
    // List is not yet open. Enter it.
    if (cep->Item->GetPlayable() == list)
    { Enter();
      cep = Callstack[Callstack.size()-1];
    }
    cep->Item = NULL; // Start over
    // Offsets structure slicing
    (Offsets&)*cep = (const Offsets&)*Callstack[Callstack.size()-2];
    /*// Speed up: start from back if closer to it
    if ((index<<1) > list->GetInfo().tech->num_items)
    { // iterate from behind
      index = list->GetInfo().tech->num_items - index;
      do
        pi = list->GetPrev(pi);
      while (index--);
    } else*/
    { // iterate from the front
      list->EnsureInfo(Playable::IF_Other);
      for(;;)
      { cep->Item = list->GetNext((PlayableInstance*)cep->Item.get());
        if (--index == 0)
          break;
        // adjust offsets
        if (!IsInCallstack(cep->Item->GetPlayable()))
          cep->Add(TechFromPlayable(cep->Item->GetPlayable()));
      }
      // TODO: conflicting start offsets?
    }
  } else if (url == "..")
  { do
    { // navigate one level up
      if (list == GetRoot()->GetPlayable())
        return false; // Error: can't navigate above the top level.
      Leave();
    } while (--index);
    return true;
  } else
  { // look for a certain URL
    url123 absurl = list->GetURL().makeAbsolute(url);
    const int_ptr<Playable>& pp = Playable::FindByURL(absurl);
    if (pp == NULL)
      return false; // If the url is in the playlist it /must/ exist in the repository, otherwise => error.
    if (IsInCallstack(pp))
      return false; // Error: Cannot navigate to recursive item
    // List is not yet open. Enter it.
    if (GetCurrent()->GetPlayable() == list)
      Enter();
    CallstackEntry ce((const Offsets&)*Callstack[Callstack.size()-2]);
    if (index > 0)
    { // forward lookup
      for(;;)
      { ce.Item = list->GetNext((PlayableInstance*)ce.Item.get());
        if (ce.Item == NULL)
          return false; // Error: not found
        if (stricmp(ce.Item->GetPlayable()->GetURL(), absurl) == 0 && --index == 0)
          break;
        // adjust offsets
        if (!IsInCallstack(ce.Item->GetPlayable()))
          ce.Add(TechFromPlayable(ce.Item->GetPlayable()));
      }
    } else  
    { // reverse lookup
      do
      { ce.Item = list->GetPrev((PlayableInstance*)ce.Item.get());
        if (ce.Item == NULL)
          return false; // Error: not found
      } while (stricmp(ce.Item->GetPlayable()->GetURL(), absurl) != 0 || --index != 0);
      // adjust offsets (always from front)
      PlayableInstance* pi = (PlayableInstance*)ce.Item.get();
      while ((pi = list->GetPrev(pi)) != NULL)
        if (!IsInCallstack(pi->GetPlayable()))
          ce.Add(TechFromPlayable(pi->GetPlayable()));
    }
    // commit
    *Callstack[Callstack.size()-1] = ce;
  }
  /*// Auto-enter playlist objects
  if (GetCurrent() && (GetCurrent()->GetPlayable()->GetFlags() & Playable::Enumerable))
    Enter();*/
  return true;
}

xstring SongIterator::Serialize(bool withlocation) const
{ DEBUGLOG(("SongIterator(%p)::Serialize()\n", this));
  if (GetRoot() == NULL)
    return xstring(); // NULL
  xstring ret = xstring::empty;
  const CallstackEntry*const* cpp = Callstack.begin();
  const Playable* list = GetRoot()->GetPlayable();
  while (++cpp != Callstack.end()) // Start at Callstack[1]
  { const PlayableInstance* pi = (const PlayableInstance*)(*cpp)->Item.get();
    if (pi == NULL || !(pi->GetPlayable()->GetFlags() & Playable::Enumerable))
      break;
    // Fetch info
    const url123& url = pi->GetPlayable()->GetURL();
    // append url to result
    ret = ret + "\"" + url.makeRelative(list->GetURL()) + "\"";
    // check if the item is unique
    unsigned n = 1;
    const PlayableInstance* pi2 = pi;
    while ((pi2 = ((PlayableCollection*)list)->GetPrev(pi2)) != NULL)
      if (stricmp(pi2->GetPlayable()->GetURL(), url) == 0)
        ++n;
    if (n > 1)
    { // Not unique => provide index
      char buf[12];
      sprintf(buf, "%u", n);
      ret = ret + buf;
    }
    // next
    ret = ret + ";";
    list = pi->GetPlayable();
  }
  if (withlocation && Location)
  { char buf[30];
    char* cp = buf;
    if (Location < 0)
      *cp++ = '-';
    T_TIME loc = fabs(Location);
    unsigned long secs = (unsigned long)loc;
    if (secs >= 86400)
      sprintf(cp, "%d:%02d:%02d", secs / 86400, secs / 3600 % 60, secs / 60 % 60);
    else if (secs > 3600) 
      sprintf(cp, "%d:%02d", secs / 3600, secs / 60 % 60);
    else
      sprintf(cp, "%d", secs / 60);
    cp += strlen(cp);
    sprintf(cp, ":%02d", secs % 60);
    loc -= secs;
    ASSERT(loc >= 0 && loc < 1);
    if (loc)
      sprintf(cp+3, "%f", loc);
    ret = ret + buf;
  }
  return ret;
}

bool SongIterator::Deserialize(const char*& str)
{ DEBUGLOG(("SongIterator(%p)::Deserialize(%s)\n", this, str));
  ASSERT(GetRoot());
  for (; *str; str += strspn(str, ";\r\n"))
  { PlayableSlice* pi = Callstack[Callstack.size()-1]->Item;
    if (pi == NULL || (pi->GetPlayable()->GetFlags() & Playable::Enumerable))
    { // Playlist navigation
      // parse part into url, index
      xstring url;
      size_t index = 1;
      if (str[0] == '"')
      { // quoted item
        const char* ep = strchr(str+1, '"');
        if (ep == NULL)
        { url = str+1;
          str += 1 + url.length(); // end of string => set str to the terminating \0
        } else
        { url.assign(str+1, ep-str-1);
          ++ep; // move behind the '"'
          // look for index
          size_t len = strcspn(ep, ";\r\n");
          if (len)
          { size_t n;
            if (sscanf(ep, "[%u]%n", &index, &n) != 1 || n != len || index == 0)
            { str = ep;
              DEBUGLOG(("SongIterator::Deserialize: invalid index at %s\n", str));
              return false; // Syntax error (invalid index)
            }
          }
          str = ep + len; 
        }
      } else
      { // unquoted => find delimiter
        size_t len = strcspn(str, ";\r\n");
        if (len == 0)
        { // String starts with a part seperator => use absolute navigation
          Reset();
          continue;
        }
        if (str[len-1] == ']')
        { // with index
          char* ep = strnrchr(str, '[', len);
          size_t n;
          if (ep == NULL || sscanf(ep+1, "%u%n", &index, &n) != 1 || n != str+len-ep-2 || index == 0)
          { str = ep+1;
            DEBUGLOG(("SongIterator::Deserialize: invalid index (2) at %s\n", ep));
            return false; // Syntax error (invalid index)
          }
          if (ep != str)
            url.assign(str, ep-str);
            // otherwise only index is specified => leave url NULL instead of "" 
        } else
          url.assign(str, len);
        str += len;
      }

      // do the navigation
      Navigate(url, index);

    } else
    { // Song navigation
      bool sign = false;
      if (*str == '-')
      { sign = true;
        ++str;
      }
      T_TIME t[4] = {0};
      T_TIME* dp = t;
      while (*str)
      { size_t n = 0;
        sscanf(str, "%lf%n", dp, &n);
        str += n;
        if (*str == 0)
          break;
        if (*str != ':')
        { DEBUGLOG(("SongIterator::Deserialize: invalid character at %s\n", str));
          return false; // Error: Invalid character
        }
        do
        { if (++dp == t + sizeof t / sizeof *t)
          { DEBUGLOG(("SongIterator::Deserialize: to many ':' at %s\n", str));
            return false; // Error: too many ':'
          } 
        } while (*++str == ':');
      }
      
      // make time
      if (dp-- != t)
        dp[0] = 60*dp[0] + dp[1]; // Minutes
      if (dp-- != t)
        dp[0] = 3600*dp[0] + dp[1]; // Hours
      if (dp-- != t)
        dp[0] = 86400*dp[0] + dp[1]; // Days

      // do the navigation
      T_TIME songlen = GetCurrent()->GetPlayable()->GetInfo().tech->songlength;
      if (sign)     
      { // reverse location
        if (songlen < 0)
        { DEBUGLOG(("SongIterator::Deserialize: cannot navigate %f from back with unknown songlength\n", songlen));
          return false; // Error: cannot navigate from back if the song length is unknown.
        }
        t[0] = songlen - t[0];
        if (t[0] < 0)
        { DEBUGLOG(("SongIterator::Deserialize: %f is beyond the end of the song at %f\n", songlen, t[0]));
          return false; // Error: offset larger than the song
        }
      } else if (songlen >= 0 && t[0] > songlen)
      { DEBUGLOG(("SongIterator::Deserialize: %f is beyond the end of the song at %f\n", songlen, t[0]));
        return false; // Error: offset larger than the song
      }
      SetLocation(t[0]);
    }
  } // next part
  return true; 
}

int SongIterator::CompareTo(const SongIterator& r, int level) const 
{ DEBUGLOG(("SongIterator(%p)::CompareTo(%p)\n", this, &r));
  ASSERT(level < Callstack.size());
  const SongIterator::CallstackEntry*const* lcpp = Callstack.begin() + level;
  const SongIterator::CallstackEntry*const* rcpp = r.Callstack.begin();
  if ((*lcpp)->Item == NULL && (*rcpp)->Item == NULL)
    return 0; // Both iterators are unassigned
  if ( (*lcpp)->Item == NULL || (*rcpp)->Item == NULL
    || (*lcpp)->Item->GetPlayable() != (*rcpp)->Item->GetPlayable() )
    return INT_MIN; // different root
  for (;;)
  { // Fetch next level
    ++lcpp;
    ++rcpp;
    ++level;
    if (lcpp == Callstack.end() || (*lcpp)->Item == NULL)
    { if (rcpp != r.Callstack.end() && (*rcpp)->Item != NULL)
        return -level; // current SongIterator is deeper
      // Item identical => compare location below
      if (Location == r.Location || (Location < 0 && r.Location < 0))
        return 0; // same
      ++level; // Location difference returns level+1
      return Location < 0 || (r.Location >= 0 && Location > r.Location) ? level : -level;
    }
    if (rcpp == r.Callstack.end() || (*rcpp)->Item == NULL)
      return level; // current SongIterator is deeper
    // Not the same Item => check their order
    if ((*lcpp)->Item != (*rcpp)->Item)
    { // Check ordering of the Items
      int cmp = ((PlayableInstance*)(*lcpp)->Item.get())->CompareTo(*(PlayableInstance*)(*rcpp)->Item.get());
      if (cmp != 0)
        return cmp > 0 ? level : -level;
    }
  }
}
