#ifndef PM123_FILTER_PLUG_H
#define PM123_FILTER_PLUG_H

#include <format.h>
#include <output_plug.h>

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if PLUGIN_INTERFACE_LEVEL == 2
#error "The filter plug-in interface level 2 (PM123 1.40a) is no longer supported."
#endif

#pragma pack(4)


/****************************************************************************
 *
 * Definitions of level 1 interface
 *
 ***************************************************************************/
#if PLUGIN_INTERFACE_LEVEL < 2 || defined(PM123_CORE)

/* level 1 interface, for compatibility */
#define PARAMS_SIZE_1 24  /* size of the FILTER_PARAMS structure prior PM123 1.32 */
#define PARAMS_SIZE_2 32  /* size of the FILTER_PARAMS structure since PM123 1.32 */

typedef struct _FILTER_PARAMS
{
  int size;

  /* specify a function which the filter should use for output */
  int  DLLENTRYP(output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  void* a;  /* only to be used with the precedent function */
  int   audio_buffersize;

  /* error message function the filter plug-in should use */
  void DLLENTRYP(error_display)( const char* );
  /* info message function the filter plug-in should use */
  /* this information is always displayed to the user right away */
  void DLLENTRYP(info_display)( const char* );

  /* added since PM123 1.32 */
  int  DLLENTRYP(pm123_getstring)( int index, int subindex, size_t bufsize, char* buf );
  void DLLENTRYP(pm123_control)( int index, void* param );

} FILTER_PARAMS;

/*
 * int pm123_getstring( int index, int subindex, int bufsize, char* buf )
 *
 *  index    - which string (see STR_* defines below)
 *  subindex - not currently used
 *  bufsize  - bytes in buf
 *  buf      - buffer for the string
 */

#define STR_NULL         0
#define STR_VERSION      1 /* PM123 version          */
#define STR_DISPLAY_TEXT 2 /* Displayed text         */
#define STR_FILENAME     3 /* Currently loaded file  */
#define STR_DISPLAY_TAG  4 /* Displayed song info    */
#define STR_DISPLAY_INFO 5 /* Displayed tech info    */

/*
 * int pm123_control( int index, void* param );
 *
 *  index - operation
 *  param - parameter for the operation
 */

#define CONTROL_NEXTMODE 1 /* Next display mode */

#if PLUGIN_INTERFACE_LEVEL < 2
/* returns 0 -> ok */
ULONG DLLENTRY filter_init  ( void** f, FILTER_PARAMS* params );

/* Notice it is the same parameters as output_play_samples()  */
/* this makes it possible to plug a decoder plug-in in either */
/* a filter plug-in or directly in an output plug-in          */
/* BUT you will have to pass void *a from above to the next   */
/* stage which will be either a filter or output              */
int   DLLENTRY filter_play_samples( void* f, FORMAT_INFO* format, char* buf, int len, int posmarker );
BOOL  DLLENTRY filter_uninit( void*  f );
#endif

#endif /* level 1 interface */


/****************************************************************************
 *
 * Definitions of level 3 interface
 *
 ***************************************************************************/
#if PLUGIN_INTERFACE_LEVEL >= 3

typedef struct _FILTER_PARAMS2
{
  int size;

  /* virtual output interface
   * To virtualize one of these functions replace the pointer at the filter_init call. */
  ULONG  DLLENTRYP(output_command)(void* a, ULONG msg, OUTPUT_PARAMS2* info);
  ULONG  DLLENTRYP(output_playing_samples)(void* a, PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param);
  int    DLLENTRYP(output_request_buffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void   DLLENTRYP(output_commit_buffer)(void* a, int len, PM123_TIME posmarker);
  PM123_TIME DLLENTRYP(output_playing_pos)(void* a);
  BOOL   DLLENTRYP(output_playing_data)(void* a);
  void*  a;  /* only to be used with the precedent functions */
  
  /* callback event
   * To virtualize these function replace the pointer at the filter_init call. */
  void  DLLENTRYP(output_event)(void* w, OUTEVENTTYPE event);
  void* w;  /* only to be used with the precedent function */

} FILTER_PARAMS2;

/* returns 0 -> ok */
ULONG DLLENTRY filter_init  (void** f, FILTER_PARAMS2* params);
void  DLLENTRY filter_update(void* f, const FILTER_PARAMS2* params);
BOOL  DLLENTRY filter_uninit(void* f);

#endif /* Level 3 interface */


#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_FILTER_PLUG_H */

