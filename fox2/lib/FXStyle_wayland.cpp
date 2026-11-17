/********************************************************************************
*                                                                               *
*                          S t y l e   ( W a y l a n d )                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxkeys.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXVisual.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXTheme.h"
#include "FXFrame.h"
#include "FXStyle.h"
#include "fxraster.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Platform part of the style: the antialiased rounded rectangle cache.
  - Wayland needs no compositor helper for this: FXRaster draws the rounded
    rectangles of the classic theme with its own arcs, so the cache and the
    XRender probe stay unused here.  They are kept so a later antialiased style
    can fill them in without touching the shared header.
*/

// Placeholder when antialiased compositing is unavailable
struct FXStyle::FXCornerMask {
  FXint         radius;
  FXCornerMask *next;
  };


using namespace FX;

/*******************************************************************************/

namespace FX {

// Drop cached chrome resources after a theme change
void FXStyle::invalidate(){
  masks=nullptr;
  }


// Probe whether the drawable supports antialiased compositing
FXbool FXStyle::hasXRender(FXDCWindow& dc){
  if(!xrenderChecked){
    xrenderChecked=true;
    xrender=false;
    }
  return xrender;
  }


// Return cached corner mask for the given radius
FXStyle::FXCornerMask* FXStyle::getMask(FXint radius){
  return nullptr;
  }


// Coverage of one pixel by a quarter disc, sampled 4x4
static FXuint discCoverage(FXint px,FXint py,FXint cx,FXint cy,FXint radius){
  const FXint samples=4;
  FXdouble rr=(FXdouble)radius*(FXdouble)radius;
  FXint inside=0;
  FXint sy,sx;
  for(sy=0; sy<samples; sy++){
    FXdouble fy=py+(sy+0.5)/samples-cy;
    for(sx=0; sx<samples; sx++){
      FXdouble fx=px+(sx+0.5)/samples-cx;
      if(fx*fx+fy*fy<=rr) inside++;
      }
    }
  return (FXuint)((inside*255+samples*samples/2)/(samples*samples));
  }


// Blend one antialiased quarter disc corner
static void blendCorner(FXDCWindow& dc,FXRaster* raster,FXint x,FXint y,FXint cx,FXint cy,FXint radius,FXColor color){
  FXint px,py;
  dc.setForeground(color);
  for(py=y; py<y+radius; py++){
    for(px=x; px<x+radius; px++){
      raster->blend(px,py,discCoverage(px,py,cx,cy,radius));
      }
    }
  }


// Distance from a point to a line segment
static FXdouble segmentDistance(FXdouble px,FXdouble py,FXdouble ax,FXdouble ay,FXdouble bx,FXdouble by){
  FXdouble dx=bx-ax;
  FXdouble dy=by-ay;
  FXdouble t=((px-ax)*dx+(py-ay)*dy)/(dx*dx+dy*dy);
  FXdouble qx,qy;
  if(t<0.0) t=0.0;
  else if(1.0<t) t=1.0;
  qx=ax+t*dx;
  qy=ay+t*dy;
  return sqrt((px-qx)*(px-qx)+(py-qy)*(py-qy));
  }


// Draw an antialiased check mark with supersampled segment coverage
void FXStyle::drawCheckMark(FXDCWindow& dc,FXint x,FXint y,FXint size,FXColor color){
  FXRaster *raster=(FXRaster*)dc.context();
  const FXint samples=4;
  FXdouble ax,ay,bx,by,cx,cy,width;
  FXint px,py,sx,sy;
  if(!raster || size<=0){
    dc.setForeground(color);
    dc.setLineWidth(FXMAX(1,size/7));
    dc.drawLine(x+(size*22)/100,y+(size*50)/100,x+(size*43)/100,y+(size*72)/100);
    dc.drawLine(x+(size*43)/100,y+(size*72)/100,x+(size*78)/100,y+(size*28)/100);
    dc.setLineWidth(1);
    return;
    }
  ax=x+size*0.22; ay=y+size*0.50;
  bx=x+size*0.43; by=y+size*0.72;
  cx=x+size*0.78; cy=y+size*0.28;
  width=FXMAX(1.0,size/14.0);
  dc.setForeground(color);
  for(py=y; py<y+size; py++){
    for(px=x; px<x+size; px++){
      FXint inside=0;
      for(sy=0; sy<samples; sy++){
        FXdouble fy=py+(sy+0.5)/samples;
        for(sx=0; sx<samples; sx++){
          FXdouble fx=px+(sx+0.5)/samples;
          if(segmentDistance(fx,fy,ax,ay,bx,by)<=width ||
             segmentDistance(fx,fy,bx,by,cx,cy)<=width) inside++;
          }
        }
      if(inside) raster->blend(px,py,(FXuint)((inside*255+samples*samples/2)/(samples*samples)));
      }
    }
  }


// Fill the outside of the rounded corners, antialiased: the coverage of a pixel
// by the quarter disc is the part of it that is inside, so the color goes down
// with the rest of the coverage
void FXStyle::cutRoundedCorners(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor color){
  FXRaster *raster=(FXRaster*)dc.context();
  FXint r,px,py,i;
  FXint cx[4],cy[4],ox[4],oy[4];
  if(!raster || w<=0 || h<=0) return;
  r=FXMIN(radius,FXMIN(w,h)/2);
  if(r<=0) return;
  dc.setForeground(color);
  cx[0]=cx[2]=x+r;
  cx[1]=cx[3]=x+w-r;
  cy[0]=cy[1]=y+r;
  cy[2]=cy[3]=y+h-r;
  ox[0]=ox[2]=x;
  ox[1]=ox[3]=x+w-r;
  oy[0]=oy[1]=y;
  oy[2]=oy[3]=y+h-r;
  for(i=0; i<4; i++){
    for(py=0; py<r; py++){
      for(px=0; px<r; px++){
        FXuint cov=discCoverage(ox[i]+px,oy[i]+py,cx[i],cy[i],r);
        if(cov<255) raster->blend(ox[i]+px,oy[i]+py,255-cov);
        }
      }
    }
  }


// Margin a popup needs for its drop shadow; the Wayland surface is premultiplied
// ARGB, so a translucent shadow is composited by the compositor
FXint FXStyle::shadowMargin(FXdouble scale){
  return FXMAX(theme->metric(METRIC_SHADOW_SIZE,scale),0);
  }


// Distance from a point to a rounded rectangle, negative inside it
static FXdouble roundRectDistance(FXdouble px,FXdouble py,FXdouble x,FXdouble y,FXdouble w,FXdouble h,FXdouble r){
  FXdouble cx=x+w*0.5;
  FXdouble cy=y+h*0.5;
  FXdouble dx=FXABS(px-cx)-(w*0.5-r);
  FXdouble dy=FXABS(py-cy)-(h*0.5-r);
  if(dx<0.0) dx=0.0;
  if(dy<0.0) dy=0.0;
  return sqrt(dx*dx+dy*dy)-r;
  }


// Draw the drop shadow around the surface of a popup: a black ring that fades
// out over the margin, written with the alpha of the surface so the window
// behind the popup shows through.  The area is the perimeter times the margin,
// which is a few thousand pixels, and cost is flat in the popup size.
void FXStyle::drawShadow(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius){
  FXRaster *raster=(FXRaster*)dc.context();
  FXint margin,px,py,r;
  if(!raster || w<=0 || h<=0) return;
  margin=shadowMargin(0.0);
  if(margin<=0) return;
  r=(radius>0)?FXMIN(radius,FXMIN(w,h)/2):0;
  for(py=y-margin; py<y+h+margin; py++){
    for(px=x-margin; px<x+w+margin; px++){
      FXdouble d=roundRectDistance(px+0.5,py+0.5,x,y,w,h,r);
      FXuint a=0;
      // The surface covers the inside; the ring is written even where the
      // shadow is fully faded, because it may hold the widget's own fill
      if(d<=0.0) continue;
      if(d<(FXdouble)margin){
        d=1.0-d/(FXdouble)margin;
        a=(FXuint)(d*d*96.0+0.5);
        }
      raster->plotAlpha(px,py,FXRGB(0,0,0),a);
      }
    }
  }


// Draw an antialiased rounded rectangle with a solid face and border
FXbool FXStyle::drawRoundRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth){
  FXRaster *raster=(FXRaster*)dc.context();
  FXint r,br,ir;
  if(!raster || w<=0 || h<=0 || radius<=0) return false;
  r=FXMIN(radius,FXMIN(w,h)/2);
  br=FXMIN(FXMAX(borderWidth,0),r);
  ir=FXMAX(r-br,0);

  // Straight bands of the outer shape; only the corners need antialiasing
  dc.setForeground(br?border:face);
  if(0<w-2*r) dc.fillRectangle(x+r,y,w-2*r,h);
  if(0<h-2*r) dc.fillRectangle(x,y+r,w,h-2*r);

  // Outer corners
  blendCorner(dc,raster,x,y,x+r,y+r,r,br?border:face);
  blendCorner(dc,raster,x+w-r,y,x+w-r,y+r,r,br?border:face);
  blendCorner(dc,raster,x,y+h-r,x+r,y+h-r,r,br?border:face);
  blendCorner(dc,raster,x+w-r,y+h-r,x+w-r,y+h-r,r,br?border:face);

  // Inner face, inset by the border width
  if(br){
    dc.setForeground(face);
    if(0<w-2*br-2*ir) dc.fillRectangle(x+br+ir,y+br,w-2*br-2*ir,h-2*br);
    if(0<h-2*br-2*ir) dc.fillRectangle(x+br,y+br+ir,w-2*br,h-2*br-2*ir);
    if(0<ir){
      blendCorner(dc,raster,x+br,y+br,x+br+ir,y+br+ir,ir,face);
      blendCorner(dc,raster,x+w-br-ir,y+br,x+w-br-ir,y+br+ir,ir,face);
      blendCorner(dc,raster,x+br,y+h-br-ir,x+br+ir,y+h-br-ir,ir,face);
      blendCorner(dc,raster,x+w-br-ir,y+h-br-ir,x+w-br-ir,y+h-br-ir,ir,face);
      }
    else{
      dc.setForeground(face);
      dc.fillRectangle(x+br,y+br,w-2*br,h-2*br);
      }
    }
  return true;
  }

}

#endif
