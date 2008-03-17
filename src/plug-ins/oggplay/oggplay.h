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

#ifndef PM123_OGGPLAY_H
#define PM123_OGGPLAY_H

#include <format.h>
#include <xio.h>
#include <ogg/ogg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DECODER_STRUCT
{
   XFILE* file;
   char   filename[_MAX_PATH];

   OggVorbis_File  vrbfile;
   vorbis_info*    vrbinfo;
   vorbis_comment* comment;

   FORMAT_INFO  output_format;
   ogg_int64_t  played_pcms;
   ogg_int64_t  resume_pcms;
   double       played_secs;

   HEV    play;       // For internal use to sync the decoder thread.
   HMTX   mutex;      // For internal use to sync the decoder thread.
   int    decodertid; // Decoder thread indentifier.
   BOOL   stop;
   BOOL   frew;
   BOOL   ffwd;
   int    jumptopos;
   ULONG  status;
   HWND   hwnd;       // PM interface main frame window handle.
   ULONG  songlength;
   int    bitrate;

   char*  metadata_buff;
   int    metadata_size;

   void DLLENTRYP(error_display)( const char* );
   void DLLENTRYP(info_display )( const char* );
   int  DLLENTRYP(output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );

   void* a;           // Only to be used with the precedent function.
   int   audio_buffersize;

} DECODER_STRUCT;

#define MAXRESYNC 15

#ifdef __cplusplus
}
#endif
#endif /* PM123_OGGPLAY_H */

