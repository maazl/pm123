/*
 * Copyright 2004 Dmitry A.Steklenev
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

#include "fileutil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define  C_DIR_SEPARATOR '\\'
#define  C_URL_SEPARATOR '/'

#if C_DIR_SEPARATOR == C_URL_SEPARATOR
  #define isslash( c ) ( c == C_DIR_SEPARATOR )
#else
  static int
  isslash( char c ) {
    return c == C_DIR_SEPARATOR ||
           c == C_URL_SEPARATOR;
  }
#endif

/*
 * rel2abs: convert a relative path name into absolute.
 *
 *    path    relative path
 *    base    base directory (must be absolute path)
 *    result  result buffer
 *    size    size of result buffer
 *
 *    return  != NULL: absolute path
 *            == NULL: error
 */

char*
rel2abs( const char* base, const char* path, char* result, size_t size )
{
  size_t bsize = strlen( base );
  size_t psize = strlen( path );
  char*  rroot = result-1;
  char*  rtail;
  size_t rsize;
  char*  pc;
  char   slash = C_DIR_SEPARATOR;

  if( bsize >= size ) {
    return NULL;
  }

  // Search a scheme name or a drive name in the relative path.
  // If the parse string contain a colon after the 1st character
  // and before any characters not allowed as part of a scheme
  // name (i.e. any not alphanumeric, '+', '.' or '-'), the scheme
  // of the url is the substring of chars up to but not including
  // the first colon.
  for( pc = (char*)path; isalnum( *pc ) || ( *pc == '+' ) || ( *pc == '-' ) || ( *pc == '.' ); pc++ )
  {}

  // The relative path should not begin with a name of a disk drive,
  // a scheme name or a name of a computer.
  if( *pc == ':' || ( psize > 1 && isslash( path[0] ) && isslash( path[1] )))
  {
    if( psize >= size ) {
      return NULL;
    }

    strcpy( result, path );
    return result;
  }

  strcpy( result, base );
  rsize = bsize;
  rtail = result + strlen( base );

  if( rsize > 1 && isslash( result[0] ) && isslash( result[1] ))
  {
    // Have UNC path. It is necessary to find the end of
    // the computer name.
    rroot = result + 3;
    while( rroot && !isslash( *rroot )) {
      ++rroot;
    }
  } else {
    // Search a scheme name or a drive name in the base path.
    for( pc = result; isalnum( *pc ) || ( *pc == '+' ) || ( *pc == '-' ) || ( *pc == '.' ); pc++ )
    {}

    if( *pc == ':' )
    {
      rroot = pc;
      // If the scheme consists more than of one symbol or its first
      // symbol not the alphabetic character that it is exact not a
      // name of the drive.
      if( pc - result > 2 || !isalpha( *result )) {
        slash = C_URL_SEPARATOR;
        // Skip location (user:password@host:port part of the url
        if( isslash( rroot[1] ) && isslash( rroot[2] )) {
          rroot += 3;
          while( *rroot && !isslash( *rroot )) {
            ++rroot;
          }
        }
      } else {
        // Have a name of a disk drive.
        if( rsize > 2 && isslash( result[2] )) {
          rroot = result + 2;
        } else {
          rroot = result + 1;
        }
      }
    }
  }

  if( rsize && rtail-1 != rroot && isslash( rtail[-1] )) {
    // Remove a last path separator if have it.
    *--rtail = 0;
     --rsize;
  }

  if( psize == 0 ) {
    // The relative path is empty.
    return result;
  }

  if( isslash( *path )) {
    // If the relative path begins from a root.
    if( !rsize ) {
      *++rroot = *path;
       ++rsize;
    }
    ++path;
    rtail = rroot + 1;
  }

  while( *path && rsize < size )
  {
    // Found link to current directory.
    if( path[0] == '.' && ( isslash( path[1] ) || !path[1] )) {
      path += 1;
      while( isslash( *path )) {
        ++path;
      }
      continue;
    }

    // Found link to parent directory.
    if( path[0] == '.' && path[1] == '.' && ( isslash( path[2] ) || !path[2] ))
    {
      path += 2;
      while( isslash( *path )) {
        ++path;
      }
      while( rtail != rroot && !isslash( *--rtail )) {
        --rsize;
      }
      continue;
    }

    // Found the fragment of the relative path.
    if( rtail != result && !isslash( rtail[-1] )) {
      *rtail++ = slash;
       rsize++;
    }
    while( !isslash( *path ) && *path && rsize < size ) {
      *rtail++ = *path++;
       rsize++;
    }
    while( isslash( *path ) && *path ) {
      ++path;
    }
  }

  if( rsize >= size ) {
    return NULL;
  }

  *rtail = 0;
  return result;
}

#if 0

#include <direct.h>

int
main( int argc, char* argv[] )
{
  char result[_MAX_PATH];
  char cwd[_MAX_PATH];

  if( argc < 2 ) {
    fprintf( stderr, "usage: rel2abs path [base]\n" );
    return 1;
  }
  if( argc == 2 ) {
    if( !getcwd( cwd, _MAX_PATH )) {
      fprintf( stderr, "cannot get current directory.\n" );
      return 1;
    }
  } else
    strcpy( cwd, argv[2] );

  if( rel2abs( cwd, argv[1], result, _MAX_PATH )) {
    printf( "%s\n", result );
  } else
    printf( "ERROR\n" );

  return 0;
}

#endif
