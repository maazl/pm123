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

#define DEBUG

#define VERSION "Real Equalizer 1.21"
#define MAX_COEF 8192
#define NUM_BANDS  32

static BOOL  mmx_present;
static BOOL  eqneedinit  = TRUE;
static BOOL  eqneedsetup = TRUE;
static HWND  hdialog     = NULLHANDLE;

static float eqbandFIR[2][NUM_BANDS][4096+4];

// note: I originally intended to use 8192 for the finalFIR arrays
// but Pentium CPUs have a too small cache (16KB) and it totally shits
// putting anything bigger

static struct
{
  float patch[4][2];            // keeps zeros around as fillers
  float finalFIR[4100][2];

} patched;

short  finalFIRmmx[4][4100][2]; // four more just for zero padding
short  finalAMPi[4];            // four copies for MMX

static float bandgain[2][32];
static BOOL  mute[2][NUM_BANDS];
static float preamp = 1.0;

// for FFT convolution...
static struct
{ fftwf_plan forward_plan;
  fftwf_plan backward_plan;
  float* time_domain;
  fftwf_complex* freq_domain;
  fftwf_complex* kernel[2];
  float* overlap[2];
} FFT;

// settings
int  FIRorder;
int  plansize;
BOOL use_mmx   = FALSE;
BOOL use_fft   = FALSE;
BOOL eqenabled = FALSE;
BOOL locklr    = FALSE;
char lasteq[CCHMAXPATH];

#define M_PI 3.14159265358979323846

#define WINDOW_HANN( n, N )    (0.50 - 0.50 * cos( 2 * M_PI * n / N ))
#define WINDOW_HAMMING( n, N ) (0.54 - 0.46 * cos( 2 * M_PI * n / N ))

#define round(n) ((n) > 0 ? (n) + 0.5 : (n) - 0.5)

#define DO_8(x) \ 
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

   int  (*_System output_play_samples)( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
   void* a;
   void (*_System error_display)( char* );

   FORMAT_INFO last_format;
   int   prevlen;

   char* temp;
   char* newsamples;

} REALEQ_STRUCT;

int  _CRT_init( void );
void _CRT_term( void );

/********** Entry point: Initialize
*/
ULONG _System
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

  // multiplied by 2 to make some space for past samples, should be enough
  f->temp = (char*)malloc( params->audio_buffersize * 2 );
  memset( f->temp, 0, params->audio_buffersize * 2 );
  f->newsamples = (char*)malloc( params->audio_buffersize );

  memset( &f->last_format, 0, sizeof( FORMAT_INFO ));
  
  return 0;
}

/********** Entry point: Cleanup
*/ 
BOOL _System
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

/* setup FIR kernel */
static BOOL fil_setup( int channels )
{
  int   i, e, channel;
  float largest = 0;

  static const float pow_2_20 = 1L << 20;
  static const float pow_2_15 = 1L << 15;
  
  if( eqneedinit ) {
    return FALSE;
  }

  memset( patched.finalFIR, 0, sizeof( patched.finalFIR ));

  /* for left, right */
  for( channel = 0; channel < channels; channel++ )
  {
    for( i = 0; i < NUM_BANDS; i++ ) {
      if( !mute[channel][i] ) {
        for( e = 0; e <= FIRorder; e++ ) {
          patched.finalFIR[e][channel] += bandgain[channel][i] * eqbandFIR[channel][i][e];
        }
      }
    }

    for( e = 0; e <= FIRorder; e++ )
    {
      register float f = fabs(patched.finalFIR[e][channel] *= 2 * WINDOW_HAMMING( e, FIRorder ));
      if( f > largest )
        largest = f;
    }
  }

  fprintf(stderr, "Alloc-1: %p %p %p %p\n", FFT.freq_domain, FFT.time_domain, FFT.forward_plan, FFT.backward_plan);
 
  /* prepare data for MMX convolution */ 

  // so the largest value don't overflow squeezed in a short
  largest += largest / pow_2_15;
  // shouldn't be bigger than 4 so shifting 12 instead of 15 should do it
  finalAMPi[0] = largest * pow( 2, 12 ) + 1;
  // only divide the coeficients by what is in finalAMPi
  largest = (float)finalAMPi[0] / pow( 2, 12 );

  // making three other copies for MMX
  finalAMPi[1] = finalAMPi[0];
  finalAMPi[2] = finalAMPi[0];
  finalAMPi[3] = finalAMPi[0];

  if( channels == 2 )
  {
    // zero padding is done with the memset() above and with patched.patch
    // check filter_samples_mmx_stereo in mmxfir.asm for an explanation
    for( i = 0; i < 4; i++ )
    {
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
    {
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

  /* debug
  for(i = 0; i < 4; i++)
  {
    printf( "\n\nprinting out finalMMX %d\n\n", i );
    for( e = 0; e < FIRorder/4; e+=4 )
    {
      printf("%f %f %f %f ",
            (float)finalFIRmmx[i][e+0][0]*largest/pow_2_20,
            (float)finalFIRmmx[i][e+0][1]*largest/pow_2_20,
            (float)finalFIRmmx[i][e+2][0]*largest/pow_2_20,
            (float)finalFIRmmx[i][e+2][1]*largest/pow_2_20);
    }

    printf( " = CUUUUUUUUUT =" );

    for( e = FIRorder/4; e < 3*FIRorder/4; e+=4 )
    {
      printf("%f %f %f %f ",
            (float)finalFIRmmx[i][e+0][0]*largest/pow_2_15,
            (float)finalFIRmmx[i][e+0][1]*largest/pow_2_15,
            (float)finalFIRmmx[i][e+2][0]*largest/pow_2_15,
            (float)finalFIRmmx[i][e+2][1]*largest/pow_2_15);
    }

    printf( " = CUUUUUUUUUT =" );

    for( e = 3*FIRorder/4; e <= FIRorder; e+=4 )
    {
      printf("%f %f %f %f ",
            (float)finalFIRmmx[i][e+0][0]*largest/pow_2_20,
            (float)finalFIRmmx[i][e+0][1]*largest/pow_2_20,
            (float)finalFIRmmx[i][e+2][0]*largest/pow_2_20,
            (float)finalFIRmmx[i][e+2][1]*largest/pow_2_20);
    }
    printf("\n%d %d %f\n\n",e,finalAMPi,largest);
  }

  printf( "\n\nprinting out finalFIR  \n\n", i );
  for( e = 0; e <= FIRorder; e++ ) {
    printf( "%f ", patched.finalFIR[e][0] );
  }
  printf( "\n\nprinting out finalFIRi \n\n", i );
  for( e = 0; e <= FIRorder; e++ ) {
    printf( "%f ", (float)finalFIRi[e][0] *largest / pow_2_15);
  }

  fflush(stdout);
  */

  
  /* prepare FFT convolution */
  
  fprintf(stderr, "FFT setup\n");

  for( channel = 0; channel < channels; channel++ )
  { float* sp = &patched.finalFIR[0][channel];
    float* dp = FFT.time_domain;
    for (e = FIRorder; e; --e)
    { *dp++ = *sp / plansize; // normalize
      sp += 2;
    }
    fprintf(stderr, "FFT setup 1\n");
    memset(dp, 0, (char*)(FFT.time_domain + plansize) - (char*)dp); //padding
    fprintf(stderr, "FFT setup 2\n");
    // transform
    fprintf(stderr, "Alloc: %p %p %p %p\n", FFT.freq_domain, FFT.time_domain, FFT.forward_plan, FFT.backward_plan);
    fftwf_execute(FFT.forward_plan);
    fprintf(stderr, "FFT setup 3\n");
    // store kernel
    memcpy(FFT.kernel[channel], FFT.freq_domain, (plansize/2+1) * sizeof(fftwf_complex));
  }

  eqneedsetup = FALSE;
  return TRUE;
}

/* setting the linear frequency functions and transforming them into the time
/* domain depending on the sampling rate */

#define START_R10  1.25 /* 17,8Hz */
#define JUMP_R10   0.1

static void
fil_init( int samplerate, int channels )
{
  float eqbandtable[NUM_BANDS+1];
  float fftspecres;

  static BOOL FFTinit = FALSE;
  
  int fftbands, i, e, channel;

  if( use_mmx ) {
    FIRorder = (FIRorder + 15) & 0xFFFFFFF0; /* multiple of 16 */
  } else {
    FIRorder = (FIRorder +  1) & 0xFFFFFFFE; /* multiple of 2  */
  }

  if( FIRorder < 2 || FIRorder >= plansize)
  {
    fprintf(stderr, "very bad! %u %u\n", FIRorder, plansize);
    return;
  }
  
  /* choose plansize. Calculate the smallest power of two that is greater or equal to FIRorder. */
  /*frexp(FIRorder-1, &plansize);
  plansize = 1 << plansize;*/
  
  fftbands   = plansize / 2 + 1;
  if (FFTinit)
  { fftwf_destroy_plan(FFT.forward_plan);
    fftwf_destroy_plan(FFT.backward_plan);
    fftwf_free( FFT.freq_domain );
    fftwf_free( FFT.time_domain );
    free(FFT.overlap[0]);
    free(FFT.overlap[1]);
    free(FFT.kernel[0]);
    free(FFT.kernel[1]);
  } else
    FFTinit = TRUE;
  FFT.freq_domain = fftwf_malloc( sizeof( *FFT.freq_domain ) * plansize ); // most likely too large
  FFT.time_domain = fftwf_malloc( sizeof( *FFT.time_domain ) * plansize );
  FFT.forward_plan = fftwf_plan_dft_r2c_1d( plansize, FFT.time_domain, FFT.freq_domain, FFTW_ESTIMATE );
  FFT.backward_plan = fftwf_plan_dft_c2r_1d( plansize, FFT.freq_domain, FFT.time_domain, FFTW_ESTIMATE );
  FFT.overlap[0] = malloc(plansize * sizeof(float));
  memset(FFT.overlap[0], 0, plansize * sizeof(float)); // initially zero
  FFT.overlap[1] = malloc(plansize * sizeof(float));
  memset(FFT.overlap[1], 0, plansize * sizeof(float)); // initially zero
  FFT.kernel[0] = malloc((plansize/2+1) * sizeof(fftwf_complex));
  FFT.kernel[1] = malloc((plansize/2+1) * sizeof(fftwf_complex));
  fprintf(stderr, "Alloc: %p %p %p %p\n", FFT.freq_domain, FFT.time_domain, FFT.forward_plan, FFT.backward_plan);

  fftspecres = (float)samplerate / plansize;

  eqbandtable[0] = 0; // before hearable stuff

  for( i = 0; i < NUM_BANDS; i++ ) {
    eqbandtable[i+1] = pow( 10.0, i * JUMP_R10 + START_R10 );
  }

  for( i = 0; i < NUM_BANDS; i++ )
  {
    FFT.freq_domain[0][0] = 0.0;
    FFT.freq_domain[0][1] = 0.0;
    e = 1;

    // the design of a band pass filter, yah

    while( fftspecres * e < eqbandtable[i] && e < fftbands - 1 )
    {
      FFT.freq_domain[e][0] = 0.0;
      FFT.freq_domain[e][1] = 0.0;
      e++;
    }

    while( fftspecres * e < eqbandtable[i+1] && e < fftbands - 1 )
    {
      FFT.freq_domain[e][0] = 1.0;
      FFT.freq_domain[e][1] = 0.0;
      e++;
    }

    while( e < fftbands )
    {
      FFT.freq_domain[e][0] = 0.0;
      FFT.freq_domain[e][1] = 0.0;
      e++;
    }

    fftwf_execute( FFT.backward_plan );
fprintf(stderr, "Alloc-e: %u, %p %p %p %p\n", e, FFT.freq_domain, FFT.time_domain, FFT.forward_plan, FFT.backward_plan);
    
    // making the FIR filter symetrical
    for( channel = 0; channel < channels; channel++ )
    {
      for( e = FIRorder/2; e; e-- ) {
        eqbandFIR[channel][i][(FIRorder/2)-e] =
        eqbandFIR[channel][i][(FIRorder/2)+e] = FFT.time_domain[e]/2/plansize;
      }

      eqbandFIR[channel][i][(FIRorder/2)] = FFT.time_domain[0]/2/plansize;
    }
  fprintf(stderr, "Alloc-v: %u, %u, %p %p %p %p\n", i, e, FFT.freq_domain, FFT.time_domain, FFT.forward_plan, FFT.backward_plan);
  }

  eqneedinit = FALSE;
  
  fprintf(stderr, "fil_init complete \n");

  fil_setup( channels );
  return;
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

/* apply one FFT cycle and process a most plansize-FIRorder samples
 */
static void do_fft_filter(fftwf_complex* kp)
{ int l;
  fftwf_complex* dp;
  // FFT
  fftwf_execute(FFT.forward_plan);
  // convolution
  dp = FFT.freq_domain;
  for (l = plansize/2+1; l; --l)
  { float tmp = kp[0][0]*dp[0][1] + kp[0][1]*dp[0][0];
    dp[0][0] = kp[0][0]*dp[0][0] - kp[0][1]*dp[0][1];
    dp[0][1] = tmp;
    ++kp;
    ++dp;
  }
  // IFFT
  fftwf_execute(FFT.backward_plan);
}

/* convert samples from short to float and store it in the FFT.time_domain buffer */
static void do_fft_load_samples_mono(const short* sp, const int len)
{ int l;
  float* dp = FFT.time_domain;
  for (l = len >> 3; l; --l) // coarse
  { DO_8(dp[p] = sp[p]);
    sp += 8;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
    *dp++ = *sp++;
  memset(dp, 0, (char*)(FFT.time_domain + plansize) - (char*)dp); //padding
}
/* convert samples from short to float and store it in the FFT.time_domain buffer
 * This will cover only one channel. The only difference to to mono-version is that
 * the source pointer is incremented by 2 per sample. 
 */
static void do_fft_load_samples_stereo(const short* sp, const int len)
{ int l;
  float* dp = FFT.time_domain;
  for (l = len >> 3; l; --l) // coarse
  { DO_8(dp[p] = sp[p<<1]);
    sp += 16;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
  { *dp++ = *sp;
    sp += 2;
  }
  memset(dp, 0, (char*)(FFT.time_domain + plansize) - (char*)dp); //padding
}

_Inline short quantize(float f)
{ if (f < -32768)
    return -32768;
  if (f > 32767)
    return 32767;
  return (short)f; // Well, dithering might be nice, but slow either.
}

/* apply overlap and add mechanism and convert samples back to short (with clipping) */
static void do_fft_overlap_add_mono(short* sp, const int len, float* overlap_buffer)
{ float* dp = FFT.time_domain;
  const float* op = overlap_buffer;
  int l2 = min(len, FIRorder);
  int l;
  // transfer overlapping samples
  for (l = l2 >> 3; l; --l) // coarse
  { DO_8(sp[p] = quantize(dp[p] + op[p]));
    sp += 8;
    dp += 8;
    op += 8;
  }
  for (l = l2 & 7; l; --l) // fine
    *sp++ = quantize(*dp++ + *op++);
  
  l2 = len - FIRorder;
  if (l2 >= 0)
  { // the usual case, transfer remaining samples
    for (l = l2 >> 3; l; --l) // coarse
    { DO_8(sp[p] = quantize(dp[p]));
      sp += 8;
      dp += 8;
    }
    for (l = l2 & 7; l; --l) // fine
      *sp++ = quantize(*dp++);
  } else
  { // the unusual and slower case, add remaining overlap
    for (l = -l2 >> 3; l; --l)
    { DO_8(dp[p] += op[p]);
      dp += 8;
      op += 8;
    }
    for (l = -l2 & 7; l; --l)
      *dp++ += *op++;
  }

  // save overlap
  memcpy(overlap_buffer, FFT.time_domain+len, FIRorder * sizeof *overlap_buffer);
}

/* apply overlap and add mechanism and convert samples back to short (with clipping)
 * This will cover only one channel. The only difference to to mono-version is that
 * the destination pointer is incremented by 2 per sample.
 */
static void do_fft_overlap_add_stereo(short* sp, const int len, float* overlap_buffer)
{ float* dp = FFT.time_domain;
  const float* op = overlap_buffer;
  int l2 = min(len, FIRorder);
  int l;
  fprintf(stderr, "sub1: %u\n", l2);  
  // transfer overlapping samples
  for (l = l2 >> 3; l; --l) // coarse
  { DO_8(sp[p<<1] = quantize(dp[p] + op[p]));
    sp += 16;
    dp += 8;
    op += 8;
  }
  for (l = l2 & 7; l; --l) // fine
  { *sp = quantize(*dp++ + *op++);
    sp += 2;
  }

  l2 = len - FIRorder;
  fprintf(stderr, "sub2: %u\n", l2);  
  if (l2 >= 0)
  { // the usual case, transfer remaining samples
    for (l = l2 >> 3; l; --l) // coarse
    { DO_8(sp[p<<1] = quantize(dp[p]));
      sp += 16;
      dp += 8;
    }
    for (l = l2 & 7; l; --l) // fine
    { *sp = quantize(*dp++);
      sp += 2;
    }
  } else
  { // the unusual and slower case, add remaining overlap
    for (l = -l2 >> 3; l; --l)
    { DO_8(dp[p] += op[p]);
      dp += 8;
      op += 8;
    }
    for (l = -l2 & 7; l; --l)
      *dp++ += *op++;
  }

  // save overlap
  memcpy(overlap_buffer, FFT.time_domain+len, FIRorder * sizeof *overlap_buffer);
}

static void filter_samples_fft_mono(short* newsamples, const short* buf, int len)
{ 
  while (len) // we might need more than one FFT cycle
  { int l2 = plansize - FIRorder;
    if (l2 > len)
      l2 = len;
    
    // convert to float (well, that bill we have to pay)
    do_fft_load_samples_mono(buf, l2);
    // do FFT
    do_fft_filter(FFT.kernel[0]);
    // convert back to short and use overlap/add
    do_fft_overlap_add_mono(newsamples, l2, FFT.overlap[0]);
    
    // next block
    len -= l2;
    newsamples += l2;
    buf += l2;
  }
}

static void filter_samples_fft_stereo( short* newsamples, const short* buf, int len )
{
  while (len) // we might need more than one FFT cycle
  { int l2 = plansize - FIRorder;
    if (l2 > len)
      l2 = len;

    fprintf(stderr, "step1: %u\n", l2);
    // left channel
    // convert to float (well, that bill we have to pay)
    do_fft_load_samples_stereo(buf, l2);
    fprintf(stderr, "step2\n");
    // do FFT
    do_fft_filter(FFT.kernel[0]);
    fprintf(stderr, "step3\n");
    // convert back to short and use overlap/add
    do_fft_overlap_add_stereo(newsamples, l2, FFT.overlap[0]);
    fprintf(stderr, "step4\n");
    
    // right channel
    // convert to float (well, that bill we have to pay)
    do_fft_load_samples_stereo(buf+1, l2);
    fprintf(stderr, "step5\n");
    // do FFT
    do_fft_filter(FFT.kernel[1]);
    fprintf(stderr, "step6\n");
    // convert back to short and use overlap/add
    do_fft_overlap_add_stereo(newsamples+1, l2, FFT.overlap[1]);
    fprintf(stderr, "step7\n");

    // next block
    len -= l2;
    l2 <<= 1; // stereo
    newsamples += l2;
    buf += l2;
    fprintf(stderr, "step8: %u\n", len);
  }
}

/* Entry point: do filtering */
int _System
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
    {
      f->last_format = *format;
      fil_init( format->samplerate, format->channels );
    }
    else if( eqneedinit  ) {
      fil_init( format->samplerate, format->channels );
    }
    else if( eqneedsetup ) {
      fil_setup( format->channels );
    }

    memcpy((char*)temp, (char*)temp+f->prevlen, FIRorder*format->channels*format->bits/8 );
    f->prevlen = len;

    if (use_fft)
    { 
      if( format->channels == 2 ) {
        filter_samples_fft_stereo( newsamples, (short*)buf, len>>2 );
      } else if( format->channels == 1 ) {
        filter_samples_fft_mono( newsamples, (short*)buf, len>>1 );
      }
    } else if( use_mmx )
    {
      if( format->channels == 2 ) {
        filter_samples_mmx_stereo( newsamples, temp, buf, len );
      } else if( format->channels == 1 ) {
        filter_samples_mmx_mono( newsamples, temp, buf, len );
      }
    }
    else
    {
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
    save_ini_value ( INIhandle, FIRorder );
    save_ini_value ( INIhandle, plansize );
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
  eqneedsetup = TRUE;
  use_mmx     = mmx_present;
  use_fft     = FALSE;
  lasteq[0]   = 0;
  plansize    = 8192;
  locklr      = FALSE;

  if( use_mmx ) {
    FIRorder  = 512;
  } else {
    FIRorder  = 64;
  }

  if(( INIhandle = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value ( INIhandle, FIRorder );
    load_ini_value ( INIhandle, plansize );
    load_ini_value ( INIhandle, eqenabled );
    load_ini_value ( INIhandle, locklr );
    load_ini_value ( INIhandle, use_mmx );
    load_ini_value ( INIhandle, use_fft );
    load_ini_string( INIhandle, lasteq, sizeof( lasteq ));

    close_ini( INIhandle );
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
      fprintf(file, "%u\n", mutes[i]);
    }
    fprintf( file, "# Preamplifier\n" );
    fprintf( file, "%g\n", preamp );

    fprintf( file, "# End of equalizer\n" );
    fclose ( file );
    return TRUE;
  }

  return FALSE;
}

static void drivedir( char* buf, char* fullpath )
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

void _System
plugin_query( PLUGIN_QUERYPARAM *param )
{
  param->type = PLUGIN_FILTER;
  param->author = "Samuel Audet";
  param->desc = VERSION;
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
    for( i = 0; i < 32; i++ ) {

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

      range = (float)SHORT2FROMMR( rangevalue );

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
  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETLIMITS, MPFROMLONG( 4096 ), MPFROMLONG( 16 ));
  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETCURRENTVALUE, MPFROMLONG( FIRorder ), 0 );
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETMASTER, MPFROMLONG( NULLHANDLE ), 0 );
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETLIMITS, MPFROMLONG( MAX_COEF ), MPFROMLONG( 32 ));
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETCURRENTVALUE, MPFROMLONG( plansize ), 0 );
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
          eqneedsetup = TRUE;
          break;

        case EQ_ZERO:
        {
          int e,i;
          MRESULT rangevalue;

          for( e = 0; e < 2; e++ ) {
            for( i = 0; i < 32; i++ ) {

              rangevalue = WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_QUERYSLIDERINFO,
                                              MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

              WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*e+i, SLM_SETSLIDERINFO,
                                 MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                                 MPFROMSHORT( SHORT2FROMMR( rangevalue ) >> 1 ));

              WinSendDlgItemMsg( hwnd, 100+NUM_BANDS*e+i, BM_SETCHECK, 0, 0 );
            }
          }
          eqneedsetup = TRUE;
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
          if (use_mmx)
          { use_fft = FALSE;
            WinSendDlgItemMsg( hwnd, ID_USEFFT, BM_SETCHECK, MPFROMSHORT( FALSE ), 0 );
          }
          break;

        case ID_USEFFT:
          use_fft = WinQueryButtonCheckstate( hwnd, id );
          if (use_fft)
          { use_mmx = FALSE;
            WinSendDlgItemMsg( hwnd, ID_USEMMX, BM_SETCHECK, MPFROMSHORT( FALSE ), 0 );
          }
          break;

        case ID_FIRORDER:
          switch( SHORT2FROMMP( mp1 ))
          {
            case SPBN_CHANGE:
              WinSendDlgItemMsg( hwnd, id, SPBM_QUERYVALUE, MPFROMP( &temp ), 0 );
              if (temp > 4096 || temp >= plansize)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG( FIRorder ), 0 ); // restore
               else
              { eqneedinit = TRUE;
                FIRorder = temp; // TODO: dirty threading issue
              }
              break;

            case SPBN_UPARROW:
              temp = (FIRorder & ~0xF) + 16;
              if (temp < plansize)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG( temp ), 0 );
              break;

            case SPBN_DOWNARROW:
              temp = (FIRorder & ~0xF) - 16;
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
              if (temp > MAX_COEF || temp <= FIRorder)
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG(plansize), 0 ); // restore
               else
              { plansize = temp;
                eqneedinit = TRUE;
              }
              break;

            case SPBN_UPARROW:
              temp = plansize << 1;
              if (temp > MAX_COEF)
                temp = MAX_COEF;
              WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG(temp), 0 );
              break;

            case SPBN_DOWNARROW:
              temp = plansize >> 1;
              if (temp <= FIRorder)
              { int i;
                frexp(FIRorder, &i);
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
                eqneedsetup = TRUE;
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
                eqneedsetup = TRUE;
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

void _System
plugin_configure( HWND hwnd, HMODULE module )
{
  if( !hdialog ) {
    hdialog = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP, ConfigureDlgProc, module, ID_EQ, NULL );
    do_warpsans( hdialog );
  }

  WinShowWindow( hdialog, TRUE );
  WinSetFocus  ( HWND_DESKTOP, WinWindowFromID( hdialog, EQ_ENABLED ));
}

BOOL _System
_DLL_InitTerm( ULONG hModule, ULONG flag )
{
  if(flag == 0)
  {
    mmx_present = detect_mmx();
    if( _CRT_init() == -1 ) {
      return FALSE;
    }
    load_ini();
    load_eq_file( lasteq, (float*)bandgain, (BOOL*)mute, &preamp );
  }
  else
  {
    save_ini();
   _CRT_term();
  }

  return TRUE;
}

