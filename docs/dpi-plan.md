# Windows DPI awareness and scaling — plan and status

Implements workstream 2 of `docs/modernization-plan.md` on top of the `fox2/`
library. The Linux/X11 build is unaffected; all new code is guarded.

## Coordinate policy

Public layout dimensions and theme metrics are **logical units**; the Windows
adapter converts to device pixels at the native boundary. Conversion stays in
one helper, and the same values are never scaled twice.

Logical units stay **opt-in** for now: the default theme scale remains 1.0, so
existing applications keep pixel-identical geometry. `[display] scale=auto`
opts into following the monitor DPI (see "Remaining work").

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
  window DPI        : 96
  device rect       : 1212,655 136x58
```

## Prerequisite — native Windows build of fox2 (done)

Build with `./scripts/build-fox2-mingw.sh --run` inside MSYS2 MINGW64, or
`.\scripts\build-fox2-mingw.ps1 -Run` from PowerShell.

The copied subset lacked the sources that the Win32 paths reference. Added
from `fox-1.7.95/` and registered in `lib/Makefile.am`:

- `fxkeyboard.cpp` — modifier state, key-code mapping, `ToUnicodeEx`
- `FXRegion.cpp` — `FXRegion::bounds`, used by `FXDCWindow` on Windows
- `FXPopup.cpp`, `FXMenuPane.cpp` — the Windows event path references
  `FXMenuPane::metaClass`

`lib/Makefile.am` also gained the `COND_OSWINDOWS` block from the original
tree (`-DFOXDLL -DFOXDLL_EXPORTS -DWIN32 ...` and `-lmsimg32`), so the DLL
exports symbols and links `AlphaBlend`.

## Remaining work

### 2. Centralized scale helper

- Add one internal conversion helper for logical ↔ device:
  `FXint FXWindow::scale(FXint)`, `FXint FXWindow::unscale(FXint)`, and a
  per-top-window scale.
- Store the effective DPI per top-level window (`FXWindow::dpi`), inherited by
  children; expose `FXint getWindowDPI() const` and `FXdouble getScale() const`.
- Wire `[display] scale=auto|<percent>`: `auto` derives the theme scale from
  the window DPI (96 → 1.0, 120 → 1.25, 144 → 1.5, 192 → 2.0); a percent
  keeps today's manual override. This is the opt-in logical mode.

### 3. Runtime DPI transitions

- Handle `WM_DPICHANGED` in the top-level window procedure; skip it for child
  windows and use `WM_DPICHANGED_AFTERPARENT` only if needed.
- Apply `lParam`'s suggested rectangle to the top-level window, update the
  window scale, recreate role fonts and DPI-dependent resources, then
  `relayout()` descendants and repaint.
- Keep an app-wide fallback when the DPI notification is unavailable.

### 4. Input and geometry boundary audit

- Window placement and client size, pointer coordinates, hit testing,
  drag/drop, scroll increments, text measurement, border widths, cursors, and
  image/icon sizing must all go through the same helper.
- Normalize input to logical space before dispatch; never mix device and
  logical values in one computation.

### 5. DPI-aware caches

- Key fonts, icons, and bitmaps by the DPI they were realized at; do not reuse
  a 96-DPI bitmap on a 192-DPI monitor.
- `FXFont` currently derives its pixel height from the DC DPI at creation
  time; it must be recreated when the window scale changes.

### 6. Validation

| Check | Status |
| --- | --- |
| 100% scaling, per-monitor-v2 | verified (`tests/dpi`) |
| 150% and 200% scaling | not yet run on this machine |
| Move between mixed-DPI monitors | not implemented yet |
| Pointer hit testing aligned | not measured yet |
| X11 build unaffected | unchanged code paths |

## Open decisions

- **Logical mode default.** The plan keeps pixel mode the default and makes
  logical units opt-in. Decide before changing any default geometry.
- **App-wide versus per-window scale.** Theme metrics are currently app-wide
  (`FXTheme::scale`); mixed-DPI monitors need a per-window scale. The theme
  remains the source of base metrics, with the window scale applied on top.
- **Font updates.** Recreating fonts on `WM_DPICHANGED` must not lose
  application-provided font overrides.
