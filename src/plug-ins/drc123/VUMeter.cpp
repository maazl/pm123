/*
 * Copyright 2013 Marcel Mueller
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

#define INCL_PM
#include "VUMeter.h"
#include <debuglog.h>
#include <os2.h>


void VUMeter::Draw(HPS ps)
{ DEBUGLOG(("VUMeter(%p)::Draw(%p)\n", this, ps));

  LONG cur = ToX(CurValue);
  LONG peak = ToX(PeakValue);
  LONG warn = ToX(WarnValue);
  LONG red = ToX(RedValue);

  POINTL xy1 = XY1;
  POINTL xy2 = XY2;
  // draw colored bar from XY1.x to cur.
  if (cur >= XY1.x)
  { // draw red part
    if (cur < XY2.x)
      xy2.x = cur;
    if (cur > red)
    { GpiSetColor(ps, CLR_RED);
      xy1.x = red + 1;
      GpiMove(ps, &xy1);
      GpiBox(ps, DRO_FILL, &xy2, 0,0);
      xy2.x = red;
    }
    // draw yellow part
    if (cur > warn)
    { GpiSetColor(ps, CLR_YELLOW);
      xy1.x = warn + 1;
      GpiMove(ps, &xy1);
      GpiBox(ps, DRO_FILL, &xy2, 0,0);
      xy2.x = warn;
    }
    // draw green part
    GpiSetColor(ps, CLR_GREEN);
    xy1.x = XY1.x;
    GpiMove(ps, &xy1);
    GpiBox(ps, DRO_FILL, &xy2, 0,0);
  }
  // draw background from cur to XY2.x.
  if (cur < XY2.x)
  { GpiSetColor(ps, CLR_DARKGRAY);
    xy1.x = cur >= XY1.x ? cur + 1 : XY1.x;
    xy2.x = XY2.x;
    GpiMove(ps, &xy1);
    GpiBox(ps, DRO_FILL, &xy2, 0,0);
    // draw peak line at peak.
    if (peak > cur && peak <= XY2.x && peak >= XY1.x)
    { // which color?
      GpiSetColor(ps, peak > red ? CLR_RED : peak > warn ? CLR_YELLOW : CLR_GREEN);
      xy1.x = xy2.x = peak;
      GpiMove(ps, &xy1);
      GpiLine(ps, &xy2);
    }
  }
}

void VUMeter::Invalidate()
{ DEBUGLOG(("VUMeter(%p)::Invalidate()\n", this));
  if (!GetHwnd())
    return;
  RECTL rcl;
  PMRASSERT(WinQueryWindowRect(GetHwnd(), &rcl));
  PMRASSERT(WinInvalidateRect(GetHwnd(), &rcl, FALSE));
}

MRESULT VUMeter::WinProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_PAINT:
    { PaintPresentationSpace ps(GetHwnd());
      PMRASSERT(WinQueryWindowRect(GetHwnd(), (RECTL*)&XY1));
      Draw(ps);
      return 0;
    }
  }
  return SubclassWindow::WinProc(msg, mp1, mp2);
}

VUMeter::VUMeter()
: MinValue(0)
, DeltaValue(1)
, WarnValue(.7)
, RedValue(.9)
, CurValue(0)
, PeakValue(0)
{ DEBUGLOG(("VUMeter(%p)::VUMeter()\n", this));
}

VUMeter::~VUMeter()
{ DEBUGLOG(("VUMeter(%p)::~VUMeter()\n", this));
}

void VUMeter::Attach(HWND hwnd)
{ DEBUGLOG(("VUMeter(%p)::Attach(%x)\n", this, hwnd));
  if (GetHwnd() == NULLHANDLE)
    SubclassWindow::Attach(hwnd);
  Invalidate();
}

void VUMeter::Detach()
{ DEBUGLOG(("VUMeter(%p)::Detach()\n", this));
  SubclassWindow::Detach();
  Invalidate();
}

