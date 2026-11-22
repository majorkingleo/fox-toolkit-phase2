# Wayland port and MDI: status and history

Status and problem log for the native Wayland platform module and the MDI
widgets. It summarises the git history up to `main` (commit `3ca6607`, 2026-11-21)
and records what each stage changed and which problems it uncovered. The design
and the per-phase plan live in `docs/wayland-plan.md`; this file is the short
version: what happened, what broke, what is still open.

## Current state

The Wayland module builds as a full platform (`FOX_PLATFORM=wayland`), with the
rasterizer, virtual window table, shells, input, popups, clipboard, DnD, text
input, scale, EGL and the whole widget set ported. It runs the `hello`, `button`,
`dpi`, `popup`, `menus`, `tooltip`, `clipboard`, `dnd`, `mditest` and calculator
gates under headless sway/Weston, and the stopwatch under a real KDE Wayland
session. Each phase of the port is gated by scripts in `scripts/` and described
in `docs/wayland-plan.md`.

MDI is ported and passes its drag gates
(`scripts/gate-wayland-mdi-drag.sh` under headless sway and
`scripts/gate-kwin-mdi-drag.sh` on a private KWin). The hard case is that MDI
children share one surface buffer: a moved window has to be painted out of the
place it left by the windows below it. Both gates check every frame of a drag,
not only the one it ends on.

## Timeline

Dates are commit dates from `git log`.

### Wayland platform, 2026-11-13 to 2026-11-18

- **Phase 0** (`9168099`): platform macros `FOX_X11` / `FOX_WAYLAND` / `WIN32`,
  module classification, build selection.
- **Phase 1** (`3ba9b8b`, `104b623`): platform-neutral software rasterizer and a
  portable region; scene test and pixel gate.
- **Phase 2** (`96ceea1`): glyph cache and text in the rasterizer.
- **Phase 3** (`ab5e5a2`, `c98efdf`, `261f05c`, `1c56195`, `b703f53`): the
  platform builds; visual, image, icon, DC, font, style and cursor modules.
- **Phase 4** (`7182783` to `1f5bfa7`): vendored protocols, scanner glue, the
  connection in `FXApp_wayland.cpp`, virtual window table, shell surfaces,
  shared-memory buffer pool, frame painting, the `hello` gate pixel-for-pixel.
- **Phase 5** (`4818195`, `568b30e`, `eceecce`): compositor configure and close,
  server-side decorations and client-side title bar fallback, interactive resize.
- **Phase 6** (`9036fdb`, `ef69bd8`, `1d52158`, `3ee062c`): seat, pointer,
  keyboard; clicks reach the widget under the pointer; moved widgets repaint.
- **Phase 7** (`2f5881b`, `1cebbad`): popup role, menus, nested menus, tool tips.
- **Phase 8** (`56ee8eb`, `3f8c209`, `aa350a7`): clipboard, primary selection,
  drag and drop.
- **Phase 9 start** (`44b9cc8`, `dbd14f8`, `2d8bfba`, `96c4bd6`, `fbb4da9`,
  `3d1182f`): text input, compositor scale, activation token, light/dark
  preference, toplevel icon, Autotools build.

Then the widget applications were ported onto the new platform (`5aa552d`,
`dc074ee`, `476ce03`, `1dbf287` and neighbours), OpenGL arrived through EGL
(`67649b8`), and theming followed over the next days (`e064066` to `9b1e17d`).

The later commits harden the frame model, which is where most of the remaining
bugs were:

- `bdbe1cc` (11-18): paint a parent before the children it covers.
- `606a764` (11-18): honour borderless windows, add a Wayland drag seam.
- `fc5014e` (11-18): clip a window to the ones above it, keep painting while
  input comes.
- `24d535d` onward (11-18/11-19): stopwatch as a Wayland gate; icon blending,
  supersampled analog face, focus crash guard.
- `8aabe6e`, `dff5ee7` (11-20): batch widget updates into one frame.
- `c053e31`, `e2eef0b` (11-21): MDI drag painting and frame pacing.

### MDI, 2026-11-16 to 2026-11-21

- `e9b5186` (11-16): port `FXMDIButton`, `FXMDIChild`, `FXMDIClient` and
  `mditest`; enable `GradientFill` on MinGW.
- `7b026e3` (11-16): `FXMDIChild` follows the window DPI.
- `146e987` (11-16): `mditest` shows how an application font follows a DPI
  change.
- `fc5014e` (11-18): Wayland clip/stacking fixes, all found through MDI.
- `c053e31` (11-21): fix MDI drag painting and frame pacing.
- `e2eef0b` (11-21): paint a moved MDI child last in the frame.

## Problems found and fixed

### Frame model and shared buffer (Wayland core)

- **Parent background painted over children.** Every window of a shell writes
  into one buffer, and a parent fills its background across the whole rect it
  covers. X11 clips a parent against its children; the Wayland repaint list did
  not. Fixed in `bdbe1cc` by handing the covered part of each visible child
  back to the child after the parent paints, parents before children.
- **Window painted over siblings stacked above it.** A window painted its
  background and content across its whole rect, overlapping siblings included.
  An MDI child's scrolled content is larger than the window that scrolls it, so
  its text ran over the whole workspace, and a child painting below another
  wiped the one on top. Fixed in `fc5014e`: `fxwaylandBindWindow()` computes the
  part the windows above leave uncovered, and the DC clips itself to it, as the
  X11 backend clips to its paint rectangle.
- **Covered areas merged into a bounding box.** Two thin strips in an L became a
  repaint of the whole window and its tree. Fixed in `c053e31` by keeping
  covered areas separate.
- **Commit in the middle of a paint wave.** A drag committed frames mid-wave, so
  the compositor saw the parent without the moved window or its content. Fixed
  in `c053e31` by dispatching the whole wave first and committing only when the
  event queue is empty.
- **Pointer drag starved the frames.** Painting and committing only happened
  when the event queue was empty, which a continuous motion stream never allows;
  the window stopped following the pointer. Fixed in `fc5014e`: paint one
  waiting rectangle and flush the frame before reading more input.
- **Use-after-free on close.** Closing an MDI child deletes the window while
  `dispatchEvent()` still uses the freed object for the cursor and traces. Fixed
  in `fc5014e` by looking the window up by id again after a handler runs.
- **Moved window painted under a lower window.** The cover pass for the strips a
  dragged MDI child left exposed windows below it, and one of those painted over
  the moved window after it had painted itself. Fixed in `e2eef0b`: the moved
  window paints itself last in the frame, after the strips and cover passes.
- **A moved window kept strips of its earlier places.** During a drag, 1 to 2
  pixel wide remnants of the frame, bevel and title bar of the moving window
  stayed in the frame at places it had already left, until the release repainted
  the client. Three faults in the sliced frame model: a strip clipped to the
  parent's current rectangle (the rest was dropped), `fxwaylandPaintQueued()`
  merging a window's rectangles into a bounding box that
  `fxwaylandPaintCovered()` then trusted, and `exposeArea()` registering a paint
  in shell coordinates while `fxwaylandExposeRect()` registered the same one in
  the coordinates of the window. The moved branch now repaints the whole tree
  once in stacking order, `FOX_WAYLAND_DUMP_FRAMES` keeps every frame of a drag,
  and both MDI gates check every frame against the one the drag ends on. Details
  in `docs/wayland-plan.md`, section "A moved window and the windows it
  uncovers".
- **A sub-window dragged to the corner grew over its neighbours.** A window
  whose top left corner lies outside the store (dragged to the upper left of an
  MDI client) had the buffer it drew into cut down to the visible part, which
  moved its drawing origin, and its scrolled content was no longer clipped to it
  and painted over the whole client. `FXRaster` has a drawing origin now, the DC
  always gets the whole store, and the visible rectangle is always intersected
  with the windows above and with the store; the bug build segfaults on the
  corner drag, the current one keeps the content inside the window. Details in
  `docs/wayland-plan.md`, section "A window that hangs out of the store", and
  the corner check in `scripts/gate-kwin-mdi-drag.sh`.
- **The content of a window lost the part another window did not cover.** A
  widget that paints a part of itself and then clears its clip painted its whole
  visible area: the DC's `rect` was the visible area instead of the paint
  rectangle, which is what a cleared clip means on X11 and Win32. A sub-window
  whose scroll area was repainted in parts whitewashed the part of the content
  that another window's repaint did not reach. The expose constructor intersects
  `rect` with the paint rectangle now; details in `docs/wayland-plan.md`,
  section "A widget that paints only a part of itself".
- **The cursor of a sub-window did not follow the pointer.** The cursor was
  remembered by the widget under the pointer only, so a sub-window that changes
  its own cursor between its title bar, its border and a drag kept the first one:
  the resize cursor stayed over the title bar and the drag cursor never appeared.
  The cache is keyed by the cursor as well now, and the motion path asks again
  after the widget handled the motion. `scripts/gate-wayland-mdi-cursor.sh`
  checks the wire log; details in `docs/wayland-plan.md`, section "The cursor of
  an MDI sub-window".
- **Pixel-block move of a child smears.** Copying the moved window's pixels
  inside the shared store (what X11 does with a native child window) was tried
  and dropped in `c053e31`: it smears the frame, the scroll viewport and the
  edges of a child that paints between two moves. A full repaint of the moved
  window is the correct model; the label extent cache and clip-aware text keep
  it cheap.
- **Update batching showed stale content.** A mode or style switch changes
  geometry, text and controls; Wayland committed every paint separately, so the
  store showed old content at the new size. Fixed in `8aabe6e`/`dff5ee7`:
  `beginUpdate()`/`endUpdate()` hold the frame, drain queued paints, repaint the
  tree in stacking order and commit one final frame.

### Input and focus

- **Click counter lost across events.** The first dispatcher replaced
  `FXApp::event` with a fresh `FXEvent` per raw event, so the second press of a
  double click always arrived as click 1. Fixed in `ae33e11` by keeping
  `FXApp::event` like the X11 and Win32 modules. Gate: `gate-wayland-doubleclick.sh`.
- **Accelerators did not run.** Keys were not dispatched to the shell. Fixed in
  `7421cf0`.
- **Focus on keyboard enter.** The core has no other way to learn that the
  compositor handed the keyboard to a shell, so the shell focuses its first
  focusable widget (`SEL_FOCUS_NEXT`). A keyboard leave does not raise
  `SEL_FOCUSOUT`, because the compositor in the gate reports the leave of one
  surface twice (documented near `gate-wayland-menu.sh`).
- **`FXTextField::onFocusSelf` crash.** Guarded in `3e8a40a` and recorded as a
  vendor patch in `docs/vendor-patches.md` (`79e0fea`).

### Popups, menus, tool tips

- Popup corner rounding and background cut (`fceb1ba`), drop shadows
  (`feb2148`), tooltip text change without resize (`9ea1b13`), menu hit-test and
  grab serial handling (see the popup/menu/tooltip gates in `docs/wayland-plan.md`).
- Borderless X11 windows needed the fifth Motif hints word for `DECOR_NONE`
  (`efe5c3e`); Wayland needed a client-side decoration switch and an interactive
  drag seam for borderless windows (`606a764`).

### Scale, resize, wheel

- Intermediate configures of a live resize were dropped (`3a1043e`), the parent
  tree is no longer exposed at every layout step (`722102b`), a resized shell is
  laid out before exposure (`db6dc03`), the previous frame is flushed when a
  configure arrives (`e4625d9`), painted frames are flushed before more input is
  read (`9841683`).
- Wheel axis flipped to match X11 (`cc60050`).
- Negative-sweep arcs drawn clockwise (`7dd8337`).

### Drawing and icons

- Disabled-icon etch mask (`7d13a51`), angled-text clip and rotation
  (`9f785f9`), rounded button radius (`0652372`), antialiased check marks
  (`0b669df`), antialiased rounded buttons (`e064066`).
- Icon alpha was clipped instead of blended, so NanoSVG antialiased edges landed
  full strength and looked jagged (`fc44c75`).
- The supersampled analog face lost its buffers on X11 and Win32 because
  `FXImage::create()` drops the pixel buffer unless `IMAGE_KEEP` is set;
  `restore()` after `create()` allocates it again (`c55721e`).

### MDI-specific

- `GradientFill` was compiled out on MinGW, so MDI title bars were flat
  (`e9b5186`).
- The MDI child title used the normal font of the system DPI, too large on a
  lower-DPI monitor; `applyTheme` now retargets it to the window DPI
  (`7b026e3`).
- The sample text font in `mditest` kept the system DPI; the test overrides
  `applyTheme` and rebuilds the font for the window DPI and display scale
  (`146e987`). This is the pattern for any application-made font.
- Title text of an unfocused MDI child is white on a light gradient; upstream
  behaviour, unchanged (`docs/test-port-plan.md`).
- The Wayland MDI drag problems are the frame-model items above (`fc5014e`,
  `c053e31`, `e2eef0b`); the drag gate compares the final frame of a 24-step
  drag against a 2-step drag and checks one committed frame per step.

## Still open

- `docs/charts-plan.md` plans a new reusable chart widget (antialiased curves,
  smooth zoom/scroll, many samples, SVG export). The FOX 1.7.95 chart package
  is already copied to `fox2/apps/chart` and builds `charttest`; the new widget
  is the remaining piece.
- MDI theming is recorded as PARTIAL/OPEN in `docs/theme-widget-checklist.md`
  (title background/text, active/inactive states, border and button decoration).
- Browser thin client deferred (`docs/modernization-plan.md`, noted in
  `docs/wayland-plan.md`); the presentation layer is meant to be shared when it
  resumes.
