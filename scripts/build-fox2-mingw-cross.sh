#!/usr/bin/env bash
#
# Cross-compile fox2 for Windows with MinGW-w64 on Linux (CachyOS/Arch).
#
# This is the fast compile check for the Win32 backend on the build host;
# the resulting DLL and test programs can be smoke-tested with Wine.  The
# build is out-of-tree in tmp/build-fox2-cross.
#
#   ./scripts/build-fox2-mingw-cross.sh
#
# Options:
#   --opengl   enable OpenGL (needs mingw-w64-mesa)
#   --debug    compile with -g -DDEBUG
#   --release  compile with -O2 -DNDEBUG
#   --jobs=N   parallel jobs (default: all cores)
#   --clean    delete the build directory first
#   --fast     skip autoreconf/configure when their inputs did not change
#   --trace    keep FXTRACE statements compiled in (no NDEBUG)
#   --run      run hello, button and dpi under Wine after building
#   -h|--help  show this help
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SRC="$ROOT/fox2"
BUILD="$ROOT/tmp/build-fox2-cross"
LOG="$BUILD/build.log"
HOST=x86_64-w64-mingw32
SYSROOT=${MINGW_SYSROOT:-/usr/x86_64-w64-mingw32}

jobs=$(nproc 2>/dev/null || echo 4)
with_opengl=no
build_type=
clean=no
fast=no
trace=no
run_test=no
run_seconds=5

usage(){
  sed -n '2,21p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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
    --jobs)     shift; jobs=$1 ;;
    --jobs=*)   jobs=${1#*=} ;;
    --run-seconds) shift; run_seconds=$1 ;;
    --run-seconds=*) run_seconds=${1#*=} ;;
    -h|--help)  usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

if [ -n "${MSYSTEM-}" ]; then
  echo "error: this is the cross build; in MSYS2 use ./scripts/build-fox2-mingw.sh" >&2
  exit 1
  fi

command -v "$HOST-g++" >/dev/null 2>&1 || {
  echo "error: $HOST-g++ not found." >&2
  echo "       run ./scripts/install-cachyos-mingw.sh first." >&2
  exit 1
  }
command -v make >/dev/null 2>&1 || {
  echo "error: make not found." >&2
  exit 1
  }
[ -f "$SRC/configure.ac" ] || {
  echo "error: $SRC/configure.ac not found." >&2
  exit 1
  }
if [ "$run_test" = yes ] && ! command -v wine >/dev/null 2>&1; then
  echo "error: --run needs wine; ./scripts/install-cachyos-mingw.sh installs it." >&2
  exit 1
  fi

if [ "$clean" = yes ]; then
  echo "==> removing $BUILD"
  rm -rf -- "$BUILD"
  fi
mkdir -p -- "$BUILD"

# WIN32 selects the Win32 backend; lib/Makefile.am adds it for the library
# only, so expose it through CPPFLAGS to the tests and tools as well.
export CPPFLAGS="-DWIN32 ${CPPFLAGS-}"

opts=("--host=$HOST" --without-x)
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

# The Win32 backend must be selected.  --without-x disables X detection, but
# AC_PATH_XTRA still probes for X libraries later on; with a target sysroot
# that has no X11 headers and libraries those probes must fail.
if ! grep -q '^X_BASE_LIBS = .*-lcomctl32' "$BUILD/lib/Makefile" ||
   grep -q '^X_BASE_LIBS = .*-lX' "$BUILD/lib/Makefile"; then
  echo "error: configure did not select the Win32 platform libraries." >&2
  echo "       X_BASE_LIBS in $BUILD/lib/Makefile is:" >&2
  grep '^X_BASE_LIBS =' "$BUILD/lib/Makefile" >&2 || true
  exit 1
  fi

echo "==> build library"
run make -j"$jobs" -C lib

echo "==> build tests"
run make -j"$jobs" -C tests hello.exe button.exe dpi.exe

echo
echo "==> artifacts"
ls -l lib/.libs/*.dll lib/.libs/*.dll.a 2>/dev/null || true
ls -l tests/.libs/hello.exe tests/.libs/button.exe tests/.libs/dpi.exe 2>/dev/null || true

if [ "$run_test" = yes ]; then
  # A dedicated prefix keeps the user's Wine setup untouched; the first run
  # creates it, which takes a while.
  if [ -z "${WINEPREFIX-}" ]; then
    WINEPREFIX="$ROOT/tmp/wineprefix"
    fi
  export WINEPREFIX
  # The import libraries link against the MinGW runtime DLLs, which wine has
  # to find next to our own DLL.
  export WINEDEBUG="${WINEDEBUG:--all}"
  export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-mscoree,mshtml=}"
  export WINEPATH="$BUILD/lib/.libs;$SYSROOT/bin"
  if [ ! -d "$WINEPREFIX" ]; then
    echo
    echo "==> initializing the Wine prefix in $WINEPREFIX"
    timeout 180 wineboot --init >/dev/null 2>&1 || true
    fi
  for prog in hello button dpi; do
    exe="tests/.libs/$prog.exe"
    [ -f "$exe" ] || exe="tests/$prog.exe"
    echo
    echo "==> running $prog under Wine for ${run_seconds} seconds"
    rc=0
    timeout "$run_seconds" wine "$exe" >"$BUILD/$prog.wine.log" 2>&1 || rc=$?
    tail -n 20 "$BUILD/$prog.wine.log"
    if [ "$rc" -eq 124 ]; then
      echo "$prog stayed in its event loop (expected)"
    elif [ "$rc" -eq 0 ]; then
      echo "note: $prog exited on its own"
    else
      echo "warning: $prog exited with status $rc" >&2
      echo "         retrying with Wine diagnostics" >&2
      rc=0
      timeout "$run_seconds" env WINEDEBUG= wine "$exe" >>"$BUILD/$prog.wine.log" 2>&1 || rc=$?
      tail -n 20 "$BUILD/$prog.wine.log" >&2
      if [ "$rc" -eq 124 ]; then
        echo "$prog reached its event loop with diagnostics; runtime DLLs were missing before"
      fi
      fi
    done
  fi

echo
echo "done. Build log: $LOG"
if [ "$trace" = yes ]; then
  echo
  echo "trace build: FXTRACE is compiled in; run with -tracelevel N or FOX_TRACE_LEVEL=N"
  echo "and redirect stderr, e.g. wine tests/.libs/dpi.exe -tracelevel 1023 2> trace.log"
  fi
