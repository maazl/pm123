/*
 * Copyright 2008-2011 M.Mueller
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

#ifndef XIO_SOCKET_H
#define XIO_SOCKET_H

#include "xio_protocol.h"

#include <types.h>

#ifndef HBASEERR
#define HBASEERR 20000
#endif


/* Class to deal with a TCP/IP socket transparent as stream.
 * The URI Syntax is: tcpip://host:port
 */
class XIOsocket : public XPROTOCOL
{private:
  int             s_handle;

 private:
  void            seterror(int err);
  void            seterror();

 public:
                  XIOsocket() : s_handle(-1) {};
  virtual         ~XIOsocket();

  // Opens a client connection. URI syntax tcpip://host:port
  // Returns 0 if it successfully connected the socket.
  // A return value of -1 shows an error.
  virtual int     open( const char* uri, XOFLAGS oflags );
  // Creates an endpoint for communication and requests a
  // connection to a remote host. A non-zero return value indicates
  // an error.
  int             open( u_long address, int port );
  // Receives data on a socket with descriptor s and stores it in
  // the buffer. When successful, the number of bytes of data received
  // into the buffer is returned. The value 0 indicates that the
  // connection is closed. The value -1 indicates an error.
  virtual int     read( void* result, unsigned int count );
  // Receives bytes on a socket up to the first new-line character (\n)
  // or until the number of bytes received is equal to n-1, whichever
  // comes first. Stores the result in string and adds a null
  // character (\0) to the end of the string. If n is equal to 1, the
  // string is empty. Returns a pointer to the string buffer if successful.
  // A NULL return value indicates an error or that the connection is
  // closed.
  virtual char*   gets( char* string, unsigned int n ); // ???
  // Sends data on a connected socket. When successful, returns 0.
  // The return value -1 indicates an error was detected on the
  // sending side of the connection.
  virtual int     write( const void* source, unsigned int count );
  // Shuts down a socket and frees resources allocated to the socket.
  // Retuns value 0 indicates success; the value -1 indicates an error.
  virtual int     close();
  // Unsupported. Always returns an error (-1L).
  virtual long    tell( long* offset64 = NULL );
  // Unsupported. Always returns an error (-1L).
  virtual long    seek( long offset, int origin, long* offset64 = NULL );
  // Unsupported. Always returns an error (-1L).
  virtual long    getsize( long* offset64 = NULL );
  // Unsupported. Always returns an error (-1).
  virtual int     getstat( _XSTAT* st );
  // Unsupported. Always returns an error (-1).
  virtual int     chsize( long size, long offset64 = 0 );
  // Return the supported properties of the current protocol
  virtual XSFLAGS supports() const;

  // Converts a string containing a valid internet address using
  // dotted-decimal notation or host name into an internet address
  // number typed as an unsigned long value.  A -1 value
  // indicates an error.
  static u_long   get_address(const char* hostname);
  // Converts a string containing a valid service or port
  // into a port number. A -1 value indicates an error.
  static int      get_service(const char* service);
};

#endif /* XIO_SOCKET_H */

