/********************************************************************************
*                                                                               *
*          F O X   P l a t f o r m   W i n d o w   I n t e r n a l              *
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
#ifndef FXPLATFORMWINDOW_H
#define FXPLATFORMWINDOW_H


////////////////////  DO NOT INCLUDE THIS PRIVATE HEADER FILE  //////////////////

// Internal seam between the portable window core and the platform module.  The
// build selects exactly one implementation at compile time.  Declarations in
// this header must never use platform types; the implementations live in
// FXWindow_win32.cpp and FXWindow_x11.cpp.
//
// Only native window creation, attaching and destruction live here.  Showing,
// hiding, placement, cursors and frame changes are later steps.


#include "fxdefs.h"


namespace FX {

class FXVisual;


/// Parameters for creating or attaching a native window.  Handles are opaque
/// to the core; the platform module knows what they point to.
struct FXWindowCreateInfo {
  void*       display;          // Display connection
  void*       instance;         // Application instance handle
  const void* classname;        // Native window class
  void*       userdata;         // Opaque pointer stored with the native window
  FXID        parent;           // Parent native window
  FXID        owner;            // Owner native window, 0 when there is none
  FXID        ownershell;       // Native shell of the owner, 0 when there is none
  FXID        shell;            // Native shell of this window, 0 for a shell itself
  FXID        colormap;         // Native colormap
  FXID        cursor;           // Default cursor, 0 when there is none
  FXID        xdndaware;        // XDND awareness atom/tag of the application
  FXint       x;                // Position in pixels
  FXint       y;
  FXint       w;                // Size in pixels, at least 1x1
  FXint       h;
  FXVisual*   visual;           // Visual to create the window for
  FXuint      flags;            // WINDOWINFO_* flags
  };


/// Flags for FXWindowCreateInfo::flags
enum {
  WINDOWINFO_SHELL=1,           // Top level window
  WINDOWINFO_TOPWINDOW=2,       // Decorated top level window
  WINDOWINFO_OVERRIDEREDIRECT=4,// Bypass the window manager
  WINDOWINFO_SAVEUNDER=8,       // Ask for save-unders
  WINDOWINFO_ENABLED=16,        // Window accepts input
  WINDOWINFO_SHOWN=32           // Window is mapped right away
  };


/// Create a native window and return its handle, or 0 on failure.
extern FXID fxplatformCreateWindow(const FXWindowCreateInfo& info);


/// Take over a native window owned by another application.
extern void fxplatformAttachWindow(const FXWindowCreateInfo& info,FXID window);


/// Destroy a native window created by fxplatformCreateWindow().
extern void fxplatformDestroyWindow(void* display,FXID window,FXID shell,FXID colormap,FXID xdndaware);


}

#endif
