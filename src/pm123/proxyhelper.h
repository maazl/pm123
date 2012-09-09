/*
 * Copyright 2006-2011 Marcel Mueller
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

#ifndef PM123_PROXYHELPER_H
#define PM123_PROXYHELPER_H

#define INCL_PM
#include <config.h>
#include <decoder_plug.h>
#include <os2.h>


/****************************************************************************
*
*  Helper class for plug-in virtualization proxies and plug-in manager
*
****************************************************************************/
class ProxyHelper
{protected:
  /* Buffer size for compatibility interface */
  static const int BUFSIZE = 16384;

  /* thread priorities for decoder thread */
  static const ULONG DECODER_HIGH_PRIORITY_CLASS = PRTYC_TIMECRITICAL;
  static const ULONG DECODER_HIGH_PRIORITY_DELTA = 0;
  static const ULONG DECODER_LOW_PRIORITY_CLASS  = PRTYC_FOREGROUNDSERVER;
  static const ULONG DECODER_LOW_PRIORITY_DELTA  = 0;

 protected:
  /// Convert file URL
  /// - discard file: prefix,
  /// - replace '/' by '\'
  /// - replace "X|" by "X:"
  /// The string url is modified in place
  static const char* ConvertUrl2File(char* url);

  /// Convert time stamp in seconds to an integer in milliseconds.
  /// Truncate leading bits in case of an overflow.
  static int        TstmpF2I(PM123_TIME pos);
  /// Convert possibly truncated time stamp in milliseconds to seconds.
  /// The missing bits are taken from a context time stamp which has to be sufficiently close
  /// to the original time stamp. Sufficient is about 24 days.
  static PM123_TIME TstmpI2F(int pos, PM123_TIME context);

  /// Convert samples in 32 bit float format in the domain [-1,1] to 16 bit integers.
  static void       Float2Short(short* dp, const float* sp, size_t count);
  /// Convert samples from 16 bit audio to 32 bit float.
  static void       Short2Float(float* dp, const short* sp, size_t count);

  /// convert META_INFO into DECODER_INFO
  static void       ConvertMETA_INFO(DECODER_INFO* dinfo, const volatile META_INFO* meta);
  /// convert DECODER_INFO2 to DECODER_INFO
  static void       ConvertINFO_BUNDLE(DECODER_INFO* dinfo, const INFO_BUNDLE_CV* info);
  static void       ConvertDECODER_INFO(const INFO_BUNDLE* info, const DECODER_INFO* dinfo);

  /// Helper function to provide error callback for plug-in API.
  PROXYFUNCDEF void DLLENTRY PluginDisplayError(const char* msg);
  /// Helper function to provide info callback for plug-in API.
  PROXYFUNCDEF void DLLENTRY PluginDisplayInfo (const char* msg);

  PROXYFUNCDEF void DLLENTRY PluginControl(int index, void* param);
  PROXYFUNCDEF int  DLLENTRY PluginGetString(int index, int subindex, size_t bufsize, char* buf);

  // global services
 protected:
  enum
  { UM_CREATEPROXYWINDOW = WM_USER,
    UM_DESTROYPROXYWINDOW
  };
 private:
  static HWND ServiceHwnd;
  friend MRESULT EXPENTRY ProxyHelperWinFn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 protected:
  static HWND CreateProxyWindow(const char* cls, void* ptr);
  static void DestroyProxyWindow(HWND hwnd);
 public:
  static void Init();
  static void Uninit();
};


#endif
