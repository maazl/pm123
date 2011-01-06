/*
 * Copyright 2007-2010 M.Mueller
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


/** @brief Java like string class.
 * @details Copying is cheap.
 * The class is thread-safe. Volatile instances are strongly thread safe.
 * The memory footprint is the same as for \c char*.
 * The storage overhead is two ints per unique string. */
class xstring
{private:
  /** Data Structure for non-mutable strings.
   * It contains an intrusive reference count, the string length and the string data.
   * ATTENSION! Pointers to this class always point to this+1 !!!
   * This is the location of the character data. All member functions take care of that.
   * The second trick is that the initialization is done at the new operator rather than the constructor. */
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
    {
      #ifdef DEBUG_MEM
      assert(_heapchk() == _HEAPOK);
      #endif
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
    /// Return the current strings length.
           size_t length() const            { return this[-1].Len; }
    /// Return the current strings content.
    /// Note that the special handling of the this pointer in the class causes this function
    /// to be valid even if this == NULL.
           char* ptr() const                { return (char*)this; }
    /// Convert a C string previously returned by ptr() back to StringData*.
    static StringData* fromPtr(char* str)   { return (StringData*)str; }
    /// Access the ix-th character in the string. No range checking is made here.
           char& operator[](size_t ix)      { return ((char*)(this))[ix]; }
  };
 public:
  enum
  { npos = (size_t)-1
  };

 private:
  int_ptr<StringData> Data;
 public:
  /// Empty string. This is different from a NULL string!
  static const xstring& empty;

 private:
              xstring(StringData* ref)      : Data(ref) {}
 protected:
  /// Initialize a new \c xstring instance with \a len bytes of uninitialized storage.
  explicit    xstring(size_t len)           : Data(new(len) StringData) {}
 public:
  /// Case insensitive comparer core.
  static int  compareI(const char* s1, const char* s2, size_t len);

  /// Create instance with the default value NULL.
              xstring()                     {}
  /// @brief Copy constructor
  /// @details The function creates a reference copy. Complexity O(1).
              xstring(xstring& r)           : Data(r.Data){} // Helper to disambiguate calls.
  /// @brief Copy constructor
  /// @details The function creates a reference copy. Complexity O(1).
              xstring(const xstring& r)     : Data(r.Data){}
  /// @brief Strongly thread safe copy constructor.
  /// @details The function creates a reference copy. Complexity O(1).
              xstring(volatile const xstring& r) : Data(r.Data){}
  /// Create a new \c xstring from a slice of another instance. O(n).
              xstring(const xstring& r, size_t start);
  /// Create a new \c xstring from a slice of another instance. O(n).
              xstring(const xstring& r, size_t start, size_t len);
  /// Create a new \c xstring from a C style string. O(n).
              xstring(const char* str);
  /// @brief Create a new \c xstring from a C style memory block.
  /// @details The memory may contain 0-bytes. O(n).
              xstring(const char* str, size_t len);
  /// Length of the string. The string must not be NULL!
  size_t      length() const                { return Data->length(); }
  /// Is NULL? Strongly thread-safe.
  bool        operator!() const volatile    { return !Data; }
  /// @brief Explicit conversion to C style string.
  /// @details The returned string includes always an additional null terminator at the end.
  /// The returned pointer is valid as long as this string is owned by at least one xstring instance.
  const char* cdata() const                 { return Data.get()->ptr(); }
  /// @brief Implicit conversion to C-style string, always null terminated, maybe NULL.
  /// @details The returned pointer is valid as long as this string is owned by at least one xstring instance.
  operator const char*() const              { return cdata(); }
  /// Return the i-th character.
  const char  operator[](size_t i) const    { ASSERT(i <= length()); return (*Data)[i]; } // Access to \0 terminator allowed
  /// Test for equality with another instance.
  bool        equals(const xstring& r) const;
  /// Test for equality with C style string.
  bool        equals(const char* str) const;
  /// Test for equality with C style memory.
  bool        equals(const char* str, size_t len) const;
  /// @brief Compare for instance equality.
  /// @details Instance equality implies value equality but not the other way around.
  bool        instEquals(const xstring& r) const { return Data == r.Data; }
  /// Compare strings (case sensitive).
  /// The NULL string is the smallest possible string, less than an empty string.
  int         compareTo(const xstring& r) const;
  /// Compare strings (case sensitive).
  /// The NULL string is the smallest possible string, less than an empty string.
  int         compareTo(const char* str) const;
  /// Compare strings (case sensitive).
  /// The NULL string is the smallest possible string, less than an empty string.
  int         compareTo(const char* str, size_t len) const;
  /// Compare strings (case insensitive).
  /// This works only for 7 bit ASCII as expected.
  int         compareToI(const xstring& r) const;
  /// Compare strings (case insensitive).
  /// This works only for 7 bit ASCII as expected.
  int         compareToI(const char* str) const;
  /// Compare strings (case insensitive).
  /// This works only for 7 bit ASCII as expected.
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
  /// Swap two instances.
  void        swap(xstring& r)              { Data.swap(r.Data); }
  /// Swap two instances. Strongly thread-safe with respect to the right xstring.
  void        swap(volatile xstring& r)     { Data.swap(r.Data); }
  /// Swap two instances. Strongly thread-safe with respect to the left xstring.
  void        swap(xstring& r) volatile     { Data.swap(r.Data); }
  /// Reset the string to NULL.
  void        reset()                       { Data.reset(); }
  /// Reset the string to NULL. Thread-safe.
  void        reset() volatile              { Data.reset(); }
  /// @brief Assign new value.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(xstring& r)                { Data = r.Data; } // Helper to disambiguate calls.
  /// @brief Assign new value.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(const xstring& r)          { Data = r.Data; }
  /// @brief Assign new value.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(const xstring& r) volatile { Data = r.Data; }
  /// @brief Assign new value. Strongly thread-safe with respect to rhs.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(volatile const xstring& r)          { Data = r.Data; }
  /// @brief Assign new value. Strongly thread-safe with respect to both strings.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(volatile const xstring& r) volatile { Data = r.Data; }
  /// Assign new slice.
  void        assign(const xstring& r, size_t start)                      { xstring(r, start).swap(*this); }
  /// Assign new slice. Thread-safe with respect to lhs.
  void        assign(const xstring& r, size_t start) volatile             { xstring(r, start).swap(*this); }
  /// Assign new slice.
  void        assign(const xstring& r, size_t start, size_t len)          { xstring(r, start, len).swap(*this); }
  /// Assign new slice. Thread-safe with respect to lhs.
  void        assign(const xstring& r, size_t start, size_t len) volatile { xstring(r, start, len).swap(*this); }
  /// Assign new string from C-style string.
  void        assign(const char* str);
  /// Assign new string from C-style string. Thread-safe with respect to lhs.
  void        assign(const char* str) volatile             { xstring(str).swap(*this); }
  /// Assign new string from C-style string.
  void        assign(const char* str, size_t len)          { xstring(str, len).swap(*this); }
  /// Assign new string from C-style string. Thread-safe with respect to lhs.
  void        assign(const char* str, size_t len) volatile { xstring(str, len).swap(*this); }
  /// @brief Assign and return \c true if changed.
  /// @details \c *this is not assigned if the value is equal.
  bool        cmpassign(const xstring& r)                  { return !equals(r) && (Data = r.Data, true); }
  /// Initialize to new string with defined length and undefined content.
  /// @return The return value points to the newly allocated content.
  /// The storage in the range [0,len) must not be modified after the current instance is either modified
  /// or passed to a copy constructor or an assignment operator.
  char*       allocate(size_t len)                         { Data = new(len) StringData; return Data->ptr(); }
  /// initialize to new string with defined length and defined content that might be modified before the string is used elsewhere.
  /// @return The return value points to the newly allocated content.
  /// The storage in the range [0,len) must not be modified after the current instance is either modified
  /// or passed to a copy constructor or an assignment operator.
  char*       allocate(size_t len, const char* src)        { assign(src, len); return Data->ptr(); }
  /// Assign new value by operator.
  xstring&    operator=(xstring& r)                         { assign(r); return *this; } // Helper to disambiguate calls.
  /// Assign new value by operator.
  xstring&    operator=(const xstring& r)                   { assign(r); return *this; }
  /// Assign new value by operator. Thread-safe with respect to lhs.
  void        operator=(const xstring& r) volatile          { assign(r); }
  /// Assign new value by operator. Strongly thread-safe with respect to rhs.
  xstring&    operator=(volatile const xstring& r)          { assign(r); return *this; }
  /// Assign new value by operator. Strongly thread-safe with respect to both xstrings.
  void        operator=(volatile const xstring& r) volatile { assign(r); }
  /// Assign new value by operator from C-style string.
  xstring&    operator=(const char* str)          { assign(str); return *this; }
  /// Assign new value by operator from C-style string.
  void        operator=(const char* str) volatile { assign(str); }
  /// @brief Concatenate strings.
  /// @details The strings must not be NULL.
  friend const xstring operator+(const xstring& l, const xstring& r);
  /// @brief Concatenate strings.
  /// @details The strings must not be NULL.
  friend const xstring operator+(const xstring& l, const char* r);
  /// @brief Concatenate strings.
  /// @details The strings must not be NULL.
  friend const xstring operator+(const char* l,    const xstring& r);
  /// sprintf, well...
  static const xstring sprintf(const char* fmt, ...);
  /// vsprintf, well...
  static const xstring vsprintf(const char* fmt, va_list va);
  /// @brief Interface to C API.
  /// @details Detach the current content from the instance and return a pointer to a C style string.
  /// The reference to the content is kept active until it is placed back to a \c xstring object
  /// by the opposite function \c fromCstr.
  const char* toCstr()                      { return Data.toCptr()->ptr(); }
  /// @brief Assign the current instance from a C style string pointer, taking the ownership.
  /// @details This MUST be a pointer initially retrieved by \c toCstr().
  void        fromCstr(const char* str)     { Data.fromCptr(StringData::fromPtr((char*)str)); }
  /// @brief Assign the current instance from a C style string pointer, taking a reference copy.
  /// @details This MUST be a pointer initially retrieved by toCstr() and
  /// the pointer must be passed to \c fromCstr later for cleanup.
  void        assignCstr(const char* str)   { StringData::fromPtr((char*)str); }
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
