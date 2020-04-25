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

#ifndef AACDECODER_H
#define AACDECODER_H

#include <neaacdec.h>
#include <xio.h>
#include <decoder_plug.h>
#include "id3v2/id3v2.h"

class AacDecoder
{protected:
  NeAACDecHandle FAAD;
  ID3V2_TAG*    Tag;
  int           Samplerate;
  int           Channels;
  unsigned      Offset;

 public:
  /// Check whether stream could be decoded by this class.
  /// @return Detected file format or NULL if not.
  static const char* Sniffer(XFILE* stream);

  AacDecoder();
  ~AacDecoder();

  const char*   Open(XFILE* stream);

  int           GetChannels() const   { return Channels; }
  int           GetSamplerate() const { return Samplerate; }
  unsigned      GetOffset() const     { return Offset; }

  /// Retrieve ID3V2 tag information if any
  /// @return Song length in seconds or -1 if unknown
  double        GetMeta(META_INFO& meta);
};

#endif // AACDECODER_H
