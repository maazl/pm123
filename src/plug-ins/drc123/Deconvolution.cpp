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
#include "FFT2Data.h"

#include <strutils.h>
#include <math.h>
#include <minmax.h>


Deconvolution::ParameterSet Deconvolution::ParameterSet::Default;

Deconvolution::ParameterSet::ParameterSet()
: Iref_count(1)
{ Enabled = false;
  FIROrder = 49152;
  PlanSize = 32736;
  TargetFile = xstring::empty;
  WindowFunction = WFN_NONE;
}

Deconvolution::ParameterSet::ParameterSet(const Parameters& r)
: Parameters(r)
{ if (!TargetFile.length())
    Enabled = false;
  else if (!Target.Load(TargetFile))
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, xstring().sprintf("Failed to load target file %s.\n%s", TargetFile.cdata(), strerror(errno)));
    Enabled = false;
  }
}

volatile int_ptr<Deconvolution::ParameterSet> Deconvolution::ParamSet(&ParameterSet::Default);
event<const Deconvolution::KernelChangeEventArgs> Deconvolution::KernelChange;
volatile unsigned Deconvolution::KernelUpdateRequest = false;


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
, LocalParamSet(ParamSet)
, NeedInit(true)
, LastEnabled(false)
, InboxLevel(0)
, Discard(false)
, Inbox(NULL)
, ForwardPlan(NULL)
, BackwardPlan(NULL)
{ Format.samplerate = 0;
  Format.channels = 0;
  Overlap[0] = Overlap[1] = NULL;
}

Deconvolution::~Deconvolution()
{ FFTDestroy();
}

void Deconvolution::LoadOverlap(float* overlap_buffer)
{ memcpy(TimeDomain.begin() + CurPlanSize - CurFIROrder2, overlap_buffer, CurFIROrder2 * sizeof *TimeDomain.begin());
  memcpy(TimeDomain.begin(), overlap_buffer + CurFIROrder2, CurFIROrder2 * sizeof *TimeDomain.begin());
}

void Deconvolution::SaveOverlap(float* overlap_buffer, int len)
{ DEBUGLOG(("Deconvolution::SaveOverlap(%p, %i) - %i\n", overlap_buffer, len, CurFIROrder));
  if (len < CurFIROrder2)
  { memmove(overlap_buffer, overlap_buffer + len, (CurFIROrder - len) * sizeof *overlap_buffer);
    memcpy(overlap_buffer + CurFIROrder - len, TimeDomain.begin() + CurFIROrder2, len * sizeof *overlap_buffer);
  } else
    memcpy(overlap_buffer, TimeDomain.begin() + len - CurFIROrder2, CurFIROrder * sizeof *overlap_buffer);
}

void Deconvolution::LoadSamplesMono(const float* sp, const int len, float* overlap_buffer)
{ DEBUGLOG(("Deconvolution::LoadSamplesMono(%p, %i, %p) - %i\n", sp, len, overlap_buffer));
  LoadOverlap(overlap_buffer);
  // fetch new samples
  float* dp = TimeDomain.begin() + CurFIROrder2;
  memcpy(dp, sp, len * sizeof *dp);
  memset(dp + len, 0, (CurPlanSize - CurFIROrder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  memset(dp, 0, (CurPlanSize - CurFIROrder - len) * sizeof *dp); // padding not required, we simply ignore part of the result.
  SaveOverlap(overlap_buffer, len);
}

void Deconvolution::LoadSamplesStereo(const float* sp, const int len, float* overlap_buffer)
{ int l;
  float* dp = TimeDomain.begin() + CurFIROrder2;
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
  float* dp = TimeDomain.begin();
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
void Deconvolution::Convolve(fftwf_complex* sp, fftwf_complex* kp)
{ fftwf_complex* dp = FreqDomain.begin();
  DEBUGLOG(("Deconvolution::Convolve(%p, %p) - %p\n", sp, kp, dp));
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

    // TODO: implicitly demultiplex the stereo samples by using the guru interface.

    if (Format.channels == 2)
    { // left channel
      LoadSamplesStereo(buf, l2, Overlap[0]);
      // do FFT
      fftwf_execute(ForwardPlan);
      // convolution
      Convolve(FreqDomain.begin(), Kernel[0].begin());
      // convert back
      StoreSamplesStereo(newsamples, l2);
      // right channel
      LoadSamplesStereo(buf+1, l2, Overlap[1]);
      // do FFT
      fftwf_execute(ForwardPlan);
      // convolution
      Convolve(FreqDomain.begin(), Kernel[1].begin());
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
      memcpy(ChannelSave, FreqDomain.begin(), CurPlanSize21 * sizeof *ChannelSave);
      // convolution
      Convolve(FreqDomain.begin(), Kernel[0].begin());
      // convert back
      StoreSamplesStereo(newsamples, l2);
      // right channel
      // convolution
      Convolve(ChannelSave, Kernel[1].begin());
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
  fftwf_free(FreqDomain.detach()); // Hack! We use a custom allocator and don't tell FreqDomain about.
  fftwf_free(TimeDomain.detach()); // Hack! We use a custom allocator and don't tell TimeDomain about.
  // do not free aliased arrays!
  Kernel[1].detach();
  delete[] Overlap[0];
  fftwf_free(Kernel[0].detach()); // Hack! We use a custom allocator and don't tell Kernel about.
}

bool Deconvolution::Setup()
{ DEBUGLOG(("Deconvolution(%p)::Setup() - %u, %u, %u\n", NeedInit, NeedFIR, NeedKernel));

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
  frexp(LocalParamSet->PlanSize-1, &i); // floor(log2(plansize-1))+1
  CurPlanSize = 2 << i; // 2**(i+1)
  // reduce size at low sampling rates
  frexp(Format.samplerate/8000, &i); // floor(log2(samprate))+1, i >= 0
  CurPlanSize >>= 4-min(4,i); // / 2**(4-i)
  DEBUGLOG(("I: %d\n", CurPlanSize));
  CurPlanSize21 = CurPlanSize / 2 + 1;

  // allocate buffers
  { Inbox       = new float[CurPlanSize << 1];
    FreqDomain.assign((fftwf_complex*)fftwf_malloc(CurPlanSize21 * sizeof *FreqDomain.begin()), CurPlanSize21);
    TimeDomain.assign((float*)fftwf_malloc((CurPlanSize+1) * sizeof *TimeDomain.begin()), CurPlanSize);
    Overlap[0]  = new float[CurPlanSize << 1];
    Overlap[1]  = Overlap[0] + CurPlanSize;
    ChannelSave = (fftwf_complex*)Overlap[1] -1; // Aliasing!
    Kernel[0].assign((fftwf_complex*)fftwf_malloc(CurPlanSize21 << 1 * sizeof *Kernel[0].begin()), CurPlanSize21);
    Kernel[1].assign(Kernel[0].begin() + CurPlanSize21, CurPlanSize21);
  }

  // prepare real 2 complex transformations
  ForwardPlan  = fftwf_plan_dft_r2c_1d(CurPlanSize, TimeDomain.begin(), FreqDomain.begin(), FFTW_ESTIMATE);
  BackwardPlan = fftwf_plan_dft_c2r_1d(CurPlanSize, FreqDomain.begin(), TimeDomain.begin(), FFTW_ESTIMATE);

  TrashBuffers();
  NeedInit = false;

 doFIR:
  // STEP 2: setup FIR order

  CurFIROrder = (LocalParamSet->FIROrder + 15) & -16; /* multiple of 16 */
  CurFIROrder <<= 1; // * 2
  frexp(Format.samplerate/8000, &i); // floor(log2(samprate))+1, i >= 0
  CurFIROrder >>= 4-min(4,i); // / 2**(4-i)
  CurFIROrder2 = CurFIROrder / 2;

  if (CurFIROrder < 2 || CurFIROrder >= CurPlanSize)
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, "very bad! The FIR order and/or the FFT plan size is invalid or the FIRorder is higher or equal to the plan size.");
    LocalParamSet->Enabled = false; // avoid crash
    return false;
  }

  DEBUGLOG(("P: FIRorder: %d, Plansize: %d\n", CurFIROrder, CurPlanSize));
  NeedFIR = FALSE;

 doEQ:
  // STEP 3: setup filter kernel
  KernelChangeEventArgs args;
  args.Params = LocalParamSet;
  args.Samplerate = Format.samplerate;
  args.FreqScale = CurPlanSize;
  args.TimeDomain = &TimeDomain;
  args.TimeScale = CurPlanSize * sqrt(CurPlanSize);

  Data2FFT d2f(LocalParamSet->Target, CurPlanSize, (double)args.Samplerate/CurPlanSize);
  // for left, right
  for (args.Channel = 0; args.Channel < 2; args.Channel++)
  { // get target response
    d2f.LoadFFT(args.Channel ? Generate::RGain : Generate::LGain, FreqDomain);
    FreqDomain[0] = 0.; // no DC

    // transform into the time domain
    fftwf_execute(BackwardPlan);
    #if defined(DEBUG_LOG) && DEBUG_LOG > 1
    for (i = 0; i <= CurPlansize/2; ++i)
      DEBUGLOG2(("TK: %i, %g\n", i, TimeDomain[i]));
    #endif

    // normalize, apply window function and store results symmetrically
    { double (*window)(double);
      switch (args.Params->WindowFunction)
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
      float* sp1 = TimeDomain.begin();
      float* sp2 = sp1 + CurPlanSize;
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
      memset(sp1, 0, (sp2-sp1) * sizeof *TimeDomain.begin());
    }

    // prepare for FFT convolution
    // transform back into the frequency domain, now with window function
    fftwf_execute_dft_r2c(ForwardPlan, TimeDomain.begin(), Kernel[args.Channel].begin());
    args.FreqDomain = &Kernel[args.Channel];
    KernelChange(args);
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

  // Check for parameter changes.
  { int_ptr<ParameterSet> params(ParamSet);
    if (params != LocalParamSet)
    { if (LocalParamSet->PlanSize != params->PlanSize)
        NeedInit = true;
      else if (LocalParamSet->FIROrder != params->FIROrder)
        NeedFIR = true;
      else if ( LocalParamSet->TargetFile != params->TargetFile
        || LocalParamSet->WindowFunction != params->WindowFunction )
        NeedKernel = true;
      // Update parameters
      LocalParamSet = params;
    }
    NeedKernel |= InterlockedXch(&KernelUpdateRequest, 0);
  }

  bool enabled = LocalParamSet->Enabled && buf != NULL && (format->channels == 1 || format->channels == 2);

  if (enabled && !LastEnabled)
  { // enable EQ
    LastEnabled = true;
    Latency = -1;
  } else if (!enabled && LastEnabled)
  { // disable EQ
    LastEnabled = false;
    LocalFlush();
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
  // Set output always to stereo
  OUTPUT_PARAMS2 vParams;
  INFO_BUNDLE_CV vInfo;
  TECH_INFO      vTechInfo;
  if (info->Info && info->Info->tech->channels == 1 && int_ptr<ParameterSet>(ParamSet)->Enabled)
  { vParams = *info;
    vInfo = *vParams.Info;
    vTechInfo = *(const TECH_INFO*)vInfo.tech;
    vTechInfo.channels = 2;
    vInfo.tech = &vTechInfo;
    vParams.Info = &vInfo;
    info = &vParams;
  }

  switch (msg)
  {case OUTPUT_TRASH_BUFFERS:
    TempPos = info->PlayingPos;
    Discard = true;
    break;
   case OUTPUT_CLOSE:
    TempPos = PosMarker;
    Discard = true;
    break;
  }

  return (*OutCommand)(OutA, msg, info);
}

