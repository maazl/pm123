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

#define  INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "fileutil.h"
#include "strutils.h"
#include "minmax.h"

#define  isslash( c ) ( c == '/' || c == '\\' )

/* Returns TRUE if the specified location is a HTTP URL. */
BOOL
is_http( const char* location ) {
  return ( strnicmp( location, "http:", 5 ) == 0 );
}

/* Returns TRUE if the specified location is a CD track. */
BOOL
is_track( const char* location ) {
  return ( strnicmp( location, "cd:"  , 3 ) == 0 );
}

/* Returns TRUE if the specified location is a URL. */
BOOL
is_url( const char* location )
{
  if( !is_track( location ) && !( isalpha( location[0] ) && location[1] == ':' ))
  {
    const char* pc;

    // If the parse string contain a colon after the 1st character
    // and before any characters not allowed as part of a scheme
    // name (i.e. any not alphanumeric, '+', '.' or '-'),
    // the scheme of the url is the substring of chars up to
    // but not including the first colon. These chars and the
    // colon are then removed from the parse string before
    // continuing.

    for( pc = location; isalnum( *pc ) || ( *pc == '+' ) || ( *pc == '-' ) || ( *pc == '.' ); ++pc )
    {}

    if( *pc == ':' ) {
      return TRUE;
    }
  }
  return FALSE;
}

/* Returns TRUE if the specified location is a regular file. */
BOOL
is_file( const char* location )
{
  return *location             &&
         !is_track( location ) &&
         !is_url  ( location );

}

/* Returns the drive letter followed by a colon (:)
   if a drive is specified in location. */
char*
sdrive( char* result, const char* location, size_t size )
{
  if( isalpha( location[0] ) && location[1] == ':' ) {
    strlcpy( result, location, min( 3, size ));
  } else {
    *result = 0;
  }
  return result;
}

/* Returns the scheme followed by a colon (:)
   of the specified location. */
char*
scheme( char* result, const char* location, size_t size )
{
  if( !isalpha( location[0] ) || location[1] != ':' )
  {
    const char* pc;

    // If the parse string contain a colon after the 1st character
    // and before any characters not allowed as part of a scheme
    // name (i.e. any not alphanumeric, '+', '.' or '-'),
    // the scheme of the url is the substring of chars up to
    // but not including the first colon.

    for( pc = location; isalnum( *pc ) || ( *pc == '+' ) || ( *pc == '-' ) || ( *pc == '.' ); pc++ )
    {}

    if( *pc == ':' ) {
      strlcpy( result, location, min( pc - location + 2, size ));
    }
  } else if( size ) {
    *result = 0;
  }
  return result;
}

/* Returns the base file name with file extension. */
char*
sfnameext( char *result, const char* location, size_t size )
{
  const char* phead  = location;
  const char* ptail  = location + strlen( location );
  const char* pc;
  BOOL        is_url = FALSE;

  // If the parse string contain a colon after the 1st character
  // and before any characters not allowed as part of a scheme
  // name (i.e. any not alphanumeric, '+', '.' or '-'),
  // the scheme of the url is the substring of chars up to
  // but not including the first colon. These chars and the
  // colon are then removed from the parse string before
  // continuing.
  while( isalnum( *phead ) || ( *phead == '+' ) || ( *phead == '-' ) || ( *phead == '.' )) {
    ++phead;
  }
  if( *phead != ':' ) {
    phead = location;
  } else {
    phead++;
    // If the scheme consists more than of one symbol or its first
    // symbol not the alphabetic character that it is exact not a
    // name of the drive.
    if( phead - location > 2 || !isalpha( *location )) {
      is_url = TRUE;
    }
  }

  // Skip location (user:password@host:port part of the url
  // or \\server part of the regular pathname) on the front.
  if(( is_url || location == phead ) && isslash( phead[0] ) && isslash( phead[1] )) {
    phead += 2;
    while( *phead && !isslash( *phead )) {
      ++phead;
    }
  }

  // Remove fragment identifier, parameters or query information,
  // if any, from the back of the url.
  if( is_url ) {
    if(( pc = strpbrk( phead, "#?;" )) != NULL ) {
      ptail = pc;
    }
  }

  // Skip path name.
  for( pc = phead; *pc && pc < ptail; pc++ ) {
    if( isslash( *pc )) {
      phead = pc + 1;
    }
  }

  // Remainder is a file name.
  strlcpy( result, phead, min( ptail - phead + 1, size ));
  return result;
}

/* Returns the file name extension, if any,
   including the leading period (.). */
char*
sfext( char* result, const char* location, size_t size )
{
  const char* phead  = location;
  const char* ptail  = location + strlen( location );
  const char* pc;
  BOOL        is_url = FALSE;

  // If the parse string contain a colon after the 1st character
  // and before any characters not allowed as part of a scheme
  // name (i.e. any not alphanumeric, '+', '.' or '-'),
  // the scheme of the url is the substring of chars up to
  // but not including the first colon. These chars and the
  // colon are then removed from the parse string before
  // continuing.
  while( isalnum( *phead ) || ( *phead == '+' ) || ( *phead == '-' ) || ( *phead == '.' )) {
    ++phead;
  }
  if( *phead != ':' ) {
    phead = location;
  } else {
    phead++;
    // If the scheme consists more than of one symbol or its first
    // symbol not the alphabetic character that it is exact not a
    // name of the drive.
    if( phead - location > 2 || !isalpha( *location )) {
      is_url = TRUE;
    }
  }

  // Skip location (user:password@host:port part of the url
  // or \\server part of the regular pathname) on the front.
  if(( is_url || location == phead ) && isslash( phead[0] ) && isslash( phead[1] )) {
    phead += 2;
    while( *phead && !isslash( *phead )) {
      ++phead;
    }
  }

  // Remove fragment identifier, parameters or query information,
  // if any, from the back of the url.
  if( is_url ) {
    if(( pc = strpbrk( phead, "#?;" )) != NULL ) {
      ptail = pc;
    }
  }

  // Skip path name.
  for( pc = phead; *pc && pc < ptail; pc++ ) {
    if( isslash( *pc )) {
      phead = pc + 1;
    }
  }

  // Remainder is a file name, search file extension.
  for( pc = ptail - 1; pc > phead && *pc != '.'; pc-- )
  {}

  if( *pc == '.' && pc != phead ) {
    strlcpy( result, pc, min( ptail - pc + 1, size ));
  } else if( size ) {
    *result = 0;
  }

  return result;
}

/* Returns the base file name without any extensions. */
char*
sfname( char* result, const char* location, size_t size )
{
  const char* phead  = location;
  const char* ptail  = location + strlen( location );
  const char* pc;
  BOOL        is_url = FALSE;

  // If the parse string contain a colon after the 1st character
  // and before any characters not allowed as part of a scheme
  // name (i.e. any not alphanumeric, '+', '.' or '-'),
  // the scheme of the url is the substring of chars up to
  // but not including the first colon. These chars and the
  // colon are then removed from the parse string before
  // continuing.
  while( isalnum( *phead ) || ( *phead == '+' ) || ( *phead == '-' ) || ( *phead == '.' )) {
    ++phead;
  }
  if( *phead != ':' ) {
    phead = location;
  } else {
    phead++;
    // If the scheme consists more than of one symbol or its first
    // symbol not the alphabetic character that it is exact not a
    // name of the drive.
    if( phead - location > 2 || !isalpha( *location )) {
      is_url = TRUE;
    }
  }

  // Skip location (user:password@host:port part of the url
  // or \\server part of the regular pathname) on the front.
  if(( is_url || location == phead ) && isslash( phead[0] ) && isslash( phead[1] )) {
    phead += 2;
    while( *phead && !isslash( *phead )) {
      ++phead;
    }
  }

  // Remove fragment identifier, parameters or query information,
  // if any, from the back of the url.
  if( is_url ) {
    if(( pc = strpbrk( phead, "#?;" )) != NULL ) {
      ptail = pc;
    }
  }

  // Skip path name.
  for( pc = phead; *pc && pc < ptail; pc++ ) {
    if( isslash( *pc )) {
      phead = pc + 1;
    }
  }

  // Remainder is a file name, skip file extension.
  for( pc = ptail - 1; pc > phead && *pc != '.'; pc-- )
  {}

  if( *pc == '.' && pc != phead ) {
    ptail = pc;
  }

  strlcpy( result, phead, min( ptail - phead + 1, size ));
  return result;
}

/* Returns the drive letter or scheme and the path of
   subdirectories, if any, including the trailing slash.
   Slashes (/), backslashes (\), or both may be present
   in location. */
char*
sdrivedir( char *result, const char* location, size_t size )
{
  const char* phead  = location;
  const char* ptail  = location + strlen( location );
  const char* pc;
  BOOL        is_url = FALSE;

  // If the parse string contain a colon after the 1st character
  // and before any characters not allowed as part of a scheme
  // name (i.e. any not alphanumeric, '+', '.' or '-'),
  // the scheme of the url is the substring of chars up to
  // but not including the first colon. These chars and the
  // colon are then removed from the parse string before
  // continuing.
  while( isalnum( *phead ) || ( *phead == '+' ) || ( *phead == '-' ) || ( *phead == '.' )) {
    ++phead;
  }
  if( *phead != ':' ) {
    phead = location;
  } else {
    phead++;
    // If the scheme consists more than of one symbol or its first
    // symbol not the alphabetic character that it is exact not a
    // name of the drive.
    if( phead - location > 2 || !isalpha( *location )) {
      is_url = TRUE;
    }
  }

  // Skip location (user:password@host:port part of the url
  // or \\server part of the regular pathname) on the front.
  if(( is_url || location == phead ) && isslash( phead[0] ) && isslash( phead[1] )) {
    phead += 2;
    while( *phead && !isslash( *phead )) {
      ++phead;
    }
  }

  // Remove fragment identifier, parameters or query information,
  // if any, from the back of the url.
  if( is_url ) {
    if(( pc = strpbrk( phead, "#?;" )) != NULL ) {
      ptail = pc;
    }
  }

  // Search end of the path name.
  for( pc = ptail - 1; pc >= phead && !isslash( *pc ); pc-- )
  {}

  if( isslash( *pc )) {
    ptail = pc + 1;
  }

  strlcpy( result, location, min( ptail - location + 1, size ));
  return result;
}

/* Passed any string value, decode from URL transmission. */
char*
sdecode( char* result, const char* location, size_t size )
{
  const char* digits = "0123456789ABCDEF";
  const char* p;
  const char* ps = location;
  char* pr = result;
  int   i;

  if( size-- ) {
    while( *ps && size )
    {
      if( *ps == '%' ) {
        if( *ps && ( p = strchr( digits, toupper( *++ps ))) != NULL ) {
          i = p - digits;
          if( *ps && ( p = strchr( digits, toupper( *++ps ))) != NULL ) {
            i = i * 16 + p - digits;
            if( size ) {
              *pr = (char)i;
            }
          }
        }
      } else {
        if( size ) {
          *pr = *ps;
        }
      }
      ++ps;
      ++pr;
      --size;
    }
    *pr = 0;
  }

  return result;
}

/* Returns TRUE if the specified location is a root directory. */
BOOL
is_root( const char* location )
{
  return strlen( location ) == 3
         && location[1] == ':'
         && isslash( location[2] );
}

/* Returns TRUE if the specified location is a directory. */
BOOL
is_dir( const char* location )
{
  struct stat fi;
  return ( stat( location, &fi ) == 0 ) && ( fi.st_mode & S_IFDIR );
}
