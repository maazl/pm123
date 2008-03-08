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

#ifndef CDDB_H
#define CDDB_H

#include <xio.h>
#include "readcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol level 6 is the same as level 5 except that the character set
   is now UTF-8 instead of ISO-8859-1. */
#define CDDB_PROTOLEVEL       5
#define CDDB_CHARSET          "UTF-8"

#define CDDB_OK               200
#define CDDB_NOT_MATCH        202
#define CDDB_DATA_FOLLOW      210
#define CDDB_MORE_DATA        211
#define CDDB_NOT_FOUND        401
#define CDDB_END_OF_DATA      998
#define CDDB_PROTOCOL_ERROR   999

#define CDDB_TITLE  1
#define CDDB_ARTIST 2
#define CDDB_ALBUM  3
#define CDDB_YEAR   4
#define CDDB_GENRE  5

#if CDDB_PROTOLEVEL >= 6
  #include <iconv.h>
#endif

typedef int CDDBRC;

/* Keeping state about the connection to a CDDB server. */
typedef struct _CDDB_CONNECTION
{
  char    servname[512];
  XFILE*  handle;
  char    username[128];
  char    hostname[128];
  char    cachedir[_MAX_PATH];

  #if CDDB_PROTOLEVEL >= 6
  iconv_t charset;
  #endif

  ULONG   discid;
  ULONG   year;
  char    genre [512];
  char    artist[512];
  char    album [512];

  char*   track_title[MAX_TRACKS];

} CDDB_CONNECTION;

/* Creates a new CDDB connection structure. Returns a pointer to a
   CDDB structure that can be used to access the database. A NULL
   pointer return value indicates an error. */
CDDB_CONNECTION* cddb_connect( void );
/* Free all resources associated with the given CDDB connection
   structure. */
void cddb_close( CDDB_CONNECTION* c );

/* Sets the user name and host name of the local machine. */
void cddb_set_email_address( CDDB_CONNECTION* c, const char* email );
/* Sets the CDDB server. */
void cddb_set_server( CDDB_CONNECTION* c, const char* url );

/* Set the character set. By default the FreeDB server uses UTF-8
   when providing CD data. When a character set is defined with this
   function any strings retrieved from or sent to the server
   will automatically be converted. Returns -1 if the specified
   character set is unknown, or no conversion from/to UTF-8 is
   available. True otherwise. */
int  cddb_set_charset( CDDB_CONNECTION* c, const char* charset );

/* Changes the directory used for caching CDDB entries locally. Returns
   a value of 0 if the cache directory was successfully changed. A
   return value of -1 indicates an error. */
int  cddb_set_cache_dir( CDDB_CONNECTION* c, const char* pathname );

/* Retrieve the first CDDB mirror server. Returns the CDDB_OK if it
   successfully retrieves the server. */
CDDBRC cddb_mirror( CDDB_CONNECTION* c, char* url, int size );
/* Retrieve the next  CDDB mirror server. Returns the CDDB_OK if it
   successfully retrieves the server. */
CDDBRC cddb_mirror_next( CDDB_CONNECTION* c, char* url, int size );

/* Query the CDDB database for a list of possible disc matches.
   Returns the CDDB_OK if it founds exact match, or CDDB_MORE_DATA if
   it founds inexact matches. */
CDDBRC cddb_disc( CDDB_CONNECTION* c, CDINFO* cdinfo, char* match, int size );
/* Retrive the next match in a CDDB query result set. Returns the CDDB_OK if it
   successfully retrieves the next match. */
CDDBRC cddb_disc_next( CDDB_CONNECTION* c, char* match, int size );
/* Retrieve a disc record from the CDDB server. Returns the CDDB_OK if it
   successfully retrieves the data. */
CDDBRC cddb_read( CDDB_CONNECTION* c, CDINFO* cdinfo, char* match );

/* Returns a specified field of the given disc record.
   The tracks numeration is started from 0. */
char*  cddb_getstring( CDDB_CONNECTION* c, int track, int type, char* result, int size );

#ifdef __cplusplus
}
#endif
#endif /* CDDB_H */

