#ifndef PM123_VISUAL_PLUG_H
#define PM123_VISUAL_PLUG_H

#include <format.h>

#if PLUGIN_INTERFACE_LEVEL < 3
#error "The old visual plug-in interface is no longer supported." \
  "Define the macro PLUGIN_INTERFACE_LEVEL to a value of at least 3" \
  "and don't forget to fill param->interface at plugin_query."
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* see decoder_plug.h and output_plug.h for more information
   on some of these functions */
typedef struct _PLUGIN_PROCS
{
  ULONG  DLLENTRYP(output_playing_samples)( FORMAT_INFO2* info, float* buf, int len );
  BOOL   DLLENTRYP(decoder_playing)( void );
  double DLLENTRYP(output_playing_pos)( void );
  ULONG  DLLENTRYP(decoder_status)( void );
  double DLLENTRYP(decoder_length)( void );

} PLUGIN_PROCS;

typedef struct _VISPLUGININIT
{
  int                 x, y, cx, cy;   /* Location where the plug-in should create its window */
  HWND                hwnd;           /* PM123's window handle */
  const PLUGIN_PROCS* procs;          /* Pointers to functions which plug-ins can utilize */
  const char*         param;          /* Parameters passed to the plug-in */
  HAB                 hab;            /* PM123's anchor block handle */

} VISPLUGININIT;

HWND DLLENTRY vis_init(const VISPLUGININIT* initdata);


#ifdef __cplusplus
}
#endif
#endif /* PM123_VISUAL_PLUG_H */
