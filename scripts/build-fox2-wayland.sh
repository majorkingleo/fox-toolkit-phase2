#!/usr/bin/env bash
#
# Build the fox2 library with the Wayland platform and check the Wayland gate.
#
# The Wayland platform is developed in the phases of docs/wayland-plan.md.  Up
# to phase 3 only the library is built: the test programs need FXApp, which
# arrives with the connection in phase 4.  The --check option verifies the
# environment (compositor socket, client libraries) only.
#
#   ./scripts/build-fox2-wayland.sh --check
#   ./scripts/build-fox2-wayland.sh
#
# Options:
#   --check    only check the environment, do not build
#   --jobs=N   parallel jobs (default: all cores)
#   --clean    delete the build directory first
#   --run      run hello, button and the DPI test for 3 s after building
#              (from phase 4 on; earlier the library is built only)
#   -h|--help  show this help
#
# Environment:
#   FOX_BUILD_DIR        build directory (default: tmp/build-fox2-wayland);
#                        under WSL use the Linux file system, e.g. ~/build/fox2-wayland
#   FOX_WAYLAND_TARGET   build target (default: FOX; use all for everything)
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SRC="$ROOT/fox2"
BUILD="${FOX_BUILD_DIR:-$ROOT/tmp/build-fox2-wayland}"
LOG="$BUILD/build.log"

jobs=$(nproc 2>/dev/null || echo 4)
check_only=no
clean=no
run_test=no

usage(){
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --check)    check_only=yes ;;
    --clean)    clean=yes ;;
    --run)      run_test=yes ;;
    --jobs)     shift; jobs=$1 ;;
    --jobs=*)   jobs=${1#*=} ;;
    -h|--help)  usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

if [ -n "${MSYSTEM-}" ]; then
  echo "error: Wayland needs Linux; in MSYS2 use ./scripts/build-fox2-mingw.sh" >&2
  exit 1
  fi

# Wayland gate: a compositor socket must be reachable
sock="${XDG_RUNTIME_DIR-}/${WAYLAND_DISPLAY:-wayland-0}"
if [ ! -S "$sock" ] && [ -S /mnt/wslg/runtime-dir/${WAYLAND_DISPLAY:-wayland-0} ]; then
  # WSL without a systemd user session: WSLg keeps its sockets here
  export XDG_RUNTIME_DIR=/mnt/wslg/runtime-dir
  sock="$XDG_RUNTIME_DIR/${WAYLAND_DISPLAY:-wayland-0}"
  fi
if [ -S "$sock" ]; then
  echo "==> compositor socket: $sock"
else
  echo "warning: no Wayland compositor socket at '$sock'" >&2
  echo "         (WSLg provides \$XDG_RUNTIME_DIR/wayland-0; KDE sets WAYLAND_DISPLAY)" >&2
  [ "$run_test" = no ] || { echo "error: --run needs a compositor." >&2; exit 1; }
  fi

# Client libraries and tools used by the Wayland platform (phases 1 to 10)
missing=0
for pc in wayland-client xkbcommon; do
  if pkg-config --exists "$pc" 2>/dev/null; then
    echo "==> found $pc $(pkg-config --modversion "$pc")"
  else
    echo "warning: pkg-config module '$pc' not found" >&2
    missing=1
    fi
  done
if command -v wayland-scanner >/dev/null 2>&1; then
  echo "==> found wayland-scanner"
else
  echo "warning: wayland-scanner not found" >&2
  missing=1
  fi

# Protocol XML for xdg-shell and the optional protocols (vendored in phase 4)
protos=
for d in "${WAYLAND_PROTOCOLS_DIR-}" /usr/share/wayland-protocols /usr/local/share/wayland-protocols; do
  [ -n "$d" ] && [ -f "$d/stable/xdg-shell/xdg-shell.xml" ] && { protos=$d; break; }
  done
if [ -n "$protos" ]; then
  echo "==> found wayland-protocols in $protos"
else
  echo "warning: xdg-shell.xml not found; install wayland-protocols" >&2
  missing=1
  fi

# Keymap data for libxkbcommon, read at run time
if [ -d /usr/share/X11/xkb ]; then
  echo "==> found xkb data in /usr/share/X11/xkb"
else
  echo "warning: /usr/share/X11/xkb not found; install xkeyboard-config" >&2
  missing=1
  fi

if [ "$missing" -ne 0 ]; then
  echo "         install libwayland-dev libxkbcommon-dev libxkbcommon-x11-0 wayland-protocols xkeyboard-config" >&2
  echo "         (Debian/Ubuntu) or wayland libxkbcommon wayland-protocols xkeyboard-config (Arch)" >&2
  fi

# Optional, only for the gates and for debugging
for tool in weston wayland-info; do
  if command -v "$tool" >/dev/null 2>&1; then
    echo "==> found $tool (optional)"
  else
    echo "==> $tool not installed (optional: headless compositor, protocol dump)"
    fi
  done

if [ "$check_only" = yes ]; then
  exit "$missing"
  fi

for tool in cmake ninja g++; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "error: $tool not found." >&2
    exit 1
    }
  done

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

echo "==> cmake configure"
run cmake -S "$SRC" -B "$BUILD" -G Ninja -DFOX_PLATFORM=wayland -DFOX_WITH_CUPS=OFF -DFOX_WITH_OPENGL=OFF

echo "==> build library"
run cmake --build "$BUILD" --target "${FOX_WAYLAND_TARGET:-FOX}" -j "$jobs"

echo
echo "==> artifacts"
ls -l "$BUILD"/lib/*FOX* 2>/dev/null || true

if [ "$run_test" = yes ]; then
  echo
  echo "warning: --run needs the GUI tests, which arrive with FXApp in phase 4" >&2
  fi

echo
echo "done. Build log: $LOG"
