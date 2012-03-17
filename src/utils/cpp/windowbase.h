/*
 * Copyright 2007-2010 Marcel Mueller
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
#include <cpp/container/vector.h>

#include <os2.h>

#include <debuglog.h>


/****************************************************************************
*
*  This abstract class represents an instance of a OS/2 PM dialog window.
*
****************************************************************************/
class DialogBase
{public:
  enum DlgFlags
  { DF_None           = 0x00,
    DF_AutoResize     = 0x01
  };
 private:
  // wrap pointer to keep PM happy
  struct init_dlg_struct
  { USHORT      size;
    DialogBase* pm;
  };

 protected: // content
  const ULONG       DlgRID;        // Resource ID of the dialog template
  const HMODULE     ResModule;     // Module handle for the above resource
  const DlgFlags    Flags;
 private:
  HWND              HwndFrame;     // Frame window handle
  bool              InitialVisible;
  int               Initialized;
  //xstring           Title;         // Keep the window title storage

 private:
  // Static members must not use EXPENTRY linkage with IBM VACPP.
  friend MRESULT EXPENTRY wb_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

 private: // non-copyable
  DialogBase(const DialogBase&);
  void operator=(const DialogBase&);

 protected:
  // Create a playlist manager window for an object, but don't open it.
  DialogBase(ULONG rid, HMODULE module, DlgFlags flags = DF_None);

  // load dialog resources and create window
  void              StartDialog(HWND owner, HWND parent = HWND_DESKTOP);
  // Dialog procedure, called by DlgProcStub
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  // Dialog initialization
  virtual void      OnInit();
  // Destruction callback
  virtual void      OnDestroy();

  // Get the window title
  xstring           GetTitle() const;
  // Set the window title
  void              SetTitle(const char* title);

  void              PostMsg(ULONG msg, MPARAM mp1, MPARAM mp2);
  // Dialog item functions
  HWND              GetDlgItem(ULONG id) { return WinWindowFromID(HwndFrame, id); }
  MRESULT           SendItemMsg(ULONG id, ULONG msg, MPARAM mp1, MPARAM mp2)
                    { return WinSendDlgItemMsg(HwndFrame, id, msg, mp1, mp2); }
  void              EnableControl(ULONG id, bool check);
  void              SetItemText(ULONG id, const char* text);
  bool              QueryButtonCheckstate(ULONG id);
  void              CheckButton(ULONG id, bool check);
  ULONG             QuerySelectedRadiobutton(ULONG id);
  void              SetSpinbuttonLimits(ULONG id, LONG low, LONG high, USHORT len);
  LONG              QuerySpinbuttonValue(ULONG id);
  void              SetSpinbuttomValue(ULONG id, LONG value);

  // Set Help Manager
  void              SetHelpMgr(HWND hhelp);

 public: // public interface
  virtual           ~DialogBase();
  // Get the window handle
  HWND              GetHwnd() const { ASSERT(HwndFrame != NULLHANDLE); return HwndFrame; }
  // Make the window visible (or not)
  virtual void      SetVisible(bool show);
  bool              GetVisible() const;
  void              Process()       { WinProcessDlg(HwndFrame); }
  // Force the window to close
  void              Destroy()       { WinDestroyWindow(HwndFrame); }
};
FLAGSATTRIBUTE(DialogBase::DlgFlags);


class NotebookDialogBase : public DialogBase
{protected:
  // Base class for notebook pages
  class PageBase;
  friend class PageBase;
  class PageBase : public DialogBase
  { friend class NotebookDialogBase;
   protected:
    NotebookDialogBase& Parent;
    xstring         MajorTitle;
    xstring         MinorTitle;
   private:
    ULONG           PageID;  // PM Notebook page ID from.
   protected:
                    PageBase(NotebookDialogBase& parent, ULONG rid, HMODULE module, DlgFlags flags = DF_None);
    virtual void    StartDialog()   { DialogBase::StartDialog(Parent.GetHwnd(), Parent.GetHwnd()); }
   public:
    /// Return the InfoFlags to be requested
    const xstring&  GetMajorTitle() { return MajorTitle; }
    const xstring&  GetMinorTitle() { return MinorTitle; }
    ULONG           GetPageID()     { return PageID; }
  };

 protected:
  vector_own<PageBase> Pages;

 protected:
  NotebookDialogBase(ULONG rid, HMODULE module, DlgFlags flags = DF_None) : DialogBase(rid, module, flags) {}
  void              StartDialog(HWND owner, USHORT nbid, HWND parent = HWND_DESKTOP);
  PageBase*         PageFromID(ULONG pageid);
};

/** Wrapper class for dialog base classes to tie the object lifetime
 * to the lifetime of the window.
 */
template <class BASE>
class ManagedDialog : public BASE, public IVref_count
{private:
  int_ptr<IVref_count> Self;
 protected:
  virtual void      OnInit();
  // If you override OnDestroy you must not access *this after ManagedDialogBase::OnDestroy returned.
  virtual void      OnDestroy();
 public:
  ManagedDialog(ULONG rid, HMODULE module, DialogBase::DlgFlags flags = DialogBase::DF_None) : BASE(rid, module, flags) {}
};

template <class BASE>
void ManagedDialog<BASE>::OnInit()
{ DEBUGLOG(("ManagedDialog(%p)::OnInit()\n", this));
  DialogBase::OnInit();
  Self = this;
}

template <class BASE>
void ManagedDialog<BASE>::OnDestroy()
{ DEBUGLOG(("ManagedDialog(%p)::OnDestroy()\n", this));
  int_ptr<IVref_count> keepalive;
  Self.swap(keepalive);
  DialogBase::OnDestroy();
  // this may get invalid here
}


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