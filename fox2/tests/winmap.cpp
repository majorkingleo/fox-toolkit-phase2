/********************************************************************************
*                                                                               *
*                   V i r t u a l   W i n d o w   M a p   T e s t             *
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
#include "../lib/fxwinmap.h"
#include <stdio.h>
#include <string.h>

/*
  Notes:

  - Console test for FXWinMap, the virtual window table of the Wayland and
    browser platforms (phase 4 of docs/wayland-plan.md); no display is needed.
  - The map is compared against a brute force model: the model keeps the parent
    of every window and its geometry, and it answers hit tests by walking all
    windows, computing their global rectangles and taking the deepest, top-most
    and visible one.  Stacking is modelled by the order in which the windows
    were created, so the test does not trust the list links it is testing.
*/

// Model of one window
struct FXModelWindow {
  FXID   id;
  FXID   parent;
  FXint  x,y,w,h;
  FXbool shown;
  FXint  order;                 // Creation order; higher is above
  FXbool used;
  };

const FXint MAXWIN=256;

// Brute force model
struct FXModel {
  FXModelWindow win[MAXWIN];
  FXint         serial;
  FXint         order;
  };


// Reset the model
static void modelClear(FXModel& m){
  FXint i;
  for(i=0; i<MAXWIN; i++) m.win[i].used=false;
  m.serial=0;
  m.order=0;
  }


// Add a window to the model; the id is the one the map handed out
static void modelCreate(FXModel& m,FXID id,FXID parent,FXint x,FXint y,FXint w,FXint h,FXbool shown){
  FXint i;
  for(i=0; i<MAXWIN; i++){
    if(!m.win[i].used){
      m.win[i].used=true;
      m.win[i].id=id;
      m.win[i].parent=parent;
      m.win[i].x=x;
      m.win[i].y=y;
      m.win[i].w=w;
      m.win[i].h=h;
      m.win[i].shown=shown;
      m.win[i].order=++m.order;
      return;
      }
    }
  }


// Find a model window
static FXModelWindow* modelFind(FXModel& m,FXID id){
  FXint i;
  for(i=0; i<MAXWIN; i++){
    if(m.win[i].used && m.win[i].id==id) return &m.win[i];
    }
  return nullptr;
  }


// Remove a window and its children from the model
static void modelDestroy(FXModel& m,FXID id){
  FXint i;
  for(i=0; i<MAXWIN; i++){
    if(m.win[i].used && m.win[i].parent==id){ modelDestroy(m,m.win[i].id); }
    }
  for(i=0; i<MAXWIN; i++){
    if(m.win[i].used && m.win[i].id==id) m.win[i].used=false;
    }
  }


// Global rectangle of a model window
static FXRectangle modelGlobal(FXModel& m,FXID id){
  FXModelWindow* w=modelFind(m,id);
  FXRectangle r(0,0,0,0);
  FXbool first=true;
  while(w){
    r.x+=w->x;
    r.y+=w->y;
    if(first){ r.w=w->w; r.h=w->h; first=false; }
    w=w->parent?modelFind(m,w->parent):nullptr;
    }
  return r;
  }


// True if the window and all its parents are shown
static FXbool modelVisible(FXModel& m,FXID id){
  FXModelWindow* w=modelFind(m,id);
  while(w){
    if(!w->shown) return false;
    w=w->parent?modelFind(m,w->parent):nullptr;
    }
  return true;
  }


// Move a window in the model, keeping its place on the screen
static void modelReparent(FXModel& m,FXID id,FXID parent){
  FXModelWindow* w=modelFind(m,id);
  FXModelWindow* p;
  FXRectangle g;
  if(!w) return;
  g=modelGlobal(m,id);
  w->parent=parent;
  w->order=++m.order;                 // A reparented window goes to the top
  if(parent){
    p=modelFind(m,parent);
    if(p){
      w->x=g.x-p->x;
      w->y=g.y-p->y;
      }
    }
  else{
    w->x=g.x;
    w->y=g.y;
    }
  }


// Recursive hit test inside a window, coordinates local to that window
static FXID modelHitIn(FXModel& m,FXID parent,FXint gx,FXint gy){
  FXModelWindow* best=nullptr;
  FXint i;
  for(i=0; i<MAXWIN; i++){
    FXModelWindow* w=&m.win[i];
    if(!w->used || w->parent!=parent) continue;
    if(!w->shown) continue;
    if(gx<w->x || gy<w->y || gx>=w->x+w->w || gy>=w->y+w->h) continue;
    if(!best || w->order>best->order) best=w;
    }
  if(best) return modelHitIn(m,best->id,gx-best->x,gy-best->y);
  return parent;
  }


// Brute force hit test: walk the tree top down in stacking order
static FXID modelHitTest(FXModel& m,FXint gx,FXint gy){
  FXModelWindow* best=nullptr;
  FXint i;
  for(i=0; i<MAXWIN; i++){
    FXModelWindow* w=&m.win[i];
    if(!w->used || w->parent!=0) continue;
    if(!w->shown) continue;
    if(gx<w->x || gy<w->y || gx>=w->x+w->w || gy>=w->y+w->h) continue;
    if(!best || w->order>best->order) best=w;
    }
  if(!best) return 0;
  return modelHitIn(m,best->id,gx-best->x,gy-best->y);
  }


/*******************************************************************************/

// Number of failed checks
static FXint failures=0;


// Record a failed check
static void check(FXbool ok,const FXchar* what){
  if(!ok){
    printf("  FAIL %s\n",what);
    failures++;
    }
  }


// Compare the map and the model over a grid of points
static void compareHits(const FXchar* what,FXWinMap& map,FXModel& model,FXint w,FXint h){
  FXint x,y;
  for(y=0; y<h; y++){
    for(x=0; x<w; x++){
      FXID a=map.hitTest(x,y);
      FXID b=modelHitTest(model,x,y);
      if(a!=b){
        printf("  FAIL %s: hit test at %d,%d is %u, model says %u\n",what,x,y,a,b);
        failures++;
        return;
        }
      }
    }
  }


/*******************************************************************************/

// Run the test battery
static FXint runTests(){
  FXWinMap map;
  FXModel model;
  FXID shell,content,button,label,popup;
  FXint i,x,y;

  // A shell with two nested children
  {
    modelClear(model);
    shell=map.create(0,0,0,200,150,WINMAP_SHELL|WINMAP_SHOWN);
    check(shell!=0,"create shell");
    modelCreate(model,shell,0,0,0,200,150,true);
    content=map.create(shell,10,10,180,130,WINMAP_SHOWN);
    modelCreate(model,content,shell,10,10,180,130,true);
    button=map.create(content,20,20,60,30,WINMAP_SHOWN);
    modelCreate(model,button,content,20,20,60,30,true);
    label=map.create(content,100,20,60,30,WINMAP_SHOWN);
    modelCreate(model,label,content,100,20,60,30,true);
    check(map.getCount()==4,"four windows");
    check(map.getParent(button)==content,"parent of button");
    check(map.getGlobalGeometry(button)==FXRectangle(30,30,60,30),"global geometry of button");
    compareHits("nested",map,model,200,150);
    check(map.hitTest(35,35)==button,"button is hit");
    check(map.hitTest(15,15)==content,"content is hit beside the button");
    check(map.hitTest(5,5)==shell,"shell is hit outside the content");
    check(map.hitTest(250,300)==0,"nothing outside the shell");

    // Hit testing is in screen coordinates: a shell that was placed somewhere
    // else moves its whole tree with it, so a point inside the shell is only
    // found once the place of the shell is added
    map.setPlacement(shell,40,28,200,150,FXWinMap::PLACE_X|FXWinMap::PLACE_Y);
    check(map.hitTest(35,35)==0,"surface coordinates find nothing once the shell moved");
    check(map.hitTest(40+35,28+35)==button,"the place of the shell has to be added");
    check(map.hitTest(40+5,28+5)==shell,"the shell is hit at its new place");
    map.setPlacement(shell,0,0,200,150,FXWinMap::PLACE_X|FXWinMap::PLACE_Y);
  }

  // Stacking: raising the left button puts it on top of the right one
  {
    FXID left=map.create(content,30,40,80,40,WINMAP_SHOWN);
    modelCreate(model,left,content,30,40,80,40,true);
    check(map.hitTest(60,55)==left,"left button is hit");
    check(map.getLast(content)==left,"left button is on top");
    map.raise(button);
    // The model knows nothing about raise; move the creation order instead
    model.win[0].order=0;
    {
      FXModelWindow* w=modelFind(model,button);
      if(w) w->order=++model.order;
    }
    check(map.getLast(content)==button,"button is on top after raise");
    check(map.hitTest(60,55)==button,"button is hit after raise");
    map.lower(button);
    check(map.getFirst(content)==button,"button is at the bottom after lower");
    check(map.hitTest(60,55)==left,"left button is hit after lower");
    {
      FXModelWindow* w=modelFind(model,button);
      if(w) w->order=0;
    }
  }

  // Hidden windows are neither visible nor hit
  {
    map.setShown(button,false);
    FXModelWindow* w=modelFind(model,button);
    if(w) w->shown=false;
    compareHits("hidden",map,model,200,150);
    map.setShown(content,false);
    w=modelFind(model,content);
    if(w) w->shown=false;
    compareHits("hidden parent",map,model,200,150);
    check(map.isShown(content)==false,"content is not shown");
    check(map.isVisible(button)==false,"button is not visible");
    map.setShown(content,true);
    w=modelFind(model,content);
    if(w) w->shown=true;
    map.setShown(button,true);
    w=modelFind(model,button);
    if(w) w->shown=true;
    compareHits("shown again",map,model,200,150);
  }

  // Reparenting keeps the window where it was on the screen
  {
    FXRectangle before=map.getGlobalGeometry(label);
    map.reparent(label,shell);
    modelReparent(model,label,shell);
    check(map.getParent(label)==shell,"label is a child of the shell now");
    check(map.getGlobalGeometry(label)==before,"label kept its place");
    compareHits("reparented",map,model,200,150);
    map.reparent(label,content);
    modelReparent(model,label,content);
    check(map.getGlobalGeometry(label)==before,"label is back in place");
    compareHits("reparented back",map,model,200,150);
  }

  // A window cannot become its own descendant
  {
    FXID parent=map.getParent(content);
    map.reparent(shell,button);
    check(map.getParent(shell)==0,"shell cannot be reparented below its child");
    check(map.getParent(content)==parent,"tree is unchanged");
  }

  // Destroying a subtree removes all of it
  {
    map.destroy(content);
    check(map.hasWindow(button)==false,"button went with its parent");
    check(map.hasWindow(label)==false,"label went with its parent");
    check(map.hasWindow(shell),"the shell survives");
    check(map.getCount()==1,"only the shell is left");
    map.destroy(shell);
    check(map.getCount()==0,"the map is empty");
  }

  // A popup shell above two windows
  {
    modelClear(model);
    shell=map.create(0,0,0,100,100,WINMAP_SHELL|WINMAP_SHOWN);
    modelCreate(model,shell,0,0,0,100,100,true);
    popup=map.create(0,50,50,40,40,WINMAP_SHELL|WINMAP_POPUP|WINMAP_SHOWN);
    modelCreate(model,popup,0,50,50,40,40,true);
    check(map.getShell(popup)==popup,"the popup is its own shell");
    check(map.hitTest(60,60)==popup,"the popup is above the first shell");
    map.lower(popup);
    check(map.hitTest(60,60)==shell,"the first shell is above after lower");
    map.destroy(popup);
    map.destroy(shell);
  }

  // Random trees against the brute force model
  for(i=0; i<200; i++){
    FXID ids[8];
    FXID parent;
    FXint n=1+((i*7)%7);
    FXint j;
    modelClear(model);
    ids[0]=map.create(0,0,0,120,120,WINMAP_SHELL|WINMAP_SHOWN);
    modelCreate(model,ids[0],0,0,0,120,120,true);
    for(j=1; j<n; j++){
      parent=ids[(i*j)%j];
      x=(i*13+j*7)%40;
      y=(i*11+j*5)%40;
      ids[j]=map.create(parent,x,y,20+((i+j)%30),20+((i*2+j)%30),WINMAP_SHOWN);
      modelCreate(model,ids[j],parent,x,y,20+((i+j)%30),20+((i*2+j)%30),true);
      }
    compareHits("random tree",map,model,120,120);
    for(j=n; j--; ){ map.destroy(ids[j]); }
    }
  return failures;
  }


/*******************************************************************************/

// Entry point
int main(int argc,char** argv){
  FXint failed;
  fxtestargs(argc,argv);
  printf("FXWinMap test\n");
  failed=runTests();
  if(failed){
    printf("FAILED: %d check(s)\n",failed);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }
