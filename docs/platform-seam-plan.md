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

## Seam 20: font metrics

Done. The 15 metric members of `FXFont.cpp` moved as whole members into
`FXFont_win32.cpp` and `FXFont_x11.cpp`, again with a scratch script
(`tmp/split-font-metrics.py`) that resolves the `WIN32` / `HAVE_XFT_H` / XLFD
branches per platform.

| Part | Location |
| --- | --- |
| Windows module | `FXFont_win32.cpp`: `hasChar()`, `getMinChar()`, `getMaxChar()`, `getFontLeading()`, `getFontSpacing()`, `leftBearing()`, `rightBearing()`, `isFontMono()`, `getFontWidth()`, `getFontHeight()`, `getFontAscent()`, `getFontDescent()`, `getCharWidth()`, `getTextWidth()` and `getTextHeight()` from the `TEXTMETRIC` block and `GetTextExtentPoint32W()` on the dummy DC, plus the surrogate-pair constants `LEAD_OFFSET` and `TAIL_OFFSET` |
| X11 module | `FXFont_x11.cpp`: the same members from `XftFont` with `XftTextExtents*()` or from `XFontStruct` and its `per_char` table |
| Portable core | the `FXString` overloads of `getTextWidth()` and `getTextHeight()`, constructors, `setAngle()`, the encoding name table and the font string parser stay in `FXFont.cpp` |

The `dc=nullptr` initialization in the four constructors is still a
`#ifdef WIN32` in `FXFont.cpp`; the `dc` member itself is declared in the
public header.

### Verification

- The removed lines of `FXFont.cpp` equal the added lines of the two modules
  apart from the `#if defined(WIN32)` / `#elif defined(HAVE_XFT_H)` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. Every widget measures text, so the Win32 and XLFD metrics run at
  startup; the Xft variant is not compiled in the WSL image.

## Seam 21: remaining window branches

Done. `FXWindow.cpp` no longer contains a platform branch. The decision the
earlier plan left open was settled in favor of per-module definitions: the
three platform-valued statics are defined once in each window module, so no
private header is needed and the public `FXWindow.h` is unchanged.

| Part | Location |
| --- | --- |
| Windows module | `FXWindow_win32.cpp`: `stringType=CF_TEXT`, `imageType=CF_DIB`, `utf16Type=CF_UNICODETEXT`, and `scale()`/`unscale()` with `MulDiv()` |
| X11 module | `FXWindow_x11.cpp`: `stringType=XA_STRING`, `imageType=XA_PIXMAP`, `utf16Type=0`, and `scale()`/`unscale()` returning the value unchanged |
| Portable core | the `*TypeName` strings and the portable drag type atoms (`octetType`, `textType`, ...) stay in `FXWindow.cpp`, as does `getScale()` |

The commented-out `setBackColor()` stub was left alone. `scale()` stays a
real member on X11 because X11 has no per-window DPI yet; a Wayland module
would implement it from the output scale.

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values (`dpi` exercises `scale()` on Windows).

## Seam 22: app display and input devices

Done. The display connection, input device handling and X error plumbing left
`FXApp.cpp` as whole members, with a scratch script
(`tmp/split-app-display.py`) that resolves the `WIN32` branches per platform.
`FXApp.cpp` shrank from 3121 to 2299 lines.

| Part | Location |
| --- | --- |
| Windows module | `FXApp_win32.cpp`: `GetOwnModuleHandle()`, `openInputDevices()`/`closeInputDevices()`, and `openDisplay()` with the stipple bitmaps created through `CreateBitmap()` |
| X11 module | `FXApp_x11.cpp`: `xerrorhandler()`/`xfatalerrorhandler()`, the XIM callbacks, `openInputDevices()`/`closeInputDevices()`, `windowTypeAtoms`, and `openDisplay()` with the XRender, Xft, XRandR and XFixes probes and the `XCreateBitmapFromData()` stipples |
| Portable core | the rest of `FXApp.cpp` |

### Verification

- The removed lines of `FXApp.cpp` equal the added lines of the two modules
  apart from the `#if defined(WIN32)` / `#else` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. `openDisplay()` runs at startup on both backends. The Xft,
  XInput2 and XRandR paths are only compiled where the headers exist; the WSL
  image has none of them.

## Seam 23: remaining app branches

Done. `FXApp.cpp` no longer contains a live platform branch; the only
`#ifdef WIN32` left is inside the long-dead `#if 0` `getForegroundWindow()`.
The branches became four private members and three internal functions, so the
public header gained only declarations without platform types. Scratch script:
`tmp/split-app-init.py`.

| Part | Location |
| --- | --- |
| Private members (`FXApp.h`) | `initPlatformMembers()` (constructor inits of the DDE, selection, XDND, window manager and Xlib members), `freePlatformMembers()` (destructor type lists), `initPlatformSettings()` and `parsePlatformArgument()` (command line switches of `init()`) |
| Internal functions (`fxplatform.h`) | `fxplatformDefaultDisplay()` (the `DISPLAY` lookup), `fxplatformDefaultFont()` (`Sans,90` with Xft, else `helvetica,90`), `fxplatformDefaultScale()` (`"auto"` on Windows, none on X11) |
| Windows module | `FXApp_win32.cpp`: the member inits, `getSystemColor()` and `getSystemFont()`, and `initPlatformSettings()` reading the system font, colors and wheel lines; `parsePlatformArgument()` accepts nothing |
| X11 module | `FXApp_x11.cpp`: the member inits, the `-sync`, `-noshm`, `-shm`, `-display`, `-im` and `-is` switches; `initPlatformSettings()` is empty |
| Portable core | the rest of `init()`, the constructor and the destructor |

### Verification

- The removed lines of `FXApp.cpp` equal the added lines of the two modules
  apart from the guards, the dedented argument blocks and the five replaced
  one-liners.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. The Win32 system colors and font are read at startup.

## Seam 24: I/O device

Done. First of the services: `FXIODevice.cpp` no longer contains a platform
branch. The nine members that touch the native handle moved as whole members
into `FXIODevice_win32.cpp` and `FXIODevice_x11.cpp` with a scratch script
(`tmp/split-iodevice.py`); the `#if` branches are resolved per platform.

| Part | Location |
| --- | --- |
| Private header | `fxiopriv.h` defines `BadHandle` (`INVALID_HANDLE_VALUE` or `-1`) for the portable class and the modules; `FXFile.cpp` still carries its own copy until its seam |
| Windows module | `FXIODevice_win32.cpp`: `mode()` and `mode(FXuint)` through `GetHandleInformation()`/`SetHandleInformation()`, `perms()`, `perms(FXuint)` (still commented out), `valid()`, `readBlock()` and `writeBlock()` with `ReadFile()`/`WriteFile()`, `close()` with `CloseHandle()`, and the `NtQueryObject()` stub |
| X11 module | `FXIODevice_x11.cpp`: the same members with `fcntl()`, `fstat()`, `fchmod()`, `read()`, `write()` and `close()`; the module is the POSIX implementation, named `x11` by the repository rule |
| Portable core | constructors, `isOpen()`, `attach()`, `detach()`, `truncate()`, `flush()`, `eof()`, `size()` and the destructor stay in `FXIODevice.cpp` |

### Verification

- The removed lines of `FXIODevice.cpp` equal the added lines of the new files
  apart from the `BadHandle` block, which moved to `fxiopriv.h`.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. The device members are compile-checked on both backends; the
  examples do not read from a raw device.

## Seam 25: mutex and thread storage key

Done. The two smallest thread primitives moved together; neither has a
platform branch left in the portable core. The modules were written by hand
from the original bodies (`tmp/split-mutex.py` generates them).

| Part | Location |
| --- | --- |
| Windows modules | `FXMutex_win32.cpp`: constructor, `lock()`, `trylock()`, `unlock()` and destructor on `CRITICAL_SECTION`; `FXAutoThreadStorageKey_win32.cpp`: the whole class on `TlsAlloc()`/`TlsSetValue()`/`TlsGetValue()`/`TlsFree()` |
| X11 modules | `FXMutex_x11.cpp`: the same members on `pthread_mutex_t`; `FXAutoThreadStorageKey_x11.cpp`: the class on `pthread_key_t`; both are the POSIX implementation, named `x11` by the repository rule |
| Portable core | `FXMutex.cpp` keeps `locked()`. `FXAutoThreadStorageKey.cpp` is gone because every member was platform code; the sources list in `Makefile.am` no longer names it |

The dead `#elif defined(WIN32)` branch of `FXMutex::trylock()` was dropped.

### Verification

- The removed lines equal the added lines of the new modules apart from the
  guards and that dead branch.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. Every example uses mutexes and thread storage through the
  application and registry.

## Seam 26: pipes

Done. `FXPipe.cpp` no longer contains a platform branch; the three members
that create or remove pipes moved whole into `FXPipe_win32.cpp` and
`FXPipe_x11.cpp` (`tmp/split-pipe.py`).

| Part | Location |
| --- | --- |
| Windows module | `FXPipe_win32.cpp`: `open()` with `CreatePipe()`; `create()` and `remove()` only return `false`, since named pipes are not available |
| X11 module | `FXPipe_x11.cpp`: `open()` with `pipe2()` (`HAVE_PIPE2`) or `pipe()` plus `fcntl()`, `create()` with `mkfifo()` and `remove()` with `unlink()` |
| Portable core | the two constructors and the `CreateNamedPipe()` note stay in `FXPipe.cpp`; its `BadHandle` block is gone because the core no longer uses it |

### Verification

- The removed lines equal the added lines of the two modules apart from the
  guards, the `BadHandle` block and the `create()`/`remove()` bodies reduced
  to the platform's branch.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. The pipe members are compile-checked; the examples do not
  create pipes.

## Seam 27: directories

Done. `FXDir.cpp` no longer contains a platform branch. Fourteen members and
the `SPACE` handle record moved as whole members into `FXDir_win32.cpp` and
`FXDir_x11.cpp`. The split script is generic now: `tmp/seamlib.py` resolves
`WIN32` / `_WIN32` branches of any shape per platform and keeps other
conditionals (`UNICODE`, `EXPERIMENTAL`, `_DIRENT_HAVE_D_TYPE`) as they are;
`tmp/split-dir.py` drives it.

| Part | Location |
| --- | --- |
| Windows module | `FXDir_win32.cpp`: `SPACE` with `HANDLE` and `WIN32_FIND_DATA`, both constructors, `open()`, `isOpen()`, both `next()`, `close()`, `create()`, `remove()`, `listFiles()`, `removeFiles()`, `listDrives()` with `GetLogicalDriveStringsW()`, `listShares()` with `WNetEnumResourceW()`, `hasSubDirectories()` on `FindFirstFile()` |
| X11 module | `FXDir_x11.cpp`: `SPACE` with `DIR*`, the same members on `opendir()`/`readdir()`/`closedir()`/`mkdir()`/`rmdir()`, a root-only drive and share list, and the hard-link count shortcut of `hasSubDirectories()` |
| Portable core | `createDirectories()` and the destructor stay in `FXDir.cpp` |

`listFiles()` and `removeFiles()` differ from platform to platform only in the
hidden-file test, but moved whole, so their loop bodies are duplicated in both
modules; unifying them behind one predicate is a separate refactor.

### Verification

- The removed lines equal the added lines of the two modules apart from the
  `WIN32` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values. The directory members are compile-checked on both backends.

## Seam 28: files

Done. `FXFile.cpp` no longer contains a platform branch. Thirteen members
moved as whole members into `FXFile_win32.cpp` and `FXFile_x11.cpp`. The
local `BadHandle` definition is gone; `FXFile.cpp` and both modules include
the shared `fxiopriv.h` from seam 24. `tmp/split-file.py` drives the split
with `tmp/seamlib.py`; it also understands the file's section separators.

| Part | Location |
| --- | --- |
| Windows module | `FXFile_win32.cpp`: `open()` on `CreateFile`, `position()` x2, `truncate()`, `flush()`, `size()`, `create()`, `link()` on `CreateHardLink`, both `symlink()` as empty stubs, `copyFiles()`, `move()` on `MoveFileEx`, `remove()` on `DeleteFile` |
| X11 module | `FXFile_x11.cpp`: the same members on `open()`/`lseek()`/`ftruncate()`/`fsync()`/`fstat()`/`link()`/`readlink()`/`symlink()`/`rename()`/`unlink()`, and `copyFiles()` with the fifo and device branches |
| Portable core | constructors, `isSerial()`, `eof()`, `identical()`, `copy()`, `moveFiles()`, `removeFiles()`, `concat()` stay in `FXFile.cpp` |

`copyFiles()` contains an inline `#if !defined(WIN32)` block for fifos and
devices. It moved whole, so its recursion logic is duplicated in both modules;
splitting out the special-file branch is a separate refactor.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from the `WIN32` guards and the `BadHandle` block.
- Fast builds on both backends; `hello`, `button` and `dpi` run with unchanged
  DPI values.

## Seam 29: processes

Done. `FXProcess.cpp` no longer contains a platform branch. Eleven items
moved as whole members into `FXProcess_win32.cpp` and `FXProcess_x11.cpp`;
`tmp/split-process.py` drives the split with `tmp/seamlib.py`.

| Part | Location |
| --- | --- |
| Windows module | `FXProcess_win32.cpp`: the command line and environment block helpers (`commandline()`, `enviroblock()`, `needquotes()`, `comparison()`, `extravars`, in their `UNICODE` and ANSI variants), `start()` on `CreateProcess`, `current()`, `handle()`, `exit()`, `suspend()` and `resume()` (still the commented-out stubs), `kill()` on `TerminateProcess`, both `wait()`, the destructor |
| X11 module | `FXProcess_x11.cpp`: `start()` on `fork()`/`execve()`, `current()`, `handle()`, `exit()`, `suspend()`/`resume()` on `SIGSTOP`/`SIGCONT`, `kill()`, both `wait()` on `waitpid()`, the destructor |
| Portable core | the constructor and `id()` stay in `FXProcess.cpp`, together with the long notes on quoting rules and environment variables |

The Windows helpers were one `#if defined(WIN32)` region spanning several
members, so they moved as a single block; the X11 module has no counterpart.
The destructor moved because it closes the process handle on Windows.

### Verification

- The removed lines equal the added lines of the two modules apart from the
  `WIN32` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The Windows
  `dpi` run reported a window DPI of 96 before `WM_DPICHANGED` on this
  session; the DPI code is untouched by this seam and the later steps report
  144 as before, so this reflects the monitor the window opened on.

## Seam 30: file statistics

Done. `FXStat.cpp` no longer contains a platform branch. Ten blocks moved as
whole members into `FXStat_win32.cpp` and `FXStat_x11.cpp`;
`tmp/split-stat.py` drives the split with `tmp/seamlib.py` and selects every
block that holds a `WIN32` conditional.

| Part | Location |
| --- | --- |
| Windows module | `FXStat_win32.cpp`: the `fxunixtime()` and `fxwintime()` FILETIME converters, `statFile()`, `statLink()`, `stat(FXFile)` on `GetFileAttributesEx`/`GetFileInformationByHandle`, the `mode(file,perm)` stub, `exists()`, the three time setters on `SetFileTime`, `isAccessible()`, both disk-space queries on `GetDiskFreeSpaceEx` |
| X11 module | `FXStat_x11.cpp`: the same members on `stat()`/`lstat()`/`fstat()`/`chmod()`/`utimensat()`/`access()`/`statvfs()`, with a `created(file,ns)` that returns false |
| Portable core | the instance accessors and the path-based wrappers (`isHidden()`, `size()`, `modified()`, `isSame()`, ...) stay in `FXStat.cpp` |

The `PERFORMANCE_RECORDER()` declarations moved with the function they
instrument. `created(file)` shared a block with the platform-specific
`created(file,ns)`; it was split first so that only the latter moved.

### Verification

- The removed lines equal the added lines of the two modules apart from the
  `WIN32` guard.
- Fast builds on both backends; `hello`, `button` and `dpi` run with 144 DPI
  on Windows.

## Seam 31: system functions

Done. `FXSystem.cpp` no longer contains a platform branch. Nineteen members
(plus the `__snprintf` declaration, which both modules need) moved into
`FXSystem_win32.cpp` and `FXSystem_x11.cpp`; `tmp/split-system.py` drives the
split. Two helper changes were needed:

- `tmp/seamlib.py` now resolves `#elif` on a platform conditional: the platform
  that owns the first branch drops the remaining branches, the other platform
  turns the first `#elif` into the opening `#if` and keeps the rest.
- The split script chunks members on two blank lines only outside of
  conditionals, because `setCurrentDrive()`/`getCurrentDrive()` and
  `system()` have blank-line gaps inside their `#if`/`#else` region.

| Part | Location |
| --- | --- |
| Windows module | `FXSystem_win32.cpp`: `user()`/`group()` stubs, user and group names via `GetUserName`, environment on `GetEnvironmentVariable`, current directory and drive, `PATHEXT`, `GetModuleFileName`, home, temp and system directories, `getUserSID()` and `getUserDirectory()` on the registry, `system()` on `CreateProcess`, `localeIsUTF8()` on `GetACP`, `dllName()` |
| X11 module | `FXSystem_x11.cpp`: the same members on `geteuid()`/`getpwuid_r()`/`getgrgid_r()`/`getenv()`/`setenv()`/`chdir()`/`getcwd()`/`/proc/self/exe`/`system()`, with `lib<name>.so` and its HP-UX and macOS variants |
| Portable core | `modeString()`, `getExecPath()` and `getHostName()` stay in `FXSystem.cpp` |

### Verification

- The removed lines equal the added lines of the two modules apart from the
  `WIN32` guards and the five `#elif` lines that became `#if` on the X11 side.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The initial
  window DPI on Windows depends on the monitor the window opens on (96 or 144
  between runs); the `WM_DPICHANGED` readings are 144.

## Seam 32: path name functions

Done. `FXPath.cpp` no longer contains a platform branch. Nineteen blocks moved
into `FXPath_win32.cpp` and `FXPath_x11.cpp`; `tmp/split-path.py` drives the
split. The file is organised in sections separated by `/****/` lines, so the
driver splits on those and on double blank lines outside conditionals, and
rebuilds the core with the separators of the kept sections.

| Part | Location |
| --- | --- |
| Windows module | `FXPath_win32.cpp`: `root()`, `share()`, `server()`, `drive()`, `isShare()`, `isServer()` for drive and UNC names, the drive-aware `directory()`, `name()`, `stem()`, `extension()`, `stripExtension()`, `expandEnvironmentVariables()` for `%VAR%`, `convert()`, `convertPathList()`, `contract()`, `simplify()`, `isAbsolute()`, `absolute()`, `relative(base,file)`, `isInside()`, `isTopDirectory()`, `isHidden()` on file attributes, `enquote()`/`dequote()` for cmd.exe, `parseArgs()` for the Windows command line, `hasExecExtension()`, `isReservedName()` |
| X11 module | `FXPath_x11.cpp`: the same members with POSIX rules (single root, `$VAR`/`${VAR}`, dot-file hiding, shell quoting) and the Windows-only name queries as stubs |
| Portable core | `expand()`, `convertToWindows()`, `convertFromWindows()`, `isRelative()`, `relative(file)`, `upLevel()`, `validPath()`, `isValidPath()`, `parseArgs(FXString)`, the `match()` family, `unique()`, `search()`, `relativize()` |

`expandEnvironmentVariables()` was a file-local helper of `expand()` that also
has a platform branch. It is now declared in the private header
`fxpathpriv.h`, listed in `noinst_HEADERS`, and loses its `static`. The unused
`<shellapi.h>` include moved to the Windows module.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from the removed `static` and the two `WIN32` guard lines with trailing comments.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The initial
  window DPI on Windows was 96 here; the `WM_DPICHANGED` readings are 144.

## Seam 33: threads

Done. `FXThread.cpp` no longer contains a platform branch. The platform blocks
moved into `FXThread_win32.cpp` (34 blocks) and `FXThread_x11.cpp` (32 blocks;
the Windows-only `NtDelayExecution` and `SetThreadDescription` helpers have no
POSIX counterpart). `tmp/split-thread.py` drives the split.

| Part | Location |
| --- | --- |
| Windows module | `FXThread_win32.cpp`: `function()` on `_beginthreadex`, `start()`, `join()`, `cancel()`, `detach()`, `exit()`, `yield()`, `pause()`, the clocks `time()`, `steadytime()`, `cputime()`, `ticks()`, the `NtDelayExecution` stub with `sleep()`/`wakeat()`, `current()`, `processors()`, `processor()`, storage keys, `priority()`, `policy()`, `affinity()`, the `SetThreadDescription` stubs with `description()`, `suspend()`, `resume()` |
| X11 module | `FXThread_x11.cpp`: the same members on pthreads, `clock_gettime()`/`nanosleep()`, `sysconf()`, `sched_*` and `pthread_setname_np()`, keeping the BSD, macOS, HP-UX and Solaris branches |
| Portable core | `selfKey`, constructor, `id()`, `running()`, both `self()` and the destructor |

`ticks()` is guarded by `defined(WIN32) && defined(_MSC_VER)`, which is not a
plain platform test. The driver maps it to `defined(_MSC_VER)` in the Windows
module and drops the branch from the X11 module, so MinGW builds keep the GNU
cycle counter branches.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from `#elif` lines that became `#if` on the X11 side, the `ticks()` guard and
  one trailing-comment guard.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The window DPI
  on Windows was 144 at startup and after `WM_DPICHANGED`.

## Seam 34: cursors

Done. `FXCursor.cpp` no longer contains a platform branch. `create()` (stock
and custom cursors, Win32 `CreateIconIndirect`/`LoadCursor`, X11 core and Xcursor
cursors) and `destroy()` moved into `FXCursor_win32.cpp` and `FXCursor_x11.cpp`.
The constructors, `detach()`, `release()`, the option and ownership accessors and
the stream functions stay in the core.

`tmp/split-generic.py` is the reusable driver: it moves each top-level block that
holds a platform conditional and also carries over what those blocks need from the
core. The `#define` helpers used only by moved code (`DISPLAY`, `DARKCOLOR`,
`CURSOR_STOCK`) moved with them and left the core; `TOPIC_CREATION` is used on
both sides and is defined in each module. `extern` declarations are copied to a
module only if its moved code calls them.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from the two `WIN32` guard lines with trailing comments.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The window DPI
  on Windows was 144 at startup and after `WM_DPICHANGED`.

## Seam 35: visuals

Done. `FXVisual.cpp` no longer contains a platform branch. Everything that
depends on the native visual moved into `FXVisual_win32.cpp` and
`FXVisual_x11.cpp`: both constructors (the X11 ones also clear `gc` and
`scrollgc`), `create()`, `destroy()`, `getPixel()` and `getColor()`. The Win32
module also holds the generic palette helpers; the X11 module holds the true,
direct, pseudo, static colour and grayscale setup, the standard colormap lookup,
`setupgc()` and the dither tables. `detach()`, `setMaxColors()`, the stream
functions, the destructor and the `FXIMPLEMENT` stay in the core.

`tmp/split-generic.py` now treats a conditional block of `#define` lines as one
unit, so `MAX_MAPSIZE` with its `HIGHENDGRAPHICS` selector moved to the X11
module together with its comment; `TOPIC_DEBUG` moved to both modules.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from the `WIN32` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The window DPI
  on Windows was 96 at startup (monitor placement) and 144 after `WM_DPICHANGED`.

## Seam 36: window device context

Done. `FXDCWindow.cpp` wrapped its whole implementation in one
`#if defined(WIN32)` / `#else` pair, so the entire class moved: the GDI version
into `FXDCWindow_win32.cpp` and the Xlib/Xft version into `FXDCWindow_x11.cpp`.
`FXDCWindow.cpp` now holds only the includes and the design notes, which apply to
both backends. Each module opens with its own section banner (`MS-Windows`,
`X-Windows`); the banner that sat between the two branches was moved to the X11
module.

`tmp/split-generic.py` keeps double blank lines between members when a block holds
several members (it only collapses runs of three or more).

### Verification

- Every removed line (2736) appears in the two modules.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The window DPI
  on Windows was 144 at startup and after `WM_DPICHANGED`.

With this seam the drawing group is done. The classes below still hold platform
branches in their core files.

## Seam 37: top level windows

Done. `FXTopWindow.cpp` no longer contains a platform branch. Twenty-seven blocks
moved into `FXTopWindow_win32.cpp` and 26 into `FXTopWindow_x11.cpp` (the
Windows-only `GetClass()` has no X11 counterpart): `create()`, `destroy()`,
`setFocus()`, `killFocus()`, `hide()`, `raise()`, `lower()`, `place()`,
`seticons()`, `settitle()`, `setdecorations()`, `getWMBorders()`, `flash()`,
`restore()`, `maximize()`, `minimize()`, `fullScreen()`, `stackingOrder()`,
`isMaximized()`, `isMinimized()`, `isFullScreen()`, `move()`, `resize()`,
`position()`, `setIcon()`, `setMiniIcon()` and the destructor. The message map,
`show()`, `close()`, the default size and `layout()`, the command and focus
handlers, the packing accessors and the stream functions stay in the core.

The Motif window manager hint constants (`MWM_*`), `DISPLAY` and the layout masks
are used by only one side, so the X11 module owns the `MWM_*` block and `DISPLAY`
(without the redundant `#ifndef WIN32`); `TOPIC_DETAIL` is defined in all three
files. `tmp/split-generic.py` now carries the comment that sits directly above a
define along with it, resolves platform guards inside moved define blocks, and
only removes a define from the core when a module takes it over.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from the `WIN32` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run. The window DPI
  on Windows was 144 at startup and after `WM_DPICHANGED`.

## Seam 38: clipping region

Done. Every member of `FXRegion` wraps a GDI `HRGN` or an Xlib `Region`, so the
whole implementation moved: 22 blocks into each of `FXRegion_win32.cpp` and
`FXRegion_x11.cpp` (constructors, destructor, assignment, `empty()`,
`contains()`, `bounds()`, `offset()`, the comparison and set operators, and
`reset()`). `FXRegion.cpp` is now an empty translation unit that keeps its
includes and the `FX` namespace so the file list stays stable. The header
notes comment is carried into both modules.

### Verification

- The removed lines equal the added lines of the two modules apart from the
  `WIN32` guards.
- Fast builds on both backends; `hello`, `button` and `dpi` run.

## Seam 39: utility functions

Done. `fxutils.cpp` keeps the portable helpers (version, trace topics, string
and colour helpers, DOS conversion). Seven blocks moved into `fxutils_win32.cpp`
and `fxutils_x11.cpp`: `fxisconsole()` (the PE header probe on Windows, a stub
elsewhere), `fxmessage()`, `fxerror()`, `fxwarning()`, `fxassert()`,
`fxverify()` and the `fxopen()`/`fxreopen()` pair. The Windows module keeps its
nested `_WINDOWS` test (debugger output and message boxes) and its `UNICODE`
test (wide file names); those select a Windows subsystem and are not platform
seams.

`fxtrace()` stays in the core because it owns the file-local trace state. Its
inline `OutputDebugString`/`fputs` tail was replaced by `fxmessage("%s",...)`,
which writes the same text through the platform module; this is the only
adaptation in the seam.

The driver now also splits blocks at `/***/` section separators, and rewrites
`defined(WIN32) && defined(UNICODE)` per platform before resolving guards.

### Verification

- Removed lines match the added lines of the core and modules apart from the
  `WIN32` guards, the `UNICODE` refinement and the `fxtrace()` change.
- Fast builds on both backends; `hello`, `button` and `dpi` run.

## Seam 40: keyboard translation

Done. Every line of `fxkeyboard.cpp` sat inside one `#ifdef WIN32` block: the
virtual-key to FOX key mapping (`wkbMapKeyCode()`), the modifier state
(`fxmodifierkeys()`), the code page lookup and the `ToUnicodeEx` adapters. The
X11 build compiled an empty file. The file was renamed with `git mv` to
`fxkeyboard_win32.cpp`, its guard became `#if defined(WIN32)`, and it is
listed with the other Windows modules in `fox2/lib/Makefile.am` and
`CMakeLists.txt`. There is no X11 counterpart and no core file left; the
declarations stay in the private `fxpriv.h`. The content is unchanged.

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run. The X11 build
  no longer compiles the file.

## Seam 41: style chrome

Done. `FXStyle.cpp` keeps the constructor, destructor and all drawing that goes
through `FXDCWindow`. The antialiased rounded rectangle path, which only exists
on X11 with XRender, moved into `FXStyle_x11.cpp`: the `FXCornerMask` cache
record, `invalidate()`, `buildCoverage()`, `compositeSolid()`, `hasXRender()`,
`getMask()` and `drawRoundRect()`. `FXStyle_win32.cpp` holds the same members as
stubs that clear the cache, report no XRender and return `false`/`nullptr`, so
`drawButton()` falls back to the plain path exactly as before.

The old `!defined(WIN32) && defined(HAVE_XRENDER_H)` tests were split: the X11
module keeps `defined(HAVE_XRENDER_H)` (the feature test stays, the platform
test is the module guard) and the Windows module resolves them to the false
branch. The driver rewrites that condition per platform before resolving.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from the rewritten `HAVE_XRENDER_H` conditions.
- Fast builds on both backends; `hello`, `button` and `dpi` run.

## Seam 42: image

Done. `FXImage.cpp` keeps the pixel operations (scale, mirror, rotate, crop,
gradients, blend, streams, `detach()`, `release()` and the constructors). Six
blocks moved into `FXImage_win32.cpp` and five into `FXImage_x11.cpp`:
`create()`, `destroy()`, `restore()`, `render()` (with the 20 `render_*`
converters and the `findshift()`/`lowbit()` helpers on X11), `resize()`, and on
Windows `GetDC()`/`ReleaseDC()`. The X11 module owns `DISPLAY()`, `MAX_MAPSIZE`
and `TOPIC_DETAIL`; the XShm tests inside it stay as `HAVE_XSHM_H` feature
tests.

The module guards in this file carried trailing comments (`#ifdef WIN32 //
WINDOWS`, `#else // X11`). `tmp/seamlib.py` now ignores trailing comments when it
recognises a platform test.

### Verification

- The removed lines equal the added lines of the core and the two modules apart
  from the platform guard lines.
- Fast builds on both backends; `hello`, `button` and `dpi` run.

## Seam 43: font

Done. `FXFont_win32.cpp` and `FXFont_x11.cpp` already held the font matching,
metrics and listing code; what was left in `FXFont.cpp` were the four
`#ifdef WIN32 dc=nullptr; #endif` initialisations in the constructors. The
`dc` member (the dummy GDI device context) is now declared unconditionally in
`FXFont.h` as `void *dc`, next to `font`, and the constructors initialise it on
every platform. The X11 backend never reads it, so behavior is unchanged; each
`FXFont` grows by one pointer there.

The one `#if defined(WIN32)` still in `FXFont.cpp` sits inside a block comment
(an old `drawText()` sketch) and is not compiled.

### Verification

- Fast builds on both backends; `hello`, `button` and `dpi` run.

## Next seams, in order

1. **Remaining core classes**, one class per seam, largest platform footprint
   first: `FXRootWindow`,
   `FXIcon`, `FXPopup`, then the files with one or two references. Presentation
   seam; needed before a browser frontend and for the Wayland SHM surface.

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
