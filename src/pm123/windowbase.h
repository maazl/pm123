/*
 * Copyright 2007-2008 Marcel Mueller
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

#ifndef WINDOWBASE_H
#define WINDOWBASE_H

#define INCL_WIN

#include <cpp/smartptr.h>
#include <cpp/xstring.h>

#include <os2.h>

#include <debuglog.h>


/****************************************************************************
*
*  This abstract class represents an instance of a OS/2 PM dialog window.
*
****************************************************************************/
class DialogBase
{private:
  // wrap pointer to keep PM happy
  struct init_dlg_struct
  { USHORT      size;
    DialogBase* pm;
  };

 protected: // content
  const ULONG       DlgRID;        // Resource ID of the dialog template
  const HMODULE     ResModule;     // Module handle for the above resource
 private:
  HWND              HwndFrame;     // playlist window handle
  bool              InitialVisible;
  int               Initialized;
  xstring           Title;         // Keep the window title storage

 private:
  // Static members must not use EXPENTRY linkage with IBM VACPP.
  friend MRESULT EXPENTRY wb_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

 protected:
  // Create a playlist manager window for an object, but don't open it.
  DialogBase(ULONG rid, HMODULE module);

  // load dialog ressources and create window
  void              StartDialog(HWND parent, HWND owner = HWND_DESKTOP);
  // Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  // Dialog initialization
  virtual void      OnInit();
  // Destruction callback
  virtual void      OnDestroy();

  // Get the window title
  xstring           GetTitle() const { return Title; };
  // Set the window title
  void              SetTitle(const xstring& title);

 public: // public interface
  virtual           ~DialogBase();
  // Get the window handle
  HWND              GetHwnd() const { ASSERT(HwndFrame != NULLHANDLE); return HwndFrame; }
  // Make the window visible (or not)
  virtual void      SetVisible(bool show);
  bool              GetVisible() const;
  // Force the window to close
  void              Destroy() { WinDestroyWindow(HwndFrame); }
};


class ManagedDialogBase : public DialogBase, public Iref_Count
{private:
  int_ptr<ManagedDialogBase> Self;
 protected:
  virtual void      OnInit();
  // If you overload OnDestroy you must mot access this after ManagedDialogBase::OnDestroy returned.
  virtual void      OnDestroy();
 public:
  ManagedDialogBase(ULONG rid, HMODULE module) : DialogBase(rid, module) {}
};


/****************************************************************************
*
*  Class to implement multiple sub windows with different window prcedures
*  in one worker class. Remember to call StartDialog before using this class.
*
****************************************************************************/
/*template <class W>
class DialogSubWindow : public DialogBase
{protected:
  W&                Worker;
  MRESULT (W::*const Function)(DialogSubWindow<W>& wnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  DialogSubWindow(W& worker, MRESULT (W::*function)(DialogSubWindow<W>& wnd, ULONG msg, MPARAM mp1, MPARAM mp2),
                  ULONG rid, HMODULE module, HWND owner, HWND parent)
    : DialogBase(rid, module, owner, parent), Worker(worker), Function(function) {}
  // Dialog procedure
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
};

template <class W>
MRESULT DialogSubWindow<W>::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ return (Worker.*Function)(*this, msg, mp1, mp2);
}*/

#endif
