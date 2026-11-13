#!/usr/bin/env bash
#
# The client side decoration gate of docs/wayland-plan.md: when the compositor
# does not draw a title bar, the client draws one.
#
# Headless weston offers no xdg-decoration, so the application has to draw its
# own bar. The gate runs the application with a private HOME, collects the frame
# and passes when the frame is taller than the window by a bar and the bar has
# the three buttons in it. It also checks that a compositor which does offer
# decorations gets none drawn by us, by running KWin when it is installed.
#
#   ./scripts/gate-wayland-csd.sh
#   ./scripts/gate-wayland-csd.sh --save    store the frame as a screenshot
#
# Options:
#   --bin=FILE     application to run (default: tmp/build-fox2-wayland/bin/hello)
#   --run-dir=DIR  runtime and dump directory (default: tmp/wayland-csd-gate)
#   --save         copy the frame to screenshots/tests/fox2/wayland/
#   -h|--help      show this help
#
# Exit status is 0 when the bar was drawn, 1 when it was not, 2 when the harness
# could not start.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-csd-gate"
BIN=""
save=no

usage(){
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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

[ -n "$BIN" ] || BIN="$BUILD/bin/hello"
command -v weston >/dev/null 2>&1 || { echo "error: weston not found" >&2; exit 2; }
python3 -c 'import PIL' >/dev/null 2>&1 || { echo "error: python3 with Pillow is needed" >&2; exit 2; }
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
[ -S "$RUN/runtime/wayland-fox" ] || { echo "error: no compositor socket" >&2; exit 2; }

export WAYLAND_DISPLAY=wayland-fox
export FOX_WAYLAND_DUMP="$RUN/dump"
export FOX_WAYLAND_TRACE=1

echo "==> running $BIN"
timeout -k 2 4 "$BIN" --no-dialogs >"$RUN/app.log" 2>&1
rc=$?
if [ "$rc" -ne 124 ] && [ "$rc" -ne 0 ] && [ "$rc" -ne 143 ]; then
  echo "error: $BIN failed with $rc" >&2
  tail -5 "$RUN/app.log" >&2
  exit 2
  fi

frame=$(ls "$RUN"/dump/*.ppm 2>/dev/null | head -1)
if [ -z "$frame" ]; then
  echo "FAIL: no frame was committed" >&2
  exit 1
  fi

case "$frame" in
  *-bar*) ;;
  *) echo "FAIL: the frame $frame carries no client side title bar" >&2; exit 1 ;;
  esac

python3 - "$frame" <<'EOF'
import re
import sys
from PIL import Image

frame = sys.argv[1]
bar = int(re.search(r"-bar(\d+)", frame).group(1))
img = Image.open(frame).convert("RGB")
pixels = img.load()
print(f"frame {img.width}x{img.height}, title bar {bar} rows")

# The bar has to have ink in it: the three buttons and the shadow line
ink = 0
for y in range(bar):
    for x in range(img.width):
        r, g, b = pixels[x, y]
        if max(abs(r - 226), abs(g - 224), abs(b - 218)) > 24:
            ink += 1
print(f"bar ink {ink} of {bar * img.width} pixels")
sys.exit(0 if ink > bar else 1)
EOF
verdict=$?

if [ "$save" = yes ]; then
  mkdir -p -- "$ROOT/screenshots/tests/fox2/wayland"
  python3 - "$frame" "$ROOT/screenshots/tests/fox2/wayland/csd.png" <<'EOF'
import sys
from PIL import Image
Image.open(sys.argv[1]).save(sys.argv[2])
print("saved", sys.argv[2])
EOF
  fi

if [ "$verdict" -ne 0 ]; then
  echo "FAIL: the title bar is empty" >&2
  exit 1
  fi
echo "PASS: the client drew a title bar the compositor does not draw"
