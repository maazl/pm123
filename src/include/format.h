#ifndef  PM123_FORMAT_H
#define  PM123_FORMAT_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

#define WM_PLAYSTOP         (WM_USER +  69)
#define WM_PLAYERROR        (WM_USER + 100)
#define WM_SEEKSTOP         (WM_USER + 666)
#define WM_METADATA         (WM_USER +  42)
#define WM_CHANGEBR         (WM_USER +  43)
#define WM_OUTPUT_OUTOFDATA (WM_USER + 667)
#define WM_PLUGIN_CONTROL   (WM_USER + 668) /* Plugin notify message */

/*
 * WM_PLUGIN_CONTROL
 *   LONGFROMMP(mp1) = notify code
 *   LONGFROMMP(mp2) = additional information
 *
 * Notify codes:
*/

#define PN_TEXTCHANGED  1   /* Display text changed */

#define WAVE_FORMAT_PCM 0x0001

typedef struct _FORMAT_INFO
{
  int size;
  int samplerate;
  int channels;
  int bits;
  int format; /* WAVE_FORMAT_PCM = 1 */

} FORMAT_INFO;

/* reduced structure for level 2 plug-in interfaces */
typedef struct
{
  int size;
  int samplerate;
  int channels;
} FORMAT_INFO2;

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_FORMAT_H */
