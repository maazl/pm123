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
static int
ftp_read_reply( XFILE* x )
{
  XPROTOCOL* p = x->protocol;

  if( !so_readline( p->s_handle, p->s_reply, sizeof( p->s_reply ))) {
    return FTP_PROTOCOL_ERROR;
  }

  if( is_ftp_info( p->s_reply )) {
    while( !is_ftp_reply( p->s_reply )) {
      if( !so_readline( p->s_handle, p->s_reply, sizeof( p->s_reply ))) {
        return FTP_PROTOCOL_ERROR;
      }
    }
  }

  if( !is_ftp_reply( p->s_reply )) {
    return FTP_PROTOCOL_ERROR;
  }

  return ( p->s_reply[0] - '0' ) * 100 +
         ( p->s_reply[1] - '0' ) * 10  +
         ( p->s_reply[2] - '0' );
}

/* Sends a command to a FTP server and checks response. */
static int
ftp_send_command( XFILE* x, const char* command,
                            const char* params )
{
  int   size = strlen( command ) + strlen( params ) + 4;
  char* send = malloc( size );

  if( !send ) {
    return FTP_PROTOCOL_ERROR;
  }

  if( params && *params ) {
    sprintf( send, "%s %s\r\n", command, params );
  } else {
    sprintf( send, "%s\r\n", command );
  }

  if( so_write( x->protocol->s_handle, send, strlen( send )) == -1 ) {
    return FTP_PROTOCOL_ERROR;
  }

  free( send );
  return ftp_read_reply( x );
}

/* Initiates the transfer of the file specified by filename. */
static int
ftp_transfer_file( XFILE* x, const char* filename, unsigned long range )
{
  char  host[XIO_MAX_HOSTNAME];
  char* p;
  int   rc;
  int   i;
  char  string[64];

  unsigned char address[6];

  // Sends a PASV command.
  if(( rc = ftp_send_command( x, "PASV", "" )) != FTP_PASSIVE_MODE ) {
    return rc;
  }

  // Finds an address and port number in a server reply.
  for( p = x->protocol->s_reply + 3; *p && !isdigit(*p); p++ )
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
  if( x->protocol->supports & XS_CAN_SEEK ) {
    rc = ftp_send_command( x, "REST", ltoa( range, string, 10 ));
    if( rc != FTP_FILE_OK && rc != FTP_OK ) {
      DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
      x->protocol->supports &= ~XS_CAN_SEEK;
      x->protocol->s_pos = 0;
      DosReleaseMutexSem( x->protocol->mtx_access );
    } else {
      DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
      x->protocol->s_pos = range;
      DosReleaseMutexSem( x->protocol->mtx_access );
    }
  }

  // Connects to data port.
  sprintf( host, "%d.%d.%d.%d", address[0],
                                address[1],
                                address[2],
                                address[3] );
  x->protocol->s_datahandle =
    so_connect( so_get_address( host ), address[4] * 256 + address[5] );

  if( x->protocol->s_datahandle == -1 ) {
    return FTP_PROTOCOL_ERROR;
  }

  // Makes the server initiate the transfer.
  return ftp_send_command( x, "RETR", filename );
}

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
static int
ftp_open( XFILE* x, const char* filename, int oflags )
{
  XURL* url = url_allocate( filename );
  char* get = url_string( url, XURL_STR_REQUEST );
  int   rc  = FTP_OK;

  for(;;)
  {
    x->protocol->s_handle = -1;
    x->protocol->s_datahandle = -1;

    if( !url || !get ) {
      rc = FTP_PROTOCOL_ERROR;
      return -1;
    }

    x->protocol->s_handle =
      so_connect( so_get_address( url->host ), url->port ? url->port : 21 );

    if( x->protocol->s_handle == -1 ) {
      rc = FTP_PROTOCOL_ERROR;
      break;
    }

    // Expects welcome message.
    if(( rc = ftp_read_reply( x )) != FTP_SERVICE_READY ) {
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
      rc = ftp_send_command( x, "USER", url->username );

      if( url->password && rc == FTP_NEED_PASSWORD ) {
        rc = ftp_send_command( x, "PASS", url->password );
      }
      if( rc != FTP_LOGGED_IN ) {
        break;
      }
    }

    // Sets a transfer mode to the binary mode.
    if(( rc = ftp_send_command( x, "TYPE", "I" )) != FTP_OK ) {
      break;
    }

    // Finds a file size.
    if(( rc = ftp_send_command( x, "SIZE", get )) == FTP_FILE_STATUS ) {
      x->protocol->s_size = strtoul( x->protocol->s_reply + 3, NULL, 10 );
    }

    // Makes the server initiate the transfer.
    if(( rc = ftp_transfer_file( x, get, 0 )) != FTP_OPEN_DATA_CONNECTION ) {
      break;
    }

    x->protocol->s_location = strdup( get );
    rc = FTP_OK;
    break;
  }

  url_free( url );
  free( get );

  if( rc == FTP_OK ) {
    return 0;
  } else {
    if( x->protocol->s_datahandle != -1 ) {
      so_close( x->protocol->s_datahandle );
    }
    if( x->protocol->s_handle != -1 ) {
      so_close( x->protocol->s_handle );
    }
    if( rc != FTP_PROTOCOL_ERROR ) {
      errno = FTPBASEERR + rc;
    }
    return -1;
  }
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
static int
ftp_read( XFILE* x, char* result, unsigned int count )
{
  int done = so_read( x->protocol->s_datahandle, result, count );

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
ftp_write( XFILE* x, const char* source, unsigned int count )
{
  errno = EBADF;
  return -1;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
static int
ftp_close( XFILE* x )
{
  so_close( x->protocol->s_datahandle );
  ftp_read_reply( x );
  ftp_send_command( x, "QUIT", "" );
  so_close( x->protocol->s_handle );
  return 0;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
static long
ftp_tell( XFILE* x )
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
ftp_seek( XFILE* x, long offset, int origin )
{
  if(!( x->protocol->supports & XS_CAN_SEEK )) {
    errno = EINVAL;
  } else {
    unsigned long range;

    switch( origin ) {
      case XIO_SEEK_SET:
        range = offset;
        break;
      case XIO_SEEK_CUR:
        range = x->protocol->s_pos  + offset;
        break;
      case XIO_SEEK_END:
        range = x->protocol->s_size - offset;
        break;
      default:
        return -1;
    }

    so_close( x->protocol->s_datahandle );
    ftp_read_reply( x );

    if( x->protocol->s_location ) {
      if( ftp_transfer_file( x, x->protocol->s_location, range ) == FTP_OPEN_DATA_CONNECTION ) {
        return range;
      }
    }
  }
  return -1;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
static long
ftp_size( XFILE* x )
{
  long size;

  DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  size = x->protocol->s_size;
  DosReleaseMutexSem( x->protocol->mtx_access );
  return size;
}

/* Initializes the ftp protocol. */
XPROTOCOL*
ftp_initialize( XFILE* x )
{
  XPROTOCOL* protocol = calloc( 1, sizeof( XPROTOCOL ));

  if( protocol ) {
    if( DosCreateMutexSem( NULL, &protocol->mtx_access, 0, FALSE ) != NO_ERROR ||
        DosCreateMutexSem( NULL, &protocol->mtx_file  , 0, FALSE ) != NO_ERROR )
    {
      ftp_terminate( x );
      return NULL;
    }

    protocol->supports =
      XS_CAN_READ | XS_CAN_SEEK | XS_USE_SPOS;

    protocol->open  = ftp_open;
    protocol->read  = ftp_read;
    protocol->write = ftp_write;
    protocol->close = ftp_close;
    protocol->tell  = ftp_tell;
    protocol->seek  = ftp_seek;
    protocol->size  = ftp_size;
  }

  return protocol;
}

/* Cleanups the ftp protocol. */
void
ftp_terminate( XFILE* x )
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

/* Maps the error number in errnum to an error message string. */
const char*
ftp_strerror( int errnum )
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

