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


#include "playbackworker.h"
//#include <os2.h>
#include <stdint.h>
// For some reason the above include does not work.
# define UINT32_MAX     (4294967295U)

#include <stdarg.h>
#include <debuglog.h>


void PlaybackWorker::BackupBuffer::Reset()
{ BufferLow = 0;
  BufferHigh = 0;
  MaxWriteIndex = 0;
  DataHigh = 0;
}

size_t PlaybackWorker::BackupBuffer::FindByWriteIndex(uint64_t wi) const
{ DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByWriteIndex(%Lu) - [%u,%u[\n", wi, BufferLow, BufferHigh));
  // Binary search in ring buffer
  size_t l = BufferLow;
  size_t r = BufferHigh;
  while (l != r)
  { size_t m = (l + r + (sizeof BufferQueue/sizeof *BufferQueue)*(l > r)) / 2 % (sizeof BufferQueue/sizeof *BufferQueue);
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
      l = (m + 1) % (sizeof BufferQueue/sizeof *BufferQueue);
    }
  }
  DEBUGLOG(("PlaybackWorker::BackupBuffer::FindByWriteIndex: no match %u\n", l));
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

void PlaybackWorker::BackupBuffer::StoreData(uint64_t wi, PM123_TIME pos, int channels, int rate, const short* data, size_t count)
{ DEBUGLOG(("PlaybackWorker::BackupBuffer::StoreData(%Lu, %f, %i, %i, %p, %u) - [%u,%u[ %u\n",
    wi, pos, channels, rate, data, count, BufferLow, BufferHigh, DataHigh));
  if (wi <= MaxWriteIndex)
  { // Discard any data beyond the current write index.
    BufferHigh = FindByWriteIndex(wi);
  }
  MaxWriteIndex = wi;
  Entry* ep = BufferQueue + BufferHigh;
  size_t newbufhigh = (BufferHigh + 1) % (sizeof BufferQueue/sizeof *BufferQueue);
  if (newbufhigh == BufferLow)
  { // Overflow => discard one old buffer
    BufferLow = (BufferLow + 1) % (sizeof BufferQueue/sizeof *BufferQueue);
  }
  while (BufferLow != BufferHigh)
  { size_t space = (BufferQueue[BufferLow].Data + sizeof DataBuffer/sizeof *DataBuffer - DataHigh) % (sizeof DataBuffer/sizeof *DataBuffer);
    if (space > count)
      break;
    BufferLow = (BufferLow + 1) % (sizeof BufferQueue/sizeof *BufferQueue);
  }
  ep->WriteIndex = wi;
  ep->Pos = pos;
  ep->Channels = channels;
  ep->SampleRate = rate;
  ep->Data = (DataHigh + count * channels) % (sizeof DataBuffer/sizeof *DataBuffer);
  if (ep->Data < DataHigh)
  { // memory wrap
    memcpy(DataBuffer + DataHigh, data, (sizeof DataBuffer/sizeof *DataBuffer - DataHigh) * sizeof (short));
    memcpy(DataBuffer, data + sizeof DataBuffer/sizeof *DataBuffer - DataHigh, ep->Data * sizeof (short));
  } else
    memcpy(DataBuffer + DataHigh, data, count);
  BufferHigh = newbufhigh;
  DataHigh = ep->Data;
  //DEBUGLOG(("PlaybackWorker::BackupBuffer::StoreData: [%u,%u[ time = %Lu\n", BufferLow, BufferHigh, ep->Time));
}

PM123_TIME PlaybackWorker::BackupBuffer::GetPosByWriteIndex(uint64_t wi) const
{ if (wi > MaxWriteIndex)
  { // Error: negative latency???
    DEBUGLOG(("PlaybackWorker::BackupBuffer::GetPosByWriteIndex(%Lu): negative Latency", wi));
    return 0;
  }
  size_t p = FindByWriteIndex(wi);
  const Entry* ep = BufferQueue + p;
  return ep->Pos - (PM123_TIME)(ep->WriteIndex - wi) / sizeof(short) / ep->Channels / ep->SampleRate;
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

bool PlaybackWorker::BackupBuffer::GetDataByWriteIndex(uint64_t wi, short* data, size_t bytes, int& channels, int& rate) const
{ if (wi > MaxWriteIndex)
    return false; // Error: negative latency???
  size_t p = FindByWriteIndex(wi);
  const Entry* ep = BufferQueue + p;
  channels = ep->Channels;
  rate = ep->SampleRate;
  size_t datastart = (ep->Data + sizeof DataBuffer/sizeof *DataBuffer - (ep->WriteIndex - wi) / sizeof(short)) % (sizeof DataBuffer/sizeof *DataBuffer);
  size_t dataend = (datastart + bytes / sizeof(short)) % (sizeof DataBuffer/sizeof *DataBuffer);
  if (datastart > dataend)
  { memcpy(data, DataBuffer + datastart, (sizeof DataBuffer/sizeof *DataBuffer - datastart) * sizeof(short));
    memcpy(data + (sizeof DataBuffer/sizeof *DataBuffer - datastart), DataBuffer, dataend * sizeof(short));
  } else
    memcpy(data, DataBuffer + datastart, bytes);
  return true;
}


PlaybackWorker::PlaybackWorker() throw()
: Server("tcp:192.168.121.137:4713")
, Volume(PA_VOLUME_INVALID)
, DrainOpDeleg(DrainOp.Completion(), *this, &PlaybackWorker::DrainOpCompletion)
{ DEBUGLOG(("PlaybackWorker(%p)::PlaybackWorker()\n", this));
}

ULONG PlaybackWorker::Init() throw()
{ try
  { Context.Connect("PM123", Server);
    return PLUGIN_OK;
  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    return ex.GetError();
  }
}

/*Worker::~Worker()
{ if (Context)
    pa_context_disconnect(Context);
  if (Mainloop)
  { pa_threaded_mainloop_stop(Mainloop);
    pa_threaded_mainloop_free(Mainloop);
  }
}*/

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
    SS.assign(PA_SAMPLE_S16NE, info->tech->channels, info->tech->samplerate);

    // The context is automatically connected at plug-in initialization,
    // but at this point we have to synchronize the connection process.
    Context.WaitReady();
    Stream.Connect(Context, "Out", SS, NULL, pl,
                     NULL, PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_NOT_MONOTONIC|PA_STREAM_AUTO_TIMING_UPDATE/*|PA_STREAM_VARIABLE_RATE*/, Volume);

    LastBuffer = NULL;
    TimeOffset = pos;
    WriteIndexOffset = 0;
    TrashFlag = true;
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
  return ret;
}

void PlaybackWorker::DrainOpCompletion(const int& success)
{ DEBUGLOG(("PlaybackWorker(%p)::DrainOpCompletion(%u)\n", this, success));
  // We raise the finish event even in case of an error to prevent
  // PM123 from hanging at the end of the song.
  RaiseOutputEvent(OUTEVENT_END_OF_DATA);
}

int PlaybackWorker::RequestBuffer(const TECH_INFO* format, short** buf) throw()
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
    *buf = LastBuffer = (short*)Stream.BeginWrite(len);
    len /= sizeof(short) * format->channels;
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
  { uint64_t wi = Stream.Write(LastBuffer, len * sizeof(short) * SS.channels);
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
{ try
  { if (!TrashFlag && Stream.GetState() == PA_STREAM_READY)
    { double tmp = Stream.GetTime()/1E6 * sizeof(short) * SS.channels * SS.rate;
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

ULONG PlaybackWorker::GetCurrentSamples(FORMAT_INFO* info, char* buf, int len) throw()
{ DEBUGLOG(("PlaybackWorker(%p)::GetCurrentSamples(%p, %p, %i,)\n", this, info, buf, len));
  try
  { if (TrashFlag || Stream.GetState() != PA_STREAM_READY)
      return 1;
    double tmp = Stream.GetTime()/1E6 * sizeof(short) * SS.channels * SS.rate;
    info->format = WAVE_FORMAT_PCM;
    info->bits = 16;
    return !Buffer.GetDataByWriteIndex((uint64_t)tmp + WriteIndexOffset, (short*)buf, len, info->channels, info->samplerate);
  } catch (const PAException& ex)
  { // We ignore errors here
    DEBUGLOG(("PlaybackWorker::GetCurrentSamples: %s\n", ex.GetMessage().cdata()));
    return 1;
  }
}


void PlaybackWorker::Error(const char* fmt, ...) throw()
{ va_list va;
  va_start(va, fmt);
  xstring err;
  err.vsprintf(fmt, va);
  (*Ctx.plugin_api->error_display)(err);
  va_end(va);
}
