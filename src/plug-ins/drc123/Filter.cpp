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

#include "Filter.h"


Filter::FILTER_STRUCT(FILTER_PARAMS2& params)
{ DEBUGLOG(("Filter(%p)::FILTER_STRUCT()\n", this));
  OutCommand       = params.output_command;
  OutRequestBuffer = params.output_request_buffer;
  OutCommitBuffer  = params.output_commit_buffer;
  OutA             = params.a;

  params.output_command        = &PROXYFUNCREF(Filter)InCommandProxy;
  params.output_request_buffer = &PROXYFUNCREF(Filter)InRequestBufferProxy;
  params.output_commit_buffer  = &PROXYFUNCREF(Filter)InCommitBufferProxy;
}

void Filter::Update(const FILTER_PARAMS2& params)
{ DEBUGLOG(("Filter(%p)::Update()\n", this));
  DosEnterCritSec();
  OutCommand       = params.output_command;
  OutRequestBuffer = params.output_request_buffer;
  OutCommitBuffer  = params.output_commit_buffer;
  OutA             = params.a;
  DosExitCritSec();
}

Filter::~FILTER_STRUCT()
{ DEBUGLOG(("Filter(%p)::~FILTER_STRUCT()\n", this));
}

PROXYFUNCIMP(ULONG DLLENTRY, Filter)InCommandProxy(Filter* a, ULONG msg, OUTPUT_PARAMS2* info)
{ return a->InCommand(msg, info);
}

PROXYFUNCIMP(int DLLENTRY, Filter)InRequestBufferProxy(Filter* a, const FORMAT_INFO2* format, float** buf)
{ return a->InRequestBuffer(format, buf);
}

PROXYFUNCIMP(void DLLENTRY, Filter)InCommitBufferProxy(Filter* a, int len, PM123_TIME pos)
{ a->InCommitBuffer(len, pos);
}
