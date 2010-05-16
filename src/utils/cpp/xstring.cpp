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


#include "xstring.h"
#include <string.h>
#include <snprintf.h>

#include <debuglog.h>

// Really dirty hack to avoid static initialization problem
static struct
{ unsigned ref;
  size_t   len;
  char     term;
} empty_ref = { 1, 0, 0 };
char* empty_str = &empty_ref.term;

const xstring& xstring::empty = *(xstring*)&empty_str;


int xstring::compareI(const char* l, const char* r, size_t len)
{ DEBUGLOG2(("xstring::compareI(%s, %s, %u)\n", l, r, len));
  while (len--)
  { register int i = (int)(unsigned char)tolower(*l++) - (unsigned char)tolower(*r++);
    DEBUGLOG2(("xstring::compareI - %i\n", i));
    if (i != 0)
      return i;
  }
  return 0;
}

xstring::xstring(const xstring& r, size_t start)
: Data(new (r.length()-start) StringData(r.Data->ptr() + start))
{ ASSERT(start <= r.length());
}
xstring::xstring(const xstring& r, size_t start, size_t len)
: Data(new (len) StringData(r.Data->ptr() + start))
{ ASSERT(start+len <= r.length());
}
xstring::xstring(const char* str)
: Data(str == NULL ? NULL : *str ? new (strlen(str)) StringData(str) : empty.Data.get())
{}
xstring::xstring(const char* str, size_t len)
: Data(str == NULL ? NULL : len ? new (len) StringData(str) : empty.Data.get())
{}

bool xstring::equals(const xstring& r) const
{ if (Data == r.Data)
    return true;
  if (!Data || !r.Data)
    return false;
  size_t len = length();
  return len == r.length() && memcmp(Data->ptr(), r.Data->ptr(), len) == 0;
}
bool xstring::equals(const char* str) const
{ if (Data->ptr() == str)
    return true;
  if (!Data || !str)
    return false;
  size_t len = strlen(str);
  return length() == len && memcmp(Data->ptr(), str, len) == 0;
}
bool xstring::equals(const char* str, size_t len) const
{ if (Data->ptr() == str)
    return true;
  if (!Data || !str)
    return false;
  return length() == len && memcmp(Data->ptr(), str, len) == 0;
}

int xstring::compareTo(const xstring& r) const
{ if (Data == r.Data)
    return 0;
  if (!Data)
    return -1;
  if (!r.Data)
    return 1;
  size_t ll = length();
  size_t rl = r.length();
  if (ll < rl)
    return -(memcmp(r.Data->ptr(), Data->ptr(), ll) | 1);
  else
    return memcmp(Data->ptr(), r.Data->ptr(), rl) | (ll != rl);
}
int xstring::compareTo(const char* str) const
{ if (!Data)
    return -(!str);
  if (Data->ptr() == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  size_t rl = strlen(str);
  if (ll < rl)
    return -(memcmp(str, Data->ptr(), ll) | 1);
   else
    return memcmp(Data->ptr(), str, rl) | (ll != rl);
}
int xstring::compareTo(const char* str, size_t rl) const
{ if (!Data)
    return -(!str);
  if (Data->ptr() == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  if (ll < rl)
    return -(memcmp(str, Data->ptr(), ll) | 1);
   else
    return memcmp(Data->ptr(), str, rl) | (ll != rl);
}

int xstring::compareToI(const xstring& r) const
{ if (Data == r.Data)
    return 0;
  if (!Data)
    return -1;
  if (!r.Data)
    return 1;
  size_t ll = length();
  size_t rl = r.length();
  if (ll < rl)
    return -(compareI(r.Data->ptr(), Data->ptr(), ll) | 1);
   else
    return compareI(Data->ptr(), r.Data->ptr(), rl) | (ll != rl);
}
int xstring::compareToI(const char* str) const
{ if (!Data)
    return -(!str);
  if (Data->ptr() == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  size_t rl = strlen(str);
  if (ll < rl)
    return -(compareI(str, Data->ptr(), ll) | 1);
   else
    return compareI(Data->ptr(), str, rl) | (ll != rl);
}
int xstring::compareToI(const char* str, size_t rl) const
{ if (!Data)
    return -(!str);
  if (Data->ptr() == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  if (ll < rl)
    return -(compareI(Data->ptr(), str, ll) | 1);
   else
    return compareI(Data->ptr(), str, rl) | (ll != rl);
}

bool xstring::startsWith(const char* r, size_t len) const
{ return len <= length() && memcmp(Data->ptr(), r, len) == 0;
}

bool xstring::startsWithI(const char* r, size_t len) const
{ return len <= length() && compareI(Data->ptr(), r, len) == 0;
}

bool xstring::endsWith(const char* r, size_t len) const
{ return len <= length() && memcmp(Data->ptr() + length() - len, r, len) == 0;
}

bool xstring::endsWithI(const char* r, size_t len) const
{ return len <= length() && compareI(Data->ptr() + length() - len, r, len) == 0;
}

void xstring::assign(const char* str)
{ if (str != cdata())
    Data = str == NULL ? NULL : *str ? new (strlen(str)) StringData(str) : empty.Data.get();
}

const xstring operator+(const xstring& l, const xstring& r)
{ size_t ll = l.length();
  size_t rl = r.length();
  if (ll == 0)
    return r;
  if (rl == 0)
    return l;
  xstring x(ll + rl);
  memcpy(x.Data->ptr(), l.Data->ptr(), ll);
  memcpy(x.Data->ptr() + ll, r.Data->ptr(), rl);
  return x;
}
const xstring operator+(const char* l, const xstring& r)
{ size_t ll = strlen(l);
  size_t rl = r.length();
  if (ll == 0)
    return r;
  xstring x(ll + rl);
  memcpy(x.Data->ptr(), l, ll);
  memcpy(x.Data->ptr() + ll, r.Data->ptr(), rl);
  return x;
}
const xstring operator+(const xstring& l, const char* r)
{ size_t ll = l.length();
  size_t rl = strlen(r);
  if (rl == 0)
    return l;
  xstring x(ll + rl);
  memcpy(x.Data->ptr(), l.Data->ptr(), ll);
  memcpy(x.Data->ptr() + ll, r, rl);
  return x;
}

const xstring xstring::sprintf(const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  xstring ret = vsprintf(fmt, va);
  va_end(va);
  return ret;
}

const xstring xstring::vsprintf(const char* fmt, va_list va)
{ DEBUGLOG2(("xstring::vsprintf(%s, %p)\n", fmt, va));
  xstring ret(vsnprintf(NULL, 0, fmt, va));
  vsnprintf(ret.Data->ptr(), ret.length()+1, fmt, va);
  return ret;
}

/*void xstring::assignCstr_safe(const char*volatile const& str)
{
  #if !defined DEBUG || DEBUG < 2
  // This will fail with debug level 2
  CritSect cs;
  #endif
  assignCstr(str);
}*/

/*const char* xstring::swapCstr(const char* str)
{ StringData* ref = Data.swapCptr(str ? StringData::fromPtr((char*)str) : NULL);
  return ref ? ref->Ptr() : NULL;
}*/

