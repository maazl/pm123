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

#define TAG_READ_ID3V2_AND_ID3V1  0
#define TAG_READ_ID3V1_AND_ID3V2  1
#define TAG_READ_ID3V2_ONLY       2
#define TAG_READ_ID3V1_ONLY       3

#define TAG_SAVE_ID3V2_AND_ID3V1  0
#define TAG_SAVE_ID3V2            1
#define TAG_SAVE_ID3V2_ONLY       2
#define TAG_SAVE_ID3V1            3
#define TAG_SAVE_ID3V1_ONLY       4

#define DLG_CONFIGURE     1
#define ID_NULL         900
#define CB_READ         910
#define CB_ID3V1_RDCH   920
#define CB_ID3V2_RDCH   930
#define CB_WRITE        940
#define CB_ID3V1_WRCH   950
#define CB_ID3V2_WRCH   960
#define CB_USEMMX       970
#define ST_CHARSET      990
#define CB_CHARSET      991
#define CB_AUTO_CHARSET 992

#define DLG_ID3TAG         2022
#define NB_ID3TAG           100

#define DLG_ID3V10         2012
#define ST_ID3_TITLE        106
#define EN_ID3_TITLE        103
#define ST_ID3_ARTIST       107
#define EN_ID3_ARTIST       104
#define ST_ID3_ALBUM        108
#define EN_ID3_ALBUM        105
#define ST_ID3_TRACK        115
#define EN_ID3_TRACK        114
#define ST_ID3_COMMENT      110
#define EN_ID3_COMMENT      109
#define ST_ID3_GENRE        111
#define CB_ID3_GENRE        101
#define ST_ID3_YEAR         113
#define EN_ID3_YEAR         112
#define PB_ID3_UNDO         200
#define PB_ID3_CLEAR        201

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
   long  resumepos;

   char* metadata_buff;
   int   metadata_size;

   void  DLLENTRYP(error_display)( const char* );
   void  DLLENTRYP(info_display )( const char* );
   int   DLLENTRYP(output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );

   void* a;               // Only to be used with the precedent function.
   int   audio_buffersize;

} DECODER_STRUCT;

