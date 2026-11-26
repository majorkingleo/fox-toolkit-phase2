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


// Draw an antialiased check mark: the capsule around the two segments that the
// X11 mask and the FXRaster path build, stroked with round caps and joins
void FXStyle::drawCheckMark(FXDCWindow& dc,FXint x,FXint y,FXint size,FXColor color){
  if(0<size && fxgdiplus()){
    Gdiplus::Graphics graphics((HDC)dc.context());
    Gdiplus::GraphicsPath path;
    Gdiplus::Pen pen(gdiplusColor(color),(Gdiplus::REAL)(2.0*FXMAX(1.0,size/14.0)));
    if(graphics.GetLastStatus()==Gdiplus::Ok){
      gdiplusAntialias(graphics);
      path.AddLine((Gdiplus::REAL)(x+size*0.22),(Gdiplus::REAL)(y+size*0.50),(Gdiplus::REAL)(x+size*0.43),(Gdiplus::REAL)(y+size*0.72));
      path.AddLine((Gdiplus::REAL)(x+size*0.43),(Gdiplus::REAL)(y+size*0.72),(Gdiplus::REAL)(x+size*0.78),(Gdiplus::REAL)(y+size*0.28));
      pen.SetLineCap(Gdiplus::LineCapRound,Gdiplus::LineCapRound,Gdiplus::DashCapRound);
      pen.SetLineJoin(Gdiplus::LineJoinRound);
      if(graphics.DrawPath(&pen,&path)==Gdiplus::Ok) return;
      }
    }
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


// Margin a popup needs for its drop shadow.  A Win32 window has no alpha
// channel, so a popup cannot carry the shadow inside itself and none of the
// shared layout has to make room: the ring goes into a layered window around
// the popup (fxshadow_win32.cpp), which reads the metric itself
FXint FXStyle::shadowMargin(FXdouble){
  return 0;
  }


// Draw the drop shadow of a popup; it goes into the layered window instead
void FXStyle::drawShadow(FXDCWindow&,FXint,FXint,FXint,FXint,FXint){
  }


// Coverage of one pixel by a quarter disc, sampled 4x4; the same coverage the
// Wayland rasterizer uses, so all backends get the same corner shape
static FXuint cornerCoverage(FXint px,FXint py,FXint cx,FXint cy,FXint radius){
  const FXint samples=4;
  FXdouble rr=(FXdouble)radius*(FXdouble)radius;
  FXint inside=0;
  FXint sx,sy;
  for(sy=0; sy<samples; sy++){
    FXdouble fy=py+(sy+0.5)/samples-cy;
    for(sx=0; sx<samples; sx++){
      FXdouble fx=px+(sx+0.5)/samples-cx;
      if(fx*fx+fy*fy<=rr) inside++;
      }
    }
  return (FXuint)((inside*255+samples*samples/2)/(samples*samples));
  }


// Blend b over a with the given coverage
static FXColor blendCoverage(FXColor a,FXColor b,FXuint coverage){
  FXint t=(FXint)coverage;
  FXint s=255-t;
  return FXRGB((FXREDVAL(a)*s+FXREDVAL(b)*t+127)/255,
               (FXGREENVAL(a)*s+FXGREENVAL(b)*t+127)/255,
               (FXBLUEVAL(a)*s+FXBLUEVAL(b)*t+127)/255);
  }


// Draw a rounded rectangle with software coverage blending.  GDI+ silently
// drops smoothing mode on some drivers (and under Wine), which left the four
// corners of every themed control hard and stair-stepped.  The straight runs
// are ordinary rectangles; only the four corner squares need per-pixel work.
FXbool FXStyle::drawRoundRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth){
  FXint r,br,px,py,cx,cy,corner,index,n;
  FXbool hasface;
  FXColor *back=nullptr;
  FXColor out;
  if(w<=0 || h<=0 || radius<=0) return false;
  r=FXMIN(radius,FXMIN(w,h)/2);
  br=FXMIN(FXMAX(borderWidth,0),r);
  hasface=(2*br<w && 2*br<h);

  // Remember the background under the corner squares before the shape covers it
  n=4*r*r;
  if(!allocElms(back,n)) back=nullptr;
  if(back){
    index=0;
    for(corner=0; corner<4; corner++){
      FXint x0=x+((corner&1)?w-r:0);
      FXint y0=y+((corner&2)?h-r:0);
      for(py=y0; py<y0+r; py++){
        for(px=x0; px<x0+r; px++){
          back[index++]=dc.readPixel(px,py);
          }
        }
      }
    }

  // Opaque straight runs; the corner squares are replaced below
  dc.setForeground(br?border:face);
  dc.fillRectangle(x,y,w,h);
  if(br && hasface){
    dc.setForeground(face);
    dc.fillRectangle(x+br,y+br,w-2*br,h-2*br);
    }

  // Replace each corner square with background, border and face at their
  // respective coverage, in the same order the GDI+ fills would use
  if(back){
    index=0;
    for(corner=0; corner<4; corner++){
      FXint x0=x+((corner&1)?w-r:0);
      FXint y0=y+((corner&2)?h-r:0);
      cx=x+((corner&1)?w-r:r);
      cy=y+((corner&2)?h-r:r);
      for(py=y0; py<y0+r; py++){
        for(px=x0; px<x0+r; px++){
          FXuint co=cornerCoverage(px,py,cx,cy,r);
          FXuint ci=0;
          if(br==0){
            out=blendCoverage(back[index++],face,co);
            }
          else{
            if(hasface) ci=cornerCoverage(px,py,cx,cy,r-br);
            out=blendCoverage(back[index++],border,co);
            out=blendCoverage(out,face,ci);
            }
          dc.setForeground(out);
          dc.drawPoint(px,py);
          }
        }
      }
    freeElms(back);
    }
  return true;
  }

}

#endif
