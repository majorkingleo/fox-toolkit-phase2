/********************************************************************************
*                                                                               *
*                I d l e   I n h i b i t   ( W a y l a n d )                    *
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
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - An inhibitor is tied to a surface: the compositor keeps the screen on while
    that surface is visible and the inhibitor object exists.  There is nothing
    to do for a surface that is hidden, the compositor takes care of it.
  - A compositor which does not offer the manager gets nothing, and the screen
    saver behaves as it always did.
*/

namespace FX {

// Ask the compositor to keep the screen on while the toplevel is visible, or
// withdraw that request
void fxwaylandSetIdleInhibit(FXWaylandApp* app,FXID window,FXbool inhibit){
  FXWaylandSurface* surf;
  if(!app) return;
  if((surf=fxwaylandFindSurface(app,window))==nullptr) return;
  if(inhibit){
    if(surf->idleinhibitor) return;
    if(!app->idlemanager){
      fxwaylandTrace("no idle inhibit protocol, the screen saver is not held off\n");
      return;
      }
    surf->idleinhibitor=zwp_idle_inhibit_manager_v1_create_inhibitor(app->idlemanager,surf->surface);
    fxwaylandTrace("idle inhibited for window %u\n",(unsigned int)window);
    }
  else if(surf->idleinhibitor){
    fxwaylandFreeIdleInhibit(surf);
    fxwaylandTrace("idle inhibit released for window %u\n",(unsigned int)window);
    }
  wl_display_flush(app->display);
  }


// Drop the inhibitor of a surface; it has to go before the surface does
void fxwaylandFreeIdleInhibit(FXWaylandSurface* surf){
  if(!surf) return;
  if(surf->idleinhibitor){
    zwp_idle_inhibitor_v1_destroy(surf->idleinhibitor);
    surf->idleinhibitor=nullptr;
    }
  }

}

#endif
