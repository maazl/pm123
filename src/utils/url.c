/*
 * Copyright 2004 Dmitry A.Steklenev
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

#include "url.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/*--------------------------------------------------
 * Initializes URL
 *-------------------------------------------------*/
static void url_init( URL* url )
{
  url->scheme   = NULL;
  url->username = NULL;
  url->password = NULL;
  url->host     = NULL;
  url->port     = 0;
  url->path     = NULL;
  url->params   = NULL;
  url->query    = NULL;
  url->fragment = NULL;
}

/*--------------------------------------------------
 * Allocates URL
 *-------------------------------------------------*/
void url_allocate( URL* url, const char* original_url )
{
  char* parsed;
  char* tail;
  char* location = NULL;
  char* pc;

  url_init( url );

  if( !original_url )
    return;

  parsed   = strdup(original_url);
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

  if(( pc = strchr( tail, '#' )) != NULL )
  {
    url->fragment = strdup(pc+1);
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

  for( pc = tail; isalnum(*pc) || (*pc=='+') || (*pc=='-') || (*pc=='.'); ++pc )
  {}

  if( *pc == ':' && pc > tail )
  {
    *pc = 0;
    url->scheme = strdup(tail);
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

  if( *tail == '/' && *(tail+1) == '/' )
  {
    if(( pc = strchr( tail+2, '/' )) != NULL )
    {
      *pc      = 0;
      location = strdup(tail+2);
      *pc      = '/';
      tail     = pc;
    }
    else
    {
      location = strdup(tail+2);
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

  if(( pc = strchr( tail, '?' )) != NULL )
  {
    url->query = strdup(pc+1);
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

  if(( pc = strchr( tail, ';' )) != NULL )
  {
    url->params = strdup(pc+1);
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

  url->path = strdup(tail);

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

      if(( pc = strchr( location, ':' )) != NULL )
      {
        url->password = strdup(pc+1);
        *pc = 0;
      }

      url->username = strdup(location);
    }

    // The port number to connect to
    if(( pc = strchr( tail, ':' )) != NULL )
    {
      *pc = 0;
      url->port = atol(pc+1);
    }

    // Remainder is host
    url->host = strdup(tail);
  }

  free( location );
  free( parsed   );
}

/*--------------------------------------------------
 * Frees URL
 *-------------------------------------------------*/
void url_free( URL* url )
{
  free( url->scheme   );
  free( url->username );
  free( url->password );
  free( url->host     );
  free( url->path     );
  free( url->params   );
  free( url->query    );
  free( url->fragment );

  url_init( url );
}

/*--------------------------------------------------
 * Returns the URL string
 *--------------------------------------------------*/
char* url_full( URL* url )
{
  unsigned len = ( url->scheme   ? strlen(url->scheme  ) : 0 ) +
                 ( url->username ? strlen(url->username) : 0 ) +
                 ( url->password ? strlen(url->password) : 0 ) +
                 ( url->host     ? strlen(url->host    ) : 0 ) +
                 ( url->path     ? strlen(url->path    ) : 0 ) +
                 ( url->params   ? strlen(url->params  ) : 0 ) +
                 ( url->query    ? strlen(url->query   ) : 0 ) +
                 ( url->fragment ? strlen(url->fragment) : 0 ) + 32;

  char* full = (char*)malloc(len);
  *full = 0;

  if( url->scheme )
  {
    strcat( full, url->scheme );
    strcat( full, ":" );
  }

  if( url->username || url->host )
    strcat( full, "//" );

  if( url->username )
  {
    strcat( full, url->username );
    if( url->password )
    {
      strcat( full, ":" );
      strcat( full, url->password );
    }
    strcat( full, "@" );
  }

  if( url->host )
    strcat( full, url->host );

  if( url->port && url->host )
  {
    char buf[16];

    strcat( full, ":" );
    strcat( full, ltoa( url->port, buf, 10 ));
  }

  if( url->path )
    strcat( full, url->path );

  if( url->params )
  {
    strcat( full, ";" );
    strcat( full, url->params );
  }

  if( url->query )
  {
    strcat( full, "?" );
    strcat( full, url->query );
  }

  if( url->fragment )
  {
    strcat( full, "?" );
    strcat( full, url->query );
  }

  return full;
}

