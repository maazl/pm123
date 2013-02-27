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

#define  INCL_DOS
#define  INCL_ERRORS
#include <stdlib.h>
#include <memory.h>
#include <process.h>
#include <errno.h>
#include <string.h>

#include <utilfct.h>
#include <debuglog.h>

#include "xio_protocol.h"
#include "xio_asyncbuffer.h"
#include "xio.h"
#include <os2.h>


#define  BUFFER_IS_HUNGRY   30 // %
#define  BUFFER_IS_SATED    50 // %


/* Advances a buffer pointer. */
inline char* XIOasyncbuffer::advance(char* begin, unsigned distance) const
{ begin += distance;
  if (begin >= tail)
    begin -= size;
  return begin;
}
/* Advances a buffer pointer. */
inline char* XIOasyncbuffer::move(char* begin, int distance) const
{ begin += distance;
  if (begin >= tail)
    begin -= size;
  else if (begin < head)
    begin += size;
  return begin;
}

/* Boosts a priority of the read-ahead thread. */
void XIOasyncbuffer::boost_priority()
{ if (!boosted)
  { DEBUGLOG(("xio123:XIOasyncbuffer(%p)::boost_priority() - thread %d (buffer is %d bytes).\n", this, tid, data_rest));
    DosSetPriority(PRTYS_THREAD, PRTYC_FOREGROUNDSERVER, +19, tid);
    boosted = 1;
  }
}

/* Normalizes a priority of the read-ahead thread. */
void XIOasyncbuffer::normal_priority()
{ if (boosted)
  { DEBUGLOG(("xio123:XIOasyncbuffer(%p)::normal_priority() - thread %d (buffer is %d bytes).\n", this, tid, data_rest));
    DosSetPriority(PRTYS_THREAD, PRTYC_REGULAR, -19, tid);
    boosted = 0;
  }
}

/* Read-ahead thread. */
void XIOasyncbuffer::read_ahead()
{
  while (evt_read_data.Wait() && !end)
  {
    Mutex::Lock lock(mtx_access);

    if (seekto != -1L)
    { // Seek command (implies trashing the buffer)
      int64_t do_seek_to = seekto;

      lock.Release();
      int64_t rc = chain->seek(do_seek_to, XIO_SEEK_SET);
      DEBUGLOG(("XIOasyncbuffer::read_ahead: seek to %lli -> %lli\n", do_seek_to, rc));
      lock.Request();
      
      if (rc == -1L)
        error = errno;
      else
      { data_tail = data_read = data_head = head;
        data_rest = data_size = 0;
      }
      // Double check: an additional seek command could have arrived.
      if (seekto == do_seek_to)
        seekto = -1L;
      
    } else if (data_size < size)
    {
      // Determines the maximum possible size of the continuous data chunk
      // for reading.
      int read_size = (data_head > data_tail ? data_head : tail) - data_tail;
      if (read_size > blocksize)
        read_size = blocksize;
      ASSERT(read_size);

      lock.Release();
      // It is possible to use the data_tail pointer without request of
      // the mutex because only this thread can change it.
      int read_done = chain->read( data_tail, read_size );
      DEBUGLOG(("XIOasyncbuffer::read_ahead: read %i -> %i, %u,%d\n", read_size, read_done, chain->eof, chain->error));
      lock.Request();

      if (read_done > 0)
      { data_size += read_done;
        data_rest += read_done;
        data_tail  = advance(data_tail, read_done);

        if (seekto != -1L)
        { // Seek arrived meanwhile
          // Check whether the target is within the new buffer.
          int64_t file_pos = read_pos - (data_size - data_rest);
          if (seekto >= file_pos && seekto <= file_pos + data_size)
          { // Buffer seek
            DEBUGLOG(("XIOasyncbuffer::read_ahead: seek buffer %lli \n", seekto));
            int dist = seekto - file_pos;
            data_read = move(data_head, dist);
            data_rest = data_size - dist;
            seekto = -1L;
          } else
          { // Real seek required => discard buffer
            data_tail = data_read = data_head = head;
            data_rest = data_size = 0;
          }
        }
      }
    }

    data_end = chain->error || chain->eof;

    if (data_end || data_size >= prefill)
      evt_have_data.Set();

    if ((data_size == size || data_end) && !end)
      // Buffer is filled up or eof.
      evt_read_data.Reset();

    if (boosted && (data_end || data_rest >= size * BUFFER_IS_SATED / 100))
      // If buffer is sated, normalizes of the priority of the read-ahead thread.
      normal_priority();
  }

}

/* Read-ahead thread stub. */
void TFNENTRY buffer_read_ahead_stub(void* arg)
{ ((XIOasyncbuffer*)arg)->read_ahead();
}

/* Allocates and initializes the buffer. */
XIOasyncbuffer::XIOasyncbuffer(XPROTOCOL* chain, unsigned int buf_size)
: XIObuffer(chain, buf_size), 
  tail(head + size),
  prefill(0),
  data_head(head),
  data_tail(head),
  data_read(head),
  data_size(0),
  data_rest(0),
  data_keep(size / 5),
  tid(-1),
  seekto(-1),
  data_end(false),
  end(false),
  boosted(false)
{}

bool XIOasyncbuffer::init()
{
  if( xio_buffer_wait())
    prefill = size * xio_buffer_fill() / 100;
/*  if( xio_buffer_wait())
  {
    int prefill = size * xio_buffer_fill() / 100;

    if( prefill > 0 ) {
      data_size = chain->read( head, prefill );

      if( data_size < 0 )
        return false;
      else if (data_size == 0)
        eof = true;

      data_rest = data_size;
      data_tail = advance( data_head, data_size );
      free_size = size - data_size;
    }
  }*/

  if ((tid = _beginthread(buffer_read_ahead_stub, NULL, 65535, this)) == -1)
    return false;

  if (data_size < size && !eof)
    // If buffer is not filled up, posts a notify to the read-ahead thread.
    evt_read_data.Set();

  if (data_size == 0)
    // If buffer is empty, boosts of the priority of the read-ahead thread.
    boost_priority();

  return XIObuffer::init();
}

/* Cleanups the buffer. */
XIOasyncbuffer::~XIOasyncbuffer()
{ // If the thread is started, set request about its termination.
  if (tid != -1)
  { { Mutex::Lock lock(mtx_access);
      end = true;
      evt_read_data.Set();
    }
    DosWaitThread((PULONG)&tid, DCWW_WAIT);
    tid = -1;
  }
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error. */
int XIOasyncbuffer::read(void* result, unsigned int count)
{
  int read_done = 0;
  { Mutex::Lock lock(mtx_access);
    while (read_done < count)
    {
      // Determines the maximum possible size of the continuous data
      // chunk for reading.
      int read_size = tail - data_read;
      if (read_size > data_rest)
        read_size = data_rest;
      if (read_size > count - read_done)
        read_size = count - read_done;

      if (read_size)
      { DEBUGLOG(("XIOasyncbuffer::read: from buffer %i\n", read_size));
        // Copy a next chunk of data in the result buffer.
        memcpy((char*)result + read_done, data_read, read_size);

        data_read  = advance(data_read, read_size);
        data_rest -= read_size;
        read_done += read_size;

        int obsolete = data_size - data_rest - data_keep;

        if (obsolete > 512)
        { // If there is too much obsolete data then move a head of the data pool.
          data_head  = advance( data_head, obsolete );
          data_size -= obsolete;

          evt_read_data.Set();
        }

        if ( !boosted && data_rest < size * BUFFER_IS_HUNGRY / 100
                      && !error && !eof )
          // If buffer is hungry, boosts of the priority of the read-ahead thread.
          boost_priority();

      } else
      { DEBUGLOG(("XIOasyncbuffer::read: look for more data %u\n", data_end));
        if (data_end) // Read ahead thread cannot read more
        { if (chain->eof)
          { eof = true;
            break;
          }
          if (chain->error)
          { obs_clear();
            errno = error = chain->error;
            return -1;
          }
        }
        // There is no error and there is no end of a file.
        // It is necessary to wait a next portion of data.
        evt_have_data.Reset();
        evt_read_data.Set();

        // Boosts of the priority of the read-ahead thread.
        boost_priority();
        lock.Release();
        // Wait a next portion of data.
        evt_have_data.Wait();
        lock.Request();
      }
    }
  }
  read_pos += read_done;
  obs_execute();
  return read_done;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
int XIOasyncbuffer::close()
{ // Stop Read-Ahead thread
  if (tid != -1)
  { { Mutex::Lock lock(mtx_access);
      end = true;
      evt_read_data.Set();
    }
    DosWaitThread((PULONG)&tid, DCWW_WAIT);
    tid = -1;
  }
  return XIObuffer::close();
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
int64_t XIOasyncbuffer::do_seek(int64_t offset)
{
  Mutex::Lock lock(mtx_access);
  // reset errors
  eof    = false;
  error  = 0;

  int64_t file_pos = read_pos - (data_size - data_rest);
  DEBUGLOG(("XIOasyncbuffer::do_seek: %lli, [%lli, %lli), %i\n", offset, file_pos, file_pos + data_size, data_rest));
  if (offset >= file_pos && offset <= file_pos + data_size)
  { int moveto = offset - file_pos;
    data_read = move(data_head, moveto);
    data_rest = data_size - moveto;

    int obsolete = data_size - data_rest - data_keep;
    DEBUGLOG(("XIOasyncbuffer::do_seek: obs = %i\n", obsolete));

    if (obsolete > 512)
    { // If there is too much obsolete data then move a head of the data pool.
      data_head  = advance(data_head, obsolete);
      data_size -= obsolete;

      obs_clear();
      evt_read_data.Set();
    } else
      obs_discard();

  } else {
    DEBUGLOG(("XIOasyncbuffer::do_seek: -> RA-thread\n"));
    obs_clear();
    seekto = offset;
    evt_read_data.Set();
    evt_have_data.Reset();
    // Boosts of the priority of the read-ahead thread.
    boost_priority();
    // Wait for the acknowledge of the read-ahead thread.
    lock.Release();
    evt_have_data.Wait();
    DEBUGLOG(("XIOasyncbuffer::do_seek: ack\n"));
    
    if (error)
      return -1L; 
  }

  return read_pos = offset;
}

int64_t XIOasyncbuffer::getsize()
{ int64_t ret = chain->getsize();
  if (eof && ret > read_pos)
  { Mutex::Lock lock(mtx_access);
    if (eof && ret > read_pos)
    { eof = false;
      evt_read_data.Set();
  } }
  return ret;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
int XIOasyncbuffer::chsize(int64_t size)
{
  Mutex::Lock lock(mtx_access);
  
  int rc = XIObuffer::chsize(size);
  if (rc != -1)
    // TODO: we should operate smarter here.
    // Trash buffer   
    seekto = read_pos;

  return rc;
}

void XIOasyncbuffer::metacallback(XIO_META type, const char* metabuff, int64_t pos)
{ Mutex::Lock lock(mtx_access);
  XIObuffer::metacallback(type, metabuff, pos);
};

