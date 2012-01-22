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

/* C++ Wrappers to PulseAudio API, incomplete */

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/proplist.h>
#include <pulse/error.h>
#include <pulse/stream.h>
#include <pulse/introspect.h>
#include <cpp/cpputil.h>
#include <cpp/event.h>
#include <cpp/xstring.h>
#include <limits.h>

#include <plugin.h>


class PAException
{protected:
  int       Error;
  xstring   Message;
 protected:
  PAException()                                 {}
 public:
  explicit PAException(int err)                 : Error(err) { if (err) Message = pa_strerror(err); }
  PAException(int err, const xstring& message)  : Error(err), Message(message) {}
  int            GetError() const               { return Error; }
  const xstring& GetMessage() const             { return Message; }
};

class PAStateException : public PAException
{public:
  explicit PAStateException(const xstring& message)  : PAException(PA_ERR_BADSTATE, message) {}
};

class PAInternalException : public PAException
{public:
  PAInternalException(const xstring& message)   : PAException(PA_ERR_INTERNAL, message) {}
};

class PAContextException : public PAException
{protected:
  PAContextException()                          {}
 public:
  explicit PAContextException(pa_context* c)    : PAException(pa_context_errno(c)) {}
  PAContextException(pa_context* c, const xstring& message)
                                                : PAException(pa_context_errno(c), message) {}
  PAContextException(int err, const xstring& message) : PAException(err, message) {}
};

class PAConnectException : public PAContextException
{public:
  PAConnectException(pa_context* c)             : PAContextException(c, xstring().sprintf("Failed to connect to server %s: %s", pa_context_get_server(c), pa_strerror(pa_context_errno(c)))) {}
  PAConnectException(pa_context* c, int err)    : PAContextException(err, xstring().sprintf("Failed to connect to server %s: %s", pa_context_get_server(c), pa_strerror(err))) {}
  //PAConnectException(int err, const xstring& message) : PAContextException(err, message) {}
};

class PAStreamException : public PAContextException
{public:
  PAStreamException(pa_stream* s, const char* msg);
  PAStreamException(pa_stream* s, int err, const char* msg);
  //PAStreamException(pa_stream* c, int err)      : PAContextException(err, xstring::sprintf("Failed to connect stream %s: %s", err, pa_strerror(err))) {}
};

/*class PAStreamConnectException : public PAContextException
{public:
  PAStreamConnectException(pa_context* c)       : PAContextException(c, xstring::sprintf("Failed to connect stream %s: %s", pa_context_get_server(c), pa_strerror(pa_context_errno(c)))) {}
  PAStreamConnectException(pa_context* c, int err) : PAContextException(err, xstring::sprintf("Failed to connect stream %s: %s", err, pa_strerror(err))) {}
  //PAConnectException(int err, const xstring& message) : PAContextException(err, message) {}
};*/


struct PASampleSpec : public pa_sample_spec
{ PASampleSpec()                                {}
  PASampleSpec(pa_sample_format_t format, uint8_t channels, uint32_t samplerate)
                                                { this->format = format; this->channels = channels; rate = samplerate; }
  void assign(pa_sample_format_t format, uint8_t channels, uint32_t samplerate)
                                                { this->format = format; this->channels = channels; rate = samplerate; }
  operator const pa_sample_spec*() const        { return this; }
  friend bool operator==(const PASampleSpec& l, const PASampleSpec& r)
                                                { return !!pa_sample_spec_equal(&l, &r); }
  friend bool operator!=(const PASampleSpec& l, const PASampleSpec& r)
                                                { return !pa_sample_spec_equal(&l, &r); }
  bool      IsValid() const                     { return !!pa_sample_spec_valid(this); }

  size_t    BytesPerSecond() const              { return pa_bytes_per_second(this); }
  size_t    FrameSize() const                   { return pa_frame_size(this); }
  size_t    SampleSize() const                  { return pa_sample_size(this); }
  pa_usec_t Bytes2uSec(uint64_t length) const   { return pa_bytes_to_usec(length, this); }
};

struct PACVolume : public pa_cvolume
{
  PACVolume()                                   {}
  PACVolume(uint8_t channels, pa_volume_t volume) { Set(channels, volume); }
  operator const pa_cvolume*()                  { return this; }

  bool      IsValid() const                     { return pa_cvolume_valid(this); }
  void      Set(uint8_t channels, pa_volume_t volume) { this->channels = channels; pa_cvolume_set(this, channels, volume); }
};

/**
 * Wrap a Pulse Audio property list to an STL associative container.
 */
class PAProplist
{public:
  typedef const char* key_type;
  typedef const char* value_type;
  typedef value_type* pointer;
  typedef int         difference_type;
  typedef unsigned    size_type;
  typedef const value_type const_reference;
  class reference
  {private:
    pa_proplist* const PL;
    key_type     const Key;
   public:
    reference(pa_proplist* pl, key_type key)    : PL(pl), Key(key) {}
    void operator=(value_type value);
    operator const_reference() const            { return pa_proplist_gets(PL, Key); }
  };
  class iterator
  { friend class PAProplist;
   private:
    pa_proplist* PL;
    key_type     Key;
    void*        State;
   protected:
    iterator(pa_proplist* pl)                   : PL(pl), Key(NULL), State(NULL) {}
   public:
    iterator()                                  {}
    const_reference operator*() const           { return pa_proplist_gets(PL, Key); }
    reference operator*()                       { return reference(PL, Key); }
    iterator& operator++(int)                   { Key = pa_proplist_iterate(PL, &State); return *this; }
    iterator  operator++()                      { iterator tmp(*this); ++*this; return tmp; }
    friend bool operator==(const iterator& l, const iterator& r)
                                                { return l.Key == r.Key; }
    friend bool operator!=(const iterator& l, const iterator& r)
                                                { return l.Key != r.Key; }
  };
  typedef const iterator const_iterator;
 private:
  class begin_iterator : public iterator
  {public:
    begin_iterator(pa_proplist* pl)             : iterator(pl) { ++*this; }
  };

 private:
  pa_proplist* PL;

 public:
  PAProplist()                                  : PL(pa_proplist_new()) {}
  PAProplist(const PAProplist& r)               : PL(pa_proplist_copy(r.PL)) {}
  ~PAProplist()                                 { pa_proplist_free(PL); }
  PAProplist& operator=(const PAProplist& r)    { pa_proplist_update(PL, PA_UPDATE_SET, r.PL); return *this; }
  void swap(PAProplist& r)                      { pa_proplist* tmp = r.PL; r.PL = PL; PL = tmp; }
  operator pa_proplist*()                       { return PL; }

  const_reference operator[](key_type key) const { return pa_proplist_gets(PL, key); }
  reference operator[](key_type key)            { return reference(PL, key); }

  iterator begin()                              { return begin_iterator(PL); }
  iterator end()                                { return iterator(PL); }
  const_iterator begin() const                  { return begin_iterator(PL); }
  const_iterator end() const                    { return iterator(PL); }

  bool empty() const                            { return pa_proplist_size(PL) == 0; }
  size_type size() const                        { return pa_proplist_size(PL); }
  size_type max_size() const                    { return INT_MAX; }
  size_type count(key_type key) const           { return pa_proplist_contains(PL, key); }
  bool contains(key_type key) const             { return (bool)pa_proplist_contains(PL, key); }
  void erase(key_type key)                      { pa_proplist_unset(PL, key); }
  void erase(iterator& i)                       { pa_proplist_unset(PL, i.Key); }
  void clear()                                  { pa_proplist_clear(PL); }
};

/* * Abstract interface to track all asynchronous operations.
 *
class PAAsync
{private: // non-copyable
  PAAsync(const PAAsync& r);
  void operator=(const PAAsync& r);
 public:
  //PAOperation()                                 : Operation(NULL) {}
  virtual ~PAAsync()                            {}

  virtual pa_operation_state_t GetState() const = 0;
  virtual void Cancel() = 0;
  virtual void Wait() = 0;
};*/

/*class PAConnect : public PAAsync
{public:
  pa_context_state_t GetConnectState() const    { return pa_context_get_state(Context); }
  virtual pa_operation_state_t GetState() const = 0;
  virtual void Cancel() = 0;
  virtual void Wait() = 0;
  virtual void OnStateChange(pa_context_state_t state) {}
  virtual void OnCompleteion(PAContext c)       {}
 public:
  static void StateCB(pa_context *c, void *userdata);
};*/

class PAOperation //: public PAAsync
{protected:
  pa_operation*      Operation;
  bool               WaitPending;

 protected:
  void               Unlink();
 private: // non-copyable
  PAOperation(const PAOperation& r);//             : Operation(r.Operation) { pa_operation_ref(Operation); }
  PAOperation& operator=(const PAOperation& r);//  { Unlink(); Operation = r.Operation; return *this; }
 public:
  PAOperation()                                 : Operation(NULL) {}
  //PAOperation(pa_operation* op)                 : Operation(op) {}
  ~PAOperation()                                { Unlink(); }
  void Attach(pa_operation* op)                 { Unlink(); Operation = op; }

  bool IsValid() const                          { return Operation != NULL; }
  pa_operation_state_t GetState() const         { return Operation ? pa_operation_get_state(Operation) : PA_OPERATION_CANCELED; }
  void Cancel();
  void Wait() throw (PAStateException);
};

/** Track an asynchronous operation.
 * You might derive from this class to receive the completion events.
 */
class PABasicOperation : public PAOperation
{private:
  int Success;
 protected:
  event<const int> CompletionEvent;
 public:
  int FetchResult() throw (PAStateException)    { Wait(); return Success; }
  event_pub<const int>& Completion()            { return CompletionEvent; }

  static void ContextSuccessCB(pa_context* c, int success, void* userdata);
  static void StreamSuccessCB(pa_stream* c, int success, void* userdata);
};

/** Wraps a PulseAudio context.
 */
class PAContext
{private:
  /// Global main loop.
  static pa_threaded_mainloop* volatile Mainloop;
  /// Counter with references to any mainloop related object.
  /// I.e. any instance of PAContext \e and any connected pa_context.
  static volatile unsigned              RefCount;
 protected:
  pa_context*                           Context;
  event<const pa_context_state_t>       StateChangeEvent;
 public:
  /** Synchronize to the mainloop
   * The lock is held as long as this object exists.
   * The lock also takes care of priority inversion.
   */
  class Lock;
  friend class Lock;
  class Lock
  {public:
    Lock()                                      { pa_threaded_mainloop_lock(PAContext::Mainloop); }
    ~Lock()                                     { pa_threaded_mainloop_unlock(PAContext::Mainloop); }
   private: // non-copyable
    Lock(const Lock&);
    void operator=(const Lock&);
  };

 private:
  static void MainloopRef();
  static void MainloopUnref();
  static void StateCB(pa_context *c, void *userdata);
 private: // non-copyable
  PAContext(const PAContext& r);
  void operator=(const PAContext& r);
 public:
  PAContext()                                   : Context(NULL) {}
  //explicit PAContext(const char* appname) throw(PAInternalException);
  //explicit PAContext(pa_context* c)             : Context(c) { pa_context_ref(Context); MainloopRef(); }
  ~PAContext();
  //PAContext(const PAContext& r)                 : Context(r.Context) { pa_context_ref(Context); MainloopRef(); }
  //PAContext& operator=(const PAContext& r);

  void Connect(const char* appname, const char* server, pa_context_flags_t flags = PA_CONTEXT_NOFLAGS) throw(PAInternalException, PAConnectException);
  void Disconnect()                             { if (Context) pa_context_disconnect(Context); }
  pa_context* GetContext()                      { return Context; }
  pa_context_state_t GetState() const           { return Context ? pa_context_get_state(Context) : PA_CONTEXT_UNCONNECTED; }
  void WaitReady() throw(PAConnectException);
  /// @return Observable to track context state changes.
  event_pub<const pa_context_state_t>& StateChange() { return StateChangeEvent; }

  static void MainloopWait()                    { pa_threaded_mainloop_wait(Mainloop); }
  static void MainloopSignal(bool wait_accept = false) { pa_threaded_mainloop_signal(Mainloop, wait_accept); }
};


FLAGSATTRIBUTE(pa_stream_flags_t);

/** Sound stream
 */
class PAStream
{public:
  pa_buffer_attr     Attr;
 protected:
  pa_stream*         Stream;
 private:
  int WaitReadyPending;

 private: // non-copyable
  PAStream(const PAStream& r);
  void operator=(const PAStream& r);
 protected:
  PAStream()                                    : Stream(NULL), WaitReadyPending(0) { memset(&Attr, -1, sizeof Attr); }
  //PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map = NULL, pa_proplist* props = NULL);
  void Create(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props)
    throw(PAContextException);
 public:
  ~PAStream();                                  // { if (Stream) pa_stream_unref(Stream); }
  //operator pa_stream*()                         { return Stream; }
  void Disconnect() throw();

  /*int Record(const char* device, pa_stream_flags_t flags)
                                                { return pa_stream_connect_record(Stream, device, &Attr, flags); }*/
  pa_stream_state_t GetState() const throw()    { return Stream ? pa_stream_get_state(Stream) : PA_STREAM_UNCONNECTED; }
  void WaitReady() throw (PAStreamException);

  pa_usec_t GetTime() throw (PAStreamException);
  void Cork(bool pause) throw (PAStreamException);
  void Flush() throw (PAStreamException);
 private:
  static void StateCB(pa_stream *p, void *userdata) throw();
};

class PASinkInput : public PAStream
{private:
  int WaitWritePending;
 public:
  PASinkInput()                                 : WaitWritePending(0) {}

  void Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
               const char* device, pa_stream_flags_t flags, const pa_cvolume* volume = NULL, pa_stream* sync_stream = NULL)
       throw(PAContextException);
  void Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
               const char* device, pa_stream_flags_t flags, pa_volume_t volume, pa_stream* sync_stream = NULL)
       throw(PAContextException)                { Connect(context, name, ss, map, props, device, flags, PACVolume(ss->channels, volume), sync_stream); }

  size_t WritableSize() throw (PAStreamException);
  void* BeginWrite(size_t& size) throw (PAStreamException);
  uint64_t Write(const void* data, size_t nbytes, int64_t offset = 0, pa_seek_mode_t seek = PA_SEEK_RELATIVE) throw(PAStreamException);
  void RunDrain(PABasicOperation& op) throw (PAStreamException);

  void SetVolume(const pa_cvolume* volume) throw (PAStreamException);
  void SetVolume(pa_volume_t volume) throw (PAStreamException);
 private:
  static void WriteCB(pa_stream *p, size_t nbytes, void *userdata) throw();
};

class PASourceOutput : public PAStream
{

};

class PASample : public PAStream
{

};
