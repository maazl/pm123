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

#ifndef XIO_SYNCBUFFER_H
#define XIO_SYNCBUFFER_H

#include "xio_buffer.h"

class XIOsyncbuffer : public XIObuffer
{private:
  unsigned int data_size; ///< Current size of the data in the buffer.
  unsigned int data_read; ///< Current read position in the data buffer.
                          ///< Note that data_read can be larger that data_size
                          ///< if the file was recently shrunken.
 private:
  virtual long do_seek( long offset, long* offset64 );
  
  // Load new data into the buffer. Return false on error.
  // The function tries to load the entire buffer size, but it succeeds also
  // with less data if the underlying stream runs into EOF.
  // data_size will tell you what has happened. If it is less than size
  // EOF is reached.
  bool    fill_buffer();

 public:
  XIOsyncbuffer(XPROTOCOL* chain, unsigned int buf_size);
  //virtual bool init();
  //virtual ~XIOsyncbuffer();
  virtual int read( void* result, unsigned int count );
  // more efficient implementation
  virtual char* gets( char* string, unsigned int n );
  virtual int close();
  virtual int chsize( long size, long offset64 = 0 );
};

#endif /* XIO_SYNCBUFFER_H */

