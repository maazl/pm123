/*
 * Copyright 2007-2008 Marcel M�eller
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
#include <utilfct.h>
#include "windowbase.h"

#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include <debuglog.h>


/****************************************************************************
*
*  class DialogBase
*
****************************************************************************/

DialogBase::DialogBase(ULONG rid, HMODULE module, DlgFlags flags)
: DlgRID(rid),
  ResModule(module),
  Flags(flags),
  HwndFrame(NULLHANDLE),
  InitialVisible(false),
  Initialized(0)
{ DEBUGLOG(("DialogBase(%p)::DialogBase(%u, %p, %x)\n", this, rid, module, flags));
}

DialogBase::~DialogBase()
{ DEBUGLOG(("DialogBase(%p{%u})::~DialogBase()\n", this, DlgRID));
  if (HwndFrame != NULLHANDLE)
    WinDestroyWindow(HwndFrame);
}

MRESULT EXPENTRY wb_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("DialogBase::DlgProcStub(%x, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  DialogBase* pb;
  if (msg == WM_INITDLG)
  { // Attach the class instance to the window.
    DialogBase::init_dlg_struct* ip = (DialogBase::init_dlg_struct*)mp2;
    pb = ip->pm;
    DEBUGLOG(("DialogBase::DlgProcStub: WM_INITDLG - %p{%i, %p}}\n", ip, ip->size, ip->pm));
    pb->HwndFrame = hwnd; // Store the hwnd early, since LoadDlg will return too late.
    pb->OnInit();
  } else
  { // Lookup class instance
    pb = (DialogBase*)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pb == NULL)
      // No class attached. We only can do default processing so far.
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  // Now call the class method
  MRESULT r = pb->DlgProc(msg, mp1, mp2);
  switch (msg)
  {case WM_INITDLG:
    // Visibility
    pb->Initialized = 1;
    if (WinIsWindowVisible(pb->HwndFrame) != pb->InitialVisible)
      pb->SetVisible(pb->InitialVisible);
    pb->Initialized = 2;
    break;
   case WM_DESTROY:
    pb->OnDestroy();
    break;
  }
  return r;
}

void DialogBase::StartDialog(HWND owner, HWND parent)
{ DEBUGLOG(("DialogBase(%p)::StartDialog(%p, %p)\n", this, owner, parent));
  // initialize dialog
  init_dlg_struct ids = { sizeof(init_dlg_struct), this };
  PMRASSERT(WinLoadDlg(parent, owner, wb_DlgProcStub, ResModule, DlgRID, &ids));
}

MRESULT DialogBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_ADJUSTWINDOWPOS:
    { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
      DEBUGLOG(("DialogBase::DlgProc WM_ADJUSTWINDOWPOS: %x\n", pswp->fl));
      if ((Flags & DF_AutoResize) && (pswp->fl & SWP_SIZE))
        dlg_adjust_resize(GetHwnd(), pswp);
    }
    break;

   case WM_WINDOWPOSCHANGED:
    { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
      DEBUGLOG(("DialogBase::DlgProc WM_WINDOWPOSCHANGED: %x, %u\n", pswp->fl, mp2));
      if ((Flags & DF_AutoResize) && (pswp->fl & SWP_SIZE))
        dlg_do_resize(GetHwnd(), pswp, pswp+1);
    }
    break;
  }

  return WinDefDlgProc(HwndFrame, msg, mp1, mp2);
}

void DialogBase::OnInit()
{ // Attach the class instance to the window.
  DEBUGLOG(("DialogBase(%p{%u})::OnInit()\n", this, DlgRID));
  PMRASSERT(WinSetWindowPtr(HwndFrame, QWL_USER, this));
}

void DialogBase::OnDestroy()
{ DEBUGLOG(("DialogBase(%p{%u})::OnDestroy()\n", this, DlgRID));
  // Keep instance no longer alive
  Initialized = 0;
  if (HwndFrame != NULLHANDLE)
  { PMRASSERT(WinSetWindowPtr(HwndFrame, QWL_USER, NULL));
    HwndFrame = NULLHANDLE;
  }
}

void DialogBase::SetVisible(bool show)
{ DEBUGLOG(("DialogBase(%p{%u})::SetVisible(%u)\n", this, DlgRID, show));

  if (Initialized == 0) // double check
  { CritSect cs;
    if (Initialized == 0)
    { InitialVisible = show;
      return;
  } }
  
  PMRASSERT(WinSetWindowPos(HwndFrame, HWND_TOP, 0, 0, 0, 0, show ? SWP_SHOW|SWP_ZORDER|SWP_ACTIVATE : SWP_HIDE));
}

bool DialogBase::GetVisible() const
{ if (Initialized < 2)
  { CritSect cs;
    if (Initialized < 2)
      return InitialVisible;
  }
  return WinIsWindowVisible(HwndFrame);
}

xstring DialogBase::GetTitle() const
{ size_t len = WinQueryWindowTextLength(HwndFrame);
  xstring ret;
  return WinQueryWindowText(HwndFrame, len+1, ret.allocate(len)) ? ret : xstring();
}

void DialogBase::SetTitle(const char* title)
{ DEBUGLOG(("DialogBase(%p)::SetTitle(%s)\n", this, title));
  PMRASSERT(WinSetWindowText(HwndFrame, title));
}

void DialogBase::PostMsg(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG(("DialogBase(%p)::PostMsg(%lu, %p, %p)\n", this, msg, mp1, mp2));
  PMRASSERT(WinPostMsg(HwndFrame, msg, mp1, mp2));
}

bool DialogBase::PullMsg(ULONG msg, MPARAM* mp1, MPARAM* mp2)
{ DEBUGLOG(("DialogBase(%p)::PullMsg(%lu,)\n", this, msg));
  QMSG qmsg;
  if (!WinPeekMsg(NULL, &qmsg, HwndFrame, msg, msg, PM_REMOVE))
    return false;
  if (mp1)
    *mp1 = qmsg.mp1;
  if (mp2)
    *mp2 = qmsg.mp2;
  return true;
}

void DialogBase::SetHelpMgr(HWND hhelp)
{ DEBUGLOG(("DialogBase(%p)::SetHelpMgr(%p)\n", this, hhelp));
  PMRASSERT(WinAssociateHelpInstance(hhelp, GetHwnd()));
}


/****************************************************************************
*
*  class NotebookDialogBase
*
****************************************************************************/

NotebookDialogBase::PageBase::PageBase(NotebookDialogBase& parent, ULONG rid, HMODULE module, DlgFlags flags)
: DialogBase(rid, module, flags),
  Parent(parent)
{}

NotebookDialogBase::NotebookDialogBase(ULONG rid, HMODULE module, DlgFlags flags)
: DialogBase(rid, module, flags)
, NotebookCtrl(NULLHANDLE)
{}

NotebookDialogBase::~NotebookDialogBase()
{ // notify current page about deselection
  PAGESELECTNOTIFY pn;
  pn.ulPageIdCur = NotebookCtrl.CurrentPageID();
  PageBase* page = PageFromID(pn.ulPageIdCur);
  if (page != NULL)
  { pn.hwndBook = NotebookCtrl.Hwnd;
    pn.ulPageIdNew = 0;
    WinSendMsg(page->GetHwnd(), WM_CONTROL, MPFROM2SHORT(ControlBase(page->GetHwnd()).ID(), BKN_PAGESELECTEDPENDING), MPFROMP(&pn));
  }
}

void NotebookDialogBase::StartDialog(HWND owner, ULONG nbid, HWND parent)
{ DEBUGLOG(("NotebookDialogBase(%p)::StartDialog(%p, %u, %p)\n", this, owner, nbid, parent));
  DialogBase::StartDialog(owner, parent);
  NotebookCtrl.Hwnd = WinWindowFromID(GetHwnd(), nbid);
  ASSERT(NotebookCtrl.Hwnd);
  // setup notebook windows
  int index = 0;
  int total = 0;
  for (PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
  { PageBase* p = *pp;
    DEBUGLOG(("NotebookDialogBase::StartDialog Page %p {%s,%s}\n", p, p->MajorTitle.cdata(), p->MinorTitle.cdata()));
    p->StartDialog();
    // Count index totals
    if (p->MajorTitle != NULL)
    { index = total = 0;
      if (p->MinorTitle != NULL)
      { PageBase*const* pp2 = pp;
        while (++pp2 != Pages.end() && (*pp2)->MajorTitle == NULL && (*pp2)->MinorTitle != NULL)
          ++total;
      }
      if (total)
        ++total;
    }
    if (total)
      ++index;
    p->PageID = nb_append_tab(NotebookCtrl.Hwnd, p->GetHwnd(), p->MajorTitle.cdata(), p->MinorTitle.cdata(), MPFROM2SHORT(index,total));
    PMASSERT(p->PageID != 0);
  }
}

NotebookDialogBase::PageBase* NotebookDialogBase::PageFromID(ULONG pageid)
{ for (PageBase*const* pp = Pages.begin(); pp != Pages.end(); ++pp)
    if ((*pp)->PageID == pageid)
      return *pp;
  return NULL;
}

MRESULT NotebookDialogBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_CONTROL:
    if (SHORT1FROMMR(mp1) == NotebookCtrl.ID())
    { switch (SHORT2FROMMP(mp1))
      { // propagate page change notifications to page windows
       case BKN_PAGESELECTEDPENDING:
        { PAGESELECTNOTIFY& notify = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
          if (notify.ulPageIdNew && notify.ulPageIdCur)
          { HWND page = PageFromID(notify.ulPageIdCur)->GetHwnd();
            WinSendMsg(page, msg, MPFROM2SHORT(ControlBase(page).ID(), BKN_PAGESELECTEDPENDING), mp2);
          }
        }
        break;
       case BKN_PAGESELECTED:
        { PAGESELECTNOTIFY& notify = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
          if (notify.ulPageIdCur)
          { PageBase* page = PageFromID(notify.ulPageIdCur);
            if (page != NULL)
              WinSendMsg(page->GetHwnd(), msg, MPFROM2SHORT(ControlBase(page->GetHwnd()).ID(), BKN_PAGESELECTED), mp2);
          }
          if (notify.ulPageIdNew)
          { PageBase* page = PageFromID(notify.ulPageIdNew);
            if (page != NULL) // We sometimes get called with an invalid ulPageIdNew.
              WinSendMsg(page->GetHwnd(), msg, MPFROM2SHORT(ControlBase(page->GetHwnd()).ID(), BKN_PAGESELECTED), mp2);
          }
        }
        break;
      }
    }
    break;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}


void SubclassWindow::DoAttach()
{ // Replace HWND parameter of window function by this.
  OldWinProc = WinSubclassWindow(Hwnd, (PFNWP)mkvreplace1(&vrWinProc, (V_FUNC)&scw_WinProcStub, this));
  PMASSERT(OldWinProc);
}

void SubclassWindow::Attach(HWND hwnd)
{ ASSERT(!Hwnd);
  Hwnd = hwnd;
  DoAttach();
}

void SubclassWindow::Detach()
{ if (Hwnd)
  { PMRASSERT(WinSubclassWindow(Hwnd, OldWinProc));
    Hwnd = NULLHANDLE;
  }
}

SubclassWindow::SubclassWindow()
: Hwnd(NULLHANDLE)
{}
SubclassWindow::SubclassWindow(HWND hwnd)
: Hwnd(hwnd)
{ DoAttach();
}

SubclassWindow::~SubclassWindow()
{ Detach();
}

MRESULT EXPENTRY scw_WinProcStub(SubclassWindow* that, ULONG msg, MPARAM mp1, MPARAM mp2)
{ return that->WinProc(msg, mp1, mp2);
}

MRESULT SubclassWindow::WinProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ return (*OldWinProc)(Hwnd, msg, mp1, mp2);
}
