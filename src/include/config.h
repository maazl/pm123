#ifndef PM123_CONFIG_H
#define PM123_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__IBMC__) || defined(__IBMCPP__)
  int  _CRT_init( void );
  void _CRT_term( void );

  #define TFNENTRY _Optlink
#else
  #define TFNENTRY
#endif

#ifndef INLINE
  #if defined(__GNUC__)
    #define INLINE __inline__
  #elif defined(__WATCOMC__)
    #define INLINE __inline
  #elif defined(__IBMC__)
    #define INLINE _Inline
  #else
    #define INLINE static
  #endif
#endif

#ifndef PM123_ENTRY
  #if defined(__IBMC__) || defined(__IBMCPP__)
    #define PM123_ENTRY  _System
    #define PM123_ENTRYP * PM123_ENTRY
  #else
    #define PM123_ENTRY  _System
    #define PM123_ENTRYP PM123_ENTRY *
  #endif
#endif

#ifdef __EMX__
  #define INIT_ATTRIBUTE __attribute__((constructor))
  #define TERM_ATTRIBUTE __attribute__((destructor))
#else
  #define INIT_ATTRIBUTE
  #define TERM_ATTRIBUTE
#endif

#ifdef __cplusplus
}
#endif
#endif /* PM123_CONFIG_H */
