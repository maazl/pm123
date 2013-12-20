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
  { MeasureMode Mode;
    Channels    Chan;
    bool        DiffOut;
    bool        RefIn;
    xstring     CalFile;
  };
  /// @brief File with measurement data.
  class MeasureFile
  : public OpenLoopFile
  , public MesParameters
  {public:
    enum Column
    { Frequency
    , LGain
    , LDelay
    , RGain
    , RDelay
    };
   private:
    virtual bool ParseHeaderField(const char* string);
    virtual bool WriteHeaderFields(FILE* f);
   public:
                MeasureFile();
    void        reset()                         { OpenLoopFile::reset(5); }
  };

 public:
  static const SVTable VTable;
  static const MeasureFile DefData;
 private:
  static MeasureFile Data;
 private:
  MesParameters MesParams;
  FreqDomainData AnaTemp2;

 protected:
                Measure(const MeasureFile& params, FILTER_PARAMS2& filterparams);
 public:
  static Measure* Factory(FILTER_PARAMS2& filterparams);
  virtual       ~Measure();
 protected:
  virtual void  ProcessFFTData(FreqDomainData (&input)[2], double scale);
 public:
  static  bool  IsRunning()                     { return CurrentMode == MODE_MEASURE; }
  static  void  SetVolume(double volume);
  static  bool  Start();
  static  void  Clear();
  static SyncRef<MeasureFile> GetData()         { return Data; }
};

#endif // MEASURE_H_
