/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 *
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
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


#define  INCL_WIN
#define  INCL_DOS
#define  INCL_ERRORS

#include <utilfct.h>
#include "pm123.h"
#include "dialog.h" // for global eq vars...
#include "iniman.h"
#include "plugman.h"
#include "properties.h"
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void
load_ini( void )
{
  HINI INIhandle;
  BUFSTREAM *b;

  void *outputs_list;
  void *decoders_list;
  void *filters_list;
  void *visuals_list;
  ULONG size;

  cfg = cfg_default;

  if(( INIhandle = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value( INIhandle, cfg.playonload );
    load_ini_value( INIhandle, cfg.autouse );
    load_ini_value( INIhandle, cfg.retainonexit );
    load_ini_value( INIhandle, cfg.retainonstop );
    load_ini_value( INIhandle, cfg.restartonstart );
    load_ini_value( INIhandle, cfg.altnavig );
    load_ini_value( INIhandle, cfg.autoturnaround );
    load_ini_value( INIhandle, cfg.recurse_dnd );
    load_ini_value( INIhandle, cfg.sort_folders );
    load_ini_value( INIhandle, cfg.folders_first );
    load_ini_value( INIhandle, cfg.append_dnd );
    load_ini_value( INIhandle, cfg.append_cmd );
    load_ini_value( INIhandle, cfg.queue_mode );
    load_ini_value( INIhandle, cfg.num_workers );
    load_ini_value( INIhandle, cfg.mode );
    load_ini_value( INIhandle, cfg.font );
    load_ini_value( INIhandle, cfg.trash );
//    load_ini_value( INIhandle, cfg.shf );
//    load_ini_value( INIhandle, cfg.rpt );
    load_ini_value( INIhandle, cfg.floatontop );
    load_ini_value( INIhandle, cfg.playonuse );
    load_ini_value( INIhandle, cfg.scroll );
    load_ini_value( INIhandle, cfg.viewmode );
    load_ini_value( INIhandle, cfg.buff_wait );
    load_ini_value( INIhandle, cfg.buff_size );
    load_ini_value( INIhandle, cfg.buff_fill );
    load_ini_string( INIhandle, cfg.pipe_name, sizeof cfg.pipe_name );
    load_ini_value( INIhandle, cfg.add_recursive );
    load_ini_value( INIhandle, cfg.save_relative );
    load_ini_value( INIhandle, cfg.show_playlist );
    load_ini_value( INIhandle, cfg.show_bmarks );
    load_ini_value( INIhandle, cfg.show_plman );
    load_ini_value( INIhandle, cfg.dock_margin );
    load_ini_value( INIhandle, cfg.dock_windows );
    load_ini_value( INIhandle, cfg.font_skinned );
    load_ini_value( INIhandle, cfg.font_attrs );
    load_ini_value( INIhandle, cfg.font_size );
    load_ini_value( INIhandle, cfg.main );

    load_ini_string( INIhandle, cfg.filedir,  sizeof( cfg.filedir ));
    load_ini_string( INIhandle, cfg.listdir,  sizeof( cfg.listdir ));
    load_ini_string( INIhandle, cfg.savedir,  sizeof( cfg.savedir ));
    load_ini_string( INIhandle, cfg.proxy,    sizeof( cfg.proxy ));
    load_ini_string( INIhandle, cfg.auth,     sizeof( cfg.auth ));
    load_ini_string( INIhandle, cfg.defskin,  sizeof( cfg.defskin ));

    load_ini_data_size( INIhandle, decoders_list, size );
    if( size > 0 && ( decoders_list = malloc( size )) != NULL )
    {
      load_ini_data( INIhandle, decoders_list, size );
      b = open_bufstream( decoders_list, size );
      if( !load_decoders( b )) {
        load_default_decoders();
      }
      close_bufstream( b );
      free( decoders_list );
    } else {
      load_default_decoders();
    }

    load_ini_data_size( INIhandle, outputs_list, size );
    if( size > 0 && ( outputs_list = malloc( size )) != NULL )
    {
      load_ini_data( INIhandle, outputs_list, size );
      b = open_bufstream( outputs_list, size );
      if( !load_outputs( b )) {
        load_default_outputs();
      }
      close_bufstream( b );
      free( outputs_list );
    } else {
      load_default_outputs();
    }

    load_ini_data_size( INIhandle, filters_list, size );
    if( size > 0 && ( filters_list = malloc( size )) != NULL )
    {
      load_ini_data( INIhandle, filters_list, size );
      b = open_bufstream( filters_list, size );
      if( !load_filters( b )) {
        load_default_filters();
      }
      close_bufstream( b );
      free( filters_list );
    } else {
      load_default_filters();
    }

    load_ini_data_size( INIhandle, visuals_list, size );
    if( size > 0 && ( visuals_list = malloc( size )) != NULL )
    {
      load_ini_data( INIhandle, visuals_list, size );
      b = open_bufstream( visuals_list, size );
      if( !load_visuals( b )) {
        load_default_visuals();
      }
      close_bufstream( b );
      free( visuals_list );
    } else {
      load_default_visuals();
    }

    close_ini( INIhandle );
  }
}

void
save_ini( void )
{
  HINI INIhandle;
  BUFSTREAM *b;

  void *outputs_list;
  void *decoders_list;
  void *filters_list;
  void *visuals_list;
  ULONG size;

  if(( INIhandle = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value( INIhandle, cfg.playonload );
    save_ini_value( INIhandle, cfg.autouse );
    save_ini_value( INIhandle, cfg.retainonexit );
    save_ini_value( INIhandle, cfg.retainonstop );
    save_ini_value( INIhandle, cfg.restartonstart );
    save_ini_value( INIhandle, cfg.altnavig );
    save_ini_value( INIhandle, cfg.autoturnaround );
    save_ini_value( INIhandle, cfg.recurse_dnd );
    save_ini_value( INIhandle, cfg.sort_folders );
    save_ini_value( INIhandle, cfg.folders_first );
    save_ini_value( INIhandle, cfg.append_dnd );
    save_ini_value( INIhandle, cfg.append_cmd );
    save_ini_value( INIhandle, cfg.queue_mode );
    save_ini_value( INIhandle, cfg.num_workers );
    save_ini_value( INIhandle, cfg.mode );
    save_ini_value( INIhandle, cfg.font );
    save_ini_value( INIhandle, cfg.trash );
//    save_ini_value( INIhandle, cfg.shf );
//    save_ini_value( INIhandle, cfg.rpt );
    save_ini_value( INIhandle, cfg.floatontop );
    save_ini_value( INIhandle, cfg.playonuse );
    save_ini_value( INIhandle, cfg.scroll );
    save_ini_value( INIhandle, cfg.viewmode );
    save_ini_value( INIhandle, cfg.buff_wait );
    save_ini_value( INIhandle, cfg.buff_size );
    save_ini_value( INIhandle, cfg.buff_fill );
    save_ini_string( INIhandle, cfg.pipe_name );
    save_ini_value( INIhandle, cfg.add_recursive );
    save_ini_value( INIhandle, cfg.save_relative );
    save_ini_value( INIhandle, cfg.show_playlist );
    save_ini_value( INIhandle, cfg.show_bmarks );
    save_ini_value( INIhandle, cfg.show_plman );
    save_ini_value( INIhandle, cfg.dock_windows );
    save_ini_value( INIhandle, cfg.dock_margin );
    save_ini_value( INIhandle, cfg.font_skinned );
    save_ini_value( INIhandle, cfg.font_attrs );
    save_ini_value( INIhandle, cfg.font_size );
    save_ini_value( INIhandle, cfg.main );

    save_ini_string( INIhandle, cfg.filedir );
    save_ini_string( INIhandle, cfg.listdir );
    save_ini_string( INIhandle, cfg.savedir );
    save_ini_string( INIhandle, cfg.proxy );
    save_ini_string( INIhandle, cfg.auth );
    save_ini_string( INIhandle, cfg.defskin );

    b = create_bufstream( 1024 );
    save_decoders( b );
    size = get_buffer_bufstream( b, &decoders_list );
    save_ini_data( INIhandle, decoders_list, size );
    close_bufstream( b );

    b = create_bufstream( 1024 );
    save_outputs( b );
    size = get_buffer_bufstream( b, &outputs_list );
    save_ini_data( INIhandle, outputs_list, size );
    close_bufstream( b );

    b = create_bufstream( 1024 );
    save_filters( b );
    size = get_buffer_bufstream( b, &filters_list );
    save_ini_data( INIhandle, filters_list, size );
    close_bufstream( b );

    b = create_bufstream( 1024 );
    save_visuals( b );
    size = get_buffer_bufstream( b, &visuals_list );
    save_ini_data( INIhandle, visuals_list, size );
    close_bufstream( b );

    close_ini(INIhandle);
  }
}

/* Copies the specified data from one profile to another. */
static BOOL
copy_ini_data( HINI ini_from, char* app_from, char* key_from,
               HINI ini_to,   char* app_to,   char* key_to )
{
  ULONG size;
  PVOID data;
  BOOL  rc = FALSE;

  if( PrfQueryProfileSize( ini_from, app_from, key_from, &size )) {
    data = malloc( size );
    if( data ) {
      if( PrfQueryProfileData( ini_from, app_from, key_from, data, &size )) {
        if( PrfWriteProfileData( ini_to, app_to, key_to, data, size )) {
          rc = TRUE;
        }
      }
      free( data );
    }
  }

  return rc;
}

/* Moves the specified data from one profile to another. */
static BOOL
move_ini_data( HINI ini_from, char* app_from, char* key_from,
               HINI ini_to,   char* app_to,   char* key_to )
{
  if( copy_ini_data( ini_from, app_from, key_from, ini_to, app_to, key_to )) {
    return PrfWriteProfileData( ini_from, app_from, key_from, NULL, 0 );
  } else {
    return FALSE;
  }
}

/* Saves the current size and position of the window specified by hwnd.
   This function will also save the presentation parameters. */
BOOL
save_window_pos( HWND hwnd, int options )
{
  char   key1st[32];
  char   key2st[32];
  char   key3st[32];
  PPIB   ppib;
  SHORT  id   = WinQueryWindowUShort( hwnd, QWS_ID );
  HINI   hini = open_module_ini();
  BOOL   rc   = FALSE;
  SWP    swp;
  POINTL pos[2];

  DosGetInfoBlocks( NULL, &ppib );

  if( hini != NULLHANDLE ) {
    sprintf( key1st, "WIN_%08X_%08lX", id, ppib->pib_ulpid );
    sprintf( key2st, "WIN_%08X", id );
    sprintf( key3st, "POS_%08X", id );

    if( WinStoreWindowPos( "PM123", key1st, hwnd )) {
      if( move_ini_data( HINI_PROFILE, "PM123", key1st, hini, "Positions", key2st )) {
        rc = TRUE;
      }
    }
    if( rc && options & WIN_MAP_POINTS ) {
      if( WinQueryWindowPos( hwnd, &swp )) {
        pos[0].x = swp.x;
        pos[0].y = swp.y;
        pos[1].x = swp.x + swp.cx;
        pos[1].y = swp.y + swp.cy;

        WinMapDlgPoints( hwnd, pos, 2, FALSE );
        rc = PrfWriteProfileData( hini, "Positions", key3st, &pos, sizeof( pos ));
      }
    }
    close_ini( hini );
  }
  return rc;
}

/* Restores the size and position of the window specified by hwnd to
   the state it was in when save_window_pos was last called.
   This function will also restore presentation parameters. */
BOOL
rest_window_pos( HWND hwnd, int options )
{
  char   key1st[32];
  char   key2st[32];
  char   key3st[32];
  PPIB   ppib;
  SHORT  id   = WinQueryWindowUShort( hwnd, QWS_ID );
  HINI   hini = open_module_ini();
  BOOL   rc   = FALSE;
  POINTL pos[2];
  SWP    swp;
  SWP    desktop;
  ULONG  len  = sizeof(pos);

  DosGetInfoBlocks( NULL, &ppib );

  if( hini != NULLHANDLE ) {
    sprintf( key1st, "WIN_%08X_%08lX", id, ppib->pib_ulpid );
    sprintf( key2st, "WIN_%08X", id );
    sprintf( key3st, "POS_%08X", id );

    if( copy_ini_data( hini, "Positions", key2st, HINI_PROFILE, "PM123", key1st )) {
      rc = WinRestoreWindowPos( "PM123", key1st, hwnd );
      PrfWriteProfileData( HINI_PROFILE, "PM123", key1st, NULL, 0 );
    }

    if( rc && options & WIN_MAP_POINTS ) {
      if( PrfQueryProfileData( hini, "Positions", key3st, &pos, &len ))
      {
        WinMapDlgPoints( hwnd, pos, 2, TRUE );
        WinSetWindowPos( hwnd, 0, pos[0].x, pos[0].y,
                         pos[1].x-pos[0].x, pos[1].y-pos[0].y, SWP_MOVE | SWP_SIZE );
      } else {
        rc = FALSE;
      }
    }

    if( rc && WinQueryWindowPos( hwnd, &swp )
           && WinQueryWindowPos( HWND_DESKTOP, &desktop ))
    {
      if( swp.y + swp.cy > desktop.cy )
      {
        swp.y = desktop.cy - swp.cy;
        WinSetWindowPos( hwnd, 0, swp.x, swp.y, 0, 0, SWP_MOVE );
      }
    }

    close_ini( hini );
  }
  return rc;
}

