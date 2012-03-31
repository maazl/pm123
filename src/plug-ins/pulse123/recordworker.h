/*
 * Copyright 2010-2012 Marcel Mueller
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

#ifndef PULSE123_RECORDWORKER_H
#define PULSE123_RECORDWORKER_H

#include "pulse123.h"
#include "pawrapper.h"
#include <decoder_plug.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <charset.h>
//#include <os2.h>


class RecordWorker
{public:
  struct Params
  { xstring          Server;
    xstring          Source;
    xstring          Port;
    FORMAT_INFO2     Format;
    pa_stream_flags_t Flags;
    Params();
    xstring ParseURL(const char* url);
    xstring ToURL() const;
  };
 private:
  int  DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  void DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, void* param);
  void*              A;
  Params             Par;
 private:
  PASourceInput      Stream;
  PAContext          Context;
  bool               Terminate;
  bool               Running;
  uint64_t           SampleOffset;
  int                RecordTID;

 public:
  RecordWorker();
  ~RecordWorker();

  ULONG Setup(const DECODER_PARAMS2& params);
  ULONG Play(const xstring& url);
  ULONG Stop();

  void  Event(OUTEVENTTYPE event);
  DECODERSTATE GetState() const;
 private:
  void  RecordThread();
  PROXYFUNCDEF void TFNENTRY RecordThreadStub(void* data);
  void  Error(const char* fmt, ...);
};


#endif
