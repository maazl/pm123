/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2008 Marcel Mueller
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
#include <cpp/mutex.h>
#include "plugman_base.h"
#include "123_util.h"
#include "pm123.h" // for startpath
#include "properties.h"
#include "playable.h"
#include "pm123.rc.h"

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
* global events
*
****************************************************************************/

event<const dec_event_args>   dec_event;
event<const OUTEVENTTYPE>     out_event;
event<const PLUGIN_EVENTARGS> plugin_event;


/****************************************************************************
*
* virtual output interface, including filter plug-ins
* This class logically connects the decoder and the output interface.
*
****************************************************************************/

class CL_GLUE
{private:
  static const dec_event_args ev_playstop;
      
  static BOOL              initialized;       // whether the following vars are true
  static OUTPUT_PROCS      procs;             // entry points of the filter chain
  static OUTPUT_PARAMS2    params;            // parameters for output_command
  static DECODER_PARAMS2   dparams;           // parameters for decoder_command
  static xstring           url;               // current URL
  static T_TIME            stoptime;          // timeindex where to stop the current playback
  static volatile unsigned playstopsent;      // DECEVENT_PLAYSTOP has been sent. May be set to 0 to discard samples.
  static T_TIME            posoffset;         // Offset to posmarker parameter to make the output timeline monotonic
  static FORMAT_INFO2      last_format;       // Format of last request_buffer
  static T_TIME            minpos;            // minimum sample position of a block from the decoder since the last dec_play
  static T_TIME            maxpos;            // maximum sample position of a block from the decoder since the last dec_play

 private:
  // Virtualize output procedures by invoking the filter plugin no. i.
  static void              virtualize         ( int i );
  // setup the entire filter chain
  static ULONG             init();
  // uninitialize the filter chain
  static void              uninit();
  // Select decoder by index.
  static int               dec_set_active     ( int number )
                                              { return decoders.set_active(number); }
  // Select decoder by name.
  // Returns -1 if a error occured,
  // returns -2 if can't find nothing,
  // returns 0  if succesful.
  static int               dec_set_active     ( const char* name );
  // Send command to the current decoder using the current content of dparams.
  static ULONG             dec_command        ( ULONG msg );
  // Send command to the current output and filter chain using the current content of params.
  static ULONG             out_command        ( ULONG msg )
                                              { return (*procs.output_command)( procs.a, msg, &params ); }
 public:
  // output control interface (C style)
  friend ULONG             out_setup          ( const Song* song );
  friend ULONG             out_close          ();
  friend void              out_set_volume     ( double volume );
  friend ULONG             out_pause          ( BOOL pause );
  friend BOOL              out_flush          ();
  friend BOOL              out_trash          ();
  // decoder control interface (C style)
  friend ULONG             dec_play           ( const Song* song, T_TIME offset, T_TIME start, T_TIME stop );
  friend ULONG             dec_stop           ();
  friend ULONG             dec_fast           ( DECFASTMODE mode );
  friend ULONG             dec_jump           ( T_TIME pos );
  friend ULONG             dec_eq             ( const float* bandgain );
  friend ULONG             dec_save           ( const char* file );
  friend T_TIME            dec_minpos         ();
  friend T_TIME            dec_maxpos         ();
  // 4 visual interface (C style)
  friend T_TIME DLLENTRY   out_playing_pos    ();
  friend BOOL   DLLENTRY   out_playing_data   ();
  friend ULONG  DLLENTRY   out_playing_samples( FORMAT_INFO* info, char* buf, int len );

 private: // glue
  PROXYFUNCDEF int DLLENTRY glue_request_buffer( void* a, const FORMAT_INFO2* format, short** buf );
  PROXYFUNCDEF void DLLENTRY glue_commit_buffer( void* a, int len, T_TIME posmarker );
  // 4 callback interface
  PROXYFUNCDEF void DLLENTRY dec_event_handler( void* a, DECEVENTTYPE event, void* param );
  PROXYFUNCDEF void DLLENTRY out_event_handler( void* w, OUTEVENTTYPE event );
};

// statics
const dec_event_args CL_GLUE::ev_playstop = { DECEVENT_PLAYSTOP, NULL };

BOOL              CL_GLUE::initialized = false;
OUTPUT_PROCS      CL_GLUE::procs;
OUTPUT_PARAMS2    CL_GLUE::params = {0};
DECODER_PARAMS2   CL_GLUE::dparams = {0};
xstring           CL_GLUE::url;
T_TIME            CL_GLUE::stoptime;
volatile unsigned CL_GLUE::playstopsent;
T_TIME            CL_GLUE::posoffset;
FORMAT_INFO2      CL_GLUE::last_format;
T_TIME            CL_GLUE::minpos;
T_TIME            CL_GLUE::maxpos;

void CL_GLUE::virtualize(int i)
{ DEBUGLOG(("CL_GLUE::virtualize(%d) - %p\n", i, params.info));

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
  par.error_display          = &pm123_display_error;
  par.info_display           = &pm123_display_info;
  par.pm123_getstring        = &pm123_getstring;
  par.pm123_control          = &pm123_control;
  if (!fil.initialize(&par))
  { pm123_display_info(xstring::sprintf("The filter plug-in %s failed to initialize.", fil.module_name));
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
  void DLLENTRYP(last_output_event)(void* w, OUTEVENTTYPE event) = params.output_event;
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
{ DEBUGLOG(("CL_GLUE::init\n"));

  params.size          = sizeof params;
  params.error_display = &pm123_display_error;
  params.info_display  = &pm123_display_info;
  // setup callback handlers
  params.output_event  = &out_event_handler;
  params.w             = NULL; // not required

  memset(&dparams, 0, sizeof dparams);
  dparams.size            = sizeof dparams;
  // setup callback handlers
  dparams.output_event    = &dec_event_handler;
  dparams.error_display   = &pm123_display_error;
  dparams.info_display    = &pm123_display_info;
  dparams.save_filename   = NULL;

  CL_OUTPUT* op = (CL_OUTPUT*)outputs.current();
  if ( op == NULL )
  { pm123_display_error( "There is no active output plug-in." );
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
{ DEBUGLOG(("CL_GLUE::uninit\n"));

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
  if( name == NULL ) {
    return dec_set_active( -1 );
  }

  int i = decoders.find_short(name);
  return i < 0
   ? -2
   : dec_set_active( i );
}

ULONG CL_GLUE::dec_command( ULONG msg )
{ DEBUGLOG(("dec_command(%d)\n", msg));
  CL_PLUGIN* pp = decoders.current();
  if ( pp == NULL )
    return 3; // no decoder active

  const DECODER_PROCS& procs = ((CL_DECODER*)pp)->get_procs();
  ULONG ret = (*procs.decoder_command)( procs.w, msg, &dparams );
  DEBUGLOG(("dec_command: %lu\n", ret));
  return ret;
}

/* invoke decoder to play an URL */
ULONG dec_play( const Song* song, T_TIME offset, T_TIME start, T_TIME stop )
{
  DEBUGLOG(("dec_play(%p{%s}, %f, %f, %f)\n", &*song, song->GetURL().cdata(), offset, start, stop));
  ULONG rc = CL_GLUE::dec_set_active(song->GetDecoder());
  if ( rc != 0 )
    return rc;

  CL_GLUE::stoptime     = stop < 0 ? 1E99 : stop;
  CL_GLUE::playstopsent = FALSE;
  CL_GLUE::posoffset    = offset;
  CL_GLUE::minpos       = 1E99;
  CL_GLUE::maxpos       = 0;

  CL_GLUE::dparams.URL                   = song->GetURL();
  CL_GLUE::dparams.jumpto                = start;
  CL_GLUE::dparams.output_request_buffer = &PROXYFUNCREF(CL_GLUE)glue_request_buffer;
  CL_GLUE::dparams.output_commit_buffer  = &PROXYFUNCREF(CL_GLUE)glue_commit_buffer;
  CL_GLUE::dparams.a                     = CL_GLUE::procs.a;
  CL_GLUE::dparams.buffersize            = cfg.buff_size*1024;
  CL_GLUE::dparams.bufferwait            = cfg.buff_wait;
  CL_GLUE::dparams.proxyurl              = cfg.proxy;
  CL_GLUE::dparams.httpauth              = cfg.auth;

  rc = CL_GLUE::dec_command( DECODER_SETUP );
  if ( rc != 0 )
    return rc;
  CL_GLUE::url = song->GetURL();

  CL_GLUE::dec_command( DECODER_SAVEDATA );

  return CL_GLUE::dec_command( DECODER_PLAY );
}

/* stop the current decoder immediately */
ULONG dec_stop( void )
{ CL_GLUE::url = NULL;
  ULONG rc = CL_GLUE::dec_command( DECODER_STOP );
  if (rc == 0)
    CL_GLUE::dec_set_active( -1 );
  return rc;
}

/* set fast forward/rewind mode */
ULONG dec_fast( DECFASTMODE mode )
{ CL_GLUE::dparams.fast = mode;
  return CL_GLUE::dec_command( DECODER_FFWD );
}

/* jump to absolute position */
ULONG dec_jump( T_TIME location )
{ // Discard stop time if we seek beyond this point.
  if (location >= CL_GLUE::stoptime)
    CL_GLUE::stoptime = 1E99;
  CL_GLUE::dparams.jumpto = location;
  ULONG rc = CL_GLUE::dec_command( DECODER_JUMPTO );
  if (rc == 0 && CL_GLUE::initialized)
  { CL_GLUE::params.temp_playingpos = location;
    CL_GLUE::out_command( OUTPUT_TRASH_BUFFERS );
  }
  return rc;
}

/* set savefilename to save the raw stream data */
ULONG dec_save( const char* file )
{ if (file != NULL && *file == 0)
    file = NULL;
  CL_GLUE::dparams.save_filename = file;
  ULONG status = dec_status();
  return status == DECODER_PLAYING || status == DECODER_STARTING || status == DECODER_PAUSED
   ? CL_GLUE::dec_command( DECODER_SAVEDATA )
   : 0;
}

T_TIME dec_minpos()
{ return CL_GLUE::minpos;
}

T_TIME dec_maxpos()
{ return CL_GLUE::maxpos;
}


/* setup new output stage or change the properties of the current one */
ULONG out_setup( const Song* song )
{ DEBUGLOG(("out_setup(%p{%s,{%i,%i,%i}})\n", &*song, song->GetURL().cdata(), song->GetInfo().format->size, song->GetInfo().format->samplerate, song->GetInfo().format->channels));
  CL_GLUE::params.formatinfo = *song->GetInfo().format;
  CL_GLUE::params.URI        = song->GetURL();
  CL_GLUE::params.info       = &song->GetInfo();
  if (!CL_GLUE::initialized)
  { ULONG rc = CL_GLUE::init(); // here we initialte the setup of the filter chain
    if (rc != 0)
      return rc;
  }
  DEBUGLOG(("out_setup before out_command %p\n", CL_GLUE::params.info));
  return CL_GLUE::out_command( OUTPUT_OPEN );
}

/* closes output device and uninitializes all filter and output plugins */
ULONG out_close()
{ if (!CL_GLUE::initialized)
    return (ULONG)-1;
  CL_GLUE::params.temp_playingpos = (*CL_GLUE::procs.output_playing_pos)( CL_GLUE::procs.a );;
  CL_GLUE::out_command( OUTPUT_TRASH_BUFFERS );
  ULONG rc = CL_GLUE::out_command( OUTPUT_CLOSE );
  CL_GLUE::uninit(); // Hmm, is it a good advise to do this in case of an error?
  return rc;
}

/* adjust output volume */
void out_set_volume( double volume )
{ if (!CL_GLUE::initialized)
    return; // can't help
  CL_GLUE::params.volume = volume;
  CL_GLUE::params.amplifier = 1.0;
  CL_GLUE::out_command( OUTPUT_VOLUME );
}

ULONG out_pause( BOOL pause )
{ if (!CL_GLUE::initialized)
    return (ULONG)-1; // error
  CL_GLUE::params.pause = pause;
  return CL_GLUE::out_command( OUTPUT_PAUSE );
}

BOOL out_flush()
{ if (!CL_GLUE::initialized)
    return FALSE;
  (*CL_GLUE::procs.output_request_buffer)( CL_GLUE::procs.a, NULL, NULL );
  return TRUE;
}

BOOL out_trash()
{ if (!CL_GLUE::initialized)
    return FALSE;
  CL_GLUE::out_command( OUTPUT_TRASH_BUFFERS );
  return TRUE;
}

/* Returns 0 = success otherwize MMOS/2 error. */
ULONG DLLENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len )
{ if (!CL_GLUE::initialized)
    return (ULONG)-1; // N/A
  return (*CL_GLUE::procs.output_playing_samples)( CL_GLUE::procs.a, info, buf, len );
}

/* Returns time in ms. */
T_TIME DLLENTRY out_playing_pos( void )
{ if (!CL_GLUE::initialized)
    return 0; // ??
  return (*CL_GLUE::procs.output_playing_pos)( CL_GLUE::procs.a );
}

/* if the output is playing. */
BOOL DLLENTRY out_playing_data( void )
{ if (!CL_GLUE::initialized)
    return FALSE;
  return (*CL_GLUE::procs.output_playing_data)( CL_GLUE::procs.a );
}

PROXYFUNCIMP(int DLLENTRY, CL_GLUE)
glue_request_buffer( void* a, const FORMAT_INFO2* format, short** buf )
{ // do not pass flush, signal DECEVENT_PLAYSTOP instead.
  if (buf == NULL)
  { if (InterlockedXch(CL_GLUE::playstopsent, TRUE) == FALSE)
      dec_event(CL_GLUE::ev_playstop);
    return 0; 
  }
  // pass request to output
  CL_GLUE::last_format = *format;
  return (*CL_GLUE::procs.output_request_buffer)(a, format, buf);
}

PROXYFUNCIMP(void DLLENTRY, CL_GLUE)
glue_commit_buffer( void* a, int len, T_TIME posmarker )
{ bool send_playstop = false;
  T_TIME pos_e = posmarker + (T_TIME)len / CL_GLUE::last_format.samplerate;
  
  // check stop offset
  if (pos_e >= CL_GLUE::stoptime)
  { pos_e = CL_GLUE::stoptime;
    // calcutate fractional part
    len = posmarker >= CL_GLUE::stoptime
      ? 0 // begin is already beyond the limit => cancel request
      : (int)((CL_GLUE::stoptime - posmarker) * CL_GLUE::last_format.samplerate +.5);
    // Signal DECEVENT_PLAYSTOP if not yet sent.
    send_playstop = InterlockedXch(CL_GLUE::playstopsent, TRUE) == FALSE;
  }

  // update min/max
  if (CL_GLUE::minpos > posmarker)
    CL_GLUE::minpos = posmarker;
  if (CL_GLUE::maxpos < pos_e)
    CL_GLUE::maxpos = pos_e;

  // pass to the output
  (*CL_GLUE::procs.output_commit_buffer)(a, len, posmarker + CL_GLUE::posoffset);

  // Signal DECEVENT_PLAYSTOP ?
  if (send_playstop)
    dec_event(CL_GLUE::ev_playstop); 
}

PROXYFUNCIMP(void DLLENTRY, CL_GLUE)
dec_event_handler( void* a, DECEVENTTYPE event, void* param )
{ DEBUGLOG(("plugman:dec_event_handler(%p, %d, %p)\n", a, event, param));
  // We handle some event here immediately.
  switch (event)
  {case DEVEVENT_CHANGETECH:
    if (CL_GLUE::url)
    { int_ptr<Playable> pp = Playable::FindByURL(CL_GLUE::url);
      if (pp)
        pp->SetTechInfo((TECH_INFO*)param);
    }
    break;
    
   case DECEVENT_CHANGEMETA:
    // Well,the backend does not support time dependant metadata.
    // From the playlist view, metadata changes should be immediately visible.
    // But during playback the change should be delayed until the apropriate buffer is really played.
    // The latter cannot be implemented with the current backend.
    if (CL_GLUE::url)
    { int_ptr<Playable> pp = Playable::FindByURL(CL_GLUE::url);
      if (pp)
        pp->SetMetaInfo((META_INFO*)param);
    }
    break;

   case DECEVENT_PLAYSTOP:
    CL_GLUE::stoptime = 0;
    // discard if already sent
    if (InterlockedXch(CL_GLUE::playstopsent, TRUE))
      return;
   default: // avoid warnings
    break;
  }   
  const dec_event_args args = { event, param };
  dec_event(args); 
}

/* proxy, because the decoder is not interested in OUTEVENT_END_OF_DATA */
PROXYFUNCIMP(void DLLENTRY, CL_GLUE)
out_event_handler( void* w, OUTEVENTTYPE event )
{ DEBUGLOG(("plugman:out_event_handler(%p, %d)\n", w, event));
  out_event(event); 
  // route high/low water events to the decoder (if any)
  switch (event)
  {case OUTEVENT_LOW_WATER:
   case OUTEVENT_HIGH_WATER:
    { CL_DECODER* dp = (CL_DECODER*)decoders.current();
      if (dp != NULL && dp->get_procs().w != NULL)
        dp->get_procs().decoder_event(dp->get_procs().w, event);
      break;
    }
   default: // avoid warnings
    break;
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
  { pm123_display_error( xstring::sprintf("Tried to load the plug-in %s twice.\n", mod.module_name) );
    return NULL;
  }
  CL_PLUGIN* pp = (*factory)(mod);
  if (!pp->load_plugin() || !list.append(pp))
  { delete pp;
    DEBUGLOG(("plugman:instantiate: failed.\n"));
    return NULL;
  }
  pp->set_enabled(enabled);
  return pp;
}

/* load a plug-in, return 0 on error */
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
    return 0;
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
    DEBUGLOG(("plugman:add_plugin_core: visuals.find returned %i\n", q));
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
  add_plugin_core("oggplay.dll",  NULL, PLUGIN_DECODER);
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

  unsigned int i, size, count;
  BOOL enabled = TRUE;
  char module_name[_MAX_PATH];

  if( read_bufstream( b, &count, sizeof count) != sizeof count)
    return FALSE;

  for( i = 0; i < count; i++ )
  { if ( read_bufstream( b, &size, sizeof size) != sizeof size
      || size >= sizeof module_name
      || read_bufstream( b, module_name, size ) != size
      || (withenabled && read_bufstream( b, &enabled, sizeof enabled) != sizeof enabled) )
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
ULONG DLLENTRY
dec_fileinfo( const char* filename, INFOTYPE* what, DECODER_INFO2* info, char* name, size_t name_size )
{
  DEBUGLOG(("dec_fileinfo(%s, %x, %p{%u,%p,%p,%p}, %s)\n", filename, *what, info, info->size, info->format, info->tech, info->meta, name));
  BOOL* checked = (BOOL*)alloca( sizeof( BOOL ) * decoders.count() );
  const CL_DECODER* dp;
  INFOTYPE what2;

  memset( checked, 0, sizeof( BOOL ) * decoders.count() );

  if (is_track(filename))
  { // check decoders that claim to support tracks
    for( int i = 0; i < decoders.count(); i++ )
    { dp = &(const CL_DECODER&)decoders[i];
      if (!dp->get_enabled() || !(dp->get_procs().type & DECODER_TRACK) )
        continue;
      checked[i] = TRUE;
      what2 = *what;
      if (dp->get_procs().decoder_fileinfo(filename, &what2, info) == 0)
        goto ok;
    }
    *what = INFO_ALL; // Even in case of an error the requested information is in fact available.
    return 200; // It makes no sense to check the others in this case.
  } else
  { // First checks decoders supporting the specified type of files.
    for ( int i = 0; i < decoders.count(); i++)
    { dp = &(const CL_DECODER&)decoders[i];
      if (!dp->get_enabled() || !is_file_supported(dp->get_procs().support, filename))
        continue;
      checked[i] = TRUE;
      what2 = *what;
      if (dp->get_procs().decoder_fileinfo(filename, &what2, info) == 0)
        goto ok;
    }
  }

  { // Next checks the rest decoders.
    for( int i = 0; i < decoders.count(); i++ )
    { dp = &(const CL_DECODER&)decoders[i];
      if (!dp->get_enabled() || checked[i])
        continue;
      what2 = *what;
      if( (*dp->get_procs().decoder_fileinfo)( filename, &what2, info ) == 0 )
        goto ok;
    }
  }
  *what = INFO_ALL; // Even in case of an error the requested information is in fact available.
  return 200;
 ok:
  DEBUGLOG(("dec_fileinfo: {{%d, %d, %d}, {%d, %lf, %d, %lf, %s}, {%d, %lf, %d}, {%d, %d, %d}} -> %s, %x\n",
    info->format->size, info->format->samplerate, info->format->channels,
    info->tech->size, info->tech->songlength, info->tech->bitrate, info->tech->totalsize, info->tech->info,
    info->phys->size, info->phys->filesize, info->phys->num_items,
    info->rpl->size, info->rpl->total_items, info->rpl->recursive,
    dp->module_name, what2));
  ASSERT((*what & ~what2) == 0); // The decoder must not reset bits.
  *what = (INFOTYPE)(what2 | *what); // do not reset bits
  if (name)
    sfnameext( name, dp->module_name, name_size );
  return 0;
}

ULONG DLLENTRY
dec_cdinfo( const char *drive, DECODER_CDINFO *info )
{
  DEBUGLOG(("dec_cdinfo(%s, %p)\n", drive, info));
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

ULONG DLLENTRY
dec_status( void )
{
  const CL_DECODER* dp = (CL_DECODER*)decoders.current();
  // TODO: this check is still not 100% thread safe
  if (dp != NULL && dp->get_procs().w != NULL)
    return dp->get_procs().decoder_status( dp->get_procs().w );
  else
    return DECODER_ERROR;
}

/* Backward compatibility */
BOOL
decoder_playing( void )
{
  ULONG status = dec_status();
  return ( status == DECODER_PLAYING || status == DECODER_STARTING || status == DECODER_PAUSED || out_playing_data());
}

/* Length in ms, should still be valid if decoder stops. */
T_TIME DLLENTRY
dec_length( void )
{
  const CL_DECODER* dp = (CL_DECODER*)decoders.current();
  // TODO: this check is still not 100% thread safe
  if (dp != NULL && dp->get_procs().w != NULL)
    return dp->get_procs().decoder_length( dp->get_procs().w );
  else
    return 0; // bah??
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

ULONG
dec_editmeta( HWND owner, const char* url, const char* decoder_name )
{ DEBUGLOG(("dec_editmeta(%x, %s, %s)\n", owner, url, decoder_name));
  ULONG rc;
  // find decoder
  int i = decoders.find_short(decoder_name);
  if (i == -1)
  { rc = 100;
  } else
  // get entry points
  { CL_DECODER& dec = (CL_DECODER&)decoders[i];
    const DECODER_PROCS& procs = dec.get_procs();
    if (!procs.decoder_editmeta)
    { rc = 400;
    } else
    { // detach configure
      rc = (*procs.decoder_editmeta)(owner, url);
    }
  }
  DEBUGLOG(("dec_editmeta: %d\n", rc));
  return rc;
}

BOOL out_is_active( int number )
{ return number >= 0 && number < outputs.count() && &outputs[number] == outputs.current();
}

int out_set_active( int number )
{ return outputs.set_active(number) -1;
}


/* Initializes the specified visual plug-in. */
BOOL vis_init( int i )
{ DEBUGLOG(("plugman:vis_init(%d)\n", i));

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


/****************************************************************************
*
* GUI stuff
*
****************************************************************************/

/* launch the configue dialog of the n-th plugin of a certain type. Use PLUGIN_NULL to use an index in the global plug-in list */
BOOL configure_plugin(int type, int i, HWND hwnd)
{ // fetch the matching container
  CL_PLUGIN_BASE_LIST* list;
  switch (type)
  {default:
    return FALSE;
   case PLUGIN_NULL:
    list = &plugins;
    break;
   case PLUGIN_VISUAL:
    list = &visuals;
    break;
   case PLUGIN_FILTER:
    list = &filters;
    break;
   case PLUGIN_DECODER:
    list = &decoders;
    break;
   case PLUGIN_OUTPUT:
    list = &outputs;
    break;
  }
  if (i < 0 || i >= list->count())
    return FALSE;
  // launch dialog
  if (type == PLUGIN_NULL)
    ((CL_MODULE&)(*list)[i]).config(hwnd);
   else
    ((CL_PLUGIN&)(*list)[i]).get_module().config(hwnd);
  return TRUE;
}

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
  DEBUGLOG(("load_plugin_menu(%p)\n", hMenu));

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
    DEBUGLOG2(("load_plugin_menu: add \"%s\"\n", entries[i].filename));
  }

  return;
}

void
dec_append_load_menu( HWND hMenu, ULONG id_base, SHORT where, DECODER_WIZZARD_FUNC* callbacks, int size )
{ DEBUGLOG(("dec_append_load_menu(%p, %d, %d, %p, %d)\n", hMenu, id_base, callbacks, size));
  int i;
  // cleanup
  SHORT lastcount = -1;
  for (i = 0; i < size; ++i)
  { SHORT newcount = SHORT1FROMMP(WinSendMsg(hMenu, MM_DELETEITEM, MPFROM2SHORT(id_base+i, FALSE), 0));
    DEBUGLOG(("dec_append_load_menu - %i %i\n", i, newcount));
    if (newcount == lastcount)
      break;
    lastcount = newcount;
  }
  // for all decoder plug-ins...
  for (i = 0; i < decoders.count(); ++i)
  { CL_DECODER& dec = (CL_DECODER&)decoders[i];
    if (dec.get_enabled() && dec.get_procs().decoder_getwizzard)
    { const DECODER_WIZZARD* da = (*dec.get_procs().decoder_getwizzard)();
      DEBUGLOG(("dec_append_load_menu: %s - %p\n", dec.module_name, da));
      MENUITEM mi = {0};
      mi.iPosition   = where;
      mi.afStyle     = MIS_TEXT;
      //mi.afAttribute = 0;
      //mi.hwndSubMenu = NULLHANDLE;
      //mi.hItem       = 0;
      for (; da != NULL; da = da->link, ++id_base)
      { if (size-- == 0)
          return; // too many entries, can't help
        // Add menu item
        mi.id        = id_base;
        SHORT pos = SHORT1FROMMR(WinSendMsg(hMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(da->prompt)));
        DEBUGLOG(("dec_append_load_menu: add %u: %s -> %p => %u\n", id_base, da->prompt, da->wizzard, pos));
        // Add callback function
        *callbacks++ = da->wizzard;
        if (mi.iPosition != MIT_END)
          ++mi.iPosition;
      }
    }
  }
}

void dec_append_accel_table( HACCEL& haccel, ULONG id_base, LONG offset, DECODER_WIZZARD_FUNC* callbacks, int size )
{ DEBUGLOG(("dec_append_accel_table(%p, %u, %u, %p, %u)\n", haccel, id_base, offset, callbacks, size));
  // Fetch content
  ULONG accelsize = WinCopyAccelTable(haccel, NULL, 0);
  PMASSERT(accelsize);
  accelsize += (size << (offset != 0)) * sizeof(ACCEL); // space for plug-in entries 
  ACCELTABLE* paccel = (ACCELTABLE*)alloca(accelsize);
  PMRASSERT(WinCopyAccelTable(haccel, paccel, accelsize));
  DEBUGLOG(("dec_append_accel_table: %i\n", paccel->cAccel));
  bool modified = false;
  // Append plug-in accelerators
  for (int i = 0; i < decoders.count(); ++i)
  { CL_DECODER& dec = (CL_DECODER&)decoders[i];
    if (dec.get_enabled() && dec.get_procs().decoder_getwizzard)
    { const DECODER_WIZZARD* da = (*dec.get_procs().decoder_getwizzard)();
      DEBUGLOG(("dec_append_accel_table: %s - %p\n", dec.module_name, da));
      for (; da != NULL; da = da->link, ++id_base)
      { if (size-- == 0)
          goto nomore; // too many entries, can't help
        DEBUGLOG(("dec_append_accel_table: at %u: %s -> %x -> %p\n", id_base, da->prompt, da->accel_key, da->wizzard));
        *callbacks++ = da->wizzard;
        if (da->accel_key)
        { // Add table entry
          ACCEL& accel = paccel->aaccel[paccel->cAccel++];
          accel.fs  = da->accel_opt;
          accel.key = da->accel_key;
          accel.cmd = id_base;
          DEBUGLOG(("dec_append_accel_table: add {%x, %x, %i}\n", accel.fs, accel.key, accel.cmd));
          modified = true;
        }
        if (da->accel_key2 && offset)
        { // Add table entry
          ACCEL& accel = paccel->aaccel[paccel->cAccel++];
          accel.fs  = da->accel_opt2;
          accel.key = da->accel_key2;
          accel.cmd = id_base + offset;
          DEBUGLOG(("dec_append_accel_table: add {%x, %x, %i}\n", accel.fs, accel.key, accel.cmd));
          modified = true;
        }
      }
    }
  }
 nomore:
  if (modified)
  { PMRASSERT(WinDestroyAccelTable(haccel));
    haccel = WinCreateAccelTable(amp_player_hab(), paccel);
    PMASSERT(haccel != NULLHANDLE);
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


/****************************************************************************
*
*  Global initialization functions
*
****************************************************************************/

void plugman_init()
{ CL_PLUGIN::init();
  CL_DECODER::init();
  CL_OUTPUT::init();
}

void plugman_uninit()
{ CL_OUTPUT::uninit();
  CL_DECODER::uninit();
  CL_PLUGIN::uninit();
}
