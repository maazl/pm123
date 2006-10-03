#ifndef __PM123_DECODER_PLUG_H
#define __PM123_DECODER_PLUG_H

#include <format.h>
#include <output_plug.h>

#ifdef __cplusplus
extern "C" {
#endif

int  PM123_ENTRY decoder_init  ( void** w );
BOOL PM123_ENTRY decoder_uninit( void*  w );

typedef enum
{
  DECODER_PLAY     = 1, /* returns 101 -> already playing
                                   102 -> error, decoder killed and restarted */
  DECODER_STOP     = 2, /* returns 101 -> already stopped
                                   102 -> error, decoder killed (and stopped) */
  DECODER_FFWD     = 3,
  DECODER_REW      = 4,
  DECODER_JUMPTO   = 5,
  DECODER_SETUP    = 6,
  DECODER_EQ       = 7,
  DECODER_BUFFER   = 8,
  DECODER_SAVEDATA = 9
} DECMSGTYPE;

typedef enum
{
  DECEVENT_PLAYSTOP,    /* The decoder finished decoding */
  DECEVENT_PLAY_ERROR,  /* The decoder cannot play more of this file */
  DECEVENT_SEEKSTOP,    /* JUMPTO operation is completed */
  DEVEVENT_CHANGETECH,  /* change bitrate or samplingrate, param points to TECH_INFO structure */
  DECEVENT_CHANGEMETA   /* change metadata, param points to META_INFO structure */ 
} DECEVENTTYPE;


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
   int   rew;         /* 1 = start rew, 0 = end rew */

   /* --- DECODER_SETUP */

   /* specify a function which the decoder should use for output */
   int  (PM123_ENTRYP output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
   void* a;           /* only to be used with the precedent function */
   int   audio_buffersize;

   const char* proxyurl;    /* NULL = none */
   const char* httpauth;    /* NULL = none */

   /* error message function the decoder should use */
   void (PM123_ENTRYP error_display)( char* );

   /* info message function the decoder should use */
   /* this information is always displayed to the user right away */
   void (PM123_ENTRYP info_display)( char* );

   HEV   playsem;     /* this semaphore is reseted when DECODER_PLAY is requested
                         and is posted on stop. No longer used by PM123 */

   HWND  hwnd;        /* commodity for PM interface, decoder must send a few
                         messages to this handle */

   /* values used for streaming inputs by the decoder */
   int   buffersize;  /* read ahead buffer in bytes, 0 = disabled */
   int   bufferwait;  /* block the first read until the buffer is filled */

   char* metadata_buffer; /* the decoder will put streaming metadata in this  */
   int   metadata_size;   /* buffer before posting WM_METADATA                */

   /* -- DECODER_BUFFER */

   int   bufferstatus;    /* reports how many bytes there are available in the buffer */

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
   int   posmarker;   /* position marker of file start */

   /* --- DECODER_REW, FFWD and JUMPTO */

   int   jumpto;      /* absolute positioning in milliseconds */
   int   ffwd;        /* 1 = start ffwd, 0 = end ffwd */
   int   rew;         /* 1 = start rew, 0 = end rew */

   /* --- DECODER_SETUP */

   /* specify a function which the decoder should use for output */
   int   (PM123_ENTRYP output_request_buffer )( void* a, const FORMAT_INFO2* format, short** buf );
   void  (PM123_ENTRYP output_commit_buffer  )( void* a, int len, int posmarker );
   void* a;           /* only to be used with the precedent functions */

   const char* proxyurl;    /* NULL = none */
   const char* httpauth;    /* NULL = none */

   /* error message function the decoder should use */
   void (PM123_ENTRYP error_display)( char* );

   /* info message function the decoder should use */
   /* this information is always displayed to the user right away */
   void (PM123_ENTRYP info_display)( char* );

   HWND  hwnd;        /* commodity for PM interface, decoder must send a few
                         messages to this handle */

   /* values used for streaming inputs by the decoder */
   int   buffersize;  /* read ahead buffer in bytes, 0 = disabled */
   int   bufferwait;  /* block the first read until the buffer is filled */

   char* metadata_buffer; /* the decoder will put streaming metadata in this  */
   int   metadata_size;   /* buffer before posting WM_METADATA                */

   /* -- DECODER_BUFFER */

   int   bufferstatus;    /* reports how many bytes there are available in the buffer */

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
ULONG PM123_ENTRY decoder_command( void* w, ULONG msg, DECODER_PARAMS* params );
#else
ULONG PM123_ENTRY decoder_command( void* w, DECMSGTYPE msg, DECODER_PARAMS2* params );
void  PM123_ENTRY decoder_event( void* w, DECEVENTTYPE event, void* param );
#endif

#define DECODER_STOPPED  0
#define DECODER_PLAYING  1
#define DECODER_STARTING 2
#define DECODER_ERROR    3

ULONG PM123_ENTRY decoder_status( void* w );

/* WARNING!! this _can_ change in time!!! returns stream length in ms */
/* the decoder should keep in memory a last valid length so the call  */
/* remains valid even if decoder_status() == DECODER_STOPPED          */
ULONG PM123_ENTRY decoder_length( void* w );

#define DECODER_MODE_STEREO         0
#define DECODER_MODE_JOINT_STEREO   1
#define DECODER_MODE_DUAL_CHANNEL   2
#define DECODER_MODE_SINGLE_CHANNEL 3

/* Technical information about the data source */
typedef struct
{  int  songlength;     /* in milliseconds, smaller than 0 -> unknown */
   int  bitrate;        /* in kbit/s, smaller than 0 -> unknown */
   int  filesize;       /* physical size of the file in kiB, smaller than 0 -> unknown */
   char info[128];      /* general technical information string */
} TECH_INFO;

/* Logical information about the data source */
typedef struct
{
   char title[128];
   char artist[128];
   char album[128];
   char year[128];
   char comment[128];
   char genre[128];
   int  track;          /* <0 = unknown */
} META_INFO;

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

   /* module stuff */
   int  numchannels;    /* 0 = not a MODule */
   int  numpatterns;
   int  numpositions;

   /* general technical information string */
   char tech_info[128];

   /* song information */
   char title[128];
   char artist[128];
   char album[128];
   char year[128];
   char comment[128];
   char genre[128];

} DECODER_INFO;

/* NOTE: the information returned is only based on the FIRST header */
typedef struct
{
   int  size;

   FORMAT_INFO format;  /* stream format after decoding */
   
   TECH_INFO   tech;    /* technical informations about the source */

   META_INFO   meta;    /* song information */

} DECODER_INFO2;

/* returns
      0 = everything's perfect, structure is set
      100 = error reading file (too small?)
      200 = decoder can't play that
      1001 = http error occured, check http_strerror() for string;
      other values = errno */
#if defined(DECODER_PLUGIN_LEVEL) && DECODER_PLUGIN_LEVEL > 1 
ULONG PM123_ENTRY decoder_fileinfo ( const char* filename, DECODER_INFO2* info );
#else
ULONG PM123_ENTRY decoder_fileinfo ( const char* filename, DECODER_INFO* info );
ULONG PM123_ENTRY decoder_trackinfo( const char* drive, int track, DECODER_INFO* info );
#endif

typedef struct _DECODER_CDINFO
{
   int sectors;
   int firsttrack;
   int lasttrack;

} DECODER_CDINFO;

ULONG PM123_ENTRY decoder_cdinfo( const char* drive, DECODER_CDINFO* info );

/* returns ORed values */
#define DECODER_FILENAME  0x1
#define DECODER_URL       0x2
#define DECODER_TRACK     0x4
#define DECODER_OTHER     0x8
/* size is i/o and is the size of the array.
   each ext should not be bigger than 8bytes */
ULONG PM123_ENTRY decoder_support( char* fileext[], int* size );

#ifdef __cplusplus
}
#endif
#endif /* __PM123_DECODER_PLUG_H */

