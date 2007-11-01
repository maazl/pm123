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


#include "mutex.h"
#include "smartptr.h"

#include <debuglog.h>


int_ptr_base::int_ptr_base(const Iref_Count* ptr)
: Ptr((Iref_Count*)ptr) // constness is handled in the derived class
{ DEBUGLOG(("int_ptr_base(%p)::int_ptr_base(%p)\n", this, ptr));
  if (Ptr)
    InterlockedInc(Ptr->Count);
}
int_ptr_base::int_ptr_base(const int_ptr_base& r)
: Ptr(r.Ptr)
{ DEBUGLOG(("int_ptr_base(%p)::int_ptr_base(&%p{%p})\n", this, &r, r.Ptr));
  if (Ptr)
    InterlockedInc(Ptr->Count);
}

Iref_Count* int_ptr_base::reassign(const Iref_Count* ptr)
{ DEBUGLOG(("int_ptr_base(%p)::reassign(%p): %p\n", this, ptr, Ptr));
  // Hack to avoid problems with (rarely used) p = p statements: increment new counter first.
  if (ptr)
    InterlockedInc(((Iref_Count*)ptr)->Count);
  Iref_Count* ret = NULL;
  if (Ptr && InterlockedDec(Ptr->Count) == 0)
    ret = Ptr;
  Ptr = (Iref_Count*)ptr; // constness is handled in the derived class
  return ret;
}

Iref_Count* int_ptr_base::reassign_weak(const Iref_Count* ptr)
{ DEBUGLOG(("int_ptr_base(%p)::reassign_weak(%p): %p\n", this, ptr, Ptr));
  // Hack to avoid problems with (rarely used) p = p statements: increment new counter first.
  if (ptr)
  { CritSect cs;
    if (ptr->Count == 0)
      // The referencounter is 0, so noone else is owning the object.
      // This is the case when the object is just created or about to be deleted.
      // We simply assign NULL in this case.
      ptr = NULL;
     else
      // This is done in the above critical section.
      // This avoids that the counter reaces zero between the above check an the increment here.
      ++((Iref_Count*)ptr)->Count;
  }
  Iref_Count* ret = NULL;
  if (Ptr && InterlockedDec(Ptr->Count) == 0)
    ret = Ptr;
  Ptr = (Iref_Count*)ptr; // constness is handled in the derived class
  return ret;
}

Iref_Count* int_ptr_base::unassign()
{ DEBUGLOG(("int_ptr_base(%p)::unassign(): %p, %i\n", this, Ptr, Ptr ? Ptr->Count : 0));
  return Ptr && InterlockedDec(Ptr->Count) == 0 ? Ptr : NULL;
}

