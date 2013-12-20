/*
 * Copyright 2013 Marcel Mueller
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

#include "Measure.h"
#include "FFT2Data.h"


Measure::MeasureFile::MeasureFile()
: OpenLoopFile(5)
{ // Set Default Parameters
  FFTSize = 262144;
  DiscardSamp = 262144;
  RefFMin = 20.;
  RefFMax = 20000.;
  RefExponent = -.2;
  RefSkipEven = false;
  RefMode = RFM_STEREO;
  RefVolume = .9;
  RefFDist = 1.00;
  AnaFBin = 1.02;
  AnaSwap = false;

  Mode = MD_Noise;
  Chan = CH_Both;
  RefIn = false;
  DiffOut = false;
  CalFile = xstring::empty;

  GainLow = -40.;
  GainHigh = +20.;
  DelayLow = -.05;
  DelayHigh = +.1;
  VULow = -40;
  VUYellow = -12;
  VURed = -6;
}

bool Measure::MeasureFile::ParseHeaderField(const char* string)
{ const char* value;
  if (!!(value = TryParam(string, "Mode")))
    Mode = (MeasureMode)atoi(value);
  else if (!!(value = TryParam(string, "Channels")))
    Chan = (Channels)atoi(value);
  else if (!!(value = TryParam(string, "DiffOut")))
    DiffOut = (bool)atoi(value);
  else if (!!(value = TryParam(string, "DiffIn")))
    RefIn = (bool)atoi(value);
  else if (!!(value = TryParam(string, "CalibrationFile")))
    CalFile = value;
  else
    return OpenLoopFile::ParseHeaderField(string);
  return true;
}

bool Measure::MeasureFile::WriteHeaderFields(FILE* f)
{
  fprintf( f,
    "##Mode=%u\n"
    "##Channels=%u\n"
    "##DiffOut=%u\n"
    "##DiffIn=%u\n"
    "##CalibrationFile=%s\n"
    , Mode, Chan, DiffOut, RefIn, CalFile.cdata() );
  return true;
}

const OpenLoop::SVTable Measure::VTable =
{ &Measure::IsRunning
, &Measure::Start
, &Measure::Stop
, &Measure::Clear
, &Measure::SetVolume
, EvDataUpdate
};
const Measure::MeasureFile Measure::DefData;
Measure::MeasureFile Measure::Data;

Measure::Measure(const MeasureFile& params, FILTER_PARAMS2& filterparams)
: OpenLoop(params, filterparams)
, MesParams(params)
{ RefMode = params.Chan == CH_Both ? RFM_STEREO : params.DiffOut ? RFM_DIFFERENTIAL : RFM_MONO;
}

Measure* Measure::Factory(FILTER_PARAMS2& filterparams)
{ SyncAccess<MeasureFile> params(Data);
  return new Measure(*params, filterparams);
}

Measure::~Measure()
{}

static inline bool operator==(const fftwf_complex& l, const float& r)
{ return l.imag() == 0. && l.real() == r;
}

void Measure::ProcessFFTData(FreqDomainData (&input)[2], double scale)
{ DEBUGLOG(("Measure::ProcessFFTData(,%g)\n", scale));
  // get reference signal
  const FreqDomainData* ref[2];
  if (MesParams.RefIn)
  { // reference signal is on channel 2
    scale = 1; // all scale factors cancel anyway

    // TODO: calibration

    ref[0] = &input[1];

    if (MesParams.Chan == CH_Both)
    { // dual channel mode
      // split reference signal into two channels
      ref[1] = &AnaTemp2;
      AnaTemp2 = *ref[0]; // copy
      // mute unused frequencies of right channel
      const fftwf_complex* rp = GetRefDesign(1).begin();
      foreach (fftwf_complex,*, dp, AnaTemp2)
        if (*rp++ == 0.)
          *dp = 0.;
    }
    // mute unused frequencies of left or single channel
    const fftwf_complex* rp = GetRefDesign(MesParams.Chan == CH_Right).begin();
    foreach (fftwf_complex,*, dp, input[1])
      if (*rp++ == 0.)
        *dp = 0.;
  } else
  { // reference signal is not recorded

    // TODO: calibration

    ref[0] = &GetRefDesign(0);
    ref[1] = &GetRefDesign(1);
  }

  // Compute average group delay
  double response[2];
  double delay = ComputeDelay(input[0], *ref[0], response[0]);
  if (MesParams.Chan == CH_Both)
  { // Take delay on right channel into account too.
    delay = (delay + ComputeDelay(input[0], *ref[1], response[1])) / 2;
    /*if (response[0] * response[1] < 0) // different sign
    { // Warning speaker inverted
    }*/
  }

  // update results
  { SyncAccess<MeasureFile> data(Data);
    FFT2Data f2d(*data, (double)Format.samplerate / Params.FFTSize, Params.AnaFBin, scale, delay);

    switch (MesParams.Chan)
    {case CH_Right:
      f2d.StoreFFT(MeasureFile::RGain, input[0], *ref[0]);
      break;
     case CH_Both:
      f2d.StoreFFT(MeasureFile::RGain, input[0], *ref[1]);
      // no break
     case CH_Left:
      f2d.StoreFFT(MeasureFile::LGain, input[0], *ref[0]);
      // no break
    }
  }

  OpenLoop::ProcessFFTData(input, scale);
}

void Measure::SetVolume(double volume)
{
  if (IsRunning())
    OpenLoop::SetVolume(volume);
  SyncAccess<Measure::MeasureFile> data(Data);
  data->RefVolume = volume;
}

bool Measure::Start()
{
  return OpenLoop::Start(MODE_MEASURE, Data.RefVolume);
}

void Measure::Clear()
{
  SyncAccess<Measure::MeasureFile> data(Data);
  data->reset();
  OpenLoop::Clear();
}
