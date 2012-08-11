/*
 * Copyright 2011-2011 Marcel Mueller
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


#include <stdlib.h>
#include <memory.h>
#include <debuglog.h>


#define SCALAR_MAGIC 0xdddddddd
#define ARRAY_MAGIC 0xeeeeeeee
#define NEW_MAGIC 0xbbbbbbbb
#define FREE_MAGIC 0xcccccccc


struct preamble
{ size_t   length;
  unsigned magic;
};


void* operator new(size_t len)
{ if (!len)
    return NULL;
  preamble* storage = (preamble*)malloc(len + sizeof(preamble) + sizeof(int));
  storage->length = len;
  storage->magic = SCALAR_MAGIC;
  ++storage;
  memset(storage, NEW_MAGIC, len);
  *(int*)((char*)storage+len) = SCALAR_MAGIC;
  DEBUGLOG(("operator new(%u) : %p\n", len, storage));
  return storage;
}

void operator delete(void* ptr)
{ if (!ptr)
    return;
  DEBUGLOG(("operator delete(%p)\n", ptr));
  preamble* storage = (preamble*)ptr -1;
  ASSERT(storage->magic == SCALAR_MAGIC);
  ASSERT(*(unsigned*)((char*)ptr+storage->length) == SCALAR_MAGIC);
  storage->magic = FREE_MAGIC;
  *(unsigned*)((char*)ptr+storage->length) = FREE_MAGIC;
  memset(ptr, 0xbb, storage->length);
  free(storage);
}

void* operator new[](size_t len)
{ if (!len)
    return NULL;
  preamble* storage = (preamble*)malloc(len + sizeof(preamble) + sizeof(int));
  storage->length = len;
  storage->magic = ARRAY_MAGIC;
  ++storage;
  *(unsigned*)((char*)storage+len) = ARRAY_MAGIC;
  DEBUGLOG(("operator new[](%u) : %p\n", len, storage));
  return storage;
}

void operator delete[](void* ptr)
{ if (!ptr)
    return;
  DEBUGLOG(("operator delete[](%p)\n", ptr));
  preamble* storage = (preamble*)ptr -1;
  ASSERT(storage->magic == ARRAY_MAGIC);
  ASSERT(*(unsigned*)((char*)ptr+storage->length) == ARRAY_MAGIC);
  storage->magic = FREE_MAGIC;
  *(unsigned*)((char*)ptr+storage->length) = FREE_MAGIC;
  memset(ptr, 0xbb, storage->length);
  free(storage);
}
