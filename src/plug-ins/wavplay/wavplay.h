/*
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef PM123_WAVPLAY_H
#define PM123_WAVPLAY_H

#define PLUGIN_INTERFACE_LEVEL 3

#define INCL_BASE
#include <decoder_plug.h>
#include <sndfile.h>
#include <format.h>
#include <xio.h>
//#include <cpp/mutex.h>
#include <os2.h>

#include <stdlib.h>

struct DECODER_STRUCT
{
  XFILE*       XFile;
  xstring      URL;
  SNDFILE*     Sndfile;

  SF_INFO      SFInfo;
  FORMAT_INFO2 Format;

  HEV          play;            // For internal use to sync the decoder thread.
  HMTX         mutex;           // For internal use to sync the decoder thread.
  int          decodertid;      // Decoder thread indentifier.
  bool         Stop;
  DECFASTMODE  Fast;
  PM123_TIME   JumpToPos;
  DECODERSTATE status;

  /** specify a function which the decoder should use for output */
  int   DLLENTRYP(OutRequestBuffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void  DLLENTRYP(OutCommitBuffer )(void* a, int len, PM123_TIME posmarker);
  /** decoder events */
  void  DLLENTRYP(DecEvent        )(void* a, DECEVENTTYPE event, void* param);
  void* A;                  /**< only to be used with the precedent functions */
};

#endif /* PM123_WAVPLAY_H */

