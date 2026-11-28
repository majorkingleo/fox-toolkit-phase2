---
name: wayland-client
description: Implement, debug or review the Wayland client side of FOX (fox2 Wayland platform) or any raw libwayland code in this repository. Use when working on surfaces, xdg-shell toplevels and popups, wl_shm buffers, configure/ack, input seat, cursors, clipboard or drag and drop, and when a Wayland client misbehaves under weston, sway or KWin.
---

# Wayland client: how it works, and how FOX uses it

## Reference code worth reading

The `wl_examples` set by LeonCSt is the best small, complete client in C++
(freestanding, one topic per file):

| File | Teaches |
| --- | --- |
| `leon410.cpp` | registry walk, binding globals |
| `leon420.cpp` | `wl_output` geometry and mode |
| `leon430.cpp` | `wl_surface` + `xdg_surface` + `xdg_toplevel`, first buffer |
| `leon440.cpp` | configure/ack, resize, keyboard events |
| `leon450.cpp` | pointer, `wl_cursor_theme`, `wl_pointer_set_cursor` |
| `leon460.cpp` | composite cursors |
| `leon470.cpp` | FreeType text into a shm buffer |
| `leon490.h` / `leon491.cpp` / `leon492.cpp` | **popup**: `xdg_positioner`, `xdg_popup`, `popup_done` |
| `leon500.cpp` | **clipboard**: `wl_data_device`, `wl_data_source`, `wl_data_offer` |

Base URL: `https://raw.githubusercontent.com/LeonCSt/beginner_code/main/wl_examples/<file>`.
Read them for the protocol sequences; do not copy the code — this repository is
LGPL and keeps its own style (see the `fox-toolkit` skill).

## Canonical sequences

**Connect and bind.** `wl_display_connect` → `wl_display_get_registry` →
`wl_registry_add_listener` → `wl_display_roundtrip` to collect globals. Bind at
`FXMIN(version,what_you_implement)`. A surface needs `wl_compositor`, a buffer
needs `wl_shm`, a window needs `xdg_wm_base`.

**Toplevel.**
```
surface   = wl_compositor_create_surface(compositor)
xdsurface = xdg_wm_base_get_xdg_surface(wmbase, surface)
xdg_surface_add_listener(xdsurface, /* configure -> ack_configure */)
toplevel  = xdg_surface_get_toplevel(xdsurface)
xdg_toplevel_add_listener(toplevel, /* configure(width,height,states), close */)
wl_surface_commit(surface)          // no buffer yet: this asks for a configure
```

**Buffer.** shared memory file (`memfd_create`, or `mkstemp` in
`$XDG_RUNTIME_DIR` + `unlink` + `ftruncate`) → `mmap(MAP_SHARED)` →
`wl_shm_create_pool` → `wl_shm_pool_create_buffer(pool,0,w,h,w*4,format)`.
`WL_SHM_FORMAT_ARGB8888` for anything with transparency, `XRGB8888` otherwise.
FreeType/glyph blending needs ARGB.

**Paint.** `wl_surface_attach(surface,buffer,0,0)` +
`wl_surface_damage_buffer(surface,x,y,w,h)` (surface-local pixels) +
`wl_surface_commit`. Anything not damaged is whatever the buffer held before.

**Configure.** Two events, in this order: `xdg_toplevel.configure` (size,
0 = client chooses; state array: maximized, fullscreen, activated) then
`xdg_surface.configure(serial)`. Ack **that** serial, then resize and repaint.
Never attach a buffer before the first configure.

**Popup.** needs a parent `xdg_surface` and a `xdg_positioner`:
```
positioner = xdg_wm_base_create_positioner(wmbase)
xdg_positioner_set_size(positioner,w,h)
xdg_positioner_set_anchor_rect(positioner,parentx,parenty,parentw,parenth)
xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_BOTTOM_LEFT)
xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT)
popup = xdg_surface_get_popup(xdsurface, parent_xdsurface, positioner)
xdg_popup_add_listener(popup, /* configure(x,y,w,h), popup_done */)
```
`popup_done` means the compositor dismissed it (click outside, parent gone):
destroy the popup, never keep it.

**Input.** `wl_seat` capabilities → `wl_seat_get_pointer` / `wl_seat_get_keyboard`.
Pointer coordinates are surface-local; there is no absolute screen position, so
a client that needs one adds the place it believes its surface has. Keyboard
gives an xkb keymap as a file descriptor: `mmap`, `xkb_keymap_new_from_string`,
`xkb_state_new`, then `xkb_state_key_get_one_sym(keycode+8)` (the +8 is the
evdev offset) and `xkb_state_update_key` to keep modifiers current.

**Clipboard.** `wl_data_device_manager_get_data_device(manager, seat)`.  Copying
creates a `wl_data_source`, offers mime types, and calls
`wl_data_device_set_selection(source, serial)` with the serial of an input event;
the client then *answers* peers that ask for a mime type by writing into the pipe
they hand to the `send` handler.  Pasting asks the current offer (which arrives
with `wl_data_device.selection`) for a mime type and reads the pipe.  Primary
selection is the same with `zwp_primary_selection_device_manager_v1`.  Text mime
types are `text/plain` and `text/plain;charset=utf-8`.

**Drag and drop.** A gesture calls
`wl_data_device_start_drag(device, source, origin_surface, icon, serial)`; the
source then only reports `cancelled`, `drop_performed` or `dnd_finished`.  The
target receives `enter`, `motion`, `leave` and `drop` with a `wl_data_offer`,
answers with `wl_data_offer_set_actions()` and
`wl_data_offer_accept(offer, serial, mime)`, and finishes with
`wl_data_offer_finish()`.  Both sides read the payload from the pipe that
`wl_data_offer_receive()` produces.

**Text input.** `zwp_text_input_manager_v3_get_text_input(manager, seat)` makes
one context per seat; the compositor decides which surface it belongs to with
`enter`/`leave`, following the keyboard focus. `enable()`, `set_content_type()`,
`set_cursor_rectangle()` and `disable()` do nothing until `commit()` follows, and
`disable()` is only legal after `enable()`.  The caret rectangle is what places
the pre-edit window.  Committed text goes to the focus widget as a key event with
`event.text`, as XIM does on X11.

**Activation.** A client cannot raise or focus its own windows. It calls
`xdg_activation_v1.get_activation_token()`, sets the serial of the input event
that caused the request (and the surface) on the token, commits it, and then
calls `xdg_activation_v1.activate(name, surface)` with the name that arrives in
the token's `done` event. Without a serial the compositor may still grant it,
which is what happens for the first window of an application.

## Rules that each cost a bug here

- **One field for the event code.** The neutral event struct in this repository
  has both `type` and `code`; every producer must set `code`, or the dispatcher
  silently drops the event (this hid the configure for a whole step).
- **`wl_shm` buffers are recycled.** A buffer handed out either starts blank or
  holds the content of two frames ago. Incremental painting into buffers loses
  every pixel the frame did not touch; keep a backing store per shell and copy
  it into the buffer on commit.
- **Destroy children before globals.** Surfaces and buffers must be gone before
  `wl_shm`/`wl_compositor`/`xdg_wm_base`/`wl_display`. A client that closes the
  display first frees proxies that still get used and crashes in
  `wl_surface_destroy`.
- **Listener structs follow the installed header.** `wl_pointer_listener` at
  libwayland 1.26 has serial arguments and 11 members; extra members may be
  left out of the initializer, changed signatures may not.
- **`wl_array` is not typed.** Iterate with `size/sizeof(uint32_t)` and index
  the cast pointer; `wl_array_for_each` needs a `void*` cursor in C++.
- **Damage is in buffer coordinates** (`damage_buffer`), not surface
  coordinates after a scale; with scale 1 they coincide.
- **No grabs.** The pointer stays with the surface that received the press.
  Interactive move/resize go through `xdg_toplevel_move/resize` with the seat
  and the serial of the press.
- **A client-side pointer grab is not a server grab.** `FXApp::mouseGrabWindow`
  only redirects what the backend dispatches, so anything the X server used to
  do on its own has to be reproduced in `lib/FXApp_wayland.cpp`.  The popup case
  is the expensive one: a menu posted at the pointer appears under the cursor,
  FOX drops its own grab there, and the press that has to dismiss the menu lands
  on the widget below it instead.  Presses inside a grabbing popup go to the
  widget under the pointer, presses outside unpost the popup (see
  `docs/wayland-plan.md`, "A press inside a popup and the press that dismisses
  it").
- **Hiding a window** means attaching a null buffer and committing; there is no
  unmap request.
- **A toplevel surface is the whole window, decorations included.** With a
  client side title bar the frame is the window plus the bar, so every window
  offset and every pointer coordinate shifts by the bar height, and hit testing
  has to keep points inside the bar with the shell instead of the widget below.
- **Focus changes pixels.** A window with the keyboard focus draws its focus
  rectangle, so two captures of the same window differ by it: a pixel gate has
  to agree on whether the capture had focus (a compositor without a seat gives
  no focus at all).
- **Focus arrives from the compositor.** The client must focus its own initial
  widget when the keyboard enters a shell; the core has no other signal.
- **`wl_pointer.button` carries Linux event codes**, not the 1, 2, 3 of X11:
  272 is the left button, 273 the right one, 274 the middle one. Passing the
  code straight to a toolkit that expects X11 numbers makes every click arrive
  as one unknown button and nothing reacts.
- **Nobody repaints what a moving window uncovers.** On X11 the server exposes
  the area a child leaves behind; with one buffer per shell every window shares
  the parent's pixels, so moving or resizing a widget must post an exposure for
  the parent (and its subtree, parents before children) or the old pixels stay
  as a ghost. Unmapping needs the same for its parent.
- **A popup grabbing its parent chain is mandatory.** A grabbing popup may only
  have a toplevel or *another grabbing popup* as its parent, so a submenu grabs
  too; and only the topmost popup of a chain may be destroyed, so a parent is
  dismissed from its children down. `xdg_popup.grab` must come before the first
  commit (grabbing a mapped popup is an error) and wants the serial of the
  press, not of an old pointer enter.
- **A tool tip must not grab.** It shows while the pointer only rests, and a
  grab would swallow the click meant for the window below. It is also the one
  popup FOX makes without an owner, so it needs the shell under the pointer as
  its parent surface.
- **The compositor never says which widget the pointer left.** Only surface
  enter/leave arrive; the chain of `SEL_ENTER`/`SEL_LEAVE` between widgets has to
  be synthesised from motion by comparing the window under the pointer with the
  previous one. Without it a menu entry never becomes active and a press on it
  does nothing, and `FXWindow::onEnter` needs `event.code=CROSSINGNORMAL` (a
  stale code from the previous event silently disarms the tool tip).
- **The clipboard selection only reaches the client with the keyboard focus.** A
  script or peer without a window never gets `wl_data_device.selection`; for
  that, and for clipboard managers, use `zwlr_data_control_v1`, which has no
  focus requirement and sets the selection without a serial.
- **`cancelled` on a data source is the only signal that the clipboard went to
  somebody else.** Without acting on it the application keeps answering with its
  own copy and a paste returns stale data.
- **An offer arrives twice** (`data_offer`, then `selection`) with the *same*
  proxy. Destroying it in between, or installing the listener twice, loses its
  mime types and crashes on the second destroy; track it by identity.
- **A `data_offer` carries the mime types before anything says what it is for.**
  `selection` makes it the clipboard, `enter` makes it a drag, and either can
  come first. Keep the pending offer and record every mime type in both lists;
  the slot that loses the race drops its *list*, never the shared proxy.
- **Do not clear the clipboard list at `selection` or the drag list at `enter`.**
  The mime types of the new offer are already recorded by then, so clearing
  there throws away the offer that is being adopted — the target then accepts
  nothing and the source sees `cancelled` after the drop. Drop the stale offer
  when the next `data_offer` is announced instead.
- **`endDrag()` must not destroy the source before the compositor answers.** In
  a self-drag (both widgets in one client) `drop` reaches the target only after
  the source has answered; destroying the `wl_data_source` early cancels the
  drag and the target never reads the data. Wait for `dnd_finished`/`cancelled`.
- **`wl_data_offer_accept()` takes a mime type in this wayland build**
  (`wl_data_offer.accept(offer, serial, mime)`), not an action, and the action
  enum has only `NONE`, `COPY`, `MOVE` and `ASK` — there is no `LINK`.
- **All windows of the client may share one `wl_surface`**, so a drag inside the
  application never leaves it and the compositor sends `enter` once. Resolve the
  widget per motion event and raise `SEL_DND_ENTER`/`SEL_DND_LEAVE` yourself,
  the same way the `SEL_ENTER` chain is synthesised for menus.
- **The scale of the compositor is not a DPI.** `wl_output.scale` is per output,
  `wp_fractional_scale_v1.preferred_scale` is per surface in 1/120 steps, and a
  surface learns which outputs it is on only through `wl_surface.enter/leave`.
  Draw the bigger buffer yourself and tell the compositor the logical size with
  `wp_viewport.set_destination()`; the fallback for a compositor without the
  viewporter is `wl_surface.set_buffer_scale()` with a whole number.
- **Everything on the wire is in logical pixels.** Configure sizes, pointer and
  drag positions, and popup placement all have to be converted at the seam once
  the client draws at the scale instead of letting the compositor stretch it.

## Map of this repository

| File | Holds |
| --- | --- |
| `fox2/lib/fxwaylandpriv.h` | connection, surfaces, buffers, event codes, private API |
| `fox2/lib/fxwayland.cpp` | event queue, surfaces, buffers, backing store, commit, dump |
| `fox2/lib/fxglwayland.cpp`, `fxglwayland.h` | EGL display and the `wl_subsurface`/`wl_egl_window` of each GL canvas |
| `fox2/lib/FXGLVisual_wayland.cpp` | EGL config selection for `FXGLVisual` |
| `fox2/lib/fxseat_wayland.cpp` | seat, pointer, keyboard, xkb, focus events |
| `fox2/lib/FXApp_wayland.cpp` | connect/close, registry, event loop, event dispatch |
| `fox2/lib/fxplatformwindow_wayland.cpp` | window seam on the virtual window table |
| `fox2/lib/FXWindow_wayland.cpp`, `FXRootWindow_wayland.cpp` | window base |
| `fox2/lib/FXTopWindow_wayland.cpp`, `FXMainWindow_wayland.cpp` | toplevel |
| `fox2/lib/FXPopup_wayland.cpp`, `FXToolTip_wayland.cpp` | popups (role still to come) |
| `fox2/lib/FXDCWindow_wayland.cpp`, `FXRaster.cpp` | software drawing |
| `fox2/protocols/` | vendored protocol XML, scanned into the build |
| `docs/wayland-plan.md` | the phase plan and what each phase proved |

## Testing

- `scripts/build-fox2-wayland.sh` (target `all` for the tests) and
  `FOX_WAYLAND_TARGET=all`.
- `scripts/gate-wayland-hello.sh` — headless weston, private `HOME`, dumps the
  committed frame and diffs it against the X11 screenshot. Weston's headless
  backend has **no seat capabilities**, so input cannot be tested there.
- `scripts/gate-wayland-input.sh` — headless sway + `wtype`; types a space into
  the focused button and expects the application to quit by itself. Space the
  press and the release apart (`wtype -s 200 -k space`): sway drops the press
  when both arrive in the same compositor frame.
- `scripts/gate-wayland-click.sh` — headless sway + `tools/wlxinject.c`, which
  keeps one `zwlr_virtual_pointer_v1` alive for the gesture and then moves,
  presses and releases it. `swaymsg seat - cursor ...` and `wlrctl` both fail on
  a headless sway: the seat has no pointer device, and `wlrctl` destroys its
  virtual pointer before the click is delivered.
- `scripts/gate-wayland-doubleclick.sh` — double click in the file list of the
  ported PathFinder and checks that the "Unknown File Type" dialog appears; it
  fails when `FXApp::dispatchEvent` loses the click counter by using a fresh
  `FXEvent` instead of the persistent `FXApp::event`.
- `scripts/gate-wayland-repaint.sh` — drives the same widget state in one click
  and in three and compares the two dumped frames; needs no reference image and
  catches ghosts of widgets that moved.
- `scripts/gate-wayland-menu.sh` — menu and submenu of `tests/menus`, checks the
  grab, the nesting and both dismissal paths (click outside, pick an entry).
- `scripts/gate-wayland-tooltip.sh` — rests the pointer on a button and checks
  that the tool tip is a popup without a grab. The injector must stay alive
  during the wait (`sleep:`); when it exits the compositor removes the virtual
  pointer and the client sees the pointer leave.
- `scripts/gate-wayland-clipboard.sh` — `tests/clipboard` plus `scripts/wlclip.c`
  as the peer, in both directions (FOX copies, peer reads; peer writes, FOX
  pastes). wlclip uses `zwlr_data_control_v1`.
- `scripts/gate-wayland-foxterm-context.sh` — the two context menus of
  `apps/foxterm`: a right click posts a grabbing popup, a click outside
  dismisses it, Copy really puts the screen on the clipboard (read back with
  `wlclip`), mouse reporting owns the right button while `Shift` still opens the
  menu, and Duplicate of the tab menu adds a tab. The child of the reporting
  phase needs `stty -icanon`: without it the mouse report waits in the line
  discipline for a newline. `Shift` is held by `wtype -M shift -s 1200` while
  `wlxinject` clicks.
- `scripts/gate-wayland-dnd.sh` — `tests/dnd` and the same injector: press on the
  source, `move:` out of its surface, `move:` onto the target, release. The
  intermediate moves are required, because the compositor only starts delivering
  the drag once the pointer has left the surface of the source.
- `scripts/gate-wayland-ime.sh` — `WAYLAND_DEBUG=1` and a `wtype` key for the
  focus; checks on the wire that the text input is bound, enabled, given a caret
  rectangle and committed. A committed string needs an input method client, which
  a headless compositor does not have.
- `scripts/gate-wayland-gl.sh` — headless sway with the **GL renderer**
  (llvmpipe) and `tests/glcanvas`: an `FXGLCanvas` is an EGL window on a
  `wl_subsurface`; the test clears red, reads the pixel back with `glReadPixels`
  and exits 0 only for red. Needs a build with `--opengl`.
- `scripts/gate-wayland-scale.sh` — the same application twice, with
  `output HEADLESS-1 ... scale 1` and `scale 2`; compares the theme scale it
  reports and the size of the dumped frame.
- `scripts/gate-wayland-activation.sh` — `WAYLAND_DEBUG=1`; follows the token
  request, the `done` answer and the `activate` call on the wire.
- `scripts/gate-wayland-settings.sh` — no display at all; stands in for `gdbus`
  and `gsettings` with scripts in a private `PATH` and checks each step of the
  light/dark chain, portal first.
- `scripts/gate-wayland-icon.sh` — KWin virtual for `xdg_toplevel_icon_v1` and
  headless sway for the fallback; KWin is the only compositor here that offers
  the protocol, so the icon path is checked against it.
- **A pixel gate must hide the session bus.** The desktop preference comes from
  the portal on the bus even when `HOME` is private, so unset
  `DBUS_SESSION_BUS_ADDRESS` and `XDG_CONFIG_HOME` or the reference frame depends
  on the desktop of whoever runs the gate.
- A build that only prints `exit: 0` can still have failed in CMake: grep the
  build log for `CMake Error`, not only for compiler errors, and check that the
  object file of a file you just edited is newer than it.
- `WAYLAND_DEBUG=1` spells proxies `name#id`, not `name@id`; grep the log
  accordingly.
- Screenshot a session with `grim -o HEADLESS-1` (sway) or from a dumped frame
  with `FOX_WAYLAND_DUMP`.
- `FOX_WAYLAND_TRACE=1` prints connect, configure, expose, bind, commit, seat
  and key lines; `WAYLAND_DEBUG=1` prints the protocol stream. Read both before
  guessing.
