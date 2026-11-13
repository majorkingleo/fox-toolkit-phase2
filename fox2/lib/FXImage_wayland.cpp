/********************************************************************************
*                                                                               *
*                          I m a g e   ( W a y l a n d )                        *
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
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXImage.h"
#include "FXIcon.h"

#if defined(FOX_WAYLAND)

#define TOPIC_CREATION  1001

/*
  Notes:

  - A Wayland image has no server-side copy: the client pixel buffer IS the
    image, and the compositor reads it from shared memory when a window commits
    it.  create() therefore only sets up the visual and makes sure there is a
    buffer; render() and restore() have nothing to move around.
  - The buffer holds FXColor, i.e. 0xAARRGGBB.  Every pixel the rasterizer
    writes is opaque and therefore already premultiplied; an icon that carries
    alpha is premultiplied when it is copied to a window buffer in phase 4.
  - release() (in FXImage.cpp) drops the buffer when it is owned; a Wayland
    image can be released and created again without losing anything else.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create image; the client buffer is the image
void FXImage::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
      visual->create();

      // Make sure there is a buffer to draw into
      if(!data){
        if(!callocElms(data,FXMAX(width,1)*FXMAX(height,1))){ throw FXImageException("unable to create image"); }
        options|=IMAGE_OWNED;
        }
      xid=1;
      }
    }
  }


// Destroy image; the pixel buffer is not a server-side resource
void FXImage::destroy(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
    xid=0;
    }
  }


// Restore client-side pixel buffer from the image; nothing to do
void FXImage::restore(){
  }


// Render server-side representation from client-side pixels; nothing to do
void FXImage::render(){
  }


// Resize both client-side and server-side representation
void FXImage::resize(FXint w,FXint h){
  if(w<1) w=1;
  if(h<1) h=1;
  FXTRACE(TOPIC_CREATION,"%s::resize(%d,%d)\n",getClassName(),w,h);
  if(width!=w || height!=h){
    if(data){
      if(!(options&IMAGE_OWNED)){                 // Need to own array
        if(!allocElms(data,w*h)){ throw FXMemoryException("unable to resize image"); }
        options|=IMAGE_OWNED;
        }
      else if(w*h!=width*height){
        if(!resizeElms(data,w*h)){ throw FXMemoryException("unable to resize image"); }
        }
      }
    width=w;
    height=h;
    }
  }


// Wayland has no device context handles
FXID FXImage::GetDC() const {
  return 0;
  }


// Nothing to release
int FXImage::ReleaseDC(FXID) const {
  return 0;
  }

}

#endif
