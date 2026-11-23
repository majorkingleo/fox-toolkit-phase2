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
#include "fx.h"
#include "chartdefs.h"
#include "chartutils.h"
#include "FXAxis.h"
#include "FXChart.h"
#include "FX2DChart.h"


/*
  Notes:
*/

using namespace FXCHART;

/*******************************************************************************/

namespace FXCHART {


// Zoom step per wheel notch and smallest rubber band size
const FXdouble zoomwheelstep=1.25;
const FXint    zoombandsize=8;


// Default chart fill style; the colors are replaced by the theme colors
const FillStyle defaultChartStyle={nullptr,FXRGB(255,255,255),FXRGBA(0,0,0,0),FXRGB(240,240,240),FXRGB(255,255,255),STIPPLE_NONE,FILLSTYLE_SOLID};


// Map
FXDEFMAP(FX2DChart) FX2DChartMap[]={
  FXMAPFUNC(SEL_COMMAND,FX2DChart::ID_XXX,FX2DChart::onCmdXXX),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FX2DChart::onMouseWheel),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FX2DChart::onLeftBtnPress),
  FXMAPFUNC(SEL_MOTION,0,FX2DChart::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FX2DChart::onLeftBtnRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FX2DChart::onUngrabbed),
  };


// Object implementation
FXIMPLEMENT(FX2DChart,FXChart,FX2DChartMap,ARRAYNUMBER(FX2DChartMap))


// Deserialization
FX2DChart::FX2DChart(){
  FXint i;
  chartstyle=defaultChartStyle;
  chartstyle.forecolor=getApp()->getBaseColor();
  chartstyle.lower=chartstyle.upper=getApp()->getBaseColor();
  bandanchorx=0;
  bandanchory=0;
  banding=false;
  zoomed=false;
  plotarea.x=plotarea.y=plotarea.w=plotarea.h=0;
  bandrect.x=bandrect.y=bandrect.w=bandrect.h=0;
  for(i=0; i<4; i++){
    savedrange[i][0]=0.0;
    savedrange[i][1]=0.0;
    }
  axes[XLO]=nullptr;
  axes[YLO]=nullptr;
  axes[XHI]=nullptr;
  axes[YHI]=nullptr;
  }


// Construct two-dimensional chart widget
FX2DChart::FX2DChart(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXChart(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  FXint i;
  chartstyle=defaultChartStyle;
  bandanchorx=0;
  bandanchory=0;
  banding=false;
  zoomed=false;
  plotarea.x=plotarea.y=plotarea.w=plotarea.h=0;
  bandrect.x=bandrect.y=bandrect.w=bandrect.h=0;
  for(i=0; i<4; i++){
    savedrange[i][0]=0.0;
    savedrange[i][1]=0.0;
    }
  axes[XLO]=new FXAxis(this,"X",TEXT_ATTACH_TOP,   TEXT_ATTACH_TOP,   AXIS_LINE|AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR|AXIS_TICKS_MINOR|AXIS_TICKS_INSIDE|AXIS_TICKS_AUTO|AXIS_GRID_MAJOR);
  axes[YLO]=new FXAxis(this,"Y",TEXT_ATTACH_RIGHT, TEXT_ATTACH_RIGHT, AXIS_LINE|AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR|AXIS_TICKS_MINOR|AXIS_TICKS_INSIDE|AXIS_TICKS_AUTO|AXIS_GRID_MAJOR);
//  axes[XHI]=new FXAxis(this,"X",TEXT_ATTACH_BOTTOM,TEXT_ATTACH_BOTTOM,AXIS_LINE);
//  axes[YHI]=new FXAxis(this,"Y",TEXT_ATTACH_LEFT,  TEXT_ATTACH_LEFT,  AXIS_LINE);


  axes[XHI]=new FXAxis(this,"X",TEXT_ATTACH_BOTTOM,TEXT_ATTACH_BOTTOM,AXIS_LINE|AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR|AXIS_TICKS_MINOR|AXIS_TICKS_INSIDE|AXIS_TICKS_AUTO);
  axes[YHI]=new FXAxis(this,"Y",TEXT_ATTACH_LEFT,  TEXT_ATTACH_LEFT,  AXIS_LINE|AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR|AXIS_TICKS_MINOR|AXIS_TICKS_INSIDE|AXIS_TICKS_AUTO);
  }


// Create window; register drag types
void FX2DChart::create(){
  FXChart::create();
  if(chartstyle.image) chartstyle.image->create();
  axes[XLO]->create();
  axes[YLO]->create();
  axes[XHI]->create();
  axes[YHI]->create();
  }


// Detach window; zero out drag types
void FX2DChart::detach(){
  FXChart::detach();
  if(chartstyle.image) chartstyle.image->detach();
  axes[XLO]->detach();
  axes[YLO]->detach();
  axes[XHI]->detach();
  axes[YHI]->detach();
  }


// Draw contents of chart
void FX2DChart::drawSelf(FXDC& dc) const {
  FXint plotleft=dc.getClipX()+marginleft;
  FXint plottop=dc.getClipY()+margintop;
  FXint plotright=dc.getClipX()+dc.getClipWidth()-marginright-1;
  FXint plotbottom=dc.getClipY()+dc.getClipHeight()-marginbottom-1;

  // Draw chart background
  drawRectangle(dc,backstyle,dc.getClipX(),dc.getClipY(),dc.getClipWidth(),dc.getClipHeight());

  // Space left after margins?
  if(plotleft<plotright && plottop<plotbottom){
    FXint caps=TEXT_ATTACH_CENTER|TEXT_JUSTIFY_CENTER;
    FXint half=0;
    FXint capx=0;
    FXint capy=0;
    FXint capw=0;
    FXint caph=0;
    FXint ss;

    // Account for caption
    if(options&CAPTION_SHOWN){
      caps=TEXT_JUSTIFY_CENTER;
      capx=(plotleft+plotright)/2;
      capy=(plottop+plotbottom)/2;
      capw=textWidth(captionfont,caption);
      caph=textHeight(captionfont,caption);
      if(options&CAPTION_LEFT){           // Caption left of chart
        caps|=TEXT_ATTACH_LEFT;
        capx=plotleft;
        plotleft+=capw+captionoffset;
        }
      else if(options&CAPTION_RIGHT){     // Caption right of chart
        caps|=TEXT_ATTACH_RIGHT;
        capx=plotright;
        plotright-=capw+captionoffset;
        }
      if(options&CAPTION_ABOVE){          // Caption above chart
        caps|=TEXT_ATTACH_TOP;
        capy=plottop;
        plottop+=caph+captionoffset;
        }
      else if(options&CAPTION_BELOW){     // Caption below chart
        caps|=TEXT_ATTACH_BOTTOM;
        capy=plotbottom;
        plotbottom-=caph+captionoffset;
        }
      }

    // Space left after caption?
    if(plotleft<plotright && plottop<plotbottom){

      // Account for the lower x-axis attributes to figure the plot size
      if(axes[XLO]->getStyle()&(AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR)){
        if(axes[XLO]->getStyle()&AXIS_TICKS_OUTSIDE){
          ss=0;
          if(axes[XLO]->getStyle()&AXIS_TICKS_MINOR) ss=axes[XLO]->getMinorTickSize();
          if(axes[XLO]->getStyle()&AXIS_TICKS_MAJOR) ss=axes[XLO]->getMajorTickSize();
          plotbottom-=ss;
          }
        if(axes[XLO]->getStyle()&AXIS_NUMBERS){
          if(!(axes[XLO]->getStyle()&AXIS_NUMBERS_INSIDE)){
            if(axes[XLO]->getNumberStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
              plotbottom-=textHeight(axes[XLO]->getNumberFont(),"8.8")+axes[XLO]->getLabelOffset();
              }
            else if(axes[XLO]->getNumberStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
              plotbottom-=textWidth(axes[XLO]->getNumberFont(),"8.8")+axes[XLO]->getLabelOffset();
              }
            }
          }
        if(axes[XLO]->getStyle()&AXIS_LABEL){
          if(axes[XLO]->getLabelStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
            plotbottom-=textHeight(axes[XLO]->getLabelFont(),axes[XLO]->getLabel())+axes[XLO]->getLabelOffset();
            }
          else if(axes[XLO]->getLabelStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
            plotbottom-=textWidth(axes[XLO]->getLabelFont(),axes[XLO]->getLabel())+axes[XLO]->getLabelOffset();
            }
          }
        }

      // Account for the upper x-axis attributes to figure the plot size
      if(axes[XHI]->getStyle()&(AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR)){
        if(axes[XHI]->getStyle()&AXIS_TICKS_OUTSIDE){
          ss=0;
          if(axes[XHI]->getStyle()&AXIS_TICKS_MINOR) ss=axes[XHI]->getMinorTickSize();
          if(axes[XHI]->getStyle()&AXIS_TICKS_MAJOR) ss=axes[XHI]->getMajorTickSize();
          plottop+=ss;
          }
        if(axes[XHI]->getStyle()&AXIS_NUMBERS){
          if(!(axes[XHI]->getStyle()&AXIS_NUMBERS_INSIDE)){
            if(axes[XHI]->getNumberStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
              plottop+=textHeight(axes[XHI]->getNumberFont(),"8.8")+axes[XHI]->getLabelOffset();
              }
            else if(axes[XHI]->getNumberStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
              plottop+=textWidth(axes[XHI]->getNumberFont(),"8.8")+axes[XHI]->getLabelOffset();
              }
            }
          }
        if(axes[XHI]->getStyle()&AXIS_LABEL){
          if(axes[XHI]->getLabelStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
            plottop+=textHeight(axes[XHI]->getLabelFont(),axes[XHI]->getLabel())+axes[XHI]->getLabelOffset();
            }
          else if(axes[XHI]->getLabelStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
            plottop+=textWidth(axes[XHI]->getLabelFont(),axes[XHI]->getLabel())+axes[XHI]->getLabelOffset();
            }
          }
        }

      // Account for the lower y-axis attributes to figure the plot size
      if(axes[YLO]->getStyle()&(AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR)){
        if(axes[YLO]->getStyle()&AXIS_TICKS_OUTSIDE){
          ss=0;
          if(axes[YLO]->getStyle()&AXIS_TICKS_MINOR) ss=axes[YLO]->getMinorTickSize();
          if(axes[YLO]->getStyle()&AXIS_TICKS_MAJOR) ss=axes[YLO]->getMajorTickSize();
          plotleft+=ss;
          }
        if(axes[YLO]->getStyle()&AXIS_NUMBERS){
          if(!(axes[YLO]->getStyle()&AXIS_NUMBERS_INSIDE)){
            if(axes[YLO]->getNumberStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
              plotleft+=textHeight(axes[YLO]->getNumberFont(),"8.8")+axes[YLO]->getNumberOffset();
              }
            else if(axes[YLO]->getNumberStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
              plotleft+=textWidth(axes[YLO]->getNumberFont(),"8.8")+axes[YLO]->getNumberOffset();
              }
            }
          }
        if(axes[YLO]->getStyle()&AXIS_LABEL){
          if(axes[YLO]->getLabelStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
            plotleft+=textHeight(axes[YLO]->getLabelFont(),axes[YLO]->getLabel())+axes[YLO]->getLabelOffset();
            }
          else if(axes[YLO]->getLabelStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
            plotleft+=textWidth(axes[YLO]->getLabelFont(),axes[YLO]->getLabel())+axes[YLO]->getLabelOffset();
            }
          }
        }

      // Account for the upper y-axis attributes to figure the plot size
      if(axes[YHI]->getStyle()&(AXIS_LABEL|AXIS_NUMBERS|AXIS_TICKS_MAJOR)){
        if(axes[YHI]->getStyle()&AXIS_TICKS_OUTSIDE){
          ss=0;
          if(axes[YHI]->getStyle()&AXIS_TICKS_MINOR) ss=axes[YHI]->getMinorTickSize();
          if(axes[YHI]->getStyle()&AXIS_TICKS_MAJOR) ss=axes[YHI]->getMajorTickSize();
          plotright-=ss;
          }
        if(axes[YHI]->getStyle()&AXIS_NUMBERS){
          if(!(axes[YHI]->getStyle()&AXIS_NUMBERS_INSIDE)){
            if(axes[YHI]->getNumberStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
              plotright-=textHeight(axes[YHI]->getNumberFont(),"8.8")+axes[YHI]->getNumberOffset();
              }
            else if(axes[YHI]->getNumberStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
              plotright-=textWidth(axes[YHI]->getNumberFont(),"8.8")+axes[YHI]->getNumberOffset();
              }
            }
          }
        if(axes[YHI]->getStyle()&AXIS_LABEL){
          if(axes[YHI]->getLabelStyle()&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
            plotright-=textHeight(axes[YHI]->getLabelFont(),axes[YHI]->getLabel())+axes[YHI]->getLabelOffset();
            }
          else if(axes[YHI]->getLabelStyle()&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
            plotright-=textWidth(axes[YHI]->getLabelFont(),axes[YHI]->getLabel())+axes[YHI]->getLabelOffset();
            }
          }
        }

      // The x numbers are centered on their ticks, so the first and the last
      // one stick out half a number past the ends of the x axis; keep that
      // room inside the frame so they are not clipped or run into the y numbers
      if(axes[XLO]->getStyle()&AXIS_NUMBERS){
        half=textWidth(axes[XLO]->getNumberFont(),"8.8")/2;
        plotleft+=half;
        plotright-=half;
        }

      // Draw plot if there's still space for it
      if(plotleft<plotright && plottop<plotbottom){

        // Remember where the plot is so mouse zooming can map pixels
        plotarea.x=(FXshort)plotleft;
        plotarea.y=(FXshort)plottop;
        plotarea.w=(FXshort)(plotright-plotleft);
        plotarea.h=(FXshort)(plotbottom-plottop);

        // Draw the caption
        if(options&CAPTION_SHOWN){
          if(!(options&(CAPTION_LEFT|CAPTION_RIGHT))){      // Caption centered above/below chart
            capx=(plotleft+plotright)/2;
            }
          if(!(options&(CAPTION_ABOVE|CAPTION_BELOW))){     // Caption centered left/right of chart
            capy=(plottop+plotbottom)/2;
            }
          dc.setFont(captionfont);
          dc.setForeground(captioncolor);
          drawText(dc,caption,capx,capy,caps);
          }

        // Now draw chart background
        drawRectangle(dc,chartstyle,plotleft,plottop,plotright-plotleft,plotbottom-plottop);

        // Draw the axes
        axes[XLO]->drawAxis(dc,plotleft,plotbottom,plotright,plotbottom,0,plottop-plotbottom);
        axes[YLO]->drawAxis(dc,plotleft,plotbottom,plotleft,plottop,plotright-plotleft,0);
        axes[XHI]->drawAxis(dc,plotleft,plottop,plotright,plottop,0,plotbottom-plottop);
        axes[YHI]->drawAxis(dc,plotright,plotbottom,plotright,plottop,plotleft-plotright,0);

        // Draw curves on top of the grid
        drawPlot(dc,plotleft,plottop,plotright,plotbottom);
        }
      }
    }
  }


// Draw the axis numbers without fading, after a zoom or a resize
void FX2DChart::skipFades(){
  FXint i;
  for(i=0; i<4; i++){
    axes[i]->skipFade();
    }
  }


// Remember the axis ranges before the first zoom
void FX2DChart::saveZoomRanges(){
  FXint i;
  if(zoomed) return;
  for(i=0; i<4; i++){
    savedrange[i][0]=axes[i]->getAxisRangeMin();
    savedrange[i][1]=axes[i]->getAxisRangeMax();
    }
  zoomed=true;
  }


// Apply new ranges to both sides of each axis and tell the target
void FX2DChart::setZoomRanges(FXdouble xlo,FXdouble xhi,FXdouble ylo,FXdouble yhi){
  if(!(xlo<xhi) || !(ylo<yhi)) return;
  axes[XLO]->setAxisRange(xlo,xhi);
  axes[XHI]->setAxisRange(xlo,xhi);
  axes[YLO]->setAxisRange(ylo,yhi);
  axes[YHI]->setAxisRange(ylo,yhi);
  skipFades();
  if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),nullptr);
  }


// Zoom the axes by factor around a point in the plot; factor below one zooms in
void FX2DChart::zoomAt(FXint x,FXint y,FXdouble factor){
  FXdouble xlo,xhi,ylo,yhi,dx,dy;
  if(factor<=0.0) return;
  if(plotarea.w<=0 || plotarea.h<=0) return;
  if(x<plotarea.x) x=plotarea.x;
  if(x>plotarea.x+plotarea.w) x=plotarea.x+plotarea.w;
  if(y<plotarea.y) y=plotarea.y;
  if(y>plotarea.y+plotarea.h) y=plotarea.y+plotarea.h;
  xlo=axes[XLO]->getAxisRangeMin();
  xhi=axes[XLO]->getAxisRangeMax();
  ylo=axes[YLO]->getAxisRangeMin();
  yhi=axes[YLO]->getAxisRangeMax();
  dx=xlo+(xhi-xlo)*(FXdouble)(x-plotarea.x)/plotarea.w;
  dy=yhi-(yhi-ylo)*(FXdouble)(y-plotarea.y)/plotarea.h;
  saveZoomRanges();
  setZoomRanges(dx+(xlo-dx)*factor,dx+(xhi-dx)*factor,dy+(ylo-dy)*factor,dy+(yhi-dy)*factor);
  }


// Zoom the axes to a rectangle in the plot
void FX2DChart::zoomRectangle(FXint x,FXint y,FXint w,FXint h){
  FXdouble xlo,xhi,ylo,yhi;
  FXint x0,x1,y0,y1;
  if(plotarea.w<=0 || plotarea.h<=0) return;
  if(w<0){ x+=w; w=-w; }
  if(h<0){ y+=h; h=-h; }
  if(w<zoombandsize || h<zoombandsize) return;
  x0=FXMAX(x,plotarea.x);
  y0=FXMAX(y,plotarea.y);
  x1=FXMIN(x+w,plotarea.x+plotarea.w);
  y1=FXMIN(y+h,plotarea.y+plotarea.h);
  if(x0>=x1 || y0>=y1) return;
  xlo=axes[XLO]->getAxisRangeMin();
  xhi=axes[XLO]->getAxisRangeMax();
  ylo=axes[YLO]->getAxisRangeMin();
  yhi=axes[YLO]->getAxisRangeMax();
  saveZoomRanges();
  setZoomRanges(xlo+(xhi-xlo)*(FXdouble)(x0-plotarea.x)/plotarea.w,
                xlo+(xhi-xlo)*(FXdouble)(x1-plotarea.x)/plotarea.w,
                yhi-(yhi-ylo)*(FXdouble)(y1-plotarea.y)/plotarea.h,
                yhi-(yhi-ylo)*(FXdouble)(y0-plotarea.y)/plotarea.h);
  }


// Return to the ranges in effect before the first zoom
void FX2DChart::resetZoom(){
  if(!zoomed) return;
  zoomed=false;
  axes[XLO]->setAxisRange(savedrange[XLO][0],savedrange[XLO][1]);
  axes[XHI]->setAxisRange(savedrange[XHI][0],savedrange[XHI][1]);
  axes[YLO]->setAxisRange(savedrange[YLO][0],savedrange[YLO][1]);
  axes[YHI]->setAxisRange(savedrange[YHI][0],savedrange[YHI][1]);
  skipFades();
  if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),nullptr);
  }


// Repaint the area covered by two band rectangles
static void updateBand(FXWindow* window,const FXRectangle& a,const FXRectangle& b){
  FXint x0=a.x;
  FXint y0=a.y;
  FXint x1=a.x+a.w;
  FXint y1=a.y+a.h;
  if(b.w>0 && b.h>0){
    x0=FXMIN(x0,b.x);
    y0=FXMIN(y0,b.y);
    x1=FXMAX(x1,b.x+b.w);
    y1=FXMAX(y1,b.y+b.h);
    }
  if(x0<x1 && y0<y1) window->update(x0-1,y0-1,x1-x0+2,y1-y0+2);
  }


// Draw the rubber band on top of the chart image
void FX2DChart::drawOverlay(FXDC& dc) const {
  if(banding && bandrect.w>0 && bandrect.h>0){
    dc.setForeground(FXRGB(0,0,0));
    dc.drawFocusRectangle(bandrect.x,bandrect.y,bandrect.w,bandrect.h);
    }
  }


// Zoom in and out with the mouse wheel
long FX2DChart::onMouseWheel(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(!isEnabled()) return 0;
  if(event->win_x<plotarea.x || event->win_x>=plotarea.x+plotarea.w) return 0;
  if(event->win_y<plotarea.y || event->win_y>=plotarea.y+plotarea.h) return 0;
  zoomAt(event->win_x,event->win_y,event->code>0 ? 1.0/zoomwheelstep : zoomwheelstep);
  return 1;
  }


// Start a rubber band, or reset the zoom on a double click
long FX2DChart::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(!isEnabled()) return 0;
  if(event->win_x<plotarea.x || event->win_x>=plotarea.x+plotarea.w) return 0;
  if(event->win_y<plotarea.y || event->win_y>=plotarea.y+plotarea.h) return 0;
  if(event->click_count>1){
    resetZoom();
    return 1;
    }
  bandanchorx=event->win_x;
  bandanchory=event->win_y;
  bandrect.x=(FXshort)bandanchorx;
  bandrect.y=(FXshort)bandanchory;
  bandrect.w=0;
  bandrect.h=0;
  banding=true;
  grab();
  return 1;
  }


// Stretch the rubber band
long FX2DChart::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXRectangle old;
  FXint x,y;
  if(!banding) return 0;
  x=event->win_x;
  y=event->win_y;
  if(x<plotarea.x) x=plotarea.x;
  if(x>plotarea.x+plotarea.w) x=plotarea.x+plotarea.w;
  if(y<plotarea.y) y=plotarea.y;
  if(y>plotarea.y+plotarea.h) y=plotarea.y+plotarea.h;
  old=bandrect;
  bandrect.x=(FXshort)FXMIN(bandanchorx,x);
  bandrect.y=(FXshort)FXMIN(bandanchory,y);
  bandrect.w=(FXshort)(FXABS(x-bandanchorx)+1);
  bandrect.h=(FXshort)(FXABS(y-bandanchory)+1);
  updateBand(this,old,bandrect);
  return 1;
  }


// Zoom to the rubber band
long FX2DChart::onLeftBtnRelease(FXObject*,FXSelector,void*){
  if(!banding) return 0;
  banding=false;
  ungrab();
  updateBand(this,bandrect,FXRectangle(0,0,0,0));
  zoomRectangle(bandrect.x,bandrect.y,bandrect.w,bandrect.h);
  bandrect.w=0;
  bandrect.h=0;
  return 1;
  }


// Cancel the rubber band when the grab is lost
long FX2DChart::onUngrabbed(FXObject*,FXSelector,void*){
  if(banding){
    banding=false;
    updateBand(this,bandrect,FXRectangle(0,0,0,0));
    bandrect.w=0;
    bandrect.h=0;
    return 1;
    }
  return 0;
  }


// Draw chart contents; subclasses draw curves here
void FX2DChart::drawPlot(FXDC&,FXint,FXint,FXint,FXint) const {
  }


long FX2DChart::onCmdXXX(FXObject*,FXSelector,void*){
  return 1;
  }


// Set chart fill style
void FX2DChart::setChartStyle(const FillStyle& fs){
  chartstyle=fs;
  recalc();
  }


// Save data
void FX2DChart::save(FXStream& store) const {
  FXChart::save(store);
  ////
  }


// Load data
void FX2DChart::load(FXStream& store){
  FXChart::load(store);
  ////
  }


// Destroy
FX2DChart::~FX2DChart(){
  delete axes[XLO];
  delete axes[YLO];
  delete axes[XHI];
  delete axes[YHI];
  }

}
