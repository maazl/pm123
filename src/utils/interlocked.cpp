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


#if defined(__WATCOMC__)
/* Watcom uses EAX EBX for arguments */

const unsigned char InterlockedXchCode[] = 
{ 0x87, 0x18,             // xchg [eax], ebx
  0x89, 0xd8,             // mov eax, ebx
  0xC3                    // ret 
};
const unsigned char InterlockedIncCode[] = 
{ 0xF0, 0xFF, 0x00,       // lock inc dword [eax]
  0xC3                    // ret
};
const unsigned char InterlockedDecCode[] = 
{ 0xF0, 0xFF, 0x08,       // lock dec dword [eax]
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret
};
const unsigned char InterlockedAddCode[] = 
{ 0xF0, 0x01, 0x18,       // lock add [eax], ebx
  0xC3                    // ret 
};
const unsigned char InterlockedSubCode[] = 
{ 0xF0, 0x29, 0x18,       // lock sub [eax], ebx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedAndCode[] = 
{ 0xF0, 0x21, 0x18,       // lock and [eax], ebx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedOrCode[] = 
{ 0xF0, 0x09, 0x18,       // lock or [eax], ebx
  0xC3                    // ret 
};
const unsigned char InterlockedXorCode[] = 
{ 0xF0, 0x31, 0x18,       // lock xor [eax], ebx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedBtsCode[] = 
{ 0xF0, 0x0F, 0xAB, 0x18, // lock bts [eax], ebx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtrCode[] = 
{ 0xF0, 0x0F, 0xB3, 0x18, // lock btr [eax], ebx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtcCode[] = 
{ 0xF0, 0x0F, 0xBB, 0x18, // lock btr [eax], ebx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};

#else
/* All others use EAX EDX for arguments */

const unsigned char InterlockedXchCode[] = 
{ 0x87, 0x10,             // xchg [eax], edx
  0x89, 0xd0,             // mov eax, edx 
  0xC3                    // ret 
};
const unsigned char InterlockedIncCode[] = 
{ 0xF0, 0xFF, 0x00,       // lock inc dword [eax]
  0xC3                    // ret
};
const unsigned char InterlockedDecCode[] = 
{ 0xF0, 0xFF, 0x08,       // lock dec dword [eax]
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret
};
const unsigned char InterlockedAddCode[] = 
{ 0xF0, 0x01, 0x10,       // lock add [eax], edx
  0xC3                    // ret 
};
const unsigned char InterlockedSubCode[] = 
{ 0xF0, 0x29, 0x10,       // lock sub [eax], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedAndCode[] = 
{ 0xF0, 0x21, 0x10,       // lock and [eax], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedOrCode[] = 
{ 0xF0, 0x09, 0x10,       // lock or [eax], edx
  0xC3                    // ret 
};
const unsigned char InterlockedXorCode[] = 
{ 0xF0, 0x31, 0x10,       // lock xor [eax], edx
  0x0F, 0x95, 0xC0,       // setne al
  0xC3                    // ret 
};
const unsigned char InterlockedBtsCode[] = 
{ 0xF0, 0x0F, 0xAB, 0x10, // lock bts [eax], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtrCode[] = 
{ 0xF0, 0x0F, 0xB3, 0x10, // lock btr [eax], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};
const unsigned char InterlockedBtcCode[] = 
{ 0xF0, 0x0F, 0xBB, 0x10, // lock btr [eax], edx
  0x0F, 0x92, 0xC0,       // setc al
  0xC3                    // ret 
};

#endif