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

#include "filter.h"
#include "infobundle.h"
#include "123_util.h"

#include <debuglog.h>


/****************************************************************************
*
* filter interface
*
****************************************************************************/

/* Assigns the addresses of the filter plug-in procedures. */
void Filter::LoadPlugin()
{ DEBUGLOG(("Filter(%p{%s})::LoadPlugin\n", this, ModRef.Key.cdata()));
  F = NULL;
  const Module& mod = ModRef;
  mod.LoadMandatoryFunction(&filter_init,   "filter_init");
  mod.LoadMandatoryFunction(&filter_update, "filter_update");
  mod.LoadMandatoryFunction(&filter_uninit, "filter_uninit");
}

bool Filter::InitPlugin()
{ return true; // filters are not initialized unless they are used
}

bool Filter::UninitPlugin()
{ DEBUGLOG(("Filter(%p{%s})::UninitPlugin\n", this, ModRef.Key.cdata()));

  if (IsInitialized())
  { (*filter_uninit)(F);
    F = NULL;
    RaisePluginChange(PluginEventArgs::Uninit);
  }
  return true;
}

bool Filter::Initialize(FILTER_PARAMS2* params)
{ DEBUGLOG(("Filter(%p{%s})::Initialize(%p)\n", this, ModRef.Key.cdata(), params));

  FILTER_PARAMS2 par = *params;
  if (IsInitialized() || (*filter_init)(&F, params) != 0)
    return false;

  if (F == NULL)
  { // plug-in does not require local structures
    // => pass the pointer of the next stage and skip virtualization of untouched function
    F = par.a;
  } else
  { // virtualize untouched functions
    if (par.output_command          == params->output_command)
      params->output_command         = vreplace1(&VRStubs[0], par.output_command, par.a);
    if (par.output_playing_samples  == params->output_playing_samples)
      params->output_playing_samples = vreplace1(&VRStubs[1], par.output_playing_samples, par.a);
    if (par.output_request_buffer   == params->output_request_buffer)
      params->output_request_buffer  = vreplace1(&VRStubs[2], par.output_request_buffer, par.a);
    if (par.output_commit_buffer    == params->output_commit_buffer)
      params->output_commit_buffer   = vreplace1(&VRStubs[3], par.output_commit_buffer, par.a);
    if (par.output_playing_pos      == params->output_playing_pos)
      params->output_playing_pos     = vreplace1(&VRStubs[4], par.output_playing_pos, par.a);
    if (par.output_playing_data     == params->output_playing_data)
      params->output_playing_data    = vreplace1(&VRStubs[5], par.output_playing_data, par.a);
  }
  RaisePluginChange(PluginEventArgs::Init);
  return true;
}


// proxy for level 1 filters
class FilterProxy1 : public Filter, protected ProxyHelper
{private:
  int   DLLENTRYP(vfilter_init         )( void** f, FILTER_PARAMS* params );
  BOOL  DLLENTRYP(vfilter_uninit       )( void*  f );
  int   DLLENTRYP(vfilter_play_samples )( void*  f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
  void* vf;
  int   DLLENTRYP(output_request_buffer)( void*  a, const TECH_INFO* format, short** buf );
  void  DLLENTRYP(output_commit_buffer )( void*  a, int len, double posmarker );
  void* a;
  FORMAT_INFO vformat;                      // format of the samples (old style)
  TechInfo    vtech;                        // format of the samples (new style)
  short       vbuffer[BUFSIZE/2];           // buffer to store incoming samples
  int         vbufsamples;                  // size of vbuffer in samples
  int         vbuflevel;                    // current filled to vbuflevel
  double      vposmarker;                   // starting point of the current buffer
  BOOL        trash_buffer;                 // TRUE: signal to discard any buffer content
  VDELEGATE   vd_filter_init;
  VREPLACE1   vr_filter_update;
  VREPLACE1   vr_filter_uninit;

 private:
  PROXYFUNCDEF ULONG DLLENTRY proxy_1_filter_init          ( FilterProxy1* pp, void** f, FILTER_PARAMS2* params );
  PROXYFUNCDEF void  DLLENTRY proxy_1_filter_update        ( FilterProxy1* pp, const FILTER_PARAMS2* params );
  PROXYFUNCDEF BOOL  DLLENTRY proxy_1_filter_uninit        ( void* f ); // empty stub
  PROXYFUNCDEF int   DLLENTRY proxy_1_filter_request_buffer( FilterProxy1* f, const TECH_INFO* format, short** buf );
  PROXYFUNCDEF void  DLLENTRY proxy_1_filter_commit_buffer ( FilterProxy1* f, int len, double posmarker );
  PROXYFUNCDEF int   DLLENTRY proxy_1_filter_play_samples  ( FilterProxy1* f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
 public:
  FilterProxy1(Module& mod) : Filter(mod) {}
  virtual void LoadPlugin();
};

void FilterProxy1::LoadPlugin()
{ DEBUGLOG(("FilterProxy1(%p{%s})::LoadPlugin()\n", this, ModRef.Key.cdata()));
  F = NULL;
  const Module& mod = ModRef;
  mod.LoadMandatoryFunction(&vfilter_init,         "filter_init");
  mod.LoadMandatoryFunction(&vfilter_uninit,       "filter_uninit");
  mod.LoadMandatoryFunction(&vfilter_play_samples, "filter_play_samples");

  filter_init   = vdelegate(&vd_filter_init,   &proxy_1_filter_init,   this);
  filter_update = (void DLLENTRYPF()(void*, const FILTER_PARAMS2*)) // type of parameter is replaced too
                  vreplace1(&vr_filter_update, &proxy_1_filter_update, this);
  // filter_uninit is initialized at the filter_init call to a non-no-op function
  // However, the returned pointer will stay the same.
  filter_uninit = vreplace1(&vr_filter_uninit, &proxy_1_filter_uninit, (void*)NULL);
}

PROXYFUNCIMP(ULONG DLLENTRY, FilterProxy1)
proxy_1_filter_init(FilterProxy1* pp, void** f, FILTER_PARAMS2* params)
{ DEBUGLOG(("proxy_1_filter_init(%p{%s}, %p, %p{a=%p})\n", pp, pp->ModRef.Key.cdata(), f, params, params->a));

  FILTER_PARAMS par;
  par.size                = sizeof par;
  par.output_play_samples = (int DLLENTRYPF()(void*, const FORMAT_INFO*, const char*, int, int))
                            &PROXYFUNCREF(FilterProxy1)proxy_1_filter_play_samples;
  par.a                   = pp;
  par.audio_buffersize    = BUFSIZE;
  par.error_display       = &pm123_display_error;
  par.info_display        = &pm123_display_info;
  par.pm123_getstring     = &pm123_getstring;
  par.pm123_control       = &pm123_control;
  int r = (*pp->vfilter_init)(&pp->vf, &par);
  if (r != 0)
    return r;
  // save some values
  pp->output_request_buffer = params->output_request_buffer;
  pp->output_commit_buffer  = params->output_commit_buffer;
  pp->a                     = params->a;
  // setup internals
  pp->vbuflevel             = 0;
  pp->trash_buffer          = FALSE;
  pp->vformat.bits          = 16;
  pp->vformat.format        = WAVE_FORMAT_PCM;
  // replace the unload function
  vreplace1(&pp->vr_filter_uninit, pp->vfilter_uninit, pp->vf);
  // now return some values
  *f = pp;
  params->output_request_buffer = (int  DLLENTRYPF()(void*, const TECH_INFO*, short**))
                                  &PROXYFUNCREF(FilterProxy1)proxy_1_filter_request_buffer;
  params->output_commit_buffer  = (void DLLENTRYPF()(void*, int, double))
                                  &PROXYFUNCREF(FilterProxy1)proxy_1_filter_commit_buffer;
  return 0;
}

PROXYFUNCIMP(void DLLENTRY, FilterProxy1)
proxy_1_filter_update(FilterProxy1* pp, const FILTER_PARAMS2* params)
{ DEBUGLOG(("proxy_1_filter_update(%p{%s}, %p)\n", pp, pp->ModRef.Key.cdata(), params));

  CritSect cs;
  // replace function pointers
  pp->output_request_buffer = params->output_request_buffer;
  pp->output_commit_buffer  = params->output_commit_buffer;
  pp->a                     = params->a;
}

PROXYFUNCIMP(BOOL DLLENTRY, FilterProxy1)
proxy_1_filter_uninit(void*)
{ return TRUE;
}

PROXYFUNCIMP(int DLLENTRY, FilterProxy1)
proxy_1_filter_request_buffer(FilterProxy1* pp, const TECH_INFO* format, short** buf)
{ DEBUGLOG(("proxy_1_filter_request_buffer(%p, %p, %p)\n", pp, format, buf));

  if ( pp->trash_buffer )
  { pp->vbuflevel = 0;
    pp->trash_buffer = FALSE;
  }

  if ( buf == 0
    || ( pp->vbuflevel != 0 &&
         (pp->vformat.samplerate != format->samplerate || pp->vformat.channels != format->channels) ))
  { // local flush
    DEBUGLOG(("proxy_1_filter_request_buffer: local flush: %d\n", pp->vbuflevel));
    // Oh well, the old output plug-ins seem to play some more samples in doubt.
    // memset( pp->vbuffer + pp->vbuflevel * pp->vformat.channels, 0, (pp->vbufsamples - pp->vbuflevel) * pp->vformat.channels * sizeof(short) );
    (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, (char*)pp->vbuffer, pp->vbuflevel * pp->vformat.channels * sizeof(short), TstmpF2I(pp->vposmarker));
  }
  if ( buf == 0 )
  { return (*pp->output_request_buffer)( pp->a, format, NULL );
  }
  pp->vformat.samplerate = format->samplerate;
  pp->vformat.channels   = format->channels;
  pp->vtech.attributes   = format->attributes;
  // We use cmpcpy because it is faster, since the content of *format does most likely not change
  // between calls to proxy_1_filter_request_buffer.
  pp->vtech.info   .cmpassign(format->info);
  pp->vtech.format .cmpassign(format->format);
  pp->vtech.decoder.cmpassign(format->decoder);
  pp->vbufsamples   = sizeof pp->vbuffer / sizeof *pp->vbuffer / format->channels;

  DEBUGLOG(("proxy_1_filter_request_buffer: %d\n", pp->vbufsamples - pp->vbuflevel));
  *buf = pp->vbuffer + pp->vbuflevel * format->channels;
  return pp->vbufsamples - pp->vbuflevel;
}

PROXYFUNCIMP(void DLLENTRY, FilterProxy1)
proxy_1_filter_commit_buffer( FilterProxy1* pp, int len, double posmarker )
{ DEBUGLOG(("proxy_1_filter_commit_buffer(%p, %d, %g)\n", pp, len, posmarker));

  if (len == 0)
    return;

  if (pp->vbuflevel == 0)
    pp->vposmarker = posmarker;

  pp->vbuflevel += len;
  if (pp->vbuflevel == pp->vbufsamples)
  { // buffer full
    DEBUGLOG(("proxy_1_filter_commit_buffer: full: %d\n", pp->vbuflevel));
    (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, (char*)pp->vbuffer, BUFSIZE, TstmpF2I(pp->vposmarker));
    pp->vbuflevel = 0;
  }
}

PROXYFUNCIMP(int DLLENTRY, FilterProxy1)
proxy_1_filter_play_samples(FilterProxy1* pp, const FORMAT_INFO* format, const char *buf, int len, int posmarker_i)
{ DEBUGLOG(("proxy_1_filter_play_samples(%p, %p{%d,%d,%d,%d}, %p, %d, %d)\n",
    pp, format, format->samplerate, format->channels, format->bits, format->format, buf, len, posmarker_i));

  if (format->format != WAVE_FORMAT_PCM || format->bits != 16)
  { pm123_display_error("The proxy for old style filter plug-ins can only handle 16 bit raw PCM data.");
    return 0;
  }
  double posmarker = TstmpI2F(posmarker_i, pp->vposmarker);
  len /= pp->vformat.channels * sizeof(short);
  int rem = len;
  while (rem != 0)
  { // request new buffer
    short* dest;
    pp->vtech.samplerate = format->samplerate;
    pp->vtech.channels   = format->channels;
    int dlen = (*pp->output_request_buffer)( pp->a, &pp->vtech, &dest );
    DEBUGLOG(("proxy_1_filter_play_samples: now at %p %d, %p, %d\n", buf, rem, dest, dlen));
    if (dlen <= 0)
      return 0; // error
    if (dlen > rem)
      dlen = rem;
    // store data
    memcpy( dest, buf, dlen * pp->vformat.channels * sizeof(short) );
    // commit destination
    (*pp->output_commit_buffer)( pp->a, dlen, posmarker + (double)(len-rem)/format->samplerate );
    buf += dlen * pp->vformat.channels * sizeof(short);
    rem -= dlen;
  }
  return len * pp->vformat.channels * sizeof(short);
}


int_ptr<Filter> Filter::FindInstance(const Module& module)
{ Mutex::Lock lock(Module::Mtx);
  Filter* fil = module.Fil;
  return fil && !fil->RefCountIsUnmanaged() ? fil : NULL;
}

int_ptr<Filter> Filter::GetInstance(Module& module)
{ if ((module.GetParams().type & PLUGIN_FILTER) == 0)
    throw ModuleException("Cannot load plug-in %s as filter.", module.Key.cdata());
  Mutex::Lock lock(Module::Mtx);
  Filter* fil = module.Fil;
  if (fil && !fil->RefCountIsUnmanaged())
    return fil;
  fil = module.GetParams().interface <= 1 ? new FilterProxy1(module) : new Filter(module);
  try
  { fil->LoadPlugin();
  } catch (...)
  { delete fil;
    throw;
  }
  return module.Fil = fil;
}
