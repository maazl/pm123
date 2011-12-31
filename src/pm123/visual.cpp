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

#include "visual.h"
#include "glue.h"
#include "eventhandler.h"
#include "controller.h"
#include <decoder_plug.h>

#include <debuglog.h>



/****************************************************************************
*
* visualization interface
*
****************************************************************************/

static ULONG DLLENTRY dec_status()
{ if (!Ctrl::IsPlaying())
    return DECODER_STOPPED;
  return Ctrl::IsPaused() ? DECODER_PAUSED : DECODER_PLAYING;
}

static PM123_TIME DLLENTRY dec_length()
{ int_ptr<APlayable> song = Ctrl::GetCurrentSong();
  return song ? song->GetInfo().obj->songlength : -1;
}

const PLUGIN_PROCS Visual::VisualCallbacks =
{ &out_playing_samples,
  &out_playing_data,
  &out_playing_pos,
  &dec_status,
  &dec_length
};

/* Assigns the addresses of the visual plug-in procedures. */
void Visual::LoadPlugin()
{ const Module& mod = *ModRef;
  DEBUGLOG(("Visual(%p{%s})::LoadPlugin()\n", this, mod.Key.cdata()));
  Hwnd = NULLHANDLE;
  mod.LoadMandatoryFunction(&plugin_deinit, "plugin_deinit");
  mod.LoadMandatoryFunction(&plugin_init,   "vis_init");
}

bool Visual::InitPlugin(HWND owner)
{ DEBUGLOG(("Visual(%p{%s})::initialize(%x) - %d %d, %d %d, %s\n",
    this, ModRef->Key.cdata(), owner, Props.x, Props.y, Props.cx, Props.cy, Props.param.cdata()));

  VISPLUGININIT visinit;
  visinit.x       = Props.x;
  visinit.y       = Props.y;
  visinit.cx      = Props.cx;
  visinit.cy      = Props.cy;
  visinit.hwnd    = owner;
  visinit.procs   = &VisualCallbacks;
  visinit.param   = Props.param;

  Hwnd = (*plugin_init)(&visinit);
  if (Hwnd == NULLHANDLE)
    return false;
  RaisePluginChange(PluginEventArgs::Init);
  return true;
}

bool Visual::UninitPlugin()
{ if (IsInitialized())
  { (*plugin_deinit)(0);
    Hwnd = NULLHANDLE;
    RaisePluginChange(PluginEventArgs::Uninit);
  }
  return true;
}

void Visual::BroadcastMsg(ULONG msg, MPARAM mp1, MPARAM mp2)
{ PluginList visuals(PLUGIN_VISUAL);
  Plugin::GetPlugins(visuals);
  for (size_t i = 0; i < visuals.size(); i++)
  { Visual& vis = (Visual&)*visuals[i];
    if (vis.GetEnabled() && vis.IsInitialized())
      WinSendMsg(vis.Hwnd, msg, mp1, mp2);
  }
}

static const VISUAL_PROPERTIES def_visuals = {0,0,0,0, FALSE, ""};

void Visual::SetProperties(const VISUAL_PROPERTIES* data)
{ if (data)
  { DEBUGLOG(("Visual(%p{%s})::set_properties(%p{%d %d, %d %d, %s})\n",
      this, ModRef->Key.cdata(), data, data->x, data->y, data->cx, data->cy, data->param.cdata()));
    Props = *data;
  } else
  { DEBUGLOG(("Visual(%p{%s})::set_properties(%p)\n", this, ModRef->Key.cdata(), data));
    Props = def_visuals;
  }
}

int_ptr<Visual> Visual::FindInstance(const Module& module)
{ Mutex::Lock lock(Module::Mtx);
  Visual* vis = module.Vis;
  return vis && !vis->RefCountIsUnmanaged() ? vis : NULL;
}

int_ptr<Visual> Visual::GetInstance(Module& module)
{ ASSERT(getTID() == 1);
  if ((module.GetParams().type & PLUGIN_VISUAL) == 0)
    throw ModuleException("Cannot load plug-in %s as visual plug-in.", module.Key.cdata());
  Mutex::Lock lock(Module::Mtx);
  Visual* vis = module.Vis;
  if (vis && !vis->RefCountIsUnmanaged())
    return vis;
  if (module.GetParams().interface < 3)
  { throw ModuleException(
      "Could not load visual plug-in %s because it is designed for PM123 before version 1.41.\n"
      "Please get a newer version of this plug-in.",
      module.ModuleName.cdata());
    return NULL;
  }
  vis = new Visual(module);
  try
  { vis->LoadPlugin();
  } catch (...)
  { delete vis;
    throw;
  }
  return module.Vis = vis;
}
