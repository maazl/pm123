/*
 * Copyright 2007-2008 M.Mueller
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


#ifndef INTERLOCKED_H
#define INTERLOCKED_H

#include <config.h>


/*****************************************************************************
*
*  Primitive functions to do atomic operations.
*
*****************************************************************************/

extern const unsigned char InterlockedXchCode[];
extern const unsigned char InterlockedIncCode[];
extern const unsigned char InterlockedDecCode[];
extern const unsigned char InterlockedAddCode[];
extern const unsigned char InterlockedSubCode[];
extern const unsigned char InterlockedAndCode[];
extern const unsigned char InterlockedOrCode[];
extern const unsigned char InterlockedXorCode[];
extern const unsigned char InterlockedBtsCode[];
extern const unsigned char InterlockedBtrCode[];
extern const unsigned char InterlockedBtcCode[];

#if defined(__GNUC__)
  #define REGCALL __attribute__((regparm(2)))
  #define InterlockedXch(x,n) (*(unsigned REGCALL(*)(volatile unsigned*,unsigned))InterlockedXchCode)(&(x),(n))
  #define InterlockedInc(x)   (*(void     REGCALL(*)(volatile unsigned*))InterlockedIncCode)(&(x))
  #define InterlockedDec(x)   (*(char     REGCALL(*)(volatile unsigned*))InterlockedDecCode)(&(x))
  #define InterlockedAdd(x,n) (*(void     REGCALL(*)(volatile unsigned*,unsigned))InterlockedAddCode)(&(x),(n))
  #define InterlockedSub(x,n) (*(char     REGCALL(*)(volatile unsigned*,unsigned))InterlockedSubCode)(&(x),(n))
  #define InterlockedAnd(x,n) (*(char     REGCALL(*)(volatile unsigned*,unsigned))InterlockedAndCode)(&(x),(n))
  #define InterlockedOr(x,n)  (*(void     REGCALL(*)(volatile unsigned*,unsigned))InterlockedOrCode)(&(x),(n))
  #define InterlockedXor(x,n) (*(char     REGCALL(*)(volatile unsigned*,unsigned))InterlockedXorCode)(&(x),(n))
  #define InterlockedBts(x,n) (*(char     REGCALL(*)(volatile unsigned*,unsigned))InterlockedBtsCode)(&(x),(n))
  #define InterlockedBtr(x,n) (*(char     REGCALL(*)(volatile unsigned*,unsigned))InterlockedBtrCode)(&(x),(n))
  #define InterlockedBtc(x,n) (*(char     REGCALL(*)(volatile unsigned*,unsigned))InterlockedBtcCode)(&(x),(n))
#elif defined(__WATCOMC__)
  #define InterlockedXch(x,n) (*(unsigned(*)(volatile unsigned*,unsigned))InterlockedXchCode)(&(x),(n))
  #define InterlockedInc(x)   (*(void(*)(volatile unsigned*))InterlockedIncCode)(&(x))
  #define InterlockedDec(x)   (*(char(*)(volatile unsigned*))InterlockedDecCode)(&(x))
  #define InterlockedAdd(x,n) (*(void(*)(volatile unsigned*,unsigned))InterlockedAddCode)(&(x),(n))
  #define InterlockedSub(x,n) (*(char(*)(volatile unsigned*,unsigned))InterlockedSubCode)(&(x),(n))
  #define InterlockedAnd(x,n) (*(char(*)(volatile unsigned*,unsigned))InterlockedAndCode)(&(x),(n))
  #define InterlockedOr(x,n)  (*(void(*)(volatile unsigned*,unsigned))InterlockedOrCode)(&(x),(n))
  #define InterlockedXor(x,n) (*(char(*)(volatile unsigned*,unsigned))InterlockedXorCode)(&(x),(n))
  #define InterlockedBts(x,n) (*(char(*)(volatile unsigned*,unsigned))InterlockedBtsCode)(&(x),(n))
  #define InterlockedBtr(x,n) (*(char(*)(volatile unsigned*,unsigned))InterlockedBtrCode)(&(x),(n))
  #define InterlockedBtc(x,n) (*(char(*)(volatile unsigned*,unsigned))InterlockedBtcCode)(&(x),(n))
#elif defined(__IBMC__) || defined(__IBMCPP__)
  #define InterlockedXch(x,n) (*(unsigned(_Optlink*)(volatile unsigned*,unsigned))InterlockedXchCode)(&(x),(n))
  #define InterlockedInc(x)   (*(void(_Optlink*)(volatile unsigned*))InterlockedIncCode)(&(x))
  #define InterlockedDec(x)   (*(char(_Optlink*)(volatile unsigned*))InterlockedDecCode)(&(x))
  #define InterlockedAdd(x,n) (*(void(_Optlink*)(volatile unsigned*,unsigned))InterlockedAddCode)(&(x),(n))
  #define InterlockedSub(x,n) (*(char(_Optlink*)(volatile unsigned*,unsigned))InterlockedSubCode)(&(x),(n))
  #define InterlockedAnd(x,n) (*(char(_Optlink*)(volatile unsigned*,unsigned))InterlockedAndCode)(&(x),(n))
  #define InterlockedOr(x,n)  (*(void(_Optlink*)(volatile unsigned*,unsigned))InterlockedOrCode)(&(x),(n))
  #define InterlockedXor(x,n) (*(char(_Optlink*)(volatile unsigned*,unsigned))InterlockedXorCode)(&(x),(n))
  #define InterlockedBts(x,n) (*(char(_Optlink*)(volatile unsigned*,unsigned))InterlockedBtsCode)(&(x),(n))
  #define InterlockedBtr(x,n) (*(char(_Optlink*)(volatile unsigned*,unsigned))InterlockedBtrCode)(&(x),(n))
  #define InterlockedBtc(x,n) (*(char(_Optlink*)(volatile unsigned*,unsigned))InterlockedBtcCode)(&(x),(n))
#else
  #error Unsupported compiler.
#endif

#endif