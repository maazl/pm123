/*
 * Copyright 2011-2012 Marcel Mueller
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
#include <cpp/dlgcontrols.h>
#include <cpp/algorithm.h>
#include <os2.h>

#include <debuglog.h>


IntrospectBase::IntrospectBase(USHORT rid, HMODULE module)
: DialogBase(rid, module, DF_AutoResize)
, StateChangeDeleg(Context.StateChange(), *this, &IntrospectBase::StateChangeHandler)
, ServerInfoDeleg(ServerInfoOp.Info(), *this, &IntrospectBase::ServerInfoHandler)
{}

IntrospectBase::~IntrospectBase()
{ Cleanup();
}

void IntrospectBase::StateChangeHandler(const pa_context_state_t& args)
{ DEBUGLOG(("IntrospectBase(%p)::StateChangeHandler(%i)\n", this, args));
  PostMsg(UM_STATE_CHANGE, MPFROMLONG(args), 0);
}

void IntrospectBase::ServerInfoHandler(const pa_server_info& info)
{ DEBUGLOG(("IntrospectBase(%p)::ServerInfoHandler(%p)\n", this, info));
  if (!&info)
  { InfoOp->Cancel();
    PostMsg(UM_UPDATE_SERVER, MPFROMLONG(Context.Errno()), 0);
  } else
  { Server = info;
    if (InfoOp->GetState() == PA_OPERATION_DONE)
      PostMsg(UM_UPDATE_SERVER, 0, 0);
  }
}

MRESULT IntrospectBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    { MRESULT ret = DialogBase::DlgProc(msg, mp1, mp2);
      do_warpsans(GetHwnd());
      // Populate MRU list
      ComboBox cb(GetCtrl(CB_SERVER));
      cb.InsertItem(Configuration.SinkServer);
      char key[] = "Server1";
      do
      { xstring url;
        ini_query(key, url);
        if (!url)
          break;
        cb.InsertItem(url);
      } while (++key[sizeof key -2] <= '9');
      return ret;
    }

   case WM_COMMAND:
    DEBUGLOG(("IntrospectBase::DlgProc:WM_COMMAND(%i,%i, %p)\n", SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), mp2));
    switch (SHORT1FROMMP(mp1))
    {case PB_UPDATE:
      PostMsg(UM_CONNECT, 0,0);
      return 0;
     case DID_OK:
      { ComboBox cb(GetCtrl(CB_SERVER));
        const xstring& server = cb.Text();
        Configuration.SinkKeepAlive = WinQueryButtonCheckstate(GetHwnd(), CB_PBKEEP);
        // update MRU list
        if (server.length())
        { char key[] = "Server1";
          int i = 0;
          int len = 0;
          xstring url;
          do
          { if (len >= 0)
            {skip:
              url.reset();
              len = (SHORT)SHORT1FROMMR(WinSendMsg(cb.Hwnd, LM_QUERYITEMTEXTLENGTH, MPFROMSHORT(i), 0));
              if (len >= 0)
              { WinSendMsg(cb.Hwnd, LM_QUERYITEMTEXT, MPFROM2SHORT(i, len+1), MPFROMP(url.allocate(len)));
                DEBUGLOG(("IntrospectBase::DlgProc: save MRU %i: (%i) %s\n", i, len, url.cdata()));
                ++i;
                if (url == server)
                  goto skip;
              }
            }
            ini_write(key, url);
          } while (++key[sizeof key -2] <= '9');
        }
        Configuration.Save();
      }
      break;
    }
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case CB_SERVER:
      switch (SHORT2FROMMP(mp1))
      {case CBN_ENTER:
        PostMsg(UM_CONNECT, 0,0);
        break;
      }
      break;
     case CB_SINKSRC:
      switch (SHORT2FROMMP(mp1))
      {case CBN_ENTER:
        PostMsg(UM_UPDATE_PORT, 0,0);
        break;
      }
    }
    break;

   case UM_CONNECT:
    { DEBUGLOG(("IntrospectBase::DlgProc:UM_CONNECT\n"));
      // destroy any old connection
      Cleanup();
      const xstring& server = WinQueryDlgItemXText(GetHwnd(), CB_SERVER);
      if (!server.length())
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, "enter server name above");
        return 0;
      }
      // open new connection
      try
      { Context.Connect("PM123", server);
      } catch (const PAException& ex)
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, ex.GetMessage());
      }
      return 0;
    }

   case UM_STATE_CHANGE:
    { pa_context_state_t state = (pa_context_state_t)LONGFROMMP(mp1);
      DEBUGLOG(("IntrospectBase::DlgProc:UM_STATE_CHANGE %u\n", state));
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
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

void IntrospectBase::Cleanup()
{ InfoOp->Cancel();
  ServerInfoOp.Cancel();
  Context.Disconnect();
}


ConfigDialog::ConfigDialog(HWND owner, HMODULE module)
: IntrospectBase(DLG_CONFIG, module)
, SinkInfoDeleg(SinkInfoOp.Info(), *this, &ConfigDialog::SinkInfoHandler)
{ InfoOp = &SinkInfoOp;
  StartDialog(owner, HWND_DESKTOP);
}

MRESULT ConfigDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    { MRESULT ret = IntrospectBase::DlgProc(msg, mp1, mp2);
      WinCheckButton(GetHwnd(), CB_PBKEEP, Configuration.SinkKeepAlive);
      if (Configuration.Sink)
        PMRASSERT(WinSetDlgItemText(GetHwnd(), CB_SINKSRC, Configuration.Sink));
      if (Configuration.SinkPort)
        PMRASSERT(WinSetDlgItemText(GetHwnd(), CB_PORT, Configuration.SinkPort));
      // Set current value
      if (Configuration.SinkServer)
      { ComboBox(+GetCtrl(CB_SERVER)).Text(Configuration.SinkServer);
        PostMsg(UM_CONNECT, 0, 0);
      }
      SpinButton sb(GetCtrl(SB_MINLATENCY));
      sb.SetLimits(0, 5000, 4);
      sb.Value(Configuration.SinkMinLatency);
      sb = SpinButton(GetCtrl(SB_MAXLATENCY));
      sb.SetLimits(100, 10000, 4);
      sb.Value(Configuration.SinkMaxLatency);
      return ret;
    }

   case WM_COMMAND:
    DEBUGLOG(("ConfigDialog::DlgProc:WM_COMMAND(%i,%i, %p)\n", SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), mp2));
    switch (SHORT1FROMMP(mp1))
    {case DID_OK:
      { Configuration.SinkServer = WinQueryDlgItemXText(GetHwnd(), CB_SERVER);
        Configuration.SinkKeepAlive = WinQueryButtonCheckstate(GetHwnd(), CB_PBKEEP);
        const xstring& sink = WinQueryDlgItemXText(GetHwnd(), CB_SINKSRC);
        Configuration.Sink = sink.length() && !sink.startsWithI("default") ? sink : xstring();
        const xstring& port = WinQueryDlgItemXText(GetHwnd(), CB_PORT);
        Configuration.SinkPort = port.length() && !port.startsWithI("default") ? port : xstring();
        Configuration.SinkMinLatency = SpinButton(GetCtrl(SB_MINLATENCY)).Value();
        Configuration.SinkMaxLatency = SpinButton(GetCtrl(SB_MAXLATENCY)).Value();
      }
      break;
    }
    break;

   case UM_DISCOVER_SERVER:
    { DEBUGLOG(("ConfigDialog::DlgProc:UM_DISCOVER_SERVER\n"));
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
      DEBUGLOG(("ConfigDialog::DlgProc:UM_UPDATE_SERVER %i\n", error));
      if (error)
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, PAConnectException(Context.GetContext(), error).GetMessage());
        return 0;
      }
      WinSetDlgItemText(GetHwnd(), ST_STATUS, "Success");
      ComboBox cb(GetCtrl(CB_SINKSRC));
      // save old value
      const xstring& oldsink = cb.Text();
      // delete old list
      cb.DeleteAll();
      SelectedSink = -1;
      // insert new list and restore old value if reasonable.
      xstring def;
      def.sprintf("default (%s)", Server.default_sink_name.cdata());
      cb.InsertItem(def);
      if (Sinks.size() != 0)
      { int defsink = -1;
        for (unsigned i = 0; i < Sinks.size(); ++i)
        { PASinkInfo& sink = *Sinks[i];
          cb.InsertItem(sink.name);
          if (SelectedSink < 0 && sink.name.compareToI(oldsink) == 0)
            SelectedSink = i;
          if (defsink < 0 && sink.name.compareToI(Server.default_sink_name) == 0)
            defsink = i;
        }
        cb.Select(SelectedSink+1);
        if (SelectedSink < 0)
          SelectedSink = defsink;
      }
    }
   case UM_UPDATE_PORT:
    { DEBUGLOG(("ConfigDialog::DlgProc:UM_UPDATE_PORT %i\n", SelectedSink));
      ComboBox cb(GetCtrl(CB_PORT));
      // save old value
      const xstring& oldport = cb.Text();
      // delete old list
      cb.DeleteAll();
      // insert new list and restore old value if reasonable.
      xstring def;
      int selected = -1;
      if ((unsigned)SelectedSink < Sinks.size())
      { PASinkInfo& sink = *Sinks[SelectedSink];
        if (sink.active_port)
          def.sprintf("default (%s)", sink.active_port->name.cdata());
        for (unsigned i = 0; i < sink.ports.size(); ++i)
        { PAPortInfo& port = sink.ports[i];
          cb.InsertItem(port.name);
          if (selected < 0 && port.name.compareToI(oldport) == 0)
            selected = i;
        }
      }
      cb.InsertItem(def ? def.cdata() : "default", 0);
      cb.Select(selected+1);
      return 0;
    }
  }
  return IntrospectBase::DlgProc(msg, mp1, mp2);
}

void ConfigDialog::SinkInfoHandler(const PASinkInfoOperation::Args& args)
{ DEBUGLOG(("ConfigDialog(%p)::SinkInfoHandler({%p, %i})\n", this, args.Info, args.Error));
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


LoadWizard::LoadWizard(HMODULE module, HWND owner, const xstring& title)
: IntrospectBase(DLG_RECORD, module)
, Title(title)
, SourceInfoDeleg(SourceInfoOp.Info(), *this, &LoadWizard::SourceInfoHandler)
{ InfoOp = &SourceInfoOp;
  StartDialog(owner, HWND_DESKTOP);
}

static const char* SamplingRates[] =
{ "8000", "11025", "12000", "16000", "22050", "24000", "32000", "44100", "48000", "96000" };

static int SamplingRateCmp(int* key, const char* elem)
{ return *key - atoi(elem);
}

MRESULT LoadWizard::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    { MRESULT ret = IntrospectBase::DlgProc(msg, mp1, mp2);
      SetTitle(Title);
      if (Configuration.SourceServer)
      { ComboBox(+GetCtrl(CB_SERVER)).Text(Configuration.SourceServer);
        PostMsg(UM_CONNECT, 0, 0);
      }
      if (Configuration.Source)
        PMRASSERT(WinSetDlgItemText(GetHwnd(), CB_SINKSRC, Configuration.Source));
      if (Configuration.SourcePort)
        PMRASSERT(WinSetDlgItemText(GetHwnd(), CB_PORT, Configuration.SourcePort));
      // Init rate spin button
      { SpinButton sb(GetCtrl(SB_RATE));
        sb.SetItems(SamplingRates, sizeof SamplingRates/sizeof *SamplingRates);
        size_t pos;
        if ( !binary_search(&Configuration.SourceRate, pos, SamplingRates, sizeof SamplingRates/sizeof *SamplingRates, &SamplingRateCmp)
          && ( pos == sizeof SamplingRates/sizeof *SamplingRates
            || (pos && 2*Configuration.SourceRate < atoi(SamplingRates[pos]) + atoi(SamplingRates[pos-1])) ))
          --pos;
        sb.Value(pos);
      }
      WinCheckButton(GetHwnd(), Configuration.SourceChannels == 1 ? RB_MONO : RB_STEREO, TRUE);
      return ret;
    }

   case WM_COMMAND:
    DEBUGLOG(("LoadWizard::DlgProc:WM_COMMAND(%i,%i, %p)\n", SHORT1FROMMP(mp1), SHORT2FROMMP(mp1), mp2));
    switch (SHORT1FROMMP(mp1))
    {case DID_OK:
      { Configuration.SourceServer = WinQueryDlgItemXText(GetHwnd(), CB_SERVER);
        const xstring& source = WinQueryDlgItemXText(GetHwnd(), CB_SINKSRC);
        Configuration.Source = source.length() && !source.startsWithI("default") ? source : xstring();
        const xstring& port = WinQueryDlgItemXText(GetHwnd(), CB_PORT);
        Configuration.SourcePort = port.length() && !port.startsWithI("default") ? port : xstring();
        Configuration.SourceRate = atoi(SamplingRates[SpinButton(GetCtrl(SB_RATE)).Value()]);
        Configuration.SourceChannels = WinQueryButtonCheckstate(GetHwnd(), RB_MONO) ? 1 : 2;
      }
      break;
    }
    break;

   case UM_DISCOVER_SERVER:
    { DEBUGLOG(("LoadWizard::DlgProc:UM_DISCOVER_SERVER\n"));
      try
      { Context.GetServerInfo(ServerInfoOp);
        Sources.clear();
        Context.GetSourceInfo(SourceInfoOp);
      } catch (const PAException& ex)
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, ex.GetMessage());
      }
      return 0;
    }

   case UM_UPDATE_SERVER:
    { int error = LONGFROMMP(mp1);
      DEBUGLOG(("LoadWizard::DlgProc:UM_UPDATE_SERVER %i\n", error));
      if (error)
      { WinSetDlgItemText(GetHwnd(), ST_STATUS, PAConnectException(Context.GetContext(), error).GetMessage());
        return 0;
      }
      WinSetDlgItemText(GetHwnd(), ST_STATUS, "Success");
      ComboBox cb(GetCtrl(CB_SINKSRC));
      // save old value
      const xstring& oldsink = cb.Text();
      // delete old list
      cb.DeleteAll();
      SelectedSource = -1;
      // insert new list and restore old value if reasonable.
      xstring def;
      def.sprintf("default (%s)", Server.default_sink_name.cdata());
      cb.InsertItem(def);
      if (Sources.size() != 0)
      { int defsink = -1;
        for (unsigned i = 0; i < Sources.size(); ++i)
        { PASourceInfo& source = *Sources[i];
          cb.InsertItem(source.name);
          if (SelectedSource < 0 && source.name.compareToI(oldsink) == 0)
            SelectedSource = i;
          if (defsink < 0 && source.name.compareToI(Server.default_sink_name) == 0)
            defsink = i;
        }
        cb.Select(SelectedSource+1);
        if (SelectedSource < 0)
          SelectedSource = defsink;
      }
    }
   case UM_UPDATE_PORT:
    { DEBUGLOG(("LoadWizard::DlgProc:UM_UPDATE_PORT %i\n", SelectedSource));
      ComboBox cb(GetCtrl(CB_PORT));
      // save old value
      const xstring& oldport = cb.Text();
      // delete old list
      cb.DeleteAll();
      // insert new list and restore old value if reasonable.
      xstring def;
      int selected = -1;
      if ((unsigned)SelectedSource < Sources.size())
      { PASourceInfo& source = *Sources[SelectedSource];
        if (source.active_port)
          def.sprintf("default (%s)", source.active_port->name.cdata());
        for (unsigned i = 0; i < source.ports.size(); ++i)
        { PAPortInfo& port = source.ports[i];
          cb.InsertItem(port.name);
          if (selected < 0 && port.name.compareToI(oldport) == 0)
            selected = i;
        }
      }
      cb.InsertItem(def ? def.cdata() : "default", 0);
      cb.Select(selected+1);
      return 0;
    }
  }
  return IntrospectBase::DlgProc(msg, mp1, mp2);
}

void LoadWizard::SourceInfoHandler(const PASourceInfoOperation::Args& args)
{ DEBUGLOG(("LoadWizard(%p)::SinkInfoHandler({%p, %i})\n", this, args.Info, args.Error));
  if (args.Info)
  { Sources.append() = new PASourceInfo(*args.Info);
    return;
  }
  if (args.Error)
    ServerInfoOp.Cancel();
  else if (ServerInfoOp.GetState() != PA_OPERATION_DONE)
    return;
  PostMsg(UM_UPDATE_SERVER, MPFROMLONG(args.Error), 0);
}

