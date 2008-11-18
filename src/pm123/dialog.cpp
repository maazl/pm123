/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 M.Mueller
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
#include "dialog.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>

#include <utilfct.h>
#include <cpp/xstring.h>
#include <cpp/stringmap.h>
#include "properties.h"
#include "controller.h"
#include "copyright.h"
#include "skin.h"
#include "iniman.h"
#include "pm123.h"
#include "123_util.h"
#include "filedlg.h"
#include <os2.h>
#include "pm123.rc.h"

#include <debuglog.h>


#define  AMP_REFRESH_CONTROLS   ( WM_USER + 1000 ) /* 0,         0                            */


static HWND  hhelp      = NULLHANDLE;


HWND amp_help_mgr()
{ return hhelp;
}

xstring amp_get_window_text( HWND hwnd )
{ xstring ret;
  char* dst = ret.raw_init(WinQueryWindowTextLength(hwnd));
  PMXASSERT(WinQueryWindowText(hwnd, ret.length()+1, dst), == ret.length());
  return ret;
}

static xstring joinstringset(const stringset& set, char delim)
{ size_t len = 0;
  strkey*const* sp;
  // determine required size
  for (sp = set.begin(); sp != set.end(); ++sp)
    len += (*sp)->Key.length() +1;
  if (len == 0)
    return xstring::empty;
  // concatenate string
  xstring ret;
  char* dp = ret.raw_init(len-1);
  sp = set.begin();
  for(;;)
  { memcpy(dp, (*sp)->Key.cdata(), (*sp)->Key.length() +1);
    dp += (*sp)->Key.length();
    if (++sp == set.end())
      break;
    *dp++ = delim;
  }
  return ret; 
}

/* Wizzard function for the default entry "File..." */
ULONG DLLENTRY amp_file_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param )
{ DEBUGLOG(("amp_file_wizzard(%p, %s, %p, %p)\n", owner, title, callback, param));

  FILEDLG filedialog = { sizeof( FILEDLG ) };
  { stringset_own ext(32);
    dec_merge_extensions(ext);
    const xstring& se = joinstringset(ext, ';') + ")"; 

    xstring type_audio = FDT_AUDIO + se;
    xstring type_all = FDT_AUDIO_ALL + se;

    APSZ types[] = {
      { (PSZ)&*type_audio }, // OS/2 and const...
      { FDT_PLAYLIST },
      { (PSZ)&*type_all }, // OS/2 and const...
      { NULL } };

    xstring wintitle = xstring::sprintf(title, " file(s)");

    filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_MULTIPLESEL;
    filedialog.ulUser         = FDU_DIR_ENABLE | FDU_RECURSEBTN;
    filedialog.pszTitle       = (PSZ)wintitle.cdata(); // OS/2 and const...
    filedialog.papszITypeList = types;
    filedialog.pszIType       = (PSZ)type_all.cdata(); // OS/2 and const...

    strlcpy( filedialog.szFullFile, cfg.filedir, sizeof filedialog.szFullFile );
    PMRASSERT(amp_file_dlg( HWND_DESKTOP, owner, &filedialog ));
  }

  ULONG ret = 300; // Cancel unless DID_OK

  if( filedialog.lReturn == DID_OK ) {
    ret = 0;

    char* file = filedialog.ulFQFCount > 1
      ? **filedialog.papszFQFilename
      : filedialog.szFullFile;

    if (*file)
      sdrivedir( cfg.filedir, file, sizeof( cfg.filedir ));

    ULONG count = 0;
    while (*file)
    { DEBUGLOG(("amp_file_wizzard: %s\n", file));
      char fileurl[_MAX_FNAME+25]; // should be sufficient in all cases
      strcpy(fileurl, "file:///");
      strcpy(fileurl + (url123::isPathDelimiter(file[0]) && url123::isPathDelimiter(file[1]) ? 5 : 8), file);
      char* dp = fileurl + strlen(fileurl);
      if (is_dir(file))
      { // Folder => add trailing slash
        if (!url123::isPathDelimiter(dp[-1]))
          *dp++ = '/';
        if (filedialog.ulUser & FDU_RECURSE_ON)
        { strcpy(dp, "?recursive");
          dp += 10;
        } else
          *dp = 0;
      }
      // convert slashes
      dp = strchr(fileurl+7, '\\');
      while (dp)
      { *dp = '/';
        dp = strchr(dp+1, '\\');
      }
      // Callback
      (*callback)(param, fileurl);
      // next file
      if (++count >= filedialog.ulFQFCount)
        break;
      file = (*filedialog.papszFQFilename)[count];
    }
  }

  WinFreeFileDlgList( filedialog.papszFQFilename );

  return ret;
}

/* Default dialog procedure for the URL dialog. */
static MRESULT EXPENTRY amp_url_dlg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch( msg )
  {case WM_CONTROL:
    if (SHORT1FROMMP(mp1) == ENT_URL)
      switch (SHORT2FROMMP(mp1))
      {case CBN_EFCHANGE:
        // Update enabled status of the OK-Button
        DEBUGLOG(("amp_url_dlg_proc: WM_CONTROL: CBN_EFCHANGE\n"));
        PMRASSERT(WinEnableWindow(WinWindowFromID(hwnd, DID_OK), WinQueryWindowTextLength(HWNDFROMMP(mp2)) != 0));
        break;
       /*case CBN_ENTER:
        WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(DID_OK), MPFROM2SHORT(CMDSRC_OTHER, FALSE));
        break;*/
      }
    break;

   case WM_INITDLG:
    { // populate drop down list
      Playlist* mru = amp_get_url_mru();
      HWND ctrl = WinWindowFromID(hwnd, ENT_URL);
      int_ptr<PlayableInstance> pi;
      for(;;)
      { pi = mru->GetNext(pi);
        DEBUGLOG(("amp_url_dlg_proc: WM_INITDLG %p %p\n", pi.get(), ctrl));
        if (pi == NULL)
          break;
        PMXASSERT(WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(pi->GetPlayable()->GetURL().cdata())), >= 0);
      }
      break;
    }

   case WM_COMMAND:
    DEBUGLOG(("amp_url_dlg_proc: WM_COMMAND: %i\n", SHORT1FROMMP(mp1)));
    if (SHORT1FROMMP(mp1) == DID_OK)
    { HWND ent = WinWindowFromID(hwnd, ENT_URL);
      const xstring& text = amp_get_window_text(ent);
      const url123& url = amp_get_cwd().makeAbsolute(text);
      if (!url)
      { WinMessageBox(HWND_DESKTOP, hwnd, xstring::sprintf("The URL \"%s\" is not well formed.", text.cdata()),
          NULL, 0, MB_CANCEL|MB_WARNING|MB_APPLMODAL|MB_MOVEABLE);
        return 0; // cancel
      }
      // Replace the text by the expanded URL
      PMRASSERT(WinSetWindowText(ent, url.cdata()));
      break; // everything OK => continue
    }
  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

/* Adds HTTP file to the playlist or load it to the player. */
ULONG DLLENTRY amp_url_wizzard( HWND owner, const char* title, DECODER_WIZZARD_CALLBACK callback, void* param )
{ DEBUGLOG(("amp_url_wizzard(%x, %s, %p, %p)\n", owner, title, callback, param));

  amp_get_url_mru()->EnsureInfo(Playable::IF_Other);

  HWND hwnd = WinLoadDlg( HWND_DESKTOP, owner, amp_url_dlg_proc, NULLHANDLE, DLG_URL, 0 );
  if (hwnd == NULLHANDLE)
    return 500;

  do_warpsans(hwnd);

  xstring wintitle = xstring::sprintf(title, " URL");
  WinSetWindowText(hwnd, (PSZ)&*wintitle);
  
  // TODO: last URL

  ULONG ret = 300;
  if (WinProcessDlg(hwnd) == DID_OK)
  { const xstring& url = amp_get_window_text(WinWindowFromID(hwnd, ENT_URL));
    DEBUGLOG(("amp_url_wizzard: %s\n", url.cdata()));
    (*callback)(param, url);
    amp_AddMRU(amp_get_url_mru(), MAX_RECALL, PlayableSlice(url));
    ret = 0;
  }
  WinDestroyWindow(hwnd);
  return ret;
}

/* Adds a user selected bookmark. */
void amp_add_bookmark(HWND owner, const PlayableSlice& item)
{ DEBUGLOG(("amp_add_bookmark(%x, {%s})\n", owner, item.GetPlayable()->GetURL().cdata()));
  // TODO: !!!!!! request information before
  const META_INFO& meta = *item.GetPlayable()->GetInfo().meta;
  xstring desc = "";
  if (*meta.artist)
    desc = xstring(meta.artist) + "-";
  if (*meta.title)
    desc = desc + meta.title;
   else
    desc = desc + item.GetPlayable()->GetURL().getShortName();

  HWND hdlg = WinLoadDlg(HWND_DESKTOP, owner, &WinDefDlgProc, NULLHANDLE, DLG_BM_ADD, NULL);
  do_warpsans(hdlg);
  WinSetDlgItemText(hdlg, EF_BM_DESC, desc);

  if (WinProcessDlg(hdlg) == DID_OK)
  { const xstring& alias = amp_get_window_text(WinWindowFromID(hdlg, EF_BM_DESC));
    if (alias != desc) // Don't set alias if not required.
    { // We have to copy the PlayableSlice to modify it.
      PlayableSlice ps(item);
      ps.SetAlias(alias);
      amp_get_default_bm()->InsertItem(ps);
    } else
      amp_get_default_bm()->InsertItem(item);
    amp_get_default_bm()->Save(PlayableCollection::SaveRelativePath);
  }

  WinDestroyWindow(hdlg);
}

/* Saves a playlist */
void amp_save_playlist(HWND owner, PlayableCollection& playlist)
{
  APSZ  types[] = {{ FDT_PLAYLIST_LST }, { FDT_PLAYLIST_M3U }, { 0 }};

  FILEDLG filedialog = {sizeof(FILEDLG)};
  filedialog.fl             = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB;
  filedialog.pszTitle       = "Save playlist";
  filedialog.ulUser         = FDU_RELATIVBTN;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_PLAYLIST_LST;

  if ((playlist.GetFlags() & Playable::Mutable) == Playable::Mutable && playlist.GetURL().isScheme("file://"))
  { // Playlist => save in place allowed => preselect our own file name
    const char* cp = playlist.GetURL().cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    strlcpy(filedialog.szFullFile, cp, sizeof filedialog.szFullFile);
    // preselect file type
    if (playlist.GetURL().getExtension().compareToI(".M3U") == 0)
      filedialog.pszIType = FDT_PLAYLIST_M3U;
    // TODO: other playlist types
  } else
  { // not mutable => only save as allowed
    // TODO: preselect directory
  }

  PMXASSERT(amp_file_dlg(HWND_DESKTOP, owner, &filedialog), != NULLHANDLE);

  if(filedialog.lReturn == DID_OK)
  { url123 file = url123::normalizeURL(filedialog.szFullFile);
    if (!(Playable::IsPlaylist(file)))
    { if (file.getExtension().length() == 0)
      { // no extension => choose automatically
        if (strcmp(filedialog.pszIType, FDT_PLAYLIST_M3U) == 0)
          file = file + ".m3u";
        else // if (strcmp(filedialog.pszIType, FDT_PLAYLIST_LST) == 0)
          file = file + ".lst";
        // TODO: other playlist types
      } else
      { amp_error(owner, "PM123 cannot write playlist files with the unsupported extension %s.", file.getExtension().cdata());
        return;
      }
    }
    const char* cp = file.cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    if (amp_warn_if_overwrite(owner, cp))
    { PlayableCollection::save_options so = PlayableCollection::SaveDefault;
      if (file.getExtension().compareToI(".m3u") == 0)
        so |= PlayableCollection::SaveAsM3U;
      if (filedialog.ulUser & FDU_RELATIV_ON)
        so |= PlayableCollection::SaveRelativePath;
      // now save
      if (!playlist.Save(file, so))
        amp_error(owner, "Failed to create playlist \"%s\". Error %s.", file.cdata(), xio_strerror(xio_errno()));
    }
  }
}

/* Loads a skin selected by the user. */
void amp_loadskin( HPS hps )
{
  FILEDLG filedialog;
  APSZ types[] = {{ FDT_SKIN }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG;
  filedialog.pszTitle       = "Load PM123 skin";
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_SKIN;

  sdrivedir( filedialog.szFullFile, cfg.defskin, sizeof( filedialog.szFullFile ));
  amp_file_dlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    bmp_load_skin( filedialog.szFullFile, amp_player_hab(), amp_player_window(), hps );
    strcpy( cfg.defskin, filedialog.szFullFile );
  }
}


/* Returns TRUE if the save stream feature has been enabled. */
void amp_save_stream( HWND hwnd, BOOL enable )
{
  if( enable )
  {
    FILEDLG filedialog;

    memset( &filedialog, 0, sizeof( FILEDLG ));
    filedialog.cbSize     = sizeof( FILEDLG );
    filedialog.fl         = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB;
    filedialog.pszTitle   = "Save stream as";

    strcpy( filedialog.szFullFile, cfg.savedir );
    amp_file_dlg( HWND_DESKTOP, hwnd, &filedialog );

    if( filedialog.lReturn == DID_OK ) {
      if( amp_warn_if_overwrite( hwnd, filedialog.szFullFile ))
      {
        Ctrl::PostCommand(Ctrl::MkSave(filedialog.szFullFile), &amp_control_event_callback);
        sdrivedir( cfg.savedir, filedialog.szFullFile, sizeof( cfg.savedir ));
      }
    }
  } else {
    Ctrl::PostCommand(Ctrl::MkSave(xstring()));
  }
}

static USHORT amp_message_box( HWND owner, const char* title,
                             const char* message, ULONG style  )
{
  char padded_title[60];
  sprintf( padded_title, "%-59s", title );

  if( owner == NULLHANDLE )
  { owner  =  HWND_DESKTOP;
    style &= ~MB_APPLMODAL;
  } else {
    style |=  MB_APPLMODAL;
  }

  return WinMessageBox( HWND_DESKTOP, owner, (PSZ)message,
                                      padded_title, 0, style );
}

/* Creates and displays a error message window.
   Use the player window as message window owner. */
void amp_player_error( const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  DEBUGLOG(("ERROR: %s\n", message.cdata()));
  amp_message_box( amp_player_window(), "PM123 Error", message, MB_ERROR | MB_OK | MB_MOVEABLE );
}

/* Creates and displays a error message window.
   The specified owner window is disabled. */
void amp_error( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  DEBUGLOG(("ERROR: %x, %s\n", owner, message.cdata()));
  amp_message_box( owner, "PM123 Error", message, MB_ERROR | MB_OK | MB_MOVEABLE );
}

/* Creates and displays a message window. */
void amp_info( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  DEBUGLOG(("INFO: %s\n", message.cdata()));
  amp_message_box( owner, "PM123 Information", message, MB_INFORMATION | MB_OK | MB_MOVEABLE );
}

/* Requests the user about specified action. Returns
   TRUE at confirmation or FALSE in other case. */
BOOL amp_query( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  return amp_message_box( owner, "PM123 Query", message, MB_QUERY | MB_YESNO | MB_MOVEABLE ) == MBID_YES;
}

/* Requests the user about specified action. Provodes a cancel button.
   Returns the pressed Button (MBID_xxx constants) */
USHORT amp_query3( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  return amp_message_box( owner, "PM123 Query", message, MB_QUERY | MB_YESNOCANCEL | MB_MOVEABLE );
}

/* Requests the user about overwriting a file. Returns
   TRUE at confirmation or at absence of a file. */
BOOL amp_warn_if_overwrite( HWND owner, const char* filename )
{
  struct stat fi;
  if( stat( filename, &fi ) == 0 ) {
    return amp_query( owner, "File %s already exists. Overwrite it?", filename );
  } else {
    return TRUE;
  }
}

/* Tells the help manager to display a specific help window. */
bool amp_show_help( SHORT resid )
{ return WinSendMsg( hhelp, HM_DISPLAY_HELP,
    MPFROMLONG( MAKELONG( resid, NULL )), MPFROMSHORT( HM_RESOURCEID )) == 0;
}


/* global init */
void dlg_init()
{
  xstring infname(startpath + "pm123.inf");
  struct stat fi;
  if( stat( infname, &fi ) != 0  )
    // If the file of the help does not placed together with the program,
    // we shall give to the help manager to find it.
    infname = "pm123.inf";

  HELPINIT hinit = { sizeof( hinit ) };
  hinit.phtHelpTable = (PHELPTABLE)MAKELONG( HLP_MAIN, 0xFFFF );
  hinit.pszHelpWindowTitle = "PM123 Help";
  #ifdef DEBUG
  hinit.fShowPanelId = CMIC_SHOW_PANEL_ID;
  #else
  hinit.fShowPanelId = CMIC_HIDE_PANEL_ID;
  #endif
  hinit.pszHelpLibraryName = (PSZ)infname.cdata();

  hhelp = WinCreateHelpInstance( amp_player_hab(), &hinit );
  if( hhelp == NULLHANDLE )
    amp_error( amp_player_window(), "Error create help instance: %s", infname.cdata() );
  else
    PMRASSERT(WinAssociateHelpInstance(hhelp, amp_player_window()));
}

void dlg_uninit()
{ if (hhelp != NULLHANDLE)
    WinDestroyHelpInstance(hhelp);
}

