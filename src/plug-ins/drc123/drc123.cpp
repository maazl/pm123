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

  { Deconvolution::Parameters deconvolution;
    Deconvolution::GetDefaultParameters(deconvolution);
    load_prf_value(deconvolution.FilterFile);
    load_prf_value(deconvolution.WindowFunction);
    load_prf_value(deconvolution.Enabled);
    load_prf_value(deconvolution.FIROrder);
    load_prf_value(deconvolution.PlanSize);
    Deconvolution::SetParameters(deconvolution);
  }
  { SyncAccess<Measure::MeasureFile> pdata(Measure::GetData());
    Measure::MeasureFile& measure = *pdata;
    load_prf_value(measure.FFTSize);
    load_prf_value(measure.DiscardSamp);
    load_prf_value(measure.RefFMin);
    load_prf_value(measure.RefFMax);
    load_prf_value(measure.RefExponent);
    load_prf_value(measure.RefSkipEven);
    load_prf_value(measure.RefVolume);
    load_prf_value(measure.RefFDist);
    load_prf_value(measure.AnaFBin);
    load_prf_value(measure.AnaSwap);
    load_prf_value(measure.GainLow);
    load_prf_value(measure.GainHigh);
    load_prf_value(measure.DelayLow);
    load_prf_value(measure.DelayHigh);
    load_prf_value(measure.Mode);
    load_prf_value(measure.Chan);
    load_prf_value(measure.DiffOut);
    load_prf_value(measure.RefIn);
    load_prf_value(measure.CalFile);
  }
  { SyncAccess<Calibrate::CalibrationFile> pdata(Calibrate::GetData());
    Calibrate::CalibrationFile& calibrate = *pdata;
    load_prf_value(calibrate.FFTSize);
    load_prf_value(calibrate.DiscardSamp);
    load_prf_value(calibrate.RefFMin);
    load_prf_value(calibrate.RefFMax);
    load_prf_value(calibrate.RefExponent);
    load_prf_value(calibrate.RefSkipEven);
    load_prf_value(calibrate.RefVolume);
    load_prf_value(calibrate.RefFDist);
    load_prf_value(calibrate.AnaFBin);
    load_prf_value(calibrate.AnaSwap);
    load_prf_value(calibrate.GainLow);
    load_prf_value(calibrate.GainHigh);
    load_prf_value(calibrate.DelayLow);
    load_prf_value(calibrate.DelayHigh);
    load_prf_value(calibrate.Mode);
    load_prf_value(calibrate.Gain2Low);
    load_prf_value(calibrate.Gain2High);
  }
}

static void save_config()
{
  do_save_prf_value("filter.WorkDir",  xstring(Filter::WorkDir));
  do_save_prf_value("openloop.RecURI", xstring(OpenLoop::RecURI));

  { Deconvolution::Parameters deconvolution;
    Deconvolution::GetParameters(deconvolution);
    save_prf_value(deconvolution.FilterFile);
    save_prf_value(deconvolution.WindowFunction);
    save_prf_value(deconvolution.Enabled);
    save_prf_value(deconvolution.FIROrder);
    save_prf_value(deconvolution.PlanSize);
  }
  { SyncAccess<Measure::MeasureFile> pdata(Measure::GetData());
    Measure::MeasureFile& measure = *pdata;
    save_prf_value(measure.FFTSize);
    save_prf_value(measure.DiscardSamp);
    save_prf_value(measure.RefFMin);
    save_prf_value(measure.RefFMax);
    save_prf_value(measure.RefExponent);
    save_prf_value(measure.RefSkipEven);
    save_prf_value(measure.RefVolume);
    save_prf_value(measure.RefFDist);
    save_prf_value(measure.AnaFBin);
    save_prf_value(measure.AnaSwap);
    save_prf_value(measure.GainLow);
    save_prf_value(measure.GainHigh);
    save_prf_value(measure.DelayLow);
    save_prf_value(measure.DelayHigh);
    save_prf_value(measure.Mode);
    save_prf_value(measure.Chan);
    save_prf_value(measure.DiffOut);
    save_prf_value(measure.RefIn);
    save_prf_value(measure.CalFile);
  }
  { SyncAccess<Calibrate::CalibrationFile> pdata(Calibrate::GetData());
    Calibrate::CalibrationFile& calibrate = *pdata;
    save_prf_value(calibrate.FFTSize);
    save_prf_value(calibrate.DiscardSamp);
    save_prf_value(calibrate.RefFMin);
    save_prf_value(calibrate.RefFMax);
    save_prf_value(calibrate.RefExponent);
    save_prf_value(calibrate.RefSkipEven);
    save_prf_value(calibrate.RefVolume);
    save_prf_value(calibrate.RefFDist);
    save_prf_value(calibrate.AnaFBin);
    save_prf_value(calibrate.AnaSwap);
    save_prf_value(calibrate.GainLow);
    save_prf_value(calibrate.GainHigh);
    save_prf_value(calibrate.DelayLow);
    save_prf_value(calibrate.DelayHigh);
    save_prf_value(calibrate.Mode);
    save_prf_value(calibrate.Gain2Low);
    save_prf_value(calibrate.Gain2High);
  }
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
