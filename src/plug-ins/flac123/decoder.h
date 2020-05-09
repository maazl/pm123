/*
 * Copyright 2012 Marcel Mueller
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


#ifndef FLACDECODER_H
#define FLACDECODER_H

#include <FLAC/stream_decoder.h>
#include <decoder_plug.h>
#include <cpp/xstring.h>
#include <cpp/atomic.h>
#include <cpp/mutex.h>
#include <xio.h>

class DecoderBase
{protected:
  XFILE*          File;
  xstring         URL;
  FLAC__StreamDecoder* Decoder;

 public:
  DecoderBase();
  ~DecoderBase();

  /// Must be called exactly once after the constructor.
  bool Init();

  bool Setup(XFILE* file, const xstring& url);

  /*unsigned GetSamplerate() const       { return FLAC__stream_decoder_get_sample_rate(Decoder); }
  unsigned GetChannels() const         { return FLAC__stream_decoder_get_channels(Decoder); }

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
  xstring VorbisGetString(const char* type) const;*/

 protected:
  /// Helper to call \c FLAC__stream_decoder_init_ogg_stream or \c FLAC__stream_decoder_init_stream respectively.
  /// @return True: succeeded.
  bool DecoderInitStream(
    FLAC__StreamDecoderWriteCallback    write_callback,
    FLAC__StreamDecoderMetadataCallback metadata_callback,
    FLAC__StreamDecoderErrorCallback    error_callback);

  static FLAC__StreamDecoderReadStatus ReadCB(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
  static FLAC__StreamDecoderSeekStatus SeekCB(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
  static FLAC__StreamDecoderTellStatus TellCB(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
  static FLAC__StreamDecoderLengthStatus LengthCB(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
  static FLAC__bool EofCB(const FLAC__StreamDecoder *decoder, void *client_data);

  static void ErrorCB(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

  PROXYFUNCDEF void DLLENTRY Observer(XIO_META type, const char* metabuff, long pos, void* arg);
};

class MetaDecoder : public DecoderBase
{private:
  const INFO_BUNDLE* Target;
 public:
  /// Must be called exactly once after the constructor.
  bool Init();
  bool Analyze(const INFO_BUNDLE* target, XFILE* file, const xstring& url);
 private:

  static void StoreVorbisComment(const char* value);

  static FLAC__StreamDecoderWriteStatus WriteCB(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
  static void MetadataCB(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
};

typedef struct DECODER_STRUCT : public DecoderBase
{
 private: // Callbacks
  void* A;
  int   DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void  DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  void  DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, const void* param);
 private: // State
  DECODERSTATE State;
  int          DecoderTID;
  FORMAT_INFO2 Format;
  FLAC__uint64 SamplePos;
  PM123_TIME   NextSkip;
  //XFILE*       SaveTo;
  float        SkipSecs;  ///< Number of seconds to skip, 0 = normal play.
 private: // Tasks
  bool         Terminate; ///< true to stop the decoder thread
  PM123_TIME   SeekTo;    ///< Location or -1
  Event        PlayEv;

 private:
  void Reset();
  PM123_TIME GetTime() const { return Format.samplerate <= 0 ? -1 : (double)SamplePos / Format.samplerate; }

 public:
  DECODER_STRUCT();

  void Setup(void* a,
    int  DLLENTRYP(req)(void*, const FORMAT_INFO2*, float**),
    void DLLENTRYP(commit)(void*, int, PM123_TIME),
    void DLLENTRYP(event)(void*, DECEVENTTYPE, const void*) )
  { Reset(); A = a; OutRequestBuffer = req; OutCommitBuffer = commit; DecEvent = event; }

  PLUGIN_RC Play(const xstring& url, PM123_TIME start);

  PLUGIN_RC Stop();

  PLUGIN_RC Seek(PM123_TIME location);

  PLUGIN_RC SetFast(float skipspeed);

  //PLUGIN_RC SaveStream(const xstring& target);

  DECODERSTATE GetState() const { return Terminate ? DECODER_STOPPING : State; }

  PM123_TIME GetLength() const;


 private:
  PROXYFUNCDEF void TFNENTRY DecoderThreadStub(void* arg);
  void DecoderThread();

  static FLAC__StreamDecoderWriteStatus WriteCB(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
  static void MetadataCB(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
} ThreadDecoder;

/*class OggDecoderThread : public OggDecoder
{private:
  Event         Play;       // For internal use to sync the decoder thread.
  Mutex         Mtx;        // For internal use to sync the decoder thread.
  int           DecoderTID; // Decoder thread identifier.
  DECODERSTATE  Status;     // Decoder status

  bool          StopRq;
  double        JumpToPos;
  DECFASTMODE   FastMode;   // Forward/rewind
  double        NextSkip;

  int   DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void  DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  void  DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, void* param);
  void* A;

  ogg_int64_t     ResumePcms;

public:
  OggDecoderThread();
  ~OggDecoderThread();

  ULONG DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params);
  DECODERSTATE GetStatus() const { return Status; }

  static int ReplaceStream(const char* sourcename, const char* destname);

 private:
  PROXYFUNCDEF void TFNENTRY DecoderThreadStub(void* arg);
  void DecoderThread();

 private: // Instance repository
  static vector<OggDecoderThread> Instances;
  static Mutex InstMtx;

};*/

#endif
