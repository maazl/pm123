/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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
#include <memory.h>
#include <string.h>
#include <stdio.h>

#include <utilfct.h>
#include <fftw3.h>
#include <format.h>
#include <filter_plug.h>
#include <output_plug.h>
#include <decoder_plug.h>
#include <plugin.h>
#include "realeq.h"

#define PLUGIN "Real Equalizer 1.21"
#define MAX_COEF 16384
#define MAX_FIR  12288
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

static BOOL mmx_present = FALSE;
static BOOL eqneedinit  = TRUE;
static BOOL eqneedFIR   = TRUE;
static BOOL eqneedEQ    = TRUE;
static HWND hdialog     = NULLHANDLE;

// note: I originally intended to use 8192 for the finalFIR arrays
// but Pentium CPUs have a too small cache (16KB) and it totally shits
// putting anything bigger.
// Update 2006-07-10 MM: Solved. Now FIR order 12288 works.
// Higher values are possible too, but will require compensation for the latency.

static struct
{
  float patch[4][2];                  // keeps zeros around as fillers
  float finalFIR[MAX_FIR+4][2];

} patched;

short  finalFIRmmx[4][MAX_FIR+4][2];  // four more just for zero padding
short  finalAMPi[4];                  // four copies for MMX

static float bandgain[2][NUM_BANDS];
static BOOL  mute[2][NUM_BANDS];
static float preamp = 1.0;

// for FFT convolution...
static struct
{ float* time_domain;         // buffer in the time domain
  fftwf_complex* freq_domain; // buffer in the frequency domain (shared memory with design)
  float* design;              // buffer to design filter kernel (shared memory with freq_domain)
  float* kernel[2];           // since the kernel is real even, it's even real in the time domain
  float* overlap[2];          // keep old samples for convolution
  fftwf_plan forward_plan;    // fftw plan for time_domain -> freq_domain
  fftwf_plan backward_plan;   // fftw plan for freq_domain -> time_domain
  fftwf_plan DCT_plan;        // fftw plan for design -> time domain
  fftwf_plan RDCT_plan;       // fftw plan for time_domain -> kernel
  int    plansize;            // plansize for the FFT convolution
  int    DCTplansize;         // plansize for the filter design
} FFT;

// settings
int  newFIRorder; // this is for the GUI only
int  newPlansize; // this is for the GUI only
int  FIRorder;    // this is for the decoder thread
BOOL use_mmx   = FALSE;
BOOL use_fft   = FALSE;
BOOL eqenabled = FALSE;
BOOL locklr    = FALSE;
char lasteq[CCHMAXPATH];

#define M_PI 3.14159265358979323846

/* Hamming window
#define WINDOW_FUNCTION( n, N ) (0.54 - 0.46 * cos( 2 * M_PI * n / N ))
 * Well, since the EQ does not provide more tham 24dB dynamics
 * we should use a much more agressive window function.
 * This is verified by calculations.
 */
#define WINDOW_FUNCTION( n, N ) (0.8 - 0.2 * cos( 2 * M_PI * n / N ))

#define round(n) ((n) > 0 ? (n) + 0.5 : (n) - 0.5)

#define DO_8(p,x) \
{  { const int p = 0; x; } \
   { const int p = 1; x; } \
   { const int p = 2; x; } \
   { const int p = 3; x; } \
   { const int p = 4; x; } \
   { const int p = 5; x; } \
   { const int p = 6; x; } \
   { const int p = 7; x; } \
}

typedef struct {

   int  (DLLENTRYP output_play_samples)( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
   void* a;
   void (DLLENTRYP error_display)( char* );

   FORMAT_INFO last_format;
   int   prevlen;

   char* temp;
   char* newsamples;

} REALEQ_STRUCT;

/********** Entry point: Initialize
*/
ULONG DLLENTRY
filter_init( void** F, FILTER_PARAMS* params )
{
  REALEQ_STRUCT* f = (REALEQ_STRUCT*)malloc( sizeof( REALEQ_STRUCT ));
  #ifdef DEBUG
  fprintf(stderr, "filter_init(%p->%p, {%u, %p, %p, %i, %p, %p})\n",
   F, f, params->size, params->output_play_samples, params->a, params->audio_buffersize, params->error_display, params->info_display);
  #endif
  *F = f;

  f->output_play_samples = params->output_play_samples;
  f->a = params->a;
  f->error_display = params->error_display;
  f->prevlen = 0;

  // multiplied by 2 to make some space for past samples, should be enough (wasn't)
  f->temp = (char*)malloc( params->audio_buffersize * 4 );
  memset( f->temp, 0, params->audio_buffersize * 4 );
  f->newsamples = (char*)malloc( params->audio_buffersize );

  memset( &f->last_format, 0, sizeof( FORMAT_INFO ));
  return 0;
}

BOOL DLLENTRY
filter_uninit( void* F )
{
  REALEQ_STRUCT* f = (REALEQ_STRUCT*)F;
  #ifdef DEBUG
  fprintf(stderr, "filter_uninit(%p)\n", F);
  #endif

  if( f != NULL )
  {
    free( f->temp );
    free( f->newsamples );
    free( f );
  }
  return TRUE;
}

typedef struct
{ float lf; // log frequency
  float lv; // log value
} EQcoef;

/* setup FIR kernel */
static BOOL fil_setup( REALEQ_STRUCT* f, int samplerate, int channels )
{
  static BOOL FFTinit = FALSE;
  int   i, channel;
  float largest;
  float fftspecres;
  // equalizer design data
  EQcoef coef[NUM_BANDS+4]; // including surounding points

  static const float pow_2_20 = 1L << 20;
  static const float pow_2_15 = 1L << 15;
  // mute = -36dB. More makes no sense since the stopband attenuation
  // of the window function does not allow significantly better results.
  const float log_mute_level = -4.144653167; // -36dB

  // dispatcher to skip code fragments
  if (!eqneedinit)
  { if (eqneedFIR)
      goto doFIR;
    if (eqneedEQ)
      goto doEQ;
    return TRUE;
  }

  // free old resources
  if (FFTinit)
  { fftwf_destroy_plan(FFT.forward_plan);
    fftwf_destroy_plan(FFT.backward_plan);
    fftwf_destroy_plan(FFT.RDCT_plan);
    fftwf_free(FFT.freq_domain);
    fftwf_free(FFT.time_domain);
    // do not free aliased design array
    free(FFT.overlap[0]);
    free(FFT.overlap[1]);
    fftwf_free(FFT.kernel[0]);
    fftwf_free(FFT.kernel[1]);
  }

  // copy global parameters for thread safety
  FFT.plansize = newPlansize;
  // round up to next power of 2
  frexp(FFT.plansize-1, &i); // floor(log2(FFT.plansize-1))+1
  i = (1<<i)+1; // 2**x
  #ifdef DEBUG
  fprintf(stderr, "I: %d - %p\n", i, FFT.DCT_plan);
  #endif

  // allocate buffers
  FFT.freq_domain = fftwf_malloc((i/2+1) * sizeof *FFT.freq_domain);
  FFT.time_domain = fftwf_malloc((i+1) * sizeof *FFT.time_domain);
  FFT.design = FFT.freq_domain[0]; // Aliasing!
  FFT.overlap[0] = malloc(FFT.plansize * sizeof(float));
  memset(FFT.overlap[0], 0, FFT.plansize * sizeof(float)); // initially zero
  FFT.overlap[1] = malloc(FFT.plansize * sizeof(float));
  memset(FFT.overlap[1], 0, FFT.plansize * sizeof(float)); // initially zero
  FFT.kernel[0] = fftwf_malloc((FFT.plansize/2+1) * sizeof(float));
  FFT.kernel[1] = fftwf_malloc((FFT.plansize/2+1) * sizeof(float));

  // prepare real 2 complex transformations
  FFT.forward_plan = fftwf_plan_dft_r2c_1d( FFT.plansize, FFT.time_domain, FFT.freq_domain, FFTW_ESTIMATE);
  FFT.backward_plan = fftwf_plan_dft_c2r_1d( FFT.plansize, FFT.freq_domain, FFT.time_domain, FFTW_ESTIMATE);
  // prepare real 2 real transformations
  FFT.RDCT_plan = fftwf_plan_r2r_1d(FFT.plansize/2+1, FFT.time_domain, FFT.kernel[0], FFTW_REDFT00, FFTW_ESTIMATE);

  eqneedinit = FALSE;

 doFIR:
  if( newFIRorder < 2 || newFIRorder >= FFT.plansize)
  { (*f->error_display)("very bad! The FIR order and/or the FFT plansize is invalid or the FIRorder is higer or equal to the plansize.");
    eqenabled = FALSE; // avoid crash
    return FALSE;
  }

  // copy global parameters for thread safety
  FIRorder = (newFIRorder+15) & -16; /* multiple of 16 */

  // calculate optimum plansize for kernel generation
  frexp((FIRorder<<1) -1, &FFT.DCTplansize); // floor(log2(2*FIRorder-1))+1
  FFT.DCTplansize = 1 << FFT.DCTplansize; // 2**x
  // free old resources
  if (FFTinit)
    fftwf_destroy_plan(FFT.DCT_plan);
  // prepare real 2 real transformations
  FFT.DCT_plan = fftwf_plan_r2r_1d(FFT.DCTplansize/2+1, FFT.design, FFT.time_domain, FFTW_REDFT00, FFTW_ESTIMATE);

  #ifdef DEBUG
  fprintf(stderr, "P: FIRorder: %d, Plansize: %d, DCT plansize: %d\n", FIRorder, FFT.plansize, FFT.DCTplansize);
  #endif
  eqneedFIR = FALSE;

 doEQ:
  memset(&patched, 0, sizeof(patched));
  fftspecres = (float)samplerate / FFT.DCTplansize;

  // Prepare design coefficients frame
  coef[0].lf = -14; // very low frequency
  coef[0].lv = log_mute_level;
  coef[1].lf = log(10); // subsonic point
  coef[1].lv = log_mute_level;
  for (i = 0; i < NUM_BANDS; ++i)
    coef[i+2].lf = log(Frequencies[i]);
  coef[NUM_BANDS+2].lf = log(32000); // keep higher frequencies at 0 dB
  coef[NUM_BANDS+2].lv = 0;
  coef[NUM_BANDS+3].lf = 14; // very high frequency
  coef[NUM_BANDS+3].lv = 0;

  /* for left, right */
  largest = 0;
  for( channel = 0; channel < channels; channel++ )
  {
    // fill band data
    for (i = 0; i < NUM_BANDS; ++i)
      coef[i+2].lv = mute[channel][i] ? log_mute_level : log(bandgain[channel][i]);

    // compose frequency spectrum
    { EQcoef* cop = coef;
      FFT.design[0] = 0; // no DC
      for (i = 1; i <= FFT.DCTplansize/2; ++i) // do not start at f=0 to avoid log(0)
      { const float f = i * fftspecres; // current frequency
        double pos;
        double val = log(f);
        while (val > cop[1].lf)
          ++cop;
        // do double logarithmic sine^2 interpolation
        pos = .5 - .5*cos(M_PI * (log(f)-cop[0].lf) / (cop[1].lf-cop[0].lf));
        val = exp(cop[0].lv + pos * (cop[1].lv - cop[0].lv));
        FFT.design[i] = val;
        #ifdef DEBUG
        fprintf(stderr, "F: %i, %g, %g -> %g = %g dB @ %g\n", i, f, pos, FFT.design[i], 20*log(val)/log(10), exp(cop[0].lf));
        #endif
    } }

    // transform into the time domain
    fftwf_execute(FFT.DCT_plan);
    #ifdef DEBUG
    for (i = 0; i <= FFT.DCTplansize/2; ++i)
      fprintf(stderr, "TK: %i, %g\n", i, FFT.time_domain[i]);
    #endif

    // normalize, apply window function and store results symmetrically
    { float* sp = FFT.time_domain;
      float* dp1 = &patched.finalFIR[FIRorder/2][channel];
      float* dp2 = dp1;
      for (i = FIRorder/2; i >= 0; --i)
      { register double f = fabs(*dp1 = *dp2 = *sp *= WINDOW_FUNCTION(i, FIRorder) / FFT.DCTplansize);
        *sp++ /= FFT.plansize; // normalize for next FFT
        #ifdef DEBUG
        fprintf(stderr, "K: %i, %g\n", i, *dp1);
        #endif
        dp1 += 2;
        dp2 -= 2;
        if (f > largest)
          largest = f;
      }
      // padding
      memset(sp, 0, (FFT.plansize-FIRorder)/2 * sizeof *FFT.time_domain);
    }

    // prepare for FFT convolution
    // transform back into the frequency domain, now with window function
    fftwf_execute_r2r(FFT.RDCT_plan, FFT.time_domain, FFT.kernel[channel]);
    #ifdef DEBUG
    for (i = 0; i <= FFT.plansize/2; ++i)
      fprintf(stderr, "FK: %i, %g\n", i, FFT.kernel[channel][i]);
    fprintf(stderr, "E: kernel completed.\n");
    #endif
  }

  /* prepare data for MMX convolution */

  // so the largest value don't overflow squeezed in a short
  largest += largest / pow_2_15;
  // shouldn't be bigger than 4 so shifting 12 instead of 15 should do it
  finalAMPi[0] = largest * (1<<12) + 1;
  // only divide the coeficients by what is in finalAMPi
  largest = (float)finalAMPi[0] / (1<<12);

  // making three other copies for MMX
  finalAMPi[1] = finalAMPi[0];
  finalAMPi[2] = finalAMPi[0];
  finalAMPi[3] = finalAMPi[0];

  if( channels == 2 )
  {
    // zero padding is done with the memset() above and with patched.patch
    // check filter_samples_mmx_stereo in mmxfir.asm for an explanation
    for( i = 0; i < 4; i++ )
    { int e;
      for( e = 0; e < FIRorder/4; e += 4 )
      {
        finalFIRmmx[i][e+0][0] = round( patched.finalFIR[e+0-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e+0][1] = round( patched.finalFIR[e+2-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e+2][0] = round( patched.finalFIR[e+1-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e+2][1] = round( patched.finalFIR[e+3-i][0]*pow_2_20/largest );

        finalFIRmmx[i][e+1][0] = round( patched.finalFIR[e+0-i][1]*pow_2_20/largest );
        finalFIRmmx[i][e+1][1] = round( patched.finalFIR[e+2-i][1]*pow_2_20/largest );
        finalFIRmmx[i][e+3][0] = round( patched.finalFIR[e+1-i][1]*pow_2_20/largest );
        finalFIRmmx[i][e+3][1] = round( patched.finalFIR[e+3-i][1]*pow_2_20/largest );
      }
      for( e = FIRorder/4; e < 3*FIRorder/4; e += 4 )
      {
        finalFIRmmx[i][e+0][0] = round( patched.finalFIR[e+0-i][0]*pow_2_15/largest );
        finalFIRmmx[i][e+0][1] = round( patched.finalFIR[e+2-i][0]*pow_2_15/largest );
        finalFIRmmx[i][e+2][0] = round( patched.finalFIR[e+1-i][0]*pow_2_15/largest );
        finalFIRmmx[i][e+2][1] = round( patched.finalFIR[e+3-i][0]*pow_2_15/largest );

        finalFIRmmx[i][e+1][0] = round( patched.finalFIR[e+0-i][1]*pow_2_15/largest );
        finalFIRmmx[i][e+1][1] = round( patched.finalFIR[e+2-i][1]*pow_2_15/largest );
        finalFIRmmx[i][e+3][0] = round( patched.finalFIR[e+1-i][1]*pow_2_15/largest );
        finalFIRmmx[i][e+3][1] = round( patched.finalFIR[e+3-i][1]*pow_2_15/largest );
      }
      for( e = 3*FIRorder/4; e <= FIRorder; e += 4 )
      {
        finalFIRmmx[i][e+0][0] = round( patched.finalFIR[e+0-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e+0][1] = round( patched.finalFIR[e+2-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e+2][0] = round( patched.finalFIR[e+1-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e+2][1] = round( patched.finalFIR[e+3-i][0]*pow_2_20/largest );

        finalFIRmmx[i][e+1][0] = round( patched.finalFIR[e+0-i][1]*pow_2_20/largest );
        finalFIRmmx[i][e+1][1] = round( patched.finalFIR[e+2-i][1]*pow_2_20/largest );
        finalFIRmmx[i][e+3][0] = round( patched.finalFIR[e+1-i][1]*pow_2_20/largest );
        finalFIRmmx[i][e+3][1] = round( patched.finalFIR[e+3-i][1]*pow_2_20/largest );
      }
    }
  }
  else // channel == 1 I hope
  {
    // zero padding is done with the memset() above and with patched.patch
    for( i = 0; i < 4; i++ )
    { int e;
      for( e = 0; e < FIRorder/4; e += 4 )
      {
        finalFIRmmx[i][e/2+0][0] = round( patched.finalFIR[e+0-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e/2+0][1] = round( patched.finalFIR[e+1-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e/2+1][0] = round( patched.finalFIR[e+2-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e/2+1][1] = round( patched.finalFIR[e+3-i][0]*pow_2_20/largest );
      }
      for( e = FIRorder/4; e < 3*FIRorder/4; e += 4 )
      {
        finalFIRmmx[i][e/2+0][0] = round( patched.finalFIR[e+0-i][0]*pow_2_15/largest );
        finalFIRmmx[i][e/2+0][1] = round( patched.finalFIR[e+1-i][0]*pow_2_15/largest );
        finalFIRmmx[i][e/2+1][0] = round( patched.finalFIR[e+2-i][0]*pow_2_15/largest );
        finalFIRmmx[i][e/2+1][1] = round( patched.finalFIR[e+3-i][0]*pow_2_15/largest );
      }
      for( e = 3*FIRorder/4; e <= FIRorder; e += 4 )
      {
        finalFIRmmx[i][e/2+0][0] = round( patched.finalFIR[e+0-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e/2+0][1] = round( patched.finalFIR[e+1-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e/2+1][0] = round( patched.finalFIR[e+2-i][0]*pow_2_20/largest );
        finalFIRmmx[i][e/2+1][1] = round( patched.finalFIR[e+3-i][0]*pow_2_20/largest );
      }
    }
  }

  eqneedEQ = FALSE;
  FFTinit = TRUE;
  return TRUE;
}

static void
filter_samples_fpu_mono( short* newsamples, short* temp, char* buf, int len )
{
  int i = 0;
  memcpy((char*)temp + FIRorder * 2, buf, len );

  while( i < len / 2 )
  {
    int   j     = 0;
    float total = 0;

    for( j = 0; j <= FIRorder; j += 4 )
    {
      total += patched.finalFIR[j  ][0] * temp[i+j  ];
      total += patched.finalFIR[j+1][0] * temp[i+j+1];
      total += patched.finalFIR[j+2][0] * temp[i+j+2];
      total += patched.finalFIR[j+3][0] * temp[i+j+3];
    }

    if( total > SHRT_MAX ) {
      newsamples[i] = SHRT_MAX;
    } else if( total < SHRT_MIN ) {
      newsamples[i] = SHRT_MIN;
    } else {
      newsamples[i] = total;
    }

    i++;
  }
}

static void
filter_samples_fpu_stereo( short* newsamples, short* temp, char* buf, int len )
{
  int i = 0;
  memcpy((char*)temp + FIRorder * 4, buf, len );

  while( i < len / 4 )
  {
    int   j        = 0;
    float total[2] = {0};

    for( j = 0; j <= FIRorder; j += 4 )
    {
      total[0] += patched.finalFIR[j  ][0] * temp[(i+j  )*2 + 0];
      total[1] += patched.finalFIR[j  ][1] * temp[(i+j  )*2 + 1];

      total[0] += patched.finalFIR[j+1][0] * temp[(i+j+1)*2 + 0];
      total[1] += patched.finalFIR[j+1][1] * temp[(i+j+1)*2 + 1];

      total[0] += patched.finalFIR[j+2][0] * temp[(i+j+2)*2 + 0];
      total[1] += patched.finalFIR[j+2][1] * temp[(i+j+2)*2 + 1];

      total[0] += patched.finalFIR[j+3][0] * temp[(i+j+3)*2 + 0];
      total[1] += patched.finalFIR[j+3][1] * temp[(i+j+3)*2 + 1];
    }

    if( total[0] > SHRT_MAX ) {
      newsamples[i*2 + 0] = SHRT_MAX;
    } else if( total[0] < SHRT_MIN ) {
      newsamples[i*2 + 0] = SHRT_MIN;
    } else {
      newsamples[i*2 + 0] = total[0];
    }

    if( total[1] > SHRT_MAX ) {
      newsamples[i*2 + 1] = SHRT_MAX;
    } else if( total[1] < SHRT_MIN ) {
      newsamples[i*2 + 1] = SHRT_MIN;
    } else {
      newsamples[i*2 + 1] = total[1];
    }

    i++;
  }
}

/* apply one FFT cycle and process at most plansize-FIRorder samples
 */
static void do_fft_filter(float* kp)
{ int l;
  fftwf_complex* dp;
  // FFT
  fftwf_execute(FFT.forward_plan);
  // Convolution in the frequency domain is simply a series of complex products.
  // But because of the symmetry of the filter kernel it is just comlpex * real.
  dp = FFT.freq_domain;
  for (l = (FFT.plansize/2+1) >> 3; l; --l)
  {
    DO_8(p,
      dp[p][0] *= kp[p];
      dp[p][1] *= kp[p];
    );
    kp += 8;
    dp += 8;
  }
  for (l = (FFT.plansize/2+1) & 7; l; --l)
  { dp[0][0] *= kp[0];
    dp[0][1] *= kp[0];
    ++kp;
    ++dp;
  }
  // IFFT
  fftwf_execute(FFT.backward_plan);
}

/* fetch saved samples */
INLINE void do_fft_load_overlap(float* overlap_buffer)
{ memcpy(FFT.time_domain + FFT.plansize - FIRorder/2, overlap_buffer, FIRorder/2 * sizeof *FFT.time_domain);
  memcpy(FFT.time_domain, overlap_buffer + FIRorder/2, FIRorder/2 * sizeof *FFT.time_domain);
}

/* store last FIRorder samples */
static void do_fft_save_overlap(float* overlap_buffer, int len)
{
  #ifdef DEBUG
  fprintf(stderr, "SO: %p, %i, %i\n", overlap_buffer, len, FIRorder);
  #endif
  if (len < FIRorder/2)
  { memmove(overlap_buffer, overlap_buffer + len, (FIRorder - len) * sizeof *overlap_buffer);
    memcpy(overlap_buffer + FIRorder - len, FFT.time_domain + FIRorder/2, len * sizeof *overlap_buffer);
  } else
    memcpy(overlap_buffer, FFT.time_domain + len - FIRorder/2, FIRorder * sizeof *overlap_buffer);
}

/* convert samples from short to float and store it in the FFT.time_domain buffer
   The samples are shifted by FIRorder/2 to the right.
   FIRorder samples before the starting position are taken from overlap_buffer.
   The overlap_buffer is updated with the last FIRorder samples before return.
 */
static void do_fft_load_samples_mono(const short* sp, const int len, float* overlap_buffer)
{ int l;
  float* dp = FFT.time_domain + FIRorder/2;
  do_fft_load_overlap(overlap_buffer);
  // fetch new samples
  for (l = len >> 3; l; --l) // coarse
  { DO_8(p, dp[p] = sp[p]);
    sp += 8;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
    *dp++ = *sp++;
  memset(dp, 0, (FFT.plansize - FIRorder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  do_fft_save_overlap(overlap_buffer, len);
}
/* convert samples from short to float and store it in the FFT.time_domain buffer
 * This will cover only one channel. The only difference to to mono-version is that
 * the source pointer is incremented by 2 per sample.
 */
static void do_fft_load_samples_stereo(const short* sp, const int len, float* overlap_buffer)
{ int l;
  float* dp = FFT.time_domain + FIRorder/2;
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
  memset(dp, 0, (FFT.plansize - FIRorder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  do_fft_save_overlap(overlap_buffer, len);
}

INLINE short quantize(float f)
{ if (f < -32768)
    return -32768;
  if (f > 32767)
    return 32767;
  return (short)f; // Well, dithering might be nice, but slow either.
}

/* store samples */
static void do_fft_store_samples_mono(short* sp, const int len)
{ float* dp = FFT.time_domain;
  int l;
  // transfer overlapping samples
  for (l = len >> 3; l; --l) // coarse
  { DO_8(p, sp[p] = quantize(dp[p]));
    sp += 8;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
    *sp++ = quantize(*dp++);
}

/* store stereo samples
 * This will cover only one channel. The only difference to to mono-version is that
 * the destination pointer is incremented by 2 per sample.
 */
static void do_fft_store_samples_stereo(short* sp, const int len)
{ float* dp = FFT.time_domain;
  int l;
  // transfer overlapping samples
  for (l = len >> 3; l; --l) // coarse
  { DO_8(p, sp[p<<1] = quantize(dp[p]));
    sp += 16;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
  { *sp = quantize(*dp++);
    sp += 2;
  }
}

static void filter_samples_fft_mono(short* newsamples, const short* buf, int len)
{
  while (len) // we might need more than one FFT cycle
  { int l2 = FFT.plansize - FIRorder;
    if (l2 > len)
      l2 = len;

    // convert to float (well, that bill we have to pay)
    do_fft_load_samples_mono(buf, l2, FFT.overlap[0]);
    // do FFT
    do_fft_filter(FFT.kernel[0]);
    // convert back to short and use overlap/add
    do_fft_store_samples_mono(newsamples, l2);

    // next block
    len -= l2;
    newsamples += l2;
    buf += l2;
  }
}

static void filter_samples_fft_stereo( short* newsamples, const short* buf, int len )
{
  while (len) // we might need more than one FFT cycle
  { int l2 = FFT.plansize - FIRorder;
    if (l2 > len)
      l2 = len;

    // left channel
    // convert to float (well, that bill we have to pay)
    do_fft_load_samples_stereo(buf, l2, FFT.overlap[0]);
    // do FFT
    do_fft_filter(FFT.kernel[0]);
    // convert back to short and use overlap/add
    do_fft_store_samples_stereo(newsamples, l2);

    // right channel
    // convert to float (well, that bill we have to pay)
    do_fft_load_samples_stereo(buf+1, l2, FFT.overlap[1]);
    // do FFT
    do_fft_filter(FFT.kernel[1]);
    // convert back to short and use overlap/add
    do_fft_store_samples_stereo(newsamples+1, l2);

    // next block
    len -= l2;
    l2 <<= 1; // stereo
    newsamples += l2;
    buf += l2;
  }
}

/* Entry point: do filtering */
int DLLENTRY
filter_play_samples( void* F, FORMAT_INFO* format, char* buf, int len, int posmarker )
{
  REALEQ_STRUCT* f = (REALEQ_STRUCT*)F;
  #ifdef DEBUG
  fprintf(stderr, "filter_play_samples(%p, {%u, %u, %u, %u, %u}, %p, %u, %u)\n",
   F, format->size, format->samplerate, format->channels, format->bits, format->format, buf, len, posmarker);
  #endif

  if( eqenabled && format->bits == 16 && ( format->channels == 1 || format->channels == 2 ))
  {
    short *temp = (short*)f->temp;
    short *newsamples = (short*)f->newsamples;


    if( memcmp( &f->last_format, format, sizeof( FORMAT_INFO )) != 0 )
    { f->last_format = *format;
      eqneedinit = TRUE;
    }
    fil_setup( f, format->samplerate, format->channels );

    if (use_fft)
    {
      if( format->channels == 2 ) {
        filter_samples_fft_stereo( newsamples, (short*)buf, len>>2 );
      } else if( format->channels == 1 ) {
        filter_samples_fft_mono( newsamples, (short*)buf, len>>1 );
      }
    } else if( use_mmx )
    {
      memmove((char*)temp, (char*)temp+f->prevlen, FIRorder*format->channels*format->bits/8 );
      f->prevlen = len;

      if( format->channels == 2 ) {
        filter_samples_mmx_stereo( newsamples, temp, buf, len );
      } else if( format->channels == 1 ) {
        filter_samples_mmx_mono( newsamples, temp, buf, len );
      }
    }
    else
    {
      memmove((char*)temp, (char*)temp+f->prevlen, FIRorder*format->channels*format->bits/8 );
      f->prevlen = len;

      if( format->channels == 2 ) {
        filter_samples_fpu_stereo( newsamples, temp, buf, len );
      } else if( format->channels == 1) {
        filter_samples_fpu_mono( newsamples, temp, buf, len );
      }
    }

    return f->output_play_samples( f->a, format, (char*)f->newsamples, len, posmarker );
  }
  else
  {
    return f->output_play_samples( f->a, format, buf, len, posmarker );
  }
}


/********** GUI stuff *******************************************************/


static void
save_ini( void )
{
  HINI INIhandle;

  if(( INIhandle = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value ( INIhandle, newFIRorder );
    save_ini_value ( INIhandle, newPlansize );
    save_ini_value ( INIhandle, eqenabled );
    save_ini_value ( INIhandle, locklr );
    save_ini_value ( INIhandle, use_mmx );
    save_ini_value ( INIhandle, use_fft );
    save_ini_string( INIhandle, lasteq );

    close_ini( INIhandle );
  }
}

static void
load_ini( void )
{
  HINI INIhandle;
  int e,i;

  for( e = 0; e < 2; e++ ) {
    for( i = 0; i < NUM_BANDS; i++ )
    {
      bandgain[e][i] = 1.0;
      mute[e][i] = FALSE;
    }
  }

  eqenabled   = FALSE;
  eqneedinit  = TRUE;
  use_mmx     = FALSE; // prefer FFT
  use_fft     = TRUE;
  lasteq[0]   = 0;
  newPlansize    = 8192;
  newFIRorder    = 4096;
  FIRorder    = 4096;
  locklr      = FALSE;

  if(( INIhandle = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value ( INIhandle, newFIRorder );
    load_ini_value ( INIhandle, newPlansize );
    load_ini_value ( INIhandle, eqenabled );
    load_ini_value ( INIhandle, locklr );
    load_ini_value ( INIhandle, use_mmx );
    load_ini_value ( INIhandle, use_fft );
    load_ini_string( INIhandle, lasteq, sizeof( lasteq ));

    close_ini( INIhandle );

    if (newPlansize <= newFIRorder)
      newFIRorder = newPlansize >> 1; // avoid crash when INI-Content is bad

    if (use_fft)
      use_mmx = FALSE; // migration from older profiles
  }
}

static BOOL
save_eq( HWND hwnd, float* gains, BOOL* mutes, float preamp )
{
  FILEDLG filedialog;
  FILE*   file;
  int     i = 0;

  memset( &filedialog, 0, sizeof(FILEDLG));
  filedialog.cbSize   = sizeof(FILEDLG);
  filedialog.fl       = FDS_CENTER | FDS_SAVEAS_DIALOG;
  filedialog.pszTitle = "Save Equalizer as...";

  if( lasteq[0] == 0 ) {
    strcpy( filedialog.szFullFile, "*.REQ" );
  } else {
    strcpy( filedialog.szFullFile, lasteq );
  }

  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    strcpy( lasteq, filedialog.szFullFile );
    file = fopen( filedialog.szFullFile, "w" );
    if( file == NULL ) {
      return FALSE;
    }

    fprintf( file, "#\n# Equalizer created with %s\n# Do not modify!\n#\n", VERSION );
    fprintf( file, "# Band gains\n" );
    for( i = 0; i < NUM_BANDS*2; i++ ) {
      fprintf( file, "%g\n", gains[i] );
    }
    fprintf(file, "# Mutes\n" );
    for( i = 0; i < NUM_BANDS*2; i++ ) {
      fprintf(file, "%lu\n", mutes[i]);
    }
    fprintf( file, "# Preamplifier\n" );
    fprintf( file, "%g\n", preamp );

    fprintf( file, "# End of equalizer\n" );
    fclose ( file );
    return TRUE;
  }

  return FALSE;
}

static
void drivedir( char* buf, char* fullpath )
{
  char drive[_MAX_DRIVE],
       path [_MAX_PATH ];

 _splitpath( fullpath, drive, path, NULL, NULL );
  strcpy( buf, drive );
  strcat( buf, path  );
}

static BOOL
load_eq_file( char* filename, float* gains, BOOL* mutes, float* preamp )
{
  FILE* file;
  int   i = 0;
  char  line[256];

  file = fopen( filename, "r" );
  if( file == NULL ) {
    return FALSE;
  }

  while( !feof( file ))
  {
    fgets( line, sizeof(line), file );
    blank_strip( line );
    if( *line && line[0] != '#' && line[0] != ';' && i < 129 )
    {
      if( i < NUM_BANDS*2 ) {
        gains[i] = atof(line);
      } else if( i > NUM_BANDS*2-1 && i < NUM_BANDS*4 ) {
        mutes[i-NUM_BANDS*2] = atoi(line);
      } else if( i == NUM_BANDS*4 ) {
        *preamp = atof(line);
      }
      i++;
    }
  }
  fclose( file );
  return TRUE;
}

static BOOL
load_eq( HWND hwnd, float* gains, BOOL* mutes, float* preamp )
{
  FILEDLG filedialog;

  memset( &filedialog, 0, sizeof( FILEDLG ));
  filedialog.cbSize = sizeof(FILEDLG);
  filedialog.fl = FDS_CENTER | FDS_OPEN_DIALOG;
  filedialog.pszTitle = "Load Equalizer";
  drivedir( filedialog.szFullFile, lasteq );
  strcat( filedialog.szFullFile, "*.REQ" );

  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    strcpy( lasteq, filedialog.szFullFile );
    return load_eq_file( filedialog.szFullFile, gains, mutes, preamp );
  }
  return FALSE;
}

void DLLENTRY
plugin_query( PLUGIN_QUERYPARAM *param )
{
  param->type = PLUGIN_FILTER;
  param->author = "Samuel Audet";
  param->desc = PLUGIN;
  param->configurable = TRUE;
}

static void
set_band( HWND hwnd, int channel, int band )
{
  MRESULT rangevalue;
  float   range,value;

  rangevalue = WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*channel+band, SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );
  value = (float)SHORT1FROMMR( rangevalue );
  range = (float)SHORT2FROMMR( rangevalue ) - 1; // minus 1 here!!!

  bandgain[channel][band] = pow(10.0,(value - range/2)/(range/2)*12/20);
  mute[channel][band] = SHORT1FROMMR( WinSendDlgItemMsg( hwnd, 100+NUM_BANDS*channel+band, BM_QUERYCHECK, 0, 0 ));
}

static void
load_dialog( HWND hwnd )
{
  int i, e;

  for( e = 0; e < 2; e++ ) {
    for( i = 0; i < NUM_BANDS; i++ ) {

      // sliders
      MRESULT rangevalue;
      float   range;

      rangevalue = WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_QUERYSLIDERINFO,
                                      MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

      WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_ADDDETENT,
                         MPFROMSHORT( SHORT2FROMMR( rangevalue )  - 1 ), 0 );
      WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_ADDDETENT,
                         MPFROMSHORT( SHORT2FROMMR( rangevalue ) >> 1 ), 0 );
      WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_ADDDETENT,
                         MPFROMSHORT( 0 ), 0 );

      range = (float)( SHORT2FROMMR( rangevalue ) - 1 );

      WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_SETSLIDERINFO,
                         MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                         MPFROMSHORT((SHORT)( 20*log10( bandgain[e][i])/12*range/2+range/2 )));
      // mute check boxes
      WinSendDlgItemMsg( hwnd, 100+NUM_BANDS*e+i, BM_SETCHECK, MPFROMCHAR( mute[e][i] ), 0 );
    }
  }

  // eq enabled check box
  WinSendDlgItemMsg( hwnd, EQ_ENABLED, BM_SETCHECK, MPFROMSHORT( eqenabled ), 0 );
  WinSendDlgItemMsg( hwnd, ID_LOCKLR,  BM_SETCHECK, MPFROMSHORT( locklr ), 0 );
  WinSendDlgItemMsg( hwnd, ID_USEMMX,  BM_SETCHECK, MPFROMSHORT( use_mmx ), 0 );
  if (!mmx_present)
    WinEnableControl(hwnd, ID_USEMMX, FALSE);
  WinSendDlgItemMsg( hwnd, ID_USEFFT,  BM_SETCHECK, MPFROMSHORT( use_fft ), 0 );

  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETMASTER, MPFROMLONG( NULLHANDLE ), 0 );
  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETLIMITS, MPFROMLONG( MAX_FIR ), MPFROMLONG( 16 ));
  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETCURRENTVALUE, MPFROMLONG( newFIRorder ), 0 );
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETMASTER, MPFROMLONG( NULLHANDLE ), 0 );
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETLIMITS, MPFROMLONG( MAX_COEF ), MPFROMLONG( 32 ));
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETCURRENTVALUE, MPFROMLONG( newPlansize ), 0 );
}

MRESULT EXPENTRY
ConfigureDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  static char nottheuser = FALSE;

  switch( msg )
  {
    case WM_INITDLG:
      nottheuser = TRUE;
      load_dialog( hwnd );
      nottheuser = FALSE;
      break;

    case WM_CLOSE:
      save_ini();
      WinDestroyWindow( hwnd );
      hdialog = NULLHANDLE;
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ))
      {
        case EQ_SAVE:
          save_eq( hwnd, (float*)bandgain, (BOOL*)mute, preamp );
          break;

        case EQ_LOAD:
          load_eq( hwnd, (float*)bandgain, (BOOL*)mute, &preamp );
          nottheuser  = TRUE;
          load_dialog( hwnd );
          nottheuser  = FALSE;
          eqneedEQ = TRUE;
          break;

        case EQ_ZERO:
        {
          int e,i;
          MRESULT rangevalue;

          for( e = 0; e < 2; e++ ) {
            for( i = 0; i < NUM_BANDS; i++ ) {

              rangevalue = WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_QUERYSLIDERINFO,
                                              MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

              WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_SETSLIDERINFO,
                                 MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                                 MPFROMSHORT( SHORT2FROMMR( rangevalue ) >> 1 ));

              WinSendDlgItemMsg( hwnd, 100+NUM_BANDS*e+i, BM_SETCHECK, 0, 0 );
            }
          }
          eqneedEQ = TRUE;
          *lasteq = 0;
          break;
        }
      }

    case WM_CONTROL:
    {
      int id = SHORT1FROMMP( mp1 );

      if( nottheuser ) {
        return 0;
      }

      switch( id )
      {
        ULONG temp;

        case EQ_ENABLED:
          eqenabled = WinQueryButtonCheckstate( hwnd, id );
          break;

        case ID_LOCKLR:
          locklr = WinQueryButtonCheckstate( hwnd, id );
          break;

        case ID_USEMMX:
          use_mmx = WinQueryButtonCheckstate( hwnd, id );
          if( use_mmx ) {
            use_fft = FALSE;
            WinSendDlgItemMsg( hwnd, ID_USEFFT, BM_SETCHECK, MPFROMSHORT( FALSE ), 0 );
          }
          break;

        case ID_USEFFT:
          use_fft = WinQueryButtonCheckstate( hwnd, id );
          if( use_fft ) {
            use_mmx = FALSE;
            WinSendDlgItemMsg( hwnd, ID_USEMMX, BM_SETCHECK, MPFROMSHORT( FALSE ), 0 );
          }
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
                FIRorder = temp; // TODO: dirty threading issue
              }
              break;

            case SPBN_UPARROW:
              temp = (newFIRorder & ~0xF) + 16;
              if (temp < newPlansize)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG( temp ), 0 );
              break;

            case SPBN_DOWNARROW:
              temp = (newFIRorder & ~0xF) - 16;
              if (temp >= 16)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG(temp), 0 );
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

        default:

          // mute check boxes
          if( id >= 100 && id < 200 ) {
            switch( SHORT2FROMMP( mp1 ))
            {
              case BN_CLICKED:
              case BN_DBLCLICKED:
              {
                int channel = 0, band = 0, raw = SHORT1FROMMP(mp1);

                raw -= 100;
                while( raw - NUM_BANDS >= 0 ) {
                  raw -= NUM_BANDS;
                  channel++;
                }
                band = raw;

                if( locklr )
                {
                  nottheuser = TRUE;
                  WinSendDlgItemMsg( hwnd, 100+NUM_BANDS*((channel+1)&1)+band, BM_SETCHECK, MPFROMSHORT(
                          SHORT1FROMMR( WinSendDlgItemMsg( hwnd, 100+NUM_BANDS*channel+band, BM_QUERYCHECK, 0, 0 ))
                          ), 0 );
                  nottheuser = FALSE;
                  set_band( hwnd, (channel+1)&1, band );
                }

                set_band( hwnd, channel, band );
                eqneedEQ = TRUE;
              }
            }
          // sliders
          } else if( id >= 200 && id < 300 ) {
            switch( SHORT2FROMMP( mp1 ))
            {
              case SLN_CHANGE:
              {
                int channel = 0, band = 0, raw = SHORT1FROMMP(mp1);

                raw -= 200;
                while( raw - NUM_BANDS >= 0 ) {
                  raw -= NUM_BANDS;
                  channel++;
                }
                band = raw;

                set_band( hwnd, channel, band );
                if( locklr )
                {
                  MRESULT rangevalue;

                  nottheuser = TRUE;
                  rangevalue = WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*channel+band, SLM_QUERYSLIDERINFO,
                                    MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

                  WinSendDlgItemMsg( hwnd, 200 + NUM_BANDS * (( channel + 1 ) & 1 ) + band,
                                     SLM_SETSLIDERINFO,
                                     MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                                     MPFROMSHORT( SHORT1FROMMR( rangevalue )));

                  nottheuser = FALSE;
                  set_band(hwnd, (channel+1)&1, band);
                }
                eqneedEQ = TRUE;
              }

              case SLN_SLIDERTRACK:
                // copy the behavior of one channel slider to the other
                if( locklr )
                {
                  int channel = 0, band = 0, raw = SHORT1FROMMP(mp1);
                  MRESULT rangevalue;

                  raw -= 200;
                  while( raw - NUM_BANDS >= 0 ) {
                    raw -= NUM_BANDS;
                    channel++;
                  }
                  band = raw;
                  nottheuser = TRUE;
                  rangevalue = WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*channel+band, SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

                  WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*((channel+1)&1)+band, SLM_SETSLIDERINFO,
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

void DLLENTRY
plugin_configure( HWND hwnd, HMODULE module )
{
  if( !hdialog ) {
    hdialog = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP, ConfigureDlgProc, module, ID_EQ, NULL );
    do_warpsans( hdialog );
  }

  WinShowWindow( hdialog, TRUE );
  WinSetFocus  ( HWND_DESKTOP, WinWindowFromID( hdialog, EQ_ENABLED ));
}

extern int INIT_ATTRIBUTE __dll_initialize( void )
{
  mmx_present = detect_mmx();

  load_ini();
  load_eq_file( lasteq, (float*)bandgain, (BOOL*)mute, &preamp );
  return 1;
}

extern int TERM_ATTRIBUTE __dll_terminate( void )
{
  save_ini();
  return 1;
}

#if defined(__IBMC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag       )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) return 0UL;
    __dll_initialize();
  } else {
    __dll_terminate ();
    _CRT_term();
  }

  return 1UL;
}
#endif
