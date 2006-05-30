#ifndef __PM123_OUTPUT_PLUG_H
#define __PM123_OUTPUT_PLUG_H

#define WM_PLAYERROR          WM_USER+100
#define WM_OUTPUT_OUTOFDATA   WM_USER+667

ULONG _System output_init(void **a);
ULONG _System output_uninit(void *a);

#define OUTPUT_OPEN          1 // may not be necessary!
#define OUTPUT_CLOSE         2
#define OUTPUT_VOLUME        3
#define OUTPUT_PAUSE         4
#define OUTPUT_SETUP         5
#define OUTPUT_TRASH_BUFFERS 6
#define OUTPUT_NOBUFFERMODE  7

typedef struct
{
   int size;

  /* --- OUTPUT_SETUP */

   FORMAT_INFO formatinfo;

   int buffersize;

   unsigned short boostclass, normalclass;
   signed   short boostdelta, normaldelta;

   void (* _System error_display)(char *);

   /* info message function the output plug-in should use */
   /* this information is always displayed to the user right away */
   void (* _System info_display)(char *);

   HWND hwnd; // commodity for PM interface, sends a few messages to this handle

  /* --- OUTPUT_VOLUME */

   char volume;
   float amplifier;

  /* --- OUTPUT_PAUSE */

   BOOL pause;

  /* --- OUTPUT_NOBUFFERMODE */

   BOOL nobuffermode;

  /* --- OUTPUT_TRASH_BUFFERS */

   ULONG temp_playingpos; // used until new buffers come in

  /* --- OUTPUT_OPEN */

   char *filename; // filename, URL or track now being played,
                   // useful for disk output

  /* --- OUTPUT_SETUP ***OUTPUT*** */
   BOOL always_hungry; // the output plug-in imposes no time restraint
                       // it is always WM_OUTPUT_OUTOFDATA

} OUTPUT_PARAMS;
ULONG _System output_command(void *a, ULONG msg, OUTPUT_PARAMS *info);

ULONG _System output_playing_samples(void *a, FORMAT_INFO *info, char *buf, int len);
int _System output_play_samples(void *a, FORMAT_INFO *format, char *buf,int len, int posmarker);
ULONG _System output_playing_pos(void *a);
BOOL _System output_playing_data(void *a);

#endif /* __PM123_OUTPUT_PLUG_H */