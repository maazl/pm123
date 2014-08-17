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
#include "../configuration.h"
#include "gui.h"
#include "../123_util.h"
#include <utilfct.h> // for do_warpsans
#include <fileutil.h>
#include <cpp/pmutils.h>
#include <cpp/smartptr.h>
#include <xio.h>
#include <os2.h>
#include <stdio.h>
#include <sys/stat.h>

#include "pm123.rc.h"

#include <debuglog.h>


#define  AMP_REFRESH_CONTROLS   ( WM_USER + 1000 ) /* 0,         0                            */


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

  USHORT rc = WinMessageBox(HWND_DESKTOP, owner, (PSZ)message, padded_title, 0, style);
  PMASSERT(rc != MBID_ERROR);
  return rc;
}


/* Wizard function for the default entry "File..." */
ULONG DLLENTRY amp_file_wizard( HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param )
{ DEBUGLOG(("amp_file_wizard(%p, %s, %p, %p)\n", owner, title, callback, param));

  FILEDLG filedialog = { sizeof( FILEDLG ) };
  sco_ptr<APSZ_list> types(amp_file_types(DECODER_FILENAME));

  xstring wintitle;
  wintitle.sprintf(title, " file(s)");

  filedialog.fl             = FDS_CENTER|FDS_OPEN_DIALOG|FDS_MULTIPLESEL;
  filedialog.ulUser         = FDU_DIR_ENABLE|FDU_RECURSEBTN;
  filedialog.pszTitle       = (PSZ)wintitle.cdata(); // OS/2 and const...
  filedialog.papszITypeList = *types;
  char type[_MAX_PATH] = FDT_ALL;
  filedialog.pszIType       = type;

  xstring filedir(Cfg::Get().filedir);
  if (filedir.length() > 5)
  { // strip file:///
    const char* cp = filedir + 5;
    if (cp[2] == '/')
      cp += 3;
    size_t len = strlen(cp);
    if (len >= sizeof filedialog.szFullFile)
      len = sizeof filedialog.szFullFile - 1;
    memcpy(filedialog.szFullFile, cp, len);
    // Append / to directory
    char* cp2 = filedialog.szFullFile + len;
    if (cp2[-1] != '/' && len < sizeof filedialog.szFullFile - 1)
    { cp2[0] = '/';
      cp2[1] = 0;
    }
  } else
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

class UrlDialog : public DialogBase
{public:
  UrlDialog(HWND owner, const char* title);
  ~UrlDialog() { Destroy(); }
  const xstring GetURL()  { return WinQueryWindowXText(GetCtrl(ENT_URL)); }
  const xstring GetDesc() { return WinQueryWindowXText(GetCtrl(EF_DESC)); }
 protected:
  virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
};

UrlDialog::UrlDialog(HWND owner, const char* title)
: DialogBase(DLG_URL, NULL, DF_AutoResize)
{ StartDialog(owner);
  ControlBase(+GetHwnd()).Text(title);
}

/* Default dialog procedure for the URL dialog. */
MRESULT UrlDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_CONTROL:
    if (SHORT1FROMMP(mp1) == ENT_URL)
      switch (SHORT2FROMMP(mp1))
      {case CBN_EFCHANGE:
        // Update enabled status of the OK-Button
        DEBUGLOG(("UrlDialog::DlgProc: WM_CONTROL: CBN_EFCHANGE\n"));
        ControlBase(+GetCtrl(DID_OK)).Enabled(WinQueryWindowTextLength(HWNDFROMMP(mp2)) != 0);
        break;
       /*case CBN_ENTER:
        WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(DID_OK), MPFROM2SHORT(CMDSRC_OTHER, FALSE));
        break;*/
      }
    break;

   case WM_INITDLG:
    { do_warpsans(GetHwnd());
      Cfg::RestWindowPos(GetHwnd());

      // populate drop down list
      Playable& mru = GUI::GetUrlMRU();
      ComboBox ctrl(GetCtrl(ENT_URL));
      ctrl.TextLimit(4096);
      int_ptr<PlayableInstance> pi;
      for(;;)
      { pi = mru.GetNext(pi);
        DEBUGLOG(("amp_url_dlg_proc: WM_INITDLG %p %p\n", pi.get(), ctrl.Hwnd));
        if (pi == NULL)
          break;
        ctrl.InsertItem(pi->GetPlayable().URL);
      }
      break;
    }
   
   case WM_DESTROY:
    Cfg::SaveWindowPos(GetHwnd());
    break;

   case WM_COMMAND:
    DEBUGLOG(("amp_url_dlg_proc: WM_COMMAND: %i\n", SHORT1FROMMP(mp1)));
    if (SHORT1FROMMP(mp1) == DID_OK)
    { ControlBase ent(GetCtrl(ENT_URL));
      const xstring& text = ent.Text();
      const url123& url = amp_get_cwd().makeAbsolute(text);
      if (!url)
      { WinMessageBox(HWND_DESKTOP, GetHwnd(), xstring().sprintf("The URL \"%s\" is not well formed.", text.cdata()),
          NULL, 0, MB_CANCEL|MB_WARNING|MB_APPLMODAL|MB_MOVEABLE);
        return 0; // cancel
      }
      // Replace the text by the expanded URL
      ent.Text(url);
      break; // everything OK => continue
    }
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

/* Adds HTTP file to the playlist or load it to the player. */
ULONG DLLENTRY amp_url_wizard( HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param )
{ DEBUGLOG(("amp_url_wizard(%x, %s, %p, %p)\n", owner, title, callback, param));

  // TODO: we should not wait synchronously on the data.
  // We should update the combo box list on change instead.
  GUI::GetUrlMRU().RequestInfo(IF_Child, PRI_Sync|PRI_Normal);

  xstring wintitle;
  wintitle.sprintf(title, " URL");
  UrlDialog dialog(owner, title);
  
  ULONG ret = 300;
  if (dialog.Process() == DID_OK)
  { const xstring& url = dialog.GetURL();
    const xstring& desc = dialog.GetDesc();
    DEBUGLOG(("amp_url_wizard: %s, %s\n", url.cdata(), desc.cdata()));
    if (desc && desc.length() > 0)
    { // Have description
      ItemInfo item;
      item.alias = desc;
      INFO_BUNDLE info = {NULL};
      info.item = &item;
      (*callback)(param, url, &info, IF_Item, IF_Item);
    } else
      (*callback)(param, url, NULL, IF_None, IF_None);
    GUI::Add2MRU(GUI::GetUrlMRU(), Cfg::Get().max_recall, *Playable::GetByURL(url));
    ret = 0;
  }
  return ret;
}

url123 amp_playlist_select(HWND owner, const char* title, FD_UserOpts opts)
{
  DEBUGLOG(("amp_playlist_select(%p, %s)\n", owner, title));
  sco_ptr<APSZ_list> types(amp_file_types(DECODER_FILENAME|DECODER_PLAYLIST));

  FILEDLG filedialog = { sizeof(FILEDLG) };
  filedialog.fl             = FDS_CENTER|FDS_OPEN_DIALOG;
  filedialog.pszTitle       = (PSZ)title;
  filedialog.papszITypeList = *types;
  char type[_MAX_PATH] = "Playlist File";
  filedialog.pszIType       = type;
  filedialog.ulUser         = opts;

  xstring listdir(Cfg::Get().listdir);
  if (listdir.length() > 8)
    // strip file:///
    strlcpy(filedialog.szFullFile, listdir+8, sizeof filedialog.szFullFile);
  else
    filedialog.szFullFile[0] = 0;
  PMRASSERT(amp_file_dlg(HWND_DESKTOP, owner, &filedialog));

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

ULONG DLLENTRY amp_new_list_wizard(HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param)
{ DEBUGLOG(("amp_new_list_wizard(%x, %s, %p, %p)\n", owner, title, callback, param));

  xstring wintitle;
  wintitle.sprintf(title, " new playlist");

  const url123& url = amp_playlist_select(owner, wintitle, FDU_NONE);
  if (!url)
    return PLUGIN_NO_OP;

  TechInfo tech;
  tech.attributes = TATTR_PLAYLIST|TATTR_STORABLE;
  tech.decoder = "plist123.dll";
  tech.format =  "PM123 Playlist";
  INFO_BUNDLE info = { NULL, &tech };

  (*callback)(param, url, &info, IF_None, IF_Tech);
  return PLUGIN_OK;
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
  const xstring& desc = item.GetDisplayName();
  WinSetDlgItemText(hdlg, EF_DESC, desc);

  if (WinProcessDlg(hdlg) == DID_OK)
  { const xstring& alias = WinQueryDlgItemXText(hdlg, EF_DESC);
    Playable& p = GUI::GetDefaultBM();
    p.RequestInfo(IF_Child, PRI_Sync|PRI_Normal);
    if (alias != desc) // Don't set alias if not required.
    { // We have to copy the PlayableRef to modify it.
      PlayableRef ps(item);
      ItemInfo ii;
      ii.alias = alias;
      ps.OverrideItem(&ii);
      p.InsertItem(ps, NULL);
    } else
      p.InsertItem(item, NULL);
    // TODO: Save
    //p.Save(PlayableCollection::SaveRelativePath);
  }

  WinDestroyWindow(hdlg);
}

url123 amp_save_playlist(HWND owner, Playable& playlist, bool saveas)
{
  if (playlist.GetInfo().tech->attributes & TATTR_SONG)
  { amp_messagef(owner, MSG_ERROR, "Cannot save a song as playlist.");
    return xstring();
  }
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
    if (types->size() <= 1) // NULL terminator counts as well.
    { amp_messagef(owner, MSG_ERROR, "You do not have any playlist plug-in installed. See Properties/Decoder plug-ins.");
      return xstring();
    }

    FILEDLG filedialog = {sizeof(FILEDLG)};
    filedialog.fl             = FDS_CENTER|FDS_SAVEAS_DIALOG|FDS_ENABLEFILELB;
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
        filedialog.pszIType = (PSZ)xstring(playlist.GetInfo().tech->format).cdata();
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
    PMRASSERT(amp_file_dlg(HWND_DESKTOP, owner, &filedialog));

    if (filedialog.lReturn != DID_OK)
      return xstring();
    dest = url123::normalizeURL(filedialog.szFullFile);
    const char* cp = dest.cdata() + 5;
    if (cp[2] == '/')
      cp += 3;
    struct stat fi;
    if ( stat(cp, &fi) == 0
      && !amp_query(owner, "File %s already exists. Overwrite it?", cp) )
      goto retry;

    relative = (filedialog.ulUser & FDU_RELATIV_ON) != 0;

    // Deduce decoder and format from file dialog format.
    decoder = amp_decoder_by_type(DECODER_PLAYLIST|DECODER_WRITABLE, filedialog.pszIType, format);
    if (decoder == NULL)
    { amp_messagef(owner, MSG_ERROR, "The format to save cannot be guessed. Please select an appropriate format to save in the file dialog.");
      goto retry;
    }
  }

  if (playlist.Save(dest, decoder, format, relative))
  { amp_messagef(owner, MSG_ERROR, "Failed to create playlist \"%s\". Error %s.", dest.cdata(), xio_strerror(xio_errno()));
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


void amp_message( HWND owner, MESSAGE_TYPE type, const char* message )
{ DEBUGLOG(("amp_message(%x, %i, %s)\n", owner, type, message));
  switch (type)
  {default: // MSG_ERROR
    ASSERT(type == MSG_ERROR);
    amp_message_box(owner, "PM123 Error", message, MB_ERROR|MB_OK|MB_MOVEABLE);
    break;
   case MSG_WARNING:
    amp_message_box(owner, "PM123 Warning", message, MB_WARNING|MB_OK|MB_MOVEABLE);
    break;
   case MSG_INFO:
    amp_message_box(owner, "PM123 Information", message, MB_INFORMATION|MB_OK|MB_MOVEABLE);
    break;
  }
}
void amp_messagef( HWND owner, MESSAGE_TYPE type, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message;
  message.vsprintf(format, args);
  va_end(args);
  amp_message(owner, type, message);
}

/* Requests the user about specified action. Returns
   TRUE at confirmation or FALSE in other case. */
BOOL amp_query( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message;
  message.vsprintf(format, args);
  va_end(args);
  return amp_message_box(owner, "PM123 Query", message, MB_QUERY|MB_YESNO|MB_MOVEABLE) == MBID_YES;
}

/* Requests the user about specified action. Provodes a cancel button.
   Returns the pressed Button (MBID_xxx constants) */
USHORT amp_query3( HWND owner, const char* format, ... )
{ va_list args;
  va_start(args, format);
  xstring message;
  message.vsprintf(format, args);
  va_end(args);
  return amp_message_box( owner, "PM123 Query", message, MB_QUERY | MB_YESNOCANCEL | MB_MOVEABLE );
}
