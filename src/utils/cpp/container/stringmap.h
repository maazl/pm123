/*
 * Copyright 2008-2011 M.Mueller
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

#ifndef STRINGMAP_H
#define STRINGMAP_H

#include <cpp/xstring.h>
#include <cpp/container/sorted_vector.h>


typedef sorted_vector<xstring, xstring, &xstring::compare> stringset;

class stringset_own : public stringset
{protected:
  // Copy constructor
  // Note that since sorted_vector_own own its object exclusively this copy constructor must do
  // a deep copy of the vector. This is up to the derived class!
  // You may use vector_own_base_copy to do the job.
  stringset_own(const stringset_own& r, size_t spare = 0) : stringset(r.size() + spare) {}
  // assignment: same problem as above
  stringset_own& operator=(const stringset_own& r);
 public:
  stringset_own() : stringset() {}
  stringset_own(size_t capacity) : stringset(capacity) {}
  void          clear() { vector_own_base_destroy(*this); }
  ~stringset_own() { clear(); }
};


template <class V>
struct strmapentry
{ const xstring Key;
  V             Value;
  strmapentry(const xstring& key) : Key(key) {}
  strmapentry(const xstring& key, const V& value) : Key(key), Value(value) {}
  static int    compare(const xstring& k, const strmapentry& e);
};

template <class V>
int strmapentry<V>::compare(const xstring& key, const strmapentry& elem)
{ return key.compareTo(elem.Key);
}

typedef strmapentry<xstring> stringmapentry;
typedef sorted_vector<stringmapentry, xstring, &strmapentry<xstring>::compare> stringmap;

class stringmap_own : public stringmap
{public:
  stringmap_own(size_t capacity) : stringmap(capacity) {}
  void          clear() { vector_own_base_destroy(*this); }
  ~stringmap_own() { clear(); }
};


/** @brief Lightweight structure for string based dispatching.
 * @details The structure has a POD layout and starts with a null terminated C string.
 * This allows a conversion of strmap* to char*. In fact you can search
 * in a sorted array of strmap with bsearch and strcmp.
 * Once you got a match you have an object of an arbitrary type V.
 * This could e.g. be a function pointer. 
 */
template <int L, class V>
struct strmap
{ char Str[L];
  V    Val;
};

/** Compares \a str against \a abbrev and returns 0 if \a str starts with \a abbrev.
 */
int TFNENTRY strabbrevicmp(const char* str, const char* abbrev);

const char* mapsearcha2_core(const char* cmd, const char* map, size_t count, size_t size);

template <class T>
inline T* mapsearch2(T* map, size_t count, const char* cmd)
{ return (T*)bsearch(cmd, map, count, sizeof(T), (int(TFNENTRY*)(const void*, const void*))&stricmp);
}
template <class T>
inline T* mapsearcha2(T* map, size_t count, const char* cmd)
{ return (T*)mapsearcha2_core(cmd, (const char*)map, count, sizeof(T));
}
#ifdef __IBMCPP__
// IBM C work around
// IBM C cannot deduce the array size from the template argument.
#define mapsearch(map, arg) mapsearch2((map), sizeof(map) / sizeof *(map), arg)
#define mapsearcha(map, arg) mapsearcha2((map), sizeof(map) / sizeof *(map), arg)
#else
template <size_t I, class T>
inline T* mapsearch(T (&map)[I], const char* cmd)
{ return (T*)bsearch(cmd, map, I, sizeof(T), (int(TFNENTRY*)(const void*, const void*))&stricmp);
}
template <size_t I, class T>
inline T* mapsearcha(T (&map)[I], const char* cmd)
{ return (T*)mapsearcha2_core(cmd, (const char*)map, I, sizeof(T));
}
#endif

#endif

