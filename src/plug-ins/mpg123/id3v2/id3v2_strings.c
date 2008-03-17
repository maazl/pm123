/*
 * Copyright (C) 1998, 1999, 2002 Espen Skoglund
 * Copyright (C) 2000-2004 Haavard Kvaalen
 * Copyright (C) 2007 Dmitry A.Steklenev
 *
 * $Id: id3v2_strings.c,v 1.2 2008/02/20 15:00:52 glass Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strutils.h>
#include <charset.h>

#include "id3v2.h"
#include "id3v2_header.h"

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

/* Get size of string in bytes including null. */
static int
id3v2_string_size( int8_t encoding, const char* source )
{
  int size = 0;

  if( source ) {
    switch( encoding )
    {
      case ID3V2_ENCODING_ISO_8859_1:
      case ID3V2_ENCODING_UTF8:
        size = strlen( source ) + 1;
        break;
      case ID3V2_ENCODING_UTF16:
      case ID3V2_ENCODING_UTF16_BOM:
        while( *source != 0 || *( source + 1 ) != 0 )
        {
          source += 2;
          size   += 2;
        }
        size += 2;
        break;
    }
  }
  return size;
}

/* Get size of string in bytes excluding null. */
static int
id3v2_string_length( int8_t encoding, const char* source )
{
  int size = 0;

  if( source ) {
    switch( encoding )
    {
      case ID3V2_ENCODING_ISO_8859_1:
      case ID3V2_ENCODING_UTF8:
        size = strlen( source );
        break;
      case ID3V2_ENCODING_UTF16:
      case ID3V2_ENCODING_UTF16_BOM:
        while( *source != 0 || *( source + 1 ) != 0 )
        {
          source += 2;
          size   += 2;
        }
        break;
    }
  }
  return size;
}

/* Converts the specified string to the locale's encoding. */
static char*
id3v2_decode( int8_t encoding, const char* source, char* result, int size )
{
  switch( encoding )
  {
    case ID3V2_ENCODING_ISO_8859_1:
      ch_convert( id3v2_get_read_charset(), source, CH_CP_NONE, result, size, 0 );
      break;

    case ID3V2_ENCODING_UTF8:
      ch_convert( 1208, source, CH_CP_NONE, result, size, 0 );
      break;

    case ID3V2_ENCODING_UTF16:
      ch_convert( 1200, source, CH_CP_NONE, result, size, 0 );
      break;

    case ID3V2_ENCODING_UTF16_BOM:
      ch_convert( 1200, source, CH_CP_NONE, result, size, CH_FLAGS_WRITE_BOM );
      break;
  }

  return result;
}

/* Returns a newly-allocated string converted from the locale's encoding. */
static char*
id3v2_encode( int8_t encoding, const char* source )
{
  int   size   = strlen( source ) * 4 + 2;
  char* result = malloc( size   );

  if( result ) {
    switch( encoding )
    {
      case ID3V2_ENCODING_ISO_8859_1:
        ch_convert( CH_CP_NONE, source, id3v2_get_save_charset(), result, size, 0 );
        break;

      case ID3V2_ENCODING_UTF8:
        ch_convert( CH_CP_NONE, source, 1208, result, size, 0 );
        break;

      case ID3V2_ENCODING_UTF16:
      case ID3V2_ENCODING_UTF16_BOM:
        ch_convert( CH_CP_NONE, source, 1200, result, size, 0 );
        break;

      default:
        free( result );
        result = NULL;
    }
  }
  return result;
}

/* Is the specified frame a text based frame. */
int
id3v2_is_text_frame( ID3V2_FRAME* frame )
{
  if(( frame ) && ( frame->fr_desc ) &&
    (( frame->fr_desc->fd_idstr[0] == 'T'  ) ||
     ( frame->fr_desc->fd_idstr[0] == 'W'  ) ||
     ( frame->fr_desc->fd_id == ID3V2_COMM ) ))
  {
    return 1;
  } else {
    return 0;
  }
}

/* Expand content type string of frame and return it.  Return NULL
   upon error. */
static char*
id3v2_get_content( ID3V2_FRAME* frame, char* result, int size )
{
  char* source;
  char* p;
  int   srclen = id3v2_string_size( ID3V2_TEXT_FRAME_ENCODING( frame ),
                                    ID3V2_TEXT_FRAME_PTR( frame ));

  if(( source = malloc( srclen )) == NULL ) {
    return NULL;
  }

  if( id3v2_decode( ID3V2_TEXT_FRAME_ENCODING( frame ),
                    ID3V2_TEXT_FRAME_PTR( frame ), source, srclen ) == NULL )
  {
    free( source );
    return NULL;
  }

  // If content is just plain text, return it.
  if( source[0] != '(' ) {
    strlcpy( result, source, size );
    free( source );
    return result;
  }

  // Expand ID3v1 genre numbers.
  p = result;

  while( source[0] == '(' && source[1] != '(' && size > 0 )
  {
    char* genre;
    int   i = 0;

    if(( source[1] == 'R' ) && ( source[2] == 'X' )) {
      source += 4;
      genre   = ( p == result ) ? "(Remix)" : " (Remix)";
    } else if( source[1] == 'C' && source[2] == 'R' ) {
      source += 4;
      genre   = ( p == result ) ? "(Cover)" : " (Cover)";
    } else {
      // Get ID3v1 genre number.
      source++;
      while( *source != ')' ) {
        i *= 10;
        i += *source++ - '0';
      }
      source++;

      // Boundary check.
      if( i > GENRE_LARGEST ) {
        continue;
      }

      genre = genres[i];

      if(( p != result ) && ( size-- > 0 )) {
        *p++ = '/';
      }
    }

    // Expand string into buffer.
    while( *genre && size > 0 ) {
      *p++ = *genre++;
       size--;
    }
  }

  // Add plaintext refinement.
  if( *source == '(' ) {
    source++;
  }
  if( *source && p != result && size-- > 0 ) {
    *p++ = ' ';
  }
  while( *source && size > 0 )
  {
    *p++ = *source++;
     size--;
  }

  *p = 0;
  // FIXME:     free( source );
  return result;
}

/* Return string contents of frame. */
char*
id3v2_get_string( ID3V2_FRAME* frame, char* result, int size )
{
  char* source = ID3V2_TEXT_FRAME_PTR( frame );
  int   offset = 0;

  // Do we even have data for this frame.
  if( !frame->fr_data ) {
    return NULL;
  }

  // Type check.
  if( !id3v2_is_text_frame( frame )) {
    return NULL;
  }

  // Check if frame is compressed.
  if( id3v2_decompress_frame( frame ) == -1 ) {
    return NULL;
  }

  if( frame->fr_desc->fd_id == ID3V2_TCON )
  {
    return id3v2_get_content( frame, result, size );
  }
  else if( frame->fr_desc->fd_id == ID3V2_TXXX ||
           frame->fr_desc->fd_id == ID3V2_WXXX )
  {
    // This is a user defined text or link frame. Skip the description.
    offset = id3v2_string_size( ID3V2_TEXT_FRAME_ENCODING( frame ), source );
  }
  else if( frame->fr_desc->fd_id == ID3V2_COMM )
  {
    // This is a comment frame. Skip the language id and the description.
    if( frame->fr_size < 5 ) {
      return NULL;
    }

    // Skip the language id.
    offset = 3;
    // Skip the description.
    offset += id3v2_string_size( ID3V2_TEXT_FRAME_ENCODING( frame ), source + offset );
  }

  if( offset >= frame->fr_size ) {
    return NULL;
  }

  return id3v2_decode( ID3V2_TEXT_FRAME_ENCODING( frame ),
                       source + offset, result, size );
}

/* Get description part of a frame. */
char*
id3v2_get_description( ID3V2_FRAME* frame, char* result, int size )
{
  int offset = 0;

  // If predefined frame, return description.
  if( frame->fr_desc->fd_id != ID3V2_TXXX &&
      frame->fr_desc->fd_id != ID3V2_WXXX &&
      frame->fr_desc->fd_id != ID3V2_COMM )
  {
    strlcpy( result, frame->fr_desc->fd_description, size );
    return result;
  }

  // Check if frame is compressed.
  if( id3v2_decompress_frame( frame ) == -1 ) {
    return NULL;
  }

  if( frame->fr_desc->fd_id == ID3V2_COMM ) {
    // This is a comment frame. Skip the language id.
    offset = 3;
  }

  if( offset >= frame->fr_size ) {
    strlcpy( result, "", size );
    return result;
  }

  return id3v2_decode( ID3V2_TEXT_FRAME_ENCODING( frame ),
                       ID3V2_TEXT_FRAME_PTR( frame ) + offset, result, size );
}


/* Set text for the indicated frame. Return 0 upon
   success, or -1 if an error occured. */
int
id3v2_set_string( ID3V2_FRAME* frame, const char* source )
{
  char*  comment      = NULL;
  char*  encoded      = NULL;
  int8_t encoding     = id3v2_get_save_encoding();
  int    encoded_size = 0;
  int    comment_size = 0;
  int    rc;

  // Type check.
  if( !id3v2_is_text_frame( frame )) {
    return -1;
  }

  if( frame->fr_desc->fd_id == ID3V2_COMM ) {
    if(( comment = id3v2_encode( encoding, "Comments" )) == NULL ) {
      return -1;
    }
  }

  // Release memory occupied by previous data.
  id3v2_clean_frame( frame );

  if(( encoded = id3v2_encode( encoding, source )) != NULL )
  {
    encoded_size = id3v2_string_length( encoding, encoded );
    comment_size = id3v2_string_size  ( encoding, comment );

    frame->fr_raw_size  = encoded_size + comment_size + 1;
    frame->fr_raw_size += comment_size ? 3 : 0;

    // We allocate 2 extra bytes. This simplifies retrieval of
    // text strings.
    frame->fr_raw_data = calloc( 1, frame->fr_raw_size + 2 );

    // Copy contents.
    *(int8_t*)frame->fr_raw_data = encoding;

    if( frame->fr_desc->fd_id == ID3V2_COMM ) {
      // <encode>xxxComments\0<comment>
      memcpy((int8_t*)frame->fr_raw_data + 1, "und", 3 );
      memcpy((int8_t*)frame->fr_raw_data + 4,  comment, comment_size );
      memcpy((int8_t*)frame->fr_raw_data + 4 + comment_size, encoded, encoded_size );
    } else {
      memcpy((int8_t*)frame->fr_raw_data + 1,  encoded, encoded_size );
    }

    frame->fr_altered = 1;
    frame->fr_data    = frame->fr_raw_data;
    frame->fr_size    = frame->fr_raw_size;

    frame->fr_owner->id3_altered = 1;
    rc =  0;
  } else {
    rc = -1;
  }

  free( encoded );
  free( comment );
  return rc;
}

