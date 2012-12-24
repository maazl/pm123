/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 * Copyright 2004      Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2012 Marcel Mueller
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
#include "pm123.rc.h"
#include "eventhandler.h"
#include "dialog.h"
#include "filedlg.h"
#include "gui.h" // ShowHelp
#include "123_util.h" // amp_font_attrs_to_string
#include "controller.h"
#include "copyright.h"
#include "plugman.h"
#include "decoder.h"
#include "visual.h"
#include "pm123.h"
#include <utilfct.h>
#include <cpp/pmutils.h>
#include <cpp/windowbase.h>
#include <cpp/dlgcontrols.h>
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
    : PageBase(parent, id, NULLHANDLE, DF_AutoResize)
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
  class PlaybackSettingsPage : public SettingsPageBase
  {public:
    PlaybackSettingsPage(PropertyDialog& parent)
    : SettingsPageBase(parent, CFG_PLAYBACK)
    { MajorTitle = "~Playback"; }
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   private:
    //void EnableRG(bool enabled);
    void SetListContent(const cfg_rgtype types[4]);
    void GetListContent(USHORT id, cfg_rgtype types[4]);
    unsigned GetListSelection(USHORT id);
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
    AtomicUnsigned Requests;
    class_delegate<PluginPage, const PluginEventArgs> PlugmanDeleg;

   public:
    PluginPage(PropertyDialog& parent, ULONG resid, PLUGIN_TYPE type,
                   const char* minor, const char* major = NULL);
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    xstring amp_cfg::* AccessPluginCfg() const;
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
    : PageBase(parent, CFG_ABOUT, NULLHANDLE, DF_AutoResize)
    { MajorTitle = "~About"; }
   protected:
    virtual void    OnInit();
  };

 public:
  PropertyDialog(HWND owner);
 protected:
  virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
};


/* Processes messages of the display page of the setup notebook. */
MRESULT PropertyDialog::SettingsPageBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    do_warpsans(GetHwnd());
    PostMsg(CFG_CHANGE, MPFROMP(&Cfg::Get()), 0);
    break;

   case CFG_GLOB_BUTTON:
    switch (SHORT1FROMMP(mp1))
    {case PB_DEFAULT:
      PostMsg(CFG_CHANGE, MPFROMP(&Cfg::Default), 0);
      break;
     case PB_UNDO:
      PostMsg(CFG_CHANGE, MPFROMP(&Cfg::Get()), 0);
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
      // gcc parser error required + prefixes
      CheckBox(+GetCtrl(CB_PLAYONLOAD    )).SetCheckState(cfg.playonload);
      CheckBox(+GetCtrl(CB_RETAINONEXIT  )).SetCheckState(cfg.retainonexit);
      CheckBox(+GetCtrl(CB_RETAINONSTOP  )).SetCheckState(cfg.retainonstop);
      CheckBox(+GetCtrl(CB_RESTARTONSTART)).SetCheckState(cfg.restartonstart);

      CheckBox(+GetCtrl(CB_TURNAROUND    )).SetCheckState(cfg.autoturnaround);
      CheckBox(+GetCtrl(RB_SONGONLY+cfg.altnavig)).SetCheckState(true);
      CheckBox(+GetCtrl(RB_ALTKEY+cfg.altbutton)).SetCheckState(true);
      return 0;
    }

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);
      cfg.playonload     = CheckBox(GetCtrl(CB_PLAYONLOAD)).QueryCheckState();
      cfg.retainonexit   = CheckBox(GetCtrl(CB_RETAINONEXIT)).QueryCheckState();
      cfg.retainonstop   = CheckBox(GetCtrl(CB_RETAINONSTOP)).QueryCheckState();
      cfg.restartonstart = CheckBox(GetCtrl(CB_RESTARTONSTART)).QueryCheckState();

      cfg.autoturnaround = CheckBox(GetCtrl(CB_TURNAROUND)).QueryCheckState();
      cfg.altnavig       = (cfg_anav)RadioButton(GetCtrl(RB_SONGONLY)).QueryCheckIndex();
      cfg.altbutton      = (cfg_button)RadioButton(GetCtrl(RB_ALTKEY)).QueryCheckIndex();
    }
  }
  return SettingsPageBase::DlgProc(msg, mp1, mp2);
}

MRESULT PropertyDialog::Settings2Page::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  { case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      // gcc parser error required + prefixes
      CheckBox(+GetCtrl(CB_AUTOUSEPL    )).SetCheckState(cfg.autouse);
      CheckBox(+GetCtrl(CB_AUTOSAVEPL   )).SetCheckState(cfg.autosave);
      CheckBox(+GetCtrl(CB_RECURSEDND   )).SetCheckState(cfg.recurse_dnd);
      CheckBox(+GetCtrl(CB_FOLDERSFIRST )).SetCheckState(cfg.folders_first);
      CheckBox(+GetCtrl(RB_ITEMNAVTO + cfg.itemaction)).SetCheckState(true);
      CheckBox(+GetCtrl(RB_DNDLOAD   + cfg.append_dnd)).SetCheckState(true);
      CheckBox(+GetCtrl(RB_CMDLOAD   + cfg.append_cmd)).SetCheckState(true);
      CheckBox(+GetCtrl(CB_QUEUEMODE    )).SetCheckState(cfg.queue_mode);
      return 0;
    }

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);
      cfg.autouse      = CheckBox(GetCtrl(CB_AUTOUSEPL   )).QueryCheckState();
      cfg.autosave     = CheckBox(GetCtrl(CB_AUTOSAVEPL  )).QueryCheckState();
      cfg.recurse_dnd  = CheckBox(GetCtrl(CB_RECURSEDND  )).QueryCheckState();
      cfg.folders_first= CheckBox(GetCtrl(CB_FOLDERSFIRST)).QueryCheckState();
      cfg.itemaction   = (cfg_action)RadioButton(GetCtrl(RB_ITEMNAVTO)).QueryCheckIndex();
      cfg.append_dnd   = CheckBox(GetCtrl(RB_DNDQUEUE    )).QueryCheckState();
      cfg.append_cmd   = CheckBox(GetCtrl(RB_CMDQUEUE    )).QueryCheckState();
      cfg.queue_mode   = CheckBox(GetCtrl(CB_QUEUEMODE   )).QueryCheckState();
      return 0;
    }
  }
  return SettingsPageBase::DlgProc(msg, mp1, mp2);
}

void PropertyDialog::PlaybackSettingsPage::SetListContent(const cfg_rgtype types[4])
{ DEBUGLOG(("PropertyDialog::PlaybackSettingsPage::SetListContent({%u,%u,%u,%u})\n",
    types[0], types[1], types[2], types[3]));
  static const char* text[4] =
  { "Album gain"
  , "Album gain, prevent clipping"
  , "Track gain"
  , "Track gain, prevent clipping"
  };
  // prepare new items array
  const char* items[4];
  unsigned contains = 0;
  size_t i;
  size_t count = 0;
  for (i = 0; i < 4; ++i)
  { unsigned v = types[i] -1;
    if (v >= 4)
      break;
    items[i] = text[v];
    contains |= 1 << v;
    ++count;
  }
  // set new values
  ListBox lb(GetCtrl(LB_RG_LIST));
  lb.DeleteAll();
  lb.InsertItems(items, count);
  // set item handles
  for (i = 0; i < count; ++i)
    lb.SetHandle(i, types[i]);

  // prepare remaining items array
  count = 0;
  for (i = 0; i < 4; ++i)
    if (!(contains & (1 << i)))
      items[count++] = text[i];
  // set new values
  lb = ListBox(GetCtrl(LB_RG_AVAILABLE));
  lb.DeleteAll();
  lb.InsertItems(items, count);
  // set item handles
  count = 0;
  for (i = 0; i < 4; ++i)
    if (!(contains & (1 << i)))
      lb.SetHandle(count++, i+1);
}

void PropertyDialog::PlaybackSettingsPage::GetListContent(USHORT id, cfg_rgtype types[4])
{ ListBox lb(GetCtrl(id));
  size_t count = lb.Count();
  for (size_t i = 0; i < 4; ++i)
    types[i] = i >= count ? CFG_RG_NONE : (cfg_rgtype)lb.QueryHandle(i);
}


unsigned PropertyDialog::PlaybackSettingsPage::GetListSelection(USHORT id)
{ unsigned selected = 0;
  ListBox lb(GetCtrl(id));
  int i = LIT_FIRST;
  while ((i =  lb.QuerySelection(i)) != LIT_NONE)
    selected |= 1 << i;
  return selected;
}

const struct PriorityEntry
{ const char Text[20];
  int        Priority;
} Priorities[] =
{ { "Normal +0", 0x200 }
, { "Normal +10", 0x20a }
, { "Normal +20", 0x214 }
, { "Normal +26", 0x21a }
, { "Normal +31", 0x21f }
, { "Foreground +0", 0x400 }
, { "Foreground +10", 0x40a }
, { "Foreground +20", 0x414 }
, { "Foreground +26", 0x41a }
, { "Foreground +31", 0x41f }
, { "Time critical +0", 0x300 }
, { "Time critical +5", 0x305 }
, { "Time critical +9", 0x309 }
, { "Time critical +10", 0x30a }
, { "Time critical +19", 0x313 }
};

static int ComparePriority(const int* key, const PriorityEntry* data)
{ return *key - data->Priority;
}

MRESULT PropertyDialog::PlaybackSettingsPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    { MRESULT mr = SettingsPageBase::DlgProc(msg, mp1, mp2);
      SpinButton(+GetCtrl(SB_RG_PREAMP      )).SetLimits(-12, +12, 3);
      SpinButton(+GetCtrl(SB_RG_PREAMP_OTHER)).SetLimits(-12, +12, 3);
      ComboBox cb(GetCtrl(CB_PRI_NORM));
      size_t i;
      for (i = 0; i < 10; ++i)
        cb.InsertItem(Priorities[i].Text, LIT_END);
      cb = ComboBox(GetCtrl(CB_PRI_HIGH));
      for (i = 5; i < 15; ++i)
        cb.InsertItem(Priorities[i].Text, LIT_END);
      SpinButton(+GetCtrl(SB_PRI_LIMIT)).SetLimits(0, 60, 3);
      return mr;
    }

   case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      CheckBox(+GetCtrl(CB_RG_ENABLE)).SetCheckState(cfg.replay_gain);
      //EnableRG(cfg.replay_gain);
      SetListContent(cfg.rg_list);
      SpinButton(+GetCtrl(SB_RG_PREAMP)).SetValue(cfg.rg_preamp);
      SpinButton(+GetCtrl(SB_RG_PREAMP_OTHER)).SetValue(cfg.rg_preamp_other);
      size_t pos;
      binary_search(&cfg.pri_normal, pos, Priorities, 10, &ComparePriority);
      ComboBox(+GetCtrl(CB_PRI_NORM)).Select(pos);
      binary_search(&cfg.pri_high, pos, Priorities+5, 10, &ComparePriority);
      ComboBox(+GetCtrl(CB_PRI_HIGH)).Select(pos);
      SpinButton(+GetCtrl(SB_PRI_LIMIT)).SetValue(cfg.pri_limit);
      return 0;
    }

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {/*case CB_RG_ENABLE:
      EnableRG(QueryButtonCheckstate(CB_RG_ENABLE));
      break;*/
     case LB_RG_LIST:
      if (SHORT2FROMMP(mp1) == LN_ENTER)
        WinSendMsg(GetHwnd(), WM_COMMAND, MPFROMSHORT(PB_RG_REMOVE), 0);
      break;
     case LB_RG_AVAILABLE:
      if (SHORT2FROMMP(mp1) == LN_ENTER)
        WinSendMsg(GetHwnd(), WM_COMMAND, MPFROMSHORT(PB_RG_ADD), 0);
      break;
    }
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_RG_UP:
      { cfg_rgtype types[4];
        GetListContent(LB_RG_LIST, types);
        unsigned selected = GetListSelection(LB_RG_LIST);
        cfg_rgtype bak;
        int state = 0;
        size_t i;
        for (i = 0; i < 4; ++i)
        { if (!(selected & (1 << i)))
          { if (state == 2)
              types[i-1] = bak;
            bak = types[i];
            state = 1;
          } else if (state)
          { types[i-1] = types[i];
            state = 2;
          }
        }
        if (state == 2)
          types[3] = bak;
        SetListContent(types);
        return 0;
      }
     case PB_RG_DOWN:
      { cfg_rgtype types[4];
        GetListContent(LB_RG_LIST, types);
        unsigned selected = GetListSelection(LB_RG_LIST);
        cfg_rgtype bak;
        int state = 0;
        size_t i;
        for (i = 4; i-- > 0; )
        { if (!(selected & (1 << i)))
          { if (state == 2)
              types[i+1] = bak;
            bak = types[i];
            state = 1;
          } else if (state)
          { types[i+1] = types[i];
            state = 2;
          }
        }
        if (state == 2)
          types[0] = bak;
        SetListContent(types);
        return 0;
      }
     case PB_RG_ADD:
      { cfg_rgtype types[4];
        GetListContent(LB_RG_LIST, types);
        cfg_rgtype newtypes[4];
        GetListContent(LB_RG_AVAILABLE, newtypes);
        unsigned selected = GetListSelection(LB_RG_AVAILABLE);
        size_t count;
        for (count = 0; count < 4; ++count)
          if (types[count] == CFG_RG_NONE)
            break;
        for (size_t i = 0; i < 4 && count < 4; ++i)
          if (selected & (1 << i))
            types[count++] = newtypes[i];
        SetListContent(types);
        return 0;
      }
     case PB_RG_REMOVE:
      { cfg_rgtype types[4];
        GetListContent(LB_RG_LIST, types);
        unsigned selected = GetListSelection(LB_RG_LIST);
        size_t target = 0;
        for (size_t i = 0; i < 4; ++i)
          if (!(selected & (1 << i)))
            types[target++] = types[i];
        while (target < 4)
          types[target++] = CFG_RG_NONE;
        SetListContent(types);
        return 0;
      }
    }
    break;

   case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);
      cfg.replay_gain = CheckBox(GetCtrl(CB_RG_ENABLE)).QueryCheckState();
      GetListContent(LB_RG_LIST, cfg.rg_list);
      cfg.rg_preamp = SpinButton(GetCtrl(SB_RG_PREAMP)).QueryValue();
      cfg.rg_preamp_other = SpinButton(GetCtrl(SB_RG_PREAMP_OTHER)).QueryValue();
      cfg.pri_normal = Priorities[ComboBox(GetCtrl(CB_PRI_NORM)).QuerySelection()].Priority;
      cfg.pri_high = Priorities[5+ComboBox(GetCtrl(CB_PRI_HIGH)).QuerySelection()].Priority;
      cfg.pri_limit = SpinButton(GetCtrl(SB_PRI_LIMIT)).QueryValue();
      return 0;
    }
  }
  return SettingsPageBase::DlgProc(msg, mp1, mp2);
}

MRESULT PropertyDialog::SystemSettingsPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  { case WM_INITDLG:
    { MRESULT mr = SettingsPageBase::DlgProc(msg, mp1, mp2);
      SpinButton(+GetCtrl(SB_TIMEOUT   )).SetLimits(1,  300, 4);
      SpinButton(+GetCtrl(SB_BUFFERSIZE)).SetLimits(0, 2048, 4);
      SpinButton(+GetCtrl(SB_FILLBUFFER)).SetLimits(1,  100, 4);
      SpinButton(+GetCtrl(SB_NUMWORKERS)).SetLimits(1,    9, 1);
      SpinButton(+GetCtrl(SB_DLGWORKERS)).SetLimits(0,    9, 1);
      return mr;
    }

    case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      char buffer[1024];
      const char* cp;
      size_t l;

      EntryField(+GetCtrl(EF_PIPE)).SetText(cfg.pipe_name);

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
      strlcpy(buffer, cfg.proxy, min(l, sizeof buffer));
      EntryField(+GetCtrl(EF_PROXY_HOST)).SetText(buffer);
      EntryField(+GetCtrl(EF_PROXY_PORT)).SetText(cp);
      cp = strchr(cfg.auth, ':');
      if (cp == NULL)
      { l = strlen(cfg.auth);
        cp = cfg.proxy + l;
        ++l;
      } else
      { ++cp;
        l = cp - cfg.auth;
      }
      strlcpy(buffer, cfg.auth, min(l, sizeof buffer));
      EntryField(+GetCtrl(EF_PROXY_USER)).SetText(buffer);
      EntryField(+GetCtrl(EF_PROXY_PASS)).SetText(cp);

      CheckBox(+GetCtrl(CB_FILLBUFFER)).SetCheckState(cfg.buff_wait);

      SpinButton(+GetCtrl(SB_TIMEOUT   )).SetValue(cfg.conn_timeout);
      SpinButton(+GetCtrl(SB_BUFFERSIZE)).SetValue(cfg.buff_size);
      SpinButton(+GetCtrl(SB_FILLBUFFER)).SetValue(cfg.buff_fill);

      SpinButton(+GetCtrl(SB_NUMWORKERS)).SetValue(cfg.num_workers);
      SpinButton(+GetCtrl(SB_DLGWORKERS)).SetValue(cfg.num_dlg_workers);
      CheckBox(+GetCtrl(CB_LOWPRIWORKERS)).SetCheckState(cfg.low_priority_workers);
      return 0;
    }

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_FILLBUFFER &&
        ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
        EnableCtrl(SB_FILLBUFFER, CheckBox(GetCtrl(CB_FILLBUFFER)).QueryCheckState());
      return 0;

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);

      cfg.pipe_name = WinQueryDlgItemXText(GetHwnd(), EF_PIPE);

      cfg.buff_size = SpinButton(GetCtrl(SB_BUFFERSIZE)).QueryValue();
      cfg.buff_fill = SpinButton(GetCtrl(SB_FILLBUFFER)).QueryValue();
      cfg.buff_wait = CheckBox(GetCtrl(CB_FILLBUFFER)).QueryCheckState();
      cfg.conn_timeout = SpinButton(GetCtrl(SB_TIMEOUT)).QueryValue();

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

      cfg.num_workers = SpinButton(GetCtrl(SB_NUMWORKERS)).QueryValue();
      cfg.num_dlg_workers = SpinButton(GetCtrl(SB_DLGWORKERS)).QueryValue();
      cfg.low_priority_workers = CheckBox(GetCtrl(CB_LOWPRIWORKERS)).QueryCheckState();

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
      SpinButton(+GetCtrl(SB_DOCK)).SetLimits(0, 30, 2);
      SpinButton(+GetCtrl(SB_RESTRICT_META)).SetLimits(10, 500, 3);
      break;

    case CFG_CHANGE:
    {
      if (mp1)
      { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
        CheckBox(+GetCtrl(CB_DOCK)).SetCheckState(cfg.dock_windows);
        { SpinButton sb(+GetCtrl(SB_DOCK));
          sb.Enable(cfg.dock_windows);
          sb.SetValue(cfg.dock_margin);
        }
        CheckBox(+GetCtrl(CB_SAVEWNDPOSBYOBJ)).SetCheckState(cfg.win_pos_by_obj);

        // load GUI
        CheckBox(+GetCtrl(RB_DISP_FILENAME+cfg.viewmode)).SetCheckState(true);
        CheckBox(+GetCtrl(RB_SCROLL_INFINITE+cfg.scroll)).SetCheckState(true);
        CheckBox(+GetCtrl(CB_SCROLL_AROUND)).SetCheckState(cfg.scroll_around);
        CheckBox(+GetCtrl(CB_RESTRICT_META)).SetCheckState(cfg.restrict_meta);
        { SpinButton sb(+GetCtrl(SB_RESTRICT_META));
          sb.Enable(cfg.restrict_meta);
          sb.SetValue(cfg.restrict_length);
        }
        CheckBox(+GetCtrl(CB_USE_SKIN_FONT)).SetCheckState(cfg.font_skinned);
        EnableCtrl(PB_FONT_SELECT, !cfg.font_skinned);
        EnableCtrl(ST_FONT_SAMPLE, !cfg.font_skinned);

        font_attrs = cfg.font_attrs;
        font_size  = cfg.font_size;
      }
      // change sample font
      xstring font_name  = amp_font_attrs_to_string( font_attrs, font_size );
      ControlBase(+GetCtrl(ST_FONT_SAMPLE)).SetText(font_name);
      WinSetPresParam(GetCtrl(ST_FONT_SAMPLE), PP_FONTNAMESIZE, font_name.length() +1, (PVOID)font_name.cdata());
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

        if (fontdialog.lReturn == DID_OK)
        { font_attrs = fontdialog.fAttrs;
          font_size  = fontdialog.fxPointSize >> 16;
          PostMsg(CFG_CHANGE, 0, 0);
        }
      }
      return 0;

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case CB_USE_SKIN_FONT:
        if ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED )
        { bool use = !!CheckBox(GetCtrl(CB_USE_SKIN_FONT)).QueryCheckState();
          EnableCtrl(PB_FONT_SELECT, !use );
          EnableCtrl(ST_FONT_SAMPLE, !use );
        }
        break;

       case CB_DOCK:
        if ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED )
          EnableCtrl(SB_DOCK, !!CheckBox(GetCtrl(CB_DOCK)).QueryCheckState());
        break;

       case CB_RESTRICT_META:
        if ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED )
          EnableCtrl(SB_RESTRICT_META, !!CheckBox(GetCtrl(CB_RESTRICT_META)).QueryCheckState());
      }
      return 0;

    case CFG_SAVE:
    { amp_cfg& cfg = *(amp_cfg*)PVOIDFROMMP(mp1);
      cfg.dock_windows  = !!CheckBox(GetCtrl(CB_DOCK)).QueryCheckState();
      cfg.dock_margin   = SpinButton(GetCtrl(SB_DOCK)).QueryValue();
      cfg.win_pos_by_obj= !!CheckBox(GetCtrl(CB_SAVEWNDPOSBYOBJ)).QueryCheckState();

      cfg.viewmode      = (cfg_disp)RadioButton(GetCtrl(RB_DISP_FILENAME)).QueryCheckIndex();
      cfg.restrict_meta = !!CheckBox(GetCtrl(CB_RESTRICT_META)).QueryCheckState();
      cfg.restrict_length=SpinButton(GetCtrl(SB_RESTRICT_META)).QueryValue();
      cfg.scroll        = (cfg_scroll)RadioButton(GetCtrl(RB_SCROLL_INFINITE)).QueryCheckIndex();
      cfg.scroll_around = !!CheckBox(GetCtrl(CB_SCROLL_AROUND)).QueryCheckState();

      cfg.font_skinned  = !!CheckBox(GetCtrl(CB_USE_SKIN_FONT)).QueryCheckState();
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
  Plugin::GetChangeEvent() += PlugmanDeleg;
}

xstring amp_cfg::* PropertyDialog::PluginPage::AccessPluginCfg() const
{ switch (List.Type)
  {case PLUGIN_DECODER:
    return &amp_cfg::decoders_list;
   case PLUGIN_FILTER:
    return &amp_cfg::filters_list;
   case PLUGIN_OUTPUT:
    return &amp_cfg::outputs_list;
   case PLUGIN_VISUAL:
    return &amp_cfg::visuals_list;
   default:
    ASSERT(false);
    return NULL;
  }
}

void PropertyDialog::PluginPage::RefreshList()
{ DEBUGLOG(("PropertyDialog::PluginPage::RefreshList()\n"));
  ListBox lb(GetCtrl(LB_PLUGINS));
  lb.DeleteAll();

  List = *Plugin::GetPluginList(List.Type);
  int_ptr<Plugin> const* ppp;
  int selected = LIT_NONE;
  for (ppp = List.begin(); ppp != List.end(); ++ppp)
  { Plugin* pp = *ppp;
    if (pp == Selected)
      selected = ppp - List.begin();
    xstring title = pp->ModRef->Key;
    if (List.Type == PLUGIN_VISUAL && ((Visual*)pp)->GetProperties().skin)
      title = title + " (Skin)";
    lb.InsertItem(title);
  }
  if (selected != LIT_NONE)
    lb.Select(selected);
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
      ControlBase(+GetCtrl(PB_PLG_ENABLE)).SetText("~Enable");
    EnableCtrl(PB_PLG_UNLOAD, false);
    EnableCtrl(PB_PLG_UP,     false);
    EnableCtrl(PB_PLG_DOWN,   false);
    EnableCtrl(PB_PLG_ENABLE, false);
  } else
  { if (List.Type == PLUGIN_OUTPUT)
      EnableCtrl(PB_PLG_ENABLE, !Selected->GetEnabled());
    else
    { ControlBase(+GetCtrl(PB_PLG_ENABLE)).SetText(Selected->GetEnabled() ? "Disabl~e" : "~Enable");
      EnableCtrl(PB_PLG_ENABLE, true);
    }
    EnableCtrl(PB_PLG_UNLOAD, true);
    EnableCtrl(PB_PLG_UP,     Selected != List[0]);
    EnableCtrl(PB_PLG_DOWN,   Selected != List[List.size()-1]);
  }
  if (Selected == NULL)
  { ControlBase(+GetCtrl(ST_PLG_AUTHOR)).SetText("");
    ControlBase(+GetCtrl(ST_PLG_DESC  )).SetText("");
    ControlBase(+GetCtrl(ST_PLG_LEVEL )).SetText("");
    EnableCtrl(PB_PLG_CONFIG, false);
  } else
  { char buffer[64];
    const PLUGIN_QUERYPARAM& params = Selected->ModRef->GetParams();
    ControlBase(+GetCtrl(ST_PLG_AUTHOR)).SetText(params.author);
    ControlBase(+GetCtrl(ST_PLG_DESC  )).SetText(params.desc);
    snprintf(buffer, sizeof buffer,        "Interface level %i%s",
      params.interface, params.interface >= PLUGIN_INTERFACE_LEVEL ? "" : " (virtualized)");
    ControlBase(+GetCtrl(ST_PLG_LEVEL )).SetText(buffer);
    EnableCtrl(PB_PLG_CONFIG, params.configurable);
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
      RequestList();
    } catch (const ModuleException& ex)
    { amp_message(Parent.GetHwnd(), MSG_ERROR, ex.GetErrorText());
    }
  }
  return rc;
}

void PropertyDialog::PluginPage::SetParams(Plugin* pp)
{ EnableCtrl(PB_PLG_SET, false);
}

void PropertyDialog::PluginPage::PlugmanNotification(const PluginEventArgs& args)
{ if (args.Type == List.Type)
  { switch (args.Operation)
    {case PluginEventArgs::Enable:
     case PluginEventArgs::Disable:
      if (args.Plug == Selected)
        RequestInfo();
      break;
     case PluginEventArgs::Load:
     case PluginEventArgs::Unload:
     case PluginEventArgs::Sequence:
      RequestList();
     default:;
    }
  }
}

/* Processes messages of the plug-ins pages of the setup notebook. */
MRESULT PropertyDialog::PluginPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PropertyDialog::PluginPage::DlgProc(%p, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  LONG i;
  switch (msg)
  {case CFG_REFRESH:
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
        { int_ptr<PluginList> def(new PluginList(List.Type));
          const xstring& err = def->LoadDefaults();
          if (err)
            EventHandler::Post(MSG_ERROR, err);
          Plugin::SetPluginList(def);
          // The GUI is updated by the PluginChange event.
        }
        break;
       case PB_UNDO:
        { xstring undocfg(Cfg::Get().*AccessPluginCfg());
          int_ptr<PluginList> list(new PluginList(List.Type));
          const xstring& err = list->Deserialize(undocfg);
          if (err)
            EventHandler::Post(MSG_ERROR, err);
          Plugin::SetPluginList(list);
          // The GUI is updated by the PluginChange event.
        }
        break;
      }
      return 0;
    }
   case CFG_SAVE:
    { ((amp_cfg*)PVOIDFROMMP(mp1))->*AccessPluginCfg() = List.Serialize();
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
          List[i]->ModRef->Config(GetHwnd());
        break;
      }
      break;

     case ML_DEC_FILETYPES:
      switch (SHORT2FROMMP(mp1))
      {case MLN_CHANGE:
        EnableCtrl(PB_PLG_SET, true);
      }
      break;

     case CB_DEC_TRYOTHER:
     case CB_DEC_SERIALIZE:
      switch (SHORT2FROMMP(mp1))
      {case BN_CLICKED:
        EnableCtrl(PB_PLG_SET, true);
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
        Plugin::SetPluginList(new PluginList(List));
      }
      break;

     case PB_PLG_ADD:
      AddPlugin();
      break;

     case PB_PLG_UP:
      if (i > 0 && (size_t)i < List.size())
      { List.move(i, i-1);
        Plugin::SetPluginList(new PluginList(List));
      }
      break;

     case PB_PLG_DOWN:
      if ((size_t)i < List.size()-1)
      { List.move(i, i+1);
        Plugin::SetPluginList(new PluginList(List));
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
        List[i]->ModRef->Config(GetHwnd());
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

void PropertyDialog::DecoderPage::RefreshInfo()
{ PluginPage::RefreshInfo();
  if (Selected == NULL)
  { MLE ft(GetCtrl(ML_DEC_FILETYPES));
    ft.SetText("");
    ft.Enable(false);
    CheckBox cb(GetCtrl(CB_DEC_TRYOTHER));
    cb.SetCheckState(false);
    cb.Enable(false);
    cb = CheckBox(GetCtrl(CB_DEC_SERIALIZE));
    cb.SetCheckState(false);
    cb.Enable(false);
    EnableCtrl(PB_PLG_SET, false);
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
      MLE ft(GetCtrl(ML_DEC_FILETYPES));
      ft.SetText(cp);
      ft.Enable(true);
    }
    stringmapentry* smp; // = sm.find("filetypes");
    smp = sm.find("tryothers");
    bool* b = smp && smp->Value ? url123::parseBoolean(smp->Value) : NULL;
    CheckBox cb(GetCtrl(CB_DEC_TRYOTHER));
    cb.SetCheckState(b && *b);
    cb.Enable(!!b);
    smp = sm.find("serializeinfo");
    b = smp && smp->Value ? url123::parseBoolean(smp->Value) : NULL;
    cb = CheckBox(GetCtrl(CB_DEC_SERIALIZE));
    cb.SetCheckState(b && *b);
    cb.Enable(!!b);
    EnableCtrl(PB_PLG_SET, false);
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
  pp->SetParam("tryothers", CheckBox(GetCtrl(CB_DEC_TRYOTHER)).QueryCheckState() ? "1" : "0");
  pp->SetParam("serializeinfo", CheckBox(GetCtrl(CB_DEC_SERIALIZE)).QueryCheckState() ? "1" : "0");
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
  ControlBase(+GetCtrl(ST_BUILT)).SetText(built);
  ControlBase(+GetCtrl(ST_AUTHORS)).SetText(SDG_AUT);
  ControlBase(+GetCtrl(ST_CREDITS)).SetText(SDG_MSG);
}


PropertyDialog::PropertyDialog(HWND owner)
: NotebookDialogBase(DLG_CONFIG, NULLHANDLE, DF_AutoResize)
{ Pages.append() = new Settings1Page(*this);
  Pages.append() = new Settings2Page(*this);
  Pages.append() = new PlaybackSettingsPage(*this);
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
  {case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {
      case PB_UNDO:
      case PB_DEFAULT:
      { HWND page = NULLHANDLE;
        LONG id = (LONG)SendCtrlMsg(NB_CONFIG, BKM_QUERYPAGEID, 0, MPFROM2SHORT(BKA_TOP,BKA_MAJOR));
        if( id && id != BOOKERR_INVALID_PARAMETERS )
          page = (HWND)SendCtrlMsg(NB_CONFIG, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(id), 0 );
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

   case WM_CLOSE:
    // Save settings
    { Cfg::ChangeAccess cfg;
      for (PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
        WinSendMsg((*pp)->GetHwnd(), CFG_SAVE, MPFROMP(&cfg), 0);
    }
    Cfg::SaveIni();
    break;
  }
  return NotebookDialogBase::DlgProc(msg, mp1, mp2);
}


PropertyDialog* APropertyDialog::Instance = NULL;

void APropertyDialog::Do(HWND owner)
{ PropertyDialog dialog(owner);
  Instance = &dialog;
  dialog.Process();
  Instance = NULL;
}

void APropertyDialog::Show()
{ if (Instance)
    Instance->SetVisible(true);
}

void APropertyDialog::Close()
{ if (Instance)
    Instance->Close();
}
