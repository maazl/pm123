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

#define INCL_GPI
#include "ResponseGraph.h"

#include <cpp/pmutils.h>
#include <os2.h>


ResponseGraph::ResponseGraph(const DataFile& data)
: Data(data)
{
}

ResponseGraph::~ResponseGraph()
{
  // TODO Auto-generated destructor stub
}

MRESULT ResponseGraph::WinProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_PAINT:
    { PaintPresentationSpace ps(GetHwnd());
      RECTL xywh;
      PMRASSERT(WinQueryWindowRect(GetHwnd(), &xywh));
      POINTL xy12[2] = {{-1000,-1000}, {10000,10000}};
      GpiSetColor(ps, 3);
      GpiSetBackColor(ps, 2);
      GpiBox(ps, DRO_OUTLINEFILL, xy12, 0,0);
      return 0;
    }
  }
  return SubclassWindow::WinProc(msg, mp1, mp2);
}
