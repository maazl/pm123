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

#include "format.h"
#include "decoder_plug.h"
#include "output_plug.h"
#include "filter_plug.h"
#include "plugin.h"
#include "utilfct.h"

#define  MAX_FILEEXT 32

typedef struct
{
  HMODULE module;
  char    module_name[_MAX_PATH];
  BOOL    enabled;
  char    fileext[MAX_FILEEXT][8];
  int     fileext_size;
  ULONG   support;

  PLUGIN_QUERYPARAM query_param;

  void* w;
  int   (PM123_ENTRYP decoder_init     )( void** w );
  BOOL  (PM123_ENTRYP decoder_uninit   )( void*  w );
  ULONG (PM123_ENTRYP decoder_command  )( void*  w, ULONG msg, DECODER_PARAMS* params );
  ULONG (PM123_ENTRYP decoder_status   )( void*  w );
  ULONG (PM123_ENTRYP decoder_length   )( void*  w );
  ULONG (PM123_ENTRYP decoder_fileinfo )( char*  filename, DECODER_INFO *info );
  ULONG (PM123_ENTRYP decoder_trackinfo)( char*  drive, int track, DECODER_INFO* info );
  ULONG (PM123_ENTRYP decoder_cdinfo   )( char*  drive, DECODER_CDINFO* info );
  ULONG (PM123_ENTRYP decoder_support  )( char*  ext[], int* size );

  void  (PM123_ENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (PM123_ENTRYP plugin_configure)( HWND hwnd, HMODULE module );

} DECODER;

typedef struct
{
  HMODULE module;
  char    module_name[_MAX_PATH];

  PLUGIN_QUERYPARAM query_param;

  void* a;
  ULONG (PM123_ENTRYP output_init           )( void** a );
  ULONG (PM123_ENTRYP output_uninit         )( void*  a );
  ULONG (PM123_ENTRYP output_command        )( void*  a, ULONG msg, OUTPUT_PARAMS* info );
  ULONG (PM123_ENTRYP output_playing_samples)( void*  a, FORMAT_INFO* info, char* buf, int len );
  int   (PM123_ENTRYP output_play_samples   )( void*  a, FORMAT_INFO* format, char* buf, int len, int posmarker );
  int   (PM123_ENTRYP output_playing_pos    )( void*  a );
  BOOL  (PM123_ENTRYP output_playing_data   )( void*  a );

  void  (PM123_ENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (PM123_ENTRYP plugin_configure)( HWND hwnd, HMODULE module );

} OUTPUT;

typedef struct
{
  HMODULE module;
  char    module_name[_MAX_PATH];
  BOOL    enabled;

  PLUGIN_QUERYPARAM query_param;

  void  *f;
  ULONG (PM123_ENTRYP filter_init        )( void** f, FILTER_PARAMS* params );
  BOOL  (PM123_ENTRYP filter_uninit      )( void*  f );
  int   (PM123_ENTRYP filter_play_samples)( void*  f, FORMAT_INFO* format, char *buf, int len, int posmarker );

  void  (PM123_ENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (PM123_ENTRYP plugin_configure)( HWND hwnd, HMODULE module );

} FILTER;

typedef struct
{
  HMODULE module;
  char    module_name[_MAX_PATH];
  int     x, y, cx, cy;
  BOOL    skin;
  BOOL    enabled;
  HWND    hwnd;
  char    param[256];
  BOOL    init;

  PLUGIN_QUERYPARAM query_param;

  HWND  (PM123_ENTRYP plugin_init     )( VISPLUGININIT* init );
  void  (PM123_ENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (PM123_ENTRYP plugin_configure)( HWND hwnd, HMODULE module );
  BOOL  (PM123_ENTRYP plugin_deinit   )( void* f );

} VISUAL;

// These externs are not supposed to be used to make stupid things,
// but only to READ for configuration purposes, or set enabled flag.
extern DECODER* decoders;
extern int num_decoders;
extern int active_decoder;
extern OUTPUT* outputs;
extern int num_outputs;
extern int active_output;
extern FILTER* filters;
extern int num_filters;
extern VISUAL* visuals;
extern int num_visuals;

#ifdef __cplusplus
extern "C" {
#endif

BOOL  remove_decoder_plugin( DECODER* plugin );
BOOL  remove_output_plugin ( OUTPUT*  plugin );
BOOL  remove_filter_plugin ( FILTER*  plugin );
BOOL  remove_visual_plugin ( VISUAL*  plugin );
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

ULONG add_plugin( const char* module_name, const VISUAL* data );

int   dec_set_name_active( char* name );
int   dec_set_active( int number );
void  dec_fill_types( char* result, size_t size );

ULONG PM123_ENTRY dec_command( ULONG msg, DECODER_PARAMS* params );
ULONG PM123_ENTRY dec_fileinfo( char* filename, DECODER_INFO* info, char* name );
ULONG PM123_ENTRY dec_cdinfo( char* drive, DECODER_CDINFO* info );
ULONG PM123_ENTRY dec_status( void );
ULONG PM123_ENTRY dec_length( void );

int   out_set_name_active( char* name );
int   out_set_active( int number );
void  out_set_volume( int volume );

ULONG PM123_ENTRY out_command( ULONG msg, OUTPUT_PARAMS* info );
ULONG PM123_ENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len );
ULONG PM123_ENTRY out_playing_pos( void );
BOOL  PM123_ENTRY out_playing_data( void );

BOOL  vis_init( HWND hwnd, int i );
void  vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL  vis_deinit( int i );

/* Backward compatibility */
BOOL  PM123_ENTRY decoder_playing( void );

/* Returns a playing time of the current file, in seconds. */
int   time_played( void );
/* Returns a total playing time of the current file. */
int   time_total ( void );

/* Plug-in menu in the main pop-up menu */
void  load_plugin_menu( HWND hmenu );
BOOL  process_possible_plugin( HWND hwnd, USHORT cmd );

#ifdef __cplusplus
}
#endif
#endif /* PM123_PLUGMAN_H */

