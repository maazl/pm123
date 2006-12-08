/*
 * Mpeg Layer audio decoder
 * ------------------------
 * Copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <process.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mpg123.h"
#include "mpg123def.h"
#include "output_plug.h"
#include "id3v1.h"

struct flags flags = { 0 , 0 };

int mmx_enable = 0;
int mmx_use    = 0;

static long outscale    = 32768;
static int  force_8bit  = 0;
static int  force_mono  = 0;
static int  down_sample = 0;

extern int tabsel_123[2][3][16];
static struct frame fr;

/* Play a frame readed by read_frame(). (re)initialize audio if necessary. */
static BOOL
play_frame( DECODER_STRUCT* w, struct frame* fr )
{
  int clip;

  if( fr->error_protection ) {
    getbits( 16 ); /* crc */
  }

  clip = (fr->do_layer)( w, fr );

  if( clip == -1 ) {
    return FALSE;
  } else {
    return TRUE;
  }
}

static void
set_synth_functions( struct frame* fr )
{
  typedef int (*func)( real*, int, unsigned char* );
  typedef int (*func_mono)( real*, unsigned char* );

  static func funcs[2][3] = {
    { synth_1to1, synth_2to1, synth_4to1 },
    { synth_1to1_8bit, synth_2to1_8bit, synth_4to1_8bit }
  };

  static func_mono funcs_mono[2][2][3] = {
    {{ synth_1to1_mono2stereo,
       synth_2to1_mono2stereo,
       synth_4to1_mono2stereo },
     { synth_1to1_8bit_mono2stereo,
       synth_2to1_8bit_mono2stereo,
       synth_4to1_8bit_mono2stereo }},
    {{ synth_1to1_mono,
       synth_2to1_mono,
       synth_4to1_mono },
     { synth_1to1_8bit_mono,
       synth_2to1_8bit_mono,
      synth_4to1_8bit_mono }}
  };

  fr->synth      = funcs[force_8bit][fr->down_sample];
  fr->synth_mono = funcs_mono[force_mono][force_8bit][fr->down_sample];
  fr->block_size = 128 >> (force_mono+force_8bit+fr->down_sample);
}

static void
save_ini( void )
{
  HINI hini;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value( hini, force_mono  );
    save_ini_value( hini, down_sample );
    save_ini_value( hini, mmx_enable  );
    close_ini( hini );
  }
}

static void
load_ini( void )
{
  HINI hini;

  force_mono  = 0;
  down_sample = 0;
  mmx_enable  = 0;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, force_mono  );
    load_ini_value( hini, down_sample );
    load_ini_value( hini, mmx_enable  );
    close_ini( hini );
  }
}

/* Decoding thread. */
static void TFNENTRY
decoder_thread( void* arg )
{
  ULONG resetcount;
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;

  w->frew = FALSE;
  w->ffwd = FALSE;
  w->stop = FALSE;

  if(!( w->file = xio_fopen( w->filename, "rb" ))) {
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

  // After opening a new file we so are in its beginning.
  if( w->jumptopos == 0 ) {
      w->jumptopos = -1;
  }

  if(!( pcm_sample = (unsigned char*)malloc( w->audio_buffersize ))) {
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  } else {
    pcm_point = 0;
  }

  xio_set_observer( w->file, w->hwnd, w->metadata_buff, w->metadata_size );

  if( !read_initialize( w, &fr )) {
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  clear_decoder();
  w->output_format.samplerate = freqs[ fr.sampling_frequency ] >> fr.down_sample;
  init_eq( w->output_format.samplerate );

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
      if( w->jumptopos >= 0 )
      {
        int jumptobyte;

        if(( w->xing_header.flags & FRAMES_FLAG ) &&
           ( w->xing_header.flags & TOC_FLAG    ) &&
           ( w->xing_header.flags & BYTES_FLAG  ))
        {
          jumptobyte = SeekPoint( w->xing_TOC, w->xing_header.bytes,
                                 (float)w->jumptopos * 100 / ( 1000.0 * 8 * 144 * w->xing_header.frames / freqs[fr.sampling_frequency] ));
        }
        else if(( w->xing_header.flags & FRAMES_FLAG ) &&
                ( w->xing_header.flags & BYTES_FLAG  ))
        {
          int songlength = 8.0 * 144 * w->xing_header.frames * 1000 / freqs[fr.sampling_frequency];
          jumptobyte = (float)w->jumptopos * w->xing_header.bytes / songlength;
        } else {
          jumptobyte = (float)w->jumptopos * w->abr * 125 / 1000;
        }

        jumptobyte += w->started;

        clear_decoder();
        seekto_pos( w, &fr, jumptobyte );
        w->jumptopos = -1;
        WinPostMsg( w->hwnd, WM_SEEKSTOP, 0, 0 );
      }

      if( w->frew ) {
        /* ie.: ~1/4 second */
        int bytes = xio_ftell( w->file ) - (tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125)/4;
        if( bytes < 0 ) {
          break;
        } else {
          seekto_pos( w, &fr, bytes );
        }
      }

      if( w->ffwd ) {
        int bytes = xio_ftell( w->file ) + (tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125)/4;
        seekto_pos( w, &fr, bytes );
      }

      if( !read_frame( w, &fr )) {
        break;
      }

      if( !play_frame( w, &fr )) {
        WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
        break;
      }
    }
    output_flush( w, &fr );
    WinPostMsg( w->hwnd, WM_PLAYSTOP, 0, 0 );
    w->status = DECODER_STOPPED;
  }

end:

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( w->file ) {
    xio_fclose( w->file );
    w->file = NULL;
  }
  if( w->save ) {
    fclose( w->save );
    w->save = NULL;
  }
  if( pcm_sample ) {
    free( pcm_sample );
    pcm_sample = NULL;
  }

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
  w->xing_header.toc = w->xing_TOC;
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

  switch( msg )
  {
    case DECODER_SETUP:
      mmx_use        = mmx_enable;
      fr.down_sample = down_sample;
      fr.synth       = synth_1to1;

      w->output_format.size   = sizeof( w->output_format );
      w->output_format.format = WAVE_FORMAT_PCM;
      w->output_format.bits   = 16;

      make_decode_tables( outscale );
      init_layer2( fr.down_sample ); // also shared tables with layer1.
      init_layer3( fr.down_sample );

      if( force_mono ) {
        // Left and right single mix.
        fr.single = 3;
        w->output_format.channels = 1;
      } else {
        // Both channels.
        fr.single = -1;
        w->output_format.channels = 2;
      }

      set_synth_functions( &fr );

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

    case DECODER_EQ:
      if(( flags.mp3_eq = flags.equalizer = info->equalizer ) == 1 ) {
        memcpy( &octave_eq, info->bandgain, sizeof( octave_eq ));
      }
      if( w->status == DECODER_PLAYING ) {
        init_eq( w->output_format.samplerate );
      }
      break;

    case DECODER_SAVEDATA:
      if( info->save_filename == NULL ) {
        w->savename[0] = 0;
      } else {
        strlcpy( w->savename, info->save_filename, sizeof( w->savename ));
      }
      break;

    default:
      if( w->error_display ) {
          w->error_display( "Unknown command." );
      }
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

  _control87( EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE |
              EM_INEXACT  | EM_INVALID   | EM_DENORMAL, MCW_EM );

  if( w->file ) {
    if( w->xing_header.flags & FRAMES_FLAG ) {
      return (float)8 * 144 * w->xing_header.frames * 1000 / freqs[fr.sampling_frequency];
    } else {
      return (float)( xio_fsize( w->file ) - w->started ) * 1000 / w->abr / 125;
    }
  }
  return 0;
}

ULONG PM123_ENTRY
decoder_fileinfo( char* filename, DECODER_INFO* info )
{
  unsigned long header;
  struct frame fr;
  int rc = 0;

  DECODER_STRUCT w = {0};

  memset( info, 0, sizeof( *info ));
  info->size = sizeof( *info );

  if( !( w.file = xio_fopen( filename, "rb" ))) {
    return xio_errno();
  }

  if( read_first_frame_header( &w, &fr, &header ))
  {
    info->format.size       = sizeof( info->format );
    info->format.format     = WAVE_FORMAT_PCM;
    info->format.bits       = 16;
    info->mpeg              = fr.mpeg25 ? 25 : ( fr.lsf ? 20 : 10 );
    info->layer             = fr.lay;
    info->format.samplerate = freqs[fr.sampling_frequency];
    info->mode              = fr.mode;
    info->modext            = fr.mode_ext;
    info->bpf               = fr.framesize + 4;
    info->bitrate           = tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index];
    info->extention         = fr.extension;
    info->junklength        = w.started;
    info->filesize          = xio_fsize( w.file );

    if( force_mono ) {
      // Left and right single mix.
      info->format.channels = 1;
    } else {
      // Both channels.
      info->format.channels = 2;
    }

    if( w.xing_header.flags & FRAMES_FLAG )
    {
      info->songlength = 8.0 * 144 * w.xing_header.frames * 1000 / ( freqs[fr.sampling_frequency] << fr.lsf );

      if( w.xing_header.flags & BYTES_FLAG ) {
        info->bitrate = (float)w.xing_header.bytes / info->songlength * 1000 / 125;
      }
    }
    else
    {
      if( !info->bitrate ) {
        rc = 200;
      } else {
        info->songlength = (float)( xio_fsize( w.file ) - w.started ) * 1000 / ( info->bitrate * 125 );
      }
    }

    sprintf( info->tech_info, "%u kbs, %.1f kHz, %s",
             info->bitrate, ( info->format.samplerate / 1000.0 ), modes( info->mode ));

    if( w.xing_header.flags & TOC_FLAG ) {
      strcat( info->tech_info, ", Xing" );
    }

    xio_get_metainfo( w.file, XIO_META_GENRE, info->genre,   sizeof( info->genre   ));
    xio_get_metainfo( w.file, XIO_META_NAME,  info->comment, sizeof( info->comment ));
    xio_get_metainfo( w.file, XIO_META_TITLE, info->title,   sizeof( info->title   ));

    if( xio_can_seek( w.file ) == XIO_CAN_SEEK_FAST )
    {
      ID3V1_TAG tag;

      if( id3v1_gettag( w.file, &tag ) == 0 )
      {
        id3v1_getstring( &tag, ID3V1_TITLE,   info->title,   sizeof( info->title   ));
        id3v1_getstring( &tag, ID3V1_ARTIST,  info->artist,  sizeof( info->artist  ));
        id3v1_getstring( &tag, ID3V1_ALBUM,   info->album,   sizeof( info->album   ));
        id3v1_getstring( &tag, ID3V1_YEAR,    info->year,    sizeof( info->year    ));
        id3v1_getstring( &tag, ID3V1_COMMENT, info->comment, sizeof( info->comment ));
        id3v1_getstring( &tag, ID3V1_GENRE,   info->genre,   sizeof( info->genre   ));
        id3v1_getstring( &tag, ID3V1_TRACK,   info->track,   sizeof( info->track   ));
      }

      info->haveinfo = DECODER_HAVE_TITLE   |
                       DECODER_HAVE_ARTIST  |
                       DECODER_HAVE_ALBUM   |
                       DECODER_HAVE_YEAR    |
                       DECODER_HAVE_GENRE   |
                       DECODER_HAVE_TRACK   |
                       DECODER_HAVE_COMMENT ;

      info->saveinfo = 1;
    }
  } else {
    rc = 200;
  }

  xio_fclose( w.file );
  return rc;
}

ULONG PM123_ENTRY
decoder_saveinfo( char* filename, DECODER_INFO* info )
{
  ID3V1_TAG tag;
  XFILE* file = xio_fopen( filename, "r+b" );

  if( !file ) {
    return xio_errno();
  }

  id3v1_clrtag   ( &tag );
  id3v1_setstring( &tag, ID3V1_TITLE,   info->title   );
  id3v1_setstring( &tag, ID3V1_ARTIST,  info->artist  );
  id3v1_setstring( &tag, ID3V1_ALBUM,   info->album   );
  id3v1_setstring( &tag, ID3V1_YEAR,    info->year    );
  id3v1_setstring( &tag, ID3V1_COMMENT, info->comment );
  id3v1_setstring( &tag, ID3V1_GENRE,   info->genre   );
  id3v1_setstring( &tag, ID3V1_TRACK,   info->track   );

  if( id3v1_settag( file, &tag ) != 0 ) {
    xio_fclose( file );
    return xio_errno();
  }

  xio_fclose( file );
  return 0;
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
    if( ext != NULL && *size >= 3 )
    {
      strcpy( ext[0], "*.MP1" );
      strcpy( ext[1], "*.MP2" );
      strcpy( ext[2], "*.MP3" );
    }
    *size = 3;
  }

  return DECODER_FILENAME | DECODER_URL | DECODER_METAINFO;
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
      WinSendDlgItemMsg( hwnd, CB_FORCEMONO,  BM_SETCHECK, MPFROMSHORT( force_mono  ), 0 );
      WinSendDlgItemMsg( hwnd, CB_DOWNSAMPLE, BM_SETCHECK, MPFROMSHORT( down_sample ), 0 );
      WinSendDlgItemMsg( hwnd, CB_USEMMX,     BM_SETCHECK, MPFROMSHORT( mmx_enable  ), 0 );

      if( !detect_mmx()) {
        WinEnableWindow( WinWindowFromID( hwnd, CB_USEMMX ), FALSE );
      }

      do_warpsans( hwnd );
      break;

    case WM_COMMAND:
      if( SHORT1FROMMP( mp1 ) == DID_OK ) {
        force_mono  = (BOOL)WinSendDlgItemMsg( hwnd, CB_FORCEMONO,  BM_QUERYCHECK, 0, 0 );
        down_sample = (BOOL)WinSendDlgItemMsg( hwnd, CB_DOWNSAMPLE, BM_QUERYCHECK, 0, 0 );
        mmx_enable  = (BOOL)WinSendDlgItemMsg( hwnd, CB_USEMMX,     BM_QUERYCHECK, 0, 0 );
        save_ini();
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
int PM123_ENTRY
plugin_configure( HWND hwnd, HMODULE module )
{
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  return 0;
}

/* Returns information about plug-in. */
int PM123_ENTRY
plugin_query( PLUGIN_QUERYPARAM* param )
{
   param->type         = PLUGIN_DECODER;
   param->author       = "Samuel Audet";
   param->desc         = "MP3 Decoder 1.22";
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
