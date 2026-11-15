/********************************************************************************
*                                                                               *
*                          C u r s o r   ( W a y l a n d )                      *
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
#include "fxkeys.h"
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
#include "FXVisual.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXCursor.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

#define TOPIC_CREATION  1001

// The stock cursor numbers live in the low byte of the options, as on the
// other platforms; they are not a public flag because a cursor is either stock
// or image based
#define CURSOR_STOCK  255

/*
  Notes:

  - The compositor owns the pointer: the client only says where it is and which
    cursor it wants while it is there.  A stock cursor is a shape name that
    cursor-shape-v1 resolves to the cursor theme of the desktop; an image
    cursor is a surface of its own that the pointer displays while it is over
    the window.
  - The cursor is set for the whole surface, so the widget under the pointer
    decides which one it is: the core reports the widget on every enter and
    motion, and fxwaylandSetCursor() compares it with the last one.
  - A shape request needs the serial of the pointer enter, because the
    compositor must know which surface the shape belongs to.
  - A compositor without cursor-shape-v1 keeps its own arrow for stock cursors;
    image cursors still work, because they do not need the protocol.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// The surface and the buffer of an image cursor
struct FXWaylandCursor {
  wl_surface*      surface;
  FXWaylandBuffer* buffer;
  };


// The cursor-shape name of a stock cursor
static FXuint stockShape(FXuint stock){
  switch(stock){
    case CURSOR_ARROW:     return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
    case CURSOR_RARROW:    return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
    case CURSOR_IBEAM:     return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT;
    case CURSOR_WATCH:     return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT;
    case CURSOR_CROSS:     return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR;
    case CURSOR_UPDOWN:    return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE;
    case CURSOR_LEFTRIGHT: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE;
    case CURSOR_MOVE:      return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE;
    }
  return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
  }


// Copy the pixels of an image cursor into a shared memory buffer; the
// compositor wants premultiplied alpha
static FXWaylandBuffer* makeCursorBuffer(FXWaylandApp* app,const FXColor* src,FXint w,FXint h){
  FXWaylandBuffer* buf;
  FXColor* dst;
  FXColor color;
  FXuint a,r,g,b;
  FXint x,y;
  if(!src) return nullptr;
  if((buf=fxwaylandCreateImageBuffer(app,w,h))==nullptr) return nullptr;
  dst=buf->pixels;
  for(y=0; y<h; y++){
    for(x=0; x<w; x++){
      color=src[y*w+x];
      a=FXALPHAVAL(color);
      if(a==255){
        dst[y*w+x]=color;
        }
      else if(a==0){
        dst[y*w+x]=0;
        }
      else{
        r=(FXREDVAL(color)*a+127)/255;
        g=(FXGREENVAL(color)*a+127)/255;
        b=(FXBLUEVAL(color)*a+127)/255;
        dst[y*w+x]=FXRGB(r,g,b)|(a<<24);
        }
      }
    }
  return buf;
  }


/*******************************************************************************/

// Create cursor
void FXCursor::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXWaylandApp* wayland=fxwayland(getApp()->getDisplay());
      FXWaylandCursor* cur;
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // A stock cursor is just a shape name; nothing to create
      if(options&CURSOR_STOCK){
        xid=(FXID)1;
        return;
        }

      // An image cursor is a surface with its pixels; should have data and fit
      // the limits the other platforms have
      if(!data){ fxerror("%s::create: cursor needs pixel data.\n",getClassName()); }
      if(width>32 || height>32){ fxerror("%s::create: cursor exceeds maximum size of 32x32 pixels\n",getClassName()); }
      if(!wayland || !wayland->compositor || !wayland->shm){
        fxwarning("%s::create: no compositor to make the cursor surface.\n",getClassName());
        return;
        }
      cur=new FXWaylandCursor;
      cur->surface=wl_compositor_create_surface(wayland->compositor);
      cur->buffer=makeCursorBuffer(wayland,data,width,height);
      if(!cur->surface || !cur->buffer){
        if(cur->buffer) fxwaylandDestroyImageBuffer(cur->buffer);
        if(cur->surface) wl_surface_destroy(cur->surface);
        delete cur;
        return;
        }
      wl_surface_attach(cur->surface,cur->buffer->buffer,0,0);
      wl_surface_damage_buffer(cur->surface,0,0,width,height);
      wl_surface_commit(cur->surface);
      wl_display_flush(wayland->display);
      xid=(FXID)cur;
      }
    }
  }


// Destroy cursor; detach() only drops the handle, which is enough for the
// stock cursors and for a connection that is going away
void FXCursor::destroy(){
  if(xid){
    if(!(options&CURSOR_STOCK)){
      FXWaylandCursor* cur=(FXWaylandCursor*)xid;
      if(cur->buffer) fxwaylandDestroyImageBuffer(cur->buffer);
      if(cur->surface) wl_surface_destroy(cur->surface);
      delete cur;
      }
    xid=0;
    }
  }


/*******************************************************************************/

// Ask for the cursor of the widget under the pointer; the compositor shows one
// cursor for the whole surface, so this is called when that widget changes
void fxwaylandSetCursor(FXWaylandApp* app,FXWindow* window,FXWindow* grabwindow){
  FXWindow* source;
  FXCursor* cur;
  FXWaylandCursor* wcur;
  if(!app || !app->pointer || !app->pointerenterserial) return;

  // A grab takes the cursor from the window holding it, even while the pointer
  // wanders over other widgets
  source=grabwindow?grabwindow:window;
  if(source==(FXWindow*)app->cursorwindow) return;
  app->cursorwindow=source;
  if(!source) return;

  // A window that hid the cursor shows nothing
  if(!source->cursorShown()){
    wl_pointer_set_cursor(app->pointer,app->pointerenterserial,nullptr,0,0);
    return;
    }

  // The drag cursor while grabbing, the default cursor otherwise
  cur=grabwindow?source->getDragCursor():source->getDefaultCursor();
  if(!cur) return;

  // Stock cursors are resolved by the compositor from its own theme
  if(cur->getOptions()&CURSOR_STOCK){
    if(app->cursordevice){
      wp_cursor_shape_device_v1_set_shape(app->cursordevice,app->pointerenterserial,stockShape(cur->getOptions()&CURSOR_STOCK));
      }
    return;
    }

  // Image cursors are their own little surface
  if((wcur=(FXWaylandCursor*)cur->id())!=nullptr && wcur->surface){
    wl_pointer_set_cursor(app->pointer,app->pointerenterserial,wcur->surface,cur->getHotX(),cur->getHotY());
    }
  }

}

#endif
