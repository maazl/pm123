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
#include <stdio.h>


LONG ResponseGraph::ToYCore(double relative)
{ LONG result = (LONG)(relative * (XY2i.y - XY1i.y) + .5) + XY1i.y;
  if (abs(result) <= 32767)
    return result;
  // PM dislikes large numbers
  return result > 0 ? 32767 : -32767;
}

LONG ResponseGraph::ToYdB(double, double mag)
{ return ToY1(log(mag) * (20./M_LN10));
}
LONG ResponseGraph::ToYT(double f, double delay)
{ return ToY2(f * delay);
}

static const char SIprefix[] =
{ 'y', 'z', 'a', 'f', 'p', 'n', 'u', 'm' // 10^-24..10^-3
, '.'                                    // [8]
, 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' }; // 10^3..10^24

void ResponseGraph::AxisText(char* target, double value)
{ if (value == 0.)
  { target[0] = '0';
    target[1] = 0;
    return;
  }
  // get digits 0123456789
  //            +1.2E+345
  char buf[10];
  sprintf(buf, "%+.1e", value);
  if (buf[0] == '-')
    *target++ = '-';
  int exponent = atoi(buf+5) + 25;
  ASSERT(exponent >= 0 && exponent <= 50);
  char separator = SIprefix[exponent/3];
  target[3] = 0;
  switch (exponent % 3)
  {default: // x11
    target[0] = separator;
    target[1] = buf[1];
    if (buf[3] != '0')
      target[2] = buf[3];
    else
      target[2] = 0;
    return;
   case 1:  // 1x1
    target[0] = buf[1];
    target[1] = separator;
    if (buf[3] != '0')
      target[2] = buf[3];
    else if (separator != '.')
      target[2] = 0;
    else
      target[1] = 0;
    return;
   case 2:  // 11x
    target[0] = buf[1];
    target[1] = buf[3];
    if (separator != '.')
      target[2] = separator;
    else
      target[2] = 0;
    return;
  }
}

void ResponseGraph::DrawLabel(HPS ps, POINTL at, int quadrant, double value)
{ char label[5];
  AxisText(label, value);
  size_t len = strlen(label);
  if (quadrant >= 3)
  { // Draw below at
    at.y -= FontMetrics.lEmHeight - 2;
  } else
    at.y += 2;
  if (quadrant & 2)
  { // draw left of at, right aligned
    GpiSetCharDirection(ps, CHDIRN_RIGHTLEFT);
    size_t l = 0;
    size_t r = len - 1;
    while (l < r)
      swap(label[l++], label[r--]);
  } else
  { at.x += 1;
    GpiSetCharDirection(ps, CHDIRN_LEFTRIGHT);
  }
  GpiCharStringAt(ps, &at, strlen(label), label);
}

void ResponseGraph::DrawText(HPS ps, POINTL at, const char* text)
{
  size_t len = strlen(text);
  at.x -= len * FontMetrics.lAveCharWidth / 2;
  GpiCharStringAt(ps, &at, len, (char*)text);
}

void ResponseGraph::DrawGraph(HPS ps, size_t column, LONG (ResponseGraph::*yscale)(double, double))
{
  column += StartCol;

  POINTL points[1000];
  POINTL* const dpe = points + sizeof points / sizeof *points;

  SyncAccess<DataFile> data(Data);
  const DataRowType*const* rp = data.Obj.begin();
  const DataRowType*const* rpe = data.Obj.end();

  bool start = true;
  while (rp != rpe)
  { POINTL* dp;
    for (dp = points; dp < dpe && rp != rpe; ++rp)
    { const DataRowType& row = **rp;
      double value = row[column];
      if (isnan(value))
        continue;
      dp->x = ToX(row[0]);
      dp->y = (this->*yscale)(row[0], value);
      ++dp;
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
  FATTRS fattrs =
  { sizeof(FATTRS)
  , 0, 0
  , "Helv", 0, 0, 8,8
  , 0, 0
  };
  GpiCreateLogFont(ps, NULL, 0, &fattrs);
  GpiQueryFontMetrics(ps, sizeof FontMetrics, &FontMetrics);
  GpiIntersectClipRectangle(ps, (RECTL*)&XY1);

  GpiMove(ps, &XY1);
  GpiBox(ps, DRO_FILL, &XY2, 0,0);

  GpiSetColor(ps, CLR_DARKGRAY);
  GpiSetBackMix(ps, BM_LEAVEALONE);
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
      xy.y = XY2.y;
      GpiMove(ps, &xy);
      xy.y = XY1.y;
      GpiLine(ps, &xy);
      DrawLabel(ps, xy, 1, lx);
    }
  }

  { // Y grid
    double ly = log10(Y1c);
    int yp = (int)floor(ly);
    ly = floor(exp(yp * M_LN10) + .5);

    double y = ceil(Y1min / ly) * ly;
    while (y <= Y1max)
    {
      POINTL xy;
      xy.x = XY1.x;
      xy.y = ToY1(y);
      DrawLabel(ps, xy, 4, y);
      GpiMove(ps, &xy);
      xy.x = XY2.x;
      GpiLine(ps, &xy);
      DrawLabel(ps, xy, 3, (y - Y1min) / Y1c * Y2c + Y2min);
      y += ly;
    }
  }

  /*GpiSetColor(ps, CLR_BLACK);
  GpiMove(ps, &XY1);
  GpiBox(ps, DRO_OUTLINE, &XY2, 0,0);*/

  GpiIntersectClipRectangle(ps, (RECTL*)&XY1);
  // Left delay
  GpiSetColor(ps, CLR_PINK);
  DrawGraph(ps, 1, &ResponseGraph::ToYT);
  // right delay
  GpiSetColor(ps, CLR_CYAN);
  DrawGraph(ps, 3, &ResponseGraph::ToYT);
  // Left amplitude
  GpiSetColor(ps, CLR_RED);
  DrawGraph(ps, 0, &ResponseGraph::ToYdB);
  // Right amplitude
  GpiSetColor(ps, CLR_BLUE);
  DrawGraph(ps, 2, &ResponseGraph::ToYdB);

  { // Legend
    GpiSetCharDirection(ps, CHDIRN_LEFTRIGHT);
    POINTL xy;
    xy.y = XY2.y - FontMetrics.lEmHeight + 2;
    xy.x = (5 * XY1.x + 8 * XY2.x) / 13;
    GpiSetColor(ps, CLR_PINK);
    DrawText(ps, xy, "L delay t >");
    xy.x = (2 * XY1.x + 11 * XY2.x) / 13;
    GpiSetColor(ps, CLR_CYAN);
    DrawText(ps, xy, "R delay t >");
    xy.x = (11 * XY1.x + 2 * XY2.x) / 13;
    GpiSetColor(ps, CLR_RED);
    DrawText(ps, xy, "< L gain dB");
    xy.x = (8 * XY1.x + 5 * XY2.x) / 13;
    GpiSetColor(ps, CLR_BLUE);
    DrawText(ps, xy, "< R gain dB");
  }
}

void ResponseGraph::Invalidate()
{ DEBUGLOG(("ResponseGraph(%p)::Invalidate()\n", this));
  if (!GetHwnd())
    return;
  RECTL rcl;
  PMRASSERT(WinQueryWindowRect(GetHwnd(), &rcl));
  PMRASSERT(WinInvalidateRect(GetHwnd(), &rcl, FALSE));
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

ResponseGraph::ResponseGraph(const SyncRef<DataFile>& data, unsigned startcol)
: Data(data)
, StartCol(startcol)
{ DEBUGLOG(("ResponseGraph(%p)::ResponseGraph(&%p, %u)\n", this, &data, startcol));
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

void ResponseGraph::SetAxis(double xmin, double xmax, double y1min, double y1max, double y2min, double y2max)
{ DEBUGLOG(("ResponseGraph(%p)::SetAxis(%f,%f, %f,%f, %f,%f)\n", this, xmin, xmax, y1min, y1max, y2min, y2max));
  Xmin  = xmin;
  Xmax  = xmax;
  LX    = log(xmin);
  LXc   = log(xmax) - LX;
  Y1min = y1min;
  Y1max = y1max;
  Y1c   = y1max - y1min;
  Y2min = y2min;
  Y2max = y2max;
  Y2c   = y2max - y2min;
  Invalidate();
}

