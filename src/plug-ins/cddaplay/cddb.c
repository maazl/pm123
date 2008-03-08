/*
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <direct.h>

#include "cddb.h"
#include "utilfct.h"
#include "snprintf.h"
#include "debuglog.h"

/* Creates a new CDDB connection structure. Returns a pointer to a
   CDDB structure that can be used to access the database. A NULL
   pointer return value indicates an error. */
CDDB_CONNECTION*
cddb_connect( void )
{
  CDDB_CONNECTION* c = calloc( sizeof( CDDB_CONNECTION ), 1 );

  if( c ) {
    strcpy( c->servname, "cddbp://freedb.freedb.org:8880" );
  }

  return c;
}

/* Closes CDDB connection after successful reception of the response.
   Returns 0 if it successfully closes the connection, or -1 if any
   errors were detected. */
static int
cddb_close_handle( CDDB_CONNECTION* c )
{
  int rc = 0;

  if( c->handle ) {
    rc = xio_fclose( c->handle );
    c->handle = NULL;
  }

  return rc;
}

/* Free all track titles associated with the given CDDB
   connection structure. */
static void
cddb_free_titles( CDDB_CONNECTION* c )
{
  int  i;
  for( i = 0; i < MAX_TRACKS; i++ ) {
    free( c->track_title[i] );
    c->track_title[i] = NULL;
  }
}

/* Free all resources associated with the given CDDB
   connection structure. */
void
cddb_close( CDDB_CONNECTION* c )
{
  cddb_close_handle( c );
  cddb_free_titles ( c );
  free( c );
}

/* Sets the user name and host name of the local machine. */
void
cddb_set_email_address( CDDB_CONNECTION* c, const char* email )
{
  char* hostpos = strchr( email, '@' );
  int   userlen = hostpos - email + 1;

  *c->hostname = 0;
  *c->username = 0;

  if( hostpos ) {
    strlcpy( c->username, email, min(  sizeof( c->username ), userlen ));
    strlcpy( c->hostname, hostpos + 1, sizeof( c->hostname ));
  } else {
    strlcpy( c->username, email, sizeof( c->username ));
  }
}

/* Creates a new directory with the specified pathname. Because only
   one directory can be created at a time, only the last component
   of pathname can name a new directory. Returns the value 0 if the
   directory was created or already exist. A return value of -1 indicates
   an error. */
static int
cddb_mkdir( const char* pathname )
{
  struct stat fi;

  if( stat( pathname, &fi ) == 0 && ( fi.st_mode & S_IFDIR )) {
    return 0;
  }
  #if defined(__GNUC__)
  if( mkdir((char*)pathname, 0755 ) == 0 ) {
  #else
  if( mkdir((char*)pathname ) == 0 ) {
  #endif
    return 0;
  }

  return -1;
}

/* Builds the name of the cached file. Returns a pointer to the
   name of the cached file. A NULL pointer return value indicates
   an error. */
static char*
cddb_cache_build_name( CDDB_CONNECTION* c, char* result,
                       const char* category, ULONG discid, int size )
{
  if( *c->cachedir )
  {
    snprintf( result, size, "%s\\%s", c->cachedir, category );

    if( cddb_mkdir( result ) == -1 ) {
      return NULL;
    }

    snprintf( result, size, "%s\\%s\\%08lx", c->cachedir, category, discid );
    return result;
  } else {
    return NULL;
  }
}

/* Changes the directory used for caching CDDB entries locally. Returns
   a value of 0 if the cache directory was successfully changed. A
   return value of -1 indicates an error. */
int
cddb_set_cache_dir( CDDB_CONNECTION* c, const char* pathname )
{
  if( cddb_mkdir( pathname ) == 0 ) {
    strlcpy( c->cachedir, pathname, sizeof( c->cachedir ));
    return  0;
  } else {
    return -1;
  }
}

/* Sets the CDDB server. */
void
cddb_set_server( CDDB_CONNECTION* c, const char* url )
{
  char type[64];

  cddb_close_handle( c );
  cddb_free_titles ( c );

  scheme( type, url, sizeof( type ));

  if( stricmp( type, "http:"  ) == 0 ||
      stricmp( type, "cddbp:" ) == 0 )
  {
    strlcpy ( c->servname, url, sizeof( c->servname ));
  } else if( !*type ) {
    snprintf( c->servname, sizeof( c->servname ), "cddbp://%s", url );
  }
}

/* Gets and parses CDDB reply. Returns a CDDB server response code.
   If the response code differs from CDDB_PROTOCOL_ERROR places the text
   following for a code in the specified buffer. */
static CDDBRC
cddb_read_reply( CDDB_CONNECTION* c, char* reply, int size )
{
  char buffer[1024];

  if( !xio_fgets( buffer, sizeof( buffer ), c->handle )) {
    return CDDB_PROTOCOL_ERROR;
  }

  if( !isdigit( buffer[0] ) || !isdigit( buffer[1] ) || !isdigit( buffer[2] )) {
    return CDDB_PROTOCOL_ERROR;
  }

  if( reply ) {
    strlcpy( reply, buffer + 4, size );
  }

  return ( buffer[0] - '0' ) * 100 +
         ( buffer[1] - '0' ) * 10  +
         ( buffer[2] - '0' );
}

/* Sends a query to a CDDB server. Returns a CDDB server response code. */
static CDDBRC
cddb_send_query( CDDB_CONNECTION* c, const char* query, char* reply, int size )
{
  char   buffer[1024];
  CDDBRC rc;

  cddb_close_handle( c );
  snprintf( buffer, sizeof( buffer ), "%s/?cmd=%s&hello=%s+%s+%s+%s&proto=%d",
            c->servname, query, c->username, c->hostname, PACKAGE_NAME, PACKAGE_VERSION, CDDB_PROTOLEVEL );

  DEBUGLOG(( "cddaplay: send CDDB query %s\n", buffer ));

  if(( c->handle = xio_fopen( buffer, "r" )) == NULL ) {
    return CDDB_PROTOCOL_ERROR;
  }

  rc = cddb_read_reply( c, reply, size );

  DEBUGLOG(( "cddaplay: CDDB reply is %d\n", rc ));
  return rc;
}

/* Receives a next line of the CDDB server response. Returns CDDB_OK
   if successful. A CDDB_PROTOCOL_ERROR return value indicates an error
   or an end-of-stream condition. */
static CDDBRC
cddb_readline( CDDB_CONNECTION* c, char* buffer, int size )
{
  if( c->handle ) {
    if( xio_fgets( buffer, size, c->handle )) {
      return CDDB_OK;
    }
  }
  return CDDB_PROTOCOL_ERROR;
}

/* Removes trailing \n. */
static char*
cddb_strip_newline( char* string )
{
  int  i;
  for( i = strlen( string ) - 1; i > 0; i-- ) {
    if( string[i] == '\n' ) {
      string[i] = 0;
    } else {
      break;
    }
  }

  return string;
}

/* Retrieve the first CDDB mirror server. Returns the CDDB_OK if it
   successfully retrieves the server. */
CDDBRC
cddb_mirror( CDDB_CONNECTION* c, char* url, int size )
{
  if( cddb_send_query( c, "sites", NULL, 0 ) != CDDB_DATA_FOLLOW ) {
    return CDDB_PROTOCOL_ERROR;
  }

  return cddb_mirror_next( c, url, size );
}

/* Retrieve the next CDDB mirror server. Returns the CDDB_OK if it
   successfully retrieves the server. */
CDDBRC
cddb_mirror_next( CDDB_CONNECTION* c, char* url, int size )
{
  char reply[2048];

  if( cddb_readline( c, reply, sizeof( reply )) == CDDB_OK )
  {
    char prot[8]   = "cddbp";
    char host[512] = "";
    int  port      = 8880;
    char path[512] = "";
    char desc[512] = "";

    // CDDB use terminating marker: `.' character in the
    // beginning of a line.
    if( *reply == '.' ) {
      cddb_close_handle( c );
      return CDDB_END_OF_DATA;
    }

    #if CDDB_PROTOLEVEL > 2
      sscanf( reply, "%511s %7s %d %511s %*s %*s %511s\n",
              host, prot, &port, path, desc );
    #else
      sscanf( reply, "%511s %d %* %* %511s\n",
              host, &port, desc );
    #endif

    if( stricmp( path, "-"  ) != 0 ) {
      snprintf( url, size, "%s://%s:%d%s", prot, host, port, path );
    } else {
      snprintf( url, size, "%s://%s:%d",   prot, host, port );
    }

    return CDDB_OK;
  } else {
    return CDDB_PROTOCOL_ERROR;
  }
}

/* Query the CDDB local cache for a list of possible disc matches.
   Returns the CDDB_OK if it founds exact match. */
static CDDBRC
cddb_cache_disc( CDDB_CONNECTION* c, CDINFO* cdinfo, char* match, int size )
{
  HDIR         hdir;
  FILEFINDBUF3 find;
  char         findname[_MAX_PATH];
  APIRET       findrc;
  struct stat  fi;

  if( !*c->cachedir ) {
    return CDDB_NOT_MATCH;
  }

  strlcpy( findname, c->cachedir, sizeof( findname ));
  strlcat( findname, "\\*", sizeof( findname ));

  findrc = findfirst( &hdir, findname, FIND_DIRECTORY, &find );

  while( findrc == NO_ERROR ) {
    if( strcmp( find.achName, "."  ) != 0 && strcmp( find.achName, ".." ) != 0 ) {
      if( cddb_cache_build_name( c, findname, find.achName, cdinfo->discid, sizeof( findname ))) {
        DEBUGLOG(( "cddaplay: query local cache %s\n", findname ));
        if( stat( findname, &fi ) == 0 && ( fi.st_mode & S_IFREG )) {
          findclose( hdir );
          snprintf( match, size, "%s %08lx", find.achName, cdinfo->discid );
          DEBUGLOG(( "cddaplay: local cache match %s\n", match ));
          return CDDB_OK;
        }
      }
    }

    findrc = findnext( hdir, &find );
  }

  findclose( hdir );
  return CDDB_NOT_MATCH;
}

/* Query the CDDB database for a list of possible disc matches.
   Returns the CDDB_OK if it founds exact match, or CDDB_MORE_DATA if
   it founds inexact matches. */
CDDBRC
cddb_disc( CDDB_CONNECTION* c, CDINFO* cdinfo, char* match, int size )
{
  int  rc = CDDB_OK;
  int  i;
  char query[1024];
  char digit[ 128];

  if(( rc = cddb_cache_disc( c, cdinfo, match, size )) == CDDB_OK ) {
    return rc;
  }

  sprintf( query, "cddb+query+%08lx+%ld+", cdinfo->discid, cdinfo->track_count );

  for( i = cdinfo->track_first; i <= cdinfo->track_last; i++ ) {
    // +150 because FreeDB database doesn't use real sector addressing.
    strlcat( query, itoa( cdinfo->track_info[i].start + 150, digit, 10 ), sizeof( query ));
    strlcat( query, "+", sizeof( query ));
  }

  strlcat( query, itoa( cdinfo->time, digit, 10 ), sizeof( query ));
  rc = cddb_send_query( c, query, match, size );

  if( rc == CDDB_MORE_DATA || rc == CDDB_DATA_FOLLOW ) {
    if(( rc = cddb_disc_next( c, match, size )) == CDDB_OK ) {
      return CDDB_MORE_DATA;
    }
  }
  if( rc == CDDB_OK ) {
    cddb_strip_newline( match );
  }

  cddb_close_handle( c );
  return rc;
}

/* Retrive the next match in a CDDB disc result set. Returns the CDDB_OK if it
   successfully retrieves the next match. */
CDDBRC
cddb_disc_next( CDDB_CONNECTION* c, char* match, int size )
{
  if( cddb_readline( c, match, size ) == CDDB_OK )
  {
    cddb_strip_newline( match );

    // CDDB use terminating marker: `.' character in the
    // beginning of a line.
    if( *match == '.' ) {
      cddb_close_handle( c );
      return CDDB_END_OF_DATA;
    }
    return CDDB_OK;
  } else {
    return CDDB_PROTOCOL_ERROR;
  }
}

/* Retrieve a disc record from the CDDB cache. Returns the CDDB_DATA_FOLLOW if it
   successfully opens the cached data. */
static CDDBRC
cddb_open_cache_info( CDDB_CONNECTION* c )
{
  char cachename[_MAX_PATH];
  struct stat fi;

  if( cddb_cache_build_name( c, cachename, c->genre, c->discid, sizeof( cachename ))) {
    if( stat( cachename, &fi ) == 0 && ( fi.st_mode & S_IFREG )) {
      DEBUGLOG(( "cddaplay: found cached disc %s\n", cachename ));
      if(( c->handle = xio_fopen( cachename, "r" )) != NULL ) {
        return CDDB_DATA_FOLLOW;
      }
    }
  }

  return CDDB_NOT_FOUND;
}

/* Retrieve a disc record from the CDDB server. Returns the CDDB_OK if it
   successfully retrieves the data. */
CDDBRC
cddb_read( CDDB_CONNECTION* c, CDINFO* cdinfo, char* match )
{
  char  query[1024];
  char  reply[1024];
  char  title[1024];
  char  cachename[_MAX_PATH];
  FILE* cachefile = NULL;
  ULONG matchid   = 0;

  cddb_free_titles( c );
  sscanf( match, "%511s %lx", c->genre, &matchid );
  c->discid = cdinfo->discid;

  if( cddb_open_cache_info( c ) != CDDB_DATA_FOLLOW )
  {
    snprintf( query, sizeof( query ), "cddb+read+%s+%08lx", c->genre, matchid );

    if( cddb_send_query( c, query, NULL, 0 ) != CDDB_DATA_FOLLOW ) {
      return CDDB_PROTOCOL_ERROR;
    }

    if( cddb_cache_build_name( c, cachename, c->genre, c->discid, sizeof( cachename ))) {
      DEBUGLOG(( "cddaplay: cache disc info to %s\n", cachename ));
      cachefile = fopen( cachename, "w" );
    }
  }

  while( cddb_readline( c, reply, sizeof( reply )) == CDDB_OK )
  {
    DEBUGLOG(( "cddaplay: discinfo is %s", reply ));

    if( cachefile ) {
      fputs( reply, cachefile );
    }

    // CDDB use terminating marker: `.' character in the
    // beginning of a line.
    if( *reply == '.' ) {
      cddb_close_handle( c );

      if( cachefile ) {
        fclose( cachefile );
      }

      return CDDB_OK;
    }

    if( strnicmp( reply, "DTITLE=", 7 ) == 0 ) {
      sscanf( reply + 7, "%511[^/]/%511[^\n]\n", c->artist, c->album );
      blank_strip( c->artist );
      blank_strip( c->album  );
      continue;
    }
    if( strnicmp( reply, "DYEAR=",  6 ) == 0 ) {
      sscanf( reply + 6, "%ld[^\n]\n", &c->year );
      continue;
    }
    if( strnicmp( reply, "TTITLE",  6 ) == 0 )
    {
      int track;

      sscanf( reply, "TTITLE%d=%1023[^\n]\n", &track, title );

      if( track < MAX_TRACKS && track >= 0 )
      {
        if( c->track_title[track] == NULL ) {
          c->track_title[track] = strdup( title );
        }
        else
        {
          char* append = malloc( strlen( c->track_title[track] ) + strlen( title ) + 1 );

          if( append ) {
            strcpy( append, c->track_title[track] );
            strcat( append, title  );

            free( c->track_title[track] );
            c->track_title[track] = append;
          }
        }
      }
      continue;
    }
  }

  if( cachefile ) {
    fclose( cachefile );
    // The data is ended abnormally, we must delete the cached file.
    unlink( cachename );
  }

  cddb_close_handle( c );
  return CDDB_PROTOCOL_ERROR;
}

/* Returns a specified field of the given disc record.
   The tracks numeration is started from 0. */
char*
cddb_getstring( CDDB_CONNECTION* c,
                int track, int type, char* result, int size )
{
  if( size ) {
    *result = 0;
  }

  if( track < MAX_TRACKS ) {
    switch( type ) {
      case CDDB_TITLE:
        if( c->track_title[track] ) {
          strlcpy( result, c->track_title[track], size );
        }
        break;
      case CDDB_ARTIST:
        strlcpy( result, c->artist, size );
        break;
      case CDDB_ALBUM:
        strlcpy( result, c->album,  size );
        break;
      case CDDB_GENRE:
        strlcpy( result, c->genre,  size );
        break;
      case CDDB_YEAR:
        if( c->year ) {
          snprintf( result, size, "%ld", c->year );
        }
        break;
    }
  }

  return result;
}

