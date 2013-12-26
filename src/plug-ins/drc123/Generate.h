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
#include <cpp/container/sorted_vector.h>

class Generate
{public:
  enum FilterMode
  { FLT_None
  , FLT_Subsonic
  , FLT_Supersonic
  };
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
    double      NormFreqLow, NormFreqHigh;///< Normalization frequency range.
    NormalizeMode NormMode;           ///< Mode used for normalization
    double      LimitGain;            ///< Maximum gain of target result [factor]
    double      LimitDelay;           ///< Maximum delay of target result [seconds]
    double      LimitGainRate;        ///< Maximum d(gain)/d(f) [factor/frequency]
    double      LimitDelayRate;       ///< Maximum d(delay)/d(f) [seconds/frequency]
    bool        InvertHighGain;       ///< Reduce gain values over LimitGain
    FilterMode  Filter;               ///< Filter frequencies outside the frequency range.
    // GUI injection
    double      GainLow, GainHigh;    ///< Display range for gain response
    double      DelayLow, DelayHigh;  ///< Display range for delay
    void        swap(Parameters& r);
                Parameters()                    {}
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
  };
  class TargetFile
  : public DataFile
  , public Parameters
  {private:
    virtual bool ParseHeaderField(const char* string);
    virtual bool WriteHeaderFields(FILE* f);
   public:
                TargetFile();
    void        reset()                         { DataFile::reset(5); }
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
  static TargetFile Data;
 public:
  static const TargetFile DefData;
 public:
  TargetFile    LocalData;

 public:
  Generate(const TargetFile& params);
 public:
  static SyncRef<TargetFile> GetData()          { return Data; }
  void          Prepare();
  void          Run();
 private:
  double        ApplyGainLimit(double gain);
  double        ApplyDelayLimit(double delay);
};

FLAGSATTRIBUTE(Generate::FilterMode);

#endif // GENERATE_H
