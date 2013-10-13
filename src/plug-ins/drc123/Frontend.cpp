/*
 * Copyright 2009-2013 Marcel Mueller
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

#define  INCL_PM
#define  INCL_BASE

#include "Frontend.h"
#include "Deconvolution.h"

#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <math.h>

#include <fileutil.h>
#include <plugin.h>
#include <cpp/url123.h>
#include <cpp/dlgcontrols.h>
#include <cpp/container/stringmap.h>

#include <os2.h>

#include <debuglog.h>


xstringconst DefRecURI("record:///0?samp=48000&stereo&in=line&share=yes");

double Frontend::XtractFrequency(const DataRowType& row, void*)
{ return row[0];
}

double Frontend::XtractGain(const DataRowType& row, void* col)
{ return log(row[(int)col]) * (20. / M_LN10);
}

double Frontend::XtractDelay(const DataRowType& row, void* col)
{ return row[(int)col] * row[0];
}

void Frontend::Init()
{
  OpenLoop::RecURI = xstring(DefRecURI);
}

Frontend::Frontend(HWND owner, HMODULE module)
: NotebookDialogBase(DLG_FRONTEND, module, DF_AutoResize)
{
  Pages.append() = new ConfigurationPage(*this);
  Pages.append() = new DeconvolutionPage(*this);
  Pages.append() = new GeneratePage(*this);
  Pages.append() = new MeasurePage(*this);
  Pages.append() = new CalibratePage(*this);
  StartDialog(owner, NB_FRONTEND);
}

Frontend::ConfigurationPage::~ConfigurationPage()
{}

MRESULT Frontend::ConfigurationPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    // Load initial values
    EntryField(+GetCtrl(EF_WORKDIR)).Text(xstring(Filter::WorkDir));
    EntryField(+GetCtrl(EF_RECURI)).Text(xstring(OpenLoop::RecURI));
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case DLG_CONFIG:
      if (SHORT2FROMMP(mp1) == BKN_PAGESELECTEDPENDING)
      { // save values before page change, also save on exit.
        const xstring& workdir = EntryField(+GetCtrl(EF_WORKDIR)).Text();
        if (!workdir.length())
          Filter::WorkDir.reset();
        else if (url123::isPathDelimiter(workdir[workdir.length()-1]))
          Filter::WorkDir = workdir;
        else
          Filter::WorkDir = workdir + "\\";
        OpenLoop::RecURI = EntryField(+GetCtrl(EF_RECURI)).Text();
      }
    }
    return 0;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_BROWSE:
      { FILEDLG fdlg = { sizeof(FILEDLG) };
        char type[_MAX_PATH];
        fdlg.fl = FDS_OPEN_DIALOG|FDS_CENTER;
        fdlg.ulUser = FDU_DIR_ENABLE|FDU_DIR_ONLY;
        fdlg.pszTitle = "Select DRC123 working directory";
        fdlg.pszIType = type;
        strlcpy(fdlg.szFullFile, ControlBase(+GetCtrl(EF_WORKDIR)).Text(), sizeof fdlg.szFullFile);

        (*Ctx.plugin_api->file_dlg)(HWND_DESKTOP, GetHwnd(), &fdlg);
        if (fdlg.lReturn == DID_OK)
        { size_t len = strlen(fdlg.szFullFile);
          if (len && !url123::isPathDelimiter(fdlg.szFullFile[len-1]))
          { fdlg.szFullFile[len] = '\\';
            fdlg.szFullFile[len+1] = 0;
          }
          ControlBase(+GetCtrl(EF_WORKDIR)).Text(fdlg.szFullFile);
        }
      }
      break;
     case PB_CURRENT:
      { const char* url = (*Ctx.plugin_api->exec_command)("current song");
        if (url && *url)
          EntryField(+GetCtrl(EF_RECURI)).Text(url);
      }
      break;
    }
    return 0;
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}


Frontend::DeconvolutionPage::DeconvolutionPage(Frontend& parent)
: PageBase(parent, DLG_DECONV, parent.ResModule, DF_AutoResize)
{ MajorTitle = "~Playback";
  MinorTitle = "Deconvolution at playback";
  Result.SetAxes(ResponseGraph::AF_LogX, 20,20000, -40,+10, -10,40);
  Result.AddGraph("< L gain dB", Kernel, &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)1, ResponseGraph::GF_None, CLR_BLUE);
  Result.AddGraph("< R gain dB", Kernel, &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)3, ResponseGraph::GF_None, CLR_RED);
  Result.AddGraph("L delay t >", Kernel, &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)2, ResponseGraph::GF_Y2, CLR_GREEN);
  Result.AddGraph("R delay t >", Kernel, &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)4, ResponseGraph::GF_Y2, CLR_PINK);
}

Frontend::DeconvolutionPage::~DeconvolutionPage()
{}

static const char* const FIROrders[] =
{ "16384"
, "24576"
, "32768"
, "49152"
, "65536"
, "98304"
};

MRESULT Frontend::DeconvolutionPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    ComboBox(+GetCtrl(CB_FIRORDER)).InsertItems(FIROrders, countof(FIROrders));
    // Load initial values
    PostCommand(PB_UNDO);
    break;

   case WM_DESTROY:
    Result.Detach();
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case DLG_DECONV:
      if (SHORT2FROMMP(mp1) == BKN_PAGESELECTED && ((PAGESELECTNOTIFY*)PVOIDFROMMP(mp2))->ulPageIdNew == GetPageID())
        PostCommand(PB_RELOAD);
      break;
     case LB_KERNEL:
      if (SHORT2FROMMP(mp1) == LN_SELECT)
      { xstringbuilder sb;
        sb.append(ControlBase(+GetCtrl(EF_WORKDIR)).Text());
        if (sb.length() && sb[sb.length()-1] != '\\')
          sb.append('\\');
        ListBox lb(+GetCtrl(LB_KERNEL));
        int sel = lb.NextSelection();
        sb.append(lb.ItemText(sel));
        Params.FilterFile = sb.get();
        //DEBUGLOG(("%s\n", Params.FilterFile.cdata()));
        PostMsg(UM_UPDATEDESCR, 0, 0);
      }
      break;
    }
    return 0;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_DEFAULT:
      Deconvolution::GetDefaultParameters(Params);
      goto load;

     case PB_UNDO:
      Deconvolution::GetParameters(Params);
     load:
      { // Load GUI from current configuration
        CheckBox(+GetCtrl(CB_ENABLE)).Enabled(Params.FilterFile != NULL);
        // Populate list box with filter kernels
        PostCommand(PB_RELOAD);
        CheckBox(+GetCtrl(CB_ENABLE)).Enabled(Params.Enabled);
        RadioButton(+GetCtrl(RB_WIN_NONE + Params.WindowFunction)).CheckState();
        int selected; // default
        switch (Params.FIROrder)
        {case 16384:
          selected = 0; break;
         case 24576:
          selected = 1; break;
         case 32768:
          selected = 2; break;
         default: //case 49152:
          selected = 3; break;
         case 65536:
          selected = 4; break;
         case 98304:
          selected = 5; break;
        }
        ComboBox(+GetCtrl(CB_FIRORDER)).Select(selected);
      }
      break;

     case PB_APPLY:
      { // Update configuration from GUI
        Params.WindowFunction = (Deconvolution::WFN)(RadioButton(+GetCtrl(RB_WIN_NONE)).CheckID() - RB_WIN_NONE);
        switch (ComboBox(+GetCtrl(CB_FIRORDER)).NextSelection())
        {case 0: // 16384
          Params.FIROrder = 16384;
          Params.PlanSize = 32768;
          break;
         case 1: // 24576
          Params.FIROrder = 24576;
          Params.PlanSize = 32768;
          break;
         case 2: // 32768
          Params.FIROrder = 32768;
          Params.PlanSize = 65536;
          break;
         case 3: // 49152
          Params.FIROrder = 49152;
          Params.PlanSize = 65536;
          break;
         case 4: // 65536
          Params.FIROrder = 65536;
          Params.PlanSize = 131072;
          break;
         case 5: // 98304
          Params.FIROrder = 98304;
          Params.PlanSize = 131072;
          break;
        }
        Deconvolution::SetParameters(Params);
      }
      break;

     case PB_RELOAD:
      UpdateDir();
      break;
    }
    return 0;

   case UM_UPDATEDESCR:
    { ControlBase descr(+GetCtrl(ST_DESCR));
      ControlBase enabled(+GetCtrl(CB_ENABLE));
      if (!Params.FilterFile)
      { descr.Text("No working directory");
        goto disable;
      }
      if (!Kernel.Load(Params.FilterFile))
      { descr.Text(strerror(errno));
       disable:
        Result.Detach();
        enabled.Enabled(false);
        return 0;
      }
      Result.Attach(GetCtrl(CC_RESULT));
      descr.Text(Kernel.Description.length() ? Kernel.Description.cdata() : "no description");
      enabled.Enabled(true);
    }
    return 0;
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

void Frontend::DeconvolutionPage::UpdateDir()
{ ListBox lb(+GetCtrl(LB_KERNEL));
  ControlBase descr(+GetCtrl(ST_DESCR));
  lb.DeleteAll();
  xstringbuilder path;
  path.append(EntryField(+GetCtrl(EF_WORKDIR)).Text());
  if (path.length())
  { if (path[path.length()-1] != '\\' && path[path.length()-1] != '/')
      path.append('\\');
    size_t pathlen = path.length();
    path.append("*.target");
    HDIR hdir = HDIR_CREATE;
    char buf[1024];
    ULONG count = 100;
    APIRET rc = DosFindFirst(path.cdata(), &hdir,
      FILE_ARCHIVED|FILE_READONLY|FILE_HIDDEN,
      &buf, sizeof buf, &count, FIL_STANDARD);
    switch (rc)
    {default:
      descr.Text(os2_strerror(rc, buf, sizeof buf));
      break;
     case NO_ERROR:
      const char* names[100];
      int selected = LIT_NONE;
      const char* currentname = Params.FilterFile ? sfnameext2(Params.FilterFile) : NULL;
      do
      { FILEFINDBUF3* fb = (FILEFINDBUF3*)buf;
        const char** np = names;
        for(;;)
        { // selected entry?
          if (currentname && stricmp(currentname, sfnameext2(fb->achName)) == 0)
          { selected = np - names;
            path.erase(pathlen);
            path.append(fb->achName);
          }
          // store entry
          *np++ = fb->achName;
          // next entry
          if (!fb->oNextEntryOffset)
            break;
          fb = (FILEFINDBUF3*)((char*)fb + fb->oNextEntryOffset);
        }
        // insert items
        lb.InsertItems(names, np - names, LIT_END);
        // next package
        count = 100;
      } while (DosFindNext(hdir, &buf, sizeof buf, &count) == NO_ERROR);
      // Select item
      if (selected != LIT_NONE)
      { lb.Select(selected);
        Params.FilterFile = path.get();
      } else
      { Params.FilterFile.reset();
        CheckBox(+GetCtrl(CB_ENABLE)).Enabled(false);
      }
      PostMsg(UM_UPDATEDESCR, 0, 0);
    }

  } else
  { // No working directory
    descr.Text("No working directory");
  }
}


Frontend::GeneratePage::~GeneratePage()
{}

MRESULT Frontend::GeneratePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return PageBase::DlgProc(msg, mp1, mp2);
}


Frontend::MeasurePage::~MeasurePage()
{}

MRESULT Frontend::MeasurePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return PageBase::DlgProc(msg, mp1, mp2);
}

