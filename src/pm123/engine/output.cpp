/*
 * Copyright 2006-2011 M.Mueller
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

/* This is the core interface to the plug-ins. It loads the plug-ins and
 * virtualizes them if required to refect always the most recent interface
 * level to the application.
 */

#define  INCL_PM
#define  INCL_DOS
#define  INCL_ERRORS

#include "output.h"
#include "proxyhelper.h"
#include "../configuration.h"
#include "../pm123.h" // for hab
#include <cpp/cppvdelegate.h>
#include <limits.h>

#include <debuglog.h>


/****************************************************************************
*
* output interface
*
****************************************************************************/

delegate<void, const PluginEventArgs> Output::PluginDeleg(&Output::PluginNotification);

Output::~Output()
{ ModRef->Out = NULL;
}

void Output::PluginNotification(void*, const PluginEventArgs& args)
{ if (args.Type == PLUGIN_OUTPUT)
  { switch(args.Operation)
    {case PluginEventArgs::Load:
      if (!args.Plug->GetEnabled())
        break;
     case PluginEventArgs::Enable:
      { // Output plug-ins are like a radio button. Only one plug-in is enabled at a time.
        int_ptr<PluginList> outputs(Outputs);
        foreach (const int_ptr<Plugin>,*, ppp, *outputs)
          if (*ppp != args.Plug)
            (*ppp)->SetEnabled(false);
      }
     default:;
    }
  }
}

/* Assigns the addresses of the output plug-in procedures. */
void Output::LoadPlugin()
{ const Module& mod = *ModRef;
  DEBUGLOG(("Output(%p{%s})::LoadPlugin()\n", this, mod.Key.cdata()));
  A = NULL;
  mod.LoadMandatoryFunction(&output_init,            "output_init");
  mod.LoadMandatoryFunction(&output_uninit,          "output_uninit");
  mod.LoadMandatoryFunction(&output_playing_samples, "output_playing_samples");
  mod.LoadMandatoryFunction(&output_playing_pos,     "output_playing_pos");
  mod.LoadMandatoryFunction(&output_playing_data,    "output_playing_data");
  mod.LoadMandatoryFunction(&output_command,         "output_command");
  mod.LoadMandatoryFunction(&output_request_buffer,  "output_request_buffer");
  mod.LoadMandatoryFunction(&output_commit_buffer,   "output_commit_buffer");
}

ULONG Output::InitPlugin()
{ DEBUGLOG(("Output(%p{%s})::InitPlugin()\n", this, ModRef->Key.cdata()));

  ULONG rc = (*output_init)(&A);
  if (rc != 0)
    A = NULL;
  else
    RaisePluginChange(PluginEventArgs::Init);
  return rc;
}

bool Output::UninitPlugin()
{ DEBUGLOG(("Output(%p{%s})::UninitPlugin()\n", this, ModRef->Key.cdata()));

  if (IsInitialized())
  { (*output_command)(A, OUTPUT_CLOSE, NULL);
    (*output_uninit)(A);
    A = NULL;
    RaisePluginChange(PluginEventArgs::Uninit);
  }
  return true;
}


// Proxy for loading level 1 plug-ins
class OutputProxy1 : public Output, protected ProxyHelper
{private:
  int          DLLENTRYP(voutput_command        )(void* a, ULONG msg, OUTPUT_PARAMS* info);
  int          DLLENTRYP(voutput_play_samples   )(void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker);
  ULONG        DLLENTRYP(voutput_playing_pos    )(void* a);
  ULONG        DLLENTRYP(voutput_playing_samples)(void* a, FORMAT_INFO* info, char* buf, int len);

  bool         voutput_trash_buffer;
  bool         voutput_flush_request;       ///< flush-request received, generate OUTEVENT_END_OF_DATA from WM_OUTPUT_OUTOFDATA
  bool         voutput_always_hungry;
  bool         voutput_opened;              ///< Flag whether the current output is already open.
  HWND         voutput_hwnd;                ///< Window handle for catching event messages
  PM123_TIME   voutput_posmarker;
  FORMAT_INFO  voutput_format;
  int          voutput_bufsamples;
  int          voutput_buffer_level;        ///< current level of voutput_buffer
  union
  { float      fbuf[BUFSIZE/2];
    short      sbuf[BUFSIZE/2];
  }            voutput_buffer;

  void         DLLENTRYP(voutput_event)(void* w, OUTEVENTTYPE event);
  void*        voutput_w;
  VDELEGATE    vd_output_command, vd_output_request_buffer, vd_output_commit_buffer, vd_output_playing_pos, vd_output_playing_samples;

 private:
  PROXYFUNCDEF ULONG      DLLENTRY proxy_1_output_command        (OutputProxy1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info);
  PROXYFUNCDEF int        DLLENTRY proxy_1_output_request_buffer (OutputProxy1* op, void* a, const FORMAT_INFO2* format, float** buf);
  PROXYFUNCDEF void       DLLENTRY proxy_1_output_commit_buffer  (OutputProxy1* op, void* a, int len, PM123_TIME posmarker);
  PROXYFUNCDEF PM123_TIME DLLENTRY proxy_1_output_playing_pos    (OutputProxy1* op, void* a);
  PROXYFUNCDEF ULONG      DLLENTRY proxy_1_output_playing_samples(OutputProxy1* op, void* a, PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param);
  friend MRESULT EXPENTRY proxy_1_output_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  /// Convert the range [0,voutput_buffer_level[ to 16 bit audio and send it to the output.
  void         SendSamples(void* a);
 public:
  OutputProxy1(Module& mod) : Output(mod), voutput_hwnd(NULLHANDLE), voutput_posmarker(0) {}
  virtual      ~OutputProxy1();
  virtual void LoadPlugin();
};

OutputProxy1::~OutputProxy1()
{ if (voutput_hwnd != NULLHANDLE)
    WinDestroyWindow(voutput_hwnd);
}

/* Assigns the addresses of the output plug-in procedures. */
void OutputProxy1::LoadPlugin()
{ const Module& mod = *ModRef;
  DEBUGLOG(("OutputProxy1(%p{%s})::LoadPlugin()\n", this, mod.Key.cdata()));
  A = NULL;
  mod.LoadMandatoryFunction(&output_init,            "output_init");
  mod.LoadMandatoryFunction(&output_uninit,          "output_uninit");
  mod.LoadMandatoryFunction(&voutput_playing_samples,"output_playing_samples");
  mod.LoadMandatoryFunction(&voutput_playing_pos,    "output_playing_pos");
  mod.LoadMandatoryFunction(&output_playing_data,    "output_playing_data");
  mod.LoadMandatoryFunction(&voutput_command,        "output_command");
  mod.LoadMandatoryFunction(&voutput_play_samples,   "output_play_samples");

  output_command         = vdelegate(&vd_output_command,         &proxy_1_output_command,         this);
  output_request_buffer  = vdelegate(&vd_output_request_buffer,  &proxy_1_output_request_buffer,  this);
  output_commit_buffer   = vdelegate(&vd_output_commit_buffer,   &proxy_1_output_commit_buffer,   this);
  output_playing_pos     = vdelegate(&vd_output_playing_pos,     &proxy_1_output_playing_pos,     this);
  output_playing_samples = vdelegate(&vd_output_playing_samples, &proxy_1_output_playing_samples, this);
}

inline void OutputProxy1::SendSamples(void* a)
{ // Convert samples to 16 bit audio
  Float2Short(voutput_buffer.sbuf, voutput_buffer.fbuf, voutput_buffer_level * voutput_format.channels);
  // Send samples
  (*voutput_play_samples)(a, &voutput_format, (char*)voutput_buffer.sbuf, voutput_buffer_level * voutput_format.channels * sizeof(short), TstmpF2I(voutput_posmarker));
  voutput_buffer_level = 0;
}

/* virtualization of level 1 output plug-ins */
PROXYFUNCIMP(ULONG DLLENTRY, OutputProxy1)
proxy_1_output_command(OutputProxy1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info)
{ DEBUGLOG(("proxy_1_output_command(%p {%s}, %p, %d, %p)\n", op, op->ModRef->Key.cdata(), a, msg, info));

  if (info == NULL) // sometimes info is NULL
    return (*op->voutput_command)(a, msg, NULL);

  OUTPUT_PARAMS params = { sizeof params };
  DECODER_INFO  dinfo  = { sizeof dinfo };

  // preprocessing
  switch (msg)
  {case OUTPUT_TRASH_BUFFERS:
    op->voutput_trash_buffer   = true;
    break;

   case OUTPUT_SETUP:
    op->voutput_hwnd = OutputProxy1::CreateProxyWindow("OutputProxy1", op);
   case OUTPUT_OPEN:
    { // convert DECODER_INFO2 to DECODER_INFO
      ProxyHelper::ConvertINFO_BUNDLE(&dinfo, info->Info);
      params.formatinfo        = dinfo.format;
      params.info              = &dinfo;
      params.hwnd              = op->voutput_hwnd;
      break;
    }
  }
  const volatile amp_cfg& cfg = Cfg::Get();
  params.buffersize            = BUFSIZE;
  int priority = cfg.pri_high; // remove volatile to avoid inconsistent samples.
  params.boostclass            = priority >> 8;
  params.boostdelta            = priority & 0xff;
  priority = cfg.pri_normal;
  params.normalclass           = priority >> 8;
  params.normaldelta           = priority & 0xff;
  params.nobuffermode          = FALSE;
  params.error_display         = &PROXYFUNCREF(ProxyHelper)PluginDisplayError;
  params.info_display          = &PROXYFUNCREF(ProxyHelper)PluginDisplayInfo;
  params.volume                = (char)(info->Volume*100+.5);
  params.pause                 = info->Pause;
  params.temp_playingpos       = TstmpF2I(info->PlayingPos);

  if (info->URL != NULL && strnicmp(info->URL, "file:", 5) == 0)
  { char* fname = (char*)alloca(strlen(info->URL)+1);
    strcpy(fname, info->URL);
    params.filename            = ConvertUrl2File(fname);
  } else
    params.filename            = info->URL;

  // call plug-in
  int r = (*op->voutput_command)(a, msg, &params);

  // postprocessing
  switch (msg)
  {case OUTPUT_SETUP:
    op->voutput_buffer_level   = 0;
    op->voutput_trash_buffer   = false;
    op->voutput_flush_request  = false;
    op->voutput_always_hungry  = params.always_hungry;
    op->voutput_opened         = false;
    op->voutput_event          = info->OutEvent;
    op->voutput_w              = info->W;
    op->voutput_format.bits    = 16;
    op->voutput_format.format  = WAVE_FORMAT_PCM;
    break;

   case OUTPUT_OPEN:
    if (r && op->voutput_opened)
      r = 0;
    op->voutput_opened = true;
    break;

   case OUTPUT_CLOSE:
    OutputProxy1::DestroyProxyWindow(op->voutput_hwnd);
    op->voutput_hwnd = NULLHANDLE;
    op->voutput_opened = false;
    break;
  }
  DEBUGLOG(("proxy_1_output_command: %d\n", r));
  return r;
}

PROXYFUNCIMP(int DLLENTRY, OutputProxy1)
proxy_1_output_request_buffer( OutputProxy1* op, void* a, const FORMAT_INFO2* format, float** buf )
{
  #ifdef DEBUG_LOG
  if (format != NULL)
    DEBUGLOG(("proxy_1_output_request_buffer(%p, %p, {%i,%i}, %p) - %d\n",
      op, a, format->samplerate, format->channels, buf, op->voutput_buffer_level));
   else
    DEBUGLOG(("proxy_1_output_request_buffer(%p, %p, %p, %p) - %d\n", op, a, format, buf, op->voutput_buffer_level));
  #endif

  if (op->voutput_trash_buffer)
  { op->voutput_buffer_level = 0;
    op->voutput_trash_buffer = false;
  }

  if ( op->voutput_buffer_level != 0
    && ( buf == 0
      || (op->voutput_format.samplerate != format->samplerate || op->voutput_format.channels != format->channels) ))
    op->SendSamples(a);
  if (buf == 0)
  { if (op->voutput_always_hungry)
      (*op->voutput_event)(op->voutput_w, OUTEVENT_END_OF_DATA);
     else
      op->voutput_flush_request = true; // wait for WM_OUTPUT_OUTOFDATA
    return 0;
  }

  *buf = op->voutput_buffer.fbuf + op->voutput_buffer_level * format->channels;
  op->voutput_format.samplerate = format->samplerate;
  op->voutput_format.channels   = format->channels;
  op->voutput_bufsamples        = sizeof op->voutput_buffer.fbuf / sizeof *op->voutput_buffer.fbuf / format->channels;
  DEBUGLOG(("proxy_1_output_request_buffer: %d\n", op->voutput_bufsamples - op->voutput_buffer_level));
  return op->voutput_bufsamples - op->voutput_buffer_level;
}

PROXYFUNCIMP(void DLLENTRY, OutputProxy1)
proxy_1_output_commit_buffer( OutputProxy1* op, void* a, int len, PM123_TIME posmarker )
{ DEBUGLOG(("proxy_1_output_commit_buffer(%p {%s}, %p, %i, %g) - %d\n",
    op, op->ModRef->Key.cdata(), a, len, posmarker, op->voutput_buffer_level));

  if (op->voutput_buffer_level == 0)
    op->voutput_posmarker = posmarker;

  op->voutput_buffer_level += len;
  if (op->voutput_buffer_level == op->voutput_bufsamples)
    op->SendSamples(a);
}

PROXYFUNCIMP(PM123_TIME DLLENTRY, OutputProxy1)
proxy_1_output_playing_pos( OutputProxy1* op, void* a )
{ DEBUGLOG(("proxy_1_output_playing_pos(%p {%s}, %p)\n", op, op->ModRef->Key.cdata(), a));
  return ProxyHelper::TstmpI2F((*op->voutput_playing_pos)(a), op->voutput_posmarker);
}

MRESULT EXPENTRY proxy_1_output_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ OutputProxy1* op = (OutputProxy1*)WinQueryWindowPtr(hwnd, QWL_USER);
  DEBUGLOG2(("proxy_1_output_winfn(%p, %u, %p, %p) - %p {%s}\n", hwnd, msg, mp1, mp2, op, op == NULL ? NULL : op->ModuleName.cdata()));
  switch (msg)
  {case WM_PLAYERROR:
    (*op->voutput_event)(op->voutput_w, OUTEVENT_PLAY_ERROR);
    return 0;
   case WM_OUTPUT_OUTOFDATA:
    if (op->voutput_flush_request) // don't care unless we have a flush_request condition
    { op->voutput_flush_request = false;
      (*op->voutput_event)(op->A, OUTEVENT_END_OF_DATA);
    }
    return 0;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

PROXYFUNCIMP(ULONG DLLENTRY, OutputProxy1)
proxy_1_output_playing_samples(OutputProxy1* op, void* a, PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param)
{ DEBUGLOG2(("proxy_1_output_playing_samples(%p{%s}, %p, %f, %p, %p) - %p \n",
    op, op == NULL ? NULL : op->ModuleName.cdata(), a, offset, cb, param));
  FORMAT_INFO fmt = { sizeof(FORMAT_INFO) };
  union
  { float f[4096];
    short s[8192]; // Only the second half of the buffer is used.
  } buf;
  ULONG rc = (*op->voutput_playing_samples)(a, &fmt, (char*)buf.s + sizeof buf / 2, sizeof buf / 2);
  if (rc != 0)
    return rc;

  // Only support 16 bps
  if (fmt.bits != 16)
    return 1;
  // Convert samples
  ProxyHelper::Short2Float(buf.f, buf.s + countof(buf.s) / 2, countof(buf.f));

  FORMAT_INFO2 fmt2;
  fmt2.channels   = fmt.channels;
  fmt2.samplerate = fmt.samplerate;
  BOOL done = TRUE;
  (*cb)(param, &fmt2, buf.f, countof(buf.f) / fmt2.channels, proxy_1_output_playing_pos(op, a), &done);
  return 0;
}


int_ptr<Output> Output::FindInstance(const Module& module)
{ Mutex::Lock lock(Module::Mtx);
  Output* out = module.Out;
  return out && !out->RefCountIsUnmanaged() ? out : NULL;
}

int_ptr<Output> Output::GetInstance(Module& module)
{ ASSERT(getTID() == 1);
  if ((module.GetParams().type & PLUGIN_OUTPUT) == 0)
    throw ModuleException("Cannot load plug-in %s as output plug-in.", module.Key.cdata());
  Mutex::Lock lock(Module::Mtx);
  Output* out = module.Out;
  if (out && !out->RefCountIsUnmanaged())
    return out;
  if (module.GetParams().interface == 2)
    throw ModuleException("The output plug-in %s is not supported. It is intended for PM123 1.40.", module.Key.cdata());
  out = module.GetParams().interface <= 1 ? new OutputProxy1(module) : new Output(module);
  try
  { out->LoadPlugin();
  } catch (...)
  { delete out;
    throw;
  }
  return module.Out = out;
}


void Output::Init()
{ PMRASSERT(WinRegisterClass(amp_player_hab, "OutputProxy1", &proxy_1_output_winfn, 0, sizeof(OutputProxy1*)));
  GetChangeEvent() += PluginDeleg;
}
