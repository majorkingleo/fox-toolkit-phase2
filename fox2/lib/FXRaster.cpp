/********************************************************************************
*                                                                               *
*                  S o f t w a r e   R a s t e r i z e r                        *
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
#include "FXArray.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXRegion.h"
#include "FXDC.h"
#include "FXString.h"
#include "fxglyphs.h"
#include "fxstipples.h"
#include "fxraster.h"

/*
  Notes:

  - Design notes stay in fxraster.h; this file holds the rasterizer and the
    X11 compatible rasterization rules of each primitive.
  - Every pixel written is opaque (alpha 255), so the buffer is both a plain
    0xAARRGGBB image and a premultiplied ARGB8888 buffer for wl_shm.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Compose source and destination pixel with the raster function
static inline FXColor fxrasterop(FXFunction func,FXColor src,FXColor dst){
  FXColor res;
  switch(func){
    case BLT_CLR:                   res=0;             break;
    case BLT_SRC_AND_DST:           res=src&dst;       break;
    case BLT_SRC_AND_NOT_DST:       res=src&~dst;      break;
    case BLT_NOT_SRC_AND_DST:       res=~src&dst;      break;
    case BLT_DST:                   res=dst;           break;
    case BLT_SRC_XOR_DST:           res=src^dst;       break;
    case BLT_SRC_OR_DST:            res=src|dst;       break;
    case BLT_NOT_SRC_AND_NOT_DST:   res=~(src|dst);    break;
    case BLT_NOT_SRC_XOR_DST:       res=~(src^dst);    break;
    case BLT_NOT_DST:               res=~dst;          break;
    case BLT_SRC_OR_NOT_DST:        res=src|~dst;      break;
    case BLT_NOT_SRC:               res=~src;          break;
    case BLT_NOT_SRC_OR_DST:        res=~src|dst;      break;
    case BLT_NOT_SRC_OR_NOT_DST:    res=~(src&dst);    break;
    case BLT_SET:                   res=~0;            break;
    default:                        res=src;           break;
    }
  return 0xff000000u|(res&0x00ffffffu);
  }


// Return angle in degrees normalized to [0,360)
static inline FXdouble reduceAngle(FXdouble ang){
  while(ang<0.0){ ang+=360.0; }
  while(ang>=360.0){ ang-=360.0; }
  return ang;
  }


// Return true if angle lies in the arc starting at start and sweeping sweep degrees
static inline FXbool insideArc(FXdouble ang,FXdouble start,FXdouble sweep){
  if(sweep>=360.0) return true;
  return reduceAngle(ang-start)<sweep;
  }


// Reset the dash pattern to the given offset
static void dashBegin(const FXuchar* pattern,FXuint length,FXuint offset,FXuint& index,FXuint& remain,FXbool& on){
  FXuint total=0;
  index=0;
  remain=0;
  on=true;
  for(FXuint i=0; i<length; i++){ total+=pattern[i]; }
  if(length==0 || total==0) return;
  offset%=total;
  FXuint i=0;
  while(offset>=pattern[i]){
    offset-=pattern[i];
    i=(i+1)%length;
    }
  index=i;
  remain=pattern[i]-offset;
  on=((i&1)==0);
  }


// Advance the dash pattern by one pixel
static inline void dashNext(const FXuchar* pattern,FXuint length,FXuint& index,FXuint& remain,FXbool& on){
  if(length==0) return;
  while(remain==0){
    index=(index+1)%length;
    remain=pattern[index];
    on=((index&1)==0);
    }
  remain--;
  if(remain==0){
    index=(index+1)%length;
    remain=pattern[index];
    on=((index&1)==0);
    }
  }


/*******************************************************************************/

// Construct rasterizer without a target
FXRaster::FXRaster(){
  target=nullptr;
  twidth=0;
  theight=0;
  tstride=0;
  clipregion=nullptr;
  maskbits=nullptr;
  maskwidth=0;
  maskheight=0;
  maskbytew=0;
  mx=0;
  my=0;
  tile=nullptr;
  tilepix=nullptr;
  tilewidth=0;
  tileheight=0;
  stipple=nullptr;
  pattern=STIPPLE_NONE;
  stipbits=nullptr;
  stipwidth=0;
  stipheight=0;
  stipbytew=0;
  font=nullptr;
  fg=0xff000000u;
  bg=0xffffffffu;
  linewidth=0;
  cap=CAP_BUTT;
  join=JOIN_MITER;
  linestyle=LINE_SOLID;
  fillstyle=FILL_SOLID;
  fillrule=RULE_EVEN_ODD;
  function=BLT_SRC;
  fillElms(dashpat,4,32);
  dashpat[0]=4;
  dashpat[1]=4;
  dashlen=2;
  dashoff=0;
  tx=0;
  ty=0;
  stx=0;
  sty=0;
  clearClipRectangle();
  }


// Construct rasterizer for a target buffer
FXRaster::FXRaster(FXColor* pixels,FXint width,FXint height,FXint stride):FXRaster(){
  setTarget(pixels,width,height,stride);
  }


// Destroy rasterizer
FXRaster::~FXRaster(){
  clearClipRegion();
  clearClipMask();
  }


/*******************************************************************************/

// Change target buffer
void FXRaster::setTarget(FXColor* pixels,FXint width,FXint height,FXint stride){
  target=pixels;
  twidth=width;
  theight=height;
  tstride=stride>0?stride:width;
  clearClipRectangle();
  }


// Return clip rectangle limited to the target
FXRectangle FXRaster::getClipRectangle() const {
  FXRectangle r;
  r.x=FXMAX(clip.x,0);
  r.y=FXMAX(clip.y,0);
  r.w=FXMIN(clip.x+clip.w,twidth)-r.x;
  r.h=FXMIN(clip.y+clip.h,theight)-r.y;
  if(r.w<0) r.w=0;
  if(r.h<0) r.h=0;
  return r;
  }


// Read back pixel
FXColor FXRaster::readPixel(FXint x,FXint y) const {
  if(!target || x<0 || y<0 || x>=twidth || y>=theight) return 0;
  return target[y*tstride+x];
  }


/*******************************************************************************/

// Return true if pixel is inside clip rectangle, clip mask and clip region
FXbool FXRaster::inClip(FXint x,FXint y) const {
  if(x<clip.x || y<clip.y || x>=clip.x+clip.w || y>=clip.y+clip.h) return false;
  if(maskbits){
    FXint dx=x-mx;
    FXint dy=y-my;
    if(dx<0 || dy<0 || dx>=maskwidth || dy>=maskheight) return false;
    if(!((maskbits[dy*maskbytew+(dx>>3)]>>(dx&7))&1)) return false;
    }
  if(clipregion && !clipregion->contains(x,y)) return false;
  return true;
  }


// Return true if the stipple bit at pixel is set
FXbool FXRaster::stipBit(FXint x,FXint y) const {
  FXint dx,dy;
  if(!stipbits) return false;
  dx=(x-stx)%stipwidth;
  if(dx<0) dx+=stipwidth;
  dy=(y-sty)%stipheight;
  if(dy<0) dy+=stipheight;
  return ((stipbits[dy*stipbytew+(dx>>3)]>>(dx&7))&1)!=0;
  }


// Draw a pixel with the raster function
void FXRaster::plot(FXint x,FXint y,FXColor src){
  if(inClip(x,y)){
    FXColor* p=target+y*tstride+x;
    *p=fxrasterop(function,src,*p);
    }
  }


// Draw a pixel honoring the fill style; stipples may skip the pixel
void FXRaster::plotStyled(FXint x,FXint y){
  FXColor src;
  FXint dx,dy;
  switch(fillstyle){
    case FILL_TILED:
      if(!tilepix) return;
      dx=(x-tx)%tilewidth;
      if(dx<0) dx+=tilewidth;
      dy=(y-ty)%tileheight;
      if(dy<0) dy+=tileheight;
      src=tilepix[dy*tilewidth+dx];
      break;
    case FILL_STIPPLED:
      if(!stipBit(x,y)) return;
      src=fg;
      break;
    case FILL_OPAQUESTIPPLED:
      src=stipBit(x,y)?fg:bg;
      break;
    default:
      src=fg;
      break;
    }
  plot(x,y,src);
  }


// Fill a horizontal span with the current fill style and clip
void FXRaster::span(FXint y,FXint x1,FXint x2){
  FXint x;
  if(y<clip.y || y>=clip.y+clip.h) return;
  for(x=x1; x<=x2; x++){ plotStyled(x,y); }
  }


/*******************************************************************************/

// Draw points
void FXRaster::drawPoint(FXint x,FXint y){
  plot(x,y,fg);
  }


// Draw points
void FXRaster::drawPoints(const FXPoint* points,FXuint npoints){
  FXuint i;
  for(i=0; i<npoints; i++){ plot(points[i].x,points[i].y,fg); }
  }


// Draw points relative
void FXRaster::drawPointsRel(const FXPoint* points,FXuint npoints){
  FXint x,y;
  FXuint i;
  if(npoints<1) return;
  x=points[0].x;
  y=points[0].y;
  plot(x,y,fg);
  for(i=1; i<npoints; i++){
    x+=points[i].x;
    y+=points[i].y;
    plot(x,y,fg);
    }
  }


// Draw a line segment, advancing the dash state
void FXRaster::plotLine(FXint x1,FXint y1,FXint x2,FXint y2,FXuint& dashindex,FXuint& dashremain,FXbool& dashon,FXbool skipfirst){
  FXint dx=FXABS(x2-x1);
  FXint dy=-FXABS(y2-y1);
  FXint sx=x1<x2?1:-1;
  FXint sy=y1<y2?1:-1;
  FXint err=dx+dy;
  FXint e2;
  FXbool on=true;
  for(;;){
    on=(linestyle==LINE_SOLID) || dashon;
    if(on){
      if(!skipfirst) plot(x1,y1,fg);
      }
    else if(linestyle==LINE_DOUBLE_DASH){
      if(!skipfirst) plot(x1,y1,bg);
      }
    skipfirst=false;
    if(x1==x2 && y1==y2) break;
    dashNext(dashpat,dashlen,dashindex,dashremain,dashon);
    e2=err+err;
    if(e2>=dy){ err+=dy; x1+=sx; }
    if(e2<=dx){ err+=dx; y1+=sy; }
    }
  }


// Draw a wide line with the current line width and cap style
void FXRaster::wideLine(FXint x1,FXint y1,FXint x2,FXint y2){
  FXPoint points[4];
  FXdouble dx,dy,len,nx,ny,half;
  FXuint dashindex,dashremain;
  FXbool dashon;
  half=linewidth*0.5;
  dx=x2-x1;
  dy=y2-y1;
  len=sqrt(dx*dx+dy*dy);
  if(len<0.5){
    fillFgRectangle(x1-(FXint)floor(half),y1-(FXint)floor(half),linewidth,linewidth);
    return;
    }
  // Dashed wide lines are stamped along the center line; the stroke is not
  // clipped to the dash pattern the way the X11 server does it
  if(linestyle!=LINE_SOLID){
    FXint x=(FXint)round(x1);
    FXint y=(FXint)round(y1);
    FXint ex=(FXint)round(x2);
    FXint ey=(FXint)round(y2);
    FXint den=FXMAX(FXABS(ex-x),FXABS(ey-y));
    FXint off=linewidth>>1;
    FXint i;
    dashBegin(dashpat,dashlen,dashoff,dashindex,dashremain,dashon);
    for(i=0; i<=den; i++){
      FXint px=(den>0)?(x+(ex-x)*i/den):x;
      FXint py=(den>0)?(y+(ey-y)*i/den):y;
      if(dashon){ fillFgRectangle(px-off,py-off,linewidth,linewidth); }
      dashNext(dashpat,dashlen,dashindex,dashremain,dashon);
      }
    return;
    }
  nx=-dy/len*half;
  ny=dx/len*half;
  // Projecting caps extend the segment by half the line width
  if(cap==CAP_PROJECTING){
    dx=dx/len*half;
    dy=dy/len*half;
    x1=(FXint)round(x1-dx);
    y1=(FXint)round(y1-dy);
    x2=(FXint)round(x2+dx);
    y2=(FXint)round(y2+dy);
    }
  points[0].x=(FXint)round(x1+nx);
  points[0].y=(FXint)round(y1+ny);
  points[1].x=(FXint)round(x2+nx);
  points[1].y=(FXint)round(y2+ny);
  points[2].x=(FXint)round(x2-nx);
  points[2].y=(FXint)round(y2-ny);
  points[3].x=(FXint)round(x1-nx);
  points[3].y=(FXint)round(y1-ny);
  fillFgPolygon(points,4);
  // Round caps close the ends with a half circle
  if(cap==CAP_ROUND){
    FXint e=linewidth>>1;
    fillFgEllipse(x1-e,y1-e,linewidth,linewidth);
    fillFgEllipse(x2-e,y2-e,linewidth,linewidth);
    }
  }


// Draw a line
void FXRaster::drawLine(FXint x1,FXint y1,FXint x2,FXint y2){
  FXuint dashindex;
  FXuint dashremain;
  FXbool dashon;
  if(linewidth>1){
    wideLine(x1,y1,x2,y2);
    return;
    }
  dashBegin(dashpat,dashlen,dashoff,dashindex,dashremain,dashon);
  plotLine(x1,y1,x2,y2,dashindex,dashremain,dashon,false);
  }


// Draw a polyline; dashes continue across the segments and joins are drawn once
void FXRaster::drawLines(const FXPoint* points,FXuint npoints){
  FXuint dashindex,dashremain,i;
  FXbool dashon;
  FXint ex;
  if(npoints<2){
    if(npoints==1) plot(points[0].x,points[0].y,fg);
    return;
    }
  if(linewidth>1){
    for(i=0; i<npoints-1; i++){
      wideLine(points[i].x,points[i].y,points[i+1].x,points[i+1].y);
      }
    for(i=1; i<npoints-1; i++){
      ex=linewidth>>1;
      if(join==JOIN_ROUND){
        fillFgEllipse(points[i].x-ex,points[i].y-ex,linewidth,linewidth);
        }
      else{
        fillFgRectangle(points[i].x-ex,points[i].y-ex,linewidth,linewidth);
        }
      }
    return;
    }
  dashBegin(dashpat,dashlen,dashoff,dashindex,dashremain,dashon);
  for(i=0; i<npoints-1; i++){
    plotLine(points[i].x,points[i].y,points[i+1].x,points[i+1].y,dashindex,dashremain,dashon,i>0);
    }
  }


// Draw a relative polyline
void FXRaster::drawLinesRel(const FXPoint* points,FXuint npoints){
  FXPoint* pts;
  FXint x,y;
  FXuint i;
  if(npoints<1) return;
  if(!allocElms(pts,npoints)) return;
  x=points[0].x;
  y=points[0].y;
  pts[0].x=x;
  pts[0].y=y;
  for(i=1; i<npoints; i++){
    x+=points[i].x;
    y+=points[i].y;
    pts[i].x=x;
    pts[i].y=y;
    }
  drawLines(pts,npoints);
  freeElms(pts);
  }


// Draw independent line segments; the dash pattern restarts for each segment
void FXRaster::drawLineSegments(const FXSegment* segments,FXuint nsegments){
  FXuint i;
  for(i=0; i<nsegments; i++){
    drawLine(segments[i].x1,segments[i].y1,segments[i].x2,segments[i].y2);
    }
  }


/*******************************************************************************/

// Draw a rectangle; as in X11 the outline runs from x to x+w and from y to
// y+h, so it covers one pixel more than the fill of the same rectangle
void FXRaster::drawRectangle(FXint x,FXint y,FXint w,FXint h){
  FXPoint points[5];
  if(w<=0 || h<=0) return;
  points[0].x=x;
  points[0].y=y;
  points[1].x=x+w;
  points[1].y=y;
  points[2].x=x+w;
  points[2].y=y+h;
  points[3].x=x;
  points[3].y=y+h;
  points[4].x=x;
  points[4].y=y;
  drawLines(points,5);
  }


// Draw rectangles
void FXRaster::drawRectangles(const FXRectangle* rectangles,FXuint nrectangles){
  FXuint i;
  for(i=0; i<nrectangles; i++){
    drawRectangle(rectangles[i].x,rectangles[i].y,rectangles[i].w,rectangles[i].h);
    }
  }


// Draw a rounded rectangle
void FXRaster::drawRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh){
  FXArc arcs[4];
  FXSegment segs[4];
  FXCapStyle oldcap=cap;
  if(ew+ew>w) ew=w>>1;
  if(eh+eh>h) eh=h>>1;
  arcs[0].x=arcs[2].x=x;
  arcs[0].y=arcs[1].y=y;
  arcs[1].x=arcs[3].x=x+w-(ew<<1);
  arcs[2].y=arcs[3].y=y+h-(eh<<1);
  arcs[0].w=arcs[1].w=arcs[2].w=arcs[3].w=ew<<1;
  arcs[0].h=arcs[1].h=arcs[2].h=arcs[3].h=eh<<1;
  arcs[0].a=5760;
  arcs[0].b=5760;
  arcs[1].a=0;
  arcs[1].b=5760;
  arcs[2].a=11520;
  arcs[2].b=5760;
  arcs[3].a=17280;
  arcs[3].b=5760;
  segs[0].x1=segs[1].x1=x+ew;
  segs[0].x2=segs[1].x2=x+w-ew;
  segs[0].y1=segs[0].y2=y;
  segs[1].y1=segs[1].y2=y+h;
  segs[2].x1=segs[2].x2=x;
  segs[3].x1=segs[3].x2=x+w;
  segs[2].y1=segs[3].y1=y+eh;
  segs[2].y2=segs[3].y2=y+h-eh;
  cap=CAP_BUTT;
  drawArcs(arcs,4);
  drawLineSegments(segs,4);
  cap=oldcap;
  }


/*******************************************************************************/

// Draw an arc; the outline follows the analytic ellipse, sampled as a polyline
// so that dashes and wide lines behave as they do for drawLines()
void FXRaster::drawArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2){
  FXPoint* points;
  FXdouble rx,ry,cx,cy,a1,a2,ang;
  FXint steps,i;
  if(w<=0 || h<=0) return;
  if(ang2==0) return;
  rx=w*0.5;
  ry=h*0.5;
  cx=x+rx;
  cy=y+ry;
  a1=ang1/64.0;
  a2=ang2/64.0;
  if(a2<0.0) a2=-a2;
  if(a2>360.0) a2=360.0;
  steps=(FXint)ceil(FXMAX(rx,ry)*a2*DTOR*2.0);
  if(steps<1) steps=1;
  if(steps>5760) steps=5760;
  if(!allocElms(points,steps+1)) return;
  for(i=0; i<=steps; i++){
    ang=(a1+a2*i/steps)*DTOR;
    points[i].x=(FXint)floor(cx+(rx-0.5)*cos(ang));
    points[i].y=(FXint)floor(cy-(ry-0.5)*sin(ang));
    }
  drawLines(points,steps+1);
  freeElms(points);
  }


// Draw arcs
void FXRaster::drawArcs(const FXArc* arcs,FXuint narcs){
  FXuint i;
  for(i=0; i<narcs; i++){
    drawArc(arcs[i].x,arcs[i].y,arcs[i].w,arcs[i].h,arcs[i].a,arcs[i].b);
    }
  }


// Draw an ellipse
void FXRaster::drawEllipse(FXint x,FXint y,FXint w,FXint h){
  drawArc(x,y,w,h,0,23040);
  }


/*******************************************************************************/

// Fill a rectangle with the current fill style
void FXRaster::fillRectangle(FXint x,FXint y,FXint w,FXint h){
  FXint x1,x2,y1,y2,xx,yy;
  if(w<=0 || h<=0) return;
  x1=FXMAX(x,clip.x);
  x2=FXMIN(x+w,clip.x+clip.w);
  y1=FXMAX(y,clip.y);
  y2=FXMIN(y+h,clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    for(xx=x1; xx<x2; xx++){
      plotStyled(xx,yy);
      }
    }
  }


// Fill rectangles
void FXRaster::fillRectangles(const FXRectangle* rectangles,FXuint nrectangles){
  FXuint i;
  for(i=0; i<nrectangles; i++){
    fillRectangle(rectangles[i].x,rectangles[i].y,rectangles[i].w,rectangles[i].h);
    }
  }


// Fill a rounded rectangle; as in X11 this is four quarter arcs and three rectangles
void FXRaster::fillRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh){
  FXArc arcs[4];
  FXRectangle recs[3];
  if(ew+ew>w) ew=w>>1;
  if(eh+eh>h) eh=h>>1;
  arcs[0].x=arcs[2].x=x;
  arcs[0].y=arcs[1].y=y;
  arcs[1].x=arcs[3].x=x+w-(ew<<1);
  arcs[2].y=arcs[3].y=y+h-(eh<<1);
  arcs[0].w=arcs[1].w=arcs[2].w=arcs[3].w=ew<<1;
  arcs[0].h=arcs[1].h=arcs[2].h=arcs[3].h=eh<<1;
  arcs[0].a=5760;
  arcs[0].b=5760;
  arcs[1].a=0;
  arcs[1].b=5760;
  arcs[2].a=11520;
  arcs[2].b=5760;
  arcs[3].a=17280;
  arcs[3].b=5760;
  recs[0].x=recs[2].x=x+ew;
  recs[0].w=recs[2].w=w-(ew<<1);
  recs[0].h=recs[2].h=eh;
  recs[0].y=y;
  recs[2].y=y+h-eh;
  recs[1].x=x;
  recs[1].y=y+eh;
  recs[1].w=w;
  recs[1].h=h-(eh<<1);
  fillArcs(arcs,4);
  fillRectangles(recs,3);
  }


// Fill an arc or chord of an ellipse
void FXRaster::fillArcSpan(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2,FXbool chord){
  FXdouble rx,ry,cx,cy,a1,a2,ang,t,dx,cdx,cdy,side,dist;
  FXint x1,x2,y1,y2,yy,xx;
  if(w<=0 || h<=0) return;
  if(ang2==0) return;
  rx=w*0.5;
  ry=h*0.5;
  cx=x+rx;
  cy=y+ry;
  a1=ang1/64.0;
  a2=ang2/64.0;
  if(a2<0.0){ a1+=a2; a2=-a2; }
  if(a2>360.0) a2=360.0;
  // Chord line, and the side of that line the arc lies on
  cdx=rx*cos((a1+a2)*DTOR)-rx*cos(a1*DTOR);
  cdy=-ry*sin((a1+a2)*DTOR)+ry*sin(a1*DTOR);
  ang=(a1+a2*0.5)*DTOR;
  dist=cdx*(rx*cos(ang)-rx*cos(a1*DTOR))+cdy*(-ry*sin(ang)+ry*sin(a1*DTOR));
  side=(dist<0.0)?-1.0:1.0;
  y1=FXMAX(y,clip.y);
  y2=FXMIN(y+h,clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    t=(yy+0.5-cy)/ry;
    if(t<-1.0 || t>1.0) continue;
    dx=rx*sqrt(1.0-t*t);
    x1=FXMAX((FXint)ceil(cx-dx-0.5),clip.x);
    x2=FXMIN((FXint)ceil(cx+dx-0.5)-1,clip.x+clip.w-1);
    for(xx=x1; xx<=x2; xx++){
      FXdouble px=xx+0.5-cx;
      FXdouble py=yy+0.5-cy;
      ang=reduceAngle(atan2(-py,px)*RTOD);
      if(!insideArc(ang,a1,a2)) continue;
      if(chord && ((cdx*py-cdy*px)<0.0?-1.0:1.0)!=side) continue;
      plotStyled(xx,yy);
      }
    }
  }


// Fill a chord
void FXRaster::fillChord(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2){
  fillArcSpan(x,y,w,h,ang1,ang2,true);
  }


// Fill chords
void FXRaster::fillChords(const FXArc* chords,FXuint nchords){
  FXuint i;
  for(i=0; i<nchords; i++){
    fillChord(chords[i].x,chords[i].y,chords[i].w,chords[i].h,chords[i].a,chords[i].b);
    }
  }


// Fill an arc
void FXRaster::fillArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2){
  fillArcSpan(x,y,w,h,ang1,ang2,false);
  }


// Fill arcs
void FXRaster::fillArcs(const FXArc* arcs,FXuint narcs){
  FXuint i;
  for(i=0; i<narcs; i++){
    fillArc(arcs[i].x,arcs[i].y,arcs[i].w,arcs[i].h,arcs[i].a,arcs[i].b);
    }
  }


// Fill an ellipse with spans
void FXRaster::fillEllipse(FXint x,FXint y,FXint w,FXint h){
  FXdouble rx,ry,cx,cy,t,dx;
  FXint x1,x2,y1,y2,yy;
  if(w<=0 || h<=0) return;
  rx=w*0.5;
  ry=h*0.5;
  cx=x+rx;
  cy=y+ry;
  y1=FXMAX(y,clip.y);
  y2=FXMIN(y+h,clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    t=(yy+0.5-cy)/ry;
    if(t<-1.0 || t>1.0) continue;
    dx=rx*sqrt(1.0-t*t);
    x1=(FXint)ceil(cx-dx-0.5);
    x2=(FXint)ceil(cx+dx-0.5)-1;
    if(x1>x2) continue;
    span(yy,x1,x2);
    }
  }


// Fill a polygon with the current fill rule; the scan line is crossed at the
// pixel centers, which is the rule the X11 server uses for polygon fills
void FXRaster::fillPolyspan(const FXPoint* points,FXuint npoints,FXbool relative){
  FXPoint* pts;
  FXdouble* cross;
  FXint* dir;
  FXuint i,j,cnt;
  FXint y,ymin,ymax,x1,x2;
  FXint wind,previous;
  FXdouble yc;
  if(npoints<3) return;
  if(!allocElms(pts,npoints)) return;
  if(!allocElms(cross,npoints)){ freeElms(pts); return; }
  if(!allocElms(dir,npoints)){ freeElms(cross); freeElms(pts); return; }
  if(relative){
    pts[0]=points[0];
    for(i=1; i<npoints; i++){
      pts[i].x=pts[i-1].x+points[i].x;
      pts[i].y=pts[i-1].y+points[i].y;
      }
    }
  else{
    for(i=0; i<npoints; i++){ pts[i]=points[i]; }
    }
  ymin=ymax=pts[0].y;
  for(i=1; i<npoints; i++){
    if(pts[i].y<ymin) ymin=pts[i].y;
    if(pts[i].y>ymax) ymax=pts[i].y;
    }
  for(y=ymin; y<ymax; y++){
    yc=y+0.5;
    cnt=0;
    for(i=0; i<npoints; i++){
      const FXPoint& p1=pts[i];
      const FXPoint& p2=pts[(i+1)%npoints];
      if((p1.y<=yc && p2.y>yc) || (p2.y<=yc && p1.y>yc)){
        cross[cnt]=p1.x+(yc-p1.y)*(FXdouble)(p2.x-p1.x)/(p2.y-p1.y);
        dir[cnt]=(p2.y>p1.y)?1:-1;
        cnt++;
        }
      }
    if(cnt<2) continue;
    // Sort the crossings, carrying the edge direction with them
    for(i=0; i+1<cnt; i++){
      for(j=i+1; j<cnt; j++){
        if(cross[j]<cross[i]){
          FXdouble s=cross[i]; cross[i]=cross[j]; cross[j]=s;
          FXint d=dir[i]; dir[i]=dir[j]; dir[j]=d;
          }
        }
      }
    if(fillrule==RULE_WINDING){
      wind=0;
      x1=0;
      for(i=0; i<cnt; i++){
        previous=wind;
        wind+=dir[i];
        if(previous==0 && wind!=0){
          x1=(FXint)ceil(cross[i]-0.5);
          }
        else if(previous!=0 && wind==0){
          x2=(FXint)ceil(cross[i]-0.5)-1;
          if(x1<=x2) span(y,x1,x2);
          }
        }
      }
    else{
      for(i=0; i+1<cnt; i+=2){
        x1=(FXint)ceil(cross[i]-0.5);
        x2=(FXint)ceil(cross[i+1]-0.5)-1;
        if(x1<=x2) span(y,x1,x2);
        }
      }
    }
  freeElms(dir);
  freeElms(cross);
  freeElms(pts);
  }


// Fill a polygon
void FXRaster::fillPolygon(const FXPoint* points,FXuint npoints){
  fillPolyspan(points,npoints,false);
  }


// Fill a concave polygon
void FXRaster::fillConcavePolygon(const FXPoint* points,FXuint npoints){
  fillPolyspan(points,npoints,false);
  }


// Fill a complex polygon
void FXRaster::fillComplexPolygon(const FXPoint* points,FXuint npoints){
  fillPolyspan(points,npoints,false);
  }


// Fill a relative polygon
void FXRaster::fillPolygonRel(const FXPoint* points,FXuint npoints){
  fillPolyspan(points,npoints,true);
  }


// Fill a relative concave polygon
void FXRaster::fillConcavePolygonRel(const FXPoint* points,FXuint npoints){
  fillPolyspan(points,npoints,true);
  }


// Fill a relative complex polygon
void FXRaster::fillComplexPolygonRel(const FXPoint* points,FXuint npoints){
  fillPolyspan(points,npoints,true);
  }


/*******************************************************************************/

// Fill a rectangle with the foreground color, ignoring the fill style
void FXRaster::fillFgRectangle(FXint x,FXint y,FXint w,FXint h){
  FXint x1,x2,y1,y2,xx,yy;
  if(w<=0 || h<=0) return;
  x1=FXMAX(x,clip.x);
  x2=FXMIN(x+w,clip.x+clip.w);
  y1=FXMAX(y,clip.y);
  y2=FXMIN(y+h,clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    for(xx=x1; xx<x2; xx++){
      plot(xx,yy,fg);
      }
    }
  }


// Fill an ellipse with the foreground color, ignoring the fill style
void FXRaster::fillFgEllipse(FXint x,FXint y,FXint w,FXint h){
  FXdouble rx,ry,cx,cy,t,dx;
  FXint x1,x2,y1,y2,yy,xx;
  if(w<=0 || h<=0) return;
  rx=w*0.5;
  ry=h*0.5;
  cx=x+rx;
  cy=y+ry;
  y1=FXMAX(y,clip.y);
  y2=FXMIN(y+h,clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    t=(yy+0.5-cy)/ry;
    if(t<-1.0 || t>1.0) continue;
    dx=rx*sqrt(1.0-t*t);
    x1=(FXint)ceil(cx-dx-0.5);
    x2=(FXint)ceil(cx+dx-0.5)-1;
    for(xx=x1; xx<=x2; xx++){ plot(xx,yy,fg); }
    }
  }


// Fill a polygon with the foreground color, ignoring the fill style
void FXRaster::fillFgPolygon(const FXPoint* points,FXuint npoints){
  FXFillStyle oldfill=fillstyle;
  fillstyle=FILL_SOLID;
  fillPolyspan(points,npoints,false);
  fillstyle=oldfill;
  }


/*******************************************************************************/

// Fill vertical gradient rectangle
void FXRaster::fillVerticalGradient(FXint x,FXint y,FXint w,FXint h,FXColor top,FXColor bottom){
  FXint rr,gg,bb,dr,dg,db,r1,g1,b1,r2,g2,b2,yl,yh,yy,dy,n,t;
  FXColor oldfg=fg;
  if(0<w && 0<h){
    r1=FXREDVAL(top);
    r2=FXREDVAL(bottom);
    g1=FXGREENVAL(top);
    g2=FXGREENVAL(bottom);
    b1=FXBLUEVAL(top);
    b2=FXBLUEVAL(bottom);
    dr=r2-r1;
    dg=g2-g1;
    db=b2-b1;
    n=FXABS(dr);
    if((t=FXABS(dg))>n) n=t;
    if((t=FXABS(db))>n) n=t;
    n++;
    if(n>h) n=h;
    if(n>128) n=128;
    rr=(r1<<16)+32767;
    gg=(g1<<16)+32767;
    bb=(b1<<16)+32767;
    yy=32767;
    dr=(dr<<16)/n;
    dg=(dg<<16)/n;
    db=(db<<16)/n;
    dy=(h<<16)/n;
    do{
      yl=yy>>16;
      yy+=dy;
      yh=yy>>16;
      setForeground(FXRGB(rr>>16,gg>>16,bb>>16));
      fillRectangle(x,y+yl,w,yh-yl);
      rr+=dr;
      gg+=dg;
      bb+=db;
      }
    while(yh<h);
    }
  setForeground(oldfg);
  }


// Fill horizontal gradient rectangle
void FXRaster::fillHorizontalGradient(FXint x,FXint y,FXint w,FXint h,FXColor left,FXColor right){
  FXint rr,gg,bb,dr,dg,db,r1,g1,b1,r2,g2,b2,xl,xh,xx,dx,n,t;
  FXColor oldfg=fg;
  if(0<w && 0<h){
    r1=FXREDVAL(left);
    r2=FXREDVAL(right);
    g1=FXGREENVAL(left);
    g2=FXGREENVAL(right);
    b1=FXBLUEVAL(left);
    b2=FXBLUEVAL(right);
    dr=r2-r1;
    dg=g2-g1;
    db=b2-b1;
    n=FXABS(dr);
    if((t=FXABS(dg))>n) n=t;
    if((t=FXABS(db))>n) n=t;
    n++;
    if(n>w) n=w;
    if(n>128) n=128;
    rr=(r1<<16)+32767;
    gg=(g1<<16)+32767;
    bb=(b1<<16)+32767;
    xx=32767;
    dr=(dr<<16)/n;
    dg=(dg<<16)/n;
    db=(db<<16)/n;
    dx=(w<<16)/n;
    do{
      xl=xx>>16;
      xx+=dx;
      xh=xx>>16;
      setForeground(FXRGB(rr>>16,gg>>16,bb>>16));
      fillRectangle(x+xl,y,xh-xl,h);
      rr+=dr;
      gg+=dg;
      bb+=db;
      }
    while(xh<w);
    }
  setForeground(oldfg);
  }


// Draw hashed box
void FXRaster::drawHashBox(FXint x,FXint y,FXint w,FXint h,FXint b){
  FXFillStyle oldfill=fillstyle;
  const FXBitmap* oldstip=stipple;
  FXStipplePattern oldpattern=pattern;
  const FXuchar* oldbits=stipbits;
  FXint oldwidth=stipwidth;
  FXint oldheight=stipheight;
  FXint oldbytew=stipbytew;
  FXint oldsx=stx;
  FXint oldsy=sty;
  fillstyle=FILL_STIPPLED;
  setStipple(STIPPLE_GRAY);
  fillRectangle(x,y,w-b,b);
  fillRectangle(x+w-b,y,b,h-b);
  fillRectangle(x+b,y+h-b,w-b,b);
  fillRectangle(x,y+b,b,h-b);
  fillstyle=oldfill;
  stipple=oldstip;
  pattern=oldpattern;
  stipbits=oldbits;
  stipwidth=oldwidth;
  stipheight=oldheight;
  stipbytew=oldbytew;
  stx=oldsx;
  sty=oldsy;
  }


// Draw focus rectangle
void FXRaster::drawFocusRectangle(FXint x,FXint y,FXint w,FXint h){
  FXFillStyle oldfill=fillstyle;
  const FXBitmap* oldstip=stipple;
  FXStipplePattern oldpattern=pattern;
  const FXuchar* oldbits=stipbits;
  FXint oldwidth=stipwidth;
  FXint oldheight=stipheight;
  FXint oldbytew=stipbytew;
  FXFunction oldfunc=function;
  FXColor oldfg=fg;
  FXint oldsx=stx;
  FXint oldsy=sty;
  fillstyle=FILL_STIPPLED;
  function=BLT_SRC_XOR_DST;
  fg=0xffffffffu;
  setStipple(STIPPLE_GRAY,x,y);
  fillRectangle(x,y,w-1,1);
  fillRectangle(x+w-1,y,1,h-1);
  fillRectangle(x+1,y+h-1,w-1,1);
  fillRectangle(x,y+1,1,h-1);
  fillstyle=oldfill;
  function=oldfunc;
  fg=oldfg;
  stipple=oldstip;
  pattern=oldpattern;
  stipbits=oldbits;
  stipwidth=oldwidth;
  stipheight=oldheight;
  stipbytew=oldbytew;
  stx=oldsx;
  sty=oldsy;
  }


/*******************************************************************************/

// Blend a glyph coverage value over the target pixel
void FXRaster::blend(FXint x,FXint y,FXuint cov){
  FXColor* p;
  FXColor d;
  FXuint inv;
  if(!inClip(x,y) || cov==0) return;
  p=target+y*tstride+x;
  if(cov>=255){
    *p=fg|0xff000000u;
    return;
    }
  d=*p;
  inv=255-cov;
  *p=0xff000000u|FXRGB((FXREDVAL(fg)*cov+FXREDVAL(d)*inv+127)/255,
                       (FXGREENVAL(fg)*cov+FXGREENVAL(d)*inv+127)/255,
                       (FXBLUEVAL(fg)*cov+FXBLUEVAL(d)*inv+127)/255);
  }


#if defined(FOX_GLYPHS_AVAILABLE)

// Draw UTF-8 text with the baseline at x,y
void FXRaster::drawText(FXint x,FXint y,const FXchar* text,FXint len){
  const FXGlyph* glyph;
  FXint p=0,xx,yy;
  if(!font || !text) return;
  if(len<0){ len=0; while(text[len]) len++; }
  while(p<len){
    glyph=font->getGlyph(fxutf8decode(text,len,p));
    if(!glyph) break;
    if(glyph->coverage){
      for(yy=0; yy<glyph->height; yy++){
        for(xx=0; xx<glyph->width; xx++){
          blend(x+glyph->bearing+xx,y-glyph->top+yy,glyph->coverage[yy*glyph->width+xx]);
          }
        }
      }
    x+=glyph->advance;
    }
  }


// Draw UTF-8 text over a filled background, as XftDrawRect plus the string
void FXRaster::drawImageText(FXint x,FXint y,const FXchar* text,FXint len){
  FXColor oldfg;
  FXint width;
  if(!font || !text) return;
  if(len<0){ len=0; while(text[len]) len++; }
  width=font->getTextWidth(text,len);
  oldfg=fg;
  fg=bg;
  fillFgRectangle(x,y-font->getAscent(),width,font->getHeight());
  fg=oldfg;
  drawText(x,y,text,len);
  }

#else

// Draw UTF-8 text with the baseline at x,y
void FXRaster::drawText(FXint,FXint,const FXchar*,FXint){
  }


// Draw UTF-8 text over a filled background
void FXRaster::drawImageText(FXint,FXint,const FXchar*,FXint){
  }

#endif


/*******************************************************************************/

// Draw area from image
void FXRaster::drawArea(const FXImage* source,FXint sx,FXint sy,FXint sw,FXint sh,FXint dx,FXint dy){
  const FXColor* src;
  FXint x1,x2,y1,y2,xx,yy,iw,ih;
  if(!source || !(src=source->getData())) return;
  iw=source->getWidth();
  ih=source->getHeight();
  x1=FXMAX(dx,clip.x);
  x2=FXMIN(dx+sw,clip.x+clip.w);
  y1=FXMAX(dy,clip.y);
  y2=FXMIN(dy+sh,clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    for(xx=x1; xx<x2; xx++){
      FXint ix=sx+xx-dx;
      FXint iy=sy+yy-dy;
      if(ix<0 || iy<0 || ix>=iw || iy>=ih) continue;
      plot(xx,yy,src[iy*iw+ix]);
      }
    }
  }


// Draw stretched area from image
void FXRaster::drawArea(const FXImage* source,FXint sx,FXint sy,FXint sw,FXint sh,FXint dx,FXint dy,FXint dw,FXint dh){
  const FXColor* src;
  FXint i,j,x,y,xs,ys,ix,iy,iw,ih;
  if(!source || !(src=source->getData())) return;
  if(dw<=0 || dh<=0) return;
  iw=source->getWidth();
  ih=source->getHeight();
  xs=(sw<<16)/dw;
  ys=(sh<<16)/dh;
  i=0;
  y=ys>>1;
  do{
    j=0;
    x=xs>>1;
    do{
      ix=sx+(x>>16);
      iy=sy+(y>>16);
      if(ix>=0 && iy>=0 && ix<iw && iy<ih){
        plot(dx+j,dy+i,src[iy*iw+ix]);
        }
      x+=xs;
      }
    while(++j<dw);
    y+=ys;
    }
  while(++i<dh);
  }


// Draw image
void FXRaster::drawImage(const FXImage* image,FXint dx,FXint dy){
  if(!image) return;
  drawArea(image,0,0,image->getWidth(),image->getHeight(),dx,dy);
  }


// Draw bitmap; as XCopyPlane, a zero bit writes the background color
void FXRaster::drawBitmap(const FXBitmap* bitmap,FXint dx,FXint dy){
  const FXuchar* src;
  FXint x1,x2,y1,y2,xx,yy,bw,bytew;
  if(!bitmap || !(src=bitmap->getData())) return;
  bw=bitmap->getWidth();
  bytew=(bw+7)>>3;
  x1=FXMAX(dx,clip.x);
  x2=FXMIN(dx+bw,clip.x+clip.w);
  y1=FXMAX(dy,clip.y);
  y2=FXMIN(dy+bitmap->getHeight(),clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    for(xx=x1; xx<x2; xx++){
      FXint ix=xx-dx;
      FXint iy=yy-dy;
      FXColor srcpixel=((src[iy*bytew+(ix>>3)]>>(ix&7))&1)?fg:bg;
      plot(xx,yy,srcpixel);
      }
    }
  }


// Draw icon; the alpha channel or alpha color acts as a shape mask
void FXRaster::drawIcon(const FXIcon* icon,FXint dx,FXint dy){
  const FXColor* src;
  FXuint options;
  FXColor transp;
  FXbool alphacolor;
  FXint x1,x2,y1,y2,xx,yy,iw,ih;
  if(!icon || !(src=icon->getData())) return;
  options=icon->getOptions();
  alphacolor=(options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS))!=0;
  transp=icon->getTransparentColor();
  iw=icon->getWidth();
  ih=icon->getHeight();
  x1=FXMAX(dx,clip.x);
  x2=FXMIN(dx+iw,clip.x+clip.w);
  y1=FXMAX(dy,clip.y);
  y2=FXMIN(dy+ih,clip.y+clip.h);
  for(yy=y1; yy<y2; yy++){
    for(xx=x1; xx<x2; xx++){
      FXColor pixel=src[(yy-dy)*iw+(xx-dx)];
      if(!(options&IMAGE_OPAQUE)){
        if(alphacolor ? (pixel==transp) : (FXALPHAVAL(pixel)==0)) continue;
        }
      plot(xx,yy,pixel);
      }
    }
  }


/*******************************************************************************/

// Set the glyph cache used by the text calls
void FXRaster::setGlyphs(FXGlyphs* glyphs){
  font=glyphs;
  }


// Get the glyph cache
FXGlyphs* FXRaster::getGlyphs() const {
  return font;
  }


// Set foreground color
void FXRaster::setForeground(FXColor clr){
  fg=clr|0xff000000u;
  }
// Set background color
void FXRaster::setBackground(FXColor clr){
  bg=clr|0xff000000u;
  }


// Set dashes
void FXRaster::setDashes(FXuint dashoffset,const FXuchar* dashpattern,FXuint dashlength){
  FXuint len=0;
  if(dashlength>32) return;
  for(FXuint i=0; i<dashlength; i++){
    dashpat[i]=dashpattern[i];
    len+=dashpattern[i];
    }
  dashlen=dashlength;
  dashoff=len?dashoffset%len:0;
  }


// Set line width
void FXRaster::setLineWidth(FXuint linewidth){
  this->linewidth=linewidth;
  }


// Set line cap style
void FXRaster::setLineCap(FXCapStyle capstyle){
  cap=capstyle;
  }


// Set line join style
void FXRaster::setLineJoin(FXJoinStyle joinstyle){
  join=joinstyle;
  }


// Set line style
void FXRaster::setLineStyle(FXLineStyle linestyle){
  this->linestyle=linestyle;
  }


// Set fill style
void FXRaster::setFillStyle(FXFillStyle fillstyle){
  this->fillstyle=fillstyle;
  }


// Set fill rule
void FXRaster::setFillRule(FXFillRule fillrule){
  this->fillrule=fillrule;
  }


// Set raster function
void FXRaster::setFunction(FXFunction func){
  function=func;
  }


// Set tile image
void FXRaster::setTile(const FXImage* image,FXint dx,FXint dy){
  tile=image;
  tilepix=image?image->getData():nullptr;
  tilewidth=image?image->getWidth():0;
  tileheight=image?image->getHeight():0;
  tx=dx;
  ty=dy;
  }


// Get tile image
const FXImage* FXRaster::getTile() const {
  return tile;
  }


// Set stipple bitmap
void FXRaster::setStipple(const FXBitmap* bitmap,FXint dx,FXint dy){
  stipple=bitmap;
  pattern=STIPPLE_NONE;
  stipbits=bitmap?bitmap->getData():nullptr;
  stipwidth=bitmap?bitmap->getWidth():0;
  stipheight=bitmap?bitmap->getHeight():0;
  stipbytew=bitmap?(bitmap->getWidth()+7)>>3:0;
  stx=dx;
  sty=dy;
  }


// Get stipple bitmap
const FXBitmap* FXRaster::getStippleBitmap() const {
  return stipple;
  }


// Set stipple pattern
void FXRaster::setStipple(FXStipplePattern pat,FXint dx,FXint dy){
  const FXStippleData& data=stipple_data[FXMIN((FXuint)pat,(FXuint)STIPPLE_CROSSDIAG)];
  stipple=nullptr;
  pattern=pat;
  stipbits=data.bits;
  stipwidth=data.width;
  stipheight=data.height;
  stipbytew=data.bytewidth;
  stx=dx;
  sty=dy;
  }


// Get stipple pattern
FXStipplePattern FXRaster::getStipplePattern() const {
  return pattern;
  }


// Set clip region; the clip rectangle still applies
void FXRaster::setClipRegion(const FXRegion& region){
  if(!clipregion){ clipregion=new FXRegion(); }
  *clipregion=region;
  }


// Clear clip region
void FXRaster::clearClipRegion(){
  delete clipregion;
  clipregion=nullptr;
  }


// Set clip rectangle
void FXRaster::setClipRectangle(FXint x,FXint y,FXint w,FXint h){
  clip.x=x;
  clip.y=y;
  clip.w=FXMAX(w,0);
  clip.h=FXMAX(h,0);
  }


// Set clip rectangle
void FXRaster::setClipRectangle(const FXRectangle& rectangle){
  setClipRectangle(rectangle.x,rectangle.y,rectangle.w,rectangle.h);
  }


// Clear clipping; this drops a clip region and a clip mask as well, which is
// what setting the clip rectangles on the X11 GC does
void FXRaster::clearClipRectangle(){
  clearClipRegion();
  clearClipMask();
  clip.x=0;
  clip.y=0;
  clip.w=twidth;
  clip.h=theight;
  }


// Set clip mask
void FXRaster::setClipMask(const FXBitmap* bitmap,FXint dx,FXint dy){
  maskbits=bitmap?bitmap->getData():nullptr;
  maskwidth=bitmap?bitmap->getWidth():0;
  maskheight=bitmap?bitmap->getHeight():0;
  maskbytew=bitmap?(bitmap->getWidth()+7)>>3:0;
  mx=dx;
  my=dy;
  }


// Clear clip mask
void FXRaster::clearClipMask(){
  maskbits=nullptr;
  maskwidth=0;
  maskheight=0;
  maskbytew=0;
  mx=0;
  my=0;
  }

}
