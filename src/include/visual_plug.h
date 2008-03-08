#ifndef PM123_VISUAL_PLUG_H
#define PM123_VISUAL_PLUG_H

#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

/* see decoder_plug.h and output_plug.h for more information
   on some of these functions */
typedef struct _PLUGIN_PROCS
{
  ULONG (DLLENTRYP output_playing_samples)( FORMAT_INFO* info, char* buf, int len );
  BOOL  (DLLENTRYP decoder_playing)( void );
  ULONG (DLLENTRYP output_playing_pos)( void );
  ULONG (DLLENTRYP decoder_status)( void );
  ULONG (DLLENTRYP decoder_command)( ULONG msg, DECODER_PARAMS* params );
  /* name is the DLL filename of the decoder that can play that file */
  ULONG (DLLENTRYP decoder_fileinfo)( const char* filename, DECODER_INFO* info, char* name );

  int   (DLLENTRYP specana_init)( int setnumsamples );
  /* int specana_init(int setnumsamples);
   * setnumsamples must be a power of 2
   * Returns the number of bands in return (setnumsamples/2+1).
   */

  int   (DLLENTRYP specana_dobands)( float bands[] );
  /*
   * int specana_dobands(float bands[]);
   * Returns the max value.
   */

  int   (DLLENTRYP pm123_getstring)( int index, int subindex, int bufsize, char* buf );
  void  (DLLENTRYP pm123_control)( int index, void* param );

  /* name is the DLL filename of the decoder that can play that track */
  ULONG (DLLENTRYP decoder_trackinfo)( char* drive, int track, DECODER_INFO* info, char* name );
  ULONG (DLLENTRYP decoder_cdinfo)( char* drive, DECODER_CDINFO* info );
  ULONG (DLLENTRYP decoder_length)( void );

} PLUGIN_PROCS, *PPLUGIN_PROCS;

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

typedef struct _VISPLUGININIT
{
  int           x, y, cx, cy;   /* Input        */
  HWND          hwnd;           /* Input/Output */
  PPLUGIN_PROCS procs;          /* Input        */
  int           id;             /* Input        */
  char*         param;          /* Input        */
  HAB           hab;            /* Input        */

} VISPLUGININIT, *PVISPLUGININIT;

HWND DLLENTRY vis_init( PVISPLUGININIT initdata );
int  DLLENTRY plugin_deinit( void );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_VISUAL_PLUG_H */
