/********************************************************************************
*                                                                               *
*                          Clipping Region   ( X 1 1 )                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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

#if defined(FOX_X11)

/*
  Notes:
  - Add some more ways to create regions
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct new empty region
FXRegion::FXRegion(){
  region=XCreateRegion();
  }


// Construct rectangle region
FXRegion::FXRegion(FXint x,FXint y,FXint w,FXint h){
  XRectangle r;
  r.x=x; r.y=y; r.width=w; r.height=h;
  region=XCreateRegion();
  XUnionRectWithRegion(&r,(Region)region,(Region)region);
  }


// Construct new region from rectangle rect
FXRegion::FXRegion(const FXRectangle& rect){
  region=XCreateRegion();
  XUnionRectWithRegion(const_cast<XRectangle*>(reinterpret_cast<const XRectangle*>(&rect)),(Region)region,(Region)region);
  }


// Construct polygon region
FXRegion::FXRegion(const FXPoint* points,FXuint npoints,FXbool winding){
  region=XPolygonRegion(const_cast<XPoint*>(reinterpret_cast<const XPoint*>(points)),npoints,winding?WindingRule:EvenOddRule);
  }


// Construct new region copied from region r
FXRegion::FXRegion(const FXRegion& r){
  region=XCreateRegion();
  XUnionRegion((Region)r.region,(Region)region,(Region)region);
  }


// Assign region r to this one
FXRegion& FXRegion::operator=(const FXRegion& r){
  if(region!=r.region){
    XDestroyRegion((Region)region);
    region=XCreateRegion();
    XUnionRegion((Region)r.region,(Region)region,(Region)region);
    }
  return *this;
  }


// Return true if region is empty
FXbool FXRegion::empty() const {
  return XEmptyRegion((Region)region);
  }


// Return true if region contains point
FXbool FXRegion::contains(FXint x,FXint y) const {
  return XPointInRegion((Region)region,x,y);
  }

// Return true if region contains rectangle
// Contributed by Daniel Gehriger <gehriger@linkcad.com>.
FXbool FXRegion::contains(FXint x,FXint y,FXint w,FXint h) const {
  return XRectInRegion((Region)region,x,y,w,h);
  }


// Return bounding box
FXRectangle FXRegion::bounds() const {
  XRectangle rect;
  XClipBox((Region)region,&rect);
  return FXRectangle(rect.x,rect.y,rect.width,rect.height);
  }


// Offset region by dx,dy
FXRegion& FXRegion::offset(FXint dx,FXint dy){
  XOffsetRegion((Region)region,dx,dy);
  return *this;
  }


// Return true if region equal to this one
FXbool FXRegion::operator==(const FXRegion& r) const {
  return XEqualRegion((Region)region,(Region)r.region);
  }


// Return true if region not equal to this one
FXbool FXRegion::operator!=(const FXRegion& r) const {
  return !XEqualRegion((Region)region,(Region)r.region);
  }


// Union region r with this one
FXRegion& FXRegion::operator+=(const FXRegion& r){
  Region res=XCreateRegion();
  XUnionRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
  return *this;
  }


// Intersect region r with this one
FXRegion& FXRegion::operator*=(const FXRegion& r){
  Region res=XCreateRegion();
  XIntersectRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
  return *this;
  }


// Subtract region r from this one
FXRegion& FXRegion::operator-=(const FXRegion& r){
  Region res=XCreateRegion();
  XSubtractRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
  return *this;
  }


// Xor region r with this one
FXRegion& FXRegion::operator^=(const FXRegion& r){
  Region res=XCreateRegion();
  XXorRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
  return *this;
  }


// Union region r with this one
FXRegion FXRegion::operator+(const FXRegion& r) const {
  FXRegion res;
  XUnionRegion((Region)region,(Region)r.region,(Region)res.region);
  return res;
  }


// Intersect region r with this one
FXRegion FXRegion::operator*(const FXRegion& r) const {
  FXRegion res;
  XIntersectRegion((Region)region,(Region)r.region,(Region)res.region);
  return res;
  }


// Subtract region r from this one
FXRegion FXRegion::operator-(const FXRegion& r) const {
  FXRegion res;
  XSubtractRegion((Region)region,(Region)r.region,(Region)res.region);
  return res;
  }


// Xor region r with this one
FXRegion FXRegion::operator^(const FXRegion& r) const {
  FXRegion res;
  XXorRegion((Region)region,(Region)r.region,(Region)res.region);
  return res;
  }


// Reset region to empty
void FXRegion::reset(){
  XDestroyRegion((Region)region);
  region=XCreateRegion();
  }


// Destroy region
FXRegion::~FXRegion(){
  XDestroyRegion((Region)region);
  }

}

#endif
