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

**Clipboard.** `wl_data_device_manager_get_data_device(manager, seat)`; set the
selection from a `wl_data_source` (`offer(mime)` + `send(mime,fd)` handler,
`cancelled` on loss) using the serial of the last keyboard event; read a paste
through the `selection` offer with `wl_data_offer_receive(offer,mime,pipefd)` and
read the other end. Text mime types are `text/plain` and `UTF8_STRING`.

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

## Map of this repository

| File | Holds |
| --- | --- |
| `fox2/lib/fxwaylandpriv.h` | connection, surfaces, buffers, event codes, private API |
| `fox2/lib/fxwayland.cpp` | event queue, surfaces, buffers, backing store, commit, dump |
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
- `scripts/gate-wayland-click.sh` — headless sway + `scripts/wlxinject.c`, which
  keeps one `zwlr_virtual_pointer_v1` alive for the gesture and then moves,
  presses and releases it. `swaymsg seat - cursor ...` and `wlrctl` both fail on
  a headless sway: the seat has no pointer device, and `wlrctl` destroys its
  virtual pointer before the click is delivered.
- `scripts/gate-wayland-repaint.sh` — drives the same widget state in one click
  and in three and compares the two dumped frames; needs no reference image and
  catches ghosts of widgets that moved.
- Screenshot a session with `grim -o HEADLESS-1` (sway) or from a dumped frame
  with `FOX_WAYLAND_DUMP`.
- `FOX_WAYLAND_TRACE=1` prints connect, configure, expose, bind, commit, seat
  and key lines; `WAYLAND_DEBUG=1` prints the protocol stream. Read both before
  guessing.
