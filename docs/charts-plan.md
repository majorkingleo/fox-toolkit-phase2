# Chart widget, test application and demo application plan

Three pieces share one data model:

- a reusable chart widget for `fox2`: antialiased curves, smooth zoom and
  scrolling, many samples, several series, legend, axis descriptions, autoscale
  and autozoom; stretch goal: a third axis and a rotatable 3D view;
- `charttest`, the widget regression application, which also hosts the live
  streaming mode;
- `chartdemo`, the user-facing application that opens data files, plots them
  and exports them.

The widget requirements are:

- smooth zoom
- smooth scrolling
- at least 10,000 points per series
- multiple series
- legend
- axis descriptions
- built on the `fox2` port
- autoscale
- autozoom
- stretch: a third axis, and rotation

The `charttest` requirements are:

- keep the existing chrome, axis, font and deterministic-data checks;
- provide a live mode that streams mouse acceleration and other computer values
  into the chart, to exercise append, follow and redraw.

The demo application requirements are:

- open CSV, XLSX and ODS files;
- show every column that contains numbers as a series;
- use the first numeric column as the X axis by default, with a way to choose
  another column;
- expose the widget's data and plot geometry so the application can write SVG
  itself.

SVG export is deliberately an application feature, not a widget feature. The
widget owns the data model, the view range and the mapping from data to device
coordinates; the application owns the file format and its styling. The widget
must therefore provide a complete read-only data and geometry API, but it must
not contain an SVG serialiser, a spreadsheet reader or a file-format layer.

## Progress

Target order: Wayland first; Win32 thin-line antialiasing is now implemented,
with X11 and the shared `FXRaster` rendering path still pending.

### M1 — data model and static curves

- [x] `FXData` storage, unlimited and ring-bounded
- [x] `FXCurve` line/marker/fill style, visibility, correct default axes
- [x] `FX2DPlot` draws visible curves with gaps, markers and plot clipping
- [x] `FX2DPlot::fitData` and `fitY`; upper/right axes follow the lower/left
- [x] `charttest` deterministic 10,000-sample series plus a marker series
- [x] Wayland build and `FOX_WAYLAND_DUMP` frame verified
- [ ] curve fills
- [ ] client-side `FXImage` buffer (chart still draws through `FXDC`)
- [x] Win32 `charttest` build and antialiased curve screenshot verified
- [ ] X11 verification

### M3 — antialiasing

- [x] `FXRaster::setAntialias` with Wu coverage for thin solid lines
- [x] segment clipping so off-screen live samples stay cheap
- [x] `FXDC`/`FXDCWindow` seam; Wayland forwards it, X11 and Win32 accept it
- [x] chart curves and marker outlines antialiased on Wayland
- [x] thin solid Win32 lines and polylines antialiased through GDI+
- [x] live mode stays around 12% CPU at 20 Hz with AA
- [ ] polygon fills, wide lines and dashed lines
- [ ] X11 antialiasing
- [ ] shared `FXRaster` chart rendering on X11 and Win32; Win32 currently uses
  the native GDI+ line path instead

### M6 — live mode in `charttest`

- [x] `LiveSource` timer and mouse speed/acceleration series
- [x] Linux CPU, memory and chart frame-time series (Win32 seam pending)
- [x] follow window and ring retention
- [x] `charttest --live`, Wayland build and `charttest-live.png` verified on sway
- [x] axis changes redraw the chart image without relayout; live mode stays
      under 10% CPU at 20 Hz on Wayland
- [x] `FXRaster` row-fill and opaque-image blit fast paths; chart update is
      about 3.5 ms and the blit is under 0.2 ms
- [x] View menu selects static/live with one two-entry radio group
- [x] Live view hides the title, starts at 20 Hz, and shows editable sample
      rate and viewport fields plus the buffer size and current values in a
      side panel; the chart keeps a fixed-width time window while scrolling
- [x] Right sidebar lists every series with a color swatch, its label and a
      visibility checkbox; toggling one refits the axis or the live window
- [x] Horizontal scrollbar below the chart, shown only in live mode, pans the
      viewport through retained history; dragging back to the end resumes
      following
- [x] Mouse wheel zooms both axes around the pointer and a rubber band drawn
      with the left button zooms into a rectangle; a double click returns to
      the ranges in effect before the first zoom
- [x] A Reset zoom button at the bottom of the sidebar restores the pre-zoom
      ranges and is greyed out while nothing is zoomed
- [x] Chart colors come from the application theme: the frame uses the window
      background, the plot area the base color, axis text the foreground color
      and the grid a contrasting shade; series colors are lifted on a dark
      background.  `charttest` shows the result with the classic and the dark
      theme
- [x] Axis numbers that come and go fade over 150 ms: a number entering the
      range fades in and one leaving it fades out where it was, while the
      numbers that stay are drawn at once.  Each number carries its own appear
      and disappear time, so a change does not disturb the others and the fade
      looks the same when the sample rate drops.  The first draw, a zoom and a
      resize show the numbers at once, and the live view only fades the time
      axis because its y axis is refitted on every sample.  A change that is
      too large or comes while the previous one is still fading, as when the
      viewport is dragged, drops the fades instead of piling numbers up
- [x] Horizontal tick labels are measured and skipped when they would overlap
      or extend past the axis during zoom; tick marks and grid lines remain
      unchanged

### M7 — chartdemo (initial CSV slice)

- [x] `chartdemo` opens multiple selected CSV files, one document per tab
- [x] Delimiter detection for comma, semicolon, tab and pipe; quoted fields,
  doubled quotes and embedded newlines are parsed
- [x] BOM-based UTF-8/16/32 detection, BOM-less UTF-16/32 heuristics, UTF-8
  validation and Windows-1252 fallback
- [x] Numeric and date/time columns are inferred; the first detected timestamp
  column is the default X axis and is plotted as elapsed seconds
- [x] X-column selector, per-series visibility controls, CSV inspection mode
  and the supplied 8,795-row Manila sample verified
- [ ] Preview/table view, explicit codec and delimiter overrides, XLSX/ODS
  import, export and cross-platform UI screenshots

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
- no read-only data and geometry API for an application that wants to render
  or export the plot itself;
- no file importers; these belong to the demo application, not to the widget.

So the port is the widget chrome plus a paint path, not a chart. Data model,
curve rendering, antialiasing, interaction and the data/geometry API have to be
written; the chrome is reused. File import and SVG export are written in the
application on top of that API.

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

Keep the widget dependency-free:

- PNG export: `FXImage` already has it.
- CSV: `FXChart` already declares a CSV drag type; the demo application does
  the actual table import and export.
- No SVG, ZIP or XML library inside the widget. The widget exposes data and
  geometry; importers and exporters live in the demo application.

Do not add Cairo, librsvg, libxml2, a spreadsheet library or a serialisation
library to the widget.

### SVG export: application code, not widget code

The widget does not serialise SVG and has no `FXChartSVG` renderer. It exposes
the model (`FXData`), the series (labels, styles, visibility), the axis ranges
and the plot rectangle, plus a data-to-device mapping. The demo application
walks that API and writes SVG.

This keeps the format decisions where they belong: grouping, coordinate
precision, CSS versus presentation attributes, embedded fonts, metadata and
multi-curve clipping are application policy. It also keeps the widget free of a
second renderer that would have to be kept geometrically identical to the
screen path.

The data and geometry API is described below and is part of the widget's public
surface, so a second application can export SVG without private access.

### File import: application code, with FOX primitives only

The demo application reads CSV, XLSX and ODS. None of this belongs in the
widget:

- CSV is parsed by a small RFC 4180-style reader in the application: quotes,
  doubled quotes, embedded newlines, CRLF, and delimiter detection for comma,
  semicolon and tab.
- XLSX and ODS are ZIP containers with XML inside. The application reads the ZIP
  central directory, inflates stored and deflated entries with zlib, and parses
  the XML with `FXXML`, which `fox2` already provides.
- zlib is not a new project dependency: `fox2` already links it when
  `HAVE_ZLIB_H` is available, because PNG I/O needs it. If `fox2` is built
  without zlib, CSV still works and XLSX/ODS report a clear error.
- No OpenXLSX, libxlsxio, libods, libxml2 or LibreOffice conversion step is
  added. The ZIP reader is a few hundred lines; the XML is consumed through the
  existing parser callbacks.

Importers produce one neutral table: column names, rows, and a numeric flag per
cell. Mapping that table to curves is shared by all three formats, so CSV, XLSX
and ODS cannot drift apart.

### Live data belongs to `charttest`

Live data exercises the widget's streaming path: append cost, ring-buffer
retention, decimation of a moving window, follow mode, repaint time and
`memmove` panning. That is widget behaviour, not file-import behaviour, so it
belongs in `charttest`, the regression application. `chartdemo` stays focused on
opening files and exporting them.

Both applications link the same `foxchart` library and the live sources are
compiled into `charttest` only. `chartdemo` does not gain a timer-driven view
unless it later needs one for a different reason.

## Architecture

Model, view and renderer separated, so interaction, export and testing stay
cheap:

- `FXData` — sample storage, the main missing piece. Append-mostly, `double x,y`
  and optional `z` (kept for the 3D item), optional x-sorted fast path, cached
  minimum/maximum, `append`, `remove`, `clear`, `findIndexAt(x)` (binary search)
  for zoom and cursor readout. A settable capacity turns it into a ring buffer
  for live sources, so history stays bounded. Read access (`size`, `getX`,
  `getY`, ranges) is public because the application needs it for export.
- `FXCurve` — series: data plus line/marker/fill style, label, visibility and
  axis index. No drawing logic. The style getters are public for the same
  reason.
- `FX2DPlot` — owns the view range (`FXAxis::axisrange`) separately from the
  data range, and the view operations: `zoomAt(anchor,factor)`, `pan(dx,dy)`,
  `fitData()`, `followLast(seconds)`. Emits `SEL_CHANGED` for a status line and
  cursor readout. Also owns the data-to-device mapping and the plot rectangle
  query used by the application's SVG writer.
- `FXChartRenderer` — internal interface with the primitives the chart needs
  (fill rect, polyline, filled polygon, text, clip rect). `FXChartRaster` is
  the only implementation (screen and PNG) and keeps the door open for a GL
  renderer. There is no SVG implementation; SVG is application code.
- `FXLegend` — drawn inside the plot image (entry = marker plus label), click
  toggles series visibility and emits a message. Keeping it in the same buffer
  keeps the AA path simple.

### Data and geometry API for applications

The application-side SVG writer must not touch widget internals, so the widget
publishes:

- `FX2DPlot::numCurves()`, `getCurve(i)`;
- `FXCurve::getLabel()`, `isVisible()`, and getters for its `LineStyle`,
  `Marker` and `FillStyle`;
- `FXData::size()`, `getX(i)`, `getY(i)`, `getXMin()`, `getXMax()`,
  `getYMin()`, `getYMax()`; indices expose logical order even when the ring
  buffer has wrapped;
- `FXAxis::getAxisRangeMin()`, `getAxisRangeMax()`, `getDataRangeMin()`,
  `getDataRangeMax()`, and a tick enumerator plus the number formatter used by
  the screen renderer, so SVG ticks and labels match;
- `FX2DChart::getPlotRect(FXRectangle&)` and
  `FX2DChart::mapToDevice(axisX,axisY,dataX,dataY,FXint&,FXint&)`.

The plot rectangle is the same one `drawSelf` uses, computed by a shared
`calcPlotRect()` instead of duplicated local arithmetic. The mapping is the
same affine transform the rasteriser applies, so an exporter cannot drift from
the screen. Everything else stays private: the back buffer, clipping,
decimation buffers and the renderer.

## Rendering target and antialiasing

`FXDC` has no cross-platform coverage-blending API. X11 uses
`XDrawLines`/`XFillPolygon`; Win32's thin, solid `drawLine()` and `drawLines()`
now use GDI+ when antialiasing is enabled, with GDI fallback; the Wayland
`FXRaster` path provides software Wu antialiasing. Win32's GDI+ support is a
targeted backend improvement, not yet the shared chart raster path. A separate
transparent curve layer cannot be composited over an X11 chart
(`FXDCWindow::drawImage` is a plain `XCopyArea`).

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

The current implementation uses per-segment Wu coverage, clipped to the plot
rectangle; the row-span maximum-coverage accumulation for dense joins, wide
lines, dashed lines and filled polygons remains future work.

Text needs an `FXGlyphs`. Wayland already provides one through
`FXFont::getGlyphCache()`; the X11 `FXFont` returns `nullptr` because Xft draws
the text itself. Give `FXFont_x11` a lazily created glyph cache from the same
fontconfig pattern (the metric comments in `FXGlyphs` say the two agree), or, if
the library should stay untouched, let the chart build its own `FXGlyphs` from
`FXFont` family, size, weight, slant and DPI. Either way text stays antialiased.
Prefer the `FXFont` change: other software-rendered widgets need it too.

SVG is unaffected by all of this: the application emits `stroke` polylines and
`stroke-linejoin` from the exposed data, and the viewer antialiases them. It can
emit the full visible-range data rather than the screen-decimated geometry,
because file size does not have the screen's per-column constraint; the axis
ranges and mapping still come from the widget.

### Why not supersampling

Rendering at 2x or 3x with `FXDC` and downscaling with `FXImage::scale(w,h,1)`
(box filter) or `2` (gamma corrected) works today without a new rasteriser, but
it costs 4x to 9x memory and fill time, needs a full readback per frame, and is
visibly softer than analytic coverage. On X11 the readback alone breaks 60 fps
zoom and pan. Rejected.

### Why not XRender

`XRender` can antialias X11 lines, but there is no equivalent on Wayland or
Win32, so the chart would look different per platform. Rejected.

## Test application (`charttest`)

`charttest` stays the minimal widget/chrome regression application and gains the
live mode. Keeping live data here means `chartdemo` remains a file viewer and
`charttest` remains the place that exercises widget behaviour under load.

### Live mode

`charttest` keeps the existing caption/margin/axis/font checks and a
deterministic dataset mode, and adds a live sheet. The sheet is a small
`FXDialogBox` with `FXButton`/`FXSpinner`/`FXCheckButton` controls and an
`FX2DPlot`; it can also be docked beside the chart.

- `LiveSource` samples on an `FXTimer` at a configurable interval. The demo
  starts at 20 Hz; the side panel edits the rate and the viewport length
  (default 10 seconds). The chart is updated at most once per frame;
- the live sheet hides the chart caption and shows the buffer size and the
  current sample values in a side panel while the chart visualises the buffer;
- the time axis keeps a fixed-width viewport and scrolls horizontally as
  samples arrive; the axis frame and ticks stay in place, and a scrollbar
  below the chart, hidden in static mode, pans back through retained history;
- the chart widget zooms itself: the wheel scales both axes around the
  pointer, the left button opens a rubber band that is dragged into a
  rectangle, and a double click restores the pre-zoom ranges.  In live mode
  the x range of a zoom becomes the viewport length, so the zoom is a
  viewport reduction and leaves the following window.  The x numbers are
  centered on their ticks, so the plot leaves half a number of room at both
  ends instead of letting the end numbers run into the y numbers;
- series:
  - mouse x and y speed (pixels/s) and acceleration (pixels/s²), derived from
    `SEL_MOTION` events and `FXThread::steadytime()`;
  - pointer event rate (events/s), measured in the event handler;
  - chart repaint time (ms), measured around `updateChart()`;
  - CPU load and resident memory from a small `LiveStats` seam: `/proc/stat`
    and `/proc/self/statm` on Linux, `GetSystemTimes` and
    `GetProcessMemoryInfo` on Win32; app uptime from
    `FXThread::steadytime()`. A metric that is unavailable is not offered as a
    series;
- the sheet has start/pause/clear, sample interval, window length and a follow
  toggle; the chart uses `followLast` with hysteresis so the x axis scrolls
  without rescaling on a single spike;
- history is a fixed ring buffer (default 60 s, capped at 100,000 samples), so
  a live chart does not grow without bound.

The live sheet is the acceptance test for append cost, decimation of a moving
window, follow mode and `memmove` panning. It is also the easiest way to show
that the widget is not limited to imported files: the same `FXData`, `FXCurve`
and `FX2DPlot` API is fed from a timer instead of from a table.

## Demo application

The demo is `chartdemo`, a new target next to `charttest`, linked against the
same static `foxchart` library. `charttest` stays as the minimal widget/chrome
regression application; `chartdemo` is the user-facing viewer.

### Window

- menu bar: File (Open, Export SVG, Export CSV, Save PNG, Quit), View (fit,
  zoom in, zoom out, legend), Help;
- chart fills the window, status bar shows file, series count, cursor x/y and
  import warnings;
- an optional series panel lists the columns, their colours and visibility.

Opening a file infers the format from the extension, then from content when the
extension is unknown. The last directory is remembered.

### Column policy

All three importers produce the same table, so the policy lives in one place:

- the first row is a header when at least one cell in it is not a number;
- a column is numeric when at least one non-empty cell parses as a number;
- non-numeric cells in a numeric column become gaps (`NaN`), not zeros;
- the first numeric column becomes X by default; the remaining numeric columns
  become series, labelled with their header;
- when the first column is not numeric, X is the row index (1, 2, 3, ...) and
  every numeric column is a series;
- a column chooser allows another numeric column to be used as X, or X to be
  the row index; choosing a column as X removes it from the series list;
- if no numeric column exists, the file is rejected with a message.

This keeps the common case zero-configuration while making the X choice
explicit.

### Formats

CSV:

- UTF-8, comma/semicolon/tab delimiter detection, quoted fields, doubled
  quotes, embedded newlines and CRLF;
- numbers use `.` as decimal separator; no locale-dependent parsing;
- a single-column file plots against the row index.

XLSX:

- first worksheet only in the first milestone;
- `xl/workbook.xml`, `xl/_rels/workbook.xml.rels` and
  `xl/worksheets/sheetN.xml`; `xl/sharedStrings.xml` for shared strings;
- numeric `<v>`, `t="s"`, `t="inlineStr"` and `t="str"` cells; formula cells
  use their cached `<v>`;
- booleans, errors and empty cells are ignored;
- dates are Excel serial numbers and are treated as numbers until a date axis
  is added.

ODS:

- first sheet only in the first milestone;
- `content.xml`; `table:table`, `table:table-row`, `table:table-cell`,
  `table:number-rows-repeated` and `table:number-columns-repeated`;
- `office:value-type` of `float`, `currency` or `percentage` with
  `office:value` is numeric; `text:p` is a string; covered cells are skipped.

ZIP:

- read the end-of-central-directory record, walk the central directory, and
  support method 0 (stored) and method 8 (deflate);
- inflate with zlib raw deflate (`inflateInit2(-MAX_WBITS)`);
- reject entries whose uncompressed size exceeds a configurable limit, and
  check that the inflated size matches, so a malformed archive cannot exhaust
  memory.

### Export

- SVG: `ChartSVGExporter` in the application, driven by the widget's public
  data/geometry API. It writes the visible x range, every visible series as a
  `<polyline>` in its own colour, markers, the axes, ticks, labels and a legend
  generated from the series list. The screen and SVG may differ in point count
  (SVG uses the full visible data); they must not differ in ranges, axis
  placement or colours.
- CSV: the application writes the table it imported, or the visible range from
  `FXData`;
- PNG: the existing `FXChart` image buffer and clipboard path, plus a Save As
  action that hands the same `FXImage` to `fxsavePNG`.

## Data and performance

- 10,000 samples per series is a data-model requirement, not a draw requirement:
  per-pixel-column min/max decimation turns any series into at most two vertices
  per plot column, exactly and cheaply, with a full-resolution polyline when
  zoomed in.
- Decimation is computed from the view range, so zoom and pan always draw the
  samples that are visible at the resolution of the screen.
- The client buffer makes incremental scrolling cheap: pan by an integer pixel
  offset is a `memmove` of the plot rectangle plus a redraw of the exposed strip.
- A live source appends to a ring-backed `FXData`; the oldest sample is dropped
  in O(1). No per-tick allocation and no unbounded growth; the text readouts in
  the live sheet are updated from the same append.
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

Export is split by ownership:

- SVG is application code. The widget exposes data, styles, axis ranges, tick
  enumeration, the plot rectangle and the affine mapping; `ChartSVGExporter`
  serialises polylines, markers, axes, ticks, labels and a legend. Golden-file
  test against a fixed dataset.
- CSV is application code. It writes the imported table, or the visible range
  iterated through `FXData`; one column per series plus the X column.
- PNG and the image clipboard stay in `FXChart`: the back buffer is already an
  `FXImage`, so `fxsavePNG` and the existing drag targets keep working.
- Print: `FXDCPrint` can consume the same widget primitives later, if wanted.
  That is also widget code, not application code.

## Placement and build

Keep the widget sources in `fox2/apps/chart`, compiled into a static `foxchart`
library (CMake and Automake) that both `charttest` and the new `chartdemo`
link. The demo application's sources live beside them:

- `charttest.cpp` — widget/chrome regression application and live mode;
- `live/` — `LiveSource`, `LiveStats` and the live sheet, compiled into
  `charttest`;
- `chartdemo.cpp` — window, menu, status bar and column policy;
- `import/` — `Table`, CSV, XLSX and ODS readers;
- `export/` — `ChartSVGExporter`, CSV writer and PNG save.

The widget stays app-local for now. If a second fox2 application uses it,
promote the widget sources to a module with public headers
(`fox2/include/fxchart/`), like the FOX 1.7.95 package. The importers and
exporters stay in the demo either way; they are not part of the widget.

The `foxchart` static library contains only the widget. `charttest` links it and
compiles `live/`; `chartdemo` links it and compiles `import/` and `export/`. An
application that wants a chart without file I/O or live sampling pulls in
neither the importers nor `live/`.

## Milestones

One stage per commit, as in the other port plans. Each stage is accepted when
the X11 build, the Win32 cross build and the Wayland build pass, and `hello`,
`button` and `dpi` still run.

1. Data model, static curves and the CSV demo. Implement `FXData`, curve,
   marker and fill drawing, series colours, and the `chartdemo` window with CSV
   import, header detection and the shared column policy. Move the chart to the
   client-side `FXImage`+`FXRaster` buffer, still aliased. Acceptance: the same
   CSV renders on X11, Wayland and Win32 with the expected series count and
   axis ranges; the existing `charttest` chrome keeps its layout and colours;
   `hello`, `button` and `dpi` are unchanged; a text hinting difference against
   Xft, if any, is recorded explicitly.
2. XLSX and ODS import. ZIP central directory, stored and deflated entries,
   `FXXML` callbacks, shared strings, repeated rows and columns, and sample
   fixtures that describe the same table as the CSV. Acceptance: all three
   formats produce identical series names and values in the import check.
3. Antialiasing. `FXRaster::setAntialias` with AA lines, polylines, polygons,
   markers and arcs; glyph cache for X11 or chart-owned glyph caches;
   `CHART_ANTIALIAS` option. Acceptance: golden screenshots on X11, Wayland and
   Win32.
4. Autoscale, tick refinement, axis descriptions for all four axes and the
   legend with click-to-toggle. Acceptance: golden screenshots and the tick
   enumerator matching the drawn ticks.
5. Interaction. Cursor-anchored wheel zoom, drag pan, double-click fit,
   keyboard reset, cursor readout through `SEL_CHANGED`, and the `memmove`
   pan optimisation. Acceptance: wheel and drag gate scripts on X11 and
   Wayland.
6. Decimation, follow mode and the live mode in `charttest`. Per-column
   min/max, view-range recomputation, streaming follow with hysteresis, the
   ring-backed `FXData`, `LiveSource`, the mouse-speed/acceleration series and
   the portable system-metric seam. Acceptance: `charttest --live` runs at
   20 Hz and 60 Hz, follow scrolls without rescaling on a spike, memory stays
   flat over a long run, and the perf test meets the frame budget.
7. Application export. `ChartSVGExporter` from the public data/geometry API,
   SVG golden files, CSV round trip, and a PNG save check. Acceptance: SVG
   axis ranges and series colours match the screen; the widget contains no SVG
   code.
8. Optional. Right-hand Y axis polish; a second X column chooser; a 3D
   rotatable view, and a GL renderer behind `FXChartRenderer` only if the perf
   gate requires it.

## Verification

- Screenshots under `screenshots/tests/fox2/x11/`, `.../wayland/` and
  `.../win32/` (`charttest-static.png`, `charttest-live.png`,
  `charttest-viewmenu.png`, `charttest-series-toggle.png`,
  `charttest-live-series-toggle.png`, `charttest-live-scrolled.png`,
  `charttest-zoom-wheel.png`, `charttest-zoom-band.png`,
  `charttest-zoom-rect.png`, `charttest-live-zoom.png`,
  `charttest-reset-zoom.png`, `charttest-dark.png`, `charttest-dark-live.png`,
  `charttest-axis-fade.png`,
  `chartdemo-csv.png`, `chartdemo-xlsx.png`,
  `chartdemo-ods.png`, `chart-aa.png`, `chart-legend.png`, `chart-zoomed.png`).
- Win32 antialiased curve capture: `screenshots/tests/fox2/win32/charttest-antialias.png`.
- Import fixtures `fox2/apps/chart/data/sample.csv`, `sample.xlsx` and
  `sample.ods` with the same columns and values, plus an import check mode that
  prints or asserts the table shape, headers and numeric columns without a
  window. It runs in the X11 build so a parser regression fails before the
  screenshot comparison.
- A Wayland chart gate script following `scripts/gate-wayland-*.sh`, including a
  wheel and drag injection through `tools/xinject`/`wlxinject`.
- A live-mode gate: start `charttest --live`, move the pointer across the chart
  with `xinject`/`wlxinject`, and check that the mouse series advance, the
  follow window scrolls, and the process memory stays flat over a few thousand
  samples. A `--live-samples N --exit` mode feeds the same source for the perf
  job and exits without user interaction.
- A perf test binary that times a full repaint at 8 x 100,000 points and fails
  above the budget.
- Golden SVG files produced by `chartdemo` and compared as text after
  normalising coordinates, so a change in decimation or style is visible in
  review. The golden files live with the application, not with the widget.
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
| Adding Cairo or librsvg | New dependency for an application-side SVG writer; the serialiser is small and exact. |
| OpenXLSX, libxlsxio or libods | New C++ stack for a demo; a small ZIP reader plus the existing `FXXML` covers the numeric-cell subset. |
| libxml2 for XLSX/ODS | `fox2` already ships `FXXML`; a second XML parser buys nothing. |
| LibreOffice or `ssconvert` conversion step | Needs an office suite at runtime and loses column fidelity; not a demo feature. |
| SVG serialiser inside the widget | Blurs ownership and adds a second renderer that must match the screen; the widget exposes data and geometry instead. |
| Live view inside `chartdemo` | Mixes file viewing with widget stress testing; `charttest` is already the regression harness and links the same `foxchart` library. |

## Open questions

- Live metrics: which system values are portable enough to ship? CPU load and
  memory need a platform seam; mouse speed, acceleration, event rate and
  repaint time are portable. What default sample rate and history length?
- X column policy: is "first numeric column" right, or should a numeric first
  column also remain a series? What should the default be when the first column
  holds labels?
- Header detection: "first row has a non-numeric cell" is a heuristic; a
  `--no-header` / UI override is planned.
- Date and time: XLSX and ODS dates are serial numbers. Keep them numeric for
  now, or add a date axis in the same phase?
- Multi-sheet workbooks: first worksheet/sheet only in the first milestone;
  later a sheet chooser.
- Module or app: does another fox2 application need the widget in this phase?
  Affects header placement, not the implementation.
- Should antialiased text match the Xft output pixel for pixel, or is "visually
  the same and metric-compatible" enough for the gates?
- Follow-mode defaults (window length, hysteresis) and whether follow should
  stop on user pan (it should, and resume on request).
- Third axis: right-hand Y only, or a second plot area with an independent
  scale.
