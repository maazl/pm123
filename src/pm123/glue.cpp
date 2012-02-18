/*
 * Copyright 2006-2012 Marcel Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 *
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

#include "glue.h"
#include "decoder.h"
#include "filter.h"
#include "output.h"
#include "eventhandler.h"

//#define DEBUG 1
#include <debuglog.h>


PM123_TIME Glue::MinPos;
PM123_TIME Glue::MaxPos;

event<const Glue::DecEventArgs> Glue::DecEvent;
event<const OUTEVENTTYPE> Glue::OutEvent;


/****************************************************************************
*
* Private implementation of class Glue
*
****************************************************************************/
class GlueImp : private Glue
{ friend class Glue;
 private:
  enum
  { FLG_PlaystopSent       // DECEVENT_PLAYSTOP has been sent. May be set to 0 to discard samples.
  };

  static const DecEventArgs CEVPlayStop;

  static bool              Initialized;       // whether the following vars are valid
  static AtomicUnsigned    Flags;
  static xstring           Url;               // current URL
  static PM123_TIME        StopTime;          // time index where to stop the current playback
  static PM123_TIME        PosOffset;         // Offset to posmarker parameter to make the output timeline monotonic
  static FORMAT_INFO2      LastFormat;        // Format of last request_buffer
  static int_ptr<Output>   OutPlug;           // currently initialized output plug-in.
  static PluginList        FilterPlugs;       // List of initialized visual plug-ins.
  static int_ptr<Decoder>  DecPlug;           // currently active decoder plug-in.
  static OutputProcs       Procs;             // entry points of the filter chain
  static OUTPUT_PARAMS2    OParams;           // parameters for output_command
  static DECODER_PARAMS2   DParams;           // parameters for decoder_command
  static delegate<void,const PluginEventArgs> PluginDeleg;

 private:
  /// Virtualize output procedures by invoking the filter plug-in no \a i.
  static void              Virtualize(int i);
  /// setup the entire filter chain
  static ULONG             Init();
  /// uninitialize the filter chain
  static void              Uninit();
  /// Select the currently active decoder
  /// @exception ModuleException Something went wrong.
  static void              DecSetActive(const char* name);
  /// Send command to the current decoder using the current content of dparams.
  static ULONG             DecCommand(DECMSGTYPE msg);
  /// Send command to the current output and filter chain using the current content of params.
  static ULONG             OutCommand(ULONG msg)
                                              { return (*Procs.output_command)(Procs.A, msg, &OParams); }
  /// Handle plug-in changes while playing.
  static void              PluginNotification(void*, const PluginEventArgs& args);

 private: // glue
  PROXYFUNCDEF int DLLENTRY GlueRequestBuffer(void* a, const FORMAT_INFO2* format, float** buf);
  PROXYFUNCDEF void DLLENTRY GlueCommitBuffer(void* a, int len, PM123_TIME posmarker);
  // 4 callback interface
  PROXYFUNCDEF void DLLENTRY DecEventHandler(void* a, DECEVENTTYPE event, void* param);
  PROXYFUNCDEF void DLLENTRY OutEventHandler(void* w, OUTEVENTTYPE event);
};

// statics
const GlueImp::DecEventArgs GlueImp::CEVPlayStop = { DECEVENT_PLAYSTOP, NULL };

bool              GlueImp::Initialized = false;
AtomicUnsigned    GlueImp::Flags(0);
xstring           GlueImp::Url;
PM123_TIME        GlueImp::StopTime;
PM123_TIME        GlueImp::PosOffset;
FORMAT_INFO2      GlueImp::LastFormat;
int_ptr<Output>   GlueImp::OutPlug;
PluginList        GlueImp::FilterPlugs(PLUGIN_FILTER);
int_ptr<Decoder>  GlueImp::DecPlug;
OutputProcs       GlueImp::Procs;
OUTPUT_PARAMS2    GlueImp::OParams = {0};
DECODER_PARAMS2   GlueImp::DParams = {(const char*)NULL};
delegate<void,const PluginEventArgs> GlueImp::PluginDeleg(&GlueImp::PluginNotification);


void GlueImp::Virtualize(int i)
{ DEBUGLOG(("GlueImp::Virtualize(%i) - %p\n", i, OParams.Info));
  if (i < 0)
    return;

  Filter& fil = (Filter&)*FilterPlugs[i];
  // Virtualize procedures
  FILTER_PARAMS2 par;
  par.size                   = sizeof par;
  par.output_command         = Procs.output_command;
  par.output_playing_samples = Procs.output_playing_samples;
  par.output_request_buffer  = Procs.output_request_buffer;
  par.output_commit_buffer   = Procs.output_commit_buffer;
  par.output_playing_pos     = Procs.output_playing_pos;
  par.output_playing_data    = Procs.output_playing_data;
  par.a                      = Procs.A;
  par.output_event           = OParams.OutEvent;
  par.w                      = OParams.W;
  if (!fil.Initialize(&par))
  { EventHandler::PostFormat(MSG_WARNING, "The filter plug-in %s failed to initialize.", fil.ModRef->Key.cdata());
    FilterPlugs.erase(i);
    Virtualize(i-1);
    return;
  }
  // store new entry points
  Procs.output_command         = par.output_command;
  Procs.output_playing_samples = par.output_playing_samples;
  Procs.output_request_buffer  = par.output_request_buffer;
  Procs.output_commit_buffer   = par.output_commit_buffer;
  Procs.output_playing_pos     = par.output_playing_pos;
  Procs.output_playing_data    = par.output_playing_data;
  Procs.A                      = fil.GetProcs().F;
  void DLLENTRYP(last_output_event)(void* w, OUTEVENTTYPE event) = OParams.OutEvent;
  // next filter
  Virtualize(i-1);
  // store new callback if virtualized by the plug-in.
  BOOL vcallback = par.output_event != last_output_event;
  last_output_event = par.output_event; // swap...
  par.output_event  = OParams.OutEvent;
  par.w             = OParams.W;
  if (vcallback)
  { // set params for next instance.
    OParams.OutEvent = last_output_event;
    OParams.W        = fil.GetProcs().F;
    DEBUGLOG(("Glue::Virtualize: callback virtualized: %p %p\n", OParams.OutEvent, OParams.W));
  }
  if (par.output_event != last_output_event)
  { // now update the decoder event
    (*fil.GetProcs().filter_update)(fil.GetProcs().F, &par);
    DEBUGLOG(("Glue::Virtualize: callback update: %p %p\n", par.output_event, par.w));
  }
}

// setup filter chain
ULONG GlueImp::Init()
{ DEBUGLOG(("GlueImp::Init\n"));

  Plugin::GetChangeEvent() += PluginDeleg;

  // setup callback handlers
  OParams.OutEvent  = &OutEventHandler;
  //params.W         = NULL; // not required

  // setup callback handlers
  DParams.DecEvent = &DecEventHandler;
  //dparams.save_filename   = NULL;

  // Fetch current active output
  { PluginList outputs(PLUGIN_OUTPUT);
    Plugin::GetPlugins(outputs);
    if (outputs.size() == 0)
    { EventHandler::Post(MSG_ERROR, "There is no active output plug-in.");
      return (ULONG)-1;  // ??
    }
    ASSERT(outputs.size() == 1);
    OutPlug = (Output*)outputs[0].get();
  }

  // initially only the output plugin
  ULONG rc = OutPlug->InitPlugin();
  if (rc != PLUGIN_OK)
    goto fail;
  Procs = OutPlug->GetProcs();
  // setup filters
  Plugin::GetPlugins(FilterPlugs);
  Virtualize(FilterPlugs.size()-1);
  // setup OutPlug
  rc = OutCommand(OUTPUT_SETUP);
  if (rc == PLUGIN_OK)
    Initialized = true;
  else
  {fail:
    Uninit(); // deinit filters
  }
  return rc;
}

void GlueImp::Uninit()
{ DEBUGLOG(("GlueImp::Uninit\n"));

  PluginDeleg.detach();

  Initialized = false;
  // uninitialize filter chain
  foreach (const int_ptr<Plugin>*, fpp, FilterPlugs)
  { Filter& fil = (Filter&)**fpp;
    if (fil.IsInitialized())
      fil.UninitPlugin();
  }
  // Uninitialize output
  OutPlug->UninitPlugin();
  OutPlug.reset();
}

void GlueImp::DecSetActive(const char* name)
{ // Uninit current DecPlug if any
  if (DecPlug && DecPlug->IsInitialized())
  { int_ptr<Decoder> dp(DecPlug);
    DecPlug.reset();
    dp->UninitPlugin();
  }
  if (name == NULL)
    return;
  // Find new DecPlug
  int_ptr<Decoder> pd(Decoder::GetDecoder(name));
  pd->InitPlugin();
  DecPlug = pd;
}

ULONG GlueImp::DecCommand(DECMSGTYPE msg)
{ DEBUGLOG(("DecCommand(%d)\n", msg));
  if (DecPlug == NULL)
    return 3; // no DecPlug active
  ULONG ret = DecPlug->DecoderCommand(msg, &DParams);
  DEBUGLOG(("DecCommand: %lu\n", ret));
  return ret;
}

/* invoke decoder to play an URL */
ULONG Glue::DecPlay(const APlayable& song, PM123_TIME offset, PM123_TIME start, PM123_TIME stop)
{
  const xstring& url = song.GetPlayable().URL;
  DEBUGLOG(("Glue::DecPlay(&%p{%s}, %f, %f,%g)\n", &song, url.cdata(), offset, start, stop));
  ASSERT((song.GetInfo().tech->attributes & TATTR_SONG));
  // set active DecPlug
  try
  { GlueImp::DecSetActive(xstring(song.GetInfo().tech->decoder));
  } catch (const ModuleException& ex)
  { EventHandler::Post(MSG_ERROR, ex.GetErrorText());
    return (ULONG)-2;
  }

  GlueImp::StopTime     = stop;
  GlueImp::Flags.bitrst(GlueImp::FLG_PlaystopSent);
  GlueImp::PosOffset    = offset;
  GlueImp::MinPos       = 1E99;
  GlueImp::MaxPos       = 0;

  GlueImp::DParams.URL              = url;
  GlueImp::DParams.JumpTo           = start;
  GlueImp::DParams.OutRequestBuffer = &PROXYFUNCREF(GlueImp)GlueRequestBuffer;
  GlueImp::DParams.OutCommitBuffer  = &PROXYFUNCREF(GlueImp)GlueCommitBuffer;
  GlueImp::DParams.A                = GlueImp::Procs.A;

  ULONG rc = GlueImp::DecCommand(DECODER_SETUP);
  if (rc != 0)
  { GlueImp::DecSetActive(NULL);
    return rc;
  }
  GlueImp::Url = url;

  GlueImp::DecCommand(DECODER_SAVEDATA);

  rc = GlueImp::DecCommand(DECODER_PLAY);
  if (rc != 0)
    GlueImp::DecSetActive(NULL);
  else
  { // TODO: I hate this delay with a spinlock.
    int cnt = 0;
    while (GlueImp::DecPlug->DecoderStatus() == DECODER_STARTING)
    { DEBUGLOG(("dec_play - waiting for Spinlock\n"));
      DosSleep(++cnt > 10);
  } }
  return rc;
}

/* stop the current DecPlug immediately */
ULONG Glue::DecStop()
{ GlueImp::Url = NULL;
  ULONG rc = GlueImp::DecCommand(DECODER_STOP);
  // Do not deactivate the DecPlug immediately.
  return rc;
}

void Glue::DecClose()
{ GlueImp::DecSetActive(NULL);
}

/* set fast forward/rewind mode */
ULONG Glue::DecFast(DECFASTMODE mode)
{ GlueImp::DParams.Fast = mode;
  return GlueImp::DecCommand(DECODER_FFWD);
}

/* jump to absolute position */
ULONG Glue::DecJump(PM123_TIME location)
{ // Discard stop time if we seek beyond this point.
  if (location >= GlueImp::StopTime)
    GlueImp::StopTime = 1E99;
  GlueImp::DParams.JumpTo = location;
  ULONG rc = GlueImp::DecCommand(DECODER_JUMPTO);
  if (rc == 0 && GlueImp::Initialized)
  { GlueImp::OParams.PlayingPos = location;
    GlueImp::OutCommand(OUTPUT_TRASH_BUFFERS);
  }
  return rc;
}

/* set savefilename to save the raw stream data */
ULONG Glue::DecSave(const char* file)
{ if (file != NULL && *file == 0)
    file = NULL;
  GlueImp::DParams.SaveFilename = file;
  ULONG status = GlueImp::DecPlug ? GlueImp::DecPlug->DecoderStatus() : 0;
  return status == DECODER_PLAYING || status == DECODER_STARTING || status == DECODER_PAUSED
   ? GlueImp::DecCommand(DECODER_SAVEDATA)
   : 0;
}


/* setup new output stage or change the properties of the current one */
ULONG Glue::OutSetup(const APlayable& song)
{ const INFO_BUNDLE_CV& info = song.GetInfo();
  DEBUGLOG(("Glue::OutSetup(&%p{%s,{%i,%i,%x...}})\n",
    &song, song.GetPlayable().URL.cdata(), info.tech->samplerate, info.tech->channels, info.tech->attributes));
  GlueImp::OParams.URL        = song.GetPlayable().URL;
  GlueImp::OParams.Info       = &info;
  // TODO: is this position correct?
  GlueImp::OParams.PlayingPos = GlueImp::PosOffset;
  
  if (!GlueImp::Initialized)
  { ULONG rc = GlueImp::Init(); // here we initiate the setup of the filter chain
    if (rc != 0)
      return rc;
  }
  DEBUGLOG(("Glue::OutSetup before OutCommand %p\n", GlueImp::OParams.Info));
  return GlueImp::OutCommand(OUTPUT_OPEN);
}

/* closes OutPlug device and uninitializes all filter and output plug-ins */
ULONG Glue::OutClose()
{ if (!GlueImp::Initialized)
    return (ULONG)-1;
  GlueImp::OParams.PlayingPos = (*GlueImp::Procs.output_playing_pos)(GlueImp::Procs.A);
  GlueImp::Initialized = false;
  GlueImp::OutCommand(OUTPUT_TRASH_BUFFERS);
  ULONG rc = GlueImp::OutCommand(OUTPUT_CLOSE);
  GlueImp::Uninit(); // Hmm, is it a good advise to do this in case of an error?
  return rc;
}

/* adjust OutPlug volume */
void Glue::OutSetVolume(double volume)
{ if (!GlueImp::Initialized)
    return; // can't help
  GlueImp::OParams.Volume = volume;
  GlueImp::OutCommand(OUTPUT_VOLUME);
}

ULONG Glue::OutPause(bool pause)
{ if (!GlueImp::Initialized)
    return (ULONG)-1; // error
  GlueImp::OParams.Pause = pause;
  return GlueImp::OutCommand(OUTPUT_PAUSE);
}

bool Glue::OutFlush()
{ if (!GlueImp::Initialized)
    return FALSE;
  (*GlueImp::Procs.output_request_buffer)(GlueImp::Procs.A, NULL, NULL);
  return TRUE;
}

bool Glue::OutTrash()
{ if (!GlueImp::Initialized)
    return FALSE;
  GlueImp::OutCommand(OUTPUT_TRASH_BUFFERS);
  return TRUE;
}

/* Returns 0 = success otherwize MMOS/2 error. */
PROXYFUNCIMP(ULONG DLLENTRY, Glue)
OutPlayingSamples(PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param)
{ if (!GlueImp::Initialized)
    return (ULONG)-1; // N/A
  return (*GlueImp::Procs.output_playing_samples)(GlueImp::Procs.A, offset, cb, param);
}

/* Returns time in ms. */
PROXYFUNCIMP(PM123_TIME DLLENTRY, Glue) OutPlayingPos()
{ if (!GlueImp::Initialized)
    return 0; // ??
  return (*GlueImp::Procs.output_playing_pos)(GlueImp::Procs.A);
}

/* if the OutPlug is playing. */
PROXYFUNCIMP(BOOL DLLENTRY, Glue) OutPlayingData()
{ if (!GlueImp::Initialized)
    return FALSE;
  return (*GlueImp::Procs.output_playing_data)(GlueImp::Procs.A);
}

PROXYFUNCIMP(int DLLENTRY, GlueImp)
GlueRequestBuffer(void* a, const FORMAT_INFO2* format, float** buf)
{ if (!GlueImp::Initialized)
    return 0;
  // do not pass flush, signal DECEVENT_PLAYSTOP instead.
  if (buf == NULL)
  { if (!GlueImp::Flags.bitset(GlueImp::FLG_PlaystopSent))
      DecEvent(GlueImp::CEVPlayStop);
    return 0;
  }
  // We are beyond the end?
  if (GlueImp::Flags.bit(GlueImp::FLG_PlaystopSent))
    return 0;
  // pass request to output
  GlueImp::LastFormat = *format;
  return (*GlueImp::Procs.output_request_buffer)(a, format, buf);
}

PROXYFUNCIMP(void DLLENTRY, GlueImp)
GlueCommitBuffer(void* a, int len, PM123_TIME posmarker)
{ if (!GlueImp::Initialized)
    return;
  bool send_playstop = false;
  PM123_TIME pos_e = posmarker + (PM123_TIME)len / GlueImp::LastFormat.samplerate;

  // check stop offset
  if (pos_e >= GlueImp::StopTime)
  { pos_e = GlueImp::StopTime;
    // calcutate fractional part
    len = posmarker >= GlueImp::StopTime
      ? 0 // begin is already beyond the limit => cancel request
      : (int)((GlueImp::StopTime - posmarker) * GlueImp::LastFormat.samplerate +.5);
    // Signal DECEVENT_PLAYSTOP if not yet sent.
    send_playstop = !GlueImp::Flags.bitset(GlueImp::FLG_PlaystopSent);
  }

  // update min/max
  if (GlueImp::MinPos > posmarker)
    GlueImp::MinPos = posmarker;
  if (GlueImp::MaxPos < pos_e)
    GlueImp::MaxPos = pos_e;

  // pass to the output
  (*GlueImp::Procs.output_commit_buffer)(a, len, posmarker + GlueImp::PosOffset);

  // Signal DECEVENT_PLAYSTOP ?
  if (send_playstop)
    DecEvent(GlueImp::CEVPlayStop);
}

PROXYFUNCIMP(void DLLENTRY, GlueImp)
DecEventHandler(void* a, DECEVENTTYPE event, void* param)
{ DEBUGLOG(("GlueImp::DecEventHandler(%p, %d, %p)\n", a, event, param));
  // We handle some event here immediately.
  switch (event)
  {case DECEVENT_CHANGEOBJ:
    /*if (Glue::url)
    { int_ptr<Playable> pp = Playable::FindByURL(Glue::url);
      if (pp)
        pp->SetTechInfo((TECH_INFO*)param);
    }*/
    break;

   case DECEVENT_CHANGEMETA:
    // TODO: Well, the backend does not support time dependant metadata.
    // From the playlist view, metadata changes should be immediately visible.
    // But during playback the change should be delayed until the apropriate buffer is really played.
    // The latter cannot be implemented with the current backend.
    if (GlueImp::Url)
    { int_ptr<Playable> pp = Playable::FindByURL(GlueImp::Url);
      if (pp)
        pp->SetMetaInfo((META_INFO*)param);
    }
    break;

   case DECEVENT_PLAYSTOP:
    GlueImp::StopTime = 0;
    // discard if already sent
    if (GlueImp::Flags.bitset(GlueImp::FLG_PlaystopSent))
      return;
   default: // avoid warnings
    break;
  }
  const DecEventArgs args = { event, param };
  DecEvent(args);
}

/* proxy, because the decoder is not interested in OUTEVENT_END_OF_DATA */
PROXYFUNCIMP(void DLLENTRY, GlueImp)
OutEventHandler(void* w, OUTEVENTTYPE event)
{ DEBUGLOG(("GlueImp::OutEventHandler(%p, %d)\n", w, event));
  OutEvent(event);
  // route high/low water events to the DecPlug (if any)
  switch (event)
  {case OUTEVENT_LOW_WATER:
   case OUTEVENT_HIGH_WATER:
    { if (DecPlug != NULL)
        GlueImp::DecPlug->DecoderEvent(event);
      break;
    }
   default: // avoid warnings
    break;
  }
}


static time_t nomsgtill = 0;

void GlueImp::PluginNotification(void*, const PluginEventArgs& args)
{ DEBUGLOG(("GlueImp::PluginNotification(, {&%p{%s}, %i})\n", &args.Plug, args.Plug.ModRef->Key.cdata(), args.Operation));
  switch (args.Operation)
  {case PluginEventArgs::Load:
   case PluginEventArgs::Unload:
    if (!args.Plug.GetEnabled())
      break;
   case PluginEventArgs::Enable:
   case PluginEventArgs::Disable:
    switch (args.Plug.PluginType)
    {case PLUGIN_DECODER:
     case PLUGIN_FILTER:
     case PLUGIN_OUTPUT:
      { time_t t = time(NULL);
        if (t > nomsgtill)
          EventHandler::Post(MSG_ERROR, "Changes to decoder, filter or output plug-in will not take effect until playback stops.");
        nomsgtill = t + 15; // disable the message for a few seconds.
      }
     default:; // avoid warnings
    }
   default:; // avoid warnings
  }
}
