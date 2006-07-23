/*
 * Code that uses fft123.dll to make some interesting data to display
 *
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

#define  INCL_WIN
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <malloc.h>

#include "utilfct.h"
#include "format.h"
#include "decoder_plug.h"
#include "output_plug.h"
#include "filter_plug.h"
#include "plugin.h"
#include "plugman.h"
#include "fftw3.h"

#define  M_PI 3.14159265358979323846

#define  WINDOW_HAMMING( n, N )  ( 0.54 - 0.46 * cos( 2 * M_PI * n / N ))
#define  WINDOW_HANN( n, N )     ( 0.50 - 0.50 * cos( 2 * M_PI * n / N ))
#define  WINDOW_BLACKMAN( n, N ) ( 0.42 - 0.50 * cos( 2 * M_PI * n / N )\
                                        + 0.08 * cos( 4 * M_PI * n / N ))

static int  numsamples;
static BOOL initialized = FALSE;

static float*         wnd;
static float*         in;
static fftwf_complex* out;
static fftwf_plan     plan;

int PM123_ENTRY
specana_init( int set_numsamples )
{
  int i;

  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );

  initialized = FALSE;
  numsamples  = set_numsamples;

  fftwf_free( wnd );
  fftwf_free( in  );
  fftwf_free( out );

  if( !( wnd = fftwf_malloc( sizeof(*wnd) * numsamples )) ||
      !( in  = fftwf_malloc( sizeof(*in ) * numsamples )) ||
      !( out = fftwf_malloc( sizeof(*out) * numsamples ))  )
  {
    return 0;
  }

  fftwf_destroy_plan( plan );
  plan = fftwf_plan_dft_r2c_1d( numsamples, in, out, FFTW_ESTIMATE );

  if( !plan ) {
    return 0;
  }

  for( i = 0; i < numsamples; i++ ) {
    wnd[i] = WINDOW_BLACKMAN( i, numsamples - 1 );
  }

  initialized = TRUE;
  return numsamples / 2 + 1;
}

#define DO_4(p,code) \
{ { static const int p = 0; code; } \
  { static const int p = 1; code; } \
  { static const int p = 2; code; } \
  { static const int p = 3; code; } \
}

int PM123_ENTRY
specana_dobands( float* bands, FORMAT_INFO* info )
{
  FORMAT_INFO bufferinfo;
  int i, e;
  float* dp = in;
  float scale;

  if( !initialized || !bands ) {
    return 0;
  }

  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );

  if( out_playing_samples( &bufferinfo, NULL, 0 ) != 0 ) {
    return 0;
  }

  // Oh, well, C++ with templates would be nice...
  if( bufferinfo.bits <= 8 )
  {
    int len = bufferinfo.channels * numsamples;
    unsigned char *sample = alloca( len+3 );

    if( !sample || out_playing_samples( &bufferinfo, sample, len ) != 0 ) {
      return 0;
    }

    switch (bufferinfo.channels)
    {case 0:
      for ( i = (numsamples+3)/4; i; --i )
      { DO_4(p, dp[p] = sample[p] - 128);
        dp += 4;
        sample += 4;
      }
      break;
     case 1:
      for ( i = (numsamples+3)/4; i; --i )
      { DO_4(p, dp[p] = (int)sample[2*p] + sample[2*p+1] - 256);
        dp += 4;
        sample += 4;
      }
      break;
     default:  
      for( i = numsamples; i; --i )
      { register float s = 0;
        for( e = bufferinfo.channels; e; --e )
          s += *sample++ - 128;
        *dp++ = s;
      }
    }
    scale = 1./128; // normalize
  }
  else if( bufferinfo.bits <= 16 )
  {
    int len = 2 * bufferinfo.channels * numsamples;
    signed short *sample = alloca( len+6 );

    if( !sample || out_playing_samples( &bufferinfo, (char*)sample, len ) != 0 ) {
      return 0;
    }

    switch (bufferinfo.channels)
    {case 0:
      for ( i = (numsamples+3)/4; i; --i )
      { DO_4(p, dp[p] = sample[p]);
        dp += 4;
        sample += 4;
      }
      break;
     case 1:
      for ( i = (numsamples+3)/4; i; --i )
      { DO_4(p, dp[p] = (int)sample[2*p] + sample[2*p+1]);
        dp += 4;
        sample += 4;
      }
      break;
     default:  
      for( i = numsamples; i; --i )
      { register float s = 0;
        for( e = bufferinfo.channels; e; --e )
          s += *sample++;
        *dp++ = s;
      }
    }
    scale = 1./32768; // normalize
  }
  else if( bufferinfo.bits <= 32 )
  {
    int len = 4 * bufferinfo.channels * numsamples;
    signed long *sample = alloca( len+12 );

    if( !sample || out_playing_samples( &bufferinfo, (char*)sample, len ) != 0 ) {
      return 0;
    }

    switch (bufferinfo.channels)
    {case 0:
      for ( i = (numsamples+3)/4; i; --i )
      { DO_4(p, dp[p] = sample[p]);
        dp += 4;
        sample += 4;
      }
      break;
     case 1:
      for ( i = (numsamples+3)/4; i; --i )
      { DO_4(p, dp[p] = (float)sample[2*p] + sample[2*p+1]);
        dp += 4;
        sample += 4;
      }
      break;
     default:  
      for( i = numsamples; i; --i )
      { register float s = 0;
        for( e = bufferinfo.channels; e; --e )
          s += *sample++;
        *dp++ = s;
      }
    }
    scale = 1./0x80000000U; // normalize
    // I am unsure how the 0dB level of >16Bit data is usually defined.
  }
  else
  {
    return 0;
  }

  // To reduce spectral leakage, the samples are multipled with a window.
  // Additionaly compensate for the channel addition above and the unnormalized FFT below.
  { float* wp = wnd; 
    dp = in;
    scale /= bufferinfo.channels * sqrt(numsamples);
    for ( i = (numsamples+3)/4; i; --i )
    { DO_4(p, dp[p] *= wp[p] * scale);
      dp += 4;
      wp += 4;
    }
  }

  fftwf_execute( plan );

  for( i = 0; i < numsamples/2 + 1; i++ ) {
    bands[i] = sqrt( out[i][0]*out[i][0] + out[i][1]*out[i][1] );
  }

  // max amplitude, already normalized above => always 1.0
  return 1;
}
