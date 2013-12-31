/*
 * Copyright 2008-2011 Marcel Mueller
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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

/* EQ hard coded to 32 1/3 octave bands, kinda combersome to change realtime
   if we want band widths that are a "good" fraction of an octave */

#define  INCL_PM
#define  INCL_DOS
#include <os2.h>
#include <stdlib.h>

#include <limits.h>
#include <math.h>
#include <float.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>

#define PLUGIN_INTERFACE_LEVEL 3

#include <utilfct.h>
#include <fftw3.h>
#include <format.h>
#include <filter_plug.h>
#include <plugin.h>
#include "realeq.h"

//#define DEBUG 2
#include <debuglog.h>

#define PLUGIN "Real Equalizer 1.23"
#define MAX_COEF 32768
#define MIN_COEF   512
#define MAX_FIR  16384
#define MIN_FIR    256
#define NUM_BANDS   32

// Equalizer filter frequencies
static const float Frequencies[NUM_BANDS] =
{ 15.84893192,
  19.95262315,
  25.11886432,
  31.62277660,
  39.81071706,
  50.11872336,
  63.09573445,
  79.43282347,
  100.0000000,
  125.8925412,
  158.4893192,
  199.5262315,
  251.1886432,
  316.2277660,
  398.1071706,
  501.1872336,
  630.9573445,
  794.3282347,
  1000.000000,
  1258.925412,
  1584.893192,
  1995.262315,
  2511.886432,
  3162.277660,
  3981.071706,
  5011.872336,
  6309.573445,
  7943.282347,
  10000.00000,
  12589.25412,
  15848.93192,
  19952.62315
};

static const PLUGIN_CONTEXT* context;

#define load_prf_value(var) \
  context->plugin_api->profile_query(#var, &var, sizeof var)

#define save_prf_value(var) \
  context->plugin_api->profile_write(#var, &var, sizeof var)

static BOOL eqneedinit;
static BOOL eqneedFIR;
static BOOL eqneedEQ;
static HWND hdialog     = NULLHANDLE;
static BOOL plugininit  = FALSE; // plug-in is initialized (only once)

static float bandgain[2][NUM_BANDS+1];  // gain in dB, first entry ist master gain
static float groupdelay[2][NUM_BANDS+1];// gruop delay in 1ms, first entry ist master delay
static BOOL  mute[2][NUM_BANDS+1];      // mute flags (TRUE = mute), first entry is master mute

// for FFT convolution...
static struct
{ float* inbox;               // buffer to collect incoming samples
  float* time_domain;         // buffer in the time domain
  fftwf_complex* freq_domain; // buffer in the frequency domain (shared memory with design)
  //float* design;              // buffer to design filter kernel (shared memory with freq_domain)
  fftwf_complex* kernel[2];   // since the kernel is real even, it's even real in the time domain
  float* overlap[2];          // keep old samples for convolution
  fftwf_complex* channel_save;// buffer to keep the frequency domain of a mono input for a second convolution
  fftwf_plan forward_plan;    // fftw plan for time_domain -> freq_domain
  fftwf_plan backward_plan;   // fftw plan for freq_domain -> time_domain
  //fftwf_plan DCT_plan;        // fftw plan for design -> time domain
  //fftwf_plan RDCT_plan;       // fftw plan for time_domain -> kernel
  int    FIRorder;            // filter kernel length
  int    plansize;            // plansize for the FFT convolution
  //int    DCTplansize;         // plansize for the filter design
} FFT;

// settings
static int  newFIRorder; // this is for the GUI only
static int  newPlansize; // this is for the GUI only
static BOOL eqenabled = FALSE;
static BOOL locklr    = FALSE;
static char lasteq[CCHMAXPATH];
static enum
{ EQ_private,
  EQ_file,
  EQ_modified
} eqstate = EQ_private; // Flag whether the current settings are a modified version of the above file.

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_LN10
#define M_LN10 2.302585093
#endif

/* Hamming window
#define WINDOW_FUNCTION( n, N ) (0.54 - 0.46 * cos( 2 * M_PI * n / N ))
 * Well, since the EQ does not provide more than 24dB dynamics
 * we should use a much more aggressive window function.
 * This is verified by calculations.
 */
#define WINDOW_FUNCTION( n, N ) (0.77 - 0.23 * cos( 2 * M_PI * n / N ))

#define round(n) ((n) > 0 ? (n) + 0.5 : (n) - 0.5)

typedef struct FILTER_STRUCT {

   ULONG  DLLENTRYP(output_command)       (struct FILTER_STRUCT* a, ULONG msg, const OUTPUT_PARAMS2* info);
   int    DLLENTRYP(output_request_buffer)(struct FILTER_STRUCT* a, const FORMAT_INFO2* format, float** buf);
   void   DLLENTRYP(output_commit_buffer) (struct FILTER_STRUCT* a, int len, PM123_TIME pos);
   void*  a;

   FORMAT_INFO2 format;

   PM123_TIME posmarker; // starting point of the inbox buffer
   int    inboxlevel;    // number of samples in inbox buffer
   int    latency;       // samples to discard before passing them to the output stage
   BOOL   enabled;       // flag whether the EQ was enabled at the last call to request_buffer
   BOOL   discard;       // TRUE: discard buffer content before next request_buffer
   PM123_TIME temppos;   // Position returned during discard

} REALEQ_STRUCT;


/********** Ini file stuff */

static void save_ini(void)
{
  DEBUGLOG(("realeq:save_ini\n"));

  save_prf_value ( newFIRorder );
  save_prf_value ( newPlansize );
  save_prf_value ( eqenabled );
  save_prf_value ( locklr );
  save_prf_value ( eqstate );
  context->plugin_api->profile_write( "lasteq", lasteq, strlen(lasteq) );
  save_prf_value ( bandgain );
  save_prf_value ( groupdelay );
  save_prf_value ( mute );

  DEBUGLOG(("realeq:save_ini - completed\n"));
}

static void load_ini(void)
{
  DEBUGLOG(("realeq:load_ini\n"));

  memset(bandgain,   0, sizeof bandgain  );
  memset(groupdelay, 0, sizeof groupdelay);
  memset(mute,       0, sizeof mute      );

  eqenabled   = FALSE;
  memset(lasteq, 0, sizeof lasteq);
  newPlansize = 8192;
  newFIRorder = 4096;
  locklr      = FALSE;
  eqstate     = EQ_private;

  load_prf_value( newFIRorder );
  load_prf_value( newPlansize );
  load_prf_value( eqenabled );
  load_prf_value( locklr );
  load_prf_value( eqstate );
  load_prf_value( lasteq );
  load_prf_value( bandgain );
  load_prf_value( groupdelay );
  load_prf_value( mute );

  // avoid crash when INI-Content is bad
  if (newPlansize < 16)
    newPlansize = 16;
   else if (newPlansize > MAX_COEF)
    newPlansize = MAX_COEF;
  if (newPlansize <= newFIRorder)
    newFIRorder = newPlansize >> 1;
  if (newFIRorder > MAX_FIR)
    newFIRorder = MAX_FIR;

  eqneedinit  = TRUE;
  DEBUGLOG(("realeq:load_ini - completed\n"));
}

static double TodB(double gain)
{ if (gain <= 0.)
    return 0.;
/*   else if (gain <= .2511886432)
    return -12.;
   else if (gain >= 3.981071706)
    return 12.;*/
   else
    return 20.*log10(gain);
}

INLINE double ToGain(double dB)
{ return exp(dB/20.*M_LN10);
}

static BOOL load_eq_file(char* filename)
{
  FILE* file;
  int   i = 0;
  char  line[256];
  DEBUGLOG(("realeq:load_eq_file(%s)\n", filename));

  if (filename == NULL || *filename == 0)
    return FALSE;
  file = fopen( filename, "r" );
  if( file == NULL ) {
    DEBUGLOG(("realeq:load_eq_file: failed, error %i\n", errno));
    return FALSE;
  }

  // once we get here, we clear the fields
  memset(bandgain,   0, sizeof bandgain  );
  memset(groupdelay, 0, sizeof groupdelay);
  memset(mute,       0, sizeof mute      );

  while( !feof( file ))
  {
    fgets( line, sizeof(line), file );
    blank_strip( line );
    if( *line && line[0] != '#' && line[0] != ';' )
    { if( i < NUM_BANDS )           // left gain
      { sscanf(line, "%f %f", bandgain[0]+i+1, groupdelay[0]+i+1);
        if (bandgain[0][i+1] != 0)
          bandgain[0][i+1] = TodB(bandgain[0][i+1]);
        groupdelay[0][i+1] *= 1000.;
      } else if( i < NUM_BANDS*2 )   // right gain
      { sscanf(line, "%f %f", bandgain[1]+i-NUM_BANDS+1, groupdelay[1]+i-NUM_BANDS+1);
        if (bandgain[1][i-NUM_BANDS+1] != 0)
          bandgain[1][i-NUM_BANDS+1] = TodB(bandgain[1][i-NUM_BANDS+1]);
        groupdelay[1][i-NUM_BANDS+1] *= 1000.;
      } else if( i < NUM_BANDS*3 )   // left mute
      { mute[0][i-NUM_BANDS*2+1] = atoi(line);
      } else if( i < NUM_BANDS*4 )   // right mute
      { mute[1][i-NUM_BANDS*3+1] = atoi(line);
      } else if( i < NUM_BANDS*4+2 ) // master gain
      { sscanf(line, "%f %f", bandgain[i-NUM_BANDS*4], groupdelay[i-NUM_BANDS*4]);
        if (bandgain[i-NUM_BANDS*4][0] != 0)
          bandgain[i-NUM_BANDS*4][0] = TodB(bandgain[i-NUM_BANDS*4][0]);
        groupdelay[i-NUM_BANDS*4][0] *= 1000.;
      } else if( i < NUM_BANDS*4+4 ) // master mute
      { mute[i-NUM_BANDS*4-2][0] = atoi(line);
      } else
        break;
      i++;
    }
  }
  fclose( file );
  DEBUGLOG(("realeq:load_eq_file: OK\n"));
  return TRUE;
}

static void init_request(void)
{
  DEBUGLOG(("realeq:init_request\n"));
  if (!plugininit) // first time?
  { load_ini();
    if ( eqstate == EQ_file ) {
      load_eq_file( lasteq );
    }
    plugininit = TRUE;
  }
}

static void trash_buffers(REALEQ_STRUCT* f)
{ DEBUGLOG(("realeq::trash_buffers(%p) - %d %d\n", f, f->inboxlevel, f->latency));
  f->inboxlevel = 0;
  f->latency    = -1;
  memset(FFT.overlap[0], 0, FFT.plansize * 2 * sizeof(float)); // initially zero
}

typedef struct
{ float lf; // log frequency
  float lv; // log value
  float de; // group delay
} EQcoef;

/* setup FIR kernel */
static BOOL fil_setup(REALEQ_STRUCT* f)
{
  static BOOL FFTinit = FALSE;
  int   i, channel;
  float fftspecres;
  // equalizer design data
  EQcoef coef[NUM_BANDS+4]; // including surounding points

  // dispatcher to skip code fragments
  if (!eqneedinit)
  { if (eqneedFIR)
      goto doFIR;
    if (eqneedEQ)
      goto doEQ;
    return TRUE;
  }

  // STEP 1: initialize buffers and FFT plans

  // free old resources
  if (FFTinit)
  { DEBUGLOG(("fil_setup: free\n"));
    free(FFT.inbox);
    fftwf_destroy_plan(FFT.forward_plan);
    fftwf_destroy_plan(FFT.backward_plan);
    //fftwf_destroy_plan(FFT.RDCT_plan);
    fftwf_free(FFT.freq_domain);
    fftwf_free(FFT.time_domain);
    // do not free aliased arrays!
    free(FFT.overlap[0]);
    fftwf_free(FFT.kernel[0]);
  }

  // copy global parameters for thread safety and round up to next power of 2
  frexp(newPlansize-1, &i); // floor(log2(plansize-1))+1
  FFT.plansize = 2 << i; // 2**(i+1)
  // reduce size at low sampling rates
  frexp(f->format.samplerate/8000, &i); // floor(log2(samprate))+1, i >= 0
  FFT.plansize >>= 4-min(4,i); // / 2**(4-i)
  DEBUGLOG(("I: %d\n", FFT.plansize));

  // allocate buffers
  FFT.inbox       = (float*)malloc(FFT.plansize * 2 * sizeof(float));
  FFT.freq_domain = (fftwf_complex*)fftwf_malloc((FFT.plansize/2+1) * sizeof *FFT.freq_domain);
  FFT.time_domain = (float*)fftwf_malloc((FFT.plansize+1) * sizeof *FFT.time_domain);
  //FFT.design      = FFT.freq_domain[0]; // Aliasing!
  FFT.overlap[0]  = (float*)malloc(FFT.plansize * 2 * sizeof(float));
  FFT.overlap[1]  = FFT.overlap[0] + FFT.plansize;
  FFT.channel_save= (fftwf_complex*)FFT.overlap[1] -1; // Aliasing!
  FFT.kernel[0]   = (fftwf_complex*)fftwf_malloc((FFT.plansize/2+1) * 2 * sizeof(fftwf_complex));
  FFT.kernel[1]   = FFT.kernel[0]  + (FFT.plansize/2+1);

  // prepare real 2 complex transformations
  FFT.forward_plan  = fftwf_plan_dft_r2c_1d( FFT.plansize, FFT.time_domain, FFT.freq_domain, FFTW_ESTIMATE );
  FFT.backward_plan = fftwf_plan_dft_c2r_1d( FFT.plansize, FFT.freq_domain, FFT.time_domain, FFTW_ESTIMATE );
  // prepare real 2 real transformations
  //FFT.RDCT_plan     = fftwf_plan_r2r_1d( FFT.plansize/2+1, FFT.time_domain, FFT.kernel[0], FFTW_REDFT00, FFTW_ESTIMATE );

  trash_buffers(f);

  eqneedinit = FALSE;

 doFIR:
  // STEP 2: setup FIR order

  // copy global parameters for thread safety
  FFT.FIRorder = (newFIRorder+15) & -16; /* multiple of 16 */
  FFT.FIRorder <<= 1; // * 2
  frexp(f->format.samplerate/8000, &i); // floor(log2(samprate))+1, i >= 0
  FFT.FIRorder >>= 4-min(4,i); // / 2**(4-i)

  if( FFT.FIRorder < 2 || FFT.FIRorder >= FFT.plansize)
  { (*context->plugin_api->message_display)(MSG_ERROR, "very bad! The FIR order and/or the FFT plansize is invalid or the FIRorder is higer or equal to the plansize.");
    eqenabled = FALSE; // avoid crash
    return FALSE;
  }

  /*// calculate optimum plansize for kernel generation
  frexp((FFT.FIRorder<<1) -1, &FFT.DCTplansize); // floor(log2(2*FIRorder-1))+1
  FFT.DCTplansize = 1 << FFT.DCTplansize; // 2**x
  // free old resources
  if (FFTinit)
    fftwf_destroy_plan(FFT.DCT_plan);
  // prepare real 2 real transformations
  FFT.DCT_plan = fftwf_plan_r2r_1d(FFT.DCTplansize/2+1, FFT.design, FFT.time_domain, FFTW_REDFT00, FFTW_ESTIMATE);*/

  DEBUGLOG(("P: FIRorder: %d, Plansize: %d\n", FFT.FIRorder, FFT.plansize));
  eqneedFIR = FALSE;

 doEQ:
  // STEP 3: setup filter kernel

  fftspecres = (float)f->format.samplerate / FFT.plansize;

  // Prepare design coefficients frame
  coef[0].lf = -14; // very low frequency
  coef[0].de = 0;
  coef[1].lf = M_LN10; // subsonic point
  coef[1].de = 0;
  for (i = 0; i < NUM_BANDS; ++i)
    coef[i+2].lf = log(Frequencies[i]);
  coef[NUM_BANDS+2].lf = log(32000); // keep higher frequencies at 0 dB
  coef[NUM_BANDS+2].lv = 0;
  coef[NUM_BANDS+2].de = 0;
  coef[NUM_BANDS+3].lf = 14; // very high frequency
  coef[NUM_BANDS+3].lv = 0;
  coef[NUM_BANDS+3].de = 0;

  /* for left, right */
  for( channel = 0; channel < 2; channel++ )
  {
    float log_mute_level;

    // muteall?
    if (mute[channel][0])
    { // clear channel
      memset(FFT.kernel[channel], 0, (FFT.plansize/2+1) * sizeof(*FFT.kernel[channel]));
      continue;
    }

    // mute = master gain -36dB. More makes no sense since the stopband attenuation
    // of the window function does not allow better results.
    coef[1].lv = coef[0].lv = log_mute_level = (bandgain[channel][0]-36)/20.*M_LN10;

    // fill band data
    for (i = 1; i <= NUM_BANDS; ++i)
    { coef[i+2-1].lv = mute[channel][i]
       ? log_mute_level
       : (bandgain[channel][i]+bandgain[channel][0]) / 20. * M_LN10;
      coef[i+2-1].de = (groupdelay[channel][0] + groupdelay[channel][i]) / 1000.;
    }

    // compose frequency spectrum
    { EQcoef* cop = coef;
      double phase = 0;
      FFT.freq_domain[0][0] = 0.; // no DC
      FFT.freq_domain[0][1] = 0.;
      for (i = 1; i <= FFT.plansize/2; ++i) // do not start at f=0 to avoid log(0)
      { const float f = i * fftspecres; // current frequency
        double pos;
        double val = log(f);
        while (val > cop[1].lf)
          ++cop;
        // integrate phase for group delay
        pos = (log(f)-cop[0].lf) / (cop[1].lf-cop[0].lf);
        phase += (cop[0].de + pos * (cop[1].de - cop[0].de)) * fftspecres * (2*M_PI);
        // do double logarithmic sine^2 interpolation for amplitude
        pos = .5 - .5*cos(M_PI * pos);
        val = exp(cop[0].lv + pos * (cop[1].lv - cop[0].lv));
        // convert to cartesian coordinates
        FFT.freq_domain[i][0] = val * cos(phase);
        FFT.freq_domain[i][1] = val * sin(phase);
        DEBUGLOG2(("F: %i, %f, %f -> %f = %f dB, %f = %f ms@ %f\n",
          i, f, pos, val, TodB(val), phase*180./M_PI, cop[0].de + pos * (cop[1].de - cop[0].de), exp(cop[0].lf)));
    } }

    // transform into the time domain
    fftwf_execute(FFT.backward_plan);
    #if defined(DEBUG_LOG) && DEBUG_LOG > 1
    for (i = 0; i <= FFT.plansize/2; ++i)
      DEBUGLOG2(("TK: %i, %g\n", i, FFT.time_domain[i]));
    #endif

    // normalize, apply window function and store results symmetrically
    { float* sp1 = FFT.time_domain;
      float* sp2 = FFT.time_domain + FFT.plansize;
      double tmp;
      *sp1++ /= FFT.plansize * FFT.plansize;
      DEBUGLOG2(("K: %i, %g\n", FFT.FIRorder/2, sp1[-1]));
      for (i = FFT.FIRorder/2 -1; i >= 0; --i)
      { tmp = WINDOW_FUNCTION(i, FFT.FIRorder) / (FFT.plansize * FFT.plansize); // normalize for next FFT
        *sp1++ *= tmp;
        *--sp2 *= tmp;
        DEBUGLOG2(("K: %i, %g, %g\n", i, sp1[-1], sp2[0]));
      }
      // padding
      memset(sp1, 0, (sp2-sp1) * sizeof *FFT.time_domain);
    }

    // prepare for FFT convolution
    // transform back into the frequency domain, now with window function
    fftwf_execute_dft_r2c(FFT.forward_plan, FFT.time_domain, FFT.kernel[channel]);
    #if defined(DEBUG_LOG) && DEBUG_LOG > 1
    for (i = 0; i <= FFT.plansize/2; ++i)
      DEBUGLOG2(("FK: %i, %g, %g\n", i, FFT.kernel[channel][i][0], FFT.kernel[channel][i][1]));
    DEBUGLOG2(("E: kernel completed.\n"));
    #endif
  }

  eqneedEQ = FALSE;
  FFTinit = TRUE;
  return TRUE;
}

/* do convolution and back transformation
 */
static void do_fft_convolute(fftwf_complex* sp, fftwf_complex* kp)
{ int l;
  float tmp;
  fftwf_complex* dp = FFT.freq_domain;
  DEBUGLOG(("realeq:do_fft_convolute(%p, %p) - %p\n", sp, kp, dp));
  // Convolution in the frequency domain is simply a series of complex products.
  for (l = (FFT.plansize/2+1) >> 3; l; --l)
  { DO_8(p,
      tmp = sp[p][0] * kp[p][0] - sp[p][1] * kp[p][1];
      dp[p][1] = sp[p][1] * kp[p][0] + sp[p][0] * kp[p][1];
      dp[p][0] = tmp;
    );
    sp += 8;
    kp += 8;
    dp += 8;
  }
  for (l = (FFT.plansize/2+1) & 7; l; --l)
  { tmp = sp[0][0] * kp[0][0] - sp[0][1] * kp[0][1];
    dp[0][1] = sp[0][1] * kp[0][0] + sp[0][0] * kp[0][1];
    dp[0][0] = tmp;
    ++sp;
    ++kp;
    ++dp;
  }
  // do IFFT
  fftwf_execute(FFT.backward_plan);
}

/* fetch saved samples */
INLINE void do_fft_load_overlap(float* overlap_buffer)
{ memcpy(FFT.time_domain + FFT.plansize - FFT.FIRorder/2, overlap_buffer, FFT.FIRorder/2 * sizeof *FFT.time_domain);
  memcpy(FFT.time_domain, overlap_buffer + FFT.FIRorder/2, FFT.FIRorder/2 * sizeof *FFT.time_domain);
}

/* store last FIRorder samples */
static void do_fft_save_overlap(float* overlap_buffer, int len)
{
  DEBUGLOG(("realeq:do_fft_save_overlap(%p, %i) - %i\n", overlap_buffer, len, FFT.FIRorder));
  if (len < FFT.FIRorder/2)
  { memmove(overlap_buffer, overlap_buffer + len, (FFT.FIRorder - len) * sizeof *overlap_buffer);
    memcpy(overlap_buffer + FFT.FIRorder - len, FFT.time_domain + FFT.FIRorder/2, len * sizeof *overlap_buffer);
  } else
    memcpy(overlap_buffer, FFT.time_domain + len - FFT.FIRorder/2, FFT.FIRorder * sizeof *overlap_buffer);
}

/* store the samples in the FFT.time_domain buffer
   The samples are shifted by FIRorder/2 to the right.
   FIRorder samples before the starting position are taken from overlap_buffer.
   The overlap_buffer is updated with the last FIRorder samples before return.
 */
static void do_fft_load_samples_mono(const float* sp, const int len, float* overlap_buffer)
{ float* dp = FFT.time_domain + FFT.FIRorder/2;
  DEBUGLOG(("realeq:do_fft_load_samples_mono(%p, %i, %p) - %i\n", sp, len, overlap_buffer));
  do_fft_load_overlap(overlap_buffer);
  // fetch new samples
  memcpy(dp, sp, len * sizeof *dp);
  memset(dp + len, 0, (FFT.plansize - FFT.FIRorder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  do_fft_save_overlap(overlap_buffer, len);
}
/* store the samples in the FFT.time_domain buffer
 * This will cover only one channel. The only difference to to mono-version is that
 * the source pointer is incremented by 2 per sample.
 */
static void do_fft_load_samples_stereo(const float* sp, const int len, float* overlap_buffer)
{ int l;
  float* dp = FFT.time_domain + FFT.FIRorder/2;
  DEBUGLOG(("realeq:do_fft_load_samples_stereo(%p, %i, %p) - %i\n", sp, len, overlap_buffer));
  do_fft_load_overlap(overlap_buffer);
  for (l = len >> 3; l; --l) // coarse
  { DO_8(p, dp[p] = sp[p<<1]);
    sp += 16;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
  { *dp++ = *sp;
    sp += 2;
  }
  memset(dp, 0, (FFT.plansize - FFT.FIRorder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  do_fft_save_overlap(overlap_buffer, len);
}

/* store stereo samples
 * This will cover only one channel. The only difference to to mono-version is that
 * the destination pointer is incremented by 2 per sample.
 */
static void do_fft_store_samples_stereo(float* sp, const int len)
{ float* dp = FFT.time_domain;
  int l;
  DEBUGLOG(("realeq:do_fft_store_samples_stereo(%p, %i)\n", sp, len));
  // transfer samples
  for (l = len >> 3; l; --l) // coarse
  { DO_8(p, sp[p<<1] = dp[p]);
    sp += 16;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
  { *sp = *dp++;
    sp += 2;
  }
}

static void filter_samples_fft(REALEQ_STRUCT* f, float* newsamples, const float* buf, int len)
{ DEBUGLOG(("realeq:filter_samples_fft(%p, %p, %p, %i)\n", f, newsamples, buf, len));

  while (len) // we might need more than one FFT cycle
  { int l2 = FFT.plansize - FFT.FIRorder;
    if (l2 > len)
      l2 = len;

    if ( f->format.channels == 2 )
    { // left channel
      do_fft_load_samples_stereo(buf, l2, FFT.overlap[0]);
      // do FFT
      fftwf_execute(FFT.forward_plan);
      // convolution
      do_fft_convolute(FFT.freq_domain, FFT.kernel[0]);
      // convert back
      do_fft_store_samples_stereo(newsamples, l2);
      // right channel
      do_fft_load_samples_stereo(buf+1, l2, FFT.overlap[1]);
      // do FFT
      fftwf_execute(FFT.forward_plan);
      // convolution
      do_fft_convolute(FFT.freq_domain, FFT.kernel[1]);
      // convert back
      do_fft_store_samples_stereo(newsamples+1, l2);
      // next block
      len -= l2;
      l2 <<= 1; // stereo
      buf += l2;
      newsamples += l2;
    } else
    { // left channel
      do_fft_load_samples_mono(buf, l2, FFT.overlap[0]);
      // do FFT
      fftwf_execute(FFT.forward_plan);
      // save data for 2nd channel
      memcpy(FFT.channel_save, FFT.freq_domain, (FFT.plansize/2+1) * sizeof *FFT.freq_domain);
      // convolution
      do_fft_convolute(FFT.freq_domain, FFT.kernel[0]);
      // convert back
      do_fft_store_samples_stereo(newsamples, l2);
      // right channel
      // convolution
      do_fft_convolute(FFT.channel_save, FFT.kernel[1]);
      // convert back
      do_fft_store_samples_stereo(newsamples+1, l2);
      // next block
      len -= l2;
      buf += l2;
      l2 <<= 1; // stereo
      newsamples += l2;
    }
  }
}

/* only update the overlap buffer, no filtering */
static void filter_samples_new_overlap(REALEQ_STRUCT* f, const float* buf, int len)
{ int l2;
  DEBUGLOG(("realeq:filter_samples_new_overlap(%p, %p, %i)\n", f, buf, len));

  l2 = FFT.plansize - FFT.FIRorder;
  if (len > l2)
  { // skip unneeded samples
    buf += (len - l2) * f->format.channels;
    len = l2;
  }

  // Well, there might be a more optimized version without copying the old overlap buffer, but who cares
  if ( f->format.channels == 2 )
  { do_fft_load_samples_stereo(buf  , l2, FFT.overlap[0]);
    do_fft_load_samples_stereo(buf+1, l2, FFT.overlap[1]);
  } else
  { do_fft_load_samples_mono(buf, l2, FFT.overlap[0]);
  }
}

/* Proxy functions to remove the first samples to compensate for the filter delay */
static int do_request_buffer(REALEQ_STRUCT* f, float** buf)
{ DEBUGLOG(("realeq:do_request_buffer(%p, %p) - %d\n", f, buf, f->latency));
  if (f->latency != 0)
  { if (f->latency < 0)
      f->latency = FFT.FIRorder >> 1;
    *buf = NULL; // discard
    return f->latency;
  } else
  { FORMAT_INFO2 fi = f->format;
    fi.channels = 2; // result is always stereo
    return (*f->output_request_buffer)(f->a, &fi, buf);
  }
}

static void do_commit_buffer(REALEQ_STRUCT* f, int len, PM123_TIME posmarker)
{ DEBUGLOG(("realeq:do_commit_buffer(%p, %d, %f) - %d\n", f, len, posmarker, f->latency));
  if (f->latency != 0)
    f->latency -= len;
  else
    (*f->output_commit_buffer)(f->a, len, posmarker);
}

/* applies the FFT filter to the current inbox content and sends the result to the next plug-in.
 * You should not call this function with f->inboxlevel == 0.
 */
static void filter_and_send(REALEQ_STRUCT* f)
{
  int    len, dlen;
  float* dbuf;
  DEBUGLOG(("realeq:filter_and_send(%p) - %d\n", f, f->inboxlevel));

  // request destination buffer
  dlen = do_request_buffer( f, &dbuf );

  if (f->discard)
  { f->inboxlevel = 0;
    do_commit_buffer(f, 0, f->temppos); // no-op
    return;
  }

  len = f->inboxlevel; // required destination length
  if (dlen < len)
  { // with fragmentation
    int rem = len;
    float* sp;

    filter_samples_fft(f, FFT.inbox, FFT.inbox, f->inboxlevel);

    // transfer data
    sp = FFT.inbox;
    for(;;)
    { if (dbuf != NULL)
        memcpy(dbuf, sp, dlen * sizeof(float) * 2);
      do_commit_buffer( f, dlen, f->posmarker + (double)(len-rem)/f->format.samplerate );
      rem -= dlen;
      if (rem == 0)
        break;
      sp += dlen * 2;
      // request next buffer
      dlen = do_request_buffer( f, &dbuf );
      if (f->discard)
      { f->inboxlevel = 0;
        do_commit_buffer(f, 0, f->temppos); // no-op
        return;
      }
      if (dlen > rem)
        dlen = rem;
    }

  } else
  { // without fragmentation
    if (dbuf == NULL)
      // only save overlap
      filter_samples_new_overlap(f, FFT.inbox, f->inboxlevel);
    else
      filter_samples_fft(f, dbuf, FFT.inbox, f->inboxlevel);
    do_commit_buffer(f, len, f->posmarker);
  }

  f->posmarker += (double)f->inboxlevel/f->format.samplerate;
  f->inboxlevel = 0;
}

static void local_flush(REALEQ_STRUCT* f)
{ // emulate input of some zeros to compensate for filter size
  int   len = (FFT.FIRorder+1) >> 1;
  DEBUGLOG(("realeq:local_flush(%p) - %d %d %d\n", f, len, f->inboxlevel, f->latency));
  while (len != 0)
  { int dlen = FFT.plansize - FFT.FIRorder - f->inboxlevel;
    if (dlen > len)
      dlen = len;
    memset(FFT.inbox + f->inboxlevel * f->format.channels, 0, dlen * f->format.channels * sizeof(float));
    // commit buffer
    f->inboxlevel += dlen;
    if (f->inboxlevel == FFT.plansize - FFT.FIRorder)
    { // enough data, apply filter
      filter_and_send(f);
      if (f->discard)
        break;
    }
    len -= dlen;
  }

  // flush buffer
  if (f->inboxlevel != 0)
    filter_and_send(f);

  // setup for restart
  trash_buffers(f);
}

/* Entry point: do filtering */
static int DLLENTRY filter_request_buffer(REALEQ_STRUCT* f, const FORMAT_INFO2* format, float** buf)
{ BOOL enabled;
  #ifdef DEBUG_LOG
  if (format != NULL)
    DEBUGLOG(("realeq:filter_request_buffer(%p, {%u, %u}, %p) - %d %d\n",
     f, format->samplerate, format->channels, buf, f->inboxlevel, f->latency));
   else
    DEBUGLOG(("realeq:filter_request_buffer(%p, %p, %p) - %d %d\n", f, format, buf, f->inboxlevel, f->latency));
  #endif

  enabled = eqenabled && buf != NULL && ( format->channels == 1 || format->channels == 2 );
  if (enabled && !f->enabled)
  { // enable EQ
    f->enabled = TRUE;
    f->latency = -1;
  } else if (!enabled && f->enabled)
  { // disable EQ
    local_flush(f);
    f->enabled = FALSE;
    if (buf == NULL) // global flush();
      return (*f->output_request_buffer)(f->a, format, buf);
  }

  if ( f->enabled )
  {
    if (f->discard)
    { trash_buffers(f);
      f->discard = FALSE;
    }
    if ( f->format.samplerate != format->samplerate || f->format.channels != format->channels )
    { if (f->format.samplerate != 0)
        local_flush( f );
      f->format.samplerate = format->samplerate;
      f->format.channels   = format->channels;
      eqneedinit = TRUE;
    } else if (eqneedFIR || eqneedinit)
    { if (f->format.samplerate != 0)
        local_flush( f );
    }

    fil_setup( f );

    *buf = FFT.inbox + f->inboxlevel * format->channels;
    DEBUGLOG(("realeq:filter_request_buffer: %p, %d\n", *buf, FFT.plansize - FFT.FIRorder - f->inboxlevel));
    return FFT.plansize - FFT.FIRorder - f->inboxlevel;
  }
  else
  {
    return (*f->output_request_buffer)( f->a, format, buf );
  }
}

static void DLLENTRY filter_commit_buffer(REALEQ_STRUCT* f, int len, PM123_TIME posmarker)
{
  DEBUGLOG(("realeq:filter_commit_buffer(%p, %u, %f) - %d %d\n", f, len, posmarker, f->inboxlevel, f->latency));

  if (!f->enabled)
  { (*f->output_commit_buffer)( f->a, len, posmarker );
    return;
  }

  if (f->inboxlevel == 0)
    // remember position and precompensate for filter delay
    f->posmarker = posmarker - (double)(FFT.FIRorder>>1)/f->format.samplerate;

  f->inboxlevel += len;

  if (f->inboxlevel == FFT.plansize - FFT.FIRorder)
  { // enough data, apply filter
    filter_and_send(f);
  }
}

static ULONG DLLENTRY filter_command(REALEQ_STRUCT* f, ULONG msg, const OUTPUT_PARAMS2* info)
{ ULONG rc;
  INFO_BUNDLE_CV ib;
  TECH_INFO ti;
  OUTPUT_PARAMS2 new_info;
  DEBUGLOG(("realeq:filter_command(%p, %u, %p)\n", f, msg, info));
  if (info->Info->tech->channels == 1 && eqenabled)
  { new_info = *info;
    ib = *new_info.Info;
    ti = *ib.tech;
    ti.channels = 2;
    ib.tech = &ti;
    new_info.Info = &ib;
    info = &new_info;
  }
  switch (msg)
  {case OUTPUT_TRASH_BUFFERS:
    f->temppos = info->PlayingPos;
    f->discard = TRUE;
    break;
   case OUTPUT_CLOSE:
    f->temppos = f->posmarker;
    f->discard = TRUE;
    break;
  }
  rc = (*f->output_command)(f->a, msg, info);
  return rc;
}

/********** Entry point: Initialize
*/
ULONG DLLENTRY filter_init(REALEQ_STRUCT** F, FILTER_PARAMS2* params)
{
  REALEQ_STRUCT* f = (REALEQ_STRUCT*)malloc(sizeof(REALEQ_STRUCT));
  DEBUGLOG(("filter_init(%p->%p, {%u, ..., %p, ..., %p})\n", F, f, params->size, params->a, params->w));

  *F = f;

  init_request();
  eqneedinit = TRUE;

  f->output_command        = params->output_command;
  f->output_request_buffer = params->output_request_buffer;
  f->output_commit_buffer  = params->output_commit_buffer;
  f->a                     = params->a;
  f->inboxlevel            = 0;
  f->enabled               = FALSE; // flag is set later
  f->discard               = FALSE;

  f->format.samplerate     = 0;
  f->format.channels       = 0;

  params->output_command        = &filter_command;
  params->output_request_buffer = &filter_request_buffer;
  params->output_commit_buffer  = &filter_commit_buffer;
  return 0;
}

void DLLENTRY filter_update(REALEQ_STRUCT *f, const FILTER_PARAMS2 *params)
{ DosEnterCritSec();
  f->output_command        = params->output_command;
  f->output_request_buffer = params->output_request_buffer;
  f->output_commit_buffer  = params->output_commit_buffer;
  f->a                     = params->a;
  DosExitCritSec();
}

BOOL DLLENTRY filter_uninit(REALEQ_STRUCT* f)
{
  DEBUGLOG(("realeq:filter_uninit(%p)\n", f));

  free(f);

  return TRUE;
}


/********** GUI stuff *******************************************************/

static BOOL save_eq(HWND hwnd)
{
  FILEDLG filedialog;
  FILE*   file;
  int     i, e;
  DEBUGLOG(("realeq:save_eq\n"));

  memset( &filedialog, 0, sizeof(FILEDLG));
  filedialog.cbSize   = sizeof(FILEDLG);
  filedialog.fl       = FDS_CENTER | FDS_SAVEAS_DIALOG;
  filedialog.pszTitle = "Save Equalizer as...";

  if( lasteq[0] == 0 ) {
    strcpy( filedialog.szFullFile, "*.REQ" );
  } else {
    strncpy( filedialog.szFullFile, lasteq, sizeof filedialog.szFullFile );
  }

  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    eqstate = EQ_file;
    file = fopen( filedialog.szFullFile, "w" );
    if( file == NULL ) {
      DEBUGLOG(("realeq:save_eq: failed\n"));
      return FALSE;
    }
    strncpy( lasteq, filedialog.szFullFile, sizeof lasteq );
    eqstate = EQ_file;

    fprintf( file, "#\n# Equalizer created with %s\n# Do not modify!\n#\n", VERSION );
    fprintf( file, "# Band gains\n" );
    for( e = 0; e < 2; ++e )
      for( i = 1; i <= NUM_BANDS; i++ )
        fprintf( file, "%g %g\n", ToGain(bandgain[e][i]), groupdelay[e][i]/1000. );
    fprintf(file, "# Mutes\n" );
    for( e = 0; e < 2; ++e )
      for( i = 1; i <= NUM_BANDS; i++ )
        fprintf(file, "%lu\n", mute[e][i]);
    fprintf( file, "# Preamplifier\n" );
    fprintf( file, "%g %g\n%g %g\n", ToGain(bandgain[0][0]), groupdelay[0][0]/1000., ToGain(bandgain[1][0]), groupdelay[1][0]/1000. );
    fprintf( file, "# Master Mute\n" );
    fprintf( file, "%lu\n%lu\n", mute[0][0], mute[1][0] );
    fprintf( file, "# End of equalizer\n" );
    fclose ( file );
    DEBUGLOG(("realeq:save_eq: OK\n"));
    return TRUE;
  }

  return FALSE;
}

static void drivedir(char* buf, char* fullpath)
{
  char drive[_MAX_DRIVE],
       path [_MAX_PATH ];

  _splitpath( fullpath, drive, path, NULL, NULL );
  strcpy( buf, drive );
  strcat( buf, path  );
}

static BOOL load_eq(HWND hwnd)
{
  FILEDLG filedialog;
  DEBUGLOG(("realeq:load_eq: OK\n"));

  memset( &filedialog, 0, sizeof( FILEDLG ));
  filedialog.cbSize = sizeof(FILEDLG);
  filedialog.fl = FDS_CENTER | FDS_OPEN_DIALOG;
  filedialog.pszTitle = "Load Equalizer";
  drivedir( filedialog.szFullFile, lasteq );
  strcat( filedialog.szFullFile, "*.REQ" );

  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    if ( load_eq_file( filedialog.szFullFile ) ) {
      strncpy( lasteq, filedialog.szFullFile, sizeof lasteq );
      eqstate = EQ_file;
      return TRUE;
    } else {
      return FALSE;
    }
  }
  return FALSE;
}

int DLLENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{ param->type         = PLUGIN_FILTER;
  param->author       = "Samuel Audet, Marcel Mueller";
  param->desc         = PLUGIN;
  param->configurable = TRUE;
  param->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/* init plug-in */
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ context = ctx;
  return 0;
}

/* set slider of channel, band to value
   channel 0 = left, channel 1 = right
   band 0..31 = EQ, -1 = master
*/
static void set_slider(HWND hwnd, int channel, int band, double value)
{ MRESULT rangevalue;
  int     id = ID_BANDL + (ID_BANDR-ID_BANDL)*channel + band;
  DEBUGLOG2(("realeq:set_slider(%p, %d, %d, %f)\n", hwnd, channel, band, value));

  if (value < -12)
  { DEBUGLOG(("realeq:set_slider: value out of range %d, %d, %f\n", channel, band, value));
    value = -12;
  } else if (value > 12)
  { DEBUGLOG(("realeq:set_slider: value out of range %d, %d, %f\n", channel, band, value));
    value = 12;
  }

  rangevalue = WinSendDlgItemMsg( hwnd, id, SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

  WinSendDlgItemMsg( hwnd, id, SLM_SETSLIDERINFO,
                     MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                     MPFROMSHORT( (value/24.+.5) * (SHORT2FROMMR(rangevalue) - 1) +.5 ));
}

static void load_sliders(HWND hwnd)
{ int e,i;
  float (*dp)[2][NUM_BANDS+1] = WinQueryButtonCheckstate( hwnd, ID_GROUPDELAY ) ? &groupdelay : &bandgain;
  for( e = 0; e < 2; e++ )
    for( i = 0; i <= NUM_BANDS; i++ )
      set_slider( hwnd, e, i-1, (*dp)[e][i] );
}

static void load_dialog(HWND hwnd)
{
  int     i, e;
  float   (*dp)[2][NUM_BANDS+1] = WinQueryButtonCheckstate( hwnd, ID_GROUPDELAY ) ? &groupdelay : &bandgain;
  SHORT   range = SHORT2FROMMR( WinSendDlgItemMsg( hwnd, ID_BANDL, SLM_QUERYSLIDERINFO, MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 ) );
  HWND    hwnd_ctrl;

  for( e = 0; e < 2; e++ ) {
    for( i = 0; i <= NUM_BANDS; i++ ) {
      hwnd_ctrl = WinWindowFromID( hwnd, ID_MASTERL + (ID_MASTERR-ID_MASTERL)*e + i );

      // mute check boxes
      WinSendDlgItemMsg( hwnd, ID_MUTEALLL + (ID_MUTEALLR-ID_MUTEALLL)*e + i, BM_SETCHECK, MPFROMCHAR( mute[e][i] ), 0 );

      // sliders
      WinSendMsg( hwnd_ctrl, SLM_ADDDETENT,
                         MPFROMSHORT( range - 1 ), 0 );
      WinSendMsg( hwnd_ctrl, SLM_ADDDETENT,
                         MPFROMSHORT( range >> 1 ), 0 );
      WinSendMsg( hwnd_ctrl, SLM_ADDDETENT,
                         MPFROMSHORT( 0 ), 0 );

      DEBUGLOG2(("load_dialog: %d %d %g\n", e, i, (*dp)[e][i]));
      set_slider( hwnd, e, i-1, (*dp)[e][i] );
    }
  }

  // eq enabled check box
  WinSendDlgItemMsg( hwnd, EQ_ENABLED,  BM_SETCHECK, MPFROMSHORT( eqenabled ), 0 );
  WinSendDlgItemMsg( hwnd, ID_LOCKLR,   BM_SETCHECK, MPFROMSHORT( locklr ), 0 );

  // last file
  hwnd_ctrl = WinWindowFromID( hwnd, EQ_FILE );
  WinSetWindowBits( hwnd_ctrl, QWL_STYLE, DT_HALFTONE * (eqstate == EQ_modified), DT_HALFTONE );
  WinSetWindowText( hwnd_ctrl, eqstate == EQ_private ? "" : lasteq );

  hwnd_ctrl = WinWindowFromID( hwnd, ID_FIRORDER );
  WinSendMsg( hwnd_ctrl, SPBM_SETMASTER, MPFROMLONG( NULLHANDLE ), 0 );
  WinSendMsg( hwnd_ctrl, SPBM_SETLIMITS, MPFROMLONG( MAX_FIR ), MPFROMLONG( MIN_FIR ));
  WinSendMsg( hwnd_ctrl, SPBM_SETCURRENTVALUE, MPFROMLONG( newFIRorder ), 0 );
  hwnd_ctrl = WinWindowFromID( hwnd, ID_PLANSIZE );
  WinSendMsg( hwnd_ctrl, SPBM_SETMASTER, MPFROMLONG( NULLHANDLE ), 0 );
  WinSendMsg( hwnd_ctrl, SPBM_SETLIMITS, MPFROMLONG( MAX_COEF ), MPFROMLONG( MIN_COEF ));
  WinSendMsg( hwnd_ctrl, SPBM_SETCURRENTVALUE, MPFROMLONG( newPlansize ), 0 );
}

static MRESULT EXPENTRY ConfigureDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  static char nottheuser = FALSE;

  switch( msg )
  {
    case WM_INITDLG:
      WinSendDlgItemMsg( hwnd, ID_GAIN,       BM_SETCHECK, MPFROMSHORT( TRUE  ), 0 );
      WinSendDlgItemMsg( hwnd, ID_GROUPDELAY, BM_SETCHECK, MPFROMSHORT( FALSE ), 0 );

      WinSetDlgItemText( hwnd, ID_UTXTL, "+12dB");
      WinSetDlgItemText( hwnd, ID_CTXTL, "0dB");
      WinSetDlgItemText( hwnd, ID_BTXTL, "-12dB");
      WinSetDlgItemText( hwnd, ID_UTXTR, "+12dB");
      WinSetDlgItemText( hwnd, ID_CTXTR, "0dB");
      WinSetDlgItemText( hwnd, ID_BTXTR, "-12dB");
      nottheuser = TRUE;
      load_dialog( hwnd );
      nottheuser = FALSE;
      break;

    case WM_DESTROY:
      hdialog = NULLHANDLE;
      break;

    case WM_CLOSE:
      save_ini();
      WinDestroyWindow( hwnd );
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ))
      {
        case EQ_SAVE:
          save_eq( hwnd );
          break;

        case EQ_LOAD:
          load_eq( hwnd );
          nottheuser = TRUE;
          load_dialog( hwnd );
          nottheuser = FALSE;
          eqneedEQ = TRUE;
          break;

        case EQ_ZERO:
        {
          memset(bandgain,   0, sizeof bandgain);
          memset(groupdelay, 0, sizeof groupdelay);
          memset(mute,       0, sizeof mute);
          eqstate = EQ_private;
          nottheuser = TRUE;
          load_dialog( hwnd );
          nottheuser = FALSE;

          eqneedEQ = TRUE;
          //*lasteq = 0; maybe it's better to keep the file...
          break;
        }
      }
      return 0;

    case WM_CONTROL:
    {
      int id = SHORT1FROMMP( mp1 );

      if( nottheuser ) {
        return 0;
      }

      switch( id )
      {
        ULONG temp;
        int i;

        case EQ_ENABLED:
          eqenabled = WinQueryButtonCheckstate( hwnd, id );
          break;

        case ID_LOCKLR:
          locklr = WinQueryButtonCheckstate( hwnd, id );
          break;

        case ID_FIRORDER:
          switch( SHORT2FROMMP( mp1 ))
          {
            case SPBN_CHANGE:
              WinSendDlgItemMsg( hwnd, id, SPBM_QUERYVALUE, MPFROMP( &temp ), 0 );
              if (temp > MAX_FIR || temp >= newPlansize)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG( newFIRorder ), 0 ); // restore
               else
              { newFIRorder = temp;
                eqneedFIR = TRUE; // no init required when only the FIRorder changes
              }
              break;

            case SPBN_UPARROW:
              frexp(newFIRorder, &i); // floor(log2(FIRorder-1))+1
              i = 1 << (i-1);
              i >>= 2;
              temp = (newFIRorder & ~(MIN_FIR-1)) + max(MIN_FIR, i);
              if (temp < newPlansize)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG( temp ), 0 );
              break;

            case SPBN_DOWNARROW:
              frexp(newFIRorder-1, &i); // floor(log2(FIRorder-1))+1
              i = 1 << (i-1);
              i >>= 2;
              temp = (newFIRorder & ~(MIN_FIR-1)) - max(MIN_FIR, i);
              if (temp >= 16)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG( temp ), 0 );
              break;
          }
          break;

        case ID_PLANSIZE:
          switch( SHORT2FROMMP( mp1 ))
          {
            case SPBN_CHANGE:
              WinSendDlgItemMsg( hwnd, id, SPBM_QUERYVALUE, MPFROMP(&temp), 0 );
              if (temp > MAX_COEF || temp <= newFIRorder)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG(newPlansize), 0 ); // restore
               else
              { newPlansize = temp;
                eqneedinit = TRUE;
              }
              break;

            case SPBN_UPARROW:
              temp = newPlansize << 1;
              if (temp > MAX_COEF)
                temp = MAX_COEF;
              WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG(temp), 0 );
              break;

            case SPBN_DOWNARROW:
              temp = newPlansize >> 1;
              if (temp <= newFIRorder)
              { int i;
                frexp(newFIRorder, &i);
                temp = 1 << i; // round up to next power of two
              }
              WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG(temp), 0 );
              break;
          }
          break;

        case ID_GAIN:
        { WinSetDlgItemText( hwnd, ID_UTXTL, "+12dB");
          WinSetDlgItemText( hwnd, ID_CTXTL, "0dB");
          WinSetDlgItemText( hwnd, ID_BTXTL, "-12dB");
          WinSetDlgItemText( hwnd, ID_UTXTR, "+12dB");
          WinSetDlgItemText( hwnd, ID_CTXTR, "0dB");
          WinSetDlgItemText( hwnd, ID_BTXTR, "-12dB");
          nottheuser = TRUE;
          load_sliders( hwnd );
          nottheuser = FALSE;
          break;
        }
        case ID_GROUPDELAY:
        { WinSetDlgItemText( hwnd, ID_UTXTL, "+12ms");
          WinSetDlgItemText( hwnd, ID_CTXTL, "0ms");
          WinSetDlgItemText( hwnd, ID_BTXTL, "-12ms");
          WinSetDlgItemText( hwnd, ID_UTXTR, "+12ms");
          WinSetDlgItemText( hwnd, ID_CTXTR, "0ms");
          WinSetDlgItemText( hwnd, ID_BTXTR, "-12ms");
          nottheuser = TRUE;
          load_sliders( hwnd );
          nottheuser = FALSE;
          break;
        }

        default:

          // mute check boxes
          if( id >= ID_MUTEALLL && id <= ID_MUTEEND ) {

            switch( SHORT2FROMMP( mp1 ))
            {
              case BN_CLICKED:
              case BN_DBLCLICKED:
              {
                int channel = 0, index = SHORT1FROMMP(mp1);

                if (eqstate == EQ_file)
                {
                  HWND ctrl = WinWindowFromID( hwnd, EQ_FILE );
                  eqstate = EQ_modified;
                  WinSetWindowBits( ctrl, QWL_STYLE, DT_HALFTONE, DT_HALFTONE );
                  WinInvalidateRect( ctrl, NULL, FALSE );
                }

                index -= ID_MUTEALLL;
                if ( index >= ID_MUTEALLR-ID_MUTEALLL ) {
                  index -= ID_MUTEALLR-ID_MUTEALLL;
                  channel = 1;
                }

                mute[channel][index] = SHORT1FROMMR( WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), BM_QUERYCHECK, 0, 0 ) );

                if( locklr )
                {
                  nottheuser = TRUE;
                  WinSendDlgItemMsg( hwnd, ID_MUTEALLL + (ID_MUTEALLR-ID_MUTEALLL)*(channel^1) + index, BM_SETCHECK,
                                     MPFROMSHORT( mute[channel^1][index] = mute[channel][index] ), 0 );
                  nottheuser = FALSE;
                }

                eqneedEQ = TRUE;
              }
            }
          // sliders
          } else if( id >= ID_MASTERL && id <= ID_BANDEND ) {

            float (*dp)[2][NUM_BANDS+1] = WinQueryButtonCheckstate( hwnd, ID_GROUPDELAY ) ? &groupdelay : &bandgain;

            switch( SHORT2FROMMP( mp1 ))
            {
              case SLN_CHANGE:
              {
                int channel = 0, index = SHORT1FROMMP(mp1), val;
                MRESULT rangevalue;
                BOOL    needeq = FALSE;

                if (eqstate == EQ_file)
                {
                  HWND ctrl = WinWindowFromID( hwnd, EQ_FILE );
                  eqstate = EQ_modified;
                  WinSetWindowBits( ctrl, QWL_STYLE, DT_HALFTONE, DT_HALFTONE );
                  WinInvalidateRect( ctrl, NULL, FALSE );
                }

                index -= ID_MASTERL;
                if ( index >= ID_MASTERR-ID_MASTERL ) {
                  index -= ID_MASTERR-ID_MASTERL;
                  channel = 1;
                }

                rangevalue = WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SLM_QUERYSLIDERINFO,
                                                MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );
                val = (int)(24.*SHORT1FROMMR( rangevalue )/(SHORT2FROMMR( rangevalue ) - 1) +.5) - 12;
                if (val != (*dp)[channel][index])
                { (*dp)[channel][index] = val;
                  if (!mute[channel][index])
                    needeq = TRUE;
                }

                if (locklr && (*dp)[channel^1][index] != val)
                {
                  nottheuser = TRUE;
                  set_slider( hwnd, channel^1, index-1, (*dp)[channel^1][index] = val );
                  nottheuser = FALSE;
                  if (!mute[channel^1][index])
                    needeq = TRUE;
                }
                if (needeq)
                  eqneedEQ = TRUE;
              }

              case SLN_SLIDERTRACK:
                // copy the behavior of one channel slider to the other
                if( locklr )
                {
                  int channel = 0, index = SHORT1FROMMP(mp1);
                  MRESULT rangevalue;

                  index -= ID_MASTERL;
                  if ( index >= ID_MASTERR-ID_MASTERL ) {
                    index -= ID_MASTERR-ID_MASTERL;
                    channel = 1;
                  }

                  nottheuser = TRUE;
                  rangevalue = WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

                  WinSendDlgItemMsg( hwnd, ID_MASTERL + (ID_MASTERR-ID_MASTERL)*(channel^1) + index, SLM_SETSLIDERINFO,
                                     MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                                     MPFROMSHORT( SHORT1FROMMR( rangevalue )));

                  nottheuser = FALSE;
                }
              }
              break;
            }
          }
          return 0;
      }
  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

HWND DLLENTRY plugin_configure(HWND hwnd, HMODULE module)
{
  init_request();

  if (!hwnd)
  {
    if (hdialog)
    { WinDestroyWindow(hdialog);
      hdialog = NULLHANDLE;
    }
    return NULLHANDLE;
  }

  if( !hdialog ) {
    hdialog = WinLoadDlg( HWND_DESKTOP, hwnd, ConfigureDlgProc, module, ID_EQ, NULL );
    do_warpsans( hdialog );
  }

  WinShowWindow( hdialog, TRUE );
  WinSetFocus  ( HWND_DESKTOP, WinWindowFromID( hdialog, EQ_ENABLED ));

  return hdialog;
}

