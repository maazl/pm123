/*
 * Copyright 2011 M.Mueller
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


#include "xstring.h"

#include <debuglog.h>


#if defined(__IBMCPP__) && defined(__DEBUG_ALLOC__)
void* xstring::StringData::operator new(size_t s, const char*, size_t, size_t l)
#else
void* xstring::StringData::operator new(size_t s, size_t l)
#endif
{ StringData* that = (StringData*)new char[s+l+1];
  DEBUGLOG2(("xstring::StringData::operator new(%u, %u) - %p\n", s, l, cp));
  // Dirty early construction
  that->Count = 0;
  that->Len = l;
  return that+1;
}
#if defined(__IBMCPP__) && defined(__DEBUG_ALLOC__)
void xstring::StringData::operator delete(void* p, const char*, size_t)
#else
void xstring::StringData::operator delete(void* p)
#endif
{ delete[] (char*)((StringData*)p-1);
}
