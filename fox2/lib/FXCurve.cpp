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
#include "fx.h"
#include "FXChartDefs.h"
#include "FXChartUtils.h"
#include "FXChart.h"
#include "FXData.h"
#include "FXCurve.h"
#include "FX2DChart.h"
#include "FX2DPlot.h"
#include <utility>

/*
  Notes:
*/


using namespace FXCHART;

/*******************************************************************************/

namespace FXCHART {

// Object implementation
FXIMPLEMENT(FXCurve,FXObject,nullptr,0)


// Deserialization
FXCurve::FXCurve():plot(nullptr),xaxis(FX2DChart::XLO),yaxis(FX2DChart::YLO),scale(1.0),offset(0.0),visible(true){
  linestyle.color=FXRGB(0,0,0);
  linestyle.weight=1;
  linestyle.style=LINESTYLE_SOLID;
  marker.color=FXRGB(0,0,0);
  marker.size=0;
  marker.style=MARKER_NONE;
  fillstyle.image=nullptr;
  fillstyle.forecolor=FXRGBA(0,0,0,0);
  fillstyle.backcolor=FXRGBA(0,0,0,0);
  fillstyle.lower=FXRGBA(0,0,0,0);
  fillstyle.upper=FXRGBA(0,0,0,0);
  fillstyle.hatch=STIPPLE_NONE;
  fillstyle.style=FILLSTYLE_OFF;
  }


// Init
FXCurve::FXCurve(FX2DPlot* plt,const FXString& nm):plot(plt),label(nm),xaxis(FX2DChart::XLO),yaxis(FX2DChart::YLO),scale(1.0),offset(0.0),visible(true){
  linestyle.color=FXRGB(0,0,0);
  linestyle.weight=1;
  linestyle.style=LINESTYLE_SOLID;
  marker.color=FXRGB(0,0,0);
  marker.size=0;
  marker.style=MARKER_NONE;
  fillstyle.image=nullptr;
  fillstyle.forecolor=FXRGBA(0,0,0,0);
  fillstyle.backcolor=FXRGBA(0,0,0,0);
  fillstyle.lower=FXRGBA(0,0,0,0);
  fillstyle.upper=FXRGBA(0,0,0,0);
  fillstyle.hatch=STIPPLE_NONE;
  fillstyle.style=FILLSTYLE_OFF;
  }


// Change Y-value scale and offset
void FXCurve::setScale(FXdouble factor,FXdouble shift){
  if(scale!=factor || offset!=shift){
    scale=factor;
    offset=shift;
    if(plot) plot->update();
    }
  }


// Return the scaled finite Y data range
FXbool FXCurve::getYRange(FXdouble& lo,FXdouble& hi) const {
  FXdouble minimum,maximum;
  if(!ydata || !ydata->getRange(minimum,maximum)) return false;
  lo=minimum*scale+offset;
  hi=maximum*scale+offset;
  if(lo>hi) std::swap(lo,hi);
  return true;
  }


// Sets the curve name
void FXCurve::setLabel(const FXString& nm){
  if(label!=nm){
    label=nm;
    plot->update();
    }
  }


// Change x axis description
void FXCurve::setXAxis(FXuchar ax){
  if(xaxis!=ax){
    xaxis=ax;
    plot->update();
    }
  }


// Change y axis description
void FXCurve::setYAxis(FXuchar ax){
  if(yaxis!=ax){
    yaxis=ax;
    plot->update();
    }
  }


// Change line style
void FXCurve::setLineStyle(const LineStyle& style){
  linestyle=style;
  plot->update();
  }


// Change marker style
void FXCurve::setMarkerStyle(const Marker& style){
  marker=style;
  plot->update();
  }


// Change fill style
void FXCurve::setFillStyle(const FillStyle& style){
  fillstyle=style;
  plot->update();
  }


// Show or hide curve
void FXCurve::setVisible(FXbool vis){
  if(visible!=vis){
    visible=vis;
    plot->update();
    }
  }


// Change x data samples
void FXCurve::setXData(std::shared_ptr<FXData> dd){
  if(xdata!=dd){
    xdata=std::move(dd);
    plot->update();
    }
  }

// Change y data samples
void FXCurve::setYData(std::shared_ptr<FXData> dd){
  if(ydata!=dd){
    ydata=std::move(dd);
    if(plot) plot->update();
    }
  }


// Save data
void FXCurve::save(FXStream& store) const {
  FXObject::save(store);
  store << label;
  store << xaxis;
  store << yaxis;
  store << linestyle.color;
  store << linestyle.weight;
  store << linestyle.style;
  store << marker.color;
  store << marker.size;
  store << marker.style;
  store << visible;
  }


// Load data
void FXCurve::load(FXStream& store){
  FXObject::load(store);
  store >> label;
  store >> xaxis;
  store >> yaxis;
  store >> linestyle.color;
  store >> linestyle.weight;
  store >> linestyle.style;
  store >> marker.color;
  store >> marker.size;
  store >> marker.style;
  store >> visible;
  }


// Destroy
FXCurve::~FXCurve(){
  xdata.reset();
  ydata.reset();
  plot=(FX2DPlot*)-1L;
  }

}
