# Wayland surface plan

Plan for a native Wayland platform module for `fox2`: windows backed by
`xdg_toplevel`/`xdg_popup` surfaces, pixels delivered through `wl_shm`, input
through `wl_seat`. It builds on the platform seam (`docs/platform-seam-plan.md`,
seams 1 to 62) and is the first test of whether that seam is wide enough for a
third platform. The browser thin client in `docs/modernization-plan.md` is
deferred, but it is meant to share this presentation layer (phases 1 and 2
below) when it is resumed, so the layers stay free of platform types.

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
   `primary-selection-unstable-v1`, `text-input-unstable-v3`, `xdg-output`,
   `xdg-activation-v1`, `xdg-toplevel-icon-v1`, `xdg-dialog-v1`.

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
| Modal loop blocks its owner | The core filters events (motion, wheel, button, key, IME, close); an owned toplevel names its owner with `xdg_toplevel.set_parent` and marks the relationship with `xdg-dialog-v1`, so a compositor can also refuse the parent |
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
- Text drawn with `FXFont::setAngle()` is rotated here by mapping every glyph's
  coverage bitmap to its rotated position (nearest neighbour); Xft renders it
  through a FreeType transform with component alpha, so angled glyph edges
  differ by about a pixel.  The metrics of an angled font also differ: Xft
  returns 0 from `getFontHeight()`/`getTextWidth()`, the Wayland font keeps the
  unrotated metrics.  The chart's `Y` labels show both effects - the X11 frame
  overprints a label it measured as empty, the Wayland frame lays it out.
- A disabled button draws `drawIconSunken`, which the X11 module builds from
  the icon's etch bitmap (its visible, dark pixels).  Wayland has no such
  bitmap, so `FXRaster::drawIconSunken` derives the mask from the icon pixels
  and paints it in the highlight and shadow colours, one pixel apart, exactly
  like the X11 mask.  Without it, disabled toolbar buttons showed their full
  colour (first seen with the ported PathFinder).
- The DC clip rectangle in `FXDCWindow::begin()` follows the drawable (0,0 to
  its size); before that `getClipX()`/`getClipWidth()` returned the FXDC default
  of 32767, which made the chart widget measure its plot far too large.  Caught
  by the `chart` port (`fox2/apps/chart`).
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
- `scripts/build-fox2-wayland.sh` builds the library, the tools and every test
  program by default; `FOX_WAYLAND_TARGET=FOX` builds the library alone.
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
- File descriptor inputs: `getNextEvent()` polls `inputs[]` and the `handles`
  descriptor sets, but `FXApp::addInput()` / `removeInput()` had no Wayland
  implementation, so nothing could register a descriptor.  The ported Adie's
  `ShellCommand` needs them to watch the pipes of the child program; both are
  now implemented like the X11 module (grow `inputs`, set or clear the
  read/write/except set, maintain `maxhandle`).
- Virtual window table: the private header `fxwinmap.h` and `FXWinMap.cpp` keep
  the child windows Wayland does not have — synthetic id, parent, geometry
  relative to the parent, stacking order (children bottom to top, shells in a
  list of their own) and shown state — with create, destroy of a whole subtree,
  reparent (keeping the place on the screen and going to the top), raise and
  lower, point and rectangle hit-testing that descends in stacking order, and
  the shell of any window. Nothing in it needs a display, so the module that
  builds the surfaces and the browser frontend share it.
- `tests/winmap` checks the table against a brute force model on a small tree:
  nested hit tests, stacking after raise and lower, hidden windows and hidden
  parents, reparenting, the rejection of a reparent that would make a loop,
  subtree destruction, popup shells above a toplevel, and 200 random trees; all
  of it runs headless on every host.
- `fxwaylandpriv.h` moved the connection out of the application module, so the
  window modules share it: the bound globals, the event queue, the event codes
  of the neutral raw event, the virtual window table and the list of shell
  surfaces (surface, `xdg_surface`, `xdg_toplevel`, and the popup role later).
  `fxwayland.cpp` holds what both sides need: queueing an event, finding,
  creating and destroying a shell surface, and turning the shell's configure
  and close requests into neutral events.
- `fxplatformwindow_wayland.cpp` answers the window seam with the table: create
  adds the row and, for a shell, its `wl_surface`/`xdg_surface`/`xdg_toplevel`;
  destroy removes both; place, show, reparent and raise/lower only touch the
  table. Attaching a foreign window is not a Wayland concept and does nothing;
  the grabs are implicit in the protocols and come back in phase 6.
- The library keeps 62 undefined platform symbols instead of 71; what is left
  is `FXWindow` and the shell classes, `FXDragCorner` and `FXComposeContext`.
- The buffer path is in place: `fxwaylandAcquireBuffer()` hands out an idle
  ARGB8888 buffer of the requested size, creating it from a sealed shared
  memory file (`XDG_RUNTIME_DIR`, `mkstemp` plus `ftruncate`), with at most
  three buffers per surface and the pool rebuilt when the size changes;
  `wl_buffer.release` clears the busy flag. `fxwaylandCommitBuffer()` damages,
  attaches and commits, and `fxwaylandDumpBuffer()` writes the committed buffer
  as a PPM into `$FOX_WAYLAND_DUMP`, which is how the gate will compare against
  the X11 screenshots once the windows paint.
- Painting works as one frame per shell: `fxwaylandBindWindow()` finds the
  surface of the window's shell, acquires the frame buffer once, and answers
  with the pixel pointer of the window's place inside it, so a child widget
  draws into the same buffer as its toplevel at its own offset. The DC clips
  every draw to the window it draws for, and the rasterizer writes straight
  into the mapped buffer. When the repaint list of the application is empty,
  the event loop calls `fxwaylandFlushFrames()`, which damages, attaches and
  commits each frame and writes the dump; a flush of the connection makes sure
  the compositor sees the commit before the loop sleeps in `pselect`.
- Each shell keeps a **backing store** of its whole window tree, and painting
  goes into that store; a commit copies the store into whichever buffer the
  pool hands out and then commits it. This is not an optimisation but a
  correctness requirement: `wl_shm` buffers are recycled, and a recycled buffer
  either starts blank or holds the content of two frames ago, so incremental
  painting into buffers would lose every pixel that a frame did not touch. The
  first version did exactly that and the dump showed it — a mostly black frame
  wherever the pass had repainted only a small part.
- The window state of a toplevel (maximized, full screen, activated) is read
  from the `xdg_toplevel.configure` state array and kept on the surface, so the
  shell classes answer `isMaximized()` and friends from what the compositor
  said instead of guessing. The size of the first `wl_output` is recorded from
  its geometry and mode events and is what `FXRootWindow` reports.
- `FXWindow_wayland.cpp` and `FXRootWindow_wayland.cpp` complete the window
  base: the drag type indices (`stringType`, `utf16Type` are indices into the
  mime table of phase 8), coordinate translation through the window table (the
  root window has no row and sits at the origin), `update()` and `repaint()` on
  the shared repaint list, and the grabs and the enable/disable state. Shapes,
  cursor warping and pointer queries need the input region, the viewport and
  the seat and keep the behaviour of a platform without them.
- The shell classes are in: `FXTopWindow_wayland.cpp` drives the `xdg_toplevel`
  (title, application id, the size limits that follow from the `DECOR_*`
  options), hides a window by detaching its buffer, and asks for maximize,
  minimize and full screen with the toplevel requests; whether a window is
  maximized or full screen is read back from the configure state, and the
  minimized flag is the one request the protocol has no state for, so we keep
  it ourselves. `FXMainWindow_wayland.cpp`, `FXPopup_wayland.cpp`,
  `FXToolTip_wayland.cpp`, `FXDragCorner_wayland.cpp` and
  `FXComposeContext_wayland.cpp` carry the placement rules of the X11 module
  over, minus what only the input and popup phases can provide.
- Windows are painted because they are exposed, not because they are asked to:
  `fxplatformShowWindow()` and the configure of a shell post exposures for the
  whole tree below them (`fxwaylandExpose()`), and a shell that the compositor
  has not configured yet is not painted at all, which is why the first frame is
  only committed after the first configure. The root window gets an id of its
  own even though it has no row in the table, and a shell with a parent that is
  not in the table hangs off the root.
- `FOX_WAYLAND_TRACE=1` prints what the module does — connect, create window,
  create surface, configure, expose, bind, new buffer, flush — because a
  Wayland client has no other way to say what it did and the compositor log
  does not show it.  Each line carries a monotonic timestamp, so a delay such
  as a resize that lags behind the pointer can be read off the log.
- Frames are flushed as soon as the paint queue drains, before more input is
  read, and before the layout of a configure starts: the frame painted for the
  previous size goes to the compositor even when the next configure is already
  queued.  Flushing only when the poll found nothing was enough for a static
  window, but a continuous stream of events (a live resize, a pointer moving
  over the window) kept the queue busy and the compositor stayed on an old
  frame until the stream paused, which is the lag at the start of a drag.
- The library now has **no** undefined platform symbols, and `hello` runs
  against the headless compositor: it creates its toplevel, gets configured,
  paints the button and commits an 83x26 frame, which `FOX_WAYLAND_DUMP` writes
  as `window-2-83x26.ppm`. The frame has the same size as the X11 screenshot
  and the same text and focus rectangle in the same place; the colours differ
  because the palette comes from the desktop settings, which are not the same
  in a bare headless session, not because of the painting.

### Phase 5: window management

Done in this phase so far:

- The compositor's configure sizes the window: the size from the toplevel
  configure is turned into a `SEL_CONFIGURE` for the shell, the layout resizes
  the window tree, the virtual window table follows, and the frame buffer is
  rebuilt on the next bind. Under a tiling compositor the window now takes the
  size it is given instead of the size it asked for.
- Decorations: `zxdg_decoration_manager_v1` is bound when the compositor
  offers it and every toplevel asks for the server side mode; KWin's virtual
  backend answers `server side`, and the trace says so. Where the compositor
  offers nothing — or answers client side — the client draws a **minimal title
  bar** of its own: font height plus a small margin, the title on the left and
  close, maximize and minimize on the right, painted by the rasterizer into the
  store of the shell before every commit. The bar is part of the surface, so the
  frame is the window plus the bar, the whole window tree is pushed down by it,
  the pointer path subtracts it again, and a press inside the bar moves the
  window (`xdg_toplevel.move`) or uses one of its buttons instead of reaching a
  widget. The dumped file name carries the bar height (`window-2-83x52-bar26.ppm`)
  so a test knows where the client area starts.
- The drag corner asks the compositor for an interactive resize
  (`xdg_toplevel.resize` with the seat and the serial of the last pointer
  enter) and only falls back to tracking the drag itself when a compositor
  gives no serial.
- `xdg_toplevel.close` becomes `SEL_CLOSE`, which is what the title bar close
  button and the window manager send.
- A window that moves or resizes has to repaint the parent it leaves behind.
  On X11 the server exposes the uncovered area; here every window shares one
  buffer, so nobody does.  `fxplatformPlaceWindow()` therefore posts an
  exposure for the parent and its whole subtree, parents before children, when
  the place of a visible child really changes, and `fxplatformShowWindow()` does
  the same for the parent when a window is unmapped.  Without it a widget that
  moves keeps a ghost of itself: pressing *After Text* or *Centered* in
  `tests/button` left the old button and the old radio buttons in the frame
  (`screenshots/tests/fox2/wayland/button-iconpos-stale.png`), the same frame is
  clean with the exposure (`button-iconpos.png`).

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

Done in this phase so far (the seat, the pointer, the keyboard and the two
gates that drive them):

- `fxseat_wayland.cpp` binds the seat's pointer and keyboard, tracks the
  surface under the pointer and the surface with the keyboard focus, and turns
  every protocol event into one neutral event: enter, leave, motion, button,
  wheel, key, focus in, focus out.  Hit-testing adds the place of the shell in
  the window table before it descends, because the pointer arrives in surface
  coordinates and the table speaks screen coordinates.
- `wl_pointer.button` carries **Linux event codes**, not the 1, 2, 3 of X11:
  272 is the left button, 273 the right one and 274 the middle one.  The first
  version passed them on unchanged, so every press arrived as button 272, FOX
  matched no `SEL_LEFTBUTTONPRESS` and no widget ever reacted — the trace said
  `button 272 down to FXButton` while nothing happened.  The module now
  translates the code and the button mask together.
- The keyboard uses the keymap the compositor sends, `xkbcommon` gives the
  keysym (which is already a FOX key code) and the text, and the modifiers are
  updated before the core sees the event.  A pointer enter remembers the serial
  the popup grab and the interactive resize need.
- A shell focuses its first focusable widget when the keyboard enters, and a
  keyboard leave raises no `SEL_FOCUSOUT`, because the compositor reports the
  leave of one surface twice, which would take the focus away between the press
  and the release of a key.
- Grabs are not a Wayland concept: `FXWindow::grab()` only sets
  `mouseGrabWindow`, and because the press in the gate is followed by the
  release on the same widget, the button still turns the pair into a command.
- Click counting stays in `FXApp`: `click_time`, `click_button`, `click_count`
  and `moved` live in its persistent `event` member and must survive from one
  button or motion event to the next.  The dispatcher uses that member, like
  the X11 and Win32 modules; replacing it with a fresh `FXEvent` per event
  resets the counter and breaks double clicks.

- `wl_pointer`: enter, leave, motion, buttons, axis (vertical and horizontal
  wheel, discrete steps), frame; coordinates are the surface-local logical
  position scaled to buffer pixels; click count and double-click timing remain
  in `FXApp`.  The vertical axis is positive towards the end of the document
  while a FOX wheel code is positive towards the top, so the seat flips the
  sign before posting `FXXWL_WHEEL`.
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

Done in this phase so far (the popup role, menus with a submenu and tool tips):

- A popup gets its `xdg_popup` when it is **shown**, not when it is created: the
  positioner fixes the size once and a popup cannot grow afterwards, so every
  show starts from a fresh surface.  The anchor rectangle is the place the core
  asked for, relative to the surface of the owner, with `SLIDE_X`, `SLIDE_Y`,
  `FLIP_X` and `FLIP_Y` so a menu near the edge stays on the output.
- The grab is taken with the serial of the **press** that opened the popup, not
  with the pointer enter of long ago; a stale serial is what a compositor may
  refuse, and the grab is requested before the first commit because the protocol
  calls grabbing a mapped popup an error.
- Menus nest: the submenu is a popup whose parent is the popup, and the protocol
  is strict about that chain — a grabbing popup may only have a toplevel or
  another grabbing popup as its parent, and only the topmost popup of a chain may
  be destroyed.  `fxwaylandDestroySurface()` therefore posts `popup_done` for the
  popups hanging off a popup and destroys them first, top down.
- A tool tip is a popup that must **not** grab: it appears while the pointer is
  only resting, and a grab would eat the click meant for the window below.  FOX
  makes a tool tip an override redirect shell without an owner, so the module
  decides from the class (an `FXPopup` grabs, an `FXToolTip` does not) and hangs
  an ownerless popup off the shell the pointer is over.
- A popup hangs off the surface of the window it **belongs to**, not off the
  surface under the pointer: the parent in the virtual window table is the shell
  of a menu, or the parent popup of a submenu, and that is the surface the
  compositor anchors it to.  The pointer surface is only the fallback for a tool
  tip, which has no parent surface.  Taking the pointer for every popup posted a
  menu from a second window (or from the keyboard) under whatever surface the
  pointer happened to be over, so the menu appeared at that surface's top-left
  instead of under its own menu title.
- FOX itself dismisses a menu on a press outside it through the grab: with the
  popup as grab owner, our dispatch sends the press to the popup, which unposts.
  The compositor's own dismissal arrives as `popup_done` and becomes the FOX
  popdown, so both paths end in the same place.
- Hit testing and `SEL_ENTER`/`SEL_LEAVE` now follow the pointer from widget to
  widget, not only per surface: the compositor never says which widget the
  pointer left, so the module compares the window under the pointer with the one
  before it.  Without that a menu entry never became *active*, and a press on it
  did nothing; the tool tip flag is set on the same path, because
  `FXWindow::onEnter` needs the crossing mode (`CROSSINGNORMAL`, not the stale
  code of the previous event).
- The menu widgets were missing from the port: `FXMenuCaption`, `FXMenuCommand`,
  `FXMenuCascade`, `FXMenuCheck`, `FXMenuRadio`, `FXMenuSeparator`, `FXMenuTitle`
  and the menu bar came over from the original tree unchanged.  `FXMenuBar`,
  `FXMenuButton` and `FXOptionMenu` stay out for now: they inherit `FXToolBar` and
  with it the docking family (`FXDockBar`, `FXDockSite`, `FXToolBarGrip`,
  `FXToolBarShell`, `FXToolBarTab`), about 4,900 lines that the library does not
  have yet.  Menus opened from a plain button work, a menu bar waits for that
  port.
- Two old bugs of the window base came out of making menus land in the right
  place.  `FXRootWindow::create()` never took the size of the output, so the root
  window stayed 1x1 and every window FOX centers landed at a negative place; and
  `translateCoordinatesTo()` **subtracted** the target place instead of adding
  it, which put every root coordinate at the mirror image of where it belongs.
  Both were invisible until a menu computed its anchor from them.
- Gates: `scripts/gate-wayland-menu.sh` runs `tests/menus` (a menu with a
  submenu, opened from a button) and checks the grab, the nesting and both
  dismissal paths — open and click outside must end in `popup_done` with the
  application still running, open, submenu and pick the third entry must end in
  `command three` and status 0.  `scripts/gate-wayland-tooltip.sh` rests the
  virtual pointer on a button and checks that the tool tip becomes a popup
  *without* grab.  The frames are kept as `menu.png`, `menu-nested.png` and
  `tooltip.png` under `screenshots/tests/fox2/wayland/`.
- The tests report what they do on stdout, which is what the gates assert on;
  the pointer has to stay alive during a wait (the injector holds the virtual
  pointer with `sleep:`), because a compositor that loses the device sends the
  pointer leave and cancels the tool tip.

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
  Escape is not covered yet: the gate has no keyboard focus on the popup, and
  the menu bar, option menu and combo box wait for the docking and list widgets.

### Phase 8: clipboard, selection, drag and drop

Done in this phase (the clipboard, the primary selection and drag and drop):

- `fxclipboard_wayland.cpp` binds `wl_data_device_manager` (core protocol, no XML
  needed) and `zwp_primary_selection_device_manager_v1` (vendored already), makes
  a data device on the seat as soon as manager and seat both exist, and keeps
  what the clipboard offers: the offer of a peer, its mime types, and the drag
  type each of them maps to.
- A Wayland drag type is the index of a mime type in a table of the connection,
  because `FXDragType` has to be a number.  The nine standard FOX types are
  registered at `openDisplay()` in the order that keeps `FXWindow::stringType`
  and `FXWindow::utf16Type` at their fixed values, and `registerDragType()` adds
  the rest.  Incoming mime types collapse onto them where the spelling differs
  (`UTF8_STRING` and `text/plain;charset=utf-8` are one type to FOX), and the
  mime type that came with the offer is kept next to the type: a peer has to be
  asked for the name *it* offered, not for our canonical one.
- Copying is not "put the text in the board": the client creates a
  `wl_data_source`, offers the mime types, calls `set_selection()` with the
  serial of the last input event, and then **answers** peers that ask for a mime
  type by writing it into a pipe.  The text is asked from the application at that
  moment, through `SEL_CLIPBOARD_REQUEST`, so a widget that changes its data does
  not have to tell the clipboard about it.
- Pasting asks the offer of the peer for a mime type and reads the pipe.  It is a
  synchronous read with a two second limit, because the answer comes from another
  process and there is no way to ask whether it will come at all.
- The compositor hands the clipboard to somebody else through `cancelled` on our
  source; that raises `SEL_CLIPBOARD_LOST` and clears the owner, so a paste after
  a peer took over reads the peer's data and not ours.  Without it the
  application kept serving its own copy, which is what the gate showed first.
- The X11 primary selection works the same way through
  `zwp_primary_selection_*`, with the data asked from the window that owns the
  selection (`SEL_SELECTION_REQUEST`), and is skipped when the compositor does
  not offer the protocol.  `wlclip` got `--read-primary` and `--write-primary`
  for it, since the data control protocol carries the primary selection from
  version 2 on.
- `scripts/gate-wayland-clipboard.sh` runs `tests/clipboard` against headless
  sway and checks all four directions: copy in FOX and read with
  `scripts/wlclip.c`, write with wlclip and paste in FOX, and the same pair for
  the primary selection.  The peer is never a FOX client, which is the
  acceptance of this phase for text.
- wlclip speaks `zwlr_data_control_v1` rather than `wl_data_device`, and that is
  a finding of its own: **a compositor sends the clipboard selection of
  `wl_data_device` only to the client that holds the keyboard focus**, so a
  script without a window never receives it.  The data control protocol exists
  for clipboard managers and needs no focus, and it sets the selection without a
  serial.  The XML is vendored next to the virtual pointer in `scripts/protocols/`.
- Two crashes came out of the first pass and both were ours: the platform members
  of the clipboard (`xcbTypeList`, `xselTypeList`, the offer arrays) were never
  initialised, so `resizeElms()` reallocated a wild pointer; and `clearOffer()`
  reset the pointers but not the allocation counts, so the next offer wrote
  through a freed one.
- Offers have to be tracked by identity: the compositor announces the same offer
  twice (`data_offer`, then `selection`) and both slots of the peer client can
  end up holding the same proxy, which then gets destroyed twice.  This cost one
  crash in the module and one in `wlclip` before the guards were in place.
- A third one was older: the rasterizer writes straight into the backing store,
  so a window whose geometry hangs out of the store ran past the end of the
  buffer and crashed in `FXRaster::blend()`.  `fxwaylandBindWindow()` now clips
  the drawable to the store and reports an empty drawable when nothing is left.

Drag and drop is the same data path, started by a gesture instead of by a
selection:

- A press that turns into a drag calls `wl_data_device.start_drag()` with a
  `wl_data_source` of the window, the serial of the press, and no icon.  The
  source only reports `dnd_finished` or `cancelled`, so `endDrag()` waits up to a
  second for that answer before it destroys the source: destroying it earlier
  tells the compositor the drag is off and the drop of the target is lost, which
  is exactly what the gate showed when the wait was missing.
- The target sees `enter`, `motion`, `leave` and `drop` on the data device with a
  `wl_data_offer`; the mime types of that offer are read when it is announced,
  and `accept(mime)` with `set_actions()` tells the source which mime type and
  which action the target wants.  In this wayland build `wl_data_offer.accept()`
  still takes a mime type rather than an action, and the action enum has no
  `LINK`, so `FXXWL_DRAG_LINK` is dropped and `COPY` and `MOVE` are preferred.
- **An offer arrives before the protocol says what it is for**: `data_offer`
  carries the mime types, `selection` makes it the clipboard and `enter` makes it
  a drag, and the two may follow in either order.  Both slots of the platform
  take the offer while it is pending and every mime type is recorded in both
  lists; the slot that loses the race drops its list, not the proxy, because the
  same proxy is still held by the other one.  Adopting a sticky offer at `enter`
  used to clear the list that had just been filled, and that is what made the
  target accept nothing and the compositor cancel the drag.
- A stale clipboard offer is dropped when the next one is announced, not when the
  `selection` event comes: the mime types of the new offer are already recorded
  by then, and clearing the list at `selection` threw them away with the old
  ones.  Same symptom, different place.
- All windows share one `wl_surface`, so a drag that stays inside the
  application never leaves the surface and the compositor sends `enter` once,
  over the widget that started the drag.  The target therefore has to be found
  per motion event: the window under the pointer is resolved through `FXWinMap`
  and, when it changes, `SEL_DND_ENTER` is raised for it and the previous one
  gets `SEL_DND_LEAVE`.  Without that the widget under the pointer never hears
  about the drag.
- `scripts/gate-wayland-dnd.sh` runs `tests/dnd` and drags the label of the
  source into the target: press, move out of the source, move over the target,
  release.  The moves matter, since the compositor only starts delivering the
  drag after the pointer left the surface of the source; `wlxinject` got
  `move:<x>,<y>` for it.

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
  and a drag of a file list in both directions.  Text is done in both
  directions; the file list (`text/uri-list` on the source, `urilistType` on the
  target) still has no gate.

### Phase 9: input method, scale, settings, polish

Done in this phase (the input method, the scale, the activation, the desktop
settings and the window icon):

- `fxtextinput_wayland.cpp` binds `zwp_text_input_manager_v3` and makes one
  `zwp_text_input_v3` for the seat, since the compositor does not hand out a
  context per widget: the input method follows the keyboard focus and the client
  enables its context while a window of its own has it.  Enable and disable are
  driven by `FXXWL_FOCUSIN`/`FXXWL_FOCUSOUT`, as `SEL_IME_START`/`SEL_IME_END`
  are on X11.
- The protocol is double buffered: `enable`, `disable`, the content type and the
  caret rectangle only take effect when `commit()` follows, so every change in
  the module ends with a commit and a flush.
- The caret rectangle is what decides where the pre-edit window goes.  It comes
  from `FXComposeContext::setSpot()` and `setArea()`, which are called by the
  widget that knows where it is typing; the rectangle is kept in the platform and
  re-sent on enable.
- A committed string is delivered as a key press and a key release that carry the
  text, which is what XIM does on X11, so a widget that inserts `event.text` does
  not care where the text came from.  The neutral event grew a `text[256]` field
  for it and `FXXWL_IME` joins the event codes.
- `scripts/gate-wayland-ime.sh` runs an application against headless sway with
  `WAYLAND_DEBUG=1` and checks the wire, not our trace: the manager is bound, the
  context is created, and `enable`, `set_cursor_rectangle` and `commit` are sent
  after the focus arrives.  A committed string cannot be tested here, because
  there is no input method client on a headless sway; the half that belongs to
  the toolkit is what the gate covers.
- `fxscale_wayland.cpp` reads the scale of the compositor: `wl_output.scale` per
  output, `wp_fractional_scale_v1.preferred_scale` per surface, and
  `wl_surface.enter`/`leave` to know which outputs a surface is on.  The scale of
  a surface is the largest of its outputs, and the fractional one wins where the
  compositor offers it, because a 1.5 screen cannot be expressed otherwise.
- The device pixels of the toolkit stay what the application draws into.  A
  window of 400 logical pixels is 600 device pixels at scale 1.5, metrics and
  fonts grow with the scale through `FXApp::setMonitorScale()` (which is
  `setDisplayScaleAuto()` in the shape of a compositor scale), and
  `wp_viewport.set_destination()` tells the compositor how many logical pixels
  the buffer covers.  That keeps the drawing crisp: the rasterizer really fills
  600 pixels instead of 400 that the compositor would have to stretch.
- A compositor without `wp_viewporter` gets `wl_surface.set_buffer_scale()` with
  the rounded scale instead, which is the only tool it understands.
- The pixel size of an output comes from its **mode**, divided by the scale; the
  `wl_output.geometry` event carries the physical size in **millimeters** and is
  not a screen size.  Taking that event for the mode made the virtual root
  window 698x393 on a 2560x1440 output: `FXPopup::popup()` clamps a menu to the
  root, so every tall menu was pushed to the top-left of the real screen, while
  X11, which knows the true size, kept it under its menu title.  Only the mode
  handler sets the screen now.
- Everything that crosses the protocol is in logical pixels and is converted at
  the seam: the configure size, the pointer and drag positions (they become
  device pixels), the popup positioner and size (they go back to logical).
- The scale is applied once from the first output at `openDisplay()`, before the
  application makes its first window, so metrics and fonts start out right
  instead of being corrected on the first configure; and the display scale of the
  platform defaults to `auto`, so a registry entry is what is needed to opt out.
- `scripts/gate-wayland-scale.sh` runs `tests/dpi` on a headless sway twice, with
  the output at scale 1 and at scale 2, and passes when the theme scale is 1.00
  and 2.00 and the dumped frame grew from 520x320 to 1040x640 — the same window,
  drawn in twice the pixels.
- `fxactivation_wayland.cpp` binds `xdg_activation_v1`.  A Wayland compositor
  does not let a client raise or focus its own windows: the client asks for an
  activation token, the token carries the evidence that the user did something
  that justifies the request (the serial of an input event), and only the token
  the compositor hands back may be used, with `activate(name, surface)`.  That is
  what `_NET_ACTIVE_WINDOW` asks for on X11, and it is why the first show of a
  toplevel and every raise go through the same request.
- The answer is asynchronous and the compositor may refuse, so nothing waits for
  the focus; a client started from a terminal has no serial to show and gets a
  token anyway, which the compositor grants for the first window of an
  application.
- `scripts/gate-wayland-activation.sh` runs an application against a headless
  sway with wayland debug output and follows the whole round trip on the wire:
  the token is requested and committed, `done(...)` comes back with a name, and
  `activate(...)` is called with it.
- `FXDesktopSettings_wayland.cpp` replaces the POSIX module on this platform and
  asks the desktop for its light/dark preference through
  `org.freedesktop.portal.Settings` first, since that is the only source a
  sandboxed application can reach, and through gsettings when no portal runs.
  The library links no D-Bus: the one call that is needed is made with gdbus,
  which is part of GLib and present wherever a portal is.  The environment
  variable, the GTK settings file and the KDE globals are still read by the
  platform neutral `FXDesktopSettings.cpp` before either is asked, so nothing
  was duplicated.
- The first thing this step broke was the hello pixel gate: the portal answers on
  the session bus even when `HOME` is private, so the frame came out in the
  light or dark classic theme depending on the desktop of whoever ran the gate.
  The gate now hides `DBUS_SESSION_BUS_ADDRESS` and `XDG_CONFIG_HOME` as well as
  `HOME`, which is what a pixel gate needs from now on.
- `scripts/gate-wayland-settings.sh` needs no display and no compositor: it
  stands in for `gdbus` and `gsettings` with two scripts of its own and checks
  every step of the chain — nothing set, `GTK_THEME`, the GTK settings file, the
  KDE color scheme, and that the portal is asked *before* gsettings.
- `fxtoplevelicon_wayland.cpp` binds `xdg_toplevel_icon_v1` and fills the hole in
  `FXTopWindow::seticons()`: the pixels of the icon become a shared memory buffer
  (premultiplied on the way, since the data of an `FXImage` is not), an icon
  object carries them to the compositor with `set_icon()` on the toplevel, and
  the buffer lives as long as the surface, because the compositor may read it
  after the request.  A compositor without the protocol is told nothing and the
  window keeps whatever the desktop puts there.
- `fxidle_wayland.cpp` binds `zwp_idle_inhibit_manager_v1` and gives
  `FXTopWindow::setIdleInhibit()` its Wayland side: an inhibitor tied to the
  surface of the toplevel, created when the request is made (or when the window
  is created, if it was asked for earlier) and destroyed when the request is
  withdrawn, with the window, or with the surface.  A compositor without the
  protocol is not asked anything.  The same call does `SetThreadExecutionState`
  on Windows and runs `xdg-screensaver suspend` and `resume` on X11, so an
  application has one API on all three.
- `scripts/gate-wayland-idle.sh` runs `tests/idle` against headless sway and
  checks the wire for `create_inhibitor` and the destroy of the inhibitor.
- `scripts/gate-wayland-icon.sh` uses KWin's virtual backend, the one compositor
  here that offers the protocol, and checks the wire for `add_buffer` and
  `set_icon`; it then runs the same application against headless sway and expects
  the documented fallback.

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

Done in this phase so far (the build wiring, the metadata and the screenshots):

- Autotools builds the Wayland platform: `./configure --with-wayland` looks for
  `wayland-client` and `xkbcommon` with pkg-config and for `wayland-scanner` in
  the path, generates the client glue out of `fox2/protocols/` with `make`, and
  compiles the same module list CMake compiles.  A Wayland build looks for no
  X11 at all, and the libraries configure finds (freetype, fontconfig, the image
  codecs) are gathered in `PLATFORM_LIBS` so that both the link line and the
  metadata are right.
- `fox2/fox-1.7.pc.in` and `fox2/fox-config.in` are installed and report the
  platform: on Wayland `fox-config --libs` prints `-lwayland-client -lxkbcommon`
  where X11 prints `-lX11 -lXext`, and nothing else changed about them.
- `docs/platform-seam-plan.md` has the Wayland module table: which module fills
  which seam, and what the seam had to grow for a compositor architecture (a
  neutral event struct, a virtual window table, a display scale that is not a
  DPI, a text field for the committed string of an input method).
- `scripts/capture-wayland-screenshots.sh` stores what the later tests draw:
  `dnd.png`, `icon.png` and `dpi-scale2.png` next to the earlier `hello.png`,
  `csd.png`, `popup.png`, `menu.png` and `tooltip.png`, all under
  `screenshots/tests/fox2/wayland/`, all from dumped frames rather than from a
  compositor screenshot.

- `scripts/gate-wayland-autotools.sh` is the check that the second build system
  works: it runs `autoreconf`, configures with `--with-wayland` in a scratch
  directory and builds the library and one test program.

## Verification

- **Compile gates:** Windows, WSL X11 and, from phase 4, WSL Wayland must all
  build; each step must leave the other two outputs unchanged.
- **Autotools gate:** `scripts/gate-wayland-autotools.sh` runs `autoreconf -if`,
  `configure --with-wayland` and `make -C lib -C tests hello` in a scratch
  directory, and passes when the library and the test program are built; the same
  script with the default platform checks that X11 did not regress.
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
  with a reason. `scripts/gate-wayland-hello.sh` does this for the first real
  window: it starts a private headless Weston, runs the application with a
  private `HOME` (so that FOX reads the built-in classic theme instead of the
  desktop's light/dark preference), collects the committed frame, and compares
  it pixel by pixel with `screenshots/tests/fox2/x11/hello.png`. It reports
  **0 differing pixels of 2158**; the stored X11 capture is two rows taller
  because it was taken with the window edge included. The gate runs against
  headless weston for two reasons: it has no seat, so no window gets the
  keyboard focus and the frame carries no focus rectangle — which is how the
  X11 capture was taken, a focused frame adds 95 pixels of dashed rectangle —
  and it offers no decorations, so the frame also carries the client side title
  bar, which the comparison skips by the height in the file name.
  The other stored captures are not usable as references yet: `button.png` and
  `dpi.png` were taken with a different desktop theme, so their palette and
  their font metrics differ from a run with a private `HOME`. Capturing fresh
  X11 references needs an X server that can run headless (`Xvfb`), which this
  machine does not have; until then those two are compared structurally (same
  window size, same ink, same layout) and not pixel by pixel.
- **Input gate:** `scripts/gate-wayland-input.sh` runs the application against a
  private headless sway with a virtual keyboard, types a space into the focused
  hello button and passes when the application quits with status 0 instead of
  being stopped. It passes; the trace shows the key arriving as keysym `0x20`
  with the text `" "` and being handled by `FXButton`.
  Two bugs came out of it: `closeDisplay()` never destroyed the widgets, so the
  surfaces were freed after the objects they were made from and the application
  crashed on shutdown with `SIGSEGV`; and the compositor's configure was
  dispatched with the wrong event field, so a tiled window kept the size it
  asked for instead of the size the compositor gave it.
  The gate itself was flaky: `wtype -k space` sends the press and the release so
  close together that sway drops the press, so it now separates them with
  `wtype -s 200 -k space` and three runs in a row pass.
- **Pointer gate:** `scripts/gate-wayland-click.sh` runs `tests/click` — one
  button filling the window, which quits when it is clicked — against a private
  headless sway and passes when the program ends with 0. The click comes from
  `tools/wlxinject.c`, a small client that keeps one
  `zwlr_virtual_pointer_v1` alive for the whole gesture: a headless sway has no
  input devices at all (its seat reports no capabilities), `swaymsg seat - cursor`
  therefore does nothing, and `wlrctl` creates a virtual pointer and destroys it
  again before the compositor delivers anything, which is why an earlier attempt
  saw no `wl_pointer.button` in the client. The injector moves the pointer to a
  layout position and then presses and releases it; the protocol XML is vendored
  as `scripts/protocols/wlr-virtual-pointer-unstable-v1.xml` and the gate builds
  the injector with `wayland-scanner` when it runs. The trace shows
  `pointer button 272 as 1 down`, `button 1 down to FXButton at +510+293`, the
  release on the same widget and `command from the button`.
  The press must not be consumed by the test widget: a handler that returns 1
  for `SEL_LEFTBUTTONPRESS` or `SEL_LEFTBUTTONRELEASE` stops `FXButton` from
  arming and from sending its command.
- **Three click bugs came before that gate:** `FXWinMap::hitTest()` wants
  coordinates of the screen — a shell carries the place it was given — while the
  pointer arrives in the coordinates of its surface, so every click hit the shell
  and no widget ever saw a press (the surface place was added before the query);
  the button codes of `wl_pointer` are event codes, not X11 numbers, so no press
  matched a FOX selector; and once that worked, two windows overlapped: the
  compositor places a toplevel and never tells the client where, so every shell
  in the table sat at the origin and a click on the lower window resolved to a
  widget of the top-most one.  A pointer event is now hit-tested inside its own
  surface (`FXWinMap::hitTest(shell,x,y)`), and the screen-wide query is only
  for the backends that know the places.  `tests/winmap` keeps a case that moves
  a shell and checks both conventions.
- **Double click gate:** `scripts/gate-wayland-doubleclick.sh` runs the ported
  PathFinder on a directory with one text file and double clicks its first row
  with the virtual pointer. The program has no association for the file, so a
  correct run opens the "Unknown File Type" dialog; the gate sees the second
  window and fails when only the main window is there. This is the regression
  test for the click counter: `FXApp` carries `click_time`, `click_button`,
  `click_count` and `moved` in its persistent `event` member, and the first
  Wayland dispatcher replaced that member with a fresh `FXEvent` for every raw
  event, so the second press always arrived as click 1.  The list selected the
  file and nothing else happened; X11 opens the dialog.  The dispatcher now
  keeps `FXApp::event`, like the X11 and Win32 modules.
- **Repaint gate:** `scripts/gate-wayland-repaint.sh` needs no reference image.
  It drives `tests/button` to the same state twice, once with one click on
  *After Text* and once with *After Text*, *Centered* and *After Text* again, and
  compares the two dumped frames byte for byte. A correct repaint makes them
  identical; without the parent exposure of phase 5 the three click frame keeps
  9 pixels of the states in between and the gate fails.
- **Focus:** the core has no other way to learn that the compositor handed the
  keyboard to a shell, so the shell focuses its first focusable widget
  (`SEL_FOCUS_NEXT`) when the keyboard enters. A keyboard leave does not raise
  `SEL_FOCUSOUT`, because the compositor in the gate reports the leave of one
  surface twice, which would otherwise take the focus away from a widget
  between the press and the release of a key.
- **Popup gate:** `scripts/gate-wayland-popup.sh` runs `tests/popup`, which
  opens a menu pane over its main window on a timer, against headless weston
  and collects the frames. It passes: the trace shows
  `create popup for window 4 under 2 at +20+12 90x56` and two frames are
  committed, one for the main window and one for the pane, which is kept as
  `screenshots/tests/fox2/wayland/popup.png`.
- **Menu gate:** `scripts/gate-wayland-menu.sh` runs `tests/menus` against
  headless sway and clicks with the virtual pointer.  It passes when the menu
  and its submenu both become grabbing popups with the submenu hanging off the
  menu (`create popup for window 10 under 5`), when a click outside the menu
  ends in `popup_done` with the application still running, and when picking the
  third entry prints `command three` and ends the program with 0.
- **Tool tip gate:** `scripts/gate-wayland-tooltip.sh` rests the virtual pointer
  on a button of `tests/tooltip` and passes when the tool tip becomes a popup
  *without* a grab (`create popup for window 2 under 3 at +197+143 52x24 without
  grab`).  A tool tip never grabs, so the click that follows is not eaten.
- **Clipboard gate:** `scripts/gate-wayland-clipboard.sh` runs `tests/clipboard`
  against headless sway with `scripts/wlclip.c` as the peer and checks four
  directions: FOX copies `fox text for the peer` and wlclip prints it, wlclip
  writes `peer text for fox` and FOX prints `pasted "peer text for fox"`, and the
  same pair for the primary selection (`fox selection for the peer` and
  `selection pasted "peer selection for fox"`).  wlclip uses the wlr data control
  protocol, since `wl_data_device` would only reach a client with the keyboard
  focus.
- **Drag and drop gate:** `scripts/gate-wayland-dnd.sh` runs `tests/dnd` against
  headless sway and drags the text label of the source into the target with the
  virtual pointer: press at `500,60`, move to `500,200` (out of the source
  surface), move to `500,140` (the target), release.  It passes when the trace
  says `drag started from window 3 with 2 types`, `drag entered the target` and
  `dropped "dragged text for the target"`.
- **Text input gate:** `scripts/gate-wayland-ime.sh` runs an application against
  headless sway, gives it the keyboard focus with `wtype` and passes when the
  wayland debug log shows `zwp_text_input_v3#N.enable()`,
  `set_cursor_rectangle(...)` and `commit()` on the wire.
- **Scale gate:** `scripts/gate-wayland-scale.sh` runs `tests/dpi` against a
  headless sway with the output at scale 1 and again at scale 2, and passes when
  the theme scale is 1.00 and 2.00, the window the compositor configured grew
  from 520x320 to 1040x640 logical pixels, and the dumped frame is 520x320 and
  2080x1280 (twice the logical size at scale 2).  It is the check that the
  scale of the compositor reaches the application, not just the platform.  The
  window is floated on a 2560x1600 output: a tiled window is sized by the
  compositor, which made the two runs incomparable and the old gate fail.
- **Activation gate:** `scripts/gate-wayland-activation.sh` runs an application
  against a headless sway with wayland debug output and passes when the log
  shows `get_activation_token`, the token `commit`, the `done(...)` answer and
  the `activate(...)` that follows.
- **Desktop settings gate:** `scripts/gate-wayland-settings.sh` runs
  `tests/desktop` (no display) in six environments and passes when it reports
  `unknown`, `dark`, `light`, `dark`, `light` and `dark`, with the portal winning
  over gsettings.
- **Preferences gate:** `scripts/gate-wayland-prefs.sh` opens the calculator's
  preferences dialog, keeps the first committed frame of its surface and
  compares the content area with the X11 reference (a budget of 200 pixels
  covers the text antialiasing of the two compositors).  Without the visibility
  check the colour page, hidden by the switcher, was painted over the settings
  page in that first frame (8151 differing pixels).
- **Keyboard gate:** `scripts/gate-wayland-keys.sh` runs the calculator against
  headless sway, types `6/2=` with `wtype` and passes when the trace shows the
  keys reaching the Calculator (the shell, whose accelerator table then acts)
  and the display area of the committed frame changed.
- **Cursor gate:** `scripts/gate-wayland-cursor.sh` runs the calculator against
  headless sway, moves one virtual pointer from the text display to a button and
  to a label, and passes when the debug log shows `set_shape(...,9)` (text) for
  the display and `set_shape(...,1)` (default) for the button and the label.
- **Resize gate:** `scripts/gate-wayland-resize.sh` grows the calculator to
  800x520, keeps every frame that is committed at the new size and compares the
  first one with the settled layout (committed as
  `screenshots/tests/fox2/wayland/calculator-800x520.png`) and with the later
  one.  Two exposures made that first frame wrong: `shellConfigure()` exposed
  the tree before the core had seen the new size, and the configure dispatch
  exposed it before the delayed layout chore had run, so the widgets were
  painted at their old places.  The configure handler now lays the tree out
  synchronously before it exposes, and the configure listener only posts the
  event.  While that pass runs, a child that moves no longer exposes its parent
  subtree (`fxplatformPlaceWindow()` remembers the place and repaints the parent
  after every single move): one resize used to queue thousands of intermediate
  exposures and take a second of CPU on a slower compositor.  A live resize
  produces a configure per pointer motion; the listener stamps each with a
  generation and the dispatch drops a configure that is no longer the newest,
  so a backlog of intermediate sizes does not each get a full layout and
  repaint.  The gate runs with the module trace on, whose output slows the
  loop enough that the old ordering fails every time.
- **Resize drag gate:** `scripts/gate-wayland-resize-drag.sh` runs the
  calculator under a private KWin virtual backend and drags the bottom-right
  border with `tools/kwininject`, which binds KWin's `org_kde_kwin_fake_input`.
  It requires the window to grow, every applied configure to be committed (a
  couple of frames of slack), and no more than 150 ms between two commits.  The
  calculator is centred by KWin, so the gate finds the client origin from the
  enter event at the output centre and then presses two pixels outside the
  client corner to grab KWin's resize border.  This is the only scripted test
  of the drag path on the compositor the users run; sway coalesces its own
  configure stream and cannot reproduce a drag.
- **Scroll gate:** `scripts/gate-wayland-scroll.sh` opens the calculator help
  window, which starts at the top of the document, rolls the wheel down three
  notches and then up three again; the frame must change on the way down and be
  identical again on the way up.  `wl_pointer` counts a positive vertical axis
  towards the end of the document while FOX counts a positive wheel towards the
  top (X11 Button4, a positive `WM_MOUSEWHEEL` delta), so the seat flips the
  sign; without it the down scroll is clamped at the top and does nothing, and
  the gate fails with "the sign is inverted".
- **Modal gate:** `scripts/gate-wayland-modal.sh` opens the calculator's
  preferences — a modal invocation — types `7`, clicks AC on the main window
  behind the dialog and types `8`; the display must stay `7` and the dialog must
  stay up, while a click inside the dialog still toggles its checkbox.  The
  click and the key are first repeated with no dialog, so a broken coordinate or
  input path is told apart from a broken filter.  Wayland dispatched both to the
  blocked window; motion, wheel, button, key and IME events now apply the same
  condition as X11 and Win32 (`invocation`, its window, `isOwnerOf`,
  `doesSaveUnder`), and a press in the title bar of a blocked window is
  swallowed when the client draws the bar itself.  The gate ends by asking the
  compositor to close the blocked window, which is what the title bar X of a
  server side decoration sends; the application used to crash there, because
  destroying the window also destroyed the button whose command was running the
  modal loop on the stack.  A close request for a window the modal loop blocks
  is now ignored.  The dialog itself names its owner (`xdg_toplevel.set_parent`)
  and is marked with `xdg-dialog-v1.set_modal` while the loop runs; KWin, which
  offers that protocol, can then keep it above its parent and refuse the
  parent's input, while compositors without it (sway) rely on the filter.
- **Toplevel icon gate:** `scripts/gate-wayland-icon.sh` runs `tests/icon`
  against KWin's virtual backend and passes when the log shows `add_buffer` and
  `set_icon`; the same application against headless sway has to report that the
  protocol is missing.
- **Decoration gate:** `scripts/gate-wayland-decoration.sh` runs the
  application against KWin's virtual backend, which is the only compositor here
  that offers `xdg-decoration` (headless weston does not), and passes when the
  trace says `decoration of window 2 is server side`. With a compositor that
  does not offer the protocol, no decoration object is made and the client draws
  its own bar — `scripts/gate-wayland-csd.sh` checks that side.
- **Client side decoration gate:** `scripts/gate-wayland-csd.sh` runs the
  application against headless weston, which offers no decorations, and passes
  when the frame carries a title bar of its own (`window-2-83x52-bar26.ppm`)
  with ink in it: 211 of the 2158 bar pixels are the three buttons and the
  shadow line. The frame is kept as
  `screenshots/tests/fox2/wayland/csd.png`.
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
