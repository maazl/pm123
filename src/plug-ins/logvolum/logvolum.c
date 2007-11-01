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

/* Filter plug-in to give the volume slitder of PM123 a logarithmic characteristic. */ 

#define  INCL_BASE
#include <os2.h>
#include <stdlib.h>

#define FILTER_PLUGIN_LEVEL 2

#include <filter_plug.h>
#include <output_plug.h>
#include <plugin.h>
#include "logvolum.h"

#include <debuglog.h>

#undef VERSION // don't know why
#define VERSION "Logarithmic Volume Control 1.0"

// internal scaling factor. This value should not exceed sqrt(10) (+10dB)
#define scalefactor 3.16227766


// internal vars
static ULONG DLLENTRYP(f_output_command)( void* a, ULONG msg, OUTPUT_PARAMS2* info );
static void* f_a;
  

static ULONG DLLENTRY
filter_command( void* f, ULONG msg, OUTPUT_PARAMS2* info )
{ DEBUGLOG(("logvolume:filter_command(%p, %u, %p)\n", f, msg, info));
  switch (msg)
  {case OUTPUT_VOLUME:
    // This is no logarithm, but it is similar to it in a reasonable range
    // and it keep the zero value at zero.
    info->volume /= scalefactor + 1. - scalefactor*info->volume; 
    break;
  }
  return (*f_output_command)( f_a, msg, info );
}

/********** Entry point: Initialize
*/
ULONG DLLENTRY
filter_init( void** F, FILTER_PARAMS2* params )
{ DEBUGLOG(("logvolume:filter_init(%p, {%u, ..., %p, ..., %p})\n", F, params->size, params->a, params->w));

  *F = NULL; // we have no instance data
 
  f_output_command = params->output_command;
  f_a              = params->a;
  
  params->output_command = &filter_command;

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

int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM *param )
{
  param->type         = PLUGIN_FILTER;
  param->author       = "Marcel Mller";
  param->desc         = VERSION;
  param->configurable = FALSE;
  param->interface    = FILTER_PLUGIN_LEVEL;
  return 0;
}
