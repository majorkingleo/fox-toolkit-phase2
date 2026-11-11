---
name: fox-toolkit
description: Understand, use, extend, and debug the FOX 1.7.95 C++ GUI toolkit in this repository. Use when working with FOX applications, widgets, event/message handlers, layouts, data targets, or library internals, or when writing FOX code that must match the library's formatting and coding conventions.
---

# FOX Toolkit: How It Works

FOX is a C++ GUI toolkit. This repository identifies itself as version 1.7.95 in
`configure.ac`. It provides a widget library, platform integration, event
dispatch, layout managers, dialogs, graphics and image support, serialization,
and utility/concurrency classes.

## Find the relevant code

- `include/` contains public headers; `lib/` contains the implementation.
- `include/fx.h` is the normal umbrella header. It includes the toolkit API and,
  unless `FX_NO_GLOBAL_NAMESPACE` is defined, imports `FX` into the global
  namespace. Use `fx3d.h` for OpenGL facilities. FOX applications should not
  include platform window-system headers directly.
- `tests/` contains small example applications and feature demonstrations.
  Start with `tests/hello.cpp` for application setup, `tests/datatarget.cpp`
  for data binding, and `tests/scribble.cpp` for custom message handlers and
  drawing.
- `doc/` contains conceptual guides, including `introduction.html`,
  `messages.html`, `layout.html`, `datatarget.html`, `window.html`,
  `guiupdate.html`, `timers.html`, and `serialization.html`.
- Build definitions are Autotools files (`configure.ac` and `Makefile.am`).
  The library target is `libFOX-1.7.la`; public headers install under
  `$(includedir)/fox-1.7`.

## The application and widget lifecycle

An application normally has one `FXApp`. It initializes the platform/display,
owns the application-wide event queue and registry, and is the ultimate owner
of the widget tree. Widgets are connected by parent-child relationships:
constructing a widget with a parent both places it in the tree and gives the
parent/layout manager responsibility for its geometry.

FOX separates C++ object construction from creation of platform resources:

1. Construct `FXApp` and call `init(argc, argv)`.
2. Construct the top-level window and its child widgets.
3. Call `app.create()` to create platform-side windows and resources.
4. Show the top-level window.
5. Call `app.run()` to dispatch events until the application exits.

Keep GUI work on the event-loop thread. Avoid long blocking work in message
handlers; use FOX timers, chores, or its worker/thread facilities when work
must happen asynchronously. Follow FOX's parent/tree ownership rules when
destroying widgets.

## Event routing: targets, selectors, and message maps

FOX uses runtime target/message dispatch instead of requiring a callback
function for every control. A widget sends a selector to an `FXObject` target.
A selector combines a message type (what happened) and an ID (which action or
source); the target's message map routes it to a member function. Because the
target is explicit, IDs only need to be unique within the class hierarchy that
handles them.

When adding a message-handling `FXObject` subclass:

- Put `FXDECLARE(ClassName)` in the class declaration.
- Define its message table with `FXDEFMAP(ClassName)` and `FXMAPFUNC` (or
  `FXMAPFUNCS` for an ID range), then define it with
  `FXIMPLEMENT(ClassName, BaseClass, Map, ARRAYNUMBER(Map))`.
- Start new enum IDs at `BaseClass::ID_LAST`, then finish the enum with the
  subclass's own `ID_LAST`.
- Use the FOX handler signature
  `long onAction(FXObject* sender, FXSelector sel, void* data)`.
- Map both message type and ID. Common types include `SEL_COMMAND` for
  activated commands, `SEL_CHANGED` for changed values, `SEL_UPDATE` for
  querying presentation/state, and event messages such as `SEL_PAINT` and
  mouse/key events.
- Return `1` when the handler processes a message and `0` when it does not.
  A mapped handler's return value does not trigger base-class dispatch; base
  handling occurs when no matching map entry exists.

Use the widget's `setTarget()` and `setSelector()` or its constructor target
and selector arguments to wire messages. Use `FXSELTYPE(sel)` and
`FXSELID(sel)` when one handler handles a range of selectors. Inspect the
specific widget header and implementation for the exact message types, IDs,
and `data` payload contract.

## Automatic UI updates and data binding

The target/message system supports a pull-style update path as well as
user-action notifications. A control may send `SEL_UPDATE` so its target can
refresh enabled/disabled, checked, visible, or displayed state from current
application data. Keep command handlers focused on changing application
state, and update handlers focused on reflecting that state in the control.
This avoids manually synchronizing every control after every action.

Use `FXDataTarget` when a supported scalar or `FXString` variable should
directly drive one or more controls. It provides bidirectional value transfer:
controls send `FXDataTarget::ID_VALUE` (or `ID_OPTION + value` for option
controls), and update queries pull the variable back into the controls.
Use explicit message handlers instead when changes need validation, complex
side effects, or domain-specific behavior. See `tests/datatarget.cpp` for
examples connecting multiple controls to the same value.

## Widget trees and layout

`FXWindow` is the base of GUI windows/widgets. Composite widgets contain
children; layout managers arrange them using child layout hints and manager
packing options. Prefer nested managers over fixed coordinates so the UI
adapts to resizing, fonts, and labels. Common choices include:

- `FXHorizontalFrame` and `FXVerticalFrame` for rows and columns.
- `FXMatrix` for grids.
- `FXPacker`/`FXTopWindow` for side-packed children.
- `FXSplitter` for user-resizable regions and `FXSwitcher` for stacked pages.

Read the relevant widget header for constructor flags; many FOX layout and
frame behaviors are encoded as bit flags.

## Drawing, resources, and persistence

Custom drawing typically handles `SEL_PAINT` and uses FOX drawing contexts
(`FXDCWindow`/`FXDC`) and FOX color, image, and font types. Do not retain a
drawing context beyond the operation for which it was acquired; follow the
specific widget's paint implementation and examples.

FOX also provides `FXStream` and file/memory stream classes for
machine-independent serialization. For a subclass that must serialize its
state, follow the `FXObject::save()`/`load()` conventions and inspect the
streaming operators and examples before changing the serialized format.

## Code formatting and coding rules

These conventions were extracted from the existing `include/` and `lib/`
sources; the parenthesised counts show which variant dominates in the tree.
Follow them when adding or changing FOX code, including the ported copy under
`fox2/`. Never reformat a file to a different style than the one it already
uses.

### Language level and idioms

- Target the C++11-era dialect the library already uses: `nullptr` (~3055 uses)
  instead of `NULL` (~286 legacy leftovers).
- Mark every overriding virtual with `override` (~1457) and keep the redundant
  `virtual` in front of it (~1436 sites), as in
  `virtual void create() override;` and `virtual FXbool canFocus() const override;`.
- Do not introduce `enum class`, `constexpr`, or `auto` (0 / 0 / ~49 uses).
  Plain `enum` inside `namespace FX` and explicit types are the house style.
- Suppress copying the classic way: declare the copy constructor and
  `operator=` `private` and leave them undefined (~444 / ~366 headers). The
  tree contains no `= delete`; match the file you edit.
- Keep platform differences inside `#ifdef WIN32`, `HAVE_*`, and similar
  conditionals, the way `FXApp.cpp`, `FXWindow.cpp`, and `xincs.h` do.
- C-style casts still dominate (~2303), but newer headers use
  `static_cast`/`reinterpret_cast` (~84 / ~131). Match the surrounding file,
  prefer the named casts in new code, and never mass-convert.

### FOX scalar types

Use FOX's typedefs from `include/fxdefs.h` rather than raw built-in types in
declarations that are part of the API/ABI or that must stay platform-neutral:
`FXchar`, `FXuchar`, `FXbool`, `FXshort`, `FXushort`, `FXint`, `FXuint`,
`FXlong`, `FXulong`, `FXfloat`, `FXdouble`, `FXival`, `FXuval`, `FXptr`,
`FXTime`, `FXColor`, `FXSelector`, `FXInputHandle`.

- `FXint`/`FXuint` for widget coordinates, sizes, and colors;
  `FXival`/`FXuval` for element counts, indices, and offsets in containers.
- `FXbool` for predicate returns (not `int`); `FXString` rather than `char*`.

### Naming

- Classes and public types: `FX` + UpperCamel (`FXButton`, `FXDataTarget`).
- Member functions are lowerCamel verb phrases (`setButtonStyle`,
  `getDefaultWidth`, `create`, `layout`). Message handlers are `on` + the
  message (`onPaint`, `onLeftBtnPress`, `onCmdGetIntValue`, `onQueryTip`).
- Data members are plain lowerCamel with no prefix and no trailing underscore
  (`flags`, `options`, `target`, `message`, `headPos`); the tree uses no `m_`
  and no `_` suffixes at all.
- Locals are deliberately terse: `p`, `tgt`, `sel`, `opts`, `x`, `y`, `w`, `h`,
  `pl`..`pb`, `dc`, `ic`, `src`, `pos`, `num`.
- Enumerators are UPPER_SNAKE, unprefixed, and used unqualified inside
  `namespace FX` (`BUTTON_TOOLBAR`, `SLIDER_HORIZONTAL`, `STATE_UP`). Macros are
  upper case too (`FXMAPFUNC`, `FXTRACE`, `DEFAULT_PAD`, `ARRAYNUMBER`).
- Message IDs are `ID_` + UPPER_SNAKE: the first one is
  `BaseClass::ID_LAST` and the enum closes with a bare `ID_LAST`.

### Indentation and braces

- Two spaces per level, spaces only. Do not add tabs; only a handful of stray
  tabs survive, mostly in enum alignment and comment columns.
- Opening brace stays on the statement line, and the closing brace is indented
  to the statement rather than to the block:

  ```
  void FXButton::setState(FXuint s){
    if(state!=s){
      state=s;
      update();
      }
    }
  ```

- Class, `enum`, and message-map closing braces follow the same rule
  (`  };`); a namespace closes with `}` in column 0.
- Control keywords take no space before the parenthesis: `if(`, `for(`,
  `while(`, `switch(` (~17208 vs 78 for `if(`). Use `else{` and `else if(`.
- Single-statement bodies omit braces:
  `if(options&BUTTON_DEFAULT) setDefault(true);`
- Message tables keep that shape:

  ```
  FXDEFMAP(FXButton) FXButtonMap[]={
    FXMAPFUNC(SEL_PAINT,0,FXButton::onPaint),
    ...
    };
  FXIMPLEMENT(FXButton,FXLabel,FXButtonMap,ARRAYNUMBER(FXButtonMap))
  ```

### Expression spacing

- No spaces around assignment, comparison, or arithmetic operators:
  `state=STATE_UP;`, `if(state!=s)`, `if((n-m)>0)` (~3713 tight assignments
  versus 39 spaced ones).
- Flag tests stay bare: `if(options&BUTTON_INITIAL)`,
  `if(!(options&SLIDER_VERTICAL))`.
- Parameters are comma-separated with no following space and defaults use a
  tight `=`:
  `FXButton(FXComposite* p,const FXString& text,FXIcon* ic=nullptr,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=BUTTON_NORMAL,...)`.
- `for` headers keep a space after each semicolon:
  `for(i=0; i<=max; i++)`.
- `*` and `&` bind to the type: `FXComposite* p`, `const FXString& text`,
  `void* data` (majority; older files sometimes write `FXComposite *p`).
- In declarations, type and name are aligned in columns with a trailing `//`
  comment; enumerators align their `=` and end with a `///` comment:

  ```
  FXint         headPos;                  // Head position
  FXColor       slotColor;                // Color of slot the head moves in
  ```

### Files, comments, and includes

- Headers start with the license banner, then `#ifndef FXNAME_H` /
  `#define FXNAME_H`. Dependent headers are included as
  `#ifndef DEP_H` / `#include "Dep.h"` / `#endif`. No `#pragma once`.
- A `.cpp` includes `"xincs.h"` first, then the `fx*.h` internals, then the
  `FX*.h` headers, with the class's own header last, followed by
  `using namespace FX;` and `namespace FX { ... }`.
- Every public declaration gets a `///` one-liner; each class gets a
  `/** ... */` block describing behaviour; implementation functions get a `//`
  line above them. Do not add Doxygen `@param`/`@return` tags (0 uses).
- Major implementation blocks are separated by the 81-column line
  `/*******************************************************************************/`.
- Use `FXTRACE(TOPIC_...,...)` for diagnostics (~1345 calls; common topics are
  `TOPIC_DETAIL`, `TOPIC_DEBUG`, `TOPIC_CONSTRUCT`, `TOPIC_CREATION`,
  `TOPIC_EVENT`).
- Files end with a newline. Trailing whitespace already exists in the tree: do
  not add more, but do not mass-clean it either.
- Long parameter lists are kept on a single line even when they exceed the
  usual width; do not introduce new wrapping.
- There is no `.clang-format` or `.editorconfig` in the project. Match the file
  being edited by hand.

## Build and verification workflow

The documented Unix build is `./configure` followed by `make`; optional
features and platform dependencies are detected/configured there (for example
OpenGL and image codecs). `make install` installs the configured library and
headers. The programs in `tests/Makefile.am` are `noinst_PROGRAMS` examples,
not an automatically executed unit-test suite. Build the relevant example
with the project build, and run it only where a display and its optional
dependencies are available.

For a change:

1. Check the public declaration in `include/` and its implementation in
   `lib/`; preserve the existing FOX naming, selector, and ownership patterns,
   and follow the formatting rules in the section above.
2. Find existing usage in `tests/` and the relevant topic in `doc/` before
   adding a new mechanism.
3. Update the example/build declarations if adding a test or demo, and update
   directly related documentation when public behavior changes.
4. Rebuild the smallest affected target, then run an appropriate GUI example
   or focused validation. Distinguish a compile check from runtime GUI
   verification.

## Known glitches and gotchas

Traps that cost real debugging time. All were reproduced in the `fox2/` port,
and all of them are origin code paths, not port artefacts. Each entry gives the
symptom, the cause with its source location, and the workaround.

### Value parsing and numbers

- **`toInt()` rejects the string `"0"`.** `FXString("0").toInt(10,&ok)` returns
  0 with `ok=false`. Cause: `__strtoll()` in `lib/fxstrtoll.cpp` consumes a
  leading `0` while looking for an `0x`/`0b` prefix, then finds no digits left
  and reports failure. Consequence: `FXSettings::readIntEntry()` returns its
  *default* for any entry whose value is `0`, which silently breaks tri-state
  settings. Workaround: read the entry as a string
  (`readStringEntry`) and compare, or supply a default that cannot be confused
  with a real value.
- **`FXString::mid()` needs a length.** There is no `mid(pos)` overload, only
  `mid(pos,n)`; also no `count(ch)` and no `endsWith()`. Use `right(n)`,
  `after(ch)`, `before(ch)`, or count characters yourself.

### Timers and events

- **`addTimeout()` fires once.** FOX timers are one-shot. Returning 1 from the
  `SEL_TIMEOUT` handler does *not* repeat it, and a handler that forgets to
  re-arm produces exactly one step (a hover fade that never fades, for
  example). Workaround: call `getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,id),ns)`
  again from inside the handler, and return 0 only when finished.
- **There is no `FXApp::getTick()`.** For elapsed-time math use
  `FXThread::time()`, which returns nanoseconds in `FXTime`.
- **`update()` does not generate X `Expose` events.** `FXWindow::update()`
  forwards to `FXApp::addRepaint()`, a client-side repaint queue that the
  event loop paints directly (see `lib/FXWindow.cpp`, `lib/FXApp.cpp`). Any
  test that waits for `Expose` to prove a repaint will hang. Workaround:
  subclass the widget, map `SEL_PAINT`, and count paint calls in-process.
- **`underCursor()` is stale inside `onLeave`.** `FXApp::enterWindow()` sets
  `cursorWindow` *before* dispatching `SEL_ENTER`, but `FXApp::leaveWindow()`
  dispatches `SEL_LEAVE` *before* clearing it, so `underCursor()` is still true
  while the leave handler runs. Do not read hover state there; pass the new
  state into the code that needs it.
- **Crossing pairs drive both enter and leave.** Real pointer crossings are
  processed on the `EnterNotify` branch, which computes
  `commonAncestor(window,cursorWindow)` and emits the leave for the old window
  before the enter for the new one. Sending a bare `LeaveNotify` to a child
  dispatches nothing, because `leaveWindow()` is called with the child's own
  parent as the ancestor. To simulate "pointer left a child", send an
  `EnterNotify` for the parent instead.

### Memory and buffers

- **`FXFREE()` takes the address of the pointer.** The macro is
  `(FX::fxfree((void**)(ptr)))` and `fxfree()` frees `*ptr`, so `FXFREE(buffer)`
  frees whatever the first bytes of the buffer happen to look like and then
  aborts with `free(): invalid pointer`. Use `FXFREE(&buffer)`, and note that
  `FXMALLOC(&buffer,type,count)` also takes the address and returns `FXbool`.
- **`FXMemoryStream::takeBuffer()` reports capacity, not content.** It returns
  `endptr-begptr`, which is the whole buffer size, so a written-out file is
  padded with uninitialised bytes. Take the byte count from `position()`
  before draining the stream, or keep ownership with `FXMALLOC` and let the
  stream use an external buffer (`open(FXStreamSave,buffer,size,false)`).

### Windows, drawing, resources

- **Every window is created with `background_pixel=0`.** Anything the toolkit
  does not paint stays black. Rounded or otherwise non-rectangular shapes must
  fill their own rect with the parent's background color before drawing, or
  the corners come out black.
- **`FXTopWindow::create()` does not map the window.** `FXApp::create()` builds
  the X windows but nothing appears until `show()`. Consequence: a window that
  was never shown makes `FXDCWindow::readPixel()`/`XGetImage()` fail with
  `BadMatch (invalid parameter attributes)` because the drawable is not
  viewable.
- **`FXStat::statFile()` reports a directory as "not a file".** Code that
  stamps or validates a *directory* (mtime/size caching, for example) always
  fails because `isFile()` is false. Resolve a directory to the file that will
  actually be read before stat-ing it.
- **A generic INI reader accepts any INI file.** `FXTheme::load()` parses a KDE
  `.colors` file successfully and yields a theme with every key unmatched, so
  "try INI, fall back to import on failure" silently produces defaults.
  Dispatch on the file suffix or on content, and treat parse success as
  unrelated to "the right format was read".

### Building extensions

- **Include order matters.** `FXStream.h` needs `FXHash.h` (which needs
  `FXElement.h`); `FXDir.h` needs `FXIO.h`; `FXThread.h` needs
  `FXAutoThreadStorageKey.h` and `FXRunnable.h`. Copy the prologue of an
  existing implementation that already uses the class (`lib/FXSettings.cpp` for
  settings/streams) instead of inventing one.
- **`FXStream` has no `FXString` operators in 1.7.95.** Serialize strings as a
  length plus raw bytes, and bound the length on read because the input may be
  untrusted.
- **Layout changes break stale binaries.** After adding members to `FXApp`,
  `FXTheme` or other core classes, helper programs built against the old
  library abort with `FXApp::init: bad arguments.` (garbage `argc`/`argv`).
  Rebuild every helper, not just the library.

### Verifying GUI behaviour on KDE Wayland

The development desktop runs KWin with Xwayland, which limits what a test can
do to a running application:

- `XWarpPointer` does not move the real pointer, so no crossing events arrive
  and hover-driven code never runs. Synthesize the crossing instead:
  `XSendEvent` with `EnterNotify`, `mode=NotifyNormal`,
  `detail=NotifyAncestor`, `send_event=True`. For a click, send
  `ButtonPress`/`ButtonRelease` the same way.
- `import -window <title>` and root captures are unreliable here; prefer
  reading pixels in-process (`FXDCWindow::readPixel`) or comparing the few
  properties the test actually cares about.
- Runs may be slower than the timer budget suggests; sample with intervals
  well below the animation time and assert on ordering and termination rather
  than on exact frame counts.
