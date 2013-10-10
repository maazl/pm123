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
{}

static int dlcmp(const double* k, const DataRowType* row)
{ double value = (*row)[0];
  if (*k > value * 1.001)
    return 1;
  if (*k < value / 1.001)
    return -1;
  return 0;
}

void FFT2Data::StoreValue(unsigned col, double f, double mag, double delay)
{ size_t pos;
  DataRowType* rp;
  if (!binary_search<const DataRowType,const double>(&f, pos, Target.begin(), Target.size(), &dlcmp))
  { rp = Target.insert(pos) = new DataRowType(Target.columns());
    (*rp)[0] = f;
  } else
    rp = Target[pos];
  (*rp)[col] = mag;
  (*rp)[col + 1] = delay;
}

void FFT2Data::StoreFFT(unsigned col, const FreqDomainData& source, const FreqDomainData& ref)
{ DEBUGLOG(("FFT2Data(%p)::StoreFFT(%u, {%u,%p}, {%u,%p})\n", this, col, source.size(), source.get(), ref.size(), ref.get()));

  IndeterminatePhase = 0;

  /*/ calc average noise RMS intensity
  double noise = 0;
  unsigned noisecnt = 0;
  for (unsigned i = 1; i < source.size(); ++i)
  { const fftwf_complex& rv = ref[i];
    if (!rv[0] && !rv[1])
    { const fftwf_complex& sv = source[i];
      noise += sv[0] * sv[0] + sv[1] * sv[1];
      ++noisecnt;
    }
  }
  noise = sqrt(noise / noisecnt);*/

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
    if (!rv.real() && ! rv.imag()) // if (rv == 0)
      continue;

    double f = FInc * i;
    if (f > fstart * FBin)
    { // reached bin size => store result
      StoreValue(col, fsum / magcnt, magsum / magcnt, delaysum / delaycnt);
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
    // magnitude
    fftwf_complex sv = source[i] / rv;
    magsum += abs(sv) * Scale;
    ++magcnt;
    // phase
    double ph = arg(sv) + 2.*M_PI * f * Delay;
    if (!isnan(lastph))
    { // create minimum phase
      double dphi = ph - lastph;
      dphi -= floor(dphi / (2.*M_PI) + .5) * (2.*M_PI);
      if (fabs(dphi) > M_PI/2.)
        ++IndeterminatePhase;
      else
      { delaysum += dphi / (f - lastf);
        ++delaycnt;
      }
    }

    lastf = f;
    lastph = ph;
  }
  StoreValue(col, fsum / magcnt, magsum / magcnt, delaysum / delaycnt);
  DEBUGLOG(("FFT2Data::StoreFFT: IndeterminatePhase: %u\n", IndeterminatePhase));
}
