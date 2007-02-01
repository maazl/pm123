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
  char    module_file[_MAX_PATH];
  char    module_name[16];
  BOOL    enabled;
  char    fileext[MAX_FILEEXT][8];
  int     fileext_size;
  ULONG   support;

  PLUGIN_QUERYPARAM query_param;

  void* w;
  int   (DLLENTRYP decoder_init     )( void** w );
  BOOL  (DLLENTRYP decoder_uninit   )( void*  w );
  ULONG (DLLENTRYP decoder_command  )( void*  w, ULONG msg, DECODER_PARAMS* params );
  ULONG (DLLENTRYP decoder_status   )( void*  w );
  ULONG (DLLENTRYP decoder_length   )( void*  w );
  ULONG (DLLENTRYP decoder_fileinfo )( char*  filename, DECODER_INFO *info );
  ULONG (DLLENTRYP decoder_saveinfo )( char*  filename, DECODER_INFO *info );
  ULONG (DLLENTRYP decoder_trackinfo)( char*  drive, int track, DECODER_INFO* info );
  ULONG (DLLENTRYP decoder_cdinfo   )( char*  drive, DECODER_CDINFO* info );
  ULONG (DLLENTRYP decoder_support  )( char*  ext[], int* size );

  void  (DLLENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (DLLENTRYP plugin_configure)( HWND hwnd, HMODULE module );

} DECODER;

typedef struct
{
  HMODULE module;
  char    module_file[_MAX_PATH];
  char    module_name[16];

  PLUGIN_QUERYPARAM query_param;

  void* a;
  ULONG (DLLENTRYP output_init           )( void** a );
  ULONG (DLLENTRYP output_uninit         )( void*  a );
  ULONG (DLLENTRYP output_command        )( void*  a, ULONG msg, OUTPUT_PARAMS* info );
  ULONG (DLLENTRYP output_playing_samples)( void*  a, FORMAT_INFO* info, char* buf, int len );
  int   (DLLENTRYP output_play_samples   )( void*  a, FORMAT_INFO* format, char* buf, int len, int posmarker );
  int   (DLLENTRYP output_playing_pos    )( void*  a );
  BOOL  (DLLENTRYP output_playing_data   )( void*  a );

  void  (DLLENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (DLLENTRYP plugin_configure)( HWND hwnd, HMODULE module );

} OUTPUT;

typedef struct
{
  HMODULE module;
  char    module_file[_MAX_PATH];
  char    module_name[16];
  BOOL    enabled;

  PLUGIN_QUERYPARAM query_param;

  void  *f;
  ULONG (DLLENTRYP filter_init        )( void** f, FILTER_PARAMS* params );
  BOOL  (DLLENTRYP filter_uninit      )( void*  f );
  int   (DLLENTRYP filter_play_samples)( void*  f, FORMAT_INFO* format, char *buf, int len, int posmarker );

  void  (DLLENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (DLLENTRYP plugin_configure)( HWND hwnd, HMODULE module );

} FILTER;

typedef struct
{
  HMODULE module;
  char    module_file[_MAX_PATH];
  char    module_name[16];
  int     x, y, cx, cy;
  BOOL    skin;
  BOOL    enabled;
  HWND    hwnd;
  char    param[256];
  BOOL    init;

  PLUGIN_QUERYPARAM query_param;

  HWND  (DLLENTRYP plugin_init     )( VISPLUGININIT* init );
  void  (DLLENTRYP plugin_query    )( PLUGIN_QUERYPARAM* param  );
  void  (DLLENTRYP plugin_configure)( HWND hwnd, HMODULE module );
  BOOL  (DLLENTRYP plugin_deinit   )( void* f );

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

ULONG DLLENTRY dec_command( ULONG msg, DECODER_PARAMS* params );
ULONG DLLENTRY dec_fileinfo( char* filename, DECODER_INFO* info, char* name );
ULONG DLLENTRY dec_saveinfo( char* filename, DECODER_INFO* info, char* name );
ULONG DLLENTRY dec_cdinfo( char* drive, DECODER_CDINFO* info );
ULONG DLLENTRY dec_status( void );
ULONG DLLENTRY dec_length( void );

int   out_set_name_active( char* name );
int   out_set_active( int number );
void  out_set_volume( int volume );

ULONG DLLENTRY out_command( ULONG msg, OUTPUT_PARAMS* info );
ULONG DLLENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len );
ULONG DLLENTRY out_playing_pos( void );
BOOL  DLLENTRY out_playing_data( void );

BOOL  vis_init( HWND hwnd, int i );
void  vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL  vis_deinit( int i );

/* Backward compatibility */
BOOL  DLLENTRY decoder_playing( void );

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

