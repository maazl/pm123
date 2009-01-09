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

#include "mpg123.h"

#include <utilfct.h>
#include <decoder_plug.h>
#include <debuglog.h>
#include <snprintf.h>
#include <eautils.h>
#include <debuglog.h>
#include <id3v1.h>
#include <id3v2.h>
#include <os2.h>

#include <stdio.h>
#include <malloc.h>
#include <process.h>
#include <errno.h>
#include <ctype.h>

#include "genre.h"


static DECODER_STRUCT** instances = NULL;
static int  instances_count = 0;
static HMTX mutex;

typedef enum
{ TAG_READ_ID3V2_AND_ID3V1,
  TAG_READ_ID3V1_AND_ID3V2,
  TAG_READ_ID3V2_ONLY,
  TAG_READ_ID3V1_ONLY,
  TAG_READ_NONE
} read_type;

typedef enum
{ TAG_SAVE_ID3V1_UNCHANGED,
  TAG_SAVE_ID3V1_WRITE,
  TAG_SAVE_ID3V1_DELETE,
  TAG_SAVE_ID3V1_NOID3V2
} save_id3v1_type;

typedef enum
{ TAG_SAVE_ID3V2_UNCHANGED,
  TAG_SAVE_ID3V2_WRITE,
  TAG_SAVE_ID3V2_DELETE,
  TAG_SAVE_ID3V2_ONDEMAND,
  TAG_SAVE_ID3V2_ONDEMANDSPC
} save_id3v2_type;

static read_type       tag_read_type          = TAG_READ_ID3V2_AND_ID3V1;
static ULONG           tag_id3v1_charset      = 1004; // ISO8859-1
static BOOL            tag_read_id3v1_autoch  = TRUE;
static save_id3v1_type tag_save_id3v1_type    = TAG_SAVE_ID3V1_WRITE;
static save_id3v2_type tag_save_id3v2_type    = TAG_SAVE_ID3V2_WRITE;
static ULONG           tag_read_id3v2_charset = 1004; // ISO8859-1
static uint8_t         tag_save_id3v2_encoding= ID3V2_ENCODING_UTF8;

static CH_ENTRY id3v2_ch_list[4]; 

/* Initialize the above list. Must be called once. */
static void init_id3v2_ch_list()
{ id3v2_ch_list[ID3V2_ENCODING_ISO_8859_1] = *ch_find(1004);
  id3v2_ch_list[ID3V2_ENCODING_UTF16_BOM]  =
  id3v2_ch_list[ID3V2_ENCODING_UTF16]      = *ch_find(1200);
  id3v2_ch_list[ID3V2_ENCODING_UTF8]       = *ch_find(1208);
  static char name4[100];
  strlcpy(name4, id3v2_ch_list[ID3V2_ENCODING_UTF16_BOM].name, sizeof name4);
  strlcat(name4, " with BOM", sizeof name4);
  id3v2_ch_list[ID3V2_ENCODING_UTF16_BOM].name = name4;
}


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
    if(!( w->save = xio_fopen( w->savename, "abU" )))
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

  if(( rc = plg_open_file( w, w->filename, "rbXU" )) != 0 ) {
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

      mpg_init( &w->mpeg, TRUE );
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
      id3v2_get_string_ex( frame, result, size, tag_read_id3v2_charset );
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
    copy_id3v2_string(tag, ID3V2_TIT2, info->title,  sizeof info->title );
    copy_id3v2_string(tag, ID3V2_TPE1, info->artist, sizeof info->artist);
    copy_id3v2_string(tag, ID3V2_TALB, info->album,  sizeof info->album );
    copy_id3v2_string(tag, ID3V2_TCON, info->genre,  sizeof info->genre );
    copy_id3v2_string(tag, ID3V2_TDRC, info->year,   sizeof info->year  );

    for( i = 1; ( frame = id3v2_get_frame( tag, ID3V2_COMM, i )) != NULL ; i++ )
    {
      id3v2_get_description( frame, buffer, sizeof( buffer ));

      // Skip iTunes specific comment tags.
      if( strnicmp( buffer, "iTun", 4 ) != 0 ) {
        id3v2_get_string_ex( frame, info->comment, sizeof( info->comment ), tag_read_id3v2_charset );
        break;
      }
    }

    if( info->size >= INFO_SIZE_2 )
    {
      copy_id3v2_string( tag, ID3V2_TCOP, info->copyright, sizeof info->copyright);
      copy_id3v2_string( tag, ID3V2_TRCK, info->track, sizeof info->track);

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
        // Identification          <text string> $00        // Type of channel         $xx
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

static void strpad(char* str, size_t len, char pad)
{ char* se = str + len;
  str += strnlen(str, len);
  while (str != se)
    *str++ = pad;
}

static ULONG detect_id3v1_codepage( const ID3V1_TAG* tag, ULONG codepage )
{
  ID3V1_TAG buf = *tag;
  strpad(buf.title,  sizeof buf.title,  ' ');
  strpad(buf.artist, sizeof buf.artist, ' ');
  strpad(buf.album,  sizeof buf.album,  ' ');
  strpad(buf.year,   sizeof buf.year,   ' ');
  strpad(buf.comment,30                ,' ');
  buf.genre = 0;
  if (tag->empty == 0 && tag->track) // ID3V1.1
    buf.empty = 0;

  return ch_detect(codepage, buf.title);
}

void static
copy_id3v1_string( ID3V1_TAG* tag, int id, char* result, int size )
{
  if( !*result ) {
    id3v1_get_string( tag, id, result, size, tag_id3v1_charset );
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

  if(( rc = plg_open_file( w, filename, "rbU" )) != 0 ) {
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
    case TAG_READ_NONE:; // avoid warning
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
    id3v2_set_string( frame, string, tag_save_id3v2_encoding );
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

  if(( rc = plg_open_file( w, filename, "r+bU" )) != 0 ) {
    DEBUGLOG(( "mpg123: unable open file for saving %s, rc=%d\n", filename, rc ));
    decoder_uninit( w );
    return rc;
  }

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  /* TODO @@@@@@@@@
  
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
      if(( save = xio_fopen( savename, "r+bU" )) == NULL ) {
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

  plg_close_file( w );*/
  DosReleaseMutexSem( w->mutex );

  if( rc != PLUGIN_OK || !copy ) {
    decoder_uninit( w );
    return rc;
  }

  if(( rc = plg_open_file( w, savename, "rbU" )) != 0 ) {
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
      if(( w->mpeg.file = xio_fopen( w->mpeg.filename, "rbXU" )) != NULL ) {
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
  /* TODO @@@@@@@@@@
  id3v2_set_read_charset( tag_read_id3v2_charset );
  id3v2_set_save_charset( tag_save_id3v2_charset, tag_save_id3v2_BOM );
  id3v1_set_read_charset( tag_read_id3v1_charset );
  id3v1_set_save_charset( tag_save_id3v1_charset );*/
}

static void
save_ini( void )
{
  HINI hini;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value( hini, tag_read_type           );
    save_ini_value( hini, tag_id3v1_charset       );
    save_ini_value( hini, tag_read_id3v1_autoch   );
    save_ini_value( hini, tag_save_id3v1_type     );
    save_ini_value( hini, tag_save_id3v2_type     );
    save_ini_value( hini, tag_read_id3v2_charset  );
    save_ini_value( hini, tag_save_id3v2_encoding );
    close_ini( hini );
  }
}

static void
load_ini( void )
{
  HINI hini;

  tag_read_type          = TAG_READ_ID3V2_AND_ID3V1;
  tag_id3v1_charset      = 819;  // ISO8859-1
  tag_read_id3v1_autoch  = TRUE;
  tag_save_id3v1_type    = TAG_SAVE_ID3V1_WRITE;
  tag_save_id3v2_type    = TAG_SAVE_ID3V2_WRITE;
  tag_read_id3v2_charset = 819;  // ISO8859-1
  tag_save_id3v2_encoding= ID3V2_ENCODING_UTF8;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, tag_read_type           );
    load_ini_value( hini, tag_id3v1_charset       );
    load_ini_value( hini, tag_read_id3v1_autoch   );
    load_ini_value( hini, tag_save_id3v1_type     );
    load_ini_value( hini, tag_save_id3v2_type     );
    load_ini_value( hini, tag_read_id3v2_charset  );
    load_ini_value( hini, tag_save_id3v2_encoding );
    close_ini( hini );
  }

  init_tag();
}

/* populate a listbox with character set items */
static void dlg_populate_charset_listbox( HWND lb, const CH_ENTRY* list, size_t count, ULONG selected )
{ while (count--)
  { SHORT idx = SHORT1FROMMR( WinSendMsg( lb, LM_INSERTITEM, MPFROMSHORT( LIT_END ), MPFROMP( list->name )));
    PMASSERT( idx != LIT_ERROR && idx != LIT_MEMERROR );
    PMRASSERT( WinSendMsg( lb, LM_SETITEMHANDLE, MPFROMSHORT( idx ), MPFROMP( list )));   
    if ( list->codepage == selected )
    { PMRASSERT( WinSendMsg( lb, LM_SELECTITEM, MPFROMSHORT( idx ), MPFROMSHORT( TRUE )));
      selected = (ULONG)-1L; // select only the first matching item 
    }   
    ++list;
  }
}

static CH_ENTRY* dlg_query_charset_listbox( HWND lb )
{ SHORT idx = SHORT1FROMMR( WinSendMsg( lb, LM_QUERYSELECTION, MPFROMSHORT( LIT_FIRST ), 0 ));
  if (idx == LIT_NONE)
    return NULL;
  CH_ENTRY* ret = (CH_ENTRY*)PVOIDFROMMR( WinSendMsg( lb, LM_QUERYITEMHANDLE, MPFROMSHORT( idx ), 0 ));
  PMASSERT(ret);
  return ret;
}

static SHORT dlg_set_charset_listbox( HWND lb, ULONG encoding )
{ SHORT idx = SHORT1FROMMR( WinSendMsg( lb, LM_QUERYITEMCOUNT, 0, 0 ));
  while (idx--)
  { CH_ENTRY* ch = (CH_ENTRY*)PVOIDFROMMR( WinSendMsg( lb, LM_QUERYITEMHANDLE, MPFROMSHORT( idx ), 0 ));
    if ( ch->codepage == encoding )
    { PMRASSERT( WinSendMsg( lb, LM_SELECTITEM, MPFROMSHORT( idx ), MPFROMSHORT( TRUE )));
      return idx;
    } 
  }
  return -1;
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      switch (tag_read_type)
      {case TAG_READ_ID3V2_AND_ID3V1:
        WinCheckButton( hwnd, RB_2R_PREFER, TRUE );
        goto both;
       case TAG_READ_ID3V1_AND_ID3V2:
        WinCheckButton( hwnd, RB_1R_PREFER, TRUE );
       both:
        WinEnableControl( hwnd, RB_1R_PREFER, TRUE ); 
        WinEnableControl( hwnd, RB_2R_PREFER, TRUE ); 
        WinCheckButton( hwnd, CB_1R_READ,   TRUE );
        WinCheckButton( hwnd, CB_2R_READ,   TRUE );
        break;
       case TAG_READ_ID3V2_ONLY:
        WinCheckButton( hwnd, CB_2R_READ,   TRUE );
        goto one;
       case TAG_READ_ID3V1_ONLY:
        WinCheckButton( hwnd, CB_1R_READ,   TRUE );
       case TAG_READ_NONE:
       one:
        WinCheckButton( hwnd, RB_2R_PREFER, TRUE );
        break;
      }

      WinCheckButton( hwnd, CB_1R_AUTOENCODING, tag_read_id3v1_autoch );
      WinCheckButton( hwnd, RB_1W_UNCHANGED + tag_save_id3v1_type, TRUE );
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_1_ENCODING ), ch_list, ch_list_size - ch_list_dbcs, tag_id3v1_charset );

      WinCheckButton( hwnd, RB_2W_UNCHANGED + tag_save_id3v2_type, TRUE );
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_2R_ENCODING ), ch_list, ch_list_size - ch_list_dbcs, tag_read_id3v2_charset );
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_2W_ENCODING ), id3v2_ch_list, sizeof id3v2_ch_list / sizeof *id3v2_ch_list, 0 );
      PMRASSERT( lb_select( hwnd, CO_2W_ENCODING, tag_save_id3v2_encoding ));

      do_warpsans( hwnd );
      break;
    }

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case CB_1R_READ:
       case CB_2R_READ:
        switch (SHORT2FROMMP(mp1))
        { BOOL both; 
         case BN_CLICKED:
          both = WinQueryButtonCheckstate( hwnd, CB_1R_READ ) && WinQueryButtonCheckstate( hwnd, CB_2R_READ );
          PMRASSERT( WinEnableControl( hwnd, RB_1R_PREFER, both ));
          PMRASSERT( WinEnableControl( hwnd, RB_2R_PREFER, both ));
          return 0;
        }
        break;
       case RB_1R_PREFER:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
         case BN_DBLCLICKED:
          WinCheckButton( hwnd, RB_2R_PREFER, FALSE );
          WinCheckButton( hwnd, RB_1R_PREFER, TRUE );
          return 0;
        }
       case RB_2R_PREFER:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
         case BN_DBLCLICKED:
          WinCheckButton( hwnd, RB_1R_PREFER, FALSE );
          WinCheckButton( hwnd, RB_2R_PREFER, TRUE );
          return 0;
        }
      }
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ))
      { case DID_OK:
        { CH_ENTRY* chp;
          SHORT val;
          switch( WinQueryButtonCheckstate( hwnd, CB_1R_READ ) + 2*WinQueryButtonCheckstate( hwnd, CB_2R_READ ))
          {case 0:
            tag_read_type = TAG_READ_NONE;
            break;
           case 1:
            tag_read_type = TAG_READ_ID3V1_ONLY;
            break;
           case 2:
            tag_read_type = TAG_READ_ID3V2_ONLY;
            break;
           default:
            tag_read_type = WinQueryButtonCheckstate( hwnd, RB_1R_PREFER ) ? TAG_READ_ID3V1_AND_ID3V2 : TAG_READ_ID3V2_AND_ID3V1;
            break;
          }
          chp = dlg_query_charset_listbox( WinWindowFromID( hwnd, CO_1_ENCODING ));
          if (chp) // Retain old invalid values
            tag_id3v1_charset       = chp->codepage;
          tag_read_id3v1_autoch   = WinQueryButtonCheckstate( hwnd, CB_1R_AUTOENCODING );
          tag_save_id3v1_type     = (save_id3v1_type)(rb_selected( hwnd, RB_1W_UNCHANGED ) - RB_1W_UNCHANGED);
          tag_save_id3v2_type     = (save_id3v2_type)(rb_selected( hwnd, RB_2W_UNCHANGED ) - RB_2W_UNCHANGED);
          chp = dlg_query_charset_listbox( WinWindowFromID( hwnd, CO_2R_ENCODING ));
          if (chp) // Retain old invalid values
            tag_read_id3v2_charset  = chp->codepage;
          val = lb_selected( hwnd, CO_2W_ENCODING, LIT_FIRST );
          if (val != LIT_NONE) // Retain old invalid values
            tag_save_id3v2_encoding = val;

          save_ini();
          init_tag();
        }
        break;
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

/* Working structure of the window procedures below */
typedef struct
{ const ID3V1_TAG* tagv1_old; // NULL = not exists
  const ID3V2_TAG* tagv2_old; // NULL = not exists
  ULONG            encoding_tagv1_old;
  ULONG            encoding_tagv2_old;
  ID3V1_TAG        tagv1;
  ID3V2_TAG*       tagv2;
  ULONG            encoding_tagv1;
  ULONG            encoding_tagv2;
  BOOL             write_tagv1;
  BOOL             write_tagv2;
} ID3_EDIT_TAGINFO;

enum
{ // Load control content from id3_edit_taginfo structure
  UM_LOAD = WM_USER +1, // mp1 = USHORT 0: current, 1: original, 2: clear
  // Store control content to id3_edit_taginfo structure
  UM_STORE,             // -> NULL: OK, != NULL: error message
  // Copy tag info from another tag version.
  UM_COPY,              // no params
  // View error message
  UM_ERROR              // mp1 = error, mp2 = caption 
}; 

/* Insert the predefined genre items into a listbox or combobox. */
static void id3v1_populate_genres( HWND lb )
{
  int i;
  WinSendMsg( lb, LM_INSERTITEM, MPFROMSHORT( LIT_END ), MPFROMP( "" )); // none
  for( i = 0; i <= GENRE_LARGEST; i++)
    WinSendMsg( lb, LM_INSERTITEM, MPFROMSHORT( LIT_END ), MPFROMP( genres[i] ));
}

static void dlg_set_text_if_empty( HWND hwnd, SHORT id, const char* text )
{ HWND ctrl = WinWindowFromID(hwnd, id);
  if ( text && *text && WinQueryWindowTextLength( ctrl ) == 0 )
    PMRASSERT( WinSetWindowText( ctrl, text ));
}

/* Load the data from a V1.x tag into the dialog page. */
static void id3v1_load( HWND hwnd, const ID3V1_TAG* data, ULONG codepage )
{ 
  if (data)
  { char buf[32];
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_TITLE,  id3v1_get_string(data, ID3V1_TITLE,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_ARTIST, id3v1_get_string(data, ID3V1_ARTIST,  buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_ALBUM,  id3v1_get_string(data, ID3V1_ALBUM,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_TRACK,  id3v1_get_string(data, ID3V1_TRACK,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_DATE,   id3v1_get_string(data, ID3V1_YEAR,    buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, CO_GENRE,  id3v1_get_string(data, ID3V1_GENRE,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_COMMENT,id3v1_get_string(data, ID3V1_COMMENT, buf, sizeof buf, codepage));
  }
}

/* Store the data into a V1.x tag. */
static void id3v1_store( HWND hwnd, ID3V1_TAG* data, ULONG codepage )
{ 
  char buf[32];
  WinQueryDlgItemText(hwnd, EN_TITLE,   sizeof buf, buf);
  id3v1_set_string(data, ID3V1_TITLE,   buf, codepage);
  WinQueryDlgItemText(hwnd, EN_ARTIST,  sizeof buf, buf);
  id3v1_set_string(data, ID3V1_ARTIST,  buf, codepage);
  WinQueryDlgItemText(hwnd, EN_ALBUM,   sizeof buf, buf);
  id3v1_set_string(data, ID3V1_ALBUM,   buf, codepage);
  WinQueryDlgItemText(hwnd, EN_TRACK,   sizeof buf, buf);
  id3v1_set_string(data, ID3V1_TRACK,   buf, codepage);
  WinQueryDlgItemText(hwnd, EN_DATE,    sizeof buf, buf);
  id3v1_set_string(data, ID3V1_YEAR,    buf, codepage);
  WinQueryDlgItemText(hwnd, CO_GENRE,   sizeof buf, buf);
  id3v1_set_string(data, ID3V1_GENRE,   buf, codepage);
  WinQueryDlgItemText(hwnd, EN_COMMENT, sizeof buf, buf);
  id3v1_set_string(data, ID3V1_COMMENT, buf, codepage);
}

/*static void id3v2_load_string( ID3V2_TAG* tag, unsigned int id, char* result, ULONG codepage )
{
  ID3V2_FRAME* frame = NULL;

  if(( frame = id3v2_get_frame( tag, id, 1 )) != NULL ) {
    id3v2_get_string_ex( frame, result, size, codepage );
  }
}*/

static void apply_id3v2_string( HWND hwnd, SHORT id, ID3V2_FRAME* frame, size_t applen, ULONG codepage)
{ HWND ctrl;
  ULONG len;
  char buf[256];

  if (!frame)
    return;
  ctrl = WinWindowFromID(hwnd, id);
  len = WinQueryWindowTextLength(ctrl);
  if (len != 0 && len != applen)
    return;
  id3v2_get_string_ex(frame, buf, sizeof buf, codepage);
  if (len)
  { char buf2[32];
    PMRASSERT(WinQueryWindowText(ctrl, sizeof buf2, buf2));
    if (strncmp(buf, buf2, applen))
      return;
  }
  PMRASSERT(WinSetWindowText(ctrl, buf));
}

/* Load the data from a V2.x tag into the dialog page. */
static void id3v2_load( HWND hwnd, const ID3V2_TAG* tag, ULONG codepage )
{
  ID3V2_FRAME* frame;
  char buf[256];
  int  i;

  if (!tag)
    return;

  apply_id3v2_string(hwnd, EN_TITLE,  id3v2_get_frame(tag, ID3V2_TIT2, 1), 30, codepage);
  apply_id3v2_string(hwnd, EN_ARTIST, id3v2_get_frame(tag, ID3V2_TPE1, 1), 30, codepage);
  apply_id3v2_string(hwnd, EN_ALBUM,  id3v2_get_frame(tag, ID3V2_TALB, 1), 30, codepage);
  // Comments
  for( i = 1; (frame = id3v2_get_frame(tag, ID3V2_COMM, i)) != NULL ; i++)
  { char buffer[32];
    id3v2_get_description(frame, buf, sizeof buffer);
    // Skip iTunes specific comment tags.
    if( strnicmp( buffer, "iTun", 4 ) != 0 )
    { apply_id3v2_string(hwnd, EN_COMMENT, frame, WinQueryDlgItemTextLength(hwnd, EN_TRACK) ? 28 : 30, codepage);
      break;
    }
  }
  apply_id3v2_string(hwnd, EN_TRACK,    id3v2_get_frame(tag, ID3V2_TRCK, 1), 0, codepage);
  apply_id3v2_string(hwnd, EN_DATE,     id3v2_get_frame(tag, ID3V2_TDRC, 1), 4, codepage);
  apply_id3v2_string(hwnd, CO_GENRE,    id3v2_get_frame(tag, ID3V2_TCON, 1), 0, codepage);
  apply_id3v2_string(hwnd, EN_COPYRIGHT,id3v2_get_frame(tag, ID3V2_TCOP, 1), 0, codepage);

  /* TODO Track format !!!!
    copy_id3v2_string( tag, ID3V2_TRCK, info->track, sizeof( info->track ));

    // Remove all unwanted track info.
    if(( i = atol( info->track )) != 0 ) {
      snprintf( info->track, sizeof( info->track ), "%02ld", atol( info->track ));
    } else {
      *info->track = 0;
    } */
}

/* Store the data from a V2.x tag. */
static void id3v2_store( HWND hwnd, const ID3V2_TAG* data, uint8_t encoding )
{
}

/* Strip leadion and trailing spaces from the window text */
static void dlg_strip_text( HWND ctrl )
{ char   buf[256];
  size_t len = WinQueryWindowText( ctrl, sizeof buf, buf );
  if (len)
  { char*  cp = buf+len;
    while (cp != buf && isblank(*--cp))
      *cp = 0;
    cp = buf;
    while (cp[0] && isblank(cp[0]))
      ++cp;
    if (buf[len-1] == 0 || cp != buf)
      PMRASSERT(WinSetWindowText(ctrl, cp));
  }
}

/* Strip leading and trailing spaces from all ID3 input fields */
static void id3_strip_all( HWND hwnd )
{ dlg_strip_text(WinWindowFromID(hwnd, EN_TITLE));
  dlg_strip_text(WinWindowFromID(hwnd, EN_ARTIST));
  dlg_strip_text(WinWindowFromID(hwnd, EN_ALBUM));
  dlg_strip_text(WinWindowFromID(hwnd, EN_TRACK));
  dlg_strip_text(WinWindowFromID(hwnd, EN_DATE));
  dlg_strip_text(WinWindowFromID(hwnd, CO_GENRE));
  dlg_strip_text(WinWindowFromID(hwnd, EN_COMMENT));
}

static const char* id3_validatetrack( HWND hwnd, unsigned* track, unsigned* total )
{ HWND ctrl = WinWindowFromID( hwnd, EN_TRACK );
  char   buf[8];
  size_t len = WinQueryWindowText( ctrl, sizeof buf, buf );
  if (len)
  { size_t n;
    if (total)
    { *total = 0;
      if (!sscanf(buf, "%u%n/%u%n", track, &n, total, &n) || n != len)
        return "The track number is not numeric or not in the format '1/2'";
    } else
    { if (!sscanf(buf, "%u%n", track, &n) || n != len)
        return "The track number is not numeric.";
    }
  }
  return NULL;
}

/* Validate ID3 date input.
 * Accepts "", "yyyy", "yyyy-[m]m", "yyyy-[m]m-[d]d", "[d]d.[m]m.yyyy".
 */
static const char* id3_validatedate( HWND hwnd, char* dest )
{ static const char* formats[] = { "%04u", "%04u-%02u", "%04u-%02u-%02u" };
  HWND ctrl = WinWindowFromID( hwnd, EN_DATE );
  char   buf[16];
  size_t len = WinQueryWindowText( ctrl, sizeof buf, buf );
  unsigned y, m, d;
  size_t n = (size_t)-1;
  if (len == 0)
  { if (dest)
      *dest = 0;
    return NULL;
  }
  // syntax
  int cnt = sscanf(buf, "%u%n-%u%n-%u%n", &y, &n, &m, &n, &d, &n);
  if (n != len)
  { // second try with d.m.y
    cnt = sscanf(buf, "%u.%u.%u%n", &d, &m, &y, &n);
    if (n != len)
      return MRFROMP("The date syntax is not valid.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
  }
  // date values
  switch (cnt)
  {case 3:
    if ( d < 1 || d > 31
      || ((((9*m) >> 3) ^ d) & 1)
      || (m == 2 && (d & 3) > !(y & 3)) )
      return MRFROMP("The date does not exist.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
   case 2:
    if (m < 1 || m > 12)
      return MRFROMP("The month is out of range.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
   case 1:
    if (y < 100 || y > 9999)
      return MRFROMP("The release year is invalid. Remeber to use four digits.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
  }
  // write date
  if (dest)
    sprintf(dest, formats[cnt-1], y, m, d);
  //PMRASSERT(WinSetWindowText(ctrl, dest));
  return NULL;
}

/* Processes messages of the dialog of edition of ID3 tag. */
static MRESULT EXPENTRY
id3all_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  DEBUGLOG2(("mpg123:id3all_page_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  {
    case WM_INITDLG:
      id3v1_populate_genres( WinWindowFromID( hwnd, CO_GENRE ));
      break;

    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd )
      { case PB_UNDO:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 1 ), 0 ));
          break;

        case PB_CLEAR:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 2 ), 0 ));
          break;
      }
      return 0;

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
      
    case UM_LOAD:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const ID3V1_TAG* tagv1;
      const ID3V2_TAG* tagv2;
      ULONG encv1;
      ULONG encv2;
      // clear old values to have a clean start
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TITLE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ARTIST,  ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ALBUM,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TRACK,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_DATE,    ""));
      PMRASSERT(WinSetDlgItemText(hwnd, CO_GENRE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COMMENT, ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COPYRIGHT, ""));
      switch (SHORT1FROMMP(mp1)) // mode
      {case 0: // load current
        tagv1 = &data->tagv1;
        encv1 = data->encoding_tagv1;
        tagv2 = data->tagv2;
        encv2 = data->encoding_tagv2;
        break;
       case 1: // load original
        tagv1 = data->tagv1_old;
        encv1 = data->encoding_tagv1_old;
        tagv2 = data->tagv2_old;
        encv2 = data->encoding_tagv2_old;
        break;
       case 2: // reset
        tagv1 = NULL;
        encv1 = tag_id3v1_charset;
        tagv2 = NULL;
        encv2 = tag_read_id3v2_charset;
      }
      switch (tag_read_type)
      {case TAG_READ_ID3V2_AND_ID3V1:
        id3v2_load( hwnd, tagv2, encv2 );
       case TAG_READ_ID3V1_ONLY:
        id3v1_load( hwnd, tagv1, encv1 );
        break;
       case TAG_READ_ID3V1_AND_ID3V2:
        id3v1_load( hwnd, tagv1, encv1 );
       case TAG_READ_ID3V2_ONLY:
        id3v2_load( hwnd, tagv2, encv2 );
       case TAG_READ_NONE:
        break;
      }
      return 0;
    }
    case UM_STORE:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const char* msg;
      unsigned track, total;
      id3_strip_all( hwnd );
      // validate
      msg = id3_validatetrack( hwnd, &track, &total );
      if (msg)
        return MRFROMP(msg);
      msg = id3_validatedate( hwnd, NULL );
      if (msg)
        return MRFROMP(msg);
      // store

      return 0; // OK
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

static MRESULT EXPENTRY
id3v1_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ DEBUGLOG2(("mpg123:id3v1_page_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  { case WM_INITDLG:
      id3v1_populate_genres( WinWindowFromID( hwnd, CO_GENRE ));
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_ENCODING ),  ch_list, ch_list_size - ch_list_dbcs, 0 );
      break;

    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd )
      { case PB_UNDO:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 1 ), 0 ));
          break;

        case PB_CLEAR:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 2 ), 0 ));
          break;

        case PB_COPY:
          PMRASSERT( WinPostMsg( hwnd, UM_COPY, 0, 0 ));
          break;
      }
      return 0;

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
      
    case UM_LOAD:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const ID3V1_TAG* tagv1;
      ULONG encv1;
      // clear old values to have a clean start
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TITLE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ARTIST,  ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ALBUM,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TRACK,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_DATE,    ""));
      PMRASSERT(lb_select(hwnd, CO_GENRE, 0));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COMMENT, ""));
      switch (SHORT1FROMMP(mp1)) // mode
      {case 0: // load current
        tagv1 = &data->tagv1;
        encv1 = data->encoding_tagv1;
        break;
       case 1: // load original
        tagv1 = data->tagv1_old;
        // Do NOT use the old value in this case.
        encv1 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
        break;
       case 2: // reset
        tagv1 = NULL;
        encv1 = tag_id3v1_charset;
      }
      id3v1_load( hwnd, tagv1, encv1 );
      dlg_set_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING), encv1);
      WinCheckButton(hwnd, CB_WRITE, data->write_tagv1);
      return 0;
    }
    case UM_STORE:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const char* msg;
      unsigned track;
      id3_strip_all( hwnd );
      // validate
      msg = id3_validatetrack( hwnd, &track, NULL );
      if (msg)
        return MRFROMP(msg);
      msg = id3_validatedate( hwnd, NULL );
      if (msg)
        return MRFROMP(msg);
      if (track && WinQueryDlgItemTextLength(hwnd, EN_COMMENT) > 28)
        return "The Comment field of ID3V1.1 tags with a track number is restricted to 28 characters.";
      // store
      data->encoding_tagv1 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
      data->write_tagv1 = WinQueryButtonCheckstate(hwnd, CB_WRITE);
      id3v1_store( hwnd, &data->tagv1, data->encoding_tagv1 );
      return 0; // OK
    }
    case UM_COPY:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      id3v2_load( hwnd, data->tagv2, data->encoding_tagv2 );
      // Restrict to Version 1
      return 0;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

static MRESULT EXPENTRY
id3v2_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ DEBUGLOG2(("mpg123:id3v1_page_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  { case WM_INITDLG:
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_ENCODING ), ch_list, ch_list_size - ch_list_dbcs, tag_read_id3v2_charset );
      break;

    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd )
      { case PB_UNDO:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 1 ), 0 ));
          break;

        case PB_CLEAR:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 2 ), 0 ));
          break;

        case PB_COPY:
          PMRASSERT( WinPostMsg( hwnd, UM_COPY, 0, 0 ));
          break;
      }
      return 0;

    case UM_LOAD:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const ID3V2_TAG* tagv2;
      ULONG encv2;
      // clear old values to have a clean start
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TITLE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ARTIST,  ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ALBUM,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TRACK,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_DATE,    ""));
      PMRASSERT(WinSetDlgItemText(hwnd, CO_GENRE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COMMENT, ""));
      switch (SHORT1FROMMP(mp1)) // mode
      {case 0: // load current
        tagv2 = data->tagv2;
        encv2 = data->encoding_tagv2;
        break;
       case 1: // load original
        tagv2 = data->tagv2_old;
        // Do NOT use the old value in this case.
        encv2 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
        break;
       case 2: // reset
        tagv2 = NULL;
        encv2 = tag_read_id3v2_charset;
      }
      id3v2_load( hwnd, tagv2, encv2 );
      dlg_set_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING), encv2);
      WinCheckButton(hwnd, CB_WRITE, data->write_tagv2);
      return 0;
    }
    case UM_STORE:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const char* msg;
      unsigned track, total;
      id3_strip_all( hwnd );
      // validate
      msg = id3_validatetrack( hwnd, &track, &total );
      if (msg)
        return MRFROMP(msg);
      msg = id3_validatedate( hwnd, NULL );
      if (msg)
        return MRFROMP(msg);
      // store
      data->encoding_tagv2 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
      data->write_tagv2 = WinQueryButtonCheckstate(hwnd, CB_WRITE);
      id3v2_store( hwnd, data->tagv2, tag_save_id3v2_encoding );
      return 0; // OK
    }
    case UM_COPY:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      id3v1_load( hwnd, &data->tagv1, data->encoding_tagv1 );
      return 0;
    }
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
    case WM_QUERYTRACKINFO:
    { MRESULT mr = WinDefDlgProc( hwnd, msg, mp1, mp2 );
      TRACKINFO* pti = (TRACKINFO*)mp2;
      if (pti->ptlMinTrackSize.x < 400)
        pti->ptlMinTrackSize.x = 400;
      if (pti->ptlMinTrackSize.y < 400)
        pti->ptlMinTrackSize.y = 400;
      return mr;
    }     
    case WM_WINDOWPOSCHANGED:
    {
      SWP* pswp = (SWP*)mp1;
      if ( (pswp[0].fl & SWP_SIZE) && pswp[1].cx )
        nb_adjust( hwnd, pswp );
      break;
    }
    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      { case NB_ID3TAG:
          if (SHORT2FROMMP(mp1) == BKN_PAGESELECTEDPENDING)
          { PAGESELECTNOTIFY* psn = (PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
            if (psn->ulPageIdCur)
            { HWND nb = (HWND)WinSendDlgItemMsg(hwnd, NB_ID3TAG, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(psn->ulPageIdCur), 0);
              PMASSERT(nb != NULLHANDLE && nb != BOOKERR_INVALID_PARAMETERS);
              const char* msg = (const char*)WinSendMsg(nb, UM_STORE, 0, 0);
              if (msg)
              { psn->ulPageIdNew = NULLHANDLE;
                PMRASSERT(WinPostMsg(hwnd, UM_ERROR, MPFROMP(msg), MPFROMP(NULL)));
                break;
              }
              if (psn->ulPageIdNew) 
              { HWND nb = (HWND)WinSendDlgItemMsg(hwnd, NB_ID3TAG, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(psn->ulPageIdNew), 0);
                PMASSERT(nb != NULLHANDLE && nb != BOOKERR_INVALID_PARAMETERS);
                PMRASSERT(WinPostMsg(nb, UM_LOAD, MPFROMSHORT(0), 0));
              }
            }
          }
          break;
      }
      return 0;
      
    case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      { case DID_OK:
        { // save and validate current page
          HWND nb = WinWindowFromID(hwnd, NB_ID3TAG);
          ULONG id = LONGFROMMR(WinSendMsg(nb, BKM_QUERYPAGEID, 0, MPFROM2SHORT(BKA_TOP, 0)));
          const char* msg;
          PMASSERT(id != NULLHANDLE && id != BOOKERR_INVALID_PARAMETERS);
          nb = (HWND)WinSendMsg(nb, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(id), 0);
          PMASSERT(nb != NULLHANDLE && nb != BOOKERR_INVALID_PARAMETERS);
          msg = (const char*)WinSendMsg(nb, UM_STORE, 0, 0);
          if (msg)
          { PMRASSERT(WinPostMsg(hwnd, UM_ERROR, MPFROMP(msg), MPFROMP(NULL)));
            return 0; // Reject the command
          }
          break;
        }
      }
      break;
      
    case UM_ERROR:
      WinMessageBox( HWND_DESKTOP, hwnd, (PSZ)mp1, (PSZ)mp2, DLG_ID3TAG, MB_CANCEL|MB_ERROR|MB_MOVEABLE );
      return 0;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Edits a ID3 tag for the specified file. */
ULONG DLLENTRY decoder_editmeta( HWND owner, const char* filename )
{
  char    caption[_MAX_FNAME] = "ID3 Tag Editor - ";
  HWND    hwnd;
  HWND    book;
  HWND    page01, page02, page03;
  MRESULT id;
  HMODULE module;
  ULONG   rc;
  DECODER_STRUCT* w;
  ID3_EDIT_TAGINFO workarea;
  ID3V2_FRAME* frame;
  int     i; 

  DEBUGLOG(("mpg123:decoder_editmeta(%p, %s)\n", owner, filename));

  // read tag
  if( decoder_init((void**)(void*)&w ) != PLUGIN_OK ) {
    return PLUGIN_NO_PLAY;
  }

  if(( rc = plg_open_file( w, filename, "rbU" )) != 0 ) {
    decoder_uninit( w );
    return rc == -1 ? PLUGIN_NO_PLAY : PLUGIN_NO_READ;
  }

  // Don't know what this mutex is good for
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  workarea.tagv1_old = &w->mpeg.tagv1;
  workarea.encoding_tagv1_old = tag_read_id3v1_autoch
    ? detect_id3v1_codepage(&w->mpeg.tagv1, tag_id3v1_charset)
    : tag_id3v1_charset;
  workarea.tagv1 = *workarea.tagv1_old;
  workarea.encoding_tagv1 = workarea.encoding_tagv1_old;
  workarea.write_tagv1 = TRUE; // TODO:

  { ID3V2_TAG* tag = id3v2_new_tag();
    // ID3V2: the current tag gets the new one,
    // some of the required information is put into the old_tag for backup purposes.
    workarea.tagv2 = w->mpeg.tagv2;
    workarea.encoding_tagv2_old = tag_read_id3v2_charset;
    if (workarea.tagv2)
    { id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TIT2, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TPE1, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TALB, 1));
      for ( i = 1; (frame = id3v2_get_frame(workarea.tagv2, ID3V2_COMM, i)) != NULL ; i++)
        id3v2_copy_frame(tag, frame);
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TRCK, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TDRC, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TCON, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TCOP, 1));
      workarea.write_tagv2 = TRUE;
    } else
      workarea.write_tagv2 = FALSE;
    workarea.tagv2_old = tag;
    workarea.encoding_tagv2 = workarea.encoding_tagv2_old;
  }

  DosReleaseMutexSem( w->mutex );

  DosQueryModFromEIP( &module, &rc, 0, NULL, &rc, (ULONG)&decoder_editmeta ); 
  hwnd = WinLoadDlg( HWND_DESKTOP, owner, id3_dlg_proc, module, DLG_ID3TAG, 0 );
  DEBUGLOG(("mpg123:decoder_editmeta: WinLoadDlg: %p (%p) - %p\n", hwnd, WinGetLastError(0), module));
  sfnameext( caption + strlen( caption ), filename, sizeof( caption ) - strlen( caption ));
  WinSetWindowText( hwnd, caption );

  book = WinWindowFromID( hwnd, NB_ID3TAG );
  do_warpsans( book );

  //WinSendMsg( book, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_FIELDBACKGROUND),
  //            MPFROMSHORT(BKA_BACKGROUNDPAGECOLORINDEX));

  page01 = WinLoadDlg( book, book, id3all_page_dlg_proc, module, DLG_ID3ALL, 0 );
  do_warpsans( page01 );
  WinSetWindowPtr( page01, QWL_USER, &workarea );
  PMRASSERT( nb_append_tab( book, page01, "All Tags", NULL, 0 ));

  page02 = WinLoadDlg( book, book, id3v1_page_dlg_proc, module, DLG_ID3V1, 0 );
  do_warpsans( page02 );
  WinSetWindowPtr( page02, QWL_USER, &workarea );
  PMRASSERT( nb_append_tab( book, page02, "ID3V1", NULL, 0 ));

  page03 = WinLoadDlg( book, book, id3v2_page_dlg_proc, module, DLG_ID3V2, 0 );
  do_warpsans( page03 );
  WinSetWindowPtr( page03, QWL_USER, &workarea );
  PMRASSERT( nb_append_tab( book, page03, "ID3V2", NULL, 0 ));

  PMRASSERT(WinPostMsg(hwnd, UM_LOAD, MPFROMSHORT(1), 0));
  rc = WinProcessDlg( hwnd );
  DEBUGLOG(("mpg123:decoder_editmeta: dlg completed - %u, %p %p (%p)\n", rc, hwnd, page01, WinGetLastError(0)));

  /*if ( rc == DID_OK )
  { WinQueryDlgItemText( page01, EN_ID3_TITLE,   sizeof( new_tag.title   ), new_tag.title   );
    WinQueryDlgItemText( page01, EN_ID3_ARTIST,  sizeof( new_tag.artist  ), new_tag.artist  );
    WinQueryDlgItemText( page01, EN_ID3_ALBUM,   sizeof( new_tag.album   ), new_tag.album   );
    WinQueryDlgItemText( page01, EN_ID3_COMMENT, sizeof( new_tag.comment ), new_tag.comment );
    WinQueryDlgItemText( page01, EN_ID3_YEAR,    sizeof( new_tag.year    ), new_tag.year    );
    WinQueryDlgItemText( page01, EN_ID3_TRACK,   sizeof( new_tag.track   ), new_tag.track   );
    WinQueryDlgItemText( page01, CB_ID3_GENRE,   sizeof( new_tag.genre   ), new_tag.genre   );
  }*/

  WinDestroyWindow( hwnd   );

  // TODO: cleanup in all cases
  plg_close_file( w );
  decoder_uninit( w );

  /*if ( rc == DID_OK )
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
      return decoder_saveinfo( filename, &new_tag ) ? 500 : 0;
    }
  }*/
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

  init_id3v2_ch_list();
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
