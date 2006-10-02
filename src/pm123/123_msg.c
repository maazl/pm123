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

#include "pm123.h"
#include "plugman.h"

//#define DEBUG 1
#include <debuglog.h>


static DECODER_PARAMS2 dec_params;

static BOOL  paused        = FALSE;
static BOOL  forwarding    = FALSE;
static BOOL  rewinding     = FALSE;
static char* save_filename = NULL;

/* Returns TRUE if the decoder is paused. */
BOOL is_paused( void ) {
  return paused;
}

/* Returns TRUE if the decoder is fast forwarding. */
BOOL is_forward( void ) {
  return forwarding;
}

/* Returns TRUE if the decoder is rewinding. */
BOOL is_rewind( void ) {
  return rewinding;
}

void
amp_msg( int msg, void *param, void *param2 )
{
  ULONG rc;

  memset( &dec_params, '\0', sizeof( dec_params ));

  switch( msg ) {
    case MSG_PLAY:
    {
      MSG_PLAY_STRUCT* data = (MSG_PLAY_STRUCT*)param;
      char cdda_url[20];
      const char* url;

      // TODO: URI!!!
      rc = out_setup( (FORMAT_INFO2*)param2, data->out_filename );
      DEBUGLOG(("amp_msg:MSG_PLAY: after setup %d %d - %d\n", ((FORMAT_INFO2*)param2)->samplerate, ((FORMAT_INFO2*)param2)->channels, rc));
      if( rc != 0 ) {
        return;
      }

      equalize_sound( gains, mutes, preamp, cfg.eq_enabled );
      dec_save( save_filename );
      
      if (data->drive != NULL && *data->drive != 0 && data->track != 0) // pm123 core sometimes passes trash in the track field
      { sprintf(cdda_url, "cdda:///%s/track%02d", data->drive, data->track);
        url = cdda_url;
      } else
      { url = data->filename;
      }

      rc = dec_play( url, data->decoder_needed );
      if( rc == -2 ) {
        char buf[1024];
        sprintf( buf, "Error: Decoder module %.8s needed to play %.800s is not loaded or enabled.",
                      data->decoder_needed, url );
        keep_last_error( buf );
      } else if ( rc != 0 ) {
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

        dec_stop();
      }

      forwarding = FALSE;
      rewinding  = FALSE;
      paused     = FALSE;

      if( out_playing_data()) {
        out_trashbuffers( out_playing_pos() );
      }

      rc = out_close();
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
        out_pause( paused = !paused );
      }
      break;

    case MSG_FWD:
      if( decoder_playing())
      {
        rewinding = FALSE;
        forwarding = !forwarding;
        if( dec_fast( forwarding ? DECODER_FAST_FORWARD : DECODER_NORMAL_PLAY ) != 0 ) {
          forwarding = FALSE;
        } else if( cfg.trash ) {
          // Going back in the stream to what is currently playing.
          int pos = out_playing_pos();
          dec_jump( pos );
          out_trashbuffers( pos );
        }
      }
      break;

    case MSG_REW:
      if( decoder_playing())
      {
        forwarding = FALSE;
        rewinding = !rewinding;
        if( dec_fast( rewinding ? DECODER_REW : DECODER_NORMAL_PLAY ) != 0 ) {
          rewinding = FALSE;
        } else if( cfg.trash ) {
          // Going back in the stream to what is currently playing.
          int pos = out_playing_pos();
          dec_jump( pos );
          out_trashbuffers( pos );
        }
      }
      break;

    case MSG_JUMP:
      if( decoder_playing())
      {
        if ( dec_jump( *(int*)param ) != 0 ) {
          // cancel seek immediately
          WinPostMsg( amp_player_window(), WM_SEEKSTOP, 0, 0 );
        } else if( cfg.trash ) {
          out_trashbuffers( *(int*)param );
        }
      }
      break;

   case MSG_SAVE:
      free( save_filename );
      save_filename = NULL;
      if( param != NULL )
        save_filename = (char*)strdup((char*)param);

      if( decoder_playing()) {
        dec_save( save_filename );
      }
      break;
  }
}

void equalize_sound( const float *gains, const BOOL *mute, float preamp, BOOL enabled )
{
  if (enabled) {
    int i;
    float bandgain[20];
    memcpy( bandgain, gains, sizeof( bandgain ));

    for( i = 0; i < 20; i++ )
      if( mute[i] )
        bandgain[i] = 0;

    for( i = 0; i < 20; i++ )
      bandgain[i] *= preamp;

    dec_eq( bandgain );
  } else {
    dec_eq( NULL );
  }
  
}

