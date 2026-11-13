/********************************************************************************
*                                                                               *
*                P l a t f o r m   S e a m   ( W a y l a n d )                  *
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
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "fxplatform.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - The seam functions that only answer questions about the platform.
  - Wayland has no process DPI awareness and no DPI per window: the compositor
    reports a scale factor per output, which reaches the window through the
    per-window DPI storage of docs/dpi-plan.md when phase 9 reads wl_output and
    wp_fractional_scale_v1.  Until then the answers are the neutral ones.
  - The display is the wayland socket name in WAYLAND_DISPLAY, exactly like
    X11 reads DISPLAY; libwayland falls back to "wayland-0" itself, so an empty
    answer is also valid.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Wayland has no process DPI awareness
FXint fxplatformSetDPIAwareness(const FXString&){
  return DPIAWARENESS_UNKNOWN;
  }


// Wayland reports scale factors, not a system DPI
FXint fxplatformGetSystemDPI(){
  return 0;
  }


// Wayland has no per-window DPI; the surface scale is used instead
FXint fxplatformGetWindowDPI(FXID){
  return 0;
  }


// Display name to connect to
const FXchar* fxplatformDefaultDisplay(const FXchar* fallback){
  const FXchar* str=getenv("WAYLAND_DISPLAY");
  return str?str:fallback;
  }


// Default font of the application
const FXchar* fxplatformDefaultFont(){
  return "Sans,90";
  }


// The compositor tells the client the scale, so nothing is configured
const FXchar* fxplatformDefaultScale(){
  return nullptr;
  }

}

#endif
