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
#include <cpp/xstring.h>
#include <cpp/container/vector.h>
#include <os2.h>
#include <math.h>


class DataFile;
struct DataRowType;

/** @brief Class to draw 2D graphs.
 * @details The class is intended to be used with a \c WC_STATIC control.
 * The control is sub classed as soon as you attach the window handle.
 * Once activated the original drawing of the \c WC_STATIC control is replaced.
 */
class ResponseGraph : public SubclassWindow
{public:
  enum AxesFlags
  { AF_None       = 0x00
  , AF_LogX       = 0x01    ///< X axes is logarithmic
  , AF_LogY1      = 0x02    ///< Y1 axes is logarithmic
  , AF_LogY2      = 0x04    ///< Y2 axes is logarithmic
  };
  struct AxesInfo
  { AxesFlags       Flags;
    double          XMin;
    double          XMax;
    double          Y1Min;
    double          Y1Max;
    double          Y2Min;
    double          Y2Max;
    AxesInfo() : Flags(AF_None), XMin(NAN), XMax(NAN), Y1Min(NAN), Y1Max(NAN), Y2Min(NAN), Y2Max(NAN) {}
  };
  typedef double (*Extractor)(const DataRowType& row, void* user);
  enum GraphFlags
  { GF_None       = 0x00
  , GF_Y2         = 0x01    ///< Use second y axes.
  };
  struct GraphInfo
  { /// Graph legend to be drawn at the top of the graph.
    /// Should be short.
    xstring         Legend;
    /// Link to an interface to provide a synchronized version of the data source.
    /// The mutex is acquired before drawing and released when finished.
    SyncRef<DataFile> Data;
    /// Extractor function used to extract the X value to be drawn.
    /// @details This function is applied to any row in \a Data.
    /// If the returned value is NaN then this row is ignored.
    Extractor       ExtractX;
    /// Extractor function used to extract the Y value to be drawn.
    /// @details This function is applied to any row in \a Data.
    /// If the returned value is NaN then this row is ignored.
    Extractor       ExtractY;
    /// User Parameter passed to the functions above as second argument.
    void*           User;
    /// Drawing flags. See @see GraphFlags.
    GraphFlags      Flags;
    /// Graph color in the system's standard color table.
    LONG            Color;
  };

 private:
  vector_own<GraphInfo> Graphs;
  AxesInfo          Axes;
  double            X0, XS, Y10, Y1S, Y20, Y2S;
  HPS               PS;
  POINTL            XY1, XY2;
  FONTMETRICS       FontMetrics;
 private:
  /// Convert X coordinate from graph to screen.
  LONG              ToX(double x);
  /// Convert relative Y position to Y coordinate.
  /// @param relative Coordinate, reasonable range: [0,1]
  /// @remarks The function also clips the return value to +-32767 to avoid PM crashes on noisy data.
  LONG              ToYCore(double relative);

  static void       AxesText(char* target, double value, int exponent = 0);
  /// Draw axes label
  /// @param ps Presentation space.
  /// @param at Point where the label belongs to.
  /// @param quadrant Where is the label to be drawn.
  /// 1 = upper right, 2 = upper left, 3 = lower left, 4 = lower right.
  /// @param Value of the axes label.
  void              DrawLabel(POINTL at, int quadrant, double value, int exponent = 0);
  void              DrawXLabel(double x);
  void              DrawYLabel(double y);
  void              DrawY12Label(double y);

  void              LinAxes(double min, double max, void (ResponseGraph::*drawlabel)(double value));
  void              LogAxes(double min, double max, void (ResponseGraph::*drawlabel)(double value));
  void              DrawGraph(const GraphInfo& graph);
  void              DrawLegend(const char* text, unsigned index);
  void              Draw();
  virtual MRESULT   WinProc(ULONG msg, MPARAM mp1, MPARAM mp2);

  void              PrepareAxes();
 public:
  ResponseGraph();
  virtual ~ResponseGraph();
  /// Activate this instance.
  /// @param hwnd Window handle of a \c WC_STATIC control.
  virtual void      Attach(HWND hwnd);
  /// Disable this instance.
  virtual void      Detach();
  /// @brief Invalidate the controls screen area.
  /// @details This invalidates the screen area of the control.
  /// If no window handle is attached this is a no-op.
  /// The function can be used to force an update of the data shown.
  void              Invalidate();
  /// Set axes of graph.
  /// @param axes The parameters to use, the content is copied by value.
  void              SetAxes(const AxesInfo& axes) { Axes = axes; PrepareAxes(); }
  void              SetAxes(AxesFlags flags, double xmin, double xmax, double y1min, double y1max, double y2min, double y2max)
  { Axes.Flags = flags; Axes.XMin = xmin; Axes.XMax = xmax; Axes.Y1Min = y1min; Axes.Y1Max = y1max; Axes.Y2Min = y2min; Axes.Y2Max = y2max; PrepareAxes(); }
  /// Add a new graph to draw.
  void              AddGraph(const GraphInfo& graph) { Graphs.append() = new GraphInfo(graph); }
  void              AddGraph(const xstring& legend, SyncRef<DataFile> data, Extractor xtractX, Extractor xtractY, void* user, GraphFlags flags, LONG color)
  { GraphInfo* gi = new GraphInfo(); Graphs.append() = gi;
    gi->Legend = legend; gi->Data = data; gi->ExtractX = xtractX; gi->ExtractY = xtractY; gi->User = user; gi->Flags = flags; gi->Color = color; }
  /// Clear all graphs.
  void              ClearGraphs() { Graphs.clear(); }
};

FLAGSATTRIBUTE(ResponseGraph::AxesFlags);
FLAGSATTRIBUTE(ResponseGraph::GraphFlags);

#endif // RESPONSEGRAPH_H
