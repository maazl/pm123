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
#include <ctype.h>
#include <snprintf.h>

#include <debuglog.h>


const xstring xstring::empty("");


int xstring::compareI(const char* l, const char* r, size_t len)
{ //DEBUGLOG(("xstring::compareI(%s, %s, %u)\n", l, r, len));
  while (len--)
  { register int i = (int)(unsigned char)tolower(*l++) - (unsigned char)tolower(*r++);
    //DEBUGLOG(("xstring::compareI - %i\n", i));
    if (i != 0)
      return i;
  }
  return 0;
}

xstring::xstring(const xstring& r, size_t start)
: Data(new (r.length()-start) StringRef(*r.Data + start))
{ assert(start <= r.length());
}
xstring::xstring(const xstring& r, size_t start, size_t len)
: Data(new (len) StringRef(*r.Data + start))
{ assert(start+len <= r.length());
}
xstring::xstring(const char* str)
: Data(str == NULL ? NULL : new (strlen(str)) StringRef(str))
{}
xstring::xstring(const char* str, size_t len)
: Data(str == NULL ? NULL : new (len) StringRef(str))
{}

bool xstring::equals(const xstring& r) const
{ if (Data == r.Data)
    return true;
  if (!Data || !r.Data)
    return false;
  size_t len = length();
  return len == r.length() && memcmp(*Data, *r.Data, len) == 0;
}
bool xstring::equals(const char* str) const
{ if (!Data ^ !str)
    return false;
  if (!Data || *Data == str)
    return true;
  size_t len = strlen(str);
  return length() == len && memcmp(*Data, str, len) == 0;
}
bool xstring::equals(const char* str, size_t len) const
{ if (!Data ^ !str)
    return false;
  if (!Data || *Data == str)
    return true;
  return length() == len && memcmp(*Data, str, len) == 0;
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
    return ((memcmp(*Data, *r.Data, ll) <= 0) << 1) -1;
  if (ll == rl)
    return memcmp(*Data, *r.Data, ll);
   else 
    return ((memcmp(*Data, *r.Data, rl) < 0) << 1) -1;
}
int xstring::compareTo(const char* str) const
{ if (!Data)
    return -(!str);
  if (*Data == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  size_t rl = strlen(str);
  if (ll < rl)
    return ((memcmp(*Data, str, ll) <= 0) << 1) -1;
  if (ll == rl)
    return memcmp(*Data, str, ll);
   else 
    return ((memcmp(*Data, str, rl) < 0) << 1) -1;
}
int xstring::compareTo(const char* str, size_t rl) const
{ if (!Data)
    return -(!str);
  if (*Data == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  if (ll < rl)
    return ((memcmp(*Data, str, ll) <= 0) << 1) -1;
  if (ll == rl)
    return memcmp(*Data, str, ll);
   else 
    return ((memcmp(*Data, str, rl) < 0) << 1) -1;
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
    return ((compareI(*Data, *r.Data, ll) <= 0) << 1) -1;
  if (ll == rl)
    return compareI(*Data, *r.Data, ll);
   else 
    return ((compareI(*Data, *r.Data, rl) < 0) << 1) -1;
}
int xstring::compareToI(const char* str) const
{ if (!Data)
    return -(!str);
  if (*Data == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  size_t rl = strlen(str);
  if (ll < rl)
    return ((compareI(*Data, str, ll) <= 0) << 1) -1;
  if (ll == rl)
    return compareI(*Data, str, ll);
   else 
    return ((compareI(*Data, str, rl) < 0) << 1) -1;
}
int xstring::compareToI(const char* str, size_t rl) const
{ if (!Data)
    return -(!str);
  if (*Data == str)
    return 0;
  if (!str)
    return 1;
  size_t ll = length();
  if (ll < rl)
    return ((compareI((char*)*Data, (char*)str, ll) <= 0) << 1) -1;
  if (ll == rl)
    return compareI((char*)*Data, (char*)str, ll);
   else 
    return ((compareI((char*)*Data, (char*)str, rl) < 0) << 1) -1;
}

bool xstring::startsWith(const char* r, size_t len) const
{ return len <= length() && memcmp(*Data, r, len) == 0;
}

bool xstring::startsWithI(const char* r, size_t len) const
{ return len <= length() && compareI(*Data, r, len) == 0;
}

void xstring::assign(const char* str)
{ Data = str == NULL ? NULL : new (strlen(str)) StringRef(str);
}
void xstring::assign(const char* str, size_t len)
{ Data = str == NULL ? NULL : new (len) StringRef(str);
}

xstring operator+(const xstring& l, const xstring& r)
{ size_t ll = l.length();
  size_t rl = r.length();
  xstring x(ll + rl);
  memcpy(*x.Data, *l.Data, ll);
  memcpy(*x.Data + ll, *r.Data, rl);
  return x;
}
xstring operator+(const char* l, const xstring& r)
{ size_t ll = strlen(l);
  size_t rl = r.length();
  xstring x(ll + rl);
  memcpy(*x.Data, l, ll);
  memcpy(*x.Data + ll, *r.Data, rl);
  return x;
}
xstring operator+(const xstring& l, const char* r)
{ size_t ll = l.length();
  size_t rl = strlen(r);
  xstring x(ll + rl);
  memcpy(*x.Data, *l.Data, ll);
  memcpy(*x.Data + ll, r, rl);
  return x;
}

xstring xstring::sprintf(const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  xstring ret = vsprintf(fmt, va);
  va_end(va);
  return ret;
}

xstring xstring::vsprintf(const char* fmt, va_list va)
{ DEBUGLOG(("xstring::vsprintf(%s, %p)\n", fmt, va));
  xstring ret(vsnprintf(NULL, 0, fmt, va));
  vsnprintf(*ret.Data, ret.length()+1, fmt, va);
  return ret;
}

