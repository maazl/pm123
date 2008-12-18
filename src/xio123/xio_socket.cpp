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

/* Converts a string containing a valid internet address using
   dotted-decimal notation or host name into an internet address
   number typed as an unsigned long value.  A -1 value
   indicates an error. */
u_long
so_get_address( const char* hostname )
{
  u_long address;
  struct hostent* entry;

  if( !hostname ) {
    errno = HBASEERR + HOST_NOT_FOUND;
    return -1;
  }

  if(( address = inet_addr( (char*)hostname )) == -1 ) {
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

/* Base64 encoding. */
char*
so_base64_encode( const char* src )
{
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  char*  str;
  char*  dst;
  size_t l;
  int    t;
  int    r;

  l = strlen( src );
  if(( str = (char*)malloc((( l + 2 ) / 3 ) * 4 + 1 )) == NULL ) {
    return NULL;
  }
  dst = str;
  r = 0;

  while( l >= 3 ) {
    t = ( src[0] << 16 ) | ( src[1] << 8 ) | src[2];
    dst[0] = base64[(t >> 18) & 0x3f];
    dst[1] = base64[(t >> 12) & 0x3f];
    dst[2] = base64[(t >>  6) & 0x3f];
    dst[3] = base64[(t >>  0) & 0x3f];
    src += 3; l -= 3;
    dst += 4; r += 4;
  }

  switch( l ) {
    case 2:
      t = ( src[0] << 16 ) | ( src[1] << 8 );
      dst[0] = base64[(t >> 18) & 0x3f];
      dst[1] = base64[(t >> 12) & 0x3f];
      dst[2] = base64[(t >>  6) & 0x3f];
      dst[3] = '=';
      dst += 4;
      r += 4;
      break;
    case 1:
      t = src[0] << 16;
      dst[0] = base64[(t >> 18) & 0x3f];
      dst[1] = base64[(t >> 12) & 0x3f];
      dst[2] = dst[3] = '=';
      dst += 4;
      r += 4;
      break;
    case 0:
      break;
  }

  *dst = 0;
  return str;
}

/* Creates an endpoint for communication and requests a
   connection to a remote host. A non-negative socket descriptor
   return value indicates success. The return value -1 indicates
   an error. */
int
so_connect( u_long address, int port )
{
  int s;
  struct sockaddr_in server = {0};

  if( address == -1 ) {
    return -1;
  }

  if(( s = socket( PF_INET, SOCK_STREAM, 0 )) != -1 )
  {
    #ifdef NONBLOCKED_CONNECT
    int dontblock;
    #endif

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = address;
    server.sin_port = htons( port );

    #ifdef NONBLOCKED_CONNECT

      dontblock = 1;
      ioctl( s, FIONBIO, (char*)&dontblock, sizeof( dontblock ));

      if( connect( s, (struct sockaddr*)&server, sizeof( server )) != -1 ||
          sock_errno() == SOCEINPROGRESS )
      {
        struct timeval timeout = {0};
        fd_set waitlist;

        timeout.tv_sec = xio_connect_timeout();

        FD_ZERO( &waitlist    );
        FD_SET ( s, &waitlist );

        if( select( s + 1, NULL, &waitlist, NULL, &timeout ) <= 0 ) {
          errno = sock_errno();
          soclose( s );
          s = -1;
        } else {
          dontblock = 0;
          ioctl( s, FIONBIO, (char*)&dontblock, sizeof( dontblock ));
        }
      } else {
        errno = sock_errno();
        soclose( s );
        s = -1;
      }
    #else
      if( connect( s, (struct sockaddr*)&server, sizeof( server )) == -1 ) {
        errno = sock_errno();
        soclose( s );
        s = -1;
      }
    #endif
  } else {
    errno = sock_errno();
  }

  return s;
}

/* Sends data on a connected socket. When successful, returns 0.
   The return value -1 indicates an error was detected on the
   sending side of the connection. */
int
so_write( int s, const void* buffer, int size )
{
  while( size )
  {
    int done = send( s, (char*)buffer, size, 0 );

    if( done <= 0 ) {
      errno = sock_errno();
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
int
so_read( int s, void* buffer, int size )
{
  int read = 0;
  int done;

  while( read < size )
  {
    done = recv( s, (char*)buffer + read, size - read, 0 );

    if( done < 0 ) {
      errno = sock_errno();
      break;
    } else if( done == 0 ) {
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
char*
so_readline( int s, char* buffer, int size )
{
  int done = 0;
  char* p  = buffer;

  while( done < size - 1 ) {
    if( recv( s, p, 1, 0 ) == 1 ) {
      if( *p == '\r' ) {
        continue;
      } else if( *p == '\n' ) {
        break;
      } else {
        ++p;
        ++done;
      }
    } else {
      if( !done ) {
        return NULL;
      } else {
        break;
      }
    }
  }

  *p = 0;
  return buffer;
}

/* Shuts down a socket and frees resources allocated to the socket.
   Retuns value 0 indicates success; the value -1 indicates an error. */
int
so_close( int s ) {
  return soclose( s );
}

