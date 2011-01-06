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
#include <errno.h>
#include <string.h>

#include <utilfct.h>
#include <debuglog.h>

#include "xio_syncbuffer.h"
#include "xio_protocol.h"
#include "xio.h"


/* Allocates and initializes the buffer. */
XIOsyncbuffer::XIOsyncbuffer(XPROTOCOL* chain, unsigned int buf_size)
: XIObuffer(chain, buf_size), 
  data_size(0),
  data_read(0)
{}

/* Load new data into the buffer. Return false on error.
   The function tries to load the entire buffer size, but it succeeds also
   with less data if the underlying stream runs into EOF.
   data_size will tell you what has happend. If it is less than size
   EOF is reached. */
bool XIOsyncbuffer::fill_buffer()
{ DEBUGLOG(("XIOsyncbuffer(%p)::fill_buffer() - %p, %u\n", this, head, size));
  unsigned int read_size = chain->read( head, size );
  if (read_size == (unsigned int)-1)
  { error = errno;
    if (error == 0) // Hmm...
      error = EINVAL;
    return false;
  }
  data_size = read_size;
  return true;
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.
   Precondition: count > 0 && !eof && XO_READ */
int XIOsyncbuffer::read( void* result, unsigned int count )
{
  DEBUGLOG(("XIOsyncbuffer(%p)::read(%p, %u) - %u, %u\n", this, result, count, data_size, data_read));
  unsigned int read_done = 0;

 again:
  if( data_size > data_read )
  { // Something is in the buffer
    unsigned int read_size = min( data_size - data_read, count - read_done );
    // Copy a next chunk of data in the result buffer.
    memcpy( (char*)result + read_done, head + data_read, read_size );

    read_done += read_size;
    data_read += read_size;
  }

  // not enough?
  if (read_done < count)
  {
    DEBUGLOG(("XIOsyncbuffer::read: fill buffer %u, %u, %u, %u, %i, %i\n",
      read_done, count, data_read, size, chain->error, chain->eof));
    // Now the buffer is neccessarily consumed => discard it.
    data_size = 0;
    data_read = 0;
    // But maybe we already run into an error in the chain. 
    if (chain->error) 
    { errno = error = chain->error;
    } else if (chain->eof)
    { eof = true;
    } else if (count - read_done >= size)
    { // Buffer bypass for large requests
      unsigned int read_size = chain->read( (char*)result + read_done, count - read_done );
      if (read_size == (unsigned int)-1)
      { error = chain->error;
      } else
      { read_done += read_size;
        if (read_size < size)
          eof = true;
      }
    } else
    { // Fill buffer
      if (fill_buffer())
        goto again;
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
    while (data_read < data_size)
    { char c = head[data_read++];
      ++read_pos;
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
    }
    // Buffer empty or file truncated.
    data_size = 0;
    data_read = 0;
    if (!fill_buffer())
    { break;
    }
    if (data_size == 0)
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
    data_read = 0;
  }
  return ret;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
long XIOsyncbuffer::do_seek( long offset, long* offset64 )
{
  // TODO: 64 bit
  // Does result lie within the buffer?
  long buf_start_pos = read_pos - data_read;
  if( offset >= buf_start_pos && offset <= buf_start_pos + data_size && !error )
  {
    data_read = offset - buf_start_pos;
    // TODO: When turning the pointer backwards in the buffer the observer events are not fired again. 
    obs_discard();
  }
  else {
    if( chain->seek( offset, XIO_SEEK_SET ) != -1L ) {
      obs_clear();
      // Discard the buffer
      data_size = 0;
      data_read = 0;
      // Reset error flags
      error = false;
      eof   = false;
    } else
    { error = chain->error;
      return -1L;
    }
  }

  return read_pos = offset; // implicitely atomic
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
int XIOsyncbuffer::chsize( long size, long size64 )
{
  int rc = XIObuffer::chsize( size, size64 );
  if (rc >= 0)
  { // TODO: 64 bit
    long buf_start_pos = read_pos - data_read;
    if (rc != -1 && size < buf_start_pos + data_size)
      data_size = size < buf_start_pos ? 0 : size - buf_start_pos;
  }
  return rc;
}

