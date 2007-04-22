/*
 * Copyright 2006 Dmitry A.Steklenev
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
#define  INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include "xio_file.h"
#include <string.h>

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
static int
file_open( XFILE* x, const char* filename, int oflags )
{
  int omode = O_BINARY;

  if(( oflags & XO_WRITE ) && ( oflags & XO_READ )) {
    omode |= O_RDWR;
  } else if( oflags & XO_WRITE ) {
    omode |= O_WRONLY;
  } else if( oflags & XO_READ ) {
    omode |= O_RDONLY;
  }

  if( oflags & XO_CREATE ) {
    omode |= O_CREAT;
  }
  if( oflags & XO_APPEND ) {
    omode |= O_APPEND;
  }
  if( oflags & XO_APPEND ) {
    omode |= O_TRUNC;
  }

  // accept file:/// URLs too
  if ( strnicmp(filename, "file:///", 8) == 0 ) {
    filename += 8;
  }

  if(( x->protocol->s_handle = sopen( filename, omode, SH_DENYNO )) == -1 ) {
    return -1;
  }

  return 0;
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
static int
file_read( XFILE* x, char* result, unsigned int count ) {
  return read( x->protocol->s_handle, result, count );
}

/* Writes count bytes from source into the file. Returns the number
   of bytes moved from the source to the file. The return value may
   be positive but less than count. A return value of -1 indicates an
   error */
static int
file_write( XFILE* x, const char* source, unsigned int count ) {
  return write( x->protocol->s_handle, source, count );
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
static int
file_close( XFILE* x ) {
  return close( x->protocol->s_handle );
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
static long
file_tell( XFILE* x ) {
  return tell( x->protocol->s_handle );
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
static long
file_seek( XFILE* x, long offset, int origin )
{
  int omode;

  switch( origin ) {
    case XIO_SEEK_SET: omode = SEEK_SET; break;
    case XIO_SEEK_CUR: omode = SEEK_CUR; break;
    case XIO_SEEK_END: omode = SEEK_END; break;
    default:
      errno = EINVAL;
      return -1L;
  }

  return lseek( x->protocol->s_handle, offset, omode );
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
static long
file_size( XFILE* x )
{
  struct stat fi = {0};
  fstat( x->protocol->s_handle, &fi );
  return fi.st_size;
}

/* Cleanups the file protocol. */
static void
file_terminate( XFILE* x )
{
  if( x->protocol ) {
    if( x->protocol->mtx_access ) {
      DosCloseMutexSem( x->protocol->mtx_access );
    }
    if( x->protocol->mtx_file ) {
      DosCloseMutexSem( x->protocol->mtx_file );
    }
    free( x->protocol );
  }
}

/* Initializes the file protocol. */
XPROTOCOL*
file_initialize( XFILE* x )
{
  XPROTOCOL* protocol = calloc( 1, sizeof( XPROTOCOL ));

  if( protocol ) {
    if( DosCreateMutexSem( NULL, &protocol->mtx_access, 0, FALSE ) != NO_ERROR ||
        DosCreateMutexSem( NULL, &protocol->mtx_file  , 0, FALSE ) != NO_ERROR )
    {
      file_terminate( x );
      return NULL;
    }

    protocol->supports =
      XS_CAN_READ   | XS_CAN_WRITE | XS_CAN_READWRITE |
      XS_CAN_CREATE | XS_CAN_SEEK  | XS_CAN_SEEK_FAST;

    protocol->open  = file_open;
    protocol->read  = file_read;
    protocol->write = file_write;
    protocol->close = file_close;
    protocol->tell  = file_tell;
    protocol->seek  = file_seek;
    protocol->size  = file_size;
    protocol->clean = file_terminate;
  }

  return protocol;
}

