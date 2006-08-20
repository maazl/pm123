/* * Copyright 2006-2006 Marcel Mueller
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

/* virtual .NET like delegates from function pointer */

#ifndef _VDELEGATE_H
#define _VDELEGATE_H

#include <format.h>
/*#define PM123_ENTRYP *_System*/

#if __cplusplus
extern "C" {
#endif

typedef int (PM123_ENTRYP V_FUNC)();

#define VDELEGATE_LEN 0x1B
/* YOU MUST NOT COPY OBJECTS OF THIS TYPE. They are not POD. */
typedef unsigned char VDELEGATE[VDELEGATE_LEN];
/* Generate proxy function which prepends an additional parameter at each function call.
 * dg     VDELEGATE structure. The lifetime of this structure must exceed the lifetime of the returned function pointer.
 * func   function to call. The function must have count +1 arguments.
 * count  number of arguments of the returned function.
 * ptr    pointer to pass as first argument to func.
 * return generated function with count arguments.
 */  
V_FUNC mkvdelegate(VDELEGATE* dg, V_FUNC func, int count, void* ptr);

#define VREPLACE1_LEN 0x0E
/* YOU MUST NOT COPY OBJECTS OF THIS TYPE. They are not POD. */
typedef unsigned char VREPLACE1[VREPLACE1_LEN];
/* Generate proxy function that replaces the first argument of the function call.
 * dg     VDELEGATE structure. The lifetime of this structure must exceed the lifetime of the returned function pointer.
 * func   function to call.
 * ptr    pointer to pass as first argument to func.
 * return generated function.
 */  
V_FUNC mkvreplace1(VREPLACE1* rp, V_FUNC func, void* ptr);

#if __cplusplus
}
#endif
#endif

