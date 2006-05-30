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

/*
 * Windows'95 -style button, PM123 customized version
 *
 */

#define INCL_WIN
#define INCL_GPI
#define INCL_DOS

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "button95.h"
#include "format.h"
#include "decoder_plug.h"
#include "pm123.h"

HBITMAP hbm, hbm2;
ULONG bubbleStyle = FCF_BORDER;
FONTMETRICS fontmetrics;

/* bitmap cache for skin support */
extern HBITMAP bmp_cache[8000];

extern ULONG mainFg, mainDark, mainBright;
#define NO_BORDERS

void DrawButtonUnPressed(HPS hps, int x1, int y1, int x2, int y2, char *text, HBITMAP hbm, int id) {
  POINTL p;
#ifndef NO_BORDERS
  p.x = x1; p.y = y1;
  GpiMove(hps, &p);

  p.x = x2; p.y = y2;

  GpiSetColor(hps, CLR_BLACK);

  GpiBox(hps, DRO_OUTLINE, &p, 0, 0);

  p.x = x1; p.y = y1; p.x; p.y++;
  GpiMove(hps, &p);

//  GpiSetColor(hps, CLR_PALEGRAY);
  GpiSetColor(hps, mainFg);
  p.x = x2-1; p.y = y2;

  GpiBox(hps, DRO_FILL, &p, 0, 0);
//  GpiSetColor(hps, CLR_DARKGRAY);
  GpiSetColor(hps, mainDark);

  GpiBox(hps, DRO_OUTLINE, &p, 0, 0);

  p.y = y2; p.x = x1;
//  GpiSetColor(hps, CLR_WHITE);
  GpiSetColor(hps, mainBright);

  GpiLine(hps, &p);

  GpiMove(hps, &p);
  p.x = x2 - 1;
  GpiLine(hps, &p);
#endif

  p.x = 0; p.y = 0;

// draw the bitmap
  WinDrawBitmap(hps, hbm, NULL, &p, 0, 0, DBM_NORMAL);

// draw the text

  if (strcmp(text, "") != 0) {
     GpiSetColor(hps, CLR_BLACK);

     p.x = 24; p.y = 6;
     GpiMove(hps, &p);

     GpiCharString(hps, strlen(text), text);
  }
}

void DrawButtonPressed(HPS hps, int x1, int y1, int x2, int y2, char *text, HBITMAP hbm, int id) {
  POINTL p;

#ifndef NO_BORDERS
  p.x = x1; p.y = y1;
  GpiMove(hps, &p);

//  GpiSetColor(hps, CLR_PALEGRAY);
  GpiSetColor(hps, mainFg);
  p.x = x2; p.y = y2;

  GpiBox(hps, DRO_FILL, &p, 0, 0);

  p.x = x1; p.y = y1;
  GpiMove(hps, &p);

  p.x = x2; p.y = y2;

  GpiSetColor(hps, CLR_BLACK);

  GpiBox(hps, DRO_OUTLINE, &p, 0, 0);

  GpiSetColor(hps, CLR_DARKGRAY);
  GpiSetColor(hps, mainDark);

  p.x = 2;
  p.y = 1; GpiMove(hps, &p); p.x = x2; p.y = y2 - 1;
  GpiBox(hps, DRO_OUTLINE, &p, 0, 0);


//  GpiSetColor(hps, CLR_WHITE);
  GpiSetColor(hps, mainBright);
  p.x = x1; p.y = y1; GpiMove(hps, &p);
  p.x = x2; GpiLine(hps, &p); GpiMove(hps, &p);
  p.y = y2; GpiLine(hps, &p);
#endif

  p.x = 0; p.y = 0;

// draw the bitmap
  WinDrawBitmap(hps, hbm, NULL, &p, 0, 0, DBM_NORMAL);

// draw the text

  if (strcmp(text, "") != 0) {

    GpiSetColor(hps, CLR_BLACK);
    p.x = 24; p.y = 6;
    GpiMove(hps, &p);

    GpiCharString(hps, strlen(text), text);
  }
}


MRESULT EXPENTRY ButtonWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
 HPS hps;
 POINTL p;
 SWP swp;
 PDATA95 b, bbb;
 PCREATESTRUCT bb;
 RECTL rcl;
 POINTL ptl;
 int yy;
 ULONG rgb = 255 * 65536 + 255 * 256 + 128, foo;

 switch (msg) {
   case WM_CHAR:
          WinSendMsg(bbb->hwndOwner, msg, mp1, mp2);
          break;
   case WM_SETTEXT:
          bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);

          strcpy(bbb->Help, (char *)mp1);
          if (bbb->bubbling == 1) {
              WinSetWindowText(bbb->BubbleClient, bbb->Help);
              WinSetWindowPos(bbb->Bubble,
                              HWND_TOP,
                              0,
                              0,
                              (strlen(bbb->Help) * 5) + 6,
                              18,
                              SWP_SIZE | SWP_SHOW);
          }
          break;
   case WM_TIMER:
                hps = WinGetPS(hwnd);
                WinStopTimer(NULLHANDLE, hwnd, TID_USERMAX - 10);

                bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                if (bbb->mouse_in_button == 1) {
                   if (bbb->bubbling == 0) {
                     bbb->Bubble = WinCreateStdWindow(HWND_DESKTOP,
                                                      0,
                                                      &bubbleStyle,
                                                      WC_STATIC,
                                                      "",
                                                      SS_TEXT | DT_CENTER | DT_VCENTER,
                                                      NULLHANDLE,
                                                      666,
                                                      &bbb->BubbleClient);

                     WinQueryPointerPos(HWND_DESKTOP, &ptl);

                     WinSetPresParam(bbb->BubbleClient, PP_FONTNAMESIZE, 13, "3.System VIO");
                     foo = GpiQueryNearestColor(hps, 0, rgb);
                     WinSetPresParam(bbb->BubbleClient, PP_BACKGROUNDCOLORINDEX, 4, &foo);

//                     WinSetPresParam(bbb->BubbleClient, PP_BACKGROUNDCOLOR, sizeof(RGB), &rgb);

                     WinSetWindowText(bbb->BubbleClient, bbb->Help);

                     if ((ptl.y - 20) > 1) yy = ptl.y - 30; else yy = ptl.y + 17;

                     WinSetWindowPos(bbb->Bubble,
                                     HWND_TOP,
                                     ptl.x, yy,
                                     (strlen(bbb->Help) * 5) + 6,
                                     18,
                                     SWP_SIZE | SWP_MOVE | SWP_SHOW);

                     bbb->bubbling = 1;
                   }

                }
                WinReleasePS(hps);
                break;
   case 0x041e:
                bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);

                bbb->mouse_in_button = 1;
                WinSendMsg(bbb->hwndOwner, 0x041e, 0, 0);
                WinStartTimer(NULLHANDLE, hwnd, TID_USERMAX - 10, 1000);
                break;
   case 0x041f:
                bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);

                if (bbb->bubbling == 1) {
                  WinDestroyWindow(bbb->Bubble);
                  bbb->bubbling = 0;
                }
                WinSendMsg(bbb->hwndOwner, 0x041f, 0, 0);
                bbb->mouse_in_button = 0;
                break;
   case WM_DEPRESS:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                  bbb->Pressed = 0;
                  if (bbb->stick == 1) {
                    *bbb->stickvar = 0;
                  }
                  WinInvalidateRect(hwnd, NULL, 1);
                  break;
   case WM_CHANGEBMP:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                  bbb->bmp2 = LONGFROMMP(mp2);
                  bbb->bmp1 = LONGFROMMP(mp1);
                  WinInvalidateRect(hwnd, NULL, 1);
                  break;
   case WM_PRESS:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                  bbb->Pressed = 1;
                  if (bbb->stick == 1) {
                    *bbb->stickvar = 1;
                  }
                  WinInvalidateRect(hwnd, NULL, 1);
                  break;
   case WM_CONTEXTMENU:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                  WinPostMsg(bbb->hwndOwner, WM_CONTEXTMENU, 0, 0);
                  break;
   case WM_BUTTON1DOWN:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                  bbb->Pressed = 1;
                  bbb->mouse_in_button = 0; /* We don't want bubbling after
                                               we've clicked on the button */

                  WinFocusChange(HWND_DESKTOP, bbb->hwndOwner, 0);
                  WinInvalidateRect(hwnd, NULL, 1);

                  WinSetCapture(HWND_DESKTOP, hwnd);

                  break;
   case WM_BUTTON1UP:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                  bbb->mouse_in_button = 0; /* We don't want bubbling after
                                               we've clicked on the button */

                  WinSetCapture(HWND_DESKTOP, NULLHANDLE);
                  p.x = ((POINTS *)&mp1)->x;
                  p.y = ((POINTS *)&mp1)->y;
                  WinQueryWindowRect(hwnd, &rcl);

                  if (WinPtInRect( WinQueryAnchorBlock( hwnd ), &rcl, &p) && bbb->Pressed == 1) {
                      bbb->Pressed = 0;
                      if (bbb->stick == 1) {
                        *bbb->stickvar = !*bbb->stickvar;
                        WinInvalidateRect(hwnd, NULL, 1);
                      }
                      WinSendMsg(bbb->hwndOwner, WM_COMMAND, MPFROMLONG(bbb->id), MPFROM2SHORT(0, SHORT2FROMMP(mp2)));
                  }
                  bbb->Pressed = 0;
                  WinInvalidateRect(hwnd, NULL, 1);

                  break;
   case WM_CREATE:
                  DosAllocMem((PPVOID)&b, sizeof(DATA95), PAG_READ | PAG_WRITE | PAG_COMMIT);
                  WinSetWindowPtr(hwnd, QWL_USER, (PVOID)b);

                  bb = (PCREATESTRUCT)PVOIDFROMMP(mp2);

                  bbb = (PDATA95)PVOIDFROMMP(mp1);
                  memcpy(b, bbb, sizeof(DATA95));
                  b->id = bb->id;
                  b->Pressed = 0;
                  b->bubbling = 0;

                  break;
   case WM_ERASEBACKGROUND:
                  WinQueryWindowRect(hwnd, &rcl);
                  hps = WinGetPS(hwnd);
                  WinFillRect(hps, &rcl, CLR_BLACK);
                  WinReleasePS(hps);
                  return((MRESULT)1);
                  break;
   case WM_PAINT:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);

                  hps  = WinBeginPaint(hwnd, NULLHANDLE, NULL);
//                  hbm  = GpiLoadBitmap(hps, NULLHANDLE, bbb->bmp2, 0, 0);
//                  hbm2 = GpiLoadBitmap(hps, NULLHANDLE, bbb->bmp1, 0, 0);
                  /* Skin support */
                  hbm  = bmp_cache[bbb->bmp2];
                  hbm2 = bmp_cache[bbb->bmp1];

                  WinQueryWindowPos(hwnd, &swp);

//                  if (playing == 0) pause_on = 0;

                  if (bbb->stick == 1) {
                     if (bbb->Pressed == 0 && *bbb->stickvar == 0) DrawButtonUnPressed(hps, 1, 1, swp.cx, swp.cy, "", hbm2, bbb->id); else
                                                                   DrawButtonPressed(hps, 1, 1, swp.cx, swp.cy, "", hbm, bbb->id);
                  } else {
                     if (bbb->Pressed == 0) DrawButtonUnPressed(hps, 1, 1, swp.cx, swp.cy, "", hbm2, bbb->id); else
                                            DrawButtonPressed(hps, 1, 1, swp.cx, swp.cy, "", hbm, bbb->id);
                  }

//                  GpiDeleteBitmap(hbm);
//                  GpiDeleteBitmap(hbm2);

                  bbb->wmpaint_kludge = 1;

                  WinEndPaint(hps);
                  break;
   case WM_DESTROY:
                  bbb = (PDATA95)WinQueryWindowPtr(hwnd, QWL_USER);
                  DosFreeMem((PVOID)bbb);
                  break;
   default:       return WinDefWindowProc (hwnd, msg, mp1, mp2);
 }
 return 0;
}

/*
 * Uncomment for main()
 *
MRESULT EXPENTRY MyWinProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
 BUTTONDATA piip;

 switch (msg) {
   case WM_CREATE:
               strcpy(piip.Text, "Testi");
               WinCreateWindow(hwnd, CLASSNAME, "Foo", WS_VISIBLE, 10, 10, 100, 60,
                               hwnd, HWND_TOP, 1024, &piip, NULL);

               break;
   case WM_ERASEBACKGROUND:
            return MRFROMLONG(TRUE);
            break;
   case WM_BUTTON1CLICK:
            DosBeep(500, 100);
            break;
   default:
           return WinDefWindowProc (hwnd, msg, mp1, mp2);
 }
 return 0;
}*/


void InitButton(HAB hab) {
  WinRegisterClass(hab, CLASSNAME, (PFNWP)ButtonWndProc,
                   CS_SYNCPAINT | CS_PARENTCLIP | CS_SIZEREDRAW,
                   16);
}

/*
 * Uncomment for testing purposes only ;-)
 *
void main() {
 ULONG flCtlData = FCF_SIZEBORDER | FCF_TASKLIST | FCF_TITLEBAR | FCF_SYSMENU;

 HAB  hab = WinInitialize(0);
 HMQ  hmq = WinCreateMsgQueue( hab, 0);
 HWND hwndFrame, hwndClient;
 QMSG qms;

 InitButton(hab);

 WinRegisterClass(hab, "template", MyWinProc, CS_SIZEREDRAW, 0);

 hwndFrame = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE,
            &flCtlData, "template", "Template",
            0, NULLHANDLE, 0, &hwndClient);

 WinSetWindowPos(hwndFrame, HWND_TOP, 1, 1, 140, 120,
                 SWP_ACTIVATE | SWP_MOVE | SWP_SIZE | SWP_SHOW);

 while (WinGetMsg(hab, &qms, (HWND)0, 0, 0)) WinDispatchMsg(hab, &qms);

 WinDestroyMsgQueue(hmq);
 WinTerminate(hab);

}*/

