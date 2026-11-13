/********************************************************************************
*                                                                               *
*                     V i r t u a l   W i n d o w   M a p                     *
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
#include "fxwinmap.h"

/*
  Notes:

  - Design notes stay in fxwinmap.h; this file holds the list handling.
  - The entries live in one array and are searched linearly: a window tree has
    a few hundred entries at most and the searches are short, so a hash would
    only add bookkeeping.  Destroyed entries are dropped by moving the last
    entry into the hole, so the array never grows beyond the peak window count.
  - Geometry is kept relative to the parent, like the X11 server keeps child
    windows; the global rectangle is the sum of the ancestors.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct an empty map
FXWinMap::FXWinMap():entries(nullptr),count(0),alloced(0),firstshell(0),lastshell(0),serial(0),flags(0){
  }


// Destroy the map
FXWinMap::~FXWinMap(){
  freeElms(entries);
  }


// Find an entry by id
FXWinEntry* FXWinMap::find(FXID id){
  FXuint i;
  if(id){
    for(i=0; i<count; i++){
      if(entries[i].id==id) return &entries[i];
      }
    }
  return nullptr;
  }


// Find an entry by id
const FXWinEntry* FXWinMap::find(FXID id) const {
  FXuint i;
  if(id){
    for(i=0; i<count; i++){
      if(entries[i].id==id) return &entries[i];
      }
    }
  return nullptr;
  }


// Return a fresh entry, appended to the array
FXWinEntry* FXWinMap::add(){
  FXWinEntry* e;
  if(count==alloced){
    FXuint cap=alloced?alloced*2:64;
    if(!resizeElms(entries,cap)) return nullptr;
    alloced=cap;
    }
  e=&entries[count++];
  clearElms(e,1);
  e->id=++serial;
  return e;
  }

// Remove an entry from its parent's child list
void FXWinMap::unlinkEntry(FXWinEntry* e){
  FXWinEntry* p;
  if(e->parent){
    p=find(e->parent);
    if(p){
      if(p->first==e->id) p->first=e->next;
      if(p->last==e->id) p->last=e->prev;
      }
    }
  else{
    if(firstshell==e->id) firstshell=e->next;
    if(lastshell==e->id) lastshell=e->prev;
    }
  if(e->prev){
    p=find(e->prev);
    if(p) p->next=e->next;
    }
  if(e->next){
    p=find(e->next);
    if(p) p->prev=e->prev;
    }
  e->prev=0;
  e->next=0;
  }


// Place an entry at the top of its parent's child list
void FXWinMap::linkTop(FXWinEntry* e){
  FXWinEntry* p;
  if(e->parent){
    p=find(e->parent);
    if(p){
      e->prev=p->last;
      e->next=0;
      if(p->last){
        FXWinEntry* l=find(p->last);
        if(l) l->next=e->id;
        }
      p->last=e->id;
      if(!p->first) p->first=e->id;
      }
    }
  else{
    e->prev=lastshell;
    e->next=0;
    if(lastshell){
      FXWinEntry* l=find(lastshell);
      if(l) l->next=e->id;
      }
    lastshell=e->id;
    if(!firstshell) firstshell=e->id;
    }
  }


// Create a window
FXID FXWinMap::create(FXID parent,FXint x,FXint y,FXint w,FXint h,FXuint flgs){
  FXWinEntry* e;
  if(parent && !find(parent)) return 0;
  if(!(e=add())) return 0;
  e->parent=parent;
  e->owner=0;
  e->x=x;
  e->y=y;
  e->w=FXMAX(w,1);
  e->h=FXMAX(h,1);
  e->flags=flgs;
  linkTop(e);
  return e->id;
  }


// Set the owner of a popup
void FXWinMap::setOwner(FXID id,FXID owner){
  FXWinEntry* e=find(id);
  if(e) e->owner=owner;
  }


// Owner of a popup
FXID FXWinMap::getOwner(FXID id) const {
  const FXWinEntry* e=find(id);
  return e?e->owner:0;
  }


// Destroy a window and everything below it
void FXWinMap::removeSubtree(FXID id){
  FXWinEntry* e;
  FXID child;
  while((e=find(id))!=nullptr){
    while((child=e->first)!=0){
      removeSubtree(child);
      if(!(e=find(id))) return;
      }
    unlinkEntry(e);
    *e=entries[--count];
    }
  }


// Destroy a window and its children
void FXWinMap::destroy(FXID id){
  FXWinEntry* e;
  if((e=find(id))!=nullptr){
    unlinkEntry(e);
    removeSubtree(id);
    }
  }


// Return true if the id is known
FXbool FXWinMap::hasWindow(FXID id) const {
  return find(id)!=nullptr;
  }


// Return the entry of a window
const FXWinEntry* FXWinMap::getEntry(FXID id) const {
  return find(id);
  }


// Parent of a window
FXID FXWinMap::getParent(FXID id) const {
  const FXWinEntry* e=find(id);
  return e?e->parent:0;
  }


// Bottom-most child
FXID FXWinMap::getFirst(FXID id) const {
  const FXWinEntry* e=find(id);
  return e?e->first:0;
  }


// Top-most child
FXID FXWinMap::getLast(FXID id) const {
  const FXWinEntry* e=find(id);
  return e?e->last:0;
  }


// Window below
FXID FXWinMap::getPrev(FXID id) const {
  const FXWinEntry* e=find(id);
  return e?e->prev:0;
  }


// Window above
FXID FXWinMap::getNext(FXID id) const {
  const FXWinEntry* e=find(id);
  return e?e->next:0;
  }


// Change the geometry relative to the parent
void FXWinMap::setGeometry(FXID id,FXint x,FXint y,FXint w,FXint h){
  FXWinEntry* e=find(id);
  if(e){
    e->x=x;
    e->y=y;
    e->w=FXMAX(w,1);
    e->h=FXMAX(h,1);
    }
  }


// Change part of the geometry
void FXWinMap::setPlacement(FXID id,FXint x,FXint y,FXint w,FXint h,FXuint flgs){
  FXWinEntry* e=find(id);
  if(e){
    if(flgs&PLACE_X) e->x=x;
    if(flgs&PLACE_Y) e->y=y;
    if(flgs&PLACE_W) e->w=FXMAX(w,1);
    if(flgs&PLACE_H) e->h=FXMAX(h,1);
    }
  }


// Geometry relative to the parent
FXRectangle FXWinMap::getGeometry(FXID id) const {
  const FXWinEntry* e=find(id);
  if(e) return FXRectangle(e->x,e->y,e->w,e->h);
  return FXRectangle(0,0,0,0);
  }


// Geometry in shell coordinates
FXRectangle FXWinMap::getGlobalGeometry(FXID id) const {
  const FXWinEntry* e=find(id);
  FXRectangle r(0,0,0,0);
  FXbool first=true;
  while(e){
    r.x+=e->x;
    r.y+=e->y;
    if(first){ r.w=e->w; r.h=e->h; first=false; }
    e=find(e->parent);
    }
  return r;
  }


// Show or hide
void FXWinMap::setShown(FXID id,FXbool shown){
  FXWinEntry* e=find(id);
  if(e){
    if(shown) e->flags|=WINMAP_SHOWN;
    else e->flags&=~WINMAP_SHOWN;
    }
  }


// True if the window itself is shown
FXbool FXWinMap::isShown(FXID id) const {
  const FXWinEntry* e=find(id);
  return e?((e->flags&WINMAP_SHOWN)!=0):false;
  }


// True if the window and all its parents are shown
FXbool FXWinMap::isVisible(FXID id) const {
  const FXWinEntry* e=find(id);
  while(e){
    if(!(e->flags&WINMAP_SHOWN)) return false;
    e=find(e->parent);
    }
  return true;
  }


// Move to the top of the stacking order
void FXWinMap::raise(FXID id){
  FXWinEntry* e=find(id);
  FXID top;
  if(!e) return;
  top=e->parent?find(e->parent)->last:lastshell;
  if(top!=id){
    unlinkEntry(e);
    linkTop(e);
    }
  }


// Move to the bottom of the stacking order
void FXWinMap::lower(FXID id){
  FXWinEntry* e=find(id);
  FXWinEntry* p;
  FXID bottom;
  if(!e) return;
  bottom=e->parent?find(e->parent)->first:firstshell;
  if(bottom==id) return;
  unlinkEntry(e);
  if(e->parent){
    p=find(e->parent);
    if(p){
      e->next=p->first;
      e->prev=0;
      if(p->first){
        FXWinEntry* f=find(p->first);
        if(f) f->prev=e->id;
        }
      p->first=e->id;
      if(!p->last) p->last=e->id;
      }
    }
  else{
    e->next=firstshell;
    e->prev=0;
    if(firstshell){
      FXWinEntry* f=find(firstshell);
      if(f) f->prev=e->id;
      }
    firstshell=e->id;
    if(!lastshell) lastshell=e->id;
    }
  }


// Move a window under a new parent, keeping its place on the screen
void FXWinMap::reparent(FXID id,FXID parent){
  FXWinEntry* e=find(id);
  FXWinEntry* p;
  FXRectangle global;
  if(!e || id==parent) return;
  if(parent && !find(parent)) return;
  // A window cannot become its own descendant
  for(p=find(parent); p; p=find(p->parent)){
    if(p->id==id) return;
    }
  global=getGlobalGeometry(id);
  unlinkEntry(e);
  e->parent=parent;
  if(parent){
    p=find(parent);
    e->x=global.x-p->x;
    e->y=global.y-p->y;
    }
  else{
    e->x=global.x;
    e->y=global.y;
    }
  linkTop(e);
  }


// Deepest visible child at the given shell coordinates
FXID FXWinMap::hitTest(FXint gx,FXint gy) const {
  const FXWinEntry* e;
  FXID child;
  FXID hit;
  FXID s;
  // Start at the shells, top-most first; they are the only entries without a parent
  for(s=lastshell; s; s=e->prev){
    e=find(s);
    if(!e) break;
    if(!(e->flags&WINMAP_SHOWN)) continue;
    if(gx<e->x || gy<e->y || gx>=e->x+e->w || gy>=e->y+e->h) continue;
    // From here on the coordinates are local to the current window
    gx-=e->x;
    gy-=e->y;
    for(;;){
      hit=e->id;
      child=e->last;
      while(child){
        const FXWinEntry* c=find(child);
        if(!c) break;
        if((c->flags&WINMAP_SHOWN) && gx>=c->x && gy>=c->y && gx<c->x+c->w && gy<c->y+c->h){
          gx-=c->x;
          gy-=c->y;
          e=c;
          break;
          }
        child=c->prev;
        }
      if(child==0) break;
      }
    return hit;
    }
  return 0;
  }


// Nearest ancestor that owns a surface
FXID FXWinMap::getShell(FXID id) const {
  const FXWinEntry* e=find(id);
  FXID shell=0;
  while(e){
    if(e->flags&WINMAP_SHELL) shell=e->id;
    e=find(e->parent);
    }
  return shell;
  }

}
