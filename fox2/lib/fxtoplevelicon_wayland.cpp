/********************************************************************************
*                                                                               *
*              T o p l e v e l   I c o n   ( W a y l a n d )                    *
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
#include "FXImage.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - A toplevel icon is a shared memory buffer that the compositor copies; the
    protocol wants it premultiplied, and the data of an FXImage is not, so the
    copy does the multiplication.  A fully transparent pixel stays zero, which
    is what the compositor expects.
  - The buffer has to outlive the request: the compositor may keep reading it
    until it has copied it, so it is dropped with the surface and not with the
    icon object.
  - The icon object itself is only a vehicle for set_icon() and can go right
    after; what the compositor keeps is the copy of the pixels.
  - A compositor that does not offer the protocol gets nothing, and the window
    still shows whatever the desktop puts there, exactly as before.
*/

namespace FX {

// Copy the image into a shared memory buffer the compositor can read; the
// protocol requires a square buffer, so a non-square icon is centered on a
// transparent square of its longer edge
static FXWaylandBuffer* makeIconBuffer(FXWaylandApp* app,FXImage* image){
  FXWaylandBuffer* buf;
  const FXColor* src=image->getData();
  FXColor* dst;
  FXColor color;
  FXuint a,r,g,b;
  FXint size,ox,oy,x,y;
  if(!src) return nullptr;
  size=FXMAX(image->getWidth(),image->getHeight());
  ox=(size-image->getWidth())/2;
  oy=(size-image->getHeight())/2;
  if((buf=fxwaylandCreateImageBuffer(app,size,size))==nullptr) return nullptr;
  dst=buf->pixels;
  for(y=0; y<image->getHeight(); y++){
    for(x=0; x<image->getWidth(); x++){
      color=src[y*image->getWidth()+x];
      a=FXALPHAVAL(color);
      if(a==255){
        dst[(oy+y)*size+ox+x]=color;
        }
      else if(a==0){
        dst[(oy+y)*size+ox+x]=0;
        }
      else{
        r=(FXREDVAL(color)*a+127)/255;
        g=(FXGREENVAL(color)*a+127)/255;
        b=(FXBLUEVAL(color)*a+127)/255;
        dst[(oy+y)*size+ox+x]=FXRGB(r,g,b)|(a<<24);
        }
      }
    }
  return buf;
  }


// Give a toplevel the icon of the application
void fxwaylandSetWindowIcon(FXWaylandApp* app,FXID window,FXImage* image){
  FXWaylandSurface* surf;
  xdg_toplevel_icon_v1* ticon;
  FXWaylandBuffer* buf;
  if(!app || !image) return;
  if((surf=fxwaylandFindSurface(app,window))==nullptr || !surf->toplevel) return;

  // Nothing to say without the protocol, or without pixels to say it with
  if(!app->iconmanager){
    fxwaylandTrace("no toplevel icon protocol, the window keeps the default icon\n");
    return;
    }
  if((buf=makeIconBuffer(app,image))==nullptr){
    fxwaylandTrace("no buffer for the icon of window %u\n",(unsigned int)window);
    return;
    }
  if((ticon=xdg_toplevel_icon_manager_v1_create_icon(app->iconmanager))==nullptr){
    fxwaylandDestroyImageBuffer(buf);
    return;
    }
  xdg_toplevel_icon_v1_set_name(ticon,"fox");
  xdg_toplevel_icon_v1_add_buffer(ticon,buf->buffer,1);
  xdg_toplevel_icon_manager_v1_set_icon(app->iconmanager,surf->toplevel,ticon);
  xdg_toplevel_icon_v1_destroy(ticon);
  wl_display_flush(app->display);

  // The compositor reads the buffer until it has copied it; the previous one of
  // this window is of no use any more
  if(surf->iconbuffer) fxwaylandDestroyImageBuffer(surf->iconbuffer);
  surf->iconbuffer=buf;
  fxwaylandTrace("icon %dx%d set on window %u\n",buf->width,buf->height,(unsigned int)window);
  }


// Drop the icon buffer of a surface
void fxwaylandFreeWindowIcon(FXWaylandSurface* surf){
  if(!surf) return;
  if(surf->iconbuffer){
    fxwaylandDestroyImageBuffer(surf->iconbuffer);
    surf->iconbuffer=nullptr;
    }
  }

}

#endif
