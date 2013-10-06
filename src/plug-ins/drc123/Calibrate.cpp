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
: DataFile(9)
, Volume(.8)
, Mode(MD_StereoLoop)
, PlanSize(65536)
{}

bool Calibrate::CalibrationFile::ParseHeaderField(const char* string)
{
  if (strnicmp(string, "VOLUME=", 7) == 0)
    Volume = atof(string + 7);
  else if (strnicmp(string, "MODE=", 5) == 0)
    Mode = (MeasureMode)atoi(string + 5);
  return true;
}

bool Calibrate::CalibrationFile::WriteHeaderFields(FILE* f)
{
  fprintf(f, "##VOLUME=%f\n##MODE=%u\n", Volume, Mode);
  return true;
}


Calibrate::CalibrationFile Calibrate::CalData;
event<const int> Calibrate::EvDataUpdate;


Calibrate::Calibrate(FILTER_PARAMS2& params)
: OpenLoop(params)
{ FFTSize = CalData.PlanSize;
  RefExponent = -.5;
  RefMode = CalData.Mode == MD_StereoLoop ? RFM_STEREO : RFM_MONO;

  const unsigned fdsize = FFTSize / 2 + 1;
  AnaCross.reset(fdsize);
  ResCross.reset(FFTSize);
  // plan for cross correlation
  CrossPlan = fftwf_plan_dft_c2r_1d(FFTSize, AnaCross.begin(), ResCross.begin(), FFTW_ESTIMATE|FFTW_DESTROY_INPUT);
}

Calibrate::~Calibrate()
{ fftwf_destroy_plan(CrossPlan);
}

ULONG Calibrate::InCommand(ULONG msg, const OUTPUT_PARAMS2* info)
{
  if (msg == OUTPUT_OPEN && info->Info->tech->channels != 2)
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, "DRC123 calibration requires stereo input.");
    return PLUGIN_ERROR;
  }
  return OpenLoop::InCommand(msg, info);
}

void Calibrate::ProcessFFTData(FreqDomainData (&input)[2], double scale)
{ double finc = (double)Format.samplerate / FFTSize;
  FFT2Data f2d(CalData, finc, FBin, scale);

  // Calculate the average group delay to avoid overflows in delta-phi.
  { const fftwf_complex* sp1 = input[0].begin();
    const fftwf_complex* sp2 = GetRefDesign(0).begin();
    foreach(fftwf_complex,*, dp, AnaCross)
      *dp = *sp1++ * conj(*sp2++);
    fftwf_execute(CrossPlan);
    // Find maximum
    double max = 0;
    unsigned pos = 0;
    double rms = 0;
    foreach(float,*, sp, ResCross)
    { double norm = *sp * *sp;
      rms += norm;
      if (norm > max)
      { max = norm;
        pos = sp - ResCross.begin();
      }
    }
    // TODO check SNR
    f2d.Delay = (double)pos / Format.samplerate;
    DEBUGLOG(("Calibrate::ProcessFFTData: Cross correlation max = %g@%u, ratio = %g, delay = %g\n",
      sqrt(max), pos, sqrt(max / rms * FFTSize), f2d.Delay));
  }

  // Update results file
  f2d.StoreFFT(1, input[0], GetRefDesign(0));
  f2d.StoreFFT(3, input[1], GetRefDesign(1));
  f2d.StoreFFT(5, input[0], GetRefDesign(1));
  f2d.StoreFFT(7, input[1], GetRefDesign(0));

  EvDataUpdate(0);
}


