/********************************************************************************
*                                                                               *
*              P l a t f o r m   W i n d o w   ( W a y l a n d )                *
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
#include "FXString.h"
#include "fxplatformwindow.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - The native window of this platform is a row in the virtual window table:
    only a shell owns a surface, every other window is drawn into the surface
    of the shell it belongs to.  The core keeps its own window objects; this
    module answers with synthetic ids and never shows them to the compositor.
  - Creating a shell creates its wl_surface, xdg_surface and xdg_toplevel (a
    popup role comes with the popup code of phase 7).  The buffer that the
    shell draws into is attached when the window paints.
  - Attaching to a window of another process is not a Wayland concept, so
    fxplatformAttachWindow() does nothing; grabs are implicit in the popup and
    seat protocol and are released in phase 6.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create a native window
FXID fxplatformCreateWindow(const FXWindowCreateInfo& info){
  FXWaylandApp* app=fxwayland(info.display);
  FXuint flags=0;
  FXID parent=info.parent;
  FXID id;
  fxwaylandTrace("create window parent %u %dx%d%+d%+d flags %u\n",(unsigned int)info.parent,info.w,info.h,info.x,info.y,info.flags);
  if(!app) return 0;
  if(info.flags&WINDOWINFO_SHELL) flags|=WINMAP_SHELL;
  if(info.flags&WINDOWINFO_SHOWN) flags|=WINMAP_SHOWN;

  // A shell hangs off the root window, which has no row in the table
  if(!app->childmap.hasWindow(parent)){
    parent=0;
    flags|=WINMAP_SHELL;
    }
  id=app->childmap.create(parent,info.x,info.y,info.w,info.h,flags);
  if(!id) return 0;
  if(flags&WINMAP_SHELL){
    if(!fxwaylandCreateSurface(app,id,(info.flags&WINDOWINFO_OVERRIDEREDIRECT)!=0)){
      fxwaylandTrace("no surface for window %u\n",(unsigned int)id);
      app->childmap.destroy(id);
      return 0;
      }
    }
  return id;
  }


// Take over a native window; there are no foreign windows on this platform
void fxplatformAttachWindow(const FXWindowCreateInfo& info,FXID window){
  }


// Destroy a native window
void fxplatformDestroyWindow(void* display,FXID window,FXID shell,FXID colormap,FXID xdndaware){
  FXWaylandApp* app=fxwayland(display);
  if(!app) return;
  fxwaylandDestroySurface(app,window);
  app->childmap.destroy(window);
  }


// Move, resize, or both
void fxplatformPlaceWindow(void* display,FXID window,FXint x,FXint y,FXint w,FXint h,FXint oldw,FXint oldh,FXbool shown,FXuint placement){
  FXWaylandApp* app=fxwayland(display);
  if(!app) return;
  app->childmap.setPlacement(window,x,y,w,h,placement);
  }


// Map or unmap a native window
void fxplatformShowWindow(void* display,FXID window,FXint w,FXint h,FXbool shown){
  FXWaylandApp* app=fxwayland(display);
  if(!app) return;
  app->childmap.setShown(window,shown);

  // Mapping a window exposes it, and with it everything below it, just like
  // the server does on X11
  if(shown) fxwaylandExpose(app,window);
  }


// Reparent a native window
void fxplatformReparentWindow(void* display,FXID window,FXID parent){
  FXWaylandApp* app=fxwayland(display);
  if(!app) return;
  app->childmap.reparent(window,parent);
  }


// Raise or lower a native window
void fxplatformRaiseWindow(void* display,FXID window,FXbool raised){
  FXWaylandApp* app=fxwayland(display);
  if(!app) return;
  if(raised) app->childmap.raise(window);
  else app->childmap.lower(window);
  }


// Release the mouse grab; grabs are implicit in the protocols that use them
void fxplatformReleaseMouseGrab(void* display,FXID cursor){
  }


// Release the keyboard grab
void fxplatformReleaseKeyboardGrab(void* display,FXTime time){
  }

}

#endif
