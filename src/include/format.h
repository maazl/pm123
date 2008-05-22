#ifndef  PM123_FORMAT_H
#define  PM123_FORMAT_H

#include <config.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

// Old style window messages
#define WM_PLAYSTOP         (WM_USER +  69)
#define WM_PLAYERROR        (WM_USER + 100)
#define WM_SEEKSTOP         (WM_USER + 666)
#define WM_METADATA         (WM_USER +  42)
#define WM_CHANGEBR         (WM_USER +  43)
#define WM_OUTPUT_OUTOFDATA (WM_USER + 667)
#define WM_PLUGIN_CONTROL   (WM_USER + 668) /* Plugin notify message */

#define PN_TEXTCHANGED  1   /* Display text changed */

#define WAVE_FORMAT_PCM 0x0001

typedef double T_TIME;
typedef double T_SIZE;

typedef struct _FORMAT_INFO
{
  unsigned int size;
  int samplerate;
  int channels;
  int bits;
  int format; /* WAVE_FORMAT_PCM = 1 */

} FORMAT_INFO;

/* reduced structure for level 2 plug-in interfaces */
typedef struct
{
  unsigned int size;    /* size of this structure */
  int samplerate;       /* sampling rate in Hz, < 0 -> unknown */
  int channels;         /* number of channels, < 0 -> unknown */
} FORMAT_INFO2;

/* Technical information about the data source */
typedef struct         /* for playlists only */
{ unsigned int size;   /* size of this structure */
  T_TIME songlength;   /* in seconds, smaller than 0 -> unknown */
  int    bitrate;      /* in kbit/s, smaller than 0 -> unknown */
  T_SIZE totalsize;    /* physical size of all included files, smaller than 0 -> unknown */
  int    total_items;  /* number of song items (for playlists), otherwise 1 */
  int    recursive;    /* Flag whether this object has some recursion in it's subobjects detected */
  char   info[128];    /* general technical information string */
} TECH_INFO;

/* Logical information about the data source */
typedef struct
{ unsigned int size;   /* size of this structure */
  char title[128];
  char artist[128];
  char album[128];
  char year[128];
  char comment[128];
  char genre[128];
  int  track;          /* <0 = unknown */
  float track_gain;    /* Defines Replay Gain values as specified at */
  float track_peak;    /* http://www.replaygain.org/ */
  float album_gain;
  float album_peak;
} META_INFO;

/* Physical info about the item */
typedef struct
{ unsigned int size;   /* size of this structure */
  T_SIZE filesize;     /* physical size of the file, smaller than 0 -> unknown */
  int    num_items;    /* number of immediate sub items (for playlists), otherwise 1 */
} PHYS_INFO;

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_FORMAT_H */
