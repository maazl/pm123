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
#include <utilfct.h>

#include "pm123.h"
#include "plugman.h"
#include "messages.h"
#include "assertions.h"

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

/* Returns TRUE if the the currently played
   stream is saved. */
BOOL
is_stream_saved( void ) {
  return *savename;
}

/* Begins playback of the specified file. Must be called
   from the main thread. */
BOOL
msg_play( HWND hwnd, char* filename, char* decoder, const FORMAT_INFO* format, int pos )
{
  char  cd_drive[3] = "";
  char  errorbuf[1024];
  int   samplesize;
  ULONG rc;

  DECODER_PARAMS dec_params = { 0 };

  ASSERT_IS_MAIN_THREAD;

  out_params.size          = sizeof( out_params );
  out_params.hwnd          = hwnd;
  out_params.boostclass    = PRTYC_TIMECRITICAL;
  out_params.normalclass   = PRTYC_REGULAR;
  out_params.boostdelta    = 0;
  out_params.normaldelta   = 31;
  out_params.error_display = pm123_display_error;
  out_params.info_display  = pm123_display_info;
  out_params.formatinfo    = *format;
  out_params.info          = &current_info;

  if( stricmp( decoder, "dpmikmod" ) == 0 ||
      stricmp( decoder, "wvplay"   ) == 0 ) {
    // Some too old plug-in ignores the suggested audio buffer size
    // and always uses a 16K buffer.
    out_params.buffersize = 16384;
  } else {
    // For fast reaction of the player we allocate the buffer in
    // length approximately in 100 milliseconds.
    samplesize = format->channels * ( format->bits / 8 );
    if( samplesize ) {
      out_params.buffersize = samplesize * format->samplerate / 10;
    } else {
      out_params.buffersize = 16384;
    }

    // Buffers are limited to 64K on Intel machines.
    if( out_params.buffersize > 65535 ) {
      out_params.buffersize = 65535;
    }

    // The buffer size must be an integer product of the number of
    // channels and of the number of bits in the sample. Because of
    // poor design of the mpg123 this size also must be an integer
    // product of the 128.
    if( samplesize ) {
      out_params.buffersize = out_params.buffersize / samplesize / 128 * samplesize * 128;
    } else {
      out_params.buffersize = out_params.buffersize / 128 * 128;
    }
  }

  if( out_command( OUTPUT_SETUP, &out_params ) != 0 ) {
    return FALSE;
  }

  out_params.filename = filename;

  if( out_command( OUTPUT_OPEN, &out_params ) != 0 ) {
    return FALSE;
  }

  if( dec_set_active( decoder ) == PLUGIN_FAILED )
  {
    strlcpy( errorbuf, "Decoder module ", sizeof( errorbuf ));
    strlcat( errorbuf, decoder, sizeof( errorbuf ));
    strlcat( errorbuf, " needed to play ", sizeof( errorbuf ));
    strlcat( errorbuf, filename, sizeof( errorbuf ));
    strlcat( errorbuf, " is not loaded or enabled.", sizeof( errorbuf ));

    msg_stop();
    pm123_display_error( errorbuf );
    return FALSE;
  }

  msg_equalize( gains, mutes, preamp, cfg.eq_enabled );

  if( *cfg.proxy_host ) {
    strlcpy( proxyurl, cfg.proxy_host, sizeof( proxyurl ));
    strlcat( proxyurl, ":", sizeof( proxyurl ));
    strlcat( proxyurl, ltoa( cfg.proxy_port, errorbuf, 10 ), sizeof( proxyurl ));
  } else {
    *proxyurl = 0;
  }

  if( *cfg.proxy_user || *cfg.proxy_pass ) {
    strlcpy( httpauth, cfg.proxy_user, sizeof( httpauth ));
    strlcat( httpauth, ":", sizeof( httpauth ));
    strlcat( httpauth, cfg.proxy_pass, sizeof( httpauth ));
  } else {
    *httpauth = 0;
  }

  dec_params.filename   = filename;
  dec_params.hwnd       = hwnd;
  dec_params.buffersize = cfg.buff_size * 1024;
  dec_params.bufferwait = cfg.buff_wait;
  dec_params.proxyurl   = proxyurl;
  dec_params.httpauth   = httpauth;
  dec_params.drive      = cd_drive;

  if( is_track( filename )) {
    sdrive( cd_drive, filename, sizeof( cd_drive ));
    dec_params.track = strack( filename );
  } else {
    dec_params.track = 0;
  }

  dec_params.audio_buffersize = out_params.buffersize;
  dec_params.error_display    = pm123_display_error;
  dec_params.info_display     = pm123_display_info;
  dec_params.metadata_buffer  = metadata;
  dec_params.metadata_size    = sizeof( metadata );

  dec_set_filters( &dec_params );
  dec_command( DECODER_SETUP, &dec_params );

  dec_params.save_filename = is_stream_saved() ? savename : NULL;
  if( dec_command( DECODER_SAVEDATA, &dec_params ) != PLUGIN_OK && is_stream_saved()) {
    pm123_display_error( "The current active decoder don't support saving of a stream.\n" );
    *savename = 0;
  }

  dec_params.jumpto = pos;
  rc = dec_command( DECODER_PLAY, &dec_params );

  if( rc != PLUGIN_OK && rc != PLUGIN_GO_ALREADY ) {
    msg_stop();
    return FALSE;
  }

  out_set_volume( truncate( cfg.defaultvol, 0, 100 ));

  while( dec_status() == DECODER_STARTING ) {
    DosSleep(1);
  }

  return TRUE;
}

/* Stops playback of the currently played file.
   Must be called from the main thread. */
BOOL
msg_stop( void )
{
  ULONG status = dec_status();
  DECODER_PARAMS dec_params = { 0 };
  ULONG rc;

  ASSERT_IS_MAIN_THREAD;

  if( status == DECODER_PLAYING  ||
      status == DECODER_STARTING ||
      status == DECODER_PAUSED   )
  {
    if( paused ) {
      out_set_volume( 0 );
      msg_pause();
    }
    if( forwarding ) {
      msg_forward();
    }
    if( rewinding ) {
      msg_rewind();
    }

    rc = dec_command( DECODER_STOP, &dec_params );

    if( rc != PLUGIN_OK && rc != PLUGIN_GO_ALREADY ) {
      return FALSE;
    }
  }

  forwarding = FALSE;
  rewinding  = FALSE;
  paused     = FALSE;

  if( out_playing_data()) {
    out_command( OUTPUT_TRASH_BUFFERS, &out_params );
  }

  rc = out_command( OUTPUT_CLOSE, &out_params );

  while( decoder_playing()) {
    DEBUGLOG(( "pm123: wait stopping, dec_status=%d, out_playing_data=%d\n",
                dec_status(), out_playing_data()));
    DosSleep(1);
  }

  dec_set_filters( NULL );
  return rc == 0;
}

/* Suspends or resumes playback of the currently played file.
   Must be called from the main thread. */
BOOL
msg_pause( void )
{
  ASSERT_IS_MAIN_THREAD;

  if( decoder_playing())
  {
    out_params.pause = paused = !paused;

    if( paused ) {
      out_command( OUTPUT_PAUSE, &out_params );
    } else {
      out_command( OUTPUT_PAUSE, &out_params );
    }
  }
  return TRUE;
}

/* Toggles a fast forward of the currently played file.
   Must be called from the main thread. */
BOOL
msg_forward( void )
{
  DECODER_PARAMS dec_params = { 0 };
  ASSERT_IS_MAIN_THREAD;

  if( decoder_playing()) {
    if( rewinding ) {
      // Stop rewinding anyway.
      dec_params.rew = rewinding = FALSE;
      dec_command( DECODER_REW, &dec_params );
    }

    dec_params.ffwd = forwarding = !forwarding;

    if( dec_command( DECODER_FFWD, &dec_params ) != PLUGIN_OK ) {
      forwarding = FALSE;
    } else if( cfg.trash ) {
      // Going back in the stream to what is currently playing.
      dec_params.jumpto = out_playing_pos();
      dec_command( DECODER_JUMPTO, &dec_params );
      out_params.temp_playingpos = out_playing_pos();
      out_command( OUTPUT_TRASH_BUFFERS, &out_params );
    }
  }
  return TRUE;
}

/* Toggles a rewind of the currently played file.
   Must be called from the main thread. */
BOOL
msg_rewind( void )
{
  DECODER_PARAMS dec_params = { 0 };
  ASSERT_IS_MAIN_THREAD;

  if( decoder_playing()) {
    if( forwarding ) {
      // Stop forwarding anyway.
      dec_params.ffwd = forwarding = FALSE;
      dec_command( DECODER_FFWD, &dec_params );
    }

    dec_params.rew = rewinding = !rewinding;

    if( dec_command( DECODER_REW, &dec_params ) != PLUGIN_OK ) {
      rewinding = FALSE;
    } else if( cfg.trash ) {
      // Going back in the stream to what is currently playing.
      dec_params.jumpto = out_playing_pos();
      dec_command( DECODER_JUMPTO, &dec_params );
      out_params.temp_playingpos = out_playing_pos();
      out_command( OUTPUT_TRASH_BUFFERS, &out_params );
    }
  }
  return TRUE;
}

/* Changes the current playing position of the currently played file.
   Must be called from the main thread. */
BOOL
msg_seek( int pos )
{
  DECODER_PARAMS dec_params = { 0 };
  ASSERT_IS_MAIN_THREAD;

  if( decoder_playing()) {
    dec_params.jumpto = pos;

    if( cfg.trash ) {
      out_params.temp_playingpos = dec_params.jumpto;
      out_command( OUTPUT_TRASH_BUFFERS, &out_params );
    }

    dec_command( DECODER_JUMPTO, &dec_params );
  }
  return TRUE;
}

/* Toggles a saving of the currently played stream.
   Must be called from the main thread. */
BOOL
msg_savestream( const char* filename )
{
  DECODER_PARAMS dec_params = { 0 };
  ASSERT_IS_MAIN_THREAD;

  if( filename ) {
    strlcpy( savename, filename, sizeof( savename ));
    dec_params.save_filename = savename;
  } else {
    *savename = 0;
    dec_params.save_filename = NULL;
  }

  if( decoder_playing()) {
    if( dec_command( DECODER_SAVEDATA, &dec_params ) != PLUGIN_OK && filename ) {
      pm123_display_error( "The current active decoder don't support saving of a stream.\n" );
      *savename = 0;
    }
  }
  return TRUE;
}

/* Toggles a equalizing of the currently played file.
   Must be called from the main thread. */
BOOL
msg_equalize( float *gains, BOOL *mute, float preamp, BOOL enabled )
{
  int i;
  float bandgain[20];
  DECODER_PARAMS dec_params = { 0 };

  ASSERT_IS_MAIN_THREAD;
  memcpy( bandgain, gains, sizeof( bandgain ));

  dec_params.bandgain  = bandgain;
  dec_params.equalizer = enabled;

  for( i = 0; i < 20; i++ ) {
    if( mute[i] ) {
      bandgain[i] = 0;
    }
  }

  for( i = 0; i < 20; i++ ) {
    bandgain[i] *= preamp;
  }

  dec_command( DECODER_EQ, &dec_params );
  return TRUE;
}
