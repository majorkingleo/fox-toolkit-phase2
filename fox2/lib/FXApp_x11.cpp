/********************************************************************************
*                                                                               *
*                A p p l i c a t i o n   O b j e c t   ( X 1 1 )                *
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
#include "fx.h"
#include "fxplatform.h"

#if !defined(WIN32)

/*
  Notes:

  - The non-Windows backend currently is X11, selected at compile time; a
    Wayland adapter would replace this file rather than extend it.

  - X11 has no process DPI awareness, no system DPI, and no per-window DPI.
    The display scale is configured instead, through [display] scale=<percent>
    or FXApp::setDisplayScale().
*/


namespace FX {


// X11 has no process DPI awareness
FXint fxplatformSetDPIAwareness(const FXString&){
  return DPIAWARENESS_UNKNOWN;
  }


// X11 has no system DPI
FXint fxplatformGetSystemDPI(){
  return 0;
  }


// X11 has no per-window DPI
FXint fxplatformGetWindowDPI(FXID){
  return 0;
  }


}

#endif
