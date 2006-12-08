/*
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

#define  INCL_DOS
#define  INCL_WIN
#include <os2.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>

#include "wavplay.h"
#include "decoder_plug.h"
#include "plugin.h"
#include "utilfct.h"
#include "snprintf.h"

static sf_count_t SNDENTRY
vio_fsize( void* x ) {
  return xio_fsize((XFILE*)x );
}

static sf_count_t SNDENTRY
vio_seek( sf_count_t offset, int whence, void* x )
{
  int pos = 0;

  switch( whence )
  {
    case SEEK_SET: pos = offset; break;
    case SEEK_CUR: pos = xio_ftell((XFILE*)x) + offset; break;
    case SEEK_END: pos = xio_fsize((XFILE*)x) + offset; break;
    default:
      return -1;
  }

  if( xio_fseek((XFILE*)x, pos, XIO_SEEK_SET ) == 0 ) {
    return pos;
  } else {
    return -1;
  }
}

static sf_count_t SNDENTRY
vio_read( void* ptr, sf_count_t count, void* x ) {
  return xio_fread( ptr, 1, count, (XFILE*)x );
}

static sf_count_t SNDENTRY
vio_write( const void* ptr, sf_count_t count, void* x ) {
  return xio_fwrite((void*)ptr, 1, count, (XFILE*)x );
}

static sf_count_t SNDENTRY
vio_tell( void *x ) {
  return xio_ftell((XFILE*)x );
}

static ULONG
snd_open( DECODER_STRUCT* w, int mode )
{
  SF_VIRTUAL_IO vio;

  if(( w->file = xio_fopen( w->filename, mode == SFM_READ ? "rb" : "r+b" )) == NULL ) {
    w->sndfile = NULL;
    return xio_errno();
  }

  vio.read        = vio_read;
  vio.write       = vio_write;
  vio.get_filelen = vio_fsize;
  vio.seek        = vio_seek;
  vio.tell        = vio_tell;

  if(( w->sndfile = sf_open_virtual( &vio, mode, &w->sfinfo, w->file )) == NULL ) {
    xio_fclose( w->file );
    w->file = NULL;
    return 200;
  }

  return 0;
}

static ULONG
snd_close( DECODER_STRUCT* w )
{
  if( w->sndfile ) {
    sf_close( w->sndfile );
    w->sndfile = NULL;
  }
  if( w->file ) {
    xio_fclose( w->file );
    w->file = NULL;
  }
  return 0;
}

/* Decoding thread. */
static void TFNENTRY
decoder_thread( void* arg )
{
  ULONG resetcount;
  ULONG markerpos;
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;
  char* buffer = NULL;

  w->frew = FALSE;
  w->ffwd = FALSE;
  w->stop = FALSE;

  if( snd_open( w, SFM_READ ) != 0 ) {
    if( w->error_display )
    {
      char errorbuf[1024];

      strlcpy( errorbuf, "Unable open file:\n", sizeof( errorbuf ));
      strlcat( errorbuf, w->filename, sizeof( errorbuf ));
      strlcat( errorbuf, "\n", sizeof( errorbuf ));
      strlcat( errorbuf, xio_strerror( xio_errno()), sizeof( errorbuf ));

      w->error_display( errorbuf );
    }
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  if(( w->sfinfo.format & SF_FORMAT_SUBMASK ) == SF_FORMAT_FLOAT ||
     ( w->sfinfo.format & SF_FORMAT_SUBMASK ) == SF_FORMAT_DOUBLE )
  {
    float scale = 1.0;
    sf_command( w->sndfile, SFC_SET_SCALE_FLOAT_INT_READ, &scale, SF_TRUE );
  }

  // After opening a new file we so are in its beginning.
  if( w->jumptopos == 0 ) {
      w->jumptopos = -1;
  }

  if(!( buffer = (unsigned char*)malloc( w->audio_buffersize ))) {
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  w->output_format.size       = sizeof( w->output_format );
  w->output_format.format     = WAVE_FORMAT_PCM;
  w->output_format.bits       = 16;
  w->output_format.channels   = w->sfinfo.channels;
  w->output_format.samplerate = w->sfinfo.samplerate;

  for(;;)
  {
    DosWaitEventSem ( w->play, SEM_INDEFINITE_WAIT );
    DosResetEventSem( w->play, &resetcount );

    if( w->stop ) {
      break;
    }

    w->status = DECODER_PLAYING;

    while( !w->stop )
    {
      int read;
      int write;

      if( w->jumptopos >= 0 )
      {
        sf_count_t jumptoframe = (float)w->jumptopos / decoder_length( w ) * w->sfinfo.frames;
        sf_seek( w->sndfile, jumptoframe, SEEK_SET );
        w->jumptopos = -1;
        WinPostMsg( w->hwnd, WM_SEEKSTOP, 0, 0 );
      }

      if( w->frew ) {
        /* ie.: ~1/2 second */
        int frame = sf_seek( w->sndfile, 0, SEEK_CUR ) - w->sfinfo.samplerate / 2;
        if( frame < 0 ) {
          break;
        } else {
          sf_seek( w->sndfile, frame, SEEK_SET );
        }
      }

      if( w->ffwd ) {
        int frame = sf_seek( w->sndfile, 0, SEEK_CUR ) + w->sfinfo.samplerate / 2;
        sf_seek( w->sndfile, frame, SEEK_SET );
      }

      markerpos = 1000.0 * sf_seek( w->sndfile, 0, SEEK_CUR ) / w->sfinfo.samplerate;
      read = sf_read_short( w->sndfile, (short*)buffer, w->audio_buffersize / sizeof( short ));

      if( read <= 0 ) {
        break;
      }

      read *= sizeof( short );
      write = w->output_play_samples( w->a, &w->output_format, buffer, read, markerpos );

      if( write != read ) {
        WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
        break;
      }
    }
    WinPostMsg( w->hwnd, WM_PLAYSTOP, 0, 0 );
    w->status = DECODER_STOPPED;
  }

end:

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
  snd_close( w );
  free( buffer );

  w->decodertid = -1;
  w->status = DECODER_STOPPED;

  DosPostEventSem   ( w->ok    );
  DosReleaseMutexSem( w->mutex );
  _endthread();
}

/* Init function is called when PM123 needs the specified decoder to play
   the stream demanded by the user. */
int PM123_ENTRY
decoder_init( void** returnw )
{
  DECODER_STRUCT* w = calloc( sizeof(*w), 1 );
  *returnw = w;

  DosCreateEventSem( NULL, &w->play,  0, FALSE );
  DosCreateEventSem( NULL, &w->ok,    0, FALSE );
  DosCreateMutexSem( NULL, &w->mutex, 0, FALSE );

  w->decodertid = -1;
  w->status = DECODER_STOPPED;
  return 0;
}

/* Uninit function is called when another decoder than yours is needed. */
BOOL PM123_ENTRY
decoder_uninit( void* arg )
{
  DECODER_STRUCT* w = arg;

  decoder_command ( w, DECODER_STOP, NULL );
  DosCloseEventSem( w->play  );
  DosCloseEventSem( w->ok    );
  DosCloseMutexSem( w->mutex );
  free( w );
  return TRUE;
}

ULONG PM123_ENTRY
decoder_command( void* arg, ULONG msg, DECODER_PARAMS* info )
{
  DECODER_STRUCT* w = arg;
  ULONG resetcount;

  switch(msg)
  {
    case DECODER_SETUP:
      w->output_play_samples = info->output_play_samples;
      w->hwnd                = info->hwnd;
      w->error_display       = info->error_display;
      w->info_display        = info->info_display;
      w->a                   = info->a;
      w->audio_buffersize    = info->audio_buffersize;
      break;

    case DECODER_PLAY:
    {
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

      if( w->decodertid != -1 ) {
        DosReleaseMutexSem( w->mutex );
        decoder_command( w, DECODER_STOP, NULL );
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
      }

      strlcpy( w->filename, info->filename, sizeof( w->filename ));

      w->jumptopos  = info->jumpto;
      w->status     = DECODER_STARTING;
      w->decodertid = _beginthread( decoder_thread, 0, 65535, (void*)w );

      DosReleaseMutexSem( w->mutex );
      DosPostEventSem   ( w->play  );
      break;
    }

    case DECODER_STOP:
    {
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

      if( w->decodertid == -1 ) {
        DosReleaseMutexSem( w->mutex );
        break;
      }

      w->stop = TRUE;

      xio_fabort( w->file );
      DosResetEventSem  ( w->ok, &resetcount );
      DosReleaseMutexSem( w->mutex );
      DosPostEventSem   ( w->play  );
      DosWaitEventSem   ( w->ok, SEM_INDEFINITE_WAIT );
      break;
    }

    case DECODER_FFWD:
      if( info->ffwd ) {
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        if( w->decodertid == -1 || xio_can_seek( w->file ) != XIO_CAN_SEEK_FAST ) {
          DosReleaseMutexSem( w->mutex );
          return 1;
        }
        DosReleaseMutexSem( w->mutex );
      }
      w->ffwd = info->ffwd;
      break;

    case DECODER_REW:
      if( info->rew ) {
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        if( w->decodertid == -1 || xio_can_seek( w->file ) != XIO_CAN_SEEK_FAST ) {
          DosReleaseMutexSem( w->mutex );
          return 1;
        }
        DosReleaseMutexSem( w->mutex );
      }
      w->frew = info->rew;
      break;

    case DECODER_JUMPTO:
      w->jumptopos = info->jumpto;
      if( w->status == DECODER_STOPPED ) {
        DosPostEventSem( w->play );
      }
      break;

    default:
      return 1;
   }

   return 0;
}

/* Returns current status of the decoder. */
ULONG PM123_ENTRY
decoder_status( void* arg ) {
  return ((DECODER_STRUCT*)arg)->status;
}

/* Returns number of milliseconds the stream lasts. */
ULONG PM123_ENTRY
decoder_length( void* arg )
{
  DECODER_STRUCT* w = arg;

  if( w->sfinfo.samplerate ) {
    return 1000.0 * w->sfinfo.frames / w->sfinfo.samplerate;
  }
  return 0;
}

static void
copy_string( SNDFILE* file, char* target, int type, int size )
{
  const char* string;

  if(( string = sf_get_string( file, type )) != NULL ) {
    strlcpy( target, string, size );
  }
}

ULONG PM123_ENTRY
decoder_fileinfo( char* filename, DECODER_INFO* info )
{
  DECODER_STRUCT w = { 0 };
  SF_FORMAT_INFO format_info;
  SF_FORMAT_INFO format_more;
  ULONG rc;

  memset( info, 0, sizeof( *info ));
  info->size = sizeof( *info );

  strlcpy( w.filename, filename, sizeof( w.filename ));
  if(( rc = snd_open( &w, SFM_READ )) == 0 )
  {
    info->format.size       = sizeof( info->format );
    info->format.format     = WAVE_FORMAT_PCM;
    info->format.bits       = 16;
    info->format.channels   = w.sfinfo.channels;
    info->format.samplerate = w.sfinfo.samplerate;
    info->mode              = info->format.channels == 1 ? 3 : 0;
    info->songlength        = 1000.0 * w.sfinfo.frames / w.sfinfo.samplerate;
    info->filesize          = xio_fsize( w.file );

    if( w.sfinfo.frames ) {
      info->bitrate = 8.0 * info->filesize * w.sfinfo.samplerate / w.sfinfo.frames / 1000;
    }

    format_info.format = w.sfinfo.format & SF_FORMAT_TYPEMASK;
    sf_command( w.sndfile, SFC_GET_FORMAT_INFO, &format_info, sizeof( format_info ));
    format_more.format = w.sfinfo.format & SF_FORMAT_SUBMASK;
    sf_command( w.sndfile, SFC_GET_FORMAT_INFO, &format_more, sizeof( format_more ));

    snprintf( info->tech_info, sizeof( info->tech_info ), "%s, %s, %.1f kHz, %s",
              format_info.name, format_more.name, (float)info->format.samplerate / 1000,
              info->format.channels == 1 ? "Mono" : "Stereo" );

    copy_string( w.sndfile, info->title,     SF_STR_TITLE,     sizeof( info->title     ));
    copy_string( w.sndfile, info->artist,    SF_STR_ARTIST,    sizeof( info->artist    ));
    copy_string( w.sndfile, info->year,      SF_STR_DATE,      sizeof( info->year      ));
    copy_string( w.sndfile, info->copyright, SF_STR_COPYRIGHT, sizeof( info->copyright ));
    copy_string( w.sndfile, info->comment,   SF_STR_COMMENT,   sizeof( info->comment   ));

    info->saveinfo = FALSE;
    info->haveinfo = DECODER_HAVE_TITLE    |
                     DECODER_HAVE_ARTIST   |
                     DECODER_HAVE_YEAR     |
                     DECODER_HAVE_COMMENT  |
                     DECODER_HAVE_COPYRIGHT;
    snd_close( &w );
  }

  return rc;
}

ULONG PM123_ENTRY
decoder_trackinfo( char* drive, int track, DECODER_INFO* info ) {
  return 200;
}

ULONG PM123_ENTRY
decoder_cdinfo( char* drive, DECODER_CDINFO* info ) {
  return 100;
}

ULONG PM123_ENTRY
decoder_support( char* ext[], int* size )
{
  if( size ) {
    if( ext != NULL && *size >= 10 )
    {
      strcpy( ext[0], "*.WAV" );
      strcpy( ext[1], "*.AIF" );
      strcpy( ext[2], "*.AU"  );
      strcpy( ext[3], "*.SND" );
      strcpy( ext[4], "*.PAF" );
      strcpy( ext[5], "*.IFF" );
      strcpy( ext[6], "*.VOC" );
      strcpy( ext[7], "*.W64" );
      strcpy( ext[8], "*.PVF" );
      strcpy( ext[9], "*.CAF" );
    }
    *size = 10;
  }

  return DECODER_FILENAME | DECODER_URL;
}

/* Returns information about plug-in. */
int PM123_ENTRY
plugin_query( PLUGIN_QUERYPARAM* param )
{
  param->type         = PLUGIN_DECODER;
  param->author       = "Dmitry A.Steklenev";
  param->desc         = "WAV Play 2.00";
  param->configurable = FALSE;
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

