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

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utilfct.h"

HAB hab = 0;
HMQ hmq = 0;

HWND hwndClient, hwndFrame;

#define DRIVES_ICON   27
#define FOLDER_ICON   26
#define FLOPPY_ICON   32
#define CD_ICON       19
#define NETDRIVE_ICON 16
#define HD_ICON       13
#define VDISK_ICON    76

typedef struct _DIR_BROWSE_RECORD
{
   MINIRECORDCORE recordCore;
   char name[CCHMAXPATH];
   BOOL loaded;
   struct _DIR_BROWSE_RECORD *path_child; // used by build_path()
} DIR_BROWSE_RECORD;

typedef struct
{
   HWND hContainer;
   HMODULE hPMWP;
   POINTL csize[3];
   char *path;
   int size;
} DIR_BROWSE_STRUCT;


void load_roots(DIR_BROWSE_STRUCT *d)
{
   int i;
   ULONG currentDrive, allDrives;
   char driveLetter[3];
   DIR_BROWSE_RECORD *newRecord;
   MINIRECORDCORE *dummyRecord;
   RECORDINSERT recordInsert = {sizeof(RECORDINSERT), (RECORDCORE *) CMA_END, NULL, TRUE, CMA_TOP, 1};

   DosError(FERR_DISABLEHARDERR);
   DosQueryCurrentDisk(&currentDrive,&allDrives);
   for(i = 0; i < 26; i++)
      if((allDrives >> i) & 1)
      {
         driveLetter[0] = (char) ('A' + i);
         driveLetter[1] = ':';
         driveLetter[2] = 0;

         newRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_ALLOCRECORD,
                    MPFROMLONG(sizeof(DIR_BROWSE_RECORD)-sizeof(MINIRECORDCORE)),
                    MPFROMSHORT(1)));

         strcpy(newRecord->name,driveLetter);
         newRecord->recordCore.pszIcon = newRecord->name;
         newRecord->recordCore.hptrIcon = WinLoadPointer(HWND_DESKTOP,d->hPMWP, DRIVES_ICON);

         newRecord->loaded = FALSE;

         recordInsert.pRecordParent = NULL;
         WinSendMsg(d->hContainer,CM_INSERTRECORD, MPFROMP(newRecord), MPFROMP(&recordInsert));


         /* inserting temporary dummy record to make + appear */
         dummyRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_ALLOCRECORD,0,MPFROMSHORT(1)));
         dummyRecord->pszIcon = "dummy";

         recordInsert.pRecordParent = (RECORDCORE *) newRecord;
         WinSendMsg(d->hContainer,CM_INSERTRECORD, MPFROMP(dummyRecord), MPFROMP(&recordInsert));
      }

   DosError(FERR_ENABLEHARDERR);
}

void remove_dummy(DIR_BROWSE_STRUCT *d, DIR_BROWSE_RECORD *parent)
{
   MINIRECORDCORE *dummyRecord[1];

   dummyRecord[0] = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                 MPFROMP(parent),MPFROM2SHORT(CMA_FIRSTCHILD,CMA_ITEMORDER)));

   if(dummyRecord[0] != NULL && (int) dummyRecord[0] != -1)
      WinSendMsg(d->hContainer,CM_REMOVERECORD,
      MPFROMP(dummyRecord),MPFROM2SHORT(1, CMA_FREE | CMA_INVALIDATE));
}

BOOL build_path(DIR_BROWSE_STRUCT *d, DIR_BROWSE_RECORD *record, char *path, int size)
{
   DIR_BROWSE_RECORD *prevParent = record;
   DIR_BROWSE_RECORD *nextParent = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                                   MPFROMP(record),MPFROM2SHORT(CMA_PARENT,CMA_ITEMORDER)));

   prevParent->path_child = NULL;

   while(nextParent != NULL)
   {
      nextParent->path_child = prevParent;

      prevParent = nextParent;
      nextParent = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                   MPFROMP(prevParent),MPFROM2SHORT(CMA_PARENT,CMA_ITEMORDER)));

      if((int)nextParent == -1)
         return FALSE;
   }

   path[size-1] = 0;
   strncpy(path,prevParent->name,size-1);
   while(prevParent->path_child != NULL)
   {
      strncat(path,"\\",size-strlen(path)-1);
      strncat(path,prevParent->path_child->name, size-strlen(path)-1);
      prevParent = prevParent->path_child;
   }

   return TRUE;
}


/* recurseOne is used to load a second level of directories to + shows up in the container */
BOOL load_children(DIR_BROWSE_STRUCT *d, DIR_BROWSE_RECORD *parent, BOOL recurseOne)
{
   DIR_BROWSE_RECORD *grandParent;
   char path[CCHMAXPATH];
   HDIR hdir;
   FILEFINDBUF3 findbuf;
   ULONG findCount;
   ULONG rc;
   DIR_BROWSE_RECORD *newRecord;
   RECORDINSERT recordInsert = {sizeof(RECORDINSERT), (RECORDCORE *) CMA_END,
                                (RECORDCORE *) parent, TRUE, CMA_TOP, 1};

   /* if already loaded, make sure all child are also loaded */
   if(parent->loaded)
   {
      if(recurseOne)
      {
         newRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                     MPFROMP(parent),MPFROM2SHORT(CMA_FIRSTCHILD,CMA_ITEMORDER)));

         if((int)newRecord == -1)
            return FALSE;

         while(newRecord != NULL)
         {
            load_children(d,newRecord,FALSE);

            newRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                        MPFROMP(newRecord),MPFROM2SHORT(CMA_NEXT,CMA_ITEMORDER)));

            if((int)newRecord == -1)
               return FALSE;
         }
      }

      return TRUE;
   }

   /* otherwize load it */
   grandParent = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,MPFROMP(parent),
                             MPFROM2SHORT(CMA_PARENT,CMA_ITEMORDER)));

   if((int)grandParent == -1) // error
      return FALSE;

   /* remove root dummy record */
   if(grandParent == NULL)
      remove_dummy(d, parent);

   if(!build_path(d, parent, path, sizeof(path)))
      return FALSE;

   strcat(path,"\\*");

   findCount = 1;
   hdir = HDIR_CREATE;
   rc = DosFindFirst(path,&hdir,MUST_HAVE_DIRECTORY | FILE_DIRECTORY,
                     &findbuf,sizeof(findbuf),&findCount,FIL_STANDARD);

   if(rc != NO_ERROR && rc != ERROR_NO_MORE_FILES)
      return FALSE;

   while(rc != ERROR_NO_MORE_FILES)
   {
      if(strcmp(findbuf.achName,".") != 0 && strcmp(findbuf.achName,"..") != 0)
      {
         /* inserting child record */
         newRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_ALLOCRECORD,
                     MPFROMLONG(sizeof(DIR_BROWSE_RECORD)-sizeof(MINIRECORDCORE)),
                     MPFROMSHORT(1)));

         strcpy(newRecord->name,findbuf.achName);
         newRecord->recordCore.pszIcon = newRecord->name;
         newRecord->recordCore.hptrIcon = WinLoadPointer(HWND_DESKTOP,d->hPMWP, FOLDER_ICON);

         newRecord->loaded = FALSE;

         WinSendMsg(d->hContainer,CM_INSERTRECORD, MPFROMP(newRecord), MPFROMP(&recordInsert));

         if(recurseOne)
            load_children(d, newRecord, FALSE);
      }

      /* finding next directory */
      findCount = 1;
      rc = DosFindNext(hdir,&findbuf,sizeof(findbuf),&findCount);

      if(rc != NO_ERROR && rc != ERROR_NO_MORE_FILES)
         return FALSE;
   }

   DosFindClose(hdir);
   parent->loaded = TRUE;

   return TRUE;
}

DIR_BROWSE_RECORD *hunt_path(DIR_BROWSE_STRUCT *d, char *path)
{
   DIR_BROWSE_RECORD *curRecord = NULL, *prevRecord = NULL;
   char find_name[CCHMAXPATH];
   char *next_slash;
   char *path_pos = path;

   // -1 makes 0 wrap to 0xFFFFFFF, so if it's not found it
   // gets biggest value since strchr will report 0 in that case, nice huh?
   next_slash = strchr(path_pos,'\\')-1 < strchr(path_pos,'/')-1 ?
                strchr(path_pos,'\\') : strchr(path_pos,'/');

   if(next_slash == NULL)
      strcpy(find_name,path_pos);
   else
   {
      strncpy(find_name,path_pos,next_slash-path_pos);
      find_name[next_slash-path_pos] = 0;
   }

   path_pos += strlen(find_name)+1;

   curRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                           MPFROMP(NULL),MPFROM2SHORT(CMA_FIRST,CMA_ITEMORDER)));

   while(curRecord != NULL)
   {
      if(stricmp(find_name,curRecord->name) == 0)
      {
         // we found our full path, break
         if(path_pos[-1] == 0 || path_pos[0] == 0)
            break;

         // will load folders
         WinSendMsg(d->hContainer,CM_EXPANDTREE,MPFROMP(curRecord),0);

         prevRecord = curRecord;
         curRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                                 MPFROMP(curRecord),MPFROM2SHORT(CMA_FIRSTCHILD,CMA_ITEMORDER)));

         // full path was not found, return last matching record
         if(curRecord == NULL)
         {
            curRecord = prevRecord;
            break;
         }

         next_slash = strchr(path_pos,'\\')-1 < strchr(path_pos,'/')-1 ?
                      strchr(path_pos,'\\') : strchr(path_pos,'/');

         if(next_slash == NULL)
            strcpy(find_name,path_pos);
         else
         {
            strncpy(find_name,path_pos,next_slash-path_pos);
            find_name[next_slash-path_pos] = 0;
         }

         path_pos += strlen(find_name)+1;
      }
      else
      {
         curRecord = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORD,
                                 MPFROMP(curRecord),MPFROM2SHORT(CMA_NEXT,CMA_ITEMORDER)));
      }
   }

   // if full path was not found, return last matching record
   if(curRecord == NULL)
      curRecord = prevRecord;
   return curRecord;
}

BOOL run_dir_browser(HWND hwnd, POINTL pos, char *text, char *path, int size)
{
   BOOL rc;
   POINTL wsize = {200, 150};

   CNRINFO cnrInfo;
   ULONG color = SYSCLR_DIALOGBACKGROUND;

   DIR_BROWSE_STRUCT *d;
   DIR_BROWSE_RECORD *record;


   WinMapDlgPoints(HWND_DESKTOP, &wsize, 1, TRUE);

   WinSetPresParam(hwnd, PP_BACKGROUNDCOLORINDEX, sizeof(color), &color);

   d = (DIR_BROWSE_STRUCT *) malloc(sizeof(*d));
   memset(d,0,sizeof(*d));
   WinSetWindowPtr(hwnd, 0, d);
   DosQueryModuleHandle("PMWP",&d->hPMWP);
   d->path = path;
   d->size = size;

   d->csize[0].x = 50;
   d->csize[0].y = 13;
   d->csize[1].x = 2;
   d->csize[1].y = 2;
   d->csize[2].x = 100;
   d->csize[2].y = 8;
   WinMapDlgPoints(HWND_DESKTOP, d->csize, 3, TRUE);


   WinCreateWindow(hwnd, WC_STATIC, text, SS_TEXT | DT_LEFT | WS_VISIBLE | SS_AUTOSIZE | WS_GROUP,
                   d->csize[1].x, d->csize[1].y, -1, d->csize[2].y, hwnd, HWND_BOTTOM, 5, NULL, NULL);

   d->hContainer = WinCreateWindow(hwnd, WC_CONTAINER, NULL,
                CCS_AUTOPOSITION | CCS_READONLY | CCS_SINGLESEL |
                CCS_MINIRECORDCORE | CCS_MINIICONS | CCS_READONLY |
                WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYNCPAINT |
                WS_TABSTOP | WS_GROUP | WS_VISIBLE,
                0, 0, 0, 0, hwnd, HWND_BOTTOM, 42, NULL, NULL);

   WinCreateWindow(hwnd, WC_BUTTON, "~Ok", BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | WS_GROUP,
                   d->csize[1].x, d->csize[1].y, d->csize[0].x, d->csize[0].y,
                   hwnd, HWND_BOTTOM, DID_OK, NULL, NULL);

   WinCreateWindow(hwnd, WC_BUTTON, "~Cancel", BS_PUSHBUTTON | WS_VISIBLE,
                   d->csize[1].x*2+d->csize[0].x, d->csize[1].y, d->csize[0].x, d->csize[0].y,
                   hwnd, HWND_BOTTOM, DID_CANCEL, NULL, NULL);

   cnrInfo.cb = sizeof(cnrInfo);
   cnrInfo.flWindowAttr = CV_TREE | CV_ICON | CV_MINI | CA_TREELINE;
   WinSendMsg(d->hContainer,CM_SETCNRINFO,MPFROMP(&cnrInfo),MPFROMLONG(CMA_FLWINDOWATTR));
   do_warpsans(d->hContainer);

   load_roots(d);

   WinSetWindowPos(hwnd, HWND_TOP, pos.x, pos.y, wsize.x, wsize.y,
                   SWP_SIZE | SWP_MOVE | SWP_SHOW);

   record = hunt_path(d,path);
   if(record != NULL)
   {
      RECTL recordPos;
      QUERYRECORDRECT queryRecordRect = {sizeof(QUERYRECORDRECT),(RECORDCORE *) record,TRUE,CMA_TEXT};

      WinSendMsg(d->hContainer,CM_SETRECORDEMPHASIS,MPFROMP(record),MPFROM2SHORT(TRUE,CRA_SELECTED));
      WinSendMsg(d->hContainer,CM_QUERYRECORDRECT,MPFROMP(&recordPos),MPFROMP(&queryRecordRect));
      WinSendMsg(d->hContainer,CM_SCROLLWINDOW,MPFROMSHORT(CMA_VERTICAL),MPFROMLONG(-recordPos.yBottom));
   }

   WinSetFocus(HWND_DESKTOP,d->hContainer);

   rc = WinProcessDlg(hwnd);
   WinDestroyWindow(hwnd);

   return rc;
}

PFNWP wpFrame;

MRESULT EXPENTRY wpDirBrowse(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   DIR_BROWSE_STRUCT *d = WinQueryWindowPtr(hwnd, 0);
   HWND hContainer = WinWindowFromID(hwnd,42);
   HWND hText = WinWindowFromID(hwnd,5);
   HWND hOK = WinWindowFromID(hwnd,DID_OK);

   switch(msg)
   {
      case WM_WINDOWPOSCHANGED:
      {
         SWP *swp = PVOIDFROMMP(mp1);
         if(swp->fl & SWP_SIZE)
         {
            SWP swpbutton = {0};
            SWP swptext = {0};
            RECTL client = {swp->x, swp->y, swp->x+swp->cx, swp->y+swp->cy};
            WinCalcFrameRect(hwnd,&client,TRUE);
            WinMapWindowPoints(HWND_DESKTOP,hwnd,(POINTL*)&client,2);

            WinQueryWindowPos(hOK,&swpbutton);
            WinQueryWindowPos(hText,&swptext);

            WinEnableWindowUpdate(hwnd,FALSE);

            wpFrame(hwnd,msg,mp1,mp2);

            WinSetWindowPos( hText, 0,
                             client.xLeft+d->csize[1].x, client.yTop - swptext.cy - 2,
                             0, 0,
                             SWP_MOVE);

            WinSetWindowPos( hContainer, 0,
                             client.xLeft, client.yBottom+swpbutton.cy+10,
                             client.xRight-client.xLeft, (client.yTop - client.yBottom) - (swpbutton.cy+10) - (swptext.cy+4),
                             SWP_SIZE | SWP_MOVE);

            WinEnableWindowUpdate(hwnd,TRUE);
            return 0;
         }

         break;
      }

      case WM_CLOSE:
         WinDismissDlg(hwnd,FALSE);
         return 0;

      case WM_DESTROY:
         free(d);
         break;

      case WM_CONTROL:
         switch(SHORT2FROMMP(mp1))
         {
            case CN_EXPANDTREE:
            {
               load_children(d, (DIR_BROWSE_RECORD *) mp2, TRUE);
               return 0;
            }
         }
         break;

      case WM_COMMAND:
      {
         CMDMSG *cmdmsg = COMMANDMSG(&msg);
         switch(cmdmsg->cmd)
         {
            case DID_OK:
            {
               DIR_BROWSE_RECORD *selected = PVOIDFROMMR(WinSendMsg(d->hContainer,CM_QUERYRECORDEMPHASIS,
                                             MPFROMLONG(CMA_FIRST),MPFROMSHORT(CRA_SELECTED)));
               WinDismissDlg(hwnd,build_path(d,selected,d->path,d->size));
               return 0;
            }

            case DID_CANCEL:
               WinDismissDlg(hwnd,FALSE);
               return 0;
         }
      }

   }
   return wpFrame(hwnd,msg,mp1,mp2);
}

BOOL open_dir_browser(HWND hwnd, POINTL pos, char *desc, char *path, int size)
{
   ULONG frameFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_MINBUTTON | FCF_MAXBUTTON |
                      FCF_SIZEBORDER  |/* FCF_MENU | FCF_ACCELTABLE | FCF_ICON | */
                      /* FCF_SHELLPOSITION | */ FCF_TASKLIST /*| FCF_DLGBORDER*/;

   HWND hwndFrame;
   FRAMECDATA fcdata;

   fcdata.cb            = sizeof(FRAMECDATA);
   fcdata.flCreateFlags = frameFlags;
   fcdata.hmodResources = (HMODULE) NULL;
   fcdata.idResources   = 1;

   hwndFrame = WinCreateWindow(
               HWND_DESKTOP,    /* Frame-window parent        */
               WC_FRAME,        /* Frame-window class         */
               "Directory Browser",   /* Window title               */
               WS_CLIPCHILDREN, /* Initially invisible        */
               0,0,0,0,         /* Size and position = 0      */
               hwnd,            /* No owner                   */
               HWND_TOP,        /* Top z-order position       */
               0,               /* Frame-window ID            */
               &fcdata,         /* Pointer to class data      */
               NULL);           /* No presentation parameters */

   wpFrame = WinSubclassWindow(hwndFrame,wpDirBrowse);

   return run_dir_browser(hwndFrame, pos, desc, path, size);
}

#if 0
int main(int argc, char *argv[])
{
   ULONG frameFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_MINBUTTON | FCF_MAXBUTTON |
                      FCF_SIZEBORDER  |/* FCF_MENU | FCF_ACCELTABLE | FCF_ICON | */
                      /* FCF_SHELLPOSITION | */ FCF_TASKLIST /*| FCF_DLGBORDER*/;

   QMSG qmsg;

   FRAMECDATA fcdata;

   setvbuf(stdout,NULL,_IONBF, 0);

   fcdata.cb            = sizeof(FRAMECDATA);
   fcdata.flCreateFlags = frameFlags;
   fcdata.hmodResources = (HMODULE) NULL;
   fcdata.idResources   = 1;

   hab = WinInitialize(0);
   if(hab)
     hmq = WinCreateMsgQueue(hab, 0);
   if(!hmq)
     return 1;

   hwndFrame = WinCreateWindow(
               HWND_DESKTOP,    /* Frame-window parent        */
               WC_FRAME,        /* Frame-window class         */
               "Directory Browser",   /* Window title               */
               WS_CLIPCHILDREN, /* Initially invisible        */
               0,0,0,0,         /* Size and position = 0      */
               0L,              /* No owner                   */
               HWND_TOP,        /* Top z-order position       */
               0,               /* Frame-window ID            */
               &fcdata,         /* Pointer to class data      */
               NULL);           /* No presentation parameters */

   wpFrame = WinSubclassWindow(hwndFrame,wpDirBrowse);

   run_dir_browser(hwndFrame, "some ver very long text", argv[1]);

   while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
      WinDispatchMsg( hab, &qmsg );

   WinDestroyMsgQueue( hmq );
   WinTerminate( hab );

   return 0;
}
#endif
