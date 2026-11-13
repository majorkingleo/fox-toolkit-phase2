#!/usr/bin/env bash
#
# The phase 4 gate of docs/wayland-plan.md: hello paints the same pixels as it
# does on X11.
#
# A private headless weston gets its own runtime directory, the application gets
# a private HOME so that it reads the built-in "classic" theme instead of the
# desktop's light/dark preference, and every committed buffer is dumped. The
# dump is compared with the X11 screenshot, pixel by pixel.
#
#   ./scripts/gate-wayland-hello.sh
#   ./scripts/gate-wayland-hello.sh --save      also store the wayland screenshot
#   ./scripts/gate-wayland-hello.sh --bin=FILE  use another binary
#
# Options:
#   --bin=FILE     application to run (default: tmp/build-fox2-wayland/bin/hello)
#   --build=DIR    build directory (default: tmp/build-fox2-wayland)
#   --run-dir=DIR  compositor runtime directory (default: tmp/wayland-gate)
#   --dump=DIR     where the frames are written (default: tmp/wayland-gate/dump)
#   --ref=FILE     reference image (default: screenshots/tests/fox2/x11/hello.png)
#   --save         copy the dump to screenshots/tests/fox2/wayland/hello.png
#   --debug        run with WAYLAND_DEBUG=1 and keep the protocol log
#   --seconds=N    how long the application may run (default: 4)
#   -h|--help      show this help
#
# Exit status is 0 when the frames match, 1 when they differ, 2 when the run
# itself failed.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-gate"
DUMP=""
REF="$ROOT/screenshots/tests/fox2/x11/hello.png"
BIN=""
SECONDS_TO_RUN=4
save=no
debug=no

usage(){
  sed -n '2,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --build)     shift; BUILD=$1 ;;
    --build=*)   BUILD=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --dump)      shift; DUMP=$1 ;;
    --dump=*)    DUMP=${1#*=} ;;
    --ref)       shift; REF=$1 ;;
    --ref=*)     REF=${1#*=} ;;
    --seconds)   shift; SECONDS_TO_RUN=$1 ;;
    --seconds=*) SECONDS_TO_RUN=${1#*=} ;;
    --save)      save=yes ;;
    --debug)     debug=yes ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/hello"
[ -n "$DUMP" ] || DUMP="$RUN/dump"

if [ ! -x "$BIN" ]; then
  echo "error: no binary at $BIN (build with FOX_WAYLAND_TARGET=all scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi
if ! command -v weston >/dev/null 2>&1; then
  echo "error: weston not found; install weston for the headless compositor" >&2
  exit 2
  fi
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to compare the frames" >&2
  exit 2
  fi
if [ ! -f "$REF" ]; then
  echo "error: reference image $REF is missing" >&2
  exit 2
  fi

rm -rf -- "$RUN"
mkdir -p -- "$RUN/runtime" "$RUN/home" "$DUMP"
RUN=$(cd -- "$RUN" && pwd)
DUMP=$(cd -- "$DUMP" && pwd)
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
if [ ! -S "$RUN/runtime/wayland-fox" ]; then
  echo "error: weston did not create a socket" >&2
  exit 2
  fi

export WAYLAND_DISPLAY=wayland-fox
export FOX_WAYLAND_DUMP="$DUMP"

echo "==> running $BIN for ${SECONDS_TO_RUN}s"
if [ "$debug" = yes ]; then
  export WAYLAND_DEBUG=1
  fi
timeout -k 2 "$SECONDS_TO_RUN" "$BIN" --no-dialogs >"$RUN/run.log" 2>&1
rc=$?
if [ "$rc" -ne 124 ] && [ "$rc" -ne 0 ] && [ "$rc" -ne 143 ]; then
  echo "error: $BIN failed with $rc" >&2
  exit 2
  fi

frame=$(ls "$DUMP"/*.ppm 2>/dev/null | head -1)
if [ -z "$frame" ]; then
  echo "error: no frame was committed; nothing to compare" >&2
  exit 2
  fi

echo "==> comparing $frame with $REF"
python3 - "$frame" "$REF" <<'EOF'
import sys
from collections import Counter
from PIL import Image

frame, ref = sys.argv[1], sys.argv[2]
a = Image.open(frame).convert("RGB")
b = Image.open(ref).convert("RGB")
print(f"frame {a.width}x{a.height}  reference {b.width}x{b.height}")
w, h = min(a.width, b.width), min(a.height, b.height)
pa, pb = a.load(), b.load()
differing = 0
worst = 0
for y in range(h):
    for x in range(w):
        ca, cb = pa[x, y], pb[x, y]
        d = max(abs(ca[0] - cb[0]), abs(ca[1] - cb[1]), abs(ca[2] - cb[2]))
        if d:
            differing += 1
            worst = max(worst, d)
print(f"differing {differing} of {w * h} pixels, worst channel delta {worst}")
if a.size != b.size:
    print(f"note: the reference has {b.width - w} extra columns and {b.height - h} extra rows")
sys.exit(0 if differing == 0 else 1)
EOF
verdict=$?

if [ "$save" = yes ]; then
  mkdir -p -- "$ROOT/screenshots/tests/fox2/wayland"
  python3 - "$frame" "$ROOT/screenshots/tests/fox2/wayland/hello.png" <<'EOF'
import sys
from PIL import Image
Image.open(sys.argv[1]).save(sys.argv[2])
print("saved", sys.argv[2])
EOF
  fi

if [ "$verdict" -ne 0 ]; then
  echo "FAIL: the frame differs from the X11 screenshot" >&2
  exit 1
  fi
echo "PASS: the frame matches the X11 screenshot"
