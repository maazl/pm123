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

#define  BSD_SELECT

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <nerrno.h>

#ifndef  TCPV40HDRS
#include <arpa/inet.h>
#include <unistd.h>
#endif

#ifdef   __IBMC__
#pragma  info( nocnd )
#endif

#define  NONBLOCKED_CONNECT

#include "xio_socket.h"
#include "xio.h"

#include <debuglog.h>

void XIOsocket::seterror(int err)
{ errno = error = err;
}
void XIOsocket::seterror()
{ errno = error = sock_errno();
}

/* Converts a string containing a valid internet address using
   dotted-decimal notation or host name into an internet address
   number typed as an unsigned long value.  A -1 value
   indicates an error. */
u_long XIOsocket::get_address( const char* hostname )
{
  u_long address;
  struct hostent* entry;

  if( !hostname ) {
    errno = HBASEERR + HOST_NOT_FOUND;
    return (u_long)-1L;
  }

  if(( address = inet_addr( (char*)hostname )) == (u_long)-1L ) {
    if(( entry = gethostbyname( (char*)hostname )) != NULL ) {
      memcpy( &address, entry->h_addr, sizeof( address ));
    } else {
      #ifdef NETDB_INTERNAL
        if( h_errno == NETDB_INTERNAL ) {
          errno = sock_errno();
        } else {
      #endif
          errno = h_errno + HBASEERR;
      #ifdef NETDB_INTERNAL
        }
      #endif
    }
  }

  return address;
}

// Converts a string containing a valid service or port
// into a port number. A -1 value indicates an error.
int XIOsocket::get_service( const char* service )
{
  int port;
  size_t len;
  if (sscanf(service, "%i%n", &port, &len) != 1 || strlen(service) != len)
  { struct servent* servent = getservbyname( (char*)service, "TCP" );
    if (servent == NULL)
    {
      #ifdef NETDB_INTERNAL
        if( h_errno == NETDB_INTERNAL ) {
          errno = sock_errno();
        } else {
      #endif
          errno = h_errno + HBASEERR;
      #ifdef NETDB_INTERNAL
        }
      #endif
      return -1;
    }
    port = servent->s_port;
  }
  return port;
}

int XIOsocket::open( const char* uri, XOFLAGS oflags )
{ DEBUGLOG(("XIOsocket(%p)::open(%s, %x)\n", this, uri, oflags));
  if (strnicmp(uri, "tcpip://", 8) != 0)
  { seterror(SOCESOCKTNOSUPPORT);
    return -1;
  }
  uri += 8;
  const char* cp = strchr(uri, ':');
  if (cp == NULL)
  { seterror(SOCESOCKTNOSUPPORT);
    return -1;
  }
  char* host = (char*)alloca(cp-uri+1);
  memcpy(host, uri, cp-uri);
  host[cp-uri] = 0;
  u_long address = get_address(host);
  if (address == (u_long)-1L)
  { error = errno;
    return -1;
  }
  int port = get_service(cp);
  if (port == -1)
  { error = errno;
    return -1;
  }

  error = 0;
  return open(address, port);
}
/* Creates an endpoint for communication and requests a
   connection to a remote host. A non-negative socket descriptor
   return value indicates success. The return value -1 indicates
   an error. */
int XIOsocket::open( u_long address, int port )
{
  struct sockaddr_in server = {0};
  DEBUGLOG(("XIOsocket(%p)::open(0x%x, %d)\n", this, address, port));

  if( address == (u_long)-1L || port == -1 ) {
    return -1;
  }

  if(( s_handle = socket( PF_INET, SOCK_STREAM, 0 )) != -1 )
  {
    #ifdef NONBLOCKED_CONNECT
    int dontblock;
    #endif
    #ifndef  TCPV40HDRS
    struct timeval tv = {0};
    #endif // TCPV40HDRS

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = address;
    server.sin_port = htons( port );

    setsockopt( s_handle, IPPROTO_TCP, SO_KEEPALIVE, NULL, TRUE );
    #ifndef  TCPV40HDRS
    tv.tv_sec = xio_socket_timeout();
    if (tv.tv_sec) {
      setsockopt( s_handle, IPPROTO_TCP, SO_RCVTIMEO, &tv, sizeof tv );
      setsockopt( s_handle, IPPROTO_TCP, SO_SNDTIMEO, &tv, sizeof tv );
    }
    #endif // TCPV40HDRS

    #ifdef NONBLOCKED_CONNECT

      dontblock = 1;
      ioctl( s_handle, FIONBIO, (char*)&dontblock, sizeof( dontblock ));

      if( connect( s_handle, (struct sockaddr*)&server, sizeof( server )) != -1 ||
          sock_errno() == SOCEINPROGRESS )
      {
        struct timeval timeout = {0};
        fd_set waitlist;

        timeout.tv_sec = xio_connect_timeout();

        FD_ZERO( &waitlist    );
        FD_SET ( s_handle, &waitlist );

        if( select( s_handle + 1, NULL, &waitlist, NULL, &timeout ) <= 0 ) {
          seterror();
          soclose( s_handle );
          s_handle = -1;
        } else {
          dontblock = 0;
          ioctl( s_handle, FIONBIO, (char*)&dontblock, sizeof( dontblock ));
        }
      } else {
        seterror();
        soclose( s_handle );
        s_handle = -1;
      }
    #else
      if( connect( s_handle, (struct sockaddr*)&server, sizeof( server )) == -1 ) {
        seterror();
        soclose( s_handle );
        s_handle = -1;
      }
    #endif // NONBLOCKED_CONNECT
  } else {
    seterror();
  }

  DEBUGLOG(("XIOsocket::open: %d\n", s_handle));
  return -(s_handle == -1);
}

/* Sends data on a connected socket. When successful, returns 0.
   The return value -1 indicates an error was detected on the
   sending side of the connection. */
int XIOsocket::write( const void* buffer, unsigned int size )
{
  int done;
  DEBUGLOG2(("XIOsocket(%p{%d})::write(%p, %d)\n", this, s_handle, buffer, size));

  while( size )
  {
    #ifdef TCPV40HDRS
    // Work around for missing SO_SNDTIMEO in 16 bit IP stack.
    struct timeval timeout = {0};

    timeout.tv_sec = xio_socket_timeout();
    if ( timeout.tv_sec ) {
      fd_set waitlist;

      FD_ZERO( &waitlist    );
      FD_SET ( s_handle, &waitlist );

      switch ( select( s_handle+1, NULL, &waitlist, NULL, &timeout )) {
        case 0: // Timeout
          seterror(SOCETIMEDOUT);
          return -1;
        case -1: // Error
          seterror();
          return -1;
      }
    }
    #endif
    done = send( s_handle, (char*)buffer, size, 0 );

    if( done <= 0 ) {
      seterror();
      return -1;
    }

    (char*&)buffer += done;
    size           -= done;
  }

  return 0;
}

/* Receives data on a socket with descriptor s and stores it in
   the buffer. When successful, the number of bytes of data received
   into the buffer is returned. The value 0 indicates that the
   connection is closed. The value -1 indicates an error. */
int XIOsocket::read( void* buffer, unsigned int size )
{
  unsigned int read = 0;
  DEBUGLOG2(("XIOsocket(%p{%d})::read(%p, %d)\n", this, s_handle, buffer, size));

  while( read < size )
  {
    #ifdef TCPV40HDRS
    // Work around for missing SO_RCVTIMEO in 16 bit IP stack.
    struct timeval timeout = {0};

    timeout.tv_sec = xio_socket_timeout();
    if ( timeout.tv_sec ) {
      fd_set waitlist;

      FD_ZERO( &waitlist    );
      FD_SET ( s_handle, &waitlist );

      switch ( select( s_handle+1, &waitlist, NULL, NULL, &timeout )) {
        case 0: // Timeout
          seterror(SOCETIMEDOUT);
          return -1;
        case -1: // Error
          seterror();
          return -1;
      }
    }
    #endif
    int done = recv( s_handle, (char*)buffer + read, size - read, 0 );
    DEBUGLOG2(("XIOsocket::read: %d, %d\n", done, sock_errno()));

    if( done < 0 ) {
      seterror();
      break;
    } else if( done == 0 ) {
      eof = true;
      break;
    } else {
      read += done;
    }
  }

  return read;
}

/* Receives bytes on a socket up to the first new-line character (\n)
   or until the number of bytes received is equal to n-1, whichever
   comes first. Stores the result in string and adds a null
   character (\0) to the end of the string. If n is equal to 1, the
   string is empty. Returns a pointer to the string buffer if successful.
   A NULL return value indicates an error or that the connection is
   closed.*/
char* XIOsocket::gets( char* buffer, unsigned int size )
{
  unsigned int done = 0;
  char* p  = buffer;
  DEBUGLOG(("XIOsocket(%p{%d})::gets(%p, %d)\n", this, s_handle, buffer, size));

  while( done < size - 1 ) {
    #ifdef TCPV40HDRS
    // Work around for missing SO_RCVTIMEO in 16 bit IP stack.
    struct timeval timeout = {0};

    timeout.tv_sec = xio_socket_timeout();
    if ( timeout.tv_sec ) {
      fd_set waitlist;

      FD_ZERO( &waitlist    );
      FD_SET ( s_handle, &waitlist );

      switch ( select( s_handle+1, &waitlist, NULL, NULL, &timeout )) {
        case 0: // Timeout
          seterror(SOCETIMEDOUT);
          return NULL;
        case -1: // Error
          seterror();
          return NULL;
      }
    }
    #endif
    int rc = recv( s_handle, p, 1, 0 );
    if( rc == 1 ) {
      if( *p == '\r' ) {
        continue;
      } else if( *p == '\n' ) {
        break;
      } else {
        ++p;
        ++done;
      }
    } else {
      DEBUGLOG(("XIOsocket::gets: error %d\n", sock_errno()));
      if( !done ) {
        if (rc == 0)
          eof = true;
        else
          seterror();
        return NULL;
      } else {
        break;
      }
    }
  }

  *p = 0;
  DEBUGLOG(("XIOsocket::gets: %s\n", buffer));
  return buffer;
}

/* Shuts down a socket and frees resources allocated to the socket.
   Retuns value 0 indicates success; the value -1 indicates an error. */
int XIOsocket::close() {
  DEBUGLOG(("XIOsocket(%p{%d})::close()\n", this, s_handle));
  int r = soclose( s_handle );
  s_handle = -1;
  if (r)
    seterror();
  return r;
}

long XIOsocket::tell(long* offset64)
{ if (offset64)
    *offset64 = -1L;
  return -1L;
}

long XIOsocket::seek(long offset, int origin, long* offset64)
{ errno = EINVAL;
  if (offset64)
    *offset64 = -1L;
  return -1L;
}

long XIOsocket::getsize(long* offset64)
{ errno = EINVAL;
  if (offset64)
    *offset64 = -1L;
  return -1L;
}

int XIOsocket::getstat(XSTAT* st)
{ errno = EINVAL;
  return -1;
}

int XIOsocket::chsize(long size, long offset64)
{ return -1;
}

XSFLAGS XIOsocket::supports() const
{ return XS_CAN_READ|XS_CAN_WRITE|XS_CAN_READWRITE;
}

XIOsocket::~XIOsocket()
{ if (s_handle != -1)
    close();
}
