/*
 * Copyright 2005 Dmitry A.Steklenev
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

#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <string.h>

#include "abs2rel.h"

#define C_DIR_SEPARATOR '\\'
#define C_DEV_SEPARATOR ':'
#define C_DOT           '.'

/*
 * abs2rel: convert an absolute path name into relative.
 *
 *    base    base directory (must be absolute path)
 *    path    absolute path
 *    result  result buffer
 *    size    size of result buffer
 *
 *    return  != NULL: relative path
 *            == NULL: error
 */

char*
abs2rel( const char* base, const char* path, char* result, size_t size )
{
  const char*  pbase = base;
  const char*  ppath = path;
  const char*  bdiff = base;
  const char*  pdiff = path;
  size_t       psize = strlen( path );
  size_t       rsize = 0;
  char*        rp    = result;

  // We search for a common part of path. It is necessary to
  // remember, that the path as against the base directory
  // contains a filename.
  while( *ppath && ( *pbase == *ppath || ( *pbase == 0 && *ppath == C_DIR_SEPARATOR )))
  {
    if( *ppath == C_DIR_SEPARATOR ) {
      bdiff = *pbase ? pbase + 1 : pbase;
      pdiff =  ppath + 1;
    }

    if( *pbase == 0 ) {
      break;
    }

    ++pbase;
    ++ppath;
  }

  // If the common part of path is not found or the common part
  // is a prefix of a network name, it is necessary to return
  // original path.
  if(( pdiff == path ) ||
     ( pdiff -  path == 2 && pdiff[-1] == C_DIR_SEPARATOR && pdiff[-2] == C_DIR_SEPARATOR ))
  {
    if( psize >= size ) {
      return NULL;
    }

    strcpy( result, path );
    return result;
  }

  // Climb up to a common part of path.
  while( *bdiff )
  {
    if( size - rsize < 3 ) {
      return NULL;
    }

    *rp++ = C_DOT;
    *rp++ = C_DOT;
    rsize += 2;

    while( *bdiff && *++bdiff != C_DIR_SEPARATOR )
    {}

    if( *bdiff || *pdiff ) {
      *rp++ = C_DIR_SEPARATOR;
      if( ++rsize == size ) {
        return NULL;
      }
    }
    if( *bdiff == C_DIR_SEPARATOR && bdiff[1] == 0 ) {
      break;
    } 
  }

  // Copying remaining part of original path.
  while( *pdiff ) {
    *rp++ = *pdiff++;
    if( ++rsize == size ) {
      return NULL;
    }
  }

  *rp = 0;
  return result;
}

#if 0

int
main( int argc, char* argv[] )
{
  char result[_MAX_PATH];
  char cwd[_MAX_PATH];

  if( argc < 2 ) {
    fprintf( stderr, "usage: abs2rel path [base]\n" );
    return 1;
  }
  if( argc == 2 ) {
    if( !getcwd( cwd, _MAX_PATH )) {
      fprintf( stderr, "cannot get current directory.\n" );
      return 1;
    }
  } else
    strcpy( cwd, argv[2] );

  if( abs2rel( cwd, argv[1], result, _MAX_PATH )) {
    printf("%s\n", result);
  } else
    printf("ERROR\n");

  return 0;
}
#endif
