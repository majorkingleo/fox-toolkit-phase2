/********************************************************************************
*                                                                               *
*                    W a y l a n d   C o n n e c t i o n   D a t a              *
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
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - The small helpers that both the application module and the window modules
    need: the event queue and the surfaces of the shells.
  - A surface is created when a shell is created and lives until the shell is
    destroyed; children have no surface of their own, they are drawn into the
    surface of their shell.
  - The configure and close events of the shell are turned into neutral events
    here, so the window module only has to translate them once.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Append an event to the queue
void fxwaylandPost(FXWaylandApp* app,const FXWaylandEvent& event){
  FXuint count;
  if(!app) return;
  count=app->eventcount-app->eventhead;
  if(count==app->eventalloced){
    FXuint cap=app->eventalloced?app->eventalloced*2:64;
    if(app->eventhead){                                  // Compact first
      memmove(app->events,app->events+app->eventhead,count*sizeof(FXWaylandEvent));
      app->eventcount=count;
      app->eventhead=0;
      }
    if(!resizeElms(app->events,cap)) return;
    app->eventalloced=cap;
    }
  app->events[app->eventcount++]=event;
  }


// Find the surface of a shell
FXWaylandSurface* fxwaylandFindSurface(FXWaylandApp* app,FXID window){
  FXWaylandSurface* s;
  if(app){
    for(s=app->surfaces; s; s=s->next){
      if(s->window==window) return s;
      }
    }
  return nullptr;
  }


// Configure of the shell: acknowledge and tell the core
static void shellConfigure(void* data,xdg_surface* xdsurface,uint32_t serial){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  FXWaylandEvent ev;
  xdg_surface_ack_configure(xdsurface,serial);
  clearElms(&ev,1);
  ev.type=FXXWL_CONFIGURE;
  ev.id=s->window;
  fxwaylandPost(s->app,ev);
  }


static const xdg_surface_listener shellSurfaceListener={
  shellConfigure
  };


// Close request of the shell
static void toplevelClose(void* data,xdg_toplevel*){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  FXWaylandEvent ev;
  clearElms(&ev,1);
  ev.type=FXXWL_CLOSE;
  ev.id=s->window;
  fxwaylandPost(s->app,ev);
  }


// Size hints of the shell; the buffer size follows the window
static void toplevelConfigure(void*,xdg_toplevel*,int32_t,int32_t,wl_array*){}


static const xdg_toplevel_listener toplevelListener={
  toplevelConfigure,
  toplevelClose
  };


// Create the surface of a shell
FXWaylandSurface* fxwaylandCreateSurface(FXWaylandApp* app,FXID window,FXbool popup){
  FXWaylandSurface* s;
  if(!app || !app->compositor || !app->wmbase) return nullptr;
  s=new FXWaylandSurface();
  s->app=app;
  s->window=window;
  s->popup=nullptr;
  s->toplevel=nullptr;
  s->surface=wl_compositor_create_surface(app->compositor);
  if(!s->surface){ delete s; return nullptr; }
  s->xdsurface=xdg_wm_base_get_xdg_surface(app->wmbase,s->surface);
  if(!s->xdsurface){ wl_surface_destroy(s->surface); delete s; return nullptr; }
  xdg_surface_add_listener(s->xdsurface,&shellSurfaceListener,s);
  if(!popup){
    s->toplevel=xdg_surface_get_toplevel(s->xdsurface);
    if(s->toplevel){ xdg_toplevel_add_listener(s->toplevel,&toplevelListener,s); }
    }
  s->next=app->surfaces;
  app->surfaces=s;
  return s;
  }


// Destroy the surface of a shell
void fxwaylandDestroySurface(FXWaylandApp* app,FXID window){
  FXWaylandSurface* s;
  FXWaylandSurface** link;
  if(!app) return;
  for(link=&app->surfaces; (s=*link)!=nullptr; link=&s->next){
    if(s->window==window){
      *link=s->next;
      if(s->toplevel) xdg_toplevel_destroy(s->toplevel);
      if(s->popup) xdg_popup_destroy(s->popup);
      if(s->xdsurface) xdg_surface_destroy(s->xdsurface);
      if(s->surface) wl_surface_destroy(s->surface);
      delete s;
      return;
      }
    }
  }

}

#endif
