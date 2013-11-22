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

#ifndef PULSE123_PLAYBACKWORKER_H
#define PULSE123_PLAYBACKWORKER_H

#include "pulse123.h"
#include "pawrapper.h"
#include <output_plug.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <charset.h>
//#include <os2.h>


typedef struct OUTPUT_STRUCT
{private: // Backup buffer
  class BackupBuffer
  {private:
    struct Entry
    { /// Location of \c Data in bytes from the start of the stream.
      /// @remarks Since pulse123 only supports 32 bit audio this is always
      /// sizeof(float) * channels * samplerate * time_since_stream_start
      uint64_t       WriteIndex;
      /// Logical position of \c Data within the stream-
      PM123_TIME     Pos;
      /// Format of the samples \e before \c Data.
      FORMAT_INFO2   Format;
      /// Data pointer. Points \e after the samples where this index entry relies to.
      size_t         Data;
    };
    /// Mutex to protect this instance
    Mutex            Mtx;
    /// Current start index in \c BufferQueue. It points to the oldest buffer.
    size_t           BufferLow;
    /// Current end index in \c BufferQueue. It points behind the buffer.
    size_t           BufferHigh;
    /// Maximum write index so far.
    uint64_t         MaxWriteIndex;
    /// Ring buffer for index entries that point into DataBuffer.
    /// @remarks The used part of the ring buffer is ordered by \c WriteIndex.
    Entry            BufferQueue[32];
    /// Ring buffer for sample data. The buffer is managed by BufferQueue.
    float            DataBuffer[262144];
   private:
    /// Return the index of the \c Entry after or at the write index \a wi.
    /// @return BufferLow => \a wi is before the first buffer.\n
    /// BufferHigh => \a wi is in or behind the last buffer.
    size_t           FindByWriteIndex(uint64_t wi) const;
   public:
    /// Clear the buffer.
    void             Reset();
    /// Create empty buffer.
    BackupBuffer()   { Reset(); }
    /// Put samples into the buffer
    /// @param wi    Write index after the last sample.
    /// @param pos   Logical position behind the last sample.
    /// @param channels Number of channels.
    /// @param rate  Sample rate.
    /// @param data  Pointer to the first sample of the first channel.
    /// @param count Number of values (not samples!).
    void             StoreData(uint64_t wi, PM123_TIME pos, int channels, int rate, const float* data, size_t count);
    /// Calculate the logical position of the write index \a wi.
    PM123_TIME       GetPosByWriteIndex(uint64_t wi);
    /// Retrieve sample data starting at write index \a wi.
    bool             GetDataByWriteIndex(uint64_t wi, OUTPUT_PLAYING_BUFFER_CB cb, void* param);
    uint64_t         GetMaxWriteIndex() const { return MaxWriteIndex; }
  };

 private:
  xstring            Server;
  xstring            Sink;
  xstring            Port;
  int                MinLatency;
  int                MaxLatency;
 private:
  PAContext          Context;
  PASampleSpec       SS;
  PASinkOutput       Stream;
 private:
  void DLLENTRYP(OutputEvent)(void* w, OUTEVENTTYPE event);
  void*              W;
  float*             LastBuffer;
  pa_volume_t        Volume;
  PAProplist         Proplist;
  PM123_TIME         TimeOffset;
  uint64_t           WriteIndexOffset;
  bool               TrashFlag;
  bool               FlushFlag;
  bool               LowWater;
  BackupBuffer       Buffer;
  PABasicOperation   DrainOp;
  class_delegate<OUTPUT_STRUCT, const int> DrainOpDeleg;

 public:
  OUTPUT_STRUCT() throw();
  ~OUTPUT_STRUCT();
  ULONG Init(const xstring& server, const xstring& sink, const xstring& port, int minlatency, int maxlatency) throw();
  ULONG Open(const char* uri, const INFO_BUNDLE_CV* info, PM123_TIME pos,
             void DLLENTRYP(output_event)(void* w, OUTEVENTTYPE event), void* w) throw();
  ULONG Close() throw();
  ULONG SetVolume(float volume) throw();
  ULONG SetPause(bool pause) throw();
  ULONG TrashBuffers(PM123_TIME pos) throw();

  int   RequestBuffer(const FORMAT_INFO2* format, float** buf) throw();
  void  CommitBuffer(int len, PM123_TIME posmarker) throw();

  BOOL  IsPlaying() throw();
  PM123_TIME GetPosition() throw();
  ULONG GetCurrentSamples(PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param) throw();

 private:
  void  DrainOpCompletion(const int& success);
  void  RaiseOutputEvent(OUTEVENTTYPE event)    { (*OutputEvent)(W, event); }
  static char* ToUTF8(char* dst, size_t len, const char* src)
                                                { return src ? ch_convert(CH_CP_NONE, src, 1208, dst, len, 0) : NULL; }
  void  PopulatePropertyList(const char* uri, volatile const META_INFO& meta);
  void  Error(const char* fmt, ...) throw();

} PlaybackWorker;


#endif // PULSE123_PLAYBACKWORKER_H
