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

/* identify the genre number */
int
id3v1_get_genre( const char* str )
{ int g;
  for( g = 0; g <= GENRE_LARGEST; g++ )
    if( stricmp( str, genres[ g ] ) == 0 )
      return g;
  return -1;
}

static const ID3V1_TAG clean_tag = { "", "", "", "", "", "", 0, 0, 0xFF };

/* Cleanups of a ID3v1 tag structure. */
void
id3v1_clean_tag( ID3V1_TAG* tag )
{
  memcpy( tag, &clean_tag, sizeof( *tag ));
}

/* Check whether TAG is clean */
int id3v1_is_clean_tag( const ID3V1_TAG* tag )
{
  return memcmp( tag->title, clean_tag.title, sizeof *tag - offsetof(ID3V1_TAG, title)) == 0;
}

/* Ensure that the tag has a valid signature */
void id3v1_make_tag_valid( ID3V1_TAG* tag )
{
  memcpy( tag->id, "TAG", 3 );
}

/* Check wether the tag has a valid signature */
int id3v1_is_tag_valid( const ID3V1_TAG* tag )
{
  return memcmp( tag->id, "TAG", 3 ) == 0;
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
id3v1_get_string( const ID3V1_TAG* tag, ID3V1_TAG_COMP type, char* result, int size, int charset )
{
  *result = 0;

  // TODO: charset autodetection

  if( tag ) {
    switch( type )
    {
      case ID3V1_TITLE:
        safecopy( result, tag->title, min( size, sizeof( tag->title )));
        ch_convert( charset, result, CH_CP_NONE, result, size, 0 );
        break;

      case ID3V1_ARTIST:
        safecopy( result, tag->artist, min( size, sizeof( tag->artist )));
        ch_convert( charset, result, CH_CP_NONE, result, size, 0 );
        break;

      case ID3V1_ALBUM:
        safecopy( result, tag->album, min( size, sizeof( tag->album )));
        ch_convert( charset, result, CH_CP_NONE, result, size, 0 );
        break;

      case ID3V1_YEAR:
        safecopy( result, tag->year, min( size, sizeof( tag->year )));
        ch_convert( charset, result, CH_CP_NONE, result, size, 0 );
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
          ch_convert( charset, result, CH_CP_NONE, result, size, 0 );
        } else {
          safecopy( result, tag->comment, min( size, sizeof( tag->comment )));
          ch_convert( charset, result, CH_CP_NONE, result, size, 0 );
        }
        break;

      case ID3V1_TRACK:
        if( !tag->empty && tag->track ) {
          snprintf( result, size, "%02d", tag->track );
          // Well, we don't have to convert numbers unless someone want's to store
          // the tags ad EBCDIC or something even worse.
        }
        break;

      case ID3V1_GENRE:
        if( tag->genre <= GENRE_LARGEST ) {
          safecopy( result, genres[ tag->genre ], size );
          // We don't need to convert these 7 bit ASCII texts.
        } else if ( tag->genre != 0xFF )
          snprintf( result, size, "#%u", tag->genre );
        break;
    }
  }

  return result;
}

/* Sets a specified field of the given tag. */
void
id3v1_set_string( ID3V1_TAG* tag, ID3V1_TAG_COMP type, const char* source, int charset )
{
  char encoded[31];

  if( tag ) {
    switch( type )
    {
      case ID3V1_TITLE:
        ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->title, encoded, sizeof( tag->title ));
        break;

      case ID3V1_ARTIST:
        ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->artist, encoded, sizeof( tag->artist ));
        break;

      case ID3V1_ALBUM:
        ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->album, encoded, sizeof( tag->album ));
        break;

      case ID3V1_YEAR:
        ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
        spacecopy( tag->year, encoded, sizeof( tag->year ));
        break;

      case ID3V1_COMMENT:
        ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
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
          tag->genre = (unsigned char)id3v1_get_genre(source);
        break;
      }
    }
  }
}

