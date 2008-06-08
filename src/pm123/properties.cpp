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
#include "controller.h"
#include "copyright.h"
#include "123_util.h"
#include "pipe.h"
#include <os2.h>

#define  CFG_REFRESH_LIST (WM_USER+1)
#define  CFG_REFRESH_INFO (WM_USER+2)
#define  CFG_DEFAULT      (WM_USER+3)
#define  CFG_UNDO         (WM_USER+4)
#define  CFG_CHANGE       (WM_USER+5)

// The properties!
const amp_cfg cfg_default =
{ "",
  TRUE,
  FALSE,
  TRUE,
  TRUE,
  FALSE,
  CFG_ANAV_SONGTIME,
  TRUE, // recurse_dnd
  FALSE,
  FALSE,
  TRUE,
  2,
  
  1, // font
  FALSE,
  { sizeof(FATTRS), 0, 0, "System VIO", 0, 0, 12L, 5L, 0, 0 },
  10,
  
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
  100, // volume
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


static ULONG
cfg_add_plugin( HWND hwnd, ULONG types )
{
  FILEDLG filedialog;
  ULONG   rc = 0;
  APSZ    ftypes[] = {{ FDT_PLUGIN }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Load a plug-in";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = ftypes;
  filedialog.pszIType       = FDT_PLUGIN;


  strcpy( filedialog.szFullFile, startpath );
  WinFileDlg( HWND_DESKTOP, hwnd, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    rc = add_plugin( filedialog.szFullFile, NULL );
    if( rc & PLUGIN_VISUAL ) {
      int num = enum_visual_plugins(NULL);
      vis_init( num - 1 );
    }
    if( rc & PLUGIN_FILTER && decoder_playing()) {
      amp_info( hwnd, "This filter will only be enabled after playback of the current file." );
    }
    WinSendMsg( hwnd, CFG_REFRESH_LIST, MPFROMLONG( rc ), 0 );
  }
  return rc;
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

    case CFG_UNDO:
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      return 0;

    case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      WinCheckButton( hwnd, CB_PLAYONLOAD,    cfg.playonload  );
      WinCheckButton( hwnd, CB_TRASHONSCAN,   cfg.trash       );
      WinCheckButton( hwnd, CB_RETAINONEXIT,  cfg.retainonexit);
      WinCheckButton( hwnd, CB_RETAINONSTOP,  cfg.retainonstop);
      
      WinCheckButton( hwnd, RB_SONGONLY + cfg.altnavig, TRUE );

      WinCheckButton( hwnd, CB_AUTOUSEPL,     cfg.autouse     );
      WinCheckButton( hwnd, CB_AUTOPLAYPL,    cfg.playonuse   );
      WinCheckButton( hwnd, CB_RECURSEDND,    cfg.recurse_dnd );
      WinCheckButton( hwnd, CB_AUTOAPPENDDND, cfg.append_dnd  );
      WinCheckButton( hwnd, CB_AUTOAPPENDCMD, cfg.append_cmd  );
      WinCheckButton( hwnd, CB_QUEUEMODE,     cfg.queue_mode  );
      return 0;
    }
    case CFG_DEFAULT:
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg_default), 0);
    case WM_COMMAND:
    case WM_CONTROL:
      return 0;

    case WM_DESTROY:
      cfg.playonload  = WinQueryButtonCheckstate( hwnd, CB_PLAYONLOAD   );
      cfg.trash       = WinQueryButtonCheckstate( hwnd, CB_TRASHONSCAN  );
      cfg.retainonexit= WinQueryButtonCheckstate( hwnd, CB_RETAINONEXIT );
      cfg.retainonstop= WinQueryButtonCheckstate( hwnd, CB_RETAINONSTOP );
      
      if (WinQueryButtonCheckstate( hwnd, RB_SONGONLY ))
        cfg.altnavig = CFG_ANAV_SONG;
      else if (WinQueryButtonCheckstate( hwnd, RB_SONGTIME ))
        cfg.altnavig = CFG_ANAV_SONGTIME;
      else if (WinQueryButtonCheckstate( hwnd, RB_TIMEONLY ))
        cfg.altnavig = CFG_ANAV_TIME;

      cfg.autouse     = WinQueryButtonCheckstate( hwnd, CB_AUTOUSEPL    );
      cfg.playonuse   = WinQueryButtonCheckstate( hwnd, CB_AUTOPLAYPL   );
      cfg.recurse_dnd = WinQueryButtonCheckstate( hwnd, CB_RECURSEDND   );
      cfg.append_dnd  = WinQueryButtonCheckstate( hwnd, CB_AUTOAPPENDDND);
      cfg.append_cmd  = WinQueryButtonCheckstate( hwnd, CB_AUTOAPPENDCMD);
      cfg.queue_mode  = WinQueryButtonCheckstate( hwnd, CB_QUEUEMODE    );
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

    case CFG_UNDO:
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      return 0;

    case CFG_CHANGE:
    { const amp_cfg& cfg = *(const amp_cfg*)PVOIDFROMMP(mp1);
      char buffer[1024];
      const char* cp;
      size_t l;

      WinCheckButton( hwnd, CB_DOCK,         cfg.dock_windows );
      WinSetDlgItemText( hwnd, EF_DOCK, itoa( cfg.dock_margin, buffer, 10 ));

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

    case CFG_DEFAULT:
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg_default), 0);
    case WM_COMMAND:
      return 0;

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == CB_FILLBUFFER &&
        ( SHORT2FROMMP(mp1) == BN_CLICKED || SHORT2FROMMP(mp1) == BN_DBLCLICKED ))
      {
        BOOL fill = WinQueryButtonCheckstate( hwnd, CB_FILLBUFFER );

        WinEnableControl( hwnd, SB_FILLBUFFER, fill );
      }
      return 0;

    case WM_DESTROY:
    {
      char buffer[8];
      size_t i;

      cfg.dock_windows = WinQueryButtonCheckstate( hwnd, CB_DOCK         );
      WinQueryDlgItemText( hwnd, EF_DOCK, sizeof buffer, buffer );
      cfg.dock_margin = atoi( buffer );

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

    case CFG_UNDO:
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg), 0);
      return 0;

    case CFG_DEFAULT:
      WinPostMsg(hwnd, CFG_CHANGE, MPFROMP(&cfg_default), 0);
      return 0;

    case CFG_CHANGE:
    { if (mp1)
      { // load GUI
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

static bool cfg_config_refresh_list(HWND hwnd, SHORT id, int (*enum_fn)(PLUGIN_BASE*const**))
{ DEBUGLOG(("cfg_config_refresh_list(%x, %p, %i)\n", hwnd, enum_fn, id));
  lb_remove_all( hwnd, id );
  PLUGIN_BASE*const* list;
  int num = (*enum_fn)(&list);
  char filename[_MAX_FNAME];
  for ( int i = 0; i < num; i++ )
    lb_add_item( hwnd, id, sfname( filename, list[i]->module_name, sizeof( filename )));
  if ( lb_size( hwnd, id))
  { lb_select( hwnd, id, 0 );
    return false;
  }
  return true;
}

static PLUGIN_BASE* cfg_config_refresh_info( HWND hwnd, int (*enum_fn)(PLUGIN_BASE*const**), SHORT i,
  SHORT st_author, SHORT st_desc, SHORT pb_enable, SHORT pb_config, SHORT pb_unload )
{ PLUGIN_BASE*const* list;
  int num = (*enum_fn)(&list);
  if( i < 0 || i >= num ) {
    WinSetDlgItemText( hwnd, st_author, "Author: <none>" );
    WinSetDlgItemText( hwnd, st_desc,   "Desc: <none>" );
    WinSetDlgItemText( hwnd, pb_enable, "~Enable" );
    WinEnableControl ( hwnd, pb_config, FALSE );
    if (pb_enable)
      WinEnableControl ( hwnd, pb_enable, FALSE );
    WinEnableControl ( hwnd, pb_unload, FALSE );
    return NULL;
  } else {
    char buffer[256];
    snprintf( buffer, sizeof buffer,    "Author: %s", list[i]->query_param.author );
    WinSetDlgItemText( hwnd, st_author, buffer );
    snprintf( buffer, sizeof buffer,    "Desc: %s", list[i]->query_param.desc );
    WinSetDlgItemText( hwnd, st_desc, buffer );
    if (pb_enable)
    { WinSetDlgItemText( hwnd, pb_enable, get_plugin_enabled(list[i]) ? "Disabl~e" : "~Enable" );
      WinEnableControl ( hwnd, pb_enable, TRUE );
    }
    WinEnableControl ( hwnd, pb_unload, TRUE );
    WinEnableControl ( hwnd, pb_config, list[i]->query_param.configurable && get_plugin_enabled(list[i]) );
    return list[i];
  }
}

/* Processes messages of the plug-ins page 1 of the setup notebook. */
static MRESULT EXPENTRY
cfg_config1_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  int num;
  PLUGIN_BASE*const* list;

  switch( msg )
  {
    case CFG_REFRESH_LIST:
      if( LONGFROMMP(mp1) & PLUGIN_VISUAL )
      { if (!cfg_config_refresh_list(hwnd, LB_VISPLUG, &enum_visual_plugins))
          (unsigned&)mp1 &= ~PLUGIN_VISUAL;
      }
      if( LONGFROMMP(mp1) & PLUGIN_DECODER )
      { if (!cfg_config_refresh_list(hwnd, LB_DECPLUG, &enum_decoder_plugins))
          (unsigned&)mp1 &= ~PLUGIN_DECODER;
      }
      if (!mp1)
        return 0;
      mp2 = MPFROMSHORT( LIT_NONE ); // Uhh, dirty fallthrough!
    case CFG_REFRESH_INFO:
      if( LONGFROMMP(mp1) & PLUGIN_VISUAL )
        cfg_config_refresh_info( hwnd, &enum_visual_plugins, SHORT1FROMMP(mp2),
          ST_VIS_AUTHOR, ST_VIS_DESC, PB_VIS_ENABLE, PB_VIS_CONFIG, PB_VIS_UNLOAD );
      if( LONGFROMMP(mp1) & PLUGIN_DECODER )
        cfg_config_refresh_info( hwnd, &enum_decoder_plugins, SHORT1FROMMP(mp2),
          ST_DEC_AUTHOR, ST_DEC_DESC, PB_DEC_ENABLE, PB_DEC_CONFIG, PB_DEC_UNLOAD );
      return 0;

    case WM_INITDLG:
      do_warpsans( hwnd );
      do_warpsans( WinWindowFromID( hwnd, ST_VIS_AUTHOR ));
      do_warpsans( WinWindowFromID( hwnd, ST_VIS_DESC   ));
      do_warpsans( WinWindowFromID( hwnd, ST_DEC_AUTHOR ));
      do_warpsans( WinWindowFromID( hwnd, ST_DEC_DESC   ));

      WinPostMsg( hwnd, CFG_REFRESH_LIST,
                  MPFROMLONG( PLUGIN_VISUAL | PLUGIN_DECODER ), 0 );
      return 0;

    case WM_CONTROL:
      if( SHORT2FROMMP( mp1 ) == LN_SELECT )
      {
        int i = WinQueryLboxSelectedItem((HWND)mp2);

        if( SHORT1FROMMP( mp1 ) == LB_VISPLUG ) {
          WinPostMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_VISUAL  ), MPFROMSHORT( i ));
        } else if( SHORT1FROMMP( mp1 ) == LB_DECPLUG ) {
          WinPostMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_DECODER ), MPFROMSHORT( i ));
        }
      }
      break;

    case WM_COMMAND:
      switch( COMMANDMSG( &msg )->cmd )
      {
        case PB_VIS_ADD:
          cfg_add_plugin( hwnd, PLUGIN_VISUAL );
          return 0;

        case PB_VIS_CONFIG:
          configure_plugin( PLUGIN_VISUAL, lb_cursored( hwnd, LB_VISPLUG ), hwnd );
          return 0;

        case PB_VIS_ENABLE:
        {
          SHORT i = lb_cursored( hwnd, LB_VISPLUG );
          num = enum_visual_plugins(&list);
          if( i >= 0 && i < num ) {
            if( get_plugin_enabled(list[i]) ) {
              vis_deinit( i );
            } else {
              vis_init( i );
            }
            set_plugin_enabled(list[i], !get_plugin_enabled(list[i]));
            WinPostMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_VISUAL ), MPFROMSHORT( i ));
          }
          return 0;
        }

        case PB_VIS_UNLOAD:
        {
          SHORT i = lb_cursored( hwnd, LB_VISPLUG );
          if( i != LIT_NONE ) {
            remove_visual_plugin( i );
            if( lb_remove_item( hwnd, LB_VISPLUG, i ) == 0 ) {
              WinPostMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_VISUAL ), MPFROMSHORT( LIT_NONE ));
            } else {
              lb_select( hwnd, LB_VISPLUG, 0 );
            }
          }
          return 0;
        }

        case PB_DEC_ADD:
          cfg_add_plugin( hwnd, PLUGIN_DECODER );
          return 0;

        case PB_DEC_CONFIG:
          configure_plugin( PLUGIN_DECODER, lb_cursored( hwnd, LB_DECPLUG ), hwnd );
          return 0;

        case PB_DEC_ENABLE:
        {
          SHORT i = lb_cursored( hwnd, LB_DECPLUG );
          num = enum_decoder_plugins(&list);
          if( i >= 0 && i < num ) {
            if( !get_plugin_enabled(list[i]) ) {
              set_plugin_enabled(list[i], TRUE);
            } else {
              // This query is non-atomic, but nothing strange will happen anyway.
              if( Ctrl::IsPlaying() && dec_is_active(i) ) {
                amp_error( hwnd, "Cannot disable currently in use decoder." );
              } else {
                set_plugin_enabled(list[i], FALSE);
              }
            }
            WinPostMsg( hwnd, CFG_REFRESH_INFO,
                        MPFROMLONG( PLUGIN_DECODER ), MPFROMSHORT( i ));
          }
          return 0;
        }

        case PB_DEC_UNLOAD:
        {
          SHORT i = lb_cursored( hwnd, LB_DECPLUG );
          if( i != LIT_NONE ) {
            if( Ctrl::IsPlaying() && dec_is_active(i) ) {
              amp_error( hwnd, "Cannot unload currently used decoder." );
            } else {
              remove_decoder_plugin( i );
              if( lb_remove_item( hwnd, LB_DECPLUG, i ) == 0 ) {
                WinSendMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_DECODER ), MPFROMSHORT( LIT_NONE ));
              } else {
                lb_select( hwnd, LB_DECPLUG, 0 );
              }
            }
          }
          return 0;
        }

        default:
          return 0;
      }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the plug-ins page 2 of the setup notebook. */
static MRESULT EXPENTRY
cfg_config2_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  int num;
  PLUGIN_BASE*const* list;

  switch( msg )
  {
    case CFG_REFRESH_LIST:
      if( LONGFROMMP(mp1) & PLUGIN_OUTPUT )
      { if (!cfg_config_refresh_list(hwnd, LB_OUTPLUG, &enum_output_plugins))
          (unsigned&)mp1 &= ~PLUGIN_OUTPUT;
      }
      if( LONGFROMMP(mp1) & PLUGIN_FILTER )
      { if (!cfg_config_refresh_list(hwnd, LB_FILPLUG, &enum_filter_plugins))
          (unsigned&)mp1 &= ~PLUGIN_FILTER;
      }
      if (!mp1)
        return 0;
      mp2 = MPFROMSHORT( LIT_NONE ); // Uhh, dirty fallthrough!
    case CFG_REFRESH_INFO:
      if( LONGFROMMP(mp1) & PLUGIN_OUTPUT )
      { PLUGIN_BASE* plug = cfg_config_refresh_info( hwnd, &enum_output_plugins, SHORT1FROMMP(mp2),
          ST_OUT_AUTHOR, ST_OUT_DESC, 0, PB_OUT_CONFIG, PB_OUT_UNLOAD );
        WinEnableControl ( hwnd, PB_OUT_ACTIVATE, plug && !out_is_active(SHORT1FROMMP(mp2)) );
      }
      if( LONGFROMMP(mp1) & PLUGIN_FILTER )
        cfg_config_refresh_info( hwnd, &enum_filter_plugins, SHORT1FROMMP(mp2),
          ST_FIL_AUTHOR, ST_FIL_DESC, PB_FIL_ENABLE, PB_FIL_CONFIG, PB_FIL_UNLOAD );
      return 0;

    case WM_INITDLG:
      do_warpsans( hwnd );
      do_warpsans( WinWindowFromID( hwnd, ST_FIL_AUTHOR ));
      do_warpsans( WinWindowFromID( hwnd, ST_FIL_DESC   ));
      do_warpsans( WinWindowFromID( hwnd, ST_OUT_AUTHOR ));
      do_warpsans( WinWindowFromID( hwnd, ST_OUT_DESC   ));

      WinPostMsg( hwnd, CFG_REFRESH_LIST,
                  MPFROMLONG( PLUGIN_OUTPUT | PLUGIN_FILTER ), 0 );
      return 0;

    case WM_CONTROL:
      if( SHORT2FROMMP( mp1 ) == LN_SELECT )
      {
        int i = WinQueryLboxSelectedItem((HWND)mp2);

        if( SHORT1FROMMP( mp1 ) == LB_OUTPLUG ) {
          WinSendMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_OUTPUT ), MPFROMSHORT( i ));
        } else if( SHORT1FROMMP( mp1 ) == LB_FILPLUG ) {
          WinSendMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_FILTER ), MPFROMSHORT( i ));
        }
      }
      break;

    case WM_COMMAND:
      switch( COMMANDMSG( &msg )->cmd )
      {
        case PB_OUT_ADD:
          cfg_add_plugin( hwnd, PLUGIN_OUTPUT );
          return 0;

        case PB_OUT_CONFIG:
          configure_plugin( PLUGIN_OUTPUT, lb_cursored( hwnd, LB_OUTPLUG ), hwnd );
          return 0;

        case PB_OUT_ACTIVATE:
        {
          SHORT i = lb_cursored( hwnd, LB_OUTPLUG );
          if( i != LIT_NONE ) {
            if( decoder_playing()) {
              amp_error( hwnd, "Cannot change active output while playing." );
            } else {
              out_set_active( i );
              WinPostMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_OUTPUT ), MPFROMSHORT( i ));
            }
          }
          return 0;
        }

        case PB_OUT_UNLOAD:
        {
          SHORT i = lb_cursored( hwnd, LB_OUTPLUG );

          if( i != LIT_NONE ) {
            if( decoder_playing() && out_is_active(i) ) {
              amp_error( hwnd, "Cannot unload currently used output." );
            } else {
              remove_output_plugin( i );
              if( lb_remove_item( hwnd, LB_OUTPLUG, i ) == 0 ) {
                WinPostMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_OUTPUT ), MPFROMSHORT( LIT_NONE ));
              } else {
                lb_select( hwnd, LB_OUTPLUG, 0 );
              }
            }
          }
          return 0;
        }

        case PB_FIL_ADD:
          cfg_add_plugin( hwnd, PLUGIN_FILTER );
          return 0;

        case PB_FIL_CONFIG:
          configure_plugin( PLUGIN_FILTER, lb_cursored( hwnd, LB_FILPLUG ), hwnd );
          return 0;

        case PB_FIL_ENABLE:
        {
          SHORT i = lb_cursored( hwnd, LB_FILPLUG );
          num = enum_filter_plugins(&list);
          if( i >= 0 && i < num ) {
            if( !get_plugin_enabled(list[i]) ) {
              set_plugin_enabled(list[i], TRUE);
              if( decoder_playing()) {
                amp_info( hwnd, "This filter will only be enabled after playback of the current file." );
              }
            } else {
              set_plugin_enabled(list[i], FALSE);
              if( decoder_playing()) {
                amp_info( hwnd, "This filter will only be disabled after playback of the current file." );
              }
            }
            WinPostMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_FILTER ), MPFROMSHORT( i ));
          }
          return 0;
        }

        case PB_FIL_UNLOAD:
        {
          SHORT i = lb_cursored( hwnd, LB_FILPLUG );

          if( i != LIT_NONE ) {
            if( decoder_playing()) {
              amp_error( hwnd, "Cannot unload currently used filter." );
            } else {
              remove_filter_plugin( i );
              if( lb_remove_item( hwnd, LB_FILPLUG, i ) == 0 ) {
                WinSendMsg( hwnd, CFG_REFRESH_INFO, MPFROMLONG( PLUGIN_FILTER ), MPFROMSHORT( LIT_NONE ));
              } else {
                lb_select( hwnd, LB_FILPLUG, 0 );
              }
            }
          }
          return 0;
        }

        default:
          return 0;
      }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the setup dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_COMMAND:
      switch( COMMANDMSG( &msg )->cmd )
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
            WinSendMsg( page,
              COMMANDMSG( &msg )->cmd == PB_UNDO ? CFG_UNDO : CFG_DEFAULT, 0, 0 );
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
                            "~Settings", MPFROM2SHORT( 1, 2 ) ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_settings2_dlg_proc, NULLHANDLE, CFG_SETTINGS2, 0 ),
                            NULL, MPFROM2SHORT( 2, 2 ) ));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_display1_dlg_proc, NULLHANDLE, CFG_DISPLAY1, 0 ),
                            "~Display", 0));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config1_dlg_proc, NULLHANDLE, CFG_CONFIG1, 0 ),
                            "~Plug-ins", MPFROM2SHORT( 1, 2 )));

  PMRASSERT( nb_append_tab( book, WinLoadDlg( book, book, cfg_config2_dlg_proc, NULLHANDLE, CFG_CONFIG2, 0 ),
                            NULL, MPFROM2SHORT( 2, 2 )));


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
  PMRASSERT( nb_append_tab( book, page, "~About", 0));

  rest_window_pos( hwnd, WIN_MAP_POINTS );
  WinSetFocus( HWND_DESKTOP, book );

  WinProcessDlg   ( hwnd );
  WinDestroyWindow( hwnd );
}
