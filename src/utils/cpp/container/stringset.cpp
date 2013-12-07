/*
 * Copyright 2011-2013 M.Mueller
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


#include "container/stringset.h"

void stringset_base::inc_refs()
{ void*const* dp(vector_base::begin());
  void*const* dpe(dp + size());
  xstring ptr;
  while (dp != dpe)
  { ptr = *(xstring*)dp++;
    ptr.toCstr();
  }
}

void stringset_base::dec_refs()
{ void*const* dp(vector_base::begin());
  void*const* dpe(dp + size());
  xstring ptr;
  while (dp != dpe)
    ptr.fromCstr((const char*)*dp++);
}

stringset_base::stringset_base(const stringset_base& r, size_t spare)
: vector_base(r, spare)
, Comparer(r.Comparer)
{ inc_refs(); }

stringset_base& stringset_base::operator=(const stringset_base& r)
{ dec_refs();
  vector_base::operator=(r);
  inc_refs();
  return *this;
}

xstring stringset_base::find(const xstring& item) const
{ size_t pos;
  if (locate(item, pos))
    return (xstring&)vector_base::at(pos);
  return xstring();
}

xstring& stringset_base::get(const xstring& item)
{ size_t pos;
  if (locate(item, pos))
    return (xstring&)vector_base::at(pos);
  return (xstring&)insert(pos);
}

const xstring& stringset_base::ensure(const xstring& item)
{ size_t pos;
  if (locate(item, pos))
    return (const xstring&)vector_base::at(pos);
  return (xstring&)insert(pos) = item;
}

xstring stringset_base::erase(const xstring& item)
{ size_t pos;
  if (locate(item, pos))
    return xstring().fromCstr((const char*)vector_base::erase(pos));
  return xstring();
}


int stringset::Compare(const void* l, const void* r)
{ return xstring::compare(*(const xstring*)&l, *(const xstring*)&r);
}

stringset::stringset(size_t capacity)
: stringset_base(&stringset::Compare, capacity)
{}

int stringsetI::Compare(const void* l, const void* r)
{ return xstring::compareI(*(const xstring*)&l, *(const xstring*)&r);
}

stringsetI::stringsetI(size_t capacity)
: stringset_base(&stringsetI::Compare, capacity)
{}
