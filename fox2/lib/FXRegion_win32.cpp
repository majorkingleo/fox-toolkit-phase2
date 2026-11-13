/********************************************************************************
*                                                                               *
*                        Clipping Region   ( W i n 3 2 )                        *
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

#if defined(WIN32)

/*
  Notes:
  - Add some more ways to create regions
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct new empty region
FXRegion::FXRegion(){
  region=(void*)CreateRectRgn(0,0,0,0);
  }


// Construct rectangle region
FXRegion::FXRegion(FXint x,FXint y,FXint w,FXint h){
  region=(void*)CreateRectRgn(x,y,x+w,y+h);
  }


// Construct new region from rectangle rect
FXRegion::FXRegion(const FXRectangle& rect){
  region=(void*)CreateRectRgn(rect.x,rect.y,rect.x+rect.w,rect.y+rect.h);
  }


// Construct polygon region
FXRegion::FXRegion(const FXPoint* points,FXuint npoints,FXbool winding){
  FXuint i;
  POINT pts[1024];
  for(i=0; i<npoints; i++){
    pts[i].x=points[i].x;
    pts[i].y=points[i].y;
    }
  region=(void*)CreatePolygonRgn(pts,npoints,winding?WINDING:ALTERNATE);
  }


// Construct new region copied from region r
FXRegion::FXRegion(const FXRegion& r){
  region=(void*)CreateRectRgn(0,0,0,0);
  CombineRgn((HRGN)region,(HRGN)r.region,(HRGN)region,RGN_COPY);
  }


// Assign region r to this one
FXRegion& FXRegion::operator=(const FXRegion& r){
  CombineRgn((HRGN)region,(HRGN)r.region,(HRGN)r.region,RGN_COPY);
  return *this;
  }


// Return true if region is empty
FXbool FXRegion::empty() const {
  return OffsetRgn((HRGN)region,0,0)==NULLREGION;
  }


// Return true if region contains point
FXbool FXRegion::contains(FXint x,FXint y) const {
  return region && PtInRegion((HRGN)region,x,y);
  }

// Return true if region contains rectangle
// Contributed by Daniel Gehriger <gehriger@linkcad.com>.
FXbool FXRegion::contains(FXint x,FXint y,FXint w,FXint h) const {
  RECT rect;
  rect.left   = x;
  rect.top    = y;
  rect.right  = x + w;
  rect.bottom = y + h;
  return region && RectInRegion((HRGN)region,&rect);
  }


// Return bounding box
FXRectangle FXRegion::bounds() const {
  RECT rect;
  GetRgnBox((HRGN)region,&rect);
  return FXRectangle((FXshort)rect.left,(FXshort)rect.top,(FXshort)(rect.right-rect.left),(FXshort)(rect.bottom-rect.top));
  }


// Offset region by dx,dy
FXRegion& FXRegion::offset(FXint dx,FXint dy){
  OffsetRgn((HRGN)region,dx,dy);
  return *this;
  }


// Return true if region equal to this one
FXbool FXRegion::operator==(const FXRegion& r) const {
  return EqualRgn((HRGN)region,(HRGN)r.region)!=0;
  }


// Return true if region not equal to this one
FXbool FXRegion::operator!=(const FXRegion& r) const {
  return EqualRgn((HRGN)region,(HRGN)r.region)==0;
  }


// Union region r with this one
FXRegion& FXRegion::operator+=(const FXRegion& r){
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_OR);
  return *this;
  }


// Intersect region r with this one
FXRegion& FXRegion::operator*=(const FXRegion& r){
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_AND);
  return *this;
  }


// Subtract region r from this one
FXRegion& FXRegion::operator-=(const FXRegion& r){
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_DIFF);
  return *this;
  }


// Xor region r with this one
FXRegion& FXRegion::operator^=(const FXRegion& r){
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_XOR);
  return *this;
  }


// Union region r with this one
FXRegion FXRegion::operator+(const FXRegion& r) const {
  FXRegion res;
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_OR);
  return res;
  }


// Intersect region r with this one
FXRegion FXRegion::operator*(const FXRegion& r) const {
  FXRegion res;
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_AND);
  return res;
  }


// Subtract region r from this one
FXRegion FXRegion::operator-(const FXRegion& r) const {
  FXRegion res;
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_DIFF);
  return res;
  }


// Xor region r with this one
FXRegion FXRegion::operator^(const FXRegion& r) const {
  FXRegion res;
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_XOR);
  return res;
  }


// Reset region to empty
void FXRegion::reset(){
  DeleteObject((HRGN)region);
  region=(void*)CreateRectRgn(0,0,0,0);
  }


// Destroy region
FXRegion::~FXRegion(){
  DeleteObject((HRGN)region);
  }

}

#endif
