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

#ifndef FILTER_H
#define FILTER_H

#include <filter_plug.h>


typedef struct FILTER_STRUCT Filter;
struct FILTER_STRUCT
{public:
  /// Working directory with all the measurement files.
  /// The path MUST end with a trailing backslash.
  static volatile xstring WorkDir;
  /// Current operating mode for new instances.
  static enum FilterMode
  { MODE_DECONVOLUTION,
    MODE_MEASURE,
    MODE_CALIBRATE
  } CurrentMode;

 protected:
  ULONG  DLLENTRYP(OutCommand)(Filter* a, ULONG msg, const OUTPUT_PARAMS2* info);
  int    DLLENTRYP(OutRequestBuffer)(Filter* a, const FORMAT_INFO2* format, float** buf);
  void   DLLENTRYP(OutCommitBuffer)(Filter* a, int len, PM123_TIME pos);
  Filter* OutA; /// only to be used with the precedent functions

 private:
  PROXYFUNCDEF ULONG DLLENTRY InCommandProxy(Filter* a, ULONG msg, const OUTPUT_PARAMS2* info);
  PROXYFUNCDEF int   DLLENTRY InRequestBufferProxy(Filter* a, const FORMAT_INFO2* format, float** buf);
  PROXYFUNCDEF void  DLLENTRY InCommitBufferProxy(Filter* a, int len, PM123_TIME pos);
 protected:
  virtual ULONG InCommand(ULONG msg, const OUTPUT_PARAMS2* info) = 0;
  /// Entry point: do filtering.
  virtual int   InRequestBuffer(const FORMAT_INFO2* format, float** buf) = 0;
  virtual void  InCommitBuffer(int len, PM123_TIME pos) = 0;
 private: // non-copyable
  FILTER_STRUCT(const FILTER_STRUCT&);
  void operator=(const FILTER_STRUCT&);
 protected:
  FILTER_STRUCT(FILTER_PARAMS2& params);
 public: // plug-in API interface
  static Filter* Factory(FILTER_PARAMS2& params);
  virtual void  Update(const FILTER_PARAMS2& params);
  virtual       ~FILTER_STRUCT();
};


#endif
