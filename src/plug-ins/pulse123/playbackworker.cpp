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


#include "playbackworker.h"
//#include <os2.h>
#include <math.h>
#include <stdint.h>
// For some reason the above include does not work.
# define UINT32_MAX     (4294967295U)

#include <cpp/cpputil.h>
#include <stdarg.h>
#include <debuglog.h>


void PlaybackWorker::BackupBuffer::Reset()
{ BufferLow = 0;
  BufferHigh = 0;
  MaxWriteIndex = 0;
  BufferQueue[countof(BufferQueue)-1].Data = 0;
  memset(DataBuffer, 0, sizeof DataBuffer);
}

size_t PlaybackWorker::BackupBuffer::FindByWriteIndex(uint64_t wi) const
{ DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByWriteIndex(%Lu) - [%u,%u[\n", wi, BufferLow, BufferHigh));
  // Binary search in ring buffer
  size_t l = BufferLow;
  size_t r = BufferHigh;
  while (l != r)
  { size_t m = (l + r + countof(BufferQueue)*(l > r)) / 2 % countof(BufferQueue);
    const Entry* ep = BufferQueue + m;
    if (ep->WriteIndex > wi)
    { // too far
      r = m;
    } else if (ep->WriteIndex == wi)
    { // exact match
      DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByWriteIndex: match %u\n", m));
      return m;
    } else
    { // before
      l = (m + 1) % countof(BufferQueue);
    }
  }
  DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByWriteIndex: no match %u -> %Lu\n", l, BufferQueue[l].WriteIndex));
  return l;
}

/*size_t PlaybackWorker::BackupBuffer::FindByTime(pa_usec_t time) const
{ DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByTime(%Lu) - [%u,%u[\n", time, BufferLow, BufferHigh));
  // Binary search in ring buffer
  size_t l = BufferLow;
  size_t r = BufferHigh;
  while (l != r)
  { size_t m = (l + r + (sizeof BufferQueue/sizeof *BufferQueue)*(l > r)) / 2 % (sizeof BufferQueue/sizeof *BufferQueue);
    const Entry* ep = BufferQueue + m;
    if (ep->Time > time)
    { // too far
      r = m;
    } else if (ep->Time == time)
    { // exact match
      DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByTime: match %u\n", m));
      return m;
    } else
    { // before
      l = (m + 1) % (sizeof BufferQueue/sizeof *BufferQueue);
    }
  }
  DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByTime: no match %u\n", l));
  return l;
}*/

void PlaybackWorker::BackupBuffer::StoreData(uint64_t wi, PM123_TIME pos, int channels, int rate, const float* data, size_t count)
{ DEBUGLOG(("PlaybackWorker::BackupBuffer::StoreData(%Lu, %f, %i, %i, %p, %u) - [%u,%u[\n",
    wi, pos, channels, rate, data, count, BufferLow, BufferHigh));
  Mutex::Lock lock(Mtx);
  if (wi <= MaxWriteIndex)
  { // Discard any data beyond the current write index.
    BufferHigh = FindByWriteIndex(wi);
  }
  MaxWriteIndex = wi;
  size_t lastbufhigh = (BufferHigh + countof(BufferQueue)-1) % countof(BufferQueue);
  Entry* ep = BufferQueue + lastbufhigh;
  size_t datahigh = ep->Data;

  // Ensure enough space
  while (BufferLow != BufferHigh)
  { if ((BufferQueue[BufferLow].Data + countof(DataBuffer) - datahigh) % countof(DataBuffer) > count)
      break;
    BufferLow = (BufferLow + 1) % countof(BufferQueue);
  }
  // Join descriptors?
  if (BufferLow == BufferHigh)
    datahigh = 0;
  else if ( ep->Format.channels == channels && ep->Format.samplerate == rate
         && fabs(ep->Pos + count * channels - pos) < 1E-6 )
    goto join;
  { // new buffer
    size_t newbufhigh = (BufferHigh + 1) % countof(BufferQueue);
    if (newbufhigh == BufferLow)
    { // Overflow => discard one old buffer
      BufferLow = (BufferLow + 1) % countof(BufferQueue);
    }
    ep = BufferQueue + BufferHigh;
    BufferHigh = newbufhigh;
    ep->Format.channels = channels;
    ep->Format.samplerate = rate;
  }
 join:
  ep->WriteIndex = wi;
  ep->Pos = pos;
  ep->Data = datahigh + count*channels;
  if (ep->Data >= countof(DataBuffer))
  { ep->Data -= countof(DataBuffer);
    // memory wrap
    memcpy(DataBuffer + datahigh, data, (countof(DataBuffer) - datahigh) * sizeof(float));
    memcpy(DataBuffer, data + countof(DataBuffer) - datahigh, ep->Data * sizeof(float));
  } else
    memcpy(DataBuffer + datahigh, data, count);
  DEBUGLOG(("PlaybackWorker::BackupBuffer::StoreData: [%u,%u[ time = %f\n", BufferLow, BufferHigh, ep->Pos));
}

PM123_TIME PlaybackWorker::BackupBuffer::GetPosByWriteIndex(uint64_t wi)
{ Mutex::Lock lock(Mtx);
  if (wi > MaxWriteIndex)
  { // Error: negative latency???
    DEBUGLOG(("PlaybackWorker::BackupBuffer::GetPosByWriteIndex(%Lu): negative Latency", wi));
    return 0;
  }
  const Entry& e = BufferQueue[FindByWriteIndex(wi)];
  return e.Pos - (PM123_TIME)(e.WriteIndex - wi) / (sizeof(float) * e.Format.channels * e.Format.samplerate);
}

/*PM123_TIME PlaybackWorker::BackupBuffer::GetPosByTimeIndex(pa_usec_t time) const
{ size_t p = FindByTime(time);
  if (p == BufferHigh)
  { // Error: negative latency???
    DEBUGLOG(("PlaybackWorker::BackupBuffer::GetPosByTimeIndex(%Lu): negative Latency", time));
    return 0;
  }
  const Entry* ep = BufferQueue + p;
  return ep->Pos - (ep->Time - time) / 1E6;
}*/

bool PlaybackWorker::BackupBuffer::GetDataByWriteIndex(uint64_t wi, OUTPUT_PLAYING_BUFFER_CB cb, void* param)
{ DEBUGLOG(("PlaybackWorker::BackupBuffer::GetDataByWriteIndex(%Lu, %p, %p)\n", wi, cb, param));
  Mutex::Lock lock(Mtx);
  if (wi > MaxWriteIndex)
    return false; // Error: negative latency???
  size_t p = FindByWriteIndex(wi);
  ASSERT(p != BufferHigh);
  const Entry* ep = BufferQueue + p;
  size_t before = (ep->WriteIndex - wi) / sizeof(float); // Requested samples start at /before/ values before ep->Data.
  size_t datastart = (ep->Data - before) % countof(DataBuffer);
  if (p == BufferLow)
  { size_t datahigh = BufferHigh == 0 ? BufferQueue[countof(BufferQueue)-1].Data : BufferQueue[BufferHigh-1].Data;
    if (before > (ep->Data - datahigh - 1) % countof(DataBuffer) + 1)
      datastart = datahigh;
  }
  for (;;)
  { p = (p + 1) % countof(BufferQueue);
    BOOL done = p == BufferHigh;
    if (datastart > ep->Data)
    { size_t count = (ep->Data - datastart + countof(DataBuffer)) / ep->Format.channels;
      size_t count2 = ep->Data / ep->Format.channels;
      DEBUGLOG(("PlaybackWorker::BackupBuffer::GetDataByWriteIndex: from %u to %u, count = %u, count2 = %u\n", datastart, ep->Data, count, count2));
      (*cb)(param, &ep->Format, DataBuffer + datastart, count - count2, ep->Pos - (PM123_TIME)count / ep->Format.samplerate, &done);
      if (done)
        break;
      (*cb)(param, &ep->Format, DataBuffer, count2, ep->Pos - (PM123_TIME)count2 / ep->Format.samplerate, &done);
    } else
    { size_t count = (ep->Data - datastart) / ep->Format.channels;
      if (count == 0)
        count = countof(DataBuffer);
      DEBUGLOG(("PlaybackWorker::BackupBuffer::GetDataByWriteIndex: from %u to %u, count = %u\n", datastart, ep->Data, count));
      (*cb)(param, &ep->Format, DataBuffer + datastart, count, ep->Pos - (PM123_TIME)count / ep->Format.samplerate, &done);
    }
    if (done)
      break;
    // next
    before = 0;
    datastart = ep->Data;
    ep = BufferQueue + p;
  }
  return true;
}


PlaybackWorker::PlaybackWorker() throw()
: Volume(PA_VOLUME_INVALID)
, DrainOpDeleg(DrainOp.Completion(), *this, &PlaybackWorker::DrainOpCompletion)
{ DEBUGLOG(("PlaybackWorker(%p)::PlaybackWorker()\n", this));
}

ULONG PlaybackWorker::Init(const xstring& server, const xstring& sink, const xstring& port, int minlatency, int maxlatency) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::Init(%s)\n", this, server.cdata()));
  Server = server;
  Sink = sink;
  Port = port;
  MinLatency = minlatency;
  MaxLatency = maxlatency;
  try
  { if (!Server || !*Server)
      throw PAException(PLUGIN_ERROR,
        "You need to configure a destination server before using pulse123.\n"
        "Go to \"Plug-ins/PulseAudio for PM123\" from the main context menu.");
    Context.Connect("PM123", Server);
    return PLUGIN_OK;
  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    return ex.GetError();
  }
}

PlaybackWorker::~PlaybackWorker()
{ DEBUGLOG(("PlaybackWorker(%p)::~PlaybackWorker()\n", this));
}

ULONG PlaybackWorker::Open(const char* uri, const INFO_BUNDLE_CV* info, PM123_TIME pos,
                           void DLLENTRYP(output_event)(void* w, OUTEVENTTYPE event), void* w) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::Open(%s, {{%i, %i},}, %f, %p, %p)\n", this,
    uri, info->tech->channels, info->tech->samplerate, pos, output_event, w));

  OutputEvent = output_event;
  W = w;

  try
  { switch (Stream.GetState())
    {default:
      // TODO: Open because of stream info change.
      return PLUGIN_OK;

     case PA_STREAM_UNCONNECTED:
     case PA_STREAM_FAILED:
     case PA_STREAM_TERMINATED:;
    }

    PAProplist pl;
    { xstring tmp; // Temporary object for strongly thread safe access to *info.
      char buf[1024];
      volatile const META_INFO* meta = info->meta;
      pl[PA_PROP_MEDIA_FILENAME]   = ToUTF8(buf, sizeof buf, uri);
      pl[PA_PROP_MEDIA_NAME]       = ToUTF8(buf, sizeof buf, tmp = meta->album);
      pl[PA_PROP_MEDIA_TITLE]      = ToUTF8(buf, sizeof buf, tmp = meta->title);
      pl[PA_PROP_MEDIA_ARTIST]     = ToUTF8(buf, sizeof buf, tmp = meta->artist);
      pl[PA_PROP_MEDIA_COPYRIGHT]  = ToUTF8(buf, sizeof buf, tmp = meta->copyright);
    }
    SS.format = PA_SAMPLE_FLOAT32LE;
    SS.channels = info->tech->channels;
    SS.rate = info->tech->samplerate;

    // The context is automatically connected at Init,
    // but at this point we have to synchronize the connection process.
    Context.WaitReady();
    if (Port)
    { PABasicOperation op;
      Context.SetSinkPort(op, Sink, Port);
      op.EnsureSuccess();
    }
    Stream.Connect(Context, "Out", &SS, NULL, pl,
                   Sink, PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_NOT_MONOTONIC|PA_STREAM_AUTO_TIMING_UPDATE/*|PA_STREAM_VARIABLE_RATE*/, Volume);

    LastBuffer = NULL;
    TimeOffset = pos;
    WriteIndexOffset = 0;
    TrashFlag = true;
    LowWater = false;
    Buffer.Reset();

    Stream.WaitReady();
    return PLUGIN_OK;
  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    return ex.GetError();
  }
}

ULONG PlaybackWorker::Close() throw()
{ DEBUGLOG(("PlaybackWorker(%p)::Close()\n", this));
  Stream.Disconnect();
  return 0;
}

ULONG PlaybackWorker::SetVolume(float volume) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::SetVolume(%f)\n", this, volume));
  try
  { // pseudo logarithmic scale
    volume /= 1 + 3.16227766*(1-volume);
    Volume = pa_sw_volume_from_linear(volume);
    if (Stream.GetState() == PA_STREAM_READY)
      Stream.SetVolume(Volume);
    return 0;

  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    return ex.GetError();
  }
}

ULONG PlaybackWorker::SetPause(bool pause) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::SetPause(%u)\n", this, pause));
  try
  { Stream.Cork(pause);
    if (LowWater)
    { LowWater = false;
      (*OutputEvent)(W, OUTEVENT_HIGH_WATER);
    }
    return 0;

  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    return ex.GetError();
  }
}

ULONG PlaybackWorker::TrashBuffers(PM123_TIME pos) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::TrashBuffers(%f)\n", this, pos));
  ULONG ret = 0;
  try
  { Stream.Flush();
  } catch (const PAException& ex)
  { // We ignore errors here
    DEBUGLOG(("PlaybackWorker::TrashBuffers %s\n", ex.GetMessage().cdata()));
    ret = ex.GetError();
  }
  TimeOffset = pos;
  TrashFlag = true;
  if (!LowWater)
  { LowWater = true;
    (*OutputEvent)(W, OUTEVENT_LOW_WATER);
  }
  return ret;
}

void PlaybackWorker::DrainOpCompletion(const int& success)
{ DEBUGLOG(("PlaybackWorker(%p)::DrainOpCompletion(%u)\n", this, success));
  // We raise the finish event even in case of an error to prevent
  // PM123 from hanging at the end of the song.
  RaiseOutputEvent(OUTEVENT_END_OF_DATA);
}

int PlaybackWorker::RequestBuffer(const FORMAT_INFO2* format, float** buf) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::RequestBuffer({%i, %i}, )\n", this,
    format ? format->channels : -1, format ? format->samplerate : -1));
  try
  { if (buf == NULL)
    { // flush request
      Stream.RunDrain(DrainOp);
      return 0;
    }
    // TODO: format changes!
    size_t len = (size_t)-1;
    *buf = LastBuffer = (float*)Stream.BeginWrite(len);
    len /= sizeof(float) * format->channels;
    DEBUGLOG(("PlaybackWorker::RequestBuffer: %i\n", len));
    return len;

  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    RaiseOutputEvent(OUTEVENT_PLAY_ERROR);
    return -ex.GetError();
  }
}
void PlaybackWorker::CommitBuffer(int len, PM123_TIME pos) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::CommitBuffer(%i, %f) - %p\n", this, len, pos, LastBuffer));
  ASSERT(LastBuffer);
  try
  { len *= SS.channels;
    uint64_t wi = Stream.Write(LastBuffer, len * sizeof(float));
    Buffer.StoreData(wi, pos + (PM123_TIME)len/SS.rate, SS.channels, SS.rate, LastBuffer, len);
    TrashFlag = false;
    LastBuffer = NULL;
  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    RaiseOutputEvent(OUTEVENT_PLAY_ERROR);
  }
}

BOOL PlaybackWorker::IsPlaying() throw()
{ return Stream.GetState() == PA_STREAM_READY;
}

PM123_TIME PlaybackWorker::GetPosition() throw()
{ // Check for buffer level
  const pa_timing_info* ti = Stream.GetTimingInfo();
  if (ti)
  { long ms = pa_bytes_to_usec(ti->write_index - ti->read_index, &SS) / 1000;
    DEBUGLOG(("PlaybackWorker::GetPosition Timing info: %u, %u, %u\n", ms, MinLatency, MaxLatency));
    if (LowWater)
    { if (ms > MinLatency + MaxLatency)
      { LowWater = false;
        (*OutputEvent)(W, OUTEVENT_HIGH_WATER);
      }
    } else
    { if (ms < MinLatency)
      { LowWater = true;
        (*OutputEvent)(W, OUTEVENT_LOW_WATER);
      }
    }
  }
  // get time
  try
  { if (!TrashFlag && Stream.GetState() == PA_STREAM_READY)
    { double tmp = Stream.GetTime()/1E6 * sizeof(float) * SS.channels * SS.rate;
      tmp = Buffer.GetPosByWriteIndex((uint64_t)tmp + WriteIndexOffset);
      DEBUGLOG(("PlaybackWorker::GetPosition: %f\n", tmp));
      return tmp;
    }
  } catch (const PAException& ex)
  { // We ignore errors here
    DEBUGLOG(("PlaybackWorker::GetPosition %s\n", ex.GetMessage().cdata()));
  }
  // Error
  return TimeOffset;
}

ULONG PlaybackWorker::GetCurrentSamples(PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::GetCurrentSamples(%f, %p, %p)\n", this, offset, cb, param));
  try
  { if (TrashFlag || Stream.GetState() != PA_STREAM_READY)
      return 1;
    double tmp = Stream.GetTime()/1E6 * (sizeof(float) * SS.channels * SS.rate);
    return !Buffer.GetDataByWriteIndex((uint64_t)tmp + WriteIndexOffset, cb, param);
  } catch (const PAException& ex)
  { // We ignore errors here
    DEBUGLOG(("PlaybackWorker::GetCurrentSamples: error %s\n", ex.GetMessage().cdata()));
    return 1;
  }
}


void PlaybackWorker::Error(const char* fmt, ...) throw()
{ va_list va;
  va_start(va, fmt);
  xstring err;
  err.vsprintf(fmt, va);
  (*Ctx.plugin_api->message_display)(MSG_ERROR, err);
  va_end(va);
}
