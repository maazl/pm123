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

#ifndef PM123_AACDECODER_H
#define PM123_AACDECODER_H

#include "aacplay.h"

#include <xio.h>
#include <mp4ff.h>
#include <neaacdec.h>
#include <cpp/mutex.h>
#include <cpp/container/vector.h>


class Mp4Decoder
{protected:
  mp4ff_t*      MP4stream;  ///< MP4 (QuickTime) file decoder, NULL if ADTS/ADIF stream
  int           Track;      ///< Track in MP4 that contains AAC data.
  mp4ff_callback_t Callbacks;///<File I/O callbacks for MP4stream
 protected:
  //HWND   hwnd;       // PM interface main frame window handle.
  int64_t       FileTime;
  int32_t       TimeScale;
  int           Samplerate;
  int           Channels;
 private:
  double        Songlength;
  long          Bitrate;

  /*char*  metadata_buff;
  int    metadata_size;*/

 protected:
  void          SetFile(XFILE* file)  { Callbacks.user_data = file; }
  XFILE*&       GetFile()             { return (XFILE*&)Callbacks.user_data; }
  int           Init(mp4ff_t* mp4);
  /// Convert metadata string to PM123s codepage.
  static xstring ConvertString(const char* tag);
 public:
  static const char* Sniffer(struct _XFILE* stream);

  Mp4Decoder();
  ~Mp4Decoder();

  int           GetChannels() const   { return Channels; }
  int           GetSamplerate() const { return Samplerate; }
  double        GetSonglength() const { return Songlength; }
  long          GetBitrate() const    { return mp4ff_get_avg_bitrate(MP4stream, Track); }

  /// Opens MP4 file. Returns 0 if it successfully opens the file.
  /// @return A nonzero return value indicates an error. A -1 return value
  /// indicates an unsupported format of the file.
  int           Open(XFILE* file);
  void          Close()               { if (MP4stream) { mp4ff_close(MP4stream); MP4stream = NULL; } }

  void          GetMeta(META_INFO& meta);
 private:
  PROXYFUNCDEF void DLLENTRY Observer(XIO_META type, const char* metabuff, long pos, void* arg);
};

struct Mp4DecoderThread : public DECODER_STRUCT, public Mp4Decoder
{private:
  xstring       URL;
  Event         Play;       ///< For internal use to sync the decoder thread.
  Mutex         Mtx;        ///< For internal use to sync the decoder thread.
  int           DecoderTID; ///< Decoder thread identifier.
  DECODERSTATE  Status;     ///< Decoder status

  bool          StopRq;
  float         SkipSecs;   ///< Forward/rewind, 0 = off
  double        JumpToPos;  ///< Perform seek ASAP, -1 = off
  double        NextSkip;
  double        PlayedSecs; ///< Seconds played so far (ignoring seek operations)

  NeAACDecHandle FAAD;
  int32_t       NumFrames;
  int32_t       CurrentFrame;///< Index of next MP4 sample to play

  // specify a function which the decoder should use for output
  int   DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void  DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  // decoder events
  void  DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, const void* param);
  void* A;                  ///< only to be used with the precedent functions

  int64_t       ResumePcms;

public:
  Mp4DecoderThread();
  ~Mp4DecoderThread();

  ULONG         DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params);
  DECODERSTATE  GetStatus() const { return Status; }
  double        GetSonglength() const { return Mp4Decoder::GetSonglength(); }

  static int    ReplaceStream(const char* sourcename, const char* destname);

 private:
  PROXYFUNCDEF void TFNENTRY DecoderThreadStub(void* arg);
  void          DecoderThread();

 private: // Instance repository
  static vector<Mp4DecoderThread> Instances;
  static Mutex InstMtx;

};

#define MAXRESYNC 15

#endif

