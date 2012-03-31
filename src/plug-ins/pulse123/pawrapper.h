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

#ifndef PAWRAPPER_H
#define PAWRAPPER_H

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

#include <debuglog.h>


class PAException
{private:
  int       Error;
  xstring   Message;
 protected:
  PAException()                                 {}
  void           SetException(int err, const xstring& message);
 public:
  explicit PAException(int err)                 : Error(0) { if (err) SetException(err, pa_strerror(err)); }
  PAException(int err, const xstring& message)  { SetException(err, message); }
  int            GetError() const               { return Error; }
  const xstring& GetMessage() const             { return Message; }
};

class PAStateException : public PAException
{public:
  explicit PAStateException(const xstring& message) : PAException(PA_ERR_BADSTATE, message) {}
};

class PAInternalException : public PAException
{public:
  explicit PAInternalException(const xstring& message) : PAException(PA_ERR_INTERNAL, message) {}
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


struct PABufferAttr : public pa_buffer_attr
{ PABufferAttr()                                { memset((pa_buffer_attr*)this, -1, sizeof(pa_buffer_attr)); }
  PABufferAttr(const pa_buffer_attr& r)         { (pa_buffer_attr&)*this = r; }
  PABufferAttr& operator=(const pa_buffer_attr& r){ (pa_buffer_attr&)*this = r; return *this; }
};

struct PASampleSpec : public pa_sample_spec
{ PASampleSpec()                                {}
  PASampleSpec(const pa_sample_spec& r)         { (pa_sample_spec&)*this = r; }
  PASampleSpec& operator=(const pa_sample_spec& r){ (pa_sample_spec&)*this = r; return *this; }
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
{ PACVolume()                                   {}
  PACVolume(const pa_cvolume& r)                { (pa_cvolume&)*this = r; }
  PACVolume(uint8_t channels, pa_volume_t volume) { Set(channels, volume); }
  PACVolume& operator=(const pa_cvolume& r)     { (pa_cvolume&)*this = r; return *this; }

  bool      IsValid() const                     { return pa_cvolume_valid(this); }
  void      Set(uint8_t channels, pa_volume_t volume) { this->channels = channels; pa_cvolume_set(this, channels, volume); }
};

struct PAChannelMap : public pa_channel_map
{ PAChannelMap()                                { channels = 0; }
  PAChannelMap(const pa_channel_map& r)         { (pa_channel_map&)*this = r; }
  PAChannelMap& operator=(const pa_channel_map& r) { (pa_channel_map&)*this = r; return *this; }
  // TODO
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
  PAProplist(const pa_proplist* r)              : PL(r ? pa_proplist_copy((pa_proplist*)r) : pa_proplist_new()) {}
  ~PAProplist()                                 { pa_proplist_free(PL); }
  PAProplist& operator=(const PAProplist& r)    { pa_proplist_update(PL, PA_UPDATE_SET, r.PL); return *this; }
  void swap(PAProplist& r)                      { pa_proplist* tmp = r.PL; r.PL = PL; PL = tmp; }
  operator pa_proplist*()                       { return PL; }

  const_reference operator[](key_type key) const{ return pa_proplist_gets(PL, key); }
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

struct PAServerInfo
{ xstring            user_name;                 /**< User name of the daemon process */
  xstring            host_name;                 /**< Host name the daemon is running on */
  xstring            server_version;            /**< Version string of the daemon */
  xstring            server_name;               /**< Server package name (usually "pulseaudio") */
  PASampleSpec       sample_spec;               /**< Default sample specification */
  xstring            default_sink_name;         /**< Name of default sink. */
  xstring            default_source_name;       /**< Name of default sink. */
  uint32_t           cookie;                    /**< A random cookie for identifying this instance of PulseAudio. */
  PAChannelMap       channel_map;               /**< Default channel map. \since 0.9.15 */

  PAServerInfo()                                {}
  PAServerInfo& operator=(const pa_server_info& r);
  PAServerInfo(const pa_server_info& r)         { *this = r; }
};

struct PAFormatInfo
{ pa_encoding_t      encoding;                  /**< The encoding used for the format */
  PAProplist         plist;                     /**< Additional encoding-specific properties such as sample rate, bitrate, etc. */

  PAFormatInfo()                                {}
  PAFormatInfo& operator=(const pa_format_info& r) { encoding = r.encoding; plist = r.plist; return *this; }
  PAFormatInfo(const pa_format_info& r)         { *this = r; }
};

struct PAPortInfo
{ xstring            name;                      /**< Name of this port */
  xstring            description;               /**< Description of this port */
  uint32_t           priority;                  /**< The higher this value is the more useful this port is as a default */

  PAPortInfo()                                  {}
  PAPortInfo& operator=(const pa_sink_port_info& r) { name = r.name; description = r.description; priority = r.priority; return *this; }
  PAPortInfo& operator=(const pa_source_port_info& r) { name = r.name; description = r.description; priority = r.priority; return *this; }
  PAPortInfo(const pa_sink_port_info& r)        { *this = r; }
  PAPortInfo(const pa_source_port_info& r)      { *this = r; }
};

/*struct PASinkPortInfo : PAPortInfo
{ PASinkPortInfo()                              {}
  PASinkPortInfo(const pa_sink_port_info& r)    { *this = r; }
};

struct PASourcePortInfo : PAPortInfo
{ PASourcePortInfo()                            {}
  PASourcePortInfo(const pa_source_port_info& r){ *this = r; }
};*/

struct PAInfo
{ xstring            name;                      /**< Name of the sink or source */
  uint32_t           index;                     /**< Index of the sink or source */
  xstring            description;               /**< Description of this sink or source */
  PASampleSpec       sample_spec;               /**< Sample spec of this sink or source */
  PAChannelMap       channel_map;               /**< Channel map of this sink or source */
  uint32_t           owner_module;              /**< Owning module index, or PA_INVALID_INDEX */
  PACVolume          volume;                    /**< Volume of the sink or source */
  int                mute;                      /**< Mute switch of the sink or source */
  uint32_t           monitor;                   /**< Index of the monitor source connected to this sink or vice versa */
  xstring            monitor_name;              /**< The name of the monitor source or sink */
  pa_usec_t          latency;                   /**< Length of queued audio in the output buffer or the filled record buffer of this source respectively. */
  xstring            driver;                    /**< Driver name. */
  PAProplist         proplist;                  /**< Property list \since 0.9.11 */
  pa_usec_t          configured_latency;        /**< The latency this device has been configured to. \since 0.9.11 */
  pa_volume_t        base_volume;               /**< Some kind of "base" volume that refers to unamplified/unattenuated volume in the context of the output device. \since 0.9.15 */
  uint32_t           n_volume_steps;            /**< Number of volume steps for sinks or sources which do not support arbitrary volumes. \since 0.9.15 */
  uint32_t           card;                      /**< Card index, or PA_INVALID_INDEX. \since 0.9.15 */
  sco_arr<PAPortInfo> ports;                    /**< Array of available ports. \since 0.9.16 */
  PAPortInfo*        active_port;               /**< Pointer to active port in the array, or NULL \since 0.9.16 */
  sco_arr<PAFormatInfo> formats;                /**< Array of formats supported by the sink or source. \since 1.0 */
};

struct PASinkInfo : public PAInfo
{ pa_sink_flags_t    flags;                     /**< Flags */
  pa_sink_state_t    state;                     /**< State \since 0.9.15 */

  PASinkInfo& operator=(const pa_sink_info& r);
  PASinkInfo(const pa_sink_info& r)             { *this = r; }
};

struct PASourceInfo : public PAInfo
{ pa_source_flags_t  flags;                     /**< Flags */
  pa_source_state_t  state;                     /**< State \since 0.9.15 */

  PASourceInfo& operator=(const pa_source_info& r);
  PASourceInfo(const pa_source_info& r)          { *this = r; }
};


class PAOperation
{protected:
  pa_operation*      Operation;
  pa_operation_state_t FinalState;
  int                WaitPending;

 protected:
  void               Unlink();
  /// Signal command completion or error.
  void               Signal();
 private: // non-copyable
  PAOperation(const PAOperation& r);//             : Operation(r.Operation) { pa_operation_ref(Operation); }
  PAOperation& operator=(const PAOperation& r);//  { Unlink(); Operation = r.Operation; return *this; }
 public:
  PAOperation()                                 : Operation(NULL), FinalState(PA_OPERATION_CANCELLED), WaitPending(0) {}
  //PAOperation(pa_operation* op)                 : Operation(op) {}
  ~PAOperation()                                { Unlink(); }
  void Attach(pa_operation* op)                 { Unlink(); Operation = op; }

  bool IsValid() const                          { return Operation != NULL; }
  pa_operation_state_t GetState() const         { return Operation ? pa_operation_get_state(Operation) : FinalState; }
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

class PAServerInfoOperation : public PAOperation
{protected:
  event<const pa_server_info> InfoEvent;
 public:
  event_pub<const pa_server_info>& Info() { return InfoEvent; }

  static void ServerInfoCB(pa_context *c, const pa_server_info *i, void *userdata);
};

class PASinkInfoOperation : public PAOperation
{public:
  struct Args
  { /// Info about a PA sink. \c NULL in case of no more items or an error.
    const pa_sink_info* Info;
    /// Non-zero: error code.
    int                 Error;
    Args(const pa_sink_info* info, int error) : Info(info), Error(error) {}
  };
 protected:
  event<const Args> InfoEvent;
 public:
  event_pub<const Args>& Info()         { return InfoEvent; }

  static void SinkInfoCB(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
};

class PASourceInfoOperation : public PAOperation
{public:
  struct Args
  { /// Info about a PA sink. \c NULL in case of no more items or an error.
    const pa_source_info* Info;
    /// Non-zero: error code.
    int                   Error;
    Args(const pa_source_info* info, int error) : Info(info), Error(error) {}
  };
 protected:
  event<const Args> InfoEvent;
 public:
  event_pub<const Args>& Info()         { return InfoEvent; }

  static void SourceInfoCB(pa_context *c, const pa_source_info *i, int eol, void *userdata);
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
    Lock()                                      { pa_threaded_mainloop_lock(PAContext::Mainloop); DEBUGLOG(("PAContext::Lock::Lock()\n")); }
    ~Lock()                                     { pa_threaded_mainloop_unlock(PAContext::Mainloop); DEBUGLOG(("PAContext::Lock::~Lock()\n")); }
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
  void Disconnect();
  pa_context* GetContext()                      { return Context; }
  pa_context_state_t GetState() const           { return Context ? pa_context_get_state(Context) : PA_CONTEXT_UNCONNECTED; }
  int Errno() const                             { return Context ? pa_context_errno(Context) : PA_ERR_CONNECTIONTERMINATED; }
  void WaitReady() throw(PAConnectException);
  /// @return Observable to track context state changes.
  event_pub<const pa_context_state_t>& StateChange() { return StateChangeEvent; }

  static void MainloopWait()                    { pa_threaded_mainloop_wait(Mainloop); }
  static void MainloopSignal(bool wait_accept = false) { pa_threaded_mainloop_signal(Mainloop, wait_accept); }

  void GetServerInfo(PAServerInfoOperation& op) throw (PAContextException);

  void GetSinkInfo(PASinkInfoOperation& op) throw (PAContextException);
  void GetSinkInfo(PASinkInfoOperation& op, uint32_t index) throw (PAContextException);
  void GetSinkInfo(PASinkInfoOperation& op, const char* name) throw (PAContextException);

  void SetSinkPort(PABasicOperation& op, const char* sink, const char* port) throw (PAContextException);

  void GetSourceInfo(PASourceInfoOperation& op) throw (PAContextException);
  void GetSourceInfo(PASourceInfoOperation& op, uint32_t index) throw (PAContextException);
  void GetSourceInfo(PASourceInfoOperation& op, const char* name) throw (PAContextException);

  void SetSourcePort(PABasicOperation& op, const char* source, const char* port) throw (PAContextException);
};


FLAGSATTRIBUTE(pa_stream_flags_t);

/** Sound stream
 */
class PAStream
{public:
  PABufferAttr       Attr;
 protected:
  pa_stream*         Stream;
  bool               Terminate;
 private:
  int                WaitReadyPending;

 private: // non-copyable
  PAStream(const PAStream& r);
  void operator=(const PAStream& r);
 protected:
  PAStream()                                    : Stream(NULL), Terminate(false), WaitReadyPending(0) {}
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

class PASinkOutput : public PAStream
{private:
  int                WaitWritePending;

 public:
  PASinkOutput()                                : WaitWritePending(0) {}

  void Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
               const char* device, pa_stream_flags_t flags, const pa_cvolume* volume = NULL, pa_stream* sync_stream = NULL)
       throw(PAContextException);
  void Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
               const char* device, pa_stream_flags_t flags, pa_volume_t volume, pa_stream* sync_stream = NULL)
       throw(PAContextException)                { PACVolume vol(ss->channels, volume); Connect(context, name, ss, map, props, device, flags, &vol, sync_stream); }

  size_t WritableSize() throw (PAStreamException);
  void* BeginWrite(size_t& size) throw (PAStreamException);
  uint64_t Write(const void* data, size_t nbytes, int64_t offset = 0, pa_seek_mode_t seek = PA_SEEK_RELATIVE) throw(PAStreamException);
  void RunDrain(PABasicOperation& op) throw (PAStreamException);

  void SetVolume(const pa_cvolume* volume) throw (PAStreamException);
  void SetVolume(pa_volume_t volume) throw (PAStreamException);
 private:
  static void WriteCB(pa_stream *p, size_t nbytes, void *userdata) throw();
};

class PASourceInput : public PAStream
{private:
  int                WaitReadPending;

 public:
  PASourceInput()                               : WaitReadPending(0) {}

  void Connect(PAContext& context, const char* name, const pa_sample_spec* ss, const pa_channel_map* map, pa_proplist* props,
               const char* device, pa_stream_flags_t flags) throw(PAContextException);

  size_t ReadableSize() throw (PAStreamException);
  const void* Peek(size_t& nbytes) throw (PAStreamException);
  void Drop() throw (PAStreamException);

 private:
  static void ReadCB(pa_stream *p, size_t nbytes, void *userdata) throw();
};

class PASample : public PAStream
{

};


#endif

