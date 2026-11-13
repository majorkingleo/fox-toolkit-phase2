/********************************************************************************
*                                                                               *
*                          I c o n   ( W a y l a n d )                          *
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

  - An icon keeps its pixels in the same client buffer as an image; the shape
    and the etch mask of the X11 module are not needed, because the rasterizer
    derives the shape from the alpha channel (or the transparent color) when it
    blits the icon and when it draws the shaded and sunken variants.
  - So the mask members stay 0 and there is no server-side work at all.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create icon
void FXIcon::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
      FXImage::create();
      }
    }
  }


// Destroy icon
void FXIcon::destroy(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
    FXImage::destroy();
    shape=0;
    etch=0;
    }
  }


// Render icon; the pixels are already where they belong
void FXIcon::render(){
  }


// Resize icon
void FXIcon::resize(FXint w,FXint h){
  FXImage::resize(w,h);
  }

}

#endif
