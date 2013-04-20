/*
 * Copyright 2012-2013 M.Mueller
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

#include "../container/btree.h"
#include <stdlib.h>
#include <debuglog.h>

static int data[65536]; // Must be a power of 2!

static int inst_comparer(const void* key, const void* elem)
{ ASSERT(elem >= data && elem < data + sizeof data / sizeof *data);
  return (char*)key - (char*)elem;
}

// untyped instance repository
static void test_untyped()
{ unsigned i;
  btree_base tree(inst_comparer);

  // create items
  unsigned scramble = rand() & (sizeof data / sizeof *data - 1);
  for (i = 0; i < sizeof data / sizeof *data; ++i)
  { DEBUGLOG(("test_untyped at %u\n", i));
    void* key = data + (i ^ scramble);
    void*& value = tree.get(key);
    ASSERT(!value);
    value = key;
    //tree.check();
  }
  tree.check();

  // check sequence
  btree_base::iterator iter(tree.begin());
  ASSERT(iter.isbegin());
  for (i = 0; i < sizeof data / sizeof *data; ++i)
  { ASSERT(!iter.isend());
    ASSERT(*iter == data + i);
    ++iter;
    ASSERT(!iter.isbegin());
  }
  ASSERT(iter.isend());

  // reverse iteration
  iter = tree.end();
  for (i = sizeof data / sizeof *data; i-- != 0;)
  { --iter;
    ASSERT(*iter == data + i);
  }
  ASSERT(iter.isbegin());

  // updates
  for (i = 0; i < 10000; ++i)
  { unsigned n = rand() & (sizeof data / sizeof *data - 1);
    DEBUGLOG(("test_untyped update #%u at %u\n", i, n));
    void* key = data + n;
    void*& value = tree.get(key);
    RASSERT(value == key);
  }
  tree.check();

  // deletes => delete every 2nd element, but not in order.
  scramble = rand() & (sizeof data / sizeof *data - 1) & ~1;
  for (i = 0; i < sizeof data / sizeof *data; i += 2)
  { void* key = data + (i ^ scramble);
    RASSERT(tree.erase(key) == key);
  }
  tree.check();

  // check result
  iter = tree.begin();
  for (i = 1; i < sizeof data / sizeof *data; i += 2)
  { ASSERT(*iter == data + i);
    ++iter;
  }

  // delete remaining elements
  scramble = rand() & (sizeof data / sizeof *data - 1);
  for (i = 0; i < sizeof data / sizeof *data; ++i)
  { void* key = data + (i ^ scramble);
    RASSERT(tree.erase(key) == ((i ^ scramble) & 1 ? key : NULL));
    //tree.check();
  }
  tree.check();

  // check result
  ASSERT(tree.begin() == tree.end());
}

int main()
{
  test_untyped();

  return 0;
}
