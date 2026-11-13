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


// Find the surface of the shell a window belongs to
FXWaylandSurface* fxwaylandShellSurface(FXWaylandApp* app,FXID window){
  FXID shell;
  if(!app || !window) return nullptr;
  shell=app->childmap.getShell(window);
  if(!shell) return nullptr;
  return fxwaylandFindSurface(app,shell);
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


// Size hints of the shell; the buffer size follows the window, and the state
// array tells us whether the compositor maximized, filled or focused it
static void toplevelConfigure(void* data,xdg_toplevel*,int32_t,int32_t,wl_array* states){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  FXuint mask=0;
  FXuint i,n=states->size/sizeof(uint32_t);
  for(i=0; i<n; i++){
    switch(((uint32_t*)states->data)[i]){
      case XDG_TOPLEVEL_STATE_MAXIMIZED: mask|=FXXWL_STATE_MAXIMIZED; break;
      case XDG_TOPLEVEL_STATE_FULLSCREEN: mask|=FXXWL_STATE_FULLSCREEN; break;
      case XDG_TOPLEVEL_STATE_ACTIVATED: mask|=FXXWL_STATE_ACTIVATED; break;
      }
    }
  s->state=mask;
  }
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
  s->buffers=nullptr;
  s->paint=nullptr;
  s->dirty=false;
  s->state=0;
  s->width=0;
  s->height=0;
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
      fxwaylandFreeBuffers(s);
      if(s->toplevel) xdg_toplevel_destroy(s->toplevel);
      if(s->popup) xdg_popup_destroy(s->popup);
      if(s->xdsurface) xdg_surface_destroy(s->xdsurface);
      if(s->surface) wl_surface_destroy(s->surface);
      delete s;
      return;
      }
    }
  }


/*******************************************************************************/

// The compositor is done with a buffer
static void bufferRelease(void* data,wl_buffer*){
  ((FXWaylandBuffer*)data)->busy=false;
  }


static const wl_buffer_listener bufferListener={
  bufferRelease
  };


// Make a shared memory file of the given size
static int createShmFile(FXuval size){
  const FXchar* dir=getenv("XDG_RUNTIME_DIR");
  FXchar name[256];
  int fd;
  if(!dir || !dir[0]) dir="/tmp";
  snprintf(name,sizeof(name),"%s/fox-shm-XXXXXX",dir);
  if((fd=mkstemp(name))<0) return -1;
  unlink(name);
  if(ftruncate(fd,(off_t)size)<0){
    close(fd);
    return -1;
    }
  return fd;
  }


// Make one buffer of the given size and map it
static FXWaylandBuffer* createBuffer(FXWaylandApp* app,FXint w,FXint h){
  FXWaylandBuffer* b;
  wl_shm_pool* pool;
  FXuval size=(FXuval)w*h*sizeof(FXColor);
  void* map;
  int fd;
  if(!app->shm) return nullptr;
  if((fd=createShmFile(size))<0){
    fxwarning("unable to create a shared memory buffer of %lu bytes\n",(unsigned long)size);
    return nullptr;
    }
  map=mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  if(map==MAP_FAILED){
    close(fd);
    fxwarning("unable to map a shared memory buffer of %lu bytes\n",(unsigned long)size);
    return nullptr;
    }
  pool=wl_shm_create_pool(app->shm,fd,(int32_t)size);
  b=new FXWaylandBuffer();
  b->buffer=wl_shm_pool_create_buffer(pool,0,w,h,w*(FXint)sizeof(FXColor),WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);
  if(!b->buffer){
    munmap(map,size);
    delete b;
    return nullptr;
    }
  wl_buffer_add_listener(b->buffer,&bufferListener,b);
  b->pixels=(FXColor*)map;
  b->map=(FXuchar*)map;
  b->mapsize=size;
  b->width=w;
  b->height=h;
  b->busy=false;
  b->next=nullptr;
  clearElms(b->pixels,w*h);
  return b;
  }


// Return an idle buffer of the given size
FXWaylandBuffer* fxwaylandAcquireBuffer(FXWaylandSurface* surf,FXint w,FXint h){
  FXWaylandBuffer* b;
  FXWaylandBuffer** link;
  FXuint n=0;
  if(!surf || w<=0 || h<=0) return nullptr;
  // Size change: the old buffers no longer fit
  if(surf->buffers && (surf->width!=w || surf->height!=h)){
    fxwaylandFreeBuffers(surf);
    }
  for(b=surf->buffers; b; b=b->next, n++){
    if(!b->busy) return b;
    }
  // Three buffers are enough; if all are held, the caller waits for a release
  if(n>=3) return nullptr;
  if((b=createBuffer(surf->app,w,h))!=nullptr){
    for(link=&surf->buffers; *link; link=&(*link)->next){}
    *link=b;
    surf->width=w;
    surf->height=h;
    }
  return b;
  }


// Attach, damage and commit
void fxwaylandCommitBuffer(FXWaylandSurface* surf,FXWaylandBuffer* buf,FXint x,FXint y,FXint w,FXint h){
  if(!surf || !buf) return;
  wl_surface_damage_buffer(surf->surface,x,y,w,h);
  wl_surface_attach(surf->surface,buf->buffer,0,0);
  wl_surface_commit(surf->surface);
  buf->busy=true;
  }


// Write a committed buffer as PPM when FOX_WAYLAND_DUMP is set
void fxwaylandDumpBuffer(FXWaylandSurface* surf,FXWaylandBuffer* buf){
  const FXchar* dir=getenv("FOX_WAYLAND_DUMP");
  FXchar path[512];
  FILE* fp;
  FXint x,y;
  FXColor c;
  if(!dir || !dir[0] || !buf) return;
  snprintf(path,sizeof(path),"%s/window-%u-%dx%d.ppm",dir,(unsigned int)surf->window,buf->width,buf->height);
  if((fp=fopen(path,"wb"))==nullptr) return;
  fprintf(fp,"P6\n%d %d\n255\n",buf->width,buf->height);
  for(y=0; y<buf->height; y++){
    for(x=0; x<buf->width; x++){
      c=buf->pixels[y*buf->width+x];
      fputc(FXREDVAL(c),fp);
      fputc(FXGREENVAL(c),fp);
      fputc(FXBLUEVAL(c),fp);
      }
    }
  fclose(fp);
  }


// Drop the buffer pool of a surface
void fxwaylandFreeBuffers(FXWaylandSurface* surf){
  FXWaylandBuffer* b;
  FXWaylandBuffer* n;
  if(!surf) return;
  for(b=surf->buffers; b; b=n){
    n=b->next;
    if(b->map) munmap(b->map,b->mapsize);
    if(b->buffer){ wl_buffer_destroy(b->buffer); }
    delete b;
    }
  surf->buffers=nullptr;
  surf->paint=nullptr;
  surf->dirty=false;
  surf->width=0;
  surf->height=0;
  }


/*******************************************************************************/

// Bind a window to the frame buffer of its shell
FXbool fxwaylandBindWindow(FXWaylandApp* app,FXID window,FXColor*& pixels,FXint& stride){
  FXWaylandSurface* s;
  FXRectangle shell;
  FXRectangle here;
  if((s=fxwaylandShellSurface(app,window))==nullptr) return false;

  // The frame is as large as the shell; children are drawn into it at their
  // place in the shell, so the whole window tree of a shell shares one buffer
  shell=app->childmap.getGeometry(s->window);
  if(shell.w<=0 || shell.h<=0) return false;

  // Acquire the frame once and keep it until the frame is committed
  if(!s->paint){
    if((s->paint=fxwaylandAcquireBuffer(s,shell.w,shell.h))==nullptr) return false;
    }
  if(s->paint->width!=shell.w || s->paint->height!=shell.h) return false;

  // Origin of the window within the frame
  here=app->childmap.getGlobalGeometry(window);
  pixels=s->paint->pixels+(FXival)here.y*s->paint->width+here.x;
  stride=s->paint->width;
  s->dirty=true;
  return true;
  }


// Commit the frames that were painted since the last call
void fxwaylandFlushFrames(FXWaylandApp* app){
  FXWaylandSurface* s;
  if(!app) return;
  for(s=app->surfaces; s; s=s->next){
    if(s->paint){
      if(s->dirty){
        fxwaylandCommitBuffer(s,s->paint,0,0,s->paint->width,s->paint->height);
        fxwaylandDumpBuffer(s,s->paint);
        }
      s->paint=nullptr;
      s->dirty=false;
      }
    }
  // The requests sit in the client buffer until we push them out
  if(app->display) wl_display_flush(app->display);
  }

}

#endif
