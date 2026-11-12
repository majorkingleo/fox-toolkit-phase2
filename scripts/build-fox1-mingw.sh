#!/usr/bin/env bash
#
# Build the unmodified FOX 1.7.95 tree with MSYS2/MinGW-w64 on Windows.
#
# The original source tree stays untouched: the build system is regenerated
# in tmp/fox1-src with the installed Autotools (the shipped configure was
# made with libtool 2.4.6, MSYS2 ships libtool 2.6.2), and the out-of-tree
# build itself lands in tmp/build-fox1-mingw.
#
# Run inside the MSYS2 MINGW64 environment:
#
#   ./scripts/build-fox1-mingw.sh
#
# Options:
#   --opengl       enable OpenGL (needs mingw-w64-x86_64-mesa)
#   --static       build a static library instead of the DLL
#   --debug        compile with -g -DDEBUG
#   --release      compile with -O2 -DNDEBUG
#   --jobs=N       parallel jobs (default: 12)
#   --clean        delete the build and scratch source directories first
#   --run          start tests/hello for 3 s after the build
#   -h|--help      show this help
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SRC="$ROOT/fox-1.7.95"
AUTOSRC="$ROOT/tmp/fox1-src"
BUILD="$ROOT/tmp/build-fox1-mingw"
LOG="$BUILD/build.log"

jobs=12
with_opengl=no
shared=yes
build_type=
clean=no
run_test=no

usage(){
  sed -n '2,23p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --opengl)   with_opengl=yes ;;
    --static)   shared=no ;;
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
  echo "       .\\scripts\\build-fox1-mingw.ps1" >&2
  exit 1
  fi

command -v g++ >/dev/null 2>&1 || {
  echo "error: g++ not found. Run ./scripts/install-msys2-deps.sh first." >&2
  exit 1
  }
command -v make >/dev/null 2>&1 || {
  echo "error: make not found. Run ./scripts/install-msys2-deps.sh first (install msys/make, not mingw32-make)." >&2
  exit 1
  }
[ -f "$SRC/configure.ac" ] || {
  echo "error: $SRC/configure.ac not found." >&2
  exit 1
  }

if [ "$clean" = yes ]; then
  echo "==> removing $BUILD and $AUTOSRC"
  rm -rf -- "$BUILD" "$AUTOSRC"
  fi
mkdir -p -- "$BUILD"

# The shipped configure/Makefile.in were generated with libtool 2.4.6 while
# MSYS2 ships 2.6.2; libtool refuses to work with that mismatch. Regenerate
# the build system in a scratch copy so the original tree stays untouched.
echo "==> preparing autotools sources in $AUTOSRC"
mkdir -p -- "$AUTOSRC"
cp -a "$SRC/." "$AUTOSRC/"
( cd -- "$AUTOSRC" && autoreconf -fi ) >"$BUILD/autoreconf.log" 2>&1 || {
  echo "error: autoreconf failed, see $BUILD/autoreconf.log" >&2
  tail -n 20 "$BUILD/autoreconf.log" >&2
  exit 1
  }

# WIN32 selects the Win32 backend in xincs.h/fxdefs.h and defines the FXID,
# FXEvent and FXnchar layout. lib/Makefile.am adds it for the library only,
# so the tests and example programs need it through CPPFLAGS as well.
export CPPFLAGS="-DWIN32 ${CPPFLAGS-}"

opts=(--disable-cups)
if [ "$with_opengl" = yes ]; then
  opts+=(--with-opengl=yes)
else
  opts+=(--with-opengl=no)
  fi
if [ "$shared" = yes ]; then
  opts+=(--enable-shared --disable-static)
else
  opts+=(--disable-shared --enable-static)
  fi
case "$build_type" in
  debug)   opts+=(--enable-debug) ;;
  release) opts+=(--enable-release) ;;
esac
# Both codecs are opt-in in configure.ac; the Linux baseline left them off.
opts+=(--disable-webp --disable-jp2)

: >"$LOG"

run(){
  echo "*** $*" >>"$LOG"
  "$@" 2>&1 | tee -a "$LOG"
  }

cd -- "$BUILD"

echo "==> configure: $AUTOSRC/configure ${opts[*]}"
run "$AUTOSRC/configure" "${opts[@]}"

echo "==> build reswrap (host utility, generates icons.cpp)"
run make -j"$jobs" -C utils

echo "==> build library"
run make -j"$jobs" -C lib

echo "==> build test programs"
run make -j"$jobs" -C tests hello.exe button.exe

echo
echo "==> artifacts"
ls -l lib/.libs/*FOX* 2>/dev/null || ls -l lib/*FOX* 2>/dev/null || true
ls -l tests/hello tests/hello.exe tests/.libs/hello.exe 2>/dev/null || true
ls -l tests/button tests/button.exe tests/.libs/button.exe 2>/dev/null || true

if [ "$run_test" = yes ]; then
  echo
  echo "==> running hello for 3 seconds"
  rc=0
  ( cd tests && PATH="$BUILD/lib/.libs:$PATH" timeout 3 ./hello ) || rc=$?
  if [ "$rc" -eq 124 ]; then
    echo "hello stayed in its event loop (expected)"
  else
    echo "warning: hello exited with status $rc" >&2
    fi
  fi

echo
echo "done. Build log: $LOG"
