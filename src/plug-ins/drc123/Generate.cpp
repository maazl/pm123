/*
 * Copyright 2012-2013 Marcel Mueller
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

#include "Generate.h"
#include <cpp/smartptr.h>


Generate::Parameters::Parameters(const Parameters& r)
{
  foreach (Measure::MeasureFile,*const*, sp, r.Measurements)
  { Measure::MeasureFile* file = new Measure::MeasureFile();
    file->FileName = (*sp)->FileName;
    Measurements.get(file->FileName) = file;
  }
  FreqLow  = r.FreqLow;
  FreqHigh = r.FreqHigh;
  FreqBin  = r.FreqBin;
  FreqFactor = r.FreqFactor;
  NormFreqLow = r.NormFreqLow;
  NormFreqHigh = r.NormFreqHigh;
  LimitGain = r.LimitGain;
  LimitDelay = r.LimitDelay;
  LimitGainRate = r.LimitGainRate;
  LimitDelayRate = r.LimitDelayRate;
  InvertHighGain = r.InvertHighGain;
  GainLow  = r.GainLow;
  GainHigh = r.GainHigh;
  DelayLow = r.DelayLow;
  DelayHigh = r.DelayHigh;
}

int Generate::Parameters::CompareMeasurement(const char*const& key, const Measure::MeasureFile& data)
{ return xstring::compareI(key, data.FileName);
}

void Generate::Parameters::swap(Parameters& r)
{
  Measurements.swap(r.Measurements);
  ::swap(FreqLow,  r.FreqLow);
  ::swap(FreqHigh, r.FreqHigh);
  ::swap(FreqBin,  r.FreqBin);
  ::swap(FreqFactor, r.FreqFactor);
  ::swap(NormFreqLow, r.NormFreqLow);
  ::swap(NormFreqHigh, r.NormFreqHigh);
  ::swap(LimitGain, r.LimitGain);
  ::swap(LimitDelay, r.LimitDelay);
  ::swap(LimitGainRate, r.LimitGainRate);
  ::swap(LimitDelayRate, r.LimitDelayRate);
  ::swap(InvertHighGain, r.InvertHighGain);
  ::swap(Filter, r.Filter);
  ::swap(GainLow,  r.GainLow);
  ::swap(GainHigh, r.GainHigh);
  ::swap(DelayLow, r.DelayLow);
  ::swap(DelayHigh, r.DelayHigh);
}


Generate::TargetFile::TargetFile()
: DataFile(5)
{ FreqLow  = 20.;
  FreqHigh = 20000.;
  FreqBin  = .25;
  FreqFactor = .005;
  NormFreqLow = 80.;
  NormFreqHigh = 500.;
  LimitGain = 10.;
  LimitDelay = .5;
  LimitGainRate = .2;
  LimitDelayRate = .05;
  InvertHighGain = true;
  Filter = FLT_Subsonic|FLT_Supersonic;
  GainLow  = -20.;
  GainHigh = +20.;
  DelayLow = -.05;
  DelayHigh = +.15;
}


void Generate::TargetFile::swap(TargetFile& r)
{ DataFile::swap(r);
  Parameters::swap(r);
}

bool Generate::TargetFile::ParseHeaderField(const char* string)
{const char* value;
  if (!!(value = TryParam(string, "Measurement")))
  { Measure::MeasureFile* file = new Measure::MeasureFile();
    file->FileName = value;
    Measurements.get(file->FileName) = file;
  } else if (!!(value = TryParam(string, "Freq")))
    sscanf(value, "%lf,%lf", &FreqLow, &FreqHigh);
  else if (!!(value = TryParam(string, "FreqBin")))
    sscanf(value, "%lf,%lf", &FreqBin, &FreqFactor);
  else if (!!(value = TryParam(string, "NormFreq")))
      sscanf(value, "%lf,%lf", &NormFreqLow, &NormFreqHigh);
  else if (!!(value = TryParam(string, "LimitGain")))
    sscanf(value, "%lf,%lf", &LimitGain, &LimitGainRate);
  else if (!!(value = TryParam(string, "LimitDelay")))
    sscanf(value, "%lf,%lf", &LimitDelay, &LimitDelayRate);
  else if (!!(value = TryParam(string, "InvertHighGain")))
    InvertHighGain = !!atoi(value);
  else if (!!(value = TryParam(string, "FilterMode")))
    Filter = (FilterMode)atoi(value);
  else if (!!(value = TryParam(string, "DispGain")))
    sscanf(value, "%lf,%lf", &GainLow, &GainHigh);
  else if (!!(value = TryParam(string, "DispDelay")))
    sscanf(value, "%lf,%lf", &DelayLow, &DelayHigh);
  else
    return DataFile::ParseHeaderField(string);
  return true;
}

bool Generate::TargetFile::WriteHeaderFields(FILE* f)
{ foreach(const Measure::MeasureFile,*const*, sp, Measurements)
    fprintf(f, "##Measurement=%s\n", (*sp)->FileName.cdata());
  fprintf( f,
    "##Freq=%f,%f\n"
    "##FreqBin=%f,%f\n"
    "##NormFreq=%f,%f\n"
    "##LimitGain=%f,%f\n"
    "##Limitdelay=%f,%f\n"
    "##InvertHighGain=%u\n"
    "##FilterMode=%u\n"
    "##DispGain=%f,%f\n"
    "##DispDelay=%f,%f\n"
    , FreqLow, FreqHigh, FreqBin, FreqFactor, NormFreqLow, NormFreqHigh
    , LimitGain, LimitGainRate, LimitDelay, LimitDelayRate, InvertHighGain, Filter
    , GainLow, GainHigh, DelayLow, DelayHigh );
  return DataFile::WriteHeaderFields(f);
}

bool Generate::TargetFile::Load(const char* filename, bool nodata)
{ Measurements.clear();
  return DataFile::Load(filename, nodata);
}


Generate::AverageCollector::AverageCollector(double minkey)
: Sum(0)
, LastKey(NAN)
, LastValue(NAN)
, Limit(NAN)
, MinKey(log(minkey))
{}

void Generate::AverageCollector::Add(double key, double value)
{ if (isnan(value))
    return;
  key = log(key);
  if (!isnan(LastKey))
  { double limit = (key + LastKey) / 2;
    Sum += (limit - Limit) * LastValue;
    Limit = limit;
  } else
    Limit = MinKey;
  LastKey = key;
  LastValue = value;
}

double Generate::AverageCollector::Finish(double maxkey)
{ maxkey = log(maxkey);
  return (Sum + (maxkey - Limit) * LastValue) / (maxkey - MinKey);
}


Generate::TargetFile Generate::Data;
const Generate::TargetFile Generate::DefData;


Generate::Generate(const TargetFile& params)
: LocalData(params)
{}

void Generate::Run()
{
  if (LocalData.Measurements.size() == 0)
    throw "No measurements selected.";

  LocalData.clear();
  // First load all files
  foreach (Measure::MeasureFile,*const*, sp, LocalData.Measurements)
  { Measure::MeasureFile& data = **sp;
    if (!data.Load())
      ; // TODO...
    // Normalize
    // Calculate average gain and delay in the range [NormFreqLow,NormFreqHigh].
    AverageCollector lgain(LocalData.NormFreqLow);
    AverageCollector rgain(LocalData.NormFreqLow);
    AverageCollector ldelay(LocalData.NormFreqLow);
    AverageCollector rdelay(LocalData.NormFreqLow);
    unsigned pos;
    data.locate(LocalData.NormFreqLow, pos);
    DataRowType*const* rp = data.begin() + pos;
    DataRowType*const* const rpe = data.end();
    while (rp != rpe)
    { DataRowType& row = **rp;
      double f = row[Measure::MeasureFile::Frequency];
      if (f > LocalData.NormFreqHigh)
        break;
      lgain.Add(f, row[Measure::MeasureFile::LGain]);
      rgain.Add(f, row[Measure::MeasureFile::RGain]);
      ldelay.Add(f, row[Measure::MeasureFile::LDelay]);
      rdelay.Add(f, row[Measure::MeasureFile::RDelay]);
      ++rp;
    }
    double ogain = 2 / (lgain.Finish(LocalData.NormFreqHigh) + rgain.Finish(LocalData.NormFreqHigh));
    double odelay = (ldelay.Finish(LocalData.NormFreqHigh) + rdelay.Finish(LocalData.NormFreqHigh)) / -2;
    // Apply offset
    rp = data.begin();
    while (rp != rpe)
    { DataRowType& row = **rp;
      row[Measure::MeasureFile::LGain] *= ogain;
      row[Measure::MeasureFile::RGain] *= ogain;
      row[Measure::MeasureFile::LDelay] += odelay;
      row[Measure::MeasureFile::RDelay] += odelay;
      ++rp;
    }
  }

  // now calculate the target response
  double f = LocalData.FreqLow;
  const double scale = 1. / LocalData.Measurements.size();
  while (f <= LocalData.FreqHigh)
  {
    DataRowType& row = *(LocalData.append() = new DataRowType(5));
    row[Generate::TargetFile::Frequency] = f;
    double& lgain = row[Generate::TargetFile::LGain] = 0;
    double& rgain = row[Generate::TargetFile::RGain] = 0;
    double& ldelay = row[Generate::TargetFile::LDelay] = 0;
    double& rdelay = row[Generate::TargetFile::RDelay] = 0;
    foreach (Measure::MeasureFile,*const*, sp, LocalData.Measurements)
    { Measure::MeasureFile& data = **sp;
      lgain += data.Interpolate(f, Measure::MeasureFile::LGain);
      rgain += data.Interpolate(f, Measure::MeasureFile::RGain);
      ldelay += data.Interpolate(f, Measure::MeasureFile::LDelay);
      rdelay += data.Interpolate(f, Measure::MeasureFile::RDelay);
    }
    lgain = ApplyGainLimit(1 / (lgain * scale));
    rgain = ApplyGainLimit(1 / (rgain * scale));
    ldelay = ApplyDelayLimit(-ldelay * scale);
    rdelay = ApplyDelayLimit(-rdelay * scale);

    // next frequency
    f += LocalData.FreqBin + f * LocalData.FreqFactor;
  }

  // TODO: limit slew rate

}

double Generate::ApplyGainLimit(double gain)
{ if (gain > LocalData.LimitGain)
  { if (LocalData.InvertHighGain)
      gain = LocalData.LimitGain - gain;
    else
      gain = LocalData.LimitGain;
  } else if (gain < 1E-6) // avoid unreasonable values
    gain = 1E-6;
  return gain;
}

double Generate::ApplyDelayLimit(double delay)
{ if (delay > LocalData.LimitDelay)
    delay = LocalData.LimitDelay;
  else if (delay < -LocalData.LimitDelay)
    delay = -LocalData.LimitDelay;
  return delay;
}
