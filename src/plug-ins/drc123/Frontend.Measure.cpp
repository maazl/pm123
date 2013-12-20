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

  Response.AddGraph("< L gain", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Measure::MeasureFile::LGain, ResponseGraph::GF_None, CLR_BLUE);
  Response.AddGraph("< R gain", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Measure::MeasureFile::RGain, ResponseGraph::GF_None, CLR_RED);
  Response.AddGraph("L delay >", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)Measure::MeasureFile::LDelay, ResponseGraph::GF_Y2, CLR_GREEN);
  Response.AddGraph("R delay >", Measure::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)Measure::MeasureFile::RDelay, ResponseGraph::GF_Y2, CLR_PINK);
}

Frontend::MeasurePage::~MeasurePage()
{}

MRESULT Frontend::MeasurePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    Response.Attach(GetCtrl(CC_RESULT));
    PostMsg(UM_UPDATECALLIST, 0,0);
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case CB_CAL_FILE:
      if (SHORT2FROMMP(mp1) == CBN_ENTER)
        PostMsg(UM_UPDATECAL, 0,0);
      break;
     case RB_CH_BOTH:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
        ControlBase(+GetCtrl(CB_DIFFOUT)).Enabled(false);
      break;
     case RB_CH_LEFT:
     case RB_CH_RIGHT:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
        ControlBase(+GetCtrl(CB_DIFFOUT)).Enabled(true);
      break;
    }
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_START:
      { SyncAccess<Measure::MeasureFile> data(Measure::GetData());
        data->Mode = (Measure::MeasureMode)(RadioButton(+GetCtrl(RB_STEREO_LOOP)).CheckID() - RB_STEREO_LOOP);
      }
      break;
    }
    break;

   case UM_UPDATECALLIST:
    UpdateDir();
    return 0;

   case UM_UPDATECAL:
    UpdateCal();
    return 0;
  }

  return OpenLoopPage::DlgProc(msg, mp1, mp2);
}

void Frontend::MeasurePage::LoadControlValues(const Measure::MeasureFile& data)
{
  RadioButton(+GetCtrl(RB_NOISE + data.Mode)).CheckState(true);
  RadioButton(+GetCtrl(RB_CH_BOTH + data.Chan)).CheckState(true);
  CheckBox diffout(+GetCtrl(CB_DIFFOUT));
  diffout.CheckState(data.DiffOut);
  diffout.Enabled(data.Chan != Measure::CH_Both);

  /*bool have_calfile = data.CalFile && data.CalFile.length();
  ControlBase(+GetCtrl(EF_FILE)).Text(have_calfile ? url123(data.FileName).getShortName().cdata() : "");*/
  CheckBox(+GetCtrl(CB_REFIN)).CheckState(data.RefIn);

  OpenLoopPage::LoadControlValues(data);

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
  data.Chan = (Measure::Channels)(RadioButton(+GetCtrl(RB_CH_BOTH)).CheckID() - RB_CH_BOTH);
  data.DiffOut = (bool)CheckBox(+GetCtrl(CB_DIFFOUT)).CheckState();

  data.RefIn = (bool)CheckBox(+GetCtrl(CB_REFIN)).CheckState();

  OpenLoopPage::StoreControlValues(data);

  if (data.FileName)
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
  RadioButton(+GetCtrl(RB_CH_BOTH)).EnableAll(!running);
  ControlBase(+GetCtrl(CB_DIFFOUT)).Enabled(!running && !RadioButton(+GetCtrl(RB_CH_BOTH)).CheckState());

  ControlBase(+GetCtrl(CB_CAL_FILE)).Enabled(!running);
  ControlBase(+GetCtrl(CB_REFIN)).Enabled(!running);

  OpenLoopPage::SetRunning(running);
}

void Frontend::MeasurePage::InvalidateGraph()
{ Response.Invalidate();
}

void Frontend::MeasurePage::UpdateDir()
{ DEBUGLOG(("Frontend::MeasurePage::UpdateDir()\n"));
  ComboBox lb(+GetCtrl(CB_CAL_FILE));
  ControlBase descr(+GetCtrl(ST_CAL_DESC));

  xstring currentname;
  if (lb.NextSelection() > 0)
    currentname = lb.Text();
  lb.DeleteAll();
  const char* names[50];
  names[0] = "- none -";
  unsigned count = 1;
  int selected = 0; // in doubt select first entry

  xstring path = Filter::WorkDir;
  if (!path.length())
  { descr.Text("No working directory");
  } else
  { DirScan dir(path, "*.calibrate", FILE_ARCHIVED|FILE_READONLY|FILE_HIDDEN);
    while (dir.Next() == 0)
    { const char* name = dir.Current()->achName;
      if (currentname && stricmp(currentname, name) == 0)
      { selected = count + lb.Count();
        path = dir.CurrentPath();
      }
      names[count++] = name;
      // package full or storage from dir.Current no longer stable?
      if (count == sizeof names / sizeof *names || dir.RemainingPackageSize() == 1)
      { lb.InsertItems(names, count, LIT_END);
        count = 0;
      }
    }
  }
  if (count)
    lb.InsertItems(names, count, LIT_END);

  // restore previous value
  lb.Select(selected);
}

void Frontend::MeasurePage::UpdateCal()
{ DEBUGLOG(("Frontend::MeasurePage::UpdateCal()\n"));
  ComboBox lb(+GetCtrl(CB_CAL_FILE));
  ControlBase descr(+GetCtrl(ST_CAL_DESC));

  int sel = lb.NextSelection();
  if (sel > 0)
  { char path[_MAX_PATH];
    xstring workdir(Filter::WorkDir);
    rel2abs(workdir, lb.ItemText(sel), path, sizeof path);
    { SyncAccess<Measure::MeasureFile> data(Measure::GetData());
      data->CalFile = path;
    }
    if (!Calibration.Load(path))
    { descr.Text(strerror(errno));
    } else
    { descr.Text(Calibration.Description.length() ? Calibration.Description.cdata() : "no description");
    }
  } else
    descr.Text("");
}

LONG Frontend::MeasurePage::DoLoadFile(FILEDLG& fdlg)
{
  fdlg.pszTitle = "Load DRC123 measurement file";
  // PM crashes if type is not writable
  char type[_MAX_PATH] = "DRC123 Measurement File (*.measure)";
  fdlg.pszIType = type;
  LONG ret = OpenLoopPage::DoLoadFile(fdlg);
  if (ret == DID_OK)
  { { SyncAccess<Measure::MeasureFile> data(Measure::GetData());
      if (!data->Load(fdlg.szFullFile))
        return 0;
      LoadControlValues(*data);
    }
    InvalidateGraph();
  }
  return ret;
}

xstring Frontend::MeasurePage::DoSaveFile()
{
  SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  if (data->Save(data->FileName))
    return xstring();
  return data->FileName;
}


void Frontend::MeasureExtPage::LoadControlValues()
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  ExtPage::LoadControlValues(*data);
}

void Frontend::MeasureExtPage::LoadDefaultValues()
{ ExtPage::LoadControlValues(Measure::DefData);
}

void Frontend::MeasureExtPage::StoreControlValues()
{ SyncAccess<Measure::MeasureFile> data(Measure::GetData());
  ExtPage::StoreControlValues(*data);
}
