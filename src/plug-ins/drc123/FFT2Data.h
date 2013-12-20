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
  DataFile& Target;
  double    FInc;
  double    FBin;
  float     Scale;
  double    Delay;
 public: // results
  unsigned  IndeterminatePhase;
 private:
  void  StoreValue(unsigned col, double f, double mag, double delay);
 public:
  /// Create instance for a target.
  /// @param target The \c DataFile where to store the result.
  /// The DataFile might be initial. The required lines are created on demand.
  /// @param finc Width of the bins in the FFT data in the frequency domain.
  /// In general samplerate / fftsize.
  /// @param fbin Size of the frequency bins in the FFT data. This is equal to the inverse FFT length in the time domain.
  /// @param scale Multiply all values by \a scale.
  /// @param delay Use this linear phase delay in seconds for phase unwrapping.
  FFT2Data(DataFile& target, double finc, double fbin, double scale = 1., double delay = 0.);

  /// Store the data in \a source to the target specified at construction.
  /// @param col Start column where the data is to be stored.
  /// All together two columns are written. One at index \a col with magnitude
  /// and one at \a col + 1 with group delay.
  /// @param source Frequency response to store.
  /// @param ref Reference signal.
  /// Only bins where the magnitude of the reference is > 0 are used.
  /// @pre target.Columns() > col + 1
  void StoreFFT(unsigned col, const FreqDomainData& source, const FreqDomainData& ref);
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

#endif // FFT2DATA_H
