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
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <string.h>

#include <utilfct.h>
#include <debuglog.h>

#include "xio_syncbuffer.h"
#include "xio_protocol.h"
#include "xio.h"


XIOsyncbuffer::XIOsyncbuffer(XPROTOCOL* chain, unsigned int buf_size)
: XIObuffer(chain, buf_size)
, data_size(0)
, data_ptr(0)
, data_unread(0)
{}

bool XIOsyncbuffer::fill_buffer(size_t minsize)
{ DEBUGLOG(("XIOsyncbuffer(%p)::fill_buffer(%u) - %u, %u\n", this, minsize, data_ptr, data_size));
  ASSERT(minsize <= size && minsize);
  ASSERT(data_unread == 0);
  // round up to next blocksize
  size_t read_size = minsize + blocksize - 1 - ((minsize-1) % blocksize);
  // limit to buffer size
  if (read_size >= size)
  { read_size = size;
    data_ptr = 0;
    goto read1;
  }
  if (data_ptr + read_size > size)
  { // Ring buffer turn around
    size_t read_size1 = size - data_ptr;
    size_t data_read = chain->read(head + data_ptr, read_size1);
    DEBUGLOG(("XIOsyncbuffer::fill_buffer: split %u, %i\n", data_read, errno));
    if (data_read == (size_t)-1)
      goto fail;
    if (data_read != read_size1) // eof is coming
      read_size = data_read;
    else
    { data_read = chain->read(head, read_size - read_size1);
      if (data_read != (size_t)-1)
        read_size += data_read;
    }
  } else
  {read1:
    read_size = chain->read(head + data_ptr, read_size);
    DEBUGLOG(("XIOsyncbuffer::fill_buffer: %u, %i\n", read_size, errno));
    if (read_size == (size_t)-1)
    {fail:
      error = errno;
      if (error == 0) // Hmm...
        error = EINVAL;
      return false;
    }
  }
  if (read_size == 0)
  { eof = true;
    return false;
  }
  data_unread = read_size;
  data_size += read_size;
  if (data_size > size)
    data_size = size;
  return true;
}

int XIOsyncbuffer::read(void* result, unsigned int count)
{
  DEBUGLOG(("XIOsyncbuffer(%p)::read(%p, %u) - %u, %u\n", this, result, count, data_size, data_ptr));
  unsigned int read_done = 0;

 again:
  unsigned int read_size = count - read_done;
  if (read_size > data_unread)
    read_size = data_unread;
  // Copy a next chunk of data in the result buffer.
  if (data_ptr + read_size > size)
  { // Ring buffer turn around
    size_t read_size1 = size - data_ptr;
    memcpy((char*)result + read_done, head + data_ptr, read_size1);
    memcpy((char*)result + read_done + read_size1, head, read_size - read_size1);
  } else
    memcpy((char*)result + read_done, head + data_ptr, read_size);
  read_done += read_size;
  data_ptr += read_size;
  if (data_ptr > size)
    data_ptr -= size;
  data_unread -= read_size;

  // not enough?
  if (read_done < count)
  { DEBUGLOG(("XIOsyncbuffer::read: fill buffer %u, %u, {%u, %u, %u}, %i, %i\n",
      read_done, count, data_size, data_ptr, data_unread, chain->error, chain->eof));
    // But maybe we already run into an error in the chain. 
    if (chain->error)
      errno = error = chain->error;
    else if (chain->eof)
      eof = true;
    else if (count - read_done <= size)
    { // Fill buffer
      if (fill_buffer(count - read_done))
        goto again;
    } else
    { // Buffer bypass for large requests
      unsigned int read_size = chain->read((char*)result + read_done, count - read_done);
      if (read_size == (size_t)-1)
        error = chain->error;
      else
      { read_done += read_size;
        if (read_size < size)
          eof = true;
      }
    }
  }

  // Also in case of an error read_pos should be kept up to date
  // and some observer events may be fired.
  read_pos += read_done;
  obs_execute();
  if (error)
  { // But then it is the end.
    obs_clear();
    return -1;
  }
  return read_done;
}

/* Reads up to n-1 characters from the stream or stop at the first
   new line. CR characters (\r) are discarded.
   Precondition: n > 1 && !eof && XO_READ */
char* XIOsyncbuffer::gets(char* string, unsigned int n)
{ char* ret = string;
  --n; // space for \n
  for(;;)
  { // Transfer characters and stop after \n
    while (data_unread)
    { char c = head[data_ptr++];
      ++read_pos;
      --data_unread;
      switch (c)
      {case '\r':
        continue;
       case '\n':
        *string++ = c;
        goto done;
       default:
        *string++ = c;
        if (--n == 0)
          goto done;
      }
      if (data_ptr == size)
        data_ptr = 0;
    }
    // Buffer empty
    if (!fill_buffer(1))
      break;
    else if (data_size == 0)
    { eof = true;
      break;
    }
  }
  if (ret == string)
    return NULL;
 done: 
  *string = 0;
  return ret;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
int XIOsyncbuffer::close()
{
  int ret = XIObuffer::close();
  if (ret == 0)
  { data_size = 0;
    data_ptr = 0;
    data_unread = 0;
  }
  return ret;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
int64_t XIOsyncbuffer::do_seek(int64_t offset)
{
  // Does result lie within the buffer?
  int64_t buf_end_pos = read_pos + data_unread;
  if (offset >= buf_end_pos - data_size && offset <= buf_end_pos && !error)
  { // Cache hit
    size_t data_end = data_ptr + data_unread; // may overflow size
    data_unread = (size_t)(buf_end_pos - offset);
    data_ptr = (data_end + size - data_unread) % size;
    DEBUGLOG(("XIOsyncbuffer::do_seek: cache hit %u, %u\n", data_unread, data_ptr));
    error = 0;
    eof = !data_unread && chain->eof;
    // TODO: When turning the pointer backwards in the buffer the observer events are not fired again. 
    obs_discard();
  } else
  { DEBUGLOG(("XIOsyncbuffer::do_seek: cache miss %llu, %llu\n", read_pos, offset));
    if (chain->seek(offset, XIO_SEEK_SET) != -1)
    { obs_clear();
      // Discard the buffer
      data_size = 0;
      data_ptr = 0;
      data_unread = 0;
      // Reset error flags
      error = 0;
      eof   = chain->eof;
    } else
    { error = chain->error;
      return -1;
    }
  }

  return read_pos = offset;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
int XIOsyncbuffer::chsize(int64_t newsize)
{
  int rc = XIObuffer::chsize(newsize);
  if (rc >= 0)
  { int64_t buf_end_pos = read_pos + data_unread;
    if (newsize < buf_end_pos)
    { int64_t strip_end = buf_end_pos - newsize; // Strip no of bytes from the back of the buffer.
      if (strip_end >= data_unread)
      { // discard entire buffer because we can't strip before data_ptr.
        data_size = 0;
        data_ptr = 0;
        data_unread = 0;
        eof = true;
      } else
      { // shorten read ahead buffer only
        data_size -= (size_t)strip_end;
        data_unread -= (size_t)strip_end;
      }
    }
  }
  return rc;
}

