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

#include <os2.h>

#include <debuglog.h>


Frontend::OpenLoopPage::OpenLoopPage(Frontend& parent, ULONG dlgid, const OpenLoop::SVTable& worker)
: FilePage(parent, dlgid)
, Worker(worker)
, UpdateRq(false)
, VolumeRq(false)
, AnaUpdateDeleg(*this, &OpenLoopPage::AnaUpdateNotify)
{}

MRESULT Frontend::OpenLoopPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_START:
      StoreControlValues();
      if ((*Worker.Start)())
        SetRunning(true);
      break;
     case PB_STOP:
      (*Worker.Stop)();
      SetRunning(false);
      break;
     case PB_CLEAR:
      (*Worker.Clear)();
      InvalidateGraph();
      break;
    }
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case SB_VOLUME:
      if (SHORT2FROMMP(mp1) == SPBN_CHANGE && !VolumeRq)
      { // defer execution to posted message
        VolumeRq = true;
        PostMsg(UM_VOLUME, 0,0);
      }
      return 0;

     case EF_FILE:
      if (SHORT2FROMMP(mp1) == EN_CHANGE)
        ControlBase(+GetCtrl(PB_SAVE)).Enabled(WinQueryWindowTextLength(HWNDFROMMP(mp2)) != 0);
      return 0;
    }
    break;

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
      InvalidateGraph();
    }
    return 0;

   case UM_VOLUME:
    if (VolumeRq)
    { VolumeRq = false;
      double volume = SpinButton(+GetCtrl(SB_VOLUME)).Value() / 100.;
      (*Worker.SetVolume)(volume);
      (*Worker.Clear)();
      InvalidateGraph();
    }
    return 0;
  }

  return FilePage::DlgProc(msg, mp1, mp2);
}

void Frontend::OpenLoopPage::LoadControlValues(const OpenLoop::OpenLoopFile& data)
{
  FilePage::LoadControlValues(data);

  SpinButton(+GetCtrl(SB_VOLUME)).Value((int)(data.RefVolume*100));

  VULeft.SetRange(data.VULow, 0, data.VUYellow, data.VURed);
  VURight.SetRange(data.VULow, 0, data.VUYellow, data.VURed);

  SetRunning((*Worker.IsRunning)());
}

void Frontend::OpenLoopPage::SetRunning(bool running)
{ DEBUGLOG(("Frontend::OpenLoopPage::SetRunning(%u)\n", running));
  ControlBase(+GetCtrl(PB_START)).Enabled(!running);
  ControlBase(+GetCtrl(PB_LOAD)).Enabled(!running);
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

bool Frontend::OpenLoopPage::LoadFile()
{ if ((*Worker.IsRunning)())
    return false;
  return FilePage::LoadFile();
}

void Frontend::OpenLoopPage::AnaUpdateNotify(const int&)
{ DEBUGLOG(("Frontend::OpenLoopPage(%p)::AnaUpdateNotify()\n", this));
  UpdateRq = true;
  PostMsg(UM_UPDATE, 0,0);
}


Frontend::ExtPage::ExtPage(Frontend& parent, ULONG dlgid)
: PageBase(parent, dlgid, parent.ResModule, DF_AutoResize)
{}

static const char* fftsize[] =
{ "32768 = 2^15 (~.7s)"
, "65536 = 2^16 (~1.4s)"
, "131072 = 2^17 (~2.7s)"
, "262144 = 2^18 (~5.5s)"
, "524288 = 2^19 (~11s)"
, "1048576 = 2^20 (~22s)"
};

MRESULT Frontend::ExtPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    ComboBox(+GetCtrl(CB_FFTSIZE)).InsertItems(fftsize, countof(fftsize));
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case EF_REFEXPONENT:
      if (SHORT2FROMMP(mp1) == EN_CHANGE)
      { double value;
        USHORT id = 0;
        if (GetValue(HWNDFROMMP(mp2), value))
        { if (value == 0.)
            id = RB_WHITE_N;
          else if (value == -.5)
            id = RB_PINK_N;
          else if (value == -1.)
            id = RB_BROWN_N;
        }
        if (id)
          RadioButton(+GetCtrl(id)).CheckState(true);
        else
          RadioButton(+GetCtrl(RB_WHITE_N)).UncheckAll();
      }
      break;
     case RB_WHITE_N:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
        ControlBase(+GetCtrl(EF_REFEXPONENT)).Text("0");
      break;
     case RB_PINK_N:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
        ControlBase(+GetCtrl(EF_REFEXPONENT)).Text("-0.5");
      break;
     case RB_BROWN_N:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
        ControlBase(+GetCtrl(EF_REFEXPONENT)).Text("-1");
      break;
     default:
      { if ( SHORT1FROMMP(mp1) == ControlBase(GetHwnd()).ID()
          && SHORT2FROMMP(mp1) == BKN_PAGESELECTED )
        { const PAGESELECTNOTIFY& notify = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
          if (notify.ulPageIdCur == GetPageID())
            StoreControlValues();
          if (notify.ulPageIdNew == GetPageID())
            LoadControlValues();
        }
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

void Frontend::ExtPage::LoadControlValues(const OpenLoop::OpenLoopFile& data)
{ DEBUGLOG(("Frontend::ExtPage(%p)::LoadControlValues(&%p)\n", this, &data));

  { ComboBox cbfftsize(GetCtrl(CB_FFTSIZE));
    int select;
    if ( frexp(data.FFTSize, &select) == .5 // is power of 2?
      && (unsigned)(select -= 16) >= countof(fftsize) )
    { // other value, not in fftsize[]
      char buffer[12];
      sprintf(buffer, "%i", data.FFTSize);
      if (cbfftsize.Count() < countof(fftsize))
        cbfftsize.InsertItem(buffer);
      else
        cbfftsize.ItemText(countof(fftsize), buffer);
      select = countof(fftsize);
    }
    cbfftsize.Select(select);
  }
  SetValue(GetCtrl(EF_DISCARD), (double)data.DiscardSamp / data.FFTSize, "%.1f");
  SetValue(GetCtrl(EF_FREQ_BIN), (data.AnaFBin - 1) * 100.);

  SetValue(GetCtrl(EF_FREQ_LOW), data.RefFMin);
  SetValue(GetCtrl(EF_FREQ_HIGH), data.RefFMax);
  SetValue(GetCtrl(EF_GAIN_LOW), data.GainLow);
  SetValue(GetCtrl(EF_GAIN_HIGH), data.GainHigh);
  SetValue(GetCtrl(EF_DELAY_LOW), data.DelayLow);
  SetValue(GetCtrl(EF_DELAY_HIGH), data.DelayHigh);
  SetValue(GetCtrl(EF_VU_LOW), data.VULow);
  SetValue(GetCtrl(EF_VU_YELLOW), data.VUYellow);
  SetValue(GetCtrl(EF_VU_RED), data.VURed);

  SetValue(GetCtrl(EF_REFEXPONENT), data.RefExponent);
  SetValue(GetCtrl(EF_REFFDIST), (data.RefFDist - 1) * 100.);
  // RB_xxx_N is synchronized by WM_CONTROL
  CheckBox(+GetCtrl(CB_SKIPEVEN)).CheckState(data.RefSkipEven);
}

void Frontend::ExtPage::StoreControlValues(OpenLoop::OpenLoopFile& data)
{ DEBUGLOG(("Frontend::ExtPage(%p)::StoreControlValues(&%p)\n", this, &data));

  { ComboBox cbfftsize(GetCtrl(CB_FFTSIZE));
    int selected = cbfftsize.NextSelection();
    if (selected == countof(fftsize))
      // custom entry
      data.FFTSize = atoi(cbfftsize.ItemText(countof(fftsize)));
    else if (selected >= 0)
      // std entry
      data.FFTSize = 1 << (15 + selected);
  }
  double tmp;
  if (GetValue(GetCtrl(EF_DISCARD), tmp))
    data.DiscardSamp = (unsigned)(data.FFTSize * tmp +.5);
  if (GetValue(GetCtrl(EF_FREQ_BIN), tmp))
    data.AnaFBin = tmp / 100. + 1;

  GetValue(GetCtrl(EF_FREQ_LOW), data.RefFMin);
  GetValue(GetCtrl(EF_FREQ_HIGH), data.RefFMax);
  GetValue(GetCtrl(EF_GAIN_LOW), data.GainLow);
  GetValue(GetCtrl(EF_GAIN_HIGH), data.GainHigh);
  GetValue(GetCtrl(EF_DELAY_LOW), data.DelayLow);
  GetValue(GetCtrl(EF_DELAY_HIGH), data.DelayHigh);
  GetValue(GetCtrl(EF_VU_LOW), data.VULow);
  GetValue(GetCtrl(EF_VU_YELLOW), data.VUYellow);
  GetValue(GetCtrl(EF_VU_RED), data.VURed);

  GetValue(GetCtrl(EF_REFEXPONENT), data.RefExponent);
  if (GetValue(GetCtrl(EF_REFFDIST), tmp))
    data.RefFDist = tmp / 100. + 1;
  data.RefSkipEven = CheckBox(+GetCtrl(CB_SKIPEVEN)).CheckState() & 1;
}
