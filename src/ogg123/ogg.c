#include <stdlib.h>
#include <memory.h>

/* This is dummy file for VAC++. */
void dummy( void )
{}

#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)

int  _CRT_init( void );
void _CRT_term( void );

unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return 1UL;
  } else {
    _dump_allocated( 0 );
    _CRT_term();
    return 1UL;
  }
}
#endif
