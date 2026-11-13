/********************************************************************************
*                                                                               *
*                  W i n d o w   O b j e c t   ( X 1 1 )                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fx.h"
#include "FXVisual.h"
#include "fxplatformwindow.h"
#include "fxpriv.h"

#if defined(FOX_X11)

#define TOPIC_DETAIL    1002
#define TOPIC_EVENT     1011

/*
  Notes:

  - X11 window creation, attaching and destruction.  A Wayland adapter would
    replace this file rather than extend it.
*/


namespace FX {


// Add a window to the WM_COLORMAP_WINDOWS property of its shell
static void fxplatformAddColormapWindow(Display* display,FXID shell,FXID window){
  Window windows[2],*windowsReturn,*windowList;
  int countReturn,i;

  // Check to see if there is already a property
  Status status=XGetWMColormapWindows(display,shell,&windowsReturn,&countReturn);

  // If no property, just create one
  if(!status){
    windows[0]=window;
    windows[1]=shell;
    XSetWMColormapWindows(display,shell,windows,2);
    }

  // There was a property, add myself to the beginning
  else{
    windowList=(Window*)malloc((sizeof(Window))*(countReturn+1));
    windowList[0]=window;
    for(i=0; i<countReturn; i++) windowList[i+1]=windowsReturn[i];
    XSetWMColormapWindows(display,shell,windowList,countReturn+1);
    XFree((char*)windowsReturn);
    free(windowList);
    }
  }


// Remove a window from the WM_COLORMAP_WINDOWS property of its shell
static void fxplatformRemoveColormapWindow(Display* display,FXID shell,FXID window){
  Window *windowsReturn;
  int countReturn,i;
  Status status=XGetWMColormapWindows(display,shell,&windowsReturn,&countReturn);
  if(status){
    for(i=0; i<countReturn; i++){
      if(windowsReturn[i]==window){
        for(i++; i<countReturn; i++) windowsReturn[i-1]=windowsReturn[i];
        XSetWMColormapWindows(display,shell,windowsReturn,countReturn-1);
        break;
        }
      }
    XFree((char*)windowsReturn);
    }
  }


// Create the native window
FXID fxplatformCreateWindow(const FXWindowCreateInfo& info){
  Display* display=(Display*)info.display;
  XSetWindowAttributes wattr;
  unsigned long mask=(CWBackPixmap|CWWinGravity|CWBitGravity|CWBorderPixel|CWEventMask|CWDontPropagate|CWOverrideRedirect|CWSaveUnder|CWColormap);
  Window window;
  FXID shell;

  // Events for normal windows
  wattr.event_mask=BASIC_EVENT_MASK;

  // Events for shell windows
  if(info.flags&WINDOWINFO_SHELL) wattr.event_mask|=SHELL_EVENT_MASK;

  // If enabled, turn on some more events
  if(info.flags&WINDOWINFO_ENABLED) wattr.event_mask|=ENABLED_EVENT_MASK;

  // FOX will not propagate events to ancestor windows
  wattr.do_not_propagate_mask=NOT_PROPAGATE_MASK;

  // Obtain colormap
  wattr.colormap=info.colormap;

  // This is needed for OpenGL
  wattr.border_pixmap=None;
  wattr.border_pixel=0;

  // Background
  wattr.background_pixmap=None;
  wattr.background_pixel=0;

  // Preserving content during resize will be faster:- not turned
  // on yet as we will have to recode all widgets to decide when to
  // repaint or not to repaint the display when resized...
  wattr.bit_gravity=ForgetGravity;

  // The window gravity is NorthWestGravity, which means
  // if a child keeps same position relative to top/left
  // of its parent window, nothing extra work is incurred.
  wattr.win_gravity=NorthWestGravity;

  // Determine override redirect
  wattr.override_redirect=(info.flags&WINDOWINFO_OVERRIDEREDIRECT)?True:False;

  // Backing store/planes/pixel not passed
  wattr.backing_store=0;
  wattr.backing_planes=0;
  wattr.backing_pixel=0;

  // Determine save-unders
  wattr.save_under=(info.flags&WINDOWINFO_SAVEUNDER)?True:False;

  // Set cursor
  if(info.cursor){
    wattr.cursor=info.cursor;
    mask|=CWCursor;
    }

  FXSTATIC_ASSERT(sizeof(FXID)>=sizeof(Window));

  // Finally, create the window
  window=XCreateWindow(display,info.parent,info.x,info.y,FXMAX(info.w,1),FXMAX(info.h,1),0,info.visual->getDepth(),InputOutput,(Visual*)info.visual->getVisual(),mask,&wattr);

  // Uh-oh, we failed
  if(!window) return 0;

  // We put the XdndAware property on all toplevel windows, so that
  // when dragging, we need to search no further than the toplevel window.
  if(info.flags&WINDOWINFO_SHELL){
    Atom propdata=(Atom)XDND_PROTOCOL_VERSION;
    XChangeProperty(display,window,info.xdndaware,XA_ATOM,32,PropModeReplace,(unsigned char*)&propdata,1);
    }

  // If window is a shell and it has an owner, make it stay on top of the owner
  if((info.flags&WINDOWINFO_SHELL) && info.ownershell){
    XSetTransientForHint(display,window,info.ownershell);
    }

  // If colormap different, set WM_COLORMAP_WINDOWS property properly
  if(info.colormap!=DefaultColormap(display,DefaultScreen(display))){
    shell=(info.flags&WINDOWINFO_SHELL)?(FXID)window:info.shell;
    fxplatformAddColormapWindow(display,shell,(FXID)window);
    }

  // Show if it was supposed to be
  if((info.flags&WINDOWINFO_SHOWN) && 0<info.w && 0<info.h) XMapWindow(display,window);

  return (FXID)window;
  }


// Take over a window belonging to another application
void fxplatformAttachWindow(const FXWindowCreateInfo& info,FXID window){
  Display* display=(Display*)info.display;
  XSetWindowAttributes wattr;
  unsigned long mask=(CWEventMask|CWDontPropagate);

  // Set event mask
  wattr.event_mask=BASIC_EVENT_MASK;
  if(info.flags&WINDOWINFO_ENABLED) wattr.event_mask|=ENABLED_EVENT_MASK;

  // Events that should not propagate
  wattr.do_not_propagate_mask=NOT_PROPAGATE_MASK;

  // Set cursor
  if(info.cursor){
    wattr.cursor=info.cursor;
    mask|=CWCursor;
    }

  // Change window attributes
  XChangeWindowAttributes(display,window,mask,&wattr);

  // Reparent window
  XReparentWindow(display,window,info.parent,0,0);
  }


// Destroy a native window we own
void fxplatformDestroyWindow(void* display,FXID window,FXID shell,FXID colormap,FXID xdndaware){
  Display* dpy=(Display*)display;

  // If colormap different, set WM_COLORMAP_WINDOWS property properly
  if(colormap!=DefaultColormap(dpy,DefaultScreen(dpy))){
    fxplatformRemoveColormapWindow(dpy,shell,window);
    }

  // Delete the XdndAware property of a toplevel window
  if(shell==window){
    XDeleteProperty(dpy,window,xdndaware);
    }

  // Delete the window
  XDestroyWindow(dpy,window);
  }


// X11 has no device context handles
FXID FXWindow::GetDC() const {
  return 0;
  }


// Nothing to release
int FXWindow::ReleaseDC(FXID) const {
  return 0;
  }


// X11 has no window classes
const void* FXWindow::GetClass() const {
  return nullptr;
  }


// Move, resize or both
void fxplatformPlaceWindow(void* display,FXID window,FXint x,FXint y,FXint w,FXint h,FXint oldw,FXint oldh,FXbool shown,FXuint placement){
  Display* dpy=(Display*)display;
  if(placement&(PLACEMENT_W|PLACEMENT_H)){
    if(0<w && 0<h){
      if(shown && (oldw<=0 || oldh<=0)){
        XMapWindow(dpy,window);
        }
      if(placement&PLACEMENT_X)
        XMoveResizeWindow(dpy,window,x,y,w,h);
      else
        XResizeWindow(dpy,window,w,h);
      }
    else if(0<oldw && 0<oldh){
      XUnmapWindow(dpy,window);
      }
    }
  else if(placement&(PLACEMENT_X|PLACEMENT_Y)){
    XMoveWindow(dpy,window,x,y);
    }
  }


// Map or unmap
void fxplatformShowWindow(void* display,FXID window,FXint w,FXint h,FXbool shown){
  Display* dpy=(Display*)display;
  if(shown){
    if(0<w && 0<h) XMapWindow(dpy,window);
    }
  else{
    XUnmapWindow(dpy,window);
    }
  }


// Reparent under a new parent
void fxplatformReparentWindow(void* display,FXID window,FXID parent){
  Display* dpy=(Display*)display;
  // See remarks in FXToolBarGrip
  XReparentWindow(dpy,window,parent,0,0);
  XFlush(dpy);
  }


// Raise or lower in the stacking order
void fxplatformRaiseWindow(void* display,FXID window,FXbool raised){
  Display* dpy=(Display*)display;
  if(raised)
    XRaiseWindow(dpy,window);
  else
    XLowerWindow(dpy,window);
  }


// Release the mouse grab
void fxplatformReleaseMouseGrab(void* display,FXID){
  Display* dpy=(Display*)display;
  XUngrabPointer(dpy,CurrentTime);
  XFlush(dpy);
  }


// Release the keyboard grab
void fxplatformReleaseKeyboardGrab(void* display,FXTime time){
  Display* dpy=(Display*)display;
  XUngrabKeyboard(dpy,time);
  XFlush(dpy);
  }


// Set window shape by means of region
void FXWindow::setShape(const FXRegion& region){
  if(xid){
#if defined(HAVE_XSHAPE_H)
    XShapeCombineRegion((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,(Region)region.region,ShapeSet);
#endif
    }
  }


// Set window shape by means of bitmap
void FXWindow::setShape(FXBitmap* bitmap){
  if(!bitmap || !bitmap->id()){ fxerror("%s::setShape: illegal bitmap specified.\n",getClassName()); }
  if(xid){
#if defined(HAVE_XSHAPE_H)
    XShapeCombineMask((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,bitmap->id(),ShapeSet);
#endif
    }
  }


// Set window shape by means of bitmap
void FXWindow::setShape(FXIcon* icon){
  if(!icon || !icon->shape){ fxerror("%s::setShape: illegal icon specified.\n",getClassName()); }
  if(xid){
#if defined(HAVE_XSHAPE_H)
    XShapeCombineMask((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,icon->shape,ShapeSet);
#endif
    }
  }


// Clear window shape to default rectangle
void FXWindow::clearShape(){
  if(xid){
#if defined(HAVE_XSHAPE_H)
    XShapeCombineMask((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,None,ShapeSet);
#endif
    }
  }


// Get pointer location (in window coordinates)
FXbool FXWindow::getCursorPosition(FXint& x,FXint& y,FXuint& buttons) const {
  if(xid){
    Window dum; int rx,ry;
    return XQueryPointer((Display*)getApp()->getDisplay(),xid,&dum,&dum,&rx,&ry,&x,&y,&buttons);
    }
  return false;
  }


// Set pointer location (in window coordinates)
// Contributed by David Heath <dave@hipgraphics.com>
FXbool FXWindow::setCursorPosition(FXint x,FXint y){
  if(xid){
    XWarpPointer((Display*)getApp()->getDisplay(),None,xid,0,0,0,0,x,y);
    return true;
    }
  return false;
  }


// Get coordinates from another window (for symmetry)
void FXWindow::translateCoordinatesFrom(FXint& tox,FXint& toy,const FXWindow* fromwindow,FXint fromx,FXint fromy) const {
  if(!fromwindow){ fxerror("%s::translateCoordinatesFrom: from-window is NULL.\n",getClassName()); }
  if(xid && fromwindow->id()){
    Window tmp;
    XTranslateCoordinates((Display*)getApp()->getDisplay(),fromwindow->id(),xid,fromx,fromy,&tox,&toy,&tmp);
    }
  }


// Get coordinates to another window (for symmetry)
void FXWindow::translateCoordinatesTo(FXint& tox,FXint& toy,const FXWindow* towindow,FXint fromx,FXint fromy) const {
  if(!towindow){ fxerror("%s::translateCoordinatesTo: to-window is NULL.\n",getClassName()); }
  if(xid && towindow->id()){
    Window tmp;
    XTranslateCoordinates((Display*)getApp()->getDisplay(),xid,towindow->id(),fromx,fromy,&tox,&toy,&tmp);
    }
  }


// Acquire the selection.
// We always generate SEL_SELECTION_LOST and SEL_SELECTION_GAINED
// because we assume the selection types may have changed, and want
// to give target opportunity to allocate the new data for these types.
FXbool FXWindow::acquireSelection(const FXDragType *types,FXuint numtypes){
  if(!types || !numtypes){ fxerror("%s::acquireSelection: should have at least one type to select.\n",getClassName()); }
  if(getApp()->selectionWindow){
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_LOST,0),&getApp()->event);
    getApp()->selectionWindow=nullptr;
    freeElms(getApp()->xselTypeList);
    getApp()->xselNumTypes=0;
    }
  if(xid){
    XSetSelectionOwner((Display*)getApp()->getDisplay(),XA_PRIMARY,xid,getApp()->event.time);
    if(XGetSelectionOwner((Display*)getApp()->getDisplay(),XA_PRIMARY)!=xid) return false;
    }
  if(!getApp()->selectionWindow){
    getApp()->selectionWindow=this;
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_GAINED,0),&getApp()->event);
    resizeElms(getApp()->xselTypeList,numtypes);
    copyElms(getApp()->xselTypeList,types,numtypes);
    getApp()->xselNumTypes=numtypes;
    }
  return true;
  }


// Release the selection
FXbool FXWindow::releaseSelection(){
  if(getApp()->selectionWindow==this){
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_LOST,0),&getApp()->event);
    getApp()->selectionWindow=nullptr;
    freeElms(getApp()->xselTypeList);
    getApp()->xselNumTypes=0;
    if(xid){
      XSetSelectionOwner((Display*)getApp()->getDisplay(),XA_PRIMARY,None,getApp()->event.time);
      }
    return true;
    }
  return false;
  }


// Acquire the clipboard
// We always generate SEL_CLIPBOARD_LOST and SEL_CLIPBOARD_GAINED
// because we assume the clipboard types may have changed, and want
// to give target opportunity to allocate the new data for these types.
FXbool FXWindow::acquireClipboard(const FXDragType *types,FXuint numtypes){
  if(!types || !numtypes){ fxerror("%s::acquireClipboard: should have at least one type to select.\n",getClassName()); }
  if(getApp()->clipboardWindow){
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_LOST,0),&getApp()->event);
    getApp()->clipboardWindow=nullptr;
    freeElms(getApp()->xcbTypeList);
    getApp()->xcbNumTypes=0;
    }
  if(xid){
    XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xcbSelection,xid,getApp()->event.time);
    if(XGetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xcbSelection)!=xid) return false;
    }
  if(!getApp()->clipboardWindow){
    getApp()->clipboardWindow=this;
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_GAINED,0),&getApp()->event);
    resizeElms(getApp()->xcbTypeList,numtypes);
    copyElms(getApp()->xcbTypeList,types,numtypes);
    getApp()->xcbNumTypes=numtypes;
    }
  return true;
  }


// Release the clipboard
FXbool FXWindow::releaseClipboard(){
  if(getApp()->clipboardWindow==this){
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_LOST,0),&getApp()->event);
    getApp()->clipboardWindow=nullptr;
    freeElms(getApp()->xcbTypeList);
    getApp()->xcbNumTypes=0;
    if(xid){
      XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xcbSelection,None,getApp()->event.time);
      }
    return true;
    }
  return false;
  }


// Set drag rectangle to block events while inside
void FXWindow::setDragRectangle(FXint x,FXint y,FXint w,FXint h,FXbool wantupdates) const {
  if(xid==0){ fxerror("%s::setDragRectangle: window has not yet been created.\n",getClassName()); }
  Window tmp;
  int tox,toy;
  XTranslateCoordinates((Display*)getApp()->getDisplay(),xid,XDefaultRootWindow((Display*)getApp()->getDisplay()),x,y,&tox,&toy,&tmp);
  getApp()->xdndRect.x=tox;
  getApp()->xdndRect.y=toy;
  getApp()->xdndWantUpdates=wantupdates;
  getApp()->xdndRect.w=w;
  getApp()->xdndRect.h=h;
  }


// Clear drag rectangle
void FXWindow::clearDragRectangle() const {
  if(xid==0){ fxerror("%s::clearDragRectangle: window has not yet been created.\n",getClassName()); }
  getApp()->xdndRect.x=0;
  getApp()->xdndRect.y=0;
  getApp()->xdndRect.w=0;
  getApp()->xdndRect.h=0;
  getApp()->xdndWantUpdates=true; // Isn't this bullshit?
  }


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

    // Append the rectangle; it is a synthetic expose event!!
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


// Scroll rectangle x,y,w,h by a shift of dx,dy
void FXWindow::scroll(FXint x,FXint y,FXint w,FXint h,FXint dx,FXint dy) const {
  if(xid && 0<w && 0<h && (dx || dy)){

    // No overlap:- repaint the whole thing
    if((w<=FXABS(dx)) || (h<=FXABS(dy))){
      getApp()->addRepaint(xid,x,y,w,h,true);
      }

    // Has overlap, so blit contents and repaint the exposed parts
    else{
      FXint  tx,ty,fx,fy,ex,ey,ew,eh;
      XEvent event;

      // Force server to catch up
      XSync((Display*)getApp()->getDisplay(),False);

      // Pull any outstanding repaint events into our own repaint rectangle list
      while(XCheckWindowEvent((Display*)getApp()->getDisplay(),xid,ExposureMask,&event)){
        if(event.xany.type==NoExpose) continue;
        getApp()->addRepaint(xid,event.xexpose.x,event.xexpose.y,event.xexpose.width,event.xexpose.height,false);
        if(event.xgraphicsexpose.count==0) break;
        }

      // Scroll all repaint rectangles of this window by the dx,dy
      getApp()->scrollRepaints(xid,dx,dy);

      // Compute blitted area
      if(dx>0){             // Content shifted right
        fx=x;
        tx=x+dx;
        ex=x;
        ew=dx;
        }
      else{                 // Content shifted left
        fx=x-dx;
        tx=x;
        ex=x+w+dx;
        ew=-dx;
        }
      if(dy>0){             // Content shifted down
        fy=y;
        ty=y+dy;
        ey=y;
        eh=dy;
        }
      else{                 // Content shifted up
        fy=y-dy;
        ty=y;
        ey=y+h+dy;
        eh=-dy;
        }

      // BLIT the contents
      XCopyArea((Display*)getApp()->getDisplay(),xid,xid,(GC)visual->scrollgc,fx,fy,w-ew,h-eh,tx,ty);

      // Post additional rectangles for the uncovered areas
      if(dy){
        getApp()->addRepaint(xid,x,ey,w,eh,true);
        }
      if(dx){
        getApp()->addRepaint(xid,ex,y,ew,h,true);
        }
      }
    }
  }


// Sent by the drop target
void FXWindow::dropFinished(FXDragAction action) const {
  if(!getApp()->xdndFinishSent){
    if(action==DRAG_ACCEPT) action=getApp()->ansAction;
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=(Display*)getApp()->getDisplay();
    se.xclient.message_type=getApp()->xdndFinished;
    se.xclient.format=32;
    se.xclient.window=getApp()->xdndSource;
    se.xclient.data.l[0]=xid;
    se.xclient.data.l[1]=(action==DRAG_REJECT)?0:1;
    se.xclient.data.l[2]=getApp()->xdndActionList[action];
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndSource,True,NoEventMask,&se);
    XFlush((Display*)getApp()->getDisplay());
    getApp()->xdndFinishSent=true;
    }
  }




// Start a drag on the types mentioned
FXbool FXWindow::beginDrag(const FXDragType *types,FXuint numtypes){
  if(xid==0){ fxerror("%s::beginDrag: window has not yet been created.\n",getClassName()); }
  if(!isDragging()){
    if(!types || numtypes<1){ fxerror("%s::beginDrag: should have at least one type to drag.\n",getClassName()); }
    XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xdndSelection,xid,getApp()->event.time);
    if(XGetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xdndSelection)!=xid){
      fxwarning("%s::beginDrag: failed to acquire DND selection.\n",getClassName());
      return false;
      }
    resizeElms(getApp()->xdndTypeList,numtypes);
    copyElms(getApp()->xdndTypeList,types,numtypes);
    getApp()->xdndNumTypes=numtypes;
    XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndTypes,XA_ATOM,32,PropModeReplace,(unsigned char*)getApp()->xdndTypeList,getApp()->xdndNumTypes);
    XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndActions,XA_ATOM,32,PropModeReplace,(unsigned char*)getApp()->xdndActionList,6);
    getApp()->xdndTarget=0;
    getApp()->xdndProxyTarget=0;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->xdndStatusPending=false;
    getApp()->xdndStatusReceived=false;
    getApp()->xdndWantUpdates=true;
    getApp()->xdndRect.x=0;
    getApp()->xdndRect.y=0;
    getApp()->xdndRect.w=0;
    getApp()->xdndRect.h=0;
    getApp()->dragWindow=this;
    return true;
    }
  return false;
  }


// Drag to new position
FXbool FXWindow::handleDrag(FXint x,FXint y,FXDragAction action){
  if(xid==0){ fxerror("%s::handleDrag: window has not yet been created.\n",getClassName()); }
  if(action<DRAG_ASK || DRAG_PRIVATE<action){ fxerror("%s::handleDrag: illegal drag action.\n",getClassName()); }
  if(isDragging()){

    Window proxywindow,window,child,win,proxywin,root;
    unsigned long ni,ba;
    unsigned char *ptr1;
    unsigned char *ptr2;
    unsigned char *ptr3;
    Atom    typ;
    int     fmt;
    Atom    version;
    int     dropx;
    int     dropy;
    XEvent  se;
    FXbool    forcepos=false;

    // Find XDND aware window at the indicated location
    root=XDefaultRootWindow((Display*)getApp()->getDisplay());
    window=0;
    proxywindow=0;
    version=0;
    win=root;
    while(1){
      if(!XTranslateCoordinates((Display*)getApp()->getDisplay(),root,win,x,y,&dropx,&dropy,&child)) break;
      proxywin=win;
      if(XGetWindowProperty((Display*)getApp()->getDisplay(),win,getApp()->xdndProxy,0,1,False,AnyPropertyType,&typ,&fmt,&ni,&ba,&ptr1)==Success){
        if(typ==XA_WINDOW && fmt==32 && ni>0){
          if(XGetWindowProperty((Display*)getApp()->getDisplay(),*((Window*)ptr1),getApp()->xdndProxy,0,1,False,AnyPropertyType,&typ,&fmt,&ni,&ba,&ptr2)==Success){
            if(typ==XA_WINDOW && fmt==32 && ni>0 && *((Window*)ptr2) == *((Window*)ptr1)){
              proxywin=*((Window*)ptr1);
              }
            XFree(ptr2);
            }
          }
        XFree(ptr1);
        }
      if(XGetWindowProperty((Display*)getApp()->getDisplay(),proxywin,getApp()->xdndAware,0,1,False,AnyPropertyType,&typ,&fmt,&ni,&ba,&ptr3)==Success){
        if(typ==XA_ATOM && fmt==32 && ni>0 && *((Atom*)ptr3)>=3){
          window=win;
          proxywindow=proxywin;
          version=FXMIN(*((Atom*)ptr3),XDND_PROTOCOL_VERSION);
          if(window!=root){XFree(ptr3);break;}
          }
        XFree(ptr3);
        }
      if(child==None) break;
      win=child;
      }

    // Changed windows
    if(window!=getApp()->xdndTarget){

      // Moving OUT of XDND aware window?
      if(getApp()->xdndTarget!=0){
        se.xclient.type=ClientMessage;
        se.xclient.display=(Display*)getApp()->getDisplay();
        se.xclient.message_type=getApp()->xdndLeave;
        se.xclient.format=32;
        se.xclient.window=getApp()->xdndTarget;
        se.xclient.data.l[0]=xid;
        se.xclient.data.l[1]=0;
        se.xclient.data.l[2]=0;
        se.xclient.data.l[3]=0;
        se.xclient.data.l[4]=0;
        XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
        }

      // Reset XDND variables
      getApp()->xdndTarget=window;
      getApp()->xdndProxyTarget=proxywindow;
      getApp()->ansAction=DRAG_REJECT;
      getApp()->xdndStatusPending=false;
      getApp()->xdndStatusReceived=false;
      getApp()->xdndWantUpdates=true;
      getApp()->xdndRect.x=x;
      getApp()->xdndRect.y=y;
      getApp()->xdndRect.w=1;
      getApp()->xdndRect.h=1;

      // Moving INTO XDND aware window?
      if(getApp()->xdndTarget!=0){
        se.xclient.type=ClientMessage;
        se.xclient.display=(Display*)getApp()->getDisplay();
        se.xclient.message_type=getApp()->xdndEnter;
        se.xclient.format=32;
        se.xclient.window=getApp()->xdndTarget;
        se.xclient.data.l[0]=xid;
        se.xclient.data.l[1]=version<<24;
        se.xclient.data.l[2]=getApp()->xdndNumTypes>=1 ? getApp()->xdndTypeList[0] : None;
        se.xclient.data.l[3]=getApp()->xdndNumTypes>=2 ? getApp()->xdndTypeList[1] : None;
        se.xclient.data.l[4]=getApp()->xdndNumTypes>=3 ? getApp()->xdndTypeList[2] : None;
        if(getApp()->xdndNumTypes>3) se.xclient.data.l[1]|=1;
        XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
        forcepos=true;
        }
      }

    // Now send a position message
    if(getApp()->xdndTarget!=0){

      // Send position if we're outside the mouse box or ignoring it
      if(forcepos || getApp()->xdndRect.w==0 || getApp()->xdndRect.h==0 || getApp()->xdndWantUpdates || x<getApp()->xdndRect.x || y<getApp()->xdndRect.y || getApp()->xdndRect.x+getApp()->xdndRect.w<=x || getApp()->xdndRect.y+getApp()->xdndRect.h<=y){

        // No outstanding status message, so send new pos right away
        if(!getApp()->xdndStatusPending){
          se.xclient.type=ClientMessage;
          se.xclient.display=(Display*)getApp()->getDisplay();
          se.xclient.message_type=getApp()->xdndPosition;
          se.xclient.format=32;
          se.xclient.window=getApp()->xdndTarget;
          se.xclient.data.l[0]=xid;
          se.xclient.data.l[1]=0;
          se.xclient.data.l[2]=MKUINT(y,x);                               // Coordinates
          se.xclient.data.l[3]=getApp()->event.time;                      // Time stamp
          se.xclient.data.l[4]=getApp()->xdndActionList[action];
          XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
          getApp()->xdndStatusPending=true;       // Waiting for the other app to respond
          }
        }
      }

    return true;
    }
  return false;
  }



struct XDNDMatch {
  Atom xdndStatus;
  Atom xdndPosition;
  Atom xdndFinished;
  Atom xdndDrop;
  Atom xdndEnter;
  Atom xdndLeave;
  };


// Scan event queue for XDND messages or selection requests
static Bool matchxdnd(Display*,XEvent* event,XPointer ptr){
  XDNDMatch *m=(XDNDMatch*)ptr;
  return event->type==SelectionRequest || (event->type==ClientMessage && (event->xclient.message_type==m->xdndStatus || event->xclient.message_type==m->xdndPosition || event->xclient.message_type==m->xdndFinished || event->xclient.message_type==m->xdndDrop || event->xclient.message_type==m->xdndEnter || event->xclient.message_type==m->xdndLeave));
  }



// Terminate the drag; if drop flag is false, don't drop even if accepted.
FXDragAction FXWindow::endDrag(FXbool drop){
  FXDragAction action=DRAG_REJECT;
  if(xid==0){ fxerror("%s::endDrag: window has not yet been created.\n",getClassName()); }
  if(isDragging()){

    FXbool nodrop=true;
    XEvent se;
    FXuint loops;
    XDNDMatch match;

    // Fill up match struct
    match.xdndStatus=getApp()->xdndStatus;
    match.xdndPosition=getApp()->xdndPosition;
    match.xdndFinished=getApp()->xdndFinished;
    match.xdndDrop=getApp()->xdndDrop;
    match.xdndEnter=getApp()->xdndEnter;
    match.xdndLeave=getApp()->xdndLeave;

    // Ever received a status
    if(getApp()->xdndStatusReceived && drop){

      // If a status message is still pending, wait for it
      if(getApp()->xdndStatusPending){
        loops=1000;
        do{
          FXTRACE(TOPIC_EVENT,"Waiting for pending XdndStatus\n");
          if(XCheckIfEvent((Display*)getApp()->getDisplay(),&se,matchxdnd,(char*)&match)){
            getApp()->dispatchEvent(se);

            // We got the status update, finally
            if(se.xclient.type==ClientMessage && se.xclient.message_type==getApp()->xdndStatus){
              FXTRACE(TOPIC_EVENT,"Got XdndStatus\n");
              getApp()->xdndStatusPending=false;
              break;
              }

            // We got a selection request message, so there is still hope that
            // the target is functioning; lets give it a bit more time...
            if(se.xselection.type==SelectionRequest && se.xselectionrequest.selection==getApp()->xdndSelection){
              FXTRACE(TOPIC_EVENT,"Got SelectionRequest\n");
              loops=1000;
              }
            }
          FXThread::sleep(10000000);
          }
        while(--loops);
        }

      // Got our status message
      if(!getApp()->xdndStatusPending && getApp()->ansAction!=DRAG_REJECT){

        FXTRACE(TOPIC_EVENT,"Sending XdndDrop\n");
        se.xclient.type=ClientMessage;
        se.xclient.display=(Display*)getApp()->getDisplay();
        se.xclient.message_type=getApp()->xdndDrop;
        se.xclient.format=32;
        se.xclient.window=getApp()->xdndTarget;
        se.xclient.data.l[0]=xid;
        se.xclient.data.l[1]=0;
        se.xclient.data.l[2]=getApp()->event.time;
        se.xclient.data.l[3]=0;
        se.xclient.data.l[4]=0;
        XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);

        // Wait until the target has processed the drop; since the
        // target may be us, we keep processing all XDND related messages
        // while we're waiting here.  After some time, we assume the target
        // may have core dumped and we fall out of the loop.....
        loops=1000;
        do{
          FXTRACE(TOPIC_EVENT,"Waiting for XdndFinish\n");
          if(XCheckIfEvent((Display*)getApp()->getDisplay(),&se,matchxdnd,(char*)&match)){
            getApp()->dispatchEvent(se);

            // Got the finish message; we now know the drop has been completed and processed
            if(se.xclient.type==ClientMessage && se.xclient.message_type==getApp()->xdndFinished){
              if(se.xclient.data.l[1]&1){
                action=DRAG_ACCEPT;
                if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_ASK]) action=DRAG_ASK;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_MOVE]) action=DRAG_MOVE;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_COPY]) action=DRAG_COPY;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_LINK]) action=DRAG_LINK;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_PRIVATE]) action=DRAG_PRIVATE;
                }
              FXTRACE(TOPIC_EVENT,"Got XdndFinish action=%u\n",action);
              break;
              }

            // We got a selection request message, so there is still hope that
            // the target is functioning; lets give it a bit more time...
            if(se.xselection.type==SelectionRequest && se.xselectionrequest.selection==getApp()->xdndSelection){
              FXTRACE(TOPIC_EVENT,"Got SelectionRequest\n");
              loops=1000;
              }
            }
          FXThread::sleep(10000000);
          }
        while(--loops);

        // We tried a drop but failed
        nodrop=false;
        }
      }

    // Didn't drop, or didn't get any response, so just send a leave
    if(nodrop){
      FXTRACE(TOPIC_EVENT,"Sending XdndLeave\n");
      se.xclient.type=ClientMessage;
      se.xclient.display=(Display*)getApp()->getDisplay();
      se.xclient.message_type=getApp()->xdndLeave;
      se.xclient.format=32;
      se.xclient.window=getApp()->xdndTarget;
      se.xclient.data.l[0]=xid;
      se.xclient.data.l[1]=0;
      se.xclient.data.l[2]=0;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
      }

    // Clean up
    XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xdndSelection,None,getApp()->event.time);
    XDeleteProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndTypes);
    XDeleteProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndActions);
    freeElms(getApp()->xdndTypeList);
    getApp()->xdndNumTypes=0;
    getApp()->xdndTarget=0;
    getApp()->xdndProxyTarget=0;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->xdndStatusPending=false;
    getApp()->xdndStatusReceived=false;
    getApp()->xdndWantUpdates=true;
    getApp()->xdndRect.x=0;
    getApp()->xdndRect.y=0;
    getApp()->xdndRect.w=0;
    getApp()->xdndRect.h=0;
    getApp()->dragWindow=nullptr;
    }
  return action;
  }


// The string type is predefined and hardwired
const FXDragType FXWindow::stringType=XA_STRING;


// The image type is predefined and hardwired
const FXDragType FXWindow::imageType=XA_PIXMAP;


// The UTF-16 string type is predefined and hardwired
FXDragType FXWindow::utf16Type=0;


// Convert a logical length to device pixels
FXint FXWindow::scale(FXint value) const {
  return value;
  }


// Convert a device length to logical units
FXint FXWindow::unscale(FXint value) const {
  return value;
  }


// Acquire grab; also switches to the drag cursor
void FXWindow::grab(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::grab %p\n",getClassName(),this);
    if(dragCursor->id()==0){ fxerror("%s::grab: Cursor has not been created yet.\n",getClassName()); }
    if(!shown()){ fxwarning("%s::grab: Window is not visible.\n",getClassName()); }
    if(GrabSuccess!=XGrabPointer((Display*)getApp()->getDisplay(),xid,false,GRAB_EVENT_MASK,GrabModeAsync,GrabModeAsync,None,dragCursor->id(),getApp()->event.time)){
      XGrabPointer((Display*)getApp()->getDisplay(),xid,false,GRAB_EVENT_MASK,GrabModeAsync,GrabModeAsync,None,dragCursor->id(),CurrentTime);
      }
    getApp()->mouseGrabWindow=this;
    }
  }


// Release grab; also switches back to the normal cursor
void FXWindow::ungrab(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::ungrab %p\n",getClassName(),this);
    getApp()->mouseGrabWindow=nullptr;
    XUngrabPointer((Display*)getApp()->getDisplay(),getApp()->event.time);
    XFlush((Display*)getApp()->getDisplay());
    }
  }


// Grab keyboard device; note grabbing keyboard generates
// SEL_FOCUSIN on the grab-window and SEL_FOCUSOUT on the old
// focus-window
void FXWindow::grabKeyboard(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::grabKeyboard %p\n",getClassName(),this);
    if(!shown()){ fxwarning("%s::ungrabKeyboard: Window is not visible.\n",getClassName()); }
    XGrabKeyboard((Display*)getApp()->getDisplay(),xid,false,GrabModeAsync,GrabModeAsync,getApp()->event.time);
    getApp()->keyboardGrabWindow=this;
    }
  }


// Ungrab keyboard device; note ungrabbing keyboard generates
// SEL_FOCUSOUT on the grab-window and SEL_FOCUSIN on the
// current focus-window
void FXWindow::ungrabKeyboard(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::ungrabKeyboard %p\n",getClassName(),this);
    getApp()->keyboardGrabWindow=nullptr;
    XUngrabKeyboard((Display*)getApp()->getDisplay(),getApp()->event.time);
    }
  }


// Show or hide the cursor
void FXWindow::showCursor(FXbool flag){
  if(!cursorShown() && flag){
    if(xid){
      XDefineCursor((Display*)getApp()->getDisplay(),xid,defaultCursor->id());
      }
    flags|=FLAG_CURSOR;
    }
  else if(cursorShown() && !flag){
    if(xid){
      XDefineCursor((Display*)getApp()->getDisplay(),xid,getApp()->getDefaultCursor(DEF_BLANK_CURSOR)->id());
      }
    flags&=~FLAG_CURSOR;
    }
  }


// Set cursor
void FXWindow::setDefaultCursor(FXCursor* cur){
  if(defaultCursor!=cur){
    if(!cur){ fxerror("%s::setDefaultCursor: NULL cursor argument.\n",getClassName()); }
    if(xid){
      if(cur->id()==0){ fxerror("%s::setDefaultCursor: Cursor has not been created yet.\n",getClassName()); }
      if(cursorShown()){
        XDefineCursor((Display*)getApp()->getDisplay(),xid,cur->id());
        }
      }
    defaultCursor=cur;
    }
  }


// Set drag cursor
void FXWindow::setDragCursor(FXCursor* cur){
  if(dragCursor!=cur){
    if(!cur){ fxerror("%s::setDragCursor: NULL cursor argument.\n",getClassName()); }
    if(xid){
      if(cur->id()==0){ fxerror("%s::setDragCursor: Cursor has not been created yet.\n",getClassName()); }
      if(grabbed()){
        XChangeActivePointerGrab((Display*)getApp()->getDisplay(),GRAB_EVENT_MASK,cur->id(),CurrentTime);
        }
      }
    dragCursor=cur;
    }
  }


// Enable the window
void FXWindow::enable(){
  if(!isEnabled()){
    flags|=FLAG_ENABLED;
    if(xid){
      FXuint events=BASIC_EVENT_MASK|ENABLED_EVENT_MASK;
      if(isShell()) events|=SHELL_EVENT_MASK;
      XSelectInput((Display*)getApp()->getDisplay(),xid,events);
      }
    }
  }


// Disable the window
void FXWindow::disable(){
  killFocus();
  if(isEnabled()){
    flags&=~FLAG_ENABLED;
    if(xid){
      FXuint events=BASIC_EVENT_MASK;
      if(isShell()) events|=SHELL_EVENT_MASK;
      XSelectInput((Display*)getApp()->getDisplay(),xid,events);
      if(getApp()->mouseGrabWindow==this){
        XUngrabPointer((Display*)getApp()->getDisplay(),CurrentTime);
        XFlush((Display*)getApp()->getDisplay());
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        getApp()->mouseGrabWindow=nullptr;
        }
      if(getApp()->keyboardGrabWindow==this){
        XUngrabKeyboard((Display*)getApp()->getDisplay(),getApp()->event.time);
        XFlush((Display*)getApp()->getDisplay());
        getApp()->keyboardGrabWindow=nullptr;
        }
      }
    }
  }

}

#endif
