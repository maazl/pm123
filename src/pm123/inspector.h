/*
 * Copyright 2008-2008 Marcel Mueller
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

#ifndef PM123_INSPECTOR_H
#define PM123_INSPECTOR_H

#define INCL_WIN
#define INCL_BASE
#include "windowbase.h"
#include <cpp/smartptr.h>
#include <cpp/queue.h>
#include <cpp/container/vector.h>
#include <os2.h>


/****************************************************************************
*
*  Class to handle info dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class InspectorDialog 
: public DialogBase
, public Iref_count
{private:
  enum
  { UM_REFRESH = WM_USER+1
  };

 private:
  vector<char>      ControllerData;
  vector<char>      WorkerData;

 private: // non copyable
                    InspectorDialog(const InspectorDialog&);
  void              operator=(const InspectorDialog&);
 private:
  // Initialize InfoDialog
                    InspectorDialog();
  //void              StartDialog();
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  // Destruction callback
  virtual void      OnDestroy();
  // Refresh the list boxes
  void              Refresh();
  
  static void       DiscardData(vector<char>& data);

 public:
  virtual           ~InspectorDialog();
  virtual void      SetVisible(bool show);

 private:
  static volatile int_ptr<InspectorDialog> Instance;
 public:
  // Fatory method
  static int_ptr<InspectorDialog> GetInstance();
  static void       UnInit();
};

#endif
