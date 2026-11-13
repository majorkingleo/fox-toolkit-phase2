#!/usr/bin/env bash
#
# Build the fox2 library natively on Windows with MSYS2/MinGW-w64.
#
# The build is out-of-tree in tmp/build-fox2-win; the fox2 tree only receives
# ignored Autotools output from autoreconf.
#
# Run inside the MSYS2 MINGW64 environment:
#
#   ./scripts/build-fox2-mingw.sh
#
# Options:
#   --opengl   enable OpenGL (needs mingw-w64-x86_64-mesa)
#   --debug    compile with -g -DDEBUG
#   --release  compile with -O2 -DNDEBUG
#   --jobs=N   parallel jobs (default: 12)
#   --clean    delete the build directory first
#   --fast     skip autoreconf/configure when their inputs did not change
#   --trace    keep FXTRACE statements compiled in (no NDEBUG)
#   --run      run hello and the DPI diagnostic after building
#   --cmake    use CMake + Ninja instead of Autotools: no autoreconf and no
#              configure run (build dir: tmp/build-fox2-win-cmake)
#   -h|--help  show this help
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SRC="$ROOT/fox2"
BUILD="$ROOT/tmp/build-fox2-win"
LOG="$BUILD/build.log"

jobs=12
with_opengl=no
build_type=
clean=no
fast=no
trace=no
run_test=no
use_cmake=no

usage(){
  sed -n '2,24p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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

if [ "${MSYSTEM-}" != "MINGW64" ]; then
  echo "error: run this script in the MSYS2 MINGW64 environment." >&2
  echo "       start C:\\msys64\\mingw64.exe, or from PowerShell run:" >&2
  echo "       .\\scripts\\build-fox2-mingw.ps1" >&2
  exit 1
  fi

command -v g++ >/dev/null 2>&1 || {
  echo "error: g++ not found. Run ./scripts/install-msys2-deps.sh first." >&2
  exit 1
  }

if [ "$use_cmake" = yes ]; then
  BUILD="$ROOT/tmp/build-fox2-win-cmake"
  LOG="$BUILD/build.log"
  for tool in cmake ninja; do
    command -v "$tool" >/dev/null 2>&1 || {
      echo "error: $tool not found. Run ./scripts/install-msys2-deps.sh first." >&2
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
  ls -l "$BUILD"/bin/*.exe "$BUILD"/bin/*FOX* "$BUILD"/lib/*FOX* 2>/dev/null || true

  if [ "$run_test" = yes ]; then
    echo
    echo "==> running hello for 3 seconds"
    rc=0
    timeout 3 "$BUILD/bin/hello.exe" || rc=$?
    if [ "$rc" -eq 124 ]; then
      echo "hello stayed in its event loop (expected)"
    else
      echo "warning: hello exited with status $rc" >&2
      fi
    echo
    echo "==> running the DPI diagnostic"
    timeout 10 "$BUILD/bin/dpi.exe" simulate || true
    fi

  echo
  echo "done. Build log: $LOG"
  if [ "$trace" = yes ]; then
    echo
    echo "trace build: FXTRACE is compiled in; run with -tracelevel N or FOX_TRACE_LEVEL=N"
    echo "and redirect stderr to a file, e.g. bin/dpi.exe -tracelevel 1023 2> trace.log"
    fi
  exit 0
  fi

command -v make >/dev/null 2>&1 || {
  echo "error: make not found. Run ./scripts/install-msys2-deps.sh first." >&2
  exit 1
  }

if [ "$clean" = yes ]; then
  echo "==> removing $BUILD"
  rm -rf -- "$BUILD"
  fi
mkdir -p -- "$BUILD"

# WIN32 selects the Win32 backend; pass it to the tests and tools as well,
# since the library Makefile.am only adds it for the library itself.
export CPPFLAGS="-DWIN32 ${CPPFLAGS-}"

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

# Compiler and linker flags end up inside the object files and the DLL, and
# make does not rebuild them when the flags change; start over in that case.
if [ "$reconfigured" = yes ]; then
  echo "==> cleaning objects (configure was rerun)"
  run make -C lib clean
  run make -C tests clean
  fi

echo "==> build library"
run make -j"$jobs" -C lib

echo "==> build tests"
run make -j"$jobs" -C tests hello.exe button.exe dpi.exe

echo
echo "==> artifacts"
ls -l lib/.libs/*FOX* 2>/dev/null || ls -l lib/*FOX* 2>/dev/null || true
ls -l tests/.libs/*.exe 2>/dev/null || true

if [ "$run_test" = yes ]; then
  echo
  echo "==> running hello for 3 seconds"
  rc=0
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" timeout 3 ./hello.exe ) || rc=$?
  if [ "$rc" -eq 124 ]; then
    echo "hello stayed in its event loop (expected)"
  else
    echo "warning: hello exited with status $rc" >&2
    fi
  echo
  echo "==> running the DPI diagnostic"
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" timeout 10 ./dpi.exe simulate ) || true
  fi

echo
echo "done. Build log: $LOG"
if [ "$trace" = yes ]; then
  echo
  echo "trace build: FXTRACE is compiled in; run with -tracelevel N or FOX_TRACE_LEVEL=N"
  echo "and redirect stderr to a file, e.g. tests/dpi.exe -tracelevel 1023 2> trace.log"
  fi
