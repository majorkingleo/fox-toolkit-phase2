/********************************************************************************
*                                                                               *
*              O u t p u t   S c a l e   ( W a y l a n d )                      *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Wayland has no window DPI: the compositor tells the client a scale, per
    output through wl_output.scale and per surface through
    wp_fractional_scale_v1.  A surface learns which outputs it is on from
    wl_surface.enter and wl_surface.leave, so moving a window to a screen with
    another scale changes the scale of that surface and of nothing else.
  - The device pixels of this toolkit are what the application draws into.  The
    window of a shell is sized in device pixels, so a window of 400 logical
    pixels is a 600 pixel wide window at scale 1.5, and the compositor is told
    through wp_viewport that the buffer covers 400 logical pixels.  That keeps
    the drawing crisp, because the rasterizer really draws 600 pixels.
  - Everything that crosses the protocol is in logical pixels and has to be
    converted: the configure size, the pointer and drag positions, and the
    geometry of a popup.  Nothing else leaves the client.
  - The application follows the scale with its theme: metrics and fonts grow
    with it, which is what the display scale is for.  The theme scale is one
    per application, so of two windows on outputs with different scales the one
    that heard about the change last wins, as it does on Windows.
*/

namespace FX {

/*******************************************************************************/

// Tell the application which scale to draw with
static void applyDisplayScale(FXWaylandSurface* s,FXdouble scale){
  FXApp* foxapp;
  if(!s->app || !(foxapp=(FXApp*)s->app->foxapp)) return;
  foxapp->setMonitorScale(scale);
  }


// The scale of a surface is the largest one of the outputs it is on
static void updateScale(FXWaylandSurface* s){
  FXWaylandOutput* out;
  FXdouble scale=1.0;
  FXuint i;
  if(s->fscale120>0){
    scale=FXdouble(s->fscale120)/120.0;         // The compositor is exact here
    }
  else{
    for(i=0; i<s->numoutputs; i++){
      for(out=s->app->outputs; out; out=out->next){
        if(out->output==s->outputs[i] && FXdouble(out->scale)>scale){
          scale=FXdouble(out->scale);
          }
        }
      }
    }
  if(scale<1.0) scale=1.0;
  if(scale==s->scale) return;
  s->scale=scale;
  fxwaylandTrace("scale %.3f on window %u\n",scale,(unsigned int)s->window);
  applyDisplayScale(s,scale);

  // The window keeps its logical size, so it grows or shrinks in device pixels
  if(s->logicalw>0 && s->logicalh>0){
    fxwaylandPostConfigure(s,s->logicalw,s->logicalh);
    }
  }


/*******************************************************************************/

// The compositor says which surface it wants at which scale, with 1/120 steps
static void fractionalPreferred(void* data,wp_fractional_scale_v1*,uint32_t scale){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  if(!s || (FXint)scale<=0) return;
  s->fscale120=(FXint)scale;
  updateScale(s);
  }


static const wp_fractional_scale_v1_listener fractionalListener={
  fractionalPreferred
  };


// A surface arrived on an output
static void surfaceEnter(void* data,wl_surface*,wl_output* output){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  FXuint i;
  if(!s) return;
  for(i=0; i<s->numoutputs; i++){
    if(s->outputs[i]==output) return;
    }
  if(s->numoutputs<FXMAX_OUTPUTS){
    s->outputs[s->numoutputs++]=output;
    }
  updateScale(s);
  }


// And left it again
static void surfaceLeave(void* data,wl_surface*,wl_output* output){
  FXWaylandSurface* s=(FXWaylandSurface*)data;
  FXuint i;
  if(!s) return;
  for(i=0; i<s->numoutputs; i++){
    if(s->outputs[i]==output){
      s->outputs[i]=s->outputs[--s->numoutputs];
      break;
      }
    }
  updateScale(s);
  }


static const wl_surface_listener surfaceOutputListener={
  surfaceEnter,
  surfaceLeave
  };


/*******************************************************************************/

// The mode of the output is its size in device pixels; the application works in
// logical pixels, so the root window is the size of the output divided by scale
static void setScreenSize(FXWaylandOutput* out){
  FXint w=out->width, h=out->height, scale=out->scale>0?out->scale:1;
  if(w<=0 || h<=0) return;
  if(out->app->output && out->app->output!=out->output) return;
  out->app->screenwidth=(w+scale-1)/scale;
  out->app->screenheight=(h+scale-1)/scale;
  }


static void outputGeometry(void* data,wl_output*,int32_t,int32_t,int32_t width,int32_t height,int32_t,const char*,const char*,int32_t){
  FXWaylandOutput* out=(FXWaylandOutput*)data;
  if(!out || width<=0 || height<=0) return;
  out->width=width;
  out->height=height;
  setScreenSize(out);
  }


static void outputMode(void* data,wl_output*,uint32_t flags,int32_t width,int32_t height,int32_t){
  FXWaylandOutput* out=(FXWaylandOutput*)data;
  if(!out || !(flags&WL_OUTPUT_MODE_CURRENT) || width<=0 || height<=0) return;
  out->width=width;
  out->height=height;
  setScreenSize(out);
  }


static void outputDone(void*,wl_output*){}


// A change of the integer scale affects every surface that is on this output
static void outputScale(void* data,wl_output*,int32_t factor){
  FXWaylandOutput* out=(FXWaylandOutput*)data;
  FXWaylandSurface* s;
  FXuint i;
  if(!out || factor<=0 || out->scale==factor) return;
  out->scale=factor;
  setScreenSize(out);
  for(s=out->app->surfaces; s; s=s->next){
    for(i=0; i<s->numoutputs; i++){
      if(s->outputs[i]==out->output){
        updateScale(s);
        break;
        }
      }
    }
  }


static const wl_output_listener outputListener={
  outputGeometry,
  outputMode,
  outputDone,
  outputScale
  };


/*******************************************************************************/

// Remember an output of the registry, and listen to what it says
void fxwaylandAddOutput(FXWaylandApp* app,wl_output* output){
  FXWaylandOutput* out;
  if(!app || !output) return;
  for(out=app->outputs; out; out=out->next) if(out->output==output) return;
  if(!(out=new FXWaylandOutput())) return;
  out->app=app;
  out->output=output;
  out->scale=1;
  out->width=0;
  out->height=0;
  out->next=app->outputs;
  app->outputs=out;
  wl_output_add_listener(output,&outputListener,out);
  }


void fxwaylandFreeOutputs(FXWaylandApp* app){
  FXWaylandOutput* out;
  FXWaylandOutput* next;
  if(!app) return;
  for(out=app->outputs; out; out=next){
    next=out->next;
    wl_output_destroy(out->output);
    delete out;
    }
  app->outputs=nullptr;
  app->output=nullptr;
  }


/*******************************************************************************/

// Give a new surface its scale plumbing; it has one scale until an output says
// otherwise, so the first frame is drawn at 1:1
void fxwaylandPrepareScale(FXWaylandApp* app,FXWaylandSurface* surf){
  if(!app || !surf) return;
  surf->viewport=nullptr;
  surf->fscale=nullptr;
  surf->fscale120=0;
  surf->numoutputs=0;
  surf->scale=1.0;
  surf->bufscale=1;
  surf->logicalw=0;
  surf->logicalh=0;
  wl_surface_add_listener(surf->surface,&surfaceOutputListener,surf);
  if(app->viewporter){
    surf->viewport=wp_viewporter_get_viewport(app->viewporter,surf->surface);
    }
  if(app->fsmanager){
    surf->fscale=wp_fractional_scale_manager_v1_get_fractional_scale(app->fsmanager,surf->surface);
    if(surf->fscale){
      wp_fractional_scale_v1_add_listener(surf->fscale,&fractionalListener,surf);
      }
    }
  fxwaylandTrace("surface of window %u scaled at %.3f\n",(unsigned int)surf->window,surf->scale);
  }


void fxwaylandFreeScale(FXWaylandSurface* surf){
  if(!surf) return;
  if(surf->fscale){ wp_fractional_scale_v1_destroy(surf->fscale); surf->fscale=nullptr; }
  if(surf->viewport){ wp_viewport_destroy(surf->viewport); surf->viewport=nullptr; }
  }


// The scale of the outputs is the display scale of the application; it has to be
// known before the first window, so that metrics and fonts start out right
void fxwaylandApplyStartupScale(FXWaylandApp* app){
  FXWaylandOutput* out;
  FXApp* foxapp;
  FXdouble scale=1.0;
  if(!app || !(foxapp=(FXApp*)app->foxapp)) return;
  for(out=app->outputs; out; out=out->next){
    if(FXdouble(out->scale)>scale) scale=FXdouble(out->scale);
    }
  fxwaylandTrace("compositor scale %.3f\n",scale);
  if(scale>1.0) foxapp->setMonitorScale(scale);
  }


FXdouble fxwaylandSurfaceScale(FXWaylandSurface* surf){
  return surf?surf->scale:1.0;
  }


// A length or a position of the protocol, in device pixels
FXint fxwaylandScaleIn(FXWaylandSurface* surf,FXint v){
  FXdouble scale=surf?surf->scale:1.0;
  return FXint(v*scale+(v<0?-0.5:0.5));
  }


// A device position back to what the protocol wants
FXint fxwaylandScaleOut(FXWaylandSurface* surf,FXint v){
  FXdouble scale=surf?surf->scale:1.0;
  if(scale<=0.0) return v;
  return FXint(v/scale+(v<0?-0.5:0.5));
  }


// The compositor asked for a logical size; the core works in device pixels
void fxwaylandPostConfigure(FXWaylandSurface* surf,FXint logicalw,FXint logicalh){
  FXWaylandEvent ev;
  if(!surf) return;
  surf->logicalw=logicalw;
  surf->logicalh=logicalh;
  clearElms(&ev,1);
  ev.code=FXXWL_CONFIGURE;
  ev.id=surf->window;
  ev.w=fxwaylandScaleIn(surf,logicalw);
  ev.h=fxwaylandScaleIn(surf,logicalh);
  fxwaylandPost(surf->app,ev);
  }

}

#endif
