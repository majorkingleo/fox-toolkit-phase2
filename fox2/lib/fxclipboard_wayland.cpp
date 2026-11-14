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
  FXDragType type;
  FXuchar* bytes=nullptr;
  FXuint size=0;
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
    wl_data_source_destroy(app->dsource);
    app->dsource=nullptr;
    fxwaylandTrace("clipboard lost to a peer\n");
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

// A mime type of the offer of a peer
static void offerOffer(void* data,wl_data_offer* offer,const char* mime){
  FXWaylandApp* app=fxwayland(data);
  FXDragType type;
  if(!app || offer!=app->doffer) return;
  type=fxwaylandTypeFromMime(app,mime);
  if(type){
    addOfferType(app,type,FXString(mime),false);
    }
  }


static void offerSourceActions(void*,wl_data_offer*,uint32_t){}

static void offerAction(void*,wl_data_offer*,uint32_t){}


static const wl_data_offer_listener offerListener={
  offerOffer,
  offerSourceActions,
  offerAction
  };


// Take over the offer of the clipboard; the compositor announces the same offer
// twice, once as data_offer and once as the selection, and destroying it in
// between loses its mime types and crashes on the second destroy
static void takeOffer(FXWaylandApp* app,wl_data_offer* offer){
  if(offer==app->doffer) return;
  if(app->doffer){
    wl_data_offer_destroy(app->doffer);
    app->doffer=nullptr;
    }
  clearOffer(app,false);
  if(offer){
    app->doffer=offer;
    wl_data_offer_add_listener(offer,&offerListener,app);
    }
  }


// The clipboard changed hands; nullptr means nobody offers anything
static void deviceSelection(void* data,wl_data_device*,wl_data_offer* offer){
  FXWaylandApp* app=fxwayland(data);
  if(!app) return;
  takeOffer(app,offer);
  fxwaylandTrace("clipboard %s\n",offer?"offered by a peer":"empty");
  }


// An offer announced before the selection itself
static void deviceDataOffer(void* data,wl_data_device*,wl_data_offer* offer){
  FXWaylandApp* app=fxwayland(data);
  if(!app) return;
  takeOffer(app,offer);
  fxwaylandTrace("data offer %p\n",(void*)offer);
  }


static void deviceEnter(void*,wl_data_device*,uint32_t,wl_surface*,wl_fixed_t,wl_fixed_t,wl_data_offer*){}
static void deviceLeave(void*,wl_data_device*){}
static void deviceMotion(void*,wl_data_device*,uint32_t,wl_fixed_t,wl_fixed_t){}
static void deviceDrop(void*,wl_data_device*){}


static const wl_data_device_listener deviceListener={
  deviceDataOffer,
  deviceEnter,
  deviceLeave,
  deviceMotion,
  deviceDrop,
  deviceSelection
  };


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


void fxwaylandClipboardData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size){
  data=readOffer(app,type,size,false);
  }


void fxwaylandSelectionData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size){
  data=readOffer(app,type,size,true);
  }

}

#endif
