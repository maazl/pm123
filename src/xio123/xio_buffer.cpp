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

#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <process.h>
#include <errno.h>

#include <utilfct.h>
#include <debuglog.h>

#include "xio_buffer.h"
#include "xio_protocol.h"

#define  BUFFER_IS_HUNGRY   30 // %
#define  BUFFER_IS_SATED    50 // %

/* Advances a buffer pointer. */
inline char*
advance( const XBUFFER* buffer, char* begin, int distance )
{
  if( distance < buffer->tail - begin ) {
    return begin + distance;
  } else {
    return begin + distance - buffer->size;
  }
}

/* Returns a distance between two buffer pointers. */
inline int
distance( const XBUFFER* buffer, char* begin, char* end )
{
  if( end >= begin ) {
    return end - begin;
  } else {
    return end - begin + buffer->size;
  }
}

/* Boosts a priority of the read-ahead thread. */
static void
buffer_boost_priority( XBUFFER* buffer )
{
  if( !buffer->boosted ) {
    DEBUGLOG(( "xio123: boosts priority of the read-ahead thread %d (buffer is %d bytes).\n", buffer->tid, buffer->data_rest ));
    DosSetPriority( PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, buffer->tid );
    buffer->boosted = 1;
  }
}

/* Normalizes a priority of the read-ahead thread. */
static void
buffer_normal_priority( XBUFFER* buffer )
{
  if( buffer->boosted ) {
    DEBUGLOG(( "xio123: normalizes priority of the read-ahead thread %d (buffer is %d bytes).\n", buffer->tid, buffer->data_rest ));
    DosSetPriority( PRTYS_THREAD, PRTYC_REGULAR, 0, buffer->tid );
    buffer->boosted = 0;
  }
}

/* Read-ahead thread. */
static void TFNENTRY
buffer_read_ahead( void* arg )
{
  XFILE*   x      = (XFILE*)arg;
  XBUFFER* buffer = x->buffer;
  ULONG    post_count;
  int      read_size;
  int      read_done;

  while( DosWaitEventSem( buffer->evt_read_data, SEM_INDEFINITE_WAIT ) == NO_ERROR && !buffer->end )
  {
    DosRequestMutexSem( buffer->mtx_file,   SEM_INDEFINITE_WAIT );
    DosRequestMutexSem( buffer->mtx_access, SEM_INDEFINITE_WAIT );

    if( buffer->free )
    {
      // Determines the maximum possible size of the continuous data chunk
      // for reading. The maximum size of a chunk is 4 kilobytes.
      read_size = distance( buffer, buffer->data_tail, buffer->tail );
      read_size = min( read_size, buffer->free );
      read_size = min( read_size, 16384 );

      DosReleaseMutexSem( buffer->mtx_access );
      // It is possible to use the data_tail pointer without request of
      // the mutex because only this thread can change it.
      read_done = x->protocol->read( x, buffer->data_tail, read_size );
      DosRequestMutexSem( buffer->mtx_access, SEM_INDEFINITE_WAIT );

      if( read_done >= 0 ) {
        buffer->data_size += read_done;
        buffer->free      -= read_done;
        buffer->data_rest += read_done;
        buffer->data_tail  = advance( buffer, buffer->data_tail, read_done );

        if( read_done < read_size ) {
          buffer->eof = 1;
        }
      } else {
        buffer->error = errno;
      }
    }

    DosPostEventSem( buffer->evt_have_data );

    if(( !buffer->free || buffer->error || buffer->eof ) && !buffer->end ) {
      // Buffer is filled up.
      DosResetEventSem( buffer->evt_read_data, &post_count );
    }


    if( buffer->boosted && ( buffer->data_rest >= buffer->size * BUFFER_IS_SATED / 100 ||
                             buffer->error || buffer->eof ))
    {
      // If buffer is sated, normalizes of the priority of the read-ahead thread.
      buffer_normal_priority( buffer );
    }

    DosReleaseMutexSem( buffer->mtx_access );
    DosReleaseMutexSem( buffer->mtx_file   );
  }

  _endthread();
}

/* Resets the buffer. */
static void
buffer_reset( XFILE* x )
{
  XBUFFER* buffer = x->buffer;

  buffer->tail       = buffer->head + buffer->size;
  buffer->free       = buffer->size;
  buffer->data_head  = buffer->head;
  buffer->data_read  = buffer->head;
  buffer->data_tail  = buffer->head;
  buffer->data_size  = 0;
  buffer->data_rest  = 0;
  buffer->data_keep  = buffer->size / 5;
  buffer->file_pos   = x->protocol->tell( x );
  buffer->eof        = 0;
  buffer->error      = 0;
}

/* Allocates and initializes the buffer. */
void
buffer_initialize( XFILE* x )
{
  XBUFFER* buffer;
  int size = xio_buffer_size();

  if( !size || ( x->oflags & XO_WRITE ) || ( x->protocol->supports & XS_NOT_BUFFERIZE )) {
    x->buffer = NULL;
    return;
  }

  buffer = (XBUFFER*)calloc( 1, sizeof( XBUFFER ));
  if( !buffer ) {
    x->buffer = NULL;
    return;
  }

  x->buffer = buffer;
  buffer->tid = -1;

  for(;;)
  {
    buffer->head = (char*)malloc( size );

    DosCreateMutexSem( NULL, &buffer->mtx_file,      0, FALSE );
    DosCreateMutexSem( NULL, &buffer->mtx_access,    0, FALSE );
    DosCreateEventSem( NULL, &buffer->evt_have_data, 0, FALSE );
    DosCreateEventSem( NULL, &buffer->evt_read_data, 0, FALSE );

    if( !buffer->head          ||
        !buffer->mtx_access    ||
        !buffer->mtx_file      ||
        !buffer->evt_read_data ||
        !buffer->evt_have_data )
    {
      break;
    }

    buffer->size = size;
    buffer_reset( x );

    if( xio_buffer_wait())
    {
      int prefill = buffer->size * xio_buffer_fill() / 100;

      if( prefill > 0 ) {
        buffer->data_size = x->protocol->read( x, buffer->head, prefill );

        if( buffer->data_size < 0 ) {
          break;
        }
        if( buffer->data_size < prefill ) {
          buffer->eof = 1;
        }

        buffer->data_rest = buffer->data_size;
        buffer->data_tail = advance( buffer, buffer->data_head, buffer->data_size );
        buffer->free      = buffer->size - buffer->data_size;
      }
    }

    if(( buffer->tid = _beginthread( buffer_read_ahead, NULL, 65535, x )) == -1 ) {
      break;
    }

    if( buffer->free && !buffer->eof ) {
      // If buffer is not filled up, posts a notify to the read-ahead thread.
      DosPostEventSem( buffer->evt_read_data );
    }
    if( buffer->data_size == 0 ) {
      // If buffer is empty, boosts of the priority of the read-ahead thread.
      buffer_boost_priority( buffer );
    }

    return;
  }

  x->buffer = NULL;
  buffer_terminate( x );
}

/* Cleanups the buffer. */
void buffer_terminate( XFILE* x )
{
  XBUFFER* buffer = x->buffer;

  if( buffer )
  {
    // If the thread is started, set request about its termination.
    if( buffer->tid != -1 ) {
      DosRequestMutexSem( buffer->mtx_access, SEM_INDEFINITE_WAIT );
      buffer->end = 1;
      DosPostEventSem( buffer->evt_read_data );
      DosReleaseMutexSem( buffer->mtx_access );
      DosWaitThread((PULONG)&buffer->tid, DCWW_WAIT );
    }

    // Cleanups.
    if( buffer->mtx_access ) {
      DosCloseMutexSem( buffer->mtx_access );
    }
    if( buffer->mtx_file ) {
      DosCloseMutexSem( buffer->mtx_file );
    }
    if( buffer->evt_read_data ) {
      DosCloseEventSem( buffer->evt_read_data );
    }
    if( buffer->evt_have_data ) {
      DosCloseEventSem( buffer->evt_have_data );
    }
    if( buffer->head ) {
      free( buffer->head );
    }
    if( x->protocol ) {
      x->protocol->clean( x );
    }
    free( buffer );
    free( x );
  }
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error. */
int buffer_read( XFILE* x, void* result, unsigned int count )
{
  int   obsolete;
  ULONG post_count;
  int   read_done;
  int   read_size;

  XBUFFER* buffer = x->buffer;

  if( !buffer ) {
    return x->protocol->read( x, result, count );
  }

  for( read_done = 0; read_done < count; )
  {
    DosRequestMutexSem( buffer->mtx_access, SEM_INDEFINITE_WAIT );

    // Determines the maximum possible size of the continuous data
    // chunk for reading.
    read_size = distance( buffer, buffer->data_read, buffer->tail );
    read_size = min( read_size, buffer->data_rest );
    read_size = min( read_size, count - read_done );

    if( read_size )
    {
      // Copy a next chunk of data in the result buffer.
      memcpy( (char*)result + read_done, buffer->data_read, read_size );

      buffer->data_read  = advance( buffer, buffer->data_read, read_size );
      buffer->data_rest -= read_size;
      read_done += read_size;

      obsolete = buffer->data_size - buffer->data_rest - buffer->data_keep;

      if( obsolete > 128 ) {
        // If there is too much obsolete data then move a head of
        // the data pool.
        buffer->data_head  = advance( buffer, buffer->data_head, obsolete );
        buffer->file_pos  += obsolete;
        buffer->data_size -= obsolete;
        buffer->free      += obsolete;

        DosPostEventSem( buffer->evt_read_data );
      }

      if( !buffer->boosted &&  buffer->data_rest < buffer->size * BUFFER_IS_HUNGRY / 100
                           && !buffer->error && !buffer->eof )
      {
        // If buffer is hungry, boosts of the priority of the read-ahead thread.
        buffer_boost_priority( buffer );
      }

      DosReleaseMutexSem( buffer->mtx_access );
    }
    else
    {
      if( buffer->eof   ) {
        DosReleaseMutexSem( buffer->mtx_access );
        return read_done;
      }
      if( buffer->error ) {
        errno = buffer->error;
        DosReleaseMutexSem( buffer->mtx_access );
        return -1;
      }

      // There is no error and there is no end of a file.
      // It is necessary to wait a next portion of data.
      DosResetEventSem( buffer->evt_have_data, &post_count );
      DosPostEventSem ( buffer->evt_read_data );

      // Boosts of the priority of the read-ahead thread.
      buffer_boost_priority( buffer );
      DosReleaseMutexSem( buffer->mtx_access );

      // Wait a next portion of data.
      DosWaitEventSem( buffer->evt_have_data, SEM_INDEFINITE_WAIT );
    }
  }

  return read_done;
}

/* Writes count bytes from source into the file. Returns the number
   of bytes moved from the source to the file. The return value may
   be positive but less than count. A return value of -1 indicates an
   error */
int
buffer_write( XFILE* x, const void* source, unsigned int count )
{
  if( x->buffer ) {
    errno = EINVAL;
    return -1;
  } else {
    return x->protocol->write( x, source, count );
  }
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
long
buffer_tell( XFILE* x )
{
  if( x->buffer )
  {
    long pos;

    DosRequestMutexSem( x->buffer->mtx_access, SEM_INDEFINITE_WAIT );
    pos = x->buffer->file_pos + x->buffer->data_size
                              - x->buffer->data_rest;
    DosReleaseMutexSem( x->buffer->mtx_access );
    return pos;
  } else {
    return x->protocol->tell( x );
  }
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long
buffer_filesize( XFILE* x ) {
  return x->protocol->size( x );
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
long
buffer_seek( XFILE* x, long offset, int origin )
{
  XBUFFER* buffer = x->buffer;
  long result;

  if( !buffer ) {
    return x->protocol->seek( x, offset, origin );
  }

  DosRequestMutexSem( buffer->mtx_access, SEM_INDEFINITE_WAIT );

  switch( origin ) {
    case XIO_SEEK_SET:
      result = offset;
      break;
    case XIO_SEEK_CUR:
      result = buffer_tell( x ) + offset;
      break;
    case XIO_SEEK_END:
      result = buffer_filesize( x ) + offset;
      break;
    default:
      DosReleaseMutexSem( buffer->mtx_access );
      return -1;
  }

  if( result >= buffer->file_pos &&
      result <= buffer->file_pos + buffer->data_size )
  {
    int obsolete;
    int moveto = result - buffer->file_pos;

    buffer->data_read = advance( buffer, buffer->data_head, moveto );
    buffer->data_rest = buffer->data_size - moveto;
    obsolete = moveto - buffer->data_keep;

    if( obsolete > 128 ) {
      // If there is too much obsolete data then move a head of
      // the data pool.
      buffer->data_head  = advance( buffer, buffer->data_head, obsolete );
      buffer->file_pos  += obsolete;
      buffer->data_size -= obsolete;
      buffer->free      += obsolete;

      DosPostEventSem( buffer->evt_read_data );
    }
  } else {
    DosReleaseMutexSem( buffer->mtx_access );
    DosRequestMutexSem( buffer->mtx_file,   SEM_INDEFINITE_WAIT );
    DosRequestMutexSem( buffer->mtx_access, SEM_INDEFINITE_WAIT );

    result = x->protocol->seek( x, result, XIO_SEEK_SET );

    if( result >= 0 ) {
      buffer->file_pos  = result;
      buffer->data_head = buffer->head;
      buffer->data_tail = buffer->head;
      buffer->data_read = buffer->head;
      buffer->data_size = 0;
      buffer->data_rest = 0;
      buffer->free      = buffer->size;
      buffer->eof       = 0;
      buffer->error     = 0;

      DosPostEventSem( buffer->evt_read_data );

      // Boosts of the priority of the read-ahead thread.
      buffer_boost_priority( buffer );
    }
    DosReleaseMutexSem( buffer->mtx_file   );
  }

  DosReleaseMutexSem( buffer->mtx_access );
  return result;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
int
buffer_truncate( XFILE* x, long size )
{
  XBUFFER* buffer = x->buffer;
  int rc;

  if( !buffer ) {
    return x->protocol->chsize( x, size );
  }

  DosRequestMutexSem( buffer->mtx_file,   SEM_INDEFINITE_WAIT );
  DosRequestMutexSem( buffer->mtx_access, SEM_INDEFINITE_WAIT );

  rc = x->protocol->chsize( x, size );
  buffer_reset( x );

  DosReleaseMutexSem( buffer->mtx_access );
  DosReleaseMutexSem( buffer->mtx_file   );
  return rc;
}

