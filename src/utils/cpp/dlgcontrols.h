/*
 * Copyright 2012-2012 Marcel Mueller
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

#ifndef DLGCONTROLS_H
#define DLGCONTROLS_H

#define INCL_WIN

#include <cpp/xstring.h>
#include <cpp/pmutils.h>

#include <os2.h>

#include <debuglog.h>


struct ControlBase
{ HWND        Hwnd;
  ControlBase(HWND hwnd)                            : Hwnd(hwnd) {}
  bool        operator!() const                     { return Hwnd == NULLHANDLE; }
  void        Enabled(bool enable)                  { PMRASSERT(WinEnableWindow(Hwnd, enable)); }
  ULONG       Style() const                         { return WinQueryWindowULong(Hwnd, QWL_STYLE); }
  void        Text(const char* text)                { PMRASSERT(WinSetWindowText(Hwnd, text)); }
  xstring     Text() const                          { return WinQueryWindowXText(Hwnd); }
};

struct CheckBox : ControlBase
{ CheckBox(HWND hwnd)                               : ControlBase(hwnd) {}
  USHORT      CheckState(USHORT checked)            { return SHORT1FROMMR(WinSendMsg(Hwnd, BM_SETCHECK, MPFROMSHORT(checked), 0)); }
  USHORT      CheckState() const                    { return SHORT1FROMMR(WinSendMsg(Hwnd, BM_QUERYCHECK, 0, 0)); }
};

struct RadioButton : ControlBase
{ RadioButton(HWND hwnd)                            : ControlBase(hwnd) {}
  bool        CheckState(bool checked = true)       { return (bool)SHORT1FROMMR(WinSendMsg(Hwnd, BM_SETCHECK, MPFROMSHORT(checked), 0)); }
  bool        CheckState() const                    { return (bool)SHORT1FROMMR(WinSendMsg(Hwnd, BM_QUERYCHECK, 0, 0)); }
  SHORT       CheckIndex() const                    { return SHORT1FROMMR(WinSendMsg(Hwnd, BM_QUERYCHECKINDEX, 0, 0)); }
  USHORT      CheckID() const;
};

struct ListBox : ControlBase
{ ListBox(HWND hwnd)                                : ControlBase(hwnd) {}
  void        DeleteAll()                           { PMRASSERT(WinSendMsg(Hwnd, LM_DELETEALL, 0, 0)); }
  void        InsertItem(const char* item, SHORT where = LIT_END) { PMXASSERT((SHORT)SHORT1FROMMR(WinSendMsg(Hwnd, LM_INSERTITEM, MPFROMSHORT(where), MPFROMP(item))), >= 0); }
  void        InsertItems(const char*const* items, unsigned count, int where = LIT_END);
  USHORT      Count() const                         { return SHORT1FROMMR(WinSendMsg(Hwnd, LM_QUERYITEMCOUNT, 0, 0)); }
  void        Handle(unsigned i, ULONG value)       { PMRASSERT(WinSendMsg(Hwnd, LM_SETITEMHANDLE, MPFROMSHORT(i), MPFROMLONG(value))); }
  ULONG       Handle(unsigned i) const              { return LONGFROMMR(WinSendMsg(Hwnd, LM_QUERYITEMHANDLE, MPFROMSHORT(i), 0)); }
  SHORT       NextSelection(SHORT after = LIT_FIRST) const { return SHORT1FROMMR(WinSendMsg(Hwnd, LM_QUERYSELECTION, MPFROMSHORT(after), 0)); }
  bool        Select(int i)                         { return (bool)WinSendMsg(Hwnd, LM_SELECTITEM, MPFROMSHORT(i), MPFROMSHORT(TRUE)); }
};

struct ComboBox : ListBox
{ ComboBox(HWND hwnd)                               : ListBox(hwnd) {}
  // Emulate unsupported message
  void        InsertItems(const char*const* items, unsigned count, int where = LIT_END);
  void        TextLimit(USHORT len)                 { PMRASSERT(WinSendMsg(Hwnd, EM_SETTEXTLIMIT, MPFROMSHORT(len), 0)); }
};

struct EntryField : ControlBase
{ EntryField(HWND hwnd)                             : ControlBase(hwnd) {}
  void        TextLimit(USHORT len)                 { PMRASSERT(WinSendMsg(Hwnd, EM_SETTEXTLIMIT, MPFROMSHORT(len), 0)); }
};

struct MLE : ControlBase
{ MLE(HWND hwnd)                                    : ControlBase(hwnd) {}
};

struct SpinButton : ControlBase
{ SpinButton(HWND hwnd) : ControlBase(hwnd) {}
  void        SetLimits(LONG low, LONG high, USHORT len) const;
  void        SetArray(const char*const* items, unsigned count) { PMRASSERT(WinSendMsg(Hwnd, SPBM_SETARRAY, MPFROMP(items), MPFROMSHORT(count))); }
  LONG        Value() const;
  void        Value(LONG value)                     { PMRASSERT(WinSendMsg(Hwnd, SPBM_SETCURRENTVALUE, MPFROMLONG(value), 0)); }
};

struct Notebook : ControlBase
{ Notebook(HWND hwnd) : ControlBase(hwnd) {}
  ULONG       GetTopPageID() const                  { return LONGFROMMR(WinSendMsg(Hwnd, BKM_QUERYPAGEID, 0, MPFROM2SHORT(BKA_TOP, 0))); }
  void        TurnToPage(ULONG id)                  { PMRASSERT(WinSendMsg(Hwnd, BKM_TURNTOPAGE, MPFROMLONG(id), 0)); }
};

#endif
