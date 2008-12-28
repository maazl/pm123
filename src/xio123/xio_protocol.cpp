/*
 * Copyright 2008 M.Mueller
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

#include "xio_protocol.h"
#include "xio_buffer.h"

#include <utilfct.h>

#include <memory.h>
#include <errno.h>

_XFILE::_XFILE()
: oflags(XO_NONE),
  protocol(NULL),
  serial(0),
  mtx(NULL),
  in_use(false),
  s_observer(0),
  s_metabuff(NULL),
  s_metasize(0)
{ DEBUGLOG(("xio:XFILE(%p)::XFILE()\n", this));
}

/* Cleanups the file structure. */
_XFILE::~_XFILE()
{ DEBUGLOG(("xio:XFILE(%p)::~XFILE()\n", this));
  serial = 0;
  delete mtx;
  mtx = NULL;
  delete protocol;
  protocol = NULL;
}

bool _XFILE::Request()
{ if (mtx)
  { mtx->Request();
  } else
  { ASSERT(!in_use);
    #ifdef NDEBUG
    if (in_use)
    { errno = EACCESS;
      return false;
    }
    #endif
    in_use = true;
  }
  return true;
}

void _XFILE::Release()
{ if (mtx)
  { mtx->Release();
  } else
  { in_use = false;
  }
}


XPROTOCOL::XPROTOCOL()
: blocksize(0),
  eof(false),
  error(0)
{
}

/* Reads up to n-1 characters from the stream or stop at the first
   new line. CR characters (\r) are discarded.
   Precondition: n > 1 && !error && !eof && XO_READ */
char* XPROTOCOL::gets( char* string, unsigned int n )
{
  char* string_bak = string;
  
  while( n > 1 && !eof ) { // save space for \0
    int len;
    if(( len = read( string, 1 )) == 1 ) {
      if( *string == '\r' ) {
        continue;
      } else if( *string == '\n' ) {
        ++string;
        --n;
        break;
      } else {
        ++string;
        --n;
      }
    } else if( len == 0 ) {
      eof   = 1;
      break;
    } else {
      error = errno;
      break;
    }
  }     
  *string = 0;
  return string != string_bak ? string_bak : NULL;
}

/* Copies string to the output file at the current position.
   It does not copy the null character (\0) at the end of the string.
   Returns -1 if an error occurs; otherwise, it returns a non-negative
   value.
   Precondition: !error && XO_WRITE */
int XPROTOCOL::puts( const char* string )
{ int rc = 0;
  while (*string) {
    // Find next \n.
    const char* cp = strchr(string, '\n');
    if (cp == NULL)
    { // write the remaining part of string.
      size_t len = strlen(string); 
      if (write( string, len ) == len)
        rc = -1;
      else 
        rc += len;
      break;
    }
    // write leading part before \n and \r\n
    size_t len = cp - string;
    if ( write( string, len ) != len
      || write( "\r\n", 2 ) != 2 )
    { rc = -1;
      break;
    }
    // continue
    rc += len;
    string += len+1;
  }
  return rc;
}

char* XPROTOCOL::get_metainfo( int type, char* result, int size )
{ *result = 0;
  return result;
}

void XPROTOCOL::set_observer( void DLLENTRYP(callback)(const char*, long, long, void*), void* arg )
{
}

int XIOreadonly::write( const void* source, unsigned int count )
{
  errno = EACCESS;
  return -1;
}

int XIOreadonly::chsize( long size, long offset64 )
{
  errno = EINVAL;
  return -1;
}

