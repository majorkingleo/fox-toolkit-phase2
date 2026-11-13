/********************************************************************************
*                                                                               *
*                     R e c t a n g l e   R e g i o n                          *
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
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "fxrects.h"

/*
  Notes:

  - Design notes stay in fxrects.h; this file holds the span algebra.
  - Every operation produces the canonical form directly: spans sorted by scan
    line and start, disjoint and non-touching, so two equal pixel sets always
    have equal lists and the lists can be compared without normalizing.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Append a span to a span list, growing the list when it is full
static FXbool spanAdd(FXSpan*& list,FXuint& count,FXuint& alloced,const FXSpan& span){
  if(count==alloced){
    FXuint cap=alloced?alloced*2:64;
    if(!resizeElms(list,cap)) return false;
    alloced=cap;
    }
  list[count++]=span;
  return true;
  }


// Append a span that follows the previous one, merging touching spans
static FXbool spanAddMerged(FXSpan*& list,FXuint& count,FXuint& alloced,const FXSpan& span){
  if(count && list[count-1].y==span.y && span.x1<=(list[count-1].x2+1)){
    if(span.x2>list[count-1].x2) list[count-1].x2=span.x2;
    return true;
    }
  return spanAdd(list,count,alloced,span);
  }


// Return true if span a sorts before span b
static inline FXbool spanBefore(const FXSpan& a,const FXSpan& b){
  return (a.y<b.y) || (a.y==b.y && a.x1<b.x1);
  }


// Return true if the list covers pixel x on scan line y
static FXbool spanCovers(const FXSpan* list,FXuint count,FXint y,FXint x){
  FXuint lo=0;
  FXuint hi=count;
  while(lo<hi){
    FXuint mid=(lo+hi)>>1;
    if(list[mid].y<y) lo=mid+1; else hi=mid;
    }
  for(; lo<count && list[lo].y==y; lo++){
    if(x>=list[lo].x1 && x<=list[lo].x2) return true;
    }
  return false;
  }


// Return true if the list covers pixels x1 through x2 on scan line y
static FXbool spanCoversSpan(const FXSpan* list,FXuint count,FXint y,FXint x1,FXint x2){
  FXuint lo=0;
  FXuint hi=count;
  while(lo<hi){
    FXuint mid=(lo+hi)>>1;
    if(list[mid].y<y) lo=mid+1; else hi=mid;
    }
  for(; lo<count && list[lo].y==y; lo++){
    if(x1>=list[lo].x1 && x2<=list[lo].x2) return true;
    }
  return false;
  }


/*******************************************************************************/

// Construct empty region
FXRects::FXRects():spans(nullptr),count(0),alloced(0){
  }


// Construct rectangle region
FXRects::FXRects(FXint x,FXint y,FXint w,FXint h):spans(nullptr),count(0),alloced(0){
  setRectangle(x,y,w,h);
  }


// Construct region copied from region r
FXRects::FXRects(const FXRects& r):spans(nullptr),count(0),alloced(0){
  *this=r;
  }


// Assign region r to this one
FXRects& FXRects::operator=(const FXRects& r){
  if(this!=&r){
    FXSpan* list=nullptr;
    FXuint n=r.count;
    if(n && !dupElms(list,r.spans,n)){ return *this; }
    freeElms(spans);
    spans=list;
    count=n;
    alloced=n;
    }
  return *this;
  }


// Destroy region
FXRects::~FXRects(){
  freeElms(spans);
  }


// Append a span to this region
FXbool FXRects::push(const FXSpan& span){
  return spanAdd(spans,count,alloced,span);
  }


// Append a span, merging it with the previous one
FXbool FXRects::pushMerged(const FXSpan& span){
  return spanAddMerged(spans,count,alloced,span);
  }


// Take over a span list
void FXRects::setSpans(FXSpan* list,FXuint n,FXuint cap){
  freeElms(spans);
  spans=list;
  count=n;
  alloced=cap;
  }


// Reset region to empty
void FXRects::clear(){
  freeElms(spans);
  count=0;
  alloced=0;
  }


/*******************************************************************************/

// Make region a single rectangle
void FXRects::setRectangle(FXint x,FXint y,FXint w,FXint h){
  FXSpan span;
  clear();
  if(w<=0 || h<=0) return;
  span.x1=x;
  span.x2=x+w-1;
  for(span.y=y; span.y<y+h; span.y++){
    if(!push(span)){ clear(); return; }
    }
  }


// Make region a polygon, filled at the pixel centers like the rasterizer
void FXRects::setPolygon(const FXPoint* points,FXuint npoints,FXbool winding){
  FXdouble* cross;
  FXint* dir;
  FXuint i,j,cnt;
  FXint y,ymin,ymax,x1,x2,wind,previous;
  FXdouble yc;
  FXSpan span;
  clear();
  if(!points || npoints<3) return;
  if(!allocElms(cross,npoints)) return;
  if(!allocElms(dir,npoints)){ freeElms(cross); return; }
  ymin=ymax=points[0].y;
  for(i=1; i<npoints; i++){
    if(points[i].y<ymin) ymin=points[i].y;
    if(points[i].y>ymax) ymax=points[i].y;
    }
  for(y=ymin; y<ymax; y++){
    yc=y+0.5;
    cnt=0;
    for(i=0; i<npoints; i++){
      const FXPoint& p1=points[i];
      const FXPoint& p2=points[(i+1)%npoints];
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
    if(winding){
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
          if(x1<=x2){
            span.y=y;
            span.x1=x1;
            span.x2=x2;
            if(!pushMerged(span)) break;
            }
          }
        }
      }
    else{
      for(i=0; i+1<cnt; i+=2){
        x1=(FXint)ceil(cross[i]-0.5);
        x2=(FXint)ceil(cross[i+1]-0.5)-1;
        if(x1<=x2){
          span.y=y;
          span.x1=x1;
          span.x2=x2;
          if(!pushMerged(span)) break;
          }
        }
      }
    }
  freeElms(dir);
  freeElms(cross);
  }


/*******************************************************************************/

// Return true if region contains point
FXbool FXRects::contains(FXint x,FXint y) const {
  return spanCovers(spans,count,y,x);
  }


// Return true if region contains the whole rectangle
FXbool FXRects::contains(FXint x,FXint y,FXint w,FXint h) const {
  FXint yy;
  if(w<=0 || h<=0) return true;
  for(yy=y; yy<y+h; yy++){
    if(!spanCoversSpan(spans,count,yy,x,x+w-1)) return false;
    }
  return true;
  }


// Return bounding box
FXRectangle FXRects::bounds() const {
  FXRectangle box(0,0,0,0);
  FXint lo=0;
  FXint hi=0;
  FXuint i;
  if(count==0) return box;
  lo=hi=spans[0].x1;
  for(i=0; i<count; i++){
    if(spans[i].x1<lo) lo=spans[i].x1;
    if(spans[i].x2>hi) hi=spans[i].x2;
    }
  return FXRectangle(lo,spans[0].y,hi-lo+1,spans[count-1].y-spans[0].y+1);
  }


// Offset region by dx,dy; the order of the spans is kept
FXRects& FXRects::offset(FXint dx,FXint dy){
  FXuint i;
  for(i=0; i<count; i++){
    spans[i].y+=dy;
    spans[i].x1+=dx;
    spans[i].x2+=dx;
    }
  return *this;
  }


// Union with region r
FXRects& FXRects::unite(const FXRects& r){
  FXSpan* list=nullptr;
  FXuint n=0;
  FXuint cap=0;
  FXuint i=0;
  FXuint j=0;
  FXbool ok=true;
  while(ok && (i<count || j<r.count)){
    if(j>=r.count || (i<count && spanBefore(spans[i],r.spans[j]))){
      ok=spanAddMerged(list,n,cap,spans[i]);
      i++;
      }
    else{
      ok=spanAddMerged(list,n,cap,r.spans[j]);
      j++;
      }
    }
  if(ok){ setSpans(list,n,cap); } else { freeElms(list); }
  return *this;
  }


// Intersect with region r
FXRects& FXRects::intersect(const FXRects& r){
  FXSpan* list=nullptr;
  FXuint n=0;
  FXuint cap=0;
  FXuint i=0;
  FXuint j=0;
  FXint y,x1,x2;
  FXSpan span;
  FXbool ok=true;
  while(ok && i<count && j<r.count){
    if(spans[i].y<r.spans[j].y){ i++; continue; }
    if(spans[i].y>r.spans[j].y){ j++; continue; }
    y=spans[i].y;
    while(ok && i<count && spans[i].y==y && j<r.count && r.spans[j].y==y){
      x1=FXMAX(spans[i].x1,r.spans[j].x1);
      x2=FXMIN(spans[i].x2,r.spans[j].x2);
      if(x1<=x2){
        span.y=y;
        span.x1=x1;
        span.x2=x2;
        ok=spanAdd(list,n,cap,span);
        }
      if(spans[i].x2<r.spans[j].x2) i++; else j++;
      }
    while(i<count && spans[i].y==y) i++;
    while(j<r.count && r.spans[j].y==y) j++;
    }
  if(ok){ setSpans(list,n,cap); } else { freeElms(list); }
  return *this;
  }


// Subtract region r
FXRects& FXRects::subtract(const FXRects& r){
  FXSpan* list=nullptr;
  FXuint n=0;
  FXuint cap=0;
  FXuint i,k;
  FXuint j=0;
  FXint y,x1,x2,b1,b2;
  FXSpan span;
  FXbool ok=true;
  for(i=0; ok && i<count; i++){
    y=spans[i].y;
    x1=spans[i].x1;
    x2=spans[i].x2;
    while(j<r.count && r.spans[j].y<y) j++;
    for(k=j; ok && k<r.count && r.spans[k].y==y && x1<=x2; k++){
      b1=r.spans[k].x1;
      b2=r.spans[k].x2;
      if(b2<x1) continue;
      if(b1>x2) break;
      if(b1>x1){
        span.y=y;
        span.x1=x1;
        span.x2=b1-1;
        ok=spanAdd(list,n,cap,span);
        }
      x1=b2+1;
      }
    if(ok && x1<=x2){
      span.y=y;
      span.x1=x1;
      span.x2=x2;
      ok=spanAdd(list,n,cap,span);
      }
    }
  if(ok){ setSpans(list,n,cap); } else { freeElms(list); }
  return *this;
  }


// Exclusive or with region r
FXRects& FXRects::xorbit(const FXRects& r){
  FXRects mine(*this);
  FXRects theirs(r);
  mine.subtract(r);
  theirs.subtract(*this);
  *this=mine;
  unite(theirs);
  return *this;
  }


// Return true if the two regions cover the same pixels
FXbool FXRects::operator==(const FXRects& r) const {
  FXuint i;
  if(count!=r.count) return false;
  for(i=0; i<count; i++){
    if(spans[i].y!=r.spans[i].y || spans[i].x1!=r.spans[i].x1 || spans[i].x2!=r.spans[i].x2) return false;
    }
  return true;
  }

}
