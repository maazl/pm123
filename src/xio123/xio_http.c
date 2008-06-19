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
http_basic_auth_to( char* result, const char* typename,
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
    strlcat( result, typename, size );
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
static int
http_read_file( XFILE* x, const char* filename, unsigned long range )
{
  int    reqsize = 16384;
  char*  request = malloc( reqsize  );
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

  if( !request || !url || proxy_addr == -1 ) {
    free( request );
    url_free( url );
    return -1;
  }

  for( redirect = 0; redirect < HTTP_MAX_REDIRECT; redirect++ )
  {
    unsigned long r_size     = 0;
    int           r_metaint  = 0;
    int           r_supports = x->protocol->supports & ~XS_CAN_SEEK;
    unsigned long r_pos      = 0;

    char  r_genre[ sizeof( x->protocol->s_genre )] = "";
    char  r_name [ sizeof( x->protocol->s_name  )] = "";
    char  r_title[ sizeof( x->protocol->s_title )] = "";

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

    if( x->protocol->supports & XS_CAN_SEEK ) {
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
        strlcpy( r_name, skip_space( request + 17 ), sizeof( x->protocol->s_name ));
      } else if( strnicmp( request, "x-audiocast-genre:", 18 ) == 0 ) {
        strlcpy( r_genre, skip_space( request + 18 ), sizeof( x->protocol->s_genre ));
      } else if( strnicmp( request, "icy-name:", 9 ) == 0 ) {
        strlcpy( r_name, skip_space( request + 9 ), sizeof( x->protocol->s_name ));
      } else if( strnicmp( request, "icy-genre:", 10 ) == 0 ) {
        strlcpy( r_genre, skip_space( request + 10 ), sizeof( x->protocol->s_genre ));
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
      DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );

      free( x->protocol->s_location );
      if(!( x->protocol->s_location = url_string( url, XURL_STR_FULLAUTH | XURL_STR_ENCODE ))) {
        rc = HTTP_PROTOCOL_ERROR;
      }

      x->protocol->supports  = r_supports;
      x->protocol->s_handle  = handle;
      x->protocol->s_pos     = r_pos;
      x->protocol->s_size    = r_size;
      x->protocol->s_metaint = r_metaint;
      x->protocol->s_metapos = r_metaint;

      strcpy( x->protocol->s_genre, r_genre );
      strcpy( x->protocol->s_name,  r_name  );
      strcpy( x->protocol->s_title, r_title );

      DosReleaseMutexSem( x->protocol->mtx_access );
    } else {
      x->protocol->s_handle = -1;
      so_close( handle );
    }
    break;
  }

  url_free( url );
  free( request );

  if( rc == HTTP_OK     ||
      rc == HTTP_PARTIAL )
  {
    return 0;
  }

  if( redirect >= HTTP_MAX_REDIRECT ) {
    errno = HTTPBASEERR + HTTP_TOO_MANY_REDIR;
  } else if( rc != HTTP_PROTOCOL_ERROR ) {
    errno = HTTPBASEERR + rc;
  }

  return -1;
}

/* Opens the file specified by filename. Returns 0 if it
   successfully opens the file. A return value of -1 shows an error. */
static int
http_open( XFILE* x, const char* filename, int oflags ) {
  return http_read_file( x, filename, 0 );
}

/* Reads count bytes from the file into buffer. Returns the number
   of bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error.     */
static int
http_read( XFILE* x, char* result, unsigned int count )
{
  int done = so_read( x->protocol->s_handle, result, count );

  if( done > 0 ) {
    DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
    x->protocol->s_pos += done;
    DosReleaseMutexSem( x->protocol->mtx_access );
  }

  return done;
}

/* Writes count bytes from source into the file. Returns the number
   of bytes moved from the source to the file. The return value may
   be positive but less than count. A return value of -1 indicates an
   error */
static int
http_write( XFILE* x, const char* source, unsigned int count )
{
  errno = EBADF;
  return -1;
}

/* Closes the file. Returns 0 if it successfully closes the file. A
   return value of -1 shows an error. */
static int
http_close( XFILE* x )
{
  int rc = 0;

  if( x->protocol->s_handle != -1 ) {
    if(( rc = so_close( x->protocol->s_handle )) != -1 ) {
      x->protocol->s_handle  = -1;
    }
  }

  return rc;
}

/* Returns the current position of the file pointer. The position is
   the number of bytes from the beginning of the file. On devices
   incapable of seeking, the return value is -1L. */
static long
http_tell( XFILE* x )
{
  long pos;

  DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  pos = x->protocol->s_pos;
  DosReleaseMutexSem( x->protocol->mtx_access );
  return pos;
}

/* Moves any file pointer to a new location that is offset bytes from
   the origin. Returns the offset, in bytes, of the new position from
   the beginning of the file. A return value of -1L indicates an
   error. */
static long
http_seek( XFILE* x, long offset, int origin )
{
  if(!( x->protocol->supports & XS_CAN_SEEK )) {
    errno = EINVAL;
  } else {
    unsigned long range;

    switch( origin ) {
      case XIO_SEEK_SET:
        range = offset;
        break;
      case XIO_SEEK_CUR:
        range = x->protocol->s_pos  + offset;
        break;
      case XIO_SEEK_END:
        range = x->protocol->s_size + offset;
        break;
      default:
        return -1;
    }

    if( range <= x->protocol->s_size &&
        x->protocol->s_location      &&
        http_close( x ) == 0 )
    {
      if( range == x->protocol->s_size ) {
        return x->protocol->s_pos = range;
        return x->protocol->s_pos;
      } else if( http_read_file( x, x->protocol->s_location, range ) == 0 ) {
        return x->protocol->s_pos;
      }
    }
  }
  return -1;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
static long
http_size( XFILE* x )
{
  long size;

  DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  size = x->protocol->s_size;
  DosReleaseMutexSem( x->protocol->mtx_access );
  return size;
}

/* Lengthens or cuts off the file to the length specified by size.
   You must open the file in a mode that permits writing. Adds null
   characters when it lengthens the file. When cuts off the file, it
   erases all data from the end of the shortened file to the end
   of the original file. */
static int
http_truncate( XFILE* x, long size )
{
  errno = EINVAL;
  return -1;
}

/* Cleanups the http protocol. */
static void
http_terminate( XFILE* x )
{
  if( x->protocol ) {
    if( x->protocol->mtx_access ) {
      DosCloseMutexSem( x->protocol->mtx_access );
    }
    if( x->protocol->mtx_file ) {
      DosCloseMutexSem( x->protocol->mtx_file );
    }
    free( x->protocol->s_location );
    free( x->protocol );
  }
}

/* Initializes the http protocol. */
XPROTOCOL*
http_initialize( XFILE* x )
{
  XPROTOCOL* protocol = calloc( 1, sizeof( XPROTOCOL ));

  if( protocol ) {
    if( DosCreateMutexSem( NULL, &protocol->mtx_access, 0, FALSE ) != NO_ERROR ||
        DosCreateMutexSem( NULL, &protocol->mtx_file  , 0, FALSE ) != NO_ERROR )
    {
      http_terminate( x );
      return NULL;
    }

    protocol->supports =
      XS_CAN_READ | XS_CAN_SEEK | XS_USE_SPOS;

    protocol->open   = http_open;
    protocol->read   = http_read;
    protocol->write  = http_write;
    protocol->close  = http_close;
    protocol->tell   = http_tell;
    protocol->seek   = http_seek;
    protocol->chsize = http_truncate;
    protocol->size   = http_size;
    protocol->clean  = http_terminate;
  }

  return protocol;
}

/* Maps the error number in errnum to an error message string. */
const char* http_strerror( int errnum )
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

