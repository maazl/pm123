/*
 * Copyright 2006 Marcel Mueller
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

/* Filter plug-in to give the volume slider of PM123 a logarithmic characteristic. */

#define  INCL_BASE
#define  INCL_WIN

#define  PLUGIN_INTERFACE_LEVEL 3

#include <filter_plug.h>
#include <output_plug.h>
#include <plugin.h>
#include "logvolum.h"
#include <utilfct.h>

#include <os2.h>
#include <stdlib.h>

#include <debuglog.h>

#undef VERSION // don't know why
#define VERSION "Logarithmic Volume Control 1.01"

// internal scaling factor. This value should not exceed sqrt(10) (+10dB)
#define scalefactor 3.16227766


// internal vars
static ULONG DLLENTRYP(f_output_command)( void* a, ULONG msg, OUTPUT_PARAMS2* info );
#ifdef WITH_SOFT_VOLUME
static int   DLLENTRYP(f_request_buffer)( void* a, const FORMAT_INFO2* format, short** buf );
static void  DLLENTRYP(f_commit_buffer) ( void* a, int len, PM123_TIME posmarker );
#endif
static void* f_a;

// Configuration
#ifdef WITH_SOFT_VOLUME
typedef struct
{ BOOL  use_software;
} configuration;
static configuration cfg =
{ FALSE
};

// Current active config
static configuration cur_cfg;
// Current state
static short* last_buf;
static int    last_chan;
static double last_volume;
#endif


static ULONG DLLENTRY
filter_command( void* f, ULONG msg, OUTPUT_PARAMS2* info )
{ DEBUGLOG(("logvolume:filter_command(%p, %u, %p)\n", f, msg, info));
  switch (msg)
  {case OUTPUT_VOLUME:
    // This is no logarithm, but it is similar to it in a reasonable range
    // and it keep the zero value at zero.
    #ifdef WITH_SOFT_VOLUME
    last_volume = info->volume / (scalefactor + 1. - scalefactor*info->volume);
    DEBUGLOG(("logvolume:filter_command vol=%f\n", last_volume));
    info->Volume = cur_cfg.use_software ? 1. : last_volume;
    #else
    info->Volume /= (scalefactor + 1. - scalefactor*info->Volume);
    #endif
    break;
  }
  return (*f_output_command)( f_a, msg, info );
}

#ifdef WITH_SOFT_VOLUME
static int DLLENTRY
request_buffer( void* f, const FORMAT_INFO2* format, short** buf )
{ int r;
  DEBUGLOG(("logvolume:request_buffer(%p, %p, %p)\n", f, format, buf));
  r = (*f_request_buffer)(f_a, format, buf);
  if (buf)
  { last_chan = format->channels;
    last_buf = *buf;
  }
  return r;
}

static void DLLENTRY
commit_buffer( void* f, int len, PM123_TIME posmarker )
{ DEBUGLOG(("logvolume:commit_buffer(%p, %i, %f)\n", f, len, posmarker));
  if (cur_cfg.use_software && last_volume != 1 && len)
  { // Translate samples
    register int i = len * last_chan;
    register short* dp = last_buf;
    const int scale = (int)(0x10000 * last_volume); // use integer math
    do
    { *dp = (short)(*dp * scale >> 16);
      ++dp;
    }
    while (--i);
  }
  (*f_commit_buffer)(f_a, len, posmarker);
}
#endif


/********** Entry point: Initialize
*/
ULONG DLLENTRY
filter_init( void** F, FILTER_PARAMS2* params )
{ DEBUGLOG(("logvolume:filter_init(%p, {%u, ..., %p, ..., %p})\n", F, params->size, params->a, params->w));

  *F = NULL; // we have no instance data

  #ifdef WITH_SOFT_VOLUME
  cur_cfg = cfg;
  #endif

  f_output_command = params->output_command;
  #ifdef WITH_SOFT_VOLUME
  f_request_buffer = params->output_request_buffer;
  f_commit_buffer  = params->output_commit_buffer;
  #endif
  f_a              = params->a;

  params->output_command        = &filter_command;
  #ifdef WITH_SOFT_VOLUME
  params->output_request_buffer = &request_buffer;
  params->output_commit_buffer  = &commit_buffer;
  #endif

  return 0;
}

void DLLENTRY
filter_update( void *F, const FILTER_PARAMS2 *params )
{ DEBUGLOG(("logvolume:filter_update(%p, {%u, ..., %p, ..., %p})\n", F, params->size, params->a, params->w));
  DosEnterCritSec();
  f_output_command = params->output_command;
  f_a              = params->a;
  DosExitCritSec();
}

BOOL DLLENTRY
filter_uninit( void* F )
{ DEBUGLOG(("logvolume:filter_uninit(%p)\n", F));

  return TRUE;
}


/********** GUI stuff ******************************************************/
#ifdef WITH_SOFT_VOLUME
static void
save_ini( void )
{
  HINI hini;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value( hini, cfg.use_software );

    close_ini( hini );
  }
}

static void
load_ini( void )
{
  HINI hini;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( hini, cfg.use_software );

    close_ini( hini );
  }
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
      WinCheckButton( hwnd, CB_SOFTVOLUM, cfg.use_software );
      break;

    case WM_COMMAND:
      if( SHORT1FROMMP( mp1 ) == DID_OK )
      {
        cfg.use_software = WinQueryButtonCheckstate( hwnd, CB_SOFTVOLUM );

        save_ini();
      }
      break;
  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}
#endif

/* Configure plug-in. */
void DLLENTRY plugin_configure( HWND howner, HMODULE module )
{
  #ifdef WITH_SOFT_VOLUME
  HWND hwnd = WinLoadDlg( HWND_DESKTOP, howner, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  do_warpsans( hwnd );
  WinProcessDlg( hwnd );
  WinDestroyWindow( hwnd );
  #endif
}


int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM *param )
{
  param->type         = PLUGIN_FILTER;
  param->author       = "Marcel Mller";
  param->desc         = VERSION;
  #ifdef WITH_SOFT_VOLUME
  param->configurable = TRUE;
  #else
  param->configurable = FALSE;
  #endif
  param->interface    = PLUGIN_INTERFACE_LEVEL;

  #ifdef WITH_SOFT_VOLUME
  load_ini();
  #endif

  return 0;
}
