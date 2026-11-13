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

## Seam 4: flush and repaint

Done. Small, but dispatch and every drawing path call into it, so it had to come
before message translation.

| Part | Location |
| --- | --- |
| Windows module | `FXApp_win32.cpp`: `FXApp::flush()` via `GdiFlush()`, `FXApp::repaint()` looping `RedrawWindow(RDW_ERASENOW\|RDW_UPDATENOW\|RDW_ALLCHILDREN)` over the top-level windows |
| X11 module | `FXApp_x11.cpp`: `FXApp::flush()` via `XSync()`/`XFlush()`, `FXApp::repaint()` via `removeRepaints(0,0,0,0,0)` |
| Portable core | `FXApp::forceRefresh()` and `FXApp::refresh()` are unchanged; they were already platform free |

Both functions are member definitions in the platform file, so no new contract
is needed in `fxplatform.h`.

### Verification

- X11 and cross builds compile; `hello`, `button` and `dpi` run with unchanged
  DPI values.
- The moved bodies are the exact arms of the former `#if defined(WIN32)` blocks.

## Seam 5: the Windows window procedure

Done. `FXival CALLBACK FXApp::wndproc()` — the static thunk that forwards each
message to `FXApp::dispatchEvent()` — now lives in `FXApp_win32.cpp` instead of
`FXApp.cpp`. It is the entry point of message translation, so it goes first; the
`WM_*` handling it feeds is still in the core.

### Verification

- Cross build compiles and the Wine smoke test runs `hello`, `button` and `dpi`
  with per-monitor-v2, system DPI 96 and window DPI 96.
- X11 build is unaffected (the thunk is Windows-only) and still reports
  `unknown/0/0/1.00`.

## Seam 6: wait cursor and drag types

Done. Five small application operations that each carried a platform branch are
now whole members in the platform modules.

| Part | Location |
| --- | --- |
| Windows module | `FXApp_win32.cpp`: `FXApp::beginWaitCursor()`/`endWaitCursor()`/`setWaitCursor()` via `SetCursor()`, `registerDragType()` via `RegisterClipboardFormatA()`, `getDragTypeName()` via `GetClipboardFormatNameA()` |
| X11 module | `FXApp_x11.cpp`: the same three cursor functions walking the widget tree with `XDefineCursor()` and `XChangeActivePointerGrab()`, `registerDragType()` via `XInternAtom()`, `getDragTypeName()` via `XGetAtomName()` |
| Portable core | `FXApp::setDefaultCursor()` stays; the wait-cursor counter and nesting logic stay inside the moved members |

### Verification

- X11 and cross builds compile; `hello`, `button` and `dpi` run with unchanged
  DPI values.
- Statement order of the moved bodies matches the former `#else`/`#if` arms.

## Seam 7: signals and input handles

Done. The two registration APIs the event queue works on are now platform code.

| Part | Location |
| --- | --- |
| Windows module | `FXApp_win32.cpp`: `FXApp::addSignal()`/`removeSignal()` with `signal()` (and the `__IBMCPP__` cast), `FXApp::addInput()`/`removeInput()` over the `HANDLE` table with `MAXIMUM_WAIT_OBJECTS` |
| X11 module | `FXApp_x11.cpp`: `FXApp::addSignal()`/`removeSignal()` with `sigaction()` and the `_POSIX_SOURCE` fallback, `FXApp::addInput()`/`removeInput()` over the `fd_set` table with `FD_SET`/`FD_CLR` and the `maxhandle` trim |
| Portable core | `signalhandler()`, `immediatesignalhandler()`, the chore list and `leaveWindow()` stay |

### Verification

- X11 and cross builds compile; `hello`, `button` and `dpi` run with unchanged
  DPI values.
- The moved bodies are the former `#if defined(WIN32)`/`#else` arms with the
  branch tests removed.

## Seam 8: window and input queries

Done. Four small queries that each carried a platform branch moved as whole
members, and the trace topics moved into the private header so the modules can
use them.

| Part | Location |
| --- | --- |
| Windows module | `FXApp_win32.cpp`: `hasInputMethod()` (always true), `findWindowAt()` with `ChildWindowFromPointEx()`, `getKeyState()` via `GetKeyState()`, `beep()` via `MessageBeep()` |
| X11 module | `FXApp_x11.cpp`: `hasInputMethod()` from `xim`, `findWindowAt()` with `XTranslateCoordinates()`, `getKeyState()` via `XQueryKeymap()`, `beep()` via `XBell()` |
| Private header | `fxapppriv.h`: the `TOPIC_*` trace topics, moved next to `MAXSIGNALS` |
| Portable core | `findWindowWithId()`, `getFocusWindow()` and the widget-tree helpers stay |

`getForegroundWindow()` is dead code (`#if 0`) and was left untouched rather
than moved.

### Verification

- Fast builds (`--fast`, no configure rerun) recompile `FXApp.cpp` and the
  platform module, as the dependency files require.
- `hello`, `button` and `dpi` run on X11 and under Wine with unchanged DPI
  values.

## Seam 9: closing the display

Done. Session teardown is the first half of the session seam; `openDisplay()`
(~400 lines) is left for its own step.

| Part | Location |
| --- | --- |
| Windows module | `FXApp_win32.cpp`: `FXApp::closeDisplay()` releasing the DDE/XDND atoms with `GlobalDeleteAtom()` and the seventeen stipples with `DeleteObject()` |
| X11 module | `FXApp_x11.cpp`: `FXApp::closeDisplay()` freeing stipples and hatch patterns with `XFreePixmap()`, closing the input method and calling `XCloseDisplay()` |
| Portable core | the common prologue (`destroy()`, `closeInputDevices()`), clearing `display`, unlocking the mutex and clearing `initialized` stay in each module; `closeInputDevices()` itself is still in the core |

### Verification

- Cross build plus Wine: `dpi.exe simulate` now runs to completion instead of
  being killed, so teardown actually executes — it reports the 144-DPI step
  (scale 1.50, 780x480) then back to 96 DPI (520x320) and exits with status 0.
- X11: a short program that opens and closes the display (`tmp/t1-check.cpp`)
  prints `PASS` and exits 0, so `XCloseDisplay()` runs on the new path.
- Both builds used `--fast`; no configure input changed.

## Seam 10: window enable and disable

Done. The input state of a window, deferred from the placement step, now lives
with the window platform modules.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: `FXWindow::enable()`/`disable()` via `EnableWindow()` plus the mouse and keyboard grab bookkeeping on disable |
| X11 module | `FXWindow_x11.cpp`: the same two members via `XSelectInput()` with the basic, shell and enabled event masks, `XUngrabPointer()`/`XUngrabKeyboard()` on disable |
| Portable core | `killFocus()` and the `FLAG_ENABLED` bookkeeping stay in the members, which now live in the modules |

`grab()`, `ungrab()`, `grabKeyboard()` and `ungrabKeyboard()` — acquiring rather
than releasing — are still in `FXWindow.cpp` and come with the event seam.

### Verification

- Fast builds (`--fast`) on both backends; `hello`, `button` and `dpi` run with
  unchanged DPI values.

## Seam 11: event translation

Done. `FXApp::dispatchEvent()` — the translation of native events into FOX
messages — now lives in the platform modules. The two backends already had
separate definitions, so each moved as a whole instead of being split into
groups (paint/configure, pointer/keyboard, DPI/DND) with new helper functions.

| Part | Location |
| --- | --- |
| Windows module | `FXApp_win32.cpp`: `FXApp::dispatchEvent(FXRawEvent&)` (`TranslateMessage()`/`DispatchMessage()`) and `FXApp::dispatchEvent(FXID,FXuint,FXuval,FXival)` with all `WM_*` handling, including `WM_DPICHANGED` and `WM_DPICHANGED_AFTERPARENT`, the DND messages and the `WM_SYNCPAINT`/`WM_DPICHANGED*` fallbacks |
| X11 module | `FXApp_x11.cpp`: `FXApp::dispatchEvent(FXRawEvent&)` with the `XEvent` switch, `addRepaint()`/`removeRepaints()` compositing, `translateKeyEvent()`, the XInput2 and XRandR/XFixes handling |
| Portable core | `FXApp.cpp` keeps the timers, chores, selection bookkeeping, `onCmd*` handlers and the widget-tree helpers; it contains no event translation any more |

Both modules now include `FXComposeContext.h` and `fxpriv.h`, which the moved
bodies use (`fxmodifierkeys()`, `wkbMapKeyCode()`, `fxsenddata()`, the
`WM_DND_*` messages and the X11 event masks).

### Verification

- The removed lines of `FXApp.cpp` equal the added lines of the two modules,
  apart from the preprocessor guards and the added includes
  (`tmp/verify-move.sh`); the move is textual.
- Windows (`build-fox2-mingw.sh --cmake`) and X11 (`build-fox2-linux.sh --cmake`
  in WSL) compile; `hello`, `button` and `dpi` run on both, `dpi simulate`
  reports the same scale and window sizes.
- `FXApp.cpp` is down from 5217 to 3121 lines.

## Seam 12: pointer and keyboard grabs

Done. The four members that acquire and release grabs, left behind by the
enable/disable step, moved as whole functions next to `enable()`/`disable()`.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: `grab()`/`ungrab()` via `SetCapture()`/`ReleaseCapture()` plus the drag and default cursors, `grabKeyboard()` via `SetActiveWindow()`, `ungrabKeyboard()` (bookkeeping only) |
| X11 module | `FXWindow_x11.cpp`: `grab()` with `XGrabPointer()` and its `CurrentTime` retry, `ungrab()`, `grabKeyboard()` with `XGrabKeyboard()`, `ungrabKeyboard()` with `XUngrabKeyboard()` |
| Portable core | `grabbed()`, `grabbedKeyboard()` and `onUngrabbed()` stay in `FXWindow.cpp` |

`TOPIC_DETAIL` is defined locally in both modules, as `FXWindow.cpp` does.

### Verification

- Fast builds on both backends (Windows native, X11 in WSL); `hello`, `button`
  and `dpi` run with unchanged DPI values.
- `FXWindow.cpp` no longer calls `XGrabPointer()`, `XGrabKeyboard()`,
  `SetCapture()` or `SetActiveWindow()`.

## Seam 13: window cursor

Done. `showCursor()`, `setDefaultCursor()` and `setDragCursor()` moved as whole
members into the window platform modules.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: `ShowCursor()`, and `SetCursor()` guarded by `underCursor()`/`grabbed()` |
| X11 module | `FXWindow_x11.cpp`: `XDefineCursor()` with the default or blank cursor, `XChangeActivePointerGrab()` during a grab |
| Portable core | `cursorShown()`, `getDefaultCursor()`/`getDragCursor()` stay in `FXWindow.cpp` |

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values.

## Seam 14: window shape

Done. The three `setShape()` overloads and `clearShape()` moved as whole members
into the window platform modules; the Windows-only `makeregion()` helper moved
with them as a `static` function.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: `SetWindowRgn()` with a copied region, or one built from a bitmap or icon mask by `makeregion()` |
| X11 module | `FXWindow_x11.cpp`: `XShapeCombineRegion()`/`XShapeCombineMask()` inside the `HAVE_XSHAPE_H` guard |
| Portable core | nothing left; the argument checks are part of each member |

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. No example exercises shaped windows, so the new code is only
  compile-checked.

## Seam 15: pointer position and coordinate translation

Done. `getCursorPosition()`, `setCursorPosition()`, `translateCoordinatesFrom()`
and `translateCoordinatesTo()` moved as whole members into the window platform
modules. The `#if 0` portable fallback of the two translate members stays in
`FXWindow.cpp` untouched.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: `GetCursorPos()`/`SetCursorPos()` with `ScreenToClient()`/`ClientToScreen()` and `fxmodifierkeys()` for the button state |
| X11 module | `FXWindow_x11.cpp`: `XQueryPointer()`, `XWarpPointer()`, `XTranslateCoordinates()` |

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values.

## Seam 16: selection, clipboard and drag rectangle

Done. `acquireSelection()`, `releaseSelection()`, `acquireClipboard()`,
`releaseClipboard()`, `setDragRectangle()` and `clearDragRectangle()` moved as
whole members into the window platform modules, with the order of the
bookkeeping and the `SEL_*_LOST`/`SEL_*_GAINED` notifications unchanged.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: clipboard ownership via `OpenClipboard()`/`EmptyClipboard()`/`SetClipboardData()`/`GetClipboardOwner()`; no X selection type lists; drag rectangle in screen coordinates via `ClientToScreen()` |
| X11 module | `FXWindow_x11.cpp`: `XSetSelectionOwner()`/`XGetSelectionOwner()` for `XA_PRIMARY` and the clipboard selection, the `xselTypeList`/`xcbTypeList` bookkeeping, drag rectangle via `XTranslateCoordinates()` and `xdndWantUpdates` |
| Portable core | `hasSelection()`, `hasClipboard()` and the `on*Lost/Gained/Request` handlers stay in `FXWindow.cpp` |

On Windows the selection members no longer carry an empty `if(xid){}` block;
nothing else differs from the original bodies.

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. No example owns the clipboard or the selection, so the moved
  code is compile-checked only.

## Seam 17: invalidation and scrolling

Done. `update(x,y,w,h)`, `repaint(x,y,w,h)` and `scroll()` moved as whole members
into the window platform modules. The clipping prologue of `update()` and
`repaint()` is identical on both sides and is duplicated into each module; the
parameterless `update()` and `repaint()` stay in `FXWindow.cpp`.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: `InvalidateRect()`, `RedrawWindow()` with `RDW_UPDATENOW`, `ScrollWindowEx()` with `SW_INVALIDATE` |
| X11 module | `FXWindow_x11.cpp`: `addRepaint()`/`removeRepaints()`/`scrollRepaints()` on the repaint list, and `scroll()` with `XSync()`, pending `Expose` events, `XCopyArea()` and the uncovered-area rectangles |
| Portable core | `update()`, `repaint()` |

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. Every example paints through `update()`/`repaint()`, so these
  paths run at startup on both backends; `scroll()` is compile-checked only.

## Seam 18: drag and drop

Done. `dropFinished()`, `beginDrag()`, `handleDrag()` and `endDrag()` moved as
whole members into the window platform modules, about 500 lines of original
code. They were moved with a scratch script (`tmp/split-dnd.py`) that resolves
each member's `WIN32` branches per platform and copies everything else
verbatim, rather than by hand.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: the `WM_DND_*` messages posted with `PostMessage()`, the shared-memory type list (`CreateFileMappingA()`/`MapViewOfFile()`), drop-window lookup via `WindowFromPoint()` and `GetProp()`, and the `PeekMessage()` wait loops in `endDrag()` |
| X11 module | `FXWindow_x11.cpp`: the XDND protocol with `XSendEvent()` client messages, proxy and awareness lookup via `XGetWindowProperty()`, the `XDNDMatch` struct with its `matchxdnd()` predicate, and the `XCheckIfEvent()` wait loops |
| Portable core | `acceptDrop()`, `inquireDNDAction()`, `didAccept()`, `isDragging()` and `isDropTarget()` stay in `FXWindow.cpp` |

`TOPIC_EVENT` is defined locally in both modules, like `TOPIC_DETAIL`.

### Verification

- The removed lines of `FXWindow.cpp` equal the added lines of the two modules
  apart from the preprocessor guards (`tmp/verify-move-window.sh`); lines that
  both branches share are duplicated into both modules.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. No example starts a drag, so the moved code is compile-checked
  only.

## Seam 19: font matching, realization and enumeration

Done. About 1,700 lines of `FXFont.cpp` moved into the new
`FXFont_win32.cpp` and `FXFont_x11.cpp`, which are registered in
`fox2/lib/CMakeLists.txt` and `fox2/lib/Makefile.am` next to the window
modules. Everything that uses the face-name helpers moved together, because
the helpers are `static`: the Win32 charset converters, the Fontconfig
weight/slant/width converters, the XLFD parsers and `xlfdFont()` are used by
`match()`, `create()` and `listFonts()` alike. A scratch script
(`tmp/split-font.py`) resolved the three-way `WIN32` / `HAVE_XFT_H` / XLFD
branches per platform; the X11 module keeps the Xft/XLFD split internally.

| Part | Location |
| --- | --- |
| Windows module | `FXFont_win32.cpp`: `FXFont::match()` with `CreateFontIndirect()` and the `TEXTMETRIC` block, the `LOGFONT` charset converters, `create()`, `detach()`, `destroy()` (stock-font select, `DeleteObject()`, `DeleteDC()`), and `listFonts()` with `EnumFontFamiliesEx()` |
| X11 module | `FXFont_x11.cpp`: both `FXFont::match()` variants (Fontconfig and XLFD), the Fontconfig and XLFD helper tables, `XLoadQueryFont()`/`XftFontClose()`/`XFreeFont()` in `create()`, `detach()`, `destroy()`, and `listFonts()` for both variants |
| Portable core | constructors, `getFamily()`/`getFoundry()`, `setAngle()`, the text metrics and extents, the encoding name table and the font string parser stay in `FXFont.cpp` |

`comparefont()` is portable but has a single user, `listFonts()`, so it is
duplicated into both modules. `TOPIC_CREATION` and `TOPIC_DETAIL` are defined
locally in both modules, and the X11 module declares `__snprintf()` itself.

### Verification

- The removed lines of `FXFont.cpp` equal the added lines of the two modules
  apart from the `#if defined(WIN32)` / `#elif defined(HAVE_XFT_H)` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. Every example creates fonts, so Win32 `match()`/`create()` and
  the XLFD variant run; the WSL image has no Xft headers, so the Fontconfig
  variant is moved verbatim but not compiled, and `listFonts()` is
  compile-checked only.

## Next seams, in order

1. **Remaining `FXWindow.cpp` branches.** The `scale()`/`unscale()` DPI helpers,
   the drag-type constants (`stringType`, `imageType` and the `*TypeName`
   strings), and the commented-out `setBackColor()` stub. The constants are
   data rather than behavior; moving them needs a private header or a per-module
   definition, so decide that before touching them.
2. **Font metrics** (`FXFont.cpp`). Text metrics and glyph extents
   (`getTextWidth()`, `getTextHeight()`, `getFontAscent()`, `getTextExtent*()`
   and friends), which use `dc`/`font` per backend. Keep the public `FXFont`
   API; the surrogate-pair constants stay with the portable code.
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
- On a Windows host the X11 backend is built in WSL (Ubuntu with `g++`, `cmake`,
  `ninja`, `libx11-dev`; WSLg provides the display):

  ```bash
  FOX_BUILD_DIR=$HOME/build/fox2-cmake ./scripts/build-fox2-linux.sh --cmake --run
  ```

  The build directory must be on the WSL file system: CMake's try-compile fails
  with `Permission denied` below `/mnt/<drive>`. A full build takes under 20 s.
  Docker would add reproducibility (a pinned distro, Xvfb for headless runs)
  but not coverage beyond this, so it is only worth it for CI.
- Keep DPI behavior identical while extracting; `tests/dpi` output and the
  session log are the comparison.
