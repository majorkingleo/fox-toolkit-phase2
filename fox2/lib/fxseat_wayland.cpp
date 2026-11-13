/********************************************************************************
*                                                                               *
*                           S e a t   ( W a y l a n d )                         *
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
#include "FXEvent.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#include <xkbcommon/xkbcommon.h>

#if defined(FOX_WAYLAND)

/*
  Notes:

  - The seat hands out a pointer and a keyboard when the hardware appears, and
    takes them away again when it goes; the capabilities event says which.
  - Input arrives in the coordinates of the surface that has the pointer, which
    is the surface of a shell.  The window that gets the event is the deepest
    visible window of that shell at that point; the core never sees a surface,
    so the events carry the shell they came from, and the application module
    turns the place inside the shell into the place inside the window.
  - There are no grabs: the pointer stays with the surface that received the
    press, which is exactly what a menu needs, so the button state is tracked
    here and the mouse grab of the core decides who gets the events.
  - The wheel arrives as a continuous axis; FOX expects one notch of 120, which
    is what a Wayland axis report of 10.0 means.
  - The keyboard needs xkbcommon to turn key codes into keysyms, and its keymap
    arrives as a file descriptor.  A compositor without xkbcommon support, or a
    keymap that does not compile, leaves the keyboard unusable but the rest of
    the application working.
*/

#define TOPIC_EVENT     1003

using namespace FX;

/*******************************************************************************/

namespace FX {

// Find the surface that wraps a wl_surface
FXWaylandSurface* fxwaylandSurfaceOf(FXWaylandApp* app,wl_surface* surface){
  FXWaylandSurface* s;
  if(app && surface){
    for(s=app->surfaces; s; s=s->next){
      if(s->surface==surface) return s;
      }
    }
  return nullptr;
  }


// The window that is under a point of a shell, and the shell itself when the
// point is not over any of its children; the client side title bar belongs to
// the shell, so nothing below it is under a point inside the bar
static FXID hitWindow(FXWaylandApp* app,FXWaylandSurface* s,FXint x,FXint y){
  FXID id;
  if(!app || !s) return 0;
  if(y<s->barh) return s->window;
  if((id=app->childmap.hitTest(x,y-s->barh))==0) id=s->window;
  return id;
  }


// Post an event that carries the shell it came from and the place inside it
static void postPointer(FXWaylandApp* app,FXWaylandSurface* s,unsigned int code,FXint x,FXint y,FXuint button,FXuint state,FXuint time,FXuint pressed){
  FXWaylandEvent ev;
  if(!app || !s) return;
  clearElms(&ev,1);
  ev.type=0;
  ev.id=hitWindow(app,s,x,y);
  ev.x=x;
  ev.y=y;
  ev.button=button;
  ev.state=state|app->modstate;
  ev.time=time;
  ev.code=code;
  ev.data[0]=(int)s->window;
  ev.data[1]=(int)pressed;
  fxwaylandPost(app,ev);
  }


/*******************************************************************************/

// The pointer entered the surface of a shell
static void pointerEnter(void* data,wl_pointer*,uint32_t serial,wl_surface* surface,wl_fixed_t sx,wl_fixed_t sy){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandSurface* s=fxwaylandSurfaceOf(app,surface);
  if(!s) return;
  app->pointerserial=serial;
  app->pointersurface=s;
  app->pointerx=wl_fixed_to_int(sx);
  app->pointery=wl_fixed_to_int(sy);
  fxwaylandTrace("pointer enter window %u at %d,%d\n",(unsigned int)s->window,app->pointerx,app->pointery);
  postPointer(app,s,FXXWL_ENTER,app->pointerx,app->pointery,0,app->pointerbuttons,0,0);
  }


// The pointer left the surface of a shell
static void pointerLeave(void* data,wl_pointer*,uint32_t serial,wl_surface* surface){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandSurface* s=fxwaylandSurfaceOf(app,surface);
  (void)serial;
  if(!s) return;
  fxwaylandTrace("pointer leave window %u\n",(unsigned int)s->window);
  postPointer(app,s,FXXWL_LEAVE,app->pointerx,app->pointery,0,app->pointerbuttons,0,0);
  }


// Whether a surface is still alive
static FXbool surfaceAlive(FXWaylandApp* app,FXWaylandSurface* s){
  FXWaylandSurface* p;
  for(p=app?app->surfaces:nullptr; p; p=p->next){
    if(p==s) return true;
    }
  return false;
  }


// The pointer moved
static void pointerMotion(void* data,wl_pointer*,uint32_t time,wl_fixed_t sx,wl_fixed_t sy){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandSurface* s=app?app->pointersurface:nullptr;
  if(!app) return;
  app->pointerx=wl_fixed_to_int(sx);
  app->pointery=wl_fixed_to_int(sy);
  postPointer(app,s,FXXWL_MOTION,app->pointerx,app->pointery,0,app->pointerbuttons,time,0);
  }


// A pointer button changed; the protocol counts 1,2,3 like X11 does
static void pointerButton(void* data,wl_pointer*,uint32_t serial,uint32_t time,uint32_t button,uint32_t state){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandSurface* s=app?app->pointersurface:nullptr;
  FXuint mask=0;
  (void)serial;
  if(!app || !s) return;
  if(button==1) mask=LEFTBUTTONMASK;
  else if(button==2) mask=MIDDLEBUTTONMASK;
  else if(button==3) mask=RIGHTBUTTONMASK;
  if(state==WL_POINTER_BUTTON_STATE_PRESSED) app->pointerbuttons|=mask;
  else app->pointerbuttons&=~mask;
  fxwaylandTrace("pointer button %u %s\n",button,state==WL_POINTER_BUTTON_STATE_PRESSED?"down":"up");
  postPointer(app,s,FXXWL_BUTTON,app->pointerx,app->pointery,button,app->pointerbuttons,time,state==WL_POINTER_BUTTON_STATE_PRESSED);
  }


// The wheel or a touchpad scroll
static void pointerAxis(void* data,wl_pointer*,uint32_t time,uint32_t axis,wl_fixed_t value){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandSurface* s=app?app->pointersurface:nullptr;
  FXint amount;
  if(!app || !s) return;
  if(axis!=WL_POINTER_AXIS_VERTICAL_SCROLL) return;
  amount=(FXint)(wl_fixed_to_double(value)*12.0);     // One notch of 10.0 is 120
  if(amount==0) return;
  fxwaylandTrace("wheel %d\n",amount);
  postPointer(app,s,FXXWL_WHEEL,app->pointerx,app->pointery,0,app->pointerbuttons,time,0);
  app->events[app->eventcount-1].data[1]=amount;
  }


static void pointerFrame(void*,wl_pointer*){}

static void pointerAxisSource(void*,wl_pointer*,uint32_t){}

static void pointerAxisStop(void*,wl_pointer*,uint32_t,uint32_t){}

static void pointerAxisDiscrete(void*,wl_pointer*,uint32_t,int32_t){}


static const wl_pointer_listener pointerListener={
  pointerEnter,
  pointerLeave,
  pointerMotion,
  pointerButton,
  pointerAxis,
  pointerFrame,
  pointerAxisSource,
  pointerAxisStop,
  pointerAxisDiscrete
  };


/*******************************************************************************/

// The keymap arrives as a file which holds the xkb text
static void keyboardKeymap(void* data,wl_keyboard*,uint32_t format,int32_t fd,uint32_t size){
  FXWaylandApp* app=fxwayland(data);
  FXchar* map;
  if(!app || format!=WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1){
    close(fd);
    return;
    }
  map=(FXchar*)mmap(nullptr,size,PROT_READ,MAP_PRIVATE,fd,0);
  close(fd);
  if(map==MAP_FAILED) return;
  if(app->xkbstate){ xkb_state_unref(app->xkbstate); app->xkbstate=nullptr; }
  if(app->xkbkeymap){ xkb_keymap_unref(app->xkbkeymap); app->xkbkeymap=nullptr; }
  if(!app->xkbcontext) app->xkbcontext=xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if(app->xkbcontext){
    app->xkbkeymap=xkb_keymap_new_from_string(app->xkbcontext,map,XKB_KEYMAP_FORMAT_TEXT_V1,XKB_KEYMAP_COMPILE_NO_FLAGS);
    if(app->xkbkeymap) app->xkbstate=xkb_state_new(app->xkbkeymap);
    }
  munmap(map,size);
  fxwaylandTrace("keymap %s\n",app->xkbstate?"ok":"failed");
  }


// The keyboard focus arrived on the surface of a shell
static void keyboardEnter(void* data,wl_keyboard*,uint32_t serial,wl_surface* surface,wl_array* keys){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandSurface* s=fxwaylandSurfaceOf(app,surface);
  FXWaylandEvent ev;
  (void)serial;
  (void)keys;
  if(!app || !s) return;
  app->keyboardsurface=s;
  clearElms(&ev,1);
  ev.type=0;
  ev.id=s->window;
  ev.code=FXXWL_FOCUSIN;
  ev.data[0]=(int)s->window;
  fxwaylandPost(app,ev);
  fxwaylandTrace("keyboard focus in window %u\n",(unsigned int)s->window);
  }


// The keyboard focus left
static void keyboardLeave(void* data,wl_keyboard*,uint32_t serial,wl_surface* surface){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandSurface* s=fxwaylandSurfaceOf(app,surface);
  (void)serial;
  if(!app || !s) return;

  // No SEL_FOCUSOUT here: the focus of the core follows the shell that got the
  // next enter, which is what the application sees; a compositor that reports
  // the leave of a surface twice would otherwise take the focus away from a
  // widget between the press and the release of a key
  app->keyboardsurface=nullptr;
  fxwaylandTrace("keyboard focus left window %u\n",(unsigned int)s->window);
  }


// A key changed; the modifier state is updated before the core sees the event
static void keyboardKey(void* data,wl_keyboard*,uint32_t serial,uint32_t time,uint32_t key,uint32_t state){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandEvent ev;
  FXbool pressed=state==WL_KEYBOARD_KEY_STATE_PRESSED;
  (void)serial;
  if(!app) return;
  if(app->xkbstate){
    xkb_state_update_key(app->xkbstate,key+8,pressed?XKB_KEY_DOWN:XKB_KEY_UP);
    app->modstate=xkb_state_serialize_mods(app->xkbstate,XKB_STATE_MODS_DEPRESSED);
    }
  clearElms(&ev,1);
  ev.type=0;
  ev.id=app->keyboardsurface?app->keyboardsurface->window:0;
  ev.code=FXXWL_KEY;
  ev.button=pressed?1:0;
  ev.data[0]=(int)key;
  ev.state=app->modstate;
  ev.time=time;
  fxwaylandPost(app,ev);
  fxwaylandTrace("key %u %s mods %x\n",key,pressed?"down":"up",app->modstate);
  }


static void keyboardModifiers(void* data,wl_keyboard*,uint32_t serial,uint32_t depressed,uint32_t latched,uint32_t locked,uint32_t group){
  FXWaylandApp* app=fxwayland(data);
  (void)serial;
  if(!app || !app->xkbstate) return;
  xkb_state_update_mask(app->xkbstate,depressed,latched,locked,0,0,group);
  app->modstate=xkb_state_serialize_mods(app->xkbstate,XKB_STATE_MODS_DEPRESSED);
  }


static void keyboardRepeatInfo(void*,wl_keyboard*,int32_t,int32_t){}


static const wl_keyboard_listener keyboardListener={
  keyboardKeymap,
  keyboardEnter,
  keyboardLeave,
  keyboardKey,
  keyboardModifiers,
  keyboardRepeatInfo
  };


/*******************************************************************************/

// The keysym of a key code, with the modifiers of the keyboard applied
FXuint fxwaylandKeySym(FXWaylandApp* app,FXuint key){
  if(!app || !app->xkbstate) return 0;
  return xkb_state_key_get_one_sym(app->xkbstate,key+8);
  }


// The text a key produces, empty when it produces none
FXString fxwaylandKeyText(FXWaylandApp* app,FXuint key){
  FXchar buffer[32];
  int len;
  if(!app || !app->xkbstate) return FXString::null;
  len=xkb_state_key_get_utf8(app->xkbstate,key+8,buffer,sizeof(buffer));
  if(len<=0) return FXString::null;
  return FXString(buffer,len);
  }


/*******************************************************************************/

// The seat tells us which devices it has
static void seatCapabilities(void* data,wl_seat* seat,uint32_t capabilities){
  FXWaylandApp* app=fxwayland(data);
  FXTRACE(TOPIC_EVENT,"seat capabilities %u\n",capabilities);
  if(!app) return;
  if(capabilities&WL_SEAT_CAPABILITY_POINTER){
    if(!app->pointer){
      app->pointer=wl_seat_get_pointer(seat);
      if(app->pointer) wl_pointer_add_listener(app->pointer,&pointerListener,app);
      }
    }
  else{
    if(app->pointer){
      wl_pointer_destroy(app->pointer);
      app->pointer=nullptr;
      app->pointersurface=nullptr;
      }
    }
  if(capabilities&WL_SEAT_CAPABILITY_KEYBOARD){
    if(!app->keyboard){
      app->keyboard=wl_seat_get_keyboard(seat);
      if(app->keyboard) wl_keyboard_add_listener(app->keyboard,&keyboardListener,app);
      }
    }
  else{
    if(app->keyboard){
      wl_keyboard_destroy(app->keyboard);
      app->keyboard=nullptr;
      }
    }
  }


// The name of the seat, for the trace
static void seatName(void* data,wl_seat*,const char* name){
  fxwaylandTrace("seat %s\n",name?name:"");
  }


static const wl_seat_listener seatListener={
  seatCapabilities,
  seatName
  };


/*******************************************************************************/

// Add the listeners of the seat
void fxwaylandOpenSeat(FXWaylandApp* app){
  if(app && app->seat){
    wl_seat_add_listener(app->seat,&seatListener,app);
    }
  }


// Drop the devices of the seat
void fxwaylandCloseSeat(FXWaylandApp* app){
  if(!app) return;
  if(app->keyboard){
    wl_keyboard_destroy(app->keyboard);
    app->keyboard=nullptr;
    }
  if(app->pointer){
    wl_pointer_destroy(app->pointer);
    app->pointer=nullptr;
    }
  if(app->xkbstate){ xkb_state_unref(app->xkbstate); app->xkbstate=nullptr; }
  if(app->xkbkeymap){ xkb_keymap_unref(app->xkbkeymap); app->xkbkeymap=nullptr; }
  if(app->xkbcontext){ xkb_context_unref(app->xkbcontext); app->xkbcontext=nullptr; }
  app->pointersurface=nullptr;
  app->keyboardsurface=nullptr;
  app->pointerbuttons=0;
  app->modstate=0;
  }

}

#endif
