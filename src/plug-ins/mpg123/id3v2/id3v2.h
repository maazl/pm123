/*
 * Copyright (C) 2010-2011 Marcel Mueller
 * Copyright (C) 2007 Dmitry A.Steklenev
 * Copyright (C) 2000-2004 Haavard Kvaalen
 * Copyright (C) 1998, 1999, 2002 Espen Skoglund
 *
 * $Id: id3v2.h,v 1.4 2008/03/28 13:26:40 glass Exp $
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

#ifndef ID3V2_H
#define ID3V2_H

#include <string.h>
#include <xio.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
typedef signed char   int8_t;
typedef unsigned char uint8_t;
typedef unsigned long uint32_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ID3V2_VERSION   4
#define ID3V2_REVISION  0

/* Option flags to id3_get_*(). */
#define ID3V2_GET_NONE    0x0000
#define ID3V2_GET_NOCHECK 0x0001
#define ID3V2_GET_CREATE  0x0002

/* The size of the read/write buffer used by file operations. */
#define ID3V2_FILE_BUFSIZE  32768


/* Structure describing the ID3 tag. */
typedef struct _ID3V2_TAG
{
  int    id3_oflags;            ///< Flags from open call
  int    id3_flags;             ///< Flags from tag header
  int    id3_altered;           ///< Set when tag has been altered
  int    id3_newtag;            ///< Set if this is a new tag

  int    id3_version;           ///< Major ID3 version number
  int    id3_revision;          ///< ID3 revision number
  int    id3_size;              ///< Size of ID3 tag (as dictated by header)

  int    id3_totalsize;         ///< Total size of ID3 tag (including header, footer and padding)
  int    id3_pos;               ///< Current position within tag.

  const char* id3_error_msg;    ///< Last error message

  void*  id3_file;
  void*  id3_filedata;

  /* Functions for doing operations within ID3 tag. */
  int    (*id3_seek)( struct _ID3V2_TAG*, int );
  void*  (*id3_read)( struct _ID3V2_TAG*, void*, int );

  /* List of ID3 frames. */
  struct _ID3V2_FRAME** id3_frames;
  int    id3_frames_count;

} ID3V2_TAG;

typedef uint32_t ID3V2_ID;

typedef uint32_t ID3V2_ID22;

/* Structure describing an ID3 frame type. */
typedef struct _ID3V2_FRAMEDESC
{
  ID3V2_ID fd_id;
  char*    fd_description;

} ID3V2_FRAMEDESC;

/* Structure describing an ID3 frame. */
typedef struct _ID3V2_FRAME
{
  ID3V2_TAG*       fr_owner;
  const ID3V2_FRAMEDESC* fr_desc;
  int              fr_flags;
  unsigned char    fr_encryption;
  unsigned char    fr_grouping;
  unsigned char    fr_altered;

  void*            fr_data;     /* Pointer to frame data, excluding headers */
  int              fr_size;     /* Size of uncompressed frame */

  void*            fr_raw_data; /* Frame data */
  unsigned int     fr_raw_size; /* Frame size */

  void*            fr_data_z;   /* The decompressed compressed frame */
  unsigned int     fr_size_z;   /* Size of decompressed compressed frame */

} ID3V2_FRAME;

/* Text encodings. */
#define ID3V2_ENCODING_ISO_8859_1 0x00
#define ID3V2_ENCODING_UTF16_BOM  0x01
#define ID3V2_ENCODING_UTF16      0x02
#define ID3V2_ENCODING_UTF8       0x03

/* ID3 frame id numbers. Works only for little endian. */
#define ID3V2_FRAME_ID(a,b,c,d) ((const ID3V2_ID)(a|b<<8|c<<16|d<<24))
#define ID3V2_NULL ((const ID3V2_ID)0)

#define ID3V2_AENC ID3V2_FRAME_ID('A','E','N','C')
#define ID3V2_APIC ID3V2_FRAME_ID('A','P','I','C')
#define ID3V2_ASPI ID3V2_FRAME_ID('A','S','P','I')
#define ID3V2_COMM ID3V2_FRAME_ID('C','O','M','M')
#define ID3V2_COMR ID3V2_FRAME_ID('C','O','M','R')
#define ID3V2_ENCR ID3V2_FRAME_ID('E','N','C','R')
#define ID3V2_EQUA ID3V2_FRAME_ID('E','Q','U','A')
#define ID3V2_EQU2 ID3V2_FRAME_ID('E','Q','U','2')
#define ID3V2_ETCO ID3V2_FRAME_ID('E','T','C','O')
#define ID3V2_GEOB ID3V2_FRAME_ID('G','E','O','B')
#define ID3V2_GRID ID3V2_FRAME_ID('G','R','I','D')
#define ID3V2_IPLS ID3V2_FRAME_ID('I','P','L','S')
#define ID3V2_LINK ID3V2_FRAME_ID('L','I','N','K')
#define ID3V2_MCDI ID3V2_FRAME_ID('M','C','D','I')
#define ID3V2_MLLT ID3V2_FRAME_ID('M','L','L','T')
#define ID3V2_OWNE ID3V2_FRAME_ID('O','W','N','E')
#define ID3V2_PRIV ID3V2_FRAME_ID('P','R','I','V')
#define ID3V2_PCNT ID3V2_FRAME_ID('P','C','N','T')
#define ID3V2_POPM ID3V2_FRAME_ID('P','O','P','M')
#define ID3V2_POSS ID3V2_FRAME_ID('P','O','S','S')
#define ID3V2_RBUF ID3V2_FRAME_ID('R','B','U','F')
#define ID3V2_RVAD ID3V2_FRAME_ID('R','V','A','D')
#define ID3V2_RVA2 ID3V2_FRAME_ID('R','V','A','2')
#define ID3V2_RVRB ID3V2_FRAME_ID('R','V','R','B')
#define ID3V2_SEEK ID3V2_FRAME_ID('S','E','E','K')
#define ID3V2_SIGN ID3V2_FRAME_ID('S','I','G','N')
#define ID3V2_SYLT ID3V2_FRAME_ID('S','Y','L','T')
#define ID3V2_SYTC ID3V2_FRAME_ID('S','Y','T','C')
#define ID3V2_TALB ID3V2_FRAME_ID('T','A','L','B')
#define ID3V2_TBPM ID3V2_FRAME_ID('T','B','P','M')
#define ID3V2_TCOM ID3V2_FRAME_ID('T','C','O','M')
#define ID3V2_TCON ID3V2_FRAME_ID('T','C','O','N')
#define ID3V2_TCOP ID3V2_FRAME_ID('T','C','O','P')
#define ID3V2_TDAT ID3V2_FRAME_ID('T','D','A','T')
#define ID3V2_TDEN ID3V2_FRAME_ID('T','D','E','N')
#define ID3V2_TDLY ID3V2_FRAME_ID('T','D','L','Y')
#define ID3V2_TDOR ID3V2_FRAME_ID('T','D','O','R')
#define ID3V2_TDRC ID3V2_FRAME_ID('T','D','R','C')
#define ID3V2_TDRL ID3V2_FRAME_ID('T','D','R','L')
#define ID3V2_TDTG ID3V2_FRAME_ID('T','D','T','G')
#define ID3V2_TENC ID3V2_FRAME_ID('T','E','N','C')
#define ID3V2_TEXT ID3V2_FRAME_ID('T','E','X','T')
#define ID3V2_TFLT ID3V2_FRAME_ID('T','F','L','T')
#define ID3V2_TIME ID3V2_FRAME_ID('T','I','M','E')
#define ID3V2_TIPL ID3V2_FRAME_ID('T','I','P','L')
#define ID3V2_TIT1 ID3V2_FRAME_ID('T','I','T','1')
#define ID3V2_TIT2 ID3V2_FRAME_ID('T','I','T','2')
#define ID3V2_TIT3 ID3V2_FRAME_ID('T','I','T','3')
#define ID3V2_TKEY ID3V2_FRAME_ID('T','K','E','Y')
#define ID3V2_TLAN ID3V2_FRAME_ID('T','L','A','N')
#define ID3V2_TLEN ID3V2_FRAME_ID('T','L','E','N')
#define ID3V2_TMCL ID3V2_FRAME_ID('T','M','C','L')
#define ID3V2_TMED ID3V2_FRAME_ID('T','M','E','D')
#define ID3V2_TMOO ID3V2_FRAME_ID('T','M','O','O')
#define ID3V2_TOAL ID3V2_FRAME_ID('T','O','A','L')
#define ID3V2_TOFN ID3V2_FRAME_ID('T','O','F','N')
#define ID3V2_TOLY ID3V2_FRAME_ID('T','O','L','Y')
#define ID3V2_TOPE ID3V2_FRAME_ID('T','O','P','E')
#define ID3V2_TORY ID3V2_FRAME_ID('T','O','R','Y')
#define ID3V2_TOWN ID3V2_FRAME_ID('T','O','W','N')
#define ID3V2_TPE1 ID3V2_FRAME_ID('T','P','E','1')
#define ID3V2_TPE2 ID3V2_FRAME_ID('T','P','E','2')
#define ID3V2_TPE3 ID3V2_FRAME_ID('T','P','E','3')
#define ID3V2_TPE4 ID3V2_FRAME_ID('T','P','E','4')
#define ID3V2_TPOS ID3V2_FRAME_ID('T','P','O','S')
#define ID3V2_TPRO ID3V2_FRAME_ID('T','P','R','O')
#define ID3V2_TPUB ID3V2_FRAME_ID('T','P','U','B')
#define ID3V2_TRCK ID3V2_FRAME_ID('T','R','C','K')
#define ID3V2_TRDA ID3V2_FRAME_ID('T','R','D','A')
#define ID3V2_TRSN ID3V2_FRAME_ID('T','R','S','N')
#define ID3V2_TRSO ID3V2_FRAME_ID('T','R','S','O')
#define ID3V2_TSIZ ID3V2_FRAME_ID('T','S','I','Z')
#define ID3V2_TSOA ID3V2_FRAME_ID('T','S','O','A')
#define ID3V2_TSOP ID3V2_FRAME_ID('T','S','O','P')
#define ID3V2_TSOT ID3V2_FRAME_ID('T','S','O','T')
#define ID3V2_TSRC ID3V2_FRAME_ID('T','S','R','C')
#define ID3V2_TSSE ID3V2_FRAME_ID('T','S','S','E')
#define ID3V2_TSST ID3V2_FRAME_ID('T','S','S','T')
#define ID3V2_TYER ID3V2_FRAME_ID('T','Y','E','R')
#define ID3V2_TXXX ID3V2_FRAME_ID('T','X','X','X')
#define ID3V2_UFID ID3V2_FRAME_ID('U','F','I','D')
#define ID3V2_USER ID3V2_FRAME_ID('U','S','E','R')
#define ID3V2_USLT ID3V2_FRAME_ID('U','S','L','T')
#define ID3V2_WCOM ID3V2_FRAME_ID('W','C','O','M')
#define ID3V2_WCOP ID3V2_FRAME_ID('W','C','O','P')
#define ID3V2_WOAF ID3V2_FRAME_ID('W','O','A','F')
#define ID3V2_WOAR ID3V2_FRAME_ID('W','O','A','R')
#define ID3V2_WOAS ID3V2_FRAME_ID('W','O','A','S')
#define ID3V2_WORS ID3V2_FRAME_ID('W','O','R','S')
#define ID3V2_WPAY ID3V2_FRAME_ID('W','P','A','Y')
#define ID3V2_WPUB ID3V2_FRAME_ID('W','P','U','B')
#define ID3V2_WXXX ID3V2_FRAME_ID('W','X','X','X')

/* Version 2.2.0 */
#define ID3V2_FRAME_ID_22(a,b,c) ((const ID3V2_ID22)(a|b<<8|c<<16))
#define ID3V2_FRAME_ID_22_RAW(a) (*(const ID3V2_ID22*)(a) & 0xffffff)

#define ID3V2_BUF  ID3V2_FRAME_ID_22('B','U','F')
#define ID3V2_CNT  ID3V2_FRAME_ID_22('C','N','T')
#define ID3V2_COM  ID3V2_FRAME_ID_22('C','O','M')
#define ID3V2_CRA  ID3V2_FRAME_ID_22('C','R','A')
#define ID3V2_CRM  ID3V2_FRAME_ID_22('C','R','M')
#define ID3V2_ETC  ID3V2_FRAME_ID_22('E','T','C')
#define ID3V2_EQU  ID3V2_FRAME_ID_22('E','Q','U')
#define ID3V2_GEO  ID3V2_FRAME_ID_22('G','E','O')
#define ID3V2_IPL  ID3V2_FRAME_ID_22('I','P','L')
#define ID3V2_LNK  ID3V2_FRAME_ID_22('L','N','K')
#define ID3V2_MCI  ID3V2_FRAME_ID_22('M','C','I')
#define ID3V2_MLL  ID3V2_FRAME_ID_22('M','L','L')
#define ID3V2_PIC  ID3V2_FRAME_ID_22('P','I','C')
#define ID3V2_POP  ID3V2_FRAME_ID_22('P','O','P')
#define ID3V2_REV  ID3V2_FRAME_ID_22('R','E','V')
#define ID3V2_RVA  ID3V2_FRAME_ID_22('R','V','A')
#define ID3V2_SLT  ID3V2_FRAME_ID_22('S','L','T')
#define ID3V2_STC  ID3V2_FRAME_ID_22('S','T','C')
#define ID3V2_TAL  ID3V2_FRAME_ID_22('T','A','L')
#define ID3V2_TBP  ID3V2_FRAME_ID_22('T','B','P')
#define ID3V2_TCM  ID3V2_FRAME_ID_22('T','C','M')
#define ID3V2_TCO  ID3V2_FRAME_ID_22('T','C','O')
#define ID3V2_TCR  ID3V2_FRAME_ID_22('T','C','R')
#define ID3V2_TDA  ID3V2_FRAME_ID_22('T','D','A')
#define ID3V2_TDY  ID3V2_FRAME_ID_22('T','D','Y')
#define ID3V2_TEN  ID3V2_FRAME_ID_22('T','E','N')
#define ID3V2_TFT  ID3V2_FRAME_ID_22('T','F','T')
#define ID3V2_TIM  ID3V2_FRAME_ID_22('T','I','M')
#define ID3V2_TKE  ID3V2_FRAME_ID_22('T','K','E')
#define ID3V2_TLA  ID3V2_FRAME_ID_22('T','L','A')
#define ID3V2_TLE  ID3V2_FRAME_ID_22('T','L','E')
#define ID3V2_TMT  ID3V2_FRAME_ID_22('T','M','T')
#define ID3V2_TOA  ID3V2_FRAME_ID_22('T','O','A')
#define ID3V2_TOF  ID3V2_FRAME_ID_22('T','O','F')
#define ID3V2_TOL  ID3V2_FRAME_ID_22('T','O','L')
#define ID3V2_TOR  ID3V2_FRAME_ID_22('T','O','R')
#define ID3V2_TOT  ID3V2_FRAME_ID_22('T','O','T')
#define ID3V2_TP1  ID3V2_FRAME_ID_22('T','P','1')
#define ID3V2_TP2  ID3V2_FRAME_ID_22('T','P','2')
#define ID3V2_TP3  ID3V2_FRAME_ID_22('T','P','3')
#define ID3V2_TP4  ID3V2_FRAME_ID_22('T','P','4')
#define ID3V2_TPA  ID3V2_FRAME_ID_22('T','P','A')
#define ID3V2_TPB  ID3V2_FRAME_ID_22('T','P','B')
#define ID3V2_TRC  ID3V2_FRAME_ID_22('T','R','C')
#define ID3V2_TRD  ID3V2_FRAME_ID_22('T','R','D')
#define ID3V2_TRK  ID3V2_FRAME_ID_22('T','R','K')
#define ID3V2_TSI  ID3V2_FRAME_ID_22('T','S','I')
#define ID3V2_TSS  ID3V2_FRAME_ID_22('T','S','S')
#define ID3V2_TT1  ID3V2_FRAME_ID_22('T','T','1')
#define ID3V2_TT2  ID3V2_FRAME_ID_22('T','T','2')
#define ID3V2_TT3  ID3V2_FRAME_ID_22('T','T','3')
#define ID3V2_TXT  ID3V2_FRAME_ID_22('T','X','T')
#define ID3V2_TXX  ID3V2_FRAME_ID_22('T','X','X')
#define ID3V2_TYE  ID3V2_FRAME_ID_22('T','Y','E')
#define ID3V2_UFI  ID3V2_FRAME_ID_22('U','F','I')
#define ID3V2_ULT  ID3V2_FRAME_ID_22('U','L','T')
#define ID3V2_WAF  ID3V2_FRAME_ID_22('W','A','F')
#define ID3V2_WAR  ID3V2_FRAME_ID_22('W','A','R')
#define ID3V2_WAS  ID3V2_FRAME_ID_22('W','A','S')
#define ID3V2_WCM  ID3V2_FRAME_ID_22('W','C','M')
#define ID3V2_WCP  ID3V2_FRAME_ID_22('W','C','P')
#define ID3V2_WPB  ID3V2_FRAME_ID_22('W','P','B')
#define ID3V2_WXX  ID3V2_FRAME_ID_22('W','X','X')

#define ID3V2_TEXT_FRAME_ENCODING( frame ) \
  (*(int8_t*)( frame )->fr_data )

#define ID3V2_TEXT_FRAME_PTR( frame ) \
  ((char*)( frame )->fr_data + 1 )

/* Creates a new ID3 tag structure. */
ID3V2_TAG* id3v2_new_tag( void );

/* Read an ID3 tag using a file pointer. Return a pointer to a
   structure describing the ID3 tag, or NULL if an error occured. */
ID3V2_TAG* id3v2_get_tag( XFILE* file, int flags );

/* Read an ID3 tag using a memory source. Return a pointer to a
   structure describing the ID3 tag, or NULL if an error occured. */
ID3V2_TAG* id3v2_load_tag( char* tagdata, size_t taglen, int flags );

/* Free all resources associated with the ID3 tag. */
void id3v2_free_tag( ID3V2_TAG* id3 );

/* When altering a file, some ID3 tags should be discarded. As the ID3
   library has no means of knowing when a file has been altered
   outside of the library, this function must be called manually
   whenever the file is altered. */
int id3v2_alter_tag( ID3V2_TAG* id3 );

/* Remove the ID3 tag from the file. Takes care of resizing
   the file, if needed. Returns 0 upon success, or -1 if an
   error occured. */
int id3v2_wipe_tag( XFILE* file, const char* savename );

/* Writes the ID3 tag to the file. Returns 0 upon success, returns 1 if
   the ID3 tag is saved  to savename or returns -1 if an error occured. */
int id3v2_set_tag( XFILE* file, ID3V2_TAG* id3, const char* savename );

/* Search in the list of frames for the ID3-tag, and return a frame
   of the indicated type.  If tag contains several frames of the
   indicated type, the third argument tells which of the frames to
   return. */
ID3V2_FRAME* id3v2_get_frame( const ID3V2_TAG* id3, ID3V2_ID type, int num );

/* Add a new frame to the ID3 tag. Return a pointer to the new
   frame, or NULL if an error occured. */
ID3V2_FRAME* id3v2_add_frame( ID3V2_TAG* id3, ID3V2_ID type );

/* Copy the given frame into another tag */
ID3V2_FRAME* id3v2_copy_frame( ID3V2_TAG* id3, const ID3V2_FRAME* frame );

/* Read next frame from the indicated ID3 tag. Return 0 upon
   success, or -1 if an error occured. */
int id3v2_read_frame( ID3V2_TAG* id3 );

/* Check if frame is compressed, and uncompress if necessary.
   Return 0 upon success, or -1 if an error occured. */
int id3v2_decompress_frame( ID3V2_FRAME* frame );

/* Remove frame from ID3 tag and release memory ocupied by it. */
int  id3v2_delete_frame( ID3V2_FRAME* frame );
/* Destroy all frames in an id3 tag, and free all data. */
void id3v2_delete_all_frames( ID3V2_TAG* id3 );
/* Release memory occupied by frame data. */
void id3v2_clean_frame( ID3V2_FRAME* frame );

/* Is the specified frame a text based frame. */
int id3v2_is_text_frame( const ID3V2_FRAME* frame );
/* Return string contents of frame. */
char* id3v2_get_string_ex( ID3V2_FRAME* frame, char* result, size_t size, unsigned def_cp );
/* Get description part of a frame. */
char* id3v2_get_description( ID3V2_FRAME* frame, char* result, size_t size );
/* Set text for the indicated frame. */
int id3v2_set_string( ID3V2_FRAME* frame, const char* source, int8_t encoding );

#ifdef __cplusplus
}
#endif
#endif /* ID3V2_H */

