/*
 * Copyright 2007 Dmitry A.Steklenev
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <snprintf.h>

#include "xio_cddb.h"
#include "xio_url.h"
#include "xio_socket.h"

/* Gets and parses CDDB reply. */
static int
cddb_read_reply( XFILE* x )
{
  char buffer[1024];

  if( !so_readline( x->protocol->s_handle, buffer, sizeof( buffer ))) {
    return CDDB_PROTOCOL_ERROR;
  }

  if( !isdigit( buffer[0] ) || !isdigit( buffer[1] ) || !isdigit( buffer[2] )) {
    return CDDB_PROTOCOL_ERROR;
  }

  return ( buffer[0] - '0' ) * 100 +
         ( buffer[1] - '0' ) * 10  +
         ( buffer[2] - '0' );
}

/* Sends a command to a CDDB server and checks response. */
static int
cddb_send_command( XFILE* x, const char* format, ... )
{
  char buffer[1024];
  va_list args;

  va_start ( args, format );
  vsnprintf( buffer, sizeof( buffer ), format, args );

  if( so_write( x->protocol->s_handle, buffer, strlen( buffer )) == -1 ) {
    return CDDB_PROTOCOL_ERROR;
  }

  return cddb_read_reply( x );
}

/* Returns the specified part of the CDDB request. */
static char*
cddb_part_of_request( const char* request,
                      const char* part, char* result, int size )
{
  const char* p;
  int partsize  = strlen( part );
  int freesize  = size - 1;

  *result = 0;

  for( p = request; *p; p++ ) {
    if( strnicmp( p, part, partsize ) == 0 && p[ partsize ] == '=' )
    {
      const char* psrc = p + partsize + 1;
      char* pres = result;

      while( *psrc && *psrc != '&' && freesize )
      {
        *pres = ( *psrc == '+' ) ? ' ' : *psrc;
         pres++;
         psrc++;
         freesize--;
      }
      *pres = 0;
      break;
    }
    if(( p = strchr( p, '&' )) == NULL ) {
      break;
    }
  }

  return url_decode( result );
}

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
static int
cddb_open( XFILE* x, const char* filename, int oflags )
{
  XURL* url = url_allocate( filename );
  int   rc  = CDDB_OK;

  char  buffer[2048];

  for(;;)
  {
    x->protocol->s_handle = -1;

    if( !url ) {
      rc = CDDB_PROTOCOL_ERROR;
      return -1;
    }

    if( !url->query ) {
      rc = CDDB_PROTOCOL_ERROR;
      break;
    }

    x->protocol->s_handle =
      so_connect( so_get_address( url->host ), url->port ? url->port : 8880 );

    if( x->protocol->s_handle == -1 ) {
      rc = CDDB_PROTOCOL_ERROR;
      break;
    }

    // Receive server sign-on banner.
    rc = cddb_read_reply( x );

    if( rc != CDDB_OK && rc != CDDB_OK_READONLY ) {
      break;
    }

    // Initial client-server handshake.
    rc = cddb_send_command( x, "cddb hello %s\n",
                            cddb_part_of_request( url->query, "hello", buffer, sizeof( buffer )));
    if( rc != CDDB_OK ) {
      break;
    }

    // Sets the server protocol level.
    rc = cddb_send_command( x, "proto %s\n",
                            cddb_part_of_request( url->query, "proto", buffer, sizeof( buffer )));

    if( rc != CDDB_OK           &&
        rc != CDDB_PROTO_OK     &&
        rc != CDDB_PROTO_ALREADY )
    {
      break;
    }

    // Query. It is not necessary to parse last CDDB reply because it
    // should be read by the client side.
    cddb_part_of_request( url->query, "cmd", buffer, sizeof( buffer ));

    if( so_write( x->protocol->s_handle, buffer, strlen( buffer )) == -1 ||
        so_write( x->protocol->s_handle, "\n", 1 ) == -1 )
    {
      rc = CDDB_PROTOCOL_ERROR;
    }
    break;
  }

  url_free( url );

  if( rc == CDDB_OK || rc == CDDB_OK_READONLY ) {
    return 0;
  } else {
    if( x->protocol->s_handle != -1 ) {
      so_close( x->protocol->s_handle );
    }
    if( rc != CDDB_PROTOCOL_ERROR ) {
      errno = CDDBBASEERR + rc;
    }
    return -1;
  }

  return 0;
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
static int
cddb_read( XFILE* x, void* result, unsigned int count )
{
  int done = so_read( x->protocol->s_handle, result, count );

  if( done > 0 ) {
    DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
    x->protocol->s_pos += done;
    DosReleaseMutexSem( x->protocol->mtx_access );
  }

  return done;
}

/* Writes count bytes from source into the file. Returns the number
   of bytes moved from the source to the file. The return value may
   be positive but less than count. A return value of -1 indicates an
   error */
static int
cddb_write( XFILE* x, const void* source, unsigned int count )
{
  errno = EBADF;
  return -1;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
static int
cddb_close( XFILE* x )
{
  so_close( x->protocol->s_handle );
  return 0;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
static long
cddb_tell( XFILE* x )
{
  long pos;

  DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  pos = x->protocol->s_pos;
  DosReleaseMutexSem( x->protocol->mtx_access );
  return pos;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
static long
cddb_seek( XFILE* x, long offset, int origin )
{
  errno = EINVAL;
  return -1;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
static long
cddb_size( XFILE* x ) {
  return -1;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
static int
cddb_truncate( XFILE* x, long size )
{
  errno = EINVAL;
  return -1;
}

/* Cleanups the cddb protocol. */
static void
cddb_terminate( XFILE* x )
{
  if( x->protocol ) {
    if( x->protocol->mtx_access ) {
      DosCloseMutexSem( x->protocol->mtx_access );
    }
    if( x->protocol->mtx_file ) {
      DosCloseMutexSem( x->protocol->mtx_file );
    }
    free( x->protocol->s_location );
    free( x->protocol );
  }
}

/* Initializes the cddb protocol. */
XPROTOCOL*
cddb_initialize( XFILE* x )
{
  XPROTOCOL* protocol = (XPROTOCOL*)calloc( 1, sizeof( XPROTOCOL ));

  if( protocol ) {
    if( DosCreateMutexSem( NULL, &protocol->mtx_access, 0, FALSE ) != NO_ERROR ||
        DosCreateMutexSem( NULL, &protocol->mtx_file  , 0, FALSE ) != NO_ERROR )
    {
      cddb_terminate( x );
      return NULL;
    }

    protocol->supports =
      XS_CAN_READ | XS_USE_SPOS | XS_NOT_BUFFERIZE;

    protocol->open   = cddb_open;
    protocol->read   = cddb_read;
    protocol->write  = cddb_write;
    protocol->close  = cddb_close;
    protocol->tell   = cddb_tell;
    protocol->seek   = cddb_seek;
    protocol->chsize = cddb_truncate;
    protocol->size   = cddb_size;
    protocol->clean  = cddb_terminate;
  }

  return protocol;
}

/* Maps the error number in errnum to an error message string. */
const char*
cddb_strerror( int errnum )
{
  switch( errnum - CDDBBASEERR )
  {
    case CDDB_OK             : return "Command okay.";
    case CDDB_OK_READONLY    : return "Command okay.";
    case CDDB_NO_MATCH       : return "No match found.";
    case CDDB_FOUND_INEXACT  : return "Found inexact matches, list follows.";
    case CDDB_SITES_OK       : return "Ok, information follows.";
    case CDDB_SITES_ERROR    : return "No site information available.";
    case CDDB_SHOOK_ALREADY  : return "Already shook hands.";
    case CDDB_CORRUPT        : return "Database entry is corrupt.";
    case CDD_NO_HANDSHAKE    : return "No handshake.";
    case CDDB_BAD_HANDSHAKE  : return "Handshake not successful, closing connection.";
    case CDDB_DENIED         : return "No connections allowed: permission denied.";
    case CDDB_TOO_MANY_USERS : return "No connections allowed: too many users.";
    case CDDB_OVERLOAD       : return "No connections allowed: system load too high.";
    case CDDB_PROTO_ILLEGAL  : return "Illegal protocol level.";
    case CDDB_PROTO_ALREADY  : return "Already have protocol level.";

    default:
      return "Unexpected CDDB protocol error.";
  }
}

