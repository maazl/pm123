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


#include "recordworker.h"
//#include <os2.h>

#include <cpp/cpputil.h>
#include <stdarg.h>
#include <stdio.h>
#include <debuglog.h>


RecordWorker::Params::Params()
: Flags(PA_STREAM_NOFLAGS)
{ // some defaults
  Format.samplerate = 48000;
  Format.channels   = 2;
}

xstring RecordWorker::Params::ParseURL(const char* url)
{ DEBUGLOG(("RecordWorker::Params::ParseURL(%s)\n", url));

  if (strnicmp(url, "pulseaudio:", 11) != 0)
    return xstring::empty;
  url += 11;

  // pulseaudio:server[/source[/port]][?parameters]
  size_t len = strcspn(url, "?/\\");
  if (!len)
    return "pulseaudio: servername missing";
  Server.assign(url, len);
  url += len;

  if (*url == '/' || *url == '\\')
  { // source specified
    len = strcspn(++url, "?/\\");
    if (len)
    { Source.assign(url, len);
      url += len;
    } else
      Source.reset();
    if (*url == '/' || *url == '\\')
    { // Port specified
      len = strcspn(++url, "?");
      if (len)
      { Port.assign(url, len);
        url += len;
      } else
        Port.reset();
    }
  }

  if (*url == '?')
  { // parameters specified
    do
    { len = strcspn(++url, "=&");
      xstring key(url, len);
      url += len;
      xstring value;
      if (*url == '=')
      { // has value
        len = strcspn(++url, "&");
        value.assign(url, len);
        url += len;
      }
      // handle param
      if (key.compareToI("samplerate") == 0 || key.compareToI("rate") == 0)
      { if ( sscanf(value, "%i%n", &Format.samplerate, &len) != 1
          || len != value.length() || Format.samplerate <= 0 )
          return xstring().sprintf("bad sampling rate: %s", value.cdata());
      } else
      if (key.compareToI("channels") == 0)
      { if ( sscanf(value, "%i%n", &Format.channels, &len) != 1
          || len != value.length() || Format.channels <= 0 )
          return xstring().sprintf("number of channels bad: %s", value.cdata());
      } else
      if (key.compareToI("mono") == 0)
      { Format.channels = 1;
      } else
      if (key.compareToI("stereo") == 0)
      { Format.channels = 2;
      } else
        return xstring().sprintf("unknown parameter %s", key.cdata());
    } while (*url);
  }

  return xstring();
}

xstring RecordWorker::Params::ToURL() const
{ // create url: pulseaudio:server[/source[/port]][?parameters]
  xstringbuilder sb;
  sb.append("pulseaudio:");
  sb.append(Server);
  if (Source)
  { sb.append('/');
    sb.append(Source);
  }
  if (Port)
  { if (!Source)
      sb.append('/');
    sb.append('/');
    sb.append(Port);
  }
  sb.appendf("?samplerate=%u&channels=%u", Format.samplerate, Format.channels);
  return sb;
}


RecordWorker::RecordWorker()
: Running(false)
, RecordTID(-1)
{ DEBUGLOG(("RecordWorker(%p)::RecordWorker()\n", this));
}

RecordWorker::~RecordWorker()
{ DEBUGLOG(("RecordWorker(%p)::~RecordWorker()\n", this));
  TID tid = RecordTID;
  if (tid >= 0)
  { Stop();
    DosWaitThread(&tid, DCWW_WAIT);
  }
}

ULONG RecordWorker::Setup(const DECODER_PARAMS2& params)
{ DEBUGLOG(("RecordWorker(%p)::Setup(...)\n", this));

  OutRequestBuffer = params.OutRequestBuffer;
  OutCommitBuffer = params.OutCommitBuffer;
  DecEvent = params.DecEvent;
  A = params.A;
  return PLUGIN_OK;
}

ULONG RecordWorker::Play(const xstring& url)
{ DEBUGLOG(("RecordWorker(%p)::Play(%s)\n", this, url.cdata()));
  if (Running)
    return PLUGIN_GO_ALREADY;
  if (Par.ParseURL(url))
    return PLUGIN_NO_PLAY;

  try
  { Context.Connect("PM123", Par.Server);
  } catch (const PAException& ex)
  { Error(ex.GetMessage());
    return ex.GetError();
  }
  Terminate = false;
  SampleOffset = 0;
  RecordTID = _beginthread(PROXYFUNCREF(RecordWorker)RecordThreadStub, NULL, 65536, this);
  DEBUGLOG(("RecordWorker(%p)::Play: TID = %i\n", this, RecordTID));
  if (RecordTID == -1)
    return PLUGIN_GO_FAILED;

  return PLUGIN_OK;
}

ULONG RecordWorker::Stop()
{ DEBUGLOG(("RecordWorker(%p)::Stop() - %%u, %u, %u\n", this,
    Terminate, Running, RecordTID));
  if (!Running || Terminate)
    return PLUGIN_GO_ALREADY;
  Terminate = true;
  return PLUGIN_OK;
}

void RecordWorker::Event(OUTEVENTTYPE event)
{ DEBUGLOG(("RecordWorker(%p)::Event(%i)\n", event));

}

DECODERSTATE RecordWorker::GetState() const
{ if (RecordTID == -1)
    return DECODER_STOPPED;
  if (!Running)
    return DECODER_STARTING;
  if (Terminate)
    return DECODER_STOPPING;
  return DECODER_PLAYING;
}

void RecordWorker::RecordThread()
{ DEBUGLOG(("RecordWorker(%p)::RecordThread()\n", this));
  try
  { // Wait for the connection to become ready.
    Context.WaitReady();
    { PASampleSpec ss;
      ss.format = PA_SAMPLE_FLOAT32LE;
      ss.rate = Par.Format.samplerate;
      ss.channels = Par.Format.channels;
      Stream.Connect(Context, "In", &ss, NULL, NULL, Par.Source, Par.Flags);
    }
    Stream.WaitReady();
    Running = true;

    while (!Terminate)
    { // Read stream data
      size_t sbytes;
      const float* source = (const float*)Stream.Peek(sbytes);
      if (Terminate)
        break;

     next:
      // Get target buffer
      float* target;
      int tcount = (*OutRequestBuffer)(A, &Par.Format, &target);
      if (tcount <= 0)
      { (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
        break;
      }
      if (Terminate)
        break;

      // forward stream data
      size_t tbytes = tcount * Par.Format.channels * sizeof(float);
      if (tbytes < sbytes)
      { // pass fragment
        memcpy(target, source, tbytes);
        (*OutCommitBuffer)(A, tcount, (double)SampleOffset / Par.Format.samplerate);
        SampleOffset += tcount;
        source += tcount;
        sbytes -= tbytes;
        goto next;
      }
      memcpy(target, source, sbytes);
      size_t scount = sbytes / (Par.Format.channels * sizeof(float));
      (*OutCommitBuffer)(A, scount, (double)SampleOffset / Par.Format.samplerate);
      SampleOffset += scount;

      // Commit data transfer to source
      Stream.Drop();
    }

  } catch (const PAException& ex)
  { (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
    Error(ex.GetMessage());
  }
  Stream.Disconnect();
  RecordTID = -1;
  Running = false;
  DEBUGLOG(("RecordWorker(%p)::RecordThread: terminate\n", this));
}

PROXYFUNCIMP(void TFNENTRY, RecordWorker) RecordThreadStub(void* that)
{ ((RecordWorker*)that)->RecordThread();
}

void RecordWorker::Error(const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  xstring err;
  err.vsprintf(fmt, va);
  (*Ctx.plugin_api->message_display)(MSG_ERROR, err);
  va_end(va);
}
