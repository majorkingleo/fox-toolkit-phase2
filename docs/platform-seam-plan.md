# Platform seam plan

Workstream 3 in `docs/modernization-plan.md`: split platform responsibilities out
of the portable FOX core one narrow operation at a time, selected at compile
time. This document records the inventory, the rules, the pilot, and the next
operations in order.

## Rules

- One narrow operation per step; build and run the existing examples before the
  next one. No broad mechanical extraction.
- Platform modules are selected at compile time (`COND_OSWINDOWS` and its
  inverse in `fox2/lib/Makefile.am`); one implementation is compiled per build.
  No plugins, no runtime loading.
- Internal contracts live in private headers and never use platform types. Only
  `FXID`, `FXint`, `FXString` and value structs cross the seam.
- Public headers stay free of operating-system handles.
- Platform files follow the repository naming rule: `<Class>_win32.cpp`,
  `<Class>_x11.cpp`, shared behavior in `<Class>.cpp`.
- Do not reformat code while moving it; a seam step should be behavior-neutral.

## Inventory

Platform conditional blocks per file, counted as `defined(WIN32)` occurrences in
`fox2/lib`:

| File | Blocks | Responsibilities |
| --- | --- | --- |
| `FXThread.cpp` | 34 | Threads, mutexes, condition variables, TLS, CPU and scheduling queries |
| `FXApp.cpp` | 27 | Display/session setup, event queue and translation, timers, clipboard, DDE/drag reply, DPI (pilot) |
| `FXFont.cpp` | 21 | Font realization, enumeration, metrics, glyph extents |
| `FXPath.cpp` | 20 | Home/temp/data directories, path separators, shortcuts |
| `FXSystem.cpp` | 18 | Environment, time and timezone, host and user names, memory info |
| `FXProcess.cpp` | 11 | Process start, pipes, wait, exit codes |
| `FXFile.cpp` | 10 | File descriptors, mapping, locking, watchers |
| `fxutils.cpp` | 9 | Message and error output, assertion handling, tracing |
| `FXIODevice.cpp` | 9 | Non-blocking device waits, serial and socket behavior |
| `FXMutex.cpp` | 5 | Lock implementation |
| `FXAutoThreadStorageKey.cpp` | 4 | TLS key allocation |
| `FXWindow.cpp` | 3 | Native window creation, shell styles, cursor, DPI use |
| `FXDir.cpp` | 2 | Directory enumeration |
| `FXCursor.cpp` | 2 | Cursor creation and stock cursors |
| `FXVisual.cpp` | 1 | Visual and colormap setup |
| `FXDCWindow.cpp` | 1 | Platform drawing entry points |
| `FXDC.cpp`, `FXDrawable.cpp`, `FXImage.cpp`, `FXIcon.cpp` | 0 | Already portable |

`fxpriv.h` and `xincs.h` choose platform headers and must stay private.

## Pilot: DPI awareness and DPI queries

Done. The operation chosen is the smallest one the DPI workstream already
depends on.

| Part | Location |
| --- | --- |
| Internal contract | `fox2/lib/fxplatform.h`: `fxplatformSetDPIAwareness()`, `fxplatformGetSystemDPI()`, `fxplatformGetWindowDPI()` |
| Windows module | `fox2/lib/FXApp_win32.cpp`: policy parsing and application with fallbacks, awareness verification, `GetDpiForSystem`/`GetDeviceCaps`, `GetDpiForWindow`; all functions resolved dynamically so old SDKs and runtimes still build and run |
| X11 module | `fox2/lib/FXApp_x11.cpp`: no process awareness, no system DPI, no per-window DPI; the display scale is configured instead |
| Build | `lib/Makefile.am`: exactly one module is added to `libFOX_1_7_la_SOURCES`; `fxplatform.h` is in `noinst_HEADERS` |
| Portable core | `FXApp::init()` calls the seam; `FXApp::getWindowDPI()` keeps the app-DPI fallback; `applyWindowDPI()` keeps walking the widget tree |

Deliberately left in `FXApp.cpp` for now:

- the `DPIAWARENESS_*` enum, which is public API,
- `WM_DPICHANGED` / `WM_DPICHANGED_AFTERPARENT` handling, which belongs with the
  event-translation seam, not with the queries,
- theme scaling and font realization, which are presentation, not platform.

There is no `fox2/windows` project to keep in sync; the Visual Studio projects
in `fox-1.7.95/windows` build the original tree. When a Windows project for
`fox2/` is added, it must list the same platform module.

### Verification

- `hello` and `button` build and run; `hello` stays in its event loop.
- `tests/dpi simulate`: per-monitor-v2, system DPI 96, scale 1.00, 96-DPI
  window, then 1.50 and 780x480 at 144 DPI, then back to 1.00 and 520x320 —
  identical to the values before the extraction.
- `--enable-foxtrace` build compiles and traces
  `DPI awareness: policy=permonitorv2 effective=4` from the Windows module.
- Only `FXApp_win32.cpp` is compiled on Windows; `FXApp_x11.cpp` is not.
- `FXApp.cpp` no longer contains any DPI API typedef, function pointer or
  platform include for this operation.

## Next seams, in order

1. **Native window creation and shell styles** (`FXWindow.cpp`). Smallest of the
   remaining window operations and the prerequisite for monitor-aware geometry:
   creation, show/hide, placement, cursor and frame style.
2. **Event queue and message translation** (`FXApp.cpp`). Move `dispatchEvent`
   and `WM_*` handling, including `WM_DPICHANGED`, next to the window seam. This
   is what enables per-window metrics, because the window's monitor DPI arrives
   with these messages.
3. **Font realization** (`FXFont.cpp`). Keep the public `FXFont` API and move
   only face matching, metrics and glyph extents.
4. **Services** (`FXThread`, `FXPath`, `FXSystem`, `FXFile`, `FXProcess`,
   `FXIODevice`). Large but self-contained; split only when a feature needs it.
5. **Drawing** (`FXDCWindow`, `FXVisual`, `FXCursor`). Presentation seam; needed
   before a browser frontend and for the Wayland SHM surface.

Do not start a later seam before the earlier one is accepted on both builds.

## Constraints

- This machine builds and runs only the Windows backend. X11 modules are moved
  mechanically and must be compiled and run on Linux before their seam is
  considered accepted.
- Keep DPI behavior identical while extracting; `tests/dpi` output and the
  session log are the comparison.
