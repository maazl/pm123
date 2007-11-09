/* * Copyright 2006-2007 Marcel Mueller
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

/* Utility factories to compile easy function adaptors in the data segment.
 * They require a platform with a IA-32 compatible instruction set and a flat
 * memory model to operate. Compiler or operating sytem is not significant.
 *
 * - Virtual .NET like delegates from function pointer.
 * - Replacer to replace objects in of member-function like functions.
 *
 * These implementations are very performant and can be altered at run time.
 * The VDELEGATE pattern cannot be solved in any other, more portable way.
 *
 * Type safe access to these function is possible through a C++ wrapper.
 * This is not part of this file.
 */

#ifndef _VDELEGATE_H
#define _VDELEGATE_H

// for DLLENTRYP
#include <format.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int DLLENTRYP(V_FUNC)();

#define VDELEGATE_LEN 0x1B
/* YOU MUST NOT COPY OBJECTS OF THIS TYPE. They are not POD like. */
typedef unsigned char VDELEGATE[VDELEGATE_LEN];
/* Generate proxy function which prepends an additional parameter at each function call.
 * dg     VDELEGATE structure. The lifetime of this structure must exceed the lifetime
 *        of the returned function pointer.
 * func   function to call. The function must have count +1 arguments.
 * count  number of arguments of the returned function.
 * ptr    pointer to pass as first argument to func.
 * return generated function with count arguments.
 * Once created the returned function is thread-safe and reentrant as far as the called
 * function is. Updating an existing object is generally not thread-safe.
 */
V_FUNC mkvdelegate(VDELEGATE* dg, V_FUNC func, int count, void* ptr);

#define VREPLACE1_LEN 0x0E
/* YOU MUST NOT COPY OBJECTS OF THIS TYPE. They are not POD like. */
typedef unsigned char VREPLACE1[VREPLACE1_LEN];
/* Generate proxy function that replaces the first argument of the function call.
 * dg     VDELEGATE structure. The lifetime of this structure must exceed the lifetime
 *        of the returned function pointer.
 * func   function to call.
 * ptr    pointer to pass as first argument to func.
 * return generated function.
 * Once created the returned function is thread-safe and reentrant as far as the called
 * function is. Updating an existing object is generally not thread-safe.
 */
V_FUNC mkvreplace1(VREPLACE1* rp, V_FUNC func, void* ptr);

#ifdef __cplusplus
}
/* Typesafe C++ wrappers */

#define PARSIZE(type) ((sizeof(type)+sizeof(int)-1)&-sizeof(int))
template <class R, class P>
R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*), P* ptr))()
{ return (R DLLENTRYP()())mkvdelegate(dg, (V_FUNC)func, 0, ptr);
}
template <class R, class P, class P1>
R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1), P* ptr))(P1)
{ return (R DLLENTRYP()(P1))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1), ptr);
}
template <class R, class P, class P1, class P2>
R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2), P* ptr))(P1,P2)
{ return (R DLLENTRYP()(P1,P2))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2), ptr);
}
template <class R, class P, class P1, class P2, class P3>
R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2,P3), P* ptr))(P1,P2,P3)
{ return (R DLLENTRYP()(P1,P2,P3))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2)+PARSIZE(P3), ptr);
}
template <class R, class P, class P1, class P2, class P3, class P4>
R DLLENTRYP2(vdelegate(VDELEGATE* dg, R DLLENTRYP(func)(P*,P1,P2,P3,P4), P* ptr))(P1,P2,P3,P4)
{ return (R DLLENTRYP()(P1,P2,P3,P4))mkvdelegate(dg, (V_FUNC)func, PARSIZE(P1)+PARSIZE(P2)+PARSIZE(P3)+PARSIZE(P4), ptr);
}
#undef PARSIZE

template <class R, class P>
R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*), P* ptr))(P*)
{ return (R DLLENTRYP()(P*))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2>
R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2), P* ptr))(P*,P2)
{ return (R DLLENTRYP()(P*,P2))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2, class P3>
R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2,P3), P* ptr))(P*,P2,P3)
{ return (R DLLENTRYP()(P*,P2,P3))mkvreplace1(rp, (V_FUNC)func, ptr);
}
template <class R, class P, class P2, class P3, class P4>
R DLLENTRYP2(vreplace1(VREPLACE1* rp, R DLLENTRYP(func)(P*,P2,P3,P4), P* ptr))(P*,P2,P3,P4)
{ return (R DLLENTRYP()(P*,P2,P3,P4))mkvreplace1(rp, (V_FUNC)func, ptr);
}
#endif
#endif

