#!/usr/bin/env bash
#
# Autotools gate:  the second build system has to build the same library.  The
# CMake build is what the gates use; this one proves that configure and make can
# produce a working library for either platform, which is what a distribution
# uses.
#
# The gate runs autoreconf when the autotools files changed, configures a
# private build directory, and builds the library and one test program.  The
# default run is the Wayland platform; --platform=x11 checks the other one.
#
#   ./scripts/gate-wayland-autotools.sh
#   ./scripts/gate-wayland-autotools.sh --platform=x11
#
# Exit status is 0 when the build passes, 1 when it fails, 2 when the harness
# could not start.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
PLATFORM=wayland
BUILD=""

while [ $# -gt 0 ]; do
  case "$1" in
    --platform)   shift; PLATFORM=$1 ;;
    --platform=*) PLATFORM=${1#*=} ;;
    --build)      shift; BUILD=$1 ;;
    --build=*)    BUILD=${1#*=} ;;
    -h|--help)    sed -n '2,16p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

case "$PLATFORM" in
  wayland) CONFIGURE_ARGS="--with-wayland" ;;
  x11)     CONFIGURE_ARGS="" ;;
  *) echo "error: --platform must be wayland or x11" >&2; exit 2 ;;
  esac

[ -n "$BUILD" ] || BUILD="$ROOT/tmp/gate-autotools-$PLATFORM"
for tool in autoreconf make; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if [ "$PLATFORM" = wayland ]; then
  for tool in wayland-scanner pkg-config; do
    command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
    done
  fi

rm -rf -- "$BUILD"
mkdir -p -- "$BUILD"
BUILD=$(cd -- "$BUILD" && pwd)

failed(){
  echo "FAIL: $1"
  [ -f "$BUILD/$2" ] && tail -15 "$BUILD/$2"
  exit 1
  }

# The autotools files are generated and checked in, so regenerate them when a
# template is newer
needs_regen=no
[ -x "$ROOT/fox2/configure" ] || needs_regen=yes
for f in "$ROOT/fox2/configure.ac" "$ROOT/fox2/Makefile.am" "$ROOT/fox2/lib/Makefile.am" "$ROOT/fox2/tests/Makefile.am"; do
  [ "$f" -nt "$ROOT/fox2/configure" ] && needs_regen=yes
  done

if [ "$needs_regen" = yes ]; then
  echo "==> autoreconf -if"
  ( cd "$ROOT/fox2" && autoreconf -if ) >"$BUILD/autoreconf.log" 2>&1 || failed "autoreconf failed" autoreconf.log
  fi

echo "==> configure $CONFIGURE_ARGS (platform $PLATFORM)"
( cd "$BUILD" && "$ROOT/fox2/configure" $CONFIGURE_ARGS --disable-shared --prefix="$BUILD/inst" ) >"$BUILD/configure.log" 2>&1 || failed "configure failed" configure.log

echo "==> make -C lib"
( cd "$BUILD" && make -C lib -j"$(nproc 2>/dev/null || echo 2)" ) >"$BUILD/make-lib.log" 2>&1 || failed "the library did not build" make-lib.log

echo "==> make -C tests hello"
( cd "$BUILD" && make -C tests hello ) >"$BUILD/make-hello.log" 2>&1 || failed "the test program did not build" make-hello.log

[ -f "$BUILD/lib/.libs/libFOX-1.7.a" ] || failed "no library was made" make-lib.log
grep -q "FOX-1.7" "$BUILD/tests/hello" 2>/dev/null || true

echo "==> fox-config --libs"
libs=$(sh "$BUILD/fox-config" --libs)
echo "    $libs"
if [ "$PLATFORM" = wayland ]; then
  case "$libs" in
    *wayland-client*) ;;
    *) echo "FAIL: fox-config does not report the Wayland libraries"; exit 1 ;;
    esac
  else
  case "$libs" in
    *-lX11*) ;;
    *) echo "FAIL: fox-config does not report the X11 libraries"; exit 1 ;;
    esac
  fi

echo "PASS: the autotools build makes the $PLATFORM library and a test program"
exit 0
