#ifndef __PM123_OUTPUT_PLUG_H
#define __PM123_OUTPUT_PLUG_H

#include <format.h>

#if __cplusplus
extern "C" {
#endif

ULONG PM123_ENTRY output_init  ( void** a );
ULONG PM123_ENTRY output_uninit( void*  a );

#define OUTPUT_OPEN          1
#define OUTPUT_CLOSE         2
#define OUTPUT_VOLUME        3
#define OUTPUT_PAUSE         4
#define OUTPUT_SETUP         5
#define OUTPUT_TRASH_BUFFERS 6 // unused!
#define OUTPUT_NOBUFFERMODE  7

typedef struct _OUTPUT_PARAMS
{
  int size;

  /* --- OUTPUT_SETUP */

  FORMAT_INFO formatinfo;

  int buffersize;

  unsigned short boostclass, normalclass;
  signed   short boostdelta, normaldelta;

  void (PM123_ENTRYP error_display)( char* );

  /* info message function the output plug-in should use */
  /* this information is always displayed to the user right away */
  void (PM123_ENTRYP info_display)( char* );

  HWND hwnd; // commodity for PM interface, sends a few messages to this handle

  /* --- OUTPUT_VOLUME */

  char  volume;
  float amplifier;

  /* --- OUTPUT_PAUSE */

  BOOL  pause;

  /* --- OUTPUT_NOBUFFERMODE */

  BOOL  nobuffermode;

  /* --- OUTPUT_TRASH_BUFFERS */

  ULONG temp_playingpos;  // used until new buffers come in

  /* --- OUTPUT_OPEN */

  char* filename;         // filename, URL or track now being played,
                          // useful for disk output

  /* --- OUTPUT_SETUP ***OUTPUT*** */

  BOOL  always_hungry;    // the output plug-in imposes no time restraint
                          // it is always WM_OUTPUT_OUTOFDATA

} OUTPUT_PARAMS;

typedef enum
{ OUTEVENT_END_OF_DATA,
  OUTEVENT_LOW_WATER,
  OUTEVENT_HIGH_WATER,
} OUTEVENTTYPE; 

typedef struct _OUTPUT_PARAMS2
{
  int size;

  /* --- OUTPUT_SETUP */

  FORMAT_INFO formatinfo;
  /* Error handlers */
  void (PM123_ENTRYP error_display)( char* );
  /* info message function the output plug-in should use */
  /* this information is always displayed to the user right away */
  void (PM123_ENTRYP info_display)( char* );

  HWND hwnd; // Window handle of PM123, normally not required

  /* callback event */
  void (PM123_ENTRYP output_event)(void* w, OUTEVENTTYPE event); 
  void* w;  /* only to be used with the precedent function */

  /* --- OUTPUT_VOLUME */

  char  volume;
  float amplifier;

  /* --- OUTPUT_PAUSE */

  BOOL  pause;

  /* --- OUTPUT_TRASH_BUFFERS */

  ULONG temp_playingpos;  // used until new buffers come in

  /* --- OUTPUT_OPEN */

  char* URI;              // filename, URL or track now being played,
                          // useful for disk output

} OUTPUT_PARAMS2;

#if !defined(OUTPUT_PLUGIN_LEVEL) || OUTPUT_PLUGIN_LEVEL == 1 
ULONG PM123_ENTRY output_command( void* a, ULONG msg, OUTPUT_PARAMS* info );
int   PM123_ENTRY output_play_samples( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
ULONG PM123_ENTRY output_playing_pos( void* a );
#else
ULONG PM123_ENTRY output_command( void* a, ULONG msg, OUTPUT_PARAMS2* info );
int   PM123_ENTRY output_request_buffer( void* a, const FORMAT_INFO* format, char** buf, int posmarker, const char* uri );
void  PM123_ENTRY output_commit_buffer( void* a, int len );
int   PM123_ENTRY output_playing_pos( void* a );
#endif
ULONG PM123_ENTRY output_playing_samples( void* a, FORMAT_INFO* info, char* buf, int len );
BOOL  PM123_ENTRY output_playing_data( void* a );

#if __cplusplus
}
#endif
#endif /* __PM123_OUTPUT_PLUG_H */

