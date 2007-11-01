#ifndef PM123_OUTPUT_PLUG_H
#define PM123_OUTPUT_PLUG_H

#define INCL_BASE
#include <format.h>
#include <os2.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

ULONG DLLENTRY output_init  ( void** a );
ULONG DLLENTRY output_uninit( void*  a );

#define OUTPUT_OPEN          1
#define OUTPUT_CLOSE         2
#define OUTPUT_VOLUME        3
#define OUTPUT_PAUSE         4
#define OUTPUT_SETUP         5
#define OUTPUT_TRASH_BUFFERS 6
#define OUTPUT_NOBUFFERMODE  7 /* obsolete */

typedef struct _OUTPUT_PARAMS
{
  int size;

  /* --- OUTPUT_SETUP */

  FORMAT_INFO formatinfo;

  int buffersize;

  unsigned short boostclass, normalclass;
  signed   short boostdelta, normaldelta;

  void DLLENTRYP(error_display)( const char* );

  /* info message function the output plug-in should use */
  /* this information is always displayed to the user right away */
  void DLLENTRYP(info_display)( const char* );

  HWND hwnd; /* commodity for PM interface, sends a few messages to this handle. */

  /* --- OUTPUT_VOLUME */

  unsigned char volume;
  float amplifier;

  /* --- OUTPUT_PAUSE */

  BOOL  pause;

  /* --- OUTPUT_NOBUFFERMODE */

  BOOL  nobuffermode;

  /* --- OUTPUT_TRASH_BUFFERS */

  ULONG temp_playingpos;  /* used until new buffers come in. */

  /* --- OUTPUT_OPEN */

  const char* filename;   /* filename, URL or track now being played, 
                             useful for disk output */

  /* --- OUTPUT_SETUP ***OUTPUT*** */

  BOOL  always_hungry;    /* the output plug-in imposes no time restraint
                             it is always WM_OUTPUT_OUTOFDATA */

} OUTPUT_PARAMS;

typedef enum
{ OUTEVENT_END_OF_DATA,   // The flush signal is passed to the ouput plugin and the last sample has been played
  OUTEVENT_LOW_WATER,     // The buffers of the output plug-in are getting low. Try to speed up the data source.
  OUTEVENT_HIGH_WATER,    // The buffers of the output plug-in are sufficiently filled.
  OUTEVENT_PLAY_ERROR     // The plug-in detected a fatal error stop immediately.
} OUTEVENTTYPE; 

typedef struct _OUTPUT_PARAMS2
{
  int size;

  /* --- OUTPUT_SETUP */

  FORMAT_INFO2 formatinfo;
  /* Error handler, a call will immediately stop the current playback. */
  void DLLENTRYP(error_display)( const char* );
  /* info message function the output plug-in should use */
  /* this information is always displayed to the user right away */
  void DLLENTRYP(info_display)( const char* );

  /* callback event */
  void DLLENTRYP(output_event)(void* w, OUTEVENTTYPE event); 
  void* w;  /* only to be used with the precedent function */

  /* --- OUTPUT_VOLUME */

  float volume;           // [0...1]
  float amplifier;

  /* --- OUTPUT_PAUSE */

  BOOL  pause;

  /* --- OUTPUT_TRASH_BUFFERS */

  double temp_playingpos;  // used until new buffers come in

  /* --- OUTPUT_OPEN */

  const char* URI;        // filename, URL or track now being played,
                          // useful for disk output

} OUTPUT_PARAMS2;

#if !defined(OUTPUT_PLUGIN_LEVEL) || OUTPUT_PLUGIN_LEVEL <= 1 
ULONG  DLLENTRY output_command( void* a, ULONG msg, OUTPUT_PARAMS* info );
int    DLLENTRY output_play_samples( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
ULONG  DLLENTRY output_playing_pos( void* a );
#else
ULONG  DLLENTRY output_command( void* a, ULONG msg, OUTPUT_PARAMS2* info );
int    DLLENTRY output_request_buffer( void* a, const FORMAT_INFO2* format, short** buf );
void   DLLENTRY output_commit_buffer( void* a, int len, double posmarker );
double DLLENTRY output_playing_pos( void* a );
#endif
ULONG  DLLENTRY output_playing_samples( void* a, FORMAT_INFO* info, char* buf, int len );
BOOL   DLLENTRY output_playing_data( void* a );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_OUTPUT_PLUG_H */

