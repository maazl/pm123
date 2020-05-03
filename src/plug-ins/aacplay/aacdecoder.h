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

#include "aacplay.h"

#include <neaacdec.h>
#include <xio.h>
#include <decoder_plug.h>
#include <cpp/mutex.h>
#include <cpp/smartptr.h>
#include <cpp/container/vector.h>
#include "id3v2/id3v2.h"


class AacDecoder
{protected:
  NeAACDecHandle FAAD;
  ID3V2_TAG*    Tag;
  int           Samplerate;
  int           Channels;
  double        Songlength;
  unsigned      Offset;     ///< Offset of AAC frames from file start
  sco_arr<unsigned char> Buffer;
  unsigned      Start;      ///< Start of unconsumed data within Buffer
  unsigned      End;        ///< End of valid data within Buffer.

 public:
  /// Check whether stream could be decoded by this class.
  /// @return Detected file format or NULL if not.
  static const char* Sniffer(XFILE* stream);

  AacDecoder();
  ~AacDecoder();

  const char*   Open(XFILE* stream);

  int           GetChannels() const   { return Channels; }
  int           GetSamplerate() const { return Samplerate; }
  double        GetSonglength() const { return Songlength; }
  unsigned      GetOffset() const     { return Offset; }
  /// Retrieve ID3V2 tag information if any
  void          GetMeta(META_INFO& meta);
};

class AacDecoderThread : public DECODER_STRUCT, public AacDecoder
{private:
  xstring       URL;
  XFILE*        File;
  Event         Play;       ///< For internal use to sync the decoder thread.
  Mutex         Mtx;        ///< For internal use to sync the decoder thread.
  int           DecoderTID; ///< Decoder thread identifier.

  bool          StopRq;
  //float         SkipSecs;   ///< Forward/rewind, 0 = off
  double        JumpToPos;  ///< Perform seek ASAP, -1 = off
  double        NextSkip;
  double        PlayedSecs; ///< Seconds played so far (ignoring seek operations)

 public:
                AacDecoderThread(const DECODER_PARAMS2* params);
  virtual       ~AacDecoderThread();
  virtual ULONG DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params);
  virtual double GetSonglength() const { return AacDecoder::GetSonglength(); }

 private:
  PROXYFUNCDEF void TFNENTRY DecoderThreadStub(void* arg);
  void          DecoderThread();

 private: // Instance repository
  static vector<AacDecoderThread> Instances;
  static Mutex InstMtx;
};

#endif // AACDECODER_H
