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

#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <config.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Because the result string always less or is equal to a location
// string all functions can safely use the same storage area for a
// location and result.

char* sdrive   ( char* result, const char* location, size_t size );
int   strack   ( const char* location );
/* Returns the scheme followed by a colon (:) of the specified location. */
char* scheme   ( char* result, const char* location, size_t size );
/* Returns the base file name without any extensions. */
char* sfname   ( char* result, const char* location, size_t size );
/* Returns the base file name without any extensions decoded from URL transmission. */
char* sdname   ( char* result, const char* location, size_t size );
/* Returns the file name extension, if any, including the leading period (.). */
char* sfext    ( char* result, const char* location, size_t size );
/* Returns the base file name with file extension. */
char* sfnameext( char* result, const char* location, size_t size );
/* Returns the base file name with file extension decoded from URL transmission. */
char* sdnameext( char* result, const char* location, size_t size );
/* Returns the drive letter or scheme and the path of
   subdirectories, if any, including the trailing slash.
   Slashes (/), backslashes (\), or both may be present
   in location. */
char* sdrivedir( char* result, const char* location, size_t size );
char* sdecode  ( char* result, const char* location, size_t size );
/* Normalize an URL: enforce scheme, convert slashes */
char* snormal  ( char* result, const char* location, size_t size );
/* Fast in place version of sfname aliasing, cannot deal with ? parameters. */
const char* sfnameext2( const char* file );
/* Strip file:// from file URLs */
const char* surl2file( const char* location );

/** rel2abs: convert a relative path name into absolute.
 * @param path    relative path
 * @param base    base directory (must be absolute path)
 * @param result  result buffer
 * @param size    size of result buffer
 * @return != NULL: absolute path
 *         == NULL: error
 */
char* rel2abs( const char* base, const char* path, char* result, size_t size );

/** abs2rel: convert an absolute path name into relative.
 * @param base    base directory (must be absolute path)
 * @param path    absolute path
 * @param result  result buffer
 * @param size    size of result buffer
 * @return != NULL: relative path
 *         == NULL: error
 */
char* abs2rel( const char* base, const char* path, char* result, size_t size );

typedef struct
{ char drive[4]; // we need only 3 but we don't want to run into alignment trouble
  int  track;
  int  sectors[2];
} CDDA_REGION_INFO;

CDDA_REGION_INFO* scdparams( CDDA_REGION_INFO* result, const char* location );

unsigned char is_cdda ( const char* location );
unsigned char is_track( const char* location );
unsigned char is_file ( const char* location );
unsigned char is_url  ( const char* location );
unsigned char is_root ( const char* location );
unsigned char is_dir  ( const char* location );



#ifdef __cplusplus
}
#endif

#endif /* FILEUTIL_H */
