#ifndef PM123_CONFIG_H
#define PM123_CONFIG_H

/* Name of package */
#define PACKAGE_NAME "PM123"
#define PACKAGE PACKAGE_NAME

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "pm123@maazl.de"

/* Version number of package */
#define PACKAGE_VERSION "1.40"
#define VERSION PACKAGE_VERSION

/* Define to the full name and version of this package. */
#define PACKAGE_STRING PACKAGE" "VERSION

#if defined(__GNUC__)
  #include <config_gcc.h>
#elif defined(__WATCOMC__)
  #include <config_wcc.h>
#elif (defined(__IBMC__) && __IBMC__ >  300) || (defined(__IBMCPP__) && __IBMCPP__ >  300)
  #include <config_icc_36.h>
#elif (defined(__IBMC__) && __IBMC__ <= 300) || (defined(__IBMCPP__) && __IBMCPP__ <= 300)
  #include <config_icc_30.h>
#else
  #error Unsupported compiler.
#endif

/* Set to 1 if compiling for OS/2 */
#define OS_IS_OS2 1

/* Target processor clips on positive float to int conversion. */
#define CPU_CLIPS_POSITIVE   1
/* Target processor clips on negative float to int conversion. */
#define CPU_CLIPS_NEGATIVE   0
/* Target processor is big endian. */
#define CPU_IS_BIG_ENDIAN    0
/* Target processor is little endian. */
#define CPU_IS_LITTLE_ENDIAN 1

/* FFTW specific configuration */

/* C compiler name and flags */
#define FFTW_CC CCNAME
/* Define to compile in long-double precision. */
#undef  FFTW_LDOUBLE
/* Define to compile in single precision. */
#define FFTW_SINGLE 1
/* Use standard C type functions. */
#define USE_CTYPE
/* extra CFLAGS for codelets */
#define CODELET_OPTIM ""
/* Defines the calling convention used by the library. */
#define FFTEXP DLLENTRY

/* libsndfile specific configuration */

/* Set to long if unknown. */
#define TYPEOF_SF_COUNT_T long
/* Set to maximum allowed value of sf_count_t type. */
#define SF_COUNT_MAX 0x7FFFFFFFL
/* Set to sizeof (long) if unknown. */
#define SIZEOF_SF_COUNT_T 4

#endif /* PM123_CONFIG_H */
