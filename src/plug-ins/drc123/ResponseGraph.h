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
#include <os2.h>
#include <math.h>


class DataFile;

class ResponseGraph : public SubclassWindow
{ const DataFile&   Data;
  double            Xmin, Xmax, LX, LXc;
  double            Y1min, Y1max, Y1c, Y2min, Y2max, Y2c;
  POINTL            XY1, XY2, XY1i, XY2i;
 private:
  LONG              ToX(double x)   { return (LONG)((log(x) - LX) / LXc * (XY2i.x - XY1i.x) + .5) + XY1i.x; }
  LONG              ToY1(double y1) { return (LONG)((y1 - Y1min) / Y1c * (XY2i.y - XY1i.y) + .5) + XY1i.y; }
  LONG              ToY2(double y2) { return (LONG)((y2 - Y2min) / Y2c * (XY2i.y - XY1i.y) + .5) + XY1i.y; }
  void              DrawGraph(HPS ps, size_t column, LONG (ResponseGraph::*yscale)(double));
  void              Draw(HPS ps);
 protected:
  void              Invalidate();
  virtual MRESULT   WinProc(ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  ResponseGraph(const DataFile& data);
  virtual ~ResponseGraph();
  /// Activate this instance.
  virtual void      Attach(HWND hwnd);
  /// Disable this instance.
  virtual void      Detach();
};

#endif // RESPONSEGRAPH_H
