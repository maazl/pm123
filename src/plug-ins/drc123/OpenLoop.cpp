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

#define INCL_BASE
#include "OpenLoop.h"
#include <os2.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>


bool OpenLoop::OpenLoopFile::ParseHeaderField(const char* string)
{ const char* value;
  if (!!(value = TryParam(string, "FFTSize")))
    FFTSize = atoi(value);
  else if (!!(value = TryParam(string, "DiscardSamp")))
    DiscardSamp = atoi(value);
  else if (!!(value = TryParam(string, "RefFreq")))
    sscanf(value, "%lf,%lf", &RefFMin, &RefFMax);
  else if (!!(value = TryParam(string, "RefExponent")))
    RefExponent = atof(value);
  else if (!!(value = TryParam(string, "RefSkipEven")))
    RefSkipEven = !!atoi(value);
  else if (!!(value = TryParam(string, "RefSkipRand")))
    RefSkipRand = !!atoi(value);
  else if (!!(value = TryParam(string, "RefMode")))
    RefMode = (Mode)atoi(value);
  else if (!!(value = TryParam(string, "RefVolume")))
    RefVolume = atof(value);
  else if (!!(value = TryParam(string, "RefFreqFactor")))
    RefFreqFactor = atof(value);
  else if (!!(value = TryParam(string, "RefEnergyDist")))
    RefEnergyDist = !!atoi(value);
  else if (!!(value = TryParam(string, "AnaSwap")))
    AnaSwap = !!atoi(value);
  else if (!!(value = TryParam(string, "LineNotch")))
    sscanf(value, "%u,%lf", &LineNotchHarmonics, &LineNotchFreq);
  else if (!!(value = TryParam(string, "DispGain")))
    sscanf(value, "%lf,%lf", &GainLow, &GainHigh);
  else if (!!(value = TryParam(string, "DispDelay")))
    sscanf(value, "%lf,%lf", &DelayLow, &DelayHigh);
  else if (!!(value = TryParam(string, "DispVU")))
    sscanf(value, "%lf,%lf,%lf", &VULow, &VUYellow, &VURed);
  else if (!!(value = TryParam(string, "AverageDelay")))
    AverageDelay = atof(value);
  else if (!!(value = TryParam(string, "PhaseUnwrap")))
    sscanf(value, "%u,%u", &PhaseUnwrapCount, &IndeterminatePhase);
  else
    return DataFile::ParseHeaderField(string);
  return true;
}

bool OpenLoop::OpenLoopFile::WriteHeaderFields(FILE* f)
{ fprintf( f,
    "##FFTSize=%u\n"
    "##DiscardSamp=%u\n"
    "##AnaSwap=%u\n"
    "##LineNotch=%u,%f\n"
    "##RefFreq=%f,%f\n"
    "##RefExponent=%f\n"
    "##RefSkipEven=%u\n"
    "##RefSkipRand=%u\n"
    "##RefMode=%u\n"
    "##RefVolume=%f\n"
    "##RefFreqFactor=%f\n"
    "##RefEnergyDist=%u\n"
    "##DispGain=%f,%f\n"
    "##DispDelay=%f,%f\n"
    "##DispVU=%f,%f,%f\n"
    "##AverageDelay=%f\n"
    "##PhaseUnwrap=%u,%u\n"
    , FFTSize, DiscardSamp, AnaSwap, LineNotchHarmonics, LineNotchFreq
    , RefFMin, RefFMax, RefExponent, RefSkipEven, RefSkipRand, RefMode, RefVolume, RefFreqFactor, RefEnergyDist
    , GainLow, GainHigh, DelayLow, DelayHigh, VULow, VUYellow, VURed
    , AverageDelay, PhaseUnwrapCount, IndeterminatePhase );
  return DataFile::WriteHeaderFields(f);
}

bool OpenLoop::OpenLoopFile::Load(const char* filename, bool nodata)
{ AverageDelay = 0.;
  PhaseUnwrapCount = 0;
  IndeterminatePhase = 0;
  return DataFile::Load(filename, nodata);
}


void OpenLoop::Statistics::Add(const float* data, unsigned inc, unsigned count)
{ Statistics temp;
  temp.Count = count;
  const float* datae = data + inc * count;
  while (data != datae)
  { double d2 = *data * *data;
    temp.Sum2 += d2;
    if (d2 > temp.Peak)
      temp.Peak = d2;
    data += inc;
  }
  temp.Peak = sqrt(temp.Peak);
  Add(temp);
}
void OpenLoop::Statistics::Add(const Statistics& r)
{ Sum2 += r.Sum2;
  Count += r.Count;
  if (r.Peak > Peak)
    Peak = r.Peak;
}


volatile xstring OpenLoop::RecURI(xstring::empty);

event<const int> OpenLoop::EvDataUpdate;

volatile bool OpenLoop::ClearRq;

OpenLoop::Statistics OpenLoop::Stat[2];
Mutex OpenLoop::StatMtx;


OpenLoop::OpenLoop(const Parameters& params, FILTER_PARAMS2& filterparams)
: Filter(filterparams)
, Params(params)
, RefTID(-1)
, InitComplete(false)
, Terminate(true)
, AnaTID(-1)
, LoopCount(0)
{
  OutEvent = filterparams.output_event;
  OutW     = filterparams.w;

  filterparams.output_event = &PROXYFUNCREF(OpenLoop)InEventProxy;

  const unsigned fdsize = Params.FFTSize / 2 + 1;
  AnaFFT[0].reset(fdsize);
  AnaFFT[1].reset(fdsize);

  // Create plan
  fftw_iodim iodim;
  iodim.n = Params.FFTSize;
  iodim.is = 2; // interleaved stereo
  iodim.os = 1;
  // We do not have the data array so far, so pass a dummy to keep fftw happy.
  // The dummy has alignment 4 because of the right channel transform.
  AnaPlan = fftwf_plan_guru_dft_r2c(1, &iodim, 0, NULL, (float*)4, AnaFFT[0].get(), FFTW_ESTIMATE|FFTW_DESTROY_INPUT);

  AnaTemp.reset(fdsize);
  ResCross.reset(params.FFTSize);
  // plan for cross correlation
  CrossPlan = fftwf_plan_dft_c2r_1d(params.FFTSize, AnaTemp.begin(), ResCross.begin(), FFTW_ESTIMATE|FFTW_DESTROY_INPUT);
}

OpenLoop::~OpenLoop()
{ fftwf_destroy_plan(CrossPlan);
  fftwf_destroy_plan(AnaPlan);
}


PROXYFUNCIMP(void DLLENTRY, OpenLoop)InEventProxy(Filter* w, OUTEVENTTYPE event)
{ ((OpenLoop*)w)->InEvent(event);
}

PROXYFUNCIMP(void TFNENTRY, OpenLoop)RefThreadStub(void* arg)
{ ((OpenLoop*)arg)->RefThreadFunc();
}

void OpenLoop::RefThreadFunc()
{
  // By default there are by far too many FP exceptions
  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );
  // Generate reference signal
  GenerateRef();
  InitComplete = true;
  if (IsLowWater)
    DosSetPriority(PRTYS_THREAD, PRTYC_FOREGROUNDSERVER, 20, RefTID);

  // Play reference signal
  size_t current = 0;
  long long playedsamples = 0;
  while (!Terminate)
  { float* buf;
    const int count = (*OutRequestBuffer)(OutA, (FORMAT_INFO2*)&VTechInfo, &buf);
    if (count <= 0)
    { (*OutEvent)(OutW, OUTEVENT_PLAY_ERROR);
      return;
    }
    unsigned remaining = count;
    while (remaining > Params.FFTSize - current)
    { unsigned samplecount = (Params.FFTSize - current) * VTechInfo.channels;
      memcpy(buf, RefData.begin() + current * VTechInfo.channels, samplecount * sizeof(float));
      remaining -= Params.FFTSize - current;
      buf += samplecount;
      current = 0;
    }
    memcpy(buf, RefData.begin() + current * VTechInfo.channels, remaining * VTechInfo.channels * sizeof(float));
    current += remaining;
    if (current == Params.FFTSize)
      current = 0;
    (*OutCommitBuffer)(OutA, count, (double)playedsamples / VTechInfo.samplerate);
    playedsamples += count;
  }
}

void OpenLoop::OverrideTechInfo(const OUTPUT_PARAMS2*& target)
{ if (target->Info)
  { VParams = *target;
    VInfo = *VParams.Info;
    VTechInfo = *(const TECH_INFO*)VInfo.tech;
    VTechInfo.channels = Params.RefMode & RFM_STEREO ? 2 : 1;
    VInfo.tech = &VTechInfo;
    VParams.Info = &VInfo;
    target = &VParams;
  }
}

inline static double myrand()
{ unsigned long l = (((rand() << 11) ^ rand()) << 11) ^ rand();
  return (double)l / ULONG_MAX;
}

void OpenLoop::GenerateRef()
{
  // initialize arrays
  const unsigned fdsize = Params.FFTSize / 2 + 1;
  RefDesign[0].reset(fdsize);
  RefDesign[0].Set0(); // all coefficients -> 0
  if (Params.RefMode & RFM_STEREO) // !MONO && ! DIFFERENTIAL
  { RefDesign[1].reset(fdsize);
    RefDesign[1].Set0(); // all coefficients -> 0
    RefData.reset(2 * Params.FFTSize);
  } else
    RefData.reset(Params.FFTSize);

  // round fmin & fmax
  const double f_bin = (double)VTechInfo.samplerate/Params.FFTSize;
  unsigned i_min = (unsigned)floor(Params.RefFMin/f_bin);
  unsigned i_max = (unsigned)ceil(Params.RefFMax/f_bin);
  if (i_max >= fdsize)
    i_max = fdsize - 1;

  // generate coefficients
  unsigned channel = Params.RefMode == RFM_RIGHT;
  double reflastf[2] = { 0., 0. }; // last frequency
  int skipcount = 0;
  double notchspacing = Params.LineNotchFreq / f_bin;
  for (size_t i = i_min; i <= i_max; ++i)
  { if ((i & 1) == 0 && Params.RefSkipEven)
      continue;
    double f = i * f_bin;
    if (Params.LineNotchHarmonics && Params.LineNotchFreq)
    { unsigned harmonic = (unsigned)(i / notchspacing + .5);
      if (harmonic <= Params.LineNotchHarmonics && fabs(i - harmonic * notchspacing) <= .5)
      { ++skipcount;
        continue;
    } }
    if (Params.RefSkipRand && Params.RefMode == RFM_STEREO && f + f * Params.RefFreqFactor >= f_bin && skipcount-- == 0)
    { skipcount = 7 + rand() & 15;
      continue;
    }
    double& lastf = reflastf[channel];
    // skip frequency because it is too close to the last one.
    if (f < lastf + lastf * Params.RefFreqFactor)
      continue;
    // calculate coefficients
    fftwf_complex& cur = RefDesign[channel][i];
    double mag = pow(f, Params.RefExponent);
    if (Params.RefEnergyDist)
      // TODO: distribute energy to left and right neighbor.
      mag *= sqrt(lastf ? f - lastf : f_bin * ((Params.RefMode == RFM_STEREO) + 1));
    // apply random phase
    cur = std::polar(mag, i && i != Params.FFTSize/2 ? 2*M_PI * myrand() : 0.);
    // next frequency
    lastf = f;
    channel ^= Params.RefMode == RFM_STEREO;
  }

  // iFFT
  { fftw_iodim iodim;
    iodim.n = Params.FFTSize;
    iodim.is = 1;
    iodim.os = 1 + (Params.RefMode != RFM_MONO); // interleaved stereo?
    fftwf_plan plan = fftwf_plan_guru_dft_c2r(1, &iodim, 0, NULL, RefDesign[0].begin(), RefData.begin()+1, FFTW_ESTIMATE|FFTW_PRESERVE_INPUT);
    fftwf_execute_dft_c2r(plan, RefDesign[0].begin(), RefData.begin());
    if (Params.RefMode == RFM_DIFFERENTIAL)
      foreach (float,*, sp, RefData)
      { sp[1] = -sp[0];
        ++sp;
      }
    else if (Params.RefMode & RFM_STEREO)
      fftwf_execute_dft_c2r(plan, RefDesign[1].begin(), RefData.begin() + 1);
    fftwf_destroy_plan(plan);
  }

  // normalize
  float fnorm = 0;
  foreach (float,*, sp, RefData)
    if (fabsf(*sp) > fnorm)
      fnorm = fabsf(*sp);
  fnorm = 1./fnorm;
  foreach (float,*, sp, RefData)
    *sp *= fnorm;
  // We need to scale RefDesign as well as it is used as reference for loopback measurements.
  // However, we have to compensate for the fftw normalization as well.
  fnorm *= sqrt(Params.FFTSize);
  foreach (fftwf_complex,*, dp, RefDesign[0])
    *dp *= fnorm;
  if (Params.RefMode & RFM_STEREO)
    foreach (fftwf_complex,*, dp, RefDesign[1])
      *dp *= fnorm;

  UncorrelatedFFTSize = Params.FFTSize >> Params.RefSkipEven >> (Params.RefMode == RFM_STEREO &&! Params.RefSkipRand);

  /*{ const TimeDomainData* td = &CrossCorrelate(RefDesign[0], RefDesign[0]);
    FILE* f = fopen("autocross.ch1","w");
    foreach(const float,*, sp, *td)
      fprintf(f, "%f\n", *sp);
    fclose(f);
    td = &CrossCorrelate(RefDesign[1], RefDesign[1]);
    f = fopen("autocross.ch2","w");
    foreach(const float,*, sp, *td)
      fprintf(f, "%f\n", *sp);
    fclose(f);
  }*/
}

PROXYFUNCIMP(void TFNENTRY, OpenLoop)AnaThreadStub(void* arg)
{ ((OpenLoop*)arg)->AnaThreadFunc();
}

void OpenLoop::AnaThreadFunc()
{ DosSetPriority(PRTYS_THREAD, PRTYC_IDLETIME, +20, 0);
  // By default there are by far too many FP exceptions
  _control87( EM_INVALID  | EM_DENORMAL  | EM_ZERODIVIDE |
              EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM );
  InitAnalyzer();
  for(;;)
  { // wait for work
    AnaEvent.Wait();
    AnaEvent.Reset();
    // Game over?
    if (Terminate)
      return;
    // process package
    int_ptr<SampleData> data(CurrentData);
    ProcessInput(*data);
  }
}

void OpenLoop::InitAnalyzer()
{}

TimeDomainData& OpenLoop::CrossCorrelate(const FreqDomainData& signal, const FreqDomainData& ref)
{ ASSERT(signal.size() == ref.size());
  { const fftwf_complex* sp1 = signal.begin();
    const fftwf_complex* sp2 = ref.begin();
    foreach(fftwf_complex,*, dp, AnaTemp)
    { *dp = *sp1++ * conj(*sp2);
      ++sp2;
    }
  }
  fftwf_execute(CrossPlan);
  return ResCross;
}

int OpenLoop::ComputeDelay(const FreqDomainData& signal, const FreqDomainData& ref, double& response)
{
  CrossCorrelate(signal, ref);

  // Find maximum
  double max = 0;   // maximum cross correlation ...
  float value;
  int pos = 0; // ... at here
  double ms = 0;
  foreach(const float,*, sp, ResCross)
  { double norm = *sp * *sp;
    ms += norm;
    if (norm > max)
    { max = norm;
      value = *sp;
      pos = sp - ResCross.begin();
    }
  }

  response = value * fabs(value) / ms;
  //pos %= UncorrelatedFFTSize;
  /*/ Map (T/2, T) to (-T/2, 0)
  if (pos > fd1.size() / 2)
    pos -= fd1.size();*/
  DEBUGLOG(("OpenLoop::ComputeDelay: Cross correlation max = %g@%u, ratio = %g, delay = %g\n",
    value, pos, max / ms, (double)pos / Format.samplerate));
  return pos;
}

int OpenLoop::AverageDelay(int delay1, int delay2)
{
  delay2 -= delay1;
  delay2 %= UncorrelatedFFTSize;
  // delay2 \elem [0,UncorrelatedFFTSize)
  if (delay2 > (int)(UncorrelatedFFTSize / 2))
    delay2 -= UncorrelatedFFTSize;
  delay1 += delay2 / 2;
  return delay1 % UncorrelatedFFTSize;
}

void OpenLoop::ProcessInput(SampleData& input)
{ DEBUGLOG(("OpenLoop(%p)::ProcessInput(.)\n", this));
  // Do FFT of left and right channel
  fftwf_execute_dft_r2c(AnaPlan, input.begin(), AnaFFT[Params.AnaSwap].begin());
  fftwf_execute_dft_r2c(AnaPlan, input.begin() + 1, AnaFFT[!Params.AnaSwap].begin());
  // Pass to next handler
  ProcessFFTData(AnaFFT, 1./input.LoopCount/sqrt(Params.FFTSize));
}

void OpenLoop::ProcessFFTData(FreqDomainData (&input)[2], double scale)
{
  EvDataUpdate(0);
}


ULONG OpenLoop::InCommand(ULONG msg, const OUTPUT_PARAMS2* info)
{
  ULONG ret;

  switch (msg)
  {case OUTPUT_SETUP:
    break;

   case OUTPUT_OPEN:
    { Format.samplerate = info->Info->tech->samplerate;
      Format.channels   = info->Info->tech->channels;
      OverrideTechInfo(info);
      ret = (*OutCommand)(OutA, msg, info);
      if (ret != 0)
        return ret;
      if (Format.channels > 2)
      { (*Ctx.plugin_api->message_display)(MSG_ERROR, "DRC123 does not support multi channel audio.");
        return PLUGIN_ERROR;
      }
      IsLowWater = false;
      InitComplete = false;
      Terminate = false;
      ClearRq = false;
      RefTID = _beginthread(PROXYFUNCREF(OpenLoop)RefThreadStub, 0, 65536, this);
      if (RefTID == -1)
      { Fail("Failed to start reference generator thread.");
        return PLUGIN_ERROR;
      }
      AnaTID = _beginthread(PROXYFUNCREF(OpenLoop)AnaThreadStub, 0, 65536, this);
      if (RefTID == -1)
      { Fail("Failed to start analyzer thread.");
        return PLUGIN_ERROR;
      }
      return 0;
    }

   case OUTPUT_PAUSE:
    // Pause is passed through - although it makes no sense because it destroys the current measurement.
    break;
   case OUTPUT_CLOSE:
    TerminateRequest();
    ret = (*OutCommand)(OutA, msg, info);
    if (RefTID > 0)
    { TID tid = RefTID;
      DosWaitThread(&tid, DCWW_WAIT);
      RefTID = -1;
    }
    if (AnaTID > 0)
    { TID tid = AnaTID;
      DosWaitThread(&tid, DCWW_WAIT);
      AnaTID = -1;
    }
    break;

   case OUTPUT_TRASH_BUFFERS:
    // Message has no meaning in open loop mode.
    return 0;
   case OUTPUT_VOLUME:
    // Volume is controlled by DRC123, but the commands are passed through the core engine.
    break;
  }
  // pass command down the filter stack.
  OverrideTechInfo(info);
  return (*OutCommand)(OutA, msg, info);
}

int OpenLoop::InRequestBuffer(const FORMAT_INFO2* format, float** buf)
{
  if (format->samplerate != Format.samplerate || format->channels != Format.channels)
  { Fail("DRC123 does not support changes of the sampling frequency or number of channels on the fly.");
    (*OutEvent)(OutW, OUTEVENT_PLAY_ERROR);
    return -1;
  }

  if (buf == NULL)
  { // flush => respond immediately by cancel.
    TerminateRequest();
    return 0;
  }

  // reuse buffer if unique
  if (!NextData || !NextData->RefCountIsUnique())
  { NextData = new SampleData(Params.FFTSize * 2);
    ResultLevel = 0;
  }
  *buf = NextData->get() + ResultLevel * format->channels;
  unsigned count = Params.FFTSize - ResultLevel;
  return Params.DiscardSamp && count > Params.DiscardSamp ? Params.DiscardSamp : count;
}

void OpenLoop::InCommitBuffer(int len, PM123_TIME pos)
{
  if (len > 0)
  { const float* start = NextData->get() + ResultLevel * Format.channels;
    Mutex::Lock lock(StatMtx);
    Stat[0].Add(start, Format.channels, len);
    if (Format.channels == 2)
      Stat[1].Add(start + 1, Format.channels, len);
    else
      Stat[1] = Stat[0];

    if (Params.DiscardSamp)
    { // do not count down before reference signal is ready.
      if (InitComplete)
        Params.DiscardSamp -= len;
      return;
    }
  }

  ResultLevel += len;
  if (ResultLevel < Params.FFTSize)
    return;
  else if (ResultLevel > Params.FFTSize)
    (*OutEvent)(OutW, OUTEVENT_PLAY_ERROR);
  else
  { // aggregate with current data
    if (ClearRq)
    { LoopCount = 0;
      ClearRq = false;
    } else
    { int_ptr<SampleData> cur(CurrentData);
      if (cur)
        *NextData += *cur;
    }
    NextData->LoopCount = ++LoopCount;
    // Pass new data to analyzer thread.
    NextData.swap(CurrentData);
    // Notify analyzer
    AnaEvent.Set();
  }
  ResultLevel = 0;
}

void OpenLoop::InEvent(OUTEVENTTYPE event)
{
  switch(event)
  {case OUTEVENT_LOW_WATER:
    IsLowWater = true;
    if (RefTID > 0 && InitComplete)
      DosSetPriority(PRTYS_THREAD, PRTYC_FOREGROUNDSERVER, 20, RefTID);
    return;

   case OUTEVENT_HIGH_WATER:
    IsLowWater = false;
    if (RefTID > 0 && InitComplete)
      DosSetPriority(PRTYS_THREAD, PRTYC_REGULAR, -20, RefTID);
    return;

   case OUTEVENT_END_OF_DATA:
    // Ignore this event.
    return;

   case OUTEVENT_PLAY_ERROR:;
  }
  // Pass through upstream filter.
  (*OutEvent)(OutW, event);
}


bool OpenLoop::Start(FilterMode mode, double volume)
{ DEBUGLOG(("OpenLoop::Start(%u, %f)\n", mode, volume));
  // Check if our plugin is enabled
  const char* ret = (*Ctx.plugin_api->exec_command)("plugin params filter drc123.dll");
  if (strstr(ret, "enabled=true") == NULL)
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, "The DRC123 plug-in is disabled. See PM123 properties dialog.");
    return false;
  }
  // check if the RecURI is configured
  xstring recuri(RecURI);
  if (!recuri || !recuri.length())
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, "The recording URI is not configured. See configuration tab.");
    return false;
  }
  // check whether some (other) playback is active.
  ret = (*Ctx.plugin_api->exec_command)("query play");
  if (strcmp(ret, "0") != 0)
    return false;
  // set volume
  xstringbuilder sb;
  sb.appendf("volume %f", volume);
  // TODO save previous value
  ret = (*Ctx.plugin_api->exec_command)(sb.cdata());
  if (!*ret)
    return false;
  // Play!
  CurrentMode = mode;
  sb.clear();
  sb.append("play \"");
  sb.append(recuri);
  sb.append('\"');
  ret = (*Ctx.plugin_api->exec_command)(sb.cdata());
  if (strcmp(ret, "0") != 0)
  { CurrentMode = MODE_DECONVOLUTION;
    return false;
  } else
    return true;
}

bool OpenLoop::Stop()
{ // deactivate whatever was active before.
  CurrentMode = MODE_DECONVOLUTION;
  (*Ctx.plugin_api->exec_command)("stop");
  // TODO: restore last song and volume
  return true;
}

void OpenLoop::SetVolume(double volume)
{
  if (CurrentMode != MODE_DECONVOLUTION)
  { // update player volume of currently playing reference signal immediately.
    char buf[20];
    sprintf(buf, "volume %f", volume * 100.);
    (*Ctx.plugin_api->exec_command)(buf);
  }
}

void OpenLoop::Fail(const char* message)
{ if (!Terminate)
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, message);
    TerminateRequest();
  }
}

void OpenLoop::GetStatistics(Statistics (&stat)[2])
{ Mutex::Lock lock(StatMtx);
  stat[0] = Stat[0];
  stat[1] = Stat[1];
  Stat[0].Reset();
  Stat[1].Reset();
}
