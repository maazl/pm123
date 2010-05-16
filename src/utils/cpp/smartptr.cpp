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


#include "mutex.h"
#include "smartptr.h"

#include <debuglog.h>


/*int_ptr_base::int_ptr_base(const Iref_Count* ptr)
: Ptr((Iref_Count*)ptr) // constness is handled in the derived class
{ DEBUGLOG2(("int_ptr_base(%p)::int_ptr_base(%p{%i})\n", this, ptr, ptr ? ptr->Count : 0));
  if (Ptr)
    InterlockedInc(&Ptr->Count);
}
int_ptr_base::int_ptr_base(const int_ptr_base& r)
: Ptr(r.Ptr)
{ DEBUGLOG2(("int_ptr_base(%p)::int_ptr_base(&%p{%p{%i}})\n", this, &r, r.Ptr, r.Ptr ? r.Ptr->Count : 0));
  if (Ptr)
    InterlockedInc(&Ptr->Count);
}
int_ptr_base::int_ptr_base(volatile const int_ptr_base& r)
: Ptr(r.Ptr)
{ DEBUGLOG2(("int_ptr_base(%p)::int_ptr_base(volatile&%p{%p{%i}})\n", this, &r, r.Ptr, r.Ptr ? r.Ptr->Count : 0));
  if (Ptr)
  { CritSect cs;
    Ptr = r.Ptr;
    if (Ptr)
      ++Ptr->Count;
  }
}

Iref_Count* int_ptr_base::reassign(const Iref_Count* ptr)
{ DEBUGLOG2(("int_ptr_base(%p)::reassign(%p{%i}): %p{%i}\n", this, ptr, ptr ? ptr->Count : 0, Ptr, Ptr ? Ptr->Count : 0));
  // Hack to avoid problems with (rarely used) p = p statements: increment new counter first.
  if (ptr)
    InterlockedInc(&((Iref_Count*)ptr)->Count); // constness is handled in the derived class
  Iref_Count* ret = (Iref_Count*)InterlockedXch(&(unsigned&)Ptr, (unsigned)ptr);
  if (ret && InterlockedDec(&ret->Count) != 0)
    ret = NULL;
  return ret;
}

Iref_Count* int_ptr_base::reassign_safe(Iref_Count*volatile const& ptr)
{ DEBUGLOG2(("int_ptr_base(%p)::reassign_safe(&%p{%i}): %p{%i}\n", this, ptr, ptr ? ptr->Count : 0, Ptr, Ptr ? Ptr->Count : 0));
  const Iref_Count* lptr = ptr;
  if (lptr)
  { CritSect cs;
    lptr = ptr;
    if (lptr)
      ++((Iref_Count*)lptr)->Count; // Constness is handled outside
  }
  Iref_Count* ret = (Iref_Count*)InterlockedXch(&(unsigned&)Ptr, (unsigned)lptr);
  if (ret && InterlockedDec(&ret->Count) != 0)
    ret = NULL;
  return ret;
}

void int_ptr_base::swap(int_ptr_base& r)
{ DEBUGLOG2(("int_ptr_base(%p)::swap(&{%p}): %p\n", this, r.Ptr, Ptr));
  CritSect cs;
  Iref_Count* tmp = Ptr;
  Ptr = r.Ptr;
  r.Ptr = tmp;
}
*/
