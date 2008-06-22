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

#define  INCL_OS2MM
#define  INCL_PM
#define  INCL_DOS
#include <os2.h>
#include <os2me.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <utilfct.h>
#include <format.h>
#include <output_plug.h>
#include <decoder_plug.h>
#include <plugin.h>
#include <debuglog.h>

#include "os2audio.h"
#define   INFO( mb ) ((BUFFERINFO*)(mb)->ulUserParm)

#if 0
#define  DosRequestMutexSem( mutex, wait )                                                 \
         DEBUGLOG(( "os2audio: request mutex %08X at line %04d\n", mutex, __LINE__ ));     \
         DEBUGLOG(( "os2audio: request mutex %08X at line %04d is completed, rc = %08X\n", \
                               mutex, __LINE__, DosRequestMutexSem( mutex, wait )))

#define  DosReleaseMutexSem( mutex )                                                       \
         DEBUGLOG(( "os2audio: release mutex %08X at line %04d\n", mutex, __LINE__ ));     \
         DosReleaseMutexSem( mutex )
#endif

#define RG_PREFER_ALBUM                   0
#define RG_PREFER_ALBUM_AND_NOT_CLIPPING  1
#define RG_PREFER_TRACK                   2
#define RG_PREFER_TRACK_AND_NOT_CLIPPING  3

static int   device       = 0;
static int   numbuffers   = 32;
static int   lockdevice   = 0;
static int   kludge48as44 = 0;
static int   force8bit    = 0;
static int   enable_rg    = 0;
static int   rg_type      = RG_PREFER_ALBUM;
static float preamp_rg    = 3;
static float preamp       = 0;
static int   configured   = 0; /* Increased on 1 at each change of a configuration. */

static void
output_error( OS2AUDIO* a, ULONG ulError )
{
  unsigned char message[1536];
  unsigned char buffer [1024];

  if( mciGetErrorString( ulError, buffer, sizeof( buffer )) == MCIERR_SUCCESS ) {
    sprintf( message, "MCI Error %lu: %s\n", ulError, buffer );
  } else {
    sprintf( message, "MCI Error %lu: Cannot query error message.\n", ulError );
  }

  a->original_info.error_display( message );
  WinPostMsg( a->original_info.hwnd, WM_PLAYERROR, 0, 0 );
}

/* Returns the current position of the audio device in milliseconds. */
static APIRET
output_position( OS2AUDIO* a, ULONG* position )
{
  MCI_STATUS_PARMS mstatp = { 0 };
  ULONG rc;

  mstatp.ulItem = MCI_STATUS_POSITION;
  rc = LOUSHORT(mciSendCommand( a->mci_device_id, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, &mstatp, 0 ));

  if( rc == MCIERR_SUCCESS ) {
    *position = mstatp.ulReturn;
  }

  return rc;
}

/* Boosts a priority of the driver thread. */
static void
output_boost_priority( OS2AUDIO* a )
{
  if( a->drivethread && !a->boosted ) {
    DEBUGLOG(( "os2audio: boosts priority of the driver thread %d.\n", a->drivethread ));
    DosSetPriority( PRTYS_THREAD, a->original_info.boostclass,
                                  a->original_info.boostdelta, a->drivethread );
    a->boosted = TRUE;
  }
}

/* Normalizes a priority of the driver thread. */
static void
output_normal_priority( OS2AUDIO* a )
{
  if( a->drivethread && a->boosted ) {
    DEBUGLOG(( "os2audio: normalizes priority of the driver thread %d.\n", a->drivethread ));
    DosSetPriority( PRTYS_THREAD, a->original_info.normalclass,
                                  a->original_info.normaldelta, a->drivethread );
    a->boosted = FALSE;
  }
}

/* When the mixer has finished writing a buffer it will
   call this function. */
static LONG APIENTRY
dart_event( ULONG status, MCI_MIX_BUFFER* buffer, ULONG flags )
{
  DEBUGLOG(( "os2audio: receive DART event, status=%d, flags=%08X\n", status, flags ));

  if( flags & MIX_WRITE_COMPLETE )
  {
    OS2AUDIO* a = INFO(buffer)->a;
    
    //DosRequestMutexSem( a->mutex, SEM_INDEFINITE_WAIT );
    
    // Now the next buffer is playing => update time index, set next buffer.
    // Since the DART thread has very high priority this is implicitely atomic
    // with respect to other PM123 threads.
    a->mci_time    = buffer->ulTime;
    a->mci_is_play = INFO(buffer)->next;

    // If we're about to be short of decoder's data
    // (2nd ahead buffer not filled), let's boost its priority!
    if( a->mci_is_play == a->mci_to_fill || INFO(a->mci_is_play)->next == a->mci_to_fill ) {
      if( !a->nomoredata ) {
        output_boost_priority(a);
      }
    } else
    // If we're out of the water (5th ahead buffer filled),
    // let's reduce the driver thread priority.
    if( a->mci_to_fill == INFO( INFO( INFO( INFO( INFO(a->mci_is_play)->next )->next )->next )->next )->next ) {
      output_normal_priority( a );
    }

    // Just too bad, the decoder fell behind...
    if( a->mci_is_play == a->mci_to_fill )
    { // Implicitely atomic with the above condition unless we do the DEBUGLOG.
      a->mci_to_fill = INFO(a->mci_is_play)->next;
      a->nomoredata  = TRUE;
      DEBUGLOG(( "os2audio: output is hungry.\n" ));
      WinPostMsg( a->original_info.hwnd, WM_OUTPUT_OUTOFDATA, 0, 0 );
    } else {
      a->playingpos = INFO(a->mci_is_play)->playingpos;
    }

    // Clear the played buffer and to place it to the end of the queue.
    // By the moment of playing of this buffer, it already must contain a new data.
    memset( buffer->pBuffer, a->zero, a->mci_buf_parms.ulBufferSize );
    a->mci_mix.pmixWrite( a->mci_mix.ulMixHandle, buffer, 1 );

    //DosReleaseMutexSem( a->mutex );
    DosPostEventSem( a->dataplayed );
  }
  return TRUE;
}

/* Changes the volume of an output device. */
static ULONG
output_set_volume( void* A, unsigned char setvolume, float setamplifier )
{
  OS2AUDIO* a = (OS2AUDIO*)A;

  // Useful when device is closed and reopened.
  a->volume    = min( setvolume, 100 );
  a->amplifier = setamplifier;

  if( a->status == DEVICE_OPENED )
  {
    MCI_SET_PARMS msp = { 0 };
    msp.ulAudio = MCI_SET_AUDIO_ALL;
    msp.ulLevel = a->volume;

    mciSendCommand( a->mci_device_id, MCI_SET, MCI_WAIT | MCI_SET_AUDIO | MCI_SET_VOLUME, &msp, 0 );
  }
  return 0;
}

/* Pauses or resumes the playback. */
static ULONG DLLENTRY
output_pause( void* A, BOOL pause )
{
  OS2AUDIO* a = (OS2AUDIO*)A;
  MCI_GENERIC_PARMS mgp = { 0 };
  ULONG rc = 0;

  if( a->status == DEVICE_OPENED )
  {
    rc = LOUSHORT(mciSendCommand( a->mci_device_id, pause ? MCI_PAUSE : MCI_RESUME, MCI_WAIT, &mgp, 0 ));

    if( rc != MCIERR_SUCCESS ) {
      output_error( a, rc );
    }
  }
  return rc;
}

/* Closes the output audio device. */
ULONG DLLENTRY
output_close( void* A )
{
  OS2AUDIO* a = (OS2AUDIO*)A;
  ULONG rc = 0;

  MCI_GENERIC_PARMS mgp = { 0 };

  DEBUGLOG(( "os2audio: closing the output device.\n" ));
  DosRequestMutexSem( a->mutex, SEM_INDEFINITE_WAIT );

  if( a->status != DEVICE_OPENED && a->status != DEVICE_FAILED ) {
    DEBUGLOG(( "os2audio: unable to close a not opened device.\n" ));
    DosReleaseMutexSem( a->mutex );
    return 0;
  }

  a->status = DEVICE_CLOSING;
  DosReleaseMutexSem( a->mutex );

  rc = mciSendCommand( a->mci_device_id, MCI_STOP,  MCI_WAIT, &mgp, 0 );

  if( LOUSHORT(rc) != MCIERR_SUCCESS ) {
    output_error( a, rc );
  }

  if( a->mci_buf_parms.ulNumBuffers ) {
    rc = mciSendCommand( a->mci_device_id, MCI_BUFFER,
                         MCI_WAIT | MCI_DEALLOCATE_MEMORY, &a->mci_buf_parms, 0 );
    if( LOUSHORT(rc) != MCIERR_SUCCESS ) {
      output_error( a, rc );
    }
  }

  rc = mciSendCommand( a->mci_device_id, MCI_MIXSETUP,
                       MCI_WAIT | MCI_MIXSETUP_DEINIT, &a->mci_mix, 0 );

  if( LOUSHORT(rc) != MCIERR_SUCCESS ) {
    output_error( a, rc );
  }

  rc = mciSendCommand( a->mci_device_id, MCI_CLOSE, MCI_WAIT, &mgp, 0 );
  if( LOUSHORT(rc) != MCIERR_SUCCESS ) {
    output_error( a, rc );
  }

  DosRequestMutexSem( a->mutex, SEM_INDEFINITE_WAIT );

  free( a->mci_buff_info );
  free( a->mci_buffers   );

  a->mci_buff_info = NULL;
  a->mci_buffers   = NULL;
  a->mci_is_play   = NULL;
  a->mci_to_fill   = NULL;
  a->mci_device_id = 0;
  a->status        = DEVICE_CLOSED;
  a->nomoredata    = TRUE;

  DEBUGLOG(( "os2audio: output device is successfully closed.\n" ));
  DosReleaseMutexSem( a->mutex );
  return rc;
}

/* Opens the output audio device. */
static ULONG
output_open( OS2AUDIO* a )
{
  OUTPUT_PARAMS* info = &a->original_info;
  ULONG  openflags = 0;
  ULONG  i;
  ULONG  rc;

  MCI_AMP_OPEN_PARMS mci_aop = {0};

  DosRequestMutexSem( a->mutex, SEM_INDEFINITE_WAIT );
  DEBUGLOG(( "os2audio: opening the output device.\n" ));

  if( a->status != DEVICE_CLOSED ) {
    DEBUGLOG(( "os2audio: unable to open a not closed device.\n" ));
    DosReleaseMutexSem( a->mutex );
    return 0;
  }

  a->status        = DEVICE_OPENING;
  a->playingpos    = 0;
  a->device        = device;
  a->lockdevice    = lockdevice;
  a->numbuffers    = numbuffers;
  a->kludge48as44  = kludge48as44;
  a->force8bit     = force8bit;
  a->mci_device_id = 0;
  a->drivethread   = 0;
  a->boosted       = FALSE;
  a->nomoredata    = TRUE;

/* Uh, well, we don't want to mask bugs here ...
  if( info->formatinfo.format     <= 0 ) { info->formatinfo.format     = WAVE_FORMAT_PCM; }
  if( info->formatinfo.samplerate <= 0 ) { info->formatinfo.samplerate = 44100; }
  if( info->formatinfo.channels   <= 0 ) { info->formatinfo.channels   = 2;     }
  if( info->formatinfo.bits       <= 0 ) { info->formatinfo.bits       = 16;    }
*/

  if( info->formatinfo.bits != 8 && !a->force8bit ) {
    a->zero = 0;
  } else {
    a->zero = 128;
  }

  DEBUGLOG(("os2audio:output_open - {%i, %i, %i, %i}\n",
    info->formatinfo.samplerate, info->formatinfo.channels, info->formatinfo.bits, info->formatinfo.format ));
  // There can be the audio device supports other formats, but
  // whether can interpret other plug-ins them correctly?
  if( info->formatinfo.format != WAVE_FORMAT_PCM ||
      info->formatinfo.samplerate <= 0 ||
      ( info->formatinfo.bits != 16 && info->formatinfo.bits != 8 ) ||
      info->formatinfo.channels <= 0 || info->formatinfo.channels > 2 )
  { rc = MCIERR_UNSUPP_FORMAT_MODE;
    goto end;
  }

  memset( &a->mci_mix, 0, sizeof( a->mci_mix ));
  memset( &a->mci_buf_parms, 0, sizeof( a->mci_buf_parms ));

  // Open the mixer device.
  mci_aop.pszDeviceType = (PSZ)MAKEULONG( MCI_DEVTYPE_AUDIO_AMPMIX, a->device );

  if( !a->lockdevice ) {
    openflags = MCI_OPEN_SHAREABLE;
  }

  rc = LOUSHORT(mciSendCommand( 0, MCI_OPEN, MCI_WAIT | MCI_OPEN_TYPE_ID | openflags, &mci_aop, 0 ));
  if( rc != MCIERR_SUCCESS ) {
    DEBUGLOG(( "os2audio:output_open:  unable to open a mixer: %u.\n", rc ));
    goto end;
  }

  a->mci_device_id = mci_aop.usDeviceID;

  // Set the MCI_MIXSETUP_PARMS data structure to match the audio stream.
  if( a->force8bit ) {
    a->mci_mix.ulBitsPerSample = 8;
  } else {
    a->mci_mix.ulBitsPerSample = info->formatinfo.bits;
  }

  if( a->kludge48as44 && info->formatinfo.samplerate == 48000 ) {
    a->mci_mix.ulSamplesPerSec = 44100;
  } else {
    a->mci_mix.ulSamplesPerSec = info->formatinfo.samplerate;
  }

  a->mci_mix.ulChannels   = info->formatinfo.channels;
  a->mci_mix.ulFormatTag  = MCI_WAVE_FORMAT_PCM;

  // Setup the mixer for playback of wave data.
  a->mci_mix.ulFormatMode = MCI_PLAY;
  a->mci_mix.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
  a->mci_mix.pmixEvent    = dart_event;

  rc = LOUSHORT(mciSendCommand( a->mci_device_id, MCI_MIXSETUP, MCI_WAIT | MCI_MIXSETUP_INIT, &a->mci_mix, 0 ));
  if( rc != MCIERR_SUCCESS ) {
    DEBUGLOG(( "os2audio:output_open: MCI_MIXSETUP failed: %u.\n", rc ));
    DEBUGLOG(( "os2audio: unable to setup a mixer.\n" ));
    goto end;
  }

  // Set up the MCI_BUFFER_PARMS data structure and allocate
  // device buffers from the mixer.
  a->numbuffers    = limit2( a->numbuffers, 5, 200 );
  a->mci_buffers   = calloc( a->numbuffers, sizeof( *a->mci_buffers   ));
  a->mci_buff_info = calloc( a->numbuffers, sizeof( *a->mci_buff_info ));

  if( !a->mci_buffers || !a->mci_buff_info ) {
    rc = MCIERR_OUT_OF_MEMORY;
    goto end;
  }

  if( a->force8bit ) {
    a->buffersize = info->buffersize / ( info->formatinfo.bits / 8 );
  } else {
    a->buffersize = info->buffersize;
  }

  a->mci_buf_parms.ulNumBuffers = a->numbuffers;
  a->mci_buf_parms.ulBufferSize = a->buffersize;
  a->mci_buf_parms.pBufList     = a->mci_buffers;

  rc = LOUSHORT(mciSendCommand( a->mci_device_id, MCI_BUFFER,
                                MCI_WAIT | MCI_ALLOCATE_MEMORY, (PVOID)&a->mci_buf_parms, 0 ));
  if( rc != MCIERR_SUCCESS ) {
    DEBUGLOG(( "os2audio:output_open: MCI_BUFFER failed: %u.\n", rc ));
    goto end;
  }

  DEBUGLOG(( "os2audio: suggested buffers is %d, allocated %d\n",
                        a->numbuffers, a->mci_buf_parms.ulNumBuffers ));
  DEBUGLOG(( "os2audio: suggested buffer size is %d, allocated %d\n",
                        a->buffersize, a->mci_buf_parms.ulBufferSize ));

  for( i = 0; i < a->mci_buf_parms.ulNumBuffers; i++ )
  {
    a->mci_buffers[i].ulFlags        = 0;
    a->mci_buffers[i].ulBufferLength = a->mci_buf_parms.ulBufferSize;
    a->mci_buffers[i].ulUserParm     = (ULONG)&a->mci_buff_info[i];

    memset( a->mci_buffers[i].pBuffer, a->zero, a->mci_buf_parms.ulBufferSize );

    a->mci_buff_info[i].a    = a;
    a->mci_buff_info[i].next = &a->mci_buffers[i+1];
  }

  a->mci_buff_info[i-1].next = &a->mci_buffers[0];

  a->mci_is_play = &a->mci_buffers[0];
  a->mci_to_fill = &a->mci_buffers[2];

  // Write buffers to kick off the amp mixer.
  rc = LOUSHORT(a->mci_mix.pmixWrite( a->mci_mix.ulMixHandle, a->mci_is_play, a->mci_buf_parms.ulNumBuffers ));
  if( rc != MCIERR_SUCCESS ) {
    DEBUGLOG(( "os2audio:output_open: pmixWrite failed: %u.\n", rc ));
    goto end;
  }

  // Current time of the device is placed at end of
  // playing the buffer, but we require this already now.
  output_position( a, &a->mci_is_play->ulTime );

end:

  if( rc != MCIERR_SUCCESS )
  {
    output_error( a, rc );
    DEBUGLOG(( "os2audio: output device opening is failed: %u.\n", rc ));

    if( a->mci_device_id ) {
      a->status = DEVICE_FAILED;
      output_close( a );
    } else {
      a->status = DEVICE_CLOSED;
    }
  } else {
    a->status = DEVICE_OPENED;
    output_set_volume( a, a->volume, a->amplifier );
    DEBUGLOG(( "os2audio: output device is successfully opened.\n" ));
  }

  DosReleaseMutexSem( a->mutex );
  return rc;
}

/* Calculates scale factor according to current replay gain values. */
static void
recalc_replay_gain( OS2AUDIO* a )
{
  float gain      = 0;
  float peak      = 0;
  float max_scale = 0;

  if( enable_rg )
  {
    if(( rg_type == RG_PREFER_ALBUM ||
         rg_type == RG_PREFER_ALBUM_AND_NOT_CLIPPING ||
         a->track_gain == 0 ) && a->album_gain != 0 )
    {
      gain = a->album_gain;
    } else {
      gain = a->track_gain;
    }

    if(( rg_type == RG_PREFER_ALBUM ||
         rg_type == RG_PREFER_ALBUM_AND_NOT_CLIPPING ||
         a->track_peak == 0 ) && a->album_peak != 0 )
    {
      peak = a->album_peak;
    } else {
      peak = a->track_peak;
    }

    if( peak ) {
      // If a peak value is above 1.0, this is already
      // clipped by decoder and we must use 1.0 instead.
      max_scale = 1.0 / min( peak, 1.0 );
    }

    if( gain != 0 ) {
      gain += preamp_rg;
    } else {
      gain += preamp;
    }

    a->scale = pow( 10, gain / 20 );

    if(( rg_type == RG_PREFER_ALBUM_AND_NOT_CLIPPING ||
         rg_type == RG_PREFER_TRACK_AND_NOT_CLIPPING ) && max_scale )
    {
      a->scale = min( a->scale, max_scale );
    }
  } else {
    a->scale = 1;
  }

  DEBUGLOG(( "os2audio: replay gain is %.2f dB, scale is %.8f (max is %.8f)\n",
                                                      gain, a->scale, max_scale ));
}

/* Scales signed 16-bit samples according to current replay gain values. */
static void
scale_16_replay_gain( OS2AUDIO* a, short* buf, int count )
{
  int i, sample;

  if( enable_rg && a->scale != 1 ) {
    for( i = 0; i < count; i++ ) {
      sample = buf[i] * a->scale;

      if( sample > 32767 ) {
        buf[i] =  32767;
      } else if( sample < -32768 ) {
        buf[i] = -32768;
      } else {
        buf[i] = sample;
      }
    }
  }
}

/* Scales unsigned 8-bit samples according to current replay gain values. */
static void
scale_08_replay_gain( OS2AUDIO* a, unsigned char* buf, int count )
{
  int i, sample;

  if( enable_rg && a->scale != 1 ) {
    for( i = 0; i < count; i++ ) {
      sample = ( buf[i] - 128 ) * a->scale + 128;

      if( sample > 255 ) {
        buf[i] = 255;
      } else if( sample < 0 ) {
        buf[i] = 0;
      } else {
        buf[i] = sample;
      }
    }
  }
}

/* This function is called by the decoder or last in chain filter plug-in
   to play samples. */
int DLLENTRY
output_play_samples( void* A, FORMAT_INFO* format, char* buf, int len, int posmarker )
{
  OS2AUDIO* a = (OS2AUDIO*)A;
  ULONG resetcount;

  DEBUGLOG(("output_play_samples({%i,%i,%i,%i,%x}, %p, %i, %i)\n",
      format->size, format->samplerate, format->channels, format->bits, format->format, buf, len, posmarker));

  // Update TID always because the decoder thread may change while playing gapless.
  { PTIB ptib;
    DosGetInfoBlocks( &ptib, NULL );
    if (a->drivethread != ptib->tib_ptib2->tib2_ultid)
    { a->drivethread = ptib->tib_ptib2->tib2_ultid;
      // If we were already boosted we also have to boost the new thread
      if (a->boosted)
        DosSetPriority( PRTYS_THREAD, a->original_info.boostclass,
                                      a->original_info.boostdelta, a->drivethread );
    }
  }

  if( a->configured != configured ) {
    recalc_replay_gain( a );
    a->configured = configured;
  }

  // Set the new format structure before re-opening.
  if( memcmp( format, &a->original_info.formatinfo, sizeof( FORMAT_INFO )) != 0 || a->status == DEVICE_CLOSED )
  {
    DEBUGLOG(( "os2audio: old format: size %d, sample: %d, channels: %d, bits: %d, id: %d\n",
        a->original_info.formatinfo.size,
        a->original_info.formatinfo.samplerate,
        a->original_info.formatinfo.channels,
        a->original_info.formatinfo.bits,
        a->original_info.formatinfo.format ));

    DEBUGLOG(( "os2audio: new format: size %d, sample: %d, channels: %d, bits: %d, id: %d\n",
        format->size,
        format->samplerate,
        format->channels,
        format->bits,
        format->format ));

    if( output_close(a) != 0 ) {
      return 0;
    }
    a->original_info.formatinfo = *format;
    if( output_open (a) != 0 ) {
      return 0;
    }
  }

  if( a->status != DEVICE_OPENED ) {
    return 0;
  }

  // If we're too quick, let's wait.
  while( a->mci_to_fill == a->mci_is_play ) {
    DosWaitEventSem ( a->dataplayed, SEM_INDEFINITE_WAIT );
    DosResetEventSem( a->dataplayed, &resetcount );

    if( a->trashed ) {
      // This portion of samples should be trashed.
      a->trashed = FALSE;
      return len;
    }
  }

  // A following buffer is already cashed by the audio device. Therefore
  // it is necessary to skip it and to fill the second buffer from current.
  DosEnterCritSec();
  if( a->mci_to_fill == INFO(a->mci_is_play)->next ) {
    INFO(a->mci_to_fill)->playingpos = a->playingpos;
    a->mci_to_fill = INFO(a->mci_to_fill)->next;
  }
  DosExitCritSec();

  if( len > 0 ) {
    if( a->force8bit && a->original_info.formatinfo.bits == 16 )
    {
      signed short*  in  = (signed short*)buf;
      unsigned char* out = a->mci_to_fill->pBuffer;
      int i;

      if( len / 2 > a->mci_buf_parms.ulBufferSize ) {
        DEBUGLOG(( "os2audio: too many samples.\n" ));
        return 0;
      }

      for( i = 0; i < len / 2; i++ ) {
        out[i] = ( in[i] >> 8 ) + 128;
      }
      scale_08_replay_gain( a, a->mci_to_fill->pBuffer, len / 2 );
    } else {
      if( len > a->mci_buf_parms.ulBufferSize ) {
        DEBUGLOG(( "os2audio: too many samples.\n" ));
        return 0;
      }

      memcpy( a->mci_to_fill->pBuffer, buf, len );

      if( a->original_info.formatinfo.bits == 8 ) {
        scale_08_replay_gain( a, a->mci_to_fill->pBuffer, len );
      } else if( a->original_info.formatinfo.bits == 16 ) {
        scale_16_replay_gain( a, (short*)a->mci_to_fill->pBuffer, len / 2 );
      }
    }

    DosEnterCritSec();
    INFO(a->mci_to_fill)->playingpos = posmarker;

    a->mci_to_fill = INFO(a->mci_to_fill)->next;
    a->nomoredata  = FALSE;
    a->trashed     = FALSE;
    DosExitCritSec();
  }

  return len;
}

/* Returns the posmarker from the buffer that the user
   currently hears. */
ULONG DLLENTRY
output_playing_pos( void* A ) {
  OS2AUDIO* a = (OS2AUDIO*)A;
  ULONG     position, playingpos;
  
  DosRequestMutexSem( a->mutex, SEM_INDEFINITE_WAIT );

  if( !a->mci_is_play || a->status == DEVICE_CLOSED ) {
    DosReleaseMutexSem( a->mutex );
    return PLUGIN_FAILED;
  }

  DosEnterCritSec();
  // This difference may overflow, but this is compensated below.
  playingpos = a->playingpos - a->mci_time;
  DosExitCritSec();

  if( LOUSHORT( output_position( a, &position )) != MCIERR_SUCCESS ) {
    DosReleaseMutexSem( a->mutex );
    return PLUGIN_FAILED;
  }

  DosReleaseMutexSem( a->mutex );

  return playingpos + position;
}

/* This function is used by visual plug-ins so the user can visualize
   what is currently being played. */
ULONG DLLENTRY
output_playing_samples( void* A, FORMAT_INFO* info, char* buf, int len )
{
  OS2AUDIO* a = (OS2AUDIO*)A;

  DosRequestMutexSem( a->mutex, SEM_INDEFINITE_WAIT );

  if( !a->mci_is_play || a->status == DEVICE_CLOSED ) {
    DosReleaseMutexSem( a->mutex );
    return PLUGIN_FAILED;
  }

  info->bits       = a->mci_mix.ulBitsPerSample;
  info->samplerate = a->mci_mix.ulSamplesPerSec;
  info->channels   = a->mci_mix.ulChannels;
  info->format     = a->mci_mix.ulFormatTag;

  if( buf && len )
  {
    MCI_MIX_BUFFER* mci_buffer;
    ULONG position = 0;
    LONG  offsetof = 0;

    DosEnterCritSec();
    mci_buffer = a->mci_is_play;
    offsetof   = a->mci_time;
    DosExitCritSec();
    
    if( LOUSHORT( output_position( a, &position )) != MCIERR_SUCCESS ) {
      DosReleaseMutexSem( a->mutex );
      return PLUGIN_FAILED;
    }

    offsetof = ( position - offsetof ) * a->mci_mix.ulSamplesPerSec / 1000 *
                                         a->mci_mix.ulChannels *
                                         a->mci_mix.ulBitsPerSample / 8;

    while( INFO(mci_buffer)->next != a->mci_to_fill && offsetof > mci_buffer->ulBufferLength ) {
      offsetof  -= mci_buffer->ulBufferLength;
      mci_buffer = INFO(mci_buffer)->next;
    }

    while( len && INFO(mci_buffer)->next != a->mci_to_fill )
    {
      int chunk_size = mci_buffer->ulBufferLength - offsetof;

      if( chunk_size > len ) {
          chunk_size = len;
      }

      memcpy( buf, (char*)mci_buffer->pBuffer + offsetof, chunk_size );

      buf += chunk_size;
      len -= chunk_size;

      offsetof   = 0;
      mci_buffer = INFO(mci_buffer)->next;
    }

    if( len ) {
      memset( buf, a->zero, len );
    }
  }

  DosReleaseMutexSem( a->mutex );
  return PLUGIN_OK;
}

/* Trashes all audio data received till this time. */
void DLLENTRY
output_trash_buffers( void* A, ULONG temp_playingpos )
{
  OS2AUDIO* a = (OS2AUDIO*)A;
  int i;

  DEBUGLOG(( "os2audio: trashing audio buffers.\n" ));
  DosRequestMutexSem( a->mutex, SEM_INDEFINITE_WAIT );

  if(!( a->status & DEVICE_OPENED )) {
    DosReleaseMutexSem( a->mutex );
    return;
  }

  for( i = 0; i < a->mci_buf_parms.ulNumBuffers; i++ ) {
    memset( a->mci_buffers[i].pBuffer, a->zero, a->mci_buf_parms.ulBufferSize );
  }

  a->playingpos = temp_playingpos;
  a->nomoredata = FALSE;
  a->trashed    = TRUE;

  // A following buffer is already cashed by the audio device. Therefore
  // it is necessary to skip it and to fill the second buffer from current.
  DosEnterCritSec();
  a->mci_to_fill = INFO(a->mci_is_play)->next;
  INFO(a->mci_to_fill)->playingpos = temp_playingpos;
  a->mci_to_fill = INFO(a->mci_to_fill)->next;
  DosExitCritSec();

  DosReleaseMutexSem( a->mutex );
  DEBUGLOG(( "os2audio: audio buffers is successfully trashed.\n" ));
}

/* This function is called when the user requests
   the use of output plug-in. */
ULONG DLLENTRY
output_init( void** A )
{
  OS2AUDIO* a;

 *A = calloc( sizeof( OS2AUDIO ), 1 );
  a = (OS2AUDIO*)*A;

  a->numbuffers = 32;
  a->volume     = 100;
  a->amplifier  = 1.0;
  a->status     = DEVICE_CLOSED;
  a->nomoredata = TRUE;
  a->scale      = 1.0;

  DosCreateEventSem( NULL, &a->dataplayed, 0, FALSE );
  DosCreateMutexSem( NULL, &a->mutex, 0, FALSE );
  return PLUGIN_OK;
}

/* This function is called when another output plug-in
   is request by the user. */
ULONG DLLENTRY
output_uninit( void* A )
{
  OS2AUDIO* a = (OS2AUDIO*)A;
  DEBUGLOG(("output_uninit: %p\n", a));

  output_close( a );

  DosCloseEventSem( a->dataplayed );
  DosCloseMutexSem( a->mutex );

  free( a );
  return PLUGIN_OK;
}

/* Returns TRUE if the output plug-in still has some buffers to play. */
BOOL DLLENTRY
output_playing_data( void* A ) {
  return !((OS2AUDIO*)A)->nomoredata;
}

static ULONG DLLENTRY
output_get_devices( char* name, int deviceid )
{
  char buffer[256];
  MCI_SYSINFO_PARMS mip;

  if( deviceid && name )
  {
    MCI_SYSINFO_LOGDEVICE mid;

    mip.pszReturn    = buffer;
    mip.ulRetSize    = sizeof( buffer );
    mip.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;
    mip.ulNumber     = deviceid;

    mciSendCommand( 0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_INSTALLNAME, &mip, 0 );

    mip.ulItem = MCI_SYSINFO_QUERY_DRIVER;
    mip.pSysInfoParm = &mid;
    strcpy( mid.szInstallName, buffer );

    mciSendCommand( 0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_ITEM, &mip, 0 );
    sprintf( name, "%s (%s)", mid.szProductInfo, mid.szVersionNumber );
  }

  mip.pszReturn    = buffer;
  mip.ulRetSize    = sizeof(buffer);
  mip.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;

  mciSendCommand( 0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_QUANTITY, &mip, 0 );
  return atoi( mip.pszReturn );
}

ULONG DLLENTRY
output_command( void* A, ULONG msg, OUTPUT_PARAMS* params )
{
  OS2AUDIO* a = (OS2AUDIO*)A;
  DEBUGLOG(("output_command(%i, %p)\n", msg, params));

  switch( msg )
  {
    case OUTPUT_OPEN:
      return output_open( a );

    case OUTPUT_PAUSE:
      return output_pause( a, params->pause );

    case OUTPUT_CLOSE:
      return output_close( a );

    case OUTPUT_VOLUME:
      return output_set_volume( a, params->volume, params->amplifier );

    case OUTPUT_SETUP:
      if( a->status != DEVICE_OPENED ) {
        // Otherwise, information on the current session are modified.
        a->original_info = *params;
        a->configured = configured;

        if( params->size >= OUTPUT_SIZE_2 && params->info->size >= INFO_SIZE_2 )
        {
          a->album_gain = params->info->album_gain;
          a->album_peak = params->info->album_peak;
          a->track_gain = params->info->track_gain;
          a->track_peak = params->info->track_peak;

          recalc_replay_gain( a );
        }
      }
      params->always_hungry = FALSE;
      return 0;

    case OUTPUT_TRASH_BUFFERS:
      output_trash_buffers( a, params->temp_playingpos );
      return 0;
  }

  return MCIERR_UNSUPPORTED_FUNCTION;
}

/********** GUI stuff ******************************************************/
static void
save_ini( void )
{
  HINI hini;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value( hini, device );
    save_ini_value( hini, lockdevice );
    save_ini_value( hini, numbuffers );
    save_ini_value( hini, force8bit );
    save_ini_value( hini, kludge48as44 );
    save_ini_value( hini, enable_rg );
    save_ini_value( hini, rg_type );
    save_ini_value( hini, preamp_rg );
    save_ini_value( hini, preamp );

    close_ini( hini );
  }
}

static void
load_ini( void )
{
  HINI hini;

  device       = 0;
  lockdevice   = 0;
  numbuffers   = 32;
  force8bit    = 0;
  kludge48as44 = 0;
  enable_rg    = 0;
  rg_type      = RG_PREFER_ALBUM;
  preamp_rg    = 3;
  preamp       = 0;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, device );
    load_ini_value( hini, lockdevice );
    load_ini_value( hini, numbuffers );
    load_ini_value( hini, force8bit );
    load_ini_value( hini, kludge48as44 );
    load_ini_value( hini, enable_rg );
    load_ini_value( hini, rg_type );
    load_ini_value( hini, preamp_rg );
    load_ini_value( hini, preamp );

    close_ini( hini );
  }
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      int i;
      int numdevice = output_get_devices( NULL, 0 );

      lb_add_item( hwnd, CB_DEVICE, "Default" );

      for( i = 1; i <= numdevice; i++ ) {
        char name[256];
        output_get_devices( name, i );
        lb_add_item( hwnd, CB_DEVICE, name );
      }

      lb_select( hwnd, CB_DEVICE, device );

      WinCheckButton( hwnd, CB_SHARED,   !lockdevice   );
      WinCheckButton( hwnd, CB_8BIT,      force8bit    );
      WinCheckButton( hwnd, CB_48KLUDGE,  kludge48as44 );
      WinCheckButton( hwnd, CB_RG_ENABLE, enable_rg    );

      lb_add_item( hwnd, CB_RG_TYPE, "Prefer album gain" );
      lb_add_item( hwnd, CB_RG_TYPE, "Prefer album gain and prevent clipping" );
      lb_add_item( hwnd, CB_RG_TYPE, "Prefer track gain" );
      lb_add_item( hwnd, CB_RG_TYPE, "Prefer track gain and prevent clipping" );
      lb_select  ( hwnd, CB_RG_TYPE, rg_type );

      WinSendDlgItemMsg( hwnd, SB_BUFFERS, SPBM_SETLIMITS, MPFROMLONG( 200 ), MPFROMLONG( 5 ));
      WinSendDlgItemMsg( hwnd, SB_BUFFERS, SPBM_SETCURRENTVALUE, MPFROMLONG( numbuffers ), 0 );

      WinEnableControl( hwnd, CB_RG_TYPE,   enable_rg );
      WinEnableControl( hwnd, SB_RG_PREAMP, enable_rg );
      WinEnableControl( hwnd, ST_RG_PREAMP, enable_rg );
      WinEnableControl( hwnd, SB_PREAMP,    enable_rg );
      WinEnableControl( hwnd, ST_PREAMP,    enable_rg );
      break;
    }

    case WM_COMMAND:
      if( SHORT1FROMMP( mp1 ) == DID_OK )
      {
        int dB1, dB2;

        device = lb_selected( hwnd, CB_DEVICE,  LIT_FIRST );
        rg_type= lb_selected( hwnd, CB_RG_TYPE, LIT_FIRST );

        lockdevice   = !WinQueryButtonCheckstate( hwnd, CB_SHARED    );
        force8bit    =  WinQueryButtonCheckstate( hwnd, CB_8BIT      );
        kludge48as44 =  WinQueryButtonCheckstate( hwnd, CB_48KLUDGE  );
        enable_rg    =  WinQueryButtonCheckstate( hwnd, CB_RG_ENABLE );

        WinSendDlgItemMsg( hwnd, SB_BUFFERS, SPBM_QUERYVALUE,
                           MPFROMP( &numbuffers ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));
        WinSendDlgItemMsg( hwnd, SB_RG_PREAMP, SPBM_QUERYVALUE,
                           MPFROMP( &dB1 ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));
        WinSendDlgItemMsg( hwnd, SB_PREAMP, SPBM_QUERYVALUE,
                           MPFROMP( &dB2 ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));

        preamp_rg = (float)dB1 / 10 - 12;
        preamp    = (float)dB2 / 10 - 12;

        configured++;
        save_ini();
      }
      break;

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_RG_ENABLE &&
        ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
      {
        BOOL enable = WinQueryButtonCheckstate( hwnd, CB_RG_ENABLE );

        WinEnableControl( hwnd, CB_RG_TYPE,   enable );
        WinEnableControl( hwnd, SB_RG_PREAMP, enable );
        WinEnableControl( hwnd, ST_RG_PREAMP, enable );
        WinEnableControl( hwnd, SB_PREAMP,    enable );
        WinEnableControl( hwnd, ST_PREAMP,    enable );
        return 0;
      }
      break;

  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
int DLLENTRY
plugin_configure( HWND howner, HMODULE module )
{
  HWND  hwnd;
  PSZ   dBList[242];
  char  buffer[64];
  int   i;
  float dB;

  hwnd = WinLoadDlg( HWND_DESKTOP, howner, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  do_warpsans( hwnd );

  if( hwnd ) {
    for( dB = -12.0, i = 0; dB < 12.1; dB += 0.1, i++ ) {
      sprintf( buffer, "%+.1f dB", dB );
      dBList[i] = strdup( buffer );
    }

    WinSendDlgItemMsg( hwnd, SB_RG_PREAMP, SPBM_SETARRAY, MPFROMP( &dBList ), MPFROMLONG( 241 ));
    WinSendDlgItemMsg( hwnd, SB_PREAMP,    SPBM_SETARRAY, MPFROMP( &dBList ), MPFROMLONG( 241 ));

    WinSendDlgItemMsg( hwnd, SB_RG_PREAMP,
                       SPBM_SETCURRENTVALUE, MPFROMLONG( preamp_rg * 10 + 120 ), 0 );
    WinSendDlgItemMsg( hwnd, SB_PREAMP,
                       SPBM_SETCURRENTVALUE, MPFROMLONG( preamp * 10 + 120 ), 0 );

    WinProcessDlg( hwnd );
    WinDestroyWindow( hwnd );

    for( dB = -12.0, i = 0; dB < 12.1; dB += 0.1, i++ ) {
      free( dBList[i] );
    }
  }
  return 0;
}

int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM* param )
{
  param->type         = PLUGIN_OUTPUT;
  param->author       = "Samuel Audet, Dmitry A.Steklenev";
  param->desc         = "DART Output 1.31";
  param->configurable = TRUE;

  load_ini();
  return 0;
}

#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return 1UL;
  } else {
    _dump_allocated( 0 );
    _CRT_term();
    return 1UL;
  }
}
#endif
