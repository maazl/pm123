/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2005 Dmitry A.Steklenev <glass@ptv.ru>
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
 *
 * The very first PM123 visual plugin - the spectrum analyzer
 */

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_GPI
#define  INCL_OS2MM
#include <os2.h>
#include <os2me.h>
#include <stdio.h>
#include <stdlib.h>
#include <dive.h>
#include <fourcc.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include <utilfct.h>
#include <format.h>
#include <decoder_plug.h>
#include <visual_plug.h>
#include <plugin.h>

#include "analyzer.h"
#define  TID_UPDATE ( TID_USERMAX - 1 )

static struct analyzer_cfg {

  ULONG update_delay;
  int   default_mode;
  int   falloff;
  int   falloff_speed;
  int   display_percent;

} cfg;

#define CLR_BGR_BLACK   0
#define CLR_BGR_GREY    1
#define CLR_ANA_BOTTOM  2
#define CLR_ANA_TOP     17
#define CLR_OSC_DIMMEST 18
#define CLR_OSC_BRIGHT  22
#define CLR_ANA_BARS    23

#define BARS_CY         3
#define BARS_SPACE      1

static  HAB    hab;
static  HWND   hconfigure;
static  HWND   hanalyzer;
static  HDIVE  hdive;
static  RGB2   palette[24];
static  float* amps;
static  int    amps_count;
static  ULONG* bars;
static  int    bars_count;
static  int*   scale;
static  ULONG  image_id;
static  BOOL   is_stopped;

static  ULONG (DLLENTRYP _decoderPlayingSamples)( FORMAT_INFO *info, char *buf, int len );
static  BOOL  (DLLENTRYP _decoderPlaying)( void );
static  int   (DLLENTRYP _specana_init)( int setnumsamples );
static  int   (DLLENTRYP _specana_dobands)( float bands[] );

static  VISPLUGININIT plug;

/* Removes comments */
static char*
uncomment_slash( char *something )
{
  int  i = 0;
  BOOL inquotes = FALSE;

  while( something[i] )
  {
    if( something[i] == '\"' ) {
      inquotes = !inquotes;
    } else if( something[i] == '/' && something[i+1] == '/' && !inquotes ) {
      something[i] = 0;
      break;
    }
    i++;
  }

  return something;
}

/* Reads RGB colors from specified file. */
static BOOL
read_color( FILE* file, RGB2* color )
{
  char line[256];
  int  r,g,b;

  if( fgets( line, sizeof( line ), file )) {
    if( color ) {
      sscanf( uncomment_slash( line ), "%d,%d,%d", &r, &g, &b );
      color->bRed   = r;
      color->bGreen = g;
      color->bBlue  = b;
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Initializes spectrum analyzer bands. */
static int
init_bands( void )
{
  float step, z;
  int   i;

  amps_count = _specana_init(( plug.cx << 1 ) * 100 / cfg.display_percent );
  bars_count = plug.cx / ( BARS_CY + BARS_SPACE );

  if( plug.cx - bars_count * ( BARS_CY + BARS_SPACE ) >= BARS_CY ) {
    ++bars_count;
  }

  amps  = realloc( amps,  sizeof(*amps ) * amps_count );
  bars  = realloc( bars,  sizeof(*bars ) * plug.cx );
  scale = realloc( scale, sizeof(*scale) * ( bars_count + 1 ));

  memset( amps,  0, sizeof( *amps  ) * amps_count );
  memset( bars,  0, sizeof( *bars  ) * plug.cx );

  step     = log( amps_count ) / bars_count;
  z        = 0;
  scale[0] = 1;

  for( i = 1; i < bars_count; i++ )
  {
    z += step;
    scale[i] = exp( z );

    if( i > 0 && scale[i] <= scale[i-1] ) {
      scale[i] = scale[i-1] + 1;
    }
  }

  scale[bars_count] = amps_count;
  return amps_count;
}

/* Free memory used by spectrum analyzer bands. */
static void
free_bands( void )
{
  free( amps  );
  free( bars  );
  free( scale );

  amps_count = 0;
  bars_count = 0;
  amps       = NULL;
  bars       = NULL;
  scale      = NULL;
}

static void
clear_analyzer( void )
{
  ULONG cx, cy;
  char* image;

  DiveBeginImageBufferAccess( hdive, image_id, &image, &cx, &cy );
  memset( image, 0, cx * cy );
  memset( bars , 0, plug.cx * sizeof( *bars ));
  DiveEndImageBufferAccess( hdive, image_id );
  DiveBlitImage( hdive, image_id, 0 );
}

/* Returns information about plug-in. */
void DLLENTRY
plugin_query( PPLUGIN_QUERYPARAM query )
{
  query->type         = PLUGIN_VISUAL;
  query->author       = "Samuel Audet, Dmitry A.Steklenev ";
  query->desc         = "Spectrum Analyzer 2.00";
  query->configurable = 1;
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      hconfigure = hwnd;
      do_warpsans( hwnd );

      WinCheckButton( hwnd, RB_ANALYZER + cfg.default_mode, TRUE );
      WinCheckButton( hwnd, CB_FALLOFF, cfg.falloff );

      WinSendDlgItemMsg( hwnd, SB_PERCENT,   SPBM_SETLIMITS,
                         MPFROMLONG( 100 ), MPFROMLONG( 1 ));
      WinSendDlgItemMsg( hwnd, SB_PERCENT,   SPBM_SETCURRENTVALUE,
                         MPFROMLONG( cfg.display_percent ), 0 );
      WinSendDlgItemMsg( hwnd, SB_FREQUENCY, SPBM_SETLIMITS,
                         MPFROMLONG( 200 ), MPFROMLONG( 1 ));
      WinSendDlgItemMsg( hwnd, SB_FREQUENCY, SPBM_SETCURRENTVALUE,
                         MPFROMLONG( 1000 / cfg.update_delay ), 0 );
      WinSendDlgItemMsg( hwnd, SB_FALLOFF,   SPBM_SETLIMITS,
                         MPFROMLONG( plug.cy ), MPFROMLONG( 1 ));
      WinSendDlgItemMsg( hwnd, SB_FALLOFF,   SPBM_SETCURRENTVALUE,
                         MPFROMLONG( cfg.falloff_speed ), 0 );
      break;
   }

    case WM_DESTROY:
    {
      ULONG value;

      cfg.default_mode = LONGFROMMR( WinSendDlgItemMsg( hwnd, RB_ANALYZER,
                                                        BM_QUERYCHECKINDEX, 0, 0 ));

      WinSendDlgItemMsg( hwnd, SB_FREQUENCY, SPBM_QUERYVALUE,
                         MPFROMP( &value ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));

      cfg.update_delay = 1000 / value;

      WinStopTimer ( hab, hanalyzer, TID_USERMAX - 1 );
      WinStartTimer( hab, hanalyzer, TID_USERMAX - 1, cfg.update_delay );

      cfg.falloff = WinQueryButtonCheckstate( hwnd, CB_FALLOFF );

      WinSendDlgItemMsg( hwnd, SB_FALLOFF, SPBM_QUERYVALUE,
                         MPFROMP( &value ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));

      cfg.falloff_speed = value;

      WinSendDlgItemMsg( hwnd, SB_PERCENT, SPBM_QUERYVALUE,
                         MPFROMP( &value ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));

      cfg.display_percent = value;

      init_bands();
      clear_analyzer();
      hconfigure  = NULLHANDLE;
      break;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
void DLLENTRY
plugin_configure( HWND hwnd, HMODULE module ) {
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
}

static MRESULT EXPENTRY
plg_win_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case DM_DRAGOVER:
    case DM_DROP:
    case 0x041f:
    case 0x041e:
    case WM_CONTEXTMENU:
    case WM_BUTTON2MOTIONSTART:
    case WM_BUTTON1MOTIONSTART:
      return WinSendMsg( plug.hwnd, msg, mp1, mp2 );

    case WM_VRNENABLED:
    {
      HPS     hps  = WinGetPS( hwnd );
      HRGN    hrgn = GpiCreateRegion( hps, 0, NULL );

      RGNRECT rgn_control;
      RECTL   rgn_rectangles[64];
      SWP     swp;
      POINTL  pos;

      if( hrgn )
      {
        WinQueryVisibleRegion( hwnd, hrgn );

        rgn_control.ircStart    = 0;
        rgn_control.crc         = sizeof( rgn_rectangles ) / sizeof( RECTL );
        rgn_control.ulDirection = RECTDIR_LFRT_TOPBOT;

        if( GpiQueryRegionRects( hps, hrgn, NULL, &rgn_control, rgn_rectangles ))
        {
          SETUP_BLITTER setup;

          WinQueryWindowPos( plug.hwnd, &swp );
          pos.x = swp.x + plug.x;
          pos.y = swp.y + plug.y;
          WinMapWindowPoints( plug.hwnd, HWND_DESKTOP, &pos, 1 );

          setup.ulStructLen       = sizeof(SETUP_BLITTER);
          setup.fInvert           = FALSE;
          setup.fccSrcColorFormat = FOURCC_LUT8;
          setup.ulSrcWidth        = plug.cx;
          setup.ulSrcHeight       = plug.cy;
          setup.ulSrcPosX         = 0;
          setup.ulSrcPosY         = 0;
          setup.ulDitherType      = 0;
          setup.fccDstColorFormat = FOURCC_SCRN;
          setup.ulDstWidth        = plug.cx;
          setup.ulDstHeight       = plug.cy;
          setup.lDstPosX          = 0;
          setup.lDstPosY          = 0;
          setup.lScreenPosX       = pos.x;
          setup.lScreenPosY       = pos.y;
          setup.ulNumDstRects     = rgn_control.crcReturned;
          setup.pVisDstRects      = rgn_rectangles;

          DiveSetupBlitter( hdive, &setup );
        }

        GpiDestroyRegion( hps, hrgn );
      }

      WinReleasePS ( hps );
      WinStartTimer( hab, hanalyzer, TID_UPDATE, cfg.update_delay );
      break;
    }

    case WM_VRNDISABLED:
      DiveSetupBlitter( hdive, 0 );
      break;

    case WM_BUTTON1DBLCLK:
    case WM_BUTTON1CLICK:
      if( ++cfg.default_mode > SHOW_DISABLED ) {
        cfg.default_mode = SHOW_ANALYZER;
      }
      clear_analyzer();
      break;

    case WM_TIMER:
    {
      long i, x, y, z, max, e, color;
      unsigned long image_cx, image_cy;
      char* image;

      DiveBeginImageBufferAccess( hdive, image_id, &image, &image_cx, &image_cy );
      memset( image, 0, image_cx * image_cy );

      for( y = 0; y < plug.cy; y += 2 ) {
        for( x = 0; x < plug.cx; x += 2 ) {
          image[( y * image_cx ) + x ] = 1;
        }
      }

      if( cfg.default_mode == SHOW_ANALYZER && _decoderPlaying() == 1 )
      {
        if(( max = _specana_dobands( amps )) != 0 ) {
          for( x = 0; x < plug.cx; x++ )
          {
            z = max( plug.cy * log10( 150 * amps[ x + 1 ] / max + 1 ), 0 );
            z = min( z, plug.cy - 1 );

            if( cfg.falloff  && bars[x] > cfg.falloff_speed ) {
              bars[x] = max( z, bars[x] - cfg.falloff_speed );
            } else {
              bars[x] = z;
            }

            for( y = 0; y < bars[x]; y++ )
            {
              color = ( CLR_ANA_TOP - CLR_ANA_BOTTOM ) * y / ( plug.cy - bars[x] ) + CLR_ANA_BOTTOM;
              color = min( CLR_ANA_TOP, color );
              image[( plug.cy - y ) * image_cx + x ] = color;
            }
          }
        }
      }
      else if( cfg.default_mode == SHOW_BARS && _decoderPlaying() == 1 )
      {
        if(( max = _specana_dobands( amps )) != 0 ) {
          for( i = 0; i < bars_count; i++ )
          {
            float a = 0;
            for( e = scale[i]; e < scale[i+1]; e++ ) {
              a += amps[e];
            }

            z = max( plug.cy * log( 30 * a / max + 1 ), 0 );
            z = min( z, plug.cy - 1 );

            if( cfg.falloff  && bars[i] > cfg.falloff_speed ) {
              bars[i] = max( z, bars[i] - cfg.falloff_speed );
            } else {
              bars[i] = z;
            }

            for( y = 0; y < bars[i]; y++ )
            {
              color = ( CLR_ANA_TOP - CLR_ANA_BOTTOM ) * y / plug.cy + CLR_ANA_BOTTOM;
              color = min( CLR_ANA_TOP, color );
              for( x = 0; x < BARS_CY; x++ ) {
                image[( plug.cy - y ) * image_cx + ( BARS_CY + BARS_SPACE ) * i + x ] = color;
              }
            }
          }
        }
      }
      else if( cfg.default_mode == SHOW_OSCILLOSCOPE && _decoderPlaying() == 1 )
      {
        FORMAT_INFO bufferinfo;
        unsigned char* sample;
        int   len;

        _decoderPlayingSamples( &bufferinfo, NULL, 0 );

        len = bufferinfo.channels * plug.cx;
        max = 0x7FFFFFFFUL >> ( 32 - bufferinfo.bits );

        if( bufferinfo.bits >  8 ) {
          len *= 2;
        }
        if( bufferinfo.bits > 16 ) {
          len *= 2;
        }

        sample = alloca( len );
        _decoderPlayingSamples( &bufferinfo, sample, len );

        for( x = 0; x < plug.cx; x++ )
        {
          z = 0;
          for( e = 0; e < bufferinfo.channels; e++ ) {
            if( bufferinfo.bits <= 8 ) {
              z += (sample[x+e] - 128) / bufferinfo.channels;
            } else if( bufferinfo.bits <= 16 ) {
              z += ((short*)sample)[x+e] / bufferinfo.channels;
            } else if( bufferinfo.bits <= 32 ) {
              z += ((long* )sample)[x+e] / bufferinfo.channels;
            }
          }

          y = plug.cy / 2 + plug.cy * z / 2 / max;
          color = ( CLR_OSC_BRIGHT - CLR_OSC_DIMMEST + 2 ) * abs(z) / max + CLR_OSC_DIMMEST;
          color = min( CLR_OSC_BRIGHT, color );
          image[ y * image_cx + x ] = color;
        }
      }

      DiveEndImageBufferAccess( hdive, image_id );

      if( _decoderPlaying() == 0 && !is_stopped )
      {
        is_stopped = TRUE;
        DiveBlitImage( hdive, 0, 0 );
        memset( bars,  0, sizeof( *bars ) * plug.cx );
      }
      else if( _decoderPlaying() == 1 )
      {
        is_stopped = FALSE;
      }

      if( _decoderPlaying() == 1 && cfg.default_mode != SHOW_DISABLED ) {
        DiveBlitImage( hdive, image_id, 0 );
      }

      break;
    }

    default:
      return WinDefWindowProc( hwnd, msg, mp1, mp2 );
  }
  return 0;
}

HWND DLLENTRY
vis_init( PVISPLUGININIT init )
{
  FILE* dat;
  HINI  hini;
  int   i;

  memcpy( &plug, init, sizeof( VISPLUGININIT ));

  cfg.update_delay    = 31;
  cfg.default_mode    = SHOW_BARS;
  cfg.falloff         = 1;
  cfg.falloff_speed   = 1;
  cfg.display_percent = 80;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, cfg.update_delay );
    load_ini_value( hini, cfg.default_mode );
    load_ini_value( hini, cfg.falloff );
    load_ini_value( hini, cfg.falloff_speed );
    load_ini_value( hini, cfg.display_percent );

    close_ini( hini );
  }

  // First get the routines
  _decoderPlayingSamples = init->procs->output_playing_samples;
  _decoderPlaying        = init->procs->decoder_playing;
  _specana_init          = init->procs->specana_init;
  _specana_dobands       = init->procs->specana_dobands;

  init_bands();

  // Open up DIVE
  if( DiveOpen( &hdive, FALSE, 0 ) != DIVE_SUCCESS ) {
    return 0;
  }

  /* Allocate image buffer (256-color) */
  image_id = 0;
  if( DiveAllocImageBuffer( hdive, &image_id, FOURCC_LUT8,
                            plug.cx, plug.cy, 0, NULL ) != DIVE_SUCCESS )
  {
    DiveClose( hdive );

    hdive = 0;
    hanalyzer = 0;

    return 0;
  }

  // Initialize PM
  hab = WinQueryAnchorBlock( plug.hwnd );
  WinRegisterClass( hab, "Spectrum Analyzer", plg_win_proc, CS_SIZEREDRAW | CS_MOVENOTIFY, 0 );

  hanalyzer = WinCreateWindow( plug.hwnd,
                               "Spectrum Analyzer",
                               "Spectrum Analyzer",
                               WS_VISIBLE,
                               plug.x,
                               plug.y,
                               plug.cx,
                               plug.cy,
                               plug.hwnd,
                               HWND_TOP,
                               plug.id,
                               NULL,
                               NULL  );

  memset( palette, 0, sizeof( palette ));

  if( plug.param && *plug.param && ( dat = fopen( plug.param, "r" )))
  {
    read_color( dat, &palette[CLR_BGR_BLACK] );
    read_color( dat, &palette[CLR_BGR_GREY ] );

    for( i = CLR_ANA_TOP; i >= CLR_ANA_BOTTOM; i-- ) {
      read_color( dat, &palette[i] );
    }

    for( i = CLR_OSC_DIMMEST; i <= CLR_OSC_BRIGHT; i++ ) {
      read_color( dat, &palette[i] );
    }

    if( !read_color( dat, &palette[CLR_ANA_BARS] )) {
      palette[CLR_ANA_BARS] = palette[CLR_ANA_TOP];
    }

    fclose( dat );
  }
  else
  {
    palette[ 2].bRed = 255; palette[ 2].bGreen =   0;
    palette[ 3].bRed = 225; palette[ 3].bGreen =  30;
    palette[ 4].bRed = 195; palette[ 4].bGreen =  85;
    palette[ 5].bRed = 165; palette[ 5].bGreen = 115;
    palette[ 6].bRed = 135; palette[ 6].bGreen = 135;
    palette[ 7].bRed = 115; palette[ 7].bGreen = 165;
    palette[ 8].bRed =  85; palette[ 8].bGreen = 195;
    palette[ 9].bRed =  30; palette[ 9].bGreen = 225;
    palette[10].bRed =   0; palette[10].bGreen = 255;
    palette[11].bRed =   0; palette[11].bGreen = 255;
    palette[12].bRed =   0; palette[12].bGreen = 255;
    palette[13].bRed =   0; palette[13].bGreen = 255;
    palette[14].bRed =   0; palette[14].bGreen = 255;
    palette[15].bRed =   0; palette[15].bGreen = 255;
    palette[16].bRed =   0; palette[16].bGreen = 255;
    palette[17].bRed =   0; palette[17].bGreen = 255;

    palette[18].bGreen = 180;
    palette[19].bGreen = 200;
    palette[20].bGreen = 225;
    palette[21].bGreen = 255;
    palette[22].bGreen = 255;

    palette[CLR_BGR_GREY].bGreen =  90;
    palette[CLR_ANA_BARS].bGreen = 255;
  }

  DiveSetSourcePalette( hdive, 0, sizeof( palette ) / sizeof( palette[0] ), (char*)&palette );
  WinSetVisibleRegionNotify( hanalyzer, TRUE );

  // Apparently the WM_VRNENABLED message not necessarily automatically
  // follows WinSetVisibleRegionNotify call. We posts it manually.
  WinPostMsg( hanalyzer, WM_VRNENABLED, MPFROMLONG( TRUE ), 0 );
  return hanalyzer;
}

int DLLENTRY
plugin_deinit( void )
{
  HINI hini;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value( hini, cfg.update_delay );
    save_ini_value( hini, cfg.default_mode );
    save_ini_value( hini, cfg.falloff );
    save_ini_value( hini, cfg.falloff_speed );
    save_ini_value( hini, cfg.display_percent );

    close_ini( hini );
  }

  WinStopTimer( hab, hanalyzer, TID_USERMAX - 1 );
  DiveFreeImageBuffer( hdive, image_id );
  WinDestroyWindow( hanalyzer );

  DiveClose( hdive );

  if( hconfigure ) {
    WinDismissDlg( hconfigure, 0 );
  }

  free_bands();

  hdive = 0;
  hanalyzer = 0;

  return PLUGIN_OK;
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
