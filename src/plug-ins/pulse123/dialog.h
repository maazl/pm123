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
#include <decoder_plug.h>
#include <cpp/event.h>
#include <cpp/windowbase.h>
#include <os2.h>

#include <debuglog.h>


/// Base class for dialog with introspection API.
class IntrospectBase : public DialogBase
{protected:
  enum
  { UM_CONNECT = WM_USER+500,
    UM_STATE_CHANGE,
    UM_DISCOVER_SERVER,
    UM_UPDATE_SERVER,
    UM_UPDATE_PORT
  };
 protected:
  PAContext         Context;
  PAServerInfoOperation ServerInfoOp;
  PAServerInfo      Server;
  PAOperation*      InfoOp;
 private:
  class_delegate<IntrospectBase,const pa_context_state_t> StateChangeDeleg;
  class_delegate<IntrospectBase,const pa_server_info> ServerInfoDeleg;
 private:
          void      StateChangeHandler(const pa_context_state_t& args);
          void      ServerInfoHandler(const pa_server_info& info);
 protected:
  IntrospectBase(USHORT rid, HMODULE module);
  ~IntrospectBase();
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
          void      Cleanup();
};

/// pulse123 configuration dialog
class ConfigDialog : public IntrospectBase
{private:
  PASinkInfoOperation SinkInfoOp;
  vector_own<PASinkInfo> Sinks;
  int               SelectedSink;

  class_delegate<ConfigDialog,const PASinkInfoOperation::Args> SinkInfoDeleg;
 private:
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
          void      SinkInfoHandler(const PASinkInfoOperation::Args& args);

 public:
  /// Create configuration dialog. Call Process() to invoke it.
  ConfigDialog(HWND owner, HMODULE module);
};

/// pulse123 load wizard dialog
class LoadWizard : public IntrospectBase
{private:
  const xstring     Title;
 private:
  PASourceInfoOperation SourceInfoOp;
  vector_own<PASourceInfo> Sources;
  int               SelectedSource;

  class_delegate<LoadWizard,const PASourceInfoOperation::Args> SourceInfoDeleg;
 private:
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
          void      SourceInfoHandler(const PASourceInfoOperation::Args& args);

 public:
  /// Create wizard dialog. Call Process() to invoke it.
  LoadWizard(HMODULE module, HWND owner, const xstring& title);
};
