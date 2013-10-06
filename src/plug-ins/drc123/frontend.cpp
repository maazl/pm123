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

#include "frontend.h"
#include "Deconvolution.h"
#include "Calibrate.h"

#include <stdlib.h>
#include <error.h>
#include <string.h>

#include <fileutil.h>
#include <plugin.h>
#include <cpp/dlgcontrols.h>
#include <cpp/container/stringmap.h>

#include <os2.h>

#include <debuglog.h>


/********** Ini file stuff */

xstringconst DefRecURI("record:///0?samp=48000&stereo&in=line&share=yes");

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
        Filter::WorkDir = EntryField(+GetCtrl(EF_WORKDIR)).Text();
        OpenLoop::RecURI = EntryField(+GetCtrl(EF_RECURI)).Text();
      }
    }
    return 0;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_BROWSE:
      { FILEDLG fdlg = { sizeof(FILEDLG) };
        char type[_MAX_PATH];
        fdlg.fl = FDS_OPEN_DIALOG;
        fdlg.ulUser = FDU_DIR_ENABLE|FDU_DIR_ONLY;
        fdlg.pszTitle = "Select DRC123 working directory";
        fdlg.pszIType = type;
        strlcpy(fdlg.szFullFile, ControlBase(+GetCtrl(EF_WORKDIR)).Text(), sizeof fdlg.szFullFile);

        (*Ctx.plugin_api->file_dlg)(GetHwnd(), GetHwnd(), &fdlg);
        if (fdlg.lReturn == DID_OK)
          ControlBase(+GetCtrl(EF_WORKDIR)).Text(fdlg.szFullFile);
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


Frontend::CalibratePage::CalibratePage(Frontend& parent)
: PageBase(parent, DLG_CALIBRATE, parent.ResModule, DF_AutoResize)
, Response(Calibrate::CalData, 1)
, XTalk(Calibrate::CalData, 5)
, AnaUpdateDeleg(*this, &CalibratePage::AnaUpdateNotify)
{ MajorTitle = "~Calibrate";
  MinorTitle = "Calibrate sound card";
  VULeft  .SetRange(-40,0, -10,-3);
  VURight .SetRange(-40,0, -10,-3);
  Response.SetAxis(20,20000, -30,+20,  -40, +60);
  XTalk   .SetAxis(20,20000, -90,+10, -200,+300);
}

Frontend::CalibratePage::~CalibratePage()
{}

MRESULT Frontend::CalibratePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    Response.Attach(GetCtrl(CC_RESULT));
    XTalk.Attach(GetCtrl(CC_RESULT2));
    // Load initial values
    RadioButton(+GetCtrl(RB_STEREO_LOOP + Calibrate::CalData.Mode)).CheckState(true);
    SpinButton(+GetCtrl(SB_VOLUME)).Value((int)(Calibrate::CalData.Volume*100));
    if (Calibrate::CalData.FileName)
      ControlBase(+GetCtrl(ST_FILE)).Text(sfnameext2(Calibrate::CalData.FileName));
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_START:
      if (Calibrate::Start())
      { ControlBase(+GetCtrl(PB_START)).Enabled(false);
        ControlBase(+GetCtrl(PB_LOAD)).Enabled(false);
        RadioButton(+GetCtrl(RB_STEREO_LOOP)).EnableAll(false);
        VULeft.Attach(GetCtrl(BX_LEFT));
        VURight.Attach(GetCtrl(BX_RIGHT));
        WinStartTimer(NULL, GetHwnd(), TID_VU, 100);
      }
      break;
     case PB_STOP:
      if (Calibrate::Stop())
      { ControlBase(+GetCtrl(PB_START)).Enabled(true);
        ControlBase(+GetCtrl(PB_LOAD)).Enabled(true);
        RadioButton(+GetCtrl(RB_STEREO_LOOP)).EnableAll(true);
      }
      WinStopTimer(NULL, GetHwnd(), TID_VU);
      VULeft.Detach();
      VURight.Detach();
      break;
     case PB_CLEAR:
      Calibrate::CalData.clear();
      break;
     case PB_LOAD:
      { FILEDLG fd = { sizeof(FILEDLG) };
        fd.fl = FDS_OPEN_DIALOG;
        fd.pszTitle = "Load calibration file";
        if ((*Ctx.plugin_api->file_dlg)(GetHwnd(), GetHwnd(), &fd) == DID_OK)
        { // TODO

        }
      }
      break;
     case PB_SAVE:
      // TODO
      break;
    }
    return 0;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case DLG_CALIBRATE:
      if (SHORT2FROMMP(mp1) == BKN_PAGESELECTED)
      { PAGESELECTNOTIFY& pn = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
        if (pn.ulPageIdCur == GetPageID())
          AnaUpdateDeleg.detach();
        if (pn.ulPageIdNew == GetPageID())
          Calibrate::GetEvDataUpdate() += AnaUpdateDeleg;
      }
      break;

     case RB_STEREO_LOOP:
     case RB_MONO_LOOP:
     case RB_LEFT_LOOP:
     case RB_RIGHT_LOOP:
      if (SHORT2FROMMP(mp2) != BN_PAINT)
        Calibrate::CalData.Mode = (Calibrate::MeasureMode)(SHORT1FROMMP(mp1) - RB_STEREO_LOOP);
      break;

     case SB_VOLUME:
      if (SHORT2FROMMP(mp2) == SPBN_CHANGE)
        Calibrate::SetVolume(SpinButton(HWNDFROMMP(mp2)).Value() / 100.);
      break;
    }
    return 0;

   case WM_TIMER:
    switch (SHORT1FROMMP(mp1))
    {case TID_VU:
      OpenLoop::Statistics stat[2];
      OpenLoop::GetStatistics(stat);
      if (stat[0].Count)
        VULeft.SetValue(stat[0].RMSdB(), stat[0].PeakdB());
      if (stat[1].Count)
        VURight.SetValue(stat[1].RMSdB(), stat[1].PeakdB());
      return 0;
    }
    break;

   case UM_UPDATE:
    Response.Invalidate();
    XTalk.Invalidate();
  }

  return PageBase::DlgProc(msg, mp1, mp2);
}

void Frontend::CalibratePage::AnaUpdateNotify(const int&)
{ DEBUGLOG(("Frontend::CalibratePage::AnaUpdateNotify()\n"));
  PostMsg(UM_UPDATE, 0,0);
}
