/*
 * Copyright 2007-2007 M.Mueller
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


#include "algorithm.h"

#include <memory.h>

#include <debuglog.h>

bool binary_search_base(const void*const* data, size_t size, int (*fcmp)(const void* elem, const void* key),
  const void* key, size_t& pos)
{ DEBUGLOG(("binary_search_base(&%p, %u, *%p, %p, &%p)\n", data, size, fcmp, key, &pos));
  size_t l = 0;
  size_t r = size;
  while (l < r)
  { size_t m = (l+r) >> 1;
    DEBUGLOG2(("binary_search_base %u-%u %u->%p\n", l, r, m, data[m]));
    // Dirty hack: NULL references do always match exactly
    // This removes problems with the initialization sequence of inst_index.
    if (!data[m])
    { pos = m;
      return true;
    }
    int cmp = (*fcmp)(data[m], key);
    DEBUGLOG2(("binary_search_base cmp = %i\n", cmp));
    if (cmp == 0)
    { pos = m;
      return true;
    }
    if (cmp < 0)
      l = m+1;
    else
      r = m;
  }
  pos = l;
  return false;
}

/*bool binary_search_base(const vector_base& data, int (*fcmp)(const void* elem, const void* key),
  const void* key, size_t& pos)
{ DEBUGLOG(("binary_search_base(&%p{%u}, *%p, %p, &%p)\n", &data, data.size(), fcmp, key, &pos));
  size_t l = 0;
  size_t r = data.size();
  while (l < r)
  { size_t m = (l+r) >> 1;
    DEBUGLOG2(("binary_search_base %u-%u %u->%p\n", l, r, m, data.at(m)));
    // Dirty hack: NULL references do always match exactly
    // This removes problems with the initialization sequence of inst_index.
    if (!data.at(m))
    { pos = m;
      return true;
    }
    int cmp = (*fcmp)(data.at(m), key);
    DEBUGLOG2(("binary_search_base cmp = %i\n", cmp));
    if (cmp == 0)
    { pos = m;
      return true;
    }
    if (cmp < 0)
      l = m+1;
    else
      r = m;
  }
  pos = l;
  return false;
}*/

void rotate_array_base(void** begin, const size_t len, int shift)
{ DEBUGLOG(("rotate_array_base(%p, %u, %i)\n", begin, len, shift));
  if (len <= 1)
    return;
  // normalize shift
  shift = (shift < 0)
   ? -shift % len
   : len-1 - (shift-1) % len; // fucking C++ division operators
  // do the shift
  size_t count = len; // count the number of moves rather than calculating the greatest common divider.
  do
  { register size_t p = shift;
    register void** dp = begin;
    void* t = *dp;
    do
    { void** sp = begin + p;
      *dp = *sp;
      --count;
      p = (p + (size_t)shift) % len;
      dp = sp;
    } while (p);
    *dp = t;
    ++begin;
  } while (--count);
}

void merge_sort_base(void** begin, void** end, int (*comp)(const void* l, const void* r))
{ DEBUGLOG(("merge_sort_base(%p, %p, %p)\n", begin, end, comp));
  void** mid;
  // Section 1: split
  { const size_t count = end - begin;
    // just a few trivial cases
    switch (count)
    {case 2:
      if ((*comp)(begin[0], begin[1]) > 0)
        swap(begin[0], begin[1]);
     case 1:
     case 0:
      return;
    }
    mid = begin + (count >> 1);
    merge_sort(begin, mid, comp);
    merge_sort(mid, end, comp);
  }
  // Section 2: merge
  for(;;)
  { // skip elements already in place
    while ((*comp)(begin[0], mid[0]) <= 0)
    { ++begin;
      if (begin == mid)
        return;
    }
    while ((*comp)(mid[-1], end[-1]) <= 0)
      --end; // cannot give an infinite loop, becaus the above exit
    // swap subarrays
    size_t count = end - begin;
    size_t shift = count - (mid - begin);
    rotate_array_base(begin, count, shift);
    mid = begin + shift;
  }
}

