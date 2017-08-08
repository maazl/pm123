/*
 * metawritedlg.cpp
 *
 *  Created on: 08.08.2017
 *      Author: mueller
 */

#include "metawritedlg.h"
#include "pm123.rc.h"
#include "../core/playable.h"
#include "../eventhandler.h"
#include <utilfct.h>


MetaWriteDlg::MetaWriteDlg(const vector<APlayable>& dest)
: DialogBase(DLG_WRITEMETA, NULLHANDLE)
, MetaFlags(DECODER_HAVE_NONE)
, Dest(dest)
, SkipErrors(false)
, WorkerTID(0)
, Cancel(false)
{ DEBUGLOG(("MetaWriteDlg(%p)::MetaWriteDlg({%u,})\n", this, dest.size()));
}

MetaWriteDlg::~MetaWriteDlg()
{ DEBUGLOG(("MetaWriteDlg(%p)::~MetaWriteDlg() - %u\n", this, WorkerTID));
  // Wait for the worker thread to complete (does not hold the SIQ)
  if (WorkerTID)
  { Cancel = true;
    ResumeSignal.Set();
    wait_thread_pm(amp_player_hab, WorkerTID, 5*60*1000);
  }
}

void TFNENTRY InfoDialogMetaWriteWorkerStub(void* arg)
{ ((MetaWriteDlg*)arg)->Worker();
}

MRESULT MetaWriteDlg::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    // setup first destination
    EntryField(+GetCtrl(EF_WMURL)).Text(Dest[0]->GetPlayable().URL);
    PostMsg(UM_START, 0, 0);
    break;

   case UM_START:
    DEBUGLOG(("MetaWriteDlg(%p)::DlgProc: UM_START\n", this));
    CurrentItem = 0;
    WorkerTID = _beginthread(&InfoDialogMetaWriteWorkerStub, NULL, 65536, this);
    ASSERT(WorkerTID != (TID)-1);
    return 0;

   case UM_STATUS:
    { DEBUGLOG(("MetaWriteDlg(%p)::DlgProc: UM_STATUS %i, %i\n", this, mp1, mp2));
      int i = LONGFROMMP(mp1);
      if (Cancel)
      { // Terminate Signal
        WinDismissDlg(GetHwnd(), DID_OK);
        return 0;
      }
      // else
      { // Update progress bar
        SWP swp;
        PMRASSERT(WinQueryWindowPos(GetCtrl(SB_WMBARBG), &swp));
        swp.cx = swp.cx * (i+1) / Dest.size();
        PMRASSERT(WinSetWindowPos(GetCtrl(SB_WMBARFG), NULLHANDLE, 0,0, swp.cx,swp.cy, SWP_SIZE));
      }

      if (RC == PLUGIN_OK)
      { // Everything fine, next item
        EntryField(+GetCtrl(EF_WMSTATUS)).Text("");
        ++i; // next item (if any)
        EntryField(+GetCtrl(EF_WMURL)).Text((size_t)i < Dest.size() ? Dest[i]->GetPlayable().URL.cdata() : "");
        return 0;
      }
      // Error, worker halted
      xstring errortext(Text);
      errortext.sprintf("Error %lu - %s", RC, errortext.cdata());
      PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_WMSTATUS, errortext));
      // 'skip all' was pressed? => continue immediately
      if (SkipErrors)
      { ResumeSignal.Set();
        return 0;
      }
      // Enable skip buttons
      EnableCtrl(PB_RETRY,     true);
      EnableCtrl(PB_WMSKIP,    true);
      EnableCtrl(PB_WMSKIPALL, true);
      return 0;
    }

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case DID_CANCEL:
      Cancel = true;
      ResumeSignal.Set();
      EntryField(+GetCtrl(EF_WMSTATUS)).Text("- cancelling -");
      EnableCtrl(DID_CANCEL, false);
      break;

     case PB_WMSKIPALL:
      SkipErrors = true;
     case PB_WMSKIP:
      // next item
      ++CurrentItem;
     case PB_RETRY:
      // Disable skip buttons
      EnableCtrl(PB_RETRY,     false);
      EnableCtrl(PB_WMSKIP,    false);
      EnableCtrl(PB_WMSKIPALL, false);
      ResumeSignal.Set();
    }
    return 0;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

ULONG MetaWriteDlg::DoDialog(HWND owner)
{ DEBUGLOG(("MetaWriteDlg(%p)::DoDialog() - %u %x\n", this, Dest.size(), MetaFlags));
  if (Dest.size() == 0 || MetaFlags == DECODER_HAVE_NONE)
    return DID_OK;
  StartDialog(owner);
  return WinProcessDlg(GetHwnd());
}

void DLLENTRY InfoDialogMetaWriteErrorHandler(MetaWriteDlg* that, MESSAGE_TYPE type, const xstring& msg)
{ that->Text = msg;
}

void MetaWriteDlg::Worker()
{
  // Catch error messages
  VDELEGATE vdErrorHandler;
  EventHandler::Handler oldHandler = EventHandler::SetLocalHandler(vdErrorHandler.assign(&InfoDialogMetaWriteErrorHandler, this));

  while (!Cancel && CurrentItem < Dest.size())
  { Playable& song = Dest[CurrentItem]->GetPlayable();
    DEBUGLOG(("MetaWriteDlg(%p)::Worker - %u %s\n", this, CurrentItem, song.DebugName().cdata()));
    // Write info
    Text.reset();
    RC = song.SaveMetaInfo(MetaData, MetaFlags);
    // Notify dialog
    PostMsg(UM_STATUS, MPFROMLONG(CurrentItem), 0);
    if (Cancel)
      return;
    // wait for acknowledge
    if (RC != PLUGIN_OK)
    { ResumeSignal.Wait();
      ResumeSignal.Reset();
    } else
      ++CurrentItem;
  }
  Cancel = true;
  // restore error handler
  EventHandler::SetLocalHandler(oldHandler);
  PostMsg(UM_STATUS, MPFROMLONG(-1), 0);
  DEBUGLOG(("MetaWriteDlg(%p)::Worker completed\n", this));
}
