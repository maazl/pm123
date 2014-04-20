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

#ifndef MEASURE_H
#define MEASURE_H

#include "OpenLoop.h"

class Deconvolution;

class Measure : public OpenLoop
{public:
  enum MeasureMode
  { MD_Noise
  , MD_Sweep
  };
  enum Channels
  { CH_Both
  , CH_Left
  , CH_Right
  };
  struct MesParameters
  { /// Measurement mode
    MeasureMode Mode;
    /// Channels to measure
    Channels    Chan;
    /// Soundcard calibration file, NULL if none.
    xstring     CalFile;
    /// Microphone calibration file, NULL if none.
    xstring     MicFile;
    /// Play the reference signal in differential mode,
    /// i.e. the right output has the inverse signal.
    bool        DiffOut;
    /// Use right line in channel as reference signal.
    bool        RefIn;
    /// Deconvolution of the reference to verify the result.
    bool        VerifyMode;
  };

  enum Column
  { Frequency
  , LGain
  , LDelay
  , RGain
  , RDelay
  , ColCount
  };
  /// @brief File with measurement data.
  class MeasureFile
  : public OpenLoopFile
  , public MesParameters
  {private:
    virtual bool ParseHeaderField(const char* string);
    virtual bool WriteHeaderFields(FILE* f);
   public:
                MeasureFile();
    void        reset()                         { OpenLoopFile::reset(ColCount); }
  };

 public:
  static const SVTable VTable;
  static const MeasureFile DefData;
 private:
  static MeasureFile Data;
 private:
  MesParameters MesParams;
  /// Another temporary, not destroyed by ComputeDelay.
  FreqDomainData AnaTemp2;
  FreqDomainData CalibrationL2L;
  FreqDomainData CalibrationR2R;
  FreqDomainData CalibrationR2L;
  FreqDomainData CalibrationL2R;

  sco_ptr<Deconvolution> VerifyFilter;

 protected:
                Measure(const MeasureFile& params, FILTER_PARAMS2& filterparams);
 public:
  static Measure* Factory(FILTER_PARAMS2& filterparams);
  virtual       ~Measure();
  virtual void  Update(const FILTER_PARAMS2& params);
 private:
  static  void  Inverse2x2(fftwf_complex& m11, fftwf_complex& m12, fftwf_complex& m21, fftwf_complex& m22);
 protected:
  virtual void  InitAnalyzer();
  virtual void  ProcessFFTData(FreqDomainData (&input)[2], double scale);
 public:
  static  bool  IsRunning()                     { return CurrentMode == MODE_MEASURE; }
  static  void  SetVolume(double volume);
  static  bool  Start();
  static  void  Clear();
  static SyncRef<MeasureFile> GetData()         { return Data; }
};

#endif // MEASURE_H_
