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


/* Dynamic string implementation
 */

#ifndef CPP_XSTRING_H
#define CPP_XSTRING_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <cpp/smartptr.h>


// Java like string class
// Copying is cheap.
// The memory footprint is the same as for char*.
// The storage overhead is two ints per unique string.
class xstring
{private:
  // Data Structure for non-mutable strings.
  // It contains an intrusive reference count and an intrusive instance of the string data.
  class StringRef : public Iref_Count
  {private:
    size_t       Len;  // Logically const, but since we initialize it before constructor
                  // it has to be non-const to avoid the error.
                  // Must be last member
   private: // Avoid array creation!
    #if defined(__IBMCPP__) && defined(DEBUG_ALLOC)
    static void* operator new[](size_t, const char*, size_t);
    static void  operator delete[](void*, const char*, size_t);
    #else
    static void* operator new[](size_t);
    static void  operator delete[](void*);
    #endif
   private: // Non-copyable
                 StringRef(const StringRef&);
           void  operator=(const StringRef&);
   public:
                 StringRef()                { (*this)[Len] = 0; } // uninitialized instance
                 StringRef(const char* str) { memcpy(this+1, str, Len); (*this)[Len] = 0; }
    #if defined(__IBMCPP__) && defined(DEBUG_ALLOC)
    static void* operator new(size_t s, const char*, size_t, size_t l)
    #else
    static void* operator new(size_t s, size_t l)
    #endif
                                            { assert(_heapchk() == _HEAPOK);
                                              char* cp = (char*)malloc(s+l+1);
                                              DEBUGLOG2(("xstring::StringRef::operator new(%u, %u) - %p\n", s, l, cp));
                                              ((size_t*)(cp+s))[-1] = l; // Dirty implicit assignment to Len before constructor entry...
                                              return cp;
                                            }
    #if defined(__IBMCPP__) && defined(DEBUG_ALLOC)
    static void  operator delete(void* p, const char*, size_t)
    #else
    static void  operator delete(void* p)
    #endif
                                            { DEBUGLOG2(("xstring::StringRef::operator delete(%p)\n", p)); free(p); }
           size_t Length() const            { return Len; }
           char* Ptr() const                { return (char*)(this+1); }
           char& operator[](size_t ix)      { return ((char*)(this+1))[ix]; }
  };
 public:
  enum
  { npos = (size_t)-1
  };

 private:
  int_ptr<StringRef> Data;
 public:
  // empty string. This is different from a NULL string!
  static const xstring empty;

 protected:
  xstring(size_t len)                       : Data(new (len) StringRef) {}
 public:
  static int  compareI(const char* s1, const char* s2, size_t len);

  xstring()                                 {}
  xstring(const xstring& r) : Data(r.Data)  {}
  xstring(const xstring& r, size_t start);
  xstring(const xstring& r, size_t start, size_t len);
  xstring(const char* str);
  xstring(const char* str, size_t len);
  // length of the string. The string must not be NULL!
  size_t      length() const                { return Data->Length(); }
  // explicit conversion
  // The returned pointer is valid as long as this string is owned by at least one xstring instance.
  const char* cdata() const                 { return Data ? Data->Ptr() : NULL; }
  // constant c-style array containing the string, always null terminated, maybe NULL.
  // The returned pointer is valid as long as this string is owned by at least one xstring instance.
  operator const char*() const              { return cdata(); }
  // return the i-th character
  const char& operator[](size_t i)          { assert(i < length()); return (*Data)[i]; }
  // test for equality
  bool        equals(const xstring& r) const;
  bool        equals(const char* str) const;
  bool        equals(const char* str, size_t len) const;
  // compare strings (case sensitive)
  // A NULL string is the smallest possible string even smaller than "".
  int         compareTo(const xstring& r) const;
  int         compareTo(const char* str) const;
  int         compareTo(const char* str, size_t len) const;
  // compare strings (case insensitive)
  // This works only for 7 bit ASCII as expected.
  int         compareToI(const xstring& r) const;
  int         compareToI(const char* str) const;
  int         compareToI(const char* str, size_t len) const;

  bool        startsWith(const xstring& r) const { return startsWith(r.Data->Ptr(), r.length()); }
  bool        startsWith(const char* r) const { return startsWith(r, strlen(r)); }
  bool        startsWith(const char* r, size_t len) const;
  bool        startsWith(char c) const      { return length() && (*Data)[0] == c; }
  bool        startsWithI(const xstring& r) const { return startsWithI(r.Data->Ptr(), r.length()); }
  bool        startsWithI(const char* r) const { return startsWithI(r, strlen(r)); }
  bool        startsWithI(const char* r, size_t len) const;
  bool        startsWithI(char c) const     { return length() && tolower((*Data)[0]) == tolower(c); }
  bool        endsWith(const xstring& r) const { return endsWith(r.Data->Ptr(), r.length()); }
  bool        endsWith(const char* r) const { return endsWith(r, strlen(r)); }
  bool        endsWith(const char* r, size_t len) const;
  bool        endsWith(char c) const        { return length() && (*Data)[length()-1] == c; }
  bool        endsWithI(const xstring& r) const { return endsWithI(r.Data->Ptr(), r.length()); }
  bool        endsWithI(const char* r) const { return endsWithI(r, strlen(r)); }
  bool        endsWithI(const char* r, size_t len) const;
  bool        endsWithI(char c) const       { return length() && tolower((*Data)[length()-1]) == tolower(c); }
  // swap two instances
  void        swap(xstring& r)              { Data.swap(r.Data); }
  // assign new value
  void        assign(const xstring& r)      { Data = r.Data; }
  void        assign(const xstring& r, size_t start) { xstring tmp(r, start); swap(tmp); }
  void        assign(const xstring& r, size_t start, size_t len) { xstring tmp(r, start, len); swap(tmp); }
  void        assign(const char* str);
  void        assign(const char* str, size_t len);
  // initialize to new string with defined length and undefined content
  char*       raw_init(size_t len)          { Data = new (len) StringRef; return Data->Ptr(); }
  // initialize to new string with defined length and defined content that might be modified befor the string is used elsewhere
  char*       raw_init(size_t len, const char* src) { assign(src, len); return Data->Ptr(); }
  // assign new value by operator
  xstring&    operator=(const xstring& r)   { assign(r); return *this; }
  xstring&    operator=(const char* str)    { assign(str); return *this; }
  // concatenate strings
  // None of the strings must be NULL.
  friend xstring operator+(const xstring& l, const xstring& r);
  friend xstring operator+(const xstring& l, const char* r);
  friend xstring operator+(const char* l,    const xstring& r);
  // printf, well...
  static xstring sprintf(const char* fmt, ...);
  static xstring vsprintf(const char* fmt, va_list va);
};

inline bool operator==(const xstring& l, const xstring& r)
{ return l.equals(r);
}
inline bool operator==(const xstring& l, const char* r)
{ return l.equals(r);
}
inline bool operator==(const char* l,    const xstring& r)
{ return r.equals(l);
}

inline bool operator!=(const xstring& l, const xstring& r)
{ return !l.equals(r);
}
inline bool operator!=(const xstring& l, const char* r)
{ return !l.equals(r);
}
inline bool operator!=(const char* l,    const xstring& r)
{ return !r.equals(l);
}

inline bool operator<(const xstring& l, const xstring& r)
{ return l.compareTo(r) < 0;
}
inline bool operator<(const xstring& l, const char* r)
{ return l.compareTo(r) < 0;
}
inline bool operator<(const char* l,    const xstring& r)
{ return r.equals(l) > 0;
}

inline bool operator<=(const xstring& l, const xstring& r)
{ return l.compareTo(r) <= 0;
}
inline bool operator<=(const xstring& l, const char* r)
{ return l.compareTo(r) <= 0;
}
inline bool operator<=(const char* l,    const xstring& r)
{ return r.equals(l) >= 0;
}

inline bool operator>(const xstring& l, const xstring& r)
{ return l.compareTo(r) > 0;
}
inline bool operator>(const xstring& l, const char* r)
{ return l.compareTo(r) > 0;
}
inline bool operator>(const char* l,    const xstring& r)
{ return r.equals(l) < 0;
}

inline bool operator>=(const xstring& l, const xstring& r)
{ return l.compareTo(r) >= 0;
}
inline bool operator>=(const xstring& l, const char* r)
{ return l.compareTo(r) >= 0;
}
inline bool operator>=(const char* l,    const xstring& r)
{ return r.equals(l) <= 0;
}

#endif
