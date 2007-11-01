/*
 * Copyright 2006 M.Mueller
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
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>

//#undef DEBUG
//#define DEBUG 2

#include <utilfct.h>
#include "plugman_base.h"
#include "pm123.h"
#include <vdelegate.h>

#include <limits.h>
#include <math.h>

#include <debuglog.h>


/* thread priorities for decoder thread */
#define  DECODER_HIGH_PRIORITY_CLASS PRTYC_FOREGROUNDSERVER // sorry, we should not lockup the system with time-critical priority
#define  DECODER_HIGH_PRIORITY_DELTA 20
#define  DECODER_LOW_PRIORITY_CLASS  PRTYC_REGULAR
#define  DECODER_LOW_PRIORITY_DELTA  1


#define DO_8(p,x) \
{  { const int p = 0; x; } \
   { const int p = 1; x; } \
   { const int p = 2; x; } \
   { const int p = 3; x; } \
   { const int p = 4; x; } \
   { const int p = 5; x; } \
   { const int p = 6; x; } \
   { const int p = 7; x; } \
}


// Convert timestamp in seconds to an integer in milliseconds.
// Truncate leading bits in case of an overflow.
static int tstmp_f2i(double pos)
{ DEBUGLOG(("tstmp_f2i(%f)\n", pos));
  // We cast to unsigned first to ensure that the range of the int is fully used.
  return pos >= 0 ? (int)(unsigned)(fmod(pos*1000., UINT_MAX+1.) + .5) : -1;
}

// Convert possibly truncated time stamp in milliseconds to seconds.
// The missing bits are taken from a context time stamp which has to be sufficiently close
// to the original time stamp. Sufficient is about ñ24 days.
static double tstmp_i2f(int pos, double context)
{ DEBUGLOG(("tstmp_i2f(%i, %f)\n", pos, context));
  if (pos < 0)
    return -1;
  double r = pos / 1000.;
  return r + (UINT_MAX+1.) * floor((context - r + UINT_MAX/2) / (UINT_MAX+1.));
}


/****************************************************************************
*
* CL_PLUGIN_BASE - C++ version of PLUGIN_BASE
*
****************************************************************************/

/* Assigns the address of the specified procedure within a plug-in. */
BOOL CL_PLUGIN_BASE::load_optional_function( void* function, const char* function_name )
{ return DosQueryProcAddr( module, 0L, (PSZ)function_name, (PFN*)function ) == NO_ERROR;
}

/* Assigns the address of the specified procedure within a plug-in. */
BOOL CL_PLUGIN_BASE::load_function( void* function, const char* function_name )
{ ULONG rc = DosQueryProcAddr( module, 0L, (PSZ)function_name, (PFN*)function );
  if( rc != NO_ERROR ) {
    char  error[1024];
    *((ULONG*)function) = 0;
    amp_player_error( "Could not load \"%s\" from %s\n%s", function_name,
                      module_name, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  }
  return TRUE;
}


/****************************************************************************
*
* CL_MODULE - object representing a plugin-DLL
*
****************************************************************************/

CL_MODULE::CL_MODULE(const char* name)
: refcount(0)
{ module = NULLHANDLE;
  strlcpy(module_name, name, sizeof module_name);
}

CL_MODULE::~CL_MODULE()
{ if (refcount)
    amp_player_error( "Fatal internal error in module handler:\n"
                      "trying to unload module %s with %d references.",
                      module_name, refcount);
   else
    unload_module();
}

/* Loads a plug-in dynamic link module. */
BOOL CL_MODULE::load_module()
{ char  load_error[_MAX_PATH];
  *load_error = 0;
  DEBUGLOG(("CL_MODULE(%p{%s})::load_module()\n", this, module_name));
  APIRET rc = DosLoadModule( load_error, sizeof( load_error ), (PSZ)module_name, &module );
  DEBUGLOG(("CL_MODULE::load_module: %p - %u\n", module, rc));
  // For some reason the API sometimes returns ERROR_INVALID_PARAMETER when loading oggplay.dll.
  // However, the module is loaded successfully.
  if( rc != NO_ERROR && rc != ERROR_INVALID_PARAMETER ) {
    char  error[1024];
    amp_player_error( "Could not load %s, %s. Error %d\n%s",
                      module_name, load_error, rc, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  }
  DEBUGLOG(("CL_MODULE({%p,%s})::load_module: TRUE\n", module, module_name));
  return TRUE;
}

/* Unloads a plug-in dynamic link module. */
BOOL CL_MODULE::unload_module()
{ DEBUGLOG(("CL_MODULE(%p{%p,%s}))::unload_module()\n", this, module, module_name));
  if (module == NULLHANDLE)
    return TRUE;
  APIRET rc = DosFreeModule( module );
  if( rc != NO_ERROR && rc != ERROR_INVALID_ACCESS ) {
    char  error[1024];
    amp_player_error( "Could not unload %s. Error %d\n%s",
                      module_name, rc, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  }
  module = NULLHANDLE;
  return TRUE;
}

/* Fills the basic properties of any plug-in.
         module:      input
         module_name: input
         query_param  output
         plugin_configure output
   return FALSE = error */
BOOL CL_MODULE::load()
{ DEBUGLOG(("CL_MODULE(%p{%s})::load()\n", this, module_name));

  void DLLENTRYP(plugin_query)( PLUGIN_QUERYPARAM *param );
  if ( !load_module() || !load_function( &plugin_query, "plugin_query" ) )
    return FALSE;

  query_param.interface = 0; // unchanged by old plug-ins
  (*plugin_query)(&query_param);

  if (query_param.interface > MAX_PLUGIN_LEVEL)
  {
    #define toconststring(x) #x
    amp_player_error( "Could not load plug-in %s because it requires a newer version of the PM123 core\n"
                      "Requested interface revision: %d, max. supported: " toconststring(MAX_PLUGIN_LEVEL),
      module_name, query_param.interface);
    #undef toconststring
    return FALSE;
  }

  return !query_param.configurable || load_function( &plugin_configure, "plugin_configure" );
}


/****************************************************************************
*
* CL_PLUGIN - object representing a plugin instance
*
****************************************************************************/

CL_PLUGIN::CL_PLUGIN(CL_MODULE& mod)
: CL_PLUGIN_BASE(mod), // copy global properties
  modref(mod),
  enabled(false)
{ mod.attach(this);
}

CL_PLUGIN::~CL_PLUGIN()
{ DEBUGLOG(("CL_PLUGIN(%p{%s})::~CL_PLUGIN\n", this, module_name));

  if (modref.detach(this)) // is last reference
    delete plugins.detach(plugins.find(modref)); // remove plugin from memory
}

void CL_PLUGIN::set_enabled(BOOL enabled)
{ this->enabled = enabled;
}


/****************************************************************************
*
* decoder interface
*
****************************************************************************/

CL_DECODER::~CL_DECODER()
{ DEBUGLOG(("CL_DECODER(%p{%s})::~CL_DECODER %p\n", this, module_name, support));

  if( support ) {
    char** cpp = support;
    while (*cpp)
      free(*cpp++);
    free( support );
  }
}

BOOL CL_DECODER::after_load()
{ DEBUGLOG(("CL_DECODER(%p{%s})::after_load()\n", this, module_name));

  char*   my_support[20];
  int     size = sizeof( my_support ) / sizeof( *my_support );
  int     i;

  w = NULL;
  enabled = TRUE;

  my_support[0] = (char*)malloc( _MAX_EXT * size );
  if( !my_support[0] ) {
    amp_player_error( "Not enough memory to decoder load." );
    return FALSE;
  }
  for( i = 1; i < size; i++ ) {
    my_support[i] = my_support[i-1] + _MAX_EXT;
  }

  type = decoder_support( my_support, &size );
  support = (char**)malloc(( size + 1 ) * sizeof( char* ));

  for( i = 0; i < size; i++ ) {
    support[i] = strdup( strupr( my_support[i] ));
    if( !support[i] ) {
      amp_player_error( "Not enough memory to decoder load." );
      return FALSE;
    }
  }
  support[i] = NULL;

  free( my_support[0] );
  return TRUE;
}

/* Assigns the addresses of the decoder plug-in procedures. */
BOOL CL_DECODER::load_plugin()
{ DEBUGLOG(("CL_DECODER(%p{%s})::load()\n", this, module_name));

  if ( !(query_param.type & PLUGIN_DECODER)
    || !load_function(&decoder_init,     "decoder_init"    )
    || !load_function(&decoder_uninit,   "decoder_uninit"  )
    || !load_function(&decoder_command,  "decoder_command" )
    || !load_function(&decoder_status,   "decoder_status"  )
    || !load_function(&decoder_length,   "decoder_length"  )
    || !load_function(&decoder_fileinfo, "decoder_fileinfo")
    || !load_function(&decoder_support,  "decoder_support" )
    || !load_function(&decoder_event,    "decoder_event"   ) )
    return FALSE;

  load_optional_function(&decoder_editmeta, "decoder_editmeta");
  load_optional_function(&decoder_getwizzard, "decoder_getwizzard");

  if (!after_load())
    return FALSE;

  if (type & DECODER_TRACK)
  { if (!load_function(&decoder_cdinfo,   "decoder_cdinfo"))
      return FALSE;
  } else
  { decoder_cdinfo = &stub_decoder_cdinfo;
  }
  return TRUE;
}

BOOL CL_DECODER::init_plugin()
{ DEBUGLOG(("CL_DECODER(%p{%s})::init_plugin()\n", this, module_name));

  return (*decoder_init)( &w ) != -1;
}

BOOL CL_DECODER::uninit_plugin()
{ DEBUGLOG(("CL_DECODER(%p{%s})::uninit_plugin()\n", this, module_name));

  (*decoder_uninit)( w );
  w = NULL;
  return TRUE;
}

PROXYFUNCIMP(ULONG DLLENTRY, CL_DECODER)
stub_decoder_cdinfo( const char* drive, DECODER_CDINFO* info )
{ return 100; // can't play CD
}


// Proxy for level 1 decoder interface
class CL_DECODER_PROXY_1 : public CL_DECODER
{private:
  ULONG  DLLENTRYP(vdecoder_command      )( void*  w, ULONG msg, DECODER_PARAMS* params );
  int    DLLENTRYP(voutput_request_buffer)( void* a, const FORMAT_INFO2* format, short** buf );
  void   DLLENTRYP(voutput_commit_buffer )( void* a, int len, double posmarker );
  void   DLLENTRYP(voutput_event         )( void* a, DECEVENTTYPE event, void* param );
  void*  a;
  ULONG  DLLENTRYP(vdecoder_fileinfo )( const char* filename, DECODER_INFO* info );
  ULONG  DLLENTRYP(vdecoder_trackinfo)( const char* drive, int track, DECODER_INFO* info );
  ULONG  DLLENTRYP(vdecoder_length   )( void* w );
  void   DLLENTRYP(error_display)( char* );
  HWND   hwnd; // Window handle for catching event messages
  ULONG  tid; // decoder thread id
  double temppos;
  DECFASTMODE lastfast;
  char   metadata_buffer[128]; // Loaded in curtun on decoder's demand WM_METADATA.
  VDELEGATE vd_decoder_command, vd_decoder_event, vd_decoder_fileinfo, vd_decoder_length;

 private:
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_decoder_command     ( CL_DECODER_PROXY_1* op, void* w, ULONG msg, DECODER_PARAMS2* params );
  PROXYFUNCDEF void   DLLENTRY proxy_1_decoder_event       ( CL_DECODER_PROXY_1* op, void* w, OUTEVENTTYPE event );
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_decoder_fileinfo    ( CL_DECODER_PROXY_1* op, const char* filename, DECODER_INFO2* info );
  PROXYFUNCDEF int    DLLENTRY proxy_1_decoder_play_samples( CL_DECODER_PROXY_1* op, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  PROXYFUNCDEF double DLLENTRY proxy_1_decoder_length      ( CL_DECODER_PROXY_1* op, void* w );
  friend MRESULT EXPENTRY proxy_1_decoder_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  CL_DECODER_PROXY_1(CL_MODULE& mod) : CL_DECODER(mod), hwnd(NULLHANDLE) {}
  virtual ~CL_DECODER_PROXY_1();
  virtual BOOL load_plugin();
};

CL_DECODER_PROXY_1::~CL_DECODER_PROXY_1()
{ if (hwnd != NULLHANDLE)
    WinDestroyWindow(hwnd);
}

/* Assigns the addresses of the decoder plug-in procedures. */
BOOL CL_DECODER_PROXY_1::load_plugin()
{ DEBUGLOG(("CL_DECODER_PROXY_1(%p{%s})::load()\n", this, module_name));

  if ( !(query_param.type & PLUGIN_DECODER)
    || !load_function(&decoder_init,       "decoder_init")
    || !load_function(&decoder_uninit,     "decoder_uninit")
    || !load_function(&decoder_status,     "decoder_status")
    || !load_function(&vdecoder_length,    "decoder_length")
    || !load_function(&vdecoder_fileinfo,  "decoder_fileinfo")
    || !load_function(&decoder_support,    "decoder_support")
    || !load_function(&vdecoder_command,   "decoder_command") )
    return FALSE;

  load_optional_function(&decoder_editmeta, "decoder_editmeta");
  load_optional_function(&decoder_getwizzard, "decoder_getwizzard");
  decoder_command   = vdelegate(&vd_decoder_command,  &proxy_1_decoder_command,  this);
  decoder_event     = vdelegate(&vd_decoder_event,    &proxy_1_decoder_event,    this);
  decoder_fileinfo  = vdelegate(&vd_decoder_fileinfo, &proxy_1_decoder_fileinfo, this);
  decoder_length    = vdelegate(&vd_decoder_length,   &proxy_1_decoder_length,   this);
  tid = (ULONG)-1;

  if (!after_load())
    return FALSE;

  if (type & DECODER_TRACK)
  { if ( !load_function(&decoder_cdinfo,     "decoder_cdinfo")
      || !load_function(&vdecoder_trackinfo, "decoder_trackinfo") )
      return FALSE;
  } else
  { decoder_cdinfo = &stub_decoder_cdinfo;
    vdecoder_trackinfo = NULL;
  }
  return TRUE;
}


/* proxy for the output callback of decoder interface level 0/1 */
PROXYFUNCIMP(int DLLENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_play_samples( CL_DECODER_PROXY_1* op, const FORMAT_INFO* format, const char* buf, int len, int posmarker )
{ DEBUGLOG(("proxy_1_decoder_play_samples(%p {%s}, %p {%u,%u,%u}, %p, %i, %i)\n",
    op, op->module_name, format, format->size, format->samplerate, format->channels, buf, len, posmarker));

  if (format->format != WAVE_FORMAT_PCM || (format->bits != 16 && format->bits != 8))
  { (*op->error_display)("PM123 does only accept PCM data with 8 or 16 bits per sample when using old-style plug-ins.");
    return 0; // error
  }

  op->temppos = -1; // buffer is empty now

  if (op->tid == (ULONG)-1)
  { PTIB ptib;
    DosGetInfoBlocks(&ptib, NULL);
    op->tid = ptib->tib_ordinal;
  }

  int bps = (format->bits >> 3) * format->channels;
  len /= bps;
  int rem = len;
  while (rem)
  { short* dest;
    int l = (*op->voutput_request_buffer)(op->a, (FORMAT_INFO2*)format, &dest);
    DEBUGLOG(("proxy_1_decoder_play_samples: now at %p %i %i %f\n", buf, l, rem, op->temppos));
    if (op->temppos != -1)
    { (*op->voutput_commit_buffer)(op->a, 0, op->temppos); // no-op
      break;
    }
    if (l <= 0)
      return (len - rem) * bps; // error
    if (l > rem)
      l = rem;
    if (format->bits == 8)
    { // convert to 16 bit
      int i;
      const unsigned char* sp = (const unsigned char*)buf;
      short* dp = dest;
      for (i = (l*format->channels) >> 3; i; --i)
      { DO_8(p, dp[p] = (sp[p] + 128) << 8);
        sp += 8;
        dp += 8;
      }
      for (i = (l*format->channels) & 7; i; --i)
      { *dp++ = (*sp++ + 128) << 8;
      }
    } else
    { memcpy(dest, buf, l*bps);
    }
    DEBUGLOG(("proxy_1_decoder_play_samples: commit: %i %f\n", posmarker, posmarker/1000. + (double)(len-rem)/format->samplerate));
    (*op->voutput_commit_buffer)(op->a, l, posmarker/1000. + (double)(len-rem)/format->samplerate);
    rem -= l;
    buf += l * bps;
  }
  return len * bps;
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(ULONG DLLENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_command( CL_DECODER_PROXY_1* op, void* w, ULONG msg, DECODER_PARAMS2* params )
{ DEBUGLOG(("proxy_1_decoder_command(%p {%s}, %p, %d, %p)\n", op, op->module_name, w, msg, params));

  if (params == NULL) // well, sometimes wired things may happen
    return (*op->vdecoder_command)(w, msg, NULL);

  static DECODER_PARAMS par1;
  memset(&par1, 0, sizeof par1);
  par1.size = sizeof par1;
  char filename[300];
  // preprocessing
  switch (msg)
  {case DECODER_PLAY:
    // decompose URL for old interface
    { char* cp = strchr(params->URL, ':') +2; // for URLs
      CDDA_REGION_INFO cd_info;
      if (is_file(params->URL))
      { if (strnicmp(params->URL, "file:///", 8) == 0)
        { strlcpy(filename, params->URL+8, sizeof filename);
          par1.filename = filename;
          cp = strchr(filename, '/');
          while (cp)
          { *cp = '\\';
            cp = strchr(cp+1, '/');
          }
        } else
          par1.filename = params->URL; // bare filename - normally this should not happen
        DEBUGLOG2(("proxy_1_decoder_command: filename=%s\n", par1.filename));
      } else if (scdparams(&cd_info, params->URL))
      { par1.drive = cd_info.drive;
        par1.track = cd_info.track;
        par1.sectors[0] = cd_info.sectors[0];
        par1.sectors[1] = cd_info.sectors[1];
      } else if (is_url(params->URL)) // URL
      { par1.URL = params->URL;
        par1.filename = params->URL; // Well, the URL parameter has never been used...
      } else
        par1.other = params->URL;

      par1.jumpto              = (int)floor(params->jumpto*1000 + .5);
    }
    break;

   case DECODER_SETUP:
    WinRegisterClass(amp_player_hab(), "CL_DECODER_PROXY_1", &proxy_1_decoder_winfn, 0, sizeof op);
    op->hwnd = WinCreateWindow(amp_player_window(), "CL_DECODER_PROXY_1", "", 0, 0,0, 0,0, NULLHANDLE, HWND_BOTTOM, 42, NULL, NULL);
    WinSetWindowPtr(op->hwnd, 0, op);

    par1.output_play_samples = (int DLLENTRYP()(void*, const FORMAT_INFO*, const char*, int, int))&PROXYFUNCREF(CL_DECODER_PROXY_1)proxy_1_decoder_play_samples;
    par1.a                   = op;
    par1.proxyurl            = params->proxyurl;
    par1.httpauth            = params->httpauth;
    par1.hwnd                = op->hwnd;
    par1.buffersize          = params->buffersize;
    par1.bufferwait          = params->bufferwait;
    par1.metadata_buffer     = op->metadata_buffer;
    par1.metadata_size       = sizeof(op->metadata_buffer);
    par1.audio_buffersize    = BUFSIZE;
    par1.error_display       = params->error_display;
    par1.info_display        = params->info_display;

    op->voutput_request_buffer = params->output_request_buffer;
    op->voutput_commit_buffer  = params->output_commit_buffer;
    op->voutput_event          = params->output_event;
    op->a                      = params->a;

    op->temppos  = -1;
    op->lastfast = DECFAST_NORMAL_PLAY;
    break;

   case DECODER_STOP:
    WinDestroyWindow(op->hwnd);
    op->hwnd = NULLHANDLE;
    op->temppos = out_playing_pos();
    break;

   case DECODER_FFWD:
   case DECODER_REW:
    DEBUGLOG(("proxy_1_decoder_command:DECODER_FFWD: %u\n", params->fast));
    par1.ffwd                = params->fast == DECFAST_FORWARD;
    par1.rew                 = params->fast == DECFAST_REWIND;
    if (params->fast != DECFAST_NORMAL_PLAY)
      op->lastfast = params->fast;
    if (op->lastfast == DECFAST_FORWARD)
      msg = DECODER_FFWD;
     else if (op->lastfast == DECFAST_REWIND)
      msg = DECODER_REW;
    op->temppos = out_playing_pos();
    op->lastfast = params->fast;
    break;

   case DECODER_JUMPTO:
    DEBUGLOG(("proxy_1_decoder_command:DECODER_JUMPTO: %f\n", params->jumpto));
    par1.jumpto              = (int)floor(params->jumpto*1000 +.5);
    op->temppos = params->jumpto;
    break;

   case DECODER_EQ:
    par1.equalizer           = params->equalizer;
    par1.bandgain            = params->bandgain;
    break;

   case DECODER_SAVEDATA:
    par1.save_filename       = params->save_filename;
    break;
  }

  return (*op->vdecoder_command)(w, msg, &par1);
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(void DLLENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_event( CL_DECODER_PROXY_1* op, void* w, OUTEVENTTYPE event )
{ DEBUGLOG(("proxy_1_decoder_event(%p {%s}, %p, %d)\n", op, op->module_name, w, event));

  switch (event)
  {case OUTEVENT_LOW_WATER:
    DosSetPriority(PRTYS_THREAD, DECODER_HIGH_PRIORITY_CLASS, DECODER_HIGH_PRIORITY_DELTA, op->tid);
    break;
   case OUTEVENT_HIGH_WATER:
    DosSetPriority(PRTYS_THREAD, DECODER_LOW_PRIORITY_CLASS, DECODER_LOW_PRIORITY_DELTA, op->tid);
    break;
   default:; // explicit no-op
  }
}

PROXYFUNCIMP(ULONG DLLENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_fileinfo( CL_DECODER_PROXY_1* op, const char* filename, DECODER_INFO2 *info )
{ DEBUGLOG(("proxy_1_decoder_fileinfo(%p, %s, %p{%u,%p,%p,%p})\n", op, filename, info, info->size, info->format, info->tech, info->meta));

  DECODER_INFO old_info;
  CDDA_REGION_INFO cd_info;
  ULONG rc;
  char buf[300];

  // purge the structure because of buggy plug-ins
  memset(&old_info, 0, sizeof old_info);
  info->tech->filesize   = -1;

  if (scdparams(&cd_info, filename))
  { if ( cd_info.track == 0 ||            // can't handle sectors
         op->vdecoder_trackinfo == NULL ) // plug-in does not support trackinfo
      return 200;
    rc = (*op->vdecoder_trackinfo)(cd_info.drive, cd_info.track, &old_info);
  } else
  { if (strnicmp(filename, "file:///", 8) == 0)
    { strlcpy(buf, filename+8, sizeof buf);
      filename = buf;
      char* cp = strchr(buf, '/');
      while (cp)
      { *cp = '\\';
        cp = strchr(cp+1, '/');
      }
    }
    // DEBUGLOG(("proxy_1_decoder_fileinfo - %s\n", filename));
    rc = (*op->vdecoder_fileinfo)(filename, &old_info);
    // get file size
    // TODO: large file support
    struct stat fi;
    if ( rc == 0 && is_file(filename) && stat( filename, &fi ) == 0 )
      info->tech->filesize = fi.st_size;
  }
  DEBUGLOG(("proxy_1_decoder_fileinfo: %lu\n", rc));

  // convert information to new format
  if (rc == 0)
  { // Slicing: the structure FORMAT_INFO2 is a subset of FORMAT_INFO.
    *info->format          = *(const FORMAT_INFO2*)&old_info.format;

    info->tech->songlength = old_info.songlength < 0 ? -1 : old_info.songlength/1000.;
    info->tech->bitrate    = old_info.bitrate;
    strlcpy(info->tech->info, old_info.tech_info, sizeof info->tech->info);
    info->tech->num_items  = 1;
    info->tech->recursive  = FALSE;

    // this part of the structure is binary compatible
    memcpy(&info->meta->title, old_info.title, offsetof(META_INFO, track) - offsetof(META_INFO, title));
    info->meta->track      = -1;
    info->meta_write      = op->decoder_editmeta && old_info.saveinfo;
  }
  return rc;
}

PROXYFUNCIMP(double DLLENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_length( CL_DECODER_PROXY_1* op, void* a )
{ DEBUGLOG(("proxy_1_decoder_length(%p, %p)\n", op, a));
  int i = (*op->vdecoder_length)(a);
  return i < 0 ? -1 : i / 1000.;
}

MRESULT EXPENTRY proxy_1_decoder_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ CL_DECODER_PROXY_1* op = (CL_DECODER_PROXY_1*)WinQueryWindowPtr(hwnd, 0);
  DEBUGLOG(("proxy_1_decoder_winfn(%p, %u, %p, %p) - %p {%s}\n", hwnd, msg, mp1, mp2, op, op == NULL ? NULL : op->module_name));
  switch (msg)
  {case WM_PLAYSTOP:
    (*op->voutput_event)(op->a, DECEVENT_PLAYSTOP, NULL);
    return 0;
   //case WM_PLAYERROR: // ignored because the error_display function implies the event.
   case WM_SEEKSTOP:
    (*op->voutput_event)(op->a, DECEVENT_SEEKSTOP, NULL);
    return 0;
   case WM_CHANGEBR:
    { // TODO: the unchanged information is missing now
      TECH_INFO ti = {sizeof(TECH_INFO), -1, LONGFROMMP(mp1), -1, ""};
      (*op->voutput_event)(op->a, DEVEVENT_CHANGETECH, &ti);
    }
    return 0;
   case WM_METADATA:
    { // TODO: the unchanged information is missing now
      META_INFO meta = {sizeof(META_INFO)};
      const char* metadata = (const char*)PVOIDFROMMP(mp1);
      const char* titlepos;
      // extract stream title
      if( metadata ) {
        titlepos = strstr( metadata, "StreamTitle='" );
        if ( titlepos )
        { unsigned int i;
          titlepos += 13;
          for( i = 0; i < sizeof( meta.title ) - 1 && *titlepos
                      && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
            meta.title[i] = *titlepos++;
          meta.title[i] = 0;
          (*op->voutput_event)(op->a, DECEVENT_CHANGEMETA, &meta);
        }
      }
    }
    return 0;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

CL_PLUGIN* CL_DECODER::factory(CL_MODULE& mod)
{ return mod.query_param.interface <= 1 ? new CL_DECODER_PROXY_1(mod) : new CL_DECODER(mod);
}


/****************************************************************************
*
* output interface
*
****************************************************************************/

/* Assigns the addresses of the out7put plug-in procedures. */
BOOL CL_OUTPUT::load_plugin()
{
  if ( !(query_param.type & PLUGIN_OUTPUT)
    || !load_function(&output_init,            "output_init")
    || !load_function(&output_uninit,          "output_uninit")
    || !load_function(&output_playing_samples, "output_playing_samples")
    || !load_function(&output_playing_pos,     "output_playing_pos")
    || !load_function(&output_playing_data,    "output_playing_data")
    || !load_function(&output_command,         "output_command")
    || !load_function(&output_request_buffer,  "output_request_buffer")
    || !load_function(&output_commit_buffer,   "output_commit_buffer") )
    return FALSE;

  a = NULL;
  return TRUE;
}

BOOL CL_OUTPUT::init_plugin()
{ DEBUGLOG(("CL_OUTPUT(%p{%s})::init_plugin()\n", this, module_name));

  return (*output_init)( &a ) == 0;
}

BOOL CL_OUTPUT::uninit_plugin()
{ DEBUGLOG(("CL_OUTPUT(%p{%s})::uninit_plugin()\n", this, module_name));

  (*output_command)( a, OUTPUT_CLOSE, NULL );
  (*output_uninit)( a );
  a = NULL;
  return TRUE;
}


// Proxy for loading level 1 plug-ins
class CL_OUTPUT_PROXY_1 : public CL_OUTPUT
{private:
  int         DLLENTRYP(voutput_command     )( void* a, ULONG msg, OUTPUT_PARAMS* info );
  int         DLLENTRYP(voutput_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  ULONG       DLLENTRYP(voutput_playing_pos )( void* a );
  short       voutput_buffer[BUFSIZE/2];
  int         voutput_buffer_level;             // current level of voutput_buffer
  BOOL        voutput_trash_buffer;
  BOOL        voutput_flush_request;            // flush-request received, generate OUTEVENT_END_OF_DATA from WM_OUTPUT_OUTOFDATA
  HWND        voutput_hwnd;                     // Window handle for catching event messages
  double      voutput_posmarker;
  FORMAT_INFO voutput_format;
  int         voutput_bufsamples;
  BOOL        voutput_always_hungry;
  void        DLLENTRYP(voutput_event)(void* w, OUTEVENTTYPE event);
  void*       voutput_w;
  VDELEGATE   vd_output_command, vd_output_request_buffer, vd_output_commit_buffer, vd_output_playing_pos;

 private:
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_output_command       ( CL_OUTPUT_PROXY_1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info );
  PROXYFUNCDEF int    DLLENTRY proxy_1_output_request_buffer( CL_OUTPUT_PROXY_1* op, void* a, const FORMAT_INFO2* format, short** buf );
  PROXYFUNCDEF void   DLLENTRY proxy_1_output_commit_buffer ( CL_OUTPUT_PROXY_1* op, void* a, int len, double posmarker );
  PROXYFUNCDEF double DLLENTRY proxy_1_output_playing_pos   ( CL_OUTPUT_PROXY_1* op, void* a );
  friend MRESULT EXPENTRY proxy_1_output_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  CL_OUTPUT_PROXY_1(CL_MODULE& mod) : CL_OUTPUT(mod), voutput_hwnd(NULLHANDLE), voutput_posmarker(0) {}
  virtual ~CL_OUTPUT_PROXY_1();
  virtual BOOL load_plugin();
};

CL_OUTPUT_PROXY_1::~CL_OUTPUT_PROXY_1()
{ if (voutput_hwnd != NULLHANDLE)
    WinDestroyWindow(voutput_hwnd);
}

/* Assigns the addresses of the out7put plug-in procedures. */
BOOL CL_OUTPUT_PROXY_1::load_plugin()
{
  if ( !(query_param.type & PLUGIN_OUTPUT)
    || !load_function(&output_init,            "output_init")
    || !load_function(&output_uninit,          "output_uninit")
    || !load_function(&output_playing_samples, "output_playing_samples")
    || !load_function(&voutput_playing_pos,    "output_playing_pos")
    || !load_function(&output_playing_data,    "output_playing_data")
    || !load_function(&voutput_command,        "output_command")
    || !load_function(&voutput_play_samples,   "output_play_samples") )
    return FALSE;

  output_command        = vdelegate(&vd_output_command,        &proxy_1_output_command,        this);
  output_request_buffer = vdelegate(&vd_output_request_buffer, &proxy_1_output_request_buffer, this);
  output_commit_buffer  = vdelegate(&vd_output_commit_buffer,  &proxy_1_output_commit_buffer,  this);
  output_playing_pos    = vdelegate(&vd_output_playing_pos,    &proxy_1_output_playing_pos,    this);

  a = NULL;
  return TRUE;
}

/* virtualization of level 1 output plug-ins */
PROXYFUNCIMP(ULONG DLLENTRY, CL_OUTPUT_PROXY_1)
proxy_1_output_command( CL_OUTPUT_PROXY_1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info )
{ DEBUGLOG(("proxy_1_output_command(%p {%s}, %p, %d, %p)\n", op, op->module_name, a, msg, info));

  if (info == NULL) // sometimes info is NULL
    return (*op->voutput_command)(a, msg, NULL);

  OUTPUT_PARAMS params;
  params.size                  = sizeof params;

  // preprocessing
  switch (msg)
  {case OUTPUT_TRASH_BUFFERS:
    op->voutput_trash_buffer   = TRUE;
    break;

   case OUTPUT_SETUP:
    WinRegisterClass(amp_player_hab(), "CL_OUTPUT_PROXY_1", &proxy_1_output_winfn, 0, sizeof op);
    op->voutput_hwnd = WinCreateWindow(amp_player_window(), "CL_OUTPUT_PROXY_1", "", 0, 0,0, 0,0, NULLHANDLE, HWND_BOTTOM, 43, NULL, NULL);
    //DEBUGLOG(("CL_OUTPUT_PROXY_1::init_plugin - C: %x, %x\n", voutput_hwnd, WinGetLastError(NULL)));
    WinSetWindowPtr(op->voutput_hwnd, 0, op);
    // copy corresponding fields
    params.formatinfo.size       = sizeof params.formatinfo;
    params.formatinfo.samplerate = info->formatinfo.samplerate;
    params.formatinfo.channels   = info->formatinfo.channels;
    params.formatinfo.bits       = 16;
    params.formatinfo.format     = WAVE_FORMAT_PCM;
    params.hwnd                  = op->voutput_hwnd;
    break;
  }
  params.buffersize            = BUFSIZE;
  params.boostclass            = DECODER_HIGH_PRIORITY_CLASS;
  params.normalclass           = DECODER_LOW_PRIORITY_CLASS;
  params.boostdelta            = DECODER_HIGH_PRIORITY_DELTA;
  params.normaldelta           = DECODER_LOW_PRIORITY_DELTA;
  params.nobuffermode          = FALSE;
  params.error_display         = info->error_display;
  params.info_display          = info->info_display;
  params.volume                = (char)(info->volume*100+.5);
  params.amplifier             = info->amplifier;
  params.pause                 = info->pause;
  params.temp_playingpos       = tstmp_f2i(info->temp_playingpos);
  if (info->URI != NULL && strnicmp(info->URI, "file://", 7) == 0)
    params.filename            = info->URI + 7;
   else
    params.filename            = info->URI;

  // call plug-in
  int r = (*op->voutput_command)(a, msg, &params);

  // postprocessing
  switch (msg)
  {case OUTPUT_SETUP:
    op->voutput_buffer_level   = 0;
    op->voutput_trash_buffer   = FALSE;
    op->voutput_flush_request  = FALSE;
    op->voutput_always_hungry  = params.always_hungry;
    op->voutput_event          = info->output_event;
    op->voutput_w              = info->w;
    op->voutput_format.size    = sizeof op->voutput_format;
    op->voutput_format.bits    = 16;
    op->voutput_format.format  = WAVE_FORMAT_PCM;
    break;

   case OUTPUT_CLOSE:
    WinDestroyWindow(op->voutput_hwnd);
    op->voutput_hwnd = NULLHANDLE;
  }
  DEBUGLOG(("proxy_1_output_command: %d\n", r));
  return r;
}

PROXYFUNCIMP(int DLLENTRY, CL_OUTPUT_PROXY_1)
proxy_1_output_request_buffer( CL_OUTPUT_PROXY_1* op, void* a, const FORMAT_INFO2* format, short** buf )
{
  #ifdef DEBUG
  if (format != NULL)
    DEBUGLOG(("proxy_1_output_request_buffer(%p, %p, {%i,%i,%i}, %p) - %d\n",
      op, a, format->size, format->samplerate, format->channels, buf, op->voutput_buffer_level));
   else
    DEBUGLOG(("proxy_1_output_request_buffer(%p, %p, %p, %p) - %d\n", op, a, format, buf, op->voutput_buffer_level));
  #endif

  if (op->voutput_trash_buffer)
  { op->voutput_buffer_level = 0;
    op->voutput_trash_buffer = FALSE;
  }

  if ( buf == 0
    || ( op->voutput_buffer_level != 0 &&
         (op->voutput_format.samplerate != format->samplerate || op->voutput_format.channels != format->channels) ))
  { // flush
    (*op->voutput_play_samples)(a, &op->voutput_format, (char*)op->voutput_buffer, op->voutput_buffer_level * op->voutput_format.channels * sizeof(short), tstmp_f2i(op->voutput_posmarker));
    op->voutput_buffer_level = 0;
  }
  if (buf == 0)
  { if (op->voutput_always_hungry)
      (*op->voutput_event)(op->voutput_w, OUTEVENT_END_OF_DATA);
     else
      op->voutput_flush_request = TRUE; // wait for WM_OUTPUT_OUTOFDATA
    return 0;
  }

  *buf = op->voutput_buffer + op->voutput_buffer_level * format->channels;
  op->voutput_format.samplerate = format->samplerate;
  op->voutput_format.channels   = format->channels;
  op->voutput_bufsamples        = sizeof op->voutput_buffer / sizeof *op->voutput_buffer / format->channels;
  DEBUGLOG(("proxy_1_output_request_buffer: %d\n", op->voutput_bufsamples - op->voutput_buffer_level));
  return op->voutput_bufsamples - op->voutput_buffer_level;
}

PROXYFUNCIMP(void DLLENTRY, CL_OUTPUT_PROXY_1)
proxy_1_output_commit_buffer( CL_OUTPUT_PROXY_1* op, void* a, int len, double posmarker )
{ DEBUGLOG(("proxy_1_output_commit_buffer(%p {%s}, %p, %i, %f) - %d\n",
    op, op->module_name, a, len, posmarker, op->voutput_buffer_level));

  if (op->voutput_buffer_level == 0)
    op->voutput_posmarker = posmarker;

  op->voutput_buffer_level += len;
  if (op->voutput_buffer_level == op->voutput_bufsamples)
  { (*op->voutput_play_samples)(a, &op->voutput_format, (char*)op->voutput_buffer, op->voutput_buffer_level * op->voutput_format.channels * sizeof(short), tstmp_f2i(op->voutput_posmarker));
    op->voutput_buffer_level = 0;
  }
}

PROXYFUNCIMP(double DLLENTRY, CL_OUTPUT_PROXY_1)
proxy_1_output_playing_pos( CL_OUTPUT_PROXY_1* op, void* a )
{ DEBUGLOG(("proxy_1_output_playing_pos(%p {%s}, %p)\n", op, op->module_name, a));
  return tstmp_i2f((*op->voutput_playing_pos)(a), op->voutput_posmarker);
}

MRESULT EXPENTRY proxy_1_output_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ CL_OUTPUT_PROXY_1* op = (CL_OUTPUT_PROXY_1*)WinQueryWindowPtr(hwnd, 0);
  DEBUGLOG2(("proxy_1_output_winfn(%p, %u, %p, %p) - %p {%s}\n", hwnd, msg, mp1, mp2, op, op == NULL ? NULL : op->module_name));
  switch (msg)
  {case WM_PLAYERROR:
    (*op->voutput_event)(op->voutput_w, OUTEVENT_PLAY_ERROR);
    return 0;
   case WM_OUTPUT_OUTOFDATA:
    if (op->voutput_flush_request) // don't care unless we have a flush_request condition
    { op->voutput_flush_request = FALSE;
      (*op->voutput_event)(op->a, OUTEVENT_END_OF_DATA);
    }
    return 0;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}


CL_PLUGIN* CL_OUTPUT::factory(CL_MODULE& mod)
{ return mod.query_param.interface <= 1 ? new CL_OUTPUT_PROXY_1(mod) : new CL_OUTPUT(mod);
}


/****************************************************************************
*
* filter interface
*
****************************************************************************/

/* Assigns the addresses of the filter plug-in procedures. */
BOOL CL_FILTER::load_plugin()
{ DEBUGLOG(("CL_FILTER(%p{%s})::load_plugin\n", this, module_name));

  if ( !(query_param.type & PLUGIN_FILTER)
    || !load_function(&filter_init,   "filter_init")
    || !load_function(&filter_update, "filter_update")
    || !load_function(&filter_uninit, "filter_uninit") )
    return FALSE;

  f = NULL;
  enabled = TRUE;
  return TRUE;
}

BOOL CL_FILTER::init_plugin()
{ return TRUE; // filters are not initialized unless they are used
}

BOOL CL_FILTER::uninit_plugin()
{ DEBUGLOG(("CL_FILTER(%p{%s})::uninit_plugin\n", this, module_name));

  (*filter_uninit)(f);
  f = NULL;
  return TRUE;
}

BOOL CL_FILTER::initialize(FILTER_PARAMS2* params)
{ DEBUGLOG(("CL_FILTER(%p{%s})::initialize(%p)\n", this, module_name, params));

  FILTER_PARAMS2 par = *params;
  if (is_initialized() || (*filter_init)(&f, params) != 0)
    return FALSE;

  if (f == NULL)
  { // plug-in does not require local structures
    // => pass the pointer of the next stage and skip virtualization of untouched function
    f = par.a;
  } else
  { // virtualize untouched functions
    if (par.output_command          == params->output_command)
      params->output_command         = vreplace1(&vrstubs[0], par.output_command, par.a);
    if (par.output_playing_samples  == params->output_playing_samples)
      params->output_playing_samples = vreplace1(&vrstubs[1], par.output_playing_samples, par.a);
    if (par.output_request_buffer   == params->output_request_buffer)
      params->output_request_buffer  = vreplace1(&vrstubs[2], par.output_request_buffer, par.a);
    if (par.output_commit_buffer    == params->output_commit_buffer)
      params->output_commit_buffer   = vreplace1(&vrstubs[3], par.output_commit_buffer, par.a);
    if (par.output_playing_pos      == params->output_playing_pos)
      params->output_playing_pos     = vreplace1(&vrstubs[4], par.output_playing_pos, par.a);
    if (par.output_playing_data     == params->output_playing_data)
      params->output_playing_data    = vreplace1(&vrstubs[5], par.output_playing_data, par.a);
  }
  return TRUE;
}


// proxy for level 1 filters
class CL_FILTER_PROXY_1 : public CL_FILTER
{private:
  int   DLLENTRYP(vfilter_init         )( void** f, FILTER_PARAMS* params );
  BOOL  DLLENTRYP(vfilter_uninit       )( void*  f );
  int   DLLENTRYP(vfilter_play_samples )( void*  f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
  void* vf;
  int   DLLENTRYP(output_request_buffer)( void*  a, const FORMAT_INFO2* format, short** buf );
  void  DLLENTRYP(output_commit_buffer )( void*  a, int len, double posmarker );
  void* a;
  void  DLLENTRYP(error_display        )( const char* );
  FORMAT_INFO vformat;                      // format of the samples
  short       vbuffer[BUFSIZE/2];           // buffer to store incoming samples
  int         vbufsamples;                  // size of vbuffer in samples
  int         vbuflevel;                    // current filled to vbuflevel
  double      vposmarker;                   // starting point of the current buffer
  BOOL        trash_buffer;                 // TRUE: signal to discard any buffer content
  VDELEGATE   vd_filter_init;
  VREPLACE1   vr_filter_update;
  VREPLACE1   vr_filter_uninit;

 private:
  PROXYFUNCDEF ULONG DLLENTRY proxy_1_filter_init          ( CL_FILTER_PROXY_1* pp, void** f, FILTER_PARAMS2* params );
  PROXYFUNCDEF void  DLLENTRY proxy_1_filter_update        ( CL_FILTER_PROXY_1* pp, const FILTER_PARAMS2* params );
  PROXYFUNCDEF BOOL  DLLENTRY proxy_1_filter_uninit        ( void* f ); // empty stub
  PROXYFUNCDEF int   DLLENTRY proxy_1_filter_request_buffer( CL_FILTER_PROXY_1* f, const FORMAT_INFO2* format, short** buf );
  PROXYFUNCDEF void  DLLENTRY proxy_1_filter_commit_buffer ( CL_FILTER_PROXY_1* f, int len, double posmarker );
  PROXYFUNCDEF int   DLLENTRY proxy_1_filter_play_samples  ( CL_FILTER_PROXY_1* f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
 public:
  CL_FILTER_PROXY_1(CL_MODULE& mod) : CL_FILTER(mod) {}
  virtual BOOL load_plugin();
};

BOOL CL_FILTER_PROXY_1::load_plugin()
{
  if ( !(query_param.type & PLUGIN_FILTER)
    || !load_function(&vfilter_init,         "filter_init")
    || !load_function(&vfilter_uninit,       "filter_uninit")
    || !load_function(&vfilter_play_samples, "filter_play_samples") )
    return FALSE;

  filter_init   = vdelegate(&vd_filter_init,   &proxy_1_filter_init,   this);
  filter_update = (void DLLENTRYP()(void*, const FILTER_PARAMS2*)) // type of parameter is replaced too
                  vreplace1(&vr_filter_update, &proxy_1_filter_update, this);
  // filter_uninit is initialized at the filter_init call to a non-no-op function
  // However, the returned pointer will stay the same.
  filter_uninit = vreplace1(&vr_filter_uninit, &proxy_1_filter_uninit, (void*)NULL);

  f = NULL;
  enabled = TRUE;
  return TRUE;
}

PROXYFUNCIMP(ULONG DLLENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_init( CL_FILTER_PROXY_1* pp, void** f, FILTER_PARAMS2* params )
{ DEBUGLOG(("proxy_1_filter_init(%p{%s}, %p, %p{a=%p})\n", pp, pp->module_name, f, params, params->a));

  FILTER_PARAMS par;
  par.size                = sizeof par;
  par.output_play_samples = (int DLLENTRYP()(void*, const FORMAT_INFO*, const char*, int, int))
                            &PROXYFUNCREF(CL_FILTER_PROXY_1)proxy_1_filter_play_samples;
  par.a                   = pp;
  par.audio_buffersize    = BUFSIZE;
  par.error_display       = params->error_display;
  par.info_display        = params->info_display;
  int r = (*pp->vfilter_init)(&pp->vf, &par);
  if (r != 0)
    return r;
  // save some values
  pp->output_request_buffer = params->output_request_buffer;
  pp->output_commit_buffer  = params->output_commit_buffer;
  pp->a                     = params->a;
  pp->error_display         = params->error_display;
  // setup internals
  pp->vbuflevel             = 0;
  pp->trash_buffer          = FALSE;
  pp->vformat.size          = sizeof pp->vformat.size;
  pp->vformat.bits          = 16;
  pp->vformat.format        = WAVE_FORMAT_PCM;
  // replace the unload function
  vreplace1(&pp->vr_filter_uninit, pp->vfilter_uninit, pp->vf);
  // now return some values
  *f = pp;
  params->output_request_buffer = (int  DLLENTRYP()(void*, const FORMAT_INFO2*, short**))
                                  &PROXYFUNCREF(CL_FILTER_PROXY_1)proxy_1_filter_request_buffer;
  params->output_commit_buffer  = (void DLLENTRYP()(void*, int, double))
                                  &PROXYFUNCREF(CL_FILTER_PROXY_1)proxy_1_filter_commit_buffer;
  return 0;
}

PROXYFUNCIMP(void DLLENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_update( CL_FILTER_PROXY_1* pp, const FILTER_PARAMS2* params )
{ DEBUGLOG(("proxy_1_filter_update(%p{%s}, %p)\n", pp, pp->module_name, params));

  DosEnterCritSec();
  // replace function pointers
  pp->output_request_buffer = params->output_request_buffer;
  pp->output_commit_buffer  = params->output_commit_buffer;
  pp->a                     = params->a;
  DosExitCritSec();
}

PROXYFUNCIMP(BOOL DLLENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_uninit( void* )
{ return TRUE;
}

PROXYFUNCIMP(int DLLENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_request_buffer( CL_FILTER_PROXY_1* pp, const FORMAT_INFO2* format, short** buf )
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
    (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, (char*)pp->vbuffer, pp->vbuflevel * pp->vformat.channels * sizeof(short), tstmp_f2i(pp->vposmarker));
  }
  if ( buf == 0 )
  { return (*pp->output_request_buffer)( pp->a, format, NULL );
  }
  pp->vformat.samplerate = format->samplerate;
  pp->vformat.channels   = format->channels;
  pp->vbufsamples        = sizeof pp->vbuffer / sizeof *pp->vbuffer / format->channels;

  DEBUGLOG(("proxy_1_filter_request_buffer: %d\n", pp->vbufsamples - pp->vbuflevel));
  *buf = pp->vbuffer + pp->vbuflevel * format->channels;
  return pp->vbufsamples - pp->vbuflevel;
}

PROXYFUNCIMP(void DLLENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_commit_buffer( CL_FILTER_PROXY_1* pp, int len, double posmarker )
{ DEBUGLOG(("proxy_1_filter_commit_buffer(%p, %d, %f)\n", pp, len, posmarker));

  if (len == 0)
    return;

  if (pp->vbuflevel == 0)
    pp->vposmarker = posmarker;

  pp->vbuflevel += len;
  if (pp->vbuflevel == pp->vbufsamples)
  { // buffer full
    DEBUGLOG(("proxy_1_filter_commit_buffer: full: %d\n", pp->vbuflevel));
    (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, (char*)pp->vbuffer, BUFSIZE, tstmp_f2i(pp->vposmarker));
    pp->vbuflevel = 0;
  }
}

PROXYFUNCIMP(int DLLENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_play_samples( CL_FILTER_PROXY_1* pp, const FORMAT_INFO* format, const char *buf, int len, int posmarker_i )
{ DEBUGLOG(("proxy_1_filter_play_samples(%p, %p{%d,%d,%d,%d}, %p, %d, %d)\n",
    pp, format, format->samplerate, format->channels, format->bits, format->format, buf, len, posmarker_i));

  if (format->format != WAVE_FORMAT_PCM || format->bits != 16)
  { (*pp->error_display)("The proxy for old style filter plug-ins can only handle 16 bit raw PCM data.");
    return 0;
  }
  double posmarker = tstmp_i2f(posmarker_i, pp->vposmarker);
  len /= pp->vformat.channels * sizeof(short);
  int rem = len;
  while (rem != 0)
  { // request new buffer
    short* dest;
    int dlen = (*pp->output_request_buffer)( pp->a, (FORMAT_INFO2*)format, &dest );
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


CL_PLUGIN* CL_FILTER::factory(CL_MODULE& mod)
{ return mod.query_param.interface <= 1 ? new CL_FILTER_PROXY_1(mod) : new CL_FILTER(mod);
}


/****************************************************************************
*
* visualization interface
*
****************************************************************************/

/* Assigns the addresses of the visual plug-in procedures. */
BOOL CL_VISUAL::load_plugin()
{
  if ( !(query_param.type & PLUGIN_VISUAL)
    || !load_function(&plugin_deinit, "plugin_deinit")
    || !load_function(&plugin_init,   "vis_init") )
    return FALSE;

  enabled = TRUE;
  hwnd = NULLHANDLE;
  return TRUE;
}

BOOL CL_VISUAL::init_plugin()
{ return TRUE; // no automatic init
}

BOOL CL_VISUAL::uninit_plugin()
{ if (!is_initialized())
    return TRUE;

  (*plugin_deinit)(0);
  hwnd = NULLHANDLE;
  return TRUE;
}

BOOL CL_VISUAL::initialize(HWND hwnd, PLUGIN_PROCS* procs, int id)
{ DEBUGLOG(("CL_VISUAL(%p{%s})::initialize(%x, %p, %d) - %d %d, %d %d, %s\n",
    this, module_name, hwnd, procs, id, props.x, props.y, props.cx, props.cy, props.param));

  VISPLUGININIT visinit;
  visinit.x       = props.x;
  visinit.y       = props.y;
  visinit.cx      = props.cx;
  visinit.cy      = props.cy;
  visinit.hwnd    = hwnd;
  visinit.procs   = procs;
  visinit.id      = id;
  visinit.param   = props.param;

  this->hwnd = (*plugin_init)(&visinit);
  return this->hwnd != NULLHANDLE;
}

void CL_VISUAL::set_properties(const VISUAL_PROPERTIES* data)
{
  #ifdef DEBUG
  if (data)
    DEBUGLOG(("CL_VISUAL(%p{%s})::set_properties(%p{%d %d, %d %d, %s})\n",
      this, module_name, data, data->x, data->y, data->cx, data->cy, data->param));
  else
    DEBUGLOG(("CL_VISUAL(%p{%s})::set_properties(%p)\n", this, module_name, data));
  #endif
  static const VISUAL_PROPERTIES def_visuals = {0,0,0,0, FALSE, ""};
  props = data == NULL ? def_visuals : *data;
}


CL_PLUGIN* CL_VISUAL::factory(CL_MODULE& mod)
{ if (mod.query_param.interface == 0)
  { amp_player_error( "Could not load visual plug-in %s because it is designed for PM123 before vesion 1.32\n"
                      "Please get a newer version of this plug-in which supports at least interface revision 1.",
      mod.module_name);
    return NULL;
  }
  return new CL_VISUAL(mod);
}


/****************************************************************************
*
* PLUGIN_BASE_LIST collection
*
****************************************************************************/

/* append an new plug-in record to the list */
BOOL CL_PLUGIN_BASE_LIST::append(CL_PLUGIN_BASE* plugin)
{ DEBUGLOG(("CL_PLUGIN_BASE_LIST(%p{%p,%d,%d})::append(%p{%p,%s})\n",
    this, list, num, size, plugin, plugin->module, plugin->module_name));

  if (num >= size)
  { int l = size + 8;
    // Note: The Debug memory management of IMB C++ will claim the objects allocated here are not freed
    // until the program ends. This is not true. The clean-up function of the debug memory manager is only called
    // before the destructors of the static objects.
    CL_PLUGIN_BASE** np = (CL_PLUGIN_BASE**)realloc(list, l * sizeof *list);
    if (np == NULL)
    { DEBUGLOG(("CL_PLUGIN_BASE_LIST::append: internal memory allocation error\n"));
      return FALSE;
    }
    list = np;
    size = l;
  }

  list[num++] = plugin;
  return TRUE;
}

/* remove the i-th plug-in record from the list */
CL_PLUGIN_BASE* CL_PLUGIN_BASE_LIST::detach(int i)
{ DEBUGLOG(("CL_PLUGIN_BASE_LIST(%p{%p,%d,%d})::detach(%i)\n", this, list, num, size, i));

  if (i > num && i <= 0)
  { DEBUGLOG(("CL_PLUGIN_BASE_LIST::detach: index out of range\n"));
    return NULL;
  }

  CL_PLUGIN_BASE* r = list[i];
  memmove(list + i, list + i +1, (--num - i) * sizeof *list);

  DEBUGLOG(("CL_PLUGIN_BASE_LIST::detach: %p\n", r));
  return r;
}

/* find a plug-in in the list
   return the position in the list or -1 if not found */
int CL_PLUGIN_BASE_LIST::find(const PLUGIN_BASE& plugin) const
{ CL_PLUGIN_BASE** pp = list + num;
  while (pp-- != list)
    if ((*pp)->module == plugin.module)
      return pp - list;
  return -1;
}
int CL_PLUGIN_BASE_LIST::find(const char* name) const
{ CL_PLUGIN_BASE** pp = list + num;
  while (pp-- != list)
  { //fprintf(stderr, "CL_PLUGIN_BASE_LIST::find %s %s\n", (*pp)->module_name, name);
    if (strcmp((*pp)->module_name, name) == 0)
      return pp - list;
  }
  return -1;
}

int CL_PLUGIN_BASE_LIST::find_short(const char* name) const
{ CL_PLUGIN_BASE** pp = list + num;
  while (pp-- != list)
  { char filename[_MAX_FNAME];
    sfnameext( filename, (*pp)->module_name, sizeof( filename ));
    if( stricmp( filename, name ) == 0 )
      return pp - list;
  }
  return -1;
}


/****************************************************************************
*
* MODULE_LIST collection
*
****************************************************************************/


/****************************************************************************
*
* PLUGIN_LIST collection
*
****************************************************************************/

CL_PLUGIN* CL_PLUGIN_LIST::detach(int i)
{ DEBUGLOG(("CL_PLUGIN_LIST(%p)::detach(%i)\n", this, i));

  if (i > count() && i <= 0)
  { DEBUGLOG(("CL_PLUGIN_LIST::detach: index out of range\n"));
    return NULL;
  }
  if ((*this)[i].is_initialized() && !(*this)[i].uninit_plugin())
  { DEBUGLOG(("CL_PLUGIN_LIST::detach: plugin %s failed to uninitialize.\n", (*this)[i].module_name));
    return NULL;
  }
  return (CL_PLUGIN*)CL_PLUGIN_BASE_LIST::detach(i);
}

BOOL CL_PLUGIN_LIST::remove(int i)
{ CL_PLUGIN* pp = detach(i);
  delete pp;
  return pp != NULL;
}

void CL_PLUGIN_LIST::clear()
{ while (count())
  { CL_PLUGIN* pp = detach(count()-1);
    DEBUGLOG(("CL_PLUGIN_LIST::clear: %p.\n", pp));
    delete pp;
  }
}


/****************************************************************************
*
* PLUGIN_LIST collection with one active plugin
*
****************************************************************************/

CL_PLUGIN* CL_PLUGIN_LIST1::detach(int i)
{ CL_PLUGIN* pp = CL_PLUGIN_LIST::detach(i);
  if ( pp == active )
    active = NULL;
  return pp;
}

int CL_PLUGIN_LIST1::set_active(int i)
{ if ( i >= count() || i < -1 )
    return -1;

  CL_PLUGIN* pp = i == -1 ? NULL : &(*this)[i];
  if (pp == active)
    return 0;

  if ( active != NULL && !active->uninit_plugin() )
    return -1;

  if (pp != NULL)
  { if (!pp->get_enabled())
      return -2;
    if (!pp->init_plugin())
      return -1;
  }
  active = pp;
  return 0;
}



