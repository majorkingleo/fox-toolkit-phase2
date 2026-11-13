#!/usr/bin/env bash
#
# Run the software rasterizer test (phase 1 of docs/wayland-plan.md).
#
# The test draws a fixed scene with FXRaster and, when a display is available
# and this is an X11 build, with FXDCWindow, then compares the two buffers cell
# by cell.  Each cell holds one primitive; the limits are documented in
# docs/wayland-plan.md.
#
#   ./scripts/build-fox2-linux.sh --cmake   # or the mingw build
#   ./scripts/run-fox2-raster.sh
#
# Options:
#   --headless   do not use the display (FXRaster only)
#   --out=DIR    output directory (default: tmp/raster)
#   --bin=FILE   raster binary (default: the CMake build in tmp)
#   -h|--help    show this help
#
# Exit status: 0 when every cell is within its limit, 1 on pixel differences
# above the limit, 2 on usage or I/O errors.
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
OUT="$ROOT/tmp/raster"
BIN="$ROOT/tmp/build-fox2-linux-cmake/bin/raster"
headless=no

while [ $# -gt 0 ]; do
  case "$1" in
    --headless) headless=yes ;;
    --out)      shift; OUT=$1 ;;
    --out=*)    OUT=${1#*=} ;;
    --bin)      shift; BIN=$1 ;;
    --bin=*)    BIN=${1#*=} ;;
    -h|--help)  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

if [ ! -x "$BIN" ]; then
  echo "error: $BIN not found; build it first:" >&2
  echo "       ./scripts/build-fox2-linux.sh --cmake" >&2
  exit 2
  fi

mkdir -p -- "$OUT"

echo "== FXRaster scene"
"$BIN" --raster-only --out="$OUT"

if [ "$headless" = yes ] || [ -z "${DISPLAY-}" ]; then
  echo
  echo "== X11 comparison skipped (headless or no DISPLAY)"
  exit 0
  fi

echo
echo "== comparison against the X11 drawing of the same scene"
"$BIN" --out="$OUT"
