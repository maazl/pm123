#ifndef PM123_DECODER_PLUG_H
#define PM123_DECODER_PLUG_H

#define INCL_BASE
#include <format.h>
#include <output_plug.h>
#include <os2.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

int  DLLENTRY decoder_init  ( void** w );
BOOL DLLENTRY decoder_uninit( void*  w );

typedef enum
{
  DECODER_PLAY     = 1, /* returns 101 -> already playing
                                   102 -> error, decoder killed and restarted */
  DECODER_STOP     = 2, /* returns 101 -> already stopped
                                   102 -> error, decoder killed (and stopped) */
  DECODER_FFWD     = 3,
  DECODER_REW      = 4, /* Level 2 plug-ins always send DECODER_FFWD */
  DECODER_JUMPTO   = 5,
  DECODER_SETUP    = 6,
  DECODER_EQ       = 7, /* obsolete, no longer used since PM123 1.40b */
  DECODER_BUFFER   = 8, /* obsolete, don't used anymore */
  DECODER_SAVEDATA = 9
} DECMSGTYPE;

/* return value:                                               */
/* PLUGIN_OK          -> ok.                                   */
/* PLUGIN_UNSUPPORTED -> command unsupported.                  */
/*                                                             */
/* DECODER_PLAY can return also:                               */
/* PLUGIN_GO_ALREADY  -> already playing.                      */
/* PLUGIN_GO_ERROR    -> error, decoder killed and restarted.  */
/*                                                             */
/* DECODER_STOP can return also:                               */
/* PLUGIN_GO_ALREADY  -> already stopped.                      */
/* PLUGIN_GO_ERROR    -> error, decoder killed and stopped.    */

typedef enum
{
  DECEVENT_PLAYSTOP   = 1, /* The decoder finished decoding */
  DECEVENT_PLAYERROR  = 2, /* A playback error occured so that PM123 should know to stop immediately */
  DECEVENT_SEEKSTOP   = 3, /* JUMPTO operation is completed */
  DEVEVENT_CHANGETECH = 4, /* change bitrate or samplingrate, param points to TECH_INFO structure */
  DECEVENT_CHANGEMETA = 5  /* change metadata, param points to META_INFO structure */
} DECEVENTTYPE;

typedef enum
{ DECFAST_NORMAL_PLAY = 0,
  DECFAST_FORWARD     = 1,
  DECFAST_REWIND      = 2
} DECFASTMODE;


typedef struct _DECODER_PARAMS
{
   int   size;

   /* --- DECODER_PLAY, STOP */

   const char* filename;
   const char* URL;
   const char* drive;       /* for CD ie.: "X:" */
   int         track;
   int         sectors[2];  /* play from sector x to sector y, currently unused by PM123 */
   const char* other;

   /* --- DECODER_REW, FFWD and JUMPTO */

   int   jumpto;      /* absolute positioning in milliseconds */
   int   ffwd;        /* 1 = start ffwd, 0 = end ffwd */
   int   rew;         /* 1 = start rew,  0 = end rew  */

   /* --- DECODER_SETUP */

   /* specify a function which the decoder should use for output */
   int  DLLENTRYP(output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
   void* a;           /* only to be used with the precedent function */
   int   audio_buffersize;

   const char* proxyurl;    /* NULL = none */
   const char* httpauth;    /* NULL = none */

   /* error message function the decoder should use */
   void DLLENTRYP(error_display)( const char* );

   /* info message function the decoder should use */
   /* this information is always displayed to the user right away */
   void DLLENTRYP(info_display)( const char* );

   HEV   playsem;     /* this semaphore is reseted when DECODER_PLAY is requested
                         and is posted on stop. No longer used by PM123, always NULLHANDLE. */

   HWND  hwnd;        /* commodity for PM interface, decoder must send a few
                         messages to this handle */

   /* values used for streaming inputs by the decoder */
   int   buffersize;  /* read ahead buffer in bytes, 0 = disabled */
   int   bufferwait;  /* block the first read until the buffer is filled */

   char* metadata_buffer; /* the decoder will put streaming metadata in this  */
   int   metadata_size;   /* buffer before posting WM_METADATA */
   int   bufferstatus;    /* obsolete, must be 0 */

   /* --- DECODER_EQ */

   /* usually only useful with MP3 decoder */
   int          equalizer;  /* TRUE or FALSE */
   const float* bandgain;   /* point to an array like this bandgain[#channels][10] */

   /* --- DECODER_SAVEDATA */

   const char*  save_filename;

} DECODER_PARAMS;

typedef struct _DECODER_PARAMS2
{
   int   size;

   /* --- DECODER_PLAY, STOP */

   const char* URL;

   /* --- DECODER_REW, FFWD and JUMPTO */

   T_TIME      jumpto;      /* absolute positioning in seconds */
   DECFASTMODE fast;        /* fast forward/rewind */

   /* --- DECODER_SETUP */

   /* specify a function which the decoder should use for output */
   int   DLLENTRYP(output_request_buffer )( void* a, const FORMAT_INFO2* format, short** buf );
   void  DLLENTRYP(output_commit_buffer  )( void* a, int len, T_TIME posmarker );
   /* decoder events */
   void  DLLENTRYP(output_event          )( void* a, DECEVENTTYPE event, void* param );
   void* a;           /* only to be used with the precedent functions */

   const char* proxyurl;    /* NULL = none */
   const char* httpauth;    /* NULL = none */

   /* error message function the decoder should use */
   void DLLENTRYP(error_display)( const char* );

   /* info message function the decoder should use */
   /* this information is always displayed to the user right away */
   void DLLENTRYP(info_display)( const char* );

   /* values used for streaming inputs by the decoder */
   unsigned int buffersize;  /* read ahead buffer in bytes, 0 = disabled */
   int   bufferwait;  /* block the first read until the buffer is filled */

   /* --- DECODER_EQ */

   /* usually only useful with MP3 decoder */
   int    equalizer;  /* TRUE or FALSE */
   const float* bandgain;   /* point to an array like this bandgain[#channels][10] */

   /* --- DECODER_SAVEDATA */

   const char* save_filename;

} DECODER_PARAMS2;

/* returns 0 -> ok
           1 -> command unsupported
           1xx -> msg specific */
#if !defined(DECODER_PLUGIN_LEVEL) || DECODER_PLUGIN_LEVEL <= 1
ULONG DLLENTRY decoder_command( void* w, ULONG msg, DECODER_PARAMS* params );
/* WARNING!! this _can_ change in time!!! returns stream length in ms */
/* the decoder should keep in memory a last valid length so the call  */
/* remains valid even if decoder_status() == DECODER_STOPPED          */
ULONG DLLENTRY decoder_length( void* w );
#else
ULONG DLLENTRY decoder_command( void* w, DECMSGTYPE msg, DECODER_PARAMS2* params );
void  DLLENTRY decoder_event  ( void* w, OUTEVENTTYPE event );
/* WARNING!! this _can_ change in time!!! returns stream length in ms */
/* the decoder should keep in memory a last valid length so the call  */
/* remains valid even if decoder_status() == DECODER_STOPPED          */
T_TIME DLLENTRY decoder_length( void* w );
#endif

#define DECODER_STOPPED  0
#define DECODER_PLAYING  1
#define DECODER_STARTING 2
#define DECODER_PAUSED   3
#define DECODER_ERROR    200

ULONG DLLENTRY decoder_status( void* w );

#define DECODER_MODE_STEREO         0
#define DECODER_MODE_JOINT_STEREO   1
#define DECODER_MODE_DUAL_CHANNEL   2
#define DECODER_MODE_SINGLE_CHANNEL 3

/* See haveinfo field of the DECODER_INFO structure. */
#define DECODER_HAVE_TITLE      0x0001
#define DECODER_HAVE_ARTIST     0x0002
#define DECODER_HAVE_ALBUM      0x0004
#define DECODER_HAVE_YEAR       0x0008
#define DECODER_HAVE_COMMENT    0x0010
#define DECODER_HAVE_GENRE      0x0020
#define DECODER_HAVE_TRACK      0x0040
#define DECODER_HAVE_COPYRIGHT  0x0080
#define DECODER_HAVE_TRACK_GAIN 0x0100
#define DECODER_HAVE_TRACK_PEAK 0x0200
#define DECODER_HAVE_ALBUM_GAIN 0x0400
#define DECODER_HAVE_ALBUM_PEAK 0x0800

#define INFO_SIZE_1  976  /* size of the DECODER_INFO structure prior PM123 1.32 */
#define INFO_SIZE_2 1264  /* size of the DECODER_INFO structure since PM123 1.32 */

/* NOTE: the information returned is only based on the FIRST header */
typedef struct _DECODER_INFO
{
   int   size;            /* see INFO_SIZE definitions */

   FORMAT_INFO format;    /* stream format after decoding */

   int   songlength;      /* in milliseconds, smaller than 0 -> unknown */
   int   junklength;      /* bytes of junk before stream start, if < 0 -> unknown */

   /* mpeg stuff */
   int   mpeg;            /* 25 = MPEG 2.5, 10 = MPEG 1.0, 20 = MPEG 2.0, 0 = not an MPEG */
   int   layer;           /* 0 = unknown */
   int   mode;            /* use it on modes(i) */
   int   modext;          /* didn't check what this one does */
   int   bpf;             /* bytes in the mpeg frame including header */
   int   bitrate;         /* in kbit/s */
   int   extention;       /* didn't check what this one does */

   /* track stuff */
   int   startsector;
   int   endsector;

   /* obsolete stuff */
   int   unused1;         /* obsolete, must be 0 */
   int   unused2;         /* obsolete, must be 0 */
   int   unused3;         /* obsolete, must be 0 */

   /* general technical information string */
   char  tech_info[128];

   /* meta information */
   char  title    [128];
   char  artist   [128];
   char  album    [128];
   char  year     [128];
   char  comment  [128];
   char  genre    [128];

   /* added since PM123 1.32 */
   char  track    [128];
   char  copyright[128];
   int   codepage;        /* Code page of the meta info. Must be 0 if the
                             code page is unknown. Don't place here any another
                             value if it is not provided by meta info. */
   int   haveinfo;        /* This flags define what of fields of the block of the
                             meta information are supported by the decoder. Can
                             be 0 if the decoder supports all fields. */
   int   saveinfo;        /* Must be 1 if the decoder can update meta info of
                             this file. Otherwise, must be 0. */
   int   filesize;        /* Size of the file. */

   float track_gain;      /* Defines Replay Gain values as specified at */
   float track_peak;      /* http://www.replaygain.org/ */
   float album_gain;
   float album_peak;

} DECODER_INFO;

/* Information types for DECODER_INFO2 */
typedef enum
{  INFO_FORMAT = 0x01,
   INFO_TECH   = 0x02,
   INFO_META   = 0x04,
   INFO_PHYS   = 0x08,
   INFO_RPL    = 0x10,
   INFO_ALL    = INFO_FORMAT|INFO_TECH|INFO_META|INFO_PHYS|INFO_RPL
} INFOTYPE;

typedef struct _DECODER_INFO2
{
   int  size;

   FORMAT_INFO2* format;  /* [INFO_FORMAT] Stream format after decoding */
   TECH_INFO*    tech;    /* [INFO_TECH] Technical informations about the source */
   META_INFO*    meta;    /* [INFO_META] Song information */
   BOOL          meta_write; /* [INFO_META] support editing the metadata (same as saveinfo in DECODER_INFO) */
   PHYS_INFO*    phys;    /* [INFO_PHYS] Basic Information about the source */
   RPL_INFO*     rpl;     /* [INFO_RPL] Playlist extensions */

} DECODER_INFO2;

/* returns
      PLUGIN_OK      = everything's perfect, structure is set,
      PLUGIN_NO_READ = error reading file (too small?),
      PLUGIN_NO_PLAY = decoder can't play that,
      other values   = errno, check xio_strerror() for string. */
#if defined(DECODER_PLUGIN_LEVEL) && DECODER_PLUGIN_LEVEL > 1
/* Request the information 'what' about 'url' from the decoder
 * 'what' is an input/output parameter. On input it is the requested information,
 * on output the returned information which must not be less than the requested bits.
 * If some type of information is not available, the bits in the return value of what
 * must be set either. The fields in the DECODER_INFO2 structure should be set
 * to their invalid values. This is the default.
 * decoder_fileinfo must not change fields that correspond to reset bits in what.
 * It must not change the pointers in DECODER_INFO2 either.
 */
ULONG DLLENTRY decoder_fileinfo ( const char* url, int* what, DECODER_INFO2* info );
/* Update meta information of file
   filename
      File to modify
   info
      New meta information
   haveinfo
      Fields of info that should be overwritten
   returns
      PLUGIN_OK      = everything's perfect, structure is saved to filename,
      other values   = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_saveinfo ( const char* filename, const META_INFO* info, int haveinfo );
#else
ULONG DLLENTRY decoder_fileinfo ( const char* filename, DECODER_INFO* info );
ULONG DLLENTRY decoder_trackinfo( const char* drive, int track, DECODER_INFO* info );
/* Update meta information of file
   filename
      File to modify
   info
      New meta information
   returns
      PLUGIN_OK      = everything's perfect, structure is saved to filename,
      other values   = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_saveinfo ( const char* filename, const DECODER_INFO* info );
#endif


typedef struct _DECODER_CDINFO
{
   int sectors;
   int firsttrack;
   int lasttrack;

} DECODER_CDINFO;

/* returns
      PLUGIN_OK      = everything's perfect, structure is set,
      PLUGIN_NO_READ = error reading required info,
      other values   = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_cdinfo( const char* drive, DECODER_CDINFO* info );

/* returns ORed values */
#define DECODER_FILENAME  0x0001 /* Decoder can play a regular file. */
#define DECODER_URL       0x0002 /* Decoder can play a internet stream or file. */
#define DECODER_TRACK     0x0004 /* Decoder can play a CD track. */
#define DECODER_OTHER     0x0008 /* Decoder can play something else. */
#define DECODER_METAINFO  0x8000 /* Decoder can save a meta info. */
#if defined(DECODER_PLUGIN_LEVEL) && DECODER_PLUGIN_LEVEL > 1
ULONG DLLENTRY decoder_support( const char** fileext, const char** filetype );
#else
/* size is i/o and is the size of the array.
   each ext should not be bigger than 8bytes */
ULONG DLLENTRY decoder_support( char* fileext[], int* size );
#endif

#if defined(DECODER_PLUGIN_LEVEL) && DECODER_PLUGIN_LEVEL > 0
ULONG DLLENTRY decoder_editmeta ( HWND owner, const char* url );
#endif

typedef void  DLLENTRYP(DECODER_WIZZARD_CALLBACK)( void* param, const char* url );

typedef ULONG DLLENTRYP(DECODER_WIZZARD_FUNC)( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param );

typedef struct _DECODER_WIZZARD
{ /* linked list */
  struct _DECODER_WIZZARD* link;
  /* String to be displayed in the context menu */
  const char*              prompt;
  /* Procedure to be called when the specified item is selected */
  DECODER_WIZZARD_FUNC     wizzard;
  /* Accreleration Table entries for normal invokation */
  USHORT                   accel_key;
  USHORT                   accel_opt;
  /* Accreleration Table entries for direct playlist manipulation in playlist manager */
  USHORT                   accel_key2;
  USHORT                   accel_opt2;
} DECODER_WIZZARD;

const DECODER_WIZZARD* DLLENTRY decoder_getwizzard( );

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_DECODER_PLUG_H */

