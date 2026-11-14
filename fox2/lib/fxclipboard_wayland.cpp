/********************************************************************************
*                                                                               *
*                     C l i p b o a r d   ( W a y l a n d )                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*********************************************************************************
* This program is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
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
#include "FXEvent.h"
#include "fxwaylandpriv.h"

#include <poll.h>
#include <unistd.h>
#include <errno.h>

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Wayland has a data device instead of selections and atoms.  A client offers
    a wl_data_source carrying a list of mime types, and a peer asks that source
    to write one mime type into a pipe it hands over.  Reading the clipboard is
    the same in reverse: we ask the offer of the peer for a mime type and read
    the pipe.  Nothing can be queried synchronously, so the offer of the peer is
    kept from the moment wl_data_device.selection announces it.
  - The mime type that came with an offer is kept next to the drag type, because
    a peer may spell the same thing differently ("UTF8_STRING" and
    "text/plain;charset=utf-8" are one type for FOX but two names to ask for).
  - The clipboard text is asked from the application when a peer asks for it,
    not when it is copied: the source lives as long as the clipboard is ours.
  - set_selection wants the serial of the input event that led to the copy, so
    the module remembers the serial of the last pointer and keyboard events.
  - The X11 primary selection has a protocol of its own, offered by some
    compositors only, and works exactly like the clipboard.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Write a buffer into the pipe of a peer and close it
static void writeAll(int fd,const FXuchar* data,FXuint size){
  const FXuchar* p=data;
  FXival n;
  while(size>0){
    n=::write(fd,p,size);
    if(n<0){
      if(errno==EINTR) continue;
      break;
      }
    if(n==0) break;
    p+=n;
    size-=n;
    }
  ::close(fd);
  }


// Read everything a peer writes into the pipe, with a limit on the wait
static FXuchar* readAll(int fd,FXuint& size){
  FXuchar* data=nullptr;
  FXuchar* grown;
  FXuint alloced=0;
  FXival n;
  struct pollfd pfd;
  size=0;
  while(true){
    pfd.fd=fd;
    pfd.events=POLLIN;
    pfd.revents=0;
    n=poll(&pfd,1,2000);
    if(n<=0) break;                             // Timeout or error ends the read
    if(size+4096>alloced){
      alloced=size+4096;
      if(!(grown=(FXuchar*)realloc(data,alloced))){ free(data); ::close(fd); size=0; return nullptr; }
      data=grown;
      }
    n=::read(fd,data+size,alloced-size);
    if(n<0){
      if(errno==EINTR) continue;
      break;
      }
    if(n==0) break;                             // End of the data
    size+=n;
    }
  ::close(fd);
  return data;
  }


// Remember one mime type of an offer
static void addOfferType(FXWaylandApp* app,FXDragType type,const FXString& mime,FXbool primary){
  FXString* mimes;
  FXDragType* types;
  FXuint* count=primary?&app->numpsoffertypes:&app->numoffertypes;
  FXuint* alloced=primary?&app->allocedpsoffer:&app->allocedoffer;
  FXString** mimelist=primary?&app->psoffermimes:&app->offermimes;
  FXDragType** typelist=primary?&app->psoffertypes:&app->offertypes;
  if(*count>=*alloced){
    FXuint n=*alloced?*alloced*2:8;
    if(!(types=(FXDragType*)realloc(*typelist,n*sizeof(FXDragType)))) return;
    *typelist=types;
    if(!(mimes=(FXString*)realloc(*mimelist,n*sizeof(FXString)))) return;
    *mimelist=mimes;
    *alloced=n;
    }
  constructElms(&(*mimelist)[*count],1);
  (*typelist)[*count]=type;
  (*mimelist)[*count]=mime;
  *count+=1;
  }


// Mime type to ask a peer for, given the drag type it offered
static FXString mimeOfType(FXWaylandApp* app,FXDragType type,FXbool primary){
  FXuint i;
  FXuint n=primary?app->numpsoffertypes:app->numoffertypes;
  FXDragType* types=primary?app->psoffertypes:app->offertypes;
  FXString* mimes=primary?app->psoffermimes:app->offermimes;
  for(i=0; i<n; i++){
    if(types[i]==type) return mimes[i];
    }
  return FXString::null;
  }


// Drop what we know about an offer
static void clearOffer(FXWaylandApp* app,FXbool primary){
  FXuint i;
  FXString* mimes=primary?app->psoffermimes:app->offermimes;
  FXuint n=primary?app->numpsoffertypes:app->numoffertypes;
  for(i=0; i<n; i++){
    mimes[i].clear();
    }
  freeElms(mimes);
  if(primary){ app->psoffermimes=nullptr; app->numpsoffertypes=0; app->allocedpsoffer=0; }
  else{ app->offermimes=nullptr; app->numoffertypes=0; app->allocedoffer=0; }
  freeElms(primary?app->psoffertypes:app->offertypes);
  if(primary){ app->psoffertypes=nullptr; }
  else{ app->offertypes=nullptr; }
  }


/*******************************************************************************/

// Hand over the data a peer asks for, if the clipboard is ours
static void sourceSend(void* data,wl_data_source*,const char* mime,int32_t fd){
  FXWaylandApp* app=fxwayland(data);
  FXuchar* bytes=nullptr;
  FXuint size=0;
  FXDragType type;
  if(!app){ ::close(fd); return; }
  type=fxwaylandTypeFromMime(app,mime);
  if(type && fxwaylandOfferData(app,type,bytes,size,FXXWL_FROM_CLIPBOARD)){
    fxwaylandTrace("clipboard send %s, %u bytes\n",mime,size);
    writeAll(fd,bytes,size);
    freeElms(bytes);
    }
  else{
    fxwaylandTrace("clipboard send %s, we have no such data\n",mime);
    ::close(fd);
    }
  }


// Somebody else took the clipboard; what we offered is gone
static void sourceCancelled(void* data,wl_data_source* source){
  FXWaylandApp* app=fxwayland(data);
  if(app && app->dsource==source){
    fxwaylandTrace("clipboard cancelled\n");
    wl_data_source_destroy(app->dsource);
    app->dsource=nullptr;
    fxwaylandClipboardLost(app);
    }
  }


static void sourceTarget(void*,wl_data_source*,const char*){}


static const wl_data_source_listener sourceListener={
  sourceTarget,
  sourceSend,
  sourceCancelled
  };


/*******************************************************************************/

static void addDragType(FXWaylandApp* app,FXDragType type,const FXString& mime);


// A mime type of the offer of a peer; it may be the clipboard or a drag
static void offerOffer(void* data,wl_data_offer* offer,const char* mime){
  FXWaylandApp* app=fxwayland(data);
  FXDragType type;
  if(!app) return;
  type=fxwaylandTypeFromMime(app,mime);
  if(!type) return;
  if(offer==app->pendingoffer || offer==app->doffer){
    addOfferType(app,type,FXString(mime),false);
    }
  if(offer==app->pendingoffer || offer==app->dragoffer){
    addDragType(app,type,FXString(mime));
    }
  }


static void offerSourceActions(void*,wl_data_offer*,uint32_t){}

static void offerAction(void*,wl_data_offer*,uint32_t){}


static const wl_data_offer_listener offerListener={
  offerOffer,
  offerSourceActions,
  offerAction
  };


// Which slot owns an offer is settled by the selection or the enter that follows
static void takeDragOffer(FXWaylandApp* app,wl_data_offer* offer);
static void clearDragTypes(FXWaylandApp* app);
static void clearDrag(FXWaylandApp* app);
static FXString mimeOfDragType(FXWaylandApp* app,FXDragType type);


static void takeOffer(FXWaylandApp* app,wl_data_offer* offer){
  if(offer==app->doffer) return;
  if(app->doffer){
    clearOffer(app,false);
    app->doffer=nullptr;
    }
  if(offer){
    if(offer!=app->dragoffer && offer!=app->pendingoffer){
      wl_data_offer_add_listener(offer,&offerListener,app);
      }
    app->doffer=offer;
    }
  app->pendingoffer=nullptr;

  // The proxy belongs to the clipboard now, so the drag does not get to use it
  if(app->dragoffer==offer){
    app->dragoffer=nullptr;
    clearDragTypes(app);
    }
  }


// The clipboard changed hands; nullptr means nobody offers anything
static void deviceSelection(void* data,wl_data_device*,wl_data_offer* offer){
  FXWaylandApp* app=fxwayland(data);
  if(!app) return;
  takeOffer(app,offer);
  fxwaylandTrace("clipboard %s\n",offer?"offered by a peer":"empty");
  }


// An offer is announced before the protocol says what it is for: enter makes it a
// drag, selection makes it the clipboard.  Both slots take it for now, and each
// mime type is recorded in both lists until that is settled.
static void deviceDataOffer(void* data,wl_data_device*,wl_data_offer* offer){
  FXWaylandApp* app=fxwayland(data);
  if(!app || !offer) return;
  if(offer==app->pendingoffer || offer==app->doffer || offer==app->dragoffer) return;
  // The peer of the old clipboard offer is gone, so are the types it had
  if(app->doffer){
    wl_data_offer_destroy(app->doffer);
    app->doffer=nullptr;
    clearOffer(app,false);
    }
  app->pendingoffer=offer;
  wl_data_offer_add_listener(offer,&offerListener,app);
  fxwaylandTrace("data offer %p\n",(void*)offer);
  }


// Remember one mime type of the drag that is over us
static void addDragType(FXWaylandApp* app,FXDragType type,const FXString& mime){
  FXString* mimes;
  FXDragType* types;
  if(app->numdragtypes>=app->alloceddrag){
    FXuint n=app->alloceddrag?app->alloceddrag*2:8;
    if(!(types=(FXDragType*)realloc(app->dragtypes,n*sizeof(FXDragType)))) return;
    app->dragtypes=types;
    if(!(mimes=(FXString*)realloc(app->dragmimes,n*sizeof(FXString)))) return;
    app->dragmimes=mimes;
    app->alloceddrag=n;
    }
  constructElms(&app->dragmimes[app->numdragtypes],1);
  app->dragtypes[app->numdragtypes]=type;
  app->dragmimes[app->numdragtypes]=mime;
  app->numdragtypes+=1;
  }


// Tell the core that a drag arrived, moved, left or was dropped
static void postDrag(FXWaylandApp* app,FXuint code,wl_surface* surface,FXint x,FXint y,FXuint action){
  FXWaylandSurface* s;
  FXWaylandEvent ev;
  if(!app || !(s=fxwaylandSurfaceOf(app,surface))) return;
  clearElms(&ev,1);
  ev.code=code;
  ev.id=s->window;
  ev.x=x;
  ev.y=y;
  ev.button=(FXint)action;
  fxwaylandPost(app,ev);
  }


// Drop what we know about the drag offer
// Forget the mime types of the drag, without touching the proxy
static void clearDragTypes(FXWaylandApp* app){
  FXuint i;
  for(i=0; i<app->numdragtypes; i++){
    app->dragmimes[i].clear();
    }
  freeElms(app->dragmimes);
  app->dragmimes=nullptr;
  freeElms(app->dragtypes);
  app->dragtypes=nullptr;
  app->numdragtypes=0;
  app->alloceddrag=0;
  }


static void clearDrag(FXWaylandApp* app){
  clearDragTypes(app);
  if(app->dragoffer && app->dragoffer!=app->doffer && app->dragoffer!=app->pendingoffer){
    wl_data_offer_destroy(app->dragoffer);
    }
  app->dragoffer=nullptr;
  }


// Adopt the offer of the data that is being dragged
static void takeDragOffer(FXWaylandApp* app,wl_data_offer* offer){
  if(offer==app->dragoffer) return;

  // The offer was announced before the enter, so its mime types are recorded
  if(offer && offer==app->pendingoffer){
    app->dragoffer=offer;
    }
  else{
    clearDrag(app);
    if(offer){
      wl_data_offer_add_listener(offer,&offerListener,app);
      app->dragoffer=offer;
      }
    }
  app->pendingoffer=nullptr;

  // The proxy is the dragged data now, not the clipboard
  if(app->doffer==offer){
    app->doffer=nullptr;
    clearOffer(app,false);
    }
  }


// A drag came over one of our surfaces
static void deviceEnter(void* data,wl_data_device*,uint32_t serial,wl_surface* surface,wl_fixed_t sx,wl_fixed_t sy,wl_data_offer* offer){
  FXWaylandApp* app=fxwayland(data);
  if(!app || !offer) return;
  takeDragOffer(app,offer);
  app->dragsurface=surface;
  app->dragserial=serial;
  app->dragfinished=false;
  app->dragaction=FXXWL_DRAG_REJECT;
  fxwaylandTrace("drag entered a window at %d,%d\n",wl_fixed_to_int(sx),wl_fixed_to_int(sy));
  postDrag(app,FXXWL_DNDENTER,surface,
           fxwaylandScaleIn(fxwaylandSurfaceOf(app,surface),wl_fixed_to_int(sx)),
           fxwaylandScaleIn(fxwaylandSurfaceOf(app,surface),wl_fixed_to_int(sy)),
           app->dragactions);
  }


// The drag moved inside the same surface
static void deviceMotion(void* data,wl_data_device*,uint32_t time,wl_fixed_t sx,wl_fixed_t sy){
  FXWaylandApp* app=fxwayland(data);
  if(!app || !app->dragoffer) return;
  (void)time;
  postDrag(app,FXXWL_DNDMOTION,app->dragsurface,
           fxwaylandScaleIn(fxwaylandSurfaceOf(app,app->dragsurface),wl_fixed_to_int(sx)),
           fxwaylandScaleIn(fxwaylandSurfaceOf(app,app->dragsurface),wl_fixed_to_int(sy)),
           app->dragactions);
  }


// The drag left without a drop
static void deviceLeave(void* data,wl_data_device*){
  FXWaylandApp* app=fxwayland(data);
  if(!app) return;
  if(app->dragoffer){
    postDrag(app,FXXWL_DNDLEAVE,app->dragsurface,0,0,app->dragaction);
    clearDrag(app);
    }
  }


// The drag was dropped on us
static void deviceDrop(void* data,wl_data_device*){
  FXWaylandApp* app=fxwayland(data);
  if(!app || !app->dragoffer) return;
  fxwaylandTrace("drag dropped, action %u\n",app->dragaction);
  postDrag(app,FXXWL_DNDDROP,app->dragsurface,0,0,app->dragaction);
  }


static const wl_data_device_listener deviceListener={
  deviceDataOffer,
  deviceEnter,
  deviceLeave,
  deviceMotion,
  deviceDrop,
  deviceSelection
  };


/*******************************************************************************/

// Move between the actions of the protocol and those of the core
static uint32_t toWaylandActions(FXuint action){
  switch(action){
    case FXXWL_DRAG_COPY: return WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
    case FXXWL_DRAG_MOVE: return WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
    case FXXWL_DRAG_ASK: return WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;
    case FXXWL_DRAG_ACCEPT: return WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY|
                                   WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE|
                                   WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;
    }
  return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;  // This protocol has no LINK
  }


static FXuint fromWaylandAction(uint32_t action){
  switch(action){
    case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY: return FXXWL_DRAG_COPY;
    case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE: return FXXWL_DRAG_MOVE;
    case WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK: return FXXWL_DRAG_ASK;
    }
  return FXXWL_DRAG_REJECT;
  }


// The actions the source allows, as a set the target can choose from
static uint32_t preferredActions(uint32_t actions){
  if(actions&WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) return WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
  if(actions&WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) return WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
  if(actions&WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK) return WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;
  return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  }


/*******************************************************************************/

// The drag source answers a peer that asks for the data, exactly like the
// clipboard source does, with the data of the window that started the drag
static void dragSourceSend(void* data,wl_data_source*,const char* mime,int32_t fd){
  FXWaylandApp* app=fxwayland(data);
  FXDragType type;
  FXuchar* bytes=nullptr;
  FXuint size=0;
  type=fxwaylandTypeFromMime(app,mime);
  if(type && fxwaylandOfferData(app,type,bytes,size,FXXWL_FROM_DRAGNDROP)){
    fxwaylandTrace("drag send %s, %u bytes\n",mime,size);
    writeAll(fd,bytes,size);
    freeElms(bytes);
    }
  else{
    fxwaylandTrace("drag send %s, we have no such data\n",mime);
    ::close(fd);
    }
  }


static void dragSourceTarget(void*,wl_data_source*,const char*){}

static void dragSourceCancelled(void* data,wl_data_source* source){
  FXWaylandApp* app=fxwayland(data);
  if(app && app->dragsource==source){
    fxwaylandTrace("drag cancelled\n");
    app->dragaction=FXXWL_DRAG_REJECT;
    app->dragfinished=true;
    wl_data_source_destroy(app->dragsource);
    app->dragsource=nullptr;
    }
  }


static void dragSourceAction(void* data,wl_data_source*,uint32_t action){
  FXWaylandApp* app=fxwayland(data);
  if(app) app->dragaction=fromWaylandAction(action);
  }


static void dragSourceDropPerformed(void* data,wl_data_source*){
  FXWaylandApp* app=fxwayland(data);
  if(app) app->dragfinished=true;
  }


static void dragSourceFinished(void* data,wl_data_source*){
  FXWaylandApp* app=fxwayland(data);
  if(app) app->dragfinished=true;
  }


static const wl_data_source_listener dragSourceListener={
  dragSourceTarget,
  dragSourceSend,
  dragSourceCancelled,
  dragSourceDropPerformed,
  dragSourceFinished,
  dragSourceAction
  };


// Start dragging the types of a window; the compositor takes it from here and
// tells us what the target does with them
FXbool fxwaylandStartDrag(FXWaylandApp* app,FXID window,const FXDragType* types,FXuint numtypes){
  FXWaylandSurface* s;
  FXuint i;
  FXuint serial;
  FXString mime;
  if(!app || !app->ddevice || !numtypes) return false;
  if((s=fxwaylandShellSurface(app,window))==nullptr) return false;
  serial=app->pointerserial;
  if(!serial){ fxwaylandTrace("no serial to start a drag with\n"); return false; }
  if(app->dragsource){
    wl_data_source_destroy(app->dragsource);
    app->dragsource=nullptr;
    }
  app->dragsource=wl_data_device_manager_create_data_source(app->ddmanager);
  if(!app->dragsource) return false;
  wl_data_source_add_listener(app->dragsource,&dragSourceListener,app);
  for(i=0; i<numtypes; i++){
    if(!(mime=fxwaylandTypeName(app,types[i])).empty()){
      wl_data_source_offer(app->dragsource,mime.text());
      }
    }
  app->dragactions=toWaylandActions(FXXWL_DRAG_COPY);
  app->dragaction=FXXWL_DRAG_REJECT;
  app->dragfinished=false;
  wl_data_source_set_actions(app->dragsource,toWaylandActions(FXXWL_DRAG_COPY)|toWaylandActions(FXXWL_DRAG_MOVE));
  wl_data_device_start_drag(app->ddevice,app->dragsource,s->surface,nullptr,serial);
  wl_display_flush(app->display);
  fxwaylandTrace("drag started from window %u with %u types\n",(unsigned int)window,numtypes);
  return true;
  }


// The drag is over; hand back the action the target took
FXuint fxwaylandEndDrag(FXWaylandApp* app,FXID window,FXbool drop){
  FXuint action=FXXWL_DRAG_REJECT;
  struct pollfd pfd;
  FXint i;
  (void)window;
  if(!app) return FXXWL_DRAG_REJECT;

  // The drop is the release of the pointer, and the compositor tells the target
  // about it after we get the release, so the source has to stay alive until the
  // drag is finished: destroying it here cancels the drag before the drop lands
  if(drop){
    for(i=0; i<100 && !app->dragfinished; i++){
      wl_display_flush(app->display);
      pfd.fd=wl_display_get_fd(app->display);
      pfd.events=POLLIN;
      pfd.revents=0;
      if(poll(&pfd,1,10)>0){
        wl_display_dispatch(app->display);
        }
      else{
        wl_display_dispatch_pending(app->display);
        }
      }
    action=app->dragaction;
    }
  if(app->dragsource){
    wl_data_source_destroy(app->dragsource);
    app->dragsource=nullptr;
    wl_display_flush(app->display);
    }
  fxwaylandTrace("drag ended with action %u\n",(unsigned int)action);
  app->dragfinished=false;
  app->dragaction=FXXWL_DRAG_REJECT;
  return action;
  }



/*******************************************************************************/

// Ask the compositor for the data devices of the seat; the manager and the seat
// both have to exist, and they may arrive in either order
void fxwaylandOpenDataDevice(FXWaylandApp* app){
  if(!app) return;
  if(app->ddmanager && app->seat && !app->ddevice){
    app->ddevice=wl_data_device_manager_get_data_device(app->ddmanager,app->seat);
    if(app->ddevice){
      wl_data_device_add_listener(app->ddevice,&deviceListener,app);
      fxwaylandTrace("data device on the seat\n");
      }
    }
  if(app->psmanager && !app->psdevice && app->seat){
    app->psdevice=zwp_primary_selection_device_manager_v1_get_device(app->psmanager,app->seat);
    fxwaylandAttachSelectionDevice(app);
    }
  }


void fxwaylandCloseDataDevice(FXWaylandApp* app){
  if(!app) return;
  if(app->dsource){ wl_data_source_destroy(app->dsource); app->dsource=nullptr; }
  if(app->ddevice){ wl_data_device_destroy(app->ddevice); app->ddevice=nullptr; }
  if(app->pssource){ zwp_primary_selection_source_v1_destroy(app->pssource); app->pssource=nullptr; }
  if(app->psdevice){ zwp_primary_selection_device_v1_destroy(app->psdevice); app->psdevice=nullptr; }
  if(app->doffer){ wl_data_offer_destroy(app->doffer); app->doffer=nullptr; }
  if(app->psoffer){ zwp_primary_selection_offer_v1_destroy(app->psoffer); app->psoffer=nullptr; }
  clearOffer(app,false);
  clearOffer(app,true);
  }


// Offer the clipboard from a window
FXbool fxwaylandAcquireClipboard(FXWaylandApp* app,FXID window,const FXDragType* types,FXuint numtypes){
  FXuint i;
  FXString mime;
  FXuint serial;
  if(!app || !app->ddevice || !numtypes) return false;
  serial=app->keyboardserial?app->keyboardserial:app->pointerserial;
  if(!serial){ fxwaylandTrace("no serial to take the clipboard with\n"); return false; }
  if(app->dsource){
    wl_data_source_destroy(app->dsource);
    app->dsource=nullptr;
    }
  app->dsource=wl_data_device_manager_create_data_source(app->ddmanager);
  if(!app->dsource) return false;
  wl_data_source_add_listener(app->dsource,&sourceListener,app);
  for(i=0; i<numtypes; i++){
    if(!(mime=fxwaylandTypeName(app,types[i])).empty()){
      wl_data_source_offer(app->dsource,mime.text());
      }
    }
  wl_data_device_set_selection(app->ddevice,app->dsource,serial);
  wl_display_flush(app->display);
  fxwaylandTrace("clipboard taken by window %u with %u types\n",(unsigned int)window,numtypes);
  return true;
  }


// Give the clipboard up
FXbool fxwaylandReleaseClipboard(FXWaylandApp* app,FXID window){
  FXuint serial;
  (void)window;
  if(!app || !app->ddevice) return false;
  if(app->dsource){
    wl_data_source_destroy(app->dsource);
    app->dsource=nullptr;
    serial=app->keyboardserial?app->keyboardserial:app->pointerserial;
    wl_data_device_set_selection(app->ddevice,nullptr,serial);
    wl_display_flush(app->display);
    }
  return true;
  }


/*******************************************************************************/

// The primary selection works the same way, with its own protocol
static void selectionSourceSend(void* data,zwp_primary_selection_source_v1*,const char* mime,int32_t fd){
  FXWaylandApp* app=fxwayland(data);
  FXDragType type;
  FXuchar* bytes=nullptr;
  FXuint size=0;
  type=fxwaylandTypeFromMime(app,mime);
  if(type && fxwaylandOfferData(app,type,bytes,size,FXXWL_FROM_SELECTION)){
    fxwaylandTrace("selection send %s, %u bytes\n",mime,size);
    writeAll(fd,bytes,size);
    freeElms(bytes);
    }
  else{
    ::close(fd);
    }
  }


static void selectionSourceCancelled(void* data,zwp_primary_selection_source_v1* source){
  FXWaylandApp* app=fxwayland(data);
  if(app && app->pssource==source){
    zwp_primary_selection_source_v1_destroy(app->pssource);
    app->pssource=nullptr;
    fxwaylandTrace("selection lost to a peer\n");
    fxwaylandSelectionLost(app);
    }
  }


static const zwp_primary_selection_source_v1_listener selectionSourceListener={
  selectionSourceSend,
  selectionSourceCancelled
  };


static void selectionOfferMime(void* data,zwp_primary_selection_offer_v1* offer,const char* mime){
  FXWaylandApp* app=fxwayland(data);
  FXDragType type;
  if(!app || offer!=app->psoffer) return;
  type=fxwaylandTypeFromMime(app,mime);
  if(type){
    addOfferType(app,type,FXString(mime),true);
    }
  }


static const zwp_primary_selection_offer_v1_listener selectionOfferListener={
  selectionOfferMime
  };


static void primarySelection(void* data,zwp_primary_selection_device_v1*,zwp_primary_selection_offer_v1* offer){
  FXWaylandApp* app=fxwayland(data);
  if(!app || offer==app->psoffer) return;
  if(app->psoffer){
    zwp_primary_selection_offer_v1_destroy(app->psoffer);
    app->psoffer=nullptr;
    }
  clearOffer(app,true);
  if(offer){
    app->psoffer=offer;
    zwp_primary_selection_offer_v1_add_listener(offer,&selectionOfferListener,app);
    }
  fxwaylandTrace("selection %s\n",offer?"offered by a peer":"empty");
  }


// An offer announced separately from the selection itself
static void primaryDataOffer(void* data,zwp_primary_selection_device_v1* device,zwp_primary_selection_offer_v1* offer){
  primarySelection(data,device,offer);
  }


static const zwp_primary_selection_device_v1_listener selectionDeviceListener={
  primaryDataOffer,
  primarySelection
  };


void fxwaylandAttachSelectionDevice(FXWaylandApp* app){
  if(app && app->psdevice){
    zwp_primary_selection_device_v1_add_listener(app->psdevice,&selectionDeviceListener,app);
    }
  }


FXbool fxwaylandAcquireSelection(FXWaylandApp* app,FXID window,const FXDragType* types,FXuint numtypes){
  FXuint i;
  FXString mime;
  FXuint serial;
  if(!app || !app->psdevice || !numtypes) return false;
  serial=app->keyboardserial?app->keyboardserial:app->pointerserial;
  if(!serial) return false;
  if(app->pssource){
    zwp_primary_selection_source_v1_destroy(app->pssource);
    app->pssource=nullptr;
    }
  app->pssource=zwp_primary_selection_device_manager_v1_create_source(app->psmanager);
  if(!app->pssource) return false;
  zwp_primary_selection_source_v1_add_listener(app->pssource,&selectionSourceListener,app);
  for(i=0; i<numtypes; i++){
    if(!(mime=fxwaylandTypeName(app,types[i])).empty()){
      zwp_primary_selection_source_v1_offer(app->pssource,mime.text());
      }
    }
  zwp_primary_selection_device_v1_set_selection(app->psdevice,app->pssource,serial);
  wl_display_flush(app->display);
  fxwaylandTrace("selection taken by window %u with %u types\n",(unsigned int)window,numtypes);
  return true;
  }


FXbool fxwaylandReleaseSelection(FXWaylandApp* app,FXID window){
  FXuint serial;
  (void)window;
  if(!app || !app->psdevice) return false;
  if(app->pssource){
    zwp_primary_selection_source_v1_destroy(app->pssource);
    app->pssource=nullptr;
    serial=app->keyboardserial?app->keyboardserial:app->pointerserial;
    zwp_primary_selection_device_v1_set_selection(app->psdevice,nullptr,serial);
    wl_display_flush(app->display);
    }
  return true;
  }


/*******************************************************************************/

// Types of the offer; the caller frees the list
void fxwaylandClipboardTypes(FXWaylandApp* app,FXDragType*& types,FXuint& numtypes){
  types=nullptr;
  numtypes=0;
  if(app && app->doffer && app->numoffertypes){
    dupElms(types,app->offertypes,app->numoffertypes);
    numtypes=app->numoffertypes;
    }
  }


void fxwaylandSelectionTypes(FXWaylandApp* app,FXDragType*& types,FXuint& numtypes){
  types=nullptr;
  numtypes=0;
  if(app && app->psoffer && app->numpsoffertypes){
    dupElms(types,app->psoffertypes,app->numpsoffertypes);
    numtypes=app->numpsoffertypes;
    }
  }


// Read a mime type out of the offer of a peer
static FXuchar* readOffer(FXWaylandApp* app,FXDragType type,FXuint& size,FXbool primary){
  int filedes[2];
  FXString mime;
  FXuchar* data=nullptr;
  size=0;
  if(!app) return nullptr;
  if(primary?app->psoffer==nullptr:app->doffer==nullptr) return nullptr;
  if((mime=mimeOfType(app,type,primary)).empty()){
    fxwaylandTrace("type %u is not offered\n",(unsigned int)type);
    return nullptr;
    }
  if(pipe(filedes)<0) return nullptr;
  if(primary){
    zwp_primary_selection_offer_v1_receive(app->psoffer,mime.text(),filedes[1]);
    }
  else{
    wl_data_offer_receive(app->doffer,mime.text(),filedes[1]);
    }
  ::close(filedes[1]);

  // The data comes back on the pipe, so the request has to reach the compositor
  // and the peer has to answer before there is anything to read
  wl_display_flush(app->display);
  wl_display_roundtrip(app->display);
  data=readAll(filedes[0],size);
  fxwaylandTrace("clipboard read %s as type %u, %u bytes\n",mime.text(),(unsigned int)type,size);
  return data;
  }


/*******************************************************************************/

// Accept the drag with a type and an action, so the source can show what will
// happen.  The wayland of this build still names a mime type in accept().
void fxwaylandAcceptDrag(FXWaylandApp* app,FXuint action,FXDragType type){
  FXString mime;
  if(!app || !app->dragoffer) return;
  app->dragaction=action;
  if((mime=mimeOfDragType(app,type)).empty()) return;
  wl_data_offer_set_actions(app->dragoffer,toWaylandActions(FXXWL_DRAG_ACCEPT),toWaylandActions(action));
  wl_data_offer_accept(app->dragoffer,app->dragserial,mime.text());
  wl_display_flush(app->display);
  }


// The drop was handled: tell the source, and forget the offer
void fxwaylandFinishDrop(FXWaylandApp* app,FXuint action){
  if(!app || !app->dragoffer) return;
  app->dragaction=action;
  if(action!=FXXWL_DRAG_REJECT){
    wl_data_offer_finish(app->dragoffer);
    wl_display_flush(app->display);
    }
  clearDrag(app);
  }


// Types of the drag that is over us; the caller frees the list
void fxwaylandDragTypes(FXWaylandApp* app,FXDragType*& types,FXuint& numtypes){
  types=nullptr;
  numtypes=0;
  if(app && app->dragoffer && app->numdragtypes){
    dupElms(types,app->dragtypes,app->numdragtypes);
    numtypes=app->numdragtypes;
    }
  }


// Mime type to ask the dragged data for
static FXString mimeOfDragType(FXWaylandApp* app,FXDragType type){
  FXuint i;
  for(i=0; i<app->numdragtypes; i++){
    if(app->dragtypes[i]==type) return app->dragmimes[i];
    }
  return FXString::null;
  }


// Read a type out of the data that is being dragged
void fxwaylandDragData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size){
  int filedes[2];
  FXString mime;
  data=nullptr;
  size=0;
  if(!app || !app->dragoffer) return;
  if((mime=mimeOfDragType(app,type)).empty()) return;
  if(pipe(filedes)<0) return;
  wl_data_offer_receive(app->dragoffer,mime.text(),filedes[1]);
  ::close(filedes[1]);
  wl_display_flush(app->display);
  wl_display_roundtrip(app->display);
  data=readAll(filedes[0],size);
  fxwaylandTrace("drag read %s as type %u, %u bytes\n",mime.text(),(unsigned int)type,size);
  }


void fxwaylandClipboardData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size){
  data=readOffer(app,type,size,false);
  }


void fxwaylandSelectionData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size){
  data=readOffer(app,type,size,true);
  }

}

#endif
