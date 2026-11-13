/********************************************************************************
*                                                                               *
*                     R e c t a n g l e   R e g i o n   T e s t                *
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
#include "fx.h"
#include "testargs.h"
#include "../lib/fxrects.h"
#include <stdio.h>
#include <string.h>

/*
  Notes:

  - Console test for FXRects, the portable region of the Wayland and browser
    platforms (phase 1 of docs/wayland-plan.md); no display is needed.
  - The region is compared against a brute force pixel model on a small grid:
    every operation is applied to both and every pixel of the grid is checked,
    so a bad span, a lost row or a missing merge shows up immediately.
  - Polygons are checked against an independent point in polygon test instead
    of against a copy of the scan converter, so the rule itself is covered.
*/

// Grid used by the pixel model
const FXint GW=48;
const FXint GH=48;


// Brute force pixel model
struct FXModel {
  FXbool pix[GW*GH];
  };


// Clear the model
static void modelClear(FXModel& m){
  memset(m.pix,0,sizeof(m.pix));
  }


// Union a rectangle into the model
static void modelRect(FXModel& m,FXint x,FXint y,FXint w,FXint h){
  FXint xx,yy;
  for(yy=FXMAX(y,0); yy<FXMIN(y+h,GH); yy++){
    for(xx=FXMAX(x,0); xx<FXMIN(x+w,GW); xx++){
      m.pix[yy*GW+xx]=true;
      }
    }
  }


// Intersect the model with a rectangle
static void modelIntersectRect(FXModel& m,FXint x,FXint y,FXint w,FXint h){
  FXint xx,yy;
  for(yy=0; yy<GH; yy++){
    for(xx=0; xx<GW; xx++){
      if(xx<x || yy<y || xx>=x+w || yy>=y+h) m.pix[yy*GW+xx]=false;
      }
    }
  }


// Subtract a rectangle from the model
static void modelSubtractRect(FXModel& m,FXint x,FXint y,FXint w,FXint h){
  FXint xx,yy;
  for(yy=FXMAX(y,0); yy<FXMIN(y+h,GH); yy++){
    for(xx=FXMAX(x,0); xx<FXMIN(x+w,GW); xx++){
      m.pix[yy*GW+xx]=false;
      }
    }
  }


// Union of two models
static void modelUnion(FXModel& m,const FXModel& o){
  FXint i;
  for(i=0; i<GW*GH; i++){ m.pix[i]=m.pix[i]||o.pix[i]; }
  }


// Intersection of two models
static void modelIntersect(FXModel& m,const FXModel& o){
  FXint i;
  for(i=0; i<GW*GH; i++){ m.pix[i]=m.pix[i]&&o.pix[i]; }
  }


// Difference of two models
static void modelSubtract(FXModel& m,const FXModel& o){
  FXint i;
  for(i=0; i<GW*GH; i++){ m.pix[i]=m.pix[i]&&!o.pix[i]; }
  }


// Exclusive or of two models
static void modelXor(FXModel& m,const FXModel& o){
  FXint i;
  for(i=0; i<GW*GH; i++){ m.pix[i]=m.pix[i]!=o.pix[i]; }
  }


// Offset the model
static void modelOffset(FXModel& m,FXint dx,FXint dy){
  FXModel n;
  FXint x,y;
  modelClear(n);
  for(y=0; y<GH; y++){
    for(x=0; x<GW; x++){
      if(m.pix[y*GW+x]){
        if(x+dx>=0 && x+dx<GW && y+dy>=0 && y+dy<GH) n.pix[(y+dy)*GW+(x+dx)]=true;
        }
      }
    }
  m=n;
  }


// Fill the model with a polygon, using an independent crossing test at the
// pixel centers with the ray going to +x
static void modelPolygon(FXModel& m,const FXPoint* points,FXuint npoints,FXbool winding){
  FXuint i;
  FXint x,y;
  modelClear(m);
  for(y=0; y<GH; y++){
    for(x=0; x<GW; x++){
      FXdouble px=x+0.5;
      FXdouble py=y+0.5;
      FXint crossings=0;
      FXint wind=0;
      for(i=0; i<npoints; i++){
        const FXPoint& p1=points[i];
        const FXPoint& p2=points[(i+1)%npoints];
        if((p1.y<=py)!=(p2.y<=py)){
          FXdouble xint=p1.x+(py-p1.y)*(FXdouble)(p2.x-p1.x)/(p2.y-p1.y);
          if(xint>px){
            crossings++;
            wind+=(p2.y>p1.y)?1:-1;
            }
          }
        }
      m.pix[y*GW+x]=winding ? (wind!=0) : ((crossings&1)!=0);
      }
    }
  }


/*******************************************************************************/

// Compare a region with the model and report the first difference
static FXbool compare(const FXchar* what,const FXRects& region,const FXModel& model){
  FXRectangle box(0,0,0,0);
  FXRectangle want(0,0,0,0);
  FXint x0=GW,x1=-1,y0=GH,y1=-1;
  FXint x,y;
  // Pixels
  for(y=0; y<GH; y++){
    for(x=0; x<GW; x++){
      if(region.contains(x,y)!=model.pix[y*GW+x]){
        printf("  %-24s pixel (%d,%d): region %d, model %d\n",what,x,y,region.contains(x,y),model.pix[y*GW+x]);
        return false;
        }
      }
    }
  // Bounding box of the model
  for(y=0; y<GH; y++){
    for(x=0; x<GW; x++){
      if(model.pix[y*GW+x]){
        if(x<x0) x0=x;
        if(x>x1) x1=x;
        if(y<y0) y0=y;
        if(y>y1) y1=y;
        }
      }
    }
  if(x1>=0) want=FXRectangle(x0,y0,x1-x0+1,y1-y0+1);
  box=region.bounds();
  if(box.x!=want.x || box.y!=want.y || box.w!=want.w || box.h!=want.h){
    printf("  %-24s bounds (%d,%d,%d,%d), expected (%d,%d,%d,%d)\n",what,box.x,box.y,box.w,box.h,want.x,want.y,want.w,want.h);
    return false;
    }
  // A rectangle query: the region must agree with the model
  {
    FXbool covered=true;
    for(y=4; covered && y<20; y++){
      for(x=4; x<20; x++){
        if(!model.pix[y*GW+x]){ covered=false; break; }
        }
      }
    if(region.contains(4,4,16,16)!=covered){
      printf("  %-24s rectangle query at (4,4,16,16)\n",what);
      return false;
      }
    }
  return true;
  }


// Deterministic pseudo random numbers
static FXuint seed=12345;
static FXuint nextRandom(){
  seed=seed*1103515245u+12345u;
  return (seed>>8);
  }


/*******************************************************************************/

// Run the test battery
static FXint runTests(){
  FXPoint star[10];
  FXPoint arrow[6];
  FXModel model;
  FXModel other;
  FXint failures=0;
  FXint i,x,y,w,h;

  // A single rectangle, on every pixel
  {
    FXRects region(3,5,20,11);
    modelClear(model);
    modelRect(model,3,5,20,11);
    if(!compare("rectangle",region,model)) failures++;
    if(region.empty()){ printf("  rectangle reported empty\n"); failures++; }
  }

  // Empty region
  {
    FXRects region(10,10,0,5);
    modelClear(model);
    if(!compare("empty",region,model)) failures++;
    if(!region.empty()){ printf("  empty region not empty\n"); failures++; }
  }

  // Union of overlapping rectangles, which must merge into one span per row
  {
    FXRects region(4,4,10,10);
    FXRects add(11,8,12,8);
    modelClear(model);
    modelRect(model,4,4,10,10);
    modelRect(model,11,8,12,8);
    region.unite(add);
    if(!compare("union",region,model)) failures++;
  }

  // Intersection, difference and exclusive or
  {
    FXRects a(4,4,24,20);
    FXRects b(14,10,24,20);
    FXRects r;
    modelClear(model);
    modelRect(model,4,4,24,20);

    modelClear(other);
    modelRect(other,14,10,24,20);

    r=a; r.intersect(b);
    modelIntersect(model,other);
    if(!compare("intersect",r,model)) failures++;

    r=a; r.subtract(b);
    modelClear(model);
    modelRect(model,4,4,24,20);
    modelSubtract(model,other);
    if(!compare("subtract",r,model)) failures++;

    r=a; r.xorbit(b);
    modelClear(model);
    modelRect(model,4,4,24,20);
    modelXor(model,other);
    if(!compare("xor",r,model)) failures++;
  }

  // Offset
  {
    FXRects region(2,2,10,6);
    modelClear(model);
    modelRect(model,2,2,10,6);
    region.offset(7,9);
    modelOffset(model,7,9);
    if(!compare("offset",region,model)) failures++;
    region.offset(-9,-11);
    modelOffset(model,-9,-11);
    if(!compare("offset back",region,model)) failures++;
  }

  // Polygon with both fill rules
  {
    FXPoint points[6];
    points[0].x=6;   points[0].y=6;
    points[1].x=40;  points[1].y=6;
    points[2].x=24;  points[2].y=20;
    points[3].x=40;  points[3].y=40;
    points[4].x=6;   points[4].y=40;
    points[5].x=20;  points[5].y=20;
    {
      FXRects region;
      region.setPolygon(points,6,false);
      modelPolygon(model,points,6,false);
      if(!compare("polygon even-odd",region,model)) failures++;
      }
    {
      FXRects region;
      region.setPolygon(points,6,true);
      modelPolygon(model,points,6,true);
      if(!compare("polygon winding",region,model)) failures++;
      }
  }

  // Star with five points, where the two rules differ
  {
    star[0].x=24; star[0].y=4;
    star[1].x=30; star[1].y=17;
    star[2].x=44; star[2].y=17;
    star[3].x=33; star[3].y=26;
    star[4].x=37; star[4].y=40;
    star[5].x=24; star[5].y=31;
    star[6].x=11; star[6].y=40;
    star[7].x=15; star[7].y=26;
    star[8].x=4;  star[8].y=17;
    star[9].x=18; star[9].y=17;
    {
      FXRects region;
      region.setPolygon(star,10,false);
      modelPolygon(model,star,10,false);
      if(!compare("star even-odd",region,model)) failures++;
      }
    {
      FXRects region;
      region.setPolygon(star,10,true);
      modelPolygon(model,star,10,true);
      if(!compare("star winding",region,model)) failures++;
      }
  }

  // Triangle created from a relative outline
  {
    arrow[0].x=8;  arrow[0].y=8;
    arrow[1].x=30; arrow[1].y=24;
    arrow[2].x=8;  arrow[2].y=40;
    {
      FXRects region;
      region.setPolygon(arrow,3,false);
      modelPolygon(model,arrow,3,false);
      if(!compare("triangle",region,model)) failures++;
      }
  }

  // Canonical form: two ways of building the same pixel set compare equal
  {
    FXRects a(4,4,20,20);
    FXRects b(4,4,10,20);
    FXRects c(14,4,10,5);
    FXRects d(14,9,10,15);
    b.unite(c);
    b.unite(d);
    if(a!=b){
      printf("  canonical form: same pixels, different lists (%u vs %u spans)\n",a.getSpanCount(),b.getSpanCount());
      failures++;
      }
    if(!(a==b)){ printf("  canonical form: equality failed\n"); failures++; }
  }

  // Random operations against the model; the rectangles stay inside the grid
  for(i=0; i<200; i++){
    FXRects a;
    FXRects b;
    modelClear(model);
    modelClear(other);
    x=nextRandom()%30;
    y=nextRandom()%30;
    w=1+nextRandom()%18;
    h=1+nextRandom()%18;
    a.setRectangle(x,y,w,h);
    modelRect(model,x,y,w,h);
    x=nextRandom()%30;
    y=nextRandom()%30;
    w=1+nextRandom()%18;
    h=1+nextRandom()%18;
    b.setRectangle(x,y,w,h);
    modelRect(other,x,y,w,h);
    switch(nextRandom()%4){
      case 0: a.unite(b);      modelUnion(model,other);     break;
      case 1: a.intersect(b);  modelIntersect(model,other); break;
      case 2: a.subtract(b);   modelSubtract(model,other);  break;
      default:a.xorbit(b);     modelXor(model,other);       break;
      }
    if(!compare("random",a,model)){ failures++; break; }
    }
  return failures;
  }


/*******************************************************************************/

// Entry point
int main(int argc,char** argv){
  FXint failures;
  fxtestargs(argc,argv);
  printf("FXRects test on a %dx%d grid\n",GW,GH);
  failures=runTests();
  if(failures){
    printf("FAILED: %d case(s)\n",failures);
    return 1;
    }
  printf("all cases passed\n");
  return 0;
  }
