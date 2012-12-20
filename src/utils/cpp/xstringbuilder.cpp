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

#include <snprintf.h>

#include <debuglog.h>


char* const xstringbuilder::Empty = "";

char* xstringbuilder::auto_alloc_raw(size_t& cap)
{ ASSERT(cap > Cap);
  // Calc new capacity
  size_t newcap = Cap < 60 ? 60 : (Cap-1)*7/4 +1;
  if (newcap > cap)
    cap = newcap;
  // allocate new storage preserving content
  return new char[cap+1];
}

void xstringbuilder::auto_alloc(size_t cap)
{ char* newdata = auto_alloc_raw(cap);
  if (Cap)
  { memcpy(newdata, Data, Len);
    delete[] Data;
  }
  // done
  Data = newdata;
  Cap = cap;
}

char* xstringbuilder::replace_core(size_t at, size_t len1, size_t len2)
{ const size_t end = at + len1;
  ASSERT(end <= Len);
  char* start = Data + at;
  if (len1 != len2)
  { const size_t newlen = Len + len2 - len1;
    if (newlen > Cap)
    { // reallocation required
      size_t newcap = newlen;
      char* newdata = auto_alloc_raw(newcap);
      if (Cap)
      { memcpy(newdata, Data, at);
        memcpy(newdata+at+len2, start, Len-end+1);
        delete[] Data;
      }
      Cap = newcap;
      Data = newdata;
      start = newdata + at;
    } else
      memmove(start+len2, start+len1, Len-end+1);
    Len = newlen;
  }
  return start;
}

xstringbuilder::xstringbuilder(size_t cap)
: Cap(cap),
  Len(0)
{ if (cap)
  { Data = new char[cap+1];
    Data[0] = 0;
  } else
    Data = Empty;
}

char* xstringbuilder::detach_array()
{ char* ret;
  Len = 0;
  if (Cap)
  { ret = Data;
  } else
  { ret = new char[1];
    *ret = 0;
  }
  Cap = 0;
  Data = Empty;
  return ret;
}

void xstringbuilder::resize(size_t len)
{ if (len > Len)
  { auto_alloc(len);
    memset(Data+Len, 0, len-Len+1);
  } else
    Data[len] = 0;
  Len = len;
}
void xstringbuilder::resize(size_t len, char filler)
{ if (len > Len)
  { auto_alloc(len);
    memset(Data+Len, filler, len-Len);
  }
  Data[len] = 0;
  Len = len;
}

void xstringbuilder::reserve(size_t cap)
{ if (cap < Len)
    cap = Len;
  if (cap == Cap)
    return; // no-op
  // adjust capacity
  if (cap == 0)
  { delete[] Data;
    Data = Empty;
  } else
  { char* newdata = new char[cap+1];
    if (Cap)
    { memcpy(newdata, Data, Len+1);
      delete[] Data;
    } else
      newdata[0] = 0;
    Data = newdata;
  }
  Cap = cap;
}

void xstringbuilder::append(const char* str, size_t len)
{ if (len == 0)
    return;
  const size_t newlen = Len + len;
  if (newlen > Cap)
  { // reallocation required
    size_t newcap = newlen;
    char* newdata = auto_alloc_raw(newcap);
    memcpy(newdata+Len, str, len); // append first to support in place operation.
    if (Cap)
    { memcpy(newdata, Data, Len);
      delete[] Data;
    }
    Data = newdata;
    Cap = newcap;
  } else
    memcpy(Data+Len, str, len);
  Data[newlen] = 0;
  Len = newlen;
}
void xstringbuilder::append(size_t count, char c)
{ if (count == 0)
    return;
  const size_t newlen = Len + count;
  if (newlen > Cap)
    auto_alloc(newlen);
  memset(Data+Len, c, count);
  Data[newlen] = 0;
  Len = newlen;
}
void xstringbuilder::appendf(const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  vappendf(fmt, va);
  va_end(va);
}
void xstringbuilder::vappendf(const char* fmt, va_list va)
{ const size_t len = vsnprintf(Data + Len, Cap - Len + 1, fmt, va);
  Len += len;
  if (Len <= Cap)
    return;
  // reallocation required
  auto_alloc(Len);
  vsnprintf(Data + Len - len, len + 1, fmt, va);
}

void xstringbuilder::insert(size_t at, char c)
{ ASSERT(at <= Len);
  if (Len == Cap)
  { // reallocation required
    size_t newcap = ++Len;
    char* newdata = auto_alloc_raw(newcap);
    if (Cap)
    { memcpy(newdata, Data, at);
      memcpy(newdata+at+1, Data+at, Len-at);
      delete[] Data;
    }
    Data = newdata;
    Cap = newcap;
  } else
  { // without reallocation
    memmove(Data+at+1, Data+at, ++Len-at);
  }
  Data[at] = c;
}
void xstringbuilder::insertf(size_t at, const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  vinsertf(at, fmt, va);
  va_end(va);
}
void xstringbuilder::vinsertf(size_t at, const char* fmt, va_list va)
{ const size_t len = vsnprintf(NULL, 0, fmt, va);
  vsnprintf(replace_core(at, 0, len), len+1, fmt, va);
}

void xstringbuilder::replacef(size_t from, size_t len, const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  vreplacef(from, len, fmt, va);
  va_end(va);
}
void xstringbuilder::vreplacef(size_t from, size_t len1, const char* fmt, va_list va)
{ const size_t len2 = vsnprintf(NULL, 0, fmt, va);
  vsnprintf(replace_core(from, len1, len2), len2+1, fmt, va);
}

size_t xstringbuilder::find(char c, size_t pos)
{ while (pos < Len && Data[pos] != c)
    ++pos;
  return pos;
}
size_t xstringbuilder::find(const char* str, size_t len, size_t pos)
{ if (len == 0)
    return pos;
  if (pos + len > Len)
    return Len;
  while (true)
  { pos = find(str[0], pos);
    if (pos + len >= Len)
      return Len;
    ++pos;
    if (memcmp(str+1, Data+pos, len-1) == 0)
      return pos-1;
  }
}

size_t xstringbuilder::rfind(char c, size_t pos)
{ while (pos && Data[pos-1] != c)
    --pos;
  return pos;
}
size_t xstringbuilder::rfind(const char* str, size_t len, size_t pos)
{ if (len == 0)
    return pos;
  if (len > Len || pos == 0)
    return 0;
  if (pos > Len-len)
    pos = Len-len+1;
  while (true)
  { pos = rfind(str[0], pos-1);
    if (pos == 0 || memcmp(str+1, Data+pos, len-1) == 0)
      return pos;
    --pos;
  }
}

static void make_find_map(unsigned char(&map)[256], const char* str, size_t len)
{ memset(map, false, 256);
  for (const char* ep = str + len; str != ep; ++str)
    map[*str] = true;
}

size_t xstringbuilder::find_any(const char* str, size_t len, size_t pos)
{ unsigned char map[256];
  make_find_map(map, str, len);
  while (pos < Len && !map[(unsigned)Data[pos]])
    ++pos;
  return pos;
}

size_t xstringbuilder::find_not_any(const char* str, size_t len, size_t pos)
{ unsigned char map[256];
  make_find_map(map, str, len);
  while (pos < Len && map[(unsigned)Data[pos]])
    ++pos;
  return pos;
}
