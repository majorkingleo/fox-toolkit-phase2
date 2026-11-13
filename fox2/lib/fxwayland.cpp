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

// Print a line of diagnostics when FOX_WAYLAND_TRACE is set; a Wayland client
// has no other way to say what it did, and the compositor logs do not show it
static FXbool traceon(){
  static FXint on=-1;
  if(on<0) on=getenv("FOX_WAYLAND_TRACE")?1:0;
  return on!=0;
  }


void fxwaylandTrace(const FXchar* format,...){
  va_list args;
  if(!traceon()) return;
  va_start(args,format);
  fputs("fxwayland: ",stderr);
  vfprintf(stderr,format,args);
  va_end(args);
  fflush(stderr);
  }


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


// Post an expose for a window and every visible window below it; mapping a
// window makes the server expose it on X11, and this is the same thing
void fxwaylandExpose(FXWaylandApp* app,FXID window){
  FXWaylandEvent ev;
  FXRectangle r;
  FXID child;
  if(!app || !window) return;
  r=app->childmap.getGeometry(window);
  if(app->childmap.isVisible(window) && r.w>0 && r.h>0){
    clearElms(&ev,1);
    ev.type=0;
    ev.id=window;
    ev.x=0;
    ev.y=0;
    ev.w=r.w;
    ev.h=r.h;
    ev.code=FXXWL_EXPOSE;
    ev.state=1;                 // Painted to order, not by the compositor
    fxwaylandPost(app,ev);
    }
  fxwaylandTrace("expose window %u %dx%d\n",(unsigned int)window,r.w,r.h);
  for(child=app->childmap.getFirst(window); child; child=app->childmap.getNext(child)){
    fxwaylandExpose(app,child);
    }
  }


// Configure of the shell: acknowledge and tell the core; the toplevel
// configure that came before it carries the size the compositor asks for
static void shellConfigure(void* data,xdg_surface* xdsurface,uint32_t serial){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  FXWaylandEvent ev;
  xdg_surface_ack_configure(xdsurface,serial);
  s->configured=true;
  fxwaylandTrace("configure %u serial %u size %dx%d\n",(unsigned int)s->window,serial,s->wantw,s->wanth);
  clearElms(&ev,1);
  ev.code=FXXWL_CONFIGURE;
  ev.id=s->window;
  ev.w=s->wantw;
  ev.h=s->wanth;
  s->wantw=0;
  s->wanth=0;
  fxwaylandPost(s->app,ev);

  // The windows could not be painted before this, so paint them now
  fxwaylandExpose(s->app,s->window);
  }


static const xdg_surface_listener shellSurfaceListener={
  shellConfigure
  };


// Close request of the shell
static void toplevelClose(void* data,xdg_toplevel*){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  FXWaylandEvent ev;
  clearElms(&ev,1);
  ev.code=FXXWL_CLOSE;
  ev.id=s->window;
  fxwaylandPost(s->app,ev);
  }


// Size hints of the shell; the buffer size follows the window, and the state
// array tells us whether the compositor maximized, filled or focused it
static void toplevelConfigure(void* data,xdg_toplevel*,int32_t width,int32_t height,wl_array* states){
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
  s->wantw=width>0?width:0;             // Zero means "the size is yours"
  s->wanth=height>0?height:0;
  fxwaylandTrace("toplevel configure %u %dx%d\n",(unsigned int)s->window,s->wantw,s->wanth);
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
  s->store=nullptr;
  s->storew=0;
  s->storeh=0;
  s->dirty=false;
  s->configured=false;
  s->wantw=0;
  s->wanth=0;
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
  fxwaylandTrace("create surface for window %u (%s)\n",(unsigned int)window,popup?"popup":"toplevel");
  s->next=app->surfaces;
  app->surfaces=s;

  // Nothing is mapped before the compositor configured the shell, so ask for
  // the first configure now by committing the surface without a buffer
  wl_surface_commit(s->surface);
  wl_display_flush(app->display);
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
      freeElms(s->store);
      s->store=nullptr;
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
  fxwaylandTrace("new buffer %dx%d map %p size %lu\n",w,h,map,(unsigned long)size);
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
  fxwaylandTrace("free buffers of window %u\n",(unsigned int)surf->window);
  for(b=surf->buffers; b; b=n){
    n=b->next;
    if(b->map) munmap(b->map,b->mapsize);
    if(b->buffer){ wl_buffer_destroy(b->buffer); }
    delete b;
    }
  surf->buffers=nullptr;
  surf->width=0;
  surf->height=0;
  }


/*******************************************************************************/

// Give the backing store of a surface the size of its shell; content that does
// not fit is dropped, which is what a resize does to a window anyway
static FXbool resizeStore(FXWaylandSurface* s,FXint w,FXint h){
  if(s->store && s->storew==w && s->storeh==h) return true;
  freeElms(s->store);
  s->store=nullptr;
  s->storew=0;
  s->storeh=0;
  if(w<=0 || h<=0) return false;
  if(!allocElms(s->store,w*h)) return false;
  clearElms(s->store,w*h);
  s->storew=w;
  s->storeh=h;
  fxwaylandTrace("store of window %u is %dx%d\n",(unsigned int)s->window,w,h);
  return true;
  }


// Bind a window to the backing store of its shell
FXbool fxwaylandBindWindow(FXWaylandApp* app,FXID window,FXColor*& pixels,FXint& stride){
  FXWaylandSurface* s;
  FXRectangle shell;
  FXRectangle place;
  FXRectangle here;
  if((s=fxwaylandShellSurface(app,window))==nullptr) return false;

  // The store is as large as the shell; every window of the tree paints into
  // it at its own place, which is why a window can be repainted on its own
  shell=app->childmap.getGeometry(s->window);
  if(!resizeStore(s,shell.w,shell.h)) return false;

  // Origin of the window inside the shell; the table reports screen
  // coordinates, so the place of the shell itself comes off again
  place=app->childmap.getGlobalGeometry(s->window);
  here=app->childmap.getGlobalGeometry(window);
  pixels=s->store+(FXival)(here.y-place.y)*s->storew+(here.x-place.x);
  stride=s->storew;
  s->dirty=true;
  fxwaylandTrace("bind window %u store %dx%d at %+d%+d pixels %p\n",(unsigned int)window,s->storew,s->storeh,here.x-place.x,here.y-place.y,pixels);
  return true;
  }


// Commit the shells that were painted since the last call
void fxwaylandFlushFrames(FXWaylandApp* app){
  FXWaylandSurface* s;
  FXWaylandBuffer* b;
  if(!app) return;
  for(s=app->surfaces; s; s=s->next){
    if(s->dirty && s->store){

      // Nothing is shown before the compositor configured the shell
      if(!s->configured) continue;

      // A recycled buffer starts blank, so the whole store goes into it
      if((b=fxwaylandAcquireBuffer(s,s->storew,s->storeh))!=nullptr){
        memcpy(b->pixels,s->store,(FXuval)s->storew*s->storeh*sizeof(FXColor));
        fxwaylandCommitBuffer(s,b,0,0,b->width,b->height);
        fxwaylandDumpBuffer(s,b);
        s->dirty=false;
        fxwaylandTrace("commit window %u %dx%d\n",(unsigned int)s->window,b->width,b->height);
        }
      }
    }
  // The requests sit in the client buffer until we push them out
  if(app->display) wl_display_flush(app->display);
  }


// Take the content away from a shell surface
void fxwaylandHideSurface(FXWaylandSurface* surf){
  if(!surf || !surf->surface) return;

  // Nothing to commit for a hidden shell; the store keeps its content so that
  // showing it again has something to copy from
  surf->dirty=false;

  // A surface without a buffer shows nothing
  wl_surface_attach(surf->surface,nullptr,0,0);
  wl_surface_commit(surf->surface);
  if(surf->app->display) wl_display_flush(surf->app->display);
  }

}

#endif
