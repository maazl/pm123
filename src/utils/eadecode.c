/*
 * Copyright 2008 Dmitry A.Steklenev
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

#define  INCL_DOS
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include "strutils.h"
#include "debuglog.h"


static BOOL appendeatype(char* dst, size_t dstlen, const char* src, size_t len)
{ size_t curlen = strnlen(dst, dstlen);
  if (!!curlen + curlen + len +1 >= dstlen)
  { DEBUGLOG(("appendeatype: truncated EA %s\n", src));
    return FALSE;
  }
  if (curlen)
    dst[curlen++] = '\t' ; // delimiter
  memcpy(dst + curlen, src, len);
  dst[curlen + len] = 0;
  return TRUE;
}

BOOL eadecode( char* dst, size_t len, USHORT type, const USHORT** eadata )
{ DEBUGLOG(("eadecode(, %04x, %04x...)\n", type, (*eadata)[0]));
  switch (type)
  {case EAT_ASCII:
    if (!appendeatype(dst, len, (const char*)(*eadata + 1), (*eadata)[0]))
      return FALSE;

   default:
    *(const char**)eadata += sizeof(USHORT) + (*eadata)[0];
    break;

   case EAT_MVST:
    { size_t count = (*eadata)[1];
      type = (*eadata)[2];
      *eadata += 3;
      while (count--)
        if (!eadecode(dst, len, type, eadata))
          return FALSE;
    }
    break;

   case EAT_MVMT:
    { size_t count = (*eadata)[1];
      *eadata += 2;
      while (count--)
      { type = *(*eadata)++;
        if (!eadecode(dst, len, type, eadata))
          return FALSE;
      }
    }
   case EAT_ASN1: // not supported
    break;
  }
  return TRUE;
}
