/*
 * Copyright (C) 1998, 1999, 2002 Espen Skoglund
 * Copyright (C) 2000-2004 Haavard Kvaalen
 * Copyright (C) 2007 Dmitry A.Steklenev
 *
 * $Id: id3v2_frame.c,v 1.6 2008/04/21 08:30:11 glass Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>

#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <debuglog.h>

#include "id3v2/id3v2.h"
#include "id3v2/id3v2_header.h"

/*
 * Description of all valid ID3v2 frames.
 */

static const ID3V2_FRAMEDESC framedesc[] =
{
  { ID3V2_AENC, "Audio encryption"                                 },
  { ID3V2_APIC, "Attached picture"                                 },
  { ID3V2_ASPI, "Audio seek point index"                           }, /* v4 only */

  { ID3V2_COMM, "Comments"                                         },
  { ID3V2_COMR, "Commercial frame"                                 },

  { ID3V2_ENCR, "Encryption method registration"                   },
  { ID3V2_EQUA, "Equalization"                                     }, /* v3 only */
  { ID3V2_EQU2, "Equalization (2)"                                 }, /* v4 only */
  { ID3V2_ETCO, "Event timing codes"                               },

  { ID3V2_GEOB, "General encapsulated object"                      },
  { ID3V2_GRID, "Group identification registration"                },

  { ID3V2_IPLS, "Involved people list"                             }, /* v3 only */

  { ID3V2_LINK, "Linked information"                               },

  { ID3V2_MCDI, "Music CD identifier"                              },
  { ID3V2_MLLT, "MPEG location lookup table"                       },

  { ID3V2_OWNE, "Ownership frame"                                  },

  { ID3V2_PRIV, "Private frame"                                    },
  { ID3V2_PCNT, "Play counter"                                     },
  { ID3V2_POPM, "Popularimeter"                                    },
  { ID3V2_POSS, "Position synchronisation frame"                   },

  { ID3V2_RBUF, "Recommended buffer size"                          },
  { ID3V2_RVAD, "Relative volume adjustment"                       }, /* v3 only */
  { ID3V2_RVA2, "RVA2 Relative volume adjustment (2)"              }, /* v4 only */
  { ID3V2_RVRB, "Reverb"                                           },

  { ID3V2_SEEK, "Seek frame"                                       }, /* v4 only */
  { ID3V2_SIGN, "Signature frame"                                  }, /* v4 only */
  { ID3V2_SYLT, "Synchronized lyric/text"                          },
  { ID3V2_SYTC, "Synchronized tempo codes"                         },

  { ID3V2_TALB, "Album/Movie/Show title"                           },
  { ID3V2_TBPM, "BPM (beats per minute)"                           },
  { ID3V2_TCOM, "Composer"                                         },
  { ID3V2_TCON, "Content type"                                     },
  { ID3V2_TCOP, "Copyright message"                                },
  { ID3V2_TDAT, "Date"                                             }, /* v3 only */
  { ID3V2_TDEN, "Encoding time"                                    }, /* v4 only */
  { ID3V2_TDLY, "Playlist delay"                                   },
  { ID3V2_TDOR, "Original release time"                            }, /* v4 only */
  { ID3V2_TDRC, "Recording time"                                   }, /* v4 only */
  { ID3V2_TDRL, "Release time"                                     }, /* v4 only */
  { ID3V2_TDTG, "Tagging time"                                     }, /* v4 only */

  { ID3V2_TENC, "Encoded by"                                       },
  { ID3V2_TEXT, "Lyricist/Text writer"                             },
  { ID3V2_TFLT, "File type"                                        },
  { ID3V2_TIME, "Time"                                             }, /* v3 only */
  { ID3V2_TIPL, "Involved people list"                             }, /* v4 only */
  { ID3V2_TIT1, "Content group description"                        },
  { ID3V2_TIT2, "Title/songname/content description"               },
  { ID3V2_TIT3, "Subtitle/Description refinement"                  },
  { ID3V2_TKEY, "Initial key"                                      },
  { ID3V2_TLAN, "Language(s)"                                      },
  { ID3V2_TLEN, "Length"                                           },
  { ID3V2_TMCL, "Musician credits list"                            }, /* v4 only */
  { ID3V2_TMOO, "Mood"                                             }, /* v4 only */
  { ID3V2_TMED, "Media type"                                       },
  { ID3V2_TOAL, "Original album/movie/show title"                  },
  { ID3V2_TOFN, "Original filename"                                },
  { ID3V2_TOLY, "Original lyricist(s)/text writer(s)"              },
  { ID3V2_TOPE, "Original artist(s)/performer(s)"                  },
  { ID3V2_TORY, "Original release year"                            }, /* v3 only */
  { ID3V2_TOWN, "File owner/licensee"                              },
  { ID3V2_TPE1, "Lead performer(s)/Soloist(s)"                     },
  { ID3V2_TPE2, "Band/orchestra/accompaniment"                     },
  { ID3V2_TPE3, "Conductor/performer refinement"                   },
  { ID3V2_TPE4, "Interpreted, remixed, or otherwise modified by"   },
  { ID3V2_TPOS, "Part of a set"                                    },
  { ID3V2_TPRO, "Produced notice"                                  }, /* v4 only */
  { ID3V2_TPUB, "Publisher"                                        },
  { ID3V2_TRCK, "Track number/Position in set"                     },
  { ID3V2_TRDA, "Recording dates"                                  }, /* v3 only */
  { ID3V2_TRSN, "Internet radio station name"                      },
  { ID3V2_TRSO, "Internet radio station owner"                     },
  { ID3V2_TSIZ, "Size"                                             }, /* v3 only */
  { ID3V2_TSOA, "Album sort order"                                 }, /* v4 only */
  { ID3V2_TSOP, "Performer sort order"                             }, /* v4 only */
  { ID3V2_TSOT, "Title sort order"                                 }, /* v4 only */

  { ID3V2_TSRC, "ISRC (international standard recording code)"     },
  { ID3V2_TSSE, "Software/Hardware and settings used for encoding" },
  { ID3V2_TSST, "Set subtitle"                                     }, /* v4 only */
  { ID3V2_TYER, "Year"                                             }, /* v3 only */
  { ID3V2_TXXX, "User defined text information frame"              },

  { ID3V2_UFID, "Unique file identifier"                           },
  { ID3V2_USER, "Terms of use"                                     },
  { ID3V2_USLT, "Unsychronized lyric/text transcription"           },

  { ID3V2_WCOM, "Commercial information"                           },
  { ID3V2_WCOP, "Copyright/Legal information"                      },
  { ID3V2_WOAF, "Official audio file webpage"                      },
  { ID3V2_WOAR, "Official artist/performer webpage"                },
  { ID3V2_WOAS, "Official audio source webpage"                    },
  { ID3V2_WORS, "Official internet radio station homepage"         },
  { ID3V2_WPAY, "Payment"                                          },
  { ID3V2_WPUB, "Publishers official webpage"                      },
  { ID3V2_WXXX, "User defined URL link frame"                      },
};

struct id3_framedesc22
{
  ID3V2_ID22 fd_v22;
  ID3V2_ID   fd_v24;
};

static struct id3_framedesc22 framedesc22[] =
{
  { ID3V2_BUF, ID3V2_RBUF },     /* Recommended buffer size */

  { ID3V2_CNT, ID3V2_PCNT },     /* Play counter */
  { ID3V2_COM, ID3V2_COMM },     /* Comments */
  { ID3V2_CRA, ID3V2_AENC },     /* Audio encryption */
  { ID3V2_CRM, ID3V2_NULL },     /* Encrypted meta frame */

  { ID3V2_ETC, ID3V2_ETCO },     /* Event timing codes */
  /* Could be converted to EQU2 */
  { ID3V2_EQU, ID3V2_NULL },     /* Equalization */

  { ID3V2_GEO, ID3V2_GEOB },     /* General encapsulated object */

  /* Would need conversion to TIPL */
  { ID3V2_IPL, ID3V2_NULL },     /* Involved people list */

  /* This is so fragile it's not worth trying to save */
  { ID3V2_LNK, ID3V2_NULL },     /* Linked information */

  { ID3V2_MCI, ID3V2_MCDI },     /* Music CD Identifier */
  { ID3V2_MLL, ID3V2_MLLT },     /* MPEG location lookup table */

  /* Would need to convert header for APIC */
  { ID3V2_PIC, ID3V2_NULL },     /* Attached picture */
  { ID3V2_POP, ID3V2_POPM },     /* Popularimeter */

  { ID3V2_REV, ID3V2_RVRB },     /* Reverb */
  /* Could be converted to RVA2 */
  { ID3V2_RVA, ID3V2_NULL },     /* Relative volume adjustment */

  { ID3V2_SLT, ID3V2_SYLT },     /* Synchronized lyric/text */
  { ID3V2_STC, ID3V2_SYTC },     /* Synced tempo codes */

  { ID3V2_TAL, ID3V2_TALB },     /* Album/Movie/Show title */
  { ID3V2_TBP, ID3V2_TBPM },     /* BPM (Beats Per Minute) */
  { ID3V2_TCM, ID3V2_TCOM },     /* Composer */
  { ID3V2_TCO, ID3V2_TCON },     /* Content type */
  { ID3V2_TCR, ID3V2_TCOP },     /* Copyright message */
  /* This could be incorporated into TDRC */
  { ID3V2_TDA, ID3V2_NULL },     /* Date */
  { ID3V2_TDY, ID3V2_TDLY },     /* Playlist delay */
  { ID3V2_TEN, ID3V2_TENC },     /* Encoded by */
  { ID3V2_TFT, ID3V2_TFLT },     /* File type */
  /* This could be incorporated into TDRC */
  { ID3V2_TIM, ID3V2_NULL },     /* Time */
  { ID3V2_TKE, ID3V2_TKEY },     /* Initial key */
  { ID3V2_TLA, ID3V2_TLAN },     /* Language(s) */
  { ID3V2_TLE, ID3V2_TLEN },     /* Length */
  { ID3V2_TMT, ID3V2_TMED },     /* Media type */
  { ID3V2_TOA, ID3V2_TOPE },     /* Original artist(s)/performer(s) */
  { ID3V2_TOF, ID3V2_TOFN },     /* Original filename */
  { ID3V2_TOL, ID3V2_TOLY },     /* Original Lyricist(s)/text writer(s) */
  /*
   * The docs says that original release year should be in
   * milliseconds!  Hopefully that is a typo.
   */
  { ID3V2_TOR, ID3V2_TDOR },     /* Original release year */
  { ID3V2_TOT, ID3V2_TOAL },     /* Original album/Movie/Show title */
  { ID3V2_TP1, ID3V2_TPE1 },     /* Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group */
  { ID3V2_TP2, ID3V2_TPE2 },     /* Band/Orchestra/Accompaniment */
  { ID3V2_TP3, ID3V2_TPE3 },     /* Conductor/Performer refinement */
  { ID3V2_TP4, ID3V2_TPE4 },     /* Interpreted, remixed, or otherwise modified by */
  { ID3V2_TPA, ID3V2_TPOS },     /* Part of a set */
  { ID3V2_TPB, ID3V2_TPUB },     /* Publisher */
  { ID3V2_TRC, ID3V2_TSRC },     /* ISRC (International Standard Recording Code) */
  { ID3V2_TRD, ID3V2_NULL },     /* Recording dates */
  { ID3V2_TRK, ID3V2_TRCK },     /* Track number/Position in set */
  { ID3V2_TSI, ID3V2_NULL },     /* Size */
  { ID3V2_TSS, ID3V2_TSSE },     /* Software/hardware and settings used for encoding */
  { ID3V2_TT1, ID3V2_TIT1 },     /* Content group description */
  { ID3V2_TT2, ID3V2_TIT2 },     /* Title/Songname/Content description */
  { ID3V2_TT3, ID3V2_TIT3 },     /* Subtitle/Description refinement */
  { ID3V2_TXT, ID3V2_TEXT },     /* Lyricist/text writer */
  { ID3V2_TXX, ID3V2_TXXX },     /* User defined text information frame */
  { ID3V2_TYE, ID3V2_TDRC },     /* Year */

  { ID3V2_UFI, ID3V2_UFID },     /* Unique file identifier */
  { ID3V2_ULT, ID3V2_USLT },     /* Unsychronized lyric/text transcription */

  { ID3V2_WAF, ID3V2_WOAF },     /* Official audio file webpage */
  { ID3V2_WAR, ID3V2_WOAR },     /* Official artist/performer webpage */
  { ID3V2_WAS, ID3V2_WOAS },     /* Official audio source webpage */
  { ID3V2_WCM, ID3V2_WCOM },     /* Commercial information */
  { ID3V2_WCP, ID3V2_WCOP },     /* Copyright/Legal information */
  { ID3V2_WPB, ID3V2_WPUB },     /* Publishers official webpage */
  { ID3V2_WXX, ID3V2_WXXX },     /* User defined URL link frame */
};

static const ID3V2_FRAMEDESC*
id3v2_find_frame_description( ID3V2_ID id )
{
  for( size_t i = 0; i < sizeof( framedesc ) / sizeof( ID3V2_FRAMEDESC ); i++ ) {
    if( framedesc[i].fd_id == id ) {
      return &framedesc[i];
    }
  }
  return NULL;
}

static ID3V2_ID
id3v2_find_v24_id( ID3V2_ID22 v22 )
{
  for( size_t i = 0; i < sizeof( framedesc22 ) / sizeof( framedesc22[0] ); i++ ) {
    if( framedesc22[i].fd_v22 == v22 ) {
      return framedesc22[i].fd_v24;
    }
  }

  return ID3V2_NULL;
}

ID3V2_FRAME::ID3V2_FRAME(ID3V2_TAG* owner)
: fr_owner(owner)
, fr_desc(NULL)
, fr_flags(0)
, fr_encryption(0)
, fr_grouping(0)
, fr_altered(0)
, fr_data(NULL)
, fr_size(0)
, fr_raw_data(NULL)
, fr_raw_size(0)
, fr_data_z(NULL)
, fr_size_z(0)
{}

ID3V2_FRAME::~ID3V2_FRAME()
{}

static int
id3v2_frame_extra_headers( ID3V2_FRAME* frame )
{
  int retv = 0;

  // If frame is compressed, we have four extra bytes in the header.
  if( frame->fr_flags & ID3V2_FHFLAG_COMPRESS ) {
    retv += 4;
  }

  // If frame is encrypted, we have one extra byte in the header.
  if( frame->fr_flags & ID3V2_FHFLAG_ENCRYPT ) {
    retv += 1;
  }

  // If frame has grouping identity, we have one extra byte in the header.
  if( frame->fr_flags & ID3V2_FHFLAG_GROUP ) {
    retv += 1;
  }

  return retv;
}

INLINE void*
id3v2_get_frame_data_ptr( ID3V2_FRAME* frame ) {
  return (char*)frame->fr_raw_data + id3v2_frame_extra_headers( frame );
}

INLINE int
id3v2_get_frame_size( ID3V2_FRAME* frame ) {
  return frame->fr_raw_size - id3v2_frame_extra_headers( frame );
}

static int
id3v2_read_frame_v22( ID3V2_TAG* id3 )
{
  ID3V2_FRAME* frame;
  ID3V2_ID22   id;
  ID3V2_ID     idv24;
  uint8_t*     buf;
  int          size;

  // Read frame header.
  buf = (uint8_t*)id3->id3_read( id3, NULL, ID3V2_FRAMEHDR_SIZE_22 );
  if( buf == NULL ) {
    return -1;
  }

  // If we encounter an invalid frame id, we assume that there
  // is some. We just skip the rest of the ID3 tag.

  if(( !((( buf[0] >= '0' ) && ( buf[0] <= '9' )) || (( buf[0] >= 'A' ) && ( buf[0] <= 'Z' ))))) {
    id3->id3_seek( id3, id3->id3_size - id3->id3_pos );
    return 0;
  }

  id = ID3V2_FRAME_ID_22_RAW(buf);
  size = buf[3] << 16 | buf[4] << 8 | buf[5];

  if(( idv24 = id3v2_find_v24_id( id )) == ID3V2_NULL ) {
    if( id3->id3_seek( id3, size ) < 0 ) {
      return -1;
    }
    return 0;
  }

  // Allocate frame.
  frame = new ID3V2_FRAME(id3);

  frame->fr_raw_size = size;
  frame->fr_desc     = id3v2_find_frame_description( idv24 );

  if( frame->fr_raw_size > 1000000 ) {
    delete frame;
    return -1;
  }

  // Initialize frame.

  // We allocate 2 extra bytes. This simplifies retrieval of
  // text strings.

  frame->fr_raw_data = calloc( 1, frame->fr_raw_size + 2 );

  if( id3->id3_read( id3, frame->fr_raw_data, frame->fr_raw_size ) == NULL )
  {
    free( frame->fr_raw_data );
    delete frame;
    return -1;
  }

  frame->fr_data = frame->fr_raw_data;
  frame->fr_size = frame->fr_raw_size;

  // Insert frame into list.
  id3->id3_frames.append() = frame;

  DEBUGLOG(( "id3v2: read frame %.4s [%.3s], fl=%04X, size=%08d\n",
             &idv24, buf, frame->fr_flags, frame->fr_raw_size));
  return 0;
}

/* Read next frame from the indicated ID3 tag. Return 0 upon
   success, or -1 if an error occurred. */
int
id3v2_read_frame( ID3V2_TAG* id3 )
{
  ID3V2_FRAME* frame;
  ID3V2_ID id;
  uint32_t raw_size;
  uint8_t* buf;

  if( id3->id3_version == 2 ) {
    return id3v2_read_frame_v22( id3 );
  }

  // Read frame header.
  buf = (uint8_t*)id3->id3_read( id3, NULL, ID3V2_FRAMEHDR_SIZE );

  if( buf == NULL ) {
    return -1;
  }

  // If we encounter an invalid frame id, we assume that there is
  // some padding in the header. We just skip the rest of the ID3
  // tag.

  if(( !((( buf[0] >= '0' ) && ( buf[0] <= '9' )) || (( buf[0] >= 'A' ) && ( buf[0] <= 'Z' ))))) {
    id3->id3_seek( id3, id3->id3_totalsize - id3->id3_pos );
    return 0;
  }

  id = *(ID3V2_ID*)buf;

  if( id3->id3_version >= 4 ) {
    raw_size = ID3_GET_SIZE28( buf[4], buf[5], buf[6], buf[7] );
  } else {
    raw_size = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
  }

  if( raw_size > 1000000 ) {
    return -1;
  }

  // Convert v2.3 frames to v2.4 if it is possible.
  switch( id ) {
    case ID3V2_TYER:
      id = ID3V2_TDRC;
      break;

    case ID3V2_EQUA:  /* Could be converted to EQU2 */
    case ID3V2_IPLS:  /* Could be converted to TMCL and TIPL */
    case ID3V2_RVAD:  /* Could be converted to RVA2 */
    case ID3V2_TDAT:  /* Could be converted to TDRC */
    case ID3V2_TIME:  /* Could be converted to TDRC */
    case ID3V2_TORY:  /* Could be converted to TDOR */
    case ID3V2_TRDA:  /* Could be converted to TDRC */
    case ID3V2_TSIZ:  /* Obsolete */
    {
      DEBUGLOG(( "id3v2: skip frame %.4s\n", &id ));

      if( id3->id3_seek( id3, raw_size ) < 0 ) {
        return -1;
      }
      return 0;
    }
  }

  // Allocate frame.
  frame = new ID3V2_FRAME(id3);

  frame->fr_flags    = buf[8] << 8 | buf[9];
  frame->fr_desc     = id3v2_find_frame_description( id );
  frame->fr_raw_size = raw_size;

  // Check if frame had a valid id.
  if( frame->fr_desc == NULL ) {
    // No. Ignore the frame.
    if( id3->id3_seek( id3, frame->fr_raw_size ) < 0 ) {
      delete frame;
      return -1;
    }
    return 0;
  }

  // Initialize frame.

  // We allocate 2 extra bytes. This simplifies retrieval of
  // text strings.

  frame->fr_raw_data = calloc( 1, frame->fr_raw_size + 2 );

  if( id3->id3_read( id3, frame->fr_raw_data, frame->fr_raw_size ) == NULL )
  {
    free( frame->fr_raw_data );
    delete frame;
    return -1;
  }

  // Check if frame is compressed using zlib.
  if(!( frame->fr_flags & ID3V2_FHFLAG_COMPRESS )) {
    frame->fr_data = id3v2_get_frame_data_ptr( frame );
    frame->fr_size = id3v2_get_frame_size( frame );
  }

  // Insert frame into list.
  id3->id3_frames.append() = frame;

  DEBUGLOG(( "id3v2: read frame %.4s [%.4s], fl=%04X, size=%08d\n",
             &id, buf, frame->fr_flags, frame->fr_raw_size));
  return 0;
}

/* Search in the list of frames for the ID3-tag, and return a frame
   of the indicated type.  If tag contains several frames of the
   indicated type, the third argument tells which of the frames to
   return. */
ID3V2_FRAME*
id3v2_get_frame( const ID3V2_TAG* id3, ID3V2_ID type, int num )
{
  unsigned i;
  for( i = 0; i < id3->id3_frames.size(); i++ )
  {
    ID3V2_FRAME* fr = id3->id3_frames[i];
    if(( fr->fr_desc ) && ( fr->fr_desc->fd_id == type )) {
      if( --num <= 0 ) {
        return fr;
      }
    }
  }
  return NULL;
}

/* Check if frame is compressed, and uncompress if necessary.
   Return 0 upon success, or -1 if an error occurred. */
int
id3v2_decompress_frame( ID3V2_FRAME* frame )
{
  z_stream z;
  int      r;

  if( !( frame->fr_flags & ID3V2_FHFLAG_COMPRESS )) {
    // Frame not compressed.
    return 0;
  }
  if( frame->fr_data_z ) {
    // Frame already decompressed.
    return 0;
  }

  // Fetch the size of the decompressed data.
  if( frame->fr_owner->id3_version >= 4 ) {
    frame->fr_size_z = ID3_GET_SIZE28(((unsigned char*)frame->fr_raw_data)[0],
                                      ((unsigned char*)frame->fr_raw_data)[1],
                                      ((unsigned char*)frame->fr_raw_data)[2],
                                      ((unsigned char*)frame->fr_raw_data)[3] );
  } else {
    frame->fr_size_z = ((unsigned char*)frame->fr_raw_data)[0] << 24 |
                       ((unsigned char*)frame->fr_raw_data)[1] << 16 |
                       ((unsigned char*)frame->fr_raw_data)[2] <<  8 |
                       ((unsigned char*)frame->fr_raw_data)[3];
  }

  if( frame->fr_size_z > 1000000 ) {
    return -1;
  }

  // Allocate memory to hold uncompressed frame.
  frame->fr_data_z = malloc( frame->fr_size_z +
                           ( id3v2_is_text_frame( frame ) ? 2 : 0 ));
  // Initialize zlib.
  z.next_in  = (Bytef*)id3v2_get_frame_data_ptr( frame );
  z.avail_in = id3v2_get_frame_size( frame );
  z.zalloc   = NULL;
  z.zfree    = NULL;
  z.opaque   = NULL;

  r = inflateInit( &z );

  switch( r )
  {
    case Z_OK:
      break;
    case Z_MEM_ERROR:
      frame->fr_owner->id3_error_msg = "zlib - no memory";
      goto error_init;
    case Z_VERSION_ERROR:
      frame->fr_owner->id3_error_msg = "zlib - invalid version";
      goto error_init;
    default:
      frame->fr_owner->id3_error_msg = "zlib - unknown error";
      goto error_init;
  }

  // Decompress frame.
  z.next_out  = (Bytef*)frame->fr_data_z;
  z.avail_out = frame->fr_size_z;
  r = inflate( &z, Z_SYNC_FLUSH );

  switch( r )
  {
    case Z_STREAM_END:
      break;
    case Z_OK:
      if( z.avail_in == 0 ) {
        // This should not be possible with a
        // correct stream. We will be nice
        // however, and try to go on.
        break;
      }
      frame->fr_owner->id3_error_msg = "zlib - buffer exhausted";
      goto error_inflate;
    default:
      frame->fr_owner->id3_error_msg = "zlib - unknown error";
      goto error_inflate;
  }

  r = inflateEnd( &z );
  if( r != Z_OK ) {
    frame->fr_owner->id3_error_msg = "zlib - inflateEnd error";
  }

  // Null-terminate text frames.
  if( id3v2_is_text_frame( frame )) {
    ((char*)frame->fr_data_z )[frame->fr_size_z] = 0;
    ((char*)frame->fr_data_z )[frame->fr_size_z + 1] = 0;
  }
  frame->fr_data = frame->fr_data_z;
  frame->fr_size = frame->fr_size_z + ( id3v2_is_text_frame( frame ) ? 2 : 0 );

  return 0;

error_inflate:
  r = inflateEnd( &z );

error_init:
  free( frame->fr_data_z );
  frame->fr_data_z = NULL;
  return -1;
}

/* Remove frame from ID3 tag and release memory occupied by it. */
int
id3v2_delete_frame( ID3V2_FRAME* frame )
{
  ID3V2_TAG* id3 = frame->fr_owner;
  int        ret = -1;
  unsigned   i;

  // Search for frame in list.
  for( i = 0; i < id3->id3_frames.size(); i++ ) {
    if( id3->id3_frames[i] == frame ) {
      id3->id3_frames.erase(i);
      id3->id3_altered = 1;
      ret = 0;
    }
  }

  // Release memory occupied by frame.
  if( frame->fr_raw_data ) {
    free( frame->fr_raw_data );
  }
  if( frame->fr_data_z ) {
    free( frame->fr_data_z );
  }

  delete frame;
  return ret;
}

/* Add a new frame to the ID3 tag. Return a pointer to the new
   frame, or NULL if an error occurred. */
ID3V2_FRAME*
id3v2_add_frame( ID3V2_TAG* id3, ID3V2_ID type )
{
  ID3V2_FRAME* frame;
  // Allocate frame.
  frame = new ID3V2_FRAME(id3);

  // Initialize frame.

  // Try finding the correct frame descriptor.
  for( size_t i = 0; i < sizeof( framedesc ) / sizeof( ID3V2_FRAMEDESC ); i++ )
  {
    if( framedesc[i].fd_id == type ) {
      frame->fr_desc = &framedesc[i];
      break;
    }
  }

  // Insert frame into list.
  id3->id3_frames.append() = frame;
  id3->id3_altered = 1;

  return frame;
}

/* Copy the given frame into another tag */
ID3V2_FRAME* id3v2_copy_frame( ID3V2_TAG* id3, const ID3V2_FRAME* frame )
{ ID3V2_FRAME* result;
  if (frame == NULL)
    return NULL;
  ASSERT(frame->fr_owner != id3); 
  result = id3v2_add_frame( id3, frame->fr_desc->fd_id );
  if (result == NULL)
    return NULL;
  // deep copy
  result->fr_flags      = frame->fr_flags;
  result->fr_encryption = frame->fr_encryption; 
  result->fr_grouping   = frame->fr_grouping;
  result->fr_altered    = 1;
  
  result->fr_raw_data   = NULL;
  result->fr_data_z     = NULL;
  result->fr_raw_size   = frame->fr_raw_size;
  result->fr_size_z     = frame->fr_size_z;
  
  if (frame->fr_raw_data)
  { result->fr_raw_data = malloc(frame->fr_raw_size + 2);
    if (!result->fr_raw_data)
    { id3v2_delete_frame(result);
      return NULL;
    }
    memcpy(result->fr_raw_data, frame->fr_raw_data, frame->fr_raw_size + 2);
  }
  if (frame->fr_data_z)
  { result->fr_data_z   = malloc(frame->fr_size_z + ( id3v2_is_text_frame( frame ) ? 2 : 0 ));
    if (!result->fr_data_z)
    { id3v2_delete_frame(result);
      return NULL;
    }
    memcpy(result->fr_data_z, frame->fr_data_z, frame->fr_size_z + ( id3v2_is_text_frame( frame ) ? 2 : 0 ));
    result->fr_data     = frame->fr_data
                        ? (char*)frame->fr_data - (char*)frame->fr_data_z + (char*)result->fr_data_z
                        : NULL;
  } else
  { result->fr_data     = frame->fr_data
                        ? (char*)frame->fr_data - (char*)frame->fr_raw_data + (char*)result->fr_raw_data
                        : NULL;
  }
  result->fr_size       = frame->fr_size;

  return result;
}

/* Destroy all frames in an id3 tag, and free all data */
void
id3v2_delete_all_frames( ID3V2_TAG* id3 )
{
  unsigned i;
  for( i = 0; i < id3->id3_frames.size(); i++ )
  {
    ID3V2_FRAME* frame = id3->id3_frames[i];

    // Release memory occupied by frame.
    if( frame->fr_raw_data ) {
      free( frame->fr_raw_data );
    }
    if( frame->fr_data_z ) {
      free( frame->fr_data_z );
    }
    delete frame;
  }

  id3->id3_frames.set_size(0);
}

/* Release memory occupied by frame data. */
void
id3v2_clean_frame( ID3V2_FRAME* frame )
{
  if( frame->fr_raw_data ) {
    free( frame->fr_raw_data );
  }
  if( frame->fr_data_z ) {
    free( frame->fr_data_z );
  }

  frame->fr_raw_data = NULL;
  frame->fr_raw_size = 0;
  frame->fr_data     = NULL;
  frame->fr_size     = 0;
  frame->fr_data_z   = NULL;
  frame->fr_size_z   = 0;
}

