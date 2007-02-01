/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pm123.h"
#include "utilfct.h"
#include "plugman.h"

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

/* Plug-in menu entry in the main pop-up menu */
typedef struct {

   char* filename;
   int   type;
   int   i;
   int   configurable;
   int   enabled;

} PLUGIN_ENTRY;

static PLUGIN_ENTRY *entries = NULL;
static int num_entries = 0;

static const char* default_decoders[] = { "mpg123.dll",   "wavplay.dll", "cddaplay.dll" };
static const char* default_outputs [] = { "os2audio.dll", "wavout.dll" };
static const char* default_filters [] = { "realeq.dll" };

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

/* Fills the query_param and returns the type of plug-ins contained
   in specified module. */
static ULONG
check_plugin( HMODULE module, const char* module_name, PLUGIN_QUERYPARAM* query_param )
{
  void (DLLENTRYP plugin_query)( PLUGIN_QUERYPARAM *param );

  if( load_function( module, &plugin_query, "plugin_query", module_name ))
  {
    plugin_query( query_param );
    return query_param->type;
  } else {
    return 0;
  }
}

/* Assigns the addresses of the decoder plug-in procedures. */
static BOOL
load_decoder( DECODER* info )
{
  HMODULE module = info->module;
  char*   module_file = info->module_file;
  int     i;

  if( check_plugin( module, module_file, &info->query_param ) & PLUGIN_DECODER )
  {
    BOOL rc =  load_function( module, &info->decoder_init, "decoder_init", module_file )
            && load_function( module, &info->decoder_uninit, "decoder_uninit", module_file )
            && load_function( module, &info->decoder_command, "decoder_command", module_file )
            && load_function( module, &info->decoder_status, "decoder_status", module_file )
            && load_function( module, &info->decoder_length, "decoder_length", module_file )
            && load_function( module, &info->decoder_fileinfo, "decoder_fileinfo", module_file )
            && load_function( module, &info->decoder_trackinfo, "decoder_trackinfo", module_file )
            && load_function( module, &info->decoder_cdinfo, "decoder_cdinfo", module_file )
            && load_function( module, &info->decoder_support, "decoder_support", module_file )
            && load_function( module, &info->plugin_query, "plugin_query", module_file );

    if( rc && info->query_param.configurable ) {
      rc = load_function( module, &info->plugin_configure, "plugin_configure", module_file );
    }

    if( rc )
    {
      char* ptrs[MAX_FILEEXT];

      info->w = NULL;
      info->enabled = TRUE;
      info->fileext_size = MAX_FILEEXT;

      memset( info->fileext, 0, sizeof( info->fileext ));

      for( i = 0; i < MAX_FILEEXT; i++ ) {
        ptrs[i] = info->fileext[i];
      }

      sfname( info->module_name, module_file, sizeof( info->module_name ));
      info->support = info->decoder_support( ptrs, &info->fileext_size );

      if( info->support & DECODER_METAINFO ) {
        rc = load_function( module, &info->decoder_saveinfo, "decoder_saveinfo", module_file );
      } else {
        info->decoder_saveinfo = NULL;
      }
    }
    return rc;
  }
  return FALSE;
}

/* Assigns the addresses of the output plug-in procedures. */
static BOOL
load_output( OUTPUT* info )
{
  HMODULE module = info->module;
  char*   module_file = info->module_file;

  if( check_plugin( module, module_file, &info->query_param ) & PLUGIN_OUTPUT )
  {
    BOOL rc =  load_function( module, &info->output_init, "output_init", module_file )
            && load_function( module, &info->output_uninit, "output_uninit", module_file )
            && load_function( module, &info->output_command, "output_command", module_file )
            && load_function( module, &info->output_play_samples, "output_play_samples", module_file )
            && load_function( module, &info->output_playing_samples, "output_playing_samples", module_file )
            && load_function( module, &info->output_playing_pos, "output_playing_pos", module_file )
            && load_function( module, &info->output_playing_data, "output_playing_data", module_file )
            && load_function( module, &info->plugin_query, "plugin_query", info->module_file );

    if( rc && info->query_param.configurable ) {
      rc = load_function( module, &info->plugin_configure, "plugin_configure", module_file );
    }
    if( rc ) {
      sfname( info->module_name, module_file, sizeof( info->module_name ));
    }

    info->a = NULL;
    return rc;
  }
  return FALSE;
}

/* Assigns the addresses of the filter plug-in procedures. */
static BOOL
load_filter( FILTER* info )
{
  HMODULE module = info->module;
  char*   module_file = info->module_file;

  if( check_plugin( module, module_file, &info->query_param ) & PLUGIN_FILTER )
  {
    BOOL rc =  load_function( module, &info->filter_init, "filter_init", module_file )
            && load_function( module, &info->filter_uninit, "filter_uninit", module_file )
            && load_function( module, &info->filter_play_samples, "filter_play_samples", module_file )
            && load_function( module, &info->plugin_query, "plugin_query", info->module_file );

    if( rc && info->query_param.configurable ) {
      rc = load_function( module, &info->plugin_configure, "plugin_configure", module_file );
    }
    if( rc ) {
      sfname( info->module_name, module_file, sizeof( info->module_name ));
    }

    info->f = NULL;
    info->enabled = TRUE;
    return rc;
  }
  return FALSE;
}

/* Assigns the addresses of the visual plug-in procedures. */
static BOOL
load_visual( VISUAL* info )
{
  HMODULE module = info->module;
  char*   module_file = info->module_file;

  if( check_plugin( module, module_file, &info->query_param ) & PLUGIN_VISUAL )
  {
    BOOL rc =  load_function( module, &info->plugin_query, "plugin_query", module_file )
            && load_function( module, &info->plugin_deinit, "plugin_deinit", module_file )
            && load_function( module, &info->plugin_init, "vis_init", module_file );

    if( rc && info->query_param.configurable ) {
      rc = load_function( module, &info->plugin_configure, "plugin_configure", module_file );
    }
    if( rc ) {
      sfname( info->module_name, module_file, sizeof( info->module_name ));
    }

    info->enabled = TRUE;
    info->init = FALSE;
    info->hwnd = NULLHANDLE;
    return rc;
  }
  return FALSE;
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
  }

  return FALSE;
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
  }

  return FALSE;
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
  }

  return FALSE;
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
  }

  return FALSE;
}

/* Unloads and removes the specified decoder plug-in from the list of loaded. */
BOOL
remove_decoder_plugin( DECODER* decoder )
{
  int  i;
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

  unload_module( decoders[i].module_file, decoders[i].module );

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

  unload_module( outputs[i].module_file, outputs[i].module );

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

  unload_module( filters[i].module_file, filters[i].module );

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

  unload_module( visuals[i].module_file, visuals[i].module );

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
  int i;

  while( num_decoders ) {
    remove_decoder_plugin( decoders );
  }

  for( i = 0; i < sizeof( default_decoders ) / sizeof( *default_decoders ); i++ ) {
    sprintf( decoder.module_file, "%s%s", startpath, default_decoders[i] );
    if( load_module( decoder.module_file, &decoder.module )) {
      if( !add_decoder_plugin( &decoder )) {
        unload_module( decoder.module_file, decoder.module );
      }
    }
  }
}

/* Adds a default output plug-ins to the list of loaded. */
void
load_default_outputs( void )
{
  OUTPUT output;
  int i;

  while( num_outputs ) {
    remove_output_plugin( outputs );
  }

  for( i = 0; i < sizeof( default_outputs ) / sizeof( *default_outputs ); i++ ) {
    sprintf( output.module_file, "%s%s", startpath, default_outputs[i] );
    if( load_module( output.module_file, &output.module )) {
      if( !add_output_plugin( &output )) {
        unload_module( output.module_file, output.module );
      }
    }
  }

  out_set_active( 0 );
}

/* Adds a default filter plug-ins to the list of loaded. */
void
load_default_filters( void )
{
  FILTER filter;
  int i;

  dec_set_active( -1 );
  while( num_filters ) {
    remove_filter_plugin( filters );
  }

  for( i = 0; i < sizeof( default_filters ) / sizeof( *default_filters ); i++ ) {
    sprintf( filter.module_file, "%s%s", startpath, default_filters[i] );
    if( load_module( filter.module_file, &filter.module )) {
      if( !add_filter_plugin( &filter )) {
        unload_module( filter.module_file, filter.module );
      }
    }
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
    if( read_bufstream( b, &decoder.module_file, size ) != size ) {
      return FALSE;
    }
    decoder.module_file[size] = 0;

    if( !load_module( decoder.module_file, &decoder.module ) ||
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
    if( read_bufstream( b, &output.module_file, size ) != size ) {
      return FALSE;
    }
    output.module_file[size] = 0;

    if( !load_module( output.module_file, &output.module ) ||
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
    if( read_bufstream( b, &filter.module_file, size ) != size ) {
      return FALSE;
    }
    filter.module_file[size] = 0;

    if( !load_module( filter.module_file, &filter.module ) ||
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
    if( read_bufstream( b, &visual.module_file, size ) != size ) {
      return FALSE;
    }
    visual.module_file[size] = 0;
    visual.hwnd  = NULLHANDLE;
    visual.skin  = FALSE;
    visual.x     = 0;
    visual.y     = 0;
    visual.cx    = 0;
    visual.cy    = 0;
   *visual.param = 0;

    if( !load_module( visual.module_file, &visual.module ) ||
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
    size = strlen( decoders[i].module_file );

    if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( write_bufstream( b, &decoders[i].module_file, size ) != size ) {
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
    size = strlen( outputs[i].module_file );

    if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( write_bufstream( b, &outputs[i].module_file, size ) != size ) {
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
    size = strlen( filters[i].module_file );

    if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
      return FALSE;
    }
    if( write_bufstream( b, &filters[i].module_file, size ) != size ) {
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
      size = strlen( visuals[i].module_file );

      if( write_bufstream( b, &size, sizeof(int)) != sizeof(int)) {
        return FALSE;
      }
      if( write_bufstream( b, &visuals[i].module_file, size ) != size ) {
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
   Returns the types found or PLUGIN_ERROR. */
ULONG
add_plugin( const char* module_name, const VISUAL* data )
{
  HMODULE module;

  if( load_module( module_name, &module ))
  {
    PLUGIN_QUERYPARAM param;
    int types = check_plugin( module, module_name, &param );

    if( types & PLUGIN_VISUAL )
    {
      VISUAL visual;

      if( data ) {
        visual = *data;
      } else {
        memset( &visual, 0, sizeof( visual ));
      }

      strcpy( visual.module_file, module_name );
      visual.module = module;

      if( !add_visual_plugin( &visual )) {
        types = types & ~PLUGIN_VISUAL;
      }
    }
    if( types & PLUGIN_FILTER )
    {
      FILTER filter;
      strcpy( filter.module_file, module_name );
      filter.module = module;

      if( !add_filter_plugin( &filter )) {
        types = types & ~PLUGIN_FILTER;
      }
    }
    if( types & PLUGIN_DECODER )
    {
      DECODER decoder;
      strcpy( decoder.module_file, module_name );
      decoder.module = module;

      if( !add_decoder_plugin( &decoder )) {
        types = types & ~PLUGIN_DECODER;
      }
    }
    if( types & PLUGIN_OUTPUT )
    {
      OUTPUT output;
      strcpy( output.module_file, module_name );
      output.module = module;

      if( !add_output_plugin( &output )) {
        types = types & ~PLUGIN_OUTPUT;
      }
    }

    if( types == 0 ) {
      unload_module( module_name, module );
    }
    return types;
  }
  return 0;
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
    if( stricmp( decoders[i].module_name, name ) == 0 ) {
      return dec_set_active( i );
    }
  }
  return -2;
}

/* returns 0 = ok,
           1 = command unsupported,
           3 = no decoder active,
           4 = no active output, others unimportant. */
ULONG DLLENTRY 
dec_command( ULONG msg, DECODER_PARAMS *params )
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
        FILTER** enabled_filters = (FILTER**)alloca( num_filters * sizeof( FILTER* ));
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
          filter_params.output_play_samples = outputs[active_output].output_play_samples;
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
          params->output_play_samples = outputs[active_output].output_play_samples;
          params->a = outputs[active_output].a;
        }
      }
    }
    return decoders[active_decoder].decoder_command( decoders[active_decoder].w, msg, params );
  } else {
    return 3; // no decoder active
  }
}

/* This code must be a part of the dec_fileinfo, but it is keeped for
   compatibility. */
static ULONG DLLENTRY
dec_trackinfo( char* drive, int track, DECODER_INFO* info, char* name )
{
  int i;

  if( name && *name ) {
    for( i = 0; i < num_decoders; i++ ) {
      if( stricmp( decoders[i].module_name, name ) == 0 ) {
        return decoders[i].decoder_trackinfo( drive, track, info );
      }
    }
  } else {
    for( i = 0; i < num_decoders; i++ )
    {
      if( decoders[i].enabled &&
          decoders[i].decoder_trackinfo &&
        ( decoders[i].support & DECODER_TRACK ))
      {
        if( decoders[i].decoder_trackinfo( drive, track, info ) == 0 ) {
          if( name ) {
            strcpy( name, decoders[i].module_name );
          }
          return 0;
        }
      }
    }
  }

  return 200;
}

/* Returns the information about the specified file. If the decoder
   name is not filled, also returns the decoder name that can play
   this file. Returns 0 if it successfully completes operation,
   returns 200 if nothing can play that, or returns an error code
   returned by decoder module. */
ULONG DLLENTRY
dec_fileinfo( char* filename, DECODER_INFO* info, char* name )
{
  int   i, j;
  ULONG rc = 200;

  memset( info, 0, sizeof( *info ));

  if( is_track( filename ))
  {
    // The filename is a CD track name.
    char drive[3];
    int  track;

    sdrive( drive, filename, sizeof( drive ));
    track = strack( filename );

    rc = dec_trackinfo( drive, track, info, name );
  } else {
    // Have name of a decoder module.
    if( name && *name ) {
      for( i = 0; i < num_decoders; i++ ) {
        if( stricmp( decoders[i].module_name, name ) == 0 ) {
          rc = decoders[i].decoder_fileinfo( filename, info );
          break;
        }
      }
    // Must find a decoder module.
    } else {
      ULONG support = is_url( filename ) ? DECODER_URL : DECODER_FILENAME;
      BOOL* checked = calloc( sizeof( BOOL ), num_decoders );

      if( checked ) {
        // First check decoders supporting the specified type of files.
        for( i = 0; i < num_decoders && rc == 200; i++ )
        {
          if( decoders[i].enabled          &&
              decoders[i].decoder_fileinfo && ( decoders[i].support & support ))
          {
            for( j = 0; j < decoders[i].fileext_size; j++ ) {
              if( wildcardfit( decoders[i].fileext[j], filename )) {
                checked[i] = TRUE;
                if( decoders[i].decoder_fileinfo( filename, info ) == 0 ) {
                  if( name ) {
                    strcpy( name, decoders[i].module_name );
                  }
                  rc = 0;
                  break;
                }
                break;
              }
            }
          }
        }

        // Next check a rest of decoders.
        for( i = 0; i < num_decoders && rc == 200; i++ )
        {
          if( decoders[i].decoder_fileinfo &&
              decoders[i].enabled          &&
              !checked[i] && ( decoders[i].support & support ))
          {
            if( decoders[i].decoder_fileinfo( filename, info ) == 0 ) {
              if( name ) {
                strcpy( name, decoders[i].module_name );
              }
              rc = 0;
              break;
            }
          }
        }
        free( checked );
      }
    }
  }

  if( rc == 0 ) {
    // Old decoders can not fill a field of the size of a file.
    if( !info->filesize && is_file( filename )) {
      struct stat fi = {0};
      if( stat( filename, &fi ) == 0 ) {
        info->filesize = fi.st_size;
      }
    }
    if( !info->codepage && cfg.tags_charset != CH_DEFAULT )
    {
      char teststring[2048];

      strlcpy( teststring, info->title,     sizeof( teststring ));
      strlcat( teststring, info->artist,    sizeof( teststring ));
      strlcat( teststring, info->album,     sizeof( teststring ));
      strlcat( teststring, info->comment,   sizeof( teststring ));
      strlcat( teststring, info->copyright, sizeof( teststring ));
      strlcat( teststring, info->genre,     sizeof( teststring ));

      info->codepage = ch_detect( cfg.tags_charset, teststring );
    }
    if( info->codepage ) {
      ch_convert( info->codepage, info->title,     CH_DEFAULT, info->title,     sizeof( info->title     ));
      ch_convert( info->codepage, info->artist,    CH_DEFAULT, info->artist,    sizeof( info->artist    ));
      ch_convert( info->codepage, info->album,     CH_DEFAULT, info->album,     sizeof( info->album     ));
      ch_convert( info->codepage, info->comment,   CH_DEFAULT, info->comment,   sizeof( info->comment   ));
      ch_convert( info->codepage, info->copyright, CH_DEFAULT, info->copyright, sizeof( info->copyright ));
      ch_convert( info->codepage, info->genre,     CH_DEFAULT, info->genre,     sizeof( info->genre     ));
    }
  }
  return rc;
}

/* Updates the meta information about the specified file. Returns 0
   if it successfully completes operation, returns 200 if nothing can
   update that, or returns an error code returned by decoder module. */
ULONG DLLENTRY
dec_saveinfo( char* filename, DECODER_INFO* info, char* name )
{
  int i;

  if( !name || !*name ) {
    return 200;
  }

  for( i = 0; i < num_decoders; i++ ) {
    if( stricmp( decoders[i].module_name, name ) == 0 ) {
      break;
    }
  }

  if( i >= num_decoders || !decoders[i].decoder_saveinfo ) {
    return 200;
  }

  if( info->codepage )
  {
    DECODER_INFO save = *info;

    ch_convert( CH_DEFAULT, info->title,     save.codepage, save.title,     sizeof( save.title     ));
    ch_convert( CH_DEFAULT, info->artist,    save.codepage, save.artist,    sizeof( save.artist    ));
    ch_convert( CH_DEFAULT, info->album,     save.codepage, save.album,     sizeof( save.album     ));
    ch_convert( CH_DEFAULT, info->comment,   save.codepage, save.comment,   sizeof( save.comment   ));
    ch_convert( CH_DEFAULT, info->copyright, save.codepage, save.copyright, sizeof( save.copyright ));
    ch_convert( CH_DEFAULT, info->genre,     save.codepage, save.genre,     sizeof( save.genre     ));

    return decoders[i].decoder_saveinfo( filename, &save );
  } else {
    return decoders[i].decoder_saveinfo( filename, info );
  }
}


ULONG DLLENTRY
dec_cdinfo( char *drive, DECODER_CDINFO *info )
{
  int  i;
  for( i = 0; i < num_decoders; i++ )
  {
    if( decoders[i].enabled &&
        decoders[i].decoder_cdinfo &&
      ( decoders[i].support & DECODER_TRACK ))
    {
      if( decoders[i].decoder_cdinfo( drive, info ) == 0 ) {
        return 0;
      }
    }
  }
  return 200;
}

ULONG DLLENTRY
dec_status( void )
{
  if( active_decoder != -1 ) {
    return decoders[active_decoder].decoder_status( decoders[active_decoder].w );
  } else {
    return DECODER_ERROR;
  }
}

/* Length in ms, should still be valid if decoder stops. */
ULONG DLLENTRY
dec_length( void )
{
  if( active_decoder != -1 ) {
    return decoders[active_decoder].decoder_length( decoders[active_decoder].w );
  } else {
    return 0;
  }
}

/* Fills specified buffer with the list of extensions of supported files. */
void
dec_fill_types( char* result, size_t size )
{
  int i, j;

  *result = 0;

  for( i = 0; i < num_decoders; i++ ) {
    if( decoders[i].enabled ) {
      for( j = 0; j < decoders[i].fileext_size; j++ ) {
        strlcat( result, decoders[i].fileext[j], size );
        strlcat( result, ";", size );
      }
    }
  }

  if( *result && result[strlen( result )-1] == ';' ) {
    // Remove the last ";".
    result[strlen( result )-1] = 0;
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
out_set_name_active( char* name )
{
  int i;

  if( name == NULL || !*name ) {
    return out_set_active( -1 );
  }

  for( i = 0; i < num_outputs; i++ )
  {
    if( stricmp( outputs[i].module_name, name ) == 0 ) {
      out_set_active( i );
      return 0;
    }
  }
  return -2;
}

/* 0 = ok, else = return code from MMOS/2. */
ULONG
out_command( ULONG msg, OUTPUT_PARAMS* ai )
{
  if( active_output != -1 ) {
    return outputs[active_output].output_command( outputs[active_output].a, msg, ai );
  } else {
    amp_player_error( "There is no active output plug-in." );
    return -1;
  }
}

void
out_set_volume( int volume )
{
  OUTPUT_PARAMS out_params = { 0 };

  out_params.volume = volume;
  out_params.amplifier = 1.0;
  out_command( OUTPUT_VOLUME, &out_params );
}

/* Returns 0 = success otherwize MMOS/2 error. */
ULONG DLLENTRY
out_playing_samples( FORMAT_INFO* info, char* buf, int len )
{
  if( active_output != -1 ) {
    return outputs[active_output].output_playing_samples( outputs[active_output].a, info, buf, len );
  } else {
    return -1;
  }
}

/* Returns time in ms. */
ULONG DLLENTRY
out_playing_pos( void )
{
  if( active_output != -1 ) {
    return outputs[active_output].output_playing_pos( outputs[active_output].a );
  } else {
    return 0;
  }
}

/* if the output is playing. */
BOOL DLLENTRY
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
  procs.decoder_command        = dec_command;
  procs.decoder_fileinfo       = dec_fileinfo;
  procs.specana_init           = specana_init;
  procs.specana_dobands        = specana_dobands;
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
    sprintf( buffer, "%s (%s)", visuals[i].query_param.desc, visuals[i].module_name );

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
    sprintf( buffer, "%s (%s)", decoders[i].query_param.desc, decoders[i].module_name );

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
    sprintf( buffer, "%s (%s)", outputs[i].query_param.desc, outputs[i].module_name );

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
    sprintf( buffer, "%s (%s)", filters[i].query_param.desc, filters[i].module_name );

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
