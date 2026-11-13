/********************************************************************************
*                                                                               *
*                            Root window   ( X 1 1 )                            *
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

#if !defined(WIN32)

/*
  Notes:

  - X11 root window: the screen root window and the display size.
*/

#define TOPIC_CREATION  1001
#define DISPLAY(app) ((Display*)((app)->display))

using namespace FX;

/*******************************************************************************/

namespace FX {

// When created, create subwindows ONLY
void FXRootWindow::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Got to have a visual
      if(!visual){ fxerror("%s::create: trying to create window without a visual.\n",getClassName()); }

      // Initialize visual
      visual->create();


      // Get handle of desktop window
      xid=RootWindow(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));

      // Obtain desktop window size
      width=DisplayWidth(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
      height=DisplayHeight(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));

      // Store for xid to C++ object mapping
      getApp()->hash.insert((void*)xid,this);


      // Normally create children
      for(FXWindow *c=getFirst(); c; c=c->getNext()) c->create();
      }
    }
  }


// Get default width
FXint FXRootWindow::getDefaultWidth(){
  return DisplayWidth(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
  }

// Get default height
FXint FXRootWindow::getDefaultHeight(){
  return DisplayHeight(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
  }

}

#endif
