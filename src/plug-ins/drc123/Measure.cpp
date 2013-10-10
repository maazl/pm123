/*
 * Copyright 2013 Marcel Mueller
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

#include "Measure.h"


Measure::MeasureFile::MeasureFile()
: DataFile(9)
{ // Set Default Parameters
  FFTSize = 131072;
  DiscardSamp = 65536;
  RefFMin = 20.;
  RefFMax = 20000.;
  RefExponent = -.5;
  RefSkipEven = true;
  RefMode = RFM_STEREO;
  AnaSwap = false;

  Volume = .8;
}

bool Measure::MeasureFile::ParseHeaderField(const char* string)
{
  if (strnicmp(string, "VOLUME=", 7) == 0)
    Volume = atof(string + 7);
  return true;
}

bool Measure::MeasureFile::WriteHeaderFields(FILE* f)
{
  fprintf(f, "##VOLUME=%f\n", Volume);
  return true;
}

Measure::MeasureFile Measure::Data;

Measure::Measure(const MeasureFile& params, FILTER_PARAMS2& filterparams)
: OpenLoop(params, filterparams)
{}

Measure* Measure::Factory(FILTER_PARAMS2& filterparams)
{ SyncAccess<MeasureFile> params(Data);
  return new Measure(params, filterparams);
}

Measure::~Measure()
{}

void Measure::ProcessFFTData(FreqDomainData (&input)[2], double scale)
{
  // TODO:
}
