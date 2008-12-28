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
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "xio_http.h"
#include "xio_url.h"
#include "xio_socket.h"
#include "xio.h"
#include <utilfct.h>

/* Get and parse HTTP reply. */
static int
http_read_reply( int s )
{
  char* p;
  char  buffer[64];

  if( !so_readline( s, buffer, sizeof( buffer ))) {
    return HTTP_PROTOCOL_ERROR;
  }

  // A valid status line looks like "HTTP/m.n xyz reason" where m
  // and n are the major and minor protocol version numbers and xyz
  // is the reply code.

  // Unfortunately, there are servers out there (NCSA 1.5.1, to name
  // just one) that do not send a version number, so we can't rely
  // on finding one, but if we do, insist on it being 1.0 or 1.1.
  // We don't care about the reason phrase.

  if( strnicmp( buffer, "HTTP", 4 ) == 0 ) {
    p = buffer + 4;
  } else if( strnicmp( buffer, "ICY", 3 ) == 0 ) {
    p = buffer + 3;
  } else {
    return HTTP_PROTOCOL_ERROR;
  }

  if( *p == '/' ) {
    if( p[1] != '1' || p[2] != '.' || ( p[3] != '0' && p[3] != '1' )) {
      return HTTP_PROTOCOL_ERROR;
    }
    p += 4;
  }
  if( *p != ' ' || !isdigit( p[1] ) || !isdigit( p[2] ) || !isdigit( p[3] )) {
    return HTTP_PROTOCOL_ERROR;
  }

  return ( p[1] - '0' ) * 100 +
         ( p[2] - '0' ) * 10  +
         ( p[3] - '0' );
}

/* Appends basic authorization string of the specified type
   to the request. */
static char*
http_basic_auth_to( char* result, const char* typname,
                                  const char* username,
                                  const char* password, int size )
{
  char  auth_string[ XIO_MAX_USERNAME + XIO_MAX_PASSWORD ];
  char* auth_encode;

  if( username ) {
    strlcpy( auth_string, username, sizeof( auth_string ));
  }

  strlcat( auth_string, ":", sizeof( auth_string ));

  if( password ) {
    strlcat( auth_string, password, sizeof( auth_string ));
  }

  auth_encode = so_base64_encode( auth_string );

  if( auth_encode ) {
    strlcat( result, typname, size );
    strlcat( result, ": Basic ", size );
    strlcat( result, auth_encode, size );
    strlcat( result, "\r\n", size );
    free( auth_encode );
  }

  return result;
}

/* Skips leading spaces. */
static char*
skip_space( char* p )
{
  while( *p && isspace(*p)) {
    ++p;
  }
  return p;
}

/* Opens the file specified by filename for reading. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
int XIOhttp::read_file( const char* filename, unsigned long range )
{
  int    reqsize = 16384;
  char*  request = (char*)malloc( reqsize  );
  int    rc;
  int    handle;
  char*  get;
  int    redirect;
  char   string[64];

  XURL*  url = url_allocate( filename );

  char   proxy_user[ XIO_MAX_USERNAME ];
  char   proxy_pass[ XIO_MAX_PASSWORD ];
  int    proxy_port = xio_http_proxy_port();
  u_long proxy_addr = xio_http_proxy_addr();

  xio_http_proxy_user( proxy_user, sizeof( proxy_user ));
  xio_http_proxy_pass( proxy_pass, sizeof( proxy_pass ));

  if( !request || !url || proxy_addr == (u_long)-1 ) {
    free( request );
    url_free( url );
    errno = error = HTTP_PROTOCOL_ERROR;
    return -1;
  }

  for( redirect = 0; redirect < HTTP_MAX_REDIRECT; redirect++ )
  {
    unsigned long r_size     = 0;
    int           r_metaint  = 0;
    XSFLAGS       r_supports = support & ~XS_CAN_SEEK;
    unsigned long r_pos      = 0;

    char  r_genre[ sizeof( s_genre )] = "";
    char  r_name [ sizeof( s_name  )] = "";
    char  r_title[ sizeof( s_title )] = "";

    if( !proxy_addr ) {
      get = url_string( url, XURL_STR_ENCODE | XURL_STR_REQUEST );
    } else {
      if( stricmp( url->scheme, "ftp" ) == 0 ) {
        get = url_string( url, XURL_STR_ENCODE | XURL_STR_FULLAUTH );
      } else {
        get = url_string( url, XURL_STR_ENCODE | XURL_STR_FULL );
      }
    }

    if( !get ) {
      rc = HTTP_PROTOCOL_ERROR;
      break;
    }

    strlcpy( request, "GET ", reqsize );
    strlcat( request, get, reqsize );
    strlcat( request, " HTTP/1.0\r\nUser-Agent: XIO123/1.0 (OS/2)\r\n", reqsize );
    strlcat( request, "Accept: audio/mpeg, audio/x-mpegurl, */*\r\n", reqsize );
    free( get );

    if( url->host ) {
      strlcat( request, "Host: ", reqsize );
      strlcat( request, url->host, reqsize );
      strlcat( request, "\r\n", reqsize );
    }

    if( proxy_addr && ( *proxy_user || *proxy_pass )) {
      http_basic_auth_to( request, "Proxy-Authorization", proxy_user, proxy_pass, reqsize );
    }
    if( url->username || url->password ) {
      http_basic_auth_to( request, "Authorization", url->username, url->password, reqsize );
    }

    if( support & XS_CAN_SEEK ) {
      strlcat( request, "Range: bytes=", reqsize );
      strlcat( request, ltoa( range, string, 10 ), reqsize );
      strlcat( request, "-\r\n", reqsize );
    }

    strlcat( request, "icy-metadata:1\r\n", reqsize );
    strlcat( request, "\r\n", reqsize );

    if( proxy_addr ) {
      handle = so_connect( proxy_addr, proxy_port );
    } else {
      handle = so_connect( so_get_address( url->host ), url->port ? url->port : 80 );
    }

    if( handle == -1 ) {
      rc = HTTP_PROTOCOL_ERROR;
      break;
    }

    if( so_write( handle, request, strlen( request )) == -1 ) {
      rc = HTTP_PROTOCOL_ERROR;
      break;
    }

    rc = http_read_reply( handle );

    while( so_readline( handle, request, reqsize ) && *request ) {
      if( strnicmp( request, "Location:", 9 ) == 0 ) {
        // Have new resource location.
        if( rc == HTTP_MOVED_PERM ||
            rc == HTTP_MOVED_TEMP ||
            rc == HTTP_SEE_OTHER   )
        {
          url_free( url );
          if(!( url = url_allocate( skip_space( request + 9 )))) {
            rc = HTTP_PROTOCOL_ERROR;
          }
        }
      } else if( strnicmp( request, "Content-Length:", 15 ) == 0 ) {
        // Content-Length is used only if we don't have Content-Range.
        if( !r_size ) {
          r_size = atol( skip_space( request + 15 ));
        }
      } else if( strnicmp( request, "Accept-Ranges: bytes", 20 ) == 0 ) {
        r_supports |= XS_CAN_SEEK;
      } else if( strnicmp( request, "Content-Range: bytes", 20 ) == 0 ) {
        char* p = skip_space( request + 20 );
        r_pos   = strtoul( p, &p, 10 ); ++p;
        r_size  = strtoul( p, &p, 10 ); ++p;
        r_size  = strtoul( p, &p, 10 );
        r_supports |= XS_CAN_SEEK;
      } else if( strnicmp( request, "icy-metaint:", 12 ) == 0 ) {
        r_metaint = atol( skip_space( request + 12 ));
      } else if( strnicmp( request, "x-audiocast-name:", 17 ) == 0 ) {
        strlcpy( r_name, skip_space( request + 17 ), sizeof( s_name ));
      } else if( strnicmp( request, "x-audiocast-genre:", 18 ) == 0 ) {
        strlcpy( r_genre, skip_space( request + 18 ), sizeof( s_genre ));
      } else if( strnicmp( request, "icy-name:", 9 ) == 0 ) {
        strlcpy( r_name, skip_space( request + 9 ), sizeof( s_name ));
      } else if( strnicmp( request, "icy-genre:", 10 ) == 0 ) {
        strlcpy( r_genre, skip_space( request + 10 ), sizeof( s_genre ));
      }
    }

    if( rc == HTTP_MOVED_PERM ||
        rc == HTTP_MOVED_TEMP ||
        rc == HTTP_SEE_OTHER  )
    {
      continue;
    }

    if( rc == HTTP_OK     ||
        rc == HTTP_PARTIAL )
    {
      if (r_metaint)
        r_supports &= XS_CAN_SEEK;

      Mutex::Lock lock(mtx_access);

      free( s_location );
      if(!( s_location = url_string( url, XURL_STR_FULLAUTH | XURL_STR_ENCODE ))) {
        rc = HTTP_PROTOCOL_ERROR;
      }

      support   = r_supports;
      s_handle  = handle;
      s_pos     = r_pos;
      s_size    = r_size;
      s_metaint = r_metaint;
      s_metapos = r_metaint;

      strcpy( s_genre, r_genre);
      strcpy( s_name,  r_name );
      strcpy( s_title, r_title);

    } else {
      s_handle = -1;
      so_close( handle );
    }
    break;
  }

  url_free( url );
  free( request );

  if( rc == HTTP_OK     ||
      rc == HTTP_PARTIAL )
  {
    error = 0;
    eof = false;
    return 0;
  }

  if( redirect >= HTTP_MAX_REDIRECT ) {
    errno = error = HTTPBASEERR + HTTP_TOO_MANY_REDIR;
  } else if( rc != HTTP_PROTOCOL_ERROR ) {
    errno = error = HTTPBASEERR + rc;
  }

  return -1;
}

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
int XIOhttp::open( const char* filename, XOFLAGS oflags ) {
  return read_file( filename, 0 );
}

/* Reads specified chunk of the data and notifies an attached
   observer about streaming metadata. Returns the number of
   bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error. */
int XIOhttp::read_and_notify( void* result, unsigned int count )
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
    return so_read( s_handle, result, count );
  }

  read_done = 0;

  while( read_done < count ) {
    if( s_metapos == 0 )
    {
      // Time to read metadata from a input stream.
      metahead = 0;
      done = so_read( s_handle, &metahead, 1 );

      if( done > 0 ) {
        if( metahead ) {
          metasize = metahead * 16;

          if(( metabuff = (char*)malloc( metasize + 1 )) == NULL ) {
            errno = error = ENOMEM;
            return -1;
          }
          done = so_read( s_handle, metabuff, metasize );
          if (done < 0)
          { errno = error = HTTP_PROTOCOL_ERROR;
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

          if(( titlepos = strstr( metabuff, "StreamTitle='" )) != NULL )
          { size_t i;
            titlepos += 13;
            for( i = 0; i < sizeof( s_title ) - 1 && *titlepos
                        && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
            {
              s_title[i] = *titlepos++;
            }

            s_title[i] = 0;
          }

          s_pos -= ( metasize + 1 );

          DEBUGLOG(("XIOhttp::read_and_notify: Callback! %s, %li, 0, %p\n", metabuff, s_pos, s_arg));
          if( s_callback )
          {
            s_callback( metabuff, s_pos, 0, s_arg );
          }
          free(metabuff);
        }
        s_metapos = s_metaint;
      }
    }

    // Determines the maximum size of the data chunk for reading.
    read_size = count - read_done;
    read_size = min( read_size, s_metapos );

    done = so_read( s_handle, (char*)result + read_done, read_size );

    if (done == -1)
    { errno = error = HTTP_PROTOCOL_ERROR;
      return -1;
    } else if( done == 0 ) {
      eof = true;
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
int XIOhttp::read( void* result, unsigned int count )
{
  int done = read_and_notify( result, count );

  if( done > 0 )
    InterlockedAdd((unsigned&)s_pos, done);

  return done;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
int XIOhttp::close()
{
  int rc = 0;

  if( s_handle != -1 ) {
    if(( rc = so_close( s_handle )) != -1 ) {
      s_handle  = -1;
      s_pos     = (unsigned long)-1;
      s_size    = (unsigned long)-1;
    }
  }

  return rc;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
long XIOhttp::tell( long* offset64 )
{
  long pos;

  // For now this is atomic
  // DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  pos = s_pos;
  // DosReleaseMutexSem( x->protocol->mtx_access );
  // TODO: 64 bit
  if (offset64)
    *offset64 = 0;
  return pos;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
long XIOhttp::seek( long offset, int origin, long* offset64 )
{
  if(!( support & XS_CAN_SEEK ) || s_metaint ) {
    errno = error = EINVAL;
  } else {
    unsigned long range;

    switch( origin ) {
      case XIO_SEEK_SET:
        range = offset;
        break;
      case XIO_SEEK_CUR:
        range = s_pos  + offset;
        break;
      case XIO_SEEK_END:
        range = s_size + offset;
        break;
      default:
        errno = EINVAL;
        return -1;
    }

    if( range <= s_size &&
        s_location      &&
        close() == 0 )
    {
      // TODO: 64 bit
      if (offset64)
        *offset64 = 0;
      errno = error = 0;
      eof   = false;

      if( range == s_size ) {
        s_pos = range;
        error = 0;
        eof = true;
        return s_pos;
      } else if( read_file( s_location, range ) == 0 ) {
        return s_pos;
      }
    }
  }
  error = errno;
  return -1;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long XIOhttp::getsize( long* offset64 )
{
  long size;

  // For now this is atomic
  // DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  size = s_size;
  // DosReleaseMutexSem( x->protocol->mtx_access );
  // TODO: 64 bit
  if (offset64)
    *offset64 = 0;
  return size;
}

char* XIOhttp::get_metainfo( int type, char* result, int size )
{
  Mutex::Lock lock(mtx_access);
  switch( type ) {
    case XIO_META_GENRE : strlcpy( result, s_genre, size ); break;
    case XIO_META_NAME  : strlcpy( result, s_name , size ); break;
    case XIO_META_TITLE : strlcpy( result, s_title, size ); break;
    default:
      *result = 0;
  }
  return result;
}

void XIOhttp::set_observer( void DLLENTRYP(callback)(const char* metabuff, long pos, long pos64, void* arg), void* arg )
{ Mutex::Lock lock(mtx_access);
  s_callback = callback;
  s_arg = arg;
}

XSFLAGS XIOhttp::supports() const
{ return support;
}

/* Cleanups the http protocol. */
XIOhttp::~XIOhttp()
{
  free( s_location );
}

/* Initializes the http protocol. */
XIOhttp::XIOhttp()
: support(XS_CAN_READ | XS_CAN_SEEK),
  s_handle(-1),
  s_pos((unsigned long)-1),
  s_size((unsigned long)-1),
  s_metaint(0),
  s_metapos(0),
  s_location(NULL),
  s_callback(NULL)
{
  memset(s_genre, 0, sizeof s_genre);
  memset(s_name , 0, sizeof s_name );
  memset(s_title, 0, sizeof s_title);
  blocksize = 4096;
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

