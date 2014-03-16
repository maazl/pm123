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


Frontend::DeconvolutionViewMode Frontend::DeconvolutionView = DVM_Target;


Frontend::DeconvolutionPage::DeconvolutionPage(Frontend& parent)
: MyPageBase(parent, DLG_DECONV)
, IterLGain(Generate::LGain)
, IterRGain(Generate::RGain)
, IterLDelay(Generate::LDelay)
, IterRDelay(Generate::RDelay)
, IterLKernel(TDC_LKernel)
, IterRKernel(TDC_RKernel)
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
    RadioButton(+GetCtrl(RB_VIEWTARGET + DeconvolutionView)).CheckState(true);
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
        PostCommand(PB_RELOAD);
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
       if (SHORT2FROMMP(mp1) == BN_CLICKED)
         SetModified(true);
       break;
     case RB_VIEWTARGET:
     case RB_VIEWGAIN:
     case RB_VIEWDELAY:
     case RB_VIEWTIME:
      if (SHORT2FROMMP(mp1) == BN_CLICKED)
      { DeconvolutionView = (DeconvolutionViewMode)(SHORT1FROMMP(mp1) - RB_VIEWTARGET);
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
    return 0;

   case UM_UPDATEGRAPH:
    UpdateGraph();
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
{ Result.ClearGraphs();
  switch (DeconvolutionView)
  {default: // DVM_TARGET
    Result.SetAxes(ResponseGraph::AF_LogX, 20,20000, -30,+20, -.2,.2);
    Result.AddGraph("< L gain", Kernel, IterLGain, ResponseGraph::GF_None, CLR_BLUE);
    Result.AddGraph("< R gain", Kernel, IterRGain, ResponseGraph::GF_None, CLR_RED);
    Result.AddGraph("L delay >", Kernel, IterLDelay, ResponseGraph::GF_Y2, CLR_GREEN);
    Result.AddGraph("R delay >", Kernel, IterRDelay, ResponseGraph::GF_Y2, CLR_PINK);
    break;
   case DVM_GainResult:
    Result.SetAxes(ResponseGraph::AF_LogX, 20,20000, -30,+20, NAN,NAN);
    RawKernelData = CurrentRawKernelData = new SharedDataFile(FDC_count);
    Result.AddGraph("< L gain", *CurrentRawKernelData, IterLGain, ResponseGraph::GF_None, CLR_BLUE);
    Result.AddGraph("< R gain", *CurrentRawKernelData, IterRGain, ResponseGraph::GF_None, CLR_RED);
    Result.AddGraph("< L gain", Kernel, IterLGain, ResponseGraph::GF_None, CLR_GREEN);
    Result.AddGraph("< R gain", Kernel, IterRGain, ResponseGraph::GF_None, CLR_PINK);
    Deconvolution::ForceKernelChange();
    break;
   case DVM_DelayResult:
    Result.SetAxes(ResponseGraph::AF_LogX, 20,20000, -.2,.2, NAN,NAN);
    RawKernelData = CurrentRawKernelData = new SharedDataFile(FDC_count);
    Result.AddGraph("L delay >", *CurrentRawKernelData, IterLDelay, ResponseGraph::GF_None, CLR_BLUE);
    Result.AddGraph("R delay >", *CurrentRawKernelData, IterRDelay, ResponseGraph::GF_None, CLR_RED);
    Result.AddGraph("L delay >", Kernel, IterLDelay, ResponseGraph::GF_None, CLR_GREEN);
    Result.AddGraph("R delay >", Kernel, IterRDelay, ResponseGraph::GF_None, CLR_PINK);
    Deconvolution::ForceKernelChange();
    break;
   case DVM_TimeResult:
    Result.SetAxes(ResponseGraph::AF_None, -1,+1, -2,2, NAN,NAN);
    RawKernelData = CurrentRawKernelData = new SharedDataFile(TDC_count);
    Result.AddGraph("left", *CurrentRawKernelData, IterLKernel, ResponseGraph::GF_None, CLR_BLUE);
    Result.AddGraph("right", *CurrentRawKernelData, IterRKernel, ResponseGraph::GF_None, CLR_RED);
    Deconvolution::ForceKernelChange();
    break;
  }
  Result.Invalidate();
}

void Frontend::DeconvolutionPage::UpdateGraph()
{ switch (DeconvolutionView)
  {default:
    return;
   case DVM_GainResult:
    Result.ClearGraphs();
    CurrentRawKernelData = RawKernelData;
    Result.AddGraph("< L gain", *CurrentRawKernelData, IterLGain, ResponseGraph::GF_None, CLR_BLUE);
    Result.AddGraph("< R gain", *CurrentRawKernelData, IterRGain, ResponseGraph::GF_None, CLR_RED);
    Result.AddGraph("< L gain", Kernel, IterLGain, ResponseGraph::GF_None, CLR_GREEN);
    Result.AddGraph("< R gain", Kernel, IterRGain, ResponseGraph::GF_None, CLR_PINK);
    break;
   case DVM_DelayResult:
    Result.ClearGraphs();
    CurrentRawKernelData = RawKernelData;
    Result.AddGraph("L delay >", *CurrentRawKernelData, IterLDelay, ResponseGraph::GF_None, CLR_BLUE);
    Result.AddGraph("R delay >", *CurrentRawKernelData, IterRDelay, ResponseGraph::GF_None, CLR_RED);
    Result.AddGraph("L delay >", Kernel, IterLDelay, ResponseGraph::GF_None, CLR_GREEN);
    Result.AddGraph("R delay >", Kernel, IterRDelay, ResponseGraph::GF_None, CLR_PINK);
    break;
   case DVM_TimeResult:
    Result.ClearGraphs();
    CurrentRawKernelData = RawKernelData;
    if (CurrentRawKernelData->size())
      Result.SetAxes(ResponseGraph::AF_None, (*(*CurrentRawKernelData)[0])[0],(*(*CurrentRawKernelData)[CurrentRawKernelData->size()-1])[0], -2,2, NAN,NAN);
    Result.AddGraph("left", *CurrentRawKernelData, IterLKernel, ResponseGraph::GF_Bounds, CLR_BLUE);
    Result.AddGraph("right", *CurrentRawKernelData, IterRKernel, ResponseGraph::GF_Bounds, CLR_RED);
    break;
  }
  Result.Invalidate();
}

void Frontend::DeconvolutionPage::KernelChange(const Deconvolution::KernelChangeEventArgs& args)
{
  if (args.Channel >= 2)
    return;

  bool needsetup = false;
  switch (DeconvolutionView)
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
