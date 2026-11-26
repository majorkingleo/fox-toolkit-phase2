/********************************************************************************
*                                                                               *
*                      P l o t - C u r v e   D e s c r i p t i o n              *
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
#ifndef FXCURVE_H
#define FXCURVE_H

#ifndef FXCHARTDEFS_H
#include "FXChartDefs.h"
#endif

#include <memory>

namespace FXCHART {


class FXData;
class FX2DPlot;


/// Describe how curve is done
class FXCHARTAPI FXCurve : public FXObject {
  FXDECLARE(FXCurve)
protected:
  FX2DPlot *plot;       // Plot control
  std::shared_ptr<FXData> xdata; // X data samples
  std::shared_ptr<FXData> ydata; // Y data samples
  FXString  label;      // Name of plot
  LineStyle linestyle;  // Line style
  Marker    marker;     // Marker style
  FillStyle fillstyle;  // Fill style
  FXuchar   xaxis;      // X-Axis
  FXuchar   yaxis;      // Y-Axis
  FXbool    visible;    // Curve is visible
protected:
  FXCurve();
private:
  FXCurve(const FXCurve&);
  FXCurve &operator=(const FXCurve&);
public:

  /// Construct curve with given name
  FXCurve(FX2DPlot* plt,const FXString& nm=FXString::null);

  /// Sets the curve name
  void setLabel(const FXString& nm);

  /// Return curve name
  const FXString& getLabel() const { return label; }

  /// Change x axis description
  void setXAxis(FXuchar ax);

  /// Return x axis description
  FXuchar getXAxis() const { return xaxis; }

  /// Change y axis description
  void setYAxis(FXuchar ax);

  /// Return y axis description
  FXuchar getYAxis() const { return yaxis; }

  /// Change line style
  void setLineStyle(const LineStyle& style);

  /// Return line style
  const LineStyle& getLineStyle() const { return linestyle; }

  /// Change marker style
  void setMarkerStyle(const Marker& style);

  /// Return marker style
  const Marker& getMarkerStyle() const { return marker; }

  /// Change fill style
  void setFillStyle(const FillStyle& style);

  /// Return fill style
  const FillStyle& getFillStyle() const { return fillstyle; }

  /// Show or hide curve
  void setVisible(FXbool vis);

  /// Return true if curve is visible
  FXbool isVisible() const { return visible; }

  /// Change x data samples
  void setXData(std::shared_ptr<FXData> dd);

  /// Return x data samples
  FXData* getXData() const { return xdata.get(); }

  /// Change y data samples
  void setYData(std::shared_ptr<FXData> dd);

  /// Return y data samples
  FXData* getYData() const { return ydata.get(); }

  /// Save curve to a stream
  virtual void save(FXStream& store) const override;

  /// Load curve from a stream
  virtual void load(FXStream& store) override;

  /// Destructor
  virtual ~FXCurve();
  };


}

#endif
