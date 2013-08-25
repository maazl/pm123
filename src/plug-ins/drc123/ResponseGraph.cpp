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
#include "DataFile.h"

#include <cpp/pmutils.h>
#include <os2.h>
#include <math.h>


void ResponseGraph::DrawGraph(HPS ps, size_t column, LONG (ResponseGraph::*yscale)(double))
{
  POINTL points[1000];
  POINTL* const dpe = points + sizeof points / sizeof *points;

  const DataFile::RowType*const* rp = Data.Content().begin();
  const DataFile::RowType*const* rpe = Data.Content().end();

  bool start = true;
  while (rp != rpe)
  { POINTL* dp;
    for (dp = points; dp < dpe && rp != rpe; ++dp, ++rp)
    {
      const DataFile::RowType& row = **rp;
      dp->x = ToX(row[0]);
      dp->y = (this->*yscale)(row[column]);
    }
    if (start)
      GpiMove(ps, points);
    GpiPolyLine(ps, dp - points, points);
    start = false;
  }
}

static const double logscale[] =
{ 1, 2, 5, 10 };

void ResponseGraph::Draw(HPS ps)
{
  GpiSetBackColor(ps, CLR_WHITE);
  GpiSetPattern(ps, PATSYM_BLANK);
  GpiSetBackMix(ps, BM_OVERPAINT);
  GpiSetColor(ps, CLR_BLACK);

  GpiMove(ps, &XY1);
  GpiBox(ps, DRO_OUTLINEFILL, &XY2, 0,0);

  GpiSetColor(ps, CLR_DARKGRAY);
  { // X grid
    double lx = log10(Xmin) - .001;
    int xp = (int)floor(lx);
    lx -= xp;
    // round to next 1/2/5
    const double* dp = logscale;
    while (lx > log10(*dp))
      ++dp;

    const double* dpe = logscale + sizeof logscale / sizeof *logscale;
    for (;;)
    { if (dp == dpe)
      { dp = logscale;
        ++xp;
      }
      lx = *dp++ * exp(xp * M_LN10);

      if (lx > Xmax * 1.001)
        break;

      POINTL xy;
      xy.x = ToX(lx);
      xy.y = XY1.y;
      GpiMove(ps, &xy);
      xy.y = XY2.y;
      GpiLine(ps, &xy);
    }
  }

  { // Y grid
    double ly = log10(Y1c);
    int yp = (int)floor(ly);
    ly = exp(yp * M_LN10);

    double y = ceil(Y1min / ly) * ly;
    while (y <= Y1max)
    {
      POINTL xy;
      xy.x = XY1.x;
      xy.y = ToY1(y);
      GpiMove(ps, &xy);
      xy.x = XY2.x;
      GpiLine(ps, &xy);

      y += ly;
    }
  }

  GpiIntersectClipRectangle(ps, (RECTL*)&XY1);
  // Left delay
  GpiSetColor(ps, CLR_PINK);
  DrawGraph(ps, 2, &ResponseGraph::ToY2);
  // right delay
  GpiSetColor(ps, CLR_CYAN);
  DrawGraph(ps, 4, &ResponseGraph::ToY2);
  // Left amplitude
  GpiSetColor(ps, CLR_RED);
  DrawGraph(ps, 1, &ResponseGraph::ToY1);
  // Right amplitude
  GpiSetColor(ps, CLR_BLUE);
  DrawGraph(ps, 3, &ResponseGraph::ToY1);
}

void ResponseGraph::Invalidate()
{ DEBUGLOG(("ResponseGraph(%p)::Invalidate()\n", this));
  RECTL rcl;
  WinQueryWindowRect(GetHwnd(), &rcl);
  WinInvalidateRect(GetHwnd(), &rcl, FALSE);
}

MRESULT ResponseGraph::WinProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_PAINT:
    { PaintPresentationSpace ps(GetHwnd());
      PMRASSERT(WinQueryWindowRect(GetHwnd(), (RECTL*)&XY1));
      // TODO: reserve space for fonts
      XY1i = XY1;
      XY2i = XY2;
      Draw(ps);
      return 0;
    }
  }
  return SubclassWindow::WinProc(msg, mp1, mp2);
}

ResponseGraph::ResponseGraph(const DataFile& data)
: Data(data)
, Xmin(20)
, Xmax(20000)
, LX(log(Xmin))
, LXc(log(Xmax) - LX)
, Y1min(-40)
, Y1max(+10)
, Y1c(Y1max - Y1min)
, Y2min(-.01)
, Y2max(.04)
, Y2c(Y2max- Y2min)
{ DEBUGLOG(("ResponseGraph(%p)::ResponseGraph(&%p)\n", this, &data));
}

ResponseGraph::~ResponseGraph()
{ DEBUGLOG(("ResponseGraph(%p)::~ResponseGraph()\n", this));
}

void ResponseGraph::Attach(HWND hwnd)
{ DEBUGLOG(("ResponseGraph(%p)::Attach(%x)\n", this, hwnd));
  if (GetHwnd() == NULLHANDLE)
    SubclassWindow::Attach(hwnd);
  Invalidate();
}

void ResponseGraph::Detach()
{ DEBUGLOG(("ResponseGraph(%p)::Detach()\n", this));
  SubclassWindow::Detach();
  Invalidate();
}
