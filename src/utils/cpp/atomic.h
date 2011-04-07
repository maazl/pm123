/*
 * Copyright 2008-2009 M.Mueller
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


#ifndef CPP_ATOMIC_H
#define CPP_ATOMIC_H

#include <config.h>
#include <interlocked.h>


/*****************************************************************************
*
*  Wrapper class around primitive functions to do atomic operations.
*
*****************************************************************************/

class AtomicUnsigned
{private:
  volatile unsigned data;
 public:
           AtomicUnsigned(unsigned i = 0) : data(i) {}
  // atomic operations
  unsigned swap(unsigned n)             { return InterlockedXch(&data, n); }
  bool     replace(unsigned o, unsigned n) { return (bool)InterlockedCxc(&data, o, n); }
  void     operator++()                 { InterlockedInc(&data); }
  bool     operator--()                 { return (bool)InterlockedDec(&data); }
  void     operator+=(unsigned n)       { InterlockedAdd(&data, n); }
  bool     operator-=(unsigned n)       { return (bool)InterlockedSub(&data, n); }
  bool     operator&=(unsigned n)       { return (bool)InterlockedAnd(&data, n); }
  void     operator|=(unsigned n)       { InterlockedOr(&data, n); }
  bool     operator^=(unsigned n)       { return (bool)InterlockedXor(&data, n); }
  bool     bitset(unsigned n)           { return (bool)InterlockedBts(&data, n); } 
  bool     bitrst(unsigned n)           { return (bool)InterlockedBtr(&data, n); } 
  bool     bitnot(unsigned n)           { return (bool)InterlockedBtc(&data, n); }
  unsigned maskset  (unsigned n)        { return InterlockedSet(&data, n); }
  unsigned maskreset(unsigned n)        { return InterlockedRst(&data, n); }
  // non atomic methods
  AtomicUnsigned& operator=(unsigned r) { data = r; return *this; }
  void     set       (unsigned r)       { data = r; }
  operator unsigned  () const           { return data; }
  unsigned get       () const           { return data; }
  bool     operator! () const           { return !data; }
  /*bool     operator==(unsigned r) const { return data == r; }
  bool     operator!=(unsigned r) const { return data != r; }
  bool     operator< (unsigned r) const { return data <  r; }
  bool     operator<=(unsigned r) const { return data <= r; }
  bool     operator> (unsigned r) const { return data >  r; }
  bool     operator>=(unsigned r) const { return data >= r; }*/
};


#endif
