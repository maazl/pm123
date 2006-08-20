/*
 * Copyright 2006-2006 Marcel Mueller
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

#include <string.h>
#include "vdelegate.h"

static const VDELEGATE vdtempl =
{ 0x31, 0xc9,                         /* xor ecx, ecx              */
  0xb1, 0x00,                         /* mov ecx, 0                */
  0xff, 0x74, 0x24, 0x00,             /* push dword [esp+0x00]     */
  0xe2, 0xfa,                         /* loop $-4                  */
  0x68, 0x00, 0x00, 0x00, 0x00,       /* push dword 0x0            */
  0xE8, 0x06, 0x00, 0x00, 0x00,       /* call $+11                 */
  0x81, 0xC4, 0x04, 0x00, 0x00, 0x00, /* add esp, 0x04             */
  0xC3                                /* ret                       */
};
#define DELEGATE_OFF_COUNT1 0x03
#define DELEGATE_OFF_COUNT2 0x07
#define DELEGATE_OFF_NOPAR  0x0a
#define DELEGATE_OFF_PTR    0x0b
#define DELEGATE_OFF_FUNC   0x10
#define DELEGATE_OFF_REF    0x14
#define DELEGATE_OFF_COUNT3 0x16

V_FUNC mkvdelegate(VDELEGATE* dg, V_FUNC func, int count, void* ptr)
{ memcpy(dg, &vdtempl, sizeof *dg);
  (*dg)[DELEGATE_OFF_COUNT3] += (*dg)[DELEGATE_OFF_COUNT2] = ((*dg)[DELEGATE_OFF_COUNT1] = count) << 2;
  *(void**)(*dg + DELEGATE_OFF_PTR) = ptr;
  *(int*)(*dg + DELEGATE_OFF_FUNC) = (char*)func - (*dg + DELEGATE_OFF_REF);
  return count ? (V_FUNC)dg : (V_FUNC)(*dg + DELEGATE_OFF_NOPAR);
}

static const VREPLACE1 vrtempl =
{ 0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, /* mov dword [esp], 0 */
  0xE9, 0x00, 0x00, 0x00, 0x00,                   /* jump $+0           */
  0xC3                                            /* ret                */
};
#define VREPLACE1_OFF_PTR   0x04
#define VREPLACE1_OFF_FUNC  0x09
#define VREPLACE1_OFF_REF   0x0d

V_FUNC mkvreplace1(VREPLACE1* rp, V_FUNC func, void* ptr)
{ memcpy(rp, &vrtempl, sizeof *rp);
  *(void**)(*rp + VREPLACE1_OFF_PTR) = ptr;
  *(int*)(*rp + VREPLACE1_OFF_FUNC) = (char*)func - (*rp + VREPLACE1_OFF_REF);
  return (V_FUNC)rp;
}
