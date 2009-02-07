/*
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef  RC_INVOKED

#define  INCL_PM
#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include <utilfct.h>
#include <decoder_plug.h>
#include <xio.h>
#include <dxhead.h>

#if defined( OS2 )

  #ifndef M_PI
  #define M_PI    3.14159265358979323846
  #endif
  #ifndef M_SQRT2
  #define M_SQRT2 1.41421356237309504880
  #endif
  #define REAL_IS_FLOAT

#endif

#include <plugin.h>
#include "common.h"

#endif /* RC_INVOKED */

#define ID_NULL             10
#define PB_UNDO             20
#define PB_CLEAR            21
#define PB_HELP             22

#define DLG_CONFIGURE     1070 /* Do not change -> Help */
#define CB_1R_READ         111
#define RB_1R_PREFER       112
#define CB_1R_AUTOENCODING 115
#define CO_1_ENCODING      116
#define RB_1W_UNCHANGED    120
#define RB_1W_ALWAYS       121
#define RB_1W_DELETE       122
#define RB_1W_NOID3V2      123
#define CB_2R_READ         131
#define RB_2R_PREFER       132
#define CB_2R_OVERRIDEENC  135
#define CO_2R_ENCODING     136
#define RB_2W_UNCHANGED    140
#define RB_2W_ALWAYS       141
#define RB_2W_DELETE       142
#define RB_2W_ONDEMAND     143
#define RB_2W_ONDEMANDSPC  144
#define CO_2W_ENCODING     145

#define DLG_ID3TAG        1028 /* Do not change -> Help */
#define NB_ID3TAG          210

#define DLG_ID3ALL         300
#define DLG_ID3V1          301
#define DLG_ID3V2          302
#define EN_TITLE           310
#define EN_ARTIST          311
#define EN_ALBUM           312
#define EN_TRACK           313
#define EN_DATE            314
#define CO_GENRE           315
#define EN_COMMENT         316
#define EN_COPYRIGHT       317
#define CO_ENCODING        320
#define PB_COPY            322
#define RB_UPDATE          330
#define RB_UNCHANGED       331
#define RB_DELETE          332
#define CB_AUTOWRITE       334
#define RB_UPDATE2         335
#define RB_UNCHANGED2      336
#define RB_CLEAN2          337
#define RB_DELETE2         338
#define CB_AUTOWRITE2      339


typedef struct _DECODER_STRUCT
{
   MPG_FILE mpeg;
   char     filename[_MAX_PATH];
   XFILE*   save;
   char     savename[_MAX_PATH];

   FORMAT_INFO output_format;

   HEV   play;            // For internal use to sync the decoder thread.
   HMTX  mutex;           // For internal use to sync the decoder thread.
   int   decodertid;      // Decoder thread indentifier.
   BOOL  stop;
   BOOL  frew;
   BOOL  ffwd;
   int   jumptopos;
   ULONG status;
   HWND  hwnd;            // PM interface main frame window handle.
   int   bitrate;         // For VBR events.
   int   posmarker;

   char* metadata_buff;
   int   metadata_size;

   void  DLLENTRYP(error_display)( const char* );
   void  DLLENTRYP(info_display )( const char* );
   int   DLLENTRYP(output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );

   void* a;               // Only to be used with the precedent function.
   int   audio_buffersize;

} DECODER_STRUCT;

/* Open MPEG file. Returns 0 if it successfully opens the file.
   A nonzero return value indicates an error. A -1 return value
   indicates an unsupported format of the file. */
int  plg_open_file  ( DECODER_STRUCT* w, const char* filename, const char* mode );
/* Closes the MPEG file. */
void plg_close_file ( DECODER_STRUCT* w );

/* Update the tags of the open file to tagv1 and tagv2.
   If either tagv1 or tagv2 is NULL the corresponding tag is untouched.
   If you pass PLG_DELETE_ID3Vx the tag is explicitely removed.
   The function returns 0 on succes orr errno on error. Additionally the
   pointer errmsg is set to an error message (static storage) if not NULL.
   When the operation cannot work in place, the function returns a temporary
   filename in savename. This temporary file contains a copy of the file with
   the new tag information. You must replace the existing file by this one
   on your own. If the operation completed in place *savename is empty. */ 
#define PLG_DELETE_ID3V1 ((ID3V1_TAG*)-1L)
#define PLG_DELETE_ID3V2 ((ID3V2_TAG*)-1L)
int plg_update_tags( DECODER_STRUCT* w, ID3V1_TAG* tagv1, ID3V2_TAG* tagv2,
                     char (*savename)[_MAX_PATH], const char** errmsg );
/* Replace file dstfile by srcfile even if dstfile is currently in use by
   an active decoder instance. The replacement will also reposition the file pointer
   of the active decoder instances if the header length changed.
   The function returns 0 on succes orr errno on error. Additionally the
   pointer errmsg is set to an error message (static storage) if not NULL. */ 
int plg_replace_file( const char* srcfile, const char* dstfile, const char** errmsg );


/* Auto detect codepage of an ID3V1 tag */
ULONG detect_id3v1_codepage( const ID3V1_TAG* tag, ULONG codepage );
/* Check whether a string entirely contains ASCII characters in the range [32,126]. */
BOOL ascii_check(const char* str);

void save_ini( void );


// Settings
typedef enum
{ TAG_READ_ID3V2_AND_ID3V1,
  TAG_READ_ID3V1_AND_ID3V2,
  TAG_READ_ID3V2_ONLY,
  TAG_READ_ID3V1_ONLY,
  TAG_READ_NONE
} read_type;

typedef enum
{ TAG_SAVE_ID3V1_UNCHANGED,
  TAG_SAVE_ID3V1_WRITE,
  TAG_SAVE_ID3V1_DELETE,
  TAG_SAVE_ID3V1_NOID3V2
} save_id3v1_type;

typedef enum
{ TAG_SAVE_ID3V2_UNCHANGED,
  TAG_SAVE_ID3V2_WRITE,
  TAG_SAVE_ID3V2_DELETE,
  TAG_SAVE_ID3V2_ONDEMAND,
  TAG_SAVE_ID3V2_ONDEMANDSPC
} save_id3v2_type;

extern read_type       tag_read_type;
extern ULONG           tag_id3v1_charset;
extern BOOL            tag_read_id3v1_autoch;
extern save_id3v1_type tag_save_id3v1_type;
extern save_id3v2_type tag_save_id3v2_type;
extern ULONG           tag_read_id3v2_charset;
extern uint8_t         tag_save_id3v2_encoding;

