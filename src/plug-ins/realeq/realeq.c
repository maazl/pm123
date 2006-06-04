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

#define VERSION "Real Equalizer 1.20"
#define MAX_COEF 8192
#define NUM_BANDS  32

static BOOL  mmx_present;
static BOOL  eqneedinit  = TRUE;
static BOOL  eqneedsetup = TRUE;

static float eqbandFIR[2][NUM_BANDS][4096];

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

// settings
int  FIRorder;
int  plansize;
BOOL use_mmx   = FALSE;
BOOL eqenabled = FALSE;
BOOL locklr    = FALSE;
char lasteq[CCHMAXPATH];

#define M_PI 3.14159265358979323846

#define WINDOW_HANN( n, N )    (0.50 - 0.50 * cos( 2 * M_PI * n / N ))
#define WINDOW_HAMMING( n, N ) (0.54 - 0.46 * cos( 2 * M_PI * n / N ))

#define round(n) ((n) > 0 ? (n) + 0.5 : (n) - 0.5)

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
      patched.finalFIR[e][channel] *= 2 * WINDOW_HAMMING( e, FIRorder );
      if( fabs( patched.finalFIR[e][channel] ) > fabs( largest )) {
        largest = fabs( patched.finalFIR[e][channel] );
      }
    }
  }
 
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

  float*         out;
  fftwf_complex* in;
  fftwf_plan     plan;

  int fftbands, i, e, channel, plansize;

  /* it is a good advice to choose the plansize automatically at this point (MM) */
  if( FIRorder < 2 ) // || plansize < 2 || (plansize & (plansize - 1))) {
    return;

  if( use_mmx ) {
    FIRorder = (FIRorder + 15) & 0xFFFFFFF0; /* multiple of 16 */
  } else {
    FIRorder = (FIRorder +  1) & 0xFFFFFFFE; /* multiple of 2  */
  }

  /* choose plansize. Calculate the smallest power of two that is greater or equal to FIRorder. */
  frexp(FIRorder-1, &plansize);
  plansize = 1 << plansize;
  
  fftbands   = plansize / 2 + 1;
  in         = fftwf_malloc( sizeof( *in  ) * plansize );
  out        = fftwf_malloc( sizeof( *out ) * plansize );
  fftspecres = (float)samplerate / plansize;

  eqbandtable[0] = 0; // before hearable stuff

  for( i = 0; i < NUM_BANDS; i++ ) {
    eqbandtable[i+1] = pow( 10.0, i * JUMP_R10 + START_R10 );
  }

  for( i = 0; i < NUM_BANDS; i++ )
  {
    in[0][0] = 0.0;
    in[0][1] = 0.0;
    e = 1;

    // the design of a band pass filter, yah

    while( fftspecres * e < eqbandtable[i] && e < fftbands - 1 )
    {
      in[e][0] = 0.0;
      in[e][1] = 0.0;
      e++;
    }

    while( fftspecres * e < eqbandtable[i+1] && e < fftbands - 1 )
    {
      in[e][0] = 1.0;
      in[e][1] = 0.0;
      e++;
    }

    while( e < fftbands )
    {
      in[e][0] = 0.0;
      in[e][1] = 0.0;
      e++;
    }

    plan = fftwf_plan_dft_c2r_1d( plansize, in, out, FFTW_ESTIMATE );
    fftwf_execute( plan );
    fftwf_destroy_plan( plan );

    // making the FIR filter symetrical
    for( channel = 0; channel < channels; channel++ )
    {
      for( e = FIRorder/2; e; e-- ) {
        eqbandFIR[channel][i][(FIRorder/2)-e] =
        eqbandFIR[channel][i][(FIRorder/2)+e] = out[e]/2/plansize;
      }

      eqbandFIR[channel][i][(FIRorder/2)] = out[0]/2/plansize;
    }
  }

  fftwf_free( in  );
  fftwf_free( out );

  eqneedinit = FALSE;

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

/* Entry point: do filtering */
int _System
filter_play_samples( void* F, FORMAT_INFO* format, char* buf, int len, int posmarker )
{
  REALEQ_STRUCT* f = (REALEQ_STRUCT*)F;

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

    if( use_mmx )
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
  WinSendDlgItemMsg( hwnd, EQ_ENABLED, BM_SETCHECK, MPFROMCHAR( eqenabled ), 0 );
  WinSendDlgItemMsg( hwnd, ID_LOCKLR,  BM_SETCHECK, MPFROMCHAR( locklr ), 0 );
  WinSendDlgItemMsg( hwnd, ID_USEMMX,  BM_SETCHECK, MPFROMCHAR( use_mmx ), 0 );

  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETLIMITS, MPFROMLONG( 4096 ), MPFROMLONG( 2 ));
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETLIMITS, MPFROMLONG( MAX_COEF ), MPFROMLONG( 2 ));
  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETCURRENTVALUE, MPFROMLONG( FIRorder ), 0 );
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
          eqenabled = SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, BM_QUERYCHECK, 0, 0 ));
          break;

        case ID_LOCKLR:
          locklr = SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, BM_QUERYCHECK, 0, 0 ));
          break;

        case ID_USEMMX:
          use_mmx = SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, BM_QUERYCHECK, 0, 0 ));
          break;

        case ID_FIRORDER:
          switch( SHORT2FROMMP( mp1 ))
          {
            case SPBN_CHANGE:
              WinSendDlgItemMsg( hwnd, id, SPBM_QUERYVALUE, MPFROMP( &FIRorder ), 0 );
              if( FIRorder > 4096 ) {
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG( 4096 ), 0 );
              } else {
                eqneedinit = TRUE;
              }
              break;

            case SPBN_UPARROW:
              WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SPBM_QUERYVALUE, MPFROMP( &temp ), 0 );
              if( use_mmx ) {
                temp = (temp+15) & 0xFFFFFFF0;
              } else {
                temp = (temp+ 1) & 0xFFFFFFFE;
              }
              if( temp > 4096 || temp < 2 ) {
                temp = 16;
              }
              WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SPBM_SETCURRENTVALUE, MPFROMLONG( temp ), 0 );
              break;

            case SPBN_DOWNARROW:
              WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SPBM_QUERYVALUE, MPFROMP(&temp), 0 );
              if( use_mmx ) {
                temp = (temp-15) & 0xFFFFFFF0;
              } else {
                temp = (temp- 1) & 0xFFFFFFFE;
              }
              if( temp > 4096 || temp < 2 ) {
                temp = 4096;
              }
              WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SPBM_SETCURRENTVALUE, MPFROMLONG(temp), 0 );
              break;
          }
          break;

        case ID_PLANSIZE:
          switch( SHORT2FROMMP( mp1 ))
          {
            case SPBN_CHANGE:
              WinSendDlgItemMsg( hwnd, id, SPBM_QUERYVALUE, MPFROMP(&plansize), 0 );
              if( plansize > MAX_COEF ) {
                WinSendDlgItemMsg( hwnd, id, SPBM_SETCURRENTVALUE, MPFROMLONG(MAX_COEF), 0 );
              } else {
                eqneedinit = TRUE;
              }
              break;

            case SPBN_UPARROW:
              WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SPBM_QUERYVALUE, MPFROMP(&temp), 0 );
              if( temp != 2 && temp != MAX_COEF ) {
                temp = log(temp)/log(2.0)+1.5;
                WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1),
                                   SPBM_SETCURRENTVALUE, MPFROMLONG((ULONG)pow(2.0,temp)), 0 );
              }
              break;

            case SPBN_DOWNARROW:
              WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1), SPBM_QUERYVALUE, MPFROMP(&temp), 0 );
              if( temp != 2 && temp != MAX_COEF ) {
                temp = log(temp)/log(2.0)-0.5;
                WinSendDlgItemMsg( hwnd, SHORT1FROMMP(mp1),
                                   SPBM_SETCURRENTVALUE, MPFROMLONG((ULONG)pow(2.0,temp)), 0 );
              }
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
  HWND dlghwnd = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP, ConfigureDlgProc, module, ID_EQ, NULL );

  if( dlghwnd )
  {
    do_warpsans( dlghwnd );
    WinSetFocus( HWND_DESKTOP,WinWindowFromID( dlghwnd, EQ_ENABLED ));
    WinShowWindow( dlghwnd, TRUE );
  }
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

