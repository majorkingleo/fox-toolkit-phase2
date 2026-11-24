/********************************************************************************
*                                                                               *
*              T w o - D i m e n s i o n a l   P l o t   W i d g e t            *
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
#include "chartdefs.h"
#include "chartutils.h"
#include "FXAxis.h"
#include "FXChart.h"
#include "FXData.h"
#include "FXCurve.h"
#include "FX2DChart.h"
#include "FX2DPlot.h"

/*
  Notes:
*/


using namespace FXCHART;

/*******************************************************************************/

namespace FXCHART {

// Object implementation
FXIMPLEMENT(FX2DPlot,FX2DChart,nullptr,0)


// Init
FX2DPlot::FX2DPlot(){
  lastdraw=0;
  }


// Construct two-dimensional plot widget
FX2DPlot::FX2DPlot(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FX2DChart(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  lastdraw=0;
  }


// Widen a range by five percent so curves do not touch the frame
static void widenRange(FXdouble& lo,FXdouble& hi){
  FXdouble margin;
  if(lo>hi) return;
  margin=(lo==hi) ? FXMAX(1.0,FXABS(lo)*0.05) : (hi-lo)*0.05;
  lo-=margin;
  hi+=margin;
  }


// Map chart line style to a DC line style
static FXLineStyle dcLineStyle(FXuchar style){
  if(style==LINESTYLE_DOTDASHED) return LINE_DOUBLE_DASH;
  if(style==LINESTYLE_NONE) return LINE_SOLID;
  if(style==LINESTYLE_SOLID) return LINE_SOLID;
  return LINE_ONOFF_DASH;
  }


// Find range of x samples on one axis; lo>hi when there is no data
void FX2DPlot::rangeX(FXint axis,FXdouble& lo,FXdouble& hi) const {
  FXint i,j;
  lo=1.0;
  hi=0.0;
  for(i=0; i<curves.no(); i++){
    FXCurve *curve=curves[i];
    FXData *data;
    if(!curve->isVisible() || curve->getXAxis()!=axis) continue;
    data=curve->getXData();
    if(!data) continue;
    for(j=0; j<data->no(); j++){
      FXdouble value=data->get(j);
      if(!Math::fpFinite(value)) continue;
      if(lo>hi){ lo=hi=value; }
      else{
        if(value<lo) lo=value;
        if(value>hi) hi=value;
        }
      }
    }
  }


// Find range of y samples on one axis; lo>hi when there is no data
void FX2DPlot::rangeY(FXint axis,FXdouble& lo,FXdouble& hi) const {
  FXint i,j;
  lo=1.0;
  hi=0.0;
  for(i=0; i<curves.no(); i++){
    FXCurve *curve=curves[i];
    FXData *data;
    if(!curve->isVisible() || curve->getYAxis()!=axis) continue;
    data=curve->getYData();
    if(!data) continue;
    for(j=0; j<data->no(); j++){
      FXdouble value=data->get(j);
      if(!Math::fpFinite(value)) continue;
      if(lo>hi){ lo=hi=value; }
      else{
        if(value<lo) lo=value;
        if(value>hi) hi=value;
        }
      }
    }
  }


// Fit all axis ranges to the curve data
void FX2DPlot::fitData(){
  FXdouble lo,hi;
  FXint axis;
  clearZoom();
  skipFades();
  for(axis=0; axis<4; axis++){
    rangeX(axis,lo,hi);
    if(lo<=hi){
      widenRange(lo,hi);
      getAxis(axis)->setDataRange(lo,hi);
      getAxis(axis)->setAxisRange(lo,hi);
      }
    else if(axis==FX2DChart::XHI){
      getAxis(axis)->setDataRange(getAxis(FX2DChart::XLO)->getDataRangeMin(),getAxis(FX2DChart::XLO)->getDataRangeMax());
      getAxis(axis)->setAxisRange(getAxis(FX2DChart::XLO)->getAxisRangeMin(),getAxis(FX2DChart::XLO)->getAxisRangeMax());
      }
    rangeY(axis,lo,hi);
    if(lo<=hi){
      widenRange(lo,hi);
      getAxis(axis)->setDataRange(lo,hi);
      getAxis(axis)->setAxisRange(lo,hi);
      }
    else if(axis==FX2DChart::YHI){
      getAxis(axis)->setDataRange(getAxis(FX2DChart::YLO)->getDataRangeMin(),getAxis(FX2DChart::YLO)->getDataRangeMax());
      getAxis(axis)->setAxisRange(getAxis(FX2DChart::YLO)->getAxisRangeMin(),getAxis(FX2DChart::YLO)->getAxisRangeMax());
      }
    }
  }


// Fit the y axis ranges to the curve data; a partial fit keeps the zoom record
void FX2DPlot::fitY(){
  FXdouble lo,hi;
  FXint axis;
  for(axis=0; axis<4; axis++){
    rangeY(axis,lo,hi);
    if(lo<=hi){
      widenRange(lo,hi);
      getAxis(axis)->setDataRange(lo,hi);
      getAxis(axis)->setAxisRange(lo,hi);
      }
    else if(axis==FX2DChart::YHI){
      getAxis(axis)->setDataRange(getAxis(FX2DChart::YLO)->getDataRangeMin(),getAxis(FX2DChart::YLO)->getDataRangeMax());
      getAxis(axis)->setAxisRange(getAxis(FX2DChart::YLO)->getAxisRangeMin(),getAxis(FX2DChart::YLO)->getAxisRangeMax());
      }
    }
  }


// Draw all visible curves
void FX2DPlot::drawPlot(FXDC& dc,FXint plotleft,FXint plottop,FXint plotright,FXint plotbottom) const {
  FXRectangle save=dc.getClipRectangle();
  FXint i,j,px,py;
  FXdouble xmin,xmax,ymin,ymax,xscale,yscale;
  FXdouble x,y;
  FXfloat fx,fy;
  FXTime start;
  if(plotleft>=plotright || plottop>=plotbottom) return;
  start=FXThread::steadytime();
  dc.setClipRectangle(plotleft,plottop,plotright-plotleft,plotbottom-plottop);
  dc.setAntialias(true);
  for(i=0; i<curves.no(); i++){
    FXCurve *curve=curves[i];
    FXAxis *xaxis;
    FXAxis *yaxis;
    FXData *xdata;
    FXData *ydata;
    FXArray<FXPointF> points;
    FXint count;
    if(!curve->isVisible()) continue;
    xaxis=getAxis(curve->getXAxis());
    yaxis=getAxis(curve->getYAxis());
    xdata=curve->getXData();
    ydata=curve->getYData();
    if(!xdata || !ydata) continue;
    xmin=xaxis->getAxisRangeMin();
    xmax=xaxis->getAxisRangeMax();
    ymin=yaxis->getAxisRangeMin();
    ymax=yaxis->getAxisRangeMax();
    if(!(xmax>xmin) || !(ymax>ymin)) continue;
    count=FXMIN(xdata->no(),ydata->no());
    if(count<=0) continue;
    xscale=((FXdouble)(plotright-plotleft))/(xmax-xmin);
    yscale=((FXdouble)(plotbottom-plottop))/(ymax-ymin);
    // Curves
    if(curve->getLineStyle().style!=LINESTYLE_NONE){
      dc.setForeground(curve->getLineStyle().color);
      dc.setLineWidth(curve->getLineStyle().weight);
      dc.setLineStyle(dcLineStyle(curve->getLineStyle().style));
      for(j=0; j<count; j++){
        x=xdata->get(j);
        y=ydata->get(j);
        if(!Math::fpFinite(x) || !Math::fpFinite(y)){
          if(points.no()>1) dc.drawLinesF(points.data(),points.no());
          points.clear();
          continue;
          }
        fx=(FXfloat)(plotleft+(x-xmin)*xscale);
        fy=(FXfloat)(plotbottom-(y-ymin)*yscale);
        if(points.no() && points.tail().x==fx && points.tail().y==fy) continue;
        points.append(FXPointF(fx,fy));
        }
      if(points.no()>1) dc.drawLinesF(points.data(),points.no());
      }

    // Markers
    if(curve->getMarkerStyle().style!=MARKER_NONE && curve->getMarkerStyle().size>0){
      for(j=0; j<count; j++){
        x=xdata->get(j);
        y=ydata->get(j);
        if(!Math::fpFinite(x) || !Math::fpFinite(y)) continue;
        px=plotleft+(FXint)Math::lrint((x-xmin)*xscale);
        py=plotbottom-(FXint)Math::lrint((y-ymin)*yscale);
        drawMarker(dc,curve->getMarkerStyle(),px,py);
        }
      }
    }
  dc.setAntialias(false);
  dc.setClipRectangle(save);
  lastdraw=FXThread::steadytime()-start;
  }


// Replace curve at index
void FX2DPlot::setCurve(FXint index,FXCurve* c){
  if(curves[index]!=c){
    delete curves[index];
    curves[index]=c;
    recalc();
    }
  }


// Append curve
void FX2DPlot::appendCurve(FXCurve* c){
  curves.append(c);
  recalc();
  }


// Insert curve at given position
void FX2DPlot::insertCurve(FXint index,FXCurve* c){
  curves.insert(index,c);
  recalc();
  }


// Remove curve at given position
void FX2DPlot::eraseCurve(FXint index){
  delete curves[index];
  curves.erase(index);
  recalc();
  }


// Remove all curves
void FX2DPlot::clearCurves(){
  for(FXint i=0; i<curves.no(); i++){
    delete curves[i];
    }
  curves.clear();
  recalc();
  }


// Save data
void FX2DPlot::save(FXStream& store) const {
  FX2DChart::save(store);
  ////
  }


// Load data
void FX2DPlot::load(FXStream& store){
  FX2DChart::load(store);
  ////
  }


// Destroy
FX2DPlot::~FX2DPlot(){
  clearCurves();
  }

}
