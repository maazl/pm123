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


#include "pulse123.h"
#include "playbackworker.h"
#include "recordworker.h"
#include "configuration.h"
#include "dialog.h"
#include <utilfct.h>
#include <plugin.h>
#include <os2.h>

#include <debuglog.h>


PLUGIN_CONTEXT Ctx;


static void fillinfo(const RecordWorker::Params& par, const INFO_BUNDLE* info)
{ // phys info is default.
  // tech info
  info->tech->samplerate = par.Format.samplerate;
  info->tech->channels = par.Format.channels;
  info->tech->attributes = TATTR_SONG;
  info->tech->info.sprintf("Recording from %s/%s/%s.",
    par.Server ? par.Server.cdata() : "(default)",
    par.Source ? par.Source.cdata() : "(default)",
    par.Port   ? par.Port.cdata()   : "(default)" );
  // obj info
  info->obj->bitrate = 32 * par.Format.channels * par.Format.samplerate;
  // meta info : none
  // attr info : default
}

/****************************************************************************
*
* Miscellaneous
*
****************************************************************************/

/* Configure plug-in. */
HWND DLLENTRY plugin_configure(HWND hwnd, HMODULE module)
{ ConfigDialog(hwnd, module).Process();
  return NULLHANDLE;
}

/// Returns information about plug-in.
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* query)
{ query->type         = PLUGIN_OUTPUT|PLUGIN_DECODER;
  query->author       = "Marcel Mueller";
  query->desc         = "PulseAudio for PM123 V1.0";
  query->configurable = TRUE;
  query->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/// init plug-in
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ Ctx = *ctx;
  Configuration.Load();
  return 0;
}

static PHYS_INFO def_phys = PHYS_INFO_INIT;
static META_INFO def_meta = META_INFO_INIT;
static ATTR_INFO def_attr = ATTR_INFO_INIT;

static ULONG DLLENTRY WizardDlg(HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param)
{ DEBUGLOG(("os2rec:WizardDlg(%p, %s, %p, %p)\n", owner, title, callback, param));
  HMODULE mod;
  getModule(&mod, NULL, 0);

  LoadWizard wiz(mod, owner, xstring().sprintf(title, " PulseAudio recording"));
  switch (wiz.Process())
  {default:
    return PLUGIN_ERROR;
   case DID_CANCEL:
    return PLUGIN_NO_OP;
   case DID_OK:;
  }

  RecordWorker::Params par;
  par.Server = Configuration.SourceServer;
  par.Source = Configuration.Source;
  par.Port   = Configuration.SourcePort;
  par.Format.samplerate = Configuration.SourceRate;
  par.Format.channels   = Configuration.SourceChannels;

  TECH_INFO tech = TECH_INFO_INIT;
  OBJ_INFO obj = OBJ_INFO_INIT;
  INFO_BUNDLE info =
  { &def_phys, &tech, &obj, &def_meta, &def_attr, NULL, NULL, NULL };
  fillinfo(par, &info);

  (*callback)(param, par.ToURL(), &info, 0, INFO_PHYS|INFO_TECH|INFO_OBJ|INFO_META|INFO_ATTR|INFO_CHILD);

  return PLUGIN_OK;
}

/* DLL entry point */
const DECODER_WIZARD* DLLENTRY decoder_getwizard( )
{
  static const DECODER_WIZARD wizardentry =
  { NULL,
    "Record PulseAudio...",
    &WizardDlg,
    0, 0,
    0, 0
  };

  return &wizardentry;
}


#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle, unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return 1UL;
  } else {
    _dump_allocated( 0 );
    _CRT_term();
    return 1UL;
  }
}
#endif


/****************************************************************************
*
*  Playback interface
*
****************************************************************************/

/// Initialize the output plug-in.
ULONG DLLENTRY output_init(PlaybackWorker** A)
{ PlaybackWorker* w = new PlaybackWorker();
  ULONG ret = w->Init(Configuration.SinkServer, Configuration.Sink, Configuration.SinkPort, Configuration.SinkMinLatency, Configuration.SinkMaxLatency);
  if (ret == 0)
    *A = w;
  return ret;
}

/// Uninitialize the output plug-in.
ULONG DLLENTRY output_uninit(PlaybackWorker* a)
{ delete a;
  return PLUGIN_OK;
}

/// Processing of a command messages.
ULONG DLLENTRY output_command(PlaybackWorker* a, OUTMSGTYPE msg, const OUTPUT_PARAMS2* info)
{ DEBUGLOG(("pulse123:output_command(%p, %d, %p)\n", a, msg, info));

  switch (msg)
  { case OUTPUT_OPEN:
      return a->Open(info->URL, info->Info, info->PlayingPos, info->OutEvent, info->W);

    case OUTPUT_CLOSE:
      return a->Close();

    case OUTPUT_VOLUME:
      return a->SetVolume(info->Volume);

    case OUTPUT_PAUSE:
      return a->SetPause(info->Pause);

    case OUTPUT_SETUP:
      return 0;

    case OUTPUT_TRASH_BUFFERS:
      return a->TrashBuffers(info->PlayingPos);
  }
  return PLUGIN_ERROR;
}

int DLLENTRY output_request_buffer(PlaybackWorker* a, const FORMAT_INFO2* format, float** buf)
{ return a->RequestBuffer(format, buf);
}

void DLLENTRY output_commit_buffer(PlaybackWorker* a, int len, PM123_TIME posmarker)
{ return a->CommitBuffer(len, posmarker);
}

ULONG DLLENTRY output_playing_samples(PlaybackWorker* a, PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param)
{ return a->GetCurrentSamples(offset, cb, param);
}

/// Returns the playing position.
PM123_TIME DLLENTRY output_playing_pos(PlaybackWorker* a)
{ return a->GetPosition();
}

/// Returns TRUE if the output plug-in still has some buffers to play.
BOOL DLLENTRY output_playing_data(PlaybackWorker* a)
{ return a->IsPlaying();
}


/****************************************************************************
*
* Recording interface
*
****************************************************************************/

// Supported file types (none)
ULONG DLLENTRY decoder_support(const DECODER_FILETYPE** types, int* count)
{ if (count)
    *count = 0;
  return DECODER_OTHER|DECODER_SONG;
}

ULONG DLLENTRY decoder_fileinfo(const char* url, struct _XFILE* handle, int* what, const INFO_BUNDLE* info,
                                DECODER_INFO_ENUMERATION_CB cb, void* param)
{ DEBUGLOG(("pulse123:decoder_fileinfo(%s, %p, &%x ...)\n", url, handle, what));
  RecordWorker::Params par;
  const xstring& error = par.ParseURL(url);
  if (error)
  { if (error.length() == 0)
      return PLUGIN_NO_PLAY;
    info->tech->info = error;
    return PLUGIN_NO_READ;
  }

  *what |= INFO_PHYS|INFO_TECH|INFO_OBJ|INFO_META|INFO_ATTR|INFO_CHILD;
  fillinfo(par, info);
  return PLUGIN_OK;
}

int DLLENTRY decoder_init(RecordWorker** w)
{ *w = new RecordWorker();
  return PLUGIN_OK;
}

BOOL DLLENTRY decoder_uninit(RecordWorker* w)
{ delete w;
  return TRUE;
}

ULONG DLLENTRY decoder_command(RecordWorker* w, DECMSGTYPE msg, const DECODER_PARAMS2* params)
{ DEBUGLOG(("pulse123:decoder_command(%p, %i, )\n", w, msg));
  switch (msg)
  {case DECODER_SETUP:
    return w->Setup(*params);
   case DECODER_PLAY:
    return w->Play(params->URL);
   case DECODER_STOP:
    return w->Stop();
   default:
    return PLUGIN_UNSUPPORTED;
  }
}

void DLLENTRY decoder_event(RecordWorker* w, OUTEVENTTYPE event)
{ w->Event(event);
}

ULONG DLLENTRY decoder_status(RecordWorker* w)
{ return w->GetState();
}

PM123_TIME DLLENTRY decoder_length(RecordWorker* w)
{ return -1;
}
