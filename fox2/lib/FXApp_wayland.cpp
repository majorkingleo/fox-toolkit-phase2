/********************************************************************************
*                                                                               *
*                          A p p   ( W a y l a n d )                            *
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
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXException.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXRootWindow.h"
#include "FXApp.h"
#include "fxapppriv.h"
#include "fxplatform.h"
#include "fxpriv.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - The connection: connect the display, walk the registry, bind the globals
    this platform needs, and keep a queue of neutral events for the core.
  - The queue is filled by the protocol listeners; getNextEvent() takes one
    event at a time and keeps the timer, signal, chore, refresh and input file
    handling of the X11 module, so the shared event loop behaves the same.
  - Globals are bound to the versions this module understands; a compositor
    that lacks wl_compositor, wl_shm or xdg_wm_base is reported and the
    connection is refused, because nothing can be shown without them.
  - Window surfaces, buffers and the virtual child windows arrive with
    FXWindow_wayland.cpp; until then the queue only carries the events of the
    connection itself (wm_base ping, registry changes, output changes).
*/

#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

/*******************************************************************************/

// Registry: bind the globals we understand
static void registryGlobal(void* data,wl_registry* registry,uint32_t name,const char* interface,uint32_t version){
  FXWaylandApp* app=fxwayland(data);
  FXTRACE(TOPIC_DETAIL,"registry global %s version %u name %u\n",interface,version,name);
  if(strcmp(interface,wl_compositor_interface.name)==0){
    app->compositor=(wl_compositor*)wl_registry_bind(registry,name,&wl_compositor_interface,FXMIN(version,4u));
    }
  else if(strcmp(interface,wl_shm_interface.name)==0){
    app->shm=(wl_shm*)wl_registry_bind(registry,name,&wl_shm_interface,1);
    }
  else if(strcmp(interface,wl_seat_interface.name)==0){
    if(!app->seat){
      app->seat=(wl_seat*)wl_registry_bind(registry,name,&wl_seat_interface,FXMIN(version,5u));
      }
    }
  else if(strcmp(interface,wl_output_interface.name)==0){
    if(!app->output){
      app->output=(wl_output*)wl_registry_bind(registry,name,&wl_output_interface,FXMIN(version,2u));
      }
    }
  else if(strcmp(interface,xdg_wm_base_interface.name)==0){
    app->wmbase=(xdg_wm_base*)wl_registry_bind(registry,name,&xdg_wm_base_interface,FXMIN(version,2u));
    }
  }


// Registry: a global went away
static void registryGlobalRemove(void*,wl_registry*,uint32_t name){
  FXTRACE(TOPIC_DETAIL,"registry global %u removed\n",name);
  }


// Shell: answer the ping so the compositor does not consider us hung
static void wmBasePing(void*,xdg_wm_base* wmbase,uint32_t serial){
  xdg_wm_base_pong(wmbase,serial);
  }


// Shared memory: remember whether 32 bit ARGB is available
static void shmFormat(void* data,wl_shm*,uint32_t format){
  FXWaylandApp* app=fxwayland(data);
  if(format==WL_SHM_FORMAT_ARGB8888) app->argb=true;
  }


// Output: the physical size and the current mode give us the screen size
static void outputGeometry(void* data,wl_output*,int32_t,int32_t,int32_t width,int32_t height,int32_t,const char*,const char*,int32_t){
  FXWaylandApp* app=fxwayland(data);
  if(width>0 && height>0){
    app->screenwidth=width;
    app->screenheight=height;
    }
  }


static void outputMode(void* data,wl_output*,uint32_t flags,int32_t width,int32_t height,int32_t){
  FXWaylandApp* app=fxwayland(data);
  if((flags&WL_OUTPUT_MODE_CURRENT) && width>0 && height>0){
    app->screenwidth=width;
    app->screenheight=height;
    }
  }


static void outputDone(void*,wl_output*){}

static void outputScale(void*,wl_output*,int32_t){}


static const wl_registry_listener registryListener={
  registryGlobal,
  registryGlobalRemove
  };

static const wl_output_listener outputListener={
  outputGeometry,
  outputMode,
  outputDone,
  outputScale
  };

static const xdg_wm_base_listener wmBaseListener={
  wmBasePing
  };

static const wl_shm_listener shmListener={
  shmFormat
  };


/*******************************************************************************/

// Initialize platform members
void FXApp::initPlatformMembers(){
  FXWaylandApp* app=new FXWaylandApp();
  app->display=nullptr;
  app->registry=nullptr;
  app->compositor=nullptr;
  app->shm=nullptr;
  app->seat=nullptr;
  app->output=nullptr;
  app->wmbase=nullptr;
  app->screenwidth=0;
  app->screenheight=0;
  app->rootwindow=0;
  app->argb=false;
  app->lost=false;
  app->events=nullptr;
  app->eventcount=0;
  app->eventalloced=0;
  app->eventhead=0;
  app->surfaces=nullptr;
  display=app;
  }


// Release platform members
void FXApp::freePlatformMembers(){
  FXWaylandApp* app=fxwayland(display);
  if(app){
    freeElms(app->events);
    delete app;
    display=nullptr;
    }
  }


// Read platform settings; the compositor has none we can read before connecting
void FXApp::initPlatformSettings(){
  }


// Parse a platform specific argument; there are none yet
FXbool FXApp::parsePlatformArgument(int argc,char** argv,FXint& j,const FXchar*& dpy,const FXchar*& istyle){
  return false;
  }


/*******************************************************************************/

// Open the connection to the compositor
FXbool FXApp::openDisplay(const FXchar* dpy){
  FXWaylandApp* app=fxwayland(display);
  FXTRACE(TOPIC_CREATION,"%s::openDisplay(%s)\n",getClassName(),dpy?dpy:"");
  if(!app) return false;
  if(app->display) return true;
  fxwaylandTrace("openDisplay %s\n",dpy?dpy:"(default)");
  app->display=wl_display_connect(dpy);
  if(!app->display){
    fxwarning("%s::openDisplay: unable to connect to a Wayland compositor.\n",getClassName());
    return false;
    }
  app->registry=wl_display_get_registry(app->display);
  wl_registry_add_listener(app->registry,&registryListener,app);
  wl_display_roundtrip(app->display);
  if(app->wmbase){ xdg_wm_base_add_listener(app->wmbase,&wmBaseListener,app); }
  if(app->shm){ wl_shm_add_listener(app->shm,&shmListener,app); }
  if(app->output){ wl_output_add_listener(app->output,&outputListener,app); }
  wl_display_roundtrip(app->display);
  if(!app->compositor || !app->shm || !app->wmbase){
    fxwarning("%s::openDisplay: the compositor lacks %s%s%s.\n",getClassName(),
              app->compositor?"":"wl_compositor ",
              app->shm?"":"wl_shm ",
              app->wmbase?"":"xdg_wm_base ");
    closeDisplay();
    return false;
    }
  if(!app->argb){
    fxwarning("%s::openDisplay: the compositor has no ARGB8888 shared memory format.\n",getClassName());
    }

  // Preregister some drag types; a Wayland drag type is the index of a mime
  // type in the table of the data device, which phase 8 fills in
  FXWindow::octetType=3;
  FXWindow::deleteType=4;
  FXWindow::textType=5;
  FXWindow::colorType=6;
  FXWindow::urilistType=7;
  FXWindow::utf8Type=8;
  FXWindow::actionType=9;

  // Clear sticky mod state
  stickyMods=0;

  // Lock the global mutex
  appMutex.lock();

  fxwaylandTrace("connected, screen %dx%d\n",app->screenwidth,app->screenheight);

  // We have been initialized
  initialized=true;
  return true;
  }


// Close the connection to the compositor
FXbool FXApp::closeDisplay(){
  FXWaylandApp* app=fxwayland(display);
  if(!app) return false;
  FXTRACE(TOPIC_CREATION,"%s::closeDisplay\n",getClassName());
  if(app->wmbase){ xdg_wm_base_destroy(app->wmbase); app->wmbase=nullptr; }
  if(app->output){ wl_output_destroy(app->output); app->output=nullptr; }
  if(app->seat){ wl_seat_destroy(app->seat); app->seat=nullptr; }
  if(app->shm){ wl_shm_destroy(app->shm); app->shm=nullptr; }
  if(app->compositor){ wl_compositor_destroy(app->compositor); app->compositor=nullptr; }
  if(app->registry){ wl_registry_destroy(app->registry); app->registry=nullptr; }
  if(app->display){ wl_display_flush(app->display); wl_display_disconnect(app->display); app->display=nullptr; }
  app->eventcount=0;
  app->eventhead=0;
  return true;
  }


/*******************************************************************************/

// Get the next event for the shared event loop
FXbool FXApp::getNextEvent(FXRawEvent& ev,FXTime blocking){
  FXWaylandApp* app=fxwayland(display);
  FXTime now,interval;
  FXint nfds;
  fd_set readfds;
  fd_set writefds;
  fd_set exceptfds;
  int maxfds;
  int displayfd;
  struct timespec delta;

  // No event
  clearElms(&ev,1);

  // If a timer is due, handle it
  if(timers && timers->due<=FXThread::time()){
    FXTimer* t=timers;
    timers=t->next;
    t->next=timerrecs;
    timerrecs=t;
    if(t->target && t->target->tryHandle(this,FXSEL(SEL_TIMEOUT,t->message),t->data)) refresh();
    return false;
    }

  // Check non-immediate signals that may have fired
  if(signalreceived){
    FXint sig=signalreceived;
    FXint nxt=MAXSIGNALS;
    signals[sig].notified=false;
    while(--nxt && !signals[nxt].notified){}
    signalreceived=nxt;
    if(signals[sig].target && signals[sig].target->tryHandle(this,FXSEL(SEL_SIGNAL,signals[sig].message),(void*)(FXival)sig)) refresh();
    return false;
    }

  // Pull what the compositor already sent us
  if(initialized && app && app->display && !app->lost){
    if(wl_display_dispatch_pending(app->display)<0){
      app->lost=true;
      }
    }

  // Anything queued?
  if(!initialized || !app || !app->display || app->eventcount==app->eventhead){

    // Watch the input file descriptors and the connection
    maxfds=maxhandle;
    readfds=handles->hnd[0];
    writefds=handles->hnd[1];
    exceptfds=handles->hnd[2];
    displayfd=-1;
    if(initialized && app && app->display){
      displayfd=wl_display_get_fd(app->display);
      FD_SET(displayfd,&readfds);
      if(displayfd>maxfds) maxfds=displayfd;
      }

    // Quick poll for ready events or inputs
    now=FXThread::time();
    delta.tv_sec=0;
    delta.tv_nsec=0;
    nfds=pselect(maxfds+1,&readfds,&writefds,&exceptfds,&delta,nullptr);

    // Nothing to do, so perform idle processing
    if(nfds==0){

      // Release the repaint rectangles as exposure
      if(repaints){
        FXRepaint* r=repaints;
        ev.type=0;
        ev.id=r->window;
        ev.x=r->rect.x;
        ev.y=r->rect.y;
        ev.w=r->rect.w-r->rect.x;
        ev.h=r->rect.h-r->rect.y;
        ev.code=FXXWL_EXPOSE;
        ev.state=r->synth;
        repaints=r->next;
        r->next=repaintrecs;
        repaintrecs=r;
        return true;
        }

      // Nothing left to paint, so finish the frames of the shells
      fxwaylandFlushFrames(app);

      // Do our chores :-)
      if(chores){
        FXChore* c=chores;
        chores=c->next;
        c->next=chorerecs;
        chorerecs=c;
        if(c->target && c->target->tryHandle(this,FXSEL(SEL_CHORE,c->message),c->data)) refresh();
        }

      // GUI updating
      if(refresher){
        if(refresher->getFirst()){
          refresher=refresher->getFirst();
          }
        else{
          while(refresher->getParent()){
            if(refresher->getNext()){ refresher=refresher->getNext(); break; }
            refresher=refresher->getParent();
            }
          }
        refresher->handle(this,FXSEL(SEL_UPDATE,0),nullptr);
        if(refresher!=refresherstop) return false;
        refresher=refresherstop=nullptr;
        }

      // More chores pending
      if(chores) return false;

      // We are not blocking
      if(blocking<=0) return false;

      // Block for a while or until the connection is readable
      maxfds=maxhandle;
      readfds=handles->hnd[0];
      writefds=handles->hnd[1];
      exceptfds=handles->hnd[2];
      if(displayfd>=0){
        FD_SET(displayfd,&readfds);
        if(displayfd>maxfds) maxfds=displayfd;
        }
      if(timers && (interval=timers->due-now)<blocking) blocking=interval;
      if(blocking<=0) return false;
      appMutex.unlock();
      if(timers || blocking<forever){
        delta.tv_sec=blocking/1000000000;
        delta.tv_nsec=blocking%1000000000;
        nfds=pselect(maxfds+1,&readfds,&writefds,&exceptfds,&delta,nullptr);
        }
      else{
        nfds=pselect(maxfds+1,&readfds,&writefds,&exceptfds,nullptr,nullptr);
        }
      appMutex.lock();
      }

    // Timed out or interrupted
    if(nfds<=0){
      if(nfds<0 && errno!=EAGAIN && errno!=EINTR){ fxerror("Application terminated: interrupt or lost connection errno=%d\n",errno); }
      return false;
      }

    // Examine the input file descriptors
    for(FXInputHandle fff=0; fff<=maxhandle; fff++){
      FXInput in=inputs[fff];
      if(fff==displayfd) continue;
      if(FD_ISSET(fff,&readfds)){
        if(in.read.target && in.read.target->tryHandle(this,FXSEL(SEL_IO_READ,in.read.message),in.read.data)) refresh();
        }
      if(FD_ISSET(fff,&writefds)){
        if(in.write.target && in.write.target->tryHandle(this,FXSEL(SEL_IO_WRITE,in.write.message),in.write.data)) refresh();
        }
      if(FD_ISSET(fff,&exceptfds)){
        if(in.excpt.target && in.excpt.target->tryHandle(this,FXSEL(SEL_IO_EXCEPT,in.excpt.message),in.excpt.data)) refresh();
        }
      }

    // Nothing arrived on the connection
    if(!initialized || !app || !app->display || displayfd<0 || !FD_ISSET(displayfd,&readfds)) return false;

    // Read what the compositor sent; the listeners queue the events
    if(wl_display_dispatch(app->display)<0){
      app->lost=true;
      return false;
      }
    }

  // Take the next queued event
  if(app->eventcount!=app->eventhead){
    ev=app->events[app->eventhead++];
    if(app->eventhead==app->eventcount){
      app->eventhead=0;
      app->eventcount=0;
      }
    return true;
    }

  return false;
  }


/*******************************************************************************/

// Dispatch a raw event
FXbool FXApp::dispatchEvent(FXRawEvent& ev){
  FXWaylandApp* app=fxwayland(display);
  FXWindow* window;
  FXEvent event;
  if(!app) return false;
  switch(ev.code){
    case FXXWL_EXPOSE:
      if(ev.id){
        window=findWindowWithId(ev.id);
        if(window){
          event.type=SEL_PAINT;
          event.rect.x=ev.x;
          event.rect.y=ev.y;
          event.rect.w=ev.w;
          event.rect.h=ev.h;
          event.synthetic=(ev.state!=0);
          window->handle(this,FXSEL(SEL_PAINT,0),&event);
          return true;
          }
        }
      return false;
    }
  return false;
  }


// Dispatch a window message; no such messages on this platform
FXival FXApp::dispatchEvent(FXID,FXuint,FXuval,FXival){
  return 0;
  }


/*******************************************************************************/

// Smart rectangle compositing algorithm; the repaint list is platform neutral,
// and its rectangles are what the event loop hands to the windows as expose
void FXApp::addRepaint(FXID win,FXint x,FXint y,FXint w,FXint h,FXbool synth){
  FXint px,py,pw,ph,newarea,area;
  FXRepaint *r,**pr;
  area=w*h;
  w+=x;
  h+=y;
  do{

    // Find overlap with outstanding rectangles
    for(r=repaints,pr=&repaints; r; pr=&r->next,r=r->next){
      if(r->window==win){

        // Tentatively conglomerate rectangles
        px=FXMIN(x,r->rect.x);
        py=FXMIN(y,r->rect.y);
        pw=FXMAX(w,r->rect.w);
        ph=FXMAX(h,r->rect.h);
        newarea=(pw-px)*(ph-py);

        // New area bigger than sum; forget about it
        if(newarea > area+r->area) continue;

        // Take old paintrect out of the list
        *pr=r->next;
        r->next=repaintrecs;
        repaintrecs=r;

        // New rectangle
        synth|=r->synth;        // Synthethic is preserved!
        area=newarea;
        x=px;
        y=py;
        w=pw;
        h=ph;
        break;
        }
      }
    }
  while(r);

  // Get rectangle, recycled if possible
  if(repaintrecs){
    r=repaintrecs;
    repaintrecs=r->next;
    }
  else{
    r=new FXRepaint;
    }

  // Fill it
  r->window=win;
  r->rect.x=x;
  r->rect.y=y;
  r->rect.w=w;
  r->rect.h=h;
  r->area=area;
  r->synth=synth;
  r->next=nullptr;
  *pr=r;
  }


// Remove repaints by dispatching them; there are no server side exposes to
// merge with, so the rectangles of the list are painted right away
void FXApp::removeRepaints(FXID win,FXint x,FXint y,FXint w,FXint h){
  FXRepaint *r,**rr;
  FXRawEvent ev;
  w+=x;
  h+=y;
  rr=&repaints;
  while((r=*rr)!=nullptr){
    if(!win || (win==r->window && x<r->rect.w && y<r->rect.h && r->rect.x<w && r->rect.y<h)){
      *rr=r->next;
      clearElms(&ev,1);
      ev.id=r->window;
      ev.x=r->rect.x;
      ev.y=r->rect.y;
      ev.w=r->rect.w-r->rect.x;
      ev.h=r->rect.h-r->rect.y;
      ev.code=FXXWL_EXPOSE;
      ev.state=r->synth;
      r->next=repaintrecs;
      repaintrecs=r;
      dispatchEvent(ev);
      continue;
      }
    rr=&r->next;
    }
  }


// Return the window at the given root coordinates
FXWindow* FXApp::findWindowAt(FXint rx,FXint ry,FXID window) const {
  return nullptr;
  }


/*******************************************************************************/

// Selection and clipboard data; the data device of wl_data_device arrives in
// phase 8, so the entry points exist but do nothing yet
void FXApp::selectionSetData(const FXWindow*,FXDragType,FXuchar*,FXuint){}
void FXApp::selectionGetData(const FXWindow*,FXDragType,FXuchar*& data,FXuint& size){ data=nullptr; size=0; }
void FXApp::selectionGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){ types=nullptr; numtypes=0; }
void FXApp::clipboardSetData(const FXWindow*,FXDragType,FXuchar*,FXuint){}
void FXApp::clipboardGetData(const FXWindow*,FXDragType,FXuchar*& data,FXuint& size){ data=nullptr; size=0; }
void FXApp::clipboardGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){ types=nullptr; numtypes=0; }
void FXApp::dragdropSetData(const FXWindow*,FXDragType,FXuchar*,FXuint){}
void FXApp::dragdropGetData(const FXWindow*,FXDragType,FXuchar*& data,FXuint& size){ data=nullptr; size=0; }
void FXApp::dragdropGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){ types=nullptr; numtypes=0; }

}

#endif
