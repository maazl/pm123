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

#define INCL_DOS
#include "pawrapper.h"

#include <interlocked.h>
#include <memory.h>
#include <os2.h>
#include <debuglog.h>


PAStreamException::PAStreamException(pa_stream* s, const char* msg)
{ pa_context* context = pa_stream_get_context(s);
  if (!context)
  { // No context ???
    Error = PA_ERR_INTERNAL;
    Message.sprintf("%s, no details available (Stream has no context)", msg);
  } else
  { Error = pa_context_errno(context);
    Message.sprintf("%s, server %s: %s", msg, pa_context_get_server(context), pa_strerror(Error));
  }
}

PAStreamException::PAStreamException(pa_stream* s, int err, const char* msg)
{ Error = err;
  pa_context* context = pa_stream_get_context(s);
  if (!context)
    Message.sprintf("%s (Stream has no context): %s", msg, pa_strerror(Error));
  else
    Message.sprintf("%s, server %s: %s", msg, pa_context_get_server(context), pa_strerror(Error));
}

void PAProplist::reference::operator=(const char* value)
{ if (value)
    pa_proplist_sets(PL, Key, value);
  else
    pa_proplist_unset(PL, Key);
}


void PAOperation::Unlink()
{ if (Operation)
  { PAContext::Lock lock;
    // The current pulseaudio implementation has no option to disable a callback
    // without canceling the operation. So the destruction of the Operation wrapper
    // must necessarily cancel the operation to avoid undefined bahavior of
    // the outstanding callback.
    pa_operation_cancel(Operation);
    if (WaitPending)
      PAContext::MainloopSignal();
    pa_operation_unref(Operation);
    Operation = NULL;
  }
}

void PAOperation::Cancel()
{ DEBUGLOG(("PAOperation(%p{%p})::Cancel()\n", this, Operation));
  // Double check: fast path without lock.
  if (!Operation || pa_operation_get_state(Operation) != PA_OPERATION_RUNNING)
    return;
  PAContext::Lock lock;
  pa_operation_cancel(Operation);
  if (WaitPending)
    PAContext::MainloopSignal();
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
  WaitPending = true;
  PAContext::Lock lock;
  for (;;)
  { PAContext::MainloopWait();
    DEBUGLOG(("PAOperation::Wait() - %i\n", pa_operation_get_state(Operation)));
    switch (pa_operation_get_state(Operation))
    {case PA_OPERATION_DONE:
      return;
     case PA_OPERATION_CANCELLED:
      throw PAStateException("Operation canceled");
     default:;
    }
  }
}

void PABasicOperation::ContextSuccessCB(pa_context* c, int success, void* userdata)
{ DEBUGLOG(("PABasicOperation::StreamSuccessCB(%p, %i, %p)\n", c, success, userdata));
  PABasicOperation& op = *(PABasicOperation*)userdata;
  op.Success = success;
  op.CompletionEvent(success);
  if (op.WaitPending)
    PAContext::MainloopSignal();
}
void PABasicOperation::StreamSuccessCB(pa_stream* s, int success, void* userdata)
{ DEBUGLOG(("PABasicOperation::StreamSuccessCB(%p, %i, %p)\n", s, success, userdata));
  PABasicOperation& op = *(PABasicOperation*)userdata;
  op.Success = success;
  op.CompletionEvent(success);
  if (op.WaitPending)
    PAContext::MainloopSignal();
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
  if (Context)
  { pa_context_disconnect(Context);
    { Lock lock;
      MainloopSignal();
    }
    pa_context_unref(Context);
    //Context = NULL;
    MainloopUnref();
  }
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
     default:;
    }

    MainloopWait();
  }
}

void PAContext::MainloopRef()
{ if (InterlockedXad(&RefCount, 1) == 0)
  { // First instance => start main loop.
    Mainloop = pa_threaded_mainloop_new();
    if (!Mainloop)
      throw PAInternalException("Failed to initialize threaded mainloop.");
    pa_threaded_mainloop_start(Mainloop);
  } else
  { // Initialized by another instance
    // Dirty spin-lock to synchronize the other thread.
    while (Mainloop == NULL)
      DosSleep(1);
  }
}
void PAContext::MainloopUnref()
{ pa_threaded_mainloop* mainloop = Mainloop;
  if (InterlockedDec(&RefCount))
    return;
  // Do not destroy a mainloop that might be instantiated by another thread.
  InterlockedCxc((volatile unsigned*)&Mainloop, (unsigned)mainloop, NULL);
  // destroy mainloop
  pa_threaded_mainloop_stop(mainloop);
  pa_threaded_mainloop_free(mainloop);
}

void PAContext::StateCB(pa_context* c, void* userdata)
{ DEBUGLOG(("PAContext::StateCB(%p, %p)\n", c, userdata));
  PAContext* that = (PAContext*)userdata;
  that->StateChangeEvent(pa_context_get_state(c));
  /*if (newstate == PA_CONTEXT_TERMINATED || newstate == PA_CONTEXT_FAILED)
    MainloopUnref();*/
  MainloopSignal();
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
    pa_stream_disconnect(Stream);
    if (WaitReadyPending)
      PAContext::MainloopSignal();
  }
}

void PAStream::WaitReady() throw(PAStreamException)
{ DEBUGLOG(("PAStream(%p{%p})::WaitReady()\n", this, Stream));
  PAContext::Lock lock;

  for (;;)
  { if(Terminate)
      goto term;
    DEBUGLOG(("PAStream::WaitReady - %i\n", pa_stream_get_state(Stream)));
    switch (pa_stream_get_state(Stream))
    {case PA_STREAM_READY:
      return;
     case PA_STREAM_FAILED:
      throw PAStreamException(Stream, "Failed to connect stream");
     case PA_STREAM_TERMINATED:
     term:
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

void PAStream::Cork(bool pause) throw (PAStreamException)
{ DEBUGLOG(("PAStream(%p{%p})::Cork(%u)\n", this, Stream, pause));
  PAContext::Lock lock;
  if (GetState() != PA_STREAM_READY)
    return;
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


void PASinkInput::Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
               const char* device, pa_stream_flags_t flags, const pa_cvolume* volume, pa_stream* sync_stream)
throw(PAContextException)
{ DEBUGLOG(("PASinkInput(%p{%p})::Connect(...,%s, %x, %p, %p)\n", this, Stream,
    device, flags, volume, sync_stream));
  Create(context, name, ss, map, props);

  pa_stream_set_write_callback(Stream, &PASinkInput::WriteCB, this);

  PAContext::Lock lock;
  if (pa_stream_connect_playback(Stream, device, &Attr, flags, volume, sync_stream) != 0)
    throw PAStreamException(Stream, xstring().sprintf("Failed to connect playback stream %s", name));
}

/*void PASinkInput::WaitWritable()
{
  if (pa_stream_writable_size()
}*/

size_t PASinkInput::WritableSize() throw (PAStreamException)
{ size_t ret = pa_stream_writable_size(Stream);
  if (ret == (size_t)-1)
    throw PAStreamException(Stream, "WritableSize undefined");
  return ret;
}

void* PASinkInput::BeginWrite(size_t& size) throw(PAStreamException)
{ DEBUGLOG(("PASinkInput(%p{%p})::BeginWrite(&%i)\n", this, Stream, size));
  PAContext::Lock lock;
  void* buffer;
  if (pa_stream_begin_write(Stream, &buffer, &size) != 0)
    throw PAStreamException(Stream, "BeginWrite failed for stream");
  DEBUGLOG(("PASinkInput::BeginWrite: %p, %i\n", buffer, size));
  return buffer;
}

uint64_t PASinkInput::Write(const void* data, size_t nbytes, int64_t offset, pa_seek_mode_t seek)
throw(PAStreamException)
{ DEBUGLOG(("PASinkInput(%p{%p})::Write(%p, %u, %Li, %u)\n", this, Stream,
    data, nbytes, offset, seek));
  PAContext::Lock lock;
  for (;;)
  { size_t len;
    for (;;)
    { if (Terminate)
        throw PAStreamException(Stream, PA_ERR_BADSTATE, "Stream has terminated");
      len = WritableSize();
      DEBUGLOG(("PASinkInput::Write - %u writable bytes\n", len));
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

void PASinkInput::RunDrain(PABasicOperation& op) throw (PAStreamException)
{ DEBUGLOG(("PASinkInput(%p{%p})::RunDrain(&%p)\n", this, Stream, &op));
  PAContext::Lock lock;
  op.Cancel();
  op.Attach(pa_stream_drain(Stream, &PABasicOperation::StreamSuccessCB, &op));
  if (!op.IsValid())
    throw PAStreamException(Stream, "Drain failed for stream");
}

void PASinkInput::SetVolume(const pa_cvolume* volume) throw (PAStreamException)
{ DEBUGLOG(("PASinkInput(%p{%p})::SetVolume({%u,...})\n", this, Stream, volume->channels));
  PAContext::Lock lock;
  pa_operation* op = pa_context_set_sink_input_volume(pa_stream_get_context(Stream), pa_stream_get_index(Stream), volume, NULL, NULL);
  if (op == NULL)
    throw PAStreamException(Stream, "SetVolume failed for stream");
  pa_operation_unref(op);
}

void PASinkInput::SetVolume(pa_volume_t volume) throw (PAStreamException)
{ SetVolume(PACVolume(pa_stream_get_sample_spec(Stream)->channels, volume));
}

void PASinkInput::WriteCB(pa_stream *p, size_t nbytes, void *userdata) throw()
{ DEBUGLOG(("PASinkInput::WriteCB(%p, %u, %p)\n", p, nbytes, userdata));
  PASinkInput& si = *(PASinkInput*)userdata;
  if (si.WaitWritePending)
    PAContext::MainloopSignal();
}
