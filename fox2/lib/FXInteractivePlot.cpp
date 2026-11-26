/********************************************************************************
*                                                                               *
*                  I n t e r a c t i v e   P l o t   W i d g e t                *
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
#include "FXAxis.h"
#include "FXCurve.h"
#include "FXData.h"
#include "FXInteractivePlot.h"

#include <limits>
#include <vector>

using namespace FXCHART;

/*******************************************************************************/

namespace FXCHART {


// Map
FXDEFMAP(FXInteractivePlot) FXInteractivePlotMap[]={
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXInteractivePlot::onLeftBtnPress),
  FXMAPFUNC(SEL_MOTION,0,FXInteractivePlot::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXInteractivePlot::onLeftBtnRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXInteractivePlot::onUngrabbed),
  FXMAPFUNC(SEL_LEAVE,0,FXInteractivePlot::onLeave),
  };


// Object implementation
FXIMPLEMENT(FXInteractivePlot,FX2DPlot,FXInteractivePlotMap,ARRAYNUMBER(FXInteractivePlotMap))


// Deserialization
FXInteractivePlot::FXInteractivePlot():panmode(false),panning(false),cursorinside(false),cursorx(0),cursory(0),pananchorx(0),pananchory(0){
  panrange[0]=panrange[1]=panrange[2]=panrange[3]=0.0;
  }


// Construct interactive plot
FXInteractivePlot::FXInteractivePlot(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FX2DPlot(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb),panmode(false),panning(false),cursorinside(false),cursorx(0),cursory(0),pananchorx(0),pananchory(0){
  panrange[0]=panrange[1]=panrange[2]=panrange[3]=0.0;
  }


// Change pan mode
void FXInteractivePlot::setPanMode(FXbool mode){
  panmode=mode;
  }


// Send SEL_CHANGED with curve index encoded as FXuval(index+1); zero remains reserved for chart-range changes
void FXInteractivePlot::notifyCurvePicked(FXint curve){
  if(curve>=0 && target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXuval)(curve+1));
  }


// Start a rubber band, or start panning
long FXInteractivePlot::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  if(!panmode) return FX2DChart::onLeftBtnPress(nullptr,0,ptr);
  FXEvent* event=(FXEvent*)ptr;
  const FXRectangle& area=getPlotArea();
  if(!isEnabled() || event->win_x<area.x || event->win_x>=area.x+area.w || event->win_y<area.y || event->win_y>=area.y+area.h) return 0;
  if(event->click_count>1){
    resetZoom();
    return 1;
    }
  pananchorx=event->win_x;
  pananchory=event->win_y;
  panrange[0]=getAxis(FX2DChart::XLO)->getAxisRangeMin();
  panrange[1]=getAxis(FX2DChart::XLO)->getAxisRangeMax();
  panrange[2]=getAxis(FX2DChart::YLO)->getAxisRangeMin();
  panrange[3]=getAxis(FX2DChart::YLO)->getAxisRangeMax();
  panning=true;
  grab();
  return 1;
  }


// Pan the view or update the crosshair
long FXInteractivePlot::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(panmode && panning){
    const FXRectangle& area=getPlotArea();
    if(area.w<=0 || area.h<=0) return 1;
    FXdouble dx=(event->win_x-pananchorx)*(panrange[1]-panrange[0])/area.w;
    FXdouble dy=(event->win_y-pananchory)*(panrange[3]-panrange[2])/area.h;
    setViewRange(panrange[0]-dx,panrange[1]-dx,panrange[2]+dy,panrange[3]+dy);
    return 1;
    }
  const FXRectangle& area=getPlotArea();
  FXbool inside=event->win_x>=area.x && event->win_x<area.x+area.w && event->win_y>=area.y && event->win_y<area.y+area.h;
  if(cursorinside!=inside || (inside && (cursorx!=event->win_x || cursory!=event->win_y))){
    cursorinside=inside;
    cursorx=event->win_x;
    cursory=event->win_y;
    update();
    }
  if(panmode) return 0;
  return FX2DChart::onMotion(nullptr,0,ptr);
  }


// Finish pan or zoom and pick a curve on a click
long FXInteractivePlot::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(panmode){
    if(!panning) return 0;
    FXbool click=FXABS(event->win_x-pananchorx)<8 && FXABS(event->win_y-pananchory)<8;
    panning=false;
    ungrab();
    if(click) notifyCurvePicked(pickCurve(event->win_x,event->win_y));
    return 1;
    }
  FXbool click=banding && FXABS(event->win_x-bandanchorx)<8 && FXABS(event->win_y-bandanchory)<8;
  long handled=FX2DChart::onLeftBtnRelease(nullptr,0,ptr);
  if(click) notifyCurvePicked(pickCurve(event->win_x,event->win_y));
  return handled;
  }


// Cancel pan when the grab is lost
long FXInteractivePlot::onUngrabbed(FXObject*,FXSelector,void* ptr){
  if(!panmode) return FX2DChart::onUngrabbed(nullptr,0,ptr);
  if(panning){
    panning=false;
    return 1;
    }
  return 0;
  }


// Hide the crosshair when the pointer leaves the plot
long FXInteractivePlot::onLeave(FXObject*,FXSelector,void*){
  if(cursorinside && !panning){
    cursorinside=false;
    update();
    }
  return 1;
  }


// Return the curve nearest to the given pixel
FXint FXInteractivePlot::pickCurve(FXint x,FXint y,FXint radius) const {
  const FXRectangle& area=getPlotArea();
  if(radius<0 || area.w<=0 || area.h<=0 || x<area.x || x>=area.x+area.w || y<area.y || y>=area.y+area.h) return -1;
  FXdouble bestdistance=(FXdouble)radius*radius;
  FXint bestcurve=-1;
  for(FXint i=0; i<numCurves(); i++){
    FXCurve* curve=getCurve(i);
    if(!curve->isVisible()) continue;
    FXData* xdata=curve->getXData();
    FXData* ydata=curve->getYData();
    if(!xdata || !ydata) continue;
    FXAxis* xaxis=getAxis(curve->getXAxis());
    FXAxis* yaxis=getAxis(curve->getYAxis());
    FXdouble xmin=xaxis->getAxisRangeMin();
    FXdouble xmax=xaxis->getAxisRangeMax();
    FXdouble ymin=yaxis->getAxisRangeMin();
    FXdouble ymax=yaxis->getAxisRangeMax();
    if(!(xmax>xmin) || !(ymax>ymin)) continue;
    FXbool lines=curve->getLineStyle().style!=LINESTYLE_NONE;
    FXbool markers=curve->getMarkerStyle().style!=MARKER_NONE && curve->getMarkerStyle().size>0;
    if(!lines && !markers) continue;
    FXint count=FXMIN(xdata->no(),ydata->no());
    std::vector<FXPointF> points;
    if(lines) points.reserve(count);
    auto checkPoints=[&](){
      if(!points.empty()){
        FXdouble distance=0.0;
        if(pickNearest(points.data(),(FXint)points.size(),x,y,radius,&distance)>=0 && (bestcurve<0 || distance<bestdistance)){
          bestdistance=distance;
          bestcurve=i;
          }
        points.clear();
        }
      };
    for(FXint j=0; j<count; j++){
      FXdouble xv=xdata->get(j);
      FXdouble yv=ydata->get(j)*curve->getScale()+curve->getOffset();
      if(!Math::fpFinite(xv) || !Math::fpFinite(yv)){
        checkPoints();
        continue;
        }
      FXdouble px=area.x+(xv-xmin)*area.w/(xmax-xmin);
      FXdouble py=area.y+area.h-(yv-ymin)*area.h/(ymax-ymin);
      FXdouble maxfloat=(FXdouble)std::numeric_limits<FXfloat>::max();
      if(!Math::fpFinite(px) || !Math::fpFinite(py) || FXABS(px)>maxfloat || FXABS(py)>maxfloat){
        checkPoints();
        continue;
        }
      FXPointF point;
      point.x=(FXfloat)px;
      point.y=(FXfloat)py;
      if(lines) points.push_back(point);
      else{
        FXdouble distance=0.0;
        if(pickNearest(&point,1,x,y,radius,&distance)>=0 && (bestcurve<0 || distance<bestdistance)){
          bestdistance=distance;
          bestcurve=i;
          }
        }
      }
    checkPoints();
    }
  return bestcurve;
  }


// Draw the crosshair and snapped values
void FXInteractivePlot::drawOverlay(FXDC& dc) const {
  FX2DChart::drawOverlay(dc);
  if(!cursorinside) return;
  const FXRectangle& area=getPlotArea();
  if(area.w<=0 || area.h<=0) return;
  FXdouble xmin=getAxis(FX2DChart::XLO)->getAxisRangeMin();
  FXdouble xmax=getAxis(FX2DChart::XLO)->getAxisRangeMax();
  if(!(xmax>xmin)) return;
  FXdouble xvalue=xmin+(xmax-xmin)*(cursorx-area.x)/area.w;
  FXColor crosshair=readableColor(FXRGB(150,150,150),getChartStyle().forecolor);
  dc.setForeground(crosshair);
  dc.setLineWidth(1);
  dc.setLineStyle(LINE_ONOFF_DASH);
  dc.drawLine(cursorx,area.y,cursorx,area.y+area.h);
  dc.drawLine(area.x,cursory,area.x+area.w,cursory);
  dc.setLineStyle(LINE_SOLID);
  FXFont* font=getAxis(FX2DChart::XLO)->getNumberFont();
  dc.setFont(font);
  FXint textheight=font->getFontHeight();
  FXint textx=area.x+4;
  FXint texty=area.y+textheight+3;
  FXint rowheight=textheight+3;
  for(FXint i=0; i<numCurves(); i++){
    FXCurve* curve=getCurve(i);
    if(!curve->isVisible()) continue;
    FXData* xs=curve->getXData();
    FXData* ys=curve->getYData();
    if(!xs || !ys) continue;
    FXint count=FXMIN(xs->no(),ys->no());
    if(count<=0) continue;
    FXint index=xs->findIndexAt(xvalue);
    if(index<0) continue;
    FXdouble samplex=xs->get(index);
    FXdouble sampley=ys->get(index);
    if(index+1<count){
      FXdouble nextx=xs->get(index+1);
      FXdouble nexty=ys->get(index+1);
      if(Math::fpFinite(samplex) && Math::fpFinite(sampley) && Math::fpFinite(nextx) && Math::fpFinite(nexty) && nextx>samplex && xvalue>=samplex && xvalue<=nextx){
        sampley+=(nexty-sampley)*(xvalue-samplex)/(nextx-samplex);
        }
      }
    if(!Math::fpFinite(sampley)) continue;
    sampley=sampley*curve->getScale()+curve->getOffset();
    FXAxis* yaxis=getAxis(curve->getYAxis());
    FXdouble ymin=yaxis->getAxisRangeMin();
    FXdouble ymax=yaxis->getAxisRangeMax();
    if(!(ymax>ymin) || sampley<ymin || sampley>ymax) continue;
    FXint pointx=area.x+(FXint)Math::lrint((xvalue-xmin)*area.w/(xmax-xmin));
    FXint pointy=area.y+area.h-(FXint)Math::lrint((sampley-ymin)*area.h/(ymax-ymin));
    dc.setForeground(curve->getLineStyle().color);
    dc.fillEllipse(pointx-3,pointy-3,7,7);
    FXString text=FXString::value("%s: %.6g",curve->getLabel().text(),sampley);
    FXint textwidth=textWidth(font,text);
    if(textx+textwidth>area.x+area.w-4){
      textx=area.x+4;
      texty+=rowheight;
      }
    if(texty>area.y+area.h) break;
    dc.setForeground(curve->getLineStyle().color);
    drawText(dc,text,textx,texty,TEXT_ATTACH_LEFT|TEXT_ATTACH_BOTTOM);
    textx+=textwidth+12;
    }
  }


// Destroy
FXInteractivePlot::~FXInteractivePlot(){
  }

}
