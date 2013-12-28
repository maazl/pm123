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

#ifndef DATAVECTOR_H
#define DATAVECTOR_H

#include <cpp/smartptr.h>
#include <fftw3.h>
#include <string.h>
#include <debuglog.h>


/** class to hold a vector of numeric data. Extends sco_arr<T>.
 * @tparam T element type. Must be numeric, i.e. provide basic arithmetic operators
 * like + - * /. Furthermore binary 0x00 must be a reasonable zero value.
 * This applies to all built in integral and floating point types as well as to
 * \c class \c complex<>, although the latter is strictly speaking undefined behavior. */
template <class T>
struct DataVector
: public sco_arr<T>
{public:
  /// Create Vector of length 0.
  DataVector()                  {}
  /// Create vector of length len.
  DataVector(size_t len)     : sco_arr<T>(len) {}
  /// Set all elements to 0. This does not change the number of elements.
  void Set0()                  { memset(begin(), 0, size() * sizeof(*get())); }
  /// Assignment.
  DataVector& operator=(const DataVector& r);
  /// Add two vectors element by element.
  /// @pre Both vectors must be of the same length.
  DataVector& operator+=(const DataVector& r);
  /// Multiply two vectors element by element.
  /// @pre Both vectors must be of the same length.
  DataVector& operator*=(const DataVector& r);
  /// Divide two vectors element by element.
  /// @pre Both vectors must be of the same length.
  DataVector& operator/=(const DataVector& r);
  /// Calculate the reciprocal of each element.
  DataVector& Invert();
};

template <class T>
DataVector<T>& DataVector<T>::operator=(const DataVector<T>& r)
{ if (size() != r.size())
    reset(r.size());
  const T* sp = r.begin();
  foreach(T,*, dp, *this)
    *dp = *sp++;
  return *this;
}

template <class T>
DataVector<T>& DataVector<T>::operator+=(const DataVector<T>& r)
{ ASSERT(size() == r.size());
  const T* sp = r.begin();
  foreach(T,*, dp, *this)
    *dp += *sp++;
  return *this;
}

template <class T>
DataVector<T>& DataVector<T>::operator*=(const DataVector<T>& r)
{ ASSERT(size() == r.size());
  const T* sp = r.begin();
  foreach(T,*, dp, *this)
    *dp *= *sp++;
  return *this;
}

template <class T>
DataVector<T>& DataVector<T>::operator/=(const DataVector<T>& r)
{ ASSERT(size() == r.size());
  const T* sp = r.begin();
  foreach(T,*, dp, *this)
    *dp /= *sp++;
  return *this;
}

template <class T>
DataVector<T>& DataVector<T>::Invert()
{ foreach(T,*, dp, *this)
    *dp = (T)1. / *dp;
  return *this;
}

/// @brief Data vector with samples in the time domain.
/// @details All elements are of type float.
typedef DataVector<float> TimeDomainData;

/// @brief Data vector of frequency domain data.
/// @details All elements are of type complex of float.
/// By convention the first and the last element are real.
/// Furthermore only the first half of the real to complex DFT data
/// are stored.
typedef DataVector<fftwf_complex> FreqDomainData;

/*struct FreqDomainData : public DataVector<fftwf_complex>
{
  FreqDomainData()              {}
  FreqDomainData(size_t fftlen) : DataVector<fftwf_complex>(fftlen) {}
  void CrossCorrelate(const FreqDomainData& r);
};*/


#endif // DATAVECTOR_H
