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
#include <string.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

#include "xio_file.h"
#include "xio_url.h"

#include <debuglog.h>

#ifdef XIO_SERIALIZE_DISK_IO

  HMTX serialize;

  // Serializes all read and write disk operations. This is
  // improve performance of poorly implemented filesystems (OS/2 version
  // of FAT32 for example).

  #define FILE_REQUEST_DISK( x ) if( x->protocol->s_serialized ) { DosRequestMutexSem( serialize, SEM_INDEFINITE_WAIT );}
  #define FILE_RELEASE_DISK( x ) if( x->protocol->s_serialized ) { DosReleaseMutexSem( serialize ); }
#else
  #define FILE_REQUEST_DISK( x )
  #define FILE_RELEASE_DISK( x )
#endif

#ifdef XIO_SERIALIZE_DISK_IO
static int
is_singletasking_fs( const char* filename )
{
  if( isalpha( filename[0] ) && filename[1] == ':' )
  {
    // Return-data buffer should be large enough to hold FSQBUFFER2
    // and the maximum data for szName, szFSDName, and rgFSAData
    // Typically, the data isn't that large.

    BYTE fsqBuffer[ sizeof( FSQBUFFER2 ) + ( 3 * CCHMAXPATH )] = { 0 };
    PFSQBUFFER2 pfsqBuffer = (PFSQBUFFER2)fsqBuffer;

    ULONG  cbBuffer        = sizeof( fsqBuffer );
    PBYTE  pszFSDName      = NULL;
    UCHAR  szDeviceName[3] = "x:";
    APIRET rc;

    szDeviceName[0] = filename[0];

    rc = DosQueryFSAttach( szDeviceName,    // Logical drive of attached FS
                           0,               // Ignored for FSAIL_QUERYNAME
                           FSAIL_QUERYNAME, // Return data for a Drive or Device
                           pfsqBuffer,      // Returned data
                          &cbBuffer );      // Returned data length

    // On successful return, the fsqBuffer structure contains
    // a set of information describing the specified attached
    // file system and the DataBufferLen variable contains
    // the size of information within the structure.

    if( rc == NO_ERROR ) {
      pszFSDName = pfsqBuffer->szName + pfsqBuffer->cbName + 1;
      if( stricmp( pszFSDName, "FAT32" ) == 0 ) {
        DEBUGLOG(( "xio123: detected %s filesystem for file %s, serialize access.\n", pszFSDName, filename ));
        return 1;
      } else {
        DEBUGLOG(( "xio123: detected %s filesystem for file %s.\n", pszFSDName, filename ));
      }
    }
  }
  return 0;
}
#endif

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
  if( oflags & XO_TRUNCATE ) {
    omode |= O_TRUNC;
  }

  if( strnicmp( filename, "file:", 5 ) == 0 )
  {
    XURL* url = url_allocate( filename );
    char* p;

    if( !url->path ) {
      url_free( url );
      errno = ENOENT;
      return -1;
    }

    // Converts leading drive letters of the form C| to C:
    // and if a drive letter is present or we have UNC path
    // strips off the slash that precedes path. Otherwise,
    // the leading slash is used.

    for( p = url->path; *p; p++ ) {
      if( *p == '/'  ) {
          *p = '\\';
      }
    }

    p = url->path;

    if( isalpha( p[1] ) && ( p[2] == '|' || p[2] == ':' )) {
      p[2] = ':';
      ++p;
    } else if ( p[1] == '\\' && p[2] == '\\' ) {
      ++p;
    }

    #ifdef XIO_SERIALIZE_DISK_IO
    x->protocol->s_serialized = is_singletasking_fs( p );
    #endif

    FILE_REQUEST_DISK(x);
    x->protocol->s_handle = sopen( p, omode, SH_DENYNO, S_IREAD | S_IWRITE );
    FILE_RELEASE_DISK(x);
    url_free( url );
  } else {
    #ifdef XIO_SERIALIZE_DISK_IO
    x->protocol->s_serialized = is_singletasking_fs( filename );
    #endif

    FILE_REQUEST_DISK(x);
    x->protocol->s_handle = sopen( filename, omode, SH_DENYNO, S_IREAD | S_IWRITE );
    FILE_RELEASE_DISK(x);
  }

  if( x->protocol->s_handle == -1 ) {
    return -1;
  } else {
    return  0;
  }
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
static int
file_read( XFILE* x, char* result, unsigned int count )
{
  int rc;

  FILE_REQUEST_DISK(x);
  rc = read( x->protocol->s_handle, result, count );
  FILE_RELEASE_DISK(x);
  return rc;
}

/* Writes count bytes from source into the file. Returns the number
   of bytes moved from the source to the file. The return value may
   be positive but less than count. A return value of -1 indicates an
   error */
static int
file_write( XFILE* x, const char* source, unsigned int count )
{
  int rc;

  FILE_REQUEST_DISK(x);
  rc = write( x->protocol->s_handle, source, count );
  FILE_RELEASE_DISK(x);
  return rc;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
static int
file_close( XFILE* x )
{
  int rc;

  FILE_REQUEST_DISK(x);
  rc = close( x->protocol->s_handle );
  FILE_RELEASE_DISK(x);
  return rc;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
static long
file_tell( XFILE* x )
{
  long rc;

  FILE_REQUEST_DISK(x);
  rc = tell( x->protocol->s_handle );
  FILE_RELEASE_DISK(x);
  return rc;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
static long
file_seek( XFILE* x, long offset, int origin )
{
  int  omode;
  long rc;

  switch( origin ) {
    case XIO_SEEK_SET: omode = SEEK_SET; break;
    case XIO_SEEK_CUR: omode = SEEK_CUR; break;
    case XIO_SEEK_END: omode = SEEK_END; break;
    default:
      errno = EINVAL;
      return -1L;
  }

  FILE_REQUEST_DISK(x);
  rc = lseek( x->protocol->s_handle, offset, omode );
  FILE_RELEASE_DISK(x);
  return rc;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
static long
file_size( XFILE* x )
{
  struct stat fi = {0};

  FILE_REQUEST_DISK(x);
  fstat( x->protocol->s_handle, &fi );
  FILE_RELEASE_DISK(x);
  return fi.st_size;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
static int
file_truncate( XFILE* x, long size )
{
  int rc;

  FILE_REQUEST_DISK(x);
  rc = chsize( x->protocol->s_handle, size );
  FILE_RELEASE_DISK(x);
  return rc;
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

    protocol->open   = file_open;
    protocol->read   = file_read;
    protocol->write  = file_write;
    protocol->close  = file_close;
    protocol->tell   = file_tell;
    protocol->seek   = file_seek;
    protocol->chsize = file_truncate;
    protocol->size   = file_size;
    protocol->clean  = file_terminate;
  }

  return protocol;
}

