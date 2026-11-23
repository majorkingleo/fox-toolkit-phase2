/********************************************************************************
*                                                                               *
*                             Style   ( W i n 3 2 )                             *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxpriv.h"

#if defined(WIN32)

// GDI+ needs the plain Windows names that fxdefs.h shadows
#define VOID void
#define PROPID ULONG
#include <gdiplus.h>
#undef PROPID
#undef VOID

/*
  Notes:

  - Platform part of the style: the rounded rectangle chrome, drawn with GDI+.
  - GDI has no antialiased geometry, but GDI+ (part of Windows since XP) fills
    the same shapes with coverage blending.  It composites against whatever the
    widget already painted, so a corner blends into the parent's background the
    way the XRender mask and the FXRaster coverage do on the other backends; no
    per-radius corner cache is needed here, and invalidate() has nothing to free.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

namespace {

// Convert a FOX color to a GDI+ color
static Gdiplus::Color gdiplusColor(FXColor color){
  return Gdiplus::Color(255,FXREDVAL(color),FXGREENVAL(color),FXBLUEVAL(color));
  }


// Coverage blending, aligned to the pixel grid: without the half pixel offset
// GDI+ centres the geometry on the pixel centres, which smears the straight
// edges of the outline over two rows instead of one
static void gdiplusAntialias(Gdiplus::Graphics& graphics){
  graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  }


// Outline of a rounded rectangle covering [x,x+w) x [y,y+h)
static FXbool gdiplusRoundRect(Gdiplus::GraphicsPath& path,FXint x,FXint y,FXint w,FXint h,FXint radius){
  FXdouble d=2.0*radius;
  if(radius<=0 || w<2*radius || h<2*radius){
    path.AddRectangle(Gdiplus::Rect(x,y,w,h));
    }
  else{
    path.StartFigure();
    path.AddArc((Gdiplus::REAL)x,(Gdiplus::REAL)y,(Gdiplus::REAL)d,(Gdiplus::REAL)d,180.0f,90.0f);
    path.AddArc((Gdiplus::REAL)(x+w-d),(Gdiplus::REAL)y,(Gdiplus::REAL)d,(Gdiplus::REAL)d,270.0f,90.0f);
    path.AddArc((Gdiplus::REAL)(x+w-d),(Gdiplus::REAL)(y+h-d),(Gdiplus::REAL)d,(Gdiplus::REAL)d,0.0f,90.0f);
    path.AddArc((Gdiplus::REAL)x,(Gdiplus::REAL)(y+h-d),(Gdiplus::REAL)d,(Gdiplus::REAL)d,90.0f,90.0f);
    path.CloseFigure();
    }
  return path.GetLastStatus()==Gdiplus::Ok;
  }

}

// Drop cached chrome resources after a theme change; GDI+ keeps no cache
void FXStyle::invalidate(){
  masks=nullptr;
  }


// Probe whether the drawable supports antialiased compositing
FXbool FXStyle::hasXRender(FXDCWindow& dc){
  if(!xrenderChecked){
    xrenderChecked=true;
    }
  return xrender;
  }


// Return cached corner mask for the given radius
FXStyle::FXCornerMask* FXStyle::getMask(FXint radius){
  return nullptr;
  }


// Draw a check mark; GDI has no small AA primitive here, use the line primitives
void FXStyle::drawCheckMark(FXDCWindow& dc,FXint x,FXint y,FXint size,FXColor color){
  dc.setForeground(color);
  dc.setLineWidth(FXMAX(1,size/7));
  dc.drawLine(x+(size*22)/100,y+(size*50)/100,x+(size*43)/100,y+(size*72)/100);
  dc.drawLine(x+(size*43)/100,y+(size*72)/100,x+(size*78)/100,y+(size*28)/100);
  dc.setLineWidth(1);
  }


// Fill the outside of the rounded corners, antialiased: the outer rectangle
// minus the rounded shape, filled in one alternate-filled path, so only the
// four slivers beyond the arcs get painted
void FXStyle::cutRoundedCorners(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor color){
  FXint r=FXMIN(radius,FXMIN(w,h)/2);
  if(0<r && 0<w && 0<h && fxgdiplus()){
    Gdiplus::Graphics graphics((HDC)dc.context());
    Gdiplus::GraphicsPath path;
    Gdiplus::GraphicsPath shape;
    if(graphics.GetLastStatus()==Gdiplus::Ok && gdiplusRoundRect(shape,x,y,w,h,r)){
      gdiplusAntialias(graphics);
      path.AddRectangle(Gdiplus::Rect(x,y,w,h));
      path.AddPath(&shape,false);
      path.SetFillMode(Gdiplus::FillModeAlternate);
      Gdiplus::SolidBrush brush(gdiplusColor(color));
      if(graphics.FillPath(&brush,&path)==Gdiplus::Ok) return;
      }
    }
  fillOutsideCorners(dc,x,y,w,h,radius,color);
  }


// Margin a popup needs for its drop shadow.  Only a surface with an alpha
// channel can carry a translucent shadow, which the Win32 window does not have,
// so no margin is reserved and the popups keep their size
FXint FXStyle::shadowMargin(FXdouble){
  return 0;
  }


// Draw the drop shadow of a popup; nothing to draw without an alpha surface
void FXStyle::drawShadow(FXDCWindow&,FXint,FXint,FXint,FXint,FXint){
  }


// Draw a rounded rectangle: the border first, then the face inset by the border
// width, so the hairline stays inside the shape and both edges are blended once
FXbool FXStyle::drawRoundRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth){
  FXint r,br;
  if(w<=0 || h<=0 || radius<=0 || !fxgdiplus()) return false;
  r=FXMIN(radius,FXMIN(w,h)/2);
  br=FXMIN(FXMAX(borderWidth,0),r);
  Gdiplus::Graphics graphics((HDC)dc.context());
  Gdiplus::GraphicsPath path;
  if(graphics.GetLastStatus()!=Gdiplus::Ok || !gdiplusRoundRect(path,x,y,w,h,r)) return false;
  gdiplusAntialias(graphics);
  if(0<br){
    Gdiplus::SolidBrush edge(gdiplusColor(border));
    if(graphics.FillPath(&edge,&path)!=Gdiplus::Ok) return false;
    if(2*br<w && 2*br<h){
      Gdiplus::GraphicsPath inner;
      Gdiplus::SolidBrush faceBrush(gdiplusColor(face));
      if(!gdiplusRoundRect(inner,x+br,y+br,w-2*br,h-2*br,r-br)) return false;
      return graphics.FillPath(&faceBrush,&inner)==Gdiplus::Ok;
      }
    return true;
    }
  Gdiplus::SolidBrush faceBrush(gdiplusColor(face));
  return graphics.FillPath(&faceBrush,&path)==Gdiplus::Ok;
  }

}

#endif
