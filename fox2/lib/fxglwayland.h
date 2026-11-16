/********************************************************************************
*                                                                               *
*              O p e n G L   S u b s u r f a c e   ( W a y l a n d )            *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#ifndef FXGLWAYLAND_H
#define FXGLWAYLAND_H

/*
  Notes:

  - A Wayland window has no native child windows, so a GL canvas is a
    wl_subsurface of the surface of its shell.  The EGL window and its surface
    belong to that subsurface, and the canvas draws through EGL, not through the
    shared memory buffer of the shell.
  - Every call is a no-op when the platform is not Wayland or the library was
    built without EGL, so the core can call the hooks unconditionally.
*/

namespace FX {

class FXApp;

/// Prepare the EGL display of the connection; false when EGL is unavailable
FXbool fxglwaylandOpen(FXApp* app);

/// Tear down the EGL display and any canvas still registered
void fxglwaylandClose(FXApp* app);

/// EGL display of the connection, or nullptr
void* fxglwaylandDisplay(FXApp* app);

/// Create the subsurface of a GL canvas; false when there is no subcompositor
FXbool fxglwaylandCreateCanvas(FXApp* app,FXID window);

/// Destroy the subsurface of a GL canvas
void fxglwaylandDestroyCanvas(FXApp* app,FXID window);

/// Move and resize the subsurface after the widget moved
void fxglwaylandPlaceCanvas(FXApp* app,FXID window);

/// Map or unmap the subsurface of a GL canvas
void fxglwaylandShowCanvas(FXApp* app,FXID window,FXbool shown);

/// EGL window of a GL canvas, or nullptr
void* fxglwaylandEglWindow(FXApp* app,FXID window);

/// EGL surface of a GL canvas at the given size; created or resized on demand
void* fxglwaylandEglSurface(FXApp* app,FXID window,FXint w,FXint h,void* config);

}

#endif
