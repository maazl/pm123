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
#include "dialog.h"

#include <charset.h>
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


static const PLUGIN_CONTEXT* context;

#define load_prf_value(var) \
  context->plugin_api->profile_query(#var, &var, sizeof var)

#define save_prf_value(var) \
  context->plugin_api->profile_write(#var, &var, sizeof var)

static DECODER_STRUCT** instances = NULL;
static int  instances_count = 0;
static HMTX mutex;

read_type       tag_read_type          = TAG_READ_ID3V2_AND_ID3V1;
ULONG           tag_id3v1_charset      = 1004; // ISO8859-1
BOOL            tag_read_id3v1_autoch  = TRUE;
save_id3v1_type tag_save_id3v1_type    = TAG_SAVE_ID3V1_WRITE;
save_id3v2_type tag_save_id3v2_type    = TAG_SAVE_ID3V2_WRITE;
ULONG           tag_read_id3v2_charset = 1004; // ISO8859-1
uint8_t         tag_save_id3v2_encoding= ID3V2_ENCODING_UTF8;


#define modes(i) ( i == 0 ? "Stereo"         : \
                 ( i == 1 ? "Joint-Stereo"   : \
                 ( i == 2 ? "Dual-Channel"   : \
                 ( i == 3 ? "Single-Channel" : "" ))))


/* Open MPEG file. Returns 0 if it successfully opens the file.
   A nonzero return value indicates an error. A -1 return value
   indicates an unsupported format of the file. */
int
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
void
plg_close_file( DECODER_STRUCT* w )
{
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
  mpg_close( &w->mpeg );
  DosReleaseMutexSem( w->mutex );
}

/* Update the tags of the open file to tagv1 and tagv2.
   If either tagv1 or tagv2 is NULL the corresponding tag is untouched.
   If you pass PLG_DELETE_ID3Vx the tag is explicitely removed.
   The function returns 0 on succes orr errno on error. Additionally the
   pointer errmsg is set to an error message (static storage) if not NULL.
   When the operation cannot work in place, the function returns a temporary
   filename in savename. This temporary file contains a copy of the file with
   the new tag information. You must replace the existing file by this one
   on your own. If the operation completed in place *savename is empty. */ 
int plg_update_tags( DECODER_STRUCT* w, ID3V1_TAG* tagv1, ID3V2_TAG* tagv2,
                       char (*savename)[_MAX_PATH], const char** errmsg )
{ int  rc = 0;
  BOOL copy = FALSE;
  XFILE* save = NULL;

  strlcpy( *savename, w->filename, sizeof( *savename ));
  (*savename)[ strlen( *savename ) - 1 ] = '~';

  if (tagv2)
  { if (tagv2 != PLG_DELETE_ID3V2)
    { int set_rc = id3v2_set_tag( w->mpeg.file, tagv2, *savename );
      if( set_rc == 1 ) {
        copy = TRUE;
      } else if( set_rc == -1 ) {
        if (errmsg)
          *errmsg = "Failed to update ID3v2 tag.";
        return xio_errno();
      }
    } else
    { if( id3v2_wipe_tag( w->mpeg.file, *savename ) == 0 ) {
        copy = TRUE;
      } else {
        if (errmsg)
          *errmsg = "Failed to remove ID3v2 tag.";
        return xio_errno();
      }
    }
  }

  if( copy ) {
    if(( save = xio_fopen( *savename, "r+bU" )) == NULL ) {
      if (errmsg)
        *errmsg = "Failed to open temporary file.";
      return xio_errno();
    }
  } else {
    save = w->mpeg.file;
    **savename = 0;
  }

  if (tagv1)
  { if (tagv1 != PLG_DELETE_ID3V1)
    { id3v1_make_tag_valid( tagv1 );
      if( id3v1_set_tag( save, tagv1 ) != 0 ) {
        if (errmsg)
          *errmsg = "Failed to write ID3v1 tag.";
        rc = xio_errno();
      }
    } else
    { if( id3v1_wipe_tag( save ) != 0 ) {
        if (errmsg)
          *errmsg = "Failed to remove ID3v1 tag.";
        rc = xio_errno();
      }
    }
  }

  if( copy ) {
    xio_fclose( save );
  }

  return rc;
}

/* Replace file dstfile by srcfile even if dstfile is currently in use by
   an active decoder instance. The replacement will also reposition the file pointer
   of the active decoder instances if the header length changed.
   The function returns 0 on succes orr errno on error. Additionally the
   pointer errmsg is set to an error message (static storage) if not NULL. */ 
int plg_replace_file( const char* srcfile, const char* dstfile, const char** errmsg )
{
  long* resumepoints;
  int   i;
  int   newstart;
  int   rc = 0;
  DECODER_STRUCT* w;
  
  if( decoder_init((void**)(void*)&w ) != PLUGIN_OK ) {
    return errno;
  }
  rc = plg_open_file( w, srcfile, "rbU" );
  if( rc != 0 ) {
    decoder_uninit( w );
    if (errmsg)
      *errmsg = "Failed to analyze temporary file.";
    return rc;
  }
  // Remember a new position of the beginning of the data stream.
  newstart = w->mpeg.started;
  plg_close_file( w );
  decoder_uninit( w );

  // Suspend all active instances of the updated file.
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );

  resumepoints = alloca( instances_count * sizeof *resumepoints );
  memset(resumepoints, -1, instances_count * sizeof *resumepoints );

  for( i = 0; i < instances_count; i++ )
  { w = instances[i];
    DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

    if( w->mpeg.file && stricmp( w->filename, dstfile ) == 0 ) {
      DEBUGLOG(( "mpg123:plg_replace_file: suspend currently used file: %s\n", w->filename ));
      resumepoints[i] = xio_ftell( w->mpeg.file ) - w->mpeg.started;
      xio_fclose( w->mpeg.file );
    } else {
      DosReleaseMutexSem( w->mutex );
    }
  }

  // Replace file.
  if( remove( dstfile ) == 0 ) {
    DEBUGLOG(("mpg123:plg_replace_file: deleted %s, replacing by %s\n", dstfile, srcfile));
    if( rename( srcfile, dstfile ) != 0 ) {
      if (errmsg)
        *errmsg = "Critical error! Failed to rename temporary file.";
      rc = errno;
    }
  } else {
    rc = errno;
    if (errmsg)
      *errmsg = "Failed to delete old file.";
    DEBUGLOG(("mpg123:plg_replace_file: failed to delete %s (rc = %i), rollback %s\n", dstfile, errno, srcfile));
    remove( srcfile );
  }

  // Resume all suspended instances of the updated file.
  for( i = 0; i < instances_count; i++ )
  { if (resumepoints[i] != -1)
    { w = instances[i];
      DEBUGLOG(( "mpg123:plg_replace_file: resumes currently used file: %s\n", w->filename ));
      if(( w->mpeg.file = xio_fopen( w->mpeg.filename, "rbXU" )) != NULL ) {
        xio_fseek( w->mpeg.file, resumepoints[i] + newstart, XIO_SEEK_SET  );
        w->mpeg.started = newstart;
      }
      DosReleaseMutexSem( w->mutex );
    }
  }

  DosReleaseMutexSem( mutex );
  return rc;
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

ULONG detect_id3v1_codepage( const ID3V1_TAG* tag, ULONG codepage )
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
copy_id3v1_string( const ID3V1_TAG* tag, int id, char* result, int size, ULONG codepage )
{
  if( !*result ) {
    id3v1_get_string( tag, id, result, size, codepage );
  }
}

void static
copy_id3v1_tag( DECODER_INFO* info, const ID3V1_TAG* tag )
{ ULONG codepage = tag_read_id3v1_autoch
    ? detect_id3v1_codepage( tag, tag_id3v1_charset )
    : tag_id3v1_charset;
    
  copy_id3v1_string( tag, ID3V1_TITLE,   info->title,   sizeof( info->title   ), codepage);
  copy_id3v1_string( tag, ID3V1_ARTIST,  info->artist,  sizeof( info->artist  ), codepage);
  copy_id3v1_string( tag, ID3V1_ALBUM,   info->album,   sizeof( info->album   ), codepage);
  copy_id3v1_string( tag, ID3V1_YEAR,    info->year,    sizeof( info->year    ), codepage);
  copy_id3v1_string( tag, ID3V1_COMMENT, info->comment, sizeof( info->comment ), codepage);
  copy_id3v1_string( tag, ID3V1_GENRE,   info->genre,   sizeof( info->genre   ), codepage);

  if( info->size >= INFO_SIZE_2 ) {
    copy_id3v1_string( tag, ID3V1_TRACK, info->track,   sizeof( info->track   ), codepage);
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

  // update frame
  if ( string == NULL || *string == 0 )
  { if (frame)
      id3v2_delete_frame( frame );
  } else
  { if( frame == NULL )
      frame = id3v2_add_frame( tag, id );
    id3v2_set_string( frame, string, tag_save_id3v2_encoding );
  }
}

BOOL ascii_check(const char* str)
{ while (*str)
  { if (*str < 0x20 || *str > 0x7E)
      return FALSE;
    ++str;
  }
  return TRUE;
}

ULONG DLLENTRY
decoder_saveinfo( const char* filename, const DECODER_INFO* info )
{
  char   savename[_MAX_PATH];
  ULONG  rc;
  int    i;
  ID3V1_TAG* tagv1;
  ID3V2_TAG* tagv2;
  BOOL   have_tagv2;

  DECODER_STRUCT* w;

  if( !*filename ) {
    return EINVAL;
  }

  strlcpy( savename, filename, sizeof( savename ));
  savename[ strlen( savename ) - 1 ] = '~';

  if( decoder_init((void**)(void*)&w ) != PLUGIN_OK ) {
    return errno;
  }

  rc = plg_open_file( w, filename, "r+bU" );
  if( rc != 0 ) {
    DEBUGLOG(( "mpg123: unable open file for saving %s, rc=%d\n", filename, rc ));
    decoder_uninit( w );
    return rc;
  }
  
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  have_tagv2 = w->mpeg.tagv2 != NULL;
  if (!have_tagv2)
    w->mpeg.tagv2 = id3v2_new_tag();
  tagv2 = w->mpeg.tagv2;

  // Apply changes to the tags. Note that they are not neccessarily written.
  if (info->haveinfo & DECODER_HAVE_TITLE)
  { replace_id3v2_string( tagv2, ID3V2_TIT2, info->title);
    id3v1_set_string( &w->mpeg.tagv1, ID3V1_TITLE,  info->title,  tag_id3v1_charset);
  }
  if (info->haveinfo & DECODER_HAVE_ARTIST)
  { replace_id3v2_string( tagv2, ID3V2_TPE1, info->artist);
    id3v1_set_string( &w->mpeg.tagv1, ID3V1_ARTIST, info->artist, tag_id3v1_charset);
  }
  if (info->haveinfo & DECODER_HAVE_ALBUM)
  { replace_id3v2_string( tagv2, ID3V2_TALB, info->album);
    id3v1_set_string( &w->mpeg.tagv1, ID3V1_ALBUM,  info->album,  tag_id3v1_charset);
  }
  if (info->haveinfo & DECODER_HAVE_YEAR)
  { replace_id3v2_string( tagv2, ID3V2_TDRC, info->year);
    id3v1_set_string( &w->mpeg.tagv1, ID3V1_YEAR,   info->year,   tag_id3v1_charset);
  }
  if (info->haveinfo & DECODER_HAVE_GENRE)
  { replace_id3v2_string( tagv2, ID3V2_TCON, info->genre);
    id3v1_set_string( &w->mpeg.tagv1, ID3V1_GENRE,  info->genre,  tag_id3v1_charset);
  }
  if (info->haveinfo & DECODER_HAVE_COPYRIGHT)
  { replace_id3v2_string( tagv2, ID3V2_TCOP, info->copyright);
  }
  if (info->haveinfo & DECODER_HAVE_TRACK)
  { replace_id3v2_string( tagv2, ID3V2_TRCK, info->track);
    id3v1_set_string( &w->mpeg.tagv1, ID3V1_TRACK,  info->track,  tag_id3v1_charset);
  }
  // FIX ME: Replay gain info also must be saved.

  // Determine save mode
  if ( tagv2->id3_started == 0 ) {
    switch (tag_save_id3v2_type)
    { DECODER_INFO new_tag_info;
      case TAG_SAVE_ID3V2_ONDEMANDSPC:
        copy_id3v2_tag(&new_tag_info, tagv2);
        if ( ascii_check(new_tag_info.title  )
          && ascii_check(new_tag_info.artist )
          && ascii_check(new_tag_info.album  )
          && ascii_check(new_tag_info.comment) )
        goto cont;
      write:
        if (!tagv2->id3_altered)
          tagv2 = NULL;
        have_tagv2 = TRUE;
        break;

      case TAG_SAVE_ID3V2_ONDEMAND:
        copy_id3v2_tag(&new_tag_info, tagv2);
      cont:
        if ( strlen(new_tag_info.title  ) > 30
          || strlen(new_tag_info.artist ) > 30
          || strlen(new_tag_info.album  ) > 30
          || strlen(new_tag_info.year   ) > 4
          || (*new_tag_info.genre && id3v1_get_genre(new_tag_info.genre) == -1)
          || *new_tag_info.copyright
          || strchr(new_tag_info.track, '/')
          || (i = strlen(new_tag_info.comment)) > 30
          || (i > 28 && *new_tag_info.track) )
          goto write;
        // Purge ID3v2 metadata
        replace_id3v2_string( tagv2, ID3V2_TIT2, NULL );
        replace_id3v2_string( tagv2, ID3V2_TPE1, NULL );
        replace_id3v2_string( tagv2, ID3V2_TALB, NULL );
        replace_id3v2_string( tagv2, ID3V2_TDRC, NULL );
        replace_id3v2_string( tagv2, ID3V2_COMM, NULL );
        replace_id3v2_string( tagv2, ID3V2_TCON, NULL );
        replace_id3v2_string( tagv2, ID3V2_TCOP, NULL );
        replace_id3v2_string( tagv2, ID3V2_TRCK, NULL );
      case TAG_SAVE_ID3V2_WRITE:
        if ( tagv2->id3_frames_count )
          goto write;
      case TAG_SAVE_ID3V2_DELETE:
        if ( have_tagv2 )
        { tagv2 = PLG_DELETE_ID3V2;
          have_tagv2 = FALSE;
          break;
        }
      default: // case TAG_SAVE_ID3V2_UNCHANGED:
        tagv2 = NULL;
        have_tagv2 = FALSE;
    }
  }

  switch (tag_save_id3v1_type)
  { case TAG_SAVE_ID3V1_NOID3V2:
      if (have_tagv2)
        goto del; 
    case TAG_SAVE_ID3V1_WRITE:
      if (!id3v1_is_clean_tag( &w->mpeg.tagv1 ))
      { tagv1 = &w->mpeg.tagv1;
        break;
      }
    case TAG_SAVE_ID3V1_DELETE:
    del:
      if (id3v1_is_tag_valid( &w->mpeg.tagv1 ))
      { tagv1 = PLG_DELETE_ID3V1;
        break;
      }
    default: // case TAG_SAVE_ID3V1_UNCHANGED:
      tagv1 = NULL;    
  }

  // Now start the transaction.
  rc = plg_update_tags( w, tagv1, tagv2, &savename, NULL );

  plg_close_file( w );
  DosReleaseMutexSem( w->mutex );
  decoder_uninit( w );

  if( rc != 0 || *savename == 0 ) {
    return rc;
  }
  // Must replace the file.
  eacopy( filename, savename );
  return plg_replace_file( savename, filename, NULL );
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

void
save_ini( void )
{
  save_prf_value( tag_read_type           );
  save_prf_value( tag_id3v1_charset       );
  save_prf_value( tag_read_id3v1_autoch   );
  save_prf_value( tag_save_id3v1_type     );
  save_prf_value( tag_save_id3v2_type     );
  save_prf_value( tag_read_id3v2_charset  );
  save_prf_value( tag_save_id3v2_encoding );
}

static void
load_ini( void )
{
  tag_read_type          = TAG_READ_ID3V2_AND_ID3V1;
  tag_id3v1_charset      = 1004;  // ISO8859-1
  tag_read_id3v1_autoch  = TRUE;
  tag_save_id3v1_type    = TAG_SAVE_ID3V1_WRITE;
  tag_save_id3v2_type    = TAG_SAVE_ID3V2_WRITE;
  tag_read_id3v2_charset = 1004;  // ISO8859-1
  tag_save_id3v2_encoding= ID3V2_ENCODING_UTF8;

  load_prf_value( tag_read_type           );
  load_prf_value( tag_id3v1_charset       );
  load_prf_value( tag_read_id3v1_autoch   );
  load_prf_value( tag_save_id3v1_type     );
  load_prf_value( tag_save_id3v2_type     );
  load_prf_value( tag_read_id3v2_charset  );
  load_prf_value( tag_save_id3v2_encoding );
}

/* Returns information about plug-in. */
int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM* param )
{
  param->type         = PLUGIN_DECODER;
  param->author       = "Samuel Audet, Dmitry A.Steklenev";
  param->desc         = "MP3 Decoder 1.25";
  param->configurable = TRUE;
  return 0;
}

/* init plug-in */
int DLLENTRY
plugin_init( const PLUGIN_CONTEXT* ctx )
{
  context = ctx;
  dialog_init();
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
