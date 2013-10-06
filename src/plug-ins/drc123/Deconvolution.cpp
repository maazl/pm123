/*
 * Copyright 2013-2013 Marcel Mueller
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

#include "Deconvolution.h"

#include <strutils.h>
#include <math.h>
#include <minmax.h>


Deconvolution::ParameterSet Deconvolution::ParameterSet::Default;

Deconvolution::ParameterSet::ParameterSet()
: Iref_count(1)
{ Enabled = false;
  FIROrder = 49152;
  PlanSize = 32736;
  //FilterFile
  WindowFunction = WFN_NONE;
  //FilterKernel
}

volatile int_ptr<Deconvolution::ParameterSet> Deconvolution::ParamSet(&ParameterSet::Default);


double Deconvolution::NoWindow(double)
{ return 1.;
}

double Deconvolution::HammingWindow(double pos)
{ return 0.54 - 0.46 * cos(2 * M_PI * pos);
}

double Deconvolution::DimmedHammingWindow(double pos)
{ return 0.77 - 0.23 * cos(2 * M_PI * pos);
}


Deconvolution::Deconvolution(FILTER_PARAMS2& params)
: Filter(params)
, NeedInit(true)
, LastEnabled(false)
, InboxLevel(0)
, Discard(true)
, Inbox(NULL)
, TimeDomain(NULL)
, FreqDomain(NULL)
, ForwardPlan(NULL)
, BackwardPlan(NULL)
{ Format.samplerate = 0;
  Format.channels = 0;
  Overlap[0] = Overlap[1] = NULL;
  Kernel[0] = Kernel[1] = NULL;
}

Deconvolution::~Deconvolution()
{ FFTDestroy();
}

void Deconvolution::LoadOverlap(float* overlap_buffer)
{ memcpy(TimeDomain + CurPlanSize - CurFIROrder2, overlap_buffer, CurFIROrder2 * sizeof *TimeDomain);
  memcpy(TimeDomain, overlap_buffer + CurFIROrder2, CurFIROrder2 * sizeof *TimeDomain);
}

void Deconvolution::SaveOverlap(float* overlap_buffer, int len)
{ DEBUGLOG(("Deconvolution::SaveOverlap(%p, %i) - %i\n", overlap_buffer, len, CurFIROrder));
  if (len < CurFIROrder2)
  { memmove(overlap_buffer, overlap_buffer + len, (CurFIROrder - len) * sizeof *overlap_buffer);
    memcpy(overlap_buffer + CurFIROrder - len, TimeDomain + CurFIROrder2, len * sizeof *overlap_buffer);
  } else
    memcpy(overlap_buffer, TimeDomain + len - CurFIROrder2, CurFIROrder * sizeof *overlap_buffer);
}

void Deconvolution::LoadSamplesMono(const float* sp, const int len, float* overlap_buffer)
{ DEBUGLOG(("Deconvolution::LoadSamplesMono(%p, %i, %p) - %i\n", sp, len, overlap_buffer));
  LoadOverlap(overlap_buffer);
  // fetch new samples
  float* dp = TimeDomain + CurFIROrder2;
  memcpy(dp, sp, len * sizeof *dp);
  memset(dp + len, 0, (CurPlanSize - CurFIROrder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  memset(dp, 0, (CurPlanSize - CurFIROrder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  SaveOverlap(overlap_buffer, len);
}

void Deconvolution::LoadSamplesStereo(const float* sp, const int len, float* overlap_buffer)
{ int l;
  float* dp = TimeDomain + CurFIROrder2;
  LoadOverlap(overlap_buffer);
  for (l = len >> 3; l; --l) // coarse
  { DO_8(p, dp[p] = sp[p<<1]);
    sp += 16;
    dp += 8;
  }
  for (l = len & 7; l; --l) // fine
  { *dp++ = *sp;
    sp += 2;
  }
  memset(dp, 0, (CurPlanSize - CurFIROrder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  SaveOverlap(overlap_buffer, len);
}

void Deconvolution::StoreSamplesStereo(float* sp, const int len)
{ DEBUGLOG(("Deconvolution::StoreSamplesStereo(%p, %i)\n", sp, len));
  float* dp = TimeDomain;
  int l;
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

/* do convolution and back transformation
 */
void Deconvolution::Convolute(fftwf_complex* sp, fftwf_complex* kp)
{ fftwf_complex* dp = FreqDomain;
  DEBUGLOG(("Deconvolution::Convolute(%p, %p) - %p\n", sp, kp, dp));
  int l;
  // Convolution in the frequency domain is simply a series of complex products.
  for (l = CurPlanSize21 >> 3; l; --l)
  { DO_8(p,
      dp[p] = sp[p] * kp[p];
    );
    sp += 8;
    kp += 8;
    dp += 8;
  }
  for (l = CurPlanSize21 & 7; l; --l)
  { *dp = *sp * *kp;
    ++sp;
    ++kp;
    ++dp;
  }
  // do IFFT
  fftwf_execute(BackwardPlan);
}

void Deconvolution::FilterSamplesFFT(float* newsamples, const float* buf, int len)
{ DEBUGLOG(("Deconvolution(%p)::FilterSamplesFFT(%p, %p, %i)\n", this, newsamples, buf, len));

  while (len) // we might need more than one FFT cycle
  { int l2 = CurPlanSize - CurFIROrder;
    if (l2 > len)
      l2 = len;

    if (Format.channels == 2)
    { // left channel
      LoadSamplesStereo(buf, l2, Overlap[0]);
      // do FFT
      fftwf_execute(ForwardPlan);
      // convolution
      Convolute(FreqDomain, Kernel[0]);
      // convert back
      StoreSamplesStereo(newsamples, l2);
      // right channel
      LoadSamplesStereo(buf+1, l2, Overlap[1]);
      // do FFT
      fftwf_execute(ForwardPlan);
      // convolution
      Convolute(FreqDomain, Kernel[1]);
      // convert back
      StoreSamplesStereo(newsamples+1, l2);
      // next block
      len -= l2;
      l2 <<= 1; // stereo
      buf += l2;
      newsamples += l2;
    } else
    { // left channel
      LoadSamplesMono(buf, l2, Overlap[0]);
      // do FFT
      fftwf_execute(ForwardPlan);
      // save data for 2nd channel
      memcpy(ChannelSave, FreqDomain, CurPlanSize21 * sizeof *FreqDomain);
      // convolution
      Convolute(FreqDomain, Kernel[0]);
      // convert back
      StoreSamplesStereo(newsamples, l2);
      // right channel
      // convolution
      Convolute(ChannelSave, Kernel[1]);
      // convert back
      StoreSamplesStereo(newsamples+1, l2);
      // next block
      len -= l2;
      buf += l2;
      l2 <<= 1; // stereo
      newsamples += l2;
    }
  }
}

void Deconvolution::FilterSamplesNewOverlap(const float* buf, int len)
{ DEBUGLOG(("Deconvolution(%p)::FilterSamplesNewOverlap(%p, %i)\n", this, buf, len));

  int l2 = CurPlanSize - CurFIROrder;
  if (len > l2)
  { // skip unneeded samples
    buf += (len - l2) * Format.channels;
    len = l2;
  }

  // Well, there might be a more optimized version without copying the old overlap buffer, but who cares
  if (Format.channels == 2)
  { LoadSamplesStereo(buf  , l2, Overlap[0]);
    LoadSamplesStereo(buf+1, l2, Overlap[1]);
  } else
  { LoadSamplesMono(buf, l2, Overlap[0]);
  }
}

int Deconvolution::DoRequestBuffer(float** buf)
{ DEBUGLOG(("Deconvolution(%p)::DoRequestBuffer(%p) - %d\n", this, buf, Latency));
  if (Latency != 0)
  { if (Latency < 0)
      Latency = CurFIROrder2;
    *buf = NULL; // discard
    return Latency;
  } else
  { FORMAT_INFO2 fi = Format;
    fi.channels = 2; // result is always stereo
    return (*OutRequestBuffer)(OutA, &fi, buf);
  }
}

void Deconvolution::DoCommitBuffer(int len, PM123_TIME pos)
{ DEBUGLOG(("Deconvolution(%p)::DoCommitBuffer(%d, %f) - %d\n", this, len, pos, Latency));
  if (Latency != 0)
    Latency -= len;
  else
    (*OutCommitBuffer)(OutA, len, pos);
}

void Deconvolution::FilterAndSend()
{ DEBUGLOG(("Deconvolution(%p)::FilterAndSend() - %d\n", this, InboxLevel));

  int    len, dlen;
  float* dbuf;

  // request destination buffer
  dlen = DoRequestBuffer(&dbuf);

  if (Discard)
  { InboxLevel = 0;
    DoCommitBuffer(0, TempPos); // no-op
    return;
  }

  len = InboxLevel; // required destination length
  if (dlen < len)
  { // with fragmentation
    int rem = len;
    float* sp;

    FilterSamplesFFT(Inbox, Inbox, InboxLevel);

    // transfer data
    sp = Inbox;
    for(;;)
    { if (dbuf != NULL)
        memcpy(dbuf, sp, dlen * sizeof(float) * 2);
      DoCommitBuffer(dlen, PosMarker + (double)(len-rem)/Format.samplerate);
      rem -= dlen;
      if (rem == 0)
        break;
      sp += dlen * 2;
      // request next buffer
      dlen = DoRequestBuffer(&dbuf);
      if (Discard)
      { InboxLevel = 0;
        DoCommitBuffer(0, TempPos); // no-op
        return;
      }
      if (dlen > rem)
        dlen = rem;
    }

  } else
  { // without fragmentation
    if (dbuf == NULL)
      // only save overlap
      FilterSamplesNewOverlap(Inbox, InboxLevel);
    else
      FilterSamplesFFT(dbuf, Inbox, InboxLevel);
    DoCommitBuffer(len, PosMarker);
  }

  PosMarker += (double)InboxLevel/Format.samplerate;
  InboxLevel = 0;
}

void Deconvolution::TrashBuffers()
{ DEBUGLOG(("Deconvolution(%p)::TrashBuffers() - %d %d\n", this, InboxLevel, Latency));
  InboxLevel = 0;
  Latency    = -1;
  memset(Overlap[0], 0, CurPlanSize * 2 * sizeof(float)); // initially zero
}

void Deconvolution::LocalFlush()
{ // emulate input of some zeros to compensate for filter size
  int   len = (CurFIROrder+1) >> 1;
  DEBUGLOG(("Deconvolution(%p)::LocalFlush() - %d %d %d\n", this, len, InboxLevel, Latency));
  while (len != 0)
  { int dlen = CurPlanSize - CurFIROrder - InboxLevel;
    if (dlen > len)
      dlen = len;
    memset(Inbox + InboxLevel * Format.channels, 0, dlen * Format.channels * sizeof(float));
    // commit buffer
    InboxLevel += dlen;
    if (InboxLevel == CurPlanSize - CurFIROrder)
    { // enough data, apply filter
      FilterAndSend();
      if (Discard)
        break;
    }
    len -= dlen;
  }

  // flush buffer
  if (InboxLevel != 0)
    FilterAndSend();

  // setup for restart
  TrashBuffers();
}

void Deconvolution::FFTDestroy()
{ DEBUGLOG(("Deconvolution(%p)::FFTDestroy()\n", this));
  delete[] Inbox;
  fftwf_destroy_plan(ForwardPlan);
  fftwf_destroy_plan(BackwardPlan);
  //fftwf_destroy_plan(FFT.RDCT_plan);
  fftwf_free(FreqDomain);
  fftwf_free(TimeDomain);
  // do not free aliased arrays!
  delete[] Overlap[0];
  delete[] Kernel[0];
}

bool Deconvolution::Setup()
{ DEBUGLOG(("Deconvolution(%p)::Setup() - %u, %u, %u\n", NeedInit, NeedFIR, NeedKernel));

  // Check for parameter changes.
  int_ptr<ParameterSet> params(ParamSet);
  Enabled = params->Enabled;

  if (LastPlanSize != params->PlanSize)
  { LastPlanSize = params->PlanSize;
    NeedInit = true;
  } else if (LastFIROrder != params->FIROrder)
  { LastFIROrder = params->FIROrder;
    NeedFIR = true;
  }

  // dispatcher to skip code fragments
  if (!NeedInit)
  { if (NeedFIR)
      goto doFIR;
    if (NeedKernel)
      goto doEQ;
    return true;
  }

  // STEP 1: initialize buffers and FFT plans

  // free old resources
  FFTDestroy();

  // round up to next power of 2
  int i;
  frexp(params->PlanSize-1, &i); // floor(log2(plansize-1))+1
  CurPlanSize = 2 << i; // 2**(i+1)
  // reduce size at low sampling rates
  frexp(Format.samplerate/8000, &i); // floor(log2(samprate))+1, i >= 0
  CurPlanSize >>= 4-min(4,i); // / 2**(4-i)
  DEBUGLOG(("I: %d\n", CurPlanSize));
  CurPlanSize21 = CurPlanSize / 2 + 1;

  // allocate buffers
  { Inbox       = new float[CurPlanSize << 1];
    FreqDomain  = (fftwf_complex*)fftwf_malloc(CurPlanSize21 * sizeof *FreqDomain);
    TimeDomain  = (float*)fftwf_malloc((CurPlanSize+1) * sizeof *TimeDomain);
    Overlap[0]  = new float[CurPlanSize << 1];
    Overlap[1]  = Overlap[0] + CurPlanSize;
    ChannelSave = (fftwf_complex*)Overlap[1] -1; // Aliasing!
    Kernel[0]   = new fftwf_complex[CurPlanSize21 << 1];
    Kernel[1]   = Kernel[0] + CurPlanSize21;
  }

  // prepare real 2 complex transformations
  ForwardPlan  = fftwf_plan_dft_r2c_1d(CurPlanSize, TimeDomain, FreqDomain, FFTW_ESTIMATE);
  BackwardPlan = fftwf_plan_dft_c2r_1d(CurPlanSize, FreqDomain, TimeDomain, FFTW_ESTIMATE);

  TrashBuffers();
  NeedInit = false;

 doFIR:
  // STEP 2: setup FIR order

  // copy global parameters for thread safety
  CurFIROrder = (params->FIROrder + 15) & -16; /* multiple of 16 */
  CurFIROrder <<= 1; // * 2
  frexp(Format.samplerate/8000, &i); // floor(log2(samprate))+1, i >= 0
  CurFIROrder >>= 4-min(4,i); // / 2**(4-i)
  CurFIROrder2 = CurFIROrder / 2;

  if (CurFIROrder < 2 || CurFIROrder >= CurPlanSize)
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, "very bad! The FIR order and/or the FFT plansize is invalid or the FIRorder is higer or equal to the plansize.");
    Enabled = false; // avoid crash
    return false;
  }

  /*// calculate optimum plansize for kernel generation
  frexp((FFT.FIRorder<<1) -1, &FFT.DCTplansize); // floor(log2(2*FIRorder-1))+1
  FFT.DCTplansize = 1 << FFT.DCTplansize; // 2**x
  // free old resources
  if (FFTinit)
    fftwf_destroy_plan(FFT.DCT_plan);
  // prepare real 2 real transformations
  FFT.DCT_plan = fftwf_plan_r2r_1d(FFT.DCTplansize/2+1, FFT.design, FFT.time_domain, FFTW_REDFT00, FFTW_ESTIMATE);*/

  DEBUGLOG(("P: FIRorder: %d, Plansize: %d\n", CurFIROrder, CurPlanSize));
  NeedFIR = FALSE;

 doEQ:
  // STEP 3: setup filter kernel

  float fftspecres = (float)Format.samplerate / CurPlanSize;
  int_ptr<TargetResponse> target; // todo

  /*// Prepare design coefficients frame
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
  coef[NUM_BANDS+3].de = 0;*/

  /* for left, right */
  for (int channel = 0; channel < 2; channel++)
  {
/*    float log_mute_level;

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
    }*/

    // compose frequency spectrum
    { Coeff* cop = target->Channels[channel].get();
      double phase = 0;
      FreqDomain[0] = 0.; // no DC
      for (i = 1; i < CurPlanSize21; ++i) // do not start at f=0 to avoid log(0)
      { const float f = i * fftspecres; // current frequency
        double pos;
        double val = log(f);
        while (val > cop[1].LF)
          ++cop;
        // integrate phase for group delay
        pos = (log(f)-cop[0].LF) / (cop[1].LF-cop[0].LF);
        phase += (cop[0].GD + pos * (cop[1].GD - cop[0].GD)) * fftspecres * (2*M_PI);
        // do double logarithmic sine^2 interpolation for amplitude
        pos = .5 - .5*cos(M_PI * pos);
        val = exp(cop[0].LV + pos * (cop[1].LV - cop[0].LV));
        // convert to cartesian coordinates
        FreqDomain[i] = std::polar(val, phase);
        DEBUGLOG2(("F: %i, %f, %f -> %f = %f dB, %f = %f ms@ %f\n",
          i, f, pos, val, TodB(val), phase*180./M_PI, cop[0].GD + pos * (cop[1].GD - cop[0].GD), exp(cop[0].LF)));
    } }

    // transform into the time domain
    fftwf_execute(BackwardPlan);
    #if defined(DEBUG_LOG) && DEBUG_LOG > 1
    for (i = 0; i <= CurPlansize/2; ++i)
      DEBUGLOG2(("TK: %i, %g\n", i, TimeDomain[i]));
    #endif

    // normalize, apply window function and store results symmetrically
    { // sample for thread safety.
      double (*window)(double);
      switch (params->WindowFunction)
      {default: // WFN_NONE
        window = &Deconvolution::NoWindow;
        break;
       case WFN_DIMMEDHAMMING:
        window = &Deconvolution::DimmedHammingWindow;
        break;
       case WFN_HAMMING:
        window = &Deconvolution::HammingWindow;
        break;
      }
      double plansize2 = (double)CurPlanSize * CurPlanSize;
      float* sp1 = TimeDomain;
      float* sp2 = TimeDomain + CurPlanSize;
      double tmp;
      *sp1++ /= plansize2;
      DEBUGLOG2(("K: %i, %g\n", CurFIROrder2, sp1[-1]));
      for (i = CurFIROrder2 -1; i >= 0; --i)
      { tmp = (*window)((double)i/CurFIROrder) / plansize2; // normalize for next FFT
        *sp1++ *= tmp;
        *--sp2 *= tmp;
        DEBUGLOG2(("K: %i, %g, %g\n", i, sp1[-1], sp2[0]));
      }
      // padding
      memset(sp1, 0, (sp2-sp1) * sizeof *TimeDomain);
    }

    // prepare for FFT convolution
    // transform back into the frequency domain, now with window function
    fftwf_execute_dft_r2c(ForwardPlan, TimeDomain, Kernel[channel]);
    #if defined(DEBUG_LOG) && DEBUG_LOG > 1
    for (i = 0; i <= CurPlansize/2; ++i)
      DEBUGLOG2(("FK: %i, %g, %g\n", i, Kernel[channel][i][0], Kernel[channel][i][1]));
    DEBUGLOG2(("E: kernel completed.\n"));
    #endif
  }

  NeedKernel = FALSE;
  return true;
}

int Deconvolution::InRequestBuffer(const FORMAT_INFO2* format, float** buf)
{
  #ifdef DEBUG_LOG
  if (format != NULL)
    DEBUGLOG(("Deconvolution(%p)::InRequestBuffer({%u, %u}, %p) - %d %d\n",
     this, format->samplerate, format->channels, buf, InboxLevel, Latency));
   else
    DEBUGLOG(("Deconvolution(%p)::InRequestBuffer(%p, %p) - %d %d\n", this, format, buf, InboxLevel, Latency));
  #endif

  bool enabled = Enabled && buf != NULL && (format->channels == 1 || format->channels == 2);

  if (enabled && !LastEnabled)
  { // enable EQ
    LastEnabled = true;
    Latency = -1;
  } else if (!enabled && LastEnabled)
  { // disable EQ
    LocalFlush();
    Enabled = false;
    if (buf == NULL) // global flush();
      return (*OutRequestBuffer)(OutA, format, buf);
  }

  if (LastEnabled)
  {
    if (Discard)
    { TrashBuffers();
      Discard = false;
    }
    if (Format.samplerate != format->samplerate || Format.channels != format->channels)
    { if (Format.samplerate != 0)
        LocalFlush();
      Format = *format;
      NeedInit = true;
    } else if (NeedFIR || NeedInit)
    { if (Format.samplerate != 0)
        LocalFlush();
    }

    Setup();

    *buf = Inbox + InboxLevel * format->channels;
    DEBUGLOG(("Deconvolution::InRequestBuffer: %p, %d\n", *buf, CurPlanSize - CurFIROrder - InboxLevel));
    return CurPlanSize - CurFIROrder - InboxLevel;
  }
  else
  {
    return (*OutRequestBuffer)(OutA, format, buf);
  }
}

void Deconvolution::InCommitBuffer(int len, PM123_TIME pos)
{ DEBUGLOG(("Deconvolution(%p)::InCommitBuffer(%u, %f) - %d %d\n", this, len, pos, InboxLevel, Latency));

  if (!LastEnabled)
  { (*OutCommitBuffer)(OutA, len, pos);
    return;
  }

  if (InboxLevel == 0)
    // remember position and precompensate for filter delay
    PosMarker = pos - (double)CurFIROrder2/Format.samplerate;

  InboxLevel += len;

  if (InboxLevel == CurPlanSize - CurFIROrder)
  { // enough data, apply filter
    FilterAndSend();
  }
}

ULONG Deconvolution::InCommand(ULONG msg, const OUTPUT_PARAMS2* info)
{
  // TODO:
  return PLUGIN_ERROR;
}

