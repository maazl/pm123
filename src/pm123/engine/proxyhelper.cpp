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

#define  INCL_PM
#include "proxyhelper.h"
#include "../eventhandler.h"
#include "../copyright.h"
#include "../pm123.h" // for amp_player_hab
#include "../gui/gui.h" // for ConstructTagString
#include "../gui/skin.h" // for bmp_query_text
#include "controller.h"
#include "../core/playable.h"

#include <filter_plug.h>
#include <charset.h>
#include <math.h>
#include <limits.h>

#include <debuglog.h>


/****************************************************************************
*
* class ProxyHelper
*
****************************************************************************/

HWND ProxyHelper::ServiceHwnd = NULLHANDLE;

const char* ProxyHelper::ConvertUrl2File(char* url)
{
  if (strnicmp(url, "file:", 5) == 0)
  { url += 5;
    { char* cp = strchr(url, '/');
      while (cp)
      { *cp = '\\';
        cp = strchr(cp+1, '/');
      }
    }
    if (strncmp(url, "\\\\\\", 3) == 0)
    { url += 3;
      if (url[1] == '|')
        url[1] = ':';
    }
  }
  return url;
}

int ProxyHelper::TstmpF2I(double pos)
{ DEBUGLOG(("ProxyHelper::TstmpF2I(%g)\n", pos));
  // We cast to unsigned first to ensure that the range of the int is fully used.
  return pos >= 0 ? (int)(unsigned)(fmod(pos*1000., UINT_MAX+1.) + .5) : -1;
}

double ProxyHelper::TstmpI2F(int pos, double context)
{ DEBUGLOG(("ProxyHelper::TstmpI2F(%i, %g)\n", pos, context));
  if (pos < 0)
    return -1;
  double r = pos / 1000.;
  return r + (UINT_MAX+1.) * floor((context - r + UINT_MAX/2) / (UINT_MAX+1.));
}

void ProxyHelper::Float2Short(short* dp, const float* sp, size_t count)
{ const float* ep = sp + count;
  while (sp != ep)
  { register float f = *sp++ * -(float)SHRT_MIN;
    if (f > (float)SHRT_MAX)
      *dp = SHRT_MAX;
    else if (f < (float)SHRT_MIN)
      *dp = SHRT_MIN;
    else
      *dp = (short)ceil(f - .5);
    ++dp;
  }
}

void ProxyHelper::Short2Float(float* dp, const short* sp, size_t count)
{ const short* ep = sp + (count & ~7);
  while (sp != ep)
  { DO_8(i, dp[i] = sp[i] / -(float)SHRT_MIN);
    sp += 8;
    dp += 8;
  }
  ep = sp + (count & 7);
  while (sp != ep)
    *dp++ = *sp++ / -(float)SHRT_MIN;
}

void ProxyHelper::ConvertMETA_INFO(DECODER_INFO* dinfo, const volatile META_INFO* meta)
{
  if (!!meta->title)
    strlcpy(dinfo->title,    xstring(meta->title),    sizeof dinfo->title);
  if (!!meta->artist)
    strlcpy(dinfo->artist,   xstring(meta->artist),   sizeof dinfo->artist);
  if (!!meta->album)
    strlcpy(dinfo->album,    xstring(meta->album),    sizeof dinfo->album);
  if (!!meta->year)
    strlcpy(dinfo->year,     xstring(meta->year),     sizeof dinfo->year);
  if (!!meta->comment)
    strlcpy(dinfo->comment,  xstring(meta->comment),  sizeof dinfo->comment);
  if (!!meta->genre)
    strlcpy(dinfo->genre,    xstring(meta->genre),    sizeof dinfo->genre);
  if (!!meta->track)
    strlcpy(dinfo->track,    xstring(meta->track),    sizeof dinfo->track);
  if (!!meta->copyright)
    strlcpy(dinfo->copyright,xstring(meta->copyright),sizeof dinfo->copyright);

  dinfo->codepage = ch_default();

  if (meta->track_gain > -1000)
    dinfo->track_gain = meta->track_gain;
  if (meta->track_peak > -1000)
    dinfo->track_peak = meta->track_peak;
  if (meta->album_gain > -1000)
    dinfo->album_gain = meta->album_gain;
  if (meta->album_peak > -1000)
    dinfo->album_peak = meta->album_peak;
}

void ProxyHelper::ConvertINFO_BUNDLE(DECODER_INFO* dinfo, const INFO_BUNDLE_CV* info)
{
  dinfo->format.samplerate = info->tech->samplerate;
  dinfo->format.channels   = info->tech->channels;
  dinfo->format.bits       = 16;
  dinfo->format.format     = WAVE_FORMAT_PCM;

  dinfo->songlength = info->obj->songlength < 0 ? -1 : (int)(info->obj->songlength * 1000.);
  dinfo->junklength = -1;
  dinfo->bitrate    = info->obj->bitrate    < 0 ? -1 : info->obj->bitrate / 1000;
  if (!!info->tech->info)
    strlcpy(dinfo->tech_info, xstring(info->tech->info), sizeof dinfo->tech_info);

  ConvertMETA_INFO(dinfo, info->meta);

  dinfo->saveinfo   = (info->tech->attributes & TATTR_WRITABLE) != 0;
  dinfo->filesize   = (int)info->phys->filesize;
}

void ProxyHelper::ConvertDECODER_INFO(const INFO_BUNDLE* info, const DECODER_INFO* dinfo)
{ if (info->phys->filesize < 0)
    info->phys->filesize = dinfo->filesize;
  info->tech->samplerate = dinfo->format.samplerate;
  info->tech->channels   = dinfo->format.channels;
  info->tech->attributes = TATTR_SONG | TATTR_STORABLE | TATTR_WRITABLE * (dinfo->saveinfo != 0);
  if (*dinfo->tech_info)
    info->tech->info     = dinfo->tech_info;
  info->obj->songlength  = dinfo->songlength < 0 ? -1 : dinfo->songlength / 1000.;
  info->obj->bitrate     = dinfo->bitrate    < 0 ? -1 : dinfo->bitrate * 1000;
  if (*dinfo->title)
    info->meta->title    = dinfo->title;
  if (*dinfo->artist)
    info->meta->artist   = dinfo->artist;
  if (*dinfo->album)
    info->meta->album    = dinfo->album;
  if (*dinfo->year)
    info->meta->year     = dinfo->year;
  if (*dinfo->comment)
    info->meta->comment  = dinfo->comment;
  if (*dinfo->genre)
    info->meta->genre    = dinfo->genre;
  if (*dinfo->track)
    info->meta->track    = dinfo->track;
  if (*dinfo->copyright)
    info->meta->copyright= dinfo->copyright;
  // Mask out zero values because they mean most likely 'undefined'.
  if (dinfo->track_gain != 0.)
    info->meta->track_gain = dinfo->track_gain;
  if (dinfo->track_peak != 0.)
    info->meta->track_peak = dinfo->track_peak;
  if (dinfo->album_gain != 0.)
    info->meta->album_gain = dinfo->album_gain;
  if (dinfo->album_peak != 0.)
    info->meta->album_peak = dinfo->album_peak;
}

PROXYFUNCIMP(void DLLENTRY, ProxyHelper) PluginDisplayError(const char* msg)
{ EventHandler::Post(MSG_ERROR, msg);
}

PROXYFUNCIMP(void DLLENTRY, ProxyHelper) PluginDisplayInfo(const char* msg)
{ EventHandler::Post(MSG_WARNING, msg);
}

PROXYFUNCIMP(void DLLENTRY, ProxyHelper) PluginControl(int index, void* param)
{
  /* TODO: pm123_control
  switch (index)
  {
    case CONTROL_NEXTMODE:
      WinSendMsg( amp_player_window(), AMP_DISPLAY_MODE, 0, 0 );
      break;
  }
  */
}

PROXYFUNCIMP(int DLLENTRY, ProxyHelper) PluginGetString(int index, int subindex, size_t bufsize, char* buf)
{ if (bufsize)
    *buf = 0;
  switch (index)
  {case STR_VERSION:
    strlcpy(buf, AMP_FULLNAME, bufsize);
    break;

   case STR_DISPLAY_TEXT:
    strlcpy(buf, bmp_query_text(), bufsize);
    break;

   case STR_FILENAME:
    { int_ptr<APlayable> song = Ctrl::GetCurrentSong();
      if (song)
        strlcpy(buf, song->GetPlayable().URL, bufsize);
      break;
    }

   case STR_DISPLAY_TAG:
    { int_ptr<APlayable> song = Ctrl::GetCurrentSong();
      if (song)
      { const xstring& text = GUI::ConstructTagString(*song->GetInfo().meta);
        strlcpy(buf, text, bufsize);
      }
      break;
    }

   case STR_DISPLAY_INFO:
    { int_ptr<APlayable> song = Ctrl::GetCurrentSong();
      if (song)
        strlcpy(buf, xstring(song->GetInfo().tech->info), bufsize);
      break;
    }

   default: break;
  }
 return(0);
}

MRESULT EXPENTRY ProxyHelperWinFn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG(("ProxyHelperWinFn(%p, %u, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch (msg)
  {case ProxyHelper::UM_CREATEPROXYWINDOW:
    { HWND proxyhwnd = WinCreateWindow(hwnd, (PSZ)PVOIDFROMMP(mp1), "", 0, 0,0, 0,0, NULLHANDLE, HWND_BOTTOM, 42, NULL, NULL);
      PMASSERT(proxyhwnd != NULLHANDLE);
      PMRASSERT(WinSetWindowPtr(proxyhwnd, QWL_USER, PVOIDFROMMP(mp2)));
      return (MRESULT)proxyhwnd;
    }
   case ProxyHelper::UM_DESTROYPROXYWINDOW:
    WinDestroyWindow(HWNDFROMMP(mp1));
    return 0;
   case WM_DESTROY:
     ProxyHelper::ServiceHwnd = NULLHANDLE;
    break;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

HWND ProxyHelper::CreateProxyWindow(const char* cls, void* ptr)
{ return (HWND)WinSendMsg(ServiceHwnd, UM_CREATEPROXYWINDOW, MPFROMP(cls), MPFROMP(ptr));
}

void ProxyHelper::DestroyProxyWindow(HWND hwnd)
{ WinSendMsg(ServiceHwnd, UM_DESTROYPROXYWINDOW, MPFROMHWND(hwnd), 0);
}

void ProxyHelper::Init()
{ PMRASSERT(WinRegisterClass(amp_player_hab, "PM123_ProxyHelper", &ProxyHelperWinFn, 0, 0));
  ServiceHwnd = WinCreateWindow(HWND_OBJECT, "PM123_ProxyHelper", NULL, 0, 0,0, 0,0, NULLHANDLE, HWND_BOTTOM, 42, NULL, NULL);
  PMASSERT(ServiceHwnd != NULLHANDLE);
}

void ProxyHelper::Uninit()
{ WinDestroyWindow(ServiceHwnd);
}
