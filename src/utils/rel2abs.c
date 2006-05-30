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

#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <string.h>

#include "rel2abs.h"

#define C_DIR_SEPARATOR '\\'
#define C_DEV_SEPARATOR ':'
#define C_DOT           '.'

/*
 * rel2abs: convert an relative path name into absolute.
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

  if( bsize >= size ) {
    return NULL;
  }

  if(( psize > 1 && path[1] == C_DEV_SEPARATOR ) ||
     ( psize > 1 && path[0] == C_DIR_SEPARATOR && path[1] == C_DIR_SEPARATOR ))
  {
    // The relative path should not begin with a name
    // of the disk drive or a name of a computer.
    if( psize >= size ) {
      return NULL;
    }

    strcpy( result, path );
    return result;
  }

  strcpy( result, base );
  rsize = bsize;
  rtail = result + strlen( base );

  if( rsize > 1 && result[1] == C_DEV_SEPARATOR ) {
    // Have name of the disk drive.
    if( rsize > 2 && result[2] == C_DIR_SEPARATOR ) {
      rroot = result + 2;
    } else {
      rroot = result + 1;
    }
  }

  if( rsize > 1 && result[0] == C_DIR_SEPARATOR
                && result[1] == C_DIR_SEPARATOR )
  {
    // Have UNC path. It is necessary to find the end of
    // the computer name.
    rroot = result + 3;
    while( rroot && *rroot != C_DIR_SEPARATOR ) {
      ++rroot;
    }
  }

  if( rsize && rtail-1 != rroot && rtail[-1] == C_DIR_SEPARATOR ) {
    // Remove last dir separator if have it.
    *--rtail = 0;
     --rsize;
  }

  if( psize == 0 ) {
    // The relative path is empty.
    return result;
  }

  if( *path == C_DIR_SEPARATOR ) {
    // If the relative path begins from a root.
    ++path;
    if( !rsize ) {
      *++rroot = C_DIR_SEPARATOR;
       ++rsize;
    }
    rtail = rroot + 1;
  }

  while( *path && rsize < size )
  {
    // Found link to current directory.
    if( path[0] == C_DOT && ( path[1] == C_DIR_SEPARATOR || !path[1] )) {
      path += 1;
      while( *path == C_DIR_SEPARATOR && *path ) {
        ++path;
      }
      continue;
    }

    // Found link to parent directory.
    if( path[0] == C_DOT && path[1] == C_DOT &&
      ( path[2] == C_DIR_SEPARATOR || !path[2] ))
    {
      path += 2;
      while( *path == C_DIR_SEPARATOR && *path ) {
        ++path;
      }
      while( rtail != rroot && *--rtail != C_DIR_SEPARATOR ) {
        --rsize;
      }
      continue;
    }

    // Found the fragment of the relative path.
    if( rtail != result && rtail[-1] != C_DIR_SEPARATOR ) {
      *rtail++ = C_DIR_SEPARATOR;
       rsize++;
    }
    while( *path != C_DIR_SEPARATOR && *path && rsize < size ) {
      *rtail++ = *path++;
       rsize++;
    }
    while( *path == C_DIR_SEPARATOR && *path ) {
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
    printf("%s\n", result);
  } else
    printf("ERROR\n");

  return 0;
}
#endif
