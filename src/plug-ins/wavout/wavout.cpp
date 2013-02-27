/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *           2013      Marcel Mueller
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
 *
 * PM123 WAVE Output plug-in.
 */

#define INCL_PM
#define INCL_DOS
#define PLUGIN_INTERFACE_LEVEL 3

#include "wavout.h"
#include "writer.h"
#include <utilfct.h>
#include <fileutil.h>
#include <cpp/pmutils.h>
#include <plugin.h>
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <debuglog.h>


PLUGIN_CONTEXT Ctx;


/* Default dialog procedure for the directorys browse dialog. */
MRESULT EXPENTRY cfg_file_dlg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  FILEDLG* filedialog = (FILEDLG*)WinQueryWindowULong(hwnd, QWL_USER);

  switch (msg)
  {
    case WM_INITDLG:
      WinEnableControl(hwnd, DID_OK, TRUE);
      do_warpsans(hwnd);
      break;

    case WM_CONTROL:
      if (SHORT1FROMMP(mp1) == DID_FILENAME_ED && SHORT2FROMMP(mp1) == EN_CHANGE)
        // Prevents DID_OK from being greyed out.
        return 0;
      break;

    case WM_COMMAND:
      if (SHORT1FROMMP(mp1) == DID_OK)
      {
        if (!is_root(filedialog->szFullFile))
          filedialog->szFullFile[strlen(filedialog->szFullFile)-1] = 0;

        filedialog->lReturn    = DID_OK;
        filedialog->ulFQFCount = 1;

        WinDismissDlg(hwnd, DID_OK);
        return 0;
      }
      break;

    case WM_ADJUSTWINDOWPOS:
      { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
        if (pswp->fl & SWP_SIZE)
          dlg_adjust_resize(hwnd, pswp);
      }
      break;

    case WM_WINDOWPOSCHANGED:
      { PSWP pswp = (PSWP)PVOIDFROMMP(mp1);
        if (pswp->fl & SWP_SIZE)
          dlg_do_resize(hwnd, pswp, pswp+1);
      }
      break;
  }
  return WinDefFileDlgProc(hwnd, msg, mp1, mp2);
}

/* Processes messages of the configuration dialog. */
MRESULT EXPENTRY cfg_dlg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  static HMODULE module;

  switch (msg)
  { case WM_INITDLG:
      module = *(HMODULE*)mp2;
      WinSetDlgItemText(hwnd, EF_FILENAME, xstring(WAVOUT::OutPath));
      WinCheckButton(hwnd, RB_SOURCEFILE + WAVOUT::UseTimestamp, TRUE);
      do_warpsans(hwnd);
      break;

    case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
        case PB_BROWSE:
        {
          FILEDLG filedialog;

          memset( &filedialog, 0, sizeof( FILEDLG ));
          filedialog.cbSize     = sizeof( FILEDLG );
          filedialog.fl         = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
          filedialog.pszTitle   = "Output directory";
          filedialog.hMod       = module;
          filedialog.usDlgId    = DLG_BROWSE;
          filedialog.pfnDlgProc = cfg_file_dlg_proc;

          WinQueryDlgItemText(hwnd, EF_FILENAME, sizeof filedialog.szFullFile, filedialog.szFullFile);

          if (*filedialog.szFullFile &&
               filedialog.szFullFile[strlen(filedialog.szFullFile) - 1] != '\\')
            strcat(filedialog.szFullFile, "\\");

          WinFileDlg(HWND_DESKTOP, hwnd, &filedialog);

          if (filedialog.lReturn == DID_OK)
            WinSetDlgItemText(hwnd, EF_FILENAME, filedialog.szFullFile);
          return 0;
        }

        case DID_OK:
        {
          const xstring& outpath = WinQueryDlgItemXText(hwnd, EF_FILENAME);
          WAVOUT::OutPath = outpath;
          WAVOUT::UseTimestamp = (bool)WinQueryButtonCheckstate(hwnd, RB_TIMESTAMP);
          Ctx.plugin_api->profile_write("outpath", outpath.cdata(), outpath.length());
          Ctx.plugin_api->profile_write("usetimestamp", &WAVOUT::UseTimestamp, 1);
          break;
        }
      }
      break;

    case WM_ADJUSTWINDOWPOS:
      { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
        if (pswp->fl & SWP_SIZE)
          dlg_adjust_resize(hwnd, pswp);
      }
      break;

    case WM_WINDOWPOSCHANGED:
      { PSWP pswp = (PSWP)PVOIDFROMMP(mp1);
        if (pswp->fl & SWP_SIZE)
          dlg_do_resize(hwnd, pswp, pswp+1);
      }
      break;
  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

/* Configure plug-in. */
HWND DLLENTRY plugin_configure(HWND hwnd, HMODULE module)
{ WinDlgBox(HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, &module);
  return NULLHANDLE;
}

/* Processing of a command messages. */
ULONG DLLENTRY output_command(WAVOUT* a, ULONG msg, const OUTPUT_PARAMS2* info)
{ DEBUGLOG(("wavout:output_command(%p, %d, %p)\n", a, msg, info));

  switch (msg)
  {case OUTPUT_OPEN:
    a->OutOpen(info->URL, info->PlayingPos);
    break;

   case OUTPUT_CLOSE:
    a->OutClose();
    break;

   case OUTPUT_VOLUME:
    break;

   case OUTPUT_PAUSE:
    a->OutPause(!!info->Pause);
    break;

   case OUTPUT_SETUP:
    a->OutSetup(info->OutEvent, info->W);
    break;

   case OUTPUT_TRASH_BUFFERS:
    a->OutTrash();
  }
  return 0;
}

/* This function is called by the decoder or last in chain
   filter plug-in to play samples. */
int DLLENTRY output_request_buffer(WAVOUT* a, FORMAT_INFO2* format, float** buf)
{ return a->RequestBuffer(format, buf);
}

void DLLENTRY output_commit_buffer(WAVOUT* a, int len, PM123_TIME pos)
{ a->CommitBuffer(len, pos);
}

/* This function is used by visual plug-ins so the user can
   visualize what is currently being played. */
ULONG DLLENTRY output_playing_samples(WAVOUT* a, PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param)
{ return a->PlayingSamples(offset, cb, param);
}

/* Returns the playing position. */
PM123_TIME DLLENTRY output_playing_pos(WAVOUT* a)
{ return a->OutPlayingPos();
}

/* Returns TRUE if the output plug-in still has some buffers to play. */
BOOL DLLENTRY output_playing_data(WAVOUT* a)
{ return FALSE;
}

/* Initialize the output plug-in. */
ULONG DLLENTRY output_init(WAVOUT** A)
{ DEBUGLOG(("wavout:output_init(%p)\n", A));
  *A = new WAVOUT();
  return PLUGIN_OK;
}

/* Uninitialize the output plug-in. */
ULONG DLLENTRY output_uninit(WAVOUT* a)
{ DEBUGLOG(("wavout:output_uninit(%p)\n", a));
  delete a;
  return PLUGIN_OK;
}

/* Returns information about plug-in. */
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* query)
{
  query->type         = PLUGIN_OUTPUT;
  query->author       = "Samuel Audet, Dmitry A.Steklenev, M.Mueller";
  query->desc         = "WAVE Output 1.40";
  query->configurable = TRUE;
  query->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/* init plug-in */
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{
  Ctx = *ctx;

  int len = Ctx.plugin_api->profile_query("outpath", NULL, 0);
  if (len >= 0)
  { xstring outpath;
    Ctx.plugin_api->profile_query("outpath", outpath.allocate(len), len);
    WAVOUT::OutPath = outpath;
  } else
    WAVOUT::OutPath = xstring::empty;
  Ctx.plugin_api->profile_query("usetimestamp", &WAVOUT::UseTimestamp, sizeof WAVOUT::UseTimestamp);

  return 0;
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
