# Porting the remaining FOX 1.7.95 test applications — plan and status

`fox-1.7.95/tests` holds 48 programs. `fox2/tests` has the console programs
among them and the GUI programs `button` and `hello`, plus tests written for
fox2 itself. This plan ports the GUI programs that are still missing, in
groups, one commit per group. A test is ported when it builds on all
platforms, runs without a person, and leaves a screenshot behind.

## Why port them

The tests written for fox2 so far (`click`, `popup`, `menus`, `tooltip`, `dnd`,
`dpi`, `idle`, ...) were each made for one feature. The old programs cover the
widgets nobody has looked at since the seam refactor: splitters, tab books,
tables, file lists, MDI, drawing contexts. They are the cheapest regression
check for the platform seam on Win32, X11 and Wayland.

## What is left

Ported or replaced already: `button`, `codecs`, `dictest`, `expression`,
`format`, `half`, `hello`, `match`, `math`, `memmap`, `minheritance`,
`parallel`, `process`, `rex`, `scan`, `thread`, `timefmt`, `unicode`, `units`,
`variant`, `xml`, plus the fox2 tests listed above.

Not ported (26), grouped by what they exercise:

| Group | Programs | What they exercise |
| --- | --- | --- |
| 1 Layout containers | `hello2`, `groupbox`, `layout`, `splitter`, `foursplit`, `switcher`, `tabbook`, `header`, `gaugetest`, `ratio` | packing, per-window DPI metrics, scroll bar and slider scaling |
| 2 Dialogs and data | `dialog`, `wizard`, `datatarget`, `calendar` | modal windows, focus, owner relations, data targets |
| 3 Lists and files | `dirlist`, `iconlist`, `table` | icons, scrolling, many items, file system access |
| 4 Drawing | `image`, `imageviewer`, `bitmapviewer`, `scribble`, `dctest` | `FXDCWindow`, `FXImage`, image codecs, file dialogs |
| 5 Special cases | `mditest`, `shutter`, `gltest`, `console` | MDI children, shutter widget, OpenGL, a program with no window |

`gltest` needs an OpenGL seam that does not exist yet (no GL canvas on
Wayland or Win32 in fox2). It stays last and may be left out; the decision is
taken when group 4 is done. `console` is a console program and belongs with the
console tests, see step 0.

## Step 0: groundwork

1. List the console tests that fail today and why; fix them or mark each one
   as a known failure with a reason. Console tests need no display, so they
   are the first thing a CTest run should be able to trust.
2. Register every test with CTest (`add_test`), using `--no-dialogs`, and a
   timeout. Console tests run to completion. GUI tests that quit by
   themselves (like `idle`) run to completion; the others get a `--timeout`
   switch in `testargs.h` so that they quit after N seconds and return 0, and
   a run that hangs is a failure. The existing build scripts keep doing
   `timeout 3` for `hello`, `button` and `dpi`.
3. Decide how a ported program gets its resources. The old programs embed GIF
   data produced by `reswrap` (`hello2`, `groupbox`, `header`, `iconlist`,
   `shutter`, `dctest`). The data stays embedded, as in the original, so
   that a test needs no file; the GIF codec in fox2 reads it. `dippy.h` is
   copied along with `dctest`.
4. Add a `scripts/capture-test-screenshots.*` pattern, or extend the existing
   capture scripts, so one command stores a screenshot per program per
   platform under `screenshots/tests/fox2/<platform>/<program>.png`.

Acceptance: `ctest` runs all existing tests on Windows and in WSL, and the
result is a list of passes plus a short, explained list of known failures.

## Procedure for each program

1. Copy the program from `fox-1.7.95/tests` to `fox2/tests`; never touch the
   original.
2. Apply the FOX formatting rules of the repository (see
   `.github/copilot-instructions.md`); adapt includes, `fxtestargs()`, and any
   API that changed in fox2.
3. Add it to `fox2/tests/CMakeLists.txt` and `fox2/tests/Makefile.am`, and to
   CTest.
4. Build and run it:
   - Windows: `scripts/build-fox2-mingw.ps1 -CMake -Fast -Run`.
   - Linux X11: `scripts/build-fox2-linux.sh --cmake --fast --run` in WSL.
   - Wayland: `scripts/build-fox2-wayland.sh` where the library links (WSL
     lacks FreeType and fontconfig development files, so the link step is only
     possible on the development machine).
5. Take the screenshot on each platform that can run it. Look at it: a
   program that runs but draws wrongly is a finding, not a pass.
6. Record findings. A defect in the library is fixed in `fox2/lib` in its own
   commit, before the group commit, with the test that shows it. A defect that
   is already in `fox-1.7.95` is written down in the table below and left.

## Order and gates

| Step | Content | Gate |
| --- | --- | --- |
| 0 | Groundwork above | `ctest` green or explained |
| 1 | Group 1, layout containers | all run on Windows and X11; screenshots at 100% and at 150% scale (`tests/dpi` style registry setting) agree on proportions |
| 2 | Group 2, dialogs and data | modal dialogs block their owner, `Esc` and the close box answer correctly, no hang when the owner is closed |
| 3 | Group 3, lists and files | file list shows the working directory; no leak of icons when the list is reloaded; icons are crisp at 150% (this is where the DPI keyed icon caches pending in `docs/dpi-plan.md` are needed) |
| 4 | Group 4, drawing | `dctest` output identical on Win32 and X11 within the tolerance the `raster` test already uses |
| 5 | Group 5 | decided per program, see above |

Wayland runs follow each step on the development machine, using the existing
`scripts/gate-wayland-*.sh` style: a program that needs a pointer or key
sequence gets a gate of its own only if the behaviour is not covered by one
of the earlier gates.

## Findings

Filled in while porting: one line per defect, with the program that showed it
and the commit that fixed it.

| Program | Defect | Fix |
| --- | --- | --- |
| | | |

## Status

| Step | State |
| --- | --- |
| 0 Groundwork | not started |
| 1 Layout containers | not started |
| 2 Dialogs and data | not started |
| 3 Lists and files | not started |
| 4 Drawing | not started |
| 5 Special cases | not started |
