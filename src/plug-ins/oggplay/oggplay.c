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

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_ERRORS
#include <os2.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <errno.h>

#include <vorbis/vorbisfile.h>
#include <decoder_plug.h>
#include <plugin.h>
#include <utilfct.h>
#include <eautils.h>
#include <snprintf.h>
#include <debuglog.h>

#include "oggplay.h"
#include "vcedit.h"

static DECODER_STRUCT** instances = NULL;
static int  instances_count = 0;
static HMTX mutex;

/* Changes the current file position to a new location within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
static int DLLENTRY
vio_seek( void* w, ogg_int64_t offset, int whence )
{
  int    pos = 0;
  XFILE* x   = ((DECODER_STRUCT*)w)->file;
  DEBUGLOG(("oggplay:vio_seek(%p, %li, %i)\n", w, offset, whence));

  if( xio_can_seek(x) < XIO_CAN_SEEK_FAST ) {
    return -1;
  }

  switch( whence )
  {
    case SEEK_SET: pos = offset; break;
    case SEEK_CUR: pos = xio_ftell(x) + offset; break;
    case SEEK_END: pos = xio_fsize(x) + offset; break;
    default:
      return -1;
  }

  if( xio_fseek( x, pos, XIO_SEEK_SET ) == 0 ) {
    return pos;
  } else {
    return -1;
  }
}

/* Finds the current position of the file. Returns the current file
   position. On error, returns -1L and sets errno to a nonzero value. */
static long DLLENTRY
vio_tell( void* w ) {
  return xio_ftell(((DECODER_STRUCT*)w)->file );
}

/* Reads up to count items of size length from the input file and
   stores them in the given buffer. The position in the file increases
   by the number of bytes read. Returns the number of full items
   successfully read, which can be less than count if an error occurs
   or if the end-of-file is met before reaching count. */
static size_t DLLENTRY
vio_read( void* ptr, size_t size, size_t count, void* w ) {
  DEBUGLOG(("oggplay:vio_read(%p, %i, %i, %p)\n", ptr, size, count, w));
  return xio_fread( ptr, size, count, ((DECODER_STRUCT*)w)->file );
}

/* Closes a pointed file. Returns 0 if it successfully closes
   the file, or -1 if any errors were detected. */
static int DLLENTRY
vio_close( void* w )
{ int rc;
  DEBUGLOG(("oggplay:vio_close(%p)\n", w));
  rc = xio_fclose(((DECODER_STRUCT*)w)->file );
  ((DECODER_STRUCT*)w)->file = NULL;
  return rc;
}

/* Opens Ogg Vorbis file. Returns 0 if it successfully opens the file.
   A nonzero return value indicates an error. A -1 return value
   indicates an unsupported format of the file. */
static ULONG
ogg_open( DECODER_STRUCT* w )
{
  ov_callbacks callbacks;
  DEBUGLOG(("oggplay:ogg_open(%p)\n", w));
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  w->comment = NULL;
  w->vrbinfo = NULL;

  if(( w->file = xio_fopen( w->filename, "rb" )) == NULL ) {
    DosReleaseMutexSem( w->mutex );
    return xio_errno();
  }

  callbacks.read_func  = vio_read;
  callbacks.seek_func  = vio_seek;
  callbacks.tell_func  = vio_tell;
  callbacks.close_func = vio_close;

  // The ov_open_callbaks() function performs full stream detection and machine
  // initialization. If it returns 0, the stream *is* Vorbis and we're
  // fully ready to decode.
  if( ov_open_callbacks( w, &w->vrbfile, NULL, 0, callbacks ) < 0 ) {
    vio_close( w );
    DosReleaseMutexSem( w->mutex );
    return -1;
  }

  if(( w->vrbinfo = ov_info( &w->vrbfile, -1 )) == NULL ) {
    ov_clear( &w->vrbfile );
    DosReleaseMutexSem( w->mutex );
    return -1;
  }

  w->songlength  = ov_time_total( &w->vrbfile, -1 ) * 1000.0;
  w->comment     = ov_comment( &w->vrbfile, -1 );
  w->bitrate     = ov_bitrate( &w->vrbfile, -1 ) / 1000;
  w->played_pcms = 0;
  w->played_secs = 0;

  w->output_format.size       = sizeof( w->output_format );
  w->output_format.format     = WAVE_FORMAT_PCM;
  w->output_format.bits       = 16;
  w->output_format.channels   = w->vrbinfo->channels;
  w->output_format.samplerate = w->vrbinfo->rate;

  if( w->metadata_buff && w->metadata_size && w->hwnd ) {
    xio_set_observer( w->file, w->hwnd, w->metadata_buff, w->metadata_size );
  }

  DosReleaseMutexSem( w->mutex );
  return 0;
}

/* Reads up to count pcm bytes from the Ogg Vorbis stream and
   stores them in the given buffer. */
static int
ogg_read( DECODER_STRUCT* w, char* buffer, int count, int* posmarker )
{
  int done;
  int read    =  0;
  int section = -1;
  int resync  =  0;
  DEBUGLOG(("oggplay:ogg_read(%p, %p, %i, %p)\n", w, buffer, count, posmarker ));

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( w->file ) {
    while( read < count )
    {
      done = ov_read( &w->vrbfile, buffer + read, count - read, 0, 2, 1, &section );

      if( done == OV_HOLE ) {
        if( resync++ < MAXRESYNC ) {
          continue;
        }
      }
      if( done <= 0 ) {
        break;
      } else {
        read += done;
      }
    }

    w->played_pcms = ov_pcm_tell ( &w->vrbfile );
    w->played_secs = ov_time_tell( &w->vrbfile );

    *posmarker = 1000 * w->played_secs;
  } else {
    read = -1;
  }

  DosReleaseMutexSem( w->mutex );
  return read;
}

/* Closes Ogg Vorbis file. */
static void
ogg_close( DECODER_STRUCT* w )
{
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( w->file ) {
    ov_clear( &w->vrbfile );
  }
  DosReleaseMutexSem( w->mutex );
}

#define OGG_SEEK_SET 0
#define OGG_SEEK_CUR 1

/* Changes the current file position to a new time within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
static int
ogg_seek( DECODER_STRUCT* w, double secs, int origin )
{
  int rc;
  DEBUGLOG(("oggplay:ogg_seek(%p, %f, %i)\n", w, secs, origin));
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( w->file ) {
    switch( origin ) {
      case OGG_SEEK_SET:
        rc = ov_time_seek_page( &w->vrbfile, secs );
        break;
      case OGG_SEEK_CUR:
        rc = ov_time_seek_page( &w->vrbfile, w->played_secs + secs );
        break;
      default:
        rc = -1;
    }

    if( rc == 0 ) {
      w->played_pcms = ov_pcm_tell ( &w->vrbfile );
      w->played_secs = ov_time_tell( &w->vrbfile );
    }
  } else {
    rc = -1;
  }

  DosReleaseMutexSem( w->mutex );
  return rc;
}

/* Returns a specified field of the given Ogg Vorbis comment. */
static char*
ogg_get_string( DECODER_STRUCT* w, char* target, char* type, int size )
{
  const char* string;
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( w->comment && ( string = vorbis_comment_query( w->comment, type, 0 )) != NULL ) {
    ch_convert( 1208, string, CH_CP_NONE, target, size, 0 );
  } else if( size ) {
    *target = 0;
  }

  DosReleaseMutexSem( w->mutex );
  return target;
}

/* Sets a specified field of the given Ogg Vorbis comment. */
static void
ogg_set_string( vorbis_comment* comment, const char* source, char* type )
{
  char string[128*4];

  ch_convert( CH_CP_NONE, source, 1208, string, sizeof( string ), 0 );
  vorbis_comment_add_tag( comment, type, string );
}

/* Decoding thread. */
static void TFNENTRY
decoder_thread( void* arg )
{
  ULONG resetcount;
  int   posmarker;
  ULONG rc;

  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;

  char* buffer = NULL;
  char  last_title[512] = "";
  char  curr_title[512];

  w->frew = FALSE;
  w->ffwd = FALSE;

  if(( rc = ogg_open( w )) != 0 )
  {
    char errorbuf[1024];

    strlcpy( errorbuf, "Unable open file:\n", sizeof( errorbuf ));
    strlcat( errorbuf, w->filename, sizeof( errorbuf ));
    strlcat( errorbuf, "\n", sizeof( errorbuf ));
    if( rc != -1 ) {
      strlcat( errorbuf, xio_strerror( xio_errno()), sizeof( errorbuf ));
    } else {
      strlcat( errorbuf, "Unsupported format of the file.", sizeof( errorbuf ));
    }

    w->error_display( errorbuf );
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  // After opening a new file we so are in its beginning.
  if( w->jumptopos == 0 ) {
      w->jumptopos = -1;
  }

  if(!( buffer = (unsigned char*)malloc( w->audio_buffersize ))) {
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
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
      int read, write;

      if( w->jumptopos >= 0 ) {
        ogg_seek( w, w->jumptopos / 1000.0, OGG_SEEK_SET );
        w->jumptopos = -1;
        WinPostMsg( w->hwnd, WM_SEEKSTOP, 0, 0 );
      }

      if( w->frew ) {
        if( ogg_seek( w, -0.5, OGG_SEEK_CUR ) != 0 ) {
          break;
        }
      }

      if( w->ffwd ) {
        ogg_seek( w, 0.5, OGG_SEEK_CUR );
      }

      read = ogg_read( w, buffer, w->audio_buffersize, &posmarker );

      if( read <= 0 ) {
        break;
      }

      write = w->output_play_samples( w->a, &w->output_format, buffer, read, posmarker );

      if( write != read ) {
        WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
        break;
      }

      if( w->metadata_buff && w->metadata_size && w->hwnd )
      {
        ogg_get_string( w, curr_title, "TITLE", sizeof( curr_title ));

        if( strcmp( curr_title, last_title ) != 0 ) {
          strlcpy ( last_title, curr_title, sizeof( last_title ));
          snprintf( w->metadata_buff, w->metadata_size, "StreamTitle='%s';", last_title );
          WinPostMsg( w->hwnd, WM_METADATA, MPFROMP( w->metadata_buff ), 0 );
        }
      }
    }
    WinPostMsg( w->hwnd, WM_PLAYSTOP, 0, 0 );
    w->status = DECODER_STOPPED;
  }

end:

  ogg_close( w );
  free( buffer );

  w->status = DECODER_STOPPED;
  _endthread();
}

/* Init function is called when PM123 needs the specified decoder to play
   the stream demanded by the user. */
int DLLENTRY
decoder_init( void** returnw )
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)calloc( sizeof(*w), 1 );
  *returnw = w;

  DosCreateEventSem( NULL, &w->play,  0, FALSE );
  DosCreateMutexSem( NULL, &w->mutex, 0, FALSE );

  w->decodertid = -1;
  w->status = DECODER_STOPPED;
  w->file = NULL;

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  instances = realloc( instances, ++instances_count * sizeof( DECODER_STRUCT* ));
  instances[ instances_count - 1 ] = w;
  DosReleaseMutexSem( mutex );

  return PLUGIN_OK;
}

/* Uninit function is called when another decoder than this is needed. */
BOOL DLLENTRY
decoder_uninit( void* arg )
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;
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
      w->output_play_samples = info->output_play_samples;
      w->hwnd                = info->hwnd;
      w->error_display       = info->error_display;
      w->info_display        = info->info_display;
      w->a                   = info->a;
      w->audio_buffersize    = info->audio_buffersize;
      w->metadata_buff       = info->metadata_buffer;
      w->metadata_buff[0]    = 0;
      w->metadata_size       = info->metadata_size;
      break;

    case DECODER_PLAY:
    {
      if( w->decodertid != -1 ) {
        if( w->status == DECODER_STOPPED ) {
          decoder_command( w, DECODER_STOP, NULL );
        } else {
          return PLUGIN_GO_ALREADY;
        }
      }

      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
      strlcpy( w->filename, info->filename, sizeof( w->filename ));
      DosReleaseMutexSem( w->mutex );

      w->jumptopos  = info->jumpto;
      w->status     = DECODER_STARTING;
      w->stop       = FALSE;
      w->songlength = 0;
      w->decodertid = _beginthread( decoder_thread, 0, 65535, (void*)w );

      DosPostEventSem( w->play  );
      break;
    }

    case DECODER_STOP:
    {
      if( w->decodertid == -1 ) {
        return PLUGIN_GO_ALREADY;
      }

      w->stop = TRUE;

      if( w->file ) {
        xio_fabort( w->file );
      }

      DosPostEventSem( w->play  );
      wait_thread( w->decodertid, 5000 );
      w->status = DECODER_STOPPED;
      w->decodertid = -1;
      break;
    }

    case DECODER_FFWD:
      if( info->ffwd ) {
        if( w->decodertid == -1 || xio_can_seek( w->file ) != XIO_CAN_SEEK_FAST ) {
          return PLUGIN_UNSUPPORTED;
        }
      }
      w->ffwd = info->ffwd;
      break;

    case DECODER_REW:
      if( info->rew ) {
        if( w->decodertid == -1 || xio_can_seek( w->file ) != XIO_CAN_SEEK_FAST ) {
          return PLUGIN_UNSUPPORTED;
        }
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
  return ((DECODER_STRUCT*)arg)->songlength;
}

/* Returns information about specified file. */
ULONG DLLENTRY
decoder_fileinfo( const char* filename, DECODER_INFO* info )
{
  DECODER_STRUCT* w;
  int rc;

  if( decoder_init((void**)(void*)&w ) != PLUGIN_OK ) {
    return PLUGIN_NO_PLAY;
  }

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
  strlcpy( w->filename, filename, sizeof( w->filename ));

  if(( rc = ogg_open( w )) != 0 ) {
    DosReleaseMutexSem( w->mutex );
    return rc == -1 ? PLUGIN_NO_PLAY : PLUGIN_NO_READ;
  }

  info->format     = w->output_format;
  info->songlength = w->songlength;
  info->mode       = w->output_format.channels == 1 ? 3 : 0;
  info->bitrate    = w->bitrate;

  if( info->size >= INFO_SIZE_2 ) {
    info->filesize = xio_fsize( w->file );
  }

  DosReleaseMutexSem( w->mutex );

  if( info->songlength <= 0 ) {
    info->songlength = -1;
  }

  snprintf( info->tech_info, sizeof( info->tech_info ), "%u kbs, %.1f kHz, %s",
            info->bitrate, ( info->format.samplerate / 1000.0 ),
            info->format.channels == 1 ? "Mono" : "Stereo" );

  ogg_get_string( w, info->artist,  "ARTIST",  sizeof( info->artist  ));
  ogg_get_string( w, info->album,   "ALBUM",   sizeof( info->album   ));
  ogg_get_string( w, info->title,   "TITLE",   sizeof( info->title   ));
  ogg_get_string( w, info->genre,   "GENRE",   sizeof( info->genre   ));
  ogg_get_string( w, info->year,    "YEAR" ,   sizeof( info->year    ));
  ogg_get_string( w, info->comment, "COMMENT", sizeof( info->comment ));

  if( !*info->year ) {
    ogg_get_string( w, info->year, "DATE", sizeof( info->year ));
  }

  if( info->size >= INFO_SIZE_2 )
  {
    char buffer[128];

    ogg_get_string( w, info->copyright, "COPYRIGHT",   sizeof( info->copyright ));
    ogg_get_string( w, info->track,     "TRACKNUMBER", sizeof( info->track     ));

    if( *info->track )
    {
      int track = atoi( info->track );
      if( track ) {
        sprintf( info->track, "%02d", track );
      }
    }

    ogg_get_string( w, buffer, "replaygain_album_gain", sizeof( buffer ));
    info->album_gain = atof( buffer );
    ogg_get_string( w, buffer, "replaygain_album_peak", sizeof( buffer ));
    info->album_peak = atof( buffer );
    ogg_get_string( w, buffer, "replaygain_track_gain", sizeof( buffer ));
    info->track_gain = atof( buffer );
    ogg_get_string( w, buffer, "replaygain_track_peak", sizeof( buffer ));
    info->track_peak = atof( buffer );

    info->saveinfo = is_file( filename );
    info->haveinfo = DECODER_HAVE_TITLE      |
                     DECODER_HAVE_ARTIST     |
                     DECODER_HAVE_ALBUM      |
                     DECODER_HAVE_YEAR       |
                     DECODER_HAVE_GENRE      |
                     DECODER_HAVE_TRACK      |
                     DECODER_HAVE_COMMENT    |
                     DECODER_HAVE_COPYRIGHT  |
                     DECODER_HAVE_TRACK_GAIN |
                     DECODER_HAVE_TRACK_PEAK |
                     DECODER_HAVE_ALBUM_GAIN |
                     DECODER_HAVE_ALBUM_PEAK ;

    info->codepage = ch_default();
  }

  ogg_close( w );
  decoder_uninit( w );
  return PLUGIN_OK;
}

static size_t
vce_read( void* ptr, size_t size, size_t count, void* file ) {
  return xio_fread( ptr, size, count, (XFILE*)file );
}
static size_t
vce_write( void* ptr, size_t size, size_t count, void* file ) {
  return xio_fwrite( ptr, size, count, (XFILE*)file );
}

/* It is called if it is necessary to change the meta information
   for the specified file. */
ULONG DLLENTRY
decoder_saveinfo( const char* filename, const DECODER_INFO* info )
{
  XFILE* file = NULL;
  XFILE* save = NULL;
  int    rc = PLUGIN_OK;
  char   savename[_MAX_PATH];
  int    i;
  char   buffer[64];

  vcedit_state*   state = NULL;
  vorbis_comment* vc;
  DECODER_STRUCT* w;

  if( !*filename ) {
    return EINVAL;
  }

  strlcpy( savename, filename, sizeof( savename ));
  savename[ strlen( savename ) - 1 ] = '~';

  for(;;)
  {
    if(( file = xio_fopen( filename, "rb" )) == NULL ) {
      rc = xio_errno();
      break;
    }
    if(( save = xio_fopen( savename, "wb" )) == NULL ) {
      rc = xio_errno();
      break;
    }
    if(( state = vcedit_new_state()) == NULL ) {
      rc = errno;
      break;
    }

    if( vcedit_open_callbacks( state, file, vce_read, vce_write ) < 0 )
    {
      rc = EINVAL;
      break;
    }

    vc = vcedit_comments( state );
    vorbis_comment_clear( vc );
    vorbis_comment_init ( vc );

    ogg_set_string( vc, info->artist,    "ARTIST"      );
    ogg_set_string( vc, info->album,     "ALBUM"       );
    ogg_set_string( vc, info->title,     "TITLE"       );
    ogg_set_string( vc, info->genre,     "GENRE"       );
    ogg_set_string( vc, info->year,      "YEAR"        );
    ogg_set_string( vc, info->year,      "DATE"        );
    ogg_set_string( vc, info->track,     "TRACKNUMBER" );
    ogg_set_string( vc, info->copyright, "COPYRIGHT"   );
    ogg_set_string( vc, info->comment,   "COMMENT"     );

    if( info->album_gain ) {
      sprintf( buffer, "%.2f dB", info->album_gain );
      ogg_set_string( vc, buffer, "replaygain_album_gain" );
    }
    if( info->album_peak ) {
      sprintf( buffer, "%.6f", info->album_peak );
      ogg_set_string( vc, buffer, "replaygain_album_peak" );
    }
    if( info->track_gain ) {
      sprintf( buffer, "%.2f dB", info->track_gain );
      ogg_set_string( vc, buffer, "replaygain_track_gain" );
    }
    if( info->track_peak ) {
      sprintf( buffer, "%.6f", info->track_peak );
      ogg_set_string( vc, buffer, "replaygain_track_peak" );
    }

    if( vcedit_write( state, save ) < 0 ) {
      rc = errno;
    }
    break;
  }

  if( state ) {
    vcedit_clear( state );
  }
  if( file  ) {
    xio_fclose( file );
  }
  if( save  ) {
    xio_fclose( save );
  }

  if( rc != PLUGIN_OK ) {
    return rc;
  }

  // Suspend all active instances of the updated file.
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );

  for( i = 0; i < instances_count; i++ )
  {
    w = instances[i];
    DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

    if( stricmp( w->filename, filename ) == 0 && w->file ) {
      DEBUGLOG(( "oggplay: suspend currently used file: %s\n", w->filename ));
      w->resume_pcms = w->played_pcms;
      ogg_close( w );
    } else {
      w->resume_pcms = -1;
    }
  }

  // Preserve EAs.
  eacopy( filename, savename );

  // Replace file.
  if( remove( filename ) == 0 ) {
    if( rename( savename, filename ) != 0 ) {
      rc = errno;
    }
  } else {
    rc = errno;
    remove( savename );
  }

  // Resume all suspended instances of the updated file.
  for( i = 0; i < instances_count; i++ )
  {
    w = instances[i];
    if( w->resume_pcms != -1 ) {
      DEBUGLOG(( "oggplay: resumes currently used file: %s\n", w->filename ));
      if( ogg_open( w ) == PLUGIN_OK ) {
        ov_pcm_seek( &w->vrbfile, w->resume_pcms );
      }
    }
    DosReleaseMutexSem( w->mutex );
  }

  DosReleaseMutexSem( mutex );
  return rc;
}

/* Returns information about specified track. */
ULONG DLLENTRY
decoder_trackinfo( const char* drive, int track, DECODER_INFO* info ) {
  return PLUGIN_NO_PLAY;
}

/* Returns information about a disc inserted to the specified drive. */
ULONG DLLENTRY
decoder_cdinfo( const char* drive, DECODER_CDINFO* info ) {
  return PLUGIN_NO_READ;
}

/* What can be played via the decoder. */
ULONG DLLENTRY
decoder_support( char* ext[], int* size )
{
  if( size ) {
    if( ext != NULL && *size > 0 ) {
      strcpy( ext[ 0], "*.OGG" );
    }
    *size = 1;
  }

  return DECODER_FILENAME | DECODER_URL | DECODER_METAINFO;
}

/* Returns information about plug-in. */
int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM* param )
{
  param->type         = PLUGIN_DECODER;
  param->author       = "Dmitry A.Steklenev";
  param->desc         = "OGG Play 2.01";
  param->configurable = FALSE;
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
                                     unsigned long flag       )
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

