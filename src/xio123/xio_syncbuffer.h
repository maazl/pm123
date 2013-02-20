/*
 * Copyright 2008-2013 M.Mueller
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
  size_t  data_size; ///< Current size of the data in the buffer, &le; \c size.
  size_t  data_ptr;  ///< Current read position in the data buffer, &lt; \c size.
  size_t  data_unread;///< Unread part of the buffer data, &le; \c data_size.
 private:
  virtual int64_t do_seek(int64_t);
  
  /// @brief Load new data into the buffer. Return false on error.
  /// @details The function tries to load at least \a minsize bytes, but it succeeds also
  /// with less data if the underlying stream runs into EOF.
  /// \c data_size will tell you what has happened. If it is less than min
  /// EOF is reached.
  /// @pre \a minsize &isin [1 .. \c size] && \c data_unread == 0
  bool    fill_buffer(size_t minsize);

 public:
  /// Allocates and initializes the buffer.
  XIOsyncbuffer(XPROTOCOL* chain, unsigned int buf_size);
  //virtual bool init();
  //virtual ~XIOsyncbuffer();
  /// Reads count bytes from the file into \a buffer. Returns the number
  /// of bytes placed in result. The return value \c 0 indicates an attempt
  /// to read at end-of-file. A return value \c -1 indicates an error.
  /// @pre count > 0 && !eof && XO_READ
  virtual int read(void* result, unsigned int count);
  // more efficient implementation
  virtual char* gets(char* string, unsigned int n);
  virtual int close();
  virtual int chsize(int64_t size);
};

#endif /* XIO_SYNCBUFFER_H */

