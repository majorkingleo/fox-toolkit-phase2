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
#include "FXString.h"
#include "fxwinmap.h"

#if defined(FOX_WAYLAND)

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "primary-selection-unstable-v1-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-toplevel-icon-v1-client-protocol.h"
#include "xdg-dialog-v1-client-protocol.h"
#include "idle-inhibit-unstable-v1-client-protocol.h"
#include "cursor-shape-v1-client-protocol.h"

// The keyboard translator of libxkbcommon, only ever used through pointers
// here so that the module that needs it includes its header
struct xkb_context;
struct xkb_keymap;
struct xkb_state;


namespace FX {


// An icon is an image; only the pointer is ever used here
class FXImage;

// The cursor of a widget; only the pointer is ever used here
class FXWindow;


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
  FXXWL_KEY        = 8,         // Key pressed or released
  FXXWL_WHEEL      = 9,         // Mouse wheel turned
  FXXWL_FOCUSIN    = 10,        // The keyboard focus is on a shell
  FXXWL_FOCUSOUT   = 11,        // The keyboard focus left a shell
  FXXWL_POPUPDONE  = 12,        // The compositor dismissed a popup
  FXXWL_DNDENTER   = 13,        // A drag entered a window
  FXXWL_DNDMOTION  = 14,        // A drag moved, or went to another window
  FXXWL_DNDLEAVE   = 15,        // A drag left the window
  FXXWL_DNDDROP    = 16,        // A drag was dropped
  FXXWL_IME        = 17         // An input method committed text
  };


/// State of a toplevel, as the compositor reports it and as we request it
enum {
  FXXWL_STATE_MAXIMIZED  = 1,           // Toplevel is maximized
  FXXWL_STATE_FULLSCREEN = 2,           // Toplevel covers the output
  FXXWL_STATE_ACTIVATED  = 4,           // Toplevel holds the focus
  FXXWL_STATE_MINIMIZED  = 8            // Toplevel was asked to iconify
  };


/// A surface can be on a handful of outputs at once; more is a clone setup
enum { FXMAX_OUTPUTS = 4 };

/// A shell can have a handful of rounded rectangles at once: its own surface, a
/// popup shadow, a scroll area viewport; more is a nesting nobody has asked for
enum { FXMAX_MASKS = 8 };


/// The surface of one shell, and the buffers it draws into
struct FXWaylandApp;
struct FXWaylandBuffer;
struct FXWaylandOutput;

/// One rounded rectangle of a shell, in store coordinates
struct FXWaylandMask {
  FXID        window;                   // Window the rounding was asked for
  FXint       radius;                   // Corner radius
  FXRectangle rect;                     // Rectangle whose corners are rounded
  };


/// A paint that is queued for a window, and the area it covers
struct FXWaylandPaint {
  FXID        window;                   // Window that is going to paint
  FXRectangle rect;                     // Part of it the paint covers
  };


struct FXWaylandSurface {
  FXWaylandApp*              app;        // The connection
  FXID                       window;     // Virtual window id of the shell
  wl_surface*                surface;    // The surface itself
  xdg_surface*               xdsurface;  // Shell surface role
  xdg_toplevel*               toplevel;   // Toplevel role, for shells
  xdg_popup*                 popup;      // Popup role, for popups
  zxdg_toplevel_decoration_v1* decoration;// Who draws the title bar
  xdg_dialog_v1*             dialog;     // Dialog role of an owned toplevel
  wp_viewport*               viewport;   // Maps the buffer onto the logical size
  wp_fractional_scale_v1*    fscale;     // The scale the compositor prefers
  FXint                      fscale120;  // That scale in 1/120 steps, 0 if none
  wl_output*                 outputs[FXMAX_OUTPUTS]; // Outputs the surface is on
  FXuint                     numoutputs; // Number of them
  xdg_toplevel_icon_v1*      ticon;      // Icon of a toplevel, while setting it
  FXWaylandBuffer*           iconbuffer; // Buffer the compositor copies it from
  zwp_idle_inhibitor_v1*     idleinhibitor; // Keeps the screen on, while it exists
  FXdouble                   scale;      // Scale this surface is drawn at
  FXint                      bufscale;   // Scale told to the compositor
  FXint                      logicalw;   // Size the compositor asked for, in
  FXint                      logicalh;   // logical pixels
  FXWaylandSurface*          parent;     // Surface the popup hangs off
  FXString                   btitle;     // Title shown in the client side bar
  FXint                      barh;       // Height of that bar, 0 when the compositor draws it
  FXbool                     borderless; // Window asked for no decorations
  FXWaylandMask              masks[FXMAX_MASKS]; // Rounded rectangles of this shell
  FXint                      nmasks;    // Number of them
  FXWaylandBuffer*           buffers;    // Buffer pool
  FXColor*                   store;      // Content of the shell window tree
  FXint                      storew;     // Size of the backing store
  FXint                      storeh;
  FXbool                     dirty;      // Something was drawn into the store
  FXbool                     configured; // The compositor configured the shell
  FXint                      wantw;      // Size the compositor asked for
  FXint                      wanth;
  FXuint                     state;      // FXXWL_STATE_* of the toplevel
  FXbool                     settled;    // One configure reached the core
  FXuint                     lastmask;   // Maximized and full screen state of that one
  FXuint                     configuregen;// Generation of the newest configure
  FXint                      width;      // Width of the pool buffers
  FXint                      height;     // Height of the pool buffers
  FXWaylandSurface*          next;       // Next surface
  };


/// One output of the compositor, and the scale it wants
struct FXWaylandOutput {
  FXWaylandApp*      app;               // The connection
  wl_output*         output;            // The output itself
  FXint              scale;             // Scale it prefers
  FXint              width;             // Current mode, in device pixels
  FXint              height;
  FXWaylandOutput*   next;              // Next output
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
  wl_subcompositor* subcompositor;      // Subsurfaces for GL canvases
  wl_shm*           shm;                // Shared memory buffers
  wl_seat*          seat;               // Input devices
  wl_pointer*       pointer;            // Pointer of the seat
  wl_keyboard*      keyboard;           // Keyboard of the seat
  struct xkb_context* xkbcontext;       // Keymap compiler
  struct xkb_keymap*  xkbkeymap;        // Keymap of the keyboard
  struct xkb_state*   xkbstate;         // State of the modifiers
  FXID              pointerwindow;      // Window the pointer is over
  FXuint            pointerserial;      // Serial of the last pointer event
  FXuint            pointerenterserial; // Serial of the pointer enter
  void*             cursorwindow;       // Widget whose cursor is shown
  void*             cursorlast;         // Cursor that was shown for it
  void*             waitcursor;         // Wait cursor while a wait block runs
  FXuint            keyboardserial;     // Serial of the last keyboard event
  FXint             pointerx;           // Position of the pointer in the shell
  FXint             pointery;
  FXuint            pointerbuttons;     // Buttons held down
  FXuint            modstate;           // Modifiers held down
  FXWaylandSurface* pointersurface;     // Surface the pointer is on
  FXWaylandSurface* keyboardsurface;    // Surface with the keyboard focus
  wl_output*        output;             // First output
  void*             egldisplay;         // EGL display, when GL is in use
  xdg_wm_base*      wmbase;             // Shell
  zxdg_decoration_manager_v1* decomanager; // Title bars, when offered
  xdg_wm_dialog_v1*          dialogmanager; // Modal dialog relationships, when offered
  xdg_toplevel_icon_manager_v1* iconmanager; // Window icons, when offered
  zwp_idle_inhibit_manager_v1* idlemanager; // Screen saver inhibition, when offered
  wp_cursor_shape_manager_v1* cursormanager; // Stock pointer shapes, when offered
  wp_cursor_shape_device_v1*  cursordevice;  // Shape device of the pointer
  xdg_activation_v1*         activation;    // Tokens that justify taking the focus
  void*                      activations;   // Tokens we are waiting for
  wp_viewporter*             viewporter;    // Scales a buffer to a logical size
  wp_fractional_scale_manager_v1* fsmanager;// Exact scales, when offered
  FXWaylandOutput*           outputs;       // All outputs of the compositor
  zwp_text_input_manager_v3* timanager;     // Input method, when offered
  zwp_text_input_v3*         tiinput;       // Our context on the seat
  wl_surface*                tisurface;     // Surface the context is on
  FXString                   preedit;       // Text the method is composing
  FXint                      tix;           // Caret rectangle of that surface
  FXint                      tiy;
  FXint                      tiw;
  FXint                      tih;
  FXbool                     tienabled;     // The context is enabled
  wl_data_device_manager*  ddmanager;       // Clipboard and drag and drop
  wl_data_device*          ddevice;         // The seat's data device
  wl_data_source*          dsource;         // What we offer on the clipboard
  wl_data_offer*           doffer;          // What somebody else offers
  zwp_primary_selection_device_manager_v1* psmanager; // The X11 primary selection
  zwp_primary_selection_device_v1*         psdevice;
  zwp_primary_selection_source_v1*         pssource;
  zwp_primary_selection_offer_v1*          psoffer;
  wl_data_source*          dragsource;      // What we are dragging
  wl_data_offer*           dragoffer;       // What a peer drags over us
  wl_data_offer*           pendingoffer;    // Announced, not yet a drag or clipboard
  FXDragType*              dragtypes;       // Drag types of that offer
  FXString*               dragmimes;       // Mime type of each of them
  FXuint                   numdragtypes;    // Number of types being dragged
  FXuint                   alloceddrag;     // Allocated entries
  FXuint                   dragactions;     // Actions the source allows
  FXuint                   dragaction;      // Action the target picked
  FXuint                   dragserial;      // Serial of the drag enter
  FXbool                   dragfinished;    // The drag is over
  wl_surface*              dragsurface;     // Surface the drag is over
  FXString*         typenames;          // Mime name of every drag type
  FXuint            numtypes;           // Number of registered drag types
  FXuint            allocedtypes;       // Allocated entries
  FXDragType*       offertypes;         // Drag types the clipboard offers
  FXString*         offermimes;         // Mime type of each of them
  FXuint            numoffertypes;      // Number of types on the clipboard
  FXuint            allocedoffer;       // Allocated entries
  FXDragType*       psoffertypes;       // Drag types the selection offers
  FXString*         psoffermimes;       // Mime type of each of them
  FXuint            numpsoffertypes;    // Number of types on the selection
  FXuint            allocedpsoffer;     // Allocated entries
  FXString          clipboardtext;      // Data we offer on the clipboard
  FXint             screenwidth;        // Size of the first output
  FXint             screenheight;
  FXID              rootwindow;         // Id of the virtual root window
  void*             foxapp;             // The FXApp, for fonts and colours
  FXbool            argb;               // ARGB8888 buffers are supported
  FXbool            lost;               // Connection failed
  FXbool            inlayout;           // A layout pass is running in the core
  FXuint            updatebatch;        // Widget update batches in progress
  FXbool            deferflush;         // Flush after the queued paints are done
  FXuint            paintgeneration;    // Paints dispatched so far
  FXuint            flushgeneration;    // Generation seen by the deferred flush
  FXWaylandEvent*   events;             // Queued neutral events
  FXuint            eventcount;         // Number of queued events
  FXuint            eventalloced;       // Allocated events
  FXuint            eventhead;          // Index of the first queued event
  FXWaylandPaint*   paints;             // Paints that are queued
  FXuint            numpaints;          // Number of queued paints
  FXuint            allocedpaints;      // Allocated entries
  FXWaylandPaint*   covered;            // Areas painted over other windows
  FXuint            numcovered;         // Number of areas
  FXuint            allocedcovered;     // Allocated entries
  FXWaylandPaint*   moved;              // Windows that moved, with the rect they left
  FXuint            nummoved;           // Number of moves
  FXuint            allocedmoved;       // Allocated entries
  FXWinMap          childmap;           // Virtual child windows
  FXWaylandSurface* surfaces;           // Surfaces of the shells
  };


/// The connection behind the display handle
inline FXWaylandApp* fxwayland(void* display){
  return (FXWaylandApp*)display;
  }


/// Print a line of diagnostics when FOX_WAYLAND_TRACE is set
void fxwaylandTrace(const FXchar* format,...);

/// Add the listeners of the seat, and drop the devices again with it
void fxwaylandOpenSeat(FXWaylandApp* app);
void fxwaylandCloseSeat(FXWaylandApp* app);

/// Find the surface that wraps a wl_surface
FXWaylandSurface* fxwaylandSurfaceOf(FXWaylandApp* app,wl_surface* surface);

/// Keysym and text of a key code, with the modifiers of the keyboard applied
FXuint fxwaylandKeySym(FXWaylandApp* app,FXuint key);
FXString fxwaylandKeyText(FXWaylandApp* app,FXuint key);

/// Append an event to the queue
void fxwaylandPost(FXWaylandApp* app,const FXWaylandEvent& event);

/// True when a queued paint already covers that part of the window
FXbool fxwaylandPaintCovered(FXWaylandApp* app,FXID window,const FXRectangle& rect);

/// Remember the part of a window that a queued paint covers
void fxwaylandPaintQueued(FXWaylandApp* app,FXID window,const FXRectangle& rect);

/// The paint of a window was dispatched
void fxwaylandPaintDispatched(FXWaylandApp* app,FXID window,const FXRectangle& rect);

/// Drop queued paints and repaint every shell in tree order
void fxwaylandRepaintAll(FXWaylandApp* app);

/// A frame went out, so no paint is queued any more
void fxwaylandPaintFlushed(FXWaylandApp* app);

/// Remember that a paint covered that area of a window, in shell coordinates
void fxwaylandCovered(FXWaylandApp* app,FXID window,const FXRectangle& area);

/// Take the first area that a paint left over another window, in shell
/// coordinates; false when there is none
FXbool fxwaylandCoveredPop(FXWaylandApp* app,FXWaylandPaint& area);

/// Remember that a window moved away from that rectangle, in shell coordinates
void fxwaylandMoved(FXWaylandApp* app,FXID window,const FXRectangle& was);

/// Find the surface of a shell
FXWaylandSurface* fxwaylandFindSurface(FXWaylandApp* app,FXID window);

/// Create the surface of a shell; returns nullptr on failure
FXWaylandSurface* fxwaylandCreateSurface(FXWaylandApp* app,FXID window,FXbool popup);

/// Give a window the popup role under the given parent window; false on failure
FXbool fxwaylandCreatePopup(FXWaylandApp* app,FXID window,FXID parentwindow,FXint x,FXint y,FXint w,FXint h,FXbool grab);

/// Mark an owned toplevel modal (or not); a compositor may block its parent
void fxwaylandSetModal(FXWaylandApp* app,FXID window,FXbool modal);

/// Ask for the cursor of the widget under the pointer
void fxwaylandSetCursor(FXWaylandApp* app,FXWindow* window,FXWindow* grabwindow);

/// A widget changed its own cursor; if it is the one the pointer is on, the
/// compositor is told again
void fxwaylandCursorChanged(FXWindow* window);

/// Ask the compositor to resize a toplevel with the corner under the pointer
FXbool fxwaylandStartResize(FXWaylandApp* app,FXID window);

/// Title of a shell, as shown in a client side title bar
void fxwaylandSetTitle(FXWaylandSurface* surf,const FXString& title);

/// Height a client side title bar needs, from the font of the application
FXint fxwaylandBarHeight(FXWaylandApp* app);

/// Paint the client side title bar into the store of its shell
void fxwaylandPaintBar(FXWaylandApp* app,FXWaylandSurface* surf);

/// Handle a press inside the client side title bar; true when it was used
FXbool fxwaylandBarPress(FXWaylandApp* app,FXWaylandSurface* surf,FXint x,FXint y);

/// Destroy the surface of a shell
void fxwaylandDestroySurface(FXWaylandApp* app,FXID window);

/// Return the surface of the shell that a window belongs to, or nullptr
FXWaylandSurface* fxwaylandShellSurface(FXWaylandApp* app,FXID window);

/// Bind a window to the frame buffer of its shell; false when it has none;
/// visible is the part of the window inside the windows above it, in the
/// coordinates of the window itself
FXbool fxwaylandBindWindow(FXWaylandApp* app,FXID window,FXColor*& pixels,FXint& w,FXint& h,FXint& stride,FXint& ox,FXint& oy,FXRectangle& visible);

/// Commit the frames that were painted since the last call
void fxwaylandFlushFrames(FXWaylandApp* app);

/// Take the content away from a shell surface, which is how a toplevel hides
void fxwaylandHideSurface(FXWaylandSurface* surf);

/// Post an expose for a window and every visible window below it
void fxwaylandExpose(FXWaylandApp* app,FXID window);

/// Post a paint for a rectangle of a window, in its own coordinates
void fxwaylandExposeRect(FXWaylandApp* app,FXID window,const FXRectangle& rect);

/// Round the corners of a rectangle of a shell surface; the corners become
/// transparent, so a popup shows the window behind it instead of its own
/// background, while the margin of its drop shadow stays outside
void fxwaylandSetRadius(FXWaylandApp* app,FXID window,FXint radius,FXint x,FXint y,FXint w,FXint h);

/// Return an idle buffer of the given size, creating the pool when needed
FXWaylandBuffer* fxwaylandAcquireBuffer(FXWaylandSurface* surf,FXint w,FXint h);

/// Make and drop a shared memory buffer that is not part of a surface pool
FXWaylandBuffer* fxwaylandCreateImageBuffer(FXWaylandApp* app,FXint w,FXint h);
void fxwaylandDestroyImageBuffer(FXWaylandBuffer* buf);

/// True while a paint of a window is still queued for the event loop
FXbool fxwaylandPaintPending(FXWaylandApp* app);

/// Attach the buffer, damage the rectangle and commit the surface
void fxwaylandCommitBuffer(FXWaylandSurface* surf,FXWaylandBuffer* buf,FXint x,FXint y,FXint w,FXint h);

/// Write a committed buffer as PPM into $FOX_WAYLAND_DUMP, if that is set
void fxwaylandDumpBuffer(FXWaylandSurface* surf,FXWaylandBuffer* buf);

/// Drop the buffer pool of a surface
void fxwaylandFreeBuffers(FXWaylandSurface* surf);

/// Register a mime type and return its drag type; an empty name is a failure
FXDragType fxwaylandRegisterType(FXWaylandApp* app,const FXString& mime);

/// Mime name of a drag type, or an empty string when it is not ours
FXString fxwaylandTypeName(FXWaylandApp* app,FXDragType type);

/// Drag type that stands for a mime type of a peer, registering it if needed
FXDragType fxwaylandTypeFromMime(FXWaylandApp* app,const FXchar* mime);

/// Bind the data devices of the connection and drop them again
void fxwaylandOpenDataDevice(FXWaylandApp* app);
void fxwaylandCloseDataDevice(FXWaylandApp* app);

/// Offer the clipboard to the compositor from a window; false when it failed
FXbool fxwaylandAcquireClipboard(FXWaylandApp* app,FXID window,
const FXDragType* types,FXuint numtypes);

/// Drop our ownership of the clipboard
FXbool fxwaylandReleaseClipboard(FXWaylandApp* app,FXID window);

/// Take the primary selection
FXbool fxwaylandAcquireSelection(FXWaylandApp* app,FXID window,const FXDragType* types,FXuint numtypes);
FXbool fxwaylandReleaseSelection(FXWaylandApp* app,FXID window);

/// Types offered by the clipboard or the selection of a peer
void fxwaylandClipboardTypes(FXWaylandApp* app,FXDragType*& types,FXuint& numtypes);
void fxwaylandSelectionTypes(FXWaylandApp* app,FXDragType*& types,FXuint& numtypes);

/// Read the clipboard or the selection; the caller frees the data
void fxwaylandClipboardData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size);
void fxwaylandSelectionData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size);

/// Which board a data request is about; the values are those of FXDNDOrigin of
/// the core, which the platform modules cannot include
enum {
  FXXWL_FROM_SELECTION=0,               // Primary selection
  FXXWL_FROM_CLIPBOARD=1,               // Clipboard
  FXXWL_FROM_DRAGNDROP=2                // Drag and drop
  };

/// Drag and drop actions, with the values of FXDragAction of the core
enum {
  FXXWL_DRAG_REJECT=0,                  // Reject all drop actions
  FXXWL_DRAG_ASK=1,                     // Ask
  FXXWL_DRAG_COPY=2,                    // Copy
  FXXWL_DRAG_MOVE=3,                    // Move
  FXXWL_DRAG_LINK=4,                    // Link
  FXXWL_DRAG_PRIVATE=5,                 // Private
  FXXWL_DRAG_ACCEPT=6                   // Accept any of them
  };

/// Ask the application for the data of a drag type it offers
FXbool fxwaylandOfferData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size,FXuint origin);

/// Tell the application that the clipboard or the selection went to a peer
void fxwaylandClipboardLost(FXWaylandApp* app);
void fxwaylandSelectionLost(FXWaylandApp* app);

/// Offer a drag of the given types from a window; false when it could not start
FXbool fxwaylandStartDrag(FXWaylandApp* app,FXID window,const FXDragType* types,FXuint numtypes);

/// End the drag a window started, and report the action the target took
FXuint fxwaylandEndDrag(FXWaylandApp* app,FXID window,FXbool drop);

/// Types offered by the drag that is over us, and their data
void fxwaylandDragTypes(FXWaylandApp* app,FXDragType*& types,FXuint& numtypes);
void fxwaylandDragData(FXWaylandApp* app,FXDragType type,FXuchar*& data,FXuint& size);

/// Accept the drag with a type and an action, so the source can show it
void fxwaylandAcceptDrag(FXWaylandApp* app,FXuint action,FXDragType type);

/// Finish the drag that was dropped and drop what we know about it
void fxwaylandFinishDrop(FXWaylandApp* app,FXuint action);

/// Add the listener of the primary selection device once it exists
void fxwaylandAttachSelectionDevice(FXWaylandApp* app);

/// Remember an output of the registry, and drop them all again
void fxwaylandAddOutput(FXWaylandApp* app,wl_output* output);
void fxwaylandFreeOutputs(FXWaylandApp* app);

/// Give a new surface its viewport and its fractional scale, and drop them
void fxwaylandPrepareScale(FXWaylandApp* app,FXWaylandSurface* surf);
void fxwaylandFreeScale(FXWaylandSurface* surf);

/// Give a toplevel the icon of the application, and drop the buffer again
void fxwaylandSetWindowIcon(FXWaylandApp* app,FXID window,FXImage* image);
void fxwaylandFreeWindowIcon(FXWaylandSurface* surf);

// Idle inhibition (fxidle_wayland.cpp)
void fxwaylandSetIdleInhibit(FXWaylandApp* app,FXID window,FXbool inhibit);
void fxwaylandFreeIdleInhibit(FXWaylandSurface* surf);

/// Ask the compositor to activate a window of ours, and drop the plumbing
void fxwaylandActivateWindow(FXWaylandApp* app,FXID window);
void fxwaylandCloseActivation(FXWaylandApp* app);

/// Follow the scale of the outputs before the first window is created
void fxwaylandApplyStartupScale(FXWaylandApp* app);

/// Scale a surface is drawn at, and the conversions of the protocol to it
FXdouble fxwaylandSurfaceScale(FXWaylandSurface* surf);
FXint fxwaylandScaleIn(FXWaylandSurface* surf,FXint v);
FXint fxwaylandScaleOut(FXWaylandSurface* surf,FXint v);

/// Tell the core that the compositor asked for a logical size
void fxwaylandPostConfigure(FXWaylandSurface* surf,FXint logicalw,FXint logicalh);

/// Create and drop the input method context of the seat
void fxwaylandOpenTextInput(FXWaylandApp* app);
void fxwaylandCloseTextInput(FXWaylandApp* app);

/// Enable and disable the input method while a window has the focus
void fxwaylandEnableTextInput(FXWaylandApp* app);
void fxwaylandDisableTextInput(FXWaylandApp* app);

/// Where the caret is, so the input method can put its window next to it
void fxwaylandSetTextInputSpot(FXWaylandApp* app,FXint x,FXint y);
void fxwaylandSetTextInputArea(FXWaylandApp* app,FXint x,FXint y,FXint w,FXint h);

/// Text the input method is composing, empty when it is not composing
FXString fxwaylandPreedit(FXWaylandApp* app);

}

#endif

#endif
