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

#ifndef PM123_PLUGMAN_H
#define PM123_PLUGMAN_H

/* maximum supported and most recent plugin-levels */
#define MAX_PLUGIN_LEVEL     2
#define VISUAL_PLUGIN_LEVEL  1
#define FILTER_PLUGIN_LEVEL  2
#define DECODER_PLUGIN_LEVEL 2
#define OUTPUT_PLUGIN_LEVEL  2

#include <plugin.h>
#include <output_plug.h>
#include <filter_plug.h>
#include <decoder_plug.h>
#include <utilfct.h>


typedef struct
{
  HMODULE module;
  char    module_name[_MAX_PATH];
  PLUGIN_QUERYPARAM query_param;

} PLUGIN_BASE;

typedef struct
{
  int     x, y, cx, cy;
  BOOL    skin;
  char    param[256];

} VISUAL_PROPERTIES;

#ifdef __cplusplus
extern "C" {
#endif

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

BOOL  load_decoders( BUFSTREAM* b );
BOOL  load_outputs ( BUFSTREAM* b );
BOOL  load_filters ( BUFSTREAM* b );
BOOL  load_visuals ( BUFSTREAM* b );
BOOL  save_decoders( BUFSTREAM* b );
BOOL  save_outputs ( BUFSTREAM* b );
BOOL  save_filters ( BUFSTREAM* b );
BOOL  save_visuals ( BUFSTREAM* b );

ULONG add_plugin( const char* module_name, const VISUAL_PROPERTIES* data );

int   enum_decoder_plugins(PLUGIN_BASE*const** list);
int   enum_output_plugins (PLUGIN_BASE*const** list);
int   enum_filter_plugins (PLUGIN_BASE*const** list);
int   enum_visual_plugins (PLUGIN_BASE*const** list);

BOOL  get_plugin_enabled(const PLUGIN_BASE* plugin);
void  set_plugin_enabled(PLUGIN_BASE* plugin, BOOL enabled);
void  configure_plugin(PLUGIN_BASE* plugin, HWND hwnd);


typedef enum
{ DECODER_NORMAL_PLAY,
  DECODER_FAST_FORWARD,
  DECODER_FAST_REWIND
} DECODER_FAST_MODE;

/* invoke decoder to play an URL */
ULONG dec_play( const char* url, const char* decoder_name );
/* stop the current decoder immediately */
ULONG dec_stop( void );
/* set fast forward/rewind mode */
ULONG dec_fast( DECODER_FAST_MODE mode );
/* jump to absolute position */
ULONG dec_jump( int location );
/* set equalizer parameters */
ULONG dec_eq  ( const float* bandgain );
/* set savefilename to save the raw stream data */
ULONG dec_save( const char* file );
/* edit ID3-data of the given file, decoder_name is optional */
ULONG dec_editmeta( HWND owner, const char* url, const char* decoder_name );

/* check whether the specified decoder is currently in use */
BOOL  dec_is_active( int number );
/* gets a merged list of the file types supported by the enabled decoders */
void  dec_fill_types( char* result, size_t size );

ULONG PM123_ENTRY dec_fileinfo( const char* filename, DECODER_INFO2* info, char* name );
ULONG PM123_ENTRY dec_cdinfo( char* drive, DECODER_CDINFO* info );
ULONG PM123_ENTRY dec_status( void );
ULONG PM123_ENTRY dec_length( void );

/* output control interface */
BOOL  out_is_active( int number );
int   out_set_active( int number );
ULONG out_setup( const FORMAT_INFO2* formatinfo, const char* URI );
ULONG out_close( void );
void  out_set_volume( double volume ); // volume: [0,1]
ULONG out_pause( BOOL pause );
void  out_trashbuffers( int temp_playingpos );
BOOL  out_flush( void );

/*ULONG PM123_ENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len );*/
ULONG PM123_ENTRY out_playing_pos( void );
BOOL  PM123_ENTRY out_playing_data( void );

/* initialize visual plug-in */
BOOL  vis_init( int i );
void  vis_init_all( BOOL skin );
void  vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 );
/* deinitialize visual plug-in */
BOOL  vis_deinit( int i );
void  vis_deinit_all( BOOL skin );

/* Backward compatibility */
BOOL  PM123_ENTRY decoder_playing( void );

/* Plug-in menu in the main pop-up menu */
void  load_plugin_menu( HWND hmenu );
/* Add additional entries in load/add menu in the main and the playlist's pop-up menu */
void  append_load_menu( HWND hMenu, ULONG id_base, BOOL multiselect, DECODER_ASSIST_FUNC* callbacks, int size );
BOOL  process_possible_plugin( HWND hwnd, USHORT cmd );

#ifdef __cplusplus
}
#endif
#endif /* PM123_PLUGMAN_H */

