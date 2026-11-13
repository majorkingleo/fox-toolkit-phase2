#!/usr/bin/env bash
#
# Build the fox2 library natively on Linux with the X11 backend.
#
# The build is out-of-tree in tmp/build-fox2-linux; the fox2 tree only
# receives ignored Autotools output from autoreconf.
#
#   ./scripts/build-fox2-linux.sh
#
# Options:
#   --opengl   enable OpenGL (needs libgl-dev)
#   --debug    compile with -g -DDEBUG
#   --release  compile with -O2 -DNDEBUG
#   --jobs=N   parallel jobs (default: all cores)
#   --clean    delete the build directory first
#   --fast     skip autoreconf/configure when their inputs did not change
#   --trace    keep FXTRACE statements compiled in (no NDEBUG)
#   --run      run hello, button and the DPI test for 3 s after building
#   --cmake    use CMake + Ninja instead of Autotools: no autoreconf and no
#              configure run (build dir: tmp/build-fox2-linux-cmake)
#   -h|--help  show this help
#
# Environment:
#   FOX_BUILD_DIR  build directory; use it under WSL to build on the Linux file
#                  system instead of /mnt/<drive>, e.g. ~/build/fox2
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SRC="$ROOT/fox2"
BUILD="${FOX_BUILD_DIR:-$ROOT/tmp/build-fox2-linux}"
LOG="$BUILD/build.log"

jobs=$(nproc 2>/dev/null || echo 4)
with_opengl=no
build_type=
clean=no
fast=no
trace=no
run_test=no
use_cmake=no

usage(){
  sed -n '2,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --opengl)   with_opengl=yes ;;
    --debug)    build_type=debug ;;
    --release)  build_type=release ;;
    --clean)    clean=yes ;;
    --fast)     fast=yes ;;
    --trace)    trace=yes ;;
    --run)      run_test=yes ;;
    --cmake)    use_cmake=yes ;;
    --jobs)     shift; jobs=$1 ;;
    --jobs=*)   jobs=${1#*=} ;;
    -h|--help)  usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

if [ -n "${MSYSTEM-}" ]; then
  echo "error: this is the Linux build; in MSYS2 use ./scripts/build-fox2-mingw.sh" >&2
  exit 1
  fi

command -v g++ >/dev/null 2>&1 || {
  echo "error: g++ not found. Install the C++ compiler (gcc)." >&2
  exit 1
  }
command -v make >/dev/null 2>&1 || {
  echo "error: make not found. Install make." >&2
  exit 1
  }
[ -f "$SRC/configure.ac" ] || {
  echo "error: $SRC/configure.ac not found." >&2
  exit 1
  }
pkg-config --exists x11 2>/dev/null || {
  echo "error: X11 development files not found (pkg-config x11)." >&2
  echo "       install libx11-dev (Debian/Ubuntu) or libx11 (Arch)." >&2
  exit 1
  }

if [ "$run_test" = yes ] && [ -z "${DISPLAY-}" ] && [ -z "${WAYLAND_DISPLAY-}" ]; then
  echo "error: --run needs a display (DISPLAY or WAYLAND_DISPLAY is not set)." >&2
  exit 1
  fi

if [ "$use_cmake" = yes ]; then
  BUILD="${FOX_BUILD_DIR:-$ROOT/tmp/build-fox2-linux-cmake}"
  LOG="$BUILD/build.log"
  for tool in cmake ninja; do
    command -v "$tool" >/dev/null 2>&1 || {
      echo "error: $tool not found. Install cmake and ninja." >&2
      exit 1
      }
    done
  if [ "$trace" = yes ] && [ "$build_type" = release ]; then
    echo "error: --trace cannot be combined with --release" >&2
    exit 2
    fi

  if [ "$clean" = yes ]; then
    echo "==> removing $BUILD"
    rm -rf -- "$BUILD"
    fi
  mkdir -p -- "$BUILD"
  : >"$LOG"

  run(){
    echo "*** $*" >>"$LOG"
    "$@" 2>&1 | tee -a "$LOG"
    }

  # CMake writes nothing into the source tree. The build type is always
  # passed so that it is reset when no option is given.
  copts=(-G Ninja -DFOX_WITH_CUPS=OFF)
  if [ "$with_opengl" = yes ]; then
    copts+=(-DFOX_WITH_OPENGL=ON)
  else
    copts+=(-DFOX_WITH_OPENGL=OFF)
    fi
  if [ "$trace" = yes ]; then
    copts+=(-DFOX_FOXTRACE=ON)
  else
    copts+=(-DFOX_FOXTRACE=OFF)
    fi
  case "$build_type" in
    debug)   copts+=(-DCMAKE_BUILD_TYPE=Debug) ;;
    release) copts+=(-DCMAKE_BUILD_TYPE=Release) ;;
    *)       copts+=(-DCMAKE_BUILD_TYPE=) ;;
  esac

  # Ninja reruns CMake by itself when a CMakeLists.txt or Makefile.am changes
  if [ "$fast" = yes ] && [ -f "$BUILD/build.ninja" ] &&
     [ "$(cat "$BUILD/.cmake-options" 2>/dev/null)" = "${copts[*]}" ]; then
    echo "==> cmake configure skipped (options unchanged)"
  else
    echo "==> cmake configure: ${copts[*]}"
    run cmake -S "$SRC" -B "$BUILD" "${copts[@]}"
    printf '%s\n' "${copts[*]}" >"$BUILD/.cmake-options"
    fi

  echo "==> build library, tools and tests"
  run cmake --build "$BUILD" -j "$jobs"

  echo
  echo "==> artifacts"
  ls -l "$BUILD"/bin/* "$BUILD"/lib/*FOX* 2>/dev/null || true

  if [ "$run_test" = yes ]; then
    for prog in hello button dpi; do
      echo
      echo "==> running $prog for 3 seconds"
      rc=0
      timeout 3 "$BUILD/bin/$prog" || rc=$?
      if [ "$rc" -eq 124 ]; then
        echo "$prog stayed in its event loop (expected)"
      else
        echo "warning: $prog exited with status $rc" >&2
        fi
      done
    echo
    echo "==> rasterizer scene (headless)"
    "$BUILD/bin/raster" --raster-only || echo "warning: raster failed" >&2
    echo
    echo "==> rectangle region test"
    "$BUILD/bin/region" || echo "warning: region failed" >&2
    echo
    echo "==> glyph cache test"
    "$BUILD/bin/glyphs" || echo "warning: glyphs failed" >&2
    echo
    echo "==> virtual window map test"
    "$BUILD/bin/winmap" || echo "warning: winmap failed" >&2
    fi

  echo
  echo "done. Build log: $LOG"
  if [ "$trace" = yes ]; then
    echo
    echo "trace build: FXTRACE is compiled in; run with -tracelevel N or FOX_TRACE_LEVEL=N"
    echo "and redirect stderr to a file, e.g. bin/dpi -tracelevel 1023 2> trace.log"
    fi
  exit 0
  fi

if [ "$clean" = yes ]; then
  echo "==> removing $BUILD"
  rm -rf -- "$BUILD"
  fi
mkdir -p -- "$BUILD"

opts=()
if [ "$with_opengl" = yes ]; then
  opts+=(--with-opengl=yes)
else
  opts+=(--with-opengl=no)
  fi
opts+=(--disable-cups)
case "$build_type" in
  debug)   opts+=(--enable-debug) ;;
  release) opts+=(--enable-release) ;;
esac
if [ "$trace" = yes ]; then
  if [ "$build_type" = release ]; then
    echo "error: --trace cannot be combined with --release" >&2
    exit 2
    fi
  opts+=(--enable-foxtrace)
  fi

: >"$LOG"

run(){
  echo "*** $*" >>"$LOG"
  "$@" 2>&1 | tee -a "$LOG"
  }

# The generated configure is stale when its autotools inputs are newer
needs_autoreconf(){
  [ -x "$SRC/configure" ] || return 0
  [ "$SRC/configure.ac" -nt "$SRC/configure" ] && return 0
  [ "$SRC/aclocal.m4" -nt "$SRC/configure" ] && return 0
  [ -n "$(find "$SRC" -name 'Makefile.am' -newer "$SRC/configure" -print -quit)" ] && return 0
  return 1
  }

# The build directory is stale when configure or any Makefile.in is newer
needs_configure(){
  [ -f "$BUILD/Makefile" ] || return 0
  [ -f "$BUILD/.configure-options" ] || return 0
  [ "$(cat "$BUILD/.configure-options")" != "${opts[*]}" ] && return 0
  [ "$SRC/configure" -nt "$BUILD/Makefile" ] && return 0
  [ -n "$(find "$SRC" -name 'Makefile.in' -newer "$BUILD/Makefile" -print -quit)" ] && return 0
  return 1
  }

if [ "$fast" = yes ] && ! needs_autoreconf; then
  echo "==> autoreconf skipped (configure is up to date)"
else
  echo "==> autoreconf"
  ( cd -- "$SRC" && autoreconf -fi ) >"$BUILD/autoreconf.log" 2>&1 || {
    echo "error: autoreconf failed, see $BUILD/autoreconf.log" >&2
    tail -n 20 "$BUILD/autoreconf.log" >&2
    exit 1
    }
  fi

cd -- "$BUILD"

if [ "$fast" = yes ] && ! needs_configure; then
  echo "==> configure skipped (build directory is up to date)"
  reconfigured=no
else
  echo "==> configure: $SRC/configure ${opts[*]}"
  run "$SRC/configure" "${opts[@]}"
  printf '%s\n' "${opts[*]}" >"$BUILD/.configure-options"
  reconfigured=yes
  fi

# Compiler and linker flags end up inside the object files, and make does not
# rebuild them when the flags change; start over in that case.
if [ "$reconfigured" = yes ]; then
  echo "==> cleaning objects (configure was rerun)"
  run make -C lib clean
  run make -C tests clean
  fi

echo "==> build library"
run make -j"$jobs" -C lib

echo "==> build tests"
run make -j"$jobs" -C tests hello button dpi raster region glyphs winmap

echo
echo "==> artifacts"
ls -l lib/.libs/*FOX* 2>/dev/null || ls -l lib/*FOX* 2>/dev/null || true
ls -l tests/hello tests/button tests/dpi tests/raster tests/region tests/glyphs tests/winmap 2>/dev/null || true

if [ "$run_test" = yes ]; then
  for prog in hello button dpi; do
    echo
    echo "==> running $prog for 3 seconds"
    rc=0
    ( cd tests && PATH="$BUILD/lib/.libs:$PATH" timeout 3 "./$prog" ) || rc=$?
    if [ "$rc" -eq 124 ]; then
      echo "$prog stayed in its event loop (expected)"
    else
      echo "warning: $prog exited with status $rc" >&2
      fi
    done
  echo
  echo "==> rasterizer scene (headless)"
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" ./raster --raster-only ) || echo "warning: raster failed" >&2
  echo
  echo "==> rectangle region test"
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" ./region ) || echo "warning: region failed" >&2
  echo
  echo "==> glyph cache test"
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" ./glyphs ) || echo "warning: glyphs failed" >&2
  echo
  echo "==> virtual window map test"
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" ./winmap ) || echo "warning: winmap failed" >&2
  fi

echo
echo "done. Build log: $LOG"
if [ "$trace" = yes ]; then
  echo
  echo "trace build: FXTRACE is compiled in; run with -tracelevel N or FOX_TRACE_LEVEL=N"
  echo "and redirect stderr to a file, e.g. tests/dpi -tracelevel 1023 2> trace.log"
  fi
