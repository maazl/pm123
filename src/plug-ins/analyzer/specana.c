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

#include <format.h>
#include <fftw3.h>

#include "specana.h"

#ifndef  M_PI
#define  M_PI 3.14159265358979323846
#endif

//#define DEBUG

#define  WINDOW_HAMMING( n, N )  ( 0.54 - 0.46 * cos( 2 * M_PI * (n) / (N) ))
#define  WINDOW_HANN( n, N )     ( 0.50 - 0.50 * cos( 2 * M_PI * (n) / (N) ))
#define  WINDOW_BLACKMAN( n, N ) ( 0.42 - 0.50 * cos( 2 * M_PI * (n) / (N) )\
                                        + 0.08 * cos( 4 * M_PI * (n) / (N) ))

// Entry points
ULONG (PM123_ENTRYP decoderPlayingSamples)( FORMAT_INFO *info, char *buf, int len );
BOOL  (PM123_ENTRYP decoderPlaying)( void );

static int            last_numsamples = 0;// number of samples in the last call
static WIN_FN         last_winfn;    // window function in wnd[]
static int            last_bps = 0;  // bytes per sample
static char*          last_buf = NULL;// last samples from output
static float*         wnd  = NULL;   // window function (2nd half) [numsamples/2]
static float*         in   = NULL;   // FFT input buffer [numsamples]
static fftwf_complex* out  = NULL;   // FFT output buffer [numsamples/2+1]
static fftwf_plan     plan = NULL;   // fftw plan


#define DO_4(p,code) \
{ { static const int p = 0; code; } \
  { static const int p = 1; code; } \
  { static const int p = 2; code; } \
  { static const int p = 3; code; } \
}


/* calculate the number of bytes per sample */
static int clac_bps(const FORMAT_INFO* info)
{ if (info->bits <= 8)
    return info->channels * sizeof(unsigned char);
   else if (info->bits <= 16)
    return info->channels * sizeof(short);
   else if (info->bits <= 32)
    return info->channels * sizeof(long);
   else
    return 0;
}


static BOOL init_fft(void)
{
  free(wnd); // always destroy window
  wnd = NULL;
  fftwf_free(in);
  fftwf_free(out);
  in  = fftwf_malloc(sizeof(*in ) * last_numsamples );
  out = fftwf_malloc(sizeof(*out) * (last_numsamples/2+1) );
  if (in == NULL || out == NULL)
    return FALSE;

  #ifdef DEBUG
  fprintf(stderr, "SA: init_fft %d\n", last_numsamples);
  #endif
  fftwf_destroy_plan(plan);
  plan = fftwf_plan_dft_r2c_1d(last_numsamples, in, out, FFTW_ESTIMATE);

  return plan != NULL;
}

static BOOL init_window(void)
{
  int i;
  float* dp;
  
  free(wnd);
  wnd = dp = malloc((last_numsamples >> 1) * sizeof *wnd);
  if (dp == NULL)
    return FALSE;
  #ifdef DEBUG
  fprintf(stderr, "SA: init_window %d\n", last_numsamples);
  #endif

  switch (last_winfn)
  {default:
    return FALSE;
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
  #ifdef DEBUG
  fprintf(stderr, "SA: window:");
  for (i = 0; i <= last_numsamples >> 1; ++i)
  { fprintf(stderr, " %.3f", wnd[i]);
  }
  fprintf(stderr, "\n");
  #endif
  return TRUE;
}

static void fetch_byte(unsigned char* sp, int ch)
{ int i,e;
  float* dp = in;
  #ifdef DEBUG
  fprintf(stderr, "SA fetch_byte(%p, %d) %d\n", sp, ch, last_numsamples); 
  #endif
  switch (ch)
  {case 1:
    for (i = last_numsamples >> 2; i; --i )
    { DO_4(p, dp[p] = sp[p] - 128);
      dp += 4;
      sp += 4;
    }
    break;
   case 2:
    for ( i = last_numsamples >> 2; i; --i )
    { DO_4(p, dp[p] = (int)sp[2*p] + sp[2*p+1] - 256);
      dp += 4;
      sp += 2 * 4;
    }
    break;
   default:  
    for( i = last_numsamples; i; --i )
    { register float s = 0;
      for( e = ch; e; --e )
        s += *sp++ - 128;
      *dp++ = s;
    }
  }
}

static void fetch_short(short* sp, int ch)
{ int i,e;
  float* dp = in;
  #ifdef DEBUG
  fprintf(stderr, "SA fetch_short(%p, %d) %d\n", sp, ch, last_numsamples); 
  #endif
  switch (ch)
  {case 1:
    for ( i = last_numsamples >> 2; i; --i )
    { DO_4(p, dp[p] = sp[p]);
      dp += 4;
      sp += 4;
    }
    break;
   case 2:
    for ( i = last_numsamples >> 2; i; --i )
    { DO_4(p, dp[p] = (int)sp[2*p] + sp[2*p+1]);
      dp += 4;
      sp += 2 * 4;
    }
    break;
   default:  
    for( i = last_numsamples; i; --i )
    { register float s = 0;
      for( e = ch; e; --e )
        s += *sp++;
      *dp++ = s;
    }
  }
}

static void fetch_long(long* sp, int ch)
{ int i,e;
  float* dp = in;
  #ifdef DEBUG
  fprintf(stderr, "SA fetch_long(%p, %d) %d\n", sp, ch, last_numsamples); 
  #endif
  switch (ch)
  {case 1:
    for ( i = last_numsamples >> 2; i; --i )
    { DO_4(p, dp[p] = sp[p]);
      dp += 4;
      sp += 4;
    }
    break;
   case 2:
    for ( i = last_numsamples >> 2; i; --i )
    { DO_4(p, dp[p] = (float)sp[2*p] + sp[2*p+1]);
      dp += 4;
      sp += 2 * 4;
    }
    break;
   default:  
    for( i = last_numsamples; i; --i )
    { register float s = 0;
      for( e = ch; e; --e )
        s += *sp++;
      *dp++ = s;
    }
  }
}

SPECANA_RET specana_do(int numsamples, WIN_FN winfn, float* bands, FORMAT_INFO* info)
{
  FORMAT_INFO bufferinfo;
  int len;
  int i;
  float scale;
  char* sample;

  #ifdef DEBUG
  fprintf(stderr, "SA: specana_do(%d, %d, %p, {%d,%d,%d,%d}) %d\n", numsamples, winfn, bands, info->size, info->samplerate, info->channels, info->bits, last_bps);
  #endif
  if (numsamples & 7)
    return SPECANA_ERROR;

  // check whether we have to examine the format first
  if (info->samplerate == 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "SA: FORMAT_INIT - %d\n", (*decoderPlayingSamples)(&bufferinfo, NULL, 0));
    #endif
    if ((*decoderPlayingSamples)(&bufferinfo, NULL, 0) != 0)
      return SPECANA_ERROR;
    last_bps = clac_bps(&bufferinfo);
    goto clb;
  } else if (last_bps <= 0)
  { last_bps = clac_bps(info);
   clb:
    free(last_buf);
    last_buf = NULL;
    // calculate bytes per sample
    if (last_bps == 0)
      return SPECANA_ERROR;
  }

 redo: // restart here in case of an unexpected format change
  // allocate required buffer length
  len = numsamples * last_bps;
  sample = alloca(len);
  #ifdef DEBUG
  fprintf(stderr, "SA: len = %d, bps = %d\n", len, last_bps);
  #endif
  
  // fetch data
  if ((*decoderPlayingSamples)(&bufferinfo, sample, len) != 0)
    return SPECANA_ERROR;
  // check for format change
  if (info->samplerate != bufferinfo.samplerate || info->channels != bufferinfo.channels || info->bits != bufferinfo.bits)
  { *info = bufferinfo;
    return SPECANA_NEWFORMAT;
  }
  // check bps
  i = clac_bps(&bufferinfo);
  if (i != last_bps)
  { if (i == 0)
      return SPECANA_ERROR;
    free(last_buf);
    last_buf = NULL;
    if (i > last_bps)
    { last_bps = i;
      goto redo;
    } else
      last_bps = i; // the buffer was too large, continue anyway
  }
  // allocate buffer for last samples?
  if (last_buf == NULL || last_numsamples != numsamples)
  { free(last_buf);
    last_buf = malloc(len);
    if (last_buf == NULL)
      return SPECANA_ERROR;
    #ifdef DEBUG
    fprintf(stderr, "SA: new last_buf\n");
    #endif
  } else if (memcmp(last_buf, sample, len) == 0)
    return SPECANA_UNCHANGED;
  memcpy(last_buf, sample, len);

  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );

  // check whether to (re)init FFT and window function
  if (plan == NULL || numsamples != last_numsamples)
  { last_numsamples = numsamples;
    last_winfn = winfn;
    if (!init_fft() || !init_window())
      return SPECANA_ERROR;
  } else if (wnd == NULL || last_winfn != winfn)
  { last_winfn = winfn;
    if (!init_window())
      return SPECANA_ERROR;
  } 

  // demultiplex samples
  // Oh, well, C++ with templates would be nice...
  switch ((bufferinfo.bits > 8) + (bufferinfo.bits > 16))
  { int e;
   case 0:
    fetch_byte((unsigned char*)sample, bufferinfo.channels);
    scale = 1./128; // normalize
    break;

   case 1:
    fetch_short((short*)sample, bufferinfo.channels);
    scale = 1./32768; // normalize
    break;
    
   case 2:
    fetch_long((long*)sample, bufferinfo.channels);
    // I am unsure how the 0dB level of >16Bit data is usually defined.
    scale = 1./0x80000000U; // normalize
  }
  #ifdef DEBUG
  fprintf(stderr, "SA: demux done %g\n", scale);
  #endif

  // To reduce spectral leakage, the samples are multipled with a window.
  { float* wp = wnd;
    float* dp2 = in + (numsamples >> 1); 
    float* dp = dp2 -1;
    for (i = numsamples >> 3; i; --i)
    { DO_4(p, dp[-p] *= wp[p]; dp2[p] *= wp[p]);
      dp -= 4;
      dp2 += 4;
      wp += 4;
    }
  }
  #ifdef DEBUG
  fprintf(stderr, "SA: win done\n");
  #endif

  // transform into the time domain (in -> out)
  fftwf_execute(plan);
  #ifdef DEBUG
  fprintf(stderr, "SA: FFT done\n");
  #endif
  // fetch the norm of the coefficients
  // Additionaly compensate for the channel addition and the unnormalized FFT above.
  { fftwf_complex* sp = out;
    scale /= bufferinfo.channels * sqrt(numsamples);
    for (i = numsamples >> 3; i; --i)
    { DO_4(p, bands[p] = sqrt(sp[p][0]*sp[p][0] + sp[p][1]*sp[p][1]) * scale);
      bands += 4;
      sp += 4;
    }
    bands[0] = sp[0][0] * scale; // nyquist frequency
  }
  #ifdef DEBUG
  fprintf(stderr, "SA: bands stored\n");
  #endif
  
  return SPECANA_OK;
}

void specana_uninit(void)
{ last_numsamples = 0;
  last_bps = 0;
  free(last_buf);
  last_buf = NULL;
  free(wnd);
  wnd = NULL;
  fftwf_free(in);
  in = NULL;
  fftwf_free(out);
  out = NULL;
  if (plan)
  { fftwf_destroy_plan(plan);
    plan = NULL;
  }
}

