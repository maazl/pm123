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
#include "Iterators.h"
#include <cpp/smartptr.h>


static inline double sqr(double value)
{ return value * value;
}


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
  NormMode = r.NormMode;
  LimitGain = r.LimitGain;
  LimitDelay = r.LimitDelay;
  GainSmoothing = r.GainSmoothing;
  DelaySmoothing = r.DelaySmoothing;
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
  ::swap(NormMode, r.NormMode);
  ::swap(LimitGain, r.LimitGain);
  ::swap(LimitDelay, r.LimitDelay);
  ::swap(GainSmoothing, r.GainSmoothing);
  ::swap(DelaySmoothing, r.DelaySmoothing);
  ::swap(InvertHighGain, r.InvertHighGain);
  ::swap(GainLow,  r.GainLow);
  ::swap(GainHigh, r.GainHigh);
  ::swap(DelayLow, r.DelayLow);
  ::swap(DelayHigh, r.DelayHigh);
}


Generate::TargetFile::TargetFile()
: DataFile(ColCount)
{ FreqLow  = 20.;
  FreqHigh = 20000.;
  FreqBin  = .2;
  FreqFactor = .01;
  NormFreqLow = 50.;
  NormFreqHigh = 500.;
  NormMode = NM_Energy;
  LimitGain = pow(10, 15./20); // +15 dB
  LimitDelay = .2;
  GainSmoothing = .2;
  DelaySmoothing = .2;
  InvertHighGain = true;
  GainLow  = -20.;
  GainHigh = +20.;
  DelayLow = -.2;
  DelayHigh = +.2;
}


void Generate::TargetFile::swap(TargetFile& r)
{ DataFile::swap(r);
  Parameters::swap(r);
}

bool Generate::TargetFile::ParseHeaderField(const char* string)
{ if (TryParam(string, "Measurement"))
  { Measure::MeasureFile* file = new Measure::MeasureFile();
    file->FileName = string;
    Measurements.get(file->FileName) = file;
  } else if (TryParam(string, "Freq"))
    sscanf(string, "%lf,%lf", &FreqLow, &FreqHigh);
  else if (TryParam(string, "FreqBin"))
    sscanf(string, "%lf,%lf", &FreqBin, &FreqFactor);
  else if (TryParam(string, "NormFreq"))
    sscanf(string, "%lf,%lf", &NormFreqLow, &NormFreqHigh);
  else if (TryParam(string, "LimitGain"))
    sscanf(string, "%lf,%lf", &LimitGain, &GainSmoothing);
  else if (TryParam(string, "LimitDelay"))
    sscanf(string, "%lf,%lf", &LimitDelay, &DelaySmoothing);
  else if (TryParam(string, "InvertHighGain"))
    InvertHighGain = !!atoi(string);
  else if (TryParam(string, "DispGain"))
    sscanf(string, "%lf,%lf", &GainLow, &GainHigh);
  else if (TryParam(string, "DispDelay"))
    sscanf(string, "%lf,%lf", &DelayLow, &DelayHigh);
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
    "##LimitDelay=%f,%f\n"
    "##InvertHighGain=%u\n"
    "##DispGain=%f,%f\n"
    "##DispDelay=%f,%f\n"
    , FreqLow, FreqHigh, FreqBin, FreqFactor, NormFreqLow, NormFreqHigh
    , LimitGain, GainSmoothing, LimitDelay, DelaySmoothing, InvertHighGain
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

void Generate::Prepare()
{
  if (LocalData.Measurements.size() == 0)
    throw "No measurements selected.";

  LocalData.clear();
  // First load all files
  foreach (Measure::MeasureFile,*const*, sp, LocalData.Measurements)
  { Measure::MeasureFile& data = **sp;
    if (!data.Load())
      throw xstring().sprintf("Failed to load measurement %s.\n%s", data.FileName.cdata(), strerror(errno));
    // Normalize
    // Calculate average gain and delay in the range [NormFreqLow,NormFreqHigh].
    AverageCollector lgain(LocalData.NormFreqLow);
    AverageCollector rgain(LocalData.NormFreqLow);
    AverageCollector ldelay(LocalData.NormFreqLow);
    AverageCollector rdelay(LocalData.NormFreqLow);
    unsigned pos;
    data.locate(LocalData.NormFreqLow, pos);
    DataRow*const* rp = data.begin() + pos;
    DataRow*const* const rpe = data.end();
    while (rp != rpe)
    { DataRow& row = **rp;
      double f = row[Measure::Frequency];
      if (f > LocalData.NormFreqHigh)
        break;
      switch (LocalData.NormMode)
      {default: // NM_Energy
        lgain.Add(f, sqr(1/row[Measure::LGain]));
        rgain.Add(f, sqr(1/row[Measure::RGain]));
        break;
       case NM_Logarithm:
        lgain.Add(f, log(row[Measure::LGain]));
        rgain.Add(f, log(row[Measure::RGain]));
        break;
      }
      ldelay.Add(f, row[Measure::LDelay]);
      rdelay.Add(f, row[Measure::RDelay]);
      ++rp;
    }
    double ogain;
    switch (LocalData.NormMode)
    {default: // NM_Energy
      ogain = sqrt((lgain.Finish(LocalData.NormFreqHigh) + rgain.Finish(LocalData.NormFreqHigh)) / 2);
      break;
     case NM_Logarithm:
      ogain = exp((lgain.Finish(LocalData.NormFreqHigh) + rgain.Finish(LocalData.NormFreqHigh)) / -2);
      break;
    }
    double odelay = (ldelay.Finish(LocalData.NormFreqHigh) + rdelay.Finish(LocalData.NormFreqHigh)) / -2;
    // Apply offset
    rp = data.begin();
    while (rp != rpe)
    { DataRow& row = **rp;
      row[Measure::LGain] *= ogain;
      row[Measure::RGain] *= ogain;
      row[Measure::LDelay] += odelay;
      row[Measure::RDelay] += odelay;
      ++rp;
    }
  }
}

struct SumCollector
{ double Sum;
  double Low;
  double High;
  void   Reset()                    { Sum = 0, Low = NAN, High = NAN; }
         SumCollector()             { Reset(); }
  void   operator+=(double value);
};

void SumCollector::operator+=(double value)
{ Sum += value;
  if (!(Low <= value))
    Low = value;
  if (!(High >= value))
    High = value;
}

struct Interpolator
{ AverageIterator LGain;
  AverageIterator RGain;
  DelayIterator LDelay;
  DelayIterator RDelay;
  bool Valid;
  Interpolator(const Measure::MeasureFile& data);
  void FetchNext(double f);
};

Interpolator::Interpolator(const Measure::MeasureFile& data)
: LGain(Measure::LGain)
, RGain(Measure::RGain)
, LDelay(Measure::LDelay)
, RDelay(Measure::RDelay)
{ Valid = LGain.Reset(data)
       && RGain.Reset(data)
       && LDelay.Reset(data)
       && RDelay.Reset(data);
}

inline void Interpolator::FetchNext(double f)
{ LGain.ReadNext(f);
  RGain.ReadNext(f);
  LDelay.ReadNext(f);
  RDelay.ReadNext(f);
}

void Generate::Run()
{
  Prepare();

  // now calculate the target response
  vector_own<Interpolator> iplist(LocalData.Measurements.size());
  foreach (Measure::MeasureFile,*const*, sp, LocalData.Measurements)
  { Interpolator* ip = new Interpolator(**sp);
    if (ip->Valid)
      iplist.append() = ip;
  }

  double f = LocalData.FreqLow;
  const double scale = 1. / LocalData.Measurements.size();
  while (f <= LocalData.FreqHigh)
  {
    DataRow& row = *(LocalData.append() = new (LocalData.columns())DataRow);
    row[Frequency] = f;
    SumCollector lgain;
    SumCollector rgain;
    SumCollector ldelay;
    SumCollector rdelay;
    foreach (Interpolator,*const*, ipp, iplist)
    { Interpolator& ip = **ipp;
      ip.FetchNext(f);
      lgain += ip.LGain.GetValue();
      rgain += ip.RGain.GetValue();
      ldelay += ip.LDelay.GetValue();
      rdelay += ip.RDelay.GetValue();
    }
    row[LGain] = ApplyGainLimit(1 / (lgain.Sum * scale));
    row[RGain] = ApplyGainLimit(1 / (rgain.Sum * scale));
    row[LDelay] = ApplyDelayLimit(-ldelay.Sum * scale);
    row[RDelay] = ApplyDelayLimit(-rdelay.Sum * scale);
    row[LGainLow] = 1 / lgain.High;
    row[LGainHigh] = 1 / lgain.Low;
    row[RGainLow] = 1 / rgain.High;
    row[RGainHigh] = 1 / rgain.Low;
    row[LDelayLow] = -ldelay.High;
    row[LDelayHigh] = -ldelay.Low;
    row[RDelayLow] = -rdelay.High;
    row[RDelayHigh] = -rdelay.Low;

    // next frequency
    f += LocalData.FreqBin + f * LocalData.FreqFactor;
  }

  // limit slew rate
  ApplySmoothing(LGain, LocalData.GainSmoothing);
  ApplySmoothing(RGain, LocalData.GainSmoothing);
  ApplySmoothing(LDelay, LocalData.DelaySmoothing);
  ApplySmoothing(RDelay, LocalData.DelaySmoothing);
}

void Generate::ApplySmoothing(unsigned col, double width)
{
  // Calc filter coefficients
  width = -1 / width * LocalData.FreqBin;
  if (width >= 0)
    return;
  const double b1 = exp(width);
  const double a0 = 1. - b1;

  // Apply IIR filter forward
  double acc = 0;
  foreach (DataRow,*const*, rpp, LocalData)
  { float& val = (**rpp)[col];
    val = acc = a0 * val + b1 * acc;
  }
  // Apply IIR filter backward
  acc = 0;
  DataRow*const* rpp = LocalData.end();
  while (rpp != LocalData.begin())
  { float& val = (**--rpp)[col];
    val = acc = a0 * val + b1 * acc;
  }

  /*sco_arr<float> differential(LocalData.size());
  sco_arr<signed char> except(LocalData.size());
  memset(except.begin(), 0, LocalData.size() * sizeof *except.get());
  float* dp = differential.begin();
  signed char* xp = except.begin();
  float last = 0;
  float lastf = 0;
  foreach (DataRow,*const*, sp, LocalData)
  { float freq = (**sp)[0];
    float value = (**sp)[col];
    float diff = (value - last) / (freq - lastf);
    *dp++ = diff;
    if (diff > rate)
      *xp = 1;
    else if (diff < -rate)
      *xp = -1;
    lastf = freq;
    last = value;
    ++xp;
  }
  dp = differential.begin();
  float* dpe = differential.end();
  for (; dp != dpe; ++dp)
  { double over = fabs(*dp) - rate;
    if (over > 0)
      ;
  }*/
}

double Generate::ApplyGainLimit(double gain)
{ if (gain > LocalData.LimitGain)
  { if (LocalData.InvertHighGain)
      gain = LocalData.LimitGain * LocalData.LimitGain / gain;
    else
      gain = LocalData.LimitGain;
  }
  if (gain < 1E-6) // avoid unreasonable values
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
