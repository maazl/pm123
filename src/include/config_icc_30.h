#ifndef PM123_CONFIG_ICC_30_H
#define PM123_CONFIG_ICC_30_H

#define CCNAME "IBM Visualage C++ 3.0"

#ifdef __cplusplus
#if !defined( bool )
  typedef int   bool;
  #define true  1
  #define false 0
#endif
extern "C" {
#endif

int  _CRT_init( void );
void _CRT_term( void );

#ifdef __cplusplus
}
#endif

#define TFNENTRY  _Optlink
#define INLINE    _Inline
#define DLLENTRY  _System
#define DLLENTRYP(sym) (*DLLENTRY sym)
#define DLLENTRYPF() (*DLLENTRY)
#define DLLENTRYP2(sym) (DLLENTRY *sym)

#define INIT_ATTRIBUTE
#define TERM_ATTRIBUTE

#define HAVE_ALLOCA       1 /* Define to 1 if you have the `alloca' function. */
#define HAVE_MEMSET       1 /* Define to 1 if you have the `memset' function. */
#define HAVE_CALLOC       1 /* Define to 1 if you have the `calloc' function. */
#define HAVE_REALLOC      1 /* Define to 1 if you have the `realloc' function. */
#define HAVE_FREE         1 /* Define to 1 if you have the `free' function. */
#define HAVE_GMTIME       1 /* Define to 1 if you have the `gmtime' function. */
#define HAVE_CEIL         1 /* Define to 1 if you have the `ceil' function. */
#define HAVE_FLOOR        1 /* Define to 1 if you have the `floor' function. */
#define HAVE_FMOD         1 /* Define to 1 if you have the `fmod' function. */
#define HAVE_FSTAT        1 /* Define to 1 if you have the `fstat' function. */
#define HAVE_LSEEK        1 /* Define to 1 if you have the `lseek' function. */
#define HAVE_OPEN         1 /* Define to 1 if you have the `open' function. */
#define HAVE_READ         1 /* Define to 1 if you have the `read' function. */
#define HAVE_WRITE        1 /* Define to 1 if you have the `write' function. */
#define HAVE_SETLOCALE    1 /* Define to 1 if you have the `setlocale' function. */
#define HAVE_SNPRINTF     1 /* Define to 1 if you have the `snprintf' function. */
#define HAVE_VSNPRINTF    1 /* Define to 1 if you have the `vsnprintf' function. */
#undef  HAVE_GETTIMEOFDAY   /* Define to 1 if you have the `gettimeofday' function. */
#undef  HAVE_FSYNC          /* Define to 1 if you have the `fsync' function. */
#undef  HAVE_UINTPTR_T      /* Define to 1 if the system has the type `uintptr_t'. */
#undef  HAVE_DECL_S_IRGRP   /* Define to 1 if S_IRGRP is defined. */
#undef  HAVE_FTRUNCATE      /* Define to 1 if you have the `ftruncate' function. */
#undef  HAVE_LRINT          /* Define to 1 if you have C99's `lrint' function. */
#undef  HAVE_LRINTF         /* Define to 1 if you have C99's `lrintf' function. */
#undef  HAVE_SSIZE_T        /* Define to 1 if the system has the type `ssize_t'. */

#define HAVE_STDLIB_H     1 /* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STRING_H     1 /* Define to 1 if you have the <string.h> header file. */
#define HAVE_SYS_STAT_H   1 /* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_TIME_H   1 /* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TYPES_H  1 /* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_LIMITS_H     1 /* Define to 1 if you have the <limits.h> header file. */
#define HAVE_MEMORY_H     1 /* Define to 1 if you have the <memory.h> header file. */
#define HAVE_LOCALE_H     1 /* Define to 1 if you have the <locale.h> header file. */
#undef  HAVE_STDINT_H       /* Define to 1 if you have the <stdint.h> header file. */
#undef  HAVE_ALLOCA_H       /* Define to 1 if you have the <alloca.h> header file. */
#undef  HAVE_INTTYPES_H     /* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_SNPRINTF_H   1 /* Define to 1 if you have the <snprintf.h> header file. */

#define SIZEOF_DOUBLE             8 /* The size of a `double', as computed by sizeof. */
#define SIZEOF_FLOAT              4 /* The size of a `float', as computed by sizeof. */
#define SIZEOF_INT                4 /* The size of a `int', as computed by sizeof. */
#define SIZEOF_LONG               4 /* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG_DOUBLE       16 /* The size of a `long double', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT       4 /* The size of a `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG      8 /* The size of a `unsigned long', as computed by sizeof. */
#define SIZEOF_VOID_P             4 /* The size of a `void*', as computed by sizeof. */
#define SIZEOF_VOID_P_FAR         4 /* The size of a `void FAR*', as computed by sizeof. */
#define SIZEOF_SIZE_T             4 /* The size of a `size_t', as computed by sizeof. */
#define SIZEOF_OFF_T              4 /* The size of a `off_t', as computed by sizeof. */
#define SIZEOF_PTRDIFF_T          4 /* The size of a `ptrdiff_t', as computed by sizeof. */
#undef  SIZEOF_LONG_LONG            /* The size of a `long long', as computed by sizeof. */
#undef  SIZEOF_UNSIGNED_LONG_LONG   /* The size of a `unsigned long long', as computed by sizeof. */
#undef  SIZEOF_INT64_T              /* The size of a `int64_t', as computed by sizeof. */
#undef  SIZEOF_SSIZE_T              /* The size of a `ssize_t', as computed by sizeof. */

#define HAVE_UNSIGNED_ENUMS       1 /* Define to 1 if you have unsigned enums. */
#undef  HAVE_FLEXIBLE_ARRAY         /* Define to 1 if the compile supports the struct hack. */

#endif /* PM123_CONFIG_ICC_30_H */
