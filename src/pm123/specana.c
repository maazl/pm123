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

int _System
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
    wnd[i] = WINDOW_HAMMING( i, numsamples - 1 );
  }

  initialized = TRUE;
  return numsamples / 2 + 1;
}

int _System
specana_dobands( float* bands )
{
  FORMAT_INFO bufferinfo;
  int i, e;

  if( !initialized || !bands ) {
    return 0;
  }

  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );

  if( out_playing_samples( &bufferinfo, NULL, 0 ) != 0 ) {
    return 0;
  }

  if( bufferinfo.bits <= 8 )
  {
    int len = bufferinfo.channels * numsamples;
    unsigned char *sample = _alloca( len );

    if( !sample || out_playing_samples( &bufferinfo, sample, len ) != 0 ) {
      return 0;
    }

    memset( in, 0, numsamples * sizeof(*in));

    for( i = 0; i < numsamples; i++ ) {
      for( e = 0; e < bufferinfo.channels; e++ ) {
        in[i] += ( *sample++ - 128 ) / bufferinfo.channels;
      }
    }
  }
  else if( bufferinfo.bits <= 16 )
  {
    int len = 2 * bufferinfo.channels * numsamples;
    signed short *sample = _alloca( len );

    if( !sample || out_playing_samples( &bufferinfo, (char*)sample, len ) != 0 ) {
      return 0;
    }

    memset( in, 0, numsamples * sizeof(*in));

    for( i = 0; i < numsamples; i++ ) {
      for( e = 0; e < bufferinfo.channels; e++ ) {
        in[i] += *sample++ / bufferinfo.channels;
      }
    }
  }
  else if( bufferinfo.bits <= 32 )
  {
    int len = 4 * bufferinfo.channels * numsamples;
    signed long *sample = _alloca( len );

    if( !sample || out_playing_samples( &bufferinfo, (char*)sample, len ) != 0 ) {
      return 0;
    }

    memset( in, 0, numsamples * sizeof(*in));

    for( i = 0; i < numsamples; i++ ) {
      for( e = 0; e < bufferinfo.channels; e++ ) {
        in[i] += *sample++ / bufferinfo.channels;
      }
    }
  }
  else
  {
    return 0;
  }

  // To reduce spectral leakage, the samples are multipled with a window.
  for( i = 0; i < numsamples; i++ ) {
    in[i] *= wnd[i];
  }

  fftwf_execute( plan );

  for( i = 0; i < numsamples/2 + 1; i++ ) {
    bands[i] = sqrt( out[i][0]*out[i][0] +out[i][1]*out[i][1] ) / numsamples;
  }

  // max amplitude
  return 0x7FFFFFFFUL >> ( 32 - bufferinfo.bits );
}
