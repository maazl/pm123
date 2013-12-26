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


template <class T>
struct DataVector
: public sco_arr<T>
{public:
  DataVector()                  {}
  DataVector(size_t fftlen)     : sco_arr<T>(fftlen) {}
  virtual ~DataVector();
  void clear()                  { memset(begin(), 0, size() * sizeof(*get())); }
  DataVector& operator=(const DataVector& r);
  DataVector& operator+=(const DataVector& r);
  DataVector& operator*=(const DataVector& r);
  DataVector& operator/=(const DataVector& r);
  DataVector& Invert();
};

template <class T>
DataVector<T>::~DataVector()
{}

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

typedef DataVector<float> TimeDomainData;
typedef DataVector<fftwf_complex> FreqDomainData;

/*struct FreqDomainData : public DataVector<fftwf_complex>
{
  FreqDomainData()              {}
  FreqDomainData(size_t fftlen) : DataVector<fftwf_complex>(fftlen) {}
  void CrossCorrelate(const FreqDomainData& r);
};*/


#endif // DATAVECTOR_H
