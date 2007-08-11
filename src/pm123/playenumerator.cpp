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


#include "playenumerator.h"

#include <cpp/mutex.h>
#include <debuglog.h>


RecursiveEnumerator::RecursiveEnumerator(RecursiveEnumerator* parent)
: Parent(parent),
  Valid(false),
  ListUpdateDelegate(*this, &ListUpdateFn)
{ DEBUGLOG(("RecursiveEnumerator(%p)::RecursiveEnumerator(%p)\n", this, parent));
}

void RecursiveEnumerator::ListUpdateFn(const PlayableCollection::change_args& args)
{ DEBUGLOG(("RecursiveEnumerator(%p)::ListUpdateFn({%p,%p,%u}) - \n", this, args.Collection, args.Item, args.Type));
  // for now we only handle delete events
  if (args.Type != PlayableCollection::Delete)
    return;
  // Note: there is a little chance that we get here while *this is no longer attached to the
  // sender of the event. In this case we simply ignore the event.
  if (!Valid || &args.Collection != &*Root)
    return; // not valid or different object
  // check wether we have references to the deleted object
  if (&**Enumerator == &args.Item)
  { // current item is about to be deleted => prefetch Prev and Next
    PrevEnumerator = Enumerator->Clone();
    PrevEnumerator->Prev();
    NextEnumerator = Enumerator->Clone();
    NextEnumerator->Next();
    Enumerator->Reset();
  } else if (PrevEnumerator != NULL && &**PrevEnumerator == &args.Item)
  { // prefetched previous item is about to be deleted => prefetch the item before
    PrevEnumerator->Prev();
  } else if (NextEnumerator != NULL && &**NextEnumerator == &args.Item)
  { // prefetched next item is about to be deleted => prefetch the next item
    NextEnumerator->Next();
  } 
}

void RecursiveEnumerator::PrevEnum()
{ DEBUGLOG(("RecursiveEnumerator(%p)::PrevEnum() - %p %p %p\n", this, &*Enumerator, &*PrevEnumerator, &*NextEnumerator));
  bool r;
  if (PrevEnumerator != NULL)
  { Enumerator = PrevEnumerator;
    PrevEnumerator = NULL;
    NextEnumerator = NULL;
  } else
  { Enumerator->Prev();
  }
}

void RecursiveEnumerator::NextEnum()
{ DEBUGLOG(("RecursiveEnumerator(%p)::NextEnum() - %p %p %p\n", this, &*Enumerator, &*PrevEnumerator, &*NextEnumerator));
  if (NextEnumerator != NULL)
  { Enumerator = NextEnumerator;
    PrevEnumerator = NULL;
    NextEnumerator = NULL;
  } else
  { Enumerator->Next();
  }
}

RecursiveEnumerator* RecursiveEnumerator::RecursionCheck(const Playable* item)
{ DEBUGLOG(("RecursiveEnumerator(%p)::RecursionCheck(%p)\n", this, item));
  RecursiveEnumerator* stack = this;
  do
  { if (stack->Root == item)
      return stack; // recursion detected
    stack = stack->Parent;
  } while (stack);
  return NULL;
}

void RecursiveEnumerator::Attach(Playable* play)
{ DEBUGLOG(("RecursiveEnumerator(%p)::Attach(%p{%s})\n", this, play, play ? play->GetURL().cdata() : "n/a"));
  ListUpdateDelegate.detach();
  Reset();
  SubIterator    = NULL;
  Enumerator     = NULL;
  assert(PrevEnumerator == NULL && NextEnumerator == NULL);
  Root = play;
  if (Root && (Root->GetFlags() & Playable::Enumerable))
  { Mutex::Lock lock(Root->Mtx);
    Enumerator = ((PlayableCollection&)*Root).GetEnumerator();
    InitNextLevel();
    ((PlayableCollection&)*Root).CollectionChange += ListUpdateDelegate;
    DEBUGLOG(("RecursiveEnumerator::Attach - %p, %p\n", &*SubIterator, &*Enumerator));
  }
}

void RecursiveEnumerator::Reset()
{ DEBUGLOG(("RecursiveEnumerator(%p)::Reset()\n", this));
  Valid = false;
  if (SubIterator != NULL)
  { Enumerator->Reset();
    PrevEnumerator = NULL;
    NextEnumerator = NULL;
  }
}


void PlayEnumerator::InitNextLevel()
{ DEBUGLOG(("PlayEnumerator(%p)::InitNextLevel()\n", this));
  SubIterator = new PlayEnumerator(this);
}

int_ptr<Song> PlayEnumerator::GetCurrentSong() const
{ DEBUGLOG(("PlayEnumerator(%p)::GetCurrentSong()\n", this));
  if (!Valid)
  { DEBUGLOG(("PlayEnumerator::GetCurrentSong: NULL\n"));
    return NULL;
  }
  if (SubIterator != NULL)
    return ((PlayEnumerator&)*SubIterator).GetCurrentSong();
  // must be a song!
  DEBUGLOG(("PlayEnumerator::GetCurrentSong: %p{%s}\n", &*Root, Root->GetURL().cdata()));
  return &(Song&)*Root;
}

int_ptr<Song> PlayEnumerator::PrevNextCore(int_ptr<Song> (PlayEnumerator::*subfn)(), void (PlayEnumerator::*enumfn)())
{ DEBUGLOG(("PlayEnumerator(%p)::PrevNextCore() - %p\n", this, &*SubIterator));
  if (!Root)
    return NULL;
  if (SubIterator == NULL)
  { // single song
    if (!Valid)
    { Valid = true;
      return &(Song&)*Root;
    } else
    { Valid = false;
      return NULL;
  } }
  // root item is enumerable 
  if (!Valid)
  { // start new iteration
    Valid = true;
    goto nextitem;
  }
 nextsubitem:
  DEBUGLOG(("PlayEnumerator::PrevNextCore : subitem\n"));
  { int_ptr<Song> r = (((PlayEnumerator&)(*SubIterator)).*subfn)();
    DEBUGLOG(("PlayEnumerator::PrevNextCore : subitem - %p\n", &*r));
    if (r)
      // has at least one subitem
      return r;
  }
 nextitem:
  DEBUGLOG(("PlayEnumerator::PrevNextCore : item\n"));
  int_ptr<Playable> current;
  // next item
  { Mutex::Lock lock(Root->Mtx);
    (this->*enumfn)();
    if (!Enumerator->IsValid())
    { // no more
      lock.Release();
      Valid = false;
      SubIterator->Attach(NULL);
      DEBUGLOG(("PlayEnumerator::PrevNextCore : last item\n"));
      return NULL;
    }
    current = &(*Enumerator)->GetPlayable();
  }
  DEBUGLOG(("PlayEnumerator::PrevNextCore : next item - %p\n", &*current));
  if (RecursionCheck(current))
    goto nextitem; // Skip recursive items
  SubIterator->Attach(current);
  goto nextsubitem;
}

PlayEnumerator::Status PlayEnumerator::GetStatus() const
{ DEBUGLOG(("PlayEnumerator(%p)::GetStatus()\n", this));
  Status s = {0};
  if (!Root) // not attached => all fileds zero
    return s;
  // Fill constant fileds first
  TECH_INFO tech = *Root->GetInfo().tech;
  s.TotalTime = tech.songlength;
  s.TotalItems = tech.num_items;
  if (SubIterator != NULL) // playlist
  { // look for subitems
    Mutex::Lock lock(Root->Mtx);
    sco_ptr<PlayableEnumerator> pe = (NextEnumerator != NULL ? NextEnumerator : Enumerator)->Clone();
    while (pe->Prev())
    { tech = *(*pe)->GetPlayable().GetInfo().tech;
      if (s.CurrentTime >= 0)
      { if (tech.songlength >= 0)
        { s.CurrentTime += tech.songlength;
        } else
        { s.CurrentTime = tech.songlength;
          if (s.CurrentItem < 0)
            break; // makes no more sense
        }
      }
      if (s.CurrentItem >= 0)
      { if (tech.num_items >= 0)
        { s.CurrentItem += tech.num_items;
        } else
        { s.CurrentItem = tech.num_items;
          if (s.CurrentTime < 0)
            break; // makes no more sense
        }
      }
    }
  } else
  { s.CurrentItem = 1;
    s.TotalItems = 1;
  }
  DEBUGLOG(("PlayEnumerator::GetStatus: {%i/%i, %f/%f}\n", s.CurrentItem, s.TotalItems, s.CurrentTime, s.TotalTime));
  return s;
}


void StatusPlayEnumerator::InitNextLevel()
{ DEBUGLOG(("StatusPlayEnumerator(%p)::InitNextLevel()\n", this));
  SubIterator = new StatusPlayEnumerator(this);
}

void StatusPlayEnumerator::PrevEnum()
{ DEBUGLOG(("StatusPlayEnumerator(%p)::PrevEnum() - %p\n", this, &*Enumerator));
  // Remove in use flag from the last item unless the item itself has been removed.
  if (Enumerator->IsValid())
    (*Enumerator)->SetInUse(false);
  PlayEnumerator::PrevEnum();
  // Set in use flag of the new item if any.
  if (Enumerator->IsValid())
    (*Enumerator)->SetInUse(true);
}

void StatusPlayEnumerator::NextEnum()
{ DEBUGLOG(("StatusPlayEnumerator(%p)::NextEnum() - %p\n", this, &*Enumerator));
  // Remove in use flag from the last item unless the item itself has been removed.
  if (Enumerator->IsValid())
    (*Enumerator)->SetInUse(false);
  PlayEnumerator::NextEnum();
  // Set in use flag of the new item if any.
  if (Enumerator->IsValid())
    (*Enumerator)->SetInUse(true);
}

void StatusPlayEnumerator::Reset()
{ DEBUGLOG(("StatusPlayEnumerator(%p)::Reset() - %p\n", this, &*Enumerator));
  // Remove in use flag from the last item unless the item itself has been removed.
  if (SubIterator != NULL && Enumerator->IsValid())
    (*Enumerator)->SetInUse(false);
  PlayEnumerator::Reset();
}

void StatusPlayEnumerator::Attach(Playable* play)
{ DEBUGLOG(("StatusPlayEnumerator(%p)::Attach(%p) - %p\n", this, play, &*Root));
  if (Root)
    Root->SetInUse(false);
  PlayEnumerator::Attach(play);
  if (Root)
    Root->SetInUse(true);
}
