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
