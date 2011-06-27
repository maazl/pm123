/*
 * Copyright 2008-2011 M.Mueller
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

#include <debuglog.h>

/* Gets and parses CDDB reply. */
int XIOcddb::read_reply()
{
  char buffer[1024];

  if( !s_socket.gets( buffer, sizeof( buffer ))) {
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
int XIOcddb::send_command( const char* format, ... )
{
  char buffer[1024];
  va_list args;

  va_start ( args, format );
  vsnprintf( buffer, sizeof( buffer ), format, args );

  if( s_socket.write( buffer, strlen( buffer )) == -1 ) {
    return CDDB_PROTOCOL_ERROR;
  }

  return read_reply();
}

/* Returns the specified part of the CDDB request. */
static char*
part_of_request( const char* request,
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
int XIOcddb::open( const char* filename, XOFLAGS oflags )
{
  XURL* url = url_allocate( filename );
  int   rc  = CDDB_OK;

  char  buffer[2048];

  for(;;)
  {
    s_socket.close();

    if( !url ) {
      rc = CDDB_PROTOCOL_ERROR;
      break;
    }

    if( !url->query ) {
      rc = CDDB_PROTOCOL_ERROR;
      break;
    }

    if( s_socket.open( XIOsocket::get_address( url->host ), url->port ? url->port : 8880 ) == -1 ) {
      rc = CDDB_PROTOCOL_ERROR;
      break;
    }

    // Receive server sign-on banner.
    rc = read_reply();

    if( rc != CDDB_OK && rc != CDDB_OK_READONLY ) {
      break;
    }

    // Initial client-server handshake.
    rc = send_command( "cddb hello %s\n",
                       part_of_request( url->query, "hello", buffer, sizeof( buffer )));
    if( rc != CDDB_OK ) {
      break;
    }

    // Sets the server protocol level.
    rc = send_command( "proto %s\n",
                       part_of_request( url->query, "proto", buffer, sizeof( buffer )));

    if( rc != CDDB_OK           &&
        rc != CDDB_PROTO_OK     &&
        rc != CDDB_PROTO_ALREADY )
    {
      break;
    }

    // Query. It is not necessary to parse last CDDB reply because it
    // should be read by the client side.
    part_of_request( url->query, "cmd", buffer, sizeof( buffer ));

    if( s_socket.puts( buffer ) == -1 ||
        s_socket.puts( "\n" ) == -1 )
    {
      rc = CDDB_PROTOCOL_ERROR;
    }
    break;
  }

  url_free( url );

  if( rc == CDDB_OK || rc == CDDB_OK_READONLY ) {
    return 0;
  } else {
    s_socket.close();
    if( rc != CDDB_PROTOCOL_ERROR ) {
      errno = error = CDDBBASEERR + rc;
    }
    return -1;
  }

  return 0;
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
int XIOcddb::read( void* result, unsigned int count )
{
  int done = s_socket.read( result, count );

  if (done == -1)
    error = errno;
  else
  { if (done == 0)
      eof = true;
    InterlockedAdd(&(volatile unsigned&)s_pos, done);
  }

  return done;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
int XIOcddb::close()
{
  return s_socket.close();
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
long XIOcddb::tell( long* offset64 )
{
  long pos;

  // For now this is atomic
  pos = s_pos;
  // TODO: 64 bit
  if (offset64)
    *offset64 = 0;
  return pos;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
long XIOcddb::seek( long offset, int origin, long* offset64 )
{
  errno = EINVAL;
  if (offset64)
    *offset64 = -1;
  return -1;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long XIOcddb::getsize( long* offset64 ) {
  if (offset64)
    *offset64 = -1;
  return -1;
}

int XIOcddb::getstat( XSTAT* st )
{ errno = EINVAL;
  return -1;
}

XSFLAGS XIOcddb::supports() const
{ return XS_CAN_READ;// | XS_NOT_BUFFERIZE;
}

/* Cleanups the cddb protocol. */
XIOcddb::~XIOcddb()
{
  close();
}

/* Initializes the cddb protocol. */
XIOcddb::XIOcddb()
: s_pos((unsigned long)-1)
{ blocksize = 4096; // Sufficient for most Records
}

/* Maps the error number in errnum to an error message string. */
const char* XIOcddb::strerror( int errnum )
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
    case CDDB_NO_HANDSHAKE   : return "No handshake.";
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

