#ifndef PM123_DECODER_PLUG_H
#define PM123_DECODER_PLUG_H

#include <format.h>
#include <output_plug.h>

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
  DECODER_EQ       = 7,
  DECODER_BUFFER   = 8, /* obsolete, don't used anymore */
  DECODER_SAVEDATA = 9
} DECMSGTYPE;

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

   double      jumpto;      /* absolute positioning in seconds */
   DECFASTMODE fast;        /* fast forward/rewind */

   /* --- DECODER_SETUP */

   /* specify a function which the decoder should use for output */
   int   DLLENTRYP(output_request_buffer )( void* a, const FORMAT_INFO2* format, short** buf );
   void  DLLENTRYP(output_commit_buffer  )( void* a, int len, double posmarker );
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
double DLLENTRY decoder_length( void* w );
#endif

#define DECODER_STOPPED  0
#define DECODER_PLAYING  1
#define DECODER_STARTING 2
#define DECODER_ERROR    3

ULONG DLLENTRY decoder_status( void* w );

#define DECODER_MODE_STEREO         0
#define DECODER_MODE_JOINT_STEREO   1
#define DECODER_MODE_DUAL_CHANNEL   2
#define DECODER_MODE_SINGLE_CHANNEL 3

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
   //char copyright[128];
   int  codepage;       /* Code page of the meta info. Must be 0 if the
                           code page is unknown. Don't place here any another
                           value if it is not provided by meta info. */
   int  filesize;       /* Size of the file. */
   int  saveinfo;       /* Must be 1 if the decoder can update meta info of
                           this file. Otherwise, must be 0. */

} DECODER_INFO;

/* NOTE: the information returned is only based on the FIRST header */
typedef struct
{
   int  size;

   FORMAT_INFO2* format;  /* stream format after decoding */
               
   TECH_INFO*    tech;    /* technical informations about the source */

   META_INFO*    meta;    /* song information */
   BOOL          meta_write; /* support editing the metadata (same as saveinfo in DECODER_INFO) */

} DECODER_INFO2;

/* returns
         0 = everything's perfect, structure is set,
       100 = error reading file (too small?),
       200 = decoder can't play that,
      other values = errno, check xio_strerror() for string. */
#if defined(DECODER_PLUGIN_LEVEL) && DECODER_PLUGIN_LEVEL > 1 
ULONG DLLENTRY decoder_fileinfo   ( const char* url, DECODER_INFO2* info );
#else
ULONG DLLENTRY decoder_fileinfo   ( const char* filename, DECODER_INFO* info );
ULONG DLLENTRY decoder_trackinfo  ( const char* drive, int track, DECODER_INFO* info );
#endif

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

ULONG DLLENTRY decoder_cdinfo( const char* drive, DECODER_CDINFO* info );

/* returns ORed values */
#define DECODER_FILENAME  0x0001 /* Decoder can play a regular file. */
#define DECODER_URL       0x0002 /* Decoder can play a internet stream or file. */
#define DECODER_TRACK     0x0004 /* Decoder can play a CD track. */
#define DECODER_OTHER     0x0008 /* Decoder can play a something else. */
#define DECODER_METAINFO  0x8000 /* Decoder can save a meta info. */
/* size is i/o and is the size of the array.
   each ext should not be bigger than 8bytes */
ULONG DLLENTRY decoder_support( char* fileext[], int* size );

#if defined(DECODER_PLUGIN_LEVEL) && DECODER_PLUGIN_LEVEL > 0 
ULONG DLLENTRY decoder_editmeta ( HWND owner, const char* url );

typedef void  DLLENTRYP(DECODER_WIZZARD_CALLBACK)( void* param, const char* url );

typedef ULONG DLLENTRYP(DECODER_WIZZARD_FUNC)( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param );

typedef struct _DECODER_WIZZARD
{ /* linked list */ 
  struct _DECODER_WIZZARD* link;
  /* String to be displayed in the context menu */
  const char*              prompt;
  /* Accreleration Table entry */
  USHORT                   accel_key;
  USHORT                   accel_options;
  /* Procedure to be called when the specified item is selected */
  DECODER_WIZZARD_FUNC     wizzard;
} DECODER_WIZZARD;

const DECODER_WIZZARD* DLLENTRY decoder_getwizzard( );
#endif

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_DECODER_PLUG_H */

