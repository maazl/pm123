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
#define  INCL_GPI

#include "Frontend.h"
#include "Measure.h"

#include <fileutil.h>
#include <cpp/url123.h>
#include <cpp/directory.h>

Frontend::MeasurePage::MeasurePage(Frontend& parent)
: OpenLoopPage(parent, DLG_MEASURE, Measure::VTable)
{ MajorTitle = "~Measure";
  MinorTitle = "Measure speaker response";

  Response.AddGraph("< L gain", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Measure::LGain, ResponseGraph::GF_None, CLR_BLUE);
  Response.AddGraph("< R gain", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Measure::RGain, ResponseGraph::GF_None, CLR_RED);
  Response.AddGraph("L delay >", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractColumn, (void*)Measure::LDelay, ResponseGraph::GF_Y2, CLR_GREEN);
  Response.AddGraph("R delay >", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractColumn, (void*)Measure::RDelay, ResponseGraph::GF_Y2, CLR_PINK);
}

Frontend::MeasurePage::~MeasurePage()
{}

MRESULT Frontend::MeasurePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("Frontend::MeasurePage(%p)::DlgProc(%x, %p, %p)\n", this, msg, mp1, mp2));
  switch (msg)
  {case WM_INITDLG:
    Response.Attach(GetCtrl(CC_RESULT));
    break;

   case WM_DESTROY:
    Response.Detach();
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case CB_CAL_FILE:
     case CB_MIC_FILE:
      if (SHORT2FROMMP(mp1) == CBN_ENTER)
        PostMsg(UM_UPDATEFILE, mp2, MPFROMSHORT(SHORT1FROMMP(mp1) + 1));
      return 0;
     case RB_STEREO:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      { ControlBase(+GetCtrl(CB_DIFFOUT)).Enabled(false);
        goto modi;
      }
      return 0;
     case RB_LEFT:
     case RB_RIGHT:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      { ControlBase(+GetCtrl(CB_DIFFOUT)).Enabled(true);
        goto modi;
      }
      return 0;
     case RB_NOISE:
     case RB_SWEEP:
     case CB_DIFFOUT:
     case CB_REFIN:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      {modi:
        SetModified();
      }
      return 0;
    }
    break;

   case UM_UPDATECALLIST:
    { xstring cal, mic;
      if (SHORT1FROMMP(mp1))
      { SyncAccess<Measure::MeasureFile> data(Measure::GetData());
        cal = sfnameext2(data->CalFile);
        mic = sfnameext2(data->MicFile);
      }
      UpdateDir(GetCtrl(CB_CAL_FILE), GetCtrl(ST_CAL_DESC), "*.calibrate", cal);
      UpdateDir(GetCtrl(CB_MIC_FILE), GetCtrl(ST_MIC_DESC), "*.microphone", mic);
    }
    return 0;

   case UM_UPDATEFILE:
    { SetModified();
      const xstring& file = UpdateFile(HWNDFROMMP(mp1), GetCtrl(SHORT1FROMMP(mp2)));
      SyncAccess<Measure::MeasureFile> data(Measure::GetData());
      switch (SHORT1FROMMP(mp2))
      {case ST_CAL_DESC:
        data->CalFile = file; break;
       case ST_MIC_DESC:
        data->MicFile = file; break;
      }
    }
    return 0;
  }

  return OpenLoopPage::DlgProc(msg, mp1, mp2);
}

void Frontend::MeasurePage::LoadControlValues(const Measure::MeasureFile& data)
{
  RadioButton(+GetCtrl(RB_NOISE + data.Mode)).CheckState(true);
  RadioButton(+GetCtrl(RB_STEREO + data.Chan)).CheckState(true);
  CheckBox diffout(+GetCtrl(CB_DIFFOUT));
  diffout.CheckState(data.DiffOut);
  diffout.Enabled(data.Chan != Measure::CH_Both);

  CheckBox(+GetCtrl(CB_REFIN)).CheckState(data.RefIn);

  OpenLoopPage::LoadControlValues(data);

  PostMsg(UM_UPDATECALLIST, MPFROMSHORT(TRUE), 0);

  Response.SetAxes(ResponseGraph::AF_LogX, data.RefFMin, data.RefFMax,
    data.GainLow, data.GainHigh, data.DelayLow, data.DelayHigh);
}

void Frontend::MeasurePage::LoadControlValues()
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  LoadControlValues(*data);
}

void Frontend::MeasurePage::StoreControlValues(Measure::MeasureFile& data)
{
  data.Mode = (Measure::MeasureMode)(RadioButton(+GetCtrl(RB_NOISE)).CheckID() - RB_NOISE);
  data.Chan = (Measure::Channels)(RadioButton(+GetCtrl(RB_STEREO)).CheckID() - RB_STEREO);
  data.DiffOut = (bool)CheckBox(+GetCtrl(CB_DIFFOUT)).CheckState();

  data.RefIn = (bool)CheckBox(+GetCtrl(CB_REFIN)).CheckState();

  OpenLoopPage::StoreControlValues(data);

  if (data.FileName.length())
    data.FileName = data.FileName + ".measure";
}
void Frontend::MeasurePage::StoreControlValues()
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  StoreControlValues(*data);
}

void Frontend::MeasurePage::SetRunning(bool running)
{
  RadioButton(+GetCtrl(RB_NOISE)).EnableAll(!running);
  RadioButton(+GetCtrl(RB_WHITE_N)).EnableAll(!running);
  RadioButton(+GetCtrl(RB_STEREO)).EnableAll(!running);
  ControlBase(+GetCtrl(CB_DIFFOUT)).Enabled(!running && !RadioButton(+GetCtrl(RB_STEREO)).CheckState());

  ControlBase(+GetCtrl(CB_CAL_FILE)).Enabled(!running);
  ControlBase(+GetCtrl(CB_MIC_FILE)).Enabled(!running);
  ControlBase(+GetCtrl(CB_REFIN)).Enabled(!running);

  OpenLoopPage::SetRunning(running);
}

void Frontend::MeasurePage::InvalidateGraph()
{ Response.Invalidate();
  SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  ControlBase(+GetCtrl(ST_SUBRESULT)).Text(xstring().sprintf( "average delay: %.1f ms, indeterminate phase: %.0f ppm",
    1E3 * data->AverageDelay, 1E6 * (int)data->IndeterminatePhase / (int)data->PhaseUnwrapCount ));
}

void Frontend::MeasurePage::UpdateDir(ComboBox lb, ControlBase desc, const char* mask, xstring& selection)
{ DEBUGLOG(("Frontend::MeasurePage::UpdateDir(%p, %p)\n", lb.Hwnd, desc.Hwnd));
  if (selection == NULL && lb.NextSelection() > 0)
    selection = lb.Text();
  lb.DeleteAll();
  lb.InsertItem("- none -");

  xstring path = Filter::WorkDir;
  if (!path.length())
  { desc.Text("No working directory");
    return;
  }
  DirScan dir(path, mask, FILE_ARCHIVED|FILE_READONLY|FILE_HIDDEN);
  stringsetI files;
  while (dir.Next() == 0)
    files.ensure(dir.CurrentFile());

  if (files.size())
    lb.InsertItems((const char*const*)files.begin(), files.size(), LIT_END);

  // restore previous value
  unsigned selected;
  if (selection && files.locate(selection, selected))
    ++selected;
  else
    selected = 0; // in doubt select the first entry;
  lb.Select(selected);

  if (dir.LastRC() != ERROR_NO_MORE_FILES)
    desc.Text(dir.LastErrorText());
  else
    PostMsg(UM_UPDATEFILE, MPFROMHWND(lb.Hwnd), MPFROMSHORT(desc.ID()));
}

xstring Frontend::MeasurePage::UpdateFile(ComboBox lb, ControlBase desc)
{ DEBUGLOG(("Frontend::MeasurePage::UpdateFile(%u, %u)\n", lb.Hwnd, desc.Hwnd));
  xstring file;
  int sel = lb.NextSelection();
  if (sel > 0)
  { char path[_MAX_PATH];
    xstring workdir(Filter::WorkDir);
    rel2abs(workdir, lb.ItemText(sel), path, sizeof path);
    file = path;
    DataFile cal;
    if (!cal.Load(path, true))
    { desc.Text(strerror(errno));
    } else
    { desc.Text(cal.Description.length() ? cal.Description.cdata() : "no description");
    }
  } else
  { desc.Text("");
    file = xstring::empty;
  }
  return file;
}

LONG Frontend::MeasurePage::DoLoadFileDlg(FILEDLG& fdlg)
{ fdlg.pszTitle = "Load DRC123 measurement file";
  // PM crashes if type is not writable
  char type[_MAX_PATH] = "DRC123 Measurement File (*.measure)";
  fdlg.pszIType = type;
  return OpenLoopPage::DoLoadFileDlg(fdlg);
}

bool Frontend::MeasurePage::DoLoadFile(const char* filename)
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  return data->Load(filename);
}

xstring Frontend::MeasurePage::DoSaveFile()
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  if (data->Save(data->FileName))
    return xstring();
  return data->FileName;
}

void Frontend::MeasureExtPage::LoadControlValues(const Measure::MeasureFile& data)
{
  CheckBox(+GetCtrl(CB_VERIFYMODE)).CheckState(data.VerifyMode);

  ExtPage::LoadControlValues(data);
}

void Frontend::MeasureExtPage::LoadControlValues()
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  LoadControlValues(*data);
}

void Frontend::MeasureExtPage::LoadDefaultValues()
{ LoadControlValues(Measure::DefData);
}

void Frontend::MeasureExtPage::StoreControlValues(Measure::MeasureFile& data)
{
  data.VerifyMode = (bool)CheckBox(+GetCtrl(CB_VERIFYMODE)).CheckState();

  ExtPage::StoreControlValues(data);
}

void Frontend::MeasureExtPage::StoreControlValues()
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  StoreControlValues(*data);
}
