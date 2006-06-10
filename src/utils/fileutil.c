/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <sys/stat.h>

#include "fileutil.h"

/* Returns the drive letter followed by a colon (:)
   if a drive is specified in pathname. */
char*
sdrive( char* result, const char *pathname )
{
  _splitpath((char*)pathname, result, NULL, NULL, NULL );
  return result;
}

/* Returns the path of subdirectories, if any, including
   the trailing slash.  Slashes (/), backslashes (\), or both
   may be present in pathname. */
char*
sdir( char* result, const char *pathname )
{
  _splitpath((char*)pathname, NULL, result, NULL, NULL );
  return result;
}

/* Returns the base file name without any extensions. */
char*
sfname( char* result, const char *pathname )
{
  _splitpath((char*)pathname, NULL, NULL, result, NULL );
  return result;
}

/* Returns the file name extension, if any,
   including the leading period (.). */
char*
sext( char* result, const char *pathname )
{
  _splitpath((char*)pathname, NULL, NULL, NULL, result );
  return result;
}

/* Returns the base file name with file extension. */
char*
sfnameext( char *result, const char *pathname )
{
  char fname[_MAX_FNAME];
  char ext  [_MAX_EXT  ];

  _splitpath((char*)pathname, NULL, NULL, fname, ext );
  strcpy( result, fname );
  strcat( result, ext   );
  return result;
}

/* Returns the drive letter and the path of
   subdirectories, if any, including the trailing slash.
   Slashes (/), backslashes (\), or both  may be present
   in pathname. */
char*
sdrivedir( char *result, const char *pathname )
{
  char drive[_MAX_DRIVE];
  char path [_MAX_PATH ];
  _splitpath((char*)pathname, drive, path, NULL, NULL );
  strcpy( result, drive );
  strcat( result, path  );
  return result;
}

/* Returns TRUE if the specified file is a HTTP URL. */
BOOL
is_http( const char* filename )
{
  return ( strnicmp( filename, "http://", 7 ) == 0 );
}

/* Returns TRUE if the specified file is a CD track. */
BOOL
is_track( const char* filename )
{
  return ( strnicmp( filename, "cd://"  , 5 ) == 0 );
}

/* Returns TRUE if the specified file is a regular file. */
BOOL
is_file( const char* filename )
{
  return *filename             &&
         !is_url  ( filename ) &&
         !is_track( filename );
}

/* Returns TRUE if the specified file is a URL. */
BOOL
is_url( const char* filename )
{
  return is_http( filename );
}

/* Returns TRUE if the specified directory is a root sirectory. */
BOOL
is_root( const char* path )
{
  size_t size = strlen( path );

  if( size == 3 && path[1] == ':'  && path[2] == '\\' ) {
    return TRUE;
  }
  if( size  > 3 && path[1] == '\\' && path[2] == '\\' ) {
    if( strchr( path + 2, '\\' ) == path + size - 1 ) {
      return TRUE;
    }
  }
  return FALSE;
}

/* Returns TRUE if the specified pathname is a directory. */
BOOL
is_dir( const char* path )
{
  struct stat fi;

  if( stat( path, &fi ) == 0 && fi.st_mode & S_IFDIR ) {
    return TRUE;
  } else {
    return FALSE;
  }
}

