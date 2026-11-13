/********************************************************************************
*                                                                               *
*                            V i s u a l   ( W a y l a n d )                    *
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
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXImage.h"

#if defined(FOX_WAYLAND)

#define TOPIC_CREATION  1001

/*
  Notes:

  - Wayland has no visuals and no colormaps: buffers are always 32 bit
    premultiplied ARGB, so the visual is a fixed truecolor description.
  - getPixel() and getColor() are the identity on the color, except that the
    alpha channel of a pixel is 255; FXImage keeps its pixels as FXColor and
    only the alpha channel tells the compositor what to blend.
  - There is no server-side handle, so the created state is a flag of its own;
    xid is 1 when the visual has been set up, exactly like the X11 module uses
    its pixmap id as the flag.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct visual
FXVisual::FXVisual():visual(nullptr),colormap(0),maxcolors(1000000),numcolors(0),numred(0),numgreen(0),numblue(0),depth(0),flags(VISUAL_DEFAULT),hint(32),type(Unknown),freemap(false){
  scrollgc=nullptr;
  gc=nullptr;
  }


// Construct visual with defaults
FXVisual::FXVisual(FXApp* a,FXuint flgs,FXuint hnt):FXId(a),visual(nullptr),colormap(0),maxcolors(1000000),numcolors(0),numred(0),numgreen(0),numblue(0),depth(0),flags(flgs),hint(hnt),type(Unknown),freemap(false){
  scrollgc=nullptr;
  gc=nullptr;
  }


// Create visual: one fixed 32 bit truecolor format for all surfaces
void FXVisual::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
      visual=nullptr;
      colormap=0;
      freemap=false;
      depth=32;
      type=Color;
      numcolors=(1<<24);
      numred=256;
      numgreen=256;
      numblue=256;
      xid=1;
      }
    }
  }


// Destroy visual; there is nothing on the server side
void FXVisual::destroy(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
    colormap=0;
    freemap=false;
    xid=0;
    }
  }


// Device pixel value for a color; a Wayland pixel is the color with opaque alpha
FXPixel FXVisual::getPixel(FXColor clr){
  return clr|0xff000000u;
  }


// Color value for a device pixel
FXColor FXVisual::getColor(FXPixel pix){
  return pix&0x00ffffffu;
  }

}

#endif
