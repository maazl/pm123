/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
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

#include "dstring.h"

#include <interlocked.h>
#include <cpp/xstring.h>

#include <string.h>


/****************************************************************************
*
*  Proxy functions to interface between xstring and DSTRING
*
****************************************************************************/

const char* DLLENTRY dstring_create( const char* cstr )
{ return xstring(cstr).toCstr();
}

void DLLENTRY dstring_free( volatile DSTRING* dst )
{ dst->reset();
}

unsigned DLLENTRY dstring_length( const DSTRING* dst )
{ return dst->length();
}

char DLLENTRY dstring_equal( const DSTRING* src1, const DSTRING* src2 )
{ return *src1 == *src2;
}

int DLLENTRY dstring_compare( const DSTRING* src1, const DSTRING* src2 )
{ return src1->compareTo(*src2);
}

void DLLENTRY dstring_copy( volatile DSTRING* dst, const DSTRING* src )
{ *dst = *src; 
}

void DLLENTRY dstring_copy_safe( volatile DSTRING* dst, volatile const DSTRING* src )
{ *dst = *src;
}

void DLLENTRY dstring_assign( volatile DSTRING* dst, const char* cstr )
{ *dst = cstr;
}

char dstring_cmpassign( DSTRING* dst, const char* cstr )
{ if (*dst == cstr)
    return false;
  *dst = cstr;
  return true;
}

void DLLENTRY dstring_append( DSTRING* dst, const char* cstr )
{ if (*dst == NULL)
    *dst = cstr;
  else
  { size_t len = strlen(cstr);
    xstring ret;
    char* dp = ret.allocate(dst->length() + len);
    memcpy(dp, dst->cdata(), dst->length());
    memcpy(dp+dst->length(), cstr, len);
    dst->swap(ret);
  }
}

char* DLLENTRY dstring_allocate( DSTRING* dst, unsigned int len )
{ return dst->allocate(len);
}

void DLLENTRY dstring_sprintf( volatile DSTRING* dst, const char* fmt, ... )
{ va_list va;
  va_start(va, fmt);
  dstring_vsprintf( dst, fmt, va );
  va_end(va);
}

void DLLENTRY dstring_vsprintf( volatile DSTRING* dst, const char* fmt, va_list va )
{ *dst = xstring::vsprintf(fmt, va);
}

