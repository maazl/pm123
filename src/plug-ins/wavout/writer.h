/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *           2013      Marcel Mueller
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

#ifndef WRITER_H
#define WRITER_H

#include <output_plug.h>
#include <cpp/mutex.h>
#include <cpp/container/vector.h>
#include <cpp/url123.h>
#include <stdint.h>
#include <format.h>
#include <xio.h>

#define BUFFERSIZE 16384

typedef struct OUTPUT_STRUCT
{private:
  struct WAVE_HEADER
  { struct ID
    { char           FourCC[4];
    };
    struct RIFF_HEADER
    { ID             IdRiff; // RIFF
      unsigned long  Len;
      ID             IdWave; // WAVE
    };
    struct CHNK_HEADER
    { ID             Id;
      unsigned long  Len;
    };
    struct FORMAT
    { unsigned short FormatTag;
      unsigned short Channels;
      unsigned long  SamplesPerSec;
      unsigned long  AvgBytesPerSec;
      unsigned short BlockAlign;
      unsigned short BitsPerSample;
    };
    RIFF_HEADER  Riff;
    CHNK_HEADER  FormatHeader;
    FORMAT       Format;
    CHNK_HEADER  DataHeader;
    WAVE_HEADER();
    void SetFormat(unsigned channels, unsigned samplingrate, unsigned bitspersample);
    void SetLength(int64_t bytes);
  };

  struct SongEntry
  { url123     URL;
    PM123_TIME Start;
    SongEntry(const xstring& url, PM123_TIME start) : URL(url), Start(start) {}
  };

 private:
  void DLLENTRYP(OutEvent)(void* w, OUTEVENTTYPE event);
  void* W;
  // Wirking set.
  // O = owned by output thread.
  // S = owned by output thread, read by controller thread unsynchronized.
  // M = shared between output and controller thread and protected by Mutex.
  xstringbuilder FileName;          ///< O Current output file name.
  XFILE* File;                      ///< O Current output file handle. \c NULL if none.
  bool IsTimestampFile;             ///< O Current output file uses timestamp naming.
  bool IsError;                     ///< S Writing failed, return error on RequestBuffer.
  Event EvPause;                    ///< Pause flag. Reset if paused.

  Mutex Mtx;                        ///< Mutex to protect shared data.
  PM123_TIME PlayingPos;            ///< M Last saved playing pos.
  vector_own<SongEntry> PlayQueue;  ///< M Queue with coming songs.
  FORMAT_INFO2 Format;              ///< M Current sample format.
  int   CurrentBuffer;              ///< M Current active buffer in FloatBuffer, i.e. the one that is filled by the source.
  WAVE_HEADER Header;               ///< O RIFF WAVE header.
  float FloatBuffer[2][BUFFERSIZE]; ///< S Two buffers for samples.
  short ShortBuffer[BUFFERSIZE];    ///< O To 16 bits converted samples.

 public:
  static volatile xstring OutPath;
  static bool UseTimestamp;

 private:
  bool OpenFile(const url123& song);
  bool WriteFile(const void* buffer, size_t count);
  void CloseFile();
  void ConvertSamples(const float* sp, size_t count);

 private: // non-copyable
  OUTPUT_STRUCT(const OUTPUT_STRUCT& r);
  void operator=(const OUTPUT_STRUCT& r);
 public:
  OUTPUT_STRUCT();
  ~OUTPUT_STRUCT() { CloseFile(); }

  // control interface
  void OutSetup(void DLLENTRYP(eh)(void*, OUTEVENTTYPE), void* w) { OutEvent = eh; W = w; }
  void OutOpen(const xstring& url, PM123_TIME start) { Mutex::Lock lock(Mtx); PlayQueue.append() = new SongEntry(url, start); }
  void OutPause(bool pause) { if (pause) EvPause.Reset(); else EvPause.Set(); }
  void OutTrash() { Mutex::Lock lock(Mtx); PlayQueue.set_size(1); }
  void OutClose() { EvPause.Reset(); }

  // status interface
  PM123_TIME OutPlayingPos() { Mutex::Lock lock(Mtx); return PlayingPos; }
  ULONG PlayingSamples(PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param);

  // data interface
  int RequestBuffer(const FORMAT_INFO2* format, float** buf);
  void CommitBuffer(int len, PM123_TIME pos);

} WAVOUT;

#endif
