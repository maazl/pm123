/*
 * Copyright 2010 Marcel Mueller
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
#include <cpp/smartptr.h>
#include <charset.h>
//#include <os2.h>


class PlaybackWorker
{private: // Backup buffer
  class BackupBuffer
  {private:
    struct Entry
    { uint64_t       WriteIndex;
      PM123_TIME     Pos;
      int            Channels;
      int            SampleRate;
      size_t         Data;
    };

    Entry            BufferQueue[200];
    size_t           BufferLow;
    size_t           BufferHigh; // exclusive bound
    uint64_t         MaxWriteIndex;
    short            DataBuffer[262144];
    size_t           DataHigh;
   private:
    size_t FindByWriteIndex(uint64_t wi) const;
   public:
    void Reset();
    void StoreData(uint64_t wi, PM123_TIME pos, int channels, int rate, const short* data, size_t count);
    PM123_TIME GetPosByWriteIndex(uint64_t wi) const;
    bool GetDataByWriteIndex(uint64_t wi, short* data, size_t bytes, int& channels, int& rate) const;
  };

 private:
  DSTRING            Server;
 private:
  PAContext          Context;
  PASampleSpec       SS;
  PASinkInput        Stream;
 private:
  void DLLENTRYP(OutputEvent)(void* w, OUTEVENTTYPE event);
  void*              W;
  short*             LastBuffer;
  pa_volume_t        Volume;
  PM123_TIME         TimeOffset;
  uint64_t           WriteIndexOffset;
  bool               TrashFlag;
  BackupBuffer       Buffer;
  PABasicOperation   DrainOp;
  class_delegate<PlaybackWorker, const int> DrainOpDeleg;

 public:
  PlaybackWorker() throw();
  ULONG Init() throw();
  //~Worker()                                     { Stream; pa_context_disconnect(Context); }
  ULONG Open(const char* uri, const INFO_BUNDLE_CV* info, PM123_TIME pos,
             void DLLENTRYP(output_event)(void* w, OUTEVENTTYPE event), void* w) throw();
  ULONG Close() throw();
  ULONG SetVolume(float volume) throw();
  ULONG SetPause(bool pause) throw();
  ULONG TrashBuffers(PM123_TIME pos) throw();

  int   RequestBuffer(const TECH_INFO* format, short** buf) throw();
  void  CommitBuffer(int len, PM123_TIME posmarker) throw();

  BOOL  IsPlaying() throw();
  PM123_TIME GetPosition() throw();
  ULONG GetCurrentSamples(FORMAT_INFO* info, char* buf, int len) throw();

 private:
  void  DrainOpCompletion(const int& success);
  void  RaiseOutputEvent(OUTEVENTTYPE event)    { (*OutputEvent)(W, event); }
  char* ToUTF8(char* dst, size_t len, const char* src)
                                                { return src ? ch_convert(CH_CP_NONE, src, 1208, dst, len, 0) : NULL; }
  void  Error(const char* fmt, ...) throw();
};


#endif // PULSE123_WORKER_H
