# Wayland surface plan

Plan for a native Wayland platform module for `fox2`: windows backed by
`xdg_toplevel`/`xdg_popup` surfaces, pixels delivered through `wl_shm`, input
through `wl_seat`. It builds on the platform seam (`docs/platform-seam-plan.md`,
seams 1 to 62) and is the first test of whether that seam is wide enough for a
third platform. The browser thin client in `docs/modernization-plan.md` shares
its presentation layer (phases 1 and 2 below).

## Goals and non-goals

Goals:

- `hello`, `button`, `dpi` and then the widget set run as a native Wayland client
  with no X11 library linked.
- Same compile-time selection as the other platforms: one platform per binary,
  no plugins, no runtime loading, no dual X11 + Wayland binary.
- Small dependency footprint: `libwayland-client`, `libxkbcommon`, FreeType and
  fontconfig (Xft already needs the last two). No GTK, Qt, cairo or pixman.
- Public headers stay free of platform types.

Non-goals for the first release: OpenGL (`FXGLCanvas`, later via `wl_egl_window`),
session management, system tray, XEMBED, screen savers, absolute window
positioning (Wayland forbids it), pointer warping.

## Starting point (measured, not assumed)

- Every platform conditional in the `fox2/lib` core classes lives in a
  `_win32.cpp` or `_x11.cpp` module. Public headers declare members and hooks on
  all platforms (seams 56 to 62).
- The `_x11.cpp` modules are 35 files, about 15,000 lines. They are not all X11:
  - About 4,500 lines are plain POSIX that a Wayland build needs unchanged:
    `FXThread`, `FXMutex`, `FXAutoThreadStorageKey`, `FXPipe`, `FXDir`,
    `FXFile`, `FXProcess`, `FXStat`, `FXSystem`, `FXPath`, `FXIODevice`,
    `fxutils`.
  - About 10,500 lines talk to Xlib, Xft, XRender, XShm or XInput and need a
    Wayland counterpart: `FXApp` (2,636), `FXFont` (1,711), `FXDCWindow` (1,214),
    `FXImage` (1,145), `FXWindow` (1,124), `FXVisual` (982), `FXTopWindow` (888),
    `FXComposeContext` (434), `fxpriv` (389), `FXIcon`, `FXStyle`, `FXRegion`,
    `FXCursor`, and the small stubs.
- The guard of the X11 modules was `#if !defined(WIN32)`, so "not Windows" meant
  "X11". Phase 0 replaced it by `#if defined(FOX_X11)` (done).
- Drawing is server-side: `FXDCWindow_x11.cpp` issues `XFillRectangle`,
  `XftDrawStringUtf8`, `XRenderComposite`; `FXImage_x11.cpp` uses `XImage`/XShm;
  `FXRegion_x11.cpp` uses `XRegion`. Wayland has no drawing requests, so the
  client must rasterize into a buffer itself.
- Every `FXWindow`, children included, owns a native window id. Wayland has no
  child windows.
- `FXRawEvent` is a public typedef of `_XEvent` or `tagMSG` in `fxdefs.h`.
- FOX key codes are X keysyms and `libxkbcommon` returns the same values, so the
  keyboard map needs no translation table.

## Design decisions

These are recommendations; each is called out for review at the end.

1. **Compile-time platform choice.** New CMake option `FOX_PLATFORM=x11|wayland|win32`
   (default by host) and an Autotools `--with-wayland`. The macros are `FOX_X11`,
   `FOX_WAYLAND` and `WIN32` (`X11` is taken by the `FXFont::X11` enumerator);
   modules are `<Class>_wayland.cpp` guarded by `#if defined(FOX_WAYLAND)`. Wayland sessions that need an X11 build use Xwayland.
2. **One `wl_surface` per shell, virtual child windows.** Only shells (`FXShell`,
   `FXTopWindow`, `FXPopup`, `FXToolTip`) get a surface and an `xdg_*` role. The
   module hands out synthetic `FXID`s for all other windows and keeps a private
   table: id, parent, rectangle, stacking order, shown flag. This stays inside
   the platform module because the seam already routes creation, placement,
   show, reparent and raise through `fxplatformCreateWindow()`,
   `fxplatformPlaceWindow()`, `fxplatformShowWindow()`,
   `fxplatformReparentWindow()` and `fxplatformRaiseWindow()`.
   - Drawing to a child id maps to the shell buffer, the child origin, and a
     clip to the child rectangle minus the siblings stacked above it.
   - Pointer events hit-test the table to find the deepest child, then enter the
     existing event path with that id, so `FXApp::findWindowWithId()` and the
     widget code are unchanged.
   - Exposure is synthesized into `FXApp::addRepaint()` (declared on every
     platform since seam 61) on show, move, resize and buffer loss.
3. **Software rendering into `wl_shm`.** A small internal rasterizer draws the
   `FXDC` primitives into a premultiplied ARGB8888 buffer. It is platform
   neutral (private header, no X or Wayland types) so the browser frontend can
   reuse it and it can be tested headless on any host, including Windows.
   Hinted FreeType glyphs, no antialiased geometry, so the classic theme stays
   pixel-comparable with the X11 build.
4. **No colormaps, no visuals.** `FXVisual_wayland` reports a fixed 32-bit
   true-colour visual. `FXImage` server-side data is the client buffer.
5. **Decorations.** Request server-side decorations through
   `xdg-decoration-unstable-v1`. Where the compositor refuses (GNOME, Weston),
   draw a minimal client-side title bar and resize edges in the shell, using
   `xdg_toplevel.move`/`resize`. `libdecor` is not used (extra dependency); it
   remains an option if the minimal bar proves inadequate.
6. **Scale.** Integer `wl_surface.set_buffer_scale` first, then
   `wp_fractional_scale_v1` with `wp_viewporter`. Both feed the per-window DPI
   storage already built in `docs/dpi-plan.md` (`FXWindow::scale()`).
7. **Protocols.** The XML files are vendored under `fox2/protocols/` with their
   license headers; glue code is produced at build time by `wayland-scanner`.
   Required: `wl_compositor`, `wl_shm`, `wl_seat`, `wl_output`, `xdg-shell`,
   `wl_data_device_manager`. Optional, probed at run time: `xdg-decoration`,
   `fractional-scale-v1`, `viewporter`, `cursor-shape-v1`,
   `primary-selection-unstable-v1`, `text-input-unstable-v3`, `xdg-output`.

## Semantics Wayland does not provide

| FOX or X11 behaviour | Wayland module behaviour |
| --- | --- |
| Set and read the absolute position of a window | Position of a shell is ignored and reported as the last popup or `0,0`; coordinates inside a shell are exact |
| `getRoot*`, `translateCoordinatesFrom` to the root | Root is the shell surface; root coordinates equal shell coordinates |
| `FXPopup::popup()` at absolute coordinates | Anchor rectangle relative to the owner shell, `xdg_positioner`, `xdg_popup` with explicit grab |
| Pointer and keyboard grab (menus, drags) | Popup grab; `fxplatformReleaseMouseGrab()` dismisses the popup chain |
| Warp pointer | Not supported; return false |
| Raise, lower, set focus, always on top | `xdg_activation_v1` where available, otherwise ignored |
| Window shape (`FXWindow` mask) | `wl_surface.set_input_region` and opaque region; shaped drawing is clipped by the rasterizer |
| Cursors, custom cursor images | `cursor-shape-v1` for stock cursors, `wl_cursor` theme or a `wl_shm` cursor surface for images |
| Clipboard, primary selection, XDND | `wl_data_device`, `primary-selection`; the DDE members of `FXApp` map to data offers and sources |
| Input method (XIM) | `text-input-v3`; `FXComposeContext_wayland.cpp` |
| Monitors, DPI | `wl_output` and `xdg-output`; per-surface `preferred_buffer_scale` / fractional scale |
| Drag corner resize | `xdg_toplevel.resize` with the corner edge |
| Key repeat | Compositor sends rate and delay only; the module repeats on a FOX timer |
| Screen size, root window | Union of `wl_output` rectangles, `FXRootWindow` is a stub |

## Phases

Every phase ends with the project gates: the Windows build
(`scripts/build-fox2-mingw.ps1 -CMake -Fast -Run`) and the WSL X11 build
(`scripts/build-fox2-linux.sh --cmake --fast --run`) still pass with unchanged
output, plus the Wayland gate where it exists. One commit per step, one doc
section per step, FOX formatting, no reformatting of moved code.

### Phase 0: build selection and module classification

- Status: all done except the gate scaffold (the last bullet but one).
- Define `FOX_X11`, `FOX_WAYLAND`, `WIN32` from the build system (`fxtarget.h`
  checks that exactly one is set and defaults to `FOX_X11` on Unix); replace the
  `#if !defined(WIN32)` guards in the `_x11.cpp` modules with
  `#if defined(FOX_X11)`.
- Rename the POSIX modules to `_posix.cpp` with guard `#if !defined(WIN32)` so
  X11 and Wayland share them. The 39 modules were split into 16 `_posix`, 5
  `_unix` (`FXCheckButton`, `FXSlider`, `FXRegistry`, `FXDesktopSettings`,
  `FXShell`; guard `#if defined(FOX_X11) || defined(FOX_WAYLAND)`) and 18
  X-specific `_x11` modules.
- CMake option `FOX_PLATFORM` and Autotools `--with-wayland` (both fail with a
  clear message until phase 3, where the Wayland platform starts to build;
  from phase 4 on it builds the applications as well); module lists in
  `fox2/lib/Makefile.am` and
  `fox2/lib/CMakeLists.txt` split into common, `win32`, `posix`, `unix` and
  `x11` lists (the `wayland` list is added with the first Wayland module).
- `FXRawEvent` selection is in one place in `fxdefs.h`; the Wayland branch is a
  forward declared `FXWaylandEvent` that phase 4 defines as a small private
  tagged struct.
- Wayland gate scaffold: `scripts/build-fox2-wayland.sh`, and a check that the
  WSL distro runs a Wayland client against WSLg's Weston
  (`$XDG_RUNTIME_DIR/wayland-0`). The KDE Wayland session of the development
  machine is the second target.
- Acceptance: the two existing builds behave exactly as before; a
  `FOX_PLATFORM=wayland` configure fails with a clear message until phase 4.

### Phase 1: software rasterizer (platform neutral)

- Status: the rasterizer, the portable region, the scene test and the pixel
  gate are done; text and the blits that need `FXBitmap` are still open (text
  belongs to phase 2, the `FXRegion` wrapper around the portable region to
  phase 3).
- Private header `fxraster.h` and `FXRaster.cpp`: ARGB8888 target with stride,
  clip region, foreground and background, line width and cap, dashes, fill
  style (solid, tile, stipple, opaque stipple), function codes
  (`BLT_SRC`, `BLT_XOR`, ...), and the primitives of `FXDC`: points, lines,
  rectangles, round rectangles, arcs, chords, ellipses, polygons (convex,
  concave, complex), gradients, hash box, focus rectangle, image, icon and
  bitmap blits.
- Portable `FXRegion` implementation on a rectangle list (also needed by phase
  3 and by the browser frontend).
- Test program `tests/raster`: draws a fixed scene to a buffer, writes a PPM,
  runs on all three hosts. Reference images come from the existing X11 build
  (`screenshots/tests/fox2/x11/`). Acceptance: lines, rectangles, fills and
  tiles pixel-identical to X11 for the classic theme; arcs and polygons within
  a stated tolerance (X11 and the rasterizer differ in end-point rules, so each
  deviation is documented, not hidden).

Done in this phase so far:

- `FXRaster` is part of the library (common source list in
  `fox2/lib/Makefile.am`, which the CMake build reads as well). Every pixel
  written is opaque `0xAARRGGBB`, which is a valid premultiplied ARGB8888
  buffer for `wl_shm`.
- The 17 dither and 6 hatch bitmaps moved out of `FXApp_x11.cpp` into the
  private header `fxstipples.h`, shared by the X11 module and the rasterizer;
  the bitmaps were compared byte for byte with the originals.
- `tests/raster` draws a 4x8 cell scene, one primitive per cell, with
  `FXRaster` and — on the X11 build and with a display — with `FXDCWindow`
  into an `FXImage`, then compares both buffers cell by cell, writes
  `raster.ppm`, `x11.ppm` and `diff.ppm`, and exits non-zero when a cell is
  outside its limit. `--raster-only` needs no display, so the same scene runs
  on Windows and headless.
- Gate: `scripts/build-fox2-linux.sh --cmake` then
  `scripts/run-fox2-raster.sh` (add `--headless` to skip the X11 side).
- Portable region: the private header `fxrects.h` and `FXRects.cpp` hold a
  region as a canonical list of horizontal spans (sorted by scan line and
  start, spans on a row disjoint and non-touching), with union, intersection,
  difference, exclusive or, offset, bounds, point and rectangle queries and
  polygon construction with both fill rules. No display is involved, so phase
  3 wraps it in `FXRegion_wayland.cpp` and the browser frontend can use it as
  it is.
- `tests/region` checks `FXRects` against a brute force pixel model on a 48x48
  grid: every operation is applied to both and every pixel is compared, plus
  the bounding boxes, a rectangle query, the canonical form (the same pixel set
  built two ways compares equal) and 200 pseudo random operations.
- The same scene run under Wine (`raster.exe --raster-only`) matches the Linux
  one pixel for pixel outside the two text cells, which stay empty on Win32
  because that build has no FreeType. The Win32 platform has no comparison path
  yet; the test reports `no drawable platform in this build` there.
- `scripts/build-fox2-wayland.sh --check` reports the build prerequisites
  (Wayland client library, `libxkbcommon`, `wayland-scanner`, compositor
  socket).

Measured against the X11 build on 2026-11-13 (CachyOS, 64x64 cells, 32 cells):
1775 of 131072 pixels differ, 1.35%, all of them in the cells below.

| cell | differing | of | reason |
| --- | --- | --- | --- |
| fill, tiled, stipple, opaque, points, line-hv, line-diag, rect | 0 | 4096 | identical |
| gradv, gradh, hashbox, focus, image, stretch, area, clip | 0 | 4096 | identical |
| lines | 32 | 4096 | one pixel per join |
| dash | 33 | 4096 | dash phase over a polyline |
| width | 52 | 4096 | wide line is a filled quad, not the X11 stroker |
| roundrect | 58 | 4096 | the four corner arcs |
| fillround | 23 | 4096 | corner arcs and the three rectangles |
| arc | 99 | 4096 | analytic sampling vs the X11 midpoint rasterizer |
| fillarc | 20 | 4096 | the arc boundary only |
| chord | 59 | 4096 | the chord line of the boundary |
| ellipse | 154 | 4096 | same rule as `arc`, full outline |
| fillellipse | 63 | 4096 | the boundary only |
| polygon, concave | 40 | 4096 | centre rule vs the X11 shape rule |
| complex, winding | 66 | 4096 | same, with the even-odd and winding rules |
| text | 406 | 4096 | FreeType coverage vs XRender component alpha |
| imagetext | 564 | 4096 | same, including the background rectangle |

Deviations, all on the one pixel outline unless noted:

- Arcs and ellipses follow the analytic ellipse (a polyline sampled from the
  exact curve), not the X11 integer midpoint algorithm, so some outline pixels
  fall on the other side of the curve. The ellipse cell is the worst case with
  154 of 4096 pixels (3.8%); no one pixel shift of the raster buffer reduces
  the difference, so it is the rounding rule and not an offset.
- Wide lines are filled quads; dashed wide lines are stamped along the centre
  line, whereas the X11 server clips the whole stroke to the dash pattern.
- Polygon fills cross the scan line at the pixel centres, the X11 shape rule
  includes a slightly different set of edge pixels.
- Text is composited from the FreeType coverage bitmap at full colour; Xft goes
  through XRender with component alpha, so the antialiased edge pixels carry
  slightly different values while the glyph shapes, their positions and every
  advance agree exactly (406 and 564 of 4096 pixels, all on glyph edges, and no
  delta in the metrics - see `tests/glyphs`).
- `fillVerticalGradient` and `fillHorizontalGradient` restore the previous
  foreground colour; the X11 module leaves the last gradient colour set, which
  is not reproduced.
- The limits in `tests/raster` are the measured numbers plus a small margin, so
  a regression fails the gate while the documented deviations pass.
- `drawBitmap`, `drawIcon`, clip masks and bitmap stipples are implemented but
  not covered by the scene yet: `FXBitmap` has no implementation in `fox2`, so
  the test uses an image blit where a bitmap blit is planned.


### Phase 2: fonts without Xft

- `FXFont_wayland.cpp` using fontconfig for matching and enumeration and
  FreeType for outlines, hinting and glyph bitmaps; glyph cache keyed by font
  and DPI; UTF-8 text extents, ascent, descent, width of a string.
- Reuse the structure of the Xft path in `FXFont_x11.cpp` (Xft is a thin layer
  over the same libraries); the translation of the font description (family,
  weight, slant, size, encoding) must produce the same fontconfig pattern.
- `FXRaster` text drawing uses the glyph cache; `drawText` and `drawImageText`
  match the X11 baseline and advance rules.
- Acceptance: `tests/raster` text scene has the same extents as the X11 build
  for the same font spec (metrics equal; pixels may differ by hinting and
  subpixel settings, which are read from fontconfig the way Xft does).

Done in this phase so far (the glyph cache and rasterizer text; the
`FXFont_wayland.cpp` wrapper belongs to the Wayland build of phase 4):

- `fxglyphs.h` and `FXGlyphs.cpp`: fontconfig matches the font with the same
  pattern the Xft path in `FXFont_x11.cpp` builds, FreeType loads the file that
  match names and sizes it the way Xft sizes it. The cache holds, per character,
  the advance, the left bearing, the bitmap position and the 8-bit coverage; it
  also reports ascent, descent, height and the widest advance, and measures UTF-8
  strings and finds the character at a pixel position.
- FreeType and fontconfig are optional: the build defines `HAVE_XFT_H` when it
  finds them, which is also what the Xft path needs. Without them the class
  keeps its interface and `open()` fails, so the Win32 build still links and
  simply has no text.
- `FXRaster::drawText` composites the coverage bitmap with the foreground
  colour, honouring the clip rectangle, the clip region and the clip mask;
  `drawImageText` fills the text box with the background first, as
  `XftDrawRect` does before the string.
- `tests/glyphs` compares the cache with `FXFont` (Xft) for the same
  description at the same resolution. On 2026-11-13 with "sans" 10 pt at
  100 dpi (Noto Sans) the deltas are 0 for ascent, descent, height, max advance
  and for the width of six strings, so the layout will match the X11 build.
- The two text cells of `tests/raster` show the same glyphs at the same
  positions with the same advances; only the antialiased edge pixels differ,
  because Xft composites through XRender and the rasterizer blends coverage
  itself.


### Phase 3: resources

- `FXVisual_wayland.cpp`: fixed truecolor, no colormap, no palette.
- `FXImage_wayland.cpp` and `FXIcon_wayland.cpp`: the server-side image is the
  client buffer; `render()` converts `FXColor` to premultiplied ARGB.
- `FXRegion_wayland.cpp`, `FXCursor_wayland.cpp` (stock cursor ids and image
  cursors, realized in phase 6), `FXStyle_wayland.cpp`.
- `FXDCWindow_wayland.cpp`: a thin shell over `FXRaster`, with the Win32
  members and `xftDraw` left unused (declared since seam 60).
- Offscreen test: `FXImage` drawn with `FXDCWindow` into a buffer, written as
  PPM, compared with `tests/raster`.
- Acceptance: widgets drawn into a standalone buffer (no compositor) look like
  the X11 screenshot for `hello` and `button`.

Done in this phase so far:

- `FOX_PLATFORM=wayland` configures and builds the library. The build defines
  `FOX_WAYLAND`, requires `wayland-client` and `xkbcommon`, probes FreeType and
  fontconfig for the glyph cache (`FOX_HAVE_FREETYPE`) and reports
  `wayland-scanner` and `wayland-protocols` for phase 4. Protocols, codecs and
  the POSIX modules are selected exactly as for X11.
- `FXRegion_wayland.cpp`: the region is `FXRects` behind the opaque handle, so
  the platform, the rasterizer and the browser frontend share one definition.
- `FXVisual_wayland.cpp`: one fixed 32 bit truecolor format, no colormap and no
  palette; `getPixel()` and `getColor()` are the identity on the color with an
  opaque alpha, because a Wayland pixel is already premultiplied ARGB.
- `FXImage_wayland.cpp` and `FXIcon_wayland.cpp`: the client buffer is the
  image, so `render()` and `restore()` have nothing to move and `create()`
  only sets up the visual and makes sure there is a buffer. Icons need no
  shape or etch pixmap; the rasterizer takes the shape from the alpha channel
  or the transparent color.
- `FXDCWindow_wayland.cpp`: the DC carries an `FXRaster` in its context handle
  and copies the `FXDC` state into it from every setter; all primitives, the
  text, the gradients and the clipping go straight to the rasterizer.
  `clipChildren()` and the window buffer need the virtual window table of
  phase 4; only images can be drawn into so far.
- `FXFont::getGlyphCache()` is a neutral accessor on the font (nullptr on X11
  and Win32, where the platform draws text itself) so the Wayland DC can hand
  the glyph cache to the rasterizer without knowing the font internals.
- `FXFont_wayland.cpp`: matching, sizing and every metric come from the glyph
  cache, with the numbers the Xft path reports (leading 0, spacing is ascent
  plus descent, bearings from the glyph bitmap, mono detection by comparing the
  advances of "i" and "M"). `listFonts()` returns false until fontconfig
  enumeration is added.
- `FXStyle_wayland.cpp` keeps the platform part of the style (corner mask cache
  and the XRender probe) unused: the classic theme's rounded rectangles come
  from `FXRaster`'s arcs, so `drawRoundRect()` reports false and the shared
  theme code takes its plain path. `FXCursor_wayland.cpp` only marks the cursor
  as created; the pointer surfaces are phase 6 work.
- `scripts/build-fox2-wayland.sh` builds the library target only until phase 4
  (`FOX_WAYLAND_TARGET=all` builds everything once FXApp exists).
- Measured on 2026-11-13: the library builds (2.3 MB) and the remaining
  undefined platform symbols went from 155 to 96 — they are `FXApp`,
  `FXWindow`, `FXShell`, `FXTopWindow`, `FXMainWindow`, `FXPopup`,
  `FXRootWindow`, `FXToolTip`, `FXDragCorner`, `FXComposeContext` and the
  `fxplatform` hooks. That list is the work of phase 4, together with the
  virtual window table; the X11 build, the Windows cross build and the pixel,
  region and glyph gates are unchanged.

### Phase 4: connection, event loop and virtual windows

- `FXApp_wayland.cpp`: connect (`wl_display_connect`), registry, bind globals,
  `xdg_wm_base` ping/pong, `wl_output`, `wl_shm` formats, close.
- Event loop: integrate with the existing `select`/timer/input-handle loop
  (`wl_display_prepare_read`, `wl_display_flush`, `wl_display_read_events`,
  `wl_display_dispatch_pending`); the display fd is one more input handle.
  Timers, idle, chores and signals stay in `FXApp.cpp`.
- Native event queue: the module pushes neutral raw events (phase 0 struct) and
  `dispatchEvent()` translates them like the X11 and Win32 modules do.
- `FXWindow_wayland.cpp`: virtual window table described in decision 2, buffer
  management (double or triple buffered `wl_shm` pool, `wl_buffer.release`),
  damage tracking (`wl_surface.damage_buffer`) from the repaint list,
  `wl_surface.frame` pacing.
- `FXShell`/`FXTopWindow`/`FXMainWindow`: a plain `xdg_toplevel` with title,
  app id (from `FXApp` name) and minimum/maximum size.
- Wayland gate: `hello` opens, draws the button, closes. A debug option
  (`FOX_WAYLAND_DUMP=<dir>`) writes every committed buffer as PPM so tests do
  not depend on compositor screenshots (Wayland restricts capture, which the
  existing plans already note).
- Acceptance: `hello` and `button` show on WSLg and on KDE, and the dumped
  buffer matches the X11 screenshot.

Done in this phase so far (the protocol files and the platform seam):

- The ten protocol XML files are vendored under `fox2/protocols/` with their
  licence headers and a `README` that names the wayland-protocols release they
  came from; a build needs `wayland-scanner`, not the package.
- The CMake build generates the client glue for all ten into the build
  directory (`client-header` and `private-code`) and compiles it into the
  library, so the protocol side of the connection is ready before the
  connection itself exists. The generated code is C, so the Wayland
  configuration enables the C language.
- `fxplatform_wayland.cpp`: the seam hooks answer for the platform — no
  process DPI awareness, no system DPI (the compositor reports a scale per
  output in phase 9), `WAYLAND_DISPLAY` as the display name, "Sans,90" as the
  default font and no configured scale.
- `FXApp_wayland.cpp`: `wl_display_connect`, the registry walk that binds
  `wl_compositor`, `wl_shm`, `wl_seat`, `wl_output` and `xdg_wm_base`, the
  `shm` format probe (ARGB8888 is remembered; its absence only warns, since an
  opaque buffer still works) and the ping/pong. `getNextEvent()` keeps the
  timer, signal, chore, refresh, repaint and input-fd handling of the X11
  module and adds the connection as one more input, with `wl_display_pending`
  and `wl_display_dispatch` filling a queue of neutral `FXWaylandEvent`s that
  `dispatchEvent()` translates. A compositor without `wl_compositor`, `wl_shm`
  or `xdg_wm_base` is reported by name and the connection is refused; a missing
  compositor only warns, so a test can still run the library headless. The
  selection, clipboard and drag-and-drop entry points exist as empty stubs
  until phase 8.
- The library builds with 71 undefined platform symbols left: the windows of
  this phase (`FXWindow`, `FXRootWindow`, `FXShell`, `FXTopWindow`,
  `FXMainWindow`, `FXPopup`, `FXToolTip`), the `fxplatformCreateWindow` family,
  `FXDragCorner` and `FXComposeContext`.

### Phase 5: window management

- `xdg_surface.configure`/`ack_configure` handling, resize (`FXTopWindow` size
  from the compositor, `layout()`, new buffer), maximize, fullscreen, minimize,
  `xdg_toplevel.close`, `wm_capabilities`, `configure_bounds`.
- Decorations: `xdg-decoration`, with the client-side fallback (title, close,
  maximize, minimize, resize edges, drop shadow optional) in
  `FXTopWindow_wayland.cpp`.
- `FXRootWindow_wayland.cpp`: size from `wl_output`/`xdg-output`.
- `FXDragCorner_wayland.cpp`: `xdg_toplevel.resize`.
- Acceptance: resize, maximize and close behave on KDE (server-side
  decorations) and WSLg/Weston (client-side decorations).

### Phase 6: input

- `wl_pointer`: enter, leave, motion, buttons, axis (vertical and horizontal
  wheel, discrete steps), frame; coordinates are the surface-local logical
  position scaled to buffer pixels; click count and double-click timing remain
  in `FXApp`.
- `wl_keyboard` with `xkbcommon`: keymap from the compositor fd, modifiers,
  keysym to FOX key code (identity), compose through `xkb_compose` for dead
  keys until `text-input-v3`, repeat via FOX timer.
- `wl_touch`: single touch mapped to the left button (multi-touch later).
- Cursors: `cursor-shape-v1` where present, else `wl_cursor` theme and a cursor
  surface; hidden cursor, image cursors from `FXCursor`.
- `FXApp::getKeyState()`, `getCursorPos()`, `findWindowAt()`, `beep()` answered
  from the tracked seat state (no synchronous queries exist in Wayland).
- Acceptance: all of `tests/button` is operable by pointer and keyboard.

### Phase 7: popups, menus, tooltips

- `FXPopup`/`FXMenuPane`: `xdg_popup` parented to the owner shell, anchor
  rectangle computed from the requested absolute position relative to the owner,
  `xdg_positioner` constraint adjustment (slide, flip), explicit grab taken in
  the event that opened it, `popup_done` mapped to the FOX popdown.
- Nested menus: nested popups with a parent chain; the module destroys them in
  reverse order as the protocol demands.
- `FXToolTip_wayland.cpp`: popup without grab or a subsurface following the
  pointer; no parent grab.
- Acceptance: menu bar, nested menus, option menus, tooltips and combo boxes
  open at the right place and close correctly on outside click and Escape.

### Phase 8: clipboard, selection, drag and drop

- `wl_data_device`: clipboard via data source/offer; `primary-selection` for
  the X-style primary selection; lazy data (the offer is read when the peer
  asks, through a pipe handled as an input handle).
- Map the DDE/selection members of `FXApp` (`ddeTypeList`, `xselTypeList`)
  and the `FXDragType` registration to MIME types; keep the FOX drag types
  `FXWindow::textType`, `urilistType` and so on mapped to `text/plain;charset=utf-8`,
  `text/uri-list`.
- Drag and drop between FOX windows and with other applications through
  `wl_data_device.start_drag` and the drag surface; drop actions mapped to
  `DRAG_COPY/MOVE/LINK/ASK`.
- Acceptance: copy and paste of text between FOX and a non-FOX Wayland client,
  and a drag of a file list in both directions.

### Phase 9: input method, scale, settings, polish

- `FXComposeContext_wayland.cpp` on `zwp_text_input_v3` (preedit, commit,
  cursor rectangle).
- Fractional scale and `wp_viewporter`; per-output scale; rescale on output
  change through the existing `WM_DPICHANGED`-style path (`applyWindowDPI`).
- `FXDesktopSettings_wayland.cpp`: colour scheme, font and scale from
  `org.freedesktop.portal.Settings` (the portal work is planned in
  `docs/theme-plan.md`; reuse, do not duplicate).
- Idle inhibition, activation (`xdg_activation_v1`), `xdg_toplevel_icon_v1`
  from the application icon.
- Acceptance: `tests/dpi` reports the compositor scale and the window changes
  scale when moved between outputs with different scales.

### Phase 10: packaging and documentation

- Autotools and CMake wiring complete, `fox-config`/pkg-config flags for the
  Wayland build, install of nothing extra.
- Document the build in `scripts/build-fox2-wayland.sh` and in WSL notes in the
  style of `docs/windows-mingw-plan.md`, and add the Wayland module table to
  `docs/platform-seam-plan.md`.
- Screenshots stored under `screenshots/tests/fox2/wayland/` as the rules ask.

## Verification

- **Compile gates:** Windows, WSL X11 and, from phase 4, WSL Wayland must all
  build; each step must leave the other two outputs unchanged.
- **Raster gate (phase 1):** `./scripts/run-fox2-raster.sh` draws the scene with
  `FXRaster` and with `FXDCWindow` and compares the buffers; it fails when a
  cell exceeds the limit recorded in the phase 1 table. The scene images are
  kept under `screenshots/tests/fox2/x11/raster.png` (X11) and
  `screenshots/tests/fox2/raster/raster.png`.
- **Region gate (phase 1):** `tests/region` needs no display and runs on every
  host; it fails when the span algebra differs from the pixel model.
- **Text gate (phase 2):** `tests/glyphs` runs on every host; on X11 it compares
  the metrics and the string widths of the glyph cache with Xft and fails on a
  difference of more than one pixel.
- **Pixel gate:** phases 1 to 4 compare dumped buffers with X11 screenshots of
  the same widget tree and theme; each difference is either fixed or recorded
  with a reason.
- **Compositors:** WSLg/Weston (client-side decorations, no `xdg-decoration`)
  and KWin on the development machine (server-side decorations, all optional
  protocols). For scripted runs `scripts/run-fox2-wayland-headless.sh` starts a
  private headless Weston (15.0.1 offers `xdg_wm_base` 5 and `wl_shm` with AR24
  and XR24) in its own runtime directory and runs a command against it.
- **In-process observation** (the approach already used for the theme tests)
  replaces desktop screenshots wherever the compositor blocks capture.
- **Run-time failure behaviour:** missing optional protocols are detected at
  bind time and disable only the feature; a missing required protocol aborts
  `FXApp::init()` with a message that names it.

## Risks

- **Virtual child windows (phase 4)** are the largest structural risk. The
  hit-test, clip and expose logic must match what X11 gives for free
  (`ClipChildren`, stacking, `Expose` ordering). A spike that renders the
  `button` window tree through the table should be the first step of phase 4.
- **Text parity (phase 2):** layout depends on font metrics, so metrics must
  match Xft before any widget-level comparison is meaningful.
- **Popup positioning (phase 7):** FOX computes absolute screen coordinates;
  Wayland wants an anchor on the parent. Menus that open from an
  already-popped-up parent need the parent chain, and compositors differ in
  how they constrain popups that start outside the output.
- **Client-side decoration quality (phase 5)** on compositors without
  server-side decorations; the minimal bar must be usable but is not a theme
  engine.
- **Event model:** Wayland has no synchronous queries (pointer position,
  modifier state, window position). Code that polls these needs the tracked
  state described in phase 6; some callers may need a small adaptation.
- **Scope creep:** GL, session management and multi-touch are excluded until the
  widget set works.

## Decisions to confirm before phase 0

1. The macros are `FOX_X11` and `FOX_WAYLAND` and the platform is chosen at configure
   time (`FOX_PLATFORM`), with no combined X11 + Wayland binary.
2. Child windows are virtual and only shells own a `wl_surface`.
3. The rasterizer is written in-tree (no cairo or pixman); text uses FreeType
   and fontconfig directly.
4. Client-side decorations are a minimal in-tree bar, not `libdecor`.
5. The first compositor targets are WSLg/Weston and KWin.
