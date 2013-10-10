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

#ifndef CALIBRATE_H
#define CALIBRATE_H

#include "OpenLoop.h"
#include "DataFile.h"
#include "DataVector.h"
#include <cpp/mutex.h>
#include <cpp/event.h>
#include <fftw3.h>


class Calibrate : public OpenLoop
{public: // Configuration interface
  enum MeasureMode
  { MD_StereoLoop
  , MD_MonoLoop
  , MD_LeftLoop
  , MD_RightLoop
  };
  struct CalParameters
  { double      Volume;
    MeasureMode Mode;
  };
  class CalibrationFile
  : public DataFile
  , public Parameters
  , public CalParameters
  {private:
    virtual bool ParseHeaderField(const char* string);
    virtual bool WriteHeaderFields(FILE* f);
   public:
                CalibrationFile();
  };

 private:
  static CalibrationFile Data;
 private:
  CalParameters CalParams;
  static event<const int> EvDataUpdate;
  FreqDomainData AnaCross;
  TimeDomainData ResCross;
  fftwf_plan    CrossPlan;

 private:
                Calibrate(const CalibrationFile& params, FILTER_PARAMS2& filterparams);
 public:
  static Calibrate* Factory(FILTER_PARAMS2& filterparams);
  virtual       ~Calibrate();
 protected:
  virtual ULONG InCommand(ULONG msg, const OUTPUT_PARAMS2* info);
  virtual void  ProcessFFTData(FreqDomainData (&input)[2], double scale);
 public:
  static  void  SetVolume(double volume)            { if (CurrentMode == MODE_CALIBRATE) OpenLoop::SetVolume(volume); }
  static  bool  Start()                             { return OpenLoop::Start(MODE_CALIBRATE); }
  static event_pub<const int>& GetEvDataUpdate()    { return EvDataUpdate; }
  static SyncRef<CalibrationFile> GetData()     { return Data; }
};


#endif // CALIBRATE_H
