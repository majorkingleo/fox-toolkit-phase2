/********************************************************************************
*                                                                               *
*            W a y l a n d   P l a t f o r m   P r i v a t e   D a t a          *
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
#ifndef FXWAYLANDPRIV_H
#define FXWAYLANDPRIV_H


////////////////////  DO NOT INCLUDE THIS PRIVATE HEADER FILE  //////////////////

// Private data shared by the Wayland modules: the connection and everything
// bound on it, the queue of neutral events, the virtual window table and the
// surface of every shell.  Only FXApp_wayland.cpp and the window modules use
// it; the public headers stay free of Wayland types.


#include "fxdefs.h"
#include "fxwinmap.h"

#if defined(FOX_WAYLAND)

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"


namespace FX {


/// Codes of the neutral raw event
enum {
  FXXWL_NONE       = 0,         // No event
  FXXWL_EXPOSE     = 1,         // Repaint a window rectangle
  FXXWL_CONFIGURE  = 2,         // The compositor resized a shell
  FXXWL_CLOSE      = 3,         // The compositor asked to close a shell
  FXXWL_ENTER      = 4,         // Pointer entered a window
  FXXWL_LEAVE      = 5,         // Pointer left a window
  FXXWL_MOTION     = 6,         // Pointer moved
  FXXWL_BUTTON     = 7,         // Pointer button pressed or released
  FXXWL_KEY        = 8          // Key pressed or released
  };


/// State of a toplevel, as the compositor reports it and as we request it
enum {
  FXXWL_STATE_MAXIMIZED  = 1,           // Toplevel is maximized
  FXXWL_STATE_FULLSCREEN = 2,           // Toplevel covers the output
  FXXWL_STATE_ACTIVATED  = 4,           // Toplevel holds the focus
  FXXWL_STATE_MINIMIZED  = 8            // Toplevel was asked to iconify
  };


/// The surface of one shell, and the buffers it draws into
struct FXWaylandApp;
struct FXWaylandBuffer;

struct FXWaylandSurface {
  FXWaylandApp*              app;        // The connection
  FXID                       window;     // Virtual window id of the shell
  wl_surface*                surface;    // The surface itself
  xdg_surface*               xdsurface;  // Shell surface role
  xdg_toplevel*               toplevel;   // Toplevel role, for shells
  xdg_popup*                 popup;      // Popup role, for popups
  FXWaylandBuffer*           buffers;    // Buffer pool
  FXColor*                   store;      // Content of the shell window tree
  FXint                      storew;     // Size of the backing store
  FXint                      storeh;
  FXbool                     dirty;      // Something was drawn into the store
  FXbool                     configured; // The compositor configured the shell
  FXuint                     state;      // FXXWL_STATE_* of the toplevel
  FXint                      width;      // Width of the pool buffers
  FXint                      height;     // Height of the pool buffers
  FXWaylandSurface*          next;       // Next surface
  };


/// One shared memory buffer of a shell surface
struct FXWaylandBuffer {
  wl_buffer*                 buffer;     // The buffer
  FXColor*                   pixels;     // Mapped pixels, width times height
  FXuchar*                   map;        // The mapping itself
  FXuval                     mapsize;    // Size of the mapping
  FXint                      width;      // Size in pixels
  FXint                      height;
  FXbool                     busy;       // Still held by the compositor
  FXWaylandBuffer*           next;       // Next buffer of the pool
  };


/// Everything that belongs to the connection
struct FXWaylandApp {
  wl_display*       display;            // Connection to the compositor
  wl_registry*      registry;           // Global registry
  wl_compositor*    compositor;         // Surfaces and regions
  wl_shm*           shm;                // Shared memory buffers
  wl_seat*          seat;               // Input devices
  wl_output*        output;             // First output
  xdg_wm_base*      wmbase;             // Shell
  FXint             screenwidth;        // Size of the first output
  FXint             screenheight;
  FXID              rootwindow;         // Id of the virtual root window
  FXbool            argb;               // ARGB8888 buffers are supported
  FXbool            lost;               // Connection failed
  FXWaylandEvent*   events;             // Queued neutral events
  FXuint            eventcount;         // Number of queued events
  FXuint            eventalloced;       // Allocated events
  FXuint            eventhead;          // Index of the first queued event
  FXWinMap          childmap;           // Virtual child windows
  FXWaylandSurface* surfaces;           // Surfaces of the shells
  };


/// The connection behind the display handle
inline FXWaylandApp* fxwayland(void* display){
  return (FXWaylandApp*)display;
  }


/// Print a line of diagnostics when FOX_WAYLAND_TRACE is set
void fxwaylandTrace(const FXchar* format,...);

/// Append an event to the queue
void fxwaylandPost(FXWaylandApp* app,const FXWaylandEvent& event);

/// Find the surface of a shell
FXWaylandSurface* fxwaylandFindSurface(FXWaylandApp* app,FXID window);

/// Create the surface of a shell; returns nullptr on failure
FXWaylandSurface* fxwaylandCreateSurface(FXWaylandApp* app,FXID window,FXbool popup);

/// Destroy the surface of a shell
void fxwaylandDestroySurface(FXWaylandApp* app,FXID window);

/// Return the surface of the shell that a window belongs to, or nullptr
FXWaylandSurface* fxwaylandShellSurface(FXWaylandApp* app,FXID window);

/// Bind a window to the frame buffer of its shell; false when it has none
FXbool fxwaylandBindWindow(FXWaylandApp* app,FXID window,FXColor*& pixels,FXint& stride);

/// Commit the frames that were painted since the last call
void fxwaylandFlushFrames(FXWaylandApp* app);

/// Take the content away from a shell surface, which is how a toplevel hides
void fxwaylandHideSurface(FXWaylandSurface* surf);

/// Post an expose for a window and every visible window below it
void fxwaylandExpose(FXWaylandApp* app,FXID window);

/// Return an idle buffer of the given size, creating the pool when needed
FXWaylandBuffer* fxwaylandAcquireBuffer(FXWaylandSurface* surf,FXint w,FXint h);

/// Attach the buffer, damage the rectangle and commit the surface
void fxwaylandCommitBuffer(FXWaylandSurface* surf,FXWaylandBuffer* buf,FXint x,FXint y,FXint w,FXint h);

/// Write a committed buffer as PPM into $FOX_WAYLAND_DUMP, if that is set
void fxwaylandDumpBuffer(FXWaylandSurface* surf,FXWaylandBuffer* buf);

/// Drop the buffer pool of a surface
void fxwaylandFreeBuffers(FXWaylandSurface* surf);

}

#endif

#endif
