/********************************************************************************
*                                                                               *
*                       Clipping Region   ( W a y l a n d )                     *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegion.h"
#include "fxrects.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Wayland has no regions, so FXRegion is the portable span list of fxrects.h.
    The same class backs the clip region of the rasterizer and the browser
    frontend, so all three agree on what a region is.
  - The opaque member "region" is a FXRects; every entry point goes through the
    same two casts, so the module stays free of platform types in the header.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// The region behind the opaque handle
static inline FXRects* rects(void* region){
  return (FXRects*)region;
  }

static inline const FXRects* rects(const void* region){
  return (const FXRects*)region;
  }


// Construct new empty region
FXRegion::FXRegion(){
  region=new FXRects();
  }


// Construct rectangle region
FXRegion::FXRegion(FXint x,FXint y,FXint w,FXint h){
  region=new FXRects(x,y,w,h);
  }


// Construct new region from rectangle rect
FXRegion::FXRegion(const FXRectangle& rect){
  region=new FXRects(rect.x,rect.y,rect.w,rect.h);
  }


// Construct polygon region
FXRegion::FXRegion(const FXPoint* points,FXuint npoints,FXbool winding){
  FXRects* r=new FXRects();
  r->setPolygon(points,npoints,winding);
  region=r;
  }


// Construct new region copied from region r
FXRegion::FXRegion(const FXRegion& r){
  region=new FXRects(*rects(r.region));
  }


// Assign region r to this one
FXRegion& FXRegion::operator=(const FXRegion& r){
  if(region!=r.region){
    *rects(region)=*rects(r.region);
    }
  return *this;
  }


// Return true if region is empty
FXbool FXRegion::empty() const {
  return rects(region)->empty();
  }


// Return true if region contains point
FXbool FXRegion::contains(FXint x,FXint y) const {
  return rects(region)->contains(x,y);
  }


// Return true if region contains rectangle
FXbool FXRegion::contains(FXint x,FXint y,FXint w,FXint h) const {
  return rects(region)->contains(x,y,w,h);
  }


// Return bounding box
FXRectangle FXRegion::bounds() const {
  return rects(region)->bounds();
  }


// Offset region by dx,dy
FXRegion& FXRegion::offset(FXint dx,FXint dy){
  rects(region)->offset(dx,dy);
  return *this;
  }


// Return true if region equal to this one
FXbool FXRegion::operator==(const FXRegion& r) const {
  return *rects(region)==*rects(r.region);
  }


// Return true if region not equal to this one
FXbool FXRegion::operator!=(const FXRegion& r) const {
  return *rects(region)!=*rects(r.region);
  }


// Union region r with this one
FXRegion& FXRegion::operator+=(const FXRegion& r){
  rects(region)->unite(*rects(r.region));
  return *this;
  }


// Intersect region r with this one
FXRegion& FXRegion::operator*=(const FXRegion& r){
  rects(region)->intersect(*rects(r.region));
  return *this;
  }


// Subtract region r from this one
FXRegion& FXRegion::operator-=(const FXRegion& r){
  rects(region)->subtract(*rects(r.region));
  return *this;
  }


// Xor region r with this one
FXRegion& FXRegion::operator^=(const FXRegion& r){
  rects(region)->xorbit(*rects(r.region));
  return *this;
  }


// Union of this region and region r
FXRegion FXRegion::operator+(const FXRegion& r) const {
  FXRegion result(*this);
  result+=r;
  return result;
  }


// Intersection of this region and region r
FXRegion FXRegion::operator*(const FXRegion& r) const {
  FXRegion result(*this);
  result*=r;
  return result;
  }


// Subtract region r from this region
FXRegion FXRegion::operator-(const FXRegion& r) const {
  FXRegion result(*this);
  result-=r;
  return result;
  }


// Xor of this region and region r
FXRegion FXRegion::operator^(const FXRegion& r) const {
  FXRegion result(*this);
  result^=r;
  return result;
  }


// Reset region to empty
void FXRegion::reset(){
  rects(region)->clear();
  }


// Destroy region
FXRegion::~FXRegion(){
  delete rects(region);
  }

}

#endif
