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


FFT2Data::FFT2Data(DataFile& target, double finc, double fbin, double scale, double delay)
: Target(target)
, FInc(finc)
, FBin(fbin)
, Scale(scale)
, Delay(delay)
, IndeterminatePhase(0)
{}

static int dlcmp(const double& k, const DataRowType& row)
{ double value = row[0];
  if (k > value * 1.0001)
    return 1;
  if (k < value / 1.0001)
    return -1;
  return 0;
}

void FFT2Data::StoreValue(unsigned col, double f, double mag, double delay)
{ size_t pos;
  DataRowType* rp;
  if (!binary_search<DataRowType,const double>(f, pos, Target, &dlcmp))
  { rp = Target.insert(pos) = new DataRowType(Target.columns());
    (*rp)[0] = f;
  } else
    rp = Target[pos];
  (*rp)[col] = mag;
  if (!isnan(delay))
    (*rp)[col + 1] = delay;
}

void FFT2Data::StoreFFT(unsigned col, const FreqDomainData& source, const FreqDomainData& ref)
{ DEBUGLOG(("FFT2Data(%p)::StoreFFT(%u, {%u,%p}, {%u,%p})\n", this, col, source.size(), source.get(), ref.size(), ref.get()));

  double lastf = 0;
  double lastph = NAN;

  double fstart = NAN;
  double fsum = 0;
  double magsum = 0;
  unsigned magcnt = 0;
  double delaysum = 0;
  unsigned delaycnt = 0;

  for (unsigned i = 1; i < source.size(); ++i)
  { const fftwf_complex& rv = ref[i];
    if (!rv.real() && !rv.imag()) // if (rv == 0)
      continue;

    double f = FInc * i;
    if (f > fstart * FBin)
    { // reached bin size => store result
      StoreValue(col, fsum / magcnt, magsum / magcnt * Scale, delaysum / delaycnt);
      fsum = 0;
      magsum = 0;
      magcnt = 0;
      delaysum = 0;
      delaycnt = 0;
      goto next;
    } else if (isnan(fstart))
     next:
      fstart = f;
    fsum += f;
    DEBUGLOG2(("FFT2Data::StoreFFT - %f, %u\n", fsum, magcnt));
    // magnitude
    fftwf_complex sv(source[i] / rv);
    magsum += abs(sv);
    ++magcnt;
    // phase
    double ph = arg(sv) / (-2*M_PI) - f * Delay; // for some reason there is a negative sign required
    if (!isnan(lastph))
    { // create minimum phase
      double dphi = ph - lastph;
      dphi -= floor(dphi + .5);
      if (fabs(dphi) > .25)
        ++IndeterminatePhase;
      else
      { delaysum += dphi / (f - lastf);
        ++delaycnt;
      }
    }

    lastf = f;
    lastph = ph;
  }
  StoreValue(col, fsum / magcnt, magsum / magcnt * Scale, delaysum / delaycnt);
  DEBUGLOG(("FFT2Data::StoreFFT: IndeterminatePhase: %u\n", IndeterminatePhase));
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

  for (unsigned i = 1; i < source.size(); ++i)
  { fftwf_complex& rv = ref[i];
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
      { rv = ref[j];
        if (rv.real() || rv.imag()) // if (rv != 0)
        { rvsum += abs(rv);
          ++rvcnt;
          last = false;
          break;
        }
      }

      StoreValue(col, fsum / magcnt, magsum / magcnt / (rvsum / rvcnt), NAN);
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
  StoreValue(col, fsum / magcnt, magsum / magcnt, NAN);
}
