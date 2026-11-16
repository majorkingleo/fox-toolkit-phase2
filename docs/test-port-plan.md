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

1. Fix the console tests that fail today, see "Console test inventory" below.
   Console tests need no display, so they are the first thing a CTest run
   should be able to trust.
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

### Console test inventory

Measured on 2026-11-16 by running every console test of the CMake build with
`--no-dialogs`, stdin closed, a 60 s limit, from `fox2/tests`; Windows is the
MinGW build, Linux is X11 in WSL. A result of "exit 0" says only that the
program ended, not that it checked anything.

Most of these programs are not tests but drivers inherited from FOX 1: they
print, they need arguments or a word list on stdin, they wait for a key, or
they run for hours. A program that returns `1` at the end of `main()` counts
as failed although it did its work. The rule from now on: a console test
takes no input, finishes within seconds, prints what it did, and its exit
code is 0 if and only if every check held.

Defect kinds: **A** library defect; **B** exit code always wrong; **C** needs
input, arguments or a person; **D** runs too long; **E** passes but checks
nothing; **F** self-checking and passing.

| Test | Windows | Linux | Kind | Why | Fix |
| --- | --- | --- | --- | --- | --- |
| `match` | exit 1 | exit 1 | A, B | `return 1` at the end; on Windows one real `BAD`: `A*` on `a/b` with `CaseFold|PathName` gives true | count the `BAD` lines and return their number; find out why `PathName` ignores `/` on Windows and fix `FXPath::match` |
| `codecs` | exit 1 | exit 1 | B, D | `return 1` at the end; only prints `Problem:` lines; the performance part fills 100 MB and takes long | count problems and return them; run the performance part only with `--perf` |
| `minheritance` | exit 1 | exit 1 | B, E | `return 1` at the end; prints the calls, checks nothing | compare the `this` pointers and the values it passes, return the number of mismatches |
| `scan` | exit 1 | exit 1 | B, E | `return 1` at the end; prints the results of `strtod`, the FOX `__strtod` and `__sscanf` side by side | return the number of lines where the FOX result differs from the C library where the C library is correct |
| `unicode` | exit 1 | exit 1 | B, E | `return 1` at the end; dumps 1.2 million lines of the character table | add a handful of known values (category, case mapping, decomposition) and check them; keep the dump behind `--dump` |
| `memmap` | exit 0 | exit 0 | C | prints usage and exits 0 when it gets no `read|write <length> <file>`; with them it waits in `getchar()` | with no arguments write a temporary file, map it, check the contents, unmap, delete it |
| `dictest` | timeout | SIGFPE | C, B | reads words from stdin; with none it divides by zero (`tot/ins`); the Windows run waits for stdin | generate the keys itself (a fixed seed), check insert, lookup, replace, remove, and the size after each |
| `rex` | exit 1 | exit 1 | C | needs `pattern [string]` on the command line | built-in table of pattern, flags, subject, expected match range and captures; keep the command line mode |
| `expression` | exit 0 | exit 0 | C, E | evaluates the expression given on the command line; no argument, nothing happens | built-in table of expressions and expected values |
| `units` | exit 0 | exit 0 | C, E | converts what the command line says; nothing without it | built-in table of conversions with expected values |
| `process` | exit 1 | exit 1 | C | starts `wordpad.exe` on Windows and `/usr/local/bin/adie` elsewhere, which usually do not exist | start the test program itself with a child mode argument, wait, check the exit code it set and its output |
| `thread` | timeout | exit 1 | C, B | job producer and pool; `return 1`; the Windows run never ends (stdin is not closed by the runner) | no `getchar()`; run a fixed number of jobs, count the ones that ran, return 0 if all did |
| `parallel` | timeout | exit 0 | C | starts a pool, then `getchar()` | the same; the existing `--pool`, `--loop`, `--invoke` modes keep running and check their results |
| `math` | timeout | timeout | D, B | tests every one of the 2^32 floats and 10^9 random doubles per function, which takes hours; `report()` prints the worst error and a count, but the exit code ignores it | give each function a bounded sample (fixed seed) by default, `--exhaustive` for the full run, and fail when the error exceeds a per-function limit in ulps |
| `format`, `half`, `timefmt`, `variant`, `xml` | exit 0 | exit 0 | E | print results, check nothing; `xml` reads `xmltests.json` | compare with known results: for `format` the output of the C library, for `half` the conversions of known bit patterns, for `timefmt` and `variant` round trips, for `xml` and `variant` the parse of the supplied files; list each in the commit that does it |
| `desktop` | exit 0 | exit 0 | E | prints the scheme, no check | none needed; it reads the system and has no expected value |
| `glyphs`, `region`, `winmap` | exit 0 | exit 0 | F | already self-checking | none |

The FOX 1 originals are used as the oracle: where a ported test and the same
program in `fox-1.7.95/tests` print different results, the difference is
looked at before either is changed. The originals may be edited to return a
useful exit code and to detect failure (they stay in `fox-1.7.95/tests` and
change nothing else); the library under them is never touched.

### Console test fix order

1. B and A: `match`, `codecs`, `minheritance`, `scan`, `unicode`. These only
   need a count and a return value, and `match` has a defect behind it.
   Done: all five return 0 on Windows and Linux. The `match` defect was in the
   test, not in the library: with `PathName` the separator is `PATHSEP`, which
   is `\` on Windows, so the test now builds its subjects with
   `PATHSEPSTRING`. `codecs` runs the performance part only with `--perf`,
   `unicode` dumps its table only with `--dump`.
2. C: `dictest`, `memmap`, `process`, `thread`, `parallel`. They get a
   self-contained default run.
   Convention: a program that reads stdin or takes arguments keeps that mode
   for benchmarking and manual use, and gets a `--test` switch that runs a
   generated, fixed data set through the class and checks every result. CTest
   runs it with `--test`. A class that already has its own unit test needs no
   such switch. Done: `dictest --test` (no unit test for `FXDictionary`
   existed; it checks insert, replace, lookup, remove, reuse of voided slots,
   copy, assign, adopt, erase, shrink and clear), and without `--test` an
   empty stdin no longer divides by zero. Done: `memmap --test` (writes a
   temporary file through a map, reads it back whole and at an offset, changes
   it through a read-write map, grows it through a larger map, checks sizes
   and contents, removes the file; the interactive `read`/`write` modes stay,
   and the program now returns 0 at the end). Done: `process --test` (the
   program starts itself as a child: exit codes 0, 3 and 42, captured stdout
   and stderr through pipes, a given environment, kill of a sleeping child,
   a program which does not exist, wait without a start). It found two
   library defects, both fixed in fox2 only: on POSIX `FXProcess::wait(code)`
   returned the raw `waitpid` status (42 came back as 10752; now the exit
   code, or minus the signal number), and on Windows `FXPipe`, `FXFile` and
   `FXMappedFile` made a handle inheritable exactly when `FXIO::Inheritable`
   was not given (inverted; FOX 1 has the same), plus the non-Unicode
   `FXProcess::start` took the stderr handle from the output stream.
3. C and E: `rex`, `expression`, `units` with tables of expected results.
4. D: `math` with a bounded default.
5. E: `format`, `half`, `timefmt`, `variant`, `xml`.

One commit per step, each with the result of both platforms in its message.

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
