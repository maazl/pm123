/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *           2005 Dmitry A.Steklenev <glass@ptv.ru>
 *           2006-2011 Marcel Mueller
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
 */

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_GPI
#define  INCL_OS2MM
#define  PLUGIN_INTERFACE_LEVEL 3
#include <os2.h>
#include <os2me.h>
#include <stdio.h>
#include <stdlib.h>
#include <dive.h>
#include <fourcc.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#include <utilfct.h>
#include <format.h>
#include <visual_plug.h>
#include <plugin.h>
#include <fftw3.h>
#include <limits.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#include "analyzer.h"
#include "colormap.h"
#include "specana.h"

//#define DEBUG_LOG
#include <debuglog.h>

#define  TID_UPDATE ( TID_USERMAX - 1 )

#define CLR_BGR_BLACK   0 // Background
#define CLR_BGR_GREY    1 // grid
#define CLR_OSC_DIMMEST 2 // oscilloscope dark
#define CLR_OSC_BRIGHT  6 // oscilloscope light
#define CLR_ANA_BARS    7 // don't know
#define CLR_ANA_BOTTOM  8 // low bar
#define CLR_ANA_TOP     38// high bar
#define CLR_SPC_BOTTOM  40// dark spectrum
#define CLR_SPC_TOP     103// full power spectrum
#define CLR_SIZE        104

#define BARS_CY         3
#define BARS_SPACE      1

#define UM_UPDATE       WM_USER // Update analyzer


/* environment data */
static  VISPLUGININIT plug;
PLUGIN_CONTEXT Ctx;

#define load_prf_value(var) \
  Ctx.plugin_api->profile_query(#var, &var, sizeof var)

#define save_prf_value(var) \
  Ctx.plugin_api->profile_write(#var, &var, sizeof var)


/* configuration data */
static struct analyzer_cfg
{ ULONG update_delay;
  int   default_mode;
  int   falloff;
  int   falloff_speed;
  int   display_freq;
  int   display_lowfreq;
  BOOL  highprec_mode;
} cfg, active_cfg;

static  HAB    hab;
static  HWND   hconfigure;
static  HWND   hanalyzer;
static  HDIVE  hdive;
static  ULONG  image_id;
static  RGB2   palette[CLR_SIZE];

ULONG DLLENTRYP(OutPlayingPamples)(PM123_TIME, OUTPUT_PLAYING_BUFFER_CB, void*) = NULL;

/* working set initialized by init_bands */
static  int    LastSamplerate = 0;
static  float  RelativeFalloffSpeed;
static  float* Data       = NULL; // RAW sample data (always single channel)
static  int    NumSamples = 0;    // FFT length
static  WIN_FN WindowFunc;        // desired window function
static  float* Amps       = NULL; // FFT data
static  int    AmpsCount  = 0;    // FFT data size
static  float* Bars       = NULL; // destination channels
static  int    barsCount  = 0;    // destination channel count
static  int*   Scale      = NULL; // mapping FFT data -> destination channels
/* internal state */
static  bool   IsStopped;
static  bool   NeedInit;
static  bool   NeedClear;
static  bool   DestroyPending = false;


/********** read palette data **********************************************/

typedef struct
{ int osc_entr;
  int ana_entr;
  int spc_entr;
  color_entry osc_tab[6];
  color_entry ana_tab[31];
  color_entry spc_tab[64];
} interpolation_data;

/* transfer interpolation data into palette */
static bool do_interpolation(const interpolation_data* src)
{ if (src->osc_entr == 0 || src->ana_entr == 0 || src->spc_entr == 0)
    return false;
  // oscilloscope colors
  interpolate(palette + CLR_OSC_DIMMEST, CLR_OSC_BRIGHT - CLR_OSC_DIMMEST +1, src->osc_tab, src->osc_entr);
  // analyzer colors
  interpolate(palette + CLR_ANA_BOTTOM, CLR_ANA_TOP - CLR_ANA_BOTTOM +1, src->ana_tab, src->ana_entr);
  // spectroscope colors
  interpolate(palette + CLR_SPC_BOTTOM, CLR_SPC_TOP - CLR_SPC_BOTTOM +1, src->spc_tab, src->spc_entr);
  return true;
}

/* Removes comments */
static bool uncomment_slash( char *something )
{
  bool inquotes = false;
  bool nonwhitespace = false;

  for (;; ++something)
  { switch (*something)
    {case 0:
      return nonwhitespace;
     case ' ':
     case '\t':
     case '\n':
     case '\r':
      continue; // whitespace
     case '/':
      if (something[1] == '/' && !inquotes)
      { *something = 0;
        return nonwhitespace;
      }
      break;
     case '\"':
      inquotes = !inquotes;
    }
    nonwhitespace = true;
  }
}

/* Reads RGB colors from specified file. */
static bool read_color(const char* line, RGB2* color)
{
  int  r,g,b;
  if (sscanf(line, "%d,%d,%d", &r, &g, &b ) != 3)
    return false;

  color->bRed   = r;
  color->bGreen = g;
  color->bBlue  = b;
  return true;
}

/* Reads a palette from a stream */
static bool read_palette( FILE* dat )
{
  char line[256];
  color_entry* cur = NULL; // The NULL value is nonsens but gcc claims that cur might be uninitialized otherwide.
  interpolation_data ipd;
  RGB2 color;

  if (!fgets( line, sizeof( line ), dat ))
    return false;

  memset(palette, 0, sizeof palette);
  ipd.osc_entr = 0;
  ipd.ana_entr = 0;
  ipd.spc_entr = 0;

  // determine file version
  if (strnicmp(line, "// ANALYZER", 11) == 0)
  { // new file format
    int version;
    if (sscanf(line+11, "%d", &version) != 1 || version > 21)
      return false;
    DEBUGLOG(("NF: %d\n", version));

    // some defaults
    palette[CLR_BGR_GREY].bGreen = 90;
    palette[CLR_ANA_BARS].bGreen = 255;

    while (fgets( line, sizeof( line ), dat ))
    { size_t p;
      char* cp;
      float pos;
      char  percent = 0;

      if (!uncomment_slash(line))
        continue; // ignore logically empty lines

      p = strcspn(line, "-=");
      switch (line[p])
      {case '=': // normal parameter
        line[p] = 0;
        DEBUGLOG(("NF: %s = %s\n", line, line+p+1));
        if (stricmp(line, "BACKGROUND") == 0)
          read_color( line+p+1, &palette[CLR_BGR_BLACK] );
         else if (stricmp(line, "DOTS") == 0)
          read_color( line+p+1, &palette[CLR_BGR_GREY] );
         else if (stricmp(line, "PEAK") == 0)
          read_color( line+p+1, &palette[CLR_ANA_BARS] );
        break;
       case '-': // indexed parameter
        cp = strchr(line+p+1, '=');
        if (cp == NULL)
          break;
        line[p] = 0;
        *cp = 0;
        if (sscanf(line+p+1, "%f%c", &pos, &percent) < 1)
          break;
        if (!read_color( cp+1, &color ))
          break;
        DEBUGLOG(("NF-: %s - %f = %s = {%d,%d,%d}\n", line, pos, cp+1, color.bRed, color.bGreen, color.bBlue));
        if (stricmp(line, "OSCILLOSCOPE") == 0)
        { if (ipd.osc_entr == sizeof ipd.osc_tab / sizeof *ipd.osc_tab)
            break; // too many entries
          cur = ipd.osc_tab + ipd.osc_entr++;
          pos /= percent == '%' ? 100 : CLR_OSC_BRIGHT-CLR_OSC_DIMMEST;
        } else if (stricmp(line, "ANALYZER") == 0)
        { if (ipd.ana_entr == sizeof ipd.ana_tab / sizeof *ipd.ana_tab)
            break; // too many entries
          cur = ipd.ana_tab + ipd.ana_entr++;
          pos /= percent == '%' ? 100 : CLR_ANA_TOP-CLR_ANA_BOTTOM;
        } else if (stricmp(line, "SPECTROSCOPE") == 0)
        { if (ipd.spc_entr == sizeof ipd.spc_tab / sizeof *ipd.spc_tab)
            break; // too many entries
          cur = ipd.spc_tab + ipd.spc_entr++;
          pos /= percent == '%' ? 100 : CLR_SPC_TOP-CLR_SPC_BOTTOM;
        } else
          break;
        cur->Pos = pos;
        RGB2YDCyl(&cur->Color, &color);
        break;
      }
    }

  } else
  { // old file format
    int i = 1;
    do
    { if (!uncomment_slash(line))
        continue; // ignore logically empty lines
      DEBUGLOG(("OF: %d - %s\n", i, line));
      switch (i)
      {case 1:
        if (!read_color( line, &palette[CLR_BGR_BLACK] ))
          return false;
        goto c2;
       case 2:
        if (!read_color( line, &palette[CLR_BGR_GREY] ))
          return false;
        goto c2;
       case 24:
        if (!read_color( line, &palette[CLR_ANA_BARS] ))
          return false;
        goto OK;
       case 3:
        cur = ipd.ana_tab;
        break;
       case 19:
        cur = ipd.osc_tab;
        break;
      }
      if (i >= 3 && i <= 18)
        // analyzer colors
        cur->Pos = (18 - i) / 15.;
       else if (i >= 19 && i <= 23)
        // scope colors
        cur->Pos = (i - 19) / 4.;
      if (!read_color( line, &color ))
        return false;
      RGB2YDCyl(&cur->Color, &color);
      ++cur;
     c2:
      ++i;

    } while (fgets( line, sizeof( line ), dat ));

    if (i != 23)
      return false;
    palette[CLR_ANA_BARS] = palette[CLR_ANA_TOP];
   OK:
    ipd.osc_entr = 5;
    ipd.ana_entr = 16;
    ipd.spc_entr = 18;
    // extrapolations for new analyzer colors
    for (i = 0; i < 16; ++i)
    { ipd.spc_tab[i].Pos = .4 + ipd.ana_tab[i].Pos * .5;
      ipd.spc_tab[i].Color = ipd.ana_tab[i].Color;
    }
    ipd.spc_tab[16].Pos = 0.;
    RGB2YDCyl(&ipd.spc_tab[16].Color, &palette[CLR_BGR_BLACK]);
    ipd.spc_tab[17].Pos = 1.;
    RGB2YDCyl(&ipd.spc_tab[17].Color, &palette[CLR_ANA_BARS]);
  }

  // do the interpolation
  return do_interpolation(&ipd);
}

static void load_default_palette(void)
{ interpolation_data ipd;
  static const RGB2 black     = {  0,   0,   0};
  static const RGB2 green     = {  0, 255,   0};
  static const RGB2 dimgreen  = {  0, 180,   0};
  static const RGB2 darkgreen = {  0,  90,   0};
  static const RGB2 red       = {  0,   0, 255};
  static const RGB2 cyan      = {255, 255,   0};
  memset(palette, 0, sizeof palette);
  // fixed colors
  palette[CLR_BGR_GREY] = darkgreen;
  palette[CLR_ANA_BARS] = cyan;
  // oscilloscope colors
  ipd.osc_entr = 2;
  ipd.osc_tab[0].Pos = 0.;
  RGB2YDCyl(&ipd.osc_tab[0].Color, &dimgreen);
  ipd.osc_tab[1].Pos = .75;
  RGB2YDCyl(&ipd.osc_tab[1].Color, &green);
  // analyzer colors
  ipd.ana_entr = 2;
  ipd.ana_tab[0].Pos = 0.;
  RGB2YDCyl(&ipd.ana_tab[0].Color, &red);
  ipd.ana_tab[1].Pos = 1.;
  RGB2YDCyl(&ipd.ana_tab[1].Color, &green);
  // spectroscope colors
  ipd.spc_entr = 4;
  ipd.spc_tab[0].Pos = 0.;
  RGB2YDCyl(&ipd.spc_tab[0].Color, &black);
  ipd.spc_tab[1].Pos = .381;
  RGB2YDCyl(&ipd.spc_tab[1].Color, &red);
  ipd.spc_tab[2].Pos = .873;
  RGB2YDCyl(&ipd.spc_tab[2].Color, &green);
  ipd.spc_tab[3].Pos = 1.;
  RGB2YDCyl(&ipd.spc_tab[3].Color, &cyan);
  // do the interpolation
  do_interpolation(&ipd);
}


/********** filter design ***************************************************/

/* Free memory used by spectrum analyzer bands. */
static void free_bands()
{ DEBUGLOG(("analyzer:free_bands %p %p %p %p\n", Data, Amps, Bars, Scale));
  NumSamples = 0;
  AmpsCount = 0;
  barsCount = 0;
  fftwf_free(Data);
  delete[] Amps;
  delete[] Bars;
  delete[] Scale;
  Data  = NULL;
  Amps  = NULL;
  Bars  = NULL;
  Scale = NULL;
}

/* Initializes spectrum analyzer bands. */
static void init_bands(int samplerate)
{
  int    i;
  double step = 0;
  int    highfreq;

  // By default there are by far too many FP exceptions
  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );

  DEBUGLOG(("INI: samplerate = %d\n", samplerate));
  RelativeFalloffSpeed = (float)cfg.falloff_speed / plug.cy;

  if ( cfg.default_mode == active_cfg.default_mode
     && cfg.display_freq == active_cfg.display_freq
     && cfg.display_lowfreq == active_cfg.display_lowfreq
     && cfg.highprec_mode == active_cfg.highprec_mode
     && LastSamplerate == samplerate
     && Amps != NULL && Bars != NULL && Scale != NULL )
  { active_cfg = cfg; // no major change
    DEBUGLOG(("INI: minor change\n"));
    return;
  }
  active_cfg = cfg;
  LastSamplerate = samplerate;
  free_bands();

  // calculate number of channels = bars, FFT length and some other specific parameters
  highfreq = (samplerate >> 1) +1; // the +1 avoids roundoff noise.
  // Note that this will create an scale index out of bounds if the FFT winwow size is larger than the sampling rate (very unlikely).
  if (highfreq > active_cfg.display_freq)
    highfreq = active_cfg.display_freq;
  WindowFunc = WINFN_HAMMING;
  switch (active_cfg.default_mode)
  {case SHOW_OSCILLOSCOPE:
    NumSamples = plug.cx << 1;
    Data = (float*)fftwf_malloc(NumSamples * sizeof *Data);
   default:
    DEBUGLOG(("no FFT mode: %d\n", active_cfg.default_mode));
    return;

   case SHOW_ANALYZER:
   case SHOW_SPECTROSCOPE:
    barsCount = plug.cx;
    NumSamples = (barsCount * samplerate << active_cfg.highprec_mode) / (highfreq - active_cfg.display_lowfreq);
    break;

   case SHOW_BARS:
    barsCount = (plug.cx + BARS_SPACE) / (BARS_CY + BARS_SPACE);
    goto logcommon;

   case SHOW_LOGSPECSCOPE:
    barsCount = plug.cy;
   logcommon:
    step = log((double)highfreq / active_cfg.display_lowfreq) / barsCount; // limit range to the nyquist frequency
    NumSamples = (int)(samplerate / (active_cfg.display_lowfreq * exp(step)) * (1 << active_cfg.highprec_mode));
  }
  DEBUGLOG(("INI: mode = %d, bars = %d, numsamples = %d\n", active_cfg.default_mode, barsCount, NumSamples));

  // round up FFT length to the next power of 2
  frexp(NumSamples-1, &NumSamples); // floor(log2(numsamples-1))+1
  NumSamples = 1 << NumSamples; // 2**x
  AmpsCount = (NumSamples >> 1) +1;

  // initialize woring set
  Data  = (float*)fftwf_malloc(NumSamples * sizeof *Data);
  Amps  = new float[AmpsCount];
  Bars  = new float[barsCount];
  Scale = new int[barsCount+1];
  memset(Bars, 0, barsCount * sizeof *Bars);

  if (step != 0)
  { // logarithmic frequency scale
    double factor = (double)active_cfg.display_lowfreq * NumSamples / samplerate;
    int* sp = Scale;
    *sp++ = (int)ceil(factor);
    for (i = 1; i <= barsCount; i++)
    { *sp = (int)ceil(exp(step * i) * factor);
      if (sp[0] <= sp[-1])
      { if (i == 1 || sp[-2] < sp[0]-1)
          sp[-1] = sp[0] - 1;
         else
          sp[0] = sp[-1] + 1;
      }
      ++sp;
    }
  } else
  { // linear frequency scale
    double factor = (double)NumSamples / samplerate;
    int* sp = Scale;
    step = (highfreq - active_cfg.display_lowfreq) / barsCount;
    *sp++ = (int)ceil(active_cfg.display_lowfreq * factor);
    for (i = 1; i <= barsCount; i++)
    { *sp = (int)ceil((i * step + active_cfg.display_lowfreq) * factor);
      if (sp[0] <= sp[-1])
        sp[0] = sp[-1] +1;
      ++sp;
    }
  }
  #ifdef DEBUG_LOG
  DEBUGLOG(("INITSCALE: st = %f,\n", step));
  for (i = 0; i <= barsCount; ++i)
    DEBUGLOG2((" %d", Scale[i]));
  #endif
}


/********** analyzer updates ************************************************/

static void clear_analyzer()
{
  ULONG cx, cy;
  char* image;

  ORASSERT(DiveBeginImageBufferAccess( hdive, image_id, &image, &cx, &cy ));
  memset( image, 0, cx * cy );
  if (Bars != NULL)
    memset(Bars , 0, barsCount * sizeof *Bars);
  ORASSERT(DiveEndImageBufferAccess( hdive, image_id ));
  ORASSERT(DiveBlitImage( hdive, image_id, 0 ));
}

/// Do FFT analysis of captured samples
static bool do_analysis()
{ switch (specana_do(Data, NumSamples, WINFN_HAMMING, Amps))
  {default:
    DEBUGLOG(("specana_do ERROR\n"));
   case SPECANA_UNCHANGED:
    DEBUGLOG(("specana_do UNCHANGED\n"));
    return false;
   case SPECANA_OK:
    return true;
  }
}

/* get intensity of a bin described by scp[0..1] in Bel (= 10*dB) */
static float get_barvalue(int* scp)
{ float a = 0;
  int e;
  for (e = scp[0]; e < scp[1]; e++)
    a += Amps[e];
  return log10( a / (scp[1] - scp[0]) // nornalize for number of added channels
    * sqrt(scp[0] * LastSamplerate / (double)NumSamples) // Scale by sqrt(frequency / 100)
    + 1E-5 );
}

static inline void update_barvalue(float* val, float z)
{ if (z < 0)
    z = 0;
   else if (z >= 1)
    z = .999999;

  if( !active_cfg.falloff || *val-RelativeFalloffSpeed <= z )
    *val = z;
   else
    *val -= RelativeFalloffSpeed;
}

static void draw_grid(char* image, unsigned long image_cx)
{ int x,y;
  for( y = 0; y < plug.cy; y += 2 )
  { for( x = 0; x < plug.cx; x += 2 )
      image[( y * image_cx ) + x ] = CLR_BGR_GREY;
  }
}

/* Do the regular update of the analysis window.
 * This is called at the WM_TIMER event of the analyzer window.
 */
static void update_analyzer()
{ DEBUGLOG2(("analyzer:update_analyzer %lu %d %d\n", hdive, needinit, needclear));

  long i, x, y;
  unsigned long image_cx, image_cy;
  char* image = NULL;

  // do fft analysis?
  if (active_cfg.default_mode == SHOW_DISABLED)
    return;
  if (active_cfg.default_mode != SHOW_OSCILLOSCOPE && !do_analysis())
    return; // no changes or error

  ORASSERT(DiveBeginImageBufferAccess( hdive, image_id, &image, &image_cx, &image_cy ));

  if (NeedClear)
  { memset( image, 0, image_cx * image_cy );
    if (Bars != NULL)
      memset(Bars , 0, barsCount * sizeof *Bars);
    NeedClear = false;
  }

  DEBUGLOG(("analyzer:update_analyzer: before switch %d %p\n", active_cfg.default_mode, image));
  switch (active_cfg.default_mode)
  {case SHOW_ANALYZER:
    memset( image, 0, image_cx * image_cy );
    draw_grid(image, image_cx);

    for( x = 0; x < plug.cx; x++ )
    {
      update_barvalue( Bars+x, .3 + .5 * get_barvalue(Scale + x) ); // 40dB range [-12dB..+28dB]

      for( y = (int)(Bars[x]*(plug.cy+1)) -1; y >= 0; y-- )
      {
        int color = (int)(( CLR_ANA_TOP - CLR_ANA_BOTTOM ) * y / (plug.cy * (1-Bars[x]))) + CLR_ANA_BOTTOM;
        image[( plug.cy - y -1) * image_cx + x ] = min( CLR_ANA_TOP, color );
      }
    }
    break;

   case SHOW_BARS:
    memset( image, 0, image_cx * image_cy );
    draw_grid(image, image_cx);

    for( i = 0; i < barsCount; i++ )
    {
      update_barvalue( Bars+i, .3 + .5 * get_barvalue(Scale + i) ); // 40dB range [-12dB..+28dB]
      //fprintf(stderr, "B: %d %d %f %f\n", i, bars_count, get_barvalue(scale + i), bars[i]);

      for( y = (int)(Bars[i]*(plug.cy+1)) -1; y >= 0; y-- )
      {
        char* ip = image + (plug.cy - y -1) * image_cx + (BARS_CY + BARS_SPACE) * i;
        int color = ( CLR_ANA_TOP - CLR_ANA_BOTTOM ) * y / (plug.cy-1) + CLR_ANA_BOTTOM;
        //int color = ( CLR_ANA_TOP - CLR_ANA_BOTTOM +1 ) * y / (plug.cy * (1-bars[i])) + CLR_ANA_BOTTOM;
        //color = min( CLR_ANA_TOP, color );
        for(x = BARS_CY; x; --x )
          *ip++ = color;
      }
    }
    break;

   case SHOW_SPECTROSCOPE:
    { char* ip = image + (plug.cy-1) * image_cx; // bottom line
      if (IsStopped)
        // first call after restart
        memset(image, 0, (plug.cy-1) * image_cx);
       else
        // move image one pixel up
        memmove(image, image+image_cx, (plug.cy-1) * image_cx);

      for( x = 0; x < plug.cx; x++ )
      { update_barvalue( Bars+x, .4 + .4 * get_barvalue(Scale + x) ); // 50dB range [-20dB..+30dB]
        ip[x] = (char)(( CLR_SPC_TOP - CLR_SPC_BOTTOM +1 ) * Bars[x]) + CLR_SPC_BOTTOM;
      }
    }
    break;

   case SHOW_LOGSPECSCOPE:
    { char* ip = image + (plug.cx-1); // last pixel, first line
      if (IsStopped)
        // first call after restart
        memset(image, 0, plug.cy*image_cx);
       else
        // move image left one pixel
        memmove(image, image+1, plug.cy*image_cx -1);

      for( i = 0; i < barsCount; i++ )
      { update_barvalue( Bars+i, .4 + .4 * get_barvalue(Scale + i) ); // 50dB range [-20dB..+30dB]
        ip[(plug.cy-i-1)*image_cx] = (char)(( CLR_SPC_TOP - CLR_SPC_BOTTOM +1 ) * Bars[i]) + CLR_SPC_BOTTOM;
      }
    }
    break;

   case SHOW_OSCILLOSCOPE:
    { float* sample = Data;

      memset( image, 0, image_cx * image_cy );
      draw_grid(image, image_cx);

      for( x = 0; x < plug.cx; x++ )
      {
        int color;
        double z = *sample++;

        y = (long)(image_cy * (1 + z) / 2);
        if (y >= 0 && y < image_cy)
        { color = (int)(( CLR_OSC_BRIGHT - CLR_OSC_DIMMEST + 2 ) * fabs(z)) + CLR_OSC_DIMMEST;
          image[ y * image_cx + x ] = min( CLR_OSC_BRIGHT, color );
        }
      }
    }
    break;
  }

  if (image != NULL)
  { ORASSERT(DiveEndImageBufferAccess( hdive, image_id ));
    ORASSERT(DiveBlitImage( hdive, image_id, 0 ));
  }
  DEBUGLOG(("analyzer:update_analyzer at end\n"));
  IsStopped = false;
}

/********** interface to the output plug-in *********************************/

/// Number of samples stored by outputsamplescb. -1 => no request pending.
static int stored_samples = -1;

static void DLLENTRY output_samples_cb(void* arg, const FORMAT_INFO2* format, const float* samples, int count, PM123_TIME pos, BOOL* done)
{ DEBUGLOG(("analyzer:output_samples_cb(%p, {%i, %i}, %p, %i, %f, *%d) - %i, %i\n",
    arg, format->samplerate, format->channels, samples, count, pos, *done, stored_samples, NumSamples));

  if (stored_samples < 0)
  { *done = TRUE;
    return;
  }

  if (NeedInit || LastSamplerate != format->samplerate)
  { init_bands(format->samplerate);
    NeedInit = false;
  }

  if (!Data || count == 0)
  { // something went wrong
    *done = TRUE;
    if (hanalyzer)
      WinPostMsg(hanalyzer, UM_UPDATE, MPFROMLONG(FALSE), 0);
    return;
  }

  float* dp = Data + stored_samples;
  stored_samples += count;
  if (stored_samples >= NumSamples)
  { // we got enough samples
    count = NumSamples - stored_samples + count;
    *done = TRUE;
  } else if (*done)
    // not enough samples => pad with zero
    memset(dp + count, 0, (NumSamples - stored_samples) * sizeof(float));

  // copy samples
  switch (format->channels)
  {case 1:
    memcpy(dp, samples, count * sizeof(float));
    break;
   case 2:
    { float* dpe = dp + count;
      while (dp != dpe)
      { *dp++ = (samples[0] + samples[1]) / 2;
        samples += 2;
      }
      break;
    }
   default:
    { float* dpe = dp + count;
      while (dp != dpe)
      { float s;
        int ch = format->channels;
        while (ch)
          s += *samples++;
        *dp++ = s / format->channels;
      }
      break;
    }
  }

  if (*done && hanalyzer)
    WinPostMsg(hanalyzer, UM_UPDATE, MPFROMLONG(TRUE), 0);
}

/// Request samples from the output plug-in
static inline void request_samples()
{ DEBUGLOG2(("analyzer:request_samples() - %i\n", stored_samples));
  // Avoid more than one request at a time.
  if ( InterlockedCxc((volatile unsigned*)&stored_samples, (unsigned)-1, 0) == (unsigned)-1
    && (*OutPlayingPamples)(0, &output_samples_cb, NULL) )
    stored_samples = -1;
}

static MRESULT EXPENTRY plg_win_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case DM_DRAGOVER:
    case DM_DROP:
    case 0x041f:
    case 0x041e:
    case WM_CHAR:
    case WM_CONTEXTMENU:
    case WM_BUTTON2MOTIONSTART:
    case WM_BUTTON1MOTIONSTART:
      // Delegate some messages to the player
      return WinSendMsg( plug.hwnd, msg, mp1, mp2 );

    case WM_VRNENABLED:
      DEBUGLOG(("analyzer:plg_win_proc: WM_VRNENABLED: %p - %u\n", hdive, cfg.update_delay));
      if (hdive)
      { HPS     hps  = WinGetPS( hwnd );
        HRGN    hrgn = GpiCreateRegion( hps, 0, NULL );

        RGNRECT rgn_control;
        RECTL   rgn_rectangles[64];
        SWP     swp;
        POINTL  pos;

        DEBUGLOG(("analyzer:plg_win_proc: WM_VRNENABLED: %p\n", hrgn));
        if (hrgn)
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

            ORASSERT(DiveSetupBlitter( hdive, &setup ));
          }

          GpiDestroyRegion( hps, hrgn );
        }

        WinReleasePS ( hps );
        WinStartTimer( hab, hanalyzer, TID_UPDATE, cfg.update_delay );
        stored_samples = -1;
      }
      break;

    case WM_VRNDISABLED:
      DEBUGLOG(("analyzer:plg_win_proc: WM_VRNDISABLED: %p\n", hdive));
      if (hdive)
      { WinStopTimer( hab, hanalyzer, TID_UPDATE );
        DiveSetupBlitter( hdive, 0 );
      }
      break;

    case WM_BUTTON1DBLCLK:
      if( ++cfg.default_mode > SHOW_DISABLED ) {
        cfg.default_mode = SHOW_ANALYZER;
      }
      NeedClear = true;
      NeedInit = true;

      DEBUGLOG(("CLICK! %d\n", cfg.default_mode));
      break;

    case WM_TIMER:
      DEBUGLOG2(("analyzer:plg_win_proc: WM_TIMER %u\n", SHORT1FROMMP(mp1)));
      if (cfg.default_mode != SHOW_DISABLED)
        request_samples();
      break;

    case UM_UPDATE:
      update_analyzer();
      stored_samples = -1;
      break;

    case WM_DESTROY: // we deinitialize here to avoid access to memory objects that are already freed.
      DEBUGLOG(("analyzer:plg_win_proc: WM_DESTROY: %p\n", hdive));
      if (hdive)
      { ORASSERT(DiveFreeImageBuffer( hdive, image_id ));
        ORASSERT(DiveClose( hdive ));
        hdive = 0;
      }

      free_bands();
      specana_uninit();

      memset(&active_cfg, 0, sizeof active_cfg);

      hanalyzer = 0;

      DestroyPending = false;
      break;

    default:
      return WinDefWindowProc( hwnd, msg, mp1, mp2 );
  }
  return 0;
}

HWND DLLENTRY vis_init( const VISPLUGININIT* init )
{
  FILE* dat;
  int   i;
  int   display_percent;

  DEBUGLOG(("analyzer:vis_init: %s\n", plug.param));
  // wait for old instances to complete destruction, since many variables are static.
  for (i = 0; DestroyPending; ++i)
  { if (i > 10)
      return 0; // can't help
    DosSleep(1);
  }

  memcpy( &plug, init, sizeof( VISPLUGININIT ));

  cfg.update_delay    = 31;
  cfg.default_mode    = SHOW_BARS;
  cfg.falloff         = 1;
  cfg.falloff_speed   = 2;
  cfg.display_freq    = -1;
  display_percent     = -1;
  cfg.display_lowfreq = 50;
  cfg.highprec_mode   = FALSE;

  load_prf_value( cfg.update_delay );
  load_prf_value( cfg.default_mode );
  load_prf_value( cfg.falloff );
  load_prf_value( cfg.falloff_speed );
  load_prf_value( cfg.display_freq );
  Ctx.plugin_api->profile_query( "cfg.display_percent", &display_percent, sizeof display_percent);
  load_prf_value( cfg.highprec_mode );

  DEBUGLOG(("vis_init: %d, %d\n", cfg.display_freq, display_percent));
  if (cfg.display_freq <= 0)
  { if (display_percent <= 0)
      cfg.display_freq = 18000;
     else
      cfg.display_freq = display_percent * 22050 / 100; // for compatibility
  }

  // mask unreasonable values
  if (cfg.display_freq < 5000)
    cfg.display_freq = 5000;
  if (cfg.default_mode > SHOW_DISABLED || cfg.default_mode < 0)
    cfg.default_mode = SHOW_DISABLED;
  cfg.highprec_mode = !!cfg.highprec_mode;


  // First get the routines
  OutPlayingPamples = init->procs->output_playing_samples;

  hab = 0;
  hconfigure = 0;
  hanalyzer = 0;
  hdive = 0;
  image_id = 0;
  IsStopped = true;
  LastSamplerate = 0;
  NeedInit = true;
  NeedClear = true;
  IsStopped = false;
  stored_samples = -1;

  // Open up DIVE
  if( DiveOpen( &hdive, FALSE, 0 ) != DIVE_SUCCESS ) {
    DEBUGLOG(("analyzer:vis_init: DiveOpen failed.\n"));
    return 0;
  }

  /* Allocate image buffer (256-color) */
  if( DiveAllocImageBuffer( hdive, &image_id, FOURCC_LUT8,
                            plug.cx, plug.cy, 0, NULL ) != DIVE_SUCCESS )
  {
    DEBUGLOG(("analyzer:vis_init: DiveAllocImageBuffer failed.\n"));
    DiveClose( hdive );

    hdive = 0;

    return 0;
  }
  DEBUGLOG(("analyzer: DIVE initialized hdive = %p, image_id = %p\n", hdive, image_id));

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
                               999,
                               NULL,
                               NULL  );

  if ( plug.param && *plug.param && (dat = fopen(plug.param, "r")) )
  { if (!read_palette(dat))
      load_default_palette();
    fclose(dat);
  } else
    load_default_palette();

  ORASSERT(DiveSetSourcePalette( hdive, 0, sizeof( palette ) / sizeof( palette[0] ), (char*)&palette ));
  WinSetVisibleRegionNotify( hanalyzer, TRUE );

  // Apparently the WM_VRNENABLED message not necessarily automatically
  // follows WinSetVisibleRegionNotify call. We posts it manually.
  WinPostMsg( hanalyzer, WM_VRNENABLED, MPFROMLONG( TRUE ), 0 );

  return hanalyzer;
}

int DLLENTRY plugin_deinit( int unload )
{
  DEBUGLOG(("analyzer:plugin_deinit(%u)\n", unload));
  save_prf_value( cfg.update_delay );
  save_prf_value( cfg.default_mode );
  save_prf_value( cfg.falloff );
  save_prf_value( cfg.falloff_speed );
  save_prf_value( cfg.display_freq );
  save_prf_value( cfg.highprec_mode );

  DestroyPending = true;
  WinStopTimer( hab, hanalyzer, TID_UPDATE );

  if (hconfigure)
    WinDismissDlg( hconfigure, 0 );

  WinDestroyWindow( hanalyzer );
  // continue in window procedure

  return 0;
}

/********** plug-in interface, GUI stuff ************************************/

/* Returns information about plug-in. */
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* query)
{
  query->type         = PLUGIN_VISUAL;
  query->author       = "Samuel Audet, Dmitry A.Steklenev, Marcel Mueller";
  query->desc         = "Spectrum Analyzer 2.10";
  query->configurable = TRUE;
  query->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/* init plug-in */
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ Ctx = *ctx;
  return 0;
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      hconfigure = hwnd;
      do_warpsans( hwnd );

      WinCheckButton( hwnd, RB_ANALYZER + cfg.default_mode, TRUE );
      WinCheckButton( hwnd, CB_FALLOFF, cfg.falloff );
      WinCheckButton( hwnd, CB_HIGHPREC, cfg.highprec_mode );

      WinSendDlgItemMsg( hwnd, SB_MAXFREQ,   SPBM_SETLIMITS,
                         MPFROMLONG( 24 ), MPFROMLONG( 5 )); // lower values may decrease speed significantly
      WinSendDlgItemMsg( hwnd, SB_MAXFREQ,   SPBM_SETCURRENTVALUE,
                         MPFROMLONG( (cfg.display_freq+500)/1000 ), 0 );
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

      WinSendDlgItemMsg( hwnd, SB_MAXFREQ, SPBM_QUERYVALUE,
                         MPFROMP( &value ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));

      cfg.display_freq = value*1000;

      cfg.highprec_mode = WinQueryButtonCheckstate( hwnd, CB_HIGHPREC );

      NeedInit = true;
      NeedClear = true;

      hconfigure  = NULLHANDLE;
      break;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
HWND DLLENTRY plugin_configure( HWND hwnd, HMODULE module )
{
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  return NULL;
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

