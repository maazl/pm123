/*
 * Copyright 2017-2017 Marcel Mueller
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

#ifndef PM123_METAWRITEDLG_H
#define PM123_METAWRITEDLG_H

#define INCL_BASE
#define INCL_PM

#include <format.h>
#include <decoder_plug.h>
#include "../core/aplayable.h"
#include "../pm123.h"
#include <cpp/windowbase.h>


/// Progress window for meta data writes. Includes asynchronous worker.
class MetaWriteDlg : private DialogBase
{public:
  META_INFO       MetaData;     ///< Information to write
  DECODERMETA     MetaFlags;    ///< Parts of MetaData to write
  const vector<APlayable>& Dest;///< Write to this songs
 private:
  // Friend nightmare because InfoDialogMetaWriteErrorHandler can't be a static member function.
  friend class InfoDialog;
  enum
  { UM_START = WM_USER+1,     ///< Start the worker thread
    UM_STATUS                 ///< Worker thread reports: at item #(ULONG)mp1
  };
  bool            SkipErrors;
 private: // Worker thread shared
  TID             WorkerTID;
  volatile unsigned CurrentItem;
  volatile bool   Cancel;
  Event           ResumeSignal;
  volatile int    RC;
  volatile xstring Text;
 private: // Worker thread
  void            Worker();
  friend void TFNENTRY InfoDialogMetaWriteWorkerStub(void*);
  friend void DLLENTRY InfoDialogMetaWriteErrorHandler(MetaWriteDlg* that, MESSAGE_TYPE type, const xstring& msg);
 protected:
  // Dialog procedure
  virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  MetaWriteDlg(const vector<APlayable>& dest);
  ~MetaWriteDlg();
  ULONG           DoDialog(HWND owner);
};

#endif // PM123_METAWRITEDLG_H
