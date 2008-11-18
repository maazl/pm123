/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 M.Mueller
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

#define  INCL_WIN
#define  INCL_DOS

#include <utilfct.h>
#include <snprintf.h>

#include "123_util.h"
#include "pm123.h"
#include "playable.h"
#include "copyright.h"
#include "skin.h" // bmp_query_text
#include "controller.h"
#include <visual_plug.h>

#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <direct.h>


void DLLENTRY
pm123_display_info( const char* info )
{
  char* message = strdup( info );
  if( message ) {
    WinPostMsg( amp_player_window(), AMP_DISPLAY_MESSAGE, MPFROMP( message ), MPFROMLONG( FALSE ));
  }
}

void DLLENTRY
pm123_display_error( const char *info )
{
  char* message = strdup( info );
  if( message ) {
    WinPostMsg( amp_player_window(), AMP_DISPLAY_MESSAGE, MPFROMP( message ), MPFROMLONG( TRUE ));
  }
}

void DLLENTRY pm123_control( int index, void* param )
{
  switch (index)
  {
    case CONTROL_NEXTMODE:
      WinSendMsg( amp_player_window(), AMP_DISPLAY_MODE, 0, 0 );
      break;
  }
}

int DLLENTRY pm123_getstring( int index, int subindex, size_t bufsize, char* buf )
{ if (bufsize)
    *buf = 0;
  switch (index)
  {case STR_VERSION:
    strlcpy( buf, AMP_FULLNAME, bufsize );
    break;

   case STR_DISPLAY_TEXT:
    strlcpy( buf, bmp_query_text(), bufsize );
    break;

   case STR_FILENAME:
    { int_ptr<Song> song = Ctrl::GetCurrentSong();
      if (song)
        strlcpy(buf, song->GetURL(), bufsize);
      break;
    }
   
   case STR_DISPLAY_TAG:
    { int_ptr<Song> song = Ctrl::GetCurrentSong();
      if (song)
      { const xstring& text = amp_construct_tag_string( &song->GetInfo() );
        strlcpy(buf, text, bufsize);
      }
      break;
    }
    
   case STR_DISPLAY_INFO:
    { int_ptr<Song> song = Ctrl::GetCurrentSong();
      if (song)
        strlcpy(buf, song->GetInfo().tech->info, bufsize);
      break;
    }

   default: break;
  }
 return(0);
}

/* Constructs a string of the displayable text from the file information. */
xstring
amp_construct_tag_string( const DECODER_INFO2* info )
{
  // TODO: a string builder would be nice...
  xstring result = xstring::empty;

  if( *info->meta->artist ) {
    result = info->meta->artist;
    if( *info->meta->title )
      result = result + ": ";
  }

  if( *info->meta->title )
    result = result + info->meta->title;

  if( *info->meta->album && *info->meta->year )
    result = xstring::sprintf("%s (%s, %s)", result.cdata(), info->meta->album, info->meta->year);
  else if( *info->meta->album )
    result = xstring::sprintf("%s (%s)", result.cdata(), info->meta->album);
  else if( *info->meta->year )
    result = xstring::sprintf("%s (%s)", result.cdata(), info->meta->year);

  if( *info->meta->comment )
    result = xstring::sprintf("%s -- %s", result.cdata(), info->meta->comment);

  return result;
}

/* Get current working directory */
url123 amp_get_cwd()
{ char cdir[_MAX_PATH+1];
  RASSERT(_getcwd(cdir, sizeof cdir -1));
  size_t len = strlen(cdir);
  // Append trailing slash?
  if (cdir[len-1] != '\\')
  { cdir[len] = '\\';
    cdir[len+1] = 0; 
  }
  return url123::normalizeURL(cdir);
}

/* Reads url from specified file. */
xstring
amp_url_from_file(const char* filename)
{
  FILE* file = fopen(filename, "r");
  if (!file)
    return (char*)NULL;
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  if (len > 4095) // some limit...
    len = 4095;
  fseek(file, 0, SEEK_SET);
  xstring ret;
  if (len > 0)
  { char* dp = ret.raw_init(len);
    if (fgets(dp, len, file))
      blank_strip(dp);
    else
      ret = NULL;
  }
  fclose(file);
  return ret;
}

xstring amp_string_from_drghstr(HSTR hstr)
{ size_t len = DrgQueryStrNameLen(hstr);
  xstring ret;
  DrgQueryStrName(hstr, len+1, ret.raw_init(len));
  return ret;
}

xstring amp_font_attrs_to_string(const FATTRS& attrs, unsigned size)
{ xstring ret = xstring::sprintf("%u.%s", size, attrs.szFacename);
  if (attrs.fsSelection & FATTR_SEL_BOLD      )
    ret = ret + ".Bold";
  if (attrs.fsSelection & FATTR_SEL_UNDERSCORE)
    ret = ret + ".Underscore";
  if (attrs.fsSelection & FATTR_SEL_ITALIC    )
    ret = ret + ".Italic";
  if (attrs.fsSelection & FATTR_SEL_OUTLINE   )
    ret = ret + ".Outline";
  if (attrs.fsSelection & FATTR_SEL_STRIKEOUT )
    ret = ret + ".Strikeout";
  return ret;
}

bool amp_string_to_font_attrs(FATTRS& attrs, unsigned& size, const char* name)
{ size_t n;
  if (sscanf(name, "%u.%32[^.]%n", &size, attrs.szFacename, &n) != 2)
    return false;
  // Attributes
  name += n;
  if (strstr(name, ".Bold"))
    attrs.fsSelection |= FATTR_SEL_BOLD;
  if (strstr(name, ".Underscore"))
    attrs.fsSelection |= FATTR_SEL_UNDERSCORE;
  if (strstr(name, ".Italic"))
    attrs.fsSelection |= FATTR_SEL_ITALIC;
  if (strstr(name, ".Outline"))
    attrs.fsSelection |= FATTR_SEL_OUTLINE;
  if (strstr(name, ".Strikeout"))
    attrs.fsSelection |= FATTR_SEL_STRIKEOUT;
  return true;
}
