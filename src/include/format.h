#ifndef  PM123_FORMAT_H
#define  PM123_FORMAT_H

#include <plugin.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WAVE_FORMAT_PCM 0x0001

/** Time index in seconds (may be fractional) */
typedef double PM123_TIME;
/** Object size in bytes (integral) - not all OS/2 compilers support 64 bit ints. */
typedef double PM123_SIZE;

/** Format for Level 1 plug-ins only */
typedef struct
{ unsigned   size;
  int        samplerate;
  int        channels;
  int        bits;
  int        format;     /* WAVE_FORMAT_PCM = 1 */
} FORMAT_INFO;

/** Physical file attributes */
typedef enum
{ PATTR_NONE     = 0x00,   /* No special attributes */
  PATTR_WRITABLE = 0x10,   /* Flag whether the file/URL is writable in place */
  PATTR_INVALID  = 0x80    /* The item is physically invalid. (Should not be set by a decoder.) */
} PHYS_ATTRIBUTES;
/** Physical info about the item */
typedef struct
{ PM123_SIZE filesize;   /* Physical size of the file, smaller than 0 -> unknown */
  int        tstmp;      /* Last modification time stamp, (time_t)-1 -> unknown */
  unsigned   attributes; /* File attributes, bit vector of PHYS_ATTRIBUTES */
} PHYS_INFO;
#define PHYS_INFO_INIT { -1, -1, PATTR_NONE }

/** Technical flags for level 2 plug-ins */
typedef enum
{ TATTR_NONE     = 0x00,
  TATTR_SONG     = 0x01, /* The item is playable by this decoder. */
  TATTR_PLAYLIST = 0x02, /* The item can have sub entries. */
  TATTR_WRITABLE = 0x10, /* This file is writable (decoder_saveinfo). */
  TATTR_STORABLE = 0x20, /* This stream is saveable (DECODER_SAVEDATA). */
  TATTR_INVALID  = 0x80  /* The item is logically invalid. (Should not be set by a decoder.) */
} TECH_ATTRIBUTES;
/** Technical format structure for level 2 plug-in interfaces */
typedef struct
{ int        samplerate; /* sampling rate in Hz, < 0 -> unknown */
  int        channels;   /* number of channels, < 0 -> unknown */
  unsigned   attributes; /* Bit vector of TECH_ATTRIBUTES */
  DSTRING    info;       /* general technical information string */
  DSTRING    format;     /* File format (if any) */
  DSTRING    decoder;    /* Decoder name, filled by PM123 core */
} TECH_INFO;
#define TECH_INFO_INIT { -1, -1, TATTR_NONE, {NULL}, {NULL}, {NULL} }

/** Detailed song info for level 2 plug-in interfaces */
typedef struct
{ PM123_TIME songlength; /* in seconds, < 0 -> unknown */
  int        bitrate;    /* in bit/s, < 0 -> unknown */
  int        num_items;  /* number of immediate subitems, < 0 -> unknown */
} OBJ_INFO;
#define OBJ_INFO_INIT { -1, -1, -1 }

/** Logical information about the data source */
typedef struct
{ DSTRING    title;      /* Use the dstring_* functions to manipulate these objects. */
  DSTRING    artist;
  DSTRING    album;
  DSTRING    year;
  DSTRING    comment;
  DSTRING    genre;
  DSTRING    track;
  DSTRING    copyright;
  float      track_gain; /* Defines Replay Gain values as specified at */
  float      track_peak; /* http://www.replaygain.org/ */
  float      album_gain;
  float      album_peak;
} META_INFO;
#define META_INFO_INIT { {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, -1000, -1000, -1000, -1000 }

/** Playlist options for ATTR_INFO */
typedef enum
{ PLO_NONE        = 0x00,  /* Standard playlist */
  PLO_ALTERNATION = 0x01,  /* Alternation list */
  PLO_SHUFFLE     = 0x10,  /* Enable shuffle for this playlist */
  PLO_NO_SHUFFLE  = 0x20   /* Disable shuffle for this playlist */
} PL_OPTIONS;
/* Playlist sort order 
typedef enum
{ PLS_DEFAULT     = 0x00,  * Inherit default setting *
  PLS_NATIVE      = 0x01,  * Native sort order of the data source *
  PLS_*/
/** Additional attributes */
typedef struct
{ unsigned   ploptions;  /* Playlist options, bit vector of PL_OPTIONS */
  DSTRING    at;         /* Last playing position. */
} ATTR_INFO;
#define ATTR_INFO_INIT { PLO_NONE, {NULL} }

/** @brief Aggregate information on recursive playlist items
 * @details This information is not an aggregate of information of the immediate subitems,
 * but an aggregate of all nested sub items. */
typedef struct
{ int        songs;      /* Number of song items (for playlists), 1 for songs, 0 otherwise */
  int        lists;      /* Number of playlist items (for playlists), otherwise 0 */
  int        invalid;    /* Number of invalid items */
  int        unknown;    /* Number of items with unknown counters. */
} RPL_INFO;
#define RPL_INFO_INIT { 0, 0, 0, 1 }

/** @brief Detailed aggregate information on recursive playlist items
 * @details This information is not an aggregate of information of the immediate subitems,
 * but an aggregate of all nested sub items. */
typedef struct
{ PM123_TIME totallength;/* Playing time in seconds of the whole playlist, 0 -> unknown */
  int        unk_length; /* Number of items with unknown length. */
  PM123_SIZE totalsize;  /* Physical size of all included files, 0 -> unknown */
  int        unk_size;   /* Number of items with unknown size. */
} DRPL_INFO;
#define DRPL_INFO_INIT { 0, 1, 0, 1 }

/** @brief Info about a referenced item in a playlist.
 * @details This kind of information is only used in conjunction with DECODER_INFO_ENUMERATION_CB / DECODER_SAVE_ENUMERATION_CB. */
typedef struct
{ DSTRING    alias;      /* Alias name for a object reference or NULL on default. */
  DSTRING    start;      /* Start location in the referenced object as PM123 location string.
                          * NULL = the beginning of the object. */
  DSTRING    stop;       /* Stop location in the referenced object as PM123 location string.
                          * NULL = until the end of the object. */
  float      pregap;     /* Gap before playing this item */
  float      postgap;    /* Gap after playing this item */
  float      gain;       /* Additional playback gain in dB */
} ITEM_INFO;
#define ITEM_INFO_INIT { {NULL}, {NULL}, {NULL}, -1, -1, -1000 }

/** Information types for \c INFO_BUNDLE */
typedef enum
{ INFO_NONE  = 0x0000,
  INFO_PHYS  = 0x0001,   /* content of PHYS_INFO */
  INFO_TECH  = 0x0002,   /* content of TECH_INFO */
  INFO_OBJ   = 0x0004,   /* content of OBJ_INFO  */
  INFO_META  = 0x0008,   /* content of META_INFO */
  INFO_ATTR  = 0x0010,   /* content of ATTR_INFO */
  INFO_CHILD = 0x0080,   /* Children */
  INFO_RPL   = 0x0100,   /* content of RPL_INFO  */
  INFO_DRPL  = 0x0200,   /* content of DRPL_INFO */
  INFO_ITEM  = 0x0400    /* content of ITEM_INFO */
} INFOTYPE;

/** All of the above information together */
typedef struct
{ PHYS_INFO* phys;       /* [INFO_PHYS] Basic Information about the source */
  TECH_INFO* tech;       /* [INFO_TECH] Technical informations about the source */
  OBJ_INFO*  obj;        /* [INFO_OBJ]  Detailed object information */
  META_INFO* meta;       /* [INFO_META] Song information */
  ATTR_INFO* attr;       /* [INFO_ATTR] Additional attributes */
  RPL_INFO*  rpl;        /* [INFO_RPL]  Recursive playlist extensions */
  DRPL_INFO* drpl;       /* [INFO_DRPL] Detailed recursive playlist extensions */
  ITEM_INFO* item;       /* [INFO_ITEM] Referenced item info */
} INFO_BUNDLE;

/** Same as \c INFO_BUNDLE but with const volatile sub-structures. */
typedef struct
{ volatile const PHYS_INFO*  phys;
  volatile const TECH_INFO*  tech;
  volatile const OBJ_INFO*   obj;
  volatile const META_INFO*  meta;
  volatile const ATTR_INFO*  attr;
  volatile const RPL_INFO*   rpl;
  volatile const DRPL_INFO*  drpl;
  volatile const ITEM_INFO*  item;
} INFO_BUNDLE_CV;

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif /* PM123_FORMAT_H */
