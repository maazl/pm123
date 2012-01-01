/*
 * Copyright 2007-2011 M.Mueller
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
#include <ctype.h>
#include <stdarg.h>
#include <cpp/smartptr.h>


#ifdef PM123_CORE
/* Base implementation in the PM123 core
 * Plug-ins will use a proxy implementation below.
 */

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
  /// Compare two strings.
  static int  compare(const xstring& l, const xstring& r) { return l.compareTo(r); }
  /// Compare strings (case insensitive).
  /// This works only for 7 bit ASCII as expected.
  int         compareToI(const xstring& r) const;
  /// Compare strings (case insensitive).
  /// This works only for 7 bit ASCII as expected.
  int         compareToI(const char* str) const;
  /// Compare strings (case insensitive).
  /// This works only for 7 bit ASCII as expected.
  int         compareToI(const char* str, size_t len) const;
  /// Case insensitive comparer core.
  static int  compareI(const char* s1, const char* s2, size_t len);
  /// Compare two strings.
  static int  compareI(const xstring& l, const xstring& r) { return l.compareToI(r); }

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
  void        assign(xstring& r)            { Data = r.Data; } // Helper to disambiguate calls.
  /// @brief Assign new value.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(const xstring& r)      { Data = r.Data; }
  /// @brief Assign new value.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(const xstring& r) volatile { Data = r.Data; }
  /// @brief Assign new value. Strongly thread-safe with respect to rhs.
  /// @details The function creates a reference copy. Complexity O(1).
  void        assign(volatile const xstring& r) { Data = r.Data; }
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
  void        assign(const char* str) volatile { xstring(str).swap(*this); }
  /// Assign new string from C-style string.
  void        assign(const char* str, size_t len) { xstring(str, len).swap(*this); }
  /// Assign new string from C-style string. Thread-safe with respect to lhs.
  void        assign(const char* str, size_t len) volatile { xstring(str, len).swap(*this); }
  /// @brief Assign and return \c true if changed.
  /// @details \c *this is not assigned if the value is equal.
  bool        cmpassign(const xstring& r)   { return !equals(r) && (Data = r.Data, true); }
  /// Initialize to new string with defined length and undefined content.
  /// @return The return value points to the newly allocated content.
  /// The storage in the range [0,len) must not be modified after the current instance is either modified
  /// or passed to a copy constructor or an assignment operator.
  char*       allocate(size_t len)          { Data = new(len) StringData; return Data->ptr(); }
  /// initialize to new string with defined length and defined content that might be modified before the string is used elsewhere.
  /// @return The return value points to the newly allocated content.
  /// The storage in the range [0,len) must not be modified after the current instance is either modified
  /// or passed to a copy constructor or an assignment operator.
  char*       allocate(size_t len, const char* src) { assign(src, len); return Data->ptr(); }
  /// Assign new value by operator.
  xstring&    operator=(xstring& r)         { assign(r); return *this; } // Helper to disambiguate calls.
  /// Assign new value by operator.
  xstring&    operator=(const xstring& r)   { assign(r); return *this; }
  /// Assign new value by operator. Thread-safe with respect to lhs.
  void        operator=(const xstring& r) volatile { assign(r); }
  /// Assign new value by operator. Strongly thread-safe with respect to rhs.
  xstring&    operator=(volatile const xstring& r) { assign(r); return *this; }
  /// Assign new value by operator. Strongly thread-safe with respect to both xstrings.
  void        operator=(volatile const xstring& r) volatile { assign(r); }
  /// Assign new value by operator from C-style string.
  xstring&    operator=(const char* str)    { assign(str); return *this; }
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

#else
/* When compiling in plug-in context provide a xstring proxy implementation.
 */

/** For the proxy to work a global symbol Ctx with the parameter ctx of plugin_init must be supplied.
 */
extern PLUGIN_CONTEXT Ctx;

/** 2nd Hack: In C++ we forward the string API calls to the runtime of the
 * PM123 core.
 * Note that you \e must not call any of these functions in a static initializer,
 * because at this point \c plugin_init has not yet been called.
 */
class xstring
{ const char* cstr;
 public:
  xstring() : cstr(NULL) {}
  xstring(xstring& r)                       : cstr(NULL) { Ctx.xstring_api->copy(this, &r); }
  xstring(const xstring& r)                 : cstr(NULL) { Ctx.xstring_api->copy(this, &r); }
  xstring(const volatile xstring& r)        : cstr(NULL) { Ctx.xstring_api->copy_safe(this, &r); }
  xstring(const char* r)                    : cstr(r ? ((const char* DLLENTRYP()(const char*))Ctx.xstring_api->create)(r) : NULL) {}
  xstring(const char* r, size_t len)        : cstr(NULL) { if (r) memcpy(Ctx.xstring_api->allocate(this, len), r, len); }
  ~xstring()                                { Ctx.xstring_api->free(this); }
  size_t      length() const                { return Ctx.xstring_api->length(this); }
  bool        operator!() const volatile    { return !cstr; }
  const char* cdata() const                 { return cstr; }
  operator const char*() const              { return cstr; }
  const char& operator[](size_t i) const    { ASSERT(i <= length()); return cstr[i]; } // Access to \0 terminator allowed
  bool        equals(const xstring& r) const { return Ctx.xstring_api->compare(this, &r) == 0; }
  int         compareTo(const xstring& r) const { return Ctx.xstring_api->compare(this, &r); }
  static int  compare(const xstring& l, const xstring& r) { return l.compareTo(r); }
  void        reset()                       { Ctx.xstring_api->free(this); }
  void        assign(xstring& r)            { Ctx.xstring_api->copy(this, &r); } // Helper to disambiguate calls.
  void        assign(const xstring& r)      { Ctx.xstring_api->copy(this, &r); }
  void        assign(const xstring& r) volatile { Ctx.xstring_api->copy(this, &r); }
  void        assign(volatile const xstring& r) { Ctx.xstring_api->copy_safe(this, &r); }
  void        assign(volatile const xstring& r) volatile { Ctx.xstring_api->copy_safe(this, &r); }
  void        assign(const char* str)       { Ctx.xstring_api->assign(this, str); }
  void        assign(const char* str) volatile { Ctx.xstring_api->assign(this, str); }
  void        assign(const char* str, size_t len) { memcpy(Ctx.xstring_api->allocate(this, len), str, len); }
  bool        cmpassign(const char* str)    { return Ctx.xstring_api->cmpassign(this, str); }
  xstring&    operator=(xstring& r)         { Ctx.xstring_api->copy(this, &r); return *this; } // Helper to disambiguate calls.
  xstring&    operator=(const xstring& r)   { Ctx.xstring_api->copy(this, &r); return *this; }
  void        operator=(const xstring& r) volatile { Ctx.xstring_api->copy(this, &r); }
  xstring&    operator=(volatile const xstring& r) { Ctx.xstring_api->copy_safe(this, &r); return *this; }
  void        operator=(volatile const xstring& r) volatile { Ctx.xstring_api->copy_safe(this, &r); }
  xstring&    operator=(const char* str)    { Ctx.xstring_api->assign(this, str); return *this; }
  void        operator=(const char* str) volatile { Ctx.xstring_api->assign(this, str); }
  void        operator+=(const char* str)   { Ctx.xstring_api->append(this, str); }
  char*       allocate(size_t len)          { return Ctx.xstring_api->allocate(this, len); }
  char*       allocate(size_t len, const char* src) { char* dst = Ctx.xstring_api->allocate(this, len); memcpy(dst, src, len); return dst; }
  xstring&    vsprintf(const char* fmt, va_list va) { Ctx.xstring_api->vsprintf(this, fmt, va); return *this; }
  xstring&    sprintf(const char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(fmt, va); va_end(va); return *this; }
};
/*inline size_t   xstring::length() const { return Ctx.xstring_api->length(this); }
inline int      xstring::compareTo(const xstring& r) const { return Ctx.xstring_api->compare(this, &r); }
inline char*    xstring::allocate(size_t len) { return Ctx.xstring_api->allocate(this, len); }
inline bool     xstring::cmpassign(const char* str) { return Ctx.xstring_api->cmpassign(this, str); }
inline void     xstring::reset() { Ctx.xstring_api->free(this); }
inline xstring& xstring::operator=(xstring& r) { Ctx.xstring_api->copy(this, &r); return *this; }
inline xstring& xstring::operator=(const xstring& r) { Ctx.xstring_api->copy(this, &r); return *this; }
inline void     xstring::operator=(const xstring& r) volatile { Ctx.xstring_api->copy(this, &r); }
inline xstring& xstring::operator=(volatile const xstring& r) { Ctx.xstring_api->copy_safe(this, &r); return *this; }
inline void     xstring::operator=(volatile const xstring& r) volatile { Ctx.xstring_api->copy_safe(this, &r); }
inline xstring& xstring::operator=(const char* str) { Ctx.xstring_api->assign(this, str); return *this; }
inline void     xstring::operator=(const char* str) volatile { Ctx.xstring_api->assign(this, str); }
inline void     xstring::operator+=(const char* str) { Ctx.xstring_api->append(this, str); }
inline xstring& xstring::sprintf(const char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(fmt, va); return *this; }
inline xstring& xstring::vsprintf(const char* fmt, va_list va) { Ctx.xstring_api->vsprintf(this, fmt, va); return *this; }*/
#endif

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


/** String builder for xstrings or C strings.
 * @remarks \c xstringbuilder must not refer to any non-inline function of \c xstring
 * because of the proxy implementation of \c xstring for plug-ins.
 */
class xstringbuilder
{private:
  static char* const Empty;
  char*       Data;
  size_t      Cap;
  size_t      Len;

 private: // non copyable
  xstringbuilder(const xstringbuilder& r);
  void        operator=(const xstringbuilder& r);
 private:
  /// Ensure space for \a cap characters, \a cap must be larger than Cap.
  /// After the function returns the storage is no longer null terminated.
  void        auto_alloc(size_t cap);
  /// Ensure space for \a cap characters, do not copy the current content.
  /// @return Returns the newly allocated storage. \c Data still points to the old storage.
  char*       auto_alloc_raw(size_t& cap);
  char*       replace_core(size_t at, size_t len1, size_t len2);
 public:
  /// Create empty string builder
  xstringbuilder()                          : Data(Empty), Cap(0), Len(0) {}
  /// Create string builder with initial capacity. (The length is still 0.)
  xstringbuilder(size_t cap);
  /// Destroy xstringbuilder and it's data.
  ~xstringbuilder()                         { if (Cap) delete[] Data; }

  /// Current length of the string.
  size_t      length() const                { return Len; }
  /// @brief Explicit conversion to C-style string, always null terminated, never NULL.
  const char* cdata() const                 { return Data; }
  /// @brief Explicit conversion to C-style string, always null terminated, never NULL.
  char*       cdata()                       { return Data; }
  /// @brief Implicit conversion to C-style string, always null terminated, never NULL.
  operator    const char*() const           { return Data; }
  /// @brief Explicit conversion to \c xstring, never NULL.
  xstring     get() const                   { return xstring(Data, Len); }
  /// @brief Implicit conversion to \c xstring, never NULL.
  operator    xstring() const               { return xstring(Data, Len); }
  /// Return a C style string that must be freed with \c delete[].
  /// This function implicitly resets the \c xstringbuilder to its initial state.
  char*       detach_array();

  /// Read the \a at's character.
  char        operator[](size_t at) const   { ASSERT(at <= Len); return Data[at]; }
  /// Access the \a at's character.
  char&       operator[](size_t at)         { ASSERT(at < Len); return Data[at]; }

  /// Clears the content but do not free the storage.
  void        clear()                       { if (Len) { Len = 0; *Data = 0; } }
  /// Adjust the string length to \a len, pad with \0 if required.
  /// @remarks This will not free any storage.
  void        resize(size_t len);
  /// Adjust the string length to \a len and pad with \a filler if required.
  /// @remarks This will not free any storage.
  void        resize(size_t len, char filler);

  /// Current length of the string.
  size_t      capacity() const              { return Cap; }
  /// Adjust capacity. This never modifies the logical content.
  void        reserve(size_t cap);

  /// Reset the \c xstringbuilder to its initial state. This frees the buffer.
  void        reset();
  /// Clear the \c xstringbuilder and adjust it's capacity to cap.
  void        reset(size_t cap)             { Len = 0; reserve(cap); }

  /// Append a sequence of characters.
  void        append(const char* str, size_t len);
  /// Append a C string.
  void        append(const char* str)       { append(str, strlen(str)); }
  /// Append an \c xstring.
  void        append(const xstring& str)    { append(str, str.length()); }
  /// Append a single character.
  void        append(char c);
  /// Append \a count copies of \a c.
  void        append(size_t count, char c);
  /// Append an integer.
  void        append(int c);
  /// Append a formatted string.
  void        appendf(const char* fmt, ...);
  /// Append a formatted string.
  void        vappendf(const char* fmt, va_list va);

  /// Append a single character.
  xstringbuilder& operator+=(char c)        { append(c); return *this; }
  /// Append a C string.
  xstringbuilder& operator+=(const char* str) { append(str); return *this; }

  /// Insert a sequence of characters at position \a at.
  /// This function cannot operate in place.
  void        insert(size_t at, const char* str, size_t len);
  /// Insert a C string at position \a at.
  /// This function cannot operate in place.
  void        insert(size_t at, const char* str);
  /// Insert an \c xstring at position \a at.
  void        insert(size_t at, const xstring& str);
  /// Insert a single character at position \a at.
  void        insert(size_t at, char c);
  /// Insert \a count copies of \a c at position \a at.
  void        insert(size_t at, size_t count, char c);
  /// Insert a formatted string at position \a at.
  void        insertf(size_t at, const char* fmt, ...);
  /// Insert a formatted string at position \a at.
  void        vinsertf(size_t at, const char* fmt, va_list va);

  /// Erase all characters starting at \a from.
  /// @remarks This will not free any storage.
  void        erase(size_t from);
  /// Erase \a len characters starting from \a at.
  /// @remarks This will not free any storage.
  void        erase(size_t at, size_t len);

  /// Replace a section by a sequence of characters.
  /// This function cannot operate in place.
  void        replace(size_t from, size_t len1, const char* str, size_t len2);
  /// Replace a section by a C string.
  /// This function cannot operate in place.
  void        replace(size_t from, size_t len, const char* str);
  /// Replace a section by an \c xstring.
  void        replace(size_t from, size_t len, const xstring& str);
  /// Replace a section by a single character.
  void        replace(size_t from, size_t len, char c);
  /// Replace a section by \a count copies of \a c.
  void        replace(size_t from, size_t len, size_t count, char c);
  /// Replace a section by a formatted string.
  void        replacef(size_t from, size_t len, const char* fmt, ...);
  /// Replace a section by a formatted string.
  void        vreplacef(size_t from, size_t len, const char* fmt, va_list va);

  /// Search for the first occurrence of \a c in the current content, starting at \a pos.
  /// @return return the position of the found character or \c length() in case of no match.
  size_t      find(char c, size_t pos = 0);
  /// Search for the first occurrence of \a str[0..len] in the current content, starting at \a pos.
  /// @return return the start position of the found substring or \c length() in case of no match.
  /// @remarks An empty string will always match: len == 0 => return pos
  size_t      find(const char* str, size_t len, size_t pos);
  /// Search for the first occurrence of \a str in the current content, starting at \a pos.
  /// @return return the start position of the found substring or \c length() in case of no match.
  /// @remarks An empty string will always match at \a pos.
  size_t      find(const char* str, size_t pos) { return find(str, strlen(str), pos); }
  /// Search for the first occurrence of \a str in the current content, starting at \a pos.
  /// @return return the start position of the found substring or \c length() in case of no match.
  /// @remarks An empty string will always match: str.length() == 0 => return pos
  size_t      find(const xstring& str, size_t pos = 0) { return find(str, str.length(), pos); }

  /// Search for the last occurrence of \a c in the current content, starting at \a pos-1.
  /// @return return the position after the found character or \c 0 in case of no match.
  size_t      rfind(char c, size_t pos);
  /// Search for the last occurrence of \a c in the current content.
  /// @return return the position after the found character or \c 0 in case of no match.
  size_t      rfind(char c)                 { return rfind(c, length()); }
  /// Search for the last occurrence of \a str[0..len] in the current content, starting at \a pos-1.
  /// @return return one after the start position of the found substring or \c 0 in case of no match.
  /// @remarks An empty string will always match: len == 0 => return pos
  size_t      rfind(const char* str, size_t len, size_t pos);
  /// Search for the last occurrence of \a str in the current content, starting at \a pos-1.
  /// @return return one after the start position of the found substring or \c 0 in case of no match.
  /// @remarks An empty string will always match: len == 0 => return pos
  size_t      rfind(const char* str, size_t pos) { return rfind(str, strlen(str), pos); }
  /// Search for the last occurrence of \a str in the current content, starting at \a pos-1.
  /// @return return one after the start position of the found substring or \c 0 in case of no match.
  /// @remarks An empty string will always match: str.length() == 0 => return pos
  size_t      rfind(const xstring& str, size_t pos) { return rfind(str, str.length(), pos); }
  /// Search for the last occurrence of \a str in the current content, starting at \a pos-1.
  /// @return return one after the start position of the found substring or \c 0 in case of no match.
  /// @remarks An empty string will always match: str.length() == 0 => return pos
  size_t      rfind(const xstring& str)     { return rfind(str, str.length(), length()); }

  /// Search for the first occurrence of any character in \a str[0..len] in the current content,
  /// starting at \a pos.
  /// @return return the position of the found character or \c length() in case of no match.
  size_t      find_any(const char* str, size_t len, size_t pos);
  /// Search for the first occurrence of any character in \a str in the current content,
  /// starting at \a pos.
  /// @return return the position of the found character or \c length() in case of no match.
  size_t      find_any(const char* str, size_t pos) { return find_any(str, strlen(str), pos); }
  /// Search for the first occurrence of any character in \a str in the current content,
  /// starting at \a pos.
  /// @return return the position of the found character or \c length() in case of no match.
  size_t      find_any(const xstring& str, size_t pos = 0) { return find_any(str, str.length(), pos); }

  /// Search for the first occurrence of any character not in \a str[0..len] in the current content,
  /// starting at \a pos.
  /// @return return the position of the found character or \c length() in case of no match.
  size_t      find_not_any(const char* str, size_t len, size_t pos);
  /// Search for the first occurrence of any character not in \a str in the current content,
  /// starting at \a pos.
  /// @return return the position of the found character or \c length() in case of no match.
  size_t      find_not_any(const char* str, size_t pos) { return find_not_any(str, strlen(str), pos); }
  /// Search for the first occurrence of any character not in \a str in the current content,
  /// starting at \a pos.
  /// @return return the position of the found character or \c length() in case of no match.
  size_t      find_not_any(const xstring& str, size_t pos = 0) { return find_not_any(str, str.length(), pos); }
};

inline void xstringbuilder::reset()
{ Len = 0;
  if (Cap)
    delete[] Data;
  Cap = 0;
  Data = Empty;
}

inline void xstringbuilder::append(char c)
{ if (Len == Cap)
    auto_alloc(Len+1);
  Data[Len++] = c;
  Data[Len] = 0;
}
inline void xstringbuilder::append(int i)
{ appendf("%i", i);
}

inline void xstringbuilder::insert(size_t at, const char* str, size_t len)
{ ASSERT(!len || str+len <= Data || str >= Data+Len); // In place operation not supported.
  memcpy(replace_core(at, 0, len), str, len);
}
inline void xstringbuilder::insert(size_t at, const char* str)
{ insert(at, str, strlen(str));
}
inline void xstringbuilder::insert(size_t at, const xstring& str)
{ memcpy(replace_core(at, 0, str.length()), str, str.length());
}
inline void xstringbuilder::insert(size_t at, size_t count, char c)
{ memset(replace_core(at, 0, count), c, count);
}

inline void xstringbuilder::erase(size_t from)
{ ASSERT(from <= Len);
  Len = from;
  Data[Len] = 0;
}
inline void xstringbuilder::erase(size_t at, size_t len)
{ size_t end = len + at;
  ASSERT(end <= Len);
  memmove(Data+at, Data+end, Len-end+1);
  Len -= len;
}

inline void xstringbuilder::replace(size_t from, size_t len1, const char* str, size_t len2)
{ ASSERT(!len2 || str+len2 <= Data || str >= Data+Len); // In place operation not supported.
  memcpy(replace_core(from, len1, len2), str, len2);
}
inline void xstringbuilder::replace(size_t from, size_t len, const char* str)
{ replace(from, len, str, strlen(str));
}
inline void xstringbuilder::replace(size_t from, size_t len, const xstring& str)
{ memcpy(replace_core(from, len, str.length()), str, str.length());
}
inline void xstringbuilder::replace(size_t from, size_t len, char c)
{ *replace_core(from, len, 1) = c;
}
inline void xstringbuilder::replace(size_t from, size_t len, size_t count, char c)
{ memset(replace_core(from, len, count), c, count);
}

#endif
