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

#ifndef FFT2DATA_H
#define FFT2DATA_H

#include "DataFile.h"
#include "DataVector.h"


/** Helper class to convert FFT results into DataFile content.
 */
class FFT2Data
{public: // parameters
  /// DataFile where to store the results.
  DataFile& Target;
  /// Width of the bins in the FFT data in the frequency domain.
  /// In general samplerate / fftsize.
  double    FInc;
  /// Size of the frequency bins in the FFT data. This is equal to the inverse FFT length in the time domain.
  double    FBin;
  /// Multiply all values by Scale.
  double    Scale;
  /// Use this linear phase delay in seconds for phase unwrapping.
  double    Delay;
 public: // results
  /// Number of phase unwraps.
  unsigned  PhaseUnwrapCount;
  /// Number of times the phase unwrapping was ambiguous.
  unsigned  IndeterminatePhase;
 public:
  /// Create instance for a target.
  /// @param target The \c DataFile where to store the results.
  /// The DataFile might be initial. The required lines are created on demand.
  /// @param finc Width of the bins in the FFT data in the frequency domain.
  /// In general samplerate / fftsize.
  /// @param fbin Size of the frequency bins in the FFT data. This is equal to the inverse FFT length in the time domain.
  FFT2Data(DataFile& target, double finc, double fbin)
  : Target(target), FInc(finc), FBin(fbin), Scale(0.), Delay(0.), PhaseUnwrapCount(0), IndeterminatePhase(0) {}

  void ResetPhaseCount() { PhaseUnwrapCount = 0; IndeterminatePhase = 0; }

  /// Store the data in \a source to the target specified at construction.
  /// @param col Start column where the data is to be stored.
  /// All together two columns are written. One at index \a col with magnitude
  /// and one at \a col + 1 with group delay.
  /// @param source Frequency response to store.
  /// @pre target.Columns() > col + 1
  void StoreFFT(unsigned col, const FreqDomainData& source);
  /// Store harmonic distortion and noise.
  /// @param col Column where the data is to be stored.
  /// @param source Frequency response to store.
  /// @param ref Reference signal.
  /// Only bins where the magnitude of the reference is zero (!) are used.
  /// The reference amplitude is interpolated from the neighbors.
  /// @param finc Width of the bins in the FFT data in the frequency domain.
  /// In general samplerate / fftsize.
  /// @param scale Multiply all values by \a scale.
  /// @pre target.Columns() > col + 1
  void StoreHDN(unsigned col, const FreqDomainData& source, const FreqDomainData& ref);
};

class GainInterpolationIterator : public DataFile::InterpolationIterator
{protected:
  float     LastKey;
 public:
  GainInterpolationIterator(const DataFile& data, unsigned col)
  : InterpolationIterator(data, col), LastKey(0) {}
  float FetchNext(float key);
};
class DelayInterpolationIterator : public DataFile::InterpolationIterator
{protected:
  float     LastKey;
 public:
  DelayInterpolationIterator(const DataFile& data, unsigned col)
  : InterpolationIterator(data, col), LastKey(0) {}
  float FetchNext(float key);
};

class Data2FFT
{public:
  const DataFile& Source;
  unsigned  TargetSize;
  double    FInc;
  double    Scale;
 public:
  Data2FFT(const DataFile& source, unsigned size, double finc, double scale = 1.)
  : Source(source), TargetSize(size/2+1), FInc(finc), Scale(scale) {}
  void LoadFFT(unsigned col, FreqDomainData& target);
  void LoadIdentity(FreqDomainData& target);
};

#endif // FFT2DATA_H
