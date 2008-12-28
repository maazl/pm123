/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "id3v1.h"
#include <minmax.h>
#include <snprintf.h>
#include <charset.h>
#include "genre.h"


static int id3v1_read_charset = CH_CP_NONE;
static int id3v1_save_charset = CH_CP_NONE;

/* Sets the writing characters set. */
void
id3v1_set_save_charset( int charset ) {
  id3v1_save_charset = charset;
}

/* Sets the reading characters set. */
void
id3v1_set_read_charset( int charset ) {
  id3v1_read_charset = charset;
}

/* Fetch string from tag. */
static void
safecopy( char* target, const char* source, int size )
{
  int i;

  strncpy( target, source, size );
  target[ size ] = 0;

  for( i = size - 1; ( i >=0 ) && ( target[i] == ' ' ); i-- ) {
    target[i] = 0;
  }
}

/* Store string to tag. */
static void
spacecopy( char* target, const char* source, int size )
{
  memset ( target, 0, size );
  strncpy( target, source, size );
}

/* Cleanups of a ID3v1 tag structure. */
void
id3v1_clean_tag( ID3V1_TAG* tag )
{
  memset( tag, 0, sizeof( *tag ));
  strcpy( tag->id, "TAG" );
  tag->genre = 0xFF;
}

/* Reads a ID3v1 tag from the input file and stores them in
   the given structure. Returns 0 if it successfully reads the
   tag or if the input file don't have a ID3v1 tag. A nonzero
   return value indicates an error. */
int
id3v1_get_tag( XFILE* x, ID3V1_TAG* tag )
{
  if( xio_fseek( x, -128, XIO_SEEK_END ) != -1 ) {
    if( xio_fread( tag, 1, 128, x ) == 128 ) {
      if( strncmp( tag->id, "TAG", 3 ) == 0 ) {
        return 0;
      } else {
        id3v1_clean_tag( tag );
        return 0;
      }
    }
  }

  id3v1_clean_tag( tag );
  return -1;
}

/* Writes a ID3v1 tag from the given structure to the output
   file. Returns 0 if it successfully writes the tag. A nonzero
   return value indicates an error. */
int
id3v1_set_tag( XFILE* x, ID3V1_TAG* tag )
{
  char id[3];

  if( tag ) {
    if( xio_fseek( x, -128, XIO_SEEK_END ) != -1 ) {
      if( xio_fread( id, 1, 3, x ) == 3 ) {
        if( strncmp( id, "TAG", 3 ) == 0 ) {
          if( xio_fwrite((char*)tag + 3, 1, 125, x ) == 125 ) {
            return 0;
          }
        } else {
          if( xio_fseek( x, 0, XIO_SEEK_END ) != -1 ) {
            if( xio_fwrite( tag, 1, 128, x ) == 128 ) {
              return 0;
            }
          }
        }
      }
    }
  }

  return -1;
}

/* Remove the tag from the file. Takes care of resizing
   the file, if needed. Returns 0 upon success, or -1 if an
   error occured. */
int
id3v1_wipe_tag( XFILE* x )
{
  char id[3];

  if( xio_fseek( x, -128, XIO_SEEK_END ) != -1 ) {
    if( xio_fread( id, 1, 3, x ) == 3 ) {
      if( strncmp( id, "TAG", 3 ) == 0 ) {
        if( xio_ftruncate( x, xio_fsize( x ) - 128 ) == 0 ) {
          return 0;
        }
      } else {
        return 0;
      }
    }
  }
  return -1;
}

/* Returns a specified field of the given tag. */
char*
id3v1_get_string( ID3V1_TAG* tag, int type, char* result, int size )
{
  *result = 0;

  // TODO: charset autodetection

  if( tag ) {
    switch( type )
    {
      case ID3V1_TITLE:
        safecopy( result, tag->title, min( size, sizeof( tag->title )));
        ch_convert( id3v1_read_charset, result, CH_CP_NONE, result, size, 0 );
        break;

      case ID3V1_ARTIST:
        safecopy( result, tag->artist, min( size, sizeof( tag->artist )));
        ch_convert( id3v1_read_charset, result, CH_CP_NONE, result, size, 0 );
        break;

      case ID3V1_ALBUM:
        safecopy( result, tag->album, min( size, sizeof( tag->album )));
        ch_convert( id3v1_read_charset, result, CH_CP_NONE, result, size, 0 );
        break;

      case ID3V1_YEAR:
        safecopy( result, tag->year, min( size, sizeof( tag->year )));
        ch_convert( id3v1_read_charset, result, CH_CP_NONE, result, size, 0 );
        break;

      // Since all non-filled fields must be padded with zeroed bytes
      // its a good assumption that all ID3v1 readers will stop reading
      // the field when they encounter a zeroed byte. If the second last
      // byte of a comment field is zeroed and the last one isn't we
      // have an extra byte to fill with information. As the comments
      // field is to short to write anything useful in the ID3v1.1 standard
      // declares that this field should be 28 characters, that the next
      // byte always should be zero and that the last byte before the
      // genre byte should contain which track on the CD this music
      // comes from.

      case ID3V1_COMMENT:
        if( tag->empty || !tag->track ) {
          safecopy( result, tag->comment, min( size, 30 ));
          ch_convert( id3v1_read_charset, result, CH_CP_NONE, result, size, 0 );
        } else {
          safecopy( result, tag->comment, min( size, sizeof( tag->comment )));
          ch_convert( id3v1_read_charset, result, CH_CP_NONE, result, size, 0 );
        }
        break;

      case ID3V1_TRACK:
        if( !tag->empty && tag->track ) {
          snprintf( result, size, "%02d", tag->track );
        }
        break;

      case ID3V1_GENRE:
        if( tag->genre <= GENRE_LARGEST ) {
          safecopy( result, genres[ tag->genre ], size );
        } else if ( tag->genre != 0xFF )
          snprintf( result, size, "#%u", tag->genre );
        break;
    }
  }

  return result;
}

/* Sets a specified field of the given tag. */
void
id3v1_set_string( ID3V1_TAG* tag, int type, const char* source )
{
  char encoded[31];

  if( tag ) {
    switch( type )
    {
      case ID3V1_TITLE:
        ch_convert( CH_CP_NONE, source, id3v1_save_charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->title, encoded, sizeof( tag->title ));
        break;

      case ID3V1_ARTIST:
        ch_convert( CH_CP_NONE, source, id3v1_save_charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->artist, encoded, sizeof( tag->artist ));
        break;

      case ID3V1_ALBUM:
        ch_convert( CH_CP_NONE, source, id3v1_save_charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->album, encoded, sizeof( tag->album ));
        break;

      case ID3V1_YEAR:
        ch_convert( CH_CP_NONE, source, id3v1_save_charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->year, encoded, sizeof( tag->year ));
        break;

      case ID3V1_COMMENT:
        ch_convert( CH_CP_NONE, source, id3v1_save_charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->comment, encoded, sizeof( tag->comment ));
        break;

      case ID3V1_TRACK:
        tag->empty = 0;
        tag->track = atol( source );
        break;

      case ID3V1_GENRE:
      { int i, l;
        if (*source == 0)
          tag->genre = 0xFF;
        else if (sscanf(source, "#%i%n", &i, &l) == 1 && l == strlen(source))
          tag->genre = i;
        else
          for( tag->genre = 0; tag->genre <= GENRE_LARGEST; tag->genre++ ) {
            if( stricmp( source, genres[ tag->genre ] ) == 0 ) {
              break;
            }
          }
        break;
      }
    }
  }
}

