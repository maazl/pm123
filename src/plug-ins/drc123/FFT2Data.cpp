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

#include "FFT2Data.h"
#include <cpp/algorithm.h>
#include <debuglog.h>


void FFT2Data::StoreFFT(unsigned col, const FreqDomainData& source)
{ DEBUGLOG(("FFT2Data(%p)::StoreFFT(%u, {%u,%p})\n", this, col, source.size(), source.get()));
  double lastf = 0;
  double lastph = NAN;

  double fstart = NAN;
  double fsum = 0;
  double magsum = 0;
  unsigned magcnt = 0;
  double phsum = 0;
  double delayfstart = NAN;

  DataFile::StoreIterator storer(Target, col, 2);

  for (unsigned i = 1; i < source.size(); ++i)
  { fftwf_complex sv(source[i]);
    double mag = abs(sv);
    // Some frequencies might have no reference signal.
    // In this case the quotient response/reference turns into INF or NAN.
    // We ignore these frequencies.
    if (isinf(mag) || isnan(mag))
      continue;
    double f = FInc * i;
    if (f > fstart * FBin)
    { // reached bin size => store result
      storer.Store(fsum / magcnt, magsum / magcnt * Scale, phsum / (lastf-delayfstart));
      fsum = 0;
      magsum = 0;
      magcnt = 0;
      phsum = 0;
      delayfstart = lastf;
      goto next;
    } else if (isnan(fstart))
     next:
      fstart = f;
    fsum += f;
    DEBUGLOG2(("FFT2Data::StoreFFT - %f, %u\n", fsum, magcnt));
    // magnitude
    magsum += mag;
    ++magcnt;
    // phase
    double ph = arg(sv) / (-2*M_PI) - f * Delay; // for some reason there is a negative sign required
    if (!isnan(lastph))
    { // create minimum phase
      double dphi = ph - lastph;
      dphi -= floor(dphi + .5);
      if (fabs(dphi) > .25)
        ++IndeterminatePhase;
      phsum += dphi;
      ++PhaseUnwrapCount;
    } else
      delayfstart = f;
    lastf = f;
    lastph = ph;
  }
  storer.Store(fsum / magcnt, magsum / magcnt * Scale, phsum / (lastf-delayfstart));
  DEBUGLOG(("FFT2Data::StoreFFT: IndeterminatePhase: %u/%u\n", IndeterminatePhase, PhaseUnwrapCount));
}
void FFT2Data::StoreHDN(unsigned col, const FreqDomainData& source, const FreqDomainData& ref)
{ DEBUGLOG(("FFT2Data(%p)::StoreHDN(%u, {%u,%p}, {%u,%p})\n", this, col, source.size(), source.get(), ref.size(), ref.get()));

  double fstart = NAN;
  double fsum = 0;
  double magsum = 0;
  unsigned magcnt = 0;

  double rvsum = 0;
  double lastrv = NAN;
  unsigned rvcnt = 0;

  DataFile::StoreIterator storer(Target, col, 1);

  for (unsigned i = 1; i < source.size(); ++i)
  { const fftwf_complex& rv = ref[i];
    if (rv.real() || rv.imag()) // if (rv != 0)
    { lastrv = abs(rv);
      if (magcnt)
      { rvsum += lastrv * 2;
        rvcnt += 2;
      } else
      { rvsum = lastrv;
        rvcnt = 1;
      }
      continue;
    } else if (isnan(lastrv))
      continue; // do not extrapolate frequency

    double f = FInc * i;
    if (f > fstart * FBin)
    { // reached bin size => store result
      // find next frequency with intensity, if any
      bool last = true;
      for (unsigned j = i + 1; j < source.size(); ++j)
      { const fftwf_complex& rv = ref[j];
        if (rv.real() || rv.imag()) // if (rv != 0)
        { rvsum += abs(rv);
          ++rvcnt;
          last = false;
          break;
        }
      }

      storer.Store(fsum / magcnt, magsum / magcnt / (rvsum / rvcnt));
      if (last)
        return;
      fsum = 0;
      magsum = 0;
      magcnt = 0;
      rvsum = lastrv;
      rvcnt = 1;
      goto next;
    } else if (isnan(fstart))
     next:
      fstart = f;
    fsum += f;
    // magnitude
    magsum += abs(source[i]) * Scale;
    ++magcnt;
  }
  storer.Store(fsum / magcnt, magsum / magcnt);
}


float GainInterpolationIterator::FetchNext(float key)
{ double gain;
  float lk = (**Left)[0];
  // extrapolate before the start or after the end?
  if (key <= lk || !Right)
  { gain = (**Left)[Column];
  } else
  { float lv = (**Left)[Column];
    float rk = (**Right)[0];
    float rv = (**Right)[Column];
    // Integral over [LastKey,lk]
    if (LastKey < lk)
      gain = lv * (lk-LastKey);
    else
      gain = (rk*lv - .5*lk*(lv+rv) + .5*LastKey*(rv-lv)) / (rk-lk) * (lk-LastKey);
    // seek for next row with row[0] > key
    for (;;)
    { if (key < rk)
      { // add integral over [lk,key]
        gain += (rk*lv - .5*lk*(lv+rv) + .5*key*(rv-lv)) / (rk-lk) * (key-lk);
        break;
      }
      // add integral over [lk,rk]
      gain += .5 * (lv+rv) * (rk-lk);
      // next value
      Right = SkipNAN((Left = Right) + 1);
      if (!Right)
      { // extrapolate remaining part
        gain += lv * (key-lk);
        break;
      }
      // update lk, rk, lv, rv
      lk = rk;
      lv = rv;
      rk = (**Right)[0];
      rv = (**Right)[Column];
    }
    gain /= key-LastKey;
  }
  LastKey = key;
  return gain;
}

float DelayInterpolationIterator::FetchNext(float key)
{ double delay;
  float lk = (**Left)[0];
  // extrapolate before the start or after the end?
  if (key <= lk || !Right)
  { delay = (**Left)[Column];
  } else
  { float rk = (**Right)[0];
    // Integral over [LastKey,lk]
    delay = (**(LastKey < lk ? Left : Right))[Column] * (lk-LastKey);
    for (;;)
    { if (key < rk)
      { // add integral over [Left,key]
        delay += (**Right)[Column] * (key-lk);
        break;
      }
      // add integral over [Left,Right]
      delay += (**Right)[Column] * (rk-lk);
      // next value
      Right = SkipNAN((Left = Right) + 1);
      if (!Right)
      { // extrapolate remaining part
        delay += (**Left)[Column] * (key-lk);
        break;
      }
      // update lk, rk
      lk = rk;
      rk = (**Right)[0];
    }
    delay /= key-LastKey;
  }
  LastKey = key;
  return delay;
}

void Data2FFT::LoadFFT(unsigned col, FreqDomainData& target)
{
  if (target.size() != TargetSize)
    target.reset(TargetSize);
  GainInterpolationIterator ipmag(Source, col);
  DelayInterpolationIterator ipdelay(Source, col + 1);
  double delay = 0;
  for (unsigned i = 0; i < target.size(); ++i)
  { double f = i * FInc;
    float mag = ipmag.FetchNext(f) * Scale;
    delay += ipdelay.FetchNext(f);
    double ph = -2 * M_PI * FInc * delay;
    target[i] = fftwf_complex(mag * cos(ph), mag * sin(ph));
  }
  target[TargetSize-1] = target[TargetSize-1].real();
}

void Data2FFT::LoadIdentity(FreqDomainData& target)
{
  if (target.size() != TargetSize)
    target.reset(TargetSize);
  for (unsigned i = 0; i < target.size(); ++i)
    target[i] = 1.;
}
