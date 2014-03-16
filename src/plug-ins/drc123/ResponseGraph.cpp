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
#include "Iterators.h"

#include <cpp/pmutils.h>
#include <os2.h>
#include <math.h>
#include <stdio.h>
#include <limits.h>


POINTL ResponseGraph::Graph[MAX_X_WIDTH];
//POINTL ResponseGraph::GraphLow[MAX_X_WIDTH];
//POINTL ResponseGraph::GraphHigh[MAX_X_WIDTH];


/*double ResponseGraph::FromX(double x)
{ double val = X0 + XS * (x - XY1.x) / (XY2.x - XY1.x);
  return Axes.Flags & AF_LogX ? exp(val) : val;
}*/

LONG ResponseGraph::ToX(double x)
{ if (Axes.Flags & AF_LogX)
    x = log(x);
  // => [0,1]
  x = (x - X0) / XS;
  // PM dislikes large numbers
  // furthermore it is useful to notify where your values got out of bounds.
  if (x <= 0)
    return XY1.x;
  if (x >= 1)
    return XY2.x;
  return (LONG)floor(x * (XY2.x - XY1.x) + .5) + XY1.x;
}

LONG ResponseGraph::ToYCore(double relative)
{ // PM dislikes large numbers
  // furthermore it is useful to notify where your values got out of bounds.
  if (relative <= 0)
    return XY1.y;
  if (relative >= 1)
    return XY2.y;
  LONG result = (LONG)(relative * (XY2.y - XY1.y) + .5) + XY1.y;
    return result;
}

/*LONG ResponseGraph::ToY(double y, bool y2)
{ if (isnan(y))
    return LONG_MIN;
  if (y2)
  { if (Axes.Flags & AF_LogY2)
      y = log(y);
    y = (y - Y20) / Y2S;
  } else
  { if (Axes.Flags & AF_LogY1)
      y = log(y);
    y = (y - Y10) / Y1S;
  }
  return ToYCore(y);
}*/

static const char SIprefix[] =
{ 'y', 'z', 'a', 'f', 'p', 'n', 'u', 'm' // 10^-24..10^-3
, '.'                                    // [8]
, 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' }; // 10^3..10^24

void ResponseGraph::AxesText(char* target, double value)
{ /*if (value == 0.)
  { target[0] = '0';
    target[1] = 0;
    return;
  }*/
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

void ResponseGraph::DrawLabel(POINTL at, int quadrant, double value)
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
  y = ry * Y2S + Y20;
  if (Axes.Flags & AF_LogY2)
    y = exp(y);
  else if (fabs(y) < fabs(Y2S) / 1E6)
    y = 0;
  DrawLabel(xy, 3, y);
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
    if (fabs(v) < fabs(max - min) / 1E6)
      v = 0;
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

bool ResponseGraph::PrepareGraph(const GraphInfo& graph, double (AggregateIterator::*getter)() const)
{ SyncAccess<DataFile> data(graph.Data);
  DEBUGLOG(("ResponseGraph(%p)::PrepareGraph(&%p{%p, },)\n", this, &graph, &*data));
  if (!graph.Reader->Reset(*data))
    return false;
  const LONG maxcount = XY2.x - XY1.x;
  const double offset = graph.Flags & GF_Average ? .5 : 0.;
  double x = X0 + XS * (-1 + offset) / maxcount;
  graph.Reader->ReadNext(Axes.Flags & AF_LogX ? exp(x) : x);
  double y;
  for (LONG i = 0; i <= maxcount; ++i)
  { x = X0 + XS * (i + offset) / maxcount;
    graph.Reader->ReadNext(Axes.Flags & AF_LogX ? exp(x) : x);
    y = (graph.Reader->*getter)();
    if (isnan(y))
      Graph[i].y = LONG_MIN;
    else if (graph.Flags & GF_Y2)
    { if (Axes.Flags & AF_LogY2)
        y = log(y);
      Graph[i].y = ToYCore((y - Y20) / Y2S);
    } else
    { if (Axes.Flags & AF_LogY1)
        y = log(y);
      Graph[i].y = ToYCore((y - Y10) / Y1S);
    }
    //GraphLow[i].y = ToY(graph.Reader->LowValue, graph.Flags & GF_Y2);
    //GraphHigh[i].y = ToY(graph.Reader->HighValue, graph.Flags & GF_Y2);
  }
  return true;
}

void ResponseGraph::DrawGraph(POINTL* data)
{
  const POINTL* const end = data + (XY2.x - XY1.x);
  for(;;)
  { // get runs of non NAN values
    for(;;)
    { if (data == end)
        return;
      if (data->y != LONG_MIN)
        break;
      ++data;
    }
    POINTL* start = data; // first non NAN point
    while (++data != end && data->y != LONG_MIN);
    // now data points one after the last non NAN point
    GpiMove(PS, start);
    GpiPolyLine(PS, data - start, start);
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
  GpiMove(PS, &XY1);
  GpiBox(PS, DRO_FILL, &XY2, 0,0);
  if (isnan(Axes.XMin) || isnan(Axes.XMax))
    return;
  FATTRS fattrs =
  { sizeof(FATTRS)
  , 0, 0
  , "Helv", 0, 0, 8,8
  , 0, 0
  };
  GpiCreateLogFont(PS, NULL, 0, &fattrs);
  GpiQueryFontMetrics(PS, sizeof FontMetrics, &FontMetrics);
  GpiIntersectClipRectangle(PS, (RECTL*)&XY1);

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

  // Setup X axis
  for (LONG i = 0; i <= XY2.x - XY1.x; ++i)
    Graph[i].x = i + XY1.x;

  // Draw Bound graphs first
  for (unsigned i = Graphs.size(); i--; )
  { const GraphInfo& graph = *Graphs[i];
    if ( !(graph.Flags & GF_Bounds)
      || !PrepareGraph(graph, &AggregateIterator::GetMinValue) )
      continue;
    LONG color = Graphs[i]->Color;
    if (!(graph.Flags & GF_RGB))
      GpiQueryLogColorTable(PS, 0, color, 1, &color); // get RGB
    // raise brightness
    color = (color | 0x03030300) >> 2;
    // Set color
    GpiCreateLogColorTable(PS, 0, LCOLF_RGB, 0, 0, NULL);
    GpiSetColor(PS, color);
    DrawGraph(Graph);
    if (!PrepareGraph(graph, &AggregateIterator::GetMaxValue))
      continue;
    DrawGraph(Graph);
    GpiCreateLogColorTable(PS, LCOL_RESET, LCOLF_INDRGB, 0, 0, NULL);
  }

  // Draw in reverse order to keep the important ones at the top.
  for (unsigned i = Graphs.size(); i--; )
  { const GraphInfo& graph = *Graphs[i];
    if (!PrepareGraph(graph, &AggregateIterator::GetValue))
      continue;
    if (graph.Flags & GF_RGB)
      GpiCreateLogColorTable(PS, 0, LCOLF_RGB, 0, 0, NULL);
    else
      GpiCreateLogColorTable(PS, LCOL_RESET, LCOLF_INDRGB, 0, 0, NULL);
    GpiSetColor(PS, graph.Color);
    DrawGraph(Graph);
  }

  // Draw legend
  GpiSetCharDirection(PS, CHDIRN_LEFTRIGHT);
  for (unsigned i = Graphs.size(); i--; )
  { const GraphInfo& graph = *Graphs[i];
    if (graph.Legend)
    { if (graph.Flags & GF_RGB)
        GpiCreateLogColorTable(PS, 0, LCOLF_RGB, 0, 0, NULL);
      else
        GpiCreateLogColorTable(PS, LCOL_RESET, LCOLF_INDRGB, 0, 0, NULL);
      GpiSetColor(PS, graph.Color);
      DrawLegend(graph.Legend, i);
    }
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
