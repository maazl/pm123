/*
 * Copyright 2013-2014 Marcel Mueller
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
#include "Calibrate.h"
#include "Deconvolution.h"


Measure::MeasureFile::MeasureFile()
: OpenLoopFile(5)
{ // Set Default Parameters
  FFTSize = 1<<19;
  DiscardSamp = 1<<19;
  RefFMin = 20.;
  RefFMax = 20000.;
  RefExponent = 0;
  RefSkipEven = false;
  RefSkipRand = true;
  RefMode = RFM_STEREO;
  RefVolume = .9;
  RefFreqFactor = 500E-6;
  RefEnergyDist = false;
  LineNotchHarmonics = 3;
  LineNotchFreq = 50.;
  AnaFBin = 0;
  AnaSwap = false;

  Mode = MD_Noise;
  Chan = CH_Both;
  CalFile = xstring::empty;
  MicFile = xstring::empty;
  RefIn = false;
  DiffOut = false;
  VerifyMode = false;

  GainLow = -40.;
  GainHigh = +20.;
  DelayLow = -.075;
  DelayHigh = +.15;
  DispBounds = true;
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
  else if (!!(value = TryParam(string, "MicrophoneFile")))
    MicFile = value;
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
    "##MicrophoneFile=%s\n"
    , Mode, Chan, DiffOut, RefIn, CalFile.cdata(), MicFile.cdata() );
  return OpenLoopFile::WriteHeaderFields(f);
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
{ if (params.Chan == CH_Both)
    Params.RefMode = RFM_STEREO;
  else if (params.DiffOut)
    Params.RefMode = RFM_DIFFERENTIAL;
  else
    Params.RefMode = (Mode)params.Chan;
}

Measure* Measure::Factory(FILTER_PARAMS2& filterparams)
{ SyncAccess<MeasureFile> params(Data);
  Deconvolution* verifier = NULL;
  if (params->VerifyMode)
    filterparams.a = verifier = new Deconvolution(filterparams);
  Measure* result = new Measure(*params, filterparams);
  result->VerifyFilter = verifier;
  return result;
}

Measure::~Measure()
{}

void Measure::Update(const FILTER_PARAMS2& params)
{ if (MesParams.VerifyMode)
    VerifyFilter->Update(params);
  else
    OpenLoop::Update(params);
}

static inline bool operator==(const fftwf_complex& l, const float& r)
{ return l.imag() == 0. && l.real() == r;
}

void Measure::Inverse2x2(fftwf_complex& m11, fftwf_complex& m12, fftwf_complex& m21, fftwf_complex& m22)
{
  fftwf_complex det = m11 * m22 - m12 * m21;
  if (det == 0.)
  { // grumble, no intensity? Can't invert, use identity (= no calibration) in doubt.
    m11 = 1.;
    m22 = 1.;
    m12 = 0.;
    m21 = 0.;
  } else
  { fftwf_complex n22 = m11 / det;
    m11 = m22 / det;
    m22 = n22;
    m12 /= -det;
    m21 /= -det;
  }
}

void Measure::InitAnalyzer()
{ DataFile cal;
  Data2FFT d2f(cal, Params.FFTSize, (double)Format.samplerate / Params.FFTSize);
  // Prepare calibration data
  if (MesParams.CalFile.length())
  { if (!cal.Load(MesParams.CalFile))
    { Fail(xstring().sprintf("Failed to open calibration file %s\n%s", MesParams.CalFile.cdata(), strerror(errno)));
      return;
    }
    bool xtalk = cal.HasColumn(Calibrate::L2RGain) && cal.HasColumn(Calibrate::L2RDelay)
              && cal.HasColumn(Calibrate::R2LGain) && cal.HasColumn(Calibrate::R2LDelay);
    if (MesParams.RefIn && !xtalk)
    { // short way for relative calibration
      // We apply the R/L delta here to L only,
      // because in ProcessFFTData the quotient L/R is only used.
      // So we save a division.
      if (!cal.HasColumn(Calibrate::DeltaGain) || !cal.HasColumn(Calibrate::DeltaDelay))
        Ctx.plugin_api->message_display(MSG_WARNING, xstring().sprintf("The calibration file %s has no differential data and is useless with reference input mode.", MesParams.CalFile.cdata()));
       else
        d2f.LoadFFT(Calibrate::DeltaGain, CalibrationL2L);
      goto caldone;
    }
    if ( !cal.HasColumn(Calibrate::LGain) || !cal.HasColumn(Calibrate::LDelay)
      || !cal.HasColumn(Calibrate::RGain) || !cal.HasColumn(Calibrate::RDelay) )
    { if (MesParams.RefIn)
        goto refin;
      Fail(xstring().sprintf("The calibration file %s does not contain calibration data and cannot be used.", MesParams.CalFile.cdata()));
      return;
    }
    d2f.LoadFFT(Calibrate::LGain, CalibrationL2L);
    d2f.LoadFFT(Calibrate::RGain, CalibrationR2R);
    if (xtalk)
    { d2f.LoadFFT(Calibrate::L2RGain, CalibrationL2R);
      d2f.LoadFFT(Calibrate::R2LGain, CalibrationR2L);
      // do matrix inversions
      for (unsigned i = 0; i < d2f.TargetSize; ++i)
        Inverse2x2(CalibrationL2L[i], CalibrationR2L[i], CalibrationL2R[i], CalibrationR2R[i]);
    } else
    { CalibrationL2L.Invert();
      CalibrationR2R.Invert();
    }
    if (MesParams.RefIn)
    { // use relative calibration
     refin:
      if (!cal.HasColumn(Calibrate::DeltaGain) || !cal.HasColumn(Calibrate::DeltaDelay))
      { Ctx.plugin_api->message_display(MSG_WARNING, xstring().sprintf("The calibration file %s has no differential data. This is recommended with reference input mode.", MesParams.CalFile.cdata()));
        goto caldone;
      }
      CalibrationR2L /= CalibrationL2L;
      // We apply the R/L delta here to L only,
      // because in ProcessFFTData the quotient L/R is only used.
      // So we save a division.
      d2f.LoadFFT(Calibrate::DeltaGain, CalibrationL2L);
      CalibrationR2L *= CalibrationL2L;
      CalibrationL2R /= CalibrationR2R;
      d2f.LoadIdentity(CalibrationR2R);
    }
  }
 caldone:

  if (MesParams.MicFile.length())
  { if (!cal.Load(MesParams.MicFile))
    { Fail(xstring().sprintf("Failed to open microphone calibration %s\n%s", MesParams.MicFile.cdata(), strerror(errno)));
      return;
    }
    if (!cal.HasColumn(Measure::LGain) || !cal.HasColumn(Measure::LDelay))
    { Fail(xstring().sprintf("The microphone calibration file %s has no calibration data and cannot be used.", MesParams.MicFile.cdata()));
      return;
    }
    if (!CalibrationL2L.size())
    { // if we have no calibration data so far, go the short way
      d2f.LoadFFT(Measure::LGain, CalibrationL2L);
      CalibrationL2L.Invert();
      goto micdone;
    }
    d2f.LoadFFT(Measure::LGain, AnaTemp);
    // join with calibration above
    CalibrationL2L /= AnaTemp;
    if (CalibrationR2L.size())
      CalibrationR2L /= AnaTemp;
  }
 micdone:;
}

void Measure::ProcessFFTData(FreqDomainData (&input)[2], double scale)
{ DEBUGLOG(("Measure::ProcessFFTData(,%g)\n", scale));
  // calibration
  if (CalibrationR2L.size())
  { // Have full matrix calibration. Calculate
    // | L |   | CalL2L CalR2L |
    // |   | * |               |
    // | R |   | CalL2R CalR2R |
    for (unsigned i = 0; i < input[0].size(); ++i)
    { fftwf_complex lin = input[0][i];
      fftwf_complex& r = input[1][i];
      input[0][i] = lin * CalibrationL2L[i] + r * CalibrationR2L[i];
      r = lin * CalibrationR2L[i] + r * CalibrationR2R[i];
    }
  } else if (CalibrationR2R.size())
  { // have two channel calibration data
    input[0] *= CalibrationL2L;
    input[1] *= CalibrationR2R;
  } else if (CalibrationL2L.size())
  { // have one channel calibration data
    input[0] *= CalibrationL2L;
  }

  // get reference signal
  const FreqDomainData* ref[2];
  if (MesParams.RefIn)
  { // reference signal is on channel 2
    scale = 1; // all scale factors cancel anyway
    ref[0] = ref[1] = &input[1];

    const fftwf_complex* rp = GetRefDesign(1).begin();
    switch (MesParams.Chan)
    {case CH_Both:
      // split reference signal into two channels
      AnaTemp2 = input[1]; // copy
      ref[1] = &AnaTemp2;
      // mute unused frequencies of right channel
      foreach (fftwf_complex,*, dp, AnaTemp2)
        if (*rp++ == 0.)
          *dp = 0.;
     case CH_Left:
      rp = GetRefDesign(0).begin();
     case CH_Right:
      // mute unused frequencies of left or single channel
      foreach (fftwf_complex,*, dp, input[1])
        if (*rp++ == 0.)
          *dp = 0.;
    }
  } else
  { // reference signal is not recorded
    ref[0] = &GetRefDesign(0);
    ref[1] = &GetRefDesign(1);
  }

  // update results
  { double response;
    SyncAccess<MeasureFile> data(Data);
    FFT2OpenLoopData f2d(*data, (double)Format.samplerate / Params.FFTSize, Params.AnaFBin + 1);
    f2d.Scale = scale;

    switch (MesParams.Chan)
    {case CH_Right:
      input[0] /= *ref[1];
      f2d.Delay = ComputeDelay(input[1], *ref[1], response);
      f2d.StoreFFT(RGain, input[0]);
      f2d.StorePhaseInfo(1);
      break;
     default: //case CH_Both:
      AnaTemp = input[0];
      AnaTemp /= *ref[1];
      f2d.Delay = ComputeDelay(input[1], *ref[1], response);
      f2d.StoreFFT(RGain, AnaTemp);
      f2d.StorePhaseInfo(1);
      // no break
     case CH_Left:
      input[0] /= *ref[0];
      f2d.Delay = ComputeDelay(input[0], *ref[0], response);
      f2d.StoreFFT(LGain, input[0]);
      f2d.StorePhaseInfo(0);
      // no break
    }
  }

  OpenLoop::ProcessFFTData(input, scale);
}

void Measure::SetVolume(double volume)
{ if (IsRunning())
    OpenLoop::SetVolume(volume);
  SyncAccess<Measure::MeasureFile> data(Data);
  data->RefVolume = volume;
}

bool Measure::Start()
{ if (Data.Mode == MD_Sweep)
  { Ctx.plugin_api->message_display(MSG_ERROR, "Sorry, sweep mode has not yet been implemented.");
    return false;
  }
  return OpenLoop::Start(MODE_MEASURE, Data.RefVolume);
}

void Measure::Clear()
{ SyncAccess<Measure::MeasureFile> data(Data);
  data->reset();
  OpenLoop::Clear();
}
