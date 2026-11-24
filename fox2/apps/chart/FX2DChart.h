/********************************************************************************
*                                                                               *
*             T w o - D i m e n s i o n a l   C h a r t   W i d g e t           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FX2DCHART_H
#define FX2DCHART_H

#ifndef FXCHART_H
#include "FXChart.h"
#endif

namespace FXCHART {


/// Base class for two-dimensional chart widgets
class FXCHARTAPI FX2DChart : public FXChart {
  FXDECLARE(FX2DChart)
protected:
  FXAxis   *axes[4];            // Axis definitions
  FillStyle chartstyle;         // Chart fill style
  mutable FXRectangle plotarea;   // Plot area in widget coordinates
  FXRectangle bandrect;         // Rubber band rectangle
  FXint     bandanchorx;        // Rubber band anchor x
  FXint     bandanchory;        // Rubber band anchor y
  FXbool    banding;            // Rubber band being dragged
  FXbool    zoomed;             // Axes zoomed away from the data range
  FXdouble  savedrange[4][2];   // Ranges in effect before the first zoom
protected:
  FX2DChart();
  virtual void drawSelf(FXDC& dc) const override;
  virtual void drawOverlay(FXDC& dc) const override;
  virtual void drawPlot(FXDC& dc,FXint plotleft,FXint plottop,FXint plotright,FXint plotbottom) const;
  void saveZoomRanges();
  void setZoomRanges(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi);
private:
  FX2DChart(const FX2DChart&);
  FX2DChart &operator=(const FX2DChart&);
public:
  long onCmdXXX(FXObject*,FXSelector,void*);
  long onMouseWheel(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
public:
  enum{
    ID_XXX=FXChart::ID_LAST,
    ID_LAST
    };
public:
  enum {
    XLO,        /// Lower X axis
    YLO,        /// Lower Y axis
    XHI,        /// Upper X axis
    YHI         /// Upper Y axis
    };
public:


  /// Construct two-dimensional chart widget
  FX2DChart(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=CAPTION_ABOVE|CAPTION_SHOWN,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=4,FXint pr=4,FXint pt=4,FXint pb=4);

  /// Create server-side resources
  virtual void create() override;

  /// Detach server-side resources
  virtual void detach() override;

  /// Return axis
  FXAxis* getAxis(FXint ax) const { return axes[ax]; }

  /// Return the plot area in widget coordinates
  const FXRectangle& getPlotArea() const { return plotarea; }

  /// Draw the axis numbers without fading on the next draw
  virtual void skipFades() override;

  /// Zoom the axes by factor around a point in the plot
  void zoomAt(FXint x,FXint y,FXdouble factor);

  /// Zoom the axes to a rectangle in the plot
  void zoomRectangle(FXint x,FXint y,FXint w,FXint h);

  /// Return to the ranges in effect before the first zoom
  void resetZoom();

  /// Set view ranges while retaining the current ranges for reset
  void setViewRange(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi);

  /// Return true if the axes are zoomed away from the data range
  FXbool isZoomed() const { return zoomed; }

  /// Forget the zoom state, keeping the current axis ranges
  void clearZoom(){ zoomed=false; }

  /// Set chart fill style
  void setChartStyle(const FillStyle& fs);

  /// Get chart fill style
  const FillStyle& getChartStyle() const { return chartstyle; }

  /// Save chart to a stream
  virtual void save(FXStream& store) const override;

  /// Load chart from a stream
  virtual void load(FXStream& store) override;

  /// Destructor
  virtual ~FX2DChart();
  };

}

#endif
