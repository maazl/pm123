/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <os2.h>
#include <string.h>

#include <utilfct.h>

#include "pm123.h"
#include "playable.h"

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

