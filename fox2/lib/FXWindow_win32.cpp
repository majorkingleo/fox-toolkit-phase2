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


}

#endif
