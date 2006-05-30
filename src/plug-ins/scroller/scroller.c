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
  The second PM123 visual plugin - the smooth scroller
*/

#define INIFILE "SCROLLER.INI"

#define INCL_DOS
#define INCL_PM
#define INCL_OS2MM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utilfct.h>

#include <format.h>
#include <decoder_plug.h>
#include <plugin.h>

HWND       hwndClient = 0;
HPAL       hpal;
VISPLUGININIT plug;
APIRET        rc;
HAB        hab;
RECTL      textRect;
int        scrollPos, direction;
char       display_msg[1024];
PFN        pm123_getstring, pm123_control;
int        noinc = 0;
HPS        hpsBuffer;
HDC        hdcMem;
BITMAPINFOHEADER2 hdr;
HBITMAP    fz;
HPS        hps;
int        txtcx;
POINTL     blitbox[4];
FATTRS     fattrs;
int        wait_cntr = 0;

int getTxtcx(HPS hps, char *string)
{
   POINTL textbox[TXTBOX_COUNT];
   GpiQueryTextBox(hps, strlen(string), string, TXTBOX_COUNT, textbox);
   return textbox[TXTBOX_BOTTOMRIGHT].x-textbox[TXTBOX_BOTTOMLEFT].x;
}


MRESULT EXPENTRY PlugWinProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
 switch (msg) {
   case DM_DRAGOVER:
   case DM_DROP:
   case 0x041f:
   case 0x041e:
   case WM_CONTEXTMENU:
   case WM_BUTTON2MOTIONSTART:
   case WM_BUTTON1MOTIONSTART:
      WinSendMsg(plug.hwnd, msg, mp1, mp2);
      break;
   case WM_CLOSE:
      break;
   case WM_BUTTON1CLICK:
      pm123_control(CONTROL_NEXTMODE, NULL);
      break;
   case WM_PLUGIN_CONTROL:
      if (LONGFROMMP(mp1) == PN_TEXTCHANGED)
       {
        pm123_getstring(STR_DISPLAY_TEXT, 0, 1024, display_msg);
        WinStopTimer(hab, hwndClient, TID_USERMAX - 1);
        scrollPos = 0;
        wait_cntr = 20;
        direction = 1;

        txtcx = getTxtcx(hpsBuffer, display_msg);

        if (txtcx < plug.cx + 1)
        {
          noinc = 1;
          WinSendMsg(hwnd, WM_TIMER, 0, 0);
        }
        else
          WinStartTimer(hab, hwndClient, TID_USERMAX - 1, 60);
       }
      break;

   case WM_PAINT:
      noinc = 1;
      WinDefWindowProc (hwnd, msg, mp1, mp2);

   case WM_TIMER:
      hps = WinGetPS(hwndClient);

      WinQueryWindowRect(hwndClient, &textRect);

      textRect.xLeft -= scrollPos;
      WinDrawText(hpsBuffer, -1L, display_msg, &textRect, CLR_GREEN, CLR_BLACK, DT_LEFT | DT_VCENTER | DT_ERASERECT);

      GpiBitBlt(hps,
                hpsBuffer,
                4,
                blitbox,
                ROP_SRCCOPY,
                BBO_IGNORE);

      WinReleasePS(hps);

      if(noinc)
         noinc = 0;
      else if(wait_cntr > 0)
         wait_cntr--;
      else
      {
         if(direction) scrollPos++; else scrollPos--;

         /* if you put "else if < 1", the first one has a tendency to kick
            in when the text is shorter than it should for some reason */

         if (scrollPos > (txtcx-plug.cx + 1))
         {
            direction = 0;
            wait_cntr = 10;
         }
         if (scrollPos < 0)
         {
            direction = 1;
            wait_cntr = 10;
         }
      }

      if(txtcx < plug.cx)
         WinStopTimer(hab, hwndClient, TID_USERMAX - 1);
      break;

   default:
      return WinDefWindowProc (hwnd, msg, mp1, mp2);
 }
 return 0;
}

BOOL setFont(HPS hps, FATTRS *fattrs)
{
   if(GpiCreateLogFont(hps, NULL, 1L, fattrs) == GPI_ERROR)
     return FALSE;
   else
   {
     GpiSetCharSet(hps, 1L);
     return TRUE;
   }
}

HWND _System vis_init(PVISPLUGININIT init)
{
 SIZEL sizel;
 HINI  INIhandle;

 memcpy(&plug, init, sizeof(VISPLUGININIT));
 pm123_getstring = (PFN) plug.procs->pm123_getstring;
 pm123_control  = (PFN) plug.procs->pm123_control;

 hab = WinQueryAnchorBlock(plug.hwnd);

 WinRegisterClass(hab, "Smooth Scroller", PlugWinProc, CS_SIZEREDRAW | CS_MOVENOTIFY, 0);

 hwndClient = WinCreateWindow(plug.hwnd,
                       "Smooth Scroller",
                       "PM123 Smooth Scroller",
                       WS_VISIBLE,
                       plug.x, plug.y,
                       plug.cx, plug.cy,
                       plug.hwnd, HWND_TOP,
                       plug.id,
                       NULL, NULL);

 sizel.cx = 0;
 sizel.cy = 0;
 hdcMem = DevOpenDC(hab, OD_MEMORY, "*", 0, NULL, NULLHANDLE);
 hpsBuffer = GpiCreatePS(hab,
                   hdcMem,
                   &sizel,
                   PU_PELS | GPIT_MICRO | GPIA_ASSOC);

 memset(&hdr, 0, sizeof(BITMAPINFOHEADER2));
 hdr.cbFix     = sizeof(BITMAPINFOHEADER2);
 hdr.cx     = plug.cx;
 hdr.cy     = plug.cy;
 hdr.cPlanes   = 1;
 hdr.cBitCount = 8;
 GpiSetColor(hpsBuffer, CLR_GREEN);
 GpiSetBackColor(hpsBuffer, CLR_BLACK);
 GpiSetBackMix(hpsBuffer,BM_OVERPAINT);

 fz = GpiCreateBitmap(hpsBuffer, &hdr, 0, NULL, NULL);
 GpiSetBitmap(hpsBuffer, fz);

 memset(&fattrs, '\0', sizeof(FATTRS));
 fattrs.usRecordLength  = sizeof(FATTRS);
 fattrs.fsSelection  = 0;
 fattrs.lMatch       = 0;
 fattrs.idRegistry      = 0;
 fattrs.usCodePage      = 0;
 fattrs.lMaxBaselineExt = 14L;
 fattrs.lAveCharWidth   = 6L;
 fattrs.fsType       = 0;
 fattrs.fsFontUse    = 0;
 strcpy(fattrs.szFacename, "WarpSans Bold");

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      load_ini_value(INIhandle,fattrs);
      close_ini(INIhandle);
   }

 setFont(hpsBuffer, &fattrs);

 pm123_getstring(STR_DISPLAY_TEXT, 0, 1024, display_msg);
 scrollPos = 0;
 wait_cntr = 20;
 direction = 1;

 blitbox[0].x = blitbox[2].x = 0;
 blitbox[0].y = blitbox[2].y = 0;
 blitbox[1].x = blitbox[3].x = plug.cx;
 blitbox[1].y = blitbox[3].y = plug.cy;

 txtcx = getTxtcx(hpsBuffer, display_msg);

 if (txtcx < plug.cx + 1)
 {
   noinc = 1;
   WinSendMsg(hwndClient, WM_TIMER, 0, 0);
 }
 else
   WinStartTimer(hab, hwndClient, TID_USERMAX - 1, 60);

 return(hwndClient);
}

int _System plugin_query(PPLUGIN_QUERYPARAM query)
{
   query->type       = PLUGIN_VISUAL;
   query->author     = "Taneli Lepp„ & Samuel Audet";
   query->desc       = "Smooth Scroller";
   query->configurable = TRUE;
   return 0;
}

int _System plugin_configure(HWND hwnd, HMODULE module)
{
   FONTDLG fontdlg = {0};
   FONTMETRICS fontmetrics;

   GpiQueryFontMetrics(hpsBuffer,sizeof(fontmetrics),&fontmetrics);

   fontdlg.cbSize = sizeof(fontdlg);
   fontdlg.hpsScreen = hpsBuffer;
   fontdlg.pszFamilyname = fontmetrics.szFamilyname;
   fontdlg.usFamilyBufLen = sizeof(fontmetrics.szFamilyname);
   fontdlg.pszTitle = "PM123 Scroller Font";
   fontdlg.pszPreview = "128 kb/s, 44.1 kHz, Joint-Stereo";
   fontdlg.fl = FNTS_CENTER | FNTS_RESETBUTTON;
   fontdlg.clrFore = CLR_GREEN;
   fontdlg.clrBack = CLR_BLACK;
   fontdlg.fAttrs.usCodePage = 850;
   fontdlg.fxPointSize = MAKEFIXED((fontmetrics.lEmHeight+fontmetrics.lAveCharWidth)/2,0);
   fontdlg.flStyle = fontmetrics.fsSelection; /* doesn't always werk */
   WinFontDlg(HWND_DESKTOP, hwnd, &fontdlg);

   if(fontdlg.lReturn == DID_OK)
   {
     GpiDeleteSetId(hpsBuffer, 1L);
     fattrs = fontdlg.fAttrs;
     setFont(hpsBuffer, &fattrs);
     WinStopTimer(NULLHANDLE, hwndClient, TID_USERMAX - 1);
     scrollPos = 0;
     wait_cntr = 20;
     direction = 1;

     txtcx = getTxtcx(hpsBuffer, display_msg);

     if (txtcx < plug.cx + 1)
     {
       noinc = 1;
       WinSendMsg(hwndClient, WM_TIMER, 0, 0);
     }
     else
       WinStartTimer(hab, hwndClient, TID_USERMAX - 1, 60);
   }
   return 0;
}

int _System plugin_deinit(int unload)
{
   HINI INIhandle;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      save_ini_value(INIhandle,fattrs);
      close_ini(INIhandle);
   }

   GpiSetBitmap(hpsBuffer, NULLHANDLE);
   GpiDeleteBitmap(fz);
   GpiDeleteSetId(hpsBuffer, 1L);
   GpiDestroyPS(hpsBuffer);
   DevCloseDC(hdcMem);
   WinStopTimer(NULLHANDLE, hwndClient, TID_USERMAX - 1);
   WinDestroyWindow(hwndClient);
   return 0;
}
