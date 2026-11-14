/********************************************************************************
*                                                                               *
*                          W i n d o w   ( W a y l a n d )                      *
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
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegion.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXApp.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Wayland has no server side child windows and no drawing requests: a window
    is a row in the virtual window table, and its pixels are a place in the
    frame buffer of its shell.  Everything the core does with window ids goes
    through the platform window seam; what remains here are the members that
    only a platform can answer.
  - The window manager keeps the stacking order of the toplevels, and there is
    no way to ask for it, so raise() and lower() only change the order of the
    virtual shells; the compositor decides what the user sees.
  - Client side grabs do not exist.  The pointer is implicitly grabbed by the
    surface that received the press, so grab() only records who owns the
    pointer, exactly as FXApp expects, and ungrab() forgets it.
  - Shapes need wl_region (for input) and a viewport (for the visible outline);
    both arrive with the popups of phase 7, so the shape calls only record for
    now that a shape was asked for.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

// Drag types; Wayland transports mime types instead of atoms, so these two
// pre-registered types get indices of their own until the data device of
// phase 8 fills in the mime table they refer to
const FXDragType FXWindow::stringType=1;
FXDragType FXWindow::utf16Type=2;


// The place of a window in the coordinates of the screen; the root window has
// no row in the table and sits at the origin
static FXRectangle screenRect(FXWaylandApp* app,const FXWindow* window){
  FXRectangle r;
  r.x=0;
  r.y=0;
  r.w=0;
  r.h=0;
  if(app && window && window->id()){
    r=app->childmap.getGlobalGeometry(window->id());
    }
  return r;
  }


/*******************************************************************************/

// Wayland has no device context handles
FXID FXWindow::GetDC() const {
  return 0;
  }


// Nothing to release
int FXWindow::ReleaseDC(FXID) const {
  return 0;
  }


// Wayland has no window classes
const void* FXWindow::GetClass() const {
  return nullptr;
  }


// Shapes wait for the popup phase, which brings the input region and viewport
void FXWindow::setShape(const FXRegion&){
  }


void FXWindow::setShape(FXBitmap* bitmap){
  if(!bitmap || !bitmap->id()){ fxerror("%s::setShape: illegal bitmap specified.\n",getClassName()); }
  }


void FXWindow::setShape(FXIcon* icon){
  if(!icon || !icon->shape){ fxerror("%s::setShape: illegal icon specified.\n",getClassName()); }
  }


void FXWindow::clearShape(){
  }


// Get pointer location (in window coordinates).  Wayland has no query, so the
// answer is the position the seat last reported, turned from the coordinates of
// the surface it is over into the coordinates of this window.  The tool tip
// needs it: it places itself at the pointer.
FXbool FXWindow::getCursorPosition(FXint& x,FXint& y,FXuint& buttons) const {
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  FXRectangle here;
  FXRectangle there;
  x=0;
  y=0;
  buttons=0;
  if(!app || !app->pointersurface) return false;
  here=screenRect(app,this);
  there=app->childmap.getGlobalGeometry(app->pointersurface->window);
  x=there.x+app->pointerx-here.x;
  y=there.y+app->pointery-here.y;
  buttons=app->pointerbuttons;
  return true;
  }


// Set pointer location; warping is not possible on this platform
FXbool FXWindow::setCursorPosition(FXint,FXint){
  return false;
  }


// Get coordinates from another window
void FXWindow::translateCoordinatesFrom(FXint& tox,FXint& toy,const FXWindow* fromwindow,FXint fromx,FXint fromy) const {
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  FXRectangle from,to;
  if(!fromwindow){ fxerror("%s::translateCoordinatesFrom: from-window is NULL.\n",getClassName()); }
  from=screenRect(app,fromwindow);
  to=screenRect(app,this);
  tox=from.x+fromx-to.x;
  toy=from.y+fromy-to.y;
  }


// Get coordinates to another window
void FXWindow::translateCoordinatesTo(FXint& tox,FXint& toy,const FXWindow* towindow,FXint fromx,FXint fromy) const {
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  FXRectangle from,to;
  if(!towindow){ fxerror("%s::translateCoordinatesTo: to-window is NULL.\n",getClassName()); }
  from=screenRect(app,this);
  to=screenRect(app,towindow);
  tox=from.x+fromx-to.x;
  toy=from.y+fromy-to.y;
  }


/*******************************************************************************/

// Update dirty rectangle
void FXWindow::update(FXint x,FXint y,FXint w,FXint h) const {
  if(xid){

    // We toss out rectangles outside the visible area
    if(x>=width || y>=height || x+w<=0 || y+h<=0) return;

    // Intersect with the window
    if(x<0){w+=x;x=0;}
    if(y<0){h+=y;y=0;}
    if(x+w>width){w=width-x;}
    if(y+h>height){h=height-y;}

    // Append the rectangle; the event loop turns it into an expose, which is
    // the only way a window gets painted on this platform
    if(w>0 && h>0){
      getApp()->addRepaint(xid,x,y,w,h,true);
      }
    }
  }


// If marked but not yet painted, paint the given area
void FXWindow::repaint(FXint x,FXint y,FXint w,FXint h) const {
  if(xid){

    // We toss out rectangles outside the visible area
    if(x>=width || y>=height || x+w<=0 || y+h<=0) return;

    // Intersect with the window
    if(x<0){w+=x;x=0;}
    if(y<0){h+=y;y=0;}
    if(x+w>width){w=width-x;}
    if(y+h>height){h=height-y;}

    if(w>0 && h>0){
      getApp()->removeRepaints(xid,x,y,w,h);
      }
    }
  }


/*******************************************************************************/

// Acquire grab; the pointer stays with the surface that got the press
void FXWindow::grab(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::grab %p\n",getClassName(),this);
    if(!shown()){ fxwarning("%s::grab: Window is not visible.\n",getClassName()); }
    getApp()->mouseGrabWindow=this;
    }
  }


// Release grab
void FXWindow::ungrab(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::ungrab %p\n",getClassName(),this);
    getApp()->mouseGrabWindow=nullptr;
    }
  }


// Grab keyboard device
void FXWindow::grabKeyboard(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::grabKeyboard %p\n",getClassName(),this);
    if(!shown()){ fxwarning("%s::grabKeyboard: Window is not visible.\n",getClassName()); }
    getApp()->keyboardGrabWindow=this;
    }
  }


// Ungrab keyboard device
void FXWindow::ungrabKeyboard(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::ungrabKeyboard %p\n",getClassName(),this);
    getApp()->keyboardGrabWindow=nullptr;
    }
  }


// Show or hide the cursor; the cursor of a surface is set when the pointer
// enters it, so this only records the state
void FXWindow::showCursor(FXbool flag){
  if(!cursorShown() && flag){
    flags|=FLAG_CURSOR;
    }
  else if(cursorShown() && !flag){
    flags&=~FLAG_CURSOR;
    }
  }


// Set cursor
void FXWindow::setDefaultCursor(FXCursor* cur){
  if(defaultCursor!=cur){
    if(!cur){ fxerror("%s::setDefaultCursor: NULL cursor argument.\n",getClassName()); }
    defaultCursor=cur;
    }
  }


// Set drag cursor
void FXWindow::setDragCursor(FXCursor* cur){
  if(dragCursor!=cur){
    if(!cur){ fxerror("%s::setDragCursor: NULL cursor argument.\n",getClassName()); }
    dragCursor=cur;
    }
  }


// Enable the window
void FXWindow::enable(){
  if(!isEnabled()){
    flags|=FLAG_ENABLED;
    }
  }


// Disable the window
void FXWindow::disable(){
  killFocus();
  if(isEnabled()){
    flags&=~FLAG_ENABLED;
    if(xid){
      if(getApp()->mouseGrabWindow==this){
        getApp()->mouseGrabWindow=nullptr;
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        }
      if(getApp()->keyboardGrabWindow==this){
        getApp()->keyboardGrabWindow=nullptr;
        }
      }
    }
  }

// Start dragging; the compositor takes it from here and the target tells us what
// it does with the data
FXbool FXWindow::beginDrag(const FXDragType* types,FXuint numtypes){
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  if(xid==0){ fxerror("%s::beginDrag: window has not yet been created.\n",getClassName()); }
  if(!isDragging()){
    if(!types || numtypes<1){ fxerror("%s::beginDrag: should have at least one type to drag.\n",getClassName()); }
    if(!fxwaylandStartDrag(app,xid,types,numtypes)) return false;
    resizeElms(getApp()->xdndTypeList,numtypes);
    copyElms(getApp()->xdndTypeList,types,numtypes);
    getApp()->xdndNumTypes=numtypes;
    getApp()->ddeAction=DRAG_COPY;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->dragWindow=this;
    return true;
    }
  return false;
  }


// End the drag and report what the target did with it
FXDragAction FXWindow::endDrag(FXbool drop){
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  FXDragAction action=DRAG_REJECT;
  if(xid==0){ fxerror("%s::endDrag: window has not yet been created.\n",getClassName()); }
  if(isDragging()){
    action=(FXDragAction)fxwaylandEndDrag(app,xid,drop);
    freeElms(getApp()->xdndTypeList);
    getApp()->xdndNumTypes=0;
    getApp()->ddeAction=DRAG_COPY;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->dragWindow=nullptr;
    }
  return action;
  }


// Acquire the PRIMARY selection; the data is offered through the data source of
// the connection, which asks this window when a peer wants it
FXbool FXWindow::acquireSelection(const FXDragType* types,FXuint numtypes){
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  if(!types || !numtypes){ fxerror("%s::acquireSelection: should have at least one type to select.\n",getClassName()); }
  if(getApp()->selectionWindow){
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_LOST,0),&getApp()->event);
    getApp()->selectionWindow=nullptr;
    freeElms(getApp()->xselTypeList);
    getApp()->xselNumTypes=0;
    }
  if(!fxwaylandAcquireSelection(app,xid,types,numtypes)) return false;
  getApp()->selectionWindow=this;
  resizeElms(getApp()->xselTypeList,numtypes);
  copyElms(getApp()->xselTypeList,types,numtypes);
  getApp()->xselNumTypes=numtypes;
  getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_GAINED,0),&getApp()->event);
  return true;
  }


// Give up the PRIMARY selection
FXbool FXWindow::releaseSelection(){
  if(getApp()->selectionWindow==this){
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_LOST,0),&getApp()->event);
    getApp()->selectionWindow=nullptr;
    freeElms(getApp()->xselTypeList);
    getApp()->xselNumTypes=0;
    fxwaylandReleaseSelection(fxwayland(getApp()->getDisplay()),xid);
    return true;
    }
  return false;
  }


// Acquire the clipboard
FXbool FXWindow::acquireClipboard(const FXDragType* types,FXuint numtypes){
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  if(!types || !numtypes){ fxerror("%s::acquireClipboard: should have at least one type to select.\n",getClassName()); }
  if(getApp()->clipboardWindow){
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_LOST,0),&getApp()->event);
    getApp()->clipboardWindow=nullptr;
    freeElms(getApp()->xcbTypeList);
    getApp()->xcbNumTypes=0;
    }
  if(!fxwaylandAcquireClipboard(app,xid,types,numtypes)) return false;
  getApp()->clipboardWindow=this;
  resizeElms(getApp()->xcbTypeList,numtypes);
  copyElms(getApp()->xcbTypeList,types,numtypes);
  getApp()->xcbNumTypes=numtypes;
  getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_GAINED,0),&getApp()->event);
  return true;
  }


// Give up the clipboard
FXbool FXWindow::releaseClipboard(){
  if(getApp()->clipboardWindow==this){
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_LOST,0),&getApp()->event);
    getApp()->clipboardWindow=nullptr;
    freeElms(getApp()->xcbTypeList);
    getApp()->xcbNumTypes=0;
    fxwaylandReleaseClipboard(fxwayland(getApp()->getDisplay()),xid);
    return true;
    }
  return false;
  }

}

#endif
