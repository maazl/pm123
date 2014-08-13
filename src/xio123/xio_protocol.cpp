/*
 * Copyright 2008-2013 M.Mueller
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


volatile xstring http_proxy(xstring::empty);
volatile xstring http_proxy_auth(xstring::empty);

volatile int  buffer_size     = 32768;
volatile int  buffer_wait     = 0;
volatile int  buffer_fill     = 30;
volatile int  connect_timeout = 30;
volatile int  socket_timeout  = 30;


_XFILE::_XFILE()
: oflags(XO_NONE),
  protocol(NULL),
  serial(0),
  mtx(NULL),
  in_use(false)
{ DEBUGLOG(("xio:XFILE(%p)::XFILE()\n", this));
}

/* Cleanups the file structure. */
_XFILE::~_XFILE()
{ DEBUGLOG(("xio:XFILE(%p)::~XFILE()\n", this));
  serial = 0;
  delete mtx;
  mtx = NULL;
  if (protocol)
  { delete protocol->set_observer(NULL);
    delete protocol;
    protocol = NULL;
  }
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
: eof(false)
, error(0)
, blocksize(0)
, observer(NULL)
{}

/* Reads up to n-1 characters from the stream or stop at the first
   new line. CR characters (\r) are discarded.
   Precondition: n > 1 && !error && !eof && XO_READ */
char* XPROTOCOL::gets(char* string, unsigned int n)
{
  char* string_bak = string;

  while (n > 1 && !eof) // save space for \0
  { int len;
    if ((len = read(string, 1)) == 1)
    { if (*string == '\r')
        continue;
      --n;
      if (*string++ == '\n')
        break;
    } else if (len == 0)
    { eof = true;
      break;
    } else
    { error = errno;
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
int XPROTOCOL::puts(const char* string)
{ int rc = 0;
  while (*string) {
    // Find next \n.
    const char* cp = strchr(string, '\n');
    if (cp == NULL)
    { // write the remaining part of string.
      int len = strlen(string);
      if (write(string, len) != len)
        rc = -1;
      else
        rc += len;
      break;
    }
    // write leading part before \n and \r\n
    int len = cp - string;
    if ( write(string, len) != len
      || write("\r\n", 2) != 2 )
    { rc = -1;
      break;
    }
    // continue
    rc += len;
    string += len+1;
  }
  return rc;
}

xstring XPROTOCOL::get_metainfo(XIO_META type)
{ return xstring();
}

XPROTOCOL::Iobserver* XPROTOCOL::set_observer(Iobserver* obs)
{ Iobserver* ret = observer;
  observer = obs;
  return ret;
}


int XIOreadonly::write(const void* source, unsigned int count)
{ errno = EACCES;
  return -1;
}

int XIOreadonly::chsize(int64_t size)
{ errno = EINVAL;
  return -1;
}

