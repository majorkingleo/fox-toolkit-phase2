/********************************************************************************
*                                                                               *
*                        C u r s o r   ( W a y l a n d )                        *
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
#include "fxkeys.h"
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXVisual.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXCursor.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - A Wayland pointer cursor is a surface of its own, and a stock cursor is a
    shape name that the compositor or cursor-shape-v1 resolves, so the cursor
    object keeps only its pixels and its hot spot here.
  - Creating and destroying are therefore no-ops for now; phase 6 realizes the
    stock shapes through cursor-shape-v1 or the wl_cursor theme, and the image
    cursors through a small wl_shm surface.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create cursor
void FXCursor::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      xid=1;
      }
    }
  }


// Destroy cursor
void FXCursor::destroy(){
  if(xid){
    xid=0;
    }
  }

}

#endif
