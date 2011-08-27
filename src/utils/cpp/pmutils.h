/*
 * Copyright 2010-2010 M.Mueller
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

#ifndef  PMUTILS_H
#define  PMUTILS_H

#define INCL_WIN
#include <debuglog.h>
#include <os2.h>

#ifdef PM123_CORE
#include <cpp/xstring.h>
#else
#include <plugin.h>
#endif

/// Get window text as xstring without length limitation.
inline xstring WinQueryWindowXText(HWND hwnd)
{ xstring ret;
  char* dst = ret.allocate(WinQueryWindowTextLength(hwnd));
  PMXASSERT(WinQueryWindowText(hwnd, ret.length()+1, dst), == ret.length());
  return ret;
}
inline xstring WinQueryDlgItemXText(HWND hwnd, USHORT id)
{ return WinQueryWindowXText(WinWindowFromID(hwnd, id));
}


/** Smart accessor for presentation space. */
class PresentationSpace
{private:
  const HPS     Hps;

 public:
                PresentationSpace(HWND hwnd)  : Hps(WinGetPS(hwnd)) { PMASSERT(Hps != NULLHANDLE); }
                ~PresentationSpace()          { PMRASSERT(WinReleasePS(Hps)); }
                operator HPS() const          { return Hps; }
};

#endif

