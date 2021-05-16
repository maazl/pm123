/*
 * Copyright 2008-2011 M.Mueller
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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

#include "id3v1/id3v1.h"
#include <minmax.h>
#include <snprintf.h>
#include <charset.h>
#include <plugin.h>
#include "genre.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <utilfct.h>
#include <cpp/xstring.h>

#include <os2.h>


/* Fetch string from tag. */
static void safecopy(char* target, const char* source, size_t size)
{ size = strnlen(source, size);
  while (size && source[size-1] == ' ')
    --size;
  memcpy(target, source, size);
  target[size] = 0;
}

/* Store string to tag. */
static void spacecopy(char* target, const char* source, size_t size)
{ memset (target, 0, size);
  strncpy(target, source, size);
}

static bool spaceequals(const char* s1, const char* s2, size_t size)
{ // skip equal part
  while (size && *s1 == *s2)
  { ++s1;
    ++s2;
    --size;
  }
  return memcmp(s1, &ID3V1_TAG::CleanTag, strnlen(s1, size)) == 0
      && memcmp(s2, &ID3V1_TAG::CleanTag, strnlen(s2, size)) == 0;
}


const ID3V1_TAG ID3V1_TAG::CleanTag = *(ID3V1_TAG*)"                                                                                                                             \0\0\xff";

bool ID3V1_TAG::Equals(const ID3V1_TAG& r) const
{ if ( genre != r.genre
  || !spaceequals(title, r.title, sizeof title)
  || !spaceequals(artist, r.artist, sizeof artist)
  || !spaceequals(album, r.album, sizeof album)
  || !spaceequals(year, r.year, sizeof year) )
    return false;
  if (empty == 0 && track)
  { if (r.empty == 0 && r.track)
      return track == r.track && spaceequals(comment, r.comment, sizeof comment);
  } else
    if (r.empty != 0 || !r.track)
      return spaceequals(comment, r.comment, 30);
  return false; // different ID3V1 version
}

/* Returns a specified field of the given tag. */
bool ID3V1_TAG::GetField(ID3V1_TAG_COMP type, xstring& result, int charset) const
{
  result.reset();
  char buffer[32];

  switch( type )
  { default:
      return false;

    case ID3V1_TITLE:
      safecopy(buffer, title, sizeof title);
      break;

    case ID3V1_ARTIST:
      safecopy(buffer, artist, sizeof artist);
      break;

    case ID3V1_ALBUM:
      safecopy(buffer, album, sizeof album);
      break;

    case ID3V1_YEAR:
      safecopy(buffer, year, sizeof year);
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
      if( empty || !track )
        safecopy(buffer, comment, 30);
      else
        safecopy(buffer, comment, sizeof comment);
      break;

    case ID3V1_TRACK:
      if( empty || !track )
        return false;
      result.sprintf("%02d", track);
      return true;

    case ID3V1_GENRE:
      if( genre <= GENRE_LARGEST )
        result = genres[genre];
        // We don't need to convert these 7 bit ASCII texts.
      else if ( genre != 0xFF )
        result.sprintf("#%u", genre);
      else
        return false;
      return true;
  }

  if (!*buffer)
    return false;
  ch_convert(charset, buffer, CH_CP_NONE, buffer, sizeof buffer, 0);
  result = buffer;
  return true;
}

/* Sets a specified field of the given tag. */
void ID3V1_TAG::SetField(ID3V1_TAG_COMP type, const char* source, int charset)
{
  if (!source) // cannot remove components of ID3V1 tags
    source = "";
  char encoded[32];

  switch( type )
  {
    case ID3V1_TITLE:
      ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
      spacecopy( title, encoded, sizeof title );
      break;

    case ID3V1_ARTIST:
      ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
      spacecopy( artist, encoded, sizeof artist );
      break;

    case ID3V1_ALBUM:
      ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
      spacecopy( album, encoded, sizeof album );
      break;

    case ID3V1_YEAR:
      ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
      spacecopy( year, encoded, sizeof year );
      break;

    case ID3V1_COMMENT:
      ch_convert( CH_CP_NONE, source, charset, encoded, sizeof( encoded ), 0);
      spacecopy( comment, encoded, sizeof comment );
      break;

    case ID3V1_TRACK:
      empty = 0;
      track = atol( source );
      break;

    case ID3V1_GENRE:
    { int i;
      size_t l;
      if (*source == 0)
        genre = 0xFF;
      else if (sscanf(source, "#%i%n", &i, &l) == 1 && l == strlen(source))
        genre = i;
      else
        genre = (unsigned char)GetGenre(source);
      break;
    }
  }
}

/* identify the genre number */
int ID3V1_TAG::GetGenre(const char* str)
{ for( size_t g = 0; g <= GENRE_LARGEST; g++ )
    if( stricmp( str, genres[ g ] ) == 0 )
      return g;
  return -1;
}

/* Reads a ID3v1 tag from the input file and stores them in
   the given structure. Returns 0 if it successfully reads the
   tag or if the input file don't have a ID3v1 tag. A nonzero
   return value indicates an error. */
int ID3V1_TAG::ReadTag(XFILE* x)
{ if ( xio_fseek(x, -128, XIO_SEEK_END) != -1
    && xio_fread(this, 1, 128, x) == 128 )
  { if (!IsValid())
      Clean();
    return 0;
  }
  Clean();
  return xio_errno();
}

/* Writes a ID3v1 tag from the given structure to the output
   file. Returns 0 if it successfully writes the tag. A nonzero
   return value indicates an error. */
int ID3V1_TAG::WriteTag(XFILE* x) const
{ char id[3];
  if ( xio_fseek(x, -128, XIO_SEEK_END) != -1
    && xio_fread( id, 1, 3, x ) == 3 )
  { if (strncmp( id, "TAG", 3 ) == 0)
    { if (xio_fwrite(title, 1, 125, x) == 125)
        return 0;
    } else
    { if ( xio_fseek(x, 0, XIO_SEEK_END) != -1
        && xio_fwrite(this, 1, 128, x) == 128 )
        return 0;
    }
  }
  return -1;
}

/* Remove the tag from the file. Takes care of resizing
   the file, if needed. Returns 0 upon success, or -1 if an
   error occured. */
int ID3V1_TAG::WipeTag(XFILE* x)
{ char id[3];
  if ( xio_fseekl(x, -128, XIO_SEEK_END) != -1
    && xio_fread(id, 1, 3, x) == 3 )
  { if ( strncmp(id, "TAG", 3) != 0
      || xio_ftruncatel(x, xio_fsizel(x) - 128) == 0 )
      return 0;
  }
  return -1;
}

static void strpad(char* str, size_t len, char pad)
{ char* se = str + len;
  str += strnlen(str, len);
  while (str != se)
    *str++ = pad;
}

unsigned int ID3V1_TAG::DetectCodepage(unsigned int codepage) const
{
  ID3V1_TAG buf = *this;
  strpad(buf.title,  sizeof buf.title,  ' ');
  strpad(buf.artist, sizeof buf.artist, ' ');
  strpad(buf.album,  sizeof buf.album,  ' ');
  strpad(buf.year,   sizeof buf.year,   ' ');
  strpad(buf.comment,sizeof buf.comment,' ');
  buf.genre = 0;
  return ch_detect(codepage, buf.title);
}
