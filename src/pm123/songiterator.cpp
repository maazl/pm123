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

#include "songiterator.h"

#include <debuglog.h>


/****************************************************************************
*
*  class SongIterator
*
****************************************************************************/

SongIterator::OffsetInfo& SongIterator::OffsetInfo::operator+=(const OffsetInfo& r)
{ if (Index >= 0)
  { if (r.Index >= 0)
      Index += r.Index;
    else
      Index = -1;
  }
  if (Time >= 0)
  { if (r.Time >= 0)
      Time += r.Time;
    else
      Time = -1;
  }
  return *this;
}

void SongIterator::SetRoot(Playable* root)
{ int_ptr<Playable> ptr(root);
  ptr.toCptr();
  ptr.fromCptr(GetRoot());
  Location::SetRoot(root);
}

SongIterator& SongIterator::operator=(const SongIterator& r)
{ DEBUGLOG(("SongIterator(%p)::operator=(&%p)\n", this, &r));
  int_ptr<Playable> ptr(r.GetRoot());
  ptr.toCptr();
  ptr.fromCptr(GetRoot());
  Location::operator=(r);
  return *this;
}

SongIterator::OffsetInfo SongIterator::CalcOffsetInfo(size_t level)
{ DEBUGLOG(("SongIterator(%p)::GetOffsetInfo(%u)\n", this, level));
  ASSERT(level <= GetLevel());
  if (level == GetLevel())
    return OffsetInfo(0, GetPosition());
  OffsetInfo off = CalcOffsetInfo(level+1);

  return off;
}
