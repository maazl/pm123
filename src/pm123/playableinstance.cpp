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


#include "playableinstance.h"
#include "playable.h"

#include <debuglog.h>


/****************************************************************************
*
*  class PlayableInstance
*
****************************************************************************/

PlayableInstance::PlayableInstance(const Playable& parent, APlayable& refto)
: PlayableRef(refto),
  Parent(&parent),
  Index(0)
{ DEBUGLOG(("PlayableInstance(%p)::PlayableInstance(&%p{%s}, &%p{%s})\n", this,
    &parent, parent.DebugName().cdata(), &refto, refto.DebugName().cdata()));
}

xstring PlayableInstance::DoDebugName() const
{ return xstring().sprintf("@%i:%s", Index, RefTo.get()->DebugName().cdata());
}

/*void PlayableInstance::Swap(PlayableRef& r)
{ DEBUGLOG(("PlayableInstance(%p)::Swap(&%p)\n", this, &r));
  ASSERT(Parent == ((PlayableInstance&)r).Parent);
  // Analyze changes
  StatusFlags events = SF_None;
  if (GetAlias() != r.GetAlias())
    events |= SF_Alias;
  if ( CompareSliceBorder(GetStart(), r.GetStart(), SB_Start)
    || CompareSliceBorder(GetStop(),  r.GetStop(),  SB_Stop) )
    events |= SF_Slice;
  // Execute changes
  if (events)
  { PlayableRef::Swap(r);
    StatusChange(change_args(*this, events));
  }
}*/

/*bool operator==(const PlayableInstance& l, const PlayableInstance& r)
{ return l.Parent == r.Parent
      && l.RefTo  == r.RefTo  // Instance equality is sufficient in case of the Playable class.
      && l.GetAlias()    == r.GetAlias();
//      && (const Slice&)l == (const Slice&)r;
}*/

/*int PlayableInstance::CompareTo(const PlayableInstance& r) const
{ if (this == &r)
    return 0;
  int li = Index;
  int ri = r.Index;
  // Removed items or different collection.
  if (li == 0 || ri == 0 || Parent != r.Parent)
    return INT_MIN;
  // Attension: nothing is locked here except for the life-time of the parent playlists.
  // So the item l or r may still be removed from the collection by another thread.
  return li - ri;
}*/
