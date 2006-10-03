/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <format.h>
#include <fileutil.h>
#include "plugman_base.h"
#include "pm123.h"

//#define DEBUG 1
#include <debuglog.h>

/* thread priorities for decoder thread */
#define  DECODER_HIGH_PRIORITY_CLASS PRTYC_FOREGROUNDSERVER // sorry, we should not lockup the system with time-critical priority
#define  DECODER_HIGH_PRIORITY_DELTA 20
#define  DECODER_LOW_PRIORITY_CLASS  PRTYC_REGULAR
#define  DECODER_LOW_PRIORITY_DELTA  1


/****************************************************************************
*
* global lists
*
****************************************************************************/

CL_MODULE_LIST         plugins;  // all plug-ins
static CL_PLUGIN_LIST1 decoders; // only decoders
static CL_PLUGIN_LIST1 outputs;  // only outputs
static CL_PLUGIN_LIST  filters;  // only filters
static CL_PLUGIN_LIST  visuals;  // only visuals


/****************************************************************************
*
* virtual output interface, including filter plug-ins
* this class logically connects the decoder and the output interface
*
****************************************************************************/

static class CL_GLUE
{private:
         BOOL              initialized;       // whether the following vars are true
         OUTPUT_PROCS      procs;             // entry points of the filter chain
         OUTPUT_PARAMS2    params;            // parameters for output_command
         DECODER_PARAMS2   dparams;           // parameters for decoder_command
         char              metadata_buffer[128]; // Loaded in curtun on decoder's demand WM_METADATA.

 private:
         void              virtualize         ( int i );
         ULONG             init();
         void              uninit();
         int               dec_set_active     ( int number )
                                              { return decoders.set_active(number); }
         int               dec_set_active     ( const char* name );
         ULONG             dec_command        ( ULONG msg );
         ULONG             out_command        ( ULONG msg )
                                              { return (*procs.output_command)( procs.a, msg, &params ); }
 public:
                           CL_GLUE();
  const  OUTPUT_PROCS&     get_procs() const  { return procs; }
  
  // decoder control interface
  friend ULONG             dec_play           ( const char* url, const char* decoder_name );
  friend ULONG             dec_stop           ( void );
  friend ULONG             dec_fast           ( DECODER_FAST_MODE mode );
  friend ULONG             dec_jump           ( int pos );
  friend ULONG             dec_eq             ( const float* bandgain );
  friend ULONG             dec_save           ( const char* file );
  // output control interface
  friend ULONG             out_setup          ( const FORMAT_INFO2* formatinfo, const char* URI );
  friend ULONG             out_close          ();
  friend void              out_set_volume     ( double volume );
  friend ULONG             out_pause          ( BOOL pause );
  friend void              out_trashbuffers   ( int temp_playingpos );
  friend BOOL              out_flush          ( void );
  // 4 visual interface
  friend ULONG PM123_ENTRY out_playing_pos    ( void );
  friend BOOL  PM123_ENTRY out_playing_data   ( void );
  friend ULONG PM123_ENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len );
 private: // glue
  // 4 callback interface
  friend void  PM123_ENTRY dec_event          ( void* w, OUTEVENTTYPE event ); 
} voutput;

CL_GLUE::CL_GLUE()
{ memset(&params, 0, sizeof params);
}

void CL_GLUE::virtualize(int i)
{ DEBUGLOG(("CL_GLUE(%p)::virtualize(%d)\n", this, i));

  if (i < 0)
    return;
  CL_FILTER& fil = (CL_FILTER&)filters[i];
  if (!fil.get_enabled())
  { virtualize(i-1);
    return;
  }
  // virtualize procedures
  FILTER_PARAMS2 par;
  par.size                   = sizeof par;
  par.output_command         = procs.output_command;
  par.output_playing_samples = procs.output_playing_samples;
  par.output_request_buffer  = procs.output_request_buffer;
  par.output_commit_buffer   = procs.output_commit_buffer;
  par.output_playing_pos     = procs.output_playing_pos;
  par.output_playing_data    = procs.output_playing_data;
  par.a                      = procs.a;
  par.output_event           = params.output_event;
  par.w                      = params.w;
  par.error_display          = &keep_last_error;
  par.info_display           = &display_info;
  if (!fil.initialize(&par))
  { char msg[500];
    sprintf(msg, "The filter plug-in %s failed to initialize.", fil.module_name);
    display_info(msg);
    virtualize(i-1);
    return;
  }
  // store new entry points
  procs.output_command         = par.output_command;
  procs.output_playing_samples = par.output_playing_samples;
  procs.output_request_buffer  = par.output_request_buffer;
  procs.output_commit_buffer   = par.output_commit_buffer;
  procs.output_playing_pos     = par.output_playing_pos;
  procs.output_playing_data    = par.output_playing_data;
  procs.a                      = fil.get_procs().f;
  void (PM123_ENTRYP last_output_event)(void* w, OUTEVENTTYPE event) = params.output_event;
  // next filter
  virtualize(i-1);
  // store new callback if virtualized by the plug-in.
  BOOL vcallback = par.output_event != last_output_event;
  last_output_event     = par.output_event; // swap...
  par.output_event      = params.output_event;
  par.w                 = params.w;
  if (vcallback)
  { // set params for next instance.
    params.output_event = last_output_event;
    params.w            = fil.get_procs().f;
    DEBUGLOG(("CL_GLUE::virtualize: callback virtualized: %p %p\n", params.output_event, params.w));
  }
  if (par.output_event != last_output_event)
  { // now update the decoder event
    (*fil.get_procs().filter_update)(fil.get_procs().f, &par);
    DEBUGLOG(("CL_GLUE::virtualize: callback update: %p %p\n", par.output_event, par.w));
  }
}

// setup filter chain
ULONG CL_GLUE::init()
{ DEBUGLOG(("CL_GLUE(%p)::init\n", this));

  params.size          = sizeof params;
  params.error_display = &keep_last_error;
  params.info_display  = &display_info;
  params.hwnd          = amp_player_window();
  // setup callback handlers
  params.output_event  = &dec_event;
  params.w             = this; // not really required
  
  memset(&dparams, 0, sizeof dparams);
  dparams.size            = sizeof dparams;
  dparams.error_display   = &keep_last_error;
  dparams.info_display    = &display_info;
  dparams.hwnd            = amp_player_window();
  dparams.metadata_buffer = voutput.metadata_buffer;
  dparams.metadata_size   = sizeof(voutput.metadata_buffer);
  dparams.save_filename   = NULL;

  CL_OUTPUT* op = (CL_OUTPUT*)outputs.current();
  if ( op == NULL )
  { amp_player_error( "There is no active output plug-in." );
    return (ULONG)-1;  // ??
  }

  // initially only the output plugin
  procs = op->get_procs();
  // setup filters
  virtualize(filters.count()-1); 
  // setup output
  ULONG rc = out_command( OUTPUT_SETUP );
  if (rc == 0)
    initialized = TRUE;
  return rc;
}

void CL_GLUE::uninit()
{ DEBUGLOG(("CL_GLUE(%p)::uninit\n", this));

  initialized = FALSE;
  // uninitialize filter chain
  for (int i = 0; i < filters.count(); ++i)
    if (filters[i].is_initialized())
      filters[i].uninit_plugin();
}

/* Returns -1 if a error occured,
   returns -2 if can't find nothing,
   returns 0  if succesful. */
int CL_GLUE::dec_set_active( const char* name )
{
  int i;
  if( name == NULL ) {
    return dec_set_active( -1 );
  }

  for( i = 0; i < decoders.count(); i++ ) {
    char filename[_MAX_FNAME];
    sfnameext( filename, decoders[i].module_name, sizeof( filename ));
    if( stricmp( filename, name ) == 0 ) {
      return dec_set_active( i );
    }
  }
  return -2;
}

ULONG CL_GLUE::dec_command( ULONG msg )
{ DEBUGLOG(("dec_command(%d)\n", msg));
  CL_PLUGIN* pp = decoders.current();
  if ( pp == NULL )
    return 3; // no decoder active

  const DECODER_PROCS& procs = ((CL_DECODER*)pp)->get_procs();
  return (*procs.decoder_command)( procs.w, msg, &dparams );
}

/* invoke decoder to play an URL */
ULONG dec_play( const char* url, const char* decoder_name )
{
  DEBUGLOG(("dec_play(%s, %s)\n", url, decoder_name));
  ULONG rc = voutput.dec_set_active( decoder_name );
  if ( rc != 0 )
    return rc;

  voutput.dparams.URL                   = url;
  voutput.dparams.posmarker             = 0; // TODO: currently...
  voutput.dparams.output_request_buffer = voutput.procs.output_request_buffer;
  voutput.dparams.output_commit_buffer  = voutput.procs.output_commit_buffer;
  voutput.dparams.a                     = voutput.procs.a;
  voutput.dparams.buffersize            = cfg.bufsize*1024;
  voutput.dparams.bufferwait            = cfg.bufwait;
  voutput.dparams.proxyurl              = cfg.proxy;
  voutput.dparams.httpauth              = cfg.auth;

  rc = voutput.dec_command( DECODER_SETUP );
  if ( rc != 0 )
    return rc;

  voutput.dec_command( DECODER_SAVEDATA );

  return voutput.dec_command( DECODER_PLAY );
}

/* stop the current decoder immediately */
ULONG dec_stop( void )
{ ULONG rc = voutput.dec_command( DECODER_STOP );
  if (rc == 0)
    voutput.dec_set_active( -1 );
  return rc;
}

/* set fast forward/rewind mode */
ULONG dec_fast( DECODER_FAST_MODE mode )
{ switch (mode)
  {case DECODER_FAST_FORWARD:
    if (voutput.dparams.rew)
    { voutput.dparams.rew = FALSE;
      voutput.dec_command( DECODER_REW );
    }
    if (!voutput.dparams.ffwd)
    { voutput.dparams.ffwd = TRUE;
      return voutput.dec_command( DECODER_FFWD );
    }
    break;

   case DECODER_FAST_REWIND:
    if (voutput.dparams.ffwd)
    { voutput.dparams.ffwd = FALSE;
      voutput.dec_command( DECODER_FFWD );
    }
    if (!voutput.dparams.rew)
    { voutput.dparams.rew = TRUE;
      return voutput.dec_command( DECODER_REW );
    }
    break;
    
   default:
    if (voutput.dparams.rew)
    { voutput.dparams.rew = FALSE;
      return voutput.dec_command( DECODER_REW );
    } else
    if (voutput.dparams.ffwd)
    { voutput.dparams.ffwd = FALSE;
      return voutput.dec_command( DECODER_FFWD );
    }
    break;
  }
  return 0;
}

/* jump to absolute position */
ULONG dec_jump( int location )
{ voutput.dparams.jumpto = location;
  return voutput.dec_command( DECODER_JUMPTO );
}

/* set equalizer parameters */
ULONG dec_eq( const float* bandgain )
{ voutput.dparams.bandgain  = bandgain;
  voutput.dparams.equalizer = bandgain != NULL;
  ULONG status = dec_status();
  return status == DECODER_PLAYING || status == DECODER_STARTING
   ? voutput.dec_command( DECODER_EQ )
   : 0;
}

/* set savefilename to save the raw stream data */
ULONG dec_save( const char* file )
{ voutput.dparams.save_filename = file;
  ULONG status = dec_status();
  return status == DECODER_PLAYING || status == DECODER_STARTING
   ? voutput.dec_command( DECODER_SAVEDATA )
   : 0;
}


/* setup new output stage or change the properties of the current one */
ULONG out_setup( const FORMAT_INFO2* formatinfo, const char* URI )
{ voutput.params.formatinfo = *formatinfo;
  if (!voutput.initialized)
  { ULONG rc = voutput.init(); // here we initialte the setup of the filter chain
    if (rc != 0)
      return rc;
  }
  voutput.params.URI = URI;
  return voutput.out_command( OUTPUT_OPEN );
}

/* closes output device and uninitializes all filter and output plugins */
ULONG out_close()
{ if (!voutput.initialized)
    return (ULONG)-1;
  ULONG rc = voutput.out_command( OUTPUT_CLOSE );
  voutput.uninit(); // Hmm, is it a good advise to do this in case of an error?
  return rc;
}

/* adjust output volume */
void out_set_volume( double volume )
{ if (!voutput.initialized)
    return; // can't help
  voutput.params.volume = volume;
  voutput.params.amplifier = 1.0;
  voutput.out_command( OUTPUT_VOLUME );
}

ULONG out_pause( BOOL pause )
{ if (!voutput.initialized)
    return (ULONG)-1; // error
  voutput.params.pause = pause;
  return voutput.out_command( OUTPUT_PAUSE );
}

void out_trashbuffers( int temp_playingpos )
{ if (!voutput.initialized)
    return; // can't help
  voutput.params.temp_playingpos = temp_playingpos;
  voutput.out_command( OUTPUT_TRASH_BUFFERS );
}

BOOL out_flush()
{ if (!voutput.initialized)
    return FALSE;
  (*voutput.procs.output_request_buffer)( voutput.procs.a, NULL, NULL );
  return TRUE;
}

/* Returns 0 = success otherwize MMOS/2 error. */
ULONG PM123_ENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len )
{ if (!voutput.initialized)
    return (ULONG)-1; // N/A
  return (*voutput.procs.output_playing_samples)( voutput.procs.a, info, buf, len );
}

/* Returns time in ms. */
ULONG PM123_ENTRY out_playing_pos( void )
{ if (!voutput.initialized)
    return 0; // ??
  return (*voutput.procs.output_playing_pos)( voutput.procs.a );
}

/* if the output is playing. */
BOOL PM123_ENTRY out_playing_data( void )
{ if (!voutput.initialized)
    return FALSE;
  return (*voutput.procs.output_playing_data)( voutput.procs.a );
}

/* proxy, because the decoder is not interested in OUTEVENT_END_OF_DATA */
void PM123_ENTRY dec_event( void* w, OUTEVENTTYPE event )
{ DEBUGLOG(("plugman:dec_event(%p, %d)\n", w, event));

  switch (event)
  {case OUTEVENT_END_OF_DATA:
    WinPostMsg(amp_player_window(),WM_PLAYERROR,0,0);
    break;
   case OUTEVENT_PLAY_ERROR:
    WinPostMsg(amp_player_window(),WM_OUTPUT_OUTOFDATA,0,0);
    break;
   default:
    CL_DECODER* dp = (CL_DECODER*)decoders.current();
    if (dp != NULL)
      dp->get_procs().decoder_event(dp->get_procs().w, event);
  }
}


/****************************************************************************
*
* plugin list modification functions
*
****************************************************************************/

/* returns the index of a new or an existing module in the modules list
 * or -1 if the module cannot be loaded.
 */
static int get_module(const char* name)
{ DEBUGLOG(("plugman:get_module(%s)\n", name));

  int p = plugins.find(name);
  if (p == -1)
  { p = plugins.count();
    CL_MODULE* pp = new CL_MODULE(name);
    if (!pp->load() || !plugins.append(pp))
    { delete pp;
      return -1;
    }
  }
    
  DEBUGLOG(("plugman:get_module: %d\n", p));
  return p;
}

static CL_PLUGIN* instantiate(CL_MODULE& mod, CL_PLUGIN* (*factory)(CL_MODULE& mod), CL_PLUGIN_LIST& list, BOOL enabled)
{ DEBUGLOG(("plugman:instantiate(%p{%s}, %p, %p, %i)\n", &mod, mod.module_name, factory, &list, enabled)); 

  if (list.find(mod) != -1)
  { amp_player_error( "Tried to load the plug-in %s twice.\n", mod.module_name );
    return NULL;
  }
  CL_PLUGIN* pp = (*factory)(mod);
  if (!pp->load_plugin() || !list.append(pp))
  { delete pp;
    DEBUGLOG(("plugman:instantiate: failed.\n"));
    return NULL;
  } else
  { pp->set_enabled(enabled);
    return pp;
  }
}

static int add_plugin_core(const char* name, const VISUAL_PROPERTIES* data, int mask, BOOL enabled = TRUE)
{
  #ifdef DEBUG
  if (data)
    DEBUGLOG(("plugman:add_plugin_core(%s, %p{%d,%d,%d,%d, %i, %s}, %x, %i)\n",
      name, data, data->x, data->y, data->cx, data->cy, data->skin, data->param, mask, enabled));
   else 
    DEBUGLOG(("plugman:add_plugin_core(%s, %p, %x, %i)\n", name, data, mask, enabled)); 
  #endif
  // make absolute path
  char module_name[_MAX_PATH];
  if (strchr(name, ':') == NULL && name[0] != '\\' && name[0] != '/')
  { // relative path
    strlcpy(module_name, startpath, sizeof module_name);
    strlcat(module_name, name, sizeof module_name);
  } else
    strlcpy(module_name, name, sizeof module_name);
  // replace '/'
  { char* cp = module_name;
    while (TRUE)
    { cp = strchr(cp, '/');
      if (cp == NULL)
        break;
      *cp++ = '\\';
  } }
  // load module
  int p = get_module(module_name);
  if (p == -1)
    return FALSE;
  // load as plugin
  int result = 0;
  CL_MODULE& module = plugins[p];
  module.attach(NULL); // keep a reference alive as long as we use the module in this code
  mask &= module.query_param.type;
  // decoder
  if ((mask & PLUGIN_DECODER) && instantiate(module, &CL_DECODER::factory, decoders, enabled))
    result |= PLUGIN_DECODER;
  // output
  if ((mask & PLUGIN_OUTPUT) && instantiate(module, &CL_OUTPUT::factory, outputs, enabled)) 
  { if (outputs.current() == NULL)
      outputs.set_active( outputs.count() -1 );
    result |= PLUGIN_OUTPUT;
  }
  // filter
  if ((mask & PLUGIN_FILTER) && instantiate(module, &CL_FILTER::factory, filters, enabled))
    result |= PLUGIN_FILTER;
  // visual
  if (mask & PLUGIN_VISUAL)
  { int q = visuals.find(module);
    if (q != -1)
    { // existing plugin. update some values
      ((CL_VISUAL&)visuals[q]).set_properties(data);
    } else if (instantiate(module, &CL_VISUAL::factory, visuals, enabled))
    { ((CL_VISUAL&)visuals[visuals.count()-1]).set_properties(data);
      result |= PLUGIN_VISUAL;
    }
  }
  // cleanup?
  if (module.detach(NULL))
    delete plugins.detach(p);

  DEBUGLOG(("plugman:add_plugin_core: %x\n", result));
  return result;
}

/* Unloads and removes the specified decoder plug-in from the list of loaded. */
BOOL remove_decoder_plugin( int i )
{ return decoders.remove(i);
}

/* Unloads and removes the specified output plug-in from the list of loaded. */
BOOL remove_output_plugin( int i )
{ return outputs.remove(i);
}

/* Unloads and removes the specified filter plug-in from the list of loaded. */
BOOL remove_filter_plugin( int i )
{ return filters.remove(i);
}

/* Unloads and removes the specified visual plug-in from the list of loaded. */
BOOL remove_visual_plugin( int i )
{ return visuals.remove(i);
}

void remove_visual_plugins( BOOL skin )
{ DEBUGLOG(("plugman:remove_visual_plugins(%i)\n", skin));

  int i = visuals.count();
  while( i-- )
    if( ((CL_VISUAL&)visuals[i]).get_properties().skin == skin )
      visuals.remove( i );
}

/* Adds a default decoder plug-ins to the list of loaded. */
void load_default_decoders( void )
{ DEBUGLOG(("load_default_decoders()\n")); 

  decoders.clear();
  fprintf(stderr, "ccc");

  add_plugin_core("mpg123.dll",   NULL, PLUGIN_DECODER);
  add_plugin_core("wavplay.dll",  NULL, PLUGIN_DECODER);
  add_plugin_core("cddaplay.dll", NULL, PLUGIN_DECODER);
  add_plugin_core("os2rec.dll",   NULL, PLUGIN_DECODER);
}

/* Adds a default output plug-ins to the list of loaded. */
void load_default_outputs( void )
{ DEBUGLOG(("load_default_outputs()\n")); 

  outputs.clear();

  add_plugin_core("os2audio.dll", NULL, PLUGIN_OUTPUT);
  add_plugin_core("wavout.dll",   NULL, PLUGIN_OUTPUT);

  // outputs.set_active(0); implicit at add_plugin
}

/* Adds a default filter plug-ins to the list of loaded. */
void load_default_filters( void )
{ DEBUGLOG(("load_default_filters()\n")); 

  decoders.set_active( -1 ); // why ever
  filters.clear();

  add_plugin_core("realeq.dll",   NULL, PLUGIN_FILTER);
  add_plugin_core("logvolum.dll", NULL, PLUGIN_FILTER, FALSE);
}

/* Adds a default visual plug-ins to the list of loaded. */
void load_default_visuals( void )
{ DEBUGLOG(("load_default_visuals()\n")); 

  remove_visual_plugins( FALSE );
}

static BOOL load_plugin( BUFSTREAM* b, int mask, BOOL withenabled )
{ DEBUGLOG(("plugman:load_plugin(%p, %x, %i)\n", b, mask, withenabled)); 

  int i, size, count;
  BOOL enabled = TRUE;
  char module_name[_MAX_PATH];

  if( read_bufstream( b, &count, sizeof(int)) != sizeof(int))
    return FALSE;

  for( i = 0; i < count; i++ )
  { if ( read_bufstream( b, &size, sizeof(int)) != sizeof(int)
      || size >= sizeof module_name
      || read_bufstream( b, module_name, size ) != size
      || (withenabled && read_bufstream( b, &enabled, sizeof(BOOL)) != sizeof(BOOL)) )
      return FALSE;
    module_name[size] = 0;
    
    if (!add_plugin_core(module_name, NULL, mask, enabled))
      return FALSE;
  }
  return TRUE;
}

/* Restores the decoders list to the state was in when
   save_decoders was last called. */
BOOL load_decoders( BUFSTREAM* b )
{ DEBUGLOG(("load_decoders(%p)\n", b)); 

  decoders.set_active( -1 );
  decoders.clear();

  return load_plugin(b, PLUGIN_DECODER, true);
}

/* Restores the outputs list to the state was in when
   save_outputs was last called. */
BOOL load_outputs( BUFSTREAM *b )
{ DEBUGLOG(("load_outputs(%p)\n", b)); 

  int active; 

  outputs.clear();

  if (!load_plugin(b, PLUGIN_OUTPUT, false))
    return FALSE;

  if( read_bufstream( b, &active, sizeof(int)) == sizeof(int))
    outputs.set_active( active );
  /* else  implicitly done
    outputs.set_active( 0 ); */
  return TRUE;
}

/* Restores the filters list to the state was in when
   save_filters was last called. */
BOOL load_filters( BUFSTREAM *b )
{ DEBUGLOG(("load_filters(%p)\n", b)); 

  filters.clear();

  return load_plugin(b, PLUGIN_FILTER, true);
}

/* Restores the visuals list to the state was in when
   save_visuals was last called. */
BOOL load_visuals( BUFSTREAM *b )
{ DEBUGLOG(("load_visuals(%p)\n", b)); 

  remove_visual_plugins( FALSE );

  return load_plugin(b, PLUGIN_VISUAL, true); 
}

static BOOL save_plugin( BUFSTREAM *b, const CL_PLUGIN& src, BOOL withenabled )
{ DEBUGLOG(("plugman:save_plugin(%p, %p{%s}, %i)\n", b, &src, src.module_name, withenabled)); 

  int size = strlen( src.module_name );
  int enabled = src.get_enabled();

  if ( write_bufstream( b, &size, sizeof(int)) != sizeof(int)
    || write_bufstream( b, &src.module_name, size ) != size
    || (withenabled && write_bufstream( b, &enabled, sizeof(int)) != sizeof(int)) )
    return FALSE;
  return TRUE;
}

/* Saves the current list of decoders. */
BOOL save_decoders( BUFSTREAM *b )
{ DEBUGLOG(("save_decoders(%p)\n", b));

  int i = decoders.count();
  if ( write_bufstream( b, &i, sizeof(int)) != sizeof(int))
    return FALSE;

  for ( i = 0; i < decoders.count(); i++ )
    if (!save_plugin(b, decoders[i], true))
      return FALSE;

  return TRUE;
}

/* Saves the current list of outputs plug-ins. */
BOOL save_outputs( BUFSTREAM *b )
{ DEBUGLOG(("save_outputs(%p)\n", b));

  int i = outputs.count();
  if (write_bufstream( b, &i, sizeof(int)) != sizeof(int))
    return FALSE;

  for (i = 0; i < outputs.count(); i++ )
    if (!save_plugin(b, outputs[i], false))
      return FALSE;

  i = outputs.get_active();
  if (write_bufstream( b, &i, sizeof(int)) != sizeof(int))
    return FALSE;

  return TRUE;
}

/* Saves the current list of filters plug-ins. */
BOOL save_filters( BUFSTREAM *b )
{ DEBUGLOG(("save_filters(%p)\n", b));

  int i = filters.count();
  if (write_bufstream(b, &i, sizeof(int)) != sizeof(int))
    return FALSE;

  for (i = 0; i < filters.count(); i++ )
    if (!save_plugin(b, filters[i], true))
      return FALSE;

  return TRUE;
}

/* Saves the current list of visuals plug-ins. */
BOOL save_visuals( BUFSTREAM *b )
{ DEBUGLOG(("save_visuals(%p)\n", b));

  int i;
  int count = 0;

  for (i = 0; i < visuals.count(); i++ )
    count += !((CL_VISUAL&)visuals[i]).get_properties().skin;
  if (write_bufstream(b, &count, sizeof(int)) != sizeof(int))
    return FALSE;

  for (i = 0; i < visuals.count(); i++ )
    if (!((CL_VISUAL&)visuals[i]).get_properties().skin && save_plugin(b, visuals[i], true))
      return FALSE;

  return TRUE;
}

/* Loads and adds the specified plug-in to the appropriate list of loaded.
   Returns the types found or 0. */
ULONG add_plugin( const char* module_name, const VISUAL_PROPERTIES* data )
{ return add_plugin_core(module_name, data, PLUGIN_VISUAL|PLUGIN_FILTER|PLUGIN_DECODER|PLUGIN_OUTPUT);
}

/* enumerate decoder plug-ins
 * list - store pointer to plugin index
 * returns number of entries */
int enum_decoder_plugins(PLUGIN_BASE*const** list)
{ if (list != NULL)
    *list = (PLUGIN_BASE*const*)decoders.get_list();
  return decoders.count();
}

/* enumerate output plug-ins
 * list - store pointer to plugin index
 * returns number of entries */
int enum_output_plugins(PLUGIN_BASE*const** list)
{ if (list != NULL)
    *list = (PLUGIN_BASE*const*)outputs.get_list();
  return outputs.count();
}

/* enumerate filter plug-ins
 * list - store pointer to plugin index
 * returns number of entries */
int enum_filter_plugins(PLUGIN_BASE*const** list)
{ if (list != NULL)
    *list = (PLUGIN_BASE*const*)filters.get_list();
  return filters.count();
}

/* enumerate visual plug-ins
 * list - store pointer to plugin index
 * returns number of entries */
int enum_visual_plugins(PLUGIN_BASE*const** list)
{ if (list != NULL)
    *list = (PLUGIN_BASE*const*)visuals.get_list();
  return visuals.count();
}

BOOL get_plugin_enabled(const PLUGIN_BASE* plugin)
{ return ((const CL_PLUGIN*)plugin)->get_enabled();
}

void set_plugin_enabled(PLUGIN_BASE* plugin, BOOL enabled)
{ ((CL_PLUGIN*)plugin)->set_enabled(enabled);
}

void configure_plugin(PLUGIN_BASE* plugin, HWND hwnd)
{ ((CL_PLUGIN*)plugin)->get_module().config(hwnd);
}


BOOL dec_is_active( int number )
{ return number >= 0 && number < decoders.count() && &decoders[number] == decoders.current();
}


static BOOL is_file_supported(const char* const* support, const char* url)
{ if (!support)
    return FALSE;
  while (*support)
  { if (wildcardfit(*support, url))
      return TRUE;
    ++support;
  }
  return FALSE;
}

/* Returns the decoder NAME that can play this file and returns 0
   if not returns error 200 = nothing can play that. */
ULONG PM123_ENTRY
dec_fileinfo( const char* filename, DECODER_INFO2* info, char* name )
{
  BOOL* checked = (BOOL*)alloca( sizeof( BOOL ) * decoders.count() );
  int   i;
  const CL_DECODER* dp;

  memset( checked, 0, sizeof( BOOL ) * decoders.count() );
  
  // Prepend file://
  if (is_file(filename) && !is_url(filename))
  { char* fname = (char*)alloca( strlen(filename) +8 );
    strcpy(fname, "file://");
    strcpy(fname+7, filename);
    filename = fname;
  }
  
  if (is_track(filename))
  { // check decoders that claim to support tracks
    for( i = 0; i < decoders.count(); i++ )
    { dp = &(const CL_DECODER&)decoders[i];
      if (!dp->get_enabled() || !(dp->get_procs().type & DECODER_TRACK) )
        continue;
      checked[i] = TRUE;
      if (dp->get_procs().decoder_fileinfo(filename, info) == 0)
        goto ok;
    }
  } else
  { // First checks decoders supporting the specified type of files.
    for (i = 0; i < decoders.count(); i++)
    { dp = &(const CL_DECODER&)decoders[i];
      if (!dp->get_enabled() || !is_file_supported(dp->get_procs().support, filename))
        continue;
      checked[i] = TRUE;
      if (dp->get_procs().decoder_fileinfo(filename, info) == 0)
        goto ok;
    }
  }

  // Next checks the rest decoders.
  for( i = 0; i < decoders.count(); i++ )
  { dp = &(const CL_DECODER&)decoders[i];
    if (!dp->get_enabled() || checked[i])
      continue;
    if( dp->get_procs().decoder_fileinfo( filename, info ) == 0 )
      goto ok;
  }

  return 200;
 ok:
  if (name)
    sfnameext( name, dp->module_name, _MAX_FNAME );
  DEBUGLOG(("dec_fileinfo: {{, %d, %d, %d, %d}, {%d, %d, %d, %s}} -> %s\n",
    info->format.samplerate, info->format.channels, info->format.bits, info->format.format,
    info->tech.songlength, info->tech.bitrate, info->tech.filesize, info->tech.info,
    name)); 
  return 0;
}

ULONG PM123_ENTRY
dec_cdinfo( char *drive, DECODER_CDINFO *info )
{
  ULONG last_rc = 200;
  int i;

  for( i = 0; i < decoders.count(); i++ )
  { const CL_DECODER& dec = (const CL_DECODER&)decoders[i];
    if ( dec.get_enabled()
      && (dec.get_procs().type & DECODER_TRACK)
      && dec.get_procs().decoder_cdinfo != NULL )
    { last_rc = dec.get_procs().decoder_cdinfo( drive, info );
      if( last_rc == 0 )
        return 0;
    }
  }
  return last_rc; // returns only the last RC ... hum
}

ULONG PM123_ENTRY
dec_status( void )
{
  const CL_DECODER* dp = (CL_DECODER*)decoders.current();
  if( dp != NULL ) {
    return dp->get_procs().decoder_status( dp->get_procs().w );
  } else {
    return DECODER_ERROR;
  }
}

/* Length in ms, should still be valid if decoder stops. */
ULONG PM123_ENTRY
dec_length( void )
{
  const CL_DECODER* dp = (CL_DECODER*)decoders.current();
  if( dp != NULL ) {
    return dp->get_procs().decoder_length( dp->get_procs().w );
  } else {
    return 0; // bah??
  }
}

/* Fills specified buffer with the list of extensions of supported files. */
void
dec_fill_types( char* result, size_t size )
{
  int i;
  size_t res_size = 0;
  char* dp = result;
  *dp = 0;

  for( i = 0; i < decoders.count(); i++ )
  { if (!decoders[i].get_enabled())
      continue;
    const char* const* sp = ((CL_DECODER&)decoders[i]).get_procs().support;
    if (!sp)
      continue;
    for ( ; *sp; ++sp)
    { int add_size = strlen(*sp) +1;
      if ( res_size + add_size >= size )
        continue;
      if (strstr(result, *sp) != NULL) // no double results
        continue;
      if (res_size)
        dp[-1] = ';';
      res_size += add_size;
      memcpy(dp, *sp, add_size);
      dp += add_size;
    }
  }
}

BOOL out_is_active( int number )
{ return number >= 0 && number < outputs.count() && &outputs[number] == outputs.current();
}

int out_set_active( int number )
{ return outputs.set_active(number) -1;
}


/* Initializes the specified visual plug-in. */
BOOL vis_init( int i )
{ DEBUGLOG(("plugman:vis_init(%d)\n", i));;;;

  if (i < 0 || i >= visuals.count())
    return FALSE;

  CL_VISUAL& vis = (CL_VISUAL&)visuals[i];
  
  if (!vis.get_enabled() || vis.is_initialized())
    return FALSE;

  PLUGIN_PROCS  procs;
  procs.output_playing_samples = out_playing_samples;
  procs.decoder_playing        = decoder_playing;
  procs.output_playing_pos     = out_playing_pos;
  procs.decoder_status         = dec_status;
  procs.decoder_fileinfo       = dec_fileinfo;
  procs.pm123_getstring        = pm123_getstring;
  procs.pm123_control          = pm123_control;
  procs.decoder_cdinfo         = dec_cdinfo;
  procs.decoder_length         = dec_length;

  return vis.initialize(amp_player_window(), &procs, i);
}

void vis_init_all( BOOL skin )
{ for( int i = 0; i < visuals.count(); i++ )
    if( ((CL_VISUAL&)visuals[i]).get_properties().skin == skin && visuals[i].get_enabled() )
      vis_init( i );
} 

/* Terminates the specified visual plug-in. */
BOOL vis_deinit( int i )
{ if ( i < 0 || i >= visuals.count() )
    return FALSE;
  return visuals[i].uninit_plugin();
}

void vis_deinit_all( BOOL skin )
{ for( int i = 0; i < visuals.count(); i++ )
    if( ((CL_VISUAL&)visuals[i]).get_properties().skin == skin )
      vis_deinit( i );
}

/* Broadcats specified message to all enabled visual plug-ins. */
void vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  int  i;

  for( i = 0; i < visuals.count(); i++ ) {
    CL_VISUAL& vis = (CL_VISUAL&)visuals[i];
    if( vis.get_enabled() && vis.is_initialized() ) {
      WinSendMsg( vis.get_hwnd(), msg, mp1, mp2 );
    }
  }
}

/* Backward compatibility */
BOOL
decoder_playing( void )
{
  ULONG status = dec_status();
  return ( status == DECODER_PLAYING || status == DECODER_STARTING || out_playing_data());
}


/****************************************************************************
*
* GUI stuff
*
****************************************************************************/

/* Plug-in menu in the main pop-up menu */
typedef struct {

   char* filename;
   int   type;
   int   i;
   int   configurable;
   int   enabled;

} PLUGIN_ENTRY;

static PLUGIN_ENTRY *entries = NULL;
static int num_entries = 0;

void
load_plugin_menu( HWND hMenu )
{
  char     buffer[2048];
  char     file[_MAX_PATH];
  MENUITEM mi;
  int      i;
  int      count;

  // Delete all
  count = LONGFROMMR( WinSendMsg( hMenu, MM_QUERYITEMCOUNT, 0, 0 ));
  for( i = 0; i < count; i++ ) {
    short item = LONGFROMMR( WinSendMsg( hMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(0), 0 ));
    WinSendMsg( hMenu, MM_DELETEITEM, MPFROM2SHORT( item, FALSE ), 0);
  }
  for( i = 0; i < num_entries; i++ ) {
    free( entries[i].filename );
  }
  free( entries );
  entries  = NULL;
  num_entries = 0;

  if( !plugins.count() )
  {
    mi.iPosition   = MIT_END;
    mi.afStyle     = MIS_TEXT;
    mi.afAttribute = MIA_DISABLED;
    mi.id          = IDM_M_BOOKMARKS + 1;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem       = 0;
    WinSendMsg( hMenu, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( "No plug-ins" ));
    return;
  }

  num_entries = plugins.count();
  entries = (PLUGIN_ENTRY*) malloc( num_entries * sizeof( PLUGIN_ENTRY ));

  for( i = 0; i < num_entries; i++ )
  { CL_MODULE& plug = plugins[i];
    sprintf( buffer, "%s (%s)", plug.query_param.desc,
             sfname( file, plug.module_name, sizeof( file )));

    entries[i].filename     = strdup( buffer );
    entries[i].type         = plug.query_param.type;
    entries[i].i            = i;
    entries[i].configurable = plug.query_param.configurable;
    entries[i].enabled      = FALSE;
    if (plug.query_param.type & PLUGIN_VISUAL)
    { int p = visuals.find(plug);
      if (p != -1 && visuals[p].get_enabled())
      { entries[i].enabled = TRUE;
        continue;
      }
    }
    if (plug.query_param.type & PLUGIN_DECODER)
    { int p = decoders.find(plug);
      if (p != -1 && decoders[p].get_enabled())
      { entries[i].enabled = TRUE;
        continue;
      }
    }
    if (plug.query_param.type & PLUGIN_OUTPUT)
    { if (outputs.current() != NULL && outputs.current()->module == plug.module)
      { entries[i].enabled = TRUE;
        continue;
      }
    }
    if (plug.query_param.type & PLUGIN_FILTER)
    { int p = filters.find(plug);
      if (p != -1 && filters[p].get_enabled())
      { entries[i].enabled = TRUE;
        continue;
      }
    }
  }

  for( i = 0; i < num_entries; i++ )
  {
    mi.iPosition = MIT_END;
    mi.afStyle = MIS_TEXT;
    mi.afAttribute = 0;

    if( !entries[i].configurable ) {
      mi.afAttribute |= MIA_DISABLED;
    }
    if( entries[i].enabled ) {
      mi.afAttribute |= MIA_CHECKED;
    }

    mi.id = IDM_M_PLUG + i + 1;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem = 0;
    WinSendMsg( hMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP( entries[i].filename ));
  }

  return;
}

BOOL
process_possible_plugin( HWND hwnd, USHORT cmd )
{
  int i = cmd - IDM_M_PLUG - 1;

  if (i >= 0 && i < num_entries )
  { plugins[entries[i].i].config(hwnd);
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Unloads and removes all loaded plug-ins. */
void remove_all_plugins( void )
{
  decoders.clear();
  filters.clear();
  visuals.clear();
  outputs.clear();

  for( int i = 0; i < num_entries; i++ ) {
    free( entries[i].filename );
  }
  free( entries );
  entries  = NULL;
  num_entries = 0;
}

