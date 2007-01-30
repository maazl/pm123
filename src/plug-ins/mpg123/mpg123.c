/*
 * Mpeg Layer audio decoder
 * ------------------------
 * Copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

#include "mpg123.h"
#include "mpg123def.h"
#include "tag.h"

#include "genre.h"

#include <debuglog.h>

struct flags flags = { 0 , 0 };

int mmx_enable = 0;
int mmx_use    = 0;

static long outscale      = 32768;
static int  force_8bit    = 0;
static int  force_mono    = 0;
static int  down_sample   = 0;
static int  codepage      = 1004; /* Codepage for ID3 tags, similar to ISO-8859-1 */
static BOOL auto_codepage = TRUE; /* Autodetect codepage.                         */

extern int tabsel_123[2][3][16];
static struct frame fr;

#define modes(i) ( i == 0 ? "Stereo"         : \
                 ( i == 1 ? "Joint-Stereo"   : \
                 ( i == 2 ? "Dual-Channel"   : \
                 ( i == 3 ? "Single-Channel" : "" ))))

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
    save_ini_value( hini, force_mono    );
    save_ini_value( hini, down_sample   );
    save_ini_value( hini, mmx_enable    );
    save_ini_value( hini, codepage      );
    save_ini_value( hini, auto_codepage );
    close_ini( hini );
  }
}

static void
load_ini( void )
{
  HINI hini;
  ULONG cp[4], cp_size;

  force_mono    = 0;
  down_sample   = 0;
  mmx_enable    = 0;
  codepage      = 1004; /* Codepage for ID3 tags, similar to ISO-8859-1 */
  auto_codepage = TRUE; /* Autodetect codepage.                         */

  // Selects russian auto-detect as default characters encoding
  // for russian peoples.
  if( DosQueryCp( sizeof( cp ), cp, &cp_size ) == NO_ERROR ) {
    if( cp[0] == 866 ) {
      codepage = 1251;
    }
  }

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, force_mono    );
    load_ini_value( hini, down_sample   );
    load_ini_value( hini, mmx_enable    );
    load_ini_value( hini, codepage      );
    load_ini_value( hini, auto_codepage );
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
decoder_fileinfo( const char* filename, DECODER_INFO* info )
{
  unsigned long header;
  struct frame fr;
  int rc = 0;

  DECODER_STRUCT w = {0};

  DEBUGLOG(("mpg123:decoder_fileinfo(%s, %p)\n", filename, info));

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
    info->format.channels   = fr.stereo;
    info->mode              = fr.mode;
    info->modext            = fr.mode_ext;
    info->bpf               = fr.framesize + 4;
    info->bitrate           = tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index];
    info->extention         = fr.extension;
    info->junklength        = w.started;
    info->filesize          = xio_fsize( w.file );

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

    // fetch ID3 tags from data
    if( xio_can_seek( w.file ) == XIO_CAN_SEEK_FAST )
    {
      tune tag;
      if ( gettag( w.file, &tag ) ) {
        // Slicing! The common part of the structures is binary compatible
        memcpy( info->title, tag.meta.title, offsetof( META_INFO, track ) - offsetof( META_INFO, title ) );
        sprintf( info->track, "%d", tag.meta.track);
      }
    }
  } else {
    rc = 200;
  }

  xio_fclose( w.file );

  DEBUGLOG(("mpg123:decoder_fileinfo: {{, %d, %d, %d, %d}}, %d, %d,\n\t%i, %i, %i, %i, %i, %i, %i ...}\n",
    info->format.samplerate, info->format.channels, info->format.bits, info->format.format,
    info->songlength, info->junklength,
    info->mpeg, info->layer, info->mode, info->modext, info->bpf, info->bitrate, info->extention)); 

  return rc;
}

ULONG PM123_ENTRY
decoder_trackinfo( const char* drive, int track, DECODER_INFO* info ) {
  return 200;
}

ULONG PM123_ENTRY
decoder_cdinfo( const char* drive, DECODER_CDINFO* info ) {
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


/****************************************************************************
 *
 *  configuration dialog
 *
 ***************************************************************************/

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      int  i;
      BOOL found = FALSE;
      char entry[64];
    
      WinCheckButton( hwnd, CB_FORCEMONO,    force_mono    );
      WinCheckButton( hwnd, CB_DOWNSAMPLE,   down_sample   );
      WinCheckButton( hwnd, CB_USEMMX,       mmx_enable    );
      WinCheckButton( hwnd, CB_AUTO_CHARSET, auto_codepage );

      if( !detect_mmx()) {
        WinEnableWindow( WinWindowFromID( hwnd, CB_USEMMX ), FALSE );
      }

      lb_remove_all( hwnd, CB_CHARSET );
      for( i = 0; i < ch_list_size; i++ ) {
        if ( ch_list[i].codepage != CH_CP_NONE ) {
          int n = sprintf( entry, "%d        ", ch_list[i].codepage );
          entry[29-2*n] = '-';
          entry[31-2*n] = 0;
        } else {
          entry[0] = 0;
        }
        strlcat( entry, ch_list[i].name, sizeof entry );
        lb_add_item( hwnd, CB_CHARSET, entry );
        lb_set_handle( hwnd, CB_CHARSET, i, (PVOID)ch_list[i].codepage );

        if( ch_list[i].codepage == codepage ) {
          lb_select( hwnd, CB_CHARSET, i );
          found = TRUE;
        }
      }
      if ( !found ) {
        char buf[12];
        sprintf( buf, "%d", codepage );
        lb_add_item( hwnd, CB_CHARSET, buf );
        lb_set_handle( hwnd, CB_CHARSET, i, (PVOID)codepage );
        lb_select( hwnd, CB_CHARSET, i );
      }

      do_warpsans( hwnd );
      break;
    }
    case WM_COMMAND:
      if( SHORT1FROMMP( mp1 ) == DID_OK ) {
        int i;

        force_mono    = WinQueryButtonCheckstate( hwnd, CB_FORCEMONO    );
        down_sample   = WinQueryButtonCheckstate( hwnd, CB_DOWNSAMPLE   );
        mmx_enable    = WinQueryButtonCheckstate( hwnd, CB_USEMMX       );
        auto_codepage = WinQueryButtonCheckstate( hwnd, CB_AUTO_CHARSET );

        i = lb_cursored( hwnd, CB_CHARSET );
        if( i != LIT_NONE ) {
          codepage = (int)lb_get_handle( hwnd, CB_CHARSET, i );
        }

        save_ini();
      }
      break;

    /*case CFG_DEFAULT:
          cpe = ch_find( 1004 );
      lb_select( hwnd, CB_CHARSET, cpe ? cpe->codepage : 0 );
      WinCheckButton( hwnd, CB_AUTO_CHARSET,   TRUE );*/

  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
int PM123_ENTRY
plugin_configure( HWND hwnd, HMODULE module )
{
  DEBUGLOG(("mpg123:plugin_configure(%p, %p)\n", hwnd, module));

  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  return 0;
}


/****************************************************************************
 *
 *  ID3 tag editor
 *
 ***************************************************************************/

/* Reads ID3 tag from the specified file. */
BOOL
readtag( const char* filename, tune* tag )
{
  XFILE* file;
  BOOL rc = FALSE;

  emptytag(tag);

  file = xio_fopen( filename, "rb" );
  if( file != NULL ) {
    rc = gettag( file, tag );
    xio_fclose( file );
  }

  if( rc && auto_codepage )
  {
    // autodetect codepage
    char cstr[ sizeof( tag->meta.title   ) +
               sizeof( tag->meta.artist  ) +
               sizeof( tag->meta.album   ) +
               sizeof( tag->meta.comment ) ];
    strcpy( cstr, tag->meta.title   );
    strcat( cstr, tag->meta.artist  );
    strcat( cstr, tag->meta.album   );
    strcat( cstr, tag->meta.comment );

    tag->codepage = ch_detect( codepage, cstr );
  }
  
  if ( rc && tag->codepage != CH_CP_NONE )
  {
    tune stag = *tag;
    // replace values in *tag by codepage converted values
    ch_convert( tag->codepage, stag.meta.title,   CH_CP_NONE, tag->meta.title,   sizeof( stag.meta.title   ));
    ch_convert( tag->codepage, stag.meta.artist,  CH_CP_NONE, tag->meta.artist,  sizeof( stag.meta.artist  ));
    ch_convert( tag->codepage, stag.meta.album,   CH_CP_NONE, tag->meta.album,   sizeof( stag.meta.album   ));
    ch_convert( tag->codepage, stag.meta.comment, CH_CP_NONE, tag->meta.comment, sizeof( stag.meta.comment ));
  }

  return rc;
}

/* Writes ID3 tag to the specified file. */
BOOL
writetag( const char* filename, const tune* tag )
{
  BOOL rc = 0;
  XFILE* file = xio_fopen( filename, "r+b" );
  if( file == NULL )
    return FALSE;

  if ( (*tag->meta.title | *tag->meta.artist  | *tag->meta.album |
       *tag->meta.year  | *tag->meta.comment | *tag->meta.genre) == 0 &&
       tag->meta.track <= 0 )
  { // all fields initial => wipe tag
    rc = wipetag( file ); 
  } else   
  { tune wtag = *tag;
    // if the codepage is not yet specified use the global configuration setting by default
    if( wtag.codepage == CH_CP_NONE )
      wtag.codepage = codepage;
    
    ch_convert( CH_CP_NONE, tag->meta.title,   tag->codepage, wtag.meta.title,   sizeof( wtag.meta.title   ));
    ch_convert( CH_CP_NONE, tag->meta.artist,  tag->codepage, wtag.meta.artist,  sizeof( wtag.meta.artist  ));
    ch_convert( CH_CP_NONE, tag->meta.album,   tag->codepage, wtag.meta.album,   sizeof( wtag.meta.album   ));
    ch_convert( CH_CP_NONE, tag->meta.comment, tag->codepage, wtag.meta.comment, sizeof( wtag.meta.comment ));

    rc = settag( file, &wtag );
  }

  xio_fclose( file );

  return rc;
}

/* local structure to pass information through WinSetWindowPtr */
typedef struct
{
  tune* tag;
  char  track[4];
} tagdata;

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
      break;
    }

    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd )
      { case PB_ID3_UNDO:
        { tagdata* data = (tagdata*)WinQueryWindowPtr( hwnd, 0 );
          int genre = data->tag->gennum;

          // map all unknown to the text "unknown"
          if ( genre < 0 || genre > GENRE_LARGEST )
            genre = -1;

          if ( data->tag->meta.track <= 0 || data->tag->meta.track > 999 )
            data->track[0] = 0;
          else
            sprintf( data->track, "%d", data->tag->meta.track );

          WinSetDlgItemText( hwnd, EN_ID3_TITLE,   data->tag->meta.title   );
          WinSetDlgItemText( hwnd, EN_ID3_ARTIST,  data->tag->meta.artist  );
          WinSetDlgItemText( hwnd, EN_ID3_ALBUM,   data->tag->meta.album   );
          WinSetDlgItemText( hwnd, EN_ID3_TRACK,   data->track             );
          WinSetDlgItemText( hwnd, EN_ID3_COMMENT, data->tag->meta.comment );
          WinSetDlgItemText( hwnd, EN_ID3_YEAR,    data->tag->meta.year    );

          WinSendDlgItemMsg( hwnd, CB_ID3_GENRE, LM_SELECTITEM,
                             MPFROMSHORT( genre+1 ), MPFROMSHORT( TRUE ));
          return 0;
        }
        case PB_ID3_CLEAR:
          WinSetDlgItemText( hwnd, EN_ID3_TITLE,   "" );
          WinSetDlgItemText( hwnd, EN_ID3_ARTIST,  "" );
          WinSetDlgItemText( hwnd, EN_ID3_ALBUM,   "" );
          WinSetDlgItemText( hwnd, EN_ID3_TRACK,   "" );
          WinSetDlgItemText( hwnd, EN_ID3_COMMENT, "" );
          WinSetDlgItemText( hwnd, EN_ID3_YEAR,    "" );
          WinSendDlgItemMsg( hwnd, CB_ID3_GENRE, LM_SELECTITEM,
                             MPFROMSHORT( 0 ), MPFROMSHORT( TRUE ));        
          return 0;
      }
      break;

    case WM_CONTROL:
      if ( SHORT1FROMMP(mp1) == EN_ID3_TRACK && SHORT2FROMMP(mp1) == EN_CHANGE )
      {
        // read the track immediately to verify the syntax
        int tmp_track;
        char buf[4];
        int len1, len2;
        tagdata* data = (tagdata*)WinQueryWindowPtr( hwnd, 0 );

        len1 = WinQueryWindowText( HWNDFROMMP(mp2), sizeof buf, buf );
        if ( len1 != 0 &&                                       // empty is always OK
             ( sscanf( buf, "%u%n", &tmp_track, &len2 ) != 1 || // can't read
               len1      != len2                             || // more input
               tmp_track >= 256 ) )                             // too large
        {
          // bad input, restore last value
          WinSetDlgItemText( hwnd, EN_ID3_TRACK, data->track );
        } else
        {
          // OK, update last value
          strcpy( data->track, buf ); 
        }  
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
ULONG PM123_ENTRY decoder_editmeta( HWND owner, const char* filename )
{
  char    caption[_MAX_FNAME] = "ID3 Tag Editor - ";
  HWND    hwnd;
  HWND    book;
  HWND    page01;
  MRESULT id;
  tune    old_tag;
  tune    new_tag;
  tagdata mywindowdata;
  HMODULE module;
  ULONG   rc;

  DEBUGLOG(("mpg123:decoder_editmeta(%p, %s)\n", owner, filename));

  if( !is_file( filename )) {
    return 400;
  }
  
  if ( is_url( filename )) {
    filename = strchr( filename, ':' ) +4;
  }

  DosQueryModFromEIP( &module, &rc, 0, NULL, &rc, (ULONG)&decoder_editmeta ); 
  // read tag
  emptytag( &old_tag );
  emptytag( &new_tag );

  readtag( filename, &old_tag );
  new_tag.codepage = old_tag.codepage;

  hwnd = WinLoadDlg( HWND_DESKTOP, owner,
                     id3_dlg_proc, module, DLG_ID3TAG, 0 );
  DEBUGLOG(("mpg123:decoder_editmeta: WinLoadDlg: %p (%p) - %p\n", hwnd, WinGetLastError(NULL), module));
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

  mywindowdata.tag = &old_tag;
  WinSetWindowPtr( page01, 0, &mywindowdata );
  WinPostMsg( page01, WM_COMMAND,
              MPFROMSHORT( PB_ID3_UNDO ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));

  rc = WinProcessDlg( hwnd );
  DEBUGLOG(("mpg123:decoder_editmeta: dlg completed - %u, %p %p (%p)\n", rc, hwnd, page01, WinGetLastError(NULL)));

  if ( rc == DID_OK )
  { WinQueryDlgItemText( page01, EN_ID3_TITLE,   sizeof( new_tag.meta.title   ), new_tag.meta.title   );
    WinQueryDlgItemText( page01, EN_ID3_ARTIST,  sizeof( new_tag.meta.artist  ), new_tag.meta.artist  );
    WinQueryDlgItemText( page01, EN_ID3_ALBUM,   sizeof( new_tag.meta.album   ), new_tag.meta.album   );
    WinQueryDlgItemText( page01, EN_ID3_COMMENT, sizeof( new_tag.meta.comment ), new_tag.meta.comment );
    WinQueryDlgItemText( page01, EN_ID3_YEAR,    sizeof( new_tag.meta.year    ), new_tag.meta.year    );

    sscanf( mywindowdata.track, "%u", &new_tag.meta.track );

    new_tag.gennum =
      SHORT1FROMMR( WinSendDlgItemMsg( page01, CB_ID3_GENRE,
                                       LM_QUERYSELECTION, MPFROMSHORT( LIT_CURSOR ), 0 )) -1;
    // keep genres that PM123 does not know
    if ( new_tag.gennum == -1 && old_tag.gennum >= 0 )
      new_tag.gennum = old_tag.gennum;
  }

  WinDestroyWindow( page01 );
  WinDestroyWindow( hwnd   );

  if ( rc == DID_OK )
  { DEBUGLOG(("mpg123:decoder_editmeta: new\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%i, %i\n",
      new_tag.meta.title, new_tag.meta.artist, new_tag.meta.album, new_tag.meta.comment, new_tag.meta.year, new_tag.gennum, new_tag.meta.track));

    if( strcmp( old_tag.meta.title,   new_tag.meta.title   ) != 0 ||
        strcmp( old_tag.meta.artist,  new_tag.meta.artist  ) != 0 ||
        strcmp( old_tag.meta.album,   new_tag.meta.album   ) != 0 ||
        strcmp( old_tag.meta.comment, new_tag.meta.comment ) != 0 ||
        strcmp( old_tag.meta.year,    new_tag.meta.year    ) != 0 ||
        old_tag.meta.track         != new_tag.meta.track          ||
        old_tag.gennum             != new_tag.gennum )
    { // save modified tag
      return writetag( filename, &new_tag ) ? 0 : 500;
    }
  }
  // Cancel or not modified
  return 300;
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

