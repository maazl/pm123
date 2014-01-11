/*
 * Copyright 2013-2013 Marcel Mueller
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

#define INCL_ERRORS
#include "Frontend.h"
#include "Generate.h"

#include <fileutil.h>
#include <cpp/directory.h>
#include <cpp/url123.h>


Frontend::DeconvolutionViewMode Frontend::DeconvolutionView = DVM_Target;
Frontend::GenerateViewMode Frontend::GenerateView = GVM_Result;
int_ptr<Frontend> Frontend::Instance;

Frontend::GeneratePage::GeneratePage(Frontend& parent)
: FilePage(parent, DLG_GENERATE)
{ MajorTitle = "~Generate";
  MinorTitle = "Generate deconvolution filter";
}

Frontend::GeneratePage::~GeneratePage()
{}

MRESULT Frontend::GeneratePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    RadioButton(+GetCtrl(RB_VIEWTARGET + GenerateView)).CheckState(true);
    Result1.Attach(GetCtrl(CC_RESULT));
    Result2.Attach(GetCtrl(CC_RESULT2));
    PostMsg(UM_UPDATEGRAPH, 0,0);
    break;

   case WM_DESTROY:
    Result1.Detach();
    Result2.Detach();
    break;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_START:
      StoreControlValues();
      Run();
    }
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case LB_KERNEL:
      if (SHORT2FROMMP(mp1) == LN_SELECT)
        PostMsg(UM_UPDATEDESCR, 0,0);
      break;
     case RB_VIEWTARGET:
     case RB_VIEWGAIN:
     case RB_VIEWDELAY:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      { GenerateView = (GenerateViewMode)(SHORT1FROMMP(mp1) - RB_VIEWTARGET);
        PostMsg(UM_UPDATEGRAPH, 0,0);
      }
      break;
    }
    break;

   case UM_UPDATEDIR:
    UpdateDir();
    return 0;

   case UM_UPDATEDESCR:
    UpdateDescr();
    return 0;

   case UM_UPDATEGRAPH:
    SetupGraph();
    return 0;
  }
  return FilePage::DlgProc(msg, mp1, mp2);
}

void Frontend::GeneratePage::LoadControlValues()
{ SyncAccess<Generate::TargetFile> data(Generate::GetData());

  LoadControlValues(*data);
}

void Frontend::GeneratePage::LoadControlValues(const Generate::TargetFile& data)
{ FilePage::LoadControlValues(data);

  SetGraphAxes(data);

  PostMsg(UM_UPDATEDIR, 0,0);
}

void Frontend::GeneratePage::StoreControlValues()
{ SyncAccess<Generate::TargetFile> data(Generate::GetData());

  FilePage::StoreControlValues(*data);

  if (data->FileName.length())
    data->FileName = data->FileName + ".target";

  xstringbuilder path(_MAX_PATH);
  xstring workdir(Filter::WorkDir);
  path.append(workdir);

  Generate::Parameters::MeasurementSet newset;
  ListBox lb(GetCtrl(LB_KERNEL));
  int cur = LIT_FIRST;
  while ((cur = lb.NextSelection(cur)) != LIT_NONE)
  { path.erase(workdir.length());
    path.append(lb.ItemText(cur));
    Measure::MeasureFile* entry = data->Measurements.erase(path);
    if (entry == NULL)
    { entry = new Measure::MeasureFile();
      entry->FileName = path.cdata();
    }
    newset.get(path) = entry;
  }
  data->Measurements.swap(newset);
}


LONG Frontend::GeneratePage::DoLoadFileDlg(FILEDLG& fdlg)
{
  fdlg.pszTitle = "Load DRC123 target file";
  // PM crashes if type is not writable
  char type[_MAX_PATH] = "DRC123 Target File (*.target)";
  fdlg.pszIType = type;
  LONG ret = FilePage::DoLoadFileDlg(fdlg);
  if (ret == DID_OK)
    SetupGraph();
  return ret;
}

bool Frontend::GeneratePage::DoLoadFile(const char* filename)
{ SyncAccess<Generate::TargetFile> data(Generate::GetData());
  return data->Load(filename);
}

xstring Frontend::GeneratePage::DoSaveFile()
{ SyncAccess<Generate::TargetFile> data(Generate::GetData());
  if (data->Save(data->FileName))
    return xstring();
  return data->FileName;
}

void Frontend::GeneratePage::UpdateDir()
{ ListBox lb(+GetCtrl(LB_KERNEL));

  ControlBase descr(+GetCtrl(ST_DESCR));
  lb.DeleteAll();
  xstring path = Filter::WorkDir;
  if (!path.length())
  { descr.Text("No working directory");
    return;
  }
  DirScan dir(path, "*.measure", FILE_ARCHIVED|FILE_READONLY|FILE_HIDDEN);
  stringsetI files;
  while (dir.Next() == 0)
    files.ensure(dir.CurrentFile());

  if (files.size())
    lb.InsertItems((const char*const*)files.begin(), files.size(), LIT_END);
  // Select items
  { SyncAccess<Generate::TargetFile> data(Generate::GetData());
    unsigned pos;
    foreach (const Measure::MeasureFile,*const*, sp, data->Measurements)
      if (files.locate(sfnameext2((*sp)->FileName), pos))
        lb.Select(pos);
  }

  if (dir.LastRC() != ERROR_NO_MORE_FILES)
    descr.Text(dir.LastErrorText());
  else
    PostMsg(UM_UPDATEDESCR, 0, 0);
}

void Frontend::GeneratePage::UpdateDescr()
{ ListBox lb(GetCtrl(LB_KERNEL));
  xstringbuilder sb;
  int selected = LIT_FIRST;
  unsigned index;
  while ((selected = lb.NextSelection(selected)) != LIT_NONE)
  { xstring file(Filter::WorkDir);
    file = file + lb.ItemText(selected);
    DataFile data;
    if (data.Load(file, true))
    { ++index;
      if (data.Description.length())
      { if (sb.length() && sb[sb.length()-1] != '\n')
          sb.append('\n');
        sb.appendf("%u: ", index);
        sb.append(data.Description);
      }
    } else
      sb.append(strerror(errno));
  }
  ControlBase(+GetCtrl(ST_DESCR)).Text(!index ? "\33 select measurements" : sb.length() ? sb.cdata() : "no description");
}

void Frontend::GeneratePage::InvalidateGraph()
{ Result1.Invalidate();
  Result2.Invalidate();
}

void Frontend::GeneratePage::SetGraphAxes(const Generate::TargetFile& data)
{ switch (GenerateView)
  {default: // VM_RESULT
    Result1.SetAxes(ResponseGraph::AF_LogX, data.FreqLow, data.FreqHigh,
      data.GainLow, data.GainHigh, NAN,NAN);
    Result2.SetAxes(ResponseGraph::AF_LogX, data.FreqLow, data.FreqHigh,
      data.DelayLow, data.DelayHigh, NAN,NAN);
    break;
   case GVM_Gain:
    Result1.SetAxes(ResponseGraph::AF_LogX, data.FreqLow, data.FreqHigh,
      data.GainLow, data.GainHigh, NAN,NAN);
    Result2.SetAxes(ResponseGraph::AF_LogX, data.FreqLow, data.FreqHigh,
      data.GainLow, data.GainHigh, NAN,NAN);
    break;
   case GVM_Delay:
    Result1.SetAxes(ResponseGraph::AF_LogX, data.FreqLow, data.FreqHigh,
      data.DelayLow, data.DelayHigh, NAN,NAN);
    Result2.SetAxes(ResponseGraph::AF_LogX, data.FreqLow, data.FreqHigh,
      data.DelayLow, data.DelayHigh, NAN,NAN);
    break;
  }
}

void Frontend::GeneratePage::SetupGraph()
{ const char *text1, *text2;
  Result1.ClearGraphs();
  Result2.ClearGraphs();
  switch (GenerateView)
  {default: // VM_Result
    text1 = " Frequency response ";
    text2 = " Group delay ";
    Result1.AddGraph("left", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Generate::LGain, ResponseGraph::GF_None, CLR_BLUE);
    Result1.AddGraph("right", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Generate::RGain, ResponseGraph::GF_None, CLR_RED);
    Result2.AddGraph("left", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractColumn, (void*)Generate::LDelay, ResponseGraph::GF_None, CLR_BLUE);
    Result2.AddGraph("right", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractColumn, (void*)Generate::RDelay, ResponseGraph::GF_None, CLR_RED);
    break;
   case GVM_Gain:
    text1 = " Left response ";
    text2 = " Right response ";
    Result1.AddGraph("result", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Generate::LGain, ResponseGraph::GF_None, CLR_BLACK);
    Result2.AddGraph("result", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)Generate::RGain, ResponseGraph::GF_None, CLR_BLACK);
    AddMeasureGraphs(Result1, &Frontend::XtractGain, Measure::LGain);
    AddMeasureGraphs(Result2, &Frontend::XtractGain, Measure::RGain);
    break;
   case GVM_Delay:
    text1 = " Left delay ";
    text2 = " Right delay ";
    Result1.AddGraph("result", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractColumn, (void*)Generate::LDelay, ResponseGraph::GF_None, CLR_BLACK);
    Result2.AddGraph("result", Generate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractColumn, (void*)Generate::RDelay, ResponseGraph::GF_None, CLR_BLACK);
    AddMeasureGraphs(Result1, &Frontend::XtractColumn, Measure::LDelay);
    AddMeasureGraphs(Result2, &Frontend::XtractColumn, Measure::RDelay);
    break;
  }
  ControlBase(+GetCtrl(GB_RESULT)).Text(text1);
  ControlBase(+GetCtrl(GB_RESULT2)).Text(text2);
  { SyncAccess<Generate::TargetFile> data(Generate::GetData());
    SetGraphAxes(*data);
  }
  InvalidateGraph();
}

static const LONG ColorMap[] =
{ CLR_BLUE
, CLR_RED
, CLR_GREEN
, CLR_PINK
, CLR_CYAN
, CLR_YELLOW
};

void Frontend::GeneratePage::AddMeasureGraphs(ResponseGraph& result, ResponseGraph::Extractor source, Measure::Column col)
{
  for (unsigned i = 0; i < MeasurementData.size() && i < sizeof ColorMap/sizeof *ColorMap; ++i)
  { DataFile& data = *MeasurementData[i];
    // title
    xstring caption = url123::normalizeURL(data.FileName).getShortName();
    if (caption.length() > 20)
    { xstring bak = caption;
      char* cp = caption.allocate(23);
      cp[0] = cp[1] = cp[2] = '.';
      memcpy(cp + 3, caption.cdata() + caption.length() - 20, 20);
    }
    // data
    result.AddGraph(caption, data, &Frontend::XtractFrequency, source, (void*)col, ResponseGraph::GF_None, ColorMap[i]);
  }
}

void Frontend::GeneratePage::Run()
{ // prepare generator
  sco_ptr<Generate> generator;
  { SyncAccess<Generate::TargetFile> data(Generate::GetData());
    generator = new Generate(*data);
  }
  // run
  try
  { generator->Run();
  } catch (const char* error)
  { (*Ctx.plugin_api->message_display)(MSG_ERROR, error);
    return;
  }
  // transfer data component of the measurements to MeasurementData
  MeasurementData.clear();
  foreach (Measure::MeasureFile,*const*, mp, generator->LocalData.Measurements)
  { DataFile* data = new DataFile(**mp);
    MeasurementData.append() = data;
    data->swap(**mp);
    // invert data
    foreach (DataRow,*const*, rp, *data)
    { DataRow& row = **rp;
      row[Measure::LGain] = 1 / row[Measure::LGain];
      row[Measure::RGain] = 1 / row[Measure::RGain];
      row[Measure::LDelay] = -row[Measure::LDelay];
      row[Measure::RDelay] = -row[Measure::RDelay];
    }
  }
  // replace result
  { SyncAccess<Generate::TargetFile> data(Generate::GetData());
    data->swap(generator->LocalData);
  }
  // and update graphs
  SetModified();
  SetupGraph();
}


Frontend::GenerateExtPage::GenerateExtPage(Frontend& parent)
: PageBase(parent, DLG_GENERATE_X, parent.ResModule, DF_AutoResize)
{ MinorTitle = "Extended filter generation options";
}

MRESULT Frontend::GenerateExtPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case DLG_GENERATE_X:
      if (SHORT2FROMMP(mp1) == BKN_PAGESELECTED)
      { const PAGESELECTNOTIFY& notify = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
        if (notify.ulPageIdCur == GetPageID())
          StoreControlValues();
        if (notify.ulPageIdNew == GetPageID())
          LoadControlValues();
      }
      break;
    }
    return 0;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_UNDO:
      LoadControlValues();
      break;
     case PB_DEFAULT:
      LoadDefaultValues();
      break;
    }
    return 0;
  }

  return PageBase::DlgProc(msg, mp1, mp2);
}

void Frontend::GenerateExtPage::LoadControlValues(const Generate::Parameters& data)
{
  SetValue(GetCtrl(EF_FREQ_LOW), data.FreqLow);
  SetValue(GetCtrl(EF_FREQ_HIGH), data.FreqHigh);
  SetValue(GetCtrl(EF_FREQ_BIN), data.FreqBin);
  SetValue(GetCtrl(EF_FREQ_FACTOR), data.FreqFactor * 100.);
  CheckBox(+GetCtrl(CB_SUBSONIC)).CheckState((data.Filter & Generate::FLT_Subsonic) != 0);
  CheckBox(+GetCtrl(CB_SUPERSONIC)).CheckState((data.Filter & Generate::FLT_Supersonic) != 0);
  SetValue(GetCtrl(EF_NORM_LOW), data.NormFreqLow);
  SetValue(GetCtrl(EF_NORM_HIGH), data.NormFreqHigh);
  RadioButton(+GetCtrl(RB_ENERGY+data.NormMode)).CheckState(true);

  SetValue(GetCtrl(EF_LIMITGAIN), 20*log10(data.LimitGain));
  SetValue(GetCtrl(EF_LIMITGAINRATE), data.LimitGainRate);
  CheckBox(+GetCtrl(CB_INVERTGAIN)).CheckState(data.InvertHighGain);
  SetValue(GetCtrl(EF_LIMITDELAY), data.LimitDelay);
  SetValue(GetCtrl(EF_LIMITDELAYRATE), data.LimitDelayRate);

  SetValue(GetCtrl(EF_GAIN_LOW), data.GainLow);
  SetValue(GetCtrl(EF_GAIN_HIGH), data.GainHigh);
  SetValue(GetCtrl(EF_DELAY_LOW), data.DelayLow);
  SetValue(GetCtrl(EF_DELAY_HIGH), data.DelayHigh);
}
void Frontend::GenerateExtPage::LoadControlValues()
{ SyncAccess<Generate::TargetFile> data(Generate::GetData());
  LoadControlValues(*data);
}

void Frontend::GenerateExtPage::StoreControlValues(Generate::Parameters& data)
{
  double tmp;
  GetValue(GetCtrl(EF_FREQ_LOW), data.FreqLow);
  GetValue(GetCtrl(EF_FREQ_HIGH), data.FreqHigh);
  GetValue(GetCtrl(EF_FREQ_BIN), data.FreqBin);
  if (GetValue(GetCtrl(EF_FREQ_FACTOR), tmp))
    data.FreqFactor = tmp / 100.;
  data.Filter
    = !!CheckBox(+GetCtrl(CB_SUBSONIC)).CheckState() * Generate::FLT_Subsonic
    | !!CheckBox(+GetCtrl(CB_SUPERSONIC)).CheckState() * Generate::FLT_Supersonic;
  GetValue(GetCtrl(EF_NORM_LOW), data.NormFreqLow);
  GetValue(GetCtrl(EF_NORM_HIGH), data.NormFreqHigh);
  data.NormMode = (Generate::NormalizeMode)(RadioButton(+GetCtrl(RB_ENERGY)).CheckID() - RB_ENERGY);

  if (GetValue(GetCtrl(EF_LIMITGAIN), tmp))
    data.LimitGain = pow(10, tmp/20);
  GetValue(GetCtrl(EF_LIMITGAINRATE), data.LimitGainRate);
  data.InvertHighGain = !!CheckBox(+GetCtrl(CB_INVERTGAIN)).CheckState();
  GetValue(GetCtrl(EF_LIMITDELAY), data.LimitDelay);
  GetValue(GetCtrl(EF_LIMITDELAYRATE), data.LimitDelayRate);

  GetValue(GetCtrl(EF_GAIN_LOW), data.GainLow);
  GetValue(GetCtrl(EF_GAIN_HIGH), data.GainHigh);
  GetValue(GetCtrl(EF_DELAY_LOW), data.DelayLow);
  GetValue(GetCtrl(EF_DELAY_HIGH), data.DelayHigh);
}
void Frontend::GenerateExtPage::StoreControlValues()
{ SyncAccess<Generate::TargetFile> data(Generate::GetData());
  StoreControlValues(*data);
}
