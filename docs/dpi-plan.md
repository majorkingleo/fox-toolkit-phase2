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
    DPI during `init()` and is updated by DPI-change events.
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
2. updates the theme scale and relayouts when `scale=auto` is active,
3. applies the suggested top-level rectangle from `lParam`.

`tests/dpi simulate` sends a synthetic 144-DPI notification so the path can be
checked on a 96-DPI machine:

```
  after WM_DPICHANGED (144 dpi):
    theme scale       : 1.50
    window DPI (FOX)  : 144
    window bounds     : 1020,536 780x480
  after WM_DPICHANGED (96 dpi):
    theme scale       : 1.00
    window DPI (FOX)  : 96
    window bounds     : 1020,536 520x320
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

## Remaining work

### 5. DPI-aware caches and fonts

- Normal and role fonts are rescaled and rebuilt when the display scale
  changes; application font overrides are preserved through the base
  description.
- Remaining: per-window font realization for mixed-DPI monitors, and keying
  icons/bitmaps by DPI so a 96-DPI bitmap is not reused on a 192-DPI monitor.
  `FXFont::match()` currently derives the pixel height from a screen-compatible
  DC (`CreateCompatibleDC(nullptr)`), so a per-window font needs an explicit
  monitor DPI passed into font creation, plus a cache keyed by `(role,dpi)`.

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
| Mixed-DPI monitor move | code path in place, not measured |
| X11 build unaffected | unchanged code paths |

## Open decisions

- **Logical mode default.** The plan keeps pixel mode the default and makes
  logical units opt-in. Decide before changing any default geometry.
- **App-wide versus per-window scale.** Theme metrics are currently app-wide
  (`FXTheme::scale`); mixed-DPI monitors need a per-window scale. The theme
  remains the source of base metrics, with the window scale applied on top.
- **Font updates.** Recreating fonts on `WM_DPICHANGED` must not lose
  application-provided font overrides.
