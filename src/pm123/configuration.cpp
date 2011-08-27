/*
 * Copyright 2007-2011 Marcel Mueller
 * Copyright 2004      Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
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

#define INCL_ERRORS
#include "configuration.h"
#include "plugman.h"
#include "visual.h"
#include "123_util.h"
#include "pm123.h"
#include "dialog.h"
#include <inimacro.h>
#include <os2.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>


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

// The properties!
const amp_cfg Cfg::Default =
{ ""
, true
, false
, true
, false
, false
, CFG_ANAV_SONG
, true
, true // recurse_dnd
, true
, false
, false
, false
, false
, 2 // num_workers
, 1

, 1 // font
, false
, { sizeof(FATTRS), 0, 0, "WarpSans Bold", 0, 0, 16, 7, 0, 0 }
, 9

, false // float on top
, CFG_SCROLL_INFINITE
, true
, CFG_DISP_ID3TAG
, 9
, "" // Proxy
, ""
, false
, 128
, 30
, 15
, "\\PIPE\\PM123"
, true // dock
, 10
, true
, 30
, 500
, true

, "oggplay.dll?enabled=true&filetypes=OGG\n"
  "mpg123.dll?enabled=true&filetypes=MP1;MP2;MP3\n"
  "wavplay.dll?enabled=true&filetypes=Digital Audio\n"
  "plist123.dll?enabled=true&filetypes=Playlist\n"
  "cddaplay.dll?enabled=true\n"
  "os2rec.dll?enabled=true\n"
, "realeq.dll?enabled=true\n"
  "logvolum.dll?enabled=false\n"
, "os2audio.dll?enabled=true\n"
  "wavout.dll?enabled=false\n"
  "pulse123.dll?enabled=false\n"
, ""
  // state
, ""
, ""
, ""
, CFG_MODE_REGULAR
, false
, false
, false
, true // add recursive
, true // save relative
};

Mutex Cfg::Mtx;
amp_cfg Cfg::Current = Cfg::Default;
event<CfgValidateArgs> Cfg::Validate;
event<const CfgChangeArgs> Cfg::Change;
HINI Cfg::HIni;


void Cfg::LoadPlugins(const char* key, xstring amp_cfg::*cfg, PLUGIN_TYPE type)
{ PluginList list(type);
  if (ini_query_xstring(HIni, INI_SECTION, key, Current.*cfg))
  { xstring err = list.Deserialize(Current.*cfg);
    if (err)
    { pm123_display_error(err);
      if (!list.size())
        goto fail;
    }
  } else
  {fail:
    list.LoadDefaults();
  }
  Plugin::SetPluginList(list);
}

void Cfg::SavePlugins(const char* key, xstring amp_cfg::*cfg, PLUGIN_TYPE type)
{
  PluginList list(type);
  Plugin::GetPlugins(list, false);
  // do not save skinned visuals
  if (type == PLUGIN_VISUAL)
  { const int_ptr<Plugin>* ppp = list.begin();
    while (ppp != list.end())
      if (((Visual&)**ppp).GetProperties().skin)
        list.erase(ppp);
      else
        ++ppp;
  }
  ini_write_xstring(HIni, INI_SECTION, key, Current.*cfg = list.Serialize());
}

void Cfg::LoadIni()
{
  xstring tmp;
  // We are not yet multi-threaded, so access is not serialized.
  amp_cfg& cfg = Current;

  load_ini_int(HIni, cfg.playonload);
  load_ini_int(HIni, cfg.autouse);
  load_ini_int(HIni, cfg.retainonexit);
  load_ini_int(HIni, cfg.retainonstop);
  load_ini_int(HIni, cfg.restartonstart);
  load_ini_int(HIni, cfg.altnavig);
  load_ini_int(HIni, cfg.autoturnaround);
  load_ini_int(HIni, cfg.recurse_dnd);
  load_ini_int(HIni, cfg.sort_folders);
  load_ini_int(HIni, cfg.folders_first);
  load_ini_int(HIni, cfg.append_dnd);
  load_ini_int(HIni, cfg.append_cmd);
  load_ini_int(HIni, cfg.queue_mode);
  load_ini_int(HIni, cfg.num_workers);
  load_ini_int(HIni, cfg.num_dlg_workers);
  load_ini_int(HIni, cfg.mode);
  load_ini_int(HIni, cfg.font);
  load_ini_int(HIni, cfg.floatontop);
  load_ini_int(HIni, cfg.scroll);
  load_ini_int(HIni, cfg.viewmode);
  load_ini_int(HIni, cfg.max_recall);
  load_ini_int(HIni, cfg.buff_wait);
  load_ini_int(HIni, cfg.buff_size);
  load_ini_int(HIni, cfg.buff_fill);
  load_ini_int(HIni, cfg.conn_timeout);
  load_ini_xstring(HIni, cfg.pipe_name);
  load_ini_int(HIni, cfg.add_recursive);
  load_ini_int(HIni, cfg.save_relative);
  load_ini_int(HIni, cfg.show_playlist);
  load_ini_int(HIni, cfg.show_bmarks);
  load_ini_int(HIni, cfg.show_plman);
  load_ini_int(HIni, cfg.dock_margin);
  load_ini_int(HIni, cfg.dock_windows);
  load_ini_int(HIni, cfg.win_pos_by_obj);
  load_ini_int(HIni, cfg.win_pos_max_age);
  load_ini_int(HIni, cfg.insp_autorefresh);
  load_ini_int(HIni, cfg.insp_autorefresh_on);
  load_ini_int(HIni, cfg.font_skinned);
  load_ini_value(HIni, cfg.font_attrs );
  load_ini_int(HIni, cfg.font_size);
  //load_ini_value(HIni, cfg.main);

  load_ini_xstring(HIni, cfg.filedir);
  load_ini_xstring(HIni, cfg.listdir);
  load_ini_xstring(HIni, cfg.savedir);
  load_ini_xstring(HIni, cfg.proxy);
  load_ini_xstring(HIni, cfg.auth);
  load_ini_xstring(HIni, cfg.defskin);

  LoadPlugins("decoders_list", &amp_cfg::decoders_list, PLUGIN_DECODER);
  LoadPlugins("outputs_list",  &amp_cfg::outputs_list,  PLUGIN_OUTPUT);
  LoadPlugins("filters_list",  &amp_cfg::filters_list,  PLUGIN_FILTER);
  LoadPlugins("visuals_list",  &amp_cfg::visuals_list,  PLUGIN_VISUAL);
}

void Cfg::SaveIni()
{
  const amp_cfg& cfg = Current;

  { Mutex::Lock lock(Mtx);

    save_ini_bool (HIni, cfg.playonload);
    save_ini_bool (HIni, cfg.autouse);
    save_ini_bool (HIni, cfg.retainonexit);
    save_ini_bool (HIni, cfg.retainonstop);
    save_ini_bool (HIni, cfg.restartonstart);
    save_ini_value(HIni, cfg.altnavig);
    save_ini_bool (HIni, cfg.autoturnaround);
    save_ini_bool (HIni, cfg.recurse_dnd);
    save_ini_bool (HIni, cfg.sort_folders);
    save_ini_bool (HIni, cfg.folders_first);
    save_ini_bool (HIni, cfg.append_dnd);
    save_ini_bool (HIni, cfg.append_cmd);
    save_ini_bool (HIni, cfg.queue_mode);
    save_ini_value(HIni, cfg.num_workers);
    save_ini_value(HIni, cfg.num_dlg_workers);
    save_ini_value(HIni, cfg.mode);
    save_ini_value(HIni, cfg.font);
    save_ini_bool (HIni, cfg.floatontop);
    save_ini_value(HIni, cfg.scroll);
    save_ini_value(HIni, cfg.viewmode);
    save_ini_value(HIni, cfg.max_recall);
    save_ini_value(HIni, cfg.buff_wait);
    save_ini_value(HIni, cfg.buff_size);
    save_ini_value(HIni, cfg.buff_fill);
    save_ini_value(HIni, cfg.conn_timeout);
    save_ini_xstring(HIni, cfg.pipe_name);
    save_ini_bool (HIni, cfg.add_recursive);
    save_ini_bool (HIni, cfg.save_relative);
    save_ini_bool (HIni, cfg.show_playlist);
    save_ini_bool (HIni, cfg.show_bmarks);
    save_ini_bool (HIni, cfg.show_plman);
    save_ini_bool (HIni, cfg.dock_windows);
    save_ini_value(HIni, cfg.dock_margin);
    save_ini_bool (HIni, cfg.win_pos_by_obj);
    save_ini_value(HIni, cfg.win_pos_max_age);
    save_ini_value(HIni, cfg.insp_autorefresh);
    save_ini_bool (HIni, cfg.insp_autorefresh_on);
    save_ini_bool (HIni, cfg.font_skinned);
    save_ini_value(HIni, cfg.font_attrs);
    save_ini_value(HIni, cfg.font_size);
    //save_ini_value(HIni, cfg.main );

    save_ini_xstring(HIni, cfg.filedir);
    save_ini_xstring(HIni, cfg.listdir);
    save_ini_xstring(HIni, cfg.savedir);
    save_ini_xstring(HIni, cfg.proxy);
    save_ini_xstring(HIni, cfg.auth);
    save_ini_xstring(HIni, cfg.defskin);

    SavePlugins("decoders_list", &amp_cfg::decoders_list, PLUGIN_DECODER);
    SavePlugins("outputs_list",  &amp_cfg::outputs_list,  PLUGIN_OUTPUT);
    SavePlugins("filters_list",  &amp_cfg::filters_list,  PLUGIN_FILTER);
    SavePlugins("visuals_list",  &amp_cfg::visuals_list,  PLUGIN_VISUAL);
  }

  CleanIniPositions();
}

// Purge outdated ini locations in the profile
void Cfg::CleanIniPositions()
{ ULONG size;
  if (!PrfQueryProfileSize(HIni, "Positions", NULL, &size))
    return;
  char* names = (char*)alloca(size+2);
  names[size] = names[size+1] = 0; // ensure termination
  PrfQueryProfileData(HIni, "Positions", NULL, names, &size);
  const time_t limit = time(NULL) - Current.win_pos_max_age * 86400;
  for (char* cp = names; *cp; cp += strlen(cp)+1)
  { if (cp[0] != 'P' || cp[1] != '_')
      continue;
    time_t age = INT_MAX;
    size = sizeof(age);
    PrfQueryProfileData(HIni, "Positions", cp, &age, &size);
    if (age < limit)
      // Purge this entry
      PrfWriteProfileData(HIni, "Positions", cp, NULL, 0);
  }
}

/** Helper structure to patch the window positions in dialog units in the
 * data saved by WinStoreWindowPos and prepend a time stamp.
 */
struct ext_pos
{ time_t     tstmp; // Time stamp when the information has been saved.
  SHORT      pos[];
};
enum
{ WPOS_X   = 3
, WPOS_Y   = 4
, WPOS_CX  = 5
, WPOS_CY  = 6
, WPOS_XR  = 9
, WPOS_YR  = 10
, WPOS_CXR = 11
, WPOS_CYR = 12
};

static void map_ext_pos(HWND hwnd, ext_pos* xp, BOOL dir)
{ DEBUGLOG(( "map_ext_pos(%p, {..., %i,%i, %i,%i, ... %i,%i, %i,%i, ...}, %i)\n", hwnd,
    xp->pos[WPOS_X], xp->pos[WPOS_Y], xp->pos[WPOS_CX], xp->pos[WPOS_CY],
    xp->pos[WPOS_XR], xp->pos[WPOS_YR], xp->pos[WPOS_CXR], xp->pos[WPOS_CYR],
    dir ));
  POINTL points[4] =
  { { xp->pos[WPOS_X],                    xp->pos[WPOS_Y] }
  , { xp->pos[WPOS_X]+xp->pos[WPOS_CX],   xp->pos[WPOS_Y]+xp->pos[WPOS_CY] }
  , { xp->pos[WPOS_XR],                   xp->pos[WPOS_YR] }
  , { xp->pos[WPOS_XR]+xp->pos[WPOS_CXR], xp->pos[WPOS_YR]+xp->pos[WPOS_CYR] }
  };
  PMRASSERT(WinMapDlgPoints(hwnd, points, 4, dir));
  xp->pos[WPOS_X] = points[0].x;               xp->pos[WPOS_Y] = points[0].y;
  xp->pos[WPOS_CX] = points[1].x-points[0].x;  xp->pos[WPOS_CY] = points[1].y-points[0].y;
  xp->pos[WPOS_XR] = points[2].x;              xp->pos[WPOS_YR] = points[2].y;
  xp->pos[WPOS_CXR] = points[3].x-points[2].x; xp->pos[WPOS_CYR] = points[3].y-points[2].y;
  DEBUGLOG(( "map_ext_pos: {..., %i,%i, %i,%i, ... %i,%i, %i,%i, ...}\n",
      xp->pos[WPOS_X], xp->pos[WPOS_Y], xp->pos[WPOS_CX], xp->pos[WPOS_CY],
      xp->pos[WPOS_XR], xp->pos[WPOS_YR], xp->pos[WPOS_CXR], xp->pos[WPOS_CYR] ));
}

/** Helper struct to construct object specific keys in PM123.ini.
 */
struct ext_pos_key
{ char prefix[6]; // P_1234
  char delimiter; // _
  char object[255];
};

/* Saves the current size and position of the window specified by hwnd.
   This function will also save the presentation parameters. */
bool Cfg::SaveWindowPos(HWND hwnd, const char* extkey)
{ DEBUGLOG(("Cfg::SaveWindowPos(%p, %s)\n", hwnd, extkey));

  char key1st[20];  // Key in OS2.INI
  { PPIB ppib;
    PTIB ptib;
    DosGetInfoBlocks( &ptib, &ppib );
    sprintf(key1st, "P_%08lX_%08lX", ppib->pib_ulpid, ptib->tib_ptib2->tib2_ultid);
  }
  ext_pos_key key2; // Key in PM123.INI
  sprintf(key2.prefix, "P_%04X", WinQueryWindowUShort(hwnd, QWS_ID));

  if (!WinStoreWindowPos("PM123", key1st, hwnd))
    return false;

  BOOL rc = FALSE;
  ULONG size;
  if (PrfQueryProfileSize(HINI_PROFILE, "PM123", key1st, &size))
  { ext_pos* data = (ext_pos*)alloca(size + offsetof(ext_pos, pos));
    time(&data->tstmp);
    if (PrfQueryProfileData(HINI_PROFILE, "PM123", key1st, &data->pos, &size))
    { // convert to absolute coordinates
      map_ext_pos(hwnd, data, FALSE);
      // Write to profile
      size += offsetof(ext_pos, pos);
      rc = PrfWriteProfileData(HIni, "Positions", key2.prefix, data, size);
      if (extkey && Current.win_pos_by_obj)
      { key2.delimiter = '_';
        strlcpy(key2.object, extkey, sizeof key2.object);
        rc &= PrfWriteProfileData(HIni, "Positions", key2.prefix, data, size);
      }
    }
  }

  PrfWriteProfileData(HINI_PROFILE, "PM123", key1st, NULL, 0);
  return rc;
}

/* Restores the size and position of the window specified by hwnd to
   the state it was in when save_window_pos was last called.
   This function will also restore presentation parameters. */
bool Cfg::RestWindowPos(HWND hwnd, const char* extkey)
{ DEBUGLOG(("Cfg::RestWindowPos(%p, %s)\n", hwnd, extkey));

  char key1st[20];  // Key in OS2.INI
  { PPIB ppib;
    PTIB ptib;
    DosGetInfoBlocks( &ptib, &ppib );
    sprintf(key1st, "P_%08lX_%08lX", ppib->pib_ulpid, ptib->tib_ptib2->tib2_ultid);
  }
  ext_pos_key key2; // Key in PM123.INI
  sprintf(key2.prefix, "P_%04X", WinQueryWindowUShort(hwnd, QWS_ID));
  ULONG size;
  if (extkey && Current.win_pos_by_obj)
  { key2.delimiter = '_';
    strlcpy(key2.object, extkey, sizeof key2.object);
    if (PrfQueryProfileSize(HIni, "Positions", key2.prefix, &size))
      goto found;
    key2.delimiter = 0;
  }
  if (!PrfQueryProfileSize(HIni, "Positions", key2.prefix, &size))
    return false;
 found:
  ext_pos* data = (ext_pos*)alloca(size);
  if (!PrfQueryProfileData(HIni, "Positions", key2.prefix, data, &size))
    return false;

  // convert to window coordinates
  map_ext_pos(hwnd, data, TRUE);
  // Clip at desktop window
  { SWP desktop;
    PMRASSERT(WinQueryWindowPos(HWND_DESKTOP, &desktop));
    // clip right
    if (data->pos[WPOS_X] > desktop.cx - 8)
      data->pos[WPOS_X] = desktop.cx - 8;
    // clip left
    else if (data->pos[WPOS_X] + data->pos[WPOS_CX] < 8)
      data->pos[WPOS_X] = 8 - data->pos[WPOS_CX];
    // clip bottom
    if (data->pos[WPOS_Y] + data->pos[WPOS_CY] < 8)
      data->pos[WPOS_Y] = 8 - data->pos[WPOS_CY];
    // clip top
    else if (data->pos[WPOS_Y] + data->pos[WPOS_CY] > desktop.cy)
      data->pos[WPOS_Y] = desktop.cy - data->pos[WPOS_CY];
  }
  // Write to OS2 profile
  size -= offsetof(ext_pos, pos);
  if (!PrfWriteProfileData(HINI_PROFILE, "PM123", key1st, &data->pos, size))
    return false;

  BOOL rc = WinRestoreWindowPos("PM123", key1st, hwnd);
  PrfWriteProfileData( HINI_PROFILE, "PM123", key1st, NULL, 0);

  return rc;
}


void Cfg::MigrateIni(const char* inipath, const char* app)
{
  ULONG len;
  char module[16];
  snprintf(module, sizeof module, "%s.dll", app);
  if (PrfQueryProfileSize(HIni, module, NULL, &len) && len)
    return; // Data already there

  xstring inifile = xstring::sprintf("%s\\%s.ini", inipath, app);
  // Check if file exists
  HFILE fh;
  if ( DosOpen(inifile, &fh, &len, 0, 0, OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_FLAGS_NOINHERIT|OPEN_SHARE_DENYNONE|OPEN_ACCESS_READONLY, NULL) != NO_ERROR )
    return;
  DosClose(fh);
  // Read old ini
  HINI hini = PrfOpenProfile(amp_player_hab, inifile);
  if (hini == NULLHANDLE)
    return;

  if (PrfQueryProfileSize(hini, "Settings", NULL, &len))
  { char* names = (char*)alloca(len);
    if (PrfQueryProfileData(hini, "Settings", NULL, names, &len))
    { while (*names)
      { char buf[1024];
        len = sizeof buf;
        if (PrfQueryProfileData(hini, "Settings", names, buf, &len))
          PrfWriteProfileData(HIni, module, names, buf, len);
        names += strlen(names)+1;
      }
    }
  }

  close_ini(hini);
}

bool Cfg::Set(amp_cfg& settings, xstring* error)
{ Mutex::Lock lock(Mtx);
  { CfgValidateArgs val(settings, Current);
    Validate(val);
    if (error)
      *error = val.Message;
    if (val.Fail)
      return false;
  }
  { amp_cfg old = Current;
    Current = settings;
    Change(CfgChangeArgs(Current, old));
  }
  return true;
}

/* Initialize properties, called from main. */
void Cfg::Init()
{
  // Open profile
  xstring inipath = amp_basepath + "\\PM123.INI"; // TODO: command line option
  HIni = PrfOpenProfile(amp_player_hab, inipath);
  if (HIni == NULLHANDLE)
    amp_fail("Failed to access PM123 configuration file %s.", inipath.cdata());

  LoadIni();

  MigrateIni(amp_basepath, "analyzer");
  MigrateIni(amp_basepath, "mpg123");
  MigrateIni(amp_basepath, "os2audio");
  MigrateIni(amp_basepath, "realeq");
}

void Cfg::Uninit()
{ if (!PrfCloseProfile(HIni))
  { char buf[1024];
    os2pm_strerror(buf, sizeof(buf));
    amp_error(HWND_DESKTOP, "PM123 failed to write its profile: %s", buf);
  }
}
