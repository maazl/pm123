#ifndef PM123_OUTPUT_PLUG_H
#define PM123_OUTPUT_PLUG_H

#define INCL_BASE
#include <format.h>
#include <os2.h>

#ifdef __cplusplus
extern "C" {
#endif

#if PLUGIN_INTERFACE_LEVEL == 2 && !defined(PM123_DECODER_PLUG_H) && !defined(PM123_DECODER_PLUG_H)
#error "The output plug-in interface level 2 (PM123 1.40a) is no longer supported."
#endif

#pragma pack(4)

/** Opaque structure to store the decoders internal state.
 * Fill it with life or simply cast it to your own type.
 */
struct OUTPUT_STRUCT;

/****************************************************************************
 *
 * Definitions common to all interface levels.
 *
 ***************************************************************************/

ULONG DLLENTRY output_init  (struct OUTPUT_STRUCT** a);
ULONG DLLENTRY output_uninit(struct OUTPUT_STRUCT*  a);

typedef enum
{ OUTPUT_OPEN          = 1,
  OUTPUT_CLOSE         = 2,
  OUTPUT_VOLUME        = 3,
  OUTPUT_PAUSE         = 4,
  OUTPUT_SETUP         = 5,
  OUTPUT_TRASH_BUFFERS = 6,
  #if PLUGIN_INTERFACE_LEVEL < 2
  OUTPUT_NOBUFFERMODE  = 7 /* obsolete */
  #endif
} OUTMSGTYPE;

/****************************************************************************
 *
 * Definitions of level 1 interface
 *
 ***************************************************************************/
#if PLUGIN_INTERFACE_LEVEL < 2 || defined(PM123_CORE)

// forward declaration
struct _DECODER_INFO;

#define OUTPUT_SIZE_1 76  /* size of the OUTPUT_PARAMS structure prior PM123 1.32 */
#define OUTPUT_SIZE_2 80  /* size of the OUTPUT_PARAMS structure since PM123 1.32 */

typedef struct _OUTPUT_PARAMS
{
  /* --- see OUTPUT_SIZE definitions */
  unsigned int size;

  /* --- OUTPUT_SETUP */
  FORMAT_INFO  formatinfo;

  int          buffersize;

  unsigned short boostclass, normalclass;
  signed   short boostdelta, normaldelta;

  void DLLENTRYP(error_display)(const char*);

  /* info message function the output plug-in should use */
  /* this information is always displayed to the user right away */
  void DLLENTRYP(info_display)(const char*);

  HWND hwnd; /* commodity for PM interface, sends a few messages to this handle. */

  /* --- OUTPUT_VOLUME */
  unsigned char volume;
  float amplifier;

  /* --- OUTPUT_PAUSE */
  BOOL  pause;

  /* --- OUTPUT_NOBUFFERMODE */
  BOOL  nobuffermode;

  /* --- OUTPUT_TRASH_BUFFERS */
  ULONG temp_playingpos;  /* used until new buffers come in. */

  /* --- OUTPUT_OPEN */
  const char* filename;   /* filename, URL or track now being played,
                             useful for disk output */

  /* --- OUTPUT_SETUP ***OUTPUT*** */
  BOOL  always_hungry;    /* the output plug-in imposes no time restraint
                             it is always WM_OUTPUT_OUTOFDATA */

  /* --- OUTPUT_SETUP */
  const struct _DECODER_INFO* info;     /* added since PM123 1.32 */

} OUTPUT_PARAMS;

#if PLUGIN_INTERFACE_LEVEL < 2
ULONG  DLLENTRY output_command(struct OUTPUT_STRUCT* a, ULONG msg, OUTPUT_PARAMS* info);
int    DLLENTRY output_play_samples(struct OUTPUT_STRUCT* a, FORMAT_INFO* format, char* buf, int len, int posmarker);
ULONG  DLLENTRY output_playing_pos(struct OUTPUT_STRUCT* a);

ULONG  DLLENTRY output_playing_samples(struct OUTPUT_STRUCT* a, FORMAT_INFO* info, char* buf, int len);
BOOL   DLLENTRY output_playing_data(struct OUTPUT_STRUCT* a);
#endif

#endif /* level 1 interface */


/****************************************************************************
 *
 * Definitions of level 3 interface
 *
 ***************************************************************************/
#if PLUGIN_INTERFACE_LEVEL >= 3
typedef enum
{ OUTEVENT_END_OF_DATA,   // The flush signal is passed to the ouput plug-in and the last sample has been played
  OUTEVENT_LOW_WATER,     // The buffers of the output plug-in are getting low. Try to speed up the data source.
  OUTEVENT_HIGH_WATER,    // The buffers of the output plug-in are sufficiently filled.
  OUTEVENT_PLAY_ERROR     // The plug-in detected a fatal error stop immediately.
} OUTEVENTTYPE; 

/** Parameter block for \c output_command. */
typedef struct _OUTPUT_PARAMS2
{
  /** Event callback. See documentation. */
  void DLLENTRYP(OutEvent)(void* w, OUTEVENTTYPE event);
  /** Only to be used with the OutEvent function. */
  void* W;

  /** --- \c OUTPUT_VOLUME, [0...1] */
  float Volume;

  /** --- \c OUTPUT_PAUSE */
  BOOL  Pause;

  /** Related playing position for \c OUTPUT_TRASH_BUFFERS, \c OUTPUT_OPEN and \c OUTPUT_CLOSE */
  PM123_TIME  PlayingPos;

  /** Filename, URL or track now being played, useful for disk output.
   * Related commands: \c OUTPUT_SETUP and \c OUTPUT_OPEN */
  xstring URL;

  /** Information on the object to play. */
  const INFO_BUNDLE_CV* Info;

} OUTPUT_PARAMS2;

/** Callback of \c output_playing_samples. Called once per buffer.
 * Return true as long as you want more samples.
 * @remarks The function might be called from synchronized context. */
typedef void DLLENTRYP(OUTPUT_PLAYING_BUFFER_CB)(void* param, const FORMAT_INFO2* format,
  const float* samples, int count, PM123_TIME pos, BOOL* done);

ULONG  DLLENTRY output_command(struct OUTPUT_STRUCT* a, OUTMSGTYPE msg, const OUTPUT_PARAMS2* info);
int    DLLENTRY output_request_buffer(struct OUTPUT_STRUCT* a, const FORMAT_INFO2* format, float** buf);
void   DLLENTRY output_commit_buffer(struct OUTPUT_STRUCT* a, int len, PM123_TIME posmarker);
PM123_TIME DLLENTRY output_playing_pos(struct OUTPUT_STRUCT* a);

ULONG  DLLENTRY output_playing_samples(struct OUTPUT_STRUCT* a, PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param);
BOOL   DLLENTRY output_playing_data(struct OUTPUT_STRUCT* a);

#endif /* level 3 interface */


#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_OUTPUT_PLUG_H */

