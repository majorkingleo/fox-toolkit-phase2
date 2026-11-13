/********************************************************************************
*                                                                               *
*            F O X   P l a t f o r m   I n t e r n a l   S e a m                *
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
#ifndef FXPLATFORM_H
#define FXPLATFORM_H


////////////////////  DO NOT INCLUDE THIS PRIVATE HEADER FILE  //////////////////

// Internal seam between the portable FOX core and the platform module.  The
// build selects exactly one implementation at compile time.  Declarations in
// this header must never use platform types; the implementations live in
// FXApp_win32.cpp and FXApp_x11.cpp.


#include "fxdefs.h"
#include "fxmath.h"
#include "FXString.h"


namespace FX {


/// Establish the requested process DPI awareness; returns the effective level.
/// Policies are "permonitorv2", "permonitor", "system", "unaware", or "auto".
extern FXint fxplatformSetDPIAwareness(const FXString& policy);


/// Return the system DPI, or 0 when the platform does not report one.
extern FXint fxplatformGetSystemDPI();


/// Return the DPI of the monitor a native window is on, or 0 when unknown.
extern FXint fxplatformGetWindowDPI(FXID window);


/// Return the display name to connect to, or the fallback if the platform has none.
extern const FXchar* fxplatformDefaultDisplay(const FXchar* fallback);


/// Return the font specification used before any setting is read.
extern const FXchar* fxplatformDefaultFont();


/// Return the display scale used when none is configured, or nullptr for none.
extern const FXchar* fxplatformDefaultScale();


}

#endif
