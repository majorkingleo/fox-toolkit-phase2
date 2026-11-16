/********************************************************************************
*                                                                               *
*              O p e n G L   S u b s u r f a c e   ( W a y l a n d )            *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "fxwaylandpriv.h"
#include "fxglwayland.h"

#if defined(FOX_WAYLAND) && defined(HAVE_EGL_H)

#include <EGL/egl.h>
#include <wayland-egl.h>

/*
  Notes:

  - A GL canvas is a wl_subsurface of the surface of its shell.  The subsurface
    has its own wl_surface and a wl_egl_window; EGL draws into it, while the
    shell keeps the shared memory buffer for the ordinary widgets around it.
  - The subsurface is desynchronized, so its commits do not hold up the frame
    of the shell; it has to be placed and resized in surface coordinates when
    the widget moves, which is what fxglwaylandPlaceCanvas() does.
  - The canvas list is per process, like the toolkit itself; entries are
    removed when the canvas is destroyed.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// One GL canvas on the compositor
struct FXGLWaylandCanvas {
  FXWaylandApp*     app;        // The connection
  FXID              window;     // Virtual window of the canvas
  wl_surface*       surface;    // The canvas surface
  wl_subsurface*    sub;        // Its role under the shell
  wl_egl_window*    eglwin;     // EGL view of that surface
  EGLSurface        eglsurface; // EGL surface, made on demand
  FXint             surfw;      // Size of eglsurface
  FXint             surfh;
  FXbool            mapped;     // A buffer has been attached
  FXGLWaylandCanvas* next;      // Next canvas
  };

static FXGLWaylandCanvas* canvases=nullptr;


// Find the canvas of a window
static FXGLWaylandCanvas* findCanvas(FXID window){
  FXGLWaylandCanvas* c;
  for(c=canvases; c; c=c->next){
    if(c->window==window) return c;
    }
  return nullptr;
  }


// Place the subsurface in surface coordinates of its shell
static void placeCanvas(FXGLWaylandCanvas* c){
  FXRectangle base;
  FXRectangle here;
  FXint x,y,w,h;
  base=c->app->childmap.getGlobalGeometry(c->app->childmap.getShell(c->window));
  here=c->app->childmap.getGlobalGeometry(c->window);
  x=here.x-base.x;
  y=here.y-base.y;
  w=FXMAX(here.w,1);
  h=FXMAX(here.h,1);
  wl_subsurface_set_position(c->sub,x,y);
  if(c->eglwin) wl_egl_window_resize(c->eglwin,w,h,0,0);
  }


// Prepare the EGL display of the connection
FXbool fxglwaylandOpen(FXApp* foxapp){
  FXWaylandApp* app;
  EGLDisplay dpy;
  EGLint major=0,minor=0;
  if(!foxapp) return false;
  app=fxwayland(foxapp->getDisplay());
  if(!app || !app->display) return false;
  if(app->egldisplay) return true;
  dpy=eglGetDisplay((EGLNativeDisplayType)app->display);
  if(dpy==EGL_NO_DISPLAY) return false;
  if(!eglInitialize(dpy,&major,&minor)) return false;
  if(!eglBindAPI(EGL_OPENGL_API)){
    eglTerminate(dpy);
    return false;
    }
  app->egldisplay=(void*)dpy;
  fxwaylandTrace("EGL %d.%d on display %p\n",(int)major,(int)minor,(void*)dpy);
  return true;
  }


// Tear down the EGL display and any canvas still registered
void fxglwaylandClose(FXApp* foxapp){
  FXWaylandApp* app;
  FXGLWaylandCanvas* c;
  if(!foxapp) return;
  app=fxwayland(foxapp->getDisplay());
  if(!app) return;
  while(canvases){
    c=canvases;
    canvases=c->next;
    if(c->eglsurface!=EGL_NO_SURFACE && app->egldisplay) eglDestroySurface((EGLDisplay)app->egldisplay,c->eglsurface);
    if(c->eglwin) wl_egl_window_destroy(c->eglwin);
    if(c->sub) wl_subsurface_destroy(c->sub);
    if(c->surface) wl_surface_destroy(c->surface);
    delete c;
    }
  if(app->egldisplay){
    eglTerminate((EGLDisplay)app->egldisplay);
    app->egldisplay=nullptr;
    }
  }


// EGL display of the connection
void* fxglwaylandDisplay(FXApp* foxapp){
  FXWaylandApp* app;
  if(!foxapp) return nullptr;
  app=fxwayland(foxapp->getDisplay());
  return app?app->egldisplay:nullptr;
  }


// Create the subsurface of a GL canvas
FXbool fxglwaylandCreateCanvas(FXApp* foxapp,FXID window){
  FXWaylandApp* app;
  FXWaylandSurface* shell;
  FXGLWaylandCanvas* c;
  FXID shellwindow;
  if(!foxapp || !window) return false;
  app=fxwayland(foxapp->getDisplay());
  if(!app || !app->compositor || !app->subcompositor) return false;
  if(findCanvas(window)) return true;
  shellwindow=app->childmap.getShell(window);
  shell=fxwaylandFindSurface(app,shellwindow);
  if(!shell || !shell->surface) return false;
  c=new FXGLWaylandCanvas();
  c->app=app;
  c->window=window;
  c->surface=wl_compositor_create_surface(app->compositor);
  c->sub=nullptr;
  c->eglwin=nullptr;
  c->eglsurface=EGL_NO_SURFACE;
  c->surfw=0;
  c->surfh=0;
  c->mapped=false;
  if(!c->surface){ delete c; return false; }
  c->sub=wl_subcompositor_get_subsurface(app->subcompositor,c->surface,shell->surface);
  if(!c->sub){
    wl_surface_destroy(c->surface);
    delete c;
    return false;
    }
  wl_subsurface_set_desync(c->sub);
  placeCanvas(c);
  {
  FXRectangle here=app->childmap.getGlobalGeometry(window);
  c->eglwin=wl_egl_window_create(c->surface,FXMAX(here.w,1),FXMAX(here.h,1));
  }
  if(!c->eglwin){
    wl_subsurface_destroy(c->sub);
    wl_surface_destroy(c->surface);
    delete c;
    return false;
    }
  // The parent has to commit once so that the compositor sees the new
  // subsurface; its buffer stays the frame that is already on screen
  wl_surface_commit(shell->surface);
  fxwaylandTrace("GL canvas %u subsurface created\n",(unsigned int)window);
  c->next=canvases;
  canvases=c;
  return true;
  }


// Destroy the subsurface of a GL canvas
void fxglwaylandDestroyCanvas(FXApp* foxapp,FXID window){
  FXWaylandApp* app;
  FXGLWaylandCanvas* c;
  FXGLWaylandCanvas** pp;
  if(!foxapp || !window) return;
  app=fxwayland(foxapp->getDisplay());
  if(!app) return;
  pp=&canvases;
  while((c=*pp)!=nullptr){
    if(c->window==window){
      *pp=c->next;
      if(c->eglsurface!=EGL_NO_SURFACE && app->egldisplay) eglDestroySurface((EGLDisplay)app->egldisplay,c->eglsurface);
      if(c->eglwin) wl_egl_window_destroy(c->eglwin);
      if(c->sub) wl_subsurface_destroy(c->sub);
      if(c->surface) wl_surface_destroy(c->surface);
      delete c;
      return;
      }
    pp=&c->next;
    }
  }


// Move and resize the subsurface after the widget moved
void fxglwaylandPlaceCanvas(FXApp* foxapp,FXID window){
  FXGLWaylandCanvas* c;
  if(!foxapp || !window) return;
  if((c=findCanvas(window))!=nullptr){
    placeCanvas(c);
    }
  }


// Map or unmap the subsurface of a GL canvas
void fxglwaylandShowCanvas(FXApp* foxapp,FXID window,FXbool shown){
  FXGLWaylandCanvas* c;
  if(!foxapp || !window) return;
  if((c=findCanvas(window))==nullptr) return;
  if(!shown && c->mapped){
    wl_surface_attach(c->surface,nullptr,0,0);
    wl_surface_commit(c->surface);
    c->mapped=false;
    }
  }


// EGL window of a GL canvas
void* fxglwaylandEglWindow(FXApp* foxapp,FXID window){
  FXGLWaylandCanvas* c;
  if(!foxapp || !window) return nullptr;
  c=findCanvas(window);
  return c?c->eglwin:nullptr;
  }


// EGL surface of a GL canvas at the given size
void* fxglwaylandEglSurface(FXApp* foxapp,FXID window,FXint w,FXint h,void* config){
  FXWaylandApp* app;
  FXGLWaylandCanvas* c;
  EGLSurface esurf;
  if(!foxapp || !window) return nullptr;
  app=fxwayland(foxapp->getDisplay());
  c=findCanvas(window);
  if(!app || !app->egldisplay || !c || !c->eglwin) return nullptr;
  w=FXMAX(w,1);
  h=FXMAX(h,1);
  if(c->eglsurface!=EGL_NO_SURFACE && c->surfw==w && c->surfh==h) return (void*)c->eglsurface;
  if(c->eglsurface!=EGL_NO_SURFACE){
    eglDestroySurface((EGLDisplay)app->egldisplay,c->eglsurface);
    c->eglsurface=EGL_NO_SURFACE;
    }
  wl_egl_window_resize(c->eglwin,w,h,0,0);
  esurf=eglCreateWindowSurface((EGLDisplay)app->egldisplay,(EGLConfig)config,(EGLNativeWindowType)(FXuval)c->eglwin,nullptr);
  if(esurf==EGL_NO_SURFACE){
    fxwaylandTrace("EGL window surface failed for canvas %u: 0x%x\n",(unsigned int)window,(unsigned int)eglGetError());
    return nullptr;
    }
  fxwaylandTrace("EGL window surface for canvas %u: %dx%d\n",(unsigned int)window,w,h);
  c->eglsurface=esurf;
  c->surfw=w;
  c->surfh=h;
  return (void*)esurf;
  }

}

#else

namespace FX {

FXbool fxglwaylandOpen(FXApp*){ return false; }
void fxglwaylandClose(FXApp*){}
void* fxglwaylandDisplay(FXApp*){ return nullptr; }
FXbool fxglwaylandCreateCanvas(FXApp*,FXID){ return false; }
void fxglwaylandDestroyCanvas(FXApp*,FXID){}
void fxglwaylandPlaceCanvas(FXApp*,FXID){}
void fxglwaylandShowCanvas(FXApp*,FXID,FXbool){}
void* fxglwaylandEglWindow(FXApp*,FXID){ return nullptr; }
void* fxglwaylandEglSurface(FXApp*,FXID,FXint,FXint,void*){ return nullptr; }

}

#endif
