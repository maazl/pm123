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

#include "xio_buffer.h"
#include "xio_protocol.h"
#include "xio.h"


/* Allocates and initializes the buffer. */
XIObuffer::XIObuffer(XPROTOCOL* chain, unsigned int buf_size)
: chain(chain),
  head(new char[buf_size]),
  size(buf_size),
  read_pos(0),
  s_obs_head(NULL),
  s_obs_tail(NULL)
{ *s_title = 0;
  blocksize = 4096; // Probably not needed by anyone
}

bool XIObuffer::init()
{
  chain->set_observer(this);
  return true;
}

/* Cleanups the buffer. */
XIObuffer::~XIObuffer()
{
  delete chain;
  chain = NULL;

  obs_clear();
  
  delete head;
  head = NULL;
}

/* open must be called BEFORE the buffer is set up, so any call to XIObuffer::open is an error! */
int XIObuffer::open( const char* filename, XOFLAGS oflags )
{
  DEBUGLOG(("xio:XIObuffer::open - invalid call!!!\n"));
  errno = error = EINVAL;
  return -1;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
int XIObuffer::close()
{
  // The buffer is read-only, so we can safely forward any call to close without flushing the buffer.
  int ret = chain->close();
  if (ret == 0) 
  { obs_clear();
    read_pos = -1;
  }
  return ret;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
long XIObuffer::tell( long* offset64 )
{
  // TODO: 64 bit
  if (offset64)
    *offset64 = 0;
  return read_pos;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long XIObuffer::getsize( long* offset64 )
{
  long ret = chain->getsize(offset64);
  // TODO 64 bit
  if (ret > read_pos && eof)
  { eof = false;
  }
  return ret;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
long XIObuffer::seek( long offset, int origin, long* offset64 )
{
  long result;

  // TODO: 64 bit!!!
  switch( origin ) {
    case XIO_SEEK_SET:
      result = offset;
      break;
    case XIO_SEEK_CUR:
      result = read_pos + offset;
      break;
    case XIO_SEEK_END:
      result = getsize();
      if (result != -1L)
      { result += offset;
        break;
      }
    default:
      errno = EINVAL;
      return -1;
  }

  return do_seek( result, offset64 );
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. Returns the value 0 if it successfully
   changes the file size. A return value of -1 shows an error.
   Precondition: XO_WRITE */
int XIObuffer::chsize( long size, long size64 )
{
  return chain->chsize( size, size64 );
}

char* XIObuffer::get_metainfo( int type, char* result, int size )
{ if ( type == XIO_META_TITLE && *s_title )
  { CritSect cs;
    if (s_title)
      strlcpy( result, s_title, size );
    else
      *result = 0;
    return result;
  } else
    return chain->get_metainfo( type, result, size );
}

void XIObuffer::obs_clear()
{ DEBUGLOG(("XIObuffer::obs_clear()\n"));
  while (s_obs_head)
  { obs_entry* entry = s_obs_head;
    s_obs_head = entry->link;
    delete entry;
  }
  s_obs_tail = NULL;
}

void XIObuffer::obs_discard()
{ DEBUGLOG2(("XIObuffer::obs_discard()\n"));
  while (s_obs_head)
  { obs_entry* entry = s_obs_head;
    if (entry->pos >= read_pos)
      return;
    s_obs_head = entry->link;
    delete entry;
  }
  s_obs_tail = NULL;
}

void XIObuffer::obs_execute()
{ DEBUGLOG2(("XIObuffer::obs_execute() - %p, %p\n", s_callback, s_arg));
  while (s_obs_head)
  { obs_entry* entry = s_obs_head;
    if (entry->pos >= read_pos)
      return;
    s_obs_head = entry->link;
    if (observer)
    { // set new title
      if( entry->metabuff ) {
        // TODO: Well, it should be up to xio_http to decode Streaming metadata.
        const char* titlepos = strstr( entry->metabuff, "StreamTitle='" );
        if ( titlepos )
        { titlepos += 13;
          unsigned i;
          for( i = 0; i < sizeof( s_title ) - 1 && *titlepos
                      && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
            s_title[i] = *titlepos++;
          s_title[i] = 0;
        }
      }
      // execute the observer
      observer->metacallback(entry->type, entry->metabuff, entry->pos, entry->pos64);
    }
    delete entry;
  }
  s_obs_tail = NULL;
}

void XIObuffer::metacallback(int type, const char* metabuff, long pos, long pos64)
{ obs_entry* entry = new obs_entry(type, metabuff, pos, pos64);
  if (s_obs_tail)
    s_obs_tail->link = entry;
  else
    s_obs_head = entry;
  s_obs_tail = entry;
  #if defined(DEBUG_LOG) && DEBUG_LOG > 1
  obs_dump();
  #endif
}

#ifdef DEBUG_LOG
void XIObuffer::obs_dump() const
{ const obs_entry* op = s_obs_head;
  while (op)
  { DEBUGLOG(("XIObuffer(%p)::obs_dump %08lx %08lx %s\n", this, op->pos64, op->pos, op->metabuff));
    op = op->link;
  }
}
#endif

XSFLAGS XIObuffer::supports() const
{ return chain->supports();
}

