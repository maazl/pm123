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
#include <cpp/directory.h>
#include <cpp/container/stringmap.h>
#include <cpp/container/stringset.h>

#include <os2.h>

#include <debuglog.h>


xstringconst DefRecURI("record:///0?samp=48000&stereo&in=line&share=yes");
int_ptr<Frontend> Frontend::Instance;


double Frontend::DBGainIterator::ScaleResult(double value) const
{ return log(value) * (20./M_LN10);
}

double Frontend::PhaseDelayIterator::ScaleResult(double value) const
{ return value * LastX * 360.;
}

void Frontend::SetValue(HWND ctrl, double value, const char* mask)
{
  char buffer[32];
  sprintf(buffer, mask, value);
  ControlBase(ctrl).Text(buffer);
}

bool Frontend::GetValue(HWND ctrl, double& value)
{
  char buffer[32];
  LONG len = WinQueryWindowText(ctrl, sizeof buffer, buffer);
  int n = -1;
  return sscanf(buffer, "%lf%n", &value, &n) == 1 && n == len;
}

void Frontend::Init()
{
  OpenLoop::RecURI = xstring(DefRecURI);
}

HWND Frontend::Show(HWND owner, HMODULE module)
{
  int_ptr<Frontend> instance = Instance;
  if (!instance)
    Instance = instance = new Frontend(owner, module);
  instance->SetVisible(true);
  return instance->GetHwnd();
}

Frontend::Frontend(HWND owner, HMODULE module)
: ManagedDialog<NotebookDialogBase>(DLG_FRONTEND, module, DF_AutoResize)
{
  Pages.append() = new ConfigurationPage(*this);
  Pages.append() = new CalibratePage(*this);
  Pages.append() = new CalibrateExtPage(*this);
  Pages.append() = new MeasurePage(*this);
  Pages.append() = new MeasureExtPage(*this);
  Pages.append() = new GeneratePage(*this);
  Pages.append() = new GenerateExtPage(*this);
  Pages.append() = new DeconvolutionPage(*this);
  Pages.append() = new DeconvolutionExtPage(*this);
  StartDialog(owner, NB_FRONTEND);
}

void Frontend::OnDestroy()
{
  Instance = NULL;
  ManagedDialog<NotebookDialogBase>::OnDestroy();
  save_config();
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
        fdlg.ulUser = FDU_DIR_ONLY;
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


void Frontend::FilePage::SetModified()
{ if (Modified)
    return;
  Modified = true;
  if (WinQueryWindowTextLength(+GetCtrl(EF_FILE)))
    ControlBase(+GetCtrl(PB_SAVE)).Enabled(true);
}

void Frontend::FilePage::ClearModified()
{ if (Modified)
    return;
  Modified = false;
  ControlBase(+GetCtrl(PB_SAVE)).Enabled(false);
}

MRESULT Frontend::FilePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    // load initial file content
    ControlBase(+GetCtrl(PB_SAVE)).Enabled(false);
    PostCommand(PB_RELOAD);
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_LOAD:
      if (LoadFile())
      { ClearModified();
        LoadControlValues();
      }
      break;
     case PB_SAVE:
      StoreControlValues();
      if (SaveFile())
        ClearModified();
      break;
     case PB_RELOAD:
      if (DoLoadFile(NULL))
      { ClearModified();
        LoadControlValues();
      }
      break;
    }
    return 0;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case EF_FILE:
      if (SHORT2FROMMP(mp1) == EN_CHANGE && Modified)
        ControlBase(+GetCtrl(PB_SAVE)).Enabled(WinQueryWindowTextLength(HWNDFROMMP(mp2)) != 0);
      return 0;
     case ML_DESCR:
      if (SHORT2FROMMP(mp1) == MLN_CHANGE)
        SetModified();
      return 0;
    }
    if (SHORT2FROMMP(mp1) == BKN_PAGESELECTED && SHORT1FROMMP(mp1) == ControlBase(GetHwnd()).ID())
    { PAGESELECTNOTIFY& pn = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
      if (pn.ulPageIdCur == GetPageID())
        StoreControlValues();
      if (pn.ulPageIdNew == GetPageID())
        LoadControlValues();
    }
    return 0;
  }

  return PageBase::DlgProc(msg, mp1, mp2);
}

void Frontend::FilePage::LoadControlValues(const DataFile& data)
{
  bool have_filename = !!data.FileName.length();
  ControlBase(+GetCtrl(PB_SAVE)).Enabled(have_filename);
  ControlBase(+GetCtrl(EF_FILE)).Text(have_filename ? url123::normalizeURL(data.FileName).getShortName().cdata() : "");
  ControlBase(+GetCtrl(ML_DESCR)).Text(data.Description);
}

void Frontend::FilePage::StoreControlValues(DataFile& data)
{ data.Description = ControlBase(+GetCtrl(ML_DESCR)).Text();
  const xstring& file = ControlBase(+GetCtrl(EF_FILE)).Text();
  if (file.length())
    data.FileName = xstring(Filter::WorkDir) + file;
  else
    data.FileName = xstring::empty;
}

LONG Frontend::FilePage::DoLoadFileDlg(FILEDLG& fdlg)
{ (*Ctx.plugin_api->file_dlg)(HWND_DESKTOP, GetHwnd(), &fdlg);
  return fdlg.lReturn == DID_OK && !DoLoadFile(fdlg.szFullFile)
    ? 0 : fdlg.lReturn;
}

bool Frontend::FilePage::LoadFile()
{
  FILEDLG fdlg = { sizeof(FILEDLG) };
  fdlg.fl = FDS_OPEN_DIALOG|FDS_CENTER|FDS_FILTERUNION;
  fdlg.ulUser = FDU_FILE_ONLY;
  strncpy(fdlg.szFullFile, xstring(Filter::WorkDir), sizeof fdlg.szFullFile);
  switch (DoLoadFileDlg(fdlg))
  {case DID_OK:
    return true;
   default:
    // failed
    WinMessageBox(HWND_DESKTOP, GetHwnd(), strerror(errno), "Failed to load file", 0, MB_OK|MB_ERROR|MB_MOVEABLE);
   case DID_CANCEL:
    return false;
  }
}

bool Frontend::FilePage::SaveFile()
{
  const xstring& err = DoSaveFile();
  if (!err)
    return true;
  // failed
  xstringbuilder sb;
  sb.append(err);
  sb.append('\n');
  sb.append(strerror(errno));
  WinMessageBox(HWND_DESKTOP, GetHwnd(), sb, "Failed to save file", 0, MB_OK|MB_ERROR|MB_MOVEABLE);
  return false;
}
