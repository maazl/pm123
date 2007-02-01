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

#include "pm123.h"
#include "utilfct.h"
#include "plugman.h"

extern OUTPUT_PARAMS out_params;

/* Constructs a string of the displayable text from the file information. */
char*
amp_construct_tag_string( char* result, const DECODER_INFO* info, int size )
{
  *result = 0;

  if( *info->artist ) {
    strlcat( result, info->artist, size );
    if( *info->title ) {
      strlcat( result, ": ", size );
    }
  }

  if( *info->title ) {
    strlcat( result, info->title, size );
  }

  if( *info->album && *info->year )
  {
    strlcat( result, " (", size );
    strlcat( result, info->album, size );
    strlcat( result, ", ", size );
    strlcat( result, info->year,  size );
    strlcat( result, ")",  size );
  }
  else
  {
    if( *info->album && !*info->year )
    {
      strlcat( result, " (", size );
      strlcat( result, info->album, size );
      strlcat( result, ")",  size );
    }
    if( !*info->album && *info->year )
    {
      strlcat( result, " (", size );
      strlcat( result, info->year, size );
      strlcat( result, ")",  size );
    }
  }

  if( *info->comment )
  {
    strlcat( result, " -- ", size );
    strlcat( result, info->comment, size );
  }

  return result;
}

/* Constructs a information text for currently loaded file
   and selects it for displaying. */
void
amp_display_filename( void )
{
  char display[512];

  if( amp_playmode == AMP_NOFILE ) {
    bmp_set_text( "No file loaded" );
    return;
  }

  switch( cfg.viewmode )
  {
    case CFG_DISP_ID3TAG:
      amp_construct_tag_string( display, &current_info, sizeof( display ));

      if( *display ) {
        bmp_set_text( display );
        break;
      }

      // if tag is empty - use filename instead of it.

    case CFG_DISP_FILENAME:
      if( current_filename != NULL && *current_filename )
      {
        if( is_url( current_filename )) {
          bmp_set_text( sdecode( display, current_filename, sizeof( display )));
        } else {
          bmp_set_text( sfname ( display, current_filename, sizeof( display )));
        }
      } else {
         bmp_set_text( "This is a bug!" );
      }
      break;

    case CFG_DISP_FILEINFO:
      bmp_set_text( current_info.tech_info );
      break;
  }
}

/* Switches to the next text displaying mode. */
void
amp_display_next_mode( void )
{
  if( cfg.viewmode == CFG_DISP_FILEINFO ) {
    cfg.viewmode = CFG_DISP_FILENAME;
  } else {
    cfg.viewmode++;
  }

  amp_display_filename();
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

ULONG handle_dfi_error( ULONG rc, const char* file )
{
 char buf[512];

 if (rc == 0) return 0;

 *buf = '\0';

 if( rc == 100 ) {
   sprintf( buf, "The file %s could not be read.", file );
 } else if( rc == 200 ) {
   sprintf( buf, "The file %s cannot be played by PM123. The file might be corrupted or the necessary plug-in not loaded or enabled.", file );
 } else {
   amp_stop();
   sprintf( buf, "%s: Error occurred: %s", file, xio_strerror( rc ));
 }

 WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, buf, "Error", 0, MB_ERROR | MB_OK );
 return 1;
}

void DLLENTRY pm123_control( int index, void* param )
{
  switch (index)
  {
    case CONTROL_NEXTMODE:
      amp_display_next_mode();
      break;
  }
}

int DLLENTRY pm123_getstring( int index, int subindex, size_t bufsize, char* buf )
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
     strlcpy(buf, current_filename, bufsize);
     break;
   default: break;
  }
 return(0);
}


