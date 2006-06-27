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

#include <utilfct.h>
#include <format.h>
#include <decoder_plug.h>
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

#define CLR_BGR_BLACK   0 // Background
#define CLR_BGR_GREY    1 // grid
#define CLR_OSC_DIMMEST 2 // oscilloscope dark
#define CLR_OSC_BRIGHT  6 // oscilloscope light
#define CLR_ANA_BARS    7 // don't know
#define CLR_SPC_BOTTOM  8 // dark spectrum
#define CLR_ANA_BOTTOM  32// low bar
#define CLR_ANA_TOP     72// high bar
#define CLR_SPC_TOP     79// full power spectrum
#define CLR_SIZE        80

#define BARS_CY         3
#define BARS_SPACE      1

static  RGB2   palette[CLR_SIZE] = { // B,G,R!
  {0,0,0},  // background
  {0,90,0}, // grid
  {0,180,0},// oscilloscope
  {0,200,0},
  {0,225,0},
  {0,255,0},
  {0,255,0},// oscilloscope
  {0,255,0},// ANA_BARS?
  {0,0,0},  // spectrum low
  {0,0,11},
  {0,0,21},
  {0,0,32},
  {0,0,43},
  {0,0,53},
  {0,0,64},
  {0,0,74},
  {0,0,85},
  {0,0,96},
  {0,0,106},
  {0,0,117},
  {0,0,128},
  {0,0,138},
  {0,0,149},
  {0,0,159},
  {0,0,170},
  {0,0,181},
  {0,0,191},
  {0,0,202},
  {0,0,213},
  {0,0,223},
  {0,0,234},
  {0,0,244},
  {0,0,255},// low bar
  {0,13,255},
  {0,25,254}, 
  {0,37,253},
  {0,48,252},
  {0,60,251}, 
  {0,71,249},
  {0,81,247},
  {0,92,245},
  {0,102,242},
  {0,112,239},
  {0,121,236},
  {0,130,232},
  {0,139,228},
  {0,147,224},
  {0,155,219},
  {0,163,214},
  {0,171,209},
  {0,178,203},
  {0,185,197},
  {0,191,191},
  {0,197,185},
  {0,203,178},
  {0,209,171},
  {0,214,163},
  {0,219,155},
  {0,224,147},
  {0,228,139},
  {0,232,130},
  {0,236,121},
  {0,239,112},
  {0,242,102},
  {0,245,92},
  {0,247,81},
  {0,249,71},
  {0,251,60},
  {0,252,48},
  {0,253,37},
  {0,254,25},
  {0,255,13},
  {0,255,0}, // high bar
  {20,255,0},
  {40,255,0},
  {60,255,0},
  {80,255,0},
  {100,255,0},
  {120,255,0},
  {140,255,0} };// high spectroscope
static  HAB    hab;
static  HWND   hconfigure;
static  HWND   hanalyzer;
static  HDIVE  hdive;
static  float* amps;
static  float* lastamps;
static  int    amps_count;
static  float* bars;
static  int    bars_count;
static  int*   scale;
static  int    specband_count;
static  int*   spec_scale;
static  ULONG  image_id;
static  BOOL   is_stopped;
static  float  relative_falloff_speed; 

static  PFN    _decoderPlayingSamples;
static  PFN    _decoderPlaying;
static  PFN    _specana_init;
static  PFN    _specana_dobands;

static  VISPLUGININIT plug;

/********** read pallette data **********************************************/

/* Removes comments */
static char*
uncomment( char *something )
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
      sscanf( uncomment( line ), "%d,%d,%d", &r, &g, &b );
      color->bRed   = r;
      color->bGreen = g;
      color->bBlue  = b;
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

static void read_palette( FILE* dat )
{ int i;

  read_color( dat, &palette[CLR_BGR_BLACK] );
  read_color( dat, &palette[CLR_BGR_GREY ] );

  for( i = CLR_OSC_DIMMEST; i <= CLR_OSC_BRIGHT; i++ ) {
    read_color( dat, &palette[i] );
  }

  read_color( dat, &palette[CLR_ANA_BARS] );

  for( i = CLR_SPC_BOTTOM; i <= CLR_SPC_TOP; i++ ) {
    read_color( dat, &palette[i] );
  }

  fclose( dat );
}

/********** filter design ***************************************************/

/* Initializes spectrum analyzer bands. */
static int
init_bands( void )
{
  float step, z;
  int   i;

  amps_count = _specana_init(( plug.cx << 2 ) * 100 / cfg.display_percent );
  bars_count = plug.cx / ( BARS_CY + BARS_SPACE );
  specband_count = plug.cy;

  if( plug.cx - bars_count * ( BARS_CY + BARS_SPACE ) >= BARS_CY ) {
    ++bars_count;
  }

  amps       = realloc( amps,       sizeof(*amps ) * amps_count );     
  lastamps   = realloc( lastamps,   sizeof(*amps ) * amps_count );     
  bars       = realloc( bars,       sizeof(*bars ) * plug.cx );        
  scale      = realloc( scale,      sizeof(*scale) * (bars_count+1));
  spec_scale = realloc( spec_scale, sizeof(*spec_scale) * (specband_count+1));

  memset( amps,  0, sizeof( *amps  ) * amps_count );
  memset( lastamps,  0, sizeof( *lastamps  ) * amps_count );
  memset( bars,  0, sizeof( *bars  ) * plug.cx );

  step     = log( amps_count ) / bars_count;
  z        = 0;
  scale[0] = 1;
  for( i = 1; i < bars_count; i++ )
  { z += step;
    scale[i] = exp( z );

    if( i > 0 && scale[i] <= scale[i-1] )
      scale[i] = scale[i-1] + 1;
  }
  scale[bars_count] = amps_count;

  step     = log( amps_count ) / specband_count;
  z        = 0;
  spec_scale[0] = 1;
  for( i = 1; i < specband_count; i++ )
  { z += step;
    spec_scale[i] = exp( z );

    if( i > 0 && spec_scale[i] <= spec_scale[i-1] )
      spec_scale[i] = spec_scale[i-1] + 1;
  }
  spec_scale[specband_count] = amps_count;

  return amps_count;
}

/* Free memory used by spectrum analyzer bands. */
static void
free_bands( void )
{
  free( amps  );
  free( lastamps );
  free( bars  );
  free( scale );
  free( spec_scale );

  amps_count = 0;
  bars_count = 0;
  amps       = NULL;
  lastamps   = NULL;
  bars       = NULL;
  scale      = NULL;
  spec_scale = NULL;
}

/* like _specana_dobands but return -1 if the values did not change,
 * most likely because of a pause condition.
 */ 
static int call_specana_dobands(void)
{ int r = _specana_dobands(amps);
  if (memcmp(amps, lastamps, amps_count * sizeof *amps) == 0)
    return -1;
  memcpy(lastamps, amps, amps_count * sizeof *amps);
  return r;
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

_Inline void update_barvalue(float* val, float z)
{
  if (z >= 1)
    z = .999999;

  if( !cfg.falloff || *val-relative_falloff_speed <= z )
    *val = z;
   else
    *val -= relative_falloff_speed;
}

/* Do the regular update of the analysis window.
 * This is called at the WM_TIMER event of the analyzer window.
 */
static void update_analyzer(void)
{
  long i, x, y, max, e;
  unsigned long image_cx, image_cy;
  char* image = NULL;

  if( _decoderPlaying() == 0 )
  { if ( !is_stopped )
    { is_stopped = TRUE;
      DiveBlitImage( hdive, 0, 0 );
      memset( bars,  0, sizeof( *bars ) * plug.cx );
    }
    return;
  }
  // _decoderPlaying() == 1

  switch (cfg.default_mode)
  {case SHOW_ANALYZER:

    max = call_specana_dobands();
    if (max <= 0) // no changes or no valid content
      break;
    
    DiveBeginImageBufferAccess( hdive, image_id, &image, &image_cx, &image_cy );
    memset( image, 0, image_cx * image_cy );

    // Grid
    for( y = 0; y < plug.cy; y += 2 ) {
      for( x = 0; x < plug.cx; x += 2 ) {
        image[( y * image_cx ) + x ] = 1;
      }
    }

    for( x = 0; x < plug.cx; x++ )
    {
      update_barvalue( bars+x, .66667*log10(250 * (amps[2*x+1]+amps[2*x+2]) / max +1));

      for( y = (int)(bars[x]*(plug.cy+1)) -1; y >= 0; y-- )
      {
        int color = ( CLR_ANA_TOP - CLR_ANA_BOTTOM +1 ) * y / (plug.cy * (1-bars[x])) + CLR_ANA_BOTTOM;
        image[( plug.cy - y -1) * image_cx + x ] = min( CLR_ANA_TOP, color );
      }
    }
    break;  

   case SHOW_BARS:

    max = call_specana_dobands();
    if (max <= 0) // no changes or no valid content
      break;
    
    DiveBeginImageBufferAccess( hdive, image_id, &image, &image_cx, &image_cy );
    memset( image, 0, image_cx * image_cy );

    // Grid
    for( y = 0; y < plug.cy; y += 2 ) {
      for( x = 0; x < plug.cx; x += 2 ) {
        image[( y * image_cx ) + x ] = 1;
      }
    }

    for( i = 0; i < bars_count; i++ )
    {
      float a = 0;
      for( e = scale[i]; e < scale[i+1]; e++ )
        a += amps[e];
      a /= scale[i+1] - scale[i];

      update_barvalue( bars+i, .66667*log10(500 * a / max +1));

      for( y = (int)(bars[i]*(plug.cy+1)) -1; y >= 0; y-- )
      {
        int color = ( CLR_ANA_TOP - CLR_ANA_BOTTOM +1 ) * y / (plug.cy * (1-bars[i])) + CLR_ANA_BOTTOM;
        color = min( CLR_ANA_TOP, color );
        for( x = 0; x < BARS_CY; x++ ) {
          image[( plug.cy - y -1) * image_cx + ( BARS_CY + BARS_SPACE ) * i + x ] = color;
        }
      }
    }
    break;

   case SHOW_SPECTROSCOPE:

    max = call_specana_dobands();
    if (max <= 0) // no changes or no valid content
      break;

    DiveBeginImageBufferAccess( hdive, image_id, &image, &image_cx, &image_cy );
    { char* ip = image + (plug.cy-1) * image_cx; // bottom line
      if (is_stopped)
        // first call after restart
        memset(image, 0, (plug.cy-1) * image_cx);
       else 
        // move image one pixel up
        memmove(image, image+image_cx, (plug.cy-1) * image_cx);
    
      for( x = 0; x < plug.cx; x++ )
      {
        update_barvalue( bars+x, .5*log10(375 * (amps[2*x+1]+amps[2*x+2]) / max * sqrt(2.20 * cfg.display_percent * x / plug.cx) +1));
        ip[x] = ( CLR_SPC_TOP - CLR_SPC_BOTTOM +1 ) * bars[x] + CLR_SPC_BOTTOM;
      }
    }
    break;

   case SHOW_LOGSPECSCOPE:

    max = call_specana_dobands();
    if (max <= 0) // no changes or no valid content
      break;

    DiveBeginImageBufferAccess( hdive, image_id, &image, &image_cx, &image_cy );
    { char* ip = image + (plug.cx-1); // last pixel, first line
      if (is_stopped)
        // first call after restart
        memset(image, 0, plug.cy*image_cx);
       else 
        // move image left one pixel
        memmove(image, image+1, plug.cy*image_cx -1);
    
      for( i = 0; i < specband_count; i++ )
      { 
        double a = 0;
        for( e = spec_scale[i]; e < spec_scale[i+1]; e++ )
          a += amps[e];
        a /= spec_scale[i+1] - spec_scale[i];
        
        update_barvalue( bars+i, .5*log10(750 * a / max * sqrt(220.*spec_scale[i]/amps_count) +1));

        ip[(plug.cy-i-1)*image_cx] =
          ( CLR_SPC_TOP - CLR_SPC_BOTTOM +1 ) * bars[i] + CLR_SPC_BOTTOM;
      }
    }
    break;

   case SHOW_OSCILLOSCOPE:

    DiveBeginImageBufferAccess( hdive, image_id, &image, &image_cx, &image_cy );
    { FORMAT_INFO bufferinfo;
      unsigned char* sample;
      int   len;

      memset( image, 0, image_cx * image_cy );

      // Grid
      for( y = 0; y < plug.cy; y += 2 ) {
        for( x = 0; x < plug.cx; x += 2 ) {
          image[( y * image_cx ) + x ] = 1;
        }
      }

      _decoderPlayingSamples( &bufferinfo, NULL, 0 );

      len = bufferinfo.channels * plug.cx;
      max = 0x7FFFFFFFUL >> ( 32 - bufferinfo.bits );

      if( bufferinfo.bits >  8 ) {
        len *= 2;
      }
      if( bufferinfo.bits > 16 ) {
        len *= 2;
      }

      sample = _alloca( len );
      _decoderPlayingSamples( &bufferinfo, sample, len );

      for( x = 0; x < plug.cx; x++ )
      {
        int color;
        int z = 0;
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
        image[ y * image_cx + x ] = min( CLR_OSC_BRIGHT, color );
      }
    }
    break;
  }

  if (image != NULL)
  { DiveEndImageBufferAccess( hdive, 0 );
    DiveBlitImage( hdive, image_id, 0 );
  }
  is_stopped = FALSE;
}


/* Returns information about plug-in. */
int _System
plugin_query( PPLUGIN_QUERYPARAM query )
{
  query->type         = PLUGIN_VISUAL;
  query->author       = "Samuel Audet, Dmitry A.Steklenev ";
  query->desc         = "Spectrum Analyzer 2.00";
  query->configurable = 1;
  return 0;
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
                         MPFROMLONG( 100 ), MPFROMLONG( 25 )); // lower values may decrease speed
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
      relative_falloff_speed = (float)value / plug.cy; 

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
int _System
plugin_configure( HWND hwnd, HMODULE module )
{
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  return 0;
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
      WinSendMsg( plug.hwnd, msg, mp1, mp2 );
      break;

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
      update_analyzer();
      break;

    default:
      return WinDefWindowProc( hwnd, msg, mp1, mp2 );
  }
  return 0;
}

HWND _System
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
  relative_falloff_speed = 1./plug.cy;
  cfg.display_percent = 80;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, cfg.update_delay );
    load_ini_value( hini, cfg.default_mode );
    load_ini_value( hini, cfg.falloff );
    load_ini_value( hini, cfg.falloff_speed );
    relative_falloff_speed = (float)cfg.falloff_speed / plug.cy;
    load_ini_value( hini, cfg.display_percent );

    close_ini( hini );
  }

  // First get the routines
  _decoderPlayingSamples = (PFN)init->procs->output_playing_samples;
  _decoderPlaying        = (PFN)init->procs->decoder_playing;
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

  if ( ( plug.param && *plug.param && (dat = fopen(plug.param, "r")) )
     || ( dat = fopen("analyzer.pal","r")) )
    read_palette(dat);

  DiveSetSourcePalette( hdive, 0, sizeof( palette ) / sizeof( palette[0] ), (char*)&palette );
  DiveSetDestinationPalette( hdive, 0, 0, 0 );
  WinSetVisibleRegionNotify( hanalyzer, TRUE );

  // Apparently the WM_VRNENABLED message not necessarily automatically
  // follows WinSetVisibleRegionNotify call. We posts it manually.
  WinPostMsg( hanalyzer, WM_VRNENABLED, MPFROMLONG( TRUE ), 0 );
  return hanalyzer;
}

int _System
plugin_deinit( int unload )
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

  return 0;
}

