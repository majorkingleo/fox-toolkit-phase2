#!/usr/bin/env bash
#
# Gate: the SVG codec loads and rasterizes on the Wayland build.
#
# tests/svg parses an embedded document, checks the natural size, a requested
# raster size and the pixels of an FXSVGImage, an FXSVGIcon and both
# FXIconSource paths.  No window is opened, so a headless sway with the pixman
# renderer is enough.
#
#   ./scripts/gate-wayland-svg.sh
#   ./scripts/gate-wayland-svg.sh --build=DIR
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-svg-gate"

while [ $# -gt 0 ]; do
  case "$1" in
    --build)     shift; BUILD=$1 ;;
    --build=*)   BUILD=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help)   sed -n '2,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

BIN="$BUILD/bin/svg"
[ -x "$BIN" ] || { echo "error: no svg test at $BIN (build fox2-wayland)" >&2; exit 2; }
command -v sway >/dev/null 2>&1 || { echo "error: sway not found" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/runtime" "$RUN/home"
chmod 700 "$RUN/runtime"
printf 'output HEADLESS-1 resolution 640x480 bg #101010 solid_color\n' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN/runtime" HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
cleanup(){
  kill "$sway_pid" 2>/dev/null || true
  wait "$sway_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

sock=""
for i in $(seq 1 200); do
  sock=$(ls "$RUN/runtime"/wayland-* 2>/dev/null | head -1)
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket; see $RUN/sway.log" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

echo "==> running svg"
timeout 60 "$BIN" >"$RUN/app.log" 2>&1
rc=$?
sed 's/^/    /' "$RUN/app.log"
if [ "$rc" -eq 0 ] && grep -q "PASS: SVG image and icon load" "$RUN/app.log"; then
  echo "PASS: the SVG codec rasterizes image and icon"
  exit 0
  fi
echo "FAIL: the SVG checks failed (exit $rc)"
exit 1
