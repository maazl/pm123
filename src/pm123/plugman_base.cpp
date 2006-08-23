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

#define  INCL_PM
#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <utilfct.h>
#include "plugman_base.h"
#include "pm123.h"
#include "vdelegate.h"

#define  DEBUG 1


/* thread priorities for decoder thread */
#define  DECODER_HIGH_PRIORITY_CLASS PRTYC_FOREGROUNDSERVER // sorry, we should not lockup the system with time-critical priority
#define  DECODER_HIGH_PRIORITY_DELTA 20
#define  DECODER_LOW_PRIORITY_CLASS  PRTYC_REGULAR
#define  DECODER_LOW_PRIORITY_DELTA  1


/****************************************************************************
*
* VREPLACE collection
*
****************************************************************************/

/*VREPLACE* CL_STUBLIST::factory()
{ if (num >= size)
  { int l = size + 10;
    VREPLACE** np = (VREPLACE**)realloc(list, l * sizeof *list);
    if (np == NULL)
    {
      #ifdef DEBUG
      fprintf(stderr, "CL_STUBLIST::factory: internal memory allocation error\n");
      #endif
      return NULL;
    }
    list = np;
    size = l;
  }
  return list[num++] = new VREPLACE;
}

void CL_STUBLIST::clear()
{ for (VREAPLACE** vrp = list + num; vrp-- != list; )
    delete *vrp;
  num = 0;
}*/


/****************************************************************************
*
* CL_PLUGIN_BASE - C++ version of PLUGIN_BASE
*
****************************************************************************/

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
{ char  error[1024];
  APIRET rc = DosLoadModule( error, sizeof( error ), (PSZ)module_name, &module );
  if( rc != NO_ERROR ) {
    amp_player_error( "Could not load %s\n%s",
                      module_name, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  }
  return TRUE;
}

/* Unloads a plug-in dynamic link module. */
BOOL CL_MODULE::unload_module()
{ APIRET rc = DosFreeModule( module );
  if( rc != NO_ERROR && rc != ERROR_INVALID_ACCESS ) {
    char  error[1024];
    amp_player_error( "Could not unload %s\n%s",
                      module_name, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  }
  module = NULLHANDLE;
  return TRUE;
}

/* Fills the basic properties of any plug-in.
         module:      input
         module_name: input
         enabled:     unused
         query_param  output
         plugin_query output
         plugin_configure output
   return FALSE = error */
BOOL CL_MODULE::load()
{
  void (PM123_ENTRYP plugin_query)( PLUGIN_QUERYPARAM *param );
  
  #ifdef DEBUG
  fprintf(stderr, "CL_MODULE(%p{%s})::load()\n", this, module_name);
  #endif

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
{
  #ifdef DEBUG
  fprintf(stderr, "CL_PLUGIN(%p{%s})::~CL_PLUGIN\n", this, module_name);
  #endif
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
{ 
  #ifdef DEBUG
  fprintf(stderr, "CL_DECODER(%p{%s})::~CL_DECODER %p\n", this, module_name, support);
  #endif
  if( support ) {
    char** cpp = support;
    while (*cpp)
      free(*cpp++);
    free( support );
  }
}
  
BOOL CL_DECODER::after_load()
{
  #ifdef DEBUG
  fprintf(stderr, "CL_DECODER(%p{%s})::after_load()\n", this, module_name);
  #endif
  char*   my_support[20];
  int     size = sizeof( my_support ) / sizeof( *my_support );
  int     i;
  BOOL    rc;

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
{
  #ifdef DEBUG
  fprintf(stderr, "CL_DECODER(%p{%s})::load()\n", this, module_name);
  #endif
  
  if ( !(query_param.type & PLUGIN_DECODER)
    || !load_function(&decoder_init,     "decoder_init")
    || !load_function(&decoder_uninit,   "decoder_uninit")
    || !load_function(&decoder_command,  "decoder_command")
    || !load_function(&decoder_status,   "decoder_status")
    || !load_function(&decoder_length,   "decoder_length")
    || !load_function(&decoder_fileinfo, "decoder_fileinfo")
    || !load_function(&decoder_cdinfo,   "decoder_cdinfo")
    || !load_function(&decoder_support,  "decoder_support")
    || !load_function(&decoder_event,    "decoder_event") )
    return FALSE;

  return after_load();
}

BOOL CL_DECODER::init_plugin()
{
  #ifdef DEBUG
  fprintf(stderr, "CL_DECODER(%p{%s})::init_plugin()\n", this, module_name);
  #endif
  return (*decoder_init)( &w ) != -1;
}

BOOL CL_DECODER::uninit_plugin()
{ 
  #ifdef DEBUG
  fprintf(stderr, "CL_DECODER(%p{%s})::uninit_plugin()\n", this, module_name);
  #endif
  (*decoder_uninit)( w );
  w = NULL;
  return TRUE;
}


// Proxy for level 1 decoder interface
class CL_DECODER_PROXY_1 : public CL_DECODER
{private:
  ULONG (PM123_ENTRYP vdecoder_command )( void*  w, ULONG msg, DECODER_PARAMS* params );
  int   (PM123_ENTRYP voutput_request_buffer )( void* a, const FORMAT_INFO* format, char** buf, int posmarker );
  void  (PM123_ENTRYP voutput_commit_buffer  )( void* a, int len );
  void* a;
  ULONG (PM123_ENTRYP vdecoder_fileinfo )( char* filename, DECODER_INFO *info );
  ULONG (PM123_ENTRYP vdecoder_trackinfo)( char* drive, int track, DECODER_INFO* info );
  ULONG tid; // decoder thread id
  VDELEGATE vd_decoder_command, vd_decoder_event, vd_decoder_fileinfo;

 private:
  PROXYFUNCDEF ULONG PM123_ENTRY proxy_1_decoder_command     ( CL_DECODER_PROXY_1* op, void* w, ULONG msg, DECODER_PARAMS2* params );
  PROXYFUNCDEF void  PM123_ENTRY proxy_1_decoder_event       ( CL_DECODER_PROXY_1* op, void* w, OUTEVENTTYPE event );
  PROXYFUNCDEF ULONG PM123_ENTRY proxy_1_decoder_fileinfo    ( CL_DECODER_PROXY_1* op, char* filename, DECODER_INFO *info );
  PROXYFUNCDEF int   PM123_ENTRY proxy_1_decoder_play_samples( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
 public: 
  CL_DECODER_PROXY_1(CL_MODULE& mod) : CL_DECODER(mod) {}
  virtual BOOL load_plugin();
};

/* Assigns the addresses of the decoder plug-in procedures. */
BOOL CL_DECODER_PROXY_1::load_plugin()
{
  #ifdef DEBUG
  fprintf(stderr, "CL_DECODER_PROXY_1(%p{%s})::load()\n", this, module_name);
  #endif

  if ( !(query_param.type & PLUGIN_DECODER)
    || !load_function(&decoder_init,       "decoder_init")
    || !load_function(&decoder_uninit,     "decoder_uninit")
    || !load_function(&decoder_status,     "decoder_status")
    || !load_function(&decoder_length,     "decoder_length")
    || !load_function(&decoder_fileinfo,   "decoder_fileinfo")
    || !load_function(&decoder_cdinfo,     "decoder_cdinfo")
    || !load_function(&decoder_support,    "decoder_support")
    || !load_function(&vdecoder_command,   "decoder_command")
    || !load_function(&vdecoder_trackinfo, "decoder_trackinfo") )
    return FALSE;
  decoder_command   = (ULONG (PM123_ENTRYP)(void*, ULONG, DECODER_PARAMS2*))
                      mkvdelegate(&vd_decoder_command,  (V_FUNC)&proxy_1_decoder_command,  3, this);
  decoder_event     = (void  (PM123_ENTRYP)(void*, OUTEVENTTYPE))
                      mkvdelegate(&vd_decoder_event,    (V_FUNC)&proxy_1_decoder_event,    2, this);
  vdecoder_fileinfo = decoder_fileinfo;
  decoder_fileinfo  = (ULONG (PM123_ENTRYP)(char*, DECODER_INFO*))
                      mkvdelegate(&vd_decoder_fileinfo, (V_FUNC)&proxy_1_decoder_fileinfo, 2, this);
  tid = (ULONG)-1;
    
  return after_load();
}

/* proxy for the output callback of decoder interface level 0/1 */
PROXYFUNCIMP(int PM123_ENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_play_samples( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker )
{ CL_DECODER_PROXY_1* op = (CL_DECODER_PROXY_1*)a;
  int rem = len;

  #ifdef DEBUG
  fprintf(stderr, "proxy_1_output_play_samples(%p {%s}, %p, %p, %i, %i)\n",
    op, op->module_name, format, buf, len, posmarker);
  #endif

  if (op->tid == (ULONG)-1)
  { PTIB ptib;
    PPIB ppib;
    DosGetInfoBlocks(&ptib, &ppib);
    op->tid = ptib->tib_ordinal;
  }

  while (rem)
  { char* dest;
    int l = (*op->voutput_request_buffer)(op->a, format, &dest, posmarker);
    #ifdef DEBUG
    fprintf(stderr, "proxy_1_output_play_samples: now at %p %i %i\n", buf, l, rem);
    #endif
    if (l == 0)
      return len - rem; // error
    if (l > rem)
      l = rem;
    memcpy(dest, buf, l);
    (*op->voutput_commit_buffer)(op->a, l); // Well, normally posmarker should be interpolated...
    rem -= l;
    buf += l; 
  }
  return len;
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(ULONG PM123_ENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_command( CL_DECODER_PROXY_1* op, void* w, ULONG msg, DECODER_PARAMS2* params )
{ DECODER_PARAMS par1;
  CDDA_REGION_INFO cd_info;
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_decoder_command(%p {%s}, %p, %d, %p)\n",
    op, op->module_name, w, msg, params);
  #endif
  
  if (params == NULL) // well, sometimes wired things may happen
    return (*op->vdecoder_command)(w, msg, NULL);

  memset(&par1, 0, sizeof par1); 
  par1.size                = sizeof par1;
  /* decompose URL for old interface */
  if (params->URL != NULL)
  { char* cp = strchr(params->URL, ':') +2; // for URLs
    if (is_file(params->URL))
    { if (is_url(params->URL))
        par1.filename = cp + (params->URL[7] == '/' && params->URL[9] == ':' && params->URL[10] == '/');
       else
        par1.filename = params->URL; // bare filename - normally this should not happen
      #ifdef DEBUG
      fprintf(stderr, "proxy_1_decoder_command: filename=%s\n", par1.filename);
      #endif
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
  }
  par1.jumpto              = params->jumpto;
  par1.ffwd                = params->ffwd;
  par1.rew                 = params->rew;
  par1.output_play_samples = &PROXYFUNCREF(CL_DECODER_PROXY_1)proxy_1_decoder_play_samples;
  par1.a                   = op;
  par1.audio_buffersize    = BUFSIZE;
  par1.proxyurl            = params->proxyurl;
  par1.httpauth            = params->httpauth;
  par1.error_display       = params->error_display;
  par1.info_display        = params->info_display;
  par1.playsem             = params->playsem;
  par1.hwnd                = params->hwnd;
  par1.buffersize          = params->buffersize;
  par1.bufferwait          = params->bufferwait;
  par1.metadata_buffer     = params->metadata_buffer;
  par1.metadata_size       = params->metadata_size;
  par1.bufferstatus        = params->bufferstatus;
  par1.equalizer           = params->equalizer;
  par1.bandgain            = params->bandgain;
  par1.save_filename       = params->save_filename;
  
  switch (msg)
  {case DECODER_SETUP:
    op->voutput_request_buffer = params->output_request_buffer;
    op->voutput_commit_buffer  = params->output_commit_buffer;
    op->a                      = params->a;
  }
  
  return (*op->vdecoder_command)(w, msg, &par1);
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(void PM123_ENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_event( CL_DECODER_PROXY_1* op, void* w, OUTEVENTTYPE event )
{
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_decoder_event(%p {%s}, %p, %d)\n",
    op, op->module_name, w, event);
  #endif
  
  switch (event)
  {case OUTEVENT_LOW_WATER:
    DosSetPriority(PRTYS_THREAD, DECODER_HIGH_PRIORITY_CLASS, DECODER_HIGH_PRIORITY_DELTA, op->tid);
    break;
   case OUTEVENT_HIGH_WATER:
    DosSetPriority(PRTYS_THREAD, DECODER_LOW_PRIORITY_CLASS, DECODER_LOW_PRIORITY_DELTA, op->tid);
    break;
  }
}

PROXYFUNCIMP(ULONG PM123_ENTRY, CL_DECODER_PROXY_1)
proxy_1_decoder_fileinfo( CL_DECODER_PROXY_1* op, char* filename, DECODER_INFO *info )
{ 
  CDDA_REGION_INFO cd_info;
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_decoder_fileinfo(%p, %s, %p)\n", op, filename, info);
  #endif
  if (scdparams(&cd_info, filename))
  { if ( cd_info.track == 0 // can't handle sectors so far
      || op->vdecoder_trackinfo == NULL )
      return 200;
    ULONG rc = (*op->vdecoder_trackinfo)(cd_info.drive, cd_info.track, info);
    #ifdef DEBUG
    fprintf(stderr, "proxy_1_decoder_fileinfo: %lu\n", rc);
    #endif
    return rc;
  } else if (is_file(filename) && is_url(filename))
    return (*op->vdecoder_fileinfo)(filename+7, info);
   else
    return (*op->vdecoder_fileinfo)(filename, info);
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
{
  #ifdef DEBUG
  fprintf(stderr, "CL_OUTPUT(%p{%s})::init()\n", this, module_name);
  #endif
  return (*output_init)( &a ) == 0;
}

BOOL CL_OUTPUT::uninit_plugin()
{
  #ifdef DEBUG
  fprintf(stderr, "CL_OUTPUT(%p{%s})::uninit()\n", this, module_name);
  #endif
  (*output_command)( a, OUTPUT_CLOSE, NULL );
  (*output_uninit)( a );
  a = NULL;
  return TRUE;
}


// Proxy for loading level 1 plug-ins
class CL_OUTPUT_PROXY_1 : public CL_OUTPUT
{private:
  int   (PM123_ENTRYP voutput_command       )( void*  a, ULONG msg, OUTPUT_PARAMS* info );
  int   (PM123_ENTRYP voutput_play_samples  )( void*  a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  char        voutput_buffer[BUFSIZE];
  FORMAT_INFO voutput_format;
  int         voutput_posmarker;
  BOOL        voutput_always_hungry;
  void        (PM123_ENTRYP voutput_event)(void* w, OUTEVENTTYPE event);
  void*       voutput_w;
  VDELEGATE   vd_output_command, vd_output_request_buffer, vd_output_commit_buffer;

 private:
  PROXYFUNCDEF ULONG PM123_ENTRY proxy_1_output_command       ( CL_OUTPUT_PROXY_1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info );
  PROXYFUNCDEF int   PM123_ENTRY proxy_1_output_request_buffer( CL_OUTPUT_PROXY_1* op, void* a, const FORMAT_INFO* format, char** buf, int posmarker );
  PROXYFUNCDEF void  PM123_ENTRY proxy_1_output_commit_buffer ( CL_OUTPUT_PROXY_1* op, void* a, int len );
 public:
  CL_OUTPUT_PROXY_1(CL_MODULE& mod) : CL_OUTPUT(mod) {}
  virtual BOOL load_plugin();
};

/* Assigns the addresses of the out7put plug-in procedures. */
BOOL CL_OUTPUT_PROXY_1::load_plugin()
{
  if ( !(query_param.type & PLUGIN_OUTPUT)
    || !load_function(&output_init,            "output_init")
    || !load_function(&output_uninit,          "output_uninit")
    || !load_function(&output_playing_samples, "output_playing_samples")
    || !load_function(&output_playing_pos,     "output_playing_pos")
    || !load_function(&output_playing_data,    "output_playing_data")
    || !load_function(&voutput_command,        "output_command")
    || !load_function(&voutput_play_samples,   "output_play_samples") )
    return FALSE;

  output_command        = (ULONG (PM123_ENTRYP)(void*, ULONG, OUTPUT_PARAMS2*))
                          mkvdelegate(&vd_output_command,        (V_FUNC)&proxy_1_output_command,        3, this);
  output_request_buffer = (int   (PM123_ENTRYP)(void*, const FORMAT_INFO*, char**, int))
                          mkvdelegate(&vd_output_request_buffer, (V_FUNC)&proxy_1_output_request_buffer, 5, this);
  output_commit_buffer  = (void  (PM123_ENTRYP)(void*, int))
                          mkvdelegate(&vd_output_commit_buffer,  (V_FUNC)&proxy_1_output_commit_buffer,  2, this);

  a = NULL;
  return TRUE;
}

/* virtualization of level 1 output plug-ins */
PROXYFUNCIMP(ULONG PM123_ENTRY, CL_OUTPUT_PROXY_1::)
proxy_1_output_command( CL_OUTPUT_PROXY_1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info )
{
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_output_command(%p {%s}, %p, %d, %p)\n",
    op, op->module_name, a, msg, info);
  #endif
    
  if (info != NULL)
  { int r;
    OUTPUT_PARAMS params;
    params.size            = sizeof params;
    params.buffersize      = BUFSIZE;
    params.boostclass      = DECODER_HIGH_PRIORITY_CLASS;
    params.normalclass     = DECODER_LOW_PRIORITY_CLASS;
    params.boostdelta      = DECODER_HIGH_PRIORITY_DELTA;
    params.normaldelta     = DECODER_LOW_PRIORITY_DELTA;
    params.nobuffermode    = FALSE;
    // copy corresponding fields
    params.formatinfo      = info->formatinfo;
    params.error_display   = info->error_display;
    params.info_display    = info->info_display;
    params.hwnd            = info->hwnd;
    params.volume          = info->volume;
    params.amplifier       = info->amplifier;
    params.pause           = info->pause;
    params.temp_playingpos = info->temp_playingpos;
    params.filename        = info->URI; // TODO: URI!
    // call plug-in
    r = (*op->voutput_command)(a, msg, &params);
    // save some values
    if (msg == OUTPUT_SETUP)
    { op->voutput_always_hungry = params.always_hungry;
      op->voutput_event = info->output_event;
      op->voutput_w = info->w;
    }
    return r;
  } else
    return (*op->voutput_command)(a, msg, NULL); // sometimes info is NULL
}

PROXYFUNCIMP(int PM123_ENTRY, CL_OUTPUT_PROXY_1)
proxy_1_output_request_buffer( CL_OUTPUT_PROXY_1* op, void* a, const FORMAT_INFO* format, char** buf, int posmarker )
{ 
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_output_request_buffer(%p, %p, {%i,%i,%i,%i,%x}, %p, %i)\n",
    op, a, format->size, format->samplerate, format->channels, format->bits, format->format,
    buf, posmarker);
  #endif
  
  if (buf == 0)
  { if (op->voutput_always_hungry)
      (*op->voutput_event)(op->voutput_w, OUTEVENT_END_OF_DATA);
    return 0;
  }

  *buf = op->voutput_buffer;
  op->voutput_format = *format;
  op->voutput_posmarker = posmarker;
  return BUFSIZE;
}

PROXYFUNCIMP(void PM123_ENTRY, CL_OUTPUT_PROXY_1)
proxy_1_output_commit_buffer( CL_OUTPUT_PROXY_1* op, void* a, int len )
{
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_output_commit_buffer(%p {%s}, %p, %i)\n",
    op, op->module_name, a, len);
  #endif
  (*op->voutput_play_samples)(a, &op->voutput_format, op->voutput_buffer, len, op->voutput_posmarker);
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
{
  #ifdef DEBUG
  fprintf(stderr, "CL_FILTER(%p{%s})::load_plugin", this, module_name);
  #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "CL_FILTER(%p{%s})::init_plugin\n", this, module_name);
  #endif
  return TRUE; // filters are not initialized unless they are used
}

BOOL CL_FILTER::uninit_plugin()
{
  #ifdef DEBUG
  fprintf(stderr, "CL_FILTER(%p{%s})::uninit_plugin\n", this, module_name);
  #endif
  (*filter_uninit)(f);
  f = NULL;
  return TRUE;
}

BOOL CL_FILTER::initialize(FILTER_PARAMS2* params)
{ 
  #ifdef DEBUG
  fprintf(stderr, "CL_FILTER(%p{%s})::initialize(%p)\n", this, module_name, params);
  #endif
  FILTER_PARAMS2 par = *params;
  if (is_initialized() || (*filter_init)(&f, params) != 0)
    return FALSE;
  // virtualize untouched functions
  if (par.output_command          == params->output_command)
    params->output_command         = (ULONG (PM123_ENTRYP)(void*, ULONG, OUTPUT_PARAMS2*))
                                     mkvreplace1(&vrstubs[0], (V_FUNC)par.output_command, par.a);
  if (par.output_playing_samples  == params->output_playing_samples)
    params->output_playing_samples = (ULONG (PM123_ENTRYP)(void*, FORMAT_INFO*, char*, int))
                                     mkvreplace1(&vrstubs[1], (V_FUNC)par.output_playing_samples, par.a);
  if (par.output_request_buffer   == params->output_request_buffer)
    params->output_request_buffer  = (int (PM123_ENTRYP)(void*, const FORMAT_INFO*, char**, int))
                                     mkvreplace1(&vrstubs[2], (V_FUNC)par.output_request_buffer, par.a);
  if (par.output_commit_buffer    == params->output_commit_buffer)
    params->output_commit_buffer   = (void (PM123_ENTRYP)(void*, int))
                                     mkvreplace1(&vrstubs[3], (V_FUNC)par.output_commit_buffer, par.a);
  if (par.output_playing_pos      == params->output_playing_pos)
    params->output_playing_pos     = (int (PM123_ENTRYP)(void*))
                                     mkvreplace1(&vrstubs[4], (V_FUNC)par.output_playing_pos, par.a);
  if (par.output_playing_data     == params->output_playing_data)
    params->output_playing_data    = (BOOL (PM123_ENTRYP)(void*))
                                     mkvreplace1(&vrstubs[5], (V_FUNC)par.output_playing_data, par.a);
  return TRUE;
}


// proxy for level 1 filters
class CL_FILTER_PROXY_1 : public CL_FILTER
{private:
  int   (PM123_ENTRYP vfilter_init         )( void** f, FILTER_PARAMS* params );
  BOOL  (PM123_ENTRYP vfilter_uninit       )( void*  f );
  int   (PM123_ENTRYP vfilter_play_samples )( void*  f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
  void*       vf;
  int   (PM123_ENTRYP output_request_buffer)( void*  a, const FORMAT_INFO* format, char **buf, int posmarker );
  void  (PM123_ENTRYP output_commit_buffer )( void*  a, int len );
  void*       a;
  FORMAT_INFO vformat;
  char*       vbuffer;
  int         vlen;
  int         vposmarker;
  char*       storebuffer;
  int         storelen;
  VDELEGATE   vd_filter_init;
  VREPLACE1   vr_filter_update;
  VREPLACE1   vr_filter_uninit;

 private:
  PROXYFUNCDEF ULONG PM123_ENTRY proxy_1_filter_init          ( CL_FILTER_PROXY_1* pp, void** f, FILTER_PARAMS2* params );
  PROXYFUNCDEF void  PM123_ENTRY proxy_1_filter_update        ( CL_FILTER_PROXY_1* pp, const FILTER_PARAMS2* params );
  PROXYFUNCDEF BOOL  PM123_ENTRY proxy_1_filter_uninit        ( void* f ); // empty stub
  PROXYFUNCDEF int   PM123_ENTRY proxy_1_filter_request_buffer( void* f, const FORMAT_INFO* format, char** buf, int posmarker );
  PROXYFUNCDEF void  PM123_ENTRY proxy_1_filter_commit_buffer ( void* f, int len );
  PROXYFUNCDEF int   PM123_ENTRY proxy_1_filter_play_samples  ( void* f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
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

  filter_init   = (ULONG (PM123_ENTRYP)(void**, FILTER_PARAMS2*))
                  mkvdelegate(&vd_filter_init, (V_FUNC)&proxy_1_filter_init, 2, this);
  filter_update = (void  (PM123_ENTRYP)(void* f, const FILTER_PARAMS2* params))
                  mkvreplace1(&vr_filter_update, (V_FUNC)&proxy_1_filter_update, this);
  // filter_uninit is initialized at the filter_init call to a non-no-op function
  // However, the returned pointer will stay the same.
  filter_uninit = (BOOL (PM123_ENTRYP)(void* f))
                  mkvreplace1(&vr_filter_uninit, (V_FUNC)&proxy_1_filter_uninit, NULL);

  f = NULL;
  enabled = TRUE;
  return TRUE;
}

PROXYFUNCIMP(ULONG PM123_ENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_init( CL_FILTER_PROXY_1* pp, void** f, FILTER_PARAMS2* params )
{ 
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_init(%p{%s}, %p, %p{a=%p})\n", pp, pp->module_name, f, params, params->a);
  #endif
  FILTER_PARAMS par;
  par.size                = sizeof par;
  par.output_play_samples = &PROXYFUNCREF(CL_FILTER_PROXY_1)proxy_1_filter_play_samples;
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
  // replace the unload function
  mkvreplace1(&pp->vr_filter_uninit, (V_FUNC)pp->vfilter_uninit, pp->vf);
  // now return some values
  *f = pp;
  params->output_request_buffer = &PROXYFUNCREF(CL_FILTER_PROXY_1)proxy_1_filter_request_buffer;
  params->output_commit_buffer  = &PROXYFUNCREF(CL_FILTER_PROXY_1)proxy_1_filter_commit_buffer;
  return 0;
}

PROXYFUNCIMP(void PM123_ENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_update( CL_FILTER_PROXY_1* pp, const FILTER_PARAMS2* params )
{ 
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_update(%p{%s}, %p)\n", pp, pp->module_name, params);
  #endif
  DosEnterCritSec();
  // replace function pointers
  pp->output_request_buffer = params->output_request_buffer;
  pp->output_commit_buffer  = params->output_commit_buffer;
  pp->a                     = params->a;
  DosExitCritSec();
}

PROXYFUNCIMP(BOOL PM123_ENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_uninit( void* )
{ return TRUE;
}

PROXYFUNCIMP(int PM123_ENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_request_buffer( void* a, const FORMAT_INFO* format, char** buf, int posmarker )
{ CL_FILTER_PROXY_1* pp = (CL_FILTER_PROXY_1*)a;
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_request_buffer(%p, %p{%d,%d,%d,%d}, %p, %d)\n",
    a, format, format->samplerate, format->channels, format->bits, format->format, buf, posmarker);
  #endif
  // we try to operate in place...
  pp->vlen = (*pp->output_request_buffer)(pp->a, format, buf, posmarker);
  if (pp->vlen > 0)
  { pp->vformat = *format;
    pp->vbuffer = *buf;
    pp->vposmarker = posmarker;
  }
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_request_buffer: %d\n", pp->vlen);
  #endif
  return pp->vlen;
}

PROXYFUNCIMP(void PM123_ENTRY, CL_FILTER_PROXY_1)
proxy_1_filter_commit_buffer( void* a, int len )
{ CL_FILTER_PROXY_1* pp = (CL_FILTER_PROXY_1*)a;
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_commit_buffer(%p, %d)\n", a, len);
  #endif
  if (len == 0 || pp->vlen == 0)
  { (*pp->output_commit_buffer)(pp->a, len);
    return;
  }
  pp->storelen = 0;
  char* buf = pp->storebuffer = pp->vbuffer;
  while (len > BUFSIZE) // do not pass buffers larger than BUFSIZE to avoid internal buffer overruns in the plugin
  { 
    #ifdef DEBUG
    fprintf(stderr, "proxy_1_filter_commit_buffer: fragmentation: %p %d\n", buf, len);
    #endif
    (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, buf, BUFSIZE, pp->vposmarker);
    buf += BUFSIZE;
    len -= BUFSIZE;
    // TODO: increment vposmarker 
  }
  // TODO: maybe we should not pass buffers less than BUFSIZE unless we have a flush condition to avoid fragmentation
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_commit_buffer: last fragment: %p->(%p, %p, %p,%d, %d)\n",
    pp->vfilter_play_samples, pp->vf, &pp->vformat, buf, len, pp->vposmarker);
  #endif
  (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, buf, len, pp->vposmarker);
  // commit destination
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_commit_buffer: before commit: %d\n", pp->storelen);
  #endif
  (*pp->output_commit_buffer)(pp->a, pp->storelen);
}

PROXYFUNCIMP(int PM123_ENTRY, CL_FILTER_PROXY_1::)
proxy_1_filter_play_samples( void* f, const FORMAT_INFO* format, const char *buf, int len, int posmarker )
{ CL_FILTER_PROXY_1* pp = (CL_FILTER_PROXY_1*)f;
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_play_samples(%p, %p{%d,%d,%d,%d}, %p, %d)\n",
    f, format, format->samplerate, format->channels, format->bits, format->format, buf, posmarker);
  #endif
  // TODO: format change!
  while (len > pp->vlen - pp->storelen)
  { 
    #ifdef DEBUG
    fprintf(stderr, "proxy_1_filter_play_samples: oversize: %p %d %d %d\n", buf, len, pp->vlen, pp->storelen);
    #endif
    // This cannot be in place, since the source buffer wasn't large enough to go beyond vlen
    memcpy(pp->storebuffer, buf, pp->vlen - pp->storelen);
    // commit destination
    (*pp->output_commit_buffer)(pp->a, pp->vlen);
    buf += pp->vlen - pp->storelen;
    len -= pp->vlen - pp->storelen; 
    // TODO: increment posmarker
    // request new buffer
    pp->vlen = (*pp->output_request_buffer)(pp->a, &pp->vformat, &pp->vbuffer, pp->vposmarker);
    if (pp->vlen <= 0)
      return 0; // error
    pp->storebuffer = pp->vbuffer;
    pp->storelen = 0;
  }
  // TODO: posmarker ...
  #ifdef DEBUG
  fprintf(stderr, "proxy_1_filter_play_samples: store: %p %d %d %d\n", buf, len, pp->vlen, pp->storelen);
  #endif
  if (buf != pp->storebuffer) // only if not in-place
    memcpy(pp->storebuffer, buf, len);
  pp->storebuffer += len;
  pp->storelen += len;
  return len;
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

BOOL CL_VISUAL::initialize(VISPLUGININIT* visinit)
{ 
  #ifdef DEBUG
  fprintf(stderr, "CL_VISUAL(%p{%s})::initialize(%p{%d,%d,%d,%d, %x, ..., %d, %s})\n",
    this, module_name, visinit, visinit->x, visinit->y, visinit->cx, visinit->cy, visinit->hwnd, visinit->id, visinit->param);
  #endif
  hwnd = (*plugin_init)(visinit);
  return hwnd != NULLHANDLE;
}

void CL_VISUAL::set_properties(const VISUAL_PROPERTIES* data)
{ static const VISUAL_PROPERTIES def_visuals = {0,0,0,0, FALSE, ""};
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
{
  #ifdef DEBUG
  fprintf(stderr, "CL_PLUGIN_BASE_LIST(%p{%p,%d,%d})::append(%p{%p,%s})\n",
    this, list, num, size, plugin, plugin->module, plugin->module_name);
  #endif
  if (num >= size)
  { int l = size + 8;
    // Note: The Debug memory management of IMB C++ will claim the objects allocated here are not freed
    // until the program ends. This is not true. The clean-up function of the debug memory manager is only called
    // before the destructors of the static objects.
    CL_PLUGIN_BASE** np = (CL_PLUGIN_BASE**)realloc(list, l * sizeof *list);
    if (np == NULL)
    {
      #ifdef DEBUG
      fprintf(stderr, "CL_PLUGIN_BASE_LIST::append: internal memory allocation error\n");
      #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "CL_PLUGIN_BASE_LIST(%p{%p,%d,%d})::detach(%i)\n", this, list, num, size, i);
  #endif
  if (i > num && i <= 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "CL_PLUGIN_BASE_LIST::detach: index out of range\n");
    #endif
    return NULL;
  }
  
  CL_PLUGIN_BASE* r = list[i];
  memmove(list + i, list + i +1, (--num - i) * sizeof *list);
  #ifdef DEBUG
  fprintf(stderr, "CL_PLUGIN_BASE_LIST::detach: %p\n", r);
  #endif
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

/****************************************************************************
*
* MODULE_LIST collection
*
****************************************************************************/

CL_MODULE* CL_MODULE_LIST::detach_request(int i)
{
  #ifdef DEBUG
  fprintf(stderr, "CL_MODULE_LIST(%p)::detach_request(%i)\n", this, i);
  if (i > count() && i <= 0)
  { fprintf(stderr, "CL_MODULE_LIST::detach_request: index out of range\n");
    return NULL;
  }
  #endif
  
  return (*this)[i].unload_request() ? detach(i) : NULL;
}


/****************************************************************************
*
* PLUGIN_LIST collection
*
****************************************************************************/                     

CL_PLUGIN* CL_PLUGIN_LIST::detach(int i)
{ 
  #ifdef DEBUG
  fprintf(stderr, "CL_PLUGIN_LIST(%p)::detach(%i)\n", this, i);
  #endif
  if (i > count() && i <= 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "CL_PLUGIN_LIST::detach: index out of range\n");
    #endif
    return NULL;
  }
  if ((*this)[i].is_initialized() && !(*this)[i].uninit_plugin())
  {
    #ifdef DEBUG
    fprintf(stderr, "CL_PLUGIN_LIST::detach: plugin %s failed to uninitialize.\n", (*this)[i].module_name);
    #endif
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
    #ifdef DEBUG
    fprintf(stderr, "CL_PLUGIN_LIST::clear: %p.\n", pp);
    #endif
    delete pp;
    #ifdef DEBUG
    fprintf(stderr, "CL_PLUGIN_LIST::clear: OK.\n");
    #endif
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



