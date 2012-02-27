/*
 * Copyright 2011 Marcel Mueller
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
#define  INCL_PM

#include "dialog.h"
#include "pulse123.h"
#include "configuration.h"
#include <plugin.h>
#include <utilfct.h>
#include <cpp/pmutils.h>
#include <os2.h>

#include <debuglog.h>


ConfigDialog::PlaybackPage::PlaybackPage(ConfigDialog& parent)
: PageBase(parent, DLG_PLAYBACK, parent.ResModule, DF_AutoResize)
, PlaybackServer(Configuration.PlaybackServer)
, StateChangeDeleg(Context.StateChange(), *this, &PlaybackPage::StateChangeHandler)
, ServerInfoDeleg(ServerInfoOp.Info(), *this, &PlaybackPage::ServerInfoHandler)
, SinkInfoDeleg(SinkInfoOp.Info(), *this, &PlaybackPage::SinkInfoHandler)
{ MajorTitle = "Playback";
}

void ConfigDialog::PlaybackPage::Cleanup()
{ SinkInfoOp.Cancel();
  ServerInfoOp.Cancel();
  Context.Disconnect();
}

ConfigDialog::PlaybackPage::~PlaybackPage()
{ Cleanup();
}

ConfigDialog::RecordPage::RecordPage(ConfigDialog& parent)
: PageBase(parent, DLG_RECORD, parent.ResModule, DF_AutoResize)
{ MajorTitle = "Record";
}

ConfigDialog::ConfigDialog(HWND owner, HMODULE module)
: NotebookDialogBase(DLG_CONFIG, module, DF_AutoResize)
{ Pages.append() = new PlaybackPage(*this);
  Pages.append() = new RecordPage(*this);
  StartDialog(owner, NB_CONFIG);
}

MRESULT ConfigDialog::PlaybackPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {
   case WM_INITDLG:
    do_warpsans(GetHwnd());
    { // Populate MRU list
      HWND ctrl = GetDlgItem(CB_PBSERVER);
      WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(PlaybackServer.cdata()));
      char key[] = "PlaybackServer1";
      do
      { xstring url;
        ini_query(key, url);
        if (!url)
          break;
        WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(url.cdata()));
      } while (++key[sizeof key -2] <= '9');
      // Set current value
      if (PlaybackServer)
      { PMRASSERT(WinSetWindowText(ctrl, PlaybackServer.cdata()));
        PostMsg(UM_CONNECT, 0, 0);
      }
    }
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case DID_OK:
      { HWND ctrl = GetDlgItem(CB_PBSERVER);
        PlaybackServer = WinQueryWindowXText(ctrl);
        ini_save(PlaybackServer);
        // update MRU list
        if (PlaybackServer && *PlaybackServer)
        { char key[] = "PlaybackServer1";
          int i = 0;
          int len = 0;
          xstring url;
          do
          { if (len >= 0)
            {skip:
              url.reset();
              len = (SHORT)SHORT1FROMMR(WinSendMsg(ctrl, LM_QUERYITEMTEXTLENGTH, MPFROMSHORT(i), 0));
              if (len >= 0)
              { WinSendMsg(ctrl, LM_QUERYITEMTEXT, MPFROM2SHORT(i, len+1), MPFROMP(url.allocate(len)));
                DEBUGLOG(("pulse123:cfg_dlg_proc save MRU %i: (%i) %s\n", i, len, url.cdata()));
                ++i;
                if (url == PlaybackServer)
                  goto skip;
              }
            }
            ini_write(key, url);
          } while (++key[sizeof key -2] <= '9');
        }
      }
      break;
    }
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case CB_PBSERVER:
      switch (SHORT2FROMMP(mp1))
      {case CBN_ENTER:
        if (PlaybackServer.cmpassign(WinQueryWindowXText(HWNDFROMMP(mp2))))
          PostMsg(UM_CONNECT, 0,0);
        break;
      }
      break;
    }
    break;

   case UM_CONNECT:
    { DEBUGLOG(("ConfigDialog::PlaybackPage::DlgProc:UM_CONNECT\n"));
      // destroy any old connection
      Cleanup();
      // open new connection
      try
      { Context.Connect("PM123", PlaybackServer);
      } catch (const PAException& ex)
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, ex.GetMessage());
      }
    }
    return 0;

   case UM_STATE_CHANGE:
    { pa_context_state_t state = (pa_context_state_t)LONGFROMMP(mp1);
      DEBUGLOG(("ConfigDialog::PlaybackPage::DlgProc:UM_STATE_CHANGE %u\n", state));
      const char* text = "";
      switch (state)
      {case PA_CONTEXT_CONNECTING:
        text = "Connecting ...";
        break;
       case PA_CONTEXT_AUTHORIZING:
        text = "Authorizing ...";
        break;
       case PA_CONTEXT_SETTING_NAME:
        text = "Set name ...";
        break;
       case PA_CONTEXT_READY:
        text = "Connected";
        PostMsg(UM_DISCOVER_SERVER, 0,0);
        break;
       case PA_CONTEXT_FAILED:
        WinSetDlgItemText(GetHwnd(), ST_STATUS, PAConnectException(Context.GetContext()).GetMessage());
        return 0;
       case PA_CONTEXT_TERMINATED:
        text = "Closed";
        break;
       case PA_CONTEXT_UNCONNECTED:;
      }
      WinSetDlgItemText(GetHwnd(), ST_STATUS, text);
      return 0;
    }

   case UM_DISCOVER_SERVER:
    { DEBUGLOG(("ConfigDialog::PlaybackPage::DlgProc:UM_DISCOVER_SERVER\n"));
      try
      { Context.GetServerInfo(ServerInfoOp);
        Sinks.clear();
        Context.GetSinkInfo(SinkInfoOp);
      } catch (const PAException& ex)
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, ex.GetMessage());
      }
      return 0;
    }

   case UM_UPDATE_SERVER:
    { int error = LONGFROMMP(mp1);
      DEBUGLOG(("ConfigDialog::PlaybackPage::DlgProc:UM_UPDATE_SERVER %i\n", error));
      if (error)
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, PAConnectException(Context.GetContext(), error).GetMessage());
        return 0;
      }
      WinSetDlgItemText(GetHwnd(), ST_STATUS, "Success");
      HWND ctrl = GetDlgItem(CB_SINK);
      // save old value
      xstring oldsink = WinQueryWindowXText(ctrl);
      // delete old list
      PMRASSERT(WinSendMsg(ctrl, LM_DELETEALL, 0, 0));
      // insert new list and restore old value if reasonable.
      xstring def;
      //def.sprintf();
      PMXASSERT(WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP("default")), >= 0);
      /* LM_INSERTMULTITEMS seems not to work.
      LBOXINFO insert = { 0, Sinks.size() };
      // HACK: Sinks is a list of pointers to PASinkInfo.
      // PASinkInfo starts with an xstring name. xsting is binary compatible to const char*.
      // So Sinks.begin() is compatible to const char** as required by LM_INSERTMULTITEMS.
      PMXASSERT(WinSendMsg(ctrl, LM_INSERTMULTITEMS, MPFROMP(&insert), MPFROMP(Sinks.begin())), >= 0);*/
      if (Sinks.size() != 0)
      { unsigned selected = 0;
        for (unsigned i = 0; i < Sinks.size(); ++i)
        { PMXASSERT(WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(Sinks[i]->name.cdata())), >= 0);
          if (!i && Sinks[i]->name.compareToI(oldsink) == 0)
          { selected = i+1;
            PMRASSERT(WinSendMsg(ctrl, LM_SELECTITEM, MPFROMSHORT(selected), MPFROMSHORT(TRUE)));
          }
        }
        // Otherwise set new default
        if (selected < 0)
          PMRASSERT(WinSetWindowText(ctrl, Server.default_sink_name));
      }
      PostMsg(UM_UPDATE_PORT, 0,0);
      return 0;
    }
   case UM_UPDATE_PORT:
    { DEBUGLOG(("ConfigDialog::PlaybackPage::DlgProc:UM_UPDATE_PORT\n"));
      HWND ctrl = GetDlgItem(CB_PORT);
      // save old value
      xstring oldport = WinQueryWindowXText(ctrl);
      // delete old list
      PMRASSERT(WinSendMsg(ctrl, LM_DELETEALL, 0, 0));
      // insert new list and restore old value if reasonable.
      if (Sinks.size() != 0)
      { int selected = -1;
        for (unsigned i = 0; i < Sinks.size(); ++i)
        { PMXASSERT(WinSendMsg(ctrl, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(Sinks[i]->name.cdata())), >= 0);
          if (i < 0 && Sinks[i]->name.compareToI(oldsink) == 0)
          { PMRASSERT(WinSendMsg(ctrl, LM_SELECTITEM, MPFROMSHORT(i), MPFROMSHORT(TRUE)));
            selected = i;
          }
        }
        // Otherwise set new default
        if (selected < 0)
          PMRASSERT(WinSetWindowText(ctrl, Server.default_sink_name));
      }
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

void ConfigDialog::PlaybackPage::StateChangeHandler(const pa_context_state_t& args)
{ DEBUGLOG(("ConfigDialog::PlaybackPage(%p)::StateChangeHandler(%i)\n", this, args));
  PostMsg(UM_STATE_CHANGE, MPFROMLONG(args), 0);
}

void ConfigDialog::PlaybackPage::ServerInfoHandler(const pa_server_info& info)
{ DEBUGLOG(("ConfigDialog::PlaybackPage(%p)::ServerInfoHandler(%p)\n", this, info));
  if (!&info)
  { SinkInfoOp.Cancel();
    PostMsg(UM_UPDATE_SERVER, MPFROMLONG(Context.Errno()), 0);
  } else
  { Server = info;
    if (SinkInfoOp.GetState() == PA_OPERATION_DONE)
      PostMsg(UM_UPDATE_SERVER, 0, 0);
  }
}

void ConfigDialog::PlaybackPage::SinkInfoHandler(const PASinkInfoOperation::Args& args)
{ DEBUGLOG(("ConfigDialog::PlaybackPage(%p)::SinkInfoHandler({%p, %i})\n", this, args.Info, args.Error));
  if (args.Info)
  { Sinks.append() = new PASinkInfo(*args.Info);
    return;
  }
  if (args.Error)
    ServerInfoOp.Cancel();
  else if (ServerInfoOp.GetState() != PA_OPERATION_DONE)
    return;
  PostMsg(UM_UPDATE_SERVER, MPFROMLONG(args.Error), 0);
}

MRESULT ConfigDialog::RecordPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return PageBase::DlgProc(msg, mp1, mp2);
}

MRESULT ConfigDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  //DEBUGLOG(("ConfigDialog::PlaybackPage::DlgProc(%u, %p, %p)\n", msg, mp1, mp2));
  switch (msg)
  {
   case WM_INITDLG:
    do_warpsans(GetHwnd());
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case DID_OK:
      // propagate to all pages
      foreach(PageBase*const*, page, Pages)
        WinSendMsg((*page)->GetHwnd(), msg, mp1, mp2);
      break;
    }
    break;
  }
  return NotebookDialogBase::DlgProc(msg, mp1, mp2);
}
