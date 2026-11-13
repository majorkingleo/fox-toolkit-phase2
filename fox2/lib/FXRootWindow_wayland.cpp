/********************************************************************************
*                                                                               *
*                     R o o t   W i n d o w   ( W a y l a n d )                 *
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
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - The root window is the screen area.  There is no surface for it and nothing
    to paint it with: it keeps id zero, so that coordinates translate to screen
    space through it, and creating it only creates its children.
  - Its default size is the size of the first output; the compositor reports it
    with the geometry and mode events, and a well behaved output reports both
    before the roundtrip of openDisplay() ends.
*/

#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// When created, create subwindows ONLY
void FXRootWindow::create(){
  if(getApp()->isInitialized()){
    FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

    // Got to have a visual
    if(!visual){ fxerror("%s::create: trying to create window without a visual.\n",getClassName()); }

    // Initialize visual
    visual->create();

    // The screen itself has no surface; creating it twice creates nothing new
    // because every child checks for its own id
    for(FXWindow *c=getFirst(); c; c=c->getNext()) c->create();
    }
  }


// Get default width
FXint FXRootWindow::getDefaultWidth(){
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  if(app && app->screenwidth>0) return app->screenwidth;
  return 1280;
  }


// Get default height
FXint FXRootWindow::getDefaultHeight(){
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  if(app && app->screenheight>0) return app->screenheight;
  return 1024;
  }


// Wayland has no device context handles
FXID FXRootWindow::GetDC() const {
  return 0;
  }


// Nothing to release
int FXRootWindow::ReleaseDC(FXID) const {
  return 0;
  }

}

#endif
