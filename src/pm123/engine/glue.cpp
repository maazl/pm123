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
#include "../configuration.h"
#include "../eventhandler.h"
#include <math.h>
#include <time.h>

//#define DEBUG 1
#include <debuglog.h>


int_ptr<Decoder>  Glue::DecPlug;
PM123_TIME Glue::MinPos;
PM123_TIME Glue::MaxPos;
event<const Glue::DecEventArgs> Glue::DecEvent;

bool Glue::Initialized = false;
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
  { FLG_PlaystopSent       ///< DECEVENT_PLAYSTOP has been sent. May be set to 0 to discard samples.
  };

  static const DecEventArgs CEVPlayStop;

  static AtomicUnsigned    Flags;
  static volatile int_ptr<Location> DecLoc;   ///< currently decoding location (song & call stack)
  static PM123_TIME        StopTime;          ///< time index where to stop the current playback
  static PM123_TIME        PosOffset;         ///< Offset to posmarker parameter to make the output timeline monotonic
  static DECODER_PARAMS2   DParams;           ///< parameters for decoder_command

  static int_ptr<APlayable> OutSong;          ///< Currently playing song
  static FORMAT_INFO2      LastFormat;        ///< Format of last request_buffer
  static float*            LastBuffer;        ///< Last buffer requested by decoder
  static float             Gain;              ///< Replay gain adjust
  static time_t            LowWaterLimit;     ///< != 0 => we are at high priority, OUTEVENT_LOW_WATER expires at ...
  static TID               DecTID;            ///< Thread ID of the decoder thread
  static int_ptr<Output>   OutPlug;           ///< currently initialized output plug-in.
  static PluginList        FilterPlugs;       ///< List of initialized visual plug-ins.
  static OutputProcs       Procs;             ///< entry points of the filter chain
  static OUTPUT_PARAMS2    OParams;           ///< parameters for output_command
  static delegate<void,const PluginEventArgs> PluginDeleg;
  static vector_own<delegate<void,const PlayableChangeArgs> > CallstackDelegs;

 private:
  /// Virtualize output procedures by invoking the filter plug-in no \a i.
  static void              Virtualize(int i);
  /// setup the entire filter chain
  static ULONG             Init();
  /// uninitialize the filter chain
  static void              Uninit();
  /// Send command to the current decoder using the current content of dparams.
  static ULONG             DecCommand(DECMSGTYPE msg);
  /// Send command to the current output and filter chain using the current content of params.
  static ULONG             OutCommand(ULONG msg)
                                              { return (*Procs.output_command)(Procs.A, msg, &OParams); }
  /// Handle plug-in changes while playing.
  static void              PluginNotification(void*, const PluginEventArgs& args);
  /// Handle song updates while playing (ReplayGain).
  static void              ItemNotification(void*, const PlayableChangeArgs& args);

  static void              SetDecoderPriority(bool high);

 private: // glue
  PROXYFUNCDEF int DLLENTRY GlueRequestBuffer(void* a, const FORMAT_INFO2* format, float** buf);
  PROXYFUNCDEF void DLLENTRY GlueCommitBuffer(void* a, int len, PM123_TIME posmarker);
  // 4 callback interface
  PROXYFUNCDEF void DLLENTRY DecEventHandler(void* a, DECEVENTTYPE event, void* param);
  PROXYFUNCDEF void DLLENTRY OutEventHandler(void* w, OUTEVENTTYPE event);
};

// statics
const GlueImp::DecEventArgs GlueImp::CEVPlayStop = { DECEVENT_PLAYSTOP, NULL };

AtomicUnsigned    GlueImp::Flags(0);
volatile int_ptr<Location> GlueImp::DecLoc;
PM123_TIME        GlueImp::StopTime;
PM123_TIME        GlueImp::PosOffset;
DECODER_PARAMS2   GlueImp::DParams = {(const char*)NULL};

int_ptr<APlayable>GlueImp::OutSong;
FORMAT_INFO2      GlueImp::LastFormat;
float*            GlueImp::LastBuffer;
float             GlueImp::Gain;
time_t            GlueImp::LowWaterLimit;
TID               GlueImp::DecTID;
int_ptr<Output>   GlueImp::OutPlug;
PluginList        GlueImp::FilterPlugs(PLUGIN_FILTER);
OutputProcs       GlueImp::Procs;
OUTPUT_PARAMS2    GlueImp::OParams = {0};

delegate<void,const PluginEventArgs> GlueImp::PluginDeleg(&GlueImp::PluginNotification);
vector_own<delegate<void,const PlayableChangeArgs> > GlueImp::CallstackDelegs;

void GlueImp::Virtualize(int i)
{ DEBUGLOG(("GlueImp::Virtualize(%i) - %p\n", i, OParams.Info));
  if (i < 0)
    return;

  Filter& fil = (Filter&)*FilterPlugs[i];
  // Virtualize procedures
  FILTER_PARAMS2 par;
  par.output_command         = Procs.output_command;
  par.output_playing_samples = Procs.output_playing_samples;
  par.output_request_buffer  = Procs.output_request_buffer;
  par.output_commit_buffer   = Procs.output_commit_buffer;
  par.output_playing_pos     = Procs.output_playing_pos;
  par.output_playing_data    = Procs.output_playing_data;
  par.a                      = Procs.A;
  par.output_event           = (void DLLENTRYPF()(struct FILTER_STRUCT*, OUTEVENTTYPE))OParams.OutEvent;
  par.w                      = (struct FILTER_STRUCT*)OParams.W;
  if (!fil.Initialize(par))
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
  Procs.A                      = fil.GetFilterPtr();
  void DLLENTRYP(last_output_event)(struct FILTER_STRUCT* w, OUTEVENTTYPE event) = (void DLLENTRYPF()(struct FILTER_STRUCT*, OUTEVENTTYPE))OParams.OutEvent;
  // next filter
  Virtualize(i-1);
  // store new callback if virtualized by the plug-in.
  BOOL vcallback = par.output_event != last_output_event;
  last_output_event = par.output_event; // swap...
  par.output_event  = (void DLLENTRYPF()(struct FILTER_STRUCT*, OUTEVENTTYPE))OParams.OutEvent;
  par.w             = (struct FILTER_STRUCT*)OParams.W;
  if (vcallback)
  { // set params for next instance.
    OParams.OutEvent = (void DLLENTRYPF()(void*, OUTEVENTTYPE))last_output_event;
    OParams.W        = fil.GetFilterPtr();
    DEBUGLOG(("Glue::Virtualize: callback virtualized: %p %p\n", OParams.OutEvent, OParams.W));
  }
  if (par.output_event != last_output_event)
  { // now update the decoder event
    fil.UpdateEvent(par);
    DEBUGLOG(("Glue::Virtualize: callback update: %p %p\n", par.output_event, par.w));
  }
}

// setup filter chain
ULONG GlueImp::Init()
{ DEBUGLOG(("GlueImp::Init\n"));

  Plugin::GetChangeEvent() += PluginDeleg;

  // setup callback handlers
  OParams.OutEvent = &OutEventHandler;
  //params.W         = NULL; // not required

  // setup callback handlers
  DParams.DecEvent = &DecEventHandler;
  //dparams.save_filename   = NULL;

  LowWaterLimit = 0;

  // Fetch current active output
  { int_ptr<PluginList> outputs(Plugin::GetPluginList(PLUGIN_OUTPUT));
    foreach (const int_ptr<Plugin>,*, ppp, *outputs)
      if ((*ppp)->GetEnabled())
      { OutPlug = (Output*)ppp->get();
        goto done;
      }
    EventHandler::Post(MSG_ERROR, "There is no active output plug-in.");
    return (ULONG)-1;  // ??

   done:;
  }

  // initially only the output plugin
  ULONG rc = OutPlug->InitPlugin();
  if (rc != PLUGIN_OK)
    goto fail;
  Procs = OutPlug->GetProcs();
  // setup filters
  FilterPlugs = *Plugin::GetPluginList(PLUGIN_FILTER);
  // removed disabled plug-ins
  for (size_t i = FilterPlugs.size(); i-- != 0; )
    if (!FilterPlugs[i]->GetEnabled())
      FilterPlugs.erase(i);
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

  CallstackDelegs.clear();
  PluginDeleg.detach();

  Initialized = false;
  // uninitialize filter chain
  foreach (const int_ptr<Plugin>,*, fpp, FilterPlugs)
  { Filter& fil = (Filter&)**fpp;
    if (fil.IsInitialized())
      fil.UninitPlugin();
  }
  // Uninitialize output
  OutPlug->UninitPlugin();
  OutPlug.reset();
  Procs.A = NULL;
}

ULONG GlueImp::DecCommand(DECMSGTYPE msg)
{ DEBUGLOG(("GlueImp::DecCommand(%d)\n", msg));
  if (DecPlug == NULL)
    return 3; // no DecPlug active
  ULONG ret = DecPlug->DecoderCommand(msg, &DParams);
  DEBUGLOG(("GlueImp::DecCommand: %lu\n", ret));
  return ret;
}

/* invoke decoder to play an URL */
ULONG Glue::DecPlay(const Location& loc, PM123_TIME offset, PM123_TIME start, PM123_TIME stop)
{ DEBUGLOG(("Glue::DecPlay(&%p, %g, %g,%g)\n", &loc, offset, start, stop));
  APlayable& song = *loc.GetCurrent();
  DEBUGLOG(("Glue::DecPlay %s\n", song.DebugName().cdata()));
  ASSERT((song.GetInfo().tech->attributes & TATTR_SONG));
  // Uninit current DecPlug if any
  DecClose();
  try
  { // Find new DecPlug
    int_ptr<Decoder> pd(Decoder::GetDecoder(xstring(song.GetInfo().tech->decoder)));
    pd->InitPlugin();
    GlueImp::DecPlug = pd;
  } catch (const ModuleException& ex)
  { EventHandler::Post(MSG_ERROR, ex.GetErrorText());
    return (ULONG)-2;
  }

  GlueImp::StopTime     = stop ? stop : 1E99;
  GlueImp::Flags.bitrst(GlueImp::FLG_PlaystopSent);
  GlueImp::PosOffset    = offset;
  GlueImp::MinPos       = 1E99;
  GlueImp::MaxPos       = 0;

  GlueImp::DParams.URL              = song.GetPlayable().URL;
  GlueImp::DParams.Info             = &song.GetInfo();
  GlueImp::DParams.JumpTo           = start;
  GlueImp::DParams.SkipSpeed        = 0;
  GlueImp::DParams.OutRequestBuffer = &PROXYFUNCREF(GlueImp)GlueRequestBuffer;
  GlueImp::DParams.OutCommitBuffer  = &PROXYFUNCREF(GlueImp)GlueCommitBuffer;
  GlueImp::DParams.A                = GlueImp::Procs.A;

  ULONG rc = GlueImp::DecCommand(DECODER_SETUP);
  if (rc != 0)
  { DecClose();
    return rc;
  }
  GlueImp::DecLoc = new Location(loc);
  GlueImp::Gain = -1000;
  for (PlayableInstance*const* cpp = loc.GetCallstack().begin(); cpp != loc.GetCallstack().end(); ++cpp)
  { delegate<void,const PlayableChangeArgs>* deleg = new delegate<void,const PlayableChangeArgs>(&GlueImp::ItemNotification);
    GlueImp::CallstackDelegs.append() = deleg;
    (*cpp)->GetInfoChange() += *deleg;
  }

  GlueImp::DecCommand(DECODER_SAVEDATA);

  GlueImp::DecTID = 0;
  rc = GlueImp::DecCommand(DECODER_PLAY);
  if (rc != 0)
    DecClose();
  else
  { // I hate this delay with a spinlock...
    SpinWait wait(30000);
    while (GlueImp::DecPlug->DecoderStatus() == DECODER_STARTING)
    { DEBUGLOG(("Glue::DecPlay - waiting for Spinlock\n"));
      if (!wait.Wait())
      { EventHandler::Post(MSG_ERROR, "The decoder did not start within 30s.");
        DecStop();
        rc = PLUGIN_FAILED;
        break;
      }
  } }
  return rc;
}

/* stop the current DecPlug immediately */
ULONG Glue::DecStop()
{ DEBUGLOG(("Glue::DecStop() - %p\n", GlueImp::DecPlug.get()));
  GlueImp::CallstackDelegs.clear();
  GlueImp::DecLoc.reset();
  if (!GlueImp::DecPlug)
    return PLUGIN_GO_FAILED;
  ULONG rc = GlueImp::DecCommand(DECODER_STOP);
  return rc;
}

void Glue::DecClose()
{ DEBUGLOG(("Glue::DecClose() - %p\n", GlueImp::DecPlug.get()));
  int_ptr<Decoder> dp;
  dp.swap(GlueImp::DecPlug);
  if (dp && dp->IsInitialized())
  { // I hate this delay with a spinlock...
    SpinWait wait(30000); // 30 s
   retry:
    switch (dp->DecoderStatus())
    {default:
      DEBUGLOG(("Glue::DecStop - waiting for Spinlock\n"));
      if (wait.Wait())
        goto retry;
      EventHandler::Post(MSG_ERROR, "The decoder did not terminate within 30s. Killing thread.");
      DosKillThread(GlueImp::DecTID);
     case DECODER_STOPPED:
     case DECODER_ERROR:;
    }
    // Destroy decoder instance
    GlueImp::DecTID = 0;
    dp->UninitPlugin();
  }
}

/* set fast forward/rewind mode */
ULONG Glue::DecFast(float skipspeed)
{ GlueImp::DParams.SkipSpeed = skipspeed;
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
    GlueImp::OParams.PlayingPos = location;
  return rc;
}

/* set savefilename to save the raw stream data */
ULONG Glue::DecSave(const char* file)
{ DEBUGLOG(("Glue::DecSave(%s)\n", file));
  if (file != NULL && *file == 0)
    file = NULL;
  GlueImp::DParams.SaveFilename = file;
  ULONG status = GlueImp::DecPlug ? GlueImp::DecPlug->DecoderStatus() : 0;
  return status == DECODER_PLAYING || status == DECODER_STARTING || status == DECODER_PAUSED
   ? GlueImp::DecCommand(DECODER_SAVEDATA)
   : 0;
}


/* setup new output stage or change the properties of the current one */
ULONG Glue::OutSetup(APlayable& song, PM123_TIME offset)
{ const INFO_BUNDLE_CV& info = song.GetInfo();
  DEBUGLOG(("Glue::OutSetup(&%p{%s,{%i,%i,%x...}})\n",
    &song, song.DebugName().cdata(), info.tech->samplerate, info.tech->channels, info.tech->attributes));
  GlueImp::OutSong = &song;
  GlueImp::OParams.URL        = song.GetPlayable().URL;
  GlueImp::OParams.Info       = &info;
  GlueImp::OParams.PlayingPos = offset;

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
  ULONG rc = GlueImp::OutCommand(OUTPUT_CLOSE);
  // Now wait for the decoder to stop until we discard the output filter chain.

  DecClose();
  GlueImp::Uninit(); // Hmm, is it a good advise to do this in case of an error?
  GlueImp::OutSong.reset();
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

/* Returns 0 = success otherwise MMOS/2 error. */
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
{ DEBUGLOG(("GlueImp::GlueRequestBuffer(%p, {%i,%i}, %p) - %u\n",
    a, format->channels, format->samplerate, buf, GlueImp::Initialized));
  if (!GlueImp::Initialized)
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
  // Check for OUTEVENT_LOW_WATER expire.
  if (GlueImp::LowWaterLimit && time(NULL) > GlueImp::LowWaterLimit)
  { LowWaterLimit = 0;
    Decoder* dp = GlueImp::DecPlug;
    if (dp != NULL)
      dp->DecoderEvent(OUTEVENT_HIGH_WATER);
  }
  // Decoder Priority
  if (!DecTID)
  { // examine decoder thread ID
    TIB* tib;
    DosGetInfoBlocks(&tib, NULL);
    DecTID = tib->tib_ptib2->tib2_ultid;
    SetDecoderPriority(LowWaterLimit != 0);
  }
  // pass request to output
  int ret = (*GlueImp::Procs.output_request_buffer)((struct FILTER_STRUCT*)a, format, buf);
  GlueImp::LastFormat = *format;
  GlueImp::LastBuffer = *buf;
  return ret;
}

PROXYFUNCIMP(void DLLENTRY, GlueImp)
GlueCommitBuffer(void* a, int len, PM123_TIME posmarker)
{ DEBUGLOG(("GlueImp::GlueCommitBuffer(%p, %i, %g) - %u\n", a, len, posmarker, GlueImp::Initialized));
  if (!GlueImp::Initialized)
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

  // Replay gain processing
  int_ptr<Location> loc = GlueImp::DecLoc;
  if (loc != NULL)
  { float gain = GlueImp::Gain;
    if (gain <= -1000)
    { // calculate gain
      GlueImp::Gain = 0; // Marker to track asynchronous changes.
      volatile const amp_cfg& cfg = Cfg::Get();
      if (cfg.replay_gain)
      { const APlayable* song = loc->GetCurrent();
        DEBUGLOG(("GlueImp::GlueCommitBuffer LOC %p{%s}\n", song, song->DebugName().cdata()));
        const volatile META_INFO& meta = *song->GetInfo().meta;
        for (size_t i = 0; i < 4; ++i)
        { switch (cfg.rg_list[i])
          {case CFG_RG_ALBUM:
            gain = meta.album_gain;
            break;
           case CFG_RG_ALBUM_NO_CLIP:
            gain = meta.album_peak;
            break;
           case CFG_RG_TRACK:
            gain = meta.track_gain;
            break;
           case CFG_RG_TRACK_NO_CLIP:
            gain = meta.track_peak;
            break;
           default:
            i = 4;
          }
          if (gain > -1000)
            break;
        }
        if (gain <= -1000)
          gain = cfg.rg_preamp_other;
        else
          gain += cfg.rg_preamp;
      } else
        gain = 0;
      // item specific gain adjust
      for (const PlayableInstance*const* cpp = loc->GetCallstack().begin(); cpp != loc->GetCallstack().end(); ++cpp)
      { float gain_adj = (*cpp)->GetInfo().item->gain;
        if (gain_adj > -1000)
          gain += gain_adj;
      }
      if (GlueImp::Gain == 0)
        GlueImp::Gain = gain;
      DEBUGLOG(("GlueImp::GlueCommitBuffer - recalc gain to %f -> %f\n", gain, GlueImp::Gain));
    }
    if (gain)
    { float* dp = GlueImp::LastBuffer;
      float* ep = dp + len * GlueImp::LastFormat.channels;
      const float factor = exp(M_LN10/20. * gain);
      while (dp != ep)
        *dp++ *= factor;
    }
  }
  // pass to the output
  (*GlueImp::Procs.output_commit_buffer)((struct FILTER_STRUCT*)a, len, posmarker + GlueImp::PosOffset);

  // Signal DECEVENT_PLAYSTOP ?
  if (send_playstop)
    DecEvent(GlueImp::CEVPlayStop);
}

PROXYFUNCIMP(void DLLENTRY, GlueImp)
DecEventHandler(void* a, DECEVENTTYPE event, void* param)
{ DEBUGLOG(("GlueImp::DecEventHandler(%p, %d, %p)\n", a, event, param));
  // Discard bad events.
  if (a != GlueImp::Procs.A)
    return;
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
    // TODO: Well, the backend does not support time dependent metadata.
    // From the playlist view, metadata changes should be immediately visible.
    // But during playback the change should be delayed until the appropriate buffer is really played.
    // The latter cannot be implemented with the current backend.
    { int_ptr<Location> loc = GlueImp::DecLoc;
      if (loc)
      { int_ptr<Playable> pp = Playable::FindByURL(loc->GetCurrent()->GetPlayable().URL);
        if (pp)
          pp->SetMetaInfo((META_INFO*)param);
      }
    }
    break;

   case DECEVENT_PLAYSTOP:
    GlueImp::StopTime = 0;
    // discard if already sent
    if (GlueImp::Flags.bitset(GlueImp::FLG_PlaystopSent))
      return;
    break;

   case DECEVENT_PLAYERROR:
    if (param)
      EventHandler::Post(MSG_ERROR, xstring((const char*)param));
   default: // avoid warnings
    break;
  }
  const DecEventArgs args = { event, param };
  DecEvent(args);
}

void GlueImp::SetDecoderPriority(bool high)
{ TID dectid = GlueImp::DecTID;
  DEBUGLOG(("GlueImp::SetDecoderPriority(%u) - %i\n", high, dectid));
  if (dectid)
  { const volatile amp_cfg& cfg = Cfg::Get();
    int priority = high ? cfg.pri_high : cfg.pri_normal;
    DosSetPriority(PRTYS_THREAD, priority >> 8, priority & 0xff, dectid);
  }
}

/* proxy, because the decoder is not interested in OUTEVENT_END_OF_DATA */
PROXYFUNCIMP(void DLLENTRY, GlueImp)
OutEventHandler(void* w, OUTEVENTTYPE event)
{ DEBUGLOG(("GlueImp::OutEventHandler(%p, %d)\n", w, event));
  OutEvent(event);
  // route high/low water events to the DecPlug (if any)
  switch (event)
  {default:
    return;
   case OUTEVENT_LOW_WATER:
    SetDecoderPriority(false);
    if (GlueImp::LowWaterLimit == 0)
      GlueImp::LowWaterLimit = time(NULL) + Cfg::Get().pri_limit; // revoke OUTEVENT_LOW_WATER after xx seconds
    break;
   case OUTEVENT_HIGH_WATER:
    SetDecoderPriority(true);
    GlueImp::LowWaterLimit = 0;
    break;
  }
  if (int_ptr<Location>(GlueImp::DecLoc)) // Only if the decoder is not stopped
  { Decoder* dp = GlueImp::DecPlug;
    if (dp != NULL)
      dp->DecoderEvent(event);
  }
}


static time_t nomsgtill = 0;

void GlueImp::PluginNotification(void*, const PluginEventArgs& args)
{ DEBUGLOG(("GlueImp::PluginNotification(, {&%p{%s}, %i})\n", args.Plug, args.Plug ? args.Plug->ModRef->Key.cdata() : "", args.Operation));
  switch (args.Operation)
  {case PluginEventArgs::Load:
   case PluginEventArgs::Unload:
    if (!args.Plug->GetEnabled())
      break;
   case PluginEventArgs::Enable:
   case PluginEventArgs::Disable:
    switch (args.Plug->PluginType)
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

void GlueImp::ItemNotification(void*, const PlayableChangeArgs& args)
{ DEBUGLOG(("GlueImp::ItemNotification(,{&%p{%s},,,%x})\n", &args.Instance, args.Instance.DebugName().cdata(), args.Changed));
  if ((args.Changed & (IF_Meta|IF_Item)))
    Gain = -1000; // recalculate gain
}
