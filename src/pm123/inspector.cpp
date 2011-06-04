/*
 * Copyright 2008-2010 Marcel Mueller
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


#include "inspector.h"
#include "pm123.rc.h"
#include "pm123.h"
#include "properties.h"
#include "controller.h"
#include "location.h"
#include "playable.h"
#include <os2.h>

#include <math.h>
#include <utilfct.h>
#include <snprintf.h>
#include <cpp/mutex.h>

#include <debuglog.h>


InspectorDialog::InspectorDialog()
: DialogBase(DLG_INSPECTOR, NULLHANDLE, DF_AutoResize)
{ DEBUGLOG(("InspectorDialog(%p)::InspectorDialog()\n", this));
  StartDialog(HWND_DESKTOP);
}

InspectorDialog::~InspectorDialog()
{ DEBUGLOG(("InspectorDialog(%p)::~InspectorDialog()\n", this));
  DiscardData(ControllerData);
  DiscardData(WorkerData);
}

/*void InfoDialog::StartDialog()
{ DEBUGLOG(("InfoDialog(%p)::StartDialog()\n", this));
  ManagedDialogBase::StartDialog(HWND_DESKTOP);
}*/

void InspectorDialog::SetVisible(bool show)
{ DialogBase::SetVisible(show);
  if (show)
    PMRASSERT(WinPostMsg(GetHwnd(), UM_REFRESH, 0, 0));
  else
    WinDestroyWindow(GetHwnd());
}

MRESULT InspectorDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    { BOOL ret = LONGFROMMR(DialogBase::DlgProc(msg, mp1, mp2));
      // initial position
      SWP swp;
      PMXASSERT(WinQueryTaskSizePos(amp_player_hab, 0, &swp), == 0);
      PMRASSERT(WinSetWindowPos(GetHwnd(), NULLHANDLE, swp.x,swp.y, 0,0, SWP_MOVE));
    
      do_warpsans(GetHwnd());
      sb_setnumlimits(GetHwnd(), SB_AUTOREFRESH, 30, 9999, 4);
      WinSendDlgItemMsg(GetHwnd(), SB_AUTOREFRESH, SPBM_SETCURRENTVALUE, MPFROMLONG(cfg.insp_autorefresh), 0);
      WinSendDlgItemMsg(GetHwnd(), CB_AUTOREFRESH, BM_SETCHECK, MPFROMSHORT(cfg.insp_autorefresh_on), 0);
      return MRFROMLONG(ret);
    }

   case WM_DESTROY:
    { cfg.insp_autorefresh_on = WinQueryButtonCheckstate(GetHwnd(), CB_AUTOREFRESH);
      LONG value;
      if(WinSendDlgItemMsg(GetHwnd(), SB_AUTOREFRESH, SPBM_QUERYVALUE, MPFROMP(&value), MPFROM2SHORT(0, SPBQ_DONOTUPDATE)))
        cfg.insp_autorefresh = (int)value;
    }
    break;

   case WM_COMMAND:
    DEBUGLOG2(("InspectorDialog::DlgProc:WM_CONTROL %i\n", SHORT1FROMMP(mp1)));
    switch (SHORT1FROMMP(mp1))
    {case DID_OK:
      Destroy();
      break;

     case PB_REFRESH:
      PMRASSERT(WinPostMsg(GetHwnd(), UM_REFRESH, 0, 0));
      break;
    }
    return 0;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case CB_AUTOREFRESH:
      switch (SHORT2FROMMP(mp1))
      {case BN_CLICKED:
       case BN_DBLCLICKED:
        if (WinQueryButtonCheckstate(GetHwnd(), CB_AUTOREFRESH))
          PMRASSERT(WinPostMsg(GetHwnd(), UM_REFRESH, 0, 0));
        else
          WinStopTimer(amp_player_hab, GetHwnd(), TID_INSP_AUTOREFR);
      }
      break;     
    }
    return 0;

   case WM_SYSCOMMAND:
    if (SHORT1FROMMP(mp1) == SC_CLOSE)
    { Destroy();
      return 0;
    }
    break;

   case WM_TIMER:
    switch (SHORT1FROMMP(mp1))
    { QMSG qmsg;
     case TID_INSP_AUTOREFR:
      // Avoid overruns
      WinStopTimer(amp_player_hab, GetHwnd(), TID_INSP_AUTOREFR);
      while (WinPeekMsg(amp_player_hab, &qmsg, GetHwnd(), WM_TIMER, WM_TIMER, PM_REMOVE));
      Refresh();
    }
    return 0;

   case UM_REFRESH:
    Refresh();
    return 0;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

void InspectorDialog::OnDestroy()
{ Instance = NULL;
  DialogBase::OnDestroy();
}

static void ControllerQCB(const Ctrl::ControlCommand& cmd1, void* arg)
{ DEBUGLOG(("InspectorDialog:ControllerQCB(&%p{%u,...}, %p)\n", &cmd1, cmd1.Cmd, arg));
  const Ctrl::ControlCommand* cmd = &cmd1;
  vector<char>& result = *(vector<char>*)arg;
  char buf[1024];
  buf[0] = result.size() ? '-' : '*'; // First item is the one in service.
  if (!cmd)
  { // Deadly pill or completely consumed item
    strcpy(buf+1, "NULL");
    result.append() = strdup(buf);
    return;
  }
  do
  { const char* cmdname;
    switch(cmd->Cmd)
    {case Ctrl::Cmd_Nop:
      strcpy(buf+1, "NoOp");
      break;

     case Ctrl::Cmd_Load:
      snprintf(buf+1, sizeof buf -1, "Load %s%s", cmd->Flags&1 ? "[continue] " : "", cmd->StrArg.cdata());
      break;

     case Ctrl::Cmd_Skip:
      snprintf(buf+1, sizeof buf -1, cmd->Flags&1 ? "Skip %+.0f" : "Skip %.0f", cmd->NumArg);
      break;

     case Ctrl::Cmd_Navigate:
     case Ctrl::Cmd_StopAt:
      { double loc = fabs(cmd->NumArg);
        unsigned long secs = (unsigned long)floor(loc);
        snprintf(buf+1, sizeof buf -1, "%s %s%s;%s%lu:%02lu:%02lu.%0.f",
          cmd->Cmd == Ctrl::Cmd_StopAt ? "StopAt" : "Navigate",
          cmd->Flags&1 ? (cmd->Flags&2 ? "[relative, playlist] " : "[relative] ") : (cmd->Flags&2 ? "[playlist] " : ""),
          cmd->StrArg ? cmd->StrArg.cdata() : "",
          cmd->NumArg<0 ? "-" : "", secs/3600, secs/60%60, secs%60, loc-secs);
      }
      break;

     case Ctrl::Cmd_Jump:
      { Location* loc = (Location*)cmd->PtrArg;
        snprintf(buf+1, sizeof buf -1, "Jump %s", loc->Serialize().cdata());
      }
      break;

     case Ctrl::Cmd_PlayStop:
      cmdname = "Play";
      goto flagcmd;
     case Ctrl::Cmd_Pause:
      cmdname = "Pause";
      goto flagcmd;
     case Ctrl::Cmd_Scan:
      cmdname = "Scan";
      goto flagcmd;
     case Ctrl::Cmd_Shuffle:
      cmdname = "Shuffle";
      goto flagcmd;
     case Ctrl::Cmd_Repeat:
      cmdname = "Repeat";
     flagcmd:
      { static const char flags[4][8] = { "reset", "on", "off", "toggle" };
        snprintf(buf+1, sizeof buf -1, "%s %s%s", cmdname,
          cmd->Cmd == Ctrl::Cmd_Scan && cmd->Flags&4 ? "rewind " : "", flags[cmd->Flags&3]);
      }
      break;
     
     case Ctrl::Cmd_Volume:
      snprintf(buf+1, sizeof buf -1, "Volume %f", cmd->NumArg);
      break;

     case Ctrl::Cmd_Save:
      snprintf(buf+1, sizeof buf -1, "Save %s", cmd->StrArg ? cmd->StrArg.cdata() : "off");
      break;
      
     case Ctrl::Cmd_Location:
      strcpy(buf+1, "LocationQuery");
      break;
             
     case Ctrl::Cmd_DecStop:
      strcpy(buf+1, "DecoderStop");
      break;
      
     case Ctrl::Cmd_OutStop:
      strcpy(buf+1, "OutputStop");
      break;
    }
    buf[sizeof buf-1] = 0;
    result.append() = strdup(buf);
    // next
    cmd = cmd->Link;
    buf[0] = '+';
  } while (cmd);
}

static void PlayableFlagsMapper(char* str, InfoFlags flags, const char* tpl)
{ static const InfoFlags flaglist[] =
  { IF_Phys, IF_Tech, IF_Obj, IF_Meta, IF_Attr, IF_Child,
    IF_Rpl, IF_Drpl, IF_Item, IF_Slice };
  for (size_t i = 0; i < sizeof(flaglist)/sizeof(*flaglist); ++i)
    if (flags & flaglist[i])
      str[i] = tpl[i];
}

static void WorkerQCB(APlayable* entry, Priority pri, bool svc, void* arg)
{ DEBUGLOG(("InspectorDialog:WorkerQCB(%p, %u, %u, %p)\n", entry, pri, svc, arg));
  vector<char>& result = *(vector<char>*)arg;
  if (entry == NULL)
  { result.append() = strdup(" NULL");
  } else
  { char buf[1024];
    RequestState req;
    entry->PeekRequest(req);
    DEBUGLOG(("InspectorDialog:WorkerQCB %x,%x, %x\n", req.ReqLow, req.ReqHigh, req.InService));
    static const char prefixchar[2][2] = { { '-', '=' }, { '+', '#' } };
    buf[0] = prefixchar[svc][pri == PRI_Normal];
    char rqstr[11] = "----------";
    PlayableFlagsMapper(rqstr, req.ReqLow, "ptomacrdis");
    PlayableFlagsMapper(rqstr, req.ReqHigh, "PTOMACRDIS");
    char isstr[11] = "----------";
    PlayableFlagsMapper(isstr, req.InService, "PTOMACRDIS");
    snprintf(buf+1, sizeof buf -1, "[%s -> %s] %s", rqstr, isstr, entry->GetPlayable().URL.cdata());
    result.append() = strdup(buf);
  }
}

void InspectorDialog::Refresh()
{ DEBUGLOG(("InspectorDialog::Refresh()\n"));

  // Refresh controller Q
  HWND lb = WinWindowFromID(GetHwnd(), LB_CONTROLLERQ);
  PMASSERT(lb != NULLHANDLE);
  PMRASSERT(WinSendMsg(lb, LM_DELETEALL, 0, 0));
  // Discard old data
  DiscardData(ControllerData);
  // Retrieve data
  Ctrl::QueueTraverse(&ControllerQCB, &ControllerData);
  // refresh listbox
  { LBOXINFO lbi = { 0, ControllerData.size() };
    WinSendMsg(lb, LM_INSERTMULTITEMS, MPFROMP(&lbi), MPFROMP(ControllerData.begin()));
  }

  // Refresh Worker Q
  lb = WinWindowFromID(GetHwnd(), LB_WORKERQ);
  PMRASSERT(WinSendMsg(lb, LM_DELETEALL, 0, 0));
  // Discard old data
  DiscardData(WorkerData);
  // Retrieve data
  Playable::QueueTraverse(&WorkerQCB, &WorkerData);
  // refresh listbox
  { LBOXINFO lbi = { 0, WorkerData.size() };
    WinSendMsg(lb, LM_INSERTMULTITEMS, MPFROMP(&lbi), MPFROMP(WorkerData.begin()));
  }

  // Next refresh
  LONG value;
  if ( WinQueryButtonCheckstate(GetHwnd(), CB_AUTOREFRESH)
    && WinSendDlgItemMsg(GetHwnd(), SB_AUTOREFRESH, SPBM_QUERYVALUE, MPFROMP(&value), MPFROM2SHORT(0, SPBQ_DONOTUPDATE)) )
    PMXASSERT(WinStartTimer(amp_player_hab, GetHwnd(), TID_INSP_AUTOREFR, value), != 0);
}

void InspectorDialog::DiscardData(vector<char>& data)
{ for (char*const* cpp = data.begin(); cpp != data.end(); ++cpp)
    free(*cpp);
  data.clear();
} 


volatile int_ptr<InspectorDialog> InspectorDialog::Instance;

int_ptr<InspectorDialog> InspectorDialog::GetInstance()
{ DEBUGLOG(("InspectorDialog::GetInstance()\n"));
  int_ptr<InspectorDialog> inst = Instance;
  if (inst == NULL)
  { CritSect cs;
    inst = Instance; // double check
    if (inst == NULL)
      Instance = inst = new InspectorDialog();
  }
  return inst;
}

void InspectorDialog::UnInit()
// close inspector
{ int_ptr<InspectorDialog> inst = Instance;
  if (inst)
    inst->SetVisible(false);
}
