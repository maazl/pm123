/****************************************************************************
 *
 * Definitions common to all interface levels.
 *
 ***************************************************************************/

#ifndef PM123_DECODER_PLUG_H
#define PM123_DECODER_PLUG_H

#define INCL_WIN
#define INCL_BASE
#include <format.h>
#include <output_plug.h>
#include <os2.h>

#ifdef __cplusplus
extern "C" {
#endif

#if PLUGIN_INTERFACE_LEVEL == 2
#error "The decoder plug-in interface level 2 (PM123 1.40a) is no longer supported."
#endif

#pragma pack(4)


/** Opaque structure to store the decoders internal state.
 * Fill it with life or simply cast it to your own type.
 */
struct DECODER_STRUCT;

/** Initialize a decoder instance.
 * @param w Return a handle in \a *w.
 * @return Return PLUGIN_OK on success. */
int  DLLENTRY decoder_init  (struct DECODER_STRUCT** w);
/** Destroy initialized decoder instance.
 * @param w Handle received by \c decoder_init.
 * @return TRUE: success */
BOOL DLLENTRY decoder_uninit(struct DECODER_STRUCT*  w);

/** Decoder flags, values can be ored. */
typedef enum
{ DECODER_FILENAME = 0x0001 /**< Decoder can play a regular file. (file:) */
, DECODER_URL      = 0x0002 /**< Decoder can play a Internet stream or file. (http:, https:, ftp:) */
, DECODER_TRACK    = 0x0004 /**< Decoder can play a CD track. (cd: cdda:) */
, DECODER_OTHER    = 0x0008 /**< Decoder can play something else. */
, DECODER_SONG     = 0x0100 /**< Decoder can play songs with this file type. */
, DECODER_PLAYLIST = 0x0200 /**< Decoder can play playlists with this file type. */
, DECODER_WRITABLE = 0x1000 /**< Decoder can save items of this type. */
, DECODER_METAINFO = 0x2000 /**< Decoder can save a meta info. */
} DECODER_TYPE;

/** Decoder commands */
typedef enum
{ DECODER_PLAY     = 1      /**< Start decoding, returns 101 -> already playing, 102 -> error, decoder killed and restarted */
, DECODER_STOP     = 2      /**< Stop decoding, returns 101 -> already stopped, 102 -> error, decoder killed (and stopped) */
, DECODER_FFWD     = 3      /**< Fast forward and rewind */
, DECODER_REW      = 4      /**< Rewind for Level 1 plug-ins @deprecated Level 2 and above plug-ins always send \c DECODER_FFWD */
, DECODER_JUMPTO   = 5      /**< Jump to a certain location within a song. */
, DECODER_SETUP    = 6      /**< Initialize decoder instance to decode a new song. */
, DECODER_EQ       = 7      /**< Set equalizer parameters. @deprecated Obsolete, no longer supported since PM123 1.40b */
, DECODER_BUFFER   = 8      /**< @deprecated Obsolete, not used anymore */
, DECODER_SAVEDATA = 9      /**< Save the raw stream into a file. */
} DECMSGTYPE;

/** Decoder events */
typedef enum
{ DECEVENT_PLAYSTOP   = 1   /**< The decoder finished decoding, either because the end of the file is reached or because of a \c DECODER_STOP command. */
, DECEVENT_PLAYERROR  = 2   /**< A decoder error occurred so that PM123 should know to stop immediately */
, DECEVENT_SEEKSTOP   = 3   /**< \c DECODER_JUMPTO operation has completed */
, DECEVENT_CHANGETECH = 4   /**< On the fly change of sampling rate, param points to TECH_INFO structure */
, DECEVENT_CHANGEOBJ  = 5   /**< On the fly change of song length, param points to OBJ_INFO structure */
, DECEVENT_CHANGEMETA = 6   /**< On the fly change of meta data, param points to META_INFO structure */
} DECEVENTTYPE;

/** Decoder states */
typedef enum
{ DECODER_STOPPED  = 0      /**< Decoder instance is idle. */
, DECODER_PLAYING  = 1      /**< Decoder instance is decoding. */
, DECODER_STARTING = 2      /**< Decoder instance has received a \c DECODER_PLAY command, but has not yet started decoding. */
, DECODER_PAUSED   = 3      /**< Decoder instance has been suspended */
, DECODER_STOPPING = 4      /**< Decoder instance has received a \c DECODER_STOP command, but the decoder thread has not terminated so far. */
, DECODER_ERROR    = 9      /**< The decoder instance is in an invalid state and should be destroyed with \c decoder_uninit. */
} DECODERSTATE;

/** Query the current state of a decoder instance */
ULONG DLLENTRY decoder_status(struct DECODER_STRUCT* w);

/** Flags to address individual fields of the meta information.
 * See \c haveinfo field of the \c DECODER_INFO structure. */
typedef enum
{ DECODER_HAVE_NONE       = 0x0000
, DECODER_HAVE_TITLE      = 0x0001
, DECODER_HAVE_ARTIST     = 0x0002
, DECODER_HAVE_ALBUM      = 0x0004
, DECODER_HAVE_YEAR       = 0x0008
, DECODER_HAVE_COMMENT    = 0x0010
, DECODER_HAVE_GENRE      = 0x0020
, DECODER_HAVE_TRACK      = 0x0040
, DECODER_HAVE_COPYRIGHT  = 0x0080
, DECODER_HAVE_TRACK_GAIN = 0x0100
, DECODER_HAVE_TRACK_PEAK = 0x0200
, DECODER_HAVE_ALBUM_GAIN = 0x0400
, DECODER_HAVE_ALBUM_PEAK = 0x0800
} DECODERMETA;

#if PLUGIN_INTERFACE_LEVEL > 0
/** Invoke decoder specific dialog to edit the meta information of an object.
 * @param owner Owner window handle
 * @param url Object to edit
 * @return Success code. See PDK documentation */
ULONG DLLENTRY decoder_editmeta(HWND owner, const char* url);
#endif

/** Callback of \c decoder_fileinfo. Called once per item. */
typedef void DLLENTRYP(DECODER_INFO_ENUMERATION_CB)(void* param, const char* url,
  const INFO_BUNDLE* info, int cached, int reliable);

typedef ULONG DLLENTRYP(DECODER_WIZARD_FUNC)(HWND owner, const char* title,
                                             DECODER_INFO_ENUMERATION_CB callback, void* param);

typedef struct _DECODER_WIZARD
{ struct _DECODER_WIZARD* link;         /**< linked list */
  const char*             prompt;       /**< String to be displayed in the context menu */
  DECODER_WIZARD_FUNC     wizard;       /**< Procedure to be called when the specified item is selected */
  USHORT                  accel_key;    /**< Acceleration Table entries for normal invocation */
  USHORT                  accel_opt;
  USHORT                  accel_key2;   /**< Acceleration Table entries for direct playlist manipulation in playlist manager */
  USHORT                  accel_opt2;
} DECODER_WIZARD;

const DECODER_WIZARD* DLLENTRY decoder_getwizard(void);



/****************************************************************************
 *
 * Definitions of level 1 interface
 *
 ***************************************************************************/
#if PLUGIN_INTERFACE_LEVEL < 2 || defined(PM123_CORE)

typedef struct _DECODER_PARAMS
{
  unsigned int size;

  /* --- DECODER_PLAY, STOP */
  const char*  filename;
  const char*  URL;
  const char*  drive;     /**< for CD ie.: "X:" */
  int          track;
  int          sectors[2];/**< play from sector x to sector y, currently unused by PM123 */
  const char*  other;

  /* --- DECODER_REW, FFWD and JUMPTO */
  int   jumpto;           /**< absolute positioning in milliseconds */
  int   ffwd;             /**< 1 = start ffwd, 0 = end ffwd */
  int   rew;              /**< 1 = start rew,  0 = end rew  */

  /* --- DECODER_SETUP */
  /* specify a function which the decoder should use for output */
  int  DLLENTRYP(output_play_samples)(void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker);
  void* a;                /**< only to be used with the precedent function */
  int   audio_buffersize;

  const char* proxyurl;   /**< NULL = none */
  const char* httpauth;   /**< NULL = none */

  /** error message function the decoder should use */
  void DLLENTRYP(error_display)(const char*);

  /** info message function the decoder should use */
  /** this information is always displayed to the user right away */
  void DLLENTRYP(info_display)(const char*);

  HEV   playsem;          /**< this semaphore is reseted when DECODER_PLAY is requested
                               and is posted on stop. No longer used by PM123, always NULLHANDLE. */

  HWND  hwnd;             /**< commodity for PM interface, decoder must send a few
                               messages to this handle */

  /* values used for streaming inputs by the decoder */
  int   buffersize;       /**< read ahead buffer in bytes, 0 = disabled */
  int   bufferwait;       /**< block the first read until the buffer is filled */

  char* metadata_buffer;  /**< the decoder will put streaming metadata in this  */
  int   metadata_size;    /**< buffer before posting WM_METADATA */
  int   bufferstatus;     /**< obsolete, must be 0 */

  /* --- DECODER_EQ */
  /* usually only useful with MP3 decoder */
  int          equalizer; /**< TRUE or FALSE */
  const float* bandgain;  /**< point to an array like this bandgain[#channels][10] */

  /* --- DECODER_SAVEDATA */
  const char*  save_filename;

} DECODER_PARAMS;

#define INFO_SIZE_1  976  /**< size of the DECODER_INFO structure prior PM123 1.32 */
#define INFO_SIZE_2 1264  /**< size of the DECODER_INFO structure since PM123 1.32 */

/* NOTE: the information returned is only based on the FIRST header */
typedef struct _DECODER_INFO
{
   int   size;            /**< see INFO_SIZE definitions */

   FORMAT_INFO format;    /**< stream format after decoding */

   int   songlength;      /**< in milliseconds, smaller than 0 -> unknown */
   int   junklength;      /**< bytes of junk before stream start, if < 0 -> unknown */

   /* mpeg stuff */
   int   mpeg;            /**< unused since PM123 1.40, 25 = MPEG 2.5, 10 = MPEG 1.0, 20 = MPEG 2.0, 0 = not an MPEG */
   int   layer;           /**< unused since PM123 1.40, 0 = unknown */
   int   mode;            /**< unused since PM123 1.40, use it on modes(i) */
   int   modext;          /**< unused since PM123 1.40, didn't check what this one does */
   int   bpf;             /**< unused since PM123 1.40, bytes in the mpeg frame including header */
   int   bitrate;         /**< in kbit/s */
   int   extention;       /**< unused since PM123 1.40, didn't check what this one does */

   /* track stuff */
   int   startsector;
   int   endsector;

   /* obsolete stuff */
   int   unused1;         /**< @deprecated must be 0 */
   int   unused2;         /**< @deprecated must be 0 */
   int   unused3;         /**< @deprecated must be 0 */

   /** general technical information string */
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
   int   codepage;        /**< Code page of the meta info. Must be 0 if the
                               code page is unknown. Don't place here any another
                               value if it is not provided by meta info. */
   int   haveinfo;        /**< This flags define what of fields of the block of the
                               meta information are supported by the decoder. Can
                               be 0 if the decoder supports all fields. */
   int   saveinfo;        /**< Must be 1 if the decoder can update meta info of
                               this file. Otherwise, must be 0. */
   int   filesize;        /**< Size of the file. */

   float track_gain;      /* Defines Replay Gain values as specified at */
   float track_peak;      /* http://www.replaygain.org/ */
   float album_gain;
   float album_peak;

} DECODER_INFO;

/* CD info structure for old style plug-ins.
 */
typedef struct _DECODER_CDINFO
{ int sectors;
  int firsttrack;
  int lasttrack;
} DECODER_CDINFO;

#if PLUGIN_INTERFACE_LEVEL <= 1
/* size is i/o and is the size of the array.
   each ext should not be bigger than 8bytes */
ULONG DLLENTRY decoder_support(char* fileext[], int* size);

/* returns 0 -> ok
           1 -> command unsupported
           1xx -> msg specific */
ULONG DLLENTRY decoder_command(struct DECODER_STRUCT* w, ULONG msg, DECODER_PARAMS* params);

/* WARNING!! this _can_ change in time!!! returns stream length in ms */
/* the decoder should keep in memory a last valid length so the call  */
/* remains valid even if decoder_status() == DECODER_STOPPED          */
ULONG DLLENTRY decoder_length(struct DECODER_STRUCT* w);

/* These modes are from the MPEG Audio specification and only used for old plug-ins. */
#define DECODER_MODE_STEREO         0
#define DECODER_MODE_JOINT_STEREO   1
#define DECODER_MODE_DUAL_CHANNEL   2
#define DECODER_MODE_SINGLE_CHANNEL 3

/* returns
      PLUGIN_OK      = everything's perfect, structure is set,
      PLUGIN_NO_READ = error reading file (too small?),
      PLUGIN_NO_PLAY = decoder can't play that,
      other values   = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_fileinfo (const char* filename, DECODER_INFO* info);
ULONG DLLENTRY decoder_trackinfo(const char* drive, int track, DECODER_INFO* info);

/* returns
      PLUGIN_OK      = everything's perfect, structure is set,
      PLUGIN_NO_READ = error reading required info,
      other values   = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_cdinfo   (const char* drive, DECODER_CDINFO* info);

/* Update meta information of file
   filename
      File to modify
   info
      New meta information
   returns
      PLUGIN_OK      = everything's perfect, structure is saved to filename,
      other values   = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_saveinfo (const char* filename, const DECODER_INFO* info);

#endif

#endif /* Level 1 interface */


/****************************************************************************
 *
 * Definitions of level 3 interface
 *
 ***************************************************************************/
#if PLUGIN_INTERFACE_LEVEL >= 3

typedef struct
{ const char* category;     /**< File type category, e.g. "Playlist file" */
  const char* eatype;       /**< File type, e.g. "PM123 playlist file" */
  const char* extension;    /**< File extension, e.g. "*.lst;*.m3u" */
  unsigned    flags;        /**< Bit vector of DECODER_TYPE */
} DECODER_FILETYPE;

ULONG DLLENTRY decoder_support(const DECODER_FILETYPE** types, int* count);

typedef enum
{ DECFAST_NORMAL_PLAY = 0
, DECFAST_FORWARD     = 1
, DECFAST_REWIND      = 2
} DECFASTMODE;

typedef struct _DECODER_PARAMS2
{
  /* --- DECODER_PLAY */
  xstring      URL;         /**< URL to play */
  const INFO_BUNDLE_CV* Info;/**< Info about the URL to play */

  /* --- DECODER_PLAY DECODER_REW, FFWD and JUMPTO */
  PM123_TIME   JumpTo;      /**< absolute positioning in seconds */
  DECFASTMODE  Fast;        /**< fast forward/rewind */

  /* --- DECODER_SETUP */
  /** specify a function which the decoder should use for output */
  int   DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void  DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  /** decoder events */
  void  DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, void* param);
  void* A;                  /**< only to be used with the precedent functions */

  /* --- DECODER_SAVEDATA */
  xstring      SaveFilename;/**< File where to save the raw stream. */

} DECODER_PARAMS2;

ULONG DLLENTRY decoder_command(struct DECODER_STRUCT* w, DECMSGTYPE msg, const DECODER_PARAMS2* params);
void  DLLENTRY decoder_event  (struct DECODER_STRUCT* w, OUTEVENTTYPE event);
/* WARNING!! this _can_ change in time!!! returns stream length in s  */
/* the decoder should keep in memory a last valid length so the call  */
/* remains valid even if decoder_status() == DECODER_STOPPED          */
PM123_TIME DLLENTRY decoder_length(struct DECODER_STRUCT* w);


/** Callback of \c decoder_savelist. Called once per item. */
typedef int DLLENTRYP(DECODER_SAVE_ENUMERATION_CB)(void* param, xstring* url,
  const INFO_BUNDLE** info, int* cached, int* reliable);


/** Request the information 'what' about 'url' from the decoder
 * 'what' is an input/output parameter. On input it is the requested information,
 * on output the returned information which must not be less than the requested bits.
 * If some type of information is not available, the bits in the return value of what
 * must be set either. The fields in the INFO_BUNDLE structure should be set
 * to their invalid values. This is the default.
 * decoder_fileinfo must not change fields that correspond to reset bits in what.
 * It must not change the pointers in INFO_BUNDLE either.
 * returns
      PLUGIN_OK      = everything's perfect, structure is set,
      PLUGIN_NO_READ = error reading file (too small?),
      PLUGIN_NO_PLAY = decoder can't play that,
      other values   = errno, check xio_strerror() for string. */
ULONG DLLENTRY decoder_fileinfo (const char* url, struct _XFILE* handle, int* what, const INFO_BUNDLE* info,
                                 DECODER_INFO_ENUMERATION_CB cb, void* param);

ULONG DLLENTRY decoder_saveinfo (const char* url, const META_INFO* info, int haveinfo, xstring* errortext);

ULONG DLLENTRY decoder_savefile (const char* url, const char* format, int* what, const INFO_BUNDLE* info,
                                 DECODER_SAVE_ENUMERATION_CB cb, void* param, xstring* errortext);

#endif /* Level 3 interface */

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_DECODER_PLUG_H */

