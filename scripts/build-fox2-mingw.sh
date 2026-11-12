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
#   --run      run hello and the DPI diagnostic after building
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
run_test=no

usage(){
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --opengl)   with_opengl=yes ;;
    --debug)    build_type=debug ;;
    --release)  build_type=release ;;
    --clean)    clean=yes ;;
    --run)      run_test=yes ;;
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

: >"$LOG"

run(){
  echo "*** $*" >>"$LOG"
  "$@" 2>&1 | tee -a "$LOG"
  }

echo "==> regenerating build system in $SRC"
( cd -- "$SRC" && autoreconf -fi ) >"$BUILD/autoreconf.log" 2>&1 || {
  echo "error: autoreconf failed, see $BUILD/autoreconf.log" >&2
  tail -n 20 "$BUILD/autoreconf.log" >&2
  exit 1
  }

cd -- "$BUILD"

echo "==> configure"
run "$SRC/configure" "${opts[@]}"

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
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" timeout 10 ./dpi.exe ) || true
  fi

echo
echo "done. Build log: $LOG"
