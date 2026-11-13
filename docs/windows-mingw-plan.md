# Windows build plan — FOX 1.7.95 baseline with MSYS2/MinGW-w64

This document plans and records the Windows equivalent of Phase 1 in
`docs/general-plan.md`: build the unmodified `fox-1.7.95/` tree from source on
this Windows machine, so the ported `fox2/` work has a verified native Windows
baseline in addition to the Linux/X11 one.

The original tree stays read-only; all generated files go to
`tmp/build-fox1-mingw/` (out-of-tree build, `tmp/` is ignored by git).

## Environment

- MSYS2 installed in `C:\msys64`; target subsystem **MINGW64**
  (`C:\msys64\mingw64`, shell `C:\msys64\usr\bin\bash.exe` or
  `C:\msys64\mingw64.exe`).
- Already present: GCC 16.2.0 (MinGW-w64), binutils, gdb, pkgconf,
  winpthreads, zlib, bzip2, xz, zstd.
- Missing on a fresh MINGW64 install: `make`, the autotools, `cmake`,
  `ninja` and the image libraries. `scripts/install-msys2-deps.sh` installs
  them.

## Platform differences from the Linux baseline

| Topic | Linux/X11 build | Windows/MinGW-w64 build |
| --- | --- | --- |
| Window backend | Xlib + Xext + Xft/fontconfig | Win32/GDI (`#if defined(WIN32)`) |
| Backend macro | none | `WIN32` (must be defined for the library **and** its users) |
| Selector | `AC_PATH_X` finds X11 | no X11, `no_x=yes`, X libs replaced by `comctl32 wsock32 winspool mpr gdi32 imm32` |
| Fonts | Xft/freetype/fontconfig | native GDI fonts, no fontconfig |
| Printing | CUPS | Win32 print API (`winspool`) |
| Threads | pthread | winpthreads (`-lpthread`) |
| OpenGL | GLX | WGL, links `opengl32`/`glu32`; needs headers (Mesa package) |
| Shared library | `.so` via libtool | `.dll` + `.dll.a` import library via libtool |

`configure.ac` maps `*mingw32*` to `COND_OSWINDOWS`; the library Makefile then
adds `-DFOXDLL -DFOXDLL_EXPORTS -DWIN32 -DNDEBUG -D_WINDOWS -DUNICODE`.
It does **not** add `WIN32` for the tests and example programs, so the build
must pass `CPPFLAGS=-DWIN32` to `configure`; otherwise the tests get the Unix
declarations (`FXID` as `unsigned long`, `XEvent`) and no longer match the
library ABI.

## Dependencies

MSYS2 (host tools, installed as `msys/` packages):

- `make` — GNU make 4.4 (do not use `mingw32-make`; libtool-generated
  makefiles expect `make`)
- `autoconf-wrapper` + `autoconf2.73` — `autoconf`/`autoreconf` (MSYS2 ships
  versioned packages behind a wrapper)
- `automake-wrapper` + `automake1.18` — `automake`/`aclocal`
- `libtool` — required by `LT_INIT`
- `diffutils`, `pkgconf` — configure helpers

MinGW-w64 (target libraries):

- `mingw-w64-x86_64-gcc`, `-binutils`, `-crt`, `-libwinpthread`
- `mingw-w64-x86_64-zlib`, `-bzip2`, `-xz` (liblzma)
- `mingw-w64-x86_64-libjpeg-turbo`, `-libpng`, `-libtiff`, `-libwebp`,
  `-openjpeg2`, `-libjxl` — image codecs (TIFF pulls JPEG, WebP, LZMA, zstd)
- optional: `mingw-w64-x86_64-mesa` for `GL/gl.h`, `GL/glu.h` and the
  `gltest` example

Not used on Windows: CUPS, X11/Xft/fontconfig, dl, rt, inotify. The baseline
keeps WebP and JPEG 2000 disabled, exactly like the Linux baseline
(`--enable-webp` / `--enable-jp2` stay opt-in).

## Scripts

- `scripts/install-msys2-deps.sh` — installs the packages above
  (`--opengl` adds Mesa, `--upgrade` runs a full system upgrade first,
  `--print` only lists the packages).
- `scripts/build-fox1-mingw.sh` — copies `fox-1.7.95/` to `tmp/fox1-src`,
  regenerates its build system with the installed Autotools, then configures
  and builds out-of-tree into `tmp/build-fox1-mingw/` (default 12 jobs, with a
  build log).
- `scripts/build-fox1-mingw.ps1` — Windows/PowerShell front end for the two
  bash scripts; sets `MSYSTEM=MINGW64` and runs them through
  `C:\msys64\usr\bin\bash.exe`.

### Why the build system is regenerated

The `configure` and `Makefile.in` files shipped in `fox-1.7.95/` were generated
with libtool 2.4.6, while MSYS2 ships libtool 2.6.2. Linking then fails with
`libtool: Version mismatch error`. The build script therefore copies the tree
to `tmp/fox1-src` and runs `autoreconf -fi` there; the original tree is not
modified by the build.


## Build steps

```powershell
# from the workspace root, PowerShell
.\scripts\build-fox1-mingw.ps1 -InstallDeps
```

or manually inside the MSYS2 MINGW64 shell:

```bash
./scripts/install-msys2-deps.sh
./scripts/build-fox1-mingw.sh
```

What the build script does:

1. Copies `fox-1.7.95/` to `tmp/fox1-src` and runs `autoreconf -fi` there
   (libtool version fix, see above).
2. `mkdir -p tmp/build-fox1-mingw`, then
   `CPPFLAGS=-DWIN32 ../fox1-src/configure --disable-cups
   --with-opengl=no --enable-shared --disable-static --disable-webp --disable-jp2`
3. `make -C utils` — builds `reswrap`, needed to generate `icons.cpp`.
4. `make -C lib` — builds `libFOX-1.7.dll` + import library.
5. `make -C tests hello.exe button.exe` — links the two smallest test programs
   against the DLL; the libtool wrapper adds `lib/.libs` to `PATH`.

On MinGW the test targets carry the `.exe` suffix; plain `hello` would fall
back to make's built-in rule and fail.

Options: `--opengl` (enable GL, needs Mesa), `--static` (static library
instead of the DLL), `--debug` / `--release`, `--jobs=N`, `--clean`,
`--fast` (skip `autoreconf`/`configure` when `configure.ac`, a `Makefile.am`
or the configure options did not change), `--run` (start `hello` for 3 s
and treat the timeout as success) and `--cmake` (see below).

### CMake + Ninja alternative

On Windows `autoreconf` and `configure` dominate the build time, so both trees
also carry a CMake build (`CMakeLists.txt`, `cmake/FOXFeatures.cmake`;
`fox2/` and `fox-1.7.95/`). Autotools stays the default; pass `--cmake`
(`-CMake` in the PowerShell wrappers) to use it:

```powershell
.\scripts\build-fox2-mingw.ps1 -CMake -Run
.\scripts\build-fox1-mingw.ps1 -CMake -Static
```

- No `autoreconf`, no `configure`, no copy of the source tree; the build
  directories are `tmp/build-fox1-mingw-cmake` and `tmp/build-fox2-win-cmake`
  (`tmp/build-fox2-linux-cmake` for `build-fox2-linux.sh --cmake`, untested).
- Source lists are read from the `Makefile.am` files, so both build systems
  stay in sync. Feature detection mirrors the `configure` switches via
  `-DFOX_WITH_OPENGL`, `-DFOX_WITH_CUPS`, `-DFOX_WITH_WEBP`, `-DFOX_WITH_JP2`,
  `-DFOX_BUILD_SHARED` (fox1) and `-DFOX_FOXTRACE` (fox2).
- Output goes to `<build>/bin` (DLL, `reswrap`, test programs) and
  `<build>/lib` (import library / static library).
- Measured (12 jobs): fox2 clean configure + build + run about 20 s, fox1 clean
  about 48 s, `--fast` no-op rebuild about 1 s. Extra fox1 tests can be built
  with `-DFOX_BUILD_ALL_TESTS=ON` (all build on MinGW except `scan`, whose
  `strtod(..., nullptr)` call is ambiguous with GCC 16; use `ninja -k 0`).
- On MinGW all symbols of the DLL are exported (`--export-all-symbols`),
  matching libtool behaviour; some classes lack `FXAPI`.

## MinGW patch

One source change was needed and is committed separately on `fox-1.7.95/`
(commit `4acedaf`, "fox-1.7.95: keep the VOID macro on MinGW"):

- `include/xincs.h` removes the `VOID` macro (`// Remove crap`). On MinGW,
  libjpeg's `jmorecfg.h` includes `<shlwapi.h>`, which pulls in `objbase.h`
  and `ole2.h`; those COM headers still need `VOID`. The `#undef` is now
  skipped for `__MINGW32__` only.


## Scope

Baseline scope is the library plus `tests/hello` (and `tests/button`). The
remaining programs (`adie`, `shutterbug`, `pathfinder`, `calculator`,
`controlpanel`, `glviewer`, `chart`) are not part of the first Windows
baseline; they are ported later on top of `fox2/`, not in `fox-1.7.95/`.

## Risks and expected problems

- **MinGW is not upstream's tested Windows path.** Upstream builds Windows
  with MSVC project files in `windows/`; the autotools path has only a
  Cygwin/MinGW note in `doc/win32.html`. Expect compile errors in Win32-only
  code and missing import libraries. Any fix is applied in `fox2/`, never in
  `fox-1.7.95/`.
- **`-no-undefined` DLL link.** The library links with `-no-undefined`, so
  every Win32 function must be covered by an import library. Known set:
  `comctl32`, `wsock32`, `winspool`, `mpr`, `gdi32`, `imm32`, `msimg32`,
  `advapi32`/`shell32`/`user32`/`kernel32` (GCC defaults), `opengl32`/`glu32`
  when GL is on. If something is still unresolved, fall back to `--static`.
- **OpenGL.** Without Mesa there are no `GL/*.h` headers; all GL bodies are
  behind `HAVE_GL_H`, so the library still compiles, but `gltest` is excluded
  and `--with-opengl=no` is used.
- **Resource generation.** `lib/icons.cpp`/`icons.h` are produced by
  `utils/reswrap`; cursor `.xbm` files are included directly by `FXApp.cpp`.
  Both work because automake adds the source directory to the include path.
- **Tests ABI.** Tests must see `WIN32`; covered by `CPPFLAGS=-DWIN32`.
- **Path handling.** Drive-letter paths and the MSYS2 `/c/...` mapping are
  avoided by building under the workspace-relative `tmp/` directory.

## Acceptance criteria

- `tmp/build-fox1-mingw/lib/.libs/libFOX-1.7-0.dll` and the `.dll.a` import
  library are produced.
- `tests/hello` and `tests/button` link and start; `hello` shows its window
  and stays in the event loop.
- The configure options, detected libraries, warnings and result are recorded
  below and in `tmp/build-fox1-mingw/build.log`.

## Result

Windows baseline built on 2026-11-12 with MSYS2 MINGW64, GCC 16.2.0,
GNU Make 4.4.1, Autoconf 2.73, Automake 1.18.1, libtool 2.6.2, 12 jobs.

- Configure options: `CPPFLAGS=-DWIN32`, `--disable-cups`,
  `--with-opengl=no`, `--enable-shared --disable-static`,
  `--disable-webp --disable-jp2`.
- Detected: `no_x` (no X11), JPEG, JPEG-XL, zlib (PNG and compression),
  bzip2, lzma, TIFF, pthread (winpthreads). Not enabled: CUPS, OpenGL,
  Xft/fontconfig, WebP, JPEG 2000.
- DLL import libraries: `comctl32 wsock32 winspool mpr gdi32 imm32 msimg32`,
  plus the MinGW defaults (`advapi32 shell32 user32 kernel32`), `pthread`,
  `jpeg`, `jxl`, `z`, `bz2`, `lzma`, `tiff`.
- Artifacts: `libFOX-1.7-0.dll` (6.7 MB), `libFOX-1.7.dll.a` import library
  (8.8 MB), `tests/hello.exe` (66 KB), `tests/button.exe` (111 KB).
- Warnings: 598, no errors. Dominant: 368
  `-Wdeprecated-enum-enum-conversion`, 85 `-Wunused-parameter`, 47
  `-Wsign-compare`, 19 `-Wmissing-field-initializers`, 17 `-Wvolatile`.
- `tests/hello.exe` starts, shows its window and stays in the event loop
  (killed by a 3 s timeout, status 124), so the Win32/GDI backend, widget
  tree, and event dispatch work.
- Build log: `tmp/build-fox1-mingw/build.log`.

