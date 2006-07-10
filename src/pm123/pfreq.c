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


/* Code for the playlist manager */

#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include "utilfct.h"
#include "pfreq.h"
#include "format.h"
#include "decoder_plug.h"
#include "pm123.h"
#include "docking.h"

static HWND plman;
static HWND container;

HWND    pm_main_menu, pm_list_menu, pm_file_menu;
FILE     *debug, *mlist, *mtemp;
ULONG   totalFiles, totalBitrate, totalFreq, totalLength, totalSecs;

void pfreq_create_container(HWND, int);
PPMREC pm_add_entry(HWND, PPMREC, char *, char *, int, void *);
void Playlist_Populate(void *);

void change_sep(char *foo)
{
 while (*foo)
  {
   if (*foo == '/') *foo = '\\';
   foo++;
  }
}

void pm_destroy_children_and_self(HWND hwnd, PPMREC rec)
{
 PPMREC file, todelete[1];

 do
  {
   file = (PPMREC) WinSendMsg(hwnd,
                              CM_QUERYRECORD,
                              MPFROMP(rec),
                              MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));

   todelete[0] = file;
   WinSendMsg(hwnd,
              CM_REMOVERECORD,
              MPFROMP(&todelete),
              MPFROM2SHORT(1, CMA_FREE));
  } while (file != NULL);

 todelete[0] = rec;
 WinSendMsg(hwnd,
            CM_REMOVERECORD,
            MPFROMP(&todelete),
            MPFROM2SHORT(1, CMA_FREE));
}


int pm_check_duplicate(HWND hwnd, char *t1)
{
 PPMREC core;
 int    res = 0;

 core = (PPMREC) PVOIDFROMMR(WinSendMsg( container, CM_QUERYRECORD, NULL, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER)));
 while (core != NULL)
  {
   if (stricmp(t1, core->text) == 0) { res = 1; break; }
   core = (PPMREC) PVOIDFROMMR(WinSendMsg( container, CM_QUERYRECORD, MPFROMP(core), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER)));
  }
 return res;
}

PPMREC pm_add_entry(HWND hwnd, PPMREC parent, char *t1, char *t2, int type, void *data)
{
 PPMREC        file;
 RECORDINSERT  insert;

 /* Allocate a record in the container */
 file = (PPMREC) WinSendMsg(hwnd,
                            CM_ALLOCRECORD,
                            MPFROMLONG(sizeof(PMREC) - sizeof(RECORDCORE)),
                            MPFROMLONG(1));

 if (file == NULL) { DosBeep(500, 100); return 0; }

 /* It never hurts to zero the contest of the allocated structs */
 memset(file,     '\0', sizeof(PMREC));
 memset(&insert,  '\0', sizeof(RECORDINSERT));

 file->rc.cb              = sizeof(RECORDCORE);
 file->rc.flRecordAttr    = 0;
 file->rc.pszTree         = strdup(t1);
 file->type               = type;
 file->text               = strdup(t2);
 file->data               = data;

 insert.cb                = sizeof(RECORDINSERT);
 insert.pRecordOrder      = (PRECORDCORE) CMA_END;
 insert.pRecordParent     = (PRECORDCORE)parent;
 insert.fInvalidateRecord = TRUE;
 insert.zOrder            = CMA_TOP;
 insert.cRecordsInsert    = 1;

 WinSendMsg(hwnd,
            CM_INSERTRECORD,
            MPFROMP(file),
            MPFROMP(&insert));

 return file;
}

void Manager_Populate(void *scrap)
{
 HAB         hab;
 HMQ         hmq;
 char        buf[256], buf2[256];
 PTHREADINFO tr;

 hab = WinInitialize(0);
 hmq = WinCreateMsgQueue(hab, 0);

 sprintf(buf, "%sPM123.MGR", startpath);
 mlist = fopen(buf, "r");
 if (mlist != NULL)
  {
   while (!feof(mlist))
    {
    *buf = '\0';
    fgets(buf, sizeof(buf), mlist);
    blank_strip(buf);

    if(buf[0] != 0 && buf[0] != '#' && buf[0] != '>' && buf[0] != '<')
     {
      change_sep(buf);
      sfname(buf2,buf);

      tr = NULL;
      tr = malloc(sizeof(THREADINFO));
      tr->parent = pm_add_entry( container, NULL, buf2, buf, PM_TYPE_LIST, NULL);
      strcpy(tr->playlist, buf);

      _beginthread(Playlist_Populate, NULL, 0xFFFF, tr);
     }
    }
   fclose(mlist);
  }

 WinDestroyMsgQueue(hmq);
 WinTerminate(hab);

}

void Playlist_Populate(void *param)
{
 PTHREADINFO tr;
 PFREC       fr;
 FILE        *list;
 char        buf[256], buf2[256], *bla, *blo;
 HAB         hab;
 HMQ         hmq;
 PPMREC      core = 0;
 char        basepath[_MAX_PATH];
 char        fullname[_MAX_PATH];


 hab = WinInitialize(0);
 hmq = WinCreateMsgQueue(hab, 0);

 tr = (PTHREADINFO)param;

 list = fopen(tr->playlist, "r");
 if (!list) return;

 sdrivedir( basepath, tr->playlist );

 while (!feof(list)) {
  *buf = '\0';
  fgets(buf, sizeof(buf), list);
  blank_strip(buf);

  if( buf[0] != 0 && buf[0] != '#' && buf[0] != '>' && buf[0] != '<' )
  {
    if( is_file( buf ) && rel2abs( basepath, buf, fullname, sizeof(fullname))) {
      strcpy( buf, fullname );
    }

    change_sep(buf);
    sfnameext( buf2, buf );
    core = pm_add_entry( container, tr->parent, buf2, buf, PM_TYPE_FILE, NULL);
  }
  else
  {
     if (buf[0] == '>' && core != NULL)
      {
       fr = NULL;
       fr = malloc(sizeof(FREC)); blo = (buf + 1);
       memset(fr, '\0', sizeof(FREC));
       bla = strtok(blo, ",");
       if (bla) fr->bitrate = atoi(bla);
       bla = strtok(NULL, ",");
       if (bla) fr->freq    = atoi(bla);
       bla = strtok(NULL, ",");
       if (bla) fr->mode    = atoi(bla);
       bla = strtok(NULL, ",");
       if (bla) fr->length  = atol(bla);
       bla = strtok(NULL, ",");
       if (bla) fr->secs    = atoi(bla);
       core->data = fr;
      }
    }
 }

 free(tr);
 fclose( list );
 WinDestroyMsgQueue(hmq);
 WinTerminate(hab);
}

int idm_pm_add(HWND hwnd)
{
  FILEDLG     filedialog;
  int         dex = 0;
  char        filez[256], buf[256];
  PTHREADINFO tr;
  APSZ        types[] = {{ FDT_PLAYLIST }, { 0 }};

  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Add Playlist(s)";
  filedialog.hMod           = NULLHANDLE;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_PLAYLIST;

  strcpy( filedialog.szFullFile, cfg.listdir );
  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
   sdrivedir(cfg.listdir, filedialog.szFullFile);
   dex = 0;
   if (filedialog.ulFQFCount > 1)
     {
      strcpy(*filedialog.papszFQFilename[dex], filedialog.szFullFile);
      strcpy(filez, filedialog.szFullFile);
     }
    else
     {
      strcpy(filez, filedialog.szFullFile);
     }

    while (dex < filedialog.ulFQFCount)
     {
     if (filez == NULL)
      return 0;

      if (!pm_check_duplicate( container, filez))
       {
        sfname(buf,filez);
        tr = NULL;
        tr = malloc(sizeof(THREADINFO));
        tr->parent = pm_add_entry( container, NULL, buf, filez, PM_TYPE_LIST, NULL);
        strcpy(tr->playlist, filez);
        _beginthread(Playlist_Populate, NULL, 0xFFFF, tr);
        sprintf(buf, "%sPM123.MGR", startpath);
        mlist = fopen(buf, "a");
        if (mlist != NULL)
         {
          fprintf(mlist, "%s\n", filez);
          fclose(mlist);
         }
       }

     dex++;
     if (dex >= filedialog.ulFQFCount)
      {
        break;
      }
     else
      {
        strcpy(filez, *filedialog.papszFQFilename[dex]);
      }
   }
  }
 WinFreeFileDlgList(filedialog.papszFQFilename);
 return 1;
}

void pm_set_title(HWND hwnd, char *title)
{
 static CNRINFO cnrInfo = { 0 };

 if (cnrInfo.pszCnrTitle != NULL) free(cnrInfo.pszCnrTitle);
 cnrInfo.pszCnrTitle           = strdup(title);
 WinSendMsg(hwnd, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_CNRTITLE));
}

static MRESULT EXPENTRY
pm_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
 POINTL ptlMouse;
 PPMREC focus, zf;
 HWND   hwndMenu;
 PFREC  fr;
 char   buf[256], buf2[256], buf3[256];
 PNOTIFYRECORDEMPHASIS emphasis;

 switch (msg) {

  case WM_CONTROL:
      switch (SHORT2FROMMP(mp1))
       {
#if 0
        case CN_INITDRAG:
         drag = (PMYMPREC) (((PCNRDRAGINIT) mp2) -> pRecord);
         if (drag != NULL)
          {

          }
         break;
#endif

        case CN_HELP:
          amp_show_help( IDH_PM );
          return 0;

        case CN_EMPHASIS:
         emphasis = (PNOTIFYRECORDEMPHASIS)mp2;
         if (emphasis != NULL)
          {
           focus = (PPMREC) emphasis->pRecord;
           if (focus && focus->type == PM_TYPE_LIST)
            {
             sprintf(buf, "%s", focus->text);
             pm_set_title( container, buf);
            }
           if (focus && focus->type == PM_TYPE_FILE)
            {
             fr = (PFREC)focus->data;
             if (fr == NULL)
               sprintf(buf, "%s", focus->text);
              else
               {
                sprintf(buf, "%s, %02u:%02u, %u kB, %u kb/s, %.1f kHz, %s", focus->text, fr->secs / 60, fr->secs % 60, fr->length/1024, fr->bitrate, fr->freq/1000.0, modes(fr->mode));
               }
             pm_set_title( container, buf);
            }
          }
         break;
        case CN_CONTEXTMENU:
         focus = (PPMREC)mp2;

         WinQueryPointerPos(HWND_DESKTOP, &ptlMouse);

         if (focus != NULL)
          {
           if (focus->type == PM_TYPE_LIST)
            {
             if (pm_list_menu == 0)
               pm_list_menu = WinLoadMenu(HWND_OBJECT, 0, PM_LIST_MENU);
             hwndMenu = pm_list_menu;
            } else
             {
              if (pm_file_menu == 0)
                pm_file_menu = WinLoadMenu(HWND_OBJECT, 0, PM_FILE_MENU);
              hwndMenu = pm_file_menu;
             }
          } else
           {
            if (pm_main_menu == 0)
              pm_main_menu = WinLoadMenu(HWND_OBJECT, 0, PM_MAIN_MENU);
            hwndMenu = pm_main_menu;
           }

         WinPopupMenu(HWND_DESKTOP,
                      hwnd,
                      hwndMenu,
                      ptlMouse.x,
                      ptlMouse.y,
                      0,
                      PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
         break;
       }
      return 0;

  case WM_COMMAND:
      switch (COMMANDMSG(&msg) -> cmd)
       {
        case IDM_PM_L_CALC:
         totalFiles = totalSecs = totalLength = 0;

         zf = (PPMREC) PVOIDFROMMR(WinSendMsg( container, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));

         if (!zf) { DosBeep(500, 50); break; }

         focus  = (PPMREC) WinSendMsg( container,
                                       CM_QUERYRECORD,
                                       MPFROMP(zf),
                                       MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));

         while (focus != NULL) {
           if (focus->type == PM_TYPE_FILE && focus->data != NULL)
            {
             fr = (PFREC)focus->data;
             totalFiles++;
             totalSecs += fr->secs;
             totalLength += (fr->length / 1024);
            }

           focus  = (PPMREC) WinSendMsg( container,
                                         CM_QUERYRECORD,
                                         MPFROMP(focus),
                                         MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
         }

         if (totalFiles > 0)
          {
           sprintf(buf, "Total %lu files, %lu kB, %lud %luh %lum %lus",
                  totalFiles,
                  totalLength,
                  ((ULONG)totalSecs / 86400),
                  ((ULONG)((totalSecs % 86400) / 3600)),
                  (((ULONG)totalSecs % 3600) / 60),
                  ((ULONG)totalSecs % 60));

           pm_set_title( container, buf);
          }
          else
           pm_set_title( container, "No statistics available for this playlist.");
         break;
        case IDM_PM_CALC:
         totalFiles = totalSecs = totalLength = 0;

         zf  = (PPMREC) WinSendMsg( container,
                                    CM_QUERYRECORD,
                                    NULL,
                                    MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

         while (zf != NULL) {
           focus  = (PPMREC) WinSendMsg( container,
                                         CM_QUERYRECORD,
                                         MPFROMP(zf),
                                         MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));

           while (focus != NULL)
            {
             if (focus->type == PM_TYPE_FILE && focus->data != NULL)
              {
               fr = (PFREC)focus->data;
               totalFiles++;
               totalSecs += fr->secs;
               totalLength += (fr->length / 1024);
              }
             focus = (PPMREC) WinSendMsg( container,
                                          CM_QUERYRECORD,
                                          MPFROMP(focus),
                                          MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
            }
           zf = (PPMREC) WinSendMsg( container,
                                     CM_QUERYRECORD,
                                     MPFROMP(zf),
                                     MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
         }
         if (totalFiles > 0)
          {
           sprintf(buf, "Total %lu files, %lu kB, %lud %luh %lum %lus",
                  totalFiles,
                  totalLength,
                  ((ULONG)totalSecs / 86400),
                  ((ULONG)((totalSecs % 86400) / 3600)),
                  (((ULONG)totalSecs % 3600) / 60),
                  ((ULONG)totalSecs % 60));

           pm_set_title( container, buf);
          }
          else
           pm_set_title( container, "No statistics available.");
         break;
        case IDM_PM_L_DELETE:
         focus = (PPMREC) PVOIDFROMMR(WinSendMsg( container, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
         if (focus != NULL)
          {
           if (focus->type == PM_TYPE_LIST)
            {
             strcpy(buf2, focus->text);
             remove(buf2);
            }
          }
         WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(IDM_PM_L_REMOVE), 0);
         break;
        case IDM_PM_L_REMOVE:
         focus = (PPMREC) PVOIDFROMMR(WinSendMsg( container, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
         if (focus != NULL)
          {
           if (focus->type == PM_TYPE_LIST)
            {
             strcpy(buf2, focus->text);
             pm_destroy_children_and_self( container, focus);

             sprintf(buf, "%sPM123.MGR", startpath);
             mlist = fopen(buf, "r");
             if (mlist != NULL)
              {
               sprintf(buf3, "%sPM123MGR.$$$", startpath);
               mtemp = fopen(buf3, "w");
               if (mtemp == NULL)
                {
                 fclose(mlist);
                 WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, "Couldn't open temp file PM123MGR.$$$. Are you out of hard disk space?", "Error", 0, MB_ERROR | MB_OK);
                 return ((MRESULT)0);
                }
               while (!feof(mlist))
                {
                 fgets(buf3, sizeof(buf3), mlist);
                 blank_strip(buf3);
                 if (stricmp(buf2, buf3) != 0) fprintf(mtemp, "%s\n", buf3);
                }
               fclose(mlist);
               fclose(mtemp);
               remove(buf);
               sprintf(buf3, "%sPM123MGR.$$$", startpath);
               rename(buf3, buf);
               WinSendMsg( container, CM_INVALIDATERECORD, MPFROMP(NULL), MPFROM2SHORT(0, 0));
              }
            }
          }
         break;
        case IDM_PM_L_LOAD:
         focus = (PPMREC) PVOIDFROMMR(WinSendMsg( container, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
         if (focus != NULL && focus->type == PM_TYPE_LIST)
            pl_load( focus->text, PL_LOAD_CLEAR );

         break;
        case IDM_PM_F_LOAD:
         focus = (PPMREC) PVOIDFROMMR(WinSendMsg( container, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
         if (focus != NULL)
          {
           if (focus->type == PM_TYPE_FILE)
            amp_load_singlefile( focus->text, 0 );
          }
         break;
        case IDM_PM_ADD:
         idm_pm_add(hwnd);
         break;
       }
      return 0;

   case WM_ERASEBACKGROUND:
      return FALSE;

    case WM_INITDLG:
    {
      HPOINTER hicon;
      LONG     color;

      pfreq_create_container( hwnd, FID_CLIENT );
      _beginthread( Manager_Populate, NULL, 0xFFFF, NULL );

      hicon = WinLoadPointer( HWND_DESKTOP, 0, ICO_MAIN );
      WinSendMsg( hwnd, WM_SETICON, (MPARAM)hicon, 0 );
      do_warpsans( hwnd );

      if( !rest_window_pos( hwnd, 0 ))
      {
        color = CLR_GREEN;
        WinSetPresParam( container, PP_FOREGROUNDCOLORINDEX,
                         sizeof(color), &color);
        color = CLR_BLACK;
        WinSetPresParam( container, PP_BACKGROUNDCOLORINDEX,
                         sizeof(color), &color);
      }

      dk_add_window ( hwnd, 0 );
      return 0;
    }

    case WM_SYSCOMMAND:
      if( SHORT1FROMMP(mp1) == SC_CLOSE ) {
        pm_show( FALSE );
        return 0;
      }
      break;

    case WM_WINDOWPOSCHANGED:
    {
      SWP* pswp = PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW ) {
        cfg.show_plman = TRUE;
      }
      if( pswp[0].fl & SWP_HIDE ) {
        cfg.show_plman = FALSE;
      }
      break;
    }
 }

 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

void pfreq_create_container( HWND hwndParent, int id )
{
 CNRINFO cnrInfo;

 container = WinCreateWindow( hwndParent,
                              WC_CONTAINER,
                              "Playlist Manager",
                              WS_VISIBLE | CCS_SINGLESEL,
                              0, 0, 0, 0,
                              hwndParent,
                              HWND_TOP,
                              id,
                              NULL,
                              NULL );

 if (!container) DosBeep(500, 100);

 cnrInfo.flWindowAttr = CV_TREE | CV_TEXT | CA_TREELINE | CA_CONTAINERTITLE | CA_TITLELEFT | CA_TITLESEPARATOR | CA_TITLEREADONLY;
 cnrInfo.hbmExpanded  = NULLHANDLE;
 cnrInfo.hbmCollapsed = NULLHANDLE;
 cnrInfo.cxTreeIndent = 8;
 cnrInfo.pszCnrTitle  = "No playlist selected. Right click for menu.";

 WinSendMsg( container, CM_SETCNRINFO, MPFROMP( &cnrInfo ),
             MPFROMLONG( CMA_FLWINDOWATTR | CMA_CXTREEINDENT | CMA_CNRTITLE ));
}

/* Sets the visibility state of the playlist manager presentation window. */
void
pm_show( BOOL show )
{
  HSWITCH hswitch = WinQuerySwitchHandle( plman, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( plman, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( plman, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}

/* Creates the playlist manager presentation window. */
HWND
pm_create( void )
{
  plman = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                      pm_dlg_proc, NULLHANDLE, DLG_PM, NULL );

  pm_show( cfg.show_plman );
  return plman;
}

/* Destroys the playlist manager presentation window. */
void
pm_destroy( void )
{
  save_window_pos ( plman, 0 );
  WinDestroyWindow( plman );
}


