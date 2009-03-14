/*
 * Copyright 2004-2008 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <string.h>

#include "filedlg.h"
#include "pm123.rc.h"
#include "properties.h"
#include "dialog.h"
#include <utilfct.h>
#include <fileutil.h>
#include <wildcards.h>

#include <os2.h>

#include <debuglog.h>


static BOOL init_done = FALSE;
static SWP  init_file_dlg;
static SWP  init_text_filename;
static SWP  init_edit_filename;
static SWP  init_text_filter;
static SWP  init_cbox_filter;
static SWP  init_text_drive;
static SWP  init_text_files;
static SWP  init_cbox_drive;
static SWP  init_text_directory;
static SWP  init_lbox_directory;
static SWP  init_lbox_files;
static SWP  init_cbox_recurse;
static SWP  init_cbox_relative;

/* Resizes the file dialog controls. */
static void
amp_file_dlg_resize( HWND hwnd, SHORT cx, SHORT cy )
{
  SWP swp[12];
  memset( &swp, 0, sizeof( swp ));

  swp[ 0].hwnd = WinWindowFromID( hwnd, DID_FILENAME_TXT );
  swp[ 0].x    = init_text_filename.x;
  swp[ 0].y    = cy - init_file_dlg.cy + init_text_filename.y;
  swp[ 0].cy   = init_text_filename.cy;
  swp[ 0].cx   = cx - init_file_dlg.cx + init_text_filename.cx;
  swp[ 0].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 1].hwnd = WinWindowFromID( hwnd, DID_FILENAME_ED );
  swp[ 1].x    = init_edit_filename.x  + 3;
  swp[ 1].y    = cy - init_file_dlg.cy + init_edit_filename.y  + 3;
  swp[ 1].cy   = init_edit_filename.cy - 6;
  swp[ 1].cx   = cx - init_file_dlg.cx + init_edit_filename.cx - 6;
  swp[ 1].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 2].hwnd = WinWindowFromID( hwnd, DID_FILTER_TXT );
  swp[ 2].x    = init_text_filter.x;
  swp[ 2].y    = cy - init_file_dlg.cy + init_text_filter.y;
  swp[ 2].cy   = init_text_filter.cy;
  swp[ 2].cx   = cx - init_file_dlg.cx + init_text_filter.cx;
  swp[ 2].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 3].hwnd = WinWindowFromID( hwnd, DID_FILTER_CB );
  swp[ 3].x    = init_cbox_filter.x;
  swp[ 3].y    = cy - init_file_dlg.cy + init_cbox_filter.y;
  swp[ 3].cy   = init_cbox_filter.cy;
  swp[ 3].cx   = cx - init_file_dlg.cx + init_cbox_filter.cx;
  swp[ 3].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 4].hwnd = WinWindowFromID( hwnd, DID_DRIVE_TXT );
  swp[ 4].x    = init_text_drive.x;
  swp[ 4].y    = cy - init_file_dlg.cy + init_text_drive.y;
  swp[ 4].cy   = init_text_drive.cy;
  swp[ 4].cx   = (LONG)(init_text_drive.cx * ((float)cx / init_file_dlg.cx ));
  swp[ 4].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 5].hwnd = WinWindowFromID( hwnd, DID_FILES_TXT );
  swp[ 5].x    = swp[4].x + swp[4].cx  + init_text_files.x - init_text_drive.x - init_text_drive.cx;
  swp[ 5].y    = cy - init_file_dlg.cy + init_text_files.y;
  swp[ 5].cy   = init_text_files.cy;
  swp[ 5].cx   = cx - swp[5].x - init_file_dlg.cx + init_text_files.cx + init_text_files.x;
  swp[ 5].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 6].hwnd = WinWindowFromID( hwnd, DID_DRIVE_CB );
  swp[ 6].x    = init_cbox_drive.x;
  swp[ 6].y    = cy - init_file_dlg.cy + init_cbox_drive.y;
  swp[ 6].cy   = init_cbox_drive.cy;
  swp[ 6].cx   = (LONG)(init_cbox_drive.cx * ((float)cx / init_file_dlg.cx ));
  swp[ 6].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 7].hwnd = WinWindowFromID( hwnd, DID_DIRECTORY_TXT );
  swp[ 7].x    = init_text_directory.x;
  swp[ 7].y    = cy - init_file_dlg.cy + init_text_directory.y;
  swp[ 7].cy   = init_text_directory.cy;
  swp[ 7].cx   = (LONG)(init_text_directory.cx * ((float)cx / init_file_dlg.cx ));
  swp[ 7].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 8].hwnd = WinWindowFromID( hwnd, DID_DIRECTORY_LB );
  swp[ 8].x    = init_lbox_directory.x;
  swp[ 8].y    = init_lbox_directory.y;
  swp[ 8].cy   = cy - swp[8].y - init_file_dlg.cy + init_lbox_directory.cy + init_lbox_directory.y;
  swp[ 8].cx   = (LONG)(init_lbox_directory.cx * ((float)cx / init_file_dlg.cx ));
  swp[ 8].fl   = SWP_MOVE | SWP_SIZE;

  swp[ 9].hwnd = WinWindowFromID( hwnd, DID_FILES_LB );
  swp[ 9].x    = swp[8].x + swp[8].cx  + init_lbox_files.x - init_lbox_directory.x - init_lbox_directory.cx;
  swp[ 9].y    = init_lbox_files.y;
  swp[ 9].cy   = cy - swp[9].y - init_file_dlg.cy + init_lbox_files.cy + init_lbox_files.y;
  swp[ 9].cx   = cx - swp[9].x - init_file_dlg.cx + init_lbox_files.cx + init_lbox_files.x;
  swp[ 9].fl   = SWP_MOVE | SWP_SIZE;

  swp[10].hwnd = WinWindowFromID( hwnd, CB_RELATIVE );
  swp[10].x    = init_cbox_relative.x;
  swp[10].y    = init_cbox_relative.y;
  swp[10].cy   = init_cbox_relative.cy;
  swp[10].cx   = cx - init_file_dlg.cx + init_cbox_relative.cx;
  swp[10].fl   = SWP_MOVE | SWP_SIZE;

  swp[11].hwnd = WinWindowFromID( hwnd, CB_RECURSE );
  swp[11].x    = init_cbox_recurse.x;
  swp[11].y    = init_cbox_recurse.y;;
  swp[11].cy   = init_cbox_recurse.cy;
  swp[11].cx   = cx - init_file_dlg.cx + init_cbox_recurse.cx;
  swp[11].fl   = SWP_MOVE | SWP_SIZE;

  WinSetMultWindowPos( WinQueryAnchorBlock( hwnd ), swp,
                                            sizeof( swp ) / sizeof( swp[0] ));
}

/* Default dialog procedure for the file dialog. */
static MRESULT EXPENTRY
amp_file_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ DEBUGLOG2(("amp_file_dlg_proc(%x, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  
  FILEDLG* filedialog =
    (FILEDLG*)WinQueryWindowULong( hwnd, QWL_USER );

  switch( msg )
  {
    case WM_INITDLG:
      // At the first activation of the first file dialog it is necessary to
      // save its layout. In the further it will be used as a template
      // at formatting all subsequent dialogues.
      if( !init_done ) {
        WinQueryWindowPos( hwnd, &init_file_dlg );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_FILENAME_TXT  ), &init_text_filename  );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_FILENAME_ED   ), &init_edit_filename  );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_FILTER_TXT    ), &init_text_filter    );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_FILTER_CB     ), &init_cbox_filter    );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_DRIVE_TXT     ), &init_text_drive     );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_DRIVE_CB      ), &init_cbox_drive     );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_FILES_TXT     ), &init_text_files     );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_DIRECTORY_TXT ), &init_text_directory );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_DIRECTORY_LB  ), &init_lbox_directory );
        WinQueryWindowPos( WinWindowFromID( hwnd, DID_FILES_LB      ), &init_lbox_files     );
        WinQueryWindowPos( WinWindowFromID( hwnd, CB_RECURSE        ), &init_cbox_recurse   );
        WinQueryWindowPos( WinWindowFromID( hwnd, CB_RELATIVE       ), &init_cbox_relative  );
        init_done = TRUE;
      }

      if( filedialog && !(filedialog->ulUser & FDU_RECURSEBTN )) {
        WinShowWindow( WinWindowFromID( hwnd, CB_RECURSE ), FALSE );
      } else {
        WinCheckButton( hwnd, CB_RECURSE, cfg.add_recursive );
      }
      if( filedialog && !(filedialog->ulUser & FDU_RELATIVBTN )) {
        WinShowWindow( WinWindowFromID( hwnd, CB_RELATIVE ), FALSE );
      } else {
        WinCheckButton( hwnd, CB_RELATIVE, cfg.save_relative );
      }
      if( filedialog && filedialog->ulUser & FDU_DIR_ENABLE ) {
        WinEnableControl( hwnd, DID_OK, TRUE  );
      }
      do_warpsans( hwnd );
      rest_window_pos( hwnd );
      break;

    case WM_DESTROY:
      save_window_pos( hwnd );
      break;

    case WM_WINDOWPOSCHANGED:
      amp_file_dlg_resize( hwnd, ((PSWP)mp1)[0].cx, ((PSWP)mp1)[0].cy );
      break;

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == DID_FILENAME_ED && SHORT2FROMMP(mp1) == EN_CHANGE )
      {
        char file[_MAX_PATH];
        WinQueryDlgItemText( hwnd, DID_FILENAME_ED, sizeof(file), file );

        if( filedialog->ulUser & FDU_RECURSEBTN ) {
          if( !*file || strcmp( file, "*"   ) == 0 ||
                        strcmp( file, "*.*" ) == 0 )
          {
            WinEnableControl( hwnd, CB_RECURSE, TRUE  );
          } else {
            WinEnableControl( hwnd, CB_RECURSE, FALSE );
          }
        }

        // Prevents DID_OK from being greyed out.
        if( filedialog->ulUser & FDU_DIR_ENABLE ) {
          return 0;
        }
      }
      break;

    case WM_COMMAND:
      if( SHORT1FROMMP(mp1) == DID_OK )
      {
        if( filedialog->ulUser & FDU_RELATIVBTN ) {
          if( !WinQueryButtonCheckstate( hwnd, CB_RELATIVE )) {
            filedialog->ulUser &= ~FDU_RELATIV_ON;
            cfg.save_relative = FALSE;
          } else {
            filedialog->ulUser |=  FDU_RELATIV_ON;
            cfg.save_relative = TRUE;
          }
        }

        if( filedialog->ulUser & FDU_DIR_ENABLE )
        {
          char file[_MAX_PATH];
          WinQueryDlgItemText( hwnd, DID_FILENAME_ED, sizeof(file), file );

          if( !*file ||
              strcmp( file, "*"   ) == 0 ||
              strcmp( file, "*.*" ) == 0 )
          {
            if( !is_root( filedialog->szFullFile )) {
              filedialog->szFullFile[strlen(filedialog->szFullFile)-1] = 0;
            }

            filedialog->lReturn    = DID_OK;
            filedialog->ulFQFCount = 1;

            if( filedialog->ulUser & FDU_RECURSEBTN ) {
              if( !WinQueryButtonCheckstate( hwnd, CB_RECURSE )) {
                filedialog->ulUser &= ~FDU_RECURSE_ON;
                cfg.add_recursive = FALSE;
              } else {
                filedialog->ulUser |=  FDU_RECURSE_ON;
                cfg.add_recursive = TRUE;
              }
            }

            WinDismissDlg( hwnd, DID_OK );
            return 0;
          }
        }
      }
      break;

    case FDM_FILTER:
    {
      HWND  hcbox = WinWindowFromID( hwnd, DID_FILTER_CB );
      ULONG pos   = WinQueryLboxSelectedItem( hcbox );
      ULONG len   = LONGFROMMR( WinSendMsg( hcbox, LM_QUERYITEMTEXTLENGTH, MPFROMSHORT(pos), 0 ));
      char* type  = (char*)malloc( len );
      BOOL  rc    = FALSE;
      char* filt;
      char  file[_MAX_PATH];

      if( !type ) {
        return WinDefFileDlgProc( hwnd, msg, mp1, mp2 );
      }

      WinQueryLboxItemText( hcbox, pos, type, len );
      WinQueryDlgItemText ( hwnd, DID_FILENAME_ED, sizeof(file), file );

      // If the selected type is not have extensions list - that it <All Files>
      // which OS/2 always adds in the list.
      if( !strchr( type, '(' )) {
        rc = TRUE;
      } else {
        strtok( type, "(" );

        while(( filt = strtok( NULL, ";)" )) != NULL ) {
          if( wildcardfit( filt, (char*)mp1 )) {
            rc = TRUE;
            break;
          }
        }
      }

      if( rc && ( strchr( file, '*' ) || strchr( file, '?' ))) {
        rc = wildcardfit( file, (char*)mp1 );
      }

      free( type );
      return MRFROMLONG( rc );
    }
  }
  return WinDefFileDlgProc( hwnd, msg, mp1, mp2 );
}

/* This function creates and displays the file dialog
 * and returns the user's selection or selections.
 */

HWND
amp_file_dlg( HWND hparent, HWND howner, PFILEDLG filedialog )
{
  filedialog->hMod       = NULLHANDLE;
  filedialog->usDlgId    = DLG_FILE;
  filedialog->pfnDlgProc = amp_file_dlg_proc;
  filedialog->fl        |= FDS_CUSTOM|FDS_ENABLEFILELB;

  return WinFileDlg( hparent, howner, filedialog );
}

