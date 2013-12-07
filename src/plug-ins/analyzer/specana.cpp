/*
 * Code that uses fft123.dll to make some interesting data to display
 *
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

#define INCL_WIN
#define PLUGIN_INTERFACE_LEVEL 3

#include "specana.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <malloc.h>

#include <format.h>
#include <fftw3.h>

#include <os2.h>

#include <debuglog.h>

#ifndef  M_PI
#define  M_PI 3.14159265358979323846
#endif

//#define DEBUG_LOG

#define  WINDOW_HAMMING( n, N )  ( 0.54 - 0.46 * cos( 2 * M_PI * (n) / (N) ))
#define  WINDOW_HANN( n, N )     ( 0.50 - 0.50 * cos( 2 * M_PI * (n) / (N) ))
#define  WINDOW_BLACKMAN( n, N ) ( 0.42 - 0.50 * cos( 2 * M_PI * (n) / (N) )\
                                        + 0.08 * cos( 4 * M_PI * (n) / (N) ))

static int            last_numsamples = 0;// number of samples in the last call
static WIN_FN         last_winfn;    // window function in wnd[]
static float*         last_buf = NULL;// last samples from output
static float*         wnd  = NULL;   // window function (2nd half) [numsamples/2]
static fftwf_complex* out  = NULL;   // FFT output buffer [numsamples/2+1]
static fftwf_plan     plan = NULL;   // fftw plan


#define DO_4(p,code) \
{ { static const int p = 0; code; } \
  { static const int p = 1; code; } \
  { static const int p = 2; code; } \
  { static const int p = 3; code; } \
}


static bool init_fft(float* in)
{
  free(wnd); // always destroy window
  wnd = NULL;
  fftwf_free(out);
  out = (fftwf_complex*)fftwf_malloc(sizeof(*out) * (last_numsamples/2+1) );
  if (out == NULL)
    return false;

  // By default there are by far too many FP exceptions
  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );

  DEBUGLOG(("SA: init_fft %p, %d, %p, %p\n", plan, last_numsamples, in, out));
  fftwf_destroy_plan(plan);
  plan = fftwf_plan_dft_r2c_1d(last_numsamples, in, out, FFTW_ESTIMATE);

  return plan != NULL;
}

static bool init_window(void)
{
  int i;
  float* dp;
  
  free(wnd);
  wnd = dp = (float*)malloc((last_numsamples >> 1) * sizeof *wnd);
  if (dp == NULL)
    return false;
  DEBUGLOG(("SA: init_window %d\n", last_numsamples));

  switch (last_winfn)
  {default:
    return false;
   case WINFN_HAMMING:
    for (i = last_numsamples >> 1; i; i -= 4)
    { DO_4(p, dp[p] = WINDOW_HAMMING(i-p-1, last_numsamples-1));
      dp += 4;
    }
    break;
   case WINFN_BLACKMAN:
    for (i = last_numsamples >> 1; i; i -= 4)
    { DO_4(p, dp[p] = WINDOW_BLACKMAN(i-p-1, last_numsamples-1));
      dp += 4;
    }
  }
  /*#ifdef DEBUG_LOG
  DEBUGLOG(("SA: window:"));
  for (i = 0; i <= last_numsamples >> 1; ++i)
  { DEBUGLOG((" %.3f", wnd[i]));
  }
  DEBUGLOG(("\n"));
  #endif*/
  return true;
}

SPECANA_RET specana_do(float* in, int numsamples, WIN_FN winfn, float* bands)
{ DEBUGLOG(("SA: specana_do(%p, %d, %d, %p)\n", in, numsamples, winfn, bands));

  if (numsamples & 7)
    return SPECANA_ERROR;

  // allocate buffer for last samples?
  int len = numsamples * sizeof(float); // From here len is the length in bytes
  if (last_buf == NULL || last_numsamples != numsamples)
  { free(last_buf);
    last_buf = (float*)malloc(len);
    if (last_buf == NULL)
      return SPECANA_ERROR;
    DEBUGLOG(("SA: new last_buf\n"));
  } else if (memcmp(last_buf, in, len) == 0)
    return SPECANA_UNCHANGED;
  memcpy(last_buf, in, len);

  // check whether to (re)init FFT and window function
  if (plan == NULL || numsamples != last_numsamples)
  { last_numsamples = numsamples;
    last_winfn = winfn;
    if (!init_fft(in) || !init_window())
      return SPECANA_ERROR;
  } else if (wnd == NULL || last_winfn != winfn)
  { last_winfn = winfn;
    if (!init_window())
      return SPECANA_ERROR;
  } 

  // To reduce spectral leakage, the samples are multipled with a window.
  { float* wp = wnd;
    float* dp2 = in + (numsamples >> 1); 
    float* dp = dp2 -1;
    for (int i = numsamples >> 3; i; --i)
    { DO_4(p, dp[-p] *= wp[p]; dp2[p] *= wp[p]);
      dp -= 4;
      dp2 += 4;
      wp += 4;
    }
  }
  DEBUGLOG(("SA: win done\n"));

  // transform into the time domain (in -> out)
  fftwf_execute(plan);
  DEBUGLOG(("SA: FFT done\n"));
  // fetch the norm of the coefficients
  // Additionally compensate for the channel addition and the unnormalized FFT above.
  { fftwf_complex* sp = out;
    float scale = 1. / sqrt(numsamples);// normalize
    for (int i = numsamples >> 3; i; --i)
    { DO_4(p, bands[p] = abs(sp[p]) * scale);
      bands += 4;
      sp += 4;
    }
    bands[0] = sp[0].real() * scale; // nyquist frequency
  }
  DEBUGLOG(("SA: bands stored\n"));
  
  return SPECANA_OK;
}

void specana_uninit(void)
{ last_numsamples = 0;
  free(last_buf);
  last_buf = NULL;
  free(wnd);
  wnd = NULL;
  fftwf_free(out);
  out = NULL;
  if (plan)
  { fftwf_destroy_plan(plan);
    plan = NULL;
  }
}

