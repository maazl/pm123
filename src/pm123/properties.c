/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
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

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_ERRORS
#include <os2.h>
#include <stdio.h>
#include <memory.h>

#include "properties.h"
#include "pm123.h"
#include "plugman.h"
#include "httpget.h"

#define  CFG_REFRESH_LIST (WM_USER+1)
#define  CFG_REFRESH_INFO (WM_USER+2)
#define  CFG_DEFAULT      (WM_USER+3)
#define  CFG_UNDO         (WM_USER+4)

amp_cfg cfg;

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
      vis_init( amp_player_window(), num_visuals - 1 );
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
cfg_page1_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_INITDLG:
    {
      char buffer[128];

      WinCheckButton( hwnd, CB_PLAYONLOAD,   cfg.playonload   );
      WinCheckButton( hwnd, CB_AUTOUSEPL,    cfg.autouse      );
      WinCheckButton( hwnd, CB_AUTOPLAYPL,   cfg.playonuse    );
      WinCheckButton( hwnd, CB_SELECTPLAYED, cfg.selectplayed );
      WinCheckButton( hwnd, CB_TRASHONSEEK,  cfg.trash        );
      WinCheckButton( hwnd, CB_DOCK,         cfg.dock_windows );

      WinSetDlgItemText( hwnd, EF_DOCK, itoa( cfg.dock_margin, buffer, 10 ));
      WinSetDlgItemText( hwnd, EF_PROXY_URL,  cfg.proxy   );
      WinSetDlgItemText( hwnd, EF_HTTP_AUTH,  cfg.auth    );
      WinCheckButton   ( hwnd, CB_FILLBUFFER, cfg.bufwait );

      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_SETLIMITS,
                               MPFROMLONG( 2048 ), MPFROMLONG( 0 ));
      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_SETCURRENTVALUE,
                               MPFROMLONG( cfg.bufsize ), 0 );
      return 0;
    }

    case WM_COMMAND:
    case WM_CONTROL:
      return 0;

    case CFG_UNDO:
      WinSendMsg( hwnd, WM_INITDLG, 0, 0 );
      return 0;

    case CFG_DEFAULT:
    {
      WinCheckButton( hwnd, CB_PLAYONLOAD,      TRUE  );
      WinCheckButton( hwnd, CB_AUTOUSEPL,       TRUE  );
      WinCheckButton( hwnd, CB_AUTOPLAYPL,      TRUE  );
      WinCheckButton( hwnd, CB_SELECTPLAYED,    FALSE );
      WinCheckButton( hwnd, CB_TRASHONSEEK,     TRUE  );
      WinCheckButton( hwnd, CB_DOCK,            TRUE  );
      WinCheckButton( hwnd, CB_FILLBUFFER,      TRUE  );

      WinSetDlgItemText( hwnd, EF_DOCK,      "10" );
      WinSetDlgItemText( hwnd, EF_PROXY_URL, ""   );
      WinSetDlgItemText( hwnd, EF_HTTP_AUTH, ""   );
      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_SETCURRENTVALUE, 0, 0 );
      return 0;
    }

    case WM_DESTROY:
    {
      char buffer[8];
      int  i;

      cfg.playonload   = WinQueryButtonCheckstate( hwnd, CB_PLAYONLOAD   );
      cfg.autouse      = WinQueryButtonCheckstate( hwnd, CB_AUTOUSEPL    );
      cfg.playonuse    = WinQueryButtonCheckstate( hwnd, CB_AUTOPLAYPL   );
      cfg.selectplayed = WinQueryButtonCheckstate( hwnd, CB_SELECTPLAYED );
      cfg.trash        = WinQueryButtonCheckstate( hwnd, CB_TRASHONSEEK  );
      cfg.dock_windows = WinQueryButtonCheckstate( hwnd, CB_DOCK         );

      WinQueryDlgItemText( hwnd, EF_DOCK, 8, buffer );
      cfg.dock_margin = atoi(buffer);

      WinSendDlgItemMsg( hwnd, SB_BUFFERSIZE, SPBM_QUERYVALUE,
                         MPFROMP( &i ), MPFROM2SHORT( 0, SPBQ_DONOTUPDATE ));
      cfg.bufsize = i;

      WinQueryDlgItemText( hwnd, EF_PROXY_URL, sizeof( cfg.proxy ), cfg.proxy );
      WinQueryDlgItemText( hwnd, EF_HTTP_AUTH, sizeof( cfg.auth  ), cfg.auth  );

      set_proxyurl( cfg.proxy );
      set_httpauth( cfg.auth  );

      cfg.bufwait = WinQueryButtonCheckstate( hwnd, CB_FILLBUFFER );
      return 0;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

static char*
cfg_attrs_to_font( const FATTRS* attrs, char* font, LONG size )
{
  sprintf( font, "%ld.%s", size, attrs->szFacename );

  if( attrs->fsSelection & FATTR_SEL_ITALIC     ) {
    strcat( font, ".Italic" );
  }
  if( attrs->fsSelection & FATTR_SEL_OUTLINE    ) {
    strcat( font, ".Outline" );
  }
  if( attrs->fsSelection & FATTR_SEL_STRIKEOUT  ) {
    strcat( font, ".Strikeout" );
  }
  if( attrs->fsSelection & FATTR_SEL_UNDERSCORE ) {
    strcat( font, ".Underscore" );
  }
  if( attrs->fsSelection & FATTR_SEL_BOLD       ) {
    strcat( font, ".Bold" );
  }

  return font;
}

/* Processes messages of the display page of the setup notebook. */
static MRESULT EXPENTRY
cfg_page2_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  static FATTRS font_attrs;
  static LONG   font_size;
  static char   font_name[FACESIZE+64];

  switch( msg ) {
    case WM_INITDLG:
    {
      int i;

      WinCheckButton( hwnd, RB_DISP_FILENAME   + cfg.viewmode, TRUE );
      WinCheckButton( hwnd, RB_SCROLL_INFINITE + cfg.scroll,   TRUE );
      WinCheckButton( hwnd, CB_USE_SKIN_FONT,    cfg.font_skinned   );

      lb_remove_all( hwnd, CB_CHARSET );
      for( i = 0; i < ch_list_size; i++ ) {
        lb_add_item( hwnd, CB_CHARSET, ch_list[i].name );
        lb_set_handle( hwnd, CB_CHARSET, i, (PVOID)ch_list[i].id );

        if( i == cfg.charset ) {
          lb_select( hwnd, CB_CHARSET, i );
        }
      }

      WinEnableControl( hwnd, PB_FONT_SELECT, !cfg.font_skinned );
      WinEnableControl( hwnd, ST_FONT_SAMPLE, !cfg.font_skinned );

      font_attrs = cfg.font_attrs;
      font_size  = cfg.font_size;

      WinSetDlgItemText( hwnd, ST_FONT_SAMPLE,
                         cfg_attrs_to_font( &font_attrs, font_name, font_size ));
      WinSetPresParam  ( WinWindowFromID( hwnd, ST_FONT_SAMPLE ),
                         PP_FONTNAMESIZE, strlen( font_name ) + 1,  font_name );
      return 0;
    }

    case WM_COMMAND:
      if( COMMANDMSG( &msg )->cmd == PB_FONT_SELECT )
      {
        FONTDLG fontdialog;
        char    fontname[ FACESIZE + 64 ];

        WinQueryDlgItemText( hwnd, ST_FONT_SAMPLE, sizeof( fontname ), fontname );

        memset( &fontdialog, 0, sizeof( FONTDLG ));

        fontdialog.cbSize         = sizeof( fontdialog );
        fontdialog.hpsScreen      = WinGetScreenPS( HWND_DESKTOP );
        fontdialog.pszFamilyname  = malloc( FACESIZE );
        fontdialog.usFamilyBufLen = FACESIZE;
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

          WinSetDlgItemText( hwnd, ST_FONT_SAMPLE,
                             cfg_attrs_to_font( &font_attrs, font_name, font_size ));
          WinSetPresParam  ( WinWindowFromID( hwnd, ST_FONT_SAMPLE ),
                             PP_FONTNAMESIZE, strlen( font_name ) + 1,  font_name );
        }

        free( fontdialog.pszFamilyname );
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

    case CFG_UNDO:
      WinSendMsg( hwnd, WM_INITDLG, 0, 0 );
      return 0;

    case CFG_DEFAULT:
    {
      WinCheckButton( hwnd, RB_SCROLL_INFINITE, TRUE );
      WinCheckButton( hwnd, RB_DISP_FILENAME,   TRUE );
      WinCheckButton( hwnd, CB_USE_SKIN_FONT,   TRUE );

      font_size = 9;

      memset( &font_attrs, 0, sizeof( font_attrs ));
      strcpy( font_attrs.szFacename, "WarpSans Bold" );

      font_attrs.usRecordLength  = sizeof(FATTRS);
      font_attrs.lMaxBaselineExt = 14L;
      font_attrs.lAveCharWidth   =  6L;

      WinSetDlgItemText( hwnd, ST_FONT_SAMPLE,
                         cfg_attrs_to_font( &font_attrs, font_name, font_size ));
      WinSetPresParam  ( WinWindowFromID( hwnd, ST_FONT_SAMPLE ),
                         PP_FONTNAMESIZE, strlen( font_name ) + 1,  font_name );

      WinEnableControl( hwnd, PB_FONT_SELECT, FALSE );
      WinEnableControl( hwnd, ST_FONT_SAMPLE, FALSE );

      lb_select( hwnd, CB_CHARSET, 0 );
      return 0;
    }

    case WM_DESTROY:
    {
      int i;

      cfg.scroll   = LONGFROMMR( WinSendDlgItemMsg( hwnd, RB_SCROLL_INFINITE, BM_QUERYCHECKINDEX, 0, 0 ));
      cfg.viewmode = LONGFROMMR( WinSendDlgItemMsg( hwnd, RB_DISP_FILENAME,   BM_QUERYCHECKINDEX, 0, 0 ));

      i = lb_cursored( hwnd, CB_CHARSET );
      if( i != LIT_NONE ) {
        cfg.charset = (int)lb_get_handle( hwnd, CB_CHARSET, i );
      }

      cfg.font_skinned = WinQueryButtonCheckstate( hwnd, CB_USE_SKIN_FONT );
      cfg.font_size    = font_size;
      cfg.font_attrs   = font_attrs;

      amp_display_filename();
      amp_invalidate( UPD_FILEINFO );
      return 0;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the plug-ins page 1 of the setup notebook. */
static MRESULT EXPENTRY
cfg_page3_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case CFG_REFRESH_LIST:
    {
      int  i;
      char filename[_MAX_FNAME];

      if( LONGFROMMP(mp1) & PLUGIN_VISUAL  )
      {
        lb_remove_all( hwnd, LB_VISPLUG );
        for( i = 0; i < num_visuals; i++ ) {
          lb_add_item( hwnd, LB_VISPLUG, sfname( filename, visuals[i].module_name, sizeof( filename )));
        }
        if( lb_size( hwnd, LB_VISPLUG )) {
          lb_select( hwnd, LB_VISPLUG, 0 );
        } else {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_VISUAL ), MPFROMSHORT( LIT_NONE ));
        }
      }

      if( LONGFROMMP(mp1) & PLUGIN_DECODER )
      {
        lb_remove_all( hwnd, LB_DECPLUG );
        for( i = 0; i < num_decoders; i++ ) {
          lb_add_item( hwnd, LB_DECPLUG, sfname( filename, decoders[i].module_name, sizeof( filename )));
        }
        if( lb_size( hwnd, LB_DECPLUG )) {
          lb_select( hwnd, LB_DECPLUG, 0 );
        } else {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_DECODER ), MPFROMSHORT( LIT_NONE ));
        }
      }
      return 0;
    }

    case CFG_REFRESH_INFO:
    {
      SHORT i = SHORT1FROMMP(mp2);
      char  buffer[256];

      if( LONGFROMMP(mp1) & PLUGIN_VISUAL  )
      {
        if( i == LIT_NONE ) {
          WinSetDlgItemText( hwnd, ST_VIS_AUTHOR, "Author: <none>" );
          WinSetDlgItemText( hwnd, ST_VIS_DESC, "Desc: <none>" );
          WinSetDlgItemText( hwnd, PB_VIS_ENABLE, "~Enable" );
          WinEnableControl ( hwnd, PB_VIS_CONFIG, FALSE );
          WinEnableControl ( hwnd, PB_VIS_ENABLE, FALSE );
          WinEnableControl ( hwnd, PB_VIS_UNLOAD, FALSE );
        } else {
          sprintf( buffer, "Author: %s", visuals[i].query_param.author );
          WinSetDlgItemText( hwnd, ST_VIS_AUTHOR, buffer );
          sprintf( buffer, "Desc: %s", visuals[i].query_param.desc );
          WinSetDlgItemText( hwnd, ST_VIS_DESC, buffer );
          WinSetDlgItemText( hwnd, PB_VIS_ENABLE, visuals[i].enabled ? "Disabl~e" : "~Enable" );
          WinEnableControl ( hwnd, PB_VIS_ENABLE, TRUE );
          WinEnableControl ( hwnd, PB_VIS_UNLOAD, TRUE );
          WinEnableControl ( hwnd, PB_VIS_CONFIG, visuals[i].query_param.configurable &&
                                                  visuals[i].enabled );
        }
      }
      if( LONGFROMMP(mp1) & PLUGIN_DECODER ) {
        if( i == LIT_NONE ) {
          WinSetDlgItemText( hwnd, ST_DEC_AUTHOR, "Author: <none>" );
          WinSetDlgItemText( hwnd, ST_DEC_DESC, "Desc: <none>" );
          WinSetDlgItemText( hwnd, PB_DEC_ENABLE, "~Enable" );
          WinEnableControl ( hwnd, PB_DEC_CONFIG, FALSE );
          WinEnableControl ( hwnd, PB_DEC_ENABLE, FALSE );
          WinEnableControl ( hwnd, PB_DEC_UNLOAD, FALSE );
        } else {
          sprintf( buffer, "Author: %s", decoders[i].query_param.author );
          WinSetDlgItemText( hwnd, ST_DEC_AUTHOR, buffer );
          sprintf( buffer, "Desc: %s", decoders[i].query_param.desc );
          WinSetDlgItemText( hwnd, ST_DEC_DESC, buffer );
          WinSetDlgItemText( hwnd, PB_DEC_ENABLE, decoders[i].enabled ? "Disabl~e" : "~Enable" );
          WinEnableControl ( hwnd, PB_DEC_ENABLE, TRUE );
          WinEnableControl ( hwnd, PB_DEC_UNLOAD, TRUE );
          WinEnableControl ( hwnd, PB_DEC_CONFIG, decoders[i].query_param.configurable &&
                                                  decoders[i].enabled );
        }
      }
      return 0;
    }

    case WM_INITDLG:
      WinSendMsg( hwnd, CFG_REFRESH_LIST,
                  MPFROMLONG( PLUGIN_VISUAL | PLUGIN_DECODER ), 0 );
      return 0;

    case WM_CONTROL:
      if( SHORT2FROMMP( mp1 ) == LN_SELECT )
      {
        int i = WinQueryLboxSelectedItem((HWND)mp2);

        if( SHORT1FROMMP( mp1 ) == LB_VISPLUG ) {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_VISUAL  ), MPFROMSHORT( i ));
        } else if( SHORT1FROMMP( mp1 ) == LB_DECPLUG ) {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_DECODER ), MPFROMSHORT( i ));
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
        {
          SHORT i = lb_cursored( hwnd, LB_VISPLUG );
          if( i != LIT_NONE ) {
            visuals[i].plugin_configure( hwnd, visuals[i].module );
          }
          return 0;
        }

        case PB_VIS_ENABLE:
        {
          SHORT i = lb_cursored( hwnd, LB_VISPLUG );
          if( i != LIT_NONE ) {
            if( visuals[i].enabled ) {
              vis_deinit( i );
            } else {
              vis_init( amp_player_window(), i );
            }
            visuals[i].enabled = !visuals[i].enabled;
            WinSendMsg( hwnd, CFG_REFRESH_INFO,
                        MPFROMLONG( PLUGIN_VISUAL ), MPFROMSHORT( i ));
          }
          return 0;
        }

        case PB_VIS_UNLOAD:
        {
          SHORT i = lb_cursored( hwnd, LB_VISPLUG );
          if( i != LIT_NONE ) {
            remove_visual_plugin( &visuals[i] );
            if( lb_remove_item( hwnd, LB_VISPLUG, i ) == 0 ) {
              WinSendMsg( hwnd, CFG_REFRESH_INFO,
                          MPFROMLONG( PLUGIN_VISUAL ), MPFROMSHORT( LIT_NONE ));
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
        {
          SHORT i = lb_cursored( hwnd, LB_DECPLUG );
          if( i != LIT_NONE ) {
            decoders[i].plugin_configure( hwnd, decoders[i].module );
          }
          return 0;
        }

        case PB_DEC_ENABLE:
        {
          SHORT i = lb_cursored( hwnd, LB_DECPLUG );
          if( i != LIT_NONE ) {
            if( !decoders[i].enabled ) {
              decoders[i].enabled = TRUE;
            } else {
              if( active_decoder == i ) {
                if( decoder_playing()) {
                  amp_error( hwnd, "Cannot disable currently in use decoder." );
                } else {
                  decoders[i].enabled = FALSE;
                  dec_set_name_active( NULL );
                }
              } else {
                decoders[i].enabled = FALSE;
              }
            }
            WinSendMsg( hwnd, CFG_REFRESH_INFO,
                        MPFROMLONG( PLUGIN_DECODER ), MPFROMSHORT( i ));
          }
          return 0;
        }

        case PB_DEC_UNLOAD:
        {
          SHORT i = lb_cursored( hwnd, LB_DECPLUG );
          if( i != LIT_NONE ) {
            if( decoder_playing() && active_decoder == i ) {
              amp_error( hwnd, "Cannot unload currently used decoder." );
            } else {
              remove_decoder_plugin( &decoders[i] );
              if( lb_remove_item( hwnd, LB_DECPLUG, i ) == 0 ) {
                WinSendMsg( hwnd, CFG_REFRESH_INFO,
                            MPFROMLONG( PLUGIN_DECODER ), MPFROMSHORT( LIT_NONE ));
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
cfg_page4_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case CFG_REFRESH_LIST:
    {
      int  i;
      char filename[_MAX_FNAME];

      if( LONGFROMMP(mp1) & PLUGIN_OUTPUT  )
      {
        lb_remove_all( hwnd, LB_OUTPLUG );
        for( i = 0; i < num_outputs; i++ ) {
          lb_add_item( hwnd, LB_OUTPLUG, sfname( filename, outputs[i].module_name, sizeof( filename )));
        }
        if( lb_size( hwnd, LB_OUTPLUG )) {
          lb_select( hwnd, LB_OUTPLUG, 0 );
        } else {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_OUTPUT ), MPFROMSHORT( LIT_NONE ));
        }
      }

      if( LONGFROMMP(mp1) & PLUGIN_FILTER )
      {
        lb_remove_all( hwnd, LB_FILPLUG );
        for( i = 0; i < num_filters; i++ ) {
          lb_add_item( hwnd, LB_FILPLUG, sfname( filename, filters[i].module_name, sizeof( filename )));
        }
        if( lb_size( hwnd, LB_FILPLUG )) {
          lb_select( hwnd, LB_FILPLUG, 0 );
        } else {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_FILTER ), MPFROMSHORT( LIT_NONE ));
        }
      }
      return 0;
    }

    case CFG_REFRESH_INFO:
    {
      SHORT i = SHORT1FROMMP(mp2);
      char  buffer[256];

      if( LONGFROMMP(mp1) & PLUGIN_OUTPUT )
      {
        if( i == LIT_NONE ) {
          WinSetDlgItemText( hwnd, ST_OUT_AUTHOR, "Author: <none>" );
          WinSetDlgItemText( hwnd, ST_OUT_DESC, "Desc: <none>" );
          WinEnableControl ( hwnd, PB_OUT_CONFIG, FALSE );
          WinEnableControl ( hwnd, PB_OUT_ACTIVATE, FALSE );
          WinEnableControl ( hwnd, PB_OUT_UNLOAD, FALSE );
        } else {
          sprintf( buffer, "Author: %s", outputs[i].query_param.author );
          WinSetDlgItemText( hwnd, ST_OUT_AUTHOR, buffer );
          sprintf( buffer, "Desc: %s", outputs[i].query_param.desc );
          WinSetDlgItemText( hwnd, ST_OUT_DESC, buffer );
          WinEnableControl ( hwnd, PB_OUT_UNLOAD, TRUE );
          WinEnableControl ( hwnd, PB_OUT_ACTIVATE, active_output != i );
          WinEnableControl ( hwnd, PB_OUT_CONFIG, outputs[i].query_param.configurable );
        }
      }
      if( LONGFROMMP(mp1) & PLUGIN_FILTER ) {
        if( i == LIT_NONE ) {
          WinSetDlgItemText( hwnd, ST_FIL_AUTHOR, "Author: <none>" );
          WinSetDlgItemText( hwnd, ST_FIL_DESC, "Desc: <none>" );
          WinSetDlgItemText( hwnd, PB_FIL_ENABLE, "~Enable" );
          WinEnableControl ( hwnd, PB_FIL_CONFIG, FALSE );
          WinEnableControl ( hwnd, PB_FIL_ENABLE, FALSE );
          WinEnableControl ( hwnd, PB_FIL_UNLOAD, FALSE );
        } else {
          sprintf( buffer, "Author: %s", filters[i].query_param.author );
          WinSetDlgItemText( hwnd, ST_FIL_AUTHOR, buffer );
          sprintf( buffer, "Desc: %s", filters[i].query_param.desc );
          WinSetDlgItemText( hwnd, ST_FIL_DESC, buffer );
          WinSetDlgItemText( hwnd, PB_FIL_ENABLE, filters[i].enabled ? "Disabl~e" : "~Enable" );
          WinEnableControl ( hwnd, PB_FIL_ENABLE, TRUE );
          WinEnableControl ( hwnd, PB_FIL_UNLOAD, TRUE );
          WinEnableControl ( hwnd, PB_FIL_CONFIG, filters[i].query_param.configurable &&
                                                  filters[i].enabled );
        }
      }
      return 0;
    }

    case WM_INITDLG:
      WinSendMsg( hwnd, CFG_REFRESH_LIST,
                  MPFROMLONG( PLUGIN_OUTPUT | PLUGIN_FILTER ), 0 );
      return 0;

    case WM_CONTROL:
      if( SHORT2FROMMP( mp1 ) == LN_SELECT )
      {
        int i = WinQueryLboxSelectedItem((HWND)mp2);

        if( SHORT1FROMMP( mp1 ) == LB_OUTPLUG ) {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_OUTPUT ), MPFROMSHORT( i ));
        } else if( SHORT1FROMMP( mp1 ) == LB_FILPLUG ) {
          WinSendMsg( hwnd, CFG_REFRESH_INFO,
                      MPFROMLONG( PLUGIN_FILTER ), MPFROMSHORT( i ));
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
        {
          SHORT i = lb_cursored( hwnd, LB_OUTPLUG );
          if( i != LIT_NONE ) {
            outputs[i].plugin_configure( hwnd, outputs[i].module );
          }
          return 0;
        }

        case PB_OUT_ACTIVATE:
        {
          SHORT i = lb_cursored( hwnd, LB_OUTPLUG );

          if( i != LIT_NONE ) {
            if( decoder_playing()) {
              amp_error( hwnd, "Cannot change active output while playing." );
            } else {
              out_set_active( i );
              WinSendMsg( hwnd, CFG_REFRESH_INFO,
                          MPFROMLONG( PLUGIN_OUTPUT ), MPFROMSHORT( i ));
            }
          }
          return 0;
        }

        case PB_OUT_UNLOAD:
        {
          SHORT i = lb_cursored( hwnd, LB_OUTPLUG );

          if( i != LIT_NONE ) {
            if( decoder_playing() && active_output == i ) {
              amp_error( hwnd, "Cannot unload currently used output." );
            } else {
              remove_output_plugin( &outputs[i] );
              if( lb_remove_item( hwnd, LB_OUTPLUG, i ) == 0 ) {
                WinSendMsg( hwnd, CFG_REFRESH_INFO,
                            MPFROMLONG( PLUGIN_OUTPUT ), MPFROMSHORT( LIT_NONE ));
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
        {
          SHORT i = lb_cursored( hwnd, LB_FILPLUG );
          if( i != LIT_NONE ) {
            filters[i].plugin_configure( hwnd, filters[i].module );
          }
          return 0;
        }

        case PB_FIL_ENABLE:
        {
          SHORT i = lb_cursored( hwnd, LB_FILPLUG );

          if( i != LIT_NONE ) {
            if( !filters[i].enabled ) {
              filters[i].enabled = TRUE;
              if( decoder_playing()) {
                amp_info( hwnd, "This filter will only be enabled after playback of the current file." );
              }
            } else {
              filters[i].enabled = FALSE;
              if( decoder_playing()) {
                amp_info( hwnd, "This filter will only be disabled after playback of the current file." );
              }
            }
            WinSendMsg( hwnd, CFG_REFRESH_INFO,
                        MPFROMLONG( PLUGIN_FILTER ), MPFROMSHORT( i ));
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
              remove_filter_plugin( &filters[i] );
              if( lb_remove_item( hwnd, LB_FILPLUG, i ) == 0 ) {
                WinSendMsg( hwnd, CFG_REFRESH_INFO,
                            MPFROMLONG( PLUGIN_FILTER ), MPFROMSHORT( LIT_NONE ));
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

/* Processes messages of the about page of the setup notebook. */
static MRESULT EXPENTRY
cfg_page5_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_COMMAND:
    case WM_CONTROL:
      return 0;

    case WM_INITDLG:
      WinSetDlgItemText( hwnd, ST_AUTHORS, SDG_AUT );
      WinSetDlgItemText( hwnd, ST_CREDITS, SDG_MSG );
      WinSetDlgItemText( hwnd, ST_TITLE1,  AMP_FULLNAME );
      return 0;
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

          if( page && page != BOOKERR_INVALID_PARAMETERS ) {
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
    {
      PSWP pswp = (PSWP)mp1;

      if( pswp[0].fl & SWP_SIZE )
      {
        SWP  swps[3];
        HWND hbtn[3];

        LONG   ibtn = sizeof(hbtn)/sizeof(HWND)-1;
        HWND   book = WinWindowFromID( hwnd, NB_CONFIG );
        LONG   i;
        RECTL  rect;
        POINTL pos[2];

        hbtn[0] = WinWindowFromID( hwnd, PB_UNDO    );
        hbtn[1] = WinWindowFromID( hwnd, PB_DEFAULT );
        hbtn[3] = WinWindowFromID( hwnd, PB_HELP    );

        // Resizes notebook window.
        if( WinQueryWindowRect( hwnd, &rect ))
        {
          WinCalcFrameRect( hwnd, &rect, TRUE );
          pos[0].x = rect.xLeft;
          pos[0].y = rect.yBottom;
          pos[1].x = rect.xRight;
          pos[1].y = rect.yTop;
          WinMapDlgPoints( hwnd, pos, 2, FALSE );
          pos[0].y += 20;
          WinMapDlgPoints( hwnd, pos, 2, TRUE  );
          WinSetWindowPos( book, 0, pos[0].x, pos[0].y,
                           pos[1].x-pos[0].x, pos[1].y-pos[0].y, SWP_MOVE | SWP_SIZE );
        }

        // Centers notebook buttons.
        if( WinQueryWindowPos( hbtn[   0], &swps[   0]) &&
            WinQueryWindowPos( hbtn[ibtn], &swps[ibtn]))
        {
          LONG move = (pswp[0].cx - (swps[ibtn].x + swps[ibtn].cx - swps[0].x))/2 - swps[0].x;

          for( i = 0; i <= ibtn; i++ )
            if( WinQueryWindowPos( hbtn[i], &swps[i] ))
              WinSetWindowPos( hbtn[i], 0, swps[i].x + move, swps[i].y, 0, 0, SWP_MOVE );
        }
      }
      break;
    }
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Creates the properties dialog. */
void
cfg_properties( HWND owner )
{
  HWND hwnd;
  HWND book;
  HWND page01;
  HWND page02;
  HWND page03;
  HWND page04;
  HWND page05;

  MRESULT id;
  char built[512];

  hwnd = WinLoadDlg( HWND_DESKTOP, owner, cfg_dlg_proc, NULLHANDLE, DLG_CONFIG, 0 );
  do_warpsans( hwnd );
  book = WinWindowFromID( hwnd, NB_CONFIG );
  do_warpsans( book );

  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROM2SHORT( 100,25 ), MPFROMSHORT( BKA_MAJORTAB ));
  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROMLONG( 0 ), MPFROMSHORT( BKA_MINORTAB ));
  WinSendMsg( book, BKM_SETNOTEBOOKCOLORS, MPFROMLONG ( SYSCLR_FIELDBACKGROUND ),
                                           MPFROMSHORT( BKA_BACKGROUNDPAGECOLORINDEX ));

  page01 = WinLoadDlg( book, book, cfg_page1_dlg_proc, NULLHANDLE, CFG_PAGE1, 0 );

  do_warpsans( page01 );
  do_warpsans( WinWindowFromID( page01, ST_PROXY_URL  ));
  do_warpsans( WinWindowFromID( page01, ST_HTTP_AUTH  ));
  do_warpsans( WinWindowFromID( page01, ST_PIXELS     ));
  do_warpsans( WinWindowFromID( page01, ST_BUFFERSIZE ));

  id = WinSendMsg( book, BKM_INSERTPAGE, 0,
                   MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR | BKA_STATUSTEXTON, BKA_FIRST ));

  WinSendMsg( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG( id ), MPFROMLONG( page01 ));
  WinSendMsg( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "~Settings" ));

  page02 = WinLoadDlg( book, book, cfg_page2_dlg_proc, NULLHANDLE, CFG_PAGE2, 0 );

  do_warpsans( page02 );
  do_warpsans( WinWindowFromID( page02, ST_CHARSET    ));

  id = WinSendMsg( book, BKM_INSERTPAGE, 0,
                   MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR | BKA_STATUSTEXTON, BKA_LAST ));

  WinSendMsg( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG( id ), MPFROMLONG( page02 ));
  WinSendMsg( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "~Display" ));

  page03 = WinLoadDlg( book, book, cfg_page3_dlg_proc, NULLHANDLE, CFG_PAGE3, 0 );

  do_warpsans( page03 );
  do_warpsans( WinWindowFromID( page03, ST_VIS_AUTHOR ));
  do_warpsans( WinWindowFromID( page03, ST_VIS_DESC   ));
  do_warpsans( WinWindowFromID( page03, ST_DEC_AUTHOR ));
  do_warpsans( WinWindowFromID( page03, ST_DEC_DESC   ));

  id = WinSendMsg( book, BKM_INSERTPAGE, 0,
                   MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR | BKA_MINOR | BKA_STATUSTEXTON, BKA_LAST ));

  WinSendMsg( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG( id ), MPFROMLONG( page03 ));
  WinSendMsg( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "~Plug-Ins" ));
  WinSendMsg( book, BKM_SETSTATUSLINETEXT, MPFROMLONG( id ), MPFROMP( "Page 1 of 2" ));

  page04 = WinLoadDlg( book, book, cfg_page4_dlg_proc, NULLHANDLE, CFG_PAGE4, 0 );

  do_warpsans( page04 );
  do_warpsans( WinWindowFromID( page04, ST_FIL_AUTHOR ));
  do_warpsans( WinWindowFromID( page04, ST_FIL_DESC   ));
  do_warpsans( WinWindowFromID( page04, ST_OUT_AUTHOR ));
  do_warpsans( WinWindowFromID( page04, ST_OUT_DESC   ));

  page05 = WinLoadDlg( book, book, cfg_page5_dlg_proc, NULLHANDLE, CFG_ABOUT, 0 );

  do_warpsans( page05 );
  do_warpsans( WinWindowFromID( page05, ST_TITLE2 ));
  do_warpsans( WinWindowFromID( page05, ST_BUILT  ));

  #if defined(__IBMC__)
    sprintf( built, "(built %s", __DATE__ );
    #if __IBMC__ <= 300
      strcat( built, " using IBM VisualAge C++ 3.08" );
    #else
      strcat( built, " using IBM VisualAge C++ 3.6"  );
    #endif
  #elif defined(__WATCOMC__)
    #if __WATCOMC__ < 1200
      sprintf( built, "(built %s using Open Watcom C++ %d.%d",
               __DATE__, __WATCOMC__ / 100, __WATCOMC__ % 100 );
    #else
      sprintf( built, "(built %s using Open Watcom C++ %d.%d",
               __DATE__, __WATCOMC__ / 100 - 11, __WATCOMC__ % 100 );
    #endif
  #endif

  strcat( built, ")" );
  WinSetDlgItemText( page05, ST_BUILT, built );

  id = WinSendMsg( book, BKM_INSERTPAGE, MPFROMLONG( id),
                   MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MINOR | BKA_STATUSTEXTON, BKA_NEXT ));

  WinSendMsg( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG( id ), MPFROMLONG( page04 ));
  WinSendMsg( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "~Plug-Ins" ));
  WinSendMsg( book, BKM_SETSTATUSLINETEXT, MPFROMLONG( id ), MPFROMP( "Page 2 of 2" ));

  id = WinSendMsg( book, BKM_INSERTPAGE, 0,
                   MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR, BKA_LAST ));

  WinSendMsg( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG( id ), MPFROMLONG( page05 ));
  WinSendMsg( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "~About" ));

  rest_window_pos( hwnd, WIN_MAP_POINTS );
  WinSetFocus( HWND_DESKTOP, book );

  WinProcessDlg   ( hwnd );
  WinDestroyWindow( hwnd );
}

