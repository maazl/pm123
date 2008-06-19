/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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
#define  INCL_GPI
#define  INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "button95.h"
#include <utilfct.h>

static MRESULT EXPENTRY
ButtonWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  PDATA95 btn = (PDATA95)WinQueryWindowPtr( hwnd, QWL_USER );

  switch( msg ) {
    case WM_CHAR:
    case WM_CONTEXTMENU:
      WinSendMsg( btn->hwnd_owner, msg, mp1, mp2 );
      break;

    case WM_SETHELP:
      strlcpy( btn->help, (char*)mp1, sizeof( btn->help ));

      if( btn->bubbling == 1 ) {
        WinSetWindowText( btn->hwnd_bubble, btn->help );
        WinSetWindowPos ( btn->hwnd_bubble, HWND_TOP, 0, 0,
                          strlen( btn->help ) * 5 + 6, 18, SWP_SIZE | SWP_SHOW);
      }
      break;

    case WM_TIMER:
    {
      HPS    hps = WinGetPS( hwnd );
      POINTL pos;
      ULONG  rgb = 255 * 65536 + 255 * 256 + 128;

      WinStopTimer( NULLHANDLE, hwnd, TID_USERMAX - 10 );

      if( btn->mouse_in_button == 1 ) {
        if( btn->bubbling == 0 )
        {
          ULONG style = FCF_BORDER;

          btn->hwnd_bubble_frame =
            WinCreateStdWindow( HWND_DESKTOP, 0, &style, WC_STATIC, "",
                                SS_TEXT | DT_CENTER | DT_VCENTER,
                                NULLHANDLE, 666, &btn->hwnd_bubble );

          WinQueryPointerPos( HWND_DESKTOP, &pos );
          WinSetPresParam( btn->hwnd_bubble, PP_FONTNAMESIZE, 13, "3.System VIO" );
          rgb = GpiQueryNearestColor( hps, 0, rgb );
          WinSetPresParam( btn->hwnd_bubble, PP_BACKGROUNDCOLORINDEX, 4, &rgb );
          WinSetWindowText( btn->hwnd_bubble, btn->help );

          if(( pos.y - 20) > 1 ) {
            pos.y -= 30;
          } else {
            pos.y += 17;
          }

          WinSetWindowPos( btn->hwnd_bubble_frame, HWND_TOP, pos.x, pos.y,
                           strlen( btn->help ) * 5 + 6, 18, SWP_SIZE | SWP_MOVE | SWP_SHOW );
          btn->bubbling = 1;
        }
      }
      WinReleasePS( hps );
      break;
    }

    case 0x041e:
      btn->mouse_in_button = 1;
      WinSendMsg( btn->hwnd_owner, msg, mp1, mp2 );
      WinStartTimer( NULLHANDLE, hwnd, TID_USERMAX - 10, 1000 );
      break;

    case 0x041f:
      if( btn->bubbling == 1 ) {
        WinDestroyWindow( btn->hwnd_bubble_frame );
        btn->bubbling = 0;
      }
      WinSendMsg( btn->hwnd_owner, msg, mp1, mp2 );
      btn->mouse_in_button = 0;
      break;

    case WM_DEPRESS:
      btn->pressed = 0;
      if( btn->stick == 1 ) {
         *btn->stickvar = 0;
      }
      WinInvalidateRect( hwnd, NULL, 1 );
      break;

    case WM_PRESS:
      btn->pressed = 1;
      if( btn->stick == 1 ) {
         *btn->stickvar = 1;
      }
      WinInvalidateRect( hwnd, NULL, 1 );
      break;

    case WM_CHANGEBMP:
      btn->bmp_release = (HBITMAP*)mp1;
      btn->bmp_pressed = (HBITMAP*)mp2;
      WinInvalidateRect( hwnd, NULL, 1 );
      break;

    case WM_BUTTON1DOWN:
      btn->pressed = 1;
      btn->mouse_in_button = 0; // We don't want bubbling after we've clicked on the button.

      WinFocusChange( HWND_DESKTOP, btn->hwnd_owner, 0 );
      WinInvalidateRect( hwnd, NULL, 1 );
      WinSetCapture( HWND_DESKTOP, hwnd );
      break;

    case WM_BUTTON1UP:
    {
      POINTL pos;
      RECTL  rec;

      pos.x = SHORT1FROMMP(mp1);
      pos.y = SHORT2FROMMP(mp1);

      btn->mouse_in_button = 0; // We don't want bubbling after we've clicked on the button.
      WinSetCapture( HWND_DESKTOP, NULLHANDLE );
      WinQueryWindowRect( hwnd, &rec );

      if( WinPtInRect( WinQueryAnchorBlock( hwnd ), &rec, &pos ) && btn->pressed == 1 )
      {
        btn->pressed = 0;
        if( btn->stick == 1 ) {
           *btn->stickvar = !*btn->stickvar;
            WinInvalidateRect( hwnd, NULL, 1 );
        }

        WinSendMsg( btn->hwnd_owner, WM_COMMAND,
                    MPFROMLONG( btn->id ), MPFROM2SHORT( 0, SHORT2FROMMP( mp2 )));
      }
      btn->pressed = 0;
      WinInvalidateRect( hwnd, NULL, 1 );
      break;
    }

    case WM_CREATE:
    {
      DATA95* data = (PDATA95)malloc( sizeof( DATA95 ));

      WinSetWindowPtr( hwnd, QWL_USER, (PVOID)data );
      *data = *(PDATA95)PVOIDFROMMP(mp1);

      data->id       = ((PCREATESTRUCT)PVOIDFROMMP(mp2))->id;
      data->pressed  = 0;
      data->bubbling = 0;
      break;
    }

    case WM_ERASEBACKGROUND:
      break;

    case WM_PAINT:
    {
      HPS    hps = WinBeginPaint( hwnd, NULLHANDLE, NULL );
      POINTL pos = { 0, 0 };

      if( btn->pressed == 0 && ( btn->stick == 0 || *btn->stickvar == 0 )) {
        WinDrawBitmap( hps, *btn->bmp_release, NULL, &pos, 0, 0, DBM_NORMAL );
      } else {
        WinDrawBitmap( hps, *btn->bmp_pressed, NULL, &pos, 0, 0, DBM_NORMAL );
      }
      WinEndPaint(hps);
      break;
    }

    case WM_DESTROY:
      free( btn );
      break;

    default:
      return WinDefWindowProc( hwnd, msg, mp1, mp2 );
  }
  return 0;
}

void
InitButton( HAB hab ) {
  WinRegisterClass( hab, CLASSNAME, (PFNWP)ButtonWndProc,
                    CS_SYNCPAINT | CS_PARENTCLIP | CS_SIZEREDRAW, 16 );
}

