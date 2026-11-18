# Windows DPI awareness and scaling — plan and status

Implements workstream 2 of `docs/modernization-plan.md` on top of the `fox2/`
library. The Linux/X11 build is unaffected; all new code is guarded.

## Coordinate policy

Theme metrics are the single DPI-scaled quantity: `FXTheme::metric()` applies
the scale, so FOX-drawn padding, borders, spacing, and row heights grow with
the DPI. Window and widget coordinates remain device pixels for now;
`FXWindow::scale()`/`unscale()` are the one conversion point for moving input
or application geometry to logical units later.

Because only metrics scale, existing pixel geometry keeps its meaning, and at
96 DPI `scale=auto` is exactly 1.0.

## Step 1 — process awareness (done)

`FXApp::init()` establishes the process DPI awareness right after reading the
registry and before it queries the system font or creates any window.

### Policy setting

The requested policy is a FOX registry entry, so it can be changed per
application without rebuilding:

```
[SETTINGS]
dpiawareness=permonitorv2
```

| Value | Meaning |
| --- | --- |
| `permonitorv2` (default) | `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2` |
| `permonitor` | `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE` |
| `system` | `DPI_AWARENESS_CONTEXT_SYSTEM_AWARE` |
| `unaware` | `DPI_AWARENESS_CONTEXT_UNAWARE` |

An unknown value logs a warning and falls back to `permonitorv2`.

### Implementation

- `lib/FXApp.cpp`: `applyDPIAwareness()` resolves `SetProcessDpiAwarenessContext`,
  `GetThreadDpiAwarenessContext`, `GetAwarenessFromDpiAwarenessContext` and
  `AreDpiAwarenessContextsEqual` from `user32.dll`, and `SetProcessDpiAwareness`
  from `shcore.dll` at runtime, so older Windows versions and older SDK headers
  still compile and run.
- Fallback order: `SetProcessDpiAwarenessContext` → `SetProcessDpiAwareness`
  (`shcore`, Windows 8.1) → `SetProcessDPIAware` (Windows Vista).
- The effective state is queried back, not assumed. A manifest that already
  fixed the awareness wins; the reported level then reflects the manifest.
  If the effective level is lower than requested, a warning is emitted.
- `getSystemDPI()` uses `GetDpiForSystem` when available and falls back to
  `GetDeviceCaps(LOGPIXELSY)`.
- `FXApp` stores the result and exposes it:
  - `FXint getDPIAwareness() const` — one of `DPIAWARENESS_UNKNOWN`,
    `DPIAWARENESS_UNAWARE`, `DPIAWARENESS_SYSTEM`, `DPIAWARENESS_PER_MONITOR`,
    `DPIAWARENESS_PER_MONITOR_V2`.
  - `FXint getDPI() const` — system DPI, 0 if unknown.
- Recommended for applications: ship a per-monitor-v2 manifest. The dynamic
  call is the fallback for applications that cannot control their manifest.

### Diagnostic

`tests/dpi.cpp` builds a small window, prints the process awareness, system
DPI, theme scale, logical window bounds, the window DPI (`GetDpiForWindow`),
and the device rectangle, then closes itself after two seconds.

Result on this machine (100% scaling, MinGW-w64 build):

```
FOX DPI diagnostic
  process awareness : 4 (per-monitor-v2)
  system DPI        : 96
  theme scale       : 1.00
  window bounds     : 1220,686 120x19
  window DPI (FOX)  : 96
  window scale      : 1.00
  window DPI        : 96
  device rect       : 1212,655 136x58
```

## Step 2 — display scale and per-window DPI (done)

- `[display] scale` in the application registry controls the theme scale:
  - `auto` (Windows default): theme scale = DPI / 96. It starts from the system
    DPI during `init()`. It is the scale of the system DPI and of windows
    whose DPI is unknown; each window with a known DPI uses its own scale
    (step 5).
  - `<percent>`: fixed scale, for example `150` for 1.5.
  - Absent on X11: whatever scale the theme file supplied is kept.
  The registry value overrides a scale that came from a `.foxtheme` file.
- `FXApp::setThemeScale()` applies the value through the existing single
  conversion point (`FXTheme::metric()`), invalidates the style, repaints, and
  recalculates top-level layouts when the value actually changes.
- `FXWindow` records the monitor DPI when it is created (`GetDpiForWindow`,
  falling back to the system DPI) and exposes `getWindowDPI()`, `getScale()`,
  `scale()`, and `unscale()`. Conversion uses `MulDiv` on Windows and is the
  identity elsewhere.
- Coordinates are still device pixels; theme metrics and font sizes are
  scaled. This keeps existing application geometry compatible while making
  FOX-drawn chrome and text grow with the display scale.
- Font descriptions are scaled by `displayScale / (systemDPI / 96)`, so a
  manual percentage changes text size at 96 DPI, while a real 150% monitor
  with `scale=auto` does not scale text twice. `normalFont` is rebuilt in
  place, and cached role fonts are released and recreated with the scaled
  size when the scale changes; `applyTheme()` re-fetches them.
- `tests/dpi` is an interactive app: a slider from 70% to 150%, `-10%`,
  `+10%`, `100%` and `Auto` buttons, live awareness/DPI/scale readouts, and a
  sample area. The window also grows with the scale so the layout stays
  visible. Screenshots: `screenshots/tests/fox2/win32/dpi-100.png`,
  `dpi-150.png`, `dpi-200.png`, and `dpi-clicks*.png`.
- `[display] scale=150` at 96 DPI now scales both metrics and text; the
  simulated 144-DPI transition below scales the window to 780x480.

## Step 3 — runtime DPI changes (done)

`FXApp::dispatchEvent()` handles `WM_DPICHANGED` and
`WM_DPICHANGED_AFTERPARENT`:

1. records the new DPI on the window and its descendants,
2. re-applies fonts, padding and chrome for that window tree at the window
   scale and relayouts it (step 5; the app-wide scale is left alone; the factor
   is 1.0 unless the application called `setDisplayScaleFactor()`, which keeps
   a user zoom across monitors, whereas `setDisplayScale()` pins an absolute
   scale and disables following the DPI),
3. applies the suggested top-level rectangle from `lParam`.

`tests/dpi simulate` creates a second top-level window and sends a synthetic
DPI notification to the main window (144, or 96 when the system DPI is already
144), so the path can be checked on any machine. The second window must keep
its scale. Output on a 144-DPI system:

```
  before WM_DPICHANGED:
    main window       : scale 1.50  font height 25  bounds 780x480
    other window      : scale 1.50  font height 25  bounds 260x80
  after WM_DPICHANGED (96 dpi) on the main window:
    window DPI (FOX)  : 96
    main window       : scale 1.00  font height 15  bounds 520x320
    other window      : scale 1.50  font height 25  bounds 260x80
  after WM_DPICHANGED (144 dpi):
    window DPI (FOX)  : 144
    main window       : scale 1.50  font height 25  bounds 780x480
    other window      : scale 1.50  font height 25  bounds 260x80
```

A 200% manual scale was also checked through `[display] scale=200`
(`screenshots/tests/fox2/win32/dpi-200.png`); the window becomes 1040x640.

## Prerequisite — native Windows build of fox2 (done)

Build with `./scripts/build-fox2-mingw.sh --fast --run` inside MSYS2 MINGW64,
or `.\scripts\build-fox2-mingw.ps1 -Fast -Run` from PowerShell.

The copied subset lacked the sources that the Win32 paths reference. Added
from `fox-1.7.95/` and registered in `lib/Makefile.am`:

- `fxkeyboard.cpp` — modifier state, key-code mapping, `ToUnicodeEx`
- `FXRegion.cpp` — `FXRegion::bounds`, used by `FXDCWindow` on Windows
- `FXPopup.cpp`, `FXMenuPane.cpp` — the Windows event path references
  `FXMenuPane::metaClass`

`lib/Makefile.am` also gained the `COND_OSWINDOWS` block from the original
tree (`-DFOXDLL -DFOXDLL_EXPORTS -DWIN32 ...` and `-lmsimg32`), so the DLL
exports symbols and links `AlphaBlend`.

## Step 4 — input and geometry boundary audit (current regime)

Layout and input are both in device pixels, so no conversion is applied:

- `WM_*` mouse coordinates become `FXEvent` window/root coordinates unchanged;
  `findWindowAt()`, `containsChild()` and widget hit testing use the same
  values.
- Drag/drop positions, wheel scrolling and text measurement are device-based
  as well.
- The interactive test proves alignment: the sample button answers clicks at
  100% and 150% (`screenshots/tests/fox2/win32/dpi-clicks.png` and
  `dpi-clicks-150.png`).

Moving application-supplied geometry to logical units remains an opt-in
compatibility decision; if taken, `FXWindow::scale()`/`unscale()` is the single
conversion point at these boundaries.

## Step 5 — per-window metrics (design, then implemented)

Problem observed on a mixed-DPI desktop: a program with a main window and a
second top-level window (a help window, a dialog) is moved so that only the
main window enters the high-DPI monitor. `WM_DPICHANGED` updated the single
app-wide `FXTheme::scale`, which rebuilt the fonts and relaid out *every*
top-level window, so the second window, still on the low-DPI monitor, grew its
text as well.

### Model

- The app-wide scale (`FXTheme::scale`, `FXApp::getDisplayScale()`) is the
  scale for windows of unknown DPI and for the system DPI. It no longer follows
  `WM_DPICHANGED`.
- Every window has a *window scale*, `FXApp::getWindowScale(dpi)`:
  - `scale=auto`: `dpifactor * windowDPI / 96`, so each window follows the
    monitor it is on, and the user zoom (`setDisplayScaleFactor()`) is kept;
  - `scale=<percent>` or `setDisplayScale()`: the pinned app-wide scale for
    every window;
  - window DPI unknown (X11, Wayland for now): the app-wide scale.
- `FXTheme::metric(subject,scale)` applies an explicit scale to the base
  metric; `metric(subject)` keeps using the app-wide scale. The theme stays the
  source of base metrics, the window scale is applied on top.
- `FXWindow::getDisplayScale()` returns the window scale and
  `FXWindow::metric(subject)` the metric at that scale. `FXWindow::getScale()`
  keeps meaning raw `dpi/96`.

### What follows the window scale

- Fonts: `getRoleFont(role,dpi)` and `getNormalFont(dpi)` realize the font
  with `windowScale(dpi) / (dpi/96)` (that is, the user factor on the monitor's
  resolution), so the physical text size is proportional to the monitor DPI.
  Per-DPI normal fonts are persistent objects that are rescaled in place when
  the app-wide scale or the theme changes, so widgets can identify them.
  `FXApp::retargetNormalFont(font,dpi)` maps a normal font of any DPI to the
  normal font of another DPI and leaves application-supplied fonts alone,
  which keeps font overrides across a DPI change.
- Padding: `FXFrame` remembers which pads were `DEFAULT_PAD` and recomputes
  them from `FXWindow::metric(METRIC_PADDING)` when the theme or the window DPI
  is applied. Explicit pads are never touched.
- Chrome: `FXButtonState::scale` and `FXStyle::drawFocusRing(...,scale)` draw
  borders, corner radius and focus ring at the window scale.
- Widgets holding the normal font (`FXText`, `FXTextField`, `FXList`,
  `FXGroupBox`, `FXMenuCaption`, `FXStatusLine`, `FXToolTip`) re-resolve it in
  `applyTheme()`; labels already did so.

### Event handling

`WM_DPICHANGED` and `WM_DPICHANGED_AFTERPARENT` record the DPI on the window
and its descendants, call `FXApp::updateWindowScale(window)` (re-apply the
theme to that window tree, then relayout it) and apply the suggested
rectangle. Other top-level windows are not touched. Switching between
automatic and pinned scale, or changing the user factor, still refreshes all
windows.

### Not covered yet

Widgets with hard-coded pixel sizes that are not theme metrics (scroll bar
thickness, slider track, icons) keep their size on every monitor; they move to
metrics when the corresponding widgets are themed. Wayland reports no window
DPI yet, so it keeps the app-wide scale until the fractional-scale protocol
is wired in (`docs/wayland-plan.md`).

## Remaining work

### 5. DPI-aware caches and fonts (partly done)

- Normal and role fonts are rescaled and rebuilt when the display scale
  changes; application font overrides are preserved through the base
  description.
- Per-window font realization is in place:
  - `FXFont` carries an explicit `resolution` (dots per inch). `match()` uses
    it for `lf.lfHeight` and for `actualSize`, falling back to
    `GetDeviceCaps(LOGPIXELSY)` and then 96 when it is 0.
  - `FXApp` keeps a cache of font sets keyed by DPI (`FXApp::FXFontSet`), each
    holding a normal font and one font per theme role.
    `getRoleFont(role,dpi)` and `getNormalFont(dpi)` return the font realized
    for that monitor; passing 0 or the system DPI returns the global fonts.
  - `FXWindow::create()` records its monitor DPI and re-applies the theme, so
    labels realize the font for the monitor they were created on; `FXLabel`
    asks for `getRoleFont(FONT_UI,getWindowDPI())` at construction and on theme
    changes.
- Per-window metrics (step 5) remove the cancellation: the font point size is
  divided by `dpi/96` and the layout metrics use the window scale, so a window
  on a 150% monitor really is 1.5 times larger than one on a 100% monitor.
- Still open: keying bitmaps by DPI.  Icons are keyed by display scale:
  `FXIcon::getScaledIcon(scale)` keeps one copy per scale with the icon, and an
  `FXSVGIcon` rasterizes that copy again from its SVG document instead of
  filtering the authored pixels, so a vector icon stays sharp at 150% and 200%.
  The software raster backend (`FXRaster::drawIcon`, Wayland) now blends the
  icon's per-pixel alpha into the background, so the antialiased vector edges
  survive to the screen; X11 and Win32 still draw through a 1-bit shape mask.
  Bitmaps have no cache users yet; when they appear, key them as `(image,scale)`
  and rebuild on scale change.

### 6. Validation

- Run on a machine/monitor at 150% and 200% (this machine is 96 DPI); the
  simulated 144-DPI transition covers the code path but not real monitor
  metrics.
- Move a live window between monitors with different scale factors and verify
  size, fonts, metrics, hit targets, and pointer alignment.
- Check that `scale=auto` at 96 DPI remains pixel-identical for the existing
  examples.

| Check | Status |
| --- | --- |
| 100% scaling, per-monitor-v2 | verified (`tests/dpi`) |
| Simulated 144-DPI transition | verified (`tests/dpi simulate`), round trip back to 96 |
| Interactive 70–150% scaling | verified (`tests/dpi`, screenshots) |
| 150% theme scale (manual) | verified (`[display] scale=150`) |
| 200% theme scale (manual) | verified (`dpi-200.png`) |
| Pointer hit testing at 100% and 150% | verified (`dpi-clicks*.png`) |
| 150% / 200% real monitor | not yet run (this machine is 96 DPI) |
| Per-monitor font realization | implemented per window (step 5), exercised by `tests/dpi simulate` |
| Mixed-DPI monitor move | simulated: only the notified window changes; real monitors not measured |
| X11 build unaffected | unchanged code paths |
| Demo session log | verified (`dpi.exe simulate` and an interactive move/resize session) |
| FXTRACE captured to file | verified with `--enable-foxtrace` and `2>trace.log` (667 lines) |

### Capturing evidence on a real monitor

The test app writes its own session log, so a multi-monitor session can be
reviewed afterwards without a debugger.

- **Log file.** `dpi.exe` appends to `dpi-log.txt` in the current directory;
  set `FOX_DPI_LOG=<path>` to write elsewhere. Sessions are appended, so
  several runs can be compared in one file.
- **Recorded automatically.** Session start (cwd, interactive or simulate,
  trace environment), a `startup` snapshot, then a snapshot whenever the
  window settles after a move, resize or DPI change, and whenever the monitor
  layout changes. Each snapshot contains: process awareness, system DPI,
  theme scale with the auto flag, FOX and native window DPI, FOX client size
  and position, frame rectangle, the monitor table (name, primary flag, DPI,
  monitor rectangle, work area) and the monitor the window is on. A dedicated
  `window DPI changed A -> B` line marks a monitor transition.
- **Recorded on demand.** The "Snapshot" button writes a `manual snapshot`
  section; scale slider moves, the Auto button and sample-button clicks are
  logged as `user: ...` lines.
- **Sample line.**
  `2026-11-12 22:43:42.494 +22ms |   monitor[0] name=\\.\DISPLAY1 primary dpi=96 rect=0,0 2560x1440 work=0,0 2560x1392`

Suggested procedure on the mixed-DPI machine:

1. Build, then start `tmp/build-fox2-win/tests/dpi.exe` from a directory you
   can find again (the log lands there).
2. Press "Snapshot" on each monitor, and again after every move or resize that
   changes the DPI, so each state is labelled in the log.
3. Exercise the slider at 150% and "Auto" on each monitor, and click the
   sample button to record hit testing.
4. Close the window; the log ends with a `session end` section.

Expected values with `auto=yes`: `window_dpi_fox` equals the monitor DPI,
`theme_scale` (the window scale in the snapshot) becomes 1.500 on a 150% monitor and 2.000 on a 200% monitor, and
`window_size` grows from 520x320 to 780x480 and 1040x640.

**FOX trace messages to a file.** `FXTRACE` is compiled out when `NDEBUG` is
defined, and the default Windows build defines it for the library, so a trace
build is required:

- `.\scripts\build-fox2-mingw.ps1 -Trace -Clean` (or
  `./scripts/build-fox2-mingw.sh --trace`), which passes
  `--enable-foxtrace` to `configure`; `lib/Makefile.am` then omits `-DNDEBUG`.
- Enable topics at run time with `-tracelevel N`, `-tracetopics 0:1023`, or
  the `FOX_TRACE_LEVEL` / `FOX_TRACE_TOPICS` environment variables.
- Capture stderr, which also carries `fxmessage`, `fxwarning` and `fxerror`:
  `dpi.exe simulate -tracelevel 1023 2>trace.log`. Keep the argument order so
  `simulate` stays first.
- On Windows the same text also goes to `OutputDebugString`, so DebugView can
  capture it; `fxwarning` and `fxerror` additionally open a message box, and
  `fxerror` then breaks into the debugger.
- The trace build replaces the artifacts in `tmp/build-fox2-win`; rebuild
  without `--trace` before normal runs.

### 7. Next actions, in order

1. **Per-window metrics.** Implemented (step 5): fonts, default padding,
   chrome and the scroll bar thickness (`FXApp::getScrollBarSize()` is the
   96-DPI size, each bar scales it at its window; `setBarSize()` pins a bar).
   Remaining: hard-coded pixel sizes in sliders, icons and `FXPacker` spacing,
   and Wayland window DPI.
2. **DPI-keyed icon/bitmap caches.** Icons: done, `FXIcon::getScaledIcon()` keeps one
   scaled copy per display scale with the icon (a single-size source is scaled, no multi-size icon yet);
   an `FXSVGIcon` rasterizes its copy again from the document, so vector icons stay sharp.
   Bitmaps are still open.
3. **Real monitor validation.** Repeat the `tests/dpi` checks on a 150% and a
   200% monitor, then move a live window between monitors of different scale
   and confirm size, fonts, metrics, hit targets, and pointer alignment.

Current state: per-monitor font realization is committed (`fox2: realize fonts
per monitor DPI`); the build and the simulated 96→144→96 round trip pass, and
100%/150% screenshots are unchanged.

## Open decisions

- **Logical mode default.** The plan keeps pixel mode the default and makes
  logical units opt-in. Decide before changing any default geometry.
- **App-wide versus per-window scale.** Decided: the app-wide scale is the
  fallback and the system-DPI scale, each window derives its own scale from
  its monitor DPI in `scale=auto`, and a pinned scale applies to all windows.
- **Font updates.** Recreating fonts on `WM_DPICHANGED` must not lose
  application-provided font overrides.
