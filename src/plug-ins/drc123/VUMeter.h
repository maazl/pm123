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

#ifndef VUMETER_H
#define VUMETER_H

#define INCL_GPI
#include <cpp/windowbase.h>
#include <os2.h>


class VUMeter : public SubclassWindow
{private:
  double            MinValue;
  double            DeltaValue;
  double            WarnValue;
  double            RedValue;
  double            CurValue;
  double            PeakValue;
  POINTL            XY1, XY2;
 private:
  LONG              ToX(double value) { return (LONG)((value - MinValue) / DeltaValue * (XY2.x - XY1.x)) + XY1.x; }
  void              Draw(HPS ps);
 protected:
  virtual MRESULT   WinProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  /// Force redraw.
          void      Invalidate();
 public:
                    VUMeter();
  virtual           ~VUMeter();
  /// Activate this instance.
  virtual void      Attach(HWND hwnd);
  /// Disable this instance.
  virtual void      Detach();
  /// Set Display range.
          void      SetRange(double min, double max, double warn, double red) { MinValue = min, DeltaValue = max - min; WarnValue = warn; RedValue = red; Invalidate(); }
  /// Set new value.
          void      SetValue(double cur, double peak) { CurValue = cur, PeakValue = peak; Invalidate(); }
};

#endif // VUMETER_H
