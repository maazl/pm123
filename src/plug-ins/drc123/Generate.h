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

#ifndef GENERATE_H
#define GENERATE_H

#include "DataFile.h"
#include "Measure.h"
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/container/sorted_vector.h>

class Generate
{public:
  enum NormalizeMode
  { NM_Energy
  , NM_Logarithm
  };
  struct Parameters
  {private:
    static int  CompareMeasurement(const char*const& key, const Measure::MeasureFile& data);
   public:
    typedef sorted_vector_own<Measure::MeasureFile,const char*,&Parameters::CompareMeasurement> MeasurementSet;
    MeasurementSet Measurements;
    double      FreqLow, FreqHigh;    ///< Display range for frequency
    double      FreqBin;              ///< Width of a frequency bin
    double      FreqFactor;           ///< Factor between neighbor frequencies.
    bool        NoPhase;              ///< Do not process phase response.
    double      NormFreqLow, NormFreqHigh;///< Normalization frequency range.
    NormalizeMode NormMode;           ///< Mode used for normalization
    double      LimitGain;            ///< Maximum gain of target result [factor]
    double      LimitDelay;           ///< Maximum delay of target result [seconds]
    double      GainSmoothing;        ///< Smooth gain values with an 2nd order IIR low pass
    double      DelaySmoothing;       ///< Smooth delay values with an 2nd order IIR low pass
    bool        InvertHighGain;       ///< Reduce gain values over LimitGain
    // GUI injection
    double      GainLow, GainHigh;    ///< Display range for gain response
    double      DelayLow, DelayHigh;  ///< Display range for delay
    void        swap(Parameters& r);
                Parameters()          {}
   protected:
    /// Copy everything but the content of the measurements.
                Parameters(const Parameters& r);
  };

  enum Column
  { Frequency
  , LGain
  , LDelay
  , RGain
  , RDelay
  , LGainLow
  , LGainHigh
  , LDelayLow
  , LDelayHigh
  , RGainLow
  , RGainHigh
  , RDelayLow
  , RDelayHigh
  , ColCount
  };
  class TargetFile
  : public DataFile
  , public Parameters
  {private:
    virtual bool ParseHeaderField(const char* string);
    virtual bool WriteHeaderFields(FILE* f);
   public:
                TargetFile();
    void        reset()                         { DataFile::reset(ColCount); }
    void        swap(TargetFile& r);
    virtual bool Load(const char* filename, bool nodata = false);
  };
 private:
  class AverageCollector
  { double      Sum;
    double      LastKey;
    double      LastValue;
    double      Limit;
    const double MinKey;
   public:
                AverageCollector(double minkey);
    void        Add(double key, double value);
    double      Finish(double maxkey);
  };

 private:
  static volatile sco_ptr<Generate> Instance;
  static event<Generate> Completed;
  static TargetFile Data;
 public:
  static const TargetFile DefData;
 public:
  TargetFile    LocalData;
  const char*   ErrorText;

 private:
  Generate(const TargetFile& params);
  void          Prepare();
  void          Run();
  PROXYFUNCDEF void TFNENTRY ThreadStub(void* arg);
 public:
  static SyncRef<TargetFile> GetData()          { return Data; }
  static bool   Start();
  static bool   Running()                       { return Instance.get() != NULL; }
  static event_pub<Generate>& GetCompleted()    { return Completed; }

 private:
  double        ApplyGainLimit(double gain);
  double        ApplyDelayLimit(double delay);
  void          ApplySmoothing(unsigned col, double width);
};

#endif // GENERATE_H
