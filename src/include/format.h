#ifndef __PM123_FORMAT_H
#define __PM123_FORMAT_H

#ifndef PM123_ENTRY
  #if defined(__IBMC__) || defined(__IBMCPP__)
    #define  PM123_ENTRY  _System
    #define  PM123_ENTRYP * PM123_ENTRY
  #else
    #define  PM123_ENTRY  _System
    #define  PM123_ENTRYP PM123_ENTRY *
  #endif
#endif

#define WM_PLAYSTOP         (WM_USER +  69)
#define WM_PLAYERROR        (WM_USER + 100)
#define WM_SEEKSTOP         (WM_USER + 666)
#define WM_METADATA         (WM_USER +  42)
#define WM_CHANGEBR         (WM_USER +  43)
#define WM_OUTPUT_OUTOFDATA (WM_USER + 667)
#define WM_PLUGIN_CONTROL   (WM_USER + 668) /* Plugin notify message */

/*
   WM_PLUGIN_CONTROL
     LONGFROMMP(mp1) = notify code
     LONGFROMMP(mp2) = additional information
*/

/* Notify codes */
#define PN_TEXTCHANGED  1   /* Display text changed */

/* AFAIK, all of those also have BitsPerSample as format specific */
#define WAVE_FORMAT_PCM       0x0001
#define WAVE_FORMAT_ADPCM     0x0002
#define WAVE_FORMAT_ALAW      0x0006
#define WAVE_FORMAT_MULAW     0x0007
#define WAVE_FORMAT_OKI_ADPCM 0x0010
#define WAVE_FORMAT_DIGISTD   0x0015
#define WAVE_FORMAT_DIGIFIX   0x0016
#define IBM_FORMAT_MULAW      0x0101
#define IBM_FORMAT_ALAW       0x0102
#define IBM_FORMAT_ADPCM      0x0103

typedef struct _FORMAT_INFO
{
  int size;
  int samplerate;
  int channels;
  int bits;
  int format; // PCM = 1

} FORMAT_INFO;

#endif /* __PM123_FORMAT_H */

