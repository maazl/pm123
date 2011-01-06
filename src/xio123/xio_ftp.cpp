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
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "xio_ftp.h"
#include "xio_url.h"
#include "xio_socket.h"

#include <interlocked.h>

static int
is_ftp_reply( const char* string )
{
  return isdigit( string[0] ) &&
         isdigit( string[1] ) &&
         isdigit( string[2] ) && ( string[3] == ' ' ||
                                   string[3] == 0 );
}

static int
is_ftp_info( const char* string )
{
  return isdigit( string[0] ) &&
         isdigit( string[1] ) &&
         isdigit( string[2] ) && string[3] == '-';
}

/* Get and parse a FTP server response. */
int XIOftp::read_reply()
{
  if( !s_socket.gets( s_reply, sizeof( s_reply ))) {
    return FTP_PROTOCOL_ERROR;
  }

  if( is_ftp_info( s_reply )) {
    while( !is_ftp_reply( s_reply )) {
      if( !s_socket.gets( s_reply, sizeof( s_reply ))) {
        return FTP_PROTOCOL_ERROR;
      }
    }
  }

  if( !is_ftp_reply( s_reply )) {
    return FTP_PROTOCOL_ERROR;
  }

  return ( s_reply[0] - '0' ) * 100 +
         ( s_reply[1] - '0' ) * 10  +
         ( s_reply[2] - '0' );
}

/* Sends a command to a FTP server and checks response. */
int XIOftp::send_command( const char* command, const char* params )
{
  int   size = strlen( command ) + strlen( params ) + 4;
  char* send = (char*)malloc( size );

  if( !send ) {
    return FTP_PROTOCOL_ERROR;
  }

  if( params && *params ) {
    sprintf( send, "%s %s\r\n", command, params );
  } else {
    sprintf( send, "%s\r\n", command );
  }

  if( s_socket.write( send, strlen( send )) == -1 ) {
    return FTP_PROTOCOL_ERROR;
  }

  free( send );
  return read_reply();
}

/* Initiates the transfer of the file specified by filename. */
int XIOftp::transfer_file( const char* filename, unsigned long range )
{
  char  host[XIO_MAX_HOSTNAME];
  char* p;
  int   rc;
  int   i;
  char  string[64];

  unsigned char address[6];

  // Sends a PASV command.
  if(( rc = send_command( "PASV", "" )) != FTP_PASSIVE_MODE ) {
    return rc;
  }

  // Finds an address and port number in a server reply.
  for( p = s_reply + 3; *p && !isdigit(*p); p++ )
  {}

  if( !*p ) {
    return FTP_BAD_SEQUENCE;
  } else {
    for( i = 0; *p && i < 6; i++, p++ ) {
      address[i] = strtoul( p, &p, 10 );
    }
    if( i < 6 ) {
      return FTP_BAD_SEQUENCE;
    }
  }

  // Sends a REST command.
  if( support & XS_CAN_SEEK ) {
    rc = send_command( "REST", ltoa( range, string, 10 ));
    if( rc != FTP_FILE_OK && rc != FTP_OK ) {
      support &= ~XS_CAN_SEEK;
      s_pos = 0;
    } else {
      s_pos = range;
    }
  }

  // Connects to data port.
  sprintf( host, "%d.%d.%d.%d", address[0],
                                address[1],
                                address[2],
                                address[3] );
  rc = s_datasocket.open( XIOsocket::get_address( host ), address[4] * 256 + address[5] );

  if( rc == -1 ) {
    return FTP_PROTOCOL_ERROR;
  }

  // Makes the server initiate the transfer.
  return send_command( "RETR", filename );
}

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
int XIOftp::open( const char* filename, XOFLAGS oflags )
{
  XURL* url = url_allocate( filename );
  char* get = url_string( url, XURL_STR_REQUEST );
  int   rc  = FTP_OK;

  for(;;)
  {
    s_socket.close();
    s_datasocket.close();

    if( !url || !get ) {
      rc = FTP_PROTOCOL_ERROR;
      break;
    }

    int rc2 = s_socket.open( XIOsocket::get_address( url->host ), url->port ? url->port : 21 );

    if( rc2 == -1 ) {
      rc = FTP_PROTOCOL_ERROR;
      break;
    }

    // Expects welcome message.
    if(( rc = read_reply()) != FTP_SERVICE_READY ) {
      break;
    }

    // Authenticate.
    if( !url->username && !url->password )
    {
      url->username = strdup( FTP_ANONYMOUS_USER );
      url->password = strdup( FTP_ANONYMOUS_PASS );
    }

    if( url->username )
    {
      rc = send_command( "USER", url->username );

      if( url->password && rc == FTP_NEED_PASSWORD ) {
        rc = send_command( "PASS", url->password );
      }
      if( rc != FTP_LOGGED_IN ) {
        break;
      }
    }

    // Sets a transfer mode to the binary mode.
    if(( rc = send_command( "TYPE", "I" )) != FTP_OK ) {
      break;
    }

    // Finds a file size.
    if(( rc = send_command( "SIZE", get )) == FTP_FILE_STATUS ) {
      s_size = strtoul( s_reply + 3, NULL, 10 );
    }

    // Makes the server initiate the transfer.
    if(( rc = transfer_file( get, 0 )) != FTP_OPEN_DATA_CONNECTION ) {
      break;
    }

    s_location = strdup( get );
    rc = FTP_OK;
    break;
  }

  url_free( url );
  free( get );

  if( rc == FTP_OK ) {
    return 0;
  } else {
    s_datasocket.close();
    s_socket.close();
    if( rc != FTP_PROTOCOL_ERROR ) {
      errno = error = FTPBASEERR + rc;
    }
    return -1;
  }
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
int XIOftp::read( void* result, unsigned int count )
{
  int done = s_datasocket.read( result, count );

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
int XIOftp::close()
{
  s_datasocket.close();
  read_reply();
  send_command( "QUIT", "" );
  s_socket.close();
  // Invalidate fields
  s_pos    = (unsigned long)-1;
  s_size   = (unsigned long)-1;
  return 0;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
long XIOftp::tell( long* offset64 )
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
long XIOftp::seek( long offset, int origin, long* offset64 )
{
  if(!( support & XS_CAN_SEEK )) {
    errno = EINVAL;
  } else {
    unsigned long range;

    switch( origin ) {
      case XIO_SEEK_SET:
        range = offset;
        break;
      case XIO_SEEK_CUR:
        range = s_pos  + offset;
        break;
      case XIO_SEEK_END:
        range = s_size + offset;
        break;
      default:
        errno = EINVAL;
        return -1;
    }

    s_datasocket.close();
    read_reply();

    if( s_location ) {
      if( transfer_file( s_location, range ) == FTP_OPEN_DATA_CONNECTION ) {
        // TODO: 64 bit
        if (offset64)
          *offset64 = 0;
        errno = error = 0;
        eof   = false;
        return range;
      }
    }
  }
  error = errno;
  return -1;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long XIOftp::getsize( long* offset64 )
{
  long size;

  // For now this is atomic
  size = s_size;
  // TODO: 64 bit
  if (offset64)
    *offset64 = 0;
  return size;
}

XSFLAGS XIOftp::supports() const
{ return support;
}

/* Cleanups the ftp protocol. */
XIOftp::~XIOftp()
{
  free( s_location );
}

/* Initializes the ftp protocol. */
XIOftp::XIOftp()
: support(XS_CAN_READ | XS_CAN_SEEK),
  s_pos((unsigned long)-1),
  s_size((unsigned long)-1),
  s_location(NULL)
{
  memset(s_reply, 0, sizeof s_reply);
  blocksize = 8192;
}

/* Maps the error number in errnum to an error message string. */
const char* XIOftp::strerror( int errnum )
{
  switch( errnum - FTPBASEERR )
  {
    case FTP_CONNECTION_ALREADY_OPEN : return "Data connection already open.";
    case FTP_OPEN_DATA_CONNECTION    : return "File status okay.";
    case FTP_OK                      : return "Command okay.";
    case FTP_FILE_STATUS             : return "File status okay.";
    case FTP_SERVICE_READY           : return "Service ready for new user.";
    case FTP_TRANSFER_COMPLETE       : return "Closing data connection.";
    case FTP_PASSIVE_MODE            : return "Entering Passive Mode.";
    case FTP_LOGGED_IN               : return "User logged in, proceed.";
    case FTP_FILE_ACTION_OK          : return "Requested file action okay, completed.";
    case FTP_DIRECTORY_CREATED       : return "Created.";
    case FTP_NEED_PASSWORD           : return "User name okay, need password.";
    case FTP_NEED_ACCOUNT            : return "Need account for login.";
    case FTP_FILE_OK                 : return "Requested file action pending further information.";
    case FTP_FILE_UNAVAILABLE        : return "File unavailable.";
    case FTP_SYNTAX_ERROR            : return "Syntax error, command unrecognized.";
    case FTP_BAD_ARGS                : return "Syntax error in parameters or arguments.";
    case FTP_NOT_IMPLEMENTED         : return "Command not implemented.";
    case FTP_BAD_SEQUENCE            : return "Bad sequence of commands.";
    case FTP_NOT_IMPL_FOR_ARGS       : return "Command not implemented for that parameter.";
    case FTP_NOT_LOGGED_IN           : return "Not logged in.";
    case FTP_FILE_NO_ACCESS          : return "File not found or no access.";
    case FTP_PROTOCOL_ERROR          : return "Unexpected socket error.";

    default:
      return "Unexpected FTP protocol error.";
  }
}

