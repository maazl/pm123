#ifndef PM123_PLUGIN_H
#define PM123_PLUGIN_H

#include "format.h"

#define PLUGIN_NULL    0x000
#define PLUGIN_VISUAL  0x001
#define PLUGIN_FILTER  0x002
#define PLUGIN_DECODER 0x004
#define PLUGIN_OUTPUT  0x008

#define PLUGIN_OK           0
#define PLUGIN_UNSUPPORTED  1
#define PLUGIN_NO_READ      100
#define PLUGIN_NO_PLAY      200
#define PLUGIN_GO_ALREADY   101
#define PLUGIN_GO_FAILED    102
#define PLUGIN_FAILED      -1
#define PLUGIN_NO_USABLE   -2

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

typedef struct _PLUGIN_QUERYPARAM
{
  int   type;         /* null, visual, filter, input. values can be ORed */
  char* author;       /* Author of the plugin                            */
  char* desc;         /* Description of the plugin                       */
  int   configurable; /* Is the plugin configurable                      */

} PLUGIN_QUERYPARAM, *PPLUGIN_QUERYPARAM;

void DLLENTRY plugin_query( PLUGIN_QUERYPARAM* param );
void DLLENTRY plugin_configure( HWND hwnd, HMODULE module );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_PLUGIN_H */
