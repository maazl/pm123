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


LONG ResponseGraph::ToX(double x)
{ if (Axes.Flags & AF_LogX)
    x = log(x);
  LONG result = (LONG)floor((x - X0) / XS * (XY2.x - XY1.x) + .5) + XY1.x;
  // PM dislikes large numbers
  // furthermore it is useful to notify where your values got out of bounds.
  if (result < XY1.x)
    return XY1.x;
  if (result > XY2.x)
    return XY2.x;
  return result;
}

LONG ResponseGraph::ToYCore(double relative)
{ LONG result = (LONG)(relative * (XY2.y - XY1.y) + .5) + XY1.y;
  // PM dislikes large numbers
  // furthermore it is useful to notify where your values got out of bounds.
  if (result < XY1.y)
    return XY1.y;
  if (result > XY2.y)
    return XY2.y;
  return result;
}

static const char SIprefix[] =
{ 'y', 'z', 'a', 'f', 'p', 'n', 'u', 'm' // 10^-24..10^-3
, '.'                                    // [8]
, 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' }; // 10^3..10^24

void ResponseGraph::AxesText(char* target, double value, int exponent)
{ /*if (value == 0.)
  { target[0] = '0';
    target[1] = 0;
    return;
  }*/
  value *= exp(-exponent * M_LN10);
  // get digits 0123456789
  //            +1.2E+345
  char buf[10];
  sprintf(buf, "%+.1e", value);
  if (buf[0] == '-')
    *target++ = '-';
  exponent += atoi(buf+5) + 25;
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

void ResponseGraph::DrawLabel(POINTL at, int quadrant, double value, int exponent)
{ char label[5];
  AxesText(label, value);
  size_t len = strlen(label);
  if (quadrant >= 3)
  { // Draw below at
    at.y -= FontMetrics.lEmHeight - 2;
  } else
    at.y += 2;
  if (quadrant & 2)
  { // draw left of at, right aligned
    GpiSetCharDirection(PS, CHDIRN_RIGHTLEFT);
    size_t l = 0;
    size_t r = len - 1;
    while (l < r)
      swap(label[l++], label[r--]);
  } else
  { at.x += 1;
    GpiSetCharDirection(PS, CHDIRN_LEFTRIGHT);
  }
  GpiCharStringAt(PS, &at, strlen(label), label);
}

void ResponseGraph::DrawXLabel(double x)
{
  POINTL xy;
  xy.x = ToX(x);
  xy.y = XY2.y;
  GpiMove(PS, &xy);
  xy.y = XY1.y;
  GpiLine(PS, &xy);
  DrawLabel(xy, 1, x);
}

void ResponseGraph::DrawYLabel(double y)
{
  double ry = Axes.Flags & AF_LogY1 ? log(y) : y;
  ry = (ry - Y10) / Y1S;
  POINTL xy;
  xy.x = XY1.x;
  xy.y = ToYCore(ry);
  DrawLabel(xy, 4, y);
  GpiMove(PS, &xy);
  xy.x = XY2.x;
  GpiLine(PS, &xy);
}

void ResponseGraph::DrawY12Label(double y)
{
  double ry = Axes.Flags & AF_LogY1 ? log(y) : y;
  ry = (ry - Y10) / Y1S;
  POINTL xy;
  xy.x = XY1.x;
  xy.y = ToYCore(ry);
  DrawLabel(xy, 4, y);
  GpiMove(PS, &xy);
  xy.x = XY2.x;
  GpiLine(PS, &xy);
  DrawLabel(xy, 3, ry * Y2S + Y20);
}

static const double logscale[] =
{ 1, 2, 5, 10 };

void ResponseGraph::LinAxes(double min, double max, void (ResponseGraph::*drawlabel)(double value))
{
  double l = log10((max-min)/6.);
  int p = (int)floor(l);
  l -= p;
  // round to next 1/2/5
  const double* dp = logscale;
  while (l > log10(*dp))
    ++dp;

  l = *dp * exp(p * M_LN10);

  double v = ceil(min / l -.001) * l;
  double ve = max + l/1000.;
  while (v <= ve)
  { (this->*drawlabel)(v);
    v += l;
  }
}

void ResponseGraph::LogAxes(double min, double max, void (ResponseGraph::*drawlabel)(double value))
{
  double l = log10(min) - .001;
  int p = (int)floor(l);
  l -= p;
  // round to next 1/2/5
  const double* dp = logscale;
  while (l > log10(*dp))
    ++dp;

  const double* dpe = logscale + sizeof logscale / sizeof *logscale;
  for (;;)
  { if (dp == dpe)
    { dp = logscale;
      ++p;
    }
    l = *dp++ * exp(p * M_LN10);
    if (l > max * 1.001)
      break;

    (this->*drawlabel)(l);
  }
}

static inline double to_y_rel(double value, bool logscale, double y0, double ys)
{ if (logscale)
    value = log(value);
  return (value - y0) / ys;
}

void ResponseGraph::DrawGraph(const GraphInfo& graph)
{
  GpiSetColor(PS, graph.Color);

  POINTL points[1000];
  POINTL* const dpe = points + sizeof points / sizeof *points;

  SyncAccess<DataFile> data(graph.Data);
  const DataRowType*const* rp = data->begin();
  const DataRowType*const* rpe = data->end();

  bool start = true;
  while (rp != rpe)
  { POINTL* dp;
    for (dp = points; dp < dpe && rp != rpe; ++rp)
    { const DataRowType& row = **rp;
      double value = (*graph.ExtractY)(row, graph.User);
      if (isnan(value))
        continue;
      value = graph.Flags & GF_Y2
        ? to_y_rel(value, Axes.Flags & AF_LogY1, Y20, Y2S)
        : to_y_rel(value, Axes.Flags & AF_LogY2, Y10, Y1S);
      dp->y = ToYCore(value);
      value = (*graph.ExtractX)(row, graph.User);
      if (isnan(value))
        continue;
      dp->x = ToX(value);
      ++dp;
    }
    if (start)
      GpiMove(PS, points);
    GpiPolyLine(PS, dp - points, points);
    start = false;
  }
}

void ResponseGraph::DrawLegend(const char* text, unsigned index)
{
  size_t len = strlen(text);
  POINTL xy;
  xy.y = XY2.y - (FontMetrics.lMaxAscender - 2) - (FontMetrics.lMaxBaselineExt - 2) * (index / 4);
  index %= 4;
  index *= 3;
  // Now index is 0, 3, 6 or 9.
  xy.x = ((11 - index) * XY1.x + (2 + index) * XY2.x) / 13;
  xy.x -= len * FontMetrics.lAveCharWidth / 2;
  GpiCharStringAt(PS, &xy, len, (char*)text);
}

void ResponseGraph::Draw()
{
  GpiSetBackColor(PS, CLR_WHITE);
  GpiSetPattern(PS, PATSYM_BLANK);
  GpiSetBackMix(PS, BM_OVERPAINT);
  FATTRS fattrs =
  { sizeof(FATTRS)
  , 0, 0
  , "Helv", 0, 0, 8,8
  , 0, 0
  };
  GpiCreateLogFont(PS, NULL, 0, &fattrs);
  GpiQueryFontMetrics(PS, sizeof FontMetrics, &FontMetrics);
  GpiIntersectClipRectangle(PS, (RECTL*)&XY1);

  GpiMove(PS, &XY1);
  GpiBox(PS, DRO_FILL, &XY2, 0,0);

  GpiSetColor(PS, CLR_DARKGRAY);
  GpiSetBackMix(PS, BM_LEAVEALONE);
  // X grid
  { void (ResponseGraph::*scalefn)(double min, double max, void (ResponseGraph::*drawlabel)(double value))
      = Axes.Flags & AF_LogX ? &ResponseGraph::LogAxes : &ResponseGraph::LinAxes;
    (this->*scalefn)(Axes.XMin, Axes.XMax, &ResponseGraph::DrawXLabel);
  }
  // Y grid
  { void (ResponseGraph::*scalefn)(double min, double max, void (ResponseGraph::*drawlabel)(double value))
      = Axes.Flags & AF_LogY1 ? &ResponseGraph::LogAxes : &ResponseGraph::LinAxes;
    void (ResponseGraph::*labelfn)(double value)
      = isnan(Axes.Y2Min) ? &ResponseGraph::DrawYLabel : &ResponseGraph::DrawY12Label;
    (this->*scalefn)(Axes.Y1Min, Axes.Y1Max, labelfn);
  }

  /*GpiSetColor(ps, CLR_BLACK);
  GpiMove(ps, &XY1);
  GpiBox(ps, DRO_OUTLINE, &XY2, 0,0);*/

  // draw graphs
  // Draw in reverse order to keep the important ones at the top.
  unsigned i;
  for (i = Graphs.size(); i--; )
    DrawGraph(*Graphs[i]);

  // Draw legend
  GpiSetCharDirection(PS, CHDIRN_LEFTRIGHT);
  for (unsigned i = Graphs.size(); i--; )
  { GpiSetColor(PS, Graphs[i]->Color);
    DrawLegend(Graphs[i]->Legend, i);
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
      PS = ps;
      PMRASSERT(WinQueryWindowRect(GetHwnd(), (RECTL*)&XY1));
      Draw();
      return 0;
    }
  }
  return SubclassWindow::WinProc(msg, mp1, mp2);
}

ResponseGraph::ResponseGraph()
{ DEBUGLOG(("ResponseGraph(%p)::ResponseGraph()\n", this));
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

inline static void preparescale(bool logscale, double& s0, double& ss, double min, double max)
{
  if (logscale)
  { s0 = log(min);
    ss = log(max / min);
  } else
  { s0 = min;
    ss = max - min;
  }
}

void ResponseGraph::PrepareAxes()
{ DEBUGLOG(("ResponseGraph(%p)::PrepareAxes() - {%x, %f,%f, %f,%f, %f,%f}\n", this,
    Axes.Flags, Axes.XMin, Axes.XMax, Axes.Y1Min, Axes.Y1Max, Axes.Y2Min, Axes.Y2Max));
  // precalculate some values
  preparescale(Axes.Flags & AF_LogX, X0, XS, Axes.XMin, Axes.XMax);
  preparescale(Axes.Flags & AF_LogY1, Y10, Y1S, Axes.Y1Min, Axes.Y1Max);
  preparescale(Axes.Flags & AF_LogY2, Y20, Y2S, Axes.Y2Min, Axes.Y2Max);
}
