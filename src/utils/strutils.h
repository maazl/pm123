/*
 * Copyright (c) 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright (c) 2007-2008 Marcel Mueller
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

#ifndef STRLUTIL_H
#define STRLUTIL_H


#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Return the length of a string excluding \0 but at most siz.
 * The function does not read any data from beyond src + size -1
 * even if the string is unterminated.
 */
size_t strnlen( const char* src, size_t size );

/** Copy src to string dst of size size.  At most size-1 characters
 * will be copied.  Always NUL terminates (unless size == 0).
 * Returns strlen(src); if retval >= size, truncation occurred.
 */
size_t strlcpy( char* dst, const char* src, size_t size );

/** Appends src to string dst of size siz (unlike strncat, size is the
 * full size of dst, not space left).  At most size-1 characters
 * will be copied.  Always NUL terminates (unless size <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= size, truncation occurred.
 */
size_t strlcat( char* dst, const char* src, size_t size );

/** Search the first occurrence of c in the first \a size bytes of string str.
 */ 
char* strnchr( const char* str, char c, size_t size );

/** Search the last occurrence of c in the first \a size bytes of string str.
 */ 
char* strnrchr( const char* str, char c, size_t size );

/** Remove line terminator from the end of the string.
 * Works with \r, \n and \r\n.
 */
void strchomp( char* str );

/** Compare and copy memory
 * Moves len bytes from src to dst returning the index of the first byte that changed.
 * If there are no changes the function returns ~0 (i.e. all bits set).
 */
size_t memcmpcpy( void* dst, const void* src, size_t len );


/** Removes leading and trailing spaces. */
char* blank_strip( char* string );
/** Replaces series of control characters by one space. */
char* control_strip( char* string );
/** Removes leading and trailing spaces and quotes. */
char* quote_strip( char* string );
/** Removes comments starting with "#". */
char* uncomment  ( char* string );


#ifdef __cplusplus
}
#endif

#endif /* STRLUTIL_H */
