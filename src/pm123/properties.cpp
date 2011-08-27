/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 * Copyright 2004      Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2011 Marcel Mueller
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
#define  INCL_ERRORS
#include "properties.h"
#include "configuration.h"
#include "windowbase.h"
#include "pm123.rc.h"
#include "dialog.h"
#include "filedlg.h"
#include "gui.h" // ShowHelp
#include "controller.h"
#include "copyright.h"
#include "plugman.h"
#include "decoder.h"
#include "visual.h"
#include "123_util.h"
#include "pm123.h"
#include <utilfct.h>
#include <cpp/pmutils.h>
#include <os2.h>
#include <stdio.h>


class PropertyDialog : public NotebookDialogBase
{public:
  enum
  { CFG_REFRESH      = WM_USER+1 // /
  , CFG_GLOB_BUTTON  = WM_USER+3 // Button-ID
  , CFG_CHANGE       = WM_USER+5 // &const amp_cfg
  , CFG_SAVE         = WM_USER+6 // &amp_cfg
  };

 private:
  class SettingsPageBase : public PageBase
  {public:
    SettingsPageBase(PropertyDialog& parent, USHORT id)
    : PageBase(parent, id, NULLHANDLE)//, DF_AutoResize)
    {}
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };
  class Settings1Page : public SettingsPageBase
  {public:
    Settings1Page(PropertyDialog& parent)
    : SettingsPageBase(parent, CFG_SETTINGS1)
    { MajorTitle = "~Behavior"; MinorTitle = "General behavior"; }
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };
  class Settings2Page : public SettingsPageBase
  {public:
    Settings2Page(PropertyDialog& parent)
    : SettingsPageBase(parent, CFG_SETTINGS2)
    { MinorTitle = "Playlist behavior"; }
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };
  class SystemSettingsPage : public SettingsPageBase
  {public:
    SystemSettingsPage(PropertyDialog& parent)
    : SettingsPageBase(parent, CFG_IOSETTINGS)
    { MajorTitle = "~System settings"; }
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };
  class DisplaySettingsPage : public SettingsPageBase
  {public:
    DisplaySettingsPage(PropertyDialog& parent)
    : SettingsPageBase(parent, CFG_DISPLAY1)
    { MajorTitle = "~Display"; }
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };
  class PluginPage : public PageBase
  {protected: // Configuration
    PluginList   List;        // List to visualize
    int_ptr<Plugin> Selected; // currently selected plugin
   private:
    xstring      UndoCfg;
    AtomicUnsigned Requests;
    class_delegate<PluginPage, const PluginEventArgs> PlugmanDeleg;

   public:
    PluginPage(PropertyDialog& parent, ULONG resid, PLUGIN_TYPE type,
                   const char* minor, const char* major = NULL);
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void         RequestList() { if (!Requests.bitset(0)) PostMsg(CFG_REFRESH, 0, 0); }
    void         RequestInfo() { if (!Requests.bitset(1)) PostMsg(CFG_REFRESH, 0, 0); }
    virtual void RefreshList();
    virtual void RefreshInfo();
    virtual void SetParams(Plugin* pp);
   private:
    ULONG        AddPlugin();
    void         PlugmanNotification(const PluginEventArgs& args);
  };
  class DecoderPage : public PluginPage
  {public:
    DecoderPage(PropertyDialog& parent)
    : PluginPage(parent, CFG_DEC_CONFIG, PLUGIN_DECODER, "Decoder Plug-ins", "~Plug-ins")
    {}
   protected:
    virtual void RefreshInfo();
    virtual void SetParams(Plugin* pp);
  };
  class AboutPage : public PageBase
  {public:
    AboutPage(PropertyDialog& parent)
    : PageBase(parent, CFG_ABOUT, NULLHANDLE)//, DF_AutoResize)
    { MajorTitle = "~About"; }
   protected:
    virtual void    OnInit();
  };

 public:
  PropertyDialog(HWND owner);
  void Process();
 protected:
  virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
};


/* Processes messages of the display page of the setup notebook. */
MRESULT PropertyDialog::SettingsPageBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  { case WM_INITDLG:
      do_warpsans(GetHwnd());
      PostMsg(CFG_CHANGE, MPFROMP(&Cfg::Get()), 0);
      break;

    case CFG_GLOB_BUTTON:
    { volatile const amp_cfg* data = &Cfg::Get();
      switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        data = &Cfg::Default;
       case PB_UNDO:
        PostMsg(CFG_CHANGE, MPFROMP(data), 0);
      }
    }
    case WM_COMMAND:
    case WM_CONTROL:
      return 0;
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

/* Processes messages of the setings page of the setup notebook. */
MRESULT PropertyDialog::Settings1Page::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  { case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);

      CheckButton(CB_PLAYONLOAD,    cfg.playonload   );
      CheckButton(CB_RETAINONEXIT,  cfg.retainonexit );
      CheckButton(CB_RETAINONSTOP,  cfg.retainonstop );
      CheckButton(CB_RESTARTONSTART,cfg.restartonstart);

      CheckButton(CB_TURNAROUND,    cfg.autoturnaround);
      CheckButton(RB_SONGONLY +     cfg.altnavig, TRUE);

      return 0;
    }

    case CFG_GLOB_BUTTON:
    { volatile const amp_cfg* data = &Cfg::Get();
      switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        data = &Cfg::Default;
       case PB_UNDO:
        PostMsg(CFG_CHANGE, MPFROMP(data), 0);
      }
      return 0;
    }

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);
      cfg.playonload  = QueryButtonCheckstate(CB_PLAYONLOAD   );
      cfg.retainonexit= QueryButtonCheckstate(CB_RETAINONEXIT );
      cfg.retainonstop= QueryButtonCheckstate(CB_RETAINONSTOP );
      cfg.restartonstart= QueryButtonCheckstate(CB_RESTARTONSTART);

      cfg.autoturnaround = QueryButtonCheckstate(CB_TURNAROUND );
      if (QueryButtonCheckstate(RB_SONGONLY ))
        cfg.altnavig = CFG_ANAV_SONG;
      else if (QueryButtonCheckstate(RB_SONGTIME ))
        cfg.altnavig = CFG_ANAV_SONGTIME;
      else if (QueryButtonCheckstate(RB_TIMEONLY ))
        cfg.altnavig = CFG_ANAV_TIME;
    }
  }
  return SettingsPageBase::DlgProc(msg, mp1, mp2);
}

MRESULT PropertyDialog::Settings2Page::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  { case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      CheckButton(CB_AUTOUSEPL,     cfg.autouse      );
      CheckButton(CB_RECURSEDND,    cfg.recurse_dnd  );
      CheckButton(CB_SORTFOLDERS,   cfg.sort_folders );
      CheckButton(CB_FOLDERSFIRST,  cfg.folders_first);
      CheckButton(CB_AUTOAPPENDDND, cfg.append_dnd   );
      CheckButton(CB_AUTOAPPENDCMD, cfg.append_cmd   );
      CheckButton(CB_QUEUEMODE,     cfg.queue_mode   );
      return 0;
    }

    case CFG_GLOB_BUTTON:
    { volatile const amp_cfg* data = &Cfg::Get();
      switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        data = &Cfg::Default;
       case PB_UNDO:
        PostMsg(CFG_CHANGE, MPFROMP(data), 0);
      }
      return 0;
    }

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);
      cfg.autouse      = QueryButtonCheckstate(CB_AUTOUSEPL    );
      cfg.recurse_dnd  = QueryButtonCheckstate(CB_RECURSEDND   );
      cfg.sort_folders = QueryButtonCheckstate(CB_SORTFOLDERS  );
      cfg.folders_first= QueryButtonCheckstate(CB_FOLDERSFIRST );
      cfg.append_dnd   = QueryButtonCheckstate(CB_AUTOAPPENDDND);
      cfg.append_cmd   = QueryButtonCheckstate(CB_AUTOAPPENDCMD);
      cfg.queue_mode   = QueryButtonCheckstate(CB_QUEUEMODE    );
      return 0;
    }
  }
  return SettingsPageBase::DlgProc(msg, mp1, mp2);
}

MRESULT PropertyDialog::SystemSettingsPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  { case WM_INITDLG:
      SetSpinbuttonLimits(SB_TIMEOUT,    1,  300, 4 );
      SetSpinbuttonLimits(SB_BUFFERSIZE, 0, 2048, 4 );
      SetSpinbuttonLimits(SB_FILLBUFFER, 1,  100, 4 );
      SetSpinbuttonLimits(SB_NUMWORKERS, 1,    9, 1 );
      SetSpinbuttonLimits(SB_DLGWORKERS, 0,    9, 1 );
      break;

    case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      char buffer[1024];
      const char* cp;
      size_t l;

      SetItemText(EF_PIPE, cfg.pipe_name );

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
      SetItemText(EF_PROXY_HOST, buffer );
      SetItemText(EF_PROXY_PORT, cp );
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
      SetItemText(EF_PROXY_USER, buffer );
      SetItemText(EF_PROXY_PASS, cp );

      CheckButton(CB_FILLBUFFER, cfg.buff_wait );

      SetSpinbuttomValue(SB_TIMEOUT, cfg.conn_timeout);
      SetSpinbuttomValue(SB_BUFFERSIZE, cfg.buff_size);
      SetSpinbuttomValue(SB_FILLBUFFER, cfg.buff_fill);

      SetSpinbuttomValue(SB_NUMWORKERS, cfg.num_workers);
      SetSpinbuttomValue(SB_DLGWORKERS, cfg.num_dlg_workers);
      return 0;
    }

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_FILLBUFFER &&
        ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
      {
        BOOL fill = QueryButtonCheckstate(CB_FILLBUFFER );
        EnableControl(SB_FILLBUFFER, fill );
      }
      return 0;

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);

      cfg.pipe_name = WinQueryDlgItemXText(GetHwnd(), EF_PIPE);

      cfg.buff_size = QuerySpinbuttonValue(SB_BUFFERSIZE);
      cfg.buff_fill = QuerySpinbuttonValue(SB_FILLBUFFER);
      cfg.buff_wait = QueryButtonCheckstate(CB_FILLBUFFER);
      cfg.conn_timeout = QuerySpinbuttonValue(SB_TIMEOUT);

      size_t len1 = WinQueryDlgItemTextLength(GetHwnd(), EF_PROXY_HOST);
      size_t len2 = WinQueryDlgItemTextLength(GetHwnd(), EF_PROXY_PORT);
      if (len2)
        ++len2;
      char* cp = cfg.proxy.allocate(len1+len2);
      WinQueryDlgItemText(GetHwnd(), EF_PROXY_HOST, len1+1, cp);
      if (len2 > 1)
      { cp += len1;
        *cp++ = ':';
        WinQueryDlgItemText(GetHwnd(), EF_PROXY_PORT, len2, cp);
      }

      len1 = WinQueryDlgItemTextLength(GetHwnd(), EF_PROXY_USER);
      len2 = WinQueryDlgItemTextLength(GetHwnd(), EF_PROXY_PASS);
      if (len2)
        ++len2;
      cp = cfg.auth.allocate(len1+len2);
      WinQueryDlgItemText(GetHwnd(), EF_PROXY_USER, len1+1, cp);
      if (len2 > 1)
      { cp += len1;
        *cp++ = ':';
        WinQueryDlgItemText(GetHwnd(), EF_PROXY_PASS, len2, cp);
      }

      cfg.num_workers = QuerySpinbuttonValue(SB_NUMWORKERS);
      cfg.num_dlg_workers = QuerySpinbuttonValue(SB_DLGWORKERS);

      return 0;
    }
  }
  return SettingsPageBase::DlgProc(msg, mp1, mp2);
}

/* Processes messages of the display page of the setup notebook. */
MRESULT PropertyDialog::DisplaySettingsPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  static FATTRS  font_attrs;
  static LONG    font_size;

  switch (msg)
  { case WM_INITDLG:
      SetSpinbuttonLimits(SB_DOCK, 0, 30, 2);
      break;

    case CFG_CHANGE:
    {
      if (mp1)
      { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
        CheckButton   (CB_DOCK, cfg.dock_windows );
        EnableControl (SB_DOCK, cfg.dock_windows );
        SetSpinbuttomValue(SB_DOCK, cfg.dock_margin);
        CheckButton   (CB_SAVEWNDPOSBYOBJ, cfg.win_pos_by_obj );

        // load GUI
        CheckButton(RB_DISP_FILENAME   + cfg.viewmode, TRUE );
        CheckButton(RB_SCROLL_INFINITE + cfg.scroll,   TRUE );
        CheckButton(CB_SCROLL_AROUND,    cfg.scroll_around  );
        CheckButton(CB_USE_SKIN_FONT,    cfg.font_skinned   );
        EnableControl(PB_FONT_SELECT, !cfg.font_skinned );
        EnableControl(ST_FONT_SAMPLE, !cfg.font_skinned );

        font_attrs = cfg.font_attrs;
        font_size  = cfg.font_size;
      }
      // change sample font
      xstring font_name  = amp_font_attrs_to_string( font_attrs, font_size );
      SetItemText(ST_FONT_SAMPLE, font_name );
      WinSetPresParam(WinWindowFromID( GetHwnd(), ST_FONT_SAMPLE ), PP_FONTNAMESIZE, font_name.length() +1, (PVOID)font_name.cdata() );
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

        WinFontDlg( HWND_DESKTOP, GetHwnd(), &fontdialog );

        if( fontdialog.lReturn == DID_OK )
        {
          font_attrs = fontdialog.fAttrs;
          font_size  = fontdialog.fxPointSize >> 16;
          PostMsg(CFG_CHANGE, 0, 0);
        }
      }
      return 0;

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case CB_USE_SKIN_FONT:
        if ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED )
        {
          BOOL use = QueryButtonCheckstate(CB_USE_SKIN_FONT );
          EnableControl(PB_FONT_SELECT, !use );
          EnableControl(ST_FONT_SAMPLE, !use );
        }
        break;

       case CB_DOCK:
        if ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED )
        {
          BOOL use = QueryButtonCheckstate(CB_DOCK );
          EnableControl(SB_DOCK, use );
        }
      }
      return 0;

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);
      cfg.dock_windows  = QueryButtonCheckstate(CB_DOCK);
      cfg.dock_margin   = QuerySpinbuttonValue(SB_DOCK);
      cfg.win_pos_by_obj= QueryButtonCheckstate(CB_SAVEWNDPOSBYOBJ);

      cfg.scroll        = (cfg_scroll)(QuerySelectedRadiobutton(RB_SCROLL_INFINITE) - RB_SCROLL_INFINITE);
      cfg.scroll_around = QueryButtonCheckstate(CB_SCROLL_AROUND);
      cfg.viewmode      = (cfg_disp)(QuerySelectedRadiobutton(RB_DISP_FILENAME) - RB_DISP_FILENAME);

      cfg.font_skinned  = QueryButtonCheckstate(CB_USE_SKIN_FONT);
      cfg.font_size     = font_size;
      cfg.font_attrs    = font_attrs;
      return 0;
    }
  }
  return SettingsPageBase::DlgProc(msg, mp1, mp2);
}

PropertyDialog::PluginPage::PluginPage
  (PropertyDialog& parent, ULONG resid, PLUGIN_TYPE type, const char* minor, const char* major)
: PageBase(parent, resid, NULLHANDLE, DF_AutoResize)
, List(type)
, PlugmanDeleg(*this, &PluginPage::PlugmanNotification)
{ MajorTitle = major;
  MinorTitle = minor;
  Plugin::GetPlugins(List);
  UndoCfg = List.Serialize();
  Plugin::GetChangeEvent() += PlugmanDeleg;
}

void PropertyDialog::PluginPage::RefreshList()
{ DEBUGLOG(("PropertyDialog::PluginPage::RefreshList()\n"));
  HWND lb = WinWindowFromID(GetHwnd(), LB_PLUGINS);
  PMASSERT(lb != NULLHANDLE);
  WinSendMsg(lb, LM_DELETEALL, 0, 0);

  Plugin::GetPlugins(List, false);
  int_ptr<Plugin> const* ppp;
  int selected = LIT_NONE;
  for (ppp = List.begin(); ppp != List.end(); ++ppp)
  { Plugin* pp = *ppp;
    if (pp == Selected)
      selected = ppp - List.begin();
    xstring title = pp->ModRef.Key;
    if (List.Type == PLUGIN_VISUAL && ((Visual*)pp)->GetProperties().skin)
      title = title + " (Skin)";
    WinSendMsg(lb, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(title.cdata()));
  }
  if (selected != LIT_NONE)
    lb_select(GetHwnd(), LB_PLUGINS, selected);
  else
  { Selected.reset();
    RequestInfo();
  }
}

void PropertyDialog::PluginPage::RefreshInfo()
{ DEBUGLOG(("PropertyDialog::PluginPage::RefreshInfo() - %p\n", Selected.get()));
  if ( Selected == NULL
    || (List.Type == PLUGIN_VISUAL && ((Visual&)*Selected).GetProperties().skin) )
  { // The following functions give an error if no such buttons. This is ignored.
    if (List.Type != PLUGIN_OUTPUT)
      SetItemText(PB_PLG_ENABLE, "~Enable");
    EnableControl(PB_PLG_UNLOAD, FALSE);
    EnableControl(PB_PLG_UP,     FALSE);
    EnableControl(PB_PLG_DOWN,   FALSE);
    EnableControl(PB_PLG_ENABLE, FALSE);
  } else
  { if (List.Type == PLUGIN_OUTPUT)
      EnableControl(PB_PLG_ENABLE, !Selected->GetEnabled());
    else
    { SetItemText(PB_PLG_ENABLE, Selected->GetEnabled() ? "Disabl~e" : "~Enable");
      EnableControl(PB_PLG_ENABLE, TRUE);
    }
    EnableControl(PB_PLG_UNLOAD, TRUE);
    EnableControl(PB_PLG_UP,     Selected != List[0]);
    EnableControl(PB_PLG_DOWN,   Selected != List[List.size()-1]);
  }
  if (Selected == NULL)
  { SetItemText(ST_PLG_AUTHOR, "");
    SetItemText(ST_PLG_DESC,   "");
    SetItemText(ST_PLG_LEVEL,  "");
    EnableControl(PB_PLG_CONFIG, FALSE);
  } else
  { char buffer[64];
    const PLUGIN_QUERYPARAM& params = Selected->ModRef.GetParams();
    SetItemText(ST_PLG_AUTHOR, params.author);
    SetItemText(ST_PLG_DESC,   params.desc);
    snprintf(buffer, sizeof buffer,        "Interface level %i%s",
      params.interface, params.interface >= PLUGIN_INTERFACE_LEVEL ? "" : " (virtualized)");
    SetItemText(ST_PLG_LEVEL,  buffer);
    EnableControl(PB_PLG_CONFIG, params.configurable);
  }
}

ULONG PropertyDialog::PluginPage::AddPlugin()
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
  amp_file_dlg(HWND_DESKTOP, GetHwnd(), &filedialog);

  if (filedialog.lReturn == DID_OK)
  { try
    { int_ptr<Plugin> pp(Plugin::Deserialize(filedialog.szFullFile, List.Type));
      Plugin::AppendPlugin(pp);
      Selected = pp;
      /* TODO: still required?
      if (rc & PLUGIN_VISUAL)
        vis_init(List->size()-1);*/
      if ((List.Type & PLUGIN_FILTER) && Ctrl::IsPlaying())
        amp_info(GetHwnd(), "This filter will only be enabled after playback stops.");
      RequestList();
    } catch (const ModuleException& ex)
    { pm123_display_error(ex.GetErrorText());
    }
  }
  return rc;
}

void PropertyDialog::PluginPage::SetParams(Plugin* pp)
{ EnableControl(PB_PLG_SET, FALSE);
}

void PropertyDialog::PluginPage::PlugmanNotification(const PluginEventArgs& args)
{ if (args.Plug.PluginType == List.Type)
  { switch (args.Operation)
    {case PluginEventArgs::Enable:
     case PluginEventArgs::Disable:
      if (&args.Plug == Selected)
        RequestInfo();
      break;
     case PluginEventArgs::Load:
     case PluginEventArgs::Unload:
      RequestList();
     default:;
    }
  }
}

void PropertyDialog::DecoderPage::RefreshInfo()
{ PluginPage::RefreshInfo();
  if (Selected == NULL)
  { HWND ctrl = WinWindowFromID(GetHwnd(), ML_DEC_FILETYPES);
    WinSetWindowText(ctrl, "");
    WinEnableWindow (ctrl, FALSE);
    ctrl = WinWindowFromID(GetHwnd(), CB_DEC_TRYOTHER);
    WinSendMsg      (ctrl, BM_SETCHECK, MPFROMSHORT(FALSE), 0);
    WinEnableWindow (ctrl, FALSE);
    ctrl = WinWindowFromID(GetHwnd(), CB_DEC_SERIALIZE);
    WinSendMsg      (ctrl, BM_SETCHECK, MPFROMSHORT(FALSE), 0);
    WinEnableWindow (ctrl, FALSE);
    EnableControl(PB_PLG_SET, FALSE);
  } else
  { stringmap_own sm(20);
    Selected->GetParams(sm);
    // TODO: Decoder supported file types vs. user file types.
    { const vector<const DECODER_FILETYPE>& filetypes = ((Decoder*)Selected.get())->GetFileTypes();
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
      HWND ctrl = WinWindowFromID(GetHwnd(), ML_DEC_FILETYPES);
      WinSetWindowText(ctrl, cp);
      WinEnableWindow (ctrl, TRUE);
    }
    stringmapentry* smp; // = sm.find("filetypes");
    smp = sm.find("tryothers");
    bool* b = smp && smp->Value ? url123::parseBoolean(smp->Value) : NULL;
    HWND ctrl = WinWindowFromID(GetHwnd(), CB_DEC_TRYOTHER);
    WinSendMsg     (ctrl, BM_SETCHECK, MPFROMSHORT(b && *b), 0);
    WinEnableWindow(ctrl, !!b);
    smp = sm.find("serializeinfo");
    b = smp && smp->Value ? url123::parseBoolean(smp->Value) : NULL;
    ctrl = WinWindowFromID(GetHwnd(), CB_DEC_SERIALIZE);
    WinSendMsg     (ctrl, BM_SETCHECK, MPFROMSHORT(b && *b), 0);
    WinEnableWindow(ctrl, !!b);
    EnableControl(PB_PLG_SET, FALSE);
  }
}

void PropertyDialog::DecoderPage::SetParams(Plugin* pp)
{ HWND ctrl = WinWindowFromID(GetHwnd(), ML_DEC_FILETYPES);
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
  pp->SetParam("tryothers", QueryButtonCheckstate(CB_DEC_TRYOTHER) ? "1" : "0");
  pp->SetParam("serializeinfo", QueryButtonCheckstate(CB_DEC_SERIALIZE) ? "1" : "0");
}

/* Processes messages of the plug-ins pages of the setup notebook. */
MRESULT PropertyDialog::PluginPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PropertyDialog::PluginPage::DlgProc(%p, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  LONG i;
  switch (msg)
  { case CFG_REFRESH:
    { unsigned req = Requests.swap(0);
      if (req & 1)
        RefreshList();
      if (req & 2)
        RefreshInfo();
      return 0;
    }
    case CFG_GLOB_BUTTON:
    { switch (SHORT1FROMMP(mp1))
      {case PB_DEFAULT:
        { PluginList def(List.Type);
          def.LoadDefaults();
          Plugin::SetPluginList(def);
          // The GUI is updated by the PluginChange event.
        }
        break;
       case PB_UNDO:
        { PluginList list(List.Type);
          list.Deserialize(UndoCfg);
          Plugin::SetPluginList(list);
          // The GUI is updated by the PluginChange event.
        }
        break;
      }
      return 0;
    }

    case WM_INITDLG:
      do_warpsans(GetHwnd());
      RequestList();
      return 0;

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case LB_PLUGINS:
        switch (SHORT2FROMMP(mp1))
        {case LN_SELECT:
          i = WinQueryLboxSelectedItem(HWNDFROMMP(mp2));
          Selected = (size_t)i < List.size() ? List[i] : NULL;
          RequestInfo();
          break;
         case LN_ENTER:
          i = WinQueryLboxSelectedItem(HWNDFROMMP(mp2));
          if ((size_t)i < List.size())
            List[i]->ModRef.Config(GetHwnd());
          break;
        }
        break;

       case ML_DEC_FILETYPES:
        switch (SHORT2FROMMP(mp1))
        {case MLN_CHANGE:
          EnableControl(PB_PLG_SET, TRUE);
        }
        break;

       case CB_DEC_TRYOTHER:
       case CB_DEC_SERIALIZE:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
          EnableControl(PB_PLG_SET, TRUE);
        }
        break;

      }
      return 0;

    case WM_COMMAND:
      i = lb_cursored(GetHwnd(), LB_PLUGINS);
      switch (SHORT1FROMMP(mp1))
      {case PB_PLG_UNLOAD:
        if ((size_t)i < List.size())
        { List.erase(i);
          Plugin::SetPluginList(List);
        }
        break;

       case PB_PLG_ADD:
        AddPlugin();
        break;

       case PB_PLG_UP:
        if (i > 0 && (size_t)i < List.size())
        { List.move(i, i-1);
          Plugin::SetPluginList(List);
        }
        break;

       case PB_PLG_DOWN:
        if ((size_t)i < List.size()-1)
        { List.move(i, i+1);
          Plugin::SetPluginList(List);
        }
        break;

       case PB_PLG_ENABLE:
        if ((size_t)i < List.size())
        { Plugin* pp = List[i];
          pp->SetEnabled(!pp->GetEnabled());
        }
        break;

       case PB_PLG_CONFIG:
        if ((size_t)i < List.size())
          List[i]->ModRef.Config(GetHwnd());
        break;

       case PB_PLG_SET:
        if ((size_t)i < List.size())
          SetParams(List[i]);
        break;
      }
      return 0;
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}


void PropertyDialog::AboutPage::OnInit()
{ PageBase::OnInit();
  do_warpsans( GetHwnd() );
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
  SetItemText(ST_BUILT, built );
  SetItemText(ST_AUTHORS, SDG_AUT );
  SetItemText(ST_CREDITS, SDG_MSG );
}


PropertyDialog::PropertyDialog(HWND owner)
: NotebookDialogBase(DLG_CONFIG, NULLHANDLE)
{ Pages.append() = new Settings1Page(*this);
  Pages.append() = new Settings2Page(*this);
  Pages.append() = new SystemSettingsPage(*this);
  Pages.append() = new DisplaySettingsPage(*this);
  Pages.append() = new DecoderPage(*this);
  Pages.append() = new PluginPage(*this, CFG_FIL_CONFIG, PLUGIN_FILTER, "Filter Plug-ins");
  Pages.append() = new PluginPage(*this, CFG_OUT_CONFIG, PLUGIN_OUTPUT, "Output Plug-ins");
  Pages.append() = new PluginPage(*this, CFG_VIS_CONFIG, PLUGIN_VISUAL, "Visual Plug-ins");
  Pages.append() = new AboutPage(*this);
  StartDialog(owner, NB_CONFIG);
}

/* Processes messages of the setup dialog. */
MRESULT PropertyDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch( msg )
  { case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
        case PB_UNDO:
        case PB_DEFAULT:
        { HWND page = NULLHANDLE;
          LONG id = (LONG)SendItemMsg(NB_CONFIG, BKM_QUERYPAGEID, 0, MPFROM2SHORT(BKA_TOP,BKA_MAJOR));
          if( id && id != BOOKERR_INVALID_PARAMETERS )
            page = (HWND)SendItemMsg(NB_CONFIG, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(id), 0 );
          if( page && page != (HWND)BOOKERR_INVALID_PARAMETERS )
            WinPostMsg(page, CFG_GLOB_BUTTON, mp1, mp2);
          return MRFROMLONG(1L);
        }

        case PB_HELP:
          GUI::ShowHelp(IDH_PROPERTIES);
          return 0;
      }
      return 0;

    case WM_INITDLG:
      Cfg::RestWindowPos(GetHwnd());
      break;

    case WM_DESTROY:
      Cfg::SaveWindowPos(GetHwnd());
      break;

    case WM_WINDOWPOSCHANGED:
      if(((SWP*)mp1)->fl & SWP_SIZE ) {
        nb_adjust( GetHwnd(), (SWP*)mp1 );
      }
      break;
  }
  return NotebookDialogBase::DlgProc(msg, mp1, mp2);
}

void PropertyDialog::Process()
{
  NotebookDialogBase::Process();
  // Save settings
  { Cfg::ChangeAccess cfg;
    for (PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
      WinSendMsg((*pp)->GetHwnd(), CFG_SAVE, MPFROMP(&cfg), 0);
  }
  Cfg::SaveIni();
}

/* Creates the properties dialog. */
void cfg_properties(HWND owner)
{
  PropertyDialog dialog(owner);
  // TODO? WinSetFocus( HWND_DESKTOP, book );
  dialog.Process();
}
