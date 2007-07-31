#ifndef PM123_FILTER_PLUG_H
#define PM123_FILTER_PLUG_H

#include <format.h>
#include <output_plug.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

/* level 1 interface, for compatibility */
typedef struct _FILTER_PARAMS
{
  int size;

  /* specify a function which the filter should use for output */
  int  (DLLENTRYP output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  void* a;  /* only to be used with the precedent function */
  int   audio_buffersize;

  /* error message function the filter plug-in should use */
  void (DLLENTRYP error_display)( const char* );
  /* info message function the filter plug-in should use */
  /* this information is always displayed to the user right away */
  void (DLLENTRYP info_display)( const char* );

} FILTER_PARAMS;

/* level 2 interface (current) */
typedef struct _FILTER_PARAMS2
{
  int size;

  /* virtual output interface
   * To virtualize one of these functions replace the pointer at the filter_init call. */
  ULONG (DLLENTRYP output_command)( void* a, ULONG msg, OUTPUT_PARAMS2* info );
  ULONG (DLLENTRYP output_playing_samples)( void* a, FORMAT_INFO* info, char* buf, int len );
  int   (DLLENTRYP output_request_buffer)( void* a, const FORMAT_INFO2* format, short** buf );
  void  (DLLENTRYP output_commit_buffer)( void* a, int len, double posmarker );
  double(DLLENTRYP output_playing_pos)( void* a );
  BOOL  (DLLENTRYP output_playing_data)( void* a );
  void* a;  /* only to be used with the precedent functions */
  
  /* callback event
   * To virtualize these function replace the pointer at the filter_init call. */
  void  (DLLENTRYP output_event)( void* w, OUTEVENTTYPE event ); 
  void* w;  /* only to be used with the precedent function */

  /* error message function the filter plug-in should use */
  void (DLLENTRYP error_display)( const char* );
  /* info message function the filter plug-in should use */
  /* this information is always displayed to the user right away */
  void (DLLENTRYP info_display )( const char* );

} FILTER_PARAMS2;

/* returns 0 -> ok */
#if !defined(FILTER_PLUGIN_LEVEL) || FILTER_PLUGIN_LEVEL == 1
ULONG DLLENTRY filter_init  ( void** f, FILTER_PARAMS* params );

/* Notice it is the same parameters as output_play_samples()  */
/* this makes it possible to plug a decoder plug-in in either */
/* a filter plug-in or directly in an output plug-in          */
/* BUT you will have to pass void *a from above to the next   */
/* stage which will be either a filter or output              */
int   DLLENTRY filter_play_samples( void* f, FORMAT_INFO* format, char* buf, int len, int posmarker );

#else /* interface level 2 */
ULONG DLLENTRY filter_init  ( void** f, FILTER_PARAMS2* params );
void  DLLENTRY filter_update( void*  f, const FILTER_PARAMS2* params );

#endif
BOOL  DLLENTRY filter_uninit( void*  f );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_FILTER_PLUG_H */

