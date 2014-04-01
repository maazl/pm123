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

#include "Calibrate.h"
#include "FFT2Data.h"
#include <debuglog.h>


Calibrate::CalibrationFile::CalibrationFile()
: OpenLoopFile(13)
{ // Set Default Parameters
  FFTSize = 65536;
  DiscardSamp = 65536;
  RefFMin = 10.;
  RefFMax = 22000.;
  RefExponent = 0.;
  RefSkipEven = true;
  RefSkipRand = true;
  RefMode = RFM_STEREO;
  RefVolume = .9;
  RefFreqFactor = 0.002;
  RefEnergyDist = false;
  LineNotchHarmonics = 0;
  LineNotchFreq = 50.;
  AnaFBin = 0.01;
  AnaSwap = false;

  Mode = MD_StereoLoop;

  GainLow = -30.;
  GainHigh = +10.;
  DelayLow = -360;
  DelayHigh = +360;
  Gain2Low = -90.;
  Gain2High = -40.;
  DispBounds = true;
  VULow = -40;
  VUYellow = -12;
  VURed = -6;
}

bool Calibrate::CalibrationFile::ParseHeaderField(const char* string)
{ const char* value;
  if (!!(value = TryParam(string, "Mode")))
    Mode = (MeasureMode)atoi(value);
  else
    return OpenLoopFile::ParseHeaderField(string);
  return true;
}

bool Calibrate::CalibrationFile::WriteHeaderFields(FILE* f)
{
  fprintf(f, "##Mode=%u\n", Mode);
  return OpenLoopFile::WriteHeaderFields(f);
}

const OpenLoop::SVTable Calibrate::VTable =
{ &Calibrate::IsRunning
, &Calibrate::Start
, &Calibrate::Stop
, &Calibrate::Clear
, &Calibrate::SetVolume
, EvDataUpdate
};
const Calibrate::CalibrationFile Calibrate::DefData;
Calibrate::CalibrationFile Calibrate::Data;


Calibrate::Calibrate(const CalibrationFile& params, FILTER_PARAMS2& filterparams)
: OpenLoop(params, filterparams)
, CalParams(params)
{ Params.RefMode = params.Mode == MD_StereoLoop ? RFM_STEREO : RFM_MONO;
}

Calibrate* Calibrate::Factory(FILTER_PARAMS2& filterparams)
{ SyncAccess<CalibrationFile> params(Data);
  return new Calibrate(*params, filterparams);
}

Calibrate::~Calibrate()
{}

ULONG Calibrate::InCommand(ULONG msg, const OUTPUT_PARAMS2* info)
{
  if (msg == OUTPUT_OPEN && info->Info->tech->channels != 2)
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, "DRC123 calibration requires stereo input.");
    return PLUGIN_ERROR;
  }
  return OpenLoop::InCommand(msg, info);
}

void Calibrate::ProcessFFTData(FreqDomainData (&input)[2], double scale)
{ // Calculate the average group delay to avoid overflows in delta-phi.
  double response;
  // Update results file
  { SyncAccess<CalibrationFile> data(Data);
    FFT2OpenLoopData f2d(*data, (double)Format.samplerate / Params.FFTSize, data->AnaFBin + 1);
    f2d.Scale = scale;
    switch (CalParams.Mode)
    {case MD_StereoLoop:
      if (Params.RefSkipEven)
      { // Calc the sum output
        AnaTemp = GetRefDesign(0);
        AnaTemp += GetRefDesign(1);
        f2d.StoreHDN(LIntermod, input[0], AnaTemp);
        f2d.StoreHDN(RIntermod, input[1], AnaTemp);
      }
      f2d.Delay = data->AverageDelay[0] = ComputeDelay(input[0], GetRefDesign(0), response);
      f2d.Delay = data->AverageDelay[1] = ComputeDelay(input[1], GetRefDesign(1), response);
      AnaTemp = input[0];
      AnaTemp /= GetRefDesign(1);
      f2d.StoreFFT(R2LGain, AnaTemp);
      AnaTemp = input[1];
      AnaTemp /= GetRefDesign(0);
      f2d.StoreFFT(L2RGain, AnaTemp);
      input[0] /= GetRefDesign(0);
      f2d.StoreFFT(LGain, input[0]);
      f2d.StorePhaseInfo(0);
      input[1] /= GetRefDesign(1);
      f2d.Delay = data->AverageDelay[1];
      f2d.StoreFFT(RGain, input[1]);
      f2d.StorePhaseInfo(1);
      break;
     case MD_BothLoop:
      f2d.Scale = 1; // scale factor cancels in this mode as well as the delay
      input[1] /= input[0];
      // blank all frequencies without intensity
      { const FreqDomainData& ref = GetRefDesign(0);
        for (unsigned i = 0; i < ref.size(); ++i)
          if (ref[i] == 0.F)
            input[1][i] = NAN;
      }
      f2d.StoreFFT(DeltaGain, input[1]);
      break;
     case MD_LeftLoop:
      if (Params.RefSkipEven)
        f2d.StoreHDN(LIntermod, input[0], GetRefDesign(0));
      input[1] /= GetRefDesign(0);
      f2d.Delay = ComputeDelay(input[0], GetRefDesign(0), response);
      f2d.StoreFFT(L2RGain, input[1]);
      input[0] /= GetRefDesign(0);
      f2d.StoreFFT(LGain, input[0]);
      f2d.StorePhaseInfo(0);
      break;
     case MD_RightLoop:
      if (Params.RefSkipEven)
        f2d.StoreHDN(RIntermod, input[1], GetRefDesign(1));
      f2d.Delay = ComputeDelay(input[1], GetRefDesign(1), response);
      input[0] /= GetRefDesign(1);
      f2d.StoreFFT(R2LGain, input[0]);
      input[1] /= GetRefDesign(1);
      f2d.StoreFFT(RGain, input[1]);
      f2d.StorePhaseInfo(1);
      break;
    }
  }

  OpenLoop::ProcessFFTData(input, scale);
}

void Calibrate::SetVolume(double volume)
{
  if (IsRunning())
    OpenLoop::SetVolume(volume);
  SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  data->RefVolume = volume;
}

bool Calibrate::Start()
{
  return OpenLoop::Start(MODE_CALIBRATE, Data.RefVolume);
}

void Calibrate::Clear()
{
  SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  data->reset();
  OpenLoop::Clear();
}
