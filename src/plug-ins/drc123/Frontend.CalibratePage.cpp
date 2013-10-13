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

#include "frontend.h"
#include "Calibrate.h"

#include <cpp/url123.h>


Frontend::CalibratePage::CalibratePage(Frontend& parent)
: PageBase(parent, DLG_CALIBRATE, parent.ResModule, DF_AutoResize)
, AnaUpdateDeleg(*this, &CalibratePage::AnaUpdateNotify)
, UpdateRq(false)
, VolumeRq(false)
{ MajorTitle = "~Calibrate";
  MinorTitle = "Calibrate sound card";
  VULeft  .SetRange(-40,0, -10,-3);
  VURight .SetRange(-40,0, -10,-3);
  Response.SetAxes(ResponseGraph::AF_LogX, 20,20000, -30,+20,  -10, +15);
  Response.AddGraph("< L gain dB", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)1, ResponseGraph::GF_None, CLR_BLUE);
  Response.AddGraph("< R gain dB", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)3, ResponseGraph::GF_None, CLR_RED);
  Response.AddGraph("L delay t >", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)2, ResponseGraph::GF_Y2, CLR_GREEN);
  Response.AddGraph("R delay t >", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)4, ResponseGraph::GF_Y2, CLR_PINK);
  XTalk.SetAxes(ResponseGraph::AF_LogX, 20,20000, -90,-40, -200,+300);
  XTalk.AddGraph("< R2L dB", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)5, ResponseGraph::GF_None, CLR_BLUE);
  XTalk.AddGraph("< L2R dB", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)7, ResponseGraph::GF_None, CLR_RED);
  XTalk.AddGraph("< L IM2 dB", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)9, ResponseGraph::GF_None, CLR_CYAN);
  XTalk.AddGraph("< R IM2 dB", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractGain, (void*)10, ResponseGraph::GF_None, CLR_YELLOW);
  XTalk.AddGraph("R2L del. t >", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)6, ResponseGraph::GF_Y2, CLR_GREEN);
  XTalk.AddGraph("L2R del. t >", Calibrate::GetData(), &Frontend::XtractFrequency, &Frontend::XtractDelay, (void*)8, ResponseGraph::GF_Y2, CLR_PINK);
}

Frontend::CalibratePage::~CalibratePage()
{}

MRESULT Frontend::CalibratePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    { Response.Attach(GetCtrl(CC_RESULT));
      XTalk.Attach(GetCtrl(CC_RESULT2));
      SetRunning(Calibrate::IsRunning());
      // Load initial values
      SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
      LoadControlValues(*data);
      break;
    }
   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_START:
      if (Calibrate::Start())
        SetRunning(true);
      break;
     case PB_STOP:
      Calibrate::Stop();
      SetRunning(false);
      break;
     case PB_CLEAR:
      { OpenLoop::Clear();
        { SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
          data->reset();
        }
        Response.Invalidate();
        XTalk.Invalidate();
        break;
      }
     case PB_LOAD:
      if (!Calibrate::IsRunning())
      { FILEDLG fd = { sizeof(FILEDLG) };
        fd.fl = FDS_OPEN_DIALOG|FDS_CENTER|FDS_FILTERUNION;
        fd.pszTitle = "Load DRC123 calibration file";
        // PM crashes if type is not writable
        char type[_MAX_PATH] = "DRC123 Calibration File (*.calibrate)";
        fd.pszIType = type;
        strncpy(fd.szFullFile, xstring(Filter::WorkDir), sizeof fd.szFullFile);
        (*Ctx.plugin_api->file_dlg)(HWND_DESKTOP, GetHwnd(), &fd);
        if (fd.lReturn == DID_OK)
        { SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
          if (!data->Load(fd.szFullFile))
          { // failed
            WinMessageBox(HWND_DESKTOP, GetHwnd(), strerror(errno), "Failed to load file", 0, MB_OK|MB_ERROR|MB_MOVEABLE);
          } else
          { LoadControlValues(*data);
            Response.Invalidate();
            XTalk.Invalidate();
          }
        }
      }
      break;
     case PB_SAVE:
      { const xstring& name = ControlBase(+GetCtrl(EF_FILE)).Text();
        if (name.length())
        { xstringbuilder file;
          file.append(xstring(Filter::WorkDir));
          file.append(name);
          file.append(".calibrate");
          SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
          // fetch description
          data->Description = ControlBase(+GetCtrl(ML_DESCR)).Text();
          // save
          if (!data->Save(file))
          { // failed
            file.append('\n');
            file.append(strerror(errno));
            WinMessageBox(HWND_DESKTOP, GetHwnd(), file, "Failed to save file", 0, MB_OK|MB_ERROR|MB_MOVEABLE);
          }
        }
      }
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

    /* case RB_STEREO_LOOP:
     case RB_MONO_LOOP:
     case RB_LEFT_LOOP:
     case RB_RIGHT_LOOP:
      if (SHORT2FROMMP(mp2) != BN_PAINT)
        Calibrate::CalData.Mode = (Calibrate::MeasureMode)(SHORT1FROMMP(mp1) - RB_STEREO_LOOP);
      break;*/

     case SB_VOLUME:
      if (SHORT2FROMMP(mp1) == SPBN_CHANGE && !VolumeRq)
      { // defer execution to posted message
        VolumeRq = true;
        PostMsg(UM_VOLUME, 0,0);
      }
      break;

     case EF_FILE:
      if (SHORT2FROMMP(mp1) == EN_CHANGE)
        ControlBase(+GetCtrl(PB_SAVE)).Enabled(WinQueryWindowTextLength(HWNDFROMMP(mp2)) != 0);
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
    if (UpdateRq)
    { UpdateRq = false;
      Response.Invalidate();
      XTalk.Invalidate();
    }
    return 0;

   case UM_VOLUME:
    if (VolumeRq)
    { VolumeRq = false;
      double volume = SpinButton(+GetCtrl(SB_VOLUME)).Value() / 100.;
      Calibrate::SetVolume(volume);
      SyncAccess<Calibrate::CalibrationFile> data(Calibrate::GetData());
      data->RefVolume = volume;
    }
    return 0;
  }

  return PageBase::DlgProc(msg, mp1, mp2);
}

void Frontend::CalibratePage::LoadControlValues(const Calibrate::CalibrationFile& data)
{
  RadioButton(+GetCtrl(RB_STEREO_LOOP + data.Mode)).CheckState(true);
  SpinButton(+GetCtrl(SB_VOLUME)).Value((int)(data.RefVolume*100));
  bool have_filename = data.FileName && data.FileName.length();
  ControlBase(+GetCtrl(PB_SAVE)).Enabled(have_filename);
  ControlBase(+GetCtrl(EF_FILE)).Text(have_filename ? url123(data.FileName).getShortName().cdata() : "");
}

void Frontend::CalibratePage::SetRunning(bool running)
{ DEBUGLOG(("Frontend::CalibratePage::SetRunning(%u)\n", running));
  ControlBase(+GetCtrl(PB_START)).Enabled(!running);
  ControlBase(+GetCtrl(PB_LOAD)).Enabled(!running);
  RadioButton(+GetCtrl(RB_STEREO_LOOP)).EnableAll(!running);
  if (running)
  { VULeft.Attach(GetCtrl(BX_LEFT));
    VURight.Attach(GetCtrl(BX_RIGHT));
    WinStartTimer(NULL, GetHwnd(), TID_VU, 100);
  } else
  { WinStopTimer(NULL, GetHwnd(), TID_VU);
    VULeft.Detach();
    VURight.Detach();
  }
}

void Frontend::CalibratePage::AnaUpdateNotify(const int&)
{ DEBUGLOG(("Frontend::CalibratePage::AnaUpdateNotify()\n"));
  UpdateRq = true;
  PostMsg(UM_UPDATE, 0,0);
}
