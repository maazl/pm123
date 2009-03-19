#ifndef PM123_PLUGIN_H
#define PM123_PLUGIN_H

#include <format.h>
#include <decoder_plug.h>
#include <stdlib.h>

#define PLUGIN_OK           0
#define PLUGIN_UNSUPPORTED  1
#define PLUGIN_NO_READ      100
#define PLUGIN_NO_PLAY      200
#define PLUGIN_NO_OP        300
#define PLUGIN_GO_ALREADY   101
#define PLUGIN_GO_FAILED    102
#define PLUGIN_FAILED       ((unsigned long)-1)
#define PLUGIN_NO_USABLE    ((unsigned long)-2)

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

typedef enum
{ PLUGIN_NULL    = 0x000,
  PLUGIN_VISUAL  = 0x001,
  PLUGIN_FILTER  = 0x002,
  PLUGIN_DECODER = 0x004,
  PLUGIN_OUTPUT  = 0x008
} PLUGIN_TYPE;

typedef struct _PLUGIN_QUERYPARAM
{
  int   type;         /* null, visual, filter, input. values can be ORed */
  char* author;       /* Author of the plugin                            */
  char* desc;         /* Description of the plugin                       */
  int   configurable; /* Is the plugin configurable                      */
  int   interface;    /* Interface revision                              */

} PLUGIN_QUERYPARAM, *PPLUGIN_QUERYPARAM;

typedef struct _PLUGIN_CONTEXT
{
  /* error message function */
  void DLLENTRYP(error_display)( const char* msg );

  /* info message function */
  /* this information is always displayed to the user right away */
  void DLLENTRYP(info_display)( const char* msg );

  /* execute remote command */
  /* See the documentation of remote commands for a description. */
  const char* DLLENTRYP(exec_command)( const char* cmd );
  
  /* retrieve configuration setting */
  int DLLENTRYP(query_profile)( const char* key, void* data, int maxlength );
  
  /* store configuration setting */
  int DLLENTRYP(write_profile)( const char* key, const void* data, int length );

} PLUGIN_CONTEXT;

#if (!defined(VISUAL_PLUGIN_LEVEL) || VISUAL_PLUGIN_LEVEL < 1) \
 && !defined(PM123_FILTER_PLUG_H) && !defined(PM123_DECODER_PLUG_H) && !defined(PM123_OUTPUT_PLUG_H)
#error The old plug-in interface is no longer supported. \
  Define the macro VISUAL_PLUGIN_LEVEL to a value of at least one \
  and donït forget to fill param->interface at plugin_query.
#endif
/* returns 0 -> ok */
int DLLENTRY plugin_query( PLUGIN_QUERYPARAM* param );
int DLLENTRY plugin_init( const PLUGIN_CONTEXT* ctx ); // Optional
int DLLENTRY plugin_configure( HWND hwnd, HMODULE module );
int DLLENTRY plugin_deinit( int unload );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_PLUGIN_H */
