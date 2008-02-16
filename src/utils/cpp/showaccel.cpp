/*
 * Copyright 2008-2008 M.Mueller
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

// Enrich menus with accelerator keys

#include "showaccel.h"

#include <snprintf.h>

#include <string.h>
#include <ctype.h>


int MenuShowAccel::AccEntry::compareTo(const ACCEL& r) const
{ int ret = (int)cmd - r.cmd; // sign function not required becaues USHORT may not overflow in int.
  return ret != 0 ? ret : (fs & (AF_HELP|AF_SYSCOMMAND)) - (r.fs & (AF_HELP|AF_SYSCOMMAND));
}

MenuShowAccel::MenuShowAccel(HACCEL accel)
{ DEBUGLOG(("MenuShowAccel::MenuShowAccel(%p)\n", accel));
  // Get accelerator table
  size_t acclen = WinCopyAccelTable(accel, NULL, 0);
  if (acclen == 0)
    return;
  ACCELTABLE* acctab = (ACCELTABLE*)alloca(acclen);
  PMRASSERT(WinCopyAccelTable(accel, acctab, acclen));
  // convert to sorted list
  AccData.reserve(acctab->cAccel); 
  ACCEL* ap = acctab->aaccel + acctab->cAccel;
  while (ap-- != acctab->aaccel)
  { AccEntry*& aep = AccData.get(*ap);
    if (!aep)
      aep = new AccEntry;
    (ACCEL&)*aep = *ap; // slicing
  }
}

// Names of virtual keys
// This table makes assumptions about the numbers of the virtual keys in OS/2
static const char VK_Names[][8] =
{ "???",
  "Mouse-1", // 0x01
  "Mouse-2",
  "Mouse-3",
  "Break",
  "Backsp.",
  "Tab",
  "Tab",
  "Return",
  "Shift",
  "Ctrl",
  "Alt",
  "AltGr",
  "Pause",
  "CapsLck",
  "Esc",
  "Space", // 0x10
  "PgUp",
  "PgDown",
  "End",
  "Home",
  "Left",
  "Up",
  "Right",
  "Down",
  "Print",
  "Insert",
  "Del",
  "Scroll",
  "Num",
  "Enter",
  "SysRq",
  "F1", // 0x20
  "F2",
  "F3",
  "F4",
  "F5",
  "F6",
  "F7",
  "F8",
  "F9",
  "F10",
  "F11",
  "F12",
  "F13",
  "F14",
  "F15",
  "F16",
  "F17", // 0x30
  "F18",
  "F19",
  "F20",
  "F21",
  "F22",
  "F23",
  "F24",
  "EndDrag",
  "Clear",
  "ErEOF",
  "PA1",
  "Attn",
  "CrSel",
  "ExSel",
  "Copy",
  "Blk1", // 0x40
  "Blk2",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "PA2", // 0x50
  "PA3",
  "Group",
  "GrpLck",
  "Appl",
  "LWin",
  "RWin"
};

void MenuShowAccel::Accel2Text(char* text, ACCEL as)
{ DEBUGLOG(("MenuShowAccel::Accel2Text(%p, {%x, %x, %u})\n", text, as.fs, as.key, as.cmd));
  *text = 0;
  // meta-keys
  if (as.fs & AF_SHIFT)
  { strcpy(text, "Sh+");
    text += 3;
  }
  if (as.fs & AF_ALT)
  { strcpy(text, "Alt+");
    text += 4;
  }
  if (as.fs & AF_CONTROL)
  { *text++ = '^';
    *text = 0;
  }
  // key text
  switch (as.fs & (AF_CHAR|AF_VIRTUALKEY|AF_SCANCODE))
  {case AF_CHAR:
    if (as.key != ' ')
    { if (isalpha(as.key))
        as.key = toupper(as.key);
      *text++ = (char)as.key;
      *text = 0;
      break;
    } else // Show space as text
      as.key = VK_SPACE;
   case AF_VIRTUALKEY:
    if (as.key >= sizeof VK_Names / sizeof *VK_Names)
      as.key = 0;
    strcpy(text, VK_Names[as.key]);
  }
}

void MenuShowAccel::UpdateAccel(HWND menu, USHORT id, const ACCEL* accel)
{ DEBUGLOG(("MenuShowAccel::UpdateAccel(%p, %i, %p)\n", menu, id, accel));
  // Fetch text
  char text[128]; // Here a static buffer is a choice.
  if (!WinSendMsg(menu, MM_QUERYITEMTEXT, MPFROM2SHORT(id, sizeof text), MPFROMP(text)))
    return;
  bool modified = false;
  // Strip old accelerator key text if any.
  char* cp = strchr(text, '\t');
  if (cp)
  { DEBUGLOG(("MenuShowAccel::UpdateAccel: removing %s from ID %i\n", cp, id));
    *cp = 0;
    modified = true;
  }
  // Check if there is a accelerator for this entry.
  if (accel)
  { char key[20];
    Accel2Text(key, *accel);
    // append keyname
    DEBUGLOG(("MenuShowAccel::UpdateAccel: add string %s, %s\n", text, key));
    size_t tlen = strlen(text);
    snprintf(text + tlen, sizeof text - tlen, "\t%s", key);
    modified = true;
  }
  // set new item text
  if (modified)
    PMRASSERT(WinSendMsg(menu, MM_SETITEMTEXT, MPFROMSHORT(id), MPFROMP(text)));
}

void MenuShowAccel::ApplyTo(HWND menu, bool incl_sub)
{ DEBUGLOG(("MenuShowAccel::ApplyTo(%p, %u)\n", menu, incl_sub));
  // Enumerate menu items
  SHORT count = SHORT1FROMMR(WinSendMsg(menu, MM_QUERYITEMCOUNT, 0, 0));
  while (count--)
  { // Fetch item data
    SHORT id = SHORT1FROMMR(WinSendMsg(menu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(count), 0));
    DEBUGLOG(("MenuShowAccel::ApplyTo: checking id %i at position %i\n", id, count));
    if (id == MIT_ERROR)
      continue;
    MENUITEM item;
    PMRASSERT(WinSendMsg(menu, MM_QUERYITEM, MPFROM2SHORT(id, FALSE), MPFROMP(&item)));

    if (item.afStyle & MIS_TEXT)
    { // Check if there is a accelerator for this entry.
      ACCEL as = {!!(item.afStyle & MIS_SYSCOMMAND) * AF_SYSCOMMAND | !!(item.afStyle & MIS_HELP) * AF_HELP, 0, id};
      UpdateAccel(menu, id, AccData.find(as));
    }
    // handle subitems
    if (incl_sub && item.afStyle & MIS_SUBMENU)
      ApplyTo(item.hwndSubMenu);
  }
}


