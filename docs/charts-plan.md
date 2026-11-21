# Chart widget plan

A reusable chart widget for `fox2`: antialiased curves, smooth zoom and
scrolling, many samples, several series, legend, axis descriptions, autoscale,
autozoom and SVG export. Stretch goal: a third axis, and a rotatable 3D view.

The requirements were:

- smooth zoom
- smooth scrolling
- at least 10,000 points per series
- multiple series
- legend
- axis descriptions
- built on the `fox2` port
- SVG export
- autoscale
- autozoom
- stretch: a third axis, and rotation

## What the fox2 port already provides

The FOX 1.7.95 chart package is copied to `fox2/apps/chart` and builds
`charttest`. The widget chrome is there:

- caption, margins, background fill styles and an `FXImage` back buffer
  (`FXChart`); `onPaint` blits that image, which is the flicker-free path we
  keep;
- four axes with labels, ticks, grid lines, number formatting, fonts and colours
  (`FX2DChart`, `FXAxis`);
- a series list (`FX2DPlot`, `FXCurve`), and `FXCurve::setYAxis` already lets a
  curve pick one of the four axes (`XLO`, `YLO`, `XHI`, `YHI`), so a right-hand
  Y axis is a matter of choosing `YHI`.

What is missing:

- `FXData` is an empty class in `FXCurve.cpp`: no storage, no API, so no
  samples can be handed to a curve at all;
- curves, markers, filled areas and a legend are never drawn;
- no autoscale, no data-range scan, no interaction, no cursor readout;
- no SVG writer: `FXSVGImage` uses nanosvg, which only reads and rasterises.

So the port is the widget chrome plus a paint path, not a chart. Data model,
curve rendering, antialiasing, interaction and export all have to be written;
the chrome is reused.

## Decisions

### OpenGL: no, for the first version

- `FXGLCanvas` on Wayland is a `wl_subsurface` with its own EGL window. Embedding
  it inside a chart, with axes, legend, clipping, MDI/docking, window moves and
  screenshots, adds platform work that is not part of the chart problem.
- GL text is a separate stack; axis labels, numbers and the legend would need
  either GL font rendering or overlay surfaces.
- Offscreen rendering to an image for PNG and print is trivial with a software
  buffer and awkward with GL.
- The software rasteriser will not be the bottleneck once data is decimated, so
  GL buys nothing until there are very large visible point counts.

Keep a renderer seam so an `FXGLChart` can be added later without changing the
public API. GL becomes the right answer only for the rotation/3D item, or if the
frame-time gate below shows that the software path cannot hold 60 fps.

### Porting another chart library: no

QCustomPlot and Qwt are Qt throughout; ImPlot needs Dear ImGui and an
immediate-mode host; gnuplot is an external process, not a widget; the
commercial libraries bring a licence and a foreign widget stack; plplot and
DISLIN carry their own canvases and device layers. Each option means
maintaining a second widget stack plus a FOX bridge, and still writing the parts
that make a FOX chart feel native.

### Third-party library inside the widget: no

Keep the chart dependency-free:

- SVG export: write a small SVG serialiser (text output, a few hundred lines).
  The SVG renderer consumes the same primitive stream as the screen renderer, so
  the export is exact and can be tested with golden files.
- PNG export: `FXImage` already has it.
- CSV export: `FXChart` already declares a CSV drag type.

Do not add Cairo, librsvg or a serialisation library for this.

## Architecture

Model, view and renderer separated, so interaction, export and testing stay
cheap:

- `FXData` — sample storage, the main missing piece. Append-only, `double x,y`
  and optional `z` (kept for the 3D item), optional x-sorted fast path, cached
  minimum/maximum, `append`, `remove`, `clear`, `findIndexAt(x)` (binary search)
  for zoom and cursor readout.
- `FXCurve` — series: data plus line/marker/fill style plus axis index. No
  drawing logic.
- `FX2DPlot` — owns the view range (`FXAxis::axisrange`) separately from the
  data range, and the view operations: `zoomAt(anchor,factor)`, `pan(dx,dy)`,
  `fitData()`, `followLast(seconds)`. Emits `SEL_CHANGED` for a status line and
  cursor readout.
- `FXChartRenderer` — internal interface with the primitives the chart needs
  (fill rect, polyline, filled polygon, text, clip rect). Two implementations:
  `FXChartRaster` (screen, PNG) and `FXChartSVG` (text). Both see the same
  decimated geometry, so screen and export agree.
- `FXLegend` — drawn inside the plot image (entry = marker plus label), click
  toggles series visibility and emits a message. Keeping it in the same buffer
  keeps the SVG export and the AA path simple.

## Rendering target and antialiasing

`FXDC` cannot antialias: X11 uses `XDrawLines`/`XFillPolygon`, Win32 uses GDI,
and the Wayland `FXRaster` plots whole pixels on purpose because X11 is the
visual reference. `FXDC` also has no alpha or coverage blending, so an
antialiased curve cannot be drawn through it, and a separate transparent curve
layer cannot be composited over an X11 chart (`FXDCWindow::drawImage` is a plain
`XCopyArea`).

The plot is therefore rasterised by code we own, into one plain `FXColor`
buffer, and handed to the window in one blit:

- create the back buffer `FXImage` with `IMAGE_KEEP|IMAGE_OWNED` (without
  `IMAGE_KEEP`, `create()` releases the client data);
- take its `getData()` pixels as the raster target;
- draw background, grid, curves, legend, axes and text into the buffer;
- `render()` (X11 and Win32) or hand the data straight to the Wayland blit, then
  `drawImage()` in `onPaint`.

The existing `FXRaster` is the right base: it is private, platform neutral, its
target format is already `FXColor` (`0xAARRGGBB`, alpha included), it has
`blend(x,y,cov)` and `blendPixel`, dashed and wide lines, caps and joins,
gradients, clipping and fill rules, and FreeType text through `FXGlyphs` with the
same fontconfig match Xft uses.

Add antialiasing there, not in the chart:

- `setAntialias(FXbool)`, default off, so every existing Wayland widget stays
  bit-identical;
- coverage-based antialiasing for lines, polylines, line segments, polygon
  fills, markers and arcs: walk the major axis, compute fractional coverage in
  the minor axis, extend to spans for widths above one, blend with `blend()`;
- accumulate coverage per polyline into a row-span buffer and blend once per
  pixel with the maximum coverage, not the sum, so joins and overlapping dense
  data do not darken;
- antialias on for series, markers, filled areas and legend swatches; off for
  grid, ticks and 1-px rules, which stay snapped to half pixels and crisp;
- dash handling reuses the existing dash machinery; split by dash length first,
  then antialias each dash.

Text needs an `FXGlyphs`. Wayland already provides one through
`FXFont::getGlyphCache()`; the X11 `FXFont` returns `nullptr` because Xft draws
the text itself. Give `FXFont_x11` a lazily created glyph cache from the same
fontconfig pattern (the metric comments in `FXGlyphs` say the two agree), or, if
the library should stay untouched, let the chart build its own `FXGlyphs` from
`FXFont` family, size, weight, slant and DPI. Either way text stays antialiased.
Prefer the `FXFont` change: other software-rendered widgets need it too.

SVG is unaffected by all of this: the serialiser emits `stroke` polylines and
`stroke-linejoin`, and the viewer antialiases them. Emit the same decimated
geometry as the screen renderer.

### Why not supersampling

Rendering at 2x or 3x with `FXDC` and downscaling with `FXImage::scale(w,h,1)`
(box filter) or `2` (gamma corrected) works today without a new rasteriser, but
it costs 4x to 9x memory and fill time, needs a full readback per frame, and is
visibly softer than analytic coverage. On X11 the readback alone breaks 60 fps
zoom and pan. Rejected.

### Why not XRender

`XRender` can antialias X11 lines, but there is no equivalent on Wayland or
Win32, so the chart would look different per platform. Rejected.

## Data and performance

- 10,000 samples per series is a data-model requirement, not a draw requirement:
  per-pixel-column min/max decimation turns any series into at most two vertices
  per plot column, exactly and cheaply, with a full-resolution polyline when
  zoomed in.
- Decimation is computed from the view range, so zoom and pan always draw the
  samples that are visible at the resolution of the screen.
- The client buffer makes incremental scrolling cheap: pan by an integer pixel
  offset is a `memmove` of the plot rectangle plus a redraw of the exposed strip.
- Target: 8 series of 100,000 samples at 1000x600 stays under 16 ms per frame on
  X11 and Wayland. Measure it in a perf test, do not assume it.

## Interaction semantics

- Wheel: exponential zoom, about 1.25 per notch, anchored at the cursor, so the
  data under the pointer stays put. Clamp to the data range with a small
  overscan.
- Drag: pan in data units. Shift-drag on an axis pans that axis only.
- Double click: fit all data (`fitData`). Keyboard: `0` reset, `+`/`-` zoom.
- Smooth means: 1:1 response for wheel and drag, no easing; easing only for
  programmatic moves (fit animation, momentum), driven by `FXTime`, never for
  direct input.
- Autoscale is an axis property (`AXIS_AUTOSCALE`): compute from the data range
  with a 5% margin. Autozoom/follow is a view mode (`followLast`): keep the last
  N seconds or points visible while data streams in, with hysteresis so a single
  spike does not rescale the whole plot. Keep the two flags separate.
- Third axis: use the existing `YHI` axis as the right-hand Y axis, with curves
  selecting it through `FXCurve::setYAxis`. A true third scale or a 3D view is a
  separate widget.

## Export

- SVG: text serialiser behind `FXChartRenderer`, emitting polylines, axes,
  ticks, labels and legend. Golden-file test against a fixed dataset.
- PNG: render the same primitives into an `FXImage` and save it.
- CSV: iterate `FXData` and write `x,y` (or all series, one column each).
- Print: `FXDCPrint` can consume the same primitive stream later, if wanted.

## Placement and build

Keep the sources in `fox2/apps/chart` for the first milestones, but compile them
into a small static library target (CMake and Automake) that `charttest` and any
future application link, instead of compiling them into one executable. If a
second fox2 application uses the widget, promote it to a module with public
headers (`fox2/include/fxchart/`), like the FOX 1.7.95 package. Decide before M1
so the headers do not have to move later.

## Milestones

One stage per commit, as in the other port plans. Each stage is accepted when
the X11 build, the Win32 cross build and the Wayland build pass, and `hello`,
`button` and `dpi` still run.

1. Data model and static curves. Implement `FXData`, draw curves, markers and
   fills from it, and add a deterministic dataset mode to `charttest` (10,000
   samples, 3 series). Move the chart to the client-side `FXImage`+`FXRaster`
   buffer, still aliased. Acceptance: the existing chrome keeps the same layout
   and colours on X11 and Wayland against the `charttest` baseline screenshot,
   `hello`, `button` and `dpi` are unchanged, and the Win32 build renders the
   same; a text hinting difference against Xft, if any, is recorded explicitly.
2. Antialiasing. `FXRaster::setAntialias` with AA lines, polylines, polygons,
   markers and arcs; glyph cache for X11 or chart-owned glyph caches;
   `CHART_ANTIALIAS` option. Acceptance: golden screenshots on X11, Wayland and
   Win32, plus an SVG export that matches the screen geometry.
3. Autoscale, tick refinement, axis descriptions for all four axes and the
   legend with click-to-toggle.
4. Interaction. Cursor-anchored wheel zoom, drag pan, double-click fit,
   keyboard reset, cursor readout through `SEL_CHANGED`, and the `memmove`
   pan optimisation.
5. Decimation and follow mode. Per-column min/max, view-range recomputation, and
   streaming follow with hysteresis. Acceptance: the perf test meets the frame
   budget.
6. SVG and CSV export. SVG golden files, CSV round trip, and a PNG export check.
7. Optional. Right-hand Y axis polish; a 3D/rotatable view, and a GL renderer
   behind `FXChartRenderer` only if the perf gate requires it.

## Verification

- Screenshots under `screenshots/tests/fox2/x11/`, `.../wayland/` and
  `.../win32/` (`chart-basic.png`, `chart-aa.png`, `chart-legend.png`,
  `chart-zoomed.png`).
- A Wayland chart gate script following `scripts/gate-wayland-*.sh`, including a
  wheel and drag injection through `tools/xinject`/`wlxinject`.
- A perf test binary that times a full repaint at 8 x 100,000 points and fails
  above the budget.
- Golden SVG files compared as text after normalising coordinates, so a change
  in decimation or style is visible in review.
- `scripts/build-fox2-linux.sh`, `scripts/build-fox2-mingw-cross.sh` and
  `scripts/build-fox2-wayland.sh` must keep passing.

## Alternatives considered

| Option | Result |
| --- | --- |
| OpenGL chart | AA is easy and throughput is high, but embedding, GL text, clipping, MDI and export cost more than the chart itself. Revisit for 3D or if the perf gate fails. |
| Port QCustomPlot / Qwt / ImPlot | Rewrite in FOX clothing; second widget stack; no gain. |
| External gnuplot | Not a widget, separate process, separate rendering. |
| Supersampling with `FXImage::scale` | Works without new raster code, but 4x-9x fill, readback per frame, softer output. |
| XRender antialiased lines | X11 only; no Wayland or Win32 seam. |
| Transparent AA curve layer over the FXDC chart | X11 `drawImage` cannot composite alpha, so the layer erases the axes. |
| Adding Cairo | New dependency for only the SVG writer; the serialiser is small and exact. |

## Open questions

- Module or app: does another fox2 application need the widget in this phase?
  Affects header placement, not the implementation.
- Should antialiased text match the Xft output pixel for pixel, or is "visually
  the same and metric-compatible" enough for the gates?
- Follow-mode defaults (window length, hysteresis) and whether follow should
  stop on user pan (it should, and resume on request).
- Third axis: right-hand Y only, or a second plot area with an independent
  scale.
