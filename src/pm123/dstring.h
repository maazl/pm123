/*
 * Copyright 2009-2010 M.Mueller
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

#ifndef  DSTRING_H
#define  DSTRING_H

#include <config.h>

#include <format.h>
#include <cpp/xstring.h>

#include <stdarg.h>


/****************************************************************************
*
*  Public DSTRING API functions.
*
****************************************************************************/

const char* DLLENTRY dstring_create( const char* cstr );

void DLLENTRY dstring_free( volatile DSTRING* dst );

unsigned DLLENTRY dstring_length( const DSTRING* src );

char DLLENTRY dstring_equal( const DSTRING* src1, const DSTRING* src2 );

int DLLENTRY dstring_compare( const DSTRING* src1, const DSTRING* src2 );

void DLLENTRY dstring_copy( volatile DSTRING* dst, const DSTRING* src );

void DLLENTRY dstring_copy_safe( volatile DSTRING* dst, volatile const DSTRING* src );

void DLLENTRY dstring_assign( volatile DSTRING* dst, const char* cstr );

char DLLENTRY dstring_cmpassign( DSTRING* dst, const char* cstr );

void DLLENTRY dstring_append( DSTRING* dst, const char* cstr );

char* DLLENTRY dstring_allocate( DSTRING* dst, unsigned int len );

void DLLENTRY dstring_sprintf( volatile DSTRING* dst, const char* fmt, ... );

void DLLENTRY dstring_vsprintf( volatile DSTRING* dst, const char* fmt, va_list va );


#endif

