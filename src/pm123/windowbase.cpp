/*
 * Copyright 2007-2008 Marcel MÅeller
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

DialogBase::DialogBase(ULONG rid, HMODULE module)
: DlgRID(rid),
  ResModule(module),
  HwndFrame(NULLHANDLE),
  InitialVisible(false),
  Initialized(0)
{ DEBUGLOG(("DialogBase(%p)::DialogBase(%u, %p)\n", this, rid, module));
}

DialogBase::~DialogBase()
{ DEBUGLOG(("DialogBase(%p)::~DialogBase()\n", this));
}

void DialogBase::StartDialog(HWND parent, HWND owner)
{ DEBUGLOG(("DialogBase(%p)::StartDialog(%p, %p)\n", this, owner, parent));
  // initialize dialog
  init_dlg_struct ids = { sizeof(init_dlg_struct), this };
  PMRASSERT(WinLoadDlg(parent, owner, wb_DlgProcStub, ResModule, DlgRID, &ids));
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

MRESULT DialogBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ return WinDefDlgProc(HwndFrame, msg, mp1, mp2);
}

void DialogBase::OnInit()
{ // Attach the class instance to the window.
  DEBUGLOG(("DialogBase(%p)::OnInit()\n", this));
  PMRASSERT(WinSetWindowPtr(HwndFrame, QWL_USER, this));
}

void DialogBase::OnDestroy()
{ DEBUGLOG(("DialogBase(%p)::OnDestroy()\n", this));
  // Keep instance no longer alive
  PMRASSERT(WinSetWindowPtr(HwndFrame, QWL_USER, NULL));
  Initialized = 0;
  HwndFrame = NULLHANDLE;
}

void DialogBase::SetVisible(bool show)
{ DEBUGLOG(("DialogBase(%p)::SetVisible(%u)\n", this, show));

  if (Initialized == 0) // double check
  { CritSect cs;
    if (Initialized == 0)
    { InitialVisible = show;
      return;
  } }
  
  //PMRASSERT(WinShowWindow(HwndFrame, show));
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

void DialogBase::SetTitle(const xstring& title)
{ DEBUGLOG(("DialogBase(%p)::SetTitle(%s)\n", this, title.cdata()));
  // Update Window Title
  PMRASSERT(WinSetWindowText(HwndFrame, (char*)title.cdata()));
  // now free the old title
  Title = title;
}


void ManagedDialogBase::OnInit()
{ DialogBase::OnInit();
  Self = this;
}

void ManagedDialogBase::OnDestroy()
{ DialogBase::OnDestroy();
  Self = NULL; // this may get invalid here
}
