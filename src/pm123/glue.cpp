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
#include "glue.h"
#include "plugman_base.h"
#include "123_util.h"
#include "properties.h"
#include "pm123.h"
#include <fileutil.h>
#include <os2.h>

#include <cpp/container.h>
//#define DEBUG 1
#include <debuglog.h>

/* thread priorities for decoder thread */
#define  DECODER_HIGH_PRIORITY_CLASS PRTYC_FOREGROUNDSERVER // sorry, we should not lockup the system with time-critical priority
#define  DECODER_HIGH_PRIORITY_DELTA 20
#define  DECODER_LOW_PRIORITY_CLASS  PRTYC_REGULAR
#define  DECODER_LOW_PRIORITY_DELTA  1


/****************************************************************************
*
* global events
*
****************************************************************************/

event<const dec_event_args>   dec_event;
event<const OUTEVENTTYPE>     out_event;


/****************************************************************************
*
* virtual output interface, including filter plug-ins
* This class logically connects the decoder and the output interface.
*
****************************************************************************/

class Glue
{private:
  static const dec_event_args ev_playstop;
      
  static BOOL              initialized;       // whether the following vars are true
  static OutputProcs       procs;             // entry points of the filter chain
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
                                              { return Decoders.SetActive(number); }
  // Select decoder by name.
  // Returns -1 if a error occured,
  // returns -2 if can't find nothing,
  // returns 0  if succesful.
  static int               dec_set_active     ( const char* name );
  // Send command to the current decoder using the current content of dparams.
  static ULONG             dec_command        ( ULONG msg );
  // Send command to the current output and filter chain using the current content of params.
  static ULONG             out_command        ( ULONG msg )
                                              { return (*procs.output_command)( procs.A, msg, &params ); }
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
const dec_event_args Glue::ev_playstop = { DECEVENT_PLAYSTOP, NULL };

BOOL              Glue::initialized = false;
OutputProcs       Glue::procs;
OUTPUT_PARAMS2    Glue::params = {0};
DECODER_PARAMS2   Glue::dparams = {0};
xstring           Glue::url;
T_TIME            Glue::stoptime;
volatile unsigned Glue::playstopsent;
T_TIME            Glue::posoffset;
FORMAT_INFO2      Glue::last_format;
T_TIME            Glue::minpos;
T_TIME            Glue::maxpos;

void Glue::virtualize(int i)
{ DEBUGLOG(("Glue::virtualize(%d) - %p\n", i, params.info));

  if (i < 0)
    return;
  Filter* fil = (Filter*)Filters[i];
  if (!fil->GetEnabled())
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
  par.a                      = procs.A;
  par.output_event           = params.output_event;
  par.w                      = params.w;
  par.error_display          = &pm123_display_error;
  par.info_display           = &pm123_display_info;
  par.pm123_getstring        = &pm123_getstring;
  par.pm123_control          = &pm123_control;
  if (!fil->Initialize(&par))
  { pm123_display_info(xstring::sprintf("The filter plug-in %s failed to initialize.", fil->GetModuleName().cdata()));
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
  procs.A                      = fil->GetProcs().F;
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
    params.w            = fil->GetProcs().F;
    DEBUGLOG(("Glue::virtualize: callback virtualized: %p %p\n", params.output_event, params.w));
  }
  if (par.output_event != last_output_event)
  { // now update the decoder event
    (*fil->GetProcs().filter_update)(fil->GetProcs().F, &par);
    DEBUGLOG(("Glue::virtualize: callback update: %p %p\n", par.output_event, par.w));
  }
}

// setup filter chain
ULONG Glue::init()
{ DEBUGLOG(("Glue::init\n"));

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

  Output* op = (Output*)Outputs.Current();
  if ( op == NULL )
  { pm123_display_error( "There is no active output plug-in." );
    return (ULONG)-1;  // ??
  }

  // initially only the output plugin
  procs = op->GetProcs();
  // setup filters
  virtualize(Filters.size()-1);
  // setup output
  ULONG rc = out_command( OUTPUT_SETUP );
  if (rc == 0)
    initialized = TRUE;
  return rc;
}

void Glue::uninit()
{ DEBUGLOG(("Glue::uninit\n"));

  initialized = FALSE;
  // uninitialize filter chain
  for (int i = 0; i < Filters.size(); ++i)
    if (Filters[i]->IsInitialized())
      Filters[i]->UninitPlugin();
}

/* Returns -1 if a error occured,
   returns -2 if can't find nothing,
   returns 0  if succesful. */
int Glue::dec_set_active( const char* name )
{
  if( name == NULL ) {
    return dec_set_active( -1 );
  }

  int i = Decoders.find(name);
  return i < 0
   ? -2
   : dec_set_active( i );
}

ULONG Glue::dec_command( ULONG msg )
{ DEBUGLOG(("dec_command(%d)\n", msg));
  Decoder* dp = (Decoder*)Decoders.Current();
  if ( dp == NULL )
    return 3; // no decoder active

  const DecoderProcs& procs = dp->GetProcs();
  ULONG ret = (*procs.decoder_command)( procs.W, msg, &dparams );
  DEBUGLOG(("dec_command: %lu\n", ret));
  return ret;
}

/* invoke decoder to play an URL */
ULONG dec_play( const Song* song, T_TIME offset, T_TIME start, T_TIME stop )
{
  DEBUGLOG(("dec_play(%p{%s}, %f, %f, %f)\n", &*song, song->GetURL().cdata(), offset, start, stop));
  ULONG rc = Glue::dec_set_active(song->GetDecoder());
  if ( rc != 0 )
    return rc;

  Glue::stoptime     = stop < 0 ? 1E99 : stop;
  Glue::playstopsent = FALSE;
  Glue::posoffset    = offset;
  Glue::minpos       = 1E99;
  Glue::maxpos       = 0;

  Glue::dparams.URL                   = song->GetURL();
  Glue::dparams.jumpto                = start;
  Glue::dparams.output_request_buffer = &PROXYFUNCREF(Glue)glue_request_buffer;
  Glue::dparams.output_commit_buffer  = &PROXYFUNCREF(Glue)glue_commit_buffer;
  Glue::dparams.a                     = Glue::procs.A;
  Glue::dparams.buffersize            = cfg.buff_size*1024;
  Glue::dparams.bufferwait            = cfg.buff_wait;
  Glue::dparams.proxyurl              = cfg.proxy;
  Glue::dparams.httpauth              = cfg.auth;

  rc = Glue::dec_command( DECODER_SETUP );
  if ( rc != 0 )
    return rc;
  Glue::url = song->GetURL();

  Glue::dec_command( DECODER_SAVEDATA );

  return Glue::dec_command( DECODER_PLAY );
}

/* stop the current decoder immediately */
ULONG dec_stop( void )
{ Glue::url = NULL;
  ULONG rc = Glue::dec_command( DECODER_STOP );
  if (rc == 0)
    Glue::dec_set_active( -1 );
  return rc;
}

/* set fast forward/rewind mode */
ULONG dec_fast( DECFASTMODE mode )
{ Glue::dparams.fast = mode;
  return Glue::dec_command( DECODER_FFWD );
}

/* jump to absolute position */
ULONG dec_jump( T_TIME location )
{ // Discard stop time if we seek beyond this point.
  if (location >= Glue::stoptime)
    Glue::stoptime = 1E99;
  Glue::dparams.jumpto = location;
  ULONG rc = Glue::dec_command( DECODER_JUMPTO );
  if (rc == 0 && Glue::initialized)
  { Glue::params.temp_playingpos = location;
    Glue::out_command( OUTPUT_TRASH_BUFFERS );
  }
  return rc;
}

/* set savefilename to save the raw stream data */
ULONG dec_save( const char* file )
{ if (file != NULL && *file == 0)
    file = NULL;
  Glue::dparams.save_filename = file;
  ULONG status = dec_status();
  return status == DECODER_PLAYING || status == DECODER_STARTING || status == DECODER_PAUSED
   ? Glue::dec_command( DECODER_SAVEDATA )
   : 0;
}

T_TIME dec_minpos()
{ return Glue::minpos;
}

T_TIME dec_maxpos()
{ return Glue::maxpos;
}


/* setup new output stage or change the properties of the current one */
ULONG out_setup( const Song* song )
{ DEBUGLOG(("out_setup(%p{%s,{%i,%i,%i}})\n", &*song, song->GetURL().cdata(), song->GetInfo().format->size, song->GetInfo().format->samplerate, song->GetInfo().format->channels));
  Glue::params.formatinfo = *song->GetInfo().format;
  Glue::params.URI        = song->GetURL();
  Glue::params.info       = &song->GetInfo();
  if (!Glue::initialized)
  { ULONG rc = Glue::init(); // here we initialte the setup of the filter chain
    if (rc != 0)
      return rc;
  }
  DEBUGLOG(("out_setup before out_command %p\n", Glue::params.info));
  return Glue::out_command( OUTPUT_OPEN );
}

/* closes output device and uninitializes all filter and output plugins */
ULONG out_close()
{ if (!Glue::initialized)
    return (ULONG)-1;
  Glue::params.temp_playingpos = (*Glue::procs.output_playing_pos)( Glue::procs.A );;
  Glue::out_command( OUTPUT_TRASH_BUFFERS );
  ULONG rc = Glue::out_command( OUTPUT_CLOSE );
  Glue::uninit(); // Hmm, is it a good advise to do this in case of an error?
  return rc;
}

/* adjust output volume */
void out_set_volume( double volume )
{ if (!Glue::initialized)
    return; // can't help
  Glue::params.volume = volume;
  Glue::params.amplifier = 1.0;
  Glue::out_command( OUTPUT_VOLUME );
}

ULONG out_pause( BOOL pause )
{ if (!Glue::initialized)
    return (ULONG)-1; // error
  Glue::params.pause = pause;
  return Glue::out_command( OUTPUT_PAUSE );
}

BOOL out_flush()
{ if (!Glue::initialized)
    return FALSE;
  (*Glue::procs.output_request_buffer)( Glue::procs.A, NULL, NULL );
  return TRUE;
}

BOOL out_trash()
{ if (!Glue::initialized)
    return FALSE;
  Glue::out_command( OUTPUT_TRASH_BUFFERS );
  return TRUE;
}

/* Returns 0 = success otherwize MMOS/2 error. */
ULONG DLLENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len )
{ if (!Glue::initialized)
    return (ULONG)-1; // N/A
  return (*Glue::procs.output_playing_samples)( Glue::procs.A, info, buf, len );
}

/* Returns time in ms. */
T_TIME DLLENTRY out_playing_pos( void )
{ if (!Glue::initialized)
    return 0; // ??
  return (*Glue::procs.output_playing_pos)( Glue::procs.A );
}

/* if the output is playing. */
BOOL DLLENTRY out_playing_data( void )
{ if (!Glue::initialized)
    return FALSE;
  return (*Glue::procs.output_playing_data)( Glue::procs.A );
}

PROXYFUNCIMP(int DLLENTRY, Glue)
glue_request_buffer( void* a, const FORMAT_INFO2* format, short** buf )
{ // do not pass flush, signal DECEVENT_PLAYSTOP instead.
  if (buf == NULL)
  { if (InterlockedXch(Glue::playstopsent, TRUE) == FALSE)
      dec_event(Glue::ev_playstop);
    return 0; 
  }
  // pass request to output
  Glue::last_format = *format;
  return (*Glue::procs.output_request_buffer)(a, format, buf);
}

PROXYFUNCIMP(void DLLENTRY, Glue)
glue_commit_buffer( void* a, int len, T_TIME posmarker )
{ bool send_playstop = false;
  T_TIME pos_e = posmarker + (T_TIME)len / Glue::last_format.samplerate;
  
  // check stop offset
  if (pos_e >= Glue::stoptime)
  { pos_e = Glue::stoptime;
    // calcutate fractional part
    len = posmarker >= Glue::stoptime
      ? 0 // begin is already beyond the limit => cancel request
      : (int)((Glue::stoptime - posmarker) * Glue::last_format.samplerate +.5);
    // Signal DECEVENT_PLAYSTOP if not yet sent.
    send_playstop = InterlockedXch(Glue::playstopsent, TRUE) == FALSE;
  }

  // update min/max
  if (Glue::minpos > posmarker)
    Glue::minpos = posmarker;
  if (Glue::maxpos < pos_e)
    Glue::maxpos = pos_e;

  // pass to the output
  (*Glue::procs.output_commit_buffer)(a, len, posmarker + Glue::posoffset);

  // Signal DECEVENT_PLAYSTOP ?
  if (send_playstop)
    dec_event(Glue::ev_playstop); 
}

PROXYFUNCIMP(void DLLENTRY, Glue)
dec_event_handler( void* a, DECEVENTTYPE event, void* param )
{ DEBUGLOG(("plugman:dec_event_handler(%p, %d, %p)\n", a, event, param));
  // We handle some event here immediately.
  switch (event)
  {case DEVEVENT_CHANGETECH:
    if (Glue::url)
    { int_ptr<Playable> pp = Playable::FindByURL(Glue::url);
      if (pp)
        pp->SetTechInfo((TECH_INFO*)param);
    }
    break;
    
   case DECEVENT_CHANGEMETA:
    // Well,the backend does not support time dependant metadata.
    // From the playlist view, metadata changes should be immediately visible.
    // But during playback the change should be delayed until the apropriate buffer is really played.
    // The latter cannot be implemented with the current backend.
    if (Glue::url)
    { int_ptr<Playable> pp = Playable::FindByURL(Glue::url);
      if (pp)
        pp->SetMetaInfo((META_INFO*)param);
    }
    break;

   case DECEVENT_PLAYSTOP:
    Glue::stoptime = 0;
    // discard if already sent
    if (InterlockedXch(Glue::playstopsent, TRUE))
      return;
   default: // avoid warnings
    break;
  }   
  const dec_event_args args = { event, param };
  dec_event(args); 
}

/* proxy, because the decoder is not interested in OUTEVENT_END_OF_DATA */
PROXYFUNCIMP(void DLLENTRY, Glue)
out_event_handler( void* w, OUTEVENTTYPE event )
{ DEBUGLOG(("plugman:out_event_handler(%p, %d)\n", w, event));
  out_event(event); 
  // route high/low water events to the decoder (if any)
  switch (event)
  {case OUTEVENT_LOW_WATER:
   case OUTEVENT_HIGH_WATER:
    { Decoder* dp = (Decoder*)Decoders.Current();
      if (dp != NULL && dp->GetProcs().W != NULL)
        dp->GetProcs().decoder_event(dp->GetProcs().W, event);
      break;
    }
   default: // avoid warnings
    break;
  }
}


/* Returns the decoder NAME that can play this file and returns 0
   if not returns error 200 = nothing can play that. */
ULONG DLLENTRY dec_fileinfo( const char* url, INFOTYPE* what, DECODER_INFO2* info, char* name, size_t name_size )
{
  DEBUGLOG(("dec_fileinfo(%s, %x, %p{%u,%p,%p,%p}, %s)\n", url, *what, info, info->size, info->format, info->tech, info->meta, name));
  bool* checked = (bool*)alloca(sizeof(bool) * Decoders.size());
  const Decoder* dp;
  INFOTYPE what2;
  ULONG rc;
  int i;

  memset(checked, 0, sizeof *checked * Decoders.size());

  int type_mask;
  if (is_cdda(url))
    type_mask = DECODER_TRACK;
  else if (strnicmp("file:", url, 5) == 0)
    type_mask = DECODER_FILENAME;
  else if (strnicmp("http:", url, 5) == 0 || strnicmp("https:", url, 6) == 0 || strnicmp("ftp:", url, 4) == 0)
    type_mask = DECODER_URL;
  else
    // Always try URL too.
    type_mask = DECODER_URL|DECODER_OTHER;

  // First checks decoders supporting the specified type of files.
  for (i = 0; i < Decoders.size(); i++)
  { dp = (Decoder*)Decoders[i];
    DEBUGLOG(("dec_fileinfo: %s -> %u %x/%x %u\n", sfnameext2(dp->GetModuleName().cdata()),
      dp->GetEnabled(), dp->GetObjectTypes(), type_mask, dp->IsFileSupported(url)));
    if (dp->GetEnabled() && (dp->GetObjectTypes() & type_mask))
    { if (!dp->IsFileSupported(url))
        continue;
      what2 = *what;
      rc = (*dp->GetProcs().decoder_fileinfo)(url, &what2, info);
      if (rc != PLUGIN_NO_PLAY)
        goto ok;
    }
    checked[i] = TRUE;
  }

  // Next checks the rest decoders.
  for (i = 0; i < Decoders.size(); i++)
  { if (checked[i])
      continue;
    dp = (Decoder*)Decoders[i];
    if (!dp->TryOthers)
      continue;
    what2 = *what;
    rc = (*dp->GetProcs().decoder_fileinfo)(url, &what2, info);
    if (rc != PLUGIN_NO_PLAY)
      goto ok;
  }

  // No decoder found
  strcpy(info->tech->info, "Cannot find a decoder that supports this item.");
  *what = INFO_ALL; // Even in case of an error the requested information is in fact available.
  return PLUGIN_NO_PLAY;
 ok:
  DEBUGLOG(("dec_fileinfo: {{%d, %d, %d}, {%d, %lf, %d, %lf, %s}, {%d, %lf, %d}, {%d, %d, %d}} -> %s, %x\n",
    info->format->size, info->format->samplerate, info->format->channels,
    info->tech->size, info->tech->songlength, info->tech->bitrate, info->tech->totalsize, info->tech->info,
    info->phys->size, info->phys->filesize, info->phys->num_items,
    info->rpl->size, info->rpl->total_items, info->rpl->recursive,
    dp->GetModuleName().cdata(), what2));
  ASSERT((*what & ~what2) == 0); // The decoder must not reset bits.
  *what = (INFOTYPE)(what2 | *what); // do not reset bits
  if (name)
    sfnameext( name, dp->GetModuleName(), name_size );
  return rc;
}

ULONG DLLENTRY dec_cdinfo( const char *drive, DECODER_CDINFO *info )
{ DEBUGLOG(("dec_cdinfo(%s, %p)\n", drive, info));
  ULONG last_rc = 200;
  int i;

  for( i = 0; i < Decoders.size(); i++ )
  { const Decoder& dec = (const Decoder&)*Decoders[i];
    if ( dec.GetEnabled()
      && (dec.GetType() & DECODER_TRACK)
      && dec.GetProcs().decoder_cdinfo != NULL )
    { last_rc = (*dec.GetProcs().decoder_cdinfo)(drive, info);
      if (last_rc == 0)
        break;
    }
  }
  return last_rc; // returns only the last RC ... hum
}

ULONG DLLENTRY dec_status( void )
{
  const Decoder* dp = (Decoder*)Decoders.Current();
  // TODO: this check is still not 100% thread safe
  if (dp != NULL && dp->GetProcs().W != NULL)
    return (*dp->GetProcs().decoder_status)( dp->GetProcs().W );
  else
    return DECODER_ERROR;
}

/* Length in ms, should still be valid if decoder stops. */
T_TIME DLLENTRY dec_length( void )
{
  const Decoder* dp = (Decoder*)Decoders.Current();
  // TODO: this check is still not 100% thread safe
  if (dp != NULL && dp->GetProcs().W != NULL)
    return dp->GetProcs().decoder_length( dp->GetProcs().W );
  else
    return 0; // bah??
}

// merge semicolon separated entries into a stringset
static void merge_ssv_list(stringset& dest, const char* entries)
{ if (entries == NULL)
    return;
  for (;;)
  { const char* cp = strchr(entries, ';');
    xstring val(entries, cp ? cp-entries : strlen(entries));
    DEBUGLOG(("glue:merge_ssv_list: %s\n", val.cdata()));
    strkey*& elem = dest.get(val);
    if (elem == NULL)
      elem = new strkey(val); // new entry
    if (cp == NULL)
      return;
    entries = cp+1;
  }
}

void dec_merge_extensions(stringset& list)
{ DEBUGLOG(("dec_merge_extensions()\n"));
  for (size_t i = 0; i < Decoders.size(); ++i)
  { const Decoder* dp = (const Decoder*)Decoders[i];
    if (dp->GetEnabled())
      merge_ssv_list(list, dp->GetExtensions());
  }
}

void dec_merge_file_types(stringset& list)
{ DEBUGLOG(("dec_merge_file_types()\n"));
  for (size_t i = 0; i < Decoders.size(); ++i)
  { const Decoder* dp = (const Decoder*)Decoders[i];
    if (dp->GetEnabled())
      merge_ssv_list(list, dp->GetFileTypes());
  }
}

ULONG
dec_editmeta( HWND owner, const char* url, const char* decoder_name )
{ DEBUGLOG(("dec_editmeta(%x, %s, %s)\n", owner, url, decoder_name));
  ULONG rc;
  // find decoder
  int i = Decoders.find(decoder_name);
  if (i == -1)
  { rc = 100;
  } else
  // get entry points
  { Decoder* dec = (Decoder*)Decoders[i];
    const DecoderProcs& procs = dec->GetProcs();
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


static BOOL vis_init_core(Visual* vis, int id)
{
  if (!vis->GetEnabled() || vis->IsInitialized())
    return FALSE;

  PLUGIN_PROCS  procs;
  procs.output_playing_samples = out_playing_samples;
  procs.decoder_playing        = out_playing_data;
  procs.output_playing_pos     = out_playing_pos;
  procs.decoder_status         = dec_status;
  procs.decoder_fileinfo       = dec_fileinfo;
  procs.pm123_getstring        = pm123_getstring;
  procs.pm123_control          = pm123_control;
  procs.decoder_cdinfo         = dec_cdinfo;
  procs.decoder_length         = dec_length;

  return vis->Initialize(amp_player_window(), &procs, id);
}

/* Initializes the specified visual plug-in. */
BOOL vis_init( int i )
{ DEBUGLOG(("plugman:vis_init(%d)\n", i));

  if (i < 0 || i >= Visuals.size())
    return FALSE;

  return vis_init_core((Visual*)Visuals[i], i);
}


void vis_init_all( BOOL skin )
{ PluginList& list = skin ? VisualsSkinned : Visuals;
  for( int i = 0; i < list.size(); i++ )
    if( list[i]->GetEnabled() )
      vis_init_core((Visual*)list[i], i + 16*skin);
}

/* Terminates the specified visual plug-in. */
BOOL vis_deinit( int i )
{ if ( i < 0 || i >= Visuals.size() )
    return FALSE;
  return Visuals[i]->UninitPlugin();
}

void vis_deinit_all( BOOL skin )
{ PluginList& list = skin ? VisualsSkinned : Visuals;
  for( int i = 0; i < list.size(); i++ )
    list[i]->UninitPlugin();
}

/* Broadcats specified message to all enabled visual plug-ins. */
void vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  int  i;

  for( i = 0; i < Visuals.size(); i++ ) {
    Visual* vis = (Visual*)Visuals[i];
    if( vis->GetEnabled() && vis->IsInitialized() )
      WinSendMsg( vis->GetHwnd(), msg, mp1, mp2 );
  }
  for( i = 0; i < VisualsSkinned.size(); i++ ) {
    Visual* vis = (Visual*)VisualsSkinned[i];
    if( vis->GetEnabled() && vis->IsInitialized() )
      WinSendMsg( vis->GetHwnd(), msg, mp1, mp2 );
  }
}

