/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 * Copyright 2004      Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2010 Marcel Mueller
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
#define  INCL_ERRORS
#include <stdio.h>
#include <memory.h>

#include "properties.h"
#include "pm123.h"
#include "gui.h"
#include "dialog.h"
#include "docking.h"
#include "pm123.rc.h"
#include "plugman.h"
#include "plugman_base.h"
#include "controller.h"
#include "copyright.h"
#include "123_util.h"
#include "pipe.h"
#include "filedlg.h"
#include <utilfct.h>
#include <errorstr.h>
#include <inimacro.h>
#include <xio.h>
#include <snprintf.h>
#include <cpp/url123.h>
#include <cpp/container/stringmap.h>
#include <os2.h>


// support xstring
const bool ini_query_xstring(HINI hini, const char* app, const char* key, xstring& dst)
{ ULONG len;
  if (PrfQueryProfileSize(hini, app, key, &len))
  { char* data = dst.allocate(len);
    if (PrfQueryProfileData(hini, app, key, data, &len))
      return true;
    dst = NULL;
  }
  return false;
}


#define  CFG_REFRESH_LIST (WM_USER+1)
#define  CFG_REFRESH_INFO (WM_USER+2)
#define  CFG_GLOB_BUTTON  (WM_USER+3)
#define  CFG_CHANGE       (WM_USER+5)

// The properties!
const amp_cfg cfg_default =
{ "",
  true,
  false,
  true,
  false,
  false,
  CFG_ANAV_SONG,
  true,
  true, // recurse_dnd
  true,
  false,
  false,
  false,
  true,
  2, // num_workers
  1,

  1, // font
  false,
  { sizeof(FATTRS), 0, 0, "WarpSans Bold", 0, 0, 16, 7, 0, 0 },
  9,

  false, // float on top
  CFG_SCROLL_INFINITE,
  true,
  CFG_DISP_ID3TAG,
  9,
  "", // Proxy
  "",
  false,
  128,
  30,
  15,
  "\\PIPE\\PM123",
  true, // dock
  10,
  false,
  30,
  500,
  true,
// state
  "",
  "",
  "",
  CFG_MODE_REGULAR,
  false,
  false,
  false,
  true, // add recursive
  true, // save relative
  { 0, 0,0, 1,1, 0, 0, 0, 0 }
};

amp_cfg cfg = cfg_default;
static HINI INIhandle;
const HINI& amp_hini = INIhandle;

struct ext_pos
{ POINTL pos[2];
  time_t tstmp; // Time stamp when the information has been saved.
};

// Purge outdated ini locations in the profile
static void clean_ini_positions(HINI hini)
{ ULONG size;
  if (!PrfQueryProfileSize(hini, "Positions", NULL, &size))
    return;
  char* names = new char[size+2];
  names[size] = names[size+1] = 0; // ensure termination
  PrfQueryProfileData(hini, "Positions", NULL, names, &size);
  const time_t limit = time(NULL) - cfg.win_pos_max_age * 86400;
  for (char* cp = names; *cp; cp += strlen(cp)+1)
  { if (!memcmp(cp, "POS_", 4))
      continue;
    ext_pos pos;
    pos.tstmp = 0;
    size = sizeof(pos);
    if (PrfQueryProfileData(hini, "Positions", cp, &pos, &size) && pos.tstmp < limit)
    { // Purge this entry
      PrfWriteProfileData(hini, "Positions", cp, NULL, 0);
      memcpy(cp, "WIN", 3);
      PrfWriteProfileData(hini, "Positions", cp, NULL, 0);
    }
  }
  delete names;
}

void
load_ini( void )
{
  xstring tmp;

  cfg = cfg_default;

  load_ini_bool ( INIhandle, cfg.playonload );
  load_ini_bool ( INIhandle, cfg.autouse );
  load_ini_bool ( INIhandle, cfg.retainonexit );
  load_ini_bool ( INIhandle, cfg.retainonstop );
  load_ini_bool ( INIhandle, cfg.restartonstart );
  load_ini_value( INIhandle, cfg.altnavig );
  load_ini_bool ( INIhandle, cfg.autoturnaround );
  load_ini_bool ( INIhandle, cfg.recurse_dnd );
  load_ini_bool ( INIhandle, cfg.sort_folders );
  load_ini_bool ( INIhandle, cfg.folders_first );
  load_ini_bool ( INIhandle, cfg.append_dnd );
  load_ini_bool ( INIhandle, cfg.append_cmd );
  load_ini_bool ( INIhandle, cfg.queue_mode );
  load_ini_value( INIhandle, cfg.num_workers );
  load_ini_value( INIhandle, cfg.num_dlg_workers );
  load_ini_value( INIhandle, cfg.mode );
  load_ini_value( INIhandle, cfg.font );
  load_ini_bool ( INIhandle, cfg.floatontop );
  load_ini_value( INIhandle, cfg.scroll );
  load_ini_value( INIhandle, cfg.viewmode );
  load_ini_value( INIhandle, cfg.max_recall );
  load_ini_value( INIhandle, cfg.buff_wait );
  load_ini_value( INIhandle, cfg.buff_size );
  load_ini_value( INIhandle, cfg.buff_fill );
  load_ini_value( INIhandle, cfg.conn_timeout );
  load_ini_xstring( INIhandle, cfg.pipe_name );
  load_ini_bool ( INIhandle, cfg.add_recursive );
  load_ini_bool ( INIhandle, cfg.save_relative );
  load_ini_bool ( INIhandle, cfg.show_playlist );
  load_ini_bool ( INIhandle, cfg.show_bmarks );
  load_ini_bool ( INIhandle, cfg.show_plman );
  load_ini_value( INIhandle, cfg.dock_margin );
  load_ini_bool ( INIhandle, cfg.dock_windows );
  load_ini_bool ( INIhandle, cfg.win_pos_by_obj );
  load_ini_value( INIhandle, cfg.win_pos_max_age );
  load_ini_value( INIhandle, cfg.insp_autorefresh );
  load_ini_bool ( INIhandle, cfg.insp_autorefresh_on );
  load_ini_bool ( INIhandle, cfg.font_skinned );
  load_ini_value( INIhandle, cfg.font_attrs );
  load_ini_value( INIhandle, cfg.font_size );
  load_ini_value( INIhandle, cfg.main );

  load_ini_xstring( INIhandle, cfg.filedir);
  load_ini_xstring( INIhandle, cfg.listdir);
  load_ini_xstring( INIhandle, cfg.savedir);
  load_ini_string( INIhandle, cfg.proxy,    sizeof( cfg.proxy ));
  load_ini_string( INIhandle, cfg.auth,     sizeof( cfg.auth ));
  load_ini_string( INIhandle, cfg.defskin,  sizeof( cfg.defskin ));

  if (!ini_query_xstring(INIhandle, INI_SECTION, "decoders_list", tmp) || Decoders.Deserialize(tmp) == PluginList::RC_Error)
    Decoders.LoadDefaults();
  if (!ini_query_xstring(INIhandle, INI_SECTION, "outputs_list", tmp) || Outputs.Deserialize(tmp) == PluginList::RC_Error)
    Outputs.LoadDefaults();
  if (!ini_query_xstring(INIhandle, INI_SECTION, "filters_list", tmp) || Filters.Deserialize(tmp) == PluginList::RC_Error)
    Filters.LoadDefaults();
  if (!ini_query_xstring(INIhandle, INI_SECTION, "visuals_list", tmp) || Visuals.Deserialize(tmp) == PluginList::RC_Error)
    Visuals.LoadDefaults();
}

void
save_ini( void )
{
  save_ini_bool ( INIhandle, cfg.playonload );
  save_ini_bool ( INIhandle, cfg.autouse );
  save_ini_bool ( INIhandle, cfg.retainonexit );
  save_ini_bool ( INIhandle, cfg.retainonstop );
  save_ini_bool ( INIhandle, cfg.restartonstart );
  save_ini_value( INIhandle, cfg.altnavig );
  save_ini_bool ( INIhandle, cfg.autoturnaround );
  save_ini_bool ( INIhandle, cfg.recurse_dnd );
  save_ini_bool ( INIhandle, cfg.sort_folders );
  save_ini_bool ( INIhandle, cfg.folders_first );
  save_ini_bool ( INIhandle, cfg.append_dnd );
  save_ini_bool ( INIhandle, cfg.append_cmd );
  save_ini_bool ( INIhandle, cfg.queue_mode );
  save_ini_value( INIhandle, cfg.num_workers );
  save_ini_value( INIhandle, cfg.num_dlg_workers );
  save_ini_value( INIhandle, cfg.mode );
  save_ini_value( INIhandle, cfg.font );
  save_ini_bool ( INIhandle, cfg.floatontop );
  save_ini_value( INIhandle, cfg.scroll );
  save_ini_value( INIhandle, cfg.viewmode );
  save_ini_value( INIhandle, cfg.max_recall );
  save_ini_value( INIhandle, cfg.buff_wait );
  save_ini_value( INIhandle, cfg.buff_size );
  save_ini_value( INIhandle, cfg.buff_fill );
  save_ini_value( INIhandle, cfg.conn_timeout );
  save_ini_xstring( INIhandle, cfg.pipe_name );
  save_ini_bool ( INIhandle, cfg.add_recursive );
  save_ini_bool ( INIhandle, cfg.save_relative );
  save_ini_bool ( INIhandle, cfg.show_playlist );
  save_ini_bool ( INIhandle, cfg.show_bmarks );
  save_ini_bool ( INIhandle, cfg.show_plman );
  save_ini_bool ( INIhandle, cfg.dock_windows );
  save_ini_value( INIhandle, cfg.dock_margin );
  save_ini_bool ( INIhandle, cfg.win_pos_by_obj );
  save_ini_value( INIhandle, cfg.win_pos_max_age );
  save_ini_value( INIhandle, cfg.insp_autorefresh );
  save_ini_bool ( INIhandle, cfg.insp_autorefresh_on );
  save_ini_bool ( INIhandle, cfg.font_skinned );
  save_ini_value( INIhandle, cfg.font_attrs );
  save_ini_value( INIhandle, cfg.font_size );
  save_ini_value( INIhandle, cfg.main );

  save_ini_xstring( INIhandle, cfg.filedir );
  save_ini_xstring( INIhandle, cfg.listdir );
  save_ini_xstring( INIhandle, cfg.savedir );
  save_ini_string( INIhandle, cfg.proxy );
  save_ini_string( INIhandle, cfg.auth );
  save_ini_string( INIhandle, cfg.defskin );

  ini_write_xstring(INIhandle, INI_SECTION, "decoders_list", Decoders.Serialize());
  ini_write_xstring(INIhandle, INI_SECTION, "outputs_list",  Outputs.Serialize());
  ini_write_xstring(INIhandle, INI_SECTION, "filters_list",  Filters.Serialize());
  ini_write_xstring(INIhandle, INI_SECTION, "visuals_list",  Visuals.Serialize());

  clean_ini_positions(INIhandle);
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

/* Saves the current size and position of the window specified by hwnd.
   This function will also save the presentation parameters. */
BOOL
save_window_pos( HWND hwnd, const char* extkey )
{
  char   key1st[32];
  char   key2[16];
  char   key3[300];
  PPIB   ppib;
  PTIB   ptib;
  SHORT  id   = WinQueryWindowUShort( hwnd, QWS_ID );
  BOOL   rc   = FALSE;
  SWP    swp;
  ext_pos pos;

  DEBUGLOG(("save_window_pos(%p{%u}, %s)\n", hwnd, id, extkey ? extkey : "<null>" ));

  DosGetInfoBlocks( &ptib, &ppib );

  sprintf( key1st, "WIN_%08lX_%08lX", ppib->pib_ulpid, ptib->tib_ptib2->tib2_ultid );
  sprintf( key2, "WIN_%08X", id );
  if (extkey && cfg.win_pos_by_obj)
  { strcpy( key3, key2 );
    key3[12] = '_';
    strlcpy( key3+13, extkey, sizeof key3 -13 );
  } else
    *key3 = 0;

  if( !WinStoreWindowPos( "PM123", key1st, hwnd ))
    return false;

  rc = copy_ini_data( HINI_PROFILE, "PM123", key1st, INIhandle, "Positions", key2 );
  if (*key3)
    rc &= copy_ini_data( HINI_PROFILE, "PM123", key1st, INIhandle, "Positions", key3 );
  PrfWriteProfileData( HINI_PROFILE, "PM123", key1st, NULL, 0 );

  if( rc && WinQueryWindowPos( hwnd, &swp )) {
    pos.pos[0].x = swp.x;
    pos.pos[0].y = swp.y;
    pos.pos[1].x = swp.x + swp.cx;
    pos.pos[1].y = swp.y + swp.cy;
    WinMapDlgPoints( hwnd, pos.pos, 2, FALSE );
    time(&pos.tstmp);
    memcpy(key2, "POS", 3);
    rc = PrfWriteProfileData( INIhandle, "Positions", key2, &pos, sizeof( pos ));
    if (*key3)
    { memcpy(key3, "POS", 3);
      rc &= PrfWriteProfileData( INIhandle, "Positions", key3, &pos, sizeof( pos ));
    }
  }
  return rc;
}

/* Restores the size and position of the window specified by hwnd to
   the state it was in when save_window_pos was last called.
   This function will also restore presentation parameters. */
BOOL
rest_window_pos( HWND hwnd, const char* extkey )
{
  char   key1st[32];
  char   key2[16];
  char   key3[300];
  PPIB   ppib;
  PTIB   ptib;
  SHORT  id   = WinQueryWindowUShort( hwnd, QWS_ID );
  BOOL   rc   = FALSE;
  POINTL pos[2];
  SWP    swp;
  SWP    desktop;
  ULONG  len  = sizeof(pos);

  DEBUGLOG(("rest_window_pos(%p{%u}, %s)\n", hwnd, id, extkey ? extkey : "<null>" ));

  DosGetInfoBlocks( &ptib, &ppib );

  if (!WinQueryWindowPos( hwnd, &swp ))
    return FALSE;

  sprintf( key1st, "WIN_%08lX_%08lX", ppib->pib_ulpid, ptib->tib_ptib2->tib2_ultid );
  sprintf( key2, "WIN_%08X", id );
  if (extkey && cfg.win_pos_by_obj)
  { strcpy( key3, key2 );
    key3[12] = '_';
    strlcpy( key3+13, extkey, sizeof key3 -13 );
  } else
    *key3 = 0;

  if( (*key3 && copy_ini_data( INIhandle, "Positions", key3, HINI_PROFILE, "PM123", key1st ))
    || copy_ini_data( INIhandle, "Positions", key2, HINI_PROFILE, "PM123", key1st ) ) {
    rc = WinRestoreWindowPos( "PM123", key1st, hwnd );
    PrfWriteProfileData( HINI_PROFILE, "PM123", key1st, NULL, 0 );
  }
  if (!rc)
    return FALSE;

  // rc = TRUE
  if (*key3)
  { memcpy(key3, "POS", 3);
    if (!PrfQueryProfileData( INIhandle, "Positions", key3, &pos, &len ) || len < sizeof pos)
      *key3 = 0; // not found
  }
  if (!*key3)
  { memcpy(key2, "POS", 3);
    rc = PrfQueryProfileData( INIhandle, "Positions", key2, &pos, &len ) && len >= sizeof pos;
  }
  if (rc)
  { WinMapDlgPoints( hwnd, pos, 2, TRUE );
    if (!extkey || *key3)
    { swp.x = pos[0].x;
      swp.y = pos[0].y;
    }
    swp.cx = pos[1].x - pos[0].x;
    swp.cy = pos[1].y - pos[0].y;
  } else {
    rc = FALSE;
  }

  if( WinQueryWindowPos( HWND_DESKTOP, &desktop ))
  { // clip right
    if( swp.x > desktop.cx - 8 )
      swp.x = desktop.cx - 8;
    // clip left
    else if( swp.x + swp.cx < 8 )
      swp.x = 8 - swp.cx;
    // clip top
    if( swp.y + swp.cy > desktop.cy )
      swp.y = desktop.cy - swp.cy;
    // clip bottom
    else if( swp.y + swp.cy < 8 )
      swp.y = 8 - swp.cy;
  }

  WinSetWindowPos( hwnd, 0, swp.x, swp.y, swp.cx, swp.cy, SWP_MOVE|SWP_SIZE );
  return rc;
}


// migrate plug-in configuration
static void migrate_ini(const char* inipath, const char* app)
{
  ULONG len;
  char module[16];
  snprintf(module, sizeof module, "%s.dll", app);
  if (PrfQueryProfileSize(INIhandle, module, NULL, &len) && len)
    return; // Data already there

  xstring inifile = xstring::sprintf("%s\\%s.ini", inipath, app);
  HINI hini = PrfOpenProfile(amp_player_hab(), inifile);
  if (hini == NULLHANDLE)
    return;

  if (PrfQueryProfileSize(hini, "Settings", NULL, &len))
  { char* names = (char*)alloca(len);
    if (PrfQueryProfileData(hini, "Settings", NULL, names, &len))
    { while (*names)
      { char buf[1024];
        len = sizeof buf;
        if (PrfQueryProfileData(hini, "Settings", names, buf, &len))
          PrfWriteProfileData(INIhandle, module, names, buf, len);
        names += strlen(names)+1;
      }
    }
  }

  close_ini(hini);
}

/* Initialize properties, called from main. */
void cfg_init()
{
  // Open profile
  xstring inipath = amp_startpath + "\\PM123.INI"; // TODO: command line option
  INIhandle = PrfOpenProfile(amp_player_hab(), inipath);
  PMASSERT(INIhandle);

  load_ini();
  // set proxy and buffer settings statically in the xio library, not that nice, but working.
  char buffer[1024];
  char* cp = strchr(cfg.proxy, ':');
  if (cp == NULL)
  { xio_set_http_proxy_host( cfg.proxy );
  } else
  { size_t l = cp - cfg.proxy +1;
    strlcpy( buffer, cfg.proxy, min( l, sizeof buffer ));
    xio_set_http_proxy_host( buffer );
    xio_set_http_proxy_port( atoi(cp+1) );
  }
  cp = strchr(cfg.auth, ':');
  if (cp == NULL)
  { xio_set_http_proxy_user( cfg.auth );
  } else
  { size_t l = cp - cfg.proxy +1;
    strlcpy( buffer, cfg.proxy, min( l, sizeof buffer ));
    xio_set_http_proxy_user( buffer );
    xio_set_http_proxy_pass( cp +1 );
  }
  xio_set_buffer_size( cfg.buff_size * 1024 );
  xio_set_buffer_wait( cfg.buff_wait );
  xio_set_buffer_fill( cfg.buff_fill );
  xio_set_connect_timeout( cfg.conn_timeout );

  migrate_ini(amp_startpath, "analyzer");
  migrate_ini(amp_startpath, "mpg123");
  migrate_ini(amp_startpath, "os2audio");
  migrate_ini(amp_startpath, "realeq");
}

void cfg_uninit()
{ if (!PrfCloseProfile(INIhandle))
  { char buf[1024];
    os2pm_strerror(buf, sizeof(buf));
    amp_error(HWND_DESKTOP, "PM123 failed to write its profile: %s", buf);
  }
}

/* Processes messages of the setings page of the setup notebook. */
static MRESULT EXPENTRY
cfg_settings1_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_INITDLG:
      do_warpsans( hwnd );
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      break;

    case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);

      WinCheckButton( hwnd, CB_PLAYONLOAD,    cfg.playonload   );
      WinCheckButton( hwnd, CB_RETAINONEXIT,  cfg.retainonexit );
      WinCheckButton( hwnd, CB_RETAINONSTOP,  cfg.retainonstop );
      WinCheckButton( hwnd, CB_RESTARTONSTART,cfg.restartonstart);

      WinCheckButton( hwnd, CB_TURNAROUND,    cfg.autoturnaround );
      WinCheckButton( hwnd, RB_SONGONLY +     cfg.altnavig, TRUE );

      return 0;
    }

    case CFG_GLOB_BUTTON:
    { const amp_cfg* data = &cfg;
      switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        data = &cfg_default;
       case PB_UNDO:
        WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(data), 0);
      }
      return 0;
    }

    case WM_DESTROY:
      cfg.playonload  = WinQueryButtonCheckstate( hwnd, CB_PLAYONLOAD   );
      cfg.retainonexit= WinQueryButtonCheckstate( hwnd, CB_RETAINONEXIT );
      cfg.retainonstop= WinQueryButtonCheckstate( hwnd, CB_RETAINONSTOP );
      cfg.restartonstart= WinQueryButtonCheckstate( hwnd, CB_RESTARTONSTART);

      cfg.autoturnaround = WinQueryButtonCheckstate( hwnd, CB_TURNAROUND );
      if (WinQueryButtonCheckstate( hwnd, RB_SONGONLY ))
        cfg.altnavig = CFG_ANAV_SONG;
      else if (WinQueryButtonCheckstate( hwnd, RB_SONGTIME ))
        cfg.altnavig = CFG_ANAV_SONGTIME;
      else if (WinQueryButtonCheckstate( hwnd, RB_TIMEONLY ))
        cfg.altnavig = CFG_ANAV_TIME;

    case WM_COMMAND:
    case WM_CONTROL:
      return 0;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

static MRESULT EXPENTRY
cfg_settings2_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ switch( msg ) {
    case WM_INITDLG:
      do_warpsans( hwnd );
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      break;

    case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      WinCheckButton( hwnd, CB_AUTOUSEPL,     cfg.autouse      );
      WinCheckButton( hwnd, CB_RECURSEDND,    cfg.recurse_dnd  );
      WinCheckButton( hwnd, CB_SORTFOLDERS,   cfg.sort_folders );
      WinCheckButton( hwnd, CB_FOLDERSFIRST,  cfg.folders_first);
      WinCheckButton( hwnd, CB_AUTOAPPENDDND, cfg.append_dnd   );
      WinCheckButton( hwnd, CB_AUTOAPPENDCMD, cfg.append_cmd   );
      WinCheckButton( hwnd, CB_QUEUEMODE,     cfg.queue_mode   );
      return 0;
    }

    case CFG_GLOB_BUTTON:
    { const amp_cfg* data = &cfg;
      switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        data = &cfg_default;
       case PB_UNDO:
        PMRASSERT(WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(data), 0));
      }
      return 0;
    }

    case WM_CONTROL:
    case WM_COMMAND:
      return 0;

    case WM_DESTROY:
    {
      cfg.autouse      = WinQueryButtonCheckstate( hwnd, CB_AUTOUSEPL    );
      cfg.recurse_dnd  = WinQueryButtonCheckstate( hwnd, CB_RECURSEDND   );
      cfg.sort_folders = WinQueryButtonCheckstate( hwnd, CB_SORTFOLDERS  );
      cfg.folders_first= WinQueryButtonCheckstate( hwnd, CB_FOLDERSFIRST );
      cfg.append_dnd   = WinQueryButtonCheckstate( hwnd, CB_AUTOAPPENDDND);
      cfg.append_cmd   = WinQueryButtonCheckstate( hwnd, CB_AUTOAPPENDCMD);
      cfg.queue_mode   = WinQueryButtonCheckstate( hwnd, CB_QUEUEMODE    );

      return 0;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

static MRESULT EXPENTRY
cfg_iosettings_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ switch( msg ) {
    case WM_INITDLG:
      do_warpsans( hwnd );
      PMRASSERT(sb_setnumlimits( hwnd, SB_TIMEOUT,    1,  300, 4 ));
      PMRASSERT(sb_setnumlimits( hwnd, SB_BUFFERSIZE, 0, 2048, 4 ));
      PMRASSERT(sb_setnumlimits( hwnd, SB_FILLBUFFER, 1,  100, 4 ));

      PMRASSERT(sb_setnumlimits( hwnd, SB_NUMWORKERS, 1,    9, 1 ));
      PMRASSERT(sb_setnumlimits( hwnd, SB_DLGWORKERS, 0,    9, 1 ));

      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      break;

    case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      char buffer[1024];
      const char* cp;
      size_t l;

      WinSetDlgItemText( hwnd, EF_PIPE, cfg.pipe_name );

      // proxy
      cp = strchr(cfg.proxy, ':');
      if (cp == NULL)
      { l = strlen(cfg.proxy);
        cp = cfg.proxy + l;
        ++l;
      } else
      { ++cp;
        l = cp - cfg.proxy;
      }
      strlcpy( buffer, cfg.proxy, min( l, sizeof buffer ));
      WinSetDlgItemText( hwnd, EF_PROXY_HOST, buffer );
      WinSetDlgItemText( hwnd, EF_PROXY_PORT, cp );
      cp = strchr(cfg.auth, ':');
      if (cp == NULL)
      { l = strlen(cfg.auth);
        cp = cfg.proxy + l;
        ++l;
      } else
      { ++cp;
        l = cp - cfg.auth;
      }
      strlcpy( buffer, cfg.auth, min( l, sizeof buffer ));
      WinSetDlgItemText( hwnd, EF_PROXY_USER, buffer );
      WinSetDlgItemText( hwnd, EF_PROXY_PASS, cp );

      WinCheckButton   ( hwnd, CB_FILLBUFFER, cfg.buff_wait );

      WinSendDlgItemMsg( hwnd, SB_TIMEOUT,    SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.conn_timeout ), 0 );
      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.buff_size ), 0 );
      WinSendDlgItemMsg( hwnd, SB_FILLBUFFER, SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.buff_fill ), 0 );

      WinSendDlgItemMsg( hwnd, SB_NUMWORKERS, SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.num_workers ), 0 );
      WinSendDlgItemMsg( hwnd, SB_DLGWORKERS, SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.num_dlg_workers ), 0 );
      return 0;
    }

    case CFG_GLOB_BUTTON:
    { const amp_cfg* data = &cfg;
      switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        data = &cfg_default;
       case PB_UNDO:
        WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(data), 0);
      }
      return 0;
    }

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_FILLBUFFER &&
        ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
      {
        BOOL fill = WinQueryButtonCheckstate( hwnd, CB_FILLBUFFER );

        WinEnableControl( hwnd, SB_FILLBUFFER, fill );
      }
    case WM_COMMAND:
      return 0;

    case WM_DESTROY:
    {
      size_t i;

      char buf[_MAX_PATH];
      *buf = 0;
      WinQueryDlgItemText( hwnd, EF_PIPE, sizeof buf, buf );
      if (cfg.pipe_name.compareToI(buf) != 0)
      { cfg.pipe_name = buf;
      }

      PMRASSERT(WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_QUERYVALUE, MPFROMP( &cfg.buff_size ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE )));
      PMRASSERT(WinSendDlgItemMsg( hwnd, SB_FILLBUFFER, SPBM_QUERYVALUE, MPFROMP( &cfg.buff_fill ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE )));
      PMRASSERT(WinSendDlgItemMsg( hwnd, SB_TIMEOUT, SPBM_QUERYVALUE, MPFROMP( &cfg.conn_timeout ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE )));

      WinQueryDlgItemText( hwnd, EF_PROXY_HOST, sizeof cfg.proxy, cfg.proxy );
      xio_set_http_proxy_host( cfg.proxy );
      i = strlen( cfg.proxy );
      if ( i < sizeof cfg.proxy - 1 ) {
        cfg.proxy[i++] = ':'; // delimiter
        WinQueryDlgItemText( hwnd, EF_PROXY_PORT, sizeof cfg.proxy - i, cfg.proxy + i );
        xio_set_http_proxy_port( atoi( cfg.proxy + i ));
        if ( cfg.proxy[i] == 0 )
          cfg.proxy[i-1] = 0; // remove delimiter
      }

      WinQueryDlgItemText( hwnd, EF_PROXY_USER, sizeof cfg.auth, cfg.auth );
      xio_set_http_proxy_user( cfg.auth );
      i = strlen( cfg.auth );
      if ( i < sizeof cfg.auth - 1 ) {
        cfg.auth[i++] = ':'; // delimiter
        WinQueryDlgItemText( hwnd, EF_PROXY_PASS, sizeof cfg.auth - i, cfg.auth + i );
        xio_set_http_proxy_pass( cfg.auth + i );
        if ( cfg.auth[i] == 0 )
          cfg.auth[i-1] = 0; // remove delimiter
      }

      cfg.buff_wait = WinQueryButtonCheckstate( hwnd, CB_FILLBUFFER );

      xio_set_buffer_size( cfg.buff_size * 1024 );
      xio_set_buffer_wait( cfg.buff_wait );
      xio_set_buffer_fill( cfg.buff_fill );
      xio_set_connect_timeout( cfg.conn_timeout );

      PMRASSERT(WinSendDlgItemMsg( hwnd, SB_NUMWORKERS, SPBM_QUERYVALUE, MPFROMP( &cfg.num_workers ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE )));
      PMRASSERT(WinSendDlgItemMsg( hwnd, SB_DLGWORKERS, SPBM_QUERYVALUE, MPFROMP( &cfg.num_dlg_workers ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE )));

      return 0;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the display page of the setup notebook. */
static MRESULT EXPENTRY
cfg_display1_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  static FATTRS  font_attrs;
  static LONG    font_size;

  switch( msg ) {
    case WM_INITDLG:
      do_warpsans( hwnd );
      PMRASSERT(sb_setnumlimits( hwnd, SB_DOCK, 0, 30, 2 ));
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      break;

    case CFG_CHANGE:
    {
      if (mp1)
      { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
        WinCheckButton   ( hwnd, CB_DOCK, cfg.dock_windows );
        WinEnableControl ( hwnd, SB_DOCK, cfg.dock_windows );
        WinSendDlgItemMsg( hwnd, SB_DOCK, SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.dock_margin ), 0 );
        WinCheckButton   ( hwnd, CB_SAVEWNDPOSBYOBJ, cfg.win_pos_by_obj );

        // load GUI
        WinCheckButton( hwnd, RB_DISP_FILENAME   + cfg.viewmode, TRUE );
        WinCheckButton( hwnd, RB_SCROLL_INFINITE + cfg.scroll,   TRUE );
        WinCheckButton( hwnd, CB_SCROLL_AROUND,    cfg.scroll_around  );
        WinCheckButton( hwnd, CB_USE_SKIN_FONT,    cfg.font_skinned   );
        WinEnableControl( hwnd, PB_FONT_SELECT, !cfg.font_skinned );
        WinEnableControl( hwnd, ST_FONT_SAMPLE, !cfg.font_skinned );

        font_attrs = cfg.font_attrs;
        font_size  = cfg.font_size;
      }
      // change sample font
      xstring font_name  = amp_font_attrs_to_string( font_attrs, font_size );
      WinSetDlgItemText( hwnd, ST_FONT_SAMPLE, font_name );
      WinSetPresParam  ( WinWindowFromID( hwnd, ST_FONT_SAMPLE ), PP_FONTNAMESIZE, font_name.length() +1, (PVOID)font_name.cdata() );
      return 0;
    }

    case CFG_GLOB_BUTTON:
    { const amp_cfg* data = &cfg;
      switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        data = &cfg_default;
       case PB_UNDO:
        WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(data), 0);
      }
      return 0;
    }

    case WM_COMMAND:
      if( COMMANDMSG( &msg )->cmd == PB_FONT_SELECT )
      { char font_family[FACESIZE];
        FONTDLG fontdialog = { sizeof( fontdialog ) };
        fontdialog.hpsScreen      = WinGetScreenPS( HWND_DESKTOP );
        fontdialog.pszFamilyname  = font_family;
        fontdialog.usFamilyBufLen = sizeof font_family;
        fontdialog.pszTitle       = "PM123 scroller font";
        fontdialog.pszPreview     = "128 kb/s, 44.1 kHz, Joint-Stereo";
        fontdialog.fl             = FNTS_CENTER | FNTS_RESETBUTTON | FNTS_INITFROMFATTRS;
        fontdialog.clrFore        = CLR_BLACK;
        fontdialog.clrBack        = CLR_WHITE;
        fontdialog.fAttrs         = font_attrs;
        fontdialog.fxPointSize    = MAKEFIXED( font_size, 0 );

        WinFontDlg( HWND_DESKTOP, hwnd, &fontdialog );

        if( fontdialog.lReturn == DID_OK )
        {
          font_attrs = fontdialog.fAttrs;
          font_size  = fontdialog.fxPointSize >> 16;
          WinPostMsg( hwnd, CFG_CHANGE, 0, 0 );
        }
      }
      return 0;

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case CB_USE_SKIN_FONT:
        if ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED )
        {
          BOOL use = WinQueryButtonCheckstate( hwnd, CB_USE_SKIN_FONT );
          WinEnableControl( hwnd, PB_FONT_SELECT, !use );
          WinEnableControl( hwnd, ST_FONT_SAMPLE, !use );
        }
        break;

       case CB_DOCK:
        if ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED )
        {
          BOOL use = WinQueryButtonCheckstate( hwnd, CB_DOCK );
          WinEnableControl ( hwnd, SB_DOCK, use );
        }
      }
      return 0;

    case WM_DESTROY:
    {
      cfg.dock_windows  = WinQueryButtonCheckstate( hwnd, CB_DOCK );
      PMRASSERT(WinSendDlgItemMsg( hwnd, SB_DOCK, SPBM_QUERYVALUE, MPFROMP( &cfg.dock_margin ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE )));
      cfg.win_pos_by_obj= WinQueryButtonCheckstate( hwnd, CB_SAVEWNDPOSBYOBJ );

      cfg.scroll        = rb_selected( hwnd, RB_SCROLL_INFINITE ) - RB_SCROLL_INFINITE;
      cfg.scroll_around = WinQueryButtonCheckstate( hwnd, CB_SCROLL_AROUND );
      cfg.viewmode      = rb_selected( hwnd, RB_DISP_FILENAME ) - RB_DISP_FILENAME;

      cfg.font_skinned  = WinQueryButtonCheckstate( hwnd, CB_USE_SKIN_FONT );
      cfg.font_size     = font_size;
      cfg.font_attrs    = font_attrs;

      GUI::RefreshDisplay();
      GUI::RearrangeDocking();
      return 0;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


struct PluginContext
{ // Head
  const size_t Size;        // structure size
  // Configuration
  PluginList*const List;    // List to visualize
  PluginList*const List2;   // Secondary List for visual Plug-Ins
  const int    RecentLevel; // Most recent interface level
  enum CtrlFlags            // Cotrol flags
  { CF_None    = 0,
    CF_List1   = 1  // List is of type PluginList1
  } const      Flags;
  bool (PluginContext::*enable_hook)(size_t i, bool enable); // Hook for Enable/Disable button
  xstring      UndoCfg;

  // Working set
  HWND         Hwnd;

  PluginContext(PluginList* list1, PluginList* list2, int level, CtrlFlags flags);

  void         RefreshList();
  Plugin*      RefreshInfo(const size_t i);
  ULONG        AddPlugin();
  bool         Configure(size_t i);
  bool         SetParams(size_t i);

  bool         decoder_enable_hook(size_t i, bool enable);
  bool         filter_enable_hook(size_t i, bool enable);
  bool         output_enable_hook(size_t i, bool enable);
  bool         visual_enable_hook(size_t i, bool enable);
};
FLAGSATTRIBUTE(PluginContext::CtrlFlags);

PluginContext::PluginContext(PluginList* list1, PluginList* list2, int level, CtrlFlags flags)
: Size(sizeof(PluginContext)),
  List(list1),
  List2(list2),
  RecentLevel(level),
  Flags(flags),
  enable_hook(NULL),
  UndoCfg(list1->Serialize())
{ switch (List->Type)
  {case PLUGIN_DECODER:
    enable_hook = &PluginContext::decoder_enable_hook;
    break;
   case PLUGIN_FILTER:
    enable_hook = &PluginContext::filter_enable_hook;
    break;
   case PLUGIN_OUTPUT:
    enable_hook = &PluginContext::output_enable_hook;
    break;
   case PLUGIN_VISUAL:
    enable_hook = &PluginContext::visual_enable_hook;
    break;
   default:
    ASSERT(1==2);
  }
}

void PluginContext::RefreshList()
{ DEBUGLOG(("PluginContext::RefreshList()\n"));
  HWND lb = WinWindowFromID(Hwnd, LB_PLUGINS);
  PMASSERT(lb != NULLHANDLE);
  WinSendMsg(lb, LM_DELETEALL, 0, 0);

  Plugin*const* ppp;
  for (ppp = List->begin(); ppp != List->end(); ++ppp)
  { const char* cp = (*ppp)->GetModuleName().cdata() + (*ppp)->GetModuleName().length();
    while (cp != (*ppp)->GetModuleName().cdata() && cp[-1] != '\\' && cp[-1] != ':' && cp[-1] != '/')
      --cp;
    WinSendMsg(lb, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(cp));
  }
  if (List2 == NULL)
    return;
  for (ppp = List2->begin(); ppp != List2->end(); ++ppp)
  { const char* cp = (*ppp)->GetModuleName().cdata() + (*ppp)->GetModuleName().length();
    while (cp != (*ppp)->GetModuleName().cdata() && cp[-1] != '\\' && cp[-1] != ':' && cp[-1] != '/')
      --cp;
    WinSendMsg(lb, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP((xstring(cp)+" (Skin)").cdata()));
  }
}

Plugin* PluginContext::RefreshInfo(const size_t i)
{ DEBUGLOG(("PluginContext::RefreshInfo(%i)\n", i));
  Plugin* pp = NULL;
  if (i >= List->size())
  { // The following functions give an error if no such buttons. This is ignored.
    WinSetDlgItemText(Hwnd, PB_PLG_ENABLE, "~Enable");
    WinEnableControl (Hwnd, PB_PLG_UNLOAD, FALSE);
    WinEnableControl (Hwnd, PB_PLG_UP,     FALSE);
    WinEnableControl (Hwnd, PB_PLG_DOWN,   FALSE);
    WinEnableControl (Hwnd, PB_PLG_ENABLE, FALSE);
    WinEnableControl (Hwnd, PB_PLG_ACTIVATE, FALSE);
    // decoder specific stuff
    if (List->Type == PLUGIN_DECODER)
    { HWND ctrl = WinWindowFromID(Hwnd, ML_DEC_FILETYPES);
      WinSetWindowText(ctrl, "");
      WinEnableWindow (ctrl, FALSE);
      ctrl = WinWindowFromID(Hwnd, CB_DEC_TRYOTHER);
      WinSendMsg      (ctrl, BM_SETCHECK, MPFROMSHORT(FALSE), 0);
      WinEnableWindow (ctrl, FALSE);
      ctrl = WinWindowFromID(Hwnd, CB_DEC_SERIALIZE);
      WinSendMsg      (ctrl, BM_SETCHECK, MPFROMSHORT(FALSE), 0);
      WinEnableWindow (ctrl, FALSE);
      WinEnableControl(Hwnd, PB_PLG_SET, FALSE);
    }
  } else
  { pp = (*List)[i];
    WinSetDlgItemText(Hwnd, PB_PLG_ENABLE, pp->GetEnabled() ? "Disabl~e" : "~Enable");
    WinEnableControl (Hwnd, PB_PLG_UNLOAD, TRUE);
    WinEnableControl (Hwnd, PB_PLG_UP,     i > 0);
    WinEnableControl (Hwnd, PB_PLG_DOWN,   i < List->size()-1);
    WinEnableControl (Hwnd, PB_PLG_ENABLE, TRUE);
    if (Flags & CF_List1)
      WinEnableControl (Hwnd, PB_PLG_ACTIVATE, ((PluginList1*)List)->GetActive() != i);
    // decode specific stuff
    if (List->Type == PLUGIN_DECODER)
    { stringmap_own sm(20);
      pp->GetParams(sm);
      // TODO: Decoder supported file types vs. user file types.
      { const vector<const DECODER_FILETYPE>& filetypes = ((Decoder*)pp)->GetFileTypes();
        size_t len = 0;
        for (const DECODER_FILETYPE*const* ftp = filetypes.begin(); ftp != filetypes.end(); ++ftp)
          if ((*ftp)->eatype)
            len += strlen((*ftp)->eatype) +1;
        char* cp = (char*)alloca(len);
        char* cp2 = cp;
        for (const DECODER_FILETYPE*const* ftp = filetypes.begin(); ftp != filetypes.end(); ++ftp)
          if ((*ftp)->eatype)
          { strcpy(cp2, (*ftp)->eatype);
            cp2 += strlen((*ftp)->eatype);
            *cp2++ = '\n';
          }
        if (cp2 != cp)
          cp2[-1] = 0;
        else
          cp = "";
        HWND ctrl = WinWindowFromID(Hwnd, ML_DEC_FILETYPES);
        WinSetWindowText(ctrl, cp);
        WinEnableWindow (ctrl, TRUE);
      }
      stringmapentry* smp; // = sm.find("filetypes");
      smp = sm.find("tryothers");
      bool* b = smp && smp->Value ? url123::parseBoolean(smp->Value) : NULL;
      HWND ctrl = WinWindowFromID(Hwnd, CB_DEC_TRYOTHER);
      WinSendMsg      (ctrl, BM_SETCHECK, MPFROMSHORT(b && *b), 0);
      WinEnableWindow (ctrl, !!b);
      smp = sm.find("serializeinfo");
      b = smp && smp->Value ? url123::parseBoolean(smp->Value) : NULL;
      ctrl = WinWindowFromID(Hwnd, CB_DEC_SERIALIZE);
      WinSendMsg      (ctrl, BM_SETCHECK, MPFROMSHORT(b && *b), 0);
      WinEnableWindow (ctrl, !!b);
      WinEnableControl(Hwnd, PB_PLG_SET, FALSE);
    }
  }
  if (pp == NULL && i >= 0 && List2 && i < List->size() + List2->size())
    pp = (*List2)[i - List->size()];
  if (pp == NULL)
  { WinSetDlgItemText(Hwnd, ST_PLG_AUTHOR, "");
    WinSetDlgItemText(Hwnd, ST_PLG_DESC,   "");
    WinSetDlgItemText(Hwnd, ST_PLG_LEVEL,  "");
    WinEnableControl (Hwnd, PB_PLG_CONFIG, FALSE);
  } else
  { char buffer[64];
    const PLUGIN_QUERYPARAM& params = pp->GetModule().GetParams();
    WinSetDlgItemText(Hwnd, ST_PLG_AUTHOR, params.author);
    WinSetDlgItemText(Hwnd, ST_PLG_DESC,   params.desc);
    snprintf(buffer, sizeof buffer,        "Interface level %i%s",
      params.interface, params.interface >= RecentLevel ? "" : " (virtualized)");
    WinSetDlgItemText(Hwnd, ST_PLG_LEVEL,  buffer);
    WinEnableControl (Hwnd, PB_PLG_CONFIG, params.configurable && pp->GetEnabled());
  }
  return pp;
}

ULONG PluginContext::AddPlugin()
{
  FILEDLG filedialog;
  ULONG   rc = 0;
  APSZ    ftypes[] = {{ "PM123 Plug-in (*.DLL)" }, { 0 }};

  memset(&filedialog, 0, sizeof(FILEDLG));

  filedialog.cbSize         = sizeof(FILEDLG);
  filedialog.fl             = FDS_CENTER|FDS_OPEN_DIALOG;
  filedialog.pszTitle       = "Load a plug-in";
  filedialog.papszITypeList = ftypes;
  char type[_MAX_PATH]      = "PM123 Plug-in";
  filedialog.pszIType       = type;

  strlcpy(filedialog.szFullFile, amp_startpath, sizeof filedialog.szFullFile);
  amp_file_dlg(HWND_DESKTOP, Hwnd, &filedialog);

  if (filedialog.lReturn == DID_OK)
  { rc = Plugin::Deserialize(filedialog.szFullFile, List->Type);
    /* TODO: still required?
    if (rc & PLUGIN_VISUAL)
      vis_init(List->size()-1);*/
    if (rc & PLUGIN_FILTER && Ctrl::IsPlaying())
      amp_info(Hwnd, "This filter will only be enabled after playback stops.");
    if (rc)
      WinSendMsg(Hwnd, CFG_REFRESH_LIST, 0, MPFROMSHORT(List->size()-1));
  }
  return rc;
}

bool PluginContext::Configure(size_t i)
{ Plugin* pp = NULL;
  if (i < List->size())
    pp = (*List)[i];
  else if (List2 && i < List->size() + List2->size())
    pp = (*List2)[i - List->size()];

  if (pp)
  { pp->GetModule().Config(Hwnd);
    return true;
  } else
    return false;
}

bool PluginContext::SetParams(size_t i)
{ if (i >= List->size())
    return false;
  Plugin* pp = (*List)[i];

  if (List->Type == PLUGIN_DECODER)
  { HWND ctrl = WinWindowFromID(Hwnd, ML_DEC_FILETYPES);
    ULONG len = WinQueryWindowTextLength(ctrl) +1;
    char* filetypes = (char*)alloca(len);
    WinQueryWindowText(ctrl, len, filetypes);
    char* cp2 = filetypes;
    while ( *cp2)
    { switch (*cp2)
      {case '\r':
        strcpy(cp2, cp2+1);
        continue;
       case '\n':
        *cp2 = ';';
      }
      ++cp2;
    }
    pp->SetParam("filetypes", filetypes);
    pp->SetParam("tryothers", WinQueryButtonCheckstate(Hwnd, CB_DEC_TRYOTHER) ? "1" : "0");
    pp->SetParam("serializeinfo", WinQueryButtonCheckstate(Hwnd, CB_DEC_SERIALIZE) ? "1" : "0");
  }
  WinEnableControl(Hwnd, PB_PLG_SET, FALSE);
  return true;
}

bool PluginContext::visual_enable_hook(size_t i, bool enable)
{ /* TODO: required?
  if (enable)
    vis_init(i);
  else
    vis_deinit(i);*/
  return true;
}

bool PluginContext::decoder_enable_hook(size_t i, bool enable)
{ // This query is non-atomic, but nothing strange will happen anyway.
  if (i > Decoders.size())
    return false;
  if (!enable && Decoders[i]->IsInitialized())
  { amp_error(Hwnd, "Cannot disable currently in use decoder.");
    return false;
  }
  return true;
}

bool PluginContext::output_enable_hook(size_t i, bool enable)
{ if (Ctrl::IsPlaying())
  { amp_error(Hwnd, "Cannot change active output while playing.");
    return false;
  }
  return true;
}

bool PluginContext::filter_enable_hook(size_t i, bool enable)
{ if (Ctrl::IsPlaying() && i < List->size() && (*List)[i]->IsInitialized())
    amp_info(Hwnd, enable
      ? "This filter will only be enabled after playback stops."
      : "This filter will only be disabled after playback stops.");
  return true;
}

/* Processes messages of the plug-ins pages of the setup notebook. */
static MRESULT EXPENTRY
cfg_config_dlg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("cfg_config_dlg_proc(%p, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  PluginContext* context = (PluginContext*)WinQueryWindowULong(hwnd, QWL_USER);
  SHORT i;

  switch( msg )
  { case CFG_REFRESH_LIST:
      context->RefreshList();
      lb_select(hwnd, LB_PLUGINS, SHORT1FROMMP(mp2));
      return 0;

    case CFG_REFRESH_INFO:
      context->RefreshInfo(SHORT1FROMMP(mp2));
      return 0;

    case CFG_GLOB_BUTTON:
    { switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        if (Ctrl::IsPlaying())
          amp_error(hwnd, "Cannot load defaults while playing.");
        else
          context->List->LoadDefaults();
        WinPostMsg(hwnd, CFG_REFRESH_LIST, 0, MPFROMSHORT(LIT_NONE));
        break;
       case PB_UNDO:
        // TODO: The used plug-ins may have changed meanwhile causing Deserialize to fail.
        if (context->List->Deserialize(context->UndoCfg) == PluginList::RC_OK)
          WinPostMsg(hwnd, CFG_REFRESH_LIST, 0, MPFROMSHORT(LIT_NONE));
        break;
      }
      return 0;
    }

    case WM_INITDLG:
      context = (PluginContext*)mp2;
      context->Hwnd = hwnd;
      WinSetWindowULong(hwnd, QWL_USER, (ULONG)context);
      do_warpsans(hwnd);
      WinPostMsg(hwnd, CFG_REFRESH_LIST, 0, MPFROMSHORT(LIT_NONE));
      return 0;

    // TODO: undo/default

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case LB_PLUGINS:
        switch (SHORT2FROMMP(mp1))
        {case LN_SELECT:
          i = WinQueryLboxSelectedItem(HWNDFROMMP(mp2));
          WinPostMsg(hwnd, CFG_REFRESH_INFO, 0, MPFROMSHORT(i));
          break;
         case LN_ENTER:
          i = WinQueryLboxSelectedItem(HWNDFROMMP(mp2));
          context->Configure(i);
          break;
        }
        break;

       case ML_DEC_FILETYPES:
        switch (SHORT2FROMMP(mp1))
        {case MLN_CHANGE:
          WinEnableControl(hwnd, PB_PLG_SET, TRUE);
        }
        break;

       case CB_DEC_TRYOTHER:
       case CB_DEC_SERIALIZE:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
          WinEnableControl(hwnd, PB_PLG_SET, TRUE);
        }
        break;

      }
      return 0;

    case WM_COMMAND:
      i = lb_cursored(hwnd, LB_PLUGINS);
      switch (SHORT1FROMMP(mp1))
      {case PB_PLG_UNLOAD:
        if (i >= 0 && i < context->List->size())
        { if ((*context->List)[i]->IsInitialized())
            amp_error(hwnd, "Cannot unload currently used plug-in.");
          else if (lb_remove_item(hwnd, LB_PLUGINS, i) == 0)
          { // something wrong => reload list
            WinPostMsg(hwnd, CFG_REFRESH_LIST, 0, MPFROMSHORT(i));
          } else
          { context->List->remove(i);
            if (i >= lb_size(hwnd, LB_PLUGINS))
              WinPostMsg(hwnd, CFG_REFRESH_INFO, 0, MPFROMSHORT(LIT_NONE));
            else
              lb_select(hwnd, LB_PLUGINS, i);
          }
        }
        break;

       case PB_PLG_ADD:
        context->AddPlugin();
        break;

       case PB_PLG_UP:
        if (i != LIT_NONE && i > 0 && i < context->List->size())
        { context->List->move(i, i-1);
          WinPostMsg(hwnd, CFG_REFRESH_LIST, 0, MPFROMSHORT(i-1));
        }
        break;

       case PB_PLG_DOWN:
        if (i != LIT_NONE && i < context->List->size()-1)
        { context->List->move(i, i+1);
          WinPostMsg(hwnd, CFG_REFRESH_LIST, 0, MPFROMSHORT(i+1));
        }
        break;

       case PB_PLG_ENABLE:
        if (i >= 0 && i < context->List->size())
        { Plugin* pp = (*context->List)[i];
          bool enable = !pp->GetEnabled();
          if ((context->*context->enable_hook)(i, enable))
          { pp->SetEnabled(enable);
            WinPostMsg(hwnd, CFG_REFRESH_INFO, 0, MPFROMSHORT(i));
          }
        }
        break;

       case PB_PLG_ACTIVATE:
        if ( i != LIT_NONE && (context->Flags & PluginContext::CF_List1) && (context->*context->enable_hook)(i, true)
          && ((PluginList1*)context->List)->SetActive(i) )
        { WinPostMsg(hwnd, CFG_REFRESH_INFO, 0, MPFROMSHORT(i));
          PMRASSERT(WinEnableControl(hwnd, PB_PLG_ACTIVATE, FALSE));
        }
        break;

       case PB_PLG_CONFIG:
        context->Configure(i);
        break;

       case PB_PLG_SET:
        context->SetParams(i);
        break;
      }
      return 0;
  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

/* Processes messages of the setup dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
        case PB_UNDO:
        case PB_DEFAULT:
        {
          LONG id;
          HWND page = NULLHANDLE;

          id = (LONG)WinSendDlgItemMsg( hwnd, NB_CONFIG, BKM_QUERYPAGEID, 0,
                                              MPFROM2SHORT(BKA_TOP,BKA_MAJOR));

          if( id && id != BOOKERR_INVALID_PARAMETERS ) {
              page = (HWND)WinSendDlgItemMsg( hwnd, NB_CONFIG,
                                              BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(id), 0 );
          }

          if( page && page != (HWND)BOOKERR_INVALID_PARAMETERS ) {
            WinPostMsg( page, CFG_GLOB_BUTTON, mp1, mp2 );
          }
          return MRFROMLONG(1L);
        }

        case PB_HELP:
          GUI::ShowHelp(IDH_PROPERTIES);
          return 0;
      }
      return 0;

    case WM_DESTROY:
      save_ini();
      save_window_pos( hwnd );
      return 0;

    case WM_WINDOWPOSCHANGED:
      if(((SWP*)mp1)->fl & SWP_SIZE ) {
        nb_adjust( hwnd, (SWP*)mp1 );
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Creates the properties dialog. */
void
cfg_properties( HWND owner )
{
  HWND hwnd;
  HWND book;
  HWND page;

  hwnd = WinLoadDlg( HWND_DESKTOP, owner, cfg_dlg_proc, NULLHANDLE, DLG_CONFIG, 0 );
  book = WinWindowFromID( hwnd, NB_CONFIG );
  do_warpsans( book );

  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROM2SHORT( 100,25 ), MPFROMSHORT( BKA_MAJORTAB ));
  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROMLONG( 0 ), MPFROMSHORT( BKA_MINORTAB ));
  WinSendMsg( book, BKM_SETNOTEBOOKCOLORS, MPFROMLONG ( SYSCLR_FIELDBACKGROUND ),
                                           MPFROMSHORT( BKA_BACKGROUNDPAGECOLORINDEX ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_settings1_dlg_proc, NULLHANDLE, CFG_SETTINGS1, 0 ),
                            "~Behavior", "General behavior", MPFROM2SHORT( 1, 2 ) ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_settings2_dlg_proc, NULLHANDLE, CFG_SETTINGS2, 0 ),
                            NULL, "Playlist behavior", MPFROM2SHORT( 2, 2 ) ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_iosettings_dlg_proc, NULLHANDLE, CFG_IOSETTINGS, 0 ),
                            "~System settings", NULL, 0 ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_display1_dlg_proc, NULLHANDLE, CFG_DISPLAY1, 0 ),
                            "~Display", NULL, 0));

  PluginContext decoder_context(&Decoders, NULL, PLUGIN_INTERFACE_LEVEL, PluginContext::CF_None);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_DEC_CONFIG, &decoder_context ),
                            "~Plug-ins", "Decoder Plug-ins", MPFROM2SHORT( 1, 4 )));

  PluginContext filter_context(&Filters, NULL, PLUGIN_INTERFACE_LEVEL, PluginContext::CF_None);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_FIL_CONFIG, &filter_context ),
                            NULL, "Filter Plug-ins", MPFROM2SHORT( 2, 4 )));

  PluginContext output_context(&Outputs, NULL, PLUGIN_INTERFACE_LEVEL, PluginContext::CF_List1);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_OUT_CONFIG, &output_context ),
                            NULL, "Output Plug-ins", MPFROM2SHORT( 3, 4 )));

  PluginContext visual_context(&Visuals, &VisualsSkinned, PLUGIN_INTERFACE_LEVEL, PluginContext::CF_None);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_VIS_CONFIG, &visual_context ),
                            NULL, "Visual Plug-ins", MPFROM2SHORT( 4, 4 )));

  page = WinLoadDlg( book, book, &WinDefDlgProc, NULLHANDLE, CFG_ABOUT, 0 );
  do_warpsans( page );
  #if defined(__IBMCPP__)
    #if __IBMCPP__ <= 300
    const char built[] = "(built " __DATE__ " using IBM VisualAge C++ 3.0x)";
    #else
    const char built[] = "(built " __DATE__ " using IBM VisualAge C++ 3.6)";
    #endif
  #elif defined(__WATCOMC__)
    char built[128];
    #if __WATCOMC__ < 1200
    sprintf( built, "(built " __DATE__ " using Open Watcom C++ %d.%d)", __WATCOMC__ / 100, __WATCOMC__ % 100 );
    #else
    sprintf( built, "(built " __DATE__ " using Open Watcom C++ %d.%d)", __WATCOMC__ / 100 - 11, __WATCOMC__ % 100 );
    #endif
  #elif defined(__GNUC__)
    char built[128];
    #if __GNUC__ < 3
    sprintf( built, "(built " __DATE__ " using gcc %d.%d)", __GNUC__, __GNUC_MINOR__ );
    #else
    sprintf( built, "(built " __DATE__ " using gcc %d.%d.%d)", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__ );
    #endif
  #else
    const char* built = 0;
  #endif
  WinSetDlgItemText( page, ST_BUILT, built );
  WinSetDlgItemText( page, ST_AUTHORS, SDG_AUT );
  WinSetDlgItemText( page, ST_CREDITS, SDG_MSG );
  PMRASSERT( nb_append_tab( book, page, "~About", NULL, 0));

  rest_window_pos( hwnd );
  WinSetFocus( HWND_DESKTOP, book );

  WinProcessDlg   ( hwnd );
  WinDestroyWindow( hwnd );

  // TODO: only in case of a change!  
  amp_pipe_destroy();
  amp_pipe_create();
  
  // TODO: adjust the number of worker threads
}
