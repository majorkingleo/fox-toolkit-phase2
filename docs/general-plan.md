# FOX port — master plan

Goal: produce a maintained, modernized FOX library in `fox2/` while the original
`fox-1.7.95/` tree stays untouched (read-only except for debugging or testing).

The plan starts from two base phases:

1. Rebuild the old library to get a verified baseline.
2. Copy the basic parts into `fox2/` and start a basic implementation there.

The later modernization workstreams live in `docs/modernization-plan.md`.

## Phase 1 — Rebuild the old library (baseline)

- Build the unmodified `fox-1.7.95/` from source on this machine
  (`./configure` then `make`) and record the exact steps, configure options,
  warnings, and the final result.
- Purpose: prove the toolchain works and capture a build/run baseline for the
  original before anything is copied or changed.
- Deliverable: a build log plus a short note of the configure options and any
  required system dependencies.

### Phase 1 result (2026-11-11)

- Out-of-tree build in `tmp/build-fox1/` so `fox-1.7.95/` stays unmodified.
- Toolchain: GCC 16.2.1, GNU Make 4.4.1, Autoconf 2.73, Automake 1.19.
- `../fox-1.7.95/configure` with default options: exit 0.
- `make -j24`: exit 0 in ~40 s; produced `lib/.libs/libFOX-1.7.so.0.0.95`
  (7.3 MB) plus the tests and example programs.
- Warnings only, no errors. Dominant one: ~2300
  `-Wdeprecated-enum-enum-conversion` from flag combinations of the unnamed
  enums, then enum arithmetic, `%lld` format mismatches, and a few unused
  parameters.
- Detected and enabled: X11, Xext, Xrender, Xcursor, Xrandr, Xfixes, Xi, XShm,
  Xshape, Xft, fontconfig, freetype, JPEG, JXL, PNG (via zlib), TIFF, zlib,
  bzip2, lzma, CUPS, OpenGL/GLU/GLX.
- Not enabled by default: WebP and JPEG 2000 (opt-in via `--enable-webp` /
  `--enable-jp2`, needing `libwebp` / `openjpeg2`). No other library is
  required.


## Phase 2 — Copy basic parts into fox2

- Create the `fox2/` tree and copy only what is required to compile and run a
  minimal window with a button. Do not copy the whole tree blindly.
- Suggested basic subset (adjust as the build demands):
  - build skeleton: Autotools files (`configure.ac`, `Makefile.am`) copied from
    the original and trimmed to the copied subset
  - `xincs.h`, `fxver.h`, `fxdefs.h`, `fx.h`
  - core object model: `FXObject`, `FXId`, `FXMetaClass`, message-map macros
  - `FXString`, `FXStream` and the minimal I/O they need
  - application/window layer: `FXApp`, `FXWindow`, `FXComposite`,
    `FXTopWindow`
  - first widgets: `FXFrame`, `FXLabel`, `FXButton`
  - first layouts: `FXHorizontalFrame`, `FXVerticalFrame`, `FXMatrix`
  - minimal drawing/font support: `FXDC`, `FXDCWindow`, `FXFont`, `FXVisual`
  - a `tests/hello.cpp`-style example that opens a window with a button
- All copied C++ must follow the FOX formatting rules (2-space indent, tight
  operators, `if(`-style braces, `Type* p`, etc.).

## Phase 3 — Start the basic implementation in fox2

- Make the copied subset compile, then make `hello` run: a window shows and the
  button sends `SEL_COMMAND` to its target.
- Keep every change in `fox2/`; never edit `fox-1.7.95/` except for debugging
  or testing.
- Grow the subset incrementally, one widget or feature at a time, rebuilding
  and re-running the example after each step.

## Later phases (modernization)

Once the base implementation builds and runs, apply the workstreams described
in `docs/modernization-plan.md`:

1. Theme system.
2. Windows DPI awareness and scaling.
3. Compile-time platform and presentation seams.
4. Browser thin-client prototype.
5. Wayland-native adapter (pre-scope).

## Ground rules

- Do not modify code in `fox-1.7.95/` except for debugging or testing.
- The target library lives in `fox2/`; copy what is required from
  `fox-1.7.95/` and modify the `fox2/` copy.
- Long or complex shell work goes into bash scripts under `tmp/` in this
  workspace.
- Follow the FOX formatting rules in all C++ code (`fox2/`, tests, tools) and
  never reformat an existing file to a different style.
