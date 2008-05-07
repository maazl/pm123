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

#ifndef PM123_INFODIALOG_H
#define PM123_INFODIALOG_H

#define INCL_WIN
#include "windowbase.h"
#include "playable.h"
#include <cpp/container.h>
#include <cpp/smartptr.h>
#include <cpp/mutex.h>
#include <cpp/event.h>
#include <os2.h>


/****************************************************************************
*
*  Class to handle info dialogs
*  All functions of this class are not thread-safe.
*
****************************************************************************/
class InfoDialog 
: public ManagedDialogBase,
  public IComparableTo<const Playable*>,
  public inst_index<InfoDialog, Playable>
{private:
  class Factory;
  friend class Factory;
  class Factory : public inst_index<InfoDialog, Playable>::IFactory
  { // singleton
    Factory() {}
   public:
    static Factory  Instance;
    virtual InfoDialog* operator()(Playable* key);
  };
  class PageBase;
  friend class PageBase;
  class PageBase : public DialogBase
  {protected:
    InfoDialog&     Parent;
    PageBase(InfoDialog& parent, ULONG rid);
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    void            StartDialog() { DialogBase::StartDialog(Parent.GetHwnd(), Parent.GetHwnd()); }
  };
  class Page1Window;
  friend class Page1Window;
  class Page1Window : public PageBase
  { // Dialog procedure
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    Page1Window(InfoDialog& parent, ULONG rid) : PageBase(parent, rid) {}
  };
  class Page2Window;
  friend class Page2Window;
  class Page2Window : public PageBase
  { // Dialog procedure
    virtual MRESULT DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
   public:
    Page2Window(InfoDialog& parent, ULONG rid) : PageBase(parent, rid) {}
  };
  
 private:
  const int_ptr<Playable> Content;
  Page1Window       Page1;
  Page2Window       Page2;
  class_delegate<InfoDialog, const Playable::change_args> ContentChangeDeleg;

 private:
  InfoDialog(Playable* p);
  void              StartDialog();
  virtual MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  void              ContentChangeEvent(const Playable::change_args& args);

 public:
  //virtual ~InfoDialog();
  //virtual void      SetVisible(bool show);
  
 public:
  // Factory method. Returns always the same instance for the same Playable.
  static int_ptr<InfoDialog> GetByKey(Playable* obj);
  // IComparableTo<Playable>
  virtual int       compareTo(const Playable*const& r) const;
 private: // getter for inst_index<InfoDialog, Playable>
  //virtual void      GetKey(const Playable*& key) const; // getter for the key
};


#endif

