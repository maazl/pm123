#ifndef PM123_CONFIG_H
#define PM123_CONFIG_H

/* Sooner or later we need this anyway */
#define INCL_DOS

#if defined(__GNUC__)
  #include <config_gcc.h>
#else
  #error Unsupported compiler.
#endif

#define HAVE_DEBUGLOG 1

#define SIZEOF_VOIDP 4

/* Name of package */
#define PACKAGE_NAME "PM123"
#define PACKAGE PACKAGE_NAME

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "pm123@maazl.de"

/* Version number of package */
#define PACKAGE_VERSION "1.41"
#define VERSION PACKAGE_VERSION

/* Define to the full name and version of this package. */
#define PACKAGE_STRING PACKAGE" "VERSION

/* booleans for C */
#if !defined(__cplusplus) && !defined(ASSEMBLER)
  typedef unsigned char bool;
  #define true  1
  #define false 0
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

/* XIO specific configuration */
/* Disabled (MM). We are here not in the kindergarden. */
/* #define XIO_SERIALIZE_DISK_IO 1 */

/* FFTW specific configuration */

/* C compiler name and flags */
#define FFTW_CC CCNAME
/* Define to compile in single precision. */
#define FFTW_SINGLE 1
/* Use standard C type functions. */
#define USE_CTYPE
/* extra CFLAGS for codelets */
#define CODELET_OPTIM ""
/* Defines the calling convention used by the library. */
#define FFTEXP DLLENTRY


#ifdef __cplusplus
/* Define the macro NOSYSTEMSTATICMEMBER to work around for the IBMVAC++ restriction
 * that static class functions may not have a defined calling convention.
 * However, this workaround relies on friend functions with static linkage. This is
 * invalid according to the C++ standard, but IBMVAC++ does not care about that fact.
 */
#ifdef NOSYSTEMSTATICMEMBER
#define PROXYFUNCDEF friend
#define PROXYFUNCIMP(ret, cls) ret
#define PROXYFUNCREF(cls)
#else
#define PROXYFUNCDEF static
#define PROXYFUNCIMP(ret, cls) ret cls::
#define PROXYFUNCREF(cls) cls::
#endif
#endif


/* Keep pulseaudio happy */
#define GETTEXT_PACKAGE


#endif /* PM123_CONFIG_H */
