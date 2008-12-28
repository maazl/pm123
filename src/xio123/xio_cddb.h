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

#ifndef XIO_CDDB_H
#define XIO_CDDB_H

#include "xio.h"
#include "xio_protocol.h"

/* The server command must be sent as part of the "Request-URL":
 *
 * cmd=server+command&hello=joe+my.host.com+clientname+version&proto=6
 *
 * Where the text following the "cmd=" represents the CDDBP command to be
 * executed, the text following the "hello=" represents the arguments to
 * the "cddb hello" command that is implied by this operation, and the
 * text following the "proto=" represents the argument to the "proto" command
 * that is implied by this operation.
 *
 * The "+" characters in the input represent spaces, and will be translated
 * by the server before performing the request. Special characters may be
 * represented by the sequence "%XX" where "XX" is a two-digit hex number
 * corresponding to the ASCII (ISO-8859-1) sequence of that character. The
 * "&" characters denote separations between the command, hello and proto
 * arguments. Newlines and carriage returns must not appear anywhere in the
 * string except at the end.
 *
 * All CDDBP commands are supported, except for "cddb hello", "cddb write",
 * "proto" and "quit".
 */

#ifndef CDDBBASEERR
#define CDDBBASEERR 23000
#endif

#define CDDB_OK             200 /* OK, read/write allowed.                       */
#define CDDB_OK_READONLY    201 /* OK, read only.                                */
#define CDDB_PROTO_OK       201 /* OK, protocol version now: cur_level.          */
#define CDDB_NO_MATCH       202 /* No match found.                               */
#define CDDB_FOUND_EXACT    210 /* Found exact matches, list follows.            */
#define CDDB_FOUND_INEXACT  211 /* Found inexact matches, list follows.          */
#define CDDB_SITES_OK       210 /* Ok, site information follows.                 */
#define CDDB_SITES_ERROR    401 /* No site information available.                */
#define CDDB_SHOOK_ALREADY  402 /* Already shook hands.                          */
#define CDDB_CORRUPT        403 /* Database entry is corrupt.                    */
#define CDDB_NO_HANDSHAKE   409 /* No handshake.                                 */
#define CDDB_BAD_HANDSHAKE  431 /* Handshake not successful, closing connection. */
#define CDDB_DENIED         432 /* No connections allowed: permission denied.    */
#define CDDB_TOO_MANY_USERS 433 /* No connections allowed: too many users.       */
#define CDDB_OVERLOAD       434 /* No connections allowed: system load too high. */
#define CDDB_PROTO_ILLEGAL  501 /* Illegal protocol level.                       */
#define CDDB_PROTO_ALREADY  502 /* Protocol level already cur_level.             */
#define CDDB_PROTOCOL_ERROR 999

class XIOcddb : public XIOreadonly
{private:
  int           s_handle;   /* Connection handle.                            */
  unsigned long s_pos;      /* Current position of the stream pointer.       */

 private:
  int read_reply();
  int send_command( const char* format, ... );

 public:
  /* Initializes the cddb protocol. */
  XIOcddb::XIOcddb();
  virtual ~XIOcddb();
  virtual int open( const char* filename, XOFLAGS oflags );
  virtual int read( void* result, unsigned int count );
  virtual int close();
  virtual long tell( long* offset64 = NULL );
  virtual long seek( long offset, int origin, long* offset64 = NULL );
  virtual long getsize( long* offset64 = NULL );
  virtual XSFLAGS supports() const;

  /* Maps the error number in errnum to an error message string. */
  static const char* strerror( int errnum );
};

#endif /* XIO_CDDB_H */

