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
#include "FFT2Data.h"

#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <math.h>

#include <fileutil.h>
#include <cpp/dlgcontrols.h>
#include <cpp/directory.h>
#include <cpp/container/stringset.h>

#include <os2.h>

#include <debuglog.h>


const Frontend::DeconvolutionViewParams Frontend::DeconvolutionViewDefault =
{ DVM_Target
,   18,22000
,  -30,  +20
,  -.2,  +.2
,  -.5,  +.5, false
,   -5,   +5
};

Frontend::DeconvolutionViewParams Frontend::DeconvolutionView = DeconvolutionViewDefault;


Frontend::DeconvolutionPage::DeconvolutionPage(Frontend& parent)
: MyPageBase(parent, DLG_DECONV)
, Kernel(Generate::ColCount)
, IterLGain(Generate::LGain)
, IterRGain(Generate::RGain)
, IterLDelay(Generate::LDelay, false)
, IterRDelay(Generate::RDelay, false)
, IterLKernel(TDC_LKernel, false)
, IterRKernel(TDC_RKernel, false)
, KernelChangeDeleg(Deconvolution::GetKernelChange(), *this, &DeconvolutionPage::KernelChange)
{ DEBUGLOG(("Frontend::DeconvolutionPage(%p)::DeconvolutionPage(&%p)) - %p\n", this, &parent, &Params.TargetFile));
  MajorTitle = "~Playback";
  MinorTitle = "Deconvolution at playback";
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
    RadioButton(+GetCtrl(RB_VIEWTARGET + DeconvolutionView.ViewMode)).CheckState(true);
    PostCommand(PB_UNDO);
    PostMsg(UM_SETUPGRAPH, 0,0);
    break;

   case WM_DESTROY:
    KernelChangeDeleg.detach();
    Result.Detach();
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case DLG_DECONV:
      if (SHORT2FROMMP(mp1) == BKN_PAGESELECTED && ((PAGESELECTNOTIFY*)PVOIDFROMMP(mp2))->ulPageIdNew == GetPageID())
      { SetAxes();
        Result.Invalidate();
        PostCommand(PB_RELOAD);
      }
      break;
     case LB_KERNEL:
      if (SHORT2FROMMP(mp1) == LN_SELECT)
      { ListBox lb(+GetCtrl(LB_KERNEL));
        int sel = lb.NextSelection();
        if (sel >= 0)
        { Params.TargetFile = Filter::WorkDir;
          Params.TargetFile = Params.TargetFile + lb.ItemText(sel);
        } else
          Params.TargetFile = xstring::empty;
        EnsureMsg(UM_UPDATEKERNEL);
        SetModified(true);
      }
      break;
     case CB_FIRORDER:
      if (SHORT2FROMMP(mp1) == CBN_ENTER)
        SetModified(true);
      break;
     case RB_WIN_NONE:
     case RB_WIN_DIMMED_HAMMING:
     case RB_WIN_HAMMING:
     case CB_SUBSONIC:
     case CB_SUPERSONIC:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
        SetModified(true);
      break;
     case RB_VIEWTARGET:
     case RB_VIEWGAIN:
     case RB_VIEWDELAY:
     case RB_VIEWTIME:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      { DeconvolutionView.ViewMode = (DeconvolutionViewMode)(SHORT1FROMMP(mp1) - RB_VIEWTARGET);
        EnsureMsg(UM_SETUPGRAPH);
      }
      break;
     case CB_ENABLE:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      { Deconvolution::Parameters params;
        Deconvolution::GetParameters(params);
        params.Enabled = !!CheckBox(+GetCtrl(CB_ENABLE)).CheckState();
        Deconvolution::SetParameters(params);
      }
    }
    return 0;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_DEFAULT:
      Deconvolution::GetDefaultParameters(Params);
      SetModified(true);
      goto load;

     case PB_UNDO:
      // Load GUI from current configuration
      Deconvolution::GetParameters(Params);
      CheckBox(+GetCtrl(CB_ENABLE)).CheckState(Params.Enabled);
      SetModified(false);
     load:
      { // Populate list box with filter kernels
        PostCommand(PB_RELOAD);
        RadioButton(+GetCtrl(RB_WIN_NONE + Params.WindowFunction)).CheckState(true);
        CheckBox(+GetCtrl(CB_SUBSONIC)).CheckState((Params.Filter & Deconvolution::FLT_Subsonic) != 0);
        CheckBox(+GetCtrl(CB_SUPERSONIC)).CheckState((Params.Filter & Deconvolution::FLT_Supersonic) != 0);
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
        Params.Filter
          = !!CheckBox(+GetCtrl(CB_SUBSONIC)).CheckState() * Deconvolution::FLT_Subsonic
          | !!CheckBox(+GetCtrl(CB_SUPERSONIC)).CheckState() * Deconvolution::FLT_Supersonic;
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
        SetModified(false);
      }
      break;

     case PB_RELOAD:
      UpdateDir();
      break;
    }
    return 0;

   case UM_UPDATEKERNEL:
    UpdateKernel();
   case UM_INVALIDATEGRAPH:
    Result.Invalidate();
    return 0;

   case UM_SETUPGRAPH:
    SetupGraph();
    Result.Invalidate();
    return 0;

   case UM_UPDATEGRAPH:
    UpdateGraph();
    Result.Invalidate();
    return 0;
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

void Frontend::DeconvolutionPage::SetModified(bool modi)
{
  ControlBase(+GetCtrl(PB_APPLY)).Enabled(modi);
  ControlBase(+GetCtrl(PB_UNDO)).Enabled(modi);
}

void Frontend::DeconvolutionPage::UpdateDir()
{ DEBUGLOG(("Frontend::DeconvolutionPage(%p)::UpdateDir()\n", this));
  ListBox lb(+GetCtrl(LB_KERNEL));
  ControlBase descr(+GetCtrl(ST_DESCR));
  lb.DeleteAll();
  xstring path = Filter::WorkDir;
  if (!path.length())
  { descr.Text("No working directory");
    return;
  }
  DirScan dir(path, "*.target", FILE_ARCHIVED|FILE_READONLY|FILE_HIDDEN);
  stringsetI files;
  while (dir.Next() == 0)
    files.ensure(dir.CurrentFile());

  if (files.size())
    lb.InsertItems((const char*const*)files.begin(), files.size(), LIT_END);

  if (dir.LastRC() != ERROR_NO_MORE_FILES)
    descr.Text(dir.LastErrorText());
  else
  { if (Params.TargetFile.length())
    { // restore selection if possible
      unsigned pos;
      if (files.locate(sfnameext2(Params.TargetFile), pos))
        lb.Select(pos);
      else
      { Params.TargetFile = xstring::empty;
        CheckBox(+GetCtrl(CB_ENABLE)).Enabled(false);
      }
    }
    EnsureMsg(UM_UPDATEKERNEL);
  }
}

void Frontend::DeconvolutionPage::UpdateKernel()
{ DEBUGLOG(("Frontend::DeconvolutionPage(%p)::UpdateKernel()\n", this));
  ControlBase descr(+GetCtrl(ST_DESCR));
  ControlBase enabled(+GetCtrl(CB_ENABLE));
  if (!Params.TargetFile.length())
  { descr.Text("\33 select target file");
    goto disable;
  }
  if (!Kernel.Load(Params.TargetFile))
  { descr.Text(strerror(errno));
   disable:
    Result.Detach();
    enabled.Enabled(false);
    return;
  }
  Result.Attach(GetCtrl(CC_RESULT));
  descr.Text(Kernel.Description.length() ? Kernel.Description.cdata() : "no description");
  enabled.Enabled(true);
}

void Frontend::DeconvolutionPage::SetupGraph()
{ Result.Graphs.clear();
  SetAxes();
  switch (DeconvolutionView.ViewMode)
  {default: // DVM_TARGET
    Result.Graphs.append() = new ResponseGraph::GraphInfo("< L gain", Kernel, IterLGain, ResponseGraph::GF_Average, CLR_BLUE);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("< R gain", Kernel, IterRGain, ResponseGraph::GF_Average, CLR_RED);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("L delay >", Kernel, IterLDelay, ResponseGraph::GF_Y2|ResponseGraph::GF_Average, CLR_GREEN);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("R delay >", Kernel, IterRDelay, ResponseGraph::GF_Y2|ResponseGraph::GF_Average, CLR_PINK);
    break;
   case DVM_GainResult:
    RawKernelData = CurrentRawKernelData = new SharedDataFile(FDC_count);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("L result", *CurrentRawKernelData, IterLGain, ResponseGraph::GF_Average, CLR_BLUE);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("R result", *CurrentRawKernelData, IterRGain, ResponseGraph::GF_Average, CLR_RED);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("L target", Kernel, IterLGain, ResponseGraph::GF_Average, CLR_GREEN);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("R target", Kernel, IterRGain, ResponseGraph::GF_Average, CLR_PINK);
    Deconvolution::ForceKernelChange();
    break;
   case DVM_DelayResult:
    RawKernelData = CurrentRawKernelData = new SharedDataFile(FDC_count);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("L result", *CurrentRawKernelData, IterLDelay, ResponseGraph::GF_Average, CLR_BLUE);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("R result", *CurrentRawKernelData, IterRDelay, ResponseGraph::GF_Average, CLR_RED);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("L target", Kernel, IterLDelay, ResponseGraph::GF_Average, CLR_GREEN);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("R target", Kernel, IterRDelay, ResponseGraph::GF_Average, CLR_PINK);
    Deconvolution::ForceKernelChange();
    break;
   case DVM_TimeResult:
    RawKernelData = CurrentRawKernelData = new SharedDataFile(TDC_count);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("left", *CurrentRawKernelData, IterLKernel, ResponseGraph::GF_Bounds, CLR_BLUE);
    Result.Graphs.append() = new ResponseGraph::GraphInfo("right", *CurrentRawKernelData, IterRKernel, ResponseGraph::GF_Bounds, CLR_RED);
    Deconvolution::ForceKernelChange();
    break;
  }
}

void Frontend::DeconvolutionPage::SetAxes()
{ switch (DeconvolutionView.ViewMode)
  {default: // DVM_TARGET
    Result.SetAxes( ResponseGraph::AF_LogX,
      DeconvolutionView.FreqLow,  DeconvolutionView.FreqHigh,
      DeconvolutionView.GainLow,  DeconvolutionView.GainHigh,
      DeconvolutionView.DelayLow, DeconvolutionView.DelayHigh );
    break;
   case DVM_GainResult:
    Result.SetAxes( ResponseGraph::AF_LogX,
      DeconvolutionView.FreqLow, DeconvolutionView.FreqHigh,
      DeconvolutionView.GainLow, DeconvolutionView.GainHigh,
      NAN, NAN );
    break;
   case DVM_DelayResult:
    Result.SetAxes( ResponseGraph::AF_LogX,
      DeconvolutionView.FreqLow,  DeconvolutionView.FreqHigh,
      DeconvolutionView.DelayLow, DeconvolutionView.DelayHigh,
      NAN, NAN );
    break;
   case DVM_TimeResult:
    if (DeconvolutionView.TimeAuto && CurrentRawKernelData && CurrentRawKernelData->size())
      Result.SetAxes(ResponseGraph::AF_None,
        (*(*CurrentRawKernelData)[0])[0], (*(*CurrentRawKernelData)[CurrentRawKernelData->size()-1])[0],
        DeconvolutionView.KernelLow, DeconvolutionView.KernelHigh,
        NAN, NAN );
    else
      Result.SetAxes(ResponseGraph::AF_None,
        DeconvolutionView.TimeLow,   DeconvolutionView.TimeHigh,
        DeconvolutionView.KernelLow, DeconvolutionView.KernelHigh,
        NAN, NAN );
    break;
  }
}

void Frontend::DeconvolutionPage::UpdateGraph()
{ switch (DeconvolutionView.ViewMode)
  {default:
    return;
   case DVM_GainResult:
    CurrentRawKernelData = RawKernelData;
    Result.Graphs[0]->Data =
    Result.Graphs[1]->Data = *CurrentRawKernelData;
    break;
   case DVM_DelayResult:
    CurrentRawKernelData = RawKernelData;
    Result.Graphs[0]->Data =
    Result.Graphs[1]->Data = *CurrentRawKernelData;
    break;
   case DVM_TimeResult:
    CurrentRawKernelData = RawKernelData;
    if (DeconvolutionView.TimeAuto)
      SetAxes();
    Result.Graphs[0]->Data =
    Result.Graphs[1]->Data = *CurrentRawKernelData;
    break;
  }
}

void Frontend::DeconvolutionPage::KernelChange(const Deconvolution::KernelChangeEventArgs& args)
{
  if (args.Channel >= 2)
    return;

  bool needsetup = false;
  switch (DeconvolutionView.ViewMode)
  {default:
    return;

   case DVM_GainResult:
   case DVM_DelayResult:
    { // acquire exclusive access to FreqDomainKernel.
      int_ptr<SharedDataFile> kernel = RawKernelData;
      if (!kernel || !kernel->Mtx.Request(0))
      { // We failed to allocate the mutex. Create a new instance.
        if (kernel)
          kernel = new SharedDataFile(*kernel);
        else
          kernel = new SharedDataFile(FDC_count);
        kernel->Mtx.Request(); // can't block!
        needsetup = true;
      }
      FFT2Data f2d(*kernel, (double)args.Samplerate/args.Params->PlanSize, 1.01);
      f2d.Scale = args.FreqScale;
      f2d.StoreFFT(args.Channel ? FDC_RGain : FDC_LGain, *args.FreqDomain);
      kernel->Mtx.Release();
      kernel.swap(RawKernelData);
    }
    break;

   case DVM_TimeResult:
    { // acquire exclusive access to TimeDomainKernel.
      int_ptr<SharedDataFile> kernel = RawKernelData;
      if (!kernel || !kernel->Mtx.Request(0))
      { // We failed to allocate the mutex. Create a new instance.
        if (kernel)
          kernel = new SharedDataFile(*kernel);
        else
          kernel = new SharedDataFile(TDC_count);
        kernel->Mtx.Request(); // can't block!
        needsetup = true;
      }
      StoreIterator storer(*kernel, args.Channel ? TDC_RKernel : TDC_LKernel, 1);

      int count = -(args.Params->FIROrder >> 1);
      const float* sp = args.TimeDomain->end() + count;
      while (count < 0)
        storer.Store((float)count++ / args.Samplerate, *sp++ * args.TimeScale);
      sp = args.TimeDomain->begin();
      while (count < args.Params->FIROrder >> 1)
        storer.Store((float)count++ / args.Samplerate, *sp++ * args.TimeScale);
      kernel->Mtx.Release();
      kernel.swap(RawKernelData);
    }
    break;
  }
  EnsureMsg(needsetup ? UM_UPDATEGRAPH : UM_INVALIDATEGRAPH);
}


MRESULT Frontend::DeconvolutionExtPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {/*case WM_INITDLG:
    break;*/

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case DLG_DECONV_X:
      if (SHORT2FROMMP(mp1) == BKN_PAGESELECTED)
      { const PAGESELECTNOTIFY& notify = *(PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
        if (notify.ulPageIdCur == GetPageID())
          StoreControlValues();
        if (notify.ulPageIdNew == GetPageID())
          LoadControlValues();
      }
      break;
     case CB_TIME_AUTO:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      { bool en = !CheckBox(+GetCtrl(CB_TIME_AUTO)).CheckState();
        ControlBase(+GetCtrl(EF_TIME_LOW)).Enabled(en);
        ControlBase(+GetCtrl(EF_TIME_HIGH)).Enabled(en);
      }
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

void Frontend::DeconvolutionExtPage::LoadControlValues(const DeconvolutionViewParams& params)
{
  SetValue(+GetCtrl(EF_FREQ_LOW),   params.FreqLow);
  SetValue(+GetCtrl(EF_FREQ_HIGH),  params.FreqHigh);
  SetValue(+GetCtrl(EF_GAIN_LOW),   params.GainLow);
  SetValue(+GetCtrl(EF_GAIN_HIGH),  params.GainHigh);
  SetValue(+GetCtrl(EF_DELAY_LOW),  params.DelayLow);
  SetValue(+GetCtrl(EF_DELAY_HIGH), params.DelayHigh);
  ControlBase low (+GetCtrl(EF_TIME_LOW));
  ControlBase high(+GetCtrl(EF_TIME_HIGH));
  SetValue(low.Hwnd,                params.TimeLow);
  SetValue(high.Hwnd,               params.TimeHigh);
  CheckBox(+GetCtrl(CB_TIME_AUTO)).CheckState(params.TimeAuto);
  low. Enabled(!params.TimeAuto);
  high.Enabled(!params.TimeAuto);
  SetValue(+GetCtrl(EF_KERNEL_LOW), params.KernelLow);
  SetValue(+GetCtrl(EF_KERNEL_HIGH),params.KernelHigh);
}

void Frontend::DeconvolutionExtPage::StoreControlValues(DeconvolutionViewParams& params)
{
  GetValue(+GetCtrl(EF_FREQ_LOW),   params.FreqLow);
  GetValue(+GetCtrl(EF_FREQ_HIGH),  params.FreqHigh);
  GetValue(+GetCtrl(EF_GAIN_LOW),   params.GainLow);
  GetValue(+GetCtrl(EF_GAIN_HIGH),  params.GainHigh);
  GetValue(+GetCtrl(EF_DELAY_LOW),  params.DelayLow);
  GetValue(+GetCtrl(EF_DELAY_HIGH), params.DelayHigh);
  GetValue(+GetCtrl(EF_TIME_LOW),   params.TimeLow);
  GetValue(+GetCtrl(EF_TIME_HIGH),  params.TimeHigh);
  params.TimeAuto = !!CheckBox(+GetCtrl(CB_TIME_AUTO)).CheckState();
  GetValue(+GetCtrl(EF_KERNEL_LOW), params.KernelLow);
  GetValue(+GetCtrl(EF_KERNEL_HIGH),params.KernelHigh);
}
