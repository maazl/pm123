/* * Copyright 2007-2007 Marcel Mueller
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


#include "repository.h"

#include <debuglog.h>


StringRepository::StringRepository(size_t size)
: Data((IStringComparable**)malloc(size * sizeof *Data)),
  Size(size),
  Count(0)
{ assert(size != 0);
}

bool StringRepository::Search(const char* str, size_t& pos) const
{ DEBUGLOG(("StringRepository::Search(%s)\n", str));
  // binary search
  size_t l = 0;
  size_t r = Count;
  while (l < r)
  { size_t m = (l+r) >> 1;
    DEBUGLOG2(("StringRepository::Search - %u, %u, %u->%p\n", l, r, m, Data[m]));
    int cmp = Data[m]->CompareTo(str);
    if (cmp == 0)
    { pos = m;
      DEBUGLOG(("StringRepository::Search - found: %u\n", m));
      return true;
    }
    if (cmp < 0)
      l = m+1;
    else
      r = m;
  }
  pos = l;
  DEBUGLOG(("StringRepository::Search - not found: %u\n", l));
  return false;
}

IStringComparable* StringRepository::Find(const char* str) const
{ DEBUGLOG(("StringRepository::Find(%s)\n", str));
  size_t p;
  return Search(str, p) ? Data[p] : NULL;
}

IStringComparable*& StringRepository::Get(const char* str)
{ DEBUGLOG(("StringRepository::Get(%s) - %p\n", str, Data));
  size_t p;
  if (Search(str, p))
    return Data[p];
  // rezize buffer?
  DEBUGLOG2(("StringRepository::Get - %u, %u\n", Size, Count));
  if (Count >= Size)
  { Size <<= 1;
    // Note: The Block allocated here is never freed, because it is required the whole time.
    // But it may be reallocated here.
    Data = (IStringComparable**)realloc(Data, Size * sizeof *Data);
  }
  IStringComparable** pp = Data + p;
  // insert new entry
  DEBUGLOG(("StringRepository::Get - %p, %p, %u, %u\n", Data, pp, p, (Count-p) * sizeof *Data));
  memmove(pp+1, pp, (Count-p) * sizeof *Data);
  ++Count;
  // return an initial slot
  return *pp = NULL; 
}

IStringComparable* StringRepository::Remove(const char* str)
{ DEBUGLOG(("StringRepository::Remove(%s)\n", str));
  size_t p;
  if (!Search(str, p))
    return NULL;
  IStringComparable** pp = Data + p;
  IStringComparable* ret = *pp;
  --Count;
  memmove(pp, pp+1, (Count-p) * sizeof *Data);
  return ret;
}
