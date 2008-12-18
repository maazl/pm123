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
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "xio_url.h"
#include <utilfct.h>

#include <debuglog.h>


/* Passed any string value, decode from URL transmission. */
char*
url_decode( char* string )
{
  const char* digits = "0123456789ABCDEF";
  const char* p;
  const char* ps = string;
  char* pr = string;
  int   i;

  if( !string ) {
    return NULL;
  }

  while( *ps )
  {
    if( *ps == '%' ) {
      if( *ps && ( p = strchr( digits, toupper( *++ps ))) != NULL ) {
        i = p - digits;
        if( *ps && ( p = strchr( digits, toupper( *++ps ))) != NULL ) {
          i = i * 16 + p - digits;
          *pr = (char)i;
        }
      }
    } else {
      *pr = *ps;
    }
    ++ps;
    ++pr;
  }
  *pr = 0;
  return string;
}

/* Passed any string value, encode for URL transmission. */
size_t
url_encode( char* result, const char* source, size_t size )
{
  const  char* digits = "0123456789ABCDEF";
  const  char* encode = " !\"#$%&'(),:;<=>?{\\}~+";
  const  char* ps = source;
  char*  pr;
  size_t done = 0;

  if( !result || !source ) {
    return 0;
  }

  pr = result + strlen( result );

  if( size ) {
    while( *ps && done < size )
    {
      if( strchr( encode, *ps )) {
        if( size - done < 3 ) {
          break;
        }
        *pr++ = '%';
        *pr++ = digits[ ((unsigned)*ps)/16 ];
        *pr++ = digits[ ((unsigned)*ps)%16 ];
        done += 3;
      } else {
        *pr++ = *ps;
        done++;
      }
      ++ps;
    }
  }
  *pr = 0;
  return done;
}

/* Calculates a length of any string value encoded for
   URL transmission. */
size_t
url_encode_size( const char* string )
{
  const char* encode = " !\"#$%&'(),:;<=>?{\\}~+";
  int size = 0;

  if( string ) {
    while( *string )
    {
      if( strchr( encode, *string )) {
        size += 3;
      } else {
        size += 1;
      }
      ++string;
    }
  }

  return size;
}

/* Calculates a length of any string value. */
static size_t
url_string_size( const char* string ) {
  return string ? strlen( string ) : 0;
}

/* Allocates a URL structure. */
XURL* url_allocate( const char* string )
{ char* parsed;
  char* tail;
  char* location = NULL;
  char* pc;
  XURL* url = (XURL*)calloc( 1, sizeof( XURL ));

  DEBUGLOG(("xio:url_allocate(%s) -> %p\n", string, url));

  if( !url || !string ) {
    return url;
  }

  parsed   = strdup( string );
  location = NULL;
  tail     = parsed;

  // 1. Remove fragment identifier, if any, from the back
  //
  // If the parse string contains a '#', then the substring
  // after the first '#' and up to the end of the parse string
  // is the fragment identifier. If the '#' is the last
  // character, or if no '#' is present, then the fragment
  // identifier is empty. The matched substring, including
  // the '#' is removed from the parse string before
  // continuing.

  if(( pc = strchr( tail, '#' )) != NULL ) {
    url->fragment = url_decode( strdup( pc + 1 ));
    *pc = 0;
  }

  // 2. Parse scheme on the front
  //
  // If the parse string contain a colon after the 1st character
  // and before any characters not allowed as part of a scheme
  // name (i.e. any not alphanumeric, '+', '.' or '-'),
  // the scheme of the url is the substring of chars up to
  // but not including the first colon. These chars and the
  // colon are then removed from the parse string before
  // continuing.

  // Skip over characters allowed in the scheme name

  for( pc = tail; isalnum( *pc ) || ( *pc == '+' ) || ( *pc == '-' ) || ( *pc == '.' ); ++pc )
  {}

  if( *pc == ':' && pc > tail )
  {
    *pc = 0;
    url->scheme = strdup( tail );
    tail = pc + 1;
  }

  // 3. Parse location (user:password@host:port) on the front
  //
  // If the parse string begins with '//', then the substring
  // of characters after '//' and up to, but not including
  // the next '/' is the network location/login of the url.
  // If no trailing '/' is present, the entire remaining
  // parse string is assigned to the network location.
  // The '//' and the network location are removed from
  // the parse string before continuing.

  if( *tail == '/' && *( tail + 1 ) == '/' ) {
    if(( pc = strchr( tail + 2, '/' )) != NULL ) {
      *pc      = 0;
      location = strdup( tail + 2 );
      *pc      = '/';
      tail     = pc;
    } else {
      location = strdup( tail + 2 );
      tail     = "";
    }
  }

  // 4. Parse query information from the back
  //
  // If the parse string contains a '?', then the substring after
  // the first '?' and up to the end of the parse string is
  // the query information. If the '?' is the last character, or
  // if no '?' is present, then the query information is empty.
  // The matched substring, including the '?', is removed from
  // the parse string before continuing.

  if(( pc = strchr( tail, '?' )) != NULL ) {
    url->query = strdup( pc + 1 );
    *pc = 0;
  }

  // 5. Parse parameters from the back
  //
  // If the parse string contains a ';', then the substring
  // after the first ';' and up to the end of the parse string
  // is the parameters. If the ';' is the last character, or
  // if no ';' is present, then parameters is empty.
  // The matched substring, including the ';' is removed from
  // the parse string before continuing.

  if(( pc = strchr( tail, ';' )) != NULL ) {
    url->params = url_decode( strdup( pc + 1 ));
    *pc = 0;
  }

  // 6. Remainder is path
  //
  // All that is left of the parse string is the url path
  // and the '/' that may precede it. Even though the
  // initial '/' is not part of the url path, the parser
  // must remember whether or not it was present so that
  // later processes can differentiate between relative
  // and absolute paths. Often this is done by simply
  // storing the preceding '/' along with the path.

  if( *tail ) {
    url->path = url_decode( strdup( tail ));
  }

  // 7. Parse location
  //
  // The user name (and password), if present, are followed by a
  // commercial at-sign "@". Within the user and password field, any ":",
  // "@", or "/" must be encoded. Note that an empty user name or
  // password is different than no user name or password; there
  // is no way to specify a password without specifying a user name.

  if( location )
  {
    tail = location;

    if(( pc = strchr( location, '@' )) != NULL )
    {
      *pc  = 0;
      tail = pc+1;

      if(( pc = strchr( location, ':' )) != NULL ) {
        url->password = url_decode( strdup( pc + 1 ));
        *pc = 0;
      }

      url->username = url_decode( strdup( location ));
    }

    // The port number to connect to
    if(( pc = strchr( tail, ':' )) != NULL ) {
      *pc = 0;
      url->port = atol( pc + 1 );
    }

    // Remainder is host
    url->host = strdup( tail );
  }

  free( location );
  free( parsed   );

  return url;
}

/* Frees a URL structure. */
void url_free( XURL* url )
{
  DEBUGLOG(("xio:url_free(%p)\n", url));

  if( url ) {
    free( url->scheme   );
    free( url->username );
    free( url->password );
    free( url->host     );
    free( url->path     );
    free( url->params   );
    free( url->query    );
    free( url->fragment );
    free( url );
  }
}

// Proxy because of ICC calling convention
static size_t url_cat(char* result, const char* source, size_t size)
{ return strlcat(result, source, size);
}

/* Returns the specified part of the URL string. */
char* url_string( XURL* url, int part )
{
  int   size;
  char* string;

  if( !url ) {
    return NULL;
  }
  
  size_t (*urllen)( const char* string );
  size_t (*urlcat)( char* result, const char* source, size_t size );

  if( part & XURL_STR_ENCODE ) {
    urllen = url_encode_size;
    urlcat = url_encode;
  } else {
    urllen = url_string_size;
    urlcat = url_cat;
  }

  part &= ~XURL_STR_ENCODE;

  if( part == XURL_STR_FULLAUTH )
  {
    size = urllen( url->scheme   ) +
           urllen( url->username ) +
           urllen( url->password ) +
           urllen( url->host     ) +
           urllen( url->path     ) +
           urllen( url->params   ) +
           urllen( url->query    ) +
           urllen( url->fragment ) + 32;
  }
  else if( part == XURL_STR_FULL )
  {
    size = urllen( url->scheme   ) +
           urllen( url->host     ) +
           urllen( url->path     ) +
           urllen( url->params   ) +
           urllen( url->query    ) +
           urllen( url->fragment ) + 32;
  }
  else if( part == XURL_STR_REQUEST )
  {
    size = urllen( url->path     ) +
           urllen( url->params   ) +
           urllen( url->query    ) +
           urllen( url->fragment ) + 4;
  } else {
    return NULL;
  }

  if(!( string = (char*)malloc(size))) {
    return NULL;
  }

  *string = 0;

  if( part == XURL_STR_FULLAUTH ||
      part == XURL_STR_FULL )
  {
    if( url->scheme ) {
      strcat( string, url->scheme );
      strcat( string, ":" );
    }
    if(( url->username && part == XURL_STR_FULLAUTH ) || url->host ) {
      strcat( string, "//" );
    }

    if( part == XURL_STR_FULLAUTH ) {
      if( url->username ) {
        urlcat( string, url->username, size );
        if( url->password ) {
          strcat( string, ":" );
          urlcat( string, url->password, size );
        }
        strcat( string, "@" );
      }
    }
    if( url->host ) {
      strcat( string, url->host );
    }
    if( url->port && url->host )
    {
      char buf[16];

      strcat( string, ":" );
      strcat( string, ltoa( url->port, buf, 10 ));
    }
  }

  if( url->path ) {
    urlcat( string, url->path, size );
  } else if( part == XURL_STR_REQUEST ) {
    strcat( string, "/" );
  }
  if( url->params ) {
    strcat( string, ";" );
    urlcat( string, url->params, size );
  }
  if( url->query ) {
    strcat( string, "?" );
    strcat( string, url->query );
  }
  if( url->fragment ) {
    strcat( string, "#" );
    urlcat( string, url->fragment, size );
  }

  return string;
}

