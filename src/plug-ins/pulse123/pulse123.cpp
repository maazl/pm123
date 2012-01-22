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

#include "pulse123.h"
#include "playbackworker.h"
#include <plugin.h>
//#include <os2.h>

#include <debuglog.h>

PLUGIN_CONTEXT Ctx;


/// Returns information about plug-in.
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* query)
{ query->type         = PLUGIN_OUTPUT;
  query->author       = "Marcel Mueller";
  query->desc         = "Pulseaudio for PM123 V0.9";
  query->configurable = FALSE;
  query->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/// init plug-in
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ Ctx = *ctx;
  return 0;
}


/// Initialize the output plug-in.
ULONG DLLENTRY output_init(void** A)
{ PlaybackWorker* w = new PlaybackWorker();
  ULONG ret = w->Init();
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


/* Default dialog procedure for the directorys browse dialog.
MRESULT EXPENTRY
cfg_file_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  FILEDLG* filedialog =
    (FILEDLG*)WinQueryWindowULong( hwnd, QWL_USER );

  switch( msg )
  {
    case WM_INITDLG:
      WinEnableControl( hwnd, DID_OK, TRUE  );
      do_warpsans( hwnd );
      break;

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == DID_FILENAME_ED && SHORT2FROMMP(mp1) == EN_CHANGE ) {
        // Prevents DID_OK from being greyed out.
        return 0;
      }
      break;

    case WM_COMMAND:
      if( SHORT1FROMMP(mp1) == DID_OK )
      {
        if( !is_root( filedialog->szFullFile )) {
          filedialog->szFullFile[strlen(filedialog->szFullFile)-1] = 0;
        }

        filedialog->lReturn    = DID_OK;
        filedialog->ulFQFCount = 1;

        WinDismissDlg( hwnd, DID_OK );
        return 0;
      }
      break;
  }
  return WinDefFileDlgProc( hwnd, msg, mp1, mp2 );
}

/ Processes messages of the configuration dialog. *
MRESULT EXPENTRY cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  static HMODULE module;

  switch( msg ) {
    case WM_INITDLG:
      module = (HMODULE)mp2;
      WinSetDlgItemText( hwnd, EF_FILENAME, outpath );
      do_warpsans( hwnd );
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 )) {
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

          WinQueryDlgItemText( hwnd, EF_FILENAME,
                               sizeof( filedialog.szFullFile ), filedialog.szFullFile );

          if( *filedialog.szFullFile &&
               filedialog.szFullFile[ strlen( filedialog.szFullFile ) - 1 ] != '\\' )
          {
            strcat( filedialog.szFullFile, "\\" );
          }

          WinFileDlg( HWND_DESKTOP, hwnd, &filedialog );

          if( filedialog.lReturn == DID_OK ) {
            WinSetDlgItemText( hwnd, EF_FILENAME, filedialog.szFullFile );
          }
          return 0;
        }

        case DID_OK:
        {
          WinQueryDlgItemText( hwnd, EF_FILENAME, sizeof( outpath ), outpath );

          if( *outpath && outpath[ strlen( outpath ) - 1 ] == '\\' && !is_root( outpath )) {
            outpath[ strlen( outpath ) - 1 ] = 0;
          }

          context->plugin_api->profile_write( "outpath", outpath, strlen(outpath) );
          break;
        }
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

* Configure plug-in. *
int DLLENTRY
plugin_configure( HWND hwnd, HMODULE module ) {
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, (PVOID)module );
  return 0;
}*/


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
