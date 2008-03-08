#ifndef PM123_DECODER_PLUG_H
#define PM123_DECODER_PLUG_H

#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

int  DLLENTRY decoder_init  ( void** w );
BOOL DLLENTRY decoder_uninit( void*  w );

#define DECODER_PLAY     1 /* returns 101 -> already playing
                                      102 -> error, decoder killed and restarted */
#define DECODER_STOP     2 /* returns 101 -> already stopped
                                      102 -> error, decoder killed (and stopped) */
#define DECODER_FFWD     3
#define DECODER_REW      4
#define DECODER_JUMPTO   5
#define DECODER_SETUP    6
#define DECODER_EQ       7
#define DECODER_BUFFER   8 /* obsolete, don't used anymore */
#define DECODER_SAVEDATA 9

typedef struct _DECODER_PARAMS
{
   int size;

   /* --- DECODER_PLAY, STOP */

   char* filename;
   void* unused1;     /* obsolete, must be NULL */
   char* drive;       /* for CD ie.: "X:" */
   int   track;
   int   sectors[2];  /* play from sector x to sector y */
   void* unused2;     /* obsolete, must be NULL */

   /* --- DECODER_REW, FFWD and JUMPTO */

   int   jumpto;      /* absolute positioning in milliseconds */
   int   ffwd;        /* 1 = start ffwd, 0 = end ffwd */
   int   rew;         /* 1 = start rew,  0 = end rew  */

   /* --- DECODER_SETUP */

   /* specify a function which the decoder should use for output */
   int  (DLLENTRYP output_play_samples)( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
   void* a;           /* only to be used with the precedent function */
   int   audio_buffersize;

   char* proxyurl;    /* NULL = none */
   char* httpauth;    /* NULL = none */

   /* error message function the decoder should use */
   void (DLLENTRYP error_display)( char* );

   /* info message function the decoder should use */
   /* this information is always displayed to the user right away */
   void (DLLENTRYP info_display)( char* );

   void* unused3;     /* obsolete, must be NULL */
   HWND  hwnd;        /* commodity for PM interface, decoder must send a few
                         messages to this handle */

   /* values used for streaming inputs by the decoder */
   int   buffersize;  /* read ahead buffer in bytes, 0 = disabled */
   int   bufferwait;  /* block the first read until the buffer is filled */

   char* metadata_buffer; /* the decoder will put streaming metadata in this  */
   int   metadata_size;   /* buffer before posting WM_METADATA */
   int   unused4;         /* obsolete, must be 0 */

   /* --- DECODER_EQ */

   /* usually only useful with MP3 decoder */
   int    equalizer;  /* TRUE or FALSE */
   float* bandgain;   /* point to an array like this bandgain[#channels][10] */

   /* --- DECODER_SAVEDATA */

   char*  save_filename;

} DECODER_PARAMS;

/* returns 0 -> ok
           1 -> command unsupported
           1xx -> msg specific */
ULONG DLLENTRY decoder_command( void* w, ULONG msg, DECODER_PARAMS* params );

#define DECODER_STOPPED  0
#define DECODER_PLAYING  1
#define DECODER_STARTING 2
#define DECODER_ERROR    3

ULONG DLLENTRY decoder_status( void* w );

/* WARNING!! this _can_ change in time!!! returns stream length in ms */
/* the decoder should keep in memory a last valid length so the call  */
/* remains valid even if decoder_status() == DECODER_STOPPED          */
ULONG DLLENTRY decoder_length( void* w );

#define DECODER_MODE_STEREO         0
#define DECODER_MODE_JOINT_STEREO   1
#define DECODER_MODE_DUAL_CHANNEL   2
#define DECODER_MODE_SINGLE_CHANNEL 3

#define modes(i) ( i == 0 ? "Stereo"         : \
                 ( i == 1 ? "Joint-Stereo"   : \
                 ( i == 2 ? "Dual-Channel"   : \
                 ( i == 3 ? "Single-Channel" : "" ))))

/* See haveinfo field of the DECODER_INFO structure. */
#define DECODER_HAVE_TITLE     0x0001
#define DECODER_HAVE_ARTIST    0x0002
#define DECODER_HAVE_ALBUM     0x0004
#define DECODER_HAVE_YEAR      0x0008
#define DECODER_HAVE_COMMENT   0x0010
#define DECODER_HAVE_GENRE     0x0020
#define DECODER_HAVE_TRACK     0x0040
#define DECODER_HAVE_COPYRIGHT 0x0080
#define DECODER_HAVE_ALL       0x00FF

/* NOTE: the information returned is only based on the FIRST header */
typedef struct _DECODER_INFO
{
   int  size;

   FORMAT_INFO format;  /* stream format after decoding */

   int  songlength;     /* in milliseconds, smaller than 0 -> unknown */
   int  junklength;     /* bytes of junk before stream start, if < 0 -> unknown */

   /* mpeg stuff */
   int  mpeg;           /* 25 = MPEG 2.5, 10 = MPEG 1.0, 20 = MPEG 2.0, 0 = not an MPEG */
   int  layer;          /* 0 = unknown */
   int  mode;           /* use it on modes(i) */
   int  modext;         /* didn't check what this one does */
   int  bpf;            /* bytes in the mpeg frame including header */
   int  bitrate;        /* in kbit/s */
   int  extention;      /* didn't check what this one does */

   /* track stuff */
   int  startsector;
   int  endsector;

   /* obsolete stuff */
   int  unused1;        /* obsolete, must be 0 */
   int  unused2;        /* obsolete, must be 0 */
   int  unused3;        /* obsolete, must be 0 */

   /* general technical information string */
   char tech_info[128];

   /* meta information */
   char title    [128];
   char artist   [128];
   char album    [128];
   char year     [128];
   char comment  [128];
   char genre    [128];
   char track    [128];
   char copyright[128];
   int  codepage;       /* Code page of the meta info. Must be 0 if the
                           code page is unknown. Don't place here any another
                           value if it is not provided by meta info. */
   int  haveinfo;       /* This flags define what of fields of the block of the
                           meta information are supported by the decoder. Can
                           be 0 if the decoder supports all fields. */
   int  saveinfo;       /* Must be 1 if the decoder can update meta info of
                           this file. Otherwise, must be 0. */
   int  filesize;       /* Size of the file. */

} DECODER_INFO;

/* returns
        0 = everything's perfect, structure is set,
      100 = error reading file (too small?),
      200 = decoder can't play that,
      other values = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_fileinfo( char* filename, DECODER_INFO* info );
ULONG DLLENTRY decoder_trackinfo( char* drive, int track, DECODER_INFO* info );

/* returns
        0 = everything's perfect, structure is saved,
      other values = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_saveinfo( char* filename, DECODER_INFO* info );

typedef struct _DECODER_CDINFO
{
   int sectors;
   int firsttrack;
   int lasttrack;

} DECODER_CDINFO;

ULONG DLLENTRY decoder_cdinfo( char* drive, DECODER_CDINFO* info );

/* returns ORed values */
#define DECODER_FILENAME  0x0001 /* Decoder can play a regular file. */
#define DECODER_URL       0x0002 /* Decoder can play a internet stream or file. */
#define DECODER_TRACK     0x0004 /* Decoder can play a CD track. */
#define DECODER_OTHER     0x0008 /* Decoder can play something. */
#define DECODER_METAINFO  0x8000 /* Decoder can save a meta info. */
/* size is i/o and is the size of the array.
   each ext should not be bigger than 8bytes */
ULONG DLLENTRY decoder_support( char* fileext[], int* size );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_DECODER_PLUG_H */

