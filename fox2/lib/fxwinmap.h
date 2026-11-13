/********************************************************************************
*                                                                               *
*                     V i r t u a l   W i n d o w   M a p                     *
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
#ifndef FXWINMAP_H
#define FXWINMAP_H

/*
  Notes:

  - Private header: not installed, not part of the FOX API.
  - Wayland has no child windows: a surface belongs to a shell, and everything
    below it exists only in the client.  This table holds those virtual windows
    for the platform module: parent, geometry, stacking order and shown state,
    so that hit-testing, reparenting, raising and the clip computation can work
    as they do on X11.
  - Ids are synthetic and never reach the compositor; the module returns 0 for
    a shell, because a shell is the surface itself.
  - Children are kept in a stacking list, bottom to top, exactly like the X11
    server keeps its children: the last child is drawn on top and is hit first.
  - The table is platform neutral on purpose, so the same code serves the
    Wayland module and the browser frontend, and it can be tested headless on
    any host (see tests/winmap).
*/

namespace FX {


/// One virtual window
struct FXWinEntry {
  FXID    id;                   // Synthetic window id, never 0
  FXID    parent;               // Parent window, 0 for a shell
  FXID    owner;                // Owner of a popup, 0 when there is none
  FXID    first;                // Bottom-most child, 0 when there is none
  FXID    last;                 // Top-most child, 0 when there is none
  FXID    prev;                 // Window below this one
  FXID    next;                 // Window above this one
  FXint   x;                    // Position relative to the parent
  FXint   y;
  FXint   w;                    // Size in pixels
  FXint   h;
  FXuint  flags;                // WINDOW_* flags
  };


/// Flags of a virtual window
enum {
  WINMAP_SHOWN  = 1,            // Window is mapped
  WINMAP_SHELL  = 2,            // Window owns the surface
  WINMAP_POPUP  = 4             // Window is a popup surface
  };


/// Table of the virtual child windows of one connection
class FXAPI FXWinMap {
private:
  FXWinEntry* entries;          // Entries
  FXuint      count;            // Number of entries
  FXuint      alloced;          // Allocated entries
  FXID        firstshell;       // Bottom-most shell
  FXID        lastshell;        // Top-most shell
  FXID        serial;           // Last handed out id
  FXuint      flags;            // Global flags
private:
  FXWinEntry* find(FXID id);
  const FXWinEntry* find(FXID id) const;
  FXWinEntry* add();
  void unlinkEntry(FXWinEntry* e);
  void linkTop(FXWinEntry* e);
  void removeSubtree(FXID id);
public:
  FXWinMap();
  ~FXWinMap();

  /// Create a window below the given parent; returns its id
  FXID create(FXID parent,FXint x,FXint y,FXint w,FXint h,FXuint flags=0);

  /// Hand out an id for a window that is not in the table, such as the root
  FXID newId(){ return ++serial; }

  /// Destroy a window and all its children
  void destroy(FXID id);

  /// Return true if the id is known
  FXbool hasWindow(FXID id) const;

  /// Return the number of windows
  FXuint getCount() const { return count; }

  /// Return the entry of a window, or nullptr
  const FXWinEntry* getEntry(FXID id) const;

  /// Parent, siblings and children
  FXID getParent(FXID id) const;
  FXID getFirst(FXID id) const;
  FXID getLast(FXID id) const;
  FXID getPrev(FXID id) const;
  FXID getNext(FXID id) const;

  /// Owner of a popup; the shell a popup surface hangs off
  void setOwner(FXID id,FXID owner);
  FXID getOwner(FXID id) const;

  /// Geometry relative to the parent
  void setGeometry(FXID id,FXint x,FXint y,FXint w,FXint h);
  FXRectangle getGeometry(FXID id) const;

  /// Geometry in the coordinates of the shell
  FXRectangle getGlobalGeometry(FXID id) const;

  /// Placement flags of setPlacement()
  enum {
    PLACE_X=1,                  // New x
    PLACE_Y=2,                  // New y
    PLACE_W=4,                  // New width
    PLACE_H=8                   // New height
    };

  /// Change part of the geometry
  void setPlacement(FXID id,FXint x,FXint y,FXint w,FXint h,FXuint flags);

  /// Shown state; a window is visible when it and all its parents are shown
  void setShown(FXID id,FXbool shown);
  FXbool isShown(FXID id) const;
  FXbool isVisible(FXID id) const;

  /// Move the window to the top or the bottom of the stacking order
  void raise(FXID id);
  void lower(FXID id);

  /// Move a window under a new parent, keeping its place on the screen
  void reparent(FXID id,FXID parent);

  /// Return the deepest visible window at a point of the screen; the shells
  /// carry the place they were given, so screen coordinates are what it needs
  FXID hitTest(FXint gx,FXint gy) const;

  /// Return the window itself or its nearest ancestor that is a shell
  FXID getShell(FXID id) const;
  };

}

#endif
