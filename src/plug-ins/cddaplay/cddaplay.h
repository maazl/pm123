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

#ifndef PM123_CDDAPLAY_H
#define PM123_CDDAPLAY_H

#ifndef  RC_INVOKED
#include "format.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DLG_CDDA        400
#define ST_CDDBSERVERS  401
#define LB_CDDBSERVERS  402
#define PB_ADD1         405
#define PB_DELETE1      406
#define ST_HTTPSERVERS  403
#define LB_HTTPSERVERS  404
#define PB_ADD2         407
#define PB_DELETE2      408
#define PB_UPDATE       409
#define ST_NEWSERVER    410
#define EF_NEWSERVER    411
#define CB_USEHTTP      412
#define CB_TRYALL       415
#define ST_EMAIL        418
#define EF_EMAIL        419
#define CB_USECDDB      424

#define DLG_MATCH       500
#define LB_MATCHES      501

typedef struct _DECODER_STRUCT
{
  CDHANDLE* disc;
  CDINFO    info;
  char      drive[4];
  int       track;
  HEV       play;         // For internal use to sync the decoder thread.
  HEV       ok;           // For internal use to sync the decoder thread.
  HMTX      mutex;        // For internal use to sync the decoder thread.
  int       decodertid;   // Decoder thread indentifier.
  BOOL      stop;
  BOOL      frew;
  BOOL      ffwd;
  int       jumptopos;
  ULONG     status;
  HWND      hwnd;         // PM interface main frame window handle.

  FORMAT_INFO output_format;

  void (DLLENTRYP error_display)( char* );
  void (DLLENTRYP info_display )( char* );
  int  (DLLENTRYP output_play_samples)( void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );

  void* a;                // Only to be used with the precedent function.
  int   audio_buffersize;

} DECODER_STRUCT;

// used internally to manage CDDB and saved in cddaplay.ini file
typedef struct _DECODER_SETTINGS
{
  char* cddb_servers[128];
  char* http_servers[128];
  BOOL  cddb_selects[128];
  BOOL  http_selects[128];

  int   num_cddb;
  int   num_http;
  BOOL  use_cddb;
  BOOL  use_http;
  BOOL  try_all_servers;
  char  proxy_url[1024];
  char  email[1024];

} DECODER_SETTINGS;

#ifdef __cplusplus
}
#endif
#endif /* PM123_CDDAPLAY_H */

