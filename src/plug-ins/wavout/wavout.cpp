/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

#define  INCL_PM
#define  INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <utilfct.h>
#include <format.h>
#include <output_plug.h>
#include <decoder_plug.h>
#include <plugin.h>
#include "wavout.h"

#include <debuglog.h>

static char outpath[CCHMAXPATH];

/* Default dialog procedure for the directorys browse dialog. */
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

/* Processes messages of the configuration dialog. */
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
          HINI hini;
          WinQueryDlgItemText( hwnd, EF_FILENAME, sizeof( outpath ), outpath );

          if( *outpath && outpath[ strlen( outpath ) - 1 ] == '\\' && !is_root( outpath )) {
            outpath[ strlen( outpath ) - 1 ] = 0;
          }

          if(( hini = open_module_ini()) != NULLHANDLE ) {
            save_ini_string( hini, outpath );
            close_ini( hini );
          }
          break;
        }
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
int PM123_ENTRY
plugin_configure( HWND hwnd, HMODULE module )
{
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, (PVOID)module );
  return 0;
}

/* Closes a output file. */
static ULONG
output_close( void* A )
{
  WAVOUT* a = (WAVOUT*)A;
  ULONG resetcount;

  if( a->opened ) {
    a->wavfile.close();
    DosResetEventSem( a->pause, &resetcount );
    free( a->buffer );
    a->buffer = NULL;
    a->opened = FALSE;
  }
  return 0;
}

/* Opens a output file. */
static ULONG
output_open( WAVOUT* a )
{
  int rc;

  // New filename, even if we didn't explicity get a close!
  if( a->opened ) {
    output_close( a );
  }

  a->playingpos = 0;
  free( a->buffer );
  a->buffer = (char*)malloc( a->original_info.buffersize );

  DosPostEventSem( a->pause );
  strcpy( a->fullpath, outpath );

  if( *a->fullpath && !is_root( a->fullpath )) {
    strcat( a->fullpath, "\\" );
    strcat( a->fullpath, a->filename );
  } else {
    strcat( a->fullpath, a->filename );
  }

  rc = a->wavfile.open( a->fullpath, CREATE,
                        a->original_info.formatinfo.samplerate,
                        a->original_info.formatinfo.channels,
                        a->original_info.formatinfo.bits,
                        a->original_info.formatinfo.format );
                        
  DEBUGLOG(("wavout:output_open: %s, %d\n", a->fullpath, rc));                       
  if( rc != 0 )
  {
    char message[1024];
    sprintf( message, "Could not open WAV file:\n%s\n%s", a->fullpath, clib_strerror( errno ));
    (*a->original_info.error_display)( message );
    return errno;
  }

  a->opened = TRUE;
  return 0;
}

/* Pauses a output file. */
static ULONG
output_pause( void* A, BOOL pause )
{
  WAVOUT* a = (WAVOUT*)A;
  ULONG   resetcount;

  if( pause ) {
    return DosResetEventSem( a->pause, &resetcount );
  } else {
    return DosPostEventSem( a->pause );
  }
}

/* Processing of a command messages. */
ULONG PM123_ENTRY
output_command( void* A, ULONG msg, OUTPUT_PARAMS* info )
{
  WAVOUT* a  = (WAVOUT*)A;
  ULONG   rc = 0;
  DEBUGLOG(("wavout:output_command(%p, %d, %p)\n", A, msg, info));

  switch( msg ) {
    case OUTPUT_OPEN:
    {
      sfname( a->filename, info->filename, sizeof( a->filename ));
      if( !*a->filename ) {
        strcpy( a->filename, "wavout" );
      }
      if( is_url( info->filename )) {
        sdecode( a->filename, a->filename, sizeof( a->filename ));
      }
      strlcat( a->filename, ".wav", sizeof( a->filename ));
      rc = output_open( a );
      break;
    }

    case OUTPUT_CLOSE:
      rc = output_close( a );
      break;

    case OUTPUT_VOLUME:
      break;

    case OUTPUT_PAUSE:
      rc = output_pause( a, info->pause );
      break;

    case OUTPUT_SETUP:
      DEBUGLOG(("wavout:output_command:OUTPUT_SETUP: {%d %d %d %d %d}\n",
        info->formatinfo.size, info->formatinfo.samplerate, info->formatinfo.channels, info->formatinfo.bits, info->formatinfo.format));
      // Make sure no important information is modified here if currently
      // opened when using another thread (ie.: always_hungry = FALSE)
      // for output which is not the case here.
      a->original_info = *info;
      info->always_hungry = TRUE;
      break;

    case OUTPUT_TRASH_BUFFERS:
      break;

    case OUTPUT_NOBUFFERMODE:
      break;

    default:
      rc = 1;
  }

  return rc;
}

/* This function is called by the decoder or last in chain
   filter plug-in to play samples. */
int PM123_ENTRY
output_play_samples( void* A, FORMAT_INFO* format, char* buf, int len, int posmarker )
{
  WAVOUT *a = (WAVOUT *) A;
  int written;
  DEBUGLOG(("wavout:output_play_samples(%p, %p, %p, %d, %d)\n", A, format, buf, len, posmarker));

  // Sets priority to idle. Normal and especially not boost priority are needed
  // or desired here, but that wouldn't be the case for real-time output
  // plug-ins.
  // DosSetPriority ( PRTYS_THREAD, PRTYC_IDLETIME, PRTYD_MAXIMUM, 0);
  DosWaitEventSem( a->pause, SEM_INDEFINITE_WAIT );

  memcpy( a->buffer, buf, len );
  a->playingpos = posmarker;

  if( memcmp( format, &a->original_info.formatinfo, sizeof( FORMAT_INFO )) != 0 ) {
    DEBUGLOG(("wavout:output_play_samples: format mismatch {%d %d %d %d %d}\n",
      format->size, format->samplerate, format->channels, format->bits, format->format));
    (*a->original_info.info_display)( "Warning: WAV data currently being written is in a different\n"
                                      "format than the opened format.  Generation of a probably\n"
                                      "invalid WAV file." );
  }

  written = a->wavfile.writeData( buf, len );

  if( written < len )
  {
    char message[2048];
    sprintf( message, "Could not write to WAV file:\n%s\n%s", a->fullpath, clib_strerror( errno ));
    WinPostMsg( a->original_info.hwnd, WM_PLAYERROR, 0, 0 );
    (*a->original_info.error_display)( message );
    a->wavfile.close();
  }

  WinPostMsg( a->original_info.hwnd, WM_OUTPUT_OUTOFDATA, 0, 0 );
  return written;
}

/* This function is used by visual plug-ins so the user can
   visualize what is currently being played. */
ULONG PM123_ENTRY
output_playing_samples( void* A, FORMAT_INFO* info, char* buf, int len )
{
  WAVOUT* a = (WAVOUT*)A;

 *info = a->original_info.formatinfo;
  memcpy( buf, a->buffer, len );

  return 0;
}

/* Returns the playing position. */
ULONG PM123_ENTRY
output_playing_pos( void* A )
{
  return ((WAVOUT*)A)->playingpos;
}

/* Returns TRUE if the output plug-in still has some buffers to play. */
BOOL PM123_ENTRY
output_playing_data( void* A )
{
  return FALSE;
}

/* Returns information about plug-in. */
int PM123_ENTRY
plugin_query( PLUGIN_QUERYPARAM* query )
{
  query->type         = PLUGIN_OUTPUT;
  query->author       = "Samuel Audet, Dmitry A.Steklenev ";
  query->desc         = "WAVE Output 1.10";
  query->configurable = TRUE;
  return 0;
}

/* Initialize the output plug-in. */
ULONG PM123_ENTRY
output_init( void** A )
{
  WAVOUT* a = (WAVOUT*)malloc( sizeof(*a));
  HINI hini;
  DEBUGLOG(("wavout:output_init(%p)\n", A));

  *A = a;
  *outpath = 0;

  if(( hini = open_module_ini()) != NULLHANDLE ) {
    load_ini_string( hini, outpath, sizeof( outpath ));
    close_ini( hini );
  }

  memset( a, 0, sizeof(*a));
  DosCreateEventSem( NULL, &a->pause, 0, TRUE );

  return 0;
}

/* Uninitialize the output plug-in. */
ULONG PM123_ENTRY
output_uninit( void* A )
{
  WAVOUT* a = (WAVOUT*)A;
  DEBUGLOG(("wavout:output_uninit(%p)\n", A));

  DosCloseEventSem( a->pause );
  free( a->buffer );
  free( a );

  return 0;
}

