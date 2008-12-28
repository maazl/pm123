/*
 * Copyright 2008-2008 Marcel Mueller
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
#include "songiterator.h"
#include "properties.h"
#include <os2.h>

#include <math.h>
#include <utilfct.h>
#include <snprintf.h>
#include <cpp/Mutex.h>

#include <debuglog.h>


InspectorDialog::InspectorDialog()
: ManagedDialogBase(DLG_INSPECTOR, NULLHANDLE)
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
{ ManagedDialogBase::SetVisible(show);
  if (show)
    PMRASSERT(WinPostMsg(GetHwnd(), UM_REFRESH, 0, 0));
}

MRESULT InspectorDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    { BOOL ret = LONGFROMMR(ManagedDialogBase::DlgProc(msg, mp1, mp2));
      // initial position
      SWP swp;
      PMXASSERT(WinQueryTaskSizePos(amp_player_hab(), 0, &swp), == 0);
      PMRASSERT(WinSetWindowPos(GetHwnd(), NULLHANDLE, swp.x,swp.y, 0,0, SWP_MOVE));
    
      do_warpsans(GetHwnd());
      WinSendDlgItemMsg(GetHwnd(), SB_AUTOREFRESH, SPBM_SETLIMITS, MPFROMLONG(9999), MPFROMLONG(20));
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
          WinStopTimer(amp_player_hab(), GetHwnd(), TID_INSP_AUTOREFR);
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
      WinStopTimer(amp_player_hab(), GetHwnd(), TID_INSP_AUTOREFR);
      while (WinPeekMsg(amp_player_hab(), &qmsg, GetHwnd(), WM_TIMER, WM_TIMER, PM_REMOVE));
      Refresh();
    }
    return 0;

   case UM_REFRESH:
    Refresh();
    return 0;
  }
  return ManagedDialogBase::DlgProc(msg, mp1, mp2);
}

void InspectorDialog::OnDestroy()
{ { CritSect cs;
    Instance = NULL;
  }
  ManagedDialogBase::OnDestroy();
}

static void ControllerQCB(const queue<Ctrl::ControlCommand*>::qentry& entry, void* arg)
{ DEBUGLOG(("InspectorDialog:ControllerQCB(&%p, %p)\n", &entry, arg));
  char buf[1024];
  buf[0] = entry.ReadActive ? '*' : '-';
  const Ctrl::ControlCommand* cmd = entry.Data;
  vector<char>& result = *(vector<char>*)arg;
  if (!cmd)
  { // Deadly pill or completely consumed item
    strcpy(buf+1, "NULL");
    result.append() = strdup(buf);
    return;
  }
  do
  { const char* cmdname;
    int len;
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
      { SongIterator* si = (SongIterator*)cmd->PtrArg;
        snprintf(buf+1, sizeof buf -1, "Jump %s", si->Serialize().cdata());
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
      { static const char* flags[4][8] = { "reset", "on", "off", "toggle" };
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

static void PlayableFlagsMapper(char* str, Playable::InfoFlags flags, const char* tpl)
{ if (flags & Playable::IF_Format)
    str[0] = tpl[0];
  if (flags & Playable::IF_Tech)
    str[1] = tpl[1];
  if (flags & Playable::IF_Meta)
    str[2] = tpl[2];
  if (flags & Playable::IF_Phys)
    str[3] = tpl[3];
  if (flags & Playable::IF_Rpl)
    str[4] = tpl[4];
  if (flags & Playable::IF_Other)
    str[5] = tpl[5];
}

static void WorkerQCB(const queue<Playable::QEntry>::qentry& entry, void* arg)
{ DEBUGLOG(("InspectorDialog:WorkerQCB(&%p, %p)\n", &entry, arg));
  char buf[1024];
  buf[0] = entry.ReadActive ? '*' : '-';
  Playable* pp = entry.Data;
  if (pp == NULL)
  { strcpy(buf+1, "NULL");
  } else
  { Playable::InfoFlags low;
    Playable::InfoFlags high;
    Playable::InfoFlags insvc;
    pp->QueryRequestState(low, high, insvc);
    char rqstr[7] = "------";
    PlayableFlagsMapper(rqstr, low, "ftmpro");
    PlayableFlagsMapper(rqstr, high, "FTMPRO");
    char isstr[7] = "------";
    PlayableFlagsMapper(isstr, insvc, "FTMPRO");
    snprintf(buf+1, sizeof buf -1, "[%s -> %s] %s", rqstr, isstr, pp->GetURL().cdata());
  }
  vector<char>& result = *(vector<char>*)arg;
  result.append() = strdup(buf);
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
    PMXASSERT(WinStartTimer(amp_player_hab(), GetHwnd(), TID_INSP_AUTOREFR, value), != 0);
}

void InspectorDialog::DiscardData(vector<char>& data)
{ for (char*const* cpp = data.begin(); cpp != data.end(); ++cpp)
    free(*cpp);
  data.clear();
} 


int_ptr<InspectorDialog> InspectorDialog::Instance;

int_ptr<InspectorDialog> InspectorDialog::GetInstance()
{ DEBUGLOG(("InspectorDialog::GetInstance()\n"));
  CritSect cs;
  if (Instance == NULL)
    Instance = new InspectorDialog();
  return Instance;
}

