/*
 * Copyright 2011-2011 M.Mueller
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

#define INCL_ERRORS
#include "pmutils.h"


/*xstring WinQueryWindowXText(HWND hwnd)
{ xstring ret;
  char* dst = ret.allocate(WinQueryWindowTextLength(hwnd));
  PMXASSERT(WinQueryWindowText(hwnd, ret.length()+1, dst), == ret.length());
  return ret;
}*/

bool WinIsMyProcess(HWND hwnd)
{ PID pid; TID tid;
  if (!WinQueryWindowProcess(hwnd, &pid, &tid))
    return false;
  PPIB ppib;
  DosGetInfoBlocks(NULL, &ppib);
  return ppib->pib_ulpid == pid;
}


const xstring DrgQueryStrXName(HSTR hstr)
{ xstring ret;
  if (hstr != NULLHANDLE)
  { size_t len = DrgQueryStrNameLen(hstr);
    DrgQueryStrName(hstr, len+1, ret.allocate(len));
  }
  return ret;
}

bool DrgVerifyStrXValue(HSTR hstr, const char* str)
{ if (!str)
    return hstr == NULLHANDLE;
  size_t len = strlen(str);
  if (len != DrgQueryStrNameLen(hstr))
    return false;
  char* cp = (char*)alloca(len+1);
  DrgQueryStrName(hstr, len+1, cp);
  return strcmp(cp, str) == 0;
}

void DrgDeleteStrings(DRAGITEM& di)
{ DrgDeleteString(di.hstrType);
  DrgDeleteString(di.hstrRMF);
  DrgDeleteString(di.hstrContainerName);
  DrgDeleteString(di.hstrSourceName);
  DrgDeleteString(di.hstrTargetName);
}


const xstring DragItem::SourcePath() const
{ xstring fullname;
  if (DI->hstrContainerName && DI->hstrSourceName)
  { size_t lenP = DrgQueryStrNameLen(DI->hstrContainerName);
    size_t lenN = DrgQueryStrNameLen(DI->hstrSourceName);
    char* cp = fullname.allocate(lenP + lenN);
    DrgQueryStrName(DI->hstrContainerName, lenP+1, cp);
    //cp[lenP] = '/';
    DrgQueryStrName(DI->hstrSourceName, lenN+1, cp+lenP);
  }
  return fullname;
}

ScopedDragInfo::ScopedDragInfo(size_t n)
: DragInfo(DrgAllocDraginfo(n))
, OwnStrings(true)
{ DEBUGLOG(("ScopedDragInfo(%p)::ScopedDragInfo(%u)\n", this, n));
  PMASSERT(DI);
}
ScopedDragInfo::ScopedDragInfo(DRAGINFO* di)
: DragInfo(DrgAccessDraginfo(di) ? di : NULL)
, OwnStrings(false)
{ DEBUGLOG(("ScopedDragInfo(%p)::ScopedDragInfo(%p)\n", this, di));
  // Check whether source and destination are the same process.
  // Then we need to mark the instance as in use, because DrgAccessDraginfo
  // sadly fails in this case.
  di->usReserved = WinIsMyProcess(di->hwndSource);
}

ScopedDragInfo::~ScopedDragInfo()
{ DEBUGLOG(("ScopedDragInfo(%p{%p,%u})::~ScopedDragInfo()\n", this, DI, OwnStrings));
  if (!DI)
    return;
  if (OwnStrings)
  { for (USHORT i = 0; i < DI->cditem; ++i)
      DrgDeleteStrings(*DrgQueryDragitemPtr(DI, i));
  } else if (DI->usReserved & 1)
    return; // Same process => only the strings owner must free the infos.
  DI->usReserved &= ~1;
  ULONG err = DrgFreeDraginfo(DI) ? 0 : WinGetErrorInfo(NULLHANDLE)->idError;
  DEBUGLOG(("ScopedDragInfo::~ScopedDragInfo: rc = %x\n", err));
  PMASSERT(!err || err == PMERR_SOURCE_SAME_AS_TARGET);
}

HWND ScopedDragInfo::Drag(HWND source, DRAGIMAGE* img, ULONG nimg, LONG vkterm)
{ DEBUGLOG(("ScopedDragInfo({%p})::Drag(%x, %p, %lu, %li)\n", DI, source, img, nimg, vkterm));
  // If DrgDrag returns NULLHANDLE, that means the user hit Esc or F1
  // while the drag was going on so the target didn't have a chance to
  // delete the string handles. So it is up to the source window to do
  // it. Unfortunately there doesn't seem to be a way to determine
  // whether the NULLHANDLE means Esc was pressed as opposed to there
  // being an error in the drag operation. So we don't attempt to figure
  // that out. To us, a NULLHANDLE means Esc was pressed...
  HWND ret = DrgDrag(source, DI, img, nimg, vkterm, NULL);
  DEBUGLOG(("ScopedDragInfo::Drag: %x\n", ret));
  if (ret)
    // Transfer string ownership to the target.
    OwnStrings = false;
  return ret;
}


bool DragTransfer::RenderToPrepare(const char* str)
{ DEBUGLOG(("DragTransfer(%p{%p})::RenderToPrepare(%s)\n", this, DT, str));
  // Send the message before setting a render-to name.
  if ((DT->pditem->fsControl & DC_PREPAREITEM) && !RenderPrepare())
    return false;
  RenderToName(str);
  // Send the message after setting a render-to name.
  if ((DT->pditem->fsControl & (DC_PREPARE|DC_PREPAREITEM)) == DC_PREPARE && !RenderPrepare())
    return false;
  return true;
}


UseDragTransfer::UseDragTransfer(DRAGTRANSFER* dt, bool isrendercomplete)
: DragTransfer(dt)
{ DEBUGLOG(("UseDragTransfer(%p)::UseDragTransfer(%p, %u) - %p\n", this, dt, isrendercomplete, dt->ulTargetInfo));
  if (isrendercomplete)
  { ASSERT((long)DT->ulTargetInfo < 0);
    DT->ulTargetInfo &= 0x7fffffff;
  } else
  { ASSERT((long)DT->ulTargetInfo > 0);
  }
}

UseDragTransfer::~UseDragTransfer()
{ DEBUGLOG(("UseDragTransfer(%p{%p})::~UseDragTransfer()\n", this, DT));
  if (DT && (long)DT->ulTargetInfo > 0)
    EndConversation(false);
  DEBUGLOG(("UseDragTransfer::~UseDragTransfer done\n"));
}

void UseDragTransfer::DetachWorker()
{ DEBUGLOG(("UseDragTransfer(%p{%p})::DetachWorker() - %p\n", this, DT, DT->ulTargetInfo));
  int_ptr<HandleDragTransfers> tmp;
  tmp.fromCptr(Worker());
  DT->ulTargetInfo = 0;
}

bool UseDragTransfer::RenderPrepare()
{ DEBUGLOG(("UseDragTransfer(%p{%p})::RenderPrepare() - %p\n", this, DT, DT->ulTargetInfo));
  ASSERT((long)DT->ulTargetInfo > 0);
  if (DragTransfer::RenderPrepare())
    return true;
  DetachWorker();
  return false;
}

bool UseDragTransfer::Render()
{ DEBUGLOG(("UseDragTransfer(%p{%p})::Render() - %p\n", this, DT, DT->ulTargetInfo));
  ASSERT((long)DT->ulTargetInfo > 0);
  if (!DragTransfer::Render())
    return false;
  DT->ulTargetInfo |= 0x80000000;
  return true;
}

void UseDragTransfer::EndConversation(bool success)
{ DEBUGLOG(("UseDragTransfer(%p{%p{%p}})::EndConversation(%u) - %p\n", this, DT, DT->pditem, success, DT->ulTargetInfo));
  ASSERT((long)DT->ulTargetInfo);
  Item().EndConversation(success);
  DetachWorker();
  DT = NULL;
}


HandleDragTransfers::HandleDragTransfers(DRAGINFO* di, HWND hwnd)
: ScopedDragInfo(di)
, Iref_count(di->cditem) // set ref count at once
, DT(DrgAllocDragtransfer(di->cditem))
{ DEBUGLOG(("HandleDragTransfers(%p)::HandleDragTransfers(%p{%u...}, %p) - %p\n", this, di, di->cditem, hwnd, DT));
  PMASSERT(DT);
  OwnStrings = true;
  for (size_t i = 0; i < di->cditem; ++i)
  { DRAGTRANSFER& dt = DT[i];
    dt.cb = sizeof(DRAGTRANSFER);
    dt.hwndClient = hwnd;
    dt.pditem = DrgQueryDragitemPtr(di, i);
    dt.usOperation = di->usOperation;
    dt.ulTargetInfo = (ULONG)this; // this should read int_ptr<HandleDragTransfers>.toCptr()
  }
}

HandleDragTransfers::~HandleDragTransfers()
{ DEBUGLOG(("HandleDragTransfers(%p)::~HandleDragTransfers() - %p\n", this, DT));
  if (!DT)
    return;
  for (size_t i = 0; i < DI->cditem; ++i)
  { DRAGTRANSFER& dt = DT[i];
    DrgDeleteString(dt.hstrSelectedRMF);
    DrgDeleteString(dt.hstrRenderToName);
    dt.ulTargetInfo = 0;
  }
  DrgFreeDragtransfer(DT);
}
