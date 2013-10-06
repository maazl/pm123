/*
 * Copyright 2009-2013 Marcel Mueller
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

#define  INCL_PM
#define  INCL_DOS

#include "drc123.h"
#include "frontend.h"
#include "Deconvolution.h"
#include "Measure.h"
#include "Calibrate.h"

#include <utilfct.h>
#include <fftw3.h>
#include <format.h>
#include <filter_plug.h>
#include <plugin.h>
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include <debuglog.h>


PLUGIN_CONTEXT Ctx;


inline static void do_load_prf_value(const char* name, bool& target)
{ (*Ctx.plugin_api->profile_query)(name, &target, 1);
}
static void do_load_prf_value(const char* name, xstring& target)
{ int len = (*Ctx.plugin_api->profile_query)(name, NULL, 0);
  if (len >= 0)
    (*Ctx.plugin_api->profile_query)(name, target.allocate(len), len);
}
static void do_load_prf_value(const char* name, volatile xstring& target)
{ int len = (*Ctx.plugin_api->profile_query)(name, NULL, 0);
  if (len >= 0)
  { xstring value;
    (*Ctx.plugin_api->profile_query)(name, value.allocate(len), len);
    target.swap(value);
  }
}
template <class T>
static void do_load_prf_value(const char* name, volatile T& target)
{ T value;
  if ((*Ctx.plugin_api->profile_query)(name, &value, sizeof(T)) == sizeof(T))
    target = value;
}

inline static void do_save_prf_value(const char* name, bool value)
{ (*Ctx.plugin_api->profile_write)(name, &value, 1);
}
inline static void do_save_prf_value(const char* name, xstring value)
{ (*Ctx.plugin_api->profile_write)(name, value.cdata(), value.length());
}
template <class T>
inline static void do_save_prf_value(const char* name, const T value)
{ (*Ctx.plugin_api->profile_write)(name, &value, sizeof(T));
}

#define load_prf_value(var) \
  do_load_prf_value(#var, var)

#define save_prf_value(var) \
  do_save_prf_value(#var, var)


static void load_config()
{
  do_load_prf_value("filter.WorkDir",  Filter::WorkDir);
  do_load_prf_value("openloop.RecURI", OpenLoop::RecURI);
  Deconvolution::Parameters deconvolution;
  load_prf_value(deconvolution.FilterFile);
  load_prf_value(deconvolution.WindowFunction);
  load_prf_value(deconvolution.Enabled);
  load_prf_value(deconvolution.FIROrder);
  load_prf_value(deconvolution.PlanSize);
  Deconvolution::SetParameters(deconvolution);
}

static void save_config()
{
  do_save_prf_value("filter.WorkDir",  xstring(Filter::WorkDir));
  do_save_prf_value("openloop.RecURI", xstring(OpenLoop::RecURI));
  Deconvolution::Parameters deconvolution;
  Deconvolution::GetParameters(deconvolution);
  save_prf_value(deconvolution.FilterFile);
  save_prf_value(deconvolution.WindowFunction);
  save_prf_value(deconvolution.Enabled);
  save_prf_value(deconvolution.FIROrder);
  save_prf_value(deconvolution.PlanSize);
}


/********** Filter processing stuff */

ULONG DLLENTRY filter_init(Filter** F, FILTER_PARAMS2* params)
{ return (*F = Filter::Factory(*params)) == NULL;
}

void DLLENTRY filter_update(Filter* F, const FILTER_PARAMS2 *params)
{ F->Update(*params);
}

BOOL DLLENTRY filter_uninit(Filter* F)
{ delete F;
  return TRUE;
}

/********** GUI stuff *******************************************************/

HWND DLLENTRY plugin_configure(HWND hwnd, HMODULE module)
{
  Frontend(hwnd, module).Process();
  save_config();
  return NULLHANDLE;
}

int DLLENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
  param->type         = PLUGIN_FILTER;
  param->author       = "Marcel Mueller";
  param->desc         = "Digital Room Correction Version 1.0";
  param->configurable = TRUE;
  param->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/* init plug-in */
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{
  Ctx = *ctx;
  load_config();
  return 0;
}

#if defined(__IBMC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return 1UL;
  } else {
    #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
    #endif
    _CRT_term();
    return 1UL;
  }
}
#endif
