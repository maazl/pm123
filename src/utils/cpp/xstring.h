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
  // It contains an intrusive reference count, the string length and the string data.
  // ATTENSION! Pointers to this class always point to this+1 !!!
  // This ist the location of the character data. All member functions take care of that.
  // The second trick is that the initialization is done at the new operator rather than the constructor.
  class StringData
  {private:
    volatile unsigned Count;
    size_t       Len; // Logically const, but since we initialize it before constructor
                      // it has to be non-const to avoid the error.
   private: // Avoid array creation and ordinary new operator!
    #if defined(__IBMCPP__) && defined(DEBUG_ALLOC)
    static void* operator new(size_t, const char*, size_t);
    static void* operator new[](size_t, const char*, size_t);
    static void  operator delete[](void*, const char*, size_t);
    #else
    static void* operator new(size_t);
    static void* operator new[](size_t);
    static void  operator delete[](void*);
    #endif
   private: // Non-copyable
                 StringData(const StringData&);
           void  operator=(const StringData&);
   private: // interface to int_ptr
    volatile unsigned& access_counter()     { return this[-1].Count; }
    friend class int_ptr<StringData>;
   public:
                 StringData()               { ((char*)this)[this[-1].Len] = 0; }
                 StringData(const char* str){ memcpy(this, str, this[-1].Len); ((char*)this)[this[-1].Len] = 0; }
    #if defined(__IBMCPP__) && defined(DEBUG_ALLOC)
    static void* operator new(size_t s, const char*, size_t, size_t l)
    #else
    static void* operator new(size_t s, size_t l)
    #endif
    { //assert(_heapchk() == _HEAPOK);
      StringData* that = (StringData*)new char[s+l+1];
      DEBUGLOG2(("xstring::StringData::operator new(%u, %u) - %p\n", s, l, cp));
      // Dirty early construction
      that->Count = 0;
      that->Len = l;
      return that+1;
    }
    #if defined(__IBMCPP__) && defined(DEBUG_ALLOC)
    static void  operator delete(void* p, const char*, size_t)
    #else
    static void  operator delete(void* p)
    #endif
    { DEBUGLOG2(("xstring::StringData::operator delete(%p)\n", p));
      delete[] (char*)((StringData*)p-1);
    }
    // Return the current strings length.
           size_t length() const            { return this[-1].Len; }
    // Return the current strings content.
    // Note that the special handling of the this pointer in the class causes this function
    // to be valid even if this == NULL.
           char* ptr() const                { return (char*)this; }
    // Convert a C string previously returned by ptr() back to StringData*.
    static StringData* fromPtr(char* str)   { return (StringData*)str; }
    // Access the ix-th character in the string. No range checking is made here.
           char& operator[](size_t ix)      { return ((char*)(this))[ix]; }
  };
 public:
  enum
  { npos = (size_t)-1
  };

 private:
  int_ptr<StringData> Data;
 public:
  // Empty string. This is different from a NULL string!
  static const xstring& empty;

 private:
  xstring(StringData* ref)                  : Data(ref) {}
 protected:
  xstring(size_t len)                       : Data(new (len) StringData) {}
 public:
  static int  compareI(const char* s1, const char* s2, size_t len);

  xstring()                                 {}
  // Copy constructor
  xstring(xstring& r)                       : Data(r.Data){} // Helper to disambiguate calls.
  xstring(const xstring& r)                 : Data(r.Data){}
  // [STS] Copy constructor, strongly thread-safe.
  xstring(volatile const xstring& r)        : Data(r.Data){}
  xstring(const xstring& r, size_t start);
  xstring(const xstring& r, size_t start, size_t len);
  xstring(const char* str);
  xstring(const char* str, size_t len);
  // length of the string. The string must not be NULL!
  size_t      length() const                { return Data->length(); }
  // explicit conversion
  // The returned pointer is valid as long as this string is owned by at least one xstring instance.
  const char* cdata() const                 { return Data.get()->ptr(); }
  // constant c-style array containing the string, always null terminated, maybe NULL.
  // The returned pointer is valid as long as this string is owned by at least one xstring instance.
  operator const char*() const              { return cdata(); }
  // return the i-th character
  const char& operator[](size_t i)          { ASSERT(i < length()); return (*Data)[i]; }
  // test for equality
  bool        equals(const xstring& r) const;
  bool        equals(const char* str) const;
  bool        equals(const char* str, size_t len) const;
  // compare strings (case sensitive)
  // The NULL string is the smallest possible string, less than "".
  int         compareTo(const xstring& r) const;
  int         compareTo(const char* str) const;
  int         compareTo(const char* str, size_t len) const;
  // compare strings (case insensitive)
  // This works only for 7 bit ASCII as expected.
  int         compareToI(const xstring& r) const;
  int         compareToI(const char* str) const;
  int         compareToI(const char* str, size_t len) const;

  bool        startsWith(const xstring& r) const { return startsWith(r.Data->ptr(), r.length()); }
  bool        startsWith(const char* r) const { return startsWith(r, strlen(r)); }
  bool        startsWith(const char* r, size_t len) const;
  bool        startsWith(char c) const      { return length() && (*Data)[0] == c; }
  bool        startsWithI(const xstring& r) const { return startsWithI(r.Data->ptr(), r.length()); }
  bool        startsWithI(const char* r) const { return startsWithI(r, strlen(r)); }
  bool        startsWithI(const char* r, size_t len) const;
  bool        startsWithI(char c) const     { return length() && tolower((*Data)[0]) == tolower(c); }
  bool        endsWith(const xstring& r) const { return endsWith(r.Data->ptr(), r.length()); }
  bool        endsWith(const char* r) const { return endsWith(r, strlen(r)); }
  bool        endsWith(const char* r, size_t len) const;
  bool        endsWith(char c) const        { return length() && (*Data)[length()-1] == c; }
  bool        endsWithI(const xstring& r) const { return endsWithI(r.Data->ptr(), r.length()); }
  bool        endsWithI(const char* r) const { return endsWithI(r, strlen(r)); }
  bool        endsWithI(const char* r, size_t len) const;
  bool        endsWithI(char c) const       { return length() && tolower((*Data)[length()-1]) == tolower(c); }
  // swap two instances
  void        swap(xstring& r)              { Data.swap(r.Data); }
  void        swap(volatile xstring& r)     { Data.swap(r.Data); }
  void        swap(xstring& r) volatile     { Data.swap(r.Data); }
  // Reset the string to NULL
  void        reset()                       { Data.reset(); }
  void        reset() volatile              { Data.reset(); }
  // assign new value
  void        assign(xstring& r)                { Data = r.Data; } // Helper to disambiguate calls.
  void        assign(const xstring& r)          { Data = r.Data; }
  void        assign(const xstring& r) volatile { Data = r.Data; }
  // assign, strongly thread safe
  void        assign(volatile const xstring& r)          { Data = r.Data; }
  void        assign(volatile const xstring& r) volatile { Data = r.Data; }
  // assign new slice
  void        assign(const xstring& r, size_t start)                      { xstring(r, start).swap(*this); }
  void        assign(const xstring& r, size_t start) volatile             { xstring(r, start).swap(*this); }
  void        assign(const xstring& r, size_t start, size_t len)          { xstring(r, start, len).swap(*this); }
  void        assign(const xstring& r, size_t start, size_t len) volatile { xstring(r, start, len).swap(*this); }
  // Assign new string from cdata.
  void        assign(const char* str);
  void        assign(const char* str) volatile             { xstring(str).swap(*this); }
  void        assign(const char* str, size_t len)          { xstring(str, len).swap(*this); }
  void        assign(const char* str, size_t len) volatile { xstring(str, len).swap(*this); }
  // initialize to new string with defined length and undefined content
  char*       raw_init(size_t len)          { Data = new(len) StringData; return Data->ptr(); }
  // initialize to new string with defined length and defined content that might be modified before the string is used elsewhere
  char*       raw_init(size_t len, const char* src) { assign(src, len); return Data->ptr(); }
  // assign new value by operator
  xstring&    operator=(xstring& r)                         { assign(r); return *this; } // Helper to disambiguate calls.
  xstring&    operator=(const xstring& r)                   { assign(r); return *this; }
  void        operator=(const xstring& r) volatile          { assign(r); }
  xstring&    operator=(volatile const xstring& r)          { assign(r); return *this; }
  void        operator=(volatile const xstring& r) volatile { assign(r); }
  xstring&    operator=(const char* str)          { assign(str); return *this; }
  void        operator=(const char* str) volatile { assign(str); }
  // concatenate strings
  // None of the strings must be NULL.
  friend const xstring operator+(const xstring& l, const xstring& r);
  friend const xstring operator+(const xstring& l, const char* r);
  friend const xstring operator+(const char* l,    const xstring& r);
  // printf, well...
  static const xstring sprintf(const char* fmt, ...);
  static const xstring vsprintf(const char* fmt, va_list va);
  /*// Interface to C API
  // Detach the current content from the instance and return a pointer to a C style string.
  // The reference to the content is kept active until it is placed back to a xstring object
  // by the opposite function fromCstr.
  const char* toCstr()                      { return Data.toCptr()->ptr(); }
  // Assign the current instance from a C style string pointer, taking the ownership.
  // This MUST be a pointer initially retrievd by toCstr().
  void        fromCstr(const char* str)     { Data.fromCptr(StringData::fromPtr((char*)str)); }
  // Assign the current instance from a C style string pointer, taking a reference copy.
  // This MUST be a pointer initially retrievd by toCstr() and the pointer must be passed to fromCstr later for cleanup.
  void        assignCstr(const char* str)   { StringData::fromPtr((char*)str); }
  // [STS] Assign the current instance from a C style string pointer, taking a reference copy.
  // This MUST be a pointer initially retrievd by toCstr() and the pointer must be passed to fromCstr later for cleanup.
  void        assignCstr_safe(const char*volatile const& str);
  // Replace the current string value by a C string previously returned by toCstr
  // and return the last value as C string pointer with the same properties.
  //const char* swapCstr(const char* str);
  static size_t lengthCstr(const char* str) { return StringData::fromPtr((char*)str)->length(); }*/
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
{ return r.compareTo(l) > 0;
}

inline bool operator<=(const xstring& l, const xstring& r)
{ return l.compareTo(r) <= 0;
}
inline bool operator<=(const xstring& l, const char* r)
{ return l.compareTo(r) <= 0;
}
inline bool operator<=(const char* l,    const xstring& r)
{ return r.compareTo(l) >= 0;
}

inline bool operator>(const xstring& l, const xstring& r)
{ return l.compareTo(r) > 0;
}
inline bool operator>(const xstring& l, const char* r)
{ return l.compareTo(r) > 0;
}
inline bool operator>(const char* l,    const xstring& r)
{ return r.compareTo(l) < 0;
}

inline bool operator>=(const xstring& l, const xstring& r)
{ return l.compareTo(r) >= 0;
}
inline bool operator>=(const xstring& l, const char* r)
{ return l.compareTo(r) >= 0;
}
inline bool operator>=(const char* l,    const xstring& r)
{ return r.compareTo(l) <= 0;
}

#endif
