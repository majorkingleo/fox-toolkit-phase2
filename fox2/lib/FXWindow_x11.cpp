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

#if !defined(WIN32)

#define TOPIC_DETAIL    1002

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
