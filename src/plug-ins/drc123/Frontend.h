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
#include "ResponseGraph.h"
#include "VUMeter.h"
#include <cpp/xstring.h>
#include <cpp/event.h>
#include <cpp/pmutils.h>
#include <cpp/windowbase.h>
#include <cpp/dlgcontrols.h>

#include <os2.h>


class Frontend : public NotebookDialogBase
{public:
  /// Currently active filter file or \c NULL if none.
  static xstring FilterFile;

 private:
  static double XtractFrequency(const DataRowType& row, void*);
  static double XtractGain(const DataRowType& row, void* col);
  static double XtractDelay(const DataRowType& row, void* col);

 private:
  static const ULONG TID_VU = 101;

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
    DataFile Kernel;
    ResponseGraph Result;
   public:
    DeconvolutionPage(Frontend& parent);
    ~DeconvolutionPage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   private:
    void UpdateDir();
  };

  class GeneratePage : public PageBase
  {public:
    GeneratePage(Frontend& parent)
    : PageBase(parent, DLG_GENERATE, parent.ResModule, DF_AutoResize)
    { MajorTitle = "~Generate";
      MinorTitle = "Generate deconvolution filter";
    }
    virtual ~GeneratePage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };

  class MeasurePage : public PageBase
  {public:
    MeasurePage(Frontend& parent)
    : PageBase(parent, DLG_MEASURE, parent.ResModule, DF_AutoResize)
    { MajorTitle = "~Measure";
      MinorTitle = "Measure room response";
    }
    virtual ~MeasurePage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };

  class CalibratePage : public PageBase
  {private:
    enum
    { UM_UPDATE = WM_USER
    , UM_VOLUME
    };
   private:
    VUMeter       VULeft;
    VUMeter       VURight;
    ResponseGraph Response;
    ResponseGraph XTalk;
    class_delegate<CalibratePage,const int> AnaUpdateDeleg;
   private:
    volatile bool UpdateRq;
    bool          VolumeRq;
   public:
    CalibratePage(Frontend& parent);
    virtual ~CalibratePage();
   protected:
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   private:
    void          LoadControlValues(const Calibrate::CalibrationFile& data);
    void          SetRunning(bool running);
    void          AnaUpdateNotify(const int&);
  };

 public:
  static void Init();

 public:
  Frontend(HWND owner, HMODULE module);
  //virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
};


#endif
