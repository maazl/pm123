/*
 * Copyright 2020 Marcel Mueller
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

#ifndef AACPLAY_H
#define AACPLAY_H

#include <decoder_plug.h>


/// Base interface for decoder instances
struct DECODER_STRUCT
{protected:
  // specify a function which the decoder should use for output
  int   DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void  DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  // decoder events
  void  DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, const void* param);
  void*           A;               // only to be used with the precedent functions

  int             State;

 public:
  void*                 operator new(size_t count);
  void*                 operator new(size_t count, DECODER_STRUCT* that) { return that; }
  void                  operator delete(void* ptr) { ::operator delete[](ptr); }
                        DECODER_STRUCT() : State(DECODER_STOPPED) {}
  virtual               ~DECODER_STRUCT() {}
  virtual ULONG         DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params);
  virtual DECODERSTATE  GetStatus() const;
  virtual double        GetSonglength() const;
};

#endif // AACPLAY_H
