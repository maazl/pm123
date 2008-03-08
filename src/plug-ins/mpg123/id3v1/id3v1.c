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

#define GENRE_LARGEST 148
static char *genres[] =
{
  /*   0 */  "Blues",
  /*   1 */  "Classic Rock",
  /*   2 */  "Country",
  /*   3 */  "Dance",
  /*   4 */  "Disco",
  /*   5 */  "Funk",
  /*   6 */  "Grunge",
  /*   7 */  "Hip-Hop",
  /*   8 */  "Jazz",
  /*   9 */  "Metal",
  /*  10 */  "New Age",
  /*  11 */  "Oldies",
  /*  12 */  "Other",
  /*  13 */  "Pop",
  /*  14 */  "R&B",
  /*  15 */  "Rap",
  /*  16 */  "Reggae",
  /*  17 */  "Rock",
  /*  18 */  "Techno",
  /*  19 */  "Industrial",
  /*  20 */  "Alternative",
  /*  21 */  "Ska",
  /*  22 */  "Death Metal",
  /*  23 */  "Pranks",
  /*  24 */  "Soundtrack",
  /*  25 */  "Euro-Techno",
  /*  26 */  "Ambient",
  /*  27 */  "Trip-Hop",
  /*  28 */  "Vocal",
  /*  29 */  "Jazz+Funk",
  /*  30 */  "Fusion",
  /*  31 */  "Trance",
  /*  32 */  "Classical",
  /*  33 */  "Instrumental",
  /*  34 */  "Acid",
  /*  35 */  "House",
  /*  36 */  "Game",
  /*  37 */  "Sound Clip",
  /*  38 */  "Gospel",
  /*  39 */  "Noise",
  /*  40 */  "AlternRock",
  /*  41 */  "Bass",
  /*  42 */  "Soul",
  /*  43 */  "Punk",
  /*  44 */  "Space",
  /*  45 */  "Meditative",
  /*  46 */  "Instrumental Pop",
  /*  47 */  "Instrumental Rock",
  /*  48 */  "Ethnic",
  /*  49 */  "Gothic",
  /*  50 */  "Darkwave",
  /*  51 */  "Techno-Industrial",
  /*  52 */  "Eletronic",
  /*  53 */  "Pop-Folk",
  /*  54 */  "Eurodance",
  /*  55 */  "Dream",
  /*  56 */  "Southern Rock",
  /*  57 */  "Comedy",
  /*  58 */  "Cult",
  /*  59 */  "Gangsta",
  /*  60 */  "Top 40",
  /*  61 */  "Christian Rap",
  /*  62 */  "Pop/Funk",
  /*  63 */  "Jungle",
  /*  64 */  "Native American",
  /*  65 */  "Cabaret",
  /*  66 */  "New Wave",
  /*  67 */  "Psychedelic",
  /*  68 */  "Rave",
  /*  69 */  "Showtunes",
  /*  70 */  "Trailer",
  /*  71 */  "Lo-Fi",
  /*  72 */  "Tribal",
  /*  73 */  "Acid Punk",
  /*  74 */  "Acid Jazz",
  /*  75 */  "Polka",
  /*  76 */  "Retro",
  /*  77 */  "Musical",
  /*  78 */  "Rock & Roll",
  /*  79 */  "Hard Rock",
  /*  80 */  "Folk",
  /*  81 */  "Folk-Rock",
  /*  82 */  "National Folk",
  /*  83 */  "Swing",
  /*  84 */  "Fast Fusion",
  /*  85 */  "Bebob",
  /*  86 */  "Latin",
  /*  87 */  "Revival",
  /*  88 */  "Celtic",
  /*  89 */  "Bluegrass",
  /*  90 */  "Avantgarde",
  /*  91 */  "Gothic Rock",
  /*  92 */  "Progressive Rock",
  /*  93 */  "Psychedelic Rock",
  /*  94 */  "Symphonic Rock",
  /*  95 */  "Slow Rock",
  /*  96 */  "Big Band",
  /*  97 */  "Chorus",
  /*  98 */  "Easy Listening",
  /*  99 */  "Acoustic",
  /*  00 */  "Humour",
  /* 101 */  "Speech",
  /* 102 */  "Chanson",
  /* 103 */  "Opera",
  /* 104 */  "Chamber Music",
  /* 105 */  "Sonata",
  /* 106 */  "Symphony",
  /* 107 */  "Booty Bass",
  /* 108 */  "Primus",
  /* 109 */  "Porn Groove",
  /* 110 */  "Satire",
  /* 111 */  "Slow Jam",
  /* 112 */  "Club",
  /* 113 */  "Tango",
  /* 114 */  "Samba",
  /* 115 */  "Folklore",
  /* 116 */  "Ballad",
  /* 117 */  "Power Ballad",
  /* 118 */  "Rhythmic Soul",
  /* 119 */  "Freestyle",
  /* 120 */  "Duet",
  /* 121 */  "Punk Rock",
  /* 122 */  "Drum Solo",
  /* 123 */  "Acapella",
  /* 124 */  "Euro-House",
  /* 125 */  "Dance Hall",
  /* 126 */  "Goa",
  /* 127 */  "Drum & Bass",
  /* 128 */  "Club-House",
  /* 129 */  "Hardcore",
  /* 130 */  "Terror",
  /* 131 */  "Indie",
  /* 132 */  "BritPop",
  /* 133 */  "Negerpunk",
  /* 134 */  "Polsk Punk",
  /* 135 */  "Beat",
  /* 136 */  "Christian Gangsta",
  /* 137 */  "Heavy Metal",
  /* 138 */  "Black Metal",
  /* 139 */  "Crossover",
  /* 140 */  "Contemporary C",
  /* 141 */  "Christian Rock",
  /* 142 */  "Merengue",
  /* 143 */  "Salsa",
  /* 144 */  "Thrash Metal",
  /* 145 */  "Anime",
  /* 146 */  "JPop",
  /* 147 */  "SynthPop",
  /* 148 */  "Unknown"
};

static int id3v1_read_charset = CH_DEFAULT;
static int id3v1_save_charset = CH_DEFAULT;

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
  if( xio_fseek( x, -128, XIO_SEEK_END ) == 0 ) {
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
    if( xio_fseek( x, -128, XIO_SEEK_END ) == 0 ) {
      if( xio_fread( id, 1, 3, x ) == 3 ) {
        if( strncmp( id, "TAG", 3 ) == 0 ) {
          if( xio_fwrite((char*)tag + 3, 1, 125, x ) == 125 ) {
            return 0;
          }
        } else {
          if( xio_fseek( x, 0, XIO_SEEK_END ) == 0 ) {
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

  if( xio_fseek( x, -128, XIO_SEEK_END ) == 0 ) {
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

  if( tag ) {
    switch( type )
    {
      case ID3V1_TITLE:
        safecopy( result, tag->title, min( size, sizeof( tag->title )));
        ch_convert( id3v1_read_charset, result, CH_DEFAULT, result, size );
        break;

      case ID3V1_ARTIST:
        safecopy( result, tag->artist, min( size, sizeof( tag->artist )));
        ch_convert( id3v1_read_charset, result, CH_DEFAULT, result, size );
        break;

      case ID3V1_ALBUM:
        safecopy( result, tag->album, min( size, sizeof( tag->album )));
        ch_convert( id3v1_read_charset, result, CH_DEFAULT, result, size );
        break;

      case ID3V1_YEAR:
        safecopy( result, tag->year, min( size, sizeof( tag->year )));
        ch_convert( id3v1_read_charset, result, CH_DEFAULT, result, size );
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
          ch_convert( id3v1_read_charset, result, CH_DEFAULT, result, size );
        } else {
          safecopy( result, tag->comment, min( size, sizeof( tag->comment )));
          ch_convert( id3v1_read_charset, result, CH_DEFAULT, result, size );
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
        }
        break;
    }
  }

  return result;
}

/* Sets a specified field of the given tag. */
void
id3v1_set_string( ID3V1_TAG* tag, int type, char* source )
{
  char encoded[31];

  if( tag ) {
    switch( type )
    {
      case ID3V1_TITLE:
        ch_convert( CH_DEFAULT, source, id3v1_save_charset, encoded, sizeof( encoded ));
        spacecopy( tag->title, encoded, sizeof( tag->title ));
        break;

      case ID3V1_ARTIST:
        ch_convert( CH_DEFAULT, source, id3v1_save_charset, encoded, sizeof( encoded ));
        spacecopy( tag->artist, encoded, sizeof( tag->artist ));
        break;

      case ID3V1_ALBUM:
        ch_convert( CH_DEFAULT, source, id3v1_save_charset, encoded, sizeof( encoded ));
        spacecopy( tag->album, encoded, sizeof( tag->album ));
        break;

      case ID3V1_YEAR:
        ch_convert( CH_DEFAULT, source, id3v1_save_charset, encoded, sizeof( encoded ));
        spacecopy( tag->year, encoded, sizeof( tag->year ));
        break;

      case ID3V1_COMMENT:
        ch_convert( CH_DEFAULT, source, id3v1_save_charset, encoded, sizeof( encoded ));
        spacecopy( tag->comment, encoded, sizeof( tag->comment ));
        break;

      case ID3V1_TRACK:
        tag->empty = 0;
        tag->track = atol( source );
        break;

      case ID3V1_GENRE:
        for( tag->genre = 0; tag->genre <= GENRE_LARGEST; tag->genre++ ) {
          if( stricmp( source, genres[ tag->genre ] ) == 0 ) {
            break;
          }
          if( tag->genre > GENRE_LARGEST ) {
            tag->genre = 0xFF;
          }
        }
        break;
    }
  }
}

