---
name: app-port
description: Port a FOX 1.x example application from fox-1.7.95/ into fox2/apps/, wire it into the CMake and Autotools builds, fix the library seams it needs, verify it against the origin binary, and store screenshots. Use when adding an application under fox2/apps (calculator, controlpanel, chart, pathfinder pattern), when a ported application fails to link or renders differently on X11 and Wayland, or when a port needs its documentation and commit split.
---

# Porting a FOX 1.x application to fox2

The worked examples are `fox2/apps/calculator`, `fox2/apps/controlpanel`,
`fox2/apps/chart` and `fox2/apps/pathfinder`; the plans and defect records are
in `docs/calculator-port-plan.md` and `docs/test-port-plan.md`.

## Ground rules

- `fox-1.7.95/` is read-only. Copy what the application needs into
  `fox2/apps/<name>/` and change it only there. Debugging in the origin tree is
  allowed, but do not leave changes behind.
- Keep copied sources byte-identical to the origin unless a fix is required.
  Check with `cmp` after copying; any change must be explained in
  `docs/test-port-plan.md`.
- Never put platform code in the application. If it needs an API that fox2
  lacks, add the missing library file or the missing seam in `fox2/lib/`, with
  a `_x11.cpp`, `_win32.cpp` or `_wayland.cpp` split when it is platform
  specific. Follow the `fox-toolkit` skill for FOX formatting.
- One commit for the application (sources, build files, registration,
  screenshots, docs). Library fixes are a separate commit, so they can be
  found and reverted without the application.

## 1. Inventory the origin application

```bash
ls -la fox-1.7.95/<name>
wc -l fox-1.7.95/<name>/*.cpp fox-1.7.95/<name>/*.h | tail
cat fox-1.7.95/<name>/Makefile.am
grep -n '#include\|#if\|#ifdef\|#if defined' fox-1.7.95/<name>/*.cpp fox-1.7.95/<name>/*.h
```

Record:

| Question | Where the answer goes |
| --- | --- |
| Program name and target | `bin_PROGRAMS`, CMake `add_executable` |
| Sources and headers | `<program>_SOURCES`, CMake source list |
| Fonts, math or other libraries | `LDADD`/`target_link_libraries` (`-lm` is common) |
| Man page, `LICENSE` | `man_MANS`, `EXTRA_DIST` |
| Images (`.gif`, `.bmp`) | embedded with `reswrap` into `icons.{h,cpp}` |
| Platform conditionals | note them; they belong in the library, not the app |
| `xincs.h`, `fxkeys.h` includes | `fox2/include` already provides both |
| Registry file the app writes | use a private `HOME` in every test run |

The application may already compile: chart needed no source change, pathfinder
only needed one missing library function.

## 2. Copy and keep the sources

```bash
mkdir -p fox2/apps/<name>
cp fox-1.7.95/<name>/*.cpp fox-1.7.95/<name>/*.h \
   fox-1.7.95/<name>/*.gif fox-1.7.95/<name>/*.bmp \
   fox-1.7.95/<name>/<Program>.1 fox-1.7.95/<name>/LICENSE \
   fox2/apps/<name>/
```

- Never copy `Makefile`, `Makefile.in` or object files.
- Do not rename the upstream sources (`PathFinder.cpp`, `charttest.cpp`,
  `ControlPanel.cpp`); only the directory is lower case.
- `icons.h`/`icons.cpp` are checked in, as in the existing applications. If
  they are missing or stale, regenerate them once with the fox2 `reswrap`
  (`fox2/utils/reswrap.cpp`):

```bash
reswrap -h -o icons.h *.gif *.bmp
reswrap -i icons.h -o icons.cpp *.gif *.bmp
```

Keep the generated header comment (it names the origin file); do not add a
build rule that needs `reswrap` at compile time.

## 3. Build definitions

`fox2/apps/<name>/CMakeLists.txt`:

```cmake
add_executable(PathFinder
  icons.cpp
  CopyDialog.cpp
  PathFinder.cpp
  Preferences.cpp
  PropertyDialog.cpp)
target_link_libraries(PathFinder PRIVATE FOX::FOX m)
```

- The target name is the binary name; keep upstream capitalisation
  (`ControlPanel`, `PathFinder`) or lower case (`calculator`, `charttest`).
- Link the `FOX::FOX` target and add `m` only when the sources use math.
- No include directories are needed: quoted headers resolve in the source
  directory, and `fx.h`/`xincs.h` come through `FOX::FOX`.

`fox2/apps/<name>/Makefile.am`:

```make
AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-1.7.la -lm

bin_PROGRAMS = PathFinder
man_MANS = PathFinder.1

# icons.h and icons.cpp are checked in; they are the output of reswrap over
# the images below, as in the calculator and control panel ports.
PathFinder_SOURCES = \
icons.h \
icons.cpp \
...

EXTRA_DIST = \
$(man_MANS) \
LICENSE \
$(wildcard *.gif) \
$(wildcard *.bmp)
```

Register the application in four places:

| File | Change |
| --- | --- |
| `fox2/apps/CMakeLists.txt` | `add_subdirectory(<name>)` |
| `fox2/apps/Makefile.am` | add `<name>` to `SUBDIRS` |
| `fox2/configure.ac` | add `apps/<name>/Makefile` to the `AC_CONFIG_FILES` list next to the other apps |
| `fox2/.gitignore` | `/apps/<name>/Makefile.in` and `/apps/<name>/<binary>` |

`autoreconf` then regenerates `apps/Makefile.in`; the build script does that
when `Makefile.am` is newer than `configure`.

## 4. Build and close the library gaps

```bash
./scripts/build-fox2-linux.sh --fast --jobs=24     # Autotools, X11
./scripts/build-fox2-linux.sh --cmake --fast       # CMake, X11, faster loop
./scripts/build-fox2-wayland.sh --jobs=24          # Wayland
```

For a quick iteration on one target:

```bash
cmake -S fox2 -B tmp/build-fox2-cmake-<name> -G Ninja \
      -DFOX_WITH_CUPS=OFF -DFOX_WITH_OPENGL=OFF
ninja -C tmp/build-fox2-cmake-<name> <Target>
ninja -C tmp/build-fox2-wayland <Target>
```

### A link error means a missing library seam

```bash
/usr/bin/ld: PathFinder.cpp.o: undefined reference to `FX::fxparsegeometry(...)'
```

1. Find the declaration in `fox2/include` (here `fxdefs.h`); it may already be
   there, which means only the implementation is missing.
2. Find the implementation in `fox-1.7.95/lib` and copy it to `fox2/lib/`
   (byte-identical).
3. Add the file to `fox2/lib/Makefile.am`; the CMake build reads that list.
4. If the file contains platform code, split it into `_x11.cpp`, `_win32.cpp`,
   `_wayland.cpp` instead of copying one mixed file.

To list every missing FX symbol at once, without waiting for one application
per link:

```bash
nm -D --undefined-only tmp/build-fox2-wayland/lib/libFOX-1.7.so* |
  c++filt | grep 'FX::' | sort -u
```

Library gaps found this way in real ports: `fxparsegeometry` (declared but
never ported), `FXApp::flush()`, `FXBitmap::GetDC()`, the Wayland DC clip
rectangle, angled text in `FXRaster`, and the sunken icon of disabled buttons.
Fix them in the platform module and record the behavior in
`docs/wayland-plan.md`, not in the application.

### Build logs lie less than exit codes

`ninja: build stopped` with only warnings above it usually means a real error
further up. Grep the log:

```bash
grep -n 'FAILED\|error:\|undefined reference' tmp/build-fox2-wayland/build.log | head
```

## 5. Verify against the origin and across platforms

Build the origin application once (the baseline script already builds the
applications into `tmp/build-fox1/<name>/`):

```bash
./scripts/build-fox1-linux.sh          # only when tmp/build-fox1 is gone
ls tmp/build-fox1/<name>/<binary>
```

Use a private `HOME` for every run: applications write a registry file, and a
private `HOME` plus no session bus also keeps the desktop theme out of the
screenshot.

### X11: fox1 and fox2 must be pixel-identical

```bash
for name in fox1 fox2; do
  case $name in
    fox1) bin=tmp/build-fox1/<name>/<binary> ;;
    fox2) bin=tmp/build-fox2-cmake-<name>/bin/<binary> ;;
  esac
  mkdir -p tmp/<app>-home-$name
  HOME=$PWD/tmp/<app>-home-$name "$bin" "$PWD/fox2/apps" &   # fixed directory
  pid=$!
  sleep 3
  id=$(tools/build/xlist -all | awk '/<Title>/{print $1; exit}')
  tools/build/xgrab "$id" tmp/<app>-$name.ppm
  kill "$pid" 2>/dev/null
  wait "$pid" 2>/dev/null
done
python3 - <<'PY'
from PIL import Image, ImageChops
a=Image.open('tmp/<app>-fox1.ppm').convert('RGB')
b=Image.open('tmp/<app>-fox2.ppm').convert('RGB')
print('identical' if ImageChops.difference(a,b).getbbox() is None else 'differs')
PY
```

- `xlist -title` compares the exact `WM_NAME`. Titles can carry a suffix:
  PathFinder uses `PathFinder:- /path`, so `xlist -all | awk` is safer.
- Build the tools once with `make -C tools` when `tools/build/` is missing.
- Capture through `xgrab`, which uses `XCompositeNameWindowPixmap` first and
  fails with `BadMatch` on an unmapped window.
- Choose a reproducible initial location (a repository directory), not `$HOME`.

If the X11 frames differ, the port is not faithful yet; fix it before looking
at Wayland.

### Wayland: one private headless compositor

```bash
RUN=tmp/wayland-<app>
rm -rf "$RUN"; mkdir -p "$RUN/runtime" "$RUN/home" "$RUN/frames"
chmod 700 "$RUN/runtime"
printf 'output HEADLESS-1 resolution 1024x768 bg #101010 solid_color\n' >"$RUN/config"
printf 'default_border pixel 0\n' >>"$RUN/config"
printf 'for_window [title="<Title>.*"] floating enable\n' >>"$RUN/config"
export XDG_RUNTIME_DIR="$RUN/runtime" HOME="$RUN/home"
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 \
       WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS XDG_CONFIG_HOME
sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
spid=$!
for i in $(seq 1 200); do
  sock=$(ls "$RUN/runtime"/wayland-* 2>/dev/null | head -1) || true
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
done
export WAYLAND_DISPLAY=$(basename "$sock")
FOX_WAYLAND_DUMP="$RUN/frames" timeout 10 \
  tmp/build-fox2-wayland/bin/<binary> "$PWD/fox2/apps" >"$RUN/app.log" 2>&1 &
wpid=$!
sleep 4
kill "$wpid" 2>/dev/null; wait "$wpid" 2>/dev/null
kill "$spid" 2>/dev/null; wait "$spid" 2>/dev/null
ls "$RUN/frames"        # window-<id>-<w>x<h>[-bar<n>].ppm
```

- `chmod 700` on `XDG_RUNTIME_DIR` is required; a tiled window must be floated
  by a sway rule or it is resized to the output and the capture changes size.
- A frame named `...-bar26` has a 26 px client side title bar (weston); with
  sway's server side decorations the frame is the client area.
- Compare the Wayland frame with the X11 frame and classify the difference by
  region (toolbar, content, status bar) before touching the code.
- To decide whether a difference belongs to the platform or the application,
  run the **X11** client inside the same headless sway's Xwayland
  (`DISPLAY=:N`, capture with `xgrab`) and compare that with the Wayland dump.
- Common, already documented sources of a difference: theme or DPI (private
  `HOME` missing), focus and selection state (headless compositors have no
  seat; the Wayland client may focus itself), angled text (Xft metrics are 0
  for a rotated font), disabled icons, and antialiasing.
- `FOX_WAYLAND_TRACE=1` and `WAYLAND_DEBUG=1` are the diagnostics; read them
  before guessing.

Store three screenshots and convert the PPMs with Pillow:

| File | Content |
| --- | --- |
| `screenshots/tests/fox1/x11/<binary>.png` | origin reference |
| `screenshots/tests/fox2/x11/<binary>.png` | ported X11 frame |
| `screenshots/tests/fox2/wayland/<binary>.png` | ported Wayland frame |

## 6. Document the port

- `docs/test-port-plan.md`: one table row per program or defect, with
  `Program | Defect | Fix`. Name the application directory and the library
  change; write "not reproduced" when the difference is an upstream quirk
  (for example the zero metrics of an angled Xft font) and explain why.
- `docs/wayland-plan.md`: add rasterizer or platform deviations to the
  "Deviations" list, and mention the port that exposed the bug.
- If the application overrides `applyTheme` for per-window DPI (see
  `ControlPanel.cpp` and the `mditest` note), record that pattern.
- Update the plan/status text only when it is now wrong; do not rewrite the
  plans for a single application.

## 7. Commit

```bash
git add fox2/lib/<library files> docs/wayland-plan.md
git commit -m "fox2: <library change> on Wayland"

git add fox2/apps/<name> fox2/apps/CMakeLists.txt fox2/apps/Makefile.am \
        fox2/configure.ac fox2/.gitignore docs/test-port-plan.md \
        screenshots/tests/fox1/x11/<binary>.png \
        screenshots/tests/fox2/x11/<binary>.png \
        screenshots/tests/fox2/wayland/<binary>.png
git commit -m "fox2: port the <name> application"
```

`git status --short` must be empty afterwards; `Makefile.in` and the built
binary must be ignored, not committed.

## Checklist

- [ ] sources `cmp`-identical to `fox-1.7.95/<name>` (or the change is documented)
- [ ] `icons.{h,cpp}` checked in, `EXTRA_DIST` complete
- [ ] CMake and Autotools build on X11
- [ ] Wayland build links and runs
- [ ] fox2 X11 frame identical to the fox1 X11 frame
- [ ] Wayland frame inspected, remaining differences explained
- [ ] three screenshots stored under `screenshots/tests/...`
- [ ] `docs/test-port-plan.md` (and `wayland-plan.md` if needed) updated
- [ ] library fixes in their own commit
- [ ] `git status` clean

## Pitfalls that cost time

- **`xlist -title` is an exact `WM_NAME` match.** Use `-all` and `awk` when the
  title has a directory suffix; an empty id makes `xgrab` print `BadWindow`.
- **Always use a private `HOME`.** Applications write `~/.<App>rc`; reused
  homes carry expanded trees, bookmarks and window geometry into the next run.
- **A missing directory argument changes the frame.** PathFinder opens the
  argument, so pass a fixed repository path in both runs.
- **`undefined reference` is a library gap, not an app bug.** Check
  `fox2/include` for the declaration and port the `.cpp`; do not reimplement
  FOX API in the application.
- **Wayland buffers are recycled.** A difference that only covers widgets that
  moved or a pane that was resized points at a repaint/backing-store problem;
  `scripts/gate-wayland-repaint.sh` is the regression gate.
- **A disabled button draws `drawIconSunken`.** On Wayland the etch mask is
  derived from the dark, visible pixels of the icon; drawing the plain icon
  makes disabled toolbar buttons look enabled.
- **Angled fonts differ by design.** Xft returns 0 from `getFontHeight()` for a
  non-zero angle, the Wayland glyph cache keeps the unrotated metrics; layouts
  that measure rotated text therefore differ (chart).
- **`pkill -f "<path>"` can match the shell running it.** The pattern is in the
  script's own command line; keep the PID from `$!` and `kill` that instead.
- **Do not compare a live-desktop X11 capture with a headless Wayland dump.**
  Theme, DPI and focus differ. Capture both in the same compositor or with the
  same private environment, and compare fox1 with fox2 on X11 first.
