/********************************************************************************
*                                                                               *
*                  W i n d o w   O b j e c t   ( W i n 3 2 )                    *
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
#include "fxplatformwindow.h"
#include "fxpriv.h"

#if defined(WIN32)

#define TOPIC_DETAIL    1002

/*
  Notes:

  - Windows window creation, attaching and destruction; one implementation is
    compiled per build, selected by the same conditional as the rest of the
    platform seam.

  - The window class name, instance handle and creation pointer come in through
    FXWindowCreateInfo; the frame style is derived from the shell flags.
*/


namespace FX {


// Create the native window
FXID fxplatformCreateWindow(const FXWindowCreateInfo& info){
  DWORD dwStyle=WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
  DWORD dwExStyle=WS_EX_NOPARENTNOTIFY;
  HWND hParent=(HWND)info.parent;
  HWND hwnd;
  if(info.flags&WINDOWINFO_SHELL){
    if(info.flags&WINDOWINFO_TOPWINDOW){
      dwStyle=WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
      }
    else if(info.flags&WINDOWINFO_OVERRIDEREDIRECT){
      // To control window placement or control decoration, a window manager
      // often needs to redirect map or configure requests. Popup windows, however,
      // often need to be mapped without a window manager getting in the way.
      dwStyle=WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
      dwExStyle=WS_EX_TOOLWINDOW;
      }
    else{
      // Other top-level shell windows (like dialogs)
      dwStyle=WS_POPUP|WS_CAPTION|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
      dwExStyle=WS_EX_DLGMODALFRAME|WS_EX_TOOLWINDOW;
      }
    if(info.owner) hParent=(HWND)info.owner;
    }

  FXSTATIC_ASSERT(sizeof(FXID)>=sizeof(HWND));

  // Create this window
  hwnd=CreateWindowEx(dwExStyle,(TCHAR*)info.classname,nullptr,dwStyle,info.x,info.y,info.w,info.h,hParent,nullptr,(HINSTANCE)info.instance,info.userdata);
  if(!hwnd) return 0;

  // We put the XdndAware property on all toplevel windows, so that
  // when dragging, we need to search no further than the toplevel window.
  if(info.flags&WINDOWINFO_SHELL){
    HANDLE propdata=(HANDLE)XDND_PROTOCOL_VERSION;
    SetProp(hwnd,(LPCTSTR)MAKELONG(info.xdndaware,0),propdata);
    }

  // Show if it was supposed to be.  Apparently, initial state
  // is neither shown nor hidden, so an explicit hide is needed.
  // Patch thanks to "Glenn Shen" <shen@hks.com>
  if(info.flags&WINDOWINFO_SHOWN)
    ShowWindow(hwnd,SW_SHOWNOACTIVATE);
  else
    ShowWindow(hwnd,SW_HIDE);

  return (FXID)hwnd;
  }


// Take over a window belonging to another application
void fxplatformAttachWindow(const FXWindowCreateInfo& info,FXID window){
  // Reparent under what WE think the parent is
  SetParent((HWND)window,(HWND)info.parent);
  }


// Destroy a native window we own
void fxplatformDestroyWindow(void*,FXID window,FXID shell,FXID,FXID xdndaware){
  // Delete the XdndAware property of a toplevel window
  if(shell==window){
    RemoveProp((HWND)window,(LPCTSTR)MAKELONG(xdndaware,0));
    }

  // Zap the window
  DestroyWindow((HWND)window);
  }


// Get this window's device context
FXID FXWindow::GetDC() const {
  return ::GetDC((HWND)xid);
  }


// Release it
int FXWindow::ReleaseDC(FXID hdc) const {
  return ::ReleaseDC((HWND)xid,(HDC)hdc);
  }


// Window class
const void* FXWindow::GetClass() const { return TEXT("FXWindow"); }


// Move, resize or both
void fxplatformPlaceWindow(void*,FXID window,FXint x,FXint y,FXint w,FXint h,FXint,FXint,FXbool,FXuint placement){
  UINT flags=SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER;
  if(!(placement&PLACEMENT_X)) flags|=SWP_NOMOVE;
  if(!(placement&(PLACEMENT_W|PLACEMENT_H))) flags|=SWP_NOSIZE;
  SetWindowPos((HWND)window,nullptr,x,y,w,h,flags);
  }


// Map or unmap
void fxplatformShowWindow(void*,FXID window,FXint,FXint,FXbool shown){
  ShowWindow((HWND)window,shown?SW_SHOWNOACTIVATE:SW_HIDE);
  }


// Reparent under a new parent
void fxplatformReparentWindow(void*,FXID window,FXID parent){
  SetParent((HWND)window,(HWND)parent);
  }


// Raise or lower in the stacking order
void fxplatformRaiseWindow(void*,FXID window,FXbool raised){
  SetWindowPos((HWND)window,raised?HWND_TOP:HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }


// Release the mouse grab, restoring the cursor
void fxplatformReleaseMouseGrab(void*,FXID cursor){
  ReleaseCapture();
  if(cursor) SetCursor((HCURSOR)cursor);
  }


// Release the keyboard grab
void fxplatformReleaseKeyboardGrab(void*,FXTime){
  }


// Set window shape by means of region
void FXWindow::setShape(const FXRegion& region){
  if(xid){
    // Make a copy so as to leave original region intact
    HRGN rgn=CreateRectRgn(0,0,0,0);
    CombineRgn(rgn,(HRGN)region.region,rgn,RGN_COPY);
    SetWindowRgn((HWND)xid,rgn,false);
    }
  }


// Builds a region from a bitmap
static HRGN makeregion(HDC hdc,FXint w,FXint h){
  const COLORREF clear=RGB(255,255,255);
  HRGN shape,run;
  FXint x,y,z;
  shape=CreateRectRgn(0,0,0,0);
  for(y=0; y<h; y++){
    for(x=0; x<w; ){
      while(x<w && GetPixel(hdc,x,y)==clear) ++x;
      z=x;
      while(x<w && GetPixel(hdc,x,y)!=clear) ++x;
      if(z<x){
        run=CreateRectRgn(z,y,x,y+1);
        CombineRgn(shape,shape,run,RGN_OR);
        DeleteObject(run);
        }
      }
    }
  return shape;
  }


// Set window shape by means of bitmap
void FXWindow::setShape(FXBitmap* bitmap){
  if(!bitmap || !bitmap->id()){ fxerror("%s::setShape: illegal bitmap specified.\n",getClassName()); }
  if(xid){
    HDC hdc=CreateCompatibleDC(nullptr);
    SelectObject(hdc,bitmap->id());
    HRGN rgn=makeregion(hdc,bitmap->getWidth(),bitmap->getHeight());
    SetWindowRgn((HWND)xid,rgn,false);
    DeleteDC(hdc);
    }
  }


// Set window shape by means of bitmap
void FXWindow::setShape(FXIcon* icon){
  if(!icon || !icon->shape){ fxerror("%s::setShape: illegal icon specified.\n",getClassName()); }
  if(xid){
    HDC hdc=CreateCompatibleDC(nullptr);
    SelectObject(hdc,icon->shape);
    HRGN rgn=makeregion(hdc,icon->getWidth(),icon->getHeight());
    SetWindowRgn((HWND)xid,rgn,false);
    DeleteDC(hdc);
    }
  }


// Clear window shape to default rectangle
void FXWindow::clearShape(){
  if(xid){
    SetWindowRgn((HWND)xid,nullptr,false);
    }
  }


// Get pointer location (in window coordinates)
FXbool FXWindow::getCursorPosition(FXint& x,FXint& y,FXuint& buttons) const {
  if(xid){
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient((HWND)xid,&pt);
    x=pt.x; y=pt.y;
    buttons=fxmodifierkeys();
    return true;
    }
  return false;
  }


// Set pointer location (in window coordinates)
// Contributed by David Heath <dave@hipgraphics.com>
FXbool FXWindow::setCursorPosition(FXint x,FXint y){
  if(xid){
    POINT pt;
    pt.x=x;
    pt.y=y;
    ClientToScreen((HWND)xid,&pt);
    SetCursorPos(pt.x,pt.y);
    return true;
    }
  return false;
  }


// Get coordinates from another window (for symmetry)
void FXWindow::translateCoordinatesFrom(FXint& tox,FXint& toy,const FXWindow* fromwindow,FXint fromx,FXint fromy) const {
  if(!fromwindow){ fxerror("%s::translateCoordinatesFrom: from-window is NULL.\n",getClassName()); }
  if(xid && fromwindow->id()){
    POINT pt;
    pt.x=fromx;
    pt.y=fromy;
    ClientToScreen((HWND)fromwindow->id(),&pt);
    ScreenToClient((HWND)xid,&pt);
    tox=pt.x;
    toy=pt.y;
    }
  }


// Get coordinates to another window (for symmetry)
void FXWindow::translateCoordinatesTo(FXint& tox,FXint& toy,const FXWindow* towindow,FXint fromx,FXint fromy) const {
  if(!towindow){ fxerror("%s::translateCoordinatesTo: to-window is NULL.\n",getClassName()); }
  if(xid && towindow->id()){
    POINT pt;
    pt.x=fromx;
    pt.y=fromy;
    ClientToScreen((HWND)xid,&pt);
    ScreenToClient((HWND)towindow->id(),&pt);
    tox=pt.x;
    toy=pt.y;
    }
  }


// Acquire grab; also switches to the drag cursor
void FXWindow::grab(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::grab %p\n",getClassName(),this);
    if(dragCursor->id()==0){ fxerror("%s::grab: Cursor has not been created yet.\n",getClassName()); }
    if(!shown()){ fxwarning("%s::grab: Window is not visible.\n",getClassName()); }
    SetCapture((HWND)xid);
    if(GetCapture()!=(HWND)xid){
      SetCapture((HWND)xid);
      }
    SetCursor((HCURSOR)dragCursor->id());
    getApp()->mouseGrabWindow=this;
    }
  }


// Release grab; also switches back to the normal cursor
void FXWindow::ungrab(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::ungrab %p\n",getClassName(),this);
    getApp()->mouseGrabWindow=nullptr;
    ReleaseCapture();
    SetCursor((HCURSOR)defaultCursor->id());
    }
  }


// Grab keyboard device; note grabbing keyboard generates
// SEL_FOCUSIN on the grab-window and SEL_FOCUSOUT on the old
// focus-window
void FXWindow::grabKeyboard(){
  if(xid){
    FXTRACE(TOPIC_DETAIL,"%s::grabKeyboard %p\n",getClassName(),this);
    if(!shown()){ fxwarning("%s::ungrabKeyboard: Window is not visible.\n",getClassName()); }
    SetActiveWindow((HWND)xid); // FIXME Check this
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
    }
  }


// Show or hide the cursor
void FXWindow::showCursor(FXbool flag){
  if(!cursorShown() && flag){
    if(xid){
      ShowCursor(true);
      }
    flags|=FLAG_CURSOR;
    }
  else if(cursorShown() && !flag){
    if(xid){
      ShowCursor(false);
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
        if(underCursor() && !grabbed()) SetCursor((HCURSOR)cur->id());
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
        SetCursor((HCURSOR)cur->id());
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
      EnableWindow((HWND)xid,true);
      }
    }
  }


// Disable the window
void FXWindow::disable(){
  killFocus();
  if(isEnabled()){
    flags&=~FLAG_ENABLED;
    if(xid){
      EnableWindow((HWND)xid,false);
      if(getApp()->mouseGrabWindow==this){
        ReleaseCapture();
        SetCursor((HCURSOR)defaultCursor->id());
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        getApp()->mouseGrabWindow=nullptr;
        }
      if(getApp()->keyboardGrabWindow==this){
        getApp()->keyboardGrabWindow=nullptr;
        }
      }
    }
  }


}

#endif
