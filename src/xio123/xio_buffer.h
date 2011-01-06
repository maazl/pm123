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

#ifndef XIO_BUFFER_H
#define XIO_BUFFER_H

#include "xio_protocol.h"
#include <string.h>

class XIObuffer : public XIOreadonly, protected XPROTOCOL::Iobserver
{private:
  // Observer class to handle meta info callbacks and delay them until the
  // appropriate part of the buffer is read.
  struct obs_entry
  { int         type;
    char*       metabuff;
    const long  pos;
    const long  pos64;
    obs_entry*  link;
    obs_entry(int type, const char* metabuff, long pos, long pos64);
    ~obs_entry() { free(metabuff); }
    char* detach() { char* ret = metabuff; metabuff = NULL; return ret; }
  };
 protected:
  XPROTOCOL* chain;        // C  Pointer to virtualized protocol

  char* head;              // C  Pointer to the first byte of the buffer.
  const unsigned int size; // C  Current size of the buffer.

  long  read_pos;          // M  Position of the logical read pointer in the associated file.

  // Entries for the observer
  obs_entry*   s_obs_head;
  obs_entry*   s_obs_tail;
  char  s_title[128];

 protected:
  // Clear the observer meta data buffer
  void         obs_clear();
  // Discard any observer entries up to file_pos
  void         obs_discard();
  // Execute the observer entries up to file_pos
  void         obs_execute();
  // Observer callback. Called from the function chain->read(). 
  virtual void metacallback(int type, const char* metabuff, long pos, long pos64);
  #ifdef DEBUG_LOG
  void         obs_dump() const;
  #endif

  // Core logic of seek. Supports only SEEK_SET.
  virtual long do_seek( long offset, long* offset64 ) = 0;
  
 public:
  XIObuffer(XPROTOCOL* chain, unsigned int buf_size);
  virtual bool init();
  virtual ~XIObuffer();
  virtual int open( const char* filename, XOFLAGS oflags );
  virtual int close();
  virtual long tell( long* offset64 = NULL );
  virtual long seek( long offset, int origin, long* offset64 = NULL );
  virtual long getsize( long* offset64 = NULL );
  virtual int chsize( long size, long offset64 = 0 ) = 0;
  virtual char* get_metainfo( int type, char* result, int size );
  virtual XSFLAGS supports() const;
};


inline XIObuffer::obs_entry::obs_entry(int type, const char* metabuff, long pos, long pos64)
: type(type),
  metabuff(strdup(metabuff)),
  pos(pos),
  pos64(pos64),
  link(NULL)
{}

#endif /* XIO_BUFFER_H */

