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

#include "xio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XBUFFER   {

  char* head;           /* Pointer to the first byte of the buffer.          */
  char* tail;           /* Pointer to beyond of the last byte of the buffer. */
  int   size;           /* Current size of the buffer.                       */
  int   free;           /* Current size of the free part of the buffer.      */
  char* data_head;      /* Pointer to the first byte of the data.            */
  char* data_tail;      /* Pointer to beyond of the last byte of the data.   */
  char* data_read;      /* Current read position in the data pool.           */
  int   data_size;      /* Current size of the data.                         */
  int   data_rest;      /* Current size of the data available for read.      */
  int   data_keep;      /* Maximum size of the obsolete data.                */
  int   tid;            /* Read-ahead thread identifier.                     */
  int   end;            /* Stops read-ahead thread and cleanups the buffer.  */
  int   boosted;        /* The priority of the read-ahead thread is boosted. */
  long  file_pos;       /* Position of the data pool in the associated file. */

  HMTX  mtx_access;     /* Serializes access to the buffer.                  */
  HMTX  mtx_file;       /* Serializes access to the associated file.         */
  HEV   evt_read_data;  /* Sends if it is possible to read into the buffer.  */
  HEV   evt_have_data;  /* Sends if the buffer have more data.               */
  int   eof;            /* Indicates whether the end of file is reached.     */
  int   error;          /* Last error code.                                  */

} XBUFFER;

/* Allocates and initializes the buffer. */
void buffer_initialize( XFILE* x );
/* Cleanups the buffer. */
void buffer_terminate( XBUFFER* buffer );

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
int  buffer_read( XFILE* x, char* result, unsigned int count );

/* Writes count bytes from source into the file. Returns the number
   of bytes moved from the source to the file. The return value may
   be positive but less than count. A return value of -1 indicates an
   error */
int  buffer_write( XFILE* x, const char* source, unsigned int count );

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
long buffer_tell( XFILE* x );

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
long buffer_seek( XFILE* x, long offset, int origin );

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long buffer_filesize( XFILE* x );

#ifdef __cplusplus
}
#endif
#endif /* XIO_BUFFER_H */

