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
#include "xio_http.h"
#include "xio_socket.h"
#include "xio.h"
#include <cpp/xstring.h>
#include <cpp/url123.h>
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <utilfct.h>

/* Get and parse HTTP reply. */
int XIOhttp::http_read_reply( )
{
  char* p;
  char  buffer[64];

  if (!s_socket.gets( buffer, sizeof(buffer)))
    return HTTP_PROTOCOL_ERROR;

  // A valid status line looks like "HTTP/m.n xyz reason" where m
  // and n are the major and minor protocol version numbers and xyz
  // is the reply code.

  // Unfortunately, there are servers out there (NCSA 1.5.1, to name
  // just one) that do not send a version number, so we can't rely
  // on finding one, but if we do, insist on it being 1.0 or 1.1.
  // We don't care about the reason phrase.

  if (strnicmp(buffer, "HTTP", 4) == 0)
    p = buffer + 4;
  else if(strnicmp(buffer, "ICY", 3) == 0)
    p = buffer + 3;
  else
    return HTTP_PROTOCOL_ERROR;

  if (*p == '/')
  { if (p[1] != '1' || p[2] != '.' || (p[3] != '0' && p[3] != '1'))
      return HTTP_PROTOCOL_ERROR;
    p += 4;
  }
  if (*p != ' ' || !isdigit(p[1]) || !isdigit(p[2]) || !isdigit(p[3]))
    return HTTP_PROTOCOL_ERROR;

  DEBUGLOG(("XIOhttp::http_read_reply: RC = %.4s\n", p));
  return (p[1] - '0') * 100 +
         (p[2] - '0') * 10  +
         (p[3] - '0');
}

/* Appends basic authorization string of the specified type
   to the request. */
void XIOhttp::http_basic_auth_to(xstringbuilder& result, const char* typname, const char* credentials)
{
  char* auth_encode = base64_encode(credentials);
  if (auth_encode)
  { result.append(typname);
    result.append(": Basic ");
    result.append(auth_encode);
    result.append("\r\n");
    free(auth_encode);
  }
}

/* Skips leading spaces. */
static char* skip_space(const char* p)
{ while (*p && isspace(*p))
    ++p;
  return (char*)p;
}

static time_t parse_datetime(const char* cp)
{ size_t len;
  char buffer[32];
  // Skip weekday
  if (!sscanf(cp, "%31s%n,%n", buffer, &len, &len))
    return -1;
  cp += len;
  // try different formats
  struct tm value;
  if ( !strptime(cp, "%d %b %Y %T", &value) // RFC 822, RFC 1123
    && !strptime(cp, "%d-%b-%y %T", &value) // RFC 850
    && !strptime(cp, "%b %d %T %Y", &value) ) // asctime
    return -1;
  // convert time
  return mktime(&value);
}

/* Opens the file specified by filename for reading. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
int XIOhttp::read_file(url123 url, int64_t range)
{
  xstringbuilder request(1024);

  int    rc;
  int    redirect;

  xstring proxy_auth(http_proxy_auth);
  xstring proxy(http_proxy);

  for (redirect = 0; redirect < HTTP_MAX_REDIRECT; redirect++)
  {
    int64_t r_size     = -1;
    time_t  r_mtime    = -1;
    int     r_metaint  = 0;
    XSFLAGS r_supports = support & ~XS_CAN_SEEK;
    int64_t r_pos      = 0;

    xstring r_genre;
    xstring r_name;
    xstring r_title;
    xstring r_type;

    request.clear();
    request.append("GET ");
    if (!proxy)
    { size_t len = request.length();
      url.appendComponentTo(request, url123::C_Request);
      // If there is no path at all append at least '/'.
      // Maybe this should better be up to normalizeURL.
      if (len == request.length())
        request.append('/');
    }
    else if (url.isScheme("ftp:"))
      url.appendComponentTo(request, url123::C_All);
    else
      url.appendComponentTo(request, url123::C_Scheme|url123::C_Host|url123::C_Request);
    request.append(" HTTP/1.1\r\n");

    request.append("Host: ");
    url.appendComponentTo(request, url123::C_Host);
    request.append("\r\n"
                   "User-Agent: XIO123/1.1 (OS/2)\r\n");
    //  "Accept: audio/mpeg, audio/x-mpegurl, */*\r\n"); causes problems

    if (proxy && proxy_auth)
      http_basic_auth_to(request, "Proxy-Authorization", proxy_auth);
    { xstringbuilder auth;
      url.appendComponentTo(auth, url123::C_Credentials);
      if (auth.length())
      { auth.erase(auth.length()-1); // remove @
        http_basic_auth_to(request, "Authorization", auth);
      }
    }

    if (range && (support & XS_CAN_SEEK))
      request.appendf("Range: bytes=%lli-\r\n", range);

    request.append("Icy-MetaData: 1\r\n"
                   "\r\n");

    if (proxy)
      rc = s_socket.open(proxy, XO_READWRITE);
    else
      rc = s_socket.open(url.getHost("80"), XO_READWRITE);
    if (rc == -1)
    { rc = HTTP_PROTOCOL_ERROR;
      break;
    }

    DEBUGLOG(("XIOhttp::read_file: request %s\n", request.cdata()));
    if (s_socket.write(request.cdata(), request.length()) == -1)
    { rc = HTTP_PROTOCOL_ERROR;
      break;
    }

    rc = http_read_reply();

    while (s_socket.gets(request.cdata(), request.capacity()) && *request)
    { if (strnicmp( request, "Location:", 9 ) == 0)
      { // Have new resource location.
        if ( rc == HTTP_MOVED_PERM ||
             rc == HTTP_MOVED_TEMP ||
             rc == HTTP_SEE_OTHER )
        { url.assign(skip_space(request + 9));
        }
      } else if (strnicmp(request, "Content-Length:", 15) == 0)
      { // Content-Length is used only if we don't have Content-Range.
        if (r_size < 0)
          r_size = atoll(skip_space(request + 15));
      } else if (strnicmp( request, "Accept-Ranges: bytes", 20) == 0)
      { r_supports |= XS_CAN_SEEK;
      } else if (strnicmp( request, "Content-Range: bytes", 20) == 0)
      { char* p = skip_space(request + 20);
        r_pos   = strtoull(p, &p, 10); ++p;
        r_size  = strtoull(p, &p, 10); ++p;
        r_size  = strtoull(p, &p, 10);
        r_supports |= XS_CAN_SEEK;
      } else if (strnicmp(request, "Content-Type:", 13) == 0)
      { r_type = skip_space(request + 13);
      } else if (strnicmp(request, "Last-Modified:", 14) == 0)
      { r_mtime = parse_datetime(request + 14 );
      } else if (strnicmp(request, "icy-metaint:", 12) == 0)
      { r_metaint = atol(skip_space(request + 12));
      } else if (strnicmp(request, "x-audiocast-name:", 17) == 0)
      { r_name = skip_space(request + 17);
      } else if (strnicmp(request, "x-audiocast-genre:", 18) == 0)
      { r_genre = skip_space(request + 18);
      } else if (strnicmp(request, "icy-name:", 9) == 0)
      { r_name = skip_space(request + 9);
      } else if (strnicmp(request, "icy-genre:", 10) == 0)
      { r_genre = skip_space(request + 10);
      }
    }

    if (rc == HTTP_MOVED_PERM ||
        rc == HTTP_MOVED_TEMP ||
        rc == HTTP_SEE_OTHER  )
      continue; // Redirect

    if (rc == HTTP_OK || rc == HTTP_PARTIAL)
    { if (r_metaint)
        r_supports &= ~XS_CAN_SEEK;

      Mutex::Lock lock(mtx_access);

      s_location = url;
      support   = r_supports;
      s_pos     = r_pos;
      s_size    = r_size;
      s_mtime   = r_mtime;
      s_metaint = r_metaint;
      s_metapos = r_metaint;

      s_genre   = r_genre;
      s_name    = r_name;
      s_title   = r_title;
      s_type    = r_type;

    } else
    { s_socket.close();
    }
    break;
  }

  if (rc == HTTP_OK || rc == HTTP_PARTIAL)
  { error = 0;
    eof = false;
    return 0;
  }
  else if (rc == HTTP_BAD_RANGE)
  { error = 0;
    eof = true;
    return 0;
  }

  if (redirect >= HTTP_MAX_REDIRECT)
    errno = error = HTTPBASEERR + HTTP_TOO_MANY_REDIR;
  else if (rc != HTTP_PROTOCOL_ERROR)
    errno = error = HTTPBASEERR + rc;

  return -1;
}

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
int XIOhttp::open(const char* filename, XOFLAGS oflags)
{ return read_file(url123(filename), 0);
}

/* Reads specified chunk of the data and notifies an attached
   observer about streaming metadata. Returns the number of
   bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error. */
int XIOhttp::read_and_notify(void* result, unsigned int count)
{
  int read_size;
  unsigned int read_done;
  int done;

  unsigned char metahead;
  int           metasize;
  char*         metabuff;
  char*         titlepos;

  DEBUGLOG2(("XIOhttp::read_and_notify(%p, %u) - %i, %i\n", result, count, s_metaint, s_metapos));

  if( !s_metaint ) {
    done = s_socket.read(result, count);
    if (done == -1)
      errno = error = s_socket.error;
    else if( done == 0 )
      eof = true;
    return done;
  }

  read_done = 0;

  while(read_done < count)
  {
    if( s_metapos == 0 )
    {
      // Time to read metadata from a input stream.
      metahead = 0;
      done = s_socket.read(&metahead, 1);

      if( done > 0 ) {
        if( metahead ) {
          metasize = metahead * 16;

          if(( metabuff = (char*)malloc( metasize + 1 )) == NULL ) {
            errno = error = ENOMEM;
            return -1;
          }
          done = s_socket.read( metabuff, metasize );
          if (done < 0)
          { errno = error = s_socket.error;
            free(metabuff);
            return -1;
          } else if(done == 0) {
            eof = true;
            free(metabuff);
            return 0;
          }

          metabuff[done] = 0;
          DEBUGLOG(("XIOhttp::read_and_notify: read meta data %s.\n", metabuff));
          Mutex::Lock lock(mtx_access);

          if ((titlepos = strstr(metabuff, "StreamTitle='")) != NULL)
          { titlepos += 13;
            char* cp = strstr(titlepos, "\';");
            if (cp)
              s_title.assign(titlepos, cp - titlepos);
          }

          s_pos -= metasize + 1;

          DEBUGLOG(("XIOhttp::read_and_notify: Callback! 2, %s, %li, 0, %p\n", metabuff, s_pos, observer));
          if (observer)
            observer->metacallback(XIO_META_TITLE, metabuff, s_pos);
          free(metabuff);
        }
        s_metapos = s_metaint;
      }
    }

    // Determines the maximum size of the data chunk for reading.
    read_size = count - read_done;
    read_size = min(read_size, s_metapos);

    done = s_socket.read((char*)result + read_done, read_size);
    if (done == -1)
    { errno = error = s_socket.error;
      return -1;
    } else if( done == 0 )
    { eof = true;
      break;
    }

    read_done += done;
    s_metapos -= done;
  }
  return read_done;
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
int XIOhttp::read(void* result, unsigned int count)
{
  int done = read_and_notify(result, count);

  if (done > 0)
    InterlockedAdd(&(volatile unsigned&)s_pos, done);

  return done;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
int XIOhttp::close()
{
  s_pos     = -1;
  s_size    = -1;
  s_mtime   = -1;
  return s_socket.close();
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
int64_t XIOhttp::tell()
{ // For now this is atomic
  // DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  return s_pos;
  // DosReleaseMutexSem( x->protocol->mtx_access );
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
int64_t XIOhttp::seek(int64_t offset, XIO_SEEK origin)
{
  if(!(support & XS_CAN_SEEK) || s_metaint)
  { errno = error = EINVAL;
    return -1;
  }
  int64_t range;

  switch (origin)
  { case XIO_SEEK_SET:
      range = offset;
      break;
    case XIO_SEEK_CUR:
      range = s_pos  + offset;
      break;
    case XIO_SEEK_END:
      if (s_size >= 0)
      { range = s_size + offset;
        break;
      }
    default:
      errno = EINVAL;
      return -1;
  }

  if ((s_size < 0 || range <= s_size) && s_location)
  { s_socket.close();
    errno = error = 0;
    eof   = false;

    if (range == s_size)
    { s_pos = range;
      error = 0;
      eof = true;
      return s_pos;
    } else if (read_file(s_location, range) == 0)
      return s_pos;
  }
  error = errno;
  return -1;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
int64_t XIOhttp::getsize()
{ // For now this is atomic
  // DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  return s_size;
  // DosReleaseMutexSem( x->protocol->mtx_access );
}

int XIOhttp::getstat( XSTATL* st )
{ // TODO: support file meta infos, modification time
  st->size = s_size;
  st->atime = -1;
  st->mtime = s_mtime;
  st->ctime = -1;
  st->attr = S_IAREAD; // The http client is always read only
  strlcpy(st->type, s_type, sizeof st->type);
  return 0;
}

xstring XIOhttp::get_metainfo(XIO_META type)
{ Mutex::Lock lock(mtx_access);
  switch (type)
  { case XIO_META_GENRE:
      return s_genre;
      break;
    case XIO_META_NAME:
      return s_name;
      break;
    case XIO_META_TITLE:
      return s_title;
      break;
    default:
      return xstring();
  }
}

XPROTOCOL::Iobserver* XIOhttp::set_observer(Iobserver* observer)
{ Mutex::Lock lock(mtx_access);
  return XIOreadonly::set_observer(observer);
}

XSFLAGS XIOhttp::supports() const
{ return support;
}

XIO_PROTOCOL XIOhttp::protocol() const
{ return XIO_PROTOCOL_HTTP;
}


/*XIOhttp::~XIOhttp()
{}*/

/* Initializes the http protocol. */
XIOhttp::XIOhttp()
: support(XS_CAN_READ | XS_CAN_SEEK),
  s_pos(-1),
  s_size(-1),
  s_mtime(-1),
  s_metaint(0),
  s_metapos(0)
{ blocksize = 8192;
}

/* Maps the error number in errnum to an error message string. */
const char* XIOhttp::strerror( int errnum )
{
  switch( errnum - HTTPBASEERR )
  {
    case HTTP_OK              : return "The request was successful.";
    case HTTP_CREATED         : return "The request was successful and a new resource was created.";
    case HTTP_ACCEPTED        : return "The request was accepted for processing.";
    case HTTP_NO_CONTENT      : return "No new information to be returned.";
    case HTTP_PARTIAL         : return "The server has fulfilled the partial request for the resource.";
    case HTTP_MULTIPLE        : return "The requested resource is available at one or more locations.";
    case HTTP_MOVED_PERM      : return "The requested resource has been assigned a new location.";
    case HTTP_MOVED_TEMP      : return "The requested resource resides at a different location.";
    case HTTP_SEE_OTHER       : return "The requested resource resides at a different location.";
    case HTTP_NOT_MODIFIED    : return "The requested resource has not been modified.";
    case HTTP_BAD_REQUEST     : return "The server could not properly interpret the request.";
    case HTTP_NEED_AUTH       : return "The request requires user authorization.";
    case HTTP_FORBIDDEN       : return "The server has refused the request.";
    case HTTP_NOT_FOUND       : return "The server cannot find the information specified in the request.";
    case HTTP_BAD_RANGE       : return "Requested range not satisfiable.";
    case HTTP_SERVER_ERROR    : return "The server encountered an unexpected condition.";
    case HTTP_NOT_IMPLEMENTED : return "The server does not support the requested feature.";
    case HTTP_BAD_GATEWAY     : return "The server received an invalid response.";
    case HTTP_UNAVAILABLE     : return "The server cannot process this request at the current time.";
    case HTTP_TOO_MANY_REDIR  : return "Too many redirections.";
    case HTTP_PROTOCOL_ERROR  : return "Unexpected socket error.";

    default:
      return "Unexpected HTTP protocol error.";
  }
}

static const char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
/* Base64 encoding. */
char* XIOhttp::base64_encode(const char* src)
{
  char*  str;
  char*  dst;
  size_t l;
  int    t;
  int    r;

  l = strlen( src );
  if(( str = (char*)malloc((( l + 2 ) / 3 ) * 4 + 1 )) == NULL ) {
    return NULL;
  }
  dst = str;
  r = 0;

  while( l >= 3 ) {
    t = ( src[0] << 16 ) | ( src[1] << 8 ) | src[2];
    dst[0] = base64[(t >> 18) & 0x3f];
    dst[1] = base64[(t >> 12) & 0x3f];
    dst[2] = base64[(t >>  6) & 0x3f];
    dst[3] = base64[(t >>  0) & 0x3f];
    src += 3; l -= 3;
    dst += 4; r += 4;
  }

  switch( l ) {
    case 2:
      t = ( src[0] << 16 ) | ( src[1] << 8 );
      dst[0] = base64[(t >> 18) & 0x3f];
      dst[1] = base64[(t >> 12) & 0x3f];
      dst[2] = base64[(t >>  6) & 0x3f];
      dst[3] = '=';
      dst += 4;
      r += 4;
      break;
    case 1:
      t = src[0] << 16;
      dst[0] = base64[(t >> 18) & 0x3f];
      dst[1] = base64[(t >> 12) & 0x3f];
      dst[2] = dst[3] = '=';
      dst += 4;
      r += 4;
      break;
    case 0:
      break;
  }

  *dst = 0;
  return str;
}

