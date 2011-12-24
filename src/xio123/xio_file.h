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

#ifndef XIO_FILE_H
#define XIO_FILE_H

#include "xio.h"
#include "xio_protocol.h"

class XIOfile : public XPROTOCOL
{private:
  HFILE s_handle;
  #ifdef XIO_SERIALIZE_DISK_IO
  bool  s_serialized;
  #endif

 public:
  /* Initializes the file protocol. */
  XIOfile();
  virtual ~XIOfile();
  virtual int open( const char* filename, XOFLAGS oflags );
  virtual int read( void* result, unsigned int count );
  virtual int write( const void* source, unsigned int count );
  virtual int close();
  virtual long tell( long* offset64 = NULL );
  virtual long seek( long offset, XIO_SEEK origin, long* offset64 = NULL );
  virtual long getsize( long* offset64 = NULL );
  virtual int getstat( XSTAT* st );
  virtual int chsize( long size, long offset64 = 0 );
  virtual XSFLAGS supports() const;
  virtual XIO_PROTOCOL protocol() const;
};

#endif /* XIO_FILE_H */

