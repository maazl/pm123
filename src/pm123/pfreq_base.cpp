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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include <os2.h>
#include "pm123.h"
#include <utilfct.h>
#include "pfreq.h"
#include "pfreq_base.h"
#include "docking.h"
#include "iniman.h"
#include "playable.h"

#include <stdarg.h>
#include <snprintf.h>
#include <cpp/smartptr.h>
#include "url.h"

#include <debuglog.h>


// Default instance of playlist manager window, representing PM123.LST in the program folder.
static sco_ptr<PlaylistManager> DefaultPM;

#ifdef DEBUG
xstring PlaylistManager::Record::DebugName() const
{ if (IsRemoved())
    return xstring::sprintf("%p{<removed>}", this);
  return xstring::sprintf("%p{%p{%s}}", this, Content, Content->GetPlayable().GetURL().getObjName().cdata());
} 
xstring PlaylistManager::Record::DebugName(const Record* rec)
{ static const xstring nullstring = "<NULL>";
  if (!rec)
    return nullstring;
  return rec->DebugName();
}

xstring PlaylistManager::DebugName() const
{ return Content->GetURL().getObjName();
} 
#endif


/****************************************************************************
*
*  class PlaylistManager
*
****************************************************************************/

HPOINTER PlaylistManager::IcoSong[3];
HPOINTER PlaylistManager::IcoPlaylist[3][3];
HPOINTER PlaylistManager::IcoEmptyPlaylist;
HPOINTER PlaylistManager::IcoInvalid;

void PlaylistManager::InitIcons()
{ IcoSong    [IC_Normal]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3);
  IcoSong    [IC_Used]                  = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3USED);
  IcoSong    [IC_Play]                  = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3PLAY);
  IcoPlaylist[IC_Normal][ICP_Closed]    = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLCLOSE);
  IcoPlaylist[IC_Normal][ICP_Open]      = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLOPEN);
  IcoPlaylist[IC_Normal][ICP_Recursive] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLRECURSIVE);
  IcoPlaylist[IC_Used]  [ICP_Closed]    = IcoPlaylist[IC_Normal][ICP_Closed];
  IcoPlaylist[IC_Used]  [ICP_Open]      = IcoPlaylist[IC_Normal][ICP_Open];
  IcoPlaylist[IC_Used]  [ICP_Recursive] = IcoPlaylist[IC_Normal][ICP_Recursive];
  IcoPlaylist[IC_Play]  [ICP_Closed]    = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLCLOSEPLAY);
  IcoPlaylist[IC_Play]  [ICP_Open]      = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLOPENPLAY);
  IcoPlaylist[IC_Play]  [ICP_Recursive] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLRECURSIVEPLAY);
  IcoEmptyPlaylist                      = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLEMPTY);
  IcoInvalid                            = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3INVLD);
}

sorted_vector<PlaylistManager, char> PlaylistManager::RPInst(8);
Mutex PlaylistManager::RPMutex;

void PlaylistManager::Init()
{ // currently a no-op
}

void PlaylistManager::UnInit()
{ DEBUGLOG(("PlaylistManager::UnInit()\n"));
  // Free stored instances.
  // The instances deregister itself from the repository.
  // Starting at the end avoids the memcpy calls for shrinking the vector.
  while (RPInst.size())
    delete RPInst[RPInst.size()-1];
}

PlaylistManager::PlaylistManager(const char* url, const char* alias)
: Content(Playable::GetByURL(url)),
  Alias(alias),
  HwndMgr(NULLHANDLE),
  HwndContainer(NULLHANDLE),
  MainMenu(NULLHANDLE),
  ListMenu(NULLHANDLE),
  FileMenu(NULLHANDLE),
  CmFocus(NULL),
  NoRefresh(false),
  RootInfoDelegate(Content->InfoChange, *this, &InfoChangeEvent, NULL)
{ DEBUGLOG(("PlaylistManager(%p)::PlaylistManager(%s)\n", this, url));
  static bool first = true;
  if (first)
  { InitIcons();
    first = false;
  }
  // These two ones are constant
  LoadWizzards[0] = &amp_file_wizzard;
  LoadWizzards[1] = &amp_url_wizzard;
  init_dlg_struct ids = { sizeof(init_dlg_struct), this };
  WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, DlgProcStub, NULLHANDLE, DLG_PM, &ids);
}

PlaylistManager::~PlaylistManager()
{ DEBUGLOG(("PlaylistManager(%p{%s})::~PlaylistManager()\n", this, DebugName().cdata()));
  // Deregister from repository automatically
  { Mutex::Lock lock(RPMutex);
    PlaylistManager* r = RPInst.erase(Content->GetURL());
    assert(r != NULL);
  }
  save_window_pos(HwndMgr, 0);
  WinDestroyWindow(HwndMgr);
  DEBUGLOG(("PlaylistManager::~PlaylistManager() - end\n"));
}

PlaylistManager* PlaylistManager::Get(const char* url, const char* alias)
{ DEBUGLOG(("PlaylistManager::Get(%s, %s)\n", url, alias ? alias : "<NULL>"));
  Mutex::Lock lock(RPMutex);
  PlaylistManager*& pp = RPInst.get(url);
  if (!pp)
    pp = new PlaylistManager(url, alias);
  return pp;
}

int PlaylistManager::CompareTo(const char* str) const
{ return stricmp(Content->GetURL(), str);
}

void PlaylistManager::SetVisible(bool show)
{ DEBUGLOG(("PlaylistManager(%p{%s})::SetVisible(%u)\n", this, DebugName().cdata(), show));
  HSWITCH hswitch = WinQuerySwitchHandle( HwndMgr, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( HwndMgr, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( HwndMgr, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}

void PlaylistManager::CreateContainer(int id)
{ DEBUGLOG(("PlaylistManager(%p{%s})::CreateContainer(%i)\n", this, DebugName().cdata(), id));
  #ifdef DEBUG
  HwndContainer = WinCreateWindow( HwndMgr, WC_CONTAINER, "Playlist Manager", WS_VISIBLE|CCS_SINGLESEL|CCS_MINIICONS|CCS_MINIRECORDCORE|CCS_VERIFYPOINTERS,
                               0, 0, 0, 0, HwndMgr, HWND_TOP, id, NULL, NULL);
  #else
  HwndContainer = WinCreateWindow( HwndMgr, WC_CONTAINER, "Playlist Manager", WS_VISIBLE|CCS_SINGLESEL|CCS_MINIICONS|CCS_MINIRECORDCORE,
                               0, 0, 0, 0, HwndMgr, HWND_TOP, id, NULL, NULL);
  #endif
  if (!HwndContainer)
    DEBUGLOG(("PlaylistManager::CreateContainer: failed to create HwndContainer, error = %lx\n", WinGetLastError(NULL)));

  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.flWindowAttr = CV_TREE|CV_NAME|CV_MINI|CA_TREELINE|CA_CONTAINERTITLE|CA_TITLELEFT|CA_TITLESEPARATOR|CA_TITLEREADONLY;
  cnrInfo.hbmExpanded  = NULLHANDLE;
  cnrInfo.hbmCollapsed = NULLHANDLE;
  cnrInfo.cxTreeIndent = WinQuerySysValue(HWND_DESKTOP, SV_CYMENU) +2;
  //cnrInfo.cyLineSpacing = 0;
  DEBUGLOG(("PlaylistManager::CreateContainer: %u\n", cnrInfo.slBitmapOrIcon.cx));
  cnrInfo.pszCnrTitle  = "No playlist selected. Right click for menu.";
  WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_FLWINDOWATTR|CMA_CXTREEINDENT|CMA_CNRTITLE|CMA_LINESPACING));
}

MRESULT PlaylistManager::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistManager(%p{%s})::DlgProc(%x, %x, %x)\n", this, DebugName().cdata(), msg, mp1, mp2));

  switch (msg)
  {case WM_INITDLG:
    { HPOINTER hicon;
      LONG     color;

      CreateContainer(FID_CLIENT);

      hicon = WinLoadPointer(HWND_DESKTOP, 0, ICO_MAIN);
      WinSendMsg(HwndMgr, WM_SETICON, (MPARAM)hicon, 0);
      do_warpsans(HwndMgr);

      if( !rest_window_pos(HwndMgr, 0))
      { color = CLR_GREEN;
        WinSetPresParam(HwndContainer, PP_FOREGROUNDCOLORINDEX, sizeof(color), &color);
        color = CLR_BLACK;
        WinSetPresParam(HwndContainer, PP_BACKGROUNDCOLORINDEX, sizeof(color), &color);
      }

      dk_add_window(HwndMgr, 0);
      
      // populate the root node
      RequestChildren(NULL);
      break;
    }

   case WM_DESTROY:
    // delete all records
    { DEBUGLOG(("PlaylistManager::DlgProc: WM_DESTROY\n"));
      RemoveChildren(NULL);
      // process outstanding UM_DELETERECORD messages before we quit to ensure that all records are back to the PM before we die.
      QMSG qmsg;
      while (WinPeekMsg(amp_player_hab(), &qmsg, HwndMgr, UM_DELETERECORD, UM_DELETERECORD, PM_REMOVE))
      { DEBUGLOG2(("PlaylistManager::DlgProc: WM_DESTROY: %x %x %x %x\n", qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2));
        DlgProc(qmsg.msg, qmsg.mp1, qmsg.mp2); // We take the short way here.
      }
      break;
    }

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
      { PNOTIFYRECORDEMPHASIS emphasis = (PNOTIFYRECORDEMPHASIS)mp2;
        if (emphasis->fEmphasisMask & CRA_CURSORED)
        { // Update title
          Record* focus = (Record*)emphasis->pRecord;
          if (focus != NULL && !focus->IsRemoved() && (focus->flRecordAttr & CRA_CURSORED))
          { EmFocus = &focus->Content->GetPlayable();
            if (EmFocus->EnsureInfoAsync(Playable::IF_Tech))
              WinPostMsg(HwndMgr, UM_UPDATEINFO, MPFROMP(&*EmFocus), 0);
          }
        }
      }
      return 0;
     
     case CN_EXPANDTREE:
      { Record* focus = (Record*)PVOIDFROMMP(mp2);
        DEBUGLOG(("PlaylistManager::DlgProc CN_EXPANDTREE %p\n", focus));
        PostRecordCommand(focus, RC_UPDATESTATUS);
        // iterate over children
        focus = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(focus), MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));
        while (focus != NULL && focus != (Record*)-1)
        { DEBUGLOG(("CM_QUERYRECORD: %s\n", Record::DebugName(focus).cdata()));
          if (!focus->Data->Updated)
            RequestChildren(focus);
          focus = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(focus), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
        }
      }  
      return 0;

     case CN_COLLAPSETREE:
      PostRecordCommand((Record*)PVOIDFROMMP(mp2), RC_UPDATESTATUS);
      return 0;

     case CN_CONTEXTMENU:
      { Record* focus = CmFocus = (Record*)mp2; // save focus for later processing of menu messages
        if (Record::IsRemoved(focus))
          return 0; // record removed
        if (focus)
          WinPostMsg(HwndContainer, CM_SETRECORDEMPHASIS, MPFROMP(focus), MPFROM2SHORT(TRUE, CRA_CURSORED)); 

        POINTL ptlMouse;
        WinQueryPointerPos(HWND_DESKTOP, &ptlMouse);
        // TODO: Mouse Position may not be reasonable, when the menu is invoked by keyboard.

        HWND   hwndMenu;
        if (focus == NULL)
        { if (MainMenu == NULLHANDLE)
          { MainMenu = WinLoadMenu(HWND_OBJECT, 0, PM_MAIN_MENU);
            mn_make_conditionalcascade( MainMenu, IDM_PM_APPEND, IDM_PM_APPFILE );
          }
          hwndMenu = MainMenu;
          mn_enable_item( hwndMenu, IDM_PM_APPEND, (Content->GetFlags() & Playable::Mutable) != 0 );
        } else if (focus->Content->GetPlayable().GetFlags() & Playable::Enumerable)
        { if (ListMenu == NULLHANDLE)
          { ListMenu = WinLoadMenu(HWND_OBJECT, 0, PM_LIST_MENU);
            mn_make_conditionalcascade( ListMenu, IDM_PM_APPEND, IDM_PM_APPFILE );
          }
          if (Record::IsRemoved(focus))
            return 0; // record could be removed at WinLoadMenu
          hwndMenu = ListMenu;
          mn_enable_item( hwndMenu, IDM_PM_APPEND, (focus->Content->GetPlayable().GetFlags() & Playable::Mutable) != 0 );
        } else
        { if (FileMenu == NULLHANDLE)
          { FileMenu = WinLoadMenu(HWND_OBJECT, 0, PM_FILE_MENU);
          }
          hwndMenu = FileMenu;
        }
        DEBUGLOG2(("PlaylistManager::DlgProc: Menu: %p %p %p\n", MainMenu, ListMenu, FileMenu));
        WinPopupMenu(HWND_DESKTOP, HwndMgr, hwndMenu, ptlMouse.x, ptlMouse.y, 0,
                     PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
      }
      return 0;
      
     case CN_BEGINEDIT:
      // TODO: normally we have to lock some functions here...
      return 0;

     case CN_REALLOCPSZ:
      { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
        Record* rec = (Record*)ed->pRecord;
        DEBUGLOG(("PlaylistManager::DlgProc CN_REALLOCPSZ %p{,%p->%p{%s},%u,} %p\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText, rec));
        *ed->ppszText = rec->Data->Text.raw_init(ed->cbText); // because of the string sharing we always have to initialize the instance
      }
      return MRFROMLONG(TRUE);

     case CN_ENDEDIT:
      { CNREDITDATA* ed = (CNREDITDATA*)PVOIDFROMMP(mp2);
        Record* rec = (Record*)ed->pRecord;
        DEBUGLOG(("PlaylistManager::DlgProc CN_ENDEDIT %p{,%p->%p{%s},%u,} %p\n", ed, ed->ppszText, *ed->ppszText, *ed->ppszText, ed->cbText, rec));
        if (rec->IsRemoved())
          return 0; // record removed
        NoRefresh = true;
        rec->Content->SetAlias(**ed->ppszText ? *ed->ppszText : NULL);
        NoRefresh = false;
      }
      return 0;
    } // switch (SHORT2FROMMP(mp1))
    break;

   case WM_INITMENU:
    switch (SHORT1FROMMP(mp1))
    {case IDM_PM_APPEND:
      // Populate context menu with plug-in specific stuff.
      { HWND mh = HWNDFROMMP(mp2);
        int i = 2;
        for (;;)
        { SHORT id = SHORT1FROMMR(WinSendMsg(mh, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0));
          if (id == MIT_ERROR)
            break;
          WinSendMsg( mh, MM_DELETEITEM, MPFROM2SHORT( id, FALSE ), 0 );
        }
        memset(LoadWizzards+2, 0, sizeof LoadWizzards - 2*sizeof *LoadWizzards ); // You never know...
        append_load_menu( mh, SHORT1FROMMP(mp1) == IDM_PM_APPOTHER,
          TRUE, LoadWizzards+2, sizeof LoadWizzards / sizeof *LoadWizzards - 2 );
      }
    }
    break;

   case WM_COMMAND:
    { SHORT cmd = SHORT1FROMMP(mp1);
      if (cmd >= IDM_PM_APPFILE && cmd < IDM_PM_APPFILE + sizeof LoadWizzards / sizeof *LoadWizzards)
      { DECODER_WIZZARD_FUNC func = LoadWizzards[cmd-IDM_PM_APPFILE];
        if (func != NULL)
          UserAdd(func, "Append%s", CmFocus); // focus from CN_CONTEXTMENU
        return 0;
      }
      switch (SHORT1FROMMP(mp1))
      {/*case IDM_PM_L_CALC:
         totalFiles = totalSecs = totalLength = 0;

         zf = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));

         if (!zf) { DosBeep(500, 50); break; }

         focus  = (PPMREC) WinSendMsg( HwndContainer,
                                       CM_QUERYRECORD,
                                       MPFROMP(zf),
                                       MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));

         while (focus != NULL)
         { if (focus->type == PM_TYPE_FILE && focus->data != NULL)
           { fr = (PFREC)focus->data;
             totalFiles++;
             totalSecs += fr->secs;
             totalLength += (fr->length / 1024);
           }

           focus  = (PPMREC) WinSendMsg( HwndContainer,
                                         CM_QUERYRECORD,
                                         MPFROMP(focus),
                                         MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
         }

         if (totalFiles > 0)
         { sprintf(buf, "Total %lu files, %lu kB, %lud %luh %lum %lus",
                  totalFiles,
                  totalLength,
                  ((ULONG)totalSecs / 86400),
                  ((ULONG)((totalSecs % 86400) / 3600)),
                  (((ULONG)totalSecs % 3600) / 60),
                  ((ULONG)totalSecs % 60));

           pm_set_title( HwndContainer, buf);
         } else
           pm_set_title( HwndContainer, "No statistics available for this playlist.");
         break;*/

  /*      case IDM_PM_CALC:
         totalFiles = totalSecs = totalLength = 0;

         zf  = (PPMREC) WinSendMsg( HwndContainer,
                                    CM_QUERYRECORD,
                                    NULL,
                                    MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

         while (zf != NULL)
         { focus  = (PPMREC) WinSendMsg( HwndContainer,
                                         CM_QUERYRECORD,
                                         MPFROMP(zf),
                                         MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));

           while (focus != NULL)
           { if (focus->type == PM_TYPE_FILE && focus->data != NULL)
             { fr = (PFREC)focus->data;
               totalFiles++;
               totalSecs += fr->secs;
               totalLength += (fr->length / 1024);
             }
             focus = (PPMREC) WinSendMsg( HwndContainer,
                                          CM_QUERYRECORD,
                                          MPFROMP(focus),
                                          MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
           }
           zf = (PPMREC) WinSendMsg( HwndContainer,
                                     CM_QUERYRECORD,
                                     MPFROMP(zf),
                                     MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
         }
         if (totalFiles > 0)
         { sprintf(buf, "Total %lu files, %lu kB, %lud %luh %lum %lus",
                  totalFiles,
                  totalLength,
                  ((ULONG)totalSecs / 86400),
                  ((ULONG)((totalSecs % 86400) / 3600)),
                  (((ULONG)totalSecs % 3600) / 60),
                  ((ULONG)totalSecs % 60));

           pm_set_title( HwndContainer, buf);
         } else
           pm_set_title( HwndContainer, "No statistics available.");
         break;*/

  /*      case IDM_PM_L_DELETE:
         focus = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
         if (focus != NULL)
         { if (focus->type == PM_TYPE_LIST)
           { strcpy(buf2, focus->text);
             remove(buf2);
           }
         }
         WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(IDM_PM_L_REMOVE), 0);
         break;
  */
       case IDM_PM_LOAD:
        { Record* focus = CmFocus;
          Playable* play;
          // TODO: (minor) this will load all objects when CmFocus gets invalidated
          if (focus == NULL)
            play = Content;
          else if (focus->IsRemoved())
            return 0; // Record removed
          else
            play = &focus->Content->GetPlayable();
          // load!
          amp_load_playable(play->GetURL(), 0);

          return 0;
        }
/*        case IDM_PM_F_LOAD:
         focus = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
         if (focus != NULL)
         { if (focus->type == PM_TYPE_FILE)
             amp_load_singlefile( focus->text, 0 );
         }
         break;
  */
       case IDM_PM_REMOVE:
        { Record* focus = CmFocus;
          DEBUGLOG(("PlaylistManager(%p{%s})::DlgProc: IDM_PM_REMOVE %p\n", this, DebugName().cdata(), focus));
          if (focus == NULL)
            return 0;
          // find parent playlist
          Record* parent = focus->Data->Parent;
          Playable* playlist;
          if (parent == NULL)
            playlist = Content;
          else if (parent->IsRemoved())
            return 0;
          else
            playlist = &parent->Content->GetPlayable();
          if (playlist->GetFlags() & Playable::Mutable) // don't modify constant object
            ((Playlist&)*playlist).RemoveItem(focus->Content);
          // the update of the container is implicitely done by the notification mechanism
        }
        return 0;
      } // switch (SHORT1FROMMP(mp1))
    }
    break;

   case WM_ERASEBACKGROUND:
    return FALSE;

   case WM_SYSCOMMAND:
    if( SHORT1FROMMP(mp1) == SC_CLOSE )
    { SetVisible(false);
      return 0;
    }
    break;

   case WM_WINDOWPOSCHANGED:
    { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW )
        cfg.show_plman = TRUE;
      if( pswp[0].fl & SWP_HIDE )
        cfg.show_plman = FALSE;
    }
    break;
    
   case UM_UPDATEINFO:
    { if (&*EmFocus != (Playable*)mp1)
        return 0; // No longer neccessary, because another UM_UPDATEINFO message is in the queue or the record does no longer exists.
      SetInfo(xstring::sprintf("%s, %s", &*EmFocus->GetURL().getDisplayName(), EmFocus->GetInfo().tech->info));
      if (&*EmFocus == (Playable*)mp1) // double check, because EmFocus may change while SetInfo
        EmFocus = NULL;
    }
    return 0;

   case UM_RECORDCOMMAND:
    { Record* rec = (Record*)PVOIDFROMMP(mp1);
      DEBUGLOG(("PlaylistManager::DlgProc: UM_RECORDCOMMAND: %s\n", Record::DebugName(rec).cdata()));
      // reset pending message flag
      Interlocked il(rec ? rec->Data->PostMsg : EvntState.PostMsg);
      if (il.bitrst(RC_UPDATECHILDREN))
        UpdateChildren(rec);
      if (il.bitrst(RC_UPDATETECH))
        UpdateTech(rec);
      if (il.bitrst(RC_UPDATESTATUS))
        UpdateStatus(rec);
      if (il.bitrst(RC_UPDATEALIAS))
        UpdateAlias(rec);
      DEBUGLOG(("PlaylistManager::DlgProc: UM_RECORDCOMMAND done %s %u\n", Record::DebugName(rec).cdata(), SHORT1FROMMP(mp2)));
      if (SHORT1FROMMP(mp2))
        FreeRecord(rec);
      return 0;
    }
    
   case UM_DELETERECORD:
    DeleteEntry((Record*)PVOIDFROMMP(mp1));
    return 0;
    
   case UM_SYNCREMOVE:
    { Record* rec = (Record*)PVOIDFROMMP(mp1);
      rec->Content = NULL;
      // deregister event handlers too.
      rec->Data->DeregisterEvents(); 
    }
    return 0;
    
  } // switch (msg)

  return WinDefDlgProc( HwndMgr, msg, mp1, mp2 );
}

static MRESULT EXPENTRY DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("PlaylistManager::DlgProcStub(%x, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  PlaylistManager* pm;
  if (msg == WM_INITDLG)
  { PlaylistManager::init_dlg_struct* ip = (PlaylistManager::init_dlg_struct*)mp2;
    DEBUGLOG(("PlaylistManager::DlgProcStub: WM_INITDLG - %p{%i, %p}}\n", ip, ip->size, ip->pm));
    WinSetWindowPtr(hwnd, QWL_USER, ip->pm);
    pm = ip->pm;
    pm->HwndMgr = hwnd; // Store the hwnd early, since LoadDlg will return too late. 
  } else
  { pm = (PlaylistManager*)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pm == NULL)
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  // now call the class method
  return pm->DlgProc(msg, mp1, mp2);
}

struct UserAddCallbackParams
{ int_ptr<Playlist> Parent;
  PlayableInstance* Before;
  UserAddCallbackParams(Playlist* parent, PlayableInstance* before) : Parent(parent), Before(before) {}
};

static void DLLENTRY UserAddCallback(void* param, const char* url)
{ DEBUGLOG(("PlaylistManager:UserAddCallback(%p, %s)\n", param, url));
  UserAddCallbackParams& ucp = *(UserAddCallbackParams*)param;
  ucp.Parent->InsertItem(url, ucp.Before);
} 

void PlaylistManager::UserAdd(DECODER_WIZZARD_FUNC wizzard, const char* title, Record* parent, Record* after)
{ DEBUGLOG(("PlaylistManager(%p)::UserAdd(%p, %s, %p, %p)\n", this, wizzard, title, parent, after));
  if (Record::IsRemoved(parent) || Record::IsRemoved(after))
    return; // sync remove happened
  UserAddCallbackParams ucp(parent ? &(Playlist&)parent->Content->GetPlayable() : &(Playlist&)*Content, after ? after->Content : NULL);
  ULONG ul = (*wizzard)(HwndMgr, title, &UserAddCallback, &ucp);
  DEBUGLOG(("PlaylistManager::UserAdd - %u\n", ul));
  // TODO: cfg.listdir obsolete?
  //sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
}


void PlaylistManager::SetTitle(const xstring& title)
{ DEBUGLOG(("PlaylistManager(%p)::SetTitle(%s)\n", this, title.cdata()));
  if (WinSetWindowText(HwndMgr, (char*)title.cdata()))
    // now free the old title
    Title = title;
}

void PlaylistManager::SetInfo(const xstring& text)
{ DEBUGLOG(("PlaylistManager(%p)::SetInfo(%s)\n", this, text.cdata()));
  CNRINFO cnrInfo = { sizeof(CNRINFO) };
  cnrInfo.pszCnrTitle = (char*)text.cdata(); // OS/2 API doesn't like const...
  if (WinSendMsg(HwndContainer, CM_SETCNRINFO, MPFROMP(&cnrInfo), MPFROMLONG(CMA_CNRTITLE)))
    // now free the old title
    Info = text;
}

void PlaylistManager::PostRecordCommand(Record* rec, RecordCommand cmd)
{ DEBUGLOG(("PlaylistManager(%p)::PostRecordCommand(%p, %u)\n", this, rec, cmd));
  Interlocked il(rec ? rec->Data->PostMsg : EvntState.PostMsg);
  // Check whether the requested bit is already set or another event or there are other events pending.
  if (il.bitset(cmd) || il != (1U<<cmd))
  { DEBUGLOG(("PlaylistManager::PostRecordCommand - nope! %u\n", (unsigned)il));
    return; // requested command is already on the way or another unexecuted message is outstanding
  }
  // There is a little chance that we generate two messages for the same record.
  // The second one will be a no-op in the window procedure.  
  if (rec)
    InterlockedInc(rec->Data->UseCount);
  if (!WinPostMsg(HwndMgr, UM_RECORDCOMMAND, MPFROMP(rec), MPFROMSHORT(TRUE)))
    FreeRecord(rec); // avoid memory leaks
}

void PlaylistManager::FreeRecord(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::FreeRecord(%p)\n", this, rec));
  if (rec && InterlockedDec(rec->Data->UseCount) == 0)
    // we can safely post this message because the record is now no longer used anyway.
    WinPostMsg(HwndMgr, UM_DELETERECORD, MPFROMP(rec), 0);
}

PlaylistManager::IC PlaylistManager::GetRecordUsage(Record* rec)
{ DEBUGLOG(("PlaylistManager::GetRecordUsage(%s)\n", Record::DebugName(rec).cdata()));
  if (rec->Content->GetPlayable().GetStatus() != STA_Used)
  { DEBUGLOG(("PlaylistManager::GetRecordUsage: unused\n"));
    return IC_Normal;
  }
  // Check wether the current call stack is the same as for the current Record ...
  const Playable* root = amp_get_current_root(); // We need no ownership here, since we only compare the reference
  do
  { if (&rec->Content->GetPlayable() == root)
    { // We are a the current root, so the call stack compared equal. 
      DEBUGLOG(("PlaylistManager::GetRecordUsage: current root\n"));
      return rec->Data->Recursive ? IC_Used : IC_Play;
    }
    if (rec->Content->GetStatus() != STA_Used)
    { // The PlayableInstance is not used, so the call stack is not equal.
      DEBUGLOG(("PlaylistManager::GetRecordUsage: instance unused\n"));
      return IC_Used;
    }
    rec = rec->Data->Parent;
  } while (rec != NULL);
  // No more Parent, let's consider this as (partially) equal.
  DEBUGLOG(("PlaylistManager::GetRecordUsage: top level\n"));
  return IC_Play; 
}

HPOINTER PlaylistManager::CalcIcon(Record* rec)
{ DEBUGLOG(("PlaylistManager::CalcIcon(%s) - %u\n", Record::DebugName(rec).cdata(), rec->Data->Recursive));
  assert(rec->Content != NULL);
  if (rec->Content->GetPlayable().GetStatus() == STA_Invalid)
    return IcoInvalid;
  else if ((rec->Content->GetPlayable().GetFlags() & Playable::Enumerable) == 0)
    return IcoSong[GetRecordUsage(rec)];
  else if (rec->Content->GetPlayable().GetInfo().tech->num_items == 0)
    return IcoEmptyPlaylist;
  else
    return IcoPlaylist[GetRecordUsage(rec)][rec->Data->Recursive ? 2 : (rec->flRecordAttr & CRA_EXPANDED) != 0];
}

bool PlaylistManager::RecursionCheck(Playable* pp, Record* parent)
{ DEBUGLOG(("PlaylistManager(%p)::RecursionCheck(%p{%s}, %s)\n", this, pp, pp->GetURL().getObjName().cdata(), Record::DebugName(parent).cdata()));
  if (pp != &*Content)
  { for(;;)
    { if (parent == NULL || parent == (Record*)-1)
      { DEBUGLOG(("PlaylistManager::RecursionCheck: no rec.\n"));
        return false;
      }
      if (parent->IsRemoved())
         // Well, we return true in case of a removed record to avoid more actions 
         return true;
      if (&parent->Content->GetPlayable() == pp)
        break;       
      parent = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(parent), MPFROM2SHORT(CMA_PARENT, CMA_ITEMORDER));
      DEBUGLOG(("PlaylistManager::RecursionCheck: recusrion check %s\n", Record::DebugName(parent).cdata()));
    }
    // recursion in playlist tree
  } // else recursion with top level
  DEBUGLOG(("PlaylistManager::RecursionCheck: recursion!\n"));
  return true;
}
bool PlaylistManager::RecursionCheck(Record* rp)
{ DEBUGLOG(("PlaylistManager::RecursionCheck(%s)\n", Record::DebugName(rp).cdata()));
  Playable* pp = &rp->Content->GetPlayable();
  if (pp != &*Content)
  { do
    { rp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(CMA_PARENT, CMA_ITEMORDER));
      DEBUGLOG2(("PlaylistManager::RecursionCheck: recusrion check %p\n", rp));
      if (rp == NULL || rp == (Record*)-1)
        return false;
      if (rp->IsRemoved())
         // Well, we return true in case of a removed record to avoid more actions 
         return true;
    } while (&rp->Content->GetPlayable() != pp);
    // recursion in playlist tree
  } // else recursion with top level
  DEBUGLOG(("PlaylistManager::RecursionCheck: recursion!\n"));
  return true;
}

void PlaylistManager::DeleteEntry(Record* entry)
{ DEBUGLOG(("PlaylistManager(%p{%s})::DeleteEntry(%s)\n", this, DebugName().cdata(), Record::DebugName(entry).cdata()));
  delete entry->Data;
  assert(LONGFROMMR(WinSendMsg(HwndContainer, CM_FREERECORD, MPFROMP(&entry), MPFROMSHORT(1))));
  DEBUGLOG(("PlaylistManager::DeleteEntry completed\n"));
}

PlaylistManager::Record* PlaylistManager::AddEntry(PlayableInstance* obj, Record* parent, Record* after)
{ DEBUGLOG(("PlaylistManager(%p{%s})::AddEntry(%p{%s}, %p, %p)\n", this, DebugName().cdata(), obj, obj->GetPlayable().GetURL().getObjName().cdata(), parent, after));
  /* Allocate a record in the HwndContainer */
  Record* rec = (Record*)WinSendMsg(HwndContainer, CM_ALLOCRECORD, MPFROMLONG(sizeof(Record) - sizeof(MINIRECORDCORE)), MPFROMLONG(1));
  if (rec == NULL)
  { DEBUGLOG(("PlaylistManager::AddEntry: CM_ALLOCRECORD failed, error %lx\n", WinGetLastError(NULL)));
    DosBeep(500, 100);
    return NULL;
  }
  if (Record::IsRemoved(parent))
    return NULL; // Parent removed

  rec->Content         = obj;
  rec->Data = new Record::record_data(*this, &InfoChangeEvent, &StatChangeEvent, rec, parent);
  rec->Data->Text      = obj->GetDisplayName();
  rec->Data->Recursive = obj->GetPlayable().GetInfo().tech->recursive && RecursionCheck(&obj->GetPlayable(), parent);
          
  rec->flRecordAttr    = 0;
  rec->pszIcon         = (PSZ)rec->Data->Text.cdata();
  rec->hptrIcon        = CalcIcon(rec);

  RECORDINSERT insert      = { sizeof(RECORDINSERT) };
  if (after)
  { insert.pRecordOrder    = (RECORDCORE*)after;
    insert.pRecordParent   = NULL;
  } else
  { insert.pRecordOrder    = (RECORDCORE*)CMA_FIRST;
    insert.pRecordParent   = (RECORDCORE*)parent;
  }
  insert.fInvalidateRecord = TRUE;
  insert.zOrder            = CMA_TOP;
  insert.cRecordsInsert    = 1;
  ULONG rc = LONGFROMMR(WinSendMsg(HwndContainer, CM_INSERTRECORD, MPFROMP(rec), MPFROMP(&insert)));
  
  obj->GetPlayable().InfoChange += rec->Data->InfoChange;
  obj->StatusChange             += rec->Data->StatChange;

  DEBUGLOG(("PlaylistManager::AddEntry: succeeded: %p %u %x\n", rec, rc, WinGetLastError(NULL)));
  return rec;
}

PlaylistManager::Record* PlaylistManager::MoveEntry(Record* entry, Record* parent, Record* after)
{ DEBUGLOG(("PlaylistManager(%p{%s})::MoveEntry(%s, %s, %s)\n", this, DebugName().cdata(),
    Record::DebugName(entry).cdata(), Record::DebugName(parent).cdata(), Record::DebugName(after).cdata()));
  TREEMOVE move =
  { (RECORDCORE*)entry,
    (RECORDCORE*)parent,
    (RECORDCORE*)(after ? after : (Record*)CMA_FIRST),
    FALSE
  };
  if (WinSendMsg(HwndContainer, CM_MOVETREE, MPFROMP(&move), 0) == FALSE)
    return NULL;
  return entry;
}

void PlaylistManager::RemoveEntry(Record* const entry)
{ DEBUGLOG(("PlaylistManager(%p{%s})::RemoveEntry(%p)\n", this, DebugName().cdata(), entry));
  if (CmFocus == entry) // TODO: no good solution
    CmFocus = NULL;
  // detach content
  entry->Content = NULL;
  // deregister events
  entry->Data->DeregisterEvents();
  // Delete the children
  RemoveChildren(entry);
  // Remove record from container
  assert(LONGFROMMR(WinSendMsg(HwndContainer, CM_REMOVERECORD, MPFROMP(&entry), MPFROM2SHORT(1, CMA_INVALIDATE))) != -1);
  // Release reference counter
  // The record will be deleted when no outstanding PostMsg is on the way.  
  FreeRecord(entry);
  DEBUGLOG(("PlaylistManager::RemoveEntry completed\n"));
}

int PlaylistManager::RemoveChildren(Record* const rp)
{ DEBUGLOG(("PlaylistManager(%p)::DeleteChildren(%p)\n", this, rp));
  Record* crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(rp ? CMA_FIRSTCHILD : CMA_FIRST, CMA_ITEMORDER));
  int count = 0;
  while (crp != NULL && crp != (Record*)-1)
  { DEBUGLOG(("CM_QUERYRECORD: %s\n", Record::DebugName(crp).cdata()));
    Record* crp2 = crp;
    crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    RemoveEntry(crp2);
    ++count;
  }
  return count;
}

void PlaylistManager::UpdateChildren(Record* const rp)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateChildren(%s)\n", this, Record::DebugName(rp).cdata()));
  // get content
  Playable* pp;
  if (rp == NULL)
    pp = Content;
  else if (rp->IsRemoved() || rp->Data->Recursive)
    return; // record removed or recursive
  else
    pp = &rp->Content->GetPlayable();
  DEBUGLOG(("PlaylistManager::UpdateChildren - %u\n", pp->GetStatus()));
  if ((pp->GetFlags() & Playable::Enumerable) == 0 || pp->GetStatus() <= STA_Invalid)
  { // Nothing to enumerate, delete children if any
    DEBUGLOG(("PlaylistManager::UpdateChildren - no children possible.\n"));
    if (RemoveChildren(rp))
      PostRecordCommand(rp, RC_UPDATESTATUS); // update icon
    return;
  }
  
  // Check if techinfo is available, otherwise wait
  if (pp->EnsureInfoAsync(Playable::IF_Tech) == 0)
  { if (rp)
      rp->Data->WaitUpdate = true;
     else
      EvntState.WaitUpdate = true;
    return;
  }

  // First check what's currently in the container.
  Record* OldHead = NULL;
  Record* OldTail = NULL;
  int status = 0;
  { // The collection is not locked so far, but the Delete Event ensures that the parent node is removed
    // from the worker's queue before the children are deleted. So we are safe here.
    Record* crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(rp), MPFROM2SHORT(rp ? CMA_FIRSTCHILD : CMA_FIRST, CMA_ITEMORDER));
    while (crp != NULL && crp != (Record*)-1)
    { DEBUGLOG(("PlaylistManager::UpdateChildren CM_QUERYRECORD: %s\n", Record::DebugName(crp).cdata()));
      if (OldTail)
        OldTail->preccNextRecord = crp;
       else
        OldHead = crp;
      OldTail = crp;
      crp = (Record*)WinSendMsg(HwndContainer, CM_QUERYRECORD, MPFROMP(crp), MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
    }
    if (OldTail)
    { status |= 1; // old exists
      OldTail->preccNextRecord = NULL;
    }
   
    // Now check what should be in the container
    DEBUGLOG(("PlaylistManager::UpdateChildren - check container.\n"));
    Mutex::Lock lock(pp->Mtx); // Lock the collection
    sco_ptr<PlayableEnumerator> ep = ((PlayableCollection*)pp)->GetEnumerator();
    crp = NULL;
    while (ep->Next())
    { status |= 2; // new exists
      // Find entry in the current content
      Record** nrpp = &OldHead;
      for (;;)
      { if (*nrpp == NULL)
        { // not found! => add
          DEBUGLOG(("PlaylistManager::UpdateChildren - not found: %p{%s}\n", &**ep, (*ep)->GetPlayable().GetURL().getObjName().cdata()));
          crp = AddEntry(&**ep, rp, crp);
          if (crp && (rp == NULL || ((rp->flRecordAttr & CRA_EXPANDED) && !crp->Data->Recursive)))
            RequestChildren(crp);
          break;
        }
        if ((*nrpp)->Content == &**ep)
        { // found! => move
          if (nrpp == &OldHead)
          { // already in right order
            DEBUGLOG(("PlaylistManager::UpdateChildren - found: %p{%s} at HEAD\n", &**ep, (*ep)->GetPlayable().GetURL().getObjName().cdata()));
            crp = OldHead;
          } else
          { // move
            DEBUGLOG(("PlaylistManager::UpdateChildren - found: %p{%s} at %p\n", &**ep, (*ep)->GetPlayable().GetURL().getObjName().cdata(), *nrpp));
            crp = MoveEntry(*nrpp, rp, crp);
          }
          // Remove from queue
          *nrpp = (Record*)(*nrpp)->preccNextRecord;
          break;
        }
        nrpp = &(Record*&)(*nrpp)->preccNextRecord; 
      }
    }
  }
  // Icon update required? (Number of items changed from or to zero.)
  if (((status>>1) ^ status) & 1)
    PostRecordCommand(rp, RC_UPDATESTATUS);

  // Well normally the queue should be empty now...
  DEBUGLOG(("PlaylistManager::UpdateChildren - OldHead = %p\n", OldHead));
  while (OldHead) // if not: delete!
  { RemoveEntry(OldHead); // The record is deleted later, so we cann access OldHead safely.
    OldHead = (Record*)OldHead->preccNextRecord;
  }
}

void PlaylistManager::UpdateTech(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateTech(%p)\n", this, rec));
  if (Record::IsRemoved(rec))
    return;
  // techinfo changed => check whether it is currently visible.
  if (rec->flRecordAttr & CRA_CURSORED)
  { // TODO: maybe this should be better up to the calling thread
    EmFocus = &rec->Content->GetPlayable();
    // continue later, because I/O may be necessary
    WinPostMsg(HwndMgr, UM_UPDATEINFO, MPFROMP(&*rec), 0);
  }
  bool recursive = rec->Content->GetPlayable().GetInfo().tech->recursive && RecursionCheck(rec);
  if (recursive != rec->Data->Recursive)
  { rec->Data->Recursive = recursive;
    // Update Icon also 
    PostRecordCommand(rec, RC_UPDATESTATUS);
  }
  // Check if UpdateChildren is waiting
  bool& wait = rec ? rec->Data->WaitUpdate : EvntState.WaitUpdate;
  if (wait)
  { // Try again
    wait = false;
    PostRecordCommand(rec, RC_UPDATECHILDREN);
  }
}

void PlaylistManager::UpdateStatus(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateIcon(%p)\n", this, rec));
  if (rec == NULL)
  { // Root node => update title
    xstring title = Alias ? Alias : Content->GetURL().getDisplayName();
    switch (Content->GetStatus())
    {case STA_Invalid:
      title = title + " [invalid]";
      break;
     case STA_Used:
      title = title + " [used]";
      break;
    }
    SetTitle(title);
  } else
  { // Record node
    if (rec->IsRemoved())
      return;
    HPOINTER icon = CalcIcon(rec);
    // update icon?
    //if (rec->hptrIcon != icon)
    { rec->hptrIcon = icon; 
      WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_NOREPOSITION));
    }
  }
}

void PlaylistManager::UpdateAlias(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::UpdateAlias(%p)\n", this, rec));
  if (Record::IsRemoved(rec))
    return;
  xstring text = rec->Content->GetDisplayName();
  rec->pszIcon = (PSZ)text.cdata();
  WinSendMsg(HwndContainer, CM_INVALIDATERECORD, MPFROMP(&rec), MPFROM2SHORT(1, CMA_TEXTCHANGED));
  rec->Data->Text = text; // now free the old text
}

void PlaylistManager::RequestChildren(Record* rec)
{ DEBUGLOG(("PlaylistManager(%p)::RequestChildren(%s)\n", this, Record::DebugName(rec).cdata()));
  Playable* pp;
  if (rec == NULL)
    pp = Content;
  else if (rec->IsRemoved())
    return;
  else
    pp = &rec->Content->GetPlayable();
  if ((pp->GetFlags() & Playable::Enumerable) == 0)
    return;
  // Call event either immediately or later, asynchronuously.
  InfoChangeEvent(Playable::change_args(*pp, pp->EnsureInfoAsync(Playable::IF_Other|Playable::IF_Tech)), rec);
}

void PlaylistManager::InfoChangeEvent(const Playable::change_args& args, Record* rec)
{ DEBUGLOG(("PlaylistManager(%p{%s})::InfoChangeEvent({%p{%s}, %x}, %p{%p{%p}})\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.GetURL().getObjName().cdata(), args.Flags, rec, rec ? rec->Content: NULL, rec && rec->Content ? &rec->Content->GetPlayable():NULL));
  DEBUGLOG(("PlaylistManager(%p{%s})::InfoChangeEvent({%p{%s}, %x}, %s)\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.GetURL().getObjName().cdata(), args.Flags, Record::DebugName(rec).cdata()));

  if (args.Flags & Playable::IF_Other)
    PostRecordCommand(rec, RC_UPDATECHILDREN);

  if (args.Flags & Playable::IF_Status)
    PostRecordCommand(rec, RC_UPDATESTATUS);

  if (rec && args.Flags & Playable::IF_Tech)
    PostRecordCommand(rec, RC_UPDATETECH);
}

void PlaylistManager::StatChangeEvent(const PlayableInstance::change_args& args, Record* rec)
{ DEBUGLOG(("PlaylistManager(%p{%s})::StatChangeEvent({%p{%s}, %x}, %p)\n", this, DebugName().cdata(),
    &args.Instance, args.Instance.GetPlayable().GetURL().getObjName().cdata(), args.Flags, rec));

  if (args.Flags & PlayableInstance::SF_Destroy)
  { WinSendMsg(HwndMgr, UM_SYNCREMOVE, MPFROMP(rec), 0);
    return;
  }

  if (args.Flags & PlayableInstance::SF_InUse)
    PostRecordCommand(rec, RC_UPDATESTATUS);

  if (args.Flags & PlayableInstance::SF_Alias)
    PostRecordCommand(rec, RC_UPDATEALIAS);
}

/*

static MRESULT EXPENTRY
pm_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  POINTL ptlMouse;
  PPMREC focus, zf;
  HWND   hwndMenu;
  PFREC  fr;
  char   buf[256], buf2[256], buf3[256];
  PNOTIFYRECORDEMPHASIS emphasis;

  switch (msg)
  {case WM_INITDLG:
    { HPOINTER hicon;
      LONG     color;

      pfreq_create_HwndContainer( hwnd, FID_CLIENT );
      _beginthread( Manager_Populate, NULL, 0xFFFF, NULL );

      hicon = WinLoadPointer( HWND_DESKTOP, 0, ICO_MAIN );
      WinSendMsg( hwnd, WM_SETICON, (MPARAM)hicon, 0 );
      do_warpsans( hwnd );

      if( !rest_window_pos( hwnd, 0 ))
      { color = CLR_GREEN;
        WinSetPresParam( HwndContainer, PP_FOREGROUNDCOLORINDEX, sizeof(color), &color);
        color = CLR_BLACK;
        WinSetPresParam( HwndContainer, PP_BACKGROUNDCOLORINDEX, sizeof(color), &color);
      }

      dk_add_window( hwnd, 0 );
      return 0;
    }

   case WM_CONTROL:
    switch (SHORT2FROMMP(mp1))
    {

     case CN_HELP:
      amp_show_help( IDH_PM );
      return 0;

    } // switch (SHORT2FROMMP(mp1))
    return 0;

   case WM_COMMAND:
    switch (COMMANDMSG(&msg) -> cmd)
     {case IDM_PM_L_CALC:
       totalFiles = totalSecs = totalLength = 0;

       zf = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));

       if (!zf) { DosBeep(500, 50); break; }

       focus  = (PPMREC) WinSendMsg( HwndContainer,
                                     CM_QUERYRECORD,
                                     MPFROMP(zf),
                                     MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));

       while (focus != NULL)
       { if (focus->type == PM_TYPE_FILE && focus->data != NULL)
         { fr = (PFREC)focus->data;
           totalFiles++;
           totalSecs += fr->secs;
           totalLength += (fr->length / 1024);
         }

         focus  = (PPMREC) WinSendMsg( HwndContainer,
                                       CM_QUERYRECORD,
                                       MPFROMP(focus),
                                       MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
       }

       if (totalFiles > 0)
       { sprintf(buf, "Total %lu files, %lu kB, %lud %luh %lum %lus",
                totalFiles,
                totalLength,
                ((ULONG)totalSecs / 86400),
                ((ULONG)((totalSecs % 86400) / 3600)),
                (((ULONG)totalSecs % 3600) / 60),
                ((ULONG)totalSecs % 60));

         pm_set_title( HwndContainer, buf);
       } else
         pm_set_title( HwndContainer, "No statistics available for this playlist.");
       break;

      case IDM_PM_CALC:
       totalFiles = totalSecs = totalLength = 0;

       zf  = (PPMREC) WinSendMsg( HwndContainer,
                                  CM_QUERYRECORD,
                                  NULL,
                                  MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

       while (zf != NULL)
       { focus  = (PPMREC) WinSendMsg( HwndContainer,
                                       CM_QUERYRECORD,
                                       MPFROMP(zf),
                                       MPFROM2SHORT(CMA_FIRSTCHILD, CMA_ITEMORDER));

         while (focus != NULL)
         { if (focus->type == PM_TYPE_FILE && focus->data != NULL)
           { fr = (PFREC)focus->data;
             totalFiles++;
             totalSecs += fr->secs;
             totalLength += (fr->length / 1024);
           }
           focus = (PPMREC) WinSendMsg( HwndContainer,
                                        CM_QUERYRECORD,
                                        MPFROMP(focus),
                                        MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
         }
         zf = (PPMREC) WinSendMsg( HwndContainer,
                                   CM_QUERYRECORD,
                                   MPFROMP(zf),
                                   MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
       }
       if (totalFiles > 0)
       { sprintf(buf, "Total %lu files, %lu kB, %lud %luh %lum %lus",
                totalFiles,
                totalLength,
                ((ULONG)totalSecs / 86400),
                ((ULONG)((totalSecs % 86400) / 3600)),
                (((ULONG)totalSecs % 3600) / 60),
                ((ULONG)totalSecs % 60));

         pm_set_title( HwndContainer, buf);
       } else
         pm_set_title( HwndContainer, "No statistics available.");
       break;

      case IDM_PM_L_DELETE:
       focus = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
       if (focus != NULL)
       { if (focus->type == PM_TYPE_LIST)
         { strcpy(buf2, focus->text);
           remove(buf2);
         }
       }
       WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(IDM_PM_L_REMOVE), 0);
       break;

      case IDM_PM_L_REMOVE:
       focus = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
       if (focus != NULL)
       { if (focus->type == PM_TYPE_LIST)
         { strcpy(buf2, focus->text);
           pm_destroy_children_and_self( HwndContainer, focus);

           sprintf(buf, "%sPM123.MGR", startpath);
           mlist = fopen(buf, "r");
           if (mlist != NULL)
           { sprintf(buf3, "%sPM123MGR.$$$", startpath);
             mtemp = fopen(buf3, "w");
             if (mtemp == NULL)
             { fclose(mlist);
               WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, "Couldn't open temp file PM123MGR.$$$. Are you out of hard disk space?", "Error", 0, MB_ERROR | MB_OK);
               return ((MRESULT)0);
             }
             while (!feof(mlist))
             { fgets(buf3, sizeof(buf3), mlist);
               blank_strip(buf3);
               if (stricmp(buf2, buf3) != 0) fprintf(mtemp, "%s\n", buf3);
             }
             fclose(mlist);
             fclose(mtemp);
             remove(buf);
             sprintf(buf3, "%sPM123MGR.$$$", startpath);
             rename(buf3, buf);
             WinSendMsg( HwndContainer, CM_INVALIDATERECORD, MPFROMP(NULL), MPFROM2SHORT(0, 0));
           }
         }
       }
       break;

      case IDM_PM_L_LOAD:
       focus = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
       if (focus != NULL && focus->type == PM_TYPE_LIST)
         pl_load( focus->text, PL_LOAD_CLEAR );

       break;

      case IDM_PM_F_LOAD:
       focus = (PPMREC) PVOIDFROMMR(WinSendMsg( HwndContainer, CM_QUERYRECORDEMPHASIS, MPFROMLONG(CMA_FIRST), MPFROMSHORT(CRA_SELECTED)));
       if (focus != NULL)
       { if (focus->type == PM_TYPE_FILE)
           amp_load_singlefile( focus->text, 0 );
       }
       break;

      case IDM_PM_ADD:
       idm_pm_add(hwnd);
       break;
     } // switch (COMMANDMSG(&msg) -> cmd)
    return 0;

   case WM_ERASEBACKGROUND:
    return FALSE;

   case WM_SYSCOMMAND:
    if( SHORT1FROMMP(mp1) == SC_CLOSE )
    { pm_show( FALSE );
      return 0;
    }
    break;

   case WM_WINDOWPOSCHANGED:
    { SWP* pswp = PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW )
        cfg.show_HwndMgr = TRUE;
      if( pswp[0].fl & SWP_HIDE )
         cfg.show_HwndMgr = FALSE;
      break;
    }
  } // switch (msg)

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}
*/


/* Repository */
/*StringRepository PlaylistManager::RPInst(16);
Mutex            PlaylistManager::RPMutex;

int PlaylistManager::CompareTo(const char* str) const
{ return stricmp(Content->GetURL(), str);
}

PlaylistManager& PlaylistManager::Get(const char* url, const char* alias)
{ 
}
*/
