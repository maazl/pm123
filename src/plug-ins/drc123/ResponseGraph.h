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

#ifndef RESPONSEGRAPH_H
#define RESPONSEGRAPH_H

#define INCL_GPI
#include <cpp/windowbase.h>
#include <cpp/mutex.h>
#include <os2.h>
#include <math.h>


class DataFile;

class ResponseGraph : public SubclassWindow
{ const SyncRef<DataFile> Data;
  const unsigned    StartCol;
  double            Xmin, Xmax, LX, LXc;
  double            Y1min, Y1max, Y1c, Y2min, Y2max, Y2c;
  POINTL            XY1, XY2, XY1i, XY2i;
  FONTMETRICS       FontMetrics;
 private:
  LONG              ToX(double x)   { return (LONG)((log(x) - LX) / LXc * (XY2i.x - XY1i.x) + .5) + XY1i.x; }
  LONG              ToYCore(double relative);
  LONG              ToY1(double y1) { return ToYCore((y1 - Y1min) / Y1c); }
  LONG              ToY2(double y2) { return ToYCore((y2 - Y2min) / Y2c); }
  LONG              ToYdB(double f, double mag);
  LONG              ToYT(double f, double delay);
  static void       AxisText(char* target, double value);
  /// Draw axis label
  /// @param ps Presentation space.
  /// @param at Point where the label belongs to.
  /// @param quadrant Where is the label to be drawn.
  /// 1 = upper right, 2 = upper left, 3 = lower left, 4 = lower right.
  /// @param Value of the axis label.
  void              DrawLabel(HPS ps, POINTL at, int quadrant, double value);
  void              DrawText(HPS ps, POINTL at, const char* text);
  void              DrawGraph(HPS ps, size_t column, LONG (ResponseGraph::*yscale)(double, double));
  void              Draw(HPS ps);
 protected:
  virtual MRESULT   WinProc(ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  ResponseGraph(const SyncRef<DataFile>& data, unsigned startcol);
  virtual ~ResponseGraph();
  /// Activate this instance.
  virtual void      Attach(HWND hwnd);
  /// Disable this instance.
  virtual void      Detach();
  /// Set axis for frequency, dB and group delay.
  void              SetAxis(double xmin, double xmax, double y1min, double y1max, double y2min, double y2max);
  void              Invalidate();
};

#endif // RESPONSEGRAPH_H
