#!/usr/bin/env bash
#
# Calculator gate: the ported calculator runs on the headless compositor, a
# virtual pointer click on the constant memory label reaches the window, and
# the committed frame matches the X11 reference pixel for pixel.
#
# The click is not there to prove that a button reacts; it is there to run the
# GUI update pass, which presses Dec and Rad (the calculator's default base and
# angle mode).  It lands on the label rather than on a button because a
# headless sway has no keyboard seat: no window ever gets the keyboard focus,
# so a frame with a focus ring cannot be reproduced.  The Stage 0 reference is
# captured the same way (docs/calculator-reference.md).
#
#   ./scripts/gate-wayland-calculator.sh
#   ./scripts/gate-wayland-calculator.sh --save    also store the wayland screenshot
#   ./scripts/gate-wayland-calculator.sh --bin=FILE
#
# Options:
#   --bin=FILE     application to run (default: tmp/build-fox2-wayland/bin/calculator)
#   --build=DIR    build directory (default: tmp/build-fox2-wayland)
#   --run-dir=DIR  compositor runtime directory (default: tmp/sway-calculator)
#   --dump=DIR     where the frames are written (default: RUN/dump)
#   --ref=FILE     reference image (default: screenshots/tests/fox1/x11/calculator-main.png)
#   --save         copy the dump to screenshots/tests/fox2/wayland/calculator.png
#   --debug        run with WAYLAND_DEBUG=1 and keep the protocol log
#   -h|--help      show this help
#
# Exit status is 0 when the frame matches, 1 when it differs, 2 when the run
# itself failed.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

ROOT=$(pwd)
BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-calculator
DUMP=
REF=screenshots/tests/fox1/x11/calculator-main.png
OUT=tmp/wlxinject
SRC=scripts/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
save=no
debug=no

usage(){
  sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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
    --save)      save=yes ;;
    --debug)     debug=yes ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/calculator"
[ -n "$DUMP" ] || DUMP="$RUN/dump"

if [ ! -x "$BIN" ]; then
  echo "error: no binary at $BIN (build with scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi
for tool in sway swaymsg wayland-scanner cc; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to compare the frames" >&2
  exit 2
  fi
if [ ! -f "$REF" ]; then
  echo "error: reference image $REF is missing" >&2
  exit 2
  fi

# The virtual pointer injector
mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN" "$DUMP"
mkdir -p -- "$RUN/home" "$DUMP"
RUN=$(cd -- "$RUN" && pwd)
DUMP=$(cd -- "$DUMP" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1024 WLXINJECT_HEIGHT=640

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
cleanup(){
  kill "${app_pid:-}" 2>/dev/null || true
  kill "$sway_pid" 2>/dev/null || true
  wait "$sway_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

sock=""
for i in $(seq 1 200); do
  sock=$(ls "$RUN"/wayland-* 2>/dev/null | head -1)
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket; see $RUN/sway.log" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

echo "==> running $BIN"
if [ "$debug" = yes ]; then
  export WAYLAND_DEBUG=1
  fi
FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$DUMP" "$BIN" >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2

fail(){
  echo "FAIL: $1" >&2
  grep -E "create surface|toplevel configure|commit window" "$RUN/app.log" | tail -8
  exit 1
  }

alive(){
  kill -0 "$app_pid" 2>/dev/null
  }

alive || fail "the application did not stay up"

swaymsg '[title="FOX Calculator"] floating enable' >/dev/null 2>&1
sleep 0.6

# The window is floating at its own size; sway reports where it put it
rect=$(swaymsg -t get_tree | python3 -c "
import json,sys
t=json.load(sys.stdin)
def walk(n):
    if n.get('name')=='FOX Calculator':
        r=n['rect']; print('%d %d %d %d'%(r['x'],r['y'],r['width'],r['height']))
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)")
[ -n "$rect" ] || fail "the calculator did not appear"
set -- $rect
wx=$1; wy=$2; ww=$3; wh=$4
echo "==> window at $wx,$wy ${ww}x${wh}"
[ "$ww" -eq 400 ] && [ "$wh" -eq 260 ] || fail "the window is ${ww}x${wh}, expected 400x260"

# Click the constant memory label; the surface has no server side decoration
ax=$((wx + 330))
ay=$((wy + 40))
echo "==> click the constant memory label at $ax,$ay"
"$OUT/wlxinject" "$ax" "$ay" move sleep:120 down sleep:120 up sleep:400 >/dev/null 2>&1
sleep 0.8

alive || fail "the application ended on the click"

frame=$(ls -t "$DUMP"/window-*.ppm 2>/dev/null | head -1)
[ -n "$frame" ] || fail "no frame was committed; nothing to compare"

echo "==> comparing $frame with $REF"
python3 - "$frame" "$REF" <<'EOF'
import sys, re
from PIL import Image
frame, ref = sys.argv[1], sys.argv[2]
a = Image.open(frame).convert("RGB")
b = Image.open(ref).convert("RGB")
print(f"frame {a.width}x{a.height}  reference {b.width}x{b.height}")
top = 0
m = re.search(r"-bar(\d+)", frame)
if m:
    top = int(m.group(1)); print(f"note: {top} rows of client side title bar are skipped")
    a = a.crop((0, top, a.width, a.height))
w, h = min(a.width, b.width), min(a.height, b.height)
pa, pb = a.load(), b.load()
differing = 0; worst = 0
for y in range(h):
    for x in range(w):
        ca, cb = pa[x, y], pb[x, y]
        d = max(abs(ca[0]-cb[0]), abs(ca[1]-cb[1]), abs(ca[2]-cb[2]))
        if d:
            differing += 1; worst = max(worst, d)
print(f"differing {differing} of {w*h} pixels, worst channel delta {worst}")
sys.exit(0 if differing == 0 else 1)
EOF
verdict=$?

if [ "$save" = yes ]; then
  mkdir -p -- "$ROOT/screenshots/tests/fox2/wayland"
  python3 - "$frame" "$ROOT/screenshots/tests/fox2/wayland/calculator.png" <<'EOF'
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
echo "PASS: the calculator frame matches the X11 screenshot"
exit 0
