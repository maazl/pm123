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
#include <os2me.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <utilfct.h>
#include <fileutil.h>
#include <format.h>
#include <output_plug.h>
#include <decoder_plug.h>
#include <plugin.h>
#include <wavout.h>
#include <debuglog.h>


static const PLUGIN_CONTEXT* context;

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

/* Configure plug-in. */
void DLLENTRY
plugin_configure( HWND hwnd, HMODULE module ) {
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, (PVOID)module );
}

/* Closes a output file. */
static ULONG
output_close( void* A )
{
  WAVOUT* a = (WAVOUT*)A;
  ULONG resetcount;

  if( a->file )
  {
    if( fseek( a->file, 0, SEEK_SET ) == 0 ) {
      fwrite( &a->header, 1, sizeof( a->header ), a->file );
    }
    fclose( a->file );

    DosResetEventSem( a->pause, &resetcount );

    free( a->buffer );
    a->buffer = NULL;
  }
  return 0;
}

/* Opens a output file. */
static ULONG
output_open( WAVOUT* a )
{
  char errorbuf[1024];

  // New filename, even if we didn't explicity get a close!
  if( a->file ) {
    output_close( a );
  }

  a->playingpos = 0;
  free( a->buffer );
  a->buffer = (char*)malloc( a->original_info.buffersize );

  DosPostEventSem( a->pause );
  strcpy( a->fullpath, outpath );

  if( stricmp( a->fullpath, "nul" ) != 0 ) {
    if( *a->fullpath && !is_root( a->fullpath )) {
      strlcat( a->fullpath, "\\", sizeof( a->fullpath ));
      strlcat( a->fullpath, a->filename, sizeof( a->fullpath ));
    } else {
      strlcat( a->fullpath, a->filename, sizeof( a->fullpath ));
    }
  }

  if(( a->original_info.formatinfo.format != WAVE_FORMAT_PCM ) ||
     ( a->original_info.formatinfo.bits != 16 && a->original_info.formatinfo.bits != 8 ) ||
     ( a->original_info.formatinfo.channels > 2 ))
  {
    strlcpy( errorbuf, "Could not write output data to file:\n" , sizeof( errorbuf ));
    strlcat( errorbuf, a->fullpath, sizeof( errorbuf ));
    strlcat( errorbuf, "\n", sizeof( errorbuf ));
    strlcat( errorbuf, "Unsupported format of the input stream.", sizeof( errorbuf ));

    a->original_info.error_display( errorbuf );
    return EINVAL;
  }

  if(( a->file = fopen( a->fullpath, "wb" )) == NULL )
  {
    strlcpy( errorbuf, "Could not open file to output data:\n", sizeof( errorbuf ));
    strlcat( errorbuf, a->fullpath, sizeof( errorbuf ));
    strlcat( errorbuf, "\n", sizeof( errorbuf ));
    strlcat( errorbuf, strerror( errno ), sizeof( errorbuf ));

    a->original_info.error_display( errorbuf );
    return errno;
  }

  strncpy( a->header.riff.id_riff, "RIFF", 4 );
  strncpy( a->header.riff.id_wave, "WAVE", 4 );
  a->header.riff.len = sizeof( a->header ) - 8;

  strncpy( a->header.format_header.id, "fmt ", 4 );
  a->header.format_header.len = sizeof( a->header.format_header ) +
                                sizeof( a->header.format ) - 8;

  a->header.format.FormatTag      = a->original_info.formatinfo.format;
  a->header.format.Channels       = a->original_info.formatinfo.channels;
  a->header.format.SamplesPerSec  = a->original_info.formatinfo.samplerate;
  a->header.format.BitsPerSample  = a->original_info.formatinfo.bits;
  a->header.format.AvgBytesPerSec = a->header.format.Channels *
                                    a->header.format.SamplesPerSec *
                                    a->header.format.BitsPerSample / 8;
  a->header.format.BlockAlign     = a->header.format.Channels *
                                    a->header.format.BitsPerSample / 8;

  strncpy( a->header.data_header.id, "data", 4 );
  a->header.data_header.len = 0;

  if( fwrite( &a->header, 1, sizeof( a->header ), a->file ) != sizeof( a->header ))
  {
    strlcpy( errorbuf, "Could not write output data to file:\n", sizeof( errorbuf ));
    strlcat( errorbuf, a->fullpath, sizeof( errorbuf ));
    strlcat( errorbuf, "\n", sizeof( errorbuf ));
    strlcat( errorbuf, strerror( errno ), sizeof( errorbuf ));

    a->original_info.error_display( errorbuf );

    fclose( a->file );
    return errno;
  }
  return PLUGIN_OK;
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
ULONG DLLENTRY
output_command( void* A, ULONG msg, OUTPUT_PARAMS* info )
{
  WAVOUT* a = (WAVOUT*)A;
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
      return output_open( a );
    }

    case OUTPUT_CLOSE:
      return output_close( a );

    case OUTPUT_VOLUME:
      return 0;

    case OUTPUT_PAUSE:
      return output_pause( a, info->pause );

    case OUTPUT_SETUP:
      DEBUGLOG(("wavout:output_command:OUTPUT_SETUP: {%d %d %d %d %d}\n",
        info->formatinfo.size, info->formatinfo.samplerate, info->formatinfo.channels, info->formatinfo.bits, info->formatinfo.format));
      info->always_hungry = TRUE;
      a->original_info = *info;
      return 0;

    case OUTPUT_TRASH_BUFFERS:
      return 0;

    case OUTPUT_NOBUFFERMODE:
      return 0;
  }

  return MCIERR_UNSUPPORTED_FUNCTION;
}

/* This function is called by the decoder or last in chain
   filter plug-in to play samples. */
int DLLENTRY
output_play_samples( void* A, FORMAT_INFO* format, char* buf, int len, int posmarker )
{
  WAVOUT *a = (WAVOUT*)A;
  int done;
  DEBUGLOG(("wavout:output_play_samples(%p, %p, %p, %d, %d)\n", A, format, buf, len, posmarker));

  DosWaitEventSem( a->pause, SEM_INDEFINITE_WAIT );

  memcpy( a->buffer, buf, len );
  a->playingpos = posmarker;

  if( memcmp( format, &a->original_info.formatinfo, sizeof( FORMAT_INFO )) != 0 ) {

    DEBUGLOG(( "wavout: old format: size %d, sample: %d, channels: %d, bits: %d, id: %d\n",

        a->original_info.formatinfo.size,
        a->original_info.formatinfo.samplerate,
        a->original_info.formatinfo.channels,
        a->original_info.formatinfo.bits,
        a->original_info.formatinfo.format ));

    DEBUGLOG(( "wavout: new format: size %d, sample: %d, channels: %d, bits: %d, id: %d\n",

        format->size,
        format->samplerate,
        format->channels,
        format->bits,
        format->format ));

    a->original_info.info_display( "WAV data currently being written is in a different"
                                   "format than the opened format. Generation of a probably"
                                   "invalid WAV file." );

    // Protection against a storm of warnings.
    a->original_info.formatinfo = *format;
  }

  if(( done = fwrite( buf, 1, len, a->file )) != len )
  {
    char errorbuf[1024];

    strlcpy( errorbuf, "Could not write output data  to file:\n", sizeof( errorbuf ));
    strlcat( errorbuf, a->fullpath, sizeof( errorbuf ));
    strlcat( errorbuf, "\n", sizeof( errorbuf ));
    strlcat( errorbuf, strerror( errno ), sizeof( errorbuf ));

    a->original_info.error_display( errorbuf );
    output_close( a );
  }

  a->header.riff.len += done;
  a->header.data_header.len += done;

  return done;
}

/* This function is used by visual plug-ins so the user can
   visualize what is currently being played. */
ULONG DLLENTRY
output_playing_samples( void* A, FORMAT_INFO* info, char* buf, int len )
{
  WAVOUT* a = (WAVOUT*)A;

 *info = a->original_info.formatinfo;

  if( len > a->original_info.buffersize ) {
    return PLUGIN_FAILED;
  }

  if( buf && len ) {
    memcpy( buf, a->buffer, len );
  }

  return PLUGIN_OK;
}

/* Returns the playing position. */
ULONG DLLENTRY
output_playing_pos( void* A )
{
  return ((WAVOUT*)A)->playingpos;
}

/* Returns TRUE if the output plug-in still has some buffers to play. */
BOOL DLLENTRY
output_playing_data( void* A )
{
  return FALSE;
}

/* Returns information about plug-in. */
int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM* query )
{
  query->type         = PLUGIN_OUTPUT;
  query->author       = "Samuel Audet, Dmitry A.Steklenev ";
  query->desc         = "WAVE Output 1.20";
  query->configurable = TRUE;
  return 0;
}

/* init plug-in */
int DLLENTRY
plugin_init( const PLUGIN_CONTEXT* ctx )
{
  context = ctx;
  return 0;
}

/* Initialize the output plug-in. */
ULONG DLLENTRY
output_init( void** A )
{
  WAVOUT* a = (WAVOUT*)malloc( sizeof(*a));
  DEBUGLOG(("wavout:output_init(%p)\n", A));

  *A = a;
  memset(outpath, 0, sizeof outpath);
  context->plugin_api->profile_query( "outpath", outpath, sizeof outpath );

  memset( a, 0, sizeof(*a));
  DosCreateEventSem( NULL, &a->pause, 0, TRUE );

  return PLUGIN_OK;
}

/* Uninitialize the output plug-in. */
ULONG DLLENTRY
output_uninit( void* A )
{
  WAVOUT* a = (WAVOUT*)A;
  DEBUGLOG(("wavout:output_uninit(%p)\n", A));

  DosCloseEventSem( a->pause );
  free( a->buffer );
  free( a );

  return PLUGIN_OK;
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
