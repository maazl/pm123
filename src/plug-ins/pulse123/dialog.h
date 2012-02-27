/*
 * Copyright 2011 Marcel Mueller
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

#include "pulse123.h"
#include <plugin.h>
#include <pawrapper.h>
#include <cpp/windowbase.h>
#include <os2.h>

#include <debuglog.h>


class ConfigDialog : public NotebookDialogBase
{private:
  enum
  { UM_CONNECT = WM_USER+10,
    UM_STATE_CHANGE,
    UM_DISCOVER_SERVER,
    UM_UPDATE_SERVER
  };

  class PlaybackPage : public PageBase
  {private:
    xstring           PlaybackServer;
    PAContext         Context;
    PAServerInfoOperation ServerInfoOp;
    PAServerInfo      Server;
    PASinkInfoOperation SinkInfoOp;
    vector_own<PASinkInfo> Sinks;

    class_delegate<PlaybackPage,const pa_context_state_t> StateChangeDeleg;
    class_delegate<PlaybackPage,const pa_server_info> ServerInfoDeleg;
    class_delegate<PlaybackPage,const PASinkInfoOperation::Args> SinkInfoDeleg;
   public:
    PlaybackPage(ConfigDialog& parent);
    ~PlaybackPage();
   private:
    virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
            void      Cleanup();
            void      StateChangeHandler(const pa_context_state_t& args);
            void      ServerInfoHandler(const pa_server_info& info);
            void      SinkInfoHandler(const PASinkInfoOperation::Args& args);
  };
  class RecordPage : public PageBase
  {
   public:
    RecordPage(ConfigDialog& parent);
   private:
    virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  };

 public:
  ConfigDialog(HWND owner, HMODULE module);
 private:
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
};
