/*
 * Copyright 2012-2014 Marcel Mueller
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


/** @brief Class to handle the configuration dialog of DRC123.
 * @details The dialog is non modal an invoked by the function \c Show.
 * @remarks The Class instance \e is created with new, but the object lifetime management
 * is internal. It is destroyed with the dialog window.
 */
class Frontend : public ManagedDialog<NotebookDialogBase>
{public:
  /// View mode of the graph in the deconvolution page.
  enum DeconvolutionViewMode
  { DVM_Target          ///< Show the selected target response only.
  , DVM_GainResult      ///< Show gain of the target response an the last filter kernel used by DRC123.
  , DVM_DelayResult     ///< Show group delay of the target response an the last filter kernel used by DRC123.
  , DVM_TimeResult      ///< Show time domain data of the last used filter kernel.
  };
  /// View mode of the graphs in the generate page.
  enum GenerateViewMode
  { GVM_Result          ///< View resulting target response only.
  , GVM_Gain            ///< Show gain of the target response and the contributing measurements.
  , GVM_Delay           ///< Show group delay of the target response and the contributing measurements.
  };

 public:
  /// Currently active graph view in the deconvolution page.
  static DeconvolutionViewMode DeconvolutionView;
  /// Currently active graph view in the generate page.
  static GenerateViewMode GenerateView;

 private:
  /// Extractor for ReponseGraph: X-axis = first column = frequency.
  static double XtractFrequency(const DataRow& row, void*);
  /// Extractor for ReponseGraph: Y-axis, select a column unconverted.
  static double XtractColumn(const DataRow& row, void* col);
  /// Extractor for ReponseGraph: Y-axis, select a column and convert from gain to dB.
  static double XtractGain(const DataRow& row, void* col);
  /// Extractor for ReponseGraph: Y-axis, select a column and convert from group delay to a phase angle at the current frequency.
  static double XtractPhaseDelay(const DataRow& row, void* col);

  /// Helper function to set the window text of a numeric text box.
  /// @param ctrl Control to manipulate.
  /// @param value Value to set.
  /// @param mask \c printf mask used to convert value to a string.
  static void   SetValue(HWND ctrl, double value, const char* mask = "%g");
  /// Helper function to set the window text of a numeric text box.
  /// @param ctrl Control where the text is read.
  /// @param value [out] Current value.
  /// @return \c true if the value could be successfully extracted. \c false if it is not numeric.
  static bool   GetValue(HWND ctrl, double& value);

 private:
  /// Page #1 of the dialog: Configuration
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

  /// Deconvolution (playback) control page.
  class DeconvolutionPage : public PageBase
  {private:
    enum
    { UM_UPDATEKERNEL = WM_USER + 300
    , UM_SETUPGRAPH
    , UM_UPDATEGRAPH
    , UM_INVALIDATEGRAPH
    };
    Deconvolution::Parameters Params;
    DataFile      Kernel;
    ResponseGraph Result;

    class_delegate<DeconvolutionPage,const Deconvolution::KernelChangeEventArgs> KernelChangeDeleg;
    enum FrequencyDomainColumn
    { FDC_Frequency
    , FDC_LGain
    , FDC_LDelay
    , FDC_RGain
    , FDC_RDelay
    , FDC_count
    };
    enum TimeDomainColumn
    { TDC_Frequency
    , TDC_LKernel
    , TDC_RKernel
    , TDC_count
    };
    volatile int_ptr<SharedDataFile> RawKernelData;
    int_ptr<SharedDataFile> CurrentRawKernelData;
   public:
    DeconvolutionPage(Frontend& parent);
    ~DeconvolutionPage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   private:
    void          SetModified(bool modi);
    void          UpdateDir();
    void          UpdateKernel();
    void          SetupGraph();
    void          UpdateGraph();
    void          KernelChange(const Deconvolution::KernelChangeEventArgs& args);
  };

  /// @brief Abstract base page for dialog pages that are based on a single \c DataFile.
  /// @details The class handles a entry field for the file name \c EF_FILE,
  /// a MLE control for the file description \c ML_DESCR as well as a load and save button \c PB_LOAD and \c PB_SAVE.
  class FilePage : public PageBase
  {private:
    bool          Modified;
   protected:
    FilePage(Frontend& parent, ULONG dlgid) : PageBase(parent, dlgid, parent.ResModule, DF_AutoResize), Modified(false) {}
    void          SetModified();
    void          ClearModified();
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
    /// @brief Partial update control state from file.
    /// @details This function only handles the controls handled by this abstract class.
    /// The function should be invoked form \c LoadControlValues() with the appropriate data.
    /// @param data Data source.
    void          LoadControlValues(const DataFile& data);
    /// @brief Update control state from file.
    /// @details You must override this function and call @code LoadControlValues(const DataFile& data) @endcode from there.
    /// Furthermore you need to handle any other controls here that are unknown to this base class.
    virtual void  LoadControlValues() = 0;
    /// @brief Partial update control \c DataFile content from control values.
    /// @details This function only handles the controls handled by this abstract class.
    /// The function should be invoked form \c StoreControlValues() with the appropriate data.
    /// @param data Target data.
    void          StoreControlValues(DataFile& data);
    /// @brief Update control \c DataFile content from control values.
    /// @details You must override this function and call @code StoreControlValues(DataFile& data) @endcode from there.
    /// Furthermore you need to handle any other controls here that are unknown to this base class.
    virtual void  StoreControlValues() = 0;

    /// @brief Invoke a file dialog to select a a file to load.
    /// @details Although the function is abstract it contains a base implementation to invoke the dialog.
    /// But it need to be overridden and the \c FILEDLG structure needs to be filled correctly
    /// before the base implementation is called. You should at least set a title and a file type.
    /// The Function is invoked from \c LoadFile().
    /// @return Returns the lReturn member of the \c FILEDLG structure.
    virtual LONG  DoLoadFileDlg(FILEDLG& fdlg) = 0;
    /// Handle \c PB_LOAD button.
    /// @return \c true if the operation succeeded, i.e. the user pressed \c DID_OK.
    /// In this case the \c LoadControlValues() is invoked.
    virtual bool  LoadFile();
    /// Handle \c PB_SAVE button.
    /// @return \c true if the operation succeeded.
    virtual bool  SaveFile();
    /// @brief Function called when a file content is to be loaded.
    /// @param filename Full name of the file to load.
    /// Might be \c NULL to load the last file again. This is consistent with \c DataFile::Load.
    /// @return true: operation succeeded, usually the return code from \c DataFile::Load.
    /// @details The function is called
    /// - if the dialog is initialized first to load any initial file content,
    /// - at a \c PB_RELOAD button press and
    /// - after the user selected a file to load.
    /// @remarks The initial call is intended to reload the file that was open the last time the dialog was used.
    virtual bool  DoLoadFile(const char* filename) = 0;
    /// Save the current \c DataFile content to the current file name. Must be implemented.
    /// @return Error message or \c NULL if the operation succeeded.
    virtual xstring DoSaveFile() = 0;
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
    virtual LONG  DoLoadFileDlg(FILEDLG& fdlg);
    virtual bool  DoLoadFile(const char* filename);
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

  /// @brief Base page for dialogs that operate with \c OpenLoop instances.
  /// @details This class handles all controls of \c FilePage and additionally
  /// a \c PB_START, \c PB_STOP and \c PB_CLEAR button as well as a volume control
  /// spin button \c SB_VOLUME and VU controls for the current line in level.
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
    virtual LONG  DoLoadFileDlg(FILEDLG& fdlg) = 0;
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
    { /// Update combo boxes with calibration files
      /// @param mp1 (bool) load selection from data
      UM_UPDATECALLIST = WM_USER + 300,
      /// Update description of calibration file
      /// @param mp1 (HWND) window handle of matching drop down list
      /// @param mp2 (SHORT) ID of description control to update
      UM_UPDATEFILE
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
            void  UpdateDir(ComboBox lb, ControlBase desc, const char* mask, xstring& selection);
    static  xstring UpdateFile(ComboBox lb, ControlBase desc);
    virtual LONG  DoLoadFileDlg(FILEDLG& fdlg);
    virtual bool  DoLoadFile(const char* filename);
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
    void          LoadControlValues(const Measure::MeasureFile& data);
    virtual void  LoadControlValues();
    virtual void  LoadDefaultValues();
    void          StoreControlValues(Measure::MeasureFile& data);
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
    virtual LONG  DoLoadFileDlg(FILEDLG& fdlg);
    virtual bool  DoLoadFile(const char* filename);
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
  static  void    Init();
  static  HWND    Show(HWND owner, HMODULE module);
 private:
  static int_ptr<Frontend> Instance;

                  Frontend(HWND owner, HMODULE module);
  //virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual void    OnDestroy();
};


#endif
