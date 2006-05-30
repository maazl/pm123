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

#define INCL_DOS 
#define INCL_PM
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "utilfct.h"

/********************************************************/
/* for non-dialog windows with controls, use in WM_CHAR */
/********************************************************/

void doControlNavigation(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
   if (!(SHORT1FROMMP(mp1) & KC_KEYUP))
   {
      /* implement tab and arrow movements around the dialog items */
      if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
         switch(SHORT2FROMMP(mp2))
         {
            case VK_DOWN:
            case VK_RIGHT:
               WinSetFocus(HWND_DESKTOP,WinEnumDlgItem(hwnd,WinQueryFocus(HWND_DESKTOP), EDI_NEXTGROUPITEM));
               break;
            case VK_UP:
            case VK_LEFT:
               WinSetFocus(HWND_DESKTOP,WinEnumDlgItem(hwnd,WinQueryFocus(HWND_DESKTOP), EDI_PREVGROUPITEM));
               break;
            case VK_TAB:
               WinSetFocus(HWND_DESKTOP,WinEnumDlgItem(hwnd,WinQueryFocus(HWND_DESKTOP), EDI_NEXTTABITEM));
               break;
            case VK_BACKTAB:
               WinSetFocus(HWND_DESKTOP,WinEnumDlgItem(hwnd,WinQueryFocus(HWND_DESKTOP), EDI_PREVTABITEM));
               break;
         }

      /* match mnemonic of buttons, and send BM_CLICK */
      else if ((SHORT1FROMMP(mp1) & KC_CHAR) || (SHORT1FROMMP(mp1) & KC_ALT))
      {
         HWND itemhwnd = WinEnumDlgItem(hwnd, 0, EDI_FIRSTTABITEM);

         do if ((BOOL)WinSendMsg(itemhwnd, WM_MATCHMNEMONIC, mp2, 0))
               WinSendMsg(itemhwnd, BM_CLICK, SHORT1FROMMP(FALSE), 0);
         while((itemhwnd = WinEnumDlgItem(hwnd, itemhwnd, EDI_NEXTTABITEM))
                != WinEnumDlgItem(hwnd, 0, EDI_FIRSTTABITEM));
      }
   }
}

static BOOL have_warpsans = -1;

BOOL check_warpsans(void)
{
   if(have_warpsans == -1)
   {
      LONG fontcounter = 0;
      HPS hps;
      BOOL rc;

      hps = WinGetPS(HWND_DESKTOP);
      rc = GpiQueryFonts(hps, QF_PUBLIC,"WarpSans", &fontcounter, 0, NULL);
      WinReleasePS(hps);
      return rc != 0 && rc != GPI_ALTERROR;
   }
   else
      return have_warpsans;
}


void do_warpsans(HWND hwnd)
{
   char *font;
   if(have_warpsans == -1)
      have_warpsans = check_warpsans();

   if(have_warpsans)
      font = "9.WarpSans";
   else
      font = "8.Helv";

   WinSetPresParam(hwnd, PP_FONTNAMESIZE, strlen(font)+1, font);
}

/* Function to SuperClass window classes (to catch WM_CREATE) */

PFNWP SuperclassWindow(HAB hab, PSZ szWinClass,PFNWP pNewWinProc)
{
    CLASSINFO ci;
    PFNWP     p;
    if(WinQueryClassInfo(hab,szWinClass,&ci))
    {
       p = ci.pfnWindowProc;
       if (!WinRegisterClass(
          hab,
          szWinClass,
          pNewWinProc,
          ci.flClassStyle & (~CS_PUBLIC),
          ci.cbWindowData))
             return (PFNWP)0;
       return p;
    }
    return (PFNWP)0;
}


HWND createClientFrame(HAB hab, HWND *hwndClient, char *classname,
             ULONG *frameFlags, PFNWP winproc, char *title)
{
   HWND HwndFrame;

   WinRegisterClass( hab, classname, winproc,
                  CS_CLIPCHILDREN | CS_SIZEREDRAW, 0 );

   HwndFrame = WinCreateStdWindow (HWND_DESKTOP, 0, frameFlags, classname,
                                 title, 0, 0, 1, hwndClient);

   return HwndFrame;
}

BOOL loadPosition(HWND hwnd, char *inifilename)
{
   SWP pos;
   HINI inifile;
   ULONG state;
   BOOL returnBool = FALSE;

   inifile = open_ini(inifilename);

   /* get window position from INI file */
   if(inifile)
   {
      ULONG sizeof_state = sizeof(state),
            sizeof_posx = sizeof(pos.x),
            sizeof_posy = sizeof(pos.y),
            sizeof_poscx = sizeof(pos.cx),
            sizeof_poscy = sizeof(pos.cy);

      if( PrfQueryProfileData(inifile,"Position","state",&state, &sizeof_state) &&
          PrfQueryProfileData(inifile,"Position","x",&pos.x, &sizeof_posx) &&
          PrfQueryProfileData(inifile,"Position","y",&pos.y, &sizeof_posy) &&
          PrfQueryProfileData(inifile,"Position","cx",&pos.cx, &sizeof_poscx) &&
          PrfQueryProfileData(inifile,"Position","cy",&pos.cy, &sizeof_poscy) )
      {

         WinSetWindowPos( hwnd, NULLHANDLE,
                          pos.x, pos.y, pos.cx, pos.cy,
                          SWP_SIZE | SWP_MOVE | SWP_SHOW | state );
         returnBool = TRUE;
      }

      close_ini(inifile);
   }
   return returnBool;
}

BOOL savePosition(HWND hwnd, char *inifilename)
{
   SWP pos;
   HINI inifile;
   ULONG state;
   BOOL returnBool = FALSE;

   inifile = open_ini(inifilename);

   /* save window position in INI file */
   if (inifile)
   {
      if (WinQueryWindowPos(hwnd, &pos))
      {
         state = 0;
         if (pos.fl & SWP_MINIMIZE)
         {
            pos.x = (LONG) WinQueryWindowUShort(hwnd, QWS_XRESTORE);
            pos.y = (LONG) WinQueryWindowUShort(hwnd, QWS_YRESTORE);
            pos.cx = (LONG) WinQueryWindowUShort(hwnd, QWS_CXRESTORE);
            pos.cy = (LONG) WinQueryWindowUShort(hwnd, QWS_CYRESTORE);
            state = SWP_MINIMIZE;
         }
         else if (pos.fl & SWP_MAXIMIZE)
         {
            pos.x = (LONG) WinQueryWindowUShort(hwnd, QWS_XRESTORE);
            pos.y = (LONG) WinQueryWindowUShort(hwnd, QWS_YRESTORE);
            pos.cx = (LONG) WinQueryWindowUShort(hwnd, QWS_CXRESTORE);
            pos.cy = (LONG) WinQueryWindowUShort(hwnd, QWS_CYRESTORE);
            state = SWP_MAXIMIZE;
         }

         PrfWriteProfileData(inifile, "Position", "state", &state, sizeof(state));
         PrfWriteProfileData(inifile, "Position", "x", &pos.x, sizeof(pos.x));
         PrfWriteProfileData(inifile, "Position", "y", &pos.y, sizeof(pos.y));
         PrfWriteProfileData(inifile, "Position", "cx", &pos.cx, sizeof(pos.cx));
         PrfWriteProfileData(inifile, "Position", "cy", &pos.cy, sizeof(pos.cy));
         returnBool = TRUE;
      }
      close_ini(inifile);
   }
   return returnBool;
}

/**************************/
/* error display handling */
/**************************/

#define ST_ERROR 42

int errornum;       /* error counter */
HWND hwnderror = 0;

PFNWP wpStatic;

/* calling thread needs a message queue */
void _System updateError(char *fmt, ...)
{
   va_list args;
   char buffer[512];

   sprintf(buffer,"%d. ",errornum++);
   va_start(args, fmt);

   if(!hwnderror)
   {
      fprintf(stderr,"\n");
      vfprintf(stderr,fmt,args);
      fprintf(stderr,"\n");
   }
   else
   {
      vsprintf(strchr(buffer,0),fmt,args);
      WinSetWindowText(hwnderror, buffer);
   }
   va_end(args);
}

MRESULT EXPENTRY wpError(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
   switch(msg)
   {
      case WM_BUTTON1CLICK:
         WinSetWindowText(hwnd, "");
         return 0;
   }

   return wpStatic(hwnd,msg,mp1,mp2);
}

void initError(HWND mainhwnd, LONG x, LONG y, LONG cx, LONG cy)
{
   LONG color = CLR_RED;
   errornum = 1;
   hwnderror = WinCreateWindow(mainhwnd, WC_STATIC, "",
               WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER,
               x, y, cx, cy,
               mainhwnd, HWND_TOP, ST_ERROR, NULL, NULL);

   WinSetPresParam(hwnderror, PP_FOREGROUNDCOLORINDEX, sizeof(color), &color);

   wpStatic = WinSubclassWindow(hwnderror, wpError);
}

void unInitError(void)
{
   WinDestroyWindow(hwnderror);
}


/**************************/
/* status display handling */
/**************************/

#define ST_STATUS 43

HWND hwndstatus = 0;

/* calling thread needs a message queue */
void _System updateStatus(char *fmt, ...)
{
   va_list args;
   char buffer[512];

   va_start(args, fmt);

   if(!hwndstatus)
   {
      vfprintf(stdout,fmt,args);
      fprintf(stdout,"\n");
   }
   else
   {
      vsprintf(buffer,fmt,args);
      WinSetWindowText(hwndstatus, buffer);
   }
   va_end(args);
}

void initStatus(HWND mainhwnd, LONG x, LONG y, LONG cx, LONG cy)
{
   LONG color = CLR_BLACK;
   hwndstatus = WinCreateWindow(mainhwnd, WC_STATIC, "",
               WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER,
               x, y, cx, cy,
               mainhwnd, HWND_TOP, ST_STATUS, NULL, NULL);

   WinSetPresParam(hwndstatus, PP_FOREGROUNDCOLORINDEX, sizeof(color), &color);
}

void unInitStatus(void)
{
   WinDestroyWindow(hwndstatus);
}


/************/
/* Notebook */
/************/

HWND createNotebook(HWND hwnd, NBPAGE *nbpage, ULONG pageCount)
{
   HWND hwndNB;
   int i;

   /* create notebook, here if the warp 4 style doesn't show,
      I am trying to minic it with the old style one */
   hwndNB =  WinCreateWindow( hwnd, WC_NOTEBOOK, NULL, WS_VISIBLE |
             BKS_TABBEDDIALOG | /* BKS_BUTTONAREA | */ WS_GROUP | WS_TABSTOP |
             /* needed for old style customizeable notebook */
             BKS_BACKPAGESTR | BKS_MAJORTABTOP | BKS_ROUNDEDTABS |
             BKS_STATUSTEXTCENTER | BKS_SPIRALBIND | BKS_TABTEXTLEFT,
             0, 0, 0, 0, hwnd, HWND_TOP, FID_CLIENT, NULL, NULL );

   if(!hwndNB) return FALSE;

   /* change colors for old style notebook not to look ugly */
   WinSendMsg(hwndNB, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_FIELDBACKGROUND),
              MPFROMSHORT(BKA_BACKGROUNDPAGECOLORINDEX));

   /* change tab width for old style notebook to something OK for most font size */
   WinSendMsg(hwndNB, BKM_SETDIMENSIONS, MPFROM2SHORT(80,25),
              MPFROMSHORT(BKA_MAJORTAB));

   /* no minor */
   WinSendMsg(hwndNB, BKM_SETDIMENSIONS, 0, MPFROMSHORT(BKA_MINORTAB));

   for(i = 0; i < pageCount; i++)
   {
      nbpage[i].ulPageId = (LONG)WinSendMsg(hwndNB,  BKM_INSERTPAGE, NULL,
           MPFROM2SHORT((BKA_STATUSTEXTON | BKA_AUTOPAGESIZE | nbpage[i].usTabType),
           BKA_LAST));

      if ( !nbpage[i].ulPageId)
        return FALSE;

      if ( !WinSendMsg(hwndNB, BKM_SETSTATUSLINETEXT, MPFROMLONG(nbpage[i].ulPageId),
           MPFROMP(nbpage[i].szStatusLineText)))
        return FALSE;

      if ( !WinSendMsg(hwndNB, BKM_SETTABTEXT, MPFROMLONG(nbpage[i].ulPageId),
           MPFROMP(nbpage[i].szTabText)))
        return FALSE;

      if (!WinSendMsg( hwndNB, BKM_SETPAGEDATA, MPFROMLONG(nbpage[i].ulPageId),
                       MPFROMP(&nbpage[i])))
        return FALSE;

                        /* if page is Major and Minor, set the Minor tab text so popup menu is more
             consistent. */
         if ( (nbpage[i].usTabType & BKA_MAJOR) && (nbpage[i].usTabType & BKA_MINOR) )
         {
            BOOKPAGEINFO    bpInfo; 

                           memset (&bpInfo ,0, sizeof(bpInfo));
                           bpInfo.cb          = sizeof(bpInfo);
                           bpInfo.fl          = BFA_MINORTABTEXT;
                           bpInfo.cbMinorTab  = strlen( nbpage[i].szTabText );
                           bpInfo.pszMinorTab = nbpage[i].szTabText;
                           if ( !WinSendMsg( hwndNB,
                                       BKM_SETPAGEINFO,
                                       MPFROMLONG( nbpage[i].ulPageId),
                                       MPFROMP(&bpInfo)) )
               return FALSE;
         }
   }

   /* return success (notebook window handle) */
   return hwndNB;
}

BOOL loadNotebookDlg(HWND hwndNotebook, NBPAGE *nbpage, ULONG pageCount)
{
   int i;
   for(i = 0; i < pageCount; i++)
   {
      /* loading dialog page frame */
      nbpage[i].hwnd = WinLoadDlg(hwndNotebook, hwndNotebook,
                             nbpage[i].pfnwpDlg, 0, nbpage[i].idDlg, NULL);

      if(!nbpage[i].hwnd) return FALSE;

   //   WinSetPresParam(nbpage[i].hwnd, PP_FONTNAMESIZE, strlen(font)+1, font);

      WinSendMsg(hwndNotebook, BKM_SETPAGEWINDOWHWND,
         MPFROMLONG(nbpage[i].ulPageId), MPFROMHWND(nbpage[i].hwnd));
   }

   /* I want the focus on the notebook tab after load */
   WinSetFocus(HWND_DESKTOP, hwndNotebook);

   return TRUE;
}

/****************************************************/
/* processing of popup menu for containers, fiuf... */
/****************************************************/

/* check if the source record is selected, and if so, return TRUE */
BOOL isSourceSelected(HWND hwnd, RECORDCORE *record)
{
   RECORDCORE *selectedRecord;

   selectedRecord = searchRecords(hwnd, 0,
                    (RECORDCORE *) CMA_FIRST, CRA_SELECTED);

   while(selectedRecord && selectedRecord != (RECORDCORE *) -1)
   {
      if(selectedRecord == record)
         return TRUE;
      selectedRecord = searchRecords(hwnd, 0,
                  (RECORDCORE *) selectedRecord, CRA_SELECTED);
   }
   return FALSE;
}

/* sets source emphasis for all selected records */
void setSelectedSourceEmphasis(HWND hwnd, BOOL on)
{
   RECORDCORE *selectedRecord;
   selectedRecord = searchRecords(hwnd, 0,
                    (RECORDCORE *) CMA_FIRST, CRA_SELECTED);

   while(selectedRecord && selectedRecord != (RECORDCORE *) -1)
   {
      setRecordSource(hwnd, 0, (RECORDCORE *) selectedRecord, on);
      selectedRecord = searchRecords(hwnd, 0,
                  (RECORDCORE *) selectedRecord, CRA_SELECTED);
   }
}

/* for WM_ENDMENU, use return info from processPopUp */
BOOL removeSourceEmphasis(HWND hwnd, removeSourceEmphasisInfo *info)
{
   if(info->PUMHwnd != hwnd) return FALSE;

   if(isSourceSelected(info->CTHwnd, info->sourceRecord))
      setSelectedSourceEmphasis(info->CTHwnd,FALSE);
   else
      setRecordSource(info->CTHwnd, 0, (RECORDCORE *) info->sourceRecord, FALSE);

   return TRUE;
}

/* hwnd = dialog who receives notification
   id = container ID
   record = source record
   recordIDM = record menu ID
   container IDM = container menu ID
   info = returned info for use with removeSourceEmphasis and others

   use in CN_CONTEXTMENU */
void processPopUp(HWND hwnd, LONG id, RECORDCORE *record,
     LONG recordIDM, LONG containerIDM, removeSourceEmphasisInfo *info)
{
   info->CTHwnd = WinWindowFromID(hwnd,id);
   info->sourceRecord = record;

   setRecordSource(hwnd, id, (RECORDCORE *) record, TRUE);

   if(record)
   {
      CNRINFO cnrInfo;

      /* source emphasis up all the selected records if source record is */
      if(isSourceSelected(WinWindowFromID(hwnd, id), record))
         setSelectedSourceEmphasis(WinWindowFromID(hwnd, id),TRUE);

      info->PUMHwnd = WinLoadMenu(hwnd, 0, recordIDM);

      WinSendDlgItemMsg(hwnd,id,CM_QUERYCNRINFO, MPFROMP(&cnrInfo),
                        MPFROMLONG(sizeof(cnrInfo)));

      /* copy cat the WPS which uses the mouse position for detail view */
      if(cnrInfo.flWindowAttr & CV_DETAIL)
      {
         POINTL mousepos;
         WinQueryPointerPos(HWND_DESKTOP, &mousepos);
         WinMapWindowPoints(HWND_DESKTOP, hwnd, &mousepos, 1);
         WinPopupMenu(hwnd, hwnd, info->PUMHwnd, mousepos.x, mousepos.y, 0,
                      PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_KEYBOARD);
      }
      else
      {
         RECTL recordpos;
         getRecordPosition(hwnd,id,record,&recordpos, CMA_ICON);
         WinPopupMenu(info->CTHwnd, hwnd, info->PUMHwnd, recordpos.xRight, recordpos.yBottom, 0,
                      PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_KEYBOARD);
      }
   }
   else
   {
      POINTL mousepos;
      WinQueryPointerPos(HWND_DESKTOP, &mousepos);
      WinMapWindowPoints(HWND_DESKTOP, hwnd, &mousepos, 1);
      info->PUMHwnd = WinLoadMenu(hwnd, 0, containerIDM);
      WinPopupMenu(hwnd, hwnd, info->PUMHwnd, mousepos.x, mousepos.y, 0,
                   PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_KEYBOARD);
   }
}

/*******************************************/
/* for docking support of frame windows.   */
/* used in WM_ADJUSTWINDOWPOS and such     */
/*******************************************/

BOOL dockWindow(PSWP pswptodock, PSWP pswpanchor, LONG margin)
{
   // docking when window to dock is under anchor window
   if(abs(pswptodock->y+pswptodock->cy - pswpanchor->y) < margin &&
      max(pswptodock->x, pswpanchor->x) < min(pswptodock->x+pswptodock->cx, pswpanchor->x+pswpanchor->cx))
   {
      pswptodock->y = pswpanchor->y - pswptodock->cy;
      // dock left
      if(abs(pswptodock->x - pswpanchor->x) < margin)
         pswptodock->x = pswpanchor->x;
      // dock right
      else if(abs( (pswptodock->x+pswptodock->cx) - (pswpanchor->x + pswpanchor->cx) ) < margin)
         pswptodock->x = pswpanchor->x+pswpanchor->cx-pswptodock->cx;
      return TRUE;
   }

   // docking when window to dock is over anchor window
   else if(abs(pswptodock->y - (pswpanchor->y + pswpanchor->cy) ) < margin &&
           max(pswptodock->x, pswpanchor->x) < min(pswptodock->x+pswptodock->cx, pswpanchor->x+pswpanchor->cx))
   {
      pswptodock->y = pswpanchor->y + pswpanchor->cy;
      // dock left
      if(abs(pswptodock->x - pswpanchor->x) < margin)
         pswptodock->x = pswpanchor->x;
      // dock right
      else if(abs( (pswptodock->x+pswptodock->cx) - (pswpanchor->x + pswpanchor->cx) ) < margin)
         pswptodock->x = pswpanchor->x+pswpanchor->cx-pswptodock->cx;
      return TRUE;
   }

   // docking when window to dock is left of anchor window
   else if(abs(pswptodock->x + pswptodock->cx - pswpanchor->x ) < margin &&
           max(pswptodock->y, pswpanchor->y) < min(pswptodock->y+pswptodock->cy, pswpanchor->y+pswpanchor->cy))
   {
      pswptodock->x = pswpanchor->x - pswptodock->cx;
      // dock bottom
      if(abs(pswptodock->y - pswpanchor->y) < margin)
         pswptodock->y = pswpanchor->y;
      // dock top
      else if(abs( (pswptodock->y+pswptodock->cy) - (pswpanchor->y + pswpanchor->cy) ) < margin)
         pswptodock->y = pswpanchor->y+pswpanchor->cy-pswptodock->cy;
      return TRUE;
   }

   // docking when window to dock is right of anchor window
   else if(abs( pswptodock->x - (pswpanchor->x + pswpanchor->cx) ) < margin &&
           max(pswptodock->y, pswpanchor->y) < min(pswptodock->y+pswptodock->cy, pswpanchor->y+pswpanchor->cy))
   {
      pswptodock->x = pswpanchor->x + pswpanchor->cx;
      // dock bottom
      if(abs(pswptodock->y - pswpanchor->y) < margin)
         pswptodock->y = pswpanchor->y;
      // dock top
      else if(abs( (pswptodock->y+pswptodock->cy) - (pswpanchor->y + pswpanchor->cy) ) < margin)
         pswptodock->y = pswpanchor->y+pswpanchor->cy-pswptodock->cy;
      return TRUE;
   }
   else
      return FALSE;
}


