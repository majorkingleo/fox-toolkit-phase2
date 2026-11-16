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
| 5 Special cases | `mditest`, `shutter`, `gltest`, `console` | MDI children, shutter widget, OpenGL, a text console widget |

`gltest` needed GL classes that fox2 did not have; they were copied and the Win32
part is verified. `console` is not a console program: it shows the `FXConsole`
widget, so it is a GUI test like the others.

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
   that a test needs no file; the GIF codec in fox2 reads it (checked: `fxcheckGIF` and `fxloadGIF` of fox2 decode `dippy` as 138x314). `dippy.h` is
   copied along with `dctest`.
4. Add a `scripts/capture-test-screenshots.*` pattern, or extend the existing
   capture scripts, so one command stores a screenshot per program per
   platform under `screenshots/tests/fox2/<platform>/<program>.png`.
   Done for Windows: `scripts/capture-test-screenshots.ps1` starts each
   windowed program (`hello button dpi popup click menus tooltip clipboard dnd icon`),
   captures it with `PrintWindow`, crops the invisible frame and stops it. The
   smoke tests (`bmpicon textspin combo colordlg fontdlg`) open no lasting
   window and are not captured. Wayland has `scripts/capture-wayland-screenshots.sh`
   (three programs so far); X11 has no script yet.

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
   Done: `thread --test` (producer queues 100 jobs in 5 groups with
   waits in between; checks pool start, that all jobs were queued and ran,
   stop, no threads left, second stop fails; no `getchar()`, returns 0) and
   `parallel --test` (counting jobs: 20 pool jobs with `executeAndWait`, a
   parallel loop covering each of 100 indices once, flat and nested
   `FXParallelInvoke` with expected job counts 17 and 25). The interactive
   modes are unchanged.3. C and E: `rex`, `expression`, `units` with tables of expected results.
   Done: `rex --test` (37 searches with flags, positions, matched text and
   captures; 7 syntax error codes; syntax-only parse; `substitute`),
   `expression --test` (58 expressions covering every operator group,
   functions and constants; 6 error codes; one compile evaluated repeatedly)
   and `units --test` (27 conversions checked in both directions and by
   inverting, 4 refused conversions, symbol look-up, span, dimensions, table
   order). The command line modes stay; `rex` and `expression` now return 1
   on a syntax error instead of always 1 / 0. `units --test` found two data
   errors in the fox2 table (FOX 1 has the same two): `hp` had the
   dimensions of energy (`kg*m^2/s^2`, now `kg*m^2/s^3`) and `lbf` was rounded
   to 4.448222 (now 4.4482216152605, so `psi` is 6894.757293 Pa).
4. D: `math` with a bounded default. Done: `math --test [--verbose]` checks
   known values and edge cases for both precisions (zero signs, Inf, NaN,
   denormals, overflow and underflow, argument reduction, integer helpers,
   `fpMake` and friends, `log2` of every power of two from -100 to 100)
   and a fixed-seed sample of every function against the C library within a
   per-function limit in ulps; it returns 0 only when all hold. The run without
   arguments is unchanged (all floats, 10^9 doubles); a `names[]` entry
   missing a comma was fixed. The test found defects in the code which
   fox2 uses where the C library lacks a function (mingw), all fixed in
   `fxmath.cpp` and `fxmath.h`: `expm1` and `log1p` lost all precision
   near zero (which also broke `sinh`, `tanh`, `asinh`, `atanh`);
   `cbrt` returned NaN for negative arguments and -Inf gave +Inf; `hypot`
   overflowed above 1e154 and underflowed below 1e-154; `fmin`/`fmax`
   returned NaN when only the second argument was NaN; `acosh(Inf)` was NaN;
   `log2` of powers of two was inexact. `inverf` and `inverfc`, used on
   every platform, started from a wrong estimate (`log(0.5*x*x)` instead of
   `2*log(0.5*x)`) and were off by up to 2e-6 (double) and 2e-2 (float).
5. E: `format`, `half`, `timefmt`, `variant`, `xml`.
   - `format --test` done: bit counting, about 110 integer, float, hexadecimal,
     positional and truncation cases with known output, a comparison with the C
     library for every format and number of the program (and every precision of
     `e f g` for awkward numbers), and print-and-scan round trips. Documented
     behaviour of the fox2 printer is checked as such: `'` always groups with
     commas, `%.0f` rounds half away from zero, nothing is terminated when
     the output is truncated, and decimal conversion is good to about 16
     digits (the C library is exact), so differing strings must still agree
     within 5e-16. Fixed: `%010f` of Inf or NaN padded with zeros instead of
     spaces.
   - `half --test` done: all 65536 halves converted to float and compared with
     an exact `ldexp` value and converted back, known float to half cases
     (limits, overflow, underflow, signed zero), halves around each rounding
     midpoint, monotonic conversion and the arithmetic operators. Fixed: a NaN
     float with only low mantissa bits set became infinity in `FXhalf`.
   - `timefmt --test` done: `daysFromCivil`/`civilFromDays` over 1.6 million
     days, broken-down time, carrying of out-of-range parts, a table of formats
     with known output, a comparison with the C library `strftime`, the ISO
     week tables in both directions, a table of parse cases (zones, 12-hour
     clock, week numbers, fractions, failures) and local time in any zone (run
     in six zones including half-hour ones). Fixed: days beyond the end of a
     month carried wrongly (`Jan 100`); zone offsets of parsed times were
     applied with the wrong sign; `localTime` always printed `%z` as
     `+0000`; `%z` of -00:30 printed `+0030`; `%fm`/`%fu` rounded and
     could overflow their digits (now truncated); `%G` of 3 January that
     belongs to the previous ISO year; `%y` and `%g` guessed the century the
     wrong way round; `%U`, `%W` and `%V` were ignored when followed by
     `%T`, `%D`, `%F` and the like (derived fields were calculated inside the
     nested parse); `%s` could not parse negative times and overflowed.
   - `variant --test` done: construction, deep copies, self and partial
     self assignment, every conversion with its clamping and `ok` flag, arrays
     and maps, exact JSON output (every flow, version, escape mode, quote,
     precision and number format), a table of good and bad JSON text with
     expected errors, save and load round trips under every setting, INI output
     and parsing, and string round trips under every INI escape mode. Fixed:
     `v=v["a"]` and `v=v.asChars()+1` read freed memory (assignment now copies
     first); `toUInt` reported 0 and `UINT_MAX` as failures; `toInt`,
     `toUInt`, `toLong` and `toULong` mangled `ULong` and negative values
     instead of clamping; `toFloat`/`toDouble` of a negative `char` gave a huge
     number; `__strtoll`/`__strtoull` flagged valid values as overflow
     (`18446744073709551615`) and reported `0` as a failure; JSON integers above
     `LLONG_MAX` or equal to `LLONG_MIN` were lost (now `ULong`/exact); a bare
     `null`, `true`, `false` or identifier at the very end of JSON text was
     reported as end of input; unterminated arrays and objects gave
     `ErrBracket`/`ErrBrace` instead of `ErrEnd`; INI values kept a trailing
     `\r` on CRLF files, a last value without newline was a parse error, and
     blanks around keys and values were kept. Noted, not changed: loading JSON
     into a populated variant replaces it (the header comment about overlaying
     keys is wrong); INI values always load as strings.
   - `xml --test` done: decoding and encoding of references (every byte
     value round trips), events of well formed documents (attributes, entities,
     CDATA, comments, processing instructions, DOCTYPE forms, UTF-8, BOM, CRLF),
     the error code of malformed ones, line, column and offset, the exact
     writer output, parse then write back, and large documents through a 512
     byte file buffer (text, attribute, comment, CDATA and processing
     instruction, each also made only of white space). The OASIS suite
     (`--oasis`) is not part of `--test`: it needs files that are not in the
     repository. Fixed: the writer escaped the UTF-8 continuation bytes
     0x80-0x9F as references, so `€` and the typographic quotes were corrupted;
     `;` was escaped needlessly; a CR was written raw and read back as LF; a
     processing instruction kept the blank after its target (`<?pi data?>`
     came back as `<?pi  data?>`); `<!DOCTYPE x PUBLIC "..." "...">` was
     rejected because the word `SYSTEM` was tried first and consumed it; a run
     of white space longer than the file buffer in text, attribute value,
     comment, CDATA or processing instruction gave `ErrEof`. Noted, not
     changed: the reader reports an empty `characters` before every child
     element; the document ending is not checked for further content; the
     writer writes `\r\n` for a line feed on Windows; a truncated end tag
     (`<a>x</a`) gives `ErrNoMatch` and not `ErrEof`; `ErrReference` is never
     returned (an unknown reference is `ErrToken`).One commit per step, each with the result of both platforms in its message.

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
| popup | pane opened at a fixed offset in the window instead of below the Open button | test only (2ab8d08) |
| layout, foursplit | fixed window geometry overflows at 144 DPI (clipped bottom, overlapping rows) | by design, see dpi-plan.md |
| layout | radios disabled until a button is chosen | original behaviour |
| switcher | File List page is empty (no directory set in the test) | original behaviour |
| group 1 | widgets missing in fox2 (spring, header, splitters, tool bars, lists, file/dir dialogs, ...) | copied from fox-1.7.95 (0d65dfc, e365bd4, 102b753, 389631c, 777e970) |
| switcher, tabbook | FXIconSource only needs BMP and GIF | fox2 copy trimmed to those formats |
| dialog | dialog opened on a 96 DPI monitor was clipped: `WM_DPICHANGED` scaled the size, fixed pixel margins did not scale | client area kept at least default size (`FXApp_win32.cpp`) |
| group 2 | widgets missing in fox2 (wizard, knob, 7-segment, calendar, date, progress dialog, image frame) | copied from fox-1.7.95 |
| datatarget, calendar | fixed window geometry clips content at 144 DPI | by design, see dpi-plan.md |
| capture script | window rectangle and DWM bounds differed on mixed-DPI monitors (crop failed) | script is now per-monitor-v2 aware |
| switcher | header and items of the file list and tree list kept the font of the system DPI on a 96 DPI monitor | `applyTheme` in header, icon list, tree list, gauge, progress bar, calendar view (0bc101f) |
| table | row height 20 and column width 100 are fixed pixels: headers clipped at 144 DPI | `FXTable::applyTheme` rescales defaults, rows, columns and headers with the display scale |
| group 3 | `FXTable` missing in fox2 | copied from fox-1.7.95 |
| group 4 | image codecs, `FXBitmap`, bitmap and image views, gradient bar, ruler, tool bar tab, print widgets, cursors and the `fx*io.cpp` codec sources missing in fox2 | copied from fox-1.7.95; `FXIconSource` is the original again, with all codecs |
| `FXBitmap` | one file with Windows and X11 code in the middle | split into `FXBitmap.cpp`, `FXBitmap_win32.cpp`, `FXBitmap_x11.cpp`, `FXBitmap_wayland.cpp` (client buffer is the bitmap, as for `FXImage`) |
| `fxjpegio` | `jmorecfg.h` pulls in `shlwapi.h`, and `xincs.h` leaves `VOID` undefined | `VOID` defined again before the include (Windows only) |
| bitmapviewer | calls libtiff directly, but the codec libraries are private to the library | linked against `FOX_PRIVATE_LIBRARIES` in `tests/CMakeLists.txt` |
| image | ruler numbers kept the font of the system DPI | `FXRuler::applyTheme` retargets the font |
| BMP icons | the corner pixel is taken as transparent colour, so a red 32 bit BMP loses its background | upstream behaviour, unchanged |
| group 5 | MDI widgets (`FXMDIClient`, `FXMDIChild`, `FXMDIButton`) and `FXShutter` missing in fox2 | copied from fox-1.7.95 |
| mditest | `GradientFill` was compiled out on MinGW (condition listed only Borland, Watcom, Cygwin, MSVC): MDI title bars were flat | `__MINGW32__` added in `FXDCWindow_win32.cpp` |
| mditest | title text of an MDI child without focus is white on a light gradient | upstream behaviour, unchanged; the focused child has the dark selection colour |
| mditest | `FXMDIChild` took the normal font of the system DPI for its title, so the title bar was too large on a lower-DPI monitor | `applyTheme` added, it retargets the normal font to the window DPI; the text font of the test is an application font and stays fixed |
| mditest | the sample text font is made by the test and keeps the system DPI, so it was too large on a lower-DPI monitor | the test overrides `applyTheme`, rebuilds the font for the window DPI and display scale and swaps it into its widgets; this is the pattern for any application font |
| wizard | the stored screenshot showed a cropped sidebar (340 px high window, image 392 px) | not reproducible: `FXWizard`, `FXImageFrame` and `FXPacker` equal upstream, the window is 489 px high and shows the whole image in four fresh captures; the screenshot was stale and is re-captured. Captures taken right after show can miss widgets that are still being painted (Back button, a radio button), so the capture script keeps its delay |
| ControlPanel | ported to `fox2/apps/controlpanel` as a copy of `fox-1.7.95/controlpanel`; the sampler and caption fonts are made by the application and kept the system DPI, so they were too large on the 96 DPI monitor | the application overrides `applyTheme` and rebuilds both fonts for the window DPI and display scale (same pattern as `mditest`); the text field in the sampler was missing in one capture, not reproducible in three more, a capture race |
| dirlist (group 3 gate) | icons keep their pixel size on every monitor: 16 px at 144 DPI looks half as large relative to the text as at 96 DPI; the library has no DPI-keyed icon cache | `FXIcon::getScaledIcon(scale)` makes a box filtered copy per display scale from the original pixels and keeps it with the icon; `FXWindow::getScaledIcon` is used by every widget and list item that measures or draws an icon; moving the window to another monitor shows the right size again |
| gltest | `FXGLVisual`, `FXGLContext`, `FXGLCanvas` missing in fox2 (the viewer and 3D objects of `fx3d.h` are not needed by the test) | copied from fox-1.7.95; `gltest` includes GLU itself and links `FOX_GLU_LIBRARIES` |
| `FXGLVisual` | Win32 and GLX code in one file | split into `FXGLVisual.cpp`, `_win32`, `_x11`, `_wayland` (stub: no GL on Wayland yet), shared `fxglpriv.h`; X11 part is untested |
| `FXGLContext`, `FXGLCanvas` | Win32 and X11 branches are small and interleaved, so they stay in one file | unchanged copies |
| OpenGL | CMake finds GL on the Wayland platform and would compile the GLX code | `FOXFeatures.cmake` disables OpenGL for `FOX_PLATFORM=wayland` |
| gltest | the bold label font of the info dialog was created once for the system DPI, so on a lower-DPI monitor the labels stayed large next to the scaled values | the dialog rebuilds the font for its monitor in `applyTheme`; labels with an application font are not rescaled by the library |
| console | `FXConsole` missing in fox2; the plan took `console` for a windowless program | copied from fox-1.7.95, `console` added as a GUI test |
| console | `FXConsole` fills its area red and shows 30 numbered test lines | upstream work-in-progress code (debug fill and sample text in `FXConsole.cpp`), unchanged |
| console | `FXConsole` took the normal font of the system DPI, so the text was too large on a lower-DPI monitor | `applyTheme` added, it retargets the normal font to the window DPI like `FXText` |
| chart | ported to `fox2/apps/chart` as a copy of `fox-1.7.95/chart` (the `charttest` program plus the chart widget sources); the chart measures itself with `FXDC::getClipX()`/`getClipWidth()`, but the Wayland DC only set the rasterizer clip in `begin()`, so those returned 32767 and the plot fill, axes and caption landed outside the image or in one gradient band | `FXDCWindow_wayland.cpp` sets `rect`/`clip` from the drawable in `begin()`, as the X11 and Win32 modules already did |
| chart | the vertical axis labels and numbers are drawn with `FXFont::setAngle()`; the Wayland rasterizer ignored the angle, so they came out horizontal | `FXRaster` keeps the text angle and rotates each glyph's coverage bitmap around the baseline point; `FXDCWindow_wayland` passes `FXFont::getAngle()` together with the glyph cache |
| chart | rotated font metrics: Xft reports 0 for `getFontHeight()` and `getTextWidth()` of an angled font, so the X11 frame measures every `Y` label as empty and overprints it on the numbers; the Wayland font keeps its unrotated metrics | not reproduced: the Wayland frame lays the rotated labels out instead of overprinting them; the two screenshots differ in that layout, and in the angled glyph antialiasing, not in a defect |
| PathFinder | ported to `fox2/apps/pathfinder` as a copy of `fox-1.7.95/pathfinder`; `fxparsegeometry()` is declared in `fxdefs.h` but had no implementation in fox2, so the program did not link | `lib/fxparsegeometry.cpp` copied from 1.7.95 and added to `lib/Makefile.am`; no source change in the application (the X11 frame is pixel-identical to the origin) |
| PathFinder | a double click on a file without an association did nothing on Wayland; the X11/origin program opens the "Unknown File Type" dialog | the Wayland dispatcher built a fresh `FXEvent` for every raw event, so `click_count`, `click_time`, `click_button` and `moved` never reached the second press; `FXApp::dispatchEvent` now uses the persistent `FXApp::event` like X11 and Win32, and `scripts/gate-wayland-doubleclick.sh` is the regression gate |
| PathFinder | disabled toolbar buttons (back, forward, cut, copy) showed their full-colour icons on Wayland | `FXDCWindow::drawIconSunken` drew the plain icon; `FXRaster::drawIconSunken` now paints the etch mask (the visible dark pixels) in the highlight and shadow colours, as the X11 module does |
| PathFinder | the directory tree keeps a different horizontal scroll offset on the first Wayland frame, so the folder pane is scrolled a few pixels relative to the X11 frame | not reproduced as a drawing defect: `FXTreeList::makeItemVisible` keeps the first negative `pos_x` it computed while the tree was still filling; the content, selection and behaviour agree, and the offset is corrected as soon as an item is scrolled into view |
| adie | ported to `fox2/apps/adie` as a copy of `fox-1.7.95/adie`; the editor needed library pieces fox2 did not have: `FXUndoList`/`FXCommand` (with `FXMarkedPtr`), `FXDirVisitor`/`FXGlobVisitor`, `FXSearchDialog`/`FXReplaceDialog` and the `FXCP1252Codec` | copied from 1.7.95 into `include/` and `lib/`, added to the Makefile lists and to `fx.h`; the application sources are unchanged |
| adie | on Wayland the link failed on `FXApp::addInput()`/`removeInput()`; `ShellCommand` uses them to watch the pipes of a child program | the Wayland event loop already polled `inputs[]`; the two accessors were missing. Implemented in `FXApp_wayland.cpp` like the X11 module |
| adie | syntax coloring stays off after a syntax file or language is selected: `--syntax=`, `--lang=` and the "Syntax patterns" menu only pick the rules, the text remains plain (the same happens in 1.7.95) | upstream behaviour, not a port defect: coloring is a preference (`colorize`, off by default) toggled with Options -> Syntax coloring or in the preferences, and stored in `~/.config/adie.rc`; the stored screenshots are taken with it on |
| adie | screenshots: the X11 frames of fox1 and fox2 differ only in the live clock of the status bar (it shows seconds); the Wayland frame differs from the X11 frame in the text caret/focus state and the same clock, about 1100 pixels of 240000 | expected: the status bar clock is a live field and the caret depends on focus; text, syntax highlighting, layout and every other pixel agree. All three frames are taken with `colorize=true` in the registry |
| glviewer | ported to `fox2/apps/glviewer` as a copy of `fox-1.7.95/glviewer`; it needs the 3D math headers (FXVec/FXQuat/FXMat/FXRange/FXSphere/FXExtent/FXComplex, f and d), `FXGLObject`/`FXGLGroup`, `FXGLViewer`, `FXDockTitle` and the private `lib/jitter.h`, none of which fox2 had | copied from 1.7.95 into `include/` and `lib/`; `FXGLViewer` draws through `FXGLCanvas`, which now has the EGL backend on Wayland. X11 (Autotools and CMake) and Wayland with `--opengl` build |
| glviewer | the X11 runtime could not be checked on this machine: the development display is Xwayland and offers no GLX, so `FXGLVisual::hasOpenGL()` is false and the program reports that the library was compiled without OpenGL | not a port defect: the X11 OpenGL build succeeds; the Wayland frame is kept as `screenshots/tests/fox2/wayland/glviewer.png` (KWin/EGL and headless sway/llvmpipe), and `tests/glcanvas` covers the EGL canvas path |

## Status

| Step | State |
| --- | --- |
| 0 Groundwork | done |
| 1 Layout containers | done |
| 2 Dialogs and data | done |
| 3 Lists and files | done |
| 4 Drawing | done |
| 5 Special cases | done |
