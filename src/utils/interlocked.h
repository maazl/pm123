/*
 * Copyright 2007-2009 M.Mueller
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

#if 0 // Signatures are only logical
// Exchange DWORD. Replaces *dst by src and return the old value of *dst.
unsigned InterlockedXch(volatile unsigned* dst, unsigned src);
// Intrelocked compare exchange. Replace *dst by new if *dst == old and
// return the old value.
unsigned InterlockedCxc(volatile unsigned* dst, unsigned old, unsigned new);

// Atomic increment of *dst.
void     InterlockedInc(volatile unsigned* dst);
// Atomic decrement of *dst. Return true if the result is non-zero.
bool     InterlockedDec(volatile unsigned* dst);

// Add src to *dst atomically.
void     InterlockedAdd(volatile unsigned* dst, unsigned src);
// Subtract src from *dst atomically. Return true if the result is non-zero.
bool     InterlockedSub(volatile unsigned* dst, unsigned src);
// Add src to *dst and return the old value of *dst atomically.
unsigned InterlockedXad(volatile unsigned* dst, unsigned src);

// *dst &= src. Return true if the result is non-zero.
bool     InterlockedAdd(volatile unsigned* dst, unsigned src);
// *dst |= src.
void     InterlockedOr (volatile unsigned* dst, unsigned src);
// *dst ^= src. Return true if the result is non-zero.
bool     InterlockedXor(volatile unsigned* dst, unsigned src);

// Set bit number bit in *dst. Return true if the bit was set before.
// bit should be in the range [0..31].
bool     InterlockedBts(volatile unsigned* dst, unsigned src);
// Reset bit number bit in *dst. Return true if the bit was set before.
// bit should be in the range [0..31].
bool     InterlockedBtr(volatile unsigned* dst, unsigned src);
// Swap bit number bit in *dst. Return true if the bit was set before.
// bit should be in the range [0..31].
bool     InterlockedBtc(volatile unsigned* dst, unsigned src);

// Sets the bits of src in *dst and return the bits that are really set.
// This is the atomic equivalent to
//   return ~*dst & (*dst |= src);
unsigned InterlockedSet(volatile unsigned* dst, unsigned src);
// Resets the bits of src in *dst and return the bits that are really reset.
// This is the atomic equivalent to
//   return *dst & ~(*dst &= ~src);
unsigned InterlockedRst(volatile unsigned* dst, unsigned src);

#endif


#if defined(__GNUC__)
  static __inline__ unsigned InterlockedXch(__volatile__ unsigned *pu, unsigned u)
  { __asm__ __volatile__("xchgl %0, %1"
                         : "+m" (*pu)
                         , "+r" (u));
    return u;
  }
  static __inline__ unsigned InterlockedCxc(__volatile__ unsigned *pu, unsigned uOld, unsigned uNew)
  { __asm__ __volatile__("lock; cmpxchgl %2, %0"
                         : "+m" (*pu)
                         , "+a" (uOld)
                         : "r"  (uNew)
                         : "cc");
    return uOld;
  }
  static __inline__ void InterlockedInc(__volatile__ unsigned *pu)
  { __asm__ __volatile__("lock; incl %0"
                         : "+m" (*pu)
                         :
                         : "cc");
  }
  static __inline__ bool InterlockedDec(__volatile__ unsigned *pu)
  { bool bRet;
    __asm__ __volatile__("lock; decl %0\n\t"
                         "setnz %1"
                         : "+m"  (*pu)
                         , "=mq" (bRet)
                         :
                         : "cc");
    return bRet;
  }
  static __inline__ void InterlockedAdd(__volatile__ unsigned *pu, const unsigned uAdd)
  { __asm__ __volatile__("lock; addl %1, %0"
                         : "+m" (*pu)
                         : "nr" (uAdd)
                         : "cc");
  }
  static __inline__ bool InterlockedSub(__volatile__ unsigned *pu, const unsigned uSub)
  { bool bRet;
    __asm__ __volatile__("lock; subl %2, %0\n\t"
                         "setnz %1"
                         : "+m"  (*pu)
                         , "=mq" (bRet)
                         : "nr"  (uSub)
                         : "cc");
    return bRet;
  }
  static __inline__ unsigned InterlockedXad(__volatile__ unsigned *pu, unsigned uAdd)
  { __asm__ __volatile__("lock; xaddl %1, %0"
                         : "+m" (*pu)
                         , "+r" (uAdd)
                         :
                         : "cc");
    return uAdd;
  }
  static __inline__ bool InterlockedAnd(__volatile__ unsigned *pu, const unsigned uAnd)
  { bool bRet;
    __asm__ __volatile__("lock; andl %2, %0\n\t"
                         "setnz %1"
                         : "+m"  (*pu)
                         , "=mq" (bRet)
                         : "nr"  (uAnd)
                         : "cc");
    return bRet;
  }
  static __inline__ void InterlockedOr(__volatile__ unsigned *pu, const unsigned uOr)
  { __asm__ __volatile__("lock; orl %1, %0"
                         : "+m"  (*pu)
                         : "nr"  (uOr)
                         : "cc");
  }
  static __inline__ bool InterlockedXor(__volatile__ unsigned *pu, const unsigned uXor)
  { bool bRet;
    __asm__ __volatile__("lock; xorl %2, %0\n\t"
                         "setnz %1"
                         : "+m"  (*pu)
                         , "=mq" (bRet)
                         : "nr"  (uXor)
                         : "cc");
    return bRet;
  }
  static __inline__ bool InterlockedBts(__volatile__ unsigned *pv, const unsigned uBit)
  { bool bRet;
    __asm__ __volatile__("lock; btsl %2, %1\n\t"
                         "setc %0"
                         : "=mq" (bRet)
                         , "+m"  (*pv)
                         : "Ir"  (uBit)
                         : "cc");
    return bRet;
  }
  static __inline__ bool InterlockedBtr(__volatile__ unsigned *pv, const unsigned uBit)
  { bool bRet;
    __asm__ __volatile__("lock; btrl %2, %1\n\t"
                         "setc %0"
                         : "=mq" (bRet),
                           "+m" (*pv)
                         : "Ir" (uBit)
                         : "cc");
    return bRet;
  }
  static __inline__ bool InterlockedBtc(__volatile__ unsigned *pv, const unsigned uBit)
  { bool bRet;
    __asm__ __volatile__("lock; btcl %2, %1\n\t"
                         "setc %0"
                         : "=mq" (bRet),
                           "+m" (*pv)
                         : "Ir" (uBit)
                         : "cc");
    return bRet;
  }
  extern const unsigned char InterlockedSetCode[];
  extern const unsigned char InterlockedRstCode[];
  #define REGCALL __attribute__((regparm(2)))
  #define InterlockedSet(x,n) (*(unsigned REGCALL(*)(volatile unsigned*,unsigned))InterlockedSetCode)((x),(n))
  #define InterlockedRst(x,n) (*(unsigned REGCALL(*)(volatile unsigned*,unsigned))InterlockedRstCode)((x),(n))
#elif defined(__WATCOMC__)
  // TODO: Watcom C provides a much more efficient inline assembly way.
  extern const unsigned char InterlockedXchCode[];
  extern const unsigned char InterlockedCxcCode[];
  extern const unsigned char InterlockedIncCode[];
  extern const unsigned char InterlockedDecCode[];
  extern const unsigned char InterlockedAddCode[];
  extern const unsigned char InterlockedSubCode[];
  extern const unsigned char InterlockedXadCode[];
  extern const unsigned char InterlockedAndCode[];
  extern const unsigned char InterlockedOrCode [];
  extern const unsigned char InterlockedXorCode[];
  extern const unsigned char InterlockedBtsCode[];
  extern const unsigned char InterlockedBtrCode[];
  extern const unsigned char InterlockedBtcCode[];
  extern const unsigned char InterlockedSetCode[];
  extern const unsigned char InterlockedRstCode[];
  #define InterlockedXch(x,n) (*(unsigned(*)(volatile unsigned*,unsigned))InterlockedXchCode)((x),(n))
  #define InterlockedCxc(x,n,c)(*(unsigned(*)(volatile unsigned*,unsigned,unsigned))InterlockedCxcCode)((x),(n),(c))
  #define InterlockedInc(x)   (*(void    (*)(volatile unsigned*))InterlockedIncCode)((x))
  #define InterlockedDec(x)   (*(char    (*)(volatile unsigned*))InterlockedDecCode)((x))
  #define InterlockedAdd(x,n) (*(void    (*)(volatile unsigned*,unsigned))InterlockedAddCode)((x),(n))
  #define InterlockedSub(x,n) (*(char    (*)(volatile unsigned*,unsigned))InterlockedSubCode)((x),(n))
  #define InterlockedXad(x,n) (*(unsigned(*)(volatile unsigned*,unsigned))InterlockedXadCode)((x),(n))
  #define InterlockedAnd(x,n) (*(char    (*)(volatile unsigned*,unsigned))InterlockedAndCode)((x),(n))
  #define InterlockedOr(x,n)  (*(void    (*)(volatile unsigned*,unsigned))InterlockedOrCode) ((x),(n))
  #define InterlockedXor(x,n) (*(char    (*)(volatile unsigned*,unsigned))InterlockedXorCode)((x),(n))
  #define InterlockedBts(x,n) (*(char    (*)(volatile unsigned*,unsigned))InterlockedBtsCode)((x),(n))
  #define InterlockedBtr(x,n) (*(char    (*)(volatile unsigned*,unsigned))InterlockedBtrCode)((x),(n))
  #define InterlockedBtc(x,n) (*(char    (*)(volatile unsigned*,unsigned))InterlockedBtcCode)((x),(n))
  #define InterlockedSet(x,n) (*(unsigned(*)(volatile unsigned*,unsigned))InterlockedSetCode)((x),(n))
  #define InterlockedRst(x,n) (*(unsigned(*)(volatile unsigned*,unsigned))InterlockedRstCode)((x),(n))
#elif defined(__IBMC__) || defined(__IBMCPP__)
  extern const unsigned char InterlockedXchCode[];
  extern const unsigned char InterlockedCxcCode[];
  extern const unsigned char InterlockedIncCode[];
  extern const unsigned char InterlockedDecCode[];
  extern const unsigned char InterlockedAddCode[];
  extern const unsigned char InterlockedSubCode[];
  extern const unsigned char InterlockedXadCode[];
  extern const unsigned char InterlockedAndCode[];
  extern const unsigned char InterlockedOrCode [];
  extern const unsigned char InterlockedXorCode[];
  extern const unsigned char InterlockedBtsCode[];
  extern const unsigned char InterlockedBtrCode[];
  extern const unsigned char InterlockedBtcCode[];
  extern const unsigned char InterlockedSetCode[];
  extern const unsigned char InterlockedRstCode[];
  #define InterlockedXch(x,n) (*(unsigned(_Optlink*)(volatile unsigned*,unsigned))InterlockedXchCode)((x),(n))
  #define InterlockedCxc(x,n,c)(*(unsigned(_Optlink*)(volatile unsigned*,unsigned,unsigned))InterlockedCxcCode)((x),(n),(c))
  #define InterlockedInc(x)   (*(void    (_Optlink*)(volatile unsigned*))InterlockedIncCode)((x))
  #define InterlockedDec(x)   (*(char    (_Optlink*)(volatile unsigned*))InterlockedDecCode)((x))
  #define InterlockedAdd(x,n) (*(void    (_Optlink*)(volatile unsigned*,unsigned))InterlockedAddCode)((x),(n))
  #define InterlockedSub(x,n) (*(char    (_Optlink*)(volatile unsigned*,unsigned))InterlockedSubCode)((x),(n))
  #define InterlockedXad(x,n) (*(unsigned(_Optlink*)(volatile unsigned*,unsigned))InterlockedXadCode)((x),(n))
  #define InterlockedAnd(x,n) (*(char    (_Optlink*)(volatile unsigned*,unsigned))InterlockedAndCode)((x),(n))
  #define InterlockedOr(x,n)  (*(void    (_Optlink*)(volatile unsigned*,unsigned))InterlockedOrCode) ((x),(n))
  #define InterlockedXor(x,n) (*(char    (_Optlink*)(volatile unsigned*,unsigned))InterlockedXorCode)((x),(n))
  #define InterlockedBts(x,n) (*(char    (_Optlink*)(volatile unsigned*,unsigned))InterlockedBtsCode)((x),(n))
  #define InterlockedBtr(x,n) (*(char    (_Optlink*)(volatile unsigned*,unsigned))InterlockedBtrCode)((x),(n))
  #define InterlockedBtc(x,n) (*(char    (_Optlink*)(volatile unsigned*,unsigned))InterlockedBtcCode)((x),(n))
  #define InterlockedSet(x,n) (*(unsigned(_Optlink*)(volatile unsigned*,unsigned))InterlockedSetCode)((x),(n))
  #define InterlockedRst(x,n) (*(unsigned(_Optlink*)(volatile unsigned*,unsigned))InterlockedRstCode)((x),(n))
#else
  #error Unsupported compiler.
#endif

#endif
