---
name: fox-toolkit
description: Understand, use, extend, and debug the FOX 1.7.95 C++ GUI toolkit in this repository. Use when working with FOX applications, widgets, event/message handlers, layouts, data targets, or library internals.
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
   `lib/`; preserve the existing FOX naming, selector, and ownership patterns.
2. Find existing usage in `tests/` and the relevant topic in `doc/` before
   adding a new mechanism.
3. Update the example/build declarations if adding a test or demo, and update
   directly related documentation when public behavior changes.
4. Rebuild the smallest affected target, then run an appropriate GUI example
   or focused validation. Distinguish a compile check from runtime GUI
   verification.
