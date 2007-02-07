#ifndef PORTABLE_SNPRINTF_H
#define PORTABLE_SNPRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#if defined(__GNUC__) || defined(__WATCOMC__)
#include <stdio.h>
#else
extern int snprintf ( char*, size_t, const char*, /*args*/ ... );
extern int vsnprintf( char*, size_t, const char*, va_list );
#endif
#endif

#ifdef __cplusplus
}
#endif