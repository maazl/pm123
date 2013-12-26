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

#include "Calibrate.h"
#include "FFT2Data.h"
#include <debuglog.h>


Calibrate::CalibrationFile::CalibrationFile()
: OpenLoopFile(13)
{ // Set Default Parameters
  FFTSize = 131072;
  DiscardSamp = 65536;
  RefFMin = 10.;
  RefFMax = 20000.;
  RefExponent = -.5;
  RefSkipEven = true;
  RefMode = RFM_STEREO;
  RefVolume = .9;
  RefFDist = 1.;
  AnaFBin = 1.02;
  AnaSwap = false;

  Mode = MD_StereoLoop;

  GainLow = -30.;
  GainHigh = +10.;
  DelayLow = -360;
  DelayHigh = +360;
  Gain2Low = -90.;
  Gain2High = -40.;
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
{ RefMode = params.Mode == MD_StereoLoop ? RFM_STEREO : RFM_MONO;
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
  double delay = ComputeDelay(input[CalParams.Mode == MD_RightLoop], GetRefDesign(CalParams.Mode == MD_RightLoop), response);
  if (CalParams.Mode == MD_StereoLoop)
    delay = (delay + ComputeDelay(input[1], GetRefDesign(1), response)) / 2;

  // Update results file
  { SyncAccess<CalibrationFile> data(Data);
    FFT2Data f2d(*data, (double)Format.samplerate / Params.FFTSize, data->AnaFBin, scale, delay);
    switch (CalParams.Mode)
    {case MD_StereoLoop:
      f2d.StoreFFT(LGain, input[0], GetRefDesign(0));
      f2d.StoreFFT(RGain, input[1], GetRefDesign(1));
      f2d.StoreFFT(R2LGain, input[0], GetRefDesign(1));
      f2d.StoreFFT(L2RGain, input[1], GetRefDesign(0));
      if (Params.RefSkipEven)
      { // Calc the sum output
        AnaTemp = GetRefDesign(0);
        AnaTemp += GetRefDesign(1);
        f2d.StoreHDN(LIntermod, input[0], AnaTemp);
        f2d.StoreHDN(RIntermod, input[1], AnaTemp);
      }
      break;
     case MD_BothLoop:
      // blank all frequencies without intensity
      AnaTemp = input[0];
      { const FreqDomainData& ref = GetRefDesign(0);
        for (unsigned i = 0; i < ref.size(); ++i)
        { const fftwf_complex& r = ref[i];
          if (r.real() == 0 && r.imag() == 0) // ref[i] == 0
            AnaTemp[i] = 0;
        }
      }
      f2d.Scale = 1; // scale factor cancels in this mode
      f2d.Delay = 0; // as well as the delay
      f2d.StoreFFT(DeltaGain, input[1], AnaTemp);
      break;
     case MD_LeftLoop:
      f2d.StoreFFT(LGain, input[0], GetRefDesign(0));
      f2d.StoreFFT(L2RGain, input[1], GetRefDesign(0));
      if (Params.RefSkipEven)
        f2d.StoreHDN(LIntermod, input[0], GetRefDesign(0));
      break;
     case MD_RightLoop:
      f2d.StoreFFT(RGain, input[1], GetRefDesign(1));
      f2d.StoreFFT(R2LGain, input[0], GetRefDesign(1));
      if (Params.RefSkipEven)
        f2d.StoreHDN(RIntermod, input[1], GetRefDesign(1));
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
