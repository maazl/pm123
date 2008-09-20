/*
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
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

#define  INCL_PM
#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>

#include <stdio.h>
#include <malloc.h>
#include <process.h>
#include <errno.h>

#include "mpg123.h"

#include <utilfct.h>
#include <decoder_plug.h>
#include <debuglog.h>
#include <snprintf.h>
#include <eautils.h>
#include <id3v1.h>
#include <id3v2.h>

#include "genre.h"


static DECODER_STRUCT** instances = NULL;
static int  instances_count = 0;
static HMTX mutex;
static int  mmx_enable = 0;

static int   tag_read_type          = TAG_READ_ID3V2_AND_ID3V1;
static ULONG tag_read_id3v2_charset = 819; // ISO8859-1
static ULONG tag_read_id3v1_charset = CH_CP_NONE;
static int   tag_save_type          = TAG_SAVE_ID3V2_AND_ID3V1;
static ULONG tag_save_id3v2_charset = 1208; // UTF-8
static BOOL  tag_save_id3v2_BOM     = FALSE;
static ULONG tag_save_id3v1_charset = CH_CP_NONE;


#define modes(i) ( i == 0 ? "Stereo"         : \
                 ( i == 1 ? "Joint-Stereo"   : \
                 ( i == 2 ? "Dual-Channel"   : \
                 ( i == 3 ? "Single-Channel" : "" ))))


/* Open MPEG file. Returns 0 if it successfully opens the file.
   A nonzero return value indicates an error. A -1 return value
   indicates an unsupported format of the file. */
static int
plg_open_file( DECODER_STRUCT* w, const char* filename, const char* mode )
{
  int rc;
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( w->filename != filename ) {
    strlcpy( w->filename, filename, sizeof( w->filename ));
  }

  if(( rc = mpg_open_file( &w->mpeg, w->filename, mode )) == 0 ) {

    if( w->hwnd ) {
      xio_set_observer( w->mpeg.file, w->hwnd, w->metadata_buff, w->metadata_size );
    }

    w->output_format.samplerate = w->mpeg.samplerate;
    w->bitrate = w->mpeg.bitrate;
  }

  DosReleaseMutexSem( w->mutex );
  return rc;
}

/* Closes the MPEG file. */
static void
plg_close_file( DECODER_STRUCT* w )
{
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
  mpg_close( &w->mpeg );
  DosReleaseMutexSem( w->mutex );
}

/* Changes the current file position to a new time within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
static int
seek_to( DECODER_STRUCT* w, int ms, int origin )
{
  int rc;

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
  rc = mpg_seek_ms( &w->mpeg, ms, origin );
  DosReleaseMutexSem( w->mutex );
  return rc;
}

/* Reads a next valid frame and decodes it. Returns 0 if it
   successfully reads a frame, or -1 if any errors were detected. */
static int
read_and_save_frame( DECODER_STRUCT* w )
{
  char errorbuf[1024];
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( mpg_read_frame( &w->mpeg ) != 0 ) {
    DosReleaseMutexSem( w->mutex );
    return -1;
  }
  if( w->posmarker < 0 ) {
      w->posmarker = mpg_tell_ms( &w->mpeg );
  }

  if( !w->save && *w->savename ) {
    // Opening a save stream.
    if(!( w->save = xio_fopen( w->savename, "ab" )))
    {
      strlcpy( errorbuf, "Could not open file to save data:\n", sizeof( errorbuf ));
      strlcat( errorbuf, w->savename, sizeof( errorbuf ));
      strlcat( errorbuf, "\n", sizeof( errorbuf ));
      strlcat( errorbuf, xio_strerror( xio_errno()), sizeof( errorbuf ));
      w->info_display( errorbuf );
      w->savename[0] = 0;
    }
  } else if( w->save && !*w->savename  ) {
    // Closing a save stream.
    xio_fclose( w->save );
    w->save = NULL;
  }

  if( w->save ) {
    if( mpg_save_frame( &w->mpeg, w->save ) != 0 )
    {
      strlcpy( errorbuf, "Error writing to stream save file:\n", sizeof( errorbuf ));
      strlcat( errorbuf, w->savename, sizeof( errorbuf ));
      strlcat( errorbuf, "\n", sizeof( errorbuf ));
      strlcat( errorbuf, xio_strerror( xio_errno()), sizeof( errorbuf ));

      xio_fclose( w->save );
      w->save = NULL;
      w->savename[0] = 0;
      w->info_display( errorbuf );
    }
  }

  mpg_decode_frame( &w->mpeg );

  if( w->mpeg.fr.bitrate != w->bitrate )
  {
    w->bitrate = w->mpeg.fr.bitrate;
    WinPostMsg( w->hwnd, WM_CHANGEBR, MPFROMLONG( w->bitrate ), 0 );
  }

  DosReleaseMutexSem( w->mutex );
  return 0;
}

static void
flush_output( DECODER_STRUCT* w, unsigned char* buffer, int size )
{
  if( size ) {
    if( w->posmarker < 0 ) {
      w->posmarker = mpg_tell_ms( &w->mpeg );
    }
    if( w->output_play_samples( w->a, &w->output_format, buffer, size, w->posmarker ) != size ) {
      WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    }
    w->posmarker = -1;
  }
}

/* Decoding thread. */
static void TFNENTRY
decoder_thread( void* arg )
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;
  unsigned char*  buffer = NULL;

  int   bufpos =  0;
  int   done;
  ULONG resetcount;
  char  errorbuf[1024];
  int   rc;

  if(( rc = plg_open_file( w, w->filename, "rb" )) != 0 ) {
    if( w->error_display )
    {
      strlcpy( errorbuf, "Unable open file:\n", sizeof( errorbuf ));
      strlcat( errorbuf, w->filename, sizeof( errorbuf ));
      strlcat( errorbuf, "\n", sizeof( errorbuf ));

      if( rc == -1 ) {
        strlcat( errorbuf, "Unsupported format of the file.", sizeof( errorbuf ));
      } else {
        strlcat( errorbuf, xio_strerror( rc ), sizeof( errorbuf ));
      }

      w->error_display( errorbuf );
    }
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  if(!( buffer = malloc( w->audio_buffersize ))) {
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  w->frew = FALSE;
  w->ffwd = FALSE;
  w->stop = FALSE;

  // After opening a new file we so are in its beginning.
  if( w->jumptopos == 0 ) {
      w->jumptopos = -1;
  }

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
      if( w->jumptopos >= 0 ) {
        seek_to( w, w->jumptopos, MPG_SEEK_SET );

        bufpos = 0;
        w->posmarker = -1;
        w->jumptopos = -1;

        WinPostMsg( w->hwnd, WM_SEEKSTOP, 0, 0 );
      }
      if( w->frew ) {
        if( seek_to( w, -250, MPG_SEEK_CUR ) != 0 ) {
          break;
        }
      } else if( w->ffwd ) {
        seek_to( w, 250, MPG_SEEK_CUR );
      }

      if( read_and_save_frame( w ) != 0 ) {
        break;
      }

      while(( done = mpg_move_sound( &w->mpeg, buffer + bufpos, w->audio_buffersize - bufpos )) > 0 )
      {
        bufpos += done;

        if( bufpos == w->audio_buffersize ) {
          flush_output( w, buffer, bufpos );
          bufpos = 0;
        }
      }
    }

    flush_output( w, buffer, bufpos );
    WinPostMsg( w->hwnd, WM_PLAYSTOP, 0, 0 );
    w->status = DECODER_STOPPED;
  }

end:

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
  plg_close_file( w );

  if( w->save ) {
    xio_fclose( w->save );
    w->save = NULL;
  }
  if( buffer ) {
    free( buffer );
  }

  w->status = DECODER_STOPPED;
  w->decodertid = -1;
  DosReleaseMutexSem( w->mutex );
  //_endthread(); // implicit on return
}

/* Init function is called when PM123 needs the specified decoder to play
   the stream demanded by the user. */
int DLLENTRY
decoder_init( void** returnw )
{
  DECODER_STRUCT* w = calloc( sizeof(*w), 1 );
  *returnw = w;

  DosCreateEventSem( NULL, &w->play,  0, FALSE );
  DosCreateMutexSem( NULL, &w->mutex, 0, FALSE );

  w->decodertid = -1;
  w->status = DECODER_STOPPED;

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  instances = realloc( instances, ++instances_count * sizeof( DECODER_STRUCT* ));
  instances[ instances_count - 1 ] = w;
  DosReleaseMutexSem( mutex );

  return PLUGIN_OK;
}

/* Uninit function is called when another decoder than yours is needed. */
BOOL DLLENTRY
decoder_uninit( void* arg )
{
  DECODER_STRUCT* w = arg;
  int i;

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  decoder_command( w, DECODER_STOP, NULL );

  for( i = 0; i < instances_count; i++ ) {
    if( instances[i] == w ) {
      if( i < instances_count - 1 ) {
        memmove( instances + i, instances + i + 1,
               ( instances_count - i - 1 ) * sizeof( DECODER_STRUCT* ));
      }
      instances = realloc( instances, --instances_count * sizeof( DECODER_STRUCT* ));
    }
  }

  DosReleaseMutexSem( mutex  );
  DosCloseEventSem( w->play  );
  DosCloseMutexSem( w->mutex );
  free( w );

  return TRUE;
}

/* There is a lot of commands to implement for this function. Parameters
   needed for each of the are described in the definition of the structure
   in the decoder_plug.h file. */
ULONG DLLENTRY
decoder_command( void* arg, ULONG msg, DECODER_PARAMS* info )
{
  DECODER_STRUCT* w = arg;

  switch( msg )
  {
    case DECODER_SETUP:

      w->output_format.size     = sizeof( w->output_format );
      w->output_format.format   = WAVE_FORMAT_PCM;
      w->output_format.bits     = 16;
      w->output_format.channels = 2;

      w->hwnd                = info->hwnd;
      w->error_display       = info->error_display;
      w->info_display        = info->info_display;
      w->output_play_samples = info->output_play_samples;
      w->a                   = info->a;
      w->audio_buffersize    = info->audio_buffersize;
      w->metadata_buff       = info->metadata_buffer;
      w->metadata_buff[0]    = 0;
      w->metadata_size       = info->metadata_size;
      w->savename[0]         = 0;

      mpg_init( &w->mpeg, mmx_enable );
      break;

    case DECODER_PLAY:
    {
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

      if( w->decodertid != -1 ) {
        DosReleaseMutexSem( w->mutex );
        if( w->status == DECODER_STOPPED ) {
          decoder_command( w, DECODER_STOP, NULL );
          DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        } else {
          return PLUGIN_GO_ALREADY;
        }
      }

      strlcpy( w->filename, info->filename, sizeof( w->filename ));

      w->status     = DECODER_STARTING;
      w->jumptopos  = info->jumpto;
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
        return PLUGIN_GO_ALREADY;
      }

      w->stop = TRUE;
      mpg_abort( &w->mpeg );
      DosReleaseMutexSem( w->mutex );

      DosPostEventSem( w->play  );
      wait_thread( w->decodertid, 5000 );

      w->status = DECODER_STOPPED;
      w->decodertid = -1;
      break;
    }

    case DECODER_FFWD:
      if( info->ffwd ) {
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        if( w->decodertid == -1 || w->mpeg.is_stream ) {
          DosReleaseMutexSem( w->mutex );
          return PLUGIN_UNSUPPORTED;
        }
        DosReleaseMutexSem( w->mutex );
      }
      w->ffwd = info->ffwd;
      break;

    case DECODER_REW:
      if( info->rew ) {
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        if( w->decodertid == -1 || w->mpeg.is_stream ) {
          DosReleaseMutexSem( w->mutex );
          return PLUGIN_UNSUPPORTED;
        }
        DosReleaseMutexSem( w->mutex );
      }
      w->frew = info->rew;
      break;

    case DECODER_JUMPTO:
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
      w->jumptopos = info->jumpto;
      DosReleaseMutexSem( w->mutex );

      if( w->status == DECODER_STOPPED ) {
        DosPostEventSem( w->play );
      }
      break;

    case DECODER_EQ:
      mpg_equalize( &w->mpeg, info->equalizer, info->bandgain );
      break;

    case DECODER_SAVEDATA:
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
      if( info->save_filename == NULL ) {
        w->savename[0] = 0;
      } else {
        strlcpy( w->savename, info->save_filename, sizeof( w->savename ));
      }
      DosReleaseMutexSem( w->mutex );
      break;

    default:
      return PLUGIN_UNSUPPORTED;
  }
  return PLUGIN_OK;
}

/* Returns current status of the decoder. */
ULONG DLLENTRY
decoder_status( void* arg ) {
  return ((DECODER_STRUCT*)arg)->status;
}

/* Returns number of milliseconds the stream lasts. */
ULONG DLLENTRY
decoder_length( void* arg ) {
  return ((DECODER_STRUCT*)arg)->mpeg.songlength;
}

void static
copy_id3v2_string( ID3V2_TAG* tag, unsigned int id, char* result, int size )
{
  ID3V2_FRAME* frame = NULL;

  if( !*result ) {
    if(( frame = id3v2_get_frame( tag, id, 1 )) != NULL ) {
      id3v2_get_string( frame, result, size );
    }
  }
}

void static
copy_id3v2_tag( DECODER_INFO* info, ID3V2_TAG* tag )
{
  ID3V2_FRAME* frame;
  char buffer[128];
  int  i;

  if( tag ) {
    copy_id3v2_string( tag, ID3V2_TIT2, info->title,   sizeof( info->title   ));
    copy_id3v2_string( tag, ID3V2_TPE1, info->artist,  sizeof( info->artist  ));
    copy_id3v2_string( tag, ID3V2_TALB, info->album,   sizeof( info->album   ));
    copy_id3v2_string( tag, ID3V2_TCON, info->genre,   sizeof( info->genre   ));
    copy_id3v2_string( tag, ID3V2_TDRC, info->year,    sizeof( info->year    ));

    for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_COMM, i )) != NULL ; i++ )
    {
      id3v2_get_description( frame, buffer, sizeof( buffer ));

      // Skip iTunes specific comment tags.
      if( strnicmp( buffer, "iTun", 4 ) != 0 ) {
        id3v2_get_string( frame, info->comment, sizeof( info->comment ) );
        break;
      }
    }

    if( info->size >= INFO_SIZE_2 )
    {
      copy_id3v2_string( tag, ID3V2_TCOP, info->copyright, sizeof( info->copyright ));
      copy_id3v2_string( tag, ID3V2_TRCK, info->track, sizeof( info->track ));

      // Remove all unwanted track info.
      if(( i = atol( info->track )) != 0 ) {
        snprintf( info->track, sizeof( info->track ), "%02ld", atol( info->track ));
      } else {
        *info->track = 0;
      }

      for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_RVA2, i )) != NULL ; i++ )
      {
        float gain = 0;
        unsigned char* data = (unsigned char*)frame->fr_data;

        // Format of RVA2 frame:
        //
        // Identification          <text string> $00
        // Type of channel         $xx
        // Volume adjustment       $xx xx
        // Bits representing peak  $xx
        // Peak volume             $xx (xx ...)

        id3v2_get_description( frame, buffer, sizeof( buffer ));

        // Skip identification string.
        data += strlen((char*)data ) + 1;
        // Search the master volume.
        while( data - (unsigned char*)frame->fr_data < frame->fr_size ) {
          if( *data == 0x01 ) {
            gain = (float)((signed char)data[1] << 8 | data[2] ) / 512;
            break;
          } else {
            data += 3 + (( data[3] + 7 ) / 8 );
          }
        }
        if( gain != 0 ) {
          if( stricmp( buffer, "album" ) == 0 ) {
            info->album_gain = gain;
          } else {
            info->track_gain = gain;
          }
        }
      }

      for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_TXXX, i )) != NULL ; i++ )
      {
        id3v2_get_description( frame, buffer, sizeof( buffer ));

        if( stricmp( buffer, "replaygain_album_gain" ) == 0 ) {
          info->album_gain = atof( id3v2_get_string( frame, buffer, sizeof( buffer )));
        } else if( stricmp( buffer, "replaygain_album_peak" ) == 0 ) {
          info->album_peak = atof( id3v2_get_string( frame, buffer, sizeof( buffer )));
        } else if( stricmp( buffer, "replaygain_track_gain" ) == 0 ) {
          info->track_gain = atof( id3v2_get_string( frame, buffer, sizeof( buffer )));
        } else if( stricmp( buffer, "replaygain_track_peak" ) == 0 ) {
          info->track_peak = atof( id3v2_get_string( frame, buffer, sizeof( buffer )));
        }
      }
    }
  }
}

void static
copy_id3v1_string( ID3V1_TAG* tag, int id, char* result, int size )
{
  if( !*result ) {
    id3v1_get_string( tag, id, result, size );
  }
}

void static
copy_id3v1_tag( DECODER_INFO* info, ID3V1_TAG* tag )
{
  copy_id3v1_string( tag, ID3V1_TITLE,   info->title,   sizeof( info->title   ));
  copy_id3v1_string( tag, ID3V1_ARTIST,  info->artist,  sizeof( info->artist  ));
  copy_id3v1_string( tag, ID3V1_ALBUM,   info->album,   sizeof( info->album   ));
  copy_id3v1_string( tag, ID3V1_YEAR,    info->year,    sizeof( info->year    ));
  copy_id3v1_string( tag, ID3V1_COMMENT, info->comment, sizeof( info->comment ));
  copy_id3v1_string( tag, ID3V1_GENRE,   info->genre,   sizeof( info->genre   ));

  if( info->size >= INFO_SIZE_2 ) {
    copy_id3v1_string( tag, ID3V1_TRACK, info->track,   sizeof( info->track   ));
  }
}

/* Returns information about specified file. */
ULONG DLLENTRY
decoder_fileinfo( const char* filename, DECODER_INFO* info )
{
  int rc = PLUGIN_OK;
  DECODER_STRUCT* w;

  DEBUGLOG(("mpg123:decoder_fileinfo(%s, %p)\n", filename, info));

  if( decoder_init((void**)(void*)&w ) != PLUGIN_OK ) {
    return PLUGIN_NO_PLAY;
  }

  if(( rc = plg_open_file( w, filename, "rb" )) != 0 ) {
    decoder_uninit( w );
    return rc == -1 ? PLUGIN_NO_PLAY : PLUGIN_NO_READ;
  }

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  info->format.size       = sizeof( info->format );
  info->format.format     = WAVE_FORMAT_PCM;
  info->format.bits       = 16;
  info->format.channels   = w->mpeg.fr.channels;
  info->format.samplerate = w->mpeg.samplerate;
  info->mpeg              = w->mpeg.fr.mpeg25 ? 25 : ( w->mpeg.fr.lsf ? 20 : 10 );
  info->layer             = w->mpeg.fr.layer;
  info->mode              = w->mpeg.fr.mode;
  info->modext            = w->mpeg.fr.mode_ext;
  info->bpf               = w->mpeg.fr.framesize + 4;
  info->bitrate           = w->mpeg.bitrate;
  info->extention         = w->mpeg.fr.extension;
  info->junklength        = w->mpeg.started;
  info->songlength        = w->mpeg.songlength;

  if( info->size >= INFO_SIZE_2 ) {
      info->filesize = w->mpeg.filesize;
  }

  sprintf( info->tech_info, "%u kbs, %.1f kHz, %s",
           info->bitrate, ( info->format.samplerate / 1000.0 ), modes( info->mode ));

  if( w->mpeg.xing_header.flags & TOC_FLAG ) {
    strcat( info->tech_info, ", Xing" );
  }

  xio_get_metainfo( w->mpeg.file, XIO_META_GENRE, info->genre,   sizeof( info->genre   ));
  xio_get_metainfo( w->mpeg.file, XIO_META_NAME,  info->comment, sizeof( info->comment ));
  xio_get_metainfo( w->mpeg.file, XIO_META_TITLE, info->title,   sizeof( info->title   ));

  switch( tag_read_type ) {
    case TAG_READ_ID3V2_AND_ID3V1:
      copy_id3v2_tag( info,  w->mpeg.tagv2 );
      copy_id3v1_tag( info, &w->mpeg.tagv1 );
      break;
    case TAG_READ_ID3V1_AND_ID3V2:
      copy_id3v1_tag( info, &w->mpeg.tagv1 );
      copy_id3v2_tag( info,  w->mpeg.tagv2 );
      break;
    case TAG_READ_ID3V2_ONLY:
      copy_id3v2_tag( info,  w->mpeg.tagv2 );
      break;
    case TAG_READ_ID3V1_ONLY:
      copy_id3v1_tag( info, &w->mpeg.tagv1 );
      break;
  }

  if( !w->mpeg.is_stream && info->size >= INFO_SIZE_2 )
  {
    info->saveinfo = 1;
    info->codepage = ch_default();
    info->haveinfo = DECODER_HAVE_TITLE   |
                     DECODER_HAVE_ARTIST  |
                     DECODER_HAVE_ALBUM   |
                     DECODER_HAVE_YEAR    |
                     DECODER_HAVE_GENRE   |
                     DECODER_HAVE_TRACK   |
                     DECODER_HAVE_COMMENT ;

    if( tag_read_type != TAG_READ_ID3V1_ONLY )
    {
      info->haveinfo |= DECODER_HAVE_COPYRIGHT  |
                        DECODER_HAVE_TRACK_GAIN |
                        DECODER_HAVE_TRACK_PEAK |
                        DECODER_HAVE_ALBUM_GAIN |
                        DECODER_HAVE_ALBUM_PEAK ;
    }
  }

  DosReleaseMutexSem( w->mutex );

  plg_close_file( w );
  decoder_uninit( w );
  DEBUGLOG(("mpg123:decoder_fileinfo: %i {%i, {,%i,%i,%i,%i}, %i, %i,..., %s,\n"
            "\t%s, %s, %s, %s, %s, %s, %s, %s\n"
            "\t%i, %x, %i, %i, %f, %f, %f, %f}\n", rc,
    info->size, info->format.samplerate, info->format.channels, info->format.bits, info->format.format,
    info->songlength, info->junklength, info->tech_info,
    info->title, info->artist, info->album, info->year, info->comment, info->genre, info->track, info->copyright,
    info->codepage, info->haveinfo, info->saveinfo, info->filesize,
    info->track_gain, info->track_peak, info->album_gain, info->album_peak));
  return rc;
}

static void
replace_id3v2_string( ID3V2_TAG* tag, unsigned int id, const char* string )
{
  ID3V2_FRAME* frame;
  char buffer[128];
  int  i;

  if( id == ID3V2_COMM ) {
    for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_COMM, i )) != NULL ; i++ )
    {
      id3v2_get_description( frame, buffer, sizeof( buffer ));

      // Skip iTunes specific comment tags.
      if( strnicmp( buffer, "iTun", 4 ) != 0 ) {
        break;
      }
    }
  } else {
    frame = id3v2_get_frame( tag, id, 1 );
  }

  if( frame == NULL ) {
    frame = id3v2_add_frame( tag, id );
  }
  if( frame != NULL ) {
    id3v2_set_string( frame, string );
  }
}

ULONG DLLENTRY
decoder_saveinfo( const char* filename, const DECODER_INFO* info )
{
  XFILE* save = NULL;
  BOOL   copy = FALSE;
  char   savename[_MAX_PATH];
  ULONG  rc = PLUGIN_OK;
  int    i;
  int    started;

  DECODER_STRUCT* w;

  if( !*filename ) {
    return EINVAL;
  }

  strlcpy( savename, filename, sizeof( savename ));
  savename[ strlen( savename ) - 1 ] = '~';

  if( decoder_init((void**)(void*)&w ) != PLUGIN_OK ) {
    return errno;
  }

  if(( rc = plg_open_file( w, filename, "r+b" )) != 0 ) {
    DEBUGLOG(( "mpg123: unable open file for saving %s, rc=%d\n", filename, rc ));
    decoder_uninit( w );
    return rc;
  }

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( !w->mpeg.tagv2 ) {
    w->mpeg.tagv2 = id3v2_new_tag();
  }

  if( w->mpeg.tagv2->id3_started == 0 ) {
    if( tag_save_type == TAG_SAVE_ID3V2_AND_ID3V1 ||
        tag_save_type == TAG_SAVE_ID3V2           ||
        tag_save_type == TAG_SAVE_ID3V2_ONLY       )
    {
      int set_rc;

      // FIX ME: Replay gain info also must be saved.

      replace_id3v2_string( w->mpeg.tagv2, ID3V2_TIT2, info->title     );
      replace_id3v2_string( w->mpeg.tagv2, ID3V2_TPE1, info->artist    );
      replace_id3v2_string( w->mpeg.tagv2, ID3V2_TALB, info->album     );
      replace_id3v2_string( w->mpeg.tagv2, ID3V2_TDRC, info->year      );
      replace_id3v2_string( w->mpeg.tagv2, ID3V2_COMM, info->comment   );
      replace_id3v2_string( w->mpeg.tagv2, ID3V2_TCON, info->genre     );
      replace_id3v2_string( w->mpeg.tagv2, ID3V2_TCOP, info->copyright );
      replace_id3v2_string( w->mpeg.tagv2, ID3V2_TRCK, info->track     );

      xio_rewind( w->mpeg.file );
      set_rc = id3v2_set_tag( w->mpeg.file, w->mpeg.tagv2, savename );

      if( set_rc == 1 ) {
        copy = TRUE;
      } else if( set_rc == -1 ) {
        rc = xio_errno();
      }
    } else if( tag_save_type == TAG_SAVE_ID3V1_ONLY && w->mpeg.tagv2->id3_newtag == 0 ) {
      xio_rewind( w->mpeg.file );
      if( id3v2_wipe_tag( w->mpeg.file, savename ) == 0 ) {
        copy = TRUE;
      } else {
        rc = xio_errno();
      }
    }
  }

  if( rc == PLUGIN_OK ) {
    if( copy ) {
      if(( save = xio_fopen( savename, "r+b" )) == NULL ) {
        rc = xio_errno();
      }
    } else {
      save = w->mpeg.file;
    }
  } else {
    DEBUGLOG(( "mpg123: unable save id3v2 tag, rc=%d\n", rc ));
  }

  if( rc == PLUGIN_OK )
  {
    if( tag_save_type == TAG_SAVE_ID3V2_AND_ID3V1 ||
        tag_save_type == TAG_SAVE_ID3V1           ||
        tag_save_type == TAG_SAVE_ID3V1_ONLY       )
    {
      id3v1_clean_tag ( &w->mpeg.tagv1 );
      id3v1_set_string( &w->mpeg.tagv1, ID3V1_TITLE,   info->title   );
      id3v1_set_string( &w->mpeg.tagv1, ID3V1_ARTIST,  info->artist  );
      id3v1_set_string( &w->mpeg.tagv1, ID3V1_ALBUM,   info->album   );
      id3v1_set_string( &w->mpeg.tagv1, ID3V1_YEAR,    info->year    );
      id3v1_set_string( &w->mpeg.tagv1, ID3V1_COMMENT, info->comment );
      id3v1_set_string( &w->mpeg.tagv1, ID3V1_GENRE,   info->genre   );
      id3v1_set_string( &w->mpeg.tagv1, ID3V1_TRACK,   info->track   );

      if( id3v1_set_tag( save, &w->mpeg.tagv1 ) != 0 ) {
        rc = xio_errno();
      }
    } else if( tag_save_type == TAG_SAVE_ID3V2_ONLY ) {
      if( id3v1_wipe_tag( save ) != 0 ) {
        rc = xio_errno();
      }
    }
    if( rc != PLUGIN_OK ) {
      DEBUGLOG(( "mpg123: unable save id3v1 tag, rc=%d\n", rc ));
    }
  }

  if( save && save != w->mpeg.file ) {
    xio_fclose( save );
  }

  plg_close_file( w );
  DosReleaseMutexSem( w->mutex );

  if( rc != PLUGIN_OK || !copy ) {
    decoder_uninit( w );
    return rc;
  }

  if(( rc = plg_open_file( w, savename, "rb" )) != 0 ) {
    decoder_uninit( w );
    return rc;
  } else {
    // Remember a new position of the beginning of the data stream.
    started = w->mpeg.started;
    plg_close_file( w );
    decoder_uninit( w );
  }

  // Suspend all active instances of the updated file.
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );

  for( i = 0; i < instances_count; i++ )
  {
    w = instances[i];
    DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

    if( stricmp( w->filename, filename ) == 0 && w->mpeg.file ) {
      DEBUGLOG(( "mpg123: suspend currently used file: %s\n", w->filename ));
      w->resumepos = xio_ftell( w->mpeg.file ) - w->mpeg.started;
      xio_fclose( w->mpeg.file );
    } else {
      w->resumepos = -1;
    }
  }

  // Preserve EAs.
  eacopy( filename, savename );

  // Replace file.
  if( remove( filename ) == 0 ) {
    DEBUGLOG(("mpg123:decoder_saveinfo: deleted %s, replacing by %s\n", filename, savename));
    if( rename( savename, filename ) != 0 ) {
      rc = errno;
    }
  } else {
    rc = errno;
    DEBUGLOG(("mpg123:decoder_saveinfo: failed to delete %s (rc = %i), rollback %s\n", filename, errno, savename));
    remove( savename );
  }

  // Resume all suspended instances of the updated file.
  for( i = 0; i < instances_count; i++ )
  {
    w = instances[i];
    if( w->resumepos != -1 ) {
      DEBUGLOG(( "mpg123: resumes currently used file: %s\n", w->filename ));
      if(( w->mpeg.file = xio_fopen( w->mpeg.filename, "rb" )) != NULL ) {
        xio_fseek( w->mpeg.file, w->resumepos + started, XIO_SEEK_SET  );
        w->mpeg.started = started;
      }
    }
    DosReleaseMutexSem( w->mutex );
  }

  DosReleaseMutexSem( mutex );
  return rc;
}

ULONG DLLENTRY
decoder_trackinfo( const char* drive, int track, DECODER_INFO* info ) {
  return PLUGIN_NO_PLAY;
}

ULONG DLLENTRY
decoder_cdinfo( const char* drive, DECODER_CDINFO* info ) {
  return PLUGIN_NO_READ;
}

ULONG DLLENTRY
decoder_support( char* ext[], int* size )
{
  if( size ) {
    if( ext != NULL && *size >= 3 )
    {
      strcpy( ext[0], "*.MP3" );
      strcpy( ext[1], "*.MP2" );
      strcpy( ext[2], "*.MP1" );
    }
    *size = 3;
  }

  return DECODER_FILENAME | DECODER_URL | DECODER_METAINFO;
}

static void
init_tag( void )
{
  id3v2_set_read_charset( tag_read_id3v2_charset );
  id3v2_set_save_charset( tag_save_id3v2_charset, tag_save_id3v2_BOM );
  id3v1_set_read_charset( tag_read_id3v1_charset );
  id3v1_set_save_charset( tag_save_id3v1_charset );
}

static void
save_ini( void )
{
  HINI hini;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value( hini, tag_read_type          );
    save_ini_value( hini, tag_read_id3v1_charset );
    save_ini_value( hini, tag_read_id3v2_charset );
    save_ini_value( hini, tag_save_type          );
    save_ini_value( hini, tag_save_id3v1_charset );
    save_ini_value( hini, tag_save_id3v2_charset );
    save_ini_value( hini, mmx_enable  );
    close_ini( hini );
  }
}

static void
load_ini( void )
{
  HINI hini;

  mmx_enable  = 0;

  tag_read_type          = TAG_READ_ID3V2_AND_ID3V1;
  tag_read_id3v2_charset = CH_CP_NONE;
  tag_read_id3v1_charset = CH_CP_NONE;
  tag_save_type          = TAG_SAVE_ID3V2_AND_ID3V1;
  tag_save_id3v2_charset = 1208; // UTF-8
  tag_save_id3v1_charset = CH_CP_NONE;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, tag_read_type          );
    load_ini_value( hini, tag_read_id3v1_charset );
    load_ini_value( hini, tag_read_id3v2_charset );
    load_ini_value( hini, tag_save_type          );
    load_ini_value( hini, tag_save_id3v1_charset );
    load_ini_value( hini, tag_save_id3v2_charset );
    load_ini_value( hini, mmx_enable );
    close_ini( hini );
  }

  init_tag();
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      const CH_ENTRY* chp;
      const CH_ENTRY* chpe;
      
      lb_add_item( hwnd, CB_READ, "ID3v2 + ID3v1"  );
      lb_add_item( hwnd, CB_READ, "ID3v1 + ID3v2"  );
      lb_add_item( hwnd, CB_READ, "ID3v2"          );
      lb_add_item( hwnd, CB_READ, "ID3v1"          );
      lb_select  ( hwnd, CB_READ, tag_read_type    );
      // charset selection boxes
      chp = ch_list;
      chpe = chp + ch_list_size;
      for( ; chp != chpe; ++chp ) {
        uconv_attribute_t uco_attr;
        int ix;
        // get charset info
        if (!ch_info( chp->codepage, &uco_attr ))
          continue;

        if (( uco_attr.esid & 0xF00 ) != 0x200 ) // no DBCS
        { // => add to ID3V1 boxes
          ix = lb_add_item  ( hwnd, CB_ID3V1_RDCH, chp->name );
          PMASSERT(ix >= 0);
          lb_set_handle( hwnd, CB_ID3V1_RDCH, ix, (PVOID)chp );
          if( chp->codepage == tag_read_id3v1_charset ) {
            lb_select( hwnd, CB_ID3V1_RDCH, ix );
          }
          ix = lb_add_item  ( hwnd, CB_ID3V1_WRCH, chp->name );
          PMASSERT(ix >= 0);
          lb_set_handle( hwnd, CB_ID3V1_WRCH, ix, (PVOID)chp );
          if( chp->codepage == tag_save_id3v1_charset ) {
            lb_select( hwnd, CB_ID3V1_WRCH, ix );
          }
        }
        ix = lb_add_item  ( hwnd, CB_ID3V2_RDCH, chp->name );
        PMASSERT(ix >= 0);
        lb_set_handle( hwnd, CB_ID3V2_RDCH, ix, (PVOID)chp );
        if( chp->codepage == tag_read_id3v2_charset ) {
          lb_select( hwnd, CB_ID3V2_RDCH, ix );
        }
        switch (chp->codepage) // restrict to valid ID3V2 charsets
        {case 819:
         case 1200:
         case 1208:
          ix = lb_add_item  ( hwnd, CB_ID3V2_WRCH, chp->name );
          PMASSERT(ix >= 0);
          lb_set_handle( hwnd, CB_ID3V2_WRCH, ix, (PVOID)chp );
          if( chp->codepage == tag_save_id3v2_charset ) {
            lb_select( hwnd, CB_ID3V2_WRCH, ix );
          }
        }
      }

      lb_add_item( hwnd, CB_WRITE, "ID3v2 + ID3v1" );
      lb_add_item( hwnd, CB_WRITE, "ID3v2"         );
      lb_add_item( hwnd, CB_WRITE, "ID3v2 only"    );
      lb_add_item( hwnd, CB_WRITE, "ID3v1"         );
      lb_add_item( hwnd, CB_WRITE, "ID3v1 only"    );
      lb_select  ( hwnd, CB_WRITE, tag_save_type   );

      WinSendDlgItemMsg( hwnd, CB_USEMMX, BM_SETCHECK, MPFROMSHORT( mmx_enable ), 0 );

      if( !detect_MMX()) {
        WinEnableWindow( WinWindowFromID( hwnd, CB_USEMMX ), FALSE );
      }

      do_warpsans( hwnd );
      break;
    }

    case WM_COMMAND:
      if( SHORT1FROMMP( mp1 ) == DID_OK )
      {
        int  i;

        if(( i = lb_cursored( hwnd, CB_READ )) != LIT_NONE ) {
          tag_read_type = i;
        }
        if(( i = lb_cursored( hwnd, CB_ID3V1_RDCH )) != LIT_NONE ) {
          tag_read_id3v1_charset = ((const CH_ENTRY*)lb_get_handle( hwnd, CB_ID3V1_RDCH, i ))->codepage;
        }
        if(( i = lb_cursored( hwnd, CB_ID3V2_RDCH )) != LIT_NONE ) {
          tag_read_id3v2_charset = ((const CH_ENTRY*)lb_get_handle( hwnd, CB_ID3V2_RDCH, i ))->codepage;
        }

        if(( i = lb_cursored( hwnd, CB_WRITE )) != LIT_NONE ) {
          tag_save_type = i;
        }
        if(( i = lb_cursored( hwnd, CB_ID3V1_WRCH )) != LIT_NONE ) {
          tag_save_id3v1_charset = ((const CH_ENTRY*)lb_get_handle( hwnd, CB_ID3V1_WRCH, i ))->codepage;
        }
        if(( i = lb_cursored( hwnd, CB_ID3V2_WRCH )) != LIT_NONE ) {
          tag_save_id3v2_charset = ((const CH_ENTRY*)lb_get_handle( hwnd, CB_ID3V2_WRCH, i ))->codepage;
        }

        mmx_enable  = (BOOL)WinSendDlgItemMsg( hwnd, CB_USEMMX, BM_QUERYCHECK, 0, 0 );
        save_ini();
        init_tag();
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
int DLLENTRY
plugin_configure( HWND hwnd, HMODULE module ) {
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  return 0;
}

/****************************************************************************
 *
 *  ID3 tag editor
 *
 ***************************************************************************/

/* Processes messages of the dialog of edition of ID3 tag. */
static MRESULT EXPENTRY
id3_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  DEBUGLOG2(("mpg123:id3_page_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  {
    case WM_INITDLG:
    {
      int i;

      for( i = 0; i <= GENRE_LARGEST; i++) {
        WinSendDlgItemMsg( hwnd, CB_ID3_GENRE, LM_INSERTITEM,
                           MPFROMSHORT( LIT_END ), MPFROMP( genres[i] ));
      }
      WinSendDlgItemMsg( hwnd, CB_ID3_GENRE, LM_INSERTITEM,
                         MPFROMSHORT( LIT_END ), MPFROMP( "" ));
      break;
    }

    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd )
      { case PB_ID3_UNDO:
        { DECODER_INFO* data = (DECODER_INFO*)WinQueryWindowPtr( hwnd, 0 );
          WinSetDlgItemText( hwnd, EN_ID3_TITLE,   data->title   );
          WinSetDlgItemText( hwnd, EN_ID3_ARTIST,  data->artist  );
          WinSetDlgItemText( hwnd, EN_ID3_ALBUM,   data->album   );
          WinSetDlgItemText( hwnd, EN_ID3_TRACK,   data->track   );
          WinSetDlgItemText( hwnd, EN_ID3_COMMENT, data->comment );
          WinSetDlgItemText( hwnd, EN_ID3_YEAR,    data->year    );
          WinSetDlgItemText( hwnd, CB_ID3_GENRE,   data->genre   );
          return 0;
        }
        case PB_ID3_CLEAR:
          WinSetDlgItemText( hwnd, EN_ID3_TITLE,   "" );
          WinSetDlgItemText( hwnd, EN_ID3_ARTIST,  "" );
          WinSetDlgItemText( hwnd, EN_ID3_ALBUM,   "" );
          WinSetDlgItemText( hwnd, EN_ID3_TRACK,   "" );
          WinSetDlgItemText( hwnd, EN_ID3_COMMENT, "" );
          WinSetDlgItemText( hwnd, EN_ID3_YEAR,    "" );
          WinSetDlgItemText( hwnd, CB_ID3_GENRE,   "" );
          return 0;
      }
      break;

    /*case WM_CONTROL:
      if ( SHORT1FROMMP(mp1) == EN_ID3_TRACK && SHORT2FROMMP(mp1) == EN_CHANGE )
      {
        // read the track immediately to verify the syntax
        int tmp_track;
        char buf[4];
        int len1, len2;
        DECODER_INFO* data = (DECODER_INFO*)WinQueryWindowPtr( hwnd, 0 );

        len1 = WinQueryWindowText( HWNDFROMMP(mp2), sizeof buf, buf );
        if ( len1 != 0 &&                                       // empty is always OK
             ( sscanf( buf, "%u%n", &tmp_track, &len2 ) != 1 || // can't read
               len1      != len2                             || // more input
               tmp_track >= 256 ) )                             // too large
        {
          // bad input, restore last value TODO!!!!!
          WinSetDlgItemText( hwnd, EN_ID3_TRACK, data->track );
        } else
        {
          // OK, update last value
          strcpy( data->track, buf ); 
        }  
      }*/
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the dialog of edition of ID3 tag. */
static MRESULT EXPENTRY
id3_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  DEBUGLOG2(("mpg123:id3_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  {
    case WM_WINDOWPOSCHANGED:
    {
      PSWP pswp = (PSWP)mp1;
      if ( (pswp[0].fl & SWP_SIZE) && pswp[1].cx ) {
        // move/resize all controls
        LONG dx = pswp[0].cx - pswp[1].cx;
        LONG dy = pswp[0].cy - pswp[1].cy;
        SWP swp_temp;
        HWND hwnd_nb = WinWindowFromID( hwnd, NB_ID3TAG );
        
        WinQueryWindowPos( hwnd_nb, &swp_temp );
        WinSetWindowPos( hwnd_nb, NULLHANDLE, 0, 0, swp_temp.cx + dx, swp_temp.cy + dy, SWP_SIZE );
      }
      break;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Edits a ID3 tag for the specified file. */
ULONG DLLENTRY decoder_editmeta( HWND owner, const char* filename )
{
  char    caption[_MAX_FNAME] = "ID3 Tag Editor - ";
  HWND    hwnd;
  HWND    book;
  HWND    page01;
  MRESULT id;
  DECODER_INFO old_tag;
  DECODER_INFO new_tag;
  HMODULE module;
  ULONG   rc;

  DEBUGLOG(("mpg123:decoder_editmeta(%p, %s)\n", owner, filename));

  memset(&old_tag, 0, sizeof old_tag);
  memset(&new_tag, 0, sizeof new_tag);
  // read tag
  rc = decoder_fileinfo(filename, &old_tag);
  if (rc != PLUGIN_OK)
  { DEBUGLOG(("mpg123:decoder_editmeta decoder_fileinfo failed with rc = %u\n", rc));
    return rc;
  }

  DosQueryModFromEIP( &module, &rc, 0, NULL, &rc, (ULONG)&decoder_editmeta ); 
  hwnd = WinLoadDlg( HWND_DESKTOP, owner, id3_dlg_proc, module, DLG_ID3TAG, 0 );
  DEBUGLOG(("mpg123:decoder_editmeta: WinLoadDlg: %p (%p) - %p\n", hwnd, WinGetLastError(0), module));
  sfnameext( caption + strlen( caption ), filename, sizeof( caption ) - strlen( caption ));
  WinSetWindowText( hwnd, caption );

  book = WinWindowFromID( hwnd, NB_ID3TAG );
  do_warpsans( book );

  WinSendMsg( book, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_FIELDBACKGROUND),
              MPFROMSHORT(BKA_BACKGROUNDPAGECOLORINDEX));

  page01 = WinLoadDlg( book, book, id3_page_dlg_proc, module, DLG_ID3V10, 0 );
  id     = WinSendMsg( book, BKM_INSERTPAGE, MPFROMLONG(0),
                       MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR, BKA_LAST ));
  do_warpsans( page01 );

  WinSendMsg ( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG(id), MPFROMLONG( page01 ));
  WinSendMsg ( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "ID3 Tag" ));
  WinSetOwner( page01, book );

  WinSetWindowPtr( page01, 0, &old_tag );
  WinPostMsg( page01, WM_COMMAND,
              MPFROMSHORT( PB_ID3_UNDO ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));

  rc = WinProcessDlg( hwnd );
  DEBUGLOG(("mpg123:decoder_editmeta: dlg completed - %u, %p %p (%p)\n", rc, hwnd, page01, WinGetLastError(0)));

  if ( rc == DID_OK )
  { WinQueryDlgItemText( page01, EN_ID3_TITLE,   sizeof( new_tag.title   ), new_tag.title   );
    WinQueryDlgItemText( page01, EN_ID3_ARTIST,  sizeof( new_tag.artist  ), new_tag.artist  );
    WinQueryDlgItemText( page01, EN_ID3_ALBUM,   sizeof( new_tag.album   ), new_tag.album   );
    WinQueryDlgItemText( page01, EN_ID3_COMMENT, sizeof( new_tag.comment ), new_tag.comment );
    WinQueryDlgItemText( page01, EN_ID3_YEAR,    sizeof( new_tag.year    ), new_tag.year    );
    WinQueryDlgItemText( page01, EN_ID3_TRACK,   sizeof( new_tag.track   ), new_tag.track   );
    WinQueryDlgItemText( page01, CB_ID3_GENRE,   sizeof( new_tag.genre   ), new_tag.genre   );
  }

  WinDestroyWindow( page01 );
  WinDestroyWindow( hwnd   );

  if ( rc == DID_OK )
  { DEBUGLOG(("mpg123:decoder_editmeta: new\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s, %s\n",
      new_tag.title, new_tag.artist, new_tag.album, new_tag.comment, new_tag.year, new_tag.genre, new_tag.track));

    if( strcmp( old_tag.title,   new_tag.title   ) != 0 ||
        strcmp( old_tag.artist,  new_tag.artist  ) != 0 ||
        strcmp( old_tag.album,   new_tag.album   ) != 0 ||
        strcmp( old_tag.comment, new_tag.comment ) != 0 ||
        strcmp( old_tag.year,    new_tag.year    ) != 0 ||
        strcmp( old_tag.genre,   new_tag.genre   ) != 0 ||
        strcmp( old_tag.track,   new_tag.track   ) != 0 )
    { // save modified tag
      return decoder_saveinfo( filename, &new_tag ) ? 0 : 500;
    }
  }
  // Cancel or not modified
  return 300;
}

/* Returns information about plug-in. */
int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM* param )
{
  param->type         = PLUGIN_DECODER;
  param->author       = "Samuel Audet, Dmitry A.Steklenev";
  param->desc         = "MP3 Decoder 1.25";
  param->configurable = TRUE;

  load_ini();
  return 0;
}

int INIT_ATTRIBUTE
__dll_initialize( void )
{
  if( DosCreateMutexSem( NULL, &mutex, 0, FALSE ) != NO_ERROR ) {
    return 0;
  }

  return 1;
}

int TERM_ATTRIBUTE
__dll_terminate( void )
{
  free( instances );
  DosCloseMutexSem( mutex );
  return 1;
}

#if defined(__IBMC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return __dll_initialize();
  } else {
    __dll_terminate();
    #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
    #endif
    _CRT_term();
    return 1UL;
  }
}
#endif
