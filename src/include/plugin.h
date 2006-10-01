/*
   PM123 Plugin Definitions
   Copyright (C) 1998 Taneli Lepp„ <rosmo@sektori.com>
                      Samuel Audet <guardia@step.polymtl.ca>
*/

#ifndef __PM123_PLUGIN_H
#define __PM123_PLUGIN_H

#include <format.h>
#include <decoder_plug.h>

#define PLUGIN_NULL    0x000
#define PLUGIN_VISUAL  0x001
#define PLUGIN_FILTER  0x002
#define PLUGIN_DECODER 0x004
#define PLUGIN_OUTPUT  0x008

#ifdef __cplusplus
extern "C" {
#endif

/* see decoder_plug.h and output_plug.h for more information
   on some of these functions */
typedef struct _PLUGIN_PROCS
{
  ULONG (PM123_ENTRYP output_playing_samples)( FORMAT_INFO* info, char* buf, int len );
  BOOL  (PM123_ENTRYP decoder_playing)( void );
  ULONG (PM123_ENTRYP output_playing_pos)( void );
  ULONG (PM123_ENTRYP decoder_status)( void );
  /* name is the DLL filename of the decoder that can play that file */
  ULONG (PM123_ENTRYP decoder_fileinfo)( const char* URL, DECODER_INFO* info, char* name );

  int   (PM123_ENTRYP pm123_getstring)( int index, int subindex, size_t bufsize, char* buf );
  void  (PM123_ENTRYP pm123_control)( int index, void* param );

  /* name is the DLL filename of the decoder that can play that track */
  ULONG (PM123_ENTRYP decoder_cdinfo)( char* drive, DECODER_CDINFO* info );
  ULONG (PM123_ENTRYP decoder_length)( void );

} PLUGIN_PROCS, *PPLUGIN_PROCS;

/*
  int pm123_getstring( int index, int subindex, int bufsize, char* buf )

    index    - which string (see STR_* defines below)
    subindex - not currently used
    bufsize  - bytes in buf
    buf      - buffer for the string
*/

#define STR_NULL         0
#define STR_VERSION      1 /* PM123 version          */
#define STR_DISPLAY_TEXT 2 /* Display text           */
#define STR_FILENAME     3 /* Currently loaded file  */

/*
  int pm123_control( int index, void* param );

    index - operation
    param - parameter for the operation
*/

#define CONTROL_NEXTMODE 1 /* Next display mode */

typedef struct _VISPLUGININIT
{
  int           x, y, cx, cy;   /* Input        */
  HWND          hwnd;           /* Input/Output */
  PPLUGIN_PROCS procs;          /* Input        */
  int           id;             /* Input        */
  const char*   param;          /* Input        */

} VISPLUGININIT, *PVISPLUGININIT;

typedef struct _PLUGIN_QUERYPARAM
{
  int   type;         /* null, visual, filter, input. values can be ORred */
  char* author;       /* Author of the plugin                             */
  char* desc;         /* Description of the plugin                        */
  int   configurable; /* Is the plugin configurable                       */
  int   interface;    /* Interface revision                               */

} PLUGIN_QUERYPARAM, *PPLUGIN_QUERYPARAM;

#if (!defined(VISUAL_PLUGIN_LEVEL) || VISUAL_PLUGIN_LEVEL < 1) \
 && !defined(__PM123_FILTER_PLUG_H) && !defined(__PM123_DECODER_PLUG_H) && !defined(__PM123_OUTPUT_PLUG_H)
#error The old plug-in interface is no longer supported. \
  Define the macro VISUAL_PLUGIN_LEVEL to a value of at least one \
  and donït forget to fill param->interface at plugin_query.
#endif
/* returns 0 -> ok */
int PM123_ENTRY plugin_query( PLUGIN_QUERYPARAM* param );
int PM123_ENTRY plugin_configure( HWND hwnd, HMODULE module );

#ifdef __cplusplus
}
#endif
#endif /* __PM123_PLUGIN_H */
