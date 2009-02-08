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

#ifndef XIO_ASYNCBUFFER_H
#define XIO_ASYNCBUFFER_H

#include "xio_buffer.h"
#include <cpp/mutex.h>

class XIOasyncbuffer : public XIObuffer
{private:
  // Working set.
  // Member type: C  Constant during the operation of the buffer
  //              R  Read-ahead thread only, unsynchronized
  //              X  XIO interface only, unsynchronized
  //              Rx Read-ahead thread writes, XIO interface reads
  //              Xr XIO interface writes, read ahead thread reads
  //              M  all write, only reliable while mtx_access is locked  
  char* tail;           // C  Pointer to beyond of the last byte of the buffer.
  int   prefill;        // C  Start sending data not before this buffer level.
  int   free_size;      // M  Current size of the free part of the buffer.
  char* data_head;      // M  Pointer to the first byte of the data.
  char* data_tail;      // R  Pointer to beyond of the last byte of the data.
  char* data_read;      // Xr Current read position in the data pool.
  int   data_size;      // M  Current size of the data.
  int   data_rest;      // M  Current size of the data available for read.
  int   data_keep;      // C  Keep old data for fast reverse seek.
  int   tid;            // C  Read-ahead thread identifier.
  long  seekto;         // M  Seek command for the read-ahead thread.
  bool  data_end;       // Rx Read ahead thread reached the end of the data or a stream error.
  bool  end;            // Xr Stops read-ahead thread and cleanups the buffer.
  bool  boosted;        // M  The priority of the read-ahead thread is boosted.

  Mutex mtx_access;     // Serializes access to the buffer.
  Event evt_read_data;  // Sends if it is possible to read into the buffer.
  Event evt_have_data;  // Sends if the buffer have more data.

 private:
  /* Advances a buffer pointer. */
  char* advance( char* begin, int distance ) const;
  /* Boosts a priority of the read-ahead thread. */
  void  boost_priority();
  /* Normalizes a priority of the read-ahead thread. */
  void  normal_priority();
  /* Read-ahead thread. */
  void  read_ahead();
  friend void TFNENTRY buffer_read_ahead_stub( void* arg );

 protected:
  // Observer callback. Called from the function chain->read(). 
  virtual void observer_cb(const char* metabuff, long pos, long pos64);
  // Core logic of seek. Supports only SEEK_SET.
  virtual long do_seek( long offset, long* offset64 );

 public:
  XIOasyncbuffer(XPROTOCOL* chain, unsigned int buf_size, unsigned int block_size);
  virtual bool init();
  virtual ~XIOasyncbuffer();
  virtual int read( void* result, unsigned int count );
  //virtual char* gets( char* string, unsigned int n );
  virtual int close();
  virtual long getsize( long* offset64 = NULL );
  virtual int chsize( long size, long size64 = 0 );
};

#endif /* XIO_ASYNCBUFFER_H */

