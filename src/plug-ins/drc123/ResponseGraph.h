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
struct DataRow;
class AggregateIterator;


/** @brief Class to draw 2D graphs.
 * @details The class is intended to be used with a \c WC_STATIC control.
 * The control is sub classed as soon as you attach the window handle.
 * Once activated the original drawing of the \c WC_STATIC control is replaced.
 * To show a graph you need provide exactly one \c AxesInfo and at least one \c GraphInfo.
 */
class ResponseGraph : public SubclassWindow
{public:
  enum
  { MAX_X_WIDTH   = 2048    ///< Maximum graph width in device pixels (static buffer).
  };
  /// Flags for axes.
  enum AxesFlags
  { AF_None       = 0x00
  , AF_LogX       = 0x01    ///< X axes is logarithmic
  , AF_LogY1      = 0x02    ///< Y1 axes is logarithmic
  , AF_LogY2      = 0x04    ///< Y2 axes is logarithmic
  };
  /// Axes descriptor.
  struct AxesInfo
  { AxesFlags       Flags;  ///< Flags
    double          XMin;   ///< Lower bound of the X axis. Obligatory.
    double          XMax;   ///< Upper bound of the X axis. Obligatory. Might be less than \c XMin in linear mode.
    double          Y1Min;  ///< Lower bound of the left Y axis. Obligatory.
    double          Y1Max;  ///< Upper bound of the left Y axis. Obligatory. Might be less than \c Y1Min in linear mode.
    double          Y2Min;  ///< Lower bound of the right Y axis. NAN if no second axis.
    double          Y2Max;  ///< Upper bound of the right Y axis. NAN if no second axis. Might be less than \c Y2Min in linear mode.
    /// Create invalid \c AxesInfo.
    AxesInfo() : Flags(AF_None), XMin(NAN), XMax(NAN), Y1Min(NAN), Y1Max(NAN), Y2Min(NAN), Y2Max(NAN) {}
  };

  /// Flags for graphs.
  enum GraphFlags
  { GF_None       = 0x00
  , GF_Y2         = 0x01    ///< Use second y axes.
  , GF_Bounds     = 0x02    ///< Show upper and lower bounds after averaging.
  , GF_Average    = 0x04    ///< The ReadIterator returns average values starting from the last call to FetchNext.
  , GF_RGB        = 0x10    ///< The Color value is RGB rather than an Index.
  };
  /// Graph descriptor.
  struct GraphInfo
  { /// Graph legend to be drawn at the top of the graph.
    /// Should be quite short.
    xstring         Legend;
    /// Link to an interface to provide a synchronized version of the data source.
    /// The mutex is acquired before drawing and released when finished.
    SyncRef<DataFile> Data;
    /// @brief Iterator over the data above to extract values.
    /// @details GraphInfo does \e not take the ownership of this Iterator.
    AggregateIterator* Reader;
    /// Drawing flags. See @see GraphFlags.
    GraphFlags      Flags;
    /// Graph color in the system's standard color table.
    LONG            Color;
    GraphInfo(const xstring& legend, SyncRef<DataFile> data, AggregateIterator& reader, GraphFlags flags, LONG color)
    : Legend(legend), Data(data), Reader(&reader), Flags(flags), Color(color) {}
  };

 public:
  /// List of currently visible graphs.
  vector_own<GraphInfo> Graphs;
 private:
  AxesInfo          Axes;
  double            X0, XS, Y10, Y1S, Y20, Y2S;
  HPS               PS;
  POINTL            XY1, XY2;
  FONTMETRICS       FontMetrics;
  // Static buffer for drawing purposes to avoid frequent allocations.
  static POINTL     Graph[MAX_X_WIDTH];
  //static POINTL     GraphLow[MAX_X_WIDTH];
  //static POINTL     GraphHigh[MAX_X_WIDTH];
 private:
  /*/// Convert X coordinate from screen to graph.
  double            FromX(double x);*/
  /// Convert X coordinate from graph to screen.
  LONG              ToX(double x);
  /// Convert relative Y position to Y coordinate.
  /// @param relative Coordinate, reasonable range: [0,1]
  /// @remarks The function also clips the return value to +-32767 to avoid PM crashes on noisy data.
  LONG              ToYCore(double relative);
  //LONG              ToY(double y, bool y2);

  /// Format number for display at an axis. Use SI prefixes.
  /// @param target Target string. The function writes at most 5 characters: "-1k2\0".
  /// @param value Value to display.
  /// @pre value == 0 || 1E-25 <= fabs(value) < 1E-25
  static void       AxesText(char* target, double value);
  /// Draw axes label
  /// @param ps Presentation space.
  /// @param at Point where the label belongs to.
  /// @param quadrant Where is the label to be drawn.
  /// 1 = upper right, 2 = upper left, 3 = lower left, 4 = lower right.
  /// @param Value of the axes label.
  void              DrawLabel(POINTL at, int quadrant, double value);
  /// Draw grid line and axis axis label at the X axis.
  /// @param x Value to draw. The location is determined automatically.
  void              DrawXLabel(double x);
  /// Draw grid line and axis axis label at the Y1 axis.
  /// @param y Value to draw. The location is determined automatically.
  void              DrawYLabel(double y);
  /// @brief Draw grid line and axis axis label at the Y1 and Y2 axis.
  /// @param y Value to draw at Y1. The location is determined automatically,
  /// as well as the Y2 value.
  /// @details The function first draws the Y1 label as \c DrawYLabel does.
  /// Then it calculates the corresponding Y2 value and draws the label at the same height.
  void              DrawY12Label(double y);

  /// @brief Draw grid and axis labels for a linear axis.
  /// @param min Minimum value of the axis.
  /// @param max Maximum value of the axis.
  /// @param drawlabel Function to invoke for each axes label.
  /// This function decides whether to draw the X or the Y axis.
  /// @details The function splits the range [min,max] in approximately 6 equidistant steps.
  void              LinAxes(double min, double max, void (ResponseGraph::*drawlabel)(double value));
  /// @brief Draw grid and axis labels for a logarithmic axis.
  /// @param min Minimum value of the axis.
  /// @param max Maximum value of the axis.
  /// @param drawlabel Function to invoke for each axes label.
  /// This function decides whether to draw the X or the Y axis.
  /// @details The function draws only at 1/2/5 locations.
  void              LogAxes(double min, double max, void (ResponseGraph::*drawlabel)(double value));
  /// Prepare XY points for a graph.
  /// @param graph Info of the graph to draw.
  /// @param getter Method to extract Y value from the iterator.
  /// @return false: failed to prepare graph => don't draw.
  bool              PrepareGraph(const GraphInfo& graph, double (AggregateIterator::*getter)() const);
  /// Draw a graph
  /// @param data XY points to draw.
  void              DrawGraph(POINTL* data);
  /// @brief Draw legend of a graph.
  /// @param text Label to draw.
  /// @param index Ordinal number of the graph, starting from 0.
  /// @details The Function shows the labels at the top of the graph.
  /// Always 4 in each line. I.e. ordinal 0 to 3 are shown in the first line,
  /// ordinal 4 to 7 in the second and so on.
  void              DrawLegend(const char* text, unsigned index);
  /// Respond to WM_PAINT.
  /// @pre Before the function is invoked the member variables PS, XY1 and XY2 need to be initialized.
  void              Draw();
  /// Window procedure of the control.
  virtual MRESULT   WinProc(ULONG msg, MPARAM mp1, MPARAM mp2);
  /// Set some internal member variables after the axes information has changed.
  /// @pre The member variable Axes must be initialized before the call.
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
  /// Set axes of graph.
  void              SetAxes(AxesFlags flags, double xmin, double xmax, double y1min, double y1max, double y2min, double y2max)
  { Axes.Flags = flags; Axes.XMin = xmin; Axes.XMax = xmax; Axes.Y1Min = y1min; Axes.Y1Max = y1max; Axes.Y2Min = y2min; Axes.Y2Max = y2max; PrepareAxes(); }
};

FLAGSATTRIBUTE(ResponseGraph::AxesFlags);
FLAGSATTRIBUTE(ResponseGraph::GraphFlags);

#endif // RESPONSEGRAPH_H
