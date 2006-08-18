/*
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef __FILEUTIL_H
#define __FILEUTIL_H

#if __cplusplus
extern "C" {
#endif

// Because the result string always less or is equal to a location
// string all functions can safely use the same storage area for a
// location and result.

char* sdrive   ( char* result, const char* location, size_t size );
char* scheme   ( char* result, const char* location, size_t size );
char* sfname   ( char* result, const char* location, size_t size );
char* sfext    ( char* result, const char* location, size_t size );
char* sfnameext( char* result, const char* location, size_t size );
char* sdrivedir( char* result, const char* location, size_t size );
char* sdecode  ( char* result, const char* location, size_t size );

typedef struct
{ char drive[4]; // we need only 3 but we don't want to run into alignment trouble
  int  track;
  int  sectors[2];
} CDDA_REGION_INFO;

CDDA_REGION_INFO* scdparams( CDDA_REGION_INFO* result, const char* location );

BOOL is_http ( const char* location );
BOOL is_cdda ( const char* location );
BOOL is_track( const char* location );
BOOL is_file ( const char* location );
BOOL is_url  ( const char* location );
BOOL is_root ( const char* location );
BOOL is_dir  ( const char* location );

#if __cplusplus
}
#endif

#endif /* __FILEUTIL_H */
