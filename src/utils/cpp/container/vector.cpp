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


#include "vector.h"
#include "../cpputil.h"

#include <memory.h>

#include <debuglog.h>


vector_base::vector_base(size_t capacity)
: Data(capacity ? new void*[capacity] : NULL),
  Size(0),
  Capacity(capacity)
{}
vector_base::vector_base(size_t size, void* elem, size_t spare)
: Data(size + spare ? new void*[size + spare] : NULL),
  Size(size),
  Capacity(size + spare)
{ void** dp = Data;
  while (size)
  { *dp++ = elem;
    --size;
  }
}
vector_base::vector_base(const vector_base& r, size_t spare)
: Data(r.Size + spare ? new void*[r.Size + spare] : NULL),
  Size(r.Size),
  Capacity(r.Size + spare)
{ DEBUGLOG(("vector_base(%p)::vector_base(&%p, %u)\n", this, &r, spare));
  memcpy(Data, r.Data, Size * sizeof *Data);
}

vector_base::~vector_base()
{ delete[] Data;
}

void vector_base::operator=(const vector_base& r)
{ DEBUGLOG(("vector_base(%p)::operator=(&%p)\n", this, &r));
  prepare_assign(r.Size);
  memcpy(Data, r.Data, Size * sizeof *Data);
}

void vector_base::swap(vector_base& r)
{ ::swap(Data, r.Data);
  ::swap(Size, r.Size);
  ::swap(Capacity, r.Capacity);
}

void** vector_base::find(const void* elem) const
{ void** dp = Data;
  void**const ep = dp + Size;
  while (dp != ep)
  { if (*dp == elem)
      return dp;
    ++dp;
  }
  return NULL;
}

void vector_base::set_size(size_t size)
{ if (size > Size)
  { if (size >= Capacity)
      reserve(size);
    memset(Data + Size, 0, (size-Size) * sizeof *Data);
  }
  Size = size;
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
  if (Size+16 < (Capacity>>2))
  { Capacity >>= 2;
    void** newdata = new void*[Capacity];
    ASSERT(newdata);
    size_t len = (char*)where - (char*)Data;
    memcpy(newdata, Data, len);
    memcpy((char*)newdata + len, where+1, (char*)(Data+Size) - (char*)where);
    // relocate where
    (char*&)where += (char*)newdata - (char*)Data;
    delete[] Data;
    Data = newdata;
  } else
    memmove(where, where+1, (char*)(Data+Size) - (char*)where);
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
  if (Capacity != size)
  { Capacity = size;
    void** newdata = size ? new void*[Capacity] : NULL;
    ASSERT(newdata != NULL || Capacity == 0);
    memcpy(newdata, Data, Size * sizeof *Data);
    delete[] Data;
    Data = newdata;
  }
}

void vector_base::prepare_assign(size_t size)
{ if (size > Capacity || size+16 < Capacity>>2)
  { // Do not use reserve directly because the data is not needed here
    delete[] Data;
    Data = NULL;
    Size = 0;
    Capacity = 0;
    reserve(size);
  }
  Size = size;
}

/*#ifdef DEBUG_LOG
xstring vector_base::debug_dump() const
{ xstring r = xstring::empty;
  for (size_t i = 0; i != size(); ++i)
    if (r.length())
      r = xstring::sprintf("%s, %p", r.cdata(), at(i));
    else
      r = xstring::sprintf("%p", at(i));
  return r;
}
#endif*/


