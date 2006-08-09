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
#include "plugman.h"
#include "pm123.h"


#define  DEBUG 1


DECODER* decoders       = NULL;
int      num_decoders   = 0;
int      active_decoder = -1;
OUTPUT*  outputs        = NULL;
int      num_outputs    = 0;
int      active_output  = -1;
FILTER*  filters        = NULL;
int      num_filters    = 0;
VISUAL*  visuals        = NULL;
int      num_visuals    = 0;

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

/* search a decoder plug-in by it's internal pointer */
static DECODER* find_decoder_by_pointer(void* w)
{ DECODER* pp = decoders + num_decoders;
  while (pp-- != decoders)
    if (pp->w == w)
      return pp;
  return NULL; // not found
}

/* search a output plug-in by it's internal pointer */
static OUTPUT* find_output_by_pointer(void* a)
{ OUTPUT* pp = outputs + num_outputs;
  while (pp-- != outputs)
    if (pp->a == a)
      return pp;
  return NULL; // not found
}

/* search a filter plug-in by it's internal pointer */
static FILTER* find_filter_by_pointer(void* f)
{ FILTER* pp = filters + num_filters;
  while (pp-- != filters)
    if (pp->f == f)
      return pp;
  return NULL; // not found
}

/* Loads a plug-in dynamic link module. */
static BOOL
load_module( const char* module_name, HMODULE* module )
{
  char error[1024] = "";
  APIRET rc = DosLoadModule( error, sizeof( error ), (PSZ)module_name, module );

  if( rc != NO_ERROR ) {
    amp_player_error( "Could not load %s\n%s",
                       module_name, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  }

  return TRUE;
}

/* Unloads a plug-in dynamic link module. */
static BOOL
unload_module( const char* module_name, HMODULE module )
{
  char  error[1024];
  ULONG rc = DosFreeModule( module );

  if( rc != NO_ERROR && rc != ERROR_INVALID_ACCESS ) {
    amp_player_error( "Could not unload %s\n%s",
                      module_name, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Assigns the address of the specified procedure within a plug-in. */
static BOOL
load_function( HMODULE module, void* function, const char* function_name,
                                               const char* module_name )
{
  char  error[1024];
  ULONG rc = DosQueryProcAddr( module, 0L, (PSZ)function_name, function );

  if( rc != NO_ERROR ) {
    *((ULONG*)function) = 0;
    amp_player_error( "Could not load \"%s\" from %s\n%s", function_name,
                      module_name, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Fills the basic properties of any plug-in.
   info: Pointer to a PLUGIN_BASE descriptor.
         Any plug-in descriptor is castable to the polymorphic base type PLUGIN_BASE.
         info->module:      input
         info->module_name: input
         info->enabled:     unused
         info->query_param  output
         info->plugin_query output
         info->plugin_configure output
   return FALSE = error */
static BOOL
load_plugin( PLUGIN_BASE* info )
{
  void (PM123_ENTRYP plugin_query)( PLUGIN_QUERYPARAM *param );
  
  #ifdef DEBUG
  fprintf(stderr, "load_plugin(%p->{%p, %s})\n", info, info->module, info->module_name);
  #endif

  if ( !load_function( info->module, &plugin_query, "plugin_query", info->module_name ) )
    return FALSE;

  info->query_param.interface = 0; // unchanged by old plug-ins 
  (*plugin_query)(&info->query_param);

  if (info->query_param.interface > MAX_PLUGIN_LEVEL)
  { 
    #define toconststring(x) #x
    amp_player_error( "Could not load plug-in %s because it requires a newer version of the PM123 core\n"
                      "Requested interface revision: %d, max. supported: " toconststring(MAX_PLUGIN_LEVEL),
      info->module_name, info->query_param.interface);
    return FALSE;
  }

  return !info->query_param.configurable
    || load_function( info->module, &info->plugin_configure, "plugin_configure", info->module_name );
}

/* Assigns the addresses of the decoder plug-in procedures. */
static BOOL
load_decoder( DECODER* info )
{
  HMODULE module = info->module;
  char*   module_name = info->module_name;
  char*   support[20];
  int     size = sizeof( support ) / sizeof( *support );
  int     i;
  BOOL    rc;

  if ( !load_plugin((PLUGIN_BASE*)info)
    || !(info->query_param.type & PLUGIN_DECODER)
    || !load_function( module, &info->decoder_init, "decoder_init", module_name )
    || !load_function( module, &info->decoder_uninit, "decoder_uninit", module_name )
    || !load_function( module, &info->decoder_command, "decoder_command", module_name )
    || !load_function( module, &info->decoder_status, "decoder_status", module_name )
    || !load_function( module, &info->decoder_length, "decoder_length", module_name )
    || !load_function( module, &info->decoder_fileinfo, "decoder_fileinfo", module_name )
    || !load_function( module, &info->decoder_trackinfo, "decoder_trackinfo", module_name )
    || !load_function( module, &info->decoder_cdinfo, "decoder_cdinfo", module_name )
    || !load_function( module, &info->decoder_support, "decoder_support", module_name ) )
    return FALSE;

  info->w = NULL;
  info->enabled = TRUE;

  for( i = 0; i < size; i++ ) {
    support[i] = malloc( _MAX_EXT );
    if( !support[i] ) {
      amp_player_error( "Not enough memory to decoder load." );
      return FALSE;
    }
  }

  info->decoder_support( support, &size );
  info->support = malloc(( size + 1 ) * sizeof( char* ));

  for( i = 0; i < size; i++ ) {
    info->support[i] = strdup( strupr( support[i] ));
    if( !info->support[i] ) {
      amp_player_error( "Not enough memory to decoder load." );
      return FALSE;
    }
  }

  info->support[i] = NULL;

  for( i = 0; i < sizeof( support ) / sizeof( *support ); i++ ) {
    free( support[i] );
  }

  return TRUE;
}

/* virtualization of level 1 output plug-ins */
// we can make these static, because no more than one output plug-in may be active at a time.
static char        proxy_output_buffer[BUFSIZE];
static int         proxy_output_len;
static FORMAT_INFO proxy_output_format;
static int         proxy_output_posmarker;
static BOOL        proxy_output_always_hungry;
static HWND        proxy_output_hwnd;

static ULONG PM123_ENTRY
proxy_output_command( void* a, ULONG msg, OUTPUT_PARAMS2* info )
{ OUTPUT* op = find_output_by_pointer(a);
  #ifdef DEBUG
  fprintf(stderr, "proxy_output_command(%p = [%p {%s}], %d, %p)\n",
    a, op, op != NULL ? op->module_name : "", msg, info);
  #endif
    
  if (op == NULL)
  { amp_player_error( "Internal error: can't identify output plugin by pointer (%p).", a );
    return 0;
  }

  if (info != NULL)
  { int r;
    OUTPUT_PARAMS params;
    params.size            = sizeof params;
    params.buffersize      = BUFSIZE;
    params.boostclass      = 4; // sorry, we should not lockup the system with time-critical priority
    params.normalclass     = 2;
    params.boostdelta      = 20;
    params.normaldelta     = 20;
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
    { proxy_output_always_hungry = params.always_hungry;
      proxy_output_hwnd = info->hwnd;
    }
    return r;
  } else
    return (*op->voutput_command)(a, msg, NULL); // sometimes info is NULL
}

static int PM123_ENTRY
proxy_output_request_buffer( void* a, const FORMAT_INFO* format, char** buf, int len,
                             int posmarker, const char* uri )
{ 
  #ifdef DEBUG
  fprintf(stderr, "proxy_output_request_buffer(%p, {%i,%i,%i,%i,%x}, %p, %i, %i, %s)\n",
    a, format->size, format->samplerate, format->channels, format->bits, format->format,
    buf, len, posmarker, uri);
  #endif
  
  if (len == 0)
  { if (proxy_output_always_hungry)
      // TODO: we should call the eventhandler here
      WinPostMsg(proxy_output_hwnd,WM_OUTPUT_OUTOFDATA,0,0);
    return 0;
  }

  *buf = proxy_output_buffer;
  proxy_output_format = *format;
  proxy_output_posmarker = posmarker;
  return proxy_output_len = min(len, BUFSIZE);
}

static int PM123_ENTRY
proxy_output_commit_buffer( void* a, int len )
{ OUTPUT* op = find_output_by_pointer(a);

  #ifdef DEBUG
  fprintf(stderr, "proxy_output_commit_buffer(%p = [%p {%s}], %i)\n",
    a, op, op != NULL ? op->module_name : "", len);
  #endif

  if (op == NULL)
  { amp_player_error( "Internal error: can't identify output plugin by pointer (%p).", a );
    return 0;
  }
  return (*op->voutput_play_samples)(a, &proxy_output_format, proxy_output_buffer, proxy_output_len, proxy_output_posmarker);
}

/* Assigns the addresses of the out7put plug-in procedures. */
static BOOL
load_output( OUTPUT* info )
{
  HMODULE module = info->module;
  char*   module_name = info->module_name;

  if ( !load_plugin((PLUGIN_BASE*)info)
    || !(info->query_param.type & PLUGIN_OUTPUT)
    || !load_function( module, &info->output_init, "output_init", module_name )
    || !load_function( module, &info->output_uninit, "output_uninit", module_name )
    || !load_function( module, &info->output_playing_samples, "output_playing_samples", module_name )
    || !load_function( module, &info->output_playing_pos, "output_playing_pos", module_name )
    || !load_function( module, &info->output_playing_data, "output_playing_data", module_name ) )
    return FALSE;

  /* check whether wee need to invoke an old plug-in via a proxy. */
  if (info->query_param.interface < 2)
  { // setup proxy
    if ( !load_function( module, &info->voutput_command, "output_command", module_name )
      || !load_function( module, &info->voutput_play_samples, "output_play_samples", module_name ) )
      return FALSE;
    info->output_command = &proxy_output_command;
    info->output_request_buffer = &proxy_output_request_buffer;
    info->output_commit_buffer = &proxy_output_commit_buffer;
      
  } else
  { // no proxy
    if ( !load_function( module, &info->output_command, "output_command", module_name )
      || !load_function( module, &info->output_request_buffer, "output_request_buffer", module_name )
      || !load_function( module, &info->output_commit_buffer, "output_commit_buffer", module_name ) )
      return FALSE;
  }

  info->a = NULL;
  return TRUE;
}

/* Assigns the addresses of the filter plug-in procedures. */
static BOOL
load_filter( FILTER* info )
{
  HMODULE module = info->module;
  char*   module_name = info->module_name;

  if ( !load_plugin((PLUGIN_BASE*)info)
    || !(info->query_param.type & PLUGIN_FILTER)
    || !load_function( module, &info->filter_init, "filter_init", module_name )
    || !load_function( module, &info->filter_uninit, "filter_uninit", module_name )
    || !load_function( module, &info->filter_play_samples, "filter_play_samples", module_name ) )
    return FALSE;

  /* check whether wee need to invoke an old plug-in via a proxy. */
  if (info->query_param.interface < 2)
  { // use proxy
    /* TODO!!!
    BOOL rc = load_function( module, &info->filter_init, "filter_init", module_name )
            | load_function( module, &info->filter_uninit, "filter_uninit", module_name )
            | load_function( module, &info->filter_play_samples, "filter_play_samples", module_name )
            | load_function( module, &info->plugin_query, "plugin_query", info->module_name );
    */
  }

  info->f = NULL;
  info->enabled = TRUE;
  return TRUE;
}

/* Assigns the addresses of the visual plug-in procedures. */
static BOOL
load_visual( VISUAL* info )
{
  HMODULE module = info->module;
  char*   module_name = info->module_name;

  if ( !load_plugin((PLUGIN_BASE*)info)
    || !(info->query_param.type & PLUGIN_VISUAL)
    || !load_function( module, &info->plugin_deinit, "plugin_deinit", module_name )
    || !load_function( module, &info->plugin_init, "vis_init", module_name ) )
    return FALSE;
    
  if (info->query_param.interface == 0)
  { amp_player_error( "Could not load visual plug-in %s because it is designed for PM123 before vesion 1.32\n"
                      "Please get a newer version of this plug-in which supports at least interface revision 1.",
      info->module_name);
    return FALSE;
  }

  info->enabled = TRUE;
  info->init = FALSE;
  info->hwnd = NULLHANDLE;
  return TRUE;
}

/* Loads and adds the specified decoder plug-in to the list of loaded. */
static BOOL
add_decoder_plugin( DECODER* decoder )
{
  int i;

  for( i = 0; i < num_decoders; i++ ) {
    if( decoders[i].module == decoder->module ) {
      return FALSE;
    }
  }

  if( load_decoder( decoder )) {
    ++num_decoders;
    decoders = realloc( decoders, num_decoders * sizeof( DECODER ));
    decoders[num_decoders-1] = *decoder;
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Loads and adds the specified output plug-in to the list of loaded. */
static BOOL
add_output_plugin( OUTPUT* output )
{
  int i;

  for( i = 0; i < num_outputs; i++ ) {
    if( outputs[i].module == output->module ) {
      return FALSE;
    }
  }

  if( load_output( output )) {
    ++num_outputs;
    outputs = realloc( outputs, num_outputs * sizeof( OUTPUT ));
    outputs[num_outputs-1] = *output;

    if( active_output == -1 ) {
      out_set_active( num_outputs - 1 );
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Loads and adds the specified filter plug-in to the list of loaded. */
static BOOL
add_filter_plugin( FILTER* filter )
{
  int i;

  for( i = 0; i < num_filters; i++ ) {
    if( filters[i].module == filter->module ) {
      return FALSE;
    }
  }

  if( load_filter( filter )) {
    ++num_filters;
    filters = realloc( filters, num_filters * sizeof( FILTER ));
    filters[num_filters-1] = *filter;
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Loads and adds the specified visual plug-in to the list of loaded. */
static BOOL
add_visual_plugin( VISUAL* visual )
{
  int i;

  for( i = 0; i < num_visuals; i++ ) {
    if( visuals[i].module == visual->module ) {
      if( visual->skin ) {
        visuals[i].x    = visual->x;
        visuals[i].y    = visual->y;
        visuals[i].cx   = visual->cx;
        visuals[i].cy   = visual->cy;
        visuals[i].skin = visual->skin;
      }
      return FALSE;
    }
  }

  if( load_visual( visual )) {
    ++num_visuals;
    visuals = realloc( visuals, num_visuals * sizeof( VISUAL ));
    visuals[num_visuals-1] = *visual;
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Unloads and removes the specified decoder plug-in from the list of loaded. */
BOOL
remove_decoder_plugin( DECODER* decoder )
{
  int  i, j;

  for( i = 0; i < num_decoders; i++ ) {
    if( &decoders[i] == decoder ) {
      break;
    }
  }
  if( i >= num_decoders ) {
    return FALSE;
  }

  if( i == active_decoder ) {
    dec_set_active( -1 );
  }

  if( decoders[i].support ) {
    for( j = 0; decoders[i].support[j]; j++ ) {
      free( decoders[i].support[j] );
    }
    free( decoders[i].support );
    decoders[i].support = NULL;
  }

  unload_module( decoders[i].module_name, decoders[i].module );

  if( i < num_decoders - 1 ) {
    memmove( &decoders[i], &decoders[i+1], ( num_decoders - i - 1 ) * sizeof( DECODER ));
  }

  --num_decoders;
  decoders = realloc( decoders, num_decoders * sizeof( DECODER ));
  return TRUE;
}

/* Unloads and removes the specified output plug-in from the list of loaded. */
BOOL
remove_output_plugin( OUTPUT* output )
{
  int  i;

  for( i = 0; i < num_outputs; i++ ) {
    if( &outputs[i] == output ) {
      break;
    }
  }
  if( i >= num_outputs ) {
    return FALSE;
  }

  if( active_output == i  ) {
    out_set_active( -1 );
  }
  if( active_output != -1 && i < active_output ) {
    --active_output;
  }

  unload_module( outputs[i].module_name, outputs[i].module );

  if( i < num_outputs - 1 ) {
    memmove( &outputs[i], &outputs[i+1], (num_outputs - i - 1 ) * sizeof( OUTPUT ));
  }

  --num_outputs;
  outputs = realloc( outputs, num_outputs * sizeof( OUTPUT ));
  return TRUE;
}

/* Unloads and removes the specified filter plug-in from the list of loaded. */
BOOL
remove_filter_plugin( FILTER* filter )
{
  int  i;

  for( i = 0; i < num_filters; i++ ) {
    if( &filters[i] == filter ) {
      break;
    }
  }
  if( i >= num_filters ) {
    return FALSE;
  }

  filter->filter_uninit( filter->f );
  filter->f = NULL;

  unload_module( filters[i].module_name, filters[i].module );

  if( i < num_filters - 1 ) {
    memmove( &filters[i], &filters[i+1], ( num_filters - i - 1 ) * sizeof( FILTER ));
  }

  --num_filters;
  filters = realloc( filters, num_filters * sizeof( FILTER ));
  return TRUE;
}

/* Unloads and removes the specified visual plug-in from the list of loaded. */
BOOL
remove_visual_plugin( VISUAL* visual )
{
  int  i;

  for( i = 0; i < num_visuals; i++ ) {
    if( &visuals[i] == visual ) {
      break;
    }
  }
  if( i >= num_visuals ) {
    return FALSE;
  }
  if( visuals[i].init ) {
    vis_deinit( i );
  }

  unload_module( visuals[i].module_name, visuals[i].module );

  if( i < num_visuals - 1 ) {
    memmove( &visuals[i], &visuals[i+1], ( num_visuals - i - 1 ) * sizeof( VISUAL ));
  }

  --num_visuals;
  visuals = realloc( visuals, num_visuals * sizeof( VISUAL ));
  return TRUE;
}

/* Adds a default decoder plug-ins to the list of loaded. */
void
load_default_decoders( void )
{
  DECODER decoder;

  while( num_decoders ) {
    remove_decoder_plugin( decoders );
  }

  strcpy( decoder.module_name, startpath );
  strcat( decoder.module_name, "mpg123.dll" );
  if( load_module( decoder.module_name, &decoder.module )) {
    add_decoder_plugin( &decoder );
  }
  strcpy( decoder.module_name, startpath );
  strcat( decoder.module_name, "wavplay.dll" );
  if( load_module( decoder.module_name, &decoder.module )) {
    add_decoder_plugin( &decoder );
  }
  strcpy( decoder.module_name, startpath );
  strcat( decoder.module_name, "cddaplay.dll" );
  if( load_module( decoder.module_name, &decoder.module )) {
    add_decoder_plugin( &decoder );
  }
}

/* Adds a default output plug-ins to the list of loaded. */
void
load_default_outputs( void )
{
  OUTPUT output;

  while( num_outputs ) {
    remove_output_plugin( outputs );
  }

  strcpy( output.module_name, startpath );
  strcat( output.module_name, "os2audio.dll" );
  if( load_module( output.module_name, &output.module )) {
    add_output_plugin( &output );
  }
  strcpy( output.module_name, startpath );
  strcat( output.module_name, "wavout.dll" );
  if( load_module( output.module_name, &output.module )) {
    add_output_plugin( &output );
  }

  out_set_active( 0 );
}

/* Adds a default filter plug-ins to the list of loaded. */
void
load_default_filters( void )
{
  FILTER filter;

  dec_set_active( -1 );
  while( num_filters ) {
    remove_filter_plugin( filters );
  }

  strcpy( filter.module_name, startpath );
  strcat( filter.module_name, "realeq.dll" );
  if( load_module( filter.module_name, &filter.module )) {
    add_filter_plugin( &filter );
  }
}

/* Adds a default visual plug-ins to the list of loaded. */
void
load_default_visuals( void )
{
  int i = 0;

  while( i < num_visuals ) {
    if( !visuals[i].skin ) {
      remove_visual_plugin( &visuals[i] );
    } else {
      ++i;
    }
  }
}

/* Restores the decoders list to the state was in when
   save_decoders was last called. */
BOOL
load_decoders( BUFSTREAM* b )
{
  int i, size, count;
  DECODER decoder;

  dec_set_active( -1 );
  while( num_decoders ) {
    remove_decoder_plugin( decoders );
  }

  if( read_bufstream( b, &count, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( read_bufstream( b, &decoder.module_name, size ) != size ) {
      return FALSE;
    }
    decoder.module_name[size] = 0;

    if( !load_module( decoder.module_name, &decoder.module ) ||
        !add_decoder_plugin( &decoder ) ||
         read_bufstream( b, &decoders[num_decoders-1].enabled, sizeof(int)) != sizeof(int))
    {
      return FALSE;
    }
  }
  return TRUE;
}

/* Restores the outputs list to the state was in when
   save_outputs was last called. */
BOOL
load_outputs( BUFSTREAM *b )
{
  int i, size, active, count;
  OUTPUT output;

  while( num_outputs ) {
    remove_output_plugin( outputs );
  }

  if( read_bufstream( b, &count, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( read_bufstream( b, &output.module_name, size ) != size ) {
      return FALSE;
    }
    output.module_name[size] = 0;

    if( !load_module( output.module_name, &output.module ) ||
        !add_output_plugin( &output ))
    {
      return FALSE;
    }
  }

  if( read_bufstream( b, &active, sizeof(int)) != sizeof(int)) {
    out_set_active( 0 );
  } else {
    out_set_active( active );
  }
  return TRUE;
}

/* Restores the filters list to the state was in when
   save_filters was last called. */
BOOL
load_filters( BUFSTREAM *b )
{
  int i, size, count;
  FILTER filter;

  while( num_filters ) {
    remove_filter_plugin( filters );
  }

  if( read_bufstream( b, &count, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( read_bufstream( b, &filter.module_name, size ) != size ) {
      return FALSE;
    }
    filter.module_name[size] = 0;

    if( !load_module( filter.module_name, &filter.module ) ||
        !add_filter_plugin( &filter ) ||
         read_bufstream( b, &filters[num_filters-1].enabled, sizeof(int)) != sizeof(int))
    {
      return FALSE;
    }
  }
  return TRUE;
}

/* Restores the visuals list to the state was in when
   save_visuals was last called. */
BOOL
load_visuals( BUFSTREAM *b )
{
  int i = 0;
  int count, size;
  VISUAL visual;

  while( i < num_visuals ) {
    if( !visuals[i].skin ) {
      remove_visual_plugin( &visuals[i] );
    } else {
      ++i;
    }
  }

  if( read_bufstream( b, &count, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( read_bufstream( b, &visual.module_name, size ) != size ) {
      return FALSE;
    }
    visual.module_name[size] = 0;
    visual.hwnd  = NULLHANDLE;
    visual.skin  = FALSE;
    visual.x     = 0;
    visual.y     = 0;
    visual.cx    = 0;
    visual.cy    = 0;
   *visual.param = 0;

    if( !load_module( visual.module_name, &visual.module ) ||
        !add_visual_plugin( &visual ) ||
         read_bufstream( b, &visuals[num_visuals-1].enabled, sizeof(int)) != sizeof(int))
    {
      return FALSE;
    }
  }
  return TRUE;
}

/* Saves the current list of decoders. */
BOOL
save_decoders( BUFSTREAM *b )
{
  int i, size;

  if( write_bufstream( b, &num_decoders, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < num_decoders; i++ ) {
    size = strlen( decoders[i].module_name );

    if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( write_bufstream( b, &decoders[i].module_name, size ) != size ) {
      return FALSE;
    }
    if( write_bufstream( b, &decoders[i].enabled, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
  }
  return TRUE;
}

/* Saves the current list of outputs plug-ins. */
BOOL
save_outputs( BUFSTREAM *b )
{
  int i, size;

  if( write_bufstream( b, &num_outputs, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < num_outputs; i++ ) {
    size = strlen( outputs[i].module_name );

    if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( write_bufstream( b, &outputs[i].module_name, size ) != size ) {
      return FALSE;
    }
  }

  if( write_bufstream( b, &active_output, sizeof(int)) != sizeof(int)) {
    return FALSE;
  } else {
    return FALSE;
  }
}

/* Saves the current list of filters plug-ins. */
BOOL
save_filters( BUFSTREAM *b )
{
  int i, size;

  if( write_bufstream( b, &num_filters, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < num_filters; i++ ) {
    size = strlen( filters[i].module_name );

    if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( write_bufstream( b, &filters[i].module_name, size ) != size ) {
      return FALSE;
    }
    if( write_bufstream( b, &filters[i].enabled, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
  }
  return TRUE;
}

/* Saves the current list of visuals plug-ins. */
BOOL
save_visuals( BUFSTREAM *b )
{
  int i, size;
  int count = 0;

  for( i = 0; i < num_visuals; i++ ) {
    if( !visuals[i].skin ) {
      ++count;
    }
  }

  if( write_bufstream( b, &count, sizeof(int)) != sizeof(int)) {
    return FALSE;
  }

  for( i = 0; i < num_visuals; i++ ) {
    if( !visuals[i].skin ) {
      size = strlen( visuals[i].module_name );

      if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
        return FALSE;
      }
      if( write_bufstream( b, &visuals[i].module_name, size ) != size ) {
        return FALSE;
      }
      if( write_bufstream( b, &visuals[i].enabled, sizeof(int)) != sizeof(int)) {
        return FALSE;
      }
    }
  }
  return TRUE;
}

/* Loads and adds the specified plug-in to the appropriate list of loaded.
   Returns the types found or 0. */
ULONG
add_plugin( const char* module_name, const VISUAL* data )
{
  PLUGIN_BASE plugin;
  strncpy(plugin.module_name, module_name, sizeof plugin.module_name);

  if ( !load_module( module_name, &plugin.module )
    || !load_plugin( &plugin ) )
    return 0;

  if( plugin.query_param.type & PLUGIN_VISUAL )
  {
    VISUAL visual;

    if( data ) {
      visual = *data;
    } else {
      memset( &visual, 0, sizeof( visual ));
    }

    memcpy(&visual, &plugin, sizeof plugin); // slicing

    if( !add_visual_plugin( &visual ))
      plugin.query_param.type &= ~PLUGIN_VISUAL;
  }
  if( plugin.query_param.type & PLUGIN_FILTER )
  {
    FILTER filter;
    memcpy(&filter, &plugin, sizeof plugin); // slicing

    if( !add_filter_plugin( &filter ))
      plugin.query_param.type &= ~PLUGIN_FILTER;
  }
  if( plugin.query_param.type & PLUGIN_DECODER )
  {
    DECODER decoder;
    memcpy(&decoder, &plugin, sizeof plugin); // slicing

    if( !add_decoder_plugin( &decoder ))
      plugin.query_param.type &= ~PLUGIN_DECODER;
  }
  if( plugin.query_param.type & PLUGIN_OUTPUT )
  {
    OUTPUT output;
    memcpy(&output, &plugin, sizeof plugin); // slicing

    if( !add_output_plugin( &output ))
      plugin.query_param.type &= ~PLUGIN_OUTPUT;
  }

  if( plugin.query_param.type == 0 )
    unload_module( module_name, plugin.module );

  return plugin.query_param.type;
}

/* Returns -2 if specified decoder is not enabled,
   returns -1 if a error occured, otherwize may returns the decoder's
   thread. */
int
dec_set_active( int number )
{
  int rc = 0;
  int current = active_decoder;

  if( number >= num_decoders || number < -1 ) {
    return -1;
  }

  if( current == number ) {
    return 0;
  }

  active_decoder = -1;

  if( current != -1 ) {
    decoders[current].decoder_uninit( decoders[current].w );
    decoders[current].w = NULL;
  }

  if( number != -1 ) {
    if( !decoders[number].enabled ) {
      rc = -2;
    } else {
      rc = decoders[number].decoder_init( &decoders[number].w );
      active_decoder = number;
    }
  }

  return rc;
}

/* Returns -1 if a error occured,
   returns -2 if can't find nothing,
   otherwize may returns the decoder's thread. */
int
dec_set_name_active( char* name )
{
  int i;
  if( name == NULL ) {
    return dec_set_active( -1 );
  }

  for( i = 0; i < num_decoders; i++ ) {
    char filename[_MAX_FNAME];
    sfnameext( filename, decoders[i].module_name, sizeof( filename ));
    if( stricmp( filename, name ) == 0 ) {
      return dec_set_active( i );
    }
  }
  return -2;
}

/* Proxy for interim use of the new output plugin interface
 * TODO: to be removed! 
 */
static int (PM123_ENTRYP _proxy_output_request_buffer)( void* a, const FORMAT_INFO* format, char** buf, int len, int posmarker, const char* uri );
static int (PM123_ENTRYP _proxy_output_commit_buffer)( void* a, int len );
static int PM123_ENTRY proxy_output_play_samples( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker )
{ int rem = len;

  #ifdef DEBUG
  fprintf(stderr, "proxy_output_play_samples(%p, %p, %p, %i, %i)\n", a, format, buf, len, posmarker);
  #endif

  while (rem)
  { char* dest;
    int l = (*_proxy_output_request_buffer)(a, format, &dest, rem, posmarker, NULL); // TODO: URI
    #ifdef DEBUG
    fprintf(stderr, "proxy_output_play_samples: now at %p %i %i\n", buf, l, rem);
    #endif
    if (l == 0)
      return len - rem; // error
    memcpy(dest, buf, l);
    (*_proxy_output_commit_buffer)(a, l); // Well, normally posmarker should be interpolated...
    rem -= l;
    buf += l; 
  }
  return len;
}

/* returns 0 = ok,
           1 = command unsupported,
           3 = no decoder active,
           4 = no active output, others unimportant. */
ULONG PM123_ENTRY dec_command( ULONG msg, DECODER_PARAMS *params )
{
  if( active_decoder != -1 )
  {
    // Is it good to setup output_play_samples and filter chain only here?
    if( msg == DECODER_SETUP )
    {
      if( active_output == -1 )
      {
        amp_player_error( "There is no active output." );
        return 4;
      }
      else
      {
        FILTER **enabled_filters = (FILTER**)alloca( num_filters * sizeof( FILTER* ));
        int num_enabled_filters = 0;
        int i = 0;

        for( i = 0; i < num_filters; i++ ) {
          if( filters[i].enabled ) {
            enabled_filters[num_enabled_filters++] = &filters[i];
          }
        }

        if( num_enabled_filters )
        {
          FILTER_PARAMS filter_params;
          i = num_enabled_filters-1;

          // You need to start from the last filter to init them all properly
          if( enabled_filters[i]->f != NULL )
          {
            enabled_filters[i]->filter_uninit( enabled_filters[i]->f );
            enabled_filters[i]->f = NULL;
          }

          filter_params.error_display = params->error_display;
          filter_params.info_display = params->error_display;
          _proxy_output_request_buffer = outputs[active_output].output_request_buffer;
          _proxy_output_commit_buffer = outputs[active_output].output_commit_buffer;
          filter_params.output_play_samples = &proxy_output_play_samples;
          filter_params.a = outputs[active_output].a;
          filter_params.audio_buffersize = params->audio_buffersize;

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
            filter_params.output_play_samples = enabled_filters[i+1]->filter_play_samples;
            filter_params.a = enabled_filters[i+1]->f;
            filter_params.audio_buffersize = params->audio_buffersize;

            enabled_filters[i]->filter_init( &enabled_filters[i]->f, &filter_params );
          }

          params->output_play_samples = enabled_filters[0]->filter_play_samples;
          params->a = enabled_filters[0]->f;
        } else {
          _proxy_output_request_buffer = outputs[active_output].output_request_buffer;
          _proxy_output_commit_buffer = outputs[active_output].output_commit_buffer;
          params->output_play_samples = &proxy_output_play_samples;
          params->a = outputs[active_output].a;
        }
      }
    }
    return decoders[active_decoder].decoder_command( decoders[active_decoder].w, msg, params );
  } else {
    return 3; // no decoder active
  }
}

/* Returns the decoder NAME that can play this file and returns 0
   if not returns error 200 = nothing can play that. */
ULONG PM123_ENTRY
dec_fileinfo( char* filename, DECODER_INFO* info, char* name )
{
  BOOL* checked = malloc( sizeof( BOOL ) * num_decoders );
  int   i, j;

  memset( checked, 0, sizeof( BOOL ) * num_decoders );

  // First checks decoders supporting the specified type of files.
  for( i = 0; i < num_decoders; i++ ) {
    if( decoders[i].enabled && decoders[i].support ) {
      for( j = 0; decoders[i].support[j]; j++ ) {
        if( wildcardfit( decoders[i].support[j], filename )) {
          if( decoders[i].decoder_fileinfo ) {
            checked[i] = TRUE;
            if( decoders[i].decoder_fileinfo( filename, info ) == 0 ) {
              if( name ) {
                sfnameext( name, decoders[i].module_name, _MAX_FNAME );
              }
              free( checked );
              return 0;
            }
          }
          break;
        }
      }
    }
  }

  // Next checks the rest decoders.
  for( i = 0; i < num_decoders; i++ ) {
    if( decoders[i].enabled && decoders[i].decoder_fileinfo && !checked[i] ) {
      if( decoders[i].decoder_fileinfo( filename, info ) == 0 ) {
        if( name ) {
          sfnameext( name, decoders[i].module_name, _MAX_FNAME );
        }
        free( checked );
        return 0;
      }
    }
  }

  free( checked );
  return 200;
}

ULONG PM123_ENTRY
dec_trackinfo( char* drive, int track, DECODER_INFO* info, char* name )
{
  ULONG last_rc = 200;
  int i;

  for( i = 0; i < num_decoders; i++ )
  {
    if( decoders[i].enabled && decoders[i].decoder_trackinfo != NULL )
    {
      last_rc = decoders[i].decoder_trackinfo( drive, track, info );
      if( last_rc == 0 && name != NULL ) {
        sfnameext( name, decoders[i].module_name, _MAX_FNAME );
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

  for( i = 0; i < num_decoders; i++ )
  {
    if( decoders[i].enabled && decoders[i].decoder_cdinfo != NULL )
    {
      last_rc = decoders[i].decoder_cdinfo( drive, info );
      if( last_rc == 0 ) {
        return 0;
      }
    }
  }

  return last_rc; // returns only the last RC ... hum
}

ULONG PM123_ENTRY
dec_status( void )
{
  if( active_decoder != -1 ) {
    return decoders[active_decoder].decoder_status( decoders[active_decoder].w );
  } else {
    return DECODER_ERROR;
  }
}

/* Length in ms, should still be valid if decoder stops. */
ULONG PM123_ENTRY
dec_length( void )
{
  if( active_decoder != -1 ) {
    return decoders[active_decoder].decoder_length( decoders[active_decoder].w );
  } else {
    return 0; // bah??
  }
}

/* Fills specified buffer with the list of extensions of supported files. */
void
dec_fill_types( char* result, size_t size )
{
  int i, j;
  int res_size = 0;
  int add_size;

  *result = 0;

  for( i = 0; i < num_decoders; i++ ) {
    if( decoders[i].enabled && decoders[i].support ) {
      for( j = 0; decoders[i].support[j]; j++ )
      {
        add_size = strlen( decoders[i].support[j] ) + 1;
        if( res_size + add_size < size )
        {
          res_size += add_size;
          strcat( result, decoders[i].support[j] );
          strcat( result, ";" );
        }
      }
    }
  }

  if( res_size ) {
    result[ res_size - 1 ] = 0; // Remove the last ";".
  }
}

int
out_set_active( int number )
{
  if( number >= num_outputs || number < -1 ) {
    return -1;
  }

  if( active_output == number ) {
    return 0;
  }

  if( active_output != -1 )
  {
    outputs[active_output].output_command( outputs[active_output].a, OUTPUT_CLOSE, NULL );
    outputs[active_output].output_uninit( outputs[active_output].a );
    outputs[active_output].a = NULL;
  }

  active_output = number;

  if( active_output != -1 )
  {
    outputs[active_output].output_init( &outputs[active_output].a );
    return 0;
  } else {
    return 0;
  }
}

/* Returns -2 if can't find nothing. */
int
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
}

/* 0 = ok, else = return code from MMOS/2. */
ULONG
out_command( ULONG msg, OUTPUT_PARAMS2* ai )
{
  // TODO: setup callback handlers
  // TODO: virtualizing filters

  if( active_output != -1 ) {
    return outputs[active_output].output_command( outputs[active_output].a, msg, ai );
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
ULONG PM123_ENTRY
out_playing_samples( FORMAT_INFO* info, char* buf, int len )
{
  if( active_output != -1 ) {
    return outputs[active_output].output_playing_samples( outputs[active_output].a, info, buf, len );
  } else {
    return -1; // ??
  }
}

/* Returns time in ms. */
ULONG PM123_ENTRY
out_playing_pos( void )
{
  if( active_output != -1 ) {
    return outputs[active_output].output_playing_pos( outputs[active_output].a );
  } else {
    return 0; // ??
  }
}

/* if the output is playing. */
BOOL PM123_ENTRY
out_playing_data( void )
{
  if( active_output != -1 ) {
    return outputs[active_output].output_playing_data( outputs[active_output].a );
  } else {
    return FALSE;
  }
}

/* Initializes the specified visual plug-in. */
BOOL
vis_init( HWND hwnd, int i )
{
  PLUGIN_PROCS  procs;
  VISPLUGININIT visinit;

  if( i >= num_visuals || visuals[i].init ) {
    return FALSE;
  }

  procs.output_playing_samples = out_playing_samples;
  procs.decoder_playing        = decoder_playing;
  procs.output_playing_pos     = out_playing_pos;
  procs.decoder_status         = dec_status;
  procs.decoder_fileinfo       = dec_fileinfo;
  procs.pm123_getstring        = pm123_getstring;
  procs.pm123_control          = pm123_control;
  procs.decoder_trackinfo      = dec_trackinfo;
  procs.decoder_cdinfo         = dec_cdinfo;
  procs.decoder_length         = dec_length;

  visinit.x       = visuals[i].x;
  visinit.y       = visuals[i].y;
  visinit.cx      = visuals[i].cx;
  visinit.cy      = visuals[i].cy;
  visinit.hwnd    = hwnd;
  visinit.procs   = &procs;
  visinit.id      = i;
  visinit.param   = visuals[i].param;
  visinit.hab     = WinQueryAnchorBlock( hwnd );
  visuals[i].hwnd = (*visuals[i].plugin_init)(&visinit);

  if( visuals[i].hwnd != NULLHANDLE ) {
    visuals[i].init = TRUE;
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Terminates the specified visual plug-in. */
BOOL
vis_deinit( int i )
{
  if( i >= num_visuals || !visuals[i].init ) {
    return FALSE;
  } else {
    visuals[i].plugin_deinit(0);
    visuals[i].hwnd = NULLHANDLE;
    visuals[i].init = FALSE;
    return TRUE;
  }
}

/* Broadcats specified message to all enabled visual plug-ins. */
void
vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  int  i;

  for( i = 0; i < num_visuals; i++ ) {
    if( visuals[i].enabled && visuals[i].hwnd != NULLHANDLE ) {
      WinSendMsg( visuals[i].hwnd, msg, mp1, mp2 );
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

/* Returns a playing time of the current file, in seconds. */
int
time_played( void ) {
  return out_playing_pos()/1000;
}

/* Returns a total playing time of the current file. */
int
time_total( void ) {
  return dec_length()/1000;
}

void
load_plugin_menu( HWND hMenu )
{
  char     buffer[2048];
  char     file[_MAX_PATH];
  MENUITEM mi;
  int      i;
  int      count;
  int      num_plugins = num_decoders + num_outputs + num_filters + num_visuals;
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

  if( !num_plugins  )
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

  num_entries = num_plugins;
  entries = (PLUGIN_ENTRY*) malloc( num_entries * sizeof( PLUGIN_ENTRY ));

  // Visual plug-ins
  for( i = 0; i < num_visuals; i++ )
  {
    sprintf( buffer, "%s (%s)", visuals[i].query_param.desc,
             sfname( file, visuals[i].module_name, sizeof( file )));

    entries[next_entry].filename     = strdup( buffer );
    entries[next_entry].type         = PLUGIN_VISUAL;
    entries[next_entry].i            = i;
    entries[next_entry].configurable = visuals[i].query_param.configurable;;
    entries[next_entry].enabled      = visuals[i].enabled;
    next_entry++;
  }

  // Decoder plug-ins
  for( i = 0; i < num_decoders; i++ )
  {
    sprintf( buffer, "%s (%s)", decoders[i].query_param.desc,
             sfname( file, decoders[i].module_name, sizeof( file )));

    entries[next_entry].filename     = strdup( buffer );
    entries[next_entry].type         = PLUGIN_DECODER;
    entries[next_entry].i            = i;
    entries[next_entry].configurable = decoders[i].query_param.configurable;
    entries[next_entry].enabled      = decoders[i].enabled;
    next_entry++;
  }

  // Output plug-ins
  for( i = 0; i < num_outputs; i++ )
  {
    sprintf( buffer, "%s (%s)", outputs[i].query_param.desc,
             sfname( file, outputs[i].module_name, sizeof( file )));

    entries[next_entry].filename     = strdup( buffer );
    entries[next_entry].type         = PLUGIN_OUTPUT;
    entries[next_entry].i            = i;
    entries[next_entry].configurable = outputs[i].query_param.configurable;
    entries[next_entry].enabled      = (i == active_output ? TRUE : FALSE);
    next_entry++;
  }

  // Filter plug-ins
  for( i = 0; i < num_filters; i++ )
  {
    sprintf( buffer, "%s (%s)", filters[i].query_param.desc,
             sfname( file, filters[i].module_name, sizeof( file )));

    entries[next_entry].filename     = strdup( buffer );
    entries[next_entry].type         = PLUGIN_FILTER;
    entries[next_entry].i            = i;
    entries[next_entry].configurable = filters[i].query_param.configurable;
    entries[next_entry].enabled      = filters[i].enabled;
    next_entry++;
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
  {
    switch( entries[i].type )
    {
      case PLUGIN_VISUAL:
        visuals [ entries[i].i ].plugin_configure( hwnd, visuals [ entries[i].i ].module );
        break;
      case PLUGIN_DECODER:
        decoders[ entries[i].i ].plugin_configure( hwnd, decoders[ entries[i].i ].module );
        break;
      case PLUGIN_OUTPUT:
        outputs [ entries[i].i ].plugin_configure( hwnd, outputs [ entries[i].i ].module );
        break;
      case PLUGIN_FILTER:
        filters [ entries[i].i ].plugin_configure( hwnd, filters [ entries[i].i ].module );
        break;
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Unloads and removes all loaded plug-ins. */
void
remove_all_plugins( void )
{
  int i;

  dec_set_active( -1 );

  while( num_decoders ) {
    remove_decoder_plugin( decoders );
  }
  while( num_filters  ) {
    remove_filter_plugin ( filters  );
  }
  while( num_visuals  ) {
    remove_visual_plugin ( visuals  );
  }
  while( num_outputs  ) {
    remove_output_plugin ( outputs  );
  }

  for( i = 0; i < num_entries; i++ ) {
    free( entries[i].filename );
  }

  free( entries );
  entries  = NULL;
  num_entries = 0;
}
