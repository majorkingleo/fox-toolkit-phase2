# Calculator port plan

Port the FOX 1.7.95 `calculator` application to `fox2` unchanged in behavior. It
is the first real application on the ported library and the acceptance test for
everything beyond `hello` and `button`.

## Why this app

- 3.3k lines of ordinary application code in eight source files (`Calculator`,
  `Preferences`, `HelpWindow`, `main`, `help`) plus the generated icon tables;
  it uses only the public FOX API and no library internals.
- It exercises what the theme and DPI workstreams still need to validate: many
  widgets side by side, several fonts, per-widget colours, nested dialog pages,
  a text display and bitmap icons.
- It produces a demo binary for manual DPI/theme checks and a second GUI target
  for the Wayland adapter besides `tests/`.
- Its widget demands are typical, so the work it forces is the work any real
  FOX application would force.

## Baseline (do first, before any porting)

`scripts/build-fox1-linux.sh` already builds the original application in
`tmp/build-fox1/calculator/calculator`. That binary is the reference.

- Capture `screenshots/tests/fox1/x11/calculator-main.png`,
  `calculator-prefs-calculator.png`, `calculator-prefs-colors.png`,
  `calculator-prefs-about.png` and `calculator-help.png` with
  `import -window <title>`.
- Record the window titles and geometries, and the arithmetic/behaviour
  checklist below as the reference result.
- Behaviour checklist: `2+3=`, `sqrt`, `1/x`, memory store/recall, base
  hexadecimal, `RAD`/`DEG` angle mode, precision change, each preferences page,
  colour change, font change, help window, and clean exit.

## Rules

- Copy from `fox-1.7.95/{lib,include,calculator}` and do not reformat; the
  library files keep their original text so `diff` against the origin stays
  meaningful.
- One stage per commit. A stage is accepted only when the X11 build
  (`scripts/build-fox2-linux.sh`), the Win32 cross build
  (`scripts/build-fox2-mingw-cross.sh`) and `hello`, `button`, `dpi` still run.
  Wayland (`scripts/build-fox2-wayland.sh`) must keep building once a stage
  touches widgets it already uses.
- Add nothing the application does not need. The goal is the application, not
  widget-set completeness.
- Every ported class touches five places: the source in `fox2/lib`, the header
  in `fox2/include`, `fox2/lib/Makefile.am`, `fox2/lib/CMakeLists.txt`, and the
  include list in `fox2/include/fx.h`. A forgotten entry shows up as a link
  error, so the stage build is the check.

## What is missing

`fox2` currently carries 239 of the 507 library sources. The application needs
these classes, which `fox2` does not have yet (line counts from
`fox-1.7.95/lib`):

| Stage | Classes | Lines | Depends on |
| --- | --- | --- | --- |
| 1 | `FXMatrix`, `FXSwitcher`, `FXHorizontalSeparator`, `FXDialogBox` (+ source) | 524, 223, 196, 117 | `FXSeparator`, `FXPacker`, `FXTopWindow` (present) |
| 2 | `fxbmpio.cpp`, `FXBMPImage`, `FXBMPIcon` | 1390, 91, 94 | `FXImage`, `FXIcon` (present) |
| 3 | `FXScrollArea`, `FXScrollBar`, `FXText`, `FXTextField`, `FXArrowButton`, `FXSpinner` | 535, 1139, 6495, 2359, 602, 656 | stages 1–2, `FXFrame`/`FXLabel` (present) |
| 4 | `FXList`, `FXScrollWindow`, `FXMenuButton`, `FXComboBox` | 2083, 259, 735, 800 | stage 3 |
| 5 | `FXColorWell`, `FXColorWheel`, `FXColorRing`, `FXColorBar`, `FXColorList`, `FXPicker`, `FXTabBook`, `FXTabItem`, `FXColorSelector`, `FXColorDialog` | 751, 477, 720, 411, 216, 227, 585, 310, 836, 273 | stages 1 and 4 |
| 6 | `FXFontSelector`, `FXFontDialog` | 784, 164 | stages 3 and 4 |
| — | `FXBitmap` (only if a link or a drawing path needs it) | 933 | — |

`FXScrollCorner` is declared in `FXScrollArea.h`, `FXListItem` in `FXList.h`,
`FXColorItem` in `FXColorList.h`, so each list stage ports its item class with
it. `FXTextField.h` and `FXDialogBox.h` already exist in `fox2/include` but
their sources do not.

## Stages

### Stage 0 — reference capture

Run the original application, capture the screenshots and record the behaviour
checklist results. No `fox2` change. Exit: the reference images and notes exist
under `screenshots/tests/fox1/x11/`.

### Stage 1 — containers and separators

`FXMatrix`, `FXSwitcher`, `FXHorizontalSeparator` (and
`FXVerticalSeparator` from the same source), `FXDialogBox` source.
Exit: both builds pass; `tests/menus` and `tests/popup` still pass, which
exercise the popup and dialog path that `FXDialogBox` uses.

### Stage 2 — bitmap icons

`fxbmpio.cpp`, `FXBMPImage`, `FXBMPIcon`. The application uses `FXBMPIcon` for
the memory indicator (`constmem.bmp`).
Exit: both builds pass; add `tests/bmpicon.cpp` only if no existing test covers
BMP loading.

### Stage 3 — scrolling and text

`FXScrollArea`, `FXScrollBar`, `FXText`, `FXTextField`, `FXArrowButton`,
`FXSpinner`. This is the biggest stage (11.8k lines) and the one most likely to
surface missing pieces in already ported code (text metrics, `FXFont`, theme
roles for selection colours).
Exit: both builds pass; a smoke test that creates an `FXText` and an
`FXSpinner` and exits (like `tmp/t1-check.cpp`) prints the expected geometry.

### Stage 4 — lists and combos

`FXList`/`FXListItem`, `FXScrollWindow`, `FXMenuButton`, `FXComboBox`.
Exit: both builds pass; a smoke test opens a combo box popup and closes it.

### Stage 5 — colour dialog

`FXColorWheel`, `FXColorRing`, `FXColorBar`, `FXColorList`/`FXColorItem`,
`FXPicker`, `FXTabBook`/`FXTabItem`, `FXColorSelector`, `FXColorDialog`,
`FXColorWell`.
Exit: a smoke test opens `FXColorDialog` from an `FXColorWell` click and closes
it; the Linux and Win32 builds pass.

### Stage 6 — font dialog

`FXFontSelector`, `FXFontDialog`. Font enumeration uses the font code path that
the DPI workstream already touched, so this stage also verifies that
`FXFont::listFonts()`-style enumeration still works after the per-DPI font
cache.
Exit: a smoke test opens the font dialog and lists faces.

### Stage 7 — the application

- New `fox2/apps/` with `apps/Makefile.am` and `apps/calculator/`.
- Copy `Calculator.{h,cpp}`, `Preferences.{h,cpp}`, `HelpWindow.{h,cpp}`,
  `help.cpp`, `main.cpp`, `LICENSE`, the six images and `calculator.1`.
- Reuse the checked-in `icons.cpp`/`icons.h`; the `reswrap` rule of the
  original `Makefile.am` stays out until `fox2/utils/reswrap` is ported, so the
  build does not depend on a tool that does not exist yet.
- Add `calculator` to `configure.ac` (`AC_CONFIG_FILES`) and `SUBDIRS`.
- Build and fix API drift only: the application code is copied, not rewritten.
Exit: `apps/calculator/calculator` builds on X11 and as a Windows cross build.

### Stage 8 — behaviour verification

- X11: walk the Stage 0 checklist in the ported application and compare the
  screenshots with the reference. With the classic theme the difference must be
  0 pixels per the compatibility policy; any difference is either a porting bug
  or a theming decision that must be recorded.
- Win32: run the same checklist under Wine from
  `scripts/build-fox2-mingw-cross.sh --run` output, including the dialogs.
- Wayland: add `scripts/gate-wayland-calculator.sh` that starts the application
  on the headless compositor, injects a few clicks with `tools/wlxinject.c`
  and compares a screenshot with the X11 one.
Exit: all three backends pass the checklist; differences are documented.

### Stage 9 — record

- Update `docs/general-plan.md` (the port is the new "real application"
  milestone) and the modernization status line.
- Add `scripts/run-fox2-calculator.sh` as the repeatable manual test.
- Note the remaining widget-set gaps that the application did not need.

## Risks

- **`FXText` is the hard part** (6.5k lines) and `FXTextField` plus `FXSpinner`
  sit on top of it. Expect most debugging here; budget accordingly.
- **Font enumeration**: `FXFontSelector` depends on listing faces through the
  font code the per-DPI cache changed. If enumeration regressed, fix it in the
  font seam, not in the application.
- **Generated icons**: keeping the checked-in `icons.cpp` avoids porting
  `utils/reswrap` (699 lines) now; port it when the generated file must change,
  and keep both build systems in sync then.
- **Theme drift**: fox2 draws buttons and labels through `FXStyle`. The classic
  theme must keep the reference pixels; if the application exposes a themed
  widget that drifts, that is a theme bug found by this port, not a reason to
  change the application.
- **Build-system drift**: three lists (`Makefile.am`, `CMakeLists.txt`,
  `fx.h`) must stay in sync per class; the stage builds catch omissions.

## Effort

About 28k lines copied, almost all mechanically; the actual work is the five
integration points per class, the link-order surprises, and Stage 3/5/6
debugging. Stages 1, 2 and 4 are short; 3, 5 and 6 dominate.

## Result

All stages are done. `fox2/apps/calculator` builds with Autotools and CMake,
its X11 window reproduces the Stage 0 screenshot with 0 differing pixels, the
arithmetic checklist (`2+3=`, `sqrt`, `1/x`, memory, hexadecimal, `RAD`/`DEG`)
produces the same display strings, the three preferences pages, the font
dialog and the help window match, and the Win32 build passes the same checklist
under Wine. `scripts/gate-wayland-calculator.sh` runs the application on a
headless compositor, clicks with the virtual pointer and compares the frame
with the X11 reference (0 of 104000 pixels differ); `scripts/run-fox2-calculator.sh`
runs it against the current display with a private HOME.

The port and the first real desktop use found and fixed these library bugs:

- the classic style drew button and frame chrome with theme colours instead of
  the widget's own highlight/shadow/border colours, which changed every button
  the calculator colours by group;
- the Wayland event queue grew only when the queued count reached the
  allocation but wrote at the tail index, so a window with ~100 children wrote
  past the queue;
- the software rasterizer accepted an expose rectangle larger than its buffer,
  so a compositor resize crashed it while repainting;
- the Wayland toplevel icon was not square, which KWin rejects with
  `invalid_buffer` and a closed connection, so no window appeared on KWin;
- a pending activation token was destroyed before its `done`, which also cost
  the connection;
- the Wayland backend never set a pointer cursor, so a text field kept the
  default arrow;
- Wayland dispatched keys straight to the focused widget instead of to the
  shell, so the accelerator table was never consulted and the calculator's
  digit and operator hot keys did nothing;
- a repaint queued while a widget was visible was still dispatched after the
  widget had been hidden, so the first frame of the preferences dialog showed
  the hidden colour page painted over the settings page;
- the Wayland backend filtered neither pointer nor key events by the modal
  invocation, so the calculator stayed live behind its own preferences dialog;
  the X11 and Win32 backends have always dispatched only to the modal window
  and its popups (and beep outside them), and Wayland now does the same;
- the Wayland pointer hit test searched every shell at once with the place the
  window table remembered, but a compositor positions a toplevel and never
  tells the client where: every shell sat at the origin, so a click on one
  window could resolve to a widget of the top-most one.  A pointer event is
  now hit-tested inside its own surface (`FXWinMap::hitTest(shell,x,y)`), and
  the screen-wide query is left to the backends that know the places;
- the Wayland wheel did not flip the sign of `wl_pointer.axis`, so the help
  window scrolled the wrong way: the protocol counts a positive vertical axis
  towards the end of the document, FOX counts a positive wheel towards the top;
- after the calculator was expanded, the first frame committed at the new size
  still showed the buttons at their old places, and stayed on screen until a
  later expose; the configure listener exposed the tree before the core knew
  the new size, and the configure dispatch exposed it before the delayed layout
  chore had run.  The dispatch now lays the tree out before exposing it;
- closing the main window while the preferences dialog ran its modal loop
  crashed the application: the close destroyed the window, and with it the
  title button whose command was still on the stack running the modal loop, so
  the button handler resumed on freed memory.  A close request for a window
  that the modal invocation blocks is now ignored, and the dialog names its
  owner (`xdg_toplevel.set_parent`) and is marked modal through `xdg-dialog-v1`
  where the compositor offers it.

The calculator's `main()` also needed the Wayland signal seam
(`FXApp::addSignal`), which the other backends had.

## Remaining widget-set gaps

`fox2` now carries 272 C++ sources in `lib/` (239 before the port). Widgets a next application
would likely miss (source file absent): `FXBitmap` and its views, the PNG,
JPEG, TIFF and other image codecs, `FXFileDialog`/`FXDirDialog`/
`FXInputDialog`/`FXChoiceBox`, `FXMessageBox`, `FXProgressBar`, `FXListBox`,
`FXTreeList`, `FXTable`, `FXSplitter`/`FX4Splitter`, `FXToolBar`/`FXDockBar`/
`FXMenuBar` (their headers exist, the implementations do not), `FXSwatch`,
`FX7Segment`, `FXScrollPane`, `FXSpring`, `FXRealSpinner` and `FXCalendarView`.
`utils/reswrap` is still unported, so the calculator keeps its checked-in
`icons.cpp` and `icons.h`.
