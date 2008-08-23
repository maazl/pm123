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


#include "container.h"
#include "cpputil.h"

#include <memory.h>
#include <string.h>

#include <debuglog.h>


vector_base::vector_base(size_t capacity)
: Data((void**)malloc(capacity * sizeof *Data)),
  Size(0),
  Capacity(capacity)
{}

vector_base::vector_base(const vector_base& r, size_t spare)
: Data((void**)malloc((r.Size + spare) * sizeof *Data)),
  Size(r.Size),
  Capacity(r.Size + spare)
{ DEBUGLOG(("vector_base(%p)::vector_base(&%p, %u)\n", this, &r, spare));
  memcpy(Data, r.Data, Size * sizeof *Data);
}

vector_base::~vector_base()
{ free(Data);
}

void vector_base::operator=(const vector_base& r)
{ prepare_assign(r.Size);
  memcpy(Data, r.Data, Size * sizeof *Data);
}

void vector_base::swap(vector_base& r)
{ ::swap(Data, r.Data);
  ::swap(Size, r.Size);
  ::swap(Capacity, r.Capacity);
}

void*& vector_base::insert(size_t where)
{ ASSERT(where <= Size);
  if (Size >= Capacity)
    reserve(Capacity > 8 ? Capacity << 1 : 16);
  void** pp = Data + where;
  memmove(pp+1, pp, (Size-where) * sizeof *Data);
  ++Size;
  //DEBUGLOG(("vector_base::insert - %p, %u, %u\n", pp, Size, Capacity));
  return *pp = NULL;
}

void*& vector_base::append()
{ if (Size >= Capacity)
    reserve(Capacity > 8 ? Capacity << 1 : 16);
  return Data[Size++] = NULL;
}

void* vector_base::erase(void**& where)
{ DEBUGLOG(("vector_base(%p)::erase(&%p) - %p %u\n", this, where, Data, Size));
  ASSERT(where >= Data && where < Data+Size);
  --Size;
  void* ret = *where;
  memmove(where, where+1, (char*)(Data+Size) - (char*)where);
  if (Size+16 < (Capacity>>2))
  { Capacity >>= 2;
    void** newdata = (void**)realloc(Data, Capacity * sizeof *Data);
    ASSERT(newdata);
    // relocate where
    (char*&)where += (char*)newdata - (char*)Data;
    Data = newdata;
  }
  return ret;
}

void vector_base::move(size_t from, size_t to)
{ ASSERT(from < Size);
  ASSERT(to < Size);
  if (from == to)
    return; // no-op
  void* p = Data[from];
  if (from < to)
    memmove(Data + from, Data + from +1, (to - from) * sizeof *Data);
  else
    memmove(Data + to +1, Data + to, (from - to) * sizeof *Data);
  Data[to] = p;
}

void vector_base::reserve(size_t size)
{ ASSERT(size >= Size);
  Capacity = size;
  Data = (void**)realloc(Data, Capacity * sizeof *Data);
  ASSERT(Data != NULL || Capacity == 0);
}

void vector_base::prepare_assign(size_t size)
{ if (size > Capacity || size+16 < Capacity>>2)
  { // Do not use reserve directly because the data is not needed here
    free(Data);
    Data = NULL;
    Size = 0;
    reserve(size);
  }
  Size = size;
}

bool binary_search_base(const vector_base& data, int (*fcmp)(const void* elem, const void* key),
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
}

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

