/*
 * Copyright 2013-2014 Marcel Mueller
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
#include "Calibrate.h"

#include <cpp/url123.h>


/*double Frontend::CalibratePage::XtractDeltaGain(const DataRowType& row, void* col)
{ return log(row[3] / row[1]) * (20. / M_LN10);
}

double Frontend::CalibratePage::XtractDeltaDelay(const DataRowType& row, void* col)
{ return (row[4] - row[2]) * row[0];
}*/

Frontend::CalibratePage::CalibratePage(Frontend& parent)
: OpenLoopPage(parent, DLG_CALIBRATE, Calibrate::VTable)
, IterLGain(Calibrate::LGain)
, IterRGain(Calibrate::RGain)
, IterDGain(Calibrate::DeltaGain)
, IterLDelay(Calibrate::LDelay)
, IterRDelay(Calibrate::RDelay)
, IterDDelay(Calibrate::DeltaDelay)
, IterR2LGain(Calibrate::R2LGain)
, IterL2RGain(Calibrate::L2RGain)
, IterLIntermod(Calibrate::LIntermod)
, IterRIntermod(Calibrate::RIntermod)
{ MajorTitle = "~Calibrate";
  MinorTitle = "Calibrate sound card";

  Response.Graphs.append() = new ResponseGraph::GraphInfo("< L gain",    Calibrate::GetData(), IterLGain,  ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_BLUE);
  Response.Graphs.append() = new ResponseGraph::GraphInfo("< R gain",    Calibrate::GetData(), IterRGain,  ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_RED);
  Response.Graphs.append() = new ResponseGraph::GraphInfo("L ph.del. >", Calibrate::GetData(), IterLDelay, ResponseGraph::GF_Y2|ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_GREEN);
  Response.Graphs.append() = new ResponseGraph::GraphInfo("R ph.del. >", Calibrate::GetData(), IterRDelay, ResponseGraph::GF_Y2|ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_PINK);
  Response.Graphs.append() = new ResponseGraph::GraphInfo("<  gain",    Calibrate::GetData(), IterDGain,  ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_CYAN);
  Response.Graphs.append() = new ResponseGraph::GraphInfo(" ph.del. >", Calibrate::GetData(), IterDDelay, ResponseGraph::GF_Y2|ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_YELLOW);
  XTalk.Graphs.append() = new ResponseGraph::GraphInfo("< R2L",   Calibrate::GetData(), IterR2LGain,   ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_BLUE);
  XTalk.Graphs.append() = new ResponseGraph::GraphInfo("< L2R",   Calibrate::GetData(), IterL2RGain,   ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_RED);
  XTalk.Graphs.append() = new ResponseGraph::GraphInfo("< L IM2", Calibrate::GetData(), IterLIntermod, ResponseGraph::GF_None|ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_CYAN);
  XTalk.Graphs.append() = new ResponseGraph::GraphInfo("< R IM2", Calibrate::GetData(), IterRIntermod, ResponseGraph::GF_None|ResponseGraph::GF_Bounds|ResponseGraph::GF_Average, CLR_YELLOW);
  //XTalk.AddGraph("R2L del. t >", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)6, ResponseGraph::GF_Y2, CLR_GREEN);
  //XTalk.AddGraph("L2R del. t >", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)8, ResponseGraph::GF_Y2, CLR_PINK);
}

Frontend::CalibratePage::~CalibratePage()
{}

MRESULT Frontend::CalibratePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG2(("Frontend::CalibratePage(%p)::DlgProc(%x, %p, %p)\n", this, msg, mp1, mp2));
  switch (msg)
  {case WM_INITDLG:
    Response.Attach(GetCtrl(CC_RESULT));
    XTalk.Attach(GetCtrl(CC_RESULT2));
    break;

   case WM_DESTROY:
    Response.Detach();
    XTalk.Detach();
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case RB_STEREO:
     case RB_LEFT:
     case RB_RIGHT:
     case RB_DIFF:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
        SetModified();
      return 0;
    }
    break;
  }

  return OpenLoopPage::DlgProc(msg, mp1, mp2);
}

void Frontend::CalibratePage::LoadControlValues(const Calibrate::CalibrationFile& data)
{
  RadioButton(+GetCtrl(RB_STEREO + data.Mode)).CheckState(true);

  OpenLoopPage::LoadControlValues(data);

  Response.SetAxes(ResponseGraph::AF_LogX, data.RefFMin, data.RefFMax,
    data.GainLow, data.GainHigh, data.DelayLow, data.DelayHigh);
  XTalk.SetAxes(ResponseGraph::AF_LogX, data.RefFMin, data.RefFMax,
    data.Gain2Low, data.Gain2High, NAN,NAN);
}
void Frontend::CalibratePage::LoadControlValues()
{ SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  LoadControlValues(*data);
}

void Frontend::CalibratePage::StoreControlValues(Calibrate::CalibrationFile& data)
{
  data.Mode = (Calibrate::MeasureMode)(RadioButton(+GetCtrl(RB_STEREO)).CheckID() - RB_STEREO);

  OpenLoopPage::StoreControlValues(data);

  if (data.FileName.length())
    data.FileName = data.FileName + ".calibrate";
}
void Frontend::CalibratePage::StoreControlValues()
{ SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  StoreControlValues(*data);
}

void Frontend::CalibratePage::SetRunning(bool running)
{
  RadioButton(+GetCtrl(RB_STEREO)).EnableAll(!running);
  OpenLoopPage::SetRunning(running);
}

void Frontend::CalibratePage::InvalidateGraph()
{ Response.Invalidate();
  XTalk.Invalidate();
  SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  ControlBase(+GetCtrl(ST_SUBRESULT)).Text(xstring().sprintf( "average delay: %.1f / %.1f ms, indeterminate phase: %u / %u",
    1E3 * data->AverageDelay[0], 1E3 * data->AverageDelay[1],
    data->IndeterminatePhase[0], data->IndeterminatePhase[1] ));
}

LONG Frontend::CalibratePage::DoLoadFileDlg(FILEDLG& fdlg)
{ fdlg.pszTitle = "Load DRC123 calibration file";
  // PM crashes if type is not writable
  char type[_MAX_PATH] = "DRC123 Calibration File (*.calibrate)";
  fdlg.pszIType = type;
  return OpenLoopPage::DoLoadFileDlg(fdlg);
}

bool Frontend::CalibratePage::DoLoadFile(const char* filename)
{ SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  return data->Load(filename);
}

xstring Frontend::CalibratePage::DoSaveFile()
{ SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  if (data->Save(data->FileName))
    return xstring();
  return data->FileName;
}

/*MRESULT Frontend::CalibrateExtPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return PageBase::DlgProc(msg, mp1, mp2);
}*/

void Frontend::CalibrateExtPage::LoadControlValues(const Calibrate::CalibrationFile& data)
{
  ExtPage::LoadControlValues(data);

  SetValue(GetCtrl(EF_GAIN2_LOW), data.Gain2Low);
  SetValue(GetCtrl(EF_GAIN2_HIGH), data.Gain2High);
}
void Frontend::CalibrateExtPage::LoadControlValues()
{ SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  LoadControlValues(*data);
}

void Frontend::CalibrateExtPage::LoadDefaultValues()
{ LoadControlValues(Calibrate::DefData);
}

void Frontend::CalibrateExtPage::StoreControlValues(Calibrate::CalibrationFile& data)
{
  ExtPage::StoreControlValues(data);

  GetValue(GetCtrl(EF_GAIN2_LOW), data.Gain2Low);
  GetValue(GetCtrl(EF_GAIN2_HIGH), data.Gain2High);
}
void Frontend::CalibrateExtPage::StoreControlValues()
{ SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
  StoreControlValues(*data);
}

