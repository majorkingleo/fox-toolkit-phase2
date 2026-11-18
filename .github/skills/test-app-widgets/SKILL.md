---
name: test-app-widgets
description: "Use the FOX 2 test applications as implementation references. Use when choosing, wiring, laying out, drawing, or debugging FOX widgets, dialogs, menus, list/table/tree controls, custom canvases, timers, data targets, window movement, clipboard, drag and drop, MDI, docking, OpenGL, SVG, DPI, or Win32/Wayland/X11 behavior. Contains a verified index of fox2/tests examples and instructions for matching an existing pattern before inventing a new one."
---

# FOX 2 test applications and widget usage

Use this skill before implementing a FOX widget in `fox2/apps`, `fox2/tests`, or
`fox2/tools`. The current examples are under `fox2/tests/`; do not use
`fox-1.7.95/tests/` as the target implementation tree.

The tests are the first reference, not the only reference. Confirm constructor
options and API details in `fox2/include/<Widget>.h`, and confirm event behavior
in `fox2/lib/<Widget>.cpp` when a test leaves something ambiguous.

## Workflow

1. Match the requested feature to the test index below.
2. Read the whole relevant test source, not an isolated constructor call.
3. Copy the widget hierarchy, message map, target/selector, creation and cleanup
   pattern into the target app.
4. Use the project's FOX formatting rules; do not copy obsolete platform
   conditionals or source-specific formatting.
5. Add a screenshot/manual gate for visual controls. Console-only tests are not
   substitutes for GUI verification.
6. If a test exposes a toolkit defect, fix the seam in `fox2/lib/`; do not add
   platform-specific code to an application.

For a test application with no obvious match, inspect neighboring examples and
search the class name in both `fox2/tests/` and `fox2/include/` before adding a
custom widget.

## Quick index: GUI tests

| Need | Test app | Primary reference |
| --- | --- | --- |
| Minimal FOX lifecycle | `hello` / `hello2` | `fox2/tests/hello.cpp`, `hello2.cpp` |
| Buttons, icon/text positioning, groups, status | `button` | `button.cpp` |
| Mouse press/release routing | `click` | `click.cpp` |
| DPI/monitor scaling | `dpi` | `dpi.cpp` |
| Popup opened from a button | `popup` | `popup.cpp` |
| Menus, cascades, separators, menu state | `menus` | `menus.cpp` |
| Tooltips | `tooltip` | `tooltip.cpp` |
| Clipboard and primary selection | `clipboard` | `clipboard.cpp` |
| Drag and drop | `dnd` | `dnd.cpp` |
| Icons and bitmaps | `icon`, `bmpicon` | `icon.cpp`, `bmpicon.cpp` |
| Text fields, `FXText`, spinners | `textspin` | `textspin.cpp` |
| Combo/list input | `combo` | `combo.cpp` |
| Color dialog/well | `colordlg`, `colordemo` | `colordlg.cpp`, `colordemo.cpp` |
| Font dialog/preview | `fontdlg` | `fontdlg.cpp` |
| Rulers | `rulerdemo` | `rulerdemo.cpp` |
| Gauge/progress displays | `gaugedemo`, `gaugetest` | `gaugedemo.cpp`, `gaugetest.cpp` |
| Idle inhibit | `idle` | `idle.cpp` |
| Frame/matrix/packer layout | `layout` | `layout.cpp` |
| Aspect-ratio geometry | `ratio` | `ratio.cpp` |
| Sortable column headers | `header` | `header.cpp` |
| Two/four-pane splitting | `splitter`, `foursplit` | `splitter.cpp`, `foursplit.cpp` |
| Stacked switcher pages | `switcher` | `switcher.cpp` |
| Tabbed pages | `tabbook` | `tabbook.cpp` |
| Grouped controls | `groupbox` | `groupbox.cpp` |
| Modal/modeless dialog | `dialog` | `dialog.cpp` |
| Wizard | `wizard` | `wizard.cpp` |
| Automatic data binding | `datatarget` | `datatarget.cpp` |
| Calendar/date selection | `calendar` | `calendar.cpp` |
| Directory trees and file lists | `dirlist` | `dirlist.cpp` |
| Icon lists/details | `iconlist` | `iconlist.cpp` |
| Editable table grid | `table` | `table.cpp` |
| Image display | `image`, `imageviewer`, `bitmapviewer` | corresponding `.cpp` files |
| Custom painting/input canvas | `scribble`, `dctest` | `scribble.cpp`, `dctest.cpp` |
| MDI | `mditest` | `mditest.cpp` |
| Collapsible shutters | `shutter` | `shutter.cpp` |
| Console text view | `console` | `console.cpp` |
| SVG display | `svg` | `svg.cpp` |
| OpenGL | `gltest` / `glcanvas` | `gltest.cpp`, `glcanvas.cpp` |
| Software/raster drawing comparison | `raster` | `raster.cpp` |
| Slider, dial, knob, 7-segment, real sliders | `datatarget` | `datatarget.cpp` |
| Range slider, arrow buttons, scroll bars, list/tree boxes | `groupbox` | `groupbox.cpp` |
| Color bar/ring/wheel and gradient bars | `colordemo`, `image` | `colordemo.cpp`, `image.cpp` |
| Toolbar docking and drag/status corner | `bitmapviewer` | `bitmapviewer.cpp` |
| Progress dialog | `datatarget` | `datatarget.cpp` |

The actual build list is authoritative: inspect `fox2/tests/CMakeLists.txt` and
`fox2/tests/Makefile.am` when a target is absent in one build configuration.

## Core lifecycle

Use the standard FOX sequence shown by `hello.cpp` and the applications:

1. Construct `FXApp` and call `init(argc,argv)`.
2. Construct a top-level window with `new`; do not put a top-level window on the
   stack.
3. Construct children and wire targets/selectors.
4. Call `app.create()`; create explicitly owned icons/fonts that are not already
   children or resources managed by a widget.
5. Show the top window from its `create()` override or immediately after
   `application.create()`.
6. Run `app.run()`; save persistent state from the close handler.

Use `FXMainWindow`, `FXDialogBox`, or the most specific top-level class. Use
`DECOR_NONE` for borderless apps only when the app supplies a visible surface,
close path, and movement strategy.

## Message maps and event routing

Pattern:

- subclass an `FXObject`/widget with `FXDECLARE`;
- add `long onCmd...(FXObject*,FXSelector,void*)` declarations;
- define `FXDEFMAP`, `FXMAPFUNC`/`FXMAPFUNCS` and `FXIMPLEMENT` in the `.cpp`;
- begin IDs at the base class's `ID_LAST`, end with `ID_LAST`;
- return `1` from a handled handler;
- use `SEL_COMMAND` for actions, `SEL_UPDATE` for pull-style state, `SEL_CHANGED`
  for incremental edits, and `SEL_PAINT`/mouse events for drawing/input.

When a child is the event source and the window must own the action, set the
child's target to the owning window and use an explicit selector/ID. For stock
widgets that swallow an event, inspect their implementation before subclassing.
`click.cpp`, `scribble.cpp`, `menus.cpp` and `dnd.cpp` demonstrate distinct
routing styles.

## Layouts and containers

- `FXVerticalFrame`/`FXHorizontalFrame`: default choice for rows/columns. Combine
  `LAYOUT_FILL_X|LAYOUT_FILL_Y` for expansion.
- `PACK_UNIFORM_WIDTH`/`PACK_UNIFORM_HEIGHT`: use for equal buttons or tabs.
- `FXMatrix`: choose `MATRIX_BY_COLUMNS` or `MATRIX_BY_ROWS`; use when all cells
  should align. See `layout.cpp` and `groupbox.cpp`.
- `FXPacker`: use padding, spacing and side layout flags rather than hard-coded
  child coordinates.
- `FXSplitter` and 4-splitter patterns: use child panes, never manually resize
  panes in an application unless `splitter.cpp` does not cover the case.
- `FXSwitcher`: each child is a page. Call `setCurrent(index)`. Buttons can target
  `FXSwitcher::ID_OPEN_FIRST+n`; see `switcher.cpp`.
- `FXTabBook`: each `FXTabItem` must pair with one page composite; see
  `tabbook.cpp`.
- `FXGroupBox`: use for a labelled related-control group; see `groupbox.cpp`.
- `FXShutter`: alternating header/item children form collapsible sections; see
  `shutter.cpp`.

Use equal button width for a mode bar by placing the buttons in an
`FXHorizontalFrame` with `PACK_UNIFORM_WIDTH` and giving each button the same
fill flags. Do not implement a separate geometry routine.

## Buttons and controls

- `FXButton`: inspect the constructor's `opts`, target and selector. Common
  options are `BUTTON_NORMAL`, `LAYOUT_FILL_X`, icon/text alignment flags, and
  explicit tooltip text.
- State: update a button with `FXButton::setState(STATE_DOWN/STATE_UP)` from a
  `SEL_UPDATE` handler when it represents a selected mode.
- `FXToggleButton`, `FXCheckButton`, `FXRadioButton`: use their built-in state
  and update/data-target APIs rather than hand-drawing check marks.
- `FXMenuButton`: attach an `FXPopup`/`FXMenuPane`; use `menus.cpp` and
  `popup.cpp` for positioning.
- `FXComboBox` versus `FXListBox`: editable text plus list versus selection-only;
  `combo.cpp` shows both population and command behavior.
- `FXTextField`: set column count and route `SEL_COMMAND`/`SEL_CHANGED` as
  needed.
- `FXText`: set visible rows/columns, scroll behaviour and wrap flags; use
  `textspin.cpp` and editors rather than building a multiline text field.
- `FXSpinner`/`FXRealSpinner`: set range and value, and keep disabled/running
  state in `SEL_UPDATE`.

SVG buttons: embed source bytes or bundle/install SVG files deterministically.
Load with `FXSVGIcon` when dimensions are required at rasterization time. Create
and render owned icons after `FXMainWindow::create()`; render into the requested
size and delete owned icons in the destructor. Give the widget room for the
scaled icon: size the row that holds it from the window scale, not a constant,
or the icon is clipped at 150% and above. See `FXSVGIcon.h/.cpp`, the SVG test
and `fox2/apps/stopwatch/StopWatch.cpp`.

## Lists, headers, trees and tables

- `FXList`: append/remove items, use an explicit selection option, and query the
  selected index on `SEL_SELECTED`/`SEL_COMMAND` as demonstrated by combo/list
  tests.
- `FXHeader`: configure item count/text/size and sort/update arrows; use
  `header.cpp` before implementing custom column-header drawing.
- `FXIconList`: choose detailed versus icon mode; populate headers before
  items in detailed mode. See `iconlist.cpp`.
- `FXDirList`/`FXFileList`: use real directory/file lists rather than manually
  building a filesystem tree. Set an initial directory and observe selection
  selectors. See `dirlist.cpp` and `tabbook.cpp`.
- `FXTable`: call `setTableSize(rows,columns)`, `setColumnText`,
  `setItemText`, set visible row/column counts and use read-only options when
  cells are not editable. Set a bounded width/height and allow column sizing.
  See `table.cpp`.
- Read-only history: use `TABLE_READONLY`; do not leave editor commands enabled.

For a history table, newest-first ordering belongs in the data model. Rebuild the
table when a popup opens, rather than continuously mutating hidden rows.

## Dialogs and popups

- Dialog button targets should use the dialog's `ID_ACCEPT`/`ID_CANCEL`.
- Modal: create the dialog, set initial values, call `execute(PLACEMENT_OWNER)`.
- Modeless: create once, call `create()` when needed and `show()`; hide rather
  than destroy if its state must survive.
- `FXPopup`/`FXMenuPane`: build commands once. `popup(grabto,rootx,rooty)` takes
  root coordinates. Clamp popup position if it is near a screen edge.
- Right-click contexts should use `SEL_RIGHTBUTTONPRESS` plus a popup/menu built
  once. Put mode-independent commands in the menu itself; add optional entries
  by mode and use `SEL_UPDATE` for checked/enabled state.
- `FXMenuCheck` is the appropriate control for a checked **Show title bar** or
  other persistent on/off command.

Check `popup.cpp`, `menus.cpp`, `dialog.cpp`, `wizard.cpp`, and the specialized
color/font dialog tests before adding new dialog infrastructure.

## Custom drawing and OpenGL

For custom drawing, use an `FXCanvas` target/selector and map `SEL_PAINT` to the
owning object, as in `scribble.cpp`:

- Construct `FXDCWindow dc(window,event)`.
- Clear/fill the exposed area first.
- Set font/foreground before drawing text.
- Use `update()` to invalidate and `repaint()` only when immediate repaint is
  actually required.
- On resize, let FOX update shell dimensions and then layout children; do not
  call Win32 APIs from an application.
- Store application geometry in model coordinates; draw from `width`/`height`.

`dctest.cpp` demonstrates line width/style, joins, caps, patterns, arcs and
text. Use it before adding custom drawing primitives. Use `FXGLCanvas` plus
`FXGLVisual` for OpenGL; see `gltest.cpp`/`glcanvas.cpp`.

Direct shell painting is acceptable for a deliberately minimal display. For
multiple interactive parts, switch to `FXFrame`/`FXSwitcher`/`FXLabel`/controls
and avoid drawing button-like interaction by hand.

## Timers and live displays

Use `FXApp::addTimeout(target,ID_TICK,nanoseconds)` and a `SEL_TIMEOUT` handler.
Prefer timestamp-based state over counting ticks. Re-arm exactly once after
handling the timeout, remove the timeout in the destructor/close path, and use
`steadytime()` for elapsed durations where available.

For a digital clock, update to the next second. For stopwatch hundredths, use
about 50 ms only while the stopwatch page is visible/running. A hidden running
model still needs only its correctness timestamp, not a high repaint frequency.

## Data binding versus explicit handlers

Use `FXDataTarget` from `datatarget.cpp` for a scalar shared by controls. It is
appropriate for an integer mode, numeric spinner, string, color, or simple
progress value when no side effect is required.

Use explicit handlers when a change must:

- switch an `FXSwitcher`,
- start/stop timers,
- persist history/settings,
- update several dependent states,
- transform or validate a value.

Do not mix a data target and explicit writes to the same value without a clear
ownership rule.

## Window, popup, clipboard and DnD rules

- Borderless or decorated movement: inspect `FXTopWindow` and the current
  borderless/drag seam before inventing native code. Win32/X11 manual movement
  uses application `move()`; compositor-driven Wayland uses `startDrag()` when
  available.
- Position/size persistence: save from `close()`, a completed move/resize, or a
  deliberate configure action. Restore before first show and clamp to visible
  screen bounds.
- Right-click menus are separate from `FXMenuPane` command history; make the
  menu own the event even when the face widget receives the original press.
- Clipboard: map request/lost/clipboard selection messages exactly as
  `clipboard.cpp` does. Provide data in a registered drag type.
- DnD: implement begin/can-drop/drop/request/end handlers and keep the drag
  payload valid for the operation; use `dnd.cpp`.

## Resource ownership

- Widgets own child widgets through the FOX widget tree.
- Explicitly owned `FXIcon`/`FXFont` objects must be created once, shared only
  intentionally, and deleted/detached by the owner.
- Do not delete a button icon while the button still exists.
- Use `FXIconSource::loadIconFile()` only when the path is deterministic after
  installation; prefer embedded SVG/icon data for a self-contained app.
- Bundle icons through app build/distribution files or embedded generated
  bytes. Do not depend on the checkout working directory.

## Console-only tests

`fox2/tests/` also contains display-free tests for codecs, containers, regex,
math, formatting, time, Unicode, JSON/variant, processes, threading and XML.
Use these for unit-level library behaviour and `--test`/`--no-dialogs` style
checks. Do not use a GUI screenshot test for non-visual algorithms if a concise
console test fits the same source pattern.

## Verification checklist

- Build the exact target in the supported backend first (Win32 MSYS2 for the
  current stopwatch work).
- Launch with a private settings directory.
- Capture a screenshot from the real window after the first paint.
- Verify creation, movement/resize, mode switching, close and restart.
- Exercise both default and persisted state.
- If a source test includes `--no-dialogs`, keep screenshot gating compatible
  with it rather than making every test interactive.

## Specialized controls and displays

- `datatarget.cpp` is the primary reference for `FXSlider`, `FXRealSlider`,
  `FXDial`, `FXKnob`, `FX7Segment`, `FXProgressBar`, `FXProgressDialog`,
  `FXSpinner`, `FXRealSpinner`, and data-target binding.
- `groupbox.cpp` is the reference for `FXRangeSlider`, `FXArrowButton`,
  `FXScrollBar`, `FXListBox`, and `FXTreeListBox`.
- `colordemo.cpp` demonstrates `FXColorBar`, `FXColorWheel`, and `FXColorRing`.
  `image.cpp` additionally demonstrates `FXGradientBar` and `FXRuler` in a
  composed UI. Use `colordlg.cpp` for `FXColorWell` and modal color selection.
- `gaugetest.cpp`/`gaugedemo.cpp` demonstrate gauges and a slider-to-display
  update path.
- `bitmapviewer.cpp` is the reference for `FXToolBar`, `FXToolBarTab`, docking
  commands (`ID_DOCK_FLOAT`, `ID_DOCK_TOP`, `ID_DOCK_LEFT`, `ID_DOCK_RIGHT`,
  `ID_DOCK_BOTTOM`), status bars, recent files, and file/image dialogs.
- `button.cpp`, `foursplit.cpp`, `bitmapviewer.cpp` and `dctest.cpp` instantiate
  `FXToolTip` and `FXStatusBar` in complete window layouts.
- `rulerdemo.cpp` is the dedicated `FXRuler`/`FXRulerView` pattern; do not use
  a ruler's drawing as a substitute for a general header or separator.

## When no dedicated test exists

Consult the nearest complete test and then the current header/implementation:

| Widget family without a dedicated test | Start with |
| --- | --- |
| `FXFileDialog`, `FXDirDialog`, `FXFileSelector`, `FXDirBox`, file progress | `bitmapviewer.cpp`, `imageviewer.cpp`, dialog headers |
| `FXDockBar`, `FXDockSite`, `FXDockTitle`, `FXToolBarShell`, `FXToolBarGrip` | `bitmapviewer.cpp` toolbar/dock setup, then `FXToolBar*.h` |
| `FXMessageBox`, `FXInputDialog`, `FXChoiceBox`, `FXSearchDialog`, `FXReplaceDialog` | `dialog.cpp` plus the exact dialog header |
| `FXFontSelector` | `fontdlg.cpp`, `dctest.cpp` font code |
| `FXColorSelector` | `colordlg.cpp`, `colordemo.cpp` |
| `FXDriveBox`, `FXPathBox` | `dirlist.cpp` and the composed file/directory controls |
| `FXBitmapView`, `FXImageView`, `FXImageFrame`, `FXBitmapFrame` | `bitmapviewer.cpp`, `imageviewer.cpp`, `image.cpp` |
| Menu component rows (`FXMenuCheck`, `FXMenuRadio`, caption/title/command/separator) | `menus.cpp`, then `FXMenu*.h` |

If no test renders the exact widget, implement the smallest example in an
existing relevant test category only when needed; otherwise cite the closest
reference and validate with a screenshot. Never claim coverage merely because
the header exists.
