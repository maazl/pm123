/*
 * Copyright 2010-2011 Marcel Mueller
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

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_PM

#include "pulse123.h"
#include "playbackworker.h"
#include <plugin.h>
#include <utilfct.h>
#include <cpp/pmutils.h>
#include <os2.h>

#include <debuglog.h>

PLUGIN_CONTEXT Ctx;

// Configuration
xstring PlaybackServer;

inline xstring ini_query(const char* key)
{ int len = (*Ctx.plugin_api->profile_query)(key, NULL, 0);
  xstring ret;
  if (len >= 0)
  { char* cp = ret.allocate(len);
    (*Ctx.plugin_api->profile_query)(key, cp, len);
  }
  return ret;
}

#define ini_load(var) var = ini_query(#var)

inline void ini_write(const char* key, const char* value)
{ int len = value ? strlen(value) : 0;
  (*Ctx.plugin_api->profile_write)(key, value, len);
}

#define ini_save(var) ini_write(#var, var)


/// Returns information about plug-in.
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* query)
{ query->type         = PLUGIN_OUTPUT;
  query->author       = "Marcel Mueller";
  query->desc         = "PulseAudio for PM123 V1.0";
  query->configurable = TRUE;
  query->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/// init plug-in
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ Ctx = *ctx;

  ini_load(PlaybackServer);

  return 0;
}


/// Initialize the output plug-in.
ULONG DLLENTRY output_init(void** A)
{ PlaybackWorker* w = new PlaybackWorker();
  ULONG ret = w->Init(PlaybackServer);
  if (ret == 0)
    *A = w;
  return ret;
}

/// Uninitialize the output plug-in.
ULONG DLLENTRY output_uninit(void* A)
{ delete (PlaybackWorker*)A;
  return PLUGIN_OK;
}

/// Processing of a command messages.
ULONG DLLENTRY output_command(void* A, ULONG msg, OUTPUT_PARAMS2* info)
{ DEBUGLOG(("pulse123:output_command(%p, %d, %p)\n", A, msg, info));
  PlaybackWorker* a = (PlaybackWorker*)A;

  switch (msg)
  { case OUTPUT_OPEN:
      return a->Open(info->URL, info->Info, info->PlayingPos, info->OutEvent, info->W);

    case OUTPUT_CLOSE:
      return a->Close();

    case OUTPUT_VOLUME:
      return a->SetVolume(info->Volume);

    case OUTPUT_PAUSE:
      return a->SetPause(info->Pause);

    case OUTPUT_SETUP:
      return 0;

    case OUTPUT_TRASH_BUFFERS:
      return a->TrashBuffers(info->PlayingPos);
  }
  return PLUGIN_ERROR;
}

int DLLENTRY output_request_buffer(void* a, const FORMAT_INFO2* format, float** buf)
{ return ((PlaybackWorker*)a)->RequestBuffer(format, buf);
}

void DLLENTRY output_commit_buffer(void* a, int len, PM123_TIME posmarker)
{ return ((PlaybackWorker*)a)->CommitBuffer(len, posmarker);
}

ULONG DLLENTRY output_playing_samples(void* a, PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param)
{ return ((PlaybackWorker*)a)->GetCurrentSamples(offset, cb, param);
}

/// Returns the playing position.
PM123_TIME DLLENTRY output_playing_pos(void* A)
{ return ((PlaybackWorker*)A)->GetPosition();
}

/// Returns TRUE if the output plug-in still has some buffers to play.
BOOL DLLENTRY output_playing_data(void* A)
{ return ((PlaybackWorker*)A)->IsPlaying();
}


/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY cfg_dlg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {
   case WM_INITDLG:
    do_warpsans(hwnd);
    { // Populate MRU list
      HWND ctrl = WinWindowFromID(hwnd, CB_PBSERVER);
      char key[] = "PlaybackServer#";
      for (key[sizeof key -2] = '0'; key[sizeof key -2] <= '9'; ++key[sizeof key -2])
      { xstring url = ini_query(key);
        if (!url)
          break;
        WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(url.cdata()));
      }
      // Set current value
      if (PlaybackServer)
        PMRASSERT(WinSetWindowText(ctrl, PlaybackServer.cdata()));
    }
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case DID_OK:
      { HWND ctrl = WinWindowFromID(hwnd, CB_PBSERVER);
        PlaybackServer = WinQueryWindowXText(ctrl);
        ini_save(PlaybackServer);
        // update MRU list
        if (PlaybackServer && *PlaybackServer)
        { char key[] = "PlaybackServer0";
          ini_write(key, PlaybackServer);
          int i = 0;
          int len = 0;
          xstring url;
          while (++key[sizeof key -2] <= '9')
          { if (len >= 0)
            {skip:
              url.reset();
              len = (SHORT)SHORT1FROMMR(WinSendMsg(ctrl, LM_QUERYITEMTEXTLENGTH, MPFROMSHORT(i), 0));
              if (len >= 0)
              { WinSendMsg(ctrl, LM_QUERYITEMTEXT, MPFROM2SHORT(i, len+1), MPFROMP(url.allocate(len)));
                DEBUGLOG(("pulse123:cfg_dlg_proc save MRU %i: (%i) %s\n", i, len, url.cdata()));
                ++i;
                if (url == PlaybackServer)
                  goto skip;
              }
            }
            ini_write(key, url);
          }
        }
      }
      break;
    }
    break;
  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

/* Configure plug-in. */
void DLLENTRY plugin_configure(HWND hwnd, HMODULE module)
{
  WinDlgBox(HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIG, (PVOID)module);
}


#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return 1UL;
  } else {
    _dump_allocated( 0 );
    _CRT_term();
    return 1UL;
  }
}
#endif
