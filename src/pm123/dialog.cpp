/*
 * Copyright 2007-2011 M.Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Leppä <rosmo@sektori.com>
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

#include "configuration.h"
#include "controller.h"
#include "copyright.h"
#include "skin.h"
#include "gui.h"
#include "123_util.h"
#include "filedlg.h"
#include <utilfct.h>
#include <fileutil.h>
#include <cpp/pmutils.h>
#include <xio.h>
#include <cpp/xstring.h>
#include <cpp/container/stringmap.h>
#include <os2.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>

#include "pm123.rc.h"

#include <debuglog.h>


#define  AMP_REFRESH_CONTROLS   ( WM_USER + 1000 ) /* 0,         0                            */


// static HWND  hhelp      = NULLHANDLE;


/*HWND amp_help_mgr()
{ return hhelp;
}*/

/*static xstring joinstringset(const stringset& set, char delim)
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
}*/

/* Wizard function for the default entry "File..." */
ULONG DLLENTRY amp_file_wizard( HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param )
{ DEBUGLOG(("amp_file_wizard(%p, %s, %p, %p)\n", owner, title, callback, param));

  FILEDLG filedialog = { sizeof( FILEDLG ) };
  sco_ptr<APSZ_list> types(amp_file_types(DECODER_FILENAME));

  xstring wintitle = xstring::sprintf(title, " file(s)");

  filedialog.fl             = FDS_CENTER|FDS_OPEN_DIALOG|FDS_MULTIPLESEL;
  filedialog.ulUser         = FDU_DIR_ENABLE|FDU_RECURSEBTN;
  filedialog.pszTitle       = (PSZ)wintitle.cdata(); // OS/2 and const...
  filedialog.papszITypeList = *types;
  char type[_MAX_PATH] = FDT_ALL;
  filedialog.pszIType       = type;

  xstring filedir(Cfg::Get().filedir);
  if (filedir.length() > 8)
    // strip file:///
    strlcpy(filedialog.szFullFile, filedir+8, sizeof filedialog.szFullFile);
  else
    filedialog.szFullFile[0] = 0;
  PMRASSERT(amp_file_dlg( HWND_DESKTOP, owner, &filedialog ));

  ULONG ret = 300; // Cancel unless DID_OK

  if( filedialog.lReturn == DID_OK ) {
    ret = 0;

    char* file = filedialog.ulFQFCount > 1
      ? **filedialog.papszFQFilename
      : filedialog.szFullFile;

    ULONG count = 0;
    while (*file)
    { DEBUGLOG(("amp_file_wizard: %s\n", file));
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
        }
        *dp = 0;
      }
      // convert slashes
      dp = strchr(fileurl+7, '\\');
      while (dp)
      { *dp = '/';
        dp = strchr(dp+1, '\\');
      }
      // Save directory
      if (count == 0)
      { size_t p = strlen(fileurl);
        while (p && fileurl[p] != '/')
          --p;
        Cfg::ChangeAccess().filedir.assign(fileurl, p);
      }
      // Callback
      (*callback)(param, fileurl, NULL, IF_None, IF_None);
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
    { Cfg::RestWindowPos(hwnd);
      // populate drop down list
      Playable& mru = GUI::GetUrlMRU();
      HWND ctrl = WinWindowFromID(hwnd, ENT_URL);
      int_ptr<PlayableInstance> pi;
      for(;;)
      { pi = mru.GetNext(pi);
        DEBUGLOG(("amp_url_dlg_proc: WM_INITDLG %p %p\n", pi.get(), ctrl));
        if (pi == NULL)
          break;
        PMXASSERT(WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(pi->GetPlayable().URL.cdata())), >= 0);
      }
      break;
    }
   
   case WM_DESTROY:
    Cfg::SaveWindowPos(hwnd);
    break;

   case WM_COMMAND:
    DEBUGLOG(("amp_url_dlg_proc: WM_COMMAND: %i\n", SHORT1FROMMP(mp1)));
    if (SHORT1FROMMP(mp1) == DID_OK)
    { HWND ent = WinWindowFromID(hwnd, ENT_URL);
      const xstring& text = WinQueryWindowXText(ent);
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
ULONG DLLENTRY amp_url_wizard( HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param )
{ DEBUGLOG(("amp_url_wizard(%x, %s, %p, %p)\n", owner, title, callback, param));

  // TODO: das geht gar nicht!
  GUI::GetUrlMRU().RequestInfo(IF_Child, PRI_Sync);

  HWND hwnd = WinLoadDlg( HWND_DESKTOP, owner, amp_url_dlg_proc, NULLHANDLE, DLG_URL, 0 );
  if (hwnd == NULLHANDLE)
    return 500;

  do_warpsans(hwnd);

  xstring wintitle = xstring::sprintf(title, " URL");
  WinSetWindowText(hwnd, (PSZ)&*wintitle);
  
  // TODO: last URL

  ULONG ret = 300;
  if (WinProcessDlg(hwnd) == DID_OK)
  { const xstring& url = WinQueryDlgItemXText(hwnd, ENT_URL);
    DEBUGLOG(("amp_url_wizard: %s\n", url.cdata()));
    (*callback)(param, url, NULL, IF_None, IF_None);
    GUI::Add2MRU(GUI::GetUrlMRU(), Cfg::Get().max_recall, *Playable::GetByURL(url));
    ret = 0;
  }
  WinDestroyWindow(hwnd);
  return ret;
}

url123 amp_playlist_select(HWND owner, const char* title)
{
  DEBUGLOG(("amp_playlist_select(%p, %s)\n", owner, title));
  sco_ptr<APSZ_list> types(amp_file_types(DECODER_FILENAME|DECODER_PLAYLIST));

  FILEDLG filedialog = { sizeof(FILEDLG) };
  filedialog.fl             = FDS_CENTER|FDS_OPEN_DIALOG;
  filedialog.pszTitle       = (PSZ)title;
  filedialog.papszITypeList = *types;
  char type[_MAX_PATH] = "Playlist File";
  filedialog.pszIType       = type;
  filedialog.ulUser         = FDU_RECURSEBTN|FDU_DIR_ENABLE;

  xstring listdir(Cfg::Get().listdir);
  if (listdir.length() > 8)
    // strip file:///
    strlcpy(filedialog.szFullFile, listdir+8, sizeof filedialog.szFullFile);
  else
    filedialog.szFullFile[0] = 0;
  PMXASSERT(amp_file_dlg(HWND_DESKTOP, owner, &filedialog), != NULLHANDLE);

  if( filedialog.lReturn == DID_OK )
  { 
    char* dp = filedialog.szFullFile + strlen(filedialog.szFullFile);
    if (is_dir(filedialog.szFullFile))
    { // Folder => add trailing slash
      if (!url123::isPathDelimiter(dp[-1]))
        *dp++ = '/';
      if (filedialog.ulUser & FDU_RECURSE_ON)
      { strcpy(dp, "?recursive");
        dp += 10;
      }
      *dp = 0;
    }

    url123 url = url123::normalizeURL(filedialog.szFullFile);
    // Save directory
    { size_t p = url.length();
      while (p && url[p] != '/')
        --p;
      Cfg::ChangeAccess().filedir.assign(url.cdata(), p);
    }
    return url;
  } else
  { return url123();
  }
}

/* Default dialog procedure for the bookmark dialog. */
static MRESULT EXPENTRY amp_bookmark_dlg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    do_warpsans(hwnd);
    Cfg::RestWindowPos(hwnd);
    break;
  
   case WM_DESTROY:
    Cfg::SaveWindowPos(hwnd);
    break;
  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

/* Adds a user selected bookmark. */
void amp_add_bookmark(HWND owner, APlayable& item)
{ DEBUGLOG(("amp_add_bookmark(%x, {%s})\n", owner, item.GetPlayable().URL.cdata()));

  HWND hdlg = WinLoadDlg(HWND_DESKTOP, owner, &amp_bookmark_dlg_proc, NULLHANDLE, DLG_BM_ADD, NULL);
  // TODO: !!!!!! request information before
  xstring desc = item.GetDisplayName();
  WinSetDlgItemText(hdlg, EF_BM_DESC, desc);

  if (WinProcessDlg(hdlg) == DID_OK)
  { const xstring& alias = WinQueryDlgItemXText(hdlg, EF_BM_DESC);
    // TODO: kein synchrones Wait!!!
    Playable& p = GUI::GetDefaultBM();
    /* TODO: SetAlias gibt es nicht mehr
    if (alias != desc) // Don't set alias if not required.
    { // We have to copy the PlayableRef to modify it.
      PlayableRef ps(item);
      ps.SetAlias(alias);
      p.InsertItem(ps);
    } else */
      p.InsertItem(item, NULL);
    // TODO: Save
    //p.Save(PlayableCollection::SaveRelativePath);
  }

  WinDestroyWindow(hdlg);
}

url123 amp_save_playlist(HWND owner, Playable& playlist, bool saveas)
{
  ASSERT(playlist.GetInfo().tech->attributes & TATTR_PLAYLIST);
  // If the item is not saveable or initial revert to save as.
  if ( !(playlist.GetInfo().phys->attributes & PATTR_WRITABLE)
      || !playlist.GetInfo().tech->decoder )
    saveas = true;

  url123 dest = playlist.URL;
  xstring decoder = playlist.GetInfo().tech->decoder;
  xstring format = playlist.GetInfo().tech->format;
  bool relative = true;

  if (saveas)
  { sco_ptr<APSZ_list> types(amp_file_types(DECODER_PLAYLIST|DECODER_WRITABLE));

    FILEDLG filedialog = {sizeof(FILEDLG)};
    filedialog.fl             = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB;
    filedialog.pszTitle       = "Save playlist as";
    filedialog.ulUser         = FDU_RELATIVBTN;
    filedialog.papszITypeList = *types;
    char type[_MAX_PATH] = "Playlist File";
    filedialog.pszIType       = type;

    xstring path; // Keep persistence of path

    if (playlist.URL.isScheme("file://"))
    {
      if ( (playlist.GetInfo().phys->attributes & PATTR_WRITABLE)
        && (playlist.GetInfo().tech->attributes & TATTR_WRITABLE) )
      { // Playlist => save in place allowed => preselect our own file name
        path = playlist.URL;
        // preselect file type
        filedialog.pszIType = (PSZ)DSTRING(playlist.GetInfo().tech->format).cdata();
      } else
      { // not mutable => only save as allowed
        path = playlist.URL.getBasePath();
      }
      const char* cp = path.cdata() + 5;
      if (cp[2] == '/')
        cp += 3;
      strlcpy(filedialog.szFullFile, cp, sizeof filedialog.szFullFile);
    }
   retry:
    PMXASSERT(amp_file_dlg(HWND_DESKTOP, owner, &filedialog), != NULLHANDLE);

    if (filedialog.lReturn != DID_OK)
      return xstring();
    dest = url123::normalizeURL(filedialog.szFullFile);
    const char* cp = dest.cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    if (!amp_warn_if_overwrite(owner, cp))
      goto retry;

    // Deduce decoder and format from file dialog format.
    int dec = amp_decoder_by_type(DECODER_PLAYLIST|DECODER_WRITABLE, filedialog.pszIType, format);
    if (dec == -1)
    { amp_info(owner, "The format to save cannot be guessed. "
                      "Please select an appropriate format to save in the file dialog.");
      goto retry;
    }
    decoder = Decoders[dec]->GetModule().Key;

    relative = (filedialog.ulUser & FDU_RELATIV_ON) != 0;
  }

  if (!playlist.Save(dest, decoder, format, relative))
  { amp_error(owner, "Failed to create playlist \"%s\". Error %s.", dest.cdata(), xio_strerror(xio_errno()));
    return xstring();
  }
  return dest;
}


/* Loads a skin selected by the user. */
bool amp_loadskin()
{
  FILEDLG filedialog;
  APSZ types[] = {{ "PM123 Skin File (*.SKN)" }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG;
  filedialog.pszTitle       = "Load PM123 skin";
  filedialog.papszITypeList = types;
  char type[_MAX_PATH] = "PM123 Skin File";
  filedialog.pszIType       = type;

  sdrivedir( filedialog.szFullFile, xstring(Cfg::Get().defskin), sizeof( filedialog.szFullFile ));
  amp_file_dlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    Cfg::ChangeAccess().defskin = filedialog.szFullFile;
    return true;
  }
  return false;
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

  return WinMessageBox(HWND_DESKTOP, owner, (PSZ)message, padded_title, 0, style);
}

/* Creates and displays a error message window.
   Use the player window as message window owner. */
void amp_player_error( const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message = xstring::vsprintf(format, args);
  va_end(args);

  DEBUGLOG(("ERROR: %s\n", message.cdata()));
  // TODO: central logger
  amp_message_box( GUI::GetFrameWindow(), "PM123 Error", message, MB_ERROR | MB_OK | MB_MOVEABLE );
}

/* Creates and displays a error message window.
   The specified owner window is disabled. */
void amp_verror( HWND owner, const char* format, va_list va )
{ xstring message = xstring::vsprintf(format, va);

  DEBUGLOG(("ERROR: %x, %s\n", owner, message.cdata()));
  amp_message_box( owner, "PM123 Error", message, MB_ERROR | MB_OK | MB_MOVEABLE );
}
void amp_error( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  amp_verror( owner, format, args );
  va_end(args);
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
/*bool amp_show_help( SHORT resid )
{ DEBUGLOG(("amp_show_help(%u)\n", resid));
  return WinSendMsg( hhelp, HM_DISPLAY_HELP,
    MPFROMSHORT( resid ), MPFROMSHORT( HM_RESOURCEID )) == 0;
}*/

/* global init */
/*void dlg_init()
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
    
  WinSetHook( amp_player_hab(), HMQ_CURRENT, HK_HELP, (PFN)&HelpHook, 0 );  
}

void dlg_uninit()
{ if (hhelp != NULLHANDLE)
    WinDestroyHelpInstance(hhelp);
}*/

