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
  `FXID`, `FXint`, `FXString` and value structs cross the seam; platform handles
  travel as opaque `void*` or as `FXID`, never as `HWND`, `Display*` or `XID`.
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

Windows (`scripts/build-fox2-mingw.sh`):

- `hello` and `button` build and run; `hello` stays in its event loop.
- `tests/dpi simulate`: per-monitor-v2, system DPI 96, scale 1.00, 96-DPI
  window, then 1.50 and 780x480 at 144 DPI, then back to 1.00 and 520x320 —
  identical to the values before the extraction.
- `--enable-foxtrace` build compiles and traces
  `DPI awareness: policy=permonitorv2 effective=4` from the Windows module.
- Only `FXApp_win32.cpp` is compiled on Windows; `FXApp_x11.cpp` is not.

Linux host, cross build (`scripts/build-fox2-mingw-cross.sh --run`):

- `autoreconf -fi` plus `--host=x86_64-w64-mingw32 --without-x` builds
  `libFOX-1.7-0.dll`, the import library and `hello`, `button`, `dpi`;
  `X_BASE_LIBS` is reduced to the Win32 libraries and `FXApp_win32.cpp` is the
  module compiled in.
- Wine starts all three programs. `dpi` reports per-monitor-v2, system DPI 96
  and window DPI 96 — the same values as the native Windows session.
- The binaries import the MinGW runtime DLLs, so `WINEPATH` carries both the
  library directory and `/usr/x86_64-w64-mingw32/bin`; a failing start is
  retried once with Wine diagnostics.

Linux/X11 (`scripts/build-fox2-linux.sh`):

- `autoreconf -fi` plus an out-of-tree build in `tmp/build-fox2-linux`
  succeeds; only `FXApp_x11.cpp` is compiled, `FXApp_win32.cpp` is not.
- The library exports the three seam functions from the X11 module:
  `FX::fxplatformSetDPIAwareness(FXString const&)`,
  `FX::fxplatformGetSystemDPI()`, `FX::fxplatformGetWindowDPI(unsigned long)`.
- `hello`, `button` and `dpi` build and stay in their event loops.
- `dpi` reports `process awareness: 0 (unknown)`, `system DPI: 0`,
  `window DPI (FOX): 0`, `theme scale: 1.00` — the documented X11 behavior;
  the display scale comes from `[display] scale=` instead.
- `FXApp.cpp` no longer contains any DPI API typedef, function pointer or
  platform include for this operation.

Portability fixes needed by the X11 build (behavior-neutral):

- `FXApp::applyWindowDPI()` is portable tree walking, so its declaration moved
  out of the `#if defined(WIN32)` block in `include/FXApp.h`; the enclosing
  `#ifdef WIN32` blocks became `#if defined(WIN32)` per the porting rules.
- The two seam calls in `FXApp::init()` moved ahead of the Windows-only block,
  so both modules are exercised; X11 returns unknown/0 and `init()` behaves as
  before.

## Seam 1: native window creation, attaching and destruction

Done. The operation is the native window handle itself; everything else about a
window stays where it is for now.

| Part | Location |
| --- | --- |
| Internal contract | `fox2/lib/fxplatformwindow.h`: `FXWindowCreateInfo`, `fxplatformCreateWindow()`, `fxplatformAttachWindow()`, `fxplatformDestroyWindow()` |
| Windows module | `fox2/lib/FXWindow_win32.cpp`: frame style from the shell, top-window and override-redirect flags, parent/owner selection, XDND property, initial show state, `SetParent()` on attach, `GetDC()`/`ReleaseDC()`/`GetClass()` |
| X11 module | `fox2/lib/FXWindow_x11.cpp`: event masks, colormap, gravity, save-under and override-redirect attributes, transient-for hint, `WM_COLORMAP_WINDOWS` bookkeeping, initial map, `GetClass()` returning null |
| Moved out of the core | the X11 event mask macros to `fxpriv.h`; `addColormapWindows()` and `remColormapWindows()` into the X11 module |
| Portable core | `FXWindow::create()`, `attach()` and `destroy()` fill the descriptor, keep the DPI sample with `applyTheme()`, the xid hash and the ownership flag |

The descriptor carries only `FXID`, `FXint`, `FXString`-free values and the
opaque handles the platform already owns: display, instance, window class and
user data. `FXVisual` is referenced by pointer and read through its public
`getVisual()`/`getDepth()` accessors.

Deliberately left in `FXWindow.cpp` for now:

- show, hide, move, position, resize, reparent, raise and lower, which belong to
  the placement step,
- cursor handling and window shape,
- `scale()`/`unscale()`, which already use the DPI sampled here,
- drag-and-drop handling, which belongs to the event-translation seam.

`GetClass()` is now declared for both platforms; it existed only on Windows
because X11 has no window classes.

### Verification

- Linux/X11 build: `hello`, `button` and `dpi` build and stay in their event
  loops; `dpi` still reports `unknown`, system DPI 0, window DPI 0, scale 1.00.
  Screenshots are in `screenshots/tests/fox2/x11/`.
- Cross build plus Wine: `hello`, `button` and `dpi` build and run;
  `dpi` still reports per-monitor-v2, system DPI 96 and window DPI 96.
- `FXWindow.cpp` contains no `CreateWindowEx`, `XCreateWindow`, `DestroyWindow`,
  `XDestroyWindow`, `DefaultColormap` or `SetParent`; only `reparent()` keeps a
  placement block, which the next step removes.
- Only `FXWindow_x11.cpp` is compiled on Linux and only `FXWindow_win32.cpp` on
  Windows and in the cross build.

## Seam 2: placement and visibility

Done. Show, hide, move, position, resize, reparent, raise and lower now go
through the same module pair as creation.

| Part | Location |
| --- | --- |
| Internal contract | `fxplatformwindow.h`: `fxplatformPlaceWindow()`, `fxplatformShowWindow()`, `fxplatformReparentWindow()`, `fxplatformRaiseWindow()`, and `fxplatformReleaseMouseGrab()`/`fxplatformReleaseKeyboardGrab()` for the grab that must be released before an unmap |
| Windows module | `FXWindow_win32.cpp`: one `SetWindowPos()` with `SWP_NOMOVE`/`SWP_NOSIZE` derived from the placement flags, `ShowWindow()`, `SetParent()`, `HWND_TOP`/`HWND_BOTTOM`, `ReleaseCapture()` plus cursor restore |
| X11 module | `FXWindow_x11.cpp`: `XMoveWindow()`, `XResizeWindow()`, `XMoveResizeWindow()`, map/unmap around zero-sized windows, `XReparentWindow()` with flush, `XRaiseWindow()`/`XLowerWindow()`, `XUngrabPointer()`/`XUngrabKeyboard()` |
| Portable core | `FXWindow::move()`, `position()`, `resize()`, `show()`, `hide()`, `reparent()`, `raise()`, `lower()` keep the bookkeeping and call the seam |

The old-size and shown-state arguments to `fxplatformPlaceWindow()` preserve the
X11 rule that a zero-sized window is unmapped and mapped again when it grows;
Windows ignores them.

Deliberately left in `FXWindow.cpp` for now:

- `enable()` and `disable()`, which change the input state and belong with the
  event seam,
- `scroll()`, which is a drawing operation,
- pointer and keyboard *grab* acquisition, only the release ahead of an unmap
  moved here.

### Verification

- Linux/X11: `hello`, `button` and `dpi` run; the captured screenshots are
  byte-identical to the ones from seam 1, so nothing moved on screen.
- Cross build plus Wine: `hello`, `button` and `dpi` run; DPI values unchanged.
- `FXWindow.cpp` contains no `SetWindowPos`, `ShowWindow`, `SetParent`,
  `XMoveWindow`, `XResizeWindow`, `XMoveResizeWindow`, `XMapWindow`,
  `XUnmapWindow`, `XReparentWindow`, `XRaiseWindow` or `XLowerWindow`.

## Seam 3: the native event queue

Done. The whole queue — records, fetch and peek — now lives in the platform
modules; only message translation is left.

| Part | Location |
| --- | --- |
| Private records | `fox2/lib/fxapppriv.h`: `FXCBSpec`, `FXTimer`, `FXSignal`, `FXChore`, `FXInput`, `FXRepaint`, `FXHandles`, `FXInvocation` — the types behind the opaque declarations in `FXApp.h`, with the platform handle table in `FXHandles` and the shared `MAXSIGNALS` |
| Windows module | `FXApp_win32.cpp`: `FXApp::getNextEvent()` around `PeekMessage()`/`MsgWaitForMultipleObjects()` with timer, signal and I/O handling; `FXApp::peekEvent()` using `PM_NOREMOVE` |
| X11 module | `FXApp_x11.cpp`: `FXApp::getNextEvent()` around `XEventsQueued()`, `pselect()` on the display and input descriptors, repaint/chore/update idle work and the motion, wheel and configure compression; `FXApp::peekEvent()` with a zero-timeout poll |

The records had to move first: they are declared as opaque types in `FXApp.h`
and defined in `FXApp.cpp`, so a platform module could not touch `timers`,
`chores`, `handles` or `repaints` without them.

### Verification

- X11 and cross builds compile; `hello`, `button` and `dpi` run, DPI values are
  unchanged (`unknown/0/0/1.00` on X11, per-monitor-v2/96/96 under Wine).
- Both moved `getNextEvent()` bodies and both `peekEvent()` bodies diff
  identical against `HEAD`, so the move cannot change behavior.
- `hello`/`button` screenshots vary slightly between two runs of the same
  binary (cursor or paint timing), `dpi` is stable; the committed screenshots
  are therefore left untouched.

## Next seams, in order

1. **Message translation** (`FXApp.cpp`). `dispatchEvent()` and the `WM_*`
   handling including `WM_DPICHANGED` and `WM_DPICHANGED_AFTERPARENT`, which is
   what makes per-window metrics real; the enable/disable input state and grab
   acquisition belong here as well.
2. **Font realization** (`FXFont.cpp`). Keep the public `FXFont` API and move
   only face matching, metrics and glyph extents.
3. **Services** (`FXThread`, `FXPath`, `FXSystem`, `FXFile`, `FXProcess`,
   `FXIODevice`). Large but self-contained; split only when a feature needs it.
4. **Drawing** (`FXDCWindow`, `FXVisual`, `FXCursor`). Presentation seam; needed
   before a browser frontend and for the Wayland SHM surface.

Do not start a later seam before the earlier one is accepted on both builds.

## Constraints

- Two builds gate every seam step: the Win32 backend with
  `scripts/build-fox2-mingw.sh` on Windows, and the X11 backend with
  `scripts/build-fox2-linux.sh` on Linux. A step is accepted only when both
  compile and their test programs run; X11 modules must not be checked in
  unbuilt.
- The Win32 backend also cross-compiles on the Linux host with
  `scripts/build-fox2-mingw-cross.sh` (MinGW-w64, installed by
  `scripts/install-cachyos-mingw.sh`). That is the fast compile check, and
  Wine can start `hello`, `button` and `dpi` from it, but it does not replace
  the native Windows run for DPI behavior.
- Keep DPI behavior identical while extracting; `tests/dpi` output and the
  session log are the comparison.
