#!/usr/bin/env bash
#
# Run the ported calculator against the display that is already up.
#
# The calculator keeps its colours, fonts, precision and window placement in
# $HOME/.config/Calculator.rc, so this script gives it a private HOME under
# tmp/ by default: every run starts from the built-in defaults, which is what
# the reference screenshots show.  Use --home to keep a settings directory.
#
#   ./scripts/run-fox2-calculator.sh                 X11 build, $DISPLAY
#   ./scripts/run-fox2-calculator.sh --wayland       Wayland build, $WAYLAND_DISPLAY
#   ./scripts/run-fox2-calculator.sh --build=DIR     another build directory
#   ./scripts/run-fox2-calculator.sh --home=DIR      keep settings in DIR
#   ./scripts/run-fox2-calculator.sh -h|--help
#
# The builds are made by scripts/build-fox2-linux.sh (X11) and
# scripts/build-fox2-wayland.sh (Wayland).  The Windows binary from
# scripts/build-fox2-mingw-cross.sh is started under Wine directly
# (tmp/build-fox2-cross/apps/calculator/.libs/calculator.exe).
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-linux
HOME_DIR=
backend=x11

usage(){
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --wayland)   backend=wayland ;;
    --build)     shift; BUILD=$1 ;;
    --build=*)   BUILD=${1#*=} ;;
    --home)      shift; HOME_DIR=$1 ;;
    --home=*)    HOME_DIR=${1#*=} ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

if [ "$backend" = wayland ]; then
  BIN="$BUILD/bin/calculator"
  if [ -z "${WAYLAND_DISPLAY-}" ]; then
    echo "error: WAYLAND_DISPLAY is not set; start a compositor first" >&2
    exit 2
    fi
else
  BIN="$BUILD/apps/calculator/calculator"
  if [ -z "${DISPLAY-}" ]; then
    echo "error: DISPLAY is not set; start an X server first" >&2
    exit 2
    fi
  fi

if [ ! -x "$BIN" ]; then
  echo "error: no calculator at $BIN" >&2
  echo "       build it with scripts/build-fox2-linux.sh" >&2
  echo "       (the Wayland build needs scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi

if [ -z "$HOME_DIR" ]; then
  HOME_DIR=tmp/calculator-home
  fi
mkdir -p -- "$HOME_DIR"
HOME_DIR=$(cd -- "$HOME_DIR" && pwd)

echo "==> $BIN"
echo "==> HOME=$HOME_DIR"
exec env HOME="$HOME_DIR" "$BIN"
