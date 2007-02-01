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

#define  INCL_WIN
#define  INCL_DOS
#define  INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debuglog.h>

#include "pm123.h"
#include "utilfct.h"
#include "plugman.h"
#include "messages.h"

static OUTPUT_PARAMS out_params;

static BOOL paused     = FALSE;
static BOOL forwarding = FALSE;
static BOOL rewinding  = FALSE;

static char proxyurl[1024];
static char httpauth[1024];
static char metadata[128];
static char savename[_MAX_PATH];

/* Returns TRUE if the player is paused. */
BOOL
is_paused( void ) {
  return paused;
}

/* Returns TRUE if the player is fast forwarding. */
BOOL
is_forward( void ) {
  return forwarding;
}

/* Returns TRUE if the player is rewinding. */
BOOL
is_rewind( void ) {
  return rewinding;
}

/* Returns TRUE if the output is always hungry. */
BOOL
is_always_hungry( void ) {
  return out_params.always_hungry;
}

/* Begins playback of the specified file. */
BOOL
msg_play( HWND hwnd, const MSG_PLAY_STRUCT* play, int pos )
{
  // TODO: URI!!!
  ULONG rc = out_setup( &play->info.format, play->url );
  DEBUGLOG(("amp_msg:MSG_PLAY: after setup %d %d - %d\n", play->info.format.samplerate, play->info.format.channels, rc));
  if( rc != 0 ) {
    return FALSE;
  }

  msg_equalize( gains, mutes, preamp, cfg.eq_enabled );
  dec_save( savename );
  
  rc = dec_play( play->url, play->decoder, pos );
  if( rc == -2 ) {
    char buf[1024];
    sprintf( buf, "Error: Decoder module %.8s needed to play %.800s is not loaded or enabled.", play->decoder, play->url );
    amp_display_error( buf );
    return FALSE;
  } else if ( rc != 0 ) {
    amp_post_message( WM_PLAYERROR, 0, 0 );
    return FALSE;
  }

  amp_volume_adjust();

  while( dec_status() == DECODER_STARTING ) {
    DosSleep( 1 );
  }
  
  return TRUE;
}

/* Stops playback of the currently played file. */
BOOL
msg_stop( void )
{
  ULONG status = dec_status();
  ULONG rc;

  if( status == DECODER_PLAYING || status == DECODER_STARTING )
  {
    if( paused ) {
      msg_pause();
    }
    if( forwarding ) {
      msg_forward();
    }
    if( rewinding ) {
      msg_rewind();
    }

    dec_stop();

    forwarding = FALSE;
    rewinding  = FALSE;
    paused     = FALSE;
  }

  rc = out_close();
  if( rc != 0 ) {
    return FALSE;
  }

  while( decoder_playing()) {
    DosSleep( 1 );
  }

  return TRUE;
}

/* Suspends or resumes playback of the currently played file. */
BOOL
msg_pause( void )
{
  if( decoder_playing()) {
    out_pause( paused = !paused );
  }
  return TRUE;
}

/* Toggles a fast forward of the currently played file. */
BOOL
msg_forward( void )
{
  if( decoder_playing())
  {
    rewinding = FALSE;
    forwarding = !forwarding;
    if( dec_fast( forwarding ? DECODER_FAST_FORWARD : DECODER_NORMAL_PLAY ) != 0 ) {
      forwarding = FALSE;
    } else if( cfg.trash ) {
      // Going back in the stream to what is currently playing.
      dec_jump( out_playing_pos() );
    }
  }
  return TRUE;
}

/* Toggles a rewind of the currently played file. */
BOOL
msg_rewind( void )
{
  if( decoder_playing())
  {
    forwarding = FALSE;
    rewinding = !rewinding;
    if( dec_fast( rewinding ? DECODER_FAST_REWIND : DECODER_NORMAL_PLAY ) != 0 ) {
      rewinding = FALSE;
    } else if( cfg.trash ) {
      // Going back in the stream to what is currently playing.
      dec_jump( out_playing_pos() );
    }
  }
  return TRUE;
}

/* Changes the current playing position of the currently played file. */
BOOL
msg_seek( int pos )
{
  if( decoder_playing())
  {
    if ( dec_jump( pos ) != 0 ) {
      // cancel seek immediately
      WinPostMsg( amp_player_window(), WM_SEEKSTOP, 0, 0 );
    }
  }
  return TRUE;
}

/* Toggles a saving of the currently played stream. */
BOOL
msg_savestream( const char* filename )
{
  if( filename ) {
    strlcpy( savename, filename, sizeof( savename ));
  } else {
    *savename = 0;
  }

  if( decoder_playing()) {
    dec_save( savename );
  }
  return TRUE;
}

/* Toggles a equalizing of the currently played file. */
BOOL
msg_equalize( const float *gains, const BOOL *mute, float preamp, BOOL enabled )
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
  return TRUE;
}

