/*
 * Mpeg Layer audio decoder
 * ------------------------
 * Copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mpg123.h"
#include "mpg123def.h"
#include "utilfct.h"

struct flags flags = { 0 , 0 };

int mmx_enable = 0;
int mmx_use    = 0;

static long outscale    = 32768;
static int  force_8bit  = 0;
static int  force_mono  = 0;
static int  down_sample = 0;
static BOOL mmx_present = FALSE;

static struct frame fr;

extern int  _CRT_init( void );
extern void _CRT_term( void );
extern int  tabsel_123[2][3][16];

BOOL _System
_DLL_InitTerm( ULONG hModule, ULONG flag )
{
  if( flag == 0 )
  {
    mmx_present = detect_mmx();
    if( _CRT_init() == -1 ) {
        return FALSE;
    }
  } else {
    _CRT_term();
  }

  return TRUE;
}

/* Play a frame read read_frame(). (re)initialize audio if necessary. */
static int
play_frame( DECODER_STRUCT* w, struct frame* fr )
{
  int clip;

  if( fr->error_protection ) {
    getbits( 16 ); /* crc */
  }

  clip = (fr->do_layer)( w, fr );

  if( clip == -1 ) {
    return -1;
  } else {
    return 0;
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

static void
clear_decoder( void )
{
  /* clear synth */
  real crap [SBLIMIT  ];
  char crap2[SBLIMIT*4];
  int  i;

  for( i = 0; i < 16; i++ )
  {
    memset( crap, 0, sizeof( crap ));
    synth_1to1( crap, 0, crap2 );
    memset( crap, 0, sizeof( crap ));
    synth_1to1( crap, 1, crap2 );
  }

  clear_layer3();
}

static void
_Optlink DecoderThread( void* arg )
{
  ULONG resetcount;
  int   init;

  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;

  for(;;)
  {
    DosWaitEventSem ( w->play, SEM_INDEFINITE_WAIT );
    DosResetEventSem( w->play, &resetcount );

    if( w->end ) {
      return;
    }

    w->status = DECODER_STARTING;
    w->last_length = -1;

    DosResetEventSem( w->playsem, &resetcount );
    DosPostEventSem ( w->ok );

    if( !open_stream( w, w->filename, -1, w->buffersize, w->bufferwait ))
    {
      WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
      w->status = DECODER_STOPPED;
      DosPostEventSem( w->playsem );
      continue;
    }

    w->jumptopos = -1;
    w->rew       =  0;
    w->ffwd      =  0;

    read_frame_init();
    init = 1;

    for( w->stop = FALSE; !w->stop && read_frame( w, &fr ); )
    {
      if(init)
      {
        clear_decoder();
        w->output_format.samplerate = freqs[ fr.sampling_frequency ] >> fr.down_sample;
        init_eq( w->output_format.samplerate );

        free( pcm_sample );
        pcm_sample = (unsigned char*)malloc( w->audio_buffersize );
        pcm_point  = 0;

        init = 0;
        w->status = DECODER_PLAYING;
        w->last_length = decoder_length( w );
      }

      if( play_frame( w, &fr ) < 0 ) {
        WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
        break;
      }

      /* samuel */
      if( w->jumptopos >= 0 )
      {
        int jumptobyte;

        if(( w->XingHeader.flags & FRAMES_FLAG ) &&
           ( w->XingHeader.flags & TOC_FLAG    ) &&
           ( w->XingHeader.flags & BYTES_FLAG  ))
        {
          jumptobyte = SeekPoint( w->XingTOC, w->XingHeader.bytes,
                                 (float)w->jumptopos * 100 / ( 1000.0 * 8 * 144 * w->XingHeader.frames / freqs[fr.sampling_frequency] ));
        }
        else if(( w->XingHeader.flags & FRAMES_FLAG ) &&
                ( w->XingHeader.flags & BYTES_FLAG  ))
        {
          int songlength = 8.0 * 144 * w->XingHeader.frames * 1000 / freqs[fr.sampling_frequency];
          jumptobyte = (float)w->jumptopos * w->XingHeader.bytes / songlength;
        } else {
          jumptobyte = (float)w->jumptopos * w->avg_bitrate * 125 / 1000;
        }

        clear_decoder();
        seekto_pos( w, &fr, jumptobyte );
        w->jumptopos = -1;
        WinPostMsg(w->hwnd,WM_SEEKSTOP,0,0);
      }

      if( w->rew ) {
        /* ie.: ~1/4 second */
        back_pos( w, &fr, (tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125)/4 );
      }
      if( w->ffwd ) {
        forward_pos( w, &fr, (tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125)/4 );
      }
    }

    if( pcm_sample )
    {
      free( pcm_sample );
      pcm_sample = NULL;
    }

    w->status = DECODER_STOPPED;
    close_stream( w );

    if( w->metastruct.save_file != NULL )
    {
      fclose( w->metastruct.save_file );
      w->metastruct.save_file = NULL;
    }

    DosPostEventSem( w->playsem );
    WinPostMsg( w->hwnd, WM_PLAYSTOP, 0, 0 );
    DosPostEventSem( w->ok );
  }
}

static void
reset_thread( DECODER_STRUCT* w )
{
  ULONG resetcount;
  w->end = FALSE;

  DosResetEventSem( w->play, &resetcount );
  DosResetEventSem( w->ok, &resetcount );
  DosKillThread   ( w->decodertid );

  w->decodertid = _beginthread( DecoderThread, 0, 64*1024, (void*)w );
}

/* Init function is called when PM123 needs the specified decoder to play
   the stream demanded by the user. */
int _System
decoder_init( void** returnw )
{
  DECODER_STRUCT* w = calloc( sizeof(*w), 1 );
  *returnw = w;

  DosCreateEventSem( NULL, &w->play, 0, FALSE );
  DosCreateEventSem( NULL, &w->ok,   0, FALSE );

  w->decodertid = _beginthread( DecoderThread, 0, 64*1024, (void*)w );
  return w->decodertid;
}

/* Uninit function is called when another decoder than yours is needed. */
BOOL _System
decoder_uninit( void* arg )
{
  DECODER_STRUCT* w = arg;

  w->end  = TRUE;
  w->stop = TRUE;

  sockfile_abort ( w->filept );
  DosPostEventSem( w->play   );
  wait_thread( w->decodertid, 2000 );

  DosCloseEventSem( w->play );
  DosCloseEventSem( w->ok   );
  return TRUE;
}

ULONG _System
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

      w->output_format.format = WAVE_FORMAT_PCM;
      w->output_format.bits   = 16;

      make_decode_tables( outscale );
      init_layer2( fr.down_sample ); // Inits also shared tables with layer1.
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

      httpauth = info->httpauth;
      proxyurl = info->proxyurl;

      w->buffersize          = info->buffersize;
      w->bufferwait          = info->bufferwait;
      w->playsem             = info->playsem;
      w->hwnd                = info->hwnd;
      w->error_display       = info->error_display;
      w->output_play_samples = info->output_play_samples;
      w->a                   = info->a;
      w->audio_buffersize    = info->audio_buffersize;

      w->metastruct.info_display       = info->info_display;
      w->metastruct.hwnd               = info->hwnd;
      w->metastruct.metadata_buffer    = info->metadata_buffer;
      w->metastruct.metadata_buffer[0] = 0;
      w->metastruct.metadata_size      = info->metadata_size;

      memset( w->metastruct.save_filename, 0, sizeof( w->metastruct.save_filename ));
      DosPostEventSem( w->playsem );
      break;

    case DECODER_PLAY:
      if( w->status == DECODER_STOPPED )
      {
        strcpy( w->filename, info->filename );

        DosResetEventSem( w->ok, &resetcount );
        DosPostEventSem ( w->play );

        if( DosWaitEventSem( w->ok, 10000 ) == ERROR_TIMEOUT )
        {
          w->status = DECODER_STOPPED;
          reset_thread( w );
        }
      }
      break;


    case DECODER_STOP:
      if( w->status != DECODER_STOPPED )
      {
        DosResetEventSem( w->ok, &resetcount );
        w->stop = TRUE;
        sockfile_abort( w->filept );
        if( DosWaitEventSem( w->ok, 10000 ) == ERROR_TIMEOUT )
        {
          w->status = DECODER_STOPPED;
          reset_thread( w );
        }
      }
      break;

    case DECODER_FFWD:
      if( info->ffwd ) {
        nobuffermode_stream( w, TRUE  );
        w->ffwd = TRUE;
      } else {
        nobuffermode_stream( w, FALSE );
        w->ffwd = FALSE;
      }
      break;

    case DECODER_REW:
      if( info->rew ) {
        nobuffermode_stream( w, TRUE );
        w->rew = TRUE;
      } else {
        nobuffermode_stream( w, FALSE );
        w->rew = FALSE;
      }
      break;

    case DECODER_JUMPTO:
      w->jumptopos = info->jumpto;
      break;

    case DECODER_EQ:
      if(( flags.mp3_eq = flags.equalizer = info->equalizer ) == 1 ) {
        memcpy( &octave_eq, info->bandgain, sizeof( octave_eq ));
      }
      if( w->status == DECODER_PLAYING ) {
        init_eq( w->output_format.samplerate );
      }
      break;

    case DECODER_BUFFER:
      info->bufferstatus = bufferstatus_stream( w );
      break;

    case DECODER_SAVEDATA:
      if( info->save_filename == NULL ) {
        w->metastruct.save_filename[0] = 0;
      } else {
        strncpy( w->metastruct.save_filename,
                 info->save_filename, sizeof( w->metastruct.save_filename ) - 1 );
      }
      break;

    default:
      (*w->error_display)( "Unknown command." );
      return 1;
  }
  return 0;
}

ULONG _System
decoder_status( void* arg ) {
  return ((DECODER_STRUCT*)arg)->status;
}

ULONG _System
decoder_length( void* arg )
{
  DECODER_STRUCT* w = arg;

  _control87( EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE |
              EM_INEXACT  | EM_INVALID   | EM_DENORMAL, MCW_EM );

  if( w->status == DECODER_PLAYING )
  {
    if( w->XingHeader.flags & FRAMES_FLAG ) {
      w->last_length = (float)8 * 144 * w->XingHeader.frames * 1000 / freqs[fr.sampling_frequency];
    } else {
      w->last_length = (float)_fsize( w->filept ) * 1000 / w->avg_bitrate / 125;
    }
  }
  return w->last_length;
}

static void _System
dummy_error_display( char* message )
{}

ULONG _System
decoder_fileinfo( char* filename, DECODER_INFO* info )
{
  int           sockmode = 0;
  int           rc       = 0;
  unsigned long header   = 0;
  int           vbr      = 0;
  int           frame_size;
  struct frame  fr;
  int           ssize;
  int           i;
  META_STRUCT   meta = {0};
  XHEADDATA     xing_header;

  memset( info, 0, sizeof( *info ));
  info->size = sizeof( *info );

  meta.metadata_size   = 512;
  meta.metadata_buffer = calloc( meta.metadata_size, 1 );
  meta.data_until_meta =  -1;

  if( !meta.metadata_buffer ) {
    rc = errno;
    goto end;
  }

  if( is_http( filename )) {
    sockmode = HTTP;
  }

  if( !( meta.filept = _fopen( filename, "rb", sockmode, 0, 0 ))) {
    rc = sockfile_errno( sockmode );
    goto end;
  }

read_again:

  rc = 0;

  if( !head_read( &meta, &header )) {
    rc = 100;
    goto end;
  }

  if( !head_check( header ))
  {
    unsigned char byte;
    int i;

    // fprintf( stderr, "Junk at the beginning\n" );
    // I even saw RIFF headers at the beginning of MPEG streams ;(

    // Step in byte steps through next 128K.
    for( i = 0; i < 128 * 1024; i++ ) {
      if( readdata( &byte, 1, 1, &meta ) != 1 ) {
        rc = 100;
        goto end;
      }

      header <<= 8;
      header |= byte;
      header &= 0xFFFFFFFFUL;

      if( head_check( header )) {
        break;
      }
    }
    if( i >= 128 * 1024 ) {
      // fprintf( stderr, "Giving up searching valid MPEG header\n" );
      rc = 200;
      goto end;
    }
  }

  frame_size = decode_header( header, 0, &fr, &ssize, dummy_error_display );

  if( _ftell( meta.filept ) > 256*1024 ) {
    rc = 200;
    goto end;
  }

  if( frame_size <= 0 ) {
    goto read_again;
  }

  if( sockmode )
  {
    int bi = fr.bitrate_index;

    vbr = 0;

    for( i = 0; i < 2; i++ )
    {
      char *data = (char*)alloca( frame_size );
      unsigned long header_next;

      if( readdata( data, 1, frame_size, &meta ) == frame_size
          && head_read( &meta, &header_next ))
      {
        if(( header_next & HDRCMPMASK ) != ( header & HDRCMPMASK )) {
          goto read_again;
        } else {
          frame_size = decode_header( header_next, 0,
                                      &fr, &ssize, dummy_error_display );
          if( bi != fr.bitrate_index ) {
            vbr = 1;
          }
        }
      } else {
        rc = 100;
        goto end;
      }
    }
  }
  else
  {
    int seek_back = 0;
    unsigned long header_next;
    int bi = fr.bitrate_index;

    vbr = 0;

    for( i = 0; i < 32; i++ ) {
      if( _fseek( meta.filept, frame_size, SEEK_CUR ) == 0 &&
          head_read( &meta, &header_next ))
      {
        seek_back -= ( frame_size + 4 );

        if(( header_next & HDRCMPMASK) != ( header & HDRCMPMASK )) {
          _fseek( meta.filept, seek_back, SEEK_CUR );
          goto read_again;
        } else {
          frame_size = decode_header( header_next, 0,
                                      &fr, &ssize, dummy_error_display );
          if( bi != fr.bitrate_index ) {
            vbr = 1;
          }
        }
      } else {
        rc = 100;
        goto end;
      }
    }
    _fseek( meta.filept, seek_back, SEEK_CUR );
  }

  // Restore a first frame broken by previous tests.
  frame_size = decode_header( header, 0, &fr, &ssize, dummy_error_display );

  // Let's try to find a xing VBR header.
  if( fr.lay == 3 )
  {
    char *buf = alloca( frame_size + 4 );

    buf[0] = ( header >> 24 ) & 0xFF;
    buf[1] = ( header >> 16 ) & 0xFF;
    buf[2] = ( header >>  8 ) & 0xFF;
    buf[3] = ( header       ) & 0xFF;

    _fread( buf + 4, 1, frame_size, meta.filept );

    xing_header.toc = NULL;
    GetXingHeader( &xing_header, buf );

    if( xing_header.flags && !( xing_header.flags & BYTES_FLAG )) {
      xing_header.bytes  = _fsize( meta.filept );
      xing_header.flags |= BYTES_FLAG;
    }
  }

  if( rc != 0 ) {
    goto end;
  }

  info->format.format     = WAVE_FORMAT_PCM;
  info->format.bits       = 16;
  info->mpeg              = fr.mpeg25 ? 25 : ( fr.lsf ? 20 : 10 );
  info->layer             = fr.lay;
  info->format.samplerate = freqs[fr.sampling_frequency];
  info->mode              = fr.mode;
  info->modext            = fr.mode_ext;
  info->bpf               = frame_size + 4;
  info->format.channels   = fr.stereo;
  info->bitrate           = tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index];
  info->extention         = fr.extension;
  info->junklength        = _ftell( meta.filept ) - 4 - frame_size;

  if( xing_header.flags & FRAMES_FLAG )
  {
    info->songlength = 8.0 * 144 * xing_header.frames * 1000 / freqs[fr.sampling_frequency];

    if( xing_header.flags & BYTES_FLAG ) {
      info->bitrate = (float)xing_header.bytes / info->songlength * 1000 / 125;
    }
  }
  else
  {
    if( !info->bitrate ) {
      rc = 200;
    } else {
      info->songlength = (float)_fsize( meta.filept ) * 1000 / ( info->bitrate * 125 );
    }
  }

  sprintf( info->tech_info, "%u kbs, %f kHz, %s",
           info->bitrate, ( info->format.samplerate / 1000.0 ), modes( info->mode ));

  if( vbr ) {
    if( xing_header.flags & TOC_FLAG ) {
      strcat( info->tech_info, ", True VBR" );
    } else {
      strcat( info->tech_info, ", VBR" );
    }
  }

  if( sockmode )
  {
    HTTP_INFO http_info;
    char* titlepos;

    sockfile_httpinfo( meta.filept, &http_info );

    strncpy( info->comment, http_info.icy_name,  sizeof( info->comment ) - 1 );
    strncpy( info->genre,   http_info.icy_genre, sizeof( info->genre   ) - 1 );

    // Title, I'd have to read at least metaint bytes to snatch this.
    titlepos = strstr( meta.metadata_buffer, "StreamTitle='" );

    if( titlepos ) {
      titlepos += 13;
      for( i = 0; i < sizeof( info->title ) - 1 && *titlepos
                  && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
      {
        info->title[i] = *titlepos++;
      }

      info->title[i] = 0;
    }
  } else {
    *info->comment = 0;
    *info->genre   = 0;
  }

end:

  if( meta.filept ) {
    _fclose( meta.filept );
  }
  if( meta.metadata_buffer ) {
    free( meta.metadata_buffer );
  }
  return rc;
}

ULONG _System
decoder_trackinfo( char* drive, int track, DECODER_INFO* info ) {
  return 200;
}

ULONG _System
decoder_cdinfo( char* drive, DECODER_CDINFO* info ) {
  return 100;
}

ULONG _System
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

  return DECODER_FILENAME | DECODER_URL;
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

      if( !mmx_present ) {
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
int _System
plugin_configure( HWND hwnd, HMODULE module )
{
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  return 0;
}

/* Returns information about plug-in. */
int _System
plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type         = PLUGIN_DECODER;
   param->author       = "Samuel Audet";
   param->desc         = "MP3 Decoder 1.22";
   param->configurable = TRUE;

   load_ini();
   return 0;
}
