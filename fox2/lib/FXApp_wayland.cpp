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
#include "fxplatformwindow.h"
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
      fxwaylandOpenSeat(app);
      fxwaylandOpenDataDevice(app);
      fxwaylandOpenTextInput(app);
      }
    }
  else if(strcmp(interface,wl_output_interface.name)==0){
    wl_output* output=(wl_output*)wl_registry_bind(registry,name,&wl_output_interface,FXMIN(version,2u));
    if(output){
      if(!app->output) app->output=output;      // The first one sizes the root
      fxwaylandAddOutput(app,output);
      }
    }

  // Exact scales and the viewport that maps a buffer onto a logical size; the
  // compositor's fractional scale is what a HiDPI screen with 1.5 asks for
  else if(strcmp(interface,wp_viewporter_interface.name)==0){
    if(!app->viewporter){
      app->viewporter=(wp_viewporter*)wl_registry_bind(registry,name,&wp_viewporter_interface,1);
      }
    }
  else if(strcmp(interface,wp_fractional_scale_manager_v1_interface.name)==0){
    if(!app->fsmanager){
      app->fsmanager=(wp_fractional_scale_manager_v1*)wl_registry_bind(registry,name,&wp_fractional_scale_manager_v1_interface,1);
      }
    }
  else if(strcmp(interface,xdg_wm_base_interface.name)==0){
    app->wmbase=(xdg_wm_base*)wl_registry_bind(registry,name,&xdg_wm_base_interface,FXMIN(version,2u));
    }
  else if(strcmp(interface,zxdg_decoration_manager_v1_interface.name)==0){
    if(!app->decomanager){
      app->decomanager=(zxdg_decoration_manager_v1*)wl_registry_bind(registry,name,&zxdg_decoration_manager_v1_interface,1);
      }
    }

  // Clipboard and drag and drop; the data device is made as soon as both the
  // manager and the seat are there, whichever arrives first
  else if(strcmp(interface,wl_data_device_manager_interface.name)==0){
    if(!app->ddmanager){
      app->ddmanager=(wl_data_device_manager*)wl_registry_bind(registry,name,&wl_data_device_manager_interface,FXMIN(version,3u));
      fxwaylandOpenDataDevice(app);
      }
    }

  // Activation, so that a window we show can ask for the focus on its own
  else if(strcmp(interface,xdg_activation_v1_interface.name)==0){
    if(!app->activation){
      app->activation=(xdg_activation_v1*)wl_registry_bind(registry,name,&xdg_activation_v1_interface,1);
      }
    }

  // Text input, offered by compositors that run an input method
  else if(strcmp(interface,zwp_text_input_manager_v3_interface.name)==0){
    if(!app->timanager){
      app->timanager=(zwp_text_input_manager_v3*)wl_registry_bind(registry,name,&zwp_text_input_manager_v3_interface,1);
      fxwaylandOpenTextInput(app);
      }
    }

  // The X11 style primary selection, offered by some compositors only
  else if(strcmp(interface,zwp_primary_selection_device_manager_v1_interface.name)==0){
    if(!app->psmanager){
      app->psmanager=(zwp_primary_selection_device_manager_v1*)wl_registry_bind(registry,name,&zwp_primary_selection_device_manager_v1_interface,1);
      fxwaylandOpenDataDevice(app);
      fxwaylandAttachSelectionDevice(app);
      }
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


// The outputs and the scale they prefer are handled by the scale module; the
// first output is also the one whose mode sizes the root window


static const wl_registry_listener registryListener={
  registryGlobal,
  registryGlobalRemove
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
  app->decomanager=nullptr;
  app->activation=nullptr;
  app->activtoken=nullptr;
  app->viewporter=nullptr;
  app->fsmanager=nullptr;
  app->outputs=nullptr;
  app->timanager=nullptr;
  app->tiinput=nullptr;
  app->tisurface=nullptr;
  app->tix=0;
  app->tiy=0;
  app->tiw=0;
  app->tih=0;
  app->tienabled=false;
  app->ddmanager=nullptr;
  app->ddevice=nullptr;
  app->dsource=nullptr;
  app->doffer=nullptr;
  app->psmanager=nullptr;
  app->psdevice=nullptr;
  app->pssource=nullptr;
  app->psoffer=nullptr;
  app->offertypes=nullptr;
  app->numoffertypes=0;
  app->offermimes=nullptr;
  app->allocedoffer=0;
  app->psoffertypes=nullptr;
  app->numpsoffertypes=0;
  app->psoffermimes=nullptr;
  app->allocedpsoffer=0;
  app->typenames=nullptr;
  app->numtypes=0;
  app->allocedtypes=0;
  app->keyboardserial=0;

  // Clipboard, selection and drag and drop: the type lists are ours, the data
  // exchange fields are shared with the other platforms
  xcbSelection=0;
  xcbTypeList=nullptr;
  xcbNumTypes=0;
  xselTypeList=nullptr;
  xselNumTypes=0;
  ddeTypeList=nullptr;
  ddeNumTypes=0;
  ddeAction=DRAG_REJECT;
  ansAction=DRAG_REJECT;
  xdndTypeList=nullptr;
  xdndNumTypes=0;
  app->screenwidth=0;
  app->screenheight=0;
  app->rootwindow=0;
  app->foxapp=this;
  app->pointer=nullptr;
  app->keyboard=nullptr;
  app->xkbcontext=nullptr;
  app->xkbkeymap=nullptr;
  app->xkbstate=nullptr;
  app->pointerwindow=0;
  app->pointerbuttons=0;
  app->modstate=0;
  app->pointersurface=nullptr;
  app->keyboardsurface=nullptr;
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
    freeElms(app->typenames);
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

  // The scale of the outputs is the display scale; it must be known before the
  // application creates its first window
  fxwaylandApplyStartupScale(app);

  // Preregister the drag types of FOX itself: a Wayland drag type is the index
  // of a mime type in a table of this connection, and the first two entries are
  // the fixed ones FXWindow declares as stringType and utf16Type
  fxwaylandRegisterType(app,"STRING");         // Must be the first: FXWindow::stringType is 1
  FXWindow::utf16Type=fxwaylandRegisterType(app,FXWindow::utf16TypeName);
  FXWindow::octetType=fxwaylandRegisterType(app,FXWindow::octetTypeName);
  FXWindow::deleteType=fxwaylandRegisterType(app,FXWindow::deleteTypeName);
  FXWindow::textType=fxwaylandRegisterType(app,FXWindow::textTypeName);
  FXWindow::colorType=fxwaylandRegisterType(app,FXWindow::colorTypeName);
  FXWindow::urilistType=fxwaylandRegisterType(app,FXWindow::urilistTypeName);
  FXWindow::utf8Type=fxwaylandRegisterType(app,FXWindow::utf8TypeName);
  FXWindow::actionType=fxwaylandRegisterType(app,FXWindow::actionTypeName);
  fxwaylandTrace("registered %u drag types\n",app->numtypes);

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

  // The windows hold the surfaces, so they have to go before the objects the
  // surfaces were made from
  if(initialized){
    destroy();
    initialized=false;
    }

  // Anything left over goes now
  while(app->surfaces){
    fxwaylandDestroySurface(app,app->surfaces->window);
    }

  // The data devices hang off the seat, so they go before it
  fxwaylandCloseDataDevice(app);
  fxwaylandCloseSeat(app);
  fxwaylandCloseActivation(app);
  fxwaylandCloseTextInput(app);
  if(app->fsmanager){ wp_fractional_scale_manager_v1_destroy(app->fsmanager); app->fsmanager=nullptr; }
  if(app->viewporter){ wp_viewporter_destroy(app->viewporter); app->viewporter=nullptr; }
  fxwaylandFreeOutputs(app);
  if(app->timanager){ zwp_text_input_manager_v3_destroy(app->timanager); app->timanager=nullptr; }
  if(app->decomanager){ zxdg_decoration_manager_v1_destroy(app->decomanager); app->decomanager=nullptr; }
  if(app->wmbase){ xdg_wm_base_destroy(app->wmbase); app->wmbase=nullptr; }
  app->output=nullptr;                    // The outputs are dropped above
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
// The widget under a point of a shell surface: the drag events of the protocol
// give the position in surface coordinates, the table speaks screen coordinates
static FXWindow* widgetAt(FXApp* foxapp,FXWaylandApp* app,FXID shell,FXint x,FXint y){
  FXWaylandSurface* s;
  const FXWinEntry* entry;
  FXint rx=x;
  FXint ry=y;
  FXID id;
  if((entry=app->childmap.getEntry(shell))!=nullptr){
    s=fxwaylandFindSurface(app,shell);
    rx=entry->x+x;
    ry=entry->y+y-(s?s->barh:0);
    }
  id=app->childmap.hitTest(rx,ry);
  return id?foxapp->findWindowWithId(id):nullptr;
  }


FXbool FXApp::dispatchEvent(FXRawEvent& ev){
  const FXTime milliseconds=1000000;
  FXWaylandApp* app=fxwayland(display);
  const FXWinEntry* entry;
  FXWaylandSurface* shellsurface;
  FXWindow* window;
  FXWindow* ancestor;
  FXint rx,ry;
  FXbool fresh;
  FXEvent event;
  if(!app) return false;

  // A fresh event for every dispatch: the click counting and the moved flag
  // read fields that only a previous event would have set.  It is assigned
  // rather than cleared, because the text is an FXString
  event=FXEvent();
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

    // The compositor configured a shell; it decides the size of a maximized or
    // full screen window and tells us when the user resizes, but it never says
    // where the window is, so the position stays what the core decided
    case FXXWL_CONFIGURE:
      if((window=findWindowWithId(ev.id))!=nullptr){
        fxwaylandTrace("dispatch configure window %u %dx%d (was %dx%d)\n",(unsigned int)ev.id,ev.w,ev.h,window->getWidth(),window->getHeight());
        event.type=SEL_CONFIGURE;
        event.rect.x=window->getX();
        event.rect.y=window->getY();
        event.rect.w=ev.w>0?ev.w:window->getWidth();
        event.rect.h=ev.h>0?ev.h:window->getHeight();
        event.synthetic=true;
        if(window->handle(this,FXSEL(SEL_CONFIGURE,0),&event)) refresh();

        // The core keeps the size the compositor asked for, so the table has to
        // follow it; the frame buffer and the hit test both come from the table
        fxplatformPlaceWindow(display,ev.id,window->getX(),window->getY(),window->getWidth(),window->getHeight(),0,0,window->shown(),PLACEMENT_X|PLACEMENT_Y|PLACEMENT_W|PLACEMENT_H);

        // The size may have changed, and the frame buffer follows the window
        fxwaylandExpose(app,ev.id);
        return true;
        }
      return false;

    // The compositor dismissed the popup, or its parent went away
    case FXXWL_POPUPDONE:
      if((window=findWindowWithId(ev.id))!=nullptr){
        window->hide();
        return true;
        }
      return false;

    // The compositor asked the user's window to go away
    case FXXWL_CLOSE:
      if((window=findWindowWithId(ev.id))!=nullptr){
        event.type=SEL_CLOSE;
        if(window->handle(this,FXSEL(SEL_CLOSE,0),&event)) refresh();
        return true;
        }
      return false;

    // A drag is over one of our windows: the widget under the pointer decides
    // whether it takes the drop, and with which action
    case FXXWL_DNDENTER:
    case FXXWL_DNDMOTION:
    case FXXWL_DNDLEAVE:
    case FXXWL_DNDDROP:
      if(ev.code==FXXWL_DNDLEAVE){
        if(dropWindow){
          event.type=SEL_DND_LEAVE;
          if(dropWindow->handle(this,FXSEL(SEL_DND_LEAVE,0),&event)) refresh();
          dropWindow=nullptr;
          }
        return true;
        }
      if(ev.code==FXXWL_DNDDROP){
        if(dropWindow){
          fxwaylandTrace("dispatch drop to %s\n",dropWindow->getClassName());
          event.type=SEL_DND_DROP;
          event.target=0;
          ansAction=DRAG_REJECT;
          if(dropWindow->handle(this,FXSEL(SEL_DND_DROP,0),&event)) refresh();
          fxwaylandFinishDrop(app,ansAction);
          dropWindow=nullptr;
          }
        return true;
        }
      window=widgetAt(this,app,ev.id,ev.x,ev.y);
      fresh=false;
      if(window!=dropWindow){
        if(dropWindow){
          event.type=SEL_DND_LEAVE;
          if(dropWindow->handle(this,FXSEL(SEL_DND_LEAVE,0),&event)) refresh();
          }
        dropWindow=nullptr;
        if(window && window->isDropEnabled()){
          dropWindow=window;
          fresh=true;
          }
        }
      if(dropWindow==nullptr) return true;

      // What the source allows becomes the action the core knows
      if(ev.button&WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) ddeAction=DRAG_COPY;
      else if(ev.button&WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) ddeAction=DRAG_MOVE;
      else if(ev.button&WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK) ddeAction=DRAG_ASK;
      else ddeAction=DRAG_COPY;
      ansAction=DRAG_REJECT;
      event.type=(ev.code==FXXWL_DNDENTER || fresh)?SEL_DND_ENTER:SEL_DND_MOTION;
      event.target=0;
      event.root_x=ev.x;
      event.root_y=ev.y;
      dropWindow->translateCoordinatesFrom(event.win_x,event.win_y,root,ev.x,ev.y);
      if(dropWindow->handle(this,FXSEL(event.type,0),&event)) refresh();

      // Tell the source whether we take it, and which type we would read
      {
        FXDragType want=event.target;
        FXDragType* types=nullptr;
        FXuint numtypes=0;
        fxwaylandDragTypes(app,types,numtypes);
        if(want==0 && numtypes) want=types[0];
        freeElms(types);
        fxwaylandAcceptDrag(app,ansAction,want);
      }
      fxwaylandTrace("drag %s on %s, action %u\n",event.type==SEL_DND_ENTER?"enter":"motion",dropWindow->getClassName(),(unsigned int)ansAction);
      return true;

    // Pointer and keyboard; where the pointer is inside the shell is in the
    // event, and the place of the shell on the screen is in the table
    case FXXWL_ENTER:
    case FXXWL_LEAVE:
    case FXXWL_MOTION:
    case FXXWL_BUTTON:
    case FXXWL_WHEEL:
      if((window=findWindowWithId(ev.id))==nullptr) return false;
      shellsurface=fxwaylandShellSurface(app,ev.data[0]);

      // A press inside the client side title bar moves the window or uses one
      // of its buttons; nothing below the bar ever sees it
      if(ev.code==FXXWL_BUTTON && ev.data[1] && shellsurface && fxwaylandBarPress(app,shellsurface,ev.x,ev.y)){
        return true;
        }

      // Screen position of the point; the compositor only tells us where it
      // is inside the surface of the shell, which may start with a title bar
      if((entry=app->childmap.getEntry(ev.data[0]))!=nullptr){
        rx=entry->x+ev.x;
        ry=entry->y+ev.y-(shellsurface?shellsurface->barh:0);
        }
      else{
        rx=ev.x;
        ry=ev.y;
        }

      // The pointer arrived at another window: the chain of SEL_ENTER and
      // SEL_LEAVE is kept here, because the compositor only reports where the
      // pointer is inside a surface and never says which widget it left.  The
      // helpers read the event of the application, not this local one, and the
      // crossing mode decides whether a widget arms its tool tip.
      this->event.time=ev.time;
      this->event.state=ev.state;
      this->event.code=mouseGrabWindow?CROSSINGGRAB:CROSSINGNORMAL;
      this->event.root_x=rx;
      this->event.root_y=ry;
      if(ev.code==FXXWL_LEAVE){
        if(cursorWindow) leaveWindow(cursorWindow,root);
        return true;
        }
      if(cursorWindow!=window){
        ancestor=FXWindow::commonAncestor(window,cursorWindow);
        leaveWindow(cursorWindow,ancestor);
        enterWindow(window,ancestor);
        }
      if(ev.code==FXXWL_ENTER) return true;

      // Where the point is inside the window that gets the event
      event.time=ev.time;
      event.state=ev.state;
      event.root_x=rx;
      event.root_y=ry;
      window->translateCoordinatesFrom(event.win_x,event.win_y,root,rx,ry);

      // The mouse wheel
      if(ev.code==FXXWL_WHEEL){
        event.type=SEL_MOUSEWHEEL;
        event.code=ev.data[1];
        if(mouseGrabWindow){
          window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
          if(mouseGrabWindow->handle(this,FXSEL(SEL_MOUSEWHEEL,0),&event)) refresh();
          return true;
          }
        while(window){
          if(window->handle(this,FXSEL(SEL_MOUSEWHEEL,0),&event)){ refresh(); break; }
          window=window->getParent();
          }
        return true;
        }

      // Motion
      if(ev.code==FXXWL_MOTION){
        event.type=SEL_MOTION;
        event.code=0;
        if((FXABS(event.root_x-event.rootclick_x)>=dragDelta) || (FXABS(event.root_y-event.rootclick_y)>=dragDelta)) event.moved=1;
        if(mouseGrabWindow){
          window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
          if(mouseGrabWindow->handle(this,FXSEL(SEL_MOTION,0),&event)) refresh();
          }
        else if(window->handle(this,FXSEL(SEL_MOTION,0),&event)) refresh();
        event.last_x=event.win_x;
        event.last_y=event.win_y;
        return true;
        }

      // Mouse buttons; the press counts clicks the way the core does it
      event.code=ev.button;
      if(ev.data[1]){
        if(ev.button==1){ event.type=SEL_LEFTBUTTONPRESS; event.state|=LEFTBUTTONMASK; }
        if(ev.button==2){ event.type=SEL_MIDDLEBUTTONPRESS; event.state|=MIDDLEBUTTONMASK; }
        if(ev.button==3){ event.type=SEL_RIGHTBUTTONPRESS; event.state|=RIGHTBUTTONMASK; }
        if(!event.moved && (event.code==event.click_button) && (milliseconds*(event.time-event.click_time)<clickSpeed)){
          event.click_count++;
          event.click_time=event.time;
          }
        else{
          event.click_count=1;
          event.click_x=event.win_x;
          event.click_y=event.win_y;
          event.rootclick_x=event.root_x;
          event.rootclick_y=event.root_y;
          event.click_button=event.code;
          event.click_time=event.time;
          }
        if((event.state&(LEFTBUTTONMASK|MIDDLEBUTTONMASK|RIGHTBUTTONMASK))!=0) event.moved=0;
        }
      else{
        if(ev.button==1){ event.type=SEL_LEFTBUTTONRELEASE; event.state&=~LEFTBUTTONMASK; }
        if(ev.button==2){ event.type=SEL_MIDDLEBUTTONRELEASE; event.state&=~MIDDLEBUTTONMASK; }
        if(ev.button==3){ event.type=SEL_RIGHTBUTTONRELEASE; event.state&=~RIGHTBUTTONMASK; }
        }
      if(mouseGrabWindow){
        window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
        if(mouseGrabWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
        }
      else if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
      fxwaylandTrace("button %u %s to %s at %+d%+d\n",ev.button,ev.data[1]?"down":"up",(mouseGrabWindow?mouseGrabWindow:window)->getClassName(),event.win_x,event.win_y);
      event.last_x=event.win_x;
      event.last_y=event.win_y;
      return true;

    case FXXWL_FOCUSIN:
    case FXXWL_FOCUSOUT:
      if((window=findWindowWithId(ev.id))!=nullptr){
        window=window->getShell();
        event.type=(ev.code==FXXWL_FOCUSIN)?SEL_FOCUSIN:SEL_FOCUSOUT;
        if(ev.code==FXXWL_FOCUSOUT && activeWindow==window){
          if(window->handle(this,FXSEL(SEL_FOCUSOUT,0),&event)) refresh();
          activeWindow=nullptr;
          fxwaylandDisableTextInput(app);
          }
        if(ev.code==FXXWL_FOCUSIN && activeWindow!=window){

          // The compositor handed the keyboard to this shell, so the shell
          // focuses its first widget that wants the focus; the core has no
          // other way to learn that the keyboard arrived
          if(window->getFocus()==nullptr){
            window->handle(window,FXSEL(SEL_FOCUS_NEXT,0),nullptr);
            }
          if(window->handle(this,FXSEL(SEL_FOCUSIN,0),&event)) refresh();
          activeWindow=window;
          fxwaylandEnableTextInput(app);
          }
        return true;
        }
      return false;

    case FXXWL_KEY:
      window=keyboardGrabWindow?keyboardGrabWindow:getFocusWindow();
      if(window){
        event.type=ev.button?SEL_KEYPRESS:SEL_KEYRELEASE;
        event.time=ev.time;
        event.code=fxwaylandKeySym(app,(FXuint)ev.data[0]);
        event.state=ev.state;
        if(ev.button) event.text=fxwaylandKeyText(app,(FXuint)ev.data[0]);
        if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
        fxwaylandTrace("key %s to %s code %x text \"%s\"\n",ev.button?"press":"release",window->getClassName(),event.code,event.text.text());
        return true;
        }
      fxwaylandTrace("key %s dropped, no focus window\n",ev.button?"press":"release");
      return false;

    case FXXWL_IME:

      // An input method does not press keys; it hands over the text it composed
      window=keyboardGrabWindow?keyboardGrabWindow:getFocusWindow();
      if(window){
        event.code=0;
        event.state=0;
        event.text=ev.text;
        event.type=SEL_KEYPRESS;
        if(window->handle(this,FXSEL(SEL_KEYPRESS,0),&event)) refresh();
        event.type=SEL_KEYRELEASE;
        if(window->handle(this,FXSEL(SEL_KEYRELEASE,0),&event)) refresh();
        fxwaylandTrace("ime \"%s\" to %s\n",ev.text,window->getClassName());
        return true;
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

// Register a drag type; a Wayland drag type is the index of a mime type in a
// table of the connection, and the name of the type is that mime type
FXDragType fxwaylandRegisterType(FXWaylandApp* app,const FXString& mime){
  FXString* grown;
  FXuint i;
  if(!app || mime.empty()) return 0;
  for(i=0; i<app->numtypes; i++){
    if(app->typenames[i]==mime) return i+1;
    }
  if(app->numtypes>=app->allocedtypes){
    FXuint n=app->allocedtypes?app->allocedtypes*2:16;
    if((grown=(FXString*)realloc(app->typenames,n*sizeof(FXString)))==nullptr) return 0;
    app->typenames=grown;
    app->allocedtypes=n;
    }
  constructElms(&app->typenames[app->numtypes],1);
  app->typenames[app->numtypes]=mime;
  app->numtypes+=1;
  fxwaylandTrace("drag type %u is %s\n",app->numtypes,mime.text());
  return app->numtypes;
  }


// Mime type of a drag type of this connection
FXString fxwaylandTypeName(FXWaylandApp* app,FXDragType type){
  if(app && type>=1 && type<=app->numtypes) return app->typenames[type-1];
  return FXString::null;
  }


// Drag type of a mime type a peer offers; the well known spellings of the same
// thing collapse onto the type FOX already knows, everything else is new
FXDragType fxwaylandTypeFromMime(FXWaylandApp* app,const FXchar* mime){
  FXString name(mime?mime:"");
  if(!app || name.empty()) return 0;
  if(name=="text/plain;charset=utf-8" || name=="UTF8_STRING" || name=="text/plain;charset=UTF-8"){
    return FXWindow::utf8Type;
    }
  if(name=="text/plain" || name=="TEXT" || name=="COMPOUND_TEXT"){
    return FXWindow::textType;
    }
  if(name=="STRING") return FXWindow::stringType;
  if(name=="text/utf16" || name=="text/plain;charset=utf-16") return FXWindow::utf16Type;
  if(name=="text/uri-list") return FXWindow::urilistType;
  if(name=="application/x-color") return FXWindow::colorType;
  if(name=="application/octet-stream") return FXWindow::octetType;
  return fxwaylandRegisterType(app,name);
  }


// Ask the window that owns the clipboard or the selection for the data of a
// type; going through FXWindow::getDNDData keeps the request dance in one place
FXbool fxwaylandOfferData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size,FXuint origin){
  FXApp* foxapp;
  FXWindow* owner=nullptr;
  data=nullptr;
  size=0;
  if(!app || !(foxapp=(FXApp*)app->foxapp)) return false;
  switch(origin){
    case FXXWL_FROM_CLIPBOARD: owner=foxapp->getClipboardWindow(); break;
    case FXXWL_FROM_SELECTION: owner=foxapp->getSelectionWindow(); break;
    default: break;
    }
  if(!owner) return false;
  return owner->getDNDData((FXDNDOrigin)origin,type,data,size);
  }


// The compositor handed the clipboard to somebody else.  FXWindow::releaseClipboard()
// does the bookkeeping FOX expects (SEL_CLIPBOARD_LOST, the type list, the owner)
// and the module has already dropped the source, so nothing is cleared twice
void fxwaylandClipboardLost(FXWaylandApp* app){
  FXApp* foxapp;
  FXWindow* owner;
  if(!app || !(foxapp=(FXApp*)app->foxapp)) return;
  if((owner=foxapp->getClipboardWindow())!=nullptr){
    owner->releaseClipboard();
    }
  }


void fxwaylandSelectionLost(FXWaylandApp* app){
  FXApp* foxapp;
  FXWindow* owner;
  if(!app || !(foxapp=(FXApp*)app->foxapp)) return;
  if((owner=foxapp->getSelectionWindow())!=nullptr){
    owner->releaseSelection();
    }
  }


/*******************************************************************************/

// Selection and clipboard data.  While this application owns the board the data
// comes from the window that owns it, exactly as on X11; otherwise it comes from
// the offer of the peer, which the data device keeps up to date
void FXApp::selectionSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


void FXApp::selectionGetData(const FXWindow*,FXDragType type,FXuchar*& data,FXuint& size){
  FXWaylandApp* app=fxwayland(display);
  data=nullptr;
  size=0;
  if(selectionWindow){
    event.type=SEL_SELECTION_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    selectionWindow->handle(this,FXSEL(SEL_SELECTION_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    fxwaylandSelectionData(app,type,data,size);
    }
  }


void FXApp::selectionGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  FXWaylandApp* app=fxwayland(display);
  types=nullptr;
  numtypes=0;
  if(selectionWindow){
    if(xselTypeList && xselNumTypes){
      dupElms(types,xselTypeList,xselNumTypes);
      numtypes=xselNumTypes;
      }
    }
  else{
    fxwaylandSelectionTypes(app,types,numtypes);
    }
  }


void FXApp::clipboardSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


void FXApp::clipboardGetData(const FXWindow*,FXDragType type,FXuchar*& data,FXuint& size){
  FXWaylandApp* app=fxwayland(display);
  data=nullptr;
  size=0;

  // Our own clipboard: ask the window that owns it
  if(clipboardWindow){
    event.type=SEL_CLIPBOARD_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    clipboardWindow->handle(this,FXSEL(SEL_CLIPBOARD_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    fxwaylandClipboardData(app,type,data,size);
    }
  }


void FXApp::clipboardGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  FXWaylandApp* app=fxwayland(display);
  types=nullptr;
  numtypes=0;
  if(clipboardWindow){
    if(xcbTypeList && xcbNumTypes){
      dupElms(types,xcbTypeList,xcbNumTypes);
      numtypes=xcbNumTypes;
      }
    }
  else{
    fxwaylandClipboardTypes(app,types,numtypes);
    }
  }


void FXApp::dragdropSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


void FXApp::dragdropGetData(const FXWindow*,FXDragType type,FXuchar*& data,FXuint& size){
  FXWaylandApp* app=fxwayland(display);
  data=nullptr;
  size=0;

  // We are the source: ask the window that started the drag, the way X11 asks it
  // through SEL_DND_REQUEST
  if(dragWindow && dragWindow->id()){
    event.type=SEL_DND_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    dragWindow->handle(this,FXSEL(SEL_DND_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    fxwaylandDragData(app,type,data,size);
    }
  }


void FXApp::dragdropGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  FXWaylandApp* app=fxwayland(display);
  types=nullptr;
  numtypes=0;
  if(dragWindow){
    if(xdndTypeList && xdndNumTypes){
      dupElms(types,xdndTypeList,xdndNumTypes);
      numtypes=xdndNumTypes;
      }
    }
  else{
    fxwaylandDragTypes(app,types,numtypes);
    }
  }


/*******************************************************************************/

// Register a drag type; a Wayland drag type is the index of a mime type
FXDragType FXApp::registerDragType(const FXString& name) const {
  return fxwaylandRegisterType(fxwayland(display),name);
  }


// Get name of registered drag type
FXString FXApp::getDragTypeName(FXDragType type) const {
  return fxwaylandTypeName(fxwayland(display),type);
  }

}

#endif
