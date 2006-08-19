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


#define  DEBUG 1

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
* plugin list modification functions
*
****************************************************************************/

/* returns the index of a new or an existing module in the modules list
 * or -1 if the module cannot be loaded.
 */
static int get_module(const char* name)
{ 
  #ifdef DEBUG
  fprintf(stderr, "plugman:get_module(%s)\n", name);
  #endif
  int p = plugins.find(name);
  if (p == -1)
  { p = plugins.count();
    CL_MODULE* pp = new CL_MODULE(name);
    if (!pp->load() || !plugins.append(pp))
    { delete pp;
      return -1;
    }
  }  
  #ifdef DEBUG
  fprintf(stderr, "plugman:get_module: %d\n", p);
  #endif
  return p;
}

static CL_PLUGIN* instantiate(CL_MODULE& mod, CL_PLUGIN* (*factory)(CL_MODULE& mod), CL_PLUGIN_LIST& list, BOOL enabled)
{ 
  #ifdef DEBUG
  fprintf(stderr, "plugman:instantiate(%p{%s}, %p, %p, %i)\n",
    &mod, mod.module_name, factory, &list, enabled); 
  #endif
  if (list.find(mod) != -1)
  {
    #ifdef DEBUG
    fprintf(stderr, "plugman:instantiate: trying to load plugin twice\n");
    #endif
    return NULL;
  }
  CL_PLUGIN* pp = (*factory)(mod);
  if (!pp->load_plugin() || !list.append(pp))
  { delete pp;
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
    fprintf(stderr, "plugman:add_plugin_core(%s, %p{%d,%d,%d,%d, %i, %s}, %x, %i)\n",
      name, data, data->x, data->y, data->cx, data->cy, data->skin, data->param, mask, enabled);
   else 
    fprintf(stderr, "plugman:add_plugin_core(%s, %p, %x, %i)\n", name, data, mask, enabled); 
  #endif
  /*// make absolute path
  char module_name[_MAX_PATH];
  if (strchr(name, ':') == NULL && name[0] != '\\' && name[0] != '/')
  { // relative path
    strlcpy(module_name, startpath, sizeof module_name);
    strlcat(module_name, name, sizeof module_name);
    name = module_name;
  }*/
  // load module
  int p = get_module(name);
  if (p == -1)
    return FALSE;
  // load as plugin
  int result = 0;
  CL_MODULE& module = plugins[p];
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
  if (!result)
    delete plugins.detach_request(p);
  #ifdef DEBUG
  fprintf(stderr, "plugman:add_plugin_core: %x\n", result);
  #endif
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
{ 
  #ifdef DEBUG
  fprintf(stderr, "plugman:remove_visual_plugins(%i)\n", skin);
  #endif
  int i = visuals.count();
  while( i-- )
    if( ((CL_VISUAL&)visuals[i]).get_properties().skin == skin )
      visuals.remove( i );
}

/* Adds a default decoder plug-ins to the list of loaded. */
void load_default_decoders( void )
{
  #ifdef DEBUG
  fprintf(stderr, "load_default_decoders()\n"); 
  #endif
  decoders.clear();
  fprintf(stderr, "ccc");

  add_plugin_core("mpg123.dll", NULL, PLUGIN_DECODER);
  add_plugin_core("wavplay.dll", NULL, PLUGIN_DECODER);
  add_plugin_core("cddaplay.dll", NULL, PLUGIN_DECODER);
}

/* Adds a default output plug-ins to the list of loaded. */
void load_default_outputs( void )
{
  #ifdef DEBUG
  fprintf(stderr, "load_default_outputs()\n"); 
  #endif
  outputs.clear();

  add_plugin_core("os2audio.dll", NULL, PLUGIN_OUTPUT);
  add_plugin_core("wavout.dll", NULL, PLUGIN_OUTPUT);

  // outputs.set_active(0); implicit at add_plugin
}

/* Adds a default filter plug-ins to the list of loaded. */
void load_default_filters( void )
{
  #ifdef DEBUG
  fprintf(stderr, "load_default_filters()\n"); 
  #endif
  decoders.set_active( -1 ); // why ever
  filters.clear();

  add_plugin_core("realeq.dll", NULL, PLUGIN_FILTER);
}

/* Adds a default visual plug-ins to the list of loaded. */
void load_default_visuals( void )
{
  #ifdef DEBUG
  fprintf(stderr, "load_default_visuals()\n"); 
  #endif
  remove_visual_plugins( FALSE );
}

static BOOL load_plugin( BUFSTREAM* b, int mask, BOOL withenabled )
{
  #ifdef DEBUG
  fprintf(stderr, "plugman:load_plugin(%p, %x, %i)\n", b, mask, withenabled); 
  #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "load_decoders(%p)\n", b); 
  #endif
  decoders.set_active( -1 );
  decoders.clear();

  return load_plugin(b, PLUGIN_DECODER, true);
}

/* Restores the outputs list to the state was in when
   save_outputs was last called. */
BOOL load_outputs( BUFSTREAM *b )
{
  #ifdef DEBUG
  fprintf(stderr, "load_outputs(%p)\n", b); 
  #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "load_filters(%p)\n", b); 
  #endif
  filters.clear();

  return load_plugin(b, PLUGIN_FILTER, true);
}

/* Restores the visuals list to the state was in when
   save_visuals was last called. */
BOOL load_visuals( BUFSTREAM *b )
{
  #ifdef DEBUG
  fprintf(stderr, "load_visuals(%p)\n", b); 
  #endif
  remove_visual_plugins( FALSE );

  return load_plugin(b, PLUGIN_VISUAL, true); 
}

static BOOL save_plugin( BUFSTREAM *b, const CL_PLUGIN& src, BOOL withenabled )
{
  #ifdef DEBUG
  fprintf(stderr, "plugman:save_plugin(%p, %p{%s}, %i)\n", b, &src, src.module_name, withenabled); 
  #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "save_decoders(%p)\n", b);
  #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "save_outputs(%p)\n", b);
  #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "save_filters(%p)\n", b);
  #endif
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
{
  #ifdef DEBUG
  fprintf(stderr, "save_visuals(%p)\n", b);
  #endif
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

/* Returns -2 if specified decoder is not enabled,
   returns -1 if a error occured,
   returns 0  if succesful. */
int dec_set_active( int number )
{ return decoders.set_active(number);
}

/* Returns -1 if a error occured,
   returns -2 if can't find nothing,
   returns 0  if succesful. */
int dec_set_name_active( char* name )
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

/* returns 0 = ok,
           1 = command unsupported,
           3 = no decoder active,
           4 = no active output, others unimportant. */
ULONG PM123_ENTRY dec_command( ULONG msg, DECODER_PARAMS2 *params )
{
  if( decoders.current() == NULL )
    return 3; // no decoder active
  const DECODER_PROCS& dec = ((CL_DECODER*)decoders.current())->get_procs();

  // Is it good to setup output_play_samples and filter chain only here?
  if( msg == DECODER_SETUP )
  {
    if( outputs.current() == NULL )
    { amp_player_error( "There is no active output." );
      return 4;
    }
    const OUTPUT_PROCS& out = ((CL_OUTPUT*)outputs.current())->get_procs();

    /*FILTER **enabled_filters = (FILTER**)alloca( num_filters * sizeof( FILTER* ));
    int num_enabled_filters = 0;
    int i = 0;

    for( i = 0; i < num_filters; i++ ) {
      if( filters[i].enabled ) {
        enabled_filters[num_enabled_filters++] = &filters[i];
      }
    }

    if( num_enabled_filters )
    {
      FILTER_PARAMS2 filter_params;
      i = num_enabled_filters-1;

      // You need to start from the last filter to init them all properly
      if( enabled_filters[i]->f != NULL )
      {
        enabled_filters[i]->filter_uninit( enabled_filters[i]->f );
        enabled_filters[i]->f = NULL;
      }

      filter_params.error_display = params->error_display;
      filter_params.info_display = params->error_display;
      filter_params.output_request_buffer = outputs[active_output].output_request_buffer;
      filter_params.output_commit_buffer = outputs[active_output].output_commit_buffer;
      //filter_params.output_play_samples = &proxy_output_play_samples;
      filter_params.a = outputs[active_output].a;

      enabled_filters[i]->filter_init( &enabled_filters[i]->f, &filter_params );

      for( i = num_enabled_filters - 2; i >= 0; i-- )
      {
        if( enabled_filters[i]->f != NULL )
        {
          enabled_filters[i]->filter_uninit( enabled_filters[i]->f );
          enabled_filters[i]->f = NULL;
        }

        filter_params.error_display = params->error_display;
        filter_params.info_display = params->info_display;
        //filter_params.output_play_samples = enabled_filters[i+1]->filter_play_samples;
        filter_params.a = enabled_filters[i+1]->f;

        enabled_filters[i]->filter_init( &enabled_filters[i]->f, &filter_params );
      }

      //params->output_play_samples = enabled_filters[0]->filter_play_samples;
      params->a = enabled_filters[0]->f;
    } else*/ {
      params->output_request_buffer = out.output_request_buffer;
      params->output_commit_buffer = out.output_commit_buffer;
      params->a = out.a;
    }
  }
  return dec.decoder_command(dec.w, msg, params);
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
dec_fileinfo( char* filename, DECODER_INFO* info, char* name )
{
  BOOL* checked = (BOOL*)_alloca( sizeof( BOOL ) * decoders.count() );
  int   i;

  memset( checked, 0, sizeof( BOOL ) * decoders.count() );
  
  // Prepend file://
  if (is_file(filename) && !is_url(filename))
  { char* fname = (char*)_alloca( strlen(filename) +8 );
    strcpy(fname, "file://");
    strcpy(fname+7, filename);
    filename = fname;
  }

  const CL_DECODER* dp;
  // First checks decoders supporting the specified type of files.
  for (i = 0; i < decoders.count(); i++)
  { dp = &(const CL_DECODER&)decoders[i];
    if (!dp->get_enabled() || !is_file_supported(dp->get_procs().support, filename))
      continue;
    checked[i] = TRUE;
    if (dp->get_procs().decoder_fileinfo(filename, info) == 0)
      goto ok;
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
  return 0;
}

ULONG PM123_ENTRY
dec_trackinfo( char* drive, int track, DECODER_INFO* info, char* name )
{
  ULONG last_rc = 200;
  int i;
  char cdda_url[20];
  
  sprintf(cdda_url, "cdda:///%s/track%02d", drive, track);

  for( i = 0; i < decoders.count(); i++ )
  { const CL_DECODER& dec = (const CL_DECODER&)decoders[i];
    if( dec.get_enabled() && (dec.get_procs().type & DECODER_TRACK) )
    { last_rc = dec.get_procs().decoder_fileinfo( cdda_url, info );
      #ifdef DEBUG
      fprintf(stderr, "dec_trackinfo: %s->decoder_fileinfo: %d\n", dec.module_name, last_rc); 
      #endif
      if( last_rc == 0 )
      { if ( name != NULL )
          sfnameext( name, dec.module_name, _MAX_FNAME );
        return 0;
      }
    }
  }
  return last_rc; // returns only the last RC ... hum
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
  int res_size = 0;
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

/* Returns -2 if can't find nothing. */
/*int
out_set_name_active( char *name )
{
  int i;

  if( name == NULL || !*name ) {
    return out_set_active( -1 );
  }

  for( i = 0; i < num_outputs; i++ )
  {
    char filename[256];
    sfnameext( filename, outputs[i].module_name, _MAX_FNAME );
    if( stricmp( filename, name ) == 0 ) {
      out_set_active( i );
      return 0;
    }
  }
  return -2;
}*/

/* 0 = ok, else = return code from MMOS/2. */
ULONG
out_command( ULONG msg, OUTPUT_PARAMS2* ai )
{
  // TODO: setup callback handlers
  // TODO: virtualizing filters
  
  CL_OUTPUT* op = (CL_OUTPUT*)outputs.current();

  if( op != NULL ) {
    return op->get_procs().output_command( op->get_procs().a, msg, ai );
  } else {
    amp_player_error( "There is no active output plug-in." );
    return -1;  // ??
  }
}

void
out_set_volume( int volume )
{
  OUTPUT_PARAMS2 out_params = { 0 };

  out_params.volume = volume;
  out_params.amplifier = 1.0;
  out_command( OUTPUT_VOLUME, &out_params );
}

/* Returns 0 = success otherwize MMOS/2 error. */
static ULONG PM123_ENTRY
out_playing_samples( FORMAT_INFO* info, char* buf, int len )
{ CL_OUTPUT* op = (CL_OUTPUT*)outputs.current();
  if ( op != NULL ) {
    return op->get_procs().output_playing_samples( op->get_procs().a, info, buf, len );
  } else {
    return -1; // ??
  }
}

/* Returns time in ms. */
ULONG PM123_ENTRY
out_playing_pos( void )
{ CL_OUTPUT* op = (CL_OUTPUT*)outputs.current();
  if ( op != NULL ) {                                             
    return op->get_procs().output_playing_pos( op->get_procs().a );
  } else {
    return 0; // ??
  }
}

/* if the output is playing. */
BOOL PM123_ENTRY
out_playing_data( void )
{ CL_OUTPUT* op = (CL_OUTPUT*)outputs.current();
  if ( op != NULL ) {
    return op->get_procs().output_playing_data( op->get_procs().a );
  } else {
    return FALSE;
  }
}

/* Initializes the specified visual plug-in. */
BOOL vis_init( int i )
{
  #ifdef DEBUG
  fprintf(stderr, "plugman:vis_init(%d)\n", i);
  #endif
  if (i < 0 || i >= visuals.count())
    return FALSE;

  CL_VISUAL& vis = (CL_VISUAL&)visuals[i];
  
  if (!vis.get_enabled() || vis.is_initialized())
    return FALSE;

  PLUGIN_PROCS  procs;
  VISPLUGININIT visinit;

  procs.output_playing_samples = out_playing_samples;
  procs.decoder_playing        = decoder_playing;
  procs.output_playing_pos     = out_playing_pos;
  procs.decoder_status         = dec_status;
  procs.decoder_fileinfo       = dec_fileinfo;
  procs.pm123_getstring        = pm123_getstring;
  procs.pm123_control          = pm123_control;
  procs.decoder_cdinfo         = dec_cdinfo;
  procs.decoder_length         = dec_length;

  visinit.x       = vis.get_properties().x;
  visinit.y       = vis.get_properties().y;
  visinit.cx      = vis.get_properties().cx;
  visinit.cy      = vis.get_properties().cy;
  visinit.hwnd    = amp_player_window();
  visinit.procs   = &procs;
  visinit.id      = i;
  visinit.param   = vis.get_properties().param;
  
  return vis.initialize(&visinit);
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
  int      next_entry  = 0;

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
  dec_set_active( -1 );

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
