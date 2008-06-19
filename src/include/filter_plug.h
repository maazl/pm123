#ifndef PM123_FILTER_PLUG_H
#define PM123_FILTER_PLUG_H

#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

#define PARAMS_SIZE_1 24  /* size of the FILTER_PARAMS structure prior PM123 1.32 */
#define PARAMS_SIZE_2 32  /* size of the FILTER_PARAMS structure since PM123 1.32 */

typedef struct _FILTER_PARAMS
{
  int size;

  /* specify a function which the filter should use for output */
  int  (DLLENTRYP output_play_samples)( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
  void* a;  /* only to be used with the precedent function */
  int   audio_buffersize;

  /* error message function the filter plug-in should use */
  void (DLLENTRYP error_display)( char* );

  /* info message function the filter plug-in should use */
  /* this information is always displayed to the user right away */
  void (DLLENTRYP info_display)( char* );

  /* added since PM123 1.32 */
  int   (DLLENTRYP pm123_getstring)( int index, int subindex, int bufsize, char* buf );
  void  (DLLENTRYP pm123_control)( int index, void* param );

} FILTER_PARAMS;

/* returns 0 -> ok */
ULONG DLLENTRY filter_init  ( void** f, FILTER_PARAMS* params );
BOOL  DLLENTRY filter_uninit( void*  f );


/* Notice it is the same parameters as output_play_samples()  */
/* this makes it possible to plug a decoder plug-in in either */
/* a filter plug-in or directly in an output plug-in          */
/* BUT you will have to pass void *a from above to the next   */
/* stage which will be either a filter or output              */
int  DLLENTRY filter_play_samples( void* f, FORMAT_INFO* format, char* buf, int len, int posmarker );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_FILTER_PLUG_H */
