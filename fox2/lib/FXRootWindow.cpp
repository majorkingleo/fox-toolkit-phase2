/********************************************************************************
*                                                                               *
*                       R o o t   W i n d o w   O b j e c t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXMetaClass.h"
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
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXRootWindow.h"

/*
  Notes:

  - Size of FXRootWindow is now size of the entire virtual display, which
    is a tiled virtual area of primary and secondary display adapters.
  - Maybe getDefaultWidth() and getDefaultHeight() now returns virtual
    screen size.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXRootWindow,FXComposite,nullptr,0)


// Construct root window
FXRootWindow::FXRootWindow(FXApp* a,FXVisual *vis):FXComposite(a,vis){
  }


// Can not attach the root window
void FXRootWindow::attach(FXID){
  }


// Can not detach the root window
void FXRootWindow::detach(){
  }


// When destroyed, destroy subwindows ONLY
void FXRootWindow::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);

      // Normally destroy children
      for(FXWindow *c=getFirst(); c; c=c->getNext()) c->destroy();

      // Remove from xid to C++ object mapping
      getApp()->hash.remove((void*)xid);
      }
    xid=0;
    }
  }


// Moving root has no effect
void FXRootWindow::move(FXint,FXint){ }


// Move and resize root has no effect
void FXRootWindow::position(FXint,FXint,FXint,FXint){ }


// Resize root window has no effect
void FXRootWindow::resize(FXint,FXint){ }


// Layout of root window
void FXRootWindow::layout(){ }


// Mark as dirty
void FXRootWindow::recalc(){ }


// Root can not be focused on
void FXRootWindow::setFocus(){ }


// Root can not be unfocused
void FXRootWindow::killFocus(){ }


// Does not destroy root window
FXRootWindow::~FXRootWindow(){
  xid=0;
  }

}
