/********************************************************************************
*                                                                               *
*                           Bitmap   ( W a y l a n d )                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxendian.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXException.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXMetaClass.h"
#include "FXString.h"
#include "FXStream.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXBitmap.h"

#if defined(FOX_WAYLAND)

/*
  Notes:
  - A Wayland bitmap has no server-side copy: the client pixel buffer IS the
    bitmap; create() only sets up the visual, render() and restore() have
    nothing to move around.
*/

#define TOPIC_CREATION  1001

// Changable bitmap options
#define BITMAP_MASK   (BITMAP_KEEP|BITMAP_SHMI|BITMAP_SHMP)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create bitmap; the client buffer is the bitmap
void FXBitmap::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
      visual->create();

      // Make sure there is a buffer to draw into
      if(!data){
        if(!callocElms(data,FXMAX(height,1)*bytewidth)){ throw FXMemoryException("unable to create bitmap"); }
        options|=BITMAP_OWNED;
        }
      xid=1;
      }
    }
  }


// Destroy bitmap; the pixel buffer is not a server-side resource
void FXBitmap::destroy(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
    xid=0;
    }
  }


// Restore client-side pixel buffer from the bitmap; nothing to do
void FXBitmap::restore(){
  }


// Render server-side representation from client-side pixels; nothing to do
void FXBitmap::render(){
  }


// Resize bitmap to the specified width and height; the contents become undefined
void FXBitmap::resize(FXint w,FXint h){
  FXint bw;
  if(w<1) w=1;
  if(h<1) h=1;
  FXTRACE(TOPIC_CREATION,"%s::resize(%d,%d)\n",getClassName(),w,h);
  bw=(w+7)>>3;

  // Resize data array; only do the work if the new
  // array is a different size as measured in bytes!
  if(data){
    if(!(options&BITMAP_OWNED)){        // Need to own array
      if(!allocElms(data,h*bw)){ throw FXMemoryException("unable to resize bitmap"); }
      options|=BITMAP_OWNED;
      }
    else if(h*bw!=height*bytewidth){
      if(!resizeElms(data,h*bw)){ throw FXMemoryException("unable to resize bitmap"); }
      }
    }

  // Remember new size
  bytewidth=bw;
  width=w;
  height=h;
  }



// Wayland has no device context handles
FXID FXBitmap::GetDC() const {
  return 0;
  }


// Nothing to release
int FXBitmap::ReleaseDC(FXID) const {
  return 0;
  }

}

#endif
