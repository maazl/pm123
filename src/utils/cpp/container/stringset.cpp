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


int stringset::Compare(const void* key, const void* elem)
{ return xstring::compare(*(const xstring*)&key, *(const xstring*)&elem);
}

void stringset::inc_refs()
{ void*const* dp(vector_base::begin());
  void*const* dpe(dp + size());
  xstring ptr;
  while (dp != dpe)
  { ptr = *(xstring*)dp++;
    ptr.toCstr();
  }
}

void stringset::dec_refs()
{ void*const* dp(vector_base::begin());
  void*const* dpe(dp + size());
  xstring ptr;
  while (dp != dpe)
    ptr.fromCstr((const char*)*dp++);
}

stringset& stringset::operator=(const stringset& r)
{ dec_refs();
  vector_base::operator=(r);
  inc_refs();
  return *this;
}

xstring stringset::find(const xstring& item) const
{ size_t pos;
  if (locate(item, pos))
    return (xstring&)vector_base::at(pos);
  return xstring();
}

xstring& stringset::get(const xstring& item)
{ size_t pos;
  if (locate(item, pos))
    return (xstring&)vector_base::at(pos);
  return (xstring&)insert(pos);
}

const xstring& stringset::ensure(const xstring& item)
{ size_t pos;
  if (locate(item, pos))
    return (const xstring&)vector_base::at(pos);
  return (xstring&)insert(pos) = item;
}

xstring stringset::erase(const xstring& item)
{ size_t pos;
  if (locate(item, pos))
    return xstring().fromCstr((const char*)vector_base::erase(pos));
  return xstring();
}

