/*
 * Copyright 2007-2013 M.Mueller
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
#include "container/btree.h"
#include <string.h>
#include <snprintf.h>

#include <debuglog.h>


// Really dirty hack to avoid static initialization problem
static struct
{ unsigned ref;
  size_t   len;
  char     term;
} empty_ref = { 1, 0, 0 };
char* const empty_str = &empty_ref.term;

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

xstring::StringData* xstring::copycore(const char* src, size_t len)
{ if (!len)
    return empty.Data.get();
  StringData* ret = new(len) StringData;
  memcpy(ret->ptr(), src, len);
  return ret;
}

xstring::StringData* xstring::sprintfcore(const char* fmt, va_list va)
{ DEBUGLOG2(("xstring::sprintfcore(%s, %p)\n", fmt, va));
  size_t len = vsnprintf(NULL, 0, fmt, va);
  if (!len)
    return empty.Data;
  StringData* data = new(len) StringData;
  vsnprintf(data->ptr(), len+1, fmt, va);
  return data;
}

xstring::xstring(const xstring& r, size_t start)
: Data(copycore(r.Data->ptr() + start, r.length()-start))
{ ASSERT(start <= r.length());
}
xstring::xstring(const xstring& r, size_t start, size_t len)
: Data(copycore(r.Data->ptr() + start, len))
{ ASSERT(start+len <= r.length());
}
xstring::xstring(const char* str)
{ if (str)
    Data = copycore(str, strlen(str));
}


bool xstring::equals(const xstring& r) const
{ if (Data == r.Data)
    return true;
  if (!Data || !r.Data)
    return false;
  size_t len = length();
  return len == r.length() && memcmp(Data->ptr(), r.Data->ptr(), len) == 0;
}
bool xstring::equals(const char* str) const
{ // Note the hack with .get()-> is required because this is a fastpath that can deal with Data == NULL.
  if (Data.get()->ptr() == str)
    return true;
  if (!Data || !str)
    return false;
  size_t len = strlen(str);
  return length() == len && memcmp(Data->ptr(), str, len) == 0;
}
bool xstring::equals(const char* str, size_t len) const
{ // Note the hack with .get()-> is required because this is a fastpath that can deal with Data == NULL.
  if (Data.get()->ptr() == str)
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
    if (str == NULL)
      Data.reset();
    else
      Data = copycore(str, strlen(str));
}
void xstring::assign(const char* str, size_t len)
{ if (str == NULL)
    Data.reset();
  else
    Data = copycore(str, len);
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

xstring& xstring::sprintf(const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  Data = sprintfcore(fmt, va);
  va_end(va);
  return *this;
}
void xstring::sprintf(const char* fmt, ...) volatile
{ va_list va;
  va_start(va, fmt);
  Data = sprintfcore(fmt, va);
  va_end(va);
}



btree_string xstring::deduplicator::Repository;
Mutex xstring::deduplicator::Mtx;

void xstring::deduplicator::deduplicate(xstring& target)
{ if (!target)
    return;
  xstring& entry = Repository.get(target);
  DEBUGLOG(("xstring::deduplicator::deduplicate(%s) : %s\n", target.cdata(), entry.cdata()));
  if (!entry)
    entry = target;
  else
    target = entry;
}
void xstring::deduplicator::deduplicate(volatile xstring& target)
{ xstring value(target);
  if (!value)
    return;
  xstring& entry = Repository.get(value);
  DEBUGLOG(("xstring::deduplicator::deduplicate(%s) : %s\n", value.cdata(), entry.cdata()));
  if (!entry)
    entry = value;
  else if (!value.instEquals(entry))
    target.cmpassign(value, entry);
}

void xstring::deduplicator::cleanup()
{ DEBUGLOG(("xstring::deduplicator::cleanup()\n"));
  #ifdef DEBUG_LOG
  unsigned keep = 0, del = 0;
  btree_string::iterator pos(Repository.begin());
  while (!pos.isend())
  { if (pos->Data->isUnique())
    { Repository.erase(pos);
      #ifdef DEBUG_LOG
      ++del;
      #endif
    } else
    { ++pos;
      #ifdef DEBUG_LOG
      ++keep;
      #endif
    }
  }
  DEBUGLOG(("xstring::deduplicator::cleanup: destroyed %u, kept %u\n", del, keep));
}
