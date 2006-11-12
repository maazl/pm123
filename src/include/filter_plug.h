#ifndef PM123_FILTER_PLUG_H
#define PM123_FILTER_PLUG_H

#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FILTER_PARAMS
{
  int size;

  /* specify a function which the filter should use for output */
  int  (PM123_ENTRYP output_play_samples)( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
  void* a;  /* only to be used with the precedent function */
  int   audio_buffersize;

  /* error message function the filter plug-in should use */
  void (PM123_ENTRYP error_display)( char* );

  /* info message function the filter plug-in should use */
  /* this information is always displayed to the user right away */
  void (PM123_ENTRYP info_display)( char* );

} FILTER_PARAMS;

/* returns 0 -> ok */
ULONG PM123_ENTRY filter_init  ( void** f, FILTER_PARAMS* params );
BOOL  PM123_ENTRY filter_uninit( void*  f );


/* Notice it is the same parameters as output_play_samples()  */
/* this makes it possible to plug a decoder plug-in in either */
/* a filter plug-in or directly in an output plug-in          */
/* BUT you will have to pass void *a from above to the next   */
/* stage which will be either a filter or output              */
int  PM123_ENTRY filter_play_samples( void* f, FORMAT_INFO* format, char* buf, int len, int posmarker );

#ifdef __cplusplus
}
#endif
#endif /* PM123_FILTER_PLUG_H */
