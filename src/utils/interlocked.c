/*
 * Copyright 2002-2008 Marcel Mueller
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

#include "interlocked.h"

#if defined(__IBMC__)
const unsigned char InterlockedXchCode[] =
{ 0x87, 0x10              // xchg [eax], edx
, 0x89, 0xd0              // mov eax, edx
, 0xC3                    // ret
};

const unsigned char InterlockedCxcCode[] =
{ 0x91                    // xchg eax, ecx
, 0xF0, 0x0F, 0xB1, 0x11  // lock cmpxchg [ecx], edx
, 0xC3                    // ret
};

const unsigned char InterlockedIncCode[] =
{ 0xF0, 0xFF, 0x00        // lock inc dword [eax]
, 0xC3                    // ret
};

const unsigned char InterlockedDecCode[] =
{ 0xF0, 0xFF, 0x08        // lock dec dword [eax]
, 0x0F, 0x95, 0xC0        // setnz al
, 0xC3                    // ret
};

const unsigned char InterlockedAddCode[] =
{ 0xF0, 0x01, 0x10        // lock add [eax], edx
, 0xC3                    // ret
};

const unsigned char InterlockedSubCode[] =
{ 0xF0, 0x29, 0x10        // lock sub [eax], edx
, 0x0F, 0x95, 0xC0        // setnz al
, 0xC3                    // ret
};

const unsigned char InterlockedXadCode[] =
{ 0xF0, 0x0F, 0xC1, 0x10  // lock xadd [eax], edx
, 0x89, 0xD0              // mov eax, edx
, 0xC3                    // ret
};

const unsigned char InterlockedAndCode[] =
{ 0xF0, 0x21, 0x10        // lock and [eax], edx
, 0x0F, 0x95, 0xC0        // setnz al
, 0xC3                    // ret
};

const unsigned char InterlockedOrCode[] =
{ 0xF0, 0x09, 0x10        // lock or [eax], edx
, 0xC3                    // ret
};

const unsigned char InterlockedXorCode[] =
{ 0xF0, 0x31, 0x10        // lock xor [eax], edx
, 0x0F, 0x95, 0xC0        // setnz al
, 0xC3                    // ret
};

const unsigned char InterlockedBtsCode[] =
{ 0xF0, 0x0F, 0xAB, 0x10  // lock bts [eax], edx
, 0x0F, 0x92, 0xC0        // setc al
, 0xC3                    // ret
};

const unsigned char InterlockedBtrCode[] =
{ 0xF0, 0x0F, 0xB3, 0x10  // lock btr [eax], edx
, 0x0F, 0x92, 0xC0        // setc al
, 0xC3                    // ret
};

const unsigned char InterlockedBtcCode[] =
{ 0xF0, 0x0F, 0xBB, 0x10  // lock btr [eax], edx
, 0x0F, 0x92, 0xC0        // setc al
, 0xC3                    // ret
};
#endif

const unsigned char InterlockedSetCode[] =
{ 0x53                    //    push ebx
, 0x51                    //    push ecx
, 0x89, 0xC3              //    mov  ebx, eax
, 0x8B, 0x00              //    mov  eax, [eax]
, 0x89, 0xD1              // l: mov  ecx, edx
, 0x09, 0xC1              //    or   ecx, eax
, 0xF0, 0x0F, 0xB1, 0x0B  //    cmpxchg [ebx], ecx
, 0x75, 0xF6              //    jnz  l
, 0xF7, 0xD0              //    not  eax
, 0x21, 0xD0              //    and  eax, edx
, 0x59                    //    pop  ecx
, 0x5B                    //    pop  ebx
, 0xC3                    //    ret
};

const unsigned char InterlockedRstCode[] =
{ 0x53                    //    push ebx
, 0x51                    //    push ecx
, 0xF7, 0xD2              //    not  edx
, 0x89, 0xC3              //    mov  ebx, eax
, 0x8B, 0x00              //    mov  eax, [eax]
, 0x89, 0xD1              // l: mov  ecx, edx
, 0x21, 0xC1              //    and  ecx, eax
, 0xF0, 0x0F, 0xB1, 0x0B  //    cmpxchg [ebx],ecx
, 0x75, 0xF6              //    jnz  l
, 0xF7, 0xD2              //    not  edx
, 0x21, 0xD0              //    and  eax, edx
, 0x59                    //    pop  ecx
, 0x5B                    //    pop  ebx
, 0xC3                    //    ret
};

