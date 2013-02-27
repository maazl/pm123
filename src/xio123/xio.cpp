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
#include <string.h>
#include <errno.h>
#include <nerrno.h>
#include <netdb.h>
#include <sys/socket.h>

#include <utilfct.h>
#include <plugin.h> // For WM_METADATA
#include <debuglog.h>
#include <cpp/container/stringmap.h>

#include "xio.h"
#include "xio_protocol.h"
#include "xio_file.h"
#include "xio_ftp.h"
#include "xio_http.h"
#include "xio_cddb.h"
#include "xio_syncbuffer.h"
#include "xio_asyncbuffer.h"
#include "xio_socket.h"

#define XIO_SERIAL 0x41290837

#ifdef NDEBUG
#define XCHECK(x,err) \
  if (!x || x->serial != XIO_SERIAL) \
    { errno = EBADF; return err; }
#else
#define XCHECK(x,err) ASSERT(x && x->serial == XIO_SERIAL)
#endif

static char   http_proxy_host[XIO_MAX_HOSTNAME];
static int    http_proxy_port;
static char   http_proxy_user[XIO_MAX_USERNAME];
static char   http_proxy_pass[XIO_MAX_PASSWORD];
static u_long http_proxy_addr;

static int  buffer_size     = 32768;
static int  buffer_wait     = 0;
static int  buffer_fill     = 30;
static int  connect_timeout = 30;
static int  socket_timeout  = 30;

/* Serializes access to the library's global data. */
static Mutex mutex;


/* Open file. Returns a pointer to a file structure that can be used
   to access the open file. A NULL pointer return value indicates an
   error. */
XFILE* DLLENTRY xio_fopen(const char* filename, const char* mode)
{ DEBUGLOG(("xio_fopen(%s, %s)\n", filename, mode));
  XFILE* x;

  if (!filename || !*filename)
  { errno = ENOENT;
    return NULL;
  }
  x = new XFILE();

  if (strnicmp(filename, "http:", 5) == 0)
    x->protocol = new XIOhttp();
  else if (strnicmp(filename, "ftp:", 4) == 0)
  { if (*http_proxy_host)
      x->protocol = new XIOhttp();
    else
      x->protocol = new XIOftp();
  }
  else if (strnicmp(filename, "cddbp:", 6) == 0)
    x->protocol = new XIOcddb();
  else if (strnicmp(filename, "tcpip:", 6) == 0)
    x->protocol = new XIOsocket();
  else if (XIOfile64::supported())
    x->protocol = new XIOfile64();
  else
    x->protocol = new XIOfile32();

  if (strchr(mode, 'r'))
  { if (strchr(mode, '+'))
      x->oflags = XO_READ | XO_WRITE;
    else
      x->oflags = XO_READ;
  } else if (strchr(mode, 'w'))
  { if (strchr(mode, '+'))
      x->oflags = XO_WRITE | XO_CREATE | XO_TRUNCATE | XO_READ;
    else
      x->oflags = XO_WRITE | XO_CREATE | XO_TRUNCATE;
  } else if (strchr(mode, 'a'))
  { if (strchr(mode, '+'))
      x->oflags = XO_WRITE | XO_CREATE | XO_APPEND | XO_READ;
    else
      x->oflags = XO_WRITE | XO_CREATE | XO_APPEND;
  } else
  { delete x;
    errno = EINVAL;
    return NULL;
  }
  
  if (strchr(mode, 'R'))
    x->oflags |= XO_NOBUFFER;
  if (strchr(mode, 'X'))
    x->oflags |= XO_ASYNCBUFFER;
  if (strchr(mode, 'U'))
    x->oflags |= XO_NOMUTEX;

  XSFLAGS support = x->protocol->supports();
  if (x->oflags & ~support & 7) // Check read, write and create flags
  { delete x;
    errno = EINVAL;
    return NULL;
  }

  if (x->protocol->open(filename, x->oflags) != 0)
  { delete x;
    return NULL;
  }

  // Set up buffer
  if (!(x->oflags & XO_NOBUFFER) && !(x->oflags & XO_WRITE))
  { int size = xio_buffer_size();
    XIObuffer* buffer;
    if ((x->oflags & XO_ASYNCBUFFER) && size)
      buffer = (XIObuffer*)new XIOasyncbuffer(x->protocol, xio_buffer_size());
    else
      buffer = new XIOsyncbuffer(x->protocol, xio_buffer_size());
    if (buffer->init())
      x->protocol = buffer;
    else
      delete buffer;
  }
 
  if (!(x->oflags & XO_NOMUTEX))
    x->mtx = new Mutex();
  
  // We got it!  
  x->serial = XIO_SERIAL;

  DEBUGLOG(("xio_open: %p\n", x));
  return x;
}

/* Reads up to count items of size length from the input file and
   stores them in the given buffer. The position in the file increases
   by the number of bytes read. Returns the number of full items
   successfully read, which can be less than count if an error occurs
   or if the end-of-file is met before reaching count. */
size_t DLLENTRY xio_fread(void* buffer, size_t size, size_t count, XFILE* x)
{ DEBUGLOG(("xio_fread(%p, %u, %u, %p)\n", buffer, size, count, x));
  XCHECK(x, 0);
  int read = count * size;

  if (!(x->oflags & XO_READ))
  { errno = EACCES;
    return 0;
  }
  errno = 0;
  if (read == 0)
    return 0;  
  if (!x->Request())
    return 0;

  size_t rc = 0;
  if (x->protocol->error)
    errno = x->protocol->error;
  else if (!x->protocol->eof)
  { int done = x->protocol->read(buffer, read);
    if (done == read)
      rc = count;
    else if (done >= 0)
    { if (done % size == 0 || x->protocol->seek(-(done % size), XIO_SEEK_CUR) == -1)
        x->protocol->eof = true;
      rc = done / size;
    } else
      x->protocol->error = errno;
  }
  x->Release();
  DEBUGLOG(("xio_fread: %u - now at %u\n", rc, x->protocol->tell()));
  return rc;
}

/* Writes up to count items, each of size bytes in length, from buffer
   to the output file. Returns the number of full items successfully
   written, which can be fewer than count if an error occurs. */
size_t DLLENTRY xio_fwrite(const void* buffer, size_t size, size_t count, XFILE* x)
{ DEBUGLOG(("xio_fwrite(%p, %u, %u, %p)\n", buffer, size, count, x));
  XCHECK(x, 0);
  int write = count * size;

  if(!(x->oflags & XO_WRITE))
  { errno = EACCES;
    return 0;
  }
  errno = 0;
  if (count == 0)
    return 0;
  if (!x->Request())
    return 0;

  size_t rc = 0;
  if (x->protocol->error)
    errno = x->protocol->error;
  else
  { int done = x->protocol->write(buffer, write);
    if (done == write)
      rc = count;
    else if (done == -1)
      x->protocol->error = errno;
    else
    { x->protocol->seek(-(done % size), XIO_SEEK_CUR);
      rc = done / size;
    }
  }
  x->Release();
  DEBUGLOG(("xio_fwrite: %u\n", rc));
  return rc;
}

/* Closes a file pointed to by x. Returns 0 if it successfully closes
   the file, or -1 if any errors were detected. */
int DLLENTRY xio_fclose(XFILE* x)
{ DEBUGLOG(("xio_fclose(%p)\n", x));
  XCHECK(x, -1);
  int ret = x->protocol->close() == 0 ? 0 : -1;
  // cleanup anyway
  delete x;
  DEBUGLOG(("xio_fclose: %i\n", ret));
  return ret;
}

/* Causes an abnormal termination of all current read/write
   operations of the file. All current and subsequent calls can
   raise an error. */
void DLLENTRY xio_fabort(XFILE* x)
{ DEBUGLOG(("xio_fabort(%p)\n", x));
  XCHECK(x,);
  x->protocol->close();
}

/* Finds the current position of the file. Returns the current file
   position. On error, returns -1L and sets errno to a nonzero value. */
int64_t DLLENTRY xio_ftelll(XFILE* x)
{ DEBUGLOG(("xio_ftell(%p)\n", x));
  XCHECK(x, -1);
  return x->protocol->tell();
}
/* Finds the current position of the file. Returns the current file
   position. On error, returns -1L and sets errno to a nonzero value. */
long DLLENTRY xio_ftell(XFILE* x)
{ int64_t ret = xio_ftelll(x);
  if (ret > 0x7fffffff)
  { errno = EINVAL;
    return -1;
  }
  return (long)ret;
}

/* Changes the current file position to a new location within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
int64_t DLLENTRY xio_fseekl(XFILE* x, int64_t offset, XIO_SEEK origin)
{ DEBUGLOG(("xio_fseekl(%p, %lli, %i)\n", x, offset, origin));
  XCHECK(x, -1);
  if (!x->Request())
    return -1;
  x->protocol->error = 0;
  x->protocol->eof   = 0;
  int64_t ret = x->protocol->seek(offset, origin);
  x->Release();
  DEBUGLOG(("xio_fseek: %lli\n", ret));
  return ret;
}
/* Changes the current file position to a new location within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
long DLLENTRY xio_fseek(XFILE* x, long int offset, XIO_SEEK origin)
{ int64_t ret = xio_fseekl(x, offset, origin);
  if (ret > 0x7fffffff)
  { errno = EINVAL;
    return -1;
  }
  return (long)ret;
}

/* Repositions the file pointer associated with stream to the beginning
   of the file. A call to xio_rewind is the same as:
   (void)xio_fseek( x, 0L, XIO_SEEK_SET )
   except that xio_rewind also clears the error indicator for
   the stream. */
int DLLENTRY xio_rewind(XFILE* x)
{ return xio_fseekl(x, 0, XIO_SEEK_SET) == -1L;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
int64_t DLLENTRY xio_fsizel(XFILE* x)
{ DEBUGLOG(("xio_fsizel(%p)\n", x));
  XCHECK(x, -1);
  int64_t ret = x->protocol->getsize();
  DEBUGLOG(("xio_fsizel: %lli\n", ret));
  return ret;
}
long DLLENTRY xio_fsize(XFILE* x)
{ int64_t ret = xio_fsizel(x);
  if (ret > 0x7fffffff)
  { errno = EINVAL;
    return -1;
  }
  return (long)ret;
}

int DLLENTRY xio_fstatl(XFILE* x, XSTATL* st)
{ DEBUGLOG(("xio_fstatl(%p)\n", x));
  XCHECK(x, -1);
  int ret = x->protocol->getstat(st);
  DEBUGLOG(("xio_fstatl: %li\n", ret));
  return ret;
}
int DLLENTRY xio_fstat(XFILE* x, XSTAT* st)
{ XSTATL st64;
  int ret = xio_fstatl(x, &st64);
  if (ret)
    return ret;
  if (st64.size > 0x7ffffff)
  { errno = EINVAL;
    return -1;
  }
  st->size = (long)st64.size;
  memcpy(&st->atime, &st64.atime, sizeof st64 - sizeof st64.size);
  return 0;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. Returns the value 0 if it successfully
   changes the file size. A return value of -1 shows an error. */
int DLLENTRY xio_ftruncatel(XFILE* x, int64_t size)
{ DEBUGLOG(("xio_ftruncatel(%p, %lli)\n", x, size));
  XCHECK(x, -1);
  if (!(x->oflags & XO_WRITE))
  { errno = EACCES;
    return -1;
  }
  if (!x->Request())
    return -1;
  int ret = x->protocol->chsize(size);
  x->Release();
  return ret;
}
int DLLENTRY xio_ftruncate(XFILE* x, long size)
{ return xio_ftruncatel(x, size);
}

/* Reads bytes from the current file position up to and including the
   first new-line character (\n), up to the end of the file, or until
   the number of bytes read is equal to n-1, whichever comes first.
   Stores the result in string and adds a null character (\0) to the
   end of the string. The string includes the new-line character, if
   read. If n is equal to 1, the string is empty. Returns a pointer
   to the string buffer if successful. A NULL return value indicates
   an error or an end-of-file condition. */
char* DLLENTRY xio_fgets(char* string, int n, XFILE* x)
{ DEBUGLOG(("xio_fgets(%p, %i, %p)\n", string, n, x));
  XCHECK(x, NULL);

  if (!(x->oflags & XO_READ))
  { errno = EACCES;
    return NULL;
  }
  if (!x->Request())
    return NULL;
  char* ret = NULL;
  if (x->protocol->error)
    errno = x->protocol->error;
  else if (n <= 1) // Trivial cases
  { if (n <= 0)
      errno = EINVAL;
    else
    { *string = 0;
      ret = string;
    }
  } else
    ret = x->protocol->eof ? NULL : x->protocol->gets(string, n);
  x->Release();
  DEBUGLOG(("xio_fgets: %s, %i\n", ret, x->protocol->error));
  return ret;
}

/* Copies string to the output file at the current position.
   It does not copy the null character (\0) at the end of the string.
   Returns -1 if an error occurs; otherwise, it returns a non-negative
   value. */
int DLLENTRY xio_fputs(const char* string, XFILE* x)
{ DEBUGLOG(("xio_fputs(%s, %p)\n", string, x));
  XCHECK(x, -1);
  
  if (!(x->oflags & XO_WRITE))
  { errno = EACCES;
    return -1;
  }
  if (!x->Request())
    return -1;

  int rc;
  if (x->protocol->error)
  { errno = x->protocol->error;
    rc = -1;
  } else
    rc = x->protocol->puts(string);

  x->Release();
  DEBUGLOG(("xio_fputs: %i\n", rc));
  return rc;
}

/* Indicates whether the end-of-file flag is set for the given stream.
   The end-of-file flag is set by several functions to indicate the
   end of the file. The end-of-file flag is cleared by calling xio_rewind,
   xio_fseek, or xio_clearerr for this stream. */
int DLLENTRY xio_feof(XFILE* x)
{ DEBUGLOG2(("xio_feof(%p)\n", x));
  XCHECK(x, 1);
  return x->protocol->eof;
}

/* Tests for an error in reading from or writing to the given stream.
   If an error occurs, the error indicator for the stream remains set
   until you close stream, call xio_rewind, or call xio_clearerr. */
int DLLENTRY xio_ferror(XFILE* x)
{ DEBUGLOG2(("xio_ferror(%p)\n", x));
  if (!x)
    return errno;
  ASSERT(x->serial == XIO_SERIAL);
  #ifdef NDEBUG
  if (x->serial != XIO_SERIAL)
  { errno = EBADF;
    return 0;
  }
  #endif
  return x->protocol->error;
}

/* Resets the error indicator and end-of-file indicator for the
   specified stream. Once set, the indicators for a specified stream
   remain set until your program calls xio_clearerr or xio_rewind.
   xio_fseek also clears the end-of-file indicator. */
void DLLENTRY xio_clearerr(XFILE* x)
{ DEBUGLOG(("xio_clearerr(%p)\n", x));
  if (!x)
  { errno = 0;
    return;
  }
  ASSERT(x->serial == XIO_SERIAL);
  #ifdef NDEBUG
  if (x->serial != XIO_SERIAL)
  { errno = EBADF;
    return;
  }
  #endif
  if (x->Request())
  { x->protocol->error = 0;
    x->protocol->eof   = 0;
    x->Release();
    errno = 0;
  }
}

/* Returns the last error code set by a library call in the current
   thread. Subsequent calls do not reset this error code. */
int DLLENTRY xio_errno()
{ return errno;
}

/* Maps the error number in errnum to an error message string. */
const char* DLLENTRY xio_strerror(int errnum)
{
  if (errnum >= CDDBBASEERR)
    return XIOcddb::strerror(errnum);
  else if (errnum >= FTPBASEERR)
    return XIOftp::strerror(errnum);
  else if (errnum >= HTTPBASEERR)
    return XIOhttp::strerror(errnum);
  else if(errnum >= HBASEERR)
    return h_strerror(errnum - HBASEERR);
  #ifdef SOCBASEERR
  else if(errnum >= SOCBASEERR)
    return sock_strerror(errnum);
  #endif
  else
    return strerror(errnum);
}

// C-API for 64 bit callbacks.
class XIOmetacallback64 : public XPROTOCOL::Iobserver
{private:
  void DLLENTRYP2(const Callback)(XIO_META type, const char* metabuff, int64_t pos, void* arg);
  void* const Arg;
 public:
  XIOmetacallback64(void DLLENTRYP(callback)(XIO_META type, const char* metabuff, int64_t pos, void* arg), void* arg)
  : Callback(callback), Arg(arg)
  { DEBUGLOG(("xio:XIOmetacallback64(%p)::XIOmetacallback64(%p, %p)\n", this, callback, arg)); }
  virtual void metacallback(XIO_META type, const char* metabuff, int64_t pos);
};

void XIOmetacallback64::metacallback(XIO_META type, const char* metabuff, int64_t pos)
{ DEBUGLOG(("xio:XIOmetacallback64(%p)::metacallback(%i, %s, %lli)\n", this, type, metabuff, pos));
  (*Callback)(type, metabuff, pos, Arg);
}

// C-API for 32 bit callbacks.
class XIOmetacallback32 : public XPROTOCOL::Iobserver
{private:
  void DLLENTRYP2(const Callback)(XIO_META type, const char* metabuff, long pos, void* arg);
  void* const Arg;
 public:
  XIOmetacallback32(void DLLENTRYP(callback)(XIO_META type, const char* metabuff, long pos, void* arg), void* arg)
  : Callback(callback), Arg(arg)
  { DEBUGLOG(("xio:XIOmetacallback32(%p)::XIOmetacallback32(%p, %p)\n", this, callback, arg)); }
  virtual void metacallback(XIO_META type, const char* metabuff, int64_t pos);
};

void XIOmetacallback32::metacallback(XIO_META type, const char* metabuff, int64_t pos)
{ DEBUGLOG(("xio:XIOmetacallback32(%p)::metacallback(%i, %s, %lli)\n", this, type, metabuff, pos));
  (*Callback)(type, metabuff, pos > 0x7fffffff ? -1 : (long)pos, Arg);
}

/* Sets a callback function that is notified when meta information
   changes in the stream. */
int DLLENTRY xio_set_metacallbackl(XFILE* x, void DLLENTRYP(callback)(XIO_META type, const char* metabuff, int64_t pos, void* arg), void* arg )
{ DEBUGLOG(("xio_set_metacallbackl(%p, %p, %p)\n", x, callback, arg));
  XCHECK(x,-1);
  if (x->Request())
  { delete x->protocol->set_observer(callback ? new XIOmetacallback64(callback, arg) : NULL);
    x->Release();
    errno = 0;
  }
  return 0;
}
/* Sets a callback function that is notified when meta information
   changes in the stream. */
int DLLENTRY xio_set_metacallback(XFILE* x, void DLLENTRYP(callback)(XIO_META type, const char* metabuff, long pos, void* arg), void* arg )
{ DEBUGLOG(("xio_set_metacallback(%p, %p, %p)\n", x, callback, arg));
  XCHECK(x,-1);
  if (x->Request())
  { delete x->protocol->set_observer(callback ? new XIOmetacallback32(callback, arg) : NULL);
    x->Release();
    errno = 0;
  }
  return 0;
}

/* Returns a specified meta information if it is
   provided by associated stream. */
char* DLLENTRY xio_get_metainfo(XFILE* x, XIO_META type, char* result, int size)
{ DEBUGLOG(("xio_get_metainfo(%p, %i, %p, %i)\n", x, type, result, size));
  XCHECK(x, NULL);
  if (!size)
  { errno = EINVAL;
    return NULL;
  }
  return x->protocol->get_metainfo(type, result, size);
}

// emulation for deprecated observers
class XIOobserveremulation : public XPROTOCOL::Iobserver
{private:
  const HWND  s_observer; /* Handle of a window that are to be notified    */
                          /* of changes in the state of the library.       */
  char* const s_metabuff; /* The library puts metadata in this buffer      */
  const int   s_metasize; /* before notifying the observer.                */
 public:
  XIOobserveremulation(unsigned long window, char* buffer, int buffer_size)
  : s_observer(window), s_metabuff(buffer), s_metasize(buffer_size)
  { DEBUGLOG(("xio:XIOobserveremulation(%p)::XIOobserveremulation(%lu, %p, %i)\n",
      this, window, buffer, buffer_size));
  }
  virtual void metacallback(XIO_META type, const char* metabuff, int64_t pos);
};

void XIOobserveremulation::metacallback(XIO_META type, const char* metabuff, int64_t pos)
{ DEBUGLOG(("xio:XIOobserveremulation(%p)::metacallback(%i, %s, %lli)\n", this, type, metabuff, pos));
  if (type == XIO_META_TITLE)
  { strlcpy(s_metabuff, metabuff, s_metasize);
    WinPostMsg(s_observer, WM_METADATA, MPFROMP(s_metabuff), MPFROMLONG(pos));
  }
}

/* Sets a handle of a window that are to be notified of changes
   in the state of the library. */
int DLLENTRY xio_set_observer(XFILE* x, unsigned long window, char* buffer, int buffer_size)
{ DEBUGLOG(("xio_set_observer(%p, %lu, %p, %i)\n", x, window, buffer, buffer_size));
  XCHECK(x, -1);
  if (x->Request())
  { delete x->protocol->set_observer(window && buffer ? new XIOobserveremulation(window, buffer, buffer_size) : NULL);
    x->Release();
    errno = 0;
  }
  return 0;
}

/* Returns XIO_NOT_SEEK (0) on streams incapable of seeking,
   XIO_CAN_SEEK (1) on streams capable of seeking and returns
   XIO_CAN_SEEK_FAST (2) on streams capable of fast seeking. */
XIO_SEEK_SUPPORT DLLENTRY xio_can_seek(XFILE* x)
{ DEBUGLOG(("xio_can_seek(%p)\n", x));
  XCHECK(x, XIO_NOT_SEEK);
  XSFLAGS support = x->protocol->supports();
  DEBUGLOG(("xio_can_seek %x\n", support));
  if (support & XS_CAN_SEEK_FAST)
    return XIO_CAN_SEEK_FAST;
  else if (support & XS_CAN_SEEK)
    return XIO_CAN_SEEK;
  else
    return XIO_NOT_SEEK;
}

XIO_PROTOCOL DLLENTRY xio_protocol(XFILE* x)
{ DEBUGLOG(("xio_protocol(%p)\n", x));
  XCHECK(x, XIO_PROTOCOL_NONE);
  return x->protocol->protocol();
}

XIO_PROTOCOL DLLENTRY xio_urlprotocol(const char* url)
{ strmap<8,XIO_PROTOCOL> map[] =
  { { "cddbp:", XIO_PROTOCOL_CDDB }
  , { "file:",  XIO_PROTOCOL_FILE }
  , { "ftp:",   XIO_PROTOCOL_FTP  }
  , { "http:",  XIO_PROTOCOL_HTTP }
  , { "tcpip:", XIO_PROTOCOL_SOCKET }
  };
  strmap<8,XIO_PROTOCOL>* p = mapsearcha(map, url);
  return p ? p->Val : XIO_PROTOCOL_NONE;
}


/* Returns the read-ahead buffer size. */
int DLLENTRY xio_buffer_size()
{ return buffer_size;
}

/* Returns fills the buffer before reading state. */
int DLLENTRY xio_buffer_wait()
{ return buffer_wait;
}

/* Returns value of prefilling of the buffer. */
int DLLENTRY xio_buffer_fill()
{ return buffer_fill;
}

/* Sets the read-ahead buffer size. */
void DLLENTRY xio_set_buffer_size(int size)
{ buffer_size = size;
}

/* Sets fills the buffer before reading state. */
void DLLENTRY xio_set_buffer_wait(int wait)
{ buffer_wait = wait;
}

/* Sets value of prefilling of the buffer. */
void DLLENTRY xio_set_buffer_fill(int percent)
{ if (percent > 0 && percent <= 100)
    buffer_fill = percent;
}

/* Returns the name of the proxy server. */
void DLLENTRY xio_http_proxy_host(char* hostname, int size)
{ Mutex::Lock lock(mutex);
  strlcpy( hostname, http_proxy_host, size );
}

/* Returns the port number of the proxy server. */
int DLLENTRY xio_http_proxy_port()
{ return http_proxy_port;
}

/* Returns the user name of the proxy server. */
void DLLENTRY xio_http_proxy_user(char* username, int size)
{ Mutex::Lock lock(mutex);
  strlcpy(username, http_proxy_user, size);
}

/* Returns the user password of the proxy server. */
void DLLENTRY
xio_http_proxy_pass(char* password, int size)
{ Mutex::Lock lock(mutex);
  strlcpy(password, http_proxy_pass, size );
}

/* Sets the name of the proxy server. */
void DLLENTRY xio_set_http_proxy_host(const char* hostname)
{ Mutex::Lock lock(mutex);
  strlcpy(http_proxy_host, hostname, sizeof http_proxy_host);
  http_proxy_addr = 0;
}

/* Sets the port number of the proxy server. */
void DLLENTRY xio_set_http_proxy_port(int port)
{ http_proxy_port = port;
}

/* Sets the user name of the proxy server. */
void DLLENTRY xio_set_http_proxy_user(const char* username)
{ Mutex::Lock lock(mutex);
  strlcpy(http_proxy_user, username, sizeof http_proxy_user);
}

/* Sets the user password of the proxy server. */
void DLLENTRY xio_set_http_proxy_pass(const char* password)
{ Mutex::Lock lock(mutex);
  strlcpy(http_proxy_pass, password, sizeof http_proxy_pass);
}

/* Returns an internet address of the proxy server.
   Returns 0 if the proxy server is not defined or -1 if
   an error occurs */
unsigned long DLLENTRY xio_http_proxy_addr()
{
  char host[XIO_MAX_HOSTNAME];
  u_long address;

  { Mutex::Lock lock(mutex);
    strlcpy( host, http_proxy_host, sizeof host);
    address = http_proxy_addr;
  }

  if (address != 0 && address != (u_long)-1)
    return address;
  if (!*host)
    return 0;

  address = XIOsocket::get_address(host);

  http_proxy_addr = address;
  return address;
}

/* Returns the TCP/IP connection timeout. */
int DLLENTRY xio_connect_timeout()
{ return connect_timeout;
}

/* Sets the TCP/IP connection timeout. */
void DLLENTRY xio_set_connect_timeout(int seconds)
{ connect_timeout = seconds;
}

/* Returns the TCP/IP connection timeout. */
int DLLENTRY xio_socket_timeout()
{ return socket_timeout;
}

/* Sets the TCP/IP connection timeout. */
void DLLENTRY xio_set_socket_timeout(int seconds)
{ socket_timeout = seconds;
}

