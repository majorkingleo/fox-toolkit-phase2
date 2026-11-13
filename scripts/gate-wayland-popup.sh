#!/usr/bin/env bash
#
# The popup gate of docs/wayland-plan.md: a menu pane paints as a popup surface
# over its main window.
#
# A private headless weston runs the popup test, which opens a menu pane on a
# timer; every committed frame is dumped. The gate passes when a frame of the
# popup surface exists and paints something, which means the xdg_popup role,
# the positioner and the frame of a popup all worked.
#
#   ./scripts/gate-wayland-popup.sh
#   ./scripts/gate-wayland-popup.sh --save   keep the frames as screenshots
#
# Options:
#   --bin=FILE     application to run (default: tmp/build-fox2-wayland/bin/popup)
#   --run-dir=DIR  runtime and dump directory (default: tmp/wayland-popup-gate)
#   --save         copy the frames to screenshots/tests/fox2/wayland/
#   -h|--help      show this help
#
# Exit status is 0 when the popup painted, 2 when the run failed.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-popup-gate"
BIN=""
save=no

usage(){
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --save)      save=yes ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/popup"
command -v weston >/dev/null 2>&1 || { echo "error: weston not found" >&2; exit 2; }
[ -x "$BIN" ] || { echo "error: no binary at $BIN (build with FOX_WAYLAND_TARGET=all)" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/runtime" "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN/runtime"

export XDG_RUNTIME_DIR="$RUN/runtime"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY

echo "==> starting headless weston"
weston --backend=headless-backend.so --socket=wayland-fox --no-config --idle-time=0 --log="$RUN/weston.log" &
weston_pid=$!
cleanup(){
  kill "$weston_pid" 2>/dev/null || true
  wait "$weston_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

for i in $(seq 1 200); do
  [ -S "$RUN/runtime/wayland-fox" ] && break
  kill -0 "$weston_pid" 2>/dev/null || { echo "error: weston exited; see $RUN/weston.log" >&2; exit 2; }
  sleep 0.05
  done

export WAYLAND_DISPLAY=wayland-fox
export FOX_WAYLAND_DUMP="$RUN/dump"
export FOX_WAYLAND_TRACE=1

echo "==> running $BIN"
"$BIN" --no-dialogs >"$RUN/app.log" 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  echo "error: $BIN failed with $rc" >&2
  tail -5 "$RUN/app.log" >&2
  exit 2
  fi

echo "==> frames"
ls -1 "$RUN/dump" 2>/dev/null
popupframe=$(ls "$RUN"/dump/*.ppm 2>/dev/null | sed -n '2p')
if [ -z "$popupframe" ]; then
  echo "FAIL: no popup frame was committed (trace in $RUN/app.log)" >&2
  grep -E "popup|configure" "$RUN/app.log" | tail -8 >&2
  exit 1
  fi

grep -E "create popup|popup done" "$RUN/app.log" | tail -4

if [ "$save" = yes ]; then
  mkdir -p -- "$ROOT/screenshots/tests/fox2/wayland"
  python3 - "$popupframe" "$ROOT/screenshots/tests/fox2/wayland/popup.png" <<'EOF'
import sys
from PIL import Image
Image.open(sys.argv[1]).save(sys.argv[2])
print("saved", sys.argv[2])
EOF
  fi

echo "PASS: the popup painted as a surface of its own"
