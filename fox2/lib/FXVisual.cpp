/********************************************************************************
*                                                                               *
*                            V i s u a l   C l a s s                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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

/*
  Notes:

  - Purpose is to provide the illusion of having True Color on all systems.
  - The visual is what determines how an RGB tuple maps to a color on the
    device; thus, the visual also knows the colormap.
  - When a drawable is created, it is created with a certain visual.
  - When painting, graphic contexts are specific for a certain visual; hence,
    the visual should probably keep track of the gc's.
  - FIRST set up pseudo color ramp, then read it back and set up the tables.
    This way, table setup can be reused for read-only colormaps [StaticColor]
    also...
  - We try to match the deepest visual not deeper than the specified depth.
  - For some info on visual setup, see:

      http://www.wolfram.com/~cwikla/articles/txa/visual.html
      http://www.wolfram.com/~cwikla/articles/txa/xcc.1.html
      http://www.wolfram.com/~cwikla/articles/txa/xcc.2.html

  - Freshly constructed FXVisual sets maxcolors to 1000000 in anticipation
    of private colormap. [FXApp however sets the default FXVisual's maximum
    to a lower value as the colormap is shared between lots of apps].
  - Always check for Standard Colormap first [even for default colormap], as
    that (a) makes initialization simpler, and (b) may give us the preferred
    colors to grab on that system [*** Too bad this does not work! ***].
  - Find closest depth to the given depth hint.
  - RGB Ordering:

       RGB 111      > | R  G  B
       BGR 000      --+--------
       RBG 110      R | x  4  2
       GBR 001      G |    x  1
       BRG 100      B |       x
       GRB 011

  - Just because I always forget:

      StaticGray   0
      GrayScale    1
      StaticColor  2
      PseudoColor  3
      TrueColor    4
      DirectColor  5

  - SGI Infinite Reality may have up to 12 bits for red, green, blue each!

  - With the "depth" we mean number of significant bits per pixel, i.e. padding
    is not included.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXVisual,FXId,nullptr,0)


// Detach visual
void FXVisual::detach(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);
    colormap=0;
    freemap=false;
    xid=0;
    }
  }


// Set maximum number of colors to allocate
void FXVisual::setMaxColors(FXuint maxcols){
  if(xid){ fxerror("%s::setMaxColors: visual already initialized.\n",getClassName()); }
  if(maxcols<2) maxcols=2;
  maxcolors=maxcols;
  }


// Save to stream
void FXVisual::save(FXStream& store) const {
  FXId::save(store);
  store << flags;
  store << hint;
  store << maxcolors;
  }


// Load from stream
void FXVisual::load(FXStream& store){
  FXId::load(store);
  store >> flags;
  store >> hint;
  store >> maxcolors;
  }


// Destroy
FXVisual::~FXVisual(){
  FXTRACE(TOPIC_CONSTRUCT,"FXVisual::~FXVisual %p\n",this);
  destroy();
  }

}
