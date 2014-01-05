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


void Data2FFT::LoadFFT(unsigned col, FreqDomainData& target)
{
  if (target.size() != TargetSize)
    target.reset(TargetSize);
  DataFile::InterpolationIterator ipmag(Source, col);
  DataFile::InterpolationIterator ipdelay(Source, col + 1);
  double ph = 0;
  for (unsigned i = 0; i < target.size(); ++i)
  { double f = i * FInc;
    float mag = ipmag.Next(f) * Scale;
    double delay = ipdelay.Next(f);
    target[i] = fftwf_complex(mag * cos(ph), mag * sin(ph));
    ph += -2 * M_PI * FInc * delay;
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
