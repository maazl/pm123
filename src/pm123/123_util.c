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
#define  INCL_GPI
#define  INCL_DOS
#define  INCL_OS2MM
#include <os2.h>
#include <os2me.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <utilfct.h>

#include "pm123.h"
#include "plugman.h"


/* Constructs a string of the displayable text from the ID3 tag. */
char*
amp_construct_tag_string( char* result, const META_INFO* tag )
{
  *result = 0;

  if( *tag->artist ) {
    strcat( result, tag->artist );
    if( *tag->title ) {
      strcat( result, ": " );
    }
  }

  if( *tag->title ) {
    strcat( result, tag->title );
  }

  if( *tag->album && *tag->year )
  {
    strcat( result, " (" );
    strcat( result, tag->album );
    strcat( result, ", " );
    strcat( result, tag->year  );
    strcat( result, ")" );
  }
  else
  {
    if( *tag->album && !*tag->year )
    {
      strcat( result, " (" );
      strcat( result, tag->album );
      strcat( result, ")" );
    }
    if( !*tag->album && *tag->year )
    {
      strcat( result, " (" );
      strcat( result, tag->year);
      strcat( result, ")" );
    }
  }

  if( *tag->comment )
  {
    strlcat( result, " -- ", sizeof( result ));
    strlcat( result, tag->comment, sizeof( result ));
  }

  return result;
}

/* Converts time to two integer suitable for display by the timer. */
void
sec2num( long seconds, int* major, int* minor )
{
  int mi = seconds % 60;
  int ma = seconds / 60;

  if( ma > 99 ) {
    mi = ma % 60; // minutes
    ma = ma / 60; // hours
  }

  if( ma > 99 ) {
    mi = ma % 24; // hours
    ma = ma / 24; // days
  }

  *major = ma;
  *minor = mi;
}

void PM123_ENTRY pm123_control(int index, void *param)
{
  switch (index)
  {
    case CONTROL_NEXTMODE:
      amp_display_next_mode();
      break;
  }
}

int PM123_ENTRY pm123_getstring(int index, int subindex, size_t bufsize, char *buf)
{
 switch (index)
  {
   case STR_NULL: *buf = '\0'; break;
   case STR_VERSION:
     strlcpy( buf, AMP_FULLNAME, bufsize );
     break;
   case STR_DISPLAY_TEXT:
     strlcpy( buf, bmp_query_text(), bufsize );
     break;
   case STR_FILENAME:
   { const MSG_PLAY_STRUCT* current = amp_get_current_file();
     strlcpy(buf, current != NULL ? current->url : "", bufsize);
     break;
   }
   default: break;
  }
 return(0);
}


