/********************************************************************************
*                                                                               *
*                       D C   W i n d o w   ( W a y l a n d )                   *
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
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegion.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXDrawable.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXFont.h"
#include "FXDC.h"
#include "fxglyphs.h"
#include "fxraster.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"
#include "FXDCWindow.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Wayland has no drawing requests, so the DC draws with the software
    rasterizer of fxraster.h into the pixel buffer of the drawable.  The
    rasterizer lives in the DC's context handle while the DC is bound.
  - The FXDC members keep the state the caller sets; every setter copies it to
    the rasterizer, which needs no lazy flag bookkeeping because copying a few
    words is cheaper than tracking which of them are stale.
  - A window's buffer belongs to its shell surface and arrives with the virtual
    window table of phase 4; until then only images can be drawn into.
  - drawIconShaded and drawIconSunken have no server-side shape and etch mask
    to work with, so they draw the icon and leave the shading to the theme; the
    deviation is recorded in docs/wayland-plan.md.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// The rasterizer bound to this DC
static inline FXRaster* raster(void* ctx){
  return (FXRaster*)ctx;
  }


// Return the pixel buffer of a drawable; an image owns its pixels, a window
// draws into the frame buffer of the shell it belongs to, at its place there
static FXbool getSurface(FXDrawable* draw,FXColor*& pixels,FXint& w,FXint& h,FXint& stride){
  FXImage* image=dynamic_cast<FXImage*>(draw);
  FXWindow* window;
  if(image){
    pixels=image->getData();
    w=image->getWidth();
    h=image->getHeight();
    stride=w;
    return pixels!=nullptr;
    }
  w=draw->getWidth();
  h=draw->getHeight();
  stride=w;
  pixels=nullptr;
  if((window=dynamic_cast<FXWindow*>(draw))!=nullptr && window->id()){
    return fxwaylandBindWindow(fxwayland(window->getApp()->getDisplay()),window->id(),pixels,stride);
    }
  return false;
  }


/*******************************************************************************/

// Construct for painting in response to expose
FXDCWindow::FXDCWindow(FXDrawable* draw,FXEvent* event):FXDC(draw->getApp()){
  surface=nullptr;
  devfg=0;
  devbg=0;
  oldpalette=0;
  oldbrush=0;
  oldpen=0;
  needsNewBrush=false;
  needsNewPen=false;
  needsPath=false;
  needsClipReset=false;
  xftDraw=nullptr;
  flags=0;
  begin(draw);
  setClipRectangle(event->rect);
  }


// Construct for normal drawing
FXDCWindow::FXDCWindow(FXDrawable* draw):FXDC(draw->getApp()){
  surface=nullptr;
  devfg=0;
  devbg=0;
  oldpalette=0;
  oldbrush=0;
  oldpen=0;
  needsNewBrush=false;
  needsNewPen=false;
  needsPath=false;
  needsClipReset=false;
  xftDraw=nullptr;
  flags=0;
  begin(draw);
  }


// Destructor
FXDCWindow::~FXDCWindow(){
  end();
  }


// Begin drawing
void FXDCWindow::begin(FXDrawable* draw){
  FXColor* pixels;
  FXint w,h,stride;
  if(draw && !surface){
    surface=draw;
    surface->visual->create();
    devfg=surface->visual->getPixel(fg);
    devbg=surface->visual->getPixel(bg);

    // Nothing to draw into: the window has no shell or no frame yet
    if(!getSurface(draw,pixels,w,h,stride)) return;
    ctx=new FXRaster(pixels,w,h,stride);
    raster(ctx)->setForeground(fg);
    raster(ctx)->setBackground(bg);
    raster(ctx)->setLineWidth(width);
    raster(ctx)->setLineCap(cap);
    raster(ctx)->setLineJoin(join);
    raster(ctx)->setLineStyle(style);
    raster(ctx)->setFillStyle(fill);
    raster(ctx)->setFillRule(rule);
    raster(ctx)->setFunction(rop);
    raster(ctx)->setDashes(dashoff,dashpat,dashlen);
    if(tile){ raster(ctx)->setTile(tile,tx,ty); }
    if(stipple){ raster(ctx)->setStipple(stipple,tx,ty); }
    else{ raster(ctx)->setStipple(pattern,tx,ty); }
    if(mask){ raster(ctx)->setClipMask(mask,cx,cy); }
    if(font){ raster(ctx)->setGlyphs((FXGlyphs*)font->getGlyphCache()); }

    // A window never draws outside of itself, even though its frame is shared
    raster(ctx)->setClipRectangle(0,0,w,h);
    }
  }


// End drawing
void FXDCWindow::end(){
  if(surface){
    delete raster(ctx);
    ctx=nullptr;
    surface=nullptr;
    }
  }


/*******************************************************************************/

// Read back pixel
FXColor FXDCWindow::readPixel(FXint x,FXint y){
  return ctx?raster(ctx)->readPixel(x,y):0;
  }


// Draw points
void FXDCWindow::drawPoint(FXint x,FXint y){
  if(ctx) raster(ctx)->drawPoint(x,y);
  }

void FXDCWindow::drawPoints(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->drawPoints(points,npoints);
  }

void FXDCWindow::drawPointsRel(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->drawPointsRel(points,npoints);
  }


// Draw lines
void FXDCWindow::drawLine(FXint x1,FXint y1,FXint x2,FXint y2){
  if(ctx) raster(ctx)->drawLine(x1,y1,x2,y2);
  }

void FXDCWindow::drawLines(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->drawLines(points,npoints);
  }

void FXDCWindow::drawLinesRel(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->drawLinesRel(points,npoints);
  }

void FXDCWindow::drawLineSegments(const FXSegment* segments,FXuint nsegments){
  if(ctx) raster(ctx)->drawLineSegments(segments,nsegments);
  }


// Draw rectangles
void FXDCWindow::drawRectangle(FXint x,FXint y,FXint w,FXint h){
  if(ctx) raster(ctx)->drawRectangle(x,y,w,h);
  }

void FXDCWindow::drawRectangles(const FXRectangle* rectangles,FXuint nrectangles){
  if(ctx) raster(ctx)->drawRectangles(rectangles,nrectangles);
  }

void FXDCWindow::drawRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh){
  if(ctx) raster(ctx)->drawRoundRectangle(x,y,w,h,ew,eh);
  }


// Draw arcs
void FXDCWindow::drawArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2){
  if(ctx) raster(ctx)->drawArc(x,y,w,h,ang1,ang2);
  }

void FXDCWindow::drawArcs(const FXArc* arcs,FXuint narcs){
  if(ctx) raster(ctx)->drawArcs(arcs,narcs);
  }

void FXDCWindow::drawEllipse(FXint x,FXint y,FXint w,FXint h){
  if(ctx) raster(ctx)->drawEllipse(x,y,w,h);
  }


// Fill rectangles
void FXDCWindow::fillRectangle(FXint x,FXint y,FXint w,FXint h){
  if(ctx) raster(ctx)->fillRectangle(x,y,w,h);
  }

void FXDCWindow::fillRectangles(const FXRectangle* rectangles,FXuint nrectangles){
  if(ctx) raster(ctx)->fillRectangles(rectangles,nrectangles);
  }

void FXDCWindow::fillRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh){
  if(ctx) raster(ctx)->fillRoundRectangle(x,y,w,h,ew,eh);
  }


// Fill chords
void FXDCWindow::fillChord(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2){
  if(ctx) raster(ctx)->fillChord(x,y,w,h,ang1,ang2);
  }

void FXDCWindow::fillChords(const FXArc* chords,FXuint nchords){
  if(ctx) raster(ctx)->fillChords(chords,nchords);
  }


// Fill arcs
void FXDCWindow::fillArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2){
  if(ctx) raster(ctx)->fillArc(x,y,w,h,ang1,ang2);
  }

void FXDCWindow::fillArcs(const FXArc* arcs,FXuint narcs){
  if(ctx) raster(ctx)->fillArcs(arcs,narcs);
  }

void FXDCWindow::fillEllipse(FXint x,FXint y,FXint w,FXint h){
  if(ctx) raster(ctx)->fillEllipse(x,y,w,h);
  }


// Fill polygons
void FXDCWindow::fillPolygon(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->fillPolygon(points,npoints);
  }

void FXDCWindow::fillConcavePolygon(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->fillConcavePolygon(points,npoints);
  }

void FXDCWindow::fillComplexPolygon(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->fillComplexPolygon(points,npoints);
  }

void FXDCWindow::fillPolygonRel(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->fillPolygonRel(points,npoints);
  }

void FXDCWindow::fillConcavePolygonRel(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->fillConcavePolygonRel(points,npoints);
  }

void FXDCWindow::fillComplexPolygonRel(const FXPoint* points,FXuint npoints){
  if(ctx) raster(ctx)->fillComplexPolygonRel(points,npoints);
  }


// Gradients
void FXDCWindow::fillVerticalGradient(FXint x,FXint y,FXint w,FXint h,FXColor top,FXColor bottom){
  if(ctx) raster(ctx)->fillVerticalGradient(x,y,w,h,top,bottom);
  }

void FXDCWindow::fillHorizontalGradient(FXint x,FXint y,FXint w,FXint h,FXColor left,FXColor right){
  if(ctx) raster(ctx)->fillHorizontalGradient(x,y,w,h,left,right);
  }


// Hash box and focus rectangle
void FXDCWindow::drawHashBox(FXint x,FXint y,FXint w,FXint h,FXint b){
  if(ctx) raster(ctx)->drawHashBox(x,y,w,h,b);
  }

void FXDCWindow::drawFocusRectangle(FXint x,FXint y,FXint w,FXint h){
  if(ctx) raster(ctx)->drawFocusRectangle(x,y,w,h);
  }


/*******************************************************************************/

// Draw area from a source drawable
void FXDCWindow::drawArea(const FXDrawable* source,FXint sx,FXint sy,FXint sw,FXint sh,FXint dx,FXint dy){
  const FXImage* image;
  if(!ctx) return;
  if(!(image=dynamic_cast<const FXImage*>(source))) return;
  raster(ctx)->drawArea(image,sx,sy,sw,sh,dx,dy);
  }


// Draw stretched area from a source drawable
void FXDCWindow::drawArea(const FXDrawable* source,FXint sx,FXint sy,FXint sw,FXint sh,FXint dx,FXint dy,FXint dw,FXint dh){
  const FXImage* image;
  if(!ctx) return;
  if(!(image=dynamic_cast<const FXImage*>(source))) return;
  raster(ctx)->drawArea(image,sx,sy,sw,sh,dx,dy,dw,dh);
  }


// Draw image
void FXDCWindow::drawImage(const FXImage* image,FXint dx,FXint dy){
  if(ctx) raster(ctx)->drawImage(image,dx,dy);
  }


// Draw bitmap
void FXDCWindow::drawBitmap(const FXBitmap* bitmap,FXint dx,FXint dy){
  if(ctx) raster(ctx)->drawBitmap(bitmap,dx,dy);
  }


// Draw icon
void FXDCWindow::drawIcon(const FXIcon* icon,FXint dx,FXint dy){
  if(ctx) raster(ctx)->drawIcon(icon,dx,dy);
  }


// Draw shaded icon; there is no shape mask to stipple through on Wayland
void FXDCWindow::drawIconShaded(const FXIcon* icon,FXint dx,FXint dy){
  if(ctx) raster(ctx)->drawIcon(icon,dx,dy);
  }


// Draw sunken icon; there is no etch mask to draw the bevel from on Wayland
void FXDCWindow::drawIconSunken(const FXIcon* icon,FXint dx,FXint dy){
  if(ctx) raster(ctx)->drawIcon(icon,dx,dy);
  }


/*******************************************************************************/

// Draw string with base line starting at x, y
void FXDCWindow::drawText(FXint x,FXint y,const FXchar* string,FXuint length){
  if(ctx) raster(ctx)->drawText(x,y,string,length);
  }

void FXDCWindow::drawText(FXint x,FXint y,const FXString& string){
  drawText(x,y,string.text(),string.length());
  }


// Draw text over filled background
void FXDCWindow::drawImageText(FXint x,FXint y,const FXchar* string,FXuint length){
  if(ctx) raster(ctx)->drawImageText(x,y,string,length);
  }

void FXDCWindow::drawImageText(FXint x,FXint y,const FXString& string){
  drawImageText(x,y,string.text(),string.length());
  }


/*******************************************************************************/

// Set foreground color
void FXDCWindow::setForeground(FXColor clr){
  fg=clr;
  devfg=surface?surface->visual->getPixel(clr):clr;
  if(ctx) raster(ctx)->setForeground(clr);
  }


// Set background color
void FXDCWindow::setBackground(FXColor clr){
  bg=clr;
  devbg=surface?surface->visual->getPixel(clr):clr;
  if(ctx) raster(ctx)->setBackground(clr);
  }


// Set dash pattern
void FXDCWindow::setDashes(FXuint dashoffset,const FXuchar *dashpattern,FXuint dashlength){
  FXuint i;
  if(dashlength>32) return;
  copyElms(dashpat,dashpattern,dashlength);
  dashlen=dashlength;
  dashoff=dashoffset;
  for(i=dashlength; i<32; i++){ dashpat[i]=0; }
  if(ctx) raster(ctx)->setDashes(dashoff,dashpat,dashlen);
  }


// Set line width
void FXDCWindow::setLineWidth(FXuint linewidth){
  width=linewidth;
  if(ctx) raster(ctx)->setLineWidth(linewidth);
  }


// Set line cap style
void FXDCWindow::setLineCap(FXCapStyle capstyle){
  cap=capstyle;
  if(ctx) raster(ctx)->setLineCap(capstyle);
  }


// Set line join style
void FXDCWindow::setLineJoin(FXJoinStyle joinstyle){
  join=joinstyle;
  if(ctx) raster(ctx)->setLineJoin(joinstyle);
  }


// Set line style
void FXDCWindow::setLineStyle(FXLineStyle linestyle){
  style=linestyle;
  if(ctx) raster(ctx)->setLineStyle(linestyle);
  }


// Set fill style
void FXDCWindow::setFillStyle(FXFillStyle fillstyle){
  fill=fillstyle;
  if(ctx) raster(ctx)->setFillStyle(fillstyle);
  }


// Set fill rule
void FXDCWindow::setFillRule(FXFillRule fillrule){
  rule=fillrule;
  if(ctx) raster(ctx)->setFillRule(fillrule);
  }


// Set blit function
void FXDCWindow::setFunction(FXFunction func){
  rop=func;
  if(ctx) raster(ctx)->setFunction(func);
  }


// Set the tile
void FXDCWindow::setTile(FXImage* image,FXint dx,FXint dy){
  tile=image;
  tx=dx;
  ty=dy;
  if(ctx) raster(ctx)->setTile(image,dx,dy);
  }


// Set the stipple bitmap
void FXDCWindow::setStipple(FXBitmap *bitmap,FXint dx,FXint dy){
  stipple=bitmap;
  pattern=STIPPLE_NONE;
  tx=dx;
  ty=dy;
  if(ctx) raster(ctx)->setStipple(bitmap,dx,dy);
  }


// Set the stipple pattern
void FXDCWindow::setStipple(FXStipplePattern pat,FXint dx,FXint dy){
  stipple=nullptr;
  pattern=pat;
  tx=dx;
  ty=dy;
  if(ctx) raster(ctx)->setStipple(pat,dx,dy);
  }


// Set clip region
void FXDCWindow::setClipRegion(const FXRegion& region){
  if(ctx) raster(ctx)->setClipRegion(region);
  }


// Set clip rectangle
void FXDCWindow::setClipRectangle(FXint x,FXint y,FXint w,FXint h){
  clip=FXRectangle(x,y,w,h);
  if(ctx) raster(ctx)->setClipRectangle(x,y,w,h);
  }

void FXDCWindow::setClipRectangle(const FXRectangle& rectangle){
  setClipRectangle(rectangle.x,rectangle.y,rectangle.w,rectangle.h);
  }


// Clear clipping
void FXDCWindow::clearClipRectangle(){
  clip=FXRectangle(0,0,surface?surface->getWidth():0,surface?surface->getHeight():0);
  if(ctx) raster(ctx)->clearClipRectangle();
  }


// Set clip mask
void FXDCWindow::setClipMask(FXBitmap* bitmap,FXint dx,FXint dy){
  mask=bitmap;
  cx=dx;
  cy=dy;
  if(ctx) raster(ctx)->setClipMask(bitmap,dx,dy);
  }


// Clear clip mask
void FXDCWindow::clearClipMask(){
  mask=nullptr;
  cx=0;
  cy=0;
  if(ctx) raster(ctx)->clearClipMask();
  }


// Set font to draw text with
void FXDCWindow::setFont(FXFont *fnt){
  font=fnt;
  if(ctx) raster(ctx)->setGlyphs(fnt?(FXGlyphs*)fnt->getGlyphCache():nullptr);
  }


// Clip against child windows; the virtual window table does this in phase 4
void FXDCWindow::clipChildren(FXbool yes){
  }


// Update brush; nothing to prepare for the rasterizer
void FXDCWindow::updateBrush(){
  }


// Update pen; nothing to prepare for the rasterizer
void FXDCWindow::updatePen(){
  }

}

#endif
