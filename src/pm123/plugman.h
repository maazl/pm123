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

#ifndef PM123_PLUGMAN_H
#define PM123_PLUGMAN_H

/* maximum supported and most recent plugin-levels */
#define MAX_PLUGIN_LEVEL     2
#define VISUAL_PLUGIN_LEVEL  1
#define FILTER_PLUGIN_LEVEL  2
#define DECODER_PLUGIN_LEVEL 2
#define OUTPUT_PLUGIN_LEVEL  2

#include "playable.h"
#include <plugin.h>
#include <format.h>
#include <output_plug.h>
#include <filter_plug.h>
#include <decoder_plug.h>
#include <visual_plug.h>
#include <utilfct.h>
#include <cpp/event.h>

typedef struct
{ HMODULE module;
  char    module_name[_MAX_PATH];
  PLUGIN_QUERYPARAM query_param;
} PLUGIN_BASE;

typedef struct
{ int     x, y, cx, cy;
  BOOL    skin;
  char    param[256];
} VISUAL_PROPERTIES;

typedef struct
{ PLUGIN_BASE* plugin;
  PLUGIN_TYPE type;
  enum event
  { Load,
    Unload,
    Enable,
    Disable,
    Init,
    Uninit
  } operation;
} PLUGIN_EVENTARGS;


/****************************************************************************
*
*  Administrative interface of plug-in manager
*  Not thread safe!
*
****************************************************************************/
BOOL  remove_decoder_plugin( int i );
BOOL  remove_output_plugin ( int i );
BOOL  remove_filter_plugin ( int i );
BOOL  remove_visual_plugin ( int i );
void  remove_visual_plugins( BOOL skin );
void  remove_all_plugins   ( void );

void  load_default_decoders( void );
void  load_default_outputs ( void );
void  load_default_filters ( void );
void  load_default_visuals ( void );

// (de-)serialize currently loaded plugins.
BOOL  load_decoders( BUFSTREAM* b );
BOOL  load_outputs ( BUFSTREAM* b );
BOOL  load_filters ( BUFSTREAM* b );
BOOL  load_visuals ( BUFSTREAM* b );
BOOL  save_decoders( BUFSTREAM* b );
BOOL  save_outputs ( BUFSTREAM* b );
BOOL  save_filters ( BUFSTREAM* b );
BOOL  save_visuals ( BUFSTREAM* b );

ULONG add_plugin( const char* module_name, const VISUAL_PROPERTIES* data );

extern event<const PLUGIN_EVENTARGS> plugin_event;


/****************************************************************************
*
*  Configuration interface of plug-in manager
*  Not thread safe
*
****************************************************************************/
int   enum_decoder_plugins(PLUGIN_BASE*const** list);
int   enum_output_plugins (PLUGIN_BASE*const** list);
int   enum_filter_plugins (PLUGIN_BASE*const** list);
int   enum_visual_plugins (PLUGIN_BASE*const** list);

BOOL  get_plugin_enabled(const PLUGIN_BASE* plugin);
void  set_plugin_enabled(PLUGIN_BASE* plugin, BOOL enabled);

/* launch the configue dialog of the n-th plugin of a certain type. Use PLUGIN_NULL to use an index in the global plug-in list */ 
BOOL  configure_plugin( int type, int i, HWND hwnd );

/****************************************************************************
*
*  Control interface for the decoder engine
*  Not thread safe
*
****************************************************************************/
/* invoke decoder to play an URL */
ULONG dec_play( const Song* song, double offset, double start, double stop );
/* stop the current decoder immediately */
ULONG dec_stop( void );
/* set fast forward/rewind mode */
ULONG dec_fast( DECFASTMODE mode );
/* jump to absolute position */
ULONG dec_jump( double location );
/* set equalizer parameters */
ULONG dec_eq  ( const float* bandgain );
/* set savefilename to save the raw stream data */
ULONG dec_save( const char* file );
/* edit ID3-data of the given file, decoder_name is optional */
ULONG dec_editmeta( HWND owner, const char* url, const char* decoder_name );
/* get the minimum sample position of a block from the decoder since the last dec_play */
double dec_minpos();
/* get the maximum sample position of a block from the decoder since the last dec_play */
double dec_maxpos();
// Decoder events
typedef struct
{ DECEVENTTYPE type;
  void*        param;
} dec_event_args;
extern event<const dec_event_args> dec_event;
// Output events
extern event<const OUTEVENTTYPE> out_event;

/****************************************************************************
*
*  Status interface for the decoder engine
*  Thread safe
*
****************************************************************************/
/* check whether the specified decoder is currently in use */
BOOL  dec_is_active( int number );

ULONG DLLENTRY dec_fileinfo( const char* filename, DECODER_INFO2* info, char* name );
ULONG DLLENTRY dec_cdinfo( const char* drive, DECODER_CDINFO* info );
ULONG DLLENTRY dec_status( void );
double DLLENTRY dec_length( void );

/* gets a merged list of the file types supported by the enabled decoders */
void  dec_fill_types( char* result, size_t size );
/* Add additional entries in load/add menu in the main and the playlist's pop-up menu */
void  dec_append_load_menu( HWND hMenu, ULONG id_base, SHORT where, DECODER_WIZZARD_FUNC* callbacks, int size );
/* Append accelerator table with plug-in specific entries */
void  dec_append_accel_table( HACCEL& haccel, ULONG id_base, LONG offset, DECODER_WIZZARD_FUNC* callbacks, int size );

/****************************************************************************
*
*  Control interface for the output engine
*  Not thread safe
*
****************************************************************************/
BOOL  out_is_active( int number );
int   out_set_active( int number );
ULONG out_setup( const Song* song );
ULONG out_close( void );
void  out_set_volume( double volume ); // volume: [0,1]
ULONG out_pause( BOOL pause );
BOOL  out_flush( void );
BOOL  out_trash( void );

/****************************************************************************
*
*  Status interface for the output engine
*  Thread safe
*
****************************************************************************/
/*ULONG DLLENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len );*/
double DLLENTRY out_playing_pos( void );
BOOL  DLLENTRY out_playing_data( void );

/* Backward compatibility */
BOOL  DLLENTRY decoder_playing( void );

/****************************************************************************
*
*  Control interface for the isual plug-ins
*  Not thread safe
*
****************************************************************************/
/* initialize visual plug-in */
BOOL  vis_init( int i );
void  vis_init_all( BOOL skin );
void  vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 );
/* deinitialize visual plug-in */
BOOL  vis_deinit( int i );
void  vis_deinit_all( BOOL skin );

/****************************************************************************
*
*  GUI extension interface of the plug-ins
*
****************************************************************************/
/* Plug-in menu in the main pop-up menu */
void  load_plugin_menu( HWND hmenu );


/****************************************************************************
*
*  Global initialization functions
*
****************************************************************************/

/* Initialize plug-in manager */
void  plugman_init();
/* Deinitialize plug-in manager */
void  plugman_uninit();

#endif /* PM123_PLUGMAN_H */

