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

#include <format.h>
#include <decoder_plug.h>
#include <output_plug.h>
#include <filter_plug.h>
#include <visual_plug.h>
#include <plugin.h>
#include <utilfct.h>

#define  _MAX_FILEEXT     32
#define  _MAX_MODULE_NAME 16

typedef struct _PLUGIN
{
  HMODULE module;
  char    file[_MAX_PATH];
  char    name[_MAX_MODULE_NAME];
  BOOL    enabled;
  BOOL    init;
  LONG    used;
  HMTX    mutex;

  PLUGIN_QUERYPARAM info;
  void  (DLLENTRYP plugin_configure)( HWND hwnd, HMODULE module );

} PLUGIN;

typedef struct _DECODER
{
  PLUGIN pc;
  char   fileext[_MAX_FILEEXT][8];
  int    fileext_size;
  ULONG  support;

  void*  data;
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

} DECODER;

typedef struct _OUTPUT
{
  PLUGIN pc;

  void*  data;
  ULONG (DLLENTRYP output_init           )( void** a );
  ULONG (DLLENTRYP output_uninit         )( void*  a );
  ULONG (DLLENTRYP output_command        )( void*  a, ULONG msg, OUTPUT_PARAMS* info );
  ULONG (DLLENTRYP output_playing_samples)( void*  a, FORMAT_INFO* info, char* buf, int len );
  int   (DLLENTRYP output_play_samples   )( void*  a, FORMAT_INFO* format, char* buf, int len, int posmarker );
  int   (DLLENTRYP output_playing_pos    )( void*  a );
  BOOL  (DLLENTRYP output_playing_data   )( void*  a );

} OUTPUT;

typedef struct _FILTER
{
  PLUGIN pc;

  void*  data;
  ULONG (DLLENTRYP filter_init        )( void** f, FILTER_PARAMS* params );
  BOOL  (DLLENTRYP filter_uninit      )( void*  f );
  int   (DLLENTRYP filter_play_samples)( void*  f, FORMAT_INFO* format, char *buf, int len, int posmarker );

} FILTER;

typedef struct _VISUAL
{
  PLUGIN pc;
  int    x, y, cx, cy;
  BOOL   skin;
  HWND   hwnd;
  char   param[256];

  HWND  (DLLENTRYP plugin_init  )( VISPLUGININIT* init );
  BOOL  (DLLENTRYP plugin_deinit)( void );

} VISUAL;

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes of the plug-ins manager. */
void pg_init( void );
/** Terminates  of the plug-ins manager. */
void pg_term( void );

/* Unloads and removes the specified plug-in from the list of loaded. */
BOOL pg_remove_decoder( const char* name );
BOOL pg_remove_output ( const char* name );
BOOL pg_remove_filter ( const char* name );
BOOL pg_remove_visual ( const char* name );

/* Unloads and removes all visual plug-ins from the list of loaded. */
void pg_remove_all_visuals( BOOL skinned );

/* Adds a default plug-ins to the list of loaded. */
void pg_load_default_decoders( void );
void pg_load_default_outputs ( void );
void pg_load_default_filters ( void );
void pg_load_default_visuals ( void );

/* Saves the current list of plug-ins. */
BOOL pg_save_decoders( BUFSTREAM* b );
BOOL pg_save_outputs ( BUFSTREAM* b );
BOOL pg_save_filters ( BUFSTREAM* b );
BOOL pg_save_visuals ( BUFSTREAM* b );

/* Restores the plug-ins list to the state was in when save was last called. */
BOOL pg_load_decoders( BUFSTREAM* b );
BOOL pg_load_outputs ( BUFSTREAM* b );
BOOL pg_load_filters ( BUFSTREAM* b );
BOOL pg_load_visuals ( BUFSTREAM* b );

/* Loads and adds the specified plug-in to the appropriate list of loaded.
   Returns the types found or PLUGIN_ERROR. */
int  pg_load_plugin( const char* filename, const VISUAL* data );

/* Invokes a specified plug-in configuration dialog. */
void pg_configure( const char* name, int type, HWND hwnd );
/* Adds plug-ins names to the specified list box control. */
void pg_expand_to( HWND hwnd, SHORT id, int type );
/* Returns an information about specified plug-in. */
void pg_get_info( const char* name, int type, PLUGIN_QUERYPARAM* info );
/* Returns TRUE if the specified plug-in is enabled. */
BOOL pg_is_enabled( const char* name, int type );
/* Enables the specified plug-in. */
BOOL pg_enable( const char* name, int type, BOOL enable );

/* Cleanups the plug-ins submenu. */
void pg_cleanup_plugin_menu( HWND menu );
/* Prepares the plug-ins submenu. */
void pg_prepare_plugin_menu( HWND menu );
/* Pocesses the plug-ins submenu. */
void pg_process_plugin_menu( HWND hwnd, HWND menu, SHORT id );

/* Decoders control functions. */
LONG  DLLENTRY dec_set_active( const char* name );
BOOL  DLLENTRY dec_set_filters( DECODER_PARAMS* params );
ULONG DLLENTRY dec_command( ULONG msg, DECODER_PARAMS* params );
ULONG DLLENTRY dec_fileinfo( const char* filename, DECODER_INFO* info, char* name );
ULONG DLLENTRY dec_saveinfo( const char* filename, DECODER_INFO* info, char* name );
ULONG DLLENTRY dec_trackinfo( char* drive, int track, DECODER_INFO* info, char* name );
ULONG DLLENTRY dec_cdinfo( char* drive, DECODER_CDINFO* info );
ULONG DLLENTRY dec_status( void );
ULONG DLLENTRY dec_length( void );
void  DLLENTRY dec_fill_types( char* result, size_t size );
BOOL  DLLENTRY dec_is_active( const char* name );
char* DLLENTRY dec_get_description( const char* name, char* result, int size );

/* Outputs control functions. */
LONG  DLLENTRY out_set_active( const char* name );
ULONG DLLENTRY out_command( ULONG msg, OUTPUT_PARAMS* info );
ULONG DLLENTRY out_set_volume( int volume );
ULONG DLLENTRY out_playing_samples( FORMAT_INFO* info, char* buffer, int size );
ULONG DLLENTRY out_playing_pos( void );
BOOL  DLLENTRY out_playing_data( void );
BOOL  DLLENTRY out_is_active( const char* name );

/* Visuals control functions. */
BOOL  DLLENTRY vis_initialize( const char* name, HWND hwnd );
void  DLLENTRY vis_initialize_all( HWND hwnd, BOOL skinned );
BOOL  DLLENTRY vis_terminate( const char* name );
void  DLLENTRY vis_terminate_all( BOOL skinned );
void  DLLENTRY vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 );

/* Backward compatibility */
BOOL  DLLENTRY decoder_playing( void );

/* Returns a playing time of the current file, in seconds. */
int time_played( void );
/* Returns a total playing time of the current file. */
int time_total ( void );

#ifdef __cplusplus
}
#endif
#endif /* PM123_PLUGMAN_H */

