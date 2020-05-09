/*
 * Copyright 2009-2013 Marcel Mueller
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef PM123_OGGDECODER_H
#define PM123_OGGDECODER_H

#include <decoder_plug.h>
#include <xio.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <cpp/mutex.h>
#include <cpp/container/vector.h>


class OggDecoder
{public:
  XFILE*          File;
  xstring         URL;
 protected:
  OggVorbis_File  VrbFile;
 private:
  vorbis_info*    VrbInfo;
  vorbis_comment* Comment;

  //HWND   hwnd;       // PM interface main frame window handle.
  double          Songlength;
  long            Bitrate;
  //FORMAT_INFO2    OutputFormat;
  double          PlayedSecs;

  /*char*  metadata_buff;
  int    metadata_size;*/

 public:
  OggDecoder();
  ~OggDecoder();

  int            GetChannels() const   { return VrbInfo->channels; }
  int            GetSamplerate() const { return VrbInfo->rate; }
  double         GetSonglength() const { return Songlength; }
  long           GetBitrate() const    { return Bitrate; }
  double         GetPos() const        { return PlayedSecs; }

  /// Opens Ogg Vorbis file. Returns 0 if it successfully opens the file.
  /// @return A nonzero return value indicates an error. A -1 return value
  /// indicates an unsupported format of the file.
  int OggOpen();
  void OggClose()                      { ov_clear(&VrbFile); }

  /// Reads up to count pcm bytes from the Ogg Vorbis stream and
  /// stores them in the given buffer.
  int OggRead(float* buffer, int count);

  /// Changes the current file position to a new time within the file.
  /// @return Returns 0 if it successfully moves the pointer. A nonzero return
  /// value indicates an error. On devices that cannot seek the return
  /// value is nonzero.
  int OggSeek(double secs);

  /// Returns a specified field of the given Ogg Vorbis comment.
  xstring VorbisGetString(const char* type) const;
  /*/// Sets a specified field of the given Ogg Vorbis comment.
  void VorbisSetString(const char* type, const char* value);*/

 private:
  PROXYFUNCDEF void DLLENTRY Observer(XIO_META type, const char* metabuff, long pos, void* arg);
};

typedef struct DECODER_STRUCT : public OggDecoder
{private:
  Event         Play;       ///< For internal use to sync the decoder thread.
  Mutex         Mtx;        ///< For internal use to sync the decoder thread.
  int           DecoderTID; ///< Decoder thread identifier.
  DECODERSTATE  Status;     ///< Decoder status

  bool          StopRq;
  float         SkipSecs;   ///< Forward/rewind
  double        JumpToPos;
  double        NextSkip;

  // specify a function which the decoder should use for output
  int   DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void  DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  // decoder events
  void  DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, const void* param);
  void* A;                  ///< only to be used with the precedent functions

  ogg_int64_t     ResumePcms;

public:
  DECODER_STRUCT();
  ~DECODER_STRUCT();

  ULONG DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params);
  DECODERSTATE GetStatus() const { return Status; }

  static int ReplaceStream(const char* sourcename, const char* destname);

 private:
  PROXYFUNCDEF void TFNENTRY DecoderThreadStub(void* arg);
  void DecoderThread();

 private: // Instance repository
  static vector<DECODER_STRUCT> Instances;
  static Mutex InstMtx;

} OggDecoderThread;

#define MAXRESYNC 15

#endif /* PM123_OGGPLAY_H */

