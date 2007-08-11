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

#include <memory.h>
#include <string.h>
#include <assert.h>

#include <debuglog.h>


vector_base::vector_base(size_t capacity)
: Data((void**)malloc(capacity * sizeof *Data)),
  Size(0),
  Capacity(capacity)
{ assert(capacity != 0); 
}

vector_base::~vector_base()
{ free(Data);
}

void*& vector_base::insert(size_t where)
{ assert(where <= Size);
  if (Size >= Capacity)
  { Capacity <<= 1;
    Data = (void**)realloc(Data, Capacity * sizeof *Data);
    assert(Data != NULL);
  }
  void** pp = Data + where;
  memmove(pp+1, pp, (Size-where) * sizeof *Data);
  ++Size;
  //DEBUGLOG(("vector_base::insert - %p, %u, %u\n", pp, Size, Capacity));
  return *pp;
}

void* vector_base::erase(size_t where)
{ assert(where < Size);
  void** pp = Data + where;
  --Size;
  void* ret = *pp;
  memmove(pp, pp+1, (Size-where) * sizeof *Data);
  if (Size+16 < (Capacity>>1))
  { Capacity >>= 1;
    Data = (void**)realloc(Data, Capacity * sizeof *Data);
  }
  return ret;
}

