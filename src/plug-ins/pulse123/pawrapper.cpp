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

#define INCL_DOS
#include "pawrapper.h"

#include <interlocked.h>
#include <memory.h>
#include <os2.h>
#include <debuglog.h>


void PAException::SetException(int err, const xstring& message)
{ DEBUGLOG(("PAException::SetException(%i, %s)\n", err, message.cdata()));
  Error = err;
  Message = message;
}

PAStreamException::PAStreamException(pa_stream* s, const char* msg)
{ pa_context* context = pa_stream_get_context(s);
  if (!context)
    // No context ???
    SetException(PA_ERR_INTERNAL, xstring().sprintf("%s, (stream has no context)", msg));
  else
  { int err = pa_context_errno(context);
    SetException(err, xstring().sprintf("%s, server %s: %s", msg, pa_context_get_server(context), pa_strerror(err)));
  }
}

PAStreamException::PAStreamException(pa_stream* s, int err, const char* msg)
{ pa_context* context = pa_stream_get_context(s);
  if (!context)
    SetException(err, xstring().sprintf("%s (stream has no context): %s", msg, pa_strerror(err)));
  else
    SetException(err, xstring().sprintf("%s, server %s: %s", msg, pa_context_get_server(context), pa_strerror(err)));
}

PAStreamEndException::PAStreamEndException()
{ SetException(PA_OK, "Stream has ended");
}

void PAProplist::reference::operator=(const char* value)
{ if (value)
    pa_proplist_sets(PL, Key, value);
  else
    pa_proplist_unset(PL, Key);
}


PAServerInfo& PAServerInfo::operator=(const pa_server_info& r)
{ user_name             = r.user_name;
  host_name             = r.host_name;
  server_version        = r.server_version;
  server_name           = r.server_name;
  sample_spec           = r.sample_spec;
  default_sink_name     = r.default_sink_name;
  default_source_name   = r.default_source_name;
  cookie                = r.cookie;
  channel_map           = r.channel_map;
  return *this;
}


PASinkInfo& PASinkInfo::operator=(const pa_sink_info& r)
{ name                  = r.name;
  index                 = r.index;
  description           = r.description;
  sample_spec           = r.sample_spec;
  channel_map           = r.channel_map;
  owner_module          = r.owner_module;
  volume                = r.volume;
  mute                  = r.mute;
  monitor               = r.monitor_source;
  monitor_name          = r.monitor_source_name;
  latency               = r.latency;
  driver                = r.driver;
  flags                 = r.flags;
  proplist              = r.proplist;
  configured_latency    = r.configured_latency;
  base_volume           = r.base_volume;
  state                 = r.state;
  n_volume_steps        = r.n_volume_steps;
  card                  = r.card;
  ports.reset(r.n_ports);
  active_port           = NULL;
  for (unsigned i = 0; i < r.n_ports; ++i)
  { pa_sink_port_info* port = r.ports[i];
    ports[i] = *port;
    if (port == r.active_port)
      active_port = ports.get() + i;
  }
  formats.reset(r.n_formats);
  for (unsigned i = 0; i < r.n_formats; ++i)
    formats[i] = *r.formats[i];
  return *this;
}

PASourceInfo& PASourceInfo::operator=(const pa_source_info& r)
{ name                  = r.name;
  index                 = r.index;
  description           = r.description;
  sample_spec           = r.sample_spec;
  channel_map           = r.channel_map;
  owner_module          = r.owner_module;
  volume                = r.volume;
  mute                  = r.mute;
  monitor               = r.monitor_of_sink;
  monitor_name          = r.monitor_of_sink_name;
  latency               = r.latency;
  driver                = r.driver;
  flags                 = r.flags;
  proplist              = r.proplist;
  configured_latency    = r.configured_latency;
  base_volume           = r.base_volume;
  state                 = r.state;
  n_volume_steps        = r.n_volume_steps;
  card                  = r.card;
  ports.reset(r.n_ports);
  active_port           = NULL;
  for (unsigned i = 0; i < r.n_ports; ++i)
  { pa_source_port_info* port = r.ports[i];
    ports[i] = *port;
    if (port == r.active_port)
      active_port = ports.get() + i;
  }
  formats.reset(r.n_formats);
  for (unsigned i = 0; i < r.n_formats; ++i)
    formats[i] = *r.formats[i];
  return *this;
}

void PAOperation::Unlink()
{ DEBUGLOG(("PAOperation(%p{%p})::Unlink()\n", this, Operation));
  if (Operation)
  { PAContext::Lock lock;
    // The current pulseaudio implementation has no option to disable a callback
    // without canceling the operation. So the destruction of the Operation wrapper
    // must necessarily cancel the operation to avoid undefined bahavior of
    // the outstanding callback.
    if (Operation)
    { pa_operation_cancel(Operation);
      if (WaitPending)
        PAContext::MainloopSignal();
      pa_operation_unref(Operation);
      Operation = NULL;
    }
  }
}

void PAOperation::Signal()
{ DEBUGLOG(("PAOperation(%p{%p})::Signal() - %u\n", this, Operation));
  FinalState = PA_OPERATION_DONE;
  pa_operation_unref(Operation);
  Operation = NULL;
  if (WaitPending)
    PAContext::MainloopSignal();
}

void PAOperation::Cancel()
{ DEBUGLOG(("PAOperation(%p{%p})::Cancel()\n", this, Operation));
  FinalState = PA_OPERATION_CANCELLED;
  // Double check: fast path without lock.
  if (GetState() == PA_OPERATION_RUNNING)
    Unlink();
}

void PAOperation::Wait() throw (PAStateException)
{ DEBUGLOG(("PAOperation(%p{%p})::Wait()\n", this, Operation));
  // Double check: fast path without lock.
  switch (pa_operation_get_state(Operation))
  {case PA_OPERATION_DONE:
    return;
   case PA_OPERATION_CANCELLED:
    throw PAStateException("Operation canceled");
   default:;
  }
  PAContext::Lock lock;
  ++WaitPending;
  for (;;)
  { PAContext::MainloopWait();
    DEBUGLOG(("PAOperation::Wait() - %i\n", pa_operation_get_state(Operation)));
    switch (pa_operation_get_state(Operation))
    {case PA_OPERATION_DONE:
      --WaitPending;
      return;
     case PA_OPERATION_CANCELLED:
      --WaitPending;
      throw PAStateException("Operation canceled");
     default:;
    }
  }
}

void PABasicOperation::EnsureSuccess() throw (PAStateException,PAContextException)
{ Wait();
  if (Success != 0)
    throw PAContextException(Success, "BasicOperation failed");
}

void PABasicOperation::ContextSuccessCB(pa_context* c, int success, void* userdata)
{ DEBUGLOG(("PABasicOperation::StreamSuccessCB(%p, %i, %p)\n", c, success, userdata));
  PABasicOperation& op = *(PABasicOperation*)userdata;
  op.Success = success;
  op.CompletionEvent(success);
  op.Signal();
}
void PABasicOperation::StreamSuccessCB(pa_stream* s, int success, void* userdata)
{ DEBUGLOG(("PABasicOperation::StreamSuccessCB(%p, %i, %p)\n", s, success, userdata));
  PABasicOperation& op = *(PABasicOperation*)userdata;
  op.Success = success;
  op.CompletionEvent(success);
  op.Signal();
}

void PAServerInfoOperation::ServerInfoCB(pa_context *c, const pa_server_info *i, void *userdata)
{ PAServerInfoOperation& op = *(PAServerInfoOperation*)userdata;
  op.InfoEvent(*i);
  op.Signal();
}

void PASinkInfoOperation::SinkInfoCB(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{ PASinkInfoOperation& op = *(PASinkInfoOperation*)userdata;
  op.InfoEvent(Args(i, eol < 0 ? pa_context_errno(c) : 0));
  if (eol)
    op.Signal();
}

void PASourceInfoOperation::SourceInfoCB(pa_context *c, const pa_source_info *i, int eol, void *userdata)
{ PASourceInfoOperation& op = *(PASourceInfoOperation*)userdata;
  op.InfoEvent(Args(i, eol < 0 ? pa_context_errno(c) : 0));
  if (eol)
    op.Signal();
}

pa_threaded_mainloop* volatile PAContext::Mainloop = NULL;
volatile unsigned              PAContext::RefCount = 0;


/*PAContext::PAContext(const char* appname) throw(PAInternalException)
{ MainloopRef();

  Context = pa_context_new(pa_threaded_mainloop_get_api(Mainloop), appname);
  if (!Context)
  { MainloopUnref();
    throw PAInternalException("Failed to initialize context.");
  }

  pa_context_set_state_callback(Context, &PAContext::StateCB, this);
}*/

PAContext::~PAContext()
{ DEBUGLOG(("PAContext(%p{%p})::~PAContext()\n", this, Context));
  Disconnect();
}

void PAContext::Connect(const char* appname, const char* server, pa_context_flags_t flags) throw(PAInternalException, PAConnectException)
{ DEBUGLOG(("PAContext(%p{%p})::Connect(%s, %s, %x)\n", this, Context, appname, server, flags));
  if (!Context)
    MainloopRef();
  // Discard any existing connection first
  Disconnect();
  //
  Context = pa_context_new(pa_threaded_mainloop_get_api(Mainloop), appname);
  if (!Context)
  { MainloopUnref();
    throw PAInternalException("Failed to initialize context.");
  }

  pa_context_set_state_callback(Context, &PAContext::StateCB, this);
  Lock lock; // synchronize
  if (pa_context_connect(Context, server, flags, NULL) != 0)
    throw PAConnectException(Context);
  //InterlockedInc(&RefCount);
  DEBUGLOG(("PAContext::Connect: %p\n", Context));
}

void PAContext::Disconnect()
{ DEBUGLOG(("PAContext(%p{%p})::Disconnect()\n", this, Context));
  if (!Context)
    return;
  { Lock lock;
    pa_context_state_t state = pa_context_get_state(Context);
    //pa_context_disconnect(Context);
    pa_context_unref(Context);
    Context = NULL;
    if (state != PA_CONTEXT_TERMINATED && state != PA_CONTEXT_FAILED)
      return; // delay MainloopUnref
    MainloopSignal();
  }
  MainloopUnref();
}

void PAContext::WaitReady() throw(PAConnectException)
{ DEBUGLOG(("PAContext(%p{%p})::WaitReady()\n", this, Context));
  Lock lock;

  for (;;)
  { DEBUGLOG(("PAContext::WaitReady - %i\n", pa_context_get_state(Context)));
    switch (pa_context_get_state(Context))
    {case PA_CONTEXT_READY:
      return;
     case PA_CONTEXT_FAILED:
      throw PAConnectException(Context);
     case PA_CONTEXT_TERMINATED:
      throw PAConnectException(Context, PA_ERR_CONNECTIONTERMINATED);
     default:
      throw PAConnectException(Context, PA_ERR_BADSTATE);
     case PA_CONTEXT_CONNECTING:
     case PA_CONTEXT_AUTHORIZING:
     case PA_CONTEXT_SETTING_NAME:;
    }
    MainloopWait();
  }
}

void PAContext::MainloopRef()
{ DEBUGLOG(("PAContext::MainloopRef() - %u\n", RefCount));
  if (InterlockedXad(&RefCount, 1) == 0)
  { // First instance => start main loop.
    pa_threaded_mainloop* mainloop = pa_threaded_mainloop_new();
    if (!mainloop)
      throw PAInternalException("Failed to initialize threaded mainloop.");
    pa_threaded_mainloop_start(mainloop);
    Mainloop = mainloop;
  } else
  { // Initialized by another instance
    // Dirty spin-lock to synchronize the other thread.
    while (Mainloop == NULL)
      DosSleep(1);
  }
}
void PAContext::MainloopUnref()
{ DEBUGLOG(("PAContext::MainloopUnref() - %u\n", RefCount));
  pa_threaded_mainloop* mainloop = Mainloop;
  ASSERT(mainloop);
  if (InterlockedDec(&RefCount))
    return;
  // Do not destroy a mainloop that might be instantiated by another thread.
  InterlockedCxc((volatile unsigned*)&Mainloop, (unsigned)mainloop, NULL);
  // destroy mainloop
  pa_threaded_mainloop_stop(mainloop);
  pa_threaded_mainloop_free(mainloop);
}

void PAContext::StateCB(pa_context* c, void* userdata)
{ PAContext* that = (PAContext*)userdata;
  pa_context_state_t newstate = pa_context_get_state(c);
  DEBUGLOG(("PAContext::StateCB(%p, %p): %i\n", c, userdata, newstate));
  if (c == that->Context)
    that->StateChangeEvent(newstate);
  else if (newstate == PA_CONTEXT_TERMINATED || newstate == PA_CONTEXT_FAILED)
    MainloopUnref();
  MainloopSignal();
}

void PAContext::GetServerInfo(PAServerInfoOperation& op) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_get_server_info(Context, &PAServerInfoOperation::ServerInfoCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "get_server_info failed");
}

void PAContext::GetSinkInfo(PASinkInfoOperation& op) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_get_sink_info_list(Context, &PASinkInfoOperation::SinkInfoCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "get_sink_info_list failed");
}
void PAContext::GetSinkInfo(PASinkInfoOperation& op, uint32_t index) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_get_sink_info_by_index(Context, index, &PASinkInfoOperation::SinkInfoCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "get_sink_info_by_index failed");
}
void PAContext::GetSinkInfo(PASinkInfoOperation& op, const char* name) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_get_sink_info_by_name(Context, name, &PASinkInfoOperation::SinkInfoCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "get_sink_info_by_name failed");
}

void PAContext::SetSinkPort(PABasicOperation& op, const char* sink, const char* port) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_set_sink_port_by_name(Context, sink, port, &PABasicOperation::ContextSuccessCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "pa_context_set_sink_port_by_name failed");
}

void PAContext::GetSourceInfo(PASourceInfoOperation& op) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_get_source_info_list(Context, &PASourceInfoOperation::SourceInfoCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "get_source_info_list failed");
}
void PAContext::GetSourceInfo(PASourceInfoOperation& op, uint32_t index) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_get_source_info_by_index(Context, index, &PASourceInfoOperation::SourceInfoCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "get_source_info_by_index failed");
}
void PAContext::GetSourceInfo(PASourceInfoOperation& op, const char* name) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_get_source_info_by_name(Context, name, &PASourceInfoOperation::SourceInfoCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "get_sink_info_by_name failed");
}

void PAContext::SetSourcePort(PABasicOperation& op, const char* source, const char* port) throw (PAContextException)
{ Lock lock;
  op.Attach(pa_context_set_source_port_by_name(Context, source, port, &PABasicOperation::ContextSuccessCB, &op));
  if (!op.IsValid())
    throw PAContextException(Context, "pa_context_set_source_port_by_name failed");
}


/*PAStream::PAStream(pa_context* context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map = NULL, pa_proplist* props = NULL)
{ memset(&Attr, -1, sizeof Attr);
  Stream = pa_stream_new_with_proplist(context, name, ss, map, props);
}*/

PAStream::~PAStream()
{ DEBUGLOG(("PAStream(%p{%p})::~PAStream()\n", this, Stream));
  if (Stream)
  { PAContext::Lock lock;
    Terminate = true;
    pa_stream_disconnect(Stream);
    if (WaitReadyPending)
      PAContext::MainloopSignal();
    pa_stream_unref(Stream);
  }
}

void PAStream::Create(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props)
throw(PAContextException)
{ DEBUGLOG(("PAStream(%p{%p})::Create(%p{%p}, %s, {%i,%i,%i}, %p, %p)\n", this, Stream,
    &context, context.GetContext(), name, ss->format, ss->rate, ss->channels, map, props));
  PAContext::Lock lock;
  // Discard any existing connection first
  if (Stream)
  { pa_stream_disconnect(Stream);
    if (WaitReadyPending)
      PAContext::MainloopSignal();
    pa_stream_unref(Stream);
  }

  Stream = pa_stream_new_with_proplist(context.GetContext(), name, ss, map, props);
  if (!Stream)
    throw PAContextException(context.GetContext(), xstring().sprintf("Failed to initialize stream %s for context: %s", name, pa_strerror(pa_context_errno(context.GetContext()))));
  pa_stream_set_state_callback(Stream, &PAStream::StateCB, this);
  Terminate = false;
}

void PAStream::Disconnect() throw()
{ DEBUGLOG(("PAStream(%p{%p})::Disconnect()\n", this, Stream));
  if (Stream)
  { PAContext::Lock lock;
    Terminate = true;
    pa_stream_unref(Stream);
    Stream = NULL;
    if (WaitReadyPending)
      PAContext::MainloopSignal();
  }
}

void PAStream::WaitReady() throw(PAStreamException)
{ DEBUGLOG(("PAStream(%p{%p})::WaitReady()\n", this, Stream));
  PAContext::Lock lock;

  for (;;)
  { if(Terminate)
      throw PAStreamEndException();
    DEBUGLOG(("PAStream::WaitReady - %i\n", pa_stream_get_state(Stream)));
    switch (pa_stream_get_state(Stream))
    {case PA_STREAM_READY:
      return;
     case PA_STREAM_FAILED:
      throw PAStreamException(Stream, "Failed to connect stream");
     case PA_STREAM_TERMINATED:
      throw PAStreamException(Stream, PA_ERR_BADSTATE, "Stream has terminated");
     default:
      throw PAStreamException(Stream, PA_ERR_BADSTATE, "Stream has not been connected");
     case PA_STREAM_CREATING:;
    }

    ++WaitReadyPending;
    PAContext::MainloopWait();
    --WaitReadyPending;
  }
}

pa_usec_t PAStream::GetTime() throw (PAStreamException)
{ pa_usec_t time;
  PAContext::Lock lock;
  if (GetState() != PA_STREAM_READY)
    return 0;
  int err = pa_stream_get_time(Stream, &time);
  if (err)
    throw PAStreamException(Stream, err, "GetTime failed");
  return time;
}

const pa_timing_info* PAStream::GetTimingInfo() throw ()
{ PAContext::Lock lock;
  if (GetState() != PA_STREAM_READY)
    return NULL;
  return pa_stream_get_timing_info(Stream);
}

void PAStream::Cork(bool pause) throw (PAStreamException)
{ DEBUGLOG(("PAStream(%p{%p})::Cork(%u)\n", this, Stream, pause));
  PAContext::Lock lock;
  pa_operation* op = pa_stream_cork(Stream, pause, NULL, NULL);
  if (op == NULL)
    throw PAStreamException(Stream, "Cork failed for stream");
  pa_operation_unref(op);
}

void PAStream::Flush() throw (PAStreamException)
{ DEBUGLOG(("PAStream(%p{%p})::Flush()\n", this, Stream));
  PAContext::Lock lock;
  if (GetState() != PA_STREAM_READY)
    return;
  pa_operation* op = pa_stream_flush(Stream, NULL, NULL);
  if (op == NULL)
    throw PAStreamException(Stream, "Flush failed for stream");
  pa_operation_unref(op);
}

void PAStream::StateCB(pa_stream *p, void *userdata) throw()
{ PAStream& str = *(PAStream*)userdata;
  if (str.WaitReadyPending)
    PAContext::MainloopSignal();
}


void PASinkOutput::Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
  const char* device, pa_stream_flags_t flags, const pa_cvolume* volume, pa_stream* sync_stream)
throw(PAContextException)
{ DEBUGLOG(("PASinkOutput(%p{%p})::Connect(...,%s, %x, %p, %p)\n", this, Stream,
    device, flags, volume, sync_stream));
  Create(context, name, ss, map, props);

  PAContext::Lock lock;
  pa_stream_set_write_callback(Stream, &PASinkOutput::WriteCB, this);

  if (pa_stream_connect_playback(Stream, device, &Attr, flags, volume, sync_stream) != 0)
    throw PAStreamException(Stream, xstring().sprintf("Failed to connect playback stream %s", name));
}

/*void PASinkOutput::WaitWritable()
{
  if (pa_stream_writable_size()
}*/

size_t PASinkOutput::WritableSize() throw (PAStreamException)
{ PAContext::Lock lock;
  size_t ret = pa_stream_writable_size(Stream);
  if (ret == (size_t)-1)
    throw PAStreamException(Stream, "Writable size undefined");
  return ret;
}

void* PASinkOutput::BeginWrite(size_t& size) throw(PAStreamException)
{ DEBUGLOG(("PASinkOutput(%p{%p})::BeginWrite(&%i)\n", this, Stream, size));
  PAContext::Lock lock;
  void* buffer;
  if (pa_stream_begin_write(Stream, &buffer, &size) != 0)
    throw PAStreamException(Stream, "BeginWrite failed for stream");
  DEBUGLOG(("PASinkOutput::BeginWrite: %p, %i\n", buffer, size));
  return buffer;
}

uint64_t PASinkOutput::Write(const void* data, size_t nbytes, int64_t offset, pa_seek_mode_t seek)
throw(PAStreamException)
{ DEBUGLOG(("PASinkOutput(%p{%p})::Write(%p, %u, %Li, %u)\n", this, Stream,
    data, nbytes, offset, seek));
  PAContext::Lock lock;
  for (;;)
  { size_t len;
    for (;;)
    { if (Terminate)
        throw PAStreamEndException();
      len = WritableSize();
      DEBUGLOG(("PASinkOutput::Write - %u writable bytes\n", len));
      if (len)
        break;
      ++WaitWritePending;
      PAContext::MainloopWait();
      --WaitWritePending;
    }
    if (len > nbytes)
      len = nbytes;
    if (pa_stream_write(Stream, data, len, NULL, offset, seek) != 0)
      throw PAStreamException(Stream, "Write failed for stream");
    nbytes -= len;
    if (nbytes == 0)
      break;
    (const char*&)data += len;
    offset = 0;
    seek = PA_SEEK_RELATIVE;
  }
  const pa_timing_info* ti = pa_stream_get_timing_info(Stream);
  // Hmm, sometimes ti is zero.
  return ti ? ti->write_index : 0;
}

void PASinkOutput::RunDrain(PABasicOperation& op) throw (PAStreamException)
{ DEBUGLOG(("PASinkOutput(%p{%p})::RunDrain(&%p)\n", this, Stream, &op));
  PAContext::Lock lock;
  op.Cancel();
  op.Attach(pa_stream_drain(Stream, &PABasicOperation::StreamSuccessCB, &op));
  if (!op.IsValid())
    throw PAStreamException(Stream, "Drain failed for stream");
}

void PASinkOutput::SetVolume(const pa_cvolume* volume) throw (PAStreamException)
{ DEBUGLOG(("PASinkOutput(%p{%p})::SetVolume({%u,...})\n", this, Stream, volume->channels));
  PAContext::Lock lock;
  pa_operation* op = pa_context_set_sink_input_volume(pa_stream_get_context(Stream), pa_stream_get_index(Stream), volume, NULL, NULL);
  if (op == NULL)
    throw PAStreamException(Stream, "SetVolume failed for stream");
  pa_operation_unref(op);
}

void PASinkOutput::SetVolume(pa_volume_t volume) throw (PAStreamException)
{ PACVolume vol(pa_stream_get_sample_spec(Stream)->channels, volume);
  SetVolume(&vol);
}

void PASinkOutput::WriteCB(pa_stream *p, size_t nbytes, void *userdata) throw()
{ DEBUGLOG(("PASinkOutput::WriteCB(%p, %u, %p)\n", p, nbytes, userdata));
  PASinkOutput& si = *(PASinkOutput*)userdata;
  if (si.WaitWritePending)
    PAContext::MainloopSignal();
}


void PASourceInput::Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
  const char* device, pa_stream_flags_t flags) throw(PAContextException)
{ DEBUGLOG(("PASourceInput(%p{%p})::Connect(...,%s, %x,)\n", this, Stream, device, flags));
  Create(context, name, ss, map, props);

  PAContext::Lock lock;
  pa_stream_set_read_callback(Stream, &PASourceInput::ReadCB, this);

  if (pa_stream_connect_record(Stream, device, &Attr, flags) != 0)
    throw PAStreamException(Stream, xstring().sprintf("Failed to connect record stream %s", name));
}

size_t PASourceInput::ReadableSize() throw (PAStreamException)
{ PAContext::Lock lock;
  size_t ret = pa_stream_readable_size(Stream);
  if (ret == (size_t)-1)
    throw PAStreamException(Stream, "Readable size undefined");
  return ret;
}

const void* PASourceInput::Peek(size_t& nbytes) throw (PAStreamException)
{ DEBUGLOG(("PASourceInput(%p{%p})::Peek()\n", this, Stream));
  PAContext::Lock lock;
  do
  { const void* data;
    if (pa_stream_peek(Stream, &data, &nbytes) != 0)
      throw PAStreamException(Stream, "Peek failed for stream");
    if (nbytes)
      return data;

    ++WaitReadPending;
    PAContext::MainloopWait();
    --WaitReadPending;

  } while (!Terminate);
  throw PAStreamEndException();
}

void PASourceInput::Drop() throw (PAStreamException)
{ DEBUGLOG(("PASourceInput(%p{%p})::Drop()\n", this, Stream));
  PAContext::Lock lock;
  if (pa_stream_drop(Stream) != 0)
    throw PAStreamException(Stream, "Drop failed for stream");
}

void PASourceInput::ReadCB(pa_stream *p, size_t nbytes, void *userdata) throw()
{ PASourceInput& si = *(PASourceInput*)userdata;
  DEBUGLOG(("PASourceInput::ReadCB(%p, %u, %p) - %i\n", p, nbytes, userdata, si.WaitReadPending));
  if (si.WaitReadPending)
    PAContext::MainloopSignal();
}
