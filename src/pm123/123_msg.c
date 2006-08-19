/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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
#define  INCL_DOSPROCESS
#define  INCL_DOSEXCEPTIONS
#define  INCL_WINSTDDRAG
#define  INCL_DOS
#define  INCL_DOSNMPIPES
#define  INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utilfct.h>
#include <format.h>
#include "pm123.h"
#include "plugman.h"

#define DEBUG 1

extern void PM123_ENTRY keep_last_error( char* );
extern void PM123_ENTRY display_info( char* );

static DECODER_PARAMS2 dec_params;
OUTPUT_PARAMS2 out_params;
/* Loaded in curtun on decoder's demand WM_METADATA. */
static char  metadata_buffer[128];

static BOOL  paused        = FALSE;
static BOOL  forwarding    = FALSE;
static BOOL  rewinding     = FALSE;
static char* save_filename = NULL;

void
amp_msg( int msg, void *param, void *param2 )
{
  ULONG rc;
  char buf[256];

  memset( &dec_params, '\0', sizeof( dec_params ));

  switch( msg ) {
    case MSG_PLAY:
    {
      MSG_PLAY_STRUCT* data = (MSG_PLAY_STRUCT*)param;
      char cdda_url[20];

      out_params.hwnd          = data->hMain;
      out_params.error_display = keep_last_error;
      out_params.info_display  = display_info;

      rc = out_command( OUTPUT_SETUP, &out_params );
      if( rc != 0 ) {
        return;
      }

      // TODO: URI!!!
      out_params.URI = data->out_filename;
      rc = out_command( OUTPUT_OPEN, &out_params );
      if( rc != 0 ) {
        return; // WM_PLAYERROR already sent .. hum..
      }

      rc = dec_set_name_active( data->decoder_needed );
      if( rc == -2 ) {
        sprintf( buf, "Error: Decoder module %s needed to play %s is not loaded or enabled.",
                       data->decoder_needed, data->filename );
        keep_last_error( buf );
      }
      if( rc == -1 || rc == -2 ) {
        amp_post_message( WM_PLAYERROR, 0, 0 );
        return;
      }

      equalize_sound( gains, mutes, preamp, cfg.eq_enabled );

      #ifdef DEBUG
      fprintf(stderr, "amp_msg: MSG_PLAY: %s, %s, %d\n", data->filename, data->drive, data->track);
      #endif
      if (data->drive != NULL && *data->drive != 0 && data->track != 0) // pm123 core sometimes passes trash in the track field
      { sprintf(cdda_url, "cdda:///%s/track%02d", data->drive, data->track);
        dec_params.URL = cdda_url;
      } else
        dec_params.URL      = data->filename;
      dec_params.hwnd       = data->hMain;
      dec_params.buffersize = cfg.bufsize*1024;
      dec_params.bufferwait = cfg.bufwait;
      dec_params.proxyurl   = cfg.proxy;
      dec_params.httpauth   = cfg.auth;
      dec_params.error_display    = keep_last_error;
      dec_params.info_display     = display_info;
      dec_params.metadata_buffer  = metadata_buffer;
      dec_params.metadata_size    = sizeof(metadata_buffer);

      dec_command( DECODER_SETUP, &dec_params );
      dec_params.save_filename = save_filename;
      dec_command( DECODER_SAVEDATA, &dec_params );

      rc = dec_command( DECODER_PLAY, &dec_params );
      if( rc != 0 ) {
        amp_post_message( WM_PLAYERROR, 0, 0 );
        return;
      }
      amp_volume_to_normal();

      while( dec_status() == DECODER_STARTING ) {
        DosSleep( 50 );
      }

      break;
    }

    case MSG_STOP:
    {
      ULONG status = dec_status();
      ULONG rc;

      if( status == DECODER_PLAYING || status == DECODER_STARTING )
      {
        if( paused ) {
          amp_msg( MSG_PAUSE, NULL, 0 );
        }
        if( forwarding ) {
          amp_msg( MSG_FWD, NULL, 0 );
        }
        if( rewinding ) {
          amp_msg( MSG_REW, NULL, 0 );
        }

        rc = dec_command( DECODER_STOP, &dec_params );

        if( rc != 0 ) {
          amp_post_message( WM_PLAYERROR, 0, 0 );
        }
      }

      forwarding = FALSE;
      rewinding  = FALSE;
      paused     = FALSE;

      if( out_playing_data()) {
        out_command( OUTPUT_TRASH_BUFFERS, &out_params );
      }

      rc = out_command( OUTPUT_CLOSE, &out_params );

      if( rc != 0 ) {
        return;
      }

      while( decoder_playing()) {
        DosSleep( 50 );
      }
      break;
    }

    case MSG_PAUSE:
      if( decoder_playing()) {
        out_params.pause = paused = !paused;
        out_command( OUTPUT_PAUSE, &out_params );
      }
      break;

    case MSG_FWD:
      if( decoder_playing())
      {
        forwarding = !forwarding;

        if (rewinding) // stop rewinding anyway
        {
          dec_params.ffwd = rewinding = FALSE;
          dec_command( DECODER_REW, &dec_params );
        }
        //out_params.nobuffermode = FALSE;
        //out_command( OUTPUT_NOBUFFERMODE, &out_params );
        dec_params.ffwd = forwarding;
        dec_command( DECODER_FFWD, &dec_params );
        if( cfg.trash )
        {
          /* going back in the stream to what is currently playing */
          dec_params.jumpto = out_playing_pos();
          dec_command( DECODER_JUMPTO, &dec_params );
          //out_params.nobuffermode = TRUE;
          //out_command( OUTPUT_NOBUFFERMODE, &out_params );
          out_params.temp_playingpos = out_playing_pos();
          out_command( OUTPUT_TRASH_BUFFERS, &out_params );
        }
      }
      break;

    case MSG_REW:
      if( decoder_playing())
      {
        rewinding = !rewinding;

        if (forwarding) // stop forwarding anyway
        {
          dec_params.ffwd = forwarding = FALSE;
          dec_command( DECODER_FFWD, &dec_params );
        }
        //out_params.nobuffermode = FALSE;
        //out_command( OUTPUT_NOBUFFERMODE, &out_params );
        dec_params.rew = rewinding;
        dec_command( DECODER_REW,&dec_params );
        if( cfg.trash )
        {
          /* going back in the stream to what is currently playing */
          dec_params.jumpto = out_playing_pos();
          dec_command( DECODER_JUMPTO, &dec_params );
          //out_params.nobuffermode = TRUE;
          //out_command( OUTPUT_NOBUFFERMODE, &out_params );
          out_params.temp_playingpos = out_playing_pos();
          out_command( OUTPUT_TRASH_BUFFERS, &out_params );
        }
      }
      break;

    case MSG_JUMP:
      if( decoder_playing())
      {
        dec_params.jumpto = *((int *)param);
        if( cfg.trash )
        {
          out_params.temp_playingpos = dec_params.jumpto;
          out_command( OUTPUT_TRASH_BUFFERS, &out_params );
        }
        dec_command( DECODER_JUMPTO, &dec_params );
      }
      break;

   case MSG_SAVE:
      free( save_filename );
      save_filename = NULL;
      if( param != NULL )
        save_filename = (char*)strdup((char*)param);

      if( decoder_playing()) {
        dec_params.save_filename = (char*)param;
        dec_command( DECODER_SAVEDATA, &dec_params );
      }
      break;
  }
}

void equalize_sound( float *gains, BOOL *mute, float preamp, BOOL enabled )
{
  int i;
  float bandgain[20];
  memcpy( bandgain, gains, sizeof( bandgain ));

  dec_params.bandgain  = bandgain;
  dec_params.equalizer = enabled;

  for( i = 0; i < 20; i++ )
    if( mute[i] )
      bandgain[i] = 0;

  for( i = 0; i < 20; i++ )
    bandgain[i] *= preamp;

  dec_command( DECODER_EQ, &dec_params );
}

