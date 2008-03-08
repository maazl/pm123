/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
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
 *
 * The main source file to cddaplay.dll plug-in
 */

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_ERRORS
#include <os2.h>

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <types.h>
#include <process.h>

#include <format.h>
#include <decoder_plug.h>
#include <plugin.h>
#include <utilfct.h>
#include <snprintf.h>
#include <debuglog.h>

#include "readcd.h"
#include "cddb.h"
#include "cddaplay.h"

#define  WM_SAVE        ( WM_USER + 1 )
#define  WM_UPDATE_DONE ( WM_USER + 2 )

#undef   CDDB_ALWAYS_QUERY

static DECODER_SETTINGS settings;
static TID              tid_update  = -1;
static CDINFO           disc_info;
static unsigned char    disc_upc[7] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static CDDB_CONNECTION* cddb;
static BOOL             cddb_used;

static ULONG negative_hits[1024];
static ULONG nh_head = 0;

/* Serializes access to the library's global data. */
static HMTX mutex;

#if CDDB_PROTOLEVEL < 6
  /* Returns the character set currently used by PM123. */
  static int
  query_pm123_charset( void )
  {
    PPIB  ppib;
    PTIB  ptib;
    HINI  hini;
    char  exefile[_MAX_PATH];
    char  inifile[_MAX_PATH];

    struct { int tags_charset; } cfg = { CH_DEFAULT };

    DosGetInfoBlocks( &ptib, &ppib );
    DosQueryModuleName( ppib->pib_hmte, sizeof(exefile), exefile );

    _splitpath( exefile, NULL, inifile, NULL, NULL );
    strcat( inifile, "pm123.ini" );

    hini = open_ini( inifile );

    if( hini ) {
      load_ini_value( hini, cfg.tags_charset )
      close_ini( hini );
    }

    return cfg.tags_charset;
  }
#else
  /* Returns the current system code page. */
  static int
  query_system_charset( void )
  {
    ULONG cpage[1] = { 0 };
    ULONG incb     = sizeof( cpage );
    ULONG oucb     = 0;

    DosQueryCp( incb, cpage, &oucb );
    return cpage[0];
  }
#endif

/* Decoding thread. */
static void TFNENTRY
decoder_thread( void* arg )
{
  ULONG  resetcount;
  ULONG  markerpos;
  ULONG  samplesize;
  APIRET rc;

  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;
  char* buffer = NULL;

  w->frew = FALSE;
  w->ffwd = FALSE;
  w->stop = FALSE;

  if(( rc = cd_open( w->drive, &w->disc )) != NO_ERROR ||
     ( rc = cd_info( w->disc , &w->info )) != NO_ERROR )
  {
    if( w->error_display )
    {
      char errorbuf[1024];
      char os2error[1024];

      snprintf( errorbuf, sizeof( errorbuf ), "Unable open disc %s\n%s\n",
                w->drive, os2_strerror( rc, os2error, sizeof( os2error )));

      w->error_display( errorbuf );
    }
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  cd_select_track( w->disc, &w->info.track_info[w->track] );

  // After opening a new track we so are in its beginning.
  if( w->jumptopos == 0 ) {
      w->jumptopos = -1;
  }

  if(( buffer = (char*)malloc( w->audio_buffersize )) == NULL ) {
    WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
    goto end;
  }

  w->output_format.size       = sizeof( w->output_format );
  w->output_format.format     = WAVE_FORMAT_PCM;
  w->output_format.bits       = 16;
  w->output_format.channels   = w->info.track_info[w->track].channels;
  w->output_format.samplerate = 44100;

  samplesize = ( w->output_format.bits / 8 ) * w->output_format.channels;

  for(;;)
  {
    DosWaitEventSem ( w->play, SEM_INDEFINITE_WAIT );
    DosResetEventSem( w->play, &resetcount );

    if( w->stop ) {
      break;
    }

    w->status = DECODER_PLAYING;

    while( !w->stop )
    {
      int read;
      int write;

      if( w->jumptopos >= 0 )
      {
        long byte;

        byte = (float)w->jumptopos / decoder_length( w ) * w->disc->size;
        byte = byte / samplesize * samplesize;

        w->jumptopos = -1;
        cd_seek( w->disc, byte );
        WinPostMsg( w->hwnd, WM_SEEKSTOP, 0, 0 );
      }

      if( w->frew ) {
        /* ie.: ~1/2 second */
        int byte = cd_tell( w->disc ) - w->output_format.samplerate / 2 * samplesize;
        if( byte < 0 ) {
          break;
        } else {
          cd_seek( w->disc, byte );
        }
      }

      if( w->ffwd ) {
        int byte = cd_tell( w->disc ) + w->output_format.samplerate / 2 * samplesize;
        cd_seek( w->disc, byte );
      }

      markerpos = 8000.0 * cd_tell( w->disc ) / w->output_format.samplerate
                                              / w->output_format.channels
                                              / w->output_format.bits;

      read = cd_read_data( w->disc, buffer, w->audio_buffersize );

      if( read <= 0 ) {
        break;
      }

      write = w->output_play_samples( w->a, &w->output_format, buffer, read, markerpos );

      if( write != read ) {
        WinPostMsg( w->hwnd, WM_PLAYERROR, 0, 0 );
        break;
      }
    }
    WinPostMsg( w->hwnd, WM_PLAYSTOP, 0, 0 );
    w->status = DECODER_STOPPED;
  }

end:

  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if( w->disc ) {
    cd_close( w->disc );
  }
  free( buffer );

  w->decodertid = -1;
  w->status = DECODER_STOPPED;

  DosPostEventSem   ( w->ok    );
  DosReleaseMutexSem( w->mutex );
  _endthread();
}

/* Init function is called when PM123 needs the specified decoder to play
   the stream demanded by the user. */
int DLLENTRY
decoder_init( void** returnw )
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)calloc( sizeof(*w), 1 );
  *returnw = w;

  DosCreateEventSem( NULL, &w->play,  0, FALSE );
  DosCreateEventSem( NULL, &w->ok,    0, FALSE );
  DosCreateMutexSem( NULL, &w->mutex, 0, FALSE );

  w->decodertid = -1;
  w->status = DECODER_STOPPED;
  return 0;
}

/* Uninit function is called when another decoder than this is needed. */
BOOL DLLENTRY
decoder_uninit( void* arg )
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;

  decoder_command ( w, DECODER_STOP, NULL );
  DosCloseEventSem( w->play  );
  DosCloseEventSem( w->ok    );
  DosCloseMutexSem( w->mutex );
  free( w );
  return TRUE;
}

/* There is a lot of commands to implement for this function. Parameters
   needed for each of the are described in the definition of the structure
   in the decoder_plug.h file. */
ULONG DLLENTRY
decoder_command( void* arg, ULONG msg, DECODER_PARAMS* info )
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;
  ULONG resetcount;

  switch(msg)
  {
    case DECODER_SETUP:
      w->output_play_samples = info->output_play_samples;
      w->hwnd                = info->hwnd;
      w->error_display       = info->error_display;
      w->info_display        = info->info_display;
      w->a                   = info->a;
      w->audio_buffersize    = info->audio_buffersize;
      break;

    case DECODER_PLAY:
    {
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

      if( w->decodertid != -1 ) {
        DosReleaseMutexSem( w->mutex );
        decoder_command( w, DECODER_STOP, NULL );
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
      }

      w->drive[0]   = info->drive[0];
      w->drive[1]   = ':';
      w->drive[2]   = 0;
      w->track      = info->track;
      w->jumptopos  = info->jumpto;
      w->status     = DECODER_STARTING;
      w->decodertid = _beginthread( decoder_thread, 0, 65535, (void*)w );

      DosReleaseMutexSem( w->mutex );
      DosPostEventSem   ( w->play  );
      break;
    }

    case DECODER_STOP:
    {
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

      if( w->decodertid == -1 ) {
        DosReleaseMutexSem( w->mutex );
        break;
      }

      w->stop = TRUE;

      DosResetEventSem  ( w->ok, &resetcount );
      DosReleaseMutexSem( w->mutex );
      DosPostEventSem   ( w->play  );
      DosWaitEventSem   ( w->ok, SEM_INDEFINITE_WAIT );
      break;
    }

    case DECODER_FFWD:
      if( info->ffwd ) {
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        if( w->decodertid == -1 ) {
          DosReleaseMutexSem( w->mutex );
          return 1;
        }
        DosReleaseMutexSem( w->mutex );
      }
      w->ffwd = info->ffwd;
      break;

    case DECODER_REW:
      if( info->rew ) {
        DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        if( w->decodertid == -1 ) {
          DosReleaseMutexSem( w->mutex );
          return 1;
        }
        DosReleaseMutexSem( w->mutex );
      }
      w->frew = info->rew;
      break;

    case DECODER_JUMPTO:
      w->jumptopos = info->jumpto;
      if( w->status == DECODER_STOPPED ) {
        DosPostEventSem( w->play );
      }
      break;

    default:
      return 1;
   }

   return 0;
}

/* Returns number of milliseconds the stream lasts. */
ULONG DLLENTRY
decoder_length( void* arg )
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;

  if( w->output_format.samplerate && w->disc )
  {
    return 8000.0 * w->disc->size / w->output_format.samplerate
                                  / w->output_format.channels
                                  / w->output_format.bits;
  } else {
    return 0;
  }
}

/* Returns current status of the decoder. */
ULONG DLLENTRY
decoder_status( void* arg ) {
  return ((DECODER_STRUCT*)arg)->status;
}

/* Returns information about specified file. */
ULONG DLLENTRY
decoder_fileinfo( char* filename, DECODER_INFO* info ) {
  return 200;
}

/* Returns information about a disc inserted to the specified drive. */
ULONG DLLENTRY
decoder_cdinfo( char* drive, DECODER_CDINFO* info )
{
  CDHANDLE* disc;
  CDINFO    fresh_info;
  APIRET    rc;

  unsigned char fresh_upc[7] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  if(( rc = cd_open( drive, &disc )) != NO_ERROR  ) {
    if( rc == ERROR_NOT_READY ) {
      DEBUGLOG(( "cddaplay: drive %s is not ready\n", drive ));
      return 0;
    } else {
      DEBUGLOG(( "cddaplay: unable open disc %s, rc = %08X\n", drive, rc ));
      return 100;
    }
  }

  if(( rc = cd_info( disc, &fresh_info )) != NO_ERROR ) {
    DEBUGLOG(( "cddaplay: unable get disc %s info, rc = %08X\n", drive, rc ));
    return 100;
  }

  cd_upc( disc, fresh_upc );

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  disc_info = fresh_info;
  memcpy( disc_upc, fresh_upc, sizeof( disc_upc ));
  DosReleaseMutexSem( mutex );

  info->sectors    = fresh_info.lead_out_address;
  info->firsttrack = fresh_info.track_first;
  info->lasttrack  = fresh_info.track_last;

  // I on a regular basis receive a UPC not similar to BCD format.
  // Why? But it is unique for each disc.
  DEBUGLOG(( "cddaplay: disc UPC is %02X%02X%02X%02X%02X%02X%02X\n",
              fresh_upc[0], fresh_upc[1], fresh_upc[2], fresh_upc[3],
              fresh_upc[4], fresh_upc[5], fresh_upc[6] ));

  cd_close( disc );
  return 0;
}

/* Processes messages of the fuzzy match dialog. */
static MRESULT EXPENTRY
fuzzy_match_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 )) {
        case DID_OK:
          WinDismissDlg( hwnd, lb_selected( hwnd, LB_MATCHES, LIT_FIRST ));
          return 0;

        case DID_CANCEL:
          WinDismissDlg( hwnd, LIT_NONE );
          return 0;
      }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Returns the next CDDB server. */
static ULONG
get_next_server( char* server, ULONG size, SHORT* index )
{
  SHORT i  = *index + 1;
  ULONG rc = 0;

  if( settings.use_http ) {
    if( !settings.try_all_servers  ) {
      while( i < settings.num_http && !settings.http_selects[i] ) {
        i++;
      }
      // If it is not selected any server, to use the first.
      if( i >= settings.num_http && *index == -1 && settings.num_http ) {
        i = 0;
      }
    }
    if( i < settings.num_http ) {
      strlcpy( server, settings.http_servers[i], size );
    } else {
      rc = -1;
    }
  } else {
    if( !settings.try_all_servers  ) {
      while( i < settings.num_cddb && !settings.cddb_selects[i] ) {
        i++;
      }
      // If it is not selected any server, to use the first.
      if( i >= settings.num_cddb && *index == -1 && settings.num_cddb ) {
        i = 0;
      }
    }
    if( i < settings.num_cddb ) {
      strlcpy( server, settings.cddb_servers[i], size );
    } else {
      rc = -1;
    }
  }

  *index = i;
  return rc;
}

/* Creates a new CDDB connection structure. Returns a pointer to a
   CDDB structure that can be used to access the database. A NULL
   pointer return value indicates an error. */
static CDDB_CONNECTION*
decoder_cddb_connect( void )
{
  char progname[_MAX_PATH];
  char cachedir[_MAX_PATH];
  #if CDDB_PROTOLEVEL >= 6
  char charset [64];
  #endif

  CDDB_CONNECTION* c;

  if(( c = cddb_connect()) != NULL ) {
    getExeName( progname, sizeof( progname ));
    sdrivedir( cachedir, progname, sizeof( progname ));
    strlcat( cachedir, "cddb", sizeof( cachedir ));
    cddb_set_cache_dir( c, cachedir );

    #if CDDB_PROTOLEVEL >= 6
      snprintf( charset, sizeof( charset ), "CP%d", query_system_charset());
      cddb_set_charset( c, charset );
    #endif
  }

  return c;
}

/* Requests ownership of the cached CDDB connection structure.
   If this is impossible then creates a new CDDB connection structure.
   Returns a pointer to a CDDB structure that can be used to
   access the database. A NULL pointer return value indicates
   an error. */
static CDDB_CONNECTION*
decoder_cddb_request( void )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );

  if( !cddb_used ) {
    if( !cddb ) {
      cddb = decoder_cddb_connect();
    }
    if(  cddb ) {
      cddb_used = TRUE;
    }
    DosReleaseMutexSem( mutex );
    return cddb;
  } else {
    return decoder_cddb_connect();
  }
}

/* Relinquishes ownership of the CDDB connection structure
   was requested by decoder_cddb_request. */
static void
decoder_cddb_release( CDDB_CONNECTION* c )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  if( c ) {
    if( c != cddb ) {
      if( !cddb_used ) {
        cddb_close( cddb );
        cddb = c;
      } else {
        cddb_close( c );
      }
    } else {
      cddb_used = FALSE;
    }
  }
  DosReleaseMutexSem( mutex );
}

/* Returns information about specified track. */
ULONG DLLENTRY
decoder_trackinfo( char* drive, int track, DECODER_INFO* info )
{
  CDHANDLE*        disc;
  CDINFO           last_info;
  unsigned char    last_upc[7];
  CDDBRC           rc;
  char             match[2048] = "";
  char             server[512];
  SHORT            i = -1, j;
  CDDB_CONNECTION* c;

  memset( info, 0, sizeof( *info ));
  info->size = sizeof( *info );

  if( cd_open( drive, &disc ) != NO_ERROR ) {
    return 100;
  }

  if( cd_upc( disc, last_upc ) == NO_ERROR &&
      memcmp( last_upc, disc_upc, sizeof( disc_upc )) == 0 )
  {
    DEBUGLOG(( "cddaplay: cached disc info is suitable.\n" ));
    DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
    last_info = disc_info;
    DosReleaseMutexSem( mutex );
  } else {
    DEBUGLOG(( "cddaplay: cached disc info is obsolete.\n" ));
    if( cd_info( disc, &last_info ) != NO_ERROR ) {
      return 100;
    }
    DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
    disc_info = last_info;
    memcpy( disc_upc, last_upc, sizeof( disc_upc ));
    DosReleaseMutexSem( mutex );
  }

  cd_close( disc );

  // data track
  if( track > last_info.track_last  ||
      track < last_info.track_first || last_info.track_info[track].is_data )
  {
    return 200;
  }

  info->startsector       = last_info.track_info[track].start;
  info->endsector         = last_info.track_info[track].end;
  info->filesize          = last_info.track_info[track].size;
  info->format.size       = sizeof( info->format );
  info->format.format     = WAVE_FORMAT_PCM;;
  info->format.samplerate = 44100;
  info->format.bits       = 16;
  info->format.channels   = last_info.track_info[track].channels;
  info->bitrate           = info->format.samplerate * info->format.bits *
                            info->format.channels / 1000;
  info->songlength        = 8000.0 * last_info.track_info[track].size / info->format.samplerate /
                            info->format.channels / info->format.bits;

  strcpy( info->tech_info, "True CD Quality" );

  if( !settings.use_cddb ) {
    return 0;
  }

  for( j = 0; j < sizeof( negative_hits ) / sizeof( negative_hits[0] ); j++ ) {
     if( negative_hits[j] == last_info.discid ) {
       DEBUGLOG(( "cddaplay: cached negative hit: %08x\n", last_info.discid ));
       return 0;
     }
  }

  if(( c = decoder_cddb_request()) == NULL ) {
    return 0;
  }

  DEBUGLOG(( "cddaplay: cached CDDB id is %08lx, request %08lx\n", c->discid, last_info.discid ));

  if( c->discid != last_info.discid )
  {
    cddb_set_email_address( c, settings.email );
    while( get_next_server( server, sizeof( server ), &i ) != -1 )
    {
      cddb_set_server( c, server );
      rc = cddb_disc ( c, &last_info, match, sizeof( match ));

      #ifdef CDDB_ALWAYS_QUERY
      if( rc == CDDB_MORE_DATA || rc == CDDB_OK )
      #else
      if( rc == CDDB_MORE_DATA )
      #endif
      {
        ULONG   select;
        HMODULE hmodule;
        char    modname[_MAX_PATH];
        HWND    hwnd;

        getModule( &hmodule, modname, sizeof( modname ));

        hwnd = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                           fuzzy_match_dlg_proc, hmodule, DLG_MATCH, NULL );

        do_warpsans( hwnd );
        do {
          #if CDDB_PROTOLEVEL < 6
            char item[2048];
            ch_convert( query_pm123_charset(), match, CH_DEFAULT, item, sizeof( item ));
            lb_add_item( hwnd, LB_MATCHES, item  );
          #else
            lb_add_item( hwnd, LB_MATCHES, match );
          #endif
        } while ( cddb_disc_next( c, match, sizeof( match )) == CDDB_OK );

        lb_select( hwnd, LB_MATCHES, 0 );

        if(( select = WinProcessDlg( hwnd )) != LIT_NONE ) {
          lb_get_item( hwnd, LB_MATCHES, select, match, sizeof( match ));
        } else {
          *match = 0;
        }
        WinDestroyWindow( hwnd );
        break;
      #ifndef CDDB_ALWAYS_QUERY
      } else if( rc == CDDB_OK ) {
        break;
      #endif
      } else {
        *match = 0;
      }
    }

    if( !*match || cddb_read( c, &last_info, match ) != CDDB_OK )
    {
      negative_hits[ nh_head++ ] = last_info.discid;
      if( nh_head >= sizeof( negative_hits ) / sizeof( negative_hits[0] )) {
         nh_head = 0;
      }
      decoder_cddb_release( c );
      return 0;
    }
  }

  // The CDDB tracks numeration is started from 0.
  i = track - last_info.track_first;

  cddb_getstring( c, i, CDDB_TITLE,  info->title,  sizeof( info->title  ));
  cddb_getstring( c, i, CDDB_ARTIST, info->artist, sizeof( info->artist ));
  cddb_getstring( c, i, CDDB_ALBUM,  info->album,  sizeof( info->album  ));
  cddb_getstring( c, i, CDDB_YEAR,   info->year,   sizeof( info->year   ));
  cddb_getstring( c, i, CDDB_GENRE,  info->genre,  sizeof( info->genre  ));

  #if CDDB_PROTOLEVEL >= 6
  info->codepage = query_system_charset();
  #endif

  decoder_cddb_release( c );
  return 0;
}

/* What can be played via the decoder. */
ULONG DLLENTRY
decoder_support( char* ext[], int* size )
{
  if( size ) {
    *size = 0;
  }

  return DECODER_TRACK;
}

/* Saves decoder's settings. */
static void
save_ini( void )
{
  HINI hini;
  int  i;

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value ( hini, settings.use_cddb );
    save_ini_value ( hini, settings.use_http );
    save_ini_value ( hini, settings.try_all_servers);
    save_ini_string( hini, settings.email );

    PrfWriteProfileData( hini, "CDDBServers", NULL, NULL, 0 );
    PrfWriteProfileData( hini, "HTTPServers", NULL, NULL, 0 );

    for( i = 0; i < settings.num_cddb; i++ ) {
      PrfWriteProfileData( hini, "CDDBServers", settings.cddb_servers[i],
                                                (void*)&settings.cddb_selects[i], 4 );
    }
    for( i = 0; i < settings.num_http; i++ ) {
      PrfWriteProfileData( hini, "HTTPServers", settings.http_servers[i],
                                                (void*)&settings.http_selects[i], 4 );
    }
    close_ini( hini );
  }
}

/* Loads decoder's settings. */
static void
load_ini( void )
{
  HINI hini;
  int  i;

  for( i = 0; i < 128; i++ )
  {
    free( settings.cddb_servers[i] );
    free( settings.http_servers[i] );

    settings.http_servers[i] = NULL;
    settings.cddb_servers[i] = NULL;
    settings.cddb_selects[i] = FALSE;
    settings.http_selects[i] = FALSE;
  }

  settings.num_cddb        = 0;
  settings.num_http        = 0;
  settings.use_cddb        = TRUE;
  settings.use_http        = TRUE;
  settings.try_all_servers = FALSE;

  strcpy( settings.email,"someone@somewhere.com" );

  if(( hini = open_module_ini()) != NULLHANDLE )
  {
    char* buffer;
    ULONG size;

    load_ini_value ( hini, settings.use_cddb );
    load_ini_value ( hini, settings.use_http );
    load_ini_value ( hini, settings.try_all_servers );
    load_ini_string( hini, settings.email, sizeof( settings.email ));

    if( PrfQueryProfileSize( hini, "CDDBServers", NULL, &size ) && size > 0 )
    {
      char* next_string;
      buffer = (char*)calloc( size, 1 );
      next_string = buffer;

      PrfQueryProfileData( hini, "CDDBServers", NULL, buffer, &size );

      for( i = 0; i < 128; i++ )
      {
        BOOL  is_selected  = FALSE;
        ULONG is_selected_size = 4;

        if( !*next_string ) {
          break;
        }

        settings.cddb_servers[i] = strdup( next_string );

        if( PrfQueryProfileData( hini, "CDDBServers", next_string, (void*)&is_selected, &is_selected_size )) {
          if( is_selected ) {
            settings.cddb_selects[i] = TRUE;
          }
        }

        next_string = strchr( next_string, 0 ) + 1;
      }

      settings.num_cddb = i;
      free( buffer );
    } else {
      settings.cddb_servers[0] = strdup( "cddbp://freedb.freedb.org:8880" );
      settings.cddb_servers[1] = strdup( "cddbp://at.freedb.org:8880" );
      settings.cddb_servers[2] = strdup( "cddbp://au.freedb.org:8880" );
      settings.cddb_servers[3] = strdup( "cddbp://ca.freedb.org:8880" );
      settings.cddb_servers[4] = strdup( "cddbp://de.freedb.org:8880" );
      settings.cddb_servers[5] = strdup( "cddbp://es.freedb.org:8880" );
      settings.cddb_servers[6] = strdup( "cddbp://fi.freedb.org:8880" );
      settings.cddb_servers[7] = strdup( "cddbp://ru.freedb.org:8880" );
      settings.cddb_servers[8] = strdup( "cddbp://uk.freedb.org:8880" );
      settings.cddb_servers[9] = strdup( "cddbp://us.freedb.org:8880" );
      settings.cddb_selects[0] = TRUE;

      settings.num_cddb = 10;
    }

    if( PrfQueryProfileSize( hini, "HTTPServers", NULL, &size ) && size > 0 )
    {
      char* next_string;
      buffer = (char*)calloc( size, 1 );
      next_string = buffer;

      PrfQueryProfileData( hini, "HTTPServers", NULL, buffer, &size );

      for( i = 0; i < 128; i++ )
      {
        BOOL  is_selected  = FALSE;
        ULONG is_selected_size = 4;

        if( !*next_string ) {
          break;
        }

        settings.http_servers[i] = strdup( next_string );

        if( PrfQueryProfileData( hini, "HTTPServers", next_string, (void*)&is_selected, &is_selected_size )) {
          if( is_selected ) {
            settings.http_selects[i] = TRUE;
          }
        }

        next_string = strchr( next_string, 0 ) + 1;
      }

      settings.num_http = i;
      free( buffer );
    } else {
      settings.http_servers[0] = strdup( "http://freedb.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[1] = strdup( "http://at.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[2] = strdup( "http://au.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[3] = strdup( "http://ca.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[4] = strdup( "http://de.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[5] = strdup( "http://es.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[6] = strdup( "http://fi.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[7] = strdup( "http://ru.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[8] = strdup( "http://uk.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_servers[9] = strdup( "http://us.freedb.org:80/~cddb/cddb.cgi" );
      settings.http_selects[0] = TRUE;

      settings.num_http = 10;
    }

    close_ini( hini );
  }
}

/* Updating thread. */
static void TFNENTRY
cfg_cddb_update_thread( void* arg )
{
  SHORT i    = -1;
  HWND  hwnd = (HWND)arg;
  HAB   hab  = WinInitialize( 0 );
  HMQ   hmq  = WinCreateMsgQueue( hab, 0 );

  char  server[512];

  CDDB_CONNECTION* cddb = cddb_connect();
  cddb_set_email_address( cddb, settings.email );

  while( get_next_server( server, sizeof( server ), &i ) != -1 )
  {
    cddb_set_server( cddb, server );
    if( cddb_mirror( cddb, server, sizeof( server )) == CDDB_OK ) {
      do {
        if( strnicmp( server, "http://", 7 ) == 0 ) {
          if( lb_search( hwnd, LB_HTTPSERVERS, LIT_FIRST, server ) == LIT_NONE ) {
            lb_add_item( hwnd, LB_HTTPSERVERS, server);
          }
        } else {
          if( lb_search( hwnd, LB_CDDBSERVERS, LIT_FIRST, server ) == LIT_NONE ) {
            lb_add_item( hwnd, LB_CDDBSERVERS, server);
          }
        }
      } while( cddb_mirror_next( cddb, server, sizeof( server )) == CDDB_OK );
    }
  }

  WinPostMsg( hwnd, WM_UPDATE_DONE, 0, 0 );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
  _endthread();
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_DESTROY:
      if( tid_update != -1 ) {
        DosKillThread(  tid_update );
        DosWaitThread( &tid_update, DCWW_WAIT );
      }

      WinSendMsg( hwnd, WM_UPDATE_DONE, 0, 0 );
      break;

    case WM_INITDLG:
    {
      int i;

      WinSendDlgItemMsg( hwnd, CB_USECDDB, BM_SETCHECK, MPFROMSHORT( settings.use_cddb ), 0 );
      WinSendDlgItemMsg( hwnd, CB_USEHTTP, BM_SETCHECK, MPFROMSHORT( settings.use_http ), 0 );
      WinSendDlgItemMsg( hwnd, CB_TRYALL,  BM_SETCHECK, MPFROMSHORT( settings.try_all_servers), 0 );
      WinSetDlgItemText( hwnd, EF_EMAIL,   settings.email);

      for( i = 0; i < settings.num_cddb; i++ ) {
        lb_add_item( hwnd, LB_CDDBSERVERS, settings.cddb_servers[i] );

        if( settings.cddb_selects[i] ) {
          lb_select( hwnd, LB_CDDBSERVERS, i );
        }
      }

      for( i = 0; i < settings.num_http; i++ ) {
        lb_add_item( hwnd, LB_HTTPSERVERS, settings.http_servers[i] );
        if( settings.http_selects[i] ) {
          lb_select( hwnd,LB_HTTPSERVERS, i );
        }
      }

      WinPostMsg( hwnd, WM_CONTROL, MPFROM2SHORT( CB_USECDDB, BN_CLICKED ),
                                    MPFROMHWND( WinWindowFromID( hwnd, CB_USECDDB )));
      break;
    }

    case WM_SAVE:
    {
      int i;

      settings.use_cddb        = (BOOL)WinSendDlgItemMsg( hwnd, CB_USECDDB, BM_QUERYCHECK, 0, 0 );
      settings.use_http        = (BOOL)WinSendDlgItemMsg( hwnd, CB_USEHTTP, BM_QUERYCHECK, 0, 0 );
      settings.try_all_servers = (BOOL)WinSendDlgItemMsg( hwnd, CB_TRYALL, BM_QUERYCHECK, 0, 0 );

      WinQueryDlgItemText( hwnd, EF_EMAIL, sizeof( settings.email ) - 1, settings.email );

      for( i = 0; i < 128; i++ )
      {
        free( settings.cddb_servers[i] );
        free( settings.http_servers[i] );

        settings.cddb_servers[i] = NULL;
        settings.http_servers[i] = NULL;
        settings.cddb_selects[i] = FALSE;
        settings.http_selects[i] = FALSE;
      }

      settings.num_cddb = lb_size( hwnd, LB_CDDBSERVERS );
      settings.num_cddb = truncate( settings.num_cddb, 0, 128 );

      for( i = 0; i < settings.num_cddb; i++ )
      {
        int size = lb_get_item_size( hwnd, LB_CDDBSERVERS, i );

        settings.cddb_servers[i] = (char*)malloc( size + 1 );
        lb_get_item( hwnd, LB_CDDBSERVERS, i, settings.cddb_servers[i], size + 1 );

        if( lb_selected( hwnd, LB_CDDBSERVERS, i - 1 ) == i ) {
          settings.cddb_selects[i] = TRUE;
        }
      }

      settings.num_http = lb_size( hwnd, LB_HTTPSERVERS );
      settings.num_http = truncate( settings.num_http, 0, 128 );

      for( i = 0; i < settings.num_http; i++ )
      {
        int size = lb_get_item_size( hwnd, LB_HTTPSERVERS, i );

        settings.http_servers[i] = (char*)malloc( size + 1 );
        lb_get_item( hwnd, LB_HTTPSERVERS, i, settings.http_servers[i], size + 1 );

        if( lb_selected( hwnd, LB_HTTPSERVERS, i - 1 ) == i ) {
          settings.http_selects[i] = TRUE;
        }
      }

      save_ini();
    }

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_USECDDB &&
        ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
      {
        BOOL use = WinQueryButtonCheckstate( hwnd, CB_USECDDB );

        WinEnableControl( hwnd, PB_UPDATE,  use );
        WinEnableControl( hwnd, CB_USEHTTP, use );
        WinEnableControl( hwnd, CB_TRYALL,  use );
        return 0;
      }
      break;

    case WM_UPDATE_DONE:
      tid_update = -1;
      WinEnableControl( hwnd, DID_OK,     TRUE );
      WinEnableControl( hwnd, PB_ADD1,    TRUE );
      WinEnableControl( hwnd, PB_DELETE1, TRUE );
      WinEnableControl( hwnd, PB_ADD2,    TRUE );
      WinEnableControl( hwnd, PB_DELETE2, TRUE );
      WinEnableControl( hwnd, PB_UPDATE,  TRUE );
      WinEnableControl( hwnd, CB_USEHTTP, TRUE );
      WinEnableControl( hwnd, CB_TRYALL,  TRUE );
      WinEnableControl( hwnd, CB_USECDDB, TRUE );
      return 0;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ))
      {
        char buffer[512];

        case DID_OK:
          WinSendMsg( hwnd, WM_SAVE, 0, 0 );
        case DID_CANCEL:
          break;

        case PB_ADD1:
          WinQueryDlgItemText( hwnd, EF_NEWSERVER, sizeof( buffer ), buffer );
          if( *buffer ) {
            lb_add_item( hwnd, LB_CDDBSERVERS, buffer );
          }
          return 0;

        case PB_ADD2:
          WinQueryDlgItemText( hwnd, EF_NEWSERVER, sizeof( buffer ), buffer );
          if( *buffer ) {
            lb_add_item( hwnd, LB_HTTPSERVERS, buffer );
          }
          return 0;

        case PB_DELETE1:
        {
          int nextitem = lb_selected( hwnd, LB_CDDBSERVERS, LIT_FIRST );
          while( nextitem != LIT_NONE ) {
            lb_remove_item( hwnd, LB_CDDBSERVERS, nextitem );
            nextitem = lb_selected( hwnd, LB_CDDBSERVERS, LIT_FIRST );
          }
          return 0;
        }

        case PB_DELETE2:
        {
          int nextitem = lb_selected( hwnd, LB_HTTPSERVERS, LIT_FIRST );
          while( nextitem != LIT_NONE ) {
            lb_remove_item( hwnd, LB_HTTPSERVERS, nextitem );
            nextitem = lb_selected( hwnd, LB_HTTPSERVERS, LIT_FIRST );
          }
          return 0;
        }

        case PB_UPDATE:
          WinEnableControl( hwnd, DID_OK,     FALSE );
          WinEnableControl( hwnd, PB_ADD1,    FALSE );
          WinEnableControl( hwnd, PB_DELETE1, FALSE );
          WinEnableControl( hwnd, PB_ADD2,    FALSE );
          WinEnableControl( hwnd, PB_DELETE2, FALSE );
          WinEnableControl( hwnd, PB_UPDATE,  FALSE );
          WinEnableControl( hwnd, CB_USEHTTP, FALSE );
          WinEnableControl( hwnd, CB_TRYALL,  FALSE );
          WinEnableControl( hwnd, CB_USECDDB, FALSE );
          WinSendMsg( hwnd, WM_SAVE, 0, 0 );

          if(( tid_update = _beginthread( cfg_cddb_update_thread, NULL, 65535, (void*)hwnd )) == -1 ) {
            WinPostMsg( hwnd, WM_UPDATE_DONE, 0, 0 );
          }
          return 0;

      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
int DLLENTRY
plugin_configure( HWND hwnd, HMODULE module )
{
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CDDA, NULL );
  return 0;
}

/* Returns information about plug-in. */
int DLLENTRY
plugin_query( PLUGIN_QUERYPARAM* param )
{
   param->type         = PLUGIN_DECODER;
   param->author       = "Samuel Audet, Dmitry A.Steklenev";
   param->desc         = "CDDA Play 1.20";
   param->configurable = TRUE;

   load_ini();
   return 0;
}

int INIT_ATTRIBUTE
__dll_initialize( void )
{
  if( DosCreateMutexSem( NULL, &mutex, 0, FALSE ) != NO_ERROR ) {
    return 0;
  }

  return 1;
}

int TERM_ATTRIBUTE
__dll_terminate( void )
{
  int  i;
  for( i = 0; i < 128; i++ ) {
    free( settings.cddb_servers[i] );
    free( settings.http_servers[i] );
  }

  DosCloseMutexSem( mutex );
  return 1;
}

#if defined(__IBMC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag       )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return __dll_initialize();
  } else {
    __dll_terminate();
    #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
    #endif
    _CRT_term();
    return 1UL;
  }
}
#endif

