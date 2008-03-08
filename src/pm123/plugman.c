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
#include "debuglog.h"
#include "snprintf.h"

DECODER** decoders       = NULL;
int       num_decoders   = 0;
int       active_decoder = -1;
OUTPUT**  outputs        = NULL;
int       num_outputs    = 0;
int       active_output  = -1;
FILTER**  filters        = NULL;
int       num_filters    = 0;
VISUAL**  visuals        = NULL;
int       num_visuals    = 0;

static HMTX  mutex;

static const char* default_decoders[] = { "mpg123.dll", "oggplay.dll", "wavplay.dll", "cddaplay.dll" };
static const char* default_outputs [] = { "os2audio.dll", "wavout.dll" };
static const char* default_filters [] = { "realeq.dll" };

/* Requests ownership of the plug-ins manager data. */
static BOOL
pg_request( void )
{
  APIRET rc = DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );

  if( rc != NO_ERROR )
  {
    char error[1024];
    amp_player_error( "Unable request the mutex semaphore.\n%s\n",
                      os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Relinquishes ownership of the plug-ins manager data was
   requested by pg_request(). */
static BOOL
pg_release( void )
{
  APIRET rc = DosReleaseMutexSem( mutex );

  if( rc != NO_ERROR )
  {
    char error[1024];
    amp_player_error( "Unable release the mutex semaphore.\n%s\n",
                      os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Loads a plug-in dynamic link module. */
static BOOL
pg_load_module( PLUGIN* plugin )
{
  char error[1024] = "";
  APIRET rc = DosLoadModule( error, sizeof( error ), plugin->file, &plugin->module );

  DEBUGLOG(( "pm123: loads plug-in (rc=%d) %s\n", rc, plugin->file ));

  if( rc != NO_ERROR ) {
    amp_player_error( "Could not load %s\n%s",
                       plugin->file, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  }

  sfname( plugin->name, plugin->file, sizeof( plugin->name ));
  DosCreateMutexSem( NULL, &plugin->mutex, 0, FALSE );
  return TRUE;
}

/* Unloads a plug-in dynamic link module. */
static BOOL
pg_unload_module( PLUGIN* plugin )
{
  char  error[1024];
  ULONG rc = DosFreeModule( plugin->module );

  DEBUGLOG(( "pm123: unloads plug-in (rc=%d) %s\n", rc, plugin->file ));
  DosCloseMutexSem( plugin->mutex );

  if( rc != NO_ERROR && rc != ERROR_INVALID_ACCESS ) {
    amp_player_error( "Could not unload %s\n%s",
                      plugin->name, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Assigns the address of the specified procedure within a plug-in. */
static BOOL
pg_load_function( PLUGIN* plugin, void* function, const char* name )
{
  char  error[1024];
  ULONG rc = DosQueryProcAddr( plugin->module, 0L, (PSZ)name, function );

  if( rc != NO_ERROR ) {
    *((ULONG*)function) = 0;
    amp_player_error( "Could not load \"%s\" from %s\n%s", name,
                      plugin->file, os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Fills the query_param and returns the type of plug-ins contained
   in specified module. */
static ULONG
pg_check_plugin( PLUGIN* plugin )
{
  void (DLLENTRYP plugin_query)( PLUGIN_QUERYPARAM* param );

  if( pg_load_function( plugin, &plugin_query, "plugin_query" )) {
    plugin_query( &plugin->info );
    return plugin->info.type;
  } else {
    return 0;
  }
}

/* Requests ownership of the specified plug-in. */
static BOOL
pg_plugin_request( PLUGIN* plugin )
{
  APIRET rc = DosRequestMutexSem( plugin->mutex, SEM_INDEFINITE_WAIT );

  if( rc != NO_ERROR )
  {
    char error[1024];
    amp_player_error( "Unable request the mutex semaphore.\n%s\n",
                      os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Relinquishes ownership of the specified plug-in was
   requested by pg_plugin_request(). */
static BOOL
pg_plugin_release( PLUGIN* plugin )
{
  APIRET rc = DosReleaseMutexSem( plugin->mutex );

  if( rc != NO_ERROR )
  {
    char error[1024];
    amp_player_error( "Unable release the mutex semaphore.\n%s\n",
                      os2_strerror( rc, error, sizeof( error )));
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Requests the rights of use of the plug-in module. */
BOOL
pg_plugin_is_used( PLUGIN* plugin )
{
  BOOL rc = FALSE;

  if( pg_plugin_request( plugin )) {
    if( plugin->used > 0 ) {
      ++plugin->used;
      rc = TRUE;
    }
    pg_plugin_release( plugin );
  }
  return rc;
}

/* Releases rights of use of the plug-in module. If the module
   is used by nobody more - unloads it. */
static BOOL
pg_plugin_no_used( PLUGIN* plugin )
{
  BOOL rc = FALSE;

  if( pg_plugin_request( plugin )) {
    if( --plugin->used <= 0 ) {
      pg_plugin_release( plugin );
      rc = pg_unload_module( plugin );
      free( plugin );
    } else {
      pg_plugin_release( plugin ) ;
      rc = TRUE;
    }
  }
  return rc;
}

/* Returns the index of the specified plug-in data. */
static int
pg_find_plugin( const char* name, int type )
{
  PLUGIN** plugins;
  int count;
  int i;

  pg_request();

  if( type & PLUGIN_DECODER ) {
    plugins = (PLUGIN**)decoders;
    count   = num_decoders;
  } else if( type & PLUGIN_OUTPUT ) {
    plugins = (PLUGIN**)outputs;
    count   = num_outputs;
  } else if( type & PLUGIN_FILTER ) {
    plugins = (PLUGIN**)filters;
    count   = num_filters;
  } else if( type & PLUGIN_VISUAL ) {
    plugins = (PLUGIN**)visuals;
    count   = num_visuals;
  } else {
    pg_release();
    return -1;
  }

  if( name ) {
    for( i = 0; i < count; i++ ) {
      if( stricmp( plugins[i]->name, name ) == 0 ) {
        pg_release();
        return i;
      }
    }
  }
  pg_release();
  return -1;
}

/* Returns the pointer to the specified plug-in data. */
static PLUGIN*
pg_query_plugin( const char* name, int type )
{
  PLUGIN** plugins;
  int count;
  int i;

  pg_request();

  if( type & PLUGIN_DECODER ) {
    plugins = (PLUGIN**)decoders;
    count   = num_decoders;
  } else if( type & PLUGIN_OUTPUT ) {
    plugins = (PLUGIN**)outputs;
    count   = num_outputs;
  } else if( type & PLUGIN_FILTER ) {
    plugins = (PLUGIN**)filters;
    count   = num_filters;
  } else if( type & PLUGIN_VISUAL ) {
    plugins = (PLUGIN**)visuals;
    count   = num_visuals;
  } else {
    pg_release();
    return NULL;
  }

  if( name ) {
    for( i = 0; i < count; i++ ) {
      if( stricmp( plugins[i]->name, name ) == 0 ) {
        pg_release();
        return plugins[i];
      }
    }
  }
  pg_release();
  return NULL;
}

/* Assigns the addresses of the decoder plug-in procedures.
   The enable/disable status should be set outside. */
static BOOL
pg_load_decoder( DECODER* decoder )
{
  int i;

  if( pg_check_plugin((PLUGIN*)decoder ) & PLUGIN_DECODER )
  {
    BOOL rc =  pg_load_function((PLUGIN*)decoder, &decoder->decoder_init, "decoder_init" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_uninit, "decoder_uninit" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_command, "decoder_command" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_status, "decoder_status" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_length, "decoder_length" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_fileinfo, "decoder_fileinfo" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_trackinfo, "decoder_trackinfo" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_cdinfo, "decoder_cdinfo" )
            && pg_load_function((PLUGIN*)decoder, &decoder->decoder_support, "decoder_support" );

    if( rc && decoder->pc.info.configurable ) {
      rc = pg_load_function((PLUGIN*)decoder, &decoder->pc.plugin_configure, "plugin_configure" );
    }

    if( rc )
    {
      char* ptrs[_MAX_FILEEXT];

      decoder->data         = NULL;
      decoder->pc.init      = FALSE;
      decoder->pc.used      = 1;
      decoder->fileext_size = _MAX_FILEEXT;

      memset( decoder->fileext, 0, sizeof( decoder->fileext ));

      for( i = 0; i < _MAX_FILEEXT; i++ ) {
        ptrs[i] = decoder->fileext[i];
      }

      decoder->support = decoder->decoder_support( ptrs, &decoder->fileext_size );

      if( decoder->support & DECODER_METAINFO ) {
        rc = pg_load_function((PLUGIN*)decoder, &decoder->decoder_saveinfo, "decoder_saveinfo" );
      } else {
        decoder->decoder_saveinfo = NULL;
      }
    }
    return rc;
  }
  return FALSE;
}

/* Assigns the addresses of the output plug-in procedures. */
static BOOL
pg_load_output( OUTPUT* output )
{
  if( pg_check_plugin((PLUGIN*)output ) & PLUGIN_OUTPUT )
  {
    BOOL rc =  pg_load_function((PLUGIN*)output, &output->output_init, "output_init" )
            && pg_load_function((PLUGIN*)output, &output->output_uninit, "output_uninit" )
            && pg_load_function((PLUGIN*)output, &output->output_command, "output_command" )
            && pg_load_function((PLUGIN*)output, &output->output_play_samples, "output_play_samples" )
            && pg_load_function((PLUGIN*)output, &output->output_playing_samples, "output_playing_samples" )
            && pg_load_function((PLUGIN*)output, &output->output_playing_pos, "output_playing_pos" )
            && pg_load_function((PLUGIN*)output, &output->output_playing_data, "output_playing_data" );

    if( rc && output->pc.info.configurable ) {
      rc = pg_load_function((PLUGIN*)output, &output->pc.plugin_configure, "plugin_configure" );
    }

    output->data       = NULL;
    output->pc.enabled = TRUE;
    output->pc.init    = FALSE;
    output->pc.used    = 1;
    return rc;
  }
  return FALSE;
}

/* Assigns the addresses of the filter plug-in procedures.
   The enable/disable status should be set outside. */
static BOOL
pg_load_filter( FILTER* filter )
{
  if( pg_check_plugin((PLUGIN*)filter ) & PLUGIN_FILTER )
  {
    BOOL rc =  pg_load_function((PLUGIN*)filter, &filter->filter_init, "filter_init" )
            && pg_load_function((PLUGIN*)filter, &filter->filter_play_samples, "filter_play_samples" )
            && pg_load_function((PLUGIN*)filter, &filter->filter_uninit, "filter_uninit" );

    if( rc && filter->pc.info.configurable ) {
      rc = pg_load_function((PLUGIN*)filter, &filter->pc.plugin_configure, "plugin_configure" );
    }

    filter->data    = NULL;
    filter->pc.init = FALSE;
    filter->pc.used = 1;
    return rc;
  }
  return FALSE;
}

/* Assigns the addresses of the visual plug-in procedures.
   The enable/disable status should be set outside. */
static BOOL
pg_load_visual( VISUAL* visual )
{
  if( pg_check_plugin((PLUGIN*)visual ) & PLUGIN_VISUAL )
  {
    BOOL rc =  pg_load_function((PLUGIN*)visual, &visual->plugin_deinit, "plugin_deinit" )
            && pg_load_function((PLUGIN*)visual, &visual->plugin_init, "vis_init" );

    if( rc && visual->pc.info.configurable ) {
      rc = pg_load_function((PLUGIN*)visual, &visual->pc.plugin_configure, "plugin_configure" );
    }

    visual->pc.init = FALSE;
    visual->pc.used = 1;
    visual->hwnd    = NULLHANDLE;
    return rc;
  }
  return FALSE;
}

/* Loads and adds the specified decoder plug-in to the list of loaded. */
static BOOL
pg_add_decoder_plugin( DECODER* decoder )
{
  if( pg_load_decoder( decoder ))
  {
    DECODER* copy = malloc( sizeof( DECODER ));

    if( copy ) {
      pg_request();
      decoders = realloc( decoders, ++num_decoders * sizeof( DECODER* ));
      memcpy( copy, decoder, sizeof( DECODER ));
      decoders[ num_decoders - 1 ] = copy;
      pg_release();
      return TRUE;
    }
  }
  return FALSE;
}

/* Loads and adds the specified output plug-in to the list of loaded. */
static BOOL
pg_add_output_plugin( OUTPUT* output )
{
  if( pg_load_output( output ))
  {
    OUTPUT* copy = malloc( sizeof( OUTPUT ));

    if( copy ) {
      pg_request();
      outputs = realloc( outputs, ++num_outputs * sizeof( OUTPUT* ));
      memcpy( copy, output, sizeof( OUTPUT ));
      outputs[ num_outputs - 1 ] = copy;

      if( active_output == -1 ) {
        out_set_active( output->pc.name );
      }
      pg_release();
      return TRUE;
    }
  }

  return FALSE;
}

/* Loads and adds the specified filter plug-in to the list of loaded. */
static BOOL
pg_add_filter_plugin( FILTER* filter )
{
  if( pg_load_filter( filter ))
  {
    FILTER* copy = malloc( sizeof( FILTER ));

    if( copy ) {
      pg_request();
      filters = realloc( filters, ++num_filters * sizeof( FILTER* ));
      memcpy( copy, filter, sizeof( FILTER ));
      filters[ num_filters - 1 ] = copy;
      pg_release();
      return TRUE;
    }
  }
  return FALSE;
}

/* Loads and adds the specified visual plug-in to the list of loaded. */
static BOOL
pg_add_visual_plugin( VISUAL* visual )
{
  int i;

  if( pg_load_visual( visual ))
  {
    pg_request();

    if(( i = pg_find_plugin( visual->pc.name, PLUGIN_VISUAL )) != -1 ) {
      if( visual->skin ) {
        visuals[i]->skin = visual->skin;
        visuals[i]->x    = visual->x;
        visuals[i]->y    = visual->y;
        visuals[i]->cx   = visual->cx;
        visuals[i]->cy   = visual->cy;
      }
      pg_release();
      pg_unload_module((PLUGIN*)visual );
    } else {
      VISUAL* copy = malloc( sizeof( VISUAL ));

      if( copy ) {
        visuals = realloc( visuals, ++num_visuals * sizeof( VISUAL* ));
        memcpy( copy, visual, sizeof( VISUAL ));
        visuals[ num_visuals - 1 ] = copy;
        pg_release();
        return TRUE;
      } else {
        pg_release();
      }
    }
  }

  return FALSE;
}

/* Initializes the specified decoder plug-in. */
static BOOL
pg_init_decoder( DECODER* decoder )
{
  if( pg_plugin_request((PLUGIN*)decoder )) {
    if( !decoder->pc.init ) {
      DEBUGLOG(( "pm123: initialize plug-in %s\n", decoder->pc.name ));
      if( decoder->decoder_init( &decoder->data ) != PLUGIN_FAILED ) {
        decoder->pc.init = TRUE;
      }
    }

    pg_plugin_release((PLUGIN*)decoder );
    return decoder->pc.init;
  } else {
    return FALSE;
  }
}

/* Initializes the specified filter plug-in. */
static BOOL
pg_init_filter( FILTER* filter, FILTER_PARAMS* params )
{
  if( pg_plugin_request((PLUGIN*)filter )) {
    if( !filter->pc.init ) {
      DEBUGLOG(( "pm123: initialize plug-in %s\n", filter->pc.name ));
      if( filter->filter_init( &filter->data, params ) != PLUGIN_FAILED ) {
        filter->pc.init = TRUE;
      }
    }
    pg_plugin_release((PLUGIN*)filter );
    return filter->pc.init;
  } else {
    return FALSE;
  }
}

/* Initializes the specified output plug-in. */
static BOOL
pg_init_output( OUTPUT* output )
{
  if( pg_plugin_request((PLUGIN*)output )) {
    if( !output->pc.init ) {
      DEBUGLOG(( "pm123: initialize plug-in %s\n", output->pc.name ));
      if( output->output_init( &output->data ) != PLUGIN_FAILED ) {
        output->pc.init = TRUE;
      }
    }
    pg_plugin_release((PLUGIN*)output );
    return output->pc.init;
  } else {
    return FALSE;
  }
}

/* Initializes the specified visual plug-in. */
static BOOL
pg_init_visual( VISUAL* visual, HWND hwnd )
{
  PLUGIN_PROCS  procs;
  VISPLUGININIT visinit;

  if( pg_plugin_request((PLUGIN*)visual )) {
    if( !visual->pc.init ) {
      DEBUGLOG(( "pm123: initialize plug-in %s\n", visual->pc.name ));

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

      visinit.x     = visual->x;
      visinit.y     = visual->y;
      visinit.cx    = visual->cx;
      visinit.cy    = visual->cy;
      visinit.hwnd  = hwnd;
      visinit.procs = &procs;
      visinit.id    = 100;
      visinit.param = visual->param;
      visinit.hab   = WinQueryAnchorBlock( hwnd );

      if(( visual->hwnd = visual->plugin_init( &visinit )) != NULLHANDLE ) {
        visual->pc.init = TRUE;
      }
    }
    pg_plugin_release((PLUGIN*)visual );
    return visual->pc.init;
  } else {
    return FALSE;
  }
}

/* Cleanups the specified decoder plug-in. */
static BOOL
pg_uninit_decoder( DECODER* decoder )
{
  BOOL rc = TRUE;
  if( pg_plugin_request((PLUGIN*)decoder )) {
    if( decoder->pc.init ) {
      DEBUGLOG(( "pm123: terminates plug-in %s\n", decoder->pc.name ));
      rc = decoder->decoder_uninit( decoder->data );

      decoder->pc.init = FALSE;
      decoder->data    = NULL;
    }
    pg_plugin_release((PLUGIN*)decoder );
  } else {
    rc = FALSE;
  }
  return rc;
}

/* Cleanups the specified output plug-in. */
static BOOL
pg_uninit_output( OUTPUT* output )
{
  BOOL rc = TRUE;

  if( pg_plugin_request((PLUGIN*)output )) {
    if( output->pc.init ) {
      DEBUGLOG(( "pm123: terminates plug-in %s\n", output->pc.name ));
      rc = output->output_uninit( output->data );

      output->data    = NULL;
      output->pc.init = FALSE;
    }
    pg_plugin_release((PLUGIN*)output );
  } else {
    rc = FALSE;
  }
  return rc;
}

/* Cleanups the specified filter plug-in. */
static BOOL
pg_uninit_filter( FILTER* filter )
{
  BOOL rc = TRUE;

  if( pg_plugin_request((PLUGIN*)filter )) {
    if( filter->pc.init ) {
      DEBUGLOG(( "pm123: terminates plug-in %s\n", filter->pc.name ));
      rc = filter->filter_uninit( filter->data );

      filter->pc.init = FALSE;
      filter->data    = NULL;
    }
    pg_plugin_release((PLUGIN*)filter );
  } else {
    rc = FALSE;
  }
  return rc;
}

/* Cleanups the specified visual plug-in. */
static BOOL
pg_uninit_visual( VISUAL* visual )
{
  BOOL rc = TRUE;

  if( pg_plugin_request((PLUGIN*)visual )) {
    if( visual->pc.init ) {
      DEBUGLOG(( "pm123: terminates plug-in %s\n", visual->pc.name ));
      rc = visual->plugin_deinit();

      visual->pc.init = FALSE;
      visual->hwnd    = NULLHANDLE;
    }
    pg_plugin_release((PLUGIN*)visual );
  } else {
    rc = FALSE;
  }
  return rc;
}

/* Unloads and removes the specified decoder plug-in from
   the list of loaded. */
BOOL
pg_remove_decoder( const char* name )
{
  int i;
  pg_request();

  if(( i = pg_find_plugin( name, PLUGIN_DECODER )) != -1 )
  {
    DECODER* decoder = decoders[i];

    if( active_decoder == i ) {
      active_decoder = -1;
    } else if( active_decoder > i ) {
      active_decoder -= 1;
    }
    if( i < num_decoders - 1 ) {
      memmove( decoders + i, decoders + i + 1, ( num_decoders - i - 1 ) * sizeof( DECODER* ));
    }
    decoders = realloc( decoders, --num_decoders * sizeof( DECODER* ));

    pg_release();
    pg_uninit_decoder( decoder );

    return pg_plugin_no_used((PLUGIN*)decoder );
  }

  pg_release();
  return FALSE;
}

/* Unloads and removes the specified output plug-in from
   the list of loaded. */
BOOL
pg_remove_output( const char* name )
{
  int i;
  pg_request();

  if(( i = pg_find_plugin( name, PLUGIN_OUTPUT )) != -1 )
  {
    OUTPUT* output = outputs[i];

    if( active_output == i ) {
      active_output = -1;
    } else if( active_output > i ) {
      active_output -= 1;
    }
    if( i < num_outputs - 1 ) {
      memmove( outputs + i, outputs + i + 1, ( num_outputs - i - 1 ) * sizeof( OUTPUT* ));
    }
    outputs = realloc( outputs, --num_outputs * sizeof( OUTPUT* ));

    pg_release();
    pg_uninit_output( output );

    return pg_plugin_no_used((PLUGIN*)output );
  }

  pg_release();
  return FALSE;
}

/* Unloads and removes the specified filter plug-in from
   the list of loaded. */
BOOL
pg_remove_filter( const char* name )
{
  int i;
  pg_request();

  if(( i = pg_find_plugin( name, PLUGIN_FILTER )) != -1 )
  {
    FILTER* filter = filters[i];

    if( i < num_filters - 1 ) {
      memmove( filters + i, filters + i + 1, ( num_filters - i - 1 ) * sizeof( FILTER* ));
    }
    filters = realloc( filters, --num_filters * sizeof( FILTER* ));

    pg_release();
    pg_uninit_filter( filter );

    return pg_plugin_no_used((PLUGIN*)filter );
  }

  pg_release();
  return FALSE;
}

/* Unloads and removes the specified visual plug-in from
   the list of loaded. */
BOOL
pg_remove_visual( const char* name )
{
  int i;
  pg_request();

  if(( i = pg_find_plugin( name, PLUGIN_VISUAL )) != -1 )
  {
    VISUAL* visual = visuals[i];

    if( i < num_visuals - 1 ) {
      memmove( visuals + i, visuals + i + 1, ( num_visuals - i - 1 ) * sizeof( VISUAL* ));
    }
    visuals = realloc( visuals, --num_visuals * sizeof( VISUAL* ));

    pg_release();
    pg_uninit_visual( visual );

    return pg_plugin_no_used((PLUGIN*)visual );
  }

  pg_release();
  return FALSE;
}

/* Unloads and removes all decoder plug-ins from the list of loaded. */
static void
pg_remove_all_decoders( void )
{
  DECODER** plugins;
  int count;
  int i;

  pg_request();
  plugins        = decoders;
  count          = num_decoders;
  active_decoder = -1;
  num_decoders   = 0;
  decoders       = NULL;
  pg_release();

  for( i = 0; i < count; i++ ) {
    pg_uninit_decoder( plugins[i] );
    pg_plugin_no_used((PLUGIN*)plugins[i] );
  }
  free( plugins );
}

/* Unloads and removes all output plug-ins from the list of loaded. */
static void
pg_remove_all_outputs( void )
{
  OUTPUT** plugins;
  int count;
  int i;

  pg_request();
  plugins       = outputs;
  count         = num_outputs;
  active_output = -1;
  num_outputs   = 0;
  outputs       = NULL;
  pg_release();

  for( i = 0; i < count; i++ ) {
    pg_uninit_output( plugins[i] );
    pg_plugin_no_used((PLUGIN*)plugins[i] );
  }
  free( plugins );
}

/* Unloads and removes all filter plug-ins from the list of loaded. */
static void
pg_remove_all_filters( void )
{
  FILTER** plugins;
  int count;
  int i;

  pg_request();
  plugins     = filters;
  count       = num_filters;
  num_filters = 0;
  filters     = NULL;
  pg_release();

  for( i = 0; i < count; i++ ) {
    pg_uninit_filter( plugins[i] );
    pg_plugin_no_used((PLUGIN*)plugins[i] );
  }
  free( plugins );
}

/* Unloads and removes all visual plug-ins from the list of loaded. */
void
pg_remove_all_visuals( BOOL skinned )
{
  int i = 0;
  pg_request();

  while( i < num_visuals ) {
    if( visuals[i]->skin == skinned )
    {
      char name[_MAX_MODULE_NAME];
      strlcpy( name, visuals[i]->pc.name, sizeof( name ));
      pg_release();
      pg_remove_visual( name );
      pg_request();
    } else {
      ++i;
    }
  }
  pg_release();
}

/* Unloads and removes all loaded plug-ins. */
static void
pg_remove_all_plugins( void )
{
  pg_remove_all_decoders();
  pg_remove_all_filters ();
  pg_remove_all_outputs ();
  pg_remove_all_visuals ( TRUE  );
  pg_remove_all_visuals ( FALSE );
}

/* Adds a default decoder plug-ins to the list of loaded. */
void
pg_load_default_decoders( void )
{
  DECODER decoder;
  int i;

  pg_remove_all_decoders();

  for( i = 0; i < sizeof( default_decoders ) / sizeof( *default_decoders ); i++ )
  {
    sprintf( decoder.pc.file, "%s%s", startpath, default_decoders[i] );
    decoder.pc.enabled = TRUE;

    if( pg_load_module((PLUGIN*)&decoder )) {
      if( !pg_add_decoder_plugin( &decoder )) {
        pg_unload_module((PLUGIN*)&decoder );
      }
    }
  }
}

/* Adds a default output plug-ins to the list of loaded. */
void
pg_load_default_outputs( void )
{
  OUTPUT output;
  int i;

  pg_remove_all_outputs();

  for( i = 0; i < sizeof( default_outputs ) / sizeof( *default_outputs ); i++ )
  {
    sprintf( output.pc.file, "%s%s", startpath, default_outputs[i] );

    if( pg_load_module((PLUGIN*)&output )) {
      if( !pg_add_output_plugin( &output )) {
        pg_unload_module((PLUGIN*)&output );
      }
    }
  }
}

/* Adds a default filter plug-ins to the list of loaded. */
void
pg_load_default_filters( void )
{
  FILTER filter;
  int i;

  pg_remove_all_filters();

  for( i = 0; i < sizeof( default_filters ) / sizeof( *default_filters ); i++ )
  {
    sprintf( filter.pc.file, "%s%s", startpath, default_filters[i] );

    if( pg_load_module((PLUGIN*)&filter )) {
      if( !pg_add_filter_plugin( &filter )) {
        pg_unload_module((PLUGIN*)&filter );
      }
    }
  }
}

/* Adds a default visual plug-ins to the list of loaded. */
void
pg_load_default_visuals( void  ) {
  pg_remove_all_visuals( FALSE );
}

/* Saves the current list of decoders. */
BOOL
pg_save_decoders( BUFSTREAM* b )
{
  int size;
  int i;

  pg_request();

  if( write_bufstream( b, &num_decoders, sizeof( num_decoders )) == sizeof( num_decoders )) {
    for( i = 0; i < num_decoders; i++ )
    {
      size = strlen( decoders[i]->pc.file );

      if( write_bufstream( b, &size, sizeof( size )) != sizeof( size )) {
        break;
      }
      if( write_bufstream( b, &decoders[i]->pc.file, size ) != size ) {
        break;
      }
      if( write_bufstream( b, &decoders[i]->pc.enabled,
                           sizeof( decoders[i]->pc.enabled )) != sizeof( decoders[i]->pc.enabled )) {
        break;
      }
    }
  }

  pg_release();
  return i == num_decoders;
}

/* Saves the current list of outputs plug-ins. */
BOOL
pg_save_outputs( BUFSTREAM* b )
{
  int size;
  int i;

  pg_request();

  if( write_bufstream( b, &num_outputs, sizeof( num_outputs )) == sizeof( num_outputs )) {
    for( i = 0; i < num_outputs; i++ ) {
      size = strlen( outputs[i]->pc.file );

      if( write_bufstream( b, &size, sizeof( size )) != sizeof( size )) {
        break;
      }
      if( write_bufstream( b, &outputs[i]->pc.file, size ) != size ) {
        break;
      }
    }

    if( i == num_outputs ) {
      if( write_bufstream( b, &active_output, sizeof( active_output )) == sizeof( active_output )) {
        pg_release();
        return TRUE;
      }
    }
  }

  pg_release();
  return FALSE;
}

/* Saves the current list of filters plug-ins. */
BOOL
pg_save_filters( BUFSTREAM* b )
{
  int size;
  int i;

  pg_request();

  if( write_bufstream( b, &num_filters, sizeof( num_filters )) == sizeof( num_filters )) {
    for( i = 0; i < num_filters; i++ )
    {
      size = strlen( filters[i]->pc.file );

      if( write_bufstream( b, &size, sizeof( size )) != sizeof( size )) {
        break;
      }
      if( write_bufstream( b, &filters[i]->pc.file, size ) != size ) {
        break;
      }
      if( write_bufstream( b, &filters[i]->pc.enabled,
                           sizeof( filters[i]->pc.enabled )) != sizeof( filters[i]->pc.enabled )) {
        break;
      }
    }
  }
  pg_release();
  return i == num_filters;
}

/* Saves the current list of visuals plug-ins. */
BOOL
pg_save_visuals( BUFSTREAM *b )
{
  int size;
  int i;
  int count = 0;

  pg_request();

  for( i = 0; i < num_visuals; i++ ) {
    if( !visuals[i]->skin ) {
      ++count;
    }
  }

  if( write_bufstream( b, &count, sizeof( count )) == sizeof( count )) {
    for( i = 0; i < num_visuals; i++ ) {
      if( !visuals[i]->skin )
      {
        size = strlen( visuals[i]->pc.file );

        if( write_bufstream( b, &size, sizeof( size )) != sizeof( size )) {
          break;
        }
        if( write_bufstream( b, &visuals[i]->pc.file, size ) != size ) {
          break;
        }
        if( write_bufstream( b, &visuals[i]->pc.enabled,
                             sizeof( visuals[i]->pc.enabled )) != sizeof( visuals[i]->pc.enabled )) {
          break;
        }
      }
    }
  }
  pg_release();
  return i == num_visuals;
}

/* Restores the decoders list to the state was in when
   save_decoders was last called. */
BOOL
pg_load_decoders( BUFSTREAM* b )
{
  int     size;
  int     count;
  int     i;
  BOOL    enabled;
  DECODER decoder;

  pg_remove_all_decoders();

  if( read_bufstream( b, &count, sizeof( count )) != sizeof( count )) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof( size )) == sizeof( size ) &&
        read_bufstream( b, &decoder.pc.file, size ) == size &&
        read_bufstream( b, &enabled, sizeof( enabled )) == sizeof( enabled ))
    {
      decoder.pc.file[size] = 0;
      decoder.pc.enabled = enabled;

      if( pg_load_module((PLUGIN*)&decoder )) {
        if( !pg_add_decoder_plugin( &decoder )) {
          pg_unload_module((PLUGIN*)&decoder );
          return FALSE;
        }
      }
    } else {
      return FALSE;
    }
  }

  return TRUE;
}

/* Restores the outputs list to the state was in when
   save_outputs was last called. */
BOOL
pg_load_outputs( BUFSTREAM *b )
{
  int    size;
  int    count;
  int    i;
  int    active;
  OUTPUT output;

  pg_remove_all_outputs();

  if( read_bufstream( b, &count, sizeof( count )) != sizeof( count )) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof( size ))  == sizeof( size ) &&
        read_bufstream( b, &output.pc.file, size ) == size )
    {
      output.pc.file[size] = 0;

      if( pg_load_module((PLUGIN*)&output )) {
        if( !pg_add_output_plugin( &output )) {
          pg_unload_module((PLUGIN*)&output );
          return FALSE;
        }
      }
    } else {
      return FALSE;
    }
  }

  if( read_bufstream( b, &active, sizeof( active )) == sizeof( active )) {
    pg_request();
    if( active < num_outputs ) {
      out_set_active( outputs[active]->pc.name );
    }
    pg_release();
  }
  return TRUE;
}

/* Restores the filters list to the state was in when
   save_filters was last called. */
BOOL
pg_load_filters( BUFSTREAM *b )
{
  int    size;
  int    count;
  int    i;
  BOOL   enabled;
  FILTER filter;

  pg_remove_all_filters();

  if( read_bufstream( b, &count, sizeof( count )) != sizeof( count )) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof( size ))  == sizeof( size ) &&
        read_bufstream( b, &filter.pc.file, size ) == size &&
        read_bufstream( b, &enabled, sizeof( enabled )) == sizeof( enabled ))
    {
      filter.pc.file[size] = 0;
      filter.pc.enabled = enabled;

      if( pg_load_module((PLUGIN*)&filter )) {
        if( !pg_add_filter_plugin( &filter )) {
          pg_unload_module((PLUGIN*)&filter );
          return FALSE;
        }
      }
    } else {
      return FALSE;
    }
  }
  return TRUE;
}

/* Restores the visuals list to the state was in when
   save_visuals was last called. */
BOOL
pg_load_visuals( BUFSTREAM *b )
{
  int    size;
  int    count;
  int    i;
  BOOL   enabled;
  VISUAL visual;

  pg_remove_all_visuals( FALSE );

  if( read_bufstream( b, &count, sizeof( count )) != sizeof( count )) {
    return FALSE;
  }

  for( i = 0; i < count; i++ ) {
    if( read_bufstream( b, &size, sizeof( size ))  == sizeof( size ) &&
        read_bufstream( b, &visual.pc.file, size ) == size &&
        read_bufstream( b, &enabled, sizeof( enabled )) == sizeof( enabled ))
    {
      visual.pc.file[size] = 0;
      visual.pc.enabled    = enabled;
      visual.hwnd          = NULLHANDLE;
      visual.skin          = FALSE;
      visual.x             = 0;
      visual.y             = 0;
      visual.cx            = 0;
      visual.cy            = 0;
      visual.param[0]      = 0;

      if( pg_load_module((PLUGIN*)&visual )) {
        if( !pg_add_visual_plugin( &visual )) {
          pg_unload_module((PLUGIN*)&visual );
          return FALSE;
        }
      }
    } else {
      return FALSE;
    }
  }
  return TRUE;
}

/* Loads and adds the specified plug-in to the appropriate list of loaded.
   Returns the types found or PLUGIN_ERROR. */
int
pg_load_plugin( const char* file, const VISUAL* data )
{
  PLUGIN plugin;

  strlcpy( plugin.file, file, sizeof( plugin.file ));
  plugin.enabled = TRUE;

  if( pg_load_module( &plugin ))
  {
    int types = pg_check_plugin( &plugin );

    if( types & PLUGIN_VISUAL )
    {
      VISUAL visual = {{ 0 }};

      if( data ) {
        visual.skin = data->skin;
        visual.x    = data->x;
        visual.y    = data->y;
        visual.cx   = data->cx;
        visual.cy   = data->cy;

        strcpy( visual.param, data->param );
      }

      visual.pc = plugin;
      if( !pg_add_visual_plugin( &visual )) {
        types = types & ~PLUGIN_VISUAL;
      }
    }

    if( types & PLUGIN_FILTER )
    {
      FILTER filter;
      filter.pc = plugin;

      if( !pg_add_filter_plugin( &filter )) {
        types = types & ~PLUGIN_FILTER;
      }
    }

    if( types & PLUGIN_DECODER )
    {
      DECODER decoder;
      decoder.pc = plugin;

      if( !pg_add_decoder_plugin( &decoder )) {
        types = types & ~PLUGIN_DECODER;
      }
    }

    if( types & PLUGIN_OUTPUT )
    {
      OUTPUT output;
      output.pc = plugin;

      if( !pg_add_output_plugin( &output )) {
        types = types & ~PLUGIN_OUTPUT;
      }
    }

    if( types == 0 ) {
      pg_unload_module( &plugin );
    }
    return types;
  }
  return 0;
}

/* Invokes a specified plug-in configuration dialog. */
void
pg_configure( const char* name, int type, HWND hwnd )
{
  PLUGIN* plugin;
  pg_request();

  if(( plugin = pg_query_plugin( name, type )) != NULL )
  {
    pg_plugin_is_used( plugin );
    pg_release();
    plugin->plugin_configure( hwnd, plugin->module );
    pg_plugin_no_used( plugin );
  } else {
    pg_release();
  }
}

/* Adds plug-ins names to the specified list box control. */
void
pg_expand_to( HWND hwnd, SHORT id, int type )
{
  PLUGIN** plugins;
  int count;
  int i;

  pg_request();

  if( type & PLUGIN_DECODER ) {
    plugins = (PLUGIN**)decoders;
    count   = num_decoders;
  } else if( type & PLUGIN_OUTPUT ) {
    plugins = (PLUGIN**)outputs;
    count   = num_outputs;
  } else if( type & PLUGIN_FILTER ) {
    plugins = (PLUGIN**)filters;
    count   = num_filters;
  } else if( type & PLUGIN_VISUAL ) {
    plugins = (PLUGIN**)visuals;
    count   = num_visuals;
  } else {
    pg_release();
    return;
  }

  for( i = 0; i < count; i++ ) {
    lb_add_item( hwnd, id, plugins[i]->name );
  }

  pg_release();
}

/* Returns an information about specified plug-in. */
void
pg_get_info( const char* name, int type, PLUGIN_QUERYPARAM* info )
{
  PLUGIN* plugin;
  pg_request();

  if(( plugin = pg_query_plugin( name, type )) != NULL ) {
    memcpy( info, &plugin->info, sizeof( *info ));
  } else {
    memset( info, 0, sizeof( *info ));
  }
  pg_release();
}

/* Returns TRUE if the specified plug-in is enabled. */
BOOL
pg_is_enabled( const char* name, int type )
{
  BOOL    rc = FALSE;
  PLUGIN* plugin;

  pg_request();
  if(( plugin = pg_query_plugin( name, type )) != NULL ) {
    rc = plugin->enabled;
  }
  pg_release();
  return rc;
}

/* Enables the specified plug-in. */
BOOL
pg_enable( const char* name, int type, BOOL enable )
{
  PLUGIN* plugin;

  pg_request();
  if(( plugin = pg_query_plugin( name, type )) != NULL ) {
    plugin->enabled = enable;
    pg_release();
    return TRUE;
  } else {
    pg_release();
    return FALSE;
  }
}

/* Returns PLUGIN_FAILED if the specified decoder is not found or
   it is not enabled or if a error occured, otherwize may returns
   the decoder's thread. */
LONG DLLENTRY
dec_set_active( const char* name )
{
  DECODER* decoder;
  int i;

  pg_request();

  if( name != NULL )
  {
    i = pg_find_plugin( name, PLUGIN_DECODER );

    if( i == -1 || !decoders[i]->pc.enabled ) {
      pg_release();
      return PLUGIN_FAILED;
    } else if( i == active_decoder ) {
      pg_release();
      return PLUGIN_OK;
    }
  }

  if( active_decoder != -1 ) {
    decoder = decoders[ active_decoder ];
    active_decoder = -1;
    pg_uninit_decoder( decoder );
  }

  if( name != NULL ) {
    if( pg_init_decoder( decoders[i] )) {
      active_decoder = i;
    }
  }

  pg_release();
  return PLUGIN_OK;
}

/* Returns a description of the specified decoder module. */
char* DLLENTRY
dec_get_description( const char* name, char* result, int size )
{
  int i;

  if( size > 0 ) {
    *result = 0;
    if( name ) {
      pg_request();
      if(( i = pg_find_plugin( name, PLUGIN_DECODER )) != -1 ) {
        strlcpy( result, decoders[i]->pc.info.desc, size );
      }
      pg_release();
    }
  }

  return result;
}

/* Links filters and an output in a chain and prepares parameters of
   the decoder. */
BOOL DLLENTRY
dec_set_filters( DECODER_PARAMS* decode_params )
{
  FILTER*       filter = NULL;
  FILTER_PARAMS filter_params;
  int i;

  pg_request();

  if( decode_params == NULL ) {
    for( i = 0; i < num_filters; i++ ) {
      pg_uninit_filter( filters[i] );
    }
  } else if( active_output == -1 ) {
    pg_release();
    amp_player_error( "There is no active output." );
    return FALSE;
  } else {
    for( i = num_filters - 1; i >= 0; i-- )
    {
      pg_uninit_filter( filters[i] );

      if( filters[i]->pc.enabled )
      {
        filter_params.error_display    = decode_params->error_display;
        filter_params.info_display     = decode_params->error_display;
        filter_params.audio_buffersize = decode_params->audio_buffersize;

        if( filter ) {
          filter_params.output_play_samples = filter->filter_play_samples;
          filter_params.a = filter->data;
        } else {
          // It is last filter in a chain of filters.
          filter_params.output_play_samples = outputs[active_output]->output_play_samples;
          filter_params.a = outputs[active_output]->data;
        }
        if( pg_init_filter( filters[i], &filter_params )) {
          filter = filters[i];
        }
      }
    }
    if( filter ) {
      decode_params->output_play_samples = filter->filter_play_samples;
      decode_params->a = filter->data;
    } else {
      // There is no filters.
      decode_params->output_play_samples = outputs[active_output]->output_play_samples;
      decode_params->a = outputs[active_output]->data;
    }
  }

  pg_release();
  return TRUE;
}

/* Returns PLUGIN_OK = ok,
           PLUGIN_UNSUPPORTED = command unsupported,
           PLUGIN_NO_USABLE   = no decoder active,
           others unimportant. */
ULONG DLLENTRY
dec_command( ULONG msg, DECODER_PARAMS* params )
{
  ULONG rc = PLUGIN_NO_USABLE;

  pg_request();
  if( active_decoder != -1 ) {
    rc = decoders[active_decoder]->decoder_command( decoders[active_decoder]->data, msg, params );
  }
  pg_release();
  return rc;
}

/* This code must be a part of the dec_fileinfo, but it is keeped for
   compatibility. */
ULONG DLLENTRY
dec_trackinfo( char* drive, int track, DECODER_INFO* info, char* name )
{
  int      i;
  ULONG    rc;
  DECODER* decoder;

  if( name && *name ) {
    pg_request();
    for( i = 0; i < num_decoders; i++ ) {
      if( stricmp( decoders[i]->pc.name, name ) == 0 ) {
        decoder = decoders[i];
        pg_plugin_is_used((PLUGIN*)decoder );
        pg_release();
        rc = decoder->decoder_trackinfo( drive, track, info );
        pg_plugin_no_used((PLUGIN*)decoder );
        return rc;
      }
    }
    pg_release();
  } else {
    pg_request();
    for( i = 0; i < num_decoders; i++ )
    {
      if( decoders[i]->pc.enabled &&
          decoders[i]->decoder_trackinfo &&
        ( decoders[i]->support & DECODER_TRACK ))
      {
        decoder = decoders[i];
        pg_plugin_is_used((PLUGIN*)decoder );
        pg_release();

        if( decoder->decoder_trackinfo( drive, track, info ) == 0 ) {
          if( name ) {
            strcpy( name, decoder->pc.name );
          }
          pg_plugin_no_used((PLUGIN*)decoder );
          return 0;
        } else {
          pg_request();
          pg_plugin_no_used((PLUGIN*)decoder );
        }
      }
    }
    pg_release();
  }

  return PLUGIN_NO_PLAY;
}

/* The dec_fileinfo helper function. */
static ULONG
dec_call_fileinfo( const char* filename, DECODER_INFO* info, DECODER* decoder, char* name )
{
  ULONG rc;

  pg_plugin_is_used((PLUGIN*)decoder );
  pg_release();
  rc = decoder->decoder_fileinfo((char*)filename, info );
  pg_plugin_no_used((PLUGIN*)decoder );
  pg_request();

  if( rc == PLUGIN_OK && name ) {
    strcpy( name, decoder->pc.name );
  }

  DEBUGLOG(( "pm123: query file %s info via %s (%s), rc=%d\n",
              filename, decoder->pc.name, decoder->pc.file, rc ));
  return rc;
}

/* Returns the information about the specified file. If the decoder
   name is not filled, also returns the decoder name that can play
   this file. Returns PLUGIN_OK if it successfully completes operation,
   returns PLUGIN_NO_PLAY if nothing can play that, or returns an error code
   returned by decoder module. */
ULONG DLLENTRY
dec_fileinfo( const char* filename, DECODER_INFO* info, char* name )
{
  int   i, j;
  ULONG rc = PLUGIN_NO_PLAY;

  memset( info, 0, sizeof( *info ));
  info->size = sizeof( *info );

  if( is_track( filename ))
  {
    char drive[3];
    int  track;

    // The filename is a CD track name.
    sdrive( drive, filename, sizeof( drive ));
    track = strack( filename );

    rc = dec_trackinfo( drive, track, info, name );
  } else {
    pg_request();
    // Have a name of the decoder module.
    if( name && *name ) {
      if(( i = pg_find_plugin( name, PLUGIN_DECODER )) != -1 ) {
        rc = dec_call_fileinfo( filename, info, decoders[i], NULL );
      }
    // Must find a decoder module.
    } else {
      BOOL* checked    = calloc( sizeof( BOOL ), num_decoders );
      int   num_checks = num_decoders;
      ULONG type       = is_url( filename ) ? DECODER_URL : DECODER_FILENAME;

      if( checked ) {
        // First check decoders supporting the specified type of files.
        for( i = 0; i < num_decoders && rc == PLUGIN_NO_PLAY; i++ )
        {
          if( decoders[i]->pc.enabled       &&
              decoders[i]->decoder_fileinfo && ( decoders[i]->support & type ))
          {
            for( j = 0; j < decoders[i]->fileext_size; j++ ) {
              if( wildcardfit( decoders[i]->fileext[j], filename )) {
                if( i < num_checks ) {
                  checked[i] = TRUE;
                }
                if( dec_call_fileinfo( filename, info, decoders[i], name ) == PLUGIN_OK ) {
                  rc = PLUGIN_OK;
                  break;
                }
                break;
              }
            }
          }
        }

        // Next check a rest of decoders.
        for( i = 0; i < num_decoders && rc == PLUGIN_NO_PLAY; i++ )
        {
          if( decoders[i]->decoder_fileinfo &&
              decoders[i]->pc.enabled       &&
              i < num_checks && !checked[i] && ( decoders[i]->support & type ))
          {
            if( dec_call_fileinfo( filename, info, decoders[i], name ) == 0 ) {
              rc = PLUGIN_OK;
              break;
            }
          }
        }
        free( checked );
      }
    }
    pg_release();
  }

  if( rc == PLUGIN_OK ) {
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
    if( info->codepage && info->codepage != ch_default()) {
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

/* Updates the meta information about the specified file. Returns PLUGIN_OK
   if it successfully completes operation, returns PLUGIN_NO_USABLE if nothing can
   update that, or returns an error code returned by decoder module. */
ULONG DLLENTRY
dec_saveinfo( const char* filename, DECODER_INFO* info, char* name )
{
  ULONG    rc;
  DECODER* decoder;
  int      i;

  if( !name || !*name ) {
    return PLUGIN_NO_USABLE;
  }

  pg_request();

  if(( i = pg_find_plugin( name, PLUGIN_DECODER )) == -1 ) {
    pg_release();
    return PLUGIN_NO_USABLE;
  }

  decoder = decoders[i];
  pg_plugin_is_used((PLUGIN*)decoder );
  pg_release();

  if(!( decoder->support & DECODER_METAINFO ) || !decoder->decoder_saveinfo ) {
    rc = PLUGIN_NO_USABLE;
  }
  else if( info->codepage )
  {
    DECODER_INFO save = *info;

    ch_convert( CH_DEFAULT, info->title,     save.codepage, save.title,     sizeof( save.title     ));
    ch_convert( CH_DEFAULT, info->artist,    save.codepage, save.artist,    sizeof( save.artist    ));
    ch_convert( CH_DEFAULT, info->album,     save.codepage, save.album,     sizeof( save.album     ));
    ch_convert( CH_DEFAULT, info->comment,   save.codepage, save.comment,   sizeof( save.comment   ));
    ch_convert( CH_DEFAULT, info->copyright, save.codepage, save.copyright, sizeof( save.copyright ));
    ch_convert( CH_DEFAULT, info->genre,     save.codepage, save.genre,     sizeof( save.genre     ));

    rc = decoder->decoder_saveinfo((char*)filename, &save );
  } else {
    rc = decoder->decoder_saveinfo((char*)filename, info  );
  }

  pg_plugin_no_used((PLUGIN*)decoder );
  return rc;
}

ULONG DLLENTRY
dec_cdinfo( char* drive, DECODER_CDINFO* info )
{
  ULONG    rc = PLUGIN_NO_READ;
  DECODER* decoder;
  int      i;

  pg_request();

  for( i = 0; i < num_decoders; i++ )
  {
    if( decoders[i]->pc.enabled &&
        decoders[i]->decoder_cdinfo &&
      ( decoders[i]->support & DECODER_TRACK ))
    {
      decoder = decoders[i];

      pg_plugin_is_used((PLUGIN*)decoder );
      pg_release();
      rc = decoder->decoder_cdinfo( drive, info );
      pg_plugin_no_used((PLUGIN*)decoder );
      pg_request();
      break;
    }
  }

  pg_release();
  return rc;
}

ULONG DLLENTRY
dec_status( void )
{
  ULONG rc = DECODER_ERROR;

  pg_request();
  if( active_decoder != -1 ) {
    rc = decoders[active_decoder]->decoder_status( decoders[active_decoder]->data );
  }
  pg_release();
  return rc;
}

/* Length in ms, should still be valid if decoder stops. */
ULONG DLLENTRY
dec_length( void )
{
  ULONG rc = 0;

  pg_request();
  if( active_decoder != -1 ) {
    rc = decoders[active_decoder]->decoder_length( decoders[active_decoder]->data );
  }
  pg_release();
  return rc;
}

/* Fills specified buffer with the list of extensions of
   supported files. */
void DLLENTRY
dec_fill_types( char* result, size_t size )
{
  int i, j;

  *result = 0;

  pg_request();
  for( i = 0; i < num_decoders; i++ ) {
    if( decoders[i]->pc.enabled ) {
      for( j = 0; j < decoders[i]->fileext_size; j++ ) {
        strlcat( result, decoders[i]->fileext[j], size );
        strlcat( result, ";", size );
      }
    }
  }
  pg_release();

  if( *result && result[strlen( result )-1] == ';' ) {
    // Remove the last ";".
    result[strlen( result )-1] = 0;
  }
}

/* Is the specified decoder active. */
BOOL DLLENTRY
dec_is_active( const char* name )
{
  BOOL rc;

  pg_request();
  rc = ( active_decoder != -1 && stricmp( decoders[active_decoder]->pc.name, name ) == 0 );
  pg_release();
  return rc;
}

/* Returns PLUGIN_FAILED if the specified output is not found or
   if a error occured, otherwize returns PLUGIN_OK. */
LONG DLLENTRY
out_set_active( const char* name )
{
  OUTPUT* output;
  int i;

  pg_request();

  if( name != NULL )
  {
    i = pg_find_plugin( name, PLUGIN_OUTPUT );

    if( i == -1 ) {
      pg_release();
      return PLUGIN_FAILED;
    } else if( i == active_output ) {
      pg_release();
      return PLUGIN_OK;
    }
  }

  if( active_output != -1 ) {
    output = outputs[ active_output ];
    active_output = -1;
    pg_uninit_output( output );
  }

  if( name != NULL ) {
    if( pg_init_output( outputs[i] )) {
      active_output = i;
    }
  }

  pg_release();
  return PLUGIN_OK;
}

/* Returns PLUGIN_OK = ok,
           PLUGIN_NO_USABLE = no output active,
           others = return code from MMOS/2. */
ULONG DLLENTRY
out_command( ULONG msg, OUTPUT_PARAMS* ai )
{
  ULONG rc = PLUGIN_NO_USABLE;

  pg_request();
  if( active_output != -1 ) {
    rc = outputs[active_output]->output_command( outputs[active_output]->data, msg, ai );
  }
  pg_release();
  return rc;
}

/* Sets the audio volume to the specified level. */
ULONG DLLENTRY
out_set_volume( int volume )
{
  OUTPUT_PARAMS out_params = { 0 };

  out_params.volume    = volume;
  out_params.amplifier = 1.0;

  return out_command( OUTPUT_VOLUME, &out_params );
}

/* Returns PLUGIN_OK = ok,
           PLUGIN_FAILED = no output active,
           others = return code from MMOS/2. */
ULONG DLLENTRY
out_playing_samples( FORMAT_INFO* info, char* buffer, int size )
{
  ULONG rc = PLUGIN_FAILED;

  pg_request();
  if( active_output != -1 ) {
    rc = outputs[active_output]->output_playing_samples( outputs[active_output]->data, info, buffer, size );
  }
  pg_release();
  return rc;
}

/* Returns time in ms. */
ULONG DLLENTRY
out_playing_pos( void )
{
  ULONG rc = 0;

  pg_request();
  if( active_output != -1 ) {
    rc = outputs[active_output]->output_playing_pos( outputs[active_output]->data );
  }
  pg_release();
  return rc;
}

/* If the output is playing. */
BOOL DLLENTRY
out_playing_data( void )
{
  BOOL rc = FALSE;

  pg_request();
  if( active_output != -1 ) {
    rc = outputs[active_output]->output_playing_data( outputs[active_output]->data );
  }
  pg_release();
  return rc;
}

/* Is the specified output plug-in active. */
BOOL DLLENTRY
out_is_active( const char* name )
{
  BOOL rc;

  pg_request();
  rc = ( active_output != -1 && stricmp( outputs[active_output]->pc.name, name ) == 0 );
  pg_release();
  return rc;
}

/* Initializes the specified visual plug-in. */
BOOL DLLENTRY
vis_initialize( const char* name, HWND hwnd )
{
  VISUAL* visual;
  BOOL    rc;

  pg_request();

  if(( visual = (VISUAL*)pg_query_plugin( name, PLUGIN_VISUAL )) != NULL )
  {
    pg_plugin_is_used((PLUGIN*)visual );
    pg_release();
    rc = pg_init_visual( visual, hwnd );
    pg_plugin_no_used((PLUGIN*)visual );
    return rc;
  } else {
    pg_release();
    return FALSE;
  }
}

/* Initializes the all specified visual plug-ins. */
void DLLENTRY
vis_initialize_all( HWND hwnd, BOOL skinned )
{
  VISUAL* visual;
  int     i;

  pg_request();
  for( i = 0; i < num_visuals; i++ ) {
    if( visuals[i]->skin == skinned )
    {
      visual = visuals[i];
      pg_plugin_is_used((PLUGIN*)visual );
      pg_release();
      pg_init_visual( visual, hwnd );
      pg_request();
      pg_plugin_no_used((PLUGIN*)visual );
    }
  }
  pg_release();
}

/* Terminates the specified visual plug-in. */
BOOL DLLENTRY
vis_terminate( const char* name )
{
  VISUAL* visual;
  BOOL    rc;

  pg_request();

  if(( visual = (VISUAL*)pg_query_plugin( name, PLUGIN_VISUAL )) != NULL )
  {
    pg_plugin_is_used((PLUGIN*)visual );
    pg_release();
    rc = pg_uninit_visual( visual );
    pg_plugin_no_used((PLUGIN*)visual );
    return rc;
  } else {
    pg_release();
    return FALSE;
  }
}

/* Terminates the all specified visual plug-ins. */
void DLLENTRY
vis_terminate_all( BOOL skinned )
{
  VISUAL* visual;
  int     i;

  pg_request();
  for( i = 0; i < num_visuals; i++ ) {
    if( visuals[i]->skin == skinned )
    {
      visual = visuals[i];
      pg_plugin_is_used((PLUGIN*)visual );
      pg_release();
      pg_uninit_visual( visual );
      pg_request();
      pg_plugin_no_used((PLUGIN*)visual );
    }
  }
  pg_release();
}

/* Broadcats specified message to all enabled visual plug-ins. */
void DLLENTRY
vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  int i;

  pg_request();
  for( i = 0; i < num_visuals; i++ ) {
    if( visuals[i]->pc.enabled && visuals[i]->hwnd != NULLHANDLE ) {
      WinSendMsg( visuals[i]->hwnd, msg, mp1, mp2 );
    }
  }
  pg_release();
}

/* Backward compatibility */
BOOL DLLENTRY
decoder_playing( void )
{
  ULONG status = dec_status();

  return ( status == DECODER_PLAYING  ||
           status == DECODER_STARTING ||
           status == DECODER_PAUSED   || out_playing_data());
}

/* Returns a playing time of the current file, in seconds. */
int
time_played( void ) {
  return out_playing_pos() / 1000;
}

/* Returns a total playing time of the current file. */
int
time_total( void )
{
  int length = dec_length();

  if( length < 0 ) {
    return -1;
  } else {
    return length / 1000;
  }
}

/* Cleanups the plug-ins submenu. */
void
pg_cleanup_plugin_menu( HWND menu )
{
  int   size = mn_size( menu );
  short i;

  pg_request();

  while( size-- ) {
    i = mn_item_id( menu, 0 );
    free( mn_get_handle( menu, i ));
    mn_remove_item( menu, i );
  }
  pg_release();
}

/* Prepares the plug-ins submenu. */
void
pg_prepare_plugin_menu( HWND menu )
{
  int     i;
  char    item[512];
  PLUGIN* plugin;
  short   id = IDM_M_PLUGINS + 1;

  pg_request();
  pg_cleanup_plugin_menu( menu );

  if( num_decoders + num_outputs + num_visuals + num_filters == 0 )
  {
    mn_add_item( menu, IDM_M_PLUGINS + 1, "No plug-ins", FALSE, FALSE, NULL );
    pg_release();
    return;
  }

  // Visual plug-ins
  for( i = 0; i < num_visuals; i++, id++  ) {
    if(( plugin = malloc( sizeof( PLUGIN ))) != NULL ) {
      snprintf( item, sizeof( item ), "%s (%s)", visuals[i]->pc.info.desc, visuals[i]->pc.name );
      memcpy( plugin, visuals[i], sizeof( PLUGIN ));
      mn_add_item( menu, id, item, visuals[i]->pc.info.configurable,
                                   visuals[i]->pc.enabled, plugin );
    }
  }
  // Decoder plug-ins
  for( i = 0; i < num_decoders; i++, id++ ) {
    if(( plugin = malloc( sizeof( PLUGIN ))) != NULL ) {
      snprintf( item, sizeof( item ), "%s (%s)", decoders[i]->pc.info.desc, decoders[i]->pc.name );
      memcpy( plugin, decoders[i], sizeof( PLUGIN ));
      mn_add_item( menu, id, item, decoders[i]->pc.info.configurable,
                                   decoders[i]->pc.enabled, plugin );
    }
  }
  // Output plug-ins
  for( i = 0; i < num_outputs; i++, id++  ) {
    if(( plugin = malloc( sizeof( PLUGIN ))) != NULL ) {
      snprintf( item, sizeof( item ), "%s (%s)", outputs[i]->pc.info.desc, outputs[i]->pc.name );
      memcpy( plugin, outputs[i], sizeof( PLUGIN ));
      mn_add_item( menu, id, item, outputs[i]->pc.info.configurable,
                                   i == active_output, plugin );
    }
  }
  // Filter plug-ins
  for( i = 0; i < num_filters; i++, id++ ) {
    if(( plugin = malloc( sizeof( PLUGIN ))) != NULL ) {
      snprintf( item, sizeof( item ), "%s (%s)", filters[i]->pc.info.desc, filters[i]->pc.name );
      memcpy( plugin, filters[i], sizeof( PLUGIN ));
      mn_add_item( menu, id, item, filters[i]->pc.info.configurable,
                                   filters[i]->pc.enabled, plugin );
    }
  }

  pg_release();
  return;
}

/* Pocesses the plug-ins submenu. */
void
pg_process_plugin_menu( HWND hwnd, HWND menu, SHORT id )
{
  PLUGIN* plugin = (PLUGIN*)mn_get_handle( menu, id );

  if( plugin ) {
    pg_configure( plugin->name, plugin->info.type, hwnd );
  }
}

/** Initializes of the plug-ins manager. */
void
pg_init( void ) {
  DosCreateMutexSem( NULL, &mutex, 0, FALSE );
}

/** Terminates  of the plug-ins manager. */
void
pg_term( void )
{
  pg_remove_all_plugins();
  DosCloseMutexSem( mutex );
}

