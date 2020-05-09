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


#include "list.h"

#include <debuglog.h>


void list_base::push_front(entry_base* elem)
{ ASSERT(elem->Prev == NULL);
  ASSERT(elem->Next == NULL);
  elem->Next = Head;
  (Head ? Head->Prev : Tail) = elem;
  Head = elem;
}

void list_base::push_back(entry_base* elem)
{ ASSERT(elem->Prev == NULL);
  ASSERT(elem->Next == NULL);
  elem->Prev = Tail;
  (Tail ? Tail->Next : Head) = elem;
  Tail = elem;
}

list_base::entry_base* list_base::pop_front()
{ if (!Head)
    return NULL;
  entry_base* ret = Head;
  Head = Head->Next;
  (Head ? Head->Prev : Tail) = NULL;
  ret->Next = NULL;
  return ret;
}

list_base::entry_base* list_base::pop_back()
{ if (!Tail)
    return NULL;
  entry_base* ret = Tail;
  Tail = Tail->Prev;
  (Tail ? Tail->Next : Head) = NULL;
  ret->Prev = NULL;
  return ret;
}

void list_base::insert(entry_base* elem, entry_base* before)
{ ASSERT(elem->Prev == NULL);
  ASSERT(elem->Next == NULL);
  entry_base*& rptr = before ? before->Prev : Tail; // right owner (next item or Tail)
  entry_base*& lptr = rptr   ? rptr->Next   : Head; // left owner (previous item or Head)
  elem->Next = before;
  elem->Prev = rptr;
  rptr = elem;
  lptr = elem;
}

void list_base::remove(entry_base* elem)
{ (elem->Prev ? elem->Prev->Next : Head) = elem->Next;
  (elem->Next ? elem->Next->Prev : Tail) = elem->Prev;
}

bool list_base::move(entry_base* elem, entry_base* before)
{ if (elem == before || elem->Next == before)
    return false; // No-op
  (elem->Prev ? elem->Prev->Next : Head) = elem->Next;
  (elem->Next ? elem->Next->Prev : Tail) = elem->Prev;
  entry_base*& rptr = before ? before->Prev : Tail; // right owner (next item or Tail)
  entry_base*& lptr = rptr   ? rptr->Next   : Head; // left owner (previous item or Head)
  elem->Next = before;
  elem->Prev = rptr;
  rptr = elem;
  lptr = elem;
  return true;
}

