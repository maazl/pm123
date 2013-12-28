/*
 * Copyright 2012-2013 Marcel Mueller
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

#ifndef FRONTEND_H
#define FRONTEND_H

#define  INCL_WIN
#include "drc123.h"
#include "DataFile.h"
#include "Deconvolution.h"
#include "Calibrate.h"
#include "Measure.h"
#include "Generate.h"
#include "ResponseGraph.h"
#include "VUMeter.h"
#include <cpp/xstring.h>
#include <cpp/container/stringset.h>
#include <cpp/container/sorted_vector.h>
#include <cpp/event.h>
#include <cpp/pmutils.h>
#include <cpp/windowbase.h>
#include <cpp/dlgcontrols.h>

#include <os2.h>


class Frontend : public NotebookDialogBase
{public:
  enum ViewMode
  { VM_Result
  , VM_Gain
  , VM_Delay
  };

 public:
  static ViewMode GenerateViewMode;

 private:
  static double XtractFrequency(const DataRow& row, void*);
  static double XtractGain(const DataRow& row, void* col);
  static double XtractDelay(const DataRow& row, void* col);
  static double XtractPhaseDelay(const DataRow& row, void* col);

  static void   SetValue(HWND ctrl, double value, const char* mask = "%g");
  static bool   GetValue(HWND ctrl, double& value);

 private:
  class ConfigurationPage : public PageBase
  {public:
    ConfigurationPage(Frontend& parent)
    : PageBase(parent, DLG_CONFIG, parent.ResModule, DF_AutoResize)
    { MajorTitle = "~Configure";
      MinorTitle = "Configuration";
    }
    virtual ~ConfigurationPage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };

  class DeconvolutionPage : public PageBase
  {private:
    enum
    { UM_UPDATEDESCR = WM_USER + 300
    };
    Deconvolution::Parameters Params;
    DataFile      Kernel;
    ResponseGraph Result;
   public:
    DeconvolutionPage(Frontend& parent);
    ~DeconvolutionPage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   private:
    void          UpdateDir();
  };

  class FilePage : public PageBase
  {
   protected:
    FilePage(Frontend& parent, ULONG dlgid) : PageBase(parent, dlgid, parent.ResModule, DF_AutoResize) {}
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void          LoadControlValues(const DataFile& data);
    virtual void  LoadControlValues() = 0;
    void          StoreControlValues(DataFile& data);
    virtual void  StoreControlValues() = 0;
    virtual LONG  DoLoadFile(FILEDLG& fdlg) = 0;
    virtual xstring DoSaveFile() = 0;
    virtual bool  LoadFile();
    virtual void  SaveFile();
  };

  class GeneratePage : public FilePage
  {private:
    enum
    { UM_UPDATEDIR   = WM_USER + 300
    , UM_UPDATEDESCR
    , UM_UPDATEGRAPH
    };
   private:
    ResponseGraph Result1;
    ResponseGraph Result2;
    /// Backup copy of measurement data for graphs.
    vector_own<DataFile> MeasurementData;
   public:
    GeneratePage(Frontend& parent);
    virtual ~GeneratePage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    virtual void  LoadControlValues();
    virtual void  StoreControlValues();
    virtual LONG  DoLoadFile(FILEDLG& fdlg);
    virtual xstring DoSaveFile();
   private:
    void          LoadControlValues(const Generate::TargetFile& data);
    void          UpdateDir();
    void          UpdateDescr();
    void          InvalidateGraph();
    void          SetGraphAxes(const Generate::TargetFile& data);
    void          SetupGraph();
    void          AddMeasureGraphs(ResponseGraph& result, ResponseGraph::Extractor source, Measure::Column col);
    void          Run();
  };

  class GenerateExtPage : public PageBase
  {public:
    GenerateExtPage(Frontend& parent);
    virtual ~GenerateExtPage() {}
   private:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void          LoadControlValues(const Generate::Parameters& params);
    void          LoadControlValues();
    void          LoadDefaultValues()   { LoadControlValues(Generate::DefData); }
    void          StoreControlValues(Generate::Parameters& params);
    void          StoreControlValues();
  };

  class OpenLoopPage : public FilePage
  {protected:
    enum
    { UM_UPDATE = WM_USER
    , UM_VOLUME
    };
    enum
    { TID_VU = 101
    };
   protected:
    const OpenLoop::SVTable& Worker;
   private:
    VUMeter       VULeft;
    VUMeter       VURight;
   private:
    volatile bool UpdateRq;
    bool          VolumeRq;
    class_delegate<OpenLoopPage,const int> AnaUpdateDeleg;
   protected:
    OpenLoopPage(Frontend& parent, ULONG dlgid, const OpenLoop::SVTable& worker);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void          LoadControlValues(const OpenLoop::OpenLoopFile& data);
    //void          StoreControlValues(OpenLoop::OpenLoopFile& data) {} // currently no-op
    virtual void  SetRunning(bool running);
    virtual void  InvalidateGraph() = 0;
    virtual bool  LoadFile();
   private:
    void          AnaUpdateNotify(const int&);
  };

  class ExtPage : public PageBase
  {public:
    ExtPage(Frontend& parent, ULONG dlgid);
    virtual ~ExtPage() {}
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void          LoadControlValues(const OpenLoop::OpenLoopFile& data);
    virtual void  LoadControlValues() = 0;
    virtual void  LoadDefaultValues() = 0;
    void          StoreControlValues(OpenLoop::OpenLoopFile& data);
    virtual void  StoreControlValues() = 0;
  };

  class MeasurePage : public OpenLoopPage
  {private:
    enum
    { UM_UPDATECALLIST = WM_USER + 300  ///< Update combo box with calibration files
    , UM_UPDATEFILE                     ///< Update calibration file description
    };

   private:
    ResponseGraph Response;
   public:
    MeasurePage(Frontend& parent);
    virtual ~MeasurePage();
   private:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void          LoadControlValues(const Measure::MeasureFile& data);
    virtual void  LoadControlValues();
    void          StoreControlValues(Measure::MeasureFile& data);
    virtual void  StoreControlValues();
    virtual void  SetRunning(bool running);
    virtual void  InvalidateGraph();
    static  void  UpdateDir(ComboBox lb, ControlBase desc, const char* mask);
    static  xstring UpdateFile(ComboBox lb, ControlBase desc);
    virtual LONG  DoLoadFile(FILEDLG& fdlg);
    virtual xstring DoSaveFile();
  };

  class MeasureExtPage : public ExtPage
  {public:
    MeasureExtPage(Frontend& parent)
    : ExtPage(parent, DLG_MEASURE_X)
    { MinorTitle = "Extended measurement options";
    }
    virtual ~MeasureExtPage() {}
   protected:
    //virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    //void          LoadControlValues(const Measure::MeasureFile& data, const GUIParameters& gui);
    virtual void  LoadControlValues();
    virtual void  LoadDefaultValues();
    //void          StoreControlValues(Measure::MeasureFile& data, GUIParameters& gui);
    virtual void  StoreControlValues();
  };

  class CalibratePage : public OpenLoopPage
  {private:
    ResponseGraph Response;
    ResponseGraph XTalk;
   /*private:
    static double XtractDeltaGain(const DataRowType& row, void* col);
    static double XtractDeltaDelay(const DataRowType& row, void* col);*/
   public:
    CalibratePage(Frontend& parent);
    virtual ~CalibratePage();
   private:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void          LoadControlValues(const Calibrate::CalibrationFile& data);
    virtual void  LoadControlValues();
    void          StoreControlValues(Calibrate::CalibrationFile& data);
    virtual void  StoreControlValues();
    virtual void  SetRunning(bool running);
    virtual void  InvalidateGraph();
    virtual LONG  DoLoadFile(FILEDLG& fdlg);
    virtual xstring DoSaveFile();
  };

  class CalibrateExtPage : public ExtPage
  {public:
    CalibrateExtPage(Frontend& parent)
    : ExtPage(parent, DLG_CALIBRATE_X)
    { MinorTitle = "Extended calibration options";
    }
    virtual ~CalibrateExtPage() {}
   protected:
    //virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    void          LoadControlValues(const Calibrate::CalibrationFile& data);
    virtual void  LoadControlValues();
    virtual void  LoadDefaultValues();
    void          StoreControlValues(Calibrate::CalibrationFile& data);
    virtual void  StoreControlValues();
  };

 public:
  static void Init();

 public:
  Frontend(HWND owner, HMODULE module);
  //virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
};


#endif
