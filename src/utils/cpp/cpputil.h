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


#ifndef CPP_UTIL_H
#define CPP_UTIL_H

// swap two objects
template <class T>
inline void swap(T& a, T& b)
{ T c = a;
  a = b;
  b = c;
}

template <class T>
inline T xchg(T& dst, T src)
{ T c = dst;
  dst = src;
  return c;
}

#define FLAGSATTRIBUTE(T) \
inline static T operator|(T l, T r) \
{ return (T)((unsigned)l|r); } \
inline static T operator&(T l, T r) \
{ return (T)((unsigned)l&r); } \
inline static T& operator|=(T& l, T r) \
{ return l = (T)((unsigned)l|r); } \
inline static T& operator&=(T& l, T r) \
{ return l = (T)((unsigned)l&r); } \
inline static T operator*(bool l, T r) \
{ return (T)(l*(unsigned)r); } \
inline static T operator*(T l, bool r) \
{ return (T)((unsigned)l*r); } \
inline static T operator~(T a) \
{ return (T)~(unsigned)a; }

// Unspecified types for operator ! and == NULL
struct unspecified_struct_type;
typedef const volatile unspecified_struct_type* unspecified_bool_type;


#endif
