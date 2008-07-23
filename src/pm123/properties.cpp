/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 * Copyright 2004      Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 Marcel Mueller
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

#include <utilfct.h>
#include <xio.h>
#include <snprintf.h>
#include "properties.h"
#include "pm123.h"
#include "dialog.h"
#include "pm123.rc.h"
#include "iniman.h"
#include "plugman.h"
#include "plugman_base.h"
#include "controller.h"
#include "copyright.h"
#include "123_util.h"
#include "pipe.h"
#include <cpp/url123.h>
#include <cpp/stringmap.h>
#include <os2.h>

#define  CFG_REFRESH_LIST (WM_USER+1)
#define  CFG_REFRESH_INFO (WM_USER+2)
#define  CFG_GLOB_BUTTON  (WM_USER+3)
#define  CFG_CHANGE       (WM_USER+5)

// The properties!
const amp_cfg cfg_default =
{ "",
  TRUE,
  FALSE,
  TRUE,
  TRUE,
  FALSE,
  FALSE,
  CFG_ANAV_SONG,
  TRUE,
  TRUE, // recurse_dnd
  TRUE,
  FALSE,
  FALSE,
  FALSE,
  TRUE,
  2,
  
  1, // font
  FALSE,
  { sizeof(FATTRS), 0, 0, "WarpSans Bold", 0, 0, 16, 7, 0, 0 },
  9,
  
  TRUE,
  FALSE, // float on top
  CFG_SCROLL_INFINITE,
  CFG_DISP_ID3TAG,
  "", // Proxy
  "",
  FALSE,
  128,
  30,
  "\\PIPE\\PM123",
  TRUE, // dock
  10,
// state
  "",
  "",
  "",
  CFG_MODE_REGULAR,
  FALSE,
  FALSE,
  FALSE,
  TRUE, // add recursive
  TRUE, // save relative
  { 0, 0,0, 1,1, 0, 0, 0, 0 }
};

amp_cfg cfg = cfg_default;


/* Initialize properties, called from main. */
void cfg_init( void )
{ // set proxy and buffer settings statically in the xio library, not that nice, but working.
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
      WinCheckButton( hwnd, CB_TRASHONSCAN,   cfg.trash        );
      WinCheckButton( hwnd, CB_RETAINONEXIT,  cfg.retainonexit );
      WinCheckButton( hwnd, CB_RETAINONSTOP,  cfg.retainonstop );
      WinCheckButton( hwnd, CB_RESTARTONSTART,cfg.restartonstart);
      
      WinCheckButton( hwnd, CB_AUTOUSEPL,     cfg.autouse      );
      WinCheckButton( hwnd, CB_AUTOPLAYPL,    cfg.playonuse    );
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
        WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(data), 0);
      }
      return 0;
    }

    case WM_DESTROY:
      cfg.playonload  = WinQueryButtonCheckstate( hwnd, CB_PLAYONLOAD   );
      cfg.trash       = WinQueryButtonCheckstate( hwnd, CB_TRASHONSCAN  );
      cfg.retainonexit= WinQueryButtonCheckstate( hwnd, CB_RETAINONEXIT );
      cfg.retainonstop= WinQueryButtonCheckstate( hwnd, CB_RETAINONSTOP );
      cfg.restartonstart= WinQueryButtonCheckstate( hwnd, CB_RESTARTONSTART);
      
      cfg.autouse     = WinQueryButtonCheckstate( hwnd, CB_AUTOUSEPL    );
      cfg.playonuse   = WinQueryButtonCheckstate( hwnd, CB_AUTOPLAYPL   );
      cfg.recurse_dnd = WinQueryButtonCheckstate( hwnd, CB_RECURSEDND   );
      cfg.sort_folders= WinQueryButtonCheckstate( hwnd, CB_SORTFOLDERS  );
      cfg.folders_first= WinQueryButtonCheckstate( hwnd, CB_FOLDERSFIRST);
      cfg.append_dnd  = WinQueryButtonCheckstate( hwnd, CB_AUTOAPPENDDND);
      cfg.append_cmd  = WinQueryButtonCheckstate( hwnd, CB_AUTOAPPENDCMD);
      cfg.queue_mode  = WinQueryButtonCheckstate( hwnd, CB_QUEUEMODE    );
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
      char buffer[1024];
      const char* cp;
      size_t l;

      WinCheckButton( hwnd, CB_TURNAROUND,cfg.autoturnaround );
      WinCheckButton( hwnd, RB_SONGONLY + cfg.altnavig, TRUE );

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

      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_SETLIMITS, MPFROMLONG( 2048 ), MPFROMLONG( 0 ));
      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.buff_size ), 0 );
      WinSendDlgItemMsg( hwnd, SB_FILLBUFFER, SPBM_SETLIMITS, MPFROMLONG( 100  ), MPFROMLONG( 1 ));
      WinSendDlgItemMsg( hwnd, SB_FILLBUFFER, SPBM_SETCURRENTVALUE, MPFROMLONG( cfg.buff_fill ), 0 );
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

      cfg.autoturnaround = WinQueryButtonCheckstate( hwnd, CB_TURNAROUND );
      if (WinQueryButtonCheckstate( hwnd, RB_SONGONLY ))
        cfg.altnavig = CFG_ANAV_SONG;
      else if (WinQueryButtonCheckstate( hwnd, RB_SONGTIME ))
        cfg.altnavig = CFG_ANAV_SONGTIME;
      else if (WinQueryButtonCheckstate( hwnd, RB_TIMEONLY ))
        cfg.altnavig = CFG_ANAV_TIME;

      WinQueryDlgItemText( hwnd, EF_PIPE, sizeof cfg.pipe_name, cfg.pipe_name );
      // restatrt pipe worker
      amp_pipe_destroy();
      amp_pipe_create();

      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_QUERYVALUE, MPFROMP( &i ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));
      cfg.buff_size = i;

      WinSendDlgItemMsg( hwnd, SB_FILLBUFFER, SPBM_QUERYVALUE, MPFROMP( &i ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));
      cfg.buff_fill = i;

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
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      break;

    case CFG_CHANGE:
    { 
      if (mp1)
      { char buffer[20];
        WinCheckButton( hwnd, CB_DOCK,         cfg.dock_windows );
        WinSetDlgItemText( hwnd, EF_DOCK, itoa( cfg.dock_margin, buffer, 10 ));

        // load GUI
        const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
        WinCheckButton( hwnd, RB_DISP_FILENAME   + cfg.viewmode, TRUE );
        WinCheckButton( hwnd, RB_SCROLL_INFINITE + cfg.scroll,   TRUE );
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
      if( SHORT1FROMMP(mp1) == CB_USE_SKIN_FONT &&
        ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
      {
        BOOL use = WinQueryButtonCheckstate( hwnd, CB_USE_SKIN_FONT );

        WinEnableControl( hwnd, PB_FONT_SELECT, !use );
        WinEnableControl( hwnd, ST_FONT_SAMPLE, !use );
      }
      return 0;

    case WM_DESTROY:
    {
      char buffer[8];
      cfg.dock_windows = WinQueryButtonCheckstate( hwnd, CB_DOCK         );
      WinQueryDlgItemText( hwnd, EF_DOCK, sizeof buffer, buffer );
      cfg.dock_margin = atoi( buffer );

      cfg.scroll   = LONGFROMMR( WinSendDlgItemMsg( hwnd, RB_SCROLL_INFINITE, BM_QUERYCHECKINDEX, 0, 0 ));
      cfg.viewmode = LONGFROMMR( WinSendDlgItemMsg( hwnd, RB_DISP_FILENAME,   BM_QUERYCHECKINDEX, 0, 0 ));

      cfg.font_skinned  = WinQueryButtonCheckstate( hwnd, CB_USE_SKIN_FONT );
      cfg.font_size     = font_size;
      cfg.font_attrs    = font_attrs;

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
    // decode specific stuff
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
      WinEnableControl (Hwnd, PB_PLG_ACTIVATE, !((PluginList1*)List)->GetActive() == i);
    // decode specific stuff
    if (List->Type == PLUGIN_DECODER)
    { stringmap_own sm(20);
      pp->GetParams(sm);
      // TODO: Decoder supported file types vs. user file types.
      stringmapentry* smp; // = sm.find("filetypes");
      const xstring& filetypes = ((Decoder*)pp)->GetFileTypes();
      char* cp;
      if (filetypes)
      { cp = (char*)alloca(filetypes.length()+1);
        memcpy(cp, filetypes.cdata(), filetypes.length()+1);
        for (char* cp2 = cp; *cp2; ++cp2)
          if (*cp2 == ';')
            *cp2 = '\n';
      } else
        cp = "";
      HWND ctrl = WinWindowFromID(Hwnd, ML_DEC_FILETYPES);
      WinSetWindowText(ctrl, cp);
      WinEnableWindow (ctrl, TRUE);
      smp = sm.find("tryothers");
      bool* b = smp && smp->Value ? url123::parseBoolean(smp->Value) : NULL;
      ctrl = WinWindowFromID(Hwnd, CB_DEC_TRYOTHER);
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
  APSZ    ftypes[] = {{ FDT_PLUGIN }, { 0 }};

  memset(&filedialog, 0, sizeof(FILEDLG));

  filedialog.cbSize         = sizeof(FILEDLG);
  filedialog.fl             = FDS_CENTER|FDS_OPEN_DIALOG|FDS_CUSTOM;
  filedialog.pszTitle       = "Load a plug-in";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = ftypes;
  filedialog.pszIType       = FDT_PLUGIN;

  strcpy(filedialog.szFullFile, startpath);
  WinFileDlg(HWND_DESKTOP, Hwnd, &filedialog);

  if (filedialog.lReturn == DID_OK)
  { rc = Plugin::Deserialize(filedialog.szFullFile, List->Type);
    if (rc & PLUGIN_VISUAL)
      vis_init(List->size()-1);
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
    for (char* cp2 = filetypes; *cp2; ++cp2)
      if (*cp2 == '\n')
        *cp2 = ';';
    pp->SetParam("filetypes", filetypes);
    pp->SetParam("tryothers", WinQueryButtonCheckstate(Hwnd, CB_DEC_TRYOTHER) ? "1" : "0");
    pp->SetParam("serializeinfo", WinQueryButtonCheckstate(Hwnd, CB_DEC_SERIALIZE) ? "1" : "0");
  }
  WinEnableControl(Hwnd, PB_PLG_SET, FALSE);
  return true;
}

bool PluginContext::visual_enable_hook(size_t i, bool enable)
{ if (enable)
    vis_init(i);
  else
    vis_deinit(i);
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
{ DEBUGLOG(("cfg_config_dlg_proc(%p, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
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
          WinPostMsg(hwnd, CFG_REFRESH_INFO, 0, MPFROMSHORT(i));
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
          amp_show_help( IDH_PROPERTIES );
          return 0;
      }
      return 0;

    case WM_DESTROY:
      save_ini();
      save_window_pos( hwnd, WIN_MAP_POINTS );
      return 0;

    case WM_WINDOWPOSCHANGED:
      if(((SWP*)mp1)->fl & SWP_SIZE ) {
        nb_adjust( hwnd );
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
                            "~Settings", NULL, MPFROM2SHORT( 1, 2 ) ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_settings2_dlg_proc, NULLHANDLE, CFG_SETTINGS2, 0 ),
                            NULL, NULL, MPFROM2SHORT( 2, 2 ) ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_display1_dlg_proc, NULLHANDLE, CFG_DISPLAY1, 0 ),
                            "~Display", NULL, 0));

  PluginContext decoder_context(&Decoders, NULL, DECODER_PLUGIN_LEVEL, PluginContext::CF_None);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_DEC_CONFIG, &decoder_context ),
                            "~Plug-ins", "Decoder Plug-ins", MPFROM2SHORT( 1, 4 )));

  PluginContext filter_context(&Filters, NULL, FILTER_PLUGIN_LEVEL, PluginContext::CF_None);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_FIL_CONFIG, &filter_context ),
                            NULL, "Filter Plug-ins", MPFROM2SHORT( 2, 4 )));

  PluginContext output_context(&Outputs, NULL, OUTPUT_PLUGIN_LEVEL, PluginContext::CF_List1);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_OUT_CONFIG, &output_context ),
                            NULL, "Output Plug-ins", MPFROM2SHORT( 3, 4 )));

  PluginContext visual_context(&Visuals, &VisualsSkinned, VISUAL_PLUGIN_LEVEL, PluginContext::CF_None);
  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config_dlg_proc, NULLHANDLE, CFG_VIS_CONFIG, &visual_context ),
                            NULL, "Visual Plug-ins", MPFROM2SHORT( 4, 4 )));

  page = WinLoadDlg( book, book, &WinDefDlgProc, NULLHANDLE, CFG_ABOUT, 0 );
  do_warpsans( page );
  do_warpsans( WinWindowFromID( page, ST_TITLE2 ));
  do_warpsans( WinWindowFromID( page, ST_BUILT  ));
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

  rest_window_pos( hwnd, WIN_MAP_POINTS );
  WinSetFocus( HWND_DESKTOP, book );

  WinProcessDlg   ( hwnd );
  WinDestroyWindow( hwnd );
}
